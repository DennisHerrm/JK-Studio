// app_modelltab_retarget.cpp - Modell-Block Stufe 5: Retargeting im Model-Tab
//
// Rollout "Retargeting" im Befehlsfeld unter Auto-Physik (wie die Rollouts von Bewegung in 3ds Max; im Modus wie Blender
// dieselbe Stelle der Seitenleiste). Begriffe je Modus: wie MotionBuilder (Charakter-Definition, Charakterisierungs-Pose,
// Plotten) bzw. wie die Retarget-Werkzeuge in Blender (Knochenliste, Ruhepose, Backen). Aktionen im Menue Animation
// (Untermenue), im Rechtsklick und in der Suche (X / F3). Rechnung: bhed/modell/retarget.h.
//
// Die Einstellungen stehen im Dokument (RigDaten::rt, Rueckgaengig); die Quelle, die Charaktere und das Ergebnis haelt
// ModellTab::rt. Das Ergebnis wird sofort wie im Spiel gepackt (rtBacken, compressBone) - die Vorschau zeigt die gepackte
// GLA, so wie das Spiel sie spielt.
#include "app_internal.h"
#include "textumbruch.h"
#include "platform.h"

#include "bhed/modell/animimport.h"
#include "bhed/modell/retarget.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }
ModellRetarget& rt() { return g_app->mtab.rt; }

void melden(const std::string& m) {
    mt().meldung = m;
    mt().rigMeldung = m;
    addStatus(m);
}

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

// Begriff je Bedienmodus (3ds Max/MotionBuilder oder Blender)
const char* begriff(Str max, Str blender) { return tr(modellBlender() ? blender : max); }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

void daneben(const char* naechster) {
    const float b = ImGui::CalcTextSize(naechster).x + ImGui::GetStyle().FramePadding.x * 2.0F;
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < b) { ImGui::NewLine(); }
}

GlaAnimation g_gebacken;   // das gepackte Ergebnis (Vorschau und Backen)

// Stufe 5b: die Zeile der animation.cfg zur Kategorie des Generators
const char* kategorieZeile(modell::GangArt a) { return a == modell::GangArt::Laufen ? "BOTH_RUN1" : "BOTH_WALK1"; }

// fps der passenden Zeile der animation.cfg der Figur (BOTH_WALK1 / BOTH_RUN1), sonst die der Karten-GLAs (20)
int presetFps(const modell::GangRegler& g) {
    for (const AnimEntry& e : g_app->animList) {
        if (e.name == kategorieZeile(g.art) && e.fps > 0) { return e.fps; }
    }
    return 20;
}

const AnimEntry* quellSequenz(const modell::RetargetDaten& d) {
    for (const AnimEntry& e : rt().quelleCfg) {
        if (e.name == d.sequenz) { return &e; }
    }
    return nullptr;
}

void bereich(const modell::RetargetDaten& d, int& erstes, int& anzahl, double& ms, int& schleife, int& fps) {
    const AnimEntry* e = quellSequenz(d);
    const int alle = rt().quelle.bildZahl();
    if (e != nullptr) {
        erstes = std::clamp(e->firstFrame, 0, std::max(alle - 1, 0));
        anzahl = std::clamp(e->numFrames, 1, std::max(alle - erstes, 1));
        ms = animBildMs(*e);
        schleife = e->loopFrame;
        fps = e->fps;
    } else {
        erstes = 0;
        anzahl = std::max(alle, 1);
        ms = 50.0;
        schleife = -1;
        fps = 20;
    }
}

std::string glaPfad() { return g_app->anim.skeletonName + ".gla"; }

// Naht eines Zyklus: Sprung vom letzten Bild zum ersten (groesster Ort eines Knochens) im Verhaeltnis zum groessten Schritt
// zwischen zwei Bildern im Zyklus - bis etwa 1 faellt die Naht nicht auf (nicht groesser als ein gewoehnlicher Schritt)
float zyklusNaht(const std::vector<std::vector<BoneMatrix>>& w) {
    if (w.size() < 3) { return -1.0F; }
    auto schritt = [&](std::size_t a, std::size_t b) {
        float m = 0.0F;
        for (std::size_t k = 0; k < w[a].size() && k < w[b].size(); ++k) {
            float d = 0.0F;
            for (int i = 0; i < 3; ++i) { d += (w[a][k].m[i][3] - w[b][k].m[i][3]) * (w[a][k].m[i][3] - w[b][k].m[i][3]); }
            m = std::max(m, std::sqrt(d));
        }
        return m;
    };
    float innen = 0.0F;
    for (std::size_t f = 1; f < w.size(); ++f) { innen = std::max(innen, schritt(f - 1, f)); }
    return innen > 1e-6F ? schritt(w.size() - 1, 0) / innen : 0.0F;
}

void zielCharakter() {
    ModellRetarget& r = rt();
    const int vorlage = modellAnimVorlageWahl();   // AP10: -2 suchen, -1 eigenes Skelett (Ketten vorgeschlagen), sonst die gewaehlte
    const std::string fuer = g_app->anim.skeletonName + "|" + std::to_string(g_app->anim.bones.size()) + "|" + std::to_string(vorlage);
    if (r.czFuer == fuer) { return; }
    r.cz = modell::rtCharakter(g_app->anim, glaPfad(), vorlage);
    r.czFuer = fuer;
    r.stand = -1;
}

std::string charakterText(const modell::RtCharakter& c) {
    return c.vorlage.empty() ? format(Str::RtVorgeschlagen, static_cast<int>(c.ketten.size())) : format(Str::RtVorlage, c.vorlage.c_str());
}

std::string ketteName(const modell::RtCharakter& c, int k, const GlaAnimation& g) {
    if (k < 0 || static_cast<std::size_t>(k) >= c.ketten.size()) { return "-"; }
    const modell::RtKette& ke = c.ketten[static_cast<std::size_t>(k)];
    std::string s = ke.schluessel();
    if (!ke.knochen.empty()) {
        s += " (" + g.bones[static_cast<std::size_t>(ke.knochen.front())].name;
        if (ke.knochen.size() > 1) { s += ".." + g.bones[static_cast<std::size_t>(ke.knochen.back())].name; }
        s += ")";
    }
    return s;
}

int gewaehlterKnochen() {
    const ModellTab& t = mt();
    if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size()) { return t.rig.griffe[static_cast<std::size_t>(t.griffWahl)].knochen; }
    if (t.knochenWahl >= 0) { return t.knochenWahl; }
    return rt().poseKnochen;
}

}  // namespace

const modell::RetargetDaten* modellRtDaten() { return &mt().dok.inhalt().rig.rt; }

// pfix M7: zug = ein Regler wird gezogen - der Schritt kommt beim ersten Bild, danach Vorschau (wie die Physik-Regler)
bool modellRtEinstellen(const char* undo, const std::function<void(modell::RetargetDaten&)>& f, bool zug) {
    ModellTab& t = mt();
    if (g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    const auto tun = [&](modell::Inhalt& in) { f(in.rig.rt); };
    if (zug && t.rt.reglerZug) {
        t.dok.vorschau(tun, false);
    } else {
        t.dok.aendern(undo, tun, false);
        t.rt.reglerZug = zug;
    }
    g_app->modelDirty = true;
    return true;
}

namespace {
// Animations-Import (FBX/glTF/BVH/COLLADA, bhed/modell/animimport.h): Skelett und Bilder als Quelle, je Animation der Datei
// eine Sequenz wie in einer animation.cfg - ohne Rueckgaengig-Schritt (auch das Nachladen nach Rueckgaengig, pfix M5)
bool importLesen(const std::string& pfad) {
    ModellRetarget& r = rt();
    modell::AnimImport imp;
    std::string fehler;
    if (!modell::animImportieren(pfad, imp, &fehler)) {
        // FIX2 P14: in der Sprache der Oberflaeche (assimp-Meldungen bleiben, wie assimp sie schreibt)
        r.quelleFehler = imp.fehlerArt.art != modell::FehlerArt::Keiner ? modellFehlerText(imp.fehlerArt) : fehler.empty() ? pfad : fehler;
        melden(format(Str::RtQuelleFehler, r.quelleFehler.c_str()));
        return false;
    }
    r.quelle = std::move(imp.anim);
    r.quelleFuer = pfad;
    r.quelleFehler.clear();
    r.quelleCfg = std::move(imp.sequenzen);
    const std::string vorn = imp.vornAusSkelett ? format(Str::AiVornAusSkelett, imp.vorn.c_str()) : imp.vorn;
    r.importInfo = format(Str::AiImportInfo, imp.format.c_str(), imp.oben.c_str(), vorn.c_str(), static_cast<double>(imp.zentimeter),
                          r.quelleCfg.empty() ? 0 : r.quelleCfg.front().fps);
    for (const modell::ModellFehler& h : imp.hinweise) { r.importInfo += "; " + modellFehlerText(h); }
    r.cq = modell::rtCharakter(r.quelle, pfad);
    zielCharakter();
    r.stand = -1;
    r.bild = 0.0F;
    return true;
}

// Die Quelle lesen (GLA, animation.cfg, Charakter) - ohne Rueckgaengig-Schritt
bool quelleLesen(const std::string& pfad) {
    if (modell::istAnimImportEndung(pfad)) { return importLesen(pfad); }
    ModellRetarget& r = rt();
    std::string bytes;
    std::string fehler;
    // eine Datei auf der Platte (Datei..., Selbsttest) zuerst - readFromArchives nimmt auch lose Dateien der Spielordner, die
    // animation.cfg kaeme dann aus den Archiven des eingestellten Spiels (mit anderen Frames)
    std::error_code ec;
    const bool lose = std::filesystem::path(pfad).is_absolute() && std::filesystem::is_regular_file(pfad, ec);
    if (lose) {
        bytes = slurp(pfad);
    } else if (!readFromArchives(pfad, bytes)) {
        bytes.clear();
    }
    GlaAnimation g;
    if (bytes.empty() || !readGla(bytes, g, &fehler)) {
        r.quelleFehler = fehler.empty() ? pfad : fehler;
        melden(format(Str::RtQuelleFehler, r.quelleFehler.c_str()));
        return false;
    }
    r.quelle = std::move(g);
    r.quelleFuer = pfad;
    r.quelleFehler.clear();
    r.quelleCfg.clear();
    std::string text;
    if (lose) {
        text = slurp(directoryOf(pfad) + "/animation.cfg");
    } else {
        for (const std::string& k : animCfgCandidates(r.quelle.skeletonName)) {
            if (readFromArchives(k, text)) { break; }
            text.clear();
        }
    }
    if (!text.empty()) { r.quelleCfg = parseAnimationCfg(text); }
    r.cq = modell::rtCharakter(r.quelle, pfad);
    zielCharakter();
    r.stand = -1;
    r.bild = 0.0F;
    return true;
}
}  // namespace

// Animations-Import: Datei als Quelle, Vorlage (z. B. Mixamo) erkannt -> zugeordnet, Vorschau sofort auf der Figur
bool modellRtImportLaden(const std::string& pfad, const std::string& sequenz) {
    ModellRetarget& r = rt();
    if (!importLesen(pfad)) { return false; }
    const modell::RtZuordnung zu = modell::rtZuordnen(r.cq, r.cz);
    std::string seq = r.quelleCfg.empty() ? std::string() : r.quelleCfg.front().name;
    for (const AnimEntry& e : r.quelleCfg) {
        if (e.name == sequenz) { seq = sequenz; }
    }
    (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& d) {
        d.quelle = pfad;
        d.vonPreset = false;
        d.zu = zu;
        d.sequenz = seq;
    });
    melden(format(Str::AiImportGeladen, fileName(pfad).c_str(), static_cast<int>(r.quelle.bones.size()), static_cast<int>(r.quelleCfg.size()),
                  charakterText(r.cq).c_str(), r.importInfo.c_str()));
    // auf einen Klick: uebertragen und die Vorschau auf der Figur abspielen
    if (modellRtUebertragen()) { r.spielt = true; }
    return true;
}

