// app_modelltab_ik.cpp - Model-Tab, Modell-Block Stufe 4: IK/FK, Fussrolle, Einrasten, Space Switching, Pfade an Gliedern
//
//   Rechnung       bhed/modell/ik.h (Zwei-Glied-IK wie der Pol in Blender, Fussrolle wie Rigify, Einrasten IK <-> FK)
//   wie 3ds Max    HI-Solver: Hand oder Fuss greifen (W) ist IK, ein Glied drehen (E) ist FK; der Wechsel schreibt
//                  Schluessel und rastet mit "Automatisch einrasten" (Vorgabe an) ohne Sprung ein (Bewegung > IK-Solver:
//                  IK aktiviert, IK/FK-Einrasten, Automatisch einrasten); Pol = Schwenkziel; Pfade ziehen wie Bewegung >
//                  Trajektorien (Schluessel auf der Bahn verschieben)
//   wie Blender    Rigify: Regler IK-FK je Glied (0 IK, 1 FK), Knoepfe "IK -> FK" (Ziel und Pol auf die FK-Pose) und
//                  "FK -> IK" (FK-Schluessel auf die IK-Pose), Elternwechsel (Raum) ohne Sprung; G bewegt das IK-Ziel nur
//                  bei IK, sonst eine klare Meldung
//   Fussrolle      R/E an Ferse, Ballen oder Zehenspitze rollt den Fuss um diesen Punkt (Rigify Fersen-Regler)
//   Raum           Child Of: das Ziel einer Hand folgt ab dem Bild der Welt, dem Becken, der Brust, dem Kopf oder der
//                  anderen Hand ("Saebel an die andere Hand"); das Spiel haengt den Saebel per Code an *r_hand/*l_hand
//                  (G2API_AttachG2Model, cg_players.cpp) - die Animation bewegt die Haende.
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/animkurven.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
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

const Str kArtText[modell::kIkArten] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};

const char* artName(int art) { return tr(kArtText[std::clamp(art, 0, modell::kIkArten - 1)]); }

bool istEnde(modell::GriffArt a) { return a == modell::GriffArt::Hand || a == modell::GriffArt::Fuss || a == modell::GriffArt::Saebelgriff; }
bool istGliedFk(modell::GriffArt a) {
    return a == modell::GriffArt::Oberarm || a == modell::GriffArt::Unterarm || a == modell::GriffArt::Oberschenkel || a == modell::GriffArt::Unterschenkel;
}
bool istRolle(modell::GriffArt a) { return a == modell::GriffArt::Ferse || a == modell::GriffArt::Ballen || a == modell::GriffArt::Zehe; }
bool istPol(modell::GriffArt a) { return a == modell::GriffArt::PolEllbogen || a == modell::GriffArt::PolKnie; }

// Das Glied, an dem gerade gearbeitet wird: im Rollout gewaehlt, sonst aus dem gewaehlten Griff
int artJetzt() {
    const ModellTab& t = mt();
    if (t.ikWahl >= 0 && t.ikWahl < modell::kIkArten) { return t.ikWahl; }
    return modellIkArtFuerGriff(t.griffWahl);
}

// Eine Aenderung der Schicht als ein Rueckgaengig-Schritt; die Spur des Glieds legt sie bei Bedarf aus dem Rig an.
bool ikAendern(Str undo, int art, const std::function<bool(modell::RigSchicht&, int)>& f) {
    ModellTab& t = mt();
    if (!modellRigDa() && !modellRigAnlegen()) { return false; }
    if (art < 0) {
        melden(tr(Str::MbIkGliedWaehlen));
        return false;
    }
    modell::IkSpur probe;
    if (!modell::ikSpurAnlegen(g_app->anim, t.rig, static_cast<modell::IkArt>(art), probe)) {
        melden(tr(Str::MbIkKeinGlied));
        return false;
    }
    bool ok = false;
    const std::string name = sequenzName();
    t.dok.aendern(tr(undo), [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[name];
        s.neuInterp = modell::Interp::Bezier;
        const int sp = modellIkSpur(art, s);
        ok = sp >= 0 && f(s, sp);
    }, false);
    g_app->modelDirty = true;
    g_app->animPlaying = false;
    return ok;
}

