// modell.h - Menue-Editor M2 Punkt 1: 3D-Modelle in Menues (ITEM_TYPE_MODEL).
// Portiert aus OpenJK (code/ui/ui_shared.cpp, code/ui/ui_saber.cpp, code/rd-vanilla/tr_light.cpp, tr_surface.cpp) und Movie Duels (ui_saber.cpp).
//
// Modell, Haut, Skelett und Animation kommen aus der vorhandenen
// Figurendarstellung (readGlm/applySkin/readGla/parseAnimationCfg/
// worldMatrices, gezeichnet von gpu::zeichneMenueFigur mit Skinning auf der
// Grafikkarte). Kamera und Licht sind aus OpenJK portiert:
//   code/ui/ui_shared.cpp Item_Model_Paint (ohne JK2_MODE): refdef mit
//     RDF_NOWORLDMODEL ueber rect + 1 / - 2, fov_x = width / 640 * 90 wenn
//     nicht gesetzt, fov_y = atan2(height, width / tan(fov_x / 2)), Grenzen
//     g2mins/g2maxs (sonst -16 -16 -24 / 16 16 32), origin = (len / 0.268,
//     Mitte y, -Mitte z), Drehung angle + realTime / rotationSpeed, ein Licht
//     500 (weiss) am Augpunkt (R_AddLightToScene);
//   ItemParse_asset_model: "ui_char_model" -> models/players/<g_char_model>/model.glm
//     (in der Engine fest verdrahtet); model_g2skin mit "|" (R_RegisterSkin) wie ihn
//     UI_UpdateCharacterSkin setzt: models/players/x/|head|torso|legs;
//   Saeber (isSaber/isSaber2 ohne isCharacter) liegen auf der Seite: (curYaw, 0, 90);
//   code/rd-vanilla/tr_light.cpp R_SetupEntityLighting ohne Weltmodell:
//     Umgebung und Richtung 150 * identityLight (+32 Umgebung), tr.sunDirection
//     ist ohne geladene Karte null, die Lichter addieren DLIGHT_AT_RADIUS *
//     r^2 / d^2 (d mindestens 16), Richtung in Entity-Achsen;
//   r_znear 4. identityLight = 1 (ohne Overbright wie im Fenster).
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
#ifndef BHED_MENUE_MODELL_H
#define BHED_MENUE_MODELL_H

#include "bhed/glm.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/spieldaten.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

struct MenueModell {
    std::string schluessel;   // Modell|Haut|Animation - neu laden, wenn er sich aendert
    bool ok = false;
    GlmModel glm;
    GlaAnimation gla;
    std::vector<AnimEntry> anims;
    std::string fehler;
    // M5b: ein .md3 (z. B. das Jedi-Outcast-Logo im Hauptmenue) - Bild 0 als starres Modell mit
    // einem Knochen; mins/maxs wie R_ModelBounds (DC->modelBounds) fuer die Kamera
    bool md3 = false;
    float mins[3]{}, maxs[3]{};
    // M5b: Jedi Outcast SP (Item_Model_Paint unter JK2_MODE): fov 45 x 45, Drehung realTime / 20 Grad,
    // model_angle/model_rotation/model_fov* ohne Wirkung
    bool jk2Kamera = false;
};

// Cvar-Lesen fuer das Item "character" (ui_char_model, ui_char_skin_*)
using CvarLeser = std::function<std::string(const std::string&)>;

// Was das Item zeigt: Modellpfad, Haut, Animation (leer = Grundstellung).
[[nodiscard]] std::string modellSchluessel(const Item& it, const CvarLeser& cvar);
// Modell, Haut, Skelett und animation.cfg aus den Spieldaten.
bool ladeMenueModell(const Spieldaten& d, const Item& it, const CvarLeser& cvar, MenueModell& out);

// Kamera, Licht und Knochen eines Bildes (Item_Model_Paint).
struct MenueModellBild {
    float viewProj[16]{};    // zeilenweise, NDC wie Direct3D (z 0..1), Entity schon eingerechnet
    float kamera[16]{};      // dasselbe ohne Entity (Weltpunkte, z. B. Klingen)
    float umgebung[3]{};     // 0..1
    float gerichtet[3]{};    // 0..1 (darf ueber 1 liegen, die Engine klemmt erst die Farbe)
    float richtung[3]{};     // in Entity-Achsen (Modellraum)
    float yawGrad = 0.0F;
    float achsen[3][3]{};    // ent.axis (vorn, links, oben)
    float ursprung[3]{};     // ent.origin
    int bild = 0;            // Bild der .gla
    int folge = 0;           // M3: Folgebild und dessen Anteil (G2_TimingModel)
    float anteil = 0;
};
// breite/hoehe: Bildpunkte des refdef (rect.w - 2 bzw. rect.h - 2 mal Massstab).
[[nodiscard]] MenueModellBild menueModellBild(const MenueModell& m, const Item& it, int zeitMs, int animSeitMs, int breite, int hoehe,
                                              std::vector<BoneMatrix>& weltKnochen);

