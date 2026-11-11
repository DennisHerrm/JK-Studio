// app_modell.cpp - der Model-Tab: Leiste, Animationsliste, NPC-Browser, Abspielen
//
// Gewuenscht (11.10.2026): "Model-Tab modernisieren" - dieselbe Bedienung wie
// in der Karte und der Zeitleiste (rc588):
//
//   oben     Icons: .glm oeffnen, aus .pk3, Skin, .gla, NPC-Browser,
//            Drehteller, Knochen
//   links    der Szenen-Explorer (gui/app_modelltab_explorer.cpp, seit AP2 mit
//            den Sequenzen als Knoten des Baums: Suche, Gruppe BOTH_/TORSO_/
//            LEGS_/FACE_, Laenge und Schleife; Klick spielt ab, "Kopieren",
//            "Ins Skript")
//   unten    Transportleiste wie die Zeitleiste: Anfang, Bild zurueck,
//            Abspielen, Bild vor, grosse Bildanzeige, Schleife, Tempo,
//            Haut, Flaechen, Schieberegler
#include "app_internal.h"
#include "textumbruch.h"
#include "tour.h"   // H2: Ziele der Tour k-skins

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

void animWaehlen(int i, bool abspielen) {
    if (i < 0 || static_cast<std::size_t>(i) >= g_app->animList.size()) { return; }
    const AnimEntry& e = g_app->animList[static_cast<std::size_t>(i)];
    g_app->animIndex = i;
    g_app->animFrame = e.firstFrame;
    g_app->animTime = 0.0F;
    g_app->animText = e.name;
    g_app->animPlaying = abspielen;
    g_app->modelDirty = true;
}

// --- Die Leiste ueber der Ansicht ----------------------------------------------------
void drawModelToolbar() {
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{sp * 0.5F, ImGui::GetStyle().ItemSpacing.y});
    if (iconKnopf(tr(Str::OpenGlm), UiIcon::Oeffnen, tr(Str::OpenGlmHint))) { openModelFile(); }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::OpenGlmPk3), UiIcon::Paket, tr(Str::OpenGlmPk3Hint))) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->pk3Kind = 2;
        refreshPk3List();
        g_app->pk3Open = true;
    }
    ImGui::SameLine();
    if (iconTextKnopf(tr(Str::NpcBrowser), UiIcon::Figur, tr(Str::NpcBrowserHint))) {
        g_app->npcBrowserOffen = !g_app->npcBrowserOffen;
    }
    ImGui::SameLine(0.0F, sp * 1.5F);
    ImGui::BeginDisabled(g_app->model.empty());
    if (iconKnopf(tr(Str::OpenSkin), UiIcon::Namen, tr(Str::OpenSkinHint))) { openSkinFile(); }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelLoadGla), UiIcon::Liste, tr(Str::ModelLoadGlaHint))) { openSkeletonFile(); }
    ImGui::SameLine(0.0F, sp * 1.5F);
    if (iconKnopf(tr(Str::ModelTurntable), UiIcon::Schleife, tr(Str::ModelTurntableHint), g_app->modellDrehteller, true,
                  true)) {
        g_app->modellDrehteller = !g_app->modellDrehteller;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelBones), UiIcon::Pfad, tr(Str::ModelBonesHint), g_app->modellKnochen, true, true)) {
        g_app->modellKnochen = !g_app->modellKnochen;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelCaps), UiIcon::Bruch, tr(Str::ModelCapsHint), g_app->showCaps, true, true)) {
        g_app->showCaps = !g_app->showCaps;
        g_app->modelDirty = true;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelTextures), UiIcon::Leuchten, tr(Str::ModelTexturesHint), g_app->showModelTextures, true,
                  true)) {
        g_app->showModelTextures = !g_app->showModelTextures;
        g_app->modelDirty = true;
    }
    ImGui::SameLine(0.0F, sp * 1.5F);
    if (iconKnopf(tr(Str::ModelResetView), UiIcon::Einpassen, tr(Str::ModelResetViewHint))) { modellAnsichtEinpassen(); }
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    if (!g_app->model.empty()) {
        ImGui::SameLine(0.0F, sp * 1.5F);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", fileName(g_app->modelPath).c_str());
    }
}