// Die Knochen eines Raums (Child Of) fuer ein Glied: Welt, Becken, Brust, Kopf, andere Hand
int raumKnochen(int art, int wahl) {
    const ModellTab& t = mt();
    const auto finde = [&](modell::Rolle r, modell::Seite se) {
        int best = -1;
        for (std::size_t b = 0; b < t.rig.erkennung.knochen.size(); ++b) {
            const modell::KnochenErkennung& k = t.rig.erkennung.knochen[b];
            if (k.rolle != r || k.seite != se) { continue; }
            // der oberste Knochen der Rolle (Becken vor dem zweiten Wirbel, der Kopf vor dem Gesicht)
            if (best < 0) { best = static_cast<int>(b); }
        }
        return best;
    };
    switch (wahl) {
        case 1: return finde(modell::Rolle::Becken, modell::Seite::Mitte);
        case 2: {
            // die Brust: der Wirbel, an dem die Schluesselbeine haengen (JKA thoracic)
            const int s = finde(modell::Rolle::Schluessel, modell::Seite::Links);
            if (s >= 0 && static_cast<std::size_t>(s) < g_app->anim.bones.size()) { return g_app->anim.bones[static_cast<std::size_t>(s)].parent; }
            return finde(modell::Rolle::Wirbel, modell::Seite::Mitte);
        }
        case 3: return finde(modell::Rolle::Kopf, modell::Seite::Mitte);
        case 4: {
            const modell::Seite andere = (art == static_cast<int>(modell::IkArt::ArmLinks) || art == static_cast<int>(modell::IkArt::BeinLinks)) ? modell::Seite::Rechts
                                                                                                                                                   : modell::Seite::Links;
            return finde(modell::Rolle::Hand, andere);
        }
        default: return -1;
    }
}

// Der naechste Knopf in derselben Zeile, wenn er noch ganz hineinpasst (sonst in die naechste, eingerueckt) - kein
// abgeschnittener Text in langen Sprachen
void daneben(const char* naechster) {
    const float b = ImGui::CalcTextSize(naechster).x + ImGui::GetStyle().FramePadding.x * 2.0F + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < b) {
        ImGui::NewLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 4.2F);   // unter die Spalte der Glieder
    }
}

}  // namespace

// --- Glied, Spur ---------------------------------------------------------------------------------------------------

