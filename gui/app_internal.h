// app_internal.h - der gemeinsame Zustand der Oberflaeche
//
// gui/app.cpp war 5138 Zeilen lang. Das ist die eine Stelle, an der jeder
// Aussenstehende zuerst haengenbleibt - und zu Recht: eine Datei, die alles
// enthaelt, laesst sich weder ueberblicken noch getrennt uebersetzen.
//
// Diese Kopfdatei haelt, was mehrere Teile brauchen: den Zustand (App), den
// Zeiger darauf und die Deklarationen der Funktionen, die ueber Dateigrenzen
// hinweg aufgerufen werden. Sie ist bewusst NICHT oeffentlich - nur die
// Dateien unter gui/ binden sie ein.
//
// Der Zustand bleibt ein einfacher Verbund mit einem globalen Zeiger. Das
// ist kein Versehen: das Programm hat genau ein Dokument, wie das Original,
// und eine Abhaengigkeitsspritze dafuer waere Zeremonie ohne Gewinn.
#ifndef BHED_GUI_APP_INTERNAL_H
#define BHED_GUI_APP_INTERNAL_H

// SplitterBehavior, ImRect und GetCurrentWindow stehen in der INTERNEN
// Kopfdatei von ImGui. Sie zu benutzen ist die Loesung, die ocornut selbst
// im Fehlerverzeichnis empfiehlt (Ausgabe 4357) - fuer Trenner gibt es
// keine oeffentliche Entsprechung.
//
// Seit rc499 auch fuer `ImGuiTable::ResizedColumn` (imgui_internal.h:3134,
// "Index of column being resized"). Drei Anlaeufe haben versucht, das
// Ziehen an einer Spaltengrenze zu ERRATEN - IsMouseDown, ein
// Breitenvergleich, ein Fenstergroessenvergleich. Alle drei waren Ersatz
// fuer eine Frage, die ImGui genau beantworten kann.
#include "imgui.h"
#include "imgui_internal.h"

#include "backend.h"
#include "audio_win32.h"
#include "icons_gen.h"
#include "platform.h"
#include "bhed/diag.h"
#include "bhed/regelwerk.h"
#include "bhed/ablauf.h"
#include "bhed/bsp.h"
#include "bhed/enttext.h"
#include "bhed/areaportal.h"
#include "bhed/holster.h"
#include "bhed/schmerz.h"
#include "bhed/camtrack.h"
#include "bhed/wegpunkte.h"
#include "bhed/moveranim.h"
#include "bhed/mission.h"
#include "bhed/missionsgraph.h"
#include "bhed/scene.h"
#include "bhed/npcklasse.h"   // rc602
#include "bhed/saberspuren.h"   // rc601 saber review: SpurPoly
#include "bhed/kollision.h"     // rc601 integ: geladeneKollision
#include "bhed/kartemission.h"  // rc609 B2: Befunde des Abgleichs
#include "bhed/gegenstaende.h"  // rc602 kartenpruefung: Gegenstaende der Karte
#include "bhed/timeline.h"
#include "bhed/spuren.h"
#include "bhed/pmanim.h"   // Zeitleiste 4: aufnahmePmFigur
#include "bhed/image.h"
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/gpushader.h"   // rc608 Teil 11: kSpielLightmapFaktor
#include "bhed/mapview.h"
#include "bhed/efxdraw.h"
#include "bhed/geschuetz.h"
#include "bhed/mover.h"
#include "bhed/md3.h"
#include "bhed/modellsetzen.h"   // rc609 M0
#include "bhed/efx/io.h"
#include "bhed/shaderscript.h"
#include "bhed/keys.h"
#include "bhed/pk3.h"
#include "bhed/portalsicht.h"   // rc609 P
#include "bhed/roff.h"
#include "bhed/sound.h"
#include "bhed/edit.h"
#include "bhed/interplay.h"
#include "bhed/keys.h"
#include "bhed/settings.h"
#include "bhed/i18n.h"
#include "bhed/ibi.h"
#include "bhed/theme.h"
#include "bhed/tree.h"
#include "bhed/validate.h"
#include "imgui.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cfloat>
#include <chrono>
#include <functional>
#include <memory>
#include <future>
#include <utility>
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstring>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace bhed::gpu {
struct KlingenQuad;
}  // namespace bhed::gpu
namespace bhed::karte {
struct BspLumps;
}  // namespace bhed::karte

namespace bhed::gui {

using i18n::Str;
using i18n::tr;

// --- Masse aus der Ressource ------------------------------------------


// --- Dialogeinheiten aus der Ressource --------------------------------
constexpr float kDluX = 1.5F;    // MS Sans Serif 8pt: baseunit 6 -> 6/4
constexpr float kDluY = 1.625F;  // baseunit 13 -> 13/8

struct Rect { float x, y, w, h; };

// Dialog 102, ausgelesen mit tools/dlgdump.py
// Aus Dialog 102 stammen jetzt nur noch die VERHAELTNISSE, nicht die
// absoluten Masse - siehe computeLayout(). Die Werte zum Nachrechnen:
//   Events      x=7   breit 191   ->  28,8 % von 663
//   Script Flow x=206 breit 372   ->  56,1 %
//   Knoepfe     x=585 breit  71   ->  10,7 %
//   Status      y=381 hoch   62   ->  13,8 % von 449
constexpr float kDialogW = 663;
constexpr float kDialogH = 449;

// Event-Editor, am Original gemessen
constexpr float kEdLeft = 100;
constexpr float kEdFieldW = 167;
constexpr float kEdFieldH = 14;
constexpr float kEdGap = 9;
constexpr float kEdBtnRow = 88;
constexpr float kEdOkW = 67;
constexpr float kEdOkPitch = 77;


// rc609 W2a Punkt 1: woher eine geoeffnete Datei kommt (.bsp, .ent, Skript).
// Mike (03.11.): "When I make changes, I expect the files I opened to be
// affected. Not be copied to the documents folder and a .pk3 file." Lose
// geoeffnete Dateien speichern ohne Rueckfrage an ihren Pfad; eine Datei aus
// einer .pk3 bietet die Wege an (speicherWegAnbieten, gui/app_dateiort.cpp).
struct DateiHerkunft {
    enum class Art : std::uint8_t { Keine, Lose, Pk3 };
    Art art = Art::Keine;
    std::string pfad;      // Lose: Plattenpfad; Pk3: Pfad der .pk3
    std::string eintrag;   // Pk3: Name im Archiv ("maps/x.bsp")
    [[nodiscard]] static DateiHerkunft lose(const std::string& p) { return {Art::Lose, p, {}}; }
    [[nodiscard]] static DateiHerkunft ausPk3(const std::string& a, const std::string& e) { return {Art::Pk3, a, e}; }
    // rc609 W2b F17: was readFromArchives gefunden hat - eine lose Datei oder einen Eintrag einer .pk3.
    [[nodiscard]] static DateiHerkunft gefunden(const std::string& her, bool istLose, const std::string& e) {
        return istLose ? lose(her) : ausPk3(her, e);
    }
};

struct WegAuftrag;   // rc609 W3b: die Erzeugung im Hintergrund (gui/app_wegpunkte.cpp)
struct WegAbdeckAuftrag;   // rc609 W4: die Abdeckung im Hintergrund (gui/app_wegpunkte.cpp)
struct WegDeckAuftrag;   // rc609 H2: die Kampfpunkt-Vorschlaege im Hintergrund (gui/app_deckung.cpp)

// Zustand der Oberflaeche. Bewusst ein einfacher Verbund - das Programm hat
// genau ein Dokument, wie das Original.
struct App {
    Document doc{Script{}};
    CommandDb db;
    std::string path;           // leer = noch nie gespeichert
    // Der ICARUS-Pfad des offenen Skripts ("md_ga/intro_jedi"), wenn es aus
    // einem Archiv kam. Sonst wird er aus `path` abgeleitet
    // (aktiverSkriptPfad). Damit findet der Nachbau den Traeger in der Karte.
    std::string skriptPfad;
    // rc609 W2a: Herkunft des offenen Skripts, wenn es aus einer .pk3 kam (path
    // ist dann leer). Mit Pfad gilt immer: lose Datei an `path`.
    DateiHerkunft skriptHerkunft;
    // Der ICARUS-Nachbau des offenen Skripts (bhed/ablauf.h): daraus
    // entstehen Kamerabahn, Zeitleiste und Szene.
    Ablauf ablauf;
    // rc604 fix604: die Plaene aus dem gemeinsamen Takt (andere Figuren im
    // Weg) - fuer die Uebersicht des Tuer-Laufs im Selbsttest.
    std::shared_ptr<const std::map<std::uint64_t, std::shared_ptr<const LaufPlan>>> koerperPlaene;
    // Gelesene Skripte fuer run/use/spawnscript, nach Pfad (klein).
    std::map<std::string, std::unique_ptr<Script>> ablaufSkripte;
    // SET_CAMERA_GROUP aus dem Ablauf: Entity -> (ab ms, Gruppe), geordnet.
    std::map<std::string, std::vector<std::pair<double, std::string>>> kameraGruppen;
    // Gelesene ROFF-Bahnen fuer play ( "PLAY_ROFF", ... ), nach dem
    // Archivpfad (klein). nullptr heisst: gesucht und nicht gefunden oder
    // nicht lesbar - auch das gemerkt, weil jeder Szenenaufbau fragt.
    std::map<std::string, std::shared_ptr<const Roff>> roffs;
    Settings settings;
    // Meldungen der Statusliste. Das Original nummeriert sie:
    //     (0) : ( Exported )
    //     (1) : ( Exported )
    std::vector<std::string> statusLines;
    std::string statusModell;                // rc608 Teil 3b: die Zeile "Command model ...", im Menue-Editor ohne Sinn
    std::vector<std::string> statusTeile;    // ... was die Statuszeile rechts gerade zeigt
    MapData map;
    // Die Entities je Karte (Kennung = MapData::path), wenn sie NICHT aus
    // der .bsp stammen: aus einer .ent (Mission, "Open .ent...") oder
    // bearbeitet.
    //
    // Gefunden beim Bau einer eigenen Mission (11.10.): ein Reiter mit einer
    // anderen Karte und zurueck - der Reiterwechsel las die Karte neu, aber
    // nur die .bsp. Alle Figuren der Mission waren fort. loadMapFromBytes
    // setzt sie von hier wieder ein; wer eine Karte ausdruecklich oeffnet,
    // bekommt sie frisch (vergissEntities).
    std::map<std::string, std::vector<MapEntity>> entitiesJeKarte;
    // rc609 W2a Punkt 1: Herkunft der geladenen .bsp (Lose: settings.mapPath,
    // Pk3: Archiv + Eintrag; Keine: neue Mission auf einer Grundkarte) und der
    // .ent, wenn sie eigens geoeffnet wurde (gilt fuer die Karte entHerkunftKarte,
    // sonst gehoert die .ent neben die .bsp).
    DateiHerkunft kartenHerkunft;
    DateiHerkunft entHerkunft;
    std::string entHerkunftKarte;
    // Punkt 3: offene Fragen "wohin speichern?" (eine je Datei aus einer .pk3),
    // die vorderste zeigt drawSpeicherWeg.
    struct SpeicherAuftrag {
        std::string anzeige;        // "maps/x.ent (zz_mod.pk3)"
        DateiHerkunft herkunft;     // Art::Pk3
        bool missionsWeg = true;    // Weg 4 moeglich (Skripte, Entities)
        std::function<void(int weg)> ausfuehren;   // 1..4
    };
    std::vector<SpeicherAuftrag> speicherFragen;
    bool speicherWegMerken = false;
    // --- Missionsbau (gui/app_mission.cpp) ---------------------------------
    struct MissionsProjekt {
        std::string name;           // "md_tpa_jedi" -> maps/md_tpa_jedi.bsp/.ent
        std::string ordner;         // "md_tpa" -> scripts/md_tpa/
        std::string karte;          // Kennung der Karte (MapData::path), bei neuen "maps/<name>.bsp"
        std::string archiv;         // .pk3 einer geladenen Mission (Klaenge, .str ...)
        std::string projektOrdner;  // Missionsordner auf der Platte
        std::string zielPk3;        // wohin "Save mission" packt
    };
    MissionsProjekt mission;
    // --- Graph-Ansicht des Script Flow (gui/app_ansichten.cpp) -------------
    struct GraphZustand {
        MissionsGraph g;
        std::string signatur;                 // geaendert -> neu bauen
        float panX = 0.0F;
        float panY = 0.0F;
        float zoom = 1.0F;
        bool angepasst = false;               // schon einmal eingepasst
        std::map<std::string, ImVec2> pos;    // von Hand verschobene Knoten
        std::string auswahl;
        std::string zieht;                    // Knoten, der gerade gezogen wird
        std::string verbindetVon;             // Verbindung, die gerade gezogen wird
        std::string verbindungVon;            // fuer das Klappmenue "Verbinden als"
        std::string verbindungNach;
        std::map<std::string, std::shared_ptr<Script>> skriptCache;   // nicht offene Skripte
        std::string cacheKarte;
        ImVec2 ursprung{0.0F, 0.0F};          // linke obere Ecke der Flaeche (Bildschirm)
        ImVec2 groesse{0.0F, 0.0F};           // Groesse der Flaeche
        // rc597: rechtwinklige Leitungen durch die Gassen, Umlenkpunkte je
        // Kante (Weltkoordinaten), in den Graphen gezogene Entities.
        bool rechtwinklig = true;
        std::map<std::string, std::vector<ImVec2>> umlenk;
        std::string ziehtUmlenk;              // Kante, deren Umlenkpunkt gezogen wird
        int ziehtUmlenkNr = -1;
        std::vector<int> immerEntities;
        // Fuer den Selbsttest, je Bild neu: wo die Beschriftung jeder Kante
        // steht (Bildschirm), schraege Stuecke und Kreuzungen mit fremden
        // Knoten im rechtwinkligen Stil.
        std::map<std::string, ImVec2> kantenMitte;
        int schraegeStuecke = 0;
        int kreuzungen = 0;
    };
    // Je Feld der geteilten Ansicht ein eigener Graph-Zustand (Zoom, Lage,
    // Auswahl) - zwei Graphen nebeneinander sollen sich nicht gegenseitig
    // verschieben. graphFeld: das Feld, das gerade gezeichnet wird.
    GraphZustand graphe[4];
    int graphFeld = 0;
    GraphZustand& graph() { return graphe[graphFeld]; }
    float spurenZoom = 1.0F;                  // Spuren-Ansicht: Zoom der Zeitachse
    bool spurenListe = true;                  // Spuren-Ansicht: die Befehlsliste rechts (rc601)
    // Wo die Bloecke im letzten Bild standen (Pfad, Rechteck) - fuer den Selbsttest.
    std::vector<std::pair<Path, ImRect>> spurenBloecke;
    // Kurze Einblendungen unten rechts (meldung / drawEinblendungen).
    struct Einblendung {
        std::string text;
        std::string ordner;   // nicht leer: Knopf "Im Explorer zeigen"
        bool warnung = false;
        double bis = 0.0;     // ImGui::GetTime(), bis wann sichtbar
    };
    std::vector<Einblendung> einblendungen;
    // Woher die Geometrie einer Karte kommt, die es unter ihrer Kennung (noch)
    // nicht gibt: eine neue Mission heisst "maps/<name>.bsp", ihre .bsp ist
    // die der Grundkarte - ein Archivname oder ein Plattenpfad. So teilen sich
    // zwei Missionen auf derselben Grundkarte nie ihre Entities.
    std::map<std::string, std::string> kartenQuelle;
    bool missionAktiv = false;
    bool neueMissionOffen = false;
    bool missionSpeichernOffen = false;
    // Kopfzeile der Kartenansicht (rc586): Klappfenster und Statistik.
    bool ansichtEinstellungenOeffnen = false;   // naechstes Bild oeffnen (Selbsttest)
    bool kartenMehrOeffnen = false;             // dito, Menue "Karten und Dateien"
    bool kartenStatistikOffen = false;
    // Karten (Kennung), deren Entities geaendert und nicht gespeichert sind.
    std::set<std::string> entitiesUngespeichert;
    bool entFrageBeantwortet = false;   // beim Beenden schon gefragt
    // Rueckgaengig fuer Entities - eigener Verlauf, gilt fuer EINE Karte.
    std::vector<std::vector<MapEntity>> entUndo;
    std::vector<std::vector<MapEntity>> entRedo;
    // mission1: die Stempel dazu (naechsterAenderungsStempel), gleich lang wie
    // entUndo/entRedo. Gleicher Stempel wie ein Skriptschritt = EIN Schritt
    // (ein Zwischenpunkt legt ein Laufziel an UND aendert das Skript).
    std::vector<std::uint64_t> entUndoStempel;
    std::vector<std::uint64_t> entRedoStempel;
    // Die Stempel der zurueckgenommenen Schritte in der Reihenfolge des
    // Zuruecknehmens: Strg+Y holt den zuletzt zurueckgenommenen zuerst. Ohne
    // diese Folge waehlte die Kartenansicht nach dem kleinsten Stempel - und
    // griff zu einem alten Wiederholen-Schritt des anderen Stapels.
    std::vector<std::uint64_t> zurueckGenommen;
    std::string entUndoKarte;
    bool entRawAnsicht = false;         // alle Schluessel roh (wie SmartEdit aus)
    // Maus ueber der Kartenansicht / der Entity-Spalte (letztes Bild) - dann
    // wirken Strg+Z, Strg+Y und Entf auf Entities.
    bool entFokusBild = false;
    bool entFokusListe = false;
    struct EntZug {
        bool kandidat = false;          // auf die gewaehlte Entity gedrueckt
        bool aktiv = false;             // und gezogen
        int index = -1;
        std::vector<MapEntity> vorher;
        // Was gezogen wird: 0 ueber den Boden, 1..3 Gizmo-Achse X/Y/Z,
        // 4 Gizmo-Ring (Blickrichtung).
        int modus = 0;
        float startOrt[3] = {0.0F, 0.0F, 0.0F};   // der Schluessel "origin" (nicht der gezeigte Ort)
        float versatz[3] = {0.0F, 0.0F, 0.0F};    // gezeigt minus "origin" beim Anfassen (rc597)
        // Mitten in einer Bewegung angefasst (rc597): kein Verschieben des
        // Ursprungs, sondern ein neuer Bahnpunkt zur eingestellten Zeit -
        // wie der Auto-Key der Kamera. Waehrend des Ziehens nur die Bahn
        // dieser Figur im Speicher, geschrieben wird beim Loslassen.
        bool zwischen = false;
        bool zwLauf = false;          // Figur (SET_NAVGOAL) statt Schiff (move)
        bool zwBewegt = false;
        int zwFigur = -1;
        double zwT = 0.0;
        Path zwPfad;
        std::vector<ActorStep> zwSteps;
        float zwOrt[3] = {0.0F, 0.0F, 0.0F};
        // Ein Schluessel zur eingestellten Zeit statt den Start zu aendern
        // (rc599, "die Rotation beim Schiff und beim Character setzt keinen
        // neuen Frame, es passt immer den ersten Punkt an"):
        //   1  Schluessel des Animationsfensters (Drehen und Verschieben)
        //   2  Schiff mitten in einem move: neuer Bahnpunkt mit Winkeln
        //   3  Figur: set ( SET_ANGLES ) zu dieser Zeit (im Laufen: beim Ankommen)
        // Die Vorschau haengt zwei Schritte an die Figur (zwSteps bleibt).
        int dkArt = 0;
        bool dkBewegt = false;
        float dkOrt[3] = {0.0F, 0.0F, 0.0F};
        float dkBasisOrt[3] = {0.0F, 0.0F, 0.0F};
        float dkBasis[3] = {0.0F, 0.0F, 0.0F};
        float dkWinkel[3] = {0.0F, 0.0F, 0.0F};
        float gierVersatz = 0.0F;                 // ebenso fuer die Blickrichtung
        float startWinkel = 0.0F;
        float startWinkel3[3] = {0.0F, 0.0F, 0.0F};   // Nicken, Gier, Rollen beim Anfassen (rc599)
        float startMaus[2] = {0.0F, 0.0F};
        float achsLaenge = 0.0F;        // Weltlaenge der Gizmo-Achse beim Anfassen
        // rc609 M0: Skalieren eines Kartenmodells (Griffe 7..9 je Modellachse, 10 gleichmaessig)
        float startSkala[3] = {1.0F, 1.0F, 1.0F};
        // Mehrfachauswahl (rc590): die anderen gewaehlten ziehen mit -
        // Ausgangsort und -winkel beim Anfassen.
        std::vector<std::pair<int, std::array<float, 3>>> gruppe;
        std::vector<float> gruppeWinkel;
    };
    EntZug entZug;
    // --- Mehrfachauswahl, Raster, Platzieren, gemerkte Ansichten (rc590) -----
    //
    // pickedEntity bleibt der HAUPTPUNKT (Inspektor, Gizmo); entAuswahl sind
    // die weiteren. Strg/Umschalt+Klick nimmt hinzu oder heraus, ein Rahmen
    // ins Leere waehlt alles darin, die Gruppe zieht mit dem Hauptpunkt.
    std::vector<int> entAuswahl;
    bool boxKandidat = false;
    bool boxAktiv = false;
    bool boxZusatz = false;
    float boxStart[2] = {0.0F, 0.0F};
    int rasterSchritt = 0;              // 0 aus (Strg rastet dann auf 8), sonst 8/16/32/64
    std::string platzierKlasse;         // Platzieren-Leiste: naechster Klick setzt diese Klasse
    bool platzierZiehen = false;        // Kachel gedrueckt und in die Ansicht gezogen
    // --- rc609 M0: vorhandene Modelle mit einem Klick in die Karte (gui/app_modellsetzen.cpp) ---
    bool modellFensterOffen = false;
    char modellSuche[128] = {};
    std::vector<modellsetzen::ModellDatei> modellListe;   // .md3 unter models/ der Spielordner
    std::size_t modellListeOrdner = 0;                    // Zahl der Spielordner beim Sammeln
    bool modellListeNeu = true;
    std::string modellPlatzieren;   // gewaehlt: der naechste Klick (oder das Loslassen) setzt es
    bool modellZiehen = false;      // aus der Liste in die Ansicht gezogen
    struct ModellGeist {            // das Modell unter der Maus, bevor es steht (3D-Vorschau)
        bool aktiv = false;
        std::string pfad;
        float ort[3]{};
        float winkel[3]{};
        float skala[3]{1.0F, 1.0F, 1.0F};
    };
    ModellGeist modellGeist;
    int modellGeistGezeichnet = 0;   // wie oft der Geist in ein Bild der Vorschau kam (Selbsttest)
    std::map<std::string, std::pair<std::string, modellsetzen::DateiStand>> modellStaende;   // Live: Fundort und Stand je Modelldatei
    std::map<std::string, int> modellFassung;                         // je neu geladener Datei hochgezaehlt
    double modellWacheZeit = -1.0;
    bool kartenModelleNeu = false;  // eine Modelldatei ist neuer: loadMapModels im naechsten Bild
    bool entGizmoSkalieren = false; // Gizmo-Leiste: Skalieren (nur Kartenmodelle)
    struct AnsichtsMarke {
        bool da = false;
        Camera cam;
    };
    std::map<std::string, std::array<AnsichtsMarke, 9>> ansichtsMarken;   // je Karte, Strg+1..9 / 1..9
    // Hervorhebung beim Ueberfahren (wie in Unreal): die Entity unter der
    // Maus in der Ansicht, und die, ueber der die Maus in der Liste steht.
    int entHover = -1;
    int entListeHover = -1;
    int entGizmoHover = 0;              // Entity-Gizmo: 0 keiner, 1..3 Achse, 4 Ring
    float rechtsklickBild[2] = {-1.0F, -1.0F};   // Bildschirmstelle des kurzen Rechtsklicks
    // Kartenansicht
    BspGeometry geo;
    BspMesh mesh;
    Camera cam;
    Orbit orbit;
    MapImage mapImage;
    // Linke Spalte: 0 Ereignisse, 1 Karte, 2 Modell
    int leftMode = 0;
    // Der Modus des vorigen Bildes.
    //
    // Beim Wechsel hat die Tabelle des neuen Modus noch keine
    // Spaltenbreiten (jeder Modus hat seit rc501 eine eigene ID). Fuer EIN
    // Bild ist die Inhaltszelle deshalb winzig - shanks Protokoll zeigt
    // ZelleW 223 statt 1932 und BildW 64 statt 1584.
    //
    // Sichtbar ist das als kurzes Zucken; schlimmer ist, dass fuer dieses
    // eine Bild ein 64 Punkte grosses Renderziel angelegt und sofort
    // wieder verworfen wird.
    int letzterModus = -1;
    // Wahr genau in dem Bild, in dem der Modus gewechselt hat.
    //
    // Gesetzt in der Aufteilungsberechnung, die in JEDEM Modus laeuft -
    // nicht in drawMapView, die nur im Karten-Modus laeuft.
    bool moduswechselBild = false;
    // Zuletzt aufgenommener Ziehweg - damit die Zeile nur bei Wechsel kommt.
    std::vector<std::size_t> dbgZiehWeg;
    GlmModel model;
    std::string modelPath;
    Camera modelCam;
    Orbit modelOrbit;
    MapImage modelImage;
    bool modelDirty = true;
    bool showCaps = false;
    // Skelett und Bewegung
    GlaAnimation anim;
    std::vector<AnimEntry> animList;
    int animIndex = -1;
    int animFrame = 0;
    bool animPlaying = false;
    float animTime = 0.0F;
    char animFilter[48] = {};
    // Der Text im Animationsfeld. Gehoert hierher, nicht in eine oertliche
    // Variable: sonst ist jede Eingabe im naechsten Bild wieder weg.
    std::string animText;
    // 0 = wie in der .cfg, 1 = immer, 2 = einmal
    int loopMode = 0;
    float animSpeed = 1.0F;
    ModelTextures modelTextures;
    // Die Haeute, die neben dem Modell liegen, und die gewaehlte.
    //
    // Ein Modell bringt oft mehrere mit: model_default.skin, model_red.skin,
    // model_blue.skin. Sie entscheiden nicht nur ueber Texturen, sondern
    // auch darueber, WELCHE Flaechen sichtbar sind - eine Zeile "kopf_b,*off"
    // schaltet eine Variante ab. Ohne Auswahl werden alle Varianten
    // gleichzeitig gezeichnet.
    std::vector<std::string> modelSkins;
    int modelSkinIndex = -1;
    int modellFlaechenListen = 0;   // H2: Klicks auf den Flaechenzaehler (Tour k-skins)
    int modellHautWechsel = 0;      // H2: vom Anwender gewaehlte Haeute (Liste, Datei) - nicht das Laden einer Figur
    std::string modelDir;
    bool showModelTextures = true;
    // Model-Tab (rc591)
    bool npcBrowserOffen = false;
    std::string npcGeladen;          // zuletzt im NPC-Browser geladen
    bool modellDrehteller = false;
    bool modellKnochen = false;
    int modellKnochenLinien = 0;     // im letzten Bild gezeichnet (Selbsttest)
    bool animScrollen = false;       // Liste zur gewaehlten Animation rollen
    // modell1: was das Neuzeichnen des Modellfensters kostet (Selbsttest
    // modellleistung liest die Differenzen je Bild) - CPU-Zeit aufsummiert in
    // ms, wie oft neu gezeichnet wurde, Zeichenaufrufe des letzten Neuzeichnens.
    double modellCpuMsSumme = 0.0;
    int modellNeuGezeichnet = 0;
    int modellAufrufe = 0;
    // modell1: die Weltmatrizen der Knochen vom letzten Neuzeichnen - die
    // Knochenansicht zeichnet dieselbe Haltung, ohne sie jedes Bild neu zu
    // rechnen (leer ohne Skelett).
    std::vector<BoneMatrix> modellWelt;
    // Mover animieren (gui/app_animation.cpp, rc592)
    bool animFensterOffen = false;
    int animEntity = -1;
    int animSchluessel = -1;
    bool animZug = false;            // eine Raute wird gezogen
    float animZugMaus[2] = {0.0F, 0.0F};
    float animZugStart[3] = {0.0F, 0.0F, 0.0F};
    float animZugSkala = 1.0F;
    std::vector<AnimSchluessel> animVorschau;
    // ROFF-Modus des Fensters (rc599): die Datei, deren Schluessel gerade
    // bearbeitet werden ("md_x/roffs/schiff"). Leer: move-Animation.
    std::string animRoff;
    // Aus den Marken (bhed/roffanim.h) gebackene Bahnen, nach dem Text der
    // Marke - die Szene fragt bei jedem Aufbau.
    std::map<std::string, std::shared_ptr<const Roff>> roffAusMarke;
    float lastModelCam[7]{};
    // --- Feinheit des Netzes ---------------------------------------------
    //
    // Stand auf 4. Das war eine Ruecksicht auf den Software-Rasterer: bei
    // 10 hat md_am_sith 325787 Dreiecke statt 106318, und die wollte er
    // nicht dreissigmal je Sekunde durchgehen.
    //
    // Seit dem GPU-Umbau kostet es nichts - shank misst konstant 120 fps
    // bei 10. Und bei 4 fehlt Geometrie, die man beim Vergleichen sucht:
    // die schwarzen Flaechen auf einem der Bilder zu rc436 waren genau das.
    //
    // Wer den Rasterer bei einer grossen Karte benutzt, stellt es herunter;
    // die Vorgabe soll das zeigen, was da ist.
    //
    // rc608 Teil 13b: Vorgabe ist das Gitter des Spiels (bhed::kPatchWieImSpiel: R_SubdividePatchToGrid mit
    // r_subdivisions 4, je Bild Spalten und Zeilen nach r_lodCurveError 250). 1 bis 10 sind die gleichmaessigen Netze
    // von frueher, je Feld so viele Teile.
    int meshDetail = bhed::kPatchWieImSpiel;
    // Nur neu zeichnen, wenn sich etwas geaendert hat. Ein Editor steht die
    // meiste Zeit still; jedes Bild neu zu rechnen kostet Strom und macht
    // den Luefter laut, ohne dass sich etwas bewegt.
    bool mapDirty = true;
    // modell1: in welchem Bild (ImGui::GetFrameCount) die Kartenansicht und die
    // Modellansicht zuletzt gezeichnet wurden - darfRuhen fragt danach: eine
    // "neu zu zeichnende" Karte braucht nur Bilder, solange man sie sieht.
    int kartenAnsichtBild = -1;
    int modellAnsichtBild = -1;
    // rc602: der Hinweis unter NPC_type im Entity-Inspektor, wie zuletzt
    // gezeichnet (leer: keiner) - fuer den Selbsttest.
    std::string entNpcHinweis;
    TextureSet textures;
    bool showEntities = true;
    // Himmelsflaechen umschliessen die Karte; ausgeblendet sieht man von
    // aussen hinein. Zum Beurteilen einer Kameraeinstellung im Freien will
    // man sie aber sehen.
    // Die Himmelsflaechen zeichnen.
    //
    // Stand bis rc355 auf AUS, und das war richtig: die Flaechen umschliessen
    // die Karte wie ein Kasten, und ohne Himmelsbox sah man nur eine graue
    // Schale, die alles verdeckte.
    //
    // Seit die Box gezeichnet wird, ist es umgekehrt: ohne sie fehlt der
    // Himmel ganz und man sieht den Verlaufshintergrund. 589 Flaechen in den
    // 43 Karten sind betroffen.
    bool showSky = true;
    bool zeigeSpuren = true;     // Schwertspuren (rc592), wie cg_saberTrail
    int spurBaender = 0;         // gezeichnete Spurbaender, aufsummiert (Selbsttest)
    bool zeigeWetter = true;     // Regen, Schnee, Staub, Nebelwolken (rc592)
    int wetterTeilchenZahl = 0;  // im letzten Bild gezeichnet (Selbsttest)
    bool zeigeFlares = true;     // Lens-Flares (rc592), wie r_flares 1
    int flareZahl = 0;           // im letzten Bild gezeichnet
    // --- Bild-im-Bild: was die gewaehlte Kamera sieht (rc592) -----------
    //
    // Solange eine Skriptkamera gewaehlt ist und die Ansicht NICHT durch sie
    // schaut, steht unten rechts ein kleines Bild aus ihrer Sicht - beim
    // Ziehen am Gizmo laeuft es mit. Kostet ein zweites, kleines GPU-Bild,
    // aber nur in dieser Lage. Abschaltbar in der Kameraleiste.
    bool kameraBildImBild = true;
    // Zaehlt jedes Leeren von texCache (rescanGamePaths). Wer sich einen
    // Zeiger in texCache merkt, vergleicht hiermit - die ADRESSE von
    // texCache aendert sich beim Leeren nicht (Code-Pruefung rc592).
    int texCacheStand = 0;
    bool vorschauGezeigt = false;   // im letzten Bild gezeichnet
    int vorschauW = 0;
    int vorschauH = 0;
    float vorschauPos[3] = {0.0F, 0.0F, 0.0F};   // wo die Vorschau-Kamera zuletzt stand (Selbsttest)
    float vorschauWinkel[3] = {0.0F, 0.0F, 0.0F};  // und wohin sie schaute
    int nebenEffektEcken = 0;     // Ecken des Effektnetzes, das die letzte Nebenansicht zeichnete (Selbsttest)
    bool rauchImGluehen = true;   // Rauch dunkelt das Gluehen ab (rc597; aus nur fuer den Selbsttest)
    // Woran arbeite ich gerade (rc597)? Die Entities des aktiven Skripts
    // hervorheben, die anderen blasser; der Name des Reiters in der Ansicht.
    bool skriptFokus = true;
    std::vector<char> entImSkript;   // je Entity: benutzt das aktive Skript sie? (leer = unbekannt)
    std::string skriptSchildText;    // was das Schild oben links zuletzt zeigte (Selbsttest)
    int szeneBauten = 0;             // wie oft die Szene neu gebaut wurde (Schluessel fuer Zwischenspeicher)
    // Fuer den Selbsttest (rc597): die Bahn der gewaehlten Figur/des Schiffs -
    // Stuecke schon gefahren (farbig) und noch vor ihr (grau); wo jede Figur
    // in diesem Bild gezeichnet wurde; die Zeitleiste: hervorgehobene Zeile,
    // Bewegungsbalken und Ausloeser darin.
    int bahnFarbig = 0;
    int bahnGrau = 0;
    // mission1: die Hinweise "kein Weg" an der Bahn der gewaehlten Figur (letztes Bild).
    std::vector<std::string> bahnKeinWeg;
    std::vector<std::array<float, 3>> figurGezeichnet;
    std::vector<int> figurBildGezeichnet;   // Endrunde rc608: das gezeichnete Bild der .gla je Figur (Selbsttest)
    // rc608: renderInfo.lastHeadAngles und lookingDebounceTime je Figur, Bildnummer -1: noch nichts.
    struct BlickStand {
        float winkel[3]{};
        double entprelltBis = -1.0;
        float gier = 0.0F;
    };
    struct BlickFolge {
        long long bild = -1;
        float winkel[3]{};
        double entprelltBis = -1.0;
        float gier = 0.0F;
        long long start = -1;    // Endrunde 4: Beginn des Vorlaufs (festes 125er-Raster, blickFolgeRechnen)
        bool mitBlick = false;   // lookRotation, mit dem gerechnet wurde
        std::map<long long, BlickStand> zwischen;   // Zwischenstaende dieses Vorlaufs (je 8 Bilder) fuer das Zurueckspulen
    };
    std::vector<BlickFolge> blickFolgen;
    std::vector<BlickFolge> blickFolgenSaber;   // Endrunde 4: derselbe Blick fuer die Clientbilder der Schwerter
    std::string tlAuswahlZeile;
    int tlBewegungsBalken = 0;
    // --- Griffe auf der Bahn (rc597) ------------------------------------------
    //
    // "kann ich den Anfangspunkt und den Endpunkt des bewegten Characters
    // setzen, im Editor selbst beim Ziehen?" Start (Spawner/Ursprung),
    // Laufziele (die Entity jedes SET_NAVGOAL) und Flugziele (der Ort jedes
    // move) der gewaehlten Figur bzw. des Schiffs.
    struct BahnZug {
        bool kandidat = false;
        bool aktiv = false;
        int art = -1;                  // 0 Start, 1 Laufziel, 2 Flugziel
        int entity = -1;               // Start: Spawner/Schiff; Laufziel: die Ziel-Entity
        Path pfad;                     // Laufziel: SET_NAVGOAL; Flugziel: move
        std::vector<MapEntity> vorher;
        float startOrt[3] = {0.0F, 0.0F, 0.0F};
        float ort[3] = {0.0F, 0.0F, 0.0F};
        float von[3] = {0.0F, 0.0F, 0.0F};   // wo das Stueck anfaengt (fuer die Vorschaulinie)
        float startMaus[2] = {0.0F, 0.0F};
        double letzterBau = 0.0;
        // Waehrend des Ziehens wird nur die Bahn DIESER Figur im Speicher
        // angepasst (rc597) - die ganze Szene neu zu rechnen kostete bis
        // 16 fps, und das Modell hinkte dem Griff hinterher. Neu gerechnet
        // wird einmal beim Loslassen.
        int figur = -1;
        std::vector<ActorStep> stepsVorher;
        float startVorher[3] = {0.0F, 0.0F, 0.0F};
        int szeneBautenBeimZug = 0;   // Selbsttest: keine Neuberechnung waehrend des Zuges
    };
    BahnZug bahnZug;
    int bahnGriffHover = -1;
    Path bahnMenuePfad;              // der SET_NAVGOAL, dessen Gangart das Menue setzt
    int bahnMenueBild = -1;          // in diesem Bild hat ein Griff den Rechtsklick genommen
    // Die Bewegungsbalken der Zeitleiste in diesem Bild (Selbsttest).
    struct TlBalken {
        Path pfad;
        ImVec2 a{0.0F, 0.0F};
        ImVec2 b{0.0F, 0.0F};
        bool gehen = false;
        int figur = -1;
    };
    std::vector<TlBalken> tlBalken;
    int tlRollFigur = -1;            // zu dieser Figurenzeile ist die Zeitleiste schon gerollt
    // Aufklappbare Darstellerzeilen (rc597, zeitleiste1): welche Figuren in
    // der Uebersicht aufgeklappt sind (Animation und Befehle darunter).
    std::set<std::string> tlOffen;
    // Die Maus stand im letzten Bild ueber der Zeitleiste (rc599): dort
    // zoomt Strg+Rad die Zeitleiste, die Oberflaeche bleibt gleich gross.
    bool tlUnterMaus = false;
    // Auto-Key der Kamera (rc597): beim Anfassen des Gizmos mitten in einer
    // Fahrt wurde dort ein Schluessel eingefuegt (wird ohne Aenderung beim
    // Loslassen zurueckgenommen).
    bool kameraAutoKeyEingefuegt = false;
    std::size_t kameraAutoKeyTiefe = 0;   // mission1: Rueckgaengig-Tiefe vor dem Auto-Key (ein Schritt mit den Werten)
    bool kameraGizmoZwischen = false;   // der Gizmo sitzt gerade an der laufenden Kamera (mitten in einer Fahrt)
    bool kamBahnNurAktiv = true;        // nur die Fahrt um die eingestellte Zeit zeigen (rc597, "Aktiv")
    int kamBahnFarbig = 0;              // Selbsttest: Stuecke der Kamerabahn, gelb (gefahren) / grau (voraus)
    int kamBahnGrau = 0;
    int tlWaitBloecke = 0;           // Selbsttest: so viele wait-Bloecke wurden in diesem Bild gezeichnet
    float tlKopfX = 0.0F;            // linke Kante der Spurkoepfe (Selbsttest: Pfeil anklicken)
    float tlZeilenH = 0.0F;
    int vorschauBilder = 0;         // wie oft gezeichnet (Selbsttest)
    // Himmelsportal (rc593): gerade im Durchgang vom Portalort aus (dort kein
    // Wetter), und ob die Ansicht zuletzt mit Portalhimmel gezeichnet wurde.
    bool imPortalDurchgang = false;
    bool himmelPortalGezeichnet = false;
    // Spiegel (rc593): gerade im Spiegeldurchgang (Ebene n.p = d in
    // spiegelEbene), und ob die Ansicht zuletzt mit Spiegelbild gezeichnet
    // wurde (Selbsttest).
    bool imSpiegelDurchgang = false;
    float spiegelEbene[4]{0.0F, 0.0F, 1.0F, 0.0F};
    bool spiegelGezeichnet = false;
    // Die Abbildung des Spiegel- oder Portaldurchgangs (rc593): die Welt
    // wird mit `welt` (4x4, zeilenweise) transformiert, Richtungen (Himmel)
    // mit `achsen` (3x3); `schnitt` ist die Schnittebene, `umkehren` die
    // Wicklung (nur Spiegel). Siehe spiegelFuer.
    struct NebenSicht {
        float welt[16]{};
        float achsen[9]{};
        float schnitt[4]{0.0F, 0.0F, 0.0F, 1.0F};
        bool umkehren = false;
        bool portal = false;
        // rc609 P: newParms.ori.origin und ori.axis[0] (R_MirrorPoint/R_MirrorVector) und pvsOrigin (oldorigin)
        float auge[3]{};
        float vorn[3]{};
        float pvsOrt[3]{};
    };
    NebenSicht nebenSicht;
    // Durch die gewaehlte Kamera schauen statt frei zu fliegen.
    bool throughCamera = false;
    // Kamerabahn und Abspielzustand
    CameraTrack camTrack;
    bool camTrackValid = false;
    // Die Kamera, mit der die Kartenansicht im letzten Bild WIRKLICH
    // gezeichnet wurde (freie Kamera, Kamerabahn oder "Through camera") -
    // und das Rechteck der Ansicht auf dem Schirm. Fuer den Selbsttest: er
    // prueft das Bild, nicht nur die Rechnung.
    Camera gezeigteKamera;
    float kartenAnsicht[4] = {0, 0, 0, 0};   // x0, y0, x1, y1
    double playMs = 0.0;
    bool playing = false;
    bool followCam = true;
    // Abspieltempo (0.25 .. 2) und Endlosschleife - wie im Sequencer. Bei
    // einem anderen Tempo als 1 bleibt der Ton aus: ein Klang liesse sich
    // nicht mitverlangsamen und liefe dem Bild davon.
    float abspielTempo = 1.0F;
    // Steht die Maus ueber der Zeitleiste? Dann gehoeren ihr die Pfeiltasten
    // (Bildschritt), nicht dem Baum. Aus dem vorigen Bild.
    bool tlUeberMaus = false;
    int tlSchritte = 0;   // H2: Bildschritte mit den Pfeiltasten (Tour k-neu-timen)
    // Welche Dauerklaenge (Schwertsummen, Lautsprecherschleifen) gerade
    // laufen - Schluessel des Mischers. Leer nach jedem stopAll.
    std::set<std::uint64_t> laufendeSchleifen;
    bool schleife = false;
    // Figuren in der Karte
    Scene scene;
    bool showActors = true;
    // Effekte zeigen. An, weil eine Karte ohne ihren Rauch anders aussieht
    // als im Spiel - und genau darum geht es bei einer Vorschau.
    bool showEffects = true;
    // Das Gluehen zeigen - der zweite Durchgang ueber die Stufen mit `glow`.
    //
    // Vorgabe wie r_DynamicGlow im Regelwerk (regelwerkAnwendenJetzt): Movie
    // Duels an, OpenJK und Original aus. Ohne das Gluehen sieht eine Karte
    // unter Movie Duels flacher aus als dort: 1026 der 4107 Shader tragen
    // `glow`, bei MD_Maps_Ep3 sind es 342 von 736 - Lichtsaeber, Konsolen,
    // Lava, Triebwerke, der Holotisch in md_am_sith.
    //
    // Abschaltbar, weil es einen zweiten Zeichendurchgang kostet und weil
    // man beim Suchen eines Fehlers das rohe Bild sehen will.
    bool showGlow = true;
    // Die Gebiete der Karte (bhed/areaportal.h), Stand fuer die Zeit der Ansicht und das Gebiet des
    // Auges (areaPortaleJetzt).
    AreaPortale areaPortale;
    std::vector<PortalEntity> portalEnts;
    std::string portalEntsKarte;
    std::map<std::string, HolsterCfg> holsterCfgs;   // cg_holster.cpp: je Spielermodell
    std::vector<char> portalOffen;
    std::map<std::pair<int, int>, std::vector<char>> areaMasken;   // je Gebiet und Cluster des Auges (R_MarkLeaves)
    int areaAuge = -2;
    int areaCluster = -2;
    // Endrunde rc608: die Kaesten der Flaechen (R_CullSurface) und was zuletzt hinter der fernen Ebene lag (areaSichtSetzen)
    std::vector<FlaechenKasten> fernKaesten;
    std::string fernKaestenKarte;
    FernStand fernZuletzt;
    // Die Schattenart wie cg_shadows: 0 aus, 1 der runde Fleck (markShadow,
    // ab Werk), 2 Schattenvolumen im Stencil-Puffer, 3 die Figur schwarz auf
    // den Boden gedrueckt - siehe gpu::zeichneLichtUndSchatten und
    // gpu::zeichneFigurSchatten. Dazu dynamisches Licht auf Waenden und Boden
    // (r_dynamiclight), ebenfalls ab Werk an.
    int schattenArt = 1;
    bool showDynLights = true;
    // rc608: "Show player" - an: Spielerfigur in Szenen mit Kameraspur wie im Spiel.
    bool spielerInCutscene = false;
    // Nebel aus der Karte (r_drawfog 1, RB_FogPass) - siehe gpu::zeichneNebel.
    bool showFog = true;
    int nebelAufrufe = 0;
    // Die Schwertlichter des letzten Bildes. Die Figuren bekommen ihr Licht
    // VOR dem Zeichnen, die Klingen stehen erst danach fest - ein Bild
    // Verzug, den niemand sieht. Die Engine rechnet alle Lichter der Szene
    // auf jede Figur (R_SetupEntityLighting), also auch das eigene Schwert.
    std::vector<EffectLight> schwertLichter;
    // Fuer die Seitenleiste: wie viele Lichter und Schatten im letzten Bild.
    int lichtAufrufe = 0;
    int schattenAnzahl = 0;


