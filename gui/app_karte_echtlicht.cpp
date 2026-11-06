// app_karte_echtlicht.cpp - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht in der
// Kamera und Schnell-Backen im Hintergrund (eigene Funktion, nicht in
// NetRadiant-Custom; siehe karte_echtlicht.h).
//
// a) Vorschau: die Lichter der Karte wie q3map2 -light (bhed/karte/echtlicht.h),
//    rc610 LICHT1: alle, deren Huelle im Bild liegt; jedes Licht mit seinem
//    Schattenwuerfel, der bleibt, bis sich in seiner Huelle etwas aendert (beim
//    Einschalten alle vorberechnet); die Sonne mit einer Schattenkarte ueber die
//    ganze Karte. Beschriftet als Vorschau - kein Bounce, kein Filter.
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
#include "bhed/schattenatlas.h"
#include "bhed/regelwerk.h"

#include <process.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace bhed::gui {
bool lumpEditKarteSpielbild(std::shared_ptr<const std::string>& daten, std::uint64_t& nummer);   // app_lumpedit.cpp
}  // namespace bhed::gui

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
        if (kartenBildRgba(s.lichtBild, rgba, b, h) && b > 0 && h > 0) {
            double c[3] = {0, 0, 0};
            const std::size_t n = static_cast<std::size_t>(b) * static_cast<std::size_t>(h);
            for (std::size_t i = 0; i < n; ++i) {
                for (int k = 0; k < 3; ++k) { c[k] += rgba[i * 4 + static_cast<std::size_t>(k)]; }
            }
            const Vektor3f f(static_cast<float>(c[0] / n), static_cast<float>(c[1] / n), static_cast<float>(c[2] / n));
            g_lichtBildFarben.emplace(s.lichtBild, f);
            return f;
        }
        // "if no light image, use shader image" (shaders.cpp LoadShaderImages)
    }
    float rgb[3] = {1, 1, 1};
    if (kameraLichtFarbe(sh, rgb)) { return Vektor3f(rgb[0], rgb[1], rgb[2]); }
    return Vektor3f(1, 1, 1);
}

// SkyProbes: die sechs Bilder der skyParms (je Name einmal)
std::map<std::string, std::vector<karte::EchtSonne::Probe>> g_himmelsProben;
const std::vector<karte::EchtSonne::Probe>& himmelsProben(const std::string& basis) {
    const auto it = g_himmelsProben.find(basis);
    if (it != g_himmelsProben.end()) { return it->second; }
    static const char* const endungen[6] = {"_lf", "_rt", "_ft", "_bk", "_up", "_dn"};
    std::vector<unsigned char> bilder[6];
    int b[6] = {0, 0, 0, 0, 0, 0};
    int h[6] = {0, 0, 0, 0, 0, 0};
    const std::vector<unsigned char>* zeiger[6] = {};
    for (int i = 0; i < 6; ++i) {
        if (kartenBildRgba(basis + endungen[i], bilder[i], b[i], h[i])) { zeiger[i] = &bilder[i]; }
    }
    return g_himmelsProben.emplace(basis, karte::echtHimmelsProben(zeiger, b, h)).first->second;
}

