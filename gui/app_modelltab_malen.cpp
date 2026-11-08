// gui/app_modelltab_malen.cpp - der Bereich Painting des Model-Tabs (Modell-Block 1c): Malen auf die Bilder hinter den
// JKA-Shadern in der 3D-Ansicht und im 2D-Bild, Werkzeuge, Ebenen, Maske, Spiegeln, Ausbluten, Umbacken, "Fuer das Spiel
// bauen". Die Rechnungen stehen in bhed/modell/malen.h (neu geschrieben; WhiteoutFlakes hat kein Texture Painting).
//
// Vorbilder der Bedienung: 3ds Max Viewport Canvas (Malen in der Ansicht, Ebenen, Pinselgroesse/-staerke, Spiegeln) und
// Blenders Arbeitsbereich Texture Paint (Werkzeuge Draw, Soften, Smear, Clone, Fill, Mask; F Radius, Umschalt+F Staerke,
// S Farbe aufnehmen, X Farben tauschen laut Blender-Handbuch "Texture Paint", 1 Malmaske laut blender_keys.txt).
#include "app_internal.h"
#include "backend.h"
#include "gpumap.h"
#include "textumbruch.h"

#include "bhed/image.h"
#include "bhed/modell/malen.h"
#include "bhed/shaderscript.h"
#include "bhed/sicherschreiben.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bhed::gui {
namespace mm = modell::texmalen;
namespace {

ModellTab& mt() { return g_app->mtab; }
ModellMalen& mz() { return g_app->mtab.malen; }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
bool blenderModus() { return g_app->settings.modellBedienung == 1; }

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    mz().meldung = m;
    addStatus(m);
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

// --- Gruppen: die Flaechen, die ein Bild zeichnen (Haut vor Shader, wie die Vorschau und der UV-Editor) -----------------

std::string shaderVon(int teil) {
    if (teil >= 0 && uz(teil) < g_app->model.surfaces.size()) {
        const GlmSurface& f = g_app->model.surfaces[uz(teil)];
        if (!f.texture.empty()) { return f.texture; }
        if (!f.shader.empty()) { return f.shader; }
    }
    const modell::Inhalt& in = mt().dok.inhalt();
    return teil >= 0 && uz(teil) < in.teile.size() ? in.teile[uz(teil)].shader : std::string();
}

struct Gruppe {
    std::string shader;
    std::vector<int> teile;
};

std::vector<Gruppe> gruppen() {
    const modell::Inhalt& in = mt().dok.inhalt();
    const int stufe = mt().lodJetzt;
    std::vector<Gruppe> g;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const modell::Teil& t = in.teile[i];
        if (t.istTag() || uz(stufe) >= t.stufen.size() || t.stufen[uz(stufe)].dreiecke() == 0) { continue; }
        const std::string s = shaderVon(static_cast<int>(i));
        if (klein(s) == "[nomaterial]" || s.empty()) { continue; }
        auto it = std::find_if(g.begin(), g.end(), [&](const Gruppe& x) { return x.shader == s; });
        if (it == g.end()) {
            g.push_back(Gruppe{s, {}});
            it = g.end() - 1;
        }
        it->teile.push_back(static_cast<int>(i));
    }
    return g;
}

int gruppeVonTeil(const std::vector<Gruppe>& g, int teil) {
    for (std::size_t i = 0; i < g.size(); ++i) {
        if (std::find(g[i].teile.begin(), g[i].teile.end(), teil) != g[i].teile.end()) { return static_cast<int>(i); }
    }
    return -1;
}

// --- Die Bilder eines Shaders: die Datei, die die Engine laedt -----------------------------------------------------------

struct BildStufe {
    std::string name;    // Name im Shader (Anzeige)
    std::string datei;   // gefundene Datei (VFS)
};

// Die Suche liest jede Kandidatendatei ganz aus den Archiven (readFromArchives entpackt); der Bereich, das 2D-Bild und
// jeder Tupfer fragen je Bild danach - ungemerkt 10 ms je Frage, 30 ms je Bild (REST Punkt 1, ptzeit). Gemerkt je Name,
// bis die Spielordner neu eingelesen werden (texCacheStand, wie die Bildtabelle texCache), die Shadertabelle eingelesen
// wird oder ein anderes Modell kommt (bilderPruefen).
struct StufenMerker {
    std::map<std::string, std::vector<BildStufe>> je;   // Shader -> seine Bilder
    std::map<std::string, std::string> datei;           // Bildname -> gefundene Datei (leer: keine)
    int texStand = -1;
    bool shaderGelesen = false;
};
StufenMerker g_stufen;

void merkerPruefen() {
    if (g_stufen.texStand != g_app->texCacheStand || g_stufen.shaderGelesen != g_app->shaderMapRead) {
        g_stufen.je.clear();
        g_stufen.datei.clear();
        g_stufen.texStand = g_app->texCacheStand;
        g_stufen.shaderGelesen = g_app->shaderMapRead;
    }
}

// die Datei, die die Engine fuer einen Bildnamen laedt (R_FindImageFile: die Endungen der Reihe nach)
const std::string& dateiVon(const std::string& name) {
    merkerPruefen();
    const auto it = g_stufen.datei.find(name);
    if (it != g_stufen.datei.end()) { return it->second; }
    std::string gefunden;
    for (const std::string& k : textureCandidates(name)) {
        std::string d;
        if (readFromArchives(k, d)) {
            gefunden = k;
            break;
        }
    }
    return g_stufen.datei.emplace(name, gefunden).first->second;
}

const ShaderInfo* shaderInfoVon(const std::string& shader) {
    if (!g_app->shaderMapRead) { return nullptr; }
    const std::string k = klein(shader);
    const std::size_t punkt = k.find_last_of('.');
    const std::size_t strich = k.find_last_of('/');
    const std::string ohne = (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) ? k.substr(0, punkt) : k;
    for (const std::string& s : {k, ohne}) {
        if (const auto it = g_app->shaderMap.find(s); it != g_app->shaderMap.end()) { return &it->second; }
    }
    return nullptr;
}

// [0] das Bild der Flaeche (Haut/Shader direkt, sonst die erste map-Stufe des gleichnamigen Skripts wie texturesFor in
// app_view3d.cpp), danach die weiteren Bildstufen des Skripts (ohne $lightmap/$whiteimage) - ParseStage in tr_shader.cpp
const std::vector<BildStufe>& bildStufen(const std::string& shader) {
    merkerPruefen();
    if (const auto it = g_stufen.je.find(shader); it != g_stufen.je.end()) { return it->second; }
    std::vector<BildStufe> out;
    const std::string direkt = dateiVon(shader);
    const ShaderInfo* info = shaderInfoVon(shader);
    if (!direkt.empty()) {
        out.push_back(BildStufe{shader, direkt});
    } else if (info != nullptr && !info->image.empty()) {
        const std::string d = dateiVon(info->image);
        if (!d.empty()) { out.push_back(BildStufe{info->image, d}); }
    }
    if (info != nullptr) {
        for (const ShaderInfo::Stage& st : info->stages) {
            if (st.lightmap || st.image.empty()) { continue; }
            const std::string d = dateiVon(st.image);
            if (d.empty()) { continue; }
            if (std::any_of(out.begin(), out.end(), [&](const BildStufe& b) { return klein(b.datei) == klein(d); })) { continue; }
            out.push_back(BildStufe{st.image, d});
        }
    }
    return g_stufen.je.emplace(shader, std::move(out)).first->second;
}

// --- Leinwaende (eine je Bild) ---------------------------------------------------------------------------------------

// REST Punkt 1: eine Stufe einer Flaeche der Vorschau, die dieses Bild zeichnet (ModelTextures::stufen: Flaechen mit
// Shader-Stufen zeichnen die Bilder der Stufen, nicht bySurface - der Sturmtruppler-Rumpf zeigte darum nie, was gemalt
// wurde). Die Bildpunkte der Stufe teilen sich alle Figuren (FigurStufe::pixel); die Vorschau bekommt eine eigene Kopie.
struct StufenBindung {
    int teil = -1;
    std::size_t stufe = 0;
    std::shared_ptr<TextureSet::Tex> eigen;
};

struct MalBild {
    mm::Leinwand l;
    mm::Verlauf v;
    std::string schluessel;               // Shader | Datei
    std::vector<int> teile;
    std::vector<std::array<int, 2>> herkunft;   // je Dreieck der Leinwand: Teil, Dreieck des Netzes
    std::vector<std::array<float, 3>> normalen; // je Dreieck in der Ruhelage
    int netzStand = -1;
    bool grundStufe = true;               // das Bild der Flaeche (bySurface in der 3D-Ansicht), sonst eine weitere Stufe
    mm::Rechteck texSchmutzig;            // fuer die 2D-Ansicht noch hochzuladen
    std::vector<StufenBindung> bindungen; // die Stufen der Vorschau mit diesem Bild
    const void* bindungenFuer = nullptr;  // gesucht fuer diese Stufenliste (ModelTextures::stufen.data())
};

std::vector<std::unique_ptr<MalBild>> g_bilder;
std::string g_bilderFuer;
int g_bilderGeneration = 0;   // zaehlt jedes Leeren (die 2D-Ansicht merkt sich Bilder nach Adresse)