// --- M3 Punkt 2: Haltung der Figur wie im Spiel -----------------------------------------
//
// UI_G2SetAnim (OpenJK code/ui/ui_main.cpp): Bilder firstFrame .. firstFrame + numFrames,
// animSpeed = 50 / frameLerp (UI_ParseAnimFileSet: frameLerp = ceil(1000 / fps), bei
// negativen fps floor), BONE_ANIM_OVERRIDE_LOOP, wenn loopFrames != -1, sonst
// BONE_ANIM_OVERRIDE; einsetzend zur Zeit DC->realTime des Setzens. Daraus G2_TimingModel
// (code/rd-vanilla/tr_ghoul2.cpp): Bild, Folgebild und Anteil des Folgebildes (die Engine
// mischt die lokalen Knochenmatrizen linear). aus = nicht geschleift und zu Ende - G2 nimmt
// die Animation weg (flags &= ~BONE_ANIM_TOTAL), die Figur steht dann in Bild 0.
struct G2Bild {
    int bild = 0;
    int folge = 0;
    float anteil = 0;
    bool aus = false;
};
[[nodiscard]] G2Bild g2Bild(const AnimEntry& a, double seitMs);

// --- Klingen der Menue-Saeber (code/ui/ui_saber.cpp, code/rd-vanilla/tr_surface.cpp) ---
//
// UI_SaberDrawBlade: Bolzen "*blade<N+1>", sonst "*flash" (tagHack) mit den
// Versaetzen je saberType - im Menue mal DC->xscale (Bildschirmbreite / 640),
// sie wachsen also mit der Aufloesung; ohne beide Bolzen die Einheitsmatrix
// (G2API_GetBoltMatrix mit ungueltigem Bolzen). Richtung NEGATIVE_X, rechts
// NEGATIVE_Y, oben POSITIVE_Z der Bolzenmatrix. org/dir im Raum des Griffs.
// false fuer SABER_SITH_SWORD (zeichnet keine Klinge).
[[nodiscard]] bool uiKlingenStrecke(const GlmModel& griff, const std::string& saberTyp, int nummer, float xscale, float org[3],
                                    float dir[3]);
// UI_SaberShouldDrawBlade: ab bladeStyle2Start gilt noBlade2, sonst noBlade.
[[nodiscard]] bool uiKlingeZeichnen(int nummer, int bladeStyle2Start, bool noBlade, bool noBlade2);

