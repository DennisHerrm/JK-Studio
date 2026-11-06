// modelltab.h - der Zustand des neuen Model-Tabs (Modell-Block 1a-1)
//
// Aufbau nach dem Entwurf `entwurf_modelltab.html` (shank 04.11.: "Bedienung
// wie im Entwurf"): oben die Bereiche (Ansehen/Pruefen, Modellieren, UV,
// Weights, Painting, Animation), links der Szenen-Explorer, in der Mitte eine
// oder vier Ansichten, rechts das Befehlsfeld mit Rollouts, unten die
// Zeitleiste. Der Bereich Ansehen/Pruefen kann, was ModView kann
// (modview_vergleich.md), gezeichnet wird ueber den Ghoul2-Weg des
// Mission-Tabs. Die Bedienung folgt 3ds Max (Vorgabe) - die Belegung wie
// Blender kommt in 1a-2.
#ifndef BHED_GUI_MODELLTAB_H
#define BHED_GUI_MODELLTAB_H

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "imgui.h"

#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/mapview.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/gewichte.h"

namespace bhed::gui {

// Ein angebautes Modell (G2API_AttachG2Model an einen Bolzen, G2API_AddBolt):
// an eine Tag-Flaeche ("*r_hand") oder einen Knochen.
struct ModellAnbau {
    std::string pfad;      // .glm in den Archiven
    GlmModel modell;
    ModelTextures texturen;
    std::string ziel;      // Tag-Flaeche oder Knochen
    bool knochen = false;
    bool versteckt = false;   // mb1a2: Auge im Explorer (nur die Ansicht)
};

// Eine der vier Ansichten. Oben, Vorne und Links sind orthographisch, die
// Perspektive ist die Kreiskamera des Model-Tabs (modelCam, modelOrbit).
// Stufe 2: eine GLA als Quelle der Plaetze einer Karten-GLA (feste Kopie, die Plaetze zeigen auf sie).
struct ModellGlaQuelle {
    std::string name;
    GlaAnimation gla;
};

enum class AnsichtArt : int { Oben = 0, Vorne = 1, Links = 2, Perspektive = 3, Unten = 4, Hinten = 5, Rechts = 6 };

struct ModellAnsicht {
    AnsichtArt art = AnsichtArt::Perspektive;
    float mitte[3]{};          // orthographisch: der Punkt in der Bildmitte
    float halbHoehe = 64.0F;   // orthographisch: halbe Bildhoehe in Einheiten
    // vom letzten Bild: Lage auf dem Schirm und die Matrix (fuer Ueberlagerung und Auswahl)
    float lo[2]{};
    float groesse[2]{};
    float viewProj[16]{};
    bool sichtbar = false;
};

enum class ModellBereich : int { Ansehen = 0, Modellieren, Uv, Weights, Painting, Animation };
inline constexpr int kModellBereiche = 6;

// Was in der Ansicht ueber dem Modell liegt (ModView: Menue "View").
struct ModellAnzeige {
    bool drahtgitter = false;   // F3
    bool kanten = false;        // F4: Flaechen mit Kanten
    bool knochen = false;       // die Knochen als Bone-Objekte
    bool flossen = false;       // Bone-Kasten mit Flossen
    bool gewichte = false;      // Gewichte an den Ecken
    bool normalen = false;
    bool tags = false;
    bool ursprung = false;      // RGB-Achsen im Ursprung
    bool huelle = false;        // Huellkasten
    bool boden = false;
    float bodenZ = -24.0F;      // DT85 ModView: grauer Boden auf -24
    bool raster = false;        // G
    bool weiss = false;         // weisse Texturen (Risse finden)
    bool risse = false;         // offene Kanten der gewaehlten Flaeche
    bool zweiseitig = false;
    bool durchsicht = false;    // Alt+X
    bool statistik = false;     // 7
    bool klammer = true;        // J: Auswahlklammer
    // mb1a2: ModView "Show vertex indexes" / "Show triangle indexes" (r_surface.cpp:864-935) an der gewaehlten
    // Flaeche, "Ruler" (model.cpp:2431-2462)
    bool eckNummern = false;
    bool dreieckNummern = false;
    bool lineal = false;
};

// mb1a2: Unterobjekte. Objekt = Flaechen (Teile); Ecke/Kante/Dreieck waehlen Ecken der gezeichneten Stufe
// (Blender 1/2/3, 3ds Max 1/2/4), Rand und Element wie Max 3 und 5.
enum class AuswahlArt : int { Objekt = 0, Ecke, Kante, Dreieck, Rand, Element };

// mb1a2: G/R/S wie Blender (transform.translate/rotate/resize, modal mit Achse und Zahl)
struct ModellModal {
    int art = 0;            // 0 keins, 1 G, 2 R, 3 S
    int achse = -1;         // 0..2 X/Y/Z
    int achsStufe = 0;      // 0 aus, 1 global, 2 lokal (Blender: zweimal dieselbe Taste)
    bool ebene = false;     // Umschalt+X: die Ebene ohne X
    std::string zahl;       // Zahleneingabe
    float mausStart[2]{};
    float mitte[3]{};
    int ansicht = 3;
};

struct ModellTab {
    ModellBereich bereich = ModellBereich::Ansehen;
    modell::Dokument dok;
    modell::G2Zustand g2;          // vom Anwender (Haut, an/aus, Wurzel)
    std::string dokModell;         // modelPath, zu dem das Dokument gehoert
    // Das gezeichnete Modell: eine Stufe des Dokuments, Haut und Texturen aus
    // App::model, Sichtbarkeit nach den Ghoul2-Regeln.
    GlmModel anzeige;
    std::vector<char> gezeichnet;  // je Flaeche (Ghoul2-Regeln)
    std::string anzeigeSchluessel; // wann neu bauen
    std::vector<char> hautAus;     // letzte Haut (skinnedOff je Flaeche)
    int lodWahl = 0;               // 0 = automatisch wie im Spiel, sonst Stufe + 1
    int lodJetzt = 0;              // gezeichnete Stufe
    int zielspiel = 0;             // modell::Zielspiel
    bool interpolieren = true;
    int sekundaer = -1;            // Animation fuer den Oberkoerper (-1 keine)
    std::string sekundaerKnochen = "lower_lumbar";
    int sekFrame = 0;
    float sekZeit = 0.0F;
    ModellAnzeige an;
    // --- mb1a2: das Auge im Szenen-Explorer - blendet NUR in der Ansicht aus (wie Max "Hide", Blenders Auge); Datei und
    // Ghoul2-Regeln bleiben. Je Teil (Flaechen, Tags) und je Knochen; bleibt beim Wechsel von Sequenz und LOD.
    std::vector<char> verstecktTeil;
    std::vector<char> verstecktKnochen;
    int verstecktStand = 0;       // zaehlt jede Aenderung (Schluessel der Anzeige)
    int symbolZaehler = 0;        // Selbsttest: gezeichnete Symbole im Explorer
    int augenZaehler = 0;         // Selbsttest: gezeichnete Augen
    // --- mb1a2: Weights ----------------------------------------------------------------------------
    int pinselArt = 0;            // modell::PinselArt
    float pinselGewicht = 1.0F;
    float pinselStaerke = 0.5F;
    float pinselRadius = 40.0F;   // Bildpunkte
    bool pinselSpiegeln = false;  // X-Spiegel beim Malen (Knochen l/r)
    bool malStrich = false;       // ein Strich laeuft (ein Rueckgaengig-Schritt)
    float letzterTupfer[2]{};
    std::vector<std::vector<modell::EckOrt>> spiegel;   // Spiegelpartner der gezeichneten Stufe
    int spiegelStand = -1;        // dok.stand, fuer den die Karte gilt
    int tupferZahl = 0;           // Selbsttest
    int gemalteEcken = 0;         // Selbsttest: Ecken des letzten Tupfers
    // --- mb1a2: Unterobjekte, Blender ---------------------------------------------------------------------
    AuswahlArt auswahlArt = AuswahlArt::Objekt;
    AuswahlArt editArt = AuswahlArt::Ecke;   // Tab kehrt zu ihr zurueck
    std::vector<modell::EckOrt> eckenWahl;   // gewaehlte Ecken (gezeichnete Stufe)
    ModellModal modal;
    int modalEndeBild = -1;       // das Bild, in dem ein Klick G/R/S beendet hat (waehlt dann nichts)
    bool bereichMenue = false;    // Strg+Tab
    bool modusMenueSichtbar = false;
    bool kontextOeffnen = false;  // Rechtsklick in die Ansicht
    bool kontextSichtbar = false;
    // --- mb1a2: Ins Skript --------------------------------------------------------------------------
    int insAnim = -1;
    int insBefehl = 0;            // 0 SET_ANIM_BOTH 1 UPPER 2 LOWER 3 HOLDTIME_BOTH 4 HOLDTIME_UPPER 5 HOLDTIME_LOWER
    int insZiel = -2;             // Reiter (-1 neues Skript, -2 der aktive)
    char insFigur[64] = {};
    int insHalteMs = 0;
    std::string insMeldung;
    std::string zaehlungText;     // Selbsttest: die Zaehlung der Zeitleiste im letzten Bild
    float fov = 80.0F;
    // Auswahl: Flaechen (Teil-Indizes) und ein Knochen
    std::vector<int> auswahl;
    int knochenWahl = -1;
    int tagWahl = -1;
    // Bedienung wie 3ds Max
    int gizmo = 0;            // 0 Q Auswahl, 1 W verschieben, 2 E drehen, 3 R skalieren
    int achse = -1;           // F5/F6/F7 X/Y/Z, -1 frei; F8 Ebenen 3..5 (XY, YZ, ZX)
    float gizmoGroesse = 1.0F;
    bool vierAnsichten = false;
    int aktiv = 3;            // aktive Ansicht
    ModellAnsicht ansichten[4];
    bool fokus = false;       // die Ansicht hat den Fokus (Tasten des Max-Modus)
    bool aktionenOffen = false;   // X: "Alle Aktionen suchen"
    int klickDrinBild = -1;   // Bild des letzten Klicks in Ansicht, Explorer, Befehlsfeld oder Leiste
    std::vector<ModellAnbau> anbauten;
    char suche[64] = {};
    std::string meldung;      // letzte Rueckmeldung (Statuszeile des Befehlsfelds)
    // Ziehen mit dem Gizmo: Griff und Startlage
    int griff = 0;
    float ziehStart[2]{};
    std::vector<modell::Inhalt> ziehVorher;
    float ziehWeg[3]{};
    // Rahmen ziehen (Auswahl)
    bool rahmen = false;
    float rahmenStart[2]{};
    // Zaehler fuer den Selbsttest
    int tastenZaehler = 0;
    std::string letzteTaste;
    int neuGebaut = 0;        // wie oft die Anzeige neu gebaut wurde
    // je Bild gezeichnete Ueberlagerung: 0 Knochen-Striche, 1 Normalen, 2 Gewichte, 3 Tags, 4 Huelle, 5 Ursprung,
    // 6 Randecken (Risse), 7 Auswahl-Striche; mb1a2: 8 Ecknummern, 9 Dreiecknummern, 10 Lineal-Striche, 11 Pinselkreis,
    // 12 gewaehlte Ecken
    int ueber[13]{};
    bool aktionenSichtbar = false;   // die Suche "Alle Aktionen" ist offen
    // Stufe 2: Speichern, Frage im Programm, animation.cfg, Karten-GLA, Marker
    std::string speicherPfad;     // wohin Strg+S schreibt (nach dem ersten Speichern)
    int gespeichertStand = -1;    // dok.stand() beim letzten Speichern
    int frageArt = 0;             // 0 keine, 1 Modell speichern, 2 Karten-GLA bauen (trotz Befunden?)
    std::string frageZiel;
    std::vector<std::string> frageZeilen;
    bool frageSichtbar = false;
    std::string cfgFuer;          // GLA, zu der die animation.cfg im Dokument gehoert
    std::string cfgPfadJetzt;
    int cfgStand = -1;
    int cfgBefunde = 0;           // Selbsttest
    std::deque<ModellGlaQuelle> kgQuellen;
    std::vector<modell::Platz> plaetze;
    char kgKarte[64] = {};
    int glaZiel = 0;              // 0 Karten-GLA, 1 _humanoid.gla (Mod-Ordner), 2 eigene GLA (eigenes Skelett)
    char eigenName[64] = {};      // Ordner und Name der eigenen GLA
    std::map<std::string, modell::PlatzQuelle> belegung;   // Platzname -> Quelle, fuer alle Ziele
    std::string plaetzeFuer;
    int platzWahl = -1;
    int kgStand = 0;
    int ziehtSequenz = -1;        // Selbsttest: zuletzt gezogene Sequenz
    int markerAenderungen = 0;
    int markerGriff = 0;          // 1 Anfang, 2 Ende des Balkens
    AnimEntry markerZug;
};

// Fuer den Selbsttest und die Leiste (gui/app_modelltab.cpp)
void modellDokumentLaden(const std::string& bytes);
void modellAnzeigeAktualisieren();
void drawModellBereiche();
void modellSequenzZeile(const AnimEntry* e);
// mb1a2
void modellDateiMenue();
void modellDateiTasten(bool nurRueckgaengig);
void drawModellInsSkriptFenster();
void modellInsSkriptOeffnen(int anim);
[[nodiscard]] std::string modellInsSkriptZeile();
bool modellInsSkriptEinfuegen();
void drawModellGewichte();
[[nodiscard]] bool modellBlender();
[[nodiscard]] const char* modellBlenderTasteQuelle(int i);
[[nodiscard]] int modellBlenderTasteZahl();
[[nodiscard]] ImGuiKeyChord modellBlenderTasteChord(int i);
[[nodiscard]] const char* modellBlenderTasteText(int i);
bool modellTupfer(int ansicht, ImVec2 maus, bool neuerStrich);
void modellStrichEnde();
bool modellUebernehmen();
int modellAufAuswahlSetzen();
int modellSpiegeln(int quelle);
void modellAuswahlArtSetzen(AuswahlArt a);
[[nodiscard]] bool modellGewichtFarbe(int flaeche, int ecke, float& gewicht);
[[nodiscard]] bool modellEckeAufSchirm(int flaeche, int ecke, ImVec2& out);
bool modellFotoMitUeberlagerung(int breite, std::vector<unsigned char>& rgba, int& w, int& h);
void modellAnsichtSetzen(AnsichtArt a);
[[nodiscard]] bool modellTeilVersteckt(int teil);
[[nodiscard]] bool modellKnochenVersteckt(int knochen);
// Auge: Teil (mit allen Kindern) aus/ein; solo: nur dieses (und seine Kinder) zeigen.
void modellTeilAuge(int teil, bool solo);
void modellKnochenAuge(int knochen, bool solo);
void modellAllesEinblenden();
[[nodiscard]] float modellAugeX();
void modellAuswahlAusblenden(bool nichtGewaehlte);
bool modellModalStarten(int art);
void modellModalEnde(bool uebernehmen);
bool modellModalTasten();
void drawModellModusMenue();
void drawModellKontextMenue();
[[nodiscard]] int modellZusatzZahl();
[[nodiscard]] const char* modellZusatzText(int i);
[[nodiscard]] const char* modellZusatzHinweis(int i);
void modellZusatzAusloesen(int i);
void modellBedienungSetzen(int b);
// Stufe 2 (app_modelltab_mb3.cpp)
void modellSpeichern(bool unter);
[[nodiscard]] bool modellUngespeichert();
void drawModellSpeichernFrage();
void drawModellAnimationBereich();
void modellMarkerZeile(const AnimEntry* e);
bool modellMarkerAendern(const AnimEntry& neu);
void modellSequenzZiehen(int anim);
bool modellPlatzSetzen(int platz, int anim);
int modellAufFreienPlatz(int anim);
void modellPlatzLeeren(int platz);
void modellGlaZielSetzen(int ziel);
[[nodiscard]] bool modellImSpielordner(const std::string& datei);
bool modellVorhandeneKartenGla();
bool modellKartenGlaBauen(const std::string& datei, bool trotzdem);
bool modellGlaSpeichern(const std::string& datei);
bool modellCfgSpeichern(const std::string& datei);
void modellGlaSpeichernFrage();
void modellCfgSpeichernFrage();
void modellKartenGlaBauenFrage();
bool modellXsiAufnehmen(const std::string& datei);
void modellXsiAufnehmenFrage();
void modellPerspektiveUmschalten();
void modellKreisenSchritt(float gier, float nick);
void drawModellExplorer(float hoehe);
void drawModellBefehlsfeld();
void drawModellArbeitsflaeche();
void drawModellAktionen();
void modellAnsichtEinpassenAktiv();
bool modellTasten();
bool modellRueckgaengig();
bool modellWiederholen();
[[nodiscard]] bool modellFokus();
void modellKlickPruefen();        // im Fenster eines Bereichs des Model-Tabs aufrufen
void modellFokusAktualisieren();  // am Anfang von handleShortcuts
[[nodiscard]] bool modellSpielKnochen(const std::string& name);
void modellFlaecheSchalten(int flaeche, std::uint32_t offFlags);
void modellWurzelSetzen(int flaeche);
bool modellAnbauen(const std::string& pfad, const std::string& ziel, bool knochen, bool ersetzen);
void modellAnbautenLoesen();
bool modellFotoSpeichern(const std::string& pfad, int breite);
void modellTexturenNeuLaden();
bool modellGlaLaden(const std::string& pfad);
[[nodiscard]] std::vector<modell::Befund> modellBefunde();
[[nodiscard]] int modellTasteZahl();
[[nodiscard]] ImGuiKeyChord modellTasteChord(int i);
[[nodiscard]] const char* modellTasteText(int i);
[[nodiscard]] bool modellFotoPixel(int breite, int hoehe, std::vector<unsigned char>& rgba, bool ueberDokument);
// Fuer den Selbsttest: das Bild einer Ansicht, ein BMP schreiben, Schirmorte (Flaeche, Mitte des Modells,
// Griff des Gizmos), die Stellung eines Anbaus, die Mitte der Auswahl.
[[nodiscard]] bool modellAnsichtPixel(int ansicht, std::vector<unsigned char>& rgba, int& breite, int& hoehe);
bool modellBmp(const std::string& pfad, int breite, int hoehe, const std::vector<unsigned char>& rgba);
[[nodiscard]] bool modellFlaecheAufSchirm(int flaeche, ImVec2& out);
[[nodiscard]] bool modellPunktAufSchirm(int ansicht, const GlmModel& m, ImVec2& out);
[[nodiscard]] bool modellGizmoGriffOrt(int griff, ImVec2& out);
[[nodiscard]] bool modellAnbauWelt(int anbau, float welt[16]);
[[nodiscard]] bool modellAuswahlMitte(float mitte[3]);

}  // namespace bhed::gui
#endif
