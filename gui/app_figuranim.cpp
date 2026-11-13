// app_figuranim.cpp - Modell-Block Stufe 7: Figuren im Mission-Tab animieren und fuer das Spiel bauen
//
// Die Rechnung steht in bhed/modell/figuranim.h (Schicht im Skript, Grundanimation, Wurzelweg und SET_ORIGIN, ICARUS-Block,
// Plaetze, Backen). Hier die Bedienung in der Vorschau des Mission-Tabs, so wie im Model-Tab (Stufe 3/3b/4):
//
//   Fenster   "Figur animieren": Grundanimation, Laenge, Start, Folgeanimation, SET_ORIGIN, Signale; Rollouts Posieren,
//             IK/FK, Pfade; Dopesheet mit IK-Zeilen und Kurven; "Fuer das Spiel bauen" mit dem Ziel der GLA
//   Ansicht   die Griffe des Control Rigs auf der Figur in der Karte (wie 3ds Max: Q waehlen, W verschieben, E drehen;
//             wie Blender: G/R modal mit X/Y/Z), IK-Ziele und Pol-Ziele, der Wurzelweg und die Pfade der Glieder zum Ziehen,
//             Klick auf eine Flaeche setzt das IK-Ziel (Fuesse auf den Boden, Haende an Tueren und Konsolen)
//   Vorschau  jede Aenderung schreibt den Block der Figur neu (ein Rueckgaengig-Schritt); beim Neuaufbau der Szene wird die
//             Karten-GLA aus allen Bloecken gebacken und wie im Spiel an _humanoid gehaengt (kinoSkeletteAbgleichen) -
//             die Zeitleiste spielt, was das Spiel spielt, auch den Sprung ohne SET_ORIGIN
//   Erreichbar Inspektor der Figur, Rechtsklick (Entity und Ansicht), Aktionssuche (X wie Max, F3 wie Blender), Menue im
//             Fenster, Tooltip mit Taste an jeder Zeile
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/ibi.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/figuranim.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/physik.h"
#include "bhed/modell/rig.h"
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
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

