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
#include "bhed/modell/posiert.h"
#include "bhed/modell/wof/poly.h"
#include "bhed/modell/wof/uv.h"
#include "bhed/modell/malen.h"
#include "bhed/modell/stapel.h"

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
    int gangWahl = -1;           // ENT2: gewaehlter Gang des Generators in der Preset-Liste (modell::GangArt: 0 Gehen, 1 Laufen), -1 keiner
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
    float nahtDicke3d = 0.0F;     // ... und ihre Strichstaerke in Bildpunkten (modellUvNahtDicke)
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
// MBMAL (Selbsttest): der Verlauf aller Leinwaende - Bytes der Schritte, Schritte, gesicherte Kacheln
struct MalVerlaufStand {
    long long bytes = 0;
    int schritte = 0;
    int kacheln = 0;
};
[[nodiscard]] MalVerlaufStand modellMalenVerlaufStand();
// MBMAL Punkt 5 (Selbsttest ptsprung): die seit `vorher` (Ergebnis der Leinwand) geaenderten Texel der gewaehlten Leinwand
// am Schirm der Ansicht - je Texel die Stelle in der Pose (jedes Dreieck, das ihn in UV bedeckt; das naechste zum Pfad zaehlt)
// und ihr Abstand zur Linie `pfad` (Mauspositionen am Schirm)
struct MalPfadPruefung {
    int geaendert = 0;        // geaenderte Texel in einem Dreieck
    int ausserhalb = 0;       // davon weiter als die Grenze vom Pfad
    int ohneDreieck = 0;      // geaenderte Texel ausserhalb aller Dreiecke (Ausbluten)
    float weitestePx = 0.0F;  // der weiteste geaenderte Texel
    std::string flaechenAusserhalb;   // die Flaechen der Texel weiter als die Grenze (Namen, je Flaeche die Zahl)
};
[[nodiscard]] MalPfadPruefung modellMalenPfadPruefen(const ModellAnsicht& a, const std::vector<std::uint8_t>& vorher, const std::vector<ImVec2>& pfad,
                                                     float grenzePx);
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
    // mbvers: geplant in der Pose der Ansicht (planNetz gezeigt) - die Orte der Bindepose und die Pose zum Zurueckrechnen
    modell::wof::NetzPose planPose;
    std::vector<BoneMatrix> planKnochen;
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
    modell::texmalen::Strich strich;  // 2D-Bild: Stellen in UV; 3D-Ansicht (MBMAL Punkt 5): Mauspositionen am Schirm
    bool strichLaeuft = false;
    bool strich2d = false;
    float letzteUv[2]{};
    bool uvDavor = false;             // MBMAL Punkt 5: der vorige Tupfer des Strichs in 3D traf das Bild (letzteUv gilt)
    float letzterOrt[3]{};            // MBMAL (Messung): der vorige Tupfer in 3D in der Ruhelage
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
    // MBNAV: Navigationsmodus wie die Knoepfe der Ansichtssteuerung von 3ds Max (linke Taste, bis Rechtsklick/Esc):
    // 0 keiner, 1 Zoom (Alt+Z), 2 Schieben (Strg+P), 3 Orbit (Strg+R), 4 Bereich (Strg+W, Blender Umschalt+B)
    int navModus = 0;
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
    // mbvers: die Pose (Knochenmatrizen der Ansicht), in der die letzte Umformung gerechnet wurde - leer ohne Skelett und in
    // der exakten Bindepose (modellBearbeitungsPose)
    std::vector<BoneMatrix> umformKnochen;
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
// ENTSCHEID1: alle gezeichneten Flaechen unter der Maus, die vorderste zuerst (jede einmal, auch eingefrorene).
[[nodiscard]] std::vector<int> modellFlaechenUnter(const ModellAnsicht& a, ImVec2 maus);
// ENTSCHEID1 (Seitenleiste): die Auswahl ausgehend von `vorher` umformen wie das Gizmo (art 1 Weg, 3 Faktoren je Achse, um `mitte`),
// ohne neuen Rueckgaengig-Schritt (den legt der Aufrufer beim ersten Aendern an).
void modellAuswahlUmformen(const modell::Inhalt& vorher, int art, const float mitte[3], const float wert[3]);
const std::vector<std::vector<std::array<float, 3>>>& modellHaut();
// mbvers: die Pose, in der dieses Bild bearbeitet wird: die Knochenmatrizen der Ansicht; leer ohne Skelett und wenn die Pose exakt die
// Bindepose ist (modell::bindepose) - dann ist das Gezeigte die Bindepose, und alles rechnet wie ohne Skelett
const std::vector<BoneMatrix>& modellBearbeitungsPose();
bool modellTeilGezeichnet(int teil);

// Fuer den Selbsttest und die Leiste (gui/app_modelltab.cpp)
void modellDokumentLaden(const std::string& bytes);
void modellAnzeigeAktualisieren();
void drawModellBereiche();
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
// ENTSCHEID1 (Seitenleiste): ein Zug mit dieser Drehung in Weltachsen um das Gelenk (wie modellRigZiehen 2, ein Rueckgaengig-Schritt je
// Ziehen) und ob der gewaehlte Griff eine Art Zug kann (1 verschieben, 2 drehen; sonst die Meldung in `grund`).
void modellRigDrehen(const BoneMatrix& dreh);
[[nodiscard]] bool modellRigZugMoeglich(int art, std::string* grund);
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
// ENT2: den erzeugten Gang (modell::GangArt: 0 Gehen, 1 Laufen) in der Preset-Liste waehlen - Kategorie Gehen bzw. Laufen, Tempo
// und Armschwung unter der Liste; eine andere Art setzt das Tempo der NPCs und misst die Figur (wie bisher die Wahl im Generator)
void modellPresetGangWaehlen(int art);
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
void modellMarkerZeile();   // die Marker der Primaersequenz (holt sie nach dem Abgleich der animation.cfg selbst, FIX612 R2)
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

// --- MBNAV: Navigation in den Ansichten wie 3ds Max bzw. Blender (gui/app_modelltab_nav.cpp) ---------------------------
void modellNavVorBild(int n);                                   // je Bild: weiche Wechsel, Verlauf der Ansichtsaenderungen
bool modellNavMaus(int n, ModellAnsicht& a, bool ueber);        // Rad, Mitteltaste, Modi; true: links/rechts gehoeren der Navigation
void modellNavUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl);
void modellNavModusSetzen(int m);                               // 1 Zoom, 2 Schieben, 3 Orbit, 4 Bereich (ein/aus)
void modellNavBlBereich();                                      // Blender Umschalt+B: ein Bereich
bool modellNavTasten();                                         // Esc beendet einen Modus
void modellNavZoomSchritt(float rasten);                        // [ ] / Ziffernblock + -
void modellNavZoom2x(bool heran);
void modellNavGrenzen(int art);                                 // 0 alle Ansichten + Auswahl (Z), 1 aktive + alles, 2 alle + alles
void modellNavZumZeiger();                                      // I
bool modellNavRueck(bool wieder);                               // Umschalt+Z / Umschalt+Y
void modellNavAnsichtenMenue();                                 // V
void modellNavKuchen();                                         // Blender `
void modellNavKreisenPx(int n, ImVec2 d, bool beginn);          // ViewCube, Gizmo
void modellNavSchiebenPx(int n, ImVec2 d);
void modellNavBlZoomZug(int n, bool beginn, float dyGesamt);    // Zoom-Knopf des Gizmos
void modellNavBlPerspOrtho();
void modellNavBlKreisen(int richtung);                          // 0 links, 1 rechts, 2 hoch, 3 runter
void modellNavBlRollen(bool links);
void modellNavBlSchieben(int richtung);
void modellNavBlAchse(AnsichtArt art);
bool modellNavBlEinpassen(bool nurAuswahl, bool mitUrsprung);
void modellNavBlVier();
[[nodiscard]] int modellNavVierPlatz(int platz);                // welche Ansicht an welchem Platz der Vierer-Ansicht steht
void modellNavMenue(int n);
bool modellNavAktionenSuche(const std::string& such);
void drawModellNavPopups();
void modellNavZuruecksetzen();
// gui/app_modelltab_ansicht.cpp
bool modellGrenzen(bool nurAuswahl, float mn[3], float mx[3]);  // der Kasten der Auswahl bzw. der gezeichneten Flaechen in der Pose
void modellAnsichtFertig(int n);                                // ein laufender weicher Wechsel endet sofort an seinem Ziel

