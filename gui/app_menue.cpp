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
    std::vector<Datei> altStaende;   // rc608: die Lesestaende dieses Bildes; die Panels halten Verweise hinein
    Textpuffer puffer;
    std::string geladen;       // Text beim Oeffnen
    std::string gespeichert;   // Text beim letzten Speichern
    Datei datei;
    int geparst = -1;          // Pufferversion des Modells
    std::set<int> lesezeichen;
    std::set<int> gefaltet;    // Anfangszeilen eingeklappter Bloecke
    std::set<int> gesperrt;    // Items, die die Flaeche nicht bewegt
    std::set<std::pair<int, int>> verborgen;   // M2 Ebenenliste: (Menue, Item) nur in der Vorschau ausgeblendet
    int umbenennen = -1;       // Item, dessen Name in der Ebenenliste bearbeitet wird
    int umbenennenMenu = -1;   // rc608: das Menue dazu; der Index allein gilt in jedem Menue
    bool umbenennenNeu = false;   // rc608: den Fokus nur beim Anfang nehmen
    int umbenennenBild = 0;
    std::string neuerName;
    int seite = 0;
    int seiteGezeigt = -1;     // welcher Reiter zuletzt offen war
    std::vector<int> auswahl;  // Items im Menue der Seite
    float scrollZeile = 0;   // in Reihen (mit Umbruch mehr als Zeilen)
    float scrollX = 0;       // ohne Umbruch: seitwaerts (Bildpunkte)
    std::uint32_t letzterCursor = 0;
    int springeZeile = -1;
    std::map<std::string, std::string> bilder;   // eigene Grafiken: Spielpfad -> Bytes
    int satz = -1;             // M2: gehoert zum Menuesatz Nr. (menus.txt), -1 = einzeln
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

enum class Ziehen { Nichts, Verschieben, Groesse, Lineal };
// M7 Punkt 12: die Werte, die die Griffe am Modell aendern (app_menue_m7.inc)
struct GizmoWerte {
    int angle = 0;
    float mins[3] = {0, 0, 0}, maxs[3] = {0, 0, 0};
    float fovx = 0, fovy = 0;
};
struct DiagDaten;   // Schnappschuss der Spieldaten fuer die Diagnose (unten)

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
    int modellAufrufe = 0;                           // Selbsttest: Zeichenaufrufe der Menue-Figuren
    double modellMs = 0;                             // reste2 Punkt 7: Prozessorzeit der Figuren (Summe, ms)
    int modellZeichnungen = 0;                       // reste2 Punkt 7: gezeichnete Figuren (Summe)
    int modellStufen = 0;                            // M7: meiste Shaderstufen einer Flaeche (Chrom des JK2-Logos)
    std::string modellFehler;                        // letzte Meldung von gpu::zeichneMenueFigur
    Textur weiss;                                    // $whiteimage / *white (tr.whiteImage)
    unsigned weissStand = 0;
    int naechstesZiel = 0;                           // Nummer des naechsten Figurenziels
    std::map<std::string, Schrift> schriften;
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
    // M5b: Menuelisten aus dem Mod-Profil; bei den eingebauten wie bisher Jedi Academy zuerst
    const std::shared_ptr<const ModProfil> pr = aktivesProfil();
    const ModProfil& listen = profilEingebaut(*pr) ? eingebautesProfil(Spiel::JA) : *pr;
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

void neuParsen(Dokument& d) {
    if (d.geparst == d.puffer.version()) { return; }
    LeseOptionen o = leseOptionen(d.name);
    d.altStaende.push_back(std::move(d.datei));
    d.datei = lies(d.name, d.puffer.text(), o);
    d.geparst = d.puffer.version();
    if (d.datei.menus.empty()) { d.seite = 0; }
    const std::vector<Seite> s = seitenImMenue(d.datei);
    d.seite = std::clamp(d.seite, 0, std::max(0, static_cast<int>(s.size()) - 1));
    // rc608: Rueckgaengig nach dem Einfuegen oder Tippen im Text-Editor kann das gewaehlte Item nehmen (war ein Absturz)
    const int mi = s.empty() ? -1 : s[static_cast<std::size_t>(d.seite)].menu;
    const int anzahl = mi < 0 ? 0 : static_cast<int>(d.datei.menus[static_cast<std::size_t>(mi)].items.size());
    d.auswahl.erase(std::remove_if(d.auswahl.begin(), d.auswahl.end(), [anzahl](int i) { return i < 0 || i >= anzahl; }), d.auswahl.end());
    if (d.umbenennen >= anzahl) { d.umbenennen = -1; }
}

int menuDerSeite(const Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (s.empty()) { return -1; }
    return s[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(s.size()) - 1))].menu;
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

// Ein Spielpfad zu einer Datei auf der Platte: ab ".../ui/" (sonst ui/<name>)
std::string spielPfadVon(const std::string& platte) {
    const std::string k = kleinPfad(platte);
    const std::size_t p = k.rfind("/ui/");
    if (p != std::string::npos) { return platte.substr(p + 1); }
    return "ui/" + fs::path(platte).filename().string();
}

