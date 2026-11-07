// modell/animkurven.h - Modell-Block Stufe 3b: Kurven der Schluessel-Schicht, Animationswerkzeuge, Aufraeumen, Reduzieren
//
// Kurven     je Knochen sieben Kanaele (Quaternion w x y z, Verschiebung x y z) ueber den Schluesseln der Schicht
//            (bhed/modell/rigdaten.h). Bezier rechnet wie eine F-Kurve von Blender (Quelle: blender/blender 792201f4,
//            GPL-2.0-or-later): Henkel nach BKE_fcurve_handles_recalc_ex (fcurve.cc) mit calchandleNurb_intern und
//            BKE_nurb_handle_smooth_fcurve (curve.cc, "Continuous Acceleration" - Vorgabe neuer Kurven seit 2.81,
//            versioning_userdef.cc: auto_smoothing_new = FCURVE_SMOOTH_CONT_ACCEL; Henkelart Auto Clamped, keyhandles_new
//            = HD_AUTO_ANIM), Auswerten nach fcurve_eval_keyframes_interpolate mit BKE_fcurve_correct_bezpart, findzero,
//            solve_cubic und berekeny. 3ds Max legt seine Rechnung nicht offen; der Modus "wie 3ds Max" zeigt dieselben
//            Kurven mit den Namen von Max (Auto, Glatt, Linear, Benutzerdefiniert ...), siehe die Oberflaeche.
// Werkzeuge  die Schieber von Blender je Kanal auf den gewaehlten Schluesseln (editors/animation/keyframes_general.cc:
//            breakdown/blend_to_neighbor/blend_to_default/push_pull/ease/smooth_fcurve_segment) und die Posen-Schieber
//            am Zeitcursor (editors/armature/pose_slide.cc: Breakdown, Push, Relax, Blend, Blend to Rest; Quaternion mit
//            interp_qt_qtqt), Timing, Werte, Anim Offset, Animation spiegeln, Marker-Werkzeuge.
// Aufraeumen ueberfluessige Schluessel (clean_fcurve), statische Kanaele, Quaternion-Vorzeichen (die Kanaele sind
//            Quaternionen, ein Euler-Filter entfaellt), Reduzieren: die gebackenen GLA-Bilder einer Sequenz zu wenigen
//            Schluesseln, jedes Knochen-Bild nach dem Backen innerhalb der Grenze aus compressBone (Stufe 3).
#ifndef BHED_MODELL_ANIMKURVEN_H
#define BHED_MODELL_ANIMKURVEN_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigdaten.h"