// --- AP2 (MBENTWURF Teil B): Szenen-Explorer (wie 3ds Max) / Outliner (wie Blender) - gui/app_modelltab_explorer.cpp -----------
// Ein Baum fuer alle Bereiche: Modell, Flaechen (Hierarchie), Tags mit Anbauten, Knochen, Rig-Griffe, Sequenzen; umschaltbar
// "Nach Hierarchie / Nach Ebenen" (Blender: Hierarchie / Sammlungen). Auge = nur in der Ansicht ausblenden, Schneeflocke
// (Blender: Pfeil "waehlbar") = einfrieren: sichtbar, aber in Baum und Ansicht nicht waehlbar.
enum class ExplorerArt : int {
    Modell = 0, KopfFlaechen, KopfTags, KopfKnochen, KopfRig, KopfSequenzen, Ebene, Flaeche, Tag, Knochen, Anbau, Griff, Sequenz, SeqFilter, Hinweis
};
// Die Arten des Filters (Reihenfolge der Symbolreihe).
enum class ExplorerFilter : int { Flaechen = 0, Kappen, Tags, Knochen, Sequenzen, Anbauten, Griffe, Zahl };
// Die Ebenen (Blender: Sammlungen); weitere Detailstufen folgen als Lod2 + n.
enum class ExplorerEbene : int { Koerper = 0, Kappen, Tags, Knochen, Anbauten, Lod2, Lod3 };

// Die Lage einer gezeichneten Zeile (nur fuer den Selbsttest gefuellt): linke Kanten der Teile, -1 = fehlt in dieser Zeile.
struct ExplorerLage {
    ExplorerArt art = ExplorerArt::Modell;
    int index = -1;            // Teil, Knochen, Sequenz, Griff, Ebene oder Anbau
    int tiefe = 0;
    std::string name;          // wie in der Zeile (ohne Zusaetze)
    float xPfeil = -1.0F;
    float xAuge = -1.0F;
    float xSymbol = -1.0F;
    float xName = -1.0F;
    float xNameEnde = -1.0F;   // rechter Rand des gezeichneten Namens
    float xFrost = -1.0F;
    float xZelleEnde = 0.0F;   // rechter Rand der Namensspalte
    float y0 = 0.0F;
    float y1 = 0.0F;
    bool gekuerzt = false;     // der Name passte nicht (endet mit ...)
    bool versteckt = false;
    bool eingefroren = false;
    bool gewaehlt = false;
    bool offen = false;
    bool kinder = false;
    int marke = 0;             // Sequenz: 1 primaer, 2 sekundaer, +4 Warnung; Flaeche: 0 an, 1 aus, 2 aus ohne Kinder, +8 Wurzel
};

struct ModellExplorerZustand {
    int art = 0;                    // 0 nach Hierarchie, 1 nach Ebenen
    bool filterAus[static_cast<int>(ExplorerFilter::Zahl)]{};
    bool knochenFlach = false;
    int sortierung = 0;             // 0 Reihenfolge der Hierarchie, 1 Name aufsteigend, 2 Name absteigend, 3 Eingefrorene zuerst
    bool torsoSekundaer = false;    // Klick auf TORSO_... spielt als Sekundaersequenz (ModView)
    int seqGruppe = 0;              // 0 alle, 1 BOTH_, 2 TORSO_, 3 LEGS_, 4 FACE_
    bool spalteWaehlbar = true;     // wie Blender: die Spalte "waehlbar" zeigen
    std::vector<char> frostTeil;    // je Teil (Flaechen und Tags)
    std::vector<char> frostKnochen;
    bool frostAnbauten = false;
    bool infoOffen = false;         // das Fenster "Info" ist offen
    std::string infoText;
    // --- Selbsttest (gefuellt, solange das Elementverzeichnis mitschreibt) ---
    std::vector<ExplorerLage> lage; // die gezeichneten Zeilen dieses Bildes
    int zeilen = 0;                 // alle Zeilen des Baums (auch die ausserhalb des Sichtbaren)
    float einzug = 0.0F;            // Einzug je Ebene in Bildpunkten
    bool koepfe = false;            // Spaltenkoepfe gezeichnet
    bool filterReihe = false;       // die Symbolreihe des Filters (wie 3ds Max) ist gezeichnet
    bool fussEbenen = false;        // der Satz zu Auge und Schneeflocke ist gezeichnet
    bool fussHinweis = false;       // die Hinweiszeile ist gezeichnet
    std::string tooltip;            // der Tooltip der Zeile unter der Maus in diesem Bild (Name und Info)
};

[[nodiscard]] ModellExplorerZustand& modellExplorer();
[[nodiscard]] const char* modellExplorerTitel();            // "Scene Explorer" bzw. "Outliner" (uebersetzt)
// Die Knoten ueber der Zeile aufklappen und sie ins Bild rollen (index: Teil, Knochen, Sequenz, Griff; bei Koepfen egal).
void modellExplorerZeigen(ExplorerArt art, int index);
void modellExplorerAufklappen(ExplorerArt art, int index, bool offen);   // Ebene: index = ExplorerEbene
void modellExplorerKnochenAlleAuf(bool offen);
void modellExplorerVorgabe();        // Art, Filter, Sortierung, Knoten und Einfrieren wie nach dem Laden (Kappen eingefroren)
void modellExplorerSucheFokus();     // H: in das Suchfeld
[[nodiscard]] bool modellTeilEingefroren(int teil);
[[nodiscard]] bool modellKnochenEingefroren(int knochen);
void modellTeilEinfrieren(int teil, bool an, bool samtKindern);
void modellKnochenEinfrieren(int knochen, bool an, bool samtKindern);
void modellEbeneEinfrieren(ExplorerEbene e, bool an);
void modellEbeneAuge(ExplorerEbene e, bool zeigen);
void modellAllesAuftauen();
// Die Auswahl einer Flaeche in der Ansicht: false und eine Meldung, wenn sie eingefroren ist.
[[nodiscard]] bool modellFlaecheWaehlbar(int teil, bool melden);
bool modellExplorerAktionenSuche(const std::string& such);   // Zeilen in "Alle Aktionen suchen"
[[nodiscard]] int modellExplorerAktionZahl();
[[nodiscard]] const char* modellExplorerAktionText(int i);
void modellExplorerAktionAusloesen(int i);
// Die Griffe des Control Rigs als Zeilen des Baums (gui/app_modelltab_rig.cpp)
enum class UiIcon : int;
[[nodiscard]] int modellRigExplorerZahl();
[[nodiscard]] std::string modellRigExplorerName(int griff);
void modellRigExplorerSymbol(int griff, UiIcon& icon, ImU32& farbe);
[[nodiscard]] std::string modellRigExplorerHinweis(int griff);
void modellRigExplorerKopfMenue();
void modellRigExplorerGriffMenue(int griff);

}  // namespace bhed::gui

