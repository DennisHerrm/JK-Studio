// app_modelltab_mb2.cpp - Modell-Block 1a-2 im Model-Tab: der Bereich Weights (Befehlsfeld), das Fenster
// "Ins Skript", die Datei-Befehle des Modells
//
// shank 06.11.: "der model tab sollte komplett sein ohne das andere. nur man muesste irgendwie waehlen koennen das man
// animationen in ein script added ohne das es platz verschwendet am besten als separates Fenster" - der Model-Tab hat
// keinen Skriptbaum, keine Knopfspalte und keine Skript-Statuszeile; Animationen kommen ueber das Fenster "Ins Skript"
// (ab Werk zu, eigenes Fenster wie die herausgenommenen Panels) in ein offenes oder neues Skript.
// Das Malen selbst (Pinsel in der Ansicht, Farbe je Knochen) steht in app_modelltab_ansicht.cpp, die Regeln fuer die
// Gewichte in bhed/modell/gewichte.h.
#include "app_internal.h"
#include "fenster_dock.h"
#include "hilfe.h"
#include "platform.h"

#include "bhed/modell/gewichte.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {

void modellNeuesSkript();   // app.cpp

namespace {

ModellTab& mt() { return g_app->mtab; }

// --- Ins Skript ------------------------------------------------------------------------------------
//
// Die Befehle wie die Sequenzliste bisher ("Ins Skript", animSetzen) und die Haltezeiten aus Q3_Interface.h:
// SET_ANIM_HOLDTIME_LOWER/UPPER/BOTH "Hold ... anim for number of milliseconds".
const char* const kBefehle[] = {"SET_ANIM_BOTH", "SET_ANIM_UPPER", "SET_ANIM_LOWER", "SET_ANIM_HOLDTIME_BOTH", "SET_ANIM_HOLDTIME_UPPER",
                                "SET_ANIM_HOLDTIME_LOWER"};
constexpr int kBefehlZahl = 6;

// SET_ANIM_UPPER fuer TORSO_, SET_ANIM_LOWER fuer LEGS_, sonst SET_ANIM_BOTH (wie animSetzen in app_modell.cpp).
int befehlFuer(const std::string& name) {
    if (name.rfind("TORSO_", 0) == 0) { return 1; }
    if (name.rfind("LEGS_", 0) == 0) { return 2; }
    return 0;
}

// Die Dauer einer Animation in ms wie PM_AnimLength: Bilder mal frameLerp (animBildMs).
int dauerMs(const AnimEntry& e) { return static_cast<int>(static_cast<double>(e.numFrames) * animBildMs(e) + 0.5); }

std::string reiterName(int i) {
    if (i == g_app->activeTab || i < 0 || static_cast<std::size_t>(i) >= g_app->tabs.size()) {
        if (!g_app->path.empty()) { return fileName(g_app->path); }
        return g_app->shownName.empty() ? std::string("unnamed.txt") : g_app->shownName;
    }
    const std::string& p = g_app->tabs[static_cast<std::size_t>(i)].path;
    return p.empty() ? std::string("unnamed.txt") : fileName(p);
}

}  // namespace

void modellInsSkriptOeffnen(int anim) {
    ModellTab& t = mt();
    t.insAnim = anim;
    if (anim >= 0 && static_cast<std::size_t>(anim) < g_app->animList.size()) {
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(anim)];
        t.insBefehl = befehlFuer(e.name);
        t.insHalteMs = dauerMs(e);
    }
    t.insMeldung.clear();
    g_app->settings.insSkriptOffen = true;
}

std::string modellInsSkriptZeile() {
    const ModellTab& t = mt();
    if (t.insAnim < 0 || static_cast<std::size_t>(t.insAnim) >= g_app->animList.size()) { return std::string(); }
    const std::string& name = g_app->animList[static_cast<std::size_t>(t.insAnim)].name;
    const int b = std::clamp(t.insBefehl, 0, kBefehlZahl - 1);
    char zeile[300];
    if (b < 3) {
        std::snprintf(zeile, sizeof zeile, "set ( /*@SET_TYPES*/ \"%s\", \"%s\" );", kBefehle[b], name.c_str());
    } else {
        std::snprintf(zeile, sizeof zeile, "set ( /*@SET_TYPES*/ \"%s\", %d );", kBefehle[b], t.insHalteMs);
    }
    if (t.insFigur[0] == '\0') { return std::string(zeile) + "\n"; }
    // Eine Figur: in ihren affect-Block wie BehavEd ihn schreibt.
    return std::string("affect ( \"") + t.insFigur + "\", /*@AFFECT_TYPE*/ FLUSH )\n{\n\t" + zeile + "\n}\n";
}