bool modellRtImportWaehlen() {
    if (g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    const std::string p = platform::openFileDialog(tr(Str::AiImport), "FBX, glTF, BVH, COLLADA (*.fbx;*.glb;*.gltf;*.bvh;*.dae)|*.fbx;*.glb;*.gltf;*.bvh;*.dae",
                                                   g_app->settings.lastDir);
    if (p.empty()) { return false; }
    return modellRtImportLaden(p);
}

bool modellRtQuelleLaden(const std::string& pfad) {
    if (modell::istAnimImportEndung(pfad)) { return modellRtImportLaden(pfad); }
    ModellRetarget& r = rt();
    if (!quelleLesen(pfad)) { return false; }
    // die Quelle gehoert ins Dokument (Rueckgaengig); Zuordnung neu (nur leere: es gibt noch keine von Hand)
    const modell::RtZuordnung zu = modell::rtZuordnen(r.cq, r.cz);
    const std::string seq = r.quelleCfg.empty() ? std::string() : r.quelleCfg.front().name;
    (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& d) {
        d.quelle = pfad;
        d.vonPreset = false;
        d.zu = zu;
        if (std::none_of(r.quelleCfg.begin(), r.quelleCfg.end(), [&](const AnimEntry& e) { return e.name == d.sequenz; })) { d.sequenz = seq; }
    });
    melden(format(Str::RtQuelleGeladen, pfad.c_str(), static_cast<int>(r.quelle.bones.size()), charakterText(r.cq).c_str()));
    return true;
}

int modellRtZuordnen(bool alle) {
    ModellRetarget& r = rt();
    zielCharakter();
    if (r.quelle.bones.empty()) {
        melden(tr(Str::RtQuelleKeine));
        return 0;
    }
    const modell::RetargetDaten* d = modellRtDaten();
    const modell::RtZuordnung neu = modell::rtZuordnen(r.cq, r.cz, alle ? nullptr : &d->zu);
    int vorher = 0;
    int nachher = 0;
    for (const int j : d->zu.quelle) { vorher += j >= 0 ? 1 : 0; }
    for (const int j : neu.quelle) { nachher += j >= 0 ? 1 : 0; }
    (void)modellRtEinstellen(tr(Str::RtUndoZuordnung), [&](modell::RetargetDaten& x) { x.zu = neu; });
    melden(format(Str::RtZugeordnet, nachher, static_cast<int>(r.cz.ketten.size()), nachher - vorher));
    return nachher;
}

bool modellRtHaltung(bool ziel, modell::RtHaltung h) {
    return modellRtEinstellen(tr(Str::RtUndoPose), [&](modell::RetargetDaten& d) { (ziel ? d.e.poseZiel : d.e.poseQuelle).haltung = h; });
}

bool modellRtPoseSpeichern(const std::string& name) {
    if (name.empty()) { return false; }
    return modellRtEinstellen(tr(Str::RtUndoPose), [&](modell::RetargetDaten& d) {
        d.posen.erase(std::remove_if(d.posen.begin(), d.posen.end(), [&](const modell::RtBenanntePose& p) { return p.name == name; }), d.posen.end());
        d.posen.push_back(modell::RtBenanntePose{name, false, d.e.poseQuelle});
        d.posen.push_back(modell::RtBenanntePose{name, true, d.e.poseZiel});
    });
}

// Rechnen, wenn sich Dokument, Quelle oder Ziel geaendert haben (je Rechnung wenige ms, auch fuer lange Sequenzen)
void modellRtAktualisieren() {
    ModellRetarget& r = rt();
    ModellTab& t = mt();
    const modell::RetargetDaten& d = *modellRtDaten();
    if (g_app->anim.bones.empty() || (r.quelle.bones.empty() && !d.vonPreset)) { return; }
    zielCharakter();
    if (d.vonPreset) {
        // Stufe 5b: der Zyklus des Generators auf dem Skelett der Figur
        const std::string fuer = "preset|" + r.czFuer;
        if (r.stand == t.dok.stand() && r.fuer == fuer) { return; }
        const auto t0 = std::chrono::steady_clock::now();
        const int fps = presetFps(d.gang);
        r.bericht = modell::RtBericht{};
        if (!modell::rtGang(g_app->anim, r.cz, d.gang, fps, r.welt, &r.bericht.fehler)) { r.welt.clear(); }
        r.weltQuelle.clear();
        r.bildMs = 1000.0 / static_cast<double>(fps);
        std::string fehler;
        if (r.welt.empty() || !modell::rtBacken(g_app->anim, r.welt, modellZielKopf(), g_gebacken, &fehler)) { g_gebacken = GlaAnimation{}; }
        r.gang = modell::rtGangPruefen(g_app->anim, r.cz, d.gang, fps);
        r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        ++r.rechnungen;
        r.stand = t.dok.stand();
        r.fuer = fuer;
        r.bild = std::clamp(r.bild, 0.0F, static_cast<float>(std::max(static_cast<int>(r.welt.size()) - 1, 0)));
        g_app->modelDirty = true;
        return;
    }
    // pfix M5: Rueckgaengig/Wiederholen von "Quelle laden" - das Dokument nennt eine andere Quelle als die gelesene: sie neu lesen
    // (ohne Schritt), sonst traefe die Zuordnung der einen die Ketten der anderen
    if (!d.quelle.empty() && d.quelle != r.quelleFuer && !quelleLesen(d.quelle)) { return; }
    const std::string fuer = r.quelleFuer + "|" + d.sequenz + "|" + r.czFuer;
    if (r.stand == t.dok.stand() && r.fuer == fuer) { return; }
    // FIX612C (PV5): die Zuordnung gilt fuer die Ketten einer Rig-Vorlage (die Wahl der Vorlage steht nicht im Dokument). Wechselt die
    // Vorlage auf andere Ketten, bleibt die Zuordnung der alten gemerkt (je Modell, Quelle und Vorlage) und kommt beim Zurueckwechseln
    // wieder - auch die von Hand gesetzten Ketten
    static std::map<std::string, modell::RtZuordnung> gemerkt;
    static std::string zuFuer;   // die Vorlage, zu deren Ketten d.zu gehoert
    const std::string schluessel = std::to_string(t.dok.ladeStand()) + "|" + r.quelleFuer + "|";
    if (d.zu.quelle.size() != r.cz.ketten.size()) {
        if (!d.zu.quelle.empty() && !zuFuer.empty()) { gemerkt[schluessel + zuFuer] = d.zu; }
        const auto alt = gemerkt.find(schluessel + r.czFuer);
        // das Ziel hat andere Ketten (anderes Modell, andere Vorlage): die gemerkte Zuordnung dieser Vorlage, sonst neu vorgeschlagen -
        // ohne eigenen Rueckgaengig-Schritt
        const modell::RtZuordnung zu = alt != gemerkt.end() && alt->second.quelle.size() == r.cz.ketten.size() ? alt->second : modell::rtZuordnen(r.cq, r.cz);
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.rt.zu = zu; }, false);
    }
    zuFuer = r.czFuer;
    int erstes = 0;
    int anzahl = 0;
    int schleife = -1;
    int fps = 20;
    bereich(d, erstes, anzahl, r.bildMs, schleife, fps);
    const auto t0 = std::chrono::steady_clock::now();
    (void)modell::rtUebertragen(r.quelle, r.cq, g_app->anim, r.cz, t.dok.inhalt().rig.rt.zu, d.e, erstes, anzahl, r.welt, &r.bericht);
    r.weltNaht = schleife >= 0 ? zyklusNaht(r.welt) : -1.0F;
    r.weltQuelle.assign(static_cast<std::size_t>(r.welt.size()), {});
    for (std::size_t f = 0; f < r.welt.size(); ++f) { r.quelle.worldMatrices(erstes + static_cast<int>(f), r.weltQuelle[f]); }
    std::string fehler;
    if (r.welt.empty() || !modell::rtBacken(g_app->anim, r.welt, modellZielKopf(), g_gebacken, &fehler)) { g_gebacken = GlaAnimation{}; }
    r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    ++r.rechnungen;
    r.stand = t.dok.stand();
    r.fuer = fuer;
    r.bild = std::clamp(r.bild, 0.0F, static_cast<float>(std::max(static_cast<int>(r.welt.size()) - 1, 0)));
    g_app->modelDirty = true;
}

bool modellRtUebertragen() {
    ModellRetarget& r = rt();
    if (modellRtDaten()->vonPreset && !r.quelle.bones.empty()) {
        (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [](modell::RetargetDaten& d) { d.vonPreset = false; });
    }
    if (r.quelle.bones.empty()) {
        melden(tr(Str::RtQuelleKeine));
        r.quelleWaehlen = true;
        return false;
    }
    r.stand = -1;
    modellRtAktualisieren();
    if (!r.bericht.fehler.empty() || r.welt.empty()) {
        melden(format(Str::RtQuelleFehler, modellFehlerText(r.bericht.fehler).c_str()));
        return false;
    }
    r.vorschau = true;
    r.oeffnen = true;
    melden(format(Str::RtErgebnis, r.bericht.knochenMitQuelle, static_cast<int>(g_app->anim.bones.size()), r.bericht.kettenZugeordnet,
                  static_cast<double>(r.bericht.k), r.ms));
    g_app->modelDirty = true;
    return true;
}

