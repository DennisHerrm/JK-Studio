// app_modelltab_kurven.cpp - Model-Tab, Modell-Block Stufe 3b: Dopesheet, Kurveneditor, Animationswerkzeuge, Aufraeumen,
// Bewegungspfade ziehen
//
//   Editor        seit AP1 der Spurbereich der Zeitleiste (app_modelltab_zeit.cpp; ein Lineal, Platz nur solange die Spuren
//                 offen sind): wie 3ds Max Spurleiste und "Track View - Dope Sheet" / "Track View - Curve Editor", wie
//                 Blender "Dope Sheet" / "Graph Editor". Links die Zeilen (Auswahl bzw. Zusammenfassung in der Skala,
//                 Spuren je Glied, Knochen, im Kurveneditor die Kanaele mit Auge), rechts die Zeit in ganzen Frames.
//   Dopesheet     Schluessel als Striche (wie 3ds Max) oder Rauten (wie Blender); Klick waehlt (Umschalt: dazu), Rahmen
//                 ziehen waehlt, gewaehlte ziehen verschiebt in der Zeit; die Zeilen der Auswahl und der Glieder waehlen
//                 alle Schluessel ihres Frames.
//   Kurveneditor  eine Kurve je Kanal (bhed/modell/animkurven.h, Bezier wie Blender), Schluessel ziehen (Zeit und Wert),
//                 Henkel ziehen (eine Auto-Art wird dabei Ausgerichtet bzw. Vektor Frei, wie Blender), Alles zeigen.
//   Werkzeuge     ueber Menue "Animation" > Animationswerkzeuge, Rechtsklick im Editor und in der Ansicht, Suche X/F3; die
//                 Schieber mit einem Regler (Vorschau sofort, Uebernehmen = ein Rueckgaengig-Schritt).
//   Pfade         Punkte des Becken- und Wurzelpfads in der Ansicht ziehen: der Knochen geht an diesem Frame dorthin
//                 (Verschiebung in der Bildebene, ein Schluessel); Stufe 4: an Haenden, Fuessen und der Saebelspitze ist der
//                 gezogene Punkt das IK-Ziel an diesem Frame (wie 3ds Max Bewegung > Trajektorien: Schluessel auf der Bahn
//                 verschieben), ein Rueckgaengig-Schritt.
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/animkurven.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"
#include "bhed/sicherschreiben.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

void melden(const std::string& m) {
    mt().meldung = m;
    mt().rigMeldung = m;
    addStatus(m);
}

std::string format(Str s, auto... a) {
    char b[700];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

const AnimEntry* sequenz() {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return nullptr; }
    return &g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
}

void bereich(int& erstes, int& anzahl) {
    const AnimEntry* e = sequenz();
    if (e != nullptr) {
        erstes = e->firstFrame;
        anzahl = std::max(e->numFrames, 1);
    } else {
        erstes = 0;
        anzahl = std::max(g_app->anim.bildZahl(), 1);
    }
}

int bildJetzt() { return std::clamp(g_app->animFrame, 0, std::max(g_app->anim.bildZahl() - 1, 0)); }

std::string sequenzName() {
    const AnimEntry* e = sequenz();
    return e != nullptr ? e->name : std::string();
}

const modell::RigSchicht& schicht() {
    static const modell::RigSchicht kLeer;
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    return s != nullptr ? *s : kLeer;
}

// Die Interpolation neuer Schluessel: Bezier mit Auto Clamped, wie 3ds Max (Vorgabe Auto-Tangenten) und Blender
// (ipo_new BEZT_IPO_BEZ, keyhandles_new HD_AUTO_ANIM; animrig/intern/fcurve.cc get_keyframe_settings).
constexpr modell::Interp kNeuInterp = modell::Interp::Bezier;

// Eine Aenderung der Schicht als ein Rueckgaengig-Schritt (nur Rig, das Netz bleibt).
void aendern(const std::string& was, const std::function<void(modell::RigSchicht&)>& f) {
    ModellTab& t = mt();
    const std::string name = sequenzName();
    t.dok.aendern(was, [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[name];
        s.neuInterp = kNeuInterp;
        f(s);
    }, false);
    ++t.kvAenderungen;
    g_app->modelDirty = true;
}

// Die Knochen des gewaehlten Griffs (mit den mitgedrehten und dem Drehknochen) oder der gewaehlte Knochen.
std::vector<int> auswahlKnochen() {
    const ModellTab& t = mt();
    std::vector<int> k;
    if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size()) {
        const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
        if (g.knochen >= 0) { k.push_back(g.knochen); }
        k.insert(k.end(), g.mit.begin(), g.mit.end());
        if (g.twist >= 0) { k.push_back(g.twist); }
    } else if (t.knochenWahl >= 0) {
        k.push_back(t.knochenWahl);
    }
    return k;
}

// Knochen mit Schluesseln in der Schicht
std::vector<int> animierteKnochen() {
    std::vector<int> k;
    const modell::RigSchicht& s = schicht();
    for (std::size_t b = 0; b < s.knochen.size(); ++b) {
        if (!s.knochen[b].empty()) { k.push_back(static_cast<int>(b)); }
    }
    return k;
}

// Fuer die Werkzeuge: gewaehlte Knochen, sonst alle animierten
std::vector<int> zielKnochen() {
    std::vector<int> k = auswahlKnochen();
    return k.empty() ? animierteKnochen() : k;
}

bool wahlNoetig() {
    if (!mt().kvWahl.empty()) { return true; }
    melden(tr(Str::MbKvKeineWahl));
    return false;
}

// --- Glieder (Sammelzeilen) ----------------------------------------------------------------------------------------------
int gliedVon(int b) {
    const ModellTab& t = mt();
    if (!modellRigDa() || static_cast<std::size_t>(b) >= t.rig.erkennung.knochen.size()) { return 5; }
    const modell::KnochenErkennung& k = t.rig.erkennung.knochen[static_cast<std::size_t>(b)];
    using R = modell::Rolle;
    if (modell::armRolle(k.rolle) || k.rolle == R::Schluessel || k.rolle == R::Hand || k.rolle == R::Finger) {
        return k.seite == modell::Seite::Rechts ? 2 : (k.seite == modell::Seite::Links ? 1 : 5);
    }
    if (modell::beinRolle(k.rolle) || k.rolle == R::Fuss || k.rolle == R::Zehen) {
        return k.seite == modell::Seite::Rechts ? 4 : (k.seite == modell::Seite::Links ? 3 : 5);
    }
    if (k.rolle == R::Becken || k.rolle == R::Wirbel || k.rolle == R::Hals || k.rolle == R::Kopf || k.rolle == R::Gesicht || k.rolle == R::Wurzel ||
        k.rolle == R::Bewegung) {
        return 0;
    }
    return 5;
}

Str gliedText(int g) {
    static const Str kText[6] = {Str::MbKvGlTorso, Str::MbKvGlArmL, Str::MbKvGlArmR, Str::MbKvGlBeinL, Str::MbKvGlBeinR, Str::MbKvGlSonst};
    return kText[std::clamp(g, 0, 5)];
}

Str kanalText(int c) {
    static const Str kText[modell::kKanaele] = {Str::MbKvKanalQW, Str::MbKvKanalQX, Str::MbKvKanalQY, Str::MbKvKanalQZ,
                                                Str::MbKvKanalTX, Str::MbKvKanalTY, Str::MbKvKanalTZ};
    return kText[std::clamp(c, 0, modell::kKanaele - 1)];
}

ImU32 kanalFarbe(int c, float alpha = 1.0F) {
    // eigene Farben: X rot, Y gruen, Z blau, W gelb; Verschiebung heller
    static const float kF[modell::kKanaele][3] = {{0.95F, 0.80F, 0.25F}, {0.95F, 0.35F, 0.35F}, {0.40F, 0.85F, 0.40F}, {0.40F, 0.60F, 1.00F},
                                                  {1.00F, 0.60F, 0.60F}, {0.65F, 1.00F, 0.65F}, {0.65F, 0.80F, 1.00F}};
    const float* f = kF[std::clamp(c, 0, modell::kKanaele - 1)];
    return ImGui::ColorConvertFloat4ToU32(ImVec4{f[0], f[1], f[2], alpha});
}

// AP1: der Name der Figur vor dem Glied einer Spur (Entwurf "Kyle - Koerper"): der Ordner des Modells
std::string figurName() {
    std::string pfad = g_app->modelPath;
    std::replace(pfad.begin(), pfad.end(), '\\', '/');
    const std::size_t ende = pfad.find_last_of('/');
    if (ende == std::string::npos || ende == 0) { return {}; }
    const std::size_t anfang = pfad.find_last_of('/', ende - 1);
    const std::size_t ab = anfang == std::string::npos ? 0 : anfang + 1;
    return pfad.substr(ab, ende - ab);
}

// --- Zeilen ------------------------------------------------------------------------------------------------------------
struct Zeile {
    int art = 0;       // 0 Zusammenfassung, 1 Glied, 2 Knochen, 3 Kanal, 4 IK-Spur (Stufe 7: knochen = ikWahlCode)
    int glied = -1;
    int knochen = -1;
    int kanal = -1;
};

std::vector<int> gezeigteKnochen() {
    const ModellTab& t = mt();
    std::vector<int> k;
    const std::vector<int> wahl = auswahlKnochen();
    const modell::RigSchicht& s = schicht();
    for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
        const bool animiert = b < s.knochen.size() && !s.knochen[b].empty();
        if (t.kvNurAnimierte && !animiert) { continue; }
        if (t.kvNurGewaehlte && !wahl.empty() && std::find(wahl.begin(), wahl.end(), static_cast<int>(b)) == wahl.end()) { continue; }
        k.push_back(static_cast<int>(b));
    }
    return k;
}

std::vector<Zeile> zeilen(bool kanaele) {
    ModellTab& t = mt();
    std::vector<Zeile> z;
    z.push_back(Zeile{0, -1, -1, -1});
    const std::vector<int> k = gezeigteKnochen();
    for (int g = 0; g < 6; ++g) {
        std::vector<int> drin;
        for (const int b : k) {
            if (gliedVon(b) == g) { drin.push_back(b); }
        }
        if (drin.empty()) { continue; }
        z.push_back(Zeile{1, g, -1, -1});
        if (t.kvGliedZu[static_cast<std::size_t>(g)] != 0) { continue; }
        for (const int b : drin) {
            z.push_back(Zeile{2, g, b, -1});
            if (kanaele && t.kvKnochenAuf.count(b) != 0U) {
                for (int c = 0; c < modell::kKanaele; ++c) { z.push_back(Zeile{3, g, b, c}); }
            }
        }
    }
    // Stufe 7 (offen aus Stufe 4): je IK-Spur mit Schluesseln eine Zeile; ihre Rauten verschieben sich wie die der Knochen
    // (und mit den FK-Schluesseln ihrer Kette, modell::wahlMitIk); nur im Dopesheet, der Kurveneditor zeigt Knochenkanaele
    const modell::RigSchicht& s = schicht();
    for (std::size_t i = 0; i < s.ik.size() && !kanaele; ++i) {
        if (!s.ik[i].ziel.empty()) { z.push_back(Zeile{4, -1, modell::ikWahlCode(static_cast<int>(i)), -1}); }
    }
    return z;
}

// Die Schluessel einer Zeile: Knochen oder IK-Spur (nullptr: keine)
const std::vector<modell::RigSchluessel>* schluesselDer(const modell::RigSchicht& s, int k) {
    if (const int sp = modell::ikWahlSpur(k); sp >= 0) { return static_cast<std::size_t>(sp) < s.ik.size() ? &s.ik[static_cast<std::size_t>(sp)].ziel : nullptr; }
    return k >= 0 && static_cast<std::size_t>(k) < s.knochen.size() ? &s.knochen[static_cast<std::size_t>(k)] : nullptr;
}

// pfix H6: dasselbe zum Aendern - in der Wahl stehen auch IK-Schluessel (ikWahlCode, negativ), nie s.knochen[code]
std::vector<modell::RigSchluessel>* schluesselDer(modell::RigSchicht& s, int k) {
    return const_cast<std::vector<modell::RigSchluessel>*>(schluesselDer(static_cast<const modell::RigSchicht&>(s), k));
}

// pfix H6: die Henkel der IK-Spuren in der Wahl neu (modell::henkelNeu rechnet nur die Knochen)
void ikHenkelDerWahl(modell::RigSchicht& s, const modell::Wahl& w) {
    std::set<int> spuren;
    for (const modell::SchluesselWahl& x : w) {
        if (const int sp = modell::ikWahlSpur(x.knochen); sp >= 0 && static_cast<std::size_t>(sp) < s.ik.size()) { spuren.insert(sp); }
    }
    for (const int sp : spuren) { modell::ikHenkelNeu(s.ik[static_cast<std::size_t>(sp)]); }
}

// Die Knochen einer Zeile (Zusammenfassung: alle gezeigten, Glied: seine)
std::vector<int> knochenDer(const Zeile& z) {
    if (z.art >= 2) { return {z.knochen}; }   // Knochen, Kanal, IK-Spur
    std::vector<int> k;
    for (const int b : gezeigteKnochen()) {
        if (z.art == 0 || gliedVon(b) == z.glied) { k.push_back(b); }
    }
    return k;
}

bool gewaehlt(int k, int bild) {
    for (const modell::SchluesselWahl& w : mt().kvWahl) {
        if (w.knochen == k && w.bild == bild) { return true; }
    }
    return false;
}

void waehlen(int k, int bild, bool dazu) {
    ModellTab& t = mt();
    if (!dazu) { t.kvWahl.clear(); }
    if (!gewaehlt(k, bild)) { t.kvWahl.push_back({k, bild}); }
    t.kvLetzterKnochen = k;
}

// --- Aktionen ------------------------------------------------------------------------------------------------------------

void wahlAusSchicht() {
    // Auswahl auf vorhandene Schluessel beschraenken (nach Loeschen, Rueckgaengig)
    ModellTab& t = mt();
    const modell::RigSchicht& s = schicht();
    t.kvWahl.erase(std::remove_if(t.kvWahl.begin(), t.kvWahl.end(), [&](const modell::SchluesselWahl& w) { return !s.hatSchluessel(w.knochen, w.bild); }),
                   t.kvWahl.end());
}

bool schluesselSetzen() {
    if (modellZeitOffenUebernehmen()) { return true; }   // AP1: Auto Key aus - die eben gestellte Pose behaelt ihre Schluessel
    std::vector<int> k = auswahlKnochen();
    if (k.empty() || g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    const int bild = bildJetzt();
    aendern(tr(Str::MbKvSetzen), [&](modell::RigSchicht& s) {
        for (const int b : k) {
            BoneMatrix m = BoneMatrix::identity();
            if (!modell::rigWert(s, b, static_cast<float>(bild), m) && s.bindeBasis) {
                const GlaBone& kb = g_app->anim.bones[static_cast<std::size_t>(b)];
                m = kb.basePoseInv * g_app->anim.localMatrix(bild, b) * kb.basePose;
            }
            modell::rigSchluesselSetzen(s, b, bild, m);
        }
    });
    for (const int b : k) { waehlen(b, bild, b != k.front()); }
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvSetzen), static_cast<int>(k.size())));
    return true;
}

bool kopieren() {
    ModellTab& t = mt();
    if (!wahlNoetig()) { return false; }
    t.kvAblage.clear();
    int erster = 1 << 30;
    for (const modell::SchluesselWahl& w : t.kvWahl) { erster = std::min(erster, w.bild); }
    const modell::RigSchicht& s = schicht();
    for (const modell::SchluesselWahl& w : t.kvWahl) {
        // pfix H6: nur Knochen-Schluessel (die Ablage traegt keinen IkWert - IK-Schluessel bleiben draussen)
        if (w.knochen < 0 || static_cast<std::size_t>(w.knochen) >= s.knochen.size()) { continue; }
        for (const modell::RigSchluessel& k : s.knochen[static_cast<std::size_t>(w.knochen)]) {
            if (k.bild != w.bild) { continue; }
            modell::RigSchluessel x = k;
            x.bild -= erster;
            t.kvAblage.emplace_back(w.knochen, x);
        }
    }
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvKopieren), static_cast<int>(t.kvAblage.size())));
    return true;
}