bool modellInsSkriptEinfuegen() {
    ModellTab& t = mt();
    const std::string text = modellInsSkriptZeile();
    Script s;
    std::vector<Diag> diag;
    if (text.empty() || !readScript(text, s, diag) || s.nodes.empty()) {
        t.insMeldung = tr(Str::MbInsSkriptUngueltig);
        return false;
    }
    // Das Ziel: ein neues Skript (neuer Reiter), der aktive Reiter oder ein anderer offener (dort ans Ende).
    if (t.insZiel == -1) {
        modellNeuesSkript();
        t.insZiel = -2;
    }
    const bool aktiv = t.insZiel < 0 || t.insZiel == g_app->activeTab || static_cast<std::size_t>(t.insZiel) >= g_app->tabs.size();
    bool ok = false;
    if (aktiv) {
        Path at;
        ok = g_app->doc.insertAfterAll(g_app->selectedPath, {s.nodes.front()}, &at);   // leerer Weg: ans Ende
        if (ok) {
            g_app->selectedPath = at;
            rebuildTree();
            g_app->camTrackValid = false;
        }
    } else {
        ok = g_app->tabs[static_cast<std::size_t>(t.insZiel)].doc.insertAfterAll(Path{}, {s.nodes.front()});
    }
    if (!ok) {
        t.insMeldung = tr(Str::MbInsSkriptUngueltig);
        return false;
    }
    char z[300];
    std::snprintf(z, sizeof z, tr(Str::MbInsSkriptEingefuegt), reiterName(aktiv ? g_app->activeTab : t.insZiel).c_str());
    t.insMeldung = z;
    addStatus(z);
    return true;
}