bool imSpielordner(const std::string& pfad) {
    std::error_code ec;
    const std::string k = kleinPfad(fs::weakly_canonical(fs::path(pfad), ec).string());
    auto pruefe = [&](const std::string& o) {
        if (o.empty()) { return false; }
        std::string g = kleinPfad(fs::weakly_canonical(fs::path(o), ec).string());
        // GameData-Ordner (eins ueber base) ebenfalls schuetzen
        const std::string eltern = kleinPfad(fs::path(g).parent_path().string());
        return k.compare(0, g.size(), g) == 0 || (!eltern.empty() && k.compare(0, eltern.size(), eltern) == 0);
    };
    for (const std::string& o : g_app->settings.gamePaths) {
        if (pruefe(o)) { return true; }
    }
    for (const std::string& o : g_app->settings.gamePathsJk2) {
        if (pruefe(o)) { return true; }
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
void textFeld(const char* k, std::string* wert, Schreiben schreiben) {
    Zustand& z = Z();
    const ImGuiID id = ImGui::GetID(k);
    if (z.halt.id == id) { *wert = z.halt.text; }
    const bool neu = eingabe(k, wert);
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

std::vector<Ersetzung> mitKopienAbgleich(const Dokument& d, std::vector<Ersetzung> e);   // M7 Punkt 3 (app_menue_m7.inc)

void aendere(std::vector<Ersetzung> e) {
    Dokument* d = doc();
    if (d == nullptr || e.empty()) { return; }
    e = mitKopienAbgleich(*d, std::move(e));
    d->puffer.ersetzeViele(std::move(e));
    neuParsen(*d);
}

// Was zu einem Rueckgaengig-Schritt gehoert: die Seite und die Auswahl
std::vector<int> anzeigeStand(const Dokument& d) {
    std::vector<int> v;
    v.reserve(d.auswahl.size() + 1);
    v.push_back(d.seite);
    v.insert(v.end(), d.auswahl.begin(), d.auswahl.end());
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
        if (setzeRect(d->datei, mi, i, r, x)) { e.insert(e.end(), x.begin(), x.end()); }
        else { ausMakroMelden(); }
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
    if (setzeEigenschaft(d->datei, menuDerSeite(*d), item, schluessel, werte, e)) { aendere(std::move(e)); }
    else { ausMakroMelden(); }
}

std::string farbText(const Farbe& c) {
    return zahlText(c.r) + " " + zahlText(c.g) + " " + zahlText(c.b) + " " + zahlText(c.a);
}

std::string inAnf(const std::string& s) { return "\"" + s + "\""; }

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
                if (static_cast<int>(m) != mi) {
                    const std::vector<Seite> s = seitenImMenue(d.datei);
                    for (std::size_t k = 0; k < s.size(); ++k) {
                        if (s[k].menu == static_cast<int>(m) && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
                    }
                }
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
    k.hoehe = [sf = u.schrift](float sc, int font, const std::string& name) {
        const Schrift* f = name.empty() ? sf(font) : schriftNamens(name);
        return f != nullptr ? schriftHoehe(*f, sc) : 0;
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
                    if (mv.nr < 0) { mv.nr = z.naechstesZiel++; }
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
                        const MenueModellBild mb = menueModellBild(mv.modell, it, jetzt, jetzt - animStart, pw, ph, mv.welt);
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

void zeichneBibliothek(Dokument& d);   // M2 (unten)
void zeichneTabLeiste(Dokument& d);   // M6 Punkt 2 (app_menue_m6.inc)
void zeichneGrenzenZeile(Dokument& d);   // M6 Punkt 10
void zeichneTabNummern(ImDrawList* dl, Dokument& d, int mi, const Abbildung& ab, ImVec2 o);
bool tabFolgeKlick(Dokument& d, int mi, float vx, float vy, const Umgebung& u);
// M7 Punkt 12 (app_menue_m7.inc)
bool gizmoVorschau(const Dokument& d, int mi, Datei& kopie);
bool gizmoKlick(Dokument& d, int mi, const Abbildung& ab, ImVec2 o, ImVec2 maus);
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
        if (d.satz >= 0) {
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
                    const std::vector<Seite> s = seitenImMenue(od.datei);
                    for (std::size_t w = 0; w < s.size(); ++w) {
                        if (s[w].menu == static_cast<int>(q) && s[w].cvar.empty()) { od.seite = static_cast<int>(w); }
                    }
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

void zeichneFlaeche(Dokument& d, ImVec2 groesse) {
    Zustand& z = Z();
    const int mi = menuDerSeite(d);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o0 = ImGui::GetCursorScreenPos();
    const float lineal = std::floor(ImGui::GetFontSize() * 1.2F);
    // Aufloesung: Fenster oder Voreinstellung (das Seitenverhaeltnis der Vorschau)
    static const float kAufl[][2] = {{0, 0}, {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3440, 1440}};
    // rc608 Punkt 8: rechts und unten ein schmaler Rand, sonst laegen die Griffe bei x = 640 und y = 480 halb ausserhalb
    const float griffRand = 6.0F;
    float bw = groesse.x - lineal - griffRand, bh = groesse.y - lineal - griffRand;
    if (z.aufloesung > 0) {
        const float ax = kAufl[z.aufloesung][0], ay = kAufl[z.aufloesung][1];
        const float f = std::min(bw / ax, bh / ay);
        bw = ax * f;
        bh = ay * f;
    }
    const ImVec2 o{o0.x + lineal, o0.y + lineal};
    const Abbildung ab = abbildung(bw, bh, z.seitenv);
    z.flaeche[0] = o.x + ab.ox;
    z.flaeche[1] = o.y + ab.oy;
    z.flaeche[2] = kSchirmBreite * ab.sx;
    z.flaeche[3] = kSchirmHoehe * ab.sy;
    dl->AddRectFilled(o, ImVec2{o.x + bw, o.y + bh}, IM_COL32(0, 0, 0, 255));
    ImGui::InvisibleButton("##flaeche", ImVec2{std::max(1.0F, groesse.x), std::max(1.0F, groesse.y)},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool hover = ImGui::IsItemHovered();
    const ImVec2 maus = ImGui::GetIO().MousePos;
    const float vx = (maus.x - o.x - ab.ox) / ab.sx;
    const float vy = (maus.y - o.y - ab.oy) / ab.sy;
    if (ImGui::BeginDragDropTarget()) {   // M2: Vorlage aus der Bibliothek
        if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("ME_VORLAGE")) {
            int n = 0;
            std::memcpy(&n, pl->Data, sizeof(n));
            setzeVorlageEin(static_cast<std::size_t>(std::max(0, n)), true, vx, vy);
        }
        ImGui::EndDragDropTarget();
    }
    if (mi < 0) {
        z.letztePrims = 0;
        return;
    }
    // Lineale
    {
        const ImU32 grund = IM_COL32(40, 40, 46, 255), strich = IM_COL32(150, 150, 160, 255);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o0.x + groesse.x, o.y}, grund);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o.x, o0.y + groesse.y}, grund);
        for (int v = 0; v <= 640; v += 10) {
            const float x = o.x + ab.x(static_cast<float>(v));
            const float l = (v % 50 == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{x, o.y - l}, ImVec2{x, o.y}, strich);
            if (v % 100 == 0) { dl->AddText(ImVec2{x + 2, o0.y}, strich, std::to_string(v).c_str()); }
        }
        for (int v = 0; v <= 480; v += 10) {
            const float y = o.y + ab.y(static_cast<float>(v));
            const float l = (v % 50 == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{o.x - l, y}, ImVec2{o.x, y}, strich);
            if (v % 100 == 0) { dl->AddText(ImVec2{o0.x + 1, y + 1}, strich, std::to_string(v).c_str()); }
        }
        if (hover) {
            dl->AddLine(ImVec2{maus.x, o0.y}, ImVec2{maus.x, o.y}, IM_COL32(255, 200, 0, 255));
            dl->AddLine(ImVec2{o0.x, maus.y}, ImVec2{o.x, maus.y}, IM_COL32(255, 200, 0, 255));
        }
    }
    dl->PushClipRect(o, ImVec2{o.x + bw, o.y + bh}, true);
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

    if (z.interaktion) {
        dl->PopClipRect();
        if (hover) {
            z.lauf.maus(mi, vx, vy);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { z.lauf.klick(mi, vx, vy); }
            if (ImGui::GetIO().MouseWheel != 0.0F) { z.lauf.rad(mi, vx, vy, static_cast<int>(ImGui::GetIO().MouseWheel)); }   // M4: Textscroll (Movie Duels)
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { z.lauf.escape(mi); }
        }
        if (satzWechsel()) { return; }   // M2 Punkt 3
        // Ein Menue, das ein Skript geoeffnet hat: dessen Seite zeigen
        for (std::size_t k = 0; k < z.lauf.menuFlags.size(); ++k) {
            if ((z.lauf.menuFlags[k] & WINDOW_HASFOCUS) != 0 && static_cast<int>(k) != mi) {
                const std::vector<Seite> s = seitenImMenue(d.datei);
                for (std::size_t q = 0; q < s.size(); ++q) {
                    if (s[q].menu == static_cast<int>(k) && s[q].cvar.empty()) { d.seite = static_cast<int>(q); }
                }
            }
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
    int unterMaus = alleUnter.empty() ? -1 : alleUnter.front();
    if (ImGui::GetIO().KeyAlt && !alleUnter.empty()) { unterMaus = naechstesUnter(alleUnter, d.auswahl); }
    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !alleUnter.empty()) {
        z.unterZeiger = alleUnter;
        ImGui::OpenPopup("##unterzeiger");
    }
    if (ImGui::BeginPopup("##unterzeiger")) {   // Rechtsklick: alles unter dem Zeiger
        for (const int i : z.unterZeiger) {
            if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
            const Item& it = m.items[static_cast<std::size_t>(i)];
            char l[300];
            std::snprintf(l, sizeof(l), "%s  (%s)##u%d", it.window.name.empty() ? "?" : it.window.name.c_str(), typName(it.type), i);
            if (ImGui::Selectable(l, std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end())) { waehle(i, false); }
        }
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
        if (z.ziehen == Ziehen::Groesse) {
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

    // Maus: Klick waehlt, Ziehen verschiebt/aendert die Groesse (M6: bei Tab-Reihenfolge waehlt er die naechste Nummer)
    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !tabFolgeKlick(d, mi, vx, vy, u) && !gizmoKlick(d, mi, ab, o, maus)) {
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
            d.auswahl.clear();
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
        if (z.ziehen == Ziehen::Verschieben && !z.ziehLaeuft && z.klickWahl >= 0) {
            d.auswahl.assign(1, z.klickWahl);
        } else if (z.ziehen == Ziehen::Verschieben && (z.zdx != 0 || z.zdy != 0)) {
            std::vector<std::pair<int, Rechteck>> neu;
            for (const int i : d.auswahl) {
                Rechteck r = clientRect(m, m.items[static_cast<std::size_t>(i)]);
                r.x += z.zdx;
                r.y += z.zdy;
                neu.emplace_back(i, r);
            }
            setzeRects(neu);
        } else if (z.ziehen == Ziehen::Groesse && z.ziehLaeuft) {
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
void editorTasten(Dokument& d) {
    Zustand& z = Z();
    const ImGuiIO& io = ImGui::GetIO();
    const int bild = ImGui::GetFrameCount();
    if (z.undoWunsch != 0) {
        const int wunsch = z.undoWunsch;
        z.undoWunsch = 0;
        (void)(wunsch < 0 ? rueckgaengig() : wiederholen());
    }
    if (io.WantTextInput || z.codeBild + 1 >= bild || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) { return; }
    const bool strg = io.KeyCtrl;
    if (strg && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) { (void)rueckgaengig(); }
    if (strg && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) { (void)wiederholen(); }
    if (z.werteBild + 1 >= bild) { return; }
    if (strg && ImGui::IsKeyPressed(ImGuiKey_V)) {
        if (const char* c = ImGui::GetClipboardText()) { fuegeItemsEin(utf8ZuLatin1(c)); }
    }
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
    for (const std::string& s : itemSchluessel(d.datei.dialekt, d.datei.spiel)) { k.push_back(s); }
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
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < breite) { ImGui::NewLine(); }
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
                case BefundArt::ZuVieleMenues: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagZuVieleMenues), b.a, b.b); return buf;
                case BefundArt::SkriptZuLang: std::snprintf(buf, sizeof(buf), trN(Str::MeDiagSkriptZuLang, b.a), b.a, b.b); return buf;
                case BefundArt::ZuVieleWerte:
                case BefundArt::ZuVieleFarben:
                case BefundArt::ZuVieleSpalten: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagZuViele), b.a, b.b); return buf;
                case BefundArt::NieSichtbar: return b.name == "rect" ? std::string(tr(Str::MeDiagNieRect)) : std::string(tr(Str::MeDiagNieVisible));
                case BefundArt::DateiFehlt: return fmt(Str::MeDiagDateiFehlt, anzeige(b.name), anzeige(b.wert));
                case BefundArt::UnbekanntIn: return fmt(Str::MeDiagUnbekanntIn, anzeige(b.name), anzeige(b.wert));
                case BefundArt::CinematicSp: return tr(Str::MeDiagCinematicSp);
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
                const std::vector<Seite> s = seitenImMenue(d.datei);
                for (std::size_t q = 0; q < s.size(); ++q) {
                    if (s[q].menu == x.menu && s[q].cvar.empty()) { d.seite = static_cast<int>(q); }
                }
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
std::string textFuerDatei(const std::string& sprache, const std::string& utf8, std::string* nichtDarstellbar = nullptr) {
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
std::vector<Pk3Datei> paketDateien(bool nurAenderungen) {
    Zustand& z = Z();
    std::vector<Pk3Datei> v;
    std::set<std::string> drin;
    for (const auto& od : z.docs) {
        std::string original;
        const bool ausSpiel = z.daten.lies(od->name, original);
        const bool geaendert = !ausSpiel || original != od->puffer.text();
        if ((!nurAenderungen || geaendert) && drin.insert(kleinPfad(od->name)).second) { v.push_back(Pk3Datei{od->name, od->puffer.text()}); }
        for (const auto& [p, b] : od->bilder) {
            if (drin.insert(kleinPfad(p)).second) { v.push_back(Pk3Datei{p, b}); }
        }
    }
    return v;
}

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
        if (ImGui::Button(tr(Str::MePruefen)) || !z.pruefGelaufen) { (void)pruefeAlleMenues(); }
        char buf[128];
        std::snprintf(buf, sizeof(buf), trN(Str::MePruefungAnzahl, z.pruefErgebnis.size()), static_cast<int>(z.pruefErgebnis.size()));
        ImGui::TextUnformatted(z.pruefErgebnis.empty() ? tr(Str::MePruefungOk) : buf);
        ImGui::BeginChild("##pruefliste");
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
            if (ImGui::Selectable(t.c_str())) {
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
    } else if (!writePk3(plan.pk3, paketDateien(true), &fehler)) {
        z.meldung = fehler;
        return {};
    }
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
    // M2: Festbreitenschrift (Spalten, Einrueckung und Minimap stehen untereinander)
    if (g_codeSchrift != nullptr) { ImGui::PushFont(g_codeSchrift, 0.0F); }
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
    if (hover && io.MousePos.x >= textX && io.MousePos.x < textX + textB) { ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput); }
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
    // ohne Umbruch: wie weit seitwaerts (duenner Balken unten)
    if (!umbruch && breiteste > textB) {
        const float a = d.scrollX / breiteste, b = std::min(1.0F, (d.scrollX + textB) / breiteste);
        dl->AddRectFilled(ImVec2{textX + a * textB, o.y + bereich.y - 3}, ImVec2{textX + b * textB, o.y + bereich.y}, IM_COL32(255, 255, 255, 60));
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
    if (g_codeSchrift != nullptr) { ImGui::PopFont(); }
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

// Ins Paket: video/<name>.roq, der Shader menu/video/<name> in shaders/<paket>.shader,
// dann je nach Verwendung ans Menue.
void videoAblegen(const std::string& name, const std::string& roq, VideoVerwendung verwendung) {
    Dokument* d = doc();
    if (d == nullptr || roq.empty()) { return; }
    Zustand& z = Z();
    const std::string video = "video/" + name + ".roq";
    const std::string shader = "menu/video/" + name;
    d->bilder[video] = roq;
    z.kinos.clear();
    const std::string datei = paketShaderDatei(*d);
    std::string& text = d->bilder[datei];
    ShaderMap da;
    parseShaderScript(text, da);
    if (da.count(shader) == 0) { text += (text.empty() || text.back() == '\n' ? "" : "\n") + videoShaderText(shader, video); }
    shaderLaden();
    parseShaderScript(text, z.shader);
    z.texturen.erase(shader);
    const int mi = menuDerSeite(*d);
    if (verwendung == VideoVerwendung::Hintergrund && d->auswahl.size() == 1) {
        const int it = d->auswahl[0];
        eigenschaft(it, "style", {konstante(d->datei, "WINDOW_STYLE_SHADER", WINDOW_STYLE_SHADER)});
        eigenschaft(it, "background", {inAnf(shader)});
    } else if (verwendung == VideoVerwendung::NeuesElement && mi >= 0) {
        fuegeEin(-1);
        const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
        const int it = static_cast<int>(m.items.size()) - 1;
        if (it >= 0) {
            eigenschaft(it, "name", {freierName(m, "video_" + name)});
            eigenschaft(it, "rect", {zahlText(-m.window.rect.x) + " " + zahlText(-m.window.rect.y) + " 640 480"});
            eigenschaft(it, "background", {inAnf(shader)});
            (void)setzeAnStelle(*d, mi, it, 0);   // ganz nach hinten: unter allen anderen Items
        }
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
        // Verwendung im Menue
        ImGui::SeparatorText(tr(Str::MeVideoVerwendung));
        Dokument* d = doc();
        const bool eins = d != nullptr && d->auswahl.size() == 1;
        auto wahl = [&](Str text, Str tip, VideoVerwendung v, bool geht) {
            ImGui::BeginDisabled(!geht);
            if (ImGui::RadioButton(tr(text), z.videoVerwendung == v)) { z.videoVerwendung = v; }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26);
                ImGui::TextUnformatted(tr(tip));
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        };
        wahl(Str::MeVideoAlsHintergrund, Str::MeVideoAlsHintergrundTip, VideoVerwendung::Hintergrund, eins);
        wahl(Str::MeVideoNeuesElement, Str::MeVideoNeuesElementTip, VideoVerwendung::NeuesElement, d != nullptr);
        wahl(Str::MeVideoAlsCinematic, Str::MeVideoAlsCinematicTip, VideoVerwendung::Cinematic, eins);
        wahl(Str::MeVideoNurPaket, Str::MeVideoNurPaketTip, VideoVerwendung::NurPaket, true);
        if (!eins && (z.videoVerwendung == VideoVerwendung::Hintergrund || z.videoVerwendung == VideoVerwendung::Cinematic)) {
            z.videoVerwendung = VideoVerwendung::NeuesElement;
        }
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
bool bauteilKachel(const char* name, int typ, ImVec2 groesse) {
    const bool geklickt = ImGui::InvisibleButton(name, groesse);
    const bool ueber = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const ImGuiStyle& st = ImGui::GetStyle();
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ueber ? (ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_FrameBg),
                      st.FrameRounding + 2.0F);
    const float fs = ImGui::GetFontSize();
    const ImU32 c = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 akzent = ImGui::GetColorU32(ImGuiCol_CheckMark);
    // das Bild der Art im oberen Teil
    const ImVec2 m{(a.x + b.x) * 0.5F, a.y + (b.y - a.y) * 0.38F};
    const float r = std::min(groesse.x, groesse.y) * 0.22F;
    switch (typ) {
        case ITEM_TYPE_TEXT: dl->AddText(nullptr, fs * 1.3F, ImVec2{m.x - fs * 0.75F, m.y - fs * 0.65F}, c, "Aa"); break;
        case ITEM_TYPE_BUTTON:
            dl->AddRectFilled(ImVec2{m.x - r * 1.6F, m.y - r * 0.6F}, ImVec2{m.x + r * 1.6F, m.y + r * 0.6F}, akzent, r * 0.3F);
            break;
        case -1:   // Bild
        case -2:   // Bild hinzufuegen
            dl->AddRect(ImVec2{m.x - r * 1.4F, m.y - r}, ImVec2{m.x + r * 1.4F, m.y + r}, c, 2.0F);
            dl->AddTriangleFilled(ImVec2{m.x - r * 1.2F, m.y + r * 0.8F}, ImVec2{m.x - r * 0.3F, m.y - r * 0.3F}, ImVec2{m.x + r * 0.6F, m.y + r * 0.8F}, c);
            dl->AddCircleFilled(ImVec2{m.x + r * 0.7F, m.y - r * 0.4F}, r * 0.25F, akzent);
            if (typ == -2) { dl->AddText(ImVec2{m.x + r * 1.2F, m.y - r * 1.6F}, akzent, "+"); }
            break;
        case ITEM_TYPE_YESNO:
            dl->AddRectFilled(ImVec2{m.x - r * 1.2F, m.y - r * 0.55F}, ImVec2{m.x + r * 1.2F, m.y + r * 0.55F}, akzent, r * 0.55F);
            dl->AddCircleFilled(ImVec2{m.x + r * 0.65F, m.y}, r * 0.42F, c);
            break;
        case ITEM_TYPE_MULTI: dl->AddText(ImVec2{m.x - fs * 1.1F, m.y - fs * 0.5F}, c, "< A >"); break;
        case ITEM_TYPE_SLIDER:
            dl->AddLine(ImVec2{m.x - r * 1.6F, m.y}, ImVec2{m.x + r * 1.6F, m.y}, c, 2.0F);
            dl->AddRectFilled(ImVec2{m.x - r * 0.2F, m.y - r * 0.6F}, ImVec2{m.x + r * 0.2F, m.y + r * 0.6F}, akzent, 2.0F);
            break;
        case ITEM_TYPE_EDITFIELD:
            dl->AddRect(ImVec2{m.x - r * 1.6F, m.y - r * 0.55F}, ImVec2{m.x + r * 1.6F, m.y + r * 0.55F}, c, 2.0F);
            dl->AddLine(ImVec2{m.x - r * 1.1F, m.y - r * 0.35F}, ImVec2{m.x - r * 1.1F, m.y + r * 0.35F}, akzent, 2.0F);
            break;
        case ITEM_TYPE_LISTBOX:
            dl->AddRect(ImVec2{m.x - r * 1.4F, m.y - r}, ImVec2{m.x + r * 1.4F, m.y + r}, c, 2.0F);
            for (int k = 0; k < 3; ++k) {
                const float y = m.y - r * 0.5F + static_cast<float>(k) * r * 0.5F;
                dl->AddLine(ImVec2{m.x - r * 1.1F, y}, ImVec2{m.x + r * 0.9F, y}, k == 1 ? akzent : c, 1.5F);
            }
            break;
        case ITEM_TYPE_MODEL: {
            const float s = r * 0.8F;
            dl->AddRect(ImVec2{m.x - s, m.y - s * 0.6F}, ImVec2{m.x + s * 0.6F, m.y + s}, c, 0.0F, 0, 1.5F);
            dl->AddLine(ImVec2{m.x - s, m.y - s * 0.6F}, ImVec2{m.x - s * 0.6F, m.y - s}, c, 1.5F);
            dl->AddLine(ImVec2{m.x + s * 0.6F, m.y - s * 0.6F}, ImVec2{m.x + s, m.y - s}, c, 1.5F);
            dl->AddLine(ImVec2{m.x + s * 0.6F, m.y + s}, ImVec2{m.x + s, m.y + s * 0.6F}, c, 1.5F);
            dl->AddLine(ImVec2{m.x - s * 0.6F, m.y - s}, ImVec2{m.x + s, m.y - s}, c, 1.5F);
            dl->AddLine(ImVec2{m.x + s, m.y - s}, ImVec2{m.x + s, m.y + s * 0.6F}, c, 1.5F);
            break;
        }
        case ITEM_TYPE_BIND:
            dl->AddRect(ImVec2{m.x - r * 0.8F, m.y - r * 0.8F}, ImVec2{m.x + r * 0.8F, m.y + r * 0.8F}, c, r * 0.3F, 0, 1.5F);
            dl->AddText(ImVec2{m.x - fs * 0.3F, m.y - fs * 0.5F}, akzent, "K");
            break;
        default: break;
    }
    // der Name darunter, mittig, abgeschnitten
    const char* ende = ImGui::FindRenderedTextEnd(name);
    const float tw = ImGui::CalcTextSize(name, ende).x;
    dl->PushClipRect(a, b, true);
    dl->AddText(ImVec2{std::max(a.x + 3, (a.x + b.x - tw) * 0.5F), b.y - fs * 1.35F}, c, name, ende);
    dl->PopClipRect();
    if (ueber) { ImGui::SetTooltip("%s", name); }
    return geklickt;
}

void zeichnePalette(Dokument& d) {
    struct W {
        int typ;
        Str name;
    };
    static const W w[] = {{ITEM_TYPE_TEXT, Str::MeWText},       {ITEM_TYPE_BUTTON, Str::MeWButton}, {-1, Str::MeWBild},
                          {ITEM_TYPE_YESNO, Str::MeWJaNein},     {ITEM_TYPE_MULTI, Str::MeWMulti},   {ITEM_TYPE_SLIDER, Str::MeWSchieber},
                          {ITEM_TYPE_EDITFIELD, Str::MeWEingabe}, {ITEM_TYPE_LISTBOX, Str::MeWListe}, {ITEM_TYPE_MODEL, Str::MeWModell},
                          {ITEM_TYPE_BIND, Str::MeWTaste}};
    ImGui::SeparatorText(tr(Str::MePalette));
    const float fs = ImGui::GetFontSize();
    const float abstand = ImGui::GetStyle().ItemSpacing.x;
    const float breit = ImGui::GetContentRegionAvail().x;
    const int spalten = std::max(1, static_cast<int>((breit + abstand) / (fs * 5.2F + abstand)));
    const ImVec2 kachel{(breit - abstand * static_cast<float>(spalten - 1)) / static_cast<float>(spalten), fs * 3.6F};
    int nr = 0;
    auto weiter = [&]() {
        if (++nr % spalten != 0) { ImGui::SameLine(); }
    };
    for (const W& x : w) {
        if (bauteilKachel(tr(x.name), x.typ, kachel)) { fuegeEin(x.typ); }
        weiter();
    }
    if (bauteilKachel(tr(Str::MeBildDazu), -2, kachel)) {
        const std::string pfad = platform::openFileDialog(tr(Str::MeBildDazu), "Images (*.png;*.jpg;*.tga)|*.png;*.jpg;*.jpeg;*.tga|", "");
        if (!pfad.empty()) {
            std::ifstream f(utf8Pfad(pfad), std::ios::binary);
            std::ostringstream s;
            s << f.rdbuf();
            std::string base = u8(utf8Pfad(pfad).stem());
            for (char& c : base) { c = (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : '_'; }
            const std::string ext = kleinPfad(u8(utf8Pfad(pfad).extension()));
            const std::string spiel = "gfx/menus/" + base + (ext == ".jpeg" ? std::string(".jpg") : ext);
            d.bilder[spiel] = s.str();
            Z().texturen.erase("gfx/menus/" + base);
            fuegeEin(-1);
            const int mi = menuDerSeite(d);
            if (mi >= 0 && !d.datei.menus[static_cast<std::size_t>(mi)].items.empty()) {
                eigenschaft(static_cast<int>(d.datei.menus[static_cast<std::size_t>(mi)].items.size()) - 1, "background",
                            {inAnf("gfx/menus/" + base)});
            }
            char b[400];
            std::snprintf(b, sizeof(b), tr(Str::MeBildHinzu), u8(utf8Pfad(pfad).filename()).c_str(), spiel.c_str());
            Z().meldung = b;
        }
    }
    zeichneBibliothek(d);
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
void setzeVorlageEin(std::size_t nr, bool mitZiel, float zx, float zy) {
    Dokument* d = doc();
    const std::vector<Vorlage>& v = vorlagen();
    if (d == nullptr || nr >= v.size()) { return; }
    const std::string vorher = d->puffer.text();
    fuegeItemsEin(v[nr].text);
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    if (mi < 0 || d->auswahl.empty()) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    if (mitZiel) {   // linke obere Ecke der eingesetzten Items an die Stelle des Loslassens
        float lx = 1e9F, ly = 1e9F;
        for (const int i : d->auswahl) {
            if (i >= 0 && static_cast<std::size_t>(i) < m.items.size()) {
                lx = std::min(lx, m.items[static_cast<std::size_t>(i)].window.rect.x);
                ly = std::min(ly, m.items[static_cast<std::size_t>(i)].window.rect.y);
            }
        }
        if (lx < 1e8F) { verschiebeAuswahl(std::round(zx - lx), std::round(zy - ly)); }
    }
    // Detailbereich: onOpen oeffnet Reiter 0 / Unterreiter 0
    if (v[nr].id == "detail") {
        neuParsen(*d);
        const Menu& mm = d->datei.menus[static_cast<std::size_t>(mi)];
        const DetailOptionen o = beispielDetail();
        if (mm.onOpen.find(o.cvarSel) == std::string::npos) {
            std::vector<Ersetzung> e;
            if (setzeEigenschaft(d->datei, mi, -1, "onOpen", {"{ " + mm.onOpen + detailOnOpen(o) + " }"}, e)) { aendere(std::move(e)); }
        }
    }
    (void)zuEinemSchritt(vorher);   // rc608: Einfuegen, an die Stelle ruecken und onOpen sind eine Handlung
    neuParsen(*d);
}

void zeichneBibliothek(Dokument& d) {
    Zustand& z = Z();
    ImGui::SeparatorText(tr(Str::MeBibliothek));
    const std::vector<Vorlage>& v = vorlagen();
    for (std::size_t i = 0; i < v.size(); ++i) {
        static const Str namen[] = {Str::MeBtKnopf, Str::MeBtSchieber, Str::MeBtJaNein, Str::MeBtReiter, Str::MeBtDetail, Str::MeBtListe};
        static const char* const ids[] = {"knopf", "schieber", "janein", "reiter", "detail", "liste"};
        std::string anzeigeName = anzeige(v[i].name);
        for (int k = 0; k < 6; ++k) {
            if (!v[i].eigen && v[i].id == ids[k]) { anzeigeName = tr(namen[k]); }
        }
        if (v[i].eigen) { anzeigeName = "* " + anzeigeName; }
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(anzeigeName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            setzeVorlageEin(i, false, 0, 0);
        }
        if (ImGui::BeginDragDropSource()) {   // auf die Flaeche ziehen
            const int n = static_cast<int>(i);
            ImGui::SetDragDropPayload("ME_VORLAGE", &n, sizeof(n));
            ImGui::TextUnformatted(anzeigeName.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
    // uebersetzung1: umbrechen - der Hinweis war schon auf Englisch breiter als die Spalte
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled("%s", tr(Str::MeBibliothekHinweis));
    ImGui::PopTextWrapPos();
    // Aus der Auswahl eine Vorlage speichern
    ImGui::BeginDisabled(d.auswahl.empty());
    ImGui::SetNextItemWidth(-1);
    eingabe("##vorlagename", &z.vorlageName, 0, tr(Str::MeVorlageName));
    if (ImGui::Button(tr(Str::MeVorlageSpeichern), ImVec2{-1, 0}) && !z.vorlageName.empty()) {
        if (speichereVorlage(vorlagenOrdner(), z.vorlageName, kopiereAuswahl())) {
            z.vorlagenGeladen = false;
            z.vorlageName.clear();
        }
    }
    ImGui::EndDisabled();
}

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
    const Menu& me = d.datei.menus[static_cast<std::size_t>(menu)];
    const Item& it = me.items[static_cast<std::size_t>(i)];
    ImGui::PushID(i);
    const bool aufSeite = menuDerSeite(d) == menu;
    const bool verborgen = d.verborgen.count({menu, i}) != 0;
    if (ebenenSymbol("##auge", verborgen ? UiIcon::Versteckt : UiIcon::Sichtbar, !verborgen, tr(Str::MeEbeneAuge))) {
        if (verborgen) { d.verborgen.erase({menu, i}); } else { d.verborgen.insert({menu, i}); }
    }
    ImGui::SameLine();
    const bool gesperrt = aufSeite && d.gesperrt.count(i) != 0;
    if (ebenenSymbol("##schloss", gesperrt ? UiIcon::Schloss : UiIcon::SchlossOffen, gesperrt, tr(Str::MeEbeneSchloss))) {
        if (gesperrt) { d.gesperrt.erase(i); } else { d.gesperrt.insert(i); }
    }
    ImGui::SameLine();
    if (d.umbenennen == i && d.umbenennenMenu == menu) {
        // rc608: nimmt den Fokus nur am Anfang. Es endet mit Enter, Esc (ohne Aenderung) oder mit dem Klick
        // daneben, also auch mit jedem Auswahl- oder Reiterwechsel; bis dahin Getipptes zaehlt.
        d.umbenennenBild = ImGui::GetFrameCount();
        ImGui::SetNextItemWidth(-1);
        if (d.umbenennenNeu) {
            ImGui::SetKeyboardFocusHere();
            d.umbenennenNeu = false;
        }
        const std::string alt = it.window.name;
        Dokument* dp = &d;
        textFeld("##neu", &d.neuerName, [dp, menu, i, alt](const std::string& t) {
            if (t.empty() || utf8ZuLatin1(t.c_str()) == alt) { return; }
            std::vector<Ersetzung> e;
            if (setzeEigenschaft(dp->datei, menu, i, "name", {inAnf(utf8ZuLatin1(t.c_str()))}, e)) { aendere(std::move(e)); }
            else { ausMakroMelden(); }
        });
        if (ImGui::IsItemDeactivated()) { d.umbenennen = -1; }
    } else {
        const bool sel = aufSeite && std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end();
        char l[300];
        std::snprintf(l, sizeof(l), "%s  %s", it.window.name.empty() ? "?" : anzeige(it.window.name).c_str(), typName(it.type));
        if (ImGui::Selectable(l, sel, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (!aufSeite) {
                const std::vector<Seite> s = seitenImMenue(d.datei);
                for (std::size_t k = 0; k < s.size(); ++k) {
                    if (s[k].menu == menu && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
                }
            }
            waehle(i, ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                d.umbenennen = i;
                d.umbenennenMenu = menu;
                d.umbenennenNeu = true;
                d.umbenennenBild = ImGui::GetFrameCount();
                d.neuerName = anzeige(it.window.name);
            }
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
    ImGui::PopID();
}

void zeichneGliederung(Dokument& d) {
    ImGui::SeparatorText(tr(Str::MeGliederung));
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        const Menu& me = d.datei.menus[m];
        ImGui::PushID(static_cast<int>(m));
        const bool offen = ImGui::TreeNodeEx("##m", ImGuiTreeNodeFlags_DefaultOpen, "menuDef %s%s", me.window.name.c_str(), me.ok ? "" : " (!)");
        if (ImGui::IsItemClicked()) {
            springe(d, me.anfang);
            const std::vector<Seite> s = seitenImMenue(d.datei);
            for (std::size_t k = 0; k < s.size(); ++k) {
                if (s[k].menu == static_cast<int>(m) && s[k].cvar.empty()) { d.seite = static_cast<int>(k); }
            }
        }
        if (offen) {
            // M2 Ebenenliste (wie der Object Inspector): oben = vorne (zuletzt gezeichnet),
            // Gruppen (group) als Knoten; Auge, Schloss, Doppelklick = umbenennen, Ziehen = Reihenfolge
            std::set<std::string> gruppenGezeigt;
            for (std::size_t r = me.items.size(); r-- > 0;) {
                const std::string& g = me.items[r].window.group;
                if (g.empty()) {
                    zeichneEbene(d, static_cast<int>(m), static_cast<int>(r));
                    continue;
                }
                if (!gruppenGezeigt.insert(g).second) { continue; }
                if (ImGui::TreeNodeEx(("##g" + g).c_str(), ImGuiTreeNodeFlags_DefaultOpen, "group %s", anzeige(g).c_str())) {
                    for (std::size_t q = me.items.size(); q-- > 0;) {
                        if (me.items[q].window.group == g) { zeichneEbene(d, static_cast<int>(m), static_cast<int>(q)); }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void zeichneEigenschaften(Dokument& d) {
    Zustand& z = Z();
    if (!ImGui::CollapsingHeader(tr(Str::MeEigenschaften), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MeNichtsGewaehlt));
        return;
    }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    if (d.auswahl.size() > 1) {
        ImGui::Text(trN(Str::MeAuswahlItems, d.auswahl.size()), static_cast<int>(d.auswahl.size()));
        auto richte = [&](Ausrichtung a) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            ausrichten(r, a);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        if (ImGui::Button(tr(Str::MeLinks))) { richte(Ausrichtung::Links); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeMitteH))) { richte(Ausrichtung::MitteH); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeRechts))) { richte(Ausrichtung::Rechts); }
        if (ImGui::Button(tr(Str::MeOben))) { richte(Ausrichtung::Oben); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeMitteV))) { richte(Ausrichtung::MitteV); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeUnten))) { richte(Ausrichtung::Unten); }
        auto verteile = [&](bool w) {
            std::vector<Rechteck> r;
            for (const int i : d.auswahl) { r.push_back(m.items[static_cast<std::size_t>(i)].window.rectClient); }
            verteilen(r, w);
            std::vector<std::pair<int, Rechteck>> neu;
            for (std::size_t k = 0; k < r.size(); ++k) { neu.emplace_back(d.auswahl[k], r[k]); }
            setzeRects(neu);
        };
        if (ImGui::Button(tr(Str::MeVerteilenH))) { verteile(true); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeVerteilenV))) { verteile(false); }
        static std::string gruppe;
        ImGui::SetNextItemWidth(-ImGui::GetFontSize() * 6);
        eingabe("##gruppe", &gruppe);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeGruppieren)) && !gruppe.empty()) {
            std::vector<Ersetzung> e;
            for (const int i : d.auswahl) { (void)setzeEigenschaft(d.datei, mi, i, "group", {gruppe}, e); }
            aendere(std::move(e));
        }
        return;
    }
    const int i = d.auswahl[0];
    if (static_cast<std::size_t>(i) >= m.items.size()) { return; }
    const Item& it = m.items[static_cast<std::size_t>(i)];
    ImGui::PushID(i);
    std::string name = it.window.name;
    textFeld("name", &name, [i](const std::string& t) {
        if (!t.empty()) { eigenschaft(i, "name", {inAnf(t)}); }
    });
    std::string gruppe = it.window.group;
    textFeld("group", &gruppe, [i](const std::string& t) { eigenschaft(i, "group", {inAnf(t)}); });
    // type / style
    // rc608 (shank: "statt das er den type immer behaelt"): nur Anzeige. Die Schluessel eines Typs gelten beim
    // anderen nicht, und das Spiel verwirft dann das ganze Menue.
    std::string typText = typName(it.type);
    ImGui::BeginDisabled();
    ImGui::InputText("type", typText.data(), typText.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(Str::MeTypFestTip)); }
    const int sty = std::clamp(it.window.style, 0, 5);
    if (ImGui::BeginCombo("style", styleName(sty))) {
        for (int t = 0; t < 6; ++t) {
            if (ImGui::Selectable(styleName(t), t == sty)) { eigenschaft(i, "style", {konstante(d.datei, styleName(t), t)}); }
        }
        ImGui::EndCombo();
    }
    float r[4] = {it.window.rectClient.x, it.window.rectClient.y, it.window.rectClient.w, it.window.rectClient.h};
    zahlenFeld("rect", r, 4, [&r] { return ImGui::DragFloat4("rect", r, 1.0F); },
               [i](const std::vector<float>& w) { setzeRects({{i, Rechteck{w[0], w[1], w[2], w[3]}}}); });
    std::string text = it.text;
    textFeld("text", &text, [i](const std::string& t) { eigenschaft(i, "text", {inAnf(t)}); });
    if (!it.text.empty() && it.text[0] == '@') {
        // fehlt der Schluessel, zeigt das Spiel nichts (SE_GetString liefert ""): so sagen statt den Schluessel zu wiederholen
        ImGui::TextDisabled("= %s", texte().kennt(it.text) ? anzeige(texte().aufloesen(it.text)).c_str() : tr(Str::MeTextFehltImSpiel));
    }
    float ts = it.textscale;
    zahlenFeld("textscale", &ts, 1, [&ts] { return ImGui::DragFloat("textscale", &ts, 0.01F, 0.05F, 4.0F); },
               [i](const std::vector<float>& w) { eigenschaft(i, "textscale", {zahlText(w[0])}); });
    float ta[2] = {it.textalignx, it.textaligny};
    zahlenFeld("textalignx/y", ta, 2, [&ta] { return ImGui::DragFloat2("textalignx/y", ta, 1.0F); }, [&d, mi, i](const std::vector<float>& w) {
        std::vector<Ersetzung> e;
        (void)setzeEigenschaft(d.datei, mi, i, "textalignx", {zahlText(w[0])}, e);
        (void)setzeEigenschaft(d.datei, mi, i, "textaligny", {zahlText(w[1])}, e);
        aendere(std::move(e));
    });
    static const char* const ausr[] = {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT"};
    if (ImGui::BeginCombo("textalign", ausr[std::clamp(it.textalignment, 0, 2)])) {
        for (int t = 0; t < 3; ++t) {
            if (ImGui::Selectable(ausr[t], t == it.textalignment)) { eigenschaft(i, "textalign", {konstante(d.datei, ausr[t], t)}); }
        }
        ImGui::EndCombo();
    }
    auto farbe = [&](const char* k, const Farbe& f) {
        float c[4] = {f.r, f.g, f.b, f.a};
        zahlenFeld(k, c, 4, [&c, k] { return ImGui::ColorEdit4(k, c, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar); },
                   [i, k](const std::vector<float>& w) { eigenschaft(i, k, {farbText(Farbe{w[0], w[1], w[2], w[3]})}); });
    };
    farbe("forecolor", it.window.foreColor);
    farbe("backcolor", it.window.backColor);
    farbe("bordercolor", it.window.borderColor);
    float border = static_cast<float>(it.window.border);
    zahlenFeld("border", &border, 1, [&border] {
        int b = static_cast<int>(border);
        const bool neu = ImGui::InputInt("border", &b, 1, 1);
        border = static_cast<float>(b);
        return neu;
    }, [i](const std::vector<float>& w) { eigenschaft(i, "border", {std::to_string(static_cast<int>(w[0]))}); });
    std::string bg = it.window.background;
    textFeld("background", &bg, [i](const std::string& t) { eigenschaft(i, "background", {inAnf(t)}); });
    std::string cvar = it.cvar;
    textFeld("cvar", &cvar, [i](const std::string& t) { eigenschaft(i, "cvar", {inAnf(t)}); });
    bool sichtbar = (it.window.flags & WINDOW_VISIBLE) != 0;
    if (ImGui::Checkbox("visible", &sichtbar)) { eigenschaft(i, "visible", {sichtbar ? "1" : "0"}); }
    bool gesperrt = d.gesperrt.count(i) != 0;
    if (ImGui::Checkbox(tr(Str::MeSperren), &gesperrt)) {
        if (gesperrt) { d.gesperrt.insert(i); } else { d.gesperrt.erase(i); }
    }
    if (ImGui::Button(tr(Str::MeNachVorn))) {
        std::vector<Ersetzung> e;
        if (itemVerschieben(d.datei, mi, i, true, e)) {
            aendere(std::move(e));
            d.auswahl.assign(1, i + 1);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MeNachHinten))) {
        std::vector<Ersetzung> e;
        if (itemVerschieben(d.datei, mi, i, false, e)) {
            aendere(std::move(e));
            d.auswahl.assign(1, i - 1);
        }
    }
    if (ImGui::Button(tr(Str::ActClone))) { dupliziereAuswahl(); }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::ActDelete))) {
        loescheAuswahl();
        ImGui::PopID();
        return;
    }
    if (!it.action.empty()) { ImGui::TextWrapped("action: %s", anzeige(it.action).c_str()); }
    (void)z;
    ImGui::PopID();
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
    if (!ImGui::CollapsingHeader(tr(Str::MeCvars), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    ImGui::TextDisabled("%s", tr(Str::MeCvarsNurVorschau));
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
        ImGui::SameLine();
        if (c.sichtbarkeit) { ImGui::TextColored(ImVec4{0.4F, 0.8F, 1.0F, 1.0F}, "%s", c.name.c_str()); }
        else { ImGui::TextUnformatted(c.name.c_str()); }
        if (ImGui::IsItemHovered() && !c.wofuer.empty()) {
            std::string t;
            for (const std::string& x : c.wofuer) { t += x + "\n"; }
            ImGui::SetTooltip("%s", t.c_str());
        }
        for (const std::string& v : c.werte) {
            ImGui::SameLine();
            if (ImGui::SmallButton(v.c_str())) { setzeCvar(c.name, v); }
        }
        ImGui::PopID();
    }
}

