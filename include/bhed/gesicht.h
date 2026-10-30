// gesicht.h - Blinzeln und Leerlaufmimik einer Figur (Pruefung 4a)
//
// CG_G2PlayerHeadAnims (cg_players.cpp:5134-5232 OpenJK, Movie Duels und
// codeJK2 gleich) laeuft je gezeichnetem Bild:
//   * Tot: FACE_DEAD, sonst nichts mehr.
//   * facial_blink: beim ersten Mal cg.time + Q_flrand(4000, 8000), dazu
//     facial_timer cg.time + Q_flrand(6000, 10000). Ist die Zeit um, schliesst
//     CG_G2SetHeadBlink die Augen (leye/reye YAW -38, BONE_ANGLES_POSTMULT -
//     die Mischzeit 80 ms wertet G2_TransformBone fuer POSTMULT nicht aus,
//     tr_ghoul2.cpp:1800-1812; ausserhalb einer Kamerafahrt zu 5 % nur das
//     linke), facial_blink = -(cg.time + 300) (SET_FACEEYESCLOSED: 1 ->
//     -(cg.time + 99999999)); danach auf, neu cg.time + Q_flrand(4000, 8000).
//   * Spricht die Figur (VoiceVolume != 0): der Mund (FACE_TALK*), facial_timer
//     cg.time + Q_flrand(2000, 7000).
//   * Sonst: ist facial_timer um, eine Mimik FACE_ALERT + Q_irand(0, 2) fuer
//     2000 ms (facial_timer = -(cg.time + 2000)), danach cg.time +
//     Q_flrand(7000, 10000).
// Q3_Face (Q3_Interface.cpp OpenJK) setzt die Zeitgeber aus den Skripten
// (SET_FACEAUX ... SET_FACENORMAL).
//
// Die Zufallszahlen zieht das Spiel aus dem gemeinsamen Q_rand aller Aufrufe;
// diese Folge ist nicht nachzubauen. Hier zieht jede Figur aus einem eigenen,
// festen Strom (EngineZufall mit einer Saat je Figur) - so bleibt die Vorschau
// beim Spulen gleich. Gerechnet wird in den Zeitpunkten, in denen ein
// Zeitgeber ablaeuft (ein Bild spaeter als im Spiel hoechstens um die Dauer
// eines gezeichneten Bilds).
#ifndef BHED_GESICHT_H
#define BHED_GESICHT_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bhed {

// Q3_Face: die Befehle SET_FACE* mit ihrer Haltezeit in Sekunden.
struct GesichtsBefehl {
    enum class Art : std::uint8_t {
        AugenZu,      // SET_FACEEYESCLOSED
        AugenAuf,     // SET_FACEEYESOPENED
        Blinzeln,     // SET_FACEBLINK
        Aux,          // SET_FACEAUX
        BlinzelnFrown,// SET_FACEBLINKFROWN (faellt im Spiel in SET_FACEFROWN durch)
        Frown,        // SET_FACEFROWN
        Smile,        // SET_FACESMILE
        Glad,         // SET_FACEGLAD
        Happy,        // SET_FACEHAPPY
        Shocked,      // SET_FACESHOCKED
        Normal,       // SET_FACENORMAL
    } art = Art::Normal;
    double ms = 0.0;
    float halten = 0.0F;
};
// Der Name aus dem Skript ("SET_FACEAUX" ...); false: keiner davon.
[[nodiscard]] bool gesichtsBefehlArt(const std::string& name, GesichtsBefehl::Art& art);

struct GesichtsEingabe {
    std::uint32_t saat = 0;
    double abMs = 0.0;      // das erste Bild der lebenden Figur
    double bisMs = 0.0;     // so weit rechnen
    double todMs = -1.0;    // health <= 0 ab (FACE_DEAD), -1: lebt
    bool kamera = false;    // in_camera: kein Zwinkern
    std::vector<std::pair<double, double>> sprechen;   // VoiceVolume != 0 (von, bis)
    std::vector<GesichtsBefehl> befehle;               // nach der Zeit
};

struct GesichtsPlan {
    // Die Augen: ab ms der Winkel (0 offen, -38 zu) und ob nur das linke
    // (Zwinkern).
    struct Lid {
        double ms = 0.0;
        float winkel = 0.0F;
        bool nurLinks = false;
    };
    std::vector<Lid> lider;
    // Die Mimik ohne Sprechen: ab ms die Animation (leer: keine neue - die
    // letzte bleibt auf dem Knochen "face" stehen).
    std::vector<std::pair<double, std::string>> mimik;
    double todMs = -1.0;
    std::vector<std::pair<double, double>> sprechen;   // aus der Eingabe
};

[[nodiscard]] GesichtsPlan gesichtPlanen(const GesichtsEingabe& e);

// Zur Zeit ms: YAW der Augenknochen je Seite und die Mimik (FACE_DEAD,
// FACE_ALERT ...; leer: keine gesetzt). sprachEnde: endete danach ein Sprechen
// (dann bleibt dessen letzte Mundanimation auf dem Knochen), sein Ende, sonst -1.
void gesichtBei(const GesichtsPlan& p, double ms, float& links, float& rechts, std::string& mimik, double& sprachEnde);

}  // namespace bhed
#endif