void drawModellInsSkriptFenster() {
    if (g_app->leftMode != 2 || !g_app->settings.insSkriptOffen) { return; }
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    // die Hoehe passt sich beim ersten Oeffnen dem Inhalt an (y = 0) - der Knopf Einfuegen bleibt sichtbar
    ImGui::SetNextWindowSize(ImVec2{fs * 32.0F, 0.0F}, ImGuiCond_FirstUseEver);
    if (dock::fensterBeginnen("modell.insskript")) {
        ImGui::PushTextWrapPos(0.0F);
        // Animation
        ImGui::TextDisabled("%s", tr(Str::MbInsSkriptAnim));
        ImGui::SameLine();
        const bool hatAnim = t.insAnim >= 0 && static_cast<std::size_t>(t.insAnim) < g_app->animList.size();
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::BeginCombo("##insanim", hatAnim ? g_app->animList[static_cast<std::size_t>(t.insAnim)].name.c_str() : "-")) {
            for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
                if (ImGui::Selectable(g_app->animList[i].name.c_str(), static_cast<int>(i) == t.insAnim)) {
                    modellInsSkriptOeffnen(static_cast<int>(i));
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button(tr(Str::MbInsSkriptGewaehlte))) { modellInsSkriptOeffnen(g_app->animIndex); }
        // Befehl
        ImGui::TextDisabled("%s", tr(Str::MbInsSkriptBefehl));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::Combo("##insbefehl", &t.insBefehl, kBefehle, kBefehlZahl);
        if (t.insBefehl >= 3) {
            ImGui::SetNextItemWidth(fs * 8.0F);
            ImGui::InputInt(tr(Str::MbInsSkriptMs), &t.insHalteMs, 50, 1000);
            t.insHalteMs = std::max(t.insHalteMs, -1);
        }
        // Figur (affect) - leer: ohne affect, in den gewaehlten Block
        ImGui::TextDisabled("%s", tr(Str::MbInsSkriptFigur));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-fs * 2.0F);
        ImGui::InputTextWithHint("##insfigur", tr(Str::MbInsSkriptFigurHint), t.insFigur, sizeof t.insFigur);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##insfiguren", ImGuiDir_Down)) { ImGui::OpenPopup("##insfigurwahl"); }
        if (ImGui::BeginPopup("##insfigurwahl")) {
            std::set<std::string> namen = {"player"};
            for (const Actor& a : g_app->scene.actors) {
                if (!a.name.empty()) { namen.insert(a.name); }
            }
            for (const std::string& n : namen) {
                if (ImGui::Selectable(n.c_str())) { std::snprintf(t.insFigur, sizeof t.insFigur, "%s", n.c_str()); }
            }
            ImGui::EndPopup();
        }
        // Zielskript: die offenen Reiter und ein neues
        ImGui::TextDisabled("%s", tr(Str::MbInsSkriptZiel));
        ImGui::SameLine();
        const std::string zielName = t.insZiel == -1 ? std::string(tr(Str::MbInsSkriptNeu))
                                                     : reiterName(t.insZiel < 0 ? g_app->activeTab : t.insZiel);
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::BeginCombo("##insziel", zielName.c_str())) {
            const std::size_t n = std::max<std::size_t>(g_app->tabs.size(), 1U);
            for (std::size_t i = 0; i < n; ++i) {
                ImGui::PushID(static_cast<int>(i));
                const bool gew = (t.insZiel < 0 && t.insZiel != -1 && static_cast<int>(i) == g_app->activeTab) || t.insZiel == static_cast<int>(i);
                if (ImGui::Selectable(reiterName(static_cast<int>(i)).c_str(), gew)) { t.insZiel = static_cast<int>(i); }
                ImGui::PopID();
            }
            if (ImGui::Selectable(tr(Str::MbInsSkriptNeu), t.insZiel == -1)) { t.insZiel = -1; }
            ImGui::EndCombo();
        }
        // Vorschau und Einfuegen
        ImGui::Separator();
        ImGui::TextDisabled("%s", tr(Str::MbInsSkriptVorschau));
        const std::string zeile = modellInsSkriptZeile();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_PlotLines));
        ImGui::TextUnformatted(zeile.empty() ? "-" : zeile.c_str());
        ImGui::PopStyleColor();
        ImGui::BeginDisabled(zeile.empty());
        if (ImGui::Button(tr(Str::MbInsSkriptEinfuegen))) { (void)modellInsSkriptEinfuegen(); }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tr(Str::MbInsSkriptEinfuegenHint));
        }
        if (!t.insMeldung.empty()) { ImGui::TextDisabled("%s", t.insMeldung.c_str()); }
        ImGui::PopTextWrapPos();
        // Schwebend so hoch wie der Inhalt: die Startlage der Panels (startLage) ist niedriger, und Einfuegen lag unter dem
        // Rand (Handbuchfoto mb2_hb_mb1a2: nur halb zu sehen). Angedockt bestimmt der Knoten die Hoehe.
        if (!ImGui::IsWindowDocked()) {
            const float noetig = std::min(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().WindowPadding.y,
                                          ImGui::GetMainViewport()->WorkSize.y * 0.9F);
            if (std::fabs(noetig - ImGui::GetWindowHeight()) > 1.0F) { ImGui::SetWindowSize(ImVec2{ImGui::GetWindowWidth(), noetig}); }
        }
    }
    dock::fensterEnde();
}

// --- Datei-Befehle im Model-Tab ----------------------------------------------------------------------

void modellDateiMenue() {
    if (ImGui::MenuItem(tr(Str::MbDateiOeffnen), "Ctrl+O")) { openModelFile(); }
    if (ImGui::MenuItem(tr(Str::OpenGlmPk3))) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->pk3Kind = 2;
        refreshPk3List();
        g_app->pk3Open = true;
    }
    // Stufe 2: .glm aus dem Modelldokument (app_modelltab_mb3.cpp)
    const bool glm = mt().dok.inhalt().herkunft == modell::Herkunft::Glm;
    if (ImGui::MenuItem(tr(Str::MbDateiSpeichern), "Ctrl+S", false, glm)) { modellSpeichern(false); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(glm ? Str::MbDateiSpeichernHint : Str::MbSpNichtGlm)); }
    if (ImGui::MenuItem(tr(Str::MbDateiSpeichernUnter), "Ctrl+Shift+S", false, glm)) { modellSpeichern(true); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(glm ? Str::MbDateiSpeichernUnterHint : Str::MbSpNichtGlm)); }
    if (ImGui::MenuItem(tr(Str::MbFoto), nullptr, false, !g_app->model.empty())) {
        const std::string p = platform::saveFileDialog(tr(Str::MbFoto), "Bitmap (*.bmp)|*.bmp", g_app->settings.lastDir,
                                                       fileName(directoryOf(g_app->modelPath)) + ".bmp");
        if (!p.empty()) { (void)modellFotoSpeichern(p, 1920); }
    }
    if (ImGui::MenuItem(tr(Str::MbInsSkriptFenster), nullptr, g_app->settings.insSkriptOffen)) {
        if (g_app->settings.insSkriptOffen) {
            g_app->settings.insSkriptOffen = false;
        } else {
            modellInsSkriptOeffnen(g_app->animIndex);
        }
    }
}

