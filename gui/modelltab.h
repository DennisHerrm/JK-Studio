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

#include <string>
#include <vector>

#include "imgui.h"

#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/mapview.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"

namespace bhed::gui {

// Ein angebautes Modell (G2API_AttachG2Model an einen Bolzen, G2API_AddBolt):
// an eine Tag-Flaeche ("*r_hand") oder einen Knochen.
struct ModellAnbau {
    std::string pfad;      // .glm in den Archiven
    GlmModel modell;
    ModelTextures texturen;
    std::string ziel;      // Tag-Flaeche oder Knochen
    bool knochen = false;
};

// Eine der vier Ansichten. Oben, Vorne und Links sind orthographisch, die
// Perspektive ist die Kreiskamera des Model-Tabs (modelCam, modelOrbit).
enum class AnsichtArt : int { Oben = 0, Vorne = 1, Links = 2, Perspektive = 3, Unten = 4 };

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
    // 6 Randecken (Risse), 7 Auswahl-Striche
    int ueber[8]{};
    bool aktionenSichtbar = false;   // die Suche "Alle Aktionen" ist offen
};

// Fuer den Selbsttest und die Leiste (gui/app_modelltab.cpp)
void modellDokumentLaden(const std::string& bytes);
void modellAnzeigeAktualisieren();
void drawModellBereiche();
void modellSequenzZeile(const AnimEntry* e);
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
