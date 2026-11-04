// modellsetzen.h - rc609 M0: ein vorhandenes Modell mit einem Klick in die Karte (Karten-Editor und
// Mission-Tab), ohne den Modell-Block.
//
// Was hier steht, rechnet ohne Oberflaeche und gilt fuer beide Tabs:
//
//   Klassen     welche Modell-Klassen ein Spiel hat, mit Quelle - und warum eine fehlt
//   Lage        angles/angle und modelscale/modelscale_vec so schreiben, dass Spiel und q3map2 dasselbe
//               lesen, und so lesen wie sie
//   Boden       der Ursprung, mit dem die Unterkante des gedrehten und skalierten Modells auf dem Boden steht
//   Kasten      die Weltgrenzen (fuer einen Clip-Brush)
//   Browser     die Modelldateien der Spielordner (Archive und lose Dateien) und die Suche darin
//   Live        der Stand einer Datei (Zeit, Groesse), um eine neuere Fassung zu erkennen
//
// Quellen (OpenJK 260c59c2, Movie Duels 9, q3map2 aus NetRadiant-Custom 20260114):
//   code/game/g_misc.cpp:387-426 SP_misc_model_static (modelscale_vec "1 1 1", modelscale ungleich 0 ueberschreibt,
//     zoffset), :373-385 SP_misc_model (G_FreeEntity: nur q3map2 nimmt es), code/cgame/cg_main.cpp:1962-1990
//     CG_CreateMiscEntFromGent (nur .md3), :2030-2049 CG_DrawMiscEnts (AnglesToAxis, ScaleModelAxis, Pruefpunkt
//     origin + zoffset + 1), code/game/g_spawn.cpp:691-692 (Spawntabelle);
//   codemp/cgame/cg_spawn.c:170-235 SP_misc_model_static (angles, sonst angle; modelscale_vec, sonst modelscale),
//     codemp/game/g_misc.c:302-305 (der Server gibt es frei);
//   codeJK2/game/g_spawn.cpp:574 (Jedi Outcast kennt nur misc_model, kein misc_model_static);
//   MD9 code/game/g_misc.cpp:409-450, code/cgame/cg_main.cpp:3212-3240 (wie OpenJK, modelscale_vec mit weniger als
//     drei Zahlen faellt auf 1 1 1 zurueck);
//   q3map2 model.cpp:1461-1480 ModelGetTransform (modelscale_vec, sonst modelscale; angles, sonst angle),
//     :1529-1530 spawnflags, :358-371 und :969-1000 (spawnflags 2 = eClipModel: Kollision beim Einbacken).
#ifndef BHED_MODELLSETZEN_H
#define BHED_MODELLSETZEN_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bhed {

struct GamePath;

