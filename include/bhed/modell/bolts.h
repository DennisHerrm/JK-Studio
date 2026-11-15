// modell/bolts.h - Bolts (Tag-Flaechen "*name") anlegen, ausrichten, an Knochen haengen und pruefen (CW-MODELL, 15.11.2026)
//
// Clone-Wars-Kette Teil 3 (Liste des Bruders, Abschnitt 7): "Bei Fahrzeugen braucht es Sitzplaetze fuer Fahrer und Schuetzen und
// Muendungen fuer Schuesse. Alle benannt und mit ausgegeben."
//
// Ein Bolt einer .glm ist eine Flaeche mit G2SURFACEFLAG_ISBOLT (mdx_format.h) und genau einem Dreieck, dessen Ecken wie jede andere
// Ecke mit dem Skelett gehen. Die Lage liest das Spiel aus dem Dreieck (G2_ProcessSurfaceBolt2, tr_ghoul2.cpp:2080 ff., in
// bhed/glm.h boltMatrix): Seite 0 (Ecke 0 -> 1) die laengste, Seite 2 (Ecke 2 -> 0) die kuerzeste, Ecke 2 der Ursprung; Spalte X
// = Richtung Ecke 2 -> 0, Y = Seite 0 rechtwinklig dazu, Z = -(Seite 0 x Seite 2). boltDreieck baut das Dreieck zu einer gewuenschten
// Lage: Ecke 0 = Ursprung + a X, Ecke 1 = Ursprung + L Y, Ecke 2 = Ursprung (a < L) - so ist Seite 0 die laengste, Seite 2 die kuerzeste
// und die Lage, die das Spiel daraus liest, genau die gewuenschte (ctest modell_bolts).
//
// Die Namen, die das Spiel an einem Fahrzeug sucht (G2API_AddBolt), stehen in boltVorlagen mit Quelle (Datei:Zeile).
#ifndef BHED_MODELL_BOLTS_H
#define BHED_MODELL_BOLTS_H

#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"

namespace bhed::modell {

enum class Zielspiel : std::uint8_t;

enum class BoltGruppe : std::uint8_t { Fahrzeug = 0, Geschuetz, Waffe, Figur };

struct BoltVorlage {
    std::string name;      // "*driver", "*muzzle3"
    BoltGruppe gruppe = BoltGruppe::Fahrzeug;
    const char* quelle = "";   // Datei:Zeile im Spielcode (OpenJK bzw. Movie Duels Update 9)
    bool fahrer = false;       // ein Sitz (Fahrer, Schuetze)
    bool muendung = false;     // eine Muendung
};
// Je Zielspiel: Fahrzeug (CLASS_VEHICLE: *driver, *droidunit, *exhaust1..n, *muzzle1..n mit Rueckfall *flash1..n, *trail1..4),
// Geschuetz (misc_emplaced_gun: *seat, *flash01, *flash02; Kanone *cannonflash), Waffe (*flash), dazu die Tags, die der Spielcode an
// Figuren sucht (gesuchteTags in g2.h). MAX_VEHICLE_MUZZLES / _EXHAUSTS: OpenJK 10 / 4, Movie Duels Update 9 12 / 12.
[[nodiscard]] std::vector<BoltVorlage> boltVorlagen(Zielspiel s);

// Das Dreieck zu einer Lage (Spalten X, Y, Z und Ursprung in m[.][3], Raum des Modells in der Grundstellung). groesse: Laenge der
// kuerzesten Seite (die laengste ist doppelt so lang).
void boltDreieck(const BoneMatrix& lage, float groesse, float aus[3][3]);
// Die Lage, die das Spiel aus dem Dreieck liest (Grundstellung). false: kein Bolt oder entartet.
[[nodiscard]] bool boltLage(const Teil& t, BoneMatrix& lage);

// Einen Bolt anlegen: Teil "*name" mit G2SURFACEFLAG_ISBOLT, ein Dreieck in jeder Stufe, alle Ecken mit Gewicht 1 an `knochen`;
// Eltern die Flaeche, deren Ecken am staerksten an `knochen` haengen (sonst -1). Gibt den Index des Teils oder -1 (Name leer,
// laenger als 63 Zeichen oder schon vergeben, kein Ghoul2-Modell).
int boltAnlegen(Inhalt& in, const std::string& name, int knochen, const BoneMatrix& lage, float groesse = 1.0F);
// Lage setzen (alle Stufen), an einen anderen Knochen haengen, umbenennen. false: kein Bolt / Name ungueltig.
bool boltLageSetzen(Inhalt& in, int teil, const BoneMatrix& lage);
bool boltKnochenSetzen(Inhalt& in, int teil, int knochen);
bool boltUmbenennen(Inhalt& in, int teil, const std::string& name);
// Der Knochen eines Bolts (Gewicht der ersten Ecke), -1 keiner.
[[nodiscard]] int boltKnochen(const Teil& t);
// Ist der Name ein gueltiger, freier Bolt-Name ("*" vorn, 2..63 Zeichen, nicht schon an einer anderen Flaeche - gross/klein egal wie
// G2_IsSurfaceLegal)? ausser: dieser Teil zaehlt nicht mit (Umbenennen).
[[nodiscard]] bool boltNameFrei(const Inhalt& in, const std::string& name, int ausser = -1);

// Pruefung fuer Fahrzeuge (nur wenn das Modell als Fahrzeug markiert ist oder die Clone-Wars-Kette an ist - das entscheidet der
// Aufrufer): ohne *driver kein Fahrer (G2API_AddBolt gibt -1, der Fahrer sitzt im Ursprung), ohne *muzzle1/*flash1 schiesst das
// Fahrzeug aus dem Ursprung.
enum class BoltBefundArt : std::uint8_t { KeinFahrer = 0, KeineMuendung };
[[nodiscard]] std::vector<BoltBefundArt> fahrzeugBoltsPruefen(const Inhalt& in);

}  // namespace bhed::modell
#endif