void zeichneMeldungen(Dokument& d) {
    Zustand& z = Z();
    if (!ImGui::CollapsingHeader(tr(Str::MeMeldungen), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    for (std::size_t k = 0; k < d.datei.meldungen.size(); ++k) {
        const Meldung& m = d.datei.meldungen[k];
        ImGui::PushID(static_cast<int>(k));
        const std::string t = meldungText(d.datei, m);
        ImGui::PushStyleColor(ImGuiCol_Text, m.fehler ? ImVec4{1, 0.45F, 0.45F, 1} : ImVec4{1, 0.85F, 0.4F, 1});
        if (ImGui::Selectable(t.c_str()) && m.datei == 0 && m.zeile > 0) { springe(d, d.puffer.zeilenAnfang(m.zeile - 1)); }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    if (z.interaktion && ImGui::CollapsingHeader(tr(Str::MeProtokoll), ImGuiTreeNodeFlags_DefaultOpen)) {
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

void zeichneLeiste(Dokument* d) {
    Zustand& z = Z();
    zeichneProjektKnopf();   // M5b Punkt 3
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeAusSpiel), UiIcon::Liste, tr(Str::MeAusSpielHint))) { z.spielListeOffen = true; }
    zeichneSpielListe();
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeMenuesatzOeffnen), UiIcon::Explorer, tr(Str::MeTipMenuesatz))) { (void)oeffneMenuesatzFuer(d); }   // M2 Punkt 3
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeDateiOeffnen), UiIcon::Oeffnen, tr(Str::MeDateiOeffnenHint))) {
        const std::string pfad = platform::openFileDialog(tr(Str::MeDateiOeffnen), "Menu (*.menu;*.txt;*.jksmenu)|*.menu;*.txt;*.jksmenu|", "");
        if (!pfad.empty()) { (void)oeffneDatei(pfad); }
    }
    if (d != nullptr) {
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeSpeichern), UiIcon::Speichern, tr(z.projektDatei.empty() ? Str::MeSpeichernHint : Str::MePrjSpeichernTip))) {
            if (!z.projektDatei.empty()) {   // M5b: im Projekt speichert der Knopf das ganze Projekt
                (void)speichereProjekt();
            } else if (d->platte.empty() || imSpielordner(d->platte)) {
                const std::string ziel = platform::saveFileDialog(tr(Str::MeSpeichern), "Menu (*.menu)|*.menu|", z.ordnerAusgabe,
                                                                  fs::path(d->name).filename().string());
                if (!ziel.empty()) { speichern(*d, ziel); }
            } else {
                speichern(*d, d->platte);
            }
        }
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
                z.videoOffen = true;
            }
        }
        zeichneVideoFenster();
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
                const std::string ziel = platform::saveFileDialog(tr(Str::MeExportPk3), "pk3 (*.pk3)|*.pk3|", z.ordnerAusgabe, "zz_menu.pk3");
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
    // Ansicht
    int sv = z.seitenv == Seitenverhaeltnis::Strecken ? 0 : 1;
    const char* svn[] = {tr(Str::MeStrecken), tr(Str::MeBalken)};
    // uebersetzung1: so breit wie der laengste Eintrag (cs "Roztáhnout (originál)" war abgeschnitten)
    ImGui::SetNextItemWidth(std::max(ImGui::CalcTextSize(svn[0]).x, ImGui::CalcTextSize(svn[1]).x) + ImGui::GetFrameHeight() +
                            ImGui::GetStyle().FramePadding.x * 2.0F);
    if (ImGui::Combo("##sv", &sv, svn, 2)) { z.seitenv = sv == 0 ? Seitenverhaeltnis::Strecken : Seitenverhaeltnis::Balken; }
    ImGui::SameLine();
    const char* const aufl[] = {tr(Str::MeAufloesungFenster), "640x480", "800x600", "1024x768", "1280x720", "1366x768", "1600x900", "1920x1080", "2560x1440", "3440x1440"};
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
        return "\tmenuDef" + nl + "\t{" + nl + "\t\tname\t\t\t\"" + name + "\"" + nl + "\t\tfullScreen\t\t1" + nl + "\t\trect\t\t\t0 0 640 480" + nl +
               "\t\tvisible\t\t\t1" + nl + "\t\tfocusColor\t\t1 1 1 1" + nl + "\t}" + nl;
    });
}

