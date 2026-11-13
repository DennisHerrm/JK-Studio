// app_modelltab_mb2.cpp - Modell-Block 1a-2 im Model-Tab: das Fenster "Ins Skript", die Datei-Befehle des Modells
// (das Befehlsfeld des Bereichs Weights steht seit AP8 in app_modelltab_gewichte.cpp)
//
// shank 06.11.: "der model tab sollte komplett sein ohne das andere. nur man muesste irgendwie waehlen koennen das man
// animationen in ein script added ohne das es platz verschwendet am besten als separates Fenster" - der Model-Tab hat
// keinen Skriptbaum, keine Knopfspalte und keine Skript-Statuszeile; Animationen kommen ueber das Fenster "Ins Skript"
// (ab Werk zu, eigenes Fenster wie die herausgenommenen Panels) in ein offenes oder neues Skript.
#include "app_internal.h"
#include "textumbruch.h"
#include "fenster_dock.h"
#include "hilfe.h"
#include "platform.h"

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
    // die Hoehe passt sich dem Inhalt an (y = 0, unten schwebend nachgefuehrt) - die Knopfreihe bleibt sichtbar
    ImGui::SetNextWindowSize(ImVec2{fs * 32.0F, 0.0F}, ImGuiCond_FirstUseEver);
    if (dock::fensterBeginnen("modell.insskript")) {
        // UIFORM (shank 10.11.: "das muss sauberer gestaltet werden das Fenster. Damit es gleichmaessiger aussieht"): der
        // Formular-Baustein - eine Beschriftungsspalte, alle Felder an derselben Kante und gleich breit, Knopfreihe unten rechts.
        {
            ui::Formular f({tr(Str::MbInsSkriptAnim), tr(Str::MbInsSkriptBefehl), tr(Str::MbInsSkriptFigur), tr(Str::MbInsSkriptZiel)});
            // Animation: Ausklapper, im Feld der Knopf "gewaehlte Animation nehmen"
            f.zeile(tr(Str::MbInsSkriptAnim));
            const bool hatAnim = t.insAnim >= 0 && static_cast<std::size_t>(t.insAnim) < g_app->animList.size();
            ui::Formular::feldMitKnopf();
            if (ImGui::BeginCombo("##insanim", hatAnim ? g_app->animList[static_cast<std::size_t>(t.insAnim)].name.c_str() : "-")) {
                for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
                    if (ImGui::Selectable(g_app->animList[i].name.c_str(), static_cast<int>(i) == t.insAnim)) {
                        modellInsSkriptOeffnen(static_cast<int>(i));
                    }
                }
                ImGui::EndCombo();
            }
            if (f.feldKnopf("##insgewaehlte", UiIcon::Zielen, tr(Str::MbInsSkriptGewaehlte), tr(Str::UfGewaehlteHint), true)) {
                modellInsSkriptOeffnen(g_app->animIndex);
            }
            // Befehl, bei den Haltezeiten darunter die Dauer
            f.zeile(tr(Str::MbInsSkriptBefehl));
            ImGui::Combo("##insbefehl", &t.insBefehl, kBefehle, kBefehlZahl);
            if (t.insBefehl >= 3) {
                f.zeileOhne();
                const ImGuiStyle& st = ImGui::GetStyle();
                ImGui::SetNextItemWidth(f.feldBreite() - ImGui::CalcTextSize(tr(Str::MbInsSkriptMs)).x - st.ItemInnerSpacing.x);
                ImGui::InputInt(tr(Str::MbInsSkriptMs), &t.insHalteMs, 50, 1000);
                t.insHalteMs = std::max(t.insHalteMs, -1);
            }
            // Figur (affect): EIN Feld mit Liste - leer: ohne affect, in den gewaehlten Block
            f.zeile(tr(Str::MbInsSkriptFigur));
            std::set<std::string> namen = {"player"};
            for (const Actor& a : g_app->scene.actors) {
                if (!a.name.empty()) { namen.insert(a.name); }
            }
            (void)f.eingabeMitListe("##insfigur", t.insFigur, sizeof t.insFigur, "", std::vector<std::string>(namen.begin(), namen.end()));
            f.hinweis(tr(Str::MbInsSkriptFigurHint));
            // Zielskript: die offenen Reiter und ein neues
            f.zeile(tr(Str::MbInsSkriptZiel));
            const std::string zielName = t.insZiel == -1 ? std::string(tr(Str::MbInsSkriptNeu))
                                                         : reiterName(t.insZiel < 0 ? g_app->activeTab : t.insZiel);
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
            // Vorschau im Code-Kasten, Meldung, Knopfreihe
            const std::string zeile = modellInsSkriptZeile();
            f.codeVorschau("##insvorschau", tr(Str::MbInsSkriptVorschau), zeile);
            const bool fehler = t.insMeldung == tr(Str::MbInsSkriptUngueltig);
            if (fehler) {
                f.fehler(t.insMeldung.c_str());
            } else {
                f.hinweis(t.insMeldung.c_str());
            }
            // Nach dem Einfuegen heisst der zweite Knopf "Schliessen" (es ist nichts mehr abzubrechen).
            const char* zweiter = t.insMeldung.empty() || fehler ? tr(Str::UfAbbrechen) : tr(Str::UfSchliessen);
            switch (f.knoepfe(tr(Str::MbInsSkriptEinfuegen), zweiter, !zeile.empty(), tr(Str::MbInsSkriptEinfuegenHint))) {
                case ui::Formular::Knopf::Haupt: (void)modellInsSkriptEinfuegen(); break;
                case ui::Formular::Knopf::Abbrechen: g_app->settings.insSkriptOffen = false; break;
                case ui::Formular::Knopf::Keiner: break;
            }
        }
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
    // ENTSCHEID3A (B10): Strg+Umschalt+Z wiederholt nur wie Blender (ed.redo); wie 3ds Max ist es Zoom Extents All, Strg+Y wiederholt
    if (taste(ImGuiMod_Ctrl | ImGuiKey_Y) || (modellBlender() && taste(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z))) {
        (void)modellWiederholen();
        return;
    }
    if (!nurRueckgaengig && taste(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
        modellNavGrenzen(2);
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

}  // namespace bhed::gui
