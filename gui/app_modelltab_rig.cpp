// app_modelltab_rig.cpp - Model-Tab, Modell-Block Stufe 3: Knochen-Erkennung, Control Rig mit einem Klick, Posen,
// Ghosts (Onion Skin), Silhouette, Bewegungspfade, Backen in die GLA
//
//   Rig anlegen        modell::rigAnlegen (bhed/modell/rig.h): Rollen je Knochen, Griffe, Pol-Ziele, Fussrolle, Saebelgriff
//   Uebersicht         eine Figur zum Anklicken (eigene Zeichnung): gruen erkannt, gelb unklar, grau leer; ein Fleck und
//                      ein Knochen ergeben eine Zuordnung von Hand (im Dokument, Strg+Z)
//   Griffe             im Szenen-Explorer (eigene Symbole, Auge), in der Ansicht wie 3ds Max als Kasten mit Gizmo (E dreht,
//                      W verschiebt das Becken), wie Blender als Ring mit R/G; jede Drehung ist FK
//                      (G2API_SetBoneAngles POSTMULT) und ein Schluessel am Bild der gewaehlten Sequenz
//   Posen              kopieren, einfuegen, gespiegelt einfuegen (Seitenachse aus den Daten), Bindepose, Handposen aus den
//                      Animationen des Spiels (Posen-Bibliothek)
//   Ghosts / Pfade     Posen davor/danach durchscheinend (gpu::ModellGeist), Spur eines Knochens oder Tags mit Punkten je
//                      Bild und hervorgehobenen Schluesseln
//   Backen             die Sequenz mit der Schicht ueber compressBone in eine GLA, auf den naechsten freien Platz des Ziels
//                      aus Stufe 2 (Karten-GLA, _humanoid.gla, eigene GLA)
#include "app_internal.h"
#include "app_modelltab_knochen.h"   // AP13K: Rig-Stile
#include "textumbruch.h"
#include "gpumap.h"

#include "bhed/gpuskin.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
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
    char b[600];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

const AnimEntry* sequenz() {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return nullptr; }
    return &g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
}

// Der Bereich der gewaehlten Sequenz (ohne Sequenz: die ganze GLA).
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

std::string rigSchluessel() {
    const modell::Inhalt& in = mt().dok.inhalt();
    std::string k = g_app->anim.name + "|" + g_app->modelPath + "|" + std::to_string(g_app->anim.bones.size()) + "|" + std::to_string(in.netzStand) + "|";
    for (const modell::Zuordnung& z : in.rig.zuordnung) {
        k += std::to_string(z.knochen) + ":" + std::to_string(static_cast<int>(z.rolle)) + ":" + std::to_string(static_cast<int>(z.seite)) + ",";
    }
    k += in.rig.nurFk ? "|fk" : "|ik";   // FIX612C: Rueckgaengig/Wiederholen des Modus baut die Griffe neu
    return k;
}

// Das Rig, falls noch keins da ist (die Posen brauchen Partner und Seitenachse).
bool rigSicher() {
    if (modellRigDa()) { return true; }
    return modellRigAnlegen();
}

// Der Hinweis zu einem Griff (Stufe 4: Hand/Fuss mit IK, Pol, Fussrolle)
Str griffHinweis(const modell::Griff& g) {
    using G = modell::GriffArt;
    const bool blender = modellBlender();
    if (g.art == G::PolEllbogen || g.art == G::PolKnie) { return Str::MbIkPolHint; }
    if (g.art == G::Ferse || g.art == G::Ballen || g.art == G::Zehe) { return Str::MbIkRolleGriffHint; }
    if (g.art == G::Hand || g.art == G::Fuss || g.art == G::Saebelgriff) { return blender ? Str::MbIkGriffHintBlender : Str::MbIkGriffHintMax; }
    return blender ? Str::MbRgGriffHintBlender : Str::MbRgGriffHintMax;
}

Str griffText(modell::GriffArt a) {
    static const Str kText[modell::kGriffArten] = {Str::MbRgBecken,     Str::MbRgWirbel,  Str::MbRgHals,      Str::MbRgKopf,     Str::MbRgSchluessel,
                                                   Str::MbRgOberarm,    Str::MbRgUnterarm, Str::MbRgHand,     Str::MbRgOberschenkel, Str::MbRgUnterschenkel,
                                                   Str::MbRgFuss,       Str::MbRgZehen,   Str::MbRgPolEllbogen, Str::MbRgPolKnie, Str::MbRgFerse,
                                                   Str::MbRgBallen,     Str::MbRgZehe,    Str::MbRgSaebelgriff};
    return kText[std::clamp(static_cast<int>(a), 0, modell::kGriffArten - 1)];
}

// Der Name einer Rolle in der Sprache der Oberflaeche (die Uebersicht; rollenSchluessel bleibt fuer Berichte).
const char* rolleText(modell::Rolle r) {
    using R = modell::Rolle;
    switch (r) {
        case R::Wurzel: return tr(Str::MbRgRWurzel);
        case R::Bewegung: return tr(Str::MbRgRBewegung);
        case R::Becken: return tr(Str::MbRgBecken);
        case R::Wirbel: return tr(Str::MbRgWirbel);
        case R::Hals: return tr(Str::MbRgHals);
        case R::Kopf: return tr(Str::MbRgKopf);
        case R::Gesicht: return tr(Str::MbRgRGesicht);
        case R::Schluessel: return tr(Str::MbRgSchluessel);
        case R::Oberarm: return tr(Str::MbRgOberarm);
        case R::OberarmTwist: return tr(Str::MbRgRTwistOA);
        case R::Unterarm: return tr(Str::MbRgUnterarm);
        case R::UnterarmTwist: return tr(Str::MbRgRTwistUA);
        case R::Hand: return tr(Str::MbRgHand);
        case R::Finger: return tr(Str::MbRgRFinger);
        case R::Oberschenkel: return tr(Str::MbRgOberschenkel);
        case R::OberschenkelTwist: return tr(Str::MbRgRTwistOS);
        case R::Unterschenkel: return tr(Str::MbRgUnterschenkel);
        case R::Fuss: return tr(Str::MbRgFuss);
        case R::Zehen: return tr(Str::MbRgZehen);
        case R::Anhang: return tr(Str::MbRgRAnhang);
        case R::TagKnochen: return tr(Str::MbRgRTag);
        case R::Sonstige: return tr(Str::MbRgRSonstige);
        case R::Keine: break;
    }
    return tr(Str::MbRgRKeine);
}

// Der Grund, warum ein Knochen unklar ist, in der Sprache der Oberflaeche.
const char* grundText(const std::string& g) {
    if (g == "seite") { return tr(Str::MbRgGrundSeite); }
    if (g == "reihenfolge") { return tr(Str::MbRgGrundReihenfolge); }
    if (g == "glied") { return tr(Str::MbRgGrundGlied); }
    return g.c_str();
}

std::string griffName(int i) {
    const ModellTab& t = mt();
    if (i < 0 || static_cast<std::size_t>(i) >= t.rig.griffe.size()) { return {}; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(i)];
    std::string n = tr(griffText(g.art));
    if (g.seite == modell::Seite::Links) { n += std::string(" ") + tr(Str::MbRgL); }
    if (g.seite == modell::Seite::Rechts) { n += std::string(" ") + tr(Str::MbRgR); }
    int gleiche = 0;
    for (const modell::Griff& x : t.rig.griffe) { gleiche += (x.art == g.art && x.seite == g.seite) ? 1 : 0; }
    if (gleiche > 1) { n += " " + std::to_string(g.nummer); }
    return n;
}

ImU32 seitenFarbe(modell::Seite s, float alpha = 1.0F) {
    // wie Biped in 3ds Max: links blau, rechts gruen; Mitte gelb (eigene Zeichnung, nur die Farbregel)
    const auto a = static_cast<int>(std::clamp(alpha, 0.0F, 1.0F) * 255.0F);
    if (s == modell::Seite::Links) { return IM_COL32(70, 140, 255, a); }
    if (s == modell::Seite::Rechts) { return IM_COL32(70, 205, 95, a); }
    return IM_COL32(235, 200, 60, a);
}

// Die Knochen, die eine Aenderung am gewaehlten Griff/Knochen trifft (Bindepose, Schluessel loeschen).
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

// Eine Aenderung der Schicht der gewaehlten Sequenz als ein Rueckgaengig-Schritt (nur Rig, das Netz bleibt).
void schichtAendern(Str was, const std::function<void(modell::RigSchicht&)>& f) {
    ModellTab& t = mt();
    const AnimEntry* e = sequenz();
    const std::string name = e != nullptr ? e->name : std::string();
    t.dok.aendern(tr(was), [&](modell::Inhalt& in) {
        modell::RigSchicht& sch = in.rig.schichten[name];
        sch.neuInterp = modell::Interp::Bezier;   // Stufe 3b: neue Schluessel wie Max und Blender
        f(sch);
    }, false);
    g_app->modelDirty = true;
}

// --- Aktionen: Menue "Animation", Rechtsklick, Suche (X/F3), Tasten ------------------------------------------------
struct RigAktion {
    Str text;
    Str hinweis;
    const char* maxTaste;       // eigene Taste des Vorbilds im Modus wie 3ds Max ("" keine)
    const char* blenderTaste;   // ... wie Blender
    ImGuiKeyChord maxChord;
    ImGuiKeyChord blenderChord;
    const char* quelle;         // Herkunft der Taste
    std::function<void()> tun;
    std::function<bool()> an;   // Haken im Menue (leer: kein Umschalter)
    int gruppe;                 // 0 Rig, 1 Pose, 2 Auswahl, 3 Anzeige, 4 Backen, 5 IK/FK (Stufe 4), 6 Auto-Physik (Stufe 6), 7 Retargeting (Stufe 5), 8 Presets (5b)
};

int artJetztRig() {
    const ModellTab& t = mt();
    if (t.ikWahl >= 0) { return t.ikWahl; }
    return modellIkArtFuerGriff(t.griffWahl);
}