// --- AP12: Auto-Skin (gui/app_modelltab_autoskin.cpp; die Gruppe im rechten Feld des Bereichs Weights setzt AP8) -------
#include "bhed/modell/autoskin.h"
namespace bhed::gui {
struct ModellAutoSkin {
    modell::AutoSkinOptionen o;          // ziel und teile setzt der Lauf (Zielspiel des Model-Tabs, Auswahl)
    bool nurAuswahl = false;             // nur die im Szenen-Explorer gewaehlten Flaechen
    char vorlage[128] = "models/players/kyle/model.glm";   // Uebertragen: das Original in den Spieldateien
    bool fensterOffen = false;
    bool fensterSichtbar = false;        // in diesem Bild gezeichnet (Selbsttest)
    // der letzte Lauf
    bool gelaufen = false;
    int ladeStand = -1;                  // Dokument::ladeStand des Laufs - ein anderes Modell hat kein Ergebnis
    std::string fehler;                  // die Meldung, wenn der letzte Lauf scheiterte
    modell::AutoSkinBericht bericht;
    modell::GewichtVergleich vergleich;  // gegen die Gewichte vor dem Lauf
    modell::VerformungsFehler verformung;
    double ms = 0.0;
};
[[nodiscard]] ModellAutoSkin& modellAutoSkinZustand();
bool modellAutoSkin();               // mit den Einstellungen rechnen: ein Rueckgaengig-Schritt, Meldung, Ergebnis
void modellAutoSkinOeffnen();        // das Fenster zeigen
void drawModellAutoSkinGruppe();     // der Inhalt der Gruppe "Automatisch (Auto-Skin)"
void drawModellAutoSkinFenster();
// --- AP1 (MBENTWURF Teil B, 11.11.): die Zeitleiste des Model-Tabs nach dem freigegebenen Entwurf, je Bedienmodus wie
// 3ds Max bzw. wie Blender (gui/app_modelltab_zeit.cpp; der Spurbereich ist das Dopesheet in gui/app_modelltab_kurven.cpp)
// Drei Fassungen je Bereich: kurz (Modellieren, UV, Weights, Painting), Sequenz (Ansehen/Pruefen), voll (Animation).
enum class ZeitFassung : int { Kurz = 0, Sequenz, Voll };
// Was die Leiste im letzten Bild gezeichnet hat (Selbsttest, Fotos): Lage der Teile (x, y, Breite, Hoehe) und Zaehler.
struct ModellZeitProbe {
    int bild = -1;            // ImGui-Bild, in dem die Leiste gezeichnet wurde
    int fassung = -1;         // ZeitFassung
    bool blender = false;
    float band[4]{};
    float griff[4]{};
    float namen[4]{};         // die Namensspalte neben der Skala
    float skala[4]{};
    float spuren[4]{};        // der Spurbereich unter der Skala (Hoehe 0: zu)
    float status[4]{};
    float kopf[4]{};          // das Kaestchen des Abspielkopfs
    float bildX0 = 0.0F;      // Lage des ersten und des letzten Bilds der Sequenz auf der Skala
    float bildX1 = 0.0F;
    int frameStriche = 0;     // feine Striche je Frame in der Skala (wie 3ds Max)
    int spurStriche = 0;      // ... in den Spuren
    int spurZeilen = 0;       // gezeichnete Spuren
    int teilstriche = 0;      // Teilstriche mit Zahl
    int striche = 0;          // Schluessel als Striche (wie 3ds Max): alle, davon rot / gruen / zweifarbig
    int stricheRot = 0;
    int stricheGruen = 0;
    int stricheZwei = 0;
    int rauten = 0;           // Schluessel als Rauten (wie Blender)
    int kopfLinien = 0;       // Linien des aktuellen Bilds (Skala und Spuren)
    int transportZeilen = 0;
    std::string namenText;    // "Selection (track bar)" / "Summary" / leer
    std::string kopfText;     // die Zahl im Kaestchen
    std::string hinweisText;  // die Zeile unter der Leiste
    std::string statusText;   // die Statuszeile (Modus · Tasten · Undo/Suche)
    std::string transport;    // die Knoepfe und Felder der Knopfreihe der Reihe nach, mit | getrennt
};
struct ModellZeit {
    // Auto Key (Max: N) / Auto-Keying (Blender): an = jeder Zug an einem Griff setzt einen Schluessel. ENTSCHEID1: ab Werk aus wie
    // 3ds Max und Blender, beim ersten Zeichnen aus Settings::modellZeitAutoKey (modellZeitAutoKey schreibt die Wahl dorthin).
    bool autoKey = false;
    bool keyModus = false;     // wie 3ds Max "Key Mode": Bild zurueck/vor (auch , .) springen von Schluessel zu Schluessel
    bool sequenzAuf = true;    // Ansehen/Pruefen: die Zeilen Primaer/Sekundaer unter der Skala
    bool konfigOffen = false;  // Fenster Zeitkonfiguration
    bool menueOeffnen = false; // das Menue der Zeitleiste (Knopf, Rechtsklick)
    bool keyingOeffnen = false;   // Blender K: das Keying-Menue
    bool modusOeffnen = false;    // Klick auf den Bedienmodus in der Statuszeile
    // Die Pose ohne Schluessel (Auto Key aus): die Rueckgaengig-Schritte der Zuege seit dem letzten Set Key
    std::vector<std::uint64_t> offen;
    int offenBild = -1;
    std::string offenSequenz;
    int offenStand = -1;
    int zugBild = -10;         // das letzte Bild, in dem ein Zug lief (sein letzter Schritt gehoert noch zur Pose)
    int verworfen = 0;         // Selbsttest: verworfene Posen
    int uebernommen = 0;       // Selbsttest: mit Set Key behaltene Posen
    int spruenge = 0;          // Selbsttest: Spruenge zum vorigen/naechsten Schluessel
    bool griffZieht = false;
    float griffMaus = 0.0F;
    float griffHoehe = 0.0F;
    bool skalaZieht = false;
    bool einstellungenGelesen = false;
    float breite = 0.0F;       // Breite der Leiste im letzten Bild (Umbruch der Knopfreihe)
    int veralteteSequenz = 0;  // Selbsttest: Bilder, in denen die Leiste mit einer Sequenz aus einer schon ersetzten animList zeichnen wollte
    ModellZeitProbe probe;
};
[[nodiscard]] ModellZeit& modellZeit();
[[nodiscard]] ZeitFassung modellZeitFassung();
[[nodiscard]] float modellZeitleisteHoehe();     // Hoehe des Bands unter den Spalten (app_anordnung.inc)
void drawModellZeitleiste();   // die Leiste (drawModelPanel, nach der Uhr); zeichnet mit einer Kopie der Primaersequenz (FIX612 R2)
bool modellZeitTasten();                         // Max: N, Key-Modus; Blender: Pfeil hoch/runter, K, Pos1 im Editor
bool modellZeitAktionenSuche(const std::string& such);   // Zeilen in "Alle Aktionen suchen"
void modellZeitAutoKey(bool an);
void modellZeitZugGemeldet(std::uint64_t schritt);   // ein Zug an einem Griff/Regler hat diesen Rueckgaengig-Schritt angelegt
bool modellZeitOffenUebernehmen();               // Set Key: die Pose ohne Schluessel behalten (false: es gab keine)
bool modellZeitSchluesselSprung(int richtung);   // zum vorigen (-1) / naechsten (+1) Schluessel
// Gemeinsame Masse und Zeichner von Skala und Spuren (Zeitleiste und Dopesheet/Kurveneditor: ein Lineal, Bild 0 buendig)
[[nodiscard]] float modellZeitNamenBreite();
[[nodiscard]] float modellZeitSkalaHoehe();
[[nodiscard]] float modellZeitSpurZeile();           // Hoehe einer Spur
[[nodiscard]] float modellZeitSpurAbstand();         // Abstand zwischen zwei Spuren
[[nodiscard]] float modellZeitSchluesselY(float skalaOben);     // Mitte der Schluessel in der Skala
[[nodiscard]] float modellZeitSchluesselZone(float skalaOben);  // darunter stehen die Zahlen (dort setzt die Maus das Bild)
void modellZeitSkala(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int anzahl, int fps);
void modellZeitNamenText(ImDrawList* dl, ImVec2 skalaLinksOben, const char* text);   // der Name neben der Skala
void modellZeitSpurGrund(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int anzahl);
// art: Bit 0 Bewegung (rot), Bit 1 Drehung (gruen); hoehe = Hoehe des Strichs bzw. Durchmesser der Raute
void modellZeitSchluessel(ImDrawList* dl, float x, float yMitte, float hoehe, unsigned art, bool gewaehlt, bool inSkala);
void modellZeitZeiger(ImDrawList* dl, ImVec2 lo, ImVec2 hi, float von, float bis, int erstes, int bild, float spurenUnten);
// app_modelltab_kurven.cpp: Skala mit den Schluesseln der Auswahl und darunter die Spuren (Dopesheet) oder der Kurveneditor
void drawModellSpurbereich(ImVec2 groesse);
void modellCfgAbgleich();   // app_modelltab_mb3.cpp: die animation.cfg im Dokument zur geladenen GLA (je Bild, aus der Leiste)
[[nodiscard]] std::vector<int> modellKvSchluesselBilder();   // Bilder mit Schluesseln der gezeigten Knochen (aufsteigend)

// --- AP3 (MBENTWURF Teil B, 11.11.): das rechte Feld als Befehlsfeld (wie 3ds Max) bzw. Eigenschaften-Editor (wie Blender)
// mit Reitern, die Kopfzeile des Tabs mit den Bereichen (gui/app_modelltab_feld.cpp). Der Baustein fuer Rahmen und Gruppen
// steht in gui/ui_rollout.h - jede Gruppe des rechten Felds beginnt mit ui::rolloutBeginn(titel, {...}) und nennt ueber
// modellReiter(), in welchem Reiter sie steht.
// Die Reiter in ihrer Reihenfolge: wie 3ds Max (Create, Modify, Hierarchy, Motion, Display, Utilities) und wie Blender
// (Tool, Object, Modifiers, Data, Bone, Constraints, Material).
enum class MbReiter : int { Erstellen = 0, Aendern, Hierarchie, Bewegung, Anzeige, Dienste, Zahl };
enum class MbReiterBl : int { Werkzeug = 0, Objekt, Modifier, Daten, Knochen, Constraints, Material, Zahl };
// Fuer ui::RolloutOpt::reiter: der Reiter einer Gruppe im laufenden Bedienmodus. auchStandard: zusaetzlich im Reiter, den der
// Bereich vorgibt (dort zeigt der Entwurf alle Gruppen des Bereichs untereinander). Mehrere Reiter: mit | verbinden.
//   ui::rolloutBeginn(tr(Str::MbRAnbauten), {.reiter = modellReiter(MbReiter::Hierarchie, MbReiterBl::Constraints)})
[[nodiscard]] unsigned modellReiter(MbReiter max, MbReiterBl blender, bool auchStandard = true);
// Der Reiter, den ein Bereich vorgibt (Entwurf: Ansehen/Pruefen -> Anzeige bzw. Objekt, Animation -> Bewegung bzw. Knochen,
// Modellieren -> Aendern bzw. Modifier, UV und Weights -> Aendern bzw. Daten, Painting -> Aendern bzw. Werkzeug).
[[nodiscard]] MbReiter modellReiterStandard(ModellBereich b);
[[nodiscard]] MbReiterBl modellReiterStandardBl(ModellBereich b);
struct ModellFeld {
    int reiter = -1;            // der gewaehlte Reiter des laufenden Bereichs und Bedienmodus (-1: der vorgegebene)
    int bereich = -1;           // fuer welchen Bereich und Bedienmodus er gilt (ein Wechsel waehlt wieder den vorgegebenen)
    bool blender = false;
    // Selbsttest: was im letzten Bild gezeichnet wurde
    float trennerVon = 0.0F;    // die Trennkante zwischen Ansichten und Feld (x von..bis)
    float trennerBis = 0.0F;
    float kopfVon = 0.0F;       // die Reihe der Bereiche in der Kopfzeile des Tabs (x von..bis, y oben, Breite eines Felds)
    float kopfBis = 0.0F;
    float kopfY = 0.0F;
    float kopfFeld = 0.0F;
    int kopfBild = -1;          // ImGui-Bild, in dem die Kopfzeile gezeichnet wurde
    std::string tippText;       // der Hinweis neben den Bereichen: ganz und wie gezeigt (gekuerzt: der Rest im Tooltip)
    std::string tippGezeigt;
};
[[nodiscard]] ModellFeld& modellFeld();
// Einen Reiter des rechten Felds waehlen (Aktionssuche, Selbsttest); gilt, solange Bereich und Bedienmodus bleiben.
void modellFeldReiter(MbReiter max, MbReiterBl blender);
// Rahmen des rechten Felds: Titelzeile, Reiter des Bedienmodus, rollender Bereich "mtrollouts" (drawModellBefehlsfeld).
void modellFeldBeginn();
void modellFeldEnde();
// Die Breite des Felds in Bildpunkten bei `frei` Bildpunkten fuer Ansichten und Feld (Einstellung modellFeldBreite, Vorgabe 22
// Zeichenhoehen) und die Trennkante links davon (ziehen aendert die Breite, Doppelklick stellt die Vorgabe her).
[[nodiscard]] float modellFeldBreite(float frei);
void modellFeldTrenner(float breite, float hoehe, float frei);
// drawModellBereiche() (oben deklariert) zeichnet seit AP3 die Kopfzeile des Model-Tabs ueber Explorer, Ansichten und rechtem
// Feld: die Bereiche als Reihe gleich breiter Felder und der Hinweis des Bedienmodus fuer den Bereich (app.cpp bereichZeichnen).

// --- AP7 (MBENTWURF Teil B, 11.11.): das rechte Feld des Bereichs UV nach dem freigegebenen Entwurf - Gruppen Auswahl,
// 1 Naehte festlegen, 2 Abwickeln, 3 Aufraeumen, Pruefen ueber den Baustein gui/ui_rollout.h (gui/wof/app_modelltab_uv.cpp).
// Der Zustand des Felds, der nicht zum Arbeitsnetz gehoert (ModellUv oben):
struct ModellUvFeld {
    int schnell = -1;        // Vorlage des Knopfs "Quick unwrap": 0 Figur, 1 Requisit, 2 harte Flaeche; -1 = nach dem Modell
    int projektion = 0;      // Form des Knopfs "Project" in der Reihenfolge des Entwurfs: 0 eben, 1 Zylinder, 2 Kugel, 3 Kasten
    bool nahtDick = false;   // Gruppe Pruefen: die Naehte mit dickem Strich
};
[[nodiscard]] ModellUvFeld& modellUvFeld();
// Die Vorlage, die "Quick unwrap" jetzt nimmt (0 Figur, 1 Requisit, 2 harte Flaeche): die gewaehlte, sonst mit Skelett die
// Figur und ohne das Requisit.
[[nodiscard]] int modellUvSchnellVorlage();
// Die Strichstaerke der Naehte in Bildpunkten: 2, mit "Naehte dick" ein Sechstel der Zeichenhoehe (mindestens 4) - im
// UV-Editor und fuer die Naht-Striche der 3D-Ansicht (app_modelltab_ansicht.cpp, ueberlagerung).
[[nodiscard]] float modellUvNahtDicke();
// --- AP8 (MBENTWURF Teil B, 11.11.): der Bereich Weights nach dem freigegebenen Entwurf (gui/app_modelltab_gewichte.cpp) -
// Rechtes Feld wie 3ds Max (Modifier-Stapel, Gewichts-Werkzeug, Spiegeln, Automatisch, Knochen, JKA-Grenzen) bzw. wie Blender
// (Pinsel, Optionen, Bereinigen, Spiegeln, Automatisch, Knochen, JKA-Grenzen); die Werkzeuge stehen in bhed/modell/gewichte.h.
// app_modelltab_ansicht.cpp: die Ecken der Flaechen im Bild einer Ansicht, in der Pose wie gezeichnet. Je Flaeche (leer:
// nicht gezeichnet) je Ecke: der Ort im Bild, der Ort in der Bildebene ohne Perspektive (rechts, oben - fuer Richtungen wie
// Blender sie nimmt), ob die Ecke im Bild liegt und ob sie zur Kamera zeigt.
struct ModellEckenBild {
    std::vector<std::vector<std::array<float, 2>>> ort;
    std::vector<std::vector<std::array<float, 2>>> ebene;
    std::vector<std::vector<char>> imBild;
    std::vector<std::vector<char>> vorn;
};
struct ModellGewichte {
    bool malen = true;               // Hauptknopf "Gewichte malen" / "Malen": die linke Taste malt; aus: sie waehlt (Ecken fuer das Werkzeug)
    int werkzeug = 0;                // 0 Pinsel (ModellTab::pinselArt), 1 Verwischen (Blender Smear), 2 Verlauf (Blender Gradient)
    int verlaufArt = 0;              // modell::VerlaufArt: 0 linear, 1 radial
    bool autoNormalisieren = true;   // Blender "Auto Normalize", 3ds Max "Normalize": die Summe je Ecke bleibt 1
    float saeubernGrenze = 0.0F;     // Blender Clean "Limit", 3ds Max "Remove Zero Limit" (Vorgabe beider: 0)
    float glaettenFaktor = 0.5F;     // Blender Smooth "Factor" 0,5 und "Iterations" 1
    int glaettenRunden = 1;
    float skalieren = 0.95F;         // 3ds Max Weight Tool "Scale Weight" (Vorgabe 0,95)
    // Spiegeln
    bool spiegelModus = false;       // 3ds Max "Mirror Mode": die Ecken links blau, rechts gruen, ohne Gegenstueck gelb
    bool nurAuswahl = false;         // nur Paare mit einer gewaehlten Ecke ("Mirror Paste" der Auswahl)
    float toleranz = 0.5F;           // "Mirror Thresh": wie weit ein Gegenstueck vom gespiegelten Ort liegen darf (Einheiten)
    bool blGewichte = true;          // Blender "Mirror Weights", "Flip Group Names", "Topology Mirror" (Vorgaben von Blender)
    bool blNamen = true;
    bool blTopologie = false;
    // Sperren je Knochen (Blender: Schloss der Vertexgruppe, K)
    std::vector<char> gesperrt;
    bool sperrMenueOeffnen = false;
    bool sperrMenueSichtbar = false;
    // W10: im Bereich Weights sind die Knochen ab Werk zu sehen - der Schalter "Bones" merkt sich seinen Stand je Bereich
    bool warDrin = false;
    bool knochenInnen = true;
    bool knochenAussen = false;
    // der Spiegelstand der gezeichneten Stufe (Schweissnetz, Partner je Punkt, Befund), gemerkt bis sich etwas aendert
    modell::Schweissnetz netz;
    std::vector<int> partner;
    modell::SpiegelBefund befund;
    int links = 0;                   // +1: die linke Seite der Figur liegt bei x > 0, -1: bei x < 0, 0: nicht bestimmbar
    std::string spiegelSchluessel;
    // der laufende Verlauf (Ziehen in der Ansicht) und die letzte Pinselbewegung (Verwischen)
    bool verlaufZieht = false;
    int verlaufAnsicht = -1;
    float verlaufStart[2]{};
    float verlaufEnde[2]{};
    std::vector<std::vector<modell::GewichtListe>> verlaufVorher;   // die Mallisten beim Beginn des Ziehens (je Teil)
    std::vector<char> verlaufHatte;  // je Teil: hatte beim Beginn schon eine Malliste
    ModellEckenBild verlaufBild;     // die Bildorte der Ecken beim Beginn (wie Blender: vert_cache sco)
    std::uint64_t verlaufSchritt = 0;   // der Rueckgaengig-Schritt des Ziehens (ohne Aenderung wird er verworfen)
    bool verlaufGeaendert = false;
    float bewegung[2]{};             // Pinselbewegung seit dem letzten Tupfer (Bildpunkte, y nach unten)
    // Selbsttest
    int zaehlBild = -1;              // das Bild, fuer das die Zaehler unten gelten
    int spiegelPunkte[4]{};          // im letzten Bild gezeichnet: blau, gruen, gelb, Mitte
    int verlaufStriche = 0;          // gezeichnete Strecken des Verlaufs
    int letzteZahl = 0;              // was der letzte Befehl gezaehlt hat (Ecken bzw. Gewichte)
};
[[nodiscard]] ModellGewichte& modellGewichte();
[[nodiscard]] modell::MalRegeln modellGewichteRegeln();      // Sperren und Auto-Normalisieren fuer jedes Werkzeug
void modellGewichteBereichSchritt();                          // W10: je Bild (Bereichswechsel)
void modellGewichteMalenSetzen(bool an);
void modellGewichteWerkzeug(int werkzeug);                    // 0 Pinsel, 1 Verwischen, 2 Verlauf
// Die gewaehlten Ecken der gezeichneten Stufe: die Unterobjekt-Auswahl, im Objektmodus die Ecken der gewaehlten Flaechen.
[[nodiscard]] modell::EckenMaske modellGewichteAuswahl(bool* da = nullptr);
int modellGewichteWahl(int art);                              // 0 Wachsen, 1 Schrumpfen, 2 Ring, 3 Schleife - Rueckgabe: gewaehlte Ecken
int modellGewichteSkalieren();                                // 3ds Max "Scale Weight" auf die Auswahl
int modellGewichteMischen();                                  // 3ds Max "Blend" auf die Auswahl
int modellGewichteBereinigen(int art);                        // 0 Alles normalisieren, 1 Saeubern, 2 Glaetten, 3 Begrenzen auf 4
int modellGewichteSpiegelnSeite(int richtung);                // +1 Blau -> Gruen (links -> rechts), -1 Gruen -> Blau, 0 Seiten tauschen
int modellGewichteSpiegelnBlender(bool alleGruppen);          // Blender "Mirror Vertex Group"
// Sperren wie Blender object.vertex_group_lock: aktion 0 umschalten, 1 sperren, 2 entsperren, 3 umkehren; maske 0 alle,
// 1 der gewaehlte Knochen, 2 alle anderen, 3 die anderen umgekehrt ("Lock Only Selected" / "Lock Only Unselected")
void modellGewichteSperren(int aktion, int maske);
[[nodiscard]] bool modellGewichtGesperrt(int knochen);
void modellGewichteSpiegelStand();                            // Schweissnetz, Partner und Befund auf den Stand des Dokuments bringen
// Ein Tupfer der Ansicht in das Dokument (modellTupfer): Pinsel bzw. Verwischen mit den Regeln, danach das Spiegeln beim Malen.
void modellGewichteTupferAnwenden(modell::Inhalt& x, const ModellAnsicht& a, const std::vector<std::vector<modell::Tupfer>>& jeTeil, int knochen,
                                  int spiegelKn, int art, float wert);
bool modellGewichteVerlaufSchritt(int ansicht, ImVec2 maus, bool neu);   // das Ziehen des Verlaufs (statt eines Tupfers)
void modellGewichteStrichEnde();
void modellGewichteUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);   // Spiegelmodus (blau/gruen/gelb), Strecke des Verlaufs
bool modellGewichteEigeneTasten();                            // wie Blender: K (Sperren-Menue)
// Die Funktionen des Bereichs fuer "Alle Aktionen suchen", das Rechtsklick-Menue der Ansicht und den Selbsttest
[[nodiscard]] int modellGewichteAktionZahl();
[[nodiscard]] int modellGewichteAktionGruppe(int i);          // 0 Werkzeug, 1 Bereinigen, 2 Spiegeln, 3 Sperren
[[nodiscard]] const char* modellGewichteAktionText(int i);
[[nodiscard]] const char* modellGewichteAktionHinweis(int i);
[[nodiscard]] const char* modellGewichteAktionTaste(int i, bool blender);
void modellGewichteAktionAusloesen(int i);
bool modellGewichteAktionenSuche(const std::string& such);
void modellGewichteKontextMenue();
void modellEckenBild(const ModellAnsicht& a, ModellEckenBild& aus);
// --- AP10 (MBENTWURF Teil B, 11.11.): das rechte Feld im Bereich Animation nach dem freigegebenen Entwurf -------------------
// Die Gruppen in der Reihenfolge des Entwurfs (Animation, Rig erstellen, Presets, Vorhandene Animation bearbeiten, Uebertragen,
// IK/FK, Anbauten und Vorschau, Fuer das Spiel bauen), dahinter, was das Programm mehr kann (Auto-Physik, Pose, Ghosts,
// Bewegungspfade, Kurven und Animationswerkzeuge, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plaetze) - alle ueber den
// Baustein gui/ui_rollout.h. gui/app_modelltab_anim.cpp haelt die Reihenfolge, die Rig-Vorlagen, den Wurzelweg und die Gruppe
// Anbauten und Vorschau; jede andere Gruppe steht in der Datei ihrer Funktion (rig, ik, physik, retarget, kurven, mb3).
struct ModellAnim {
    int vorlage = -2;             // Rig-Vorlage: -2 automatisch (modell::rtVorlageFuer), -1 eigenes Skelett (Ketten vorgeschlagen), sonst Index in modell::rtVorlagen()
    std::string vorlageFuer;      // das Skelett, fuer das die Wahl gilt (ein anderes Skelett: wieder automatisch)
    int erkannt = -1;             // die erkannte Vorlage dieses Skeletts (-1 keine)
    bool generatorOffen = false;  // Presets: der Generator ist aufgeklappt
    std::string wegFuer;          // Wurzelweg: wofuer er gerechnet ist (GLA, Sequenz, Stand des Dokuments)
    float weg = 0.0F;
    bool wegDa = false;
};
[[nodiscard]] ModellAnim& modellAnim();
// Die Gruppen des Bereichs Animation, die drawModellRigBereich zeichnet (Entwurf und Auto-Physik, Pose, Ghosts, Bewegungspfade).
void drawModellAnimFeld();
// Rig-Vorlage: die Wahl fuer modell::rtCharakter (-2 suchen, -1 keine, sonst der Index), die wirksame Vorlage (-1 keine) und
// die erkannte; Setzen legt ein vorhandenes Rig neu an. Die Rollen der Vorlage fuer Knochen, die die Erkennung offen laesst
// oder unklar nennt (modellRigAnlegen stellt sie vor die Zuordnungen von Hand).
[[nodiscard]] int modellAnimVorlageWahl();
[[nodiscard]] int modellAnimVorlage();
[[nodiscard]] int modellAnimVorlageErkannt();
void modellAnimVorlageSetzen(int wahl);
[[nodiscard]] std::vector<modell::Zuordnung> modellAnimVorlageZuordnung();
// Tabelle (Skelett, Massstab, Erkannt als) und Auswahl der Vorlage (Gruppen als Segment, darunter die Vorlagen der Gruppe) -
// der Kopf der Gruppe "Rig erstellen" (app_modelltab_rig.cpp).
void drawModellAnimVorlage();
// "Control Rig erstellen" (nurFk = false; ENT2: eine Sequenz ohne Schicht geht dabei von FK auf IK auf das Rig) bzw.
// "Animation-Rig (nur FK)"; "Auf Rig legen": die Bilder der Sequenz als Schluessel an den Griffen, die Glieder von FK auf IK
// (Anteil 1, Ziel und Pol aus der FK-Pose - die Bewegung bleibt).
bool modellAnimRigErstellen(bool nurFk);
bool modellAnimAufRigLegen();
// Frames zu Schluesseln reduzieren / Animation spiegeln (app_modelltab_kurven.cpp) - fuer die Gruppen und die Werkzeugleiste.
bool modellAnimReduzieren();
bool modellAnimSpiegeln();
// Der Wurzelweg der gewaehlten Sequenz: waagrechter Weg des Beckens vom ersten zum letzten Bild (mit der Schicht), wie Stufe 7
// im Mission-Tab (FaWurzelWeg). false: keine Sequenz oder kein Becken erkannt.
[[nodiscard]] bool modellAnimWurzelweg(float& weg);
// Die Gruppen (jede ein ui::rolloutBeginn): Entwurf A1, A2, A4, A8, A9 und die weiteren des Programms.
void modellAnimGruppeAnimation();        // app_modelltab_mb3.cpp: Name, Ziel-GLA, Plaetze
void modellAnimGruppeRig();              // app_modelltab_rig.cpp: Rig erstellen (Vorlagen)
void modellAnimGruppeBearbeiten();       // app_modelltab_kurven.cpp: Vorhandene Animation bearbeiten
void modellAnimGruppeAnbauten();         // app_modelltab_anim.cpp: Anbauten und Vorschau
void modellAnimGruppeBauen();            // app_modelltab_mb3.cpp: Fuer das Spiel bauen
void modellAnimGruppePose();             // app_modelltab_rig.cpp
void modellAnimGruppeGeister();
void modellAnimGruppePfade();
void modellAnimGruppeKurven();           // app_modelltab_kurven.cpp
void modellAnimGruppePosenBibliothek();
// Die Werkzeuge des Bereichs Animation fuer die Werkzeugleiste wie Blender (Entwurf A11: Pose kopieren, Spiegeln,
// Ausduennen) - AP4 zeichnet die Leiste, hier stehen Symbol, Text, Hinweis und Aktion.
struct ModellAnimWerkzeug {
    UiIcon icon;
    const char* text = "";
    const char* hinweis = "";
    const char* taste = "";      // die Taste im laufenden Bedienmodus (leer: keine)
    bool aktiv = true;
};
[[nodiscard]] int modellAnimWerkzeugZahl();
[[nodiscard]] ModellAnimWerkzeug modellAnimWerkzeug(int i);
bool modellAnimWerkzeugAusloesen(int i);
// Selbsttest: wo in der Gruppe Presets die Wahl des Ziel-Platzes steht (Bildschirm; leer: nicht gezeichnet)
[[nodiscard]] ImRect modellPresetPlatzKasten();

