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

// Der naechste Knopf daneben, wenn er Platz hat, sonst darunter (nichts abschneiden, auch nicht in langen Sprachen)
void daneben(Str naechster) {
    const float b = ImGui::CalcTextSize(tr(naechster)).x + ImGui::GetStyle().FramePadding.x * 2.0F;
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < b) { ImGui::NewLine(); }
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

// --- Rollout ---------------------------------------------------------------------------------------------------------

void drawModellPhysikBereich() {
    ModellTab& t = mt();
    if (!modellRigDa()) { return; }
    modellPhysikAktualisieren();
    if (t.physikOeffnen) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        t.physikOeffnen = false;
    }
    const bool offen = ImGui::CollapsingHeader(tr(Str::PhRollout));
    ImGui::SetItemTooltip("%s", tr(Str::PhRolloutHint));
    if (!offen) { return; }
    t.physikRolloutBild = ImGui::GetFrameCount();
    const modell::PhysikEinstellung* ep = modellPhysikEinstellung();
    const modell::PhysikEinstellung e = ep != nullptr ? *ep : modell::PhysikEinstellung{};
    bool an = e.an;
    if (ImGui::Checkbox(tr(Str::PhAn), &an)) { (void)modellPhysikAn(an); }
    ImGui::SetItemTooltip("%s", tr(Str::PhAnHint));
    bool zeigen = t.physikZeigen;
    if (ImGui::Checkbox(tr(Str::PhZeigen), &zeigen)) {
        t.physikZeigen = zeigen;
        g_app->modelDirty = true;
    }
    ImGui::SetItemTooltip("%s", tr(Str::PhZeigenHint));
    if (!t.physikMassen.gueltig()) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "%s", format(Str::PhFehler, modellFehlerText(t.physikMassen.fehler).c_str()).c_str());
        ImGui::PopTextWrapPos();
        return;
    }
    ImGui::BeginDisabled(!e.an);
    float staerke = e.staerke;
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(tr(Str::PhStaerke)).x - ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::SliderFloat(tr(Str::PhStaerke), &staerke, 0.0F, 1.0F, "%.2f")) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.staerke = staerke; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    ImGui::SetItemTooltip("%s", tr(Str::PhStaerkeHint));
    const auto schalter = [&](Str text, Str hinweis, bool wert, const std::function<void(modell::PhysikEinstellung&, bool)>& setzen) {
        bool v = wert;
        if (ImGui::Checkbox(tr(text), &v)) { (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { setzen(x, v); }); }
        ImGui::SetItemTooltip("%s", tr(hinweis));
    };
    schalter(Str::PhSprung, Str::PhSprungHint, e.sprung, [](modell::PhysikEinstellung& x, bool v) { x.sprung = v; });
    schalter(Str::PhKontakte, Str::PhKontakteHint, e.kontakte, [](modell::PhysikEinstellung& x, bool v) { x.kontakte = v; });
    // Flug
    ImGui::PushTextWrapPos(0.0F);
    if (t.physik.fluege.empty()) {
        cjkTextDisabled("%s", tr(Str::PhKeinFlug));
    } else {
        // Bilder wie die Zeitleiste des Model-Tabs: ab dem ersten der Sequenz gezaehlt
        for (const auto& [a, l] : t.physik.fluege) {
            cjkTextUnformatted((format(Str::PhFlug, a - t.physik.erstes, l - t.physik.erstes) + " (" + tr(e.absprung >= 0 ? Str::PhFlugHand : Str::PhFlugAuto) +
                                    ")").c_str());
        }
    }
    ImGui::PopTextWrapPos();
    if (ImGui::SmallButton(tr(Str::PhAbsprungHier))) { (void)modellPhysikFlugHier(false); }
    ImGui::SetItemTooltip("%s", tr(Str::PhAbsprungHint));
    daneben(Str::PhLandungHier);
    if (ImGui::SmallButton(tr(Str::PhLandungHier))) { (void)modellPhysikFlugHier(true); }
    ImGui::SetItemTooltip("%s", tr(Str::PhLandungHint));
    daneben(Str::PhFlugAutomatisch);
    ImGui::BeginDisabled(e.absprung < 0);
    if (ImGui::SmallButton(tr(Str::PhFlugAutomatisch))) { (void)modellPhysikFlugAutomatisch(); }
    ImGui::SetItemTooltip("%s", tr(Str::PhFlugAutomatischHint));
    ImGui::EndDisabled();
    const float breit = ImGui::GetFontSize() * 5.0F;
    float g = e.schwerkraft;
    ImGui::SetNextItemWidth(breit);
    if (ImGui::DragFloat(tr(Str::PhSchwerkraft), &g, 5.0F, 0.0F, 4000.0F, "%.0f")) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.schwerkraft = g; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    ImGui::SetItemTooltip("%s", tr(Str::PhSchwerkraftHint));
    float k = e.kontaktAbstand;
    ImGui::SetNextItemWidth(breit);
    if (ImGui::DragFloat(tr(Str::PhKontaktAbstand), &k, 0.1F, 0.0F, 32.0F, "%.1f")) {
        (void)einstellen(tr(Str::PhUndoEinstellung), [&](modell::PhysikEinstellung& x) { x.kontaktAbstand = k; }, true);
    }
    if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
    ImGui::SetItemTooltip("%s", tr(Str::PhKontaktAbstandHint));
    // Federketten
    ImGui::Separator();
    schalter(Str::PhFedern, Str::PhFedernHint, e.federn, [](modell::PhysikEinstellung& x, bool v) { x.federn = v; });
    if (ImGui::SmallButton(tr(Str::PhKetteHier))) { (void)modellPhysikKetteHier(); }
    ImGui::SetItemTooltip("%s", tr(Str::PhKetteHierHint));
    daneben(Str::PhKettenVorschlag);
    if (ImGui::SmallButton(tr(Str::PhKettenVorschlag))) { (void)modellPhysikKettenVorschlag(); }
    ImGui::SetItemTooltip("%s", tr(Str::PhKettenVorschlagHint));
    if (e.ketten.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", tr(Str::PhKeineKette));
        ImGui::PopTextWrapPos();
    }
    for (std::size_t i = 0; i < e.ketten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const modell::PhysikKette c = e.ketten[i];
        ImGui::AlignTextToFramePadding();
        cjkTextUnformatted(knochenName(c.knochen));
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten.erase(x.ketten.begin() + static_cast<std::ptrdiff_t>(i)); }
            });
        }
        ImGui::SetItemTooltip("%s", tr(Str::PhKetteEntfernen));
        float f = c.frequenz;
        float d = c.daempfung;
        ImGui::SetNextItemWidth(breit);
        if (ImGui::SliderFloat(tr(Str::PhFrequenz), &f, 0.2F, 10.0F, "%.1f Hz")) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].frequenz = f; }
            }, true);
        }
        if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
        ImGui::SetItemTooltip("%s", tr(Str::PhFrequenzHint));
        ImGui::SetNextItemWidth(breit);
        if (ImGui::SliderFloat(tr(Str::PhDaempfung), &d, 0.0F, 1.0F, "%.2f")) {
            (void)einstellen(tr(Str::PhUndoKette), [&](modell::PhysikEinstellung& x) {
                if (i < x.ketten.size()) { x.ketten[i].daempfung = d; }
            }, true);
        }
        if (ImGui::IsItemDeactivated()) { t.physikZug = false; }
        ImGui::SetItemTooltip("%s", tr(Str::PhDaempfungHint));
        ImGui::PopID();
    }
    ImGui::EndDisabled();
    // Befund
    ImGui::Separator();
    ImGui::PushTextWrapPos(0.0F);
    if (t.physik.warnungen > 0) {
        float weit = 0.0F;
        for (const modell::PhysikBild& pb : t.physik.bilder) { weit = std::max(weit, pb.abstand); }
        cjkTextColored(ImVec4{1.0F, 0.35F, 0.3F, 1.0F}, "%s",
                           format(Str::PhWarnung, t.physik.warnBild - t.physik.erstes, t.physik.warnungen, static_cast<double>(weit)).c_str());
    } else if (std::any_of(t.physik.bilder.begin(), t.physik.bilder.end(), [](const modell::PhysikBild& pb) { return pb.imStand; })) {
        cjkTextDisabled("%s", tr(Str::PhImStand));
    }
    if (e.an && !t.physik.fluege.empty() && e.sprung) {
        cjkTextDisabled("%s", format(Str::PhErgebnis, static_cast<double>(t.physik.parabelAbw), static_cast<double>(t.physik.drehimpulsVorher * 100.0F),
                                         static_cast<double>(t.physik.drehimpulsNachher * 100.0F))
                                      .c_str());
    }
    ImGui::PopTextWrapPos();
}

}  // namespace bhed::gui
