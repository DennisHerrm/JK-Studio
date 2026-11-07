// modell/figuranim.h - Modell-Block Stufe 7: Figuren im Mission-Tab animieren und fuer das Spiel bauen
//
// Eine Figur der Szene bekommt eine Schicht (FK-Schluessel, IK-Spuren, Raeume - dieselbe RigSchicht wie im Model-Tab)
// ueber einer Grundanimation ihres Skeletts. "Fuer das Spiel bauen" backt jede Schicht (rigBacken) auf einen Platz
// BOTH_CIN_n der Karten-GLA models/players/_humanoid_<karte>/ (Stufe 2: zusammenfuehren, eine vorhandene Karten-GLA der
// Spieldaten behaelt ihre Plaetze) und schreibt den ICARUS-Block der Figur:
//
//     affect ( "<figur>", FLUSH )
//     {
//         set ( "SET_BEHAVIOR_STATE", "BS_CINEMATIC" );       Kinomodus (wie die Raven-Zwischensequenzen)
//         wait ( <start> );
//         waitsignal ( "<signal>" );                           wartet auf eine andere Figur (optional)
//         set ( "SET_ANIM_BOTH", "BOTH_CIN_n" );
//         set ( "SET_ANIM_HOLDTIME_BOTH", <dauer> );
//         wait ( <dauer> );
//         set ( "SET_ORIGIN", < Start + Wurzelweg > );
//         set ( "SET_ANIM_BOTH", "<Folgeanimation>" );
//         signal ( "<signal>" );                               gibt eine andere Figur frei (optional)
//     }
//
// Wurzelweg und SET_ORIGIN (Stufe 0, nachweise_im_spiel.md, Laeufe e/f/i in MD und im Original-JKA): die Bewegung des
// Beckens in einer Animation wird angezeigt, der Ursprung des NPC wandert aber nicht mit (gemessen: 945 Bildpunkte
// Sprung in MD). Wechselt das Skript danach die Animation, springt die Figur an den Start zurueck. Mit
// SET_ORIGIN im selben Moment wie die Folgeanimation bleibt sie stehen. Q3_SetOrigin (Q3_Interface.cpp:1073 OpenJK, MD9
// gleich) setzt ps.origin (+1 in z), loescht die Geschwindigkeit und haelt pm_time 160 ms. Der Versatz ist der Weg des
// Beckens vom letzten Bild der Animation zum ersten Bild der Folgeanimation (waagrecht, im Raum der Figur), in die Welt
// gedreht wie gpu::baueFigurWelt (Gier + 90 Grad, Groesse aus der .npc) - so steht das Becken nach dem Wechsel dort, wo es
// am Ende der Animation stand; die Hoehe bleibt die des Starts (Pmove setzt die Figur danach auf den Boden).
//
// Die Schicht steht als Kommentarzeilen im Skript, direkt vor dem Block (wie die ROFF-Schluessel, bhed/roffanim.h):
//
//     //(BehavEd) figur "kyle" basis "BOTH_STAND1" bilder 80 fps 20 platz "BOTH_CIN_3" folge "BOTH_STAND1" ...
//     //(BehavEd) figur+ "kyle" k 12 2 0 1 ...         je Knochen seine Schluessel
//     //(BehavEd) figur+ "kyle" ik 1 3 ...             je IK-Spur ihre Schluessel
//     //(BehavEd) figur+ "kyle" fremd 1 "luke" "rhand" 10 30 ...
//
// Damit gehoert die Animation zum Skript: Rueckgaengig, Kopieren und Speichern nehmen sie mit. Das Spiel liest den Kommentar
// nicht; IBIze und JK Studio ueberspringen ihn.
#ifndef BHED_MODELL_FIGURANIM_H
#define BHED_MODELL_FIGURANIM_H

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigdaten.h"
#include "bhed/script.h"