void modellAnsichtEinpassen() {
    if (g_app->model.empty()) { return; }
    for (int k = 0; k < 3; ++k) {
        g_app->modelCam.pos[k] = (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F;
    }
    float size = 0.0F;
    for (int k = 0; k < 3; ++k) { size = std::max(size, g_app->model.maxs[k] - g_app->model.mins[k]); }
    // Abstand so, dass die Figur das Bild fuellt. 2.2 war zu weit - eine 66 Einheiten hohe Figur stand als Streichholz
    // in der Mitte.
    g_app->modelOrbit.distance = std::max(size * 1.15F, 24.0F);
    g_app->modelCam.angles[0] = 0.0F;
    g_app->modelCam.angles[2] = 0.0F;   // MBKOPF: ohne den Rollwinkel von Umschalt+Ziffernblock 4/6
    // MBKOPF: von vorne - die Figur schaut in der .glm nach -Y (REST M9), die Kamera also entlang +Y; bis rc611 180 (von
    // der Seite). Der Model-Tab passt danach auf die gezeichneten Flaechen in der Pose ein (mittig, einpassenOffen).
    g_app->modelCam.angles[1] = 90.0F;
    // mb1a1: die Kamera VOR die Mitte, nicht in sie hinein. Orbit::turn nimmt
    // den Drehpunkt vor der Kamera (pos + vorn * Abstand); mit pos = Mitte lag
    // er hinter der Figur, und die Kamera blieb in ihrer Mitte stehen (seit
    // rc591, gefunden mit dem Selbsttest modellblock).
    float vorn[3];
    g_app->modelCam.forward(vorn);
    for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] -= vorn[k] * g_app->modelOrbit.distance; }
    g_app->mtab.einpassenOffen = true;
    g_app->modelDirty = true;
}

// --- NPC-Browser ------------------------------------------------------------------------
//
// Alle NPC-Typen der .npc-Dateien; ein Klick laedt Modell und Haut der Figur,
// wie das Spiel sie zeigt (playerModel, customSkin).
bool npcLaden(const std::string& typ) {
    if (g_app->npcMap.empty()) { (void)npcTypenListe(); }
    const auto it = g_app->npcMap.find(klein(typ));
    if (it == g_app->npcMap.end() || it->second.playerModel.empty()) { return false; }
    const std::string pfad = "models/players/" + it->second.playerModel + "/model.glm";
    std::string fehler;
    if (!oeffneModellAusArchiv(pfad, &fehler)) {
        meldung(pfad + ": " + fehler, "", true);
        return false;
    }
    if (!it->second.customSkin.empty()) {
        const std::string skin = "model_" + it->second.customSkin + ".skin";
        for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
            if (klein(g_app->modelSkins[i]) == klein(skin)) { applySkinByIndex(static_cast<int>(i)); }
        }
    }
    g_app->npcGeladen = typ;
    g_app->modelDirty = true;
    return true;
}

const std::vector<std::string>& npcTypenListe() {
    static std::vector<std::string> typen;
    static std::size_t stand = static_cast<std::size_t>(-1);
    if (g_app->npcMap.empty()) {
        (void)npcDateienLesen(g_app->npcMap);   // rc604 jk2: Jedi Outcast liest NPCs.cfg
    }
    if (stand != g_app->npcMap.size()) {
        typen.clear();
        for (const auto& [t, d] : g_app->npcMap) {
            if (!d.playerModel.empty()) { typen.push_back(t); }
        }
        stand = g_app->npcMap.size();
    }
    return typen;
}