// --- AP4 (MBENTWURF Teil B, 12.11.): der Model-Tab im Bedienmodus wie Blender (gui/app_modelltab_blender.cpp) ----------
// Entwurf (plaene/modellblock_stufe0/entwurf_modelltab.html) und shank 10.11.: "es geht ja nicht nur um controls like blender
// oder 3ds max sondern das aussehen" - wie Blender links die Werkzeugleiste (T) mit Auswaehlen, Bewegen, Drehen, Skalieren und
// den Werkzeugen des Bereichs, eine grosse Ansicht mit dem Modus im Kopf, rechts der Outliner ueber dem Eigenschaften-Editor,
// die Seitenleiste (N). Der Max-Modus bleibt, wie er ist. Jeder Bedienmodus merkt sich seine Fensteranordnung.
struct ModellBlender {
    bool werkzeugleiste = true;     // T (Blender: Toolbar, wm.context_toggle space_data.show_region_toolbar)
    bool seitenleiste = false;      // N (Blender: Sidebar, space_data.show_region_ui)
    int seitenReiter = 0;           // Seitenleiste: 0 Objekt (Item), 1 Ansicht (View)
    float outlinerAnteil = 0.44F;   // Hoehe des Outliners ueber den Eigenschaften (Entwurf: 44 % der Spalte)
    int werkzeug = 0;               // Ansehen/Pruefen: 0 Flaeche waehlen, 1 Messen, 2 Knochen waehlen
    int messPunkte = 0;             // Messen: 0 keiner, 1 der erste, 2 beide
    float messA[3]{};
    float messB[3]{};
    // Selbsttest: was im letzten Bild gezeichnet wurde (Bildschirmpunkte)
    int bild = -1;
    ImRect leiste;                  // die Werkzeugleiste (bzw. der schmale Streifen, wenn sie aus ist)
    ImRect outliner;                // Outliner oben rechts (Titel und Baum)
    ImRect feld;                    // der Eigenschaften-Editor darunter
    ImRect seite;                   // die Seitenleiste
    ImRect modusFeld;               // der Modus im Kopf der aktiven Ansicht
    ImRect hinweis;                 // "N Seitenleiste - T Werkzeuge" unten rechts in der Ansicht
    std::string modusText;          // der Kopf der Ansicht: "Benutzer-Perspektive - Objektmodus"
    std::vector<std::pair<std::string, ImRect>> knoepfe;   // Werkzeugleiste: Text und Rechteck je Knopf
    int trennstrich = -1;           // nach so vielen Knoepfen steht der Trennstrich
    std::string seiteText;          // die Zeilen der Seitenleiste (Beschriftung = Wert, mit | getrennt)
    // ENTSCHEID1: die Zahlenfelder der Seitenleiste (Kennung ort0..2, dreh0..2, masse0..2; Rechteck; eingebbar)
    struct SeitenFeld {
        std::string id;
        ImRect rect;
        bool eingebbar = false;
        bool grau = false;               // der Wert steht in der Farbe inaktiver Felder (nur zum Ablesen)
        std::string beschriftung;        // die gezeichnete Beschriftung links und ihr Platz (Punkte)
        float beschriftungBreite = 0.0F;
        float platz = 0.0F;
    };
    std::vector<SeitenFeld> seitenFelder;
    int seitenSchritte = 0;         // Selbsttest: Rueckgaengig-Schritte, die Eingaben in der Seitenleiste angelegt haben
    // ENTSCHEID1: Werkzeuggruppen wie Blender (operator_menu_hold) und die Namen der Werkzeuge
    std::vector<std::string> gruppen;   // die Texte der Knoepfe, die eine Gruppe sind
    std::vector<ImRect> dreiecke;       // je Gruppenknopf das kleine Dreieck unten rechts
    std::string gruppeOffen;            // der Knopf, dessen Gruppe offen ist (leer: keine)
    float gruppeOffenNach = -1.0F;      // Sekunden zwischen Druecken und Oeffnen der Gruppe
    bool gruppeGezogen = false;         // durch Ziehen geoeffnet (nicht durch Halten)
    std::vector<std::pair<std::string, ImRect>> gruppenEintraege;   // die Eintraege der offenen Gruppe
    bool namenGezeigt = false;          // im letzten Bild standen die Namen neben den Werkzeugen
    int fotos = 0;                  // Werkzeug Foto: geschriebene Bilder
};
[[nodiscard]] ModellBlender& modellBl();
// Die linke Spalte des Model-Tabs (Breite in Zeichenhoehen, Titel des Panels): wie 3ds Max der Szenen-Explorer, wie Blender die
// Werkzeugleiste (aus: ein schmaler Streifen mit dem Pfeil zum Zeigen).
[[nodiscard]] const char* modellLinksTitel();
void drawModellWerkzeugleiste(float hoehe);
// Wie Blender rechts: Outliner (Titel, Baum, Trennkante) - liefert die Hoehe, die dem Eigenschaften-Editor darunter bleibt.
float drawModellBlOutliner(float breite, float hoehe);
// Die Seitenleiste (N) rechts neben den Ansichten: Breite (0: aus oder Max-Modus) und Inhalt.
[[nodiscard]] float modellSeitenleisteBreite(float frei);
void drawModellSeitenleiste(ImVec2 groesse);
// In jeder Ansicht nach ihrem Kopf: der Hinweis "N Seitenleiste - T Werkzeuge" und die Messung.
void modellBlUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl);
// Ein Klick (oder Zug von start nach ende) in der Ansicht mit dem Werkzeug Messen bzw. Knochen waehlen: true = verbraucht.
bool modellBlKlick(const ModellAnsicht& a, bool klick, ImVec2 start, ImVec2 ende);
// Im Kopf der Ansicht (wie Blender): "<Name der Ansicht> - <Modus>" mit dem Modus als Ausklapper; liefert das rechte Ende.
float modellBlModusKopf(ImDrawList* dl, int n, float x, float y, float hoehe, const char* ansichtName);
[[nodiscard]] const char* modellBlModusName(ModellBereich b);
// Tasten (T, N), "Alle Aktionen suchen", Rechtsklick-Menue der Ansicht.
bool modellBlTasten();
bool modellBlAktionenSuche(const std::string& such);
void modellBlKontextMenue();
// Die Werkzeuge der Leiste im laufenden Bereich (Auswaehlen, Bewegen, Drehen, Skalieren und die des Bereichs).
[[nodiscard]] int modellBlWerkzeugZahl();
[[nodiscard]] const char* modellBlWerkzeugText(int i);
[[nodiscard]] bool modellBlWerkzeugAn(int i);
// ENTSCHEID1: die Werkzeuggruppen zeigen wieder ihr erstes Werkzeug (wie nach dem Start).
void modellBlGruppenVorgabe();
bool modellBlWerkzeugAusloesen(int i);
// Die gemessene Strecke in Spieleinheiten (false: noch keine zwei Punkte).
[[nodiscard]] bool modellBlAbstand(float& abstand);

