// modell/animcfg.h - animation.cfg lesen, bearbeiten, schreiben und pruefen; Karten-GLA und Plaetze (Modell-Block Stufe 2)
//
// Die Datei ist eine Zeile je Sequenz (G_ParseAnimationFile, OpenJK code/game/NPC_stats.cpp:879-1027; MD9 gleich):
//
//     BOTH_WALK1          \t30293\t30\t-1\t20
//     Name (animTable)      erster Frame, Anzahl, loopFrames, fps
//
// Kommentare und Leerzeilen bleiben beim Schreiben stehen; eine unveraenderte Zeile wird Byte fuer Byte geschrieben, eine
// geaenderte oder neue wie Raven sie schreibt ("%-20s\t%d\t%d\t%d\t%d", die Zeilen der JKA-Datei).
#ifndef BHED_MODELL_ANIMCFG_H
#define BHED_MODELL_ANIMCFG_H

#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"

namespace bhed::modell {

// bhed/modell/g2.h (das Dokument traegt die animation.cfg, g2.h braucht das Dokument)
enum class Zielspiel : std::uint8_t;

struct CfgZeile {
    std::string text;        // die Zeile der Datei ohne '\n' (mit '\r', wenn sie eins hatte)
    bool eintrag = false;    // eine Sequenz (Name und mindestens drei Zahlen)
    AnimEntry e;
    bool geaendert = false;  // neu schreiben statt `text`
};

struct AnimCfg {
    std::vector<CfgZeile> zeilen;
    bool schlussUmbruch = true;   // die Datei endet mit '\n'
    bool crlf = true;             // neue Zeilen mit "\r\n" (wie die Raven-Dateien)