bool einfuegen() {
    ModellTab& t = mt();
    if (t.kvAblage.empty()) {
        melden(tr(Str::MbKvKeineWahl));
        return false;
    }
    const int bild = bildJetzt();
    const auto ablage = t.kvAblage;
    aendern(tr(Str::MbKvEinfuegen), [&](modell::RigSchicht& s) {
        for (const auto& [b, k] : ablage) {
            if (s.knochen.size() <= static_cast<std::size_t>(b)) { s.knochen.resize(static_cast<std::size_t>(b) + 1U); }
            auto& v = s.knochen[static_cast<std::size_t>(b)];
            modell::RigSchluessel x = k;
            x.bild += bild;
            v.erase(std::remove_if(v.begin(), v.end(), [&](const modell::RigSchluessel& y) { return y.bild == x.bild; }), v.end());
            v.insert(std::lower_bound(v.begin(), v.end(), x.bild, [](const modell::RigSchluessel& a, int f) { return a.bild < f; }), x);
            (void)modell::quatFolge(v);
            modell::henkelNeu(v);
        }
    });
    t.kvWahl.clear();
    for (const auto& [b, k] : ablage) { t.kvWahl.push_back({b, k.bild + bild}); }
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvEinfuegen), static_cast<int>(ablage.size())));
    return true;
}

bool loeschen() {
    ModellTab& t = mt();
    if (!wahlNoetig()) { return false; }
    const modell::Wahl w = t.kvWahl;
    aendern(tr(Str::MbKvLoeschen), [&](modell::RigSchicht& s) {
        for (const modell::SchluesselWahl& x : w) { modell::rigSchluesselLoeschen(s, x.knochen, x.bild); }
    });
    t.kvWahl.clear();
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvLoeschen), static_cast<int>(w.size())));
    return true;
}

void alleWaehlen() {
    ModellTab& t = mt();
    t.kvWahl.clear();
    const modell::RigSchicht& s = schicht();
    for (const int b : gezeigteKnochen()) {
        if (static_cast<std::size_t>(b) >= s.knochen.size()) { continue; }
        for (const modell::RigSchluessel& k : s.knochen[static_cast<std::size_t>(b)]) { t.kvWahl.push_back({b, k.bild}); }
    }
}

void interpSetzen(modell::Interp ip) {
    ModellTab& t = mt();
    if (!wahlNoetig()) { return; }
    const modell::Wahl w = t.kvWahl;
    aendern(tr(Str::MbKvGruppeInterp), [&](modell::RigSchicht& s) {
        for (const modell::SchluesselWahl& x : w) {
            std::vector<modell::RigSchluessel>* v = schluesselDer(s, x.knochen);   // pfix H6
            if (v == nullptr) { continue; }
            for (modell::RigSchluessel& k : *v) {
                if (k.bild == x.bild) { k.interp = ip; }
            }
        }
        modell::henkelNeu(s);
        ikHenkelDerWahl(s, w);
    });
}

void henkelSetzen(modell::Henkel h) {
    ModellTab& t = mt();
    if (!wahlNoetig()) { return; }
    const modell::Wahl w = t.kvWahl;
    aendern(tr(Str::MbKvGruppeTangenten), [&](modell::RigSchicht& s) {
        for (const modell::SchluesselWahl& x : w) {
            std::vector<modell::RigSchluessel>* v = schluesselDer(s, x.knochen);   // pfix H6
            if (v == nullptr) { continue; }
            for (modell::RigSchluessel& k : *v) {
                if (k.bild != x.bild) { continue; }
                for (int c = 0; c < modell::kKanaele; ++c) { k.henkelL[c] = k.henkelR[c] = h; }
                if (k.interp != modell::Interp::Bezier) { k.interp = modell::Interp::Bezier; }   // Tangenten wirken nur auf Bezier
            }
        }
        modell::henkelNeu(s);
        ikHenkelDerWahl(s, w);
    });
}

// --- Regler (Schieber mit Vorschau) ------------------------------------------------------------------------------------
enum class ReglerArt : int { Keine = 0, Kurve, Pose, Einfuegen, Entfernen, Versatz };

void reglerOeffnen(ReglerArt art, int werkzeug, float vorgabe, float lo, float hi, Str text, Str hinweis) {
    ModellTab& t = mt();
    if (art == ReglerArt::Kurve && !wahlNoetig()) { return; }
    if (art == ReglerArt::Versatz && !wahlNoetig()) { return; }
    t.kvRegler = static_cast<int>(art);
    t.kvReglerWerkzeug = werkzeug;
    t.kvReglerWert = vorgabe;
    t.kvReglerLo = lo;
    t.kvReglerHi = hi;
    t.kvReglerText = static_cast<int>(text);
    t.kvReglerHinweis = static_cast<int>(hinweis);
    t.kvReglerStart = schicht();
    t.kvReglerAngewandt = false;
    t.kvReglerOeffnen = true;
}

modell::RigSchicht reglerErgebnis() {
    ModellTab& t = mt();
    modell::RigSchicht s = t.kvReglerStart;
    s.neuInterp = kNeuInterp;
    switch (static_cast<ReglerArt>(t.kvRegler)) {
        case ReglerArt::Kurve:
            (void)modell::kurvenWerkzeug(s, t.kvWahl, static_cast<modell::KurvenWerkzeug>(t.kvReglerWerkzeug), t.kvReglerWert);
            break;
        case ReglerArt::Pose:
            (void)modell::poseSchieben(s, zielKnochen(), bildJetzt(), static_cast<modell::PosenSchieber>(t.kvReglerWerkzeug), t.kvReglerWert);
            break;
        case ReglerArt::Einfuegen: (void)modell::zeitEinfuegen(s, bildJetzt(), static_cast<int>(std::lround(t.kvReglerWert))); break;
        case ReglerArt::Entfernen: (void)modell::zeitEntfernen(s, bildJetzt(), static_cast<int>(std::lround(t.kvReglerWert))); break;
        case ReglerArt::Versatz: (void)modell::zeitVersatz(s, t.kvWahl, t.kvReglerWert); break;
        case ReglerArt::Keine: break;
    }
    return s;
}

void reglerVorschau() {
    ModellTab& t = mt();
    const modell::RigSchicht neu = reglerErgebnis();
    const std::string name = sequenzName();
    if (!t.kvReglerAngewandt) {
        t.kvReglerSchritt = t.dok.aendern(tr(static_cast<Str>(t.kvReglerText)), [&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
        t.kvReglerAngewandt = true;
    } else {
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
    }
    ++t.kvAenderungen;
    g_app->modelDirty = true;
}

void reglerZu(bool behalten) {
    ModellTab& t = mt();
    if (!behalten && t.kvReglerAngewandt) { (void)t.dok.abbrechen(t.kvReglerSchritt); }
    if (behalten && !t.kvReglerAngewandt) { reglerVorschau(); }
    if (behalten) {
        const ReglerArt art = static_cast<ReglerArt>(t.kvRegler);
        const int n = art == ReglerArt::Pose ? static_cast<int>(zielKnochen().size()) : static_cast<int>(t.kvWahl.size());
        melden(format(Str::MbKvGeaendert, tr(static_cast<Str>(t.kvReglerText)), n));
        wahlAusSchicht();
    }
    t.kvRegler = 0;
    g_app->modelDirty = true;
}

void drawRegler() {
    ModellTab& t = mt();
    if (t.kvReglerOeffnen) {
        ImGui::OpenPopup("###kvregler");
        t.kvReglerOeffnen = false;
    }
    if (t.kvRegler == 0) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 22.0F, 0.0F}, ImGuiCond_Appearing);
    if (!ImGui::BeginPopup("###kvregler")) {
        // ausserhalb geklickt: wie Abbrechen
        if (t.kvRegler != 0) { reglerZu(false); }
        return;
    }
    t.kvReglerSichtbar = ImGui::GetFrameCount();
    cjkTextUnformatted(tr(static_cast<Str>(t.kvReglerText)));
    ImGui::PushTextWrapPos(fs * 21.0F);
    cjkTextDisabled("%s", tr(static_cast<Str>(t.kvReglerHinweis)));
    ImGui::PopTextWrapPos();
    const ReglerArt art = static_cast<ReglerArt>(t.kvRegler);
    ImGui::SetNextItemWidth(-1.0F);
    bool geaendert = false;
    if (art == ReglerArt::Einfuegen || art == ReglerArt::Entfernen) {
        int n = static_cast<int>(std::lround(t.kvReglerWert));
        geaendert = ImGui::SliderInt((std::string(tr(Str::MbKvBilderZahl)) + "###kvwert").c_str(), &n, 1, 60);
        t.kvReglerWert = static_cast<float>(n);
    } else {
        geaendert = ImGui::SliderFloat((std::string(tr(art == ReglerArt::Versatz ? Str::MbKvBilderZahl : Str::MbKvFaktor)) + "###kvwert").c_str(), &t.kvReglerWert,
                                       t.kvReglerLo, t.kvReglerHi, "%.2f");
    }
    ImGui::SetItemTooltip("%s", tr(Str::MbKvReglerHint));
    if (geaendert && art != ReglerArt::Einfuegen && art != ReglerArt::Entfernen) { reglerVorschau(); }
    if (ImGui::Button(tr(Str::MbKvAnwenden)) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
        reglerZu(true);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MbKvAbbrechen)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        reglerZu(false);
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// --- weitere Werkzeuge -------------------------------------------------------------------------------------------------

void wahlWerkzeug(Str text, const std::function<modell::Wahl(modell::RigSchicht&, const modell::Wahl&)>& f) {
    ModellTab& t = mt();
    if (!wahlNoetig()) { return; }
    const modell::Wahl w = t.kvWahl;
    modell::Wahl neu;
    aendern(tr(text), [&](modell::RigSchicht& s) { neu = f(s, w); });
    t.kvWahl = neu;
    melden(format(Str::MbKvGeaendert, tr(text), static_cast<int>(neu.size())));
}

void zeitenTeilen(bool vomLetzten) {
    ModellTab& t = mt();
    std::vector<int> k = modell::wahlKnochen(t.kvWahl);
    if (k.size() < 2U) {
        melden(tr(Str::MbKvKeineWahl));
        return;
    }
    int n = 0;
    const int letzter = std::find(k.begin(), k.end(), t.kvLetzterKnochen) != k.end() ? t.kvLetzterKnochen : k.back();
    aendern(tr(vomLetzten ? Str::MbKvZeitenVomLetzten : Str::MbKvZeitenTeilen), [&](modell::RigSchicht& s) { n = modell::zeitenTeilen(s, k, vomLetzten ? letzter : -1); });
    wahlAusSchicht();
    melden(format(Str::MbKvGeaendert, tr(vomLetzten ? Str::MbKvZeitenVomLetzten : Str::MbKvZeitenTeilen), n));
}

bool animationSpiegeln() {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    if (g_app->anim.empty() || sequenz() == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return false;
    }
    if (!modellRigDa() && !modellRigAnlegen()) { return false; }
    int n = 0;
    const modell::Rig& r = mt().rig;
    aendern(tr(Str::MbKvAnimSpiegeln), [&](modell::RigSchicht& s) { n = modell::animationSpiegeln(g_app->anim, r, s, erstes, anzahl); });
    melden(format(Str::MbKvGespiegelt, n, anzahl));
    return true;
}

bool reduzieren() {
    ModellTab& t = mt();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const AnimEntry* e = sequenz();
    if (g_app->anim.empty() || e == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return false;
    }
    modell::RigSchicht neu;
    modell::ReduzierBericht br;
    // Stufe 4: nur die gewaehlten Knochen und/oder eine Spanne (aus 3b offen)
    modell::ReduzierAuswahl aw;
    const bool mitAuswahl = t.kvRedNurGewaehlte || t.kvRedSpanne;
    if (t.kvRedNurGewaehlte) {
        aw.knochen = modell::wahlKnochen(t.kvWahl);
        if (aw.knochen.empty()) { aw.knochen = auswahlKnochen(); }
        if (aw.knochen.empty()) {
            melden(tr(Str::MbRgKeineAuswahl));
            return false;
        }
    }
    if (t.kvRedSpanne) {
        aw.von = std::clamp(std::min(t.kvRedVon, t.kvRedBis), erstes, erstes + anzahl - 1);
        aw.bis = std::clamp(std::max(t.kvRedVon, t.kvRedBis), erstes, erstes + anzahl - 1);
    }
    if (!modell::rigReduzieren(g_app->anim, schicht(), erstes, anzahl, neu, &br, mitAuswahl ? &aw : nullptr)) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehler).c_str()));
        return false;
    }
    const std::string name = e->name;
    t.dok.aendern(tr(Str::MbKvReduzieren), [&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
    t.kvBericht = br;
    t.kvWahl.clear();
    ++t.kvAenderungen;
    g_app->modelDirty = true;
    if (mitAuswahl) {
        melden(format(Str::MbIkReduziert, e->name.c_str(), br.gewaehlt, br.nachher, br.behalten, br.dicht, br.aussen, br.maxAnteil * 100.0));
    } else {
        melden(format(Str::MbKvReduziert, e->name.c_str(), br.nachher, br.vorher, br.linear, br.bezier, br.statisch, br.maxAnteil * 100.0, br.eineStufe));
    }
    return true;
}

// Stufe 4: die Spanne aus den gewaehlten Schluesseln (sonst das aktuelle Bild bis zum Ende der Sequenz)
void spanneAusWahl() {
    ModellTab& t = mt();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    if (!t.kvWahl.empty()) {
        t.kvRedVon = t.kvRedBis = t.kvWahl.front().bild;
        for (const modell::SchluesselWahl& w : t.kvWahl) {
            t.kvRedVon = std::min(t.kvRedVon, w.bild);
            t.kvRedBis = std::max(t.kvRedBis, w.bild);
        }
    } else {
        t.kvRedVon = bildJetzt();
        t.kvRedBis = erstes + anzahl - 1;
    }
}

void aufraeumen(Str text, int art) {
    int n = 0;
    std::vector<int> k = auswahlKnochen();
    aendern(tr(text), [&](modell::RigSchicht& s) {
        if (art == 0) { n = modell::ueberfluessigeEntfernen(s, k); }
        if (art == 1) { n = modell::statischeEntfernen(s, k); }
        if (art == 2) { n = modell::quatFilter(s, k); }
    });
    wahlAusSchicht();
    melden(format(Str::MbKvGeaendert, tr(text), n));
}

bool poseSpeichern() {
    ModellTab& t = mt();
    if (g_app->anim.empty() || g_app->modellWelt.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    std::string name = t.kvPoseName;
    if (name.empty()) { name = "pose " + std::to_string(t.dok.inhalt().rig.posen.size() + 1U); }
    const modell::BibliothekPose p = modell::bibliothekPose(name, modell::poseAus(g_app->anim, g_app->modellWelt));
    t.dok.aendern(tr(Str::MbKvPoseSpeichern), [&](modell::Inhalt& in) {
        auto& l = in.rig.posen;
        l.erase(std::remove_if(l.begin(), l.end(), [&](const modell::BibliothekPose& x) { return x.name == name; }), l.end());
        l.push_back(p);
    }, false);
    t.kvPoseWahl = static_cast<int>(t.dok.inhalt().rig.posen.size()) - 1;
    melden(format(Str::MbKvPoseGespeichert, name.c_str()));
    return true;
}

bool poseAnwenden(bool gespiegelt) {
    ModellTab& t = mt();
    const auto& l = t.dok.inhalt().rig.posen;
    if (t.kvPoseWahl < 0 || static_cast<std::size_t>(t.kvPoseWahl) >= l.size()) {
        melden(tr(Str::MbKvPoseLeer));
        return false;
    }
    if (gespiegelt && !modellRigDa() && !modellRigAnlegen()) { return false; }
    const modell::RigPose p = modell::ausBibliothek(l[static_cast<std::size_t>(t.kvPoseWahl)]);
    const std::vector<int> nur = auswahlKnochen();
    const int bild = bildJetzt();
    int n = 0;
    const modell::Rig& r = t.rig;
    aendern(tr(gespiegelt ? Str::MbKvPoseGespiegelt : Str::MbKvPoseAnwenden), [&](modell::RigSchicht& s) { n = modell::poseEinfuegen(g_app->anim, r, p, gespiegelt, bild, nur, s); });
    melden(format(Str::MbKvGeaendert, tr(gespiegelt ? Str::MbKvPoseGespiegelt : Str::MbKvPoseAnwenden), n));
    return true;
}

void poseLoeschen() {
    ModellTab& t = mt();
    const auto& l = t.dok.inhalt().rig.posen;
    if (t.kvPoseWahl < 0 || static_cast<std::size_t>(t.kvPoseWahl) >= l.size()) { return; }
    const int i = t.kvPoseWahl;
    t.dok.aendern(tr(Str::MbKvPoseLoeschen), [&](modell::Inhalt& in) { in.rig.posen.erase(in.rig.posen.begin() + i); }, false);
    t.kvPoseWahl = -1;
}

// --- Stufe 4: Posen-Bibliothek als Datei (eigener Ordner, nie im Spielordner) ------------------------------------------

std::string pfadPlus(const std::string& a, const std::string& b) {
    const std::u8string r = (utf8Pfad(a) / utf8Pfad(b)).u8string();
    return std::string(r.begin(), r.end());
}

std::string dateiName(std::string n) {
    for (char& c : n) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') { c = '_'; }
    }
    while (!n.empty() && (n.back() == ' ' || n.back() == '.')) { n.pop_back(); }
    return n.empty() ? std::string("posen") : n;
}

}  // namespace

