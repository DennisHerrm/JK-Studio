// app_karte_echtlicht.cpp - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht in der
// Kamera und Schnell-Backen im Hintergrund (eigene Funktion, nicht in
// NetRadiant-Custom; siehe karte_echtlicht.h).
//
// a) Vorschau: die Lichter der Karte wie q3map2 -light (bhed/karte/echtlicht.h),
//    je Bild hoechstens kLichtMax, die naechsten zuerst; Schattenwuerfel fuer die
//    kSchattenMax naechsten, einige je Bild neu (ein verschobenes Licht sofort);
//    die Sonne mit einer Schattenkarte ueber die ganze Karte. Beschriftet als
//    Vorschau - kein Bounce, kein Filter.
// b) Schnell-Backen: nach einer Pause ohne Aenderung baut bhed_q3map2 die Karte
//    (-bsp -meta, -vis -fast, -light -fast) in einen eigenen temporaeren Ordner,
//    mit Prioritaet "Leerlauf"; eine neue Aenderung bricht den Bau ab. Danach
//    zeigt die Kamera die gebauten Flaechen mit ihren echten Lightmaps, bis sich
//    die Karte wieder aendert. Die Karte des Anwenders und eine .bsp neben ihr
//    bleiben unberuehrt.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karte_echtlicht.h"

#include "karte_bau_plattform.h"
#include "karteneditor.h"
#include "platform.h"

#include "bhed/bspgeo.h"
#include "bhed/i18n.h"
#include "bhed/karte/echtlicht.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/regelwerk.h"

#include <process.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <set>
#include <thread>
#include <tuple>

