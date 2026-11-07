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
    int gruppe;                 // 0 Rig, 1 Pose, 2 Auswahl, 3 Anzeige, 4 Backen, 5 IK/FK (Stufe 4), 6 Auto-Physik (Stufe 6)
};

int artJetztRig() {
    const ModellTab& t = mt();
    if (t.ikWahl >= 0) { return t.ikWahl; }
    return modellIkArtFuerGriff(t.griffWahl);
}

const std::vector<RigAktion>& rigAktionen() {
    static const std::vector<RigAktion> kListe = {
        {Str::MbRgAnlegen, Str::MbRgAnlegenHint, "", "", 0, 0, "", [] { (void)modellRigAnlegen(); }, {}, 0},
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
            if (ImGui::BeginMenu(tr(gruppe == 5 ? Str::MbIkGruppe : Str::PhRollout))) {
                aktionenAlsMenue(gruppe);
                ImGui::EndMenu();
            }
            ImGui::SetItemTooltip("%s", tr(gruppe == 5 ? Str::MbIkRolloutHint : Str::PhRolloutHint));
            continue;
        }
        if (a.gruppe != gruppe) {
            if (gruppe >= 0) { ImGui::Separator(); }
            gruppe = a.gruppe;
            ImGui::TextDisabled("%s", tr(kGruppen[std::clamp(gruppe, 0, 5)]));
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
        if (ImGui::Selectable(text.c_str())) {
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
    t.rig = modell::rigAnlegen(g_app->anim, in, 0, in.rig.zuordnung);
    t.rigFuer = rigSchluessel();
    t.griffVersteckt.assign(t.rig.griffe.size(), 0);
    if (t.griffWahl >= static_cast<int>(t.rig.griffe.size())) { t.griffWahl = -1; }
    int ik = 0;
    for (const modell::Griff& g : t.rig.griffe) { ik += g.ik ? 1 : 0; }
    melden(format(Str::MbRgAngelegt, static_cast<int>(t.rig.griffe.size()) - ik, ik, t.rig.erkennung.erkannt, static_cast<int>(g_app->anim.bones.size()),
                  t.rig.erkennung.unklar));
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
    const int wahl = t.griffWahl;
    (void)modellRigAnlegen();
    t.griffWahl = wahl < static_cast<int>(t.rig.griffe.size()) ? wahl : -1;
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

void modellRigZiehen(int gizmoArt, const float wert[3]) {
    ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return; }
    if (!t.rigZieht) {
        const modell::RigSchicht* s = modellRigSchichtJetzt();
        t.rigSchichtStart = s != nullptr ? *s : modell::RigSchicht{};
        t.rigWeltStart = g_app->modellWelt;
        t.rigZiehBild = bildJetzt();
        t.rigZieht = true;
        const AnimEntry* e = sequenz();
        const std::string name = e != nullptr ? e->name : std::string();
        t.dok.aendern(tr(gizmoArt == 1 ? Str::MbRgUndoVerschieben : Str::MbRgUndoDrehen), [&](modell::Inhalt& in) { (void)in.rig.schichten[name]; }, false);
        g_app->animPlaying = false;
    }
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

void modellRigZiehenEnde() { mt().rigZieht = false; }

// G und R wie Blender (transform.translate / transform.rotate im Pose-Modus): Maus nach rechts dreht um die Blickachse der
// aktiven Ansicht (X/Y/Z: Weltachse), Klick/Enter uebernimmt, Esc/rechts bricht ab.
bool modellRigModalStarten(int art) {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Animation || t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
    // Stufe 4: G bewegt auch Hand, Fuss, Saebelgriff und Pol (IK), R rollt an Ferse/Ballen/Zehe
    const bool ende = g.art == modell::GriffArt::Hand || g.art == modell::GriffArt::Fuss || g.art == modell::GriffArt::Saebelgriff;
    const bool pol = g.art == modell::GriffArt::PolEllbogen || g.art == modell::GriffArt::PolKnie;
    if (art == 3 || (art == 1 && g.art != modell::GriffArt::Becken && !ende && !pol) || (art == 2 && pol)) {
        melden(tr(pol ? Str::MbIkPolNurVerschieben : (g.ik ? Str::MbIkRolleNurDrehen : Str::MbRgNurDrehen)));
        return true;
    }
    // wie Blender (Rigify): G greift das IK-Ziel nur bei IK - sonst eine Meldung statt eines Modals, das nichts tut
    if (art == 1 && (ende || pol) && modellBlender() && modellIkAnteil(modellIkArtFuerGriff(t.griffWahl)) < 0.5F) {
        char b[400];
        std::snprintf(b, sizeof b, tr(Str::MbIkNurIk), tr(modell::ikArm(static_cast<modell::IkArt>(std::max(modellIkArtFuerGriff(t.griffWahl), 0))) ?
                                                           (g.seite == modell::Seite::Links ? Str::MbIkArmL : Str::MbIkArmR) :
                                                           (g.seite == modell::Seite::Links ? Str::MbIkBeinL : Str::MbIkBeinR)));
        melden(b);
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
        if (abbruch && t.rigZieht) { (void)t.dok.abbrechen(); }
        t.rigModal = 0;
        t.rigZieht = false;
        g_app->modelDirty = true;
    }
    return true;
}

// --- Posen ----------------------------------------------------------------------------------------------------------------

bool modellPoseKopieren() {
    ModellTab& t = mt();
    if (g_app->modellWelt.empty()) {
        melden(tr(Str::MbRgKeineGla));
        return false;
    }
    t.kopie = modell::poseAus(g_app->anim, g_app->modellWelt);
    t.kopieDa = true;
    melden(format(Str::MbRgKopiert, static_cast<int>(t.kopie.namen.size()), bildJetzt()));
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
                   [&](modell::RigSchicht& s) { n = modell::poseEinfuegen(g_app->anim, t.rig, t.kopie, gespiegelt, bild, {}, s); });
    melden(format(gespiegelt ? Str::MbRgGespiegeltOk : Str::MbRgEingefuegt, n, bild));
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
    schichtAendern(alle ? Str::MbRgBindeAlle : Str::MbRgBindeAuswahl, [&](modell::RigSchicht& s) { n = modell::bindePose(g_app->anim, bild, nur, s); });
    melden(format(Str::MbRgBindeOk, n, bild));
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
    schichtAendern(Str::MbRgUndoHand, [&](modell::RigSchicht& s) { n = modell::handPoseSetzen(g_app->anim, t.rig, seite, quelle, bild, s); });
    if (n == 0) {
        melden(tr(Str::MbRgKeineHand));
        return false;
    }
    melden(format(Str::MbRgHandOk, n, bild));
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
    melden(format(Str::MbRgGeloescht, static_cast<int>(k.size()), bild));
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
        melden(format(Str::MbSpFehler, br.fehler.c_str()));
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
    if (!modellRigDa() || !t.rigZeigen) { return; }
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

// --- Szenen-Explorer -----------------------------------------------------------------------------------------------

void modellRigExplorer(const std::string& such) {
    ModellTab& t = mt();
    if (!modellRigDa()) { return; }
    char kopf[160];
    std::snprintf(kopf, sizeof kopf, tr(Str::MbRgExplorer), static_cast<int>(t.rig.griffe.size()));
    const bool offen = ImGui::TreeNodeEx("##mtrig", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "     %s", kopf);
    {
        const ImVec2 lo = ImGui::GetItemRectMin();
        const float h = ImGui::GetTextLineHeight();
        uiIcon(ImGui::GetWindowDrawList(), UiIcon::MbRig, ImVec2{lo.x + ImGui::GetTreeNodeToLabelSpacing(), lo.y + (ImGui::GetItemRectSize().y - h) * 0.5F}, h,
               ImGui::GetColorU32(uiIconFarbe(UiIcon::MbRig)));
    }
    ImGui::SetItemTooltip("%s", tr(Str::MbRgExplorerHint));
    if (ImGui::BeginPopupContextItem("##rigmenue")) {
        aktionenAlsMenue(0);
        ImGui::EndPopup();
    }
    if (!offen) { return; }
    t.griffVersteckt.resize(t.rig.griffe.size(), 0);
    for (std::size_t i = 0; i < t.rig.griffe.size(); ++i) {
        const modell::Griff& g = t.rig.griffe[i];
        const std::string name = griffName(static_cast<int>(i));
        std::string such2 = name;
        for (char& c : such2) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (!such.empty() && such2.find(such) == std::string::npos) { continue; }
        ImGui::PushID(static_cast<int>(9000 + i));
        const bool versteckt = t.griffVersteckt[i] != 0;
        {
            // das Auge links wie bei den Flaechen (Spalte modellAugeX)
            const ImVec2 zurueck = ImGui::GetCursorScreenPos();
            const float h = ImGui::GetTextLineHeight();
            ImGui::SetCursorScreenPos(ImVec2{modellAugeX(), zurueck.y});
            if (ImGui::InvisibleButton("##auge", ImVec2{h, h})) {
                t.griffVersteckt[i] = versteckt ? 0 : 1;
                g_app->modelDirty = true;
            }
            if (ImGui::IsItemVisible()) {
                uiIcon(ImGui::GetWindowDrawList(), versteckt ? UiIcon::Versteckt : UiIcon::Sichtbar, ImVec2{modellAugeX(), zurueck.y}, h,
                       ImGui::GetColorU32(uiIconFarbe(versteckt ? UiIcon::Versteckt : UiIcon::Sichtbar)));
            }
            ImGui::SetItemTooltip("%s", tr(Str::MbAugeHint));
            ++t.augenZaehler;
            ImGui::SetCursorScreenPos(zurueck);
        }
        ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (t.griffWahl == static_cast<int>(i)) { f |= ImGuiTreeNodeFlags_Selected; }
        if (versteckt) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
        ImGui::TreeNodeEx("##griff", f, "     %s", name.c_str());
        if (versteckt) { ImGui::PopStyleColor(); }
        if (ImGui::IsItemVisible()) {
            const ImVec2 lo = ImGui::GetItemRectMin();
            const float h = ImGui::GetTextLineHeight();
            const UiIcon icon = g.ik ? UiIcon::MbPol : UiIcon::MbGriff;
            ImVec4 farbe = ImGui::ColorConvertU32ToFloat4(seitenFarbe(g.seite));
            uiIcon(ImGui::GetWindowDrawList(), icon, ImVec2{lo.x + ImGui::GetTreeNodeToLabelSpacing(), lo.y + (ImGui::GetItemRectSize().y - h) * 0.5F}, h,
                   ImGui::GetColorU32(farbe));
            ++t.symbolZaehler;
        }
        if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), name.c_str()); }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s\n%s", g.knochen >= 0 ? g_app->anim.bones[static_cast<std::size_t>(g.knochen)].name.c_str() : "-",
                              tr(griffHinweis(g)));
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) { modellRigGriffWaehlen(static_cast<int>(i)); }
        if (ImGui::BeginPopupContextItem("##griffmenue")) {
            modellRigGriffWaehlen(static_cast<int>(i));
            aktionenAlsMenue(1);
            ImGui::Separator();
            aktionenAlsMenue(2);
            if (modellIkArtFuerGriff(static_cast<int>(i)) >= 0) {   // Stufe 4: IK/FK als Untermenue wie im Menue Animation
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
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::TreePop();
}

// --- Befehlsfeld ----------------------------------------------------------------------------------------------------

namespace {

bool knopf(Str text, Str hinweis, bool aktiv = true) {
    ImGui::BeginDisabled(!aktiv);
    const bool k = ImGui::Button(tr(text));
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("%s", tr(hinweis));
    return k;
}

void daneben(Str naechster) {
    const float b = ImGui::CalcTextSize(tr(naechster)).x + ImGui::GetStyle().FramePadding.x * 2.0F;
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < b) { ImGui::NewLine(); }
}

}  // namespace