void zeichneSeiten(Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (!ImGui::BeginTabBar("##seiten", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton)) { return; }
    // Seitenwechsel aus dem Code (Cursor, Gliederung, Skript): den Reiter waehlen
    const bool erzwingen = d.seite != d.seiteGezeigt;
    for (std::size_t k = 0; k < s.size(); ++k) {
        const std::string l = anzeige(s[k].titel) + "##s" + std::to_string(k);
        const ImGuiTabItemFlags f = (erzwingen && static_cast<int>(k) == d.seite) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem(l.c_str(), nullptr, f)) {
            if (!erzwingen && d.seite != static_cast<int>(k)) {
                d.seite = static_cast<int>(k);
                d.auswahl.clear();
            }
            d.seiteGezeigt = static_cast<int>(k);
            ImGui::EndTabItem();
        }
    }
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

// Bausteine: Palette der Item-Typen und die Bibliothek
void panelBausteine(Dokument& d) {
    ImGui::PushID("panelBausteine");
    zeichnePalette(d);
    ImGui::PopID();
}

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

// Eigenschaften mit Cvars, Meldungen der Engine und Skript-Protokoll als Abschnitte
void panelEigenschaften(Dokument& d) {
    ImGui::PushID("panelEigenschaften");
    zeichneOriginal(d);        // M6 Punkt 8 (oben: Unterschied und Zuruecksetzen sofort sichtbar)
    zeichneEigenschaften(d);
    zeichneModSchluessel(d);   // M5b Punkt 4
    zeichneLayout(d);
    zeichneKomponente(d);      // M7 Punkt 3 (nach dem Layout: dessen Knoepfe bleiben ohne Rollen sichtbar)
    if (g_app->settings.menueTextUnten) { zeichneEreignisse(d); }   // mit Reitern: eigener Reiter Ereignisse
    zeichneCvars(d);
    zeichneMeldungen(d);
    ImGui::PopID();
}