// pfix H1: Regler und Zug ohne Uebernehmen schliessen (Strg+Z bei offenem Regler)
void modellKvOffenesAbbrechen() {
    ModellTab& t = mt();
    if (t.kvRegler != 0) { reglerZu(false); }
    if (t.kvModal != 0) {
        if (t.kvZugAngelegt) {
            (void)t.dok.abbrechen(t.kvZugSchritt);
            t.kvWahl = t.kvZugWahl;
        }
        t.kvModal = 0;
        t.kvModalLaeuft = false;
        t.kvModalEndeBild = ImGui::GetFrameCount();
    }
}

std::string modellPosenOrdner() {
    // Dokumente\behaved\posen (wie die Missionen), im Selbsttest BHED_POSEN
    if (const char* p = std::getenv("BHED_POSEN"); p != nullptr && p[0] != '\0') { return p; }
    const std::string d = platform::dokumenteOrdner();
    return d.empty() ? std::string() : pfadPlus(d, "behaved\\posen");
}

bool modellPosenSpeichern() {
    ModellTab& t = mt();
    const auto& posen = t.dok.inhalt().rig.posen;
    if (posen.empty()) {
        melden(tr(Str::MbKvPoseLeer));
        return false;
    }
    const std::string ordner = modellPosenOrdner();
    if (ordner.empty()) {
        melden(tr(Str::MbIkPosenKeinOrdner));
        return false;
    }
    const std::string name = dateiName(t.posenDatei[0] != '\0' ? std::string(t.posenDatei) : std::string("posen"));
    const std::string datei = pfadPlus(ordner, name + ".jkposen");
    if (modellImSpielordner(datei)) {
        melden(format(Str::MbIkPosenSpielordner, datei.c_str()));
        return false;
    }
    // REST N7: wie alle anderen Schreiber des Model-Tabs erst daneben, dann umbenennen - ein Fehler mitten im Schreiben
    // laesst die alte Datei stehen (bis hier ofstream mit trunc: eine halbe .jkposen, die posenAusText ganz ablehnt)
    std::string fehler;
    if (!sicherSchreiben(datei, modell::posenText(posen), &fehler)) {
        diag::detail("Model-Tab: Posen - " + fehler);
        melden(format(Str::MbSpFehler, datei.c_str()));
        return false;
    }
    t.posenOrdnerJetzt = ordner;
    melden(format(Str::MbIkPosenGespeichert, static_cast<int>(posen.size()), datei.c_str()));
    return true;
}

bool modellPosenLaden(const std::string& datei) {
    ModellTab& t = mt();
    std::ifstream f(utf8Pfad(datei), std::ios::binary);
    std::stringstream b;
    b << f.rdbuf();
    std::vector<modell::BibliothekPose> posen = t.dok.inhalt().rig.posen;
    const std::size_t vorher = posen.size();
    std::string fehler;
    if (!f || !modell::posenAusText(b.str(), posen, fehler)) {
        melden(format(Str::MbIkPosenFehler, datei.c_str(), fehler.c_str()));
        return false;
    }
    t.dok.aendern(tr(Str::MbIkUndoPosen), [&](modell::Inhalt& in) { in.rig.posen = posen; }, false);
    t.kvPoseWahl = posen.empty() ? -1 : 0;
    melden(format(Str::MbIkPosenGeladen, static_cast<int>(posen.size() - std::min(vorher, posen.size())), static_cast<int>(posen.size()), datei.c_str()));
    return true;
}

namespace {

// Die Posen-Dateien des Ordners (Name ohne Endung, ganzer Pfad)
std::vector<std::pair<std::string, std::string>> posenDateien() {
    std::vector<std::pair<std::string, std::string>> d;
    std::error_code ec;
    const std::string ordner = modellPosenOrdner();
    if (ordner.empty()) { return d; }
    for (const auto& e : std::filesystem::directory_iterator(utf8Pfad(ordner), ec)) {
        if (!e.is_regular_file(ec) || e.path().extension() != ".jkposen") { continue; }
        const std::u8string n = e.path().stem().u8string();
        const std::u8string v = e.path().u8string();
        d.emplace_back(std::string(n.begin(), n.end()), std::string(v.begin(), v.end()));
    }
    std::sort(d.begin(), d.end());
    return d;
}

void posenLadenMenue() {
    const auto d = posenDateien();
    if (d.empty()) { cjkTextDisabled("%s", format(Str::MbIkPosenKeine, modellPosenOrdner().c_str()).c_str()); }
    for (const auto& [n, pfad] : d) {
        if (ImGui::MenuItem(n.c_str())) { (void)modellPosenLaden(pfad); }
        ImGui::SetItemTooltip("%s", pfad.c_str());
    }
}

void markerGrenzen() {
    const AnimEntry* e = sequenz();
    if (e == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return;
    }
    int n = 0;
    const std::vector<int> k = zielKnochen();
    aendern(tr(Str::MbKvMkGrenzen), [&](modell::RigSchicht& s) { n = modell::markerSchluessel(s, k, e->firstFrame, e->firstFrame + std::max(e->numFrames, 1) - 1); });
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvMkGrenzen), n));
}

void markerSchleife() {
    const AnimEntry* e = sequenz();
    if (e == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return;
    }
    int n = 0;
    const std::vector<int> k = zielKnochen();
    const int von = e->firstFrame + std::max(e->loopFrame, 0);
    aendern(tr(Str::MbKvMkSchleife), [&](modell::RigSchicht& s) { n = modell::schleifeSchliessen(s, k, von, e->firstFrame + std::max(e->numFrames, 1) - 1); });
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvMkSchleife), n));
}

void markerWaehlen() {
    ModellTab& t = mt();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    t.kvWahl.clear();
    const modell::RigSchicht& s = schicht();
    for (const int b : gezeigteKnochen()) {
        if (static_cast<std::size_t>(b) >= s.knochen.size()) { continue; }
        for (const modell::RigSchluessel& k : s.knochen[static_cast<std::size_t>(b)]) {
            if (k.bild >= erstes && k.bild < erstes + anzahl) { t.kvWahl.push_back({b, k.bild}); }
        }
    }
    melden(format(Str::MbKvGeaendert, tr(Str::MbKvMkWaehlen), static_cast<int>(t.kvWahl.size())));
}

void editorOeffnen(int art) {
    ModellTab& t = mt();
    if (t.kvOffen && t.kvArt == art) {
        t.kvOffen = false;
    } else {
        t.kvOffen = true;
        t.kvArt = art;
        t.kvEinpassen = true;
    }
    if (t.bereich != ModellBereich::Animation) { t.bereich = ModellBereich::Animation; }
}

// --- Aktionsliste --------------------------------------------------------------------------------------------------------
struct KvAktion {
    Str text;
    Str hinweis;
    const char* maxTaste;       // Taste des Vorbilds im Modus wie 3ds Max ("" keine)
    const char* blenderTaste;   // ... wie Blender
    ImGuiKeyChord maxChord;
    ImGuiKeyChord blenderChord;
    const char* quelle;         // Herkunft der Taste (Tabelle)
    std::function<void()> tun;
    std::function<bool()> an;   // Haken im Menue (leer: kein Umschalter)
    int gruppe;                 // 0 Editor, 1 Schluessel, 2 Interp, 3 Tangenten, 4 Werkzeuge, 5 Pose, 6 Timing, 7 Werte, 8 Bibliothek, 9 Marker, 10 Aufraeumen
    bool imEditor = false;      // die Taste gilt nur, wenn die Maus ueber dem Editor steht (wie die Editor-Belegung von Blender)
};

const std::vector<KvAktion>& kvAktionen() {
    using K = modell::KurvenWerkzeug;
    using P = modell::PosenSchieber;
    static const std::vector<KvAktion> kListe = {
        {Str::MbKvDsOeffnen, Str::MbKvOeffnenHint, "", "", 0, 0, "", [] { editorOeffnen(0); }, [] { return mt().kvOffen && mt().kvArt == 0; }, 0},
        {Str::MbKvKeOeffnen, Str::MbKvOeffnenHint, "", "", 0, 0, "", [] { editorOeffnen(1); }, [] { return mt().kvOffen && mt().kvArt == 1; }, 0},
        {Str::MbKvNurAnimierte, Str::MbKvNurAnimierteHint, "", "", 0, 0, "", [] { mt().kvNurAnimierte = !mt().kvNurAnimierte; }, [] { return mt().kvNurAnimierte; }, 0},
        {Str::MbKvNurGewaehlte, Str::MbKvNurGewaehlteHint, "", "", 0, 0, "", [] { mt().kvNurGewaehlte = !mt().kvNurGewaehlte; }, [] { return mt().kvNurGewaehlte; }, 0},
        {Str::MbKvAllesZeigen, Str::MbKvAllesZeigenHint, "", "", 0, 0, "", [] { mt().kvEinpassen = true; }, {}, 0},
        {Str::MbKvSetzen, Str::MbKvSetzenHint, "K", "I", ImGuiKey_K, ImGuiKey_I, "max_hotkeys_de.txt: Keys einstellen K | Pose | anim.keyframe_insert | I PRESS",
         [] { (void)schluesselSetzen(); }, {}, 1},
        {Str::MbKvKopieren, Str::MbKvKopierenHint, "", "", 0, 0, "", [] { (void)kopieren(); }, {}, 1},
        {Str::MbKvEinfuegen, Str::MbKvEinfuegenHint, "", "", 0, 0, "", [] { (void)einfuegen(); }, {}, 1},
        {Str::MbKvLoeschen, Str::MbKvLoeschenHint, "", "X", 0, ImGuiKey_X, "Dopesheet | wm.call_menu | X PRESS name=DOPESHEET_MT_delete", [] { (void)loeschen(); }, {}, 1,
         true},
        {Str::MbKvAlleWaehlen, Str::MbKvAlleWaehlenHint, "", "", 0, 0, "", [] { alleWaehlen(); }, {}, 1},
        {Str::MbKvVerschieben, Str::MbKvVerschiebenHint, "", "G", 0, ImGuiKey_G, "Graph Editor | transform.translate | G PRESS", [] {
             if (wahlNoetig()) { mt().kvModal = 1; }
         },
         {}, 1, true},
        {Str::MbKvSkalieren, Str::MbKvSkalierenHint, "", "S", 0, ImGuiKey_S, "Graph Editor | transform.resize | S PRESS", [] {
             if (wahlNoetig()) { mt().kvModal = 2; }
         },
         {}, 1, true},
        {Str::MbKvIpKonstant, Str::MbKvIpHint, "", "", 0, 0, "", [] { interpSetzen(modell::Interp::Konstant); }, {}, 2},
        {Str::MbKvIpLinear, Str::MbKvIpHint, "", "", 0, 0, "", [] { interpSetzen(modell::Interp::Linear); }, {}, 2},
        {Str::MbKvIpBezier, Str::MbKvIpHint, "", "", 0, 0, "", [] { interpSetzen(modell::Interp::Bezier); }, {}, 2},
        {Str::MbKvTgAutoGeklemmt, Str::MbKvTgHint, "", "", 0, 0, "", [] { henkelSetzen(modell::Henkel::AutoGeklemmt); }, {}, 3},
        {Str::MbKvTgAuto, Str::MbKvTgHint, "", "", 0, 0, "", [] { henkelSetzen(modell::Henkel::Auto); }, {}, 3},
        {Str::MbKvTgVektor, Str::MbKvTgHint, "", "", 0, 0, "", [] { henkelSetzen(modell::Henkel::Vektor); }, {}, 3},
        {Str::MbKvTgAusgerichtet, Str::MbKvTgHint, "", "", 0, 0, "", [] { henkelSetzen(modell::Henkel::Ausgerichtet); }, {}, 3},
        {Str::MbKvTgFrei, Str::MbKvTgHint, "", "", 0, 0, "", [] { henkelSetzen(modell::Henkel::Frei); }, {}, 3},
        {Str::MbKvWzBreakdown, Str::MbKvWzBreakdownHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::Breakdown), 0.0F, -1.0F, 1.0F, Str::MbKvWzBreakdown, Str::MbKvWzBreakdownHint); }, {}, 4},
        {Str::MbKvWzNachbar, Str::MbKvWzNachbarHint, "", "Alt+D", 0, ImGuiMod_Alt | ImGuiKey_D, "Graph Editor | wm.call_menu | Alt+D PRESS name=GRAPH_MT_key_blending",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::BlendNachbar), 0.0F, -1.0F, 1.0F, Str::MbKvWzNachbar, Str::MbKvWzNachbarHint); }, {}, 4, true},
        {Str::MbKvWzVorgabe, Str::MbKvWzVorgabeHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::BlendVorgabe), 1.0F, 0.0F, 1.0F, Str::MbKvWzVorgabe, Str::MbKvWzVorgabeHint); }, {}, 4},
        {Str::MbKvWzPushPull, Str::MbKvWzPushPullHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::PushPull), 1.0F, 0.0F, 2.0F, Str::MbKvWzPushPull, Str::MbKvWzPushPullHint); }, {}, 4},
        {Str::MbKvWzEase, Str::MbKvWzEaseHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::Ease), 0.0F, -1.0F, 1.0F, Str::MbKvWzEase, Str::MbKvWzEaseHint); }, {}, 4},
        {Str::MbKvWzGlaetten, Str::MbKvWzGlaettenHint, "", "Alt+S", 0, ImGuiMod_Alt | ImGuiKey_S, "Graph Editor | wm.call_menu | Alt+S PRESS name=GRAPH_MT_key_smoothing",
         [] { reglerOeffnen(ReglerArt::Kurve, static_cast<int>(K::Glaetten), 1.0F, 0.0F, 1.0F, Str::MbKvWzGlaetten, Str::MbKvWzGlaettenHint); }, {}, 4, true},
        {Str::MbKvPsBreakdown, Str::MbKvPsBreakdownHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Pose, static_cast<int>(P::Breakdown), 0.5F, 0.0F, 1.0F, Str::MbKvPsBreakdown, Str::MbKvPsBreakdownHint); }, {}, 5},
        {Str::MbKvPsPush, Str::MbKvPsPushHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Pose, static_cast<int>(P::Push), 0.5F, 0.0F, 1.0F, Str::MbKvPsPush, Str::MbKvPsPushHint); }, {}, 5},
        {Str::MbKvPsRelax, Str::MbKvPsRelaxHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Pose, static_cast<int>(P::Relax), 0.5F, 0.0F, 1.0F, Str::MbKvPsRelax, Str::MbKvPsRelaxHint); }, {}, 5},
        {Str::MbKvPsBlend, Str::MbKvPsBlendHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Pose, static_cast<int>(P::Blend), 0.5F, 0.0F, 1.0F, Str::MbKvPsBlend, Str::MbKvPsBlendHint); }, {}, 5},
        {Str::MbKvPsRuhe, Str::MbKvPsRuheHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Pose, static_cast<int>(P::BlendRuhe), 1.0F, 0.0F, 1.0F, Str::MbKvPsRuhe, Str::MbKvPsRuheHint); }, {}, 5},
        {Str::MbKvEinrastenCursor, Str::MbKvEinrastenCursorHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvEinrastenCursor, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::schluesselEinrasten(s, w, modell::Einrasten::AufCursor, bildJetzt()); }); },
         {}, 6},
        {Str::MbKvErsterCursor, Str::MbKvErsterLetzterHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvErsterCursor, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::schluesselEinrasten(s, w, modell::Einrasten::ErsterAufCursor, bildJetzt()); }); },
         {}, 6},
        {Str::MbKvLetzterCursor, Str::MbKvErsterLetzterHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvLetzterCursor, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::schluesselEinrasten(s, w, modell::Einrasten::LetzterAufCursor, bildJetzt()); }); },
         {}, 6},
        {Str::MbKvZeitEinfuegen, Str::MbKvZeitEinfuegenHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Einfuegen, 0, 1.0F, 1.0F, 60.0F, Str::MbKvZeitEinfuegen, Str::MbKvZeitEinfuegenHint); }, {}, 6},
        {Str::MbKvZeitEntfernen, Str::MbKvZeitEntfernenHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Entfernen, 0, 1.0F, 1.0F, 60.0F, Str::MbKvZeitEntfernen, Str::MbKvZeitEntfernenHint); }, {}, 6},
        {Str::MbKvZeitVersatz, Str::MbKvZeitVersatzHint, "", "", 0, 0, "",
         [] { reglerOeffnen(ReglerArt::Versatz, 0, 0.0F, -20.0F, 20.0F, Str::MbKvZeitVersatz, Str::MbKvZeitVersatzHint); }, {}, 6},
        {Str::MbKvZeitenTeilen, Str::MbKvZeitenTeilenHint, "", "", 0, 0, "", [] { zeitenTeilen(false); }, {}, 6},
        {Str::MbKvZeitenVomLetzten, Str::MbKvZeitenVomLetztenHint, "", "", 0, 0, "", [] { zeitenTeilen(true); }, {}, 6},
        {Str::MbKvWtNull, Str::MbKvWtNullHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvWtNull, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::werteAendern(s, w, modell::WertArt::Null, bildJetzt()); }); }, {}, 7},
        {Str::MbKvWtAktuell, Str::MbKvWtAktuellHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvWtAktuell, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::werteAendern(s, w, modell::WertArt::AktuellerWert, bildJetzt()); }); }, {}, 7},
        {Str::MbKvWtNegieren, Str::MbKvWtNegierenHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvWtNegieren, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::werteAendern(s, w, modell::WertArt::Negieren, bildJetzt()); }); }, {}, 7},
        {Str::MbKvWtUmkehren, Str::MbKvWtUmkehrenHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvWtUmkehren, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::werteAendern(s, w, modell::WertArt::Umkehren, bildJetzt()); }); }, {}, 7},
        {Str::MbKvWtSpiegeln, Str::MbKvWtSpiegelnHint, "", "", 0, 0, "",
         [] { wahlWerkzeug(Str::MbKvWtSpiegeln, [](modell::RigSchicht& s, const modell::Wahl& w) { return modell::werteAendern(s, w, modell::WertArt::SpiegelnCursor, bildJetzt()); }); }, {}, 7},
        {Str::MbKvAnimOffset, Str::MbKvAnimOffsetHint, "", "", 0, 0, "", [] { mt().kvAnimOffset = !mt().kvAnimOffset; }, [] { return mt().kvAnimOffset; }, 7},
        {Str::MbKvAnimSpiegeln, Str::MbKvAnimSpiegelnHint, "", "", 0, 0, "", [] { (void)animationSpiegeln(); }, {}, 7},
        {Str::MbKvPoseSpeichern, Str::MbKvPoseSpeichernHint, "", "", 0, 0, "", [] { (void)poseSpeichern(); }, {}, 8},
        {Str::MbKvPoseAnwenden, Str::MbKvPoseAnwendenHint, "", "", 0, 0, "", [] { (void)poseAnwenden(false); }, {}, 8},
        {Str::MbKvPoseGespiegelt, Str::MbKvPoseAnwendenHint, "", "", 0, 0, "", [] { (void)poseAnwenden(true); }, {}, 8},
        {Str::MbIkPosenSpeichern, Str::MbIkPosenSpeichernHint, "", "", 0, 0, "", [] { (void)modellPosenSpeichern(); }, {}, 8},
        {Str::MbIkPosenOrdner, Str::MbIkPosenOrdnerHint, "", "", 0, 0, "", [] {
             const std::string o = modellPosenOrdner();
             std::error_code ec;
             if (!o.empty()) { std::filesystem::create_directories(utf8Pfad(o), ec); }
             if (!o.empty()) { platform::openInExplorer(o); }
         },
         {}, 8},
        {Str::MbKvMkGrenzen, Str::MbKvMkGrenzenHint, "", "", 0, 0, "", [] { markerGrenzen(); }, {}, 9},
        {Str::MbKvMkSchleife, Str::MbKvMkSchleifeHint, "", "", 0, 0, "", [] { markerSchleife(); }, {}, 9},
        {Str::MbKvMkWaehlen, Str::MbKvMkWaehlenHint, "", "", 0, 0, "", [] { markerWaehlen(); }, {}, 9},
        {Str::MbKvReduzieren, Str::MbKvReduzierenHint, "", "", 0, 0, "", [] { (void)reduzieren(); }, {}, 10},
        {Str::MbIkRedNurGewaehlte, Str::MbIkRedNurGewaehlteHint, "", "", 0, 0, "", [] { mt().kvRedNurGewaehlte = !mt().kvRedNurGewaehlte; },
         [] { return mt().kvRedNurGewaehlte; }, 10},
        {Str::MbIkRedSpanne, Str::MbIkRedSpanneHint, "", "", 0, 0, "", [] {
             mt().kvRedSpanne = !mt().kvRedSpanne;
             if (mt().kvRedSpanne) { spanneAusWahl(); }
         },
         [] { return mt().kvRedSpanne; }, 10},
        {Str::MbKvUeberfluessig, Str::MbKvUeberfluessigHint, "", "", 0, 0, "", [] { aufraeumen(Str::MbKvUeberfluessig, 0); }, {}, 10},
        {Str::MbKvStatisch, Str::MbKvStatischHint, "", "", 0, 0, "", [] { aufraeumen(Str::MbKvStatisch, 1); }, {}, 10},
        {Str::MbKvQuatFilter, Str::MbKvQuatFilterHint, "", "", 0, 0, "", [] { aufraeumen(Str::MbKvQuatFilter, 2); }, {}, 10},
    };
    return kListe;
}