namespace bhed::gui::karteneditor {

using i18n::Str;

namespace {

namespace fs = std::filesystem;
using karte::EchtLicht;
using karte::Vektor3f;

Editor& ed() { return editor(); }

// --- a) Vorschau --------------------------------------------------------------------

// Shader-Skripte (q3map_surfacelight, q3map_sun, q3map_lightRGB), je Datei einmal zerlegt
std::map<std::string, karte::ShaderDatei> g_dateien;
std::map<std::string, karte::ShaderSkript> g_skripte;
const karte::ShaderSkript& skript(const std::string& shader) {
    static const karte::ShaderSkript leer;
    const auto it = g_skripte.find(shader);
    if (it != g_skripte.end()) { return it->second; }
    karte::ShaderSkript s;
    if (ed().karteSpiel) {
        const karte::ShaderEintrag* e = ed().karteSpiel->inventar.shaderFinden(shader);
        if (e != nullptr && !e->istStandard()) {
            auto d = g_dateien.find(e->datei);
            if (d == g_dateien.end()) {
                std::string t;
                (void)ed().karteSpiel->fs.textLesen(e->datei, t);
                d = g_dateien.emplace(e->datei, karte::shaderDateiZerlegen(t)).first;
            }
            s = karte::shaderSkriptLesen(d->second, shader);
        }
    }
    return g_skripte.emplace(shader, s).first->second;
    (void)leer;
}

// LoadShaderImages (shaders.cpp): lightImage = q3map_lightImage, sonst das Shaderbild;
// si.color = ColorNormalize(averageColor) (die Normierung macht echtlicht.cpp).
std::map<std::string, Vektor3f> g_lichtBildFarben;
Vektor3f shaderFarbe(const std::string& sh) {
    const karte::ShaderSkript& s = skript(sh);
    if (!s.lichtBild.empty()) {
        const auto it = g_lichtBildFarben.find(s.lichtBild);
        if (it != g_lichtBildFarben.end()) { return it->second; }
        std::vector<unsigned char> rgba;
        int b = 0;
        int h = 0;
        Vektor3f f(1, 1, 1);
        if (kartenBildRgba(s.lichtBild, rgba, b, h) && b > 0 && h > 0) {
            double c[3] = {0, 0, 0};
            const std::size_t n = static_cast<std::size_t>(b) * static_cast<std::size_t>(h);
            for (std::size_t i = 0; i < n; ++i) {
                for (int k = 0; k < 3; ++k) { c[k] += rgba[i * 4 + static_cast<std::size_t>(k)]; }
            }
            f = Vektor3f(static_cast<float>(c[0] / n), static_cast<float>(c[1] / n), static_cast<float>(c[2] / n));
        }
        g_lichtBildFarben.emplace(s.lichtBild, f);
        return f;
    }
    float rgb[3] = {1, 1, 1};
    if (kameraTexturFarbe(sh, rgb)) { return Vektor3f(rgb[0], rgb[1], rgb[2]); }
    return Vektor3f(1, 1, 1);
}

struct LichtWelt {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    std::size_t entStand = static_cast<std::size_t>(-1);
    std::vector<EchtLicht> flaechen;   // Flaechenlicht und Sonnen der Himmel (haengen an der Geometrie)
    karte::EchtLichtWelt welt;         // alles zusammen
    karte::Kasten grenzen{Vektor3f(0, 0, 0), Vektor3f(0, 0, 0)};
};
LichtWelt g_welt;

// Die Schluessel der Lichter (und ihrer Ziele) - aendert sich auch beim Ziehen,
// ehe ein Rueckgaengig-Schritt entsteht.
std::size_t lichtSignatur() {
    std::size_t h = 1469598103934665603ULL;
    const auto dazu = [&h](const std::string& s) {
        for (const unsigned char c : s) { h = (h ^ c) * 1099511628211ULL; }
        h = (h ^ 0xFF) * 1099511628211ULL;
    };
    for (const auto& e : ed().karte.entitaeten()) {
        const std::string& k = e->klassenname();
        const bool licht = k.size() >= 5 && (k[0] == 'l' || k[0] == 'L');
        if (!licht && e->wert("targetname").empty() && e != ed().karte.welt()) { continue; }
        for (const auto& [s, w] : e->schluessel()) {
            dazu(s);
            dazu(w);
        }
    }
    return h;
}

void weltAktualisieren(std::size_t geoStand) {
    LichtWelt& w = g_welt;
    if (w.geoStand != geoStand) {
        w.geoStand = geoStand;
        const karte::EchtLichtWelt f = karte::echtLichterSammeln(
            ed().karte,
            [](const std::string& sh) {
                const karte::ShaderSkript& s = skript(sh);
                karte::EchtFlaechenShader r;
                r.flaechenlicht = static_cast<float>(s.flaechenlicht);
                if (s.lichtFarbe) {
                    r.farbe = Vektor3f(static_cast<float>(s.lichtRGB[0]), static_cast<float>(s.lichtRGB[1]), static_cast<float>(s.lichtRGB[2]));
                } else {
                    r.farbe = shaderFarbe(sh);
                }
                r.unterteilung = static_cast<float>(s.lichtUnterteilung);
                r.backsplashAnteil = static_cast<float>(s.backsplashAnteil);
                r.backsplashWeite = static_cast<float>(s.backsplashWeite);
                r.himmel = s.himmel;
                r.zweiseitig = s.zweiseitig || s.nebel;
                return r;
            },
            [](const std::string& sh) {
                const karte::ShaderSkript& s = skript(sh);
                karte::EchtSonne r;
                r.da = s.sonne;
                r.farbe = Vektor3f(static_cast<float>(s.sonneFarbe[0]), static_cast<float>(s.sonneFarbe[1]), static_cast<float>(s.sonneFarbe[2]));
                r.staerke = static_cast<float>(s.sonneStaerke);
                r.grad = static_cast<float>(s.sonneGrad);
                r.hoehe = static_cast<float>(s.sonneHoehe);
                for (const karte::ShaderSkript::Sonne& so : s.sonnen) {
                    r.sonnen.push_back({Vektor3f(static_cast<float>(so.farbe[0]), static_cast<float>(so.farbe[1]), static_cast<float>(so.farbe[2])),
                                        static_cast<float>(so.staerke), static_cast<float>(so.grad), static_cast<float>(so.hoehe)});
                }
                for (const karte::ShaderSkript::Himmelslicht& h : s.himmelsLichter) {
                    r.himmel.push_back({static_cast<float>(h.wert), h.schritte, h.horizontMin, h.horizontMax});
                }
                if (!r.himmel.empty()) { r.shaderFarbe = shaderFarbe(sh); }
                return r;
            });
        w.flaechen.clear();
        for (const EchtLicht& l : f.lichter) {
            if (l.art == EchtLicht::Art::Flaeche || (l.art == EchtLicht::Art::Sonne && l.quelle == nullptr)) {
                w.flaechen.push_back(l);
                // Flaechenlichter mit Ecken: die Ecken haengen am Licht
            }
        }
        // Grenzen der Welt (fuer die Schattenkarte der Sonne)
        bool erst = true;
        Vektor3f mn(0, 0, 0);
        Vektor3f mx(0, 0, 0);
        for (const auto& e : ed().karte.entitaeten()) {
            for (const auto& p : e->kinder()) {
                if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
                const karte::Kasten b = static_cast<const karte::Pinsel&>(*p).grenzen(ed().karte.kontext);
                const Vektor3f a = b.origin - b.extents;
                const Vektor3f z = b.origin + b.extents;
                for (int i = 0; i < 3; ++i) {
                    mn[i] = erst ? a[i] : std::min(mn[i], a[i]);
                    mx[i] = erst ? z[i] : std::max(mx[i], z[i]);
                }
                erst = false;
            }
        }
        w.grenzen = karte::Kasten((mn + mx) * 0.5F, (mx - mn) * 0.5F);
        w.entStand = static_cast<std::size_t>(-1);
    }
    const std::size_t sig = lichtSignatur();
    if (w.entStand != sig) {
        w.entStand = sig;
        karte::EchtLichtWelt e = karte::echtLichterSammeln(ed().karte, nullptr, nullptr);
        e.lichter.insert(e.lichter.end(), w.flaechen.begin(), w.flaechen.end());
        w.welt = std::move(e);
    }
}

// Schattenwuerfel: welcher Platz gehoert welchem Licht (Ort, Huelle, Geometrie)
struct Platz {
    bool gueltig = false;
    Vektor3f pos{0, 0, 0};
    float huelle = 0;
    std::size_t geoStand = 0;
    std::size_t zuletzt = 0;   // Bildnummer der letzten Verwendung
};
std::array<Platz, kartegpu::kSchattenMax> g_plaetze;
std::size_t g_bild = 0;
// Sonnenkarten: Kaskade 0 (ganze Karte), 1 (um die Kamera), Himmelssonnen
struct SonnenKarte {
    bool gueltig = false;
    Vektor3f richtung{0, 0, 0};
    Vektor3f mitte{0, 0, 0};
    std::size_t geoStand = 0;
};
std::array<SonnenKarte, 2> g_kaskade;
std::array<SonnenKarte, kartegpu::kHimmelSonnenMax> g_himmelKarten;

bool gleicherOrt(const Vektor3f& a, const Vektor3f& b) {
    return std::fabs(a[0] - b[0]) < 0.01F && std::fabs(a[1] - b[1]) < 0.01F && std::fabs(a[2] - b[2]) < 0.01F;
}

kartegpu::LichtGpu gpuLicht(const EchtLicht& l, std::vector<float>& ecken) {
    kartegpu::LichtGpu g{};
    g.pos[0] = l.ursprung[0];
    g.pos[1] = l.ursprung[1];
    g.pos[2] = l.ursprung[2];
    g.pos[3] = l.photonen;
    g.farbe[0] = l.farbe[0];
    g.farbe[1] = l.farbe[1];
    g.farbe[2] = l.farbe[2];
    g.farbe[3] = l.art == EchtLicht::Art::Punkt ? 0.0F : l.art == EchtLicht::Art::Spot ? 1.0F : l.art == EchtLicht::Art::Sonne ? 2.0F : 3.0F;
    const Vektor3f r = l.art == EchtLicht::Art::Sonne ? l.sonnenRichtung : l.normale;
    g.richtung[0] = r[0];
    g.richtung[1] = r[1];
    g.richtung[2] = r[2];
    g.richtung[3] = l.radiusJeAbstand;
    g.werte[0] = l.linear ? 1.0F : 0.0F;
    g.werte[1] = l.winkel ? 1.0F : 0.0F;
    g.werte[2] = l.fade;
    g.werte[3] = l.huelle;
    g.mehr[0] = -1.0F;
    g.mehr[1] = 0;
    g.mehr[2] = l.winkelSkala;
    g.mehr[3] = l.extraAbstand;
    if (l.art == EchtLicht::Art::Flaeche && l.wicklung.size() >= 3) {
        g.flaeche[0] = l.add;
        g.flaeche[1] = l.zweiseitig ? 1.0F : 0.0F;
        g.flaeche[2] = static_cast<float>(ecken.size() / 3);
        g.flaeche[3] = static_cast<float>(l.wicklung.size());
        for (const Vektor3f& v : l.wicklung) {
            ecken.push_back(v[0]);
            ecken.push_back(v[1]);
            ecken.push_back(v[2]);
        }
    }
    g.zusatz[0] = l.negativ ? 1.0F : 0.0F;
    return g;
}

// --- Stufe 9c: Schattengeometrie nach den Regeln von q3map2 ------------------------------
//
// light_trace.cpp PopulateWithBSPModel: nur Flaechen mit castShadows (worldspawn und
// func_group 1, andere Entities _castShadows/_cs, sonst 0 - ENTITY_CAST_SHADOWS),
// ohne C_NODRAW, durchscheinende nur mit alphashadow/lightfilter, Patches nur mit
// -patchshadows, Himmel nie. SetupTraceNodes_r: Blaetter in strukturell deckenden
// Pinseln sind fest (Punkt-/Spot-/Flaechenlicht: TraceLine ohne testAll haelt dort);
// fuer die Sonne (testAll) zaehlen sie nur, wenn der Strahl vorher keinen Himmel traf.
struct SchattenGeo {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    bool patches = false;
    unsigned geraet = ~0U;
    kartegpu::Puffer3 puffer;
    std::vector<kartegpu::SchattenTeil> teile;
    std::size_t dreiecke = 0;
};
SchattenGeo g_schatten;

int entitaetWirft(const karte::Entitaet& e, bool welt) {
    if (welt) { return 1; }   // WORLDSPAWN_CAST_SHADOWS (func_group gehoert nach -bsp zur Welt)
    for (const char* k : {"_castShadows", "_cs"}) {
        const std::string w = e.wert(k);
        if (!w.empty()) { return std::atoi(w.c_str()); }
    }
    return 0;   // ENTITY_CAST_SHADOWS
}

void schattenGeoBauen(std::size_t geoStand, bool patches) {
    SchattenGeo& sg = g_schatten;
    sg.teile.clear();
    sg.dreiecke = 0;
    // Eimer: Art, Zelle (1024er Raster), Shader (nur alphashadow)
    std::map<std::tuple<int, int, int, int, std::string>, std::vector<kartegpu::Punkt3>> eimer;
    const auto dreieck = [&eimer](int art, const std::string& sh, const Vektor3f* p, const Vektor3f& n, const karte::Vektor2f* st) {
        const Vektor3f m = (p[0] + p[1] + p[2]) / 3.0F;
        const auto zelle = [](float v) { return static_cast<int>(std::floor(v / 1024.0F)); };
        auto& v = eimer[{art, zelle(m[0]), zelle(m[1]), zelle(m[2]), art == 3 ? sh : std::string()}];
        for (int i = 0; i < 3; ++i) {
            kartegpu::Punkt3 q{};
            q.pos[0] = p[i][0];
            q.pos[1] = p[i][1];
            q.pos[2] = p[i][2];
            q.normale[0] = n[0];
            q.normale[1] = n[1];
            q.normale[2] = n[2];
            q.st[0] = st != nullptr ? st[i][0] : 0.0F;
            q.st[1] = st != nullptr ? st[i][1] : 0.0F;
            q.farbe = 0xFFFFFFFFU;
            v.push_back(q);
        }
    };
    const auto welt = ed().karte.welt();
    for (const auto& ep : ed().karte.entitaeten()) {
        const karte::Entitaet& ent = *ep;
        const bool istWelt = ep == welt || ent.klassenname() == "func_group";
        const int wirft = entitaetWirft(ent, istWelt);
        for (const auto& prim : ent.kinder()) {
            if (prim->art() == karte::Primitiv::Art::Pinsel) {
                const auto& pinsel = static_cast<const karte::Pinsel&>(*prim);
                bool durch = false;
                bool himmel = false;
                for (const auto& fl : pinsel.flaechen()) {
                    const karte::ShaderSkript& s = skript(fl->shader());
                    durch = durch || s.durchscheinend;
                    himmel = himmel || s.himmel;
                }
                // SetBrushContents: durchscheinende Pinsel sind immer detail; deckend = keine durchscheinende Seite
                const bool volumen = istWelt && !pinsel.istDetail() && !durch;
                for (const auto& fl : pinsel.flaechen()) {
                    if (!fl->traegtBei()) { continue; }
                    const karte::Wicklung& wk = fl->wicklung();
                    if (wk.numpoints < 3) { continue; }
                    const karte::ShaderSkript& s = skript(fl->shader());
                    int art = -1;
                    if (s.himmel) {
                        art = 2;
                    } else if (s.nichtZeichnen || wirft <= 0 || (s.durchscheinend && !s.alphaSchatten && !s.lichtFilter)) {
                        art = volumen ? (himmel ? 4 : 1) : -1;
                    } else {
                        art = (s.alphaSchatten || s.lichtFilter) ? 3 : 0;
                    }
                    if (art < 0) { continue; }
                    const karte::Vektor3d& nd = fl->ebene().normal();
                    const Vektor3f n(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                    for (std::size_t i = 2; i < wk.numpoints; ++i) {
                        const Vektor3f p[3] = {Vektor3f(wk[0].vertex), Vektor3f(wk[i - 1].vertex), Vektor3f(wk[i].vertex)};
                        const karte::Vektor2f st[3] = {karte::Vektor2f(static_cast<float>(wk[0].texcoord[0]), static_cast<float>(wk[0].texcoord[1])),
                                                       karte::Vektor2f(static_cast<float>(wk[i - 1].texcoord[0]), static_cast<float>(wk[i - 1].texcoord[1])),
                                                       karte::Vektor2f(static_cast<float>(wk[i].texcoord[0]), static_cast<float>(wk[i].texcoord[1]))};
                        dreieck(art, fl->shader(), p, n, st);
                    }
                }
            } else if (prim->art() == karte::Primitiv::Art::Patch && patches && wirft > 0) {
                const auto& patch = static_cast<const karte::Patch&>(*prim);
                const karte::ShaderSkript& s = skript(patch.shader());
                if (s.himmel || s.nichtZeichnen || (s.durchscheinend && !s.alphaSchatten && !s.lichtFilter)) { continue; }
                const int art = (s.alphaSchatten || s.lichtFilter) ? 3 : 0;
                const karte::PatchTesselierung& tess = patch.tesselierung();
                for (std::size_t sIdx = 0; sIdx < tess.numStrips; ++sIdx) {
                    const std::size_t basis = sIdx * tess.lenStrips;
                    for (std::size_t k = 0; k + 2 < tess.lenStrips; ++k) {
                        const unsigned ia = tess.indices[basis + k];
                        const unsigned ib = tess.indices[basis + k + 1];
                        const unsigned ic = tess.indices[basis + k + 2];
                        if (ia >= tess.vertices.size() || ib >= tess.vertices.size() || ic >= tess.vertices.size()) { continue; }
                        const Vektor3f p[3] = {tess.vertices[ia].vertex, tess.vertices[ib].vertex, tess.vertices[ic].vertex};
                        const karte::Vektor2f st[3] = {tess.vertices[ia].texcoord, tess.vertices[ib].texcoord, tess.vertices[ic].texcoord};
                        dreieck(art, patch.shader(), p, tess.vertices[ia].normal, st);
                    }
                }
            }
        }
    }
    std::vector<kartegpu::Punkt3> alle;
    for (auto& [schluessel, v] : eimer) {
        kartegpu::SchattenTeil teil;
        teil.puffer = &sg.puffer;
        teil.erstes = alle.size();
        teil.anzahl = v.size();
        teil.art = std::get<0>(schluessel);
        if (teil.art == 3) { teil.bild = kameraTexturBild(std::get<4>(schluessel)); }
        for (int i = 0; i < 3; ++i) {
            teil.mn[i] = 1e30F;
            teil.mx[i] = -1e30F;
        }
        for (const kartegpu::Punkt3& q : v) {
            for (int i = 0; i < 3; ++i) {
                teil.mn[i] = std::min(teil.mn[i], q.pos[i]);
                teil.mx[i] = std::max(teil.mx[i], q.pos[i]);
            }
        }
        if (teil.art == 3 && teil.bild == nullptr) { teil.art = 0; }   // Bild noch nicht da: deckend
        alle.insert(alle.end(), v.begin(), v.end());
        sg.teile.push_back(teil);
    }
    sg.dreiecke = alle.size() / 3;
    if (!alle.empty()) { (void)sg.puffer.setzen(alle.data(), alle.size() * sizeof(kartegpu::Punkt3)); }
    sg.geoStand = geoStand;
    sg.patches = patches;
    sg.geraet = kartegpu::geraetStand();
}

// --- b) Schnell-Backen ------------------------------------------------------------------

struct Ergebnis {
    BspGeometry geo;
    BspMesh netz;
};

struct Backen {
    std::thread faden;
    std::atomic<bool> abbrechen{false};
    std::atomic<int> stand{0};         // 0 nichts, 2 laeuft, 3 fertig, 4 abgebrochen, 5 Fehler
    std::mutex mutex;
    std::unique_ptr<Ergebnis> ergebnis;
    std::string fehler;
    double sekunden = 0;
    std::size_t kartenStand = 0;       // der gebaute Stand
    std::string wahl;                  // Stufe 8d: was GPU bake Auto gewaehlt hat und warum
};
Backen g_backen;
std::size_t g_letzterStand = static_cast<std::size_t>(-1);
std::chrono::steady_clock::time_point g_letzteAenderung = std::chrono::steady_clock::now();
std::size_t g_gebackenStand = static_cast<std::size_t>(-1);
std::size_t g_fehlerStand = static_cast<std::size_t>(-1);   // dieser Stand scheiterte: nicht noch einmal

// was die Kamera zeigt
struct Gebacken {
    kartegpu::GebackenNetz netz;
    std::vector<void*> lightmaps;
    struct Teil {
        std::string shader;
        int lightmap = -1;
        std::uint32_t erster = 0;
        std::uint32_t anzahl = 0;
    };
    std::vector<Teil> teile;
    unsigned geraet = ~0U;
    void leeren() {
        netz.leeren();
        for (void* l : lightmaps) {
            if (l != nullptr) { kartegpu::bildFreigeben(l); }
        }
        lightmaps.clear();
        teile.clear();
    }
};
// nie zerstoert: beim Beenden ist das Grafikgeraet schon weg (ein Release im
// statischen Destruktor stuerzte ab); der Prozess gibt alles frei
Gebacken& g_gebacken = *new Gebacken;

void fadenAbwarten() {
    if (g_backen.faden.joinable()) {
        g_backen.abbrechen = true;
        g_backen.faden.join();
    }
}

std::string q3map2Pfad() {
    std::string p = platform::executableDirectory();
    if (!p.empty() && p.back() != '/' && p.back() != '\\') { p += '/'; }
    return p + "bhed_q3map2.exe";
}

std::string tempOrdner() {
    std::error_code ec;
    fs::path t = fs::temp_directory_path(ec);
    if (ec) { t = fs::path(platform::executableDirectory()); }
    t /= "bhed_schnellbacken_" + std::to_string(static_cast<unsigned long>(_getpid()));
    return t.string();
}

}  // namespace

std::string schnellBspFuer(std::size_t kartenStand) {
    if (!ed().echtLicht.backen || g_gebackenStand != kartenStand || g_backen.stand.load() == 2) { return std::string(); }
    const fs::path p = fs::path(tempOrdner()) / "maps" / "schnell.bsp";
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) { return std::string(); }
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

namespace {

void schnellBauStarten(std::size_t kartenStand) {
    fadenAbwarten();
    Editor& e = ed();
    const std::string ordner = tempOrdner();
    std::error_code ec;
    fs::create_directories(fs::path(ordner) / "maps", ec);
    const std::string stamm = "schnell";
    const std::string karte = (fs::path(ordner) / "maps" / (stamm + ".map")).string();
    std::string fehlerText;
    if (!karte::karteSpeichern(karte, e.karte, false, &fehlerText)) {
        e.echtLicht.bauStand = 5;
        return;
    }
    e.echtLicht.bauOrdner = ordner;
    const std::string grund = "\"" + q3map2Pfad() + "\" -game " + (kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja") +
                              " -fs_basepath \"" + enginePfadAnzeige() + "\" -fs_homepath \"\" -fs_game " + bauSpielName() + " ";
    const std::string bsp = karte.substr(0, karte.size() - 4) + ".bsp";
    // -vis nur mit einer .prt (eine Karte, die leckt, hat keine - waehrend des Bauens
    // oft der Fall; -light geht auch ohne Sichtbarkeit)
    // Stufe 8g: mit Build > GPU bake rechnet die GPU das Licht (ohne die Nachpruefung: schnell).
    // Stufe 8d: Auto - bhed_q3map2 waehlt mit den gemerkten Zeiten des Schnell-Backens dieser Karte
    const karte::GpuBacken modus = e.bau.gpuBacken;
    const karte::GpuZeiten zeiten = modus == karte::GpuBacken::Auto ? gpuZeitenHolen(e.name, "-light -fast") : karte::GpuZeiten{};
    const std::string gpu = modus == karte::GpuBacken::Ein    ? "-bhedgpubake -bhedgpubakecheck 0 "
                            : modus == karte::GpuBacken::Auto ? "-bhedgpubakeauto -bhedgpubakezeiten " + std::to_string(zeiten.cpuMs) + " " +
                                                                    std::to_string(zeiten.gpuMs) + " -bhedgpubakecheck 0 "
                                                              : std::string();
    const std::vector<std::string> befehle = {grund + "-meta \"" + karte + "\"", grund + "-vis -fast \"" + bsp + "\"",
                                              grund + "-light -fast " + gpu + "\"" + bsp + "\""};
    const std::string kartenName = e.name;
    g_backen.abbrechen = false;
    g_backen.stand = 2;
    g_backen.kartenStand = kartenStand;
    e.echtLicht.bauStand = 2;
    g_backen.faden = std::thread([befehle, bsp, modus, zeiten, kartenName] {
        const auto t0 = std::chrono::steady_clock::now();
        for (std::size_t schritt = 0; schritt < befehle.size(); ++schritt) {
            const std::string& b = befehle[schritt];
            std::error_code ec;
            if (schritt == 1 && !fs::exists(fs::path(bsp.substr(0, bsp.size() - 4) + ".prt"), ec)) { continue; }
            bauplattform::Prozess p;
            p.niedrigePrioritaet(true);
            std::string grund;
            // Stufe 8d: beim -light-Schritt die Ausgabe lesen (die Auto-Wahl) und die Dauer messen
            const bool licht = schritt == 2;
            const auto lichtStart = std::chrono::steady_clock::now();
            std::string ausgabe;
            if (!p.starten(b, std::string(), licht, grund)) {
                std::lock_guard<std::mutex> l(g_backen.mutex);
                g_backen.fehler = "bhed_q3map2: " + grund;
                g_backen.stand = 5;
                return;
            }
            while (!p.warten(30)) {
                if (licht) { ausgabe += p.lesen(); }
                if (g_backen.abbrechen) {
                    p.beenden();
                    g_backen.stand = 4;
                    return;
                }
            }
            if (licht) {
                ausgabe += p.lesen();
                karte::GpuBackBericht bericht;
                std::size_t a = 0;
                while (a < ausgabe.size()) {
                    std::size_t z = ausgabe.find('\n', a);
                    if (z == std::string::npos) { z = ausgabe.size(); }
                    std::string zeile = ausgabe.substr(a, z - a);
                    if (!zeile.empty() && zeile.back() == '\r') { zeile.pop_back(); }
                    (void)karte::gpuBackZeile(zeile, bericht);
                    a = z + 1;
                }
                const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - lichtStart).count();
                if (p.rueckgabe() == 0 && modus != karte::GpuBacken::Aus) {
                    const bool gpuLief = !bericht.keineGpu && (modus == karte::GpuBacken::Ein || bericht.autoGpu);
                    gpuZeitMerken(kartenName, "-light -fast", gpuLief, sek);
                }
                std::lock_guard<std::mutex> l(g_backen.mutex);
                g_backen.wahl = gpuWahlText(bericht, zeiten.cpuMs, zeiten.gpuMs);
            }
            if (p.rueckgabe() != 0) {
                std::lock_guard<std::mutex> l(g_backen.mutex);
                g_backen.fehler = "bhed_q3map2 ended with code " + std::to_string(p.rueckgabe());
                g_backen.stand = 5;
                return;
            }
        }
        std::string bytes;
        {
            std::ifstream f(fs::path(bsp), std::ios::binary);
            std::ostringstream s;
            s << f.rdbuf();
            bytes = s.str();
        }
        auto r = std::make_unique<Ergebnis>();
        std::string fehler;
        if (!readBspGeometry(bytes, r->geo, &fehler)) {
            std::lock_guard<std::mutex> l(g_backen.mutex);
            g_backen.fehler = "reading the quick bake: " + fehler;
            g_backen.stand = 5;
            return;
        }
        r->netz = buildMesh(r->geo, 4, true);
        std::lock_guard<std::mutex> l(g_backen.mutex);
        g_backen.ergebnis = std::move(r);
        g_backen.sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        g_backen.stand = 3;
    });
}

// Ein gebautes Netz auf die Grafikkarte (Schnell-Backen, Stufe 8a: die Vorlage)
void netzUebernehmen(Gebacken& ziel, std::unique_ptr<Ergebnis> r) {
    if (!r) { return; }
    ziel.leeren();
    std::vector<kartegpu::GebackenPunkt> punkte;
    punkte.reserve(r->netz.verts.size());
    for (const BspVertex& v : r->netz.verts) {
        kartegpu::GebackenPunkt p{};
        std::copy(v.xyz, v.xyz + 3, p.pos);
        std::copy(v.st, v.st + 2, p.st);
        std::copy(v.lightmap, v.lightmap + 2, p.lm);
        std::copy(v.normal, v.normal + 3, p.normale);
        p.farbe = static_cast<std::uint32_t>(v.colour[0]) | (static_cast<std::uint32_t>(v.colour[1]) << 8U) |
                  (static_cast<std::uint32_t>(v.colour[2]) << 16U) | (static_cast<std::uint32_t>(v.colour[3]) << 24U);
        punkte.push_back(p);
    }
    if (!ziel.netz.setzen(punkte, r->netz.indexes)) { return; }
    for (std::size_t i = 0; i < r->geo.lightmaps.size(); ++i) {
        ziel.lightmaps.push_back(kartegpu::lightmapAnlegen(r->geo.lightmaps[i].data(), r->geo.lightmapW(i), r->geo.lightmapH(i)));
    }
    for (const BspMesh::Batch& b : r->netz.batches) {
        if (b.shader < 0 || static_cast<std::size_t>(b.shader) >= r->geo.shaders.size()) { continue; }
        const BspShader& sh = r->geo.shaders[static_cast<std::size_t>(b.shader)];
        if (!sh.isDrawn() || sh.isSky()) { continue; }
        ziel.teile.push_back({sh.name, b.lightmap, b.firstIndex, b.numIndexes});
    }
    ziel.geraet = kartegpu::geraetStand();
}

void ergebnisUebernehmen() {
    std::unique_ptr<Ergebnis> r;
    {
        std::lock_guard<std::mutex> l(g_backen.mutex);
        r = std::move(g_backen.ergebnis);
    }
    netzUebernehmen(g_gebacken, std::move(r));
}

// Stufe 8a Punkt 5c: die Vorlage - eine fertige .bsp, gezeichnet und nicht bearbeitbar
Gebacken& g_vorlage = *new Gebacken;
std::unique_ptr<Ergebnis> g_vorlageWartet;          // gelesen, noch nicht auf der Grafikkarte
std::vector<karte::Linie> g_vorlageLinien;          // Kanten fuer die 2D-Ansichten
std::size_t g_vorlageStand = 0;
std::string g_vorlagePfad;

}  // namespace

bool echtLichtAktiv() {
    const Editor& e = ed();
    return e.echtLicht.an && (e.kameraE.modus == karte::KameraModus::Textur || e.kameraE.modus == karte::KameraModus::TexturDraht) &&
           kartegpu::echtLichtVerfuegbar();
}

SpielDarstellung spielDarstellung(const std::string& shader) {
    SpielDarstellung d;
    const karte::ShaderSkript& s = skript(shader);
    d.unsichtbar = s.nichtZeichnen;   // C_NODRAW: q3map2 laesst die Flaeche weg
    // tr_shader.cpp NameToAFunc: GT0 (> 0), LT128 (< 0.5), GE128 (>= 0.5)
    if (s.stufeAlpha == 1) {
        d.alphaFunktion = 3;
        d.alphaRef = 0.0F;
    } else if (s.stufeAlpha == 2) {
        d.alphaFunktion = 2;
        d.alphaRef = 0.5F;
    } else if (s.stufeAlpha == 3) {
        d.alphaFunktion = 5;
        d.alphaRef = 0.5F;
    }
    d.mischQuelle = s.mischQuelle;
    d.mischZiel = s.mischZiel;
    return d;
}

namespace {

// Bildrechteck (Kacheln) einer Kugel; false: ausserhalb des Bilds. vp spaltenweise (clip = M p).
bool kugelImBild(const float* vp, int breite, int hoehe, const Vektor3f& c, float r, int kachelnX, int kachelnY, int& x0, int& y0, int& x1, int& y1) {
    float mnx = 1e30F;
    float mny = 1e30F;
    float mxx = -1e30F;
    float mxy = -1e30F;
    bool hinten = false;
    int vorn = 0;
    for (int k = 0; k < 8; ++k) {
        const float p[3] = {c[0] + ((k & 1) != 0 ? r : -r), c[1] + ((k & 2) != 0 ? r : -r), c[2] + ((k & 4) != 0 ? r : -r)};
        const float x = vp[0] * p[0] + vp[4] * p[1] + vp[8] * p[2] + vp[12];
        const float y = vp[1] * p[0] + vp[5] * p[1] + vp[9] * p[2] + vp[13];
        const float w = vp[3] * p[0] + vp[7] * p[1] + vp[11] * p[2] + vp[15];
        if (w <= 1.0F) {
            hinten = true;
            continue;
        }
        ++vorn;
        const float sx = (x / w * 0.5F + 0.5F) * static_cast<float>(breite);
        const float sy = (0.5F - y / w * 0.5F) * static_cast<float>(hoehe);
        mnx = std::min(mnx, sx);
        mny = std::min(mny, sy);
        mxx = std::max(mxx, sx);
        mxy = std::max(mxy, sy);
    }
    if (hinten) {
        if (vorn == 0) {
            // ganz hinter dem Auge? Nur, wenn auch der Mittelpunkt weit hinten liegt
            const float w = vp[3] * c[0] + vp[7] * c[1] + vp[11] * c[2] + vp[15];
            if (w < -r) { return false; }
        }
        x0 = 0;
        y0 = 0;
        x1 = kachelnX - 1;
        y1 = kachelnY - 1;
        return true;
    }
    if (mxx < 0 || mxy < 0 || mnx > static_cast<float>(breite) || mny > static_cast<float>(hoehe)) { return false; }
    x0 = std::clamp(static_cast<int>(mnx) / kartegpu::kKachel, 0, kachelnX - 1);
    y0 = std::clamp(static_cast<int>(mny) / kartegpu::kKachel, 0, kachelnY - 1);
    x1 = std::clamp(static_cast<int>(mxx) / kartegpu::kKachel, 0, kachelnX - 1);
    y1 = std::clamp(static_cast<int>(mxy) / kartegpu::kKachel, 0, kachelnY - 1);
    return true;
}

// Ein Platz fuer den Schattenwuerfel eines Lichts: vorhanden, neu (Budget) oder -1
int wuerfelPlatz(const EchtLicht& l, std::size_t geoStand, bool gross, int& budget) {
    const float weite = std::min(l.huelle, 65536.0F);
    for (std::size_t s = 0; s < g_plaetze.size(); ++s) {
        const Platz& p = g_plaetze[s];
        if (p.gueltig && p.geoStand == geoStand && gleicherOrt(p.pos, l.ursprung) && p.huelle == weite) { return static_cast<int>(s); }
    }
    if (budget <= 0) { return -1; }
    const std::size_t von = gross ? 0 : static_cast<std::size_t>(kartegpu::kSchattenGross);
    const std::size_t bis = gross ? static_cast<std::size_t>(kartegpu::kSchattenGross) : g_plaetze.size();
    std::size_t best = von;
    for (std::size_t s = von; s < bis; ++s) {
        if (!g_plaetze[s].gueltig) {
            best = s;
            break;
        }
        if (g_plaetze[s].zuletzt < g_plaetze[best].zuletzt) { best = s; }
    }
    if (g_plaetze[best].gueltig && g_plaetze[best].zuletzt == g_bild) { return -1; }   // alle in diesem Bild belegt
    const float pos[3] = {l.ursprung[0], l.ursprung[1], l.ursprung[2]};
    if (!kartegpu::schattenWuerfel(static_cast<int>(best), pos, weite, g_schatten.teile)) { return -1; }
    g_plaetze[best] = Platz{true, l.ursprung, weite, geoStand, g_bild};
    --budget;
    ++ed().echtLicht.schattenNeu;
    return static_cast<int>(best);
}

}  // namespace

void echtLichtBildVorbereiten(const Vektor3f& auge, const float* vp, int breite, int hoehe, std::size_t geoStand) {
    Editor& e = ed();
    const auto t0 = std::chrono::steady_clock::now();
    ++g_bild;
    weltAktualisieren(geoStand);
    if (g_schatten.geoStand != geoStand || g_schatten.patches != e.echtLicht.patchSchatten || g_schatten.geraet != kartegpu::geraetStand()) {
        schattenGeoBauen(geoStand, e.echtLicht.patchSchatten);
        for (Platz& p : g_plaetze) { p.gueltig = false; }
        for (SonnenKarte& k : g_kaskade) { k.gueltig = false; }
        for (SonnenKarte& k : g_himmelKarten) { k.gueltig = false; }
    }
    const karte::EchtLichtWelt& w = g_welt.welt;
    e.echtLicht.lichter = w.lichter.size();
    const int kachelnX = std::max(1, (breite + kartegpu::kKachel - 1) / kartegpu::kKachel);
    const int kachelnY = std::max(1, (hoehe + kartegpu::kKachel - 1) / kartegpu::kKachel);
    // 1. Sichtbarkeit (Huelle im Bild) und Rang nach dem Beitrag am Auge
    struct Wahl {
        std::size_t i = 0;
        float wert = 0;
        float faktor = 1;   // Himmelssonnen: Energie der weggelassenen
        int platz = -1;     // Sonne: Schicht bzw. -3 Kaskaden
        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
    };
    std::vector<Wahl> wahl;
    std::vector<std::size_t> sonnen;
    for (std::size_t i = 0; i < w.lichter.size(); ++i) {
        const EchtLicht& l = w.lichter[i];
        if (l.art == EchtLicht::Art::Sonne) {
            sonnen.push_back(i);
            continue;
        }
        Wahl x;
        x.i = i;
        if (!kugelImBild(vp, breite, hoehe, l.ursprung, std::min(l.huelle, 65536.0F), kachelnX, kachelnY, x.x0, x.y0, x.x1, x.y1)) { continue; }
        const Vektor3f d = l.ursprung - auge;
        const float ab = std::max(16.0F, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
        x.wert = l.linear ? std::max(0.0F, l.photonen * karte::kEchtLinearSkala - ab * l.fade) : l.photonen / (ab * ab);
        wahl.push_back(x);
    }
    std::stable_sort(wahl.begin(), wahl.end(), [](const Wahl& a, const Wahl& b) { return a.wert > b.wert; });
    // 2. Sonnen: die staerkste ohne q3map_skyLight bekommt die Kaskaden, die anderen
    // je eine Karte (zu viele: gleichmaessig ausgewaehlt, Energie erhalten)
    std::vector<Wahl> sw;
    {
        std::size_t haupt = static_cast<std::size_t>(-1);
        for (const std::size_t i : sonnen) {
            if (!w.lichter[i].himmel && (haupt == static_cast<std::size_t>(-1) || w.lichter[i].photonen > w.lichter[haupt].photonen)) { haupt = i; }
        }
        std::vector<std::size_t> andere;
        for (const std::size_t i : sonnen) {
            if (i != haupt) { andere.push_back(i); }
        }
        if (haupt != static_cast<std::size_t>(-1)) {
            Wahl x;
            x.i = haupt;
            x.platz = -3;
            x.x1 = kachelnX - 1;
            x.y1 = kachelnY - 1;
            sw.push_back(x);
        }
        const std::size_t n = andere.size();
        const std::size_t m = std::min<std::size_t>(n, kartegpu::kHimmelSonnenMax);
        double alle = 0;
        double gewaehlt = 0;
        for (const std::size_t i : andere) { alle += w.lichter[i].photonen; }
        std::vector<std::size_t> auswahl;
        for (std::size_t j = 0; j < m; ++j) {
            auswahl.push_back(andere[j * n / m]);
            gewaehlt += w.lichter[auswahl.back()].photonen;
        }
        for (std::size_t j = 0; j < auswahl.size(); ++j) {
            Wahl x;
            x.i = auswahl[j];
            x.platz = static_cast<int>(j);
            x.faktor = gewaehlt > 0 ? static_cast<float>(alle / gewaehlt) : 1.0F;
            x.x1 = kachelnX - 1;
            x.y1 = kachelnY - 1;
            sw.push_back(x);
        }
    }
    // 3. Sonnenkarten: hoechstens eine Kaskade und zwei Himmelskarten je Bild neu
    const Vektor3f wm = g_welt.grenzen.origin;
    const Vektor3f we = g_welt.grenzen.extents;
    const float radius = std::sqrt(we[0] * we[0] + we[1] * we[1] + we[2] * we[2]) + 64.0F;
    int sonnenBudget = 1;
    int himmelBudget = 2;
    bool hauptDa = false;
    for (const Wahl& x : sw) {
        const EchtLicht& l = w.lichter[x.i];
        const float r[3] = {l.sonnenRichtung[0], l.sonnenRichtung[1], l.sonnenRichtung[2]};
        if (x.platz == -3) {
            hauptDa = true;
            SonnenKarte& k0 = g_kaskade[0];
            if ((!k0.gueltig || k0.geoStand != geoStand || !gleicherOrt(k0.richtung, l.sonnenRichtung)) && sonnenBudget > 0) {
                const float mi[3] = {wm[0], wm[1], wm[2]};
                k0.gueltig = kartegpu::sonnenKaskade(0, r, mi, radius, radius + 64.0F, g_schatten.teile);
                k0.richtung = l.sonnenRichtung;
                k0.geoStand = geoStand;
                g_kaskade[1].gueltig = false;
                --sonnenBudget;
            }
            SonnenKarte& k1 = g_kaskade[1];
            const Vektor3f dk = auge - k1.mitte;
            const bool weg = std::sqrt(dk[0] * dk[0] + dk[1] * dk[1] + dk[2] * dk[2]) > 256.0F;
            if (k0.gueltig && (!k1.gueltig || k1.geoStand != geoStand || !gleicherOrt(k1.richtung, l.sonnenRichtung) || weg) && sonnenBudget > 0) {
                const float mi[3] = {auge[0], auge[1], auge[2]};
                const Vektor3f dz = auge - wm;
                const float tiefe = radius + std::sqrt(dz[0] * dz[0] + dz[1] * dz[1] + dz[2] * dz[2]) + 64.0F;
                k1.gueltig = kartegpu::sonnenKaskade(1, r, mi, 1536.0F, tiefe, g_schatten.teile);
                k1.richtung = l.sonnenRichtung;
                k1.mitte = auge;
                k1.geoStand = geoStand;
                --sonnenBudget;
            }
        } else if (x.platz >= 0) {
            SonnenKarte& k = g_himmelKarten[static_cast<std::size_t>(x.platz)];
            if ((!k.gueltig || k.geoStand != geoStand || !gleicherOrt(k.richtung, l.sonnenRichtung)) && himmelBudget > 0) {
                const float mi[3] = {wm[0], wm[1], wm[2]};
                k.gueltig = kartegpu::himmelsSonne(x.platz, r, mi, radius, radius + 64.0F, g_schatten.teile);
                k.richtung = l.sonnenRichtung;
                k.geoStand = geoStand;
                --himmelBudget;
            }
        }
    }
    if (!hauptDa) { kartegpu::sonnenSchattenAus(); }
    // 4. Lichtliste: Sonnen, dann nach Rang; Schattenwuerfel fuer die ersten
    // kSchattenMax (die ersten kSchattenGross gross), hoechstens 8 neu je Bild.
    // Wer noch auf seinen Wuerfel wartet, fehlt in diesem Bild (sonst leuchtete es
    // kurz durch die Waende); die Lichter danach kommen ohne Schatten.
    std::vector<kartegpu::LichtGpu> gpu;
    std::vector<float> ecken;
    std::vector<const Wahl*> liste;
    gpu.reserve(kartegpu::kLichtMax);
    int budget = 8;
    for (const Wahl& x : sw) {
        if (gpu.size() >= static_cast<std::size_t>(kartegpu::kLichtMax)) { break; }
        const EchtLicht& l = w.lichter[x.i];
        kartegpu::LichtGpu g = gpuLicht(l, ecken);
        g.pos[3] *= x.faktor;
        const bool karte = x.platz == -3 ? g_kaskade[0].gueltig : g_himmelKarten[static_cast<std::size_t>(x.platz)].gueltig;
        if (!karte) { continue; }   // die Karte kommt in einem der naechsten Bilder
        g.mehr[0] = static_cast<float>(x.platz);
        gpu.push_back(g);
        liste.push_back(&x);
    }
    std::size_t rang = 0;
    for (const Wahl& x : wahl) {
        if (gpu.size() >= static_cast<std::size_t>(kartegpu::kLichtMax)) { break; }
        const EchtLicht& l = w.lichter[x.i];
        const std::size_t eckenVorher = ecken.size();
        kartegpu::LichtGpu g = gpuLicht(l, ecken);
        if (rang < static_cast<std::size_t>(kartegpu::kSchattenMax)) {
            const int platz = wuerfelPlatz(l, geoStand, rang < static_cast<std::size_t>(kartegpu::kSchattenGross), budget);
            ++rang;
            if (platz < 0) {
                ecken.resize(eckenVorher);
                continue;
            }
            g_plaetze[static_cast<std::size_t>(platz)].zuletzt = g_bild;
            g.mehr[0] = static_cast<float>(platz);
            g.mehr[1] = std::min(l.huelle, 65536.0F);
        }
        gpu.push_back(g);
        liste.push_back(&x);
    }
    // 5. Kachelmasken
    const int worte = std::max(1, static_cast<int>((gpu.size() + 31) / 32));
    std::vector<std::uint32_t> kacheln(static_cast<std::size_t>(kachelnX) * static_cast<std::size_t>(kachelnY) * static_cast<std::size_t>(worte), 0U);
    for (std::size_t j = 0; j < liste.size(); ++j) {
        const Wahl& x = *liste[j];
        const std::uint32_t bit = 1U << (j % 32U);
        const std::size_t wort = j / 32U;
        for (int ky = x.y0; ky <= x.y1; ++ky) {
            for (int kx = x.x0; kx <= x.x1; ++kx) {
                kacheln[(static_cast<std::size_t>(ky) * static_cast<std::size_t>(kachelnX) + static_cast<std::size_t>(kx)) * static_cast<std::size_t>(worte) + wort] |= bit;
            }
        }
    }
    const float umgebung[3] = {w.umgebung[0], w.umgebung[1], w.umgebung[2]};
    const float minimum[3] = {w.minimum[0], w.minimum[1], w.minimum[2]};
    kartegpu::lichtSetzen(gpu, ecken, kacheln, kachelnX, kachelnY, worte, umgebung, minimum);
    e.echtLicht.gezeichnet = gpu.size();
    e.echtLicht.lichtMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

std::string echtLichtHinweis() {
    const Editor& e = ed();
    char z[256];
    if (e.echtLicht.gebackenZeigen) {
        std::snprintf(z, sizeof(z), trK(Str::KeLichtGebacken), e.echtLicht.bauSekunden);
        return z;
    }
    std::string s = trK(Str::KeLichtVorschau);
    if (e.echtLicht.backen && e.echtLicht.bauStand == 2) { s += std::string(" - ") + trK(Str::KeLichtBaeckt); }
    return s;
}

void echtLichtSchritt(std::size_t kartenStand) {
    Editor& e = ed();
    // fertigen Bau abholen
    const int st = g_backen.stand.load();
    if (st == 3) {
        if (g_backen.faden.joinable()) { g_backen.faden.join(); }
        ergebnisUebernehmen();
        g_gebackenStand = g_backen.kartenStand;
        e.echtLicht.bauSekunden = g_backen.sekunden;
        e.echtLicht.bauStand = 3;
        {   // Stufe 8d: die Wahl von Auto in der Konsole - nur wenn sie sich aendert
            std::lock_guard<std::mutex> l(g_backen.mutex);
            if (!g_backen.wahl.empty() && g_backen.wahl != e.bau.gpuWahlText) {
                e.bau.gpuWahlText = g_backen.wahl;
                konsoleSchreiben(KonsolenZeile::Art::Ausgabe, g_backen.wahl + "\n");
            }
        }
        g_backen.stand = 0;
    } else if (st == 4 || st == 5) {
        if (g_backen.faden.joinable()) { g_backen.faden.join(); }
        e.echtLicht.bauStand = st;
        if (st == 5) {
            g_fehlerStand = g_backen.kartenStand;
            std::lock_guard<std::mutex> l(g_backen.mutex);
            fehler(g_backen.fehler);
        }
        g_backen.stand = 0;
    }
    if (kartenStand != g_letzterStand) {
        g_letzterStand = kartenStand;
        g_letzteAenderung = std::chrono::steady_clock::now();
        if (g_backen.stand.load() == 2) {   // eine neue Aenderung bricht den laufenden Bau ab
            g_backen.abbrechen = true;
            ++e.echtLicht.baueAbgebrochen;
        }
    }
    e.echtLicht.gebackenZeigen = e.echtLicht.backen && g_gebackenStand == kartenStand && g_gebacken.netz.gueltig() &&
                                 g_gebacken.geraet == kartegpu::geraetStand();
    if (!e.echtLicht.backen) {
        if (g_backen.stand.load() == 2) { g_backen.abbrechen = true; }
        return;
    }
    const bool leer = e.karte.entitaeten().empty();
    if (!leer && g_backen.stand.load() == 0 && !g_backen.faden.joinable() && g_gebackenStand != kartenStand && g_fehlerStand != kartenStand &&
        std::chrono::duration<double>(std::chrono::steady_clock::now() - g_letzteAenderung).count() >= e.echtLicht.pauseSekunden) {
        if (e.echtLicht.bauStand == 4 || e.echtLicht.bauStand == 5 || e.echtLicht.bauStand == 3 || e.echtLicht.bauStand == 0 ||
            e.echtLicht.bauStand == 1) {
            schnellBauStarten(kartenStand);
        }
    } else if (g_backen.stand.load() == 0 && g_gebackenStand != kartenStand) {
        e.echtLicht.bauStand = 1;   // wartet auf die Pause
    }
}

void echtLichtBeenden() {
    fadenAbwarten();   // die Grafik-Teile bleiben (das Geraet kann schon weg sein)
}

bool vorlageLaden(const std::string& bytes, const std::string& pfad, std::string* fehlerText) {
    auto r = std::make_unique<Ergebnis>();
    std::string f;
    if (!readBspGeometry(bytes, r->geo, &f)) {
        if (fehlerText != nullptr) { *fehlerText = f; }
        return false;
    }
    r->netz = buildMesh(r->geo, 4, true);
    // Kanten (je Dreieck drei, doppelte einmal) fuer die 2D-Ansichten
    g_vorlageLinien.clear();
    std::set<std::pair<std::uint32_t, std::uint32_t>> schon;
    const karte::Farbe grau = 0xFF707070U;
    for (std::size_t i = 0; i + 2 < r->netz.indexes.size(); i += 3) {
        for (int k = 0; k < 3; ++k) {
            const std::uint32_t a = r->netz.indexes[i + static_cast<std::size_t>(k)];
            const std::uint32_t b = r->netz.indexes[i + static_cast<std::size_t>((k + 1) % 3)];
            if (a >= r->netz.verts.size() || b >= r->netz.verts.size() || !schon.insert({std::min(a, b), std::max(a, b)}).second) { continue; }
            const BspVertex& va = r->netz.verts[a];
            const BspVertex& vb = r->netz.verts[b];
            g_vorlageLinien.push_back({Vektor3f(va.xyz[0], va.xyz[1], va.xyz[2]), Vektor3f(vb.xyz[0], vb.xyz[1], vb.xyz[2]), grau});
        }
    }
    g_vorlageWartet = std::move(r);
    g_vorlagePfad = pfad;
    ++g_vorlageStand;
    ++ed().sichtStand;
    return true;
}

void vorlageEntfernen() {
    g_vorlage.leeren();
    g_vorlageWartet.reset();
    g_vorlageLinien.clear();
    g_vorlagePfad.clear();
    ++g_vorlageStand;
    ++ed().sichtStand;
}

bool vorlageAktiv() { return !g_vorlagePfad.empty(); }
const std::string& vorlagePfad() { return g_vorlagePfad; }
const std::vector<karte::Linie>& vorlageLinien() { return g_vorlageLinien; }
std::size_t vorlageStand() { return g_vorlageStand; }

bool vorlageZeichnen(const float* matrix) {
    if (g_vorlageWartet) { netzUebernehmen(g_vorlage, std::move(g_vorlageWartet)); }
    if (!vorlageAktiv() || !g_vorlage.netz.gueltig()) { return false; }
    for (const Gebacken::Teil& t : g_vorlage.teile) {
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.wegschneiden = true;
        float rgb[3] = {1, 1, 1};
        void* bild = kameraTexturBild(t.shader);
        if (bild == nullptr && kameraTexturFarbe(t.shader, rgb)) {
            z.farbe[0] = rgb[0];
            z.farbe[1] = rgb[1];
            z.farbe[2] = rgb[2];
        }
        z.bild = bild;
        void* lm = (t.lightmap >= 0 && static_cast<std::size_t>(t.lightmap) < g_vorlage.lightmaps.size()) ? g_vorlage.lightmaps[static_cast<std::size_t>(t.lightmap)] : nullptr;
        kartegpu::gebackenZeichnen(g_vorlage.netz, t.erster, t.anzahl, lm, z);
    }
    return true;
}

bool gebackenZeichnen(const float* matrix) {
    if (!ed().echtLicht.gebackenZeigen) { return false; }
    for (const Gebacken::Teil& t : g_gebacken.teile) {
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.wegschneiden = true;
        void* bild = nullptr;
        float rgb[3] = {1, 1, 1};
        bild = kameraTexturBild(t.shader);
        if (bild == nullptr && kameraTexturFarbe(t.shader, rgb)) {
            z.farbe[0] = rgb[0];
            z.farbe[1] = rgb[1];
            z.farbe[2] = rgb[2];
        }
        z.bild = bild;
        void* lm = (t.lightmap >= 0 && static_cast<std::size_t>(t.lightmap) < g_gebacken.lightmaps.size()) ? g_gebacken.lightmaps[static_cast<std::size_t>(t.lightmap)] : nullptr;
        kartegpu::gebackenZeichnen(g_gebacken.netz, t.erster, t.anzahl, lm, z);
    }
    return true;
}

}  // namespace bhed::gui::karteneditor
