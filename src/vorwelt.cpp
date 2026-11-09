// vorwelt.cpp - was das Spiel vor der Welt registriert (bhed/vorwelt.h, REST4 Punkt 2)
#include "bhed/vorwelt.h"

#include "bhed/regelwerk.h"

#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <utility>

namespace bhed::vorwelt {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool gleich(const std::string& a, const char* b) { return klein(a) == klein(b); }
// Q_stricmpn(b, a, n) == 0: die ersten n Zeichen ohne Gross/Klein
bool beginnt(const std::string& a, const char* b) {
    const std::string kb = klein(b);
    return klein(a).compare(0, kb.size(), kb) == 0 && a.size() >= kb.size();
}

}  // namespace

ComLeser::ComLeser(std::string text) : m_t(text.c_str()) {}

std::string ComLeser::wort(bool zeilenwechsel) {
    constexpr std::size_t kMaxToken = 1024;   // MAX_TOKEN_CHARS
    m_letztes.clear();
    if (m_ende) { return m_letztes; }
    const std::size_t n = m_t.size();
    bool neueZeile = false;
    int c = 0;
    for (;;) {
        while (m_p < n && static_cast<unsigned char>(m_t[m_p]) <= ' ') {   // SkipWhitespace (eurofix)
            if (m_t[m_p] == '\n') { neueZeile = true; }
            ++m_p;
        }
        if (m_p >= n) {
            m_ende = true;   // *data_p = NULL
            return m_letztes;
        }
        if (neueZeile && !zeilenwechsel) { return m_letztes; }
        c = m_t[m_p];
        if (c == '/' && m_p + 1 < n && m_t[m_p + 1] == '/') {
            m_p += 2;
            while (m_p < n && m_t[m_p] != '\n') { ++m_p; }
        } else if (c == '/' && m_p + 1 < n && m_t[m_p + 1] == '*') {
            m_p += 2;
            while (m_p < n && (m_t[m_p] != '*' || m_p + 1 >= n || m_t[m_p + 1] != '/')) { ++m_p; }
            if (m_p < n) { m_p += 2; }
        } else {
            break;
        }
    }
    if (c == '"') {
        ++m_p;
        for (;;) {
            c = m_p < n ? m_t[m_p] : 0;
            ++m_p;
            if (c == '"' || c == 0) {
                if (c == 0) { m_p = n; }
                return m_letztes;
            }
            if (m_letztes.size() < kMaxToken - 1) { m_letztes += static_cast<char>(c); }
        }
    }
    do {
        if (m_letztes.size() < kMaxToken - 1) { m_letztes += static_cast<char>(c); }
        ++m_p;
        c = m_p < n ? static_cast<int>(static_cast<signed char>(m_t[m_p])) : 0;
    } while (c > 32);
    return m_letztes;
}

void ComLeser::restDerZeile() {
    if (m_ende) { return; }
    while (m_p < m_t.size()) {
        if (m_t[m_p++] == '\n') { break; }
    }
}

void ComLeser::klammernUeberspringen() {
    int tiefe = !m_letztes.empty() && m_letztes[0] == '{' ? 1 : 0;   // "for tr_shader which just ate the brace"
    do {
        const std::string w = wort(true);
        if (w.size() == 1U) {
            if (w[0] == '{') { ++tiefe; }
            if (w[0] == '}') { --tiefe; }
        }
    } while (tiefe != 0 && !m_ende);
}