void dreieckeAus(MalBild& m) {
    const modell::Inhalt& in = mt().dok.inhalt();
    const int stufe = mt().lodJetzt;
    std::vector<mm::Dreieck> d;
    m.herkunft.clear();
    m.normalen.clear();
    for (const int teil : m.teile) {
        if (teil < 0 || uz(teil) >= in.teile.size() || uz(stufe) >= in.teile[uz(teil)].stufen.size()) { continue; }
        const modell::Netz& n = in.teile[uz(teil)].stufen[uz(stufe)];
        for (int i = 0; i < n.dreiecke(); ++i) {
            mm::Dreieck x;
            for (int e = 0; e < 3; ++e) {
                const modell::Ecke& c = n.ecken[uz(n.kanten[uz(i * 3 + e)].ecke)];
                x.uv[e][0] = c.uv[0];
                x.uv[e][1] = c.uv[1];
                for (int k = 0; k < 3; ++k) { x.ort[e][k] = c.ort[k]; }
            }
            float a[3];
            float b[3];
            for (int k = 0; k < 3; ++k) {
                a[k] = x.ort[1][k] - x.ort[0][k];
                b[k] = x.ort[2][k] - x.ort[0][k];
            }
            std::array<float, 3> nn{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
            const float l = std::hypot(nn[0], nn[1], nn[2]);
            if (l > 0.0F) {
                for (float& c : nn) { c /= l; }
            }
            d.push_back(x);
            m.herkunft.push_back({teil, i});
            m.normalen.push_back(nn);
        }
    }
    mm::dreieckeSetzen(m.l, d, m.netzStand < 0);
    m.netzStand = in.netzStand;
}

void bilderPruefen() {
    const std::string fuer = g_app->modelPath + "|" + std::to_string(g_app->modelSkinIndex) + "|" + std::to_string(mt().lodJetzt);
    if (fuer != g_bilderFuer) {
        g_bilder.clear();
        g_stufen.je.clear();
        g_stufen.datei.clear();
        ++g_bilderGeneration;
        g_bilderFuer = fuer;
        mz().gruppe = -1;
        mz().stufe = 0;
    }
}

MalBild* bildFuer(int gruppe, int stufe, bool laden) {
    bilderPruefen();
    const std::vector<Gruppe> g = gruppen();
    if (gruppe < 0 || uz(gruppe) >= g.size()) { return nullptr; }
    const std::vector<BildStufe>& st = bildStufen(g[uz(gruppe)].shader);
    if (st.empty()) { return nullptr; }
    const int s = std::clamp(stufe, 0, static_cast<int>(st.size()) - 1);
    const std::string schl = g[uz(gruppe)].shader + "|" + st[uz(s)].datei;
    for (auto& b : g_bilder) {
        if (b->schluessel == schl) {
            if (b->netzStand != mt().dok.inhalt().netzStand) { dreieckeAus(*b); }
            return b.get();
        }
    }
    if (!laden) { return nullptr; }
    std::string bytes;
    if (!readFromArchives(st[uz(s)].datei, bytes)) { return nullptr; }
    auto m = std::make_unique<MalBild>();
    std::string fehler;
    if (!mm::leinwandAusDatei(m->l, st[uz(s)].datei, bytes, &fehler)) {
        melden(format(Str::PtBildFehler, st[uz(s)].datei.c_str(), fehler.c_str()));
        return nullptr;
    }
    m->l.ausbluten = mz().ausbluten;
    m->schluessel = schl;
    m->teile = g[uz(gruppe)].teile;
    m->grundStufe = s == 0;
    dreieckeAus(*m);
    g_bilder.push_back(std::move(m));
    return g_bilder.back().get();
}

MalBild* jetzt(bool laden) {
    ModellMalen& z = mz();
    const std::vector<Gruppe> g = gruppen();
    if (g.empty()) { return nullptr; }
    if (z.gruppe < 0 || uz(z.gruppe) >= g.size()) {
        // die Gruppe der Auswahl, sonst die mit den meisten Dreiecken
        int beste = 0;
        int dz = -1;
        const modell::Inhalt& in = mt().dok.inhalt();
        for (std::size_t i = 0; i < g.size(); ++i) {
            int n = 0;
            for (const int t : g[i].teile) { n += in.teile[uz(t)].stufen[uz(mt().lodJetzt)].dreiecke(); }
            if (n > dz) {
                dz = n;
                beste = static_cast<int>(i);
            }
        }
        if (!mt().auswahl.empty()) {
            const int gw = gruppeVonTeil(g, mt().auswahl.front());
            if (gw >= 0) { beste = gw; }
        }
        z.gruppe = beste;
    }
    return bildFuer(z.gruppe, z.stufe, laden);
}

// Ein Bild der Vorschau aus der Leinwand nachfuehren: gleich gross nur das Rechteck r (Bildpunkte und Grafikkarte, gpu::
// figurBildBereich: Stufe 0 und die Pyramide darueber - bis REST Punkt 1 je Tupfer das ganze Bild samt Pyramide und
// Pruefsumme neu), verkleinert (r_picmip/Bildgrenze) das ganze Bild mit derselben Rechnung wie beim Laden.
void bildNachfuehren(TextureSet::Tex& x, const MalBild& m, const mm::Rechteck& r) {
    if (x.width == m.l.b && x.height == m.l.h && x.rgba.size() == m.l.bild.size()) {
        for (int y = r.y0; y <= r.y1 && r.x0 <= r.x1; ++y) {
            const std::size_t i = m.l.texel(r.x0, y) * 4U;
            std::memcpy(&x.rgba[i], &m.l.bild[i], uz(r.x1 - r.x0 + 1) * 4U);
        }
        if (x.mips.empty() && gpu::figurBildBereich(&x, r.x0, r.y0, r.x1, r.y1)) { return; }
    } else if (x.width > 0 && x.height > 0) {
        image::Image im;
        im.width = m.l.b;
        im.height = m.l.h;
        im.rgba = m.l.bild;
        im.ok = true;
        int w = 0;
        int h = 0;
        std::vector<unsigned char> px;
        image::shrinkTo(im, std::max(x.width, x.height), w, h, px);
        if (w != x.width || h != x.height) { return; }
        x.rgba = std::move(px);
    } else {
        return;
    }
    if (!x.mips.empty()) {
        x.mips.clear();
        x.buildMips();
    }
    x.wache = TexWache{};   // meldet den neuen Inhalt (die Grafikkarte laedt neu, gpumap_win32.cpp figurBildWeg)
}

// Die Stufen der Vorschau suchen, deren Bild diese Leinwand ist (je Flaeche der Gruppe: die Stufen des Shaders, deren
// Bild dieselbe Datei laedt; animMap mit mehreren Bildern und $whiteimage nicht)
void bindungenSuchen(MalBild& m) {
    const auto& st = g_app->modelTextures.stufen;
    m.bindungen.clear();
    m.bindungenFuer = st.data();
    for (const int t : m.teile) {
        if (t < 0 || uz(t) >= st.size() || st[uz(t)].empty()) { continue; }
        const ShaderInfo* info = shaderInfoVon(shaderVon(t));
        if (info == nullptr) { continue; }
        for (std::size_t k = 0; k < st[uz(t)].size(); ++k) {
            const ModelTextures::FigurStufe& fs = st[uz(t)][k];
            if (fs.art.weiss || fs.pixel.size() != 1U || fs.pixel[0] == nullptr || fs.art.stufe >= info->stages.size()) { continue; }
            if (klein(dateiVon(info->stages[fs.art.stufe].image)) == klein(m.l.datei)) { m.bindungen.push_back(StufenBindung{t, k, nullptr}); }
        }
    }
}

// Die Stufen der Vorschau dieser Leinwand nachfuehren; eine Stufe ohne eigene Kopie (das erste Mal, neu geladene
// Texturen) bekommt sie und das ganze Bild. true, wenn eine Stufe da ist.
bool stufenNachfuehren(MalBild& m, mm::Rechteck r) {
    auto& st = g_app->modelTextures.stufen;
    if (m.bindungenFuer != st.data()) { bindungenSuchen(m); }
    bool da = false;
    for (StufenBindung& b : m.bindungen) {
        if (uz(b.teil) >= st.size() || b.stufe >= st[uz(b.teil)].size()) { continue; }
        ModelTextures::FigurStufe& fs = st[uz(b.teil)][b.stufe];
        if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr) { continue; }
        mm::Rechteck rb = r;
        if (fs.pixel[0].get() != b.eigen.get()) {
            b.eigen = std::make_shared<TextureSet::Tex>(*fs.pixel[0]);
            b.eigen->mips.clear();   // die Grafikkarte rechnet dieselbe Pyramide (ladeMitStufen/figurBildBereich wie buildMips)
            fs.pixel[0] = b.eigen;
            rb = mm::Rechteck{0, 0, m.l.b - 1, m.l.h - 1};
        }
        bildNachfuehren(*b.eigen, m, rb);
        da = true;
    }
    return da;
}

// Das Bild in die Vorschau: die Texturen der Flaechen der Gruppe (bySurface fuer das Bild der Flaeche, die Bilder der
// Shader-Stufen) und die 2D-Ansicht
void anzeigeNachfuehren(MalBild& m) {
    if (m.l.schmutzig.leer()) { return; }
    const auto ab = std::chrono::steady_clock::now();
    m.texSchmutzig.dazu(m.l.schmutzig);
    const mm::Rechteck r{std::max(m.l.schmutzig.x0, 0), std::max(m.l.schmutzig.y0, 0), std::min(m.l.schmutzig.x1, m.l.b - 1), std::min(m.l.schmutzig.y1, m.l.h - 1)};
    bool ansicht = stufenNachfuehren(m, r);
    if (m.grundStufe) {
        auto& by = g_app->modelTextures.bySurface;
        for (const int t : m.teile) {
            if (t >= 0 && uz(t) < by.size()) { bildNachfuehren(by[uz(t)], m, r); }
        }
        ansicht = true;
    }
    if (ansicht) {
        g_app->modelDirty = true;
        ++mz().ansichtStand;   // die 3D-Ansichten zeichnen im selben Bild neu (Stand in eineAnsicht)
    }
    m.l.schmutzig = mm::Rechteck{};
    mz().hochladenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab).count();
    ++mz().hochgeladen;
}

// --- Treffer in der Ansicht ----------------------------------------------------------------------------------------

