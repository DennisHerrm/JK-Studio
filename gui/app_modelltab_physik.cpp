// app_modelltab_physik.cpp - Modell-Block Stufe 6: Auto-Physik im Model-Tab
//
// Rollout "Auto-Physik" im Befehlsfeld unter IK/FK (wie die Rollouts von Bewegung in 3ds Max; im Modus wie Blender dieselbe
// Stelle in der Seitenleiste), Aktionen im Menue Animation (Untermenue), im Rechtsklick und in der Suche (X / F3), dazu die
// Ueberlagerung in der Ansicht: Schwerpunkt mit Lot auf den Boden, sein Weg ueber die Sequenz (Flug orange), die
// Stuetzflaeche der Fuesse im Kontakt (rot, wenn der Schwerpunkt im Stand ausserhalb liegt). Rechnung: bhed/modell/physik.h.
//
// Die Einstellungen stehen in der Schicht der Sequenz (Dokument, Rueckgaengig); die Physik-Spur rechnet
// modellPhysikAktualisieren nach jeder Aenderung des Dokuments neu und legt sie ohne eigenen Rueckgaengig-Schritt in die
// Schicht (sie folgt nur aus Schicht und Einstellung - Rueckgaengig bringt beides gemeinsam zurueck).
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/ik.h"
#include "bhed/modell/physik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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

std::string sequenzName() {
    const AnimEntry* e = sequenz();
    return e != nullptr ? e->name : std::string();
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
    anzahl = std::min(anzahl, std::max(g_app->anim.bildZahl() - erstes, 0));
}

// MRESTE: Frames wie die Zeitleiste - wie 3ds Max ab 0 in der Sequenz, wie Blender ab 1 (vorbilder_max_blender.md)
int abEins() { return modellBlender() ? 1 : 0; }

int bildJetzt() { return std::clamp(g_app->animFrame, 0, std::max(g_app->anim.bildZahl() - 1, 0)); }

modell::PhysikUmgebung umgebung() {
    modell::PhysikUmgebung u;
    u.bodenZ = mt().an.bodenZ;   // die Bodenebene des Model-Tabs (ModView: -24)
    const AnimEntry* e = sequenz();
    u.bildMs = e != nullptr ? animBildMs(*e) : 50.0;
    return u;
}

// Eine Aenderung der Einstellung als ein Rueckgaengig-Schritt (beim Ziehen eines Reglers: Schritt beim ersten Bild, danach
// Vorschau)
bool einstellen(const char* undo, const std::function<void(modell::PhysikEinstellung&)>& f, bool zug = false) {
    ModellTab& t = mt();
    if (!modellRigDa() && !modellRigAnlegen()) { return false; }
    const std::string name = sequenzName();
    const auto tun = [&](modell::Inhalt& in) { f(in.rig.schichten[name].physik.e); };
    if (zug && t.physikZug) {
        t.dok.vorschau(tun, false);
    } else {
        t.dok.aendern(undo, tun, false);
        t.physikZug = zug;
    }
    g_app->modelDirty = true;
    return true;
}

const char* knochenName(int k) {
    return k >= 0 && static_cast<std::size_t>(k) < g_app->anim.bones.size() ? g_app->anim.bones[static_cast<std::size_t>(k)].name.c_str() : "-";
}


int gewaehlterKnochen() {
    const ModellTab& t = mt();
    if (t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size()) { return t.rig.griffe[static_cast<std::size_t>(t.griffWahl)].knochen; }
    return t.knochenWahl;
}

}  // namespace

const modell::PhysikEinstellung* modellPhysikEinstellung() {
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    return s != nullptr ? &s->physik.e : nullptr;
}

