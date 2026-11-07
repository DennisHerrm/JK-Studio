// figuranim.h - Modell-Block Stufe 7: der Zustand von "Figur animieren" im Mission-Tab (gui/app_figuranim.cpp)
//
// Die Daten der Animation stehen im Skript (bhed/modell/figuranim.h); hier steht nur, was die Oberflaeche zwischen zwei
// Bildern braucht: welche Figur, welcher Griff, ein laufender Zug, die Auswahl im Dopesheet, das Bauen.
#ifndef BHED_GUI_FIGURANIM_H
#define BHED_GUI_FIGURANIM_H

#include <array>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/figuranim.h"
#include "bhed/modell/rig.h"

namespace bhed::gui {

struct FigurAnimZustand {
    bool offen = false;             // das Fenster
    std::string figur;              // die bearbeitete Figur (affect-Name)
    bool posieren = true;           // Griffe in der Ansicht
    int griffWahl = -1;             // Index in die Griffliste (figurAnimGriffe)
    int gizmo = 2;                  // wie 3ds Max: 0 Q waehlen, 1 W verschieben, 2 E drehen
    int achse = -1;                 // -1 Blickachse, 0..2 X/Y/Z der Karte
    bool autoEinrasten = true;      // 3ds Max "Automatisch einrasten"
    bool zeigeWurzel = true;        // Pfad des Beckens (Weg der Figur)
    bool zeigeGlieder = false;      // Pfade an Haenden und Fuessen
    bool zielKlick = false;
    float letzterTreffer[3]{};      // der Punkt der Flaeche des letzten Klicks (Ziel auf Flaeche)         // der naechste Klick in die Karte setzt das IK-Ziel auf die Flaeche
    // Ein Zug (Griff mit Maus, Blender G/R modal)
    bool zieht = false;
    int zugArt = 0;                 // 1 verschieben, 2 drehen, 3 Pfadpunkt
    int zugPfadBild = -1;
    bool modal = false;             // wie Blender: G/R ohne gedrueckte Taste
    float mausStart[2]{};
    float zugSkala = 1.0F;          // Einheiten je Bildpunkt in der Tiefe des Griffs
    int zugBild = 0;
    modell::RigSchicht schichtStart;
    modell::RigSchicht schichtZug;  // die Schicht waehrend des Zugs (Anzeige: weltErsatz)
    std::vector<BoneMatrix> weltErsatz;
    int ersatzFigur = -1;           // Szene-Index der Figur, deren Knochen weltErsatz ersetzt
    // Dopesheet
    modell::Wahl dsWahl;
    bool dsZieht = false;
    float dsMaus = 0.0F;
    int dsVersatz = 0;
    bool kurvenZeigen = false;
    int dsErste = 0;                // erste sichtbare Zeile
    // Bauen
    int ziel = 0;                   // 0 Karten-GLA, 1 _humanoid.gla (Mod-Ordner), 2 eigene GLA
    char eigenName[64] = {};
    char ordner[260] = {};
    std::vector<std::string> befunde;   // Fragen vor dem Bauen
    bool frage = false;
    std::string meldung;
    bool zeigePhysik = true;        // Stufe 6: Schwerpunkt, Weg und Stuetzflaeche in der Karte
    int physikGezeichnet = 0;       // Selbsttest
    int physikRolloutBild = -1;     // Selbsttest
    bool bauenZeigen = false;       // Stufe 6: nach dem Bauen den Rollout aufklappen und in Sicht rollen (Meldung sichtbar)
    // Aktionssuche (X wie 3ds Max, F3 wie Blender)
    bool sucheOeffnen = false;
    bool sucheSichtbar = false;
    // Rechtsklick in der Ansicht
    bool kontextOeffnen = false;
    bool kontextSichtbar = false;
    // Selbsttest: je Bild gezeichnet
    int griffeGezeichnet = 0;
    int pfadPunkte = 0;
    int rauten = 0;
    int ikRauten = 0;
    int kurvenPunkte = 0;
    std::vector<std::array<float, 4>> rautenOrte;   // Knochen (oder ikWahlCode), Bild, x, y der gezeichneten Rauten
    int aenderungen = 0;
    int nachgezogen = 0;            // SET_ORIGIN nach dem Neuaufbau der Szene angepasst
    double ueberlagerungMs = 0.0;
    // SET_ORIGIN nachziehen: Stand des Skripts nach dem eigenen Schreiben
    std::size_t geschriebenTiefe = static_cast<std::size_t>(-1);
    std::string geschriebenZeilen;
};

}  // namespace bhed::gui
#endif