std::vector<std::string> npcKlassenEffekte(const std::string& classname, const std::string& npcTyp, int spawnflags, bool vorladen,
                                           int spiel) {
    using L = std::initializer_list<const char*>;
    std::vector<std::string> out;
    const auto dazu = [&out](L l) {
        for (const char* e : l) { out.emplace_back(e); }
    };
    const L galakMech = {"galak/trace_beam", "galak/beam_warmup", "env/med_explode2", "env/small_explode2", "galak/explode",
                         "blaster/smoke_bolton"};   // NPC_GalakMech_Precache (AI_GalakMech.cpp:79 MD9, codeJK2 :64)
    if (spiel == kRegelwerkJediOutcast) {
        // codeJK2 NPC_spawn.cpp: SP_NPC_Galak ruft NPC_GalakMech_Precache VOR SP_NPC_spawner, die Droiden ihr *_Precache danach -
        // ohne Bedingung (auch mit targetname); Effektnamen wie in codeJK2/game/AI_*.cpp
        if (gleich(classname, "NPC_Galak")) {
            if ((spawnflags & 1) != 0) { dazu(galakMech); }
        } else if (gleich(classname, "NPC_Droid_Interrogator")) {
            dazu({"droidexplosion1"});
        } else if (gleich(classname, "NPC_Droid_Probe")) {
            dazu({"probehead", "env/med_explode2", "probeexplosion1", "bryar/muzzle_flash"});
        } else if (gleich(classname, "NPC_Droid_Mark1")) {
            dazu({"env/med_explode2", "probeexplosion1", "blaster/smoke_bolton", "bryar/muzzle_flash", "droidexplosion1"});
        } else if (gleich(classname, "NPC_Droid_Mark2")) {
            dazu({"droidexplosion1", "env/med_explode2", "blaster/smoke_bolton", "bryar/muzzle_flash"});
        } else if (gleich(classname, "NPC_Droid_ATST")) {
            dazu({"env/med_explode2", "blaster/smoke_bolton", "droidexplosion1"});
        } else if (gleich(classname, "NPC_Droid_Remote") || gleich(classname, "NPC_Droid_Seeker") || gleich(classname, "NPC_Droid_Mouse")) {
            dazu({"env/small_explode"});
        } else if (gleich(classname, "NPC_Droid_Sentry")) {
            dazu({"bryar/muzzle_flash", "env/med_explode"});
        } else if (gleich(classname, "NPC_Droid_Gonk") || gleich(classname, "NPC_Droid_R2D2") || gleich(classname, "NPC_Droid_Protocol")) {
            dazu({"env/med_explode"});
        } else if (gleich(classname, "NPC_Droid_R5D2")) {
            dazu({"env/med_explode", "droid_smoke", "r5d2head"});
        }
        return out;
    }
    const bool md = spiel == kRegelwerkMovieDuels;
    // Movie Duels SP_NPC_Galak (NPC_spawn.cpp MD9): mit spawnflags 1 NPC_GalakMech_Precache vor SP_NPC_spawner, immer
    if (md && gleich(classname, "NPC_Galak") && (spawnflags & 1) != 0) { dazu(galakMech); }
    if (!vorladen || npcTyp.empty()) { return out; }
    // NPC_PrecacheByClassName (NPC_stats.cpp:1440-1570 OpenJK, MD9 :1451-1590), Effekte aus code/game/AI_*.cpp
    const std::string& t = npcTyp;
    if (gleich(t, "gonk") || beginnt(t, "r2d2") || gleich(t, "protocol")) {
        dazu({"env/med_explode"});
    } else if (gleich(t, "mouse") || gleich(t, "seeker") || beginnt(t, "remote")) {
        dazu({"env/small_explode"});
    } else if (gleich(t, "atst")) {
        dazu({"env/med_explode2", "blaster/smoke_bolton", "explosions/droidexplosion1"});
    } else if (beginnt(t, "r5d2")) {
        dazu({"env/med_explode", "volumetric/droid_smoke", "chunks/r5d2head"});
    } else if (gleich(t, "mark1")) {
        dazu({"env/med_explode2", "explosions/probeexplosion1", "blaster/smoke_bolton", "bryar/muzzle_flash", "explosions/droidexplosion1"});
    } else if (gleich(t, "mark2")) {
        dazu({"explosions/droidexplosion1", "env/med_explode2", "blaster/smoke_bolton", "bryar/muzzle_flash"});
    } else if (gleich(t, "interrogator")) {
        dazu({"explosions/droidexplosion1"});
    } else if (gleich(t, "probe")) {
        dazu({"chunks/probehead", "env/med_explode2", "explosions/probeexplosion1", "bryar/muzzle_flash"});
    } else if (gleich(t, "howler")) {
        dazu({"howler/sonic"});
    } else if (gleich(t, "mutant_rancor")) {
        dazu({"mrancor/breath"});
    } else if (gleich(t, "sand_creature")) {
        dazu({"env/sand_dive", "env/sand_spray", "env/sand_move", "env/sand_move_breach"});
    } else if (gleich(t, "sentry")) {
        dazu({"bryar/muzzle_flash", "env/med_explode"});
    } else if (md && gleich(t, "galak_mech")) {
        dazu(galakMech);
    } else if (gleich(t, "boba_fett")) {
        // Boba_Precache (AI_BobaFett.cpp:224 OpenJK, :233 MD9)
        if (md) {
            dazu({"boba/jetSP", "boba/fthrw", "flamethrower/flame_impact", "volumetric/black_smoke", "chunks/dustFall", "rockettrooper/flameNEW"});
        } else {
            dazu({"boba/jetSP", "boba/fthrw", "volumetric/black_smoke", "chunks/dustFall"});
        }
    } else if (gleich(t, "rockettrooper2") || gleich(t, "rockettrooper2Officer")) {
        dazu({"rockettrooper/flameNEW", "rockettrooper/light_cone"});
    } else if (gleich(t, "tavion_scepter")) {
        dazu({"scepter/beam_warmup.efx", "scepter/beam.efx", "scepter/slam_warmup.efx", "scepter/slam.efx", "scepter/impact.efx"});
    } else if (gleich(t, "tavion_sith_sword")) {
        dazu({"scepter/recharge.efx", "scepter/invincibility.efx", "scepter/sword.efx"});
    } else if (gleich(t, "rosh_dark")) {
        dazu({"force/kothos_recharge.efx", "force/kothos_beam.efx"});
    } else if (gleich(t, "cultist_destroyer")) {
        dazu({"force/destruction_exp"});
    }
    // shadowtrooper*, minemonster, rancor, wampa, tusken*, saboteur*, jawa*: nur Klaenge
    return out;
}