FigurAnimZustand& fa() { return g_app->figAnim; }

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    fa().meldung = m;
    addStatus(m);
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(c == '\\' ? '/' : std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

modell::Zielspiel zielspiel() {
    if (regelwerkJediOutcast()) { return modell::Zielspiel::Jk2Sp; }
    return regelwerkMovieDuels() ? modell::Zielspiel::MdSp : modell::Zielspiel::JkaSp;
}

// level.mapname: "maps/academy1.bsp" -> "academy1" (wie kinoSkeletteAbgleichen)
std::string karte() {
    std::string k = klein(g_app->map.path);
    if (const auto s = k.find_last_of('/'); s != std::string::npos) { k = k.substr(s + 1); }
    if (k.size() > 4 && k.compare(k.size() - 4, 4, ".bsp") == 0) { k.resize(k.size() - 4); }
    return k;
}

int actorIndex(const std::string& name) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        if (g_app->scene.actors[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

const App::ActorAssets* assetsVon(int ai) {
    if (ai < 0 || static_cast<std::size_t>(ai) >= g_app->actorAssets.size() || !g_app->actorAssets[static_cast<std::size_t>(ai)]) { return nullptr; }
    return g_app->actorAssets[static_cast<std::size_t>(ai)].get();
}

// Die eigenen Zeilen der animation.cfg des Skeletts (ohne den Anhang der Karten-GLA - die Grundanimation kommt aus dem
// Skelett selbst, nie aus einem Platz, den diese Figuren belegen).
const std::vector<AnimEntry>& eigeneAbschnitte(const App::Skeleton& sk) { return sk.grundGemerkt ? sk.grundListe : sk.sections; }

// --- Die Daten einer Figur (aus dem Skript, je Stand zwischengespeichert) -------------------------------------------------

struct FigurDaten {
    bool da = false;
    std::string fehler;
    int actor = -1;
    modell::FigurAnim anim;
    const App::Skeleton* skelett = nullptr;
    GlaAnimation basis;
    modell::Rig rig;
    float fw[16]{};
    float start[3]{};
    double startMs = 0.0;
    int becken = -1;
    int folgeBild = -1;
    modell::RigSchicht wirk;   // mit fremden Raeumen eingerechnet (und der Physik-Spur, Stufe 6)
    std::vector<std::array<float, 3>> wurzel;
    modell::PhysikBericht physik;   // Stufe 6
};

struct RigSpeicher {
    std::string schluessel;
    modell::Rig rig;
};

std::map<std::string, std::shared_ptr<FigurDaten>> g_daten;
std::map<std::string, std::string> g_datenSchluessel;
std::map<std::string, RigSpeicher> g_rigs;

// Das Rig eines Skeletts mit Modell (Tags, Fussrolle) - einmal je Skelett und Modell
const modell::Rig& rigFuer(const GlaAnimation& g, const GlmModel& m, const std::string& schluessel) {
    RigSpeicher& r = g_rigs[schluessel];
    if (r.schluessel != schluessel || r.rig.leer()) {
        modell::Inhalt in;
        modell::ausGlmModell(m, in);
        r.rig = modell::rigAnlegen(g, in, 0, {});
        r.schluessel = schluessel;
    }
    return r.rig;
}

// Wann laeuft das SET_ANIM_BOTH dieser Figur in der Szene? (Der Ablauf kennt waitsignal und signal.)
double animStartInSzene(const Actor& a, const std::string& platz, double vorgabe) {
    for (const ActorStep& s : a.steps) {
        if (s.kind == ActorStep::Kind::Anim && s.part == AnimPart::Both && klein(s.text) == klein(platz)) { return s.startMs; }
    }
    return vorgabe;
}

bool figurLage(const Actor& a, double ms, float fw[16], float start[3]) {
    const ActorState st = a.at(ms);
    ActorDraw d;
    for (int q = 0; q < 3; ++q) {
        d.pos[q] = st.pos[q];
        start[q] = st.pos[q];
    }
    d.yaw = st.angles[1];
    d.yawOffset = static_cast<float>(g_app->actorYaw);
    npcSkalaFuer(a.npcType, d.skala);
    figurWeltMatrix(d, fw);
    return a.haveStart;
}

std::string zeilenText(const std::vector<std::string>& z) {
    std::string t;
    for (const std::string& x : z) { t += x + "\n"; }
    return t;
}

// Die Daten ohne fremde Raeume (die brauchen die anderen Figuren)
std::shared_ptr<FigurDaten> datenRoh(const std::string& figur) {
    const std::vector<std::string> zeilen = modell::figurZeilenImSkript(g_app->doc.script(), figur);
    const std::string schluessel = zeilenText(zeilen) + "|" + std::to_string(g_app->szeneBauten) + "|" + g_app->map.path + "|" +
                                   std::to_string(g_app->actorAssets.size());
    auto& alt = g_daten[figur];
    if (alt && g_datenSchluessel[figur] == schluessel) { return alt; }
    auto d = std::make_shared<FigurDaten>();
    g_datenSchluessel[figur] = schluessel;
    alt = d;
    if (zeilen.empty()) {
        d->fehler = "kein Block";
        return d;
    }
    d->actor = actorIndex(figur);
    const App::ActorAssets* as = assetsVon(d->actor);
    if (as == nullptr || as->skeleton == nullptr || as->skeleton->anim.empty()) {
        d->fehler = tr(Str::FaKeinSkelett);
        // die Einstellungen trotzdem (ohne Schicht), damit das Fenster sie zeigt
        GlaAnimation leer;
        (void)modell::figurAusZeilen(zeilen, leer, modell::Rig{}, d->anim);
        return d;
    }
    d->skelett = as->skeleton;
    modell::FigurAnim vorab;
    {
        GlaAnimation leer;
        (void)modell::figurAusZeilen({zeilen.front()}, leer, modell::Rig{}, vorab);
    }
    std::string f;
    if (!modell::basisGla(as->skeleton->anim, eigeneAbschnitte(*as->skeleton), vorab.basis, vorab.bilder, vorab.fps, d->basis, &f)) {
        d->fehler = f;
        d->anim = vorab;
        return d;
    }
    d->rig = rigFuer(d->basis, as->model, as->model.animFile + "|" + std::to_string(d->basis.bones.size()) + "|" + as->model.name);
    if (!modell::figurAusZeilen(zeilen, d->basis, d->rig, d->anim, &f)) {
        d->fehler = f;
        return d;
    }
    if (d->anim.schicht.knochen.size() != d->basis.bones.size()) { d->anim.schicht.knochen.resize(d->basis.bones.size()); }
    const Actor& a = g_app->scene.actors[static_cast<std::size_t>(d->actor)];
    d->startMs = animStartInSzene(a, d->anim.platz, d->anim.startMs);
    (void)figurLage(a, d->startMs, d->fw, d->start);
    for (std::size_t g = 0; g < d->rig.griffe.size(); ++g) {
        if (d->rig.griffe[g].art == modell::GriffArt::Becken) { d->becken = d->rig.griffe[g].knochen; }
    }
    if (const AnimEntry* fe = [&]() -> const AnimEntry* {
            for (const AnimEntry& e : eigeneAbschnitte(*as->skeleton)) {
                if (e.name == d->anim.folge) { return &e; }
            }
            return nullptr;
        }()) {
        d->folgeBild = fe->firstFrame;
    }
    d->da = true;
    return d;
}

void physikEinrechnen(const FigurDaten& d, modell::RigSchicht& s, modell::PhysikBericht* bericht);   // Stufe 6, unten

// Mit fremden Raeumen und Wurzelweg
std::shared_ptr<FigurDaten> daten(const std::string& figur) {
    std::shared_ptr<FigurDaten> d = datenRoh(figur);
    if (!d->da || !d->wurzel.empty() || d->basis.empty()) { return d; }
    d->wirk = d->anim.schicht;
    if (!d->anim.fremd.empty()) {
        std::vector<std::shared_ptr<FigurDaten>> halten;
        std::vector<modell::FigurLage> andere;
        for (const modell::FremdRaum& fr : d->anim.fremd) {
            const std::shared_ptr<FigurDaten> o = datenRoh(fr.figur);
            if (!o->da) { continue; }
            halten.push_back(o);
            modell::FigurLage l{&o->anim, &o->basis, {}, o->startMs};
            std::memcpy(l.fw, o->fw, sizeof l.fw);
            andere.push_back(l);
        }
        modell::FigurLage selbst{&d->anim, &d->basis, {}, d->startMs};
        std::memcpy(selbst.fw, d->fw, sizeof selbst.fw);
        (void)modell::fremdeEinrechnen(d->basis, selbst, andere, d->wirk);
    }
    physikEinrechnen(*d, d->wirk, &d->physik);   // Stufe 6
    d->wurzel = modell::wurzelPfad(d->basis, d->wirk, d->becken, d->fw);
    return d;
}

// Die Weltlage zu einer Zeit: Bild und Anteil
bool bildBei(const FigurDaten& d, double ms, int& f0, int& f1, float& anteil) {
    const double lerp = modell::figurBildMs(d.anim);
    const double x = std::clamp((ms - d.startMs) / lerp, 0.0, static_cast<double>(d.basis.numFrames - 1));
    f0 = static_cast<int>(std::floor(x));
    f1 = std::min(f0 + 1, d.basis.numFrames - 1);
    anteil = static_cast<float>(x - f0);
    return ms >= d.startMs - 0.5 && ms <= d.startMs + static_cast<double>(d.basis.numFrames) * lerp;
}

int bildJetzt(const FigurDaten& d) {
    const double lerp = modell::figurBildMs(d.anim);
    return static_cast<int>(std::clamp(std::lround((g_app->playMs - d.startMs) / lerp), 0L, static_cast<long>(std::max(d.basis.numFrames - 1, 0))));
}

void zeitAufBild(const FigurDaten& d, int bild) {
    g_app->playMs = d.startMs + static_cast<double>(bild) * modell::figurBildMs(d.anim);
    g_app->playing = false;
    g_app->mapDirty = true;
}

// Punkt im Raum der Figur -> Karte (fw)
void inKarte(const float fw[16], const float p[3], float o[3]) {
    for (int r = 0; r < 3; ++r) { o[r] = fw[r * 4] * p[0] + fw[r * 4 + 1] * p[1] + fw[r * 4 + 2] * p[2] + fw[r * 4 + 3]; }
}
BoneMatrix fwMatrix(const float fw[16]) {
    BoneMatrix m;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) { m.m[r][c] = fw[r * 4 + c]; }
    }
    return m;
}
// Richtung der Karte -> Raum der Figur (ohne Ort, mit Groesse)
void richtungInFigur(const float fw[16], const float v[3], float o[3]) {
    const BoneMatrix inv = modell::starrInvers(fwMatrix(fw));
    for (int r = 0; r < 3; ++r) { o[r] = inv.m[r][0] * v[0] + inv.m[r][1] * v[1] + inv.m[r][2] * v[2]; }
}

// Stufe 6: die Physik-Spur der Figur. Boden: traceRay der Karte senkrecht unter dem Punkt (der Raum der Figur ist gedreht
// und skaliert, die Senkrechte bleibt), Schwerkraft: g_gravity der Karte (Scene::schwerkraft, worldspawn "gravity") im Raum
// der Figur (geteilt durch ihre Groesse aus der .npc), Takt: frameLerp der Figur.
void physikEinrechnen(const FigurDaten& d, modell::RigSchicht& s, modell::PhysikBericht* bericht) {
    if (!s.physik.e.an || d.basis.empty()) {
        s.physik.knochen.clear();
        return;
    }
    const BoneMatrix fwm = fwMatrix(d.fw);
    const BoneMatrix inv = modell::starrInvers(fwm);
    const float skala = std::hypot(fwm.m[0][0], fwm.m[1][0], fwm.m[2][0]);
    modell::PhysikUmgebung u;
    const float sohle[3] = {0.0F, 0.0F, d.start[2] - 24.0F};   // ohne Treffer: die Sohle (DEFAULT_MINS_2, bg_public.h)
    float sohleF[3];
    for (int r = 0; r < 3; ++r) { sohleF[r] = inv.m[r][0] * d.start[0] + inv.m[r][1] * d.start[1] + inv.m[r][2] * sohle[2] + inv.m[r][3]; }
    u.bodenZ = sohleF[2];
    u.bildMs = modell::figurBildMs(d.anim);
    u.boden = [&](float x, float y, float zOben, float& z) {
        const float pf[3] = {x, y, zOben};
        float pk[3];
        inKarte(d.fw, pf, pk);
        const float von[3] = {pk[0], pk[1], pk[2]};
        const float bis[3] = {pk[0], pk[1], pk[2] - 1024.0F};
        if (g_app->geo.empty()) { return false; }
        const TraceTreffer t = traceRay(g_app->geo, von, bis);
        if (!t.hit) { return false; }
        z = inv.m[2][0] * t.point[0] + inv.m[2][1] * t.point[1] + inv.m[2][2] * t.point[2] + inv.m[2][3];
        return true;
    };
    s.physik.e.schwerkraft = g_app->scene.schwerkraft / std::max(skala, 1e-3F);
    (void)modell::physikRechnen(d.basis, d.rig, s, 0, std::min(d.anim.bilder, d.basis.bildZahl()), u, bericht);
}

// --- Schreiben ins Skript ------------------------------------------------------------------------------------------------

// SET_ORIGIN fuer eine Figur aus den aktuellen Daten
std::array<float, 3> ursprungFuer(const modell::FigurAnim& a, const FigurDaten& d) {
    if (d.basis.empty() || d.skelett == nullptr) { return {d.start[0], d.start[1], d.start[2]}; }
    modell::RigSchicht wirk = a.schicht;
    if (!a.fremd.empty()) {
        std::vector<std::shared_ptr<FigurDaten>> halten;
        std::vector<modell::FigurLage> andere;
        for (const modell::FremdRaum& fr : a.fremd) {
            const std::shared_ptr<FigurDaten> o = datenRoh(fr.figur);
            if (!o->da) { continue; }
            halten.push_back(o);
            modell::FigurLage l{&o->anim, &o->basis, {}, o->startMs};
            std::memcpy(l.fw, o->fw, sizeof l.fw);
            andere.push_back(l);
        }
        modell::FigurLage selbst{&a, &d.basis, {}, d.startMs};
        std::memcpy(selbst.fw, d.fw, sizeof selbst.fw);
        (void)modell::fremdeEinrechnen(d.basis, selbst, andere, wirk);
    }
    physikEinrechnen(d, wirk, nullptr);   // Stufe 6
    int folge = -1;
    for (const AnimEntry& e : eigeneAbschnitte(*d.skelett)) {
        if (e.name == a.folge) { folge = e.firstFrame; }
    }
    const std::array<float, 3> v = modell::wurzelVersatz(d.basis, wirk, d.becken, d.skelett->anim, folge);
    return modell::setOriginOrt(d.start, d.fw, v);
}

bool schreiben(const modell::FigurAnim& a, const FigurDaten* d, Str was) {
    const std::array<float, 3> u = d != nullptr ? ursprungFuer(a, *d) : std::array<float, 3>{0.0F, 0.0F, 0.0F};
    const Script neu = modell::mitFigurBlock(g_app->doc.script(), a.figur, &a, u);
    if (!skriptBlockErsetzen(neu, tr(was))) { return false; }
    fa().geschriebenTiefe = g_app->doc.undoDepth();
    fa().geschriebenZeilen = zeilenText(modell::figurZeilenImSkript(neu, a.figur));
    ++fa().aenderungen;
    return true;
}

// Die Schicht der aktuellen Figur aendern (ein Rueckgaengig-Schritt)
bool schichtAendern(Str was, const std::function<void(modell::FigurAnim&, const FigurDaten&)>& f) {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    if (!d->da) {
        melden(d->fehler.empty() ? tr(Str::FaKeineFigur) : d->fehler);
        return false;
    }
    modell::FigurAnim a = d->anim;
    f(a, *d);
    return schreiben(a, d.get(), was);
}

// --- Griffe ---------------------------------------------------------------------------------------------------------------

// Ein Griff in der Ansicht: FK-Griff des Rigs, IK-Ziel oder Pol-Ziel eines Glieds
struct FaGriff {
    int art = 0;      // 0 FK (rig.griffe[nr]), 1 IK-Ziel, 2 Pol
    int nr = -1;      // Griff bzw. modell::IkArt
    float ort[3]{};   // in der Karte
    modell::Seite seite = modell::Seite::Mitte;
};

modell::IkArt ikArtFuerGriff(const modell::Rig& r, int g) {
    if (g < 0 || static_cast<std::size_t>(g) >= r.griffe.size()) { return modell::IkArt::ArmRechts; }
    const modell::Griff& x = r.griffe[static_cast<std::size_t>(g)];
    const bool links = x.seite == modell::Seite::Links;
    switch (x.art) {
        case modell::GriffArt::Oberarm:
        case modell::GriffArt::Unterarm:
        case modell::GriffArt::Hand: return links ? modell::IkArt::ArmLinks : modell::IkArt::ArmRechts;
        default: return links ? modell::IkArt::BeinLinks : modell::IkArt::BeinRechts;
    }
}
bool gliedGriff(const modell::Rig& r, int g) {
    if (g < 0 || static_cast<std::size_t>(g) >= r.griffe.size()) { return false; }
    switch (r.griffe[static_cast<std::size_t>(g)].art) {
        case modell::GriffArt::Oberarm:
        case modell::GriffArt::Unterarm:
        case modell::GriffArt::Hand:
        case modell::GriffArt::Oberschenkel:
        case modell::GriffArt::Unterschenkel:
        case modell::GriffArt::Fuss: return true;
        default: return false;
    }
}
bool endGriff(const modell::Rig& r, int g) {
    return g >= 0 && static_cast<std::size_t>(g) < r.griffe.size() &&
           (r.griffe[static_cast<std::size_t>(g)].art == modell::GriffArt::Hand || r.griffe[static_cast<std::size_t>(g)].art == modell::GriffArt::Fuss);
}

std::vector<FaGriff> griffeBei(const FigurDaten& d, const modell::RigSchicht& s, int bild, std::vector<BoneMatrix>* weltAus = nullptr) {
    std::vector<FaGriff> v;
    std::vector<BoneMatrix> w;
    modell::rigWelt(d.basis, s, bild, bild, 0.0F, w);
    for (std::size_t g = 0; g < d.rig.griffe.size(); ++g) {
        const modell::Griff& x = d.rig.griffe[g];
        if (x.ik || x.knochen < 0 || static_cast<std::size_t>(x.knochen) >= w.size() || x.art == modell::GriffArt::Saebelgriff) { continue; }
        FaGriff f;
        f.art = 0;
        f.nr = static_cast<int>(g);
        f.seite = x.seite;
        const float p[3] = {w[static_cast<std::size_t>(x.knochen)].m[0][3], w[static_cast<std::size_t>(x.knochen)].m[1][3], w[static_cast<std::size_t>(x.knochen)].m[2][3]};
        inKarte(d.fw, p, f.ort);
        v.push_back(f);
    }
    for (const modell::IkSpur& sp : s.ik) {
        const modell::IkZiel z = modell::ikZielAm(sp, static_cast<float>(bild), w, modell::plattformAm(s, static_cast<float>(bild)));
        if (!z.da || z.ik <= 0.0F) { continue; }
        FaGriff f;
        f.art = 1;
        f.nr = static_cast<int>(sp.art);
        f.seite = (sp.art == modell::IkArt::ArmLinks || sp.art == modell::IkArt::BeinLinks) ? modell::Seite::Links : modell::Seite::Rechts;
        const float p[3] = {z.ziel.m[0][3], z.ziel.m[1][3], z.ziel.m[2][3]};
        inKarte(d.fw, p, f.ort);
        v.push_back(f);
        FaGriff pol = f;
        pol.art = 2;
        inKarte(d.fw, z.pol, pol.ort);
        v.push_back(pol);
    }
    if (weltAus != nullptr) { *weltAus = std::move(w); }
    return v;
}

ImU32 griffFarbe(modell::Seite s, bool gewaehlt) {
    if (gewaehlt) { return IM_COL32(255, 255, 255, 255); }
    // wie der Model-Tab: links blau, rechts gruen, Mitte gelb
    switch (s) {
        case modell::Seite::Links: return IM_COL32(80, 150, 255, 235);
        case modell::Seite::Rechts: return IM_COL32(90, 220, 110, 235);
        default: return IM_COL32(250, 210, 70, 235);
    }
}

std::string griffName(const FigurDaten& d, const FaGriff& g) {
    if (g.art == 0 && g.nr >= 0 && static_cast<std::size_t>(g.nr) < d.rig.griffe.size()) {
        const int k = d.rig.griffe[static_cast<std::size_t>(g.nr)].knochen;
        return k >= 0 && static_cast<std::size_t>(k) < d.basis.bones.size() ? d.basis.bones[static_cast<std::size_t>(k)].name : std::string("?");
    }
    static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
    return std::string(tr(kGlied[std::clamp(g.nr, 0, 3)])) + (g.art == 1 ? " IK" : " Pol");
}

int gewaehlterGriff(const std::vector<FaGriff>& v) {
    const int w = fa().griffWahl;
    return w >= 0 && static_cast<std::size_t>(w) < v.size() ? w : -1;
}

// Die IK-Spur eines Glieds (legt sie aus dem Rig an); -1: das Rig hat das Glied nicht
int spurFuer(modell::RigSchicht& s, const FigurDaten& d, modell::IkArt art) {
    int i = modell::ikSpurIndex(s, art);
    if (i >= 0) { return i; }
    modell::IkSpur sp;
    if (!modell::ikSpurAnlegen(d.basis, d.rig, art, sp)) { return -1; }
    s.ik.push_back(sp);
    return static_cast<int>(s.ik.size()) - 1;
}

// Das Glied des gewaehlten Griffs (IK-Ziel, Pol oder FK-Griff an einem Glied); false: keins
bool gliedDerWahl(const FigurDaten& d, modell::IkArt& art) {
    const std::vector<FaGriff> v = griffeBei(d, d.wirk, bildJetzt(d));
    const int w = gewaehlterGriff(v);
    if (w < 0) { return false; }
    const FaGriff& g = v[static_cast<std::size_t>(w)];
    if (g.art != 0) {
        art = static_cast<modell::IkArt>(g.nr);
        return true;
    }
    if (!gliedGriff(d.rig, g.nr)) { return false; }
    art = ikArtFuerGriff(d.rig, g.nr);
    return true;
}

// --- Ziehen und Drehen ---------------------------------------------------------------------------------------------------

// Die Schicht des Zugs aus der Startschicht und der Mausbewegung (dx, dy in Bildpunkten)
void zugAnwenden(const FigurDaten& d, float dx, float dy) {
    FigurAnimZustand& z = fa();
    modell::RigSchicht s = z.schichtStart;
    std::vector<BoneMatrix> welt;
    const std::vector<FaGriff> v = griffeBei(d, s, z.zugBild, &welt);
    const int w = gewaehlterGriff(v);
    const Camera& c = g_app->gezeigteKamera;
    float rechts[3];
    float oben[3];
    float vorn[3];
    c.right(rechts);
    c.up(oben);
    c.forward(vorn);
    if (z.zugArt == 3 && d.becken >= 0) {
        // Pfadpunkt des Wurzelwegs: das Becken an diesem Bild in der Bildebene
        float wegK[3];
        for (int q = 0; q < 3; ++q) { wegK[q] = (rechts[q] * dx - oben[q] * dy) * z.zugSkala; }
        if (z.achse >= 0) {
            for (int q = 0; q < 3; ++q) { wegK[q] = q == z.achse ? (wegK[q] != 0.0F ? wegK[q] : -dy * z.zugSkala) : 0.0F; }
        }
        float weg[3];
        richtungInFigur(d.fw, wegK, weg);
        modell::rigKnochenVerschieben(d.basis, s, d.becken, z.zugBild, welt, weg);
        z.schichtZug = s;
        return;
    }
    if (w < 0) { return; }
    const FaGriff& g = v[static_cast<std::size_t>(w)];
    if (z.zugArt == 1) {
        float wegK[3];
        if (ImGui::GetIO().KeyShift) {
            wegK[0] = wegK[1] = 0.0F;
            wegK[2] = -dy * z.zugSkala;
        } else {
            for (int q = 0; q < 3; ++q) { wegK[q] = (rechts[q] * dx - oben[q] * dy) * z.zugSkala; }
        }
        if (z.achse >= 0) {
            const float l = wegK[z.achse];
            wegK[0] = wegK[1] = wegK[2] = 0.0F;
            wegK[z.achse] = l != 0.0F ? l : -dy * z.zugSkala;
        }
        float weg[3];
        richtungInFigur(d.fw, wegK, weg);
        if (g.art == 1 || (g.art == 0 && endGriff(d.rig, g.nr))) {
            // Hand/Fuss greifen = IK (wie 3ds Max), das Ziel in der Karte verschoben
            const modell::IkArt art = g.art == 1 ? static_cast<modell::IkArt>(g.nr) : ikArtFuerGriff(d.rig, g.nr);
            const int sp = spurFuer(s, d, art);
            if (sp < 0) { return; }
            const modell::IkZiel iz = modell::ikZielAm(s.ik[static_cast<std::size_t>(sp)], static_cast<float>(z.zugBild), welt, modell::plattformAm(s, static_cast<float>(z.zugBild)));
            BoneMatrix ziel = iz.da && iz.ik > 0.0F ? iz.ziel : modell::starr(welt[static_cast<std::size_t>(s.ik[static_cast<std::size_t>(sp)].ende.knochen)]);
            for (int q = 0; q < 3; ++q) { ziel.m[q][3] += weg[q]; }
            modell::ikZielSetzen(d.basis, s, sp, z.zugBild, ziel);
        } else if (g.art == 2) {
            const int sp = spurFuer(s, d, static_cast<modell::IkArt>(g.nr));
            if (sp < 0) { return; }
            const modell::IkZiel iz = modell::ikZielAm(s.ik[static_cast<std::size_t>(sp)], static_cast<float>(z.zugBild), welt, modell::plattformAm(s, static_cast<float>(z.zugBild)));
            const float pol[3] = {iz.pol[0] + weg[0], iz.pol[1] + weg[1], iz.pol[2] + weg[2]};
            modell::ikPolSetzen(d.basis, s, sp, z.zugBild, pol);
        } else if (d.rig.griffe[static_cast<std::size_t>(g.nr)].art == modell::GriffArt::Becken) {
            const BoneMatrix keine = BoneMatrix::identity();
            modell::rigGriffDrehen(d.basis, d.rig, g.nr, z.zugBild, welt, keine, weg, s);
        } else {
            return;   // FK-Griffe drehen nur (E)
        }
        z.schichtZug = s;
        return;
    }
    // Drehen: um die Blickachse (frei) oder die Achse der Karte, ein halbes Grad je Bildpunkt (wie der Ring-Gizmo)
    float achseK[3] = {-vorn[0], -vorn[1], -vorn[2]};
    if (z.achse >= 0) { achseK[0] = achseK[1] = achseK[2] = 0.0F, achseK[z.achse] = 1.0F; }
    float achse[3];
    richtungInFigur(d.fw, achseK, achse);
    const float grad = (dx - dy) * 0.5F;
    const BoneMatrix dreh = modell::drehungUm(achse, grad);
    if (g.art == 1 || (g.art == 0 && endGriff(d.rig, g.nr) && modell::ikSpurIndex(s, ikArtFuerGriff(d.rig, g.nr)) >= 0 &&
                       modell::ikAnteilAm(s.ik[static_cast<std::size_t>(modell::ikSpurIndex(s, ikArtFuerGriff(d.rig, g.nr)))], static_cast<float>(z.zugBild)) > 0.0F)) {
        // IK-Ziel drehen (die Hand nimmt die Drehung des Ziels)
        const modell::IkArt art = g.art == 1 ? static_cast<modell::IkArt>(g.nr) : ikArtFuerGriff(d.rig, g.nr);
        const int sp = spurFuer(s, d, art);
        if (sp < 0) { return; }
        const modell::IkZiel iz = modell::ikZielAm(s.ik[static_cast<std::size_t>(sp)], static_cast<float>(z.zugBild), welt, modell::plattformAm(s, static_cast<float>(z.zugBild)));
        if (!iz.da) { return; }
        BoneMatrix ziel = iz.ziel;
        const float o[3] = {ziel.m[0][3], ziel.m[1][3], ziel.m[2][3]};
        ziel = dreh * ziel;
        for (int q = 0; q < 3; ++q) { ziel.m[q][3] = o[q]; }
        modell::ikZielSetzen(d.basis, s, sp, z.zugBild, ziel);
        z.schichtZug = s;
        return;
    }
    if (g.art != 0) { return; }
    // FK: ein Glied unter IK wechselt erst auf FK (eingerastet wie Max "Automatisch einrasten")
    if (gliedGriff(d.rig, g.nr)) {
        const int sp = modell::ikSpurIndex(s, ikArtFuerGriff(d.rig, g.nr));
        if (sp >= 0 && modell::ikAnteilAm(s.ik[static_cast<std::size_t>(sp)], static_cast<float>(z.zugBild)) > 0.0F) {
            modell::ikFkWechseln(d.basis, s, sp, z.zugBild, 0.0F, true);
            modell::rigWelt(d.basis, s, z.zugBild, z.zugBild, 0.0F, welt);
        }
    }
    const float nichts[3] = {0.0F, 0.0F, 0.0F};
    modell::rigGriffDrehen(d.basis, d.rig, g.nr, z.zugBild, welt, dreh, nichts, s);
    z.schichtZug = s;
}

void zugBeginnen(const FigurDaten& d, int art, const float ort[3]) {
    FigurAnimZustand& z = fa();
    const ImGuiIO& io = ImGui::GetIO();
    z.zieht = true;
    z.zugArt = art;
    z.mausStart[0] = io.MousePos.x;
    z.mausStart[1] = io.MousePos.y;
    z.zugBild = bildJetzt(d);
    z.schichtStart = d.anim.schicht;
    z.schichtZug = d.anim.schicht;
    z.ersatzFigur = d.actor;
    const Camera& c = g_app->gezeigteKamera;
    float f[3];
    c.forward(f);
    const float tiefe = std::max(16.0F, (ort[0] - c.pos[0]) * f[0] + (ort[1] - c.pos[1]) * f[1] + (ort[2] - c.pos[2]) * f[2]);
    const float bildH = std::max(1.0F, g_app->kartenAnsicht[3] - g_app->kartenAnsicht[1]);
    z.zugSkala = 2.0F * tiefe * std::tan(c.fovY * 3.14159265F / 360.0F) / bildH;
    zeitAufBild(d, z.zugBild);
}

void zugEnde(bool uebernehmen) {
    FigurAnimZustand& z = fa();
    if (!z.zieht) { return; }
    z.zieht = false;
    z.modal = false;
    z.ersatzFigur = -1;
    z.weltErsatz.clear();
    if (!uebernehmen) { return; }
    const std::shared_ptr<FigurDaten> d = daten(z.figur);
    if (!d->da) { return; }
    modell::FigurAnim a = d->anim;
    a.schicht = z.schichtZug;
    const Str was = z.zugArt == 3 ? Str::FaUndoPfad : (z.zugArt == 1 ? Str::FaUndoVerschieben : Str::FaUndoDrehen);
    (void)schreiben(a, d.get(), was);
}

// --- Aktionen ------------------------------------------------------------------------------------------------------------

bool figurDa() {
    const std::shared_ptr<FigurDaten> d = daten(fa().figur);
    return d->da;
}

void schluesselSetzen() {
    (void)schichtAendern(Str::FaUndoSchluessel, [](modell::FigurAnim& a, const FigurDaten& d) {
        const int bild = bildJetzt(d);
        std::vector<BoneMatrix> welt;
        const std::vector<FaGriff> v = griffeBei(d, a.schicht, bild, &welt);
        const int w = gewaehlterGriff(v);
        std::vector<int> knochen;
        if (w >= 0 && v[static_cast<std::size_t>(w)].art == 0) {
            knochen.push_back(d.rig.griffe[static_cast<std::size_t>(v[static_cast<std::size_t>(w)].nr)].knochen);
        } else {
            for (const modell::Griff& g : d.rig.griffe) {
                if (!g.ik && g.knochen >= 0) { knochen.push_back(g.knochen); }
            }
        }
        for (const int k : knochen) {
            BoneMatrix m = BoneMatrix::identity();
            if (!modell::rigWert(a.schicht, k, static_cast<float>(bild), m)) { m = BoneMatrix::identity(); }
            modell::rigSchluesselSetzen(a.schicht, k, bild, m);
        }
    });
}

void schluesselLoeschen() {
    (void)schichtAendern(Str::FaUndoLoeschen, [](modell::FigurAnim& a, const FigurDaten& d) {
        if (!fa().dsWahl.empty()) {
            for (const modell::SchluesselWahl& x : modell::wahlMitIk(a.schicht, fa().dsWahl)) { modell::rigSchluesselLoeschen(a.schicht, x.knochen, x.bild); }
            fa().dsWahl.clear();
            return;
        }
        const int bild = bildJetzt(d);
        const std::vector<FaGriff> v = griffeBei(d, a.schicht, bild);
        const int w = gewaehlterGriff(v);
        for (std::size_t g = 0; g < d.rig.griffe.size(); ++g) {
            if (w >= 0 && (v[static_cast<std::size_t>(w)].art != 0 || v[static_cast<std::size_t>(w)].nr != static_cast<int>(g))) { continue; }
            modell::rigSchluesselLoeschen(a.schicht, d.rig.griffe[g].knochen, bild);
        }
    });
}

void ikSchalten(modell::IkArt art, bool ik) {
    (void)schichtAendern(Str::MbIkUndoWechsel, [&](modell::FigurAnim& a, const FigurDaten& d) {
        const int sp = spurFuer(a.schicht, d, art);
        if (sp < 0) {
            melden(tr(Str::MbIkKeinGlied));
            return;
        }
        modell::ikFkWechseln(d.basis, a.schicht, sp, bildJetzt(d), ik ? 1.0F : 0.0F, fa().autoEinrasten);
    });
}

void ikEinrasten(modell::IkArt art, bool zielAufFk) {
    (void)schichtAendern(Str::MbIkUndoEinrasten, [&](modell::FigurAnim& a, const FigurDaten& d) {
        const int sp = spurFuer(a.schicht, d, art);
        if (sp < 0) { return; }
        const int bild = bildJetzt(d);
        if (zielAufFk) {
            modell::ikAufFk(d.basis, a.schicht, sp, bild, modell::ikAnteilAm(a.schicht.ik[static_cast<std::size_t>(sp)], static_cast<float>(bild)));
        } else {
            (void)modell::fkAufIk(d.basis, a.schicht, sp, bild);
        }
    });
}

// Fuss auf den Boden: das IK-Ziel senkrecht auf die Flaeche darunter (traceRay der Karte), so hoch ueber ihr wie der
// Fussknochen im ersten Bild der Grundanimation ueber der Sohle der Figur stand (Sohle = Ursprung - 24, DEFAULT_MINS_2,
// bg_public.h OpenJK/MD9).
bool fussAufBoden(modell::IkArt art) {
    if (art != modell::IkArt::BeinLinks && art != modell::IkArt::BeinRechts) {
        melden(tr(Str::FaNurBein));
        return false;
    }
    bool ok = false;
    (void)schichtAendern(Str::FaUndoBoden, [&](modell::FigurAnim& a, const FigurDaten& d) {
        const int sp = spurFuer(a.schicht, d, art);
        if (sp < 0) { return; }
        const int bild = bildJetzt(d);
        std::vector<BoneMatrix> welt;
        modell::rigWelt(d.basis, a.schicht, bild, bild, 0.0F, welt);
        const modell::IkSpur& spur = a.schicht.ik[static_cast<std::size_t>(sp)];
        const modell::IkZiel iz = modell::ikZielAm(spur, static_cast<float>(bild), welt, modell::plattformAm(a.schicht, static_cast<float>(bild)));
        BoneMatrix ziel = iz.da && iz.ik > 0.0F ? iz.ziel : modell::starr(welt[static_cast<std::size_t>(spur.ende.knochen)]);
        std::vector<BoneMatrix> erstes;
        d.basis.worldMatrices(0, erstes);
        float fussK[3];
        const float f0[3] = {erstes[static_cast<std::size_t>(spur.ende.knochen)].m[0][3], erstes[static_cast<std::size_t>(spur.ende.knochen)].m[1][3],
                             erstes[static_cast<std::size_t>(spur.ende.knochen)].m[2][3]};
        inKarte(d.fw, f0, fussK);
        const float hoehe = fussK[2] - (d.start[2] - 24.0F);
        float ortK[3];
        const float o[3] = {ziel.m[0][3], ziel.m[1][3], ziel.m[2][3]};
        inKarte(d.fw, o, ortK);
        const float von[3] = {ortK[0], ortK[1], ortK[2] + 32.0F};
        const float bis[3] = {ortK[0], ortK[1], ortK[2] - 512.0F};
        TraceTreffer t;
        if (g_app->geo.empty() || !(t = traceRay(g_app->geo, von, bis)).hit) {
            melden(tr(Str::FaKeinBoden));
            return;
        }
        const float neuK[3] = {ortK[0], ortK[1], t.point[2] + hoehe};
        const float wegK[3] = {0.0F, 0.0F, neuK[2] - ortK[2]};
        float weg[3];
        richtungInFigur(d.fw, wegK, weg);
        for (int q = 0; q < 3; ++q) { ziel.m[q][3] += weg[q]; }
        modell::ikZielSetzen(d.basis, a.schicht, sp, bild, ziel);
        ok = true;
    });
    return ok;
}

// Das IK-Ziel auf einen Punkt der Karte (Klick auf eine Flaeche: Tuer, Konsole, Boden)
bool zielAufPunkt(modell::IkArt art, const float punkt[3]) {
    bool ok = false;
    (void)schichtAendern(Str::FaUndoZiel, [&](modell::FigurAnim& a, const FigurDaten& d) {
        const int sp = spurFuer(a.schicht, d, art);
        if (sp < 0) { return; }
        const int bild = bildJetzt(d);
        std::vector<BoneMatrix> welt;
        modell::rigWelt(d.basis, a.schicht, bild, bild, 0.0F, welt);
        const modell::IkSpur& spur = a.schicht.ik[static_cast<std::size_t>(sp)];
        const modell::IkZiel iz = modell::ikZielAm(spur, static_cast<float>(bild), welt, modell::plattformAm(a.schicht, static_cast<float>(bild)));
        BoneMatrix ziel = iz.da && iz.ik > 0.0F ? iz.ziel : modell::starr(welt[static_cast<std::size_t>(spur.ende.knochen)]);
        const BoneMatrix inv = modell::starrInvers(fwMatrix(d.fw));
        for (int q = 0; q < 3; ++q) { ziel.m[q][3] = inv.m[q][0] * punkt[0] + inv.m[q][1] * punkt[1] + inv.m[q][2] * punkt[2] + inv.m[q][3]; }
        modell::ikZielSetzen(d.basis, a.schicht, sp, bild, ziel);
        ok = true;
    });
    return ok;
}

// Ab dem Bild folgt das IK-Ziel einem Knochen einer anderen Figur (Griff, Uebergabe) bis zum Ende
bool anFremdenKnochen(modell::IkArt art, const std::string& andere, const std::string& knochen) {
    const std::shared_ptr<FigurDaten> o = daten(andere);
    if (!o->da) {
        melden(tr(Str::FaAndereNichtAnimiert));
        return false;
    }
    const int k = boneIndex(o->basis, knochen);
    if (k < 0) { return false; }
    bool ok = false;
    (void)schichtAendern(Str::FaUndoFremd, [&](modell::FigurAnim& a, const FigurDaten& d) {
        const int sp = spurFuer(a.schicht, d, art);
        if (sp < 0) { return; }
        const int bild = bildJetzt(d);
        std::vector<BoneMatrix> welt;
        modell::rigWelt(d.basis, d.wirk, bild, bild, 0.0F, welt);
        const modell::IkSpur& spur = a.schicht.ik[static_cast<std::size_t>(sp)];
        const modell::IkZiel iz = modell::ikZielAm(spur, static_cast<float>(bild), welt, modell::plattformAm(a.schicht, static_cast<float>(bild)));
        const BoneMatrix ziel = iz.da && iz.ik > 0.0F ? iz.ziel : modell::starr(welt[static_cast<std::size_t>(spur.ende.knochen)]);
        // der Knochen der anderen Figur zur selben Zeit
        int f0 = 0;
        int f1 = 0;
        float an = 0.0F;
        (void)bildBei(*o, d.startMs + static_cast<double>(bild) * modell::figurBildMs(a), f0, f1, an);
        std::vector<BoneMatrix> wo;
        modell::rigWelt(o->basis, o->anim.schicht, f0, f1, an, wo);
        const BoneMatrix fremdK = fwMatrix(o->fw) * modell::starr(wo[static_cast<std::size_t>(k)]);
        const BoneMatrix zielK = fwMatrix(d.fw) * ziel;
        modell::FremdRaum fr;
        fr.art = art;
        fr.figur = andere;
        fr.knochen = knochen;
        fr.von = bild;
        fr.bis = a.bilder - 1;
        fr.versatz = modell::starrInvers(fremdK) * zielK;
        a.fremd.erase(std::remove_if(a.fremd.begin(), a.fremd.end(), [&](const modell::FremdRaum& x) { return x.art == art; }), a.fremd.end());
        a.fremd.push_back(fr);
        ok = true;
    });
    return ok;
}

bool fremdLoesen(modell::IkArt art) {
    return schichtAendern(Str::FaUndoFremd, [&](modell::FigurAnim& a, const FigurDaten&) {
        a.fremd.erase(std::remove_if(a.fremd.begin(), a.fremd.end(), [&](const modell::FremdRaum& x) { return x.art == art; }), a.fremd.end());
    });
}

// Stufe 6: Auto-Physik der Figur (Einstellungen in der Schicht, ein Rueckgaengig-Schritt im Skript)
bool physikEinstellen(Str was, const std::function<void(modell::PhysikEinstellung&, const FigurDaten&)>& f) {
    return schichtAendern(was, [&](modell::FigurAnim& a, const FigurDaten& d) { f(a.schicht.physik.e, d); });
}

}  // namespace

