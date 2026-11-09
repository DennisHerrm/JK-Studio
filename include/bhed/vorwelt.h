// vorwelt.h - was das Spiel vor der Welt registriert (REST4 Punkt 2)
//
// Das Register "ein Bild je Name" (bhed/bildregister.h) haengt an der Reihenfolge: wer ein Bild zuerst laedt, bestimmt
// mipmap, allowPicmip, allowTC und Klammern fuer alle (R_FindImageFile_NoLoad, rd-vanilla tr_image.cpp:936-971). Hier
// stehen die Aufrufe, die das Spiel bei einem frischen Start bis zur Welt macht, je Regelwerk aus den Quellen (OpenJK
// code/, Movie Duels Update 9 code/, Jedi Outcast codeJK2/ mit der OpenJK-Oberflaeche unter JK2_MODE). Die Vorschau hat
// keine vorige Karte - sie laedt jede Karte wie nach einem frischen Spielstart (AllocatedImages ueberlebt im Spiel den
// Kartenwechsel, R_ImageList_LevelLoadEnd raeumt nur, was die neue Karte nicht braucht; ohne vorige Karte gibt es nichts).
#ifndef BHED_VORWELT_H
#define BHED_VORWELT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed::vorwelt {

// COM_ParseExt / SkipRestOfLine / SkipBracedSection (qcommon/q_shared.cpp:188-545 OpenJK, MD9 gleich) ueber einem Text,
// der wie der Puffer der Engine am ersten NUL endet.
class ComLeser {
public:
    explicit ComLeser(std::string text);
    // COM_ParseExt: Leerraum (unsigned <= ' '), // und /* */ ueberspringen; ohne allowLineBreaks endet es leer an einem
    // Zeilenwechsel; "..." am Stueck, sonst bis zu einem Zeichen <= 32 (char). Leer am Ende (data_p NULL).
    std::string wort(bool zeilenwechsel);
    void restDerZeile();          // SkipRestOfLine
    void klammernUeberspringen();  // SkipBracedSection (mit dem zuletzt gelesenen Wort als com_token)
    [[nodiscard]] bool ende() const noexcept { return m_ende; }

private:
    std::string m_t;
    std::size_t m_p = 0;
    bool m_ende = false;
    std::string m_letztes;   // com_token
};

// SP_NPC_spawner -> NPC_PrecacheByClassName (NPC_stats.cpp:1440-1570 OpenJK, MD9 :1451-1590) bzw. die Klassenfunktionen von
// Jedi Outcast (codeJK2 NPC_spawn.cpp: SP_NPC_Droid_* rufen ihr *_Precache nach SP_NPC_spawner, immer): die Effekte, die
// G_EffectIndex dabei in CS_EFFECTS eintraegt, in ihrer Folge. `vorladen`: der Spawner hat weder targetname noch delay > 0
// (sonst SVF_NPC_PRECACHE, und CG_NPC_Precache laeuft erst nach der Welt). `classname` fuer SP_NPC_Galak (spawnflags 1).
[[nodiscard]] std::vector<std::string> npcKlassenEffekte(const std::string& classname, const std::string& npcTyp, int spawnflags,
                                                         bool vorladen, int spiel);

// VEH_LoadVehicle im Einzelspieler (game/bg_vehicleLoad.cpp:1021-1395 OpenJK, MD9 gleich gebaut, QAGAME): die Effekte
// (VF_EFFECT/VF_EFFECT_CLIENT -> G_EffectIndex, auch aus den Waffen weap1/weap2 der .vwp, dazu die festen am Ende) und die
// Haeute, die RE_RegisterSkin sofort laedt; Shader (VF_SHADER*) registriert das Spielmodul nicht. `veh` und `vwp` sind
// die Dateien in der Folge, in der BG_VehicleLoadParms / BG_VehWeaponLoadParms sie aneinanderhaengen.
struct FahrzeugLaden {
    bool gefunden = false;
    std::vector<std::string> effekte;
    std::vector<std::string> haeute;
};
[[nodiscard]] std::string fahrzeugTexte(const std::vector<std::string>& dateien);
[[nodiscard]] FahrzeugLaden fahrzeugLaden(const std::string& vehText, const std::string& vwpText, const std::string& name);

// Feste Registrierungen in der Reihenfolge der Engine.
enum class Art : std::uint8_t {
    ShaderNoMip,     // RE_RegisterShaderNoMip: R_FindShader(..., qfalse)
    Shader,          // RE_RegisterShader / R_FindShader(..., qtrue)
    Schrift,         // RE_RegisterFont: Shader "fonts/<Name ohne Pfad>" ohne Mipmaps (tr_font.cpp:1070-1072), auch ohne .fontdat
    Modell,          // RE_RegisterModel
    Effekt,          // CFxScheduler::RegisterEffect
    Menues,          // UI_LoadMenus / CG_LoadMenus: die Liste `name` mit allen Menues darin
    ShaderVergessen  // R_InitShaders: die Shadertabelle beginnt neu, die Bilder bleiben (R_InitImages raeumt nichts)
};
struct Schritt {
    Art art;
    std::string name;
};
// CL_InitRenderer und UI_Init beim Programmstart (inGameLoad false): R_Init, Zeichensatz, Konsole, menus.txt, AssetCache.
[[nodiscard]] std::vector<Schritt> spielstart(int spiel);
// SV_SpawnServer: RE_SVModelInit (R_InitShaders, CreateExternalShaders) vor den Entities.
[[nodiscard]] std::vector<Schritt> kartenStart(int spiel);
// Nach dem Server, vor dem Spielstand: CL_StartHunkUsers (SCR_UpdateScreen) - CL_InitRenderer und UI_Init(inGameLoad).
[[nodiscard]] std::vector<Schritt> nachDemServer(int spiel);
// CG_Init bis vor CG_RegisterEffects: Schriften, HUD-Menues, gfx/hud/mp_levelload, Modelle und Effekte aus CG_RegisterSounds.
[[nodiscard]] std::vector<Schritt> cgInit(int spiel);

}  // namespace bhed::vorwelt

#endif  // BHED_VORWELT_H
