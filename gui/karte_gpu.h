// karte_gpu.h - Karten-Editor: Zeichnen ueber Direct3D 11.
//
// NRC zeichnet jede Ansicht mit OpenGL in ein eigenes FBO mit MSAA
// (xywindow.cpp:477-496, camwindow.cpp:1606-1623) und blendet es ein. Hier
// dasselbe mit Direct3D 11: je Ansicht ein Renderziel (mit Mehrfachabtastung,
// aufgeloest in eine Textur, die ImGui zeigt). Linien in Weltkoordinaten mit
// einer Matrix (Projektion x Modelview), Breite 1 oder breiter (glLineWidth:
// ueber einen Geometrie-Shader zu Rechtecken in Bildpunkten).
//
// Ohne Direct3D (OpenGL-Rueckfall der Oberflaeche) zeichnet die Oberflaeche
// dieselben Linien mit ImGui (verfuegbar() == false).
#ifndef BHED_GUI_KARTE_GPU_H
#define BHED_GUI_KARTE_GPU_H

#include "bhed/karte/xyansicht.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bhed::gui::kartegpu {

[[nodiscard]] bool verfuegbar();

// Ein Linienpuffer auf der Grafikkarte (z. B. alle Pinselkanten einer
// Ansicht) - einmal fuellen, oft zeichnen.
class Puffer {
public:
    Puffer() = default;
    ~Puffer();
    Puffer(const Puffer&) = delete;
    Puffer& operator=(const Puffer&) = delete;
    void setzen(const std::vector<karte::Linie>& linien);
    [[nodiscard]] std::size_t anzahl() const { return m_anzahl; }

private:
    friend void zeichnen(const Puffer& p, const float* matrix, float breite);
    friend void zeichnen(const Puffer& p, const float* matrix, float breite, unsigned muster, int faktor);
    void* m_vb = nullptr;      // ID3D11Buffer*
    std::size_t m_platz = 0;   // Linien, fuer die der Puffer Platz hat
    std::size_t m_anzahl = 0;
    unsigned m_stand = ~0U;    // render::texturStand() beim Anlegen
};

// In das Renderziel "kennung" zeichnen (anlegen bzw. neu anlegen bei anderer
// Groesse/MSAA). hinten: Loeschfarbe RGBA 0..1.
bool beginnen(int kennung, int breite, int hoehe, int msaa, const float* hinten, bool tiefe = false);
// matrix: 4x4 spaltenweise (wie OpenGL: Clip = M * Welt).
void zeichnen(const Puffer& p, const float* matrix, float breite);
// Dasselbe mit glLineStipple( faktor, muster ) (RENDER_LINESTIPPLE, z. B.
// $XY_OVERLAY: Faktor 3, Muster 0xaaaa).
void zeichnen(const Puffer& p, const float* matrix, float breite, unsigned muster, int faktor);
// Einmal-Linien (Raster, Overlay) ueber einen gemeinsamen Puffer.
void zeichnen(const std::vector<karte::Linie>& linien, const float* matrix, float breite);
// Punkte wie GL_POINTS mit glPointSize( groesse ): ein Quadrat je Linie am
// Punkt "a" (b wird nicht gebraucht), ohne Tiefentest.
void punkte(const std::vector<karte::Linie>& punkte, const float* matrix, float groesse);
// XORRectangle::render (libs/gtkutil/xorrectangle.h): das Auswahlrechteck in
// Geraetekoordinaten (-1..1) - drei Fuellungen (additiv, Filter, Alpha) und
// der Rand. art: 0 auswaehlen, 1 abwaehlen, 2 umschalten (rect_t::EModifier).
void auswahlRechteck(float x0, float y0, float x1, float y1, int art);
// Aufloesen und die vorigen Renderziele wiederherstellen.
void beenden();
// Die Textur eines Renderziels fuer ImGui::Image (nullptr, wenn keine).
[[nodiscard]] void* textur(int kennung);
void freigeben();

// --- 3D (Kamera, camwindow.cpp Cam_Draw) ------------------------------------
//
// NRC zeichnet die Kamera mit der festen OpenGL-Pipeline: Zustaende aus
// renderstate.cpp (Tiefentest, Mischen, Wegschneiden mit glFrontFace(GL_CW),
// Alpha-Test, Licht aus Blickrichtung, glPolygonOffset fuer Linien im
// Polygonmodus GL_LINE, Strichelmuster). Hier dasselbe mit Shadern; die
// Tiefe ist D24 wie Qts FBO (GL_DEPTH_COMPONENT24).