const std::vector<RigAktion>& rigAktionen() {
    static const std::vector<RigAktion> kListe = {
        {Str::MbRgAnlegen, Str::MbRgAnlegenHint, "", "", 0, 0, "", [] { (void)modellAnimRigErstellen(false); }, {}, 0},
        // AP10 (Entwurf, Gruppe "Rig erstellen"): das Rig nur mit FK und die Animation der Sequenz auf das Rig
        {Str::AzRigFk, Str::AzRigFkHint, "", "", 0, 0, "", [] { (void)modellAnimRigErstellen(true); }, [] { return modellRigDa() && modellRigNurFk(); }, 0},
        {Str::AzAufRig, Str::AzAufRigHint, "", "", 0, 0, "", [] { (void)modellAnimAufRigLegen(); }, {}, 0},
        {Str::MbRgUebersicht, Str::MbRgUebersichtHint, "", "", 0, 0, "", [] { mt().uebersichtOffen = !mt().uebersichtOffen; },
         [] { return mt().uebersichtOffen; }, 0},
        {Str::MbRgGriffeZeigen, Str::MbRgGriffeZeigenHint, "", "", 0, 0, "", [] {
             mt().rigZeigen = !mt().rigZeigen;
             g_app->modelDirty = true;
         },
         [] { return mt().rigZeigen; }, 0},
        {Str::MbRgSchluesselLoeschen, Str::MbRgSchluesselLoeschenHint, "", "", 0, 0, "", [] { (void)modellRigSchluesselLoeschen(); }, {}, 0},
        {Str::MbRgKopieren, Str::MbRgKopierenHint, "", "Ctrl+C", 0, ImGuiMod_Ctrl | ImGuiKey_C, "Doku | pose.copy | Strg+C", [] { (void)modellPoseKopieren(); }, {}, 1},
        {Str::MbRgEinfuegen, Str::MbRgEinfuegenHint, "", "Ctrl+V", 0, ImGuiMod_Ctrl | ImGuiKey_V, "Doku | pose.paste | Strg+V", [] { (void)modellPoseEinfuegen(false); }, {}, 1},
        {Str::MbRgGespiegelt, Str::MbRgGespiegeltHint, "", "Ctrl+Shift+V", 0, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V,
         "Doku | pose.paste flipped=True | Strg+Umschalt+V", [] { (void)modellPoseEinfuegen(true); }, {}, 1},
        {Str::MbRgBindeAuswahl, Str::MbRgBindeAuswahlHint, "", "Alt+R", 0, ImGuiMod_Alt | ImGuiKey_R, "Pose | pose.rot_clear | Alt+R PRESS (blender_keys.txt)",
         [] { (void)modellBindepose(false); }, {}, 1},
        {Str::MbRgBindeAlle, Str::MbRgBindeAlleHint, "", "", 0, 0, "", [] { (void)modellBindepose(true); }, {}, 1},
        {Str::MbRgHandOffenL, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Links, 0); }, {}, 1},
        {Str::MbRgHandFaustL, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Links, 1); }, {}, 1},
        {Str::MbRgHandSaebelL, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Links, 2); }, {}, 1},
        {Str::MbRgHandOffenR, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Rechts, 0); }, {}, 1},
        {Str::MbRgHandFaustR, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Rechts, 1); }, {}, 1},
        {Str::MbRgHandSaebelR, Str::MbRgHandHint, "", "", 0, 0, "", [] { (void)modellHandpose(modell::Seite::Rechts, 2); }, {}, 1},
        {Str::MbRgGegenseite, Str::MbRgGegenseiteHint, "", "Ctrl+Shift+M", 0, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_M,
         "Doku | pose.select_mirror | Strg+Umschalt+M", [] { (void)modellAuswahlGegenseite(); }, {}, 2},
        {Str::MbRgEltern, Str::MbRgElternHint, "Page Up", "[", ImGuiKey_PageUp, ImGuiKey_LeftBracket,
         "max_hotkeys_de.txt: Bild auf - Uebergeordnetes Objekt auswaehlen | Doku | pose.select_hierarchy direction=PARENT | [",
         [] { (void)modellAuswahlEltern(); }, {}, 2},
        {Str::MbRgKinder, Str::MbRgKinderHint, "Page Down", "]", ImGuiKey_PageDown, ImGuiKey_RightBracket,
         "max_hotkeys_de.txt: Bild ab - Untergeordnetes Objekt auswaehlen | Doku | pose.select_hierarchy direction=CHILD | ]",
         [] { (void)modellAuswahlKinder(); }, {}, 2},
        {Str::MbRgGhosts, Str::MbRgGhostsHint, "", "", 0, 0, "", [] {
             mt().geister.an = !mt().geister.an;
             g_app->modelDirty = true;
         },
         [] { return mt().geister.an; }, 3},
        {Str::MbRgSilhouette, Str::MbRgSilhouetteHint, "", "", 0, 0, "", [] {
             mt().silhouette = !mt().silhouette;
             g_app->modelDirty = true;
         },
         [] { return mt().silhouette; }, 3},
        {Str::MbRgPfadHinzu, Str::MbRgPfadHinzuHint, "", "", 0, 0, "", [] { (void)modellPfadHinzu(); }, {}, 3},
        {Str::MbIkPfadSpitze, Str::MbIkPfadSpitzeHint, "", "", 0, 0, "", [] { (void)modellPfadSpitzeHinzu(); }, {}, 3},
        {Str::MbRgPfadeLeeren, Str::MbRgPfadeLeerenHint, "", "", 0, 0, "", [] { modellPfadeLeeren(); }, {}, 3},
        {Str::MbRgBacken, Str::MbRgBackenHint, "", "", 0, 0, "", [] { (void)modellRigBacken(); }, {}, 4},
        // Stufe 4: IK/FK - keine eigenen Tasten in max_hotkeys_de.txt und blender_keys.txt; in der Ansicht W/E bzw. G/R
        {Str::MbIkAufIk, Str::MbIkAufIkHint, "", "", 0, 0, "", [] { (void)modellIkWechseln(artJetztRig(), true); }, {}, 5},
        {Str::MbIkAufFk, Str::MbIkAufFkHint, "", "", 0, 0, "", [] { (void)modellIkWechseln(artJetztRig(), false); }, {}, 5},
        {Str::MbIkZielAufFk, Str::MbIkZielAufFkHint, "", "", 0, 0, "", [] { (void)modellIkEinrasten(artJetztRig(), true); }, {}, 5},
        {Str::MbIkFkAufIk, Str::MbIkFkAufIkHint, "", "", 0, 0, "", [] { (void)modellIkEinrasten(artJetztRig(), false); }, {}, 5},
        {Str::MbIkAuto, Str::MbIkAutoHint, "", "", 0, 0, "", [] { mt().ikAutoEinrasten = !mt().ikAutoEinrasten; }, [] { return mt().ikAutoEinrasten; }, 5},
        {Str::MbIkRaumWelt, Str::MbIkRaumHint, "", "", 0, 0, "", [] { (void)modellIkRaum(artJetztRig(), 0); }, {}, 5},
        {Str::MbIkRaumBecken, Str::MbIkRaumHint, "", "", 0, 0, "", [] { (void)modellIkRaum(artJetztRig(), 1); }, {}, 5},
        {Str::MbIkRaumBrust, Str::MbIkRaumHint, "", "", 0, 0, "", [] { (void)modellIkRaum(artJetztRig(), 2); }, {}, 5},
        {Str::MbIkRaumKopf, Str::MbIkRaumHint, "", "", 0, 0, "", [] { (void)modellIkRaum(artJetztRig(), 3); }, {}, 5},
        {Str::MbIkRaumHand, Str::MbIkRaumHint, "", "", 0, 0, "", [] { (void)modellIkRaum(artJetztRig(), 4); }, {}, 5},
        {Str::MbIkSaebelHand, Str::MbIkSaebelHandHint, "", "", 0, 0, "", [] { (void)modellIkSaebelHand(); }, {}, 5},
        {Str::MbIkLoeschen, Str::MbIkLoeschenHint, "", "", 0, 0, "", [] { (void)modellIkSchluesselLoeschen(artJetztRig()); }, {}, 5},
        {Str::MbIkBacken, Str::MbIkBackenHint, "", "", 0, 0, "", [] { (void)modellIkBacken(artJetztRig()); }, {}, 5},
        // Stufe 6: Auto-Physik - weder 3ds Max (max_hotkeys_de.txt) noch Blender (blender_keys.txt) haben eine Taste dafuer
        {Str::PhAn, Str::PhAnHint, "", "", 0, 0, "", [] {
             const modell::PhysikEinstellung* e = modellPhysikEinstellung();
             (void)modellPhysikAn(e == nullptr || !e->an);
         },
         [] {
             const modell::PhysikEinstellung* e = modellPhysikEinstellung();
             return e != nullptr && e->an;
         },
         6},
        {Str::PhRolloutOeffnen, Str::PhRolloutOeffnenHint, "", "", 0, 0, "", [] { mt().physikOeffnen = true; }, {}, 6},
        {Str::PhZeigen, Str::PhZeigenHint, "", "", 0, 0, "", [] {
             mt().physikZeigen = !mt().physikZeigen;
             g_app->modelDirty = true;
         },
         [] { return mt().physikZeigen; }, 6},
        {Str::PhAbsprungHier, Str::PhAbsprungHint, "", "", 0, 0, "", [] { (void)modellPhysikFlugHier(false); }, {}, 6},
        {Str::PhLandungHier, Str::PhLandungHint, "", "", 0, 0, "", [] { (void)modellPhysikFlugHier(true); }, {}, 6},
        {Str::PhFlugAutomatisch, Str::PhFlugAutomatischHint, "", "", 0, 0, "", [] { (void)modellPhysikFlugAutomatisch(); }, {}, 6},
        {Str::PhKetteHier, Str::PhKetteHierHint, "", "", 0, 0, "", [] { (void)modellPhysikKetteHier(); }, {}, 6},
        {Str::PhKettenVorschlag, Str::PhKettenVorschlagHint, "", "", 0, 0, "", [] { (void)modellPhysikKettenVorschlag(); }, {}, 6},
        // Stufe 5: Retargeting - keine Tasten in max_hotkeys_de.txt und blender_keys.txt
        {Str::RtQuelleWaehlen, Str::RtQuelleWaehlenHint, "", "", 0, 0, "", [] {
             mt().rt.quelleWaehlen = true;
             mt().rt.oeffnen = true;
         },
         {}, 7},
        // Animations-Import: Mixamo/glTF/BVH auf einen Klick (keine Taste in Max/Blender: dort Datei > Importieren)
        {Str::AiImport, Str::AiImportHint, "", "", 0, 0, "", [] {
             mt().rt.oeffnen = true;
             (void)modellRtImportWaehlen();
         },
         {}, 7},
        {Str::RtUebertragen, Str::RtUebertragenHint, "", "", 0, 0, "", [] { (void)modellRtUebertragen(); }, {}, 7},
        {Str::RtVorschau, Str::RtVorschauHint, "", "", 0, 0, "", [] { (void)modellRtVorschau(!mt().rt.vorschau); }, [] { return mt().rt.vorschau; }, 7},
        {Str::RtQuelleZeigen, Str::RtQuelleZeigenHint, "", "", 0, 0, "", [] {
             mt().rt.quelleZeigen = !mt().rt.quelleZeigen;
             g_app->modelDirty = true;
         },
         [] { return mt().rt.quelleZeigen; }, 7},
        {Str::RtAutoLeer, Str::RtAutoLeerHint, "", "", 0, 0, "", [] { (void)modellRtZuordnen(false); }, {}, 7},
        {Str::RtAutoAlle, Str::RtAutoAlleHint, "", "", 0, 0, "", [] { (void)modellRtZuordnen(true); }, {}, 7},
        {Str::RtTBeide, Str::RtHaltungHint, "", "", 0, 0, "", [] {
             (void)modellRtHaltung(false, modell::RtHaltung::T);
             (void)modellRtHaltung(true, modell::RtHaltung::T);
         },
         {}, 7},
        {Str::RtABeide, Str::RtHaltungHint, "", "", 0, 0, "", [] {
             (void)modellRtHaltung(false, modell::RtHaltung::A);
             (void)modellRtHaltung(true, modell::RtHaltung::A);
         },
         {}, 7},
        {Str::RtBackenMax, Str::RtBackenHint, "", "", 0, 0, "", [] { (void)modellRtBacken(); }, {}, 7},
        {Str::RtRolloutOeffnen, Str::RtRolloutHint, "", "", 0, 0, "", [] { mt().rt.oeffnen = true; }, {}, 7},
        // Stufe 5b: Presets
        {Str::PrRolloutOeffnen, Str::PrRolloutHint, "", "", 0, 0, "", [] {
             mt().rt.presetOeffnen = true;
             modellAnim().generatorOffen = true;
             modellPresetGangWaehlen(mt().rt.gangWahl >= 0 ? mt().rt.gangWahl : static_cast<int>(modellRtDaten()->gang.art));
             modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
             ui::rolloutZeigen(tr(Str::PrRollout));
         },
         {}, 8},
        // ENT2: der erzeugte Gang steht als Eintrag in der Preset-Liste (Gehen (erzeugt) / Laufen (erzeugt))
        {Str::EnGangWaehlen, Str::EnGangHint, "", "", 0, 0, "", [] {
             mt().rt.presetOeffnen = true;
             modellPresetGangWaehlen(static_cast<int>(modellRtDaten()->gang.art));
             modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
             ui::rolloutZeigen(tr(Str::PrRollout));
         },
         {}, 8},
        {Str::PrAuflegen, Str::PrAuflegenHint, "", "", 0, 0, "", [] { (void)modellPresetAuflegen(); }, {}, 8},
        {Str::AiBibliothekOeffnen, Str::AiBibliothekHint, "", "", 0, 0, "", [] {
             mt().rt.presetOeffnen = true;
             modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
             ui::rolloutZeigen(tr(Str::PrRollout));
         },
         {}, 8},
        {Str::PrMessenAktion, Str::PrMessenHint, "", "", 0, 0, "", [] { (void)modellPresetMessen(); }, {}, 8},
    };
    return kListe;
}

