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
    const std::string fuer = g_app->anim.skeletonName + "|" + std::to_string(g_app->anim.bones.size());
    if (r.czFuer == fuer) { return; }
    r.cz = modell::rtCharakter(g_app->anim, glaPfad());
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
        r.quelleFehler = fehler.empty() ? pfad : fehler;
        melden(format(Str::RtQuelleFehler, r.quelleFehler.c_str()));
        return false;
    }
    r.quelle = std::move(imp.anim);
    r.quelleFuer = pfad;
    r.quelleFehler.clear();
    r.quelleCfg = std::move(imp.sequenzen);
    r.importInfo = format(Str::AiImportInfo, imp.format.c_str(), imp.oben.c_str(), imp.vorn.c_str(), static_cast<double>(imp.zentimeter),
                          r.quelleCfg.empty() ? 0 : r.quelleCfg.front().fps);
    for (const std::string& h : imp.hinweise) { r.importInfo += "; " + h; }
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
    if (d.zu.quelle.size() != r.cz.ketten.size()) {
        // das Ziel hat andere Ketten (anderes Modell): leere Zuordnung neu, ohne eigenen Rueckgaengig-Schritt
        const modell::RtZuordnung zu = modell::rtZuordnen(r.cq, r.cz);
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.rt.zu = zu; }, false);
    }
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
        melden(format(Str::RtQuelleFehler, r.bericht.fehler.c_str()));
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
        melden(format(Str::RtQuelleFehler, fehler.c_str()));
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
    if (!ImGui::BeginPopupModal((std::string(tr(Str::RtFensterTitel)) + "###mtrtquelle").c_str(), nullptr)) { return; }
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

// --- Rollout -----------------------------------------------------------------------------------------------------------

