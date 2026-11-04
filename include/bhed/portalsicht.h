// portalsicht.h - rc609 P: Spiegel und Portalkameras wie die Engine (OpenJK und Movie Duels 9 gleich).
//
// Gemeinsam fuer die Vorschau im Mission-Tab und den Karten-Editor (Echtzeit-Licht und "Gebaute Karte wie im
// Spiel"): welche Flaeche mit `sort portal` (SS_PORTAL) in diesem Bild eine zweite Sicht bekommt und wie diese
// Sicht liegt. Nachgebaut sind
//   - SP_misc_portal_surface, locateCamera/setCamera (code/game/g_misc.cpp) und CG_Portal (code/cgame/cg_ents.cpp):
//     was als RT_PORTALSURFACE beim Zeichner ankommt (Richtung ueber DirToByte/ByteToDir, Roll als ganze Grad);
//   - R_SortDrawSurfs, R_MirrorViewBySurface, SurfIsOffscreen, IsMirror, R_GetPortalOrientations, R_MirrorPoint,
//     R_MirrorVector, R_PlaneForSurface (code/rd-vanilla/tr_main.cpp): die erste Flaeche, die nicht ausserhalb des
//     Bilds liegt, nicht ganz abgewandt ist, (als Portalkamera) innerhalb von portalRange liegt und ein
//     misc_portal_surface in 64 Einheiten ihrer Ebene hat; eine Sicht je Bild, keine Sicht in der Sicht
//     (viewParms.isPortal);
//   - RB_BeginDrawingView (tr_backend.cpp): die Schnittebene liegt genau auf portalPlane.
#ifndef BHED_PORTALSICHT_H
#define BHED_PORTALSICHT_H

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace bhed {

struct MapEntity;

// q_math.c: DirToByte (die naechste der 162 Richtungen von bytedirs, 0 ohne Treffer) und ByteToDir.
[[nodiscard]] int dirZuByte(const float dir[3]);
void byteZuDir(int b, float dir[3]);
// q_math.c PerpendicularVector und RotatePointAroundVector (Grad).
void senkrechteZu(const float src[3], float dst[3]);
void dreheUmAchse(float dst[3], const float achse[3], const float punkt[3], float grad);
// q_math.c PlaneFromPoints: Normale (c - a) x (b - a), Abstand a.n; false bei entartetem Dreieck.
bool ebeneAusPunkten(const float a[3], const float b[3], const float c[3], float ebene[4]);

// Ein misc_portal_surface, wie CG_Portal es als RT_PORTALSURFACE an den Zeichner gibt.
struct PortalSchirm {
    float ort[3]{};              // e.origin (s.origin)
    float kamOrt[3]{};           // e.oldorigin (s.origin2): beim Spiegel gleich dem Ort
    float achse[3][3]{};         // e.axis: ByteToDir(eventParm), dazu die negierte Senkrechte und das Kreuzprodukt
    int frame = 0;               // Drehtempo (spawnflags 1: 25, 2: 75)
    int skinNum = 0;             // Roll in ganzen Grad: (int)(clientNum / 256.0 * 360)
    std::size_t entity = 0;      // Index in der Entity-Liste
    [[nodiscard]] bool spiegel() const { return kamOrt[0] == ort[0] && kamOrt[1] == ort[1] && kamOrt[2] == ort[2]; }
};
// Alle misc_portal_surface der Karte in der Reihenfolge der Entities (= Reihenfolge der refEntities). `gespawnt`:
// ob das Spiel die Entity anlegt (Schwierigkeit, notsingle ...). Ohne gefundene Kamera gibt es keinen Schirm
// (locateCamera: G_FreeEntity).
[[nodiscard]] std::vector<PortalSchirm> portalSchirme(const std::vector<MapEntity>& ents, const std::function<bool(const MapEntity&)>& gespawnt);

// Eine Flaeche mit einem Portal-Shader, so wie R_MirrorViewBySurface sie sieht (tess nach rb_surfaceTable).
struct PortalFlaeche {
    std::vector<std::array<float, 3>> ecken;     // tess.xyz
    std::vector<std::array<float, 3>> normalen;  // tess.normal
    std::vector<int> indizes;                    // tess.indexes (Dreiecke)
    float ebene[4]{0.0F, 0.0F, 1.0F, 0.0F};      // R_PlaneForSurface: Normale und Abstand
    float portalWeite = 0.0F;                    // shader.portalRange (alphaGen portal; ohne: 0)
    int sortierung = 0;                          // Rang des Shaders im Sortierschluessel (sortedIndex)
};

// Die Hauptsicht (viewParms): Ort, Achsen (vorn, rechts, oben), halbe Bildwinkel als Tangens, r_znear, Fernebene.
struct PortalAuge {
    float ort[3]{};
    float vorn[3]{1.0F, 0.0F, 0.0F};
    float rechts[3]{0.0F, -1.0F, 0.0F};
    float oben[3]{0.0F, 0.0F, 1.0F};
    float tanX = 1.0F;
    float tanY = 1.0F;
    float nah = 4.0F;
    float fern = 65536.0F;
};

// Die zweite Sicht. Gezeichnet wird von der echten Kamera aus mit der Welt durch `welt` (4x4 zeilenweise, die
// Umkehrung der Abbildung von R_MirrorPoint) - gleichwertig zur Kamera am abgebildeten Ort. `achsen` bildet
// Richtungen ab (Himmel), `schnitt` ist portalPlane als (n, -d), `umkehren` die Wicklung (nur Spiegel).
struct PortalSicht {
    float welt[16]{};
    float achsen[9]{};
    float schnitt[4]{0.0F, 0.0F, 0.0F, 1.0F};
    bool umkehren = false;
    bool portal = false;          // Portalkamera (sonst Spiegel)
    float auge[3]{};              // newParms.ori.origin (R_MirrorPoint)
    float vorn[3]{};              // newParms.ori.axis[0] (R_MirrorVector)
    float pvsOrt[3]{};            // newParms.pvsOrigin = e.oldorigin
    float ebene[4]{};             // die Ebene der Flaeche
    std::size_t flaeche = 0;      // Index in der Flaechenliste
    std::size_t schirm = 0;       // Index in der Schirmliste
};

// R_SortDrawSurfs: die erste Portalflaeche (nach `sortierung`, bei Gleichstand die naechste - die Reihenfolge der
// Engine haengt dort am Weg durch den BSP-Baum), fuer die R_MirrorViewBySurface eine Sicht zeichnet. `zeitMs`:
// tr.refdef.time (Drehung der Portalkamera). `imBild`: ob der Schirm in diesem Bild beim Zeichner ankommt
// (Snapshot: PVS und Gebiete); `flaecheImBild`: ob die Flaeche in der Liste der drawSurfs steht (R_MarkLeaves); leer = alle.
[[nodiscard]] bool portalSichtWaehlen(const PortalAuge& auge, const std::vector<PortalFlaeche>& flaechen, const std::vector<PortalSchirm>& schirme,
                                      double zeitMs, PortalSicht& aus, const std::function<bool(std::size_t)>& imBild = {},
                                      const std::function<bool(std::size_t)>& flaecheImBild = {});

}  // namespace bhed

#endif