bool modellRtVorschau(bool an) {
    ModellRetarget& r = rt();
    if (an && r.welt.empty() && !modellRtUebertragen()) { return false; }
    r.vorschau = an;
    if (!an) { r.spielt = false; }
    g_app->modelDirty = true;
    return true;
}

// Die Uhr der Vorschau: je Bild des Programms, neben der Uhr der Zeitleiste (drawModelPanel) - nicht im Rollout, das zugeklappt
// oder (Messung) gar nicht gezeichnet sein kann
void modellRtUhr() {
    ModellRetarget& r = rt();
    if (!r.vorschau || !r.spielt || r.welt.empty()) { return; }
    r.bild += ImGui::GetIO().DeltaTime * 1000.0F / static_cast<float>(std::max(r.bildMs, 1.0));
    const float n = static_cast<float>(r.welt.size());
    if (r.bild >= n - 1.0F) { r.bild = std::fmod(r.bild, std::max(n - 1.0F, 1.0F)); }
    g_app->modelDirty = true;
}

bool modellRtPose(std::vector<BoneMatrix>& welt) {
    const ModellRetarget& r = rt();
    if (!r.vorschau || g_gebacken.empty() || g_gebacken.bones.size() != g_app->anim.bones.size()) { return false; }
    const int n = g_gebacken.numFrames;
    const int a = std::clamp(static_cast<int>(std::floor(r.bild)), 0, n - 1);
    const int b = std::min(a + 1, n - 1);
    // wie das Spiel: die lokalen Matrizen zwischen zwei gepackten Bildern linear gemischt (tr_ghoul2.cpp:1531)
    g_gebacken.worldMatricesLerp(a, b, r.bild - static_cast<float>(a), welt);
    return true;
}

bool modellRtBacken() {
    ModellRetarget& r = rt();
    if (r.welt.empty() && !modellRtUebertragen()) {
        melden(tr(Str::RtKeinErgebnis));
        return false;
    }
    modellRtAktualisieren();
    const modell::RetargetDaten& d = *modellRtDaten();
    int erstes = 0;
    int anzahl = 0;
    int schleife = -1;
    int fps = 20;
    double ms = 50.0;
    bereich(d, erstes, anzahl, ms, schleife, fps);
    if (d.vonPreset) {
        // der Zyklus des Generators: Schleife ab Bild 0, fps der Kategorie
        anzahl = static_cast<int>(r.welt.size());
        schleife = 0;
        fps = presetFps(d.gang);
    }
    GlaAnimation g;
    std::string fehler;
    if (!modell::rtBacken(g_app->anim, r.welt, modellZielKopf(), g, &fehler)) {
        melden(format(Str::RtQuelleFehler, modellFehlerText(fehler).c_str()));
        return false;
    }
    const std::string name = d.vonPreset ? std::string(kategorieZeile(d.gang.art)) : (d.sequenz.empty() ? std::string("retarget") : d.sequenz);
    const std::string platz = modellGlaAufPlatz(std::move(g), name, schleife, fps, d.platz.empty() ? name : d.platz);
    if (platz.empty()) { return false; }
    melden(format(Str::RtGebacken, name.c_str(), fileName(glaPfad()).c_str(), anzahl, fps, platz.c_str()));
    return true;
}

// --- Ueberlagerung: die Quelle als Skelett neben der Figur -------------------------------------------------------------

void modellRtUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellRetarget& r = rt();
    r.gezeichnet = 0;
    if (!r.vorschau || !r.quelleZeigen || r.weltQuelle.empty()) { return; }
    const std::vector<BoneMatrix>& w = r.weltQuelle[static_cast<std::size_t>(std::clamp(static_cast<int>(r.bild), 0, static_cast<int>(r.weltQuelle.size()) - 1))];
    // neben der Figur: auf der waagrechten Achse (X oder Y), die in dieser Ansicht am weitesten nach rechts zeigt; Abstand ihre
    // Breite und die der Quelle
    float breite = 0.0F;
    for (int k = 0; k < 2; ++k) { breite = std::max(breite, g_app->model.maxs[k] - g_app->model.mins[k]); }
    float qBreite = 0.0F;
    for (const BoneMatrix& m : w) { qBreite = std::max({qBreite, std::fabs(m.m[0][3]), std::fabs(m.m[1][3])}); }
    const float abstand = std::max(breite, 32.0F) * 0.5F + qBreite * 1.1F;
    float versatz[3] = {0.0F, 0.0F, 0.0F};
    {
        const float null[3] = {0.0F, 0.0F, 0.0F};
        ImVec2 s0;
        float best = 0.0F;
        if (modellAufSchirm(a, null, s0)) {
            for (int k = 0; k < 2; ++k) {
                float p[3] = {0.0F, 0.0F, 0.0F};
                p[k] = abstand;
                ImVec2 sk;
                if (modellAufSchirm(a, p, sk) && std::fabs(sk.x - s0.x) > std::fabs(best)) {
                    best = sk.x - s0.x;
                    versatz[0] = versatz[1] = 0.0F;
                    versatz[k] = best > 0.0F ? abstand : -abstand;
                }
            }
        }
    }
    for (int k = 0; k < 3; ++k) { r.versatz[k] = versatz[k]; }
    const ImU32 farbe = IM_COL32(80, 210, 230, 230);
    const ImU32 blass = IM_COL32(80, 210, 230, 110);
    const float dicke = std::max(1.5F, ImGui::GetFontSize() * 0.12F);
    for (std::size_t b = 0; b < w.size() && b < r.quelle.bones.size(); ++b) {
        // die Linie zum Anker (Kette: das vorige Glied, Finger: die Hand), sonst zum Eltern - im flachen JK2-Skelett haengen Arme und
        // Finger an thoracic, ueber die Eltern sah die Quelle wie ein Stern aus
        const int p = b < r.cq.folgt.size() && r.cq.folgt[b] >= 0 ? r.cq.folgt[b] : r.quelle.bones[b].parent;
        const bool inKette = r.cq.ketteVon(static_cast<int>(b)) >= 0;
        const float pb[3] = {w[b].m[0][3] + versatz[0], w[b].m[1][3] + versatz[1], w[b].m[2][3] + versatz[2]};
        ImVec2 sb;
        if (!modellAufSchirm(a, pb, sb)) { continue; }
        dl->AddCircleFilled(sb, inKette ? dicke * 1.6F : dicke, inKette ? farbe : blass, 10);
        ++r.gezeichnet;
        if (p < 0 || static_cast<std::size_t>(p) >= w.size() || r.quelle.bones[static_cast<std::size_t>(p)].parent < 0) { continue; }
        const BoneMatrix& m = w[static_cast<std::size_t>(p)];
        const float pp[3] = {m.m[0][3] + versatz[0], m.m[1][3] + versatz[1], m.m[2][3] + versatz[2]};
        ImVec2 sp;
        if (modellAufSchirm(a, pp, sp)) { dl->AddLine(sp, sb, inKette ? farbe : blass, dicke); }
    }
    // Beschriftung ueber dem Kopf der Quelle
    float oben[3] = {versatz[0], versatz[1], -1e30F};
    for (const BoneMatrix& m : w) { oben[2] = std::max(oben[2], m.m[2][3]); }
    oben[2] += 6.0F;
    ImVec2 so;
    if (modellAufSchirm(a, oben, so)) {
        const std::string txt = tr(Str::RtQuelleKurz) + std::string(": ") + fileName(r.quelleFuer);
        const ImVec2 g = ImGui::CalcTextSize(txt.c_str());
        // ganz in der Ansicht (sonst am rechten Rand abgeschnitten)
        const float x = std::clamp(so.x - g.x * 0.5F, a.lo[0] + 4.0F, a.lo[0] + a.groesse[0] - g.x - 4.0F);
        const float y = std::clamp(so.y - g.y, a.lo[1] + g.y * 1.5F, a.lo[1] + a.groesse[1] - g.y - 4.0F);
        dl->AddText(ImVec2{x, y}, farbe, txt.c_str());
    }
}

// --- Quelle waehlen ----------------------------------------------------------------------------------------------------