void drawNpcBrowser() {
    if (!g_app->npcBrowserOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 22.0F, fs * 30.0F}, ImGuiCond_FirstUseEver);
    if (!ui::fensterBegin((std::string(tr(Str::NpcBrowser)) + "###npcbrowser").c_str(), &g_app->npcBrowserOffen)) {
        ImGui::End();
        return;
    }
    static char suche[64] = "";
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##npcsuche", tr(Str::NpcSearch), suche, sizeof(suche));
    const std::string such = klein(suche);
    const auto& typen = npcTypenListe();
    ImGui::TextDisabled(trN(Str::NpcCount, typen.size()), static_cast<int>(typen.size()));
    ImGui::BeginChild("##npcliste", ImVec2{-1.0F, -1.0F}, ImGuiChildFlags_Borders);
    // modell1: wie die Animationsliste nur die sichtbaren Zeilen (1323 Typen);
    // die Treffer der Suche neu nur bei anderer Liste (npcTypenListe baut neu,
    // wenn sich die Zahl der NPCs aendert) oder Suche.
    static std::vector<const std::string*> treffer;
    static const std::string* trefferListe = nullptr;
    static std::size_t trefferAnzahl = 0;
    static std::size_t trefferNpcs = 0;
    static std::string trefferSuche;
    if (trefferListe != typen.data() || trefferAnzahl != typen.size() || trefferNpcs != g_app->npcMap.size() ||
        trefferSuche != such) {
        trefferListe = typen.data();
        trefferAnzahl = typen.size();
        trefferNpcs = g_app->npcMap.size();
        trefferSuche = such;
        treffer.clear();
        for (const std::string& t : typen) {
            if (!such.empty() && klein(t).find(such) == std::string::npos) {
                const auto it = g_app->npcMap.find(t);
                if (it == g_app->npcMap.end() || klein(it->second.playerModel).find(such) == std::string::npos) {
                    continue;
                }
            }
            treffer.push_back(&t);
        }
    }
    const std::string geladen = klein(g_app->npcGeladen);
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(treffer.size()));
    while (clipper.Step()) {
        for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r) {
            const std::string& t = *treffer[static_cast<std::size_t>(r)];
            const auto it = g_app->npcMap.find(t);
            ImGui::PushID(t.c_str());
            if (ImGui::Selectable(t.c_str(), geladen == t)) { modellVerwerfenDann([t] { (void)npcLaden(t); }); }   // pfix H3
            if (it != g_app->npcMap.end()) {
                ImGui::SameLine(fs * 11.0F);
                ImGui::TextDisabled("%s%s%s", it->second.playerModel.c_str(), it->second.customSkin.empty() ? "" : " / ",
                                    it->second.customSkin.c_str());
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

// --- In der Ansicht: Drehteller und Knochen ----------------------------------------------
void modellDrehteller() {
    if (!g_app->modellDrehteller || g_app->model.empty()) { return; }
    g_app->modelOrbit.turn(g_app->modelCam, -ImGui::GetIO().DeltaTime * 30.0F, 0.0F);
}

// modell1: braucht der Model-Tab Bilder, obwohl niemand etwas tut (darfRuhen)?
// Eine laufende Animation und der Drehteller aendern das Bild aus sich heraus -
// solange die Ansicht gezeichnet wird.
bool modellBrauchtBilder() {
    if (g_app->modellAnsichtBild != ImGui::GetFrameCount() || g_app->model.empty()) { return false; }
    const bool spielt = g_app->animPlaying && !g_app->anim.empty() && g_app->animIndex >= 0 &&
                        static_cast<std::size_t>(g_app->animIndex) < g_app->animList.size();
    // Stufe 5: die Vorschau des Retargetings / Presets spielt ebenso aus sich heraus (sonst blieb sie im Leerlauf stehen, rt_zeit1)
    const bool rtSpielt = g_app->mtab.rt.vorschau && g_app->mtab.rt.spielt && !g_app->mtab.rt.welt.empty();
    return spielt || rtSpielt || g_app->modellDrehteller;
}

// Modell-Block 1a-1: die dritte Zeile - Primaer (ganzer Koerper) und
// Sekundaer (ab dem Sekundaer-Start) wie ModView, dazu die Zaehlung.
void modellSequenzZeile(const AnimEntry* e) {
    const ModellTab& t = g_app->mtab;
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", tr(Str::MbPrimaer));
    ImGui::SameLine();
    ImGui::TextUnformatted(e != nullptr ? e->name.c_str() : "-");
    ImGui::SameLine(0.0F, ImGui::GetFontSize() * 1.5F);
    char sek[160];
    std::snprintf(sek, sizeof sek, tr(Str::MbSekundaer), t.sekundaerKnochen.c_str());
    ImGui::TextDisabled("%s", sek);
    ImGui::SameLine();
    const bool hatSek = t.sekundaer >= 0 && static_cast<std::size_t>(t.sekundaer) < g_app->animList.size();
    ImGui::TextUnformatted(hatSek ? g_app->animList[static_cast<std::size_t>(t.sekundaer)].name.c_str() : "-");
    if (hatSek) {
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::MbSekundaerAus))) {
            g_app->mtab.sekundaer = -1;
            g_app->modelDirty = true;
        }
    }
    ImGui::SameLine(0.0F, ImGui::GetFontSize() * 1.5F);
    ImGui::TextDisabled("%s", tr(modellBlender() ? Str::MbZeitHinweisBlender : Str::MbZeitHinweis));   // mb1a2: Zaehlung je Modus
}