int modellIkArtFuerGriff(int griff) {
    const ModellTab& t = mt();
    if (griff < 0 || static_cast<std::size_t>(griff) >= t.rig.griffe.size()) { return -1; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
    if (g.seite == modell::Seite::Mitte) { return -1; }
    using G = modell::GriffArt;
    const bool links = g.seite == modell::Seite::Links;
    switch (g.art) {
        case G::Oberarm: case G::Unterarm: case G::Hand: case G::PolEllbogen: case G::Saebelgriff:
            return static_cast<int>(links ? modell::IkArt::ArmLinks : modell::IkArt::ArmRechts);
        case G::Oberschenkel: case G::Unterschenkel: case G::Fuss: case G::Zehen: case G::PolKnie: case G::Ferse: case G::Ballen: case G::Zehe:
            return static_cast<int>(links ? modell::IkArt::BeinLinks : modell::IkArt::BeinRechts);
        default: return -1;
    }
}

int modellIkSpur(int art, modell::RigSchicht& s) {
    if (art < 0 || art >= modell::kIkArten) { return -1; }
    const int da = modell::ikSpurIndex(s, static_cast<modell::IkArt>(art));
    if (da >= 0) { return da; }
    modell::IkSpur sp;
    if (!modell::ikSpurAnlegen(g_app->anim, mt().rig, static_cast<modell::IkArt>(art), sp)) { return -1; }
    s.ik.push_back(std::move(sp));
    return static_cast<int>(s.ik.size()) - 1;
}

float modellIkAnteil(int art) {
    const modell::RigSchicht& s = schicht();
    const int i = art >= 0 ? modell::ikSpurIndex(s, static_cast<modell::IkArt>(art)) : -1;
    return i >= 0 ? modell::ikAnteilAm(s.ik[static_cast<std::size_t>(i)], static_cast<float>(bildJetzt())) : 0.0F;
}

// --- Aktionen ------------------------------------------------------------------------------------------------------

bool modellIkWechselnRoh(int art, bool ik, bool einrasten) {
    const int bild = bildJetzt();
    const bool ok = ikAendern(Str::MbIkUndoWechsel, art, [&](modell::RigSchicht& s, int sp) {
        modell::ikFkWechseln(g_app->anim, s, sp, bild, ik ? 1.0F : 0.0F, einrasten);
        return true;
    });
    if (ok) { melden(format(ik ? Str::MbIkGewechseltIk : Str::MbIkGewechseltFk, artName(art), bild)); }
    return ok;
}

bool modellIkWechseln(int art, bool ik) { return modellIkWechselnRoh(art, ik, mt().ikAutoEinrasten || modellBlender()); }

bool modellIkReglerSetzen(int art, float regler) {
    const int bild = bildJetzt();
    const float neu = 1.0F - std::clamp(regler, 0.0F, 1.0F);
    return ikAendern(Str::MbIkUndoWechsel, art, [&](modell::RigSchicht& sch, int sp) {
        if (sch.ik[static_cast<std::size_t>(sp)].ziel.empty()) {
            modell::ikAufFk(g_app->anim, sch, sp, bild, neu);
        } else {
            modell::ikFkWechseln(g_app->anim, sch, sp, bild, neu, false);
        }
        return true;
    });
}

bool modellIkEinrasten(int art, bool zielAufFk) {
    const int bild = bildJetzt();
    int n = 0;
    const bool ok = ikAendern(Str::MbIkUndoEinrasten, art, [&](modell::RigSchicht& s, int sp) {
        if (zielAufFk) {
            // Ziel und Pol auf die FK-Pose; der Anteil bleibt (Rigify ik2fk aendert den Regler nicht)
            modell::ikAufFk(g_app->anim, s, sp, bild, s.ik[static_cast<std::size_t>(sp)].ziel.empty() ? 0.0F : modell::ikAnteilAm(s.ik[static_cast<std::size_t>(sp)], static_cast<float>(bild)));
        } else {
            n = modell::fkAufIk(g_app->anim, s, sp, bild);
        }
        return true;
    });
    if (ok) { melden(zielAufFk ? format(Str::MbIkEingerastetZiel, artName(art), bild) : format(Str::MbIkEingerastetFk, artName(art), n, bild)); }
    return ok;
}

bool modellIkRaum(int art, int wahl) {
    const int raum = raumKnochen(art, wahl);
    if (wahl > 0 && raum < 0) {
        melden(tr(Str::MbIkRaumFehlt));
        return false;
    }
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const int bild = bildJetzt();
    const bool ok = ikAendern(Str::MbIkUndoRaum, art, [&](modell::RigSchicht& s, int sp) {
        modell::ikRaumWechseln(g_app->anim, s, sp, bild, raum, erstes);
        return true;
    });
    static const Str kRaum[5] = {Str::MbIkRaumWelt, Str::MbIkRaumBecken, Str::MbIkRaumBrust, Str::MbIkRaumKopf, Str::MbIkRaumHand};
    if (ok) { melden(format(Str::MbIkRaumOk, artName(art), tr(kRaum[std::clamp(wahl, 0, 4)]), bild)); }
    return ok;
}

bool modellIkSaebelHand() {
    // die Saebelhand: der gewaehlte Saebelgriff, sonst die rechte Hand (*r_hand traegt den ersten Saebel)
    const ModellTab& t = mt();
    int art = static_cast<int>(modell::IkArt::ArmRechts);
    if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size() && t.rig.griffe[static_cast<std::size_t>(t.griffWahl)].art == modell::GriffArt::Saebelgriff) {
        art = modellIkArtFuerGriff(t.griffWahl);
    }
    return modellIkRaum(art, 4);
}

bool modellIkRolle(int art, float rolle, float spitze) {
    if (art >= 0 && modell::ikArm(static_cast<modell::IkArt>(art))) {
        melden(tr(Str::MbIkRolleNurBein));
        return false;
    }
    const int bild = bildJetzt();
    const bool ok = ikAendern(Str::MbIkUndoRolle, art, [&](modell::RigSchicht& s, int sp) {
        modell::ikRolleSetzen(g_app->anim, s, sp, bild, rolle, spitze);
        return true;
    });
    if (ok) { melden(format(Str::MbIkRolleOk, artName(art), static_cast<double>(rolle), bild)); }
    return ok;
}

