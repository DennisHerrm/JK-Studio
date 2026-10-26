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
                    float rgb[3] = {1, 1, 1};
                    if (kameraTexturFarbe(sh, rgb)) { r.farbe = Vektor3f(rgb[0], rgb[1], rgb[2]); }
                }
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
                return r;
            });
        w.flaechen.clear();
        for (const EchtLicht& l : f.lichter) {
            if (l.art == EchtLicht::Art::Flaeche || (l.art == EchtLicht::Art::Sonne && l.quelle == nullptr)) { w.flaechen.push_back(l); }
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
struct SonnenStand {
    bool gueltig = false;
    Vektor3f richtung{0, 0, 0};
    std::size_t geoStand = 0;
};
SonnenStand g_sonne;

bool gleicherOrt(const Vektor3f& a, const Vektor3f& b) {
    return std::fabs(a[0] - b[0]) < 0.01F && std::fabs(a[1] - b[1]) < 0.01F && std::fabs(a[2] - b[2]) < 0.01F;
}

kartegpu::LichtGpu gpuLicht(const EchtLicht& l) {
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
    g.mehr[0] = l.negativ ? -2.0F : -1.0F;
    g.mehr[1] = 0;
    g.mehr[2] = l.winkelSkala;
    g.mehr[3] = l.extraAbstand;
    return g;
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
    const std::vector<std::string> befehle = {grund + "-meta \"" + karte + "\"", grund + "-vis -fast \"" + bsp + "\"",
                                              grund + "-light -fast \"" + bsp + "\""};
    g_backen.abbrechen = false;
    g_backen.stand = 2;
    g_backen.kartenStand = kartenStand;
    e.echtLicht.bauStand = 2;
    g_backen.faden = std::thread([befehle, bsp] {
        const auto t0 = std::chrono::steady_clock::now();
        for (std::size_t schritt = 0; schritt < befehle.size(); ++schritt) {
            const std::string& b = befehle[schritt];
            std::error_code ec;
            if (schritt == 1 && !fs::exists(fs::path(bsp.substr(0, bsp.size() - 4) + ".prt"), ec)) { continue; }
            bauplattform::Prozess p;
            p.niedrigePrioritaet(true);
            std::string grund;
            if (!p.starten(b, std::string(), false, grund)) {
                std::lock_guard<std::mutex> l(g_backen.mutex);
                g_backen.fehler = "bhed_q3map2: " + grund;
                g_backen.stand = 5;
                return;
            }
            while (!p.warten(30)) {
                if (g_backen.abbrechen) {
                    p.beenden();
                    g_backen.stand = 4;
                    return;
                }
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

void echtLichtBildVorbereiten(const Vektor3f& auge, std::size_t /*szeneStand*/, std::size_t geoStand,
                              const std::vector<kartegpu::SchattenTeil>& teile) {
    Editor& e = ed();
    const auto t0 = std::chrono::steady_clock::now();
    ++g_bild;
    weltAktualisieren(geoStand);
    const karte::EchtLichtWelt& w = g_welt.welt;
    e.echtLicht.lichter = w.lichter.size();
    // Auswahl: die Sonnen, dann nach dem Abstand des Auges zur Huelle
    std::vector<std::pair<float, std::size_t>> reihe;
    reihe.reserve(w.lichter.size());
    for (std::size_t i = 0; i < w.lichter.size(); ++i) {
        const EchtLicht& l = w.lichter[i];
        if (l.art == EchtLicht::Art::Sonne) {
            reihe.emplace_back(-1e30F, i);
            continue;
        }
        const Vektor3f d = l.ursprung - auge;
        reihe.emplace_back(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) - l.huelle, i);
    }
    std::stable_sort(reihe.begin(), reihe.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    if (reihe.size() > static_cast<std::size_t>(kartegpu::kLichtMax)) { reihe.resize(kartegpu::kLichtMax); }
    std::vector<kartegpu::LichtGpu> gpu;
    gpu.reserve(reihe.size());
    int schattenBudget = 8;   // Lichter mit neuem Schattenwuerfel je Bild
    int mitSchatten = 0;
    const EchtLicht* sonne = nullptr;
    for (const auto& [prio, i] : reihe) {
        (void)prio;
        const EchtLicht& l = w.lichter[i];
        kartegpu::LichtGpu g = gpuLicht(l);
        if (l.art == EchtLicht::Art::Sonne) {
            if (sonne == nullptr) { sonne = &l; }
        } else if (!l.negativ && mitSchatten < kartegpu::kSchattenMax) {
            ++mitSchatten;
            const float weite = std::min(l.huelle, 65536.0F);
            int platz = -1;
            for (std::size_t s = 0; s < g_plaetze.size(); ++s) {
                const Platz& p = g_plaetze[s];
                if (p.gueltig && p.geoStand == geoStand && gleicherOrt(p.pos, l.ursprung) && p.huelle == weite) {
                    platz = static_cast<int>(s);
                    break;
                }
            }
            if (platz < 0 && schattenBudget > 0) {
                // der am laengsten nicht gebrauchte Platz
                std::size_t best = 0;
                for (std::size_t s = 1; s < g_plaetze.size(); ++s) {
                    if (!g_plaetze[s].gueltig || g_plaetze[s].zuletzt < g_plaetze[best].zuletzt) { best = s; }
                    if (!g_plaetze[s].gueltig) { break; }
                }
                if (g_plaetze[best].zuletzt != g_bild) {
                    const float pos[3] = {l.ursprung[0], l.ursprung[1], l.ursprung[2]};
                    if (kartegpu::schattenWuerfel(static_cast<int>(best), pos, weite, teile)) {
                        g_plaetze[best] = Platz{true, l.ursprung, weite, geoStand, g_bild};
                        platz = static_cast<int>(best);
                        --schattenBudget;
                        ++e.echtLicht.schattenNeu;
                    }
                }
            }
            if (platz >= 0) {
                g_plaetze[static_cast<std::size_t>(platz)].zuletzt = g_bild;
                g.mehr[0] = static_cast<float>(platz);
                g.mehr[1] = weite;
            }
        }
        gpu.push_back(g);
    }
    // Sonne: Schattenkarte ueber die ganze Karte (bei neuer Geometrie oder Richtung)
    if (sonne != nullptr) {
        if (!g_sonne.gueltig || g_sonne.geoStand != geoStand || !gleicherOrt(g_sonne.richtung, sonne->sonnenRichtung)) {
            const Vektor3f m = g_welt.grenzen.origin;
            const Vektor3f x = g_welt.grenzen.extents;
            const float halb = std::max({x[0], x[1], x[2], 64.0F}) * 1.75F;
            const float r[3] = {sonne->sonnenRichtung[0], sonne->sonnenRichtung[1], sonne->sonnenRichtung[2]};
            const float mi[3] = {m[0], m[1], m[2]};
            g_sonne.gueltig = kartegpu::sonnenSchatten(r, mi, halb, teile);
            g_sonne.richtung = sonne->sonnenRichtung;
            g_sonne.geoStand = geoStand;
        }
    } else if (g_sonne.gueltig) {
        kartegpu::sonnenSchattenAus();
        g_sonne.gueltig = false;
    }
    const float umgebung[3] = {w.umgebung[0], w.umgebung[1], w.umgebung[2]};
    const float minimum[3] = {w.minimum[0], w.minimum[1], w.minimum[2]};
    kartegpu::lichtSetzen(gpu, umgebung, minimum);
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