struct LichtWelt {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    std::size_t entStand = static_cast<std::size_t>(-1);
    std::vector<EchtLicht> flaechen;   // Flaechenlicht und Sonnen der Himmel (haengen an der Geometrie)
    karte::EchtLichtWelt welt;         // alles zusammen
    karte::Kasten grenzen{Vektor3f(0, 0, 0), Vektor3f(0, 0, 0)};
    std::size_t nummer = 0;            // rc610 LICHT1: je neu gesammelter Lichterliste eins mehr
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

std::size_t g_neuGeladenStand = 0;   // Stufe 9d: ed.neuGeladen beim letzten Leeren der Shader-Zwischenspeicher

void weltAktualisieren(std::size_t geoStand) {
    LichtWelt& w = g_welt;
    if (g_neuGeladenStand != ed().neuGeladen) {   // Stufe 9d: Flush & Reload Shaders - die Skripte neu lesen
        g_neuGeladenStand = ed().neuGeladen;
        g_dateien.clear();
        g_skripte.clear();
        g_lichtBildFarben.clear();
        g_himmelsProben.clear();
        w.geoStand = static_cast<std::size_t>(-1);
        w.entStand = static_cast<std::size_t>(-1);
    }
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
                r.autosprite = s.autosprite;   // Stufe 9d
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
                                        static_cast<float>(so.staerke), static_cast<float>(so.grad), static_cast<float>(so.hoehe),
                                        static_cast<float>(so.abweichung * 3.14159265358979 / 180.0), so.proben});   // Stufe 9d: q3map_sunExt
                }
                for (const karte::ShaderSkript::Himmelslicht& h : s.himmelsLichter) {
                    r.himmel.push_back({static_cast<float>(h.wert), h.schritte, h.horizontMin, h.horizontMax});
                }
                if (!r.himmel.empty()) {
                    r.shaderFarbe = shaderFarbe(sh);
                    if (!s.himmelsBild.empty()) { r.proben = himmelsProben(s.himmelsBild); }
                }
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
        ++w.nummer;
    }
}

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
        float radius = 0;
        for (const Vektor3f& v : l.wicklung) {
            ecken.push_back(v[0]);
            ecken.push_back(v[1]);
            ecken.push_back(v[2]);
            const Vektor3f d = v - l.ursprung;
            radius = std::max(radius, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
        }
        float flaeche = 0;
        for (std::size_t i = 2; i < l.wicklung.size(); ++i) {
            const Vektor3f a = l.wicklung[i - 1] - l.wicklung[0];
            const Vektor3f b = l.wicklung[i] - l.wicklung[0];
            const Vektor3f x(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
            flaeche += 0.5F * std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        }
        g.zusatz[1] = flaeche;
        g.zusatz[2] = radius;
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
// rc610 LICHT1: der Inhalt eines Eimers (Pruefsumme der Ecken, Art, Bild) und seine Grenzen - aendert er sich, gelten
// die Wuerfel nicht mehr, deren Huelle die alten oder neuen Grenzen trifft
using EimerSchluessel = std::tuple<int, int, int, int, std::string>;
struct EimerInhalt {
    std::uint64_t summe = 0;
    float mn[3]{0, 0, 0};
    float mx[3]{0, 0, 0};
};
struct SchattenGeo {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    bool patches = false;
    unsigned geraet = ~0U;
    kartegpu::Puffer3 puffer;
    std::vector<kartegpu::SchattenTeil> teile;
    std::size_t dreiecke = 0;
    std::size_t modellDreiecke = 0;   // Stufe 9d: davon aus misc_model
    std::map<EimerSchluessel, EimerInhalt> inhalt;
    std::vector<std::array<float, 6>> geaendert;   // Grenzen der seit dem letzten Bau geaenderten Eimer (mn, mx)
    bool alleGeaendert = true;                     // erster Bau, neues Geraet: jeder Wuerfel neu
};
SchattenGeo g_schatten;

// FNV-1a ueber 8-Byte-Worte (Ecken der Schattengeometrie)
std::uint64_t pruefsumme(const void* daten, std::size_t bytes, std::uint64_t h) {
    const auto* b = static_cast<const unsigned char*>(daten);
    std::size_t i = 0;
    for (; i + 8 <= bytes; i += 8) {
        std::uint64_t w = 0;
        std::memcpy(&w, b + i, 8);
        h = (h ^ w) * 1099511628211ULL;
    }
    for (; i < bytes; ++i) { h = (h ^ b[i]) * 1099511628211ULL; }
    return h;
}

// rc609 L: ParseEntityCompileParams (map.cpp) - worldspawn und func_group WORLDSPAWN_CAST_SHADOWS (1), andere
// ENTITY_CAST_SHADOWS (0); danach GetEntityShadowFlags (bspfile_abstract.cpp): _castShadows bzw. _cs, wenn
// gesetzt - auch bei worldspawn und func_group (bis rc608 galt dort immer 1)
int entitaetWirft(const karte::Entitaet& e, bool welt) {
    for (const char* k : {"_castShadows", "_cs"}) {
        const std::string w = e.wert(k);
        if (!w.empty()) { return std::atoi(w.c_str()); }
    }
    return welt ? 1 : 0;
}

// games.cpp game_sof2 (game_ja, game_jk2): "default patchMeta subdivisions tolerance" (patchSubdivisions)
constexpr int kPatchUnterteilungen = 8;

void schattenGeoBauen(std::size_t geoStand, bool patches) {
    SchattenGeo& sg = g_schatten;
    sg.teile.clear();
    sg.dreiecke = 0;
    sg.modellDreiecke = 0;
    std::size_t patchDreiecke = 0;
    // Eimer: Art, Zelle (1024er Raster), Shader (nur alphashadow)
    std::map<EimerSchluessel, std::vector<kartegpu::Punkt3>> eimer;
    const auto dreieck = [&eimer](int art, const std::string& sh, const Vektor3f* p, const Vektor3f& n, const karte::Vektor2f* st) {
        const Vektor3f m = (p[0] + p[1] + p[2]) / 3.0F;
        const auto zelle = [](float v) { return static_cast<int>(std::floor(v / 512.0F)); };
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
                // rc609 L: das Netz von -light statt der Editor-Ansicht - PopulateWithBSPModel (light_trace.cpp):
                // TessellatedMesh( ds, info.patchIterations ), patchIterations = IterationsForCurve( longestCurve,
                // patchSubdivisions ) (lightmaps_ydnar.cpp), longestCurve aus ParsePatch; entartete Patches
                // verwirft ParsePatch schon in -bsp
                std::vector<karte::EchtNetzPunkt> netz;
                for (const karte::PatchPunkt& pp : patch.punkte()) { netz.push_back(karte::EchtNetzPunkt{pp.vertex, Vektor3f(0, 0, 0), {pp.texcoord[0], pp.texcoord[1]}}); }
                int breite = static_cast<int>(patch.breite());
                int hoehe = static_cast<int>(patch.hoehe());
                float kurve = 0;
                if (!karte::echtPatchSteuernetz(netz, breite, hoehe, kurve)) { continue; }
                karte::echtNetzTesselieren(netz, breite, hoehe, karte::echtIterationenFuerKurve(kurve, kPatchUnterteilungen));
                // MeshQuadIterator (q3map2.h): je Viereck (q0 q1 q2) und (q0 q2 q3), die Ecken nach (x + y) & 1
                for (int y = 0; y + 1 < hoehe; ++y) {
                    for (int x = 0; x + 1 < breite; ++x) {
                        const int pw[5] = {x + (y * breite), x + ((y + 1) * breite), x + 1 + ((y + 1) * breite), x + 1 + (y * breite), x + (y * breite)};
                        const int r = (x + y) & 1;
                        const auto P = [&netz, &pw, r](int i) -> const karte::EchtNetzPunkt& { return netz[static_cast<std::size_t>(pw[r + i])]; };
                        for (const std::array<int, 3>& d : {std::array<int, 3>{0, 1, 2}, std::array<int, 3>{0, 2, 3}}) {
                            const Vektor3f p[3] = {P(d[0]).ort, P(d[1]).ort, P(d[2]).ort};
                            const karte::Vektor2f st[3] = {karte::Vektor2f(P(d[0]).st[0], P(d[0]).st[1]), karte::Vektor2f(P(d[1]).st[0], P(d[1]).st[1]),
                                                           karte::Vektor2f(P(d[2]).st[0], P(d[2]).st[1])};
                            dreieck(art, patch.shader(), p, P(d[0]).normale, st);
                            ++patchDreiecke;
                        }
                    }
                }
            }
        }
    }
    // Stufe 9d: misc_model werfen Schatten wie in -light - ihre Dreiecke stehen in der .bsp (AddTriangleModels,
    // castShadows in der Welt 1, sonst die Schluessel _castShadows/_cs); ohne C_NODRAW, durchscheinende nur mit
    // alphashadow/lightfilter, Himmel als Himmel. Gruppen (castShadows > 1) werfen auf alles mit
    // recvShadows 1 - wie die Vorgabe.
    karte::echtModellFlaechen(ed().karte, [&dreieck, &sg](const karte::EchtModellFlaeche& f) {
        if (f.wirft <= 0) { return; }
        const karte::ShaderSkript& s = skript(f.shader);
        int art = 0;
        if (s.himmel) {
            art = 2;
        } else if (s.nichtZeichnen || (s.durchscheinend && !s.alphaSchatten && !s.lichtFilter)) {
            return;
        } else if (s.alphaSchatten || s.lichtFilter) {
            art = 3;
        }
        for (std::size_t i = 0; i + 2 < f.ort.size(); i += 3) {
            const Vektor3f p[3] = {f.ort[i], f.ort[i + 1], f.ort[i + 2]};
            const karte::Vektor2f st[3] = {f.st[i], f.st[i + 1], f.st[i + 2]};
            dreieck(art, f.shader, p, f.normale[i], st);
        }
        sg.modellDreiecke += f.ort.size() / 3;
    });
    std::vector<kartegpu::Punkt3> alle;
    std::map<EimerSchluessel, EimerInhalt> inhalt;
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
        EimerInhalt& in = inhalt[schluessel];
        in.summe = pruefsumme(v.data(), v.size() * sizeof(kartegpu::Punkt3), 1469598103934665603ULL);
        const std::uintptr_t bild = reinterpret_cast<std::uintptr_t>(teil.bild);
        in.summe = pruefsumme(&teil.art, sizeof(teil.art), pruefsumme(&bild, sizeof(bild), in.summe));
        std::copy(teil.mn, teil.mn + 3, in.mn);
        std::copy(teil.mx, teil.mx + 3, in.mx);
        alle.insert(alle.end(), v.begin(), v.end());
        sg.teile.push_back(teil);
    }
    // rc610 LICHT1: welche Eimer sich geaendert haben (neu, weg, anderer Inhalt) - alte und neue Grenzen
    sg.geaendert.clear();
    const auto dazu = [&sg](const EimerInhalt& x) { sg.geaendert.push_back({x.mn[0], x.mn[1], x.mn[2], x.mx[0], x.mx[1], x.mx[2]}); };
    for (const auto& [k, x] : inhalt) {
        const auto alt = sg.inhalt.find(k);
        if (alt == sg.inhalt.end() || alt->second.summe != x.summe) {
            dazu(x);
            if (alt != sg.inhalt.end()) { dazu(alt->second); }
        }
    }
    for (const auto& [k, x] : sg.inhalt) {
        if (inhalt.find(k) == inhalt.end()) { dazu(x); }
    }
    sg.alleGeaendert = sg.alleGeaendert || sg.geraet != kartegpu::geraetStand() || sg.geaendert.size() * 2 > inhalt.size() + sg.inhalt.size();
    sg.inhalt = std::move(inhalt);
    sg.dreiecke = alle.size() / 3;
    ed().echtLicht.schattenModellDreiecke = sg.modellDreiecke;
    ed().echtLicht.schattenPatchDreiecke = patchDreiecke;
    if (!alle.empty()) { (void)sg.puffer.setzen(alle.data(), alle.size() * sizeof(kartegpu::Punkt3)); }
    sg.geoStand = geoStand;
    sg.patches = patches;
    sg.geraet = kartegpu::geraetStand();
}