// --- AP5 (MBENTWURF Teil B, 12.11.): Bereich Ansehen/Pruefen vollstaendig nach dem Entwurf (gui/app_modelltab_pruefen.cpp) ------
// Gruppen Boden und Kamera, Auswahl, Szene (.mvs wie ModView, bhed/modell/szene.h), Skins zweispaltig; die fuenf fehlenden
// Anzeige-Schalter von ModView (Alpha, Texturfilter, Knochengewicht-Schwelle, Flaeche hervorheben + Knochenbezuege, nicht
// schattenwerfende Flaechen); der Hinweis "LOD - Dreiecke" in der Ansicht; vier Ansichten je Bereich wie 3ds Max; Overlays und
// Schattierungs-Auswahl wie Blender (Umschalt+Alt+Z, Z).
struct ModellPruefen {
    bool alpha = true;           // Alpha-Blending: an = wie der Shader des Spiels (ModView bUseAlpha), aus = deckend
    bool filter = true;          // Texturfilter: an = wie im Spiel, aus = naechster Bildpunkt (ModView bBilinear)
    bool schwelleAn = false;     // Knochengewicht-Schwelle (ModView bBoneWeightThreshholdingActive)
    float schwelle = 0.05F;      // Anteil 0..1 (ModView fBoneWeightThreshholdPercent 5)
    bool hervorheben = false;    // Flaeche hervorheben + Knochenbezuege (ModView bSurfaceHighlight + ShowsBoneWeighting)
    bool schatten = false;       // nicht schattenwerfende Flaechen (ModView bShowUnshadowableSurfaces)
    bool overlaysAus[4]{};       // je Ansicht: Blender space_data.overlay.show_overlays aus
    int vierJeBereich[kModellBereiche] = {-1, -1, -1, -1, -1, -1};   // wie 3ds Max: die Wahl von Alt+W je Bereich (-1: Vorgabe)
    int letzterBereich = -1;
    int letzteBedienung = -1;
    bool schattierungOeffnen = false;   // Z wie Blender
    // Selbsttest: was das letzte Bild gezeichnet hat
    ImRect lodHinweis;                  // der Hinweis unten rechts in der aktiven Ansicht (leer: nicht gezeichnet)
    std::string lodHinweisText;
    int schattenFlaechen = 0;           // so viele Flaechen bekamen rote Striche
    int bezugKnochen = 0;               // so viele Knochenbezuege der hervorgehobenen Flaeche wurden beschriftet
    int schwelleEcken = 0;              // so viele Ecken haben in der Anzeige Gewichte verloren
    bool schattierungSichtbar = false;
    std::string lodText;                // der Satz unter LOD 1-10 (P2)
    std::string pruefWerte;             // die Zeilen der Pruefung fuer das Spiel (P6): "Name=Wert|..."
};
[[nodiscard]] ModellPruefen& modellPr();
// Die Gruppen von Ansehen/Pruefen aus dieser Datei (Aufruf in drawModellBefehlsfeld) und die Skins in "Detailstufe und Skin".
void modellPrRolloutAuswahl();
void modellPrRolloutBoden();
void modellPrRolloutSzene();
void modellPrHaeute();
// Jedes Bild: vier Ansichten je Bereich wie 3ds Max (R7), die Schattierungs-Auswahl (Z wie Blender).
void modellPrSchritt();
// In jeder Ansicht nach der Ueberlagerung: rote Striche, Knochenbezuege, der Hinweis "LOD - Dreiecke" (Ansehen/Pruefen); n < 0: das
// Foto der aktiven Ansicht (ohne Hinweis, ohne Probe).
void modellPrUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl);
// So viele gezeichnete Flaechen werfen keinen eigenen Schatten (mindestens SHADER_MAX_VERTEXES / 2 Ecken im Zielspiel).
[[nodiscard]] int modellPrSchattenZahl();
// Was das Bild der Ansicht n ausser dem Stand der Ansicht bestimmt (Alpha, Filter, Overlays) - fuer das Neuzeichnen.
[[nodiscard]] std::string modellPrStand(int n);
// Die Knochengewicht-Schwelle auf das gezeichnete Modell (wie r_glm.cpp ModView: ohne Umverteilen); gehoert in den Schluessel.
void modellPrSchwelleAnwenden(GlmModel& m);
[[nodiscard]] std::string modellPrSchwelleSchluessel();
// Die Zahl der Schwelle hinter ihrem Schalter in den Menues der Kopfzeile.
void modellPrSchwelleZeile();
// Die Zeilen der Gruppe "Pruefung fuer das Spiel" nach dem Entwurf: Balken der Flaeche, Flaechen, Sequenzen, gesuchte Tags, Shader,
// LODs, Flaechen ohne eigenen Schatten.
void modellPrPruefZeilen();
// Die Orte der Ecken je Flaeche in der Pose (gui/app_modelltab_ansicht.cpp, wie die Ueberlagerung sie zeichnet).
[[nodiscard]] const std::vector<std::vector<std::array<float, 3>>>& modellHautOrte();
// Szene: als .mvs speichern, ein ModView-Skript lesen (true: gelesen).
bool modellSzeneSpeichern(const std::string& pfad);
bool modellSzeneLesen(const std::string& pfad);
// "Alle Aktionen suchen": Flaechenliste, Overlays, Gruppen von Ansehen/Pruefen.
bool modellPrAktionenSuche(const std::string& such);
// Die Werte der Info einer Zeile des Szenen-Explorers (gui/app_modelltab_explorer.cpp) fuer die Gruppe "Auswahl".
[[nodiscard]] std::string modellExplorerInfo(ExplorerArt art, int index);