    // Welche Liste hat die Auswahl zuletzt bekommen?
    //
    // 0 = keine, 1 = Ereignisliste, 2 = Skriptbaum.
    //
    // Windows zeigt die Auswahl einer Liste, die NICHT den Fokus hat,
    // gedaempft statt in der Betonungsfarbe. Genau das fehlte hier: wer in
    // der Ereignisliste etwas anklickte und danach im Baum arbeitete, sah
    // beide Markierungen gleich kraeftig und wusste nicht mehr, welche
    // gilt.
    int selectionOwner = 0;
    // Diagnose: fester Zuschlag auf die Blickrichtung der Figuren, in Grad.
    // Warum es ihn gibt, steht bei ActorDraw::yawOffset in bhed/mapview.h.
    int actorYaw = 0;
    // Die im Bild angeklickte Entity.
    //
    // SomaZ' BSP-Entity-Edit kann das ueber einen Nebenpuffer mit Kennungen
    // (ogl_fbo.py). Unser Zeichner ist Software, da genuegt es, die Marken
    // beim Zeichnen samt Bildschirmort zu merken und beim Klick die
    // naechstgelegene zu nehmen.
    struct PickedMarker {
        float sx = 0.0F;
        float sy = 0.0F;
        std::size_t entity = 0;
        // Steht eine Wand zwischen Kamera und Entity? Dann zeichnet die
        // Ansicht ihr Icon nur blass (rc592) - wie Unreal Sprites hinter
        // Waenden verbirgt. Geprueft beim Neuzeichnen, nicht je Bild.
        bool verdeckt = false;
    };
    std::vector<PickedMarker> markers;
    // Namen an Entities und Figuren in der Ansicht (wie die Beschriftungen
    // im Unreal-Viewport). Die Figuren-Stellen entstehen beim Zeichnen.
    bool zeigeNamen = false;
    struct NamensMarke {
        float sx = 0.0F;
        float sy = 0.0F;
        std::string name;
    };
    std::vector<NamensMarke> figurNamen;

    // Ist die Kamera angeklickt?
    //
    // Ist sie es nicht, steht sie einfach da, wo sie zur eingestellten Zeit
    // ist - EINE Kamera, wie in 3ds Max oder Maya. Erst wenn man sie
    // anklickt, erscheint ihre Bahn mit den Schluesselstellen. Genau so
    // macht es Max: "By default, an object's trajectory is visible in the
    // viewports only when the object is selected."
    bool cameraSelected = false;
    // Welcher Schluessel auf der Bahn ist angeklickt? -1 = keiner.
    //
    // Ein Schluessel ist ein weisses Quadrat auf der Bahn und entspricht
    // GENAU einer Zeile im Skript: dem camera(MOVE)-Befehl, dessen Pfad im
    // Shot mitgefuehrt wird. Diese Zuordnung ist die Grundlage fuer alles
    // Weitere - ohne sie kann man zwar etwas verschieben, aber nicht
    // sagen, was danach im Skript stehen soll.
    int selectedKey = -1;
    // Welcher Schluessel wurde zuletzt ins Detailprotokoll geschrieben?
    // Verhindert, dass die Zeilen jedes Bild neu anfallen.
    int loggedKey = -2;
    // Wo die Schluessel zuletzt auf dem Bildschirm sassen.
    struct KeyMark {
        float sx = 0.0F;
        float sy = 0.0F;
        bool imBild = true;   // ausserhalb: nicht anklickbar, Platz bleibt
        Path path;
    };
    // Die Zeit, zu der der bearbeitete Schluessel zuletzt gewaehlt wurde.
    // Bewegt sich die Zeitleiste davon weg, geht der Schluessel mit (siehe
    // schluesselMitZeit) - ausser es steht eine ungeschriebene Aenderung an.
    double gizmoZeitBezug = -1.0;
    // Stand im letzten Bild eine Geisterkamera am Zielpunkt? (Selbsttest)
    bool geisterKamera = false;
    std::vector<KeyMark> keyMarks;

    // --- Das Gizmo -------------------------------------------------------
    //
    // Drei Achsen am ausgewaehlten Schluessel, wie in 3ds Max: X rot, Y
    // gruen, Z blau. Ziehen an einer Achse verschiebt NUR entlang dieser
    // Achse.
    //
    // Was hier NOCH NICHT geschieht: ins Skript zurueckschreiben. Die
    // Verschiebung steht bis auf Weiteres nur in gizmoOffset und ist
    // sichtbar, aber folgenlos. Das ist Absicht - erst soll sich das
    // Anfassen richtig anfuehlen, und erst danach fasse ich das
    // Bearbeitungsmodell an, an dem roundtrip und edittest haengen.
    // Was tut ein Klick? Wie in 3ds Max drei Betriebsarten, umschaltbar
    // ueber das Rechtsklickmenue.
    //
    //   Select      nur auswaehlen, kein Gizmo im Bild
    //   Move        die drei Achsen, Ziehen verschiebt
    //   Rotate      spaeter - Drehen heisst den PAN-Befehl aendern, und
    //               der kommt zusammen mit dem Rueckschreiben
    enum class GizmoMode : std::uint8_t { Select, Move, Rotate };

    // Bezugssystem der Griffe - wie die "Reference Coordinate System"-Liste
    // in 3ds Max.
    //
    //   Welt  - die Achsen liegen fest an X, Y, Z der Karte. Zum Setzen
    //           einer Kamera an einen bestimmten Ort ist das das
    //           natuerliche System: "zehn Einheiten weiter nach Norden".
    //   Lokal - die Achsen haengen an der Kamera: vorwaerts, rechts, oben.
    //           Zum Nachjustieren einer Einstellung besser: "einen Schritt
    //           zurueck", "etwas nach oben schwenken".
    //
    // Max fuehrt fuer JEDE Umformung eine eigene Einstellung ("If you
    // change this pull down list for move, that does not change it for
    // rotate"). Deshalb hier zwei Felder und nicht eines.
    enum class GizmoSpace : std::uint8_t { World, Local };
    // Vorgabe: AUSWAEHLEN.
    //
    // Mit Verschieben als Vorgabe ist jeder Klick in die Naehe einer Achse
    // ein Verschieben, bevor man ueberhaupt etwas ausgewaehlt hat. Max
    // startet ebenfalls mit dem Auswahlwerkzeug.
    // Seit rc588 VERSCHIEBEN: das Gizmo erscheint erst, wenn die Kamera
    // gewaehlt ist - wie bei den Entities; ein Klick daneben waehlt nicht aus
    // Versehen etwas anderes.
    GizmoMode gizmoMode = GizmoMode::Move;
    // Verschieben in Weltachsen - so war es bisher, und so erwartet man es
    // beim Setzen eines Ortes.
    GizmoSpace gizmoMoveSpace = GizmoSpace::World;
    // Drehen um die Kameraachsen - das entspricht pitch/yaw/roll des
    // PAN-Befehls unmittelbar, ohne Umrechnung.
    GizmoSpace gizmoRotSpace = GizmoSpace::Local;
    // Die drei Achsenrichtungen, EINGEFROREN beim Anfassen. Im Weltsystem
    // sind es X, Y, Z; im lokalen rechts, oben, vorwaerts. Eingefroren aus
    // demselben Grund wie in rc156: eine Achse, die mit dem gezogenen Punkt
    // mitwandert, koppelt zurueck.
    float gizmoAxisDir[3][3]{};
    // Dieselben Richtungen, wie sie zuletzt GEZEICHNET wurden, und die
    // Winkel des gewaehlten Schluessels dazu. Beim Anfassen wird daraus
    // eingefroren - dieselbe Arbeitsteilung wie bei gizmoSx/gizmoRingSx:
    // das Zeichnen weiss, wo etwas liegt, der Griff liest es ab.
    float gizmoDirNow[3][3]{};
    float gizmoAngNow[3]{};
    // Die Winkel der Kamera beim Anfassen - Grundlage der Weltdrehung.
    float gizmoAngBase[3]{};
    int gizmoAxis = -1;            // -1 = wird nicht gezogen, sonst 0/1/2
    float gizmoOffset[3]{};        // Vorschau-Verschiebung des Schluessels
    // Wo die Achsen zuletzt auf dem Bildschirm lagen - fuer den Griff und
    // fuer die Umrechnung Mausweg -> Weltweg.
    float gizmoSx[3][2]{};
    float gizmoSy[3][2]{};
    float gizmoWorldLen = 0.0F;    // Laenge einer Achse in Welteinheiten
    // Die Drehringe, auf den Bildschirm projiziert - ein Ring je Achse,
    // 48 Teile plus Schlusspunkt. Der Ueberfahr-Test im Drehmodus misst
    // gegen DIESE Punkte. Vorher mass er gegen die Achsenstrecken des
    // Verschiebens, die im Drehmodus gar nicht gezeichnet werden - deshalb
    // war der Ring "schwer greifbar": getroffen wurde eine unsichtbare,
    // veraltete Linie. Ungueltige Punkte stehen auf -1.
    static constexpr int kRingPts = 49;
    float gizmoRingSx[3][kRingPts]{};
    float gizmoRingSy[3][kRingPts]{};
    bool gizmoShown = false;       // steht ueberhaupt eines im Bild?
    float gizmoDragSx = 0.0F;      // Mausort beim Griff
    float gizmoDragSy = 0.0F;
    float gizmoStart[3]{};         // gizmoOffset beim Griff
    // Die Achse, EINGEFROREN beim Griff.
    //
    // Das ist die Behebung des Fehlers aus dem GIF: "es gibt beim moven
    // auch einen gewissen punkt wo es out of range ist und sich nicht mehr
    // zurueck bewegen laesst."
    //
    // Vorher rechnete das Ziehen mit den Achsen, WIE SIE GERADE STEHEN. Die
    // wandern aber mit dem Punkt mit - eine Rueckkopplung: der Punkt
    // bewegt sich, die Achse dreht und verkuerzt sich in der Perspektive,
    // derselbe Mausweg bedeutet plotzlich mehr, der Punkt bewegt sich
    // weiter. Und laeuft er hinter die Kamera, misslingt das Projizieren,
    // die Achse hat keine Bildlaenge mehr - und nichts geht mehr, auch
    // nicht zurueck.
    //
    // 3ds Max macht es anders und richtig: der Bezug wird beim Anfassen
    // festgelegt und gilt fuer den ganzen Zug. Genau das steht hier.
    float gizmoFixSx[2]{};
    float gizmoFixSy[2]{};
    float gizmoFixLen = 0.0F;
    // Welche Achse? 0/1/2, und 3 = das Kaestchen in der Mitte.
    //
    // Max nennt es "center box": "You can constrain translation to the
    // viewport plane by dragging the center box." Damit bewegt man in der
    // Bildebene, also in alle Richtungen zugleich - das war der Wunsch.
    static constexpr int kGizmoFree = 3;
    // Der Drehring um die Hochachse, auch im Verschieben (rc597, wie bei den
    // Figuren: Pfeile und Ring zugleich).
    static constexpr int kGizmoYawRing = 4;
    bool kamYawRingInMove = false;   // dieser Ring liegt gerade im Bild (gizmoRingSx[2])
    bool kamRingAusMove = false;     // gedreht wird ueber diesen Ring - danach zurueck ins Verschieben
    // Rechts und oben der Kamera beim Griff, fuer die freie Bewegung.
    float gizmoFreeRight[3]{};
    float gizmoFreeUp[3]{};
    float gizmoFreeScale = 0.0F;   // Welteinheiten je Bildpunkt
    // Ueber welcher Achse steht die Maus gerade? Nur zum Aufleuchten.
    int gizmoHover = -1;
    // Die PAN-Werte des ausgewaehlten Schluessels, als Vorschau.
    //
    // Wie gizmoOffset, aber fuer das Drehen: drei Winkel, die auf die
    // bestehenden addiert werden.
    float gizmoAngles[3]{};
    // Wird gerade gedreht? Dann der Mausort und der Ausgangswert.
    bool gizmoRotating = false;
    float gizmoRotStartSx = 0.0F;
    float gizmoRotStartSy = 0.0F;
    float gizmoRotStart[3]{};
    // Wo die Kamera zuletzt auf dem Bildschirm sass - fuer den Klick.
    float cameraSx = -1.0F;
    float cameraSy = -1.0F;
    int pickedEntity = -1;
    // Ob gerade durch die Kamera geschaut wird, und welcher Befehl dahinter
    // steht. Wird beim Zeichnen der Ansicht ermittelt und im Band angezeigt
    // - deshalb im Programmzustand und nicht oertlich.
    bool camViewActive = false;
    std::string camViewLabel;
    // Warum keine Figurenmodelle da sind - im Klartext, nicht als pauschaler
    // Hinweis. Die Kette hat vier Glieder, und jedes kann reissen.
    std::string actorWhy;
    float camViewFov = 90.0F;

    // Sichtbarkeit einzelner Entities und ganzer Klassen.
    //
    // Nach dem Vorbild des Ebenen-Verwalters von 3ds Max: eine Klasse ist
    // eine Ebene, das Auge davor schaltet sie um, und ein Alt-Klick stellt
    // sie allein (Solo). Eine einzelne Entity kann zusaetzlich fuer sich
    // ausgeblendet werden - versteckt eine Ebene, gilt das fuer alle ihre
    // Mitglieder, wie in Max auch.
    std::set<std::string> hiddenClasses;
    std::set<std::size_t> hiddenEntities;
    std::string expandedClass;

    [[nodiscard]] bool entityVisible(std::size_t index,
                                     const std::string& classname) const {
        return hiddenClasses.count(classname) == 0 &&
               hiddenEntities.count(index) == 0;
    }
    // Je Figur ein geladenes Modell samt Texturen. Der Index passt zu
    // scene.actors.
    // Skelett und Abschnittsliste einer .gla.
    //
    // Getrennt von den Figuren, weil sich fast alle dieselbe teilen:
    // Spielermodelle zeigen samt und sonders auf
    // models/players/_humanoid/_humanoid. Einmal lesen genuegt.
    struct Skeleton {
        GlaAnimation anim;
        std::vector<AnimEntry> sections;   // aus der animation.cfg
        // Die Abschnitte des Kino-Skeletts der Karte (Bilder hinter den eigenen,
        // siehe GlaAnimation::anhang) ersetzen gleichnamige und kommen sonst
        // dazu. Die eigenen gemerkt, und fuer welche Karte der Anhang gilt.
        std::vector<AnimEntry> grundListe;
        bool grundGemerkt = false;
        std::string kinoKarte;   // "<karte>|<regelwerk>" (pruefung1)
    };
    // Schluessel ist der Pfad aus dem Modellkopf, ohne Endung.
    std::map<std::string, Skeleton> skeletons;
    // Schon gemeldete Paare aus Skelett und unbekanntem Animationsnamen.
    // Ohne dieses Gedaechtnis stuende die Meldung in JEDEM Bild im
    // Protokoll - sechzig Zeilen je Sekunde.
    std::set<std::string> unknownAnims;
    // Entschluesselte Klaenge, fuer die Mundanimation.
    //
    // playSound() entschluesselt bei jedem Abspielen neu - das geht,
    // solange es einmal je Klang passiert. Der Mund braucht die
    // Abtastwerte aber in JEDEM Bild, und eine .wav je Bild neu zu
    // entschluesseln waere nicht tragbar.
    //
    // Ein Eintrag ohne ok heisst: schon versucht, geht nicht - dann wird
    // es nicht in jedem Bild erneut versucht.
    std::map<std::string, sound::Sound> soundCache;
    // Dieselben Klaenge, schon auf das Ausgabeformat (44,1 kHz Stereo)
    // umgerechnet. playSound entpackte und rechnete bisher bei JEDEM
    // Abspielen neu - 130 bis 215 ms je Klang, waehrend das Bild stand.
    // rc601 ton: der Mischer liest DIREKT aus diesen Eintraegen (tonDaten,
    // ohne Kopie) - nur anlegen, nie loeschen oder ersetzen, solange das
    // Tongeraet Stimmen hat; und vor `audio` deklariert lassen.
    std::map<std::string, std::vector<std::int16_t>> klangPuffer;
    // So viele Bilder zaehlen nach dem Start des Abspielens nicht als
    // Abspielzeit: die Zeit fuers Vorladen der Klaenge wuerde sonst auf
    // einmal draufgeschlagen, und der Anfang der Sequenz waere uebersprungen.
    int uhrAussetzen = 0;
    // Bis wann der Compile-Knopf "Compiled" (gruen, Haken) bzw. "Failed"
    // (rot) zeigt - ImGui::GetTime(). shank, 27.09.: Uebersetzen geht so
    // schnell, dass man nicht sieht, ob es passiert ist.
    double kompiliertBis = 0.0;
    double kompiliertFehlerBis = 0.0;

    // Die Netze der bewegten Brush-Modelle, nach Untermodellnummer.
    // Einmal gebaut, dann je Bild nur noch verschoben.
    std::map<int, BspMesh> brushMeshes;