void modellPhysikAktualisieren() {
    ModellTab& t = mt();
    if (!modellRigDa()) { return; }
    const std::string seq = sequenzName();
    if (t.physikStand == t.dok.stand() && t.physikSequenz == seq && t.physikBoden == t.an.bodenZ) { return; }
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const bool an = s != nullptr && s->physik.e.an;
    // Waehrend eines Zugs (Griff, Kurve, Regler der Staerke) bleibt die letzte Spur; gerechnet wird beim Loslassen
    if (an && ImGui::IsMouseDown(ImGuiMouseButton_Left) && t.physikStand >= 0) { return; }
    if (t.physikRigFuer != t.rigFuer) {
        t.physikMassen = modell::massenModell(g_app->anim, t.rig);
        t.physikRigFuer = t.rigFuer;
    }
    int erstes = 0;
    int anzahl = 0;
    bereich(erstes, anzahl);
    const auto t0 = std::chrono::steady_clock::now();
    const modell::RigSchicht leer;
    if (an) {
        modell::RigSchicht neu = *s;
        (void)modell::physikRechnen(g_app->anim, t.rig, neu, erstes, anzahl, umgebung(), &t.physik);
        t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[seq].physik.knochen = std::move(neu.physik.knochen); }, false);
    } else {
        (void)modell::physikAnalyse(g_app->anim, t.rig, s != nullptr ? *s : leer, erstes, anzahl, umgebung(), t.physik);
        if (s != nullptr && !s->physik.knochen.empty()) {
            t.dok.vorschau([&](modell::Inhalt& in) { in.rig.schichten[seq].physik.knochen.clear(); }, false);
        }
    }
    t.physikMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    ++t.physikRechnungen;
    t.physikStand = t.dok.stand();
    t.physikSequenz = seq;
    t.physikBoden = t.an.bodenZ;
    g_app->modelDirty = true;
}

bool modellPhysikEinstellen(const char* undo, const std::function<void(modell::PhysikEinstellung&)>& f) { return einstellen(undo, f); }

bool modellPhysikAn(bool an) {
    const bool ok = einstellen(tr(Str::PhUndoAn), [&](modell::PhysikEinstellung& e) { e.an = an; });
    if (ok && an) { mt().physikOeffnen = true; }
    return ok;
}

bool modellPhysikFlugHier(bool landung) {
    const int b = bildJetzt();
    return einstellen(tr(Str::PhUndoFlug), [&](modell::PhysikEinstellung& e) {
        // die andere Grenze aus der Rechnung, wenn sie noch automatisch ist
        const ModellTab& t = mt();
        if (e.absprung < 0 || e.landung < 0) {
            const std::pair<int, int> fl = t.physik.fluege.empty() ? std::pair<int, int>{b, b} : t.physik.fluege.front();
            e.absprung = fl.first;
            e.landung = fl.second;
        }
        (landung ? e.landung : e.absprung) = b;
    });
}

bool modellPhysikFlugAutomatisch() {
    return einstellen(tr(Str::PhUndoFlug), [](modell::PhysikEinstellung& e) {
        e.absprung = -1;
        e.landung = -1;
    });
}

bool modellPhysikKetteHier() {
    const int k = gewaehlterKnochen();
    if (k < 0) {
        melden(tr(Str::PhKetteGewaehlt));
        return false;
    }
    return einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& e) {
        for (const modell::PhysikKette& c : e.ketten) {
            if (c.knochen == k) { return; }
        }
        modell::PhysikKette c;
        c.knochen = k;
        e.ketten.push_back(c);
        e.federn = true;
    });
}