bool figurAnimPhysikAn(bool an) {
    return physikEinstellen(Str::PhUndoAn, [&](modell::PhysikEinstellung& e, const FigurDaten&) { e.an = an; });
}

bool figurAnimPhysik(const std::function<void(modell::PhysikEinstellung&)>& f) {
    return physikEinstellen(Str::PhUndoEinstellung, [&](modell::PhysikEinstellung& e, const FigurDaten&) { f(e); });
}

bool figurAnimPhysikFlugHier(bool landung) {
    return physikEinstellen(Str::PhUndoFlug, [&](modell::PhysikEinstellung& e, const FigurDaten& d) {
        const int b = bildJetzt(d);
        if (e.absprung < 0 || e.landung < 0) {
            const std::pair<int, int> fl = d.physik.fluege.empty() ? std::pair<int, int>{b, b} : d.physik.fluege.front();
            e.absprung = fl.first;
            e.landung = fl.second;
        }
        (landung ? e.landung : e.absprung) = b;
    });
}

int figurAnimPhysikKetten() {
    int neu = 0;
    (void)physikEinstellen(Str::PhUndoKette, [&](modell::PhysikEinstellung& e, const FigurDaten& d) {
        for (const int k : modell::kettenVorschlag(d.rig, d.basis)) {
            if (std::any_of(e.ketten.begin(), e.ketten.end(), [&](const modell::PhysikKette& c) { return c.knochen == k; })) { continue; }
            modell::PhysikKette c;
            c.knochen = k;
            e.ketten.push_back(c);
            ++neu;
        }
        e.federn = true;
    });
    melden(format(Str::PhKettenNeu, neu));
    return neu;
}