void drawModellRtQuelleFenster() {
    ModellRetarget& r = rt();
    if (r.quelleWaehlen) {
        ImGui::OpenPopup("###mtrtquelle");
        r.quelleWaehlen = false;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 30.0F, fs * 24.0F}, ImGuiCond_Appearing);
    if (!ui::modalBegin((std::string(tr(Str::RtFensterTitel)) + "###mtrtquelle").c_str(), nullptr)) { return; }
    static std::vector<std::string> glas;
    if (glas.empty()) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".gla"})) { glas.push_back(f.name); }
        }
        std::sort(glas.begin(), glas.end());
        glas.erase(std::unique(glas.begin(), glas.end()), glas.end());
    }
    static char suche[96] = "";
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##rtsuche", tr(Str::MbSuche), suche, sizeof suche);
    const std::string such = klein(suche);
    ImGui::BeginChild("##rtliste", ImVec2{-1.0F, -ImGui::GetFrameHeightWithSpacing()}, ImGuiChildFlags_Borders);
    for (const std::string& g : glas) {
        if (!such.empty() && klein(g).find(such) == std::string::npos) { continue; }
        if (ImGui::Selectable(g.c_str(), g == r.quelleFuer)) {
            (void)modellRtQuelleLaden(g);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();
    if (ImGui::Button(tr(Str::RtDatei))) {
        const std::string p = platform::openFileDialog(tr(Str::RtFensterTitel),
                                                       "Ghoul2 animation (*.gla)|*.gla|FBX, glTF, BVH, COLLADA (*.fbx;*.glb;*.gltf;*.bvh;*.dae)|*.fbx;*.glb;*.gltf;*.bvh;*.dae",
                                                       g_app->settings.lastDir);
        if (!p.empty()) {
            (void)modellRtQuelleLaden(p);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SetItemTooltip("%s", tr(Str::RtDateiHint));
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EditorCancel))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// --- Gruppe "Uebertragen (Retargeting)" (AP10, Entwurf A5) -----------------------------------------------------------------
// Form des Entwurfs: Quelle und Ziel als Tabelle, die Teile "Grundpose ausrichten" und "Ketten zuordnen" als Knoepfe im Raster
// (sie klappen ihren Teil auf), die Ketten als Uebersicht, die Schritte als Schalterliste, der Hauptknopf, das Fussrutschen als
// Wert. Der Inhalt ist der von rc611 (Quelle waehlen, Import, Sequenz, Vorschau, Zuordnung, Pose, Schritte, Pruefen, Platz).

namespace {

// Die Ketten des Ziels in den Gruppen des Entwurfs (Wirbelsaeule und Kopf, Arme, Beine, Finger, Anhaenge): wie viele zugeordnet sind.
void kettenUebersicht(const ModellRetarget& r, const modell::RetargetDaten& d) {
    struct Gruppe {
        Str name;
        int alle = 0;
        int zugeordnet = 0;
    };
    Gruppe gruppen[5] = {{Str::AzKetteWirbel}, {Str::AzKetteArme}, {Str::AzKetteBeine}, {Str::AzKetteFinger}, {Str::AzKetteAnhang}};
    for (std::size_t i = 0; i < r.cz.ketten.size(); ++i) {
        int wo = 0;
        switch (r.cz.ketten[i].art) {
            case modell::KettenArt::Wirbel: case modell::KettenArt::Hals: case modell::KettenArt::Kopf: wo = 0; break;
            case modell::KettenArt::Schluessel: case modell::KettenArt::Arm: wo = 1; break;
            case modell::KettenArt::Bein: wo = 2; break;
            case modell::KettenArt::Finger: wo = 3; break;
            case modell::KettenArt::Anhang: wo = 4; break;
        }
        ++gruppen[wo].alle;
        if (i < d.zu.quelle.size() && d.zu.quelle[i] >= 0) { ++gruppen[wo].zugeordnet; }
    }
    std::vector<ui::WertZeile> zeilen;
    for (const Gruppe& x : gruppen) {
        if (x.alle == 0) { continue; }
        zeilen.push_back({tr(x.name), format(Str::AzKettenWert, x.zugeordnet, x.alle), x.zugeordnet == x.alle ? ui::Ton::Gut : ui::Ton::Warnung, tr(Str::RtKettenHint)});
    }
    ui::wertTabelle(zeilen.data(), static_cast<int>(zeilen.size()));
}

// Ein Teil der Gruppe, den ein Knopf des Rasters auf- und zuklappt (der Zustand steht wie bei den Gruppen im Fenster).
bool teilOffen(const char* kennung, bool oeffnen) {
    ImGuiStorage* speicher = ImGui::GetStateStorage();
    const ImGuiID id = ImGui::GetID(kennung);
    if (oeffnen) { speicher->SetBool(id, true); }
    return speicher->GetBool(id, false);
}

void teilUmschalten(const char* kennung) {
    ImGuiStorage* speicher = ImGui::GetStateStorage();
    const ImGuiID id = ImGui::GetID(kennung);
    speicher->SetBool(id, !speicher->GetBool(id, false));
}

// Ketten-Zuordnung: je Kette des Ziels die Kette der Quelle
void teilKetten(ModellRetarget& r, const modell::RetargetDaten& d) {
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::RtAutoLeer), {.icon = UiIcon::Erzeugen, .hinweis = tr(Str::RtAutoLeerHint)})) { (void)modellRtZuordnen(false); }
    if (ui::werkzeugKnopf(tr(Str::RtAutoAlle), {.icon = UiIcon::Neuladen, .hinweis = tr(Str::RtAutoAlleHint)})) { (void)modellRtZuordnen(true); }
    ui::rasterEnde();
    if (!ImGui::BeginTable("##rtketten", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchSame)) { return; }
    ImGui::TableSetupColumn(tr(Str::RtSpalteZiel));
    ImGui::TableSetupColumn(tr(Str::RtSpalteQuelle));
    ImGui::TableHeadersRow();
    for (std::size_t i = 0; i < r.cz.ketten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        const std::string zn = ketteName(r.cz, static_cast<int>(i), g_app->anim);
        cjkTextUnformatted(zn.c_str());
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", zn.c_str()); }
        ImGui::TableNextColumn();
        const int j = i < d.zu.quelle.size() ? d.zu.quelle[i] : -1;
        const bool hand = i < d.zu.vonHand.size() && d.zu.vonHand[i] != 0;
        ImGui::SetNextItemWidth(-1.0F);
        const std::string kurz = j >= 0 && static_cast<std::size_t>(j) < r.cq.ketten.size() ? r.cq.ketten[static_cast<std::size_t>(j)].schluessel() : "-";
        if (ImGui::BeginCombo("##q", (kurz + (hand ? " *" : "")).c_str())) {
            for (int k = -1; k < static_cast<int>(r.cq.ketten.size()); ++k) {
                if (ImGui::Selectable(ketteName(r.cq, k, r.quelle).c_str(), k == j)) {
                    (void)modellRtEinstellen(tr(Str::RtUndoZuordnung), [&](modell::RetargetDaten& x) {
                        x.zu.quelle.resize(r.cz.ketten.size(), -1);
                        x.zu.vonHand.resize(r.cz.ketten.size(), 0);
                        x.zu.quelle[i] = k;
                        x.zu.vonHand[i] = 1;
                    });
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("%s\n%s", ketteName(r.cq, j, r.quelle).c_str(), tr(Str::RtKetteHint));
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// Retarget-Pose: Haltung von Quelle und Ziel, Abweichung, der gewaehlte Knochen von Hand, benannte Posen
void teilPose(ModellRetarget& r, const modell::RetargetDaten& d) {
    const char* haltungen[3] = {tr(Str::RtHaltungBinde), tr(Str::RtHaltungT), tr(Str::RtHaltungA)};
    for (const bool ziel : {false, true}) {
        int h = static_cast<int>(ziel ? d.e.poseZiel.haltung : d.e.poseQuelle.haltung);
        ui::zeile(tr(ziel ? Str::RtPoseZiel : Str::RtPoseQuelle), tr(Str::RtHaltungHint));
        if (ImGui::Combo(ziel ? "##rthaltungziel" : "##rthaltungquelle", &h, haltungen, 3)) { (void)modellRtHaltung(ziel, static_cast<modell::RtHaltung>(h)); }
        ImGui::SetItemTooltip("%s", tr(Str::RtHaltungHint));
    }
    const float ab = r.bericht.poseAbweichung;
    ui::absatz(format(Str::RtPoseAbweichung, static_cast<double>(ab)).c_str(), ab > 5.0F ? ui::Ton::Warnung : ui::Ton::Leise);
    // von Hand: der gewaehlte Knochen des Ziels um seine Achsen
    const int k = gewaehlterKnochen();
    if (k >= 0 && static_cast<std::size_t>(k) < g_app->anim.bones.size()) {
        ui::absatz(format(Str::RtPoseKnochen, g_app->anim.bones[static_cast<std::size_t>(k)].name.c_str()).c_str(), ui::Ton::Normal);
        static const char* const kAchsen[3] = {"X", "Y", "Z"};
        for (int ach = 0; ach < 3; ++ach) {
            for (const float grad : {-5.0F, 5.0F}) {
                ImGui::PushID(ach * 2 + (grad > 0.0F ? 1 : 0));
                char b[16];
                std::snprintf(b, sizeof b, "%s%s5", kAchsen[ach], grad > 0.0F ? "+" : "-");
                if (ach > 0 || grad > 0.0F) { ImGui::SameLine(); }
                if (ImGui::SmallButton(b)) {
                    (void)modellRtEinstellen(tr(Str::RtUndoPose), [&](modell::RetargetDaten& x) {
                        std::vector<std::array<float, 4>>& v = x.e.poseZiel.vonHand;
                        if (v.size() != g_app->anim.bones.size()) { v.assign(g_app->anim.bones.size(), {1.0F, 0.0F, 0.0F, 0.0F}); }
                        float achse[3] = {0.0F, 0.0F, 0.0F};
                        achse[ach] = 1.0F;
                        const BoneMatrix m = modell::matrixAusQuat(v[static_cast<std::size_t>(k)].data()) * modell::drehungUm(achse, grad);
                        float q[4];
                        modell::quatAusMatrix(m, q);
                        v[static_cast<std::size_t>(k)] = {q[0], q[1], q[2], q[3]};
                    });
                }
                ImGui::SetItemTooltip("%s", tr(Str::RtPoseDrehenHint));
                ImGui::PopID();
            }
        }
    } else {
        ui::absatz(tr(Str::RtKnochenWahl));
    }
    if (ui::werkzeugKnopf(tr(Str::RtPoseZurueck), {.icon = UiIcon::Rueckgaengig, .hinweis = tr(Str::RtPoseZurueckHint)})) {
        (void)modellRtEinstellen(tr(Str::RtUndoPose), [](modell::RetargetDaten& x) {
            x.e.poseZiel.vonHand.clear();
            x.e.poseQuelle.vonHand.clear();
        });
    }
    // benannte Posen
    ui::zeile(tr(Str::RtPoseName));
    ImGui::InputText("##rtpname", r.poseName, sizeof r.poseName);
    if (ui::werkzeugKnopf(tr(Str::RtPoseSpeichern), {.icon = UiIcon::Speichern, .hinweis = tr(Str::RtPoseSpeichernHint)})) { (void)modellRtPoseSpeichern(r.poseName); }
    bool erste = true;
    for (std::size_t i = 0; i < d.posen.size(); ++i) {
        if (!d.posen[i].ziel) { continue; }
        ImGui::PushID(static_cast<int>(i));
        if (!erste) { daneben(d.posen[i].name.c_str()); }
        erste = false;
        if (ImGui::SmallButton(d.posen[i].name.c_str())) {
            const std::string n = d.posen[i].name;
            (void)modellRtEinstellen(tr(Str::RtUndoPose), [&](modell::RetargetDaten& x) {
                for (const modell::RtBenanntePose& p : x.posen) {
                    if (p.name == n) { (p.ziel ? x.e.poseZiel : x.e.poseQuelle) = p.pose; }
                }
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtPoseLadenHint));
        ImGui::PopID();
    }
}

// Reihenfolge der Schritte, Reach, Laenge von Hand
void teilSchritte(ModellRetarget& r, const modell::RetargetDaten& d) {
    static const Str kName[4] = {Str::RtSchrittBecken, Str::RtSchrittFk, Str::RtSchrittIk, Str::RtSchrittWurzel};
    static const Str kHint[4] = {Str::RtSchrittBeckenHint, Str::RtSchrittFkHint, Str::RtSchrittIkHint, Str::RtSchrittWurzelHint};
    for (std::size_t i = 0; i < 4; ++i) {
        ImGui::PushID(static_cast<int>(i));
        const int si = static_cast<int>(d.e.reihenfolge[i]);
        ImGui::BeginDisabled(i == 0);
        if (ImGui::ArrowButton("##hoch", ImGuiDir_Up)) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { std::swap(x.e.reihenfolge[i], x.e.reihenfolge[i - 1]); });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtHochHint));
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(i == 3);
        if (ImGui::ArrowButton("##runter", ImGuiDir_Down)) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { std::swap(x.e.reihenfolge[i], x.e.reihenfolge[i + 1]); });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtRunterHint));
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        cjkTextUnformatted(tr(kName[si]));
        ImGui::SetItemTooltip("%s", tr(kHint[si]));
        ImGui::PopID();
    }
    const auto regler = [&](Str text, Str hinweis, float wert, const std::function<void(modell::RtEinstellung&, float)>& setzen) {
        float v = wert;
        if (ui::regler(tr(text), &v, 0.0F, 1.0F, "%.2f", tr(hinweis))) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { setzen(x.e, v); }, true);
        }
        if (ImGui::IsItemDeactivated()) { r.reglerZug = false; }
    };
    regler(Str::RtReachFuss, Str::RtReachHint, d.e.reachFuss, [](modell::RtEinstellung& x, float v) { x.reachFuss = v; });
    regler(Str::RtReachHand, Str::RtReachHint, d.e.reachHand, [](modell::RtEinstellung& x, float v) { x.reachHand = v; });
    bool vonHand = d.e.laengeVonHand > 0.0F;
    if (ui::schalter(tr(Str::RtLaengeVonHand), &vonHand, tr(Str::RtLaengeHint))) {
        const float k = r.bericht.k;
        (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.laengeVonHand = vonHand ? std::max(k, 0.01F) : 0.0F; });
    }
    if (vonHand) {
        float k = d.e.laengeVonHand;
        ui::zeileOhne();
        if (ImGui::DragFloat("##rtk", &k, 0.005F, 0.05F, 20.0F, "%.3f")) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.laengeVonHand = k; }, true);
        }
        if (ImGui::IsItemDeactivated()) { r.reglerZug = false; }
        ImGui::SetItemTooltip("%s", tr(Str::RtLaengeHint));
    } else {
        ui::absatz(format(Str::RtLaengeWert, static_cast<double>(r.bericht.k)).c_str());
    }
}

}  // namespace