bool nachSchirm(const ModellAnsicht& a, const float p[3], ImVec2& out, float& w) {
    const float* m = a.viewProj;
    const float x = m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3];
    const float y = m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7];
    w = m[12] * p[0] + m[13] * p[1] + m[14] * p[2] + m[15];
    if (w <= 1e-3F) { return false; }
    out = ImVec2{a.lo[0] + (x / w * 0.5F + 0.5F) * a.groesse[0], a.lo[1] + (0.5F - y / w * 0.5F) * a.groesse[1]};
    return true;
}

struct Treffer {
    int teil = -1;
    int dreieck = -1;
    float l[3]{};      // perspektivisch richtige Schwerpunktkoordinaten
    float pose[3]{};   // Treffpunkt in der Pose
    float tiefe = 2.0F;
};

bool treffer(const ModellAnsicht& a, ImVec2 maus, Treffer& tr) {
    const ModellTab& t = mt();
    const auto& h = modellHaut();
    tr = Treffer{};
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        const GlmSurface& s = t.anzeige.surfaces[i];
        for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
            ImVec2 p[3];
            float w[3];
            float z[3];
            bool ok = true;
            for (std::size_t e = 0; e < 3 && ok; ++e) {
                const float* q = h[i][s.indexes[k + e]].data();
                ok = nachSchirm(a, q, p[e], w[e]);
                const float* m = a.viewProj;
                z[e] = (m[8] * q[0] + m[9] * q[1] + m[10] * q[2] + m[11]) / std::max(w[e], 1e-3F);
            }
            if (!ok) { continue; }
            const float f = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y);
            if (std::fabs(f) < 1e-6F) { continue; }
            const float b0 = ((p[1].x - maus.x) * (p[2].y - maus.y) - (p[2].x - maus.x) * (p[1].y - maus.y)) / f;
            const float b1 = ((p[2].x - maus.x) * (p[0].y - maus.y) - (p[0].x - maus.x) * (p[2].y - maus.y)) / f;
            const float b2 = 1.0F - b0 - b1;
            if (b0 < 0.0F || b1 < 0.0F || b2 < 0.0F) { continue; }
            const float tiefe = b0 * z[0] + b1 * z[1] + b2 * z[2];
            if (tiefe >= tr.tiefe) { continue; }
            // perspektivisch richtig: durch w teilen (die Schirm-Koordinaten sind nach der Teilung linear)
            const float q0 = b0 / w[0];
            const float q1 = b1 / w[1];
            const float q2 = b2 / w[2];
            const float qs = q0 + q1 + q2;
            tr.tiefe = tiefe;
            tr.teil = static_cast<int>(i);
            tr.dreieck = static_cast<int>(k / 3);
            tr.l[0] = q0 / qs;
            tr.l[1] = q1 / qs;
            tr.l[2] = q2 / qs;
            for (int c = 0; c < 3; ++c) {
                tr.pose[c] = h[i][s.indexes[k]][uz(c)] * tr.l[0] + h[i][s.indexes[k + 1]][uz(c)] * tr.l[1] + h[i][s.indexes[k + 2]][uz(c)] * tr.l[2];
            }
        }
    }
    return tr.teil >= 0;
}

// Welt je Bildpunkt an einem Ort der Pose (fuer den Radius des Pinsels in Bildpunkten)
float weltJePixel(const ModellAnsicht& a, const float p[3]) {
    ImVec2 s0;
    float w0 = 0.0F;
    if (!nachSchirm(a, p, s0, w0)) { return 1.0F; }
    float best = 0.0F;
    for (int k = 0; k < 3; ++k) {
        float q[3] = {p[0], p[1], p[2]};
        q[k] += 1.0F;
        ImVec2 s1;
        float w1 = 0.0F;
        if (!nachSchirm(a, q, s1, w1)) { continue; }
        best = std::max(best, std::hypot(s1.x - s0.x, s1.y - s0.y));
    }
    return best > 1e-6F ? 1.0F / best : 1.0F;
}

// Treffer -> Ort in der Ruhelage (die Ecken des Dokuments mit denselben Schwerpunktkoordinaten); -1 ohne Dreieck
int ruheOrt(const MalBild& m, const Treffer& tr, float ort[3], float uv[2]) {
    for (std::size_t i = 0; i < m.herkunft.size(); ++i) {
        if (m.herkunft[i][0] != tr.teil || m.herkunft[i][1] != tr.dreieck) { continue; }
        mm::punktIm(m.l.dreiecke[i], tr.l[0], tr.l[1], tr.l[2], uv, ort);
        return static_cast<int>(i);
    }
    return -1;
}

// je Dreieck: zeigt es in dieselbe Richtung wie die getroffene Flaeche (Blender "Normal Falloff", Vorgabe 80 Grad)?
std::vector<std::uint8_t> zugewandt(const MalBild& m, const std::array<float, 3>& ref) {
    const float grenze = std::cos(80.0F * 3.14159265F / 180.0F);
    std::vector<std::uint8_t> z(m.normalen.size(), 0);
    for (std::size_t i = 0; i < z.size(); ++i) {
        const auto& n = m.normalen[i];
        z[i] = n[0] * ref[0] + n[1] * ref[1] + n[2] * ref[2] > grenze ? 1 : 0;
    }
    return z;
}

float druckJetzt() {
    const ModellMalen& z = mz();
    return z.stiftDa ? z.stiftDruck : 1.0F;
}

mm::Pinsel pinselJetzt(float radius) {
    const ModellMalen& z = mz();
    mm::Pinsel p = z.pinsel;
    p.werkzeug = static_cast<mm::Werkzeug>(std::clamp(z.werkzeug, 0, mm::kWerkzeuge - 1));
    p.radius = radius;
    p.druckGroesse = z.druckGroesse && z.stiftDa;
    p.druckStaerke = z.druckStaerke && z.stiftDa;
    p.klonVersatz[0] = z.klonVersatz[0];
    p.klonVersatz[1] = z.klonVersatz[1];
    return p;
}

void strichBeginnen(MalBild& m) {
    ModellMalen& z = mz();
    mm::schrittBeginnen(m.v, m.l, tr(Str::PtUndoStrich));
    z.strich = mm::Strich{};
    z.strichLaeuft = true;
    z.klonErster = true;
}

void klonVersatzSetzen(const float uv[2]) {
    ModellMalen& z = mz();
    if (z.klonErster && z.klonQuelleDa) {
        z.klonVersatz[0] = z.klonQuelle[0] - uv[0];
        z.klonVersatz[1] = z.klonQuelle[1] - uv[1];
    }
    z.klonErster = false;
}

}  // namespace

// --- Malen in der 3D-Ansicht ------------------------------------------------------------------------------------------

bool modellMalenAktiv() { return mt().bereich == ModellBereich::Painting && !g_app->model.empty(); }

bool modellMalenAnsicht(const ModellAnsicht& a, ImVec2 maus, bool neu) {
    ModellMalen& z = mz();
    Treffer trf;
    if (!treffer(a, maus, trf)) { return false; }
    const std::vector<Gruppe> g = gruppen();
    if (neu) {
        // die Gruppe unter dem Pinsel wird die Leinwand (wie Blender das Bild des Materials unter dem Pinsel)
        const int gw = gruppeVonTeil(g, trf.teil);
        if (gw < 0) {
            melden(tr(Str::PtKeinBild));
            return false;
        }
        if (gw != z.gruppe) {
            z.gruppe = gw;
            z.stufe = 0;
        }
    } else if (!z.strichLaeuft) {
        return false;
    }
    MalBild* m = jetzt(true);
    if (m == nullptr) {
        if (neu) { melden(tr(Str::PtKeinBild)); }
        return false;
    }
    float ort[3];
    float uv[2];
    const int di = ruheOrt(*m, trf, ort, uv);
    if (di < 0) { return false; }
    // Ctrl+Klick: Quelle fuer Klonen (Blender: Strg+Linksklick setzt die Quelle)
    if (neu && ImGui::GetIO().KeyCtrl && z.werkzeug == static_cast<int>(mm::Werkzeug::Klonen)) {
        z.klonQuelle[0] = uv[0];
        z.klonQuelle[1] = uv[1];
        z.klonQuelleDa = true;
        melden(format(Str::PtKlonQuelle, static_cast<double>(uv[0]), static_cast<double>(uv[1])));
        return true;
    }
    // Pinselgroesse in der Welt: Bildpunkte mal Welt je Bildpunkt am Treffpunkt der Pose (gleiche Einheiten wie die Ruhelage)
    const float radius = z.radiusPx * weltJePixel(a, trf.pose);
    const mm::Pinsel p = pinselJetzt(radius);
    if (p.werkzeug == mm::Werkzeug::Fuellen) {
        if (!neu) { return true; }
        mm::schrittBeginnen(m->v, m->l, tr(Str::PtUndoFuellen));
        // UV ausserhalb 0..1 wiederholt sich (GL_REPEAT): der Texel im Bild
        const int x = std::clamp(static_cast<int>((uv[0] - std::floor(uv[0])) * static_cast<float>(m->l.b)), 0, m->l.b - 1);
        const int y = std::clamp(static_cast<int>((uv[1] - std::floor(uv[1])) * static_cast<float>(m->l.h)), 0, m->l.h - 1);
        const int n = mm::fuellen(m->l, m->v, p, x, y);
        mm::schrittEnde(m->v, m->l);
        z.texelGemalt = n;
        ++z.tupfer;
        ++z.befehle;
        anzeigeNachfuehren(*m);
        return true;
    }
    if (neu) {
        strichBeginnen(*m);
        if (p.werkzeug == mm::Werkzeug::Klonen && !z.klonQuelleDa) { melden(tr(Str::PtKlonOhneQuelle)); }
    }
    if (p.werkzeug == mm::Werkzeug::Klonen) { klonVersatzSetzen(uv); }
    const std::vector<std::uint8_t> zw = zugewandt(*m, m->normalen[uz(di)]);
    std::array<float, 3> nGesp = m->normalen[uz(di)];
    nGesp[uz(z.spiegelAchse)] = -nGesp[uz(z.spiegelAchse)];
    const std::vector<std::uint8_t> zwGesp = z.spiegeln ? zugewandt(*m, nGesp) : std::vector<std::uint8_t>{};
    int n = 0;
    float voriges[2] = {z.letzteUv[0], z.letzteUv[1]};
    for (const std::array<float, 3>& st : z.strich.weiter(ort, std::max(radius * std::max(p.abstand, 0.01F), 1e-4F))) {
        const float richtung[2] = {uv[0] - voriges[0], uv[1] - voriges[1]};
        n += mm::tupfer3d(m->l, m->v, p, st.data(), druckJetzt(), &zw, neu ? nullptr : richtung);
        if (z.spiegeln) {
            float sp[3];
            mm::spiegelPunkt(st.data(), z.spiegelAchse, sp);
            n += mm::tupfer3d(m->l, m->v, p, sp, druckJetzt(), &zwGesp, nullptr);
        }
        ++z.tupfer;
    }
    z.letzteUv[0] = uv[0];
    z.letzteUv[1] = uv[1];
    z.texelGemalt += n;
    anzeigeNachfuehren(*m);
    return true;
}