void modellDateiTasten(bool nurRueckgaengig) {
    const ImGuiIO& io = ImGui::GetIO();
    const auto taste = [](ImGuiKeyChord c) { return ImGui::IsKeyChordPressed(c); };
    if (taste(ImGuiMod_Ctrl | ImGuiKey_Z)) {
        (void)modellRueckgaengig();
        return;
    }
    if (taste(ImGuiMod_Ctrl | ImGuiKey_Y) || taste(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
        (void)modellWiederholen();
        return;
    }
    if (nurRueckgaengig) { return; }
    const bool alt = io.KeyAlt && !io.KeyCtrl;
    if (taste(ImGuiMod_Ctrl | ImGuiKey_O) || (alt && ImGui::IsKeyPressed(ImGuiKey_O, false))) {
        openModelFile();
        return;
    }
    // Stufe 2: Strg+S gilt dem Modell (wie BehavEd Alt+S speichern, Alt+V speichern unter), kein Skript.
    if (taste(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) || (alt && ImGui::IsKeyPressed(ImGuiKey_V, false))) {
        modellSpeichern(true);
        return;
    }
    if (taste(ImGuiMod_Ctrl | ImGuiKey_S) || (alt && ImGui::IsKeyPressed(ImGuiKey_S, false))) {
        modellSpeichern(false);
        return;
    }
    if (taste(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_I)) {
        modellInsSkriptOeffnen(g_app->animIndex);
        return;
    }
    if (alt && ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        if (confirmQuit()) {
            g_app->quitConfirmed = true;
            g_app->wantQuit = true;
        }
        return;
    }
    if (alt && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
        g_app->prefsOpen = true;
        return;
    }
    if (taste(ImGuiKey_F1)) { hilfeOeffnen(); }
}

// --- Bereich Weights: das Befehlsfeld --------------------------------------------------------------

void drawModellGewichte() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const GlaAnimation& a = g_app->anim;
    const bool blender = g_app->settings.modellBedienung == 1;
    ImGui::PushTextWrapPos(0.0F);
    // Knochen
    ImGui::TextDisabled("%s", tr(Str::MbGwKnochen));
    ImGui::SameLine();
    const bool hatKnochen = t.knochenWahl >= 0 && static_cast<std::size_t>(t.knochenWahl) < a.bones.size();
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##gwknochen", hatKnochen ? a.bones[static_cast<std::size_t>(t.knochenWahl)].name.c_str() : "-")) {
        for (std::size_t b = 0; b < a.bones.size(); ++b) {
            if (ImGui::Selectable(a.bones[b].name.c_str(), static_cast<int>(b) == t.knochenWahl)) {
                t.knochenWahl = static_cast<int>(b);
                g_app->modelDirty = true;
            }
        }
        ImGui::EndCombo();
    }
    if (!hatKnochen) { ImGui::TextDisabled("%s", tr(Str::MbGwKnochenWaehlen)); }
    // Werkzeug
    static const Str kArten[modell::kPinselArten] = {Str::MbGwHinzu, Str::MbGwAbziehen, Str::MbGwGlaetten, Str::MbGwSetzen};
    static const Str kHinweise[modell::kPinselArten] = {Str::MbGwHinzuHint, Str::MbGwAbziehenHint, Str::MbGwGlaettenHint, Str::MbGwSetzenHint};
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, ImGui::GetStyle().ItemSpacing.y});
    for (int k = 0; k < modell::kPinselArten; ++k) {
        if (k > 0) { ImGui::SameLine(); }
        const bool an = t.pinselArt == k;
        if (an) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
        if (ImGui::Button(tr(kArten[k]))) { t.pinselArt = k; }
        if (an) { ImGui::PopStyleColor(); }
        ImGui::SetItemTooltip("%s", tr(kHinweise[k]));
    }
    ImGui::PopStyleVar();
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextItemWidth(fs * 8.0F);
    ImGui::SliderFloat(tr(Str::MbGwGewicht), &t.pinselGewicht, 0.0F, 1.0F, "%.3f");
    // 3ds Max Weight Tool: Festwerte 0 / .1 / .25 / .5 / .75 / .9 / 1 (vorbilder_max_blender.md); Blender: Pinsel + Menue
    if (!blender) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, ImGui::GetStyle().ItemSpacing.y});
        static const float kFest[] = {0.0F, 0.1F, 0.25F, 0.5F, 0.75F, 0.9F, 1.0F};
        for (std::size_t k = 0; k < sizeof kFest / sizeof kFest[0]; ++k) {
            if (k > 0) { ImGui::SameLine(); }
            char n[16];
            std::snprintf(n, sizeof n, "%g##fest", static_cast<double>(kFest[k]));
            if (ImGui::SmallButton(n)) { t.pinselGewicht = kFest[k]; }
        }
        ImGui::PopStyleVar();
    }
    ImGui::SetNextItemWidth(fs * 8.0F);
    ImGui::SliderFloat(tr(Str::MbGwStaerke), &t.pinselStaerke, 0.0F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(fs * 8.0F);
    ImGui::SliderFloat(tr(Str::MbGwRadius), &t.pinselRadius, 2.0F, 400.0F, "%.0f px");
    ImGui::Checkbox(tr(Str::MbGwSpiegelMalen), &t.pinselSpiegeln);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbGwSpiegelHint)); }
    ImGui::BeginDisabled(!hatKnochen);
    if (ImGui::Button(tr(Str::MbGwAufAuswahl))) { (void)modellAufAuswahlSetzen(); }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("%s", tr(Str::MbGwAufAuswahlHint));
    if (ImGui::Button(tr(Str::MbGwSpiegelnPlus))) { (void)modellSpiegeln(1); }
    ImGui::SetItemTooltip("%s", tr(Str::MbGwSpiegelHint));
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MbGwSpiegelnMinus))) { (void)modellSpiegeln(-1); }
    ImGui::SetItemTooltip("%s", tr(Str::MbGwSpiegelHint));
    // Pruefhilfen live (g2.h / gewichte.h): Mallisten ueber vier, Knochen je Flaeche wie das Spiel sie bekaeme
    const int stufe = t.lodJetzt;
    int ueber4 = 0;
    int malend = 0;
    std::vector<std::string> ueber32;
    const modell::Grenzen& g = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || static_cast<std::size_t>(stufe) >= in.teile[i].stufen.size()) { continue; }
        const modell::Netz& n = in.teile[i].stufen[static_cast<std::size_t>(stufe)];
        ueber4 += modell::eckenUeberVier(n);
        malend += modell::malt(n) ? 1 : 0;
        if (i < t.anzeige.surfaces.size()) {
            std::set<int> k;
            for (const GlmVertex& v : t.anzeige.surfaces[i].verts) {
                for (int j = 0; j < 4; ++j) {
                    if (v.weights[j] > 0.0F) { k.insert(v.bones[j]); }
                }
            }
            if (static_cast<int>(k.size()) > g.knochenJeFlaeche) { ueber32.push_back(in.teile[i].name + " (" + std::to_string(k.size()) + ")"); }
        }
    }
    ImGui::Separator();
    ImGui::TextDisabled(tr(Str::MbGwPruef), g.name, g.gewichteJeEcke, g.knochenJeFlaeche);
    const ImVec4 warn = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
    if (ueber4 > 0) { ImGui::TextColored(warn, tr(Str::MbGwUeber4), ueber4); }
    for (const std::string& n : ueber32) { ImGui::TextColored(warn, tr(Str::MbGwUeber32), n.c_str(), g.knochenJeFlaeche); }
    if (ueber4 == 0 && ueber32.empty()) { ImGui::TextDisabled("%s", tr(Str::MbGwPruefOk)); }
    ImGui::BeginDisabled(malend == 0);
    if (ImGui::Button(tr(Str::MbGwUebernehmen))) { (void)modellUebernehmen(); }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tr(Str::MbGwUebernehmenHint));
    }
    ImGui::TextDisabled("%s", tr(blender ? Str::MbGwTastenBlender : Str::MbGwTastenMax));
    ImGui::PopTextWrapPos();
}

}  // namespace bhed::gui