void drawModellRetargetBereich() {
    ModellRetarget& r = rt();
    if (g_app->anim.bones.empty()) { return; }
    modellRtAktualisieren();
    drawModellRtQuelleFenster();
    if (r.oeffnen) {
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
        ui::rolloutZeigen(tr(Str::AzRUebertragen));   // aufklappen und ins Bild rollen (Menue, Rechtsklick, Suche)
        r.oeffnen = false;
    }
    const std::string kurz = r.quelle.bones.empty() ? std::string(tr(Str::AzRtInfoKeine)) : fileName(r.quelleFuer);
    if (!ui::rolloutBeginn(tr(Str::AzRUebertragen), {.kurzinfo = kurz.c_str(), .hinweis = tr(Str::RtRolloutHint)})) { return; }
    r.rolloutBild = ImGui::GetFrameCount();
    zielCharakter();
    const modell::RetargetDaten d = *modellRtDaten();
    const float fs = ImGui::GetFontSize();
    // Quelle und Ziel
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::RtQuelleWaehlen), {.icon = UiIcon::Oeffnen, .hinweis = tr(Str::RtQuelleWaehlenHint)})) { r.quelleWaehlen = true; }
    if (ui::werkzeugKnopf(tr(Str::AiImport), {.icon = UiIcon::Paket, .hinweis = tr(Str::AiImportHint)})) { (void)modellRtImportWaehlen(); }
    ui::rasterEnde();
    if (!r.importInfo.empty() && modell::istAnimImportEndung(r.quelleFuer)) { ui::absatz(r.importInfo.c_str()); }
    if (r.quelle.bones.empty()) {
        ui::tipp(tr(Str::RtQuelleKeine));
        ui::rolloutEnde();
        r.baeumeOeffnen = false;
        return;
    }
    const auto skelett = [](const std::string& datei, std::size_t knochen, const modell::RtCharakter& c) {
        char b[400];
        std::snprintf(b, sizeof b, trN(Str::AzSkelettWert, static_cast<int>(knochen)), datei.c_str(), static_cast<int>(knochen));
        return std::string(b) + " \xc2\xb7 " + charakterText(c);
    };
    ui::wertTabelle({{tr(Str::AzQuelle), skelett(fileName(r.quelleFuer), r.quelle.bones.size(), r.cq), ui::Ton::Normal,
                      format(Str::RtQuelle, r.quelleFuer.c_str(), static_cast<int>(r.quelle.bones.size()), charakterText(r.cq).c_str())},
                     {tr(Str::AzZiel), skelett(fileName(glaPfad()), g_app->anim.bones.size(), r.cz), ui::Ton::Normal,
                      format(Str::RtZiel, glaPfad().c_str(), static_cast<int>(g_app->anim.bones.size()), charakterText(r.cz).c_str())}});
    // Sequenz der Quelle
    ui::zeile(tr(Str::RtSequenz), tr(Str::RtSequenzHint));
    if (ImGui::BeginCombo("##rtsequenz", d.sequenz.empty() ? tr(Str::RtSequenzAlle) : d.sequenz.c_str(), ImGuiComboFlags_HeightLarge)) {
        if (ImGui::Selectable(tr(Str::RtSequenzAlle), d.sequenz.empty())) {
            (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [](modell::RetargetDaten& x) { x.sequenz.clear(); });
        }
        for (const AnimEntry& e : r.quelleCfg) {
            if (ImGui::Selectable(e.name.c_str(), e.name == d.sequenz)) {
                const std::string n = e.name;
                (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& x) { x.sequenz = n; });
                r.bild = 0.0F;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::RtSequenzHint));
    // Vorschau
    ui::rasterBeginn(2);
    bool vorschau = r.vorschau;
    if (ui::schalter(tr(Str::RtVorschau), &vorschau, tr(Str::RtVorschauHint))) { (void)modellRtVorschau(vorschau); }
    if (ui::schalter(tr(Str::RtQuelleZeigen), &r.quelleZeigen, tr(Str::RtQuelleZeigenHint))) { g_app->modelDirty = true; }
    ui::rasterEnde();
    if (r.vorschau && !r.welt.empty()) {
        if (ImGui::Button(r.spielt ? tr(Str::RtAnhalten) : tr(Str::RtAbspielen))) { r.spielt = !r.spielt; }
        ImGui::SetItemTooltip("%s", tr(Str::RtAbspielenHint));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::SliderFloat("##rtbild", &r.bild, 0.0F, static_cast<float>(r.welt.size() - 1), "%.0f")) { g_app->modelDirty = true; }
        ImGui::SetItemTooltip("%s", tr(Str::RtBildHint));
    }
    // Grundpose ausrichten, Ketten zuordnen, Schritte: Knoepfe im Raster, die ihren Teil aufklappen
    const bool poseOffen = teilOffen("##rtteilpose", r.baeumeOeffnen);
    const bool kettenOffen = teilOffen("##rtteilketten", r.baeumeOeffnen);
    const bool schritteOffen = teilOffen("##rtteilschritte", r.baeumeOeffnen);
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(begriff(Str::RtPoseMax, Str::RtPoseBlender), {.icon = UiIcon::Figur, .hinweis = tr(Str::RtPoseHint), .an = poseOffen})) { teilUmschalten("##rtteilpose"); }
    if (ui::werkzeugKnopf(begriff(Str::RtKettenMax, Str::RtKettenBlender), {.icon = UiIcon::FeldConstraints, .hinweis = tr(Str::RtKettenHint), .an = kettenOffen})) {
        teilUmschalten("##rtteilketten");
    }
    if (ui::werkzeugKnopf(tr(Str::RtSchritte), {.icon = UiIcon::Liste, .hinweis = tr(Str::RtSchritteHint), .an = schritteOffen})) { teilUmschalten("##rtteilschritte"); }
    ui::rasterEnde();
    kettenUebersicht(r, d);
    if (poseOffen) { teilPose(r, d); }
    if (kettenOffen) { teilKetten(r, d); }
    // die Schritte als Schalterliste (ihre Reihenfolge im Teil "Schritte"), dazu die Fusskontakte
    static const Str kSchritt[4] = {Str::RtSchrittBecken, Str::RtSchrittFk, Str::RtSchrittIk, Str::RtSchrittWurzel};
    static const Str kSchrittHint[4] = {Str::RtSchrittBeckenHint, Str::RtSchrittFkHint, Str::RtSchrittIkHint, Str::RtSchrittWurzelHint};
    ui::rasterBeginn(2);
    for (std::size_t i = 0; i < 4; ++i) {
        const modell::RtSchritt s = d.e.reihenfolge[i];
        const int si = static_cast<int>(s);
        bool an = s == modell::RtSchritt::Becken ? d.e.becken : (s == modell::RtSchritt::Fk ? d.e.fk : (s == modell::RtSchritt::Ik ? d.e.ik : d.e.wurzel));
        if (ui::schalter(tr(kSchritt[si]), &an, tr(kSchrittHint[si]))) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) {
                (s == modell::RtSchritt::Becken ? x.e.becken : (s == modell::RtSchritt::Fk ? x.e.fk : (s == modell::RtSchritt::Ik ? x.e.ik : x.e.wurzel))) = an;
            });
        }
    }
    bool kon = d.e.kontakte;
    if (ui::schalter(tr(Str::RtKontakte), &kon, tr(Str::RtKontakteHint))) {
        (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.kontakte = kon; });
    }
    ui::rasterEnde();
    if (schritteOffen) { teilSchritte(r, d); }
    // Platz und Hauptknopf
    static char platz[48] = {};
    if (d.platz != platz) { std::snprintf(platz, sizeof platz, "%s", d.platz.c_str()); }
    ui::zeile(tr(Str::RtPlatz), tr(Str::RtPlatzHint));
    if (ImGui::InputTextWithHint("##rtplatz", d.sequenz.empty() ? "retarget" : d.sequenz.c_str(), platz, sizeof platz, ImGuiInputTextFlags_EnterReturnsTrue) ||
        ImGui::IsItemDeactivatedAfterEdit()) {
        const std::string p = platz;
        (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& x) { x.platz = p; });
    }
    ImGui::SetItemTooltip("%s", tr(Str::RtPlatzHint));
    if (ui::werkzeugKnopf(begriff(Str::RtBackenMax, Str::RtBackenBlender), {.icon = UiIcon::Abspielen, .hinweis = tr(Str::RtBackenHint), .haupt = true})) { (void)modellRtBacken(); }
    // Pruefen
    if (!r.bericht.fehler.empty()) {
        ui::absatz(format(Str::RtQuelleFehler, modellFehlerText(r.bericht.fehler).c_str()).c_str(), ui::Ton::Fehler);
    } else if (!r.welt.empty()) {
        const bool rutscht = r.bericht.rutschenMax > std::max(0.5F, r.bericht.rutschenQuelle * 1.5F);
        const std::string genau = format(Str::RtRutschen, static_cast<double>(r.bericht.rutschenMax), static_cast<double>(r.bericht.rutschenQuelle)) + "\n" + tr(Str::RtRutschenHint);
        ui::wertTabelle({{tr(Str::AzFussrutschen), format(Str::AzEinheiten, static_cast<double>(r.bericht.rutschenMax)), rutscht ? ui::Ton::Warnung : ui::Ton::Gut, genau}});
        ui::absatz(format(Str::RtErgebnis, r.bericht.knochenMitQuelle, static_cast<int>(g_app->anim.bones.size()), r.bericht.kettenZugeordnet, static_cast<double>(r.bericht.k), r.ms).c_str());
        // Fussrutsch-Messer je Bild (links + rechts), Spruenge rot darunter
        std::vector<float> v(r.bericht.bilder.size());
        for (std::size_t i = 0; i < v.size(); ++i) { v[i] = std::max(r.bericht.bilder[i].rutschen[0], r.bericht.bilder[i].rutschen[1]); }
        if (!v.empty()) {
            const ImVec2 lo = ImGui::GetCursorScreenPos();
            const float breit = ImGui::GetContentRegionAvail().x;
            ImGui::PlotHistogram("##rtmesser", v.data(), static_cast<int>(v.size()), 0, nullptr, 0.0F, std::max(1.0F, r.bericht.rutschenMax), ImVec2{breit, fs * 2.5F});
            ImGui::SetItemTooltip("%s", tr(Str::RtMesserHint));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float h = fs * 2.5F;
            for (const int s : r.bericht.spruenge) {
                const float x = lo.x + (static_cast<float>(s) + 0.5F) / static_cast<float>(v.size()) * breit;
                dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, lo.y + h}, IM_COL32(255, 70, 60, 230), 2.0F);
            }
            const float x = lo.x + (r.bild + 0.5F) / static_cast<float>(v.size()) * breit;
            dl->AddLine(ImVec2{x, lo.y}, ImVec2{x, lo.y + h}, IM_COL32(250, 230, 80, 230), 1.0F);
        }
        if (r.bericht.spruenge.empty()) {
            ui::absatz(tr(Str::RtKeineSpruenge));
        } else {
            ui::absatz(format(Str::RtSpruenge, static_cast<int>(r.bericht.spruenge.size())).c_str(), ui::Ton::Fehler);
            for (std::size_t i = 0; i < r.bericht.spruenge.size() && i < 12; ++i) {
                ImGui::PushID(static_cast<int>(i));
                if (i > 0) { daneben("0000"); }
                if (ImGui::SmallButton(std::to_string(r.bericht.spruenge[i]).c_str())) {
                    r.bild = static_cast<float>(r.bericht.spruenge[i]);
                    r.vorschau = true;
                    r.spielt = false;
                    g_app->modelDirty = true;
                }
                const int sk = i < r.bericht.sprungKnochen.size() ? r.bericht.sprungKnochen[i] : -1;
                ImGui::SetItemTooltip("%s (%s)", tr(Str::RtSprungHint),
                                      sk >= 0 && static_cast<std::size_t>(sk) < g_app->anim.bones.size() ? g_app->anim.bones[static_cast<std::size_t>(sk)].name.c_str() : "-");
                ImGui::PopID();
            }
        }
    }
    r.baeumeOeffnen = false;
    ui::rolloutEnde();
}