// --- AP13S (MBENTWURF Teil B): Modifier-Stapel und Glaetten-Vorschau (gui/app_modelltab_stapel.cpp) -----------------------
// Abgleich M1, M2, W1: je Flaeche ein Stapel (modell::Teil::stapel, bhed/modell/stapel.h) - wie 3ds Max die Gruppe
// "Modifier-Stapel" im Reiter Aendern (Bereiche Modellieren und Weights), wie Blender die Gruppe "Modifier" im Reiter Modifier.
// Jeder Befehl ist ein Rueckgaengig-Schritt des Dokuments.
struct ModellStapel {
    std::uint32_t gewaehlt = 0;          // Kennung des gewaehlten Eintrags (0: keiner - gewaehlt ist die Stufe des Bereichs)
    std::uint32_t kontext = 0;           // der Eintrag des offenen Rechtsklick-Menues
    int zeigen = 0;                      // "Modifier-Stapel zeigen": so viele Bilder lang Reiter waehlen und ins Bild rollen
    // die Zeichnung: das Modell der Ansicht mit den Eintraegen, die rechnen (sonst zeichnet die Ansicht ModellTab::anzeige)
    GlmModel zeichnung;
    bool zeichnungAn = false;
    int zeichnungFuer = -1;              // ModellTab::neuGebaut, zu dem die Zeichnung gehoert
    std::vector<char> ersetzt;           // je Flaeche: ihr Netz kommt aus dem Stapel
    struct TeilStand {
        modell::NetzZahlen vor;                  // das Netz des Dokuments (Stufe der Ansicht)
        std::vector<modell::NetzZahlen> nach;    // je Eintrag: das Netz hinter ihm
        std::vector<int> status;                 // je Eintrag: 0 gerechnet, sonst modell::wof::Ablehnung
    };
    std::vector<TeilStand> stand;        // je Flaeche (leer, solange kein Eintrag rechnet)
    // ein gezogener Regler haelt EINEN Rueckgaengig-Schritt offen (Dokument::vorschau)
    ImGuiID reglerAktiv = 0;
    int reglerStand = -1;
    // Selbsttest: was das Feld im letzten Bild gezeichnet hat und was gerechnet wurde
    int probeBild = -1;
    int probeTeil = -1;
    std::vector<std::string> probeZeilen;        // die Zeilen von oben nach unten
    std::vector<ImRect> probeRechtecke;
    int probeGewaehlt = -1;
    int befehle = 0;                     // ausgefuehrte Befehle am Stapel
    int zeichnungen = 0;                 // so oft wurde die Zeichnung neu gerechnet
    int dateiAuswertungen = 0;           // so oft rechnete der Stapel fuer Speichern oder Export
    // der Inhalt fuer die Datei (nur wenn ein Eintrag in die Datei geht), fuer den Stand des Dokuments `dateiFuer`
    modell::Inhalt dateiInhalt;
    int dateiFuer = -1;
    const void* dateiVon = nullptr;      // das Dokument, zu dem er gehoert
};
[[nodiscard]] ModellStapel& modellStapel();
// Die Gruppe im rechten Feld (Aufruf in den Bereichen Modellieren und Weights).
void drawModellStapelGruppe();
// Die Flaeche, deren Stapel das Feld zeigt: die gewaehlte, sonst die der Auswahl des Modellierens, sonst die einzige, sonst die
// erste mit einem Eintrag (-1: keine).
[[nodiscard]] int modellStapelTeil();
// Was die Ansicht zeichnet: ModellTab::anzeige, solange kein Eintrag rechnet - sonst das Modell mit den ausgerechneten Flaechen.
[[nodiscard]] const GlmModel& modellStapelZeichnung();
[[nodiscard]] bool modellStapelErsetzt(int flaeche);
// Was Speichern und Export schreiben: das Dokument selbst (dieselbe Referenz), solange kein Eintrag in die Datei geht - sonst
// der Inhalt mit den ausgerechneten Flaechen (einmal je Stand des Dokuments gerechnet; gilt bis zur naechsten Aenderung).
[[nodiscard]] const modell::Inhalt& modellStapelDateiInhalt();
// Das Netz, das die Ansicht fuer die Flaeche zeigt (Stufe der Ansicht) - fuer die Pruefung "Vorschau = angewendet".
[[nodiscard]] modell::Netz modellStapelVorschauNetz(int teil);
[[nodiscard]] modell::StapelUmgebung modellStapelUmgebung();
// Befehle (true: ausgefuehrt; sonst steht der Grund in der Meldung). `sichtbar`: -1 in der Liste nach oben, +1 nach unten.
bool modellStapelHinzufuegen(modell::ModArt art);
bool modellStapelEntfernen(std::uint32_t kennung);
bool modellStapelSchalten(std::uint32_t kennung, bool datei);
bool modellStapelBewegen(std::uint32_t kennung, int sichtbar);
bool modellStapelAnwenden(std::uint32_t kennung);   // wie Blender "Apply" (nur dieser), wie 3ds Max "Collapse To" (alle bis hierher)
bool modellStapelAllesAnwenden();
void modellStapelZeigen();                          // Bereich, Reiter und Gruppe ins Bild
// Untermenue fuer den Rechtsklick der Ansicht und das Menue; Zeilen in "Alle Aktionen suchen".
void modellStapelMenue();
bool modellStapelAktionenSuche(const std::string& such);

