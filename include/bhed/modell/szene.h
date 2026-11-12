// szene.h - AP5 (MBENTWURF Teil B, Abgleich P7): die Szene des Bereichs Ansehen/Pruefen als ModView-Skript (.mvs)
//
// ModView (Raven, JACoders/OpenJK-tools ModView/script.cpp, GPL v2) speichert "Szenen" als Textdatei im GenericParser2-Format:
//
//   loadmodel
//   {
//       name        "model"
//       modelfile   "models/players/kyle/model.glm"
//       bonename_secondarystart "lower_lumbar"     (vor locksequence_secondary, sonst liest ModView es nicht)
//       locksequence "BOTH_STAND1"
//       locksequence_secondary "TORSO_WEAPONREADY3"
//       oldskinfile "models/players/kyle/model_default.skin"
//       surfaces_off { name0 "head" }
//       boltmodel { name "saber_1"  modelfile "models/weapons2/saber_1/saber_1.glm"  parent "model"  bolttosurface "*r_hand" }
//   }
//
// Gelesen wird wie Script_Read / Script_Parse_LoadModel / Script_Parse_BoltModel / Script_Parse_ReadOptionalArgs: Schluessel
// ohne Ruecksicht auf Gross-/Kleinschreibung, "name" und "modelfile" Pflicht, ein boltmodel braucht "parent" und "bolttobone"
// oder "bolttosurface". Was JK Studio (noch) nicht abbildet, steht in `nichtUebernommen` (ModView meldet nichts davon; wir sagen
// es): Anbauten an Anbauten, Sequenzen und Skins eines Anbaus, Mehrfach-Sequenzen (multiseq_*), Haeute mit "ethnic".
// Geschrieben wird wie Script_Write: ModView liest die Datei, die Schluessel "jks_*" (Detailstufe, Sichtfeld, Boden) uebergeht es.
#ifndef BHED_MODELL_SZENE_H
#define BHED_MODELL_SZENE_H

#include <string>
#include <string_view>
#include <vector>

namespace bhed::modell {

struct SzenenAnbau {
    std::string name;      // Kennung im Skript ("saber_1")
    std::string modell;    // modelfile
    std::string eltern;    // parent (Kennung)
    std::string ziel;      // Tag ("*r_hand") oder Knochen
    bool knochen = false;  // bolttobone statt bolttosurface
};

struct Szene {
    std::string name = "model";
    std::string modell;                  // modelfile (Pfad im Spiel, z. B. models/players/kyle/model.glm)
    std::string basisOrdner;             // basedir (ModView: NPC->MVS)
    std::string wurzel;                  // surfacename_rootoverride
    std::string sekundaerStart;          // bonename_secondarystart
    std::string primaer;                 // locksequence
    std::string sekundaer;               // locksequence_secondary
    std::string haut;                    // oldskinfile
    std::vector<std::string> flaechenAn;            // surfaces_on
    std::vector<std::string> flaechenAus;           // surfaces_off
    std::vector<std::string> flaechenOhneKinder;    // surfaces_offnochildren
    std::vector<SzenenAnbau> anbauten;
    // JK Studio (ModView uebergeht sie): -1 / < 0 = nicht gesetzt
    int lod = -1;                        // jks_lod: 0 automatisch wie im Spiel, sonst Stufe + 1
    float sichtfeld = -1.0F;             // jks_fov
    int boden = -1;                      // jks_floor: 0 aus, 1 an
    float bodenZ = 0.0F;                 // jks_floorz
    std::vector<std::string> nichtUebernommen;   // beim Lesen: was im Skript stand, aber nicht abgebildet wird
};

// Was an einem Skript nicht stimmt - fuer eine Meldung in der Sprache der Oberflaeche (FIX612 R6; der Text in *fehler von
// szeneLesen bleibt der englische von ModView, fuer das Protokoll)
enum class SzeneFehlerArt { Keiner, Klammer, KeinBlock, KeinSchluessel, ElternUnbekannt };
struct SzeneFehler {
    SzeneFehlerArt art = SzeneFehlerArt::Keiner;
    int zeile = 0;             // Klammer: die Zeile
    std::string block;         // KeinBlock, KeinSchluessel, ElternUnbekannt: "loadmodel" bzw. "boltmodel"
    std::string schluessel;    // KeinSchluessel: der fehlende Schluessel; ElternUnbekannt: die Kennung des Elternmodells
};

// Liest ein ModView-Skript. false: kein loadmodel, Klammerfehler oder Pflichtschluessel fehlt (Text in *fehler wie ModView,
// die Art des Fehlers in *was).
[[nodiscard]] bool szeneLesen(std::string_view text, Szene& aus, std::string* fehler, SzeneFehler* was = nullptr);
// Schreibt die Szene so, wie ModView sie schreibt (Tabulatoren, Reihenfolge von R_ModelContainer_WriteOptional).
[[nodiscard]] std::string szeneSchreiben(const Szene& s);
// Die Kennung eines Modells im Skript wie GenerateHandleName: Dateiname ohne Pfad und Endung, klein, bei Doppel "_2", "_3" ...
[[nodiscard]] std::string szenenKennung(const std::string& pfad, const std::vector<std::string>& vergeben);

}  // namespace bhed::modell
#endif
