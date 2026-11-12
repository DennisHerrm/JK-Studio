// modell/retarget.h - Modell-Block Stufe 5: Animationsuebertragung (Retargeting) zwischen Skeletten und Rig-Vorlagen
//
// Vorbilder nur nach ihrer Doku (kein Code): Unreal IK Retargeter (Retarget-Pose, Ketten, Op-Stapel Becken -> FK -> IK ->
// Wurzel), MotionBuilder/HumanIK (Charakterisieren in der T-Pose, Reach, Plotten). Die Rechnung ist eigene:
//
// Charakter   je Skelett die Ketten (Wirbel, Hals, Kopf, Schluesselbein, Arm, Bein, Finger, Anhang) aus der Knochen-Erkennung
//             (Stufe 3) oder einer Vorlage (JKA _humanoid 53, JK2 72, Mixamo, Biped, CAT und die 34 eigenen Skelette des
//             Basisspiels), dazu Becken, Wurzel (model_root), Bewegungsknochen (Motion) und Drehknochen (...X).
// Retarget-   je Skelett eine Pose ueber der Grundstellung (Bindepose, A = Einheit): je Knochen M wie die Schicht (X'(b) =
// Pose        X'(p) X(p)^-1 X(b) M, BONE_ANGLES_POSTMULT). "Ausrichten" dreht Wirbel und Hals senkrecht, Ober-/Unterarm
//             waagrecht zur Seite (T) oder 45 Grad nach unten (A), Ober-/Unterschenkel senkrecht nach unten - je Glied die
//             kleinste Drehung (Ausgleich der Bindepose). Von Hand: zusaetzliche Drehung je Knochen.
// FK          je Zielknochen mit Quelle: R_z(f) = R_q(f) R_q(P)^-1 R_z(P) (Weltdrehung als Unterschied zur Retarget-Pose,
//             P = Pose); ohne Quelle folgt er seinem Eltern (oder dem Knochen, dem er zugeordnet ist: Finger und Tags der Hand
//             unter dem Unterarm) mit der Lage der Pose. Orte aus den Laengen des Ziels. Drehknochen ohne Quelle nehmen den
//             Anteil der Drehung des Endknochens um die Achse des Segments, der ihrer Lage auf der Strecke entspricht.
// Becken      Ort z(f) = z(P) + k (q(f) - q(P)), k = Beinlaenge Ziel / Quelle (Hoehe des Beckens ueber dem tiefsten Fusspunkt
//             in der Retarget-Pose) - die Schrittweite waechst mit dem Bein, die Fuesse rutschen nicht.
// IK          Haende/Fuesse: Ziel = Ort der Pose + k (Quelle - Pose der Quelle), Reach 0..1 mischt FK -> Ziel (HumanIK
//             Reach T); Zwei-Glied-IK aus Stufe 4 (zweiGliedIk), Pol = das Gelenk der FK-Pose. Fusskontakte (Regel aus
//             Stufe 6: tiefster Fusspunkt hoechstens kontaktAbstand ueber seinem tiefsten Stand) fuehren den Fuss waagrecht
//             gleichmaessig mit dem Boden der Quelle (skaliert) ab dem Ort beim Aufsetzen.
// Wurzel      model_root und Motion: Ort wie das Becken skaliert (Wurzelbewegung der Quelle, skaliert).
// Reihenfolge Becken -> FK -> IK -> Wurzel (Unreal-Vorgabe), umstellbar; jeder Schritt wirkt auf die Welt des vorigen.
// Backen      lokale A(b) = B(p) X(p)^-1 X(b) B(b)^-1 (aus X(b) = X(p) B(p)^-1 A(b) B(b), gla.h) und compressBone wie Carcass
//             (14 Bytes); die neue GLA hat das Skelett des Ziels, die Bilder der Quelle, ihre fps (animation.cfg).
// Pruefen     Fussrutschen: waagrechte Abweichung des Fusses im Stand von der gleichmaessigen Bewegung des Bodens der Quelle
//             (skaliert; mit Wurzelbewegung steht der Boden, bei einem Gang auf der Stelle gleitet er nach hinten), Einheiten;
//             Spruenge (ein Bild, in dem sich ein Knochen des Ziels viel mehr dreht/bewegt als seine Quelle).
#ifndef BHED_MODELL_RETARGET_H
#define BHED_MODELL_RETARGET_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigdaten.h"