std::string aktionText(const RigAktion& a) {
    const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
    std::string s = tr(a.text);
    if (taste[0] != '\0') { s += std::string(" (") + taste + ")"; }
    return s;
}

void aktionAusloesen(const RigAktion& a) {
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(a.text);
    g_app->modelDirty = true;
}

void aktionenAlsMenue(int gruppeNur) {
    int gruppe = -1;
    static const Str kGruppen[6] = {Str::MbRgGruppeRig, Str::MbRgGruppePose, Str::MbRgGruppeAuswahl, Str::MbRgGruppeAnzeige, Str::MbRgGruppeBacken, Str::MbIkGruppe};
    const auto& liste = rigAktionen();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const RigAktion& a = liste[i];
        if (gruppeNur >= 0 && a.gruppe != gruppeNur) { continue; }
        // Stufe 4: IK/FK als eigenes Untermenue (das Menue Animation passte sonst nicht mehr auf den Schirm, mb6_hb_en);
        // Stufe 6: Auto-Physik ebenso
        if (gruppeNur < 0 && a.gruppe >= 5) {
            if (gruppe == a.gruppe) { continue; }
            gruppe = a.gruppe;
            ImGui::Separator();
            // Stufe 5: Retargeting ebenso
            const Str menue = gruppe == 5 ? Str::MbIkGruppe : (gruppe == 6 ? Str::PhRollout : (gruppe == 7 ? Str::RtRollout : Str::PrRollout));
            const Str hinweis = gruppe == 5 ? Str::MbIkRolloutHint : (gruppe == 6 ? Str::PhRolloutHint : (gruppe == 7 ? Str::RtRolloutHint : Str::PrRolloutHint));
            if (ImGui::BeginMenu(tr(menue))) {
                aktionenAlsMenue(gruppe);
                ImGui::EndMenu();
            }
            ImGui::SetItemTooltip("%s", tr(hinweis));
            continue;
        }
        if (a.gruppe != gruppe) {
            if (gruppe >= 0) { ImGui::Separator(); }
            gruppe = a.gruppe;
            // die Untermenues tragen ihre eigene Ueberschrift (Auto-Physik zeigte "IK/FK", kGruppen endet bei 5)
            cjkTextDisabled("%s", tr(gruppe <= 5 ? kGruppen[std::max(gruppe, 0)] : (gruppe == 6 ? Str::PhRollout : (gruppe == 7 ? Str::RtRollout : Str::PrRollout))));
        }
        ImGui::PushID(static_cast<int>(i));
        const char* taste = modellBlender() ? a.blenderTaste : a.maxTaste;
        if (ImGui::MenuItem(tr(a.text), taste[0] != '\0' ? taste : nullptr, a.an ? a.an() : false)) { aktionAusloesen(a); }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
        ImGui::PopID();
    }
}

}  // namespace

int modellRigAktionZahl() { return static_cast<int>(rigAktionen().size()); }
int modellRigAktionGruppe(int i) { return rigAktionen()[static_cast<std::size_t>(i)].gruppe; }
const char* modellRigAktionText(int i) { return tr(rigAktionen()[static_cast<std::size_t>(i)].text); }
const char* modellRigAktionHinweis(int i) { return tr(rigAktionen()[static_cast<std::size_t>(i)].hinweis); }
const char* modellRigAktionTaste(int i, bool blender) {
    const RigAktion& a = rigAktionen()[static_cast<std::size_t>(i)];
    return blender ? a.blenderTaste : a.maxTaste;
}
void modellRigAktionAusloesen(int i) { aktionAusloesen(rigAktionen()[static_cast<std::size_t>(i)]); }

bool modellRigHatTaste(ImGuiKeyChord c) {
    if (c == 0 || mt().bereich != ModellBereich::Animation) { return false; }
    const bool blender = modellBlender();
    for (const RigAktion& a : rigAktionen()) {
        if ((blender ? a.blenderChord : a.maxChord) == c) { return true; }
    }
    return false;
}

// Die Tasten der Aktionen (nur im Bereich Animation; sonst gehoeren Strg+C/V usw. den anderen Bereichen).
bool modellRigTasten() {
    if (mt().bereich != ModellBereich::Animation) { return false; }
    const bool blender = modellBlender();
    for (const RigAktion& a : rigAktionen()) {
        const ImGuiKeyChord c = blender ? a.blenderChord : a.maxChord;
        if (c != 0 && ImGui::IsKeyChordPressed(c)) {
            aktionAusloesen(a);
            return true;
        }
    }
    return false;
}

// Fuer die Suche "Alle Aktionen" (X / F3): die Zeilen, die `such` (klein) enthalten; true, wenn eine ausgeloest wurde.
bool modellRigAktionenSuche(const std::string& such) {
    for (const RigAktion& a : rigAktionen()) {
        const std::string text = aktionText(a);
        std::string k = text;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (!such.empty() && k.find(such) == std::string::npos) { continue; }
        if (modellAktionZeile(text.c_str(), static_cast<int>(ModellBereich::Animation))) {
            aktionAusloesen(a);
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
    }
    return false;
}

void modellAnimationMenue() {
    aktionenAlsMenue(-1);
    ImGui::Separator();
    modellKurvenMenue();   // Stufe 3b: Animationswerkzeuge
}

void modellRigKontextMenue() {
    if (ImGui::BeginMenu(tr(Str::MbRgGruppeRig))) {
        aktionenAlsMenue(-1);
        ImGui::EndMenu();
    }
    modellKurvenKontextMenue();   // Stufe 3b
}

// --- Rig anlegen ----------------------------------------------------------------------------------------------------------

bool modellRigDa() { return !mt().rig.leer() && !mt().rigFuer.empty(); }

bool modellRigAnlegen() {
    ModellTab& t = mt();
    if (g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    const modell::Inhalt& in = t.dok.inhalt();
    // AP10: was die Erkennung offen laesst oder unklar nennt, weiss die Vorlage des Skeletts; Zuordnungen von Hand gelten zuletzt
    std::vector<modell::Zuordnung> zuordnung = modellAnimVorlageZuordnung();
    zuordnung.insert(zuordnung.end(), in.rig.zuordnung.begin(), in.rig.zuordnung.end());
    // FIX612C (P3-5): der gewaehlte Griff, um ihn im neuen Rig wiederzufinden - der Index allein traefe nach dem Wechsel nur FK <->
    // Control Rig einen anderen Griff (die IK-Griffe stehen zwischen den FK- und den Saebelgriffen)
    const bool hatteWahl = t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size();
    const modell::Griff alt = hatteWahl ? t.rig.griffe[static_cast<std::size_t>(t.griffWahl)] : modell::Griff{};
    t.rig = modell::rigAnlegen(g_app->anim, in, 0, zuordnung);
    // AP10 "Animation-Rig (nur FK)": ohne die Griffe, die erst mit IK wirken (Pol-Ziele, Ferse, Ballen, Zehe)
    const bool nurFk = in.rig.nurFk;
    if (nurFk) {
        auto& gr = t.rig.griffe;
        gr.erase(std::remove_if(gr.begin(), gr.end(), [](const modell::Griff& x) { return x.ik; }), gr.end());
    }
    t.rigFuer = rigSchluessel();
    t.griffVersteckt.assign(t.rig.griffe.size(), 0);
    t.griffWahl = -1;
    for (std::size_t i = 0; hatteWahl && i < t.rig.griffe.size(); ++i) {
        const modell::Griff& g = t.rig.griffe[i];
        if (g.art == alt.art && g.seite == alt.seite && g.knochen == alt.knochen && g.nummer == alt.nummer && g.tag == alt.tag) {
            t.griffWahl = static_cast<int>(i);
            break;
        }
    }
    int ik = 0;
    for (const modell::Griff& g : t.rig.griffe) { ik += g.ik ? 1 : 0; }
    if (nurFk) {
        melden(format(Str::AzNurFkMeldung, static_cast<int>(t.rig.griffe.size()), t.rig.erkennung.erkannt, static_cast<int>(g_app->anim.bones.size()), t.rig.erkennung.unklar));
    } else {
        melden(format(Str::MbRgAngelegt, static_cast<int>(t.rig.griffe.size()) - ik, ik, t.rig.erkennung.erkannt, static_cast<int>(g_app->anim.bones.size()),
                      t.rig.erkennung.unklar));
    }
    g_app->modelDirty = true;
    return !t.rig.leer();
}

void modellRigAktualisieren() {
    ModellTab& t = mt();
    if (t.rigFuer.empty()) { return; }
    const std::string k = rigSchluessel();
    if (k == t.rigFuer) { return; }
    // ein anderes Modell / eine andere GLA: das Rig gilt nicht mehr; eine Zuordnung von Hand: neu anlegen
    const std::string alt = t.rigFuer.substr(0, t.rigFuer.find('|', t.rigFuer.find('|') + 1));
    const std::string neu = k.substr(0, k.find('|', k.find('|') + 1));
    if (alt != neu || g_app->anim.bones.empty()) {
        t.rig = modell::Rig{};
        t.rigFuer.clear();
        t.griffWahl = -1;
        return;
    }
    (void)modellRigAnlegen();   // FIX612C: findet den gewaehlten Griff selbst wieder
}

void modellRigZuordnen(int knochen, modell::Rolle rolle, modell::Seite seite) {
    ModellTab& t = mt();
    if (knochen < 0 || static_cast<std::size_t>(knochen) >= g_app->anim.bones.size()) { return; }
    t.dok.aendern(tr(Str::MbRgUndoZuordnung), [&](modell::Inhalt& in) {
        auto& z = in.rig.zuordnung;
        z.erase(std::remove_if(z.begin(), z.end(), [&](const modell::Zuordnung& x) { return x.knochen == knochen; }), z.end());
        z.push_back(modell::Zuordnung{knochen, rolle, seite});
    }, false);
    if (t.rigFuer.empty()) {
        (void)modellRigAnlegen();
    } else {
        modellRigAktualisieren();
    }
    melden(format(Str::MbRgZugeordnet, g_app->anim.bones[static_cast<std::size_t>(knochen)].name.c_str(), rolleText(rolle)));
}

void modellRigGriffWaehlen(int griff) {
    ModellTab& t = mt();
    if (griff < 0 || static_cast<std::size_t>(griff) >= t.rig.griffe.size()) {
        t.griffWahl = -1;
        return;
    }
    t.griffWahl = griff;
    t.knochenWahl = t.rig.griffe[static_cast<std::size_t>(griff)].knochen;
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
    melden(format(g.ik ? Str::MbRgGriffIk : Str::MbRgGriffGewaehlt, griffName(griff).c_str(),
                  g.knochen >= 0 ? g_app->anim.bones[static_cast<std::size_t>(g.knochen)].name.c_str() : "-"));
    g_app->modelDirty = true;
}

// --- Schicht ---------------------------------------------------------------------------------------------------------------

const modell::RigSchicht* modellRigSchichtJetzt() {
    const modell::Inhalt& in = mt().dok.inhalt();
    const AnimEntry* e = sequenz();
    const auto it = in.rig.schichten.find(e != nullptr ? e->name : std::string());
    return it == in.rig.schichten.end() ? nullptr : &it->second;
}

void modellRigNachMal(int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus, const std::vector<BoneFrameOverride>& ueber) {
    aus.clear();
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    if (s == nullptr) { return; }
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    // die Schicht gehoert zur Sequenz: ausserhalb ihrer Bilder wirkt sie nicht
    const float bild = static_cast<float>(bildA) + (bildB != bildA ? anteil : 0.0F);
    if (bild < static_cast<float>(erstes) || bild > static_cast<float>(erstes + anzahl)) { return; }
    modell::rigNachMalAuf(g_app->anim, *s, bildA, bildB, anteil, aus, ueber);   // Stufe 3b: mit Bindebasis umgerechnet, Stufe 4: IK
}

namespace {

// Die Welt eines Bilds der gewaehlten Sequenz mit ihrer Schicht.
void weltBei(int bild, std::vector<BoneMatrix>& welt) {
    std::vector<std::pair<int, BoneMatrix>> m;
    modellRigNachMal(bild, bild, 0.0F, m);
    g_app->anim.worldMatricesLerp(bild, bild, 0.0F, {}, m, welt);
}

void ortAus(const BoneMatrix& m, float p[3]) {
    p[0] = m.m[0][3];
    p[1] = m.m[1][3];
    p[2] = m.m[2][3];
}

}  // namespace

bool modellRigGriffWelt(int griff, const std::vector<BoneMatrix>& welt, float ort[3]) {
    const ModellTab& t = mt();
    if (griff < 0 || static_cast<std::size_t>(griff) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
    if (g.knochen < 0 || static_cast<std::size_t>(g.knochen) >= welt.size() || static_cast<std::size_t>(g.knochen) >= g_app->anim.bones.size()) { return false; }
    const BoneMatrix& x = welt[static_cast<std::size_t>(g.knochen)];
    const bool amKnochen = !g.ik && g.art != modell::GriffArt::Saebelgriff;
    if (amKnochen) {
        ortAus(x, ort);
        return true;
    }
    // Stufe 4: der Pol steht, wo die IK-Spur ihn hat
    if (g.art == modell::GriffArt::PolEllbogen || g.art == modell::GriffArt::PolKnie) { return modellIkPolWelt(griff, welt, ort, nullptr); }
    // Fussrolle, Saebelgriff: der Ort der Grundstellung, starr am Knochen (X * B^-1 * Ort)
    float lokal[3];
    g_app->anim.bones[static_cast<std::size_t>(g.knochen)].basePoseInv.transform(g.ort, lokal);
    x.transform(lokal, ort);
    return true;
}

namespace {

// Ein Zug am gewaehlten Griff (Gizmo oder G/R wie Blender) auf die Schicht beim Anfassen: Stufe 4 zuerst die IK (Hand,
// Fuss, Saebelgriff, Pol, Fussrolle; ein Glied unter IK wechselt wie 3ds Max auf FK), sonst FK wie Stufe 3.
void rigZugAnwenden(int gizmoArt, const BoneMatrix& dreh, const float weg[3]) {
    ModellTab& t = mt();
    modell::RigSchicht neu = t.rigSchichtStart;
    neu.neuInterp = modell::Interp::Bezier;   // Stufe 3b: neue Schluessel wie Max und Blender (Bezier, Auto Clamped)
    const int ik = modellIkZiehen(gizmoArt, dreh, weg, neu);
    if (ik != 1) {
        modell::rigGriffDrehen(g_app->anim, t.rig, t.griffWahl, t.rigZiehBild, t.rigWeltStart, dreh, weg, neu);
        modellKurvenAnimOffset(t.rigSchichtStart, neu, auswahlKnochen(), t.rigZiehBild);   // Stufe 3b
    }
    const AnimEntry* e = sequenz();
    const std::string name = e != nullptr ? e->name : std::string();
    t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[name] = neu; }, false);
    g_app->modelDirty = true;
}

}  // namespace

bool modellRigGriffMitte(float m[3]) {
    const ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
    if (g.ik) { return modellIkGriffMitte(t.griffWahl, m); }   // Stufe 4: Pol (W), Fussrolle (E)
    // verschieben: das Becken (FK) und Hand/Fuss/Saebelgriff (IK, Stufe 4); die anderen Griffe drehen
    const bool ende = g.art == modell::GriffArt::Hand || g.art == modell::GriffArt::Fuss || g.art == modell::GriffArt::Saebelgriff;
    if (t.gizmo == 1 && g.art != modell::GriffArt::Becken && !ende) { return false; }
    return modellRigGriffWelt(t.griffWahl, g_app->modellWelt, m);
}

namespace {

// Der erste Zug eines Ziehens: Schicht und Welt beim Anfassen merken, ein Rueckgaengig-Schritt.
void zugBeginnen(int gizmoArt) {
    ModellTab& t = mt();
    if (!t.rigZieht) {
        const modell::RigSchicht* s = modellRigSchichtJetzt();
        t.rigSchichtStart = s != nullptr ? *s : modell::RigSchicht{};
        t.rigWeltStart = g_app->modellWelt;
        t.rigZiehBild = bildJetzt();
        t.rigZieht = true;
        const AnimEntry* e = sequenz();
        const std::string name = e != nullptr ? e->name : std::string();
        t.rigSchritt = t.dok.aendern(tr(gizmoArt == 1 ? Str::MbRgUndoVerschieben : Str::MbRgUndoDrehen), [&](modell::Inhalt& in) { (void)in.rig.schichten[name]; }, false);
        modellZeitZugGemeldet(t.rigSchritt);   // AP1: Auto Key aus - die Pose bleibt ohne Schluessel bis Set Key
        g_app->animPlaying = false;
    }
}

}  // namespace

void modellRigZiehen(int gizmoArt, const float wert[3]) {
    ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return; }
    zugBeginnen(gizmoArt);
    BoneMatrix dreh = BoneMatrix::identity();
    float weg[3] = {0.0F, 0.0F, 0.0F};
    if (gizmoArt == 2) {
        // je Weltachse nacheinander (wie umformen): X, dann Y, dann Z
        for (int k = 0; k < 3; ++k) {
            if (wert[k] == 0.0F) { continue; }
            float ax[3] = {0.0F, 0.0F, 0.0F};
            ax[k] = 1.0F;
            dreh = modell::drehungUm(ax, wert[k]) * dreh;
        }
    } else {
        std::memcpy(weg, wert, sizeof weg);
    }
    rigZugAnwenden(gizmoArt, dreh, weg);
}