constexpr int kKvGruppen = 11;
const Str kKvGruppenText[kKvGruppen] = {Str::MbKvGruppeEditor, Str::MbKvGruppeSchluessel, Str::MbKvGruppeInterp, Str::MbKvGruppeTangenten,
                                        Str::MbKvGruppeWerkzeuge, Str::MbKvGruppePose, Str::MbKvGruppeTiming, Str::MbKvGruppeWerte,
                                        Str::MbKvGruppeBibliothek, Str::MbKvGruppeMarker, Str::MbKvGruppeAufraeumen};

// Der Name einer Zeile in der Sprache und im Modus (Tangenten: wie Max andere Namen)
Str aktionName(const KvAktion& a) {
    if (!modellBlender() && a.gruppe == 3) {
        switch (a.text) {
            case Str::MbKvTgAutoGeklemmt: return Str::MbKvTgMaxAuto;
            case Str::MbKvTgAuto: return Str::MbKvTgMaxGlatt;
            case Str::MbKvTgVektor: return Str::MbKvTgMaxLinear;
            case Str::MbKvTgAusgerichtet: return Str::MbKvTgMaxAusgerichtet;
            case Str::MbKvTgFrei: return Str::MbKvTgMaxFrei;
            default: break;
        }
    }
    return a.text;
}

// In der Suche mit der Gruppe davor (Namen wie "Glaetten" oder, wie 3ds Max, "Linear" gibt es sonst doppelt: Weights,
// Interpolation und Tangente) und mit der Taste dahinter.
std::string aktionText(const KvAktion& a) {
    const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
    std::string s = std::string(tr(kKvGruppenText[std::clamp(a.gruppe, 0, kKvGruppen - 1)])) + ": " + tr(aktionName(a));
    if (taste[0] != '\0') { s += std::string(" (") + taste + ")"; }
    return s;
}

// Der Tooltip: was es tut, dazu die Taste des Modus (wenn es eine gibt)
std::string tooltipText(const KvAktion& a) {
    const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
    std::string s = tr(a.hinweis);
    if (taste[0] != '\0') { s += std::string("  [") + taste + "]"; }
    return s;
}

void ausloesen(const KvAktion& a) {
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(aktionName(a));
    g_app->modelDirty = true;
}

void gruppeAlsMenue(int g) {
    const auto& l = kvAktionen();
    for (std::size_t i = 0; i < l.size(); ++i) {
        const KvAktion& a = l[i];
        if (a.gruppe != g) { continue; }
        ImGui::PushID(static_cast<int>(i));
        const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
        if (ImGui::MenuItem(tr(aktionName(a)), taste[0] != '\0' ? taste : nullptr, a.an ? a.an() : false)) { ausloesen(a); }
        ImGui::SetItemTooltip("%s", tooltipText(a).c_str());
        ImGui::PopID();
    }
    if (g == 8) {   // die Posen der Bibliothek zum Waehlen
        const auto& posen = mt().dok.inhalt().rig.posen;
        if (!posen.empty()) { ImGui::Separator(); }
        for (std::size_t i = 0; i < posen.size(); ++i) {
            ImGui::PushID(static_cast<int>(1000 + i));
            if (ImGui::MenuItem(posen[i].name.c_str(), nullptr, mt().kvPoseWahl == static_cast<int>(i))) { mt().kvPoseWahl = static_cast<int>(i); }
            ImGui::PopID();
        }
    }
}

void alleAlsUntermenues() {
    for (int g = 0; g < kKvGruppen; ++g) {
        if (ImGui::BeginMenu(tr(kKvGruppenText[g]))) {
            gruppeAlsMenue(g);
            if (g == 8 && ImGui::BeginMenu(tr(Str::MbIkPosenLaden))) {   // Stufe 4: die Bibliothek aus einer Datei
                posenLadenMenue();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
    }
}

// --- Zeit und Werte <-> Bildpunkte ---------------------------------------------------------------------------------------
struct Feld {
    ImVec2 lo;
    ImVec2 hi;
    float von = 0.0F;   // Frames
    float bis = 1.0F;
    float yVon = -1.0F; // Kurveneditor: Werte
    float yBis = 1.0F;
    [[nodiscard]] float x(float f) const { return lo.x + (f - von) / std::max(bis - von, 1e-3F) * (hi.x - lo.x); }
    [[nodiscard]] float bildAus(float px) const { return von + (px - lo.x) / std::max(hi.x - lo.x, 1.0F) * (bis - von); }
    [[nodiscard]] float y(float w) const { return hi.y - (w - yVon) / std::max(yBis - yVon, 1e-6F) * (hi.y - lo.y); }
    [[nodiscard]] float wertAus(float py) const { return yVon + (hi.y - py) / std::max(hi.y - lo.y, 1.0F) * (yBis - yVon); }
};

Feld g_feld;   // vom letzten Bild (Selbsttest, Ziehen)
std::vector<Zeile> g_zeilen;
std::vector<float> g_zeileY;   // Mitte je Zeile (Dopesheet)

// AP1: das Lineal ist die Skala der Zeitleiste (ein Lineal fuer Zeitleiste und Spuren, Bild 0 ueberall an derselben
// Stelle; Zaehlung wie Max ab 0 in der Sequenz, wie Blender ab 1) - gezeichnet in app_modelltab_zeit.cpp.
void zeitLineal(ImDrawList* dl, const Feld& f, float oben, float hoehe) {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const AnimEntry* e = sequenz();
    modellZeitSkala(dl, ImVec2{f.lo.x, oben}, ImVec2{f.hi.x, oben + hoehe}, f.von, f.bis, erstes, e != nullptr ? anzahl : 0, e != nullptr ? e->fps : 20);
}

void zeitZeiger(ImDrawList* dl, const Feld& f, float oben) {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    modellZeitZeiger(dl, ImVec2{f.lo.x, oben}, ImVec2{f.hi.x, oben + modellZeitSkalaHoehe()}, f.von, f.bis, erstes, bildJetzt(), f.hi.y);
}

// AP1: was ein Schluessel bewegt - Bit 0 Bewegung (Verschiebung im Rahmen des Knochens, IK-Ziele), Bit 1 Drehung.
// Die Spurleiste von 3ds Max faerbt danach (Rot = Position, Gruen = Drehung); Skalierung speichert JKA nicht.
unsigned schluesselArt(const modell::RigSchluessel& k) {
    constexpr float kEps = 1e-4F;
    unsigned art = 0U;
    if (std::fabs(k.t[0]) > kEps || std::fabs(k.t[1]) > kEps || std::fabs(k.t[2]) > kEps) { art |= 1U; }
    if (std::fabs(k.q[1]) > kEps || std::fabs(k.q[2]) > kEps || std::fabs(k.q[3]) > kEps) { art |= 2U; }
    return art;
}

void bildSetzen(int b) {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    g_app->animFrame = std::clamp(b, erstes, erstes + anzahl - 1);
    g_app->animTime = 0.0F;
    g_app->animPlaying = false;
    g_app->modelDirty = true;
}

// Die Schicht waehrend eines Ziehens: aus dem Anfangsstand, mit den Schluesseln um d Frames verschoben.
void zeitZiehen(int d, float faktor, bool skalieren) {
    ModellTab& t = mt();
    modell::RigSchicht s = t.kvZugStart;
    modell::Wahl w = skalieren ? modell::schluesselSkalieren(s, t.kvZugWahl, faktor, static_cast<float>(bildJetzt())) : modell::schluesselVerschieben(s, t.kvZugWahl, d);
    const std::string name = sequenzName();
    if (!t.kvZugAngelegt) {
        t.kvZugSchritt = t.dok.aendern(tr(skalieren ? Str::MbKvSkalieren : Str::MbKvVerschieben), [&](modell::Inhalt& in) { in.rig.schichten[name] = s; }, false);
        t.kvZugAngelegt = true;
    } else {
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[name] = s; }, false);
    }
    t.kvWahl = w;
    ++t.kvAenderungen;
    g_app->modelDirty = true;
}

void zugBeginnen(float mausX) {
    ModellTab& t = mt();
    t.kvZugStart = schicht();
    t.kvZugWahl = t.kvWahl;
    t.kvZugAngelegt = false;
    t.kvZugMaus[0] = mausX;
    t.kvZugMaus[1] = ImGui::GetIO().MousePos.y;
}

// --- Dopesheet ----------------------------------------------------------------------------------------------------------