namespace bhed::modell {

// Ein IK-Ziel am Knochen einer anderen Figur (Griff, Uebergabe): von Bild `von` bis `bis` dieser Figur folgt das Ziel
// des Glieds dem Knochen der anderen Figur mit festem Versatz. Gebacken wird das in die GLA dieser Figur.
struct FremdRaum {
    IkArt art = IkArt::ArmRechts;
    std::string figur;
    std::string knochen;
    int von = 0;
    int bis = 0;
    BoneMatrix versatz = BoneMatrix::identity();   // Lage des Ziels im Raum des fremden Knochens (starr)
};

struct FigurAnim {
    std::string figur;                 // affect-Name (NPC_targetname)
    std::string basis = "BOTH_STAND1"; // Grundanimation des Skeletts, unter der Schicht
    int bilder = 60;                   // Laenge in Bildern
    int fps = 20;                      // wie die BOTH_CIN-Zeilen der Raven-Karten-GLAs
    double startMs = 0.0;              // wait vor dem SET_ANIM_BOTH (ab Skriptbeginn)
    std::string platz;                 // BOTH_CIN_n (leer: der naechste freie beim Bauen)
    std::string folge = "BOTH_STAND1"; // Folgeanimation am Ende
    std::string warteSignal;           // waitsignal vor dem Start
    std::string sendeSignal;           // signal am Ende
    bool setOrigin = true;             // SET_ORIGIN mit der Folgeanimation (aus: die Rot-Probe aus Stufe 0)
    bool kino = true;                  // SET_BEHAVIOR_STATE BS_CINEMATIC am Anfang
    RigSchicht schicht;
    std::vector<FremdRaum> fremd;
};

// --- Kommentarzeilen ---------------------------------------------------------------------------------------------------
[[nodiscard]] std::vector<std::string> figurZeilen(const FigurAnim& a);
// Liest die Zeilen einer Figur (Marke zuerst). Die IK-Spuren werden aus dem Rig neu angelegt (ikSpurAnlegen) und bekommen
// ihre Schluessel; ohne Rig (r leer) bleiben sie aus. false: keine Marke.
[[nodiscard]] bool figurAusZeilen(const std::vector<std::string>& zeilen, const GlaAnimation& g, const Rig& r, FigurAnim& aus,
                                  std::string* fehler = nullptr);
[[nodiscard]] bool istFigurMarke(const std::string& zeile, std::string* figur = nullptr);
[[nodiscard]] bool istFigurDaten(const std::string& zeile, std::string* figur = nullptr);

// --- Grundanimation -----------------------------------------------------------------------------------------------------
// Die Bilder [0, bilder) der Grundanimation im Takt der Figur: Bild f zeigt das Bild der Grundanimation zur Zeit
// f * 1000 / fps (frameForAnimationIn: Schleife nach loopFrames, sonst das letzte Bild gehalten, bg_panimate.cpp:4723).
// Skelett, Grundstellung und Vorrat-Eintraege wie die Quelle (Anhang eingeschlossen), gleiche Eintraege geteilt.
[[nodiscard]] bool basisGla(const GlaAnimation& skelett, const std::vector<AnimEntry>& abschnitte, const std::string& basis, int bilder, int fps,
                            GlaAnimation& aus, std::string* fehler = nullptr);
// ms je Bild der Figur (frameLerp wie animBildMs).
[[nodiscard]] double figurBildMs(const FigurAnim& a);
// Dauer der Animation (Haltezeit und wait): Bilder * frameLerp (wie "Ins Skript", Stufe 1a-2).
[[nodiscard]] int figurDauerMs(const FigurAnim& a);

// --- Wurzelweg ----------------------------------------------------------------------------------------------------------
// Der Weg des Beckens (Knochen `becken`) im Raum der Figur: Ort am letzten Bild der Animation (mit Schicht) minus Ort im
// ersten Bild der Folgeanimation (`folgeBild` der Skelett-GLA, ohne Schicht). z = 0 (die Hoehe bleibt).
[[nodiscard]] std::array<float, 3> wurzelVersatz(const GlaAnimation& basis, const RigSchicht& s, int becken, const GlaAnimation& skelett,
                                                 int folgeBild);
// Der Ursprung fuer SET_ORIGIN: start + R * versatz, R die Drehung der Figur in die Welt (fw: 4x4 zeilenweise wie
// gpu::baueFigurWelt samt Groesse).
[[nodiscard]] std::array<float, 3> setOriginOrt(const float start[3], const float fw[16], const std::array<float, 3>& versatz);
// Der Pfad des Beckens ueber alle Bilder in der Welt (fw) - der Weg der Figur.
[[nodiscard]] std::vector<std::array<float, 3>> wurzelPfad(const GlaAnimation& basis, const RigSchicht& s, int becken, const float fw[16]);

// --- Fremde Raeume ------------------------------------------------------------------------------------------------------
// Die Weltlage (Karte) eines Knochens der anderen Figur: ihre Welt fw mal die Weltmatrix des Knochens am Bild.
struct FigurLage {
    const FigurAnim* anim = nullptr;
    const GlaAnimation* basis = nullptr;
    float fw[16]{};
    double startMs = 0.0;   // wann ihr SET_ANIM_BOTH laeuft (aus der Szene)
};
// Die Schicht mit den fremden Raeumen eingerechnet (ikZielSetzen je Bild der Spanne). Die anderen Figuren in `andere`
// nach Name. Gibt die Zahl der gesetzten Ziele.
int fremdeEinrechnen(const GlaAnimation& basis, const FigurLage& selbst, const std::vector<FigurLage>& andere, RigSchicht& s);

// --- ICARUS -------------------------------------------------------------------------------------------------------------
// Der Block der Figur (Marke, Datenzeilen, affect). ursprung: SET_ORIGIN (nur mit setOrigin).
[[nodiscard]] std::vector<Node> figurBlock(const FigurAnim& a, const std::array<float, 3>& ursprung);
// Index der Marke der Figur auf oberster Ebene (danach Datenzeilen und der affect).
[[nodiscard]] std::optional<std::size_t> findeFigurBlock(const Script& s, const std::string& figur);
// Alle Figuren mit Marke im Skript (Namen in Reihenfolge).
[[nodiscard]] std::vector<std::string> figurenImSkript(const Script& s);
// Die Kommentarzeilen des Blocks der Figur (leer: keiner).
[[nodiscard]] std::vector<std::string> figurZeilenImSkript(const Script& s, const std::string& figur);
// Neues Skript mit dem Block (ersetzt den alten oder steht am Anfang); a == nullptr: der Block faellt weg.
[[nodiscard]] Script mitFigurBlock(const Script& s, const std::string& figur, const FigurAnim* a, const std::array<float, 3>& ursprung);

// --- Bauen --------------------------------------------------------------------------------------------------------------
struct FigurBauEingabe {
    const FigurAnim* anim = nullptr;
    const GlaAnimation* basis = nullptr;      // basisGla
    RigSchicht schicht;                       // mit fremden Raeumen (fremdeEinrechnen)
};
struct FigurBauBericht {
    std::vector<std::string> plaetze;         // je Figur der Platz
    int bilder = 0;
    int vorrat = 0;
    int vorhandenBehalten = 0;                // Plaetze der vorhandenen Karten-GLA
    std::string fehler;
};
// Die Plaetze verteilen: der gemerkte Platz einer Figur bleibt (er ist ihrer), sonst der erste freie (nicht in `belegt`,
// nicht an eine andere Figur vergeben). leer: kein Platz frei (fehler sagt es).
[[nodiscard]] std::vector<std::string> plaetzeVerteilen(const std::vector<const FigurAnim*>& figuren, const std::vector<std::string>& belegt,
                                                        Zielspiel s, std::string* fehler = nullptr);
// Die Karten-GLA (bzw. die Plaetze fuer das Ziel): je Figur ihre Schicht gebacken, dazu die Plaetze der vorhandenen
// Karten-GLA (`vorhanden`, deren Zeilen `vorhandenCfg`), die keine Figur belegt. kopfName: Pfad im Kopf.
[[nodiscard]] bool figurenBacken(const std::vector<FigurBauEingabe>& eingaben, const std::vector<std::string>& plaetze, const GlaAnimation* vorhanden,
                                 const std::vector<AnimEntry>& vorhandenCfg, Zielspiel s, std::vector<GlaAnimation>& gebacken,
                                 std::vector<Platz>& aus, FigurBauBericht* bericht = nullptr);

}  // namespace bhed::modell
#endif
