// gui/app_modelltab_malen.cpp - der Bereich Painting des Model-Tabs (Modell-Block 1c): Malen auf die Bilder hinter den
// JKA-Shadern in der 3D-Ansicht und im 2D-Bild, Werkzeuge, Ebenen, Maske, Spiegeln, Ausbluten, Umbacken, "Fuer das Spiel
// bauen". Die Rechnungen stehen in bhed/modell/malen.h (neu geschrieben; WhiteoutFlakes hat kein Texture Painting).
//
// Vorbilder der Bedienung: 3ds Max Viewport Canvas (Malen in der Ansicht, Ebenen, Pinselgroesse/-staerke, Spiegeln) und
// Blenders Arbeitsbereich Texture Paint (Werkzeuge Draw, Soften, Smear, Clone, Fill, Mask; F Radius, Umschalt+F Staerke,
// S Farbe aufnehmen, X Farben tauschen laut Blender-Handbuch "Texture Paint", 1 Malmaske laut blender_keys.txt).
// AP9 (rc612): das rechte Feld in der Form des freigegebenen Entwurfs - Gruppen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das
// Spiel ueber den Baustein gui/ui_rollout.h; die Tasten [ und ] fuer die Pinselgroesse.
#include "app_internal.h"
#include "ui_menue.h"   // KLEIN2: ui::menue - Menues bleiben im Fenster
#include "backend.h"
#include "gpumap.h"
#include "textumbruch.h"

#include "bhed/image.h"
#include "bhed/modell/malen.h"
#include "bhed/modell/malen_mess.h"
#include "bhed/shaderscript.h"
#include "bhed/sicherschreiben.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <array>
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

// PTFIX3 P29: der Name, unter dem das Spiel ein Bild fuehrt (GenerateImageMappingName, rd-vanilla tr_image.cpp:95-111 OpenJK,
// MD9 :92-107): klein, '\' -> '/', bis zum ersten Punkt, hoechstens MAX_QPATH - 1 = 63 Zeichen. R_FindImageFile_NoLoad sucht
// danach - "torso.tga" und "torso.jpg" sind im Spiel EIN Bild, das zuerst geladene (bis hier eine Leinwand je Datei).
std::string bildSchluessel(const std::string& name) {
    std::string s;
    for (std::size_t i = 0; i < name.size() && i < 63U; ++i) {
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(name[i])));
        if (c == '.') { break; }
        if (c == '\\') { c = '/'; }
        s.push_back(c);
    }
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
    std::string name;         // Name im Shader (Anzeige)
    std::string datei;        // gefundene Datei (VFS)
    std::string schluessel;   // P29: der Name ohne Endung, unter dem das Spiel das Bild fuehrt (bildSchluessel)
    bool grundUv = true;      // P30: eine Stufe zeichnet das Bild mit der UV der Flaeche (tcGen base, kein tcMod)
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