// AP1: das Dopesheet ist der Spurbereich der Zeitleiste (Entwurf zeitleiste(max)): oben die Skala, in ihr die Zeile der
// Auswahl (wie 3ds Max die Spurleiste, wie Blender die Zusammenfassung), darunter eine Spur je Glied und Knochen. Die
// Schluessel wie 3ds Max als Striche (Rot = Bewegung, Gruen = Drehung) hinter einem feinen Strich je Frame, wie Blender
// als Rauten. In der Skala: oben (Schluessel) waehlt und verschiebt die Maus Schluessel, unten (Zahlen) setzt sie das Bild.
void drawDopesheet(const ImVec2 groesse) {
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    const float zeileH = modellZeitSpurZeile();
    const float teilung = zeileH + modellZeitSpurAbstand();
    const float linksB = modellZeitNamenBreite();
    const float linealH = modellZeitSkalaHoehe();
    g_zeilen = zeilen(false);
    g_zeileY.assign(g_zeilen.size(), 0.0F);
    const modell::RigSchicht& s = schicht();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    if (t.kvEinpassen) {
        t.kvVon = static_cast<float>(erstes) - 1.0F;
        t.kvBis = static_cast<float>(erstes + anzahl);
        t.kvEinpassen = false;
    }
    ImGui::BeginChild("##dszeilen", ImVec2{groesse.x, groesse.y}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Feld f;
    f.lo = ImVec2{lo.x + linksB, lo.y + linealH};
    f.hi = ImVec2{lo.x + groesse.x, lo.y + std::max(groesse.y, linealH)};
    f.von = t.kvVon;
    f.bis = t.kvBis;
    const float spurenOben = f.lo.y + ImGui::GetStyle().ItemSpacing.y;
    const float schluesselY = modellZeitSchluesselY(lo.y);
    const float zahlenY = modellZeitSchluesselZone(lo.y);
    const std::string figur = figurName();
    // links: Namen. Die Zeile der Auswahl steht neben der Skala, die Spuren darunter (Klick auf ein Glied = auf/zu).
    modellZeitNamenText(dl, lo, tr(modellBlender() ? Str::MbKvZusammenfassung : Str::MzAuswahlSpur));
    float y = spurenOben - t.kvScrollY;
    dl->PushClipRect(ImVec2{lo.x, spurenOben}, ImVec2{f.lo.x, f.hi.y}, true);
    for (std::size_t i = 0; i < g_zeilen.size(); ++i) {
        const Zeile& z = g_zeilen[i];
        if (z.art == 0) {
            g_zeileY[i] = schluesselY;
            continue;
        }
        g_zeileY[i] = y + zeileH * 0.5F;
        if (y + zeileH >= spurenOben && y <= f.hi.y) {
            std::string name;
            float einzug = 0.0F;
            if (z.art == 1) {
                // wie im Entwurf "Kyle - Koerper": Figur und Glied; das Dreieck davor klappt die Knochen des Glieds auf und zu
                name = (figur.empty() ? std::string() : figur + " \xc2\xb7 ") + tr(gliedText(z.glied));
                const bool zu = t.kvGliedZu[static_cast<std::size_t>(z.glied)] != 0;
                const float g = std::floor(zeileH * 0.5F);
                const ImVec2 m{std::floor(lo.x + g * 0.7F), std::floor(y + zeileH * 0.5F)};
                if (zu) {
                    dl->AddTriangleFilled(ImVec2{m.x - g * 0.3F, m.y - g * 0.5F}, ImVec2{m.x - g * 0.3F, m.y + g * 0.5F}, ImVec2{m.x + g * 0.4F, m.y}, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                } else {
                    dl->AddTriangleFilled(ImVec2{m.x - g * 0.5F, m.y - g * 0.3F}, ImVec2{m.x + g * 0.5F, m.y - g * 0.3F}, ImVec2{m.x, m.y + g * 0.4F}, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                }
                einzug = fs * 0.9F;
            }
            if (z.art == 2) {
                name = g_app->anim.bones[static_cast<std::size_t>(z.knochen)].name;
                einzug = fs * 1.7F;
            }
            if (z.art == 4) {
                static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
                const int sp = modell::ikWahlSpur(z.knochen);
                name = "IK " + std::string(tr(kGlied[std::clamp(static_cast<int>(s.ik[static_cast<std::size_t>(sp)].art), 0, 3)]));
                einzug = fs * 1.7F;
            }
            const bool wahlZeile = z.art == 2 && !auswahlKnochen().empty() && auswahlKnochen().front() == z.knochen;
            dl->AddText(ImVec2{lo.x + einzug, y + (zeileH - fs) * 0.5F}, ImGui::GetColorU32(wahlZeile ? ImGuiCol_PlotHistogram : ImGuiCol_Text), name.c_str());
            ImGui::SetCursorScreenPos(ImVec2{lo.x, y});
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::InvisibleButton("##zn", ImVec2{linksB, zeileH})) {
                if (z.art == 1) { t.kvGliedZu[static_cast<std::size_t>(z.glied)] ^= 1; }
                if (z.art == 2) {
                    modellRigGriffWaehlen(-1);
                    t.knochenWahl = z.knochen;
                }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && ImGui::CalcTextSize(name.c_str()).x + einzug > linksB) { ImGui::SetTooltip("%s", name.c_str()); }
            ImGui::PopID();
        }
        y += teilung;
    }
    dl->PopClipRect();
    const float inhaltH = static_cast<float>(g_zeilen.size() > 0 ? g_zeilen.size() - 1 : 0) * teilung;
    // rechts: Skala und Spuren (ein Knopf ueber alles, damit die Maus hier dem Editor gehoert)
    ImGui::SetCursorScreenPos(ImVec2{f.lo.x, lo.y});
    ImGui::InvisibleButton("##dsfeld", ImVec2{std::max(f.hi.x - f.lo.x, 1.0F), std::max(f.hi.y - lo.y, 1.0F)}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool ueber = ImGui::IsItemHovered();
    zeitLineal(dl, f, lo.y, linealH);
    // Spuren: Grund (wie 3ds Max mit einem Strich je Frame) und Schluessel; die Zeile der Auswahl steht in der Skala
    const float r = zeileH * 0.32F;
    t.kvRauten = 0;
    for (std::size_t i = 0; i < g_zeilen.size(); ++i) {
        const bool inSkala = g_zeilen[i].art == 0;
        const float ym = g_zeileY[i];
        if (!inSkala && (ym + zeileH * 0.5F < spurenOben || ym - zeileH * 0.5F > f.hi.y)) { continue; }
        dl->PushClipRect(ImVec2{f.lo.x, inSkala ? lo.y : spurenOben}, ImVec2{f.hi.x, inSkala ? f.lo.y : f.hi.y}, true);
        if (!inSkala) { modellZeitSpurGrund(dl, ImVec2{f.lo.x, ym - zeileH * 0.5F}, ImVec2{f.hi.x, ym + zeileH * 0.5F}, f.von, f.bis, erstes, anzahl); }
        const std::vector<int> ks = knochenDer(g_zeilen[i]);
        std::map<int, unsigned> bilder;   // Bild -> was die Schluessel dort bewegen (schluesselArt)
        std::set<int> gew;
        for (const int b : ks) {
            const std::vector<modell::RigSchluessel>* liste = schluesselDer(s, b);
            if (liste == nullptr) { continue; }
            for (const modell::RigSchluessel& k : *liste) {
                bilder[k.bild] |= schluesselArt(k) | (modell::ikWahlSpur(b) >= 0 ? 1U : 0U);
                if (gewaehlt(b, k.bild)) { gew.insert(k.bild); }
            }
        }
        const float hoehe = inSkala ? std::floor(fs * 0.92F) : zeileH - 2.0F;
        for (const auto& [b, art] : bilder) {
            const float x = f.x(static_cast<float>(b));
            if (x < f.lo.x - r || x > f.hi.x + r) { continue; }
            // die Raute so gross wie im Entwurf: in der Skala fast eine Schrifthoehe, in den Spuren knapp drei Viertel der Zeile
            modellZeitSchluessel(dl, x, ym, modellBlender() && !inSkala ? hoehe * 0.72F : hoehe, art, gew.count(b) != 0U, inSkala);
            ++t.kvRauten;
        }
        dl->PopClipRect();
    }
    if (g_zeilen.size() <= 1U && f.hi.y - spurenOben > fs) {
        dl->AddText(ImVec2{f.lo.x + fs * 0.4F, spurenOben + fs * 0.2F}, ImGui::GetColorU32(ImGuiCol_TextDisabled), tr(Str::MbKvKeineSchluessel));
    }
    dl->PushClipRect(ImVec2{f.lo.x, lo.y}, f.hi, true);
    zeitZeiger(dl, f, lo.y);
    // Rahmen
    const ImGuiIO& io = ImGui::GetIO();
    if (t.kvZiehArt == 1 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dl->AddRect(ImVec2{t.kvZugMaus[0], t.kvZugMaus[1]}, io.MousePos, IM_COL32(255, 255, 255, 200));
        dl->AddRectFilled(ImVec2{t.kvZugMaus[0], t.kvZugMaus[1]}, io.MousePos, IM_COL32(255, 255, 255, 30));
    }
    dl->PopClipRect();
    g_feld = f;
    // --- Maus -------------------------------------------------------------------------------------------------------
    const auto rauteUnter = [&](ImVec2 m, int& zeile, int& bild) {
        zeile = -1;
        for (std::size_t i = 0; i < g_zeilen.size(); ++i) {
            if (g_zeilen[i].art == 0 ? (m.y >= lo.y && m.y < zahlenY) : (m.y >= spurenOben && std::fabs(m.y - g_zeileY[i]) <= zeileH * 0.5F)) { zeile = static_cast<int>(i); }
        }
        bild = static_cast<int>(std::lround(f.bildAus(m.x)));
        if (zeile < 0 || std::fabs(f.x(static_cast<float>(bild)) - m.x) > std::max(r * 1.3F, 4.0F)) { return false; }
        for (const int b : knochenDer(g_zeilen[static_cast<std::size_t>(zeile)])) {
            if (s.hatSchluessel(b, bild)) { return true; }
        }
        return false;
    };
    if (ueber && io.MouseWheel != 0.0F) {
        if (io.KeyCtrl || io.KeyShift) {
            t.kvScrollY = std::clamp(t.kvScrollY - io.MouseWheel * teilung * 3.0F, 0.0F, std::max(inhaltH - (f.hi.y - spurenOben), 0.0F));
        } else {   // Zoom um die Maus
            const float um = f.bildAus(io.MousePos.x);
            const float k = std::pow(0.85F, io.MouseWheel);
            t.kvVon = um + (t.kvVon - um) * k;
            t.kvBis = um + (t.kvBis - um) * k;
        }
    }
    if (ueber && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const float d = io.MouseDelta.x / std::max(f.hi.x - f.lo.x, 1.0F) * (t.kvBis - t.kvVon);
        t.kvVon -= d;
        t.kvBis -= d;
        t.kvScrollY = std::clamp(t.kvScrollY - io.MouseDelta.y, 0.0F, std::max(inhaltH - (f.hi.y - spurenOben), 0.0F));
    }
    t.kvScrollY = std::clamp(t.kvScrollY, 0.0F, std::max(inhaltH - (f.hi.y - spurenOben), 0.0F));
    if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        int zeile = -1;
        int bild = 0;
        if (rauteUnter(io.MousePos, zeile, bild)) {
            const std::vector<int> ks = knochenDer(g_zeilen[static_cast<std::size_t>(zeile)]);
            bool schonGewaehlt = false;
            for (const int b : ks) { schonGewaehlt = schonGewaehlt || gewaehlt(b, bild); }
            if (!schonGewaehlt || io.KeyShift) {
                if (!io.KeyShift) { t.kvWahl.clear(); }
                for (const int b : ks) {
                    if (s.hatSchluessel(b, bild)) { waehlen(b, bild, true); }
                }
            }
            zugBeginnen(io.MousePos.x);
            t.kvZiehArt = 2;
        } else if (io.MousePos.y >= zahlenY && io.MousePos.y < spurenOben) {
            t.kvZiehArt = 4;   // unter den Schluesseln der Skala (Zahlen): Klick und Ziehen setzen das Bild
        } else {
            t.kvZiehArt = 1;
            t.kvZugMaus[0] = io.MousePos.x;
            t.kvZugMaus[1] = io.MousePos.y;
        }
    }
    if (t.kvZiehArt == 4) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            bildSetzen(static_cast<int>(std::lround(f.bildAus(io.MousePos.x))));
        } else {
            t.kvZiehArt = 0;
        }
    }
    if (t.kvZiehArt == 2) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const int d = static_cast<int>(std::lround((io.MousePos.x - t.kvZugMaus[0]) / std::max(f.hi.x - f.lo.x, 1.0F) * (t.kvBis - t.kvVon)));
            if (d != 0 || t.kvZugAngelegt) { zeitZiehen(d, 1.0F, false); }
        } else {
            if (t.kvZugAngelegt) { melden(format(Str::MbKvGeaendert, tr(Str::MbKvVerschieben), static_cast<int>(t.kvWahl.size()))); }
            t.kvZiehArt = 0;
        }
    }
    if (t.kvZiehArt == 1 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        // Rahmen: Zeilen und Frames darin
        const float x0 = std::min(t.kvZugMaus[0], io.MousePos.x);
        const float x1 = std::max(t.kvZugMaus[0], io.MousePos.x);
        const float y0 = std::min(t.kvZugMaus[1], io.MousePos.y);
        const float y1 = std::max(t.kvZugMaus[1], io.MousePos.y);
        if (!io.KeyShift) { t.kvWahl.clear(); }
        if (x1 - x0 < 3.0F && y1 - y0 < 3.0F) {
            bildSetzen(static_cast<int>(std::lround(f.bildAus(io.MousePos.x))));   // Klick ins Leere: Zeit wie Blender
        }
        for (std::size_t i = 0; i < g_zeilen.size(); ++i) {
            if (g_zeileY[i] < y0 - zeileH * 0.3F || g_zeileY[i] > y1 + zeileH * 0.3F) { continue; }
            if (g_zeilen[i].art != 0 && g_zeileY[i] < spurenOben) { continue; }   // unter die Skala gerollte Spuren
            for (const int b : knochenDer(g_zeilen[i])) {
                const std::vector<modell::RigSchluessel>* liste = schluesselDer(s, b);
                if (liste == nullptr) { continue; }
                for (const modell::RigSchluessel& k : *liste) {
                    const float x = f.x(static_cast<float>(k.bild));
                    if (x >= x0 - r && x <= x1 + r && x1 - x0 >= 3.0F) { waehlen(b, k.bild, true); }
                }
            }
        }
        t.kvZiehArt = 0;
    }
    if (ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x == 0.0F) { ImGui::OpenPopup("##kvkontext"); }
    if (ImGui::BeginPopup("##kvkontext")) {
        alleAlsUntermenues();
        ImGui::EndPopup();
    }
    if (ueber && t.kvZiehArt == 0 && !ImGui::IsPopupOpen("##kvkontext")) {
        int zeile = -1;
        int bild = 0;
        if (rauteUnter(io.MousePos, zeile, bild)) { ImGui::SetTooltip("%s", tr(Str::MbKvDsHint)); }
    }
    t.kvHover = t.kvHover || ueber;
    ImGui::EndChild();
}

// --- Kurveneditor --------------------------------------------------------------------------------------------------------

// Der Wert eines Kanals bei x so, wie die Figur ihn sieht: linear dreht mit slerp (Stufe 3), die anderen Abschnitte wie die
// F-Kurve.
float kanalAnzeige(const std::vector<modell::RigSchluessel>& v, int c, float x) {
    if (c >= 4 || v.size() < 2U || x <= static_cast<float>(v.front().bild) || x >= static_cast<float>(v.back().bild)) { return modell::kanalAuswerten(v, c, x); }
    std::size_t i = 0;
    while (i + 1 < v.size() && static_cast<float>(v[i + 1].bild) <= x) { ++i; }
    if (v[i].interp != modell::Interp::Linear) { return modell::kanalAuswerten(v, c, x); }
    BoneMatrix m;
    if (!modell::schluesselWert(v, x, m)) { return 0.0F; }
    float q[4];
    modell::quatAusMatrix(m, q);
    // dieselbe Halbkugel wie der linke Schluessel (die Kurve springt sonst)
    if (q[0] * v[i].q[0] + q[1] * v[i].q[1] + q[2] * v[i].q[2] + q[3] * v[i].q[3] < 0.0F) {
        for (float& w : q) { w = -w; }
    }
    return q[c];
}

struct KurvenStand {
    int dok = -1;
    std::string seq;
    std::vector<std::pair<int, int>> kanaele;   // (Knochen, Kanal)
    std::vector<std::vector<ImVec2>> punkte;    // je Kanal (Frame, Wert)
};
KurvenStand g_kurven;

bool kanalSichtbar(int b, int c) {
    const ModellTab& t = mt();
    const auto it = t.kvKanalAus.find(b);
    return it == t.kvKanalAus.end() || (it->second & (1U << static_cast<unsigned>(c))) == 0U;
}

void kurvenAbtasten() {
    ModellTab& t = mt();
    std::vector<std::pair<int, int>> kan;
    for (const int b : gezeigteKnochen()) {
        for (int c = 0; c < modell::kKanaele; ++c) {
            if (kanalSichtbar(b, c)) { kan.emplace_back(b, c); }
        }
    }
    if (g_kurven.dok == t.dok.stand() && g_kurven.seq == sequenzName() && g_kurven.kanaele == kan) { return; }
    g_kurven.dok = t.dok.stand();
    g_kurven.seq = sequenzName();
    g_kurven.kanaele = kan;
    g_kurven.punkte.assign(kan.size(), {});
    const modell::RigSchicht& s = schicht();
    for (std::size_t i = 0; i < kan.size(); ++i) {
        const auto& [b, c] = kan[i];
        if (static_cast<std::size_t>(b) >= s.knochen.size() || s.knochen[static_cast<std::size_t>(b)].empty()) { continue; }
        const auto& v = s.knochen[static_cast<std::size_t>(b)];
        // vier Proben je Frame zwischen erstem und letztem Schluessel (ausserhalb waagerecht)
        const int von = v.front().bild;
        const int bis = v.back().bild;
        for (int q = von * 4; q <= bis * 4; ++q) {
            const float x = static_cast<float>(q) * 0.25F;
            g_kurven.punkte[i].emplace_back(x, kanalAnzeige(v, c, x));
        }
    }
}

