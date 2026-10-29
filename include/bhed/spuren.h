// spuren.h - das EINE Modell der Zeitleiste im Mission-Tab (zeitleiste1)
//
// Aus dem ICARUS-Nachbau (bhed/ablauf.h), der Kamerabahn (bhed/camtrack.h),
// der Szene (bhed/scene.h) und der flachen Zeitleiste (bhed/timeline.h)
// entsteht hier eine Liste von Bloecken je Zeile - mit den ECHTEN Dauern:
// ein Satz so lang wie seine Klangdatei, ein Lauf bis zur Ankunft, eine
// Animation bis zum Ende ihrer Haltezeit, eine Kamerafahrt so lang wie ihr
// letztes Argument.
//
// Vorher gab es zwei Modelle: die Zeitleiste ordnete die Befehle per
// Textsuche im Label Kategorien zu, und die Feldansicht "Spuren" baute ein
// eigenes (Dialogbloecke fest 2,5 s lang). Jetzt zeichnen beide nur noch,
// was hier steht - gleiche Dauern ueberall.
//
// Ohne ImGui und ohne g_app: was nur die Oberflaeche weiss (Laenge einer
// Klangdatei, Untertitel aus dem StringEd-Paket, Musik hinter einem use),
// kommt ueber die Rueckrufe in SpurenQuellen herein. So laesst sich das
// Modell im Kern-Test (tests/spurentest.cpp) gegen eine Mini-Szene pruefen.
//
// Nichts wird erfunden: die Kamera interpoliert im Spiel nur linear
// (CGCam_Update, cg_camera.cpp), also gibt es fuer Zoom und Blende nur
// Knickpunkte, keine Kurventangenten; ein Lauf hat keine Wunschdauer (die
// Dauer kommt aus Wegsuche und Tempo), also ist er nicht in der Laenge
// ziehbar.
#ifndef BHED_SPUREN_H
#define BHED_SPUREN_H

#include "bhed/ablauf.h"
#include "bhed/camtrack.h"
#include "bhed/scene.h"
#include "bhed/script.h"
#include "bhed/timeline.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::spuren {

enum class Art : std::uint8_t {
    // Kamera (camera ( ... ), cg_camera.cpp)
    Einstellung,   // Uebersicht "Kamera - Ort": ein MOVE bis zum naechsten
    Fahrt,         // MOVE
    Schwenk,       // PAN
    Zoom,          // ZOOM
    Rollen,        // ROLL
    Blende,        // FADE
    Wackeln,       // SHAKE
    Folgen,        // FOLLOW
    KameraBahn,    // PATH (ROFF der Kamera)
    KameraAn,      // ENABLE
    KameraAus,     // DISABLE
    // Ton
    Satz,          // sound ( KANAL, datei ) - Laenge der Datei
    Musik,         // target_play_music per use (oder die Levelmusik)
    // Ablauf
    Warten,        // wait / waitsignal / dowait
    Signal,        // signal ( name )
    Benutzen,      // use ( name )
    Setzen,        // set ( SET_..., wert )
    Befehl,        // alles andere, das der Nachbau ausfuehrt
    Kartenwechsel, // target_level_change - hier endet die Szene
    NichtNachgebildet,  // ein Befehl, den die Vorschau (noch) nicht nachbildet
    // Darsteller
    Laufen,        // SET_NAVGOAL bis zur Ankunft (alle Teilstuecke)
    Gleiten,       // move ( ... ) auf einer Entity
    Drehen,        // rotate ( ... )
    Roff,          // play ( PLAY_ROFF, datei )
    Animation,     // SET_ANIM_* bis zum Ende der Haltezeit
    Stehen,        // zwischen zwei Laeufen, sichtbar
    Unsichtbar,    // SET_INVISIBLE, remove, kill, noch nicht da
};

enum class WarteArt : std::uint8_t { Zeit, Signal, Aufgabe };

struct Clip {
    Art art = Art::Befehl;
    double startMs = 0.0;
    double endMs = 0.0;
    // Beschriftung: Name der Kameramarke bzw. "(x, y, z)", Animationsname,
    // Zielname, Signalname ... (ohne Uebersetzung - die Oberflaeche setzt
    // die Saetze zusammen).
    std::string text;
    std::string befehl;     // der Befehl, wie er im Skript steht ("" wenn unbekannt)
    Path pfad;              // Zeile im OFFENEN Skript (leer: anderes Skript oder Nachbau)
    std::string fremd;      // Skript, aus dem der Befehl kommt, wenn nicht das offene
    Path fremdPfad;         // dann: die Zeile in jenem Skript ("Skriptzeile oeffnen")
    std::string wer;        // Darsteller (affect-Ziel), "" = Traeger des Skripts
    int bahn = 0;           // Unterbahn, wenn sich Bloecke einer Zeile ueberlappen
    // Kamera: dieser Befehl beginnt, bevor der vorige derselben Gruppe fertig
    // ist. CGCam_Update uebernimmt Ort/Winkel/Bildwinkel erst am ENDE eines
    // Uebergangs (cg_camera.cpp) und der neue rechnet vom alten Anfang - die
    // Kamera springt zurueck. vorigerStartMs: wo der vorige begann.
    bool springt = false;
    double vorigerStartMs = -1.0;
    // Einstellung: in diesem Block springt eine Kamerabewegung zurueck.
    bool problem = false;
    // Ton
    std::string datei;
    std::string kanal;
    std::string untertitel;
    std::string schluessel;   // StringEd-Schluessel des Untertitels
    double klangMs = 0.0;     // Laenge der Klangdatei (0: unbekannt)
    // Warten
    WarteArt warteArt = WarteArt::Zeit;
    std::string wartetAuf;    // Signal- oder Aufgabenname
    std::vector<std::pair<double, std::string>> sender;   // signal: wann, von wem
    // Darsteller
    int nummer = 0;           // Laeufe: 1, 2, 3 ... in der Reihenfolge
    std::string gang;         // Laeufe: BOTH_WALK1 (gehen) oder BOTH_RUN1 (rennen)
    AnimPart teil = AnimPart::Both;
    float von[3]{};
    float nach[3]{};
    // Zoom: Ziel-Bildwinkel; Blende: Ziel-Deckkraft (0..1).
    float wert = 0.0F;
    // Darf man den Block ziehen? Verschieben: es gibt eine Zeile im offenen
    // Skript, vor der ein wait die Zeit bestimmt. Laenge: der Befehl hat ein
    // Dauer-Argument (durationArgIndex) bzw. ist ein wait mit Zahl.
    bool verschiebbar = false;
    bool laengeZiehbar = false;
};