// --- Die Leiste unter allen drei Spalten -------------------------------------------------
void drawModelPanel() {
    modellKlickPruefen();   // Modell-Block 1a-1: die Leiste gehoert zum Model-Tab (Tasten wie 3ds Max)
    const bool have = !g_app->model.empty();
    const float fs = ImGui::GetFontSize();
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    const bool animDa = have && !g_app->anim.empty() && g_app->animIndex >= 0 &&
                        static_cast<std::size_t>(g_app->animIndex) < g_app->animList.size();
    const AnimEntry* e = animDa ? &g_app->animList[static_cast<std::size_t>(g_app->animIndex)] : nullptr;

    // --- Abspielen (die Uhr) -----------------------------------------------------
    modellRtUhr();   // Stufe 5: die Vorschau des Retargetings / Presets
    if (e != nullptr && g_app->animPlaying) {
        const bool wantLoop = (g_app->loopMode == 1) || (g_app->loopMode == 0 && e->loopFrame >= 0);
        g_app->animTime += ImGui::GetIO().DeltaTime * g_app->animSpeed * static_cast<float>(std::max(e->fps, 1));
        while (g_app->animTime >= 1.0F) {
            g_app->animTime -= 1.0F;
            ++g_app->animFrame;
            if (g_app->animFrame >= e->firstFrame + e->numFrames) {
                if (wantLoop) {
                    // Ab loopFrame, nicht ab dem Anfang: manche Animationen
                    // haben einen Vorlauf, der nur einmal laufen soll.
                    const int from = (g_app->loopMode == 1 && e->loopFrame < 0) ? 0 : std::max(e->loopFrame, 0);
                    g_app->animFrame = e->firstFrame + from;
                } else {
                    g_app->animFrame = e->firstFrame + e->numFrames - 1;
                    g_app->animPlaying = false;
                }
            }
            g_app->modelDirty = true;
        }
        // Modell-Block 1a-1: gemischt zwischen zwei Bildern (Interpolation)
        // aendert sich die Pose in jedem Bild, nicht nur beim Bildwechsel.
        if (g_app->mtab.interpolieren) { g_app->modelDirty = true; }
        // Die Sekundaeranimation im eigenen Takt (ab dem Sekundaer-Start).
        ModellTab& t = g_app->mtab;
        if (t.sekundaer >= 0 && static_cast<std::size_t>(t.sekundaer) < g_app->animList.size()) {
            const AnimEntry& s2 = g_app->animList[static_cast<std::size_t>(t.sekundaer)];
            const bool schleife = (g_app->loopMode == 1) || (g_app->loopMode == 0 && s2.loopFrame >= 0);
            t.sekZeit += ImGui::GetIO().DeltaTime * g_app->animSpeed * static_cast<float>(std::max(s2.fps, 1));
            while (t.sekZeit >= 1.0F) {
                t.sekZeit -= 1.0F;
                ++t.sekFrame;
                if (t.sekFrame >= s2.firstFrame + s2.numFrames) {
                    t.sekFrame = schleife ? s2.firstFrame + std::max(s2.loopFrame, 0) : s2.firstFrame + s2.numFrames - 1;
                }
                g_app->modelDirty = true;
            }
        }
    }

    // --- Zeile 1: Transportleiste ------------------------------------------------
    ImGui::BeginDisabled(e == nullptr);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{fs * 0.2F, ImGui::GetStyle().ItemSpacing.y});
    if (iconKnopf(tr(Str::TlRewind), UiIcon::ZumAnfang, nullptr) && e != nullptr) {
        g_app->animFrame = e->firstFrame;
        g_app->animTime = 0.0F;
        g_app->modelDirty = true;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelPrevFrame), UiIcon::Zurueck, nullptr) && e != nullptr) {
        g_app->animFrame = std::max(e->firstFrame, g_app->animFrame - 1);
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }
    ImGui::SameLine();
    if (iconKnopf(g_app->animPlaying ? tr(Str::ModelStop) : tr(Str::ModelPlay),
                  g_app->animPlaying ? UiIcon::Pause : UiIcon::Abspielen, nullptr, g_app->animPlaying)) {
        if (!g_app->animPlaying && e != nullptr && g_app->animFrame >= e->firstFrame + e->numFrames - 1) {
            g_app->animFrame = e->firstFrame;
        }
        g_app->animPlaying = !g_app->animPlaying;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::ModelNextFrame), UiIcon::Vor, nullptr) && e != nullptr) {
        g_app->animFrame = std::min(e->firstFrame + e->numFrames - 1, g_app->animFrame + 1);
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }
    // Bild und Zeit, gross
    ImGui::SameLine(0.0F, fs * 0.6F);
    {
        // Modell-Block 1a-1: wie 3ds Max ab 0 gezaehlt - 12 / 39 heisst Bild 12
        // von 0..39 (stufenplan.md, Entscheidung shank 04.11.: Zeitleiste im
        // Max-Modus ab 0, passt direkt zur animation.cfg).
        // mb1a2: wie Blender ab 1 (frame_start 1, blender_dump.txt) - 1 / 40 heisst das erste von 40 Bildern; der
        // GLA-Frame (ab 0) steht im Tooltip (stufenplan.md, Empfehlung zur Zaehlung).
        const bool abEins = modellBlender();
        const int rel = e != nullptr ? g_app->animFrame - e->firstFrame : 0;
        char jetzt[32];
        char gesamt[64];
        std::snprintf(jetzt, sizeof(jetzt), "%d", e != nullptr ? rel + (abEins ? 1 : 0) : (abEins ? 1 : 0));
        std::snprintf(gesamt, sizeof(gesamt), " / %d   %.2f s", e != nullptr ? std::max(e->numFrames - (abEins ? 0 : 1), 0) : 0,
                      e != nullptr ? static_cast<double>(rel) / static_cast<double>(std::max(e->fps, 1)) : 0.0);
        const ImVec2 t1 = ImGui::CalcTextSize(jetzt);
        const ImVec2 t2 = ImGui::CalcTextSize(gesamt);
        const float hh = ImGui::GetFrameHeight();
        ImGui::Dummy(ImVec2{t1.x + t2.x + fs * 0.8F, hh});
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding + 2.0F);
        dl->AddText(ImVec2{a.x + fs * 0.4F, a.y + (hh - t1.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), jetzt);
        dl->AddText(ImVec2{a.x + fs * 0.4F + t1.x, a.y + (hh - t1.y) * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                    gesamt);
        if (e != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip(tr(Str::MbGlaBild), g_app->animFrame, rel);
        }
        g_app->mtab.zaehlungText = std::string(jetzt) + gesamt;   // Selbsttest
    }
    ImGui::SameLine(0.0F, fs * 0.6F);
    // Schleife: aus der animation.cfg (aus), erzwungen (an)
    if (iconKnopf(tr(Str::ModelLoop), UiIcon::Schleife, tr(Str::ModelLoopHint), g_app->loopMode == 1, true, true)) {
        g_app->loopMode = g_app->loopMode == 1 ? 0 : 1;
    }
    ImGui::SameLine();
    {
        static const float kTempi[] = {0.1F, 0.25F, 0.5F, 1.0F, 1.5F, 2.0F, 3.0F};
        char tempo[16];
        std::snprintf(tempo, sizeof(tempo), "%gx", static_cast<double>(g_app->animSpeed));
        ImGui::SetNextItemWidth(fs * 3.4F);
        if (ImGui::BeginCombo("##modeltempo", tempo, ImGuiComboFlags_NoArrowButton)) {
            for (const float v : kTempi) {
                char n[16];
                std::snprintf(n, sizeof(n), "%gx", static_cast<double>(v));
                if (ImGui::Selectable(n, std::fabs(v - g_app->animSpeed) < 0.001F)) { g_app->animSpeed = v; }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::ModelSpeed)); }
    }
    ImGui::SameLine();
    // Modell-Block 1a-1: Interpolation an/aus (ModView Interpolate).
    if (iconKnopf(tr(Str::MbInterpolation), UiIcon::Animieren, tr(Str::MbInterpolationHint), g_app->mtab.interpolieren, true, true)) {
        g_app->mtab.interpolieren = !g_app->mtab.interpolieren;
        g_app->modelDirty = true;
    }
    ImGui::PopStyleVar();
    ImGui::EndDisabled();
    // Die gewaehlte Animation, ihr Name
    if (e != nullptr) {
        ImGui::SameLine(0.0F, fs * 0.8F);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(e->name.c_str());
    }
    // Haut und Flaechen rechts daneben
    if (have && !g_app->modelSkins.empty()) {
        ImGui::SameLine(0.0F, fs * 1.2F);
        ImGui::SetNextItemWidth(fs * 11.0F);
        const std::string current =
            (g_app->modelSkinIndex >= 0 && static_cast<std::size_t>(g_app->modelSkinIndex) < g_app->modelSkins.size())
                ? g_app->modelSkins[static_cast<std::size_t>(g_app->modelSkinIndex)]
                : std::string("-");
        if (ImGui::BeginCombo(tr(Str::ModelSkin), current.c_str())) {
            for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
                const bool selS = (static_cast<int>(i) == g_app->modelSkinIndex);
                if (ImGui::Selectable(g_app->modelSkins[i].c_str(), selS)) {
                    applySkinByIndex(static_cast<int>(i));
                    ++g_app->modellHautWechsel;
                }
                if (selS) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::ModelSkinHint)); }
        // H2: die Auswahlliste meldet ihre Beschriftung sonst nur aufgeklappt (Tour k-skins, Helfer).
        if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), tr(Str::ModelSkin)); }
        ImGui::SameLine();
        int shown = 0;
        int total = 0;
        for (const GlmSurface& sf : g_app->model.surfaces) {
            if (sf.isTag()) { continue; }
            ++total;
            if (sf.isVisible(g_app->showCaps)) { ++shown; }
        }
        ImGui::TextDisabled(trN(Str::ModelSurfaces, shown, total), shown, total);
        // Klick: alle Flaechen ins Detailprotokoll (siehe Watt Tambor, rc3xx).
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::ModelSurfaceDump)); }
        tourZielMelden("modell.flaechen", ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()});   // H2: Ziel der Tour k-skins
        if (ImGui::IsItemClicked()) {
            modellFlaechenProtokoll();
            ++g_app->modellFlaechenListen;
        }
    }

    // --- Zeile 2: Schieberegler ----------------------------------------------------
    if (e == nullptr) {
        int none = 0;
        ImGui::BeginDisabled();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderInt("##frameoff", &none, 0, 0, "");
        ImGui::EndDisabled();
        modellSequenzZeile(nullptr);
        modellMarkerZeile(nullptr);   // Stufe 2
        return;
    }
    int rel = g_app->animFrame - e->firstFrame;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderInt("##frame", &rel, 0, std::max(e->numFrames - 1, 0), "")) {
        g_app->animFrame = e->firstFrame + rel;
        g_app->animTime = 0.0F;
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }
    modellSequenzZeile(e);
    modellMarkerZeile(e);   // Stufe 2: die Zeile der animation.cfg, bearbeitbar
    (void)sp;
}