void drawKurven(const ImVec2 groesse) {
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    const float linksB = modellZeitNamenBreite();   // AP1: Namensspalte und Skala der Zeitleiste
    const float linealH = modellZeitSkalaHoehe();
    const modell::RigSchicht& s = schicht();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    kurvenAbtasten();
    if (t.kvEinpassen) {
        t.kvVon = static_cast<float>(erstes) - 1.0F;
        t.kvBis = static_cast<float>(erstes + anzahl);
        float lo = 1e9F;
        float hi = -1e9F;
        for (const auto& p : g_kurven.punkte) {
            for (const ImVec2& q : p) {
                lo = std::min(lo, q.y);
                hi = std::max(hi, q.y);
            }
        }
        if (lo > hi) {
            lo = -1.0F;
            hi = 1.0F;
        }
        const float rand = std::max((hi - lo) * 0.08F, 0.05F);
        t.kvYVon = lo - rand;
        t.kvYBis = hi + rand;
        t.kvEinpassen = false;
    }
    ImGui::BeginChild("##kezeilen", ImVec2{groesse.x, groesse.y}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // links: neben der Skala der Name des Editors, darunter die Knochen (Klick klappt die Kanaele auf), Kanaele mit Auge
    modellZeitNamenText(dl, lo, t.kvTitel.c_str());
    const float kanaeleOben = linealH + ImGui::GetStyle().ItemSpacing.y;
    ImGui::SetCursorScreenPos(ImVec2{lo.x, lo.y + kanaeleOben});
    ImGui::BeginChild("##kekanaele", ImVec2{linksB - fs * 0.3F, std::max(groesse.y - kanaeleOben, 1.0F)}, ImGuiChildFlags_Borders);
    for (const int b : gezeigteKnochen()) {
        ImGui::PushID(b);
        const bool auf = t.kvKnochenAuf.count(b) != 0U;
        if (ImGui::Selectable((std::string(auf ? "- " : "+ ") + g_app->anim.bones[static_cast<std::size_t>(b)].name).c_str(), !auswahlKnochen().empty() && auswahlKnochen().front() == b)) {
            if (auf) {
                t.kvKnochenAuf.erase(b);
            } else {
                t.kvKnochenAuf.insert(b);
            }
        }
        if (auf) {
            for (int c = 0; c < modell::kKanaele; ++c) {
                ImGui::PushID(c);
                bool an = kanalSichtbar(b, c);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::ColorConvertU32ToFloat4(kanalFarbe(c)));
                if (ImGui::Checkbox(tr(kanalText(c)), &an)) {
                    unsigned& m = t.kvKanalAus[b];
                    m = an ? (m & ~(1U << static_cast<unsigned>(c))) : (m | (1U << static_cast<unsigned>(c)));
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    Feld f;
    f.lo = ImVec2{lo.x + linksB, lo.y + linealH};
    f.hi = ImVec2{lo.x + groesse.x, lo.y + groesse.y};
    f.von = t.kvVon;
    f.bis = t.kvBis;
    f.yVon = t.kvYVon;
    f.yBis = t.kvYBis;
    ImGui::SetCursorScreenPos(f.lo);
    ImGui::InvisibleButton("##kefeld", ImVec2{std::max(f.hi.x - f.lo.x, 1.0F), std::max(f.hi.y - f.lo.y, 1.0F)}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool ueber = ImGui::IsItemHovered();
    dl->PushClipRect(ImVec2{f.lo.x, lo.y}, f.hi, true);
    dl->AddRectFilled(f.lo, f.hi, ImGui::GetColorU32(ImGuiCol_ChildBg));
    zeitLineal(dl, f, lo.y, linealH);
    // Null-Linie und Werteraster: Schritt 1/2/5 * 10^n, so dass die Zahlen mindestens 1,5 Zeilen auseinander stehen
    {
        const float proWert = (f.hi.y - f.lo.y) / std::max(f.yBis - f.yVon, 1e-6F);
        float schritt = 1e-4F;
        for (int n = 0; n < 40 && schritt * proWert < fs * 1.5F; ++n) { schritt *= (n % 3 == 1) ? 2.5F : 2.0F; }   // 1, 2, 5, 10, 20, 50 ...
        const float erster = std::ceil(f.yVon / schritt) * schritt;
        for (int k = 0; k < 64; ++k) {
            const float w = erster + static_cast<float>(k) * schritt;
            if (w > f.yBis) { break; }
            const float yy = f.y(w);
            if (yy < f.lo.y || yy > f.hi.y) { continue; }
            const bool null = std::fabs(w) < schritt * 0.5F;
            dl->AddLine(ImVec2{f.lo.x, yy}, ImVec2{f.hi.x, yy}, ImGui::GetColorU32(ImGuiCol_Separator, null ? 0.8F : 0.25F));
            char z[24];
            std::snprintf(z, sizeof z, "%g", static_cast<double>(null ? 0.0F : w));
            dl->AddText(ImVec2{f.lo.x + 2.0F, yy - fs}, ImGui::GetColorU32(ImGuiCol_TextDisabled), z);
        }
    }
    // Kurven
    t.kvKurvenPunkte = 0;
    for (std::size_t i = 0; i < g_kurven.kanaele.size(); ++i) {
        const auto& [b, c] = g_kurven.kanaele[i];
        const auto& p = g_kurven.punkte[i];
        if (p.empty()) { continue; }
        const bool aktiv = !auswahlKnochen().empty() && auswahlKnochen().front() == b;
        const ImU32 farbe = kanalFarbe(c, aktiv ? 1.0F : 0.55F);
        // ausserhalb waagerecht
        dl->AddLine(ImVec2{f.lo.x, f.y(p.front().y)}, ImVec2{f.x(p.front().x), f.y(p.front().y)}, farbe, 1.0F);
        dl->AddLine(ImVec2{f.x(p.back().x), f.y(p.back().y)}, ImVec2{f.hi.x, f.y(p.back().y)}, farbe, 1.0F);
        ImVec2 vor{f.x(p.front().x), f.y(p.front().y)};
        for (std::size_t q = 1; q < p.size(); ++q) {
            const ImVec2 n{f.x(p[q].x), f.y(p[q].y)};
            if ((n.x >= f.lo.x || vor.x >= f.lo.x) && (vor.x <= f.hi.x)) { dl->AddLine(vor, n, farbe, aktiv ? 2.0F : 1.3F); }
            vor = n;
        }
        t.kvKurvenPunkte += static_cast<int>(p.size());
    }
    // Schluessel und Henkel
    const float r = fs * 0.28F;
    for (const auto& [b, c] : g_kurven.kanaele) {
        if (static_cast<std::size_t>(b) >= s.knochen.size()) { continue; }
        const auto& v = s.knochen[static_cast<std::size_t>(b)];
        for (std::size_t i = 0; i < v.size(); ++i) {
            const modell::RigSchluessel& k = v[i];
            const ImVec2 pk{f.x(static_cast<float>(k.bild)), f.y(modell::kanalWert(k, c))};
            const bool g = gewaehlt(b, k.bild);
            if (g && t.kvHenkelZeigen && (k.interp == modell::Interp::Bezier || (i > 0 && v[i - 1].interp == modell::Interp::Bezier))) {
                const ImVec2 hl{f.x(static_cast<float>(k.bild) + k.hl[c][0]), f.y(modell::kanalWert(k, c) + k.hl[c][1])};
                const ImVec2 hr{f.x(static_cast<float>(k.bild) + k.hr[c][0]), f.y(modell::kanalWert(k, c) + k.hr[c][1])};
                dl->AddLine(hl, pk, IM_COL32(200, 200, 200, 200), 1.0F);
                dl->AddLine(pk, hr, IM_COL32(200, 200, 200, 200), 1.0F);
                dl->AddCircleFilled(hl, r * 0.8F, IM_COL32(230, 230, 230, 255));
                dl->AddCircleFilled(hr, r * 0.8F, IM_COL32(230, 230, 230, 255));
            }
            dl->AddRectFilled(ImVec2{pk.x - r, pk.y - r}, ImVec2{pk.x + r, pk.y + r}, g ? IM_COL32(255, 175, 60, 255) : IM_COL32(20, 20, 20, 255));
            dl->AddRect(ImVec2{pk.x - r, pk.y - r}, ImVec2{pk.x + r, pk.y + r}, kanalFarbe(c), 0.0F, 0, 1.0F);
        }
    }
    zeitZeiger(dl, f, lo.y);
    const ImGuiIO& io = ImGui::GetIO();
    if (t.kvZiehArt == 1 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dl->AddRect(ImVec2{t.kvZugMaus[0], t.kvZugMaus[1]}, io.MousePos, IM_COL32(255, 255, 255, 200));
    }
    if (g_kurven.kanaele.empty()) {
        dl->AddText(ImVec2{f.lo.x + fs, f.lo.y + fs}, ImGui::GetColorU32(ImGuiCol_TextDisabled), tr(Str::MbKvKeineSchluessel));
    }
    dl->PopClipRect();
    g_feld = f;
    // --- Maus -------------------------------------------------------------------------------------------------------
    // Treffer: Henkel eines gewaehlten Schluessels zuerst, dann Schluessel
    struct Treffer {
        int knochen = -1;
        int index = -1;
        int kanal = -1;
        int henkel = 0;   // 0 Schluessel, 1 links, 2 rechts
    };
    const auto treffer = [&](ImVec2 m) {
        Treffer best;
        float bd = r * 2.2F;
        for (const auto& [b, c] : g_kurven.kanaele) {
            if (static_cast<std::size_t>(b) >= s.knochen.size()) { continue; }
            const auto& v = s.knochen[static_cast<std::size_t>(b)];
            for (std::size_t i = 0; i < v.size(); ++i) {
                const modell::RigSchluessel& k = v[i];
                const float x = static_cast<float>(k.bild);
                const float w = modell::kanalWert(k, c);
                const auto pruefe = [&](float px, float py, int h) {
                    const float d = std::hypot(m.x - px, m.y - py);
                    if (d < bd) {
                        bd = d;
                        best = Treffer{b, static_cast<int>(i), c, h};
                    }
                };
                if (gewaehlt(b, k.bild) && t.kvHenkelZeigen) {
                    pruefe(f.x(x + k.hl[c][0]), f.y(w + k.hl[c][1]), 1);
                    pruefe(f.x(x + k.hr[c][0]), f.y(w + k.hr[c][1]), 2);
                }
                pruefe(f.x(x), f.y(w), 0);
            }
        }
        return best;
    };
    if (ueber && io.MouseWheel != 0.0F) {
        const float k = std::pow(0.85F, io.MouseWheel);
        if (io.KeyCtrl) {   // Werte zoomen
            const float um = f.wertAus(io.MousePos.y);
            t.kvYVon = um + (t.kvYVon - um) * k;
            t.kvYBis = um + (t.kvYBis - um) * k;
        } else {
            const float um = f.bildAus(io.MousePos.x);
            t.kvVon = um + (t.kvVon - um) * k;
            t.kvBis = um + (t.kvBis - um) * k;
        }
    }
    if (ueber && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const float dx = io.MouseDelta.x / std::max(f.hi.x - f.lo.x, 1.0F) * (t.kvBis - t.kvVon);
        const float dy = io.MouseDelta.y / std::max(f.hi.y - f.lo.y, 1.0F) * (t.kvYBis - t.kvYVon);
        t.kvVon -= dx;
        t.kvBis -= dx;
        t.kvYVon += dy;
        t.kvYBis += dy;
    }
    if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const Treffer h = treffer(io.MousePos);
        diag::detail("Kurveneditor: Druck bei " + std::to_string(static_cast<int>(io.MousePos.x)) + "/" + std::to_string(static_cast<int>(io.MousePos.y)) +
                     " - Knochen " + std::to_string(h.knochen) + ", Schluessel " + std::to_string(h.index) + ", Kanal " + std::to_string(h.kanal) + ", Henkel " +
                     std::to_string(h.henkel) + ", gewaehlt " + std::to_string(t.kvWahl.size()));
        if (h.knochen >= 0) {
            const int bild = s.knochen[static_cast<std::size_t>(h.knochen)][static_cast<std::size_t>(h.index)].bild;
            if (h.henkel == 0 && (!gewaehlt(h.knochen, bild) || io.KeyShift)) { waehlen(h.knochen, bild, io.KeyShift); }
            zugBeginnen(io.MousePos.x);
            t.kvZugKnochen = h.knochen;
            t.kvZugIndex = h.index;
            t.kvZugKanal = h.kanal;
            t.kvZugHenkel = h.henkel;
            t.kvZiehArt = h.henkel == 0 ? 3 : 5;
        } else {
            t.kvZiehArt = 1;
            t.kvZugMaus[0] = io.MousePos.x;
            t.kvZugMaus[1] = io.MousePos.y;
        }
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.MousePos.x >= f.lo.x && io.MousePos.x <= f.hi.x && io.MousePos.y >= lo.y && io.MousePos.y < f.lo.y &&
        ImGui::IsWindowHovered()) {
        t.kvZiehArt = 4;
    }
    if (t.kvZiehArt == 4) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            bildSetzen(static_cast<int>(std::lround(f.bildAus(io.MousePos.x))));
        } else {
            t.kvZiehArt = 0;
        }
    }
    if ((t.kvZiehArt == 3 || t.kvZiehArt == 5) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float dxBild = (io.MousePos.x - t.kvZugMaus[0]) / std::max(f.hi.x - f.lo.x, 1.0F) * (t.kvBis - t.kvVon);
        const float dyWert = -(io.MousePos.y - t.kvZugMaus[1]) / std::max(f.hi.y - f.lo.y, 1.0F) * (t.kvYBis - t.kvYVon);
        if (std::fabs(io.MousePos.x - t.kvZugMaus[0]) + std::fabs(io.MousePos.y - t.kvZugMaus[1]) > 2.0F || t.kvZugAngelegt) {
            modell::RigSchicht neu = t.kvZugStart;
            modell::Wahl w = t.kvZugWahl;
            const int c = t.kvZugKanal;
            if (t.kvZiehArt == 3) {
                // Wert: der gezogene Kanal aller gewaehlten Schluessel; Zeit: alle gewaehlten (ganze Frames)
                for (const modell::SchluesselWahl& x : w) {
                    std::vector<modell::RigSchluessel>* v = schluesselDer(neu, x.knochen);   // pfix H6
                    if (v == nullptr) { continue; }
                    for (modell::RigSchluessel& k : *v) {
                        if (k.bild == x.bild) { modell::kanalSetzen(k, c, modell::kanalWert(k, c) + dyWert); }
                    }
                }
                w = modell::schluesselVerschieben(neu, w, static_cast<int>(std::lround(dxBild)));
                modell::henkelNeu(neu);
            } else {
                // Henkel: Auto-Arten werden Ausgerichtet, Vektor wird Frei (Blender: transform an einem Henkel)
                auto& v = neu.knochen[static_cast<std::size_t>(t.kvZugKnochen)];
                modell::RigSchluessel& k = v[static_cast<std::size_t>(t.kvZugIndex)];
                const bool links = t.kvZugHenkel == 1;
                modell::Henkel& art = links ? k.henkelL[c] : k.henkelR[c];
                modell::Henkel& gegen = links ? k.henkelR[c] : k.henkelL[c];
                if (art == modell::Henkel::Auto || art == modell::Henkel::AutoGeklemmt) {
                    art = modell::Henkel::Ausgerichtet;
                    if (gegen == modell::Henkel::Auto || gegen == modell::Henkel::AutoGeklemmt) { gegen = modell::Henkel::Ausgerichtet; }
                }
                if (art == modell::Henkel::Vektor) { art = modell::Henkel::Frei; }
                float* h = links ? k.hl[c] : k.hr[c];
                h[0] = links ? std::min(h[0] + dxBild, -0.001F) : std::max(h[0] + dxBild, 0.001F);
                h[1] += dyWert;
                if (k.interp != modell::Interp::Bezier) { k.interp = modell::Interp::Bezier; }
                modell::henkelNeuGezogen(v, true, c, t.kvZugIndex, links);
            }
            const std::string name = sequenzName();
            if (!t.kvZugAngelegt) {
                t.dok.aendern(tr(t.kvZiehArt == 3 ? Str::MbKvVerschieben : Str::MbKvGruppeTangenten), [&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
                t.kvZugAngelegt = true;
            } else {
                t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
            }
            t.kvWahl = w;
            ++t.kvAenderungen;
            g_app->modelDirty = true;
        }
    } else if (t.kvZiehArt == 3 || t.kvZiehArt == 5) {
        t.kvZiehArt = 0;
    }
    if (t.kvZiehArt == 1 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float x0 = std::min(t.kvZugMaus[0], io.MousePos.x);
        const float x1 = std::max(t.kvZugMaus[0], io.MousePos.x);
        const float y0 = std::min(t.kvZugMaus[1], io.MousePos.y);
        const float y1 = std::max(t.kvZugMaus[1], io.MousePos.y);
        if (!io.KeyShift) { t.kvWahl.clear(); }
        if (x1 - x0 < 3.0F && y1 - y0 < 3.0F) { bildSetzen(static_cast<int>(std::lround(f.bildAus(io.MousePos.x)))); }
        for (const auto& [b, c] : g_kurven.kanaele) {
            if (static_cast<std::size_t>(b) >= s.knochen.size()) { continue; }
            for (const modell::RigSchluessel& k : s.knochen[static_cast<std::size_t>(b)]) {
                const float x = f.x(static_cast<float>(k.bild));
                const float yy = f.y(modell::kanalWert(k, c));
                if (x >= x0 && x <= x1 && yy >= y0 && yy <= y1 && x1 - x0 >= 3.0F) { waehlen(b, k.bild, true); }
            }
        }
        t.kvZiehArt = 0;
    }
    if (ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x == 0.0F) { ImGui::OpenPopup("##kvkontext"); }
    if (ImGui::BeginPopup("##kvkontext")) {
        alleAlsUntermenues();
        ImGui::EndPopup();
    }
    if (ueber && t.kvZiehArt == 0 && treffer(io.MousePos).knochen >= 0 && !ImGui::IsPopupOpen("##kvkontext")) { ImGui::SetTooltip("%s", tr(Str::MbKvKeHint)); }
    t.kvHover = t.kvHover || ueber;
    ImGui::EndChild();
}

// G / S wie Blender: Schluessel mit der Maus verschieben oder skalieren, bis Klick/Enter oder Esc/rechts.
void modalSchritt() {
    ModellTab& t = mt();
    if (t.kvModal == 0) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    if (!t.kvModalLaeuft) {
        zugBeginnen(io.MousePos.x);
        t.kvModalLaeuft = true;
    }
    const float breite = std::max(g_feld.hi.x - g_feld.lo.x, 1.0F);
    const float dx = (io.MousePos.x - t.kvZugMaus[0]) / breite * (t.kvBis - t.kvVon);
    if (t.kvModal == 1) {
        zeitZiehen(static_cast<int>(std::lround(dx)), 1.0F, false);
    } else {
        zeitZiehen(0, std::max(1.0F + (io.MousePos.x - t.kvZugMaus[0]) / (breite * 0.25F), 0.0F), true);
    }
    const bool fertig = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool abbruch = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    if (fertig || abbruch) {
        if (abbruch && t.kvZugAngelegt) {
            (void)t.dok.abbrechen(t.kvZugSchritt);
            t.kvWahl = t.kvZugWahl;
        }
        t.kvModal = 0;
        t.kvModalLaeuft = false;
        t.kvModalEndeBild = ImGui::GetFrameCount();
    }
}

}  // namespace

// --- oeffentlich -------------------------------------------------------------------------------------------------------

int modellKurvenAktionZahl() { return static_cast<int>(kvAktionen().size()); }
const char* modellKurvenAktionText(int i) { return tr(aktionName(kvAktionen()[static_cast<std::size_t>(i)])); }
const char* modellKurvenAktionHinweis(int i) { return tr(kvAktionen()[static_cast<std::size_t>(i)].hinweis); }
const char* modellKurvenAktionTaste(int i, bool blender) {
    const KvAktion& a = kvAktionen()[static_cast<std::size_t>(i)];
    return blender ? a.blenderTaste : a.maxTaste;
}
const char* modellKurvenAktionQuelle(int i) { return kvAktionen()[static_cast<std::size_t>(i)].quelle; }
ImGuiKeyChord modellKurvenAktionChord(int i, bool blender) {
    const KvAktion& a = kvAktionen()[static_cast<std::size_t>(i)];
    return blender ? a.blenderChord : a.maxChord;
}
int modellKurvenAktionGruppe(int i) { return kvAktionen()[static_cast<std::size_t>(i)].gruppe; }
const char* modellKurvenGruppeText(int g) { return tr(kKvGruppenText[std::clamp(g, 0, kKvGruppen - 1)]); }
void modellKurvenAktionAusloesen(int i) { ausloesen(kvAktionen()[static_cast<std::size_t>(i)]); }
std::string modellKurvenAktionSuchText(int i) { return aktionText(kvAktionen()[static_cast<std::size_t>(i)]); }
int modellKurvenAktionNach(int text) {
    const auto& l = kvAktionen();
    for (std::size_t i = 0; i < l.size(); ++i) {
        if (static_cast<int>(l[i].text) == text) { return static_cast<int>(i); }
    }
    return -1;
}

void modellKurvenMenue() {
    if (ImGui::BeginMenu(tr(Str::MbKvMenue))) {
        alleAlsUntermenues();
        ImGui::EndMenu();
    }
}

void modellKurvenKontextMenue() { modellKurvenMenue(); }

bool modellKurvenAktionenSuche(const std::string& such) {
    if (modellZeitAktionenSuche(such)) { return true; }   // AP1: die Aktionen der Zeitleiste (Auto Key, Spuren, Zeitkonfiguration ...)
    for (const KvAktion& a : kvAktionen()) {
        const std::string text = aktionText(a);
        std::string k = text;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (!such.empty() && k.find(such) == std::string::npos) { continue; }
        if (ImGui::Selectable(text.c_str())) {
            ausloesen(a);
            return true;
        }
        ImGui::SetItemTooltip("%s", tooltipText(a).c_str());
    }
    return false;
}

bool modellKurvenTasten() {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation) { return false; }
    if (t.kvModal != 0) { return true; }   // G/S laufen (modalSchritt im Editor)
    const bool blender = modellBlender();
    for (const KvAktion& a : kvAktionen()) {
        const ImGuiKeyChord c = blender ? a.blenderChord : a.maxChord;
        if (c == 0 || (a.imEditor && !t.kvHoverVorher)) { continue; }
        if (ImGui::IsKeyChordPressed(c)) {
            ausloesen(a);
            return true;
        }
    }
    return false;
}

bool modellKurvenTastenImEditor() { return mt().kvHoverVorher; }

void drawModellAnimEditor() {
    // der Regler der Werkzeuge (Popup); der Editor selbst ist der Spurbereich der Zeitleiste (drawModellSpurbereich)
    ModellTab& t = mt();
    t.kvHoverVorher = t.kvHover;
    t.kvHover = false;
    drawRegler();
}

// AP1: der Spurbereich der Zeitleiste (Aufruf in drawModellZeitleiste, Bereich Animation): die Skala mit den Schluesseln
// der Auswahl, darunter - solange die Spuren offen sind - das Dopesheet oder der Kurveneditor. Bis rc611 ein eigener
// Kasten mit Menueleiste zwischen Ansichten und Zeitleiste, ab Werk zu; Menue und Filter stehen jetzt im Menue der
// Zeitleiste und im Rechtsklick.
void drawModellSpurbereich(ImVec2 groesse) {
    ModellTab& t = mt();
    const bool blender = modellBlender();
    const bool kurven = t.kvOffen && t.kvArt == 1;
    const Str titel = !kurven ? (blender ? Str::MbKvDsBl : Str::MbKvDsMax) : (blender ? Str::MbKvKeBl : Str::MbKvKeMax);
    t.kvTitel = tr(titel);
    const auto ab = std::chrono::steady_clock::now();
    if (groesse.x > ImGui::GetFontSize() * 4.0F) {
        if (kurven) {
            drawKurven(groesse);
        } else {
            drawDopesheet(groesse);
        }
    }
    modalSchritt();
    t.kvZeitMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab).count();
    if (t.kvOffen) { t.kvGezeichnet = ImGui::GetFrameCount(); }
}

// AP1: die Bilder mit Schluesseln der gezeigten Knochen und IK-Spuren in der Sequenz, aufsteigend (Sprung zum vorigen /
// naechsten Schluessel: wie Blender Pfeil runter / hoch, wie 3ds Max der Key-Modus)
std::vector<int> modellKvSchluesselBilder() {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    std::set<int> bilder;
    const modell::RigSchicht& s = schicht();
    const auto dazu = [&](const std::vector<modell::RigSchluessel>& liste) {
        for (const modell::RigSchluessel& k : liste) {
            if (k.bild >= erstes && k.bild < erstes + anzahl) { bilder.insert(k.bild); }
        }
    };
    for (const int b : gezeigteKnochen()) {
        if (static_cast<std::size_t>(b) < s.knochen.size()) { dazu(s.knochen[static_cast<std::size_t>(b)]); }
    }
    for (const modell::IkSpur& sp : s.ik) { dazu(sp.ziel); }
    return {bilder.begin(), bilder.end()};
}

// --- Befehlsfeld (AP10: ueber den Baustein gui/ui_rollout.h) ---------------------------------------------------------------

bool modellAnimReduzieren() { return reduzieren(); }

bool modellAnimSpiegeln() { return animationSpiegeln(); }

// Entwurf, Gruppe "Rig erstellen": "Auf Rig legen" - die Bilder der Sequenz werden Schluessel an den Griffen (rigReduzieren ueber
// die ganze Sequenz, alle Knochen), und die IK-Ziele der Glieder des Rigs folgen an jedem Bild der Bewegung (ikAufFkBereich mit
// Anteil 0: das Ziel liegt auf der Hand bzw. dem Fuss, die Pose bleibt FK und damit gleich). Danach laesst sich jeder Griff
// bearbeiten; wer eine Hand oder einen Fuss greift, wechselt ohne Sprung auf IK. Ein Rueckgaengig-Schritt.
bool modellAnimAufRigLegen() {
    ModellTab& t = mt();
    const AnimEntry* e = sequenz();
    if (g_app->anim.empty() || e == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return false;
    }
    if (!modellRigDa() && !modellRigAnlegen()) { return false; }
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    modell::RigSchicht neu;
    modell::ReduzierBericht br;
    if (!modell::rigReduzieren(g_app->anim, schicht(), erstes, anzahl, neu, &br, nullptr)) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehler).c_str()));
        return false;
    }
    int glieder = 0;
    if (!modellAnim().nurFk) {
        for (int art = 0; art < modell::kIkArten; ++art) {
            const int spur = modellIkSpur(art, neu);
            if (spur < 0) { continue; }
            modell::ikAufFkBereich(g_app->anim, neu, spur, erstes, erstes + anzahl - 1, 0.0F);
            ++glieder;
        }
    }
    const std::string name = e->name;
    t.dok.aendern(tr(Str::AzAufRig), [&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
    t.kvBericht = br;
    t.kvWahl.clear();
    ++t.kvAenderungen;
    g_app->modelDirty = true;
    melden(format(Str::AzAufRigMeldung, name.c_str(), static_cast<int>(br.nachher), static_cast<int>(br.vorher), glieder));
    return true;
}

// Entwurf A4: "Vorhandene Animation bearbeiten" - woher die Bilder kommen (Sequenz, GLA, Bereich in GLA-Frames), "In Schluessel
// umwandeln" (Frames zu Schluesseln reduzieren) und "Zurueck in die GLA backen" im Raster; die Optionen des Umwandelns (nur
// gewaehlte Knochen, nur eine Spanne) eingeklappt. Bis rc611 standen die beiden Knoepfe in den Gruppen "Kurven und
// Animationswerkzeuge" und "Backen".
void modellAnimGruppeBearbeiten() {
    ModellTab& t = mt();
    const AnimEntry* e = sequenz();
    if (!ui::rolloutBeginn(tr(Str::AzRBearbeiten))) { return; }
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const std::string gla = fileName(g_app->anim.skeletonName) + ".gla";
    char bilder[64] = "-";
    if (e != nullptr) { std::snprintf(bilder, sizeof bilder, "%d \xe2\x80\x93 %d (%d)", e->firstFrame, e->firstFrame + std::max(e->numFrames, 1) - 1, std::max(e->numFrames, 1)); }
    ui::wertTabelle({{tr(Str::AzQuelle), e != nullptr ? e->name + " \xc2\xb7 " + gla : gla, ui::Ton::Normal, g_app->anim.skeletonName + ".gla"},
                     {tr(Str::AzBilder), bilder, e != nullptr ? ui::Ton::Normal : ui::Ton::Leise, {}},
                     {tr(Str::AzSchluessel), std::to_string(s != nullptr ? s->schluesselZahl() : 0), ui::Ton::Normal, {}}});
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbKvReduzieren), {.icon = UiIcon::Ruhig, .hinweis = tr(Str::MbKvReduzierenHint), .aktiv = e != nullptr})) { (void)reduzieren(); }
    if (ui::werkzeugKnopf(tr(Str::MbRgBacken), {.icon = UiIcon::Erzeugen, .hinweis = tr(Str::MbRgBackenHint), .aktiv = e != nullptr})) { (void)modellRigBacken(); }
    ui::rasterEnde();
    if (ui::mehrBeginn(tr(Str::AzOptionen))) {
        // Stufe 4: nur gewaehlte Knochen / eine Spanne
        (void)ui::schalter(tr(Str::MbIkRedNurGewaehlte), &t.kvRedNurGewaehlte, tr(Str::MbIkRedNurGewaehlteHint));
        if (ui::schalter(tr(Str::MbIkRedSpanne), &t.kvRedSpanne, tr(Str::MbIkRedSpanneHint)) && t.kvRedSpanne) { spanneAusWahl(); }
        if (t.kvRedSpanne) {
            ImGui::SetNextItemWidth(-1.0F);
            // MRESTE: die Spanne in der Zaehlung der Zeitleiste (wie Max ab 0 in der Sequenz, wie Blender ab 1); intern GLA-Frames
            int erstes = 0;
            int anzahl = 0;
            bereich(erstes, anzahl);
            const int versatz = (modellBlender() ? 1 : 0) - erstes;
            int von = t.kvRedVon + versatz;
            int bis = t.kvRedBis + versatz;
            if (ImGui::DragIntRange2("##redspanne", &von, &bis, 1.0F, versatz, std::max(g_app->anim.bildZahl() - 1, 0) + versatz, tr(Str::MbIkRedVon), tr(Str::MbIkRedBis))) {
                t.kvRedVon = von - versatz;
                t.kvRedBis = bis - versatz;
            }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkRedSpanneHint));
        }
        ui::absatz(tr(Str::MbRgBackenInfo));
        ui::mehrEnde();
    }
    ui::tipp(tr(Str::AzBearbTipp));
    ui::rolloutEnde();
}