modell::PhysikBericht figurAnimPhysikBericht(const std::string& figur) {
    const std::shared_ptr<FigurDaten> d = daten(figur);
    return d->da ? d->physik : modell::PhysikBericht{};
}

modell::FigurAnim figurAnimKopie(const std::string& figur) {
    const std::shared_ptr<FigurDaten> d = daten(figur);
    return d->da ? d->anim : modell::FigurAnim{};
}

// --- Von aussen ------------------------------------------------------------------------------------------------------------

bool figurAnimierbar(int ai) {
    const App::ActorAssets* as = assetsVon(ai);
    return as != nullptr && as->skeleton != nullptr && !as->skeleton->anim.empty() && g_app->scene.actors[static_cast<std::size_t>(ai)].brushModel <= 0;
}

bool figurAnimStarten(const std::string& figur) {
    FigurAnimZustand& z = fa();
    z.offen = true;
    z.figur = figur;
    z.griffWahl = -1;
    z.dsWahl.clear();
    const int ai = actorIndex(figur);
    if (ai < 0 || !figurAnimierbar(ai)) {
        melden(tr(Str::FaKeinSkelett));
        return false;
    }
    if (modell::findeFigurBlock(g_app->doc.script(), figur)) { return true; }
    // neu: Grundanimation, die die Figur am Zeiger spielt (aus dem eigenen Skelett), sonst BOTH_STAND1
    const App::ActorAssets* as = assetsVon(ai);
    modell::FigurAnim a;
    a.figur = figur;
    a.startMs = std::max(0.0, std::round(g_app->playMs));
    const std::string jetzt = g_app->scene.actors[static_cast<std::size_t>(ai)].at(g_app->playMs).animation;
    a.basis = "BOTH_STAND1";
    for (const AnimEntry& e : eigeneAbschnitte(*as->skeleton)) {
        if (!jetzt.empty() && e.name == jetzt && klein(jetzt).rfind("both_", 0) == 0) { a.basis = jetzt; }
    }
    a.schicht.knochen.assign(as->skeleton->anim.bones.size(), {});
    a.schicht.neuInterp = modell::Interp::Bezier;   // neue Schluessel wie Max und Blender (Stufe 3b)
    // Platz: frei in der vorhandenen Karten-GLA und bei den anderen Figuren
    std::vector<const modell::FigurAnim*> fig;
    std::vector<modell::FigurAnim> andere;
    for (const std::string& n : modell::figurenImSkript(g_app->doc.script())) {
        andere.push_back(daten(n)->anim);
    }
    for (const modell::FigurAnim& x : andere) { fig.push_back(&x); }
    fig.push_back(&a);
    std::vector<std::string> belegt;
    const std::string kinoName = "models/players/_humanoid_" + karte() + "/_humanoid_" + karte();
    if (const auto it = g_app->skeletons.find(kinoName); it != g_app->skeletons.end()) {
        for (const AnimEntry& e : it->second.sections) { belegt.push_back(e.name); }
    }
    std::string f;
    const std::vector<std::string> pl = modell::plaetzeVerteilen(fig, belegt, zielspiel(), &f);
    if (pl.empty()) {
        melden(format(Str::MbKgKeinePlaetze, modell::grenzen(zielspiel()).name));
        return false;
    }
    a.platz = pl.back();
    // die Lage am Start aus der Szene (fuer SET_ORIGIN)
    FigurDaten d;
    d.skelett = as->skeleton;
    (void)modell::basisGla(as->skeleton->anim, eigeneAbschnitte(*as->skeleton), a.basis, a.bilder, a.fps, d.basis);
    d.rig = rigFuer(d.basis, as->model, as->model.animFile + "|" + std::to_string(d.basis.bones.size()) + "|" + as->model.name);
    for (const modell::Griff& g : d.rig.griffe) {
        if (g.art == modell::GriffArt::Becken) { d.becken = g.knochen; }
    }
    d.startMs = a.startMs;
    (void)figurLage(g_app->scene.actors[static_cast<std::size_t>(ai)], a.startMs, d.fw, d.start);
    if (!schreiben(a, &d, Str::FaUndoAnlegen)) { return false; }
    melden(format(Str::FaAngelegt, figur.c_str(), a.platz.c_str(), a.basis.c_str()));
    return true;
}