// --- b) Schnell-Backen ------------------------------------------------------------------

struct Ergebnis {
    BspGeometry geo;
    BspMesh netz;
    std::string daten;   // rc608 Teil 11: die .bsp selbst (Spielbild: Entities, Lichtstile, eigenes Netz mit Himmel)
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
    std::shared_ptr<const std::string> daten;   // rc608 Teil 11: fuer das Spielbild
    std::uint64_t nummer = 0;                   // je uebernommenem Ergebnis neu
    void leeren() {
        netz.leeren();
        daten.reset();
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
                                              grund + "-light -fast " + std::string(patchSchattenAn() ? "-patchshadows " : "") + gpu + "\"" + bsp + "\""};
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
                    gpuZeitMerken(kartenName, "-light -fast", gpuWegDesLaufs(bericht, modus), sek);
                }
                std::lock_guard<std::mutex> l(g_backen.mutex);
                g_backen.wahl = gpuWahlText(bericht, zeiten);
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
        r->daten = std::move(bytes);
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
    static std::uint64_t nummern = 0;
    ziel.daten = std::make_shared<const std::string>(std::move(r->daten));
    ziel.nummer = ++nummern;
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

// rc610 R2a: alle Stufen (parseShaderScript wie die Vorschau) je Datei der Spieldaten des Editors einmal zerlegt
const ShaderInfo* echtShaderInfo(const std::string& shader) {
    static std::map<std::string, ShaderMap> dateien;
    static std::size_t neuGeladen = static_cast<std::size_t>(-1);
    static const void* spiel = nullptr;
    if (neuGeladen != ed().neuGeladen || spiel != ed().karteSpiel.get()) {   // Flush & Reload Shaders, anderes Spiel
        neuGeladen = ed().neuGeladen;
        spiel = ed().karteSpiel.get();
        dateien.clear();
    }
    if (!ed().karteSpiel) { return nullptr; }
    const karte::ShaderEintrag* e = ed().karteSpiel->inventar.shaderFinden(shader);
    if (e == nullptr || e->istStandard()) { return nullptr; }
    auto d = dateien.find(e->datei);
    if (d == dateien.end()) {
        std::string t;
        (void)ed().karteSpiel->fs.textLesen(e->datei, t);
        ShaderMap m;
        parseShaderScript(t, m);
        d = dateien.emplace(e->datei, std::move(m)).first;
    }
    std::string k = shader;
    for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    const auto it = d->second.find(k);
    return it == d->second.end() ? nullptr : &it->second;
}

SpielStufe spielStufe(const ShaderInfo::Stage& st) {
    SpielStufe s;
    // GL_-Faktoren wie ShaderSkript::mischQuelle (1 ONE ... 10 ONE_MINUS_DST_ALPHA); GL_SRC_ALPHA_SATURATE wie der Leser als SRC_ALPHA
    const auto code = [](BlendFactor f) {
        switch (f) {
            case BlendFactor::One: return 1;
            case BlendFactor::Zero: return 2;
            case BlendFactor::SrcAlpha:
            case BlendFactor::SrcAlphaSaturate: return 3;
            case BlendFactor::OneMinusSrcAlpha: return 4;
            case BlendFactor::DstColor: return 5;
            case BlendFactor::OneMinusDstColor: return 6;
            case BlendFactor::SrcColor: return 7;
            case BlendFactor::OneMinusSrcColor: return 8;
            case BlendFactor::DstAlpha: return 9;
            case BlendFactor::OneMinusDstAlpha: return 10;
        }
        return 1;
    };
    const bool deckend = st.srcFactor == BlendFactor::One && st.dstFactor == BlendFactor::Zero;
    if (!deckend) {
        s.mischQuelle = code(st.srcFactor);
        s.mischZiel = code(st.dstFactor);
    }
    // tr_shader.cpp NameToAFunc: GT0 (> 0), LT128 (< 0.5), GE128 (>= 0.5), GE192 (>= 0.75)
    switch (st.alphaTest) {
        case AlphaTest::Gt0: s.alphaFunktion = 3; s.alphaRef = 0.0F; break;
        case AlphaTest::Lt128: s.alphaFunktion = 2; s.alphaRef = 0.5F; break;
        case AlphaTest::Ge128: s.alphaFunktion = 5; s.alphaRef = 0.5F; break;
        case AlphaTest::Ge192: s.alphaFunktion = 5; s.alphaRef = 0.75F; break;
        case AlphaTest::None: break;
    }
    // ParseStage: "Wer blendFunc schreibt, schreibt keine Tiefe" (ausser depthWrite)
    s.tiefeSchreiben = deckend || st.depthWrite;
    for (int c = 0; c < 3; ++c) {
        if (st.rgbConst[c] >= 0.0F) { s.farbe[c] = st.rgbConst[c]; }
    }
    if (st.alphaConst >= 0.0F) { s.farbe[3] = st.alphaConst; }
    if (st.alphaGen == AlphaGen::Portal) { s.portalWeite = st.portalWeite; }
    std::string bild = st.image;
    const std::size_t punkt = bild.find_last_of('.');
    if (punkt != std::string::npos && punkt + 5 >= bild.size() && bild.find('/', punkt) == std::string::npos) { bild.resize(punkt); }
    if (!bild.empty()) { s.bild = "?" + bild; }
    return s;
}