// Ein Viereck einer Klinge, Ecken in der Reihenfolge der Bildkoordinaten
// (0,0) (1,0) (1,1) (0,1).
struct UiKlingenViereck {
    float ecke[4][3]{};
    bool kern = false;   // RT_LINE mit <farbe>_line, sonst RT_SABER_GLOW mit <farbe>_glow
};
// Was UI_DoSaber (bzw. die Movie-Duels-Arten) der Szene uebergibt. Die
// Zufallsanteile (Q_flrand, crandoms) nehmen ihre Mitte.
struct UiKlingenMasse {
    bool da = false;            // unter 0.5 Laenge: nichts
    bool glanz = false;         // RT_SABER_GLOW eingetragen
    float glanzRadius = 0;      // saber.radius des Glanzes
    float glanzLaenge = 0;      // saber.saberLength
    float glanzStaerke = 1;     // shaderRGBA / 255 des Glanzes (Movie Duels 0.8)
    float griffRadius = 0;      // RB_SurfaceSaberGlow: der Griff-Klecks (Renderer)
    float kernRadius = 0;       // RT_LINE: halbe Breite
    int kernMal = 1;            // Movie Duels SFX traegt den Kern zweimal ein
    float glanzVersatz = 0;     // Movie Duels Rebels/CW/Maul: Glanz beginnt so weit vorn
};
// OpenJK code/ui/ui_saber.cpp UI_DoSaber: radiusmult 1 + 2 / Laenge, wenn kuerzer als
// laengeMax; Glanz (radius - 7.5 %) x radiusmult, Kern radius / 3 x radiusmult;
// rd-vanilla RB_SurfaceSaberGlow: Griff-Klecks 5.5 + Q_flrand(0, 1) * 0.25.
[[nodiscard]] UiKlingenMasse uiKlingenMasseOpenJK(float laenge, float laengeMax, float radius);
// Movie Duels code/ui/ui_saber.cpp UI_SaberDrawBlade: welche UI_Do*Saber-Funktion die Klinge
// zeichnet - 0 UI_DoSaber (cg_SFXSabers < 1, "Raven blade"), 1 Ep1, 2 Ep2, 3 Ep3, 4 SFX
// (Vorgabe), 5 OT, 6 RotJ, 7 TFA, 8 Unstable, 9 Rebels, 10 CW, 11 Maul; die instabilen
// Saeberarten (SABER_UNSTABLE, SABER_STAFF_UNSTABLE, SABER_STAFF_ELECTROSTAFF,
// SABER_SINGLE_KYLO_REN, SABER_ELECTROSTAFF) immer 12 = UI_DoSaberUnstable; ueber 11
// keine Klinge (-1, switch ohne Fall).
inline constexpr int kMdKlingeInstabil = 12;
[[nodiscard]] int mdKlingenArt(int sfxSabers, const std::string& saberTyp);
// Die Groessen-Cvars der Art (cg_SFXSabersGlowSize<EP1|...>, cg_SFXSabersCoreSize...);
// "" = die Art liest keine (UI_DoSaberUnstable).
[[nodiscard]] std::string mdGroessenCvar(int art, bool kern);
// TranslateSaberColor und das switch (color) der Art: Glanz- und Kern-Shader; kern2 = der
// zweite Kern, den UI_DoSaber/Rebels/CW/Maul bei genau "rgb" (SABER_RGB) noch eintragen.
// rgbWerte = die Farbe ist >= SABER_RGB (rgb oder RGB-Werte): Glanzfarbe aus ui_rgb_saber*.
struct MdKlingenShader {
    std::string glanz, kern, kern2;
    bool rgbWerte = false;
};
// serenity = g_SerenityJediEngineMode (UI_DoUnstableSaber nimmt dann sabers_tfa/unstable_line).
[[nodiscard]] MdKlingenShader mdKlingenShader(int art, const std::string& farbe, bool serenity = false);
// Die Masse der Art (Zufallsanteile crandoms/Q_flrand/flrand: ihre Mitte):
//   effectradius = (radius * 1.6 + Zufall) * radiusmult * Glanzgroesse * Glanzfaktor,
//   coreradius = (radius * 0.4 + Zufall) * radiusmult * Kerngroesse * Kernfaktor;
//   Glanz nur, wenn laenge - effectradius / 2 > 0, Laenge laenge - effectradius / 2,
//   Staerke 0.8; Kern zweimal (SFX-Familie) bzw. einmal (UI_DoSaber, Rebels, CW, Maul),
//   Rebels/CW/Maul setzen den Glanz um effectradius / 4 nach vorn (glanzVersatz).
// Movie-Duels-Renderer RB_SurfaceSaberGlow: Griff-Klecks 2.5 + Q_flrand(0, 0.5) * 0.15.
[[nodiscard]] UiKlingenMasse uiKlingenMasseMD(int art, float laenge, float laengeMax, float radius, float glanzGroesse, float kernGroesse);
// Die Vierecke: RB_SurfaceSaberGlow legt von glanzLaenge zur Wurzel alle
// radius * 0.65 ein Sprite (DoSprite, der Radius waechst je Sprite um 0.017),
// dann den Griff-Klecks; der Kern RT_LINE geht von der Spitze bis 1 hinter die
// Wurzel, quer zu Auge und Klinge (RB_SurfaceLine, DoLine), kernMal-mal.
// auge = viewParms.ori.origin, links/oben = viewParms.ori.axis[1]/[2]
// (im Menue: Ursprung, (0,1,0), (0,0,1)).
void uiSaberVierecke(const float org[3], const float dir[3], float laenge, const UiKlingenMasse& m, const float auge[3], const float links[3],
                     const float oben[3], std::vector<UiKlingenViereck>& aus);

}  // namespace bhed::menue

#endif  // BHED_MENUE_MODELL_H
