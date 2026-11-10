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

#include <array>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "imgui.h"

#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/mapview.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/fehler.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/gewichte.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/physik.h"
#include "bhed/modell/retarget.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/wof/poly.h"
#include "bhed/modell/wof/uv.h"
#include "bhed/modell/malen.h"

namespace bhed::gpu {
struct ModellGeist;
}

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

// Stufe 3: ein Bewegungspfad (Knochen oder Tag) mit Farbe.
struct ModellPfad {
    int knochen = -1;
    int tag = -1;        // Teil-Index eines Tags (dann knochen = -1)
    float farbe[4]{};
    bool spitze = false; // Stufe 4: die Spitze des Saebels, der an diesem Tag angebaut ist
};
// Stufe 3: Ghosts (Onion Skin)
struct ModellGeisterEinst {
    bool an = false;
    int davor = 3;
    int danach = 3;
    int abstand = 2;     // Bilder zwischen zwei Ghosts
    float farbeDavor[4] = {0.30F, 0.60F, 1.00F, 0.35F};
    float farbeDanach[4] = {1.00F, 0.55F, 0.25F, 0.35F};
};

// MBKOPF: Orthographisch = 3ds Max "Orthographic" (U) / Blender "User Orthographic" - frei gedreht; die Richtung
// (ModellTab::orthoWinkel) teilen sich alle Ansichten dieser Art, wie die Perspektive sich modelCam teilt
enum class AnsichtArt : int { Oben = 0, Vorne = 1, Links = 2, Perspektive = 3, Unten = 4, Hinten = 5, Rechts = 6, Orthographisch = 7 };

// MBKOPF: die Schattierung gehoert der einen Ansicht wie in 3ds Max (Viewport Shading Label Menu: Drahtgitter nur in
// einer Ansicht) und Blender (Viewport Shading je 3D-Ansicht); Raster (G) und Statistik (7) schalten in Max ebenso
// nur die aktive Ansicht.
struct ModellSchattierung {
    bool drahtgitter = false;   // F3 / Blender: Wireframe
    bool kanten = false;        // F4: Flaechen mit Kanten / Blender: Overlay Wireframe
    bool zweiseitig = false;    // Blender: Backface Culling aus
    bool weiss = false;         // weisse Texturen / Blender: Solid
    // MBRASTER: ab Werk an wie 3ds Max (Home Grid, G) und Blender (Overlays: Grid, Floor, Axes X und Y; Z aus). 3ds Max: G schaltet
    // das ganze Home Grid samt Weltachsen; Blender: `raster` das Raster der Achsenansichten (Grid), `rasterBoden` den Boden der
    // Perspektive und der freien Orthoansicht (Floor), `achsen` die farbigen Weltachsen.
    bool raster = true;         // G / Blender: Grid
    bool rasterBoden = true;    // Blender: Floor
    bool achsen[3] = {true, true, false};   // Blender: Axes X Y Z
    bool statistik = false;     // 7
};