// Ein Eckpunkt eines Dreiecks.
struct Punkt3 {
    float pos[3];
    float normale[3];
    float st[2];
    std::uint32_t farbe;
};
// Ein Linienende: extra.w = 1 - Kante eines Polygons im Linienmodus, extra.xyz
// ein weiterer Punkt der Flaeche (Wegschneiden, Polygon-Versatz); extra.w = 0
// - GL_LINES, extra.xyz die beim Zeichnen geltende Normale.
struct LinienPunkt {
    float pos[3];
    std::uint32_t farbe;
    float extra[4];
};

// Ein Puffer auf der Grafikkarte (Dreiecke oder Linien), einmal fuellen.
class Puffer3 {
public:
    Puffer3() = default;
    ~Puffer3();
    Puffer3(const Puffer3&) = delete;
    Puffer3& operator=(const Puffer3&) = delete;
    void setzen(const void* daten, std::size_t bytes);
    [[nodiscard]] bool gueltig() const;

private:
    friend struct Zugriff3;
    void* m_vb = nullptr;
    std::size_t m_platz = 0;
    unsigned m_stand = ~0U;
};

enum class TiefenTest { Aus, Kleiner, KleinerGleich, Groesser };   // GL_LESS, GL_LEQUAL, GL_GREATER
enum class Mischen { Aus, Alpha, Additiv, Spiel };   // SRC_ALPHA/ONE_MINUS_SRC_ALPHA, ONE/ONE; Spiel: mischQuelle/mischZiel (Stufe 9c)
struct Zustand3 {
    const float* matrix = nullptr;      // projection * modelview (spaltenweise)
    float farbe[4]{1, 1, 1, 1};         // glColor (mal Eckenfarbe)
    bool licht = false;                 // RENDER_LIGHTING
    float vpn[3]{0, 0, 1};              // Lichtrichtung (Blick zurueck)
    void* bild = nullptr;               // RENDER_TEXTURE (GL_MODULATE)
    void* himmel = nullptr;             // Skybox-Wuerfel (skybox_fp.glsl)
    float auge[3]{0, 0, 0};             // u_view_origin, Wegschneiden der Linien
    int alphaFunktion = 0;              // 0 immer, 1 gleich, 2 kleiner, 3 groesser, 4 kleiner gleich, 5 groesser gleich
    float alphaRef = 0;
    TiefenTest tiefe = TiefenTest::Kleiner;
    bool tiefeSchreiben = true;
    bool wegschneiden = true;           // RENDER_CULLFACE
    Mischen mischen = Mischen::Aus;
    bool versatz = false;               // RENDER_OFFSETLINE
    float versatzFaktor = -1;           // glPolygonOffset(-1, 1)
    float versatzEinheiten = 1;
    unsigned muster = 0;                // RENDER_LINESTIPPLE: Muster (0 = aus)
    int musterFaktor = 1;
    float breite = 1;                   // glLineWidth (nur Linien)
    // RENDER_FOG mit GL_EXP (prtview, Stufe 3 Punkt 5): rgb die Farbe, a die
    // Dichte (0 = aus); f = exp(-Dichte * Abstand), Abstand = -z im Auge (= w).
    float nebel[4]{0, 0, 0, 0};
    // Stufe 7: Echtzeit-Licht (lichtSetzen) statt des Lichts aus Blickrichtung
    bool echtLicht = false;
    // Stufe 9c: blendFunc der ersten Stufe wie im Spiel (ShaderSkript: 1 ONE, 2 ZERO,
    // 3 SRC_ALPHA, 4 ONE_MINUS_SRC_ALPHA, 5 DST_COLOR, 6 ONE_MINUS_DST_COLOR,
    // 7 SRC_COLOR, 8 ONE_MINUS_SRC_COLOR, 9 DST_ALPHA, 10 ONE_MINUS_DST_ALPHA)
    int mischQuelle = 0;
    int mischZiel = 0;
};
// Punkte wie punkte(), aber mit Tiefentest gegen die Tiefe der Kamera (fuer
// $DEEPPOINT: erst die verdeckten mit GL_GREATER, dann die sichtbaren).
void punkte(const std::vector<karte::Linie>& punkte, const float* matrix, float groesse, TiefenTest test);
// In das Renderziel mit Tiefe zeichnen (beginnen(..., true) vorher).
void dreiecke(const Puffer3& p, std::size_t erstes, std::size_t anzahl, const Zustand3& z);
void linien(const Puffer3& p, std::size_t erstes, std::size_t anzahl, const Zustand3& z);
void dreiecke(const std::vector<Punkt3>& p, const Zustand3& z);
void linien(const std::vector<LinienPunkt>& p, const Zustand3& z);