int modellPhysikKettenVorschlag() {
    if (!modellRigDa() && !modellRigAnlegen()) { return 0; }
    const std::vector<int> v = modell::kettenVorschlag(mt().rig, g_app->anim);
    int neu = 0;
    (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& e) {
        for (const int k : v) {
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

// --- Ueberlagerung ---------------------------------------------------------------------------------------------------

void modellPhysikUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    t.physikGezeichnet = 0;
    const modell::PhysikEinstellung* e = modellPhysikEinstellung();
    if (!t.physikZeigen || e == nullptr || !e->an || !t.physikMassen.gueltig() || g_app->modellWelt.size() != g_app->anim.bones.size()) { return; }
    const modell::PhysikBericht& b = t.physik;
    const int jetzt = bildJetzt() - b.erstes;
    const bool imBereich = jetzt >= 0 && static_cast<std::size_t>(jetzt) < b.bilder.size();
    const modell::PhysikBild* pb = imBereich ? &b.bilder[static_cast<std::size_t>(jetzt)] : nullptr;
    const bool rot = pb != nullptr && pb->ausserhalb;
    const ImU32 gruen = IM_COL32(90, 220, 120, 230);
    const ImU32 warn = IM_COL32(255, 70, 60, 240);
    const ImU32 flug = IM_COL32(255, 170, 40, 230);
    // Weg des Schwerpunkts ueber die Sequenz (mit Physik), Flug orange
    ImVec2 vor;
    bool vorDa = false;
    for (std::size_t i = 0; i < b.bilder.size(); ++i) {
        const modell::Vec3& p = b.bilder[i].spNeu;
        const float w[3] = {p[0], p[1], p[2]};
        ImVec2 s;
        const bool da = modellAufSchirm(a, w, s);
        if (da && vorDa) { dl->AddLine(vor, s, b.bilder[i].flug ? flug : IM_COL32(200, 200, 210, 150), b.bilder[i].flug ? 2.0F : 1.0F); }
        vor = s;
        vorDa = da;
    }
    ++t.physikGezeichnet;
    // Stuetzflaeche auf dem Boden
    if (pb != nullptr && pb->stuetz.size() >= 2) {
        std::vector<ImVec2> pts;
        for (const auto& q : pb->stuetz) {
            const float w[3] = {q[0], q[1], t.an.bodenZ};
            ImVec2 s;
            if (modellAufSchirm(a, w, s)) { pts.push_back(s); }
        }
        if (pts.size() == pb->stuetz.size()) {
            dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), rot ? warn : gruen, ImDrawFlags_Closed, 2.0F);
            ++t.physikGezeichnet;
        }
    }
    // Schwerpunkt jetzt (aus der sichtbaren Welt) mit Lot auf den Boden
    const modell::Vec3 c = modell::schwerpunkt(g_app->anim, t.physikMassen, g_app->modellWelt);
    const float cw[3] = {c[0], c[1], c[2]};
    const float bw[3] = {c[0], c[1], t.an.bodenZ};
    ImVec2 sc;
    ImVec2 sb;
    if (modellAufSchirm(a, cw, sc)) {
        if (modellAufSchirm(a, bw, sb)) {
            for (int k = 0; k < 12; k += 2) {
                const float u0 = static_cast<float>(k) / 12.0F;
                const float u1 = static_cast<float>(k + 1) / 12.0F;
                dl->AddLine(ImVec2{sc.x + (sb.x - sc.x) * u0, sc.y + (sb.y - sc.y) * u0}, ImVec2{sc.x + (sb.x - sc.x) * u1, sc.y + (sb.y - sc.y) * u1},
                            rot ? warn : gruen, 1.5F);
            }
            const float k = ImGui::GetFontSize() * 0.35F;
            dl->AddLine(ImVec2{sb.x - k, sb.y}, ImVec2{sb.x + k, sb.y}, rot ? warn : gruen, 2.0F);
            dl->AddLine(ImVec2{sb.x, sb.y - k}, ImVec2{sb.x, sb.y + k}, rot ? warn : gruen, 2.0F);
        }
        // wie ein Schwerpunkt-Symbol: Kreis mit zwei gefuellten Vierteln (eigene Zeichnung)
        const float r = ImGui::GetFontSize() * 0.5F;
        const ImU32 f = rot ? warn : IM_COL32(250, 230, 80, 240);
        dl->AddCircleFilled(sc, r, IM_COL32(20, 20, 24, 200), 20);
        dl->PathArcTo(sc, r, 0.0F, 1.5707964F, 8);
        dl->PathLineTo(sc);
        dl->PathFillConvex(f);
        dl->PathArcTo(sc, r, 3.1415927F, 4.712389F, 8);
        dl->PathLineTo(sc);
        dl->PathFillConvex(f);
        dl->AddCircle(sc, r, f, 20, 1.5F);
        if (rot) { dl->AddText(ImVec2{sc.x + r * 1.4F, sc.y - r}, warn, tr(Str::PhAusserhalb)); }
        ++t.physikGezeichnet;
    }
}