    // Die Effekte der Karte: fx_runner-Entities und ihre .efx.
    //
    // Ueber die 23 Karten von MD_Maps_Ep1/2/4/6 sind das 374 Stueck - nach
    // waypoint und info_null die haeufigste Entity ueberhaupt. Rauch,
    // Funken, Dampf; ohne sie sieht ein Hangar aus wie ein leerer Raum.
    std::map<std::string, efx::Effect> effects;   // Pfad -> Inhalt
    std::vector<EffectInstance> effectRunners;    // was in DIESER Karte laeuft
    // Wie viele davon beim Zerbrechen spielen (MoverSim::bruchEffekte) -
    // sie haengen wie die per use gestarteten fx_runner am Skript.
    std::size_t bruchFxZahl = 0;
    // rc604: misc_trip_mine - laserTrapThink (code/game/wp_trip_mine.cpp:135-146
    // OpenJK) beim Scharfmachen: G_Sound warning.wav und s.loopSound hum_loop.wav.
    std::vector<KartenKlang> fallenKlaenge;
    // rc604: misc_trip_mine - wann das Modell zu sehen ist (laserfalleLauf):
    // Entity -> ab (EF_NODRAW weg) .. bis (G_FreeEntity, < 0 offen). Ohne
    // Eintrag: immer.
    std::map<std::size_t, std::pair<double, double>> fallenSicht;
    // rc604: wie viele misc_trip_mine im Vorschauzeitraum explodieren (WP_Explode).
    std::size_t fallenExplosionen = 0;
    // rc605 (fix605): WP_Explode ruft G_UseTargets( self, attacker ) (g_weapon.cpp:
    // 221-224 OpenJK) - die Ziele explodierender Fallen fuer die Mover-Simulation
    // (unaufgeloest: target_counter, target_relay rechnet MoverSim). fallenUses
    // gingen in die letzte moverSim.baue (zur Karte fallenUsesKarte), fallenUsesNeu
    // kommen aus dem letzten fallenEffekteSetzen.
    // rc605: die Uses gehen in den Nachbau (AblaufWelt::fallenUses), der
    // Zaehler und usescript rechnet und MoverSim die aufgeloesten gibt.
    std::vector<AblaufWelt::FallenUse> fallenUses;
    std::string fallenUsesKarte;
    std::vector<AblaufWelt::FallenUse> fallenUsesNeu;
    // Rc608: die Geschuetze der Karte (app_view3d_geschuetz.inc). Was ihr Lauf in
    // hurtTodeBerechnen ergab, zaehlt im naechsten Durchgang: G_UseTargets, was G_Damage
    // zerbricht, und Laserfallen, die er sprengt (Entity -> ms) - je fuer diese Karte.
    GeschuetzSim geschuetzSim;
    double geschuetzBildMs = 0.0;     // Zeit des laufenden Bilds (fuer gi.pointcontents)
    int geschuetzSchuesse = 0;
    int geschuetzEinschlaege = 0;
    std::vector<AblaufWelt::FallenUse> geschuetzUses;
    std::vector<AblaufWelt::FallenUse> geschuetzUsesNeu;
    std::vector<AblaufWelt::SchadenBruch> schadenBrueche;
    std::vector<AblaufWelt::SchadenBruch> schadenBruecheNeu;
    // Rc608 Teil 6: G_ApplyKnockback und Schmerz aus hurtTodeBerechnen, gelten im naechsten Durchgang.
    std::map<std::string, std::vector<LaufStoss>> stoesseNeu;
    std::function<double(const std::string& figur, const std::string& anim)> animDauerFn;
    std::vector<SchmerzAufruf> schmerzenNeu;
    std::vector<SchmerzAufruf> schmerzen;
    std::shared_ptr<const std::map<std::string, std::vector<std::shared_ptr<const LaufPlan>>>> stossWege;
    std::string stossSchluessel;
    std::string schadenKarte;
    std::set<int> schadenZiele;       // takedamage: func_breakable, misc_model_breakable, misc_trip_mine
    std::map<int, double> fallenFremd;
    std::map<int, double> fallenFremdNeu;
    std::string fallenFremdKarte;
    // rc605 (Ende): die Explosionen der Laserfallen (WP_Explode) - G_RadiusDamage
    // trifft auch Figuren (hurtTodeBerechnen). fallenKnalle aus dem letzten
    // fallenEffekteSetzen zur Karte fallenKnalleKarte, fallenKnalleNeu von jetzt.
    struct FallenKnall {
        double ms = 0.0;
        float ort[3]{};
        int entity = -1;
        bool operator==(const FallenKnall&) const = default;
    };
    std::vector<FallenKnall> fallenKnalle;
    std::string fallenKnalleKarte;
    std::vector<FallenKnall> fallenKnalleNeu;
    // Pruefung 4b: die Treffer von fx_target_beam auf Figuren (G_Damage mit
    // "damage") - wie die Explosionen aus dem letzten Durchgang.
    struct StrahlTreffer {
        double ms = 0.0;
        std::string figur;
        int schaden = 0;
        int entity = -1;
        int ziel = -1;   // Rc608: statt einer Figur eine Kartenentity (Geschuetz, Zerbrechliches)
        bool operator==(const StrahlTreffer&) const = default;
    };
    std::vector<StrahlTreffer> strahlTreffer;
    std::string strahlTrefferKarte;
    std::vector<StrahlTreffer> strahlTrefferNeu;
    // rc605 (Ende): die Figuren, die MoverSim::blockiert meldet (Index ->
    // Name), fuer MoverSim::quetschungen in hurtTodeBerechnen.
    std::vector<std::string> quetschFiguren;
    std::size_t fallenKettenExplosionen = 0;   // rc605 (Ende): von anderen Fallen gesprengt
    // rc605 (Ende): die Tode durch trigger_hurt (hurtTodeBerechnen) zu Karte und
    // Skript hurtTodeSchluessel - der naechste Nachbau desselben Skripts toetet
    // sie (AblaufWelt::hurtTode).
    std::vector<AblaufWelt::HurtTod> hurtTode;
    std::string hurtTodeSchluessel;
    // ... und wie oft eine Figur getroffen war: am Netz (G2_RadiusTracePolys)
    // bzw. nur am Kasten, weil ihr Modell noch nicht geladen war.
    int fallenNetzTreffer = 0;
    int fallenKastenTreffer = 0;
    // rc605 (Rest): Spuren, die an einer anderen Falle, einem misc_model_breakable
    // oder einem Modell am Bolzen einer Figur endeten (laserfalleSpurBei).
    int fallenFallenTreffer = 0;
    int fallenModellTreffer = 0;
    int fallenAnbauTreffer = 0;
    // rc604: Knochenanimationen der Geschuetze (misc_sentry_turret klappt auf,
    // misc_ion_cannon je Schuss) - Entity -> Animationen nach Zeit.
    std::map<std::size_t, std::vector<KnochenAnim>> geschuetzAnims;
    // rc605 FU4: misc_sentry_turret ohne Feind - die Gier von bone_hinge nach
    // jedem pas_think (pasSchwenkLauf).
    std::map<std::size_t, std::vector<PasSchwenk>> pasSchwenks;
    std::size_t ionenSchuesse = 0;   // Effekte env/ion_cannon im Vorschauzeitraum
    // Ebenso die Effekt-Notizen der ROFF-Bahnen (roffEffektStarts, rc592).
    std::size_t roffFxZahl = 0;
    // Tueren, Plattformen, func_wall & Co. - siehe bhed/mover.h. Wird in
    // prepareScene aus den use-Ereignissen und den Figurenwegen gebaut.
    MoverSim moverSim;
    // Die Klaenge der Mover (soundSet) und die bmodelSets aus
    // sound/sound.txt, die sie auf Dateien abbilden. Die Saetze werden
    // einmal gelesen; leer und `bmodelSetsGelesen` heisst: nicht gefunden.
    std::vector<MoverKlang> moverKlaenge;
    std::map<std::string, std::vector<std::string>> bmodelSets;
    bool bmodelSetsGelesen = false;
    // Wie viele Effektlichter zuletzt brannten. Nur, damit die Meldung
    // darueber bei jeder AENDERUNG kommt und nicht in jedem Bild - bei
    // sechzig Bildern je Sekunde waere sonst das Protokoll unlesbar.
    std::size_t lastFxLightCount = 0;
    // Ebenso fuer die Schusseffekte - Meldung bei jeder Aenderung, nicht
    // in jedem Bild.
    std::size_t lastShotFxCount = 0;

    // Die zuletzt gemeldete Gesamtzeit je Bild, in Millisekunden.
    //
    // Gefragt: "hast du jetzt eingebaut im Debug-Log, was wie viel kostet
    // und warum?"
    //
    // Bis rc299 nicht - die Aufschluesselung stand nur im
    // Mauszeiger-Kasten und hinter einem Menuepunkt. Damit muss man beim
    // Messen gleichzeitig hinsehen, und beim Abspielen geht das schlecht.
    //
    // Jetzt schreibt sie sich selbst, sobald sich die Gesamtzeit deutlich
    // aendert.
    float lastFrameMsLogged = -1.0F;
    // Die zuletzt gemeldete Effektuebersicht. Nur, damit sie bei einer
    // AENDERUNG erscheint und nicht bei jedem Aufbau der Ansicht.
    std::string lastEffectSummary;

    // --- Wohin die Zeit je Bild geht --------------------------------------
    //
    // Gemeldet: "ich merke, dass ich beim Abspielen nur 30 fps habe. Koennen
    // wir debuggen beim Laufen, was so viel kostet?"
    //
    // Bisher gab es nur die Gesamtzeit je Bild. Damit weiss man, DASS es
    // langsam ist, aber nicht WO - und raten hilft beim Beschleunigen noch
    // weniger als beim Suchen von Fehlern.
    //
    // Alle Werte in Millisekunden, geglaettet wie die Gesamtzeit: eine
    // ungeglaettete Anzeige springt zu stark, um etwas abzulesen.
    struct FrameCost {
        float scene = 0.0F;      // Szene und Figurenzustaende aufbauen
        float effects = 0.0F;    // Effektnetz bauen
        float map = 0.0F;        // Karte zeichnen
        float actors = 0.0F;     // Figuren zeichnen
        float glow = 0.0F;       // zweiter Durchgang und Weichzeichnen
        float gizmos = 0.0F;     // Gizmos und Blende
        float total = 0.0F;
        // Was die Oberflaeche wirklich braucht, aus ImGui. Steht daneben,
        // damit die Luecke zwischen Summe und Wirklichkeit sichtbar bleibt
        // statt sich zu verstecken.
        float frame = 0.0F;
        // --- Dieselben Zahlen UNGEGLAETTET ------------------------------
        //
        // Die geglaetteten oben sind fuer die Anzeige da (eine springende
        // Zahl liest niemand). Zum MESSEN taugen sie nicht, und das hat
        // eine Runde gekostet:
        //
        //     #20 Karte  91,1    #40 152,1    #60 181,9
        //
        // Das sah nach einer Ansammlung aus - als wuerde sich etwas
        // aufstauen. Es war die Glaettung beim Einschwingen: die Zuwaechse
        // halbieren sich (61,0 dann 29,8), der Grenzwert liegt bei rund
        // 210 ms. Nach zwanzig Bildern steht dort erst die Haelfte des
        // wahren Werts.
        //
        // Wer eine geglaettete Zahl protokolliert, misst seine eigene
        // Glaettung mit.
        float rawMap = 0.0F;
        float rawTotal = 0.0F;
        // rc603 fix603: weitere Teile ungeglaettet (Selbsttest "spulen" -
        // welches Teil beim Zurueckspulen teurer wird): die Schwerttakte
        // (saberRechneBis), Funken/Klirren (saberEffekteBei), die
        // Brandspuren (saberSpurenDa/-Anhaengen), das Effektnetz und die
        // Figuren.
        float rawSaber = 0.0F;
        float rawSaberFx = 0.0F;
        float rawSpuren = 0.0F;
        float rawEffects = 0.0F;
        float rawScene = 0.0F;
    };
    FrameCost frameCost;
    // rc603 fix603: eine Pruefsumme dessen, was das letzte Bild gezeigt hat
    // (Effektnetz samt Brandspuren, gezeichnete Klingen, Figuren mit ihren
    // Bildern, bewegte Modelle) und die Zeit, fuer die es gerechnet wurde.
    // Nur mit bildSummeAn (Selbsttest "spulen": Vor- und Zurueckspulen
    // zeigen zur selben Zeit Bit fuer Bit dasselbe).
    // Je Teil eine: 0 Effektnetz (mit Brandspuren), 1 gezeichnete Klingen,
    // 2 Figuren, 3 bewegte Modelle, 4 der Klirrblitz (Durchsicht: er ist seit
    // rc601 saber review 2c ein 2D-Bild nach den Balken, saberBlitzZeichnen2D,
    // und lag damit in keinem der Teile 0-3).
    bool bildSummeAn = false;
    double bildSummeMs = -1.0;
    std::array<std::uint64_t, 5> bildSumme{};
    int bildSummeBild = -1;
    // --- Ist die Karte fertig geladen? ----------------------------------
    //
    // Der GPU-Weg darf das Netz nur anfassen, wenn niemand mehr daran baut.
    // Eine Pruefung auf "nicht leer" reicht dafuer NICHT: `g_app->mesh` wird
    // an mehreren Stellen im laufenden Betrieb neu zugewiesen
    // (app.cpp:7863, app_view3d.cpp:8786 und 8947), und zwischen dem Blick
    // auf `empty()` und dem Lesen der Stapel kann genau das passieren.
    //
    // Gemeldet: Absturz beim Laden, aber NUR wenn der Schalter vorher schon
    // an war. Mit ausgeschaltetem Schalter, oder wenn er nach dem Laden
    // umgelegt wird, laeuft es. Genau dieses Zeitfenster.
    //
    // Die Marke wird bewusst NICHT aus dem Netz abgeleitet, sondern an den
    // Stellen gesetzt, die das Netz austauschen. Was man aus dem Zustand
    // erraten kann, kann man auch falsch erraten.
    bool mapBereit = false;
    // Was der letzte Versuch gemeldet hat. Leer heisst: kein Fehler.
    std::string gpuFehler;
    // Wie viele Aufrufe zuletzt abgesetzt wurden. 0 bei eingeschaltetem
    // Schalter heisst: es ist nichts passiert - und das sieht aus wie eine
    // schwarze Karte, also gehoert es angezeigt.
    int gpuAufrufe = 0;
    // Wie viele Stapel uebersprungen wurden - fehlende Textur oder Shader.
    // Ein Loch in der Karte hat genau hier seine Ursache.
    int gpuUebersprungen = 0;
    // --- Getrennt je Durchgang -------------------------------------------
    //
    // `gpuAufrufe` mischte Karte, Mover und Figuren in einer Zahl. Getrennt
    // sieht man sofort, welcher Durchgang gar nicht laeuft - bei "die
    // Charaktere fehlen komplett" ist das der Unterschied zwischen "wird
    // nicht gerufen" und "scheitert beim Zeichnen".
    int gpuKarte = 0;
    int gpuMover = 0;
    int gpuFiguren2 = 0;
    // rc603: CPU-Zeit im Figurenabschnitt der Kartenansicht (alle
    // Durchgaenge), aufsummiert in ms - der Selbsttest figurleistung liest die
    // Differenz je Bild (Figurstufen: ohne, Bezugsweg, schneller Stufenweg).
    double figurCpuMsSumme = 0.0;
    // Was die Debugschicht seit dem Start gemeldet hat, jede Meldung
    // einmal. Bis rc447 ging sie an den Windows-Debugger und war damit
    // unsichtbar; jetzt steht sie im Bericht.
    std::vector<std::string> d3dMeldungen;
    // Was auf den FIGUREN liegt: Flaechenname, Shader, Mischart.
    //
    // Die schwarzen Vierecke auf dem Hologrammtisch sind Figurenflaechen -
    // welche, war aus dem Bild nicht zu entscheiden, und geraten habe ich
    // in dieser Sache schon zweimal. Der Bericht sagt es jetzt.
    // Je KOMBINATION aus Shader, Skin und Mischart, mit Anzahl.
    //
    // Die erste Fassung zaehlte jede Flaeche einzeln auf und brach bei 60
    // ab - ein einziger Battledroid hat 26, und die Hologramme, wegen derer
    // die Liste gebaut wurde, standen gar nicht mehr drin.
    std::map<std::string, int> figurArten;
    int gpuGluehen = 0;
    // Das Effektnetz wurde bisher nur zu gpuAufrufe addiert und nie einzeln
    // gezaehlt - in der Aufstellung fehlte es damit.
    int gpuEffekte = 0;

    // Die Fenstergroesse, getrennt von der Rastergroesse in mapImage.
    int mapViewW = 0;
    int mapViewH = 0;
    // Wie viele Bilder seit dem letzten Stillstand vergangen sind. 0 heisst
    // "bewegt sich gerade".
    int stillCount = 0;
    // Die Kamera des letzten Bildes, um Bewegung zu erkennen.
    float lastCamPos[3]{};
    float lastCamAng[3]{};
    bool haveLastCam = false;

    bool logFrameCost = false;
    // Nicht jedes Bild eine Zeile: bei 60 Bildern je Sekunde waere das
    // Protokoll in einer Minute unlesbar. Jedes zwanzigste genuegt, um
    // einen Verlauf zu sehen.
    int frameCostTick = 0;
    BspMesh effectMesh;                           // je Bild neu
    // --- rc604: surfaceSprites (gui/app_gras.cpp) ---------------------------
    // Je Shader der Karte die Stufen mit surfaceSprites: ihr Textureintrag
    // (Eckenfarbe, beide Seiten), die Angaben und ob sie additiv mischt.
    struct GrasStufe {
        int platz = 0;
        SurfaceSprite ss;
        std::size_t stufenNr = 0;
        bool additiv = false;
    };
    std::vector<std::vector<GrasStufe>> grasStufen;
    int grasStand = 0;                 // zaehlt jede Aenderung an grasStufen
    BspMesh grasNetz;                  // je Bild neu (baueGrasNetz)
    BspMesh grasNetzNeben;             // Kameravorschau / Himmelsportal (baueGrasNetzNeben)
    int grasViereckeNeben = 0;
    float grasReichweite = 1.0F;       // rangescalefactor der Hauptsicht
    int grasVierecke = 0;              // im letzten Bild gezeichnet (Selbsttest)
    int grasFlaechen = 0;
    int grasNebelAufrufe = 0;          // Nebel ueber den Halmen im letzten Bild
    int grasPatches = 0;               // davon Patches (Dreiecke wie RB_SurfaceGrid)
    bool zeigeGras = true;             // Selbsttest: mit und ohne vergleichen
    // surfaceSprites-Stufen, die beim Laden als flache Schicht auf die
    // Flaeche kamen - die Engine zeichnet sie nie so (tr_shade.cpp:1888 MD).
    int grasFlachSchichten = 0;
    int grasPlatzNull = 0;   // rc605: Shader mit surfaceSprites an Platz 0 (unsichtbar)
    int grasBrushVierecke = 0;     // rc605: davon auf Brush-Modellen (letztes Hauptbild)
    double grasBrushMitteZ = 0.0;  // rc605: ihre mittlere Hoehe (Selbsttest)

    // --- .md3-Modelle der Karte ------------------------------------------
    //
    // 1549 Entities in den Karten der Mod zeigen auf eine .md3 - Konsolen,
    // Kisten, Rohre, Raumschiffe. Ohne sie fehlt der halbe Raum.
    //
    // Gehalten wird je Datei EINMAL das gelesene Modell, dazu je gebrauchtem
    // Bild ein fertiges Netz. Ein Modell mit dreissig Bildern, das
    // abgespielt wird, kostet damit dreissig Netze - aber nur die, die
    // wirklich vorkommen.
    struct MapModel {
        // Pruefung 4b: misc_model_breakable mit "gravity" - die Bahn ab dem
        // ersten Wurf (target4, wirfObjekt); leer: steht.
        std::shared_ptr<const AbwurfBahn> wurf;
        std::string path;
        int frames = 1;
        float origin[3]{};
        // ALLE drei Winkel, wie CG_CreateMiscEntFromGent sie mit
        // AnglesToAxis anwendet - vorher nur die Gier.
        float angles[3]{};
        // Je Achse: "modelscale_vec", darueber "modelscale" (G_SpawnFloat,
        // ungleich null). Vorher wurde die Groesse gelesen und nie benutzt.
        float scale[3]{1.0F, 1.0F, 1.0F};
        int shaderBase = 0;   // erste Nummer in textures.byShader
        // Die Nummer in map.entities - damit ein misc_model_breakable
        // nach seinem Bruch das Schadensmodell zeigen oder verschwinden
        // kann (MoverSim::modellAt).
        std::size_t entity = 0;
        // Eine MD3-Figur (NpcDef::legsModel): steht im ersten Bild, statt
        // alle Bilder der Datei durchzulaufen.
        bool standbild = false;
        // "model2" eines Brush-Movers (rc592): das md3 faehrt mit dem
        // Brush-Modell *anBrush mit (g_mover.cpp: s.modelindex2, CG_Mover
        // zeichnet es an lerpOrigin mit lerpAngles). 0 = ein normales
        // Kartenmodell.
        int anBrush = 0;
        // misc_model_static: zoffset + 1 ueber origin steht das Licht (CG_DrawMiscEnts).
        float lichtHoehe = 0.0F;
    };
    std::map<std::string, Md3Model> md3Files;          // Pfad -> Modell
    std::map<std::string, BspMesh> md3Meshes;          // "Pfad#Bild" -> Netz
    std::vector<MapModel> mapModels;
    // rc602 kartenpruefung: Gegenstaende (item_*, weapon_*, ammo_*). Die mit
    // .md3 stehen als MapModel in mapModels (gegenstand = true); die mit .glm
    // zeichnet Ghoul2 starr mit G2_GenerateWorldMatrix(s.angles) - hier.
    struct GegenstandGlm {
        std::size_t entity = 0;
        std::string pfad;
        float welt[16]{};
        float ort[3]{};
        bool versteckt = false;   // nur Selbsttest
        // rc604 fix604b (B3): EF_SHADER_ANIM - das animMap-Bild s.frame statt
        // nach der Zeit (R_BindAnimatedImage mit RF_SETANIMINDEX); -1: nach Zeit.
        int bildIndex = -1;
        // Pruefung 4b: CG_Item "saber pickup item needs to be more visible"
        // (IT_WEAPON/WP_SABER ohne ITMSF_NOGLOW) - Huelle, Sprites, Licht;
        // oben = ent.axis[2] aus spinAngles.
        bool schwertLeuchten = false;
        float oben[3]{0.0F, 0.0F, 1.0F};
    };
    std::vector<GegenstandGlm> gegenstandGlm;
    // Pruefung 4b: die Huelle (gfx/effects/solidWhite_cull, RF_RGB_TINT) je Modell.
    std::map<std::string, ModelTextures> schwertHuellen;
    // Pruefung 3: gezeichnete Abwuerfe (Scene::abwuerfe) seit dem letzten Nullen (Selbsttest).
    int abwurfGezeichnet = 0;
    // Je Kartenmodell die Figur der Szene, die es bewegt (move, rotate,
    // PLAY_ROFF, SET_INVISIBLE, remove) - oder -1. Nach jedem Szenenaufbau.
    std::vector<int> modellFigur;
    // .md3, die erst ein use braucht: Schadens- und Benutzmodelle (_d1/_u1)
    // und die Bruchstuecke (MoverSim::nebenModelle). Pfad -> erste Nummer
    // in textures.byShader; das Modell selbst steht in md3Files.
    std::map<std::string, int> nebenModelle;
    // Was beim Laden der Karte NICHT gefunden wurde - fuer den Selbsttest
    // (allekarten), der prueft, ob es doch irgendwo in den Archiven liegt.
    std::vector<std::string> fehlendeTexturen;
    std::vector<std::string> fehlendeModelle;

    // --- Modelle, die an einem Emitter haengen --------------------------
    //
    // "Emitters don't draw themselves, but they may need to add an
    // attached model" (FxPrimitives.cpp:1375). Der Rauch war rc323; hier
    // kommt der Koerper dazu - bei probehead.efx der fliegende
    // Droidenkopf selbst.
    //
    // 70 Vorkommen in den 376 Effektdateien.
    //
    // Geladen wird beim Missionsstart, nicht beim ersten Gebrauch: sonst
    // faellt eine .md3 mitten in ein Bild, und das haben wir in rc307 bis
    // rc311 ausfuehrlich gehabt.
    struct EmitterModel {
        std::string path;
        int shaderBase = 0;
    };
    std::map<std::string, EmitterModel> emitterModels;   // Pfad -> Angaben
    // Was gerade fliegt, je Bild neu bestimmt. Steht hier, weil die Liste
    // der lebenden Effekte in einem anderen Block gerechnet wird als der,
    // in dem gezeichnet wird.
    std::vector<EmitterModelDraw> emitterFlug;

    // Shadername -> Nummer in textures.byShader, fuer die Effekte.
    //
    // Die .efx nennen ihre Shader beim Namen; ohne diese Zuordnung
    // zeichneten wir Partikel als farbige Vierecke ohne Bild. Sie stehen
    // hinten in derselben Liste wie die Texturen der Karte und der
    // .md3-Modelle - eine Liste, ein Weg, eine Stelle zum Nachsehen.
    EffectShaderSlots effectShaders;

    struct ActorAssets {
        GlmModel model;
        ModelTextures textures;
        // Zeigt in skeletons. std::map haelt seine Knoten an Ort und
        // Stelle, also bleibt der Zeiger gueltig, auch wenn spaeter weitere
        // Skelette dazukommen - bei std::vector waere er es nicht.
        const Skeleton* skeleton = nullptr;
        bool tried = false;
    };
    // rc604 fix604: je Figur ein Verweis auf das EINE geladene Modell
    // (modelCache) statt einer Kopie samt Bildern - fuenf Obi-Wans hielten
    // fuenf Kopien, und jedes prepareScene legte sie neu an (neue Adressen,
    // also lud die Grafik sie auch neu hoch). Ohne Modell: ein leerer Eintrag.
    std::vector<std::shared_ptr<const ActorAssets>> actorAssets;
    NpcMap npcMap;
    // Die Klingen aus ext_data/sabers/*.sab. Schluessel ist der Eintragname
    // aus dem `saber`-Schluessel einer .npc-Datei.
    SaberMap saberMap;
    // Die Waffen aus ext_data/weapons.dat. Schluessel ist der weapontype
    // aus dem `weapon`-Schluessel einer .npc-Datei, z.B. "WP_BLASTER".
    WeaponMap weaponMap;
    // Die Waffenmodelle, gepuffert. Schluessel ist der volle .md3-Pfad aus
    // weapons.dat. Gebraucht wegen ihres Tags "tag_flash" - dort sitzt die
    // Muendung.
    std::map<std::string, Md3Model> weaponModels;
    // Die Missionen des Spiels zum zuletzt gewaehlten Archiv (rc603: das
    // ganze Spiel, nicht nur das Archiv).
    std::vector<Mission> missions;
    bool missionPickOpen = false;
    std::string missionArchive;
    // --- rc603: Load mission ueber das ganze Spiel ---------------------------
    //
    // Spielordner NUR fuer diese Sitzung (spielordnerFuerArchiv, bhed/mission.h):
    // leer = die eingerichteten (settings.gamePaths). Wird nie gespeichert.
    std::vector<std::string> sitzungsSpielordner;
    // rc603 (Durchsicht): die Ordner, die rescanGamePaths zuletzt durchsucht
    // hat - auch die ohne .pk3, die in gamePaths fehlen.
    std::vector<std::string> eingeleseneSpielordner;
    bool spielordnerEingelesen = false;
    // Die Karten, die im Hintergrund gelesen werden (missionsSuche), in
    // derselben Reihenfolge wie deren Auftraege, und die Skriptdateien des
    // Spiels zum Aufloesen danach.
    std::unique_ptr<MissionsSuche> missionsSuche;
    std::chrono::steady_clock::time_point missionsSucheAb;   // fuer das Protokoll
    std::vector<MissionsKarte> missionsSuchKarten;
    std::vector<std::string> missionsSuchArchive;   // Archivpfad je Karte (das gewinnende)
    std::vector<std::string> missionsSuchSchluessel;   // Schluessel im Zwischenspeicher je Karte
    std::unordered_set<std::string> missionsSkriptNamen;
    // Schon gelesene Skriptverweise je Karte (Archiv, Eintrag, Versatz,
    // Groesse von .bsp und .ent) - ein zweites "Load mission" desselben
    // Spiels liest keine Karte noch einmal.
    std::map<std::string, std::vector<MapData::ScriptRef>> missionsVerweise;
    char missionsFilter[64] = {};
    bool npcMapRead = false;

    // Die Shaderskripte EINMAL fuer die ganze Sitzung.
    //
    // Vorher lag die Tabelle als oertliche Veraenderliche in
    // texturesFor(), also wurde sie JE MODELL neu aufgebaut: alle
    // .shader-Dateien aus allen Archiven lesen und zerlegen. Bei shanks
    // Movie-Duels-Installation sind das 40 Archive und 9014 Eintraege - und
    // die Mission hat 64 Figuren. Das Protokoll zeigte den Satz "9014
    // Shadereintraege gelesen" dutzendfach; das Programm schien zu haengen,
    // rechnete aber dieselbe Tabelle immer wieder.
    ShaderMap shaderMap;
    // Beim Beenden: der Reiter, in dem man vor der ersten Speichernfrage
    // stand - dorthin bei "Abbrechen" (confirmQuit).
    int quitZurueck = -1;
    // Das erste Skript der zuletzt geladenen Mission (sc2.file) - nach dem
    // Laden soll sein Reiter vorn stehen (loadMission).
    std::string missionErstesSkript;
    // fogparms aller Nebelshader - mit shaderMap zusammen gelesen.
    NebelMap nebelMap;
    // --- Bilder nur EINMAL lesen und entpacken --------------------------
    //
    // Gemessen in rc186: von 6,2 s beim Laden gingen 5,0 s in das Aufloesen
    // der Figurentexturen - 81 Prozent. Der Grund: texturesFor() lief JE
    // MODELL und JE FLAECHE los, las die Datei aus dem Archiv (also mit
    // Entpacken) und entzifferte das Bild neu. Dieselbe Ruestung an zehn
    // Sturmtrupplern hiess zehnmal dieselbe Arbeit.
    //
    // Die Engine macht es genauso wie hier: sie fuehrt eine Tabelle nach
    // NAMEN (AllocatedImages in tr_image.cpp) und gibt ein schon geladenes
    // Bild einfach zurueck. Der Schluessel wird dort ueber
    // GenerateImageMappingName gebildet - klein geschrieben, ohne Endung,
    // Schraegstriche vereinheitlicht. Genau das tut texKey() unten.
    std::unordered_map<std::string, TextureSet::Tex> texCache;
    // rc603: die Bildpunkte der Figurstufen (ModelTextures::FigurStufe::pixel),
    // je Bild einmal - Schluessel wie texCache. Schwach gehalten: das Bild
    // lebt, solange eine Figur es traegt.
    std::unordered_map<std::string, std::weak_ptr<const TextureSet::Tex>> figurStufenBilder;
    // Welche Eintraege von texCache die AKTUELLE Karte benutzt hat (ueber
    // loadTextureFor: Kartenflaechen, Himmel, Stufen, Effekte, Kartenmodelle).
    // Beim naechsten Kartenwechsel fliegen die alten heraus, die die neue
    // Karte nicht wieder braucht - wie RE_RegisterImages_LevelLoadEnd, das
    // Bilder ohne Verwendung im neuen Level loescht. Vorher wuchs texCache
    // mit jeder Karte (Kartenpruefung 27.09.: 8 Karten, 91 -> 857 MB).
    // Figuren (@256) und Klingenbilder stehen nie hier drin.
    std::unordered_set<std::string> kartenBilder;
    // Welcher Knoten wurde zuletzt durch EINFUEGEN angewaehlt (nicht durch
    // einen Klick)? Ein angewaehlter Block nimmt neue Befehle nur auf, wenn
    // DU ihn angeklickt hast - sonst liefe der naechste Klick in der
    // Ereignisliste in den gerade angelegten Block (die "Klebrigkeit" aus
    // rc532). Ein Klick in den Baum leert es.
    Path auswahlDurchEinfuegen;
    // "Last compile: ..." in der Statuszeile (leer ohne .ibi) - hier, damit
    // der Selbsttest es lesen kann.
    std::string letzteKompilierung;
    bool shaderMapRead = false;