void drawModellRetargetBereich() {
    ModellRetarget& r = rt();
    if (g_app->anim.bones.empty()) { return; }
    modellRtAktualisieren();
    drawModellRtQuelleFenster();
    if (r.oeffnen) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        r.oeffnen = false;
    }
    const bool offen = ImGui::CollapsingHeader(tr(Str::RtRollout));
    ImGui::SetItemTooltip("%s", tr(Str::RtRolloutHint));
    if (!offen) { return; }
    r.rolloutBild = ImGui::GetFrameCount();
    zielCharakter();
    const modell::RetargetDaten d = *modellRtDaten();
    const float fs = ImGui::GetFontSize();
    ImGui::PushTextWrapPos(0.0F);
    // Quelle und Ziel
    if (ImGui::Button(tr(Str::RtQuelleWaehlen))) { r.quelleWaehlen = true; }
    ImGui::SetItemTooltip("%s", tr(Str::RtQuelleWaehlenHint));
    daneben(tr(Str::AiImport));
    if (ImGui::Button(tr(Str::AiImport))) { (void)modellRtImportWaehlen(); }
    ImGui::SetItemTooltip("%s", tr(Str::AiImportHint));
    if (!r.importInfo.empty() && modell::istAnimImportEndung(r.quelleFuer)) { ImGui::TextDisabled("%s", r.importInfo.c_str()); }
    if (r.quelle.bones.empty()) {
        cjkTextDisabled("%s", tr(Str::RtQuelleKeine));
        ImGui::PopTextWrapPos();
        return;
    }
    std::string quellText = r.quelleFuer;
    if (std::filesystem::path(quellText).is_absolute()) {
        // nur die letzten vier Teile (models/players/<skelett>/<datei>), der ganze Pfad im Tooltip
        std::string kurz;
        int teile = 0;
        for (auto it = std::filesystem::path(quellText).end(); it != std::filesystem::path(quellText).begin() && teile < 4; ++teile) {
            --it;
            kurz = it->generic_string() + (kurz.empty() ? "" : "/" + kurz);
        }
        quellText = ".../" + kurz;
    }
    cjkTextUnformatted(format(Str::RtQuelle, quellText.c_str(), static_cast<int>(r.quelle.bones.size()), charakterText(r.cq).c_str()).c_str());
    ImGui::SetItemTooltip("%s", r.quelleFuer.c_str());
    cjkTextUnformatted(format(Str::RtZiel, glaPfad().c_str(), static_cast<int>(g_app->anim.bones.size()), charakterText(r.cz).c_str()).c_str());
    // Sequenz der Quelle
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(Str::RtSequenz)).x - ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::BeginCombo(tr(Str::RtSequenz), d.sequenz.empty() ? tr(Str::RtSequenzAlle) : d.sequenz.c_str(), ImGuiComboFlags_HeightLarge)) {
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
    bool vorschau = r.vorschau;
    if (ImGui::Checkbox(tr(Str::RtVorschau), &vorschau)) { (void)modellRtVorschau(vorschau); }
    ImGui::SetItemTooltip("%s", tr(Str::RtVorschauHint));
    daneben(tr(Str::RtQuelleZeigen));
    if (ImGui::Checkbox(tr(Str::RtQuelleZeigen), &r.quelleZeigen)) { g_app->modelDirty = true; }
    ImGui::SetItemTooltip("%s", tr(Str::RtQuelleZeigenHint));
    if (r.vorschau && !r.welt.empty()) {
        if (ImGui::SmallButton(r.spielt ? tr(Str::RtAnhalten) : tr(Str::RtAbspielen))) { r.spielt = !r.spielt; }
        ImGui::SetItemTooltip("%s", tr(Str::RtAbspielenHint));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::SliderFloat("##rtbild", &r.bild, 0.0F, static_cast<float>(r.welt.size() - 1), "%.0f")) { g_app->modelDirty = true; }
        ImGui::SetItemTooltip("%s", tr(Str::RtBildHint));
    }
    // Ketten-Zuordnung
    if (r.baeumeOeffnen) { ImGui::SetNextItemOpen(true, ImGuiCond_Always); }
    if (ImGui::TreeNodeEx(begriff(Str::RtKettenMax, Str::RtKettenBlender), ImGuiTreeNodeFlags_None)) {
        ImGui::SetItemTooltip("%s", tr(Str::RtKettenHint));
        if (ImGui::SmallButton(tr(Str::RtAutoLeer))) { (void)modellRtZuordnen(false); }
        ImGui::SetItemTooltip("%s", tr(Str::RtAutoLeerHint));
        daneben(tr(Str::RtAutoAlle));
        if (ImGui::SmallButton(tr(Str::RtAutoAlle))) { (void)modellRtZuordnen(true); }
        ImGui::SetItemTooltip("%s", tr(Str::RtAutoAlleHint));
        if (ImGui::BeginTable("##rtketten", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchSame)) {
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
        ImGui::TreePop();
    } else {
        ImGui::SetItemTooltip("%s", tr(Str::RtKettenHint));
    }
    // Retarget-Pose
    if (r.baeumeOeffnen) { ImGui::SetNextItemOpen(true, ImGuiCond_Always); }
    if (ImGui::TreeNodeEx(begriff(Str::RtPoseMax, Str::RtPoseBlender), ImGuiTreeNodeFlags_None)) {
        ImGui::SetItemTooltip("%s", tr(Str::RtPoseHint));
        const char* haltungen[3] = {tr(Str::RtHaltungBinde), tr(Str::RtHaltungT), tr(Str::RtHaltungA)};
        for (const bool ziel : {false, true}) {
            int h = static_cast<int>(ziel ? d.e.poseZiel.haltung : d.e.poseQuelle.haltung);
            ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(ziel ? Str::RtPoseZiel : Str::RtPoseQuelle)).x - ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::Combo(tr(ziel ? Str::RtPoseZiel : Str::RtPoseQuelle), &h, haltungen, 3)) { (void)modellRtHaltung(ziel, static_cast<modell::RtHaltung>(h)); }
            ImGui::SetItemTooltip("%s", tr(Str::RtHaltungHint));
        }
        const float ab = r.bericht.poseAbweichung;
        if (ab > 5.0F) {
            cjkTextColored(ImVec4{1.0F, 0.6F, 0.25F, 1.0F}, "%s", format(Str::RtPoseAbweichung, static_cast<double>(ab)).c_str());
        } else {
            cjkTextDisabled("%s", format(Str::RtPoseAbweichung, static_cast<double>(ab)).c_str());
        }
        // von Hand: der gewaehlte Knochen des Ziels um seine Achsen
        const int k = gewaehlterKnochen();
        if (k >= 0 && static_cast<std::size_t>(k) < g_app->anim.bones.size()) {
            cjkTextUnformatted(format(Str::RtPoseKnochen, g_app->anim.bones[static_cast<std::size_t>(k)].name.c_str()).c_str());
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
            cjkTextDisabled("%s", tr(Str::RtKnochenWahl));
        }
        if (ImGui::SmallButton(tr(Str::RtPoseZurueck))) {
            (void)modellRtEinstellen(tr(Str::RtUndoPose), [](modell::RetargetDaten& x) {
                x.e.poseZiel.vonHand.clear();
                x.e.poseQuelle.vonHand.clear();
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtPoseZurueckHint));
        // benannte Posen
        ImGui::SetNextItemWidth(fs * 9.0F);
        ImGui::InputTextWithHint("##rtpname", tr(Str::RtPoseName), r.poseName, sizeof r.poseName);
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::RtPoseSpeichern))) { (void)modellRtPoseSpeichern(r.poseName); }
        ImGui::SetItemTooltip("%s", tr(Str::RtPoseSpeichernHint));
        for (std::size_t i = 0; i < d.posen.size(); ++i) {
            if (!d.posen[i].ziel) { continue; }
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton(d.posen[i].name.c_str())) {
                const std::string n = d.posen[i].name;
                (void)modellRtEinstellen(tr(Str::RtUndoPose), [&](modell::RetargetDaten& x) {
                    for (const modell::RtBenanntePose& p : x.posen) {
                        if (p.name == n) { (p.ziel ? x.e.poseZiel : x.e.poseQuelle) = p.pose; }
                    }
                });
            }
            ImGui::SetItemTooltip("%s", tr(Str::RtPoseLadenHint));
            ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::NewLine();
        ImGui::TreePop();
    } else {
        ImGui::SetItemTooltip("%s", tr(Str::RtPoseHint));
    }
    // Schritte in ihrer Reihenfolge
    if (r.baeumeOeffnen) { ImGui::SetNextItemOpen(true, ImGuiCond_Always); }
    if (ImGui::TreeNodeEx(tr(Str::RtSchritte), ImGuiTreeNodeFlags_None)) {
        ImGui::SetItemTooltip("%s", tr(Str::RtSchritteHint));
        static const Str kName[4] = {Str::RtSchrittBecken, Str::RtSchrittFk, Str::RtSchrittIk, Str::RtSchrittWurzel};
        static const Str kHint[4] = {Str::RtSchrittBeckenHint, Str::RtSchrittFkHint, Str::RtSchrittIkHint, Str::RtSchrittWurzelHint};
        for (std::size_t i = 0; i < 4; ++i) {
            ImGui::PushID(static_cast<int>(i));
            const modell::RtSchritt s = d.e.reihenfolge[i];
            const int si = static_cast<int>(s);
            bool an = s == modell::RtSchritt::Becken ? d.e.becken : (s == modell::RtSchritt::Fk ? d.e.fk : (s == modell::RtSchritt::Ik ? d.e.ik : d.e.wurzel));
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
            if (ImGui::Checkbox(tr(kName[si]), &an)) {
                (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) {
                    (s == modell::RtSchritt::Becken ? x.e.becken : (s == modell::RtSchritt::Fk ? x.e.fk : (s == modell::RtSchritt::Ik ? x.e.ik : x.e.wurzel))) = an;
                });
            }
            ImGui::SetItemTooltip("%s", tr(kHint[si]));
            ImGui::PopID();
        }
        const auto regler = [&](Str text, Str hinweis, float wert, const std::function<void(modell::RtEinstellung&, float)>& setzen) {
            float v = wert;
            ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(text)).x - ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::SliderFloat(tr(text), &v, 0.0F, 1.0F, "%.2f")) {
                (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { setzen(x.e, v); }, true);
            }
            if (ImGui::IsItemDeactivated()) { r.reglerZug = false; }
            ImGui::SetItemTooltip("%s", tr(hinweis));
        };
        regler(Str::RtReachFuss, Str::RtReachHint, d.e.reachFuss, [](modell::RtEinstellung& x, float v) { x.reachFuss = v; });
        regler(Str::RtReachHand, Str::RtReachHint, d.e.reachHand, [](modell::RtEinstellung& x, float v) { x.reachHand = v; });
        bool kon = d.e.kontakte;
        if (ImGui::Checkbox(tr(Str::RtKontakte), &kon)) {
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.kontakte = kon; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtKontakteHint));
        bool vonHand = d.e.laengeVonHand > 0.0F;
        if (ImGui::Checkbox(tr(Str::RtLaengeVonHand), &vonHand)) {
            const float k = r.bericht.k;
            (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.laengeVonHand = vonHand ? std::max(k, 0.01F) : 0.0F; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtLaengeHint));
        if (vonHand) {
            float k = d.e.laengeVonHand;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(fs * 5.0F);
            if (ImGui::DragFloat("##rtk", &k, 0.005F, 0.05F, 20.0F, "%.3f")) {
                (void)modellRtEinstellen(tr(Str::RtUndoSchritte), [&](modell::RetargetDaten& x) { x.e.laengeVonHand = k; }, true);
            }
            if (ImGui::IsItemDeactivated()) { r.reglerZug = false; }
            ImGui::SetItemTooltip("%s", tr(Str::RtLaengeHint));
        } else {
            ImGui::SameLine();
            cjkTextDisabled("%s", format(Str::RtLaengeWert, static_cast<double>(r.bericht.k)).c_str());
        }
        ImGui::TreePop();
    } else {
        ImGui::SetItemTooltip("%s", tr(Str::RtSchritteHint));
    }
    // Pruefen
    ImGui::Separator();
    if (!r.bericht.fehler.empty()) {
        cjkTextColored(ImVec4{1.0F, 0.35F, 0.3F, 1.0F}, "%s", format(Str::RtQuelleFehler, r.bericht.fehler.c_str()).c_str());
    } else if (!r.welt.empty()) {
        cjkTextDisabled("%s", format(Str::RtErgebnis, r.bericht.knochenMitQuelle, static_cast<int>(g_app->anim.bones.size()), r.bericht.kettenZugeordnet,
                                         static_cast<double>(r.bericht.k), r.ms)
                                      .c_str());
        const bool rutscht = r.bericht.rutschenMax > std::max(0.5F, r.bericht.rutschenQuelle * 1.5F);
        const std::string rt_ = format(Str::RtRutschen, static_cast<double>(r.bericht.rutschenMax), static_cast<double>(r.bericht.rutschenQuelle));
        if (rutscht) {
            cjkTextColored(ImVec4{1.0F, 0.6F, 0.25F, 1.0F}, "%s", rt_.c_str());
        } else {
            cjkTextUnformatted(rt_.c_str());
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtRutschenHint));
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
            cjkTextDisabled("%s", tr(Str::RtKeineSpruenge));
        } else {
            cjkTextColored(ImVec4{1.0F, 0.35F, 0.3F, 1.0F}, "%s", format(Str::RtSpruenge, static_cast<int>(r.bericht.spruenge.size())).c_str());
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
    // Backen auf einen Platz
    ImGui::Separator();
    static char platz[48] = {};
    if (d.platz != platz) { std::snprintf(platz, sizeof platz, "%s", d.platz.c_str()); }
    ImGui::SetNextItemWidth(fs * 9.0F);
    if (ImGui::InputTextWithHint(tr(Str::RtPlatz), d.sequenz.empty() ? "retarget" : d.sequenz.c_str(), platz, sizeof platz, ImGuiInputTextFlags_EnterReturnsTrue) ||
        ImGui::IsItemDeactivatedAfterEdit()) {
        const std::string p = platz;
        (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& x) { x.platz = p; });
    }
    ImGui::SetItemTooltip("%s", tr(Str::RtPlatzHint));
    if (ImGui::Button(begriff(Str::RtBackenMax, Str::RtBackenBlender))) { (void)modellRtBacken(); }
    ImGui::SetItemTooltip("%s", tr(Str::RtBackenHint));
    ImGui::PopTextWrapPos();
    r.baeumeOeffnen = false;
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