bool modellIkSchluesselLoeschen(int art) {
    const int bild = bildJetzt();
    bool da = false;
    const bool ok = ikAendern(Str::MbIkUndoLoeschen, art, [&](modell::RigSchicht& s, int sp) {
        modell::IkSpur& x = s.ik[static_cast<std::size_t>(sp)];
        for (const modell::RigSchluessel& k : x.ziel) { da = da || k.bild == bild; }
        modell::ikSchluesselLoeschen(x, bild);
        return true;
    });
    if (ok) { melden(format(da ? Str::MbIkGeloescht : Str::MbIkKeinSchluessel, artName(art), bild)); }
    return ok && da;
}

bool modellIkBacken(int art) {
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    int n = 0;
    const bool ok = ikAendern(Str::MbIkUndoBacken, art, [&](modell::RigSchicht& s, int sp) {
        n = modell::fkAufIkBereich(g_app->anim, s, sp, erstes, erstes + anzahl - 1);
        // danach FK: ein Schluessel mit Anteil 0 am Anfang, die anderen Schluessel der Spur fallen weg
        modell::IkSpur& x = s.ik[static_cast<std::size_t>(sp)];
        x.ziel.clear();
        x.wert.clear();
        return true;
    });
    if (ok) { melden(format(Str::MbIkGebacken, artName(art), anzahl, n)); }
    return ok;
}

// --- Ziehen in der Ansicht (Gizmo, G/R wie Blender) -------------------------------------------------------------------

int modellIkZiehen(int gizmoArt, const BoneMatrix& dreh, const float weg[3], modell::RigSchicht& neu) {
    ModellTab& t = mt();
    if (t.griffWahl < 0 || static_cast<std::size_t>(t.griffWahl) >= t.rig.griffe.size()) { return 0; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(t.griffWahl)];
    const int art = modellIkArtFuerGriff(t.griffWahl);
    if (art < 0) { return 0; }
    const int bild = t.rigZiehBild;
    const std::vector<BoneMatrix>& welt = t.rigWeltStart;
    const bool blender = modellBlender();
    const int sp0 = modell::ikSpurIndex(t.rigSchichtStart, static_cast<modell::IkArt>(art));
    const float anteil = sp0 >= 0 ? modell::ikAnteilAm(t.rigSchichtStart.ik[static_cast<std::size_t>(sp0)], static_cast<float>(bild)) : 0.0F;
    const bool ikJetzt = anteil >= 0.5F;
    const bool bewegt = weg != nullptr && (weg[0] != 0.0F || weg[1] != 0.0F || weg[2] != 0.0F);
    if (istGliedFk(g.art)) {
        if (!ikJetzt) { return 0; }   // FK wie bisher
        if (blender) {
            // Rigify: die FK-Kette unter IK zeigt sich nicht - klar sagen, statt still zu drehen
            melden(format(Str::MbIkFkBlender, artName(art)));
            return 1;
        }
        // 3ds Max: ein Glied drehen ist FK - der Wechsel schreibt Schluessel, eingerastet ohne Sprung
        const int sp = modellIkSpur(art, neu);
        modell::ikFkWechseln(g_app->anim, neu, sp, bild, 0.0F, true);
        t.ikZugArt = 4;
        return 2;   // weiter mit FK auf der umgeschalteten Schicht
    }
    if (!istEnde(g.art) && !istPol(g.art) && !istRolle(g.art)) { return 0; }
    if (blender && !ikJetzt) {
        // wie Blender (Rigify): das IK-Ziel greift nur bei IK; sonst ist es die FK-Kette
        if (istEnde(g.art) && gizmoArt == 2) { return 0; }   // R dreht die Hand in FK
        melden(format(Str::MbIkNurIk, artName(art)));
        return 1;
    }
    const int sp = modellIkSpur(art, neu);
    if (sp < 0) {
        melden(tr(Str::MbIkKeinGlied));
        return 1;
    }
    const modell::IkSpur& spur = neu.ik[static_cast<std::size_t>(sp)];
    if (istEnde(g.art)) {
        if (gizmoArt == 2 && !ikJetzt && !bewegt) { return 0; }   // FK-Drehung (die Hand ist hier FK)
        BoneMatrix ziel = modell::starr(welt[static_cast<std::size_t>(spur.ende.knochen)]);
        if (gizmoArt == 2) {
            // das Ziel um den Ursprung der Hand drehen
            const float p[3] = {ziel.m[0][3], ziel.m[1][3], ziel.m[2][3]};
            BoneMatrix d = dreh;
            d.m[0][3] = d.m[1][3] = d.m[2][3] = 0.0F;
            ziel = d * ziel;
            for (int k = 0; k < 3; ++k) { ziel.m[k][3] = p[k]; }
            t.ikZugArt = 5;
        } else {
            for (int k = 0; k < 3; ++k) { ziel.m[k][3] += weg != nullptr ? weg[k] : 0.0F; }
            t.ikZugArt = 1;
        }
        modell::ikZielSetzen(g_app->anim, neu, sp, bild, ziel);
        return 1;
    }
    if (istPol(g.art)) {
        if (gizmoArt != 1) {
            melden(tr(Str::MbIkPolNurVerschieben));
            return 1;
        }
        float p[3];
        if (!modellIkPolWelt(t.griffWahl, welt, p, &t.rigSchichtStart)) { return 1; }
        for (int k = 0; k < 3; ++k) { p[k] += weg != nullptr ? weg[k] : 0.0F; }
        modell::ikPolSetzen(g_app->anim, neu, sp, bild, p);
        t.ikZugArt = 2;
        return 1;
    }
    // Fussrolle: der Anteil der Drehung um die Rollachse des Fusses (Swing-Twist), ab der Rolle beim Anfassen
    if (gizmoArt != 2) {
        melden(tr(Str::MbIkRolleNurDrehen));
        return 1;
    }
    float achse[3];
    {
        const int fuss = spur.ende.knochen;
        const BoneMatrix lage = modell::starr(welt[static_cast<std::size_t>(fuss)]) * g_app->anim.bones[static_cast<std::size_t>(fuss)].basePoseInv;
        for (int i = 0; i < 3; ++i) { achse[i] = lage.m[i][0] * spur.rollAchse[0] + lage.m[i][1] * spur.rollAchse[1] + lage.m[i][2] * spur.rollAchse[2]; }
    }
    float q[4];
    modell::quatAusMatrix(dreh, q);
    const float l = std::sqrt(achse[0] * achse[0] + achse[1] * achse[1] + achse[2] * achse[2]);
    const float proj = l > 1e-6F ? (q[1] * achse[0] + q[2] * achse[1] + q[3] * achse[2]) / l : 0.0F;
    const float grad = 2.0F * std::atan2(proj, q[0]) * 180.0F / 3.14159265F;
    const modell::IkZiel z = sp0 >= 0 ? modell::ikZielAm(t.rigSchichtStart.ik[static_cast<std::size_t>(sp0)], static_cast<float>(bild), welt) : modell::IkZiel{};
    const float start = z.da ? z.rolle : 0.0F;
    const float spitze = g.art == modell::GriffArt::Zehe ? 1.0F : (g.art == modell::GriffArt::Ballen ? 0.0F : (z.da ? z.spitze : 0.0F));
    // die Ferse rollt nach hinten (Rolle < 0), Ballen und Zehenspitze nach vorn (> 0) - wie die zwei Richtungen des Fersen-
    // Reglers von Rigify (roll2 nur negativ, roll1/roll3 nur positiv); ziehen nach rechts verstaerkt die Rolle des Punkts
    const float rolle = g.art == modell::GriffArt::Ferse ? std::min(start - grad, 0.0F) : std::max(start + grad, 0.0F);
    modell::ikRolleSetzen(g_app->anim, neu, sp, bild, rolle, spitze);
    t.ikZugArt = 3;
    return 1;
}

