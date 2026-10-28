// optimierer.h - Karten-Editor, Stufe 5: der Optimierer. Er zaehlt, was eine
// Karte gegen die Grenzen der Engine verbraucht (Punkt 2), sucht Bauprobleme
// (Punkt 3) und behebt sie auf Knopfdruck (Punkt 4).
//
// NRC hat keinen Optimierer - das ist eine eigene Funktion.
// Portiert aus OpenJK (die Zaehlregeln) und aus q3map2: gezaehlt wird wie
// die Engine zaehlt, mit den Regeln aus deren Quelltext:
//   - OpenJK code/game/g_spawn.cpp G_SpawnGEntityFromSpawnVars / G_ParseSpawnVars
//     / G_AddSpawnVarToken (was nach dem Spawn bleibt, Schluessel je Entitaet),
//     g_utils.cpp G_FindConfigstringIndex (Modelle, Klaenge, Effekte),
//     g_fx.cpp SP_CreateRain/Snow/Wind/WindZone/PuffSystem und g_misc.cpp
//     SP_misc_weather_zone (Welteffekte), die SP_*-Funktionen, die sich beim
//     Spawn selbst freigeben (g_misc.cpp, g_mover.cpp, g_nav.cpp, ...);
//   - code/qcommon/cm_load.cpp CMod_LoadSubmodels / CMod_LoadPatches und
//     cm_patch.cpp (patchkollision.h);
//   - code/rd-vanilla/tr_bsp.cpp (Lightmaps, Lichtgitter, Flaechen, Nebel);
//   - q3map2 (NetRadiant-Custom 20260114): map.cpp (func_group, group_info),
//     bsp.cpp / writebsp.cpp (Modellnummern, SetLightStyles, UnparseEntities).
// Movie Duels und Jedi Outcast mit ihren Abweichungen (grenzen.h).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#ifndef BHED_KARTE_OPTIMIERER_H
#define BHED_KARTE_OPTIMIERER_H

#include "bhed/karte/grenzen.h"
#include "bhed/karte/mathe.h"

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bhed {
struct Gegenstandsliste;
}