void modellMalenEnde() {
    ModellMalen& z = mz();
    if (!z.strichLaeuft) { return; }
    z.strichLaeuft = false;
    for (auto& b : g_bilder) { mm::schrittEnde(b->v, b->l); }
    ++z.befehle;
    z.letzterBefehl = tr(Str::PtUndoStrich);
}

void modellMalenUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    if (!modellMalenAktiv()) { return; }
    if (mz().testTupfen && &a == &mt().ansichten[mt().aktiv]) {
        // Selbsttest ptzeit: ein Tupfer je Bild auf die Mitte der Ansicht (ein Strich, der nicht endet)
        (void)modellMalenAnsicht(a, ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.45F}, true);
    }
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (m.x < a.lo[0] || m.y < a.lo[1] || m.x > a.lo[0] + a.groesse[0] || m.y > a.lo[1] + a.groesse[1]) { return; }
    const ModellMalen& z = mz();
    const ImU32 c = z.werkzeug == static_cast<int>(mm::Werkzeug::Maske) ? IM_COL32(250, 90, 90, 230) : IM_COL32(255, 255, 255, 220);
    dl->AddCircle(m, z.radiusPx, c, 48, 1.5F);
    dl->AddCircle(m, z.radiusPx * std::clamp(z.pinsel.haerte, 0.0F, 1.0F), IM_COL32(255, 255, 255, 90), 48, 1.0F);
    if (z.modalArt != 0) {
        char b[64];
        std::snprintf(b, sizeof b, z.modalArt == 1 ? "%.0f px" : "%.2f", z.modalArt == 1 ? static_cast<double>(z.radiusPx) : static_cast<double>(z.pinsel.staerke));
        dl->AddText(ImVec2{m.x + z.radiusPx + 6.0F, m.y - 8.0F}, IM_COL32(255, 255, 255, 255), b);
    }
}

// Tablet: Druck aus WM_POINTER (main_win32.cpp), 0..1
void modellStiftDruck(float druck) {
    ModellMalen& z = mz();
    z.stiftDa = true;
    z.stiftDruck = std::clamp(druck, 0.0F, 1.0F);
}

const mm::Leinwand* modellMalenLeinwand() {
    const MalBild* m = jetzt(true);
    return m != nullptr ? &m->l : nullptr;
}

bool modellMalenRueckgaengig() {
    MalBild* m = jetzt(false);
    if (m == nullptr || !mm::rueckgaengig(m->v, m->l)) { return false; }
    anzeigeNachfuehren(*m);
    melden(tr(Str::PtRueckgaengig));
    return true;
}

bool modellMalenWiederholen() {
    MalBild* m = jetzt(false);
    if (m == nullptr || !mm::wiederholen(m->v, m->l)) { return false; }
    anzeigeNachfuehren(*m);
    return true;
}

// --- Befehle ---------------------------------------------------------------------------------------------------------

namespace {

void stapelBefehl(Str name, const std::function<void(std::vector<mm::Ebene>&, int&)>& f) {
    MalBild* m = jetzt(true);
    if (m == nullptr) {
        melden(tr(Str::PtKeinBild));
        return;
    }
    std::vector<mm::Ebene> st = m->l.ebenen;
    int aktiv = m->l.aktiv;
    f(st, aktiv);
    mm::stapelAendern(m->v, m->l, tr(name), st, aktiv);
    anzeigeNachfuehren(*m);
    ++mz().befehle;
    mz().letzterBefehl = tr(name);
}

void ebeneNeu() {
    stapelBefehl(Str::PtEbeneNeu, [](std::vector<mm::Ebene>& st, int& aktiv) {
        mm::Ebene e;
        e.name = format(Str::PtEbeneName, static_cast<int>(st.size()));
        e.rgba.assign(st.front().rgba.size(), 0);
        st.insert(st.begin() + aktiv + 1, std::move(e));
        ++aktiv;
    });
}

void ebeneLoeschen() {
    MalBild* m = jetzt(false);
    if (m == nullptr || m->l.aktiv == 0) {
        melden(tr(Str::PtGrundBleibt));
        return;
    }
    stapelBefehl(Str::PtEbeneLoeschen, [](std::vector<mm::Ebene>& st, int& aktiv) {
        st.erase(st.begin() + aktiv);
        aktiv = std::max(0, aktiv - 1);
    });
}

void ebeneVerschieben(int d) {
    MalBild* m = jetzt(false);
    if (m == nullptr) { return; }
    const int a = m->l.aktiv;
    const int b = a + d;
    if (a == 0 || b < 1 || b >= static_cast<int>(m->l.ebenen.size())) { return; }   // das Original bleibt unten
    stapelBefehl(d > 0 ? Str::PtEbeneHoch : Str::PtEbeneRunter, [a, b](std::vector<mm::Ebene>& st, int& aktiv) {
        std::swap(st[uz(a)], st[uz(b)]);
        aktiv = b;
    });
}

void ebeneVereinen() {
    MalBild* m = jetzt(false);
    if (m == nullptr || m->l.aktiv == 0) {
        melden(tr(Str::PtGrundBleibt));
        return;
    }
    // die aktive Ebene in die darunter: wie das Ergebnis der beiden (Mischart und Deckkraft eingerechnet)
    stapelBefehl(Str::PtEbeneVereinen, [m](std::vector<mm::Ebene>& st, int& aktiv) {
        mm::Leinwand l;
        l.b = m->l.b;
        l.h = m->l.h;
        l.ebenen = {st[uz(aktiv - 1)], st[uz(aktiv)]};
        l.ebenen[0].deckkraft = 1.0F;
        mm::zusammensetzen(l);
        st[uz(aktiv - 1)].rgba = l.bild;
        st.erase(st.begin() + aktiv);
        --aktiv;
    });
}

void maskeAendern(Str name, bool umkehren) {
    MalBild* m = jetzt(true);
    if (m == nullptr) { return; }
    mm::schrittBeginnen(m->v, m->l, tr(name));
    mm::sichern(m->v, m->l, -1, mm::Rechteck{0, 0, m->l.b - 1, m->l.h - 1});
    for (std::uint8_t& x : m->l.maske) { x = umkehren ? static_cast<std::uint8_t>(255 - x) : 0; }
    mm::schrittEnde(m->v, m->l);
    ++mz().befehle;
    mz().letzterBefehl = tr(name);
    melden(tr(name));
}

void umbackenBefehl() {
    MalBild* m = jetzt(true);
    if (m == nullptr) { return; }
    if (!mm::leinwandUmbacken(m->l, m->v, tr(Str::PtUmbacken))) {
        melden(tr(Str::PtUmbackenNichts));
        return;
    }
    anzeigeNachfuehren(*m);
    ++mz().befehle;
    mz().letzterBefehl = tr(Str::PtUmbacken);
    melden(tr(Str::PtUmbackenFertig));
}

std::string ausgabeOrdner() {
    const ModellMalen& z = mz();
    if (z.ordner[0] != '\0') { return z.ordner; }
    if (const char* p = std::getenv("BHED_PAINT_MOD"); p != nullptr && p[0] != '\0') { return p; }
    if (g_app->missionAktiv && !g_app->mission.projektOrdner.empty()) { return g_app->mission.projektOrdner; }
    const std::string d = platform::dokumenteOrdner();
    return d.empty() ? std::string() : d + "\\behaved\\bauen\\paint";
}

// nie in einen Spielordner: der Ordner ueber jedem Suchordner des Spiels (GameData ...) ist tabu
bool imSpiel(const std::string& ordner) {
    const std::string o = klein(ordner) + "/";
    for (const GamePath& gp : g_app->gamePaths) {
        std::string w = klein(gp.directory);
        while (!w.empty() && w.back() == '/') { w.pop_back(); }
        const std::size_t s = w.find_last_of('/');
        const std::string wurzel = s == std::string::npos ? w : w.substr(0, s);
        if (wurzel.size() > 3 && o.rfind(wurzel + "/", 0) == 0) { return true; }
    }
    return false;
}

}  // namespace