void modellRigDrehen(const BoneMatrix& dreh) {
    ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return; }
    zugBeginnen(2);
    const float nichts[3] = {0.0F, 0.0F, 0.0F};
    rigZugAnwenden(2, dreh, nichts);
}

void modellRigZiehenEnde() { mt().rigZieht = false; }

// Kann der gewaehlte Griff diese Art Zug (1 verschieben, 2 drehen, 3 skalieren)? Sonst steht in `grund` die Meldung. Gilt fuer G/R
// und die Seitenleiste (ENTSCHEID1).
bool modellRigZugMoeglich(int art, std::string* grund) {
    const ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
    // Stufe 4: G bewegt auch Hand, Fuss, Saebelgriff und Pol (IK), R rollt an Ferse/Ballen/Zehe
    const bool ende = g.art == modell::GriffArt::Hand || g.art == modell::GriffArt::Fuss || g.art == modell::GriffArt::Saebelgriff;
    const bool pol = g.art == modell::GriffArt::PolEllbogen || g.art == modell::GriffArt::PolKnie;
    if (art == 3 || (art == 1 && g.art != modell::GriffArt::Becken && !ende && !pol) || (art == 2 && pol)) {
        if (grund != nullptr) { *grund = tr(pol ? Str::MbIkPolNurVerschieben : (g.ik ? Str::MbIkRolleNurDrehen : Str::MbRgNurDrehen)); }
        return false;
    }
    // wie Blender (Rigify): G greift das IK-Ziel nur bei IK - sonst eine Meldung statt eines Modals, das nichts tut
    if (art == 1 && (ende || pol) && modellBlender() && modellIkAnteil(modellIkArtFuerGriff(t.griffWahl)) < 0.5F) {
        if (grund != nullptr) {
            char b[400];
            std::snprintf(b, sizeof b, tr(Str::MbIkNurIk), tr(modell::ikArm(static_cast<modell::IkArt>(std::max(modellIkArtFuerGriff(t.griffWahl), 0))) ?
                                                               (g.seite == modell::Seite::Links ? Str::MbIkArmL : Str::MbIkArmR) :
                                                               (g.seite == modell::Seite::Links ? Str::MbIkBeinL : Str::MbIkBeinR)));
            *grund = b;
        }
        return false;
    }
    return true;
}

// G und R wie Blender (transform.translate / transform.rotate im Pose-Modus): Maus nach rechts dreht um die Blickachse der
// aktiven Ansicht (X/Y/Z: Weltachse), Klick/Enter uebernimmt, Esc/rechts bricht ab.
bool modellRigModalStarten(int art) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation || t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return false; }
    std::string grund;
    if (!modellRigZugMoeglich(art, &grund)) {
        melden(grund);
        return true;
    }
    t.rigModal = art;
    t.rigModalAchse = -1;
    t.rigZiehStart[0] = ImGui::GetIO().MousePos.x;
    t.rigZiehStart[1] = ImGui::GetIO().MousePos.y;
    t.rigZieht = false;
    return true;
}

bool modellRigModalTasten() {
    ModellTab& t = mt();
    if (t.rigModal == 0) { return false; }
    for (int k = 0; k < 3; ++k) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_X + k), false)) { t.rigModalAchse = t.rigModalAchse == k ? -1 : k; }
    }
    const ImVec2 m = ImGui::GetIO().MousePos;
    const float dx = m.x - t.rigZiehStart[0];
    const float dy = m.y - t.rigZiehStart[1];
    float wert[3] = {0.0F, 0.0F, 0.0F};
    const ModellAnsicht& a = t.ansichten[t.aktiv];
    // Blickachse und Bildachsen der aktiven Ansicht aus ihrer Matrix (Zeilen 0/1/3 der Projektion)
    const float* vp = a.viewProj;
    float rechts[3] = {vp[0], vp[1], vp[2]};
    float hoch[3] = {vp[4], vp[5], vp[6]};
    float blick[3] = {rechts[1] * hoch[2] - rechts[2] * hoch[1], rechts[2] * hoch[0] - rechts[0] * hoch[2], rechts[0] * hoch[1] - rechts[1] * hoch[0]};
    for (float* v : {rechts, hoch, blick}) {
        const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 1e-6F) {
            for (int k = 0; k < 3; ++k) { v[k] /= l; }
        }
    }
    if (t.rigModal == 2) {
        const float grad = dx * 0.5F;
        if (t.rigModalAchse >= 0) {
            wert[t.rigModalAchse] = grad;
            modellRigZiehen(2, wert);
        } else {
            // um die Blickachse: als Drehung zerlegt in die Weltachsen geht nicht - eigene Drehung
            if (!t.rigZieht) { modellRigZiehen(2, wert); }
            const float nichts[3] = {0.0F, 0.0F, 0.0F};
            rigZugAnwenden(2, modell::drehungUm(blick, grad), nichts);
        }
    } else {
        const float proPx = 2.0F * std::max(a.halbHoehe, 1.0F) / std::max(a.groesse[1], 1.0F);
        for (int k = 0; k < 3; ++k) { wert[k] = (dx * rechts[k] - dy * hoch[k]) * proPx; }
        if (t.rigModalAchse >= 0) {
            for (int k = 0; k < 3; ++k) {
                if (k != t.rigModalAchse) { wert[k] = 0.0F; }
            }
        }
        modellRigZiehen(1, wert);
    }
    const bool fertig = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool abbruch = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    if (fertig || abbruch) {
        if (abbruch && t.rigZieht) { (void)t.dok.abbrechen(t.rigSchritt); }
        t.rigModal = 0;
        t.rigZieht = false;
        g_app->modelDirty = true;
    }
    return true;
}