bool figurAnimEntfernen(const std::string& figur) {
    if (!modell::findeFigurBlock(g_app->doc.script(), figur)) { return false; }
    const Script neu = modell::mitFigurBlock(g_app->doc.script(), figur, nullptr, {});
    return skriptBlockErsetzen(neu, tr(Str::FaUndoEntfernen));
}

// Eine Einstellung des Blocks aendern (Fenster, Selbsttest)
bool figurAnimEinstellen(const std::function<void(modell::FigurAnim&)>& f) {
    return schichtAendern(Str::FaUndoEinstellung, [&](modell::FigurAnim& a, const FigurDaten&) { f(a); });
}

// Waehrend eines Zugs zeigt die Figur die Schicht des Zugs (die gebackene Karten-GLA kommt erst beim Loslassen)
void figurAnimErsatz(std::size_t ai, ActorDraw& d) {
    FigurAnimZustand& z = fa();
    d.weltErsatz = nullptr;
    if (!z.zieht || z.ersatzFigur != static_cast<int>(ai)) { return; }
    const std::shared_ptr<FigurDaten> fd = daten(z.figur);
    if (!fd->da || fd->actor != static_cast<int>(ai)) { return; }
    int f0 = 0;
    int f1 = 0;
    float an = 0.0F;
    if (!bildBei(*fd, g_app->playMs, f0, f1, an)) { return; }
    modell::rigWelt(fd->basis, z.schichtZug, f0, f1, an, z.weltErsatz);
    if (d.anim != nullptr && z.weltErsatz.size() == d.anim->bones.size()) { d.weltErsatz = &z.weltErsatz; }
}