// --- Gruppe Auto-Physik (AP10: ueber den Baustein gui/ui_rollout.h; mehr als der Entwurf zeigt, ab Werk zugeklappt) ----------

void drawModellPhysikBereich() {
    ModellTab& t = mt();
    if (!modellRigDa()) { return; }
    modellPhysikAktualisieren();
    if (t.physikOeffnen) {
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
        ui::rolloutZeigen(tr(Str::PhRollout));   // aufklappen und ins Bild rollen (Menue, Rechtsklick, Suche)
        t.physikOeffnen = false;
    }
    if (!ui::rolloutBeginn(tr(Str::PhRollout), {.zu = true, .hinweis = tr(Str::PhRolloutHint)})) { return; }
    t.physikRolloutBild = ImGui::GetFrameCount();
    const modell::PhysikEinstellung* ep = modellPhysikEinstellung();
    const modell::PhysikEinstellung e = ep != nullptr ? *ep : modell::PhysikEinstellung{};
    ui::rasterBeginn(2);
    bool an = e.an;
    if (ui::schalter(tr(Str::PhAn), &an, tr(Str::PhAnHint))) { (void)modellPhysikAn(an); }
    bool zeigen = t.physikZeigen;
    if (ui::schalter(tr(Str::PhZeigen), &zeigen, tr(Str::PhZeigenHint))) {
        t.physikZeigen = zeigen;
        g_app->modelDirty = true;
    }
    ui::rasterEnde();
    if (!t.physikMassen.gueltig()) {
        ui::absatz(format(Str::PhFehler, modellFehlerText(t.physikMassen.fehler).c_str()).c_str(), ui::Ton::Warnung);
        ui::rolloutEnde();
        return;
    }
    ImGui::BeginDisabled(!e.an);
    float staerke = e.staerke;
    if (ui::regler(tr(Str::PhStaerke), &staerke, 0.0F, 1.0F, "%.2f", tr(Str::PhStaerkeHint))) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.staerke = staerke; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    const auto schalter = [&](Str text, Str hinweis, bool wert, const std::function<void(modell::PhysikEinstellung&, bool)>& setzen) {
        bool v = wert;
        if (ui::schalter(tr(text), &v, tr(hinweis))) { (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { setzen(x, v); }); }
    };
    schalter(Str::PhSprung, Str::PhSprungHint, e.sprung, [](modell::PhysikEinstellung& x, bool v) { x.sprung = v; });
    schalter(Str::PhKontakte, Str::PhKontakteHint, e.kontakte, [](modell::PhysikEinstellung& x, bool v) { x.kontakte = v; });
    // Flug
    if (t.physik.fluege.empty()) {
        ui::absatz(tr(Str::PhKeinFlug));
    } else {
        // Bilder wie die Zeitleiste des Model-Tabs: ab dem ersten der Sequenz gezaehlt
        for (const auto& [a, l] : t.physik.fluege) {
            ui::absatz((format(Str::PhFlug, a - t.physik.erstes + abEins(), l - t.physik.erstes + abEins()) + " (" + tr(e.absprung >= 0 ? Str::PhFlugHand : Str::PhFlugAuto) + ")").c_str(),
                       ui::Ton::Normal);
        }
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::PhAbsprungHier), {.icon = UiIcon::Hinfliegen, .hinweis = tr(Str::PhAbsprungHint)})) { (void)modellPhysikFlugHier(false); }
    if (ui::werkzeugKnopf(tr(Str::PhLandungHier), {.icon = UiIcon::Boden, .hinweis = tr(Str::PhLandungHint)})) { (void)modellPhysikFlugHier(true); }
    if (ui::werkzeugKnopf(tr(Str::PhFlugAutomatisch), {.icon = UiIcon::Neuladen, .hinweis = tr(Str::PhFlugAutomatischHint), .aktiv = e.absprung >= 0})) {
        (void)modellPhysikFlugAutomatisch();
    }
    ui::rasterEnde();
    float schwerkraft = e.schwerkraft;
    ui::zeile(tr(Str::PhSchwerkraft), tr(Str::PhSchwerkraftHint));
    if (ImGui::DragFloat("##phschwerkraft", &schwerkraft, 5.0F, 0.0F, 4000.0F, "%.0f")) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.schwerkraft = schwerkraft; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    ImGui::SetItemTooltip("%s", tr(Str::PhSchwerkraftHint));
    float kontakt = e.kontaktAbstand;
    ui::zeile(tr(Str::PhKontaktAbstand), tr(Str::PhKontaktAbstandHint));
    if (ImGui::DragFloat("##phkontakt", &kontakt, 0.1F, 0.0F, 32.0F, "%.1f")) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.kontaktAbstand = kontakt; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    ImGui::SetItemTooltip("%s", tr(Str::PhKontaktAbstandHint));
    // Federketten
    schalter(Str::PhFedern, Str::PhFedernHint, e.federn, [](modell::PhysikEinstellung& x, bool v) { x.federn = v; });
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::PhKetteHier), {.icon = UiIcon::Plus, .hinweis = tr(Str::PhKetteHierHint)})) { (void)modellPhysikKetteHier(); }
    if (ui::werkzeugKnopf(tr(Str::PhKettenVorschlag), {.icon = UiIcon::Erzeugen, .hinweis = tr(Str::PhKettenVorschlagHint)})) { (void)modellPhysikKettenVorschlag(); }
    ui::rasterEnde();
    if (e.ketten.empty()) { ui::absatz(tr(Str::PhKeineKette)); }
    for (std::size_t i = 0; i < e.ketten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const modell::PhysikKette c = e.ketten[i];
        if (ui::eintrag(knochenName(c.knochen), tr(Str::PhKetteEntfernen))) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten.erase(x.ketten.begin() + static_cast<std::ptrdiff_t>(i)); }
            });
        }
        float f = c.frequenz;
        float d = c.daempfung;
        if (ui::regler(tr(Str::PhFrequenz), &f, 0.2F, 10.0F, "%.1f Hz", tr(Str::PhFrequenzHint))) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].frequenz = f; }
            }, true);
        }
        if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
        if (ui::regler(tr(Str::PhDaempfung), &d, 0.0F, 1.0F, "%.2f", tr(Str::PhDaempfungHint))) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].daempfung = d; }
            }, true);
        }
        if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
        ImGui::PopID();
    }
    ImGui::EndDisabled();
    // Befund
    if (t.physik.warnungen > 0) {
        float weit = 0.0F;
        for (const modell::PhysikBild& pb : t.physik.bilder) { weit = std::max(weit, pb.abstand); }
        ui::absatz(format(Str::PhWarnung, t.physik.warnBild - t.physik.erstes + abEins(), t.physik.warnungen, static_cast<double>(weit)).c_str(), ui::Ton::Fehler);
    } else if (std::any_of(t.physik.bilder.begin(), t.physik.bilder.end(), [](const modell::PhysikBild& pb) { return pb.imStand; })) {
        ui::absatz(tr(Str::PhImStand));
    }
    if (e.an && !t.physik.fluege.empty() && e.sprung) {
        ui::absatz(format(Str::PhErgebnis, static_cast<double>(t.physik.parabelAbw), static_cast<double>(t.physik.drehimpulsVorher * 100.0F),
                          static_cast<double>(t.physik.drehimpulsNachher * 100.0F))
                       .c_str());
    }
    ui::rolloutEnde();
}

}  // namespace bhed::gui