// --- Posen ----------------------------------------------------------------------------------------------------------------

namespace {

// ENT2: liegt ein Glied am Bild auf IK (nach Control Rig erstellen liegt die Animation von FK auf IK auf dem Rig), folgen Ziel und
// Pol der neuen FK-Pose (ikAufFk mit dem Anteil am Bild) - eine eingefuegte Pose, die Bindepose oder eine Handpose zeigt sich dann
// wie gesetzt, statt von der IK des Glieds verdeckt zu werden
void ikFolgtFk(modell::RigSchicht& s, int bild) {
    for (std::size_t i = 0; i < s.ik.size(); ++i) {
        const float anteil = modell::ikAnteilAm(s.ik[i], static_cast<float>(bild));
        if (!s.ik[i].ziel.empty() && anteil > 0.0F) { modell::ikAufFk(g_app->anim, s, static_cast<int>(i), bild, anteil); }
    }
}

}  // namespace

bool modellPoseKopieren() {
    ModellTab& t = mt();
    if (g_app->modellWelt.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    t.kopie = modell::poseAus(g_app->anim, g_app->modellWelt);
    t.kopieDa = true;
    melden(format(Str::MbRgKopiert, static_cast<int>(t.kopie.namen.size()), modellZeitBildZahl(bildJetzt())));
    return true;
}

bool modellPoseEinfuegen(bool gespiegelt) {
    ModellTab& t = mt();
    if (!t.kopieDa) {
        melden(tr(Str::MbRgKeineKopie));
        return false;
    }
    if (!rigSicher()) { return false; }
    const int bild = bildJetzt();
    int n = 0;
    schichtAendern(gespiegelt ? Str::MbRgGespiegelt : Str::MbRgEinfuegen,
                   [&](modell::RigSchicht& s) {
                       n = modell::poseEinfuegen(g_app->anim, t.rig, t.kopie, gespiegelt, bild, {}, s);
                       ikFolgtFk(s, bild);
                   });
    melden(format(gespiegelt ? Str::MbRgGespiegeltOk : Str::MbRgEingefuegt, n, modellZeitBildZahl(bild)));
    return n > 0;
}

bool modellBindepose(bool alle) {
    if (g_app->anim.bones.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    const std::vector<int> nur = alle ? std::vector<int>{} : auswahlKnochen();
    if (!alle && nur.empty()) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    const int bild = bildJetzt();
    int n = 0;
    schichtAendern(alle ? Str::MbRgBindeAlle : Str::MbRgBindeAuswahl, [&](modell::RigSchicht& s) {
        n = modell::bindePose(g_app->anim, bild, nur, s);
        ikFolgtFk(s, bild);
    });
    melden(format(Str::MbRgBindeOk, n, modellZeitBildZahl(bild)));
    return true;
}

bool modellHandpose(modell::Seite seite, int art) {
    ModellTab& t = mt();
    if (!rigSicher()) { return false; }
    // Posen-Bibliothek: offen = Bindepose, Faust aus BOTH_MELEE1 (Faustschlag), Saebelgriff aus BOTH_STAND1 (Saebel in der
    // Hand) - jeweils das erste Bild der Zeile in der animation.cfg dieser GLA.
    int quelle = -1;
    if (art != 0) {
        const char* name = art == 1 ? "BOTH_MELEE1" : "BOTH_STAND1";
        for (const AnimEntry& e : g_app->animList) {
            if (e.name == name) { quelle = e.firstFrame; }
        }
        if (quelle < 0) {
            melden(format(Str::MbRgHandFehlt, name));
            return false;
        }
    }
    const int bild = bildJetzt();
    int n = 0;
    schichtAendern(Str::MbRgUndoHand, [&](modell::RigSchicht& s) {
        n = modell::handPoseSetzen(g_app->anim, t.rig, seite, quelle, bild, s);
        ikFolgtFk(s, bild);
    });
    if (n == 0) {
        melden(tr(Str::MbRgKeineHand));
        return false;
    }
    melden(format(Str::MbRgHandOk, n, modellZeitBildZahl(bild)));
    return true;
}

bool modellRigSchluesselLoeschen() {
    const std::vector<int> k = auswahlKnochen();
    if (k.empty()) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    const int bild = bildJetzt();
    schichtAendern(Str::MbRgSchluesselLoeschen, [&](modell::RigSchicht& s) {
        for (const int b : k) { modell::rigSchluesselLoeschen(s, b, bild); }
    });
    melden(format(Str::MbRgGeloescht, static_cast<int>(k.size()), modellZeitBildZahl(bild)));
    return true;
}

// --- Auswahl ----------------------------------------------------------------------------------------------------------

namespace {
bool knochenWaehlen(int b) {
    ModellTab& t = mt();
    if (b < 0 || static_cast<std::size_t>(b) >= g_app->anim.bones.size()) {
        melden(tr(Str::MbRgKeinerDa));
        return false;
    }
    t.knochenWahl = b;
    t.griffWahl = t.rig.griffVon(b);
    melden(format(Str::MbRgKnochenGewaehlt, g_app->anim.bones[static_cast<std::size_t>(b)].name.c_str()));
    g_app->modelDirty = true;
    return true;
}
}  // namespace

bool modellAuswahlGegenseite() {
    ModellTab& t = mt();
    if (t.knochenWahl < 0 || !rigSicher()) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    const int p = static_cast<std::size_t>(t.knochenWahl) < t.rig.partner.size() ? t.rig.partner[static_cast<std::size_t>(t.knochenWahl)] : -1;
    return knochenWaehlen(p);
}

bool modellAuswahlEltern() {
    const ModellTab& t = mt();
    if (t.knochenWahl < 0 || static_cast<std::size_t>(t.knochenWahl) >= g_app->anim.bones.size()) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    return knochenWaehlen(g_app->anim.bones[static_cast<std::size_t>(t.knochenWahl)].parent);
}

bool modellAuswahlKinder() {
    const ModellTab& t = mt();
    if (t.knochenWahl < 0) {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
        if (g_app->anim.bones[b].parent == t.knochenWahl) { return knochenWaehlen(static_cast<int>(b)); }
    }
    melden(tr(Str::MbRgKeinerDa));
    return false;
}

// --- Bewegungspfade -----------------------------------------------------------------------------------------------------

bool modellPfadHinzu() {
    ModellTab& t = mt();
    static const float kFarben[6][4] = {{1.0F, 0.85F, 0.2F, 1.0F}, {0.3F, 0.85F, 1.0F, 1.0F}, {1.0F, 0.4F, 0.8F, 1.0F},
                                        {0.5F, 1.0F, 0.4F, 1.0F}, {1.0F, 0.55F, 0.25F, 1.0F}, {0.75F, 0.6F, 1.0F, 1.0F}};
    ModellPfad p;
    if (t.tagWahl >= 0 && t.knochenWahl < 0) {
        p.tag = t.tagWahl;
    } else if (t.knochenWahl >= 0) {
        p.knochen = t.knochenWahl;
    } else if (t.tagWahl >= 0) {
        p.tag = t.tagWahl;
    } else {
        melden(tr(Str::MbRgKeineAuswahl));
        return false;
    }
    for (const ModellPfad& x : t.pfade) {
        if (x.knochen == p.knochen && x.tag == p.tag) {
            melden(tr(Str::MbRgPfadDa));
            return false;
        }
    }
    std::memcpy(p.farbe, kFarben[t.pfade.size() % 6U], sizeof p.farbe);
    t.pfade.push_back(p);
    const std::string name = p.tag >= 0 ? t.dok.inhalt().teile[static_cast<std::size_t>(p.tag)].name : g_app->anim.bones[static_cast<std::size_t>(p.knochen)].name;
    melden(format(Str::MbRgPfadNeu, name.c_str()));
    g_app->modelDirty = true;
    return true;
}

void modellPfadeLeeren() {
    mt().pfade.clear();
    g_app->modelDirty = true;
}

// --- Backen -----------------------------------------------------------------------------------------------------------

bool modellRigBacken() {
    const AnimEntry* e = sequenz();
    if (g_app->anim.empty() || e == nullptr) {
        melden(tr(Str::MbRgKeineSequenz));
        return false;
    }
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const modell::RigSchicht leer;
    GlaAnimation g;
    modell::BackBericht br;
    if (!modell::rigBacken(g_app->anim, s != nullptr ? *s : leer, e->firstFrame, e->numFrames, false, modellZielKopf(), g, &br)) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehler).c_str()));
        return false;
    }
    const std::string name = e->name + "_rig";
    const std::string platz = modellGlaAufPlatz(std::move(g), name, e->loopFrame, e->fps);
    if (platz.empty()) { return false; }
    melden(format(Str::MbRgGebacken, e->name.c_str(), br.bilder, br.neu, br.unveraendert, platz.c_str()));
    return true;
}

// --- Ghosts ----------------------------------------------------------------------------------------------------------

std::vector<gpu::ModellGeist> modellGeister() {
    ModellTab& t = mt();
    std::vector<gpu::ModellGeist> aus;
    t.geisterGezeichnet = 0;
    if (!t.geister.an || g_app->anim.empty() || g_app->modellWelt.empty()) { return aus; }
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const int jetzt = bildJetzt();
    std::vector<BoneMatrix> welt;
    for (const int richtung : {-1, 1}) {
        const int zahl = richtung < 0 ? t.geister.davor : t.geister.danach;
        for (int k = 1; k <= zahl; ++k) {
            const int f = jetzt + richtung * k * std::max(t.geister.abstand, 1);
            if (f < erstes || f >= erstes + anzahl) { break; }
            weltBei(f, welt);
            gpu::ModellGeist gg;
            gpu::baueKnochenmatrizen(welt, g_app->anim.bones, gg.knochen);
            std::memcpy(gg.farbe, richtung < 0 ? t.geister.farbeDavor : t.geister.farbeDanach, sizeof gg.farbe);
            aus.push_back(std::move(gg));
        }
    }
    t.geisterGezeichnet = static_cast<int>(aus.size());
    return aus;
}

// --- Ansicht: Griffe und Pfade ------------------------------------------------------------------------------------------

bool modellRigGriffAufSchirm(int griff, ImVec2& out) {
    float p[3];
    const ModellTab& t = mt();
    return modellRigGriffWelt(griff, g_app->modellWelt, p) && modellAufSchirm(t.ansichten[t.aktiv], p, out);
}