bool modellIkPolWelt(int griff, const std::vector<BoneMatrix>& welt, float ort[3], const modell::RigSchicht* s) {
    const ModellTab& t = mt();
    const int art = modellIkArtFuerGriff(griff);
    const modell::RigSchicht& sch = s != nullptr ? *s : schicht();
    const int i = art >= 0 ? modell::ikSpurIndex(sch, static_cast<modell::IkArt>(art)) : -1;
    if (i >= 0 && !sch.ik[static_cast<std::size_t>(i)].ziel.empty() && welt.size() == g_app->anim.bones.size()) {
        const modell::IkZiel z = modell::ikZielAm(sch.ik[static_cast<std::size_t>(i)], static_cast<float>(s != nullptr ? t.rigZiehBild : bildJetzt()), welt);
        std::memcpy(ort, z.pol, sizeof(float) * 3U);
        return true;
    }
    // ohne IK-Schluessel: der Pol der Grundstellung, starr am Gelenk (rigAnlegen)
    if (griff < 0 || static_cast<std::size_t>(griff) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
    if (g.knochen < 0 || static_cast<std::size_t>(g.knochen) >= welt.size()) { return false; }
    float lokal[3];
    g_app->anim.bones[static_cast<std::size_t>(g.knochen)].basePoseInv.transform(g.ort, lokal);
    welt[static_cast<std::size_t>(g.knochen)].transform(lokal, ort);
    return true;
}

bool modellIkGriffMitte(int griff, float m[3]) {
    const ModellTab& t = mt();
    if (griff < 0 || static_cast<std::size_t>(griff) >= t.rig.griffe.size()) { return false; }
    const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
    if (istPol(g.art)) { return t.gizmo == 1 && modellIkPolWelt(griff, g_app->modellWelt, m, nullptr); }
    if (istRolle(g.art)) { return t.gizmo == 2 && modellRigGriffWelt(griff, g_app->modellWelt, m); }
    return false;
}

// --- Ansicht: IK-Ziele ---------------------------------------------------------------------------------------------

void modellIkUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    const modell::RigSchicht& s = schicht();
    if (s.ik.empty() || g_app->modellWelt.size() != g_app->anim.bones.size()) { return; }
    const float r = ImGui::GetFontSize() * 0.55F;
    const float bild = static_cast<float>(bildJetzt());
    for (const modell::IkSpur& sp : s.ik) {
        const modell::IkZiel z = modell::ikZielAm(sp, bild, g_app->modellWelt);
        if (!z.da || z.ik <= 0.0F) { continue; }
        const float o[3] = {z.ziel.m[0][3], z.ziel.m[1][3], z.ziel.m[2][3]};
        ImVec2 so;
        if (!modellAufSchirm(a, o, so)) { continue; }
        // wie das IK-Ziel des HI-Solvers: ein Kreuz, dazu die Linie vom Ansatz der Kette (eigene Zeichnung)
        const ImU32 farbe = IM_COL32(255, 120, 60, 230);
        dl->AddLine(ImVec2{so.x - r, so.y}, ImVec2{so.x + r, so.y}, farbe, 2.0F);
        dl->AddLine(ImVec2{so.x, so.y - r}, ImVec2{so.x, so.y + r}, farbe, 2.0F);
        dl->AddCircle(so, r * 0.55F, farbe, 16, 1.5F);
        float w[3];
        ImVec2 sw;
        const BoneMatrix& x = g_app->modellWelt[static_cast<std::size_t>(sp.oben.knochen)];
        w[0] = x.m[0][3];
        w[1] = x.m[1][3];
        w[2] = x.m[2][3];
        if (modellAufSchirm(a, w, sw)) { dl->AddLine(sw, so, IM_COL32(255, 120, 60, 120), 1.0F); }
        if (z.raum >= 0 && static_cast<std::size_t>(z.raum) < g_app->modellWelt.size()) {
            // Child Of: eine Linie zum Raum
            const BoneMatrix& rx = g_app->modellWelt[static_cast<std::size_t>(z.raum)];
            const float rw[3] = {rx.m[0][3], rx.m[1][3], rx.m[2][3]};
            ImVec2 sr;
            if (modellAufSchirm(a, rw, sr)) {
                for (int k = 0; k < 10; k += 2) {
                    const float u0 = static_cast<float>(k) / 10.0F;
                    const float u1 = static_cast<float>(k + 1) / 10.0F;
                    dl->AddLine(ImVec2{so.x + (sr.x - so.x) * u0, so.y + (sr.y - so.y) * u0}, ImVec2{so.x + (sr.x - so.x) * u1, so.y + (sr.y - so.y) * u1},
                                IM_COL32(120, 220, 255, 200), 1.5F);
                }
            }
        }
        ++t.ikGezeichnet;
    }
}