SpielDarstellung spielDarstellung(const std::string& shader) {
    SpielDarstellung d;
    const karte::ShaderSkript& s = skript(shader);
    d.unsichtbar = s.nichtZeichnen;   // C_NODRAW: q3map2 laesst die Flaeche weg
    d.eckLicht = s.eckLicht;          // rc608 Teil 11: C_VERTEXLIT
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
    // das Spiel nimmt das Bild der Stufe (mit seinem Alpha), nicht qer_editorimage (oft ein .jpg)
    if ((d.alphaFunktion != 0 || d.mischQuelle != 0) && !s.stufeBild.empty()) { d.bild = "?" + s.stufeBild; }
    // Stufe 9d: fogParms ohne Stufe - das Spiel zeichnet nur den Nebel darueber (RB_FogPass, FP_LE)
    d.nebelFlaeche = s.nebelParms && !s.stufeGefunden;
    d.himmel = s.himmel;
    // rc609 P: SS_PORTAL; ohne jede Stufe setzt FinishShader (tr_shader.cpp) sort auf SS_FOG - dann ist es kein Portal
    d.portal = s.portal && s.stufeGefunden;
    d.portalWeite = static_cast<float>(s.portalWeite);
    // rc610 R2a: eine Portalflaeche zeichnet das Spiel mit RB_StageIteratorGeneric - alle Stufen ueber der Spiegel- bzw.
    // Portalsicht (Spiegel-Shader des Testers: die zweite Stufe ist der Boden darueber)
    if (d.portal) {
        if (const ShaderInfo* si = echtShaderInfo(shader)) {
            for (std::size_t i = 1; i < si->stages.size(); ++i) {
                if (si->stages[i].ss.aktiv() || si->stages[i].image.empty()) { continue; }   // surfaceSprites: keine Schicht
                d.weitere.push_back(spielStufe(si->stages[i]));
            }
        }
    }
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

// rc610 LICHT1: die Kacheln einer Kugel enger als der Kasten um sie (kugelImBild) - je Kachelspalte und -zeile die beiden
// Ebenen durch das Auge (clip.x - a * clip.w = 0 bzw. clip.y - b * clip.w = 0); eine Kugel trifft die Spalte, wenn sie
// beide Halbraeume beruehrt. Konservativ: was sichtbar ist, liegt in beiden; das Bild bleibt gleich, die Bildpunkte
// pruefen weniger Lichter.
struct KachelEbenen {
    std::vector<float> a;    // Spaltengrenzen in NDC (links nach rechts)
    std::vector<float> an;   // |Normale| der Ebene clip.x - a * clip.w
    std::vector<float> b;    // Zeilengrenzen in NDC (oben nach unten)
    std::vector<float> bn;
};

void kachelEbenenSetzen(const float* vp, int breite, int hoehe, int kachelnX, int kachelnY, KachelEbenen& k) {
    const auto laenge = [](float x, float y, float z) { return std::sqrt(x * x + y * y + z * z); };
    k.a.resize(static_cast<std::size_t>(kachelnX) + 1);
    k.an.resize(k.a.size());
    for (std::size_t i = 0; i < k.a.size(); ++i) {
        const float a = std::min(1.0F, static_cast<float>(static_cast<int>(i) * kartegpu::kKachel) / static_cast<float>(breite)) * 2.0F - 1.0F;
        k.a[i] = a;
        k.an[i] = laenge(vp[0] - a * vp[3], vp[4] - a * vp[7], vp[8] - a * vp[11]);
    }
    k.b.resize(static_cast<std::size_t>(kachelnY) + 1);
    k.bn.resize(k.b.size());
    for (std::size_t i = 0; i < k.b.size(); ++i) {
        const float b = 1.0F - std::min(1.0F, static_cast<float>(static_cast<int>(i) * kartegpu::kKachel) / static_cast<float>(hoehe)) * 2.0F;
        k.b[i] = b;
        k.bn[i] = laenge(vp[1] - b * vp[3], vp[5] - b * vp[7], vp[9] - b * vp[11]);
    }
}

// Engt das Rechteck [x0, x1] x [y0, y1] ein; false: keine Kachel
bool kugelKacheln(const float* vp, const KachelEbenen& k, const Vektor3f& c, float r, int& x0, int& y0, int& x1, int& y1) {
    const float lx = vp[0] * c[0] + vp[4] * c[1] + vp[8] * c[2] + vp[12];
    const float ly = vp[1] * c[0] + vp[5] * c[1] + vp[9] * c[2] + vp[13];
    const float lw = vp[3] * c[0] + vp[7] * c[1] + vp[11] * c[2] + vp[15];
    int von = x1 + 1;
    int bis = x0 - 1;
    for (int j = x0; j <= x1; ++j) {
        const auto u = static_cast<std::size_t>(j);
        if (lx - k.a[u] * lw >= -r * k.an[u] && k.a[u + 1] * lw - lx >= -r * k.an[u + 1]) {
            von = std::min(von, j);
            bis = j;
        }
    }
    if (von > bis) { return false; }
    x0 = von;
    x1 = bis;
    von = y1 + 1;
    bis = y0 - 1;
    for (int j = y0; j <= y1; ++j) {
        const auto u = static_cast<std::size_t>(j);
        if (k.b[u] * lw - ly >= -r * k.bn[u] && ly - k.b[u + 1] * lw >= -r * k.bn[u + 1]) {
            von = std::min(von, j);
            bis = j;
        }
    }
    if (von > bis) { return false; }
    y0 = von;
    y1 = bis;
    return true;
}

// --- rc610 LICHT1: ein Schattenwuerfel je Licht, behalten --------------------------------
//
// Meldung (Nutzer, 05.11.): am Anfang eines Hangars fehlte das Licht am Ende. Bis rc609 bekamen je Bild nur die 128
// staerksten Lichter (nach dem Beitrag am Auge) einen Wuerfel (32 zu 512, 96 zu 256, hoechstens 8 neu je Bild, Plaetze
// nach Rang neu vergeben), die weiteren leuchteten ohne Schatten, und mehr als 512 Lichter je Bild fielen weg.
// Jetzt: jedes Licht (ausser den Sonnen) hat seinen Wuerfel im Atlas (bhed/schattenatlas.h), Schluessel Ort und
// Weite (gleiche Lichter teilen sich einen). Neu gerechnet wird er nur, wenn sich in seiner Huelle ein Eimer der
// Schattengeometrie aendert, wenn das Licht wandert oder seine Weite sich aendert, oder bei einem neuen Grafikgeraet.
// Das erste Bild nach dem Einschalten rechnet alle (bis kErstesBildMs), danach jedes Bild bis kBildMs, die im Bild
// zuerst; wer noch wartet, fehlt (sonst leuchtete es kurz durch die Waende), die Ecke zeigt den Fortschritt. Passt nicht
// alles ins Budget, bekommen die kleinsten Huellen kleinere bzw. keine Wuerfel - die Statuszeile sagt es.
constexpr double kErstesBildMs = 1500.0;
constexpr double kBildMs = 8.0;

struct Wuerfel {
    AtlasPlatz platz;
    Vektor3f pos{0, 0, 0};
    float weite = 0;
    bool fertig = false;
};
std::unordered_map<std::uint64_t, Wuerfel> g_wuerfel;
SchattenAtlas g_atlas;
std::vector<Wuerfel*> g_lichtWuerfel;   // je Licht von g_welt.welt.lichter sein Wuerfel (nullptr: Sonne); Elemente einer
                                        // unordered_map behalten ihre Adresse, bis sie entfernt werden (nur in wuerfelVerteilen)
std::size_t g_wuerfelStand = static_cast<std::size_t>(-1);   // g_welt.nummer der Verteilung
unsigned g_wuerfelGeraet = ~0U;
int g_wuerfelFest = 0;      // Selbsttest: alle Wuerfel dieser Groesse (0: nach der Huelle)
int g_wuerfelFestStand = 0;
std::size_t g_ohnePlatz = 0;
std::string g_meldung;      // die zuletzt in die Statuszeile geschriebene Meldung
bool g_zaehlen = true;      // Wuerfel neu zaehlen (nach Verteilen, Rechnen, Pruefen) - nicht jedes Bild
std::chrono::steady_clock::time_point g_letzterAufruf{};

float wuerfelWeite(const EchtLicht& l) { return std::min(l.huelle, 65536.0F); }

std::uint64_t wuerfelSchluessel(const EchtLicht& l) {
    const float w[4] = {l.ursprung[0], l.ursprung[1], l.ursprung[2], wuerfelWeite(l)};
    return pruefsumme(w, sizeof(w), 1469598103934665603ULL) | 1U;   // nie 0
}

// ein Viertel des Budgets des Grafikspeichers, 256 MiB bis 2 GiB (ohne Angabe 1 GiB)
std::size_t atlasBudget() {
    constexpr std::size_t kMiB = 1024U * 1024U;
    const std::size_t b = kartegpu::grafikBudget();
    return b == 0 ? 1024U * kMiB : std::clamp(b / 4U, 256U * kMiB, 2048U * kMiB);
}

// Wuerfel und Lichter zuordnen (neue Lichter, Groessen im Budget); bei neuem Lichterstand, Geraet oder Selbsttest-Groesse
void wuerfelVerteilen(const karte::EchtLichtWelt& w) {
    const bool geraetNeu = g_wuerfelGeraet != kartegpu::geraetStand();
    if (!geraetNeu && g_wuerfelStand == g_welt.nummer && g_wuerfelFestStand == g_wuerfelFest) { return; }
    if (geraetNeu) {
        g_wuerfel.clear();
        g_atlas.leeren();
        g_atlas.budgetSetzen(atlasBudget());
        g_wuerfelGeraet = kartegpu::geraetStand();
    }
    g_wuerfelStand = g_welt.nummer;
    g_wuerfelFestStand = g_wuerfelFest;
    std::vector<std::uint64_t> schluessel;
    std::vector<float> weiten;
    std::vector<Vektor3f> orte;
    std::unordered_set<std::uint64_t> schon;
    std::vector<std::uint64_t> jeLicht(w.lichter.size(), 0);
    for (std::size_t i = 0; i < w.lichter.size(); ++i) {
        const EchtLicht& l = w.lichter[i];
        if (l.art == EchtLicht::Art::Sonne) { continue; }
        const std::uint64_t k = wuerfelSchluessel(l);
        jeLicht[i] = k;
        if (!schon.insert(k).second) { continue; }
        schluessel.push_back(k);
        weiten.push_back(wuerfelWeite(l));
        orte.push_back(l.ursprung);
    }
    const std::vector<int> groessen = g_wuerfelFest > 0 ? std::vector<int>(weiten.size(), g_wuerfelFest) : wuerfelGroessenVerteilen(weiten, g_atlas.budget());
    for (auto it = g_wuerfel.begin(); it != g_wuerfel.end();) {   // nicht mehr gebraucht
        if (schon.count(it->first) == 0) {
            g_atlas.freigeben(it->second.platz);
            it = g_wuerfel.erase(it);
        } else {
            ++it;
        }
    }
    std::vector<std::size_t> reihe(schluessel.size());   // die grossen zuerst
    for (std::size_t i = 0; i < reihe.size(); ++i) { reihe[i] = i; }
    std::stable_sort(reihe.begin(), reihe.end(), [&groessen](std::size_t a, std::size_t b) { return groessen[a] > groessen[b]; });
    const auto belegen = [&](bool alleNeu) {
        bool gut = true;
        for (const std::size_t i : reihe) {
            Wuerfel& x = g_wuerfel[schluessel[i]];
            x.pos = orte[i];
            x.weite = weiten[i];
            if (alleNeu || x.platz.groesse != groessen[i]) {
                g_atlas.freigeben(x.platz);
                x.platz = AtlasPlatz{};
                x.fertig = false;
            }
            if (groessen[i] > 0 && !x.platz.gueltig()) {
                x.platz = g_atlas.belegen(groessen[i]);
                x.fertig = false;
                gut = gut && x.platz.gueltig();
            }
        }
        return gut;
    };
    // Seiten zersplittert (kein Platz) oder halb leer (andere Karte, viele Lichter weg): alles neu verteilen - so belegt
    // der Atlas nur, was die Lichter brauchen
    if ((!belegen(false) && g_wuerfelFest == 0) || g_atlas.belegteSeiten() * 2 < g_atlas.seiten()) {
        g_atlas.leeren();
        for (auto& [k, x] : g_wuerfel) { x.platz = AtlasPlatz{}; }
        (void)belegen(true);
    }
    g_atlas.kuerzen();
    if (!kartegpu::schattenAtlasSeiten(g_atlas.seiten())) {   // kein Grafikspeicher: ohne Wuerfel
        g_atlas.leeren();
        for (auto& [k, x] : g_wuerfel) {
            x.platz = AtlasPlatz{};
            x.fertig = false;
        }
    }
    g_ohnePlatz = 0;
    for (const auto& [k, x] : g_wuerfel) { g_ohnePlatz += x.platz.gueltig() ? 0U : 1U; }
    g_zaehlen = true;
    g_lichtWuerfel.assign(w.lichter.size(), nullptr);
    for (std::size_t i = 0; i < jeLicht.size(); ++i) {
        if (jeLicht[i] != 0) { g_lichtWuerfel[i] = &g_wuerfel[jeLicht[i]]; }
    }
}

// Nach einem Neubau der Schattengeometrie: die Wuerfel, deren Huelle einen geaenderten Eimer trifft
void wuerfelPruefen() {
    SchattenGeo& sg = g_schatten;
    for (auto& [k, x] : g_wuerfel) {
        if (!x.fertig) { continue; }
        bool trifft = sg.alleGeaendert;
        for (std::size_t j = 0; j < sg.geaendert.size() && !trifft; ++j) {
            const std::array<float, 6>& b = sg.geaendert[j];
            float d2 = 0;
            for (int i = 0; i < 3; ++i) {
                const float v = x.pos[i] < b[static_cast<std::size_t>(i)] ? b[static_cast<std::size_t>(i)] - x.pos[i]
                                : x.pos[i] > b[static_cast<std::size_t>(i) + 3] ? x.pos[i] - b[static_cast<std::size_t>(i) + 3] : 0.0F;
                d2 += v * v;
            }
            trifft = d2 <= x.weite * x.weite;
        }
        if (trifft) {
            x.fertig = false;
            g_zaehlen = true;
        }
    }
    sg.alleGeaendert = false;
    sg.geaendert.clear();
}

}  // namespace