int modellRigGriffUnter(const ModellAnsicht& a, ImVec2 maus) {
    const ModellTab& t = mt();
    if (!modellRigDa() || !t.rigZeigen) { return -1; }
    if (modellKdGriffeAlsFormen()) { return modellKdGriffUnter(a, maus); }   // AP13K: in den Rig-Stilen trifft die sichtbare Form
    const float reich = ImGui::GetFontSize() * 0.7F;
    int best = -1;
    float bd = reich;
    for (std::size_t i = 0; i < t.rig.griffe.size(); ++i) {
        if (i < t.griffVersteckt.size() && t.griffVersteckt[i] != 0) { continue; }
        float p[3];
        ImVec2 s;
        if (!modellRigGriffWelt(static_cast<int>(i), g_app->modellWelt, p) || !modellAufSchirm(a, p, s)) { continue; }
        const float d = std::hypot(maus.x - s.x, maus.y - s.y);
        if (d < bd) {
            bd = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

namespace {

void pfadZeichnen(const ModellAnsicht& a, ImDrawList* dl, const ModellPfad& p) {
    ModellTab& t = mt();
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const int jetzt = bildJetzt();
    const int von = std::max(erstes, jetzt - t.pfadDavor);
    const int bis = std::min(erstes + anzahl - 1, jetzt + t.pfadDanach);
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const ImU32 farbe = ImGui::ColorConvertFloat4ToU32(ImVec4{p.farbe[0], p.farbe[1], p.farbe[2], p.farbe[3]});
    const ImU32 blass = ImGui::ColorConvertFloat4ToU32(ImVec4{p.farbe[0], p.farbe[1], p.farbe[2], p.farbe[3] * 0.55F});
    int knochen = p.knochen;
    if (p.tag >= 0) {
        // Stufe 4: der Ort des Tags ist sein Bolzen (boltMatrix wie G2_ProcessSurfaceBolt2), mit Saebel dessen Spitze;
        // die Schluessel zeigt der Knochen des Tags
        const modell::Inhalt& in = t.dok.inhalt();
        if (static_cast<std::size_t>(p.tag) >= in.teile.size() || in.teile[static_cast<std::size_t>(p.tag)].stufen.empty()) { return; }
        const modell::Netz& n = in.teile[static_cast<std::size_t>(p.tag)].stufen[0];
        if (n.ecken.empty()) { return; }
        knochen = n.ecken[0].knochen[0];
    }
    if (knochen < 0 || static_cast<std::size_t>(knochen) >= g_app->anim.bones.size()) { return; }
    std::vector<BoneMatrix> welt;
    ImVec2 vor;
    bool hatVor = false;
    for (int f = von; f <= bis; ++f) {
        weltBei(f, welt);
        float w[3];
        if (p.tag >= 0) {
            if (!modellSaebelSpitze(p, welt, w)) { continue; }   // Stufe 4: Bolzen des Tags bzw. Spitze des angebauten Saebels
        } else {
            ortAus(welt[static_cast<std::size_t>(knochen)], w);
        }
        ImVec2 sp;
        if (!modellAufSchirm(a, w, sp)) {
            hatVor = false;
            continue;
        }
        if (hatVor) { dl->AddLine(vor, sp, f <= jetzt ? farbe : blass, 2.0F); }
        const bool schluessel = s != nullptr && s->hatSchluessel(knochen, f);
        if (schluessel) {
            dl->AddRectFilled(ImVec2{sp.x - 4.0F, sp.y - 4.0F}, ImVec2{sp.x + 4.0F, sp.y + 4.0F}, IM_COL32(255, 255, 255, 255));
            dl->AddRect(ImVec2{sp.x - 4.0F, sp.y - 4.0F}, ImVec2{sp.x + 4.0F, sp.y + 4.0F}, farbe, 0.0F, 0, 1.5F);
            ++t.pfadSchluessel;
        } else {
            dl->AddCircleFilled(sp, f == jetzt ? 4.5F : 2.2F, f == jetzt ? IM_COL32(255, 255, 255, 255) : farbe);
        }
        ++t.pfadPunkte;
        vor = sp;
        hatVor = true;
    }
    if (p.spitze) {
        // Stufe 4: die Klinge am aktuellen Frame als Linie (das Modell des Griffs zeigt sie nicht), damit die Spitze zu sehen ist
        weltBei(jetzt, welt);
        float sp3[3];
        float an3[3];
        ImVec2 s0;
        ImVec2 s1;
        if (modellSaebelSpitze(p, welt, sp3, an3) && modellAufSchirm(a, an3, s0) && modellAufSchirm(a, sp3, s1)) { dl->AddLine(s0, s1, blass, 3.0F); }
    }
}

}  // namespace

void modellRigUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation) { return; }
    modellRigAktualisieren();
    for (const ModellPfad& p : t.pfade) { pfadZeichnen(a, dl, p); }
    modellPhysikAktualisieren();         // Stufe 6: die Physik-Spur nach jeder Aenderung des Dokuments
    modellPhysikUeberlagerung(a, dl);    // Stufe 6: Schwerpunkt, Weg, Stuetzflaeche
    modellRtUeberlagerung(a, dl);        // Stufe 5: die Quelle des Retargetings neben der Figur
    if (!modellRigDa() || !t.rigZeigen) { return; }
    if (modellKdGriffeAlsFormen()) {   // AP13K: in den Rig-Stilen stehen Griffe und IK-Ziele als Formen im Bild der Ansicht
        modellKdUeberlagerung(a, dl);  // ... hier nur IK/FK am Glied (Control Rig)
        return;
    }
    modellIkUeberlagerung(a, dl);   // Stufe 4: IK-Ziele, Raum
    const float r = ImGui::GetFontSize() * 0.45F;
    const bool blender = modellBlender();
    for (std::size_t i = 0; i < t.rig.griffe.size(); ++i) {
        if (i < t.griffVersteckt.size() && t.griffVersteckt[i] != 0) { continue; }
        const modell::Griff& g = t.rig.griffe[i];
        float p[3];
        ImVec2 s;
        if (!modellRigGriffWelt(static_cast<int>(i), g_app->modellWelt, p) || !modellAufSchirm(a, p, s)) { continue; }
        const bool gewaehlt = t.griffWahl == static_cast<int>(i);
        const ImU32 farbe = gewaehlt ? IM_COL32(255, 255, 255, 255) : seitenFarbe(g.seite, g.ik ? 0.6F : 0.95F);
        const float dicke = gewaehlt ? 2.5F : 1.5F;
        switch (g.art) {
            case modell::GriffArt::PolEllbogen:
            case modell::GriffArt::PolKnie: {   // Raute, gestrichelte Linie zum Gelenk
                dl->AddQuad(ImVec2{s.x, s.y - r}, ImVec2{s.x + r, s.y}, ImVec2{s.x, s.y + r}, ImVec2{s.x - r, s.y}, farbe, dicke);
                float j[3];
                ImVec2 sj;
                if (static_cast<std::size_t>(g.knochen) < g_app->modellWelt.size()) {
                    ortAus(g_app->modellWelt[static_cast<std::size_t>(g.knochen)], j);
                    if (modellAufSchirm(a, j, sj)) {
                        for (int k = 0; k < 8; k += 2) {
                            const float u0 = static_cast<float>(k) / 8.0F;
                            const float u1 = static_cast<float>(k + 1) / 8.0F;
                            dl->AddLine(ImVec2{s.x + (sj.x - s.x) * u0, s.y + (sj.y - s.y) * u0}, ImVec2{s.x + (sj.x - s.x) * u1, s.y + (sj.y - s.y) * u1}, farbe, 1.0F);
                        }
                    }
                }
                break;
            }
            case modell::GriffArt::Ferse:
            case modell::GriffArt::Ballen:
            case modell::GriffArt::Zehe:   // Dreieck auf der Spitze (Drehpunkt der Fussrolle)
                dl->AddTriangle(ImVec2{s.x, s.y + r * 0.8F}, ImVec2{s.x - r * 0.8F, s.y - r * 0.6F}, ImVec2{s.x + r * 0.8F, s.y - r * 0.6F}, farbe, dicke);
                break;
            case modell::GriffArt::Saebelgriff:   // Kreis mit Kreuz (Griffpunkt des Saebels)
                dl->AddCircle(s, r * 0.8F, farbe, 16, dicke);
                dl->AddLine(ImVec2{s.x - r, s.y}, ImVec2{s.x + r, s.y}, farbe, dicke);
                dl->AddLine(ImVec2{s.x, s.y - r}, ImVec2{s.x, s.y + r}, farbe, dicke);
                break;
            default:
                if (blender) {   // wie Blender: Ring (eigene Form)
                    dl->AddCircle(s, r * (g.art == modell::GriffArt::Becken ? 1.6F : 1.1F), farbe, 24, dicke);
                } else {         // wie 3ds Max: Kasten
                    const float h = r * (g.art == modell::GriffArt::Becken ? 1.5F : 1.0F);
                    dl->AddRect(ImVec2{s.x - h, s.y - h}, ImVec2{s.x + h, s.y + h}, farbe, 0.0F, 0, dicke);
                }
                break;
        }
        ++t.griffeGezeichnet;
    }
}

// --- Szenen-Explorer: die Griffe als Zeilen des Baums (gezeichnet in gui/app_modelltab_explorer.cpp) ----------------

int modellRigExplorerZahl() { return modellRigDa() ? static_cast<int>(mt().rig.griffe.size()) : 0; }

std::string modellRigExplorerName(int griff) { return griffName(griff); }

void modellRigExplorerSymbol(int griff, UiIcon& icon, ImU32& farbe) {
    if (griff < 0 || static_cast<std::size_t>(griff) >= mt().rig.griffe.size()) { return; }   // FIX612C (P3-7)
    const modell::Griff& g = mt().rig.griffe[static_cast<std::size_t>(griff)];
    icon = g.ik ? UiIcon::MbPol : UiIcon::MbGriff;
    farbe = seitenFarbe(g.seite);
}

// Der Tooltip der Zeile: der Knochen des Griffs und was der Griff tut.
std::string modellRigExplorerHinweis(int griff) {
    if (griff < 0 || static_cast<std::size_t>(griff) >= mt().rig.griffe.size()) { return {}; }   // FIX612C (P3-7)
    const modell::Griff& g = mt().rig.griffe[static_cast<std::size_t>(griff)];
    return (g.knochen >= 0 ? g_app->anim.bones[static_cast<std::size_t>(g.knochen)].name : std::string("-")) + "\n" + tr(griffHinweis(g));
}

void modellRigExplorerKopfMenue() { aktionenAlsMenue(0); }

void modellRigExplorerGriffMenue(int griff) {
    modellRigGriffWaehlen(griff);
    aktionenAlsMenue(1);
    ImGui::Separator();
    aktionenAlsMenue(2);
    if (modellIkArtFuerGriff(griff) >= 0) {   // Stufe 4: IK/FK als Untermenue wie im Menue Animation
        ImGui::Separator();
        if (ImGui::BeginMenu(tr(Str::MbIkGruppe))) {
            aktionenAlsMenue(5);
            ImGui::EndMenu();
        }
        ImGui::SetItemTooltip("%s", tr(Str::MbIkRolloutHint));
    }
    ImGui::Separator();   // Stufe 6: Auto-Physik (Federkette ab diesem Knochen)
    if (ImGui::BeginMenu(tr(Str::PhRollout))) {
        aktionenAlsMenue(6);
        ImGui::EndMenu();
    }
    ImGui::SetItemTooltip("%s", tr(Str::PhRolloutHint));
}

// --- Befehlsfeld ----------------------------------------------------------------------------------------------------
// AP10: die Gruppen des Bereichs Animation stehen in der Reihenfolge des freigegebenen Entwurfs (app_modelltab_anim.cpp,
// drawModellAnimFeld) und zeichnen ueber den Baustein gui/ui_rollout.h.

namespace {

// Die Taste einer Aktion im laufenden Bedienmodus fuer das Tastenfeld des Knopfs (nullptr: keine).
const char* tasteVon(Str text) {
    for (const RigAktion& a : rigAktionen()) {
        if (a.text != text) { continue; }
        const char* k = modellBlender() ? a.blenderTaste : a.maxTaste;
        return k[0] != '\0' ? k : nullptr;
    }
    return nullptr;
}

}  // namespace

void drawModellRigBereich() {
    modellRigAktualisieren();
    drawModellAnimFeld();
    modellKdRollout();   // AP13K: Knochen- und Rig-Darstellung (ab Werk zugeklappt)
}

// Entwurf A2: "Rig erstellen" - Skelett, Massstab und erkannte Vorlage als Tabelle, die Auswahl der Vorlage, der Hauptknopf
// "Control Rig erstellen", daneben "Animation-Rig (nur FK)" und "Auf Rig legen"; darunter, was die Gruppe bis rc611 zeigte
// (Rig-Uebersicht, Stand der Erkennung, Griffe zeigen, der gewaehlte Griff, Schluessel).
void modellAnimGruppeRig() {
    ModellTab& t = mt();
    const bool rig = modellRigDa();
    const bool nurFk = modellRigNurFk();
    static const int kVorlagenZahl = static_cast<int>(modell::rtVorlagen().size());
    char info[64];
    std::snprintf(info, sizeof info, trN(Str::AzVorlagenInfo, kVorlagenZahl), kVorlagenZahl);
    if (!ui::rolloutBeginn(tr(Str::AzRRig), {.kurzinfo = info, .reiter = modellReiter(MbReiter::Hierarchie, MbReiterBl::Knochen)})) { return; }
    drawModellAnimVorlage();
    const bool gla = !g_app->anim.bones.empty();
    // steht das Control Rig schon, erkennt der Hauptknopf neu (wie "Re-Generate Rig")
    if (ui::werkzeugKnopf(tr(rig && !nurFk ? Str::MbRgNeu : Str::AzRigErstellen),
                          {.icon = UiIcon::MbRig, .hinweis = tr(Str::AzRigErstellenHint), .haupt = true, .aktiv = gla})) {
        (void)modellAnimRigErstellen(false);
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::AzRigFk), {.icon = UiIcon::MbKnochen, .hinweis = tr(Str::AzRigFkHint), .an = rig && nurFk, .aktiv = gla})) {
        (void)modellAnimRigErstellen(true);
    }
    if (ui::werkzeugKnopf(tr(Str::AzAufRig), {.icon = UiIcon::Folgen, .hinweis = tr(Str::AzAufRigHint), .aktiv = gla && sequenz() != nullptr})) {
        (void)modellAnimAufRigLegen();
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgUebersicht), {.icon = UiIcon::Figur, .hinweis = tr(Str::MbRgUebersichtHint), .an = t.uebersichtOffen, .aktiv = rig})) {
        t.uebersichtOffen = !t.uebersichtOffen;
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgSchluesselLoeschen), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::MbRgSchluesselLoeschenHint), .aktiv = rig})) {
        (void)modellRigSchluesselLoeschen();
    }
    ui::rasterEnde();
    if (rig) {
        int ik = 0;
        for (const modell::Griff& g : t.rig.griffe) { ik += g.ik ? 1 : 0; }
        ui::absatz(format(Str::MbRgStand, static_cast<int>(t.rig.griffe.size()) - ik, ik, t.rig.erkennung.erkannt, static_cast<int>(g_app->anim.bones.size()),
                          t.rig.erkennung.unklar)
                       .c_str());
        if (t.rig.erkennung.unklar > 0) { ui::absatz(format(Str::MbRgUnklarHinweis, t.rig.erkennung.unklar).c_str(), ui::Ton::Warnung); }
        if (ui::schalter(tr(Str::MbRgGriffeZeigen), &t.rigZeigen, tr(Str::MbRgGriffeZeigenHint))) { g_app->modelDirty = true; }
        if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size()) {
            const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
            ui::absatz(griffName(t.griffWahl).c_str(), ui::Ton::Normal);
            ui::absatz(tr(griffHinweis(g)));
            if (g.twist >= 0) {
                ui::absatz(format(Str::MbRgTwistInfo, g_app->anim.bones[static_cast<std::size_t>(g.twist)].name.c_str(), static_cast<double>(g.twistAnteil * 100.0F)).c_str());
            }
        } else {
            ui::absatz(tr(Str::MbRgGriffWaehlen));
        }
        const modell::RigSchicht* s = modellRigSchichtJetzt();
        ui::absatz(format(Str::MbRgSchluesselZahl, s != nullptr ? s->schluesselZahl() : 0).c_str());
    } else {
        ui::absatz(tr(Str::MbRgNochKeins));
    }
    ui::tipp(tr(Str::AzRigTipp));
    ui::rolloutEnde();
}