// --- Saebelspitze (Pfade) ---------------------------------------------------------------------------------------------

bool modellSaebelSpitze(const ModellPfad& p, const std::vector<BoneMatrix>& welt, float ort[3], float* klingenAnfang) {
    const ModellTab& t = mt();
    if (p.tag < 0 || static_cast<std::size_t>(p.tag) >= t.dok.inhalt().teile.size()) { return false; }
    const std::string& tag = t.dok.inhalt().teile[static_cast<std::size_t>(p.tag)].name;
    const int ts = surfaceIndex(g_app->model, tag);
    BoneMatrix hand;
    if (ts < 0 || !boltMatrix(g_app->model, ts, welt, g_app->anim, hand)) { return false; }
    if (!p.spitze) {
        ort[0] = hand.m[0][3];
        ort[1] = hand.m[1][3];
        ort[2] = hand.m[2][3];
        return true;
    }
    for (const ModellAnbau& an : t.anbauten) {
        if (an.knochen || an.ziel != tag) { continue; }
        int bs = surfaceIndex(an.modell, "*blade1");
        if (bs < 0) { bs = surfaceIndex(an.modell, "*flash"); }
        BoneMatrix kl;
        if (bs < 0 || !boltMatrixRigid(an.modell, bs, kl)) { continue; }
        // die Klinge wie beim Zeichnen: vom Bolzen *blade1 eine Laenge von 32 (wp_saberLoad.cpp:364 lengthMax) entlang
        // der ersten Achse zurueck (app_view3d.cpp, Klinge der Figur)
        const float sp[3] = {kl.m[0][3] - kl.m[0][0] * 32.0F, kl.m[1][3] - kl.m[1][0] * 32.0F, kl.m[2][3] - kl.m[2][0] * 32.0F};
        hand.transform(sp, ort);
        if (klingenAnfang != nullptr) {
            const float b[3] = {kl.m[0][3], kl.m[1][3], kl.m[2][3]};
            hand.transform(b, klingenAnfang);
        }
        return true;
    }
    return false;
}