const char* presetKategorieText(const std::string& k) {
    if (k == "idle") { return tr(Str::AiKatStehen); }
    if (k == "walk") { return tr(Str::PrGehen); }
    if (k == "run") { return tr(Str::PrLaufen); }
    if (k == "combat") { return tr(Str::AiKatKampf); }
    return tr(Str::AiKatSonst);
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
        melden(format(Str::RtQuelleFehler, r.bericht.fehler.c_str()));
        return false;
    }
    r.vorschau = true;
    r.spielt = true;
    r.presetOeffnen = true;
    g_app->modelDirty = true;
    return true;
}

void drawModellPresetBibliothekInhalt(ModellRetarget& r, const std::vector<PresetEintrag>& b);

// Gruppe Bibliothek: Kategorie -> Animation (Vorschau beim Auflegen) -> Platz -> Auf Figur legen -> Backen
void drawModellPresetBibliothek() {
    ModellRetarget& r = rt();
    const std::vector<PresetEintrag>& b = modellPresetBibliothek();
    // eigene Kennungen: Platz und Backen heissen wie im Rollout Retargeting (ImGui-Kennung sonst doppelt)
    ImGui::PushID("aibibliothek");
    drawModellPresetBibliothekInhalt(r, b);
    ImGui::PopID();
}