// "Fuer das Spiel bauen": jedes bemalte Bild unter seinem Spielpfad in den Ausgabeordner (Format und Groesse des Originals)
int modellMalenBauen() {
    const std::string ordner = ausgabeOrdner();
    if (ordner.empty()) {
        melden(tr(Str::PtKeinOrdner));
        return -1;
    }
    if (imSpiel(ordner)) {
        melden(format(Str::PtImSpielordner, ordner.c_str()));
        return -1;
    }
    int n = 0;
    for (auto& b : g_bilder) {
        if (b->l.bild == b->l.grund) { continue; }   // unbemalt: nichts schreiben (das Spiel nimmt das Original)
        std::string bytes;
        std::string fehler;
        if (!mm::leinwandBytes(b->l, bytes, &fehler)) {
            melden(format(Str::PtBauFehler, b->l.datei.c_str(), fehler.c_str()));
            return -1;
        }
        const std::filesystem::path ziel = std::filesystem::path(utf8Pfad(ordner)) / utf8Pfad(b->l.datei);
        std::error_code ec;
        std::filesystem::create_directories(ziel.parent_path(), ec);
        if (!sicherSchreiben(ziel.string(), bytes, &fehler)) {
            melden(format(Str::PtBauFehler, ziel.string().c_str(), fehler.c_str()));
            return -1;
        }
        b->l.gespeichert = b->l.stand;
        diag::detail("Painting: " + ziel.string() + " " + std::to_string(bytes.size()) + " Bytes, " + std::to_string(b->l.b) + "x" + std::to_string(b->l.h));
        ++n;
    }
    mz().gebaut = n;
    mz().gebautOrdner = ordner;
    melden(n == 0 ? std::string(tr(Str::PtNichtsBemalt)) : format(Str::PtGebaut, n, ordner.c_str()));
    return n;
}

namespace {

// --- Aktionen: Menue, Rechtsklick, Suche, Tasten, Tooltips ----------------------------------------------------------

struct MalAktion {
    Str text;
    Str hinweis;
    const char* maxTaste;
    const char* blenderTaste;
    ImGuiKeyChord maxChord;
    ImGuiKeyChord blenderChord;
    std::function<void()> tun;
    std::function<bool()> an;
    int gruppe;   // 0 Werkzeuge, 1 Pinsel, 2 Ebenen, 3 Maske, 4 Bild
};

void werkzeug(mm::Werkzeug w) { mz().werkzeug = static_cast<int>(w); }
bool istWerkzeug(mm::Werkzeug w) { return mz().werkzeug == static_cast<int>(w); }

void farbeAufnehmen() {
    // die Farbe unter der Maus: in der 3D-Ansicht der Treffer, im 2D-Bild der Texel (Blender S "Sample Color")
    ModellMalen& z = mz();
    z.aufnehmen = true;
}

const std::vector<MalAktion>& aktionen() {
    using W = mm::Werkzeug;
    static const std::vector<MalAktion> kListe = {
        {Str::PtWzPinsel, Str::PtWzPinselHint, "", "", 0, 0, [] { werkzeug(W::Pinsel); }, [] { return istWerkzeug(W::Pinsel); }, 0},
        {Str::PtWzWeich, Str::PtWzWeichHint, "", "", 0, 0, [] { werkzeug(W::Weichzeichnen); }, [] { return istWerkzeug(W::Weichzeichnen); }, 0},
        {Str::PtWzWischen, Str::PtWzWischenHint, "", "", 0, 0, [] { werkzeug(W::Verwischen); }, [] { return istWerkzeug(W::Verwischen); }, 0},
        {Str::PtWzKlonen, Str::PtWzKlonenHint, "", "", 0, 0, [] { werkzeug(W::Klonen); }, [] { return istWerkzeug(W::Klonen); }, 0},
        {Str::PtWzFuellen, Str::PtWzFuellenHint, "", "", 0, 0, [] { werkzeug(W::Fuellen); }, [] { return istWerkzeug(W::Fuellen); }, 0},
        {Str::PtWzMaske, Str::PtWzMaskeHint, "", "", 0, 0, [] { werkzeug(W::Maske); }, [] { return istWerkzeug(W::Maske); }, 0},
        {Str::PtWzRadierer, Str::PtWzRadiererHint, "", "", 0, 0, [] { werkzeug(W::Radierer); }, [] { return istWerkzeug(W::Radierer); }, 0},
        // Pinsel: Blender-Handbuch Texture Paint (F Radius, Umschalt+F Staerke, S Farbe aufnehmen, X Farben tauschen)
        {Str::PtRadiusZiehen, Str::PtRadiusZiehenHint, "", "F", 0, ImGuiKey_F, [] { mz().modalArt = 1; mz().modalStart = ImGui::GetIO().MousePos.x; mz().modalWert = mz().radiusPx; }, {}, 1},
        {Str::PtStaerkeZiehen, Str::PtStaerkeZiehenHint, "", "Shift+F", 0, ImGuiMod_Shift | ImGuiKey_F,
         [] { mz().modalArt = 2; mz().modalStart = ImGui::GetIO().MousePos.x; mz().modalWert = mz().pinsel.staerke; }, {}, 1},
        {Str::PtFarbeAufnehmen, Str::PtFarbeAufnehmenHint, "", "S", 0, ImGuiKey_S, [] { farbeAufnehmen(); }, [] { return mz().aufnehmen; }, 1},
        {Str::PtFarbenTauschen, Str::PtFarbenTauschenHint, "", "X", 0, ImGuiKey_X, [] { std::swap(mz().pinsel.farbe, mz().farbe2); }, {}, 1},
        {Str::PtSpiegeln, Str::PtSpiegelnHint, "", "", 0, 0, [] { mz().spiegeln = !mz().spiegeln; }, [] { return mz().spiegeln; }, 1},
        {Str::PtDruckGroesse, Str::PtDruckGroesseHint, "", "", 0, 0, [] { mz().druckGroesse = !mz().druckGroesse; }, [] { return mz().druckGroesse; }, 1},
        {Str::PtDruckStaerke, Str::PtDruckStaerkeHint, "", "", 0, 0, [] { mz().druckStaerke = !mz().druckStaerke; }, [] { return mz().druckStaerke; }, 1},
        {Str::PtRueckseiten, Str::PtRueckseitenHint, "", "", 0, 0, [] { mz().pinsel.rueckseiten = !mz().pinsel.rueckseiten; }, [] { return mz().pinsel.rueckseiten; }, 1},
        // Ebenen (Max Viewport Canvas: Ebenen-Fenster; Blender: Bilder je Material)
        {Str::PtEbeneNeu, Str::PtEbeneNeuHint, "", "", 0, 0, [] { ebeneNeu(); }, {}, 2},
        {Str::PtEbeneLoeschen, Str::PtEbeneLoeschenHint, "", "", 0, 0, [] { ebeneLoeschen(); }, {}, 2},
        {Str::PtEbeneHoch, Str::PtEbeneHochHint, "", "", 0, 0, [] { ebeneVerschieben(1); }, {}, 2},
        {Str::PtEbeneRunter, Str::PtEbeneRunterHint, "", "", 0, 0, [] { ebeneVerschieben(-1); }, {}, 2},
        {Str::PtEbeneVereinen, Str::PtEbeneVereinenHint, "", "", 0, 0, [] { ebeneVereinen(); }, {}, 2},
        // Maske (Blender 1: use_paint_mask laut blender_keys.txt "Image Paint")
        {Str::PtMaskeAn, Str::PtMaskeAnHint, "", "1", 0, ImGuiKey_1, [] { mz().maskeZeigen = !mz().maskeZeigen; }, [] { return mz().maskeZeigen; }, 3},
        {Str::PtMaskeLeeren, Str::PtMaskeLeerenHint, "", "", 0, 0, [] { maskeAendern(Str::PtMaskeLeeren, false); }, {}, 3},
        {Str::PtMaskeUmkehren, Str::PtMaskeUmkehrenHint, "", "", 0, 0, [] { maskeAendern(Str::PtMaskeUmkehren, true); }, {}, 3},
        // Bild
        {Str::PtEditorZeigen, Str::PtEditorZeigenHint, "", "", 0, 0, [] { mz().editorOffen = !mz().editorOffen; }, [] { return mz().editorOffen; }, 4},
        {Str::PtUvZeigen, Str::PtUvZeigenHint, "", "", 0, 0, [] { mz().uvZeigen = !mz().uvZeigen; }, [] { return mz().uvZeigen; }, 4},
        {Str::PtUmbacken, Str::PtUmbackenHint, "", "", 0, 0, [] { umbackenBefehl(); }, {}, 4},
        {Str::PtBauen, Str::PtBauenHint, "", "", 0, 0, [] { (void)modellMalenBauen(); }, {}, 4},
    };
    return kListe;
}

const char* tasteVon(const MalAktion& a) { return blenderModus() ? a.blenderTaste : a.maxTaste; }

void ausloesen(const MalAktion& a) {
    if (mt().bereich != ModellBereich::Painting) { mt().bereich = ModellBereich::Painting; }
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(a.text);
    g_app->modelDirty = true;
}

std::string tooltip(const MalAktion& a) {
    std::string s = tr(a.hinweis);
    const char* k = tasteVon(a);
    if (k[0] != '\0') { s += "\n" + format(Str::UvTasteHint, k); }
    return s;
}

void alsMenue() {
    static const Str kGruppen[5] = {Str::PtGrWerkzeuge, Str::PtGrPinsel, Str::PtGrEbenen, Str::PtGrMaske, Str::PtGrBild};
    const auto& liste = aktionen();
    int gruppe = -1;
    bool unter = false;
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const MalAktion& a = liste[i];
        if (a.gruppe != gruppe) {
            if (unter) { ImGui::EndMenu(); }
            gruppe = a.gruppe;
            unter = ImGui::BeginMenu(tr(kGruppen[uz(gruppe)]));
            if (!unter) {
                while (i + 1 < liste.size() && liste[i + 1].gruppe == gruppe) { ++i; }
                continue;
            }
        }
        ImGui::PushID(static_cast<int>(i));
        const char* k = tasteVon(a);
        if (ImGui::MenuItem(tr(a.text), k[0] != '\0' ? k : nullptr, a.an ? a.an() : false)) { ausloesen(a); }
        ImGui::SetItemTooltip("%s", tooltip(a).c_str());
        ImGui::PopID();
    }
    if (unter) { ImGui::EndMenu(); }
}