std::string fahrzeugTexte(const std::vector<std::string>& dateien) {
    // BG_VehicleLoadParms / BG_VehWeaponLoadParms (bg_vehicleLoad.cpp): jede Datei (tempReadBuffer[len] = 0) mit strcat
    // angehaengt; endet das Bisherige auf '}', erst ein Leerzeichen ("should be a stand-alone token")
    std::string t;
    for (const std::string& d : dateien) {
        if (!t.empty() && t.back() == '}') { t += ' '; }
        t += d.c_str();
    }
    return t;
}

FahrzeugLaden fahrzeugLaden(const std::string& vehText, const std::string& vwpText, const std::string& name) {
    FahrzeugLaden out;
    // VF_EFFECT / VF_EFFECT_CLIENT der Fahrzeuge (vehicleFields, bg_vehicleLoad.cpp:480-660 OpenJK, MD9 dieselben Namen)
    static const char* const kEffekte[] = {"exhaustFX", "turboFX", "turboStartFX", "trailFX",   "impactFX",   "explodeFX",  "wakeFX",
                                           "dmgFX",     "injureFX", "noseFX",      "lwingFX",   "rwingFX",    "armorLowFX", "armorGoneFX"};
    static const char* const kWaffenEffekte[] = {"muzzleFX", "shotFX", "impactFX"};   // vehWeaponFields (:160-200)
    const auto block = [](ComLeser& l, const std::string& n) {
        for (;;) {   // "look for the right vehicle"
            const std::string w = l.wort(true);
            if (w.empty()) { return false; }
            if (klein(w) == klein(n)) { break; }
            l.klammernUeberspringen();
        }
        return l.wort(true) == "{";
    };
    std::string modell = "models/map_objects/ships/swoop.md3";   // BG_VehicleSetDefaults
    std::string haut;
    bool hautDa = false;
    int explosionsSchaden = 1;   // VEH_DEFAULT_EXP_DMG (bg_vehicles.h) ist nicht 0
    float schwebeHoehe = 0.0F;
    std::string waffe[2];
    ComLeser l(vehText);
    if (name.empty() || !block(l, name)) { return out; }
    for (;;) {
        l.restDerZeile();
        const std::string schluessel = l.wort(true);
        if (schluessel.empty()) { return out; }   // VEHICLE_NONE - was registriert ist, bleibt
        if (schluessel == "}") { break; }
        const std::string wert = l.wort(true);
        if (wert.empty()) { continue; }
        const std::string k = klein(schluessel.substr(0, 127));
        if (k == "weap1" || k == "weap2") {
            waffe[k == "weap1" ? 0 : 1] = wert.substr(0, 127);
            continue;
        }
        for (const char* e : kEffekte) {
            if (k == klein(e)) { out.effekte.push_back(wert); }
        }
        if (k == "model") { modell = wert; }
        if (k == "skin") {
            haut = wert;
            hautDa = true;
        }
        if (k == "explosiondamage") { explosionsSchaden = std::atoi(wert.c_str()); }
        if (k == "hoverheight") { schwebeHoehe = static_cast<float>(std::atof(wert.c_str())); }
    }
    out.gefunden = true;
    // "NOW: if we have any weapons, go ahead and load them" - VF_WEAPON -> VEH_VehWeaponIndexForName -> VEH_LoadVehWeapon
    std::vector<std::string> geladen;
    for (const std::string& w : waffe) {
        if (w.empty()) { continue; }
        bool schon = false;
        for (const std::string& g : geladen) { schon = schon || klein(g) == klein(w); }
        if (schon) { continue; }
        geladen.push_back(w);
        ComLeser wl(vwpText);
        if (!block(wl, w)) { continue; }
        for (;;) {
            wl.restDerZeile();
            const std::string schluessel = wl.wort(true);
            if (schluessel.empty() || schluessel == "}") { break; }
            const std::string wert = wl.wort(true);
            if (wert.empty()) { continue; }
            for (const char* e : kWaffenEffekte) {
                if (klein(schluessel.substr(0, 127)) == klein(e)) { out.effekte.push_back(wert); }
            }
        }
    }
    // Haeute (#ifndef _JK2MP): jede aus der Liste "a|b" (ratl-Tokenizer, leere Teile fallen weg), sonst model_default.skin -
    // RE_RegisterSkin sofort ("this will just turn off surfs ...")
    if (hautDa && !haut.empty()) {
        std::size_t a = 0;
        while (a <= haut.size()) {
            const std::size_t b = haut.find('|', a);
            const std::string teil = haut.substr(a, b == std::string::npos ? std::string::npos : b - a);
            if (!teil.empty()) { out.haeute.push_back("models/players/" + modell + "/model_" + teil + ".skin"); }
            if (b == std::string::npos) { break; }
            a = b + 1;
        }
    } else {
        out.haeute.push_back("models/players/" + modell + "/model_default.skin");
    }
    if (explosionsSchaden != 0) { out.effekte.emplace_back("ships/ship_explosion_mark"); }
    if (schwebeHoehe > 0.0F) { out.effekte.emplace_back("ships/swoop_dust"); }
    out.effekte.emplace_back("volumetric/black_smoke");   // QAGAME: immer
    out.effekte.emplace_back("ships/fire");
    return out;
}