void drawModellPresetBibliothekInhalt(ModellRetarget& r, const std::vector<PresetEintrag>& b) {
    r.bibliothekY = ImGui::GetCursorPosY();
    ImGui::SeparatorText(tr(Str::AiBibliothek));
    ImGui::SetItemTooltip("%s", tr(Str::AiBibliothekHint));
    if (b.empty()) {
        ImGui::TextDisabled("%s", tr(Str::AiKeineBibliothek));
        return;
    }
    std::vector<std::string> kat;
    for (const PresetEintrag& e : b) {
        if (std::find(kat.begin(), kat.end(), e.kategorie) == kat.end()) { kat.push_back(e.kategorie); }
    }
    r.bibliothekKategorie = std::clamp(r.bibliothekKategorie, 0, static_cast<int>(kat.size()) - 1);
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(Str::AiKategorie)).x - ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::BeginCombo(tr(Str::AiKategorie), presetKategorieText(kat[static_cast<std::size_t>(r.bibliothekKategorie)]))) {
        for (std::size_t k = 0; k < kat.size(); ++k) {
            if (ImGui::Selectable(presetKategorieText(kat[k]), static_cast<int>(k) == r.bibliothekKategorie)) { r.bibliothekKategorie = static_cast<int>(k); }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::AiBibliothekHint));
    const std::string& wk = kat[static_cast<std::size_t>(r.bibliothekKategorie)];
    const float fs = ImGui::GetFontSize();
    int zeilen = 0;
    for (const PresetEintrag& e : b) { zeilen += e.kategorie == wk ? 1 : 0; }
    ImGui::BeginChild("##aibib", ImVec2{-1.0F, std::min(static_cast<float>(zeilen), 6.0F) * ImGui::GetTextLineHeightWithSpacing() + fs * 0.6F},
                      ImGuiChildFlags_Borders);
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b[i].kategorie != wk) { continue; }
        ImGui::PushID(static_cast<int>(i));
        // ein Klick waehlt, ein Doppelklick legt auf die Figur
        if (ImGui::Selectable(b[i].name.c_str(), r.bibliothekWahl == static_cast<int>(i), ImGuiSelectableFlags_AllowDoubleClick)) {
            r.bibliothekWahl = static_cast<int>(i);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { (void)modellPresetBibliothekAuflegen(static_cast<int>(i)); }
        }
        ImGui::SetItemTooltip("%s", format(Str::AiQuelle, b[i].quelle.c_str(), b[i].lizenz.c_str()).c_str());
        ImGui::PopID();
    }
    ImGui::EndChild();
    const bool gewaehlt = r.bibliothekWahl >= 0 && static_cast<std::size_t>(r.bibliothekWahl) < b.size();
    if (gewaehlt) {
        const PresetEintrag& e = b[static_cast<std::size_t>(r.bibliothekWahl)];
        ImGui::TextDisabled("%s", format(Str::AiQuelle, e.quelle.c_str(), e.lizenz.c_str()).c_str());
    }
    ImGui::BeginDisabled(!gewaehlt);
    if (ImGui::Button(tr(Str::AiAuflegen))) { (void)modellPresetBibliothekAuflegen(r.bibliothekWahl); }
    ImGui::SetItemTooltip("%s", tr(Str::AiAuflegenHint));
    ImGui::EndDisabled();
    const modell::RetargetDaten& d = *modellRtDaten();
    const bool vonBibliothek = gewaehlt && r.quelleFuer == b[static_cast<std::size_t>(r.bibliothekWahl)].datei && !d.vonPreset;
    if (vonBibliothek) {
        // Platz (wie im Rollout Retargeting) und Backen
        char platz[64];
        std::snprintf(platz, sizeof platz, "%s", d.platz.c_str());
        // daneben nur, wenn Feld und Beschriftung ganz passen (sonst stand "Slot" in langen Sprachen hinter dem Rand)
        ImGui::SameLine();
        if (ImGui::GetContentRegionAvail().x < fs * 8.0F + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(tr(Str::RtPlatz)).x) { ImGui::NewLine(); }
        ImGui::SetNextItemWidth(fs * 8.0F);
        if (ImGui::InputText(tr(Str::RtPlatz), platz, sizeof platz, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase)) {
            const std::string p = platz;
            (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { x.platz = p; });
        }
        ImGui::SetItemTooltip("%s", tr(Str::RtPlatzHint));
        daneben(tr(Str::RtBackenMax));
        if (ImGui::Button(tr(Str::RtBackenMax))) { (void)modellRtBacken(); }
        ImGui::SetItemTooltip("%s", tr(Str::RtBackenHint));
        if (!r.welt.empty() && r.weltNaht >= 0.0F) {
            ImGui::TextDisabled("%s", format(Str::AiNahtlos, static_cast<int>(r.welt.size()), r.quelleCfg.empty() ? 0 : r.quelleCfg.front().fps,
                                             static_cast<double>(r.weltNaht)).c_str());
        }
    }
}