// Kurven und Animationswerkzeuge: Animation spiegeln, Anim Offset. Dopesheet und Kurveneditor oeffnet die Zeitleiste (AP1: Knopf
// "Spuren" und die Umschaltung daneben; dazu Menue Animation und die Suche) - hier steht kein zweiter Knopf dafuer.
void modellAnimGruppeKurven() {
    ModellTab& t = mt();
    if (!ui::rolloutBeginn(tr(Str::MbKvRollout), {.zu = true, .reiter = modellReiter(MbReiter::Bewegung, MbReiterBl::Werkzeug)})) { return; }
    if (ui::werkzeugKnopf(tr(Str::MbKvAnimSpiegeln), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MbKvAnimSpiegelnHint), .aktiv = sequenz() != nullptr})) {
        (void)animationSpiegeln();
    }
    if (ui::schalter(tr(Str::MbKvAnimOffset), &t.kvAnimOffset, tr(Str::MbKvAnimOffsetHint))) { g_app->modelDirty = true; }
    ui::rolloutEnde();
}

void modellAnimGruppePosenBibliothek() {
    ModellTab& t = mt();
    const auto& posen = t.dok.inhalt().rig.posen;
    char info[16];
    std::snprintf(info, sizeof info, "%d", static_cast<int>(posen.size()));
    if (!ui::rolloutBeginn(tr(Str::MbKvGruppeBibliothek), {.kurzinfo = posen.empty() ? nullptr : info, .zu = true, .reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Daten)})) {
        return;
    }
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##kvposename", tr(Str::MbKvPoseName), t.kvPoseName, sizeof t.kvPoseName);
    if (ui::werkzeugKnopf(tr(Str::MbKvPoseSpeichern), {.icon = UiIcon::Speichern, .hinweis = tr(Str::MbKvPoseSpeichernHint)})) { (void)poseSpeichern(); }
    if (posen.empty()) { ui::absatz(tr(Str::MbKvPoseLeer)); }
    for (std::size_t i = 0; i < posen.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(posen[i].name.c_str(), t.kvPoseWahl == static_cast<int>(i))) { t.kvPoseWahl = static_cast<int>(i); }
        ImGui::SetItemTooltip("%s", tr(Str::MbKvPoseAnwendenHint));
        ImGui::PopID();
    }
    const bool gewaehlt = t.kvPoseWahl >= 0;
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbKvPoseAnwenden), {.icon = UiIcon::Abspielen, .hinweis = tr(Str::MbKvPoseAnwendenHint), .aktiv = gewaehlt})) { (void)poseAnwenden(false); }
    if (ui::werkzeugKnopf(tr(Str::MbKvPoseGespiegelt), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MbKvPoseAnwendenHint), .aktiv = gewaehlt})) { (void)poseAnwenden(true); }
    if (ui::werkzeugKnopf(tr(Str::MbKvPoseLoeschen), {.icon = UiIcon::Loeschen, .aktiv = gewaehlt})) { poseLoeschen(); }
    ui::rasterEnde();
    // Stufe 4: die Bibliothek als Datei (Dokumente\behaved\posen)
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##posendatei", tr(Str::MbIkPosenDatei), t.posenDatei, sizeof t.posenDatei);
    ImGui::SetItemTooltip("%s", tr(Str::MbIkPosenSpeichernHint));
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbIkPosenSpeichern), {.icon = UiIcon::SpeichernUnter, .hinweis = tr(Str::MbIkPosenSpeichernHint), .aktiv = !posen.empty()})) {
        (void)modellPosenSpeichern();
    }
    if (ui::werkzeugKnopf(tr(Str::MbIkPosenLaden), {.icon = UiIcon::Oeffnen, .hinweis = tr(Str::MbIkPosenLadenHint)})) { ImGui::OpenPopup("##posenladen"); }
    ui::rasterEnde();
    if (ImGui::BeginPopup("##posenladen")) {
        posenLadenMenue();
        ImGui::EndPopup();
    }
    ui::rolloutEnde();
}