bool figurAnimTastenAktiv() {
    const FigurAnimZustand& z = fa();
    return z.offen && z.posieren && !g_app->playing && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && figurDa();
}

// --- Die Karten-GLA der Vorschau aus allen Bloecken -------------------------------------------------------------------------

const App::Skeleton* figurenKinoFuer(const std::string& kinoName, const App::Skeleton* archiv, std::string& stand) {
    const std::vector<std::string> figuren = modell::figurenImSkript(g_app->doc.script());
    if (figuren.empty() || zielspiel() == modell::Zielspiel::Jk2Sp) {
        stand.clear();
        return archiv;
    }
    std::string text;
    for (const std::string& f : figuren) { text += zeilenText(modell::figurZeilenImSkript(g_app->doc.script(), f)); }
    const std::string schluessel = kinoName + "|" + std::to_string(std::hash<std::string>{}(text)) + "|" +
                                   std::to_string(archiv != nullptr ? archiv->anim.numFrames : 0) + "|" + std::to_string(g_app->actorAssets.size());
    stand = "|fig" + std::to_string(std::hash<std::string>{}(schluessel));
    if (g_app->figurKinoStand == schluessel) { return g_app->figurKino.anim.empty() ? archiv : &g_app->figurKino; }
    g_app->figurKinoStand = schluessel;
    g_app->figurKino = App::Skeleton{};
    std::vector<std::shared_ptr<FigurDaten>> d;
    std::vector<const modell::FigurAnim*> anims;
    for (const std::string& f : figuren) {
        std::shared_ptr<FigurDaten> x = daten(f);
        if (!x->da) {
            diag::detail("Figur animieren: " + f + " nicht gebacken - " + x->fehler);
            continue;
        }
        d.push_back(x);
        anims.push_back(&x->anim);
    }
    if (d.empty()) { return archiv; }
    std::vector<std::string> belegt;
    if (archiv != nullptr) {
        for (const AnimEntry& e : archiv->sections) { belegt.push_back(e.name); }
    }
    std::string fehler;
    const std::vector<std::string> pl = modell::plaetzeVerteilen(anims, belegt, zielspiel(), &fehler);
    std::vector<modell::FigurBauEingabe> ein;
    for (const auto& x : d) { ein.push_back(modell::FigurBauEingabe{&x->anim, &x->basis, x->wirk}); }
    std::vector<GlaAnimation> geb;
    std::vector<modell::Platz> plaetze;
    modell::FigurBauBericht br;
    GlaAnimation k;
    std::vector<AnimEntry> cfg;
    modell::ZusammenBericht zb;
    if (pl.empty() || !modell::figurenBacken(ein, pl, archiv != nullptr ? &archiv->anim : nullptr, archiv != nullptr ? archiv->sections : std::vector<AnimEntry>{},
                                              zielspiel(), geb, plaetze, &br) ||
        !modell::zusammenfuehren(plaetze, kinoName, k, cfg, &zb)) {
        diag::warn("Figur animieren: Karten-GLA nicht gebacken - " + fehler + br.fehler + zb.fehler);
        return archiv;
    }
    k.name = kinoName;
    g_app->figurKino.anim = std::move(k);
    g_app->figurKino.sections = cfg;
    diag::detail("Figur animieren: Karten-GLA " + kinoName + " gebacken, " + std::to_string(d.size()) + " Figuren, " +
                 std::to_string(g_app->figurKino.anim.numFrames) + " Bilder, " + std::to_string(cfg.size()) + " Zeilen");
    return &g_app->figurKino;
}