// Pose, Ghosts und Bewegungspfade: mehr als der Entwurf zeigt (Zeile A14) - ab Werk zugeklappt, Platz nur, wenn gebraucht.
void modellAnimGruppePose() {
    ModellTab& t = mt();
    if (!ui::rolloutBeginn(tr(Str::MbRgGruppePose), {.zu = true})) { return; }
    const bool anim = !g_app->anim.bones.empty();
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbRgKopieren), {.icon = UiIcon::Duplizieren, .taste = tasteVon(Str::MbRgKopieren), .hinweis = tr(Str::MbRgKopierenHint), .aktiv = anim})) {
        (void)modellPoseKopieren();
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgEinfuegen), {.icon = UiIcon::Plus, .taste = tasteVon(Str::MbRgEinfuegen), .hinweis = tr(Str::MbRgEinfuegenHint), .aktiv = t.kopieDa})) {
        (void)modellPoseEinfuegen(false);
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgGespiegelt), {.icon = UiIcon::Tauschen, .taste = tasteVon(Str::MbRgGespiegelt), .hinweis = tr(Str::MbRgGespiegeltHint), .aktiv = t.kopieDa})) {
        (void)modellPoseEinfuegen(true);
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgBindeAuswahl), {.icon = UiIcon::Rueckgaengig, .taste = tasteVon(Str::MbRgBindeAuswahl), .hinweis = tr(Str::MbRgBindeAuswahlHint), .aktiv = anim})) {
        (void)modellBindepose(false);
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgBindeAlle), {.icon = UiIcon::Figur, .hinweis = tr(Str::MbRgBindeAlleHint), .aktiv = anim})) { (void)modellBindepose(true); }
    ui::rasterEnde();
    // Handposen: je Seite eine Reihe Offen / Faust / Saebel (aus den Animationen des Spiels)
    ui::absatz(tr(Str::MbRgHandposen));
    static const Str kArten[3] = {Str::MbRgOffen, Str::MbRgFaust, Str::MbRgSaebel};
    ImGui::BeginDisabled(!anim);
    for (const modell::Seite se : {modell::Seite::Links, modell::Seite::Rechts}) {
        const bool links = se == modell::Seite::Links;
        const std::string texte[3] = {std::string(tr(kArten[0])) + (links ? "##hl" : "##hr"), std::string(tr(kArten[1])) + (links ? "##hl" : "##hr"),
                                      std::string(tr(kArten[2])) + (links ? "##hl" : "##hr")};
        const char* const zeiger[3] = {texte[0].c_str(), texte[1].c_str(), texte[2].c_str()};
        const char* const hinweise[3] = {tr(Str::MbRgHandHint), tr(Str::MbRgHandHint), tr(Str::MbRgHandHint)};
        ui::zeile(tr(links ? Str::MbRgL : Str::MbRgR), tr(Str::MbRgHandHint));
        int art = -1;   // drei Ausloeser, keine Wahl: nichts ist gedrueckt
        if (ui::segment(links ? "##handl" : "##handr", zeiger, 3, &art, 3, -1, hinweise, ui::feldBreite()) && art >= 0) { (void)modellHandpose(se, art); }
    }
    ImGui::EndDisabled();
    ui::absatz(tr(Str::MbRgGruppeAuswahl));
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbRgGegenseite), {.icon = UiIcon::Tauschen, .taste = tasteVon(Str::MbRgGegenseite), .hinweis = tr(Str::MbRgGegenseiteHint), .aktiv = anim})) {
        (void)modellAuswahlGegenseite();
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgEltern), {.icon = UiIcon::Zurueck, .taste = tasteVon(Str::MbRgEltern), .hinweis = tr(Str::MbRgElternHint), .aktiv = anim})) {
        (void)modellAuswahlEltern();
    }
    if (ui::werkzeugKnopf(tr(Str::MbRgKinder), {.icon = UiIcon::Vor, .taste = tasteVon(Str::MbRgKinder), .hinweis = tr(Str::MbRgKinderHint), .aktiv = anim})) {
        (void)modellAuswahlKinder();
    }
    ui::rasterEnde();
    ui::rolloutEnde();
}

void modellAnimGruppeGeister() {
    ModellTab& t = mt();
    if (!ui::rolloutBeginn(tr(Str::MbRgGhosts), {.zu = true, .reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Daten)})) { return; }
    ModellGeisterEinst& ge = t.geister;
    ui::rasterBeginn(2);
    bool neu = ui::schalter(tr(Str::MbRgGhostsAn), &ge.an, tr(Str::MbRgGhostsHint));
    neu = ui::schalter(tr(Str::MbRgSilhouette), &t.silhouette, tr(Str::MbRgSilhouetteHint)) || neu;
    ui::rasterEnde();
    ImGui::BeginDisabled(!ge.an);
    neu = ui::reglerInt(tr(Str::MbRgDavor), &ge.davor, 0, 10, "%d", tr(Str::MbRgDavorHint)) || neu;
    neu = ui::reglerInt(tr(Str::MbRgDanach), &ge.danach, 0, 10, "%d", tr(Str::MbRgDavorHint)) || neu;
    neu = ui::reglerInt(tr(Str::MbRgAbstand), &ge.abstand, 1, 20, "%d", tr(Str::MbRgAbstandHint)) || neu;
    ui::zeile(tr(Str::MbRgFarbeDavor), tr(Str::MbRgFarbeHint));
    neu = ImGui::ColorEdit4("##geistdavor", ge.farbeDavor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar) || neu;
    ImGui::SetItemTooltip("%s", tr(Str::MbRgFarbeHint));
    ui::zeile(tr(Str::MbRgFarbeDanach), tr(Str::MbRgFarbeHint));
    neu = ImGui::ColorEdit4("##geistdanach", ge.farbeDanach, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar) || neu;
    ImGui::SetItemTooltip("%s", tr(Str::MbRgFarbeHint));
    ImGui::EndDisabled();
    if (neu) { g_app->modelDirty = true; }
    ui::rolloutEnde();
}

void modellAnimGruppePfade() {
    ModellTab& t = mt();
    char info[16];
    std::snprintf(info, sizeof info, "%d", static_cast<int>(t.pfade.size()));
    if (!ui::rolloutBeginn(tr(Str::MbRgPfade), {.kurzinfo = t.pfade.empty() ? nullptr : info, .zu = true, .reiter = modellReiter(MbReiter::Bewegung, MbReiterBl::Daten)})) {
        return;
    }
    const bool anim = !g_app->anim.bones.empty();
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbRgPfadHinzu), {.icon = UiIcon::Pfad, .hinweis = tr(Str::MbRgPfadHinzuHint), .aktiv = anim})) { (void)modellPfadHinzu(); }
    if (ui::werkzeugKnopf(tr(Str::MbIkPfadSpitze), {.icon = UiIcon::Zielen, .hinweis = tr(Str::MbIkPfadSpitzeHint), .aktiv = anim})) { (void)modellPfadSpitzeHinzu(); }
    if (ui::werkzeugKnopf(tr(Str::MbRgPfadeLeeren), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::MbRgPfadeLeerenHint), .aktiv = !t.pfade.empty()})) { modellPfadeLeeren(); }
    ui::rasterEnde();
    bool neu = ui::reglerInt(tr(Str::MbRgPfadDavor), &t.pfadDavor, 0, 120, "%d", tr(Str::MbRgPfadBereichHint));
    neu = ui::reglerInt(tr(Str::MbRgPfadDanach), &t.pfadDanach, 0, 120, "%d", tr(Str::MbRgPfadBereichHint)) || neu;
    int weg = -1;
    for (std::size_t i = 0; i < t.pfade.size(); ++i) {
        ModellPfad& p = t.pfade[i];
        ImGui::PushID(static_cast<int>(i));
        neu = ImGui::ColorEdit4("##farbe", p.farbe, ImGuiColorEditFlags_NoInputs) || neu;
        ImGui::SameLine();
        const modell::Inhalt& in = t.dok.inhalt();
        const std::string name = p.tag >= 0 && static_cast<std::size_t>(p.tag) < in.teile.size() ? in.teile[static_cast<std::size_t>(p.tag)].name + (p.spitze ? std::string(" ") + tr(Str::MbIkSpitzeName) : std::string())
                                 : p.knochen >= 0 && static_cast<std::size_t>(p.knochen) < g_app->anim.bones.size()
                                     ? g_app->anim.bones[static_cast<std::size_t>(p.knochen)].name
                                     : std::string("-");
        if (ui::eintrag(name.c_str(), tr(Str::MbRgPfadWeg))) { weg = static_cast<int>(i); }
        ImGui::PopID();
    }
    if (weg >= 0) {
        t.pfade.erase(t.pfade.begin() + weg);
        neu = true;
    }
    if (neu) { g_app->modelDirty = true; }
    ui::rolloutEnde();
}