std::vector<Schritt> spielstart(int spiel) {
    std::vector<Schritt> s;
    // CL_InitRenderer (client/cl_main.cpp:932-944 OpenJK, MD9 :1050-1060): re.BeginRegistration -> R_Init -> R_InitShaders mit
    // CreateExternalShaders (tr_shader.cpp:3863-3869: projectionShadow, sun), dann Zeichensatz, Konsolenschrift (nur OpenJK
    // und JK2_MODE: "ocr_a"), white, console
    s.push_back({Art::Shader, "projectionShadow"});
    s.push_back({Art::Shader, "sun"});
    s.push_back({Art::ShaderNoMip, "gfx/2d/charsgrid_med"});
    if (spiel != kRegelwerkMovieDuels) { s.push_back({Art::Schrift, "ocr_a"}); }
    s.push_back({Art::Shader, "white"});
    s.push_back({Art::Shader, "console"});
    // CL_InitUI -> _UI_Init(inGameLoad qfalse) (ui/ui_main.cpp:2633-2752 OpenJK, MD9 :5600-5710): UI_LoadMenus(ui_menuFiles
    // "ui/menus.txt"), white, AssetCache, in Movie Duels (NEW_FEEDER, q_shared.h:319) danach die Rahmen der Figurenwahl
    s.push_back({Art::Menues, "ui/menus.txt"});
    s.push_back({Art::ShaderNoMip, "white"});
    for (const char* a : {"gfx/menus/scrollbar.tga", "gfx/menus/scrollbar_arrow_dwn_a.tga", "gfx/menus/scrollbar_arrow_up_a.tga",
                          "gfx/menus/scrollbar_arrow_left.tga", "gfx/menus/scrollbar_arrow_right.tga", "gfx/menus/scrollbar_thumb.tga",
                          "menu/new/slider", "menu/new/sliderthumb"}) {
        s.push_back({Art::ShaderNoMip, a});
    }
    if (spiel == kRegelwerkMovieDuels) {
        for (const char* a : {"anakin", "jk", "katarn", "kylo", "luke", "obiwan", "oldrepublic", "quigon", "rey", "vader", "windu"}) {
            s.push_back({Art::ShaderNoMip, std::string("gfx/menus/cursor_") + a + ".tga"});   // AssetCache MD9 :6685-6695
        }
        for (const char* a : {"gfx/menus/charactermenuicons/icon_border.tga", "gfx/menus/charactermenuicons/icon_border_gold.tga",
                              "gfx/menus/charactermenuicons/icon_background.tga"}) {
            s.push_back({Art::ShaderNoMip, a});
        }
    }
    return s;
}