namespace modellsetzen {

// --- Klassen je Spiel ------------------------------------------------------------------------------
struct KlassenAngebot {
    std::string klasse;    // "misc_model" / "misc_model_static"
    bool da = false;       // anbieten?
    std::string quelle;    // wo das Spiel bzw. q3map2 sie nimmt
    std::string grund;     // warum nicht (leer, wenn da)
};
// spiel: kRegelwerkOpenJK, kRegelwerkMovieDuels oder kRegelwerkJediOutcast (bhed/regelwerk.h);
// mitQuelle: die Karte hat eine .map, die q3map2 baut (misc_model wird eingebacken).
[[nodiscard]] std::vector<KlassenAngebot> modellKlassen(int spiel, bool mitQuelle);
// Die Vorgabe beim Einfuegen: mit Quelle misc_model (Lightmaps), sonst misc_model_static; "" = keine.
[[nodiscard]] std::string vorgabeKlasse(int spiel, bool mitQuelle);

// --- Lage --------------------------------------------------------------------------------------------
struct Lage {
    float winkel[3]{0.0F, 0.0F, 0.0F};   // Nicken, Gier, Rollen (wie "angles")
    float skala[3]{1.0F, 1.0F, 1.0F};
};
using Schluessel = std::vector<std::pair<std::string, std::string>>;
// Was zu schreiben ist (Wert "" = Schluessel entfernen): genau einer von angle/angles und hoechstens einer
// von modelscale/modelscale_vec - dann lesen SP, MP und q3map2 dasselbe, unabhaengig von der Reihenfolge.
[[nodiscard]] Schluessel lageSchluessel(const Lage& l);
// Lesen wie SP_misc_model_static in der Reihenfolge der Schluessel (G_ParseField: der letzte von angle/angles
// gewinnt; modelscale_vec, darueber modelscale ungleich 0). movieDuels: MD9 faellt bei weniger als drei Zahlen
// in modelscale_vec auf 1 1 1 zurueck.
[[nodiscard]] Lage lageWieSpiel(const Schluessel& keys, bool movieDuels);
// Lesen wie q3map2 ModelGetTransform: modelscale_vec, sonst modelscale; angles, sonst angle.
[[nodiscard]] Lage lageWieQ3map2(const Schluessel& keys);

// AnglesToAxis (q_math.c): achse[0] vorn, [1] links, [2] oben; danach je Achse mit skala (ScaleModelAxis).
void achsen(const Lage& l, float achse[3][3]);
// Der Ursprung (z), mit dem die tiefste Ecke des Kastens mins..maxs nach Drehung und Skalierung auf bodenZ liegt.
[[nodiscard]] float ursprungZAufBoden(float bodenZ, const float mins[3], const float maxs[3], const Lage& l);
// Die Hoehe der Flaeche unter einer senkrechten Spur nach unten: CM_TraceThroughBrush haelt SURFACE_CLIP_EPSILON (0.125,
// code/qcommon/cm_local.h:152, cm_trace.cpp:456) vor der Ebene an - endpos liegt also 0.125 / normaleZ ueber dem Boden.
[[nodiscard]] float bodenHoehe(float endZ, float normaleZ);
// Die Weltgrenzen des gedrehten, skalierten Kastens am Ursprung ort.
void weltKasten(const float mins[3], const float maxs[3], const Lage& l, const float ort[3], float aus0[3], float aus1[3]);

// --- Browser -----------------------------------------------------------------------------------------
struct ModellDatei {
    std::string pfad;       // wie im Spiel ("models/map_objects/bespin/panels.md3")
    std::string herkunft;   // .pk3 bzw. voller Pfad der losen Datei
    bool lose = false;
};
// Alle Dateien unter models/ mit einer der Endungen (klein, mit Punkt) aus den Spielordnern - je Name (ohne
// Gross/klein) die Fassung, die das Spiel liest (FS_FOpenFileRead: spaetere Ordner zuerst, je Ordner die .pk3
// vor den losen Dateien, mit dirVorPak umgekehrt). Nach Pfad sortiert.
[[nodiscard]] std::vector<ModellDatei> modellDateien(const std::vector<GamePath>& ordner, const std::vector<std::string>& endungen,
                                                     bool dirVorPak);
// Suche: alle Woerter (durch Leerzeichen getrennt, ohne Gross/klein) muessen im Pfad stehen.
[[nodiscard]] std::vector<std::size_t> modellSuche(const std::vector<ModellDatei>& d, const std::string& suche);

// --- Live --------------------------------------------------------------------------------------------
struct DateiStand {
    bool da = false;
    std::int64_t zeit = 0;
    std::uint64_t groesse = 0;
    [[nodiscard]] bool operator==(const DateiStand& o) const { return da == o.da && zeit == o.zeit && groesse == o.groesse; }
};
[[nodiscard]] DateiStand dateiStand(const std::string& vollerPfad);
// Wo das Spiel die Datei findet, ohne sie zu lesen (dieselbe Reihenfolge wie liesWieSpiel/FS_FOpenFileRead): die
// .pk3 bzw. der volle Pfad der losen Datei; "" = nirgends.
[[nodiscard]] std::string fundort(const std::vector<GamePath>& ordner, const std::string& name, bool dirVorPak, bool* lose = nullptr);

}  // namespace modellsetzen
}  // namespace bhed

#endif