// Bilder: 2D mit Mipmaps (GL_LINEAR_MIPMAP_LINEAR, anisotrop, GL_REPEAT) und
// Wuerfel (6 gleich grosse Seiten +X -X +Y -Y +Z -Z, GL_LINEAR, GL_CLAMP).
// Das Ergebnis gilt, solange geraetStand() gleich bleibt.
// mipStufe: GL_TEXTURE_BASE_LEVEL ("Texture Quality"), als MostDetailedMip
[[nodiscard]] void* bildAnlegen(const unsigned char* rgba, int breite, int hoehe, int mipStufe = 0);
// SetTexParameters / SetTexAnisotropy (textures.cpp:89-128): der Abtaster der Texturen
void abtasterSetzen(int modus, bool anisotropie);
// Fuer Bilder, die ImGui zeichnet (Texturbrowser): ab hier ohne Mischen
// (glDisable( GL_BLEND )), bis ImDrawCallback_ResetRenderState kommt.
// dl ist eine ImDrawList*.
void ohneMischen(void* dl);
[[nodiscard]] void* wuerfelAnlegen(const unsigned char* const seiten[6], int groesse);
// Freigeben erst nach dem laufenden Bild: ImGuis Zeichenlisten dieses Bildes
// koennen das Bild noch nennen (RefreshShaders aus dem Menue oder einem
// Schritt nach dem Zeichnen - Lauf k11e: Absturz 0x87D). aufraeumen() gibt
// die Bilder frueherer Bilder frei (zu Beginn jedes Editor-Bildes).
void bildFreigeben(void* bild);
void aufraeumen();
[[nodiscard]] unsigned geraetStand();
// --- Stufe 7/9c: Echtzeit-Licht (eigene Funktion, Vorschau) ----------------
//
// Die Grafikkarte rechnet das direkte Licht je Bildpunkt mit den Formeln von
// q3map2 (bhed/karte/echtlicht.h). Stufe 9c: bis kLichtMax Lichter je Bild in
// einem Puffer, je Kachel (kKachel Bildpunkte) eine Bitmaske der Lichter, deren
// Huelle die Kachel trifft; Flaechenlichter mit ihren Ecken (Formfaktor).
// Schatten: Schattenwuerfel (zwei Groessen), die Sonne in zwei Kaskaden (ganze
// Karte, um die Kamera) und die Sonnen von q3map_skyLight je eine Karte; der
// Strahl zur Sonne endet am Himmel wie in q3map2 (Himmelstiefe S, darunter die
// Verdecker O).
inline constexpr int kLichtMax = 512;          // Lichter je Bild
inline constexpr int kSchattenGross = 32;      // Schattenwuerfel 512 x 512 (die wichtigsten)
inline constexpr int kSchattenKlein = 96;      // Schattenwuerfel 256 x 256
inline constexpr int kSchattenMax = kSchattenGross + kSchattenKlein;
inline constexpr int kHimmelSonnenMax = 16;    // Sonnen mit eigener Karte (1024 x 1024)
inline constexpr int kKaskadeGroesse = 2048;   // Bildpunkte je Seite einer Sonnenkaskade
inline constexpr int kKachel = 32;             // Bildpunkte je Kachel der Lichtliste
struct LichtGpu {
    float pos[4];        // xyz Ursprung, w photons
    float farbe[4];      // rgb, a Art (0 Punkt, 1 Spot, 2 Sonne, 3 Flaeche)
    float richtung[4];   // xyz Spot-/Flaechennormale bzw. zur Sonne, w radiusByDist
    float werte[4];      // x linear, y Winkel, z fade, w Huelle
    float mehr[4];       // x Schatten: Wuerfel >= 0, Sonne: Himmelskarte >= 0 oder -3 Kaskaden; -1 ohne; y Weite; z angleScale; w extraDist
    float flaeche[4];    // x add, y zweiseitig, z erste Ecke, w Eckenzahl (0: Naeherung ohne Ecken)
    float zusatz[4];     // x 1 negativ, y Flaeche (Flaechenlicht), z Radius des Stuecks, w frei
};
// Ein Teil der Schattengeometrie (Punkt3-Ecken): art 0 deckende Dreiecke
// (Strahlen aller Lichter), 1 nur Volumen (strukturelle deckende Pinsel, die
// q3map2 als feste Blaetter sieht: Punkt- und Flaechenlichter), 2 Himmel (die
// Himmelstiefe der Sonne), 3 alphashadow (mit Alpha-Test des Bilds).
struct SchattenTeil {
    const Puffer3* puffer = nullptr;
    std::size_t erstes = 0;
    std::size_t anzahl = 0;
    int art = 0;
    void* bild = nullptr;        // art 3
    float mn[3]{0, 0, 0};        // Grenzen (Ausschnitt je Licht)
    float mx[3]{0, 0, 0};
};
[[nodiscard]] bool echtLichtVerfuegbar();
// Lichter, Ecken der Flaechenlichter (xyz je Ecke), die Kachelmasken (worte je
// Kachel, kachelnX * kachelnY Kacheln), Umgebung und Minimum (rgb).
void lichtSetzen(const std::vector<LichtGpu>& lichter, const std::vector<float>& ecken, const std::vector<std::uint32_t>& kacheln, int kachelnX,
                 int kachelnY, int worte, const float* umgebung, const float* minimum);