std::vector<Schritt> kartenStart(int /*spiel*/) {
    // RE_SVModelInit (rd-vanilla tr_init.cpp:1935-1945): R_InitImages (die Bilder bleiben), R_InitShaders
    return {{Art::ShaderVergessen, ""}, {Art::Shader, "projectionShadow"}, {Art::Shader, "sun"}};
}

std::vector<Schritt> nachDemServer(int spiel) {
    // CL_MapLoading -> CL_FlushMemory hat Renderer und Oberflaeche beendet (rendererStarted/uiStarted qfalse, Texturen bleiben:
    // R_DeleteTextures nur mit destroyWindow); das naechste SCR_UpdateScreen -> CL_StartHunkUsers (cl_scrn.cpp:490) startet
    // beide neu - UI_Init mit inGameLoad (cls.state zwischen DISCONNECTED und ACTIVE, cl_ui.cpp:312): "ui/ingame.txt"
    // (unter JK2_MODE weiter ui_menuFiles, ui_main.cpp:2737-2747)
    std::vector<Schritt> s = {{Art::ShaderVergessen, ""}};
    for (const Schritt& x : spielstart(spiel)) {
        s.push_back(x.art == Art::Menues && spiel != kRegelwerkJediOutcast ? Schritt{Art::Menues, "ui/ingame.txt"} : x);
    }
    return s;
}

std::vector<Schritt> cgInit(int spiel) {
    std::vector<Schritt> s;
    const bool jo = spiel == kRegelwerkJediOutcast;
    // CG_Init (cgame/cg_main.cpp:2108-2160 OpenJK, MD9 :3365-3421, codeJK2 :1690-1735)
    s.push_back({Art::ShaderNoMip, "gfx/2d/charsgrid_med"});
    s.push_back({Art::Schrift, "ocr_a"});
    s.push_back({Art::Schrift, "ergoec"});
    s.push_back({Art::Shader, "white"});
    s.push_back({Art::ShaderNoMip, "gfx/hud/load_tick"});
    s.push_back({Art::ShaderNoMip, "gfx/hud/load_tick_cap"});
    if (jo) {
        for (const char* f : {"heal", "levitation", "speed", "push", "pull", "telepathy", "grip", "l1", "saber_throw", "saber_defend",
                              "saber_attack"}) {
            s.push_back({Art::ShaderNoMip, std::string("gfx/hud/f_icon_") + f});
        }
    } else {
        std::vector<const char*> f = {"lt_heal", "levitation", "speed", "push", "pull", "lt_telepathy", "dk_grip", "dk_l1", "saber_throw",
                                      "saber_defend", "saber_attack", "dk_rage", "lt_protect", "lt_absorb", "dk_drain", "sight"};
        if (spiel == kRegelwerkMovieDuels) {
            for (const char* x : {"dk_destruction", "dk_stasis", "lt_grasp", "lt_repulse", "dk_lightning_strike", "dk_fear", "dk_deadlysight",
                                  "lt_projection", "lt_blast"}) {
                f.push_back(x);
            }
        }
        for (const char* x : f) { s.push_back({Art::ShaderNoMip, std::string("gfx/mp/f_icon_") + x}); }
        if (spiel == kRegelwerkMovieDuels) {
            s.push_back({Art::ShaderNoMip, "gfx/mp/new_f_icon_dk_stasis"});
            s.push_back({Art::ShaderNoMip, "gfx/hud/i_icon_flame"});
        }
    }
    // CG_LoadHudMenu: cg_hudFiles, leer "ui/jahud.txt" (codeJK2 "ui/jk2hud.txt")
    s.push_back({Art::Menues, jo ? "ui/jk2hud.txt" : "ui/jahud.txt"});
    // CG_GameStateReceived: levelLoad, CM_LoadMap, CG_RegisterSounds (Modell und Effekte darin), dann CG_RegisterGraphics
    s.push_back({Art::ShaderNoMip, "gfx/hud/mp_levelload"});
    if (!jo) { s.push_back({Art::Effekt, "env/slide_dust"}); }
    s.push_back({Art::Modell, "models/items/psgun.glm"});
    s.push_back({Art::Effekt, "turret/explode"});
    s.push_back({Art::Effekt, jo ? "spark_exp_nosnd" : "sparks/spark_exp_nosnd"});
    if (jo) { s.push_back({Art::Effekt, "water_impact"}); }
    return s;
}

}  // namespace bhed::vorwelt
