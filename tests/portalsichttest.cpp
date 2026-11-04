// portalsichttest.cpp - rc609 P: Spiegel und Portalkameras wie die Engine (bhed/portalsicht.h)
//
//   - DirToByte/ByteToDir (q_math.c): die Richtung der Portalkamera ist eine der 162 Richtungen
//   - misc_portal_surface/misc_portal_camera wie g_misc.cpp und CG_Portal (Roll in ganzen Grad, Drehtempo, G_Find)
//   - R_MirrorViewBySurface: SurfIsOffscreen (Bild, abgewandt, portalRange), 64 Einheiten, Sortierung, portalPlane
#include "bhed/bsp.h"
#include "bhed/portalsicht.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
bool nah(float a, float b, float g = 1e-4F) { return std::fabs(a - b) <= g; }

MapEntity ent(std::vector<std::pair<std::string, std::string>> keys) {
    MapEntity e;
    for (const auto& [k, v] : keys) {
        if (k == "classname") { e.classname = v; }
        if (k == "targetname") { e.targetname = v; }
        if (k == "origin") { e.origin = v; }
    }
    e.keys = std::move(keys);
    return e;
}

// Ein Viereck in der Ebene z = h (Normale nach oben) von -g..g
PortalFlaeche boden(float h, float g, float weite, int sortierung) {
    PortalFlaeche f;
    f.ecken = {{-g, -g, h}, {g, -g, h}, {g, g, h}, {-g, g, h}};
    f.normalen = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    f.indizes = {0, 1, 2, 0, 2, 3};
    f.ebene[0] = 0;
    f.ebene[1] = 0;
    f.ebene[2] = 1;
    f.ebene[3] = h;
    f.portalWeite = weite;
    f.sortierung = sortierung;
    return f;
}

// Auge bei o, Blick nach unten (Nicken 89), 90 Grad
PortalAuge augeUeber(float x, float y, float z, bool nachUnten = true) {
    PortalAuge a;
    a.ort[0] = x;
    a.ort[1] = y;
    a.ort[2] = z;
    if (nachUnten) {
        a.vorn[0] = 0;
        a.vorn[1] = 0;
        a.vorn[2] = -1;
        a.oben[0] = 1;
        a.oben[1] = 0;
        a.oben[2] = 0;
        a.rechts[0] = 0;
        a.rechts[1] = -1;
        a.rechts[2] = 0;
    } else {
        a.vorn[0] = 0;
        a.vorn[1] = 0;
        a.vorn[2] = 1;
        a.oben[0] = 1;
        a.oben[1] = 0;
        a.oben[2] = 0;
        a.rechts[0] = 0;
        a.rechts[1] = 1;
        a.rechts[2] = 0;
    }
    a.tanX = 1.0F;
    a.tanY = 1.0F;
    return a;
}
}  // namespace