// --- Stufe 5b: Presets (Generator) ---------------------------------------------------------------------------------------


bool modellPresetMessen() {
    ModellRetarget& r = rt();
    zielCharakter();
    const modell::RetargetDaten& d = *modellRtDaten();
    const char* zeile = kategorieZeile(d.gang.art);
    for (const AnimEntry& e : g_app->animList) {
        if (e.name != zeile) { continue; }
        float tempo = 0.0F;
        float schritt = 0.0F;
        if (!modell::gangMessen(g_app->anim, r.cz, e.firstFrame, e.numFrames, animBildMs(e), tempo, schritt)) { break; }
        (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) {
            x.gang.tempo = tempo;
            x.gang.schritt = schritt;
        });
        melden(format(Str::PrGemessen, zeile, static_cast<double>(tempo), static_cast<double>(schritt)));
        return true;
    }
    melden(format(Str::PrNichtGemessen, zeile));
    return false;
}

// --- Preset-Bibliothek (data/presets/presets.txt): freie Animationen anderer Urheber als BVH auf ihrem eigenen Skelett ---
// Zeile: kategorie|name|datei (relativ zu data/presets)|quelle|lizenz|platz|schleife (1/0). Ausgeliefert wird nur die Quelle (CC0 bzw.
// CC BY 4.0), keine Datei des Spiels: auf die Figur kommt sie erst hier, mit dem Retargeting auf deren Skelett.
const std::vector<PresetEintrag>& modellPresetBibliothek() {
    ModellRetarget& r = rt();
    if (!r.bibliothekGeladen) {
        r.bibliothekGeladen = true;
        r.bibliothek.clear();
        const std::filesystem::path d = std::filesystem::path(g_app->dataDir).parent_path() / "presets";
        r.bibliothekOrdner = d.string();
        std::istringstream in(slurp((d / "presets.txt").string()));
        for (std::string z; std::getline(in, z);) {
            if (!z.empty() && z.back() == '\r') { z.pop_back(); }
            if (z.empty() || z[0] == '#') { continue; }
            std::vector<std::string> f;
            std::size_t a0 = 0;
            for (std::size_t i = 0; i <= z.size(); ++i) {
                if (i == z.size() || z[i] == '|') {
                    f.push_back(z.substr(a0, i - a0));
                    a0 = i + 1;
                }
            }
            if (f.size() < 7) { continue; }
            r.bibliothek.push_back(PresetEintrag{f[0], f[1], (d / f[2]).string(), f[3], f[4], f[5], f[6] == "1"});
        }
    }
    return r.bibliothek;
}

bool modellPresetBibliothekAuflegen(int i) {
    const std::vector<PresetEintrag>& b = modellPresetBibliothek();
    if (i < 0 || static_cast<std::size_t>(i) >= b.size()) {
        melden(tr(Str::AiKeineBibliothek));
        return false;
    }
    const PresetEintrag& e = b[static_cast<std::size_t>(i)];
    ModellRetarget& r = rt();
    r.bibliothekWahl = i;
    if (!modellRtImportLaden(e.datei)) { return false; }
    // ein Zyklus: Schleife ab Bild 0 (wie die Gaenge des Spiels), sonst einmal; Platz aus der Bibliothek
    for (AnimEntry& a : r.quelleCfg) { a.loopFrame = e.schleife ? 0 : -1; }
    (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.platz = e.platz; });
    r.stand = -1;
    modellRtAktualisieren();
    r.vorschau = !r.welt.empty();
    r.spielt = r.vorschau;
    r.presetOeffnen = true;
    melden(format(Str::AiQuelle, e.quelle.c_str(), e.lizenz.c_str()));
    return r.vorschau;
}

bool modellPresetAuflegen() {
    if (g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    (void)modellRtEinstellen(tr(Str::PrUndo), [](modell::RetargetDaten& x) {
        x.vonPreset = true;
        if (x.platz.empty()) { x.platz = kategorieZeile(x.gang.art); }
    });
    ModellRetarget& r = rt();
    r.stand = -1;
    modellRtAktualisieren();
    if (r.welt.empty()) {
        melden(format(Str::RtQuelleFehler, modellFehlerText(r.bericht.fehler).c_str()));
        return false;
    }
    r.vorschau = true;
    r.spielt = true;
    r.presetOeffnen = true;
    g_app->modelDirty = true;
    return true;
}

// --- Gruppe "Presets" (AP10, Entwurf A3; ENT2) --------------------------------------------------------------------------------
// Form des Entwurfs: die Kategorien als Segment (die acht des Entwurfs, auch leer - ENT2), die Presets der Kategorie als Liste mit
// Quelle und Lizenz, in Gehen und Laufen dazu der Gang des eigenen Generators als Eintrag ("Gehen (erzeugt)", Quelle Generator),
// der Ziel-Platz, beim erzeugten Gang die Regler Tempo und Armschwung, "Vorschau auf Figur" und "Generator ..." im Raster, der
// Hauptknopf "Auf Figur legen (Retarget)", der Tipp. "Generator ..." zeigt beim erzeugten Gang den Rest des Generators (Haltung,
// Schrittlaenge, Hueftschwung, Hub, Messen); Rechtsklick auf einen Eintrag nennt seine Befehle.

namespace {

// Der Platz, den der Nutzer fuer das gewaehlte Preset wuenscht, bevor es geladen ist (danach steht er im Dokument: rt.platz).
char g_presetPlatz[64] = {};
int g_presetPlatzFuer = -1;
ImRect g_presetPlatzKasten;   // Selbsttest: wo die Wahl des Ziel-Platzes steht
bool g_generatorZeigen = false;   // Generator ... ist eben aufgeklappt: den Rest ins Bild rollen

// Die Kategorien des Entwurfs in seiner Folge (Stehen, Gehen, Laufen, Rueckwaerts, Ducken, Springen, Kampf, Gesten), auch leer
// (shank 12.11.: "Wir holen uns noch presets"); was presets.txt sonst nennt (other), dahinter in der Folge der Datei.
constexpr const char* kKategorienEntwurf[8] = {"idle", "walk", "run", "back", "crouch", "jump", "combat", "gesture"};

std::vector<std::string> presetKategorien(const std::vector<PresetEintrag>& b) {
    std::vector<std::string> kat(std::begin(kKategorienEntwurf), std::end(kKategorienEntwurf));
    for (const PresetEintrag& e : b) {
        if (std::find(kat.begin(), kat.end(), e.kategorie) == kat.end()) { kat.push_back(e.kategorie); }
    }
    return kat;
}

// Der Name einer Kategorie; was keine des Entwurfs ist: Sonstiges
const char* presetKategorieName(const std::string& k) {
    if (k == "idle") { return tr(Str::AiKatStehen); }
    if (k == "walk") { return tr(Str::AzKatGehen); }
    if (k == "run") { return tr(Str::AzKatLaufen); }
    if (k == "back") { return tr(Str::AzKatRueck); }
    if (k == "crouch") { return tr(Str::AzKatDucken); }
    if (k == "jump") { return tr(Str::AzKatSpringen); }
    if (k == "combat") { return tr(Str::AiKatKampf); }
    if (k == "gesture") { return tr(Str::AzKatGesten); }
    return tr(Str::AiKatSonst);
}

// Der erzeugte Gang einer Kategorie (modell::GangArt) bzw. -1: Gehen und Laufen kann der Generator
int gangDerKategorie(const std::string& k) {
    if (k == "walk") { return static_cast<int>(modell::GangArt::Gehen); }
    if (k == "run") { return static_cast<int>(modell::GangArt::Laufen); }
    return -1;
}

// Die Quelle kurz fuer die Liste (der Teil vor " - "), dahinter die Lizenz: "Quaternius · CC0 1.0"
std::string quelleKurz(const PresetEintrag& e) {
    const std::size_t p = e.quelle.find(" - ");
    return (p == std::string::npos ? e.quelle : e.quelle.substr(0, p)) + " \xc2\xb7 " + e.lizenz;
}

// Das gewaehlte Preset laden, uebertragen und als Vorschau abspielen; der gewuenschte Platz gilt.
bool presetVorschau(int wahl) {
    if (!modellPresetBibliothekAuflegen(wahl)) { return false; }
    if (g_presetPlatzFuer == wahl && g_presetPlatz[0] != '\0') {
        const std::string p = g_presetPlatz;
        (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.platz = p; });
    }
    return true;
}

// Der erzeugte Gang als Vorschau auf der Figur, auf dem gewuenschten Platz (sonst BOTH_WALK1 / BOTH_RUN1)
bool gangVorschau() {
    const ModellRetarget& r = rt();
    if (r.gangWahl < 0) { return false; }
    const std::string p = g_presetPlatz[0] != '\0' ? std::string(g_presetPlatz) : std::string(kategorieZeile(static_cast<modell::GangArt>(r.gangWahl)));
    (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.platz = p; });
    return modellPresetAuflegen();
}