void drawModellKurvenBereich() {
    modellAnimGruppeKurven();
    modellAnimGruppePosenBibliothek();
}

// Anim Offset (Griff gezogen): die Aenderung am Zugbild auf die anderen Schluessel der betroffenen Knochen.
void modellKurvenAnimOffset(const modell::RigSchicht& vorher, modell::RigSchicht& neu, const std::vector<int>& knochen, int bild) {
    ModellTab& t = mt();
    if (!t.kvAnimOffset) { return; }
    for (const int k : knochen) {
        if (k < 0 || static_cast<std::size_t>(k) >= neu.knochen.size()) { continue; }
        modell::RigSchluessel alt;
        alt.bild = bild;
        BoneMatrix m = BoneMatrix::identity();
        if (modell::rigWert(vorher, k, static_cast<float>(bild), m)) {
            modell::quatAusMatrix(m, alt.q);
            for (int a = 0; a < 3; ++a) { alt.t[a] = m.m[a][3]; }
        }
        const auto& v = neu.knochen[static_cast<std::size_t>(k)];
        const auto it = std::find_if(v.begin(), v.end(), [bild](const modell::RigSchluessel& x) { return x.bild == bild; });
        if (it == v.end()) { continue; }
        const modell::RigSchluessel jetzt = *it;
        std::vector<int> nur;
        for (const modell::SchluesselWahl& w : t.kvWahl) {
            if (w.knochen == k) { nur.push_back(w.bild); }
        }
        (void)modell::animOffset(neu, k, alt, jetzt, nur);
    }
}

// --- Bewegungspfade ziehen -------------------------------------------------------------------------------------------------
namespace {

// Ein Pfad, der sich ziehen laesst: Becken, Wurzel, Bewegung (Verschiebung); Stufe 4: Hand, Fuss, Saebelspitze und die
// Saebel-Tags (das IK-Ziel des Glieds an diesem Bild)
int pfadGlied(const ModellPfad& p) {
    const ModellTab& t = mt();
    if (!modellRigDa()) { return -1; }
    int k = p.knochen;
    if (p.tag >= 0) {
        const modell::Inhalt& in = t.dok.inhalt();
        if (static_cast<std::size_t>(p.tag) >= in.teile.size() || in.teile[static_cast<std::size_t>(p.tag)].stufen.empty() ||
            in.teile[static_cast<std::size_t>(p.tag)].stufen[0].ecken.empty()) {
            return -1;
        }
        k = in.teile[static_cast<std::size_t>(p.tag)].stufen[0].ecken[0].knochen[0];   // der Knochen des Tags (*r_hand: rhand)
    }
    // ein Tag haengt an einem Tag-Knochen unter der Hand (JKA: *r_hand an rhang_tag_bone unter rradius): die Hand derselben
    // Seite; ein Knochen ist selbst Hand oder Fuss
    if (k < 0 || static_cast<std::size_t>(k) >= t.rig.erkennung.knochen.size()) { return -1; }
    if (p.tag >= 0) {
        const modell::Seite se = t.rig.erkennung.knochen[static_cast<std::size_t>(k)].seite;
        for (const modell::TagErkennung& tg : t.rig.tags) {
            if (tg.teil == p.tag && (tg.art == modell::TagArt::SaebelRechts || tg.art == modell::TagArt::SaebelLinks)) {
                return static_cast<int>(tg.seite == modell::Seite::Links ? modell::IkArt::ArmLinks : modell::IkArt::ArmRechts);
            }
        }
        if (se == modell::Seite::Mitte) { return -1; }
        return static_cast<int>(se == modell::Seite::Links ? modell::IkArt::ArmLinks : modell::IkArt::ArmRechts);
    }
    const modell::Rolle r = t.rig.erkennung.knochen[static_cast<std::size_t>(k)].rolle;
    if (r != modell::Rolle::Hand && r != modell::Rolle::Fuss) { return -1; }
    for (std::size_t i = 0; i < t.rig.griffe.size(); ++i) {
        const modell::Griff& g = t.rig.griffe[i];
        if ((g.art == modell::GriffArt::Hand || g.art == modell::GriffArt::Fuss) && g.knochen == k) { return modellIkArtFuerGriff(static_cast<int>(i)); }
    }
    return -1;
}

std::string pfadName(const ModellPfad& p) {
    const modell::Inhalt& in = mt().dok.inhalt();
    if (p.tag >= 0 && static_cast<std::size_t>(p.tag) < in.teile.size()) {
        return in.teile[static_cast<std::size_t>(p.tag)].name + (p.spitze ? std::string(" ") + tr(Str::MbIkSpitzeName) : std::string());
    }
    return p.knochen >= 0 && static_cast<std::size_t>(p.knochen) < g_app->anim.bones.size() ? g_app->anim.bones[static_cast<std::size_t>(p.knochen)].name : std::string("-");
}

bool ziehbar(const ModellPfad& p) {
    const ModellTab& t = mt();
    if (!modellRigDa()) { return false; }
    if (pfadGlied(p) >= 0) { return true; }
    if (p.knochen < 0 || static_cast<std::size_t>(p.knochen) >= t.rig.erkennung.knochen.size()) { return false; }
    const modell::Rolle r = t.rig.erkennung.knochen[static_cast<std::size_t>(p.knochen)].rolle;
    return r == modell::Rolle::Becken || r == modell::Rolle::Wurzel || r == modell::Rolle::Bewegung;
}

// Der Weltort eines Pfads am Bild (Knochen, Tag, Saebelspitze)
bool pfadOrt(const ModellPfad& p, const modell::RigSchicht& s, int bild, float w[3]) {
    std::vector<BoneMatrix> welt;
    modell::rigWelt(g_app->anim, s, bild, bild, 0.0F, welt);
    if (p.tag >= 0) { return modellSaebelSpitze(p, welt, w); }
    if (p.knochen < 0 || static_cast<std::size_t>(p.knochen) >= welt.size()) { return false; }
    const BoneMatrix& x = welt[static_cast<std::size_t>(p.knochen)];
    w[0] = x.m[0][3];
    w[1] = x.m[1][3];
    w[2] = x.m[2][3];
    return true;
}

// Der Pfadpunkt unter der Maus: Pfad, Bild
bool punktUnter(const ModellAnsicht& a, ImVec2 maus, int& pfad, int& bild) {
    ModellTab& t = mt();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const int jetzt = bildJetzt();
    const int von = std::max(erstes, jetzt - t.pfadDavor);
    const int bis = std::min(erstes + anzahl - 1, jetzt + t.pfadDanach);
    const modell::RigSchicht& s = schicht();
    float bd = ImGui::GetFontSize() * 0.5F;
    pfad = -1;
    for (std::size_t i = 0; i < t.pfade.size(); ++i) {
        for (int f = von; f <= bis; ++f) {
            float w[3];
            if (!pfadOrt(t.pfade[i], s, f, w)) { continue; }
            ImVec2 sp;
            if (!modellAufSchirm(a, w, sp)) { continue; }
            const float d = std::hypot(maus.x - sp.x, maus.y - sp.y);
            if (d < bd) {
                bd = d;
                pfad = static_cast<int>(i);
                bild = f;
            }
        }
    }
    return pfad >= 0;
}

}  // namespace

bool modellPfadZiehenStart(const ModellAnsicht& a, ImVec2 maus) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation || t.pfade.empty() || g_app->anim.empty()) { return false; }
    int pfad = -1;
    int bild = 0;
    if (!punktUnter(a, maus, pfad, bild)) { return false; }
    if (!ziehbar(t.pfade[static_cast<std::size_t>(pfad)])) {
        melden(tr(Str::MbKvPfadNurAnzeige));
        return true;   // der Klick gehoert dem Pfad (kein Rahmen)
    }
    t.kvPfadZug = pfad;
    t.kvPfadBild = bild;
    t.kvPfadStart = schicht();
    t.kvPfadAngelegt = false;
    t.kvPfadMaus[0] = maus.x;
    t.kvPfadMaus[1] = maus.y;
    (void)pfadOrt(t.pfade[static_cast<std::size_t>(pfad)], t.kvPfadStart, bild, t.kvPfadOrt);
    g_app->animPlaying = false;
    return true;
}

bool modellPfadZiehtGerade() { return mt().kvPfadZug >= 0; }

void modellPfadZiehenWeiter(const ModellAnsicht& a) {
    ModellTab& t = mt();
    if (t.kvPfadZug < 0) { return; }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (t.kvPfadAngelegt) { melden(format(Str::MbKvPfadGezogen, pfadName(t.pfade[static_cast<std::size_t>(t.kvPfadZug)]).c_str(), t.kvPfadBild)); }
        t.kvPfadZug = -1;
        return;
    }
    const ImVec2 m = ImGui::GetIO().MousePos;
    const float dx = m.x - t.kvPfadMaus[0];
    const float dy = m.y - t.kvPfadMaus[1];
    if (std::fabs(dx) + std::fabs(dy) < 1.0F && !t.kvPfadAngelegt) { return; }
    // die Bildebene durch den Punkt: rechts/oben aus der Matrix der Ansicht; je Achse die Bildpunkte je Einheit
    const float* vp = a.viewProj;
    float rechts[3] = {vp[0], vp[1], vp[2]};
    float hoch[3] = {vp[4], vp[5], vp[6]};
    for (float* v : {rechts, hoch}) {
        const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 1e-6F) {
            for (int k = 0; k < 3; ++k) { v[k] /= l; }
        }
    }
    ImVec2 s0;
    ImVec2 sr;
    ImVec2 sh;
    const float* p = t.kvPfadOrt;
    const float pr[3] = {p[0] + rechts[0], p[1] + rechts[1], p[2] + rechts[2]};
    const float ph[3] = {p[0] + hoch[0], p[1] + hoch[1], p[2] + hoch[2]};
    if (!modellAufSchirm(a, p, s0) || !modellAufSchirm(a, pr, sr) || !modellAufSchirm(a, ph, sh)) { return; }
    // [sr - s0, sh - s0] * (u, v) = (dx, dy)
    const float a11 = sr.x - s0.x;
    const float a12 = sh.x - s0.x;
    const float a21 = sr.y - s0.y;
    const float a22 = sh.y - s0.y;
    const float det = a11 * a22 - a12 * a21;
    if (std::fabs(det) < 1e-6F) { return; }
    const float u = (dx * a22 - a12 * dy) / det;
    const float v = (a11 * dy - a21 * dx) / det;
    const float weg[3] = {u * rechts[0] + v * hoch[0], u * rechts[1] + v * hoch[1], u * rechts[2] + v * hoch[2]};
    modell::RigSchicht neu = t.kvPfadStart;
    neu.neuInterp = kNeuInterp;
    std::vector<BoneMatrix> welt;
    modell::rigWelt(g_app->anim, t.kvPfadStart, t.kvPfadBild, t.kvPfadBild, 0.0F, welt);
    const ModellPfad& pf = t.pfade[static_cast<std::size_t>(t.kvPfadZug)];
    const int glied = pfadGlied(pf);
    if (glied >= 0) {
        // Stufe 4: der Punkt ist das IK-Ziel an diesem Bild (Hand/Fuss bzw. die Hand des Saebels: der Saebel geht mit, seine
        // Spitze um denselben Weg) - wie 3ds Max Trajektorien, Schluessel auf der Bahn verschieben
        const int sp = modellIkSpur(glied, neu);
        if (sp >= 0) {
            // nur dieser Punkt der Bahn: die Nachbarn halten ihre Pose (Max: Schluessel auf der Trajektorie verschieben)
            int erstes = 0;
            int anzahl = 0;
            bereich(erstes, anzahl);
            modell::ikNurAmBild(g_app->anim, neu, sp, t.kvPfadBild, erstes, erstes + anzahl - 1);
            BoneMatrix ziel = modell::starr(welt[static_cast<std::size_t>(neu.ik[static_cast<std::size_t>(sp)].ende.knochen)]);
            for (int k = 0; k < 3; ++k) { ziel.m[k][3] += weg[k]; }
            modell::ikZielSetzen(g_app->anim, neu, sp, t.kvPfadBild, ziel);
            if (pf.tag >= 0) {
                // ein Tag (Saebelspitze) haengt ueber seine Ecken auch am Unterarm (boltMatrix mischt die Gewichte): das Ziel der
                // Hand so nachfuehren, bis der Punkt dort steht (Newton-freie Festpunkt-Schritte, bis 1/1000 Einheit)
                const float soll[3] = {t.kvPfadOrt[0] + weg[0], t.kvPfadOrt[1] + weg[1], t.kvPfadOrt[2] + weg[2]};
                for (int schritt = 0; schritt < 12; ++schritt) {
                    float ist[3];
                    if (!pfadOrt(pf, neu, t.kvPfadBild, ist)) { break; }
                    const float d[3] = {soll[0] - ist[0], soll[1] - ist[1], soll[2] - ist[2]};
                    if (std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) < 1e-3F) { break; }
                    for (int k = 0; k < 3; ++k) { ziel.m[k][3] += d[k]; }
                    modell::ikZielSetzen(g_app->anim, neu, sp, t.kvPfadBild, ziel);
                }
            }
        }
    } else {
        const int knochen = pf.knochen;
        modell::rigKnochenVerschieben(g_app->anim, neu, knochen, t.kvPfadBild, welt, weg);
        modellKurvenAnimOffset(t.kvPfadStart, neu, {knochen}, t.kvPfadBild);
    }
    const std::string name = sequenzName();
    if (!t.kvPfadAngelegt) {
        t.dok.aendern(format(Str::MbKvPfadGezogen, pfadName(pf).c_str(), t.kvPfadBild), [&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
        t.kvPfadAngelegt = true;
    } else {
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
    }
    t.kvPfadWeg[0] = weg[0];
    t.kvPfadWeg[1] = weg[1];
    t.kvPfadWeg[2] = weg[2];
    ++t.kvAenderungen;
    g_app->modelDirty = true;
}

void modellPfadTooltip(const ModellAnsicht& a, ImVec2 maus) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation || t.pfade.empty() || t.kvPfadZug >= 0) { return; }
    int pfad = -1;
    int bild = 0;
    if (!punktUnter(a, maus, pfad, bild)) { return; }
    ImGui::SetTooltip("%s", tr(ziehbar(t.pfade[static_cast<std::size_t>(pfad)]) ? Str::MbKvPfadZiehenHint : Str::MbKvPfadNurAnzeige));
}

// Selbsttest: wo steht ein Schluessel?
bool modellDsRauteAufSchirm(int knochen, int bild, ImVec2& out) {
    for (std::size_t i = 0; i < g_zeilen.size() && i < g_zeileY.size(); ++i) {
        if (g_zeilen[i].art == 2 && g_zeilen[i].knochen == knochen) {
            out = ImVec2{g_feld.x(static_cast<float>(bild)), g_zeileY[i]};
            return true;
        }
    }
    return false;
}

bool modellKePunktAufSchirm(int knochen, int kanal, int bild, float wert, ImVec2& out) {
    (void)knochen;
    (void)kanal;
    out = ImVec2{g_feld.x(static_cast<float>(bild)), g_feld.y(wert)};
    return true;
}

bool modellKvZeileAufSchirm(int art, int glied, ImVec2& out) {
    for (std::size_t i = 0; i < g_zeilen.size() && i < g_zeileY.size(); ++i) {
        if (g_zeilen[i].art == art && (art != 1 || g_zeilen[i].glied == glied)) {
            out = ImVec2{g_feld.lo.x, g_zeileY[i]};
            return true;
        }
    }
    return false;
}

float modellKvBildAufSchirm(float bild) { return g_feld.x(bild); }

bool modellKvFeldEnthaelt(ImVec2 p) { return p.x >= g_feld.lo.x && p.x <= g_feld.hi.x && p.y >= g_feld.lo.y && p.y <= g_feld.hi.y; }

bool modellPfadPunktAufSchirm(int pfad, int bild, ImVec2& out) {
    ModellTab& t = mt();
    if (pfad < 0 || static_cast<std::size_t>(pfad) >= t.pfade.size()) { return false; }
    float w[3];
    if (!pfadOrt(t.pfade[static_cast<std::size_t>(pfad)], schicht(), bild, w)) { return false; }
    return modellAufSchirm(t.ansichten[t.aktiv], w, out);
}

}  // namespace bhed::gui