int main() {
    std::printf("portalsicht: Spiegel und Portalkameras wie die Engine\n");

    // --- DirToByte / ByteToDir ---------------------------------------------------------------
    {
        float d[3];
        byteZuDir(5, d);   // bytedirs[5] = (0, 0, 1)
        pruefe(nah(d[0], 0) && nah(d[1], 0) && nah(d[2], 1), "ByteToDir(5) ist (0 0 1) wie bytedirs[5]");
        pruefe(dirZuByte(d) == 5, "DirToByte((0 0 1)) ist 5");
        const float schief[3] = {0.96F, 0.28F, 0.0F};   // 16,3 Grad - die naechste Richtung der Tabelle
        float q[3];
        byteZuDir(dirZuByte(schief), q);
        pruefe(q[0] * schief[0] + q[1] * schief[1] + q[2] * schief[2] > 0.95F && !(nah(q[0], schief[0], 1e-3F) && nah(q[1], schief[1], 1e-3F)),
               "eine schiefe Richtung wird auf eine der 162 Richtungen gerundet (nicht genau)");
        const float null3[3] = {0, 0, 0};
        pruefe(dirZuByte(null3) == 0, "DirToByte((0 0 0)) ist 0 (bestd 0)");
        byteZuDir(162, d);
        pruefe(nah(d[0], 0) && nah(d[1], 0) && nah(d[2], 0), "ByteToDir ausserhalb der Tabelle ist (0 0 0)");
    }

    // --- misc_portal_surface / misc_portal_camera ---------------------------------------------
    {
        std::vector<MapEntity> e;
        e.push_back(ent({{"classname", "worldspawn"}}));
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 8"}}));
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "100 0 8"}, {"target", "kam"}}));
        e.push_back(ent({{"classname", "misc_portal_camera"}, {"targetname", "KAM"}, {"origin", "500 0 96"}, {"roll", "10"}, {"spawnflags", "4"},
                         {"target", "blick"}}));
        e.push_back(ent({{"classname", "info_notnull"}, {"targetname", "blick"}, {"origin", "600 30 96"}}));
        e.push_back(ent({{"classname", "misc_portal_camera"}, {"targetname", "kam"}, {"origin", "900 0 0"}}));   // zweite: nicht die erste
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 0"}, {"target", "fehlt"}}));
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 0"}, {"target", "dreh"}}));
        e.push_back(ent({{"classname", "misc_portal_camera"}, {"targetname", "dreh"}, {"origin", "0 0 0"}, {"spawnflags", "3"}, {"angles", "0 90 0"},
                         {"angle", "-1"}}));
        const std::vector<PortalSchirm> s = portalSchirme(e, {});
        pruefe(s.size() == 3U, "drei Schirme: Spiegel, Kamera, Drehkamera; ohne gefundene Kamera keiner (G_FreeEntity)");
        if (s.size() == 3U) {
            pruefe(s[0].spiegel() && s[0].entity == 1U, "ohne target ein Spiegel (origin2 = origin)");
            pruefe(!s[1].spiegel() && nah(s[1].kamOrt[0], 500) && nah(s[1].kamOrt[2], 96), "G_Find: die erste Entity mit dem Namen (ohne Gross/Klein)");
            // roll 10: clientNum = (int)(10/360*256) = 7, skinNum = (int)(7/256*360) = 9
            pruefe(s[1].skinNum == 9 && s[1].frame == 0, "Roll 10 wird ganze 9 Grad (clientNum 7), spawnflags 4 dreht nicht");
            float soll[3] = {100.0F, 30.0F, 0.0F};
            const float l = std::sqrt(soll[0] * soll[0] + soll[1] * soll[1]);
            soll[0] /= l;
            soll[1] /= l;
            float q[3];
            byteZuDir(dirZuByte(soll), q);
            pruefe(nah(s[1].achse[0][0], q[0]) && nah(s[1].achse[0][1], q[1]) && nah(s[1].achse[0][2], q[2]),
                   "die Blickrichtung zum target kommt ueber DirToByte/ByteToDir");
            pruefe(s[2].frame == 25, "spawnflags 1 (und 2): Drehtempo 25");
            // "angle" -1 steht nach "angles": s.angles = (0 -1 0) -> G_SetMovedir: nach oben
            pruefe(nah(s[2].achse[0][2], 1.0F), "angles und angle: das letzte Paar gewinnt, (0 -1 0) heisst nach oben (G_SetMovedir)");
        }
    }

    // --- R_MirrorViewBySurface ----------------------------------------------------------------
    {
        std::vector<MapEntity> e;
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 10"}}));
        const std::vector<PortalSchirm> s = portalSchirme(e, {});
        std::vector<PortalFlaeche> f = {boden(0.0F, 256.0F, 0.0F, 0)};
        PortalSicht v;
        bool da = portalSichtWaehlen(augeUeber(10, 20, 200), f, s, 0.0, v);
        pruefe(da && !v.portal && v.umkehren, "Spiegelboden von oben: Spiegelsicht mit umgekehrter Wicklung");
        if (da) {
            pruefe(nah(v.schnitt[0], 0) && nah(v.schnitt[1], 0) && nah(v.schnitt[2], 1) && nah(v.schnitt[3], 0),
                   "portalPlane genau auf der Ebene (glClipPlane, RB_BeginDrawingView)");
            pruefe(nah(v.auge[0], 10) && nah(v.auge[1], 20) && nah(v.auge[2], -200), "R_MirrorPoint: das Auge unter dem Boden");
            pruefe(nah(v.vorn[2], 1), "R_MirrorVector: der Blick nach oben");
            pruefe(nah(v.pvsOrt[0], 0) && nah(v.pvsOrt[2], 10), "pvsOrigin ist der Ort des misc_portal_surface (oldorigin)");
            // welt(x) = R x + 2 d n: (1 2 3) -> (1 2 -3)
            const float x[3] = {1, 2, 3};
            float y[3];
            for (int z = 0; z < 3; ++z) { y[z] = v.welt[z * 4] * x[0] + v.welt[z * 4 + 1] * x[1] + v.welt[z * 4 + 2] * x[2] + v.welt[z * 4 + 3]; }
            pruefe(nah(y[0], 1) && nah(y[1], 2) && nah(y[2], -3), "die Welt wird an der Ebene gespiegelt");
        }
        pruefe(!portalSichtWaehlen(augeUeber(10, 20, -200, false), f, s, 0.0, v), "von unten (abgewandt) keine Sicht");
        pruefe(!portalSichtWaehlen(augeUeber(10, 20, 200), f, s, 0.0, v, {}, [](std::size_t) { return false; }),
               "Flaeche nicht in den drawSurfs (R_MarkLeaves): keine Sicht");
        pruefe(!portalSichtWaehlen(augeUeber(10, 20, 200), f, s, 0.0, v, [](std::size_t) { return false; }), "Schirm nicht im Snapshot: keine Sicht");
        pruefe(!portalSichtWaehlen(augeUeber(10, 20, 200, false), f, s, 0.0, v), "Blick weg vom Spiegel (ausserhalb des Bilds) keine Sicht");
        std::vector<MapEntity> weit = {ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 65"}})};
        pruefe(!portalSichtWaehlen(augeUeber(10, 20, 200), f, portalSchirme(weit, {}), 0.0, v), "Schirm 65 Einheiten ueber der Ebene: keine Sicht");
        // zwei Spiegel: der Shader mit kleinerem Rang gewinnt, auch wenn er weiter weg liegt
        std::vector<PortalFlaeche> zwei = {boden(0.0F, 64.0F, 0.0F, 3), boden(-40.0F, 64.0F, 0.0F, 1)};
        pruefe(portalSichtWaehlen(augeUeber(10, 20, 200), zwei, s, 0.0, v) && v.flaeche == 1U, "R_SortDrawSurfs: die Flaeche mit kleinerem Rang zuerst");
        zwei[1].sortierung = 3;
        pruefe(portalSichtWaehlen(augeUeber(10, 20, 200), zwei, s, 0.0, v) && v.flaeche == 0U, "bei gleichem Rang die naechste");
    }

    // --- Portalkamera: portalRange und Abbildung ---------------------------------------------
    {
        std::vector<MapEntity> e;
        e.push_back(ent({{"classname", "misc_portal_surface"}, {"origin", "0 0 4"}, {"target", "k"}}));
        e.push_back(ent({{"classname", "misc_portal_camera"}, {"targetname", "k"}, {"origin", "1000 0 50"}, {"angles", "0 0 0"}}));
        const std::vector<PortalSchirm> s = portalSchirme(e, {});
        std::vector<PortalFlaeche> ohne = {boden(0.0F, 64.0F, 0.0F, 0)};
        PortalSicht v;
        pruefe(!portalSichtWaehlen(augeUeber(0, 0, 100), ohne, s, 0.0, v), "Portalkamera ohne alphaGen portal (portalRange 0): keine Sicht");
        std::vector<PortalFlaeche> mit = {boden(0.0F, 64.0F, 256.0F, 0)};
        pruefe(!portalSichtWaehlen(augeUeber(0, 0, 300), mit, s, 0.0, v), "weiter als portalRange 256: keine Sicht");
        const bool da = portalSichtWaehlen(augeUeber(0, 0, 100), mit, s, 0.0, v);
        pruefe(da && v.portal && !v.umkehren, "innerhalb von portalRange: Portalsicht ohne Umkehrung");
        if (da) {
            // welt(Kameraort) = Ort des Schirms auf der Ebene
            float y[3];
            const float c[3] = {1000, 0, 50};
            for (int z = 0; z < 3; ++z) { y[z] = v.welt[z * 4] * c[0] + v.welt[z * 4 + 1] * c[1] + v.welt[z * 4 + 2] * c[2] + v.welt[z * 4 + 3]; }
            pruefe(nah(y[0], 0) && nah(y[1], 0) && nah(y[2], 0, 1e-3F), "die Kamera liegt abgebildet auf dem Schirm (surface->origin)");
            pruefe(nah(v.pvsOrt[0], 1000) && nah(v.pvsOrt[2], 50), "pvsOrigin ist der Ort der Kamera");
            // portalPlane: vor der Kamera (Blick +x): x >= 1000
            pruefe(nah(v.schnitt[0], 1) && nah(v.schnitt[3], -1000), "portalPlane vor der Kamera");
            // Das Auge 100 ueber dem Schirm sieht durch die Kamera: 100 vor ihr auf ihrer Blickachse
            pruefe(nah(v.auge[0], 900, 1e-2F) && nah(v.auge[1], 0, 1e-2F) && nah(v.auge[2], 50, 1e-2F), "R_MirrorPoint: das Auge hinter der Kamera");
        }
    }

    std::printf(g_fehler == 0 ? "portalsicht: alles gut\n" : "portalsicht: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