// Den Schattenwuerfel slot (< kSchattenGross: 512, sonst 256) fuer ein Licht an
// pos (Weite = Huelle) aus den Teilen der Arten 0, 1, 3 rendern. Vor beginnen().
bool schattenWuerfel(int slot, const float* pos, float weite, const std::vector<SchattenTeil>& teile);
// Eine Sonnenkaskade (0: ganze Karte, 1: um die Kamera): richtung zur Sonne,
// Kasten um mitte mit halb (quer) und tiefe (laengs).
bool sonnenKaskade(int stufe, const float* richtung, const float* mitte, float halb, float tiefe, const std::vector<SchattenTeil>& teile);
// Die Karte einer Sonne aus q3map_skyLight (Schicht < kHimmelSonnenMax), ganze Karte.
bool himmelsSonne(int schicht, const float* richtung, const float* mitte, float halb, float tiefe, const std::vector<SchattenTeil>& teile);
void sonnenSchattenAus();
// Gezeichnete Dreiecke aller Schattenkarten bisher (das Budget je Bild).
[[nodiscard]] std::size_t schattenDreiecke();

// Schnell-Backen (Stufe 7 Punkt 5b): die gebauten Flaechen einer .bsp mit ihren
// echten Lightmaps. Ecke wie bhed::BspVertex (xyz, st, Lightmap-st, Normale, Farbe).
struct GebackenPunkt {
    float pos[3];
    float st[2];
    float lm[2];
    float normale[3];
    std::uint32_t farbe;
};
class GebackenNetz {
public:
    GebackenNetz() = default;
    ~GebackenNetz();
    GebackenNetz(const GebackenNetz&) = delete;
    GebackenNetz& operator=(const GebackenNetz&) = delete;
    bool setzen(const std::vector<GebackenPunkt>& punkte, const std::vector<std::uint32_t>& indizes);
    void leeren();
    [[nodiscard]] bool gueltig() const;

private:
    friend struct ZugriffGebacken;
    void* m_vb = nullptr;
    void* m_ib = nullptr;
    unsigned m_stand = ~0U;
};
// Eine 128x128-Lightmap (RGB) als Bild ohne Mipmaps; mit bildFreigeben freigeben.
[[nodiscard]] void* lightmapAnlegen(const std::uint8_t* rgb, int breite, int hoehe);
// Einen Bereich zeichnen: Textur (z.bild) mal Lightmap (nullptr: die Eckfarbe,
// LIGHTMAP_BY_VERTEX) mal 2 (r_mapOverBrightBits), Zustand wie dreiecke().
void gebackenZeichnen(const GebackenNetz& netz, std::uint32_t erster, std::uint32_t anzahl, void* lightmap, const Zustand3& z);

// Fuer den Selbsttest: ein Bildpunkt des aufgeloesten Renderziels.
bool pixel(int kennung, int x, int y, unsigned char rgba[4]);
// Stufe 9c: das ganze Bild (RGBA, Zeilen von oben) fuer die Lichtmessung.
bool bildLesen(int kennung, std::vector<unsigned char>& rgba, int& breite, int& hoehe);
// Fuer den Selbsttest: die Abtastungen (MSAA) des Renderziels, 0 ohne Ziel.
[[nodiscard]] int abtastungen(int kennung);

}  // namespace bhed::gui::kartegpu

#endif