std::string tooltipVon(Str text) {
    const auto& liste = aktionen();
    const auto it = std::find_if(liste.begin(), liste.end(), [&](const MalAktion& a) { return a.text == text; });
    return it != liste.end() ? tooltip(*it) : std::string(tr(text));
}

bool knopf(Str text, float breite = 0.0F) {
    const auto& liste = aktionen();
    const auto it = std::find_if(liste.begin(), liste.end(), [&](const MalAktion& a) { return a.text == text; });
    const bool an = it != liste.end() && it->an && it->an();
    if (an) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
    const bool k = ImGui::Button(tr(text), ImVec2{breite, 0.0F});
    if (an) { ImGui::PopStyleColor(); }
    if (it != liste.end()) {
        ImGui::SetItemTooltip("%s", tooltip(*it).c_str());
        if (k) { ausloesen(*it); }
    }
    return k;
}

}  // namespace

int modellMalenAktionZahl() { return static_cast<int>(aktionen().size()); }
int modellMalenAktionGruppe(int i) { return aktionen()[uz(i)].gruppe; }
const char* modellMalenAktionText(int i) { return tr(aktionen()[uz(i)].text); }
const char* modellMalenAktionHinweis(int i) { return tr(aktionen()[uz(i)].hinweis); }
const char* modellMalenAktionTaste(int i, bool blender) { return blender ? aktionen()[uz(i)].blenderTaste : aktionen()[uz(i)].maxTaste; }
void modellMalenAktionAusloesen(int i) { ausloesen(aktionen()[uz(i)]); }
void modellMalenMenue() { alsMenue(); }

void modellMalenKontextMenue() {
    if (!modellMalenAktiv()) { return; }
    if (ImGui::BeginMenu(tr(Str::PtMenue))) {
        alsMenue();
        ImGui::EndMenu();
    }
}

bool modellMalenAktionenSuche(const std::string& such) {
    bool ausgeloest = false;
    for (const MalAktion& a : aktionen()) {
        std::string t = tr(a.text);
        const char* k = tasteVon(a);
        if (k[0] != '\0') { t += std::string(" (") + k + ")"; }
        if (!such.empty() && klein(t).find(klein(such)) == std::string::npos) { continue; }
        ImGui::PushID(&a);
        if (ImGui::Selectable(t.c_str()) && !ausgeloest) {
            ausloesen(a);
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", tooltip(a).c_str());
        ImGui::PopID();
    }
    return ausgeloest;
}

bool modellMalenTasten() {
    if (!modellMalenAktiv()) { return false; }
    ModellMalen& z = mz();
    if (z.modalArt != 0) {
        // F / Umschalt+F: waagerecht ziehen, Klick oder Enter uebernimmt, Rechtsklick oder Esc bricht ab (Blender radial_control)
        const float dx = ImGui::GetIO().MousePos.x - z.modalStart;
        if (z.modalArt == 1) {
            z.radiusPx = std::clamp(z.modalWert + dx, 1.0F, 500.0F);
        } else {
            z.pinsel.staerke = std::clamp(z.modalWert + dx / 200.0F, 0.0F, 1.0F);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (z.modalArt == 1) { z.radiusPx = z.modalWert; } else { z.pinsel.staerke = z.modalWert; }
            z.modalArt = 0;
        } else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            z.modalArt = 0;
            z.modalEndeBild = ImGui::GetFrameCount();
        }
        return true;
    }
    const bool blender = blenderModus();
    for (const MalAktion& a : aktionen()) {
        const ImGuiKeyChord c = blender ? a.blenderChord : a.maxChord;
        if (c != 0 && ImGui::IsKeyChordPressed(c)) {
            ausloesen(a);
            return true;
        }
    }
    return false;
}

bool modellMalenModal() { return mz().modalArt != 0 || mz().modalEndeBild == ImGui::GetFrameCount(); }

// Die Farbe unter der Maus aufnehmen (nach S): ein Klick in die Ansicht
bool modellMalenAufnehmen(const ModellAnsicht& a, ImVec2 maus) {
    ModellMalen& z = mz();
    if (!z.aufnehmen) { return false; }
    z.aufnehmen = false;
    Treffer trf;
    if (!treffer(a, maus, trf)) { return true; }
    const std::vector<Gruppe> g = gruppen();
    const int gw = gruppeVonTeil(g, trf.teil);
    if (gw < 0) { return true; }
    z.gruppe = gw;
    MalBild* m = jetzt(true);
    if (m == nullptr) { return true; }
    float ort[3];
    float uv[2];
    if (ruheOrt(*m, trf, ort, uv) < 0) { return true; }
    const int x = std::clamp(static_cast<int>((uv[0] - std::floor(uv[0])) * static_cast<float>(m->l.b)), 0, m->l.b - 1);
    const int y = std::clamp(static_cast<int>((uv[1] - std::floor(uv[1])) * static_cast<float>(m->l.h)), 0, m->l.h - 1);
    for (int k = 0; k < 4; ++k) { z.pinsel.farbe[uz(k)] = static_cast<float>(m->l.bild[m->l.texel(x, y) * 4U + uz(k)]) / 255.0F; }
    return true;
}

// --- Rollouts im Befehlsfeld -------------------------------------------------------------------------------------------