// --- Uebersicht: die Figur zum Anklicken ------------------------------------------------------------------------------

namespace {

struct Fleck {
    modell::Rolle rolle;
    modell::Seite seite;
    float x, y;   // 0..1 im Feld; die Figur schaut den Betrachter an (ihre rechte Seite links im Bild)
};

const std::vector<Fleck>& flecken() {
    using R = modell::Rolle;
    using S = modell::Seite;
    static const std::vector<Fleck> kListe = [] {
        std::vector<Fleck> f = {{R::Kopf, S::Mitte, 0.50F, 0.04F},  {R::Gesicht, S::Mitte, 0.50F, 0.11F}, {R::Hals, S::Mitte, 0.50F, 0.18F},
                                {R::Wirbel, S::Mitte, 0.50F, 0.30F}, {R::Becken, S::Mitte, 0.50F, 0.46F},  {R::Bewegung, S::Mitte, 0.50F, 0.52F},
                                {R::Wurzel, S::Mitte, 0.50F, 0.97F}, {R::Anhang, S::Mitte, 0.62F, 0.57F}};
        const Fleck rechts[] = {{R::Schluessel, S::Rechts, 0.42F, 0.21F},  {R::Oberarm, S::Rechts, 0.33F, 0.24F},   {R::OberarmTwist, S::Rechts, 0.29F, 0.30F},
                                {R::Unterarm, S::Rechts, 0.25F, 0.36F},    {R::UnterarmTwist, S::Rechts, 0.21F, 0.42F}, {R::Hand, S::Rechts, 0.17F, 0.48F},
                                {R::Finger, S::Rechts, 0.13F, 0.55F},      {R::TagKnochen, S::Rechts, 0.22F, 0.54F}, {R::Oberschenkel, S::Rechts, 0.44F, 0.52F},
                                {R::OberschenkelTwist, S::Rechts, 0.43F, 0.62F}, {R::Unterschenkel, S::Rechts, 0.42F, 0.72F}, {R::Fuss, S::Rechts, 0.41F, 0.87F},
                                {R::Zehen, S::Rechts, 0.37F, 0.92F},       {R::Gesicht, S::Rechts, 0.41F, 0.09F},    {R::Anhang, S::Rechts, 0.38F, 0.60F}};
        for (const Fleck& x : rechts) {
            f.push_back(x);
            f.push_back(Fleck{x.rolle, S::Links, 1.0F - x.x, x.y});
        }
        return f;
    }();
    return kListe;
}

// die Linien der Figur: Paare von Flecken (Rolle, Seite)
void figurLinien(ImDrawList* dl, const std::function<ImVec2(modell::Rolle, modell::Seite)>& ort, ImU32 farbe) {
    using R = modell::Rolle;
    using S = modell::Seite;
    const auto l = [&](R a, S sa, R b, S sb) { dl->AddLine(ort(a, sa), ort(b, sb), farbe, 2.0F); };
    l(R::Kopf, S::Mitte, R::Hals, S::Mitte);
    l(R::Hals, S::Mitte, R::Wirbel, S::Mitte);
    l(R::Wirbel, S::Mitte, R::Becken, S::Mitte);
    for (const S s : {S::Links, S::Rechts}) {
        l(R::Hals, S::Mitte, R::Schluessel, s);
        l(R::Schluessel, s, R::Oberarm, s);
        l(R::Oberarm, s, R::Unterarm, s);
        l(R::Unterarm, s, R::Hand, s);
        l(R::Hand, s, R::Finger, s);
        l(R::Becken, S::Mitte, R::Oberschenkel, s);
        l(R::Oberschenkel, s, R::Unterschenkel, s);
        l(R::Unterschenkel, s, R::Fuss, s);
        l(R::Fuss, s, R::Zehen, s);
    }
}

std::vector<int> knochenMit(modell::Rolle r, modell::Seite s) {
    std::vector<int> k;
    const auto& e = mt().rig.erkennung.knochen;
    for (std::size_t b = 0; b < e.size(); ++b) {
        if (e[b].rolle == r && e[b].seite == s) { k.push_back(static_cast<int>(b)); }
    }
    return k;
}

}  // namespace

void drawModellRigUebersicht() {
    ModellTab& t = mt();
    if (!t.uebersichtOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 34.0F, fs * 26.0F}, ImGuiCond_FirstUseEver);
    if (!ui::fensterBegin((std::string(tr(Str::MbRgUebersicht)) + "###riguebersicht").c_str(), &t.uebersichtOffen)) {
        ImGui::End();
        return;
    }
    modellRigAktualisieren();
    if (!modellRigDa()) {
        cjkTextDisabled("%s", tr(Str::MbRgNochKeins));
        ImGui::BeginDisabled(g_app->anim.bones.empty());
        if (ImGui::Button(tr(Str::MbRgAnlegen))) { (void)modellAnimRigErstellen(false); }   // wie der Hauptknopf (ENT2: mit der Animation von FK auf IK)
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("%s", tr(Str::MbRgAnlegenHint));
        ImGui::End();
        return;
    }
    const auto& ek = t.rig.erkennung.knochen;
    // links die Figur
    const float feldB = std::min(ImGui::GetContentRegionAvail().x * 0.48F, ImGui::GetContentRegionAvail().y * 0.9F);
    ImGui::BeginChild("##figur", ImVec2{feldB, -1.0F}, ImGuiChildFlags_Borders);
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    const ImVec2 groesse = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const auto ort = [&](modell::Rolle r, modell::Seite s) {
        for (const Fleck& f : flecken()) {
            if (f.rolle == r && f.seite == s) { return ImVec2{lo.x + f.x * groesse.x, lo.y + f.y * groesse.y}; }
        }
        return lo;
    };
    figurLinien(dl, ort, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    const float r = fs * 0.42F;
    int spot = 0;
    for (const Fleck& f : flecken()) {
        const std::vector<int> k = knochenMit(f.rolle, f.seite);
        const bool unklar = std::any_of(k.begin(), k.end(), [&](int b) { return ek[static_cast<std::size_t>(b)].unklar; });
        const ImVec2 p{lo.x + f.x * groesse.x, lo.y + f.y * groesse.y};
        const ImU32 farbe = k.empty() ? IM_COL32(120, 120, 120, 200) : (unklar ? IM_COL32(240, 200, 40, 255) : IM_COL32(80, 200, 100, 255));
        const int id = static_cast<int>(f.rolle) * 3 + static_cast<int>(f.seite);
        ImGui::SetCursorScreenPos(ImVec2{p.x - r, p.y - r});
        ImGui::PushID(spot++);
        if (ImGui::InvisibleButton("##fleck", ImVec2{r * 2.0F, r * 2.0F})) { t.uebersichtRolle = id; }
        const bool ueber = ImGui::IsItemHovered();
        ImGui::PopID();
        dl->AddCircleFilled(p, r, farbe);
        if (t.uebersichtRolle == id || ueber) { dl->AddCircle(p, r + 2.0F, IM_COL32(255, 255, 255, 255), 0, 2.0F); }
        if (k.size() > 1U) {
            const std::string z = std::to_string(k.size());
            dl->AddText(ImVec2{p.x + r, p.y - r * 1.6F}, ImGui::GetColorU32(ImGuiCol_Text), z.c_str());
        }
        if (ueber) {
            std::string tip = std::string(tr(Str::MbRgRolle)) + ": " + rolleText(f.rolle) +
                              (f.seite == modell::Seite::Links ? std::string(" ") + tr(Str::MbRgL) : f.seite == modell::Seite::Rechts ? std::string(" ") + tr(Str::MbRgR) : std::string());
            for (const int b : k) {
                tip += "\n" + g_app->anim.bones[static_cast<std::size_t>(b)].name;
                if (ek[static_cast<std::size_t>(b)].unklar) { tip += "  (" + std::string(tr(Str::MbRgUnklar)) + ": " + grundText(ek[static_cast<std::size_t>(b)].grund) + ")"; }
            }
            if (k.empty()) { tip += std::string("\n") + tr(Str::MbRgLeer); }
            ImGui::SetTooltip("%s", tip.c_str());
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // rechts: der gewaehlte Fleck mit seinen Knochen, darunter alle unklaren und alle ohne Rolle
    ImGui::BeginChild("##rollen", ImVec2{-1.0F, -1.0F});
    ImGui::PushTextWrapPos(0.0F);
    cjkTextDisabled("%s", tr(Str::MbRgUebersichtHint));
    ImGui::PopTextWrapPos();
    if (t.uebersichtRolle >= 0) {
        const auto rolle = static_cast<modell::Rolle>(t.uebersichtRolle / 3);
        const auto seite = static_cast<modell::Seite>(t.uebersichtRolle % 3);
        ImGui::SeparatorText((std::string(rolleText(rolle)) + (seite == modell::Seite::Links    ? std::string(" ") + tr(Str::MbRgL)
                                                               : seite == modell::Seite::Rechts ? std::string(" ") + tr(Str::MbRgR)
                                                                                                : std::string()))
                                 .c_str());
        for (const int b : knochenMit(rolle, seite)) {
            ImGui::PushID(b);
            if (ImGui::Selectable(g_app->anim.bones[static_cast<std::size_t>(b)].name.c_str(), t.knochenWahl == b)) { (void)knochenWaehlen(b); }
            if (ImGui::BeginPopupContextItem("##k")) {
                if (ImGui::MenuItem(tr(Str::MbRgRolleWeg))) { modellRigZuordnen(b, modell::Rolle::Keine, modell::Seite::Mitte); }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::BeginCombo("##zuordnen", tr(Str::MbRgZuordnen))) {
            for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
                const std::string z = g_app->anim.bones[b].name + "  (" + rolleText(ek[b].rolle) + ")";
                if (ImGui::Selectable(z.c_str())) { modellRigZuordnen(static_cast<int>(b), rolle, seite); }
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("%s", tr(Str::MbRgZuordnenHint));
    }
    ImGui::SeparatorText(tr(Str::MbRgUnklarListe));
    for (std::size_t b = 0; b < ek.size(); ++b) {
        if (!ek[b].unklar && ek[b].rolle != modell::Rolle::Keine) { continue; }
        ImGui::PushID(static_cast<int>(20000 + b));
        const std::string z = g_app->anim.bones[b].name + "  " + rolleText(ek[b].rolle) + (ek[b].unklar ? std::string("  (") + grundText(ek[b].grund) + ")" : std::string());
        if (ImGui::Selectable(z.c_str(), t.knochenWahl == static_cast<int>(b))) {
            (void)knochenWaehlen(static_cast<int>(b));
            // ein gewaehlter Fleck nimmt den Knochen auf
            if (t.uebersichtRolle >= 0) {
                modellRigZuordnen(static_cast<int>(b), static_cast<modell::Rolle>(t.uebersichtRolle / 3), static_cast<modell::Seite>(t.uebersichtRolle % 3));
            }
        }
        ImGui::SetItemTooltip("%s", tr(Str::MbRgZuordnenHint));
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::End();
}

}  // namespace bhed::gui