namespace bhed::modell {

// --- Charakter (Ketten) -------------------------------------------------------------------------------------------
enum class KettenArt : std::uint8_t { Wirbel = 0, Hals, Kopf, Schluessel, Arm, Bein, Finger, Anhang };
inline constexpr int kKettenArten = 8;
[[nodiscard]] const char* kettenSchluessel(KettenArt a) noexcept;   // "spine", "neck", ...

struct RtKette {
    KettenArt art = KettenArt::Wirbel;
    Seite seite = Seite::Mitte;
    int nummer = 0;                 // mehrere gleicher Art und Seite (Finger 0..4, Beine einer Kreatur von vorn nach hinten)
    std::vector<int> knochen;       // Haupt-Knochen vom Ansatz zur Spitze (ohne Drehknochen)
    std::vector<int> twist;         // Drehknochen der Kette
    [[nodiscard]] std::string schluessel() const;   // "arm_l_0" - gleich bei Quelle und Ziel heisst zugeordnet
};

struct RtCharakter {
    std::vector<RtKette> ketten;
    int becken = -1;
    int wurzel = -1;                // model_root
    int bewegung = -1;              // Motion
    std::vector<int> folgt;         // je Knochen: dem er ohne Quelle folgt (-1 sein Eltern); Finger/Tags unter dem Unterarm -> Hand
    std::vector<float> twistAnteil; // je Knochen: Anteil des Drehknochens auf seinem Segment (0 kein Drehknochen)
    std::vector<int> twistEnde;     // je Drehknochen: der Endknochen, dessen Drehung er teilt (Hand, Unterarm, Unterschenkel)
    std::vector<int> twistAnfang;   // je Drehknochen: Anfang des Segments (Achse Anfang -> Ende)
    std::string vorlage;            // Name der Vorlage ("" = vorgeschlagen aus der Erkennung)
    float massstab = 0.0F;          // fScale der GLA (mdx_format.h: "scale it was built with"; nur Angabe)
    int vorgeschlagen = 0;          // Ketten aus dem Vorschlag (unbekanntes Skelett)
    [[nodiscard]] int ketteVon(int knochen) const noexcept;   // -1 keine
};

// Vorlagen: Erkennung ueber Dateiname und Knochennamen (Anteil der Namen der Vorlage, die das Skelett hat).
struct RtVorlageInfo {
    std::string name;      // "_humanoid (JKA)", "rancor", "Mixamo" ...
    std::string datei;     // Pfad im Spiel ("" bei Mixamo/Biped/CAT)
    int knochen = 0;
    float massstab = 0.0F;
    int ketten = 0;
    std::string gruppe;    // AP10: Gruppe fuer die Auswahl ("human", "droid", "vehicle", "object", "sand" oder ein Eigenname wie "Rancor")
};
[[nodiscard]] std::vector<RtVorlageInfo> rtVorlagen();
// AP10: die Gruppen der Vorlagen in der Reihenfolge der Auswahl im Model-Tab (Entwurf: Mensch, Rancor, Wampa, Tauntaun, Howler,
// AT-ST, Droiden, Fahrzeuge/Schiffe); nur Gruppen, die eine Vorlage haben.
[[nodiscard]] std::vector<std::string> rtVorlagenGruppen();
// AP10: die Rollen, die eine Vorlage den Knochen des Skeletts gibt (ueber die Namen, wie rtCharakter): Becken, Wurzel, Bewegung,
// je Kette Wirbel / Hals / Kopf / Schluesselbein / Oberarm, Unterarm, Hand / Oberschenkel, Unterschenkel, Fuss, Zehen / Finger /
// Anhang und die Drehknochen von Oberarm, Unterarm und Oberschenkel. Fuer "Control Rig erstellen": was die Erkennung (rig.h)
// offen laesst, weiss die Vorlage. Leer: keine solche Vorlage oder kein Knochen des Skeletts traegt einen ihrer Namen.
[[nodiscard]] std::vector<Zuordnung> rtVorlageRollen(const GlaAnimation& g, int vorlage);
// Die passende Vorlage (-1 keine): erst Dateiname + Knochenzahl, sonst die meisten gleichen Namen (mindestens 80 %).
[[nodiscard]] int rtVorlageFuer(const GlaAnimation& g, const std::string& datei = {});
// Charakter: aus der Vorlage (vorlage >= 0; -2 = suchen) oder vorgeschlagen aus der Erkennung (Stufe 3) und der Hierarchie.
[[nodiscard]] RtCharakter rtCharakter(const GlaAnimation& g, const std::string& datei = {}, int vorlage = -2);
[[nodiscard]] RtCharakter rtVorschlag(const GlaAnimation& g);

// --- Zuordnung -----------------------------------------------------------------------------------------------------
// Die Zuordnung der Ketten ueber ihren Schluessel; `alt` (gleiche Zahl Ketten): Eintraege von Hand bleiben, nur leere
// werden gefuellt (wie "Auto-Map Chains" nur fuer leere Ketten).
[[nodiscard]] RtZuordnung rtZuordnen(const RtCharakter& q, const RtCharakter& z, const RtZuordnung* alt = nullptr);
// Je Knochen des Ziels der Knochen der Quelle (-1 keiner): in zugeordneten Ketten ueber die Lage auf der Kette (normierte
// Laenge in der Pose), uebrige Knochen ueber den gleichen Namen; Drehknochen ueber den Namen.
[[nodiscard]] std::vector<int> rtKnochenZuordnung(const GlaAnimation& gq, const RtCharakter& q, const GlaAnimation& gz, const RtCharakter& z,
                                                  const RtZuordnung& zu);

// --- Retarget-Pose ---------------------------------------------------------------------------------------------------
// Die Weltmatrizen der Pose (Grundstellung, ausgerichtet nach haltung, dazu die Drehungen von Hand).
void rtPoseWelt(const GlaAnimation& g, const RtCharakter& c, const RtPose& p, std::vector<BoneMatrix>& welt);
// Der groesste Winkel (Grad) zwischen den Richtungen der Arm- und Beinglieder zweier Posen ueber die Zuordnung (0 = gleiche
// Grundhaltung). Prueft den Ausgleich der Bindepose.
[[nodiscard]] float rtPoseAbweichung(const GlaAnimation& gq, const RtCharakter& q, const std::vector<BoneMatrix>& wq, const GlaAnimation& gz,
                                     const RtCharakter& z, const std::vector<BoneMatrix>& wz, const RtZuordnung& zu);

// --- Uebertragen -----------------------------------------------------------------------------------------------------
struct RtBild {
    std::array<float, 2> rutschen{};   // Abweichung des Fusses im Stand vom Boden der Quelle (links, rechts)
    std::array<bool, 2> kontakt{};
    bool sprung = false;
};
struct RtBericht {
    float k = 1.0F;                // Verhaeltnis der Beinlaengen (Ziel / Quelle)
    float beinQuelle = 0.0F;
    float beinZiel = 0.0F;
    int knochenMitQuelle = 0;
    int kettenZugeordnet = 0;
    float rutschenMax = 0.0F;      // groesstes Fussrutschen des Ziels (Einheiten)
    float rutschenQuelle = 0.0F;   // das der Quelle gegen ihre eigene gleichmaessige Bewegung, mit k skaliert
    std::vector<RtBild> bilder;
    std::vector<int> spruenge;     // Bilder (ab 0) mit Sprung
    std::vector<int> sprungKnochen;   // je Sprung der erste Knochen des Ziels, der springt
    float poseAbweichung = 0.0F;   // rtPoseAbweichung der beiden Posen
    std::string fehler;
};
// Die Welt des Ziels je Bild [erstes, erstes + anzahl) der Quelle. false: kein Becken in Quelle oder Ziel (Grund im Bericht).
bool rtUebertragen(const GlaAnimation& gq, const RtCharakter& q, const GlaAnimation& gz, const RtCharakter& z, const RtZuordnung& zu,
                   const RtEinstellung& e, int erstes, int anzahl, std::vector<std::vector<BoneMatrix>>& welt, RtBericht* bericht = nullptr);
// Backen: die Welten als GLA mit dem Skelett des Ziels (Name kopfName, fScale des Ziels), ein Bild je Welt.
[[nodiscard]] bool rtBacken(const GlaAnimation& gz, const std::vector<std::vector<BoneMatrix>>& welt, const std::string& kopfName, GlaAnimation& aus,
                            std::string* fehler = nullptr);

// Die Welten als Schicht ueber der Bindepose (bindeBasis, je Knochen und Bild ein Schluessel M = B^-1 A B, linear): fuer
// Figuren im Mission-Tab (Stufe 7), deren Schicht beim Bauen in die Karten-GLA gebacken wird. Bild f = Welt f.
[[nodiscard]] RigSchicht rtAlsSchicht(const GlaAnimation& gz, const std::vector<std::vector<BoneMatrix>>& welt);

// --- Pruefen -------------------------------------------------------------------------------------------------------
// Spruenge: ein Bild, in dem sich ein Zielknochen um mehr als 2 x seine Quelle + 1 Grad dreht oder um mehr als
// 2 x k x ihren Weg + 1 Einheit bewegt (Anzeige-Schwelle; das Spiel kennt keine).
[[nodiscard]] std::vector<int> rtSpruenge(const std::vector<std::vector<BoneMatrix>>& quelle, const std::vector<std::vector<BoneMatrix>>& ziel,
                                         const std::vector<int>& knochenQuelle, float k, std::vector<int>* knochen = nullptr);
// Die Welt eines Bilds der Quelle (wie worldMatrices).
void rtWeltQuelle(const GlaAnimation& g, int bild, std::vector<BoneMatrix>& welt);

// --- Stufe 5b: Geh-/Laufzyklus aus Reglern (eigener Generator) ---------------------------------------------------------
// Auf der Stelle wie die Gaenge des Spiels (die Figur bewegt das Spiel): je Fuss Stand (Fuss flach am Boden der Pose, er
// gleitet mit konstantem Tempo von vorn nach hinten - relativ zum Boden steht er) und Schwung (Kosinus nach vorn, Sinus hoch);
// Becken mit Wippen, Seitwaertsschwingen, Drehen und Kippen; Oberkoerper gegengedreht, Arme gegengleich. Alles periodisch in
// der Phase: Bild N (= Bild 0 des naechsten Zyklus) ist Bild 0, der Zyklus ist nahtlos.
// Vorgaben: Tempo wie die NPCs des Spiels (walkSpeed 90, runSpeed 300, NPC_stats.cpp:1996-1997 OpenJK); Standphase 60 % beim
// Gehen (J. Perry, J. M. Burnfield, Gait Analysis, 2. Aufl. 2010, Kap. 1: Stand 60 %, Schwung 40 %), ~35 % beim Laufen
// (T. F. Novacheck, The biomechanics of running, Gait & Posture 7, 1998: Standphase 39 % beim Joggen, kuerzer beim Sprint);
// Beckendrehung +-4 Grad und Kippen +-4 Grad je Seite beim Gehen (Perry/Burnfield Kap. 7 "Pelvis"). Die uebrigen Werte sind
// Regler (Haltung, Armschwung, Hub) ohne Spielwert.
// Sekunden je Doppelschritt: 2 * schritt / tempo.
[[nodiscard]] float gangZyklus(const GangRegler& r) noexcept;
// Die Welt zur Phase (0..1) und der ganze Zyklus mit fps Bildern je Sekunde (round(zyklus * fps) Bilder). Das Becken senkt
// sich so weit, dass jeder Standfuss erreichbar bleibt (daraus das Wippen des umgekehrten Pendels).
bool rtGangBild(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, float phase, std::vector<BoneMatrix>& welt, std::string* fehler = nullptr);
bool rtGang(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, int fps, std::vector<std::vector<BoneMatrix>>& welt, std::string* fehler = nullptr);
// Tempo und Schrittlaenge einer Gang-Animation der Figur (auf der Stelle wie die des Spiels): im Stand (Kontaktregel wie
// rtFussRutschen) gleitet der Knoechel mit dem Tempo nach hinten; Schritt = Tempo * Dauer / (2 * Zyklen), Zyklen = Kontakte
// des linken Fusses. false: kein Stand gefunden.
bool gangMessen(const GlaAnimation& g, const RtCharakter& c, int erstes, int anzahl, double bildMs, float& tempo, float& schritt);
// Pruefen: groesste Abweichung des Fusses im Stand vom Boden der Pose und vom gleichmaessigen Gleiten (Einheiten).
struct GangBefund {
    float bodenAbw = 0.0F;
    float gleitAbw = 0.0F;
    float naht = 0.0F;            // Welt der Phase 1 gegen Phase 0 (Ort, Einheiten)
    int standBilder = 0;
};
[[nodiscard]] GangBefund rtGangPruefen(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, int fps);

}  // namespace bhed::modell
#endif