    [[nodiscard]] std::vector<AnimEntry> eintraege() const;
    // Index der Zeile mit diesem Namen (gross/klein egal wie GetIDForString), -1 = keine.
    [[nodiscard]] int zeileVon(const std::string& name) const;
    // Setzt die Sequenz (neue Zeile am Ende, wenn es sie nicht gibt).
    void setzen(const AnimEntry& e);
    bool entfernen(const std::string& name);
};

[[nodiscard]] AnimCfg animCfgLesen(const std::string& text);
[[nodiscard]] std::string animCfgSchreiben(const AnimCfg& c);
[[nodiscard]] std::string animCfgZeile(const AnimEntry& e);

// --- Grenzen der animation.cfg je Zielspiel ----------------------------------------------------------------------
struct CfgGrenzen {
    int maxBytes;          // laengste Datei, die der Leser nimmt (0 = keine Grenze)
    int animDateien;       // MAX_ANIM_FILES
    bool kartenGla;        // laedt models/players/_humanoid_<karte>/ (G_ParseAnimFileSet)
    int kinoPlaetze;       // BOTH_CIN_1 .. n
};
[[nodiscard]] const CfgGrenzen& cfgGrenzen(Zielspiel s) noexcept;

enum class CfgBefundArt : std::uint8_t {
    HinterGla,      // erster Frame + Anzahl hinter dem Ende der GLA (T-Pose in OpenJK-Fassungen)
    ErsterFrame,    // erster Frame nicht in 0..65535 (unsigned short)
    Anzahl,         // Anzahl nicht in 0..65535
    Schleife,       // loopFrames nicht in -1..127 (signed char)
    ZuGross,        // Datei zu lang fuer den Puffer des Lesers
    Knochen,        // Karten-GLA ohne 53 Knochen
    NameKopf,       // Name im Kopf der GLA nicht gleich dem Ordner
    AnimDateien,    // mehr Animationssaetze als MAX_ANIM_FILES
    KeineKartenGla, // Zielspiel laedt keine Karten-GLA (JK2, MP)
    Unbekannt,      // Name nicht in der Animationsliste des Spiels
    NurSp,          // eigene GLA (eigenes Skelett) laedt nur der Einzelspieler
    MpHinweis,      // _humanoid.gla im Mehrspieler: Server und Client brauchen dieselbe Datei
};

struct CfgBefund {
    CfgBefundArt art = CfgBefundArt::HinterGla;
    std::string name;   // Sequenz oder Datei
    int wert = 0;
    int grenze = 0;
};

// Die Zeilen einer animation.cfg gegen die GLA (glaBilder Frames) und das Zielspiel; bytes = Groesse der Datei.
// bekannt: die Namen der Animationsliste des Spiels (leer = nicht pruefen).
[[nodiscard]] std::vector<CfgBefund> cfgPruefen(const std::vector<AnimEntry>& anims, int glaBilder, std::size_t bytes,
                                                Zielspiel s, const std::vector<std::string>& bekannt = {});

// --- Karten-GLA ----------------------------------------------------------------------------------------------------
// Pfad wie G_ParseAnimFileSet (NPC_stats.cpp:1137-1147): models/players/<skelett>_<karte>/<skelett>_<karte> (ohne Endung).
[[nodiscard]] std::string kartenGlaPfad(const std::string& skelett, const std::string& karte);
// Der Name im Kopf der GLA gegen ihren Ordner (models/players/X/X): leer = passt, sonst der erwartete Name.
[[nodiscard]] std::string kopfNameErwartet(const std::string& glaPfadOhneEndung);
// Pruefungen der fertigen Karten-GLA: 53 Knochen, Name im Kopf = Ordner, Zielspiel laedt Karten-GLAs.
[[nodiscard]] std::vector<CfgBefund> kartenGlaPruefen(const GlaAnimation& a, const std::string& glaPfadOhneEndung, Zielspiel s);
// Ziel _humanoid.gla (ersetzt das Skelett aller Figuren, im eigenen Mod-Ordner): im Mehrspieler der Hinweis auf den
// Gleichstand von Server und Client.
[[nodiscard]] std::vector<CfgBefund> humanoidGlaPruefen(Zielspiel s);
// Ziel eigene GLA (eigenes Skelett): nur Einzelspieler, Name im Kopf = Ordner.
[[nodiscard]] std::vector<CfgBefund> eigeneGlaPruefen(const GlaAnimation& a, const std::string& glaPfadOhneEndung, Zielspiel s);
// Die Zahl der Animationssaetze (verschiedene Skelette) gegen MAX_ANIM_FILES.
[[nodiscard]] std::vector<CfgBefund> animDateienPruefen(int saetze, Zielspiel s);

// Eine Quelle fuer einen Platz: Frames einer geladenen GLA.
struct PlatzQuelle {
    const GlaAnimation* gla = nullptr;
    std::string glaName;     // fuer die Anzeige
    AnimEntry sequenz;       // erster Frame, Anzahl, Schleife, fps in der Quelle
};

struct Platz {
    std::string name;        // BOTH_CIN_n
    bool belegt = false;
    PlatzQuelle quelle;
};

// Die Plaetze BOTH_CIN_1 .. n des Zielspiels, belegt aus einer vorhandenen animation.cfg.
[[nodiscard]] std::vector<Platz> kinoPlaetze(Zielspiel s, const std::vector<AnimEntry>& vorhanden);

struct ZusammenBericht {
    int bilder = 0;
    int vorrat = 0;          // Eintraege im Vorrat nach dem Zusammenlegen
    std::string fehler;
};

// Mehrere Quellen zu einer GLA zusammenfuehren: das Skelett der ersten Quelle, je belegtem Platz seine Frames
// hintereinander; die Vorratseintraege (14 Bytes, MC_UnCompressQuat) werden unveraendert uebernommen und gleiche
// zusammengelegt. Jede Quelle braucht dieselben Knochennamen und dieselbe Grundstellung (sonst bedeuteten die Eintraege
// etwas anderes). aus: GLA mit Namen kopfName, cfg: die Zeilen fuer animation.cfg (Platzname, Anfang in der neuen GLA).
[[nodiscard]] bool zusammenfuehren(const std::vector<Platz>& plaetze, const std::string& kopfName, GlaAnimation& aus,
                                   std::vector<AnimEntry>& cfg, ZusammenBericht* bericht = nullptr);

// Ziel _humanoid.gla: die GLA bleibt ganz (alle Frames, Vorrat, Kopf), die Frames jedes belegten Platzes kommen hinten
// dazu (Vorratseintraege unveraendert, gleiche zusammengelegt); cfg: je Platz die Zeile mit dem neuen Anfang.
[[nodiscard]] bool anhaengen(const GlaAnimation& basis, const std::vector<Platz>& plaetze, GlaAnimation& aus, std::vector<AnimEntry>& cfg,
                             ZusammenBericht* bericht = nullptr);

}  // namespace bhed::modell
#endif