// Was der Zeichner von jeder Flaeche sieht - ins Detailprotokoll.
//
// Gemeldet: "bei Watt Tambor fehlt der Brustpanzer". Die Suche danach hat
// zwoelf Runden gebraucht, weil von aussen nicht zu sehen war, WELCHE Zahl
// nicht stimmt. Diese Ausgabe stellt alle nebeneinander.
void modellFlaechenProtokoll() {
    diag::info("Flaechen von \"" + g_app->model.name + "\":");
    for (std::size_t i = 0; i < g_app->model.surfaces.size(); ++i) {
        const GlmSurface& sf = g_app->model.surfaces[i];
        if (sf.isTag()) { continue; }
        float mn[3] = {1e9F, 1e9F, 1e9F};
        float mx[3] = {-1e9F, -1e9F, -1e9F};
        for (const GlmVertex& v : sf.verts) {
            for (int k = 0; k < 3; ++k) {
                mn[k] = std::min(mn[k], v.xyz[k]);
                mx[k] = std::max(mx[k], v.xyz[k]);
            }
        }
        std::set<int> knochen;
        for (const GlmVertex& v : sf.verts) {
            for (int k = 0; k < 4; ++k) {
                if (v.weights[k] > 0.0001F) { knochen.insert(static_cast<int>(v.bones[k])); }
            }
        }
        std::string kn;
        for (const int k : knochen) { kn += std::to_string(k) + " "; }
        const bool hatTextur = i < g_app->modelTextures.bySurface.size() && !g_app->modelTextures.bySurface[i].empty();
        char zeile[320];
        std::snprintf(zeile, sizeof zeile,
                      "   %-22s Ecken %-5zu Dreiecke %-5zu sichtbar %d Textur %d Flagge 0x%02x  x %.1f..%.1f y %.1f..%.1f "
                      "z %.1f..%.1f  Knochen %s",
                      sf.name.c_str(), sf.verts.size(), sf.indexes.size() / 3,
                      static_cast<int>(sf.isVisible(g_app->showCaps)), static_cast<int>(hatTextur),
                      static_cast<unsigned>(sf.flags), static_cast<double>(sf.verts.empty() ? 0.0F : mn[0]),
                      static_cast<double>(sf.verts.empty() ? 0.0F : mx[0]),
                      static_cast<double>(sf.verts.empty() ? 0.0F : mn[1]),
                      static_cast<double>(sf.verts.empty() ? 0.0F : mx[1]),
                      static_cast<double>(sf.verts.empty() ? 0.0F : mn[2]),
                      static_cast<double>(sf.verts.empty() ? 0.0F : mx[2]), kn.c_str());
        diag::info(zeile);
    }
}

}  // namespace bhed::gui