void drawModellPresetBereich() {
    ModellRetarget& r = rt();
    if (g_app->anim.bones.empty()) { return; }
    if (r.presetOeffnen) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        r.presetOeffnen = false;
    }
    const bool offen = ImGui::CollapsingHeader(tr(Str::PrRollout));
    ImGui::SetItemTooltip("%s", tr(Str::PrRolloutHint));
    if (!offen) { return; }
    r.presetRolloutBild = ImGui::GetFrameCount();
    const modell::RetargetDaten d = *modellRtDaten();
    ImGui::PushTextWrapPos(0.0F);
    const auto setze = [&](const std::function<void(modell::GangRegler&)>& f) {
        (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { f(x.gang); });
    };
    // Kategorie und Haltung
    const char* kategorien[2] = {tr(Str::PrGehen), tr(Str::PrLaufen)};
    int k = static_cast<int>(d.gang.art);
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(Str::PrKategorie)).x - ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::Combo(tr(Str::PrKategorie), &k, kategorien, 2)) {
        setze([&](modell::GangRegler& g) {
            g.art = static_cast<modell::GangArt>(k);
            g.tempo = k == 1 ? 300.0F : 90.0F;   // runSpeed / walkSpeed der NPCs (NPC_stats.cpp:1996-1997)
        });
        (void)modellPresetMessen();
    }
    ImGui::SetItemTooltip("%s", tr(Str::PrKategorieHint));
    const char* haltungen[modell::kGangHaltungen] = {tr(Str::PrHaltNormal), tr(Str::PrHaltSchwer), tr(Str::PrHaltSchleichend), tr(Str::PrHaltVerletzt),
                                                     tr(Str::PrHaltSaebel)};
    int h = static_cast<int>(d.gang.haltung);
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(Str::PrHaltung)).x - ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::Combo(tr(Str::PrHaltung), &h, haltungen, modell::kGangHaltungen)) { setze([&](modell::GangRegler& g) { g.haltung = static_cast<modell::GangHaltung>(h); }); }
    ImGui::SetItemTooltip("%s", tr(Str::PrHaltungHint));
    // Regler (ein Rueckgaengig-Schritt je Zug)
    const auto regler = [&](Str text, Str hinweis, float wert, float lo, float hi, const char* fmt, const std::function<void(modell::GangRegler&, float)>& f) {
        float v = wert;
        ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(text)).x - ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::SliderFloat(tr(text), &v, lo, hi, fmt)) {
            if (r.presetZug) {
                mt().dok.vorschau([&](modell::Inhalt& in) { f(in.rig.rt.gang, v); }, false);
            } else {
                (void)modellRtEinstellen(tr(Str::PrUndo), [&](modell::RetargetDaten& x) { f(x.gang, v); });
                r.presetZug = true;
            }
            g_app->modelDirty = true;
        }
        if (ImGui::IsItemDeactivated()) { r.presetZug = false; }
        ImGui::SetItemTooltip("%s", tr(hinweis));
    };
    regler(Str::PrTempo, Str::PrTempoHint, d.gang.tempo, 10.0F, 600.0F, "%.0f", [](modell::GangRegler& g, float v) { g.tempo = v; });
    regler(Str::PrSchritt, Str::PrSchrittHint, d.gang.schritt, 2.0F, 120.0F, "%.1f", [](modell::GangRegler& g, float v) { g.schritt = v; });
    regler(Str::PrArm, Str::PrFaktorHint, d.gang.armschwung, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.armschwung = v; });
    regler(Str::PrHuefte, Str::PrFaktorHint, d.gang.hueftschwung, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.hueftschwung = v; });
    regler(Str::PrHub, Str::PrFaktorHint, d.gang.hub, 0.0F, 2.0F, "%.2f", [](modell::GangRegler& g, float v) { g.hub = v; });
    const std::string messen = format(Str::PrMessen, kategorieZeile(d.gang.art));
    if (ImGui::SmallButton(messen.c_str())) { (void)modellPresetMessen(); }
    ImGui::SetItemTooltip("%s", tr(Str::PrMessenHint));
    // Auf die Figur legen, Befund
    if (ImGui::Button(tr(Str::PrAuflegen))) { (void)modellPresetAuflegen(); }
    ImGui::SetItemTooltip("%s", tr(Str::PrAuflegenHint));
    if (d.vonPreset && !r.welt.empty()) {
        const modell::GangBefund& b = r.gang;
        cjkTextDisabled("%s", format(Str::PrBefund, static_cast<double>(modell::gangZyklus(d.gang)), static_cast<int>(r.welt.size()), presetFps(d.gang),
                                         static_cast<double>(b.bodenAbw), static_cast<double>(b.gleitAbw), static_cast<double>(b.naht))
                                      .c_str());
        cjkTextDisabled("%s", tr(Str::PrLizenz));
    }
    drawModellPresetBibliothek();
    ImGui::PopTextWrapPos();
}

}  // namespace bhed::gui