// --- Stufe 9d: Nebel (RB_FogPass) ---------------------------------------------------------
struct NebelWelt {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    std::size_t entStand = static_cast<std::size_t>(-1);
    std::vector<karte::EchtNebel> nebel;
};
NebelWelt g_nebel;

void nebelBildSetzen(const Vektor3f& auge, const float* vp, std::size_t geoStand) {
    NebelWelt& w = g_nebel;
    static std::size_t nebelNeuGeladen = 0;   // nach Flush & Reload Shaders (fogParms geaendert)
    if (nebelNeuGeladen != ed().neuGeladen) {
        nebelNeuGeladen = ed().neuGeladen;
        w.geoStand = static_cast<std::size_t>(-1);
    }
    const std::string globalAlt = w.nebel.empty() || !w.nebel.back().global ? std::string() : w.nebel.back().shader;
    const auto welt = ed().karte.welt();
    std::string globalNeu = welt ? welt->wert("_fog") : std::string();
    if (globalNeu.empty() && welt) { globalNeu = welt->wert("fog"); }
    if (w.geoStand != geoStand || globalAlt != globalNeu) {
        w.geoStand = geoStand;
        w.nebel = karte::echtNebelSammeln(ed().karte, [](const std::string& sh) {
            const karte::ShaderSkript& s = skript(sh);
            karte::EchtNebelShader r;
            r.nebel = s.nebelParms;
            r.farbe = Vektor3f(static_cast<float>(s.nebelFarbe[0]), static_cast<float>(s.nebelFarbe[1]), static_cast<float>(s.nebelFarbe[2]));
            r.tiefe = static_cast<float>(s.nebelTiefe);
            r.richtung = Vektor3f(static_cast<float>(s.nebelRichtung[0]), static_cast<float>(s.nebelRichtung[1]), static_cast<float>(s.nebelRichtung[2]));
            r.nichtZeichnen = s.nichtZeichnen;
            r.deckend = !s.durchscheinend && !s.himmel;
            return r;
        });
        ed().echtLicht.nebel = w.nebel.size();
    }
    std::vector<kartegpu::NebelEcht> gpu;
    std::vector<float> ebenen;
    int global = -1;
    for (const karte::EchtNebel& n : w.nebel) {
        if (gpu.size() >= static_cast<std::size_t>(kartegpu::kNebelMax)) { break; }
        kartegpu::NebelEcht x{};
        x.farbe[0] = n.farbe[0];
        x.farbe[1] = n.farbe[1];
        x.farbe[2] = n.farbe[2];
        x.farbe[3] = n.tcScale;
        std::memcpy(x.flaeche, n.ebene, sizeof(x.flaeche));
        x.info[0] = static_cast<float>(ebenen.size() / 4);
        x.info[1] = n.global ? 0.0F : static_cast<float>(n.ebenen.size() / 4);
        x.info[2] = n.hatFlaeche ? 1.0F : 0.0F;
        x.info[3] = n.hatFlaeche ? auge[0] * n.ebene[0] + auge[1] * n.ebene[1] + auge[2] * n.ebene[2] + n.ebene[3] : 1.0F;   // eye_t
        if (n.global && global < 0) { global = static_cast<int>(gpu.size()); }
        ebenen.insert(ebenen.end(), n.ebenen.begin(), n.ebenen.end());
        gpu.push_back(x);
    }
    // Blickrichtung: die w-Zeile der Kameramatrix (Abstand vor dem Auge)
    float vorn[4] = {vp[3], vp[7], vp[11], 0};
    const float l = std::sqrt(vorn[0] * vorn[0] + vorn[1] * vorn[1] + vorn[2] * vorn[2]);
    if (l > 0) {
        for (int i = 0; i < 3; ++i) { vorn[i] /= l; }
    }
    vorn[3] = vorn[0] * auge[0] + vorn[1] * auge[1] + vorn[2] * auge[2];
    kartegpu::nebelSetzen(gpu, ebenen, global, vorn);
}