struct Zeile {
    std::vector<Clip> clips;
    int bahnen = 1;
};

// Ein Knickpunkt einer linearen Kurve (Zoom, Blende). Zwei Punkte mit
// gleicher Zeit sind ein Sprung.
struct Knick {
    double ms = 0.0;
    float wert = 0.0F;
};

struct Darsteller {
    std::string name;       // affect-Ziel / targetname
    std::string anzeige;    // lesbar: "Luke" aus NPC_type "luke"
    std::string npcType;
    int figur = -1;         // Index in Scene::actors
    bool sichtbarIrgendwann = false;
    Zeile oben;             // Uebersicht: aussagekraeftige Animationen
    Zeile laufen;           // Laufen, Gleiten, Drehen, Roff
    Zeile stehen;           // steht still (gestrichelt)
    Zeile unsichtbar;
    Zeile animation;        // Detail: alle Animationen
    Zeile befehle;          // Detail: die Befehle dieser Spur
};

struct Spuren {
    // Uebersicht
    Zeile ort;              // Einstellungen (+ Kamera an/aus als Marken)
    Zeile drehung;          // PAN
    // Detail
    Zeile fahrt;
    Zeile schwenk;
    Zeile zoom;
    Zeile blende;
    Zeile rollen;
    Zeile wackeln;
    Zeile folgen;
    Zeile kameraBahn;
    std::vector<Knick> sichtfeld;   // Bildwinkel (horizontal) wie gezeichnet
    std::vector<Knick> schwarz;     // Deckkraft der Blende 0..1
    // Ton und Ablauf
    Zeile sprache;
    Zeile musik;
    Zeile skript;           // Spur des Skripttraegers ohne Kamerabefehle
    std::vector<Darsteller> darsteller;
    double dauerMs = 0.0;   // bis hier reicht die Leiste
    double kartenwechselMs = -1.0;
    std::string kartenwechselKarte;
    int ueberlappungen = 0; // Kamerabefehle, die zurueckspringen
    int nichtNachgebildet = 0;
};

struct SpurenQuellen {
    const Timeline* zeitleiste = nullptr;
    const CameraTrack* kamera = nullptr;
    const Scene* szene = nullptr;
    const Ablauf* ablauf = nullptr;
    const Script* skript = nullptr;   // das offene Skript (die Pfade zeigen hinein)
    // Laenge einer Klangdatei in ms (0: unbekannt).
    std::function<double(const std::string& datei)> klangMs;
    // Untertitel zu einer Klangdatei ("" ohne), schluessel: der StringEd-Schluessel.
    std::function<std::string(const std::string& datei, std::string* schluessel)> untertitel;
    // Musik hinter einem use ( name ) - "" wenn das Ziel keine Musik spielt.
    std::function<std::string(const std::string& ziel)> musikVon;
    // Ein anderes Skript (run, use, spawnscript) nach seinem Namen - fuer den
    // Befehlstext der Bloecke, die von dort kommen (nullptr: unbekannt).
    std::function<const Script*(const std::string& skript)> fremdesSkript;
    std::string weltMusik;  // worldspawn "music" (laeuft ab 0)
};

[[nodiscard]] Spuren baue(const SpurenQuellen& q);

// Bildwinkel/Deckkraft einer Knickliste zur Zeit ms (linear, wie das Spiel).
[[nodiscard]] float kurveBei(const std::vector<Knick>& k, double ms);

// "luke" -> "Luke", "md_jedi_male" -> "Jedi male"; ohne NPC_type der Name.
[[nodiscard]] std::string anzeigeName(const std::string& npcType, const std::string& name);

// Bildet die Vorschau diesen Befehl nach? (camera TRACK/DISTANCE, print und
// Befehle, die der Nachbau nicht kennt, nicht.)
[[nodiscard]] bool nachgebildet(const Node& n);

// Ueberlappende Bloecke auf Unterbahnen verteilen (setzt Clip::bahn,
// Zeile::bahnen). Ein Block ohne Dauer belegt mindestens mindestMs.
void bahnenVerteilen(Zeile& z, double mindestMs);

}  // namespace bhed::spuren

#endif