// die Datei, die die Engine fuer einen Bildnamen laedt (R_FindImageFile -> R_LoadImage: die Endungen der Reihe nach, P29)
const std::string& dateiVon(const std::string& name) {
    merkerPruefen();
    const auto it = g_stufen.datei.find(name);
    if (it != g_stufen.datei.end()) { return it->second; }
    std::string gefunden;
    // PTFIX3 P29 / MERGE2: die Dateien in der Folge von R_LoadImage (textureCandidates)
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

// P30: die Stufe zeichnet ihr Bild mit der UV der Flaeche - tcGen base und keine tcMod-Kette (ComputeTexCoords, tr_shade.cpp:
// tcGen environment/vector und jeder tcMod bilden die Ecken anders auf das Bild ab, der Pinsel traefe andere Inseln)
bool stufeMitGrundUv(const ShaderInfo::Stage& st) {
    return st.texGen == TexGen::Base && st.numTexMods == 0;
}

// [0] das Bild der Flaeche (Haut/Shader direkt, sonst die erste map-Stufe des gleichnamigen Skripts wie texturesFor in
// app_view3d.cpp), danach die weiteren Bildstufen des Skripts (ohne $lightmap/$whiteimage) - ParseStage in tr_shader.cpp.
// PTFIX3: zusammengefasst nach dem Namen ohne Endung, wie das Spiel jedes Bild einmal fuehrt (P29); grundUv (P30) fuer [0] aus
// den Stufen des Skripts, wenn es eines gibt (R_FindShader nimmt es vor dem Bild), sonst wahr (Shader aus dem Bild: eine
// Stufe mit tcGen base).
const std::vector<BildStufe>& bildStufen(const std::string& shader) {
    merkerPruefen();
    if (const auto it = g_stufen.je.find(shader); it != g_stufen.je.end()) { return it->second; }
    std::vector<BildStufe> out;
    const std::string direkt = dateiVon(shader);
    const ShaderInfo* info = shaderInfoVon(shader);
    const auto grundIn = [info](const std::string& schl) {
        return info == nullptr || std::any_of(info->stages.begin(), info->stages.end(), [&](const ShaderInfo::Stage& st) {
                   return !st.lightmap && !st.image.empty() && bildSchluessel(st.image) == schl && stufeMitGrundUv(st);
               });
    };
    if (!direkt.empty()) {
        const std::string schl = bildSchluessel(shader);
        out.push_back(BildStufe{shader, direkt, schl, grundIn(schl)});
    } else if (info != nullptr && !info->image.empty()) {
        const std::string d = dateiVon(info->image);
        const std::string schl = bildSchluessel(info->image);
        if (!d.empty()) { out.push_back(BildStufe{info->image, d, schl, grundIn(schl)}); }
    }
    if (info != nullptr) {
        for (const ShaderInfo::Stage& st : info->stages) {
            if (st.lightmap || st.image.empty()) { continue; }
            const std::string d = dateiVon(st.image);
            if (d.empty()) { continue; }
            const std::string schl = bildSchluessel(st.image);
            const auto da = std::find_if(out.begin(), out.end(), [&](const BildStufe& b) { return b.schluessel == schl; });
            if (da != out.end()) {
                da->grundUv = da->grundUv || stufeMitGrundUv(st);
                continue;
            }
            out.push_back(BildStufe{st.image, d, schl, stufeMitGrundUv(st)});
        }
    }
    return g_stufen.je.emplace(shader, std::move(out)).first->second;
}

// P29: die Bildnamen eines Shadernamens in der Folge, in der das Spiel sie laedt - R_FindShader nimmt das Skript (gesucht ohne
// Endung), dessen Stufen je R_FindImageFile rufen (ParseStage; animMap jedes Bild der Folge), sonst das Bild des Namens
std::vector<std::string> registrierteBilder(const std::string& shader) {
    const ShaderInfo* info = shaderInfoVon(shader);
    if (info == nullptr) { return {shader}; }
    std::vector<std::string> out;
    for (const ShaderInfo::Stage& st : info->stages) {
        if (st.lightmap || st.weiss) { continue; }
        if (!st.animImages.empty()) {
            out.insert(out.end(), st.animImages.begin(), st.animImages.end());
        } else if (!st.image.empty()) {
            out.push_back(st.image);
        }
    }
    return out;
}

// P29: die Datei des Bilds `schl` wie im Spiel - die der ersten Registrierung, die ein Bild laedt: R_LoadMDXM die Shader der
// Flaechen in ihrer Folge (tr_ghoul2.cpp:5404-5416 MD9, "[nomaterial]" geleert), danach RE_RegisterSkin die Zeilen der Haut in
// der Folge der Datei (tr_skin.cpp:290-337, GlmSurface::hautZeile). Laedt ein Name nichts, legt R_FindImageFile kein Bild an.
// Leer: keine Registrierung laedt es (dann die Datei der Stufe).
std::string dateiFuerSchluessel(const std::string& schl) {
    std::vector<std::string> namen;
    for (const GlmSurface& f : g_app->model.surfaces) {
        if (!f.shader.empty() && f.shader[0] != '[') { namen.push_back(f.shader); }
    }
    std::vector<const GlmSurface*> haut;
    for (const GlmSurface& f : g_app->model.surfaces) {
        if (!f.texture.empty() && f.hautZeile >= 0) { haut.push_back(&f); }
    }
    std::stable_sort(haut.begin(), haut.end(), [](const GlmSurface* a, const GlmSurface* b) { return a->hautZeile < b->hautZeile; });
    for (const GlmSurface* f : haut) { namen.push_back(f->texture); }
    for (const std::string& n : namen) {
        for (const std::string& b : registrierteBilder(n)) {
            if (bildSchluessel(b) != schl) { continue; }
            if (const std::string& d = dateiVon(b); !d.empty()) { return d; }
        }
    }
    return {};
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

// P24: die Dreiecke einer Detailstufe, als die Leinwand sie das erste Mal zeigte - "Auf neue UV umbacken" paart je Index
// nur mit denselben Dreiecken derselben Stufe (herkunft gleich)
struct Ladestand {
    std::vector<mm::Dreieck> d;
    std::vector<std::array<int, 2>> herkunft;
};

struct MalBild {
    mm::Leinwand l;
    mm::Verlauf v;
    std::string schluessel;               // P29: der Name ohne Endung - eine Leinwand je Bild, wie das Spiel jedes Bild einmal fuehrt
    std::vector<int> teile;               // P30: die Flaechen des Pinsels - Shader mit dem Bild in einer Stufe mit Grund-UV
    std::vector<int> anzeigeTeile;        // alle Flaechen, deren Shader das Bild in einer Stufe zeichnet (Vorschau)
    std::map<int, Ladestand> beimLaden;   // P24: je Detailstufe
    std::vector<std::array<int, 2>> herkunft;   // je Dreieck der Leinwand: Teil, Dreieck des Netzes
    std::vector<std::array<float, 3>> normalen; // je Dreieck in der Ruhelage
    int netzStand = -1;
    int lod = -1;                         // P1: Detailstufe der Dreiecke
    int bemaltFuer = -1;                  // P6: l.stand des letzten Vergleichs mit dem Original
    bool bemaltAnders = false;
    std::vector<mm::Rechteck> texSchmutzig;   // fuer die 2D-Ansicht noch hochzuladen (MBMAL: die geaenderten Flaechen statt ihrer Huelle)
    std::vector<StufenBindung> bindungen; // die Stufen der Vorschau mit diesem Bild
    std::uint64_t bindungenFuer = 0;      // P31: gesucht fuer diese Texturen (ModelTextures::ladeNummer, 0: neu suchen)
    int maskeFuer = -1;                   // P8: l.maskeStand der gemerkten Maskenflaechen (-1: neu bestimmen)
    std::vector<std::array<int, 4>> maskeFlaechen;   // P8: maskierte Texel als Rechtecke x0, y0, x1, y1 (x1, y1 ausschliesslich)
};

std::vector<std::unique_ptr<MalBild>> g_bilder;
int g_bilderLadung = -1;      // P23: der Ladestand des Modelldokuments, zu dem die Leinwaende gehoeren (Dokument::ladeStand)
int g_bilderLod = -1;         // P1: die Detailstufe, deren Dreiecke die Leinwaende tragen
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
    mm::dreieckeSetzen(m.l, d, false);
    // P24: der Ladestand je Stufe (das erste Zeigen); passen die Dreiecke nicht mehr dazu (Detach, neuer Teil), bleibt nichts
    // zum Umbacken - vorher galt fuer alle Stufen der Stand der ersten Anzeige, und Umbacken paarte Dreiecke verschiedener Stufen
    const auto lst = m.beimLaden.try_emplace(stufe, Ladestand{d, m.herkunft}).first;
    m.l.dreieckeBeimLaden = lst->second.herkunft == m.herkunft ? lst->second.d : std::vector<mm::Dreieck>{};
    m.netzStand = in.netzStand;
    m.lod = stufe;
}

// Pruefung seit rc610 P1: die Leinwaende gehoeren zum Modell, nicht zur Detailstufe oder Haut - eine Leinwand haengt an ihrem
// Bild, nur ihre Dreiecke an der Stufe (beim Wechsel neu wie bei einem neuen Netz). Vorher leerte ein Sprung der automatischen
// Stufe (Zoomen) oder ein Hautwechsel alle Leinwaende samt Ebenen und Rueckgaengig ohne Frage.
// PTFIX3 P23: ein neu geladenes Modell verwirft sie, auch dieselbe Datei noch einmal (nach "verwerfen" oder einer Aenderung auf
// der Platte) - vorher verglich das den Pfad, die Leinwaende blieben, und die Frage kam bei jedem Laden wieder.
void bilderPruefen() {
    const int geladen = mt().dok.ladeStand();
    if (geladen != g_bilderLadung) {
        g_bilder.clear();
        g_stufen.je.clear();
        g_stufen.datei.clear();
        ++g_bilderGeneration;
        g_bilderLadung = geladen;
        mz().gruppe = -1;
        mz().stufe = 0;
    }
    if (g_bilderLod != mt().lodJetzt) {
        g_bilderLod = mt().lodJetzt;
        for (auto& b : g_bilder) { dreieckeAus(*b); }
    }
}

// Punkt 2 (MRESTE Teil 4): die Flaechen aller Gruppen, deren Shader dieses Bild in einer Stufe zeichnet. Das Spiel laedt jedes
// Bild einmal (R_FindImageFile sucht nach dem Namen, tr_image.cpp OpenJK) und jede Stufe, die es nennt, zeigt die bemalte Datei -
// bis hier gehoerte die Leinwand zu Shader und Datei: Aayla torso.tga (Shader torso und torso_cull) hatte zwei Leinwaende, die
// Flaechen des anderen Shaders zeigten das Original, und "Fuer das Spiel bauen" schrieb die Datei zweimal (die letzte gewann).
// PTFIX3 P30: der Pinsel nur auf den Flaechen, deren Shader das Bild mit ihrer UV zeichnet (eine Stufe ohne tcGen/tcMod); zeichnen
// es alle nur verschoben (Rauch mit tcMod rotate), die Flaechen, deren eigenes Bild es ist. Die Vorschau bekommt alle.
struct BildTeile {
    std::vector<int> pinsel;
    std::vector<int> anzeige;
};

BildTeile teileMitBild(const std::vector<Gruppe>& g, const std::string& schl) {
    BildTeile t;
    std::vector<int> eigen;
    for (const Gruppe& x : g) {
        const std::vector<BildStufe>& st = bildStufen(x.shader);
        bool nennt = false;
        bool grund = false;
        for (const BildStufe& b : st) {
            if (b.schluessel != schl) { continue; }
            nennt = true;
            grund = grund || b.grundUv;
        }
        if (!nennt) { continue; }
        t.anzeige.insert(t.anzeige.end(), x.teile.begin(), x.teile.end());
        if (grund) { t.pinsel.insert(t.pinsel.end(), x.teile.begin(), x.teile.end()); }
        if (st.front().schluessel == schl) { eigen.insert(eigen.end(), x.teile.begin(), x.teile.end()); }
    }
    if (t.pinsel.empty()) { t.pinsel = std::move(eigen); }
    return t;
}

MalBild* bildFuer(int gruppe, int stufe, bool laden) {
    bilderPruefen();
    const std::vector<Gruppe> g = gruppen();
    if (gruppe < 0 || uz(gruppe) >= g.size()) { return nullptr; }
    const std::vector<BildStufe>& st = bildStufen(g[uz(gruppe)].shader);
    if (st.empty()) { return nullptr; }
    const int s = std::clamp(stufe, 0, static_cast<int>(st.size()) - 1);
    const std::string schl = st[uz(s)].schluessel;
    const std::string stufenDatei = st[uz(s)].datei;
    // P30: die Flaechen jedes Mal neu (Detach/Grundkoerper in einen neuen Teil, Hautwechsel: weitere Flaechen zeigen das Bild) -
    // vorher nur beim Anlegen, Tupfer auf den neuen Flaechen malten nichts
    BildTeile bt = teileMitBild(g, schl);
    for (auto& b : g_bilder) {
        if (b->schluessel == schl) {
            if (b->teile != bt.pinsel || b->anzeigeTeile != bt.anzeige) {
                b->teile = std::move(bt.pinsel);
                b->anzeigeTeile = std::move(bt.anzeige);
                b->bindungenFuer = 0;
                dreieckeAus(*b);
            } else if (b->netzStand != mt().dok.inhalt().netzStand || b->lod != mt().lodJetzt) {
                dreieckeAus(*b);
            }
            return b.get();
        }
    }
    if (!laden) { return nullptr; }
    // P29: die Datei der ersten Registrierung im Spiel (Yoda: Haut hair.tga, das Skript hair laedt hair.png)
    std::string datei = dateiFuerSchluessel(schl);
    if (datei.empty()) { datei = stufenDatei; }
    std::string bytes;
    if (!readFromArchives(datei, bytes)) { return nullptr; }
    auto m = std::make_unique<MalBild>();
    std::string fehler;
    if (!mm::leinwandAusDatei(m->l, datei, bytes, &fehler)) {
        melden(format(Str::PtBildFehler, datei.c_str(), fehler.empty() ? tr(Str::PtFeUnlesbar) : fehler.c_str()));
        return nullptr;
    }
    m->l.ausbluten = mz().ausbluten;
    m->schluessel = schl;
    m->teile = std::move(bt.pinsel);
    m->anzeigeTeile = std::move(bt.anzeige);
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
// MBMAL: `teile` sind die geaenderten Flaechen (mm::schmutzRechtecke) - erst die Bildpunkte aller Flaechen, dann je Flaeche
// die Grafikkarte (jede Stufe der Pyramide entsteht aus den fertigen Bildpunkten darunter).
void bildNachfuehren(TextureSet::Tex& x, const MalBild& m, const std::vector<mm::Rechteck>& teile) {
    if (x.width == m.l.b && x.height == m.l.h && x.rgba.size() == m.l.bild.size()) {
        for (const mm::Rechteck& r : teile) {
            for (int y = r.y0; y <= r.y1 && r.x0 <= r.x1; ++y) {
                const std::size_t i = m.l.texel(r.x0, y) * 4U;
                std::memcpy(&x.rgba[i], &m.l.bild[i], uz(r.x1 - r.x0 + 1) * 4U);
            }
        }
        bool hochgeladen = x.mips.empty();
        for (std::size_t i = 0; i < teile.size() && hochgeladen; ++i) { hochgeladen = gpu::figurBildBereich(&x, teile[i].x0, teile[i].y0, teile[i].x1, teile[i].y1); }
        if (hochgeladen) { return; }
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

// Die Stufen der Vorschau suchen, deren Bild diese Leinwand ist (je Flaeche, die das Bild zeigt: die Stufen des Shaders, deren
// Bild dasselbe Bild des Spiels ist - P29 nach dem Namen ohne Endung; animMap mit mehreren Bildern und $whiteimage nicht)
void bindungenSuchen(MalBild& m) {
    const auto& st = g_app->modelTextures.stufen;
    m.bindungen.clear();
    m.bindungenFuer = g_app->modelTextures.ladeNummer;
    for (const int t : m.anzeigeTeile) {
        if (t < 0 || uz(t) >= st.size() || st[uz(t)].empty()) { continue; }
        const ShaderInfo* info = shaderInfoVon(shaderVon(t));
        if (info == nullptr) { continue; }
        for (std::size_t k = 0; k < st[uz(t)].size(); ++k) {
            const ModelTextures::FigurStufe& fs = st[uz(t)][k];
            if (fs.art.weiss || fs.pixel.size() != 1U || fs.pixel[0] == nullptr || fs.art.stufe >= info->stages.size()) { continue; }
            if (bildSchluessel(info->stages[fs.art.stufe].image) == m.schluessel) { m.bindungen.push_back(StufenBindung{t, k, nullptr}); }
        }
    }
}

// Die Stufen der Vorschau dieser Leinwand nachfuehren; eine Stufe ohne eigene Kopie (das erste Mal, neu geladene
// Texturen) bekommt sie und das ganze Bild. true, wenn eine Stufe da ist.
bool stufenNachfuehren(MalBild& m, const std::vector<mm::Rechteck>& teile) {
    auto& st = g_app->modelTextures.stufen;
    if (m.bindungenFuer != g_app->modelTextures.ladeNummer) { bindungenSuchen(m); }   // P31: nach der Ladenummer, nicht der Adresse
    bool da = false;
    for (StufenBindung& b : m.bindungen) {
        if (uz(b.teil) >= st.size() || b.stufe >= st[uz(b.teil)].size()) { continue; }
        ModelTextures::FigurStufe& fs = st[uz(b.teil)][b.stufe];
        if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr) { continue; }
        if (fs.pixel[0].get() != b.eigen.get()) {
            b.eigen = std::make_shared<TextureSet::Tex>(*fs.pixel[0]);
            b.eigen->mips.clear();   // die Grafikkarte rechnet dieselbe Pyramide (ladeMitStufen/figurBildBereich wie buildMips)
            fs.pixel[0] = b.eigen;
            bildNachfuehren(*b.eigen, m, {mm::Rechteck{0, 0, m.l.b - 1, m.l.h - 1}});
        } else {
            bildNachfuehren(*b.eigen, m, teile);
        }
        da = true;
    }
    return da;
}

// Das Bild in die Vorschau: die Texturen der Flaechen der Gruppe (bySurface fuer das Bild der Flaeche, die Bilder der
// Shader-Stufen) und die 2D-Ansicht
void anzeigeNachfuehren(MalBild& m) {
    if (m.l.schmutzig.leer()) { return; }
    const mm::Stoppuhr uhr(mm::messung().hochladenMs);
    const auto ab = std::chrono::steady_clock::now();
    // MBMAL: die geaenderten Flaechen statt ihrer Huelle (ein Tupfer ueber eine UV-Naht: zwei kleine Flaechen statt des Rechtecks
    // um beide Inseln - bis zum ganzen Bild samt Pyramide, je Tupfer und je Textur der Vorschau)
    const std::vector<mm::Rechteck> teile = mm::schmutzRechtecke(m.l);
    // die 2D-Ansicht laedt sie beim naechsten Zeichnen; wird sie lange nicht gezeichnet, bleibt eine Huelle statt der Liste
    constexpr std::size_t kTeileHoechstens = 512;
    if (m.texSchmutzig.size() + teile.size() > kTeileHoechstens) {
        mm::Rechteck huelle = m.l.schmutzig;
        for (const mm::Rechteck& t : m.texSchmutzig) { huelle.dazu(t); }
        m.texSchmutzig.assign(1U, huelle);
    } else {
        m.texSchmutzig.insert(m.texSchmutzig.end(), teile.begin(), teile.end());
    }
    if (mm::messung().an) {
        for (const mm::Rechteck& r : teile) { mm::messung().hochTexel += static_cast<long long>(r.x1 - r.x0 + 1) * static_cast<long long>(r.y1 - r.y0 + 1); }
    }
    bool ansicht = stufenNachfuehren(m, teile);
    // das Bild der Flaeche (bySurface: Flaechen ohne Shader-Stufen) - je Flaeche, deren erstes Bild diese Datei ist
    auto& by = g_app->modelTextures.bySurface;
    for (const int t : m.anzeigeTeile) {
        if (t < 0 || uz(t) >= by.size()) { continue; }
        const std::vector<BildStufe>& st = bildStufen(shaderVon(t));
        if (st.empty() || st[0].schluessel != m.schluessel) { continue; }
        bildNachfuehren(by[uz(t)], m, teile);
        ansicht = true;
    }
    if (ansicht) {
        g_app->modelDirty = true;
        ++mz().ansichtStand;   // die 3D-Ansichten zeichnen im selben Bild neu (Stand in eineAnsicht)
    }
    mm::schmutzigLeeren(m.l);
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

// MBMAL Punkt 5: die Ecken der gezeichneten Flaechen in der Pose am Schirm einer Ansicht - einmal je Mausereignis statt je
// Strahl (ein Strich verteilt seine Tupfer am Schirm, jeder Tupfer ist ein Strahl gegen dasselbe Netz)
struct SchirmEcke {
    ImVec2 p;
    float w = 0.0F;
    float z = 0.0F;
    bool ok = false;
};
using SchirmNetz = std::vector<std::vector<SchirmEcke>>;   // je Flaeche der Haut (leer: nicht gezeichnet)

void schirmNetzBauen(const ModellAnsicht& a, SchirmNetz& n) {
    const ModellTab& t = mt();
    const auto& h = modellHaut();
    n.resize(h.size());
    for (std::size_t i = 0; i < h.size(); ++i) {
        std::vector<SchirmEcke>& e = n[i];
        e.clear();
        if (i >= t.anzeige.surfaces.size() || !modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        e.resize(h[i].size());
        for (std::size_t k = 0; k < h[i].size(); ++k) {
            const float* q = h[i][k].data();
            e[k].ok = nachSchirm(a, q, e[k].p, e[k].w);
            const float* m = a.viewProj;
            e[k].z = (m[8] * q[0] + m[9] * q[1] + m[10] * q[2] + m[11]) / std::max(e[k].w, 1e-3F);
        }
    }
}

// Strahl durch `maus` gegen das Netz der Pose: das vorderste Dreieck, wie es die Ansicht zeigt
bool treffer(const SchirmNetz& n, ImVec2 maus, Treffer& tr) {
    const ModellTab& t = mt();
    const auto& h = modellHaut();
    tr = Treffer{};
    if (mm::messung().an) { ++mm::messung().strahlen; }
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size() && i < n.size(); ++i) {
        const std::vector<SchirmEcke>& ecke = n[i];
        if (ecke.empty()) { continue; }
        const GlmSurface& s = t.anzeige.surfaces[i];
        for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
            const SchirmEcke& c0 = ecke[s.indexes[k]];
            const SchirmEcke& c1 = ecke[s.indexes[k + 1]];
            const SchirmEcke& c2 = ecke[s.indexes[k + 2]];
            if (!c0.ok || !c1.ok || !c2.ok) { continue; }
            // schnell verwerfen: ausserhalb des Rechtecks um das Dreieck (ein Bildpunkt Rand fuer die Rundung - ein Punkt mit
            // b0, b1, b2 >= 0 liegt darin, das Ergebnis ist dasselbe wie ohne)
            if (maus.x < std::min({c0.p.x, c1.p.x, c2.p.x}) - 1.0F || maus.x > std::max({c0.p.x, c1.p.x, c2.p.x}) + 1.0F ||
                maus.y < std::min({c0.p.y, c1.p.y, c2.p.y}) - 1.0F || maus.y > std::max({c0.p.y, c1.p.y, c2.p.y}) + 1.0F) {
                continue;
            }
            const ImVec2 p[3] = {c0.p, c1.p, c2.p};
            const float w[3] = {c0.w, c1.w, c2.w};
            const float z[3] = {c0.z, c1.z, c2.z};
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

bool treffer(const ModellAnsicht& a, ImVec2 maus, Treffer& tr) {
    SchirmNetz n;
    schirmNetzBauen(a, n);
    return treffer(n, maus, tr);
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
    m.v.offen.strich = ++z.strichNummer;   // TEST1411 O
    z.strichBilder = 1;
    z.strich = mm::Strich{};
    z.strichLaeuft = true;
    z.strich2d = false;   // MBMAL: der Strich gehoert der Ansicht, in der er beginnt (das 2D-Bild setzt es danach)
    z.uvDavor = false;
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

// MBMAL Punkt 5: die zugewandten Dreiecke (und gespiegelt) fuer das Dreieck des letzten Tupfers - neu nur, wenn ein Tupfer
// ein anderes Dreieck trifft
struct ZugewandtJe {
    int dreieck = -1;
    std::vector<std::uint8_t> zw;
    std::vector<std::uint8_t> zwGesp;
};

// MBMAL Punkt 5: ein Tupfer an einer getroffenen Stelle des Strichs. Der Treffer kommt in die Ruhelage (dort ist der Tupfer
// eine Kugel), der Radius aus den Bildpunkten des Pinsels an seiner Stelle der Pose, die zugewandten Dreiecke nach seiner
// Flaeche; Spiegeln wie eingestellt. erster: der Tupfer, mit dem der Strich beginnt. -1, wenn das Dreieck nicht zum Bild gehoert.
int tupferAn(MalBild& m, const ModellAnsicht& a, const Treffer& trf, bool erster, ZugewandtJe& zj) {
    ModellMalen& z = mz();
    mm::Messung& ms = mm::messung();
    float ort[3];
    float uv[2];
    const int di = ruheOrt(m, trf, ort, uv);
    if (di < 0) { return -1; }
    // Pinselgroesse in der Welt: Bildpunkte mal Welt je Bildpunkt am Treffpunkt der Pose (gleiche Einheiten wie die Ruhelage)
    const float radius = z.radiusPx * weltJePixel(a, trf.pose);
    const mm::Pinsel p = pinselJetzt(radius);
    if (p.werkzeug == mm::Werkzeug::Klonen) { klonVersatzSetzen(uv); }
    if (zj.dreieck != di) {
        zj.dreieck = di;
        zj.zw = zugewandt(m, m.normalen[uz(di)]);
        std::array<float, 3> nGesp = m.normalen[uz(di)];
        nGesp[uz(z.spiegelAchse)] = -nGesp[uz(z.spiegelAchse)];
        zj.zwGesp = z.spiegeln ? zugewandt(m, nGesp) : std::vector<std::uint8_t>{};
    }
    if (ms.an && !erster) {
        // der Weg in der Ruhelage seit dem vorigen Tupfer - ein Sprung zwischen Koerperteilen bekommt keine Tupfer dazwischen
        const double weg = std::hypot(ort[0] - z.letzterOrt[0], ort[1] - z.letzterOrt[1], ort[2] - z.letzterOrt[2]);
        ms.wegRuhe += weg;
        ms.ereignisseWeit += weg > 10.0 * static_cast<double>(radius) ? 1 : 0;
        if (weg > ms.wegRuheMax) {
            ms.wegRuheMax = weg;
            ms.sprungFlaechen[0] = ms.letzteFlaeche;
            ms.sprungFlaechen[1] = trf.teil;
            for (int k = 0; k < 3; ++k) {
                ms.sprungVon[k] = static_cast<double>(z.letzterOrt[k]);
                ms.sprungNach[k] = static_cast<double>(ort[k]);
            }
        }
    }
    ms.letzteFlaeche = trf.teil;
    // Verwischen: der Weg in UV seit dem vorigen Tupfer, wenn er dieses Bild traf (wie im 2D-Bild)
    const float richtung[2] = {uv[0] - z.letzteUv[0], uv[1] - z.letzteUv[1]};
    int n = mm::tupfer3d(m.l, m.v, p, ort, druckJetzt(), &zj.zw, z.uvDavor ? richtung : nullptr);
    if (z.spiegeln) {
        float sp[3];
        mm::spiegelPunkt(ort, z.spiegelAchse, sp);
        n += mm::tupfer3d(m.l, m.v, p, sp, druckJetzt(), &zj.zwGesp, nullptr);
    }
    ++z.tupfer;
    z.letzteUv[0] = uv[0];
    z.letzteUv[1] = uv[1];
    z.uvDavor = true;
    for (int k = 0; k < 3; ++k) { z.letzterOrt[k] = ort[k]; }
    return n;
}

SchirmNetz g_schirmNetz;   // je Mausereignis neu gefuellt (der Speicher bleibt)

// TEST1411 O: das Bild unter dem Pinsel - die Leinwand der Gruppe der Flaeche (Stufe 0: das Bild der Flaeche), je Strich gemerkt (ein
// Tupfer ueber dem Haar ohne Bild fragt nicht jedes Mal neu); nullptr ohne Bild
std::map<int, MalBild*> g_bildJeTeil;
long long g_bildJeTeilStrich = -1;

MalBild* bildUnter(int teil, int& gruppe) {
    ModellMalen& z = mz();
    if (g_bildJeTeilStrich != z.strichNummer) {
        g_bildJeTeil.clear();
        g_bildJeTeilStrich = z.strichNummer;
    }
    const std::vector<Gruppe> g = gruppen();
    gruppe = gruppeVonTeil(g, teil);
    const auto it = g_bildJeTeil.find(teil);
    if (it != g_bildJeTeil.end()) { return it->second; }
    MalBild* b = gruppe >= 0 ? bildFuer(gruppe, 0, true) : nullptr;
    g_bildJeTeil[teil] = b;
    return b;
}

// Ein weiteres Bild im laufenden Strich: sein Schritt traegt die Kennung des Strichs (Rueckgaengig nimmt alle zusammen)
void strichAuchAuf(MalBild& b) {
    ModellMalen& z = mz();
    if (b.v.laeuft) { return; }
    mm::schrittBeginnen(b.v, b.l, tr(Str::PtUndoStrich));
    b.v.offen.strich = z.strichNummer;
    ++z.strichBilder;
}

}  // namespace

// --- Malen in der 3D-Ansicht ------------------------------------------------------------------------------------------

bool modellMalenAktiv() { return mt().bereich == ModellBereich::Painting && !g_app->model.empty(); }

bool modellMalenAnsicht(const ModellAnsicht& a, ImVec2 maus, bool neu) {
    ModellMalen& z = mz();
    mm::Messung& ms = mm::messung();
    const mm::Stoppuhr ansichtUhr(ms.ansichtMs);
    if (!neu && (!z.strichLaeuft || z.strich2d)) {
        // MBMAL: ein Strich aus dem 2D-Bild malt hier nicht weiter. Bis hier rief die aktive Ansicht bei jeder Mausbewegung mit
        // gedrueckter Taste den Tupfer in 3D - lag die Maus am Schirm ueber einem Dreieck der Figur (auch ausserhalb der Ansicht,
        // oder der Strich lief aus dem 2D-Bild in die Ansicht), setzte Strich::weiter Tupfer auf die Gerade von der Stelle im Bild
        // (in Texeln) zum Treffpunkt (in der Ruhelage): zehntausende Tupfer in einem Bild und Streifen quer ueber das Bild.
        return false;
    }
    mm::Stoppuhr trefferUhr(ms.trefferMs);
    schirmNetzBauen(a, g_schirmNetz);
    // MBMAL Punkt 5: der Abstand der Tupfer gilt am Schirm (Spacing: Anteil des Pinselradius in Bildpunkten) - wie Substance
    // Painter und Blender (Stroke Spacing im Bildraum, je Tupfer eine Projektion auf die sichtbare Flaeche)
    // FIX612B P2-8: der Radius mindestens 1 Bildpunkt wie bei den Tasten [ ] und F - mit 0 (getippt) lagen auf 20 Bildpunkten
    // Mausweg 200 000 Tupferstellen, jede mit einem Strahl gegen alle Dreiecke
    const float schritt = std::max(std::clamp(z.radiusPx, 1.0F, 500.0F) * std::max(z.pinsel.abstand, 0.01F), 1e-4F);
    const float stelle[3] = {maus.x, maus.y, 0.0F};
    ZugewandtJe zj;
    if (!neu) {
        MalBild* m = jetzt(true);
        if (m == nullptr) { return false; }
        trefferUhr.halt();
        // MBMAL Punkt 5: die Tupfer liegen auf der Linie zwischen den Mauspositionen am Schirm, jeder mit einem eigenen Strahl
        // gegen das gezeigte Netz der Pose. Ueber dem Hintergrund oder einem anderen Bild malt ein Tupfer nichts. Bis hier
        // verband Strich::weiter die Treffer in der Ruhelage: sprang der Pinsel zwischen Koerperteilen (r_hand -> l_hand, in
        // der Pose nah, in der Ruhelage 42 Einheiten), lag dort eine Tupferlinie - Streifen, wo der Pinsel nie war.
        int n = 0;
        std::vector<MalBild*> bemalt;   // TEST1411 O: jedes Bild, in das dieses Mausereignis gemalt hat
        for (const std::array<float, 3>& st : z.strich.weiter(stelle, schritt)) {
            // ausserhalb der Ansicht zeigt sie keine Flaeche: dort malt der Tupfer nichts (der Strich laeuft weiter)
            const bool drin = st[0] >= a.lo[0] && st[1] >= a.lo[1] && st[0] <= a.lo[0] + a.groesse[0] && st[1] <= a.lo[1] + a.groesse[1];
            Treffer trf;
            mm::Stoppuhr uhr(ms.trefferMs);
            const bool hit = drin && treffer(g_schirmNetz, ImVec2{st[0], st[1]}, trf);
            uhr.halt();
            int k = hit ? tupferAn(*m, a, trf, false, zj) : -1;
            // TEST1411 O: trifft der Tupfer eine Flaeche mit einem anderen Bild, malt er dort weiter - wie Viewport Canvas mit
            // Multi/Sub-Object (je Teilmaterial sein Bild) und Substance Painter (ueber alle Texture Sets). Das Ziel folgt der
            // bemalten Flaeche (die 2D-Leinwand zeigt dasselbe Bild). "Nur Ziel-Textur bemalen" haelt den Strich wie bis rc612 im Ziel.
            if (k < 0 && hit && !z.nurZiel) {
                int gw = -1;
                MalBild* anderes = bildUnter(trf.teil, gw);
                if (anderes != nullptr && anderes != m) {
                    strichAuchAuf(*anderes);
                    m = anderes;
                    zj = ZugewandtJe{};   // die Dreiecke gehoeren zum anderen Bild
                    z.uvDavor = false;    // Verwischen beginnt auf dem neuen Bild neu
                    z.gruppe = gw;
                    z.stufe = 0;
                    k = tupferAn(*m, a, trf, false, zj);
                }
            }
            if (k < 0) {
                z.uvDavor = false;
                if (ms.an) { ++ms.tupferDaneben; }
                continue;
            }
            n += k;
            if (std::find(bemalt.begin(), bemalt.end(), m) == bemalt.end()) { bemalt.push_back(m); }
        }
        if (bemalt.empty()) { return false; }
        if (ms.an) { ++ms.ereignisse; }
        z.texelGemalt += n;
        for (MalBild* b : bemalt) { anzeigeNachfuehren(*b); }
        return true;
    }
    Treffer trf;
    if (!treffer(g_schirmNetz, maus, trf)) { return false; }
    // die Gruppe unter dem Pinsel wird die Leinwand (wie Blender das Bild des Materials unter dem Pinsel)
    const int gw = gruppeVonTeil(gruppen(), trf.teil);
    if (gw < 0) {
        melden(tr(Str::PtKeinBild));
        return false;
    }
    if (gw != z.gruppe) {
        z.gruppe = gw;
        z.stufe = 0;
    }
    MalBild* m = jetzt(true);
    if (m == nullptr) {
        melden(tr(Str::PtKeinBild));
        return false;
    }
    float ort[3];
    float uv[2];
    if (ruheOrt(*m, trf, ort, uv) < 0) { return false; }
    // Ctrl+Klick: Quelle fuer Klonen (Blender: Strg+Linksklick setzt die Quelle)
    if (ImGui::GetIO().KeyCtrl && z.werkzeug == static_cast<int>(mm::Werkzeug::Klonen)) {
        z.klonQuelle[0] = uv[0];
        z.klonQuelle[1] = uv[1];
        z.klonQuelleDa = true;
        melden(format(Str::PtKlonQuelle, static_cast<double>(uv[0]), static_cast<double>(uv[1])));
        return true;
    }
    const mm::Pinsel p = pinselJetzt(z.radiusPx * weltJePixel(a, trf.pose));
    if (p.werkzeug == mm::Werkzeug::Fuellen) {
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
    strichBeginnen(*m);
    if (p.werkzeug == mm::Werkzeug::Klonen && !z.klonQuelleDa) { melden(tr(Str::PtKlonOhneQuelle)); }
    trefferUhr.halt();
    (void)z.strich.weiter(stelle, schritt);   // der Strich beginnt an der Maus (der erste Tupfer ist dieser Treffer)
    const int n = tupferAn(*m, a, trf, true, zj);
    if (ms.an) { ++ms.ereignisse; }
    z.texelGemalt += std::max(n, 0);
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

// P1: ein bemaltes Bild, das noch nicht fuer das Spiel gebaut ist (Stand seit dem Laden oder dem letzten Bauen geaendert)
bool modellMalenUngespeichert() {
    return std::any_of(g_bilder.begin(), g_bilder.end(), [](const std::unique_ptr<MalBild>& b) { return b->l.stand != b->l.gespeichert; });
}

std::vector<std::pair<std::string, bool>> modellMalenBildStaende() {
    std::vector<std::pair<std::string, bool>> aus;
    for (const auto& b : g_bilder) { aus.emplace_back(b->l.datei, b->l.bild != b->l.grund); }
    return aus;
}

MalVerlaufStand modellMalenVerlaufStand() {
    MalVerlaufStand st;
    for (const auto& b : g_bilder) {
        for (const std::vector<mm::UndoSchritt>* liste : {&b->v.zurueck, &b->v.vor}) {
            for (const mm::UndoSchritt& x : *liste) {
                st.bytes += static_cast<long long>(mm::schrittBytes(x));
                st.kacheln += static_cast<int>(x.kacheln.size());
                ++st.schritte;
            }
        }
    }
    return st;
}

MalPfadPruefung modellMalenPfadPruefen(const ModellAnsicht& a, const std::vector<std::uint8_t>& vorher, const std::vector<ImVec2>& pfad, float grenzePx) {
    MalPfadPruefung r;
    const MalBild* m = jetzt(true);
    if (m == nullptr || pfad.empty() || vorher.size() != m->l.bild.size()) { return r; }
    const mm::Leinwand& l = m->l;
    const ModellTab& t = mt();
    const auto& h = modellHaut();
    const auto abstand = [&pfad](ImVec2 p) {
        float best = std::hypot(p.x - pfad[0].x, p.y - pfad[0].y);
        for (std::size_t k = 1; k < pfad.size(); ++k) {
            const ImVec2 a0 = pfad[k - 1];
            const ImVec2 d{pfad[k].x - a0.x, pfad[k].y - a0.y};
            const float ll = d.x * d.x + d.y * d.y;
            const float s = ll > 0.0F ? std::clamp(((p.x - a0.x) * d.x + (p.y - a0.y) * d.y) / ll, 0.0F, 1.0F) : 0.0F;
            best = std::min(best, std::hypot(p.x - (a0.x + d.x * s), p.y - (a0.y + d.y * s)));
        }
        return best;
    };
    const auto anders = [&](std::size_t i) { return std::memcmp(&l.bild[i * 4U], &vorher[i * 4U], 4U) != 0; };
    std::vector<float> naechst(uz(l.b) * uz(l.h), -1.0F);   // je Texel der kleinste Abstand zum Pfad (-1: in keinem Dreieck)
    std::vector<std::int32_t> flaeche(naechst.size(), -1);   // die Flaeche des naechsten
    for (std::size_t i = 0; i < l.dreiecke.size() && i < m->herkunft.size(); ++i) {
        const int teil = m->herkunft[i][0];
        const std::size_t k = uz(m->herkunft[i][1]) * 3U;
        if (teil < 0 || uz(teil) >= h.size() || uz(teil) >= t.anzeige.surfaces.size() || k + 2 >= t.anzeige.surfaces[uz(teil)].indexes.size()) { continue; }
        const auto& idx = t.anzeige.surfaces[uz(teil)].indexes;
        const std::array<float, 3>& q0 = h[uz(teil)][idx[k]];
        const std::array<float, 3>& q1 = h[uz(teil)][idx[k + 1]];
        const std::array<float, 3>& q2 = h[uz(teil)][idx[k + 2]];
        mm::texelJeDreieck(l.dreiecke[i], l.b, l.h, [&](int x, int y, float l0, float l1, float l2) {
            const std::size_t ti = l.texel(x, y);
            if (!anders(ti)) { return; }
            float p[3];
            for (std::size_t c = 0; c < 3; ++c) { p[c] = q0[c] * l0 + q1[c] * l1 + q2[c] * l2; }
            ImVec2 s;
            float w = 0.0F;
            if (!nachSchirm(a, p, s, w)) { return; }
            const float d = abstand(s);
            if (naechst[ti] < 0.0F || d < naechst[ti]) {
                naechst[ti] = d;
                flaeche[ti] = teil;
            }
        });
    }
    std::map<int, int> je;
    for (std::size_t ti = 0; ti < naechst.size(); ++ti) {
        if (!anders(ti)) { continue; }
        if (naechst[ti] < 0.0F) {
            ++r.ohneDreieck;
            continue;
        }
        ++r.geaendert;
        r.weitestePx = std::max(r.weitestePx, naechst[ti]);
        if (naechst[ti] > grenzePx) {
            ++r.ausserhalb;
            ++je[flaeche[ti]];
        }
    }
    const modell::Inhalt& in = t.dok.inhalt();
    for (const auto& [f, n] : je) {
        r.flaechenAusserhalb += (r.flaechenAusserhalb.empty() ? "" : ", ") + (f >= 0 && uz(f) < in.teile.size() ? in.teile[uz(f)].name : std::to_string(f)) + " " + std::to_string(n);
    }
    return r;
}

// P4: Strg+Z/Strg+Y gelten dem juengsten Schritt ueber alle Leinwaende (fortlaufende Nummer der Schritte), nicht nur dem
// gerade gewaehlten Bild - vorher nahm Strg+Z nach den Strichen des gewaehlten Bilds einen Schritt des Modelldokuments zurueck
bool modellMalenRueckgaengig() {
    // FIX612B P2-10: Strg+Z mitten im Strich beendet ihn zuerst - dann nimmt Rueckgaengig den ganzen Strich, und die gehaltene
    // Taste malt nicht weiter. Vorher lief der Strich nach dem Rueckgaengig weiter, ohne dass etwas gesichert wurde: was danach
    // gemalt wurde, brachte kein Rueckgaengig zurueck.
    modellMalenEnde();
    mz().strich2d = false;
    MalBild* m = nullptr;
    for (auto& b : g_bilder) {
        mm::schrittEnde(b->v, b->l);
        if (!b->v.zurueck.empty() && (m == nullptr || b->v.zurueck.back().nummer > m->v.zurueck.back().nummer)) { m = b.get(); }
    }
    if (m == nullptr) { return false; }
    // TEST1411 O: ein Strich ueber mehrere Bilder geht als Ganzes zurueck (jedes Bild, dessen juengster Schritt zu ihm gehoert)
    const long long strich = m->v.zurueck.back().strich;
    if (!mm::rueckgaengig(m->v, m->l)) { return false; }
    anzeigeNachfuehren(*m);
    for (auto& b : g_bilder) {
        if (strich != 0 && b.get() != m && !b->v.zurueck.empty() && b->v.zurueck.back().strich == strich && mm::rueckgaengig(b->v, b->l)) { anzeigeNachfuehren(*b); }
    }
    melden(tr(Str::PtRueckgaengig));
    return true;
}

// PTFIX3 P25: eine lineare Kette ueber alle Leinwaende - jede neue Handlung verwirft das Wiederholen auf jedem Bild (ein Schritt
// mit kleinerer Nummer als der juengste abgelegte wurde vor ihr zurueckgenommen); vorher leerte ein Strich nur `vor` der eigenen
// Leinwand, und Strg+Y brachte den Strich eines anderen Bilds zurueck.
bool modellMalenWiederholen() {
    modellMalenEnde();   // FIX612B P2-10: wie Rueckgaengig - ein laufender Strich endet zuerst
    mz().strich2d = false;
    long long juengster = 0;
    for (auto& b : g_bilder) {
        mm::schrittEnde(b->v, b->l);
        if (!b->v.zurueck.empty()) { juengster = std::max(juengster, b->v.zurueck.back().nummer); }
    }
    MalBild* m = nullptr;
    for (auto& b : g_bilder) {
        auto& vor = b->v.vor;
        vor.erase(std::remove_if(vor.begin(), vor.end(), [juengster](const mm::UndoSchritt& x) { return x.nummer < juengster; }), vor.end());
        if (!vor.empty() && (m == nullptr || vor.back().nummer < m->v.vor.back().nummer)) { m = b.get(); }
    }
    if (m == nullptr) { return false; }
    const long long strich = m->v.vor.back().strich;   // TEST1411 O: der ganze Strich
    if (!mm::wiederholen(m->v, m->l)) { return false; }
    anzeigeNachfuehren(*m);
    for (auto& b : g_bilder) {
        if (strich != 0 && b.get() != m && !b->v.vor.empty() && b->v.vor.back().strich == strich && mm::wiederholen(b->v, b->l)) { anzeigeNachfuehren(*b); }
    }
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
    ++m->l.maskeStand;   // P8: die Maske hat sich ohne Strich geaendert (die 2D-Ansicht liest sie neu)
    ++mz().befehle;
    mz().letzterBefehl = tr(name);
    melden(tr(name));
}

void umbackenBefehl() {
    MalBild* m = jetzt(true);
    if (m == nullptr) { return; }
    // P24: nur mit denselben Dreiecken derselben Stufe (vorher still nichts bei ungleicher Zahl, bei gleicher Zahl verzerrt)
    const auto lst = m->beimLaden.find(m->lod);
    if (lst == m->beimLaden.end() || lst->second.herkunft != m->herkunft) {
        melden(tr(Str::PtUmbackenAnders));
        return;
    }
    if (!mm::leinwandUmbacken(m->l, m->v, tr(Str::PtUmbacken))) {
        melden(tr(Str::PtUmbackenNichts));
        return;
    }
    lst->second.d = m->l.dreiecke;   // der neue Stand dieser Stufe (leinwandUmbacken setzt dreieckeBeimLaden)
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
            melden(format(Str::PtBauFehler, b->l.datei.c_str(), tr(Str::PtFeKeinBild)));
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

// AP9: ein Schritt der Pinselgroesse. Der Faktor ist der von Blenders brush.scale_size auf [ und ] (0,9 und 1 / 0,9 - ein
// Schritt hin und einer zurueck ergeben wieder denselben Radius); die Grenzen sind die des Ziehens mit F.
void pinselSchritt(bool groesser) {
    constexpr float kSchritt = 0.9F;
    ModellMalen& z = mz();
    z.radiusPx = std::clamp(groesser ? z.radiusPx / kSchritt : z.radiusPx * kSchritt, 1.0F, 500.0F);
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
        // AP9: die Groesse in Schritten - im Entwurf wie 3ds Max "[ ] Groesse" (dort zoomen [ und ] sonst die Ansicht: im Bereich
        // Painting gehoeren sie dem Pinsel); Blender-Handbuch "Brush Settings": "The size can be decreased/increased using [ and ]"
        {Str::PtPinselKleiner, Str::PtPinselKleinerHint, "[", "[", ImGuiKey_LeftBracket, ImGuiKey_LeftBracket, [] { pinselSchritt(false); }, {}, 1},
        {Str::PtPinselGroesser, Str::PtPinselGroesserHint, "]", "]", ImGuiKey_RightBracket, ImGuiKey_RightBracket, [] { pinselSchritt(true); }, {}, 1},
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
        // TEST1411 O: die Textur der Flaeche als Ziel (Pipette; Alt+Klick in die Ansicht) und der alte Weg nur ins Ziel
        {Str::T1oTexturAufnehmen, Str::T1oTexturAufnehmenHint, "", "", 0, 0, [] { mz().texturAufnehmen = !mz().texturAufnehmen; }, [] { return mz().texturAufnehmen; }, 4},
        {Str::T1oNurZiel, Str::T1oNurZielHint, "", "", 0, 0, [] { mz().nurZiel = !mz().nurZiel; }, [] { return mz().nurZiel; }, 4},
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
            unter = ui::menue(tr(kGruppen[uz(gruppe)]));
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
    if (ui::menue(tr(Str::PtMenue))) {
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
        if (modellAktionZeile(t.c_str(), static_cast<int>(ModellBereich::Painting)) && !ausgeloest) {
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
        // die Schritte der Pinselgroesse wiederholen sich, solange die Taste gehalten wird
        const bool wiederholt = a.text == Str::PtPinselKleiner || a.text == Str::PtPinselGroesser;
        if (c != 0 && ImGui::IsKeyChordPressed(c, wiederholt ? ImGuiInputFlags_Repeat : ImGuiInputFlags_None)) {
            ausloesen(a);
            return true;
        }
    }
    return false;
}

bool modellMalenModal() { return mz().modalArt != 0 || mz().modalEndeBild == ImGui::GetFrameCount(); }

// Die Farbe unter der Maus aufnehmen (nach S): ein Klick in die Ansicht
bool modellMalenZielAufnehmen(const ModellAnsicht& a, ImVec2 maus) {
    ModellMalen& z = mz();
    z.texturAufnehmen = false;
    Treffer trf;
    const int gw = treffer(a, maus, trf) ? gruppeVonTeil(gruppen(), trf.teil) : -1;
    if (gw < 0) {
        melden(tr(Str::PtKeinBild));
        return false;
    }
    z.gruppe = gw;
    z.stufe = 0;
    const MalBild* m = jetzt(true);
    melden(format(Str::T1oZiel, m != nullptr ? fileName(m->l.datei).c_str() : gruppen()[uz(gw)].shader.c_str()));
    return true;
}

bool modellMalenAufnehmen(const ModellAnsicht& a, ImVec2 maus) {
    ModellMalen& z = mz();
    if (z.texturAufnehmen) {   // TEST1411 O: die Pipette ist scharf - der Klick waehlt die Textur, er malt nicht
        (void)modellMalenZielAufnehmen(a, maus);
        return true;
    }
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

// P19 (Pruefung seit rc610): neu geladene Texturen der Vorschau (Haut, Neu laden, Detach/Grundkoerper im Bereich
// Modellieren): die bemalten Bilder wieder in ihre Stufen - in jedem Bereich, jedes Bild (vorher nur im Bereich Painting,
// bis dahin zeigte die Ansicht das Original). Nur fuer die Leinwaende des geladenen Modells.
void modellMalenVorschauAbgleich() {
    if (g_bilderLadung != mt().dok.ladeStand()) { return; }
    for (auto& b : g_bilder) {
        if (b->l.bemalt() && b->bindungenFuer != g_app->modelTextures.ladeNummer) {   // P31
            mm::schmutzigDazu(b->l, mm::Rechteck{0, 0, b->l.b - 1, b->l.h - 1});
            anzeigeNachfuehren(*b);
        }
    }
}

// --- Gruppen im rechten Feld (AP9, MBENTWURF Teil B) ------------------------------------------------------------------------
//
// Form und Reihenfolge nach dem freigegebenen Entwurf (plaene/modellblock_stufe0/entwurf_modelltab.html, Bereich Painting;
// Abgleich T0a-T10): Ziel (Shader, Bild) - Werkzeuge (Raster, das gewaehlte als Hauptknopf) - Pinsel (Kurzinfo mit den Tasten
// des Bedienmodus, Farbe und Regler) - Ebenen (Liste mit Auge und Schloss, Raster) - Tipp. Was das Programm mehr kann als der
// Entwurf, bleibt an der Stelle, an die es gehoert: Stufe des Shaders und Umbacken beim Ziel, Weichzeichnen und Radieren bei
// den Werkzeugen, zweite Farbe und Farbe aufnehmen in der Zeile Farbe, Haerte, Spiegeln, Stiftdruck, Rueckseiten und Ausbluten
// unter "Erweitert" (ab Werk zugeklappt), Mischart und Deckkraft der gewaehlten Ebene unter der Liste, "Fuer das Spiel" als
// letzte Gruppe (wie 3ds Max auch im Reiter Dienstprogramme). Der Baustein steht in gui/ui_rollout.h.

namespace {

std::string mitKennung(Str text, const char* kennung) { return std::string(tr(text)) + "###" + kennung; }

const MalAktion* aktionVon(Str text) {
    const auto& liste = aktionen();
    const auto it = std::find_if(liste.begin(), liste.end(), [&](const MalAktion& a) { return a.text == text; });
    return it != liste.end() ? &*it : nullptr;
}

// Eine Aktion als Knopf des Bausteins: Symbol, Text, die Taste des Bedienmodus als Tastenfeld, ihr Hinweis als Tooltip.
// haupt: in der Betonungsfarbe (das gewaehlte Werkzeug, "Fuer das Spiel bauen").
void aktionsKnopf(Str text, UiIcon icon, bool haupt = false) {
    const MalAktion* a = aktionVon(text);
    if (a == nullptr) { return; }
    const char* k = tasteVon(*a);
    if (ui::werkzeugKnopf(tr(text), {.icon = icon, .taste = k[0] != '\0' ? k : nullptr, .hinweis = tr(a->hinweis), .haupt = haupt, .an = a->an && a->an()})) { ausloesen(*a); }
}

// Dieselbe Aktion als quadratischer Symbolknopf (in der Zeile Farbe).
void symbolKnopf(Str text, UiIcon icon) {
    const MalAktion* a = aktionVon(text);
    if (a == nullptr) { return; }
    if (iconKnopf(tr(text), icon, tooltip(*a).c_str(), a->an && a->an())) { ausloesen(*a); }
}

// Ein Pfad, der nicht in die Breite passt: vorne um ganze Ordner gekuerzt (".../kyle/torso", "...\\ap9_mod"); der ganze steht im Tooltip.
std::string pfadGekuerzt(const std::string& pfad, float breite) {
    if (ImGui::CalcTextSize(pfad.c_str()).x <= breite) { return pfad; }
    std::string kurz = pfad;
    std::size_t ab = pfad.find_first_of("/\\");
    while (ab != std::string::npos) {
        kurz = "..." + pfad.substr(ab);
        if (ImGui::CalcTextSize(kurz.c_str()).x <= breite) { return kurz; }
        ab = pfad.find_first_of("/\\", ab + 1U);
    }
    // auch der kuerzeste passt nicht (sehr schmales Feld): hinten kuerzen und mit "..." kennzeichnen statt abzuschneiden
    while (!kurz.empty() && ImGui::CalcTextSize((kurz + "...").c_str()).x > breite) {
        // ein ganzes UTF-8-Zeichen: erst die Folgebytes, dann das Startbyte
        while (!kurz.empty() && (static_cast<unsigned char>(kurz.back()) & 0xC0U) == 0x80U) { kurz.pop_back(); }
        if (!kurz.empty()) { kurz.pop_back(); }
    }
    return kurz + "...";
}

// Der Platz fuer den Text eines Ausklappers in der Formularzeile (Feldbreite ohne Pfeil und Rand).
float ausklapperText() { return ui::feldBreite() - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x * 2.0F; }

// Ziel: worauf gemalt wird - der Shader (die Flaechen, die sein Bild zeigen), die Stufe des Shaders, das Bild.
// TEST1411 O: die Bilder des Modells als kleine Vorschaubilder mit Namen (wie Blenders Texture Slots und die Teilmaterialien von
// Max Multi/Sub-Object) - aus der Leinwand, sobald sie bemalt wird, sonst aus dem Bild der Vorschau. Neu verkleinert, wenn sich die
// Quelle aendert (beim Malen hoechstens alle 15 Bilder); die Anzeige ist nur ein Vorschaubild, die Leinwand bleibt unberuehrt.
struct VorschauBild {
    void* id = nullptr;
    unsigned texStand = 0;
    const void* quelle = nullptr;
    unsigned stand = 0;
    int bild = -1000;
    int b = 0;
    int h = 0;
};
std::map<std::string, VorschauBild> g_vorschau;

// das Bild der Vorschau zu einer Gruppe: die Stufe der ersten Flaeche, deren Bild das der Gruppe ist
const TextureSet::Tex* vorschauTex(const Gruppe& g) {
    const std::vector<BildStufe>& st = bildStufen(g.shader);
    const ShaderInfo* info = shaderInfoVon(g.shader);
    if (st.empty() || info == nullptr) { return nullptr; }
    const auto& stufen = g_app->modelTextures.stufen;
    for (const int t : g.teile) {
        if (t < 0 || uz(t) >= stufen.size()) { continue; }
        for (const ModelTextures::FigurStufe& fs : stufen[uz(t)]) {
            if (fs.art.weiss || fs.pixel.size() != 1U || fs.pixel[0] == nullptr || fs.art.stufe >= info->stages.size()) { continue; }
            if (bildSchluessel(info->stages[fs.art.stufe].image) == st.front().schluessel) { return fs.pixel[0].get(); }
        }
    }
    return nullptr;
}

void* vorschauBild(int gi, const Gruppe& g, int kante) {
    const std::vector<BildStufe>& st = bildStufen(g.shader);
    if (st.empty()) { return nullptr; }
    VorschauBild& v = g_vorschau[st.front().schluessel];
    const MalBild* m = bildFuer(gi, 0, false);
    const TextureSet::Tex* tex = m == nullptr ? vorschauTex(g) : nullptr;
    const std::uint8_t* px = m != nullptr ? m->l.bild.data() : (tex != nullptr ? tex->rgba.data() : nullptr);
    const int qb = m != nullptr ? m->l.b : (tex != nullptr ? tex->width : 0);
    const int qh = m != nullptr ? m->l.h : (tex != nullptr ? tex->height : 0);
    if (px == nullptr || qb <= 0 || qh <= 0) { return v.id; }
    const void* quelle = m != nullptr ? static_cast<const void*>(m) : static_cast<const void*>(tex);
    const unsigned stand = m != nullptr ? static_cast<unsigned>(m->l.stand) : 0U;
    const bool neu = v.id == nullptr || v.texStand != render::texturStand();
    const bool anders = v.quelle != quelle || v.stand != stand;
    if (!neu && (!anders || ImGui::GetFrameCount() - v.bild < 15)) { return v.id; }
    // verkleinern: je Zielpunkt der Mittelwert ueber hoechstens 4 x 4 Punkte seines Blocks
    const float f = static_cast<float>(kante) / static_cast<float>(std::max(qb, qh));
    const int zb = std::max(1, static_cast<int>(std::lround(static_cast<float>(qb) * f)));
    const int zh = std::max(1, static_cast<int>(std::lround(static_cast<float>(qh) * f)));
    std::vector<std::uint8_t> out(static_cast<std::size_t>(zb) * static_cast<std::size_t>(zh) * 4U);
    for (int y = 0; y < zh; ++y) {
        const int y0 = y * qh / zh;
        const int y1 = std::max(y0 + 1, (y + 1) * qh / zh);
        for (int x = 0; x < zb; ++x) {
            const int x0 = x * qb / zb;
            const int x1 = std::max(x0 + 1, (x + 1) * qb / zb);
            unsigned summe[4] = {0, 0, 0, 0};
            unsigned n = 0;
            for (int sy = y0; sy < y1; sy += std::max(1, (y1 - y0) / 4)) {
                for (int sx = x0; sx < x1; sx += std::max(1, (x1 - x0) / 4)) {
                    const std::uint8_t* q = px + (static_cast<std::size_t>(sy) * static_cast<std::size_t>(qb) + static_cast<std::size_t>(sx)) * 4U;
                    for (int k = 0; k < 4; ++k) { summe[k] += q[k]; }
                    ++n;
                }
            }
            std::uint8_t* o = out.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(zb) + static_cast<std::size_t>(x)) * 4U;
            for (int k = 0; k < 3; ++k) { o[k] = static_cast<std::uint8_t>(summe[k] / std::max(n, 1U)); }
            o[3] = 255;   // deckend: die Vorschau zeigt die Farben, nicht den Alpha-Kanal
        }
    }
    if (neu || v.b != zb || v.h != zh) {
        v.id = render::createTexture(out.data(), zb, zh);
        v.texStand = render::texturStand();
    } else {
        render::updateTexture(v.id, out.data(), zb, zh);
    }
    v.quelle = quelle;
    v.stand = stand;
    v.bild = ImGui::GetFrameCount();
    v.b = zb;
    v.h = zh;
    return v.id;
}

void vorschauRaster(const std::vector<Gruppe>& gr) {
    ModellMalen& z = mz();
    z.vorschauKaesten.assign(gr.size(), ImRect{});
    if (gr.empty()) { return; }
    const float fs = ImGui::GetFontSize();
    const float breite = std::max(ImGui::GetContentRegionAvail().x, fs * 4.0F);
    const float luecke = std::floor(fs * 0.35F);
    const int spalten = std::clamp(static_cast<int>((breite + luecke) / (fs * 4.6F + luecke)), 1, 8);
    const float zelle = std::floor((breite - luecke * static_cast<float>(spalten - 1)) / static_cast<float>(spalten));
    const float bildH = std::floor(zelle * 0.78F);
    const float zeileH = bildH + fs + 4.0F;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const int kante = std::clamp(static_cast<int>(bildH * 2.0F), 32, 256);   // doppelte Aufloesung fuer hohe DPI
    for (std::size_t i = 0; i < gr.size(); ++i) {
        const int sp = static_cast<int>(i) % spalten;
        const int ze = static_cast<int>(i) / spalten;
        const ImVec2 o{start.x + static_cast<float>(sp) * (zelle + luecke), start.y + static_cast<float>(ze) * (zeileH + luecke)};
        ImGui::SetCursorScreenPos(o);
        ImGui::PushID(static_cast<int>(i));
        const bool geklickt = ImGui::InvisibleButton("##ptvorschau", ImVec2{zelle, zeileH});
        const bool ueber = ImGui::IsItemHovered();
        ImGui::PopID();
        const bool gewaehlt = static_cast<int>(i) == z.gruppe;
        z.vorschauKaesten[i] = ImRect{o, ImVec2{o.x + zelle, o.y + zeileH}};
        const std::vector<BildStufe>& st = bildStufen(gr[i].shader);
        const std::string name = st.empty() ? fileName(gr[i].shader) : fileName(st.front().datei.empty() ? st.front().name : st.front().datei);
        if (ueber) { ImGui::SetTooltip(tr(Str::T1oVorschauHint), gr[i].shader.c_str()); }
        if (geklickt) {
            z.gruppe = static_cast<int>(i);
            z.stufe = 0;
            melden(format(Str::T1oZiel, name.c_str()));
        }
        dl->AddRectFilled(o, ImVec2{o.x + zelle, o.y + bildH}, ImGui::GetColorU32(ImGuiCol_FrameBg), 2.0F);
        if (void* id = vorschauBild(static_cast<int>(i), gr[i], kante)) {
            const VorschauBild& v = g_vorschau[st.front().schluessel];
            const float s = std::min((zelle - 4.0F) / static_cast<float>(std::max(v.b, 1)), (bildH - 4.0F) / static_cast<float>(std::max(v.h, 1)));
            const float bw = std::floor(static_cast<float>(v.b) * s);
            const float bh = std::floor(static_cast<float>(v.h) * s);
            const ImVec2 b0{std::floor(o.x + (zelle - bw) * 0.5F), std::floor(o.y + (bildH - bh) * 0.5F)};
            dl->AddImage(reinterpret_cast<ImTextureID>(id), b0, ImVec2{b0.x + bw, b0.y + bh});
        }
        const ImU32 rahmen = gewaehlt ? ImGui::GetColorU32(ui::betonungsFarbe()) : ImGui::GetColorU32(ueber ? ImGuiCol_ButtonHovered : ImGuiCol_Border);
        dl->AddRect(o, ImVec2{o.x + zelle, o.y + bildH}, rahmen, 2.0F, 0, gewaehlt ? 2.0F : 1.0F);
        // der Name unter dem Bild, gekuerzt auf die Zelle (ganz im Tooltip)
        std::string zeigen = name;
        while (zeigen.size() > 3U && ImGui::CalcTextSize(zeigen.c_str()).x > zelle) { zeigen = zeigen.substr(0, zeigen.size() - 4U) + "..."; }
        dl->AddText(ImVec2{std::floor(o.x + (zelle - ImGui::CalcTextSize(zeigen.c_str()).x) * 0.5F), o.y + bildH + 2.0F},
                    ImGui::GetColorU32(gewaehlt ? ImGuiCol_Text : ImGuiCol_TextDisabled), zeigen.c_str());
    }
    const int reihen = (static_cast<int>(gr.size()) + spalten - 1) / spalten;
    ImGui::SetCursorScreenPos(start);
    ImGui::Dummy(ImVec2{breite, static_cast<float>(reihen) * (zeileH + luecke) - luecke});
}

void rolloutZiel(const std::vector<Gruppe>& gr, MalBild* m) {
    ModellMalen& z = mz();
    const std::string datei = m != nullptr ? fileName(m->l.datei) : std::string();
    // wie Blender steht das Ziel (dort die Texture Slots) auch im Reiter Material
    if (!ui::rolloutBeginn(tr(Str::PtGrZiel), {.kurzinfo = datei.empty() ? nullptr : datei.c_str(), .reiter = modellReiter(MbReiter::Aendern, MbReiterBl::Material), .hinweis = tr(Str::PtZielHint)})) {
        return;
    }
    vorschauRaster(gr);   // TEST1411 O: schneller Wechsel ohne Ausklappliste
    if (!gr.empty()) {
        const int gw = std::clamp(z.gruppe, 0, static_cast<int>(gr.size()) - 1);
        const std::string& shader = gr[uz(gw)].shader;
        ui::zeile(tr(Str::PtZielShader), tr(Str::PtGruppeHint));
        if (ImGui::BeginCombo("##ptgruppe", pfadGekuerzt(shader, ausklapperText()).c_str())) {
            for (std::size_t i = 0; i < gr.size(); ++i) {
                if (ImGui::Selectable(gr[i].shader.c_str(), static_cast<int>(i) == gw)) {
                    z.gruppe = static_cast<int>(i);
                    z.stufe = 0;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("%s\n%s", shader.c_str(), tr(Str::PtGruppeHint));
        const std::vector<BildStufe>& st = bildStufen(shader);
        if (st.size() > 1U) {
            const int s = std::clamp(z.stufe, 0, static_cast<int>(st.size()) - 1);
            ui::zeile(tr(Str::PtZielStufe), tr(Str::PtStufeHint));
            const std::string& stufeName = st[uz(s)].name;
            const std::size_t trenner = stufeName.find_last_of("/\\");
            const float kuerzest = ImGui::CalcTextSize(trenner == std::string::npos ? stufeName.c_str() : ("..." + stufeName.substr(trenner)).c_str()).x;
            std::string vorschau = format(Str::PtStufe, s + 1, "");
            if (ImGui::CalcTextSize(vorschau.c_str()).x + kuerzest > ausklapperText()) {
                vorschau = std::to_string(s + 1) + ": ";   // schmales Feld: die Beschriftung der Zeile nennt die Stufe schon
            }
            vorschau += pfadGekuerzt(stufeName, std::max(ausklapperText() - ImGui::CalcTextSize(vorschau.c_str()).x, 0.0F));
            if (ImGui::BeginCombo("##ptstufe", vorschau.c_str())) {
                for (std::size_t i = 0; i < st.size(); ++i) {
                    if (ImGui::Selectable(format(Str::PtStufe, static_cast<int>(i) + 1, st[i].name.c_str()).c_str(), static_cast<int>(i) == s)) { z.stufe = static_cast<int>(i); }
                }
                ImGui::EndCombo();
            }
            ImGui::SetItemTooltip("%s\n%s", st[uz(s)].name.c_str(), tr(Str::PtStufeHint));
        }
    }
    if (m != nullptr) {
        const int bits = m->l.tgaBits > 0 ? m->l.tgaBits : (m->l.alpha ? 32 : 24);
        const char* art = image::formatName(static_cast<image::Format>(m->l.format));
        ui::wertTabelle({{tr(Str::PtGrBild), format(Str::PtZielBildWert, datei.c_str(), m->l.b, m->l.h), ui::Ton::Normal, format(Str::PtBildInfo, m->l.datei.c_str(), m->l.b, m->l.h, art, bits)},
                         {tr(Str::PtZielFormat), format(Str::PtZielFormatWert, art, bits), ui::Ton::Normal, {}}});
    } else {
        ui::absatz(tr(Str::PtKeinBild), ui::Ton::Warnung);
    }
    (void)ui::schalter(tr(Str::T1oNurZiel), &z.nurZiel, tr(Str::T1oNurZielHint));   // TEST1411 O
    ui::rasterBeginn(2);
    // wie im Entwurf (AP9) "Bild zeigen | Auf neue UVs backen" in der ersten Reihe; TEST1411 O: die Pipette (Alt+Klick) darunter
    aktionsKnopf(Str::PtEditorZeigen, UiIcon::Bildschirm);
    aktionsKnopf(Str::PtUmbacken, UiIcon::Neuladen);
    aktionsKnopf(Str::T1oTexturAufnehmen, UiIcon::MalPipette);
    ui::rasterEnde();
    ui::rolloutEnde();
}

// Werkzeuge: zuerst die fuenf des Entwurfs (Pinsel, Fuellen, Klonen, Wischen, Maske), dann Weichzeichnen und Radieren; das
// gewaehlte ist der Hauptknopf. Die Kurzinfo nennt das Vorbild des Bedienmodus (Viewport Canvas / Texture Paint).
void rolloutWerkzeuge() {
    if (!ui::rolloutBeginn(tr(Str::PtGrWerkzeuge), {.kurzinfo = tr(blenderModus() ? Str::PtRolloutBlender : Str::PtRolloutMax), .hinweis = tr(Str::PtWerkzeugeHint)})) { return; }
    struct Eintrag {
        Str text;
        UiIcon icon;
        mm::Werkzeug werkzeug;
    };
    static const Eintrag kFolge[mm::kWerkzeuge] = {
        {Str::PtWzPinsel, UiIcon::MalPinsel, mm::Werkzeug::Pinsel},       {Str::PtWzFuellen, UiIcon::MalFuellen, mm::Werkzeug::Fuellen},
        {Str::PtWzKlonen, UiIcon::MalKlonen, mm::Werkzeug::Klonen},       {Str::PtWzWischen, UiIcon::MalWischen, mm::Werkzeug::Verwischen},
        {Str::PtWzMaske, UiIcon::MalMaske, mm::Werkzeug::Maske},          {Str::PtWzWeich, UiIcon::MalWeich, mm::Werkzeug::Weichzeichnen},
        {Str::PtWzRadierer, UiIcon::MalRadierer, mm::Werkzeug::Radierer},
    };
    ui::rasterBeginn(2);
    for (const Eintrag& e : kFolge) { aktionsKnopf(e.text, e.icon, istWerkzeug(e.werkzeug)); }
    ui::rasterEnde();
    ui::rolloutEnde();
}

// Pinsel: Farbe, Radius, Staerke, Abstand wie im Entwurf; die Einstellung des gewaehlten Werkzeugs (Fuellen: Toleranz, Maske:
// Freigeben); alles Weitere unter "Erweitert".
void rolloutPinsel() {
    ModellMalen& z = mz();
    const float fs = ImGui::GetFontSize();
    if (!ui::rolloutBeginn(tr(Str::PtGrPinsel), {.kurzinfo = tr(blenderModus() ? Str::PtInfoBlender : Str::PtInfoMax), .hinweis = tr(Str::PtPinselHint)})) { return; }
    // Farbe: Pinselfarbe, zweite Farbe, tauschen, aufnehmen - passt das nicht nebeneinander, stehen die Knoepfe darunter
    ui::zeile(tr(Str::PtFarbe), tr(Str::PtFarbeHint));
    const float feld = ImGui::GetFrameHeight();
    const float luecke = std::floor(fs * 0.3F);
    const bool eineZeile = ui::feldBreite() >= feld * 4.0F + luecke * 2.0F + 2.0F;
    ImGui::ColorEdit4("##ptfarbe", z.pinsel.farbe.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
    ImGui::SetItemTooltip("%s", tr(Str::PtFarbeHint));
    ImGui::SameLine(0.0F, luecke);
    ImGui::ColorEdit4("##ptfarbe2", z.farbe2.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
    ImGui::SetItemTooltip("%s", tr(Str::PtFarbe2Hint));
    if (eineZeile) {
        ImGui::SameLine(0.0F, luecke);
    } else {
        ui::zeileOhne();
    }
    symbolKnopf(Str::PtFarbenTauschen, UiIcon::Tauschen);
    ImGui::SameLine(0.0F, 2.0F);
    symbolKnopf(Str::PtFarbeAufnehmen, UiIcon::MalPipette);
    const std::string radiusHinweis = tooltipVon(Str::PtRadiusZiehen) + "\n" + tr(Str::PtRadiusTasten);
    if (ui::regler(mitKennung(Str::PtRadius, "ptradius").c_str(), &z.radiusPx, 1.0F, 300.0F, "%.0f px", radiusHinweis.c_str(), ImGuiSliderFlags_Logarithmic)) {
        z.radiusPx = std::clamp(z.radiusPx, 1.0F, 500.0F);   // FIX612B P2-8: auch getippt (Strg+Klick) die Grenzen der Tasten [ ] und F
    }
    ui::regler(mitKennung(Str::PtStaerke, "ptstaerke").c_str(), &z.pinsel.staerke, 0.0F, 1.0F, "%.2f", tooltipVon(Str::PtStaerkeZiehen).c_str());
    float abst = z.pinsel.abstand * 100.0F;
    if (ui::regler(mitKennung(Str::PtAbstand, "ptabstand").c_str(), &abst, 1.0F, 200.0F, "%.0f %%", tr(Str::PtAbstandHint))) { z.pinsel.abstand = abst / 100.0F; }
    if (istWerkzeug(mm::Werkzeug::Fuellen)) { ui::regler(mitKennung(Str::PtToleranz, "pttol").c_str(), &z.pinsel.fuellToleranz, 0.0F, 1.0F, "%.2f", tr(Str::PtToleranzHint)); }
    if (istWerkzeug(mm::Werkzeug::Maske)) { ui::schalter(tr(Str::PtMaskeFrei), &z.pinsel.maskeLoeschen, tr(Str::PtMaskeFreiHint)); }
    if (ui::mehrBeginn(mitKennung(Str::PtErweitert, "ptmehr").c_str())) {
        ui::regler(mitKennung(Str::PtHaerte, "pthaerte").c_str(), &z.pinsel.haerte, 0.0F, 1.0F, "%.2f", tr(Str::PtHaerteHint));
        // Spiegeln mit seiner Achse
        ui::schalter(tr(Str::PtSpiegeln), &z.spiegeln, tooltipVon(Str::PtSpiegeln).c_str());
        static const char* const kAchsen[3] = {"X", "Y", "Z"};
        ui::zeile(tr(Str::PtAchse), tr(Str::PtAchseHint));
        ui::segment("##ptachse", kAchsen, 3, &z.spiegelAchse, 3, -1, nullptr, std::min(ui::feldBreite(), fs * 7.0F));
        // Stiftdruck
        ui::rasterBeginn(2);
        ui::schalter(tr(Str::PtDruckGroesse), &z.druckGroesse, tr(Str::PtDruckGroesseHint));
        ui::schalter(tr(Str::PtDruckStaerke), &z.druckStaerke, tr(Str::PtDruckStaerkeHint));
        ui::rasterEnde();
        ui::absatz(z.stiftDa ? format(Str::PtStiftDa, static_cast<double>(z.stiftDruck)).c_str() : tr(Str::PtStiftFehlt));
        ui::schalter(tr(Str::PtRueckseiten), &z.pinsel.rueckseiten, tr(Str::PtRueckseitenHint));
        ui::zeile(tr(Str::PtAusbluten), tr(Str::PtAusblutenHint));
        ImGui::SetNextItemWidth(std::min(ui::feldBreite(), fs * 7.0F));
        if (ImGui::InputInt("##ptausbluten", &z.ausbluten)) {
            z.ausbluten = std::clamp(z.ausbluten, 0, 64);
            for (auto& b : g_bilder) {
                b->l.ausbluten = z.ausbluten;
                const auto laden = b->l.dreieckeBeimLaden;
                mm::dreieckeSetzen(b->l, b->l.dreiecke, false);
                b->l.dreieckeBeimLaden = laden;
            }
        }
        ImGui::SetItemTooltip("%s", tr(Str::PtAusblutenHint));
        ui::mehrEnde();
    }
    ui::rolloutEnde();
}

Str mischName(mm::Mischart art) {
    static const Str kNamen[mm::kMischarten] = {Str::PtMischNormal, Str::PtMischMult, Str::PtMischAdd, Str::PtMischScreen, Str::PtMischOverlay};
    return kNamen[std::clamp(static_cast<int>(art), 0, mm::kMischarten - 1)];
}

// Eine Zeile der Ebenenliste: der Name ueber die Breite (mit der Mischart in Klammern, wenn sie nicht Normal ist), die
// gewaehlte Ebene in der Betonungsfarbe; rechts das Auge (sichtbar) bzw. am Original das Schloss - es bleibt unberuehrt.
void ebenenZeile(MalBild& m, int i) {
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const bool aktiv = m.l.aktiv == i;
    const bool sichtbar = m.l.ebenen[uz(i)].sichtbar;
    // P13: die Grundebene (Name leer) in der Sprache der Oberflaeche - wie im Entwurf mit ihrer Datei ("Original torso.tga")
    std::string text = m.l.ebenen[uz(i)].name.empty() ? std::string(tr(Str::PtEbeneOriginal)) + " " + fileName(m.l.datei) : m.l.ebenen[uz(i)].name;
    if (i > 0 && m.l.ebenen[uz(i)].misch != mm::Mischart::Normal) { text += std::string(" (") + tr(mischName(m.l.ebenen[uz(i)].misch)) + ")"; }
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), fs * 4.0F);
    const float hoehe = std::floor(fs * 1.7F);
    const float knopf = std::floor(fs * 1.3F);
    const float randX = std::floor(fs * 0.45F);
    const float textB = std::max(breite - randX * 2.0F - knopf, fs);
    const ImVec2 textG = ImGui::CalcTextSize(text.c_str());
    ImGui::PushID(i);
    ImGui::SetNextItemAllowOverlap();   // das Auge liegt auf der Zeile und bekommt seinen Klick
    if (ImGui::InvisibleButton("##ebene", ImVec2{breite, hoehe})) { m.l.aktiv = i; }
    const bool ueber = ImGui::IsItemHovered();
    const ImRect bb{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
    IMGUI_TEST_ENGINE_ITEM_INFO(ImGui::GetItemID(), text.c_str(), g.LastItemData.StatusFlags | (aktiv ? ImGuiItemStatusFlags_Checked : 0));
    const char* hinweis = tr(i == 0 ? Str::PtEbeneGrundHint : Str::PtEbeneWahlHint);
    if (textG.x > textB) {
        ImGui::SetItemTooltip("%s\n%s", text.c_str(), hinweis);   // der Name ist gekuerzt: ganz im Tooltip
    } else {
        ImGui::SetItemTooltip("%s", hinweis);
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rund = ui::rolloutStil() == ui::RolloutStil::Blender ? std::floor(fs * 0.3F) : 2.0F;
    // die gewaehlte Ebene wie der Hauptknopf des Bausteins: Betonungsfarbe, hellere Kante, Schrift nach der Helligkeit
    const ImVec4 betont = ui::betonungsFarbe();
    const auto zuWeiss = [](const ImVec4& c, float t) { return ImVec4{c.x + (1.0F - c.x) * t, c.y + (1.0F - c.y) * t, c.z + (1.0F - c.z) * t, 1.0F}; };
    ImVec4 grund = ImGui::GetStyleColorVec4(ueber ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 kante = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 schrift = ImGui::GetStyleColorVec4(sichtbar ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    if (aktiv) {
        grund = ueber ? zuWeiss(betont, 0.15F) : betont;
        kante = zuWeiss(betont, 0.25F);
        const bool hell = betont.x * 0.3F + betont.y * 0.59F + betont.z * 0.11F > 0.6F;
        schrift = hell ? ImVec4{0.0F, 0.0F, 0.0F, 1.0F} : ImVec4{1.0F, 1.0F, 1.0F, 1.0F};
        if (!sichtbar) { schrift.w = 0.6F; }
    }
    dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(grund), rund);
    dl->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(kante), rund);
    const ImVec2 textOrt{bb.Min.x + randX, std::floor(bb.Min.y + (hoehe - fs) * 0.5F)};
    ImGui::PushStyleColor(ImGuiCol_Text, schrift);
    ImGui::RenderTextEllipsis(dl, textOrt, ImVec2{textOrt.x + textB, bb.Max.y}, textOrt.x + textB, text.c_str(), nullptr, &textG);
    ImGui::PopStyleColor();
    const ImVec2 danach = ImGui::GetCursorScreenPos();
    const ImVec2 knopfOrt{bb.Max.x - std::floor(randX * 0.5F) - knopf, std::floor(bb.Min.y + (hoehe - knopf) * 0.5F)};
    const float symbol = std::floor(knopf * 0.72F);
    const ImVec2 symbolOrt{std::floor(knopfOrt.x + (knopf - symbol) * 0.5F), std::floor(knopfOrt.y + (knopf - symbol) * 0.5F)};
    if (i > 0) {
        ImGui::SetCursorScreenPos(knopfOrt);
        const bool umschalten = ImGui::InvisibleButton(tr(Str::PtEbeneSicht), ImVec2{knopf, knopf});
        IMGUI_TEST_ENGINE_ITEM_INFO(ImGui::GetItemID(), tr(Str::PtEbeneSicht), g.LastItemData.StatusFlags | (sichtbar ? ImGuiItemStatusFlags_Checked : 0));
        if (ImGui::IsItemHovered()) { dl->AddRectFilled(knopfOrt, ImVec2{knopfOrt.x + knopf, knopfOrt.y + knopf}, ImGui::GetColorU32(ImGuiCol_ButtonHovered), rund); }
        ImGui::SetItemTooltip("%s", tr(Str::PtEbeneSichtHint));
        uiIcon(dl, sichtbar ? UiIcon::Sichtbar : UiIcon::Versteckt, symbolOrt, symbol, ImGui::GetColorU32(schrift));
        ImGui::SetCursorScreenPos(danach);
        if (umschalten) {
            stapelBefehl(Str::PtEbeneSicht, [i, sichtbar](std::vector<mm::Ebene>& st, int&) { st[uz(i)].sichtbar = !sichtbar; });
        }
    } else {
        const ImGuiID schloss = ImGui::GetID("##ptschloss");
        if (ImGui::ItemAdd(ImRect{knopfOrt, ImVec2{knopfOrt.x + knopf, knopfOrt.y + knopf}}, schloss, nullptr, ImGuiItemFlags_NoNav)) {
            uiIcon(dl, UiIcon::Schloss, symbolOrt, symbol, ImGui::GetColorU32(uiIconFarbe(UiIcon::Schloss)));
        }
        IMGUI_TEST_ENGINE_ITEM_INFO(schloss, "##ptschloss", 0);
    }
    ImGui::PopID();
}

// Ebenen: die Liste von oben (wie 3ds Max und Blender), Mischart und Deckkraft der gewaehlten Ebene, die Befehle im Raster.
void rolloutEbenen(MalBild& m) {
    ModellMalen& z = mz();
    const float fs = ImGui::GetFontSize();
    char zahl[16];
    std::snprintf(zahl, sizeof zahl, "%d", static_cast<int>(m.l.ebenen.size()));
    if (!ui::rolloutBeginn(tr(Str::PtGrEbenen), {.kurzinfo = zahl, .hinweis = tr(Str::PtEbenenHint)})) { return; }
    const ImGuiStyle& stil = ImGui::GetStyle();
    const float eng = std::floor(fs * 0.18F);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{stil.ItemSpacing.x, eng});
    for (int i = static_cast<int>(m.l.ebenen.size()) - 1; i >= 0; --i) { ebenenZeile(m, i); }
    ImGui::PopStyleVar();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + stil.ItemSpacing.y - eng);
    // Mischart und Deckkraft der gewaehlten Ebene; das Original hat keine (gesperrt)
    const int a = std::clamp(m.l.aktiv, 0, static_cast<int>(m.l.ebenen.size()) - 1);
    ImGui::BeginDisabled(a == 0);
    ui::zeile(tr(Str::PtEbeneMisch), tr(Str::PtEbeneMischHint));
    int mi = static_cast<int>(m.l.ebenen[uz(a)].misch);
    const std::string arten = std::string(tr(Str::PtMischNormal)) + '\0' + tr(Str::PtMischMult) + '\0' + tr(Str::PtMischAdd) + '\0' + tr(Str::PtMischScreen) + '\0' + tr(Str::PtMischOverlay) + '\0' + '\0';
    if (ImGui::Combo("##ptmisch", &mi, arten.c_str())) {
        stapelBefehl(Str::PtEbeneMisch, [a, mi](std::vector<mm::Ebene>& st, int&) { st[uz(a)].misch = static_cast<mm::Mischart>(mi); });
    }
    ImGui::SetItemTooltip("%s", tr(Str::PtEbeneMischHint));
    float d = m.l.ebenen[uz(a)].deckkraft * 100.0F;
    const float deckAlt = m.l.ebenen[uz(a)].deckkraft;   // P3: der Regler setzt den Wert schon im Klickbild - den alten vorher merken
    if (ui::regler(mitKennung(Str::PtDeckkraft, "ptdeck").c_str(), &d, 0.0F, 100.0F, "%.0f %%", tr(Str::PtEbeneDeckHint))) {
        m.l.ebenen[uz(a)].deckkraft = d / 100.0F;   // live; der Rueckgaengig-Schritt beim Loslassen
        mm::zusammensetzen(m.l);
        mm::schmutzigDazu(m.l, mm::Rechteck{0, 0, m.l.b - 1, m.l.h - 1});
        anzeigeNachfuehren(m);
    }
    if (ImGui::IsItemActivated()) { z.deckVorher = deckAlt; }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        const float neu = m.l.ebenen[uz(a)].deckkraft;
        m.l.ebenen[uz(a)].deckkraft = z.deckVorher;
        stapelBefehl(Str::PtEbeneDeck, [a, neu](std::vector<mm::Ebene>& st, int&) { st[uz(a)].deckkraft = neu; });
    }
    ImGui::EndDisabled();
    // Neue Ebene und Zusammenfuegen wie im Entwurf zuerst
    ui::rasterBeginn(2);
    aktionsKnopf(Str::PtEbeneNeu, UiIcon::Plus);
    aktionsKnopf(Str::PtEbeneVereinen, UiIcon::Boden);
    aktionsKnopf(Str::PtEbeneHoch, UiIcon::MalHoch);
    aktionsKnopf(Str::PtEbeneRunter, UiIcon::MalRunter);
    aktionsKnopf(Str::PtEbeneLoeschen, UiIcon::Loeschen);
    aktionsKnopf(Str::PtMaskeAn, UiIcon::MalMaske);
    aktionsKnopf(Str::PtMaskeLeeren, UiIcon::MalMaskeLeer);
    aktionsKnopf(Str::PtMaskeUmkehren, UiIcon::MalMaskeUmkehr);
    ui::rasterEnde();
    ui::rolloutEnde();
}

// Fuer das Spiel: der Ausgabeordner, der Hauptknopf, die Zahl der bemalten Bilder (auch als Kurzinfo der Gruppe).
void rolloutAusgabe() {
    ModellMalen& z = mz();
    const float fs = ImGui::GetFontSize();
    int bemalt = 0;
    // P6: der Vergleich mit dem Original nur, wenn sich das Bild geaendert hat (vorher jedes Bild alle Leinwaende ganz)
    for (const auto& b : g_bilder) {
        if (b->bemaltFuer != b->l.stand) {
            const mm::Stoppuhr uhr(mm::messung().vergleichMs);
            b->bemaltFuer = b->l.stand;
            b->bemaltAnders = b->l.bild != b->l.grund;
        }
        bemalt += b->bemaltAnders ? 1 : 0;
    }
    char info[96];
    std::snprintf(info, sizeof info, trN(Str::PtInfoBemalt, bemalt), bemalt);
    // wie 3ds Max auch im Reiter Dienstprogramme, wie Blender im Reiter Werkzeug (dem des Bereichs)
    if (!ui::rolloutBeginn(tr(Str::PtGrAusgabe), {.kurzinfo = info, .ton = bemalt > 0 ? ui::Ton::Normal : ui::Ton::Leise, .reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Werkzeug), .hinweis = tr(Str::PtAusgabeHint)})) {
        return;
    }
    const std::string ordner = ausgabeOrdner();
    ui::zeile(tr(Str::PtOrdner), tr(Str::PtOrdnerHint));
    const float wahl = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 2.0F;
    const float feldB = std::max(ui::feldBreite() - wahl - 2.0F, fs * 2.0F);
    ImGui::SetNextItemWidth(feldB);
    // der Platzhalter zeigt das Ende des Pfads (den Ordner, in den geschrieben wird), der ganze steht im Tooltip
    ImGui::InputTextWithHint("##ptordner", pfadGekuerzt(ordner, feldB - ImGui::GetStyle().FramePadding.x * 2.0F).c_str(), z.ordner, sizeof z.ordner);
    ImGui::SetItemTooltip("%s\n%s", ordner.c_str(), tr(Str::PtOrdnerHint));
    ImGui::SameLine(0.0F, 2.0F);
    if (ImGui::Button("...##ptordnerwahl")) {
        const std::string o = platform::pickFolder(tr(Str::PtOrdner), ordner);
        if (!o.empty()) { std::snprintf(z.ordner, sizeof z.ordner, "%s", o.c_str()); }
    }
    ImGui::SetItemTooltip("%s", tr(Str::PtOrdnerWaehlen));
    aktionsKnopf(Str::PtBauen, UiIcon::Speichern, true);
    ui::wertTabelle({{tr(Str::PtBemalt), std::to_string(bemalt), bemalt > 0 ? ui::Ton::Normal : ui::Ton::Leise, {}}});
    ui::rolloutEnde();
}

}  // namespace

void drawModellMalenBereich() {
    ModellMalen& z = mz();
    const mm::Stoppuhr bereichUhr(mm::messung().bereichMs);
    bilderPruefen();
    const std::vector<Gruppe> gr = gruppen();
    // TEST1411 O: ein Klick auf eine Flaeche im Szenen-Explorer (oder eine neue Auswahl in der Ansicht) waehlt ihre Textur als Ziel
    const int teilWahl = mt().auswahl.empty() ? -1 : mt().auswahl.front();
    if (teilWahl != z.auswahlTeil) {
        z.auswahlTeil = teilWahl;
        const int gw = teilWahl >= 0 ? gruppeVonTeil(gr, teilWahl) : -1;
        if (gw >= 0 && gw != z.gruppe) {
            z.gruppe = gw;
            z.stufe = 0;
        }
    }
    MalBild* m = jetzt(true);
    rolloutZiel(gr, m);
    rolloutWerkzeuge();
    rolloutPinsel();
    if (m != nullptr) {
        rolloutEbenen(*m);
        // der Tipp des Entwurfs steht als eigener Kasten unter den Ebenen (nur im Reiter, den der Bereich vorgibt)
        if (ui::feldReiterStandard()) { ui::tipp(tr(Str::PtEbenenTipp)); }
    }
    rolloutAusgabe();
    // die letzte Meldung des Malens, solange die Meldungszeile des Felds (ModellTab::meldung) schon eine andere zeigt -
    // nur im Reiter, den der Bereich vorgibt
    if (!z.meldung.empty() && z.meldung != mt().meldung && ui::feldReiterStandard()) {
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

// P8 (Pruefung seit rc610): die maskierten Texel (>= 128) als Rechtecke - je Zeile die Laeufe, ein Lauf mit gleichem Anfang
// und Ende wie einer der Zeile darueber verlaengert dessen Rechteck. Dieselbe Flaeche wie ein Rechteck je Lauf und Zeile;
// bestimmt nur bei neuem Stand (vorher jedes Bild alle Texel gelesen, je Lauf und Zeile ein Rechteck).
void maskeFlaechenNeu(MalBild& m) {
    const mm::Stoppuhr uhr(mm::messung().maskeMs);
    if (mm::messung().an) { ++mm::messung().maskeGelesen; }
    m.maskeFlaechen.clear();
    std::vector<std::size_t> offen;   // Rechtecke bis zur vorigen Zeile, nach x0 geordnet
    std::vector<std::size_t> weiter;
    for (int y = 0; y < m.l.h; ++y) {
        weiter.clear();
        std::size_t k = 0;
        int x = 0;
        while (x < m.l.b) {
            if (m.l.maske[m.l.texel(x, y)] < 128) {
                ++x;
                continue;
            }
            const int x0 = x;
            while (x < m.l.b && m.l.maske[m.l.texel(x, y)] >= 128) { ++x; }
            while (k < offen.size() && m.maskeFlaechen[offen[k]][0] < x0) { ++k; }
            if (k < offen.size() && m.maskeFlaechen[offen[k]][0] == x0 && m.maskeFlaechen[offen[k]][2] == x) {
                m.maskeFlaechen[offen[k]][3] = y + 1;
                weiter.push_back(offen[k]);
                ++k;
            } else {
                m.maskeFlaechen.push_back({x0, y, x, y + 1});
                weiter.push_back(m.maskeFlaechen.size() - 1U);
            }
        }
        std::swap(offen, weiter);
    }
    m.maskeFuer = m.l.maskeStand;
}

void drawModellMalEditor(ImVec2 groesse) {
    ModellTab& t = mt();
    ModellMalen& z = mz();
    const mm::Stoppuhr editorUhr(mm::messung().editorMs);
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
        m->texSchmutzig.clear();
    } else if (pl.fuer != m || pl.generation != g_bilderGeneration) {
        render::updateTexture(pl.id, m->l.bild.data(), m->l.b, m->l.h);
        pl.fuer = m;
        pl.generation = g_bilderGeneration;
        m->texSchmutzig.clear();
    } else if (!m->texSchmutzig.empty()) {
        for (const mm::Rechteck& r : m->texSchmutzig) { render::updateTextureBereich(pl.id, m->l.bild.data(), m->l.b, m->l.h, r.x0, r.y0, r.x1 + 1, r.y1 + 1); }
        m->texSchmutzig.clear();
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
        // die Maske als rote Flaeche (geschuetzt), Rechtecke gemerkt je Stand der Maske (P8; MBMAL: nicht je Stand des Bilds -
        // jeder Tupfer mit dem Pinsel las sonst alle Texel der Maske neu, 4 Millionen im 2048er Bild)
        z.maskeTexel = 0;
        if (m->maskeFuer != m->l.maskeStand) {
            maskeFlaechenNeu(*m);
            z.maskeTexel = m->l.b * m->l.h;
        }
        const float fb = static_cast<float>(m->l.b);
        const float fh = static_cast<float>(m->l.h);
        for (const std::array<int, 4>& r : m->maskeFlaechen) {
            dl->AddRectFilled(nachSchirm2(static_cast<float>(r[0]) / fb, static_cast<float>(r[1]) / fh), nachSchirm2(static_cast<float>(r[2]) / fb, static_cast<float>(r[3]) / fh),
                              IM_COL32(230, 40, 40, 90));
        }
        z.maskeRechtecke = static_cast<int>(m->maskeFlaechen.size());
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


// AP11 (Modelle in Tabs): die Leinwaende samt Ebenen und Rueckgaengig gehoeren dem Modell - sie gehen mit in sein Fach und kommen beim
// Zurueckwechseln wieder (vorher verwarf ein anderer Ladestand sie). Die 2D-Ansicht merkt sich Bilder nach Adresse: neue Generation.
void modellMalenTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        std::vector<std::unique_ptr<MalBild>> bilder;
        int ladung = -1;
        int lod = -1;
    };
    if (!f) { f = std::make_unique<Fach>(); }
    Fach& x = static_cast<Fach&>(*f);
    std::swap(g_bilder, x.bilder);
    std::swap(g_bilderLadung, x.ladung);
    std::swap(g_bilderLod, x.lod);
    ++g_bilderGeneration;
    g_stufen.je.clear();
    g_stufen.datei.clear();
}

}  // namespace bhed::gui