    // Schon geladene Modelle, damit gleiche Figuren nicht mehrfach kosten.
    //
    // In einer Massenszene sind die meisten Figuren derselbe Sturmtruppler.
    // Das Protokoll zeigte rund zwanzigmal hintereinander "42 gefunden, 0
    // nicht gefunden" - jedes Mal dasselbe Modell samt Texturen neu
    // eingelesen und verkleinert.
    //
    // Schluessel ist der Modellordner: models/players/<name>.
    std::map<std::string, std::shared_ptr<const ActorAssets>> modelCache;
    // rc604 fix604: fuer welche Karte modelCache zuletzt aufgeraeumt wurde -
    // beim Kartenwechsel fallen die Modelle, die keine Figur mehr nennt.
    std::string modelCacheKarte;
    // Die Modelle der Schwertgriffe. Schluessel ist der VOLLE Pfad aus der
    // .sab-Datei - anders als bei Figuren, die immer unter
    // models/players/<name>/model.glm liegen.
    std::map<std::string, ActorAssets> hiltCache;
    // rc602: Ghoul2-Modelle der Gegenstaende (ohne .skin: G2API_InitGhoul2Model
    // ohne customSkin nimmt die Shadernamen aus dem .glm).
    std::map<std::string, ActorAssets> gegenstandGlmCache;
    Timeline timeline;
    // Klaenge beim Abspielen mitspielen. Bis wohin sie schon ausgeloest
    // wurden, damit keiner zweimal kommt.
    bool playAudio = true;
    bool stumm = false;
    // Die Ueberlagerung des GPU-Wegs: Entitykreuze, Kamera, Bahnen, Gizmos
    // und die Blende als durchsichtiges Bild ueber dem GPU-Bild.
    MapImage gizmoBild;
    // gizmoBild.depth haelt seit rc594 die ROHEN Werte des Tiefenpuffers;
    // umgerechnet (gpu::abstandAusTiefe) wird nur, wo eine Linie hinfaellt.
    // Vorher alle 3,3 Millionen je Bild - beim Fliegen ein Zehntel der Zeit.
    float gizmoTiefeNah = 4.0F;
    float gizmoTiefeFern = 1.0F;
    // Das Rechteck [x0, y0, x1, y1), in das die Gizmos in diesem und im
    // vorigen Bild gezeichnet haben (rc594): nur dort wird das Bild geleert
    // und hochgeladen, statt jedes Bild 13 MB.
    int grafikNeuAufbauten = 0;   // wie oft das Geraet neu aufgebaut wurde (rc596)
    std::size_t vorgewaermt = 0;   // Stand von Karte, Bildern und Effekten beim letzten Vorwaermen
    bool ruheImTest = false;      // Selbsttest: den Ruhezustand ausnahmsweise erlauben
    // --- Was das Skript bewegt (rc597) ----------------------------------------
    //
    // "sollte sich die ent nicht mit dem schiff mitbewegen?" - je Entity der
    // Versatz vom Schluessel "origin" zum Ort, an dem Szene oder Mover-
    // Simulation sie zur eingestellten Zeit zeigen (Schiff, Tuer, Figur).
    // Marke, Kasten und Gizmo stehen dort. Neu bei jedem Neuzeichnen.
    std::unordered_map<int, std::array<float, 3>> entVersatz;
    // Ebenso die Blickrichtung: Gier des Modells jetzt minus Schluessel
    // "angle" - der Pfeil zeigt, wohin es gerade schaut ("was bringt das
    // facing? es schaut nicht da hin, wo das Model hinsieht").
    std::unordered_map<int, float> entGierVersatz;
    // Was ein Klick ins Bild treffen kann, ausser Marken: die Netze der
    // beweglichen Modelle mit ihrer Stellung (rc597, "ich kann das schiff
    // nicht anklicken").
    struct PickMover {
        const BspMesh* mesh = nullptr;
        int entity = -1;
        float welt[16]{};
    };
    std::vector<PickMover> pickMover;
    int umrissBilder = 0;         // wie oft ein Umriss der Auswahl gezeichnet wurde (Selbsttest)
    int vorschauEffektNetze = 0;  // Effektnetze fuer die Kameravorschau gebaut (Selbsttest)
    int vorschauEffektEcken = 0;  // Ecken im letzten davon
    int gizmoBox[4] = {0, 0, 0, 0};
    int gizmoBoxAlt[4] = {0, 0, 0, 0};
    void* gizmoTexture = nullptr;
    int gizmoTexW = 0;
    int gizmoTexH = 0;
    // Balken und Vollbild-Blende beim Blick durch die Kamera: als Rechtecke
    // ueber dem GPU-Bild (rc594) statt als Bild in Fenstergroesse.
    bool blendeZeigen = false;
    float blendeBalken = 0.0F;
    float blendeFarbe[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    bool gizmoZeigen = false;   // im letzten Bild ueber das GPU-Bild gelegt?   // Selbsttest: Klaenge verarbeiten, aber nicht ausgeben
    double audioUpTo = -1.0;
    float lastCam[7]{};
    // Vorgaben wie das Spiel: r_mapOverBrightBits 0 (Lightmaps, Eckenfarben und Gitter unveraendert)
    // und kein Grundlicht. Beide Regler sind Hilfen fuer dunkle Karten.
    float mapBrightness = bhed::gpu::kSpielLightmapFaktor;
    float mapMinLight = 0.0F;
    // Zeit fuers letzte Bild, geglaettet - eine ungeglaettete Anzeige
    // springt so, dass man sie nicht lesen kann.
    float mapFrameMs = 0.0F;
    std::string mapBytes;      // fuer den erneuten Netzbau bei anderer Feinheit
    // .pk3-Blaetterei
    std::vector<GamePath> gamePaths;
    std::vector<FoundFile> pk3Files;
    // Kopf und Oberkoerper dem Blickziel nachdrehen.
    //
    // Gemeldet: "immer noch zu weit gedreht." Mit diesem Schalter laesst
    // sich das in einem Klick entscheiden: AUS zeigt allein die Animation,
    // also die Pose, wie sie in der .gla steht. Sieht die Figur dann
    // richtig aus, liegt es an der zusaetzlichen Drehung; sieht sie
    // genauso aus, liegt es an der Pose oder am Standwinkel.
    //
    // Eine Vorschau darf so einen Schalter haben - sie soll ja Fragen
    // beantworten, nicht nur huebsch sein.
    bool lookRotation = true;
    Audio audio;
    // Ein ZWEITES Geraet, nur fuer die Musik.
    //
    // Gemeldet: "jetzt hoere ich die Musik, aber die Stimmen nicht mehr."
    //
    // Der Grund steht in der Windows-Schnittstelle: waveOutWrite auf EIN
    // Geraet reiht die Puffer hintereinander - es mischt sie nicht. Die
    // Musik von md_ga_jedi laeuft ueber eine Minute; alles, was danach
    // geschrieben wird, wartet, bis sie durch ist.
    //
    // Vor rc255 gab es keine Musik, also fiel es nicht auf: die Stimmen
    // ueberlappen sich in diesen Skripten nicht, und kurze Klaenge
    // hintereinander klingen wie gleichzeitige.
    //
    // Ein zweites Geraet ist auch das, was die Engine tut - Musik ist dort
    // kein Klangkanal, sondern ein eigener Strom
    // (S_StartBackgroundTrack, cg_main.cpp:2965). Windows mischt zwei
    // offene WAVE_MAPPER-Geraete selbst.
    Audio musicAudio;
    // Beide Geraete, beim Start im Hintergrund geoeffnet (rc594): waveOutOpen
    // braucht beim ersten Mal rund 200 ms, und das stand als Ruckler mitten
    // in der ersten Kamerafahrt mit Klang. Uebernommen wird im Hauptfaden
    // (tonUebernehmen) - vorher fasst niemand audio/musicAudio an.
    struct TonVorab {
        Audio ton;
        Audio musik;
    };
    std::future<TonVorab> tonVorab;
    // Laenge je Klangdatei in Millisekunden, -1 = nicht gefunden.
    std::unordered_map<std::string, double> soundLen;
    // Nach dem Schreiben: wie lange steht die Bestaetigung noch, und wie
    // hell? Sekunden, laeuft herunter.
    float gizmoDoneFade = 0.0F;
    // Kamera wie die Entities (gui/app_kamera.cpp, rc588): Maus ueber der
    // Kamera, und "Steuern" (Pilot) - man sitzt in einer Einstellung und
    // fliegt sie frei, bis man uebernimmt oder abbricht.
    bool kameraHover = false;
    // --- Wegnetz der NPCs (gui/app_wegpunkte.cpp, rc589) ------------------
    bool zeigeWegnetz = false;        // Netz in der Ansicht
    bool wegFensterOffen = false;     // Fenster "Waypoints"
    int wegWerkzeug = 0;              // 0 keins, 1 setzen (Kette), 2 verbinden; rc609 W3b: 3 Pinsel, 4 Lasso, 5 Pfad, 6 Startpunkt
    int wegVon = -1;                  // verbinden: Entity des ersten Klicks
    int wegLetzter = -1;              // setzen: zuletzt gesetzter (Kette)
    float wegAbstand = 192.0F;        // Rasterweite beim Erzeugen
    float wegAbstandGenutzt = 192.0F; // rc609 W3a: die Rasterweite des letzten Erzeugens (weiter bei mehr Punkten, als das Spiel nimmt)
    float wegAbstandVoll = 0.0F;      // rc609 W4: die Rasterweite der letzten vollen Erzeugung auf dieser Karte (0: keine)
    std::vector<WegErzeugungInfo::Luecke> wegLuecken;   // rc609 W3a: Teile, die das letzte Erzeugen nicht anbinden konnte
    bool wegKlickVerbraucht = false;  // ein Werkzeugklick waehlt nichts aus
    WegNetz wegNetz;
    std::size_t wegNetzSchl = 0;      // Pruefsumme der Wegentities beim Bau
    bool wegNetzGebaut = false;       // wegNetz passt zu wegNetzSchl (auch ein leeres)
    std::vector<WegBefund> wegBefunde;
    bool wegGeprueft = false;
    std::string wegMeldung;           // Ergebnis des letzten Erzeugens
    // rc609 W1 (Mike): neue und verschobene Wegpunkte mit den Nachbarn im
    // Umkreis (= wegAbstand) verbinden, die die Kantenpruefung des Spiels
    // bestehen; beim Loeschen die Nachbarn verbinden (Luecke schliessen).
    bool wegAutoVerbinden = true;
    bool wegLueckeSchliessen = false;
    std::string wegMuster = "wp_#";  // Umbenennen nach Muster (# = Nummer)
    // rc609 W3b (Mike: "highlight/draw an area and then click generate. Like paint a path around the map."):
    std::shared_ptr<WegAuftrag> wegAuftrag;           // die Erzeugung, die gerade im Hintergrund rechnet
    std::vector<WegBereich> wegBereiche;              // gemalte Bereiche der geladenen Karte (gespeichert je Karte)
    std::string wegBereicheKarte;                     // fuer welche Karte wegBereiche gilt
    bool wegNeuSperre = false;                        // der naechste gemalte Bereich ist ein Sperrbereich
    float wegPinsel = 96.0F;                          // Halbmesser des Pinsels
    std::vector<std::array<float, 3>> wegStrich;      // der Strich, der gerade gemalt wird (Boden)
    bool wegStrichAktiv = false;
    std::vector<std::array<float, 3>> wegMarken;      // Stellen des letzten Pfads ohne Kante oder Punkt
    std::vector<char> wegAbseits;                     // je Entity: Wegpunkt ausserhalb des Hauptteils des Netzes
    int wegTeileZahl = 0;                             // Teile des Netzes ausser dem Hauptteil
    // rc609 W4 (Mike: point_combat "automatically connect to waypoints (no target needed)"; Plan G Testroute, Abdeckung):
    // Werkzeug 7 Kampfpunkt setzen, 8 Testroute.
    std::vector<NavKampfPunkt> wegKampf;              // die Kampfpunkte wie im Spiel (navKampfPunkte)
    std::shared_ptr<const NavWelt> wegKampfWelt;      // das Netz, aus dem wegKampf gerechnet ist
    bool wegVersetzen = true;                         // ein Wegpunkt, den das Spiel verwirft, rueckt an die naechste gueltige Stelle
    bool wegRouteSpringen = true;                     // Testroute fuer eine Figur mit SCF_NAV_CAN_JUMP
    std::vector<std::array<float, 3>> wegRouteKlick;  // die angeklickten Punkte (0..2)
    std::vector<std::array<float, 3>> wegRoute;       // der Weg: Klick, Knoten ..., Klick (leer: keiner)
    std::vector<char> wegRouteSprung;                 // je Stueck des Wegs: eine Sprungkante
    std::vector<int> wegRouteKnoten;                  // die NAV-Knoten des Wegs (navAStar)
    bool wegAbdeckungAn = false;                      // begehbare Flaechen ohne Wegpunkt in Reichweite einfaerben
    std::shared_ptr<WegAbdeckAuftrag> wegAbdeckAuftrag;   // rechnet im Hintergrund
    WegAbdeckung wegAbdeckung;                        // das Ergebnis
    std::size_t wegAbdeckungSchl = 0;                 // fuer diese Entities und Rasterweite (0: keins)
    // rc609 H2 (Mike: point_combat "usually behind cover"): Vorschlaege hinter Deckung (NPC_FindCombatPoint CP_CLEAR|CP_COVER).
    std::shared_ptr<WegDeckAuftrag> wegDeckAuftrag;   // rechnet im Hintergrund
    std::vector<WegDeckung> wegDeck;                  // die Vorschlaege (Geisterpunkte)
    int wegDeckStand = -1;                            // entStand, fuer den sie gelten (-1: keine)
    int wegDeckVerworfen = 0;                         // vom Spiel keinem Wegpunkt zugeordnet oder nicht erreichbar: verworfen
    float wegDeckAbstand = 0.0F;                      // Abstand der Vorschlaege (0: Vorgabe des Regelwerks)
    int wegDeckHoechstens = 0;                        // hoechstens so viele (0: Vorgabe aus der Zahl der Wegpunkte)
    bool wegDeckNurBereich = false;                   // nur in den gemalten Bereichen (W3b)
    std::string wegDeckMeldung;
    int entStand = 0;                 // zaehlt jede Entity-Aenderung (nachAenderung)
    // --- Draufsicht (rc589) ----------------------------------------------
    // Senkrecht nach unten; was hoeher liegt als draufSchnitt, schneidet die
    // vordere Ebene weg (Decken verdecken sonst den Boden).
    bool draufsicht = false;
    float draufSchnitt = 0.0F;
    Camera vorDraufsicht;
    // Ging die letzte Aenderung an der gewaehlten Kamera ins SKRIPT? Dann
    // gehoert Strg+Z ueber der Karte dem Skript, nicht den Entities
    // (entityFokus) - sonst naehme es nach einem Kamerazug eine aeltere
    // Entity-Aenderung zurueck.
    bool kameraSchriebSkript = false;
    bool kameraPilot = false;
    int pilotSchluessel = -1;
    Camera pilotVorher;
    // --- Das Spielfenster (Paket kamera, rc601) -------------------------
    //
    // Wo das Spielbild in der Ansicht liegt (mittig, Anteile), im letzten
    // gezeichneten Bild. Ausserhalb deckt die Ueberlagerung ab: beim Blick
    // durch die Kamera und beim Abspielen schwarz (spielRahmenDeckung 255),
    // in der freien Ansicht und im Pilot halbdurchsichtig, ohne Rahmen 0.
    SpielRahmen spielRahmen;
    int spielRahmenDeckung = 0;
    // rc601 integ: zuletzt angewandtes Regelwerk (-1 = noch nie, 0 OpenJK,
    // 1 Movie Duels) und die Schwierigkeits-Einstellung dazu (regelwerkPruefen).
    int regelwerkMdStand = -1;
    int regelwerkSchwierigkeitStand = -2;
    // rc604: das zuletzt angewandte Spiel (kRegelwerkOpenJK/MovieDuels/
    // JediOutcast, -1 = noch nie) und der Ordner des geladenen Befehlssatzes
    // (leer = der von loadModel, data/base oder die eigene .bhc).
    int regelwerkSpielStand = -1;
    std::string befehlssatzOrdner;
    // Wo Balken und Blende zuletzt lagen (Bildschirm, x0 y0 x1 y1) und wie
    // hoch ein Balken war - fuer den Selbsttest.
    float blendeFlaeche[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    float blendeBalkenPx = 0.0F;
    // Die halbe Breite und Hoehe der Sichtpyramide der zuletzt gezeichneten
    // laufenden Kamera im Abstand 1 (tan der halben Bildwinkel).
    float kamPyramideTan[2] = {0.0F, 0.0F};
    // Der senkrechte Bildwinkel der Kameravorschau unten rechts.
    float vorschauFovY = 0.0F;
    bool pk3Open = false;
    // 0 = Skripte, 1 = Karten, 2 = Modelle
    int pk3Kind = 0;
    char pk3Filter[64] = {};
    // Nach welchem Archiv gefiltert wird. Leer heisst: alle.
    //
    // Bei einer vollstaendigen Installation sind das vierzig Archive mit
    // Hunderten Dateien - ohne diesen Filter sucht man in einer Liste, in
    // der assets1.pk3 und der eigene Mod durcheinanderstehen.
    std::string pk3Archive;
    std::string settingsFile;
    std::string dataDir;

    const theme::Theme* activeTheme = nullptr;
    // Die Farbgebung darf erst angewendet werden, wenn ImGui einen Kontext
    // hat. createApp() und loadSettings() laufen aber VOR CreateContext() -
    // ein ImGui::GetStyle() dort loest die Zusicherung
    // "No current context" aus und das Programm bricht beim Start ab.
    // Deshalb nur vormerken und im ersten Bild anwenden.
    bool themePending = true;
    float uiScale = 1.0F;    // vom Nutzer gewaehlt (Strg+Mausrad, Menue)
    float dpiScale = 1.0F;   // vom Bildschirm, wird von aussen gesetzt
    float appliedScale = -1.0F;  // was zuletzt tatsaechlich angewendet wurde
    TreeOptions treeOpt;
    Expanded expanded;
    std::vector<Row> rows;
    int selected = -1;
    int selectedCommand = -1;   // Zeile in der Ereignisliste links
    Path selectedPath;
    // Mehrfachauswahl. selectedPath bleibt der ZULETZT angeklickte Knoten -
    // an ihm haengen Einfuegen und der Bezug fuer Umschalt-Klick.
    std::vector<Path> selection;
    // --- Mehrere Skripte gleichzeitig -------------------------------------
    //
    // Der Aufbau ist bewusst schlicht: es gibt weiterhin GENAU EIN lebendes
    // Dokument (doc, path, selection, selectedPath, selected, expanded).
    // Alles im Programm arbeitet damit weiter wie bisher - hunderte
    // Zugriffsstellen bleiben unangetastet.
    //
    // Die anderen offenen Skripte liegen daneben GEPARKT. Beim Umschalten
    // wird das lebende hineingelegt und das gewaehlte herausgeholt.
    //
    // Warum so und nicht "ueberall das aktive Dokument nachschlagen": das
    // waere der saubere Aufbau, aber er beruehrt jede Stelle in app.cpp, die
    // g_app->doc anfasst. Ein Tausch an EINER Stelle ist weniger elegant und
    // sehr viel weniger riskant - und von aussen nicht zu unterscheiden.
    // Wie heisst das aktive Skript auf seinem Reiter?
    //
    // Aus einem Archiv geladene Skripte haben KEINEN Pfad - dorthin laesst
    // sich nicht zurueckspeichern. Ohne einen eigenen Namen hiessen dann
    // alle Reiter "unnamed.icarus", und gerade beim Laden einer Mission mit
    // zehn Skripten waere die Leiste wertlos.
    std::string shownName;

    static constexpr int kMaxSplit = 4;
    static_assert(sizeof(graphe) / sizeof(graphe[0]) == kMaxSplit, "ein Graph-Zustand je Feld");

    struct Parked {
        // Document hat keinen Standardkonstruktor - ein leeres Skript ist
        // ein gueltiger Anfangszustand und genau das, was ein neuer Reiter
        // zeigt.
        Document doc{Script{}};
        // Feste Kennung des Reiters, vergeben beim ersten Gebrauch
        // (tabKennung). Die NUMMER eines Reiters aendert sich, sobald ein
        // anderer davor geschlossen wird - wer daran etwas merkt (die
        // Rollposition), gibt es sonst dem Nachruecker weiter.
        int uid = 0;
        std::string path;
        std::vector<Path> selection;
        Path selectedPath;
        int selected = -1;
        Expanded expanded;
        std::string shownName;
        std::string skriptPfad;
        DateiHerkunft skriptHerkunft;   // rc609 W2a

        // Die Feldaufteilung DIESES Reiters: wie viele Felder, welcher
        // Reiter in welchem steht, wo der Fokus lag. Jeder Reiter hat sein
        // eigenes Fenster - "wenn ich im ersten tab 4 reiter offen habe
        // und dann in den nebenan gehe, dann sollte da wieder ein
        // einzelner sein und nicht global ueber alle verteilt". Ein
        // frischer Reiter beginnt mit EINEM Feld.
        int splitCount = 1;
        int splitTabs[kMaxSplit] = {0, 0, 0, 0};
        int splitFocus = 0;

        // Welche Karte gehoert zu diesem Reiter?
        //
        // NICHT die Karte selbst. Eine geladene Karte belegt mit Geometrie,
        // Lightmaps und Texturen leicht hundert Megabyte; zehn Reiter mit
        // je einer Kopie waeren ein Gigabyte, und beim Laden einer Mission
        // zeigen ohnehin alle zehn Skripte auf DIESELBE Karte.
        //
        // Deshalb nur der Pfad. Beim Umschalten wird verglichen: ist es
        // dieselbe Karte, bleibt sie stehen und es kostet nichts. Ist es
        // eine andere, wird sie geladen; ist keine gesetzt, wird die alte
        // weggeraeumt.
        //
        // Das ist der Grund, warum ein neuer Reiter jetzt leer ist statt
        // die Karte des vorigen zu erben.
        std::string mapPath;
        // Wie die Karte HEISST, nicht nur wo sie liegt.
        //
        // Missionen kommen aus einem Archiv und haben deshalb GAR KEINEN
        // Pfad auf der Platte - mapPath ist dann leer. Der Reiterwechsel
        // verglich aber genau diesen Pfad, also war "leer gegen leer" immer
        // gleich, und die Karte wurde nie gewechselt. Genau so wurde es
        // gemeldet: "die map ist nicht pro tab".
        //
        // MapData::path traegt die Kennung (etwa "maps/md_am_sith.bsp"),
        // auch bei einer Karte aus dem Archiv.
        std::string mapId;
        // Hat dieser Reiter eine EIGENE Karte?
        //
        // Wahr nur, wenn fuer ihn ausdruecklich eine geladen wurde - eine
        // Mission oder ein "Karte oeffnen". Ein Skript, das man einfach so
        // aufmacht, bekommt sie nicht: es teilt sich die des
        // Arbeitsbereichs.
        //
        // Ohne diese Unterscheidung schreibt sich beim Parken JEDER Reiter
        // die gerade geladene Karte zu, und aus dem Mitbenutzen wird wieder
        // ein Besitz.
        bool ownMap = false;
        // rc609 W2a: woher seine Karte kam - eine .pk3 ausserhalb der Spielordner
        // findet loadMapFromArchive(mapId) nicht wieder.
        DateiHerkunft kartenHerkunft;
        // rc604: die Spielordner, zu denen seine Karte gehoert (die der
        // Sitzung beim Laden, leer = die eingerichteten) - gemerkt wie die
        // Karte (mit eigener oder geerbter Karte). Beim Hinwechseln gelten
        // wieder sie: ein JK2-Missionsreiter neben einem von Jedi Academy
        // liest sonst die Dateien und Regeln des anderen Spiels.
        std::vector<std::string> spielordner;
        bool spielordnerGemerkt = false;
        // Wo die ANSICHT stand - Ort, Blickrichtung, Zeitpunkt.
        //
        // Gemeldet: "er speichert nur nicht die kamera position wo ich
        // war". Ohne das faengt man nach jedem Reiterwechsel wieder am
        // Ausgangspunkt an und sucht die Stelle, an der man gearbeitet hat.
        //
        // Gehoert zum Reiter und nicht zum Programm: zwei Skripte spielen
        // an verschiedenen Ecken derselben Karte, und jedes will seine
        // eigene Sicht.
        float camPos[3]{};
        float camAng[3]{};
        double playMs = 0.0;
        bool camSaved = false;
    };
    // Die offenen Skripte. Der aktive Eintrag ist ein PLATZHALTER - seine
    // Felder stehen in doc/path/... und werden beim Umschalten
    // zurueckgelegt.
    std::vector<Parked> tabs;
    int activeTab = 0;

    // --- Geteilte Ansicht ------------------------------------------------
    //
    // Rechts neben dem Skriptbaum ein ZWEITER, der einen anderen Reiter
    // zeigt. Gedacht zum Vergleichen und zum Uebertragen: "this is how I
    // usually work with multiple scripts open where I copy and paste
    // commands between them."
    //
    // Die rechte Haelfte ist absichtlich nur zum LESEN und Kopieren. Sie
    // hat kein eigenes Dokument im Sinne von Bearbeiten - das lebende
    // Dokument bleibt eines, und damit bleiben Rueckgaengig, Einfuegen und
    // alles Uebrige eindeutig. Wer rechts etwas aendern will, wechselt den
    // Reiter; dann ist es links.
    // Wie viele Spalten insgesamt? 1 = nur der Hauptbaum, bis 4.
    //
    // Wie in VS Code, wo man den Bereich in bis zu vier Gruppen teilt. Die
    // ERSTE ist immer der bearbeitbare Hauptbaum; die weiteren sind
    // Vergleichsspalten und nur zum Lesen - dieselbe Begruendung wie in
    // rc150: an rows/selection/selected haengt jede Bearbeitung, und die
    // wollen wir nicht vervierfachen.
    int splitCount = 1;

    // Je Vergleichsspalte ein eigener Satz. Drei davon, weil die erste
    // Spalte der Hauptbaum ist.
    struct SplitPane {
        int tab = 0;                 // welcher Reiter darin steht
        int ansicht = 0;             // Classic / Graph / Spuren (Feld 0: settings.flussAnsicht)
        std::vector<Row> rows;       // sein Baum
        int selected = -1;           // dort angeklickte Zeile
        Path path;                   // ihr Weg
        bool dirty = true;           // neu bauen?
        Expanded expanded;           // eigener Klappzustand
        std::vector<std::uint8_t> marke;   // Aenderungsrand je Zeile
    };
    // Ein Satz je Feld - jetzt auch fuer das erste.
    //
    // Bis rc161 war das erste Feld eine Sonderrolle: es zeigte immer das
    // lebende Dokument, die anderen ihre geparkten. Damit war nur das erste
    // bearbeitbar.
    //
    // Der Umbau kommt OHNE ein zweites lebendes Dokument aus, und das ist
    // der Kern der Sache: das Feld mit dem FOKUS wird das lebende. Klickt
    // man in ein anderes, wird das bisherige geparkt und dessen Reiter
    // hervorgeholt - genau der Tausch, den es seit rc148 schon gibt.
    //
    // Damit bleibt es bei EINEM Dokument, das bearbeitet wird. Rueckgaengig,
    // Einfuegen, Loeschen und alles Uebrige bleiben unangetastet; keine der
    // hunderten Zugriffsstellen muss fragen, welches Feld gemeint ist. Und
    // von aussen sieht es aus, als waeren alle vier bearbeitbar - denn das
    // sind sie, eines nach dem anderen, so wie in VS Code auch immer nur
    // eine Gruppe den Schreibzeiger hat.
    SplitPane splitPanes[kMaxSplit];
    // Wo jeder REITER zuletzt stand, senkrecht.
    //
    // Gemeldet: "jedes Mal, wenn ich wieder irgendwo in ein anderes Skript
    // klicke, springt das erste Fenster ungewollt umher."
    //
    // Der Grund ist strukturell: das Feld mit dem Fokus wird als HAUPTBAUM
    // gezeichnet, die anderen als Vergleichsbaeume - das sind verschiedene
    // Fenster mit eigener Rollposition. Beim Fokuswechsel wandert ein Skript
    // also von einem Fenster ins andere und landet dort, wo dieses zuletzt
    // stand.
    //
    // Gemerkt wird deshalb je REITER, nicht je Feld: die Position gehoert
    // zum Skript, nicht zu dem Kasten, in dem es gerade steht.
    std::map<int, float> tabScroll;   // Schluessel: Parked::uid, nicht die Nummer
    // --- Lesezeichen und Aenderungsrand (wie Notepad++) -------------------
    //
    // Beides je Reiter an seiner festen Kennung (Parked::uid) und je Knoten
    // an SEINER Kennung (Node::kennung) - Wege aendern sich bei jeder
    // Bearbeitung, Kennungen nicht, auch nicht ueber Rueckgaengig.
    struct AenderungsStand {
        // Knoten in Dateireihenfolge mit ihrem eigenen Text: beim Oeffnen
        // und beim letzten Speichern.
        std::vector<std::pair<Kennung, std::string>> geladen, gespeichert;
        // Die Befehle selbst, wie sie beim Oeffnen waren (ohne Blockinhalt) -
        // fuer "Revert to original" im Rechtsklickmenue.
        std::map<Kennung, Node> original;
    };
    std::map<int, AenderungsStand> aenderungen;
    std::map<int, std::set<Kennung>> lesezeichen;
    std::vector<std::uint8_t> zeilenMarke;   // parallel zu rows
    float rinneX0 = 0.0F;                    // linke Kante des Randes (Selbsttest)
    int naechsteTabUid = 1;
    // Welchen Reiter jeder Kasten zuletzt zeigte - daran erkennt man den
    // Wechsel, bei dem die Rollposition wiederhergestellt werden muss.
    std::map<std::string, int> lastTabInBox;
    // Welches Feld hat den Fokus? Dieses zeigt das lebende Dokument.
    int focusPane = 0;

    // --- Wem gehoert die Aufteilung? -------------------------------------
    //
    // Gemeldet: "wenn ich alle 4 nebeneinander oeffne und dann ins zweite
    // Fenster klicke, bin ich ploetzlich in einem anderen Tab... dabei
    // sollte er das nur im ersten anzeigen, weil ich die DA gesplittet
    // habe."
    //
    // Genau richtig gedacht. Die Aufteilung hing bisher am AKTIVEN Reiter -
    // und der wandert beim Klick in ein Feld mit, weil dort ein anderes
    // Skript steht. Also wanderte die Aufteilung mit und wurde beim
    // naechsten Reiterwechsel in einen Reiter geschrieben, der nie geteilt
    // wurde.
    //
    // Jetzt gibt es einen BESITZER: den Reiter, in dem geteilt wurde. Er
    // ist im Reiterband hervorgehoben und behaelt die Aufteilung. Welches
    // Skript gerade bearbeitet wird, sagt der leuchtende Rahmen um das
    // Feld - dafuer ist er da.
    //
    // Damit sind zwei Fragen getrennt, die vorher eine waren:
    //   "In welchem Arbeitsbereich bin ich?"  -> homeTab
    //   "Welches Skript aendere ich gerade?"  -> activeTab
    int homeTab = 0;

    // --- Ziehen zwischen den Feldern --------------------------------------
    //
    // Die Nutzlast, die ImGui traegt. Nur kleine Werte: ImGui kopiert sie
    // sofort in einen eigenen Puffer, und ein Zeiger darin waere spaetestens
    // beim Loslassen ungueltig (so steht es in der Doku zu
    // SetDragDropPayload).
    //
    // Deshalb NICHT der Knoten und nicht sein Weg, sondern nur: aus welchem
    // Feld, aus welchem Reiter, welche Zeile. Den Rest sucht das Ziel selbst
    // heraus - es hat beides ohnehin vorliegen.
    struct DragNode {
        int pane = -1;
        int tab = -1;
        int row = -1;
    };

    // --- Protokoll ueber Reiter, Felder und Groessen ----------------------
    //
    // Gewuenscht, um sicherzugehen, dass mehrere gleichzeitig geoeffnete
    // Skripte und das Verstellen der Fenstergroessen wirklich tun, was sie
    // sollen. In dieser Sitzung kamen die meisten Ueberraschungen aus genau
    // diesen beiden Ecken.
    //
    // Geschrieben wird nur bei einer AENDERUNG - sonst faellt es jedes Bild
    // an. Dafuer merkt sich diese Aufnahme den zuletzt geschriebenen Stand.
    struct LayoutLog {
        int tabs = -1;
        int activeTab = -1;
        int splitCount = -1;
        int focusPane = -1;
        int paneTabs[kMaxSplit] = {-1, -1, -1, -1};
        int frameW = -1;
        int frameH = -1;
        int eventsW = -1;
        int flowW = -1;
        int toolH = -1;
        // Das Fenster war zu klein fuer alles, was hineinsoll - siehe
        // Layout::tooSmall in app.cpp.
        bool tooSmall = false;
        int fracX = -1;   // in Promille, damit der Vergleich ganzzahlig ist
        int fracY = -1;
        int tlFrac = -1;
    };
    LayoutLog lastLayoutLog;

    // Wie tief war der Rueckgaengig-Stapel zuletzt, und in welchem Reiter?
    //
    // Aendert sich die Tiefe, wurde BEARBEITET - und dann soll im Protokoll
    // stehen, WELCHES Skript es getroffen hat. Genau das ist die Frage beim
    // Arbeiten an vier Skripten nebeneinander: landet die Aenderung dort,
    // wo ich hingeklickt habe?
    std::size_t lastUndoDepth = 0;
    int lastEditTab = -1;
    // Fokuswechsel, angefordert waehrend des Zeichnens.
    //
    // Nicht sofort ausgefuehrt: mitten im Zeichnen das Dokument zu
    // tauschen zoege dem gerade gezeichneten Baum den Boden weg.
    int focusRequest = -1;

    // --- Anordnung der Kartenansicht ------------------------------------
    //
    // Die Einstellungen der Ansicht standen als drei Zeilen Kaestchen UNTER
    // dem Bild und nahmen dort dauerhaft Platz weg - Platz, den die
    // Zeitleiste braucht. Jetzt stehen sie in einer Spalte am rechten Rand
    // der Ansicht, die man ein- und ausklappen kann. So macht es Blender
    // mit seiner Seitenleiste (N).
    bool mapSidebar = true;
    float mapSidebarW = 0.0F;   // gemessen, nicht geraten
    // Hoehe der Zeitleiste, als Anteil der Kartenspalte. Ziehbar am oberen
    // Rand, wie in Blender.
    float timelineFrac = 0.22F;
    // Zeigt die Zeitleiste Bilder oder Sekunden? In 3ds Max laesst sich das
    // umschalten, und beides ist gebraucht: Sekunden fuer die Laenge einer
    // Einstellung, Bilder fuer das genaue Setzen eines Schluessels.
    bool timelineFrames = false;
    // Bilder je Sekunde fuer die Umrechnung. JKA rechnet intern in
    // Millisekunden; 30 ist das, womit die Filmleute der Mod arbeiten.
    int timelineFps = 30;
    // Die Spurenliste aufgeklappt? Blender nennt das den Kanalbereich.
    bool timelineChannels = true;
    // --- Ausschnitt der Zeitleiste ---------------------------------------
    //
    // Bei 62 Sekunden Skript ist ein wait von 200 ms drei Bildpunkte breit -
    // sichtbar, aber nicht zu treffen und schon gar nicht zu lesen. Mit
    // Zoom wird aus der Uebersicht ein Werkzeug.
    //
    // Gespeichert wird der ANFANG in Millisekunden und die Vergroesserung;
    // die sichtbare Spanne ist Dauer/Vergroesserung. Nicht Anfang und Ende,
    // weil sonst beim Zoomen zwei Werte gleichzeitig wandern und man sich
    // beim Klemmen leicht vertut.
    double timelineViewStartMs = 0.0;
    double timelineZoom = 1.0;
    // Wie weit hinein? 32-fach heisst bei einer Minute Skript knapp zwei
    // Sekunden im Bild - das ist die Aufloesung, in der man Schluessel
    // setzt. Vorher waren 400 erlaubt: 0,15 Sekunden im Bild, und die
    // Zahlen im Lineal standen uebereinander. "so klein brauche ich es
    // nicht" - stimmt, und es war auch nicht lesbar.
    //
    // EIN Wert, statt der Zahl an sechs Stellen: sechs Abschriften einer
    // Grenze laufen auseinander, und dann klemmt eine Stelle anders als
    // die andere.
    static constexpr double kMaxZoom = 32.0;

    // --- Eine Dauer in der Zeitleiste ziehen ------------------------------
    //
    // Gezogen wird die RECHTE KANTE eines Blocks; das aendert seine DAUER.
    // Den Block als Ganzes zu verschieben ergibt in ICARUS keinen Sinn: ein
    // Skript laeuft der Reihe nach, der Anfang eines Befehls ergibt sich
    // aus allem davor. Was man wirklich einstellt, ist "wie lange".
    //
    // Waehrend des Ziehens wird NICHTS geschrieben - nur die Vorschau
    // wandert. Erst beim Loslassen geht es ueber replaceAt() ins Skript und
    // damit in den Rueckgaengig-Speicher. Dieselbe Regel wie beim Gizmo.
    // Eigene Namen, NICHT dragFrom/dragging - die gehoeren dem Ziehen im
    // Skriptbaum. Zwei Vorgaenge, die sich ein Merkmal teilen, stoeren
    // einander frueher oder spaeter.
    // Wo die Zeitleiste gerade liegt - fuer den Selbsttest (zeitleiste),
    // der echte Mauszuege auf Bloecke, Kanten und Namen setzt.
    float tlFeldX = 0.0F;        // linke Kante der Zeitachse (Bildschirm)
    float tlFeldW = 0.0F;        // ihre Breite
    double tlSichtStart = 0.0;   // sichtbarer Ausschnitt in ms
    double tlSichtSpanne = 1.0;
    float tlLinealY = 0.0F;      // Mitte des Lineals
    std::vector<std::pair<std::string, float>> tlZeilenY;   // Name -> Mitte der Zeile
    // Was in DIESEM Bild unter der Maus angeklickt wurde. Entschieden wird
    // erst am Ende der Spuren: dann gewinnt der OBEN liegende (zuletzt
    // gezeichnete) Block. Vorher griff der erste - in der Script-Zeile der
    // Kamerablock UNTER einem wait, das man sah (Zeitleistentest 27.09.).
    // Einen Block der Zeitleiste greifen und verschieben (wie Ripple-Edit
    // im Sequencer): der Pfad des Befehls, wo die Maus anfing, sein Anfang,
    // der Massstab, und ob schon gezogen wird (erst ab 4 Bildpunkten).
    Path tlZiehPfad;
    float tlZiehMausX = 0.0F;
    double tlZiehStartMs = 0.0;
    double tlZiehMsJePx = 1.0;
    bool tlZiehAktiv = false;
    double tlZiehDelta = 0.0;
    struct TlKandidat {
        bool da = false;
        Path pfad;
        double startMs = 0.0;
        double endMs = 0.0;
        bool kamera = false;   // Kamerazeile (sonst eine Spur)
        int figur = -1;        // Figur der Spur, -1 = keine
    };
    // Die drei Hauptspalten (links | Script Flow | Actions) im letzten Bild:
    // tatsaechliche Breiten, Grenzen und die Lage der zwei Teiler - fuer den
    // Selbsttest (fenster), der die Teiler bis an die Enden zieht.
    float spaltenB[3]{};
    float spaltenMin[3]{};
    float spaltenMax[3]{};
    float teilerX[2]{};
    // Beim Anfassen eines Spaltenteilers: Mausposition und Breite der
    // gezogenen Spalte (in den Einheiten von spaltenZiel). Gezogen wird
    // ABSOLUT von dort aus - siehe die Griffe nach EndTable.
    float spaltenLuecke = 0.0F;   // frei rechts neben der letzten Hauptspalte
    // --- Was Undo/Redo zuletzt getan hat (Statuszeile) und die Undo-Liste
    //
    // shank: "When I undo, it isn't clear what was changed. Maybe it could
    // scroll to what was changed or have some sort of message popup at the
    // bottom like 'undo: wait 1000'?" - und eine Undo-Tabelle wie in 3ds Max.
    std::string undoMeldung;             // leer = keine
    bool undoMeldungRedo = false;        // stammt sie von einem Redo?
    std::size_t undoMeldungTiefe = 0;    // undoDepth danach - aendert sich das, ist sie veraltet
    int undoListeAnfrage = 0;            // 0 nichts, 1 Undo-Liste oeffnen, 2 Redo-Liste
    bool undoListeRedo = false;          // zeigt die offene Liste Redo-Schritte?
    std::vector<std::string> undoListeEintraege;   // neuester zuerst
    int undoListeMarke = 0;              // bis zu welchem Eintrag (einschliesslich)
    std::string undoStatusText;          // was unten gerade steht (fuer den Selbsttest)
    // Fuer den Selbsttest: zu welcher Zeile zuletzt der Randhinweis stand.
    int randVorschauZeile = -1;
    // Mitte des Randes je Zeile im letzten Bild (Bildschirm), und wie oft
    // die Original-Vorschau gezeichnet wurde - beides fuer den Selbsttest.
    std::vector<ImVec2> randMitte;
    int vorschauGezeichnet = 0;
    float teilerStartMaus = 0.0F;
    float teilerStartBreite = 0.0F;
    // Die Zeitleiste: ihre Hoehe im letzten Bild, der Platz, aus dem sie als
    // Anteil entsteht, und beim Anfassen der Kante Mausposition und Hoehe.
    float tlHoeheIst = 0.0F;
    float tlPlatz = 0.0F;
    float tlKanteStartMaus = 0.0F;
    float tlKanteStartHoehe = 0.0F;
    float tlKanteX = 0.0F;   // Mitte der Ziehkante, fuer den Selbsttest
    float tlKanteY = 0.0F;
    float teilerY = 0.0F;
    float spaltenReq[3]{};    // Diagnose: WidthRequest / WidthGiven / Ziel
    float spaltenGiven[3]{};
    float spaltenZiel[3]{};
    float tabelleOben = 0.0F;
    float tabelleUnten = 0.0F;
    float spaltenGesamt = 0.0F;
    // Was Zellraender und Trennlinien der Hauptspalten kosten (gemessen).
    float spaltenUeberhang = 40.0F;
    bool spaltenUeberhangGemessen = false;   // die erste Messung gilt immer
    int teilerAnzahl = 0;
    int tlUeberlappungen = 0;     // im letzten Bild rot markierte Bloecke (Selbsttest)
    TlKandidat tlGriffKandidat;   // Kante gepackt (Feldansicht "Spuren")
    Path tlDragPath;             // welcher Block, leer = keiner
    double tlDragStartMs = 0.0;  // sein Anfang - der bleibt stehen
    double tlDragEndMs = 0.0;    // sein Ende, waehrend des Ziehens
    bool tlDragging = false;
    // --- zeitleiste1: die Zeitleiste im Mission-Tab (gui/app_zeitleiste.cpp) ---
    spuren::Spuren spuren;                 // das Modell (prepareScene -> zeitleisteModellBauen)
    bool zlDetail = false;                 // Ansicht Detail statt Uebersicht (Tab)
    bool zlNurAktive = false;              // nur Darsteller, die im Ausschnitt etwas tun
    bool zlProbleme = false;               // nur Stellen mit Hinweisen
    char zlSuche[96] = {};                 // das Suchfeld
    bool zlAbspielenUmschalten = false;    // Leertaste ueber der Leiste (im naechsten Bild)
    struct ZlAuswahl {
        bool da = false;
        int zeile = 0;
        spuren::Clip clip;
        std::string anzeige;               // Darsteller, lesbar
    };
    ZlAuswahl zlAuswahl;                   // erklaert in der Seitenleiste ("Was passiert hier?")
    ZlAuswahl zlKandidat;                  // in diesem Bild angeklickt
    TlKandidat zlGriffKand;                // Kante gepackt (der zuletzt gezeichnete gewinnt)
    spuren::Clip zlGriffClip;              // ... und ihr Block
    TlKandidat zlKlickKand;                // Block angeklickt
    bool zlKante = false;                  // eine rechte Kante wird gezogen
    Path zlKantePfad;
    double zlKanteEndMs = 0.0;
    spuren::Clip zlZiehClip;               // was gezogen wird (Hinweistext, Zurueckschreiben)
    float zlZiehMausX = 0.0F;
    double zlZiehMsJePx = 1.0;
    // Durch Ziehen geaendert: angezeigter Knoten -> (geaenderter Knoten, sein Text davor).
    std::map<Kennung, std::pair<Kennung, std::string>> zlGeaendert;
    std::map<std::string, std::vector<float>> zlWellen;   // Wellenform je Klangdatei (Spitze je 50 ms)
    std::string zlTexteKarte;              // Untertitel: fuer welche Karte und welches Spiel geladen
    std::map<std::string, std::string> zlTexte;
    std::vector<std::string> zlTexteEbenen;
    std::string zlSzene;                   // Anfangsausschnitt (30 s) je Szene
    // Fuer den Selbsttest: was in diesem Bild gezeichnet wurde.
    ImRect zlNavRect;
    ImRect zlSucheRect;                    // uebersetzung2: das Suchfeld der Zeitleiste
    ImRect zlErklaerungRect;
    float zlErklaerungInhaltH = 0.0F;      // so hoch war der Inhalt des Kastens im letzten Bild
    std::string zlErklaerungTitel;
    int zlZeileGeoeffnet = 0;
    // Zeitleiste 2 Punkt 7: Untertitel im Vorschau-Bild.
    int untertitelStufe = -1;              // g_subtitles der Vorschau (0, 1, 2); -1 = Vorgabe des Spiels
    bool untertitelZeigen = false;         // dieses Bild zeigt die Szene zur Zeit des Zeigers
    std::string untertitelJetzt;           // Selbsttest: die gezeichneten Zeilen, "\n" dazwischen
    float untertitelOben = 0.0F;           // Selbsttest: Bildzeilen der Schrift (oben, unten)
    float untertitelUnten = 0.0F;
    float untertitelFarbe[4]{};            // Selbsttest: die Farbe der Zeilen
    // Zeitleiste 2 Punkt 1: Aufnehmen - ab welcher Abweichung von der Geraden
    // ein weiterer Laufpunkt noetig ist (Einheiten).
    float aufnahmeToleranz = 24.0F;
    struct ZlBlock {
        std::string zeile;
        ImRect r;
        spuren::Art art = spuren::Art::Befehl;
        Path pfad;
        std::string text;
        bool beschriftet = false;          // Beschriftung ganz zu sehen
        bool geaendert = false;            // durch Ziehen geaendert (gestrichelt)
    };
    std::vector<ZlBlock> zlBloecke;
    struct ZlText {
        ImRect r;
        float zeileY = 0.0F;
    };
    std::vector<ZlText> zlBeschriftungen;
    float zlZeichenMs = 0.0F;              // was das Zeichnen der Zeilen kostet (letztes Bild)
    // Der Filmstreifen: Bilder der Skriptkamera alle 2 s. Aufgenommen in einem
    // kurzen Durchgang (je Bild der Ansicht eines, die Ansicht und die Zeit des
    // Zeigers bleiben stehen - drawMapView, filmBild), gezeigt bis zum naechsten.
    struct ZlFilmBild {
        double ms = 0.0;
        void* textur = nullptr;
        int w = 0;
        int h = 0;
        unsigned stand = 0;                // render::texturStand() beim Anlegen
    };
    std::vector<ZlFilmBild> zlFilm;
    int zlFilmWunsch = -1;                 // in diesem Bild der Ansicht: Filmbild Nummer ... aufnehmen
    int zlFilmPass = -1;                   // laufender Durchgang: naechstes Bild (-1: keiner)
    double zlFilmZeitMs = 0.0;             // ... zu dieser Zeit (nur waehrend drawMapView zeichnet)
    std::uint64_t zlModellStand = 0;       // zaehlt die Modellbauten
    std::uint64_t zlFilmStand = 0;         // fuer diesen Modellbau gelten die Bilder
    int zlFilmAufnahmen = 0;               // Selbsttest: aufgenommene Bilder

    // Wo die Trennlinien des Rasters liegen, als Anteil - senkrecht und
    // waagerecht. Ziehbar, wie die Kanten zwischen Bereichen in Blender.
    // "I want to be able to change the size of each script flow view".
    // Welche Figur ist angewaehlt? -1 heisst keine.
    //
    // Dieselbe Rolle wie cameraSelected fuer die Kamera: erst die Auswahl
    // bringt die Bahn ins Bild. "ich will den charakter anklicken um zu
    // sehen wo er hinlaeuft, aehnlich wie bei der Kamera".
    int selectedActor = -1;
    // Wo die Figuren gerade auf dem BILDSCHIRM stehen - fuer den Klick.
    // Dieselbe Arbeitsteilung wie bei cameraSx/cameraSy: das Zeichnen
    // weiss es, der Klick liest es ab. Leer heisst: nicht im Bild.
    struct ActorMark {
        int index = -1;
        float sx = -1.0F;
        float sy = -1.0F;
    };
    std::vector<ActorMark> actorMarks;

    float splitFracX = 0.5F;
    float splitFracY = 0.5F;
    // Reiterwechsel aus dem Klappfeld des FOKUSSIERTEN Feldes - aus
    // demselben Grund angefordert statt sofort ausgefuehrt.
    int tabRequest = -1;

    // Der Doppelklick zeigt, was in einer Zeile steht.
    // Der volle Text einer Zeile, den ein Doppelklick rechts zeigt.
    std::string splitShowText;

    // Ziehen im Baum: welcher Knoten haengt an der Maus?
    Path dragFrom;
    // Aus WELCHEM Reiter und Feld der gezogene Knoten stammt.
    //
    // Der Weg allein genuegt nicht mehr, seit man zwischen Skripten ziehen
    // kann: derselbe Weg meint in einem anderen Dokument etwas anderes.
    // -1 im Feld heisst "aus dem Hauptbaum".
    int dragFromTab = -1;
    int dragFromPane = -1;
    bool dragging = false;
    // Nach einem Tastendruck muss die Zeile in den sichtbaren Bereich.
    bool scrollToSelected = false;      // Weg des ausgewaehlten Knotens, leer = keiner
    std::vector<Issue> issues;

    // Event-Editor
    bool editorOpen = false;
    // Die MITTE des Ereignisfensters. Jedes Bild wird das Fenster mit dem
    // Drehpunkt 0.5/0.5 dorthin gesetzt; waechst es, waechst es in alle
    // Richtungen gleich. Nur Ziehen mit der Maus verschiebt sie. Negativ =
    // beim naechsten Bild auf die Mitte der Arbeitsflaeche setzen.
    ImVec2 editorMitte{-1.0F, -1.0F};
    // Wie breit die Feldreihe im Ereignisfenster GEMESSEN war. Nur fuers
    // Protokoll - die Breite, mit der gerechnet wird, steht seit rc568 vor
    // `Begin` fest (`reiheW` in drawEditor). Siehe dort, warum.
    float editorRowW = 0.0F;
    const Command* editorCmd = nullptr;
    std::vector<std::string> editorValues;
    std::vector<char> editorIsExpr;   // Feld wurde auf Ausdruck umgestellt
    // Wie `editorIsExpr` beim OEFFNEN aussah.
    //
    // Nur damit laesst sich sagen, ob der Benutzer den Ausdrucksmodus
    // ANGEFASST hat. `argForParam` gibt das alte Argument unveraendert
    // zurueck, solange der Text derselbe ist - samt seiner Art. Nach einem
    // Revert stand deshalb beim naechsten Oeffnen wieder "Expr!".
    std::vector<char> editorWarExpr;
    // Welche Felder nach einer Aenderung neu zu befuellen sind.
    //
    // Anlass: wer die Auswahlliste von SET_PARM1 auf SET_HEALTH stellt,
    // bekommt zwar ein Zahlenfeld statt eines Textfeldes - im Feld stand
    // aber weiter "DEFAULT". Die Feldbeschreibungen wurden jedes Bild neu
    // berechnet, die WERTE nicht.
    //
    // -1 heisst: nichts zu tun. Sonst die Stelle, ab der es weitergeht.
    int editorRefreshFrom = -1;
    // Der Beenden-Knopf hat gedrueckt. Die Fensterschicht liest es und
    // schliesst - der Knopf selbst kann das nicht, er kennt kein Fenster.
    bool wantQuit = false;
    // Wurde schon gefragt?
    //
    // Der Beenden-Knopf fragt selbst und setzt dann wantQuit. Die
    // Fensterschicht schickt daraufhin WM_CLOSE - und DAS fragt seit rc145
    // ebenfalls. Ergebnis: zweimal derselbe Dialog, gemeldet als "I have to
    // click No twice for it to close".
    //
    // Also merken, dass die Frage schon beantwortet ist.
    bool quitConfirmed = false;

    // --- Die Frage "Speichern?" im eigenen Stil ---------------------------
    //
    // Vorher war es ein Windows-Systemdialog (MessageBox). Der sieht aus wie
    // aus einem anderen Programm - graue Knoepfe, eigene Schrift, eigene
    // Farben - und genau das war der Einwand.
    //
    // Ihn nachzubauen ist mehr als Kosmetik: eine MessageBox ANTWORTET
    // SOFORT, ein ImGui-Fenster spannt sich ueber mehrere Bilder. Aus
    //
    //     if (confirmDiscard()) { oeffnen(); }
    //
    // wird deshalb
    //
    //     withUnsaved([] { oeffnen(); });
    //
    // Was nach der Antwort geschehen soll, wird MITGEGEBEN und aufgehoben,
    // bis der Anwender geklickt hat.
    // Eine Ja/Nein-Frage im Stil einer MessageBox - "Open?", "Exit?",
    // "Copy entire script?", "You seem to be pasting ...". Die Antwort wird
    // erst nach dem Schliessen ausgefuehrt (drawFrage).
    bool frageOffen = false;
    std::string frageText;
    std::function<void()> frageJa;
    std::function<void()> frageNein;
    // Der Text, den behaved zuletzt selbst in die Windows-Zwischenablage
    // gelegt hat. Steht er noch drin, gilt die interne Ablage (samt allem,
    // was ein Text nicht traegt).
    std::string ablageText;
    // Die Ablage des ANWENDERS - fuer alle Reiter (rc601). Document::clip_ ist
    // nur noch der Zwischenschritt beim Einfuegen: er stirbt mit seinem Reiter
    // (Schliessen, Neu, Oeffnen, "+"). Gemeldet: "copy a command, close the
    // script file - it won't paste into another file". Geschrieben NUR in
    // ablageNachWindows, zusammen mit ablageText: steht in Windows noch genau
    // dieser Text (auch ueber den Verlauf Win+V), gehoert diese Ablage dazu.
    std::vector<Node> ablage;
    bool askSaveOpen = false;
    std::string askSaveName;
    std::function<void()> askSaveThen;
    // Was bei "Abbrechen" geschehen soll. Meist nichts - beim Beenden aber
    // doch: dort muss die angefangene Kette zurueckgesetzt werden.
    std::function<void()> askSaveCancel;
    // Ab welchem Reiter beim Beenden noch zu fragen ist.
    //
    // Ohne das fragte die Kette ewig: "Nein" heisst wegwerfen, aber es
    // macht das Dokument nicht sauber - also fand die naechste Runde
    // DENSELBEN Reiter wieder und fragte erneut. Gemeldet als "Clicking No
    // when asked to save before closing never closes the program".
    //
    // Jetzt merkt sich die Kette, wie weit sie gekommen ist. Bei
    // "Abbrechen" faengt sie von vorn an.
    std::size_t quitAskFrom = 0;
    bool editorForceRefresh = false;   // "Neu auswerten" - alles zuruecksetzen
    std::vector<int> editorKinds;      // Feldarten des vorigen Bildes
    std::vector<std::string> editorDefs;
    // Zustand der Helferzeilen. Einer fuer alle Felder: im Original klappen
    // sie auch nur unter EINEM Feld auf.
    // --- Der Ausdruckshelfer JE SPALTE ------------------------------------
    //
    // Im Original (Bild 2 von shank) hat jede Spalte ihren eigenen Satz aus
    // FLOAT / SET_PARM1+Get / ORIGIN+Tag / Bereich+Rnd, und alle sind
    // gleichzeitig zu sehen. Hier gab es sie EINMAL, und ein eigenes Feld
    // bestimmte, unter welchem Feld sie standen: "when you press the Expr!
    // button, it can only show one at a time."
    //
    // Also je Feld ein eigener Satz. Die Reihen wachsen mit der Zahl der
    // Felder mit; `helferGross()` sorgt dafuer.
    std::vector<std::string> helpGetType;
    std::vector<std::string> helpGetName;
    std::vector<std::string> helpTagName;
    std::vector<std::string> helpTagType;
    // Wo die Felder der Reihe stehen - fuer die Spalten darunter. Wird beim
    // Zeichnen der Reihe vermerkt, damit die Helfer genau unter ihrem Feld
    // sitzen und nicht unter einer geschaetzten Stelle.
    bool dumpAngefordert = false;  // Debug > Alles ins Protokoll
    bool layoutOpen = false;   // Debug > Aufteilung zeigen
    // Gemessene Breiten der Kartenansicht, fuer denselben Auszug. Sie
    // entstehen tief in `drawEventsList` und `drawMapView`; ohne sie muesste
    // man die Luecke zwischen Karte und Skriptbaum wieder schaetzen.
    // Fensterbreite des VORIGEN Bildes.
    //
    // Eine feste Tabellenspalte behaelt beim Vergroessern des Fensters ihre
    // Pixelbreite - der Anteil aendert sich also von allein. Ohne diesen
    // Vergleich ist eine Fenstergroessenaenderung von einem Ziehen nicht zu
    // unterscheiden, und der gespeicherte Wert wandert bei jedem Ziehen am
    // Fensterrand mit.
    float letzteFensterB = 0.0F;
    // Wie viele Bilder die Fensterbreite schon gleich geblieben ist.
    //
    // Eine ImGui-Tabelle uebernimmt eine neue Spaltenbreite erst im
    // NAECHSTEN Bild. Ein Bild Ruhe reicht deshalb nicht - siehe die
    // Begruendung bei `fensterStabil` in drawMain().
    int fensterStabilSeit = 0;
    // --- Gemessene Kanten des Ereignisfensters --------------------------
    //
    // Sechs Runden lang habe ich die Ausrichtung der beiden Zeilen geraten.
    // Beim Hauptfenster war der Wendepunkt, dass die Zahlen sichtbar wurden
    // (rc491) - hier waren sie es nie.
    //
    // Gemessen wird die RECHTE KANTE beider Zeilen in derselben Spalte.
    // Sind sie gleich, stimmt die Ausrichtung; ist die untere groesser,
    // ragen die Helfer hinaus.
    float dbgZelle0 = 0.0F;    // Zellbreite der ersten Spalte
    float dbgFeldEnde = 0.0F;  // rechte Kante der Feldzeile
    float dbgHelferEnde = 0.0F;  // rechte Kante der Helferzeilen
    // Was die Spalte BESTELLT bekam, gegen das, was die Zelle liefert.
    // Weichen beide auseinander, hat ImGui die feste Spalte vergroessert -
    // und genau daraus entstand die Rueckkopplung in rc549.
    float dbgBestellt0 = 0.0F;
    float dbgKnopfB = 0.0F;    // knopfB der Feldzeile
    float dbgKnopfW = 0.0F;    // knopfW der Helferzeilen
    float dbgLetzteDiff = -9999.0F;  // zuletzt protokollierte Differenz
    float dbgComboMinB = 0.0F; // Mindestbreite des letzten Klappfensters
    // Hoehe, die das Kindfenster der Klappliste bekommt. Ausdruecklich
    // gerechnet, damit Klappfenster und Kind sich nicht gegenseitig
    // aufschaukeln - siehe comboEdit().
    float dbgComboListenH = 0.0F;
    float modeBarW = 0.0F;     // gemessene Breite der Modusleiste
    int reiterWahl = -1;       // rc609 T1: ImGuis Wahl in der Reiterleiste der Skripte (je Fenster, tabfenster.cpp)
    float dbgZelleW = 0.0F;    // die Tabellenzelle
    float dbgRestW = 0.0F;     // was nach der Ebenenliste uebrig ist
    float dbgBildW = 0.0F;     // die Kartenflaeche selbst
    std::vector<float> editorFeldX;
    // Und wie BREIT jedes Feld ist. Die Helferspalte darunter richtet sich
    // danach - vorher nahm sie eine feste Zahl und war breiter als ihr
    // Feld. Die Bloecke ueberlappten sich, und wer oben lag, bekam den
    // Klick: shank konnte nur den ERSTEN Parameter aendern.
    std::vector<float> editorFeldW;
    std::vector<float> helpRangeLow;
    std::vector<float> helpRangeHigh;
    Path editorPath;        // leer = neuer Knoten, sonst der zu aendernde
    bool editorInsert = false;

    // Suchen
    bool prefsOpen = false;
    // Stand der Einstellungen beim Oeffnen des Fensters - "Cancel" (und das
    // X) spielen ihn zurueck. Das Original hat OK und Cancel.
    Settings prefsVorher;
    bool prefsHatVorher = false;
    // Alt+M: die MRU-Liste oeffnen (das Klappfenster gehoert dem Knopf).
    bool mruOeffnen = false;
    bool aboutOpen = false;
    bool changelogOpen = false;   // Hilfe: Changelog (alle Fassungen, jede Sprache)
    bool findOpen = false;
    // Das Meldungsfenster. Die Statuszeile zaehlt nur; wer nachlesen will,
    // klickt den Zaehler an.
    bool messagesOpen = false;
    // Eigene Fenster fuer Tastenkuerzel und Spielordner.
    //
    // Vorher steckte alles im Einstellungsfenster untereinander - man
    // musste an den Ordnern vorbeiscrollen, um an die Tasten zu kommen, und
    // umgekehrt. Drei Sachen, drei Fenster.
    bool keysOpen = false;
    // Eine Zeile im Kuerzelfenster wartet auf eine Taste - dann loest kein
    // Kuerzel etwas aus, sonst wirkte derselbe Druck doppelt.
    bool kuerzelWartet = false;
    bool pathsOpen = false;
    // Die Uebersicht ueber das Zusammenspiel der offenen Skripte.
    bool interplayOpen = false;

    // War beim BEGINN dieses Bildes ein Dialog offen?
    //
    // Die Schalter oben taugen dafuer nicht: ein Dialog, der mit Eingabe
    // schliesst, setzt seinen Schalter mitten im Bild auf false. Die
    // Tastenbehandlung laeuft danach, sieht "kein Dialog offen" und
    // verarbeitet dieselbe Eingabetaste ein zweites Mal - der Editor ging
    // zu und sofort wieder auf.
    //
    // Die Regel lautet: wer beim Bildanfang offen war, hat die Taste
    // bereits bekommen. Sie darf im selben Bild nicht noch einmal wirken.
    bool dialogAtFrameStart = false;
    // "Whole-string match only" ist im Original vorgewaehlt.
    bool findWhole = true;
    // Ersetzen im Find-Fenster (wie Notepad++): nur innerhalb der Auswahl?
    bool findInSelection = false;
    // Find war zu Beginn des Bildes offen (wie dialogAtFrameStart, aber
    // getrennt: bei Find gehen Rueckgaengig/Wiederholen durch).
    bool findAtFrameStart = false;
    // Der Ersatztext - fuer den Selbsttest von aussen setzbar.
    std::string findErsatz;
    // "Find similar" im Rechtsklickmenue: das Suchfenster beim naechsten
    // Zeichnen mit diesen Angaben fuellen.
    bool findVorbelegen = false;
    std::string findVorName;
    std::string findVorText;
    // Welches Kuerzel zuletzt ausgeloest hat, und wie oft ueberhaupt eins -
    // fuer den Selbsttest, der jede Belegung aendert und ausprobiert.
    keys::Action letzteKuerzelAktion = keys::Action::Unhandled;
    int kuerzelZaehler = 0;
    // Nur fuer den Selbsttest: Open, Save as und Save all melden, statt sie
    // auszufuehren. Die ersten beiden oeffnen einen Windows-Dialog, das
    // dritte saehe auch andere Reiter - beides darf ein Test nicht.
    bool kuerzelNurMelden = false;
    // Wie viele Zeilen im letzten Bild als "gleich" hinterlegt waren - fuer
    // den Selbsttest.
    int gleicheSichtbar = 0;
    // Die zuletzt gesuchten Angaben - F3 und Umschalt+F3 suchen damit
    // weiter, auch wenn das Suchfenster zu ist.
    FindOptions findLetzte;
    std::vector<Path> findHits;
    int findAt = 0;
};

// Der eine Zustand. Definiert in app.cpp.
extern App* g_app;
// rc605 FU4 (Selbsttest geschuetzanim, app_view3d.cpp): Drehung eines Knochens des
// Wachgeschuetzes durch den Schwenk ohne Feind, in Grad (-1: nicht messbar).
float pasKnochenDrehung(std::size_t ent, double ms, const std::string& knochen);

// Masseinheiten des Originaldialogs.
float dx(float v);
float dy(float v);

// --- Ueber Dateigrenzen hinweg benutzt --------------------------------
//
// Nur, was wirklich woanders gebraucht wird. Alles Uebrige bleibt in
// seiner Datei und in einer anonymen Namensgruppe.
std::string slurp(const std::string& path);
std::string directoryOf(const std::string& path);
std::string fileName(const std::string& path);
// Eine Datei aus den Spielordnern lesen.
//
// fromArchive nimmt, wenn es nicht null ist, den Pfad des Archivs auf, aus
// dem die Datei kam. Das ist keine Spielerei: Movie Duels ERSETZT Dateien
// des Originalspiels unter demselben Namen, und ohne diese Angabe sieht man
// im Protokoll nicht, welche Fassung man geladen hat.
//
// rc609 W2b F17: wie das Spiel auch die losen Dateien der Spielordner
// (liesWieSpiel, bhed/pk3.h; fs_dirbeforepak aus den Einstellungen). Kommt die
// Datei lose, steht ihr voller Pfad in fromArchive und istLose ist true.
bool readFromArchives(const std::string& name, std::string& out,
                      std::string* fromArchive = nullptr, bool* istLose = nullptr);
void rescanGamePaths();
void refreshPk3List();
void rebuildTree();
// Der Fenstertitel (app.cpp; im Map-Editor wie NRCs Sys_SetTitle, Stufe 7 Punkt 6f)
std::string windowTitle();
void setStatus(const char* fmt, int a, int b);
// Steht das Skript schon in einem Reiter? -1 = nein (gui/app.cpp).
int reiterMitPfad(const std::string& p);
int reiterMitSkript(const std::string& name);
void oeffneSkriptAusSpeicher(const std::string& data, const std::string& name);
void addStatus(const std::string& line);

// Schreibt den Bericht INS PROTOKOLL - Grafikkarte, Zustaende, Zaehler,
// Zeiten, Shader und die Meldungen der Debugschicht.
//
// Ohne Zutun aufgerufen: beim Beenden und beim Absturz. Es gibt keinen
// Knopf dafuer, und das ist Absicht - ein Werkzeug, das man erst
// einschalten muss, ist genau dann aus, wenn man es braucht.
void schreibeBericht();
// abMs: an welcher Stelle IM KLANG begonnen wird. Null heisst von vorn.
// Gebraucht, wenn der Ton mitten im Abspielen wieder eingeschaltet wird.
// Einen Klang abspielen. `background` heisst Musik: eigenes Geraet, und ein
// neues Stueck ersetzt das laufende - siehe musicAudio.
// `lautstaerke` ist 1 fuer alles, was ohne Ort gespielt wird (Sprache,
// Musik, Vorschau). Effektklaenge geben die Entfernungsdaempfung mit -
// siehe lautstaerkeAus in app.cpp.
void playSound(const std::string& name, double abMs = 0.0,
               bool background = false, float lautstaerke = 1.0F,
               float rechts = -1.0F, std::uint64_t schluessel = 0);

// Einen Klang als SCHLEIFE fuer `dauerMs` spielen - die Fahrgeraeusche der
// Mover (loopSound, BMS_MID). Der Puffer wird bis zur Dauer wiederholt.
void playSoundSchleife(const std::string& name, double dauerMs, float lautstaerke, float rechts = -1.0F,
                       std::uint64_t schluessel = 0);
// Musik mit Intro und Schleife (S_StartBackgroundTrack), ab abMs.
void spieleMusik(const std::string& spec, double abMs);
// Die Levelmusik aus dem worldspawn ("music"), leer wenn keine.
std::string weltMusik();
// Klang hinter einem use (target_play_music, target_speaker), leer ohne (app_view3d.cpp)
std::string useSoundFor(const std::string& ziel, bool* istMusik);

double soundLengthMs(const std::string& name);
// Klaenge am Ort, wie die Engine sie hoert (app_view3d.cpp)
void spieleSkriptKlang(const std::string& werSpur, const std::string& datei, const std::string& kanalName,
                       double ms);
void spieleLautsprecher(const std::string& name, const std::string& datei);
// ICARUS-Nachbau (app_view3d.cpp)
std::string aktiverSkriptPfad();
const Script* skriptFuerAblauf(const std::string& pfad);
// Der Reiter mit dem Skript `pfad` (ICARUS-Name, z. B. "md_ga/kill_fett2"): aus dem
// Speicher geoeffnet oder aus einer Datei unter .../scripts/. aktivLebend: beim
// aktiven Reiter gelten die Angaben der App statt der geparkten. -1: keiner.
int reiterMitAblaufSkript(const std::string& pfad, bool aktivLebend);
std::shared_ptr<const Roff> roffFuer(const std::string& name);
AblaufWelt ablaufWelt();
void pfadeAufsSkript();
void kinoSkeletteAbgleichen();
// Gruppen der Entity-Liste (und Farben der Marken in der Ansicht).
enum class UiIcon;
struct EntityKat {
    Str titel;
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    UiIcon icon;   // in der Liste und in der Karte (rc588)
};
int entityKategorien();
const EntityKat& entityKategorieInfo(int k);
int entityKategorie(const MapEntity& e);
// Das Icon einer Entity: das ihrer Gruppe, sonst nach Klasse (Ausloeser,
// Spielerstart, Kartenwechsel, Licht ...), sonst das allgemeine.
UiIcon entityIcon(const MapEntity& e);
std::string entitySkriptName(const MapEntity& e);
bool entityOrt(const MapEntity& e, float out[3]);
// Wie entityOrt, aber dort, wo die Entity zur eingestellten Zeit zu sehen ist
// (rc597): Schluessel "origin" plus App::entVersatz.
bool entityOrtAnzeige(const MapEntity& e, float out[3]);
// Die gezeigte Blickrichtung in Grad: Schluessel "angle" plus App::entGierVersatz.
float entityGierVersatz(int ei);
float entityGierAnzeige(const MapEntity& e);
void entityVersatz(int ei, float d[3]);
// Die Entity des Modells unter dem Bildschirmpunkt (Schiff, Tuer, Kartenmodell),
// -1 = keines.
int moverUnterMaus(float sx, float sy);
// Der NPC-Spawner einer Figur der Szene (-1 = keiner).
int entityFuerFigur(int figur);
// Umgekehrt: die Figur der Szene zu einer Entity - ein Spawner ueber den
// Namen, ein Brush-Modell ("*N") ueber Actor::brushModel (-1 = keine).
int figurFuerEntity(int ei);
// Wird die Entity gerade gezogen? Dann um wie viel (Ursprung, Winkel) seit
// dem Anfassen - damit Figur und Schiff live mitgehen (rc597).
bool zugVersatz(int ei, float d[3], float& dGier);
// Ebenso Nicken und Rollen seit dem Anfassen (rc599, Drehmodus).
bool zugKippen(int ei, float& dNicken, float& dRollen);
// Die Leiste Verschieben | Drehen fuer eine gewaehlte Entity (rc599).
void entityGizmoLeiste();
// Bahnpunkt mitten in einer Bewegung (rc597): bewegt sich die Figur bzw. das
// Schiff der Entity zur Zeit ms gerade (Lauf oder move), welche Figur und
// welcher Befehl - und laesst sich dort teilen?
bool bahnPunktMoeglich(int ei, double ms, int& figur, Path& pfad, bool& lauf);
// mission1: gemeinsames Rueckgaengig fuer Entities und Skript (app_mission.cpp).
std::uint64_t entUndoOben();
std::uint64_t entRedoOben();
void verbindeMitSkriptSchritt();
bool missionRueckgaengig();
bool missionWiederholen();
bool fahrtSchluesselMoeglich(int ei, double ms, int& figur, Path& vorPfad);
bool fahrtSchluesselSchreiben(int figur, double ms, const float kOrt[3], const float kWinkel[3]);
bool laufZielNachLaufMoeglich(int ei, double ms, int& figur);
bool laufZielNachLaufSchreiben(int figur, const float kOrt[3]);
// Die Bewegung bei ms teilen, mit dem neuen Punkt K: ein Schiff faehrt bis
// ms nach K und von dort weiter; eine Figur bekommt ein neues Laufziel bei K.
// Die Befehle danach behalten ihre Zeit.
bool bahnPunktSchreiben(int figur, const Path& pfad, bool lauf, const float k[3], double ms,
                        const float* kWinkel = nullptr);
// Die Blickrichtung einer Figur zur Zeit ms als set ( SET_ANGLES ) - mitten
// im Laufen beim Ankommen (rc599).
bool figurBlickSetzen(int figur, double ms, float gier);
// Gizmo am Zeiger, waehrend das Animationsfenster diese Entity zeigt (rc599).
bool animKeyMoeglich(int ei, double ms);
bool animKeySetzen(double ms, const float ort[3], const float winkel[3]);
// Griffe auf der Bahn (rc597, gui/app_mission.cpp): Maus (vor entityZiehen),
// Zeichnen samt Menue Gehen/Rennen, und fuer den Selbsttest, wo sie liegen.
void bahnGriffeMaus();
void bahnGriffeZeichnen();
int bahnGriffZahl();
bool bahnGriffPunkt(int nr, ImVec2& out);
int bahnGriffArt(int nr);
// Gehen oder Rennen ab diesem SET_NAVGOAL - schreibt SET_WALKING/SET_RUNNING
// direkt davor (vorhandene werden umgestellt). Ein Rueckgaengig-Schritt.
void gangartSetzen(const Path& navgoal, bool gehen);
// Blick beim Laufen (rc597): SET_WATCHTARGET direkt vor dem SET_NAVGOAL -
// auf eine Entity ("NULL" = in Laufrichtung). rueckwaertsLaufen legt eine
// Marke (ref_tag) hinter den Start und laesst die Figur auf sie schauen.
void blickSetzen(const Path& navgoal, const std::string& ziel);
void rueckwaertsLaufen(const Path& navgoal);
void gangartMenueInhalt();
void zurEntityFliegen(std::size_t i);
void entityMenue(std::size_t i);
bool verschiebeImAblauf(const Path& p, double delta, bool nurDieser, Path* warte = nullptr, std::string* vorher = nullptr);
// zeitleiste1 (gui/app_zeitleiste.cpp)
void zeitleisteModellBauen();
// Zeitleiste 2 Punkt 7 (app_zeitleiste.cpp): g_subtitles der Vorschau, der
// Schalter in der Transportleiste und die Untertitel im Spielrahmen a..b
// (nach Balken und Blende, wie CG_Draw2D).
[[nodiscard]] int untertitelStufe();
void untertitelSchalter();
void untertitelZeichnen(ImDrawList* dl, ImVec2 a, ImVec2 b);
// Zeitleiste 2 Punkt 1 (app_mission.cpp): eine Figur steuern, daraus Laufziele.
[[nodiscard]] bool aufnahmeLaeuft();
[[nodiscard]] bool aufnahmeMoeglich(int figur);
void aufnahmeStarten(int figur);
void aufnahmeBeenden(bool schreiben);
void aufnahmeKnopf();                                    // in der Transportleiste
void aufnahmeBild(ImDrawList* dl, ImVec2 a, ImVec2 b);   // jedes Bild: Steuern, Spur, Hinweis
[[nodiscard]] int aufnahmeProben();                      // Selbsttest
[[nodiscard]] int aufnahmePunkte();                      // Selbsttest: Laufziele mit der Toleranz
// rc608: Die Spielerfigur fehlt in dieser Szene zu dieser Zeit nur im Bild (Actor::vorschauVerborgen).
// Wer sie gerade selbst steuert, sieht sie immer; "Show player" zeigt sie wie im Spiel.
[[nodiscard]] bool spielerNichtZeichnen(const Actor& a, double ms);
// rc608: Sitzt die Figur gerade auf einem Fahrzeug? Dann Ort, Blick und Animation vom Fahrzeug (AttachRiders,
// AnimateRiders); reitet in `d`. Nur im Bild: Ziele und Ton rechnen weiter mit dem Stand aus dem Skript.
void reitenAnwenden(const Actor& a, double ms, ActorState& st, ActorDraw& d);
// rc608 Teil 13d: Ein gerittener Speeder oder ein Tier schaut nach dem Aufsteigen dorthin, wohin sein Reiter schaut
// (ProcessOrientCommands); ein Tier steht in BOTH_VT_IDLE1 (AnimateVehicle). `fahrzeug` ist der Index in
// scene.actors, `sv` sein Stand zu `ms`.
void fahrzeugLenken(std::size_t fahrzeug, double ms, ActorState& sv);
const App::Skeleton* skeletonFor(const GlmModel& model);
void aufnahmeFigurZustand(int figur, double ms, ActorState& st);   // das Modell am gesteuerten Ort
// Zeitleiste 4 Punkt 3: was PM_Footsteps von der Figur weiss, und ob ihr Modell
// eine Animation kennt (PM_SetAnimFinal setzt sonst nichts).
PmFigur aufnahmePmFigur(int figur, double ms);
bool aufnahmeAnimBekannt(int figur, const std::string& anim);
void aufnahmeHinweis(ImVec2& a, ImVec2& b);              // Selbsttest: Rechteck des Hinweises im letzten Bild
float zeitleisteNamenBreite(float verfuegbar);
void zeitleisteNavigation(float x0, float w, double gesamt);
void zeitleisteSpuren(float x0, float w, float labelW, double sichtStart, double sichtSpanne, double ablaufEnde, float hoehe);
float zeitleisteErklaerungHoehe(float verfuegbar);
void zeitleisteErklaerung(float breite, float hoehe);
void zeitleisteDauerSchreiben(const spuren::Clip& c, double neuMs);
void zeitleisteVerschieben(const spuren::Clip& c, double delta, bool nurDieser);
void zeitleisteFilmBild(int nummer, const std::vector<std::uint8_t>& rgba, int breite, int hoehe);
std::string untertitelFuer(const std::string& klang, std::string* schluessel);
void openEditorForNode(const Path& p);
std::vector<Path> selectionOrCurrent();
bool isSelected(const Path& p);
void selectRow(std::size_t rowIndex, bool ctrl, bool shift);
void doLoadMap();

// Die drei Ansichten der linken Spalte.
void drawMapToolbar();
void drawPickedEntity();
void drawEntityLayers(float height);
CameraState camStateAt(double ms);
// Auto-Key (rc597): steht die Zeit MITTEN in einer Kamerafahrt (nicht an
// ihrem Ende), der Schluessel dieser Fahrt, sonst -1.
int kameraZwischenSchluesselNr(double ms);
// Die Fahrt bei ms in zwei teilen: ein neuer Schluessel genau dort, mit der
// Stellung, die die Kamera dort hat; Anfang und Ende bleiben, alles danach
// behaelt seine Zeit. Waehlt den neuen Schluessel. Ein Rueckgaengig-Schritt.
bool kameraSchluesselEinfuegen(double ms);
// Die Kamera der Bild-im-Bild-Vorschau, oder false, wenn keine gezeigt wird
// (siehe App::kameraBildImBild). `aspekt` = Breite / Hoehe des Bildes.
bool vorschauKamera(Camera& vc, float aspekt);
// Die Vorschau ueber die Kartenansicht legen (nach dem Kartenbild).
void kameraVorschauZeigen();
// Schaltbare und flackernde Lichter zur Zeit der Zeitleiste in die Lightmaps
// mischen (rc593). Billig, wenn sich nichts geaendert hat.
void lichtStileZuZeit();
// Der Ort des misc_skyportal der Karte (rc593), false ohne eines.
bool himmelPortalOrt(float ort[3]);
// Der Spiegel oder die Portalkamera, die die Kamera gerade sieht (rc593, rc609 P: R_MirrorViewBySurface ueber
// bhed/portalsicht): Ebene n.p = d, die Sicht in App::nebenSicht. false ohne eine. breite/hoehe: das Bild.
bool spiegelFuer(const Camera& cam, int breite, int hoehe, float ebene[4]);
// rc609 P, gemeinsam mit dem Karten-Editor (app_portalsicht.cpp): die Flaechen mit `sort portal` einer .bsp
// (R_PlaneForSurface) mit ihren Blaettern und die misc_portal_surface; die Sicht fuer eine Kamera (nur Flaechen und Schirme,
// die vom Auge aus im Bild sind: R_MarkLeaves, Snapshot); die Hauptsicht als PortalAuge und die Sicht als NebenSicht.
struct PortalKarte {
    std::vector<PortalFlaeche> flaechen;
    std::vector<std::vector<int>> blaetter;   // je Flaeche die Blaetter, in denen sie steht
    std::vector<PortalSchirm> schirme;
};
[[nodiscard]] PortalKarte portalKarteAus(const BspGeometry& geo, const TextureSet& bilder, const std::vector<MapEntity>& ents);
[[nodiscard]] bool portalSichtFuer(const PortalKarte& k, const BspGeometry& geo, const AreaPortale& portale, const Camera& cam, int breite, int hoehe,
                                   double zeitMs, PortalSicht& aus);
[[nodiscard]] PortalAuge portalAugeAus(const Camera& cam, int breite, int hoehe, float fern);
void nebenSichtAus(const PortalSicht& p, App::NebenSicht& ns);
// Die MOVE-Schluessel des Skripts, in der Reihenfolge, in der App::selectedKey
// zaehlt: Ort, Winkel (erstes PAN danach, sonst das vorige) und Zeile.
struct KameraSchluessel {
    float pos[3]{};
    float ang[3]{};
    bool hasAng = false;
    Path path;
    Path panPath;             // das PAN dieser Einstellung (leer: keins, Winkel vom vorigen)
    std::string marke;        // MOVE steht auf tag( "name", ORIGIN ) - dann der ref_tag
    std::string panMarke;     // PAN steht auf tag( "name", ANGLES )
};
// Eine Kameramarke (ref_tag, waypoint_navgoal) in der Karte aufloesen - Ort und
// Winkel, wie das Spiel sie sieht (ein target dreht den ref_tag zum Ziel).
bool kameraMarkeOrt(const std::string& name, float* o, float* a);
std::vector<KameraSchluessel> kameraSchluessel(const Script& script);
// Der Schluessel, der zur Zeit ms gilt (letzter begonnener MOVE), oder -1.
int schluesselFuerZeit(double ms);
// Eine Karte ueber ihre Kennung aus den Archiven holen (Mission aus .pk3).
bool loadMapFromArchive(const std::string& id);
// rc602 kartenpruefung: der aufgestellte Gegenstand (item_*, weapon_*, ammo_*)
// zu einer Entity - Ort nach dem Fallen, Modell; false = keiner (spawnt nicht,
// wartet auf use, unsichtbar, im Festen entfernt). Fuer Pruefung und Selbsttest.
bool gegenstandInstanz(std::size_t entity, float ort[3], std::string* modell, bool* glm);
// Steht der Gegenstand im Spiel NICHT (wartet auf use, Schwierigkeit/
// notsingle, ITMSF_INVISIBLE, im Festen entfernt)? Dann mit Grund.
bool gegenstandNichtImSpiel(std::size_t entity, std::string* grund);
// Stufe 6 (OpenJK#1358): ein misc_model_static, dessen Pruefpunkt origin + (0 0 zoffset+1)
// im Festen liegt - das Spiel zeichnet es nie (Vorschau und Kartenpruefung).
bool miscModelStaticImFesten(const MapEntity& e);
bool miscModelStaticImFesten(const MapEntity& e, const BspGeometry& geo);
// rc608 Teil 13b: Wohin kartenModelleAufstellen die Kartenmodelle stellt - die Vorschau (g_app) oder das Spielbild des
// Karten-Editors. Dieselben Regeln (G_SpawnGEntityFromSpawnVars, SP_misc_model_static, die festen Modelle, model2).
struct KartenModellZiel {
    const std::vector<MapEntity>* entities = nullptr;
    const BspGeometry* geo = nullptr;
    TextureSet* textures = nullptr;
    std::map<std::string, Md3Model>* dateien = nullptr;
    std::vector<App::MapModel>* modelle = nullptr;
    std::vector<std::string>* fehlend = nullptr;
};
void kartenModelleAufstellen(const KartenModellZiel& z, int& gefunden, int& fehlt);
// rc604: G_SpawnGEntityFromSpawnVars entfernt jede Entity mit "notsingle" oder dem Bit der Schwierigkeit
// (spawnflags & (1 << (8 + g_spskill)), code/game/g_spawn.cpp:1064-1075, 1096-1101 OpenJK, :1260-1265 MD) - vor ihrer
// Spawnfunktion. Vorschau und Spielbild des Karten-Editors zeigen sie dann nicht.
bool gespawntImSpiel(const MapEntity& e);
// rc608 Teil 13c: Die Brush-Modelle ("*N"), die das Spiel zeichnet, und ihre Stellung - eine Regel fuer die Vorschau und
// das Spielbild des Karten-Editors. Die Welt ist Modell 0 (R_AddWorldSurfaces); jedes andere Modell kommt nur ueber seine
// Entity ins Bild (CG_AddCEntity: s.modelindex, an currentOrigin/currentAngles), also nur mit einer Entity, die das Spiel
// spawnt (G_SpawnGEntityFromSpawnVars) und die es zeichnet (kein trigger_*, SVF_NOCLIENT). Die Stellung kommt aus der
// Mover-Simulation (InitMover: pos1, bei START_OPEN pos2; func_wall/func_usable START_OFF unsichtbar; func_rotating,
// func_bobbing), sonst der Ursprung der Entity (q3map2 zieht ihn bei einem origin-Brush von der Geometrie ab).
// `anderswo[k]`: Modell k zeichnet der Aufrufer selbst (Figuren mit Brush-Modell).
struct BrushStellung {
    int modell = 0;
    float ort[3]{};
    float winkel[3]{};
};
std::vector<BrushStellung> brushModelleImSpiel(const std::vector<MapEntity>& entities, const BspGeometry& geo, const MoverSim& sim, double ms,
                                               const std::vector<bool>& anderswo);
// rc608 Teil 13c: Ist das Gebietsportal einer Entity zur Zeit ms offen (SV_AdjustAreaPortalState)? Eine Tuer, sobald sie
// ihre Anfangsstellung verlassen hat (Use_BinaryMover; zu wieder in MOVER_POS1), func_wall/func_usable/func_breakable,
// solange sie nicht zu sehen sind. Vorschau (areaPortaleJetzt) und Spielbild des Karten-Editors.
bool portalOffenImSpiel(const PortalEntity& p, const MoverSim& sim, double ms);
// Selbsttest: einen Gegenstand verstecken (Vergleich mit/ohne).
void gegenstandVerstecken(std::size_t entity, bool aus);
// Die Bytes einer Karte ueber ihre Kennung: aus den Archiven, sonst ueber
// kartenQuelle (neue Mission).
bool kartenBytes(const std::string& id, std::string& out);
// Die jetzigen Entities der Karte merken (entitiesJeKarte) - nach .ent und
// nach jeder Bearbeitung.
void merkeEntities();
// Vor dem ausdruecklichen Oeffnen einer Karte: die gemerkten vergessen.
void vergissEntities(const std::string& kennung);
// Breite der Entity-Spalte im Map-Tab, in Zeichenhoehen. Seit rc586 18 statt
// 15: der Inspektor steht darin, und rechts neben der Ansicht ist die
// Seitenleiste weggefallen.
inline constexpr float kEntitySpalte = 18.0F;

// --- Bedien-Icons (gui/uiicons.cpp) ----------------------------------------
// M4: kleinste Oberflaechengroesse (Grundschrift 16 px -> mindestens 12 px, wie die
// kleinste Stufe im Ansicht-Menue)
inline constexpr float kUiScaleMin = 0.75F;
void setUiScale(App* a, float s);   // begrenzt auf kUiScaleMin..3 (app_ablage.inc)

enum class UiIcon : int {
    Neu, Oeffnen, Speichern, Mehr, Karte, Paket, Liste, Skript, Entity, Namen, Figur, Effekt,
    Leuchten, Licht, Nebel, Himmel, Kamera, Einstellungen, Statistik, Plus, Boden, Hinfliegen,
    Duplizieren, Loeschen, Rueckgaengig, Wiederholen, Explorer, Suche, Verschieben, Drehen,
    Mission, Schliessen, Haken, Warnung, Abspielen, Pause,
    Zurueck, Vor, Zielen, Steuern,
    // Wegnetz und Draufsicht (rc589)
    Pfad, Oben, Erzeugen,
    Raster, Animieren,
    // Zeitleiste (rc588)
    ZumAnfang, Schleife, Ton, Folgen, Zeilen, Ruhig, ZoomPlus, ZoomMinus, Einpassen,
    // Entity-Arten in Liste und Karte (rc588)
    Marke, Blitz, Tuer, Wegpunkt, Bruch,
    // Sichtbarkeit in der Entity-Liste (rc588, statt "O" und "-")
    Sichtbar, Versteckt,
    // Bild-im-Bild der Kamera (rc592)
    Bildschirm,
    // Menue-Editor M4: Ton aus, Schloss zu/offen (Ebenenliste)
    TonAus, Schloss, SchlossOffen,
    // rc608 Teil 3b: Speichern unter neben Speichern; zwei gleiche Disketten sagten nichts
    SpeichernUnter,
};
void uiIcon(ImDrawList* dl, UiIcon icon, ImVec2 pos, float groesse, ImU32 farbe);
// Quadratischer Icon-Knopf; der Text ist Kennung und Tooltip-Titel.
// umschalter: ein Ein/Aus-Schalter - ausgeschaltet wird das Icon gedaempft.
bool iconKnopf(const char* text, UiIcon icon, const char* hinweis, bool an = false, bool aktiv = true,
               bool umschalter = false);
ImVec4 uiIconFarbe(UiIcon icon);
// Icon und Text nebeneinander.
bool iconTextKnopf(const char* text, UiIcon icon, const char* hinweis, bool aktiv = true);
float iconTextKnopfBreite(const char* text);
// Das Handbuch im Standardbrowser, auf der Seite der Sprache (rc599; rc600:
// auch eine bestimmte Seite, etwa "tutorials.html" von der Startflaeche).
void handbuchOeffnen(const char* seite = "index.html");

// --- Kamera (gui/app_kamera.cpp) -------------------------------------------
// Die Vorschau am Gizmo (gizmoOffset/gizmoAngles) ins Skript schreiben - beim
// Loslassen. Einstellungen auf einem ref_tag verschieben den ref_tag.
bool kameraVorschauSchreiben();
bool kameraOrtSetzen(int key, const float ort[3]);
bool kameraWinkelSetzen(int key, const float ang[3]);
bool kameraDauerSetzen(int key, double ms);
bool kameraAufAnsicht(int key);
void kameraSchluesselWaehlen(int key, bool zeitMit);
void kameraNeueEinstellung();
bool kameraEinstellungLoeschen(int key);
void kameraPilotStarten();
void kameraPilotBeenden(bool uebernehmen);
// Paket kamera, rc601 (Review): g_app->cam, im Pilot mit dem Bildwinkel der
// freien Ansicht statt dem der Einstellung - zum Merken (Strg+1..9, Draufsicht).
Camera freieKamera();
// Beim Pilotieren: die geflogene Ansicht als Schluessel zur eingestellten
// Zeit (rc597) - mitten in einer Fahrt ein neuer, sonst der dieser Zeit.
bool kameraPilotSchluesselSetzen();
// Die Kamerabahn kraeftig obenauf, wie die der Figuren (rc597): grau voraus,
// gelb schon gefahren, die Schluessel als Quadrate.
void kameraBahnZeichnen();
void kameraUeberlagerung(float rasterSkal);

// --- Das Spielfenster (Paket kamera, rc601; gui/app_view3d.cpp) ---------------
//
// Seitenverhaeltnis und cg_fovAspectAdjust des Spiels aus den Einstellungen
// (0 = wie der Hauptbildschirm, -1 = automatisch nach den Spielordnern) -
// damit rechnet die Ansicht den Bildwinkel wie CG_CalcFOVFromX.
bool movieDuelsOrdner(const std::string& verzeichnis);
float spielFensterSeiten();
bool spielFovAspectAdjust();
SpielFenster spielFensterJetzt();
// Das Rechteck des Spielbilds (g_app->spielRahmen) in der Ansicht a..b.
void spielRahmenRechteck(ImVec2 a, ImVec2 b, ImVec2& ra, ImVec2& rb);
// Ausserhalb des Spielbilds abdecken (g_app->spielRahmenDeckung).
void spielRahmenUeberlagern(ImDrawList* dl, ImVec2 a, ImVec2 b);
// Deckung ausserhalb des Rahmens in der freien Ansicht und im Pilot.
inline constexpr int kSpielRahmenHalb = 140;
// Die Einstellungen dazu im Klappfenster der Ansicht.
void spielFensterEinstellungen();

// --- Wegnetz (gui/app_wegpunkte.cpp) -----------------------------------------
void wegNetzAktuell();                       // neu bauen, wenn sich etwas geaendert hat
void wegUeberlagerung();                     // Netz in der Ansicht
void wegWerkzeugMaus(float rasterSkal);      // Setzen/Verbinden; setzt wegKlickVerbraucht
void drawWegFenster();                       // Fenster "Waypoints"
// rc609 W3b: Erzeugung im Hintergrund. Voll: alles neu (wie bisher "Erzeugen"); Bereich: im gemalten Bereich `bereich`;
// Start: vom angeklickten Boden `saat` aus; Anschluss: die Teile ausser dem Hauptteil; Sprung: nur Sprungkanten. Das
// Ergebnis kommt in EINEM Rueckgaengig-Schritt (wegAuftragPruefen, jedes Bild); `hintergrund` false rechnet sofort.
enum class WegAuftragArt { Voll, Bereich, Start, Anschluss, Sprung };
bool wegAuftragStarten(WegAuftragArt art, const std::vector<std::array<float, 3>>& saat = {}, int bereich = -1, bool hintergrund = true);
[[nodiscard]] bool wegAuftragLaeuft();
void wegAuftragAbbrechen();
void wegAuftragPruefen();
[[nodiscard]] float wegAuftragAnteil();
[[nodiscard]] int wegAuftragSchritt();
// Pfad: Punkte entlang der Linie (Boden), verbunden, wo das Spiel die Kante behaelt (ein Schritt). Zahl der Punkte.
int wegPfadSetzen(const std::vector<std::array<float, 3>>& linie);
void wegBereicheSpeichern();
void wegBereicheAktuell();
int wegGenerierteEntfernen();
bool wegVerbinden(int vonEntity, int nachEntity, bool sprung);
int wegSetzen(const float boden[3], int verbindeMit);
// rc609 W4: setzt einen point_combat am Boden (origin = Boden + 25 wie ein Wegpunkt, kein target), ein Schritt.
int wegKampfSetzen(const float boden[3]);
// rc609 W4: die Kampfpunkte wie im Spiel neu rechnen, wenn sich das Netz geaendert hat (nicht waehrend des Ziehens).
void wegKampfAktuell();
// rc609 W4: Testroute von a nach b - der Weg, den ein NPC im Spiel naehme (GetNearestNode, navAStar); false: keiner.
bool wegRouteRechnen(const std::array<float, 3>& a, const std::array<float, 3>& b);
// rc609 W4: Abdeckung - laeuft sie gerade, und das Ergebnis passt zu den Entities?
[[nodiscard]] bool wegAbdeckungLaeuft();
[[nodiscard]] bool wegAbdeckungAktuell();
// rc609 H2: Kampfpunkt-Vorschlaege hinter Deckung (gui/app_deckung.cpp). Starten rechnet im Hintergrund (`hintergrund`
// false: sofort); Pruefen (jedes Bild) uebernimmt das Ergebnis, wenn die Entities noch dieselben sind, und verwirft
// Punkte, denen das Spiel keinen Wegpunkt zuordnet. Annehmen: ein Vorschlag (Index) oder alle (-1), EIN Schritt.
bool wegDeckungStarten(bool hintergrund = true);
void wegDeckungAbbrechen();
[[nodiscard]] bool wegDeckungLaeuft();
[[nodiscard]] float wegDeckungAnteil();
void wegDeckungPruefen();
int wegDeckungAnnehmen(int index);
void wegDeckungVerwerfen();
// Die Vorgaben: Abstand und Kampfpunkte je Wegpunkt, gemessen an den Originalkarten des Regelwerks.
[[nodiscard]] float wegDeckungAbstandVorgabe();
[[nodiscard]] int wegDeckungHoechstensVorgabe();
void wegDeckungFenster();
void wegDeckungZeichnen(ImDrawList* dl, float px);
// Startpunkte der vollen Erzeugung (gui/app_wegpunkte.cpp).
void wegStartsVoll(const std::vector<MapEntity>& v, std::vector<std::array<float, 3>>& starts, std::vector<int>& startEntity);
// rc609 W1: Verweise (target..target4, targetJump) auf diese Namen (klein
// geschrieben) zaehlen bzw. entfernen - ohne Rueckgaengig-Schritt.
int verweiseZaehlen(const std::set<std::string>& namenKlein);
int verweiseEntfernen(const std::set<std::string>& namenKlein);
// Die Wegpunkte, mit denen Entity i ueber target..target4 verbunden ist (in
// beiden Richtungen, ohne targetJump) - ihre Namen; ohne Namen bekommen sie einen.
std::vector<std::string> wegNachbarNamen(int i);
// Kanten wie im Spiel abgleichen (ohne Rueckgaengig-Schritt): bei `trennen`
// die Kanten der Punkte, die die Pruefung nicht mehr bestehen, weg; dann mit
// den Wegpunkten im Umkreis verbinden (ohne die in `ohne`). Meldet in der
// Statuszeile. Rueckgabe: ob sich etwas geaendert hat.
// rc609 W3b: `umkreis` > 0 statt wegAbstand (ein Bereich schliesst in zwei seiner Rasterweiten an).
bool wegKantenAbgleichen(const std::vector<int>& punkte, bool trennen, const std::vector<int>& ohne = {}, float umkreis = -1.0F);
// Nach dem Loeschen: je Gruppe (Nachbarn eines geloeschten Punkts) die
// Nachbarn untereinander verbinden, wo die Pruefung es erlaubt.
int wegLueckenSchliessen(const std::vector<std::vector<std::string>>& gruppen);
// In Kopien von Wegpunkten zeigen die Verweise untereinander auf die neuen Namen.
void wegKopienUmschreiben(const std::vector<int>& quellen, const std::vector<int>& kopien);
// Die gewaehlten Wegpunkte nach Muster umbenennen, Verweise ziehen mit (ein Schritt).
int wegUmbenennen(const std::string& muster);
// Die gewaehlten Wegpunkte mit ihren Nachbarn verbinden (ein Schritt).
bool wegAuswahlVerbinden();
// Die Spuren fuer den Netzbau der geladenen Karte (gui/app_bewegung.cpp); rc609 W3b: mit einer Kopie der Entities.
[[nodiscard]] NavBauHilfe navBauHilfeKarte();
[[nodiscard]] NavBauHilfe navBauHilfeKarte(const std::vector<MapEntity>& ents);
// rc609 W3a: die Welt der Erzeugung fuer die geladene Karte - Kollision des Spiels mit offenen Tueren, Knoten und
// Kanten wie im Spiel (NavKantenPruefer). rc609 W3b: rechnet auf einer eigenen Kopie der Entities und braucht die
// geladene Karte danach nicht mehr (auch in einem anderen Faden).
struct WegWeltKarte {
    std::vector<MapEntity> ents;
    NavBauHilfe hilfe;
    NavVersetzt tueren;
    std::unique_ptr<NavKantenPruefer> pruefer;
    WegWelt welt;
};
[[nodiscard]] std::unique_ptr<WegWeltKarte> wegWeltKarte();
[[nodiscard]] std::unique_ptr<WegWeltKarte> wegWeltKarte(std::vector<MapEntity> ents);
// --- Draufsicht ---
void draufsichtSchalten(bool an);
// Fuer beide aus app_mission.cpp:
bool kartenPunktAufSchirm(const float w[3], ImVec2& out);
bool kartenStrahlUnterMaus(float sx, float sy, TraceTreffer& t);
void entitiesGeaendert(std::vector<MapEntity> vorher);
std::string freierEntityName(const std::string& prefix);
void drawKameraInspector(float hoehe);

// --- Missionsbau (gui/app_mission.cpp) -----------------------------------
void drawEntityInspector(float hoehe);
// --- Model-Tab (gui/app_modell.cpp, rc591) -----------------------------------
void animWaehlen(int i, bool abspielen);
bool animInsSkript(int i);
void drawAnimListe(float hoehe);
void modellAnsichtEinpassen();
bool npcLaden(const std::string& typ);
const std::vector<std::string>& npcTypenListe();
void drawNpcBrowser();
void modellDrehteller();
void modellUeberlagerung(ImVec2 lo, ImVec2 groesse);
bool modellBrauchtBilder();
void modellFlaechenProtokoll();
// --- Mover animieren (gui/app_animation.cpp, rc592) ---------------------------
bool entityAnimierbar(const MapEntity& e);
void animStarten(int entity);
bool animSchreiben(const std::vector<AnimSchluessel>& k);
int animSchluesselAmZeiger();
void animUeberlagerung();
void animMaus();
void drawAnimFenster();
// ROFF-Bahnen bauen und aendern (rc599): die Bahn aus der Marke eines
// offenen Skripts (nullptr: keine Marke), die .rof-Dateien eines Skripts
// beim Speichern (Anzahl geschrieben), wohin sie gehoeren, und eine fertige
// Datei der gewaehlten Entity zum Bearbeiten in Schluessel zerlegen.
std::shared_ptr<const Roff> roffAusMarken(const std::string& datei);
int roffDateienSchreiben(const std::string& skriptDatei, const Script& s);
std::string roffZielOrdner(const std::string& skriptDatei);
bool roffBearbeiten(const std::string& datei);
// --- Wetter (gui/app_wetter.cpp, rc592) ----------------------------------------
// (gpu::KlingenQuad steht in gpumap.h; hier nur der Name.)
int wetterTeilchen(std::vector<bhed::gpu::KlingenQuad>& aus, const Camera& cam);
// rc608 Teil 13b: dasselbe fuer eine andere Karte (das Spielbild des Karten-Editors, editor = true: eigener Satz).
struct WetterQuelle {
    const std::vector<MapEntity>* entities = nullptr;
    const BspGeometry* geo = nullptr;
    std::string karte;          // Pfad oder Name der .bsp (Weather/<name>.cfg)
    std::uint64_t stand = 0;    // aendert sich mit den Entities
    std::shared_ptr<const KollisionsKarte> kollision;   // ContentsOutside; ohne gilt alles als draussen
    double ms = 0.0;            // Zeit der Teilchen
};
int wetterTeilchenFuer(const WetterQuelle& q, bool editor, std::vector<bhed::gpu::KlingenQuad>& aus, const Camera& cam);
int wetterWolkenMitBild(const std::string& bild);   // rc604 fix604b (B6), Selbsttest
int flareSchilde(std::vector<bhed::gpu::KlingenQuad>& aus, const Camera& cam, float aspekt);
// rc604 surfaceSprites: regnet es (R_IsRaining, tr_WorldEffects.cpp:2360 -
// irgendeine Teilchenwolke) und der Wind der Wetterzonen.
bool wetterFuerGras(float wind[3]);
// rc608 Teil 13c: dasselbe fuer eine Quelle (editor: der Satz des Karten-Editors).
bool wetterFuerGrasIn(const WetterQuelle& q, bool editor, float wind[3]);
// --- surfaceSprites (gui/app_gras.cpp, rc604) ----------------------------------
// rc608 Teil 13c: wohin grasStufeLaden die Halm-Stufen legt - leer: die Vorschau (g_app->grasStufen/grasStand). Die
// Bilder gehen in den Satz, in den loadMapTextures gerade laedt.
struct GrasStufenZiel {
    std::vector<std::vector<App::GrasStufe>>* stufen = nullptr;
    int* stand = nullptr;
};
void grasStufeLaden(const ShaderInfo& info, const ShaderInfo::Stage& st, std::size_t stufenNr, const std::string& name,
                    std::size_t shaderNr, const GrasStufenZiel& ziel);
// Der Satz, in den loadMapTextures gerade laedt (die Vorschau oder das Ziel von loadMapTexturesFuer).
TextureSet& ladeSatz();
// rc608 Teil 13c: woraus die Halme eines Bildes entstehen - dieselbe Rechnung (RB_DrawSurfaceSprites) fuer die Vorschau
// und das Spielbild des Karten-Editors (editor: eigener Zwischenspeicher der Flaechen).
struct GrasQuelle {
    const BspMesh* welt = nullptr;
    const BspGeometry* geo = nullptr;
    const std::vector<std::vector<App::GrasStufe>>* stufen = nullptr;
    int stand = 0;
    const std::map<int, BspMesh>* brushNetze = nullptr;
    const std::vector<MoverDraw>* brushStand = nullptr;   // die Brush-Modelle dieses Bildes (brush > 0, entity < 0)
    double ms = 0.0;
    bool regnet = false;
    float wind[3]{};
    bool editor = false;
};
// Die Halme eines Bildes in `netz` (netzId 3); Rueckgabe: Zahl der Vierecke.
int grasNetzFuer(const GrasQuelle& q, const Camera& cam, float reichweite, BspMesh& netz);
// rc605: movers - die Stellungen dieses Bildes (Brush-Modelle tragen auch Halme).
// rc605: movers - die Stellungen dieses Bildes (Brush-Modelle tragen auch Halme).
void baueGrasNetz(const Camera& cam, float reichweite, const std::vector<MoverDraw>* movers = nullptr);
void baueGrasNetzNeben(const Camera& cam, float reichweite);
// rc605 (Rest) Selbsttest: die Spur von laserTrapThink einer misc_trip_mine zur
// Zeit ms (gui/app_view3d.cpp); neuBauen liest die Hindernisse neu.
bool fallenStrahlProbe(std::size_t entity, double ms, float ende[3], float normale[3], bool neuBauen);
int fallenFigurProbe(std::size_t ai, double ms, const float start[3], const float ende[3], bool mitAnbau);
// Selbsttest: Mitte der groessten Grasflaeche (vertical/flattened, mit alleArten auch effect).
bool grasTestZiel(float mitte[3], bool alleArten = false, bool nurPatch = false);
// je(i) fuer i in [0, n), auf die Kerne verteilt, sobald es sich lohnt (rc594).
// Fuer Strahltests (traceRay ist fadenfest); je(i) darf nur Platz i schreiben.
void spurenVerteilt(std::size_t n, const std::function<void(std::size_t)>& je);
void drawNeueMission();
void drawMissionSpeichern();
void entityNeuMenue(bool amMauspunkt);
void entityZiehen(float rasterSkal);
// Kaesten, Blickrichtung, Gizmo und Namen ueber dem Bild (nach entityZiehen).
void entityUeberlagerung(float rasterSkal);
// Welche Entities das aktive Skript benutzt (App::entImSkript) und das Schild
// mit dem Namen des aktiven Reiters oben links in der Ansicht (rc597).
void skriptFokusAuffrischen();
void skriptSchild();
bool entityGizmoPunkt(int griff, ImVec2& out);
int neueEntityAn(const std::string& klasse, const float ort[3]);
int neueEntityHier(const std::string& klasse, bool amMauspunkt);
bool entityLoeschen(int i);   // ohne Frage: Verweise auf Wegpunkte gehen mit (rc609 W1)
// rc609 W1: loeschen samt Verweisen; verweise -1 fragen, 0 nur Wegpunkte, 1 alle.
bool entitiesLoeschen(std::vector<int> a, int verweise);
int entityDuplizieren(int i);
bool entityAufBoden(int i);
bool entityKeySetzen(int i, const std::string& key, const std::string& wert);
// Mehrfachauswahl (rc590)
std::vector<int> entityAuswahl();          // Hauptpunkt zuerst, dann die weiteren
bool entityIstGewaehlt(int i);
void entityAuswahlUmschalten(int i);       // Strg+Klick
void entityAuswahlSetzen(int i);           // nur diese (-1: keine)
bool entityAuswahlLoeschen();
int entityAuswahlDuplizieren();
void platzierLeiste();                     // Kacheln in der Ansicht (app_view3d.cpp)
void platzierMaus();                       // Klick/Loslassen in der Ansicht setzt die gewaehlte Kachel
void ansichtsMarkenTasten();
int entityPlatzieren(const std::string& klasse, float sx, float sy);   // am Bildschirmpunkt (app_mission.cpp)
// --- rc609 M0: vorhandene Modelle in die Karte (gui/app_modellsetzen.cpp) ---------------
// Das Spiel der Vorschau fuer die Klassenwahl (kRegelwerkOpenJK/MovieDuels/JediOutcast).
int modellSpiel();
// Die .md3 der Spielordner (neu gesammelt, wenn sich die Ordner aenderten) und die Treffer der Suche.
const std::vector<modellsetzen::ModellDatei>& modellBrowserListe();
std::vector<std::size_t> modellBrowserTreffer(const std::string& suche);
// Wohin ein Modell am Bildschirmpunkt kaeme: auf die Flaeche unter der Maus (an einer Wand davor), die Unterkante
// des Modells (R_ModelBounds) auf dem Boden der Spielkollision. false: kein Treffer oder Modell nicht lesbar.
bool modellOrtAn(const std::string& pfad, float sx, float sy, const modellsetzen::Lage& lage, float ort[3]);
// Setzt es als misc_model_static (ein Rueckgaengig-Schritt, gewaehlt). -1: das Spiel kennt die Klasse nicht oder
// kein Ort (Grund in der Statuszeile).
int modellSetzenAn(const std::string& pfad, float sx, float sy);
// Die Lage einer Entity, wie die Vorschau sie liest, und das Schreiben (genau ein Winkel- und ein Groessenschluessel).
modellsetzen::Lage modellLageVon(const MapEntity& e);
void modellLageSetzen(MapEntity& e, const modellsetzen::Lage& l);
bool istKartenModell(const MapEntity& e);   // misc_model* mit "model"
void drawModellFenster();
void modellPlatzierMaus();
// Live: je Sekunde den Stand der Modelldateien der Karte pruefen; neuere laden (alle Exemplare behalten Lage).
void modellLiveWache();
// Die Fassung einer Modelldatei (0 = nie neu geladen) - Teil des Schluessels ihres Bildblocks.
int modellFassungVon(const std::string& pfadKlein);
// Selbsttest: Bildschirmort eines Gizmo-Griffs der gezeigten Entity (app_mission.cpp; 1..3, 4, 7..10).
bool entityGizmoGriffOrt(int griff, ImVec2& out);
bool entityRueckgaengig();
// Was auf Entity-NUMMERN zeigt (Auswahl, Animationsfenster, Wegkette) nach
// einem Kartenwechsel vergessen bzw. nach dem Loeschen von Nummer i
// nachziehen (Code-Pruefung rc592).
void entityNummernVergessen();
void entityNummernNachLoeschen(int i);
// Nach einem Umbau der Liste (Rueckgaengig, Erzeugtes entfernen): Nummern,
// die nicht mehr passen, vergessen.
void entityNummernPruefen();
bool entityWiederholen();
bool entityUndoMoeglich();
bool entityRedoMoeglich();
bool neueMissionAnlegen(const std::string& karte, const std::string& name, const std::string& ordner,
                        std::string* fehler);
void missionAusGeladener(const Mission& m);
bool missionSpeichernNach(const std::string& pk3, std::string* fehler);
std::string missionStandardOrdner(const std::string& name);
bool missionBeendenFrage();
// Script Flow: Classic / Graph / Spuren (gui/app_ansichten.cpp)
// Die Ansicht eines Feldes (0 Classic, 1 Graph, 2 Spuren). Feld 0 merkt sie
// sich in den Einstellungen.
int& feldAnsicht(int pane);
void ansichtsUmschalter(int pane = 0);
void drawFlussAnsicht(float breite, float hoehe, float kopfH, int pane = 0);
void drawGraphAnsicht();
void drawSpurenAnsicht();
void graphEinpassen(float breite, float hoehe);
std::string graphVerbinden(const std::string& vonSchluessel, const std::string& nachSchluessel, const std::string& art);
bool kantenGriff(const Path& pfad, double startMs, double endMs, const ImVec2& a, const ImVec2& b, double msJeBildpunkt);
// Missionsordner (mission.txt), Startflaeche, Einblendungen (rc586)
void missionsDateiSchreiben();
std::vector<std::pair<std::string, std::string>> letzteMissionen();
bool missionAusOrdnerLaden(const std::string& ordner, std::string* fehler);
// Zuletzt geoeffnete Missionen (rc599): die gemerkten (Ordner und Archive)
// und die Missionsordner, ohne Dubletten - fuer Menue und Startflaeche.
struct LetzteMission {
    std::string eintrag;   // wie in Settings::recentMissions
    std::string name;      // "md_am_sith"
    std::string ort;       // Ordner oder Archivname, zum Anzeigen
};
std::vector<LetzteMission> letzteMissionenAlle();
void letzteMissionOeffnen(const std::string& eintrag);
void missionsOrdnerWaehlen();
void drawStartflaeche(float breite, float hoehe);
// rc608: die Bauteile der Startflaeche; der Menue-Editor baut seinen Startbildschirm ebenfalls daraus.
struct StartSpalte {
    float links = 0.0F;    // Fensterkoordinaten, fuer SetCursorPosX und PushTextWrapPos
    float breite = 0.0F;
};
struct StartKarte {
    const char* titel;
    const char* unter;
    UiIcon icon;
    const char* hinweis;   // Tooltip oder nullptr
};
struct StartKnopf {
    const char* text;
    UiIcon icon;
    const char* hinweis;
};
// Beginnt das Kindfenster (zu mit ImGui::EndChild) und zeichnet Logo, Titel und Untertitel.
StartSpalte startKopf(const char* fensterId, ImVec2 groesse, const char* titel, const char* unter);
// Die Karten nebeneinander, im schmalen Fenster untereinander. Rueckgabe: Index der geklickten, sonst -1.
int startKarten(const StartSpalte& sp, const StartKarte* karten, int anzahl);
// "Neu hier?" und die Knoepfe dazu; was nicht mehr in die Zeile passt, bricht um. Rueckgabe wie oben.
int startKnopfZeile(const StartSpalte& sp, const char* titel, const StartKnopf* knoepfe, int anzahl);
// Die Hoehe, die eine Karte fuer ihre Texte braucht (bei dieser Breite)
float startKarteHoehe(const char* titel, const char* unter, float breite);
void meldung(const std::string& text, const std::string& ordner, bool warnung = false);

// --- rc609 W2b: Entity-Texteditor (gui/app_enttext.cpp) ---------------------------
// F13: eine Eingabe (UTF-8) als Latin-1-Bytes fuer die Entities; false und eine Meldung,
// wenn ein Zeichen nicht in Latin-1 liegt (nichts wird still ersetzt).
bool eingabeNachLatin1(const std::string& utf8, std::string& out);
// Der Text, aus dem die Entities einer Karte gelesen wurden (Entity-Lump, .ent):
// Kommentare, Leerzeilen und Reihenfolge bleiben darin erhalten.
void entTextQuelle(const std::string& karte, const std::string& text);
// Der Text der aktuellen Karte, abgeglichen mit map.entities - so schreibt "Entities speichern".
const std::string& entTextFuerSpeichern();
bool entTextUebernehmen();          // Strg+Enter: lesen wie das Spiel, ein Rueckgaengig-Schritt
void entTextAusblenden();           // H: die gewaehlten Entities in 3D ausblenden
void entTextAlleEinblenden();       // Alt+H
bool entTextHatFokus();             // Panel hat den Fokus: F3/Strg+F/H gehoeren ihm
void entTextMittelklick();          // Mittelklick in 3D: "x y z" an der Schreibmarke (BSP-Entity-Edit)
bool entTextKoordinatenEinfuegen(const float p[3]);
int entTextMarke();                 // Selbsttest: Schreibmarke (Byte im Puffer), -1 = keine
void drawEntTextInhalt();
void drawEntTextFenster();
// fuer den Selbsttest
const std::string& entTextPuffer();
const std::string& entTextDoc();
bool entTextGeaendert();
int entTextFehlerZeile();
const std::string& entTextMeldung();
int entTextTrefferNr();
int entTextTrefferZahl();
bool entTextHervorhebung();
void entTextHinterlegt(int& von, int& bis);
int entTextBlockZeile(int entity);
ImVec2 entTextZeilenOrt(int zeile, int spalte);
int entTextErsteSichtbareZeile();
ImVec2 entTextKopfOrt();
void entTextMeldungOrt(ImVec2& a, ImVec2& b);
// --- rc609 J: Shader, Nebel und Flaechen fertiger Karten (gui/app_lumpedit.cpp) ---------------
// Beim Laden (loadMapFromBytes): die Bytes, aus denen die Vorschau liest - mit den bearbeiteten
// Lumps, wenn diese Karte welche hat (Reiterwechsel), sonst bytes.
const std::string& lumpEditKarteGeladen(const std::string& karte, const std::string& bytes);
void lumpEditVergessen(const std::string& karte);           // frisch von der Platte (F3)
bool lumpEditUngespeichert(const std::string& karte);
bool lumpEditBytes(const std::string& karte, std::string& out);   // nur mit ungespeicherten Aenderungen
bool lumpEditHatFokus();
bool lumpEditRueckgaengig();
bool lumpEditWiederholen();
void drawLumpEditFenster();
void lumpEditMaus();             // Klick in die Vorschau
void lumpEditUeberlagerung();    // Hervorhebung in der Vorschau
// Karten-Editor: die geoeffnete .bsp (leer: keine), Klick in die Kamera, Spielbild, Hervorhebung
void lumpEditKarteSetzen(const std::string& quelle, const std::string& bytes);
bool lumpEditKarteWahl();
bool lumpEditKarteKlick(const float start[3], const float ende[3]);
bool lumpEditKarteSpielbild(std::shared_ptr<const std::string>& daten, std::uint64_t& nummer);
void lumpEditKarteUeberlagerung(const std::function<bool(const float*, ImVec2&)>& abbild);
// Speichern auf den Wegen aus W2a (app_dateiort.cpp); gespeichert() nach Erfolg.
bool bspLumpsSpeichern(const karte::BspLumps& lumps, const std::function<void()>& gespeichert);
// app_ablage.inc: Geometrie aus Bytes (wie beim Laden) und die daraus aufgestellte Szene vergessen
bool karteGeometrieLesen(const std::string& bytes, const std::string& shownPath);
void kartenSzeneVergessen();
void kartenGeometrieVergessen();
// fuer den Selbsttest
int lumpEditFlaeche();
void lumpEditFlaecheSetzen(int i);
bool lumpEditTauschen(const std::string& name, bool alle);
bool lumpEditFlagsSetzen(int shader, std::uint32_t surfaceFlags, std::uint32_t contentFlags);
const karte::BspLumps* lumpEditLumps();
const std::string& lumpEditDateiBytes();
const std::string& lumpEditPuffer();
void lumpEditPufferSetzen(const std::string& utf8);
bool lumpEditTextUebernehmen();
int lumpEditFehlerZeile();
const std::string& lumpEditMeldung();
const std::string& lumpEditWarnung();
const std::string& lumpEditNutzungText();
int lumpEditHervorShader();
std::size_t lumpEditRueckgaengigZahl();
bool lumpEditIstUngespeichert();
void lumpEditSpeichern();
void lumpEditReiter(int r);
void lumpEditWaehlenSetzen(bool an);
ImVec2 lumpEditZeilenOrt(int zeile);
ImVec2 lumpEditKnopfOrt(const std::string& name);
std::string lumpEditText();
int lumpEditLightmapRegel(int lightmap, const std::string& shader);
// Die ganze Entity-Liste ersetzen (app_mission.cpp): ein Rueckgaengig-Schritt;
// neuerSchritt=false haengt an den letzten an (Live-Tippen im Entity-Text).
bool entitiesErsetzen(std::vector<MapEntity> neu, bool neuerSchritt);
void drawEinblendungen();
// Gibt es an der Mission Ungespeichertes (Entities oder Skripte)?
bool missionUngespeichert();
// Kopfzeile der Kartenansicht (gui/app_view3d.cpp)
void kartenNetzNeu();
void ansichtEinstellungen();
float ansichtsSchalter(bool nurMessen);
void drawKartenStatistik();
bool entityFokus();
int reiterFuerSkriptRef(const std::string& ref);
// Die Zeilen fuer eine Nutzlast "bhed_new" aus der Ereignisliste (rc597, fuer
// den Graphen); leer, wenn sie nicht passt.
std::vector<Node> knotenAusNeuNutzlast(const void* daten, int groesse);
std::string aktiverSkriptRef();
bool skriptTextLaden(const std::string& ref, std::string& text);
void skriptOeffnen(const std::string& ref, bool anlegen);
// Einen Reiter speichern, auf Wunsch unter einem neuen Pfad (ohne Dialog).
bool reiterSpeichernUnter(int reiter, const std::string& pfad);
void selectByPath(const Path& p);
// Graph und Spuren (rc601): das Rechtsklick-Menue einer Zeile des offenen
// Skripts (true = geaendert; `editor` = den Editor danach oeffnen) und das
// Ablegen eines Befehls aus der Ereignisliste vor/hinter eine Zeile.
bool befehlsMenue(const Path& p, bool& editor);
bool neuAblegenAn(const ImGuiPayload* pl, const Path& p, bool davor);
void drawMapSidebar();
void drawMapView();
void drawModelToolbar();
void drawModelPanel();
void drawModelView();
void drawTimeline();
void drawCameraGizmos();
// dock2: Schalter des durchgehenden DockSpace (dock4: ab Werk an, BHED_DOCKSPACE=0 = Rueckweg).
bool& dockraumAn();
// Steht der Bereich (0..2) in der Standardanordnung? Zuruecksetzen baut sie im
// naechsten Bild neu auf; der Blattknoten eines Panels der Standardanordnung.
bool dockraumStandard(int bereich);
void dockraumZuruecksetzen(int bereich);
ImGuiID dockraumHeim(const char* panel);
// Menue "Fenster" im DockSpace: steht das Panel ausserhalb des Hauptfensters,
// in ein eigenes Fenster nehmen, zurueck an seinen Platz der Standardanordnung.
bool dockraumDraussen(const char* panel);
void dockraumHerausnehmen(const char* panel);
void dockraumAndocken(const char* panel);
// dock3: nach dem Laden einer Anordnung den Baum des Bereichs neu lesen.
void dockraumNeuLesen(int bereich);

// Kartendaten laden - aus einer Datei oder aus einem Archiv.
void loadMap(const std::string& path);
void loadMapFromBytes(const std::string& bytes, const std::string& shownPath);
// Darf beendet werden? Prueft ALLE Reiter, nicht nur den aktiven.
bool confirmQuit();
// Die Gizmo-Vorschau ins Skript schreiben - ueber replaceAt, also mit
// Rueckgaengig.
bool gizmoWriteBack();
// Die Drehung ins Skript schreiben - sie gehoert zum PAN, nicht zum MOVE.
bool gizmoWriteAngles();
// Vor dem Wegwerfen fragen und DANACH weitermachen - siehe app.cpp.
void withUnsaved(std::function<void()> then,
                 std::function<void()> onCancel = nullptr);
void drawAskSave();
void loadMapTextures();
// rc608 Teil 11: dieselben Regeln fuer eine andere Karte (die gebaute Karte des Karten-Editors).
// rc608 Teil 13c: mit `gras` auch die Halm-Stufen (surfaceSprites) in dieses Ziel.
void loadMapTexturesFuer(const BspGeometry& geo, TextureSet& ziel, std::unordered_set<std::string>& bilder,
                         const std::function<void()>& danach = {},   // rc608 Teil 13b: danach, solange das Ziel gilt
                         const GrasStufenZiel& gras = {});
// Die Textur zu EINEM Shadernamen. Auch die .md3-Modelle gehen diesen Weg -
// zwei getrennte waeren zwei Gelegenheiten, verschieden falsch zu liegen.
bool loadTextureFor(const std::string& shaderName, TextureSet::Tex& out);
// rc608 Teil 9b: ein Bild ohne Shadersuche (map, animMap, Himmelsbox: R_FindImageFile).
bool loadImageFile(const std::string& bild, TextureSet::Tex& out);
// rc608 Teil 12: Bilder im Voraus nebenlaeufig lesen, entziffern und verkleinern (gui/app_view3d.cpp).
// bildVorratFuellen nimmt Auftraege (je eine Kandidatenliste; die erste lesbare Datei zaehlt, Dateien mit Eintrag in
// texCache gelten als erledigt) und legt die Ergebnisse nach DATEINAMEN in einen Vorrat. bildLesen(datei) ist genau
// readFromArchives + image::decode + Verkleinern auf bildGrenzeFuer - aus dem Vorrat, wenn die Datei dort liegt.
// bildVorratLeeren verwirft, was niemand abgeholt hat. Das Ergebnis haengt nie am Vorrat, nur die Zeit.
struct BildVorratStand {
    std::size_t neu = 0;
    std::size_t faeden = 0;
};
BildVorratStand bildVorratFuellen(const std::vector<std::vector<std::string>>& auftraege);
[[nodiscard]] bool bildLesen(const std::string& datei, TextureSet::Tex& t);
void bildVorratLeeren();
void bildVorratAbschalten(bool aus);   // Selbsttest leistung: dieselben Bilder ohne Vorrat (Gegenprobe)
// rc608 Teil 10: Nummer der Bildstufe hinter einer ersetzenden Lichtstufe (GL_ONE GL_ZERO), sonst -1 (gui/app_ablage.inc).
int lichtErsetztBildstufen(const ShaderInfo& info);
// rc604 fix604b (B1): eine Zusatzstufe laden (gui/app.cpp, aus loadMapTextures).
void zusatzstufeLaden(const ShaderInfo& info, const ShaderInfo::Stage& st, const std::string& shaderName,
                      TextureSet::Tex& t);
// rc604 fix604b (B1): animMap der Basisstufe laden (gui/app.cpp).
void basisBildfolgeLaden(const ShaderInfo& info, int eigenerIndex, const std::string& shaderName,
                         TextureSet::Tex& t);
// rc604 fix604b (B1): Farbe/Alpha einer weiteren Effektstufe (gui/app_view3d.cpp).
void effektStufeFarbe(const ShaderInfo::Stage& st, TextureSet::Tex& z);
// Was die Kartenleiste als Namen zeigt: die Mission, wenn ihre Karte vorn
// liegt, sonst den Namen der Karte (rc592) - siehe drawMapToolbar.
std::string leistenTitel();
// Sagt im Detailprotokoll, WO eine fehlende Datei stattdessen liegt - oder
// dass ihr Name in keinem Archiv vorkommt. Ohne das heisst "nicht
// gefunden" nur "such weiter, irgendwo".
void reportMissingTexture(const std::string& name);
// In app_view3d.cpp definiert.
//
// WICHTIG: diese Deklarationen gehoeren HIERHER, nicht in den anonymen
// Namensraum von app.cpp. Ein anonymer Namensraum ist je Uebersetzungseinheit
// ein eigener - eine Deklaration darin verspricht eine Definition in
// DERSELBEN Datei. Steht sie woanders, meldet der Uebersetzer eine
// deklarierte, aber nie definierte Funktion, und das ist genau richtig so.
struct ComboEditResult {
    bool edited = false;
    bool picked = false;
};
ComboEditResult comboEdit(const char* id, std::string& value,
                          const std::vector<TypeEntry>* entries, float width);
void loadModelTextures();
void applySkinByIndex(int index);
void loadActorModels();
void openScriptFromMemory(const std::string& data, const std::string& shownName, const DateiHerkunft& herkunft = {});
// --- rc609 W2a: Dateien an Ort und Stelle (gui/app_dateiort.cpp) ---------------
// Eine Karte bzw. ein Skript aus einer bestimmten .pk3 oeffnen (pk3-Browser,
// Selbsttest) - die Herkunft merkt sich Archiv und Eintrag.
bool karteAusArchivOeffnen(const std::string& pk3, const std::string& eintrag);
// Beim Reiterwechsel: dieselbe Karte aus ihrer .pk3, ohne die gemerkten Entities zu verwerfen.
bool karteAusPk3Nachladen(const std::string& pk3, const std::string& eintrag);
bool skriptAusArchivOeffnen(const std::string& pk3, const std::string& eintrag);
// Punkt 2: getrennte Befehle. Ergebnis: true, wenn geschrieben (oder eine Frage
// nach dem Weg gestellt wurde - dann false und speicherFragen nicht leer).
bool entitiesSpeichern();
bool entitiesInBspUebernehmen();
// Strg+S fuer ein Skript ohne Pfad, das aus einer .pk3 kam.
bool skriptAusPk3Speichern();
// "Save mission" im neuen Ablauf: jede Datei der Mission an ihren Ort, ohne .pk3.
bool missionAnOrtSpeichern();
// Punkt 3: Weg fuer eine Datei aus einer .pk3 - gemerkt/Einstellung oder Frage.
void speicherWegAnbieten(App::SpeicherAuftrag auftrag);
void drawSpeicherWeg();
// Schreibt Bytes auf einem der Wege (1 in die .pk3, 2 lose in den Mod-Ordner,
// 3 Speichern unter). Sicherung .bak fuer .bsp und .pk3. neu: die Herkunft danach.
bool aufWegSchreiben(int weg, const DateiHerkunft& h, const std::string& bytes, DateiHerkunft* neu);
// Datei sicher ersetzen (F5) mit Meldung im Programm bei Fehler.
bool dateiSicherSchreiben(const std::string& pfad, const std::string& bytes);
// rc609 J: vorher <pfad>.bak (wie fuer jede .bsp), dann sicher ersetzen.
bool bspSicherungUndSchreiben(const std::string& pfad, const std::string& bytes);
// F3/F4 ohne Dialog (Selbsttest, doLoadMap, openEntFile).
void karteDateiLaden(const std::string& p);
bool entDateiLaden(const std::string& p);
// Ein Modell aus den Archiven ins Modellfenster laden (pk3-Browser, Selbsttest).
bool oeffneModellAusArchiv(const std::string& name, std::string* fehler);
// Bytes eines Eintrags aus einer bestimmten .pk3 (W2a; rc609 B2).
bool ausPk3Lesen(const std::string& pk3, const std::string& eintrag, std::string& out);
// Verlauf der Entities leeren (W2a F3: Karte frisch von der Platte; rc609 B2: nach dem Nachladen).
void entVerlaufLeeren();
// --- rc609 B2: Mission <-> Map ohne Verlust (gui/app_kartemission.cpp) ---------
struct KarteMissionStand {
    std::vector<KarteMissionBefund> befunde;      // des letzten Nachladens
    std::array<int, 5> zusammenfassung{};         // unveraendert, geaendert, eigene angehaengt, eigene schon da, entfernt
    bool fensterOffen = false;                    // die Liste der Bezuege
    int nachgeladen = 0;                          // wie oft nachgeladen
};
[[nodiscard]] const KarteMissionStand& karteMissionStand();
// Map-Tab, File > "Karte der Mission bearbeiten": Quelle oder .bsp der Mission oeffnen
// und mit ihr verbinden - der Bau schreibt in ihre Datei, der Mission-Tab laedt nach.
bool karteDerMissionBearbeiten();
[[nodiscard]] bool karteDerMissionMoeglich();
[[nodiscard]] std::string karteDerMissionName();
// Aus dem Bau des Map-Tabs (app_karte_bau.cpp): Beginn (gebaute .bsp) und Ende (ok: alle Schritte 0).
void karteMissionBauBeginnt(const std::string& karteDatei, const std::string& bsp);
void karteMissionBauFertig(const std::string& karteDatei, bool ok);
void drawKarteMission();
// Selbsttest (BHED_EDITORTEST): Ruhe des Ereigniseditors und Klickpruefung
// der Knoepfe. Einmal je Bild am Ende von draw(); ohne die Variable leer.
void selbsttest();
// Ja/Nein-Frage stellen; `ja`/`nein` laufen nach dem Schliessen.
void frage(const std::string& text, std::function<void()> ja,
           std::function<void()> nein = {});
void drawFrage();
// Welcher Knopf neben einem Feld des Ereigniseditors steht - wie im
// Original (Abgleich 27.09., jeder Befehl geoeffnet):
//   Keiner  die fuehrende Klappliste (SET_TYPES, CHANNELS, PLAY_TYPES,
//           DECLARE_TYPE, CAMERA_COMMANDS) - sie ist die Auswahl selbst;
//   Helfer  jedes Feld HINTER einer fuehrenden Klappliste: "Helper", nur
//           die Helferzeilen seines Typs (str: Get, Zahl: Get + Rnd,
//           vec: Get + Tag), zurueck mit "Revert";
//   Expr    alle Felder der uebrigen Befehle, auch Klapplisten wie
//           FLUSH/INSERT bei affect: "Expr!", volle Helferzeilen.
enum class FeldKnopf : std::uint8_t { Keiner, Expr, Helfer };
FeldKnopf feldKnopf(const Command& c, std::size_t feld);
// Ist die Klappliste eine Auswahl, deren Eintraege eigene Felder mitbringen
// (SET_TYPES: SET_HEALTH will <int>, SET_ORIGIN <vec>)?
bool auswahlMitFeldern(const CommandDb& db, const std::string& typeset);
// Verschobenes wiederfinden: Kennungen vor der Bewegung merken, danach die
// Knoten mit diesen Kennungen markieren (und ihre Bloecke aufklappen).
std::vector<Kennung> kennungenVon(const std::vector<Path>& wege);

// Ein schwebendes Fenster zeigen UND nach vorn holen.
//
// `id` ist die feste Kennung aus dem Titel ("###find"). Nur den Merker zu
// setzen reicht nicht: stand das Fenster schon offen, aber verdeckt, blieb
// es verdeckt - gemeldet als "I am unable to open it again".
void fensterZeigen(bool& offen, const char* id);

// Kamera, Zeitleiste, Szene, Mover und Effekte aus dem aktiven Skript neu
// aufbauen (app_view3d.cpp).
void prepareScene();

// --- Paket bewegung (rc601): Schwerkraft der Figuren (app_bewegung.cpp) ---
// Rechnet Actor::hoehe fuer die fertige Szene ein (scene.h,
// schwerkraftEinrechnen) gegen die Welt der Karte und die Brush-Modelle an
// ihrem jeweiligen Platz. mitMovern: die Mover-Simulation ist gebaut und
// traegt mit (Aufzuege, Tueren) - nur noetig, wenn sich ein Mover mit
// tragender Flaeche bewegt (schwerkraftMitMovernNoetig).
void schwerkraftVorschau(bool mitMovern);
// rc604 fix604: Raster und Speicher der Schwerkraft mit Movern (Vorgabe an;
// aus: der alte Weg, fuer den Vergleich im Selbsttest, BHED_SCHWER_VERGLEICH).
extern std::atomic<bool> gSchwerkraftSchnell;
// rc601 (bewegung, Wege wie im Spiel): die Brushes der Karte fuer die Spuren
// von NAV/STEER (beim Laden der .bsp), und daraus die Welt fuer planeLauf -
// Wegnetz wie im Spiel, Spuren, Sichttabelle, Boden. Zwischengespeichert,
// bis sich Karte oder Entities aendern.
void kollisionLaden(const std::string& bsp, const std::string& karte);
// rc601 integ: dieselbe Kollisionskarte fuer andere Teile (die Lichtschwerter,
// Paket saber) - nullptr, wenn fuer `karte` keine geladen ist.
[[nodiscard]] std::shared_ptr<const KollisionsKarte> geladeneKollision(const std::string& karte);
[[nodiscard]] std::shared_ptr<const NavWelt> navWeltVorschau();
// rc609 W4: dieselbe Welt ohne Mover-Plan - die Karte, wie sie nach dem Netzbau beim Laden steht (Kampfpunkte, Testroute).
[[nodiscard]] std::shared_ptr<const NavWelt> navWeltSpawn();
// rc609 H2: dieselbe Welt fuer andere Entities der geladenen Karte (ohne Zwischenspeicher); nullptr ohne Geometrie.
[[nodiscard]] std::shared_ptr<const NavWelt> navWeltFuer(const std::vector<MapEntity>& ents);
// rc601 (bewegung, teil 3): den Stand der Mover aus der Mover-Simulation
// (g_app->moverSim, bis endeMs) fuer die Wege uebernehmen (NavWelt::mover,
// spurZeit). true: er hat sich geaendert - Ablauf und Szene noch einmal.
bool navMoverPlanAusSim(double endeMs);
// Pruefung 4b: die Welt fuer den Spurtest der Effektteilchen (setzeEffektSpurWelt,
// bhed/efxdraw.h) aus Kollisionskarte, Mover-Stand und Figuren der Szene.
void effektSpurWeltSetzen();
// Pruefung 4b: misc_model_breakable mit "gravity" - auf den Boden gesetzt und an
// den Hang gelegt (objektAbsetzen), die Wuerfe der MoverSim als Bahn am
// Kartenmodell (App::MapModel::wurf).
void objektWuerfeSetzen();
[[nodiscard]] bool schwerkraftMitMovernNoetig();
// rc601 (bewegung, Durchsicht 2): die Schwierigkeit der Vorschau (g_spskill
// 0-3, Einstellungen) setzen - die Szene wird danach neu aufgebaut, denn
// yawSpeed und Tempo mancher NPCs haengen daran (bhed/scene.h,
// npcWerteBeimSpawnen).
void schwierigkeitWaehlen(int n);
// rc601 integ: der zentrale Schalter OpenJK / Movie Duels (app_regelwerk.cpp,
// bhed/regelwerk.h). regelwerkPruefen jedes Bild: wendet Einstellung und
// Erkennung an und baut die Szene neu, wenn sich etwas aenderte (true: neu
// angewandt). schwierigkeitJetzt: g_spskill, wie es gilt (-1 = Vorgabe des
// Regelwerks aufgeloest).
[[nodiscard]] bool regelwerkOrdnerErkannt();
bool regelwerkPruefen();
// rc604 (Jedi Outcast): das Spiel, wie es gerade gilt (regelwerkSpiel nach
// Einstellung, Spielordnern und Sitzung); ist ein Ordner JK2 (jk2sp.exe bzw.
// eine JK2-Karte, zwischengespeichert); die eingerichteten Spielordner des
// Spiels nach den Einstellungen (gamePathsJk2 bei Jedi Outcast, sonst
// gamePaths); das Regelwerk sofort anwenden (rescanGamePaths, vor dem
// Einlesen); den Befehlssatz des Regelwerks laden (data/jk2 bzw. data/base).
[[nodiscard]] int regelwerkSpielErmitteln();
[[nodiscard]] bool jediOutcastOrdner(const std::string& verzeichnis);
[[nodiscard]] const std::vector<std::string>& eingerichteteSpielordner();
// Die Bilder der Vorschau wie die Engine: r_picmip halbiert jedes Bild n-mal (Upload32, Vorgabe 0 in OpenJK und
// Movie Duels, Einstellung bildPicmip), darueber nur die Grenze der Grafikkarte (glConfig.maxTextureSize).
inline constexpr int kBildGrenze = 4096;
// Fuer image::shrinkTo: die Kantenlaenge, die genau um 2^bildPicmip verkleinert.
[[nodiscard]] int bildGrenzeFuer(const image::Image& im);
// Fuer die Schluessel in texCache: dasselbe Bild in einer anderen Stufe ist ein anderes.
[[nodiscard]] std::string bildStufeText();
void regelwerkAnwendenJetzt();
void areaPortaleJetzt(const float auge[3], const float* vorn = nullptr);
// CM_SnapPVS(ort): die Gebietsmaske dieser Sicht in den Indexpuffer der Karte (nur wenn sie sich aendert).
// rc609 P: `fernOrt` - der Ort der Sicht fuer die ferne Ebene, wenn er nicht der des PVS ist (Spiegel und Portal:
// newParms.ori.origin gegen pvsOrigin).
void areaSichtSetzen(const float ort[3], const float* vorn = nullptr, const float* fernOrt = nullptr);
// tr.distanceCull: R_LoadEntities, worldspawn "distanceCull", Vorgabe 12000.
[[nodiscard]] float kartenDistanceCull();
// rc608 Teil 11: fuer eine andere Karte (das Spielbild des Karten-Editors) - die Vorschau ruft sie mit g_app->geo
// (dazu nebelListeFuer in app_view3d.cpp).
[[nodiscard]] float kartenFernFuer(const BspGeometry& geo);
void befehlssatzFuerRegelwerk();
[[nodiscard]] int schwierigkeitJetzt();
[[nodiscard]] std::string regelwerkStatusZeile();

// Ersetzen aus dem Find-Fenster (siehe ersetzeInKnoten in edit.h).
// ersetzeEinen: steht die Auswahl auf einem Treffer, wird er ersetzt, dann
// geht es zum naechsten - wie "Replace" in Notepad++. ersetzeAlle: alle
// Treffer, je Dokument EIN Rueckgaengig-Schritt; alleDokumente nimmt die
// anderen Reiter mit. Rueckgabe: Zahl der ersetzten Felder.
int ersetzeEinen();
int ersetzeAlle(bool alleDokumente);

// Die wirkende Belegung einer Aktion (Vorgabe oder eigene) und ihr Name -
// fuer den Selbsttest; in app.cpp heissen sie chordFor/chordName.
ImGuiKeyChord kuerzelVon(keys::Action a);
const char* kuerzelName(keys::Action a);
// Feste Kennung eines Reiters (Parked::uid), -1 fuer eine ungueltige Nummer.
int tabKennung(int tab);
// Einen Reiter schliessen wie mit dem x am Reiter (fragt bei Aenderungen).
void reiterSchliessen(int index);
// Nur Schritt i des Undo-Stapels zuruecknehmen (0 = aeltester), die spaeteren
// bleiben. Ein neuer Schritt. `bericht`: was zurueckgenommen wurde.
bool einzelnenSchrittZuruecknehmen(std::size_t i, std::string* bericht);
// Aenderungsrand je Zeile: 0 nichts, 1 geaendert (orange), 2 geaendert und
// gespeichert (gruen), 3 nach dem Speichern wieder wie beim Oeffnen (blau).
enum : std::uint8_t { kMarkeKeine = 0, kMarkeGeaendert = 1, kMarkeGespeichert = 2, kMarkeZurueck = 3 };
// Der Rand des Skript-Editors (app_anordnung.inc) - auch im Code-Editor des
// Menue-Editors: Lesezeichenpunkt links, Aenderungsstreifen rechts.
// (Die beiden stehen dort im anonymen Namensraum; diese reichen sie durch.)
[[nodiscard]] float randBreiteSkript();
void randZeichnenSkript(ImDrawList* dl, float x0, float breite, float y, float hoehe, std::uint8_t marke, bool lesezeichen);
void aenderungsStandNeu();         // aktiver Reiter: Originalstand = jetzt
// "Revert to original": die inhaltlich geaenderten unter `wege`, mit ihrem Original.
std::vector<std::pair<Path, Node>> zuruecksetzbar(const std::vector<Path>& wege);
// Die Vorschau dazu: wie sah dieser Befehl beim Oeffnen aus? Leer, wenn es
// nichts zurueckzusetzen gibt. `neu` wird gesetzt, wenn der Befehl erst
// nach dem Oeffnen entstanden ist (dann gibt es kein Original).
std::string originalZeile(const Path& weg, bool* neu = nullptr);
// Die Vorschau als EINE Zeile: "wait ( 3000 )", bei mehreren "... (+2)",
// lange Zeilen gekuerzt. Leer, wenn nichts zurueckzusetzen ist.
std::string originalKurz(const std::vector<std::pair<Path, Node>>& zurueck);
// Zeichnet sie in einen offenen Hinweis: grau "Original:", daneben der Befehl.
void originalVorschau(const std::vector<std::pair<Path, Node>>& zurueck);
void aenderungenGespeichert();     // aktiver Reiter: gespeicherter Stand = jetzt
std::vector<std::uint8_t> zeilenMarken(int tab, const Script& s, const std::vector<Row>& rows);
bool hatLesezeichen(int tab, Kennung k);
void lesezeichenUmschalten(const std::vector<Path>& wege);
void lesezeichenSpringen(bool vor);
void lesezeichenAlleWeg();
void lesezeichenLaden();           // aktiver Reiter, aus den Einstellungen
void lesezeichenMerken();          // aktiver Reiter, in die Einstellungen
// Ein Befehlssymbol als Vektorzeichnung (gui/vektoricons.cpp). false, wenn
// es das Symbol nicht als Zeichnung gibt.
bool vektorIcon(ImDrawList* dl, const char* name, ImVec2 pos, float groesse, bool alt);
void markiereKennungen(const std::vector<Kennung>& ks);
// Zwischenablage wie im Original: intern UND als Text "//(BHVD)" + Skript
// in der Windows-Zwischenablage.
void ablageNachWindows();
void kopieren(bool ausschneiden);                   // die Auswahl
void kopiereWeg(const Path& weg, bool ausschneiden); // eine Zeile (Rechtsklick)
void einfuegen(const Path& hinter);
// Zwischen ImGui_ImplWin32_NewFrame und ImGui::NewFrame: meldet die
// simulierte Maus des Selbsttests als LETZTE Stelle. Sonst leer.
void selbsttestVorBild();
// Wie ein Klick in die Ereignisliste: Befehl hinter die Auswahl setzen.
void fuegeBefehlEin(const Command& c);
void neuerReiter();
void changelogZeigen();
// Fuer den Selbsttest (Modus "pruefung"): Laden, Uebersetzen, Save all und
// Save As mit festem Pfad - ohne Windows-Dialog.
void ladePfad(const std::string& p);
void uebersetzen();
void alleSpeichern();
bool speichernUnterPfad(const std::string& p);
void dateiVonAussen(const std::string& pfad);
void dateiAbgelegt(const std::string& pfad, float x, float y);   // Stufe 9 Punkt 3
void abgelegteDateienVerarbeiten();
// Wie "Open" nach dem Dateidialog.
void ladeSkriptDatei(const std::string& pfad);
void selbsttestMerkeElement(ImGuiContext* ctx, ImGuiID id, const ImRect& bb);
void selbsttestMerkeText(ImGuiContext* ctx, ImGuiID id, const char* label);

// Auf einen Reiter wechseln, von aussen.
//
// switchTab() selbst hat Voreinstellungen und bleibt in app.cpp; sie hier
// noch einmal hinzuschreiben waere eine zweite Wahrheit, und genau daran
// ist in dieser Sitzung schon mehr als einmal etwas auseinandergelaufen.
void activateTab(int index);
// Vor dem Wegwerfen fragen und DANACH weitermachen.
//
// Kein bool mehr: die Frage steht im eigenen Stil im Bild und dauert
// mehrere Bilder. Was danach geschehen soll, wird mitgegeben.
void withUnsaved(std::function<void()> then,
                 std::function<void()> onCancel);
int frameForAnimation(const std::string& name, double sinceMs, bool hold);
// Schreibt fuer jede Figur eine Zeile ins Detailprotokoll: was sie an der
// Zeitmarke gerade tut. Auf Zuruf ueber das Ansichtsmenue.
void dumpActorsAtPlayhead();
// Einen Klang entschluesselt und gepuffert holen, fuer die Mundanimation.
// Gibt nullptr, wenn er nicht gefunden oder nicht lesbar ist.
const sound::Sound* soundFor(const std::string& name);
// Ein Klang fertig fuer die Ausgabe (44,1 kHz Stereo), zwischengespeichert.
const std::vector<std::int16_t>* klangFuerWiedergabe(const std::string& name);
// Alle Klaenge der Zeitleiste vorladen - vor dem Start der Uhr.
void klaengeVorladen();
ModelTextures texturesFor(const GlmModel& model);
// Das Bild einer Figurstufe, nur als Datei gesucht wie R_FindImageFile (app_view3d.cpp).
std::shared_ptr<const TextureSet::Tex> ladeStufenBild(const std::string& bildName);
// Pruefung 4b: dasselbe Modell ganz mit EINEM Shader (refEntity.customShader,
// R_AddGhoulSurfaces) und der Farbe `farbe` fuer alle Stufen (RF_RGB_TINT:
// FigurFarbe::Entity). Leer, wenn der Shader fehlt.
ModelTextures texturenMitShader(const GlmModel& model, const std::string& shader, FigurFarbe farbe);
// Bilder und Farbe einer Lichtschwertklinge (app_view3d.cpp) - auch fuer die
// Saeber in Menues (Menue-Editor M2).
const TextureSet::Tex* bladeTexture(const std::string& farbe, bool glow);
void bladeColorFor(const std::string& name, std::uint8_t out[3]);
void insertCameraHere();
void loadSkeletonForModel();
void openModelFile();
void openSkinFile();
void openSkeletonFile();
void openEntFile();
void addGamePathDirectory(const std::string& dir);
// rc604 jk2: die NPC-Beschreibungen des Spiels nach dem Regelwerk lesen
// (Jedi Outcast: ext_data/NPCs.cfg). Rueckgabe: gelesene Dateien.
int npcDateienLesen(NpcMap& m);
// Stufe 7: dasselbe in zwei Teilen. npcLeseAuftrag (Hauptfaden) bestimmt die
// Dateien und je Datei die Archive in der Reihenfolge von readFromArchives;
// npcAuftragLesen liest und zerlegt sie - ohne g_app und ohne diag, also in
// jedem Faden. npcDateienLesen ist beides nacheinander (dasselbe Ergebnis).
struct NpcLeseAuftrag {
    struct Datei {
        std::string name;
        bool jk2 = false;   // parseNpcFile( ..., jk2 )
        std::vector<std::pair<std::string, Pk3Entry>> archive;   // Pfad, Eintrag; das erste lesbare gilt
    };
    std::vector<Datei> dateien;
    bool jk2 = false;            // Jedi Outcast: dateien[0] ist ext_data/NPCs.cfg
};
[[nodiscard]] NpcLeseAuftrag npcLeseAuftrag();
// Rueckgabe: gelesene Dateien; meldungen: was npcDateienLesen ins Protokoll schreibt
int npcAuftragLesen(const NpcLeseAuftrag& auftrag, NpcMap& m, std::vector<std::string>& meldungen);
// rc603: die Spielordner, die gerade gelten - die dieser Sitzung
// (sitzungsSpielordner), sonst die eingerichteten.
const std::vector<std::string>& aktiveSpielordner();
// rc603: die Spielordner dieser Sitzung setzen (leer = die eingerichteten)
// und neu einlesen, wenn sich dadurch etwas aendert.
void setzeSitzungsSpielordner(std::vector<std::string> ordner);
void loadMissionArchive();
// Die Missionen des Spiels zu einem Archiv suchen und die Auswahl zeigen -
// das, was "Load mission..." nach dem Dateidialog tut (rc603: das ganze Spiel,
// die Missionen des Archivs vorn).
// wunsch: der Name einer Mission - dann gleich diese (zuletzt geoeffnet).
void missionenAusArchiv(const std::string& archiv, const std::string& wunsch = std::string());
// rc603: die Hintergrundsuche abbrechen (Auswahl zu, Mission geladen).
void missionsSucheBeenden();
// rc603: ist die Hintergrundsuche fertig, ihre Missionen in die Liste nehmen.
void missionsSucheUebernehmen();
void loadMission(const Mission& m);
void drawMissionPicker();

// --- rc601 ton: der Vorschauton wie im Spiel (gui/app_ton.cpp) ---------------
//
// Mitschrift fuer den Selbsttest: was die Vorschau spielen wuerde, auch wenn
// sie stumm ist (g_app->stumm). Nur aufgezeichnet, solange eingeschaltet.
struct KlangMitschrift {
    // 'E' Einmalklang gestartet, 'S' Schleife gestartet, 'P' Pegel neu,
    // 'X' angehalten, 'B' Beitrag EINER Quelle zu einer Schleife, 'M' Musik.
    char art = 'E';
    std::string quelle;      // Figur oder Entity, falls bekannt
    std::string datei;
    float links = 0.0F;      // Lautstaerke je Ohr, wie sie in den Mischer geht
    float rechts = 0.0F;
    std::uint64_t id = 0;    // Stimme bzw. Kanalschluessel
    double ms = 0.0;         // Abspielzeit (playMs)
    double abMs = 0.0;       // Einstieg im Klang (Wiedereinstieg)
};
std::vector<KlangMitschrift>& klangMitschrift();
void klangMitschriftAn(bool an);
void klangMitschreiben(const KlangMitschrift& m);
void klangMitschreiben(char art, const std::string& quelle, const std::string& datei, float links,
                       float rechts, std::uint64_t id, double abMs = 0.0);
// Alles anhalten, was der Raumklang gerade fuehrt (zusaetzlich zu
// audio.stopAll() und laufendeSchleifen.clear()).
void tonZuruecksetzen();

// Der Hoerer: die Kamera, die die 3D-Ansicht zeigt - beim Abspielen die
// Skriptkamera (camStateAt), sonst "Through camera" oder die freie Kamera.
// rechts ist der Rechtsvektor mit Roll (sound::rechtsAusWinkeln). Die Engine:
// cgi_S_Respatialize(..., cg.refdef.vieworg, cg.refdef.viewaxis, ...),
// cg_view.cpp:3307 MD (app_view3d.cpp).
void tonHoerer(float ort[3], float rechts[3]);

// Ein Klang, wie cgi_S_StartSound ihn bekommt. Er wird gefuehrt und jedes
// Bild nachgeregelt wie in S_Respatialize: Ort (fest oder der Figur bzw. dem
// Mover folgend), Kanal (Daempfung, Pegel, Stomp), Hoerer-Entity.
struct KlangWunsch {
    std::string datei;
    std::string quelle;            // fuer Mitschrift und Protokoll
    std::string figur;             // folgt dieser Figur (Scene::find), solange sie im Snapshot ist
    int moverModell = -1;          // folgt der Mitte dieses Brush-Modells (EV_BMODEL_SOUND)
    float ort[3]{};                // fester Ort (bzw. Ersatz, solange die Figur unbekannt ist)
    sound::Kanal kanal = sound::Kanal::Auto;
    bool amHoerer = false;         // auf der Hoerer-Entity (Spieler, EV_GLOBAL_SOUND): immer voll
    std::uint64_t schluessel = 0;  // Channel-Stomp (0: CHAN_AUTO sucht sich einen freien Kanal)
    double abMs = 0.0;             // Einstieg im Klang
};
// Liefert die Nummer des Klangs, 0 wenn er nicht gestartet wurde (Datei
// fehlt, schon vorbei, beim Start stumm und kein Stimmkanal).
std::uint64_t tonStarte(const KlangWunsch& w);

// Fuer Einmalklaenge am festen Ort - gedacht fuer das Schwertpaket
// (Klirren, Wandtreffer): cgi_S_StartSound(ort, ..., kanal, ...).
//   ms                 Zeitpunkt des Ereignisses auf der Zeitleiste. Liegt er
//                      VOR dem laufenden Abspielfenster (audioUpTo), beginnt
//                      der Klang an der passenden Stelle, sonst von vorn.
//   imSnapshotPruefen  true fuer Klaenge, die als Ereignis/Temp-Entity ueber
//                      den Snapshot kommen (G_Sound, G_AddEvent): dann nur,
//                      wenn der Ort vom Hoerer aus im PVS liegt. false fuer
//                      cgame-Klaenge (direktes cgi_S_StartSound).
// Danach regelt tonBild ihn jedes Bild nach (Hoerer bewegt sich).
std::uint64_t spieleRaumKlang(const std::string& datei, const float ort[3], sound::Kanal kanal, double ms,
                              bool imSnapshotPruefen = false);
// rc601 saber review: der Channel-Stomp-Schluessel einer Entity und eines
// Kanals, wie ihn die Skriptklaenge nehmen (S_PickChannel/S_CheckChannelStomp).
std::uint64_t tonKanalSchluessel(const std::string& wer, sound::Kanal k);

// Ein sound-Befehl des Skripts nach PlayIcarusSound: wer = Entity (schon
// aufgeloest), schluessel = Channel-Stomp der Stimmkanaele.
void tonSkriptKlang(const std::string& wer, const std::string& datei, sound::Kanal k, std::uint64_t schluessel,
                    double ms, double abMs);
// Wie spieleSkriptKlang, mit Einstieg mitten im Klang (Wiedereinstieg).
void spieleSkriptKlang(const std::string& werSpur, const std::string& datei, const std::string& kanalName,
                       double ms, double abMs);

// Einmal je Bild beim Abspielen: alle laufenden Klaenge neu
// (S_Respatialize), die Schleifen frisch (S_AddLoopSounds), die Klingen
// (soundOn wie CG_Player).
void tonBild();
// rc601 ton (Review): einmal am ENDE jedes Bildes (draw): hat tonBild in
// diesem Bild nicht gemeldet (Abspielen aus, auf welchem Weg auch immer:
// Ende der Zeitleiste, Reiterwechsel, Schluesselsprung, Spurenansicht ...),
// enden die Schleifen - wie S_ClearLoopingSounds jedes Bild in der Engine.
// Einmalklaenge laufen weiter.
void tonWache();
// Die Kartenereignisse im Abspielfenster (vonMs, bisMs]: target_speaker,
// Tueren, ROFF-Notizen, Zerbrechen, fx_runner.
void tonEreignisse(double vonMs, double bisMs);
// Ton wieder an / Abspielen fortgesetzt: jeden laufenden sound-Befehl an
// seiner Stelle wieder einsetzen.
void tonWiedereinstieg();
// Alles vorab entpacken, was die Szene klingen laesst (Schwerter,
// Lautsprecher, Tueren, Zerbrechen).
void tonVorladen();
// Liegt ein Kasten (Ursprung + mins/maxs) vom Hoerer aus im PVS?
bool tonImSnapshot(const float ort[3], const float mins[3], const float maxs[3]);
// Die Mischpegel (s_volume, s_volumeVoice, s_musicvolume) aus den
// Einstellungen, Vorgaben wie die Engine.
sound::Mischpegel tonPegel();
float tonMusikPegel();
// Die im Hintergrund geoeffneten Tongeraete uebernehmen (app.cpp).
void tonUebernehmen(bool warten);
// rc601 ton (Review): Einblick fuer Selbsttest und Protokoll - wie viele
// Schleifenkanaele bzw. gefuehrte Einmalklaenge der Raumklang gerade hat
// (datei leer: alle), und der Puffer, den der Mischer fuer eine Datei bekommt.
std::size_t tonSchleifenZahl(const std::string& datei = std::string());
std::size_t tonEinmalZahl(const std::string& datei = std::string());
sound::Mixer::Daten tonMischerDaten(const std::string& datei);

// --- Paket saber (rc601): Lichtschwerter an Waenden und gegeneinander -------
// (gui/app_saber.cpp; die Rechnungen in bhed/saberkontakt.h)
//
// Aus drawMapView herausgezogen, damit die Schwertproben dieselbe
// Knochenrechnung haben wie die Zeichnung (app_view3d.cpp):
void figurBilderSetzen(const Actor& a, const App::ActorAssets& assets, const ActorState& st, double ms, ActorDraw& d);
// reste2 Punkt 6: die GPU-Tabelle des Statistikfensters der Karte, Zeile fuer Zeile (true = eingerueckt)
[[nodiscard]] std::vector<std::pair<std::string, bool>> gpuTabellenZeilen();
void figurWeltMatrix(const ActorDraw& a, float fw[16]);
// rc608: CG_UpdateLookAngles je Bild (siehe blickFolgeRechnen); figurKopfGier nimmt den letzten Stand.
void blickFolgeRechnen(const Actor& a, std::size_t ai, const App::ActorAssets& assets, double ms);
void figurKopfGier(const Actor& a, const ActorState& st, ActorDraw& d);
// Endrunde 4: dasselbe zur Zeit ms aus einem eigenen Stand (figurProbe der Schwerter)
void figurKopfGierBei(const Actor& a, std::size_t ai, const App::ActorAssets& assets, const ActorState& st, double ms, ActorDraw& d);
void anbautenFuer(const Actor& a, const ActorState& st, double ms, ActorDraw& d);
void npcSkalaFuer(const std::string& npcType, float aus[3]);
// Pruefung 3: die Knochen (welt) und die Weltmatrix (fw) einer Figur im
// Zustand st zur Zeit ms - dieselbe Rechnung wie die Zeichnung
// (figurBilderSetzen, figurKopfGier, figurKnochen, figurWeltMatrix).
bool figurPose(const Actor& a, const App::ActorAssets& assets, const ActorState& st, double ms, ActorDraw& d,
               std::vector<BoneMatrix>& welt, float fw[16]);
// Pruefung 3: renderInfo.torsoAngles aus dem chestBolt des gezeichneten
// Skeletts (cg_players.cpp:7516-7527 OJ); false: kein chestBolt (VectorClear).
bool figurRumpfWinkel(const Actor& a, const App::ActorAssets& assets, double ms, float winkel[3]);
// Pruefung 3: renderInfo.handRPoint (Bolzen "*r_hand") und muzzleDir (Richtung
// der ersten Klinge des Schwerts in der rechten Hand) zur Zeit ms.
bool figurHandPunkt(const Actor& a, const App::ActorAssets& assets, double ms, float hand[3], float richtung[3]);
// Pruefung 4a: renderInfo.eyePoint aus dem headBolt (kopfBolzen) zur Zeit ms;
// false: kein headBolt (dann gilt der Ursprung der Figur).
bool figurAugenPunkt(const Actor& a, const App::ActorAssets& assets, double ms, float auge[3]);
// Rc608: die Geschuetze (gui/app_view3d_geschuetz.inc).
// Rc608: was sonst Schaden nimmt und zerbricht (takedamage): func_breakable
// (SP_func_breakable: ohne spawnflags 1 mindestens health 10), misc_model_breakable
// (SP_misc_model_breakable: health) und misc_trip_mine (health 15).
struct BruchZiel {
    int entity = -1;
    int health = 0;
    float lo[3]{};              // absmin
    float hi[3]{};              // absmax
    float splashOrt[3]{};       // org in funcBBrushDieGo / misc_model_breakable_die
    int splashSchaden = 0;
    float splashWeite = 0.0F;
    double bruchVerzug = 0.0;   // "delay": funcBBrushDie -> funcBBrushDieGo
    bool nurSchwer = false;     // FL_DMG_BY_HEAVY_WEAP_ONLY
    bool nurSchwert = false;    // FL_DMG_BY_SABER_ONLY
    bool mine = false;
    double abMs = -1.0e300;     // Mine: takedamage ab
    double bisMs = 1.0e300;     // Mine: G_FreeEntity
};
std::vector<BruchZiel> bruchZieleDerKarte();
GeschuetzWelt geschuetzWeltJetzt();
void geschuetzeStarten();
void geschuetzErgebnisUebernehmen();
void geschuetzGeschosseZeichnen(double jetzt, std::vector<EffectInstance>& aus);
// Pruefung 4a: renderInfo.muzzlePoint - der Bolzen "*flash" des Waffenmodells in
// der rechten Hand (CG_Player, cg_players.cpp:7581-7596 OJ); false: keins.
bool figurMuendung(const Actor& a, const App::ActorAssets& assets, double ms, float muendung[3]);
// Pruefung 4a: rumpfWinkel, handPunkt, augenPunkt und fussPunkte der
// SzenenHilfe aus den gezeichneten Skeletten (index: Name -> actorAssets).
void figurHakenSetzen(SzenenHilfe& h, const std::map<std::string, std::size_t>& index);
// Pruefung 3: renderInfo.footLPoint/footRPoint (footLBolt/footRBolt) zur Zeit ms -
// nur G_StandardHumanoid ("*l_leg_foot") und CLASS_ATST ("*l_foot"), wie
// PM_AdjustStandAnimForSlope es verlangt.
bool figurFussPunkte(const Actor& a, const App::ActorAssets& assets, double ms, float links[3], float rechts[3]);
// Zeitleiste 5: dasselbe fuer einen gegebenen Zustand (die Aufnahme zeichnet die
// Figur nicht aus a.at(ms), sondern aus ihrem eigenen Stand).
bool figurFussPunkte(const Actor& a, const App::ActorAssets& assets, const ActorState& st, double ms, float links[3],
                     float rechts[3]);
// Pruefung 3: items.dat des Regelwerks (FindItemForWeapon/FindItemForAmmo).
const Gegenstandsliste& gegenstandslisteJetzt();
// Pruefung 3: der Gegenstand einer Waffe (giType IT_WEAPON, giTag == waffe)
// bzw. des Kristalls (IT_AMMO, AMMO_FORCE) - nullptr: keiner.
const Gegenstand* gegenstandFuerWaffe(const std::string& waffe);
const Gegenstand* gegenstandKraftKristall();
// Pruefung 3: das Modell, mit dem ein Abwurf gezeichnet wird (Waffe/Kristall:
// worldmodel aus items.dat, Schwert: der Griff).
std::string abwurfModell(const TodesAbwurf& ab);
// Pruefung 3: ein eigener Klang "*death1.wav" einer Figur (CG_CustomSound) als
// Datei - leer, wenn die Figur keinen hat.
std::string eigenerKlangDatei(const std::string& figur, const std::string& name);
const efx::Effect* effectByName(const std::string& name);
// rc608 Teil 13c: wohin Effektdateien und ihre Shader geladen werden - leer: die Vorschau (g_app->effects, g_app->textures,
// g_app->effectShaders). Das Spielbild des Karten-Editors hat eigene (die Vorschau leert ihre bei jedem Kartenwechsel).
// Die Bilder der Shader gehen in den Satz, in den loadMapTextures gerade laedt (ladeSatz) - `satz` muss dieser sein.
struct EffektZiel {
    std::map<std::string, efx::Effect>* effekte = nullptr;
    TextureSet* satz = nullptr;
    EffectShaderSlots* plaetze = nullptr;
};
// rc604: eine Effektdatei samt Shadern laden (app_view3d.cpp) - auch fuer den Selbsttest.
bool ladeEffektDatei(const std::string& path, const std::string& herkunft, const EffektZiel& ziel = {});
// rc604 Endrunde: die Folgeeffekte (impactfx, deathfx, emitfx, playfx) Stufe fuer Stufe vorab, samt Shadern.
void ladeFolgeeffekte(std::vector<const efx::Effect*> offen, int* nachgeladenAus, int* ohneDateiAus, const EffektZiel& ziel = {});
// rc608 Teil 13c: ein fx_runner als Effekt nach SP_fx_runner/fx_runner_link (g_fx.cpp) - Datei, Richtung, Abstand, erster
// Durchgang; startMs < 0, wenn er auf ein use wartet (STARTOFF, ONESHOT). false: keine Effektdatei.
bool fxRunnerAus(const MapEntity& e, const std::vector<MapEntity>& alle, const EffektZiel& ziel, EffectInstance& inst);
// Nach dem Laden einer .bsp (kollisionLaden): die Schwertspuren der alten Karte
// vergessen. rc601 integ: die Brushes kommen aus geladeneKollision().
void saberKarteGeladen();
// Die Takte bis ms rechnen (Clientbilder 8 ms, Serverbilder 50 ms).
void saberRechneBis(double ms);
// Funken, Kochen und Klirren als Effekte fuer die Effektliste zu nowMs.
std::vector<EffectInstance> saberEffekteBei(double nowMs);
// Die Klaenge im Abspielfenster (vonMs, bisMs] - wie tonEreignisse.
void saberTonEreignisse(double vonMs, double bisMs);
// In der Klingenschleife der Zeichnung: die Spitze wB auf die Laenge dieses
// Clientbildes setzen (CG_DoSaber). false: die Klinge steht nicht (Laenge 0).
bool saberKlingeZeichnen(int figur, int fach, int klinge, const float wA[3], float wB[3], float laengeSzene);
// rc601 saber review: die Laenge, die CG_DoSaber in diesem Clientbild bekommt
// (L+8 beim Wachsen, an Waenden gekuerzt, Zuendgeschwindigkeit nach Kampfstil),
// sobald die Schwertrechnung sie kennt; sonst laengeSzene. 0: steht nicht.
float saberZeichenLaengeBei(int figur, int fach, int klinge, double ms, float laengeSzene);
// Fuer Selbsttest und Protokoll.
struct SaberGezeichnet {
    int figur = -1;
    int fach = 0;
    int klinge = 0;
    float wurzel[3]{};
    float spitze[3]{};
};
std::vector<SaberGezeichnet> saberGezeichnetesBild();
// rc603 fix603: das Bild (ImGui::GetFrameCount), in dem die Liste oben entstand.
int saberGezeichnetesBildNr();
std::vector<SaberKlirren> saberKlirrenBis(double ms);
std::vector<SaberWandEreignis> saberWandEreignisseBis(double ms);
struct SaberZaehlung {
    long long takte = 0;
    long long figurenProben = 0;
    int klirren = 0;
    int funken = 0;
    int wandKlaenge = 0;
    double erstesKlirrenMs = -1.0;
    double ms = 0.0;
};
// Die ganze Szene von 0 bis bisMs der Reihe nach (Szenenlauf im Selbsttest).
SaberZaehlung saberZaehlen(double bisMs);
std::string saberLeistung();
void ladeEffektShader(const efx::Effect& ef, const EffektZiel& ziel = {});
// CG_SaberClashFlare (rc601 saber review 2c: als 2D-Bild nach den Kinobalken).
// saberBlitzAnsicht merkt beim Rendern die Kamera einer Ansicht (0 Hauptansicht,
// 1 Kameravorschau; kam nullptr: keine), saberBlitzZeichnen2D zeichnet den
// Blitz ueber deren Bild [a, b].
void saberBlitzAnsicht(int ansicht, const Camera* kam, int w, int h, double nowMs);
void saberBlitzZeichnen2D(int ansicht, ImDrawList* dl, ImVec2 a, ImVec2 b, bool nachBalken);
extern int g_saberBlitze;
extern int g_saberBlitzeNetz;
extern int g_saberBlitze2D;
extern int g_saberBlitzeNachBalken;
// rc601 saber review: Brandspuren (CG_CreateSaberMarks, CG_AddMarks) - ins
// Effektnetz haengen, ob es welche gibt, und fuer den Selbsttest die lebenden
// Marken zu ms (rechnet bis ms).
// Pruefung 3: mit den Decals der Effekte `effekte` im selben Markenspeicher.
void saberSpurenAnhaengen(BspMesh& netz, double nowMs, const std::vector<EffectInstance>* effekte = nullptr);
bool saberSpurenDa(double nowMs, const std::vector<EffectInstance>* effekte = nullptr);
std::vector<SpurPoly> saberSpurenBei(double ms, const std::vector<EffectInstance>* effekte = nullptr);
// Alles vergessen (Selbsttest: Vorspulen gegen Abspielen vergleichen).
void saberVergessen();
// rc603 fix603, nur fuer den Selbsttest: hoechstens so viele Clientbilder im
// Speicher (0: die Vorgabe, 10 min) - so laufen Luecken, Schnappschuesse und
// der gerettete laufende Stand auch in kurzen Szenen.
void saberObergrenze(std::size_t bilder);
extern int g_saberSpurenGezeichnet;

// Stufe 8c: das Logo von JK Studio (PNG-Ressource JKLOGO) als Textur, b x h Bildpunkte (gui/app.cpp)
void* jkLogoTextur(int& b, int& h);

}  // namespace bhed::gui
#endif