// Nach dem Neuaufbau der Szene: steht die Figur am Start anders als beim Schreiben (waitsignal, Kinomodus), wird SET_ORIGIN im
// selben Rueckgaengig-Schritt nachgezogen - nur direkt nach dem eigenen Schreiben (sonst kaeme es Strg+Z in die Quere).
void figurAnimNachziehen() {
    FigurAnimZustand& z = fa();
    if (z.zieht || !g_app->camTrackValid || z.geschriebenTiefe != g_app->doc.undoDepth() || z.figur.empty()) { return; }
    if (zeilenText(modell::figurZeilenImSkript(g_app->doc.script(), z.figur)) != z.geschriebenZeilen) {
        z.geschriebenTiefe = static_cast<std::size_t>(-1);
        return;
    }
    z.geschriebenTiefe = static_cast<std::size_t>(-1);
    const std::shared_ptr<FigurDaten> d = daten(z.figur);
    if (!d->da || !d->anim.setOrigin) { return; }
    const std::array<float, 3> u = ursprungFuer(d->anim, *d);
    const auto i = modell::findeFigurBlock(g_app->doc.script(), z.figur);
    if (!i) { return; }
    const Script& s = g_app->doc.script();
    std::size_t j = *i;
    while (j < s.nodes.size() && !(s.nodes[j].kind == Node::Kind::Command && s.nodes[j].name == "affect")) { ++j; }
    if (j >= s.nodes.size()) { return; }
    for (const Node& c : s.nodes[j].children) {
        if (c.name != "set" || c.args.size() < 2 || c.args[0].text != "SET_ORIGIN") { continue; }
        float v[3] = {0.0F, 0.0F, 0.0F};
        if (std::sscanf(c.args[1].text.c_str(), "%f %f %f", &v[0], &v[1], &v[2]) != 3) { return; }
        if (std::fabs(v[0] - u[0]) < 0.0015F && std::fabs(v[1] - u[1]) < 0.0015F && std::fabs(v[2] - u[2]) < 0.0015F) { return; }
        const Script neu = modell::mitFigurBlock(s, z.figur, &d->anim, u);
        g_app->doc.ersetzeImLetztenSchritt(neu);
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        ++z.nachgezogen;
        diag::detail("Figur animieren: SET_ORIGIN von " + z.figur + " nachgezogen");
        return;
    }
}

}  // namespace bhed::gui

#include "app_figuranim_ui.inc"
