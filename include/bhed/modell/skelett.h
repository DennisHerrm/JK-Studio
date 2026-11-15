// modell/skelett.h - ein eigenes Skelett anlegen und ein Modell daran binden (CW-MODELL, 15.11.2026)
//
// Clone-Wars-Kette Teil 3 (Liste des Bruders, Abschnitt 7): "Fuer eigene Skelette (Fahrzeuge, Laeufer wie AT-TE oder AT-RT, Droiden)
// eine eigene Liste" - der Weg Knochen anlegen -> GLA speichern -> GLM daran binden -> Animationen keyen -> speichern, im Model-Tab
// ohne Handarbeit ausserhalb. Vorbild der Bedienung: 3ds Max Bone Tools / CAT (Bones anlegen, Link), Blender Armature (Edit Mode:
// Bone hinzufuegen, Parent; Parent with Empty Groups / Assign).
//
// Ein Skelett ist eine GlaAnimation (bhed/gla.h): je Knochen Name, Eltern, Grundstellung B (Raum des Modells) und ihre Umkehrung, je
// Bild und Knochen die lokale Stellung A als Eintrag im Vorrat (14 Bytes, compressBone wie Carcass/G2C). Ein neuer Knochen steht in
// jedem vorhandenen Bild mit A = Einheit: X(b) = X(p) B(p)^-1 B(b) - er geht starr mit seinem Eltern (gla.h worldMatrices). Die
// Grundstellung eines Knochens ist eine Verschiebung an seinen Ort (Drehung Einheit, wie die Bones von 3ds Max vor dem Ausrichten);
// gedreht wird beim Keyen.
//
// Laden kann das Spiel eine eigene GLA nur im Einzelspieler (NurSp in animcfg.h); der Name im Kopf ist der Ordner
// (models/players/<n>/<n>, kopfNameErwartet).
#ifndef BHED_MODELL_SKELETT_H
#define BHED_MODELL_SKELETT_H

#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"

namespace bhed::modell {

// Ein neues Skelett: ein Knochen model_root im Ursprung (wie die Skelette von Raven), ein Bild (Grundstellung). kopfPfad ohne
// Endung ("models/players/atrt/atrt").
[[nodiscard]] GlaAnimation skelettNeu(const std::string& kopfPfad);

// Ein Knochen an `ort` (Raum des Modells, Grundstellung) unter `eltern` (-1: eine weitere Wurzel). Index oder -1 (Name leer, laenger
// als 63 Zeichen, schon vergeben; Eltern ungueltig; mehr als 255 Knochen - eine Ecke nennt ihren Knochen mit einem Byte).
int knochenAnlegen(GlaAnimation& a, const std::string& name, int eltern, const float ort[3]);
// Die Grundstellung an einen anderen Ort (die Kinder bleiben, wo sie sind; die Bilder behalten ihre lokalen Stellungen).
bool knochenVerschieben(GlaAnimation& a, int knochen, const float ort[3]);
bool knochenUmbenennen(GlaAnimation& a, int knochen, const std::string& name);
// Unter einen anderen Eltern haengen (keine Schleife; -1 = Wurzel).
bool knochenEltern(GlaAnimation& a, int knochen, int eltern);
// Einen Knochen ohne Kinder entfernen; Ecken des Inhalts, die an ihm hingen, gehen an seinen Eltern (oder Knochen 0), die Nummern
// dahinter ruecken nach. false: hat Kinder, ist der einzige Knochen oder ungueltig.
bool knochenEntfernen(GlaAnimation& a, Inhalt& in, int knochen);

// Teile (alle Stufen) starr an einen Knochen: jede Ecke Gewicht 1 (wie Blender "Assign" mit Gewicht 1, 3ds Max Skin mit einem Knochen).
void flaechenAnKnochen(Inhalt& in, const std::vector<int>& teile, int knochen);
// Den Inhalt an das Skelett binden: Pfad der GLA im Kopf, Knochenzahl; Verweise auf Knochen, die es nicht gibt, gehen an Knochen 0.
void modellAnSkelett(Inhalt& in, const GlaAnimation& a);
// Ein Bild hinten anhaengen: je Knochen die lokale Stellung A (Einheit = Grundstellung). false: Zahl der Matrizen passt nicht.
bool bildAnhaengen(GlaAnimation& a, const std::vector<BoneMatrix>& lokal);

}  // namespace bhed::modell
#endif