// Ein Regler des Generators (ein Rueckgaengig-Schritt je Zug, waehrend des Zugs Vorschau)
void gangRegler(ModellRetarget& r, Str text, Str hinweis, float wert, float lo, float hi, const char* fmt, const std::function<void(modell::GangRegler&, float)>& f) {
    float v = wert;
    if (ui::regler(tr(text), &v, lo, hi, fmt, tr(hinweis))) {
        if (r.presetZug) {
            mt().dok.vorschau([&](modell::Inhalt& in) { f(in.rig.rt.gang, v); }, false);
        } else {
            (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { f(x.gang, v); });
            r.presetZug = true;
        }
        g_app->modelDirty = true;
    }
    if (ImGui::IsItemDeactivated()) { r.presetZug = false; }
}

// Der Rest des Generators (Generator ...): Haltung, Schrittlaenge, Hueftschwung, Hub, Messen, Befund
void gangMehr(ModellRetarget& r, const modell::RetargetDaten& d) {
    const char* haltungen[modell::kGangHaltungen] = {tr(Str::PrHaltNormal), tr(Str::PrHaltSchwer), tr(Str::PrHaltSchleichend), tr(Str::PrHaltVerletzt),
                                                     tr(Str::PrHaltSaebel)};
    int h = static_cast<int>(d.gang.haltung);
    ui::zeile(tr(Str::PrHaltung), tr(Str::PrHaltungHint));
    if (ImGui::Combo("##prhaltung", &h, haltungen, modell::kGangHaltungen)) {
        (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.gang.haltung = static_cast<modell::GangHaltung>(h); });
    }
    ImGui::SetItemTooltip("%s", tr(Str::PrHaltungHint));
    gangRegler(r, Str::PrSchritt, Str::PrSchrittHint, d.gang.schritt, 2.0F, 120.0F, "%.1f", [](modell::GangRegler& g, float v) { g.schritt = v; });
    gangRegler(r, Str::PrHuefte, Str::PrFaktorHint, d.gang.hueftschwung, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.hueftschwung = v; });
    gangRegler(r, Str::PrHub, Str::PrFaktorHint, d.gang.hub, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.hub = v; });
    const std::string messen = format(Str::PrMessen, kategorieZeile(d.gang.art));
    if (ui::werkzeugKnopf(messen.c_str(), {.icon = UiIcon::Zielen, .hinweis = tr(Str::PrMessenHint)})) { (void)modellPresetMessen(); }
    if (d.vonPreset && !r.welt.empty()) {
        const modell::GangBefund& bf = r.gang;
        ui::absatz(format(Str::PrBefund, static_cast<double>(modell::gangZyklus(d.gang)), static_cast<int>(r.welt.size()), presetFps(d.gang), static_cast<double>(bf.bodenAbw),
                          static_cast<double>(bf.gleitAbw), static_cast<double>(bf.naht))
                       .c_str());
        ui::absatz(tr(Str::PrLizenz));
    }
}

}  // namespace

ImRect modellPresetPlatzKasten() { return g_presetPlatzKasten; }

void modellPresetGangWaehlen(int art) {
    ModellRetarget& r = rt();
    art = std::clamp(art, 0, 1);
    r.gangWahl = art;
    r.bibliothekWahl = -1;
    r.bibliothekKategorie = art == static_cast<int>(modell::GangArt::Laufen) ? 2 : 1;   // Gehen bzw. Laufen in kKategorienEntwurf
    if (static_cast<int>(modellRtDaten()->gang.art) != art) {
        (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) {
            x.gang.art = static_cast<modell::GangArt>(art);
            x.gang.tempo = art == static_cast<int>(modell::GangArt::Laufen) ? 300.0F : 90.0F;   // runSpeed / walkSpeed der NPCs (NPC_stats.cpp:1996-1997)
        });
        (void)modellPresetMessen();
    }
}