void echtLichtBildVorbereiten(const Vektor3f& auge, const float* vp, int breite, int hoehe, std::size_t geoStand) {
    Editor& e = ed();
    const auto t0 = std::chrono::steady_clock::now();
    ++g_bild;
    weltAktualisieren(geoStand);
    static std::size_t schattenNeuGeladen = 0;   // Stufe 9d: nach Flush & Reload Shaders auch die Schattengeometrie (Shader-Regeln)
    const bool patches = patchSchattenAn();   // rc609 L: die Einstellung der Karte
    if (g_schatten.geoStand != geoStand || g_schatten.patches != patches || g_schatten.geraet != kartegpu::geraetStand() ||
        schattenNeuGeladen != e.neuGeladen) {
        schattenNeuGeladen = e.neuGeladen;
        schattenGeoBauen(geoStand, patches);
        wuerfelPruefen();
        for (SonnenKarte& k : g_kaskade) { k.gueltig = false; }
        for (SonnenKarte& k : g_himmelKarten) { k.gueltig = false; }
    }
    const karte::EchtLichtWelt& w = g_welt.welt;
    wuerfelVerteilen(w);
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
    KachelEbenen ebenen;
    kachelEbenenSetzen(vp, breite, hoehe, kachelnX, kachelnY, ebenen);
    for (std::size_t i = 0; i < w.lichter.size(); ++i) {
        const EchtLicht& l = w.lichter[i];
        if (l.art == EchtLicht::Art::Sonne) {
            sonnen.push_back(i);
            continue;
        }
        Wahl x;
        x.i = i;
        if (!kugelImBild(vp, breite, hoehe, l.ursprung, std::min(l.huelle, 65536.0F), kachelnX, kachelnY, x.x0, x.y0, x.x1, x.y1) ||
            !kugelKacheln(vp, ebenen, l.ursprung, std::min(l.huelle, 65536.0F), x.x0, x.y0, x.x1, x.y1)) {
            continue;
        }
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
        std::vector<std::size_t> proben;   // Stufe 9d: die anderen Proben der Hauptsonne (q3map_sunExt, _samples)
        for (const std::size_t i : sonnen) {
            if (i == haupt) { continue; }
            if (haupt != static_cast<std::size_t>(-1) && w.lichter[i].sonnenGruppe != 0 && w.lichter[i].sonnenGruppe == w.lichter[haupt].sonnenGruppe) {
                proben.push_back(i);
            } else {
                andere.push_back(i);
            }
        }
        if (haupt != static_cast<std::size_t>(-1)) {
            Wahl x;
            x.i = haupt;
            x.platz = -3;
            x.x1 = kachelnX - 1;
            x.y1 = kachelnY - 1;
            sw.push_back(x);
            // die Proben teilen die Kaskaden der Hauptsonne (Schatten aus ihrer Richtung, Licht aus der eigenen) -
            // eine eigene Karte ueber die ganze Karte waere grob und liesse Licht durch
            for (const std::size_t i : proben) {
                Wahl y = x;
                y.i = i;
                y.platz = -4;
                sw.push_back(y);
            }
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
    // 4. Schattenwuerfel: erst die der Lichter im Bild (nach Rang), dann die uebrigen - bis das Zeitbudget des Bilds
    // erschoepft ist (das erste Bild nach dem Einschalten wartet laenger)
    {
        const auto jetzt = std::chrono::steady_clock::now();
        const double budgetMs = std::chrono::duration<double>(jetzt - g_letzterAufruf).count() > 0.25 ? kErstesBildMs : kBildMs;
        g_letzterAufruf = jetzt;
        bool weiter = true;
        const auto rechnen = [&](Wuerfel* z) {
            if (!weiter || z == nullptr || z->fertig || !z->platz.gueltig()) { return; }
            if (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - jetzt).count() > budgetMs) {
                weiter = false;
                return;
            }
            Wuerfel& x = *z;
            const float pos[3] = {x.pos[0], x.pos[1], x.pos[2]};
            x.fertig = true;
            if (!kartegpu::schattenWuerfel(x.platz.seite, x.platz.wuerfel, x.platz.groesse, pos, x.weite, g_schatten.teile)) {
                g_atlas.freigeben(x.platz);   // geht nicht: ohne Schatten statt jedes Bild wieder
                x.platz = AtlasPlatz{};
                ++g_ohnePlatz;
            }
            ++e.echtLicht.schattenNeu;
            g_zaehlen = true;
        };
        for (const Wahl& x : wahl) { rechnen(g_lichtWuerfel[x.i]); }
        for (Wuerfel* z : g_lichtWuerfel) { rechnen(z); }
    }
    if (g_zaehlen) {
        g_zaehlen = false;
        std::size_t ausstehend = 0;
        std::size_t mitPlatz = 0;
        for (const auto& [k, x] : g_wuerfel) {
            mitPlatz += x.platz.gueltig() ? 1U : 0U;
            ausstehend += x.platz.gueltig() && !x.fertig ? 1U : 0U;
        }
        e.echtLicht.wuerfel = mitPlatz;
        e.echtLicht.wuerfelAusstehend = ausstehend;
    }
    e.echtLicht.wuerfelOhnePlatz = g_ohnePlatz;
    e.echtLicht.wuerfelBytes = kartegpu::schattenAtlasBytes();
    // Grenze des Budgets: in die Statuszeile (statt still fehlender Schatten)
    if (g_ohnePlatz > 0) {
        char z[300];
        std::snprintf(z, sizeof(z), trK(Str::KeLichtWuerfelGrenze), static_cast<int>(g_ohnePlatz), static_cast<int>(g_wuerfel.size()),
                      static_cast<int>(g_atlas.budget() / (1024U * 1024U)));
        if (g_meldung != z) {
            g_meldung = z;
            e.statusBefehl = z;
        }
    } else if (!g_meldung.empty()) {
        if (e.statusBefehl == g_meldung) { e.statusBefehl.clear(); }
        g_meldung.clear();
    }
    // 5. Lichtliste: die Sonnen vorn, dann die Lichter nach der Tiefe ihres Mittelpunkts; wer auf seinen Wuerfel wartet,
    // fehlt in diesem Bild (sonst leuchtete es kurz durch die Waende), wer keinen Platz hat, kommt ohne Schatten
    std::vector<kartegpu::LichtGpu> gpu;
    std::vector<float> ecken;
    std::vector<const Wahl*> liste;
    gpu.reserve(sw.size() + wahl.size());
    for (const Wahl& x : sw) {
        const EchtLicht& l = w.lichter[x.i];
        kartegpu::LichtGpu g = gpuLicht(l, ecken);
        g.pos[3] *= x.faktor;
        const bool karte = x.platz <= -3 ? g_kaskade[0].gueltig : g_himmelKarten[static_cast<std::size_t>(x.platz)].gueltig;
        if (!karte) { continue; }   // die Karte kommt in einem der naechsten Bilder
        g.mehr[0] = static_cast<float>(std::max(x.platz, -3));
        gpu.push_back(g);
        liste.push_back(&x);
    }
    const int sonnenZahl = static_cast<int>(gpu.size());
    // Tiefe = w der Kamera (wie SV_Position.w im Shader); eine Huelle reicht |w-Zeile| * r weit
    const float wZeile = std::sqrt(vp[3] * vp[3] + vp[7] * vp[7] + vp[11] * vp[11]);
    struct Tief {
        const Wahl* x = nullptr;
        float tiefe = 0;
    };
    std::vector<Tief> tief;
    tief.reserve(wahl.size());
    e.echtLicht.fehlend = 0;
    e.echtLicht.ohneSchatten = 0;
    for (const Wahl& x : wahl) {
        const Wuerfel* z = g_lichtWuerfel[x.i];
        if (z != nullptr && z->platz.gueltig() && !z->fertig) {
            ++e.echtLicht.fehlend;
            continue;
        }
        const Vektor3f& o = w.lichter[x.i].ursprung;
        tief.push_back({&x, vp[3] * o[0] + vp[7] * o[1] + vp[11] * o[2] + vp[15]});
    }
    std::stable_sort(tief.begin(), tief.end(), [](const Tief& a, const Tief& b) { return a.tiefe < b.tiefe; });
    std::vector<std::uint32_t> zbins(static_cast<std::size_t>(kartegpu::kZbins) * 2U, 0U);
    std::vector<bool> zbinBelegt(static_cast<std::size_t>(kartegpu::kZbins), false);
    for (const Tief& t : tief) {
        const EchtLicht& l = w.lichter[t.x->i];
        kartegpu::LichtGpu g = gpuLicht(l, ecken);
        const Wuerfel* z = g_lichtWuerfel[t.x->i];
        if (z != nullptr && z->platz.gueltig()) {
            // die Ecke des Wuerfels in seiner Seite (Vielfache von 64): je Zeile 512 / Groesse Wuerfel zu sechs Seiten
            const AtlasPlatz& p = z->platz;
            const int jeZeile = 512 / p.groesse;
            const int ex = (p.wuerfel % jeZeile) * 6 * p.groesse;
            const int ey = (p.wuerfel / jeZeile) * p.groesse;
            g.mehr[0] = static_cast<float>((p.seite * 64 + ex / 64) * 64 + ey / 64);
            g.mehr[1] = z->weite;
            g.zusatz[3] = static_cast<float>(p.groesse);
        } else {
            ++e.echtLicht.ohneSchatten;
        }
        const auto j = static_cast<std::uint32_t>(gpu.size());
        gpu.push_back(g);
        liste.push_back(t.x);
        // die Tiefenstufen, die die Huelle trifft (eine Stufe Rand je Seite gegen Rundung)
        const float r = wuerfelWeite(l) * wZeile;
        const int b0 = std::max(0, kartegpu::zbinFuer(t.tiefe - r) - 1);
        const int b1 = std::min(kartegpu::kZbins - 1, kartegpu::zbinFuer(t.tiefe + r) + 1);
        for (int b = b0; b <= b1; ++b) {
            const auto bb = static_cast<std::size_t>(b);
            if (!zbinBelegt[bb]) {
                zbinBelegt[bb] = true;
                zbins[bb * 2] = j;
            }
            zbins[bb * 2 + 1] = j + 1;
        }
    }
    // 6. Kachelmasken (die Sonnen gelten ueberall und stehen nicht darin)
    const int worte = std::max(1, static_cast<int>((gpu.size() + 31) / 32));
    std::vector<std::uint32_t> kacheln(static_cast<std::size_t>(kachelnX) * static_cast<std::size_t>(kachelnY) * static_cast<std::size_t>(worte), 0U);
    for (std::size_t j = static_cast<std::size_t>(sonnenZahl); j < liste.size(); ++j) {
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
    const float minimumEcken[3] = {w.minimumEcken[0], w.minimumEcken[1], w.minimumEcken[2]};
    kartegpu::lichtSetzen(gpu, sonnenZahl, ecken, kacheln, kachelnX, kachelnY, worte, zbins, umgebung, minimum, minimumEcken);
    nebelBildSetzen(auge, vp, geoStand);   // Stufe 9d
    e.echtLicht.gezeichnet = gpu.size();
    e.echtLicht.lichtMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

std::string echtLichtHinweis() {
    const Editor& e = ed();
    char z[256];
    if (e.echtLicht.spielbildGezeigt) {   // rc608 Teil 11
        std::snprintf(z, sizeof(z), trK(Str::KeLichtSpielbild), e.echtLicht.gebackenZeigen ? e.echtLicht.bauSekunden : 0.0);
        return e.echtLicht.gebackenZeigen ? std::string(z) : std::string(trK(Str::KeLichtSpielbildVorlage));
    }
    if (e.echtLicht.gebackenZeigen) {
        std::snprintf(z, sizeof(z), trK(Str::KeLichtGebacken), e.echtLicht.bauSekunden);
        return z;
    }
    std::string s = trK(Str::KeLichtVorschau);
    if (e.echtLicht.wuerfelAusstehend > 0) {   // rc610 LICHT1: die Schattenwuerfel werden noch gerechnet
        std::snprintf(z, sizeof(z), trK(Str::KeLichtWuerfelFortschritt), static_cast<int>(e.echtLicht.wuerfel - e.echtLicht.wuerfelAusstehend),
                      static_cast<int>(e.echtLicht.wuerfel));
        s += std::string(" - ") + z;
    }
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
        neuZeichnen();   // rc610 R1: das gebackene Bild zeigen
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

void echtLichtWuerfelFest(int groesse) { g_wuerfelFest = groesse; }

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
    r->daten = bytes;
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

// rc608 Teil 11: Die gebaute Karte im Spielbild - Schnell-Backen, sonst die Vorlage.
namespace {
const Gebacken* spielQuelle() {
    if (g_vorlageWartet) { netzUebernehmen(g_vorlage, std::move(g_vorlageWartet)); }
    if (ed().echtLicht.gebackenZeigen && g_gebacken.netz.gueltig() && g_gebacken.daten) { return &g_gebacken; }
    if (vorlageAktiv() && g_vorlage.netz.gueltig() && g_vorlage.daten) { return &g_vorlage; }
    return nullptr;
}
}  // namespace

namespace {
bool g_lumpSpielbild = false;       // rc609 J: die bearbeitete fertige Karte zeigen
bool g_lumpSpielbildGezeigt = false;
}  // namespace

bool lumpSpielbildAn() { return g_lumpSpielbild && ed().echtLicht.spielbild; }
void lumpSpielbildSetzen(bool an) {
    g_lumpSpielbild = an;
    if (an) { ed().echtLicht.spielbild = true; }
    ++ed().sichtStand;
}
bool lumpSpielbildGezeigt() { return g_lumpSpielbildGezeigt; }

void* spielbildFuerKamera(const karte::Kamera& k, int breite, int hoehe) {
    g_lumpSpielbildGezeigt = false;
    if (!ed().echtLicht.spielbild) { return nullptr; }
    // rc609 J: die geoeffnete .bsp mit den bearbeiteten Shadern, Nebeln und Flaechen - durch denselben
    // Zeichner; die Pinsel treten zurueck wie beim Schnell-Backen.
    if (g_lumpSpielbild) {
        std::shared_ptr<const std::string> daten;
        std::uint64_t nummer = 0;
        if (lumpEditKarteSpielbild(daten, nummer)) {
            void* bild = spielbildZeichnen(daten, nummer, ed().bspHerkunft.quelle, k, breite, hoehe);
            g_lumpSpielbildGezeigt = bild != nullptr;
            return bild;
        }
    }
    const Gebacken* q = spielQuelle();
    if (q == nullptr) { return nullptr; }
    return spielbildZeichnen(q->daten, q->nummer, q == &g_vorlage ? g_vorlagePfad : ed().letzteKarte, k, breite, hoehe);
}

void spielbildTiefe(const float* matrix) {
    const Gebacken* q = spielQuelle();
    if (q == nullptr) { return; }
    for (const Gebacken::Teil& t : q->teile) {
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.wegschneiden = true;
        z.mischen = kartegpu::Mischen::NurTiefe;
        kartegpu::gebackenZeichnen(q->netz, t.erster, t.anzahl, nullptr, z);
    }
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