namespace bhed::karte {

class Karte;

// --- Momentaufnahme der Karte (fuer die Arbeit im Hintergrund) -------------
//
// Auf dem Hauptfaden aus der Karte gezogen (kartenAbbild), danach ohne Bezug
// zur Karte: der Optimierer laeuft in einem eigenen Faden. Die Indizes sind
// die der Karte zum Zeitpunkt der Aufnahme (Entitaet in Karte::entitaeten(),
// Primitiv in Entitaet::kinder()).
struct AbbildFlaeche {
    std::string shader;
    Ebene3d ebene;
    std::vector<Vektor3d> punkte;   // die Wicklung (Brush::buildWindings)
    int inhalt = 0;                 // m_contentFlags
    int flaechenFlags = 0;
};
struct AbbildPrimitiv {
    bool patch = false;
    bool detail = false;
    bool entartet = false;          // Pinsel::entartet (keine gueltige Wicklung)
    std::vector<AbbildFlaeche> flaechen;
    int breite = 0;                 // Patch
    int hoehe = 0;
    std::vector<std::array<float, 3>> kontrollpunkte;   // zeilenweise
    std::string shader;             // Patch
    Vektor3f mins{0, 0, 0};
    Vektor3f maxs{0, 0, 0};
};
struct AbbildEntitaet {
    std::vector<std::pair<std::string, std::string>> schluessel;
    std::string klasse;
    bool behaelter = false;          // Brush-Entity (!eclass->fixedsize): braucht Pinsel
    std::vector<AbbildPrimitiv> prims;
    [[nodiscard]] std::string wert(const std::string& schluessel) const;   // "" wenn nicht da (ohne Gross/klein)
    [[nodiscard]] bool hat(const std::string& schluessel) const;
};
struct KartenAbbild {
    std::vector<AbbildEntitaet> ents;
    std::size_t stand = 0;   // Rueckgaengig::stand() zum Zeitpunkt
};
[[nodiscard]] KartenAbbild kartenAbbild(const Karte& k);

// --- Zaehlen ---------------------------------------------------------------

struct ObjektRef {
    int entitaet = -1;     // Index in KartenAbbild::ents (bzw. im Entity-Lump der BSP)
    int primitiv = -1;     // Index in AbbildEntitaet::prims
    int bspFlaeche = -1;   // Index der Zeichenflaeche in der BSP
    std::string text;      // was es ist ("worldspawn, Pinsel 12", "textures/x", "maps/a.rof")
    int flaeche = -1;      // Index der Flaeche im Pinsel
    std::string wert;      // Behebung je Objekt (EntitaetSchluessel: der neue Wert)
};
struct Beitrag {
    ObjektRef ref;
    long long wert = 0;
};
enum class ZaehlQuelle { Karte, Bsp };
struct Zaehlung {
    std::string id;           // Grenze (grenzen.h)
    ZaehlQuelle quelle = ZaehlQuelle::Karte;
    long long wert = 0;
    bool schaetzung = false;  // nicht genau so zu zaehlen wie die Engine
    bool obergrenze = false;  // die Schaetzung liegt ueber dem wahren Wert (nur "ueber", wenn auch "sicher" darueber liegt)
    long long sicher = -1;    // der Teil, der sicher eintritt (z. B. NPC-Spawner ohne targetname), -1 = keiner
    std::string hinweis;      // Erklaerung der Zaehlung (Englisch, fuer den Bericht)
    std::vector<Beitrag> beitraege;   // groesste zuerst, hoechstens kMaxBeitraege
};
inline constexpr std::size_t kMaxBeitraege = 12;

struct ZaehlOptionen {
    GrenzSpiel spiel = GrenzSpiel::JkaSp;
    // NPC_type -> playerModel (ext_data/npcs/*.npc); leer = unbekannt.
    std::function<std::string(const std::string&)> npcModell;
    // items.dat des Spiels (Gegenstaende spawnen ohne Eintrag in spawns[]).
    const Gegenstandsliste* gegenstaende = nullptr;
    // Stufe 8a: "Editor-Daten behalten" baut mit -keeplights (wie _keepLights im worldspawn)
    bool lichterBehalten = false;
};

// Eine Entitaet nur mit Schluesseln (Entity-Lump der BSP oder Karte).
using EntSchluessel = std::vector<std::pair<std::string, std::string>>;

// Bleibt die Entitaet nach G_SpawnGEntityFromSpawnVars? (spawns[] + die
// SP_*-Funktionen, die sich selbst freigeben, je Spiel; "notsingle")
[[nodiscard]] bool bleibtNachSpawn(const EntSchluessel& e, GrenzSpiel spiel, const Gegenstandsliste* gegenstaende = nullptr);
// G_ParseSpawnVars / G_AddSpawnVarToken: Zahl der Paare und Zeichen samt Endnullen.
[[nodiscard]] std::pair<long long, long long> spawnVariablen(const EntSchluessel& e);

// Aus der Karte (ohne Bauen): alles, was die .map schon sagt.
[[nodiscard]] std::vector<Zaehlung> zaehleKarte(const KartenAbbild& a, const ZaehlOptionen& o);

// Aus der gebauten .bsp (RBSP): jeder Lump, Lichtgitter, Lightmaps, Flaechen,
// Patch-Kollision, Entity-Lump; prtDaten: die .prt daneben (leer: ohne Portale).
struct BspAnalyse {
    bool ok = false;
    std::string fehler;
    std::vector<Zaehlung> werte;
    // fuer die Pruefungen (Punkt 3)
    long long lichtgitterUeberlauf = 0;   // Zellen mit Index >= Zahl der Punkte
    bool lichtgitterFehlpassung = false;  // "light grid array mismatch" (tr_bsp.cpp)
    long long lichtgitterErwartet = 0;    // gridBounds-Produkt der Engine
    std::array<float, 3> gitterGroesse{64, 64, 128};
    long long cluster = 0;
    long long portale = 0;
    long long sichtBytes = 0;
    long long dreieckeJeAnsicht = 0;      // Schaetzung: Dreiecke im PVS des schlimmsten Clusters
    int dreieckeCluster = -1;
};
// CM_PointLeafnum (cm_test.cpp): der Cluster des Blatts an diesem Punkt
// (-1: im Festen oder ausserhalb), -2 bei einer ungueltigen BSP.
[[nodiscard]] int bspCluster(const std::string& bspDaten, const Vektor3f& p);
[[nodiscard]] BspAnalyse analysiereBsp(const std::string& bspDaten, const ZaehlOptionen& o, const std::string& prtDaten = std::string());

// --- Bewerten --------------------------------------------------------------

enum class Ampel { Ok, Knapp, Ueber, Hinweis };
struct Bewertung {
    const Grenze* grenze = nullptr;
    Zaehlung zaehlung;
    double anteil = 0;       // wert / erlaubt
    Ampel ampel = Ampel::Ok;
};
// knappAb: ab diesem Anteil "knapp" (Einstellung, Vorgabe 0.9). Harte Grenzen
// werden "ueber", wenn wert > erlaubt (ausser obergrenze); weiche
// (Formatgrenzen) hoechstens "Hinweis".
[[nodiscard]] std::vector<Bewertung> bewerten(GrenzSpiel s, const std::vector<Zaehlung>& z, double knappAb = 0.9);
[[nodiscard]] const char* ampelName(Ampel a);   // "ok" / "near" / "over" / "note"

// --- Pruefungen auf Bauprobleme (Punkt 3) ------------------------------------

enum class Schwere { Hinweis, Warnung, Fehler };
enum class Behebung {
    Keine,                // nur anzeigen
    Caulk,                // die betroffenen Flaechen bekommen common/caulk (verdeckte Flaechen)
    Detail,               // Make Detail
    Struktur,             // Make Structural
    Einrasten,            // Snap to Grid (Raster 1)
    PinselEntfernen,      // die Pinsel loeschen
    PatchTeilen,          // bobToolz Split patch
    ModelleZusammenlegen, // Brush-Entities mit gleichen Schluesseln zu einer
    WeltSchluessel,       // worldspawn-Schluessel setzen (gridsize, _lightmapscale)
    EntitaetEntfernen,    // die Entitaet loeschen
    TexturErsetzen,       // die Flaechen bekommen Befund::wert (common/origin -> system/origin)
    EntitaetSchluessel,   // je Entitaet Befund::schluessel = ObjektRef::wert (zoffset)
};
struct Befund {
    std::string pruefung;            // Kennung der Pruefung ("verdeckt", "leck", ...)
    Schwere schwere = Schwere::Hinweis;
    std::string quelle;              // Engine-/q3map2-Stelle oder Tutorial
    std::string detail;              // Technisches (Englisch, fuer den Bericht)
    // Stufe 6 (Punkt 5): dieselbe Angabe als englische Vorlage mit %s und den Werten - die
    // Oberflaeche uebersetzt die Vorlage (kOptimiererDetailVorlagen) und setzt die Werte ein.
    std::string detailVorlage;
    std::vector<std::string> detailWerte;
    long long anzahl = 0;            // wie viele (Flaechen, Pinsel, Paare, ...)
    std::vector<ObjektRef> objekte;  // die betroffenen Objekte
    Behebung behebung = Behebung::Keine;
    bool sicher = false;             // darf "Alles Sichere beheben" (ungefaehrlich)
    std::string schluessel;          // fuer WeltSchluessel
    std::string wert;
    bool schaetzung = false;         // Vorschlag aus einer Faustregel / Schaetzung
};

// Was der Optimierer vom Shader wissen muss (vom Texturinventar, auf dem
// Hauptfaden gesammelt). Ohne Angabe: nach dem Namen (textures/common/...).
struct ShaderArt {
    bool bekannt = true;       // Shader oder Bild gefunden
    bool undurchsichtig = true;   // deckt, was dahinter liegt (nicht trans, nonsolid, fog, liquid, ...)
    bool sichtbar = true;      // erzeugt eine Zeichenflaeche (nicht nodraw/caulk, kein Werkzeug)
    bool werkzeug = false;     // clip, trigger, hint, skip, areaportal, origin, ...
    // aus dem Shader-Skript (shaderSkriptLesen), sonst die Vorgaben von q3map2
    double flaechenlicht = 0.0;      // q3map_surfacelight
    double backsplashAnteil = 0.05;  // q3map_backsplash <Prozent> (q3map2.h:100 DEF_BACKSPLASH_FRACTION)
    double backsplashWeite = 23.0;   // q3map_backsplash ... <Abstand> (q3map2.h:101 DEF_BACKSPLASH_DISTANCE)
    bool himmel = false;             // surfaceparm sky
    bool ursprung = false;           // surfaceparm origin (games.cpp:803 C_ORIGIN): ein Ursprungs-Pinsel
    int lightmapBreite = 0;          // q3map_lightmapSize W H (0: nicht gesetzt) - Stufe 6, NRC#48/#98
    int lightmapHoehe = 0;
};
[[nodiscard]] ShaderArt shaderArtNachName(const std::string& shader);
// Stufe 6: "%s" der Vorlage der Reihe nach durch die Werte ersetzen (sonst nichts, auch kein %%).
[[nodiscard]] std::string detailText(const std::string& vorlage, const std::vector<std::string>& werte);
// setzt detailVorlage, detailWerte und detail (englisch)
void detailSetzen(Befund& b, const char* vorlage, std::vector<std::string> werte);
// alle Vorlagen, die der Optimierer benutzt (fuer die Uebersetzung und ihren Test)
extern const char* const kOptimiererDetailVorlagen[];
extern const std::size_t kOptimiererDetailVorlagenAnzahl;
// Was q3map2 aus dem Skript eines Shaders liest, das der Optimierer braucht
// (shaders.cpp ParseShaderFile: q3map_surfacelight, q3map_backsplash,
// surfaceparm sky / origin). text = der Inhalt der .shader-Datei.
struct ShaderSkript {
    bool gefunden = false;
    double flaechenlicht = 0.0;
    double backsplashAnteil = 0.05;
    double backsplashWeite = 23.0;
    bool himmel = false;
    bool ursprung = false;
    int lightmapBreite = 0;   // q3map_lightmapSize (shaders.cpp: si.lmCustomWidth/Height)
    int lightmapHoehe = 0;
    // Stufe 7 (Echtzeit-Licht): q3map_sun / q3map_sunExt <r> <g> <b> <Staerke> <Grad> <Hoehe>
    // (shaders.cpp:1049-1100) und q3map_lightRGB <r> <g> <b>
    bool sonne = false;
    double sonneFarbe[3] = {1, 1, 1};
    double sonneStaerke = 0;
    double sonneGrad = 0;
    double sonneHoehe = 0;
    bool lichtFarbe = false;
    double lichtRGB[3] = {1, 1, 1};
    // Stufe 9c (Echtzeit-Licht wie im Spiel): alle Sonnen ("sun" wie SOF2, q3map_sun,
    // q3map_sunExt), q3map_skyLight <Wert> <Schritte> [hMin hMax Farbe],
    // q3map_lightSubdivide, q3map_lightImage, die surfaceparms, die q3map2 fuer die
    // Schatten braucht (games.cpp game_sof2: trans, alphashadow, lightfilter, nodraw,
    // fog, ...), cull, autosprite und die erste Stufe (alphaFunc, blendFunc) fuer die
    // Darstellung wie im Spiel.
    struct Sonne {
        double farbe[3] = {1, 1, 1};
        double staerke = 0;
        double grad = 0;
        double hoehe = 0;
    };
    std::vector<Sonne> sonnen;
    struct Himmelslicht {
        double wert = 0;
        int schritte = 2;
        int horizontMin = 0;
        int horizontMax = 90;
        bool farbeAusHimmel = true;   // sample_color
    };
    std::vector<Himmelslicht> himmelsLichter;
    std::string himmelsBild;        // skyParms <farbox> (si.skyParmsImageBase)
    double lichtUnterteilung = 0;   // q3map_lightSubdivide
    std::string lichtBild;          // q3map_lightImage
    bool durchscheinend = false;    // C_TRANSLUCENT
    bool alphaSchatten = false;     // C_ALPHASHADOW
    bool lichtFilter = false;       // C_LIGHTFILTER
    bool nichtZeichnen = false;     // C_NODRAW (surfaceparm nodraw)
    bool nebel = false;             // C_FOG
    bool zweiseitig = false;        // cull none / twosided / disable (si.twoSided)
    bool autosprite = false;        // deformVertexes autosprite / autosprite2
    // erste Stufe (wie das Spiel sie zeichnet)
    int stufeAlpha = 0;             // alphaFunc: 0 keine, 1 GT0, 2 LT128, 3 GE128
    int mischQuelle = 0;            // blendFunc: 0 keine; sonst GL_-Faktoren (1 ONE, 2 ZERO, 3 SRC_ALPHA, 4 ONE_MINUS_SRC_ALPHA, 5 DST_COLOR, 6 ONE_MINUS_DST_COLOR, 7 SRC_COLOR, 8 ONE_MINUS_SRC_COLOR, 9 DST_ALPHA, 10 ONE_MINUS_DST_ALPHA)
    int mischZiel = 0;
    bool stufeGefunden = false;
};
[[nodiscard]] ShaderSkript shaderSkriptLesen(const std::string& text, const std::string& name);
// Eine .shader-Datei einmal zerlegt (Stufe 6, schnellerer Start des
// Optimierers): die Woerter und je Shader-Name (klein) das Wort, mit dem sein
// erster Block auf oberster Ebene beginnt. shaderSkriptLesen(datei, name)
// ergibt dasselbe wie shaderSkriptLesen(text, name), ohne die Datei je Name
// neu zu zerlegen.
struct ShaderDatei {
    std::vector<std::string> woerter;
    std::unordered_map<std::string, std::size_t> anfang;
};
[[nodiscard]] ShaderDatei shaderDateiZerlegen(const std::string& text);
[[nodiscard]] ShaderSkript shaderSkriptLesen(const ShaderDatei& datei, const std::string& name);

// Lichtstile in der Karte: Lichter mit "style"/"_style" (q3map2 light.cpp:433)
// oder mit targetname (schaltbar: q3map2 gibt ihnen in RBSP einen Stil, light.cpp:337).
[[nodiscard]] bool lichtstile(const KartenAbbild& a);
[[nodiscard]] bool lichtstileInKarte(const Karte& k);
// Schalter im Build-Menue, die mit diesem Spiel nicht gehen: externe
// Lightmaps (-external, -lightmapsize ungleich 128: light.cpp:2559-2584) bei
// Jedi Outcast (die Engine laedt keine, NRC#98) und bei Lichtstilen in jedem
// Spiel (lightmaps_ydnar.cpp:3109 "fixme: make this work with external
// lightmaps", NRC#48). befehle = (Eintrag, Befehlszeile).
struct BauBefehlProblem {
    std::string eintrag;
    std::string schalter;   // "-external", "-lightmapsize 256"
    std::string grund;      // Englisch, mit Quelle
};
[[nodiscard]] std::vector<BauBefehlProblem> bauBefehlProbleme(GrenzSpiel spiel, bool mitLichtstilen,
                                                              const std::vector<std::pair<std::string, std::string>>& befehle);

struct PruefOptionen {
    GrenzSpiel spiel = GrenzSpiel::JkaSp;
    double knappAb = 0.9;
    std::map<std::string, ShaderArt> shader;   // klein -> Art; fehlt ein Name: shaderArtNachName
    bool shaderBekanntGeprueft = false;        // "bekannt" stammt aus dem Inventar
    std::string linDaten;                      // die .lin (Leck), leer: kein Leck
    const BspAnalyse* bsp = nullptr;           // nach einem Bau
    const std::string* bspDaten = nullptr;
    std::vector<Bewertung> bewertungen;        // aus Punkt 2 (fuer Lightmaps, Modelle)
    int blockGroesse = 1024;                   // _blocksize (q3map2 bsp.cpp:260)
    long long portalSchwelle = 8000;           // VIS-Hinweis ab so vielen Portalen (Faustregel, Einstellung)
    long long dreieckSchwelle = 20000;         // Dreieck-Hinweis ab (r_speeds-Faustregel, Einstellung)
    std::vector<std::pair<std::string, double>>* zeiten = nullptr;   // je Pruefung die Dauer in ms (optional)
    std::vector<std::pair<std::string, std::string>> bauBefehle;     // Build-Menue: (Eintrag, Befehlszeile)
    double riesigAb = 16384.0;                 // Pinsel ab dieser Kantenlaenge melden (GT#271)
    bool lichterBehalten = false;              // Stufe 8a: "Editor-Daten behalten" (-keeplights) - Pruefung "enttext"
};
[[nodiscard]] std::vector<Befund> pruefeKarte(const KartenAbbild& a, const PruefOptionen& o);
[[nodiscard]] const char* schwereName(Schwere s);   // "note" / "warning" / "error"

// --- Beheben (Punkt 4) -----------------------------------------------------------

struct BehebungsErgebnis {
    bool ok = false;
    long long geaendert = 0;   // Flaechen, Pinsel, Patches, Entitaeten
    std::string undoName;      // der Rueckgaengig-Schritt
    std::string meldung;       // Hinweis (Englisch), leer wenn alles glatt ging
};
// Passt die Momentaufnahme noch zur Karte (Rueckgaengig-Stand gleich)?
[[nodiscard]] bool abbildPasst(const Karte& k, const KartenAbbild& a);
// Was die Behebung tun wird (Englisch, fuer die Vorschau).
[[nodiscard]] std::string behebungsVorschau(const Befund& b);
// Ein Befund, ein Rueckgaengig-Schritt ("Optimizer: ..."). Nur, wenn die
// Momentaufnahme noch passt (sonst ok = false: neu pruefen).
BehebungsErgebnis beheben(Karte& k, const KartenAbbild& a, const Befund& b);
// Jeder Befund mit "sicher" in EINEM Rueckgaengig-Schritt; je Befund das Ergebnis.
std::vector<std::pair<std::string, BehebungsErgebnis>> allesSichereBeheben(Karte& k, const KartenAbbild& a, const std::vector<Befund>& befunde);

}  // namespace bhed::karte

#endif