void drawModellPresetBereich() {
    ModellRetarget& r = rt();
    if (g_app->anim.bones.empty()) { return; }
    const modell::RetargetDaten d = *modellRtDaten();
    if (r.presetOeffnen) {
        ui::rolloutSetzen(tr(Str::PrRollout), true);   // aufklappen (ins Bild rollen die Aktionen "... oeffnen" selbst)
        // ein Gang aus dem Generator liegt auf der Figur: sein Eintrag ist gewaehlt
        if (d.vonPreset && r.gangWahl < 0) { modellPresetGangWaehlen(static_cast<int>(d.gang.art)); }
        r.presetOeffnen = false;
    }
    const std::vector<PresetEintrag>& b = modellPresetBibliothek();
    const std::vector<std::string> kat = presetKategorien(b);
    r.bibliothekKategorie = std::clamp(r.bibliothekKategorie, 0, static_cast<int>(kat.size()) - 1);
    const std::string& wk = kat[static_cast<std::size_t>(r.bibliothekKategorie)];
    const int gangHier = gangDerKategorie(wk);
    int zeilen = gangHier >= 0 ? 1 : 0;
    for (const PresetEintrag& e : b) { zeilen += e.kategorie == wk ? 1 : 0; }
    char info[96] = {};
    std::snprintf(info, sizeof info, "%s \xc2\xb7 %d", presetKategorieName(wk), zeilen);
    if (!ui::rolloutBeginn(tr(Str::PrRollout), {.kurzinfo = info, .hinweis = tr(Str::PrRolloutHint)})) { return; }
    r.presetRolloutBild = ImGui::GetFrameCount();
    r.bibliothekY = ImGui::GetCursorPosY();
    const float fs = ImGui::GetFontSize();
    const bool gewaehlt = r.bibliothekWahl >= 0 && static_cast<std::size_t>(r.bibliothekWahl) < b.size();
    const bool gang = !gewaehlt && r.gangWahl >= 0;
    // die Kategorien als eine Wahl in zwei Spalten; ein Wechsel nimmt die Wahl nicht weg (sie steht dann nur nicht in der Liste)
    std::vector<const char*> texte;
    for (const std::string& k : kat) { texte.push_back(presetKategorieName(k)); }
    (void)ui::segment("##aikategorie", texte.data(), static_cast<int>(texte.size()), &r.bibliothekKategorie, 2, -1, nullptr, 0.0F, true);
    if (zeilen == 0) {
        // leer (noch keine Presets mit freier Lizenz): ein kurzer Hinweis statt einer leeren Flaeche
        ui::tipp(tr(Str::EnPresetsLeer));
    } else {
        // die Eintraege der Kategorie: ein Klick waehlt, ein Doppelklick legt als Vorschau auf die Figur; rechts Quelle und Lizenz
        ImGui::BeginChild("##aibib", ImVec2{-1.0F, std::min(static_cast<float>(zeilen), 6.0F) * ImGui::GetTextLineHeightWithSpacing() + fs * 0.6F}, ImGuiChildFlags_Borders);
        // die gewaehlte Zeile: Schrift hell oder dunkel nach der Auswahlfarbe (wie ui::segment; das erste helle Thema waehlt
        // dunkelblau, die dunkle Schrift stand darauf kaum lesbar)
        const ImVec4 kopf = ImGui::GetStyleColorVec4(ImGuiCol_Header);
        const bool kopfHell = kopf.x * 0.299F + kopf.y * 0.587F + kopf.z * 0.114F > 0.6F;
        const ImVec4 wahlText = kopfHell ? ImVec4{0.0F, 0.0F, 0.0F, 1.0F} : ImVec4{1.0F, 1.0F, 1.0F, 1.0F};
        // Quelle rechtsbuendig in der Zeile, wenn sie neben den Namen passt
        const auto rechts = [&](const char* name, const std::string& ql, bool istWahl) {
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            const float qlB = ImGui::CalcTextSize(ql.c_str()).x;
            if (lo.x + ImGui::CalcTextSize(name).x + fs + qlB > hi.x) { return false; }
            const ImU32 farbe = istWahl ? ImGui::GetColorU32(ImVec4{wahlText.x, wahlText.y, wahlText.z, 0.7F}) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
            ImGui::GetWindowDrawList()->AddText(ImVec2{std::floor(hi.x - qlB), lo.y}, farbe, ql.c_str());
            return true;
        };
        if (gangHier >= 0) {
            // der Gang des eigenen Generators als Eintrag, oben (immer zu sehen; Entwurf: "Gehen - Saebel gezogen · Generator")
            const char* name = tr(gangHier == static_cast<int>(modell::GangArt::Laufen) ? Str::EnGangLaufen : Str::EnGangGehen);
            const bool istWahl = gang && r.gangWahl == gangHier;
            if (istWahl) { ImGui::PushStyleColor(ImGuiCol_Text, wahlText); }
            const bool geklickt = ImGui::Selectable(name, istWahl, ImGuiSelectableFlags_AllowDoubleClick);
            if (istWahl) { ImGui::PopStyleColor(); }
            if (geklickt) {
                modellPresetGangWaehlen(gangHier);
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { (void)gangVorschau(); }
            }
            ImGui::SetItemTooltip("%s", tr(Str::EnGangHint));
            (void)rechts(name, tr(Str::EnGangQuelle), istWahl);
            if (ImGui::BeginPopupContextItem("##engang")) {
                if (r.gangWahl != gangHier || gewaehlt) { modellPresetGangWaehlen(gangHier); }
                if (ImGui::MenuItem(tr(Str::AzPresetVorschau))) { (void)gangVorschau(); }
                if (ImGui::MenuItem(tr(Str::AzPresetAuflegen)) && gangVorschau()) { (void)modellRtBacken(); }
                if (ImGui::MenuItem(tr(Str::PrMessenAktion))) { (void)modellPresetMessen(); }
                if (ImGui::MenuItem(tr(Str::AzGenerator), nullptr, modellAnim().generatorOffen)) {
                    modellAnim().generatorOffen = !modellAnim().generatorOffen;
                    g_generatorZeigen = modellAnim().generatorOffen;
                }
                ImGui::EndPopup();
            }
        }
        bool wahlQuellePasst = false;
        for (std::size_t i = 0; i < b.size(); ++i) {
            if (b[i].kategorie != wk) { continue; }
            ImGui::PushID(static_cast<int>(i));
            const bool istWahl = r.bibliothekWahl == static_cast<int>(i);
            if (istWahl) { ImGui::PushStyleColor(ImGuiCol_Text, wahlText); }
            const bool geklickt = ImGui::Selectable(b[i].name.c_str(), istWahl, ImGuiSelectableFlags_AllowDoubleClick);
            if (istWahl) { ImGui::PopStyleColor(); }
            if (geklickt) {
                r.bibliothekWahl = static_cast<int>(i);
                r.gangWahl = -1;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { (void)presetVorschau(static_cast<int>(i)); }
            }
            ImGui::SetItemTooltip("%s", format(Str::AiQuelle, b[i].quelle.c_str(), b[i].lizenz.c_str()).c_str());
            const bool passt = rechts(b[i].name.c_str(), quelleKurz(b[i]), istWahl);
            wahlQuellePasst = wahlQuellePasst || (passt && istWahl);
            // Rechtsklick: die Befehle des Eintrags
            if (ImGui::BeginPopupContextItem("##aieintrag")) {
                r.bibliothekWahl = static_cast<int>(i);
                r.gangWahl = -1;
                if (ImGui::MenuItem(tr(Str::AzPresetVorschau))) { (void)presetVorschau(static_cast<int>(i)); }
                if (ImGui::MenuItem(tr(Str::AzPresetAuflegen)) && presetVorschau(static_cast<int>(i))) { (void)modellRtBacken(); }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (gewaehlt && r.bibliothekWahl < 0) { ++fix612Probe().presetWahlVeraltet; }   // Selbsttest (FIX612 P3-1)
        // Quelle und Lizenz des gewaehlten Presets darunter nur, wenn sie nicht in seine Zeile passten
        if (gewaehlt && b[static_cast<std::size_t>(r.bibliothekWahl)].kategorie == wk && !wahlQuellePasst) {
            ui::absatz(format(Str::AiQuelle, b[static_cast<std::size_t>(r.bibliothekWahl)].quelle.c_str(), b[static_cast<std::size_t>(r.bibliothekWahl)].lizenz.c_str()).c_str());
        }
    }
    // Ziel-Platz: vor dem Laden der Platz der Bibliothek bzw. des Gangs (aenderbar), danach der des Uebertragens
    const bool geladen = gewaehlt ? r.quelleFuer == b[static_cast<std::size_t>(r.bibliothekWahl)].datei && !d.vonPreset
                                  : gang && d.vonPreset && static_cast<int>(d.gang.art) == r.gangWahl && !r.welt.empty();
    const int platzFuer = gewaehlt ? r.bibliothekWahl : (gang ? 100000 + r.gangWahl : -1);
    const std::string eigenerPlatz = gewaehlt ? b[static_cast<std::size_t>(r.bibliothekWahl)].platz
                                              : (gang ? std::string(kategorieZeile(static_cast<modell::GangArt>(r.gangWahl))) : std::string());
    if (g_presetPlatzFuer != platzFuer) {
        g_presetPlatzFuer = platzFuer;
        std::snprintf(g_presetPlatz, sizeof g_presetPlatz, "%s", eigenerPlatz.c_str());
    }
    if (geladen && d.platz != g_presetPlatz && !ImGui::IsAnyItemActive()) { std::snprintf(g_presetPlatz, sizeof g_presetPlatz, "%s", d.platz.c_str()); }
    // Ziel-Platz wie im Entwurf als Wahl: der eigene Platz, die Plaetze der anderen Presets der Kategorie, der erste freie Kino-Platz
    // der Ziel-GLA (BOTH_CIN_n) und ein Name, den das Feld Platz von "Uebertragen" gesetzt hat
    std::vector<std::string> plaetze;
    const auto dazu = [&](const std::string& p) {
        if (!p.empty() && std::find(plaetze.begin(), plaetze.end(), p) == plaetze.end()) { plaetze.push_back(p); }
    };
    dazu(eigenerPlatz);
    for (const PresetEintrag& e : b) {
        if (e.kategorie == wk && plaetze.size() < 2U) { dazu(e.platz); }
    }
    for (const modell::Platz& p : mt().plaetze) {
        if (!p.belegt && p.name.rfind("BOTH_CIN_", 0) == 0) {
            dazu(p.name);
            break;
        }
    }
    if (g_presetPlatz[0] != '\0' && std::find(plaetze.begin(), plaetze.end(), std::string(g_presetPlatz)) == plaetze.end()) { plaetze.insert(plaetze.begin(), g_presetPlatz); }
    std::vector<const char*> platzTexte;
    std::vector<const char*> platzHinweise;
    int platzWahl = -1;
    for (std::size_t i = 0; i < plaetze.size(); ++i) {
        platzTexte.push_back(plaetze[i].c_str());
        platzHinweise.push_back(tr(Str::AzZielPlatzHint));
        if (plaetze[i] == g_presetPlatz) { platzWahl = static_cast<int>(i); }
    }
    const bool wahlDa = gewaehlt || gang;
    ImGui::BeginDisabled(!wahlDa);
    ui::absatz(tr(Str::AzZielPlatz), ui::Ton::Leise);
    const ImVec2 platzOben = ImGui::GetCursorScreenPos();
    if (!platzTexte.empty() && ui::segment("##aiplatz", platzTexte.data(), static_cast<int>(platzTexte.size()), &platzWahl, 0, -1, platzHinweise.data()) && platzWahl >= 0) {
        std::snprintf(g_presetPlatz, sizeof g_presetPlatz, "%s", plaetze[static_cast<std::size_t>(platzWahl)].c_str());
        if (geladen) {
            const std::string p = g_presetPlatz;
            (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.platz = p; });
        }
    }
    g_presetPlatzKasten = platzTexte.empty() ? ImRect{} : ImRect{platzOben, ImGui::GetItemRectMax()};
    ImGui::EndDisabled();
    // der erzeugte Gang: Tempo und Armschwung unter der Liste (Entwurf), mit Generator ... der Rest des Generators
    if (gang) {
        gangRegler(r, Str::PrTempo, Str::PrTempoHint, d.gang.tempo, 10.0F, 600.0F, "%.0f", [](modell::GangRegler& g, float v) { g.tempo = v; });
        gangRegler(r, Str::PrArm, Str::PrFaktorHint, d.gang.armschwung, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.armschwung = v; });
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::AzPresetVorschau), {.icon = UiIcon::Sichtbar, .hinweis = tr(gang ? Str::PrAuflegenHint : Str::AiAuflegenHint), .aktiv = wahlDa})) {
        (void)(gang ? gangVorschau() : presetVorschau(r.bibliothekWahl));
    }
    if (ui::werkzeugKnopf(tr(Str::AzGenerator), {.icon = UiIcon::Einstellungen, .hinweis = tr(Str::EnGeneratorHint), .an = modellAnim().generatorOffen && gang})) {
        if (modellAnim().generatorOffen && gang) {
            modellAnim().generatorOffen = false;
        } else {
            // ohne erzeugten Gang: den der Kategorie (Laufen) bzw. Gehen waehlen
            if (!gang) { modellPresetGangWaehlen(gangHier >= 0 ? gangHier : static_cast<int>(modell::GangArt::Gehen)); }
            modellAnim().generatorOffen = true;
            g_generatorZeigen = true;
        }
    }
    ui::rasterEnde();
    if (ui::werkzeugKnopf(tr(Str::AzPresetAuflegen), {.icon = UiIcon::Abspielen, .hinweis = tr(Str::AzPresetAuflegenHint), .haupt = true, .aktiv = wahlDa})) {
        // noch nicht geladen: erst laden bzw. erzeugen und uebertragen, dann auf den Platz
        if (geladen || (gang ? gangVorschau() : presetVorschau(r.bibliothekWahl))) { (void)modellRtBacken(); }
    }
    if (gewaehlt && geladen && !r.welt.empty() && r.weltNaht >= 0.0F) {
        ui::absatz(format(Str::AiNahtlos, static_cast<int>(r.welt.size()), r.quelleCfg.empty() ? 0 : r.quelleCfg.front().fps, static_cast<double>(r.weltNaht)).c_str());
    }
    // Generator ...: der Rest des Generators unter dem Hauptknopf (Knopf und Hauptknopf bleiben, wo sie sind); beim Aufklappen
    // ins Bild gerollt
    if (gang && modellAnim().generatorOffen) {
        const ImVec2 oben = ImGui::GetCursorScreenPos();
        gangMehr(r, d);
        if (g_generatorZeigen) {
            ImGui::ScrollToRect(ImGui::GetCurrentWindow(), ImRect{oben, ImVec2{oben.x + 1.0F, ImGui::GetCursorScreenPos().y}}, ImGuiScrollFlags_KeepVisibleEdgeY);
            g_generatorZeigen = false;
        }
    }
    ui::tipp(tr(Str::AzPresetTipp));
    ui::rolloutEnde();
}

}  // namespace bhed::gui
