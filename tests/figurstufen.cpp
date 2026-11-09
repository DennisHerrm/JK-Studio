// Probe fuer bhed/figurstufen.h (rc602): die Shaderstufen einer
// Figurenflaeche wie ComputeColors/ComputeTexCoords der Engine.
#include "bhed/figurstufen.h"
#include "bhed/shaderscript.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}
bool nah(float a, float b) { return std::fabs(a - b) < 1e-4F; }
}  // namespace

int main() {
    // rc604: ein fehlendes Stufenbild wie R_FindShader/R_LoadMDXM.
    {
        const FigurShaderErsatz zweite = figurShaderMitFehlendemBild(1, 3, true);
        const FigurShaderErsatz erste = figurShaderMitFehlendemBild(0, 3, true);
        const FigurShaderErsatz glm = figurShaderMitFehlendemBild(2, 3, false);
        const FigurShaderErsatz alle = figurShaderMitFehlendemBild(3, 3, false);
        pruefe(zweite.stufen == 1U && !zweite.ersatzbild && !zweite.unsichtbar,
               "Bild der zweiten Stufe fehlt (Haut): die erste Stufe bleibt");
        pruefe(erste.stufen == 0U && erste.unsichtbar && !erste.ersatzbild,
               "Bild der ersten Stufe fehlt (Haut): keine Stufe, unsichtbar");
        pruefe(glm.ersatzbild && !glm.unsichtbar, "Bild fehlt ohne Haut: tr.defaultShader (Kasten)");
        pruefe(alle.stufen == 3U && !alle.ersatzbild && !alle.unsichtbar, "alle Bilder da: der Shader, wie er ist");
        const std::vector<unsigned char> k = figurErsatzbild();
        pruefe(k.size() == 16U * 16U * 4U && k[0] == 255U && k[(5U * 16U + 5U) * 4U] == 32U &&
                   k[(15U * 16U + 7U) * 4U + 3U] == 255U,
               "Ersatzbild: 16 x 16, innen 32, Rand 255");
    }
    // Wortgleich aus MD_Models_PT.pk3 (shaders/battledroid.shader) und
    // MD_Models_Legends.pk3 (shaders/kyle_pack_v2.shader).
    const std::string text =
        "models/players/battledroid/arms\n{\n"
        "    {\n        map models/players/battledroid/arms\n        rgbGen const ( 0.815686 0.717647 0.603922 )\n    }\n"
        "    {\n        map models/players/battledroid/arms\n        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n    }\n"
        "    {\n        map models/players/battledroid/arms\n        blendFunc GL_ZERO GL_SRC_COLOR\n"
        "        rgbGen lightingDiffuse\n    }\n"
        "    {\n        map models/players/battledroid/arms_spec\n        blendFunc GL_SRC_ALPHA GL_ONE\n        detail\n"
        "        alphaGen lightingSpecular\n    }\n}\n"
        "models/players/test/chrom\n{\n"
        "    {\n        map $lightmap\n    }\n"
        "    {\n        map textures/common/env\n        tcGen environment\n        blendFunc GL_ONE GL_ONE\n"
        "        rgbGen wave sin 0.5 0.5 0 1\n    }\n"
        "    {\n        map models/players/test/glow\n        blendFunc GL_ONE GL_ONE\n        glow\n"
        "        rgbGen oneMinusEntity\n        alphaGen const 0.25\n    }\n}\n"
        // rc603 Durchsicht: wortgleich aus MD_Models_PT.pk3 (shaders/mb2_clone.shader) -
        // das Visier aller Klonkrieger (clonetrooper_p1/p2, republictrooper).
        "models/players/MB2_Clones/visor\n{\n"
        "    {\n        map $whiteimage\n        rgbGen const ( 0.0 0.0 0.0 )\n    }\n"
        "    {\n        map textures/common/environ9a\n        blendFunc GL_SRC_ALPHA GL_ONE\n"
        "        alphaGen lightingSpecular\n\t\tdetail\n        tcGen environment\n    }\n}\n";
    ShaderMap m;
    parseShaderScript(text, m);
    // --- rc603 Durchsicht: $whiteimage und $lightmap sind Stufen -------------
    //
    // ParseStage (tr_shader.cpp:1202-1220 MD) bindet fuer "map $whiteimage"
    // tr.whiteImage und fuer "map $lightmap" an einem Modell (lightmapsNone,
    // tr_skin.cpp:337 / tr_ghoul2.cpp:3673 MD: R_FindShader(..., lightmapsNone,
    // ...)) ebenfalls tr.whiteImage - die Stufe wird gezeichnet
    // (RB_IterateStagesGeneric, R_BindAnimatedImage). Das Visier: erst weiss
    // mal rgbGen const (0 0 0) deckend = schwarz, dann der Glanz darauf.
    {
        const auto& vis = figurStufen(m["models/players/mb2_clones/visor"]);
        pruefe(vis.size() == 2U && vis[0].stufe == 0U && vis[0].weiss && vis[0].farbe == FigurFarbe::Fest &&
                   nah(vis[0].rgb[0], 0.0F) && nah(vis[0].rgb[2], 0.0F) && !vis[1].weiss && vis[1].umgebung &&
                   vis[1].alpha == FigurAlpha::Spekular,
               "Klonvisier: $whiteimage-Stufe (schwarz) bleibt Stufe 0 (" + std::to_string(vis.size()) + " Stufen)");
        const auto& lm = figurStufen(m["models/players/test/chrom"]);
        pruefe(lm.size() == 3U && lm[0].stufe == 0U, "$lightmap an einer Figur: tr.whiteImage, Stufe 0 bleibt (" +
                                                       std::to_string(lm.size()) + " Stufen)");
    }
    // REST3 Punkt 6: ein Shader nur aus $whiteimage (wortgleich aus assets1.pk3 shaders/gfx2.shader) - die Huelle eines liegenden
    // Lichtschwerts (CG_Item). Bis REST3 legte parseShaderScript ihn nicht ab (kein Bild), die Huelle fehlte.
    {
        ShaderMap w;
        parseShaderScript("gfx/effects/solidWhite_cull\n{\n\tnomipmaps\n    {\n        map $whiteimage\n        blendFunc GL_ONE GL_ONE\n"
                          "        rgbGen vertex\n    }\n}\n",
                          w);
        const auto it = w.find("gfx/effects/solidwhite_cull");
        pruefe(it != w.end() && figurStufen(it->second).size() == 1U && figurStufen(it->second)[0].weiss,
               "solidWhite_cull: Shader nur aus $whiteimage steht in der Tabelle, eine weisse Stufe");
    }
    // REST6: das rgbGen der Bildstufe (ShaderInfo::rgbArt) - wortgleich aus MD_Assets.pk3 shaders/rift_statuefix.shader; auf der
    // Welt macht lightingDiffuse die Flaeche schwarz (weltDiffus, gpustate.h).
    {
        ShaderMap st;
        parseShaderScript("models/map_objects/rift/statue\n{\n\tq3map_nolightmap\n\tq3map_onlyvertexlighting\n    {\n"
                          "        map models/map_objects/rift/statue\n        blendFunc GL_ONE GL_ZERO\n        rgbGen lightingDiffuse\n    }\n}\n",
                          st);
        const auto it = st.find("models/map_objects/rift/statue");
        pruefe(it != st.end() && it->second.rgbArt == RgbGenArt::LightingDiffuse && it->second.rgbVertex,
               "rift_statuefix: die Bildstufe traegt rgbGen lightingDiffuse (ShaderInfo::rgbArt)");
    }
    const auto& bd = figurStufen(m["models/players/battledroid/arms"]);
    pruefe(bd.size() == 4U, "Kampfdroide: vier Stufen");
    if (bd.size() == 4U) {
        // REST8: constantColor als byte wie ParseStage - 255 * 0.815686 = 207,9999 -> 207, 255 * 0.603922 = 154,0001 -> 154
        pruefe(bd[0].farbe == FigurFarbe::Fest && nah(bd[0].rgb[0], 207.0F / 255.0F) && nah(bd[0].rgb[2], 154.0F / 255.0F),
               "Stufe 0: rgbGen const (unbeleuchtet getoent)");
        pruefe(bd[1].farbe == FigurFarbe::Weiss && bd[1].alpha == FigurAlpha::Eins,
               "Stufe 1: ohne rgbGen bei GL_SRC_ALPHA -> identityLighting (weiss)");
        pruefe(bd[2].farbe == FigurFarbe::Licht, "Stufe 2: rgbGen lightingDiffuse (Gitterlicht)");
        pruefe(bd[3].alpha == FigurAlpha::Spekular && bd[3].farbe == FigurFarbe::Weiss,
               "Stufe 3: alphaGen lightingSpecular");
    }
    const auto& ch = figurStufen(m["models/players/test/chrom"]);
    // rc603 Durchsicht: die $lightmap-Stufe bleibt (tr.whiteImage, ohne rgbGen
    // und ohne blendFunc: CGEN_IDENTITY, ParseStage).
    pruefe(ch.size() == 3U && ch[0].stufe == 0U && ch[0].weiss && ch[0].farbe == FigurFarbe::Weiss &&
               !ch[1].weiss && !ch[2].weiss,
           "$lightmap-Stufe an Figuren: tr.whiteImage, weiss");
    if (ch.size() == 3U) {
        pruefe(ch[1].umgebung && ch[1].farbe == FigurFarbe::Welle, "tcGen environment, rgbGen wave");
        pruefe(ch[2].glow && ch[2].farbe == FigurFarbe::Schwarz && ch[2].alpha == FigurAlpha::Fest &&
                   nah(ch[2].a, 63.0F / 255.0F),   // REST8: 255 * 0.25 = 63,75 -> 63 (constantColor ist ein byte)
               "glow, oneMinusEntity (schwarz), alphaGen const 0.25");
        // FIX5: wie RB_CalcWaveColor - sinTable[256] (knapp unter 1), als Byte: 0,5 -> 127/255, 0,99999 -> 254/255
        pruefe(figurWelle(ch[1].rgbWelle, 0.25F) == 254.0F / 255.0F && figurWelle(ch[1].rgbWelle, 0.0F) == 127.0F / 255.0F,
               "rgbGen wave sin 0.5 0.5 0 1: 127/255 bei 0 s, 254/255 bei 0.25 s (Byte wie im Spiel)");
    }
    // RB_CalcSpecularAlpha (tr_shade_calc.cpp:994 MD / 1046 OJ): Licht von
    // oben, Auge von oben. rc603 Durchsicht: ilength = Q_rsqrt(viewer.viewer)
    // (q_math.c:375 MD / 360 OJ, eine Newton-Stufe) - Q_rsqrt(100) = 0.0998449
    // statt 0.1, l = 0.998449, l^4 * 255 = 253.4 -> 253, nicht 255 (gerechnet
    // in float wie die Engine). Auge seitlich unter 45 Grad: 0.7071 * (1 -
    // 0.00046) hoch 4 = 0.2495 -> b = 63.
    const float n[3] = {0.0F, 0.0F, 1.0F};
    const float l[3] = {0.0F, 0.0F, 1.0F};
    const float oben[3] = {0.0F, 0.0F, 10.0F};
    const float schraeg[3] = {10.0F, 0.0F, 10.0F};
    const float unten[3] = {0.0F, 0.0F, -10.0F};
    pruefe(figurSpekularAlpha(n, l, oben) == 253.0F / 255.0F,
           "Glanz: Auge in der Spiegelrichtung -> 253/255 (Q_rsqrt wie die Engine, nicht 255) = " +
               std::to_string(figurSpekularAlpha(n, l, oben) * 255.0F));
    pruefe(nah(figurSpekularAlpha(n, l, schraeg), 63.0F / 255.0F), "Glanz: 45 Grad -> 63/255 (l^4 * 255, abgeschnitten)");
    pruefe(figurSpekularAlpha(n, l, unten) == 0.0F, "Glanz: dahinter -> 0");
    // RB_CalcEnvironmentTexCoords (tr_shade_calc.cpp:868 MD / 902 OJ): n =
    // (1,0,0), Auge entlang +x. rc603 Durchsicht: VectorNormalizeFast
    // (Q_rsqrt(25) = 0.199690) -> viewer (0.998449,0,0), d = 0.998449,
    // st = (d - 0.5 * 0.998449, 0) = (0.499224, 0) - nicht 0.5.
    float uv[2];
    const float nx[3] = {1.0F, 0.0F, 0.0F};
    const float ax[3] = {5.0F, 0.0F, 0.0F};
    figurUmgebungsUV(nx, ax, uv);
    pruefe(std::fabs(uv[0] - 0.49922439F) < 1e-6F && nah(uv[1], 0.0F),
           "Chrom: st = (n.x d - 0.5 v.x, n.y d - 0.5 v.y) mit VectorNormalizeFast = " + std::to_string(uv[0]));
    // Noch ein schraeger Fall: Auge (3,4,0) -> (0.299535, -0.399380).
    const float a34[3] = {3.0F, 4.0F, 0.0F};
    figurUmgebungsUV(nx, a34, uv);
    pruefe(std::fabs(uv[0] - 0.29953465F) < 1e-6F && std::fabs(uv[1] + 0.39937952F) < 1e-6F,
           "Chrom: Auge (3,4,0) -> (0.299535, -0.399380)");
    std::printf("%s: %d Fehler\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