bool modellPfadSpitzeHinzu() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    // das Tag des Saebels: *r_hand (der erste Saebel), sonst *l_hand - mit einem angebauten Saebel
    for (const char* tag : {"*r_hand", "*l_hand"}) {
        int teil = -1;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].name == tag) { teil = static_cast<int>(i); }
        }
        if (teil < 0) { continue; }
        ModellPfad p;
        p.tag = teil;
        p.spitze = true;
        float o[3];
        if (g_app->modellWelt.empty() || !modellSaebelSpitze(p, g_app->modellWelt, o)) { continue; }
        for (const ModellPfad& x : t.pfade) {
            if (x.tag == teil && x.spitze) {
                melden(tr(Str::MbRgPfadDa));
                return false;
            }
        }
        static const float kFarbe[4] = {0.55F, 0.95F, 1.0F, 1.0F};
        std::memcpy(p.farbe, kFarbe, sizeof p.farbe);
        t.pfade.push_back(p);
        melden(format(Str::MbIkSpitzeNeu, tag));
        g_app->modelDirty = true;
        return true;
    }
    melden(tr(Str::MbIkKeinSaebel));
    return false;
}

// --- Befehlsfeld: Rollout IK/FK -----------------------------------------------------------------------------------------

void drawModellIkBereich() {
    ModellTab& t = mt();
    if (!modellRigDa()) { return; }
    // wie 3ds Max: das IK-Rollout geht auf, sobald ein Glied gewaehlt wird (Bewegung > IK-Solver)
    const int vonGriff = modellIkArtFuerGriff(t.griffWahl);
    if (t.griffWahl != t.ikZuletztGriff) {
        t.ikZuletztGriff = t.griffWahl;
        if (vonGriff >= 0) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            t.ikWahl = -1;
        }
    }
    const bool offen = ImGui::CollapsingHeader(tr(Str::MbIkRollout));
    ImGui::SetItemTooltip("%s", tr(Str::MbIkRolloutHint));
    if (!offen) { return; }
    t.ikRolloutBild = ImGui::GetFrameCount();
    const bool blender = modellBlender();
    const int jetzt = artJetzt();
    const modell::RigSchicht& s = schicht();
    if (!blender) {
        ImGui::Checkbox(tr(Str::MbIkAuto), &t.ikAutoEinrasten);
        ImGui::SetItemTooltip("%s", tr(Str::MbIkAutoHint));
    }
    // je Glied eine Zeile: Name (waehlt), IK aktiviert (Max) bzw. Regler IK-FK (Blender), Einrasten
    for (int art = 0; art < modell::kIkArten; ++art) {
        ImGui::PushID(art);
        modell::IkSpur probe;
        const bool da = modell::ikSpurAnlegen(g_app->anim, t.rig, static_cast<modell::IkArt>(art), probe);
        ImGui::BeginDisabled(!da);
        const float anteil = modellIkAnteil(art);
        if (ImGui::Selectable(artName(art), jetzt == art, ImGuiSelectableFlags_AllowOverlap, ImVec2{ImGui::GetFontSize() * 4.2F, 0.0F})) { t.ikWahl = art; }
        ImGui::SetItemTooltip("%s", tr(Str::MbIkGliedHint));
        ImGui::SameLine();
        if (blender) {
            // Rigify: IK-FK, 0 = IK, 1 = FK
            float regler = 1.0F - anteil;
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0F);
            if (ImGui::SliderFloat("##ikfk", &regler, 0.0F, 1.0F, "%.2f")) { (void)modellIkReglerSetzen(art, regler); }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkReglerHint));
            daneben(tr(Str::MbIkKnopfIkFk));
            if (ImGui::SmallButton(tr(Str::MbIkKnopfIkFk))) { (void)modellIkEinrasten(art, true); }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkZielAufFkHint));
            daneben(tr(Str::MbIkKnopfFkIk));
            if (ImGui::SmallButton(tr(Str::MbIkKnopfFkIk))) { (void)modellIkEinrasten(art, false); }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkFkAufIkHint));
        } else {
            bool an = anteil >= 0.5F;
            if (ImGui::Checkbox(tr(Str::MbIkAn), &an)) { (void)modellIkWechseln(art, an); }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkAnHint));
            daneben(tr(Str::MbIkKnopfEinrasten));
            if (ImGui::SmallButton(tr(Str::MbIkKnopfEinrasten))) { (void)modellIkEinrasten(art, an ? false : true); }
            ImGui::SetItemTooltip("%s", tr(Str::MbIkEinrastenMaxHint));
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::Separator();
    if (jetzt < 0) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", tr(Str::MbIkGliedWaehlen));
        ImGui::PopTextWrapPos();
        return;
    }
    cjkTextUnformatted(artName(jetzt));
    const int i = modell::ikSpurIndex(s, static_cast<modell::IkArt>(jetzt));
    const modell::IkZiel z = i >= 0 ? modell::ikZielAm(s.ik[static_cast<std::size_t>(i)], static_cast<float>(bildJetzt()), g_app->modellWelt) : modell::IkZiel{};
    ImGui::SameLine();
    cjkTextDisabled("%s", format(Str::MbIkStand, z.da && z.ik >= 0.5F ? "IK" : "FK", i >= 0 ? static_cast<int>(s.ik[static_cast<std::size_t>(i)].ziel.size()) : 0).c_str());
    // Raum (Child Of / Space Switching)
    static const Str kRaum[5] = {Str::MbIkRaumWelt, Str::MbIkRaumBecken, Str::MbIkRaumBrust, Str::MbIkRaumKopf, Str::MbIkRaumHand};
    int raumJetzt = 0;
    for (int w = 1; w < 5 && z.da; ++w) {
        if (z.raum >= 0 && raumKnochen(jetzt, w) == z.raum) { raumJetzt = w; }
    }
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##ikraum", tr(kRaum[raumJetzt]))) {
        for (int w = 0; w < 5; ++w) {
            if (ImGui::Selectable(tr(kRaum[w]), w == raumJetzt)) { (void)modellIkRaum(jetzt, w); }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MbIkRaumHint));
    if (!modell::ikArm(static_cast<modell::IkArt>(jetzt))) {
        float rolle = z.da ? z.rolle : 0.0F;
        float spitze = z.da ? z.spitze : 0.0F;
        const float b = ImGui::GetContentRegionAvail().x * 0.5F;
        ImGui::SetNextItemWidth(b);
        const bool r1 = ImGui::SliderFloat(tr(Str::MbIkRolle), &rolle, -60.0F, 60.0F, "%.1f");
        ImGui::SetItemTooltip("%s", tr(Str::MbIkRolleHint));
        ImGui::SetNextItemWidth(b);
        const bool r2 = ImGui::SliderFloat(tr(Str::MbIkSpitze), &spitze, 0.0F, 1.0F, "%.2f");
        ImGui::SetItemTooltip("%s", tr(Str::MbIkSpitzeHint));
        if (r1 || r2) { (void)modellIkRolle(jetzt, rolle, spitze); }
    }
    if (ImGui::Button(tr(Str::MbIkLoeschen))) { (void)modellIkSchluesselLoeschen(jetzt); }
    ImGui::SetItemTooltip("%s", tr(Str::MbIkLoeschenHint));
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MbIkBacken))) { (void)modellIkBacken(jetzt); }
    ImGui::SetItemTooltip("%s", tr(Str::MbIkBackenHint));
}

}  // namespace bhed::gui