// FIX612C (Codepruefung seit rc611, P3-5): "Animation-Rig (nur FK)" - der Modus des Rigs (true: ohne IK-Griffe, Hand und Fuss
// greifen ohne IK). Steht im Dokument (modell::RigDaten::nurFk). Setzen ohne Rueckgaengig-Schritt (Selbsttests; die Knoepfe
// gehen ueber modellAnimRigErstellen).
[[nodiscard]] bool modellRigNurFk();
void modellRigNurFkSetzen(bool an);
// FIX612C: Proben fuer den Selbsttest (P3-7 Zeilen des Explorers hinter dem Listenende, P3-6 Bild der Anzeige "Knochen waehlen")
struct Fix612cProbe {
    int explorerHinterEnde = 0;
    int knochenHinweisBild = -1;
};
[[nodiscard]] Fix612cProbe& fix612cProbe();

// FIX612B (Funde der Code-Pruefung P2): der Inhalt, wie die Ansicht ihn zeigt und Speichern ihn schreibt (offene Mallisten in
// einer Kopie uebernommen); haelt ein offener Strich, Zug oder ein Kaestchen die gezeichnete Stufe; einen offenen Pinselstrich,
// Verlauf oder Gizmo-Zug beenden (Strg+Y im Zug) und mit `zuruecknehmen` auch zuruecknehmen (Strg+Z; true: es war etwas offen).
[[nodiscard]] const modell::Inhalt& modellInhaltGezeigt();
[[nodiscard]] bool modellStufeGehalten();
bool modellAnsichtOffenesAbbrechen(bool zuruecknehmen);

// FIX612 P3-4: vier Ansichten je Bereich (wie 3ds Max) abgleichen - jedes Bild und beim Umschalten des Bedienmodus
// (gui/app_modelltab_pruefen.cpp).
void modellPrAnsichtenAbgleich();

// FIX612 (Code-Pruefung seit rc611, P3-1 und P3-3): Zaehler fuer den Selbsttest. Eine Stelle zaehlt, wenn die Oberflaeche in einem
// Bild mit einem Stand weiterrechnen will, den ein Klick desselben Bilds schon ungueltig gemacht hat. Beide bleiben 0.
struct Fix612Probe {
    int presetWahlVeraltet = 0;   // Presets: der gewaehlte Eintrag der Bibliothek gilt nicht mehr (Klick auf einen erzeugten Gang)
    int platzHinterEnde = 0;      // Platzliste der Ziel-GLA: eine Zeile hinter dem Ende der Liste (Leeren bei der eigenen GLA)
};
[[nodiscard]] inline Fix612Probe& fix612Probe() {
    static Fix612Probe p;
    return p;
}

}  // namespace bhed::gui
#endif