namespace bhed::modell {

// --- Kanaele ---------------------------------------------------------------------------------------------------------
[[nodiscard]] float kanalWert(const RigSchluessel& k, int kanal) noexcept;
// Den Wert setzen; die Henkel gehen mit (BKE_fcurve_keyframe_move_value_with_handles - sie sind relativ gespeichert).
void kanalSetzen(RigSchluessel& k, int kanal, float wert) noexcept;
// Die Henkel aller Kanaele neu rechnen (nach jeder Aenderung der Schluessel). glatt = Continuous Acceleration (Vorgabe),
// false = FCURVE_SMOOTH_NONE.
void henkelNeu(std::vector<RigSchluessel>& v, bool glatt = true);
void henkelNeu(RigSchicht& s);
// Beim Ziehen eines Henkels (Kanal, Index, links): ein ausgerichteter Gegenhenkel folgt dem gezogenen (bezt->f1 gewaehlt).
void henkelNeuGezogen(std::vector<RigSchluessel>& v, bool glatt, int kanalGezogen, int indexGezogen, bool linksGezogen);
// Ein Kanal bei Bild x (ausserhalb konstant wie FCURVE_EXTRAPOLATE_CONSTANT).
[[nodiscard]] float kanalAuswerten(const std::vector<RigSchluessel>& v, int kanal, float x);
// Quaternion-Vorzeichen fortlaufend: jedes q in der Halbkugel des vorigen (q und -q drehen gleich). Gibt die Zahl der
// gewendeten Schluessel.
int quatFolge(std::vector<RigSchluessel>& v);

// Teile von Blender, offen fuer den ctest
namespace bez {
[[nodiscard]] int findeNull(float x, float q0, float q1, float q2, float q3, float* o);   // findzero
[[nodiscard]] float yBei(float f1, float f2, float f3, float f4, float t);                // berekeny
void korrigiere(const float v1[2], float v2[2], float v3[2], const float v4[2]);         // BKE_fcurve_correct_bezpart
}  // namespace bez

// --- Auswahl ---------------------------------------------------------------------------------------------------------
struct SchluesselWahl {
    int knochen = -1;
    int bild = 0;
    bool operator==(const SchluesselWahl& o) const noexcept { return knochen == o.knochen && bild == o.bild; }
};
using Wahl = std::vector<SchluesselWahl>;
// Die gewaehlten Schluessel eines Knochens als Indizes in seine Liste (sortiert).
[[nodiscard]] std::vector<int> wahlIndizes(const RigSchicht& s, const Wahl& w, int knochen);
[[nodiscard]] std::vector<int> wahlKnochen(const Wahl& w);
// Stufe 7: die Wahl samt den IK-Schluesseln (ikWahlCode) am Bild jedes gewaehlten FK-Schluessels eines Glieds ihrer
// Kette und umgekehrt den FK-Schluesseln der Kette am Bild jedes gewaehlten IK-Schluessels - das Auto-Einrasten schreibt
// beide zusammen, Verschieben/Skalieren/Einrasten nehmen sie gemeinsam mit.
[[nodiscard]] Wahl wahlMitIk(const RigSchicht& s, const Wahl& w);

// --- Werkzeuge auf den gewaehlten Schluesseln (je Kanal wie die Schieber im Graph Editor von Blender) ------------------
enum class KurvenWerkzeug : std::uint8_t { Breakdown = 0, BlendNachbar, BlendVorgabe, PushPull, Ease, Glaetten };
// faktor wie in Blender: Breakdown/BlendNachbar/Ease -1..1, BlendVorgabe 0..1, PushPull 0..2 (1 = unveraendert),
// Glaetten 0..1. zusatz: Ease "sharpness" (Vorgabe 2), Glaetten "sigma" (Vorgabe 0,33; Filterbreite 6).
// Gibt die Zahl der geaenderten Schluessel.
int kurvenWerkzeug(RigSchicht& s, const Wahl& w, KurvenWerkzeug art, float faktor, float zusatz = -1.0F);

// --- Posen-Schieber am Zeitcursor (pose_slide.cc) --------------------------------------------------------------------
enum class PosenSchieber : std::uint8_t { Breakdown = 0, Push, Relax, Blend, BlendRuhe };
// Fuer jeden Knochen mit Schluesseln (aus `knochen`, leer = alle): die Werte am vorigen und naechsten Schluessel aller
// dieser Knochen (wie die Schluesselliste von Blender), gemischt nach `art`; schreibt einen Schluessel am Bild.
int poseSchieben(RigSchicht& s, const std::vector<int>& knochen, int bild, PosenSchieber art, float faktor);

// --- Timing ----------------------------------------------------------------------------------------------------------
// Verschieben um d Bilder; Schluessel, die auf einen nicht gewaehlten fallen, ersetzen ihn (wie Blender nach dem
// Verschieben: der gewaehlte bleibt). Gibt die neue Auswahl.
Wahl schluesselVerschieben(RigSchicht& s, const Wahl& w, int d);
// Skalieren in der Zeit um `drehpunkt` (gerundet auf ganze Bilder, wie Einrasten auf Frames).
Wahl schluesselSkalieren(RigSchicht& s, const Wahl& w, float faktor, float drehpunkt);
enum class Einrasten : std::uint8_t { AufCursor = 0, ErsterAufCursor, LetzterAufCursor };
Wahl schluesselEinrasten(RigSchicht& s, const Wahl& w, Einrasten art, int cursor);
// Zwischenbilder: ab Bild `ab` n Bilder einfuegen (spaetere Schluessel ruecken nach hinten) bzw. entfernen (Schluessel
// in [ab, ab + n) fallen weg, spaetere ruecken vor). Gilt fuer alle Knochen. Gibt die Zahl der bewegten Schluessel.
int zeitEinfuegen(RigSchicht& s, int ab, int n);
int zeitEntfernen(RigSchicht& s, int ab, int n);
// Zeitversatz: die Werte der gewaehlten Schluessel aus der um `bilder` versetzten Kurve, zyklisch ueber den ersten bis
// letzten Schluessel (time_offset_fcurve_segment).
int zeitVersatz(RigSchicht& s, const Wahl& w, float bilder);
// Schluesselzeiten teilen: jeder Knochen aus `knochen` bekommt Schluessel an allen Zeiten der anderen (die Kurve bleibt,
// der Wert wird an der Stelle gelesen). vomLetzten >= 0: genau die Zeiten dieses Knochens, andere fallen weg.
int zeitenTeilen(RigSchicht& s, const std::vector<int>& knochen, int vomLetzten);

// --- Werte -----------------------------------------------------------------------------------------------------------
enum class WertArt : std::uint8_t { Null = 0, AktuellerWert, Negieren, Umkehren, SpiegelnCursor };
// Null: Einheit (q = 1, t = 0); AktuellerWert: der Wert der Kurve am Cursor; Negieren: umgekehrte Drehung und -t;
// Umkehren: die Reihenfolge in der Zeit je Knochen (erster <-> letzter gewaehlter); SpiegelnCursor: Bild -> 2 c - Bild
// (wie "Mirror over current frame"). Gibt die neue Auswahl.
Wahl werteAendern(RigSchicht& s, const Wahl& w, WertArt art, int cursor);

// --- Anim Offset -----------------------------------------------------------------------------------------------------
// Die Aenderung eines Knochens am Bild (vorher -> nachher, je M) auf seine anderen Schluessel uebertragen: q_k' = q_k *
// (q_vorher^-1 q_nachher), t_k' = t_k + (t_nachher - t_vorher). nurBilder leer = alle Schluessel des Knochens.
int animOffset(RigSchicht& s, int knochen, const RigSchluessel& vorher, const RigSchluessel& nachher, const std::vector<int>& nurBilder);

// --- Spiegeln, Posen-Bibliothek, Marker ------------------------------------------------------------------------------
// Die ganze Sequenz an der Seitenachse spiegeln: je Bild in [erstes, erstes + anzahl) die gespiegelte Pose (Partner ueber
// l_/r_, D'(b) = S D(p) S, poseEinfuegen) als Schluessel aller Knochen.
int animationSpiegeln(const GlaAnimation& g, const Rig& r, RigSchicht& s, int erstes, int anzahl);
[[nodiscard]] BibliothekPose bibliothekPose(const std::string& name, const RigPose& p);
[[nodiscard]] RigPose ausBibliothek(const BibliothekPose& b);
// Marker der animation.cfg-Zeile: Schluessel am ersten und letzten Bild der Sequenz (Werte der Kurve), Schleife schliessen
// (das letzte Bild bekommt den Wert des ersten bzw. des Schleifenbilds).
int markerSchluessel(RigSchicht& s, const std::vector<int>& knochen, int erstes, int letztes);
int schleifeSchliessen(RigSchicht& s, const std::vector<int>& knochen, int vonBild, int letztes);

// --- Posen-Bibliothek als Datei (Stufe 4) ------------------------------------------------------------------------------
// Text, UTF-8: "JKStudioPosen 1", je Pose "pose <Name>", "knochen <n>", je Knochen "<Name> m00 m01 m02 m03 m10 ... m23".
// Laden ersetzt Posen gleichen Namens und haengt neue an. fehler: der Grund.
[[nodiscard]] std::string posenText(const std::vector<BibliothekPose>& posen);
[[nodiscard]] bool posenAusText(const std::string& text, std::vector<BibliothekPose>& posen, std::string& fehler);

// --- Aufraeumen ------------------------------------------------------------------------------------------------------
// Ueberfluessige Schluessel wie clean_fcurve (Schwelle 0,001): ein Schluessel faellt, wenn jeder seiner Kanaele ihn
// entfernen wuerde. Gibt die Zahl der entfernten.
int ueberfluessigeEntfernen(RigSchicht& s, const std::vector<int>& knochen, float schwelle = 0.001F);
// Statische Kanaele: ein Knochen, dessen Schluessel alle gleich sind, behaelt einen; ueber der GLA (keine Bindebasis)
// faellt auch dieser, wenn er die Einheit ist. Gibt die Zahl der entfernten Schluessel.
int statischeEntfernen(RigSchicht& s, const std::vector<int>& knochen, float schwelle = 0.001F);
int quatFilter(RigSchicht& s, const std::vector<int>& knochen);

// Die Grenze eines Knochen-Bilds (14 Bytes) aus Stufe 3: |1 - |q|^2| |I - R|_F + (4 * 3 + 2 * 3 * 2 sqrt 2) / 16383
// (MC_UnCompressQuat normiert nicht; compressBone zielt auf die naechste Drehung und quantisiert je Anteil hoechstens
// 1,5 Stufen). Und der Abstand zweier Knochen-Bilder in der Drehung (Frobenius der entpackten 3x3). Die Verschiebung
// rundet compressBone auf 1/64 (halbe Stufe je Seite): die Grenze ist eine Stufe (verschiebungsStufen <= 1).
[[nodiscard]] double kompressionsGrenze(const std::uint8_t* c14) noexcept;
[[nodiscard]] double drehAbstand(const std::uint8_t* a14, const std::uint8_t* b14) noexcept;
[[nodiscard]] int verschiebungsStufen(const std::uint8_t* a14, const std::uint8_t* b14) noexcept;

struct ReduzierBericht {
    int knochen = 0;
    int bilder = 0;
    long long vorher = 0;      // Knochen * Bilder (je Bild ein Schluessel)
    long long nachher = 0;     // Schluessel der neuen Schicht
    int linear = 0;            // Knochen, die linear weniger Schluessel brauchten
    int bezier = 0;
    int statisch = 0;          // Knochen mit einem Schluessel
    double maxAnteil = 0.0;    // groesster Drehabstand / Grenze nach dem Backen (<= 1)
    int ueberGrenze = 0;       // Knochen-Bilder ueber der Grenze oder mehr als eine Stufe Verschiebung (0 erwartet)
    long long eineStufe = 0;   // Knochen-Bilder mit genau einer Stufe Verschiebung (Rundung, innerhalb der Grenze)
    std::string schlimmster;   // Knochen und Bild zu maxAnteil
    std::string fehler;
    // Stufe 4 (Auswahl): reduzierte Knochen, unveraendert uebernommene Schluessel, Schluessel ausserhalb der Spanne, die die
    // Grenze dort brauchte (die Henkel der Nachbarn aendern sich mit), dicht gesetzte Knochen-Bilder ausserhalb
    int gewaehlt = 0;
    long long behalten = 0;
    int aussen = 0;
    long long dicht = 0;
};
// Stufe 4: nur einige Knochen und/oder eine Zeitspanne reduzieren (aus Stufe 3b offen). Ausserhalb der Auswahl bleibt die
// Schicht: mit Bindebasis die Schluessel der Quelle, sonst (ueber der GLA kann ein Knochen mit Schluesseln in der Bindebasis
// nicht duenn bleiben) ein Schluessel je Bild aus dem Backen; Knochen ohne Schluessel und ohne IK bleiben ohne.
struct ReduzierAuswahl {
    std::vector<int> knochen;   // leer = alle
    int von = -1;               // erstes Bild der Spanne (absolut), -1 = Anfang der Sequenz
    int bis = -1;               // letztes Bild, -1 = Ende
};
// Die Bilder [erstes, erstes + anzahl) wie rigBacken sie mit der Schicht `quelle` schreiben wuerde, als neue Schicht mit
// Bindebasis und wenigen Schluesseln: je Knochen gierig (Schluessel an das Bild mit dem groessten Anteil an der Grenze),
// linear und Bezier (Auto Clamped), das mit weniger Schluesseln. Gebacken bleibt jedes Knochen-Bild innerhalb der
// Grenze (Drehung) und hoechstens eine Stufe Verschiebung.
// Die Schluessel liegen bei den Bildern der Sequenz (absolut).
// Stufe 4: die IK-Spuren der Quelle gehen mit (sie wirken auf den reduzierten Schluesseln wie vorher).
[[nodiscard]] bool rigReduzieren(const GlaAnimation& g, const RigSchicht& quelle, int erstes, int anzahl, RigSchicht& aus, ReduzierBericht* bericht = nullptr,
                                 const ReduzierAuswahl* auswahl = nullptr);
// Nachpruefen: die Schicht `reduziert` gebacken gegen `quelle` gebacken (dieselben Bilder), Anteil und Abweichungen.
[[nodiscard]] bool reduziertPruefen(const GlaAnimation& g, const RigSchicht& quelle, const RigSchicht& reduziert, int erstes, int anzahl,
                                    ReduzierBericht& bericht);

}  // namespace bhed::modell
#endif