// M5 Punkt 2: der Reiter Ereignisse (wie in Unreal UMG je Element)
void panelEreignisse(Dokument& d) {
    ImGui::PushID("panelEreignisse");
    zeichneEreignisse(d);
    ImGui::PopID();
}

// Die rechte Spalte: Reiter, der gewaehlte steht in den Einstellungen. Das Programm
// wechselt ihn nur auf Wunsch (setzeReiter), nie ungefragt.
void panelReiter(Dokument& d) {
    Zustand& z = Z();
    Settings& st = g_app->settings;
    if (z.reiterGewollt == -2) { z.reiterGewollt = std::clamp(st.menueReiter, 0, 3); }
    if (ImGui::BeginTabBar("##rechtsReiter")) {
        struct R {
            Str name;
            void (*f)(Dokument&);
            bool rollt;   // eigener Rollbereich (der Text rollt selbst)
        };
        static const R reiter[] = {{Str::MeReiterHierarchie, panelHierarchie, true},
                                   {Str::MeReiterText, panelText, false},
                                   {Str::MeReiterEigenschaften, panelEigenschaften, true},
                                   {Str::MeEreignisse, panelEreignisse, true}};
        const int gewollt = z.reiterGewollt;
        for (int i = 0; i < static_cast<int>(std::size(reiter)); ++i) {
            const std::string l = std::string(tr(reiter[i].name)) + "###reiter" + std::to_string(i);
            const ImGuiTabItemFlags f = i == gewollt ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem(l.c_str(), nullptr, f)) {
                if (gewollt < 0 || i == gewollt) { st.menueReiter = i; }
                if (reiter[i].rollt) {
                    ImGui::BeginChild("##inhalt", ImVec2{0, 0}, ImGuiChildFlags_None);
                    reiter[i].f(d);
                    ImGui::EndChild();
                } else {
                    reiter[i].f(d);
                }
                if (reiter[i].f != panelHierarchie && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) { z.werteBild = ImGui::GetFrameCount(); }
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
    editorTasten(*d);
    d = doc();
    if (d == nullptr) { return; }
    d->puffer.beiwerkFesthalten(anzeigeStand(*d));
    if (d->umbenennen >= 0 && d->umbenennenBild + 1 < ImGui::GetFrameCount()) { d->umbenennen = -1; }   // Zeile nicht mehr da
    const float fs = ImGui::GetFontSize();
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    Settings& st = g_app->settings;
    (void)hoehe;
    if (z.linksBreite <= 0) { z.linksBreite = fs * 13; }
    if (z.rechtsBreite <= 0) { z.rechtsBreite = st.menueRechtsBreite > 0 ? st.menueRechtsBreite * fs : fs * (st.menueTextUnten ? 19 : 28); }
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
    const bool links = da("menue.bausteine");
    const bool mitte = da("menue.flaeche");
    const bool rechts = da(unten ? "menue.eigenschaften" : "menue.reiter");
    if (unten && z.codeHoehe <= 0) { z.codeHoehe = std::floor(rest.y * 0.38F); }
    const float h = text && (links || mitte || rechts) ? std::max(fs * 6, rest.y - z.codeHoehe - 8) : rest.y;
    if (links) {
        spalte("menue.bausteine", ImVec2{mitte || rechts ? z.linksBreite : 0.0F, h}, *d);
        if (mitte || rechts) {
            ImGui::SameLine(0, 0);
            spaltenTeiler("##spl1", z.linksBreite, true, fs * 6, rest.x * 0.4F, h);
            ImGui::SameLine(0, 0);
        }
    }
    if (mitte) {
        const float mitteB = std::max(fs * 10, rest.x - (links ? z.linksBreite + 7 : 0.0F) - (rechts ? z.rechtsBreite + 7 : 0.0F));
        spalte("menue.flaeche", ImVec2{rechts ? mitteB : 0.0F, h}, *d);
        if (rechts) {
            ImGui::SameLine(0, 0);
            spaltenTeiler("##spl2", z.rechtsBreite, true, fs * (unten ? 10 : 12), rest.x * (unten ? 0.5F : 0.6F), h, true);
            if (!unten) { st.menueRechtsBreite = std::floor(z.rechtsBreite / fs * 100.0F) / 100.0F; }
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
        for (const auto& dd : z.docs) {   // schon offen (eigene Dateien): deren Text
            if (!da && kleinPfad(dd->name) == kleinPfad(f)) {
                inhalt = dd->puffer.text();
                da = true;
            }
        }
        if (!da) { continue; }
        if (oeffne(f, "", inhalt)) {
            doc()->satz = nr;
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

std::string letzteMeldung() { return Z().meldung; }

bool oeffneAusSpiel(const std::string& spielPfad) {
    datenPruefen();
    std::string t;
    if (!Z().daten.lies(spielPfad, t)) { return false; }
    return oeffne(spielPfad, "", t);
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

std::string cvarWert(const std::string& name) {
    Zustand& z = Z();
    if (z.interaktion) { return z.lauf.cvar(name); }
    const auto it = z.vorschauCvars.find(kleinPfad(name));
    return it == z.vorschauCvars.end() ? std::string() : it->second;
}

// rc608: Rueckgaengig stellt auch Seite und Auswahl wieder her (festgehalten in anzeigeFesthalten)
void anzeigeHolen(Dokument& d) {
    const std::vector<int>& b = d.puffer.beiwerk();
    if (b.empty()) { return; }
    d.seite = b[0];
    d.auswahl.assign(b.begin() + 1, b.end());
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
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    const std::string basis = typ < 0 ? "image" : std::string(typName(typ)).substr(10);
    std::string name = basis;
    for (char& c : name) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    name = freierName(m, name);
    std::string einr = "\t\t";
    if (!m.items.empty() && m.items.front().kopf.datei == 0) {
        const std::string& t = d->puffer.text();
        std::size_t a = m.items.front().anfang;
        std::size_t s = a;
        while (s > 0 && t[s - 1] != '\n') { --s; }
        einr = t.substr(s, a - s);
    }
    const Rechteck r{static_cast<float>(220 - m.window.rect.x), static_cast<float>(200 - m.window.rect.y), 200, 40};
    std::vector<Ersetzung> e;
    if (itemEinfuegen(d->datei, mi, itemVorlage(typ, d->datei.dialekt, name, r, einr, d->puffer.zeilenUmbruch()), e)) {
        aendere(std::move(e));
        const int neu = static_cast<int>(d->datei.menus[static_cast<std::size_t>(mi)].items.size()) - 1;
        waehle(neu, false);
    }
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

void dupliziereAuswahl() { fuegeItemsEin(kopiereAuswahl()); }

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
    d->gesperrt.clear();   // die Nummern verschieben sich
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
std::string modProfil() { return aktivesProfil()->name; }
bool legeProfilVorlageAn(const std::string& pfad) { return profilVorlageSchreiben(pfad); }
void beimBeenden() {   // ungespeicherte Arbeit bleibt in der Sicherung (nur, wenn der Editor in dieser Sitzung lief)
    if (Z().sicherungGeprueft) { sicherungJetzt(); }
}

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

void setzeBalken(bool balken) { Z().seitenv = balken ? Seitenverhaeltnis::Balken : Seitenverhaeltnis::Strecken; }

std::string letzterSatzWechsel() { return Z().satzWechsel; }

void sperreItem(int item, bool sperren) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    if (sperren) { d->gesperrt.insert(item); } else { d->gesperrt.erase(item); }
}

void verbergeItem(int item, bool verbergen) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
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
            setzeVorlageEin(i, true, x, y);
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