void drawModellMalenBereich() {
    ModellMalen& z = mz();
    bilderPruefen();
    // neu geladene Texturen der Vorschau (Haut, Neu laden): die bemalten Bilder wieder in ihre Stufen
    for (auto& b : g_bilder) {
        if (b->l.bemalt() && b->bindungenFuer != g_app->modelTextures.stufen.data()) {
            b->l.schmutzig = mm::Rechteck{0, 0, b->l.b - 1, b->l.h - 1};
            anzeigeNachfuehren(*b);
        }
    }
    const float fs = ImGui::GetFontSize();
    const std::vector<Gruppe> g = gruppen();
    MalBild* m = jetzt(true);
    if (ImGui::CollapsingHeader(tr(blenderModus() ? Str::PtRolloutBlender : Str::PtRolloutMax), ImGuiTreeNodeFlags_DefaultOpen)) {
        // Werkzeuge
        const Str kWz[mm::kWerkzeuge] = {Str::PtWzPinsel, Str::PtWzKlonen, Str::PtWzWischen, Str::PtWzWeich, Str::PtWzFuellen, Str::PtWzMaske, Str::PtWzRadierer};
        const float rechts = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (int i = 0; i < mm::kWerkzeuge; ++i) {
            const float b = ImGui::CalcTextSize(tr(kWz[i])).x + ImGui::GetStyle().FramePadding.x * 2.0F;
            if (i > 0) {
                ImGui::SameLine(0.0F, 2.0F);
                if (ImGui::GetCursorScreenPos().x + b > rechts) { ImGui::NewLine(); }
            }
            knopf(kWz[i]);
        }
        // Pinsel
        ImGui::SetNextItemWidth(-fs * 0.5F);
        ImGui::SliderFloat("##ptradius", &z.radiusPx, 1.0F, 300.0F, tr(Str::PtRadiusFmt), ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("%s", tooltipVon(Str::PtRadiusZiehen).c_str());
        ImGui::SetNextItemWidth(-fs * 0.5F);
        ImGui::SliderFloat("##ptstaerke", &z.pinsel.staerke, 0.0F, 1.0F, tr(Str::PtStaerkeFmt));
        ImGui::SetItemTooltip("%s", tooltipVon(Str::PtStaerkeZiehen).c_str());
        ImGui::SetNextItemWidth(-fs * 0.5F);
        ImGui::SliderFloat("##pthaerte", &z.pinsel.haerte, 0.0F, 1.0F, tr(Str::PtHaerteFmt));
        ImGui::SetItemTooltip("%s", tr(Str::PtHaerteHint));
        ImGui::SetNextItemWidth(-fs * 0.5F);
        float abst = z.pinsel.abstand * 100.0F;
        if (ImGui::SliderFloat("##ptabstand", &abst, 1.0F, 200.0F, tr(Str::PtAbstandFmt))) { z.pinsel.abstand = abst / 100.0F; }
        ImGui::SetItemTooltip("%s", tr(Str::PtAbstandHint));
        if (z.werkzeug == static_cast<int>(mm::Werkzeug::Fuellen)) {
            ImGui::SetNextItemWidth(-fs * 0.5F);
            ImGui::SliderFloat("##pttol", &z.pinsel.fuellToleranz, 0.0F, 1.0F, tr(Str::PtToleranzFmt));
            ImGui::SetItemTooltip("%s", tr(Str::PtToleranzHint));
        }
        if (z.werkzeug == static_cast<int>(mm::Werkzeug::Maske)) {
            ImGui::Checkbox(tr(Str::PtMaskeFrei), &z.pinsel.maskeLoeschen);
            ImGui::SetItemTooltip("%s", tr(Str::PtMaskeFreiHint));
        }
        // Farbe
        ImGui::ColorEdit4("##ptfarbe", z.pinsel.farbe.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", tr(Str::PtFarbeHint));
        ImGui::SameLine();
        ImGui::ColorEdit4("##ptfarbe2", z.farbe2.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", tr(Str::PtFarbe2Hint));
        ImGui::SameLine();
        knopf(Str::PtFarbenTauschen);
        ImGui::SameLine();
        knopf(Str::PtFarbeAufnehmen);
        // Spiegeln, Druck, Rueckseiten
        bool sp = z.spiegeln;
        if (ImGui::Checkbox(tr(Str::PtSpiegeln), &sp)) { z.spiegeln = sp; }
        ImGui::SetItemTooltip("%s", tooltipVon(Str::PtSpiegeln).c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(fs * 3.5F);
        ImGui::Combo("##ptachse", &z.spiegelAchse, "X\0Y\0Z\0");
        ImGui::SetItemTooltip("%s", tr(Str::PtAchseHint));
        ImGui::Checkbox(tr(Str::PtDruckGroesse), &z.druckGroesse);
        ImGui::SetItemTooltip("%s", tr(Str::PtDruckGroesseHint));
        ImGui::SameLine();
        ImGui::Checkbox(tr(Str::PtDruckStaerke), &z.druckStaerke);
        ImGui::SetItemTooltip("%s", tr(Str::PtDruckStaerkeHint));
        ImGui::PushTextWrapPos(0.0F);   // umbrechen statt am Rand abschneiden (alle Sprachen)
        cjkTextDisabled("%s", z.stiftDa ? format(Str::PtStiftDa, static_cast<double>(z.stiftDruck)).c_str() : tr(Str::PtStiftFehlt));
        ImGui::PopTextWrapPos();
        ImGui::Checkbox(tr(Str::PtRueckseiten), &z.pinsel.rueckseiten);
        ImGui::SetItemTooltip("%s", tr(Str::PtRueckseitenHint));
        ImGui::SetNextItemWidth(fs * 5.0F);
        if (ImGui::InputInt(tr(Str::PtAusbluten), &z.ausbluten)) {
            z.ausbluten = std::clamp(z.ausbluten, 0, 64);
            for (auto& b : g_bilder) {
                b->l.ausbluten = z.ausbluten;
                const auto laden = b->l.dreieckeBeimLaden;
                mm::dreieckeSetzen(b->l, b->l.dreiecke, false);
                b->l.dreieckeBeimLaden = laden;
            }
        }
        ImGui::SetItemTooltip("%s", tr(Str::PtAusblutenHint));
    }
    if (ImGui::CollapsingHeader(tr(Str::PtGrBild), ImGuiTreeNodeFlags_DefaultOpen)) {
        // Bild: Gruppe (Textur) und Stufe des Shaders
        if (!g.empty()) {
            const int gw = std::clamp(z.gruppe, 0, static_cast<int>(g.size()) - 1);
            ImGui::SetNextItemWidth(-fs * 0.5F);
            if (ImGui::BeginCombo("##ptgruppe", g[uz(gw)].shader.c_str())) {
                for (std::size_t i = 0; i < g.size(); ++i) {
                    if (ImGui::Selectable(g[i].shader.c_str(), static_cast<int>(i) == gw)) {
                        z.gruppe = static_cast<int>(i);
                        z.stufe = 0;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetItemTooltip("%s", tr(Str::PtGruppeHint));
            const std::vector<BildStufe>& st = bildStufen(g[uz(gw)].shader);
            if (st.size() > 1U) {
                ImGui::SetNextItemWidth(-fs * 0.5F);
                const int s = std::clamp(z.stufe, 0, static_cast<int>(st.size()) - 1);
                if (ImGui::BeginCombo("##ptstufe", format(Str::PtStufe, s + 1, st[uz(s)].name.c_str()).c_str())) {
                    for (std::size_t i = 0; i < st.size(); ++i) {
                        if (ImGui::Selectable(format(Str::PtStufe, static_cast<int>(i) + 1, st[i].name.c_str()).c_str(), static_cast<int>(i) == s)) { z.stufe = static_cast<int>(i); }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SetItemTooltip("%s", tr(Str::PtStufeHint));
            }
        }
        if (m != nullptr) {
            ImGui::PushTextWrapPos(0.0F);
            cjkTextDisabled("%s", format(Str::PtBildInfo, m->l.datei.c_str(), m->l.b, m->l.h, image::formatName(static_cast<image::Format>(m->l.format)),
                                             m->l.tgaBits > 0 ? m->l.tgaBits : (m->l.alpha ? 32 : 24))
                                          .c_str());
            ImGui::PopTextWrapPos();
        } else {
            ImGui::TextDisabled("%s", tr(Str::PtKeinBild));
        }
        knopf(Str::PtUmbacken);
        ImGui::SameLine();
        knopf(Str::PtEditorZeigen);
    }
    if (m != nullptr && ImGui::CollapsingHeader(tr(Str::PtGrEbenen), ImGuiTreeNodeFlags_DefaultOpen)) {
        // oben die oberste Ebene, wie Max und Blender
        for (int i = static_cast<int>(m->l.ebenen.size()) - 1; i >= 0; --i) {
            mm::Ebene& e = m->l.ebenen[uz(i)];
            ImGui::PushID(i);
            bool sicht = e.sichtbar;
            if (i > 0 && ImGui::Checkbox("##sicht", &sicht)) {
                stapelBefehl(Str::PtEbeneSicht, [i, sicht](std::vector<mm::Ebene>& st, int&) { st[uz(i)].sichtbar = sicht; });
            }
            if (i > 0) {
                ImGui::SetItemTooltip("%s", tr(Str::PtEbeneSichtHint));
                ImGui::SameLine();
            }
            if (ImGui::Selectable(e.name.c_str(), m->l.aktiv == i, 0, ImVec2{fs * 6.0F, 0.0F})) { m->l.aktiv = i; }
            ImGui::SetItemTooltip("%s", tr(i == 0 ? Str::PtEbeneGrundHint : Str::PtEbeneWahlHint));
            if (i > 0) {
                ImGui::SameLine();
                // so breit wie der laengste Name der Mischarten in dieser Sprache (cs "Normální" war abgeschnitten)
                float mb = 0.0F;
                for (const Str n : {Str::PtMischNormal, Str::PtMischMult, Str::PtMischAdd, Str::PtMischScreen, Str::PtMischOverlay}) { mb = std::max(mb, ImGui::CalcTextSize(tr(n)).x); }
                ImGui::SetNextItemWidth(mb + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0F);
                int mi = static_cast<int>(e.misch);
                const std::string arten = std::string(tr(Str::PtMischNormal)) + '\0' + tr(Str::PtMischMult) + '\0' + tr(Str::PtMischAdd) + '\0' + tr(Str::PtMischScreen) + '\0' +
                                          tr(Str::PtMischOverlay) + '\0' + '\0';
                if (ImGui::Combo("##misch", &mi, arten.c_str())) {
                    stapelBefehl(Str::PtEbeneMisch, [i, mi](std::vector<mm::Ebene>& st, int&) { st[uz(i)].misch = static_cast<mm::Mischart>(mi); });
                }
                ImGui::SetItemTooltip("%s", tr(Str::PtEbeneMischHint));
                ImGui::SameLine();
                // die Deckkraft daneben nur, wenn "100 %" und der Griff ganz hineinpassen - sonst in eine eigene Zeile
                if (ImGui::GetContentRegionAvail().x - fs * 0.5F < ImGui::CalcTextSize("100 %").x + ImGui::GetStyle().FramePadding.x * 2.0F + fs * 2.0F) {
                    ImGui::NewLine();
                }
                ImGui::SetNextItemWidth(-fs * 0.5F);
                float d = e.deckkraft * 100.0F;
                if (ImGui::SliderFloat("##deck", &d, 0.0F, 100.0F, "%.0f %%")) {
                    e.deckkraft = d / 100.0F;   // live; der Rueckgaengig-Schritt beim Loslassen
                    mm::zusammensetzen(m->l);
                    m->l.schmutzig = mm::Rechteck{0, 0, m->l.b - 1, m->l.h - 1};
                    anzeigeNachfuehren(*m);
                }
                if (ImGui::IsItemActivated()) { z.deckVorher = e.deckkraft; }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    const float neu = e.deckkraft;
                    e.deckkraft = z.deckVorher;
                    stapelBefehl(Str::PtEbeneDeck, [i, neu](std::vector<mm::Ebene>& st, int&) { st[uz(i)].deckkraft = neu; });
                }
                ImGui::SetItemTooltip("%s", tr(Str::PtEbeneDeckHint));
            }
            ImGui::PopID();
        }
        // die Knoepfe fliessen in Zeilen (wie die Werkzeuge oben) - keiner ragt ueber den Rand, auch nicht in langen Sprachen
        const Str kEbenenKnoepfe[] = {Str::PtEbeneNeu, Str::PtEbeneLoeschen, Str::PtEbeneHoch, Str::PtEbeneRunter, Str::PtEbeneVereinen, Str::PtMaskeLeeren, Str::PtMaskeUmkehren};
        for (std::size_t i = 0; i < std::size(kEbenenKnoepfe); ++i) {
            if (i > 0) {
                ImGui::SameLine(0.0F, 2.0F);
                if (ImGui::GetContentRegionAvail().x < ImGui::CalcTextSize(tr(kEbenenKnoepfe[i])).x + ImGui::GetStyle().FramePadding.x * 2.0F) { ImGui::NewLine(); }
            }
            knopf(kEbenenKnoepfe[i]);
        }
    }
    if (ImGui::CollapsingHeader(tr(Str::PtGrAusgabe), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(-fs * 2.5F);
        ImGui::InputTextWithHint("##ptordner", ausgabeOrdner().c_str(), z.ordner, sizeof z.ordner);
        ImGui::SetItemTooltip("%s", tr(Str::PtOrdnerHint));
        ImGui::SameLine(0.0F, 2.0F);
        if (ImGui::Button("...##ptordnerwahl")) {
            const std::string o = platform::pickFolder(tr(Str::PtOrdner), ausgabeOrdner());
            if (!o.empty()) { std::snprintf(z.ordner, sizeof z.ordner, "%s", o.c_str()); }
        }
        ImGui::SetItemTooltip("%s", tr(Str::PtOrdnerHint));
        knopf(Str::PtBauen, -fs * 0.5F);
        int bemalt = 0;
        for (const auto& b : g_bilder) { bemalt += b->l.bild != b->l.grund ? 1 : 0; }
        ImGui::TextDisabled("%s", format(Str::PtBemaltZahl, bemalt).c_str());
    }
    if (!z.meldung.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", z.meldung.c_str());
        ImGui::PopTextWrapPos();
    }
}

// --- Das 2D-Bild links neben den Ansichten -------------------------------------------------------------------------------

float modellMalEditorBreite(float frei) {
    ModellMalen& z = mz();
    if (!modellMalenAktiv() || !z.editorOffen) { return 0.0F; }
    const float fs = ImGui::GetFontSize();
    if (z.breite <= 0.0F) { z.breite = frei * 0.42F; }
    z.breite = std::clamp(z.breite, fs * 12.0F, std::max(frei - fs * 12.0F, fs * 12.0F));
    return z.breite;
}

void drawModellMalEditor(ImVec2 groesse) {
    ModellTab& t = mt();
    ModellMalen& z = mz();
    z.hover = false;
    MalBild* m = jetzt(true);
    const float fs = ImGui::GetFontSize();
    ImGui::BeginChild("mtmaleditor", groesse, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextUnformatted(tr(blenderModus() ? Str::PtEditorBlender : Str::PtEditorMax));
    if (m != nullptr) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", format(Str::PtEditorKopf, fileName(m->l.datei).c_str(), m->l.b, m->l.h).c_str());
    }
    ImGui::SameLine();
    knopf(Str::PtUvZeigen);
    ImGui::SameLine(0.0F, 2.0F);
    knopf(Str::PtMaskeAn);
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    const ImVec2 gr{std::max(ImGui::GetContentRegionAvail().x, 32.0F), std::max(ImGui::GetContentRegionAvail().y, 32.0F)};
    ImGui::InvisibleButton("##ptleinwand", gr, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hover = ImGui::IsItemHovered();
    z.hover = hover;
    z.leinwand[0] = lo.x;
    z.leinwand[1] = lo.y;
    z.leinwand[2] = gr.x;
    z.leinwand[3] = gr.y;
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right) || ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
        t.klickDrinBild = ImGui::GetFrameCount();
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(lo, ImVec2{lo.x + gr.x, lo.y + gr.y}, true);
    dl->AddRectFilled(lo, ImVec2{lo.x + gr.x, lo.y + gr.y}, IM_COL32(38, 38, 42, 255));
    if (m == nullptr) {
        dl->AddText(ImVec2{lo.x + fs, lo.y + fs}, IM_COL32(200, 200, 200, 255), tr(Str::PtKeinBild));
        dl->PopClipRect();
        ImGui::EndChild();
        return;
    }
    // Ansicht: das Bild so gross wie moeglich (Seitenverhaeltnis des Bilds), Rad zoomt, Mitte schiebt
    const ImGuiIO& io = ImGui::GetIO();
    if (hover && io.MouseWheel != 0.0F) { z.zoom = std::clamp(z.zoom * std::pow(1.15F, io.MouseWheel), 0.1F, 64.0F); }
    if (hover && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        z.versatz[0] += io.MouseDelta.x;
        z.versatz[1] += io.MouseDelta.y;
    }
    const float seiten = static_cast<float>(m->l.b) / static_cast<float>(m->l.h);
    float bw = gr.x;
    float bh = bw / seiten;
    if (bh > gr.y) {
        bh = gr.y;
        bw = bh * seiten;
    }
    bw *= z.zoom;
    bh *= z.zoom;
    const ImVec2 k0{lo.x + (gr.x - bw) * 0.5F + z.versatz[0], lo.y + (gr.y - bh) * 0.5F + z.versatz[1]};
    const ImVec2 k1{k0.x + bw, k0.y + bh};
    // Bild auf der Grafikkarte, ein Platz je Groesse (render::createTexture hat kein Freigeben einzeln, wie der UV-Editor):
    // neu ganz, wenn ein anderes Bild den Platz hatte, sonst nur das geaenderte Rechteck
    struct Platz {
        void* id = nullptr;
        unsigned stand = 0;
        const MalBild* fuer = nullptr;
        int generation = -1;
    };
    static std::map<std::pair<int, int>, Platz> plaetze;
    Platz& pl = plaetze[{m->l.b, m->l.h}];
    if (pl.id == nullptr || pl.stand != render::texturStand()) {
        pl.id = render::createTexture(m->l.bild.data(), m->l.b, m->l.h);
        pl.stand = render::texturStand();
        pl.fuer = m;
        pl.generation = g_bilderGeneration;
        m->texSchmutzig = mm::Rechteck{};
    } else if (pl.fuer != m || pl.generation != g_bilderGeneration) {
        render::updateTexture(pl.id, m->l.bild.data(), m->l.b, m->l.h);
        pl.fuer = m;
        pl.generation = g_bilderGeneration;
        m->texSchmutzig = mm::Rechteck{};
    } else if (!m->texSchmutzig.leer()) {
        render::updateTextureBereich(pl.id, m->l.bild.data(), m->l.b, m->l.h, m->texSchmutzig.x0, m->texSchmutzig.y0, m->texSchmutzig.x1 + 1, m->texSchmutzig.y1 + 1);
        m->texSchmutzig = mm::Rechteck{};
    }
    dl->AddImage(reinterpret_cast<ImTextureID>(pl.id), k0, k1);
    const auto nachSchirm2 = [&](float u, float v) { return ImVec2{k0.x + u * bw, k0.y + v * bh}; };
    if (z.uvZeigen) {
        int gez = 0;
        for (const mm::Dreieck& d : m->l.dreiecke) {
            const ImVec2 a = nachSchirm2(d.uv[0][0], d.uv[0][1]);
            const ImVec2 b = nachSchirm2(d.uv[1][0], d.uv[1][1]);
            const ImVec2 c = nachSchirm2(d.uv[2][0], d.uv[2][1]);
            dl->AddTriangle(a, b, c, IM_COL32(255, 255, 255, 60), 1.0F);
            ++gez;
        }
        z.dreieckeGezeichnet = gez;
    }
    if (z.maskeZeigen) {
        // die Maske als rote Flaeche (geschuetzt): Zeilen zu Streifen zusammengefasst
        for (int y = 0; y < m->l.h; ++y) {
            int x = 0;
            while (x < m->l.b) {
                if (m->l.maske[m->l.texel(x, y)] < 128) {
                    ++x;
                    continue;
                }
                const int x0 = x;
                while (x < m->l.b && m->l.maske[m->l.texel(x, y)] >= 128) { ++x; }
                const float y0f = static_cast<float>(y) / static_cast<float>(m->l.h);
                const float y1f = static_cast<float>(y + 1) / static_cast<float>(m->l.h);
                dl->AddRectFilled(nachSchirm2(static_cast<float>(x0) / static_cast<float>(m->l.b), y0f), nachSchirm2(static_cast<float>(x) / static_cast<float>(m->l.b), y1f),
                                  IM_COL32(230, 40, 40, 90));
            }
        }
    }
    // Malen in 2D: Radius in Bildpunkten des Schirms -> Texel
    const float texelJePixel = static_cast<float>(m->l.b) / bw;
    if (hover) { dl->AddCircle(io.MousePos, z.radiusPx, IM_COL32(255, 255, 255, 220), 48, 1.5F); }
    const float u = (io.MousePos.x - k0.x) / bw;
    const float v = (io.MousePos.y - k0.y) / bh;
    if (hover && z.modalArt == 0 && z.modalEndeBild != ImGui::GetFrameCount() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (z.aufnehmen) {
            z.aufnehmen = false;
            const int x = std::clamp(static_cast<int>(u * static_cast<float>(m->l.b)), 0, m->l.b - 1);
            const int y = std::clamp(static_cast<int>(v * static_cast<float>(m->l.h)), 0, m->l.h - 1);
            for (int k = 0; k < 4; ++k) { z.pinsel.farbe[uz(k)] = static_cast<float>(m->l.bild[m->l.texel(x, y) * 4U + uz(k)]) / 255.0F; }
        } else if (io.KeyCtrl && z.werkzeug == static_cast<int>(mm::Werkzeug::Klonen)) {
            z.klonQuelle[0] = u;
            z.klonQuelle[1] = v;
            z.klonQuelleDa = true;
            melden(format(Str::PtKlonQuelle, static_cast<double>(u), static_cast<double>(v)));
        } else if (z.werkzeug == static_cast<int>(mm::Werkzeug::Fuellen)) {
            mm::schrittBeginnen(m->v, m->l, tr(Str::PtUndoFuellen));
            z.texelGemalt = mm::fuellen(m->l, m->v, pinselJetzt(1.0F), static_cast<int>(std::floor(u * static_cast<float>(m->l.b))), static_cast<int>(std::floor(v * static_cast<float>(m->l.h))));
            mm::schrittEnde(m->v, m->l);
            ++z.befehle;
        } else {
            strichBeginnen(*m);
            z.strich2d = true;
            z.letzteUv[0] = u;
            z.letzteUv[1] = v;
        }
    }
    if (z.strichLaeuft && z.strich2d && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const mm::Pinsel p = pinselJetzt(z.radiusPx * texelJePixel);
        if (p.werkzeug == mm::Werkzeug::Klonen) {
            const float uv[2] = {u, v};
            klonVersatzSetzen(uv);
        }
        const float pos[3] = {u * static_cast<float>(m->l.b), v * static_cast<float>(m->l.h), 0.0F};
        int n = 0;
        for (const std::array<float, 3>& s : z.strich.weiter(pos, std::max(p.radius * std::max(p.abstand, 0.01F), 0.25F))) {
            const float su = s[0] / static_cast<float>(m->l.b);
            const float sv = s[1] / static_cast<float>(m->l.h);
            const float richtung[2] = {su - z.letzteUv[0], sv - z.letzteUv[1]};
            n += mm::tupfer2d(m->l, m->v, p, su, sv, druckJetzt(), richtung);
            z.letzteUv[0] = su;
            z.letzteUv[1] = sv;
            ++z.tupfer;
        }
        z.texelGemalt += n;
    }
    if (z.strichLaeuft && z.strich2d && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        z.strich2d = false;
        modellMalenEnde();
    }
    anzeigeNachfuehren(*m);
    dl->PopClipRect();
    ImGui::EndChild();
}

}  // namespace bhed::gui