void drawModellRigBereich() {
    ModellTab& t = mt();
    modellRigAktualisieren();
    const bool rig = modellRigDa();
    if (ImGui::CollapsingHeader(tr(Str::MbRgGruppeRig), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (knopf(rig ? Str::MbRgNeu : Str::MbRgAnlegen, Str::MbRgAnlegenHint, !g_app->anim.bones.empty())) { (void)modellRigAnlegen(); }
        daneben(Str::MbRgUebersicht);
        if (knopf(Str::MbRgUebersicht, Str::MbRgUebersichtHint, rig)) { t.uebersichtOffen = !t.uebersichtOffen; }
        ImGui::PushTextWrapPos(0.0F);
        if (rig) {
            int ik = 0;
            for (const modell::Griff& g : t.rig.griffe) { ik += g.ik ? 1 : 0; }
            ImGui::TextDisabled("%s", format(Str::MbRgStand, static_cast<int>(t.rig.griffe.size()) - ik, ik, t.rig.erkennung.erkannt,
                                             static_cast<int>(g_app->anim.bones.size()), t.rig.erkennung.unklar)
                                          .c_str());
            if (t.rig.erkennung.unklar > 0) {
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "%s", format(Str::MbRgUnklarHinweis, t.rig.erkennung.unklar).c_str());
            }
            if (ImGui::Checkbox(tr(Str::MbRgGriffeZeigen), &t.rigZeigen)) { g_app->modelDirty = true; }
            ImGui::SetItemTooltip("%s", tr(Str::MbRgGriffeZeigenHint));
            if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size()) {
                const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
                ImGui::Text("%s", griffName(t.griffWahl).c_str());
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextDisabled("%s", tr(griffHinweis(g)));
                ImGui::PopTextWrapPos();
                if (g.twist >= 0) {
                    ImGui::TextDisabled("%s", format(Str::MbRgTwistInfo, g_app->anim.bones[static_cast<std::size_t>(g.twist)].name.c_str(),
                                                     static_cast<double>(g.twistAnteil * 100.0F))
                                                  .c_str());
                }
            } else {
                ImGui::TextDisabled("%s", tr(Str::MbRgGriffWaehlen));
            }
            const modell::RigSchicht* s = modellRigSchichtJetzt();
            ImGui::TextDisabled("%s", format(Str::MbRgSchluesselZahl, s != nullptr ? s->schluesselZahl() : 0).c_str());
        } else {
            ImGui::TextDisabled("%s", tr(Str::MbRgNochKeins));
        }
        ImGui::PopTextWrapPos();
        if (knopf(Str::MbRgSchluesselLoeschen, Str::MbRgSchluesselLoeschenHint, rig)) { (void)modellRigSchluesselLoeschen(); }
    }
    drawModellIkBereich();   // Stufe 4: IK/FK (wie 3ds Max Bewegung > IK-Solver, wie Rigify IK-FK)
    drawModellPhysikBereich();   // Stufe 6: Auto-Physik (wie die Rollouts von Bewegung in 3ds Max)
    // Pose, Ghosts, Pfade und Backen ab Werk zu: Platz nur, wenn gebraucht - darunter folgen GLA, animation.cfg und die
    // Plaetze aus Stufe 2 (mb4e_mb3: offen schoben sie die Plaetze aus dem Bild).
    if (ImGui::CollapsingHeader(tr(Str::MbRgGruppePose))) {
        const bool anim = !g_app->anim.bones.empty();
        if (knopf(Str::MbRgKopieren, Str::MbRgKopierenHint, anim)) { (void)modellPoseKopieren(); }
        daneben(Str::MbRgEinfuegen);
        if (knopf(Str::MbRgEinfuegen, Str::MbRgEinfuegenHint, t.kopieDa)) { (void)modellPoseEinfuegen(false); }
        daneben(Str::MbRgGespiegelt);
        if (knopf(Str::MbRgGespiegelt, Str::MbRgGespiegeltHint, t.kopieDa)) { (void)modellPoseEinfuegen(true); }
        if (knopf(Str::MbRgBindeAuswahl, Str::MbRgBindeAuswahlHint, anim)) { (void)modellBindepose(false); }
        daneben(Str::MbRgBindeAlle);
        if (knopf(Str::MbRgBindeAlle, Str::MbRgBindeAlleHint, anim)) { (void)modellBindepose(true); }
        ImGui::TextDisabled("%s", tr(Str::MbRgHandposen));
        ImGui::SetItemTooltip("%s", tr(Str::MbRgHandHint));
        static const Str kArten[3] = {Str::MbRgOffen, Str::MbRgFaust, Str::MbRgSaebel};
        for (const modell::Seite se : {modell::Seite::Links, modell::Seite::Rechts}) {
            ImGui::PushID(static_cast<int>(se));
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(tr(se == modell::Seite::Links ? Str::MbRgL : Str::MbRgR));
            for (int a = 0; a < 3; ++a) {
                ImGui::SameLine();
                ImGui::PushID(a);
                if (knopf(kArten[a], Str::MbRgHandHint, anim)) { (void)modellHandpose(se, a); }
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::TextDisabled("%s", tr(Str::MbRgGruppeAuswahl));
        if (knopf(Str::MbRgGegenseite, Str::MbRgGegenseiteHint, anim)) { (void)modellAuswahlGegenseite(); }
        daneben(Str::MbRgEltern);
        if (knopf(Str::MbRgEltern, Str::MbRgElternHint, anim)) { (void)modellAuswahlEltern(); }
        daneben(Str::MbRgKinder);
        if (knopf(Str::MbRgKinder, Str::MbRgKinderHint, anim)) { (void)modellAuswahlKinder(); }
    }
    if (ImGui::CollapsingHeader(tr(Str::MbRgGhosts))) {
        ModellGeisterEinst& g = t.geister;
        bool neu = ImGui::Checkbox(tr(Str::MbRgGhostsAn), &g.an);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgGhostsHint));
        ImGui::SameLine();
        neu |= ImGui::Checkbox(tr(Str::MbRgSilhouette), &t.silhouette);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgSilhouetteHint));
        ImGui::BeginDisabled(!g.an);
        const float b = ImGui::GetContentRegionAvail().x * 0.45F;
        ImGui::SetNextItemWidth(b);
        neu |= ImGui::SliderInt(tr(Str::MbRgDavor), &g.davor, 0, 10);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgDavorHint));
        ImGui::SetNextItemWidth(b);
        neu |= ImGui::SliderInt(tr(Str::MbRgDanach), &g.danach, 0, 10);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgDavorHint));
        ImGui::SetNextItemWidth(b);
        neu |= ImGui::SliderInt(tr(Str::MbRgAbstand), &g.abstand, 1, 20);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgAbstandHint));
        neu |= ImGui::ColorEdit4(tr(Str::MbRgFarbeDavor), g.farbeDavor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgFarbeHint));
        ImGui::SameLine();
        neu |= ImGui::ColorEdit4(tr(Str::MbRgFarbeDanach), g.farbeDanach, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgFarbeHint));
        ImGui::EndDisabled();
        if (neu) { g_app->modelDirty = true; }
    }
    if (ImGui::CollapsingHeader(tr(Str::MbRgPfade))) {
        if (knopf(Str::MbRgPfadHinzu, Str::MbRgPfadHinzuHint, !g_app->anim.bones.empty())) { (void)modellPfadHinzu(); }
        daneben(Str::MbIkPfadSpitze);
        if (knopf(Str::MbIkPfadSpitze, Str::MbIkPfadSpitzeHint, !g_app->anim.bones.empty())) { (void)modellPfadSpitzeHinzu(); }
        daneben(Str::MbRgPfadeLeeren);
        if (knopf(Str::MbRgPfadeLeeren, Str::MbRgPfadeLeerenHint, !t.pfade.empty())) { modellPfadeLeeren(); }
        const float b = ImGui::GetContentRegionAvail().x * 0.45F;
        ImGui::SetNextItemWidth(b);
        bool neu = ImGui::SliderInt(tr(Str::MbRgPfadDavor), &t.pfadDavor, 0, 120);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgPfadBereichHint));
        ImGui::SetNextItemWidth(b);
        neu |= ImGui::SliderInt(tr(Str::MbRgPfadDanach), &t.pfadDanach, 0, 120);
        ImGui::SetItemTooltip("%s", tr(Str::MbRgPfadBereichHint));
        int weg = -1;
        for (std::size_t i = 0; i < t.pfade.size(); ++i) {
            ModellPfad& p = t.pfade[i];
            ImGui::PushID(static_cast<int>(i));
            neu |= ImGui::ColorEdit4("##farbe", p.farbe, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            const modell::Inhalt& in = t.dok.inhalt();
            const std::string name = p.tag >= 0 && static_cast<std::size_t>(p.tag) < in.teile.size() ? in.teile[static_cast<std::size_t>(p.tag)].name + (p.spitze ? std::string(" ") + tr(Str::MbIkSpitzeName) : std::string())
                                     : p.knochen >= 0 && static_cast<std::size_t>(p.knochen) < g_app->anim.bones.size()
                                         ? g_app->anim.bones[static_cast<std::size_t>(p.knochen)].name
                                         : std::string("-");
            ImGui::TextUnformatted(name.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) { weg = static_cast<int>(i); }
            ImGui::SetItemTooltip("%s", tr(Str::MbRgPfadWeg));
            ImGui::PopID();
        }
        if (weg >= 0) {
            t.pfade.erase(t.pfade.begin() + weg);
            neu = true;
        }
        if (neu) { g_app->modelDirty = true; }
    }
    if (ImGui::CollapsingHeader(tr(Str::MbRgGruppeBacken))) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::MbRgBackenInfo));
        ImGui::PopTextWrapPos();
        if (knopf(Str::MbRgBacken, Str::MbRgBackenHint, sequenz() != nullptr)) { (void)modellRigBacken(); }
    }
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
    if (!ImGui::Begin((std::string(tr(Str::MbRgUebersicht)) + "###riguebersicht").c_str(), &t.uebersichtOffen)) {
        ImGui::End();
        return;
    }
    modellRigAktualisieren();
    if (!modellRigDa()) {
        ImGui::TextDisabled("%s", tr(Str::MbRgNochKeins));
        if (knopf(Str::MbRgAnlegen, Str::MbRgAnlegenHint, !g_app->anim.bones.empty())) { (void)modellRigAnlegen(); }
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
    ImGui::TextDisabled("%s", tr(Str::MbRgUebersichtHint));
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