struct ModellAnsicht {
    AnsichtArt art = AnsichtArt::Perspektive;
    ModellSchattierung sch;    // MBKOPF
    // MBKOPF: ViewCube/Gizmo - weicher Wechsel der Blickrichtung (Winkel wie Camera::angles), danach `animArt`
    double animStart = -1.0;
    float animVon[3]{};
    float animZu[3]{};
    AnsichtArt animArt = AnsichtArt::Perspektive;
    float animDauer = 0.2F;    // Sekunden dieses Wechsels
    // Home des ViewCube fliegt (max_viewcube.h goHome): Abstand und Drehpunkt der Perspektive gehen mit, das Ende ist genau `animEnde`
    bool animFlug = false;
    float animAbstand[2]{};    // von, zu
    float animZiel[2][3]{};    // Drehpunkt von, zu
    float animEnde[3]{};       // Winkel am Ende des Flugs
    float kopfRechts = 0.0F;   // MBKOPF: Schirm-x, bis zu dem die Felder der Kopfzeile reichen (links)
    float kopfLinks2 = 0.0F;   // MBKOPF: Blender - ab hier die Felder rechts (Overlays, Shading)
    float kopfUnten = 0.0F;    // MBKOPF: Schirm-y der Unterkante der Kopfzeile
    float wuerfel[4]{};        // MBKOPF: Rechteck von ViewCube/Gizmo (x0, y0, x1, y1), 0 wenn nicht gezeichnet
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
// MBKOPF: Drahtgitter, Kanten, zweiseitig, weiss, Raster und Statistik je Ansicht (ModellSchattierung); Knochen
// (g_app->modellKnochen) und Flossen (settings.modellFlossen) liegen ausserhalb.
struct ModellAnzeige {
    bool gewichte = false;      // Gewichte an den Ecken
    bool normalen = false;
    bool tags = false;
    bool ursprung = false;      // RGB-Achsen im Ursprung
    bool huelle = false;        // Huellkasten
    bool boden = false;
    float bodenZ = -24.0F;      // DT85 ModView: grauer Boden auf -24
    bool risse = false;         // offene Kanten der gewaehlten Flaeche
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
    std::uint64_t schritt = 0;   // pfix H1: der Rueckgaengig-Schritt von G/R/S
    int achse = -1;         // 0..2 X/Y/Z
    int achsStufe = 0;      // 0 aus, 1 global, 2 lokal (Blender: zweimal dieselbe Taste)
    bool ebene = false;     // Umschalt+X: die Ebene ohne X
    std::string zahl;       // Zahleneingabe
    float mausStart[2]{};
    float mitte[3]{};
    int ansicht = 3;
};

// Stufe 5: Retargeting (app_modelltab_retarget.cpp) - die Einstellungen stehen im Dokument (RigDaten::rt), hier die
// geladene Quelle, die Charaktere und das Ergebnis der letzten Rechnung
// Ein Eintrag der Preset-Bibliothek (data/presets/presets.txt)
struct PresetEintrag {
    std::string kategorie;   // idle, walk, run, combat, other
    std::string name;
    std::string datei;       // absoluter Pfad der BVH
    std::string quelle;      // Urheber und Werk
    std::string lizenz;      // CC0 1.0, CC BY 4.0
    std::string platz;       // Vorgabe fuer den Platz (BOTH_WALK1 ...)
    bool schleife = true;    // Zyklus (Schleife ab Bild 0), sonst einmal (Angriff, Tod ...)
};

struct ModellRetarget {
    GlaAnimation quelle;
    std::string quelleFuer;          // Pfad, zu dem `quelle` gehoert
    std::string quelleFehler;
    std::string importInfo;      // Animations-Import: Format, Achsen, Einheit, fps der Datei ("" = .gla)
    // Preset-Bibliothek (data/presets): Eintraege, gewaehlte Kategorie/Eintrag, Naht des uebertragenen Zyklus
    std::vector<PresetEintrag> bibliothek;
    bool bibliothekGeladen = false;
    std::string bibliothekOrdner;
    int bibliothekKategorie = 0;
    int bibliothekWahl = -1;
    float weltNaht = -1.0F;
    float bibliothekY = -1.0F;   // Lage der Gruppe Bibliothek im Fenster der Rollouts (Inhalt, fuer das Rollen im Selbsttest)
    std::vector<AnimEntry> quelleCfg;
    modell::RtCharakter cq;
    modell::RtCharakter cz;
    std::string czFuer;              // Skelett, zu dem `cz` gehoert
    std::vector<std::vector<BoneMatrix>> welt;        // Ziel je Bild
    std::vector<std::vector<BoneMatrix>> weltQuelle;  // Quelle je Bild (Ueberlagerung)
    modell::RtBericht bericht;
    int stand = -1;                  // Dokument-Stand der Rechnung
    std::string fuer;                // Quelle|Sequenz|Ziel der Rechnung
    bool vorschau = false;           // die Figur zeigt das Ergebnis
    bool spielt = false;
    float bild = 0.0F;               // Abspielort (Bild des Ergebnisses, mit Bruchteil)
    double bildMs = 50.0;            // frameLerp der Quellsequenz
    bool quelleZeigen = true;        // die Quelle als Skelett neben der Figur
    bool oeffnen = false;            // Rollout aufklappen (Aktion)
    bool quelleWaehlen = false;      // Auswahlfenster oeffnen
    bool baeumeOeffnen = false;      // Ketten, Pose und Schritte aufklappen (Handbuch, Aktion)
    int poseKnochen = -1;            // Knochen fuer die Pose von Hand
    char poseName[48] = {};
    // Stufe 5b: Presets
    bool presetOeffnen = false;
    bool presetZug = false;          // Regler wird gezogen (ein Rueckgaengig-Schritt)
    bool reglerZug = false;          // pfix M7: Regler Reach/Zugfeld Beinlaenge wird gezogen (ein Rueckgaengig-Schritt)
    int presetRolloutBild = -1;      // Selbsttest
    modell::GangBefund gang;         // Befund des letzten Zyklus
    int rolloutBild = -1;            // Selbsttest
    int gezeichnet = 0;              // Selbsttest: gezeichnete Knochen der Quelle
    float versatz[3] = {};           // wo die Quelle neben der Figur steht (Handbuch: beide ins Bild)
    int rechnungen = 0;
    double ms = 0.0;                 // Dauer der letzten Rechnung
};

// Modell-Block 1b: UV erstellen (gui/wof/app_modelltab_uv.cpp) - der UV-Editor links neben den Ansichten, die Befehle,
// der Zwischenstand des Arbeitsnetzes (eine Gruppe: die Flaechen, die eine Textur zeichnen, eine LOD-Stufe)
struct ModellUv {
    bool offen = true;            // der UV-Editor steht neben den Ansichten (nur im Bereich UV)
    float breite = 0.0F;          // Breite des Editors (Kante zieht)
    int gruppe = -1;              // Index in die Gruppen (-1: die der Auswahl)
    int ebene = 2;                // modell::wof::UvEbene: 0 Ecke, 1 Kante, 2 Flaeche, 3 Insel
    int werkzeug = 0;             // 0 Auswahl (Q), 1 Verschieben, 2 Drehen, 3 Skalieren
    bool raster = true;
    bool pixelFang = false;
    int hintergrund = 0;          // 0 Textur, 1 Schachbrett, 2 nichts
    bool dehnungZeigen = false;
    bool ueberlappZeigen = true;
    int methode = 1;              // modell::wof::UvMethode - wie WF UvViewOptions::unwrapMethod = 1 (minimale Dehnung)
    int projAchse = 3;            // 0 X, 1 Y, 2 Z, 3 passend
    int packUmfang = 0;           // modell::wof::PackUmfang
    int abstand = 4;
    bool drehen = true;
    bool spiegeln = false;
    float winkel = 60.0F;         // Naehte nach Winkel (WF seamAngle)
    float zoom = 0.9F;            // Anteil der Kachel an der kleineren Seite
    float mitte[2] = {0.5F, 0.5F};
    // Arbeitsnetz und Befunde (neu, wenn sich Dokument, Stufe, Gruppe oder Haut aendern)
    std::string fuer;
    modell::wof::UvNetz netz;
    modell::wof::UvInseln inseln;
    modell::wof::UvBefunde befunde;
    modell::wof::Verzerrung mass;
    std::vector<float> dehnung;
    // Ziehen / modal (G/R/S wie Blender)
    int zug = 0;                  // 0 keins, 1 Verschieben, 2 Drehen, 3 Skalieren
    std::uint64_t zugSchritt = 0; // pfix H1: sein Rueckgaengig-Schritt
    bool modal = false;
    int modalArt = 0;
    bool zugAngelegt = false;
    float zugStart[2]{};
    float zugMitte[2]{};
    modell::Inhalt zugInhalt;
    modell::wof::UvNetz zugNetz;
    bool rahmen = false;
    float rahmenStart[2]{};
    bool hover = false;           // die Maus ist ueber dem Editor
    bool kontextOeffnen = false;
    bool menueOeffnen = false;    // Blender U: das Abwickeln-Menue
    bool ausrichtenOeffnen = false;   // MRESTE: Blender Umschalt+W, das Menue Ausrichten
    // Selbsttest / Handbuch
    int dreieckeGezeichnet = 0;
    int nahtStriche3d = 0;        // Naht-Striche in der 3D-Ansicht im letzten Bild
    int befehle = 0;
    std::string letzterBefehl;
    std::string meldung;
    bool texturDa = false;
    float leinwand[4]{};          // Lage der Leinwand im letzten Bild (x, y, b, h)
    double bildMs = 0.0;          // CPU-Zeit des Editors im letzten Bild
};

// Modell-Block 1c: Texture Painting (gui/app_modelltab_malen.cpp)
[[nodiscard]] bool modellMalenAktiv();       // Bereich Painting mit Modell
void drawModellMalenBereich();               // Rollouts im Befehlsfeld
void modellMalenVorschauAbgleich();          // P19: jedes Bild, in jedem Bereich - bemalte Bilder in neu geladene Stufen
float modellMalEditorBreite(float frei);     // 0: kein 2D-Bild
void drawModellMalEditor(ImVec2 groesse);
void modellMalenMenue();                     // Hauptmenue "Painting"
void modellMalenKontextMenue();
bool modellMalenTasten();
[[nodiscard]] bool modellMalenModal();       // F / Umschalt+F laufen (die Ansicht malt nicht)
bool modellMalenAktionenSuche(const std::string& such);
bool modellMalenAnsicht(const ModellAnsicht& a, ImVec2 maus, bool neu);   // ein Tupfer (neu: Strich beginnt)
bool modellMalenAufnehmen(const ModellAnsicht& a, ImVec2 maus);          // nach S: Farbe unter der Maus
void modellMalenEnde();
void modellMalenUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
bool modellMalenRueckgaengig();
[[nodiscard]] bool modellMalenUngespeichert();   // Pruefung seit rc610 P1: ein bemaltes Bild ist nicht gebaut
bool modellMalenWiederholen();
int modellMalenBauen();                      // Bilder geschrieben, -1 Fehler
void modellStiftDruck(float druck);
// Selbsttest: die Leinwand der gewaehlten Gruppe (laedt sie; nullptr ohne Bild)
[[nodiscard]] const modell::texmalen::Leinwand* modellMalenLeinwand();
[[nodiscard]] int modellMalenAktionZahl();
[[nodiscard]] int modellMalenAktionGruppe(int i);
[[nodiscard]] const char* modellMalenAktionText(int i);
[[nodiscard]] const char* modellMalenAktionHinweis(int i);
[[nodiscard]] const char* modellMalenAktionTaste(int i, bool blender);
void modellMalenAktionAusloesen(int i);

// Modell-Block 1d: Modellieren (gui/wof/app_modelltab_mesh.cpp) - das Arbeitsnetz der gezeichneten Stufe, die Auswahl je
// Ebene (wie 3ds Max: jede Unterebene behaelt ihre), das Werkzeugkaestchen (Vorschau, Uebernehmen / + / Abbrechen)
enum class MeshWerkzeug : int {
    Keins = 0, Extrude, Inset, Bevel, Chamfer, Connect, LoopCut, Weld, Bridge, Detach, Symmetrize, ToQuads, Merge, Create, Slice, Hinge,
};
struct ModellMesh {
    modell::wof::PolyNetz netz;
    std::string fuer;                 // Dokument-Stand und Stufe des Netzes
    modell::wof::PAuswahl auswahl;    // Punkte, Kanten, Vielecke (Nummern des Netzes)
    std::vector<int> punktFolge;      // Punkte in der Reihenfolge des Waehlens (Merge erster/letzter, Target Weld)
    // Werkzeugkaestchen
    MeshWerkzeug werkzeug = MeshWerkzeug::Keins;
    bool kaestchenOffen = false;
    std::uint64_t schritt = 0;        // pfix H1: der Rueckgaengig-Schritt des offenen Kaestchens (Dokument::abbrechen)
    modell::Inhalt basis;             // der Inhalt vor dem Werkzeug
    modell::Inhalt basisMitTeil;      // dazu ein neuer Teil des Plans (Detach, Grundkoerper in einen neuen Teil)
    modell::wof::PolyNetz basisNetz;
    modell::wof::PAuswahl basisAuswahl;
    modell::wof::PolyNetz planNetz;   // nach dem Plan, Betrag 0
    modell::wof::Plan plan;
    bool neuPlanen = true;
    float betrag = 0.0F;
    int art = 0;                      // Extrude/Inset/Bevel: Art; Detach: 0 Element 1 neuer Teil; Merge: Art
    int segmente = 1;
    float profil = 0.5F;              // MRESTE: Chamfer Max Tension / Blender Shape (0,5 Kreis)
    float hoehe = 8.0F;               // Bevel
    float schwelle = 0.0F;            // Weld (0: Vorgabe)
    int verdrehen = 0;                // Bridge
    int achse = 0;                    // Symmetrize, Slice
    float schnittOrt = 0.0F;          // Slice: Lage der Ebene auf der Achse
    float hingeWinkel = 30.0F;        // Hinge: Winkel in Grad
    int hingeNr = 0;                  // Hinge: welche Randkante der Auswahl (hingeKanten)
    int hingeZahl = 0;
    bool quickAn = false;             // QuickSlice: zwei Klicks in einer Ansicht
    bool quickHatVon = false;
    ImVec2 quickVon{};
    bool vonPlus = true;
    modell::wof::QuadParam quad;
    modell::wof::GrundParam grund;
    bool grundNeuerTeil = false;
    // Auswahl-Modi in der Ansicht
    bool cutAn = false;               // Max Cut / Blender Knife: Klicks auf Kanten und Punkte
    modell::wof::SchnittPunkt cutVon;
    bool cutHatVon = false;
    bool zielWeldAn = false;          // Target Weld: Klick auf den Zielpunkt
    int zielQuelle = -1;
    // Requisit in die Karte
    char requisitName[48] = "prop";
    std::string requisitVfs;          // models/map_objects/<karte>/<name>.md3, wenn eingesetzt
    // Meldung und Selbsttest
    std::string meldung;
    int befehle = 0;
    std::string letzterBefehl;
    int kantenGezeichnet = 0;         // Kanten der Ueberlagerung im letzten Bild
    float kaestchenLage[4]{};         // Werkzeugkaestchen im letzten Bild (x, y, b, h)
    bool menueOeffnen = false;        // Blender-Menues (Alt+E, M, X, Strg+F ...) und das Kontextmenue
    int menueArt = 0;
};

// Modell-Block 1c: Texture Painting (gui/app_modelltab_malen.cpp) - Werkzeug, Pinsel, Farbe, Spiegeln, die 2D-Ansicht; die
// Leinwaende (je Bild eine, mit Ebenen, Maske und Rueckgaengig) leben in der .cpp
struct ModellMalen {
    int gruppe = -1;                  // Textur (Flaechen mit demselben Bild), -1: die der Auswahl
    int stufe = 0;                    // Bildstufe des Shaders (0: das Bild der Flaeche)
    int werkzeug = 0;                 // modell::texmalen::Werkzeug
    modell::texmalen::Pinsel pinsel;
    std::array<float, 4> farbe2{0.0F, 0.0F, 0.0F, 1.0F};
    float radiusPx = 24.0F;           // Pinselradius in Bildpunkten des Schirms (3D und 2D)
    bool druckGroesse = false;
    bool druckStaerke = true;
    bool stiftDa = false;             // ein Stift hat Druck gemeldet (WM_POINTER)
    float stiftDruck = 1.0F;
    bool spiegeln = false;
    int spiegelAchse = 0;             // 0 x (wie das Spiegeln der Gewichte), 1 y, 2 z
    int ausbluten = 4;                // Texel ueber die UV-Grenzen (wie das Umbacken in 1b)
    bool editorOffen = true;
    bool uvZeigen = true;
    bool maskeZeigen = true;
    float breite = 0.0F;
    float zoom = 1.0F;
    float versatz[2]{};
    float klonQuelle[2]{};
    bool klonQuelleDa = false;
    float klonVersatz[2]{};
    bool klonErster = false;
    modell::texmalen::Strich strich;
    bool strichLaeuft = false;
    bool strich2d = false;
    float letzteUv[2]{};
    int modalArt = 0;                 // 1 Radius (F), 2 Staerke (Umschalt+F)
    float modalStart = 0.0F;
    float modalWert = 0.0F;
    int modalEndeBild = -1;
    bool aufnehmen = false;           // S: der naechste Klick nimmt die Farbe auf
    char ordner[260] = {};            // Ausgabe ("Fuer das Spiel bauen"), leer: Vorgabe
    float deckVorher = 1.0F;
    // Meldung und Selbsttest
    std::string meldung;
    int befehle = 0;
    std::string letzterBefehl;
    int tupfer = 0;
    int texelGemalt = 0;
    double hochladenMs = 0.0;
    int hochgeladen = 0;
    int ansichtStand = 0;             // REST Punkt 1: zaehlt jede neue Textur der 3D-Ansicht (die Ansichten zeichnen dann neu)
    bool hover = false;
    float leinwand[4]{};
    int dreieckeGezeichnet = 0;
    int gebaut = 0;
    std::string gebautOrdner;
    bool testTupfen = false;          // Selbsttest ptzeit: je Bild ein Tupfer auf die Mitte der aktiven Ansicht
    int maskeTexel = 0;               // P8 (Selbsttest): im letzten Bild fuer "Maske zeigen" gelesene Texel
    int maskeRechtecke = 0;           // P8: im letzten Bild gezeichnete Rechtecke der Maske
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
    // MRESTE: wie Blender (km_weight_paint) - Strg malt umgekehrt, Umschalt glaettet (fuer den ganzen Strich), F/Umschalt+F/
    // Strg+F ziehen Radius, Staerke, Gewicht (wm.radial_control: Klick/Enter uebernimmt, Esc/Rechtsklick bricht ab)
    int strichArt = 0;            // modell::PinselArt des laufenden Strichs
    int gwModal = 0;              // 0 aus, 1 Radius, 2 Staerke, 3 Gewicht
    float gwModalStart = 0.0F;    // Maus x beim Start
    float gwModalWert = 0.0F;     // Wert beim Start (Esc stellt ihn wieder her)
    int gwModalEnde = -1;         // das Bild, in dem ein Klick das Ziehen beendet hat (malt dann nicht)
    bool gwAufnehmenKlick = false;   // "Gewicht aufnehmen" aus Menue/Suche: der naechste Klick in die Ansicht nimmt auf
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
    float orthoWinkel[3] = {0.0F, 90.0F, 0.0F};   // MBKOPF: AnsichtArt::Orthographisch - pitch, yaw, roll (Quake)
    bool einpassenOffen = false;   // MBKOPF: neue Figur - die Perspektive einpassen, sobald die Ansicht ihre Groesse kennt
    int kopfMenueZaehler = 0;      // MBKOPF: Selbsttest - Menues der Kopfzeile, die offen gezeichnet wurden
    // MBKOPF: ViewCube "Aktuelle Ansicht als Home setzen" - gilt bis zur naechsten Figur (in der Vorlage bis zum Programmende)
    bool homeDa = false;
    float homeWinkel[3]{};
    float homeAbstand = 0.0F;
    float homeZiel[3]{};
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
    // 12 gewaehlte Ecken; MBKOPF: 13 Hinweis "Flaeche waehlen", 14 Statistik
    int ueber[15]{};
    bool aktionenSichtbar = false;   // die Suche "Alle Aktionen" ist offen
    // Stufe 2: Speichern, Frage im Programm, animation.cfg, Karten-GLA, Marker
    std::string speicherPfad;     // wohin Strg+S schreibt (nach dem ersten Speichern)
    int gespeichertStand = -1;    // dok.stand() beim letzten Speichern
    bool beendenBeantwortet = false;   // pfix H3: "Verwerfen und beenden?" mit Ja beantwortet
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
    std::string plaetzeFuer;
    int platzWahl = -1;
    int kgStand = 0;
    int ziehtSequenz = -1;        // Selbsttest: zuletzt gezogene Sequenz
    int markerAenderungen = 0;
    int markerGriff = 0;          // 1 Anfang, 2 Ende des Balkens
    AnimEntry markerZug;
    // Stufe 3: Control Rig, Posen, Ghosts, Pfade (app_modelltab_rig.cpp; Schluessel und Zuordnungen im Dokument)
    modell::Rig rig;
    std::string rigFuer;          // GLA + Modell, fuer die das Rig gilt
    int griffWahl = -1;
    std::vector<char> griffVersteckt;   // Auge im Explorer
    bool rigZeigen = true;
    bool uebersichtOffen = false;
    int uebersichtRolle = -1;     // gewaehlter Fleck der Uebersicht (Rolle * 3 + Seite)
    modell::RigPose kopie;
    bool kopieDa = false;
    ModellGeisterEinst geister;
    bool silhouette = false;
    std::vector<ModellPfad> pfade;
    int pfadDavor = 10;
    int pfadDanach = 10;
    // Ziehen eines Griffs (Gizmo) und G/R wie Blender
    bool rigZieht = false;
    std::uint64_t rigSchritt = 0;     // pfix H1: der Rueckgaengig-Schritt des Ziehens
    float rigZiehStart[2]{};
    std::vector<BoneMatrix> rigWeltStart;
    modell::RigSchicht rigSchichtStart;
    int rigZiehBild = 0;
    int rigModal = 0;             // 1 G, 2 R (Blender)
    int rigModalAchse = -1;
    // Selbsttest-Zaehler je Bild
    int griffeGezeichnet = 0;
    int geisterGezeichnet = 0;
    int pfadPunkte = 0;
    int pfadSchluessel = 0;
    std::string rigMeldung;
    // Stufe 3b: Dopesheet und Kurveneditor (app_modelltab_kurven.cpp)
    bool kvOffen = false;
    int kvArt = 0;                // 0 Dopesheet, 1 Kurveneditor
    float kvHoehe = 0.0F;         // unter den Ansichten (0: Vorgabe)
    std::string kvTitel;          // Selbsttest: der Name des Editors im letzten Bild
    bool kvEinpassen = true;
    float kvVon = 0.0F;
    float kvBis = 40.0F;
    float kvYVon = -1.0F;
    float kvYBis = 1.0F;
    float kvScrollY = 0.0F;
    bool kvNurAnimierte = true;
    bool kvNurGewaehlte = true;   // wie Blender (Only Show Selected) und die Spuransicht: die gewaehlten, ohne Wahl alle
    bool kvHenkelZeigen = true;
    std::array<char, 6> kvGliedZu{};
    std::set<int> kvKnochenAuf;
    std::map<int, unsigned> kvKanalAus;   // je Knochen ausgeblendete Kanaele (Bit je Kanal)
    modell::Wahl kvWahl;
    int kvLetzterKnochen = -1;
    std::vector<std::pair<int, modell::RigSchluessel>> kvAblage;
    int kvZiehArt = 0;            // 1 Rahmen, 2 Zeit ziehen (Dopesheet), 3 Schluessel (Kurven), 4 Zeit setzen, 5 Henkel
    float kvZugMaus[2]{};
    modell::RigSchicht kvZugStart;
    modell::Wahl kvZugWahl;
    bool kvZugAngelegt = false;
    std::uint64_t kvZugSchritt = 0;   // pfix H1
    int kvZugKnochen = -1;
    int kvZugIndex = -1;
    int kvZugKanal = -1;
    int kvZugHenkel = 0;
    int kvModal = 0;              // 1 G, 2 S (Blender)
    bool kvModalLaeuft = false;
    int kvModalEndeBild = -1;
    bool kvHover = false;
    bool kvHoverVorher = false;
    int kvRegler = 0;
    int kvReglerWerkzeug = 0;
    float kvReglerWert = 0.0F;
    float kvReglerLo = 0.0F;
    float kvReglerHi = 1.0F;
    int kvReglerText = 0;         // Str
    int kvReglerHinweis = 0;
    modell::RigSchicht kvReglerStart;
    bool kvReglerAngewandt = false;
    std::uint64_t kvReglerSchritt = 0;   // pfix H1
    bool kvReglerOeffnen = false;
    int kvReglerSichtbar = -1;
    bool kvAnimOffset = false;
    char kvPoseName[64] = {};
    int kvPoseWahl = -1;
    modell::ReduzierBericht kvBericht;
    int kvPfadZug = -1;
    int kvPfadBild = 0;
    modell::RigSchicht kvPfadStart;
    bool kvPfadAngelegt = false;
    float kvPfadMaus[2]{};
    float kvPfadOrt[3]{};
    float kvPfadWeg[3]{};
    // Selbsttest
    int kvRauten = 0;
    int kvKurvenPunkte = 0;
    int kvAenderungen = 0;
    int kvGezeichnet = -1;
    double kvZeitMs = 0.0;
    // Stufe 4: IK/FK (app_modelltab_ik.cpp), Reduzieren mit Auswahl, Posen-Datei (app_modelltab_kurven.cpp)
    bool ikAutoEinrasten = true;  // wie 3ds Max "Automatisch einrasten" (HI-Solver): ein Wechsel ohne Sprung
    int ikWahl = -1;              // Glied im Rollout IK/FK (modell::IkArt), -1: aus dem gewaehlten Griff
    int ikZuletztGriff = -2;      // das Rollout geht auf, wenn ein Glied neu gewaehlt wird (wie Max)
    int ikRolloutBild = -1;       // Selbsttest: das letzte Bild, in dem das Rollout IK/FK offen war
    int ikGezeichnet = 0;         // Selbsttest: gezeichnete IK-Ziele je Bild
    int ikZugArt = 0;             // Selbsttest: letzte IK-Bedienung (1 Ziel, 2 Pol, 3 Rolle, 4 Wechsel auf FK, 5 Ziel drehen)
    // Stufe 6: Auto-Physik (app_modelltab_physik.cpp) - die Einstellungen stehen in der Schicht (Dokument), hier die Rechnung
    int physikStand = -1;         // Stand des Dokuments der letzten Rechnung
    std::string physikSequenz;
    float physikBoden = 0.0F;
    modell::PhysikBericht physik; // Schwerpunkt, Kontakte, Stuetzflaeche, Fluege je Bild der Sequenz
    modell::MassenModell physikMassen;
    std::string physikRigFuer;    // fuer welches Rig das Massenmodell gilt
    bool physikZeigen = true;     // Schwerpunkt, Weg und Stuetzflaeche in der Ansicht
    bool physikOeffnen = false;   // das Rollout aufklappen (Aktion)
    bool physikZug = false;       // Regler wird gezogen (ein Rueckgaengig-Schritt)
    bool ikZug = false;           // pfix M7: Regler IK-FK/Rolle/Spitze wird gezogen (ein Rueckgaengig-Schritt)
    int physikRolloutBild = -1;   // Selbsttest
    int physikGezeichnet = 0;     // Selbsttest: gezeichnete Teile der Ueberlagerung je Bild
    int physikRechnungen = 0;     // Selbsttest: Zahl der Rechnungen
    double physikMs = 0.0;        // Dauer der letzten Rechnung
    bool kvRedNurGewaehlte = false;
    bool kvRedSpanne = false;
    int kvRedVon = 0;
    int kvRedBis = 0;
    char posenDatei[64] = {};
    std::string posenOrdnerJetzt; // Selbsttest: der Ordner der letzten Posen-Datei
    ModellRetarget rt;            // Stufe 5
    ModellUv uv;                  // Modell-Block 1b
    ModellMesh mesh;              // Modell-Block 1d
    ModellMalen malen;            // Modell-Block 1c
};

// Modell-Block 1b: UV (gui/wof/app_modelltab_uv.cpp)
void drawModellUvBereich();                 // Rollouts im Befehlsfeld
float modellUvEditorBreite(float frei);     // 0: kein Editor
void drawModellUvEditor(ImVec2 groesse);
void modellUvMenue();                       // Hauptmenue "UV"
void modellUvKontextMenue();                // Rechtsklick in der Ansicht
bool modellUvTasten();
bool modellUvAktionenSuche(const std::string& such);
// REST N6: ein Fehler der Modell-Bibliothek in der Sprache der Oberflaeche (app_modelltab_mb3.cpp)
std::string modellFehlerText(const modell::ModellFehler& f);
std::string modellFehlerText(const std::string& englisch);
std::array<float, 3> modellUvVorn();          // REST M9: die Vorderseite der Figur, mit der die Naehte rechnen
void drawModellUvPopups();
// fuer die 3D-Ansicht: die Naht-Marken der gezeichneten Stufe als Eckenpaare (Teil, Ecke a, Ecke b)
const std::vector<std::array<int, 3>>& modellUvNahtStriche();
[[nodiscard]] int modellUvAktionZahl();
[[nodiscard]] int modellUvAktionGruppe(int i);
[[nodiscard]] const char* modellUvAktionText(int i);
[[nodiscard]] const char* modellUvAktionHinweis(int i);
[[nodiscard]] const char* modellUvAktionTaste(int i, bool blender);
void modellUvAktionAusloesen(int i);
void modellUvNetzAktualisieren();
[[nodiscard]] int modellUvGruppenZahl();   // Texturen (Gruppen) der gezeichneten Stufe
// .md3 im Model-Tab (Modell-Block 1b): lesen ins Dokument und als Modell zeigen; false mit Grund
bool modellMd3Oeffnen(const std::string& bytes, const std::string& name, std::string* fehler);

// Modell-Block 1d: Modellieren (gui/wof/app_modelltab_mesh.cpp)
void drawModellMeshBereich();               // Rollouts im Befehlsfeld
void modellMeshMenue();                     // Hauptmenue "Modellieren"
void modellMeshKontextMenue();              // Rechtsklick in der Ansicht
bool modellMeshTasten();
bool modellMeshAktionenSuche(const std::string& such);
void drawModellMeshPopups();                // Werkzeugkaestchen und Menues
void modellMeshUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
// ein Klick oder Rahmen in der Ansicht im Bereich Modellieren (true: verbraucht)
bool modellMeshKlick(const ModellAnsicht& a, bool klick, ImVec2 start, ImVec2 ende);
void modellMeshNetzAktualisieren();
[[nodiscard]] int modellMeshAktionZahl();
[[nodiscard]] int modellMeshAktionGruppe(int i);
[[nodiscard]] const char* modellMeshAktionText(int i);
[[nodiscard]] const char* modellMeshAktionHinweis(int i);
[[nodiscard]] const char* modellMeshAktionTaste(int i, bool blender);
[[nodiscard]] ImGuiKeyChord modellMeshAktionChord(int i, bool blender);
[[nodiscard]] std::string modellMeshAktionTooltip(int i);
// QuickSlice: die Ebene durch zwei Schirmpunkte der Ansicht und ihre Blickrichtung (zwei Klicks); false ohne Schnitt
bool modellMeshQuickSlice(const ModellAnsicht& a, ImVec2 von, ImVec2 nach);
void modellMeshAktionAusloesen(int i);
bool modellMeshWerkzeugStarten(MeshWerkzeug w);
void modellMeshUebernehmen(bool weiter);
void modellMeshAbbrechen();
// pfix H1: Strg+Z / Strg+Y bei offenem Werkzeugkaestchen, Regler oder Zug - erst dieses abbrechen (true: es war etwas offen)
bool modellOffenesAbbrechen();
void modellFlaechenAbgleichen();   // pfix H2: die Flaechen des gezeichneten Modells nach den Teilen des Dokuments
void modellKvOffenesAbbrechen();   // Kurven-Regler und Zug im Dopesheet/Kurveneditor
void modellUvZugAbbrechen();       // Zug im UV-Editor
void modellMeshVorschau();                  // nach einer Aenderung der Werte des Kaestchens
bool modellNeuesRequisit();                 // ein leeres .md3-Dokument (ein Teil) zum Modellieren
bool modellRequisitInKarte();               // als .md3 in den Import-Ordner und als misc_model in die offene Karte
bool modellRequisitBearbeiten(const std::string& vfsPfad);   // Rueckweg: ein .md3 der Karte im Model-Tab
// fuer die Ansicht (gui/app_modelltab_ansicht.cpp)
bool modellDreieckUnter(const ModellAnsicht& a, ImVec2 maus, int& teil, int& dreieck);
const std::vector<std::vector<std::array<float, 3>>>& modellHaut();
bool modellTeilGezeichnet(int teil);

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
bool modellGewichtAufnehmen(int ansicht, ImVec2 maus);   // MRESTE: Blender Umschalt+X (paint.weight_sample)
bool modellGewichteTasten();                              // MRESTE: Tasten im Bereich Weights wie Blender
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
// Stufe 3 (app_modelltab_rig.cpp): Knochen-Erkennung, Control Rig, Posen, Ghosts, Bewegungspfade
bool modellRigAnlegen();
void modellRigAktualisieren();    // das Rig zur GLA/zum Modell (nach Laden, Zuordnung)
[[nodiscard]] bool modellRigDa();
void drawModellRigBereich();      // Rollouts im Bereich Animation
void drawModellRigUebersicht();   // Fenster "Rig-Uebersicht" (Figur-Schema)
void modellAnimationMenue();      // Menueleiste "Animation" im Model-Tab
void modellRigExplorer(const std::string& such);
void modellRigKontextMenue();     // Untermenue "Rig" im Rechtsklick der Ansicht
[[nodiscard]] int modellRigAktionZahl();
[[nodiscard]] int modellRigAktionGruppe(int i);   // 0 Rig .. 5 IK/FK (Stufe 4)
[[nodiscard]] const char* modellRigAktionText(int i);
[[nodiscard]] const char* modellRigAktionHinweis(int i);
[[nodiscard]] const char* modellRigAktionTaste(int i, bool blender);
void modellRigAktionAusloesen(int i);
void modellRigGriffWaehlen(int griff);
[[nodiscard]] modell::RigSchicht* modellRigSchicht(bool anlegen);   // die Schicht der gewaehlten Sequenz (im Dokument)
[[nodiscard]] const modell::RigSchicht* modellRigSchichtJetzt();
void modellRigNachMal(int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus, const std::vector<BoneFrameOverride>& ueber = {});
[[nodiscard]] bool modellRigGriffWelt(int griff, const std::vector<BoneMatrix>& welt, float ort[3]);
[[nodiscard]] std::vector<gpu::ModellGeist> modellGeister();
[[nodiscard]] bool modellRigGriffMitte(float m[3]);
void modellRigZiehen(int gizmoArt, const float wert[3]);   // Gizmo: 1 verschieben (Einheiten je Weltachse), 2 drehen (Grad je Weltachse)
[[nodiscard]] int modellRigGriffUnter(const ModellAnsicht& a, ImVec2 maus);
void modellRigUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
[[nodiscard]] bool modellAufSchirm(const ModellAnsicht& a, const float p[3], ImVec2& out);
void modellRigZiehenEnde();
bool modellRigModalStarten(int art);
bool modellRigModalTasten();
bool modellPoseKopieren();
bool modellPoseEinfuegen(bool gespiegelt);
bool modellBindepose(bool alle);
bool modellHandpose(modell::Seite seite, int art);   // 0 offen, 1 Faust, 2 Saebelgriff
bool modellAuswahlGegenseite();
bool modellAuswahlEltern();
bool modellAuswahlKinder();
bool modellPfadHinzu();
void modellPfadeLeeren();
bool modellRigBacken();
bool modellRigSchluesselLoeschen();
void modellRigZuordnen(int knochen, modell::Rolle rolle, modell::Seite seite);
bool modellRigTasten();                              // die Tasten der Rig-Aktionen (Bereich Animation)
bool modellRigAktionenSuche(const std::string& such);   // Zeilen in "Alle Aktionen suchen"
[[nodiscard]] bool modellRigGriffAufSchirm(int griff, ImVec2& out);
// Stufe 3b (app_modelltab_kurven.cpp): Dopesheet, Kurveneditor, Animationswerkzeuge, Pfade ziehen
void drawModellAnimEditor();
[[nodiscard]] float modellAnimEditorHoehe(float frei);   // 0, wenn zu
void drawModellAnimEditorUnten(ImVec2 groesse);
void drawModellKurvenBereich();
void modellKurvenMenue();
void modellKurvenKontextMenue();
bool modellKurvenAktionenSuche(const std::string& such);
bool modellKurvenTasten();
[[nodiscard]] int modellKurvenAktionZahl();
[[nodiscard]] const char* modellKurvenAktionText(int i);
[[nodiscard]] const char* modellKurvenAktionHinweis(int i);
[[nodiscard]] const char* modellKurvenAktionTaste(int i, bool blender);
[[nodiscard]] const char* modellKurvenAktionQuelle(int i);
[[nodiscard]] ImGuiKeyChord modellKurvenAktionChord(int i, bool blender);
[[nodiscard]] int modellKurvenAktionGruppe(int i);
[[nodiscard]] const char* modellKurvenGruppeText(int g);
void modellKurvenAktionAusloesen(int i);
[[nodiscard]] std::string modellKurvenAktionSuchText(int i);   // die Zeile in der Suche (mit Gruppe und Taste)
[[nodiscard]] int modellKurvenAktionNach(int text);           // Index zur Kennung (Str), -1 keine
void modellKurvenAnimOffset(const modell::RigSchicht& vorher, modell::RigSchicht& neu, const std::vector<int>& knochen, int bild);
bool modellPfadZiehenStart(const ModellAnsicht& a, ImVec2 maus);
void modellPfadZiehenWeiter(const ModellAnsicht& a);
[[nodiscard]] bool modellPfadZiehtGerade();
void modellPfadTooltip(const ModellAnsicht& a, ImVec2 maus);
[[nodiscard]] bool modellDsRauteAufSchirm(int knochen, int bild, ImVec2& out);
[[nodiscard]] bool modellKePunktAufSchirm(int knochen, int kanal, int bild, float wert, ImVec2& out);
[[nodiscard]] bool modellKvZeileAufSchirm(int art, int glied, ImVec2& out);
[[nodiscard]] float modellKvBildAufSchirm(float bild);
[[nodiscard]] bool modellKvFeldEnthaelt(ImVec2 p);   // im Zeitfeld des Editors (letztes Bild)
[[nodiscard]] bool modellPfadPunktAufSchirm(int pfad, int bild, ImVec2& out);
// Stufe 4 (app_modelltab_ik.cpp): IK/FK, Fussrolle, Einrasten, Space Switching, Pfade an Haenden/Fuessen/Saebel
[[nodiscard]] int modellIkArtFuerGriff(int griff);              // modell::IkArt des Glieds eines Griffs, -1 keins
int modellIkSpur(int art, modell::RigSchicht& s);               // die Spur des Glieds (legt sie aus dem Rig an), -1 ohne
[[nodiscard]] float modellIkAnteil(int art);                    // IK-Anteil am aktuellen Bild
bool modellIkWechseln(int art, bool ik);                        // eingerastet mit Auto-Einrasten (Max) bzw. immer (Blender)
bool modellIkWechselnRoh(int art, bool ik, bool einrasten);
bool modellIkEinrasten(int art, bool zielAufFk);                // true: IK -> FK (Ziel/Pol auf FK), false: FK -> IK
bool modellIkRaum(int art, int wahl);                           // 0 Welt, 1 Becken, 2 Brust, 3 Kopf, 4 andere Hand
bool modellIkSaebelHand();
bool modellIkReglerSetzen(int art, float regler, bool zug = false);               // wie Blender (Rigify IK-FK): 0 IK, 1 FK, ohne Einrasten
bool modellIkRolle(int art, float rolle, float spitze, bool zug = false);
bool modellIkSchluesselLoeschen(int art);
bool modellIkBacken(int art);
// Gizmo / G-R: 0 nicht IK (FK wie bisher), 1 erledigt, 2 weiter mit FK (auf FK gewechselt). `neu` ist die Schicht des Zugs.
int modellIkZiehen(int gizmoArt, const BoneMatrix& dreh, const float weg[3], modell::RigSchicht& neu);
bool modellIkPolWelt(int griff, const std::vector<BoneMatrix>& welt, float ort[3], const modell::RigSchicht* s);
[[nodiscard]] bool modellIkGriffMitte(int griff, float m[3]);
void modellIkUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
void drawModellIkBereich();
// Stufe 5: Retargeting (app_modelltab_retarget.cpp)
bool modellRtQuelleLaden(const std::string& pfad);
// Animations-Import (FBX/glTF/BVH/COLLADA): Datei als Quelle, Vorlage zuordnen, uebertragen und Vorschau abspielen
bool modellRtImportLaden(const std::string& pfad, const std::string& sequenz = {});
bool modellRtImportWaehlen();   // Dateidialog
const std::vector<PresetEintrag>& modellPresetBibliothek();
bool modellPresetBibliothekAuflegen(int i);   // Eintrag i: importieren, uebertragen, Vorschau abspielen
bool modellRtEinstellen(const char* undo, const std::function<void(modell::RetargetDaten&)>& f, bool zug = false);
[[nodiscard]] const modell::RetargetDaten* modellRtDaten();
int modellRtZuordnen(bool alle);
bool modellRtHaltung(bool ziel, modell::RtHaltung h);
bool modellRtUebertragen();
bool modellRtVorschau(bool an);
bool modellRtBacken();
bool modellRtPoseSpeichern(const std::string& name);
bool modellRtPose(std::vector<BoneMatrix>& welt);   // posieren(): die Figur zeigt das Ergebnis (Vorschau)
void modellRtAktualisieren();
void modellRtUhr();   // je Bild: die Vorschau weiter (drawModelPanel)
void modellRtUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
void drawModellRetargetBereich();
void drawModellRtQuelleFenster();
// Stufe 5b: Presets (Generator)
bool modellPresetMessen();
bool modellPresetAuflegen();
void drawModellPresetBereich();
// Stufe 6: Auto-Physik (app_modelltab_physik.cpp)
void modellPhysikAktualisieren();
void modellPhysikUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
void drawModellPhysikBereich();
bool modellPhysikAn(bool an);
bool modellPhysikEinstellen(const char* undo, const std::function<void(modell::PhysikEinstellung&)>& f);
bool modellPhysikFlugHier(bool landung);
bool modellPhysikFlugAutomatisch();
bool modellPhysikKetteHier();
int modellPhysikKettenVorschlag();
[[nodiscard]] const modell::PhysikEinstellung* modellPhysikEinstellung();
bool modellPfadSpitzeHinzu();
[[nodiscard]] bool modellSaebelSpitze(const ModellPfad& p, const std::vector<BoneMatrix>& welt, float ort[3], float* klingenAnfang = nullptr);
// Stufe 4 (app_modelltab_kurven.cpp): Posen-Bibliothek als Datei
[[nodiscard]] std::string modellPosenOrdner();
bool modellPosenSpeichern();
bool modellPosenLaden(const std::string& datei);
// Stufe 2 (app_modelltab_mb3.cpp)
void modellSpeichern(bool unter);
[[nodiscard]] bool modellUngespeichert();
// pfix H3: ungespeichert -> "Verwerfen?" (Ja: danach), sonst gleich danach; beim Beenden dieselbe Frage
void modellVerwerfenDann(std::function<void()> danach);
[[nodiscard]] bool modellBeendenFrage();
void modellBeendenVergessen();
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
[[nodiscard]] std::string modellCfgBefundText(const modell::CfgBefund& x, modell::Zielspiel s);   // Stufe 7
bool modellVorhandeneKartenGla();
bool modellKartenGlaBauen(const std::string& datei, bool trotzdem);
bool modellGlaSpeichern(const std::string& datei);
bool modellCfgSpeichern(const std::string& datei);
void modellGlaSpeichernFrage();
void modellCfgSpeichernFrage();
void modellKartenGlaBauenFrage();
bool modellXsiAufnehmen(const std::string& datei);
std::string modellGlaAufPlatz(GlaAnimation g, const std::string& name, int loopFrames, int fps, const std::string& wunsch = {});
[[nodiscard]] std::string modellZielKopf();
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
void modellAnsichtAchsenVon(AnsichtArt a, float rechts[3], float hoch[3], float blick[3]);   // REST M9 (Selbsttest)
bool modellBmp(const std::string& pfad, int breite, int hoehe, const std::vector<unsigned char>& rgba);
[[nodiscard]] bool modellFlaecheAufSchirm(int flaeche, ImVec2& out);
[[nodiscard]] bool modellPunktAufSchirm(int ansicht, const GlmModel& m, ImVec2& out);
[[nodiscard]] bool modellGizmoGriffOrt(int griff, ImVec2& out);
[[nodiscard]] bool modellGizmoRingOrt(int achse, float grad, ImVec2& out);   // Stufe 3
[[nodiscard]] bool modellAnbauWelt(int anbau, float welt[16]);
[[nodiscard]] bool modellAuswahlMitte(float mitte[3]);

// --- MBKOPF: Kopfzeile der Ansicht (gui/app_modelltab_kopf.cpp), ViewCube und Navigations-Gizmo
// (gui/wof/app_modelltab_viewcube.cpp) -------------------------------------------------------------------------------
[[nodiscard]] ModellSchattierung& modellSch();   // die Schattierung der aktiven Ansicht
// MBRASTER: der Boden unter den tiefsten Punkt der Figur in der Pose; die Hintergrundfarbe der Ansicht (0..1, der Verlauf als
// Mittelwert) fuer die Farben des Rasters und der Gizmo-Kugeln
void modellBodenAufTiefstenPunkt();
// MBRASTER (Abnahme MBKOPF): die Achsenkugeln des Navigations-Gizmos wie Blender gizmo_axis_draw, von hinten nach vorn sortiert
// (i: 0..5 = +X -X +Y -Y +Z -Z, tiefe: zur Kamera positiv); `ausgerichtet`: die Achse, entlang der man genau schaut, sonst -1
struct ModellGizmoKugel {
    int i = 0;
    float tiefe = 0.0F;
    ImVec2 ort{};
};
void modellGizmoKugelnZeichnen(ImDrawList* dl, const ModellGizmoKugel* kugeln, ImVec2 mitte, float kugelR, float groesse, int unter, int ausgerichtet);
void modellHintergrundFarbe(float rgb[3]);
void drawModellKopfzeile(int n, ModellAnsicht& a);
void modellAnzeigeKontext();                       // Rechtsklick-Menue: die Menues der Kopfzeile der aktiven Ansicht
bool modellKopfAktionenSuche(const std::string& such);   // "Alle Aktionen suchen": jeder Anzeige-Schalter
[[nodiscard]] const char* modellAnsichtText(AnsichtArt a);
[[nodiscard]] bool modellKopfTreffer(const ModellAnsicht& a, ImVec2 maus);   // Maus auf Kopfzeile, ViewCube oder Gizmo
void drawModellNavigation(int n, ModellAnsicht& a);    // ViewCube (Max) oder Gizmo (Blender), nach dem Bild der Ansicht
[[nodiscard]] bool modellNavigationSichtbar(int n);
// Blickrichtung (pitch, yaw, roll wie Camera::angles) der Ansicht n; Ziel: `art` (eine Achsenansicht, Perspektive oder
// Orthographisch mit `winkel`), weich wie 3ds Max (ViewCube "animated transitions") und Blender (Smooth View).
void modellAnsichtWinkel(const ModellAnsicht& a, float winkel[3]);
void modellBlickSetzen(int n, AnsichtArt art, const float winkel[3], bool weich);
void modellAnsichtAnimieren(int n);
void modellStartansicht(int n);              // die Figur von vorne, mittig, Perspektive - sofort (Laden, Zuruecksetzen)
void modellHomeFlug(int n);                  // Home: die Kamera fliegt zur Home-Ansicht (gesetzt oder die Startansicht)
void modellHomeSetzen(int n);                // die Ansicht n ist ab jetzt Home
void modellRollenSchritt(float grad);         // Blender Umschalt+Ziffernblock 4/6
void modellGegenseite();                      // Blender Ziffernblock 9
void modellAnsichtSetzenIn(int n, AnsichtArt a);   // eine Ansicht der Kopfzeile (Menue [Ansicht]) - wie die Tasten
void modellAnsichtEinpassenIn(int n);
void modellAllesEinpassenIn(int n);            // Blender Pos1 (view3d.view_all): die ganze Figur, die Richtung bleibt
// "Alle Aktionen suchen": diese Taste steht dort schon als Anzeige-Schalter (mit Haken und Ort) - nicht doppelt
[[nodiscard]] bool modellKopfSucheHatTaste(ImGuiKeyChord chord);
[[nodiscard]] float modellBlickDauer();       // Sekunden eines weichen Wechsels
// Selbsttest
[[nodiscard]] int modellKopfSchalterZahl();
[[nodiscard]] const char* modellKopfSchalterText(int i);
[[nodiscard]] const char* modellKopfSchalterHinweis(int i);
[[nodiscard]] bool& modellKopfSchalterWert(int i, int ansicht);
[[nodiscard]] bool modellKopfSchalterGeht(int i);
[[nodiscard]] bool modellKopfSchalterHier(int i, int ansicht);   // MBRASTER: im Bedienmodus und in dieser Ansicht sichtbar
[[nodiscard]] bool modellKopfSchalterBrauchtAuswahl(int i);
[[nodiscard]] bool modellKopfFeldOrt(int ansicht, int feld, ImVec2& out);   // 0 [+], 1 [Ansicht], 2 [Schattierung], 3 Overlays, 4 Shading-Menue
[[nodiscard]] bool modellWuerfelOrt(int ansicht, int x, int y, int z, ImVec2& out);   // Flaeche/Kante/Ecke (-1/0/1 je Achse); 9,9,9 = Home
[[nodiscard]] bool modellGizmoOrt(int ansicht, int teil, ImVec2& out);   // 0..5 +X -X +Y -Y +Z -Z, 6 Zoom, 7 Verschieben, 8 Persp/Ortho
[[nodiscard]] bool modellKompassOrt(int ansicht, int teil, ImVec2& out);   // 0..3 N O S W, 4 ein Punkt des Rings vor dem Wuerfel
[[nodiscard]] bool modellWuerfelPfeilOrt(int ansicht, int teil, ImVec2& out);   // 0..3 Pfeil oben/unten/links/rechts, 4/5 Rollpfeil rechts/links
[[nodiscard]] float modellWuerfelAlpha(int ansicht);   // Deckkraft des ViewCube im letzten Bild (0: nicht gezeichnet)

}  // namespace bhed::gui
#endif
