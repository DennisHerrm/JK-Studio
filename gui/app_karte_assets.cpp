// app_karte_assets.cpp - Karten-Editor, Stufe 9 Punkt 11: Asset-Browser (eigene Funktion,
// nicht in NetRadiant-Custom).
//
// View > Map tools > Asset browser: Modelle, Prefabs und Klaenge (karte::assetsSammeln) mit
// Vorschau - der erste Shader eines MD3 als Bild (texturBild wie der Texturbrowser), der
// Grundriss eines Prefabs, die Wellenform eines Klangs -, Suche, Ziehen auf eine 2D-Ansicht oder
// die Kamera (oder Doppelklick: vor die Kamera): ein Modell wird misc_model, ein Klang
// target_speaker mit "noise", ein Prefab wird importiert (Map_ImportFile) und an den Ort
// geschoben. "Save selection as prefab" schreibt die Auswahl (Map_SaveSelected) nach
// <Import-Ordner>/prefabs.
#include "karteneditor.h"

#include "app_internal.h"
#include "platform.h"

#include "bhed/karte/assets.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/sound.h"

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

struct AssetZustand {
    bool offen = false;
    int art = 0;
    char suche[128] = {};
    char prefabName[64] = "prefab";
    std::vector<karte::Asset> liste;
    const void* spiel = nullptr;   // fuer welches Spiel gesammelt
    bool neu = true;
    std::map<std::string, std::string> shader;
    std::map<std::string, std::array<float, 64>> wellen;
    std::map<std::string, karte::PrefabUmriss> umrisse;
    std::map<std::size_t, ImVec2> zellen;   // Mitte jeder sichtbaren Zelle (Index in liste), fuer den Selbsttest
    bool posSetzen = false;
    ImVec2 pos;
};
AssetZustand& g_a = *new AssetZustand;

std::string prefabOrdner() { return platform::settingsDirectory() + "\\import\\prefabs"; }

void sammeln() {
    Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    std::error_code ec;
    const std::string imp = platform::settingsDirectory() + "\\import";
    g_a.liste = karte::assetsSammeln(sp ? &sp->fs : nullptr, std::filesystem::is_directory(std::filesystem::path(imp), ec) ? imp : std::string());
    g_a.spiel = sp.get();
    g_a.neu = false;
}

std::string lesen(const std::string& spielPfad) {
    Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    std::string b;
    if (sp) { (void)sp->fs.dateiLesen(spielPfad, b); }
    return b;
}

void vorschau(ImDrawList* dl, const karte::Asset& a, ImVec2 o, float s) {
    const ImU32 rand = IM_COL32(90, 90, 90, 255);
    dl->AddRect(o, ImVec2(o.x + s, o.y + s), rand);
    if (a.art == karte::AssetArt::Modell) {
        auto it = g_a.shader.find(a.pfad);
        if (it == g_a.shader.end()) {
            const std::string sh = a.name.size() > 4 && a.name.substr(a.name.size() - 4) == ".md3" ? karte::md3ErsterShader(lesen(a.pfad)) : std::string();
            it = g_a.shader.emplace(a.pfad, sh).first;
        }
        if (void* bild = it->second.empty() ? nullptr : texturBild(it->second)) {
            dl->AddImage(reinterpret_cast<ImTextureID>(bild), ImVec2(o.x + 2, o.y + 2), ImVec2(o.x + s - 2, o.y + s - 2));
        } else {
            dl->AddText(ImVec2(o.x + 6, o.y + s * 0.4F), IM_COL32(200, 200, 200, 255), "MD3");
        }
    } else if (a.art == karte::AssetArt::Klang) {
        auto it = g_a.wellen.find(a.pfad);
        if (it == g_a.wellen.end()) {
            const std::string b = lesen(a.pfad);
            const sound::Sound so = sound::decode(reinterpret_cast<const unsigned char*>(b.data()), b.size());
            it = g_a.wellen.emplace(a.pfad, karte::wellenform(so)).first;
        }
        const float mitte = o.y + s * 0.5F;
        for (int i = 0; i < 64; ++i) {
            const float x = o.x + 2 + (s - 4) * static_cast<float>(i) / 64.0F;
            const float h = it->second[static_cast<std::size_t>(i)] * (s * 0.45F);
            dl->AddLine(ImVec2(x, mitte - h), ImVec2(x, mitte + h), IM_COL32(110, 200, 255, 255));
        }
    } else {
        auto it = g_a.umrisse.find(a.pfad);
        if (it == g_a.umrisse.end()) {
            std::string t;
            {
                std::ifstream f(std::filesystem::path(std::u8string(a.pfad.begin(), a.pfad.end())), std::ios::binary);
                t.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            }
            it = g_a.umrisse.emplace(a.pfad, karte::prefabUmriss(t)).first;
        }
        const karte::PrefabUmriss& u = it->second;
        const float w = std::max(1.0F, std::max(u.x1 - u.x0, u.y1 - u.y0));
        const float k = (s - 8) / w;
        for (const auto& q : u.kaesten) {   // von oben, y nach oben
            dl->AddRect(ImVec2(o.x + 4 + (q[0] - u.x0) * k, o.y + s - 4 - (q[3] - u.y0) * k), ImVec2(o.x + 4 + (q[2] - u.x0) * k, o.y + s - 4 - (q[1] - u.y0) * k),
                        IM_COL32(230, 200, 120, 255));
        }
    }
}

}  // namespace

bool assetBrowserOffen() { return g_a.offen; }
void assetBrowserUmschalten() {
    g_a.offen = !g_a.offen;
    g_a.neu = true;
}

bool assetAbgelegt(std::size_t index, ImVec2 punkt) {
    Editor& ed = e();
    if (index >= g_a.liste.size()) { return false; }
    const karte::Asset a = g_a.liste[index];
    const karte::Vektor3f ort = ablageOrt(punkt);
    char o[96];
    std::snprintf(o, sizeof(o), "%g %g %g", static_cast<double>(ort[0]), static_cast<double>(ort[1]), static_cast<double>(ort[2]));
    if (a.art == karte::AssetArt::Prefab) {
        if (!karteImportierenAus(a.pfad)) { return false; }
        karte::Kasten b;
        if (auswahlGrenzen(b)) {
            const karte::Vektor3f boden(b.origin[0], b.origin[1], b.origin[2] - b.extents[2]);
            ed.bearbeitung.verschiebeAuswahl(ort - boden);
        }
    } else {
        karte::Befehl undo(ed.karte.undo, a.art == karte::AssetArt::Modell ? "entityCreate -class misc_model" : "entityCreate -class target_speaker");
        ed.karte.auswahl.leeren();
        auto ent = ed.karte.neueEntitaet(a.art == karte::AssetArt::Modell ? "misc_model" : "target_speaker", false);
        ent->setzen("origin", o);
        ent->setzen(a.art == karte::AssetArt::Modell ? "model" : "noise", a.pfad);
        ed.karte.entitaetEinfuegen(ent);
        ed.karte.auswahl.waehlen(ent);
    }
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[600];
    std::snprintf(t, sizeof(t), trK(Str::KeAbAbgelegt), a.pfad.c_str(), static_cast<double>(ort[0]), static_cast<double>(ort[1]), static_cast<double>(ort[2]));
    ausgabe(t);
    return true;
}

std::vector<karte::Asset> assetListe(int art) {
    if (g_a.neu || g_a.spiel != (e().karteSpiel ? e().karteSpiel.get() : e().spiel.get())) { sammeln(); }
    std::vector<karte::Asset> r;
    for (const std::size_t i : karte::assetsFiltern(g_a.liste, static_cast<karte::AssetArt>(art), g_a.suche)) { r.push_back(g_a.liste[i]); }
    return r;
}

void assetSucheSetzen(const std::string& suche, int art) {
    std::snprintf(g_a.suche, sizeof(g_a.suche), "%s", suche.c_str());
    g_a.art = art;
}

bool assetZelleMitte(std::size_t index, ImVec2& aus) {
    const auto it = g_a.zellen.find(index);
    if (it == g_a.zellen.end()) { return false; }
    aus = it->second;
    return true;
}

void assetBrowserNach(ImVec2 ort) {
    g_a.posSetzen = true;
    g_a.pos = ort;
}

std::size_t assetIndex(const std::string& pfad) {
    for (std::size_t i = 0; i < g_a.liste.size(); ++i) {
        if (g_a.liste[i].pfad == pfad) { return i; }
    }
    return static_cast<std::size_t>(-1);
}

bool prefabSpeichern(const std::string& name) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(prefabOrdner()), ec);
    std::string n;
    for (const char c : name) { n += (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-') ? c : '_'; }
    if (n.empty()) { n = "prefab"; }
    const std::string pfad = prefabOrdner() + "\\" + n + ".map";
    if (!auswahlSpeichernUnter(pfad)) { return false; }
    g_a.neu = true;
    g_a.umrisse.erase(pfad);
    char t[600];
    std::snprintf(t, sizeof(t), trK(Str::KeAbGespeichert), pfad.c_str());
    ausgabe(t);
    return true;
}

void assetAblageZiel() {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w == nullptr || !ImGui::GetDragDropPayload()) { return; }
    if (ImGui::BeginDragDropTargetCustom(w->Rect(), w->ID)) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("JKS_ASSET")) {
            std::size_t i = 0;
            std::memcpy(&i, p->Data, sizeof(i));
            (void)assetAbgelegt(i, ImGui::GetIO().MousePos);
        }
        ImGui::EndDragDropTarget();
    }
}

void assetBrowserZeichnen() {
    if (!g_a.offen) { return; }
    if (g_a.neu || g_a.spiel != (e().karteSpiel ? e().karteSpiel.get() : e().spiel.get())) { sammeln(); }
    const std::string titel = std::string(trK(Str::KeAbMenue)) + "###assetbrowser";
    ImGui::SetNextWindowSize(ImVec2(560, 480), ImGuiCond_FirstUseEver);
    if (g_a.posSetzen) {
        ImGui::SetNextWindowPos(g_a.pos);
        g_a.posSetzen = false;
    }
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen)) {
        const Str arten[3] = {Str::KeAbModelle, Str::KeAbPrefabs, Str::KeAbKlaenge};
        for (int i = 0; i < 3; ++i) {
            if (i > 0) { ImGui::SameLine(); }
            if (ImGui::RadioButton((std::string(trK(arten[i])) + "##abart" + std::to_string(i)).c_str(), g_a.art == i)) { g_a.art = i; }
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeAbNeu)) + "##abneu").c_str())) { g_a.neu = true; }
        ImGui::SetNextItemWidth(260);
        ImGui::InputText((std::string(trK(Str::KeAbSuche)) + "##absuche").c_str(), g_a.suche, sizeof(g_a.suche));
        const std::vector<std::size_t> treffer = karte::assetsFiltern(g_a.liste, static_cast<karte::AssetArt>(g_a.art), g_a.suche);
        ImGui::SameLine();
        ImGui::Text(trK(Str::KeAbAnzahl), static_cast<int>(treffer.size()));
        if (g_a.art == 1) {
            ImGui::SetNextItemWidth(160);
            ImGui::InputText("##abprefabname", g_a.prefabName, sizeof(g_a.prefabName));
            ImGui::SameLine();
            if (ImGui::Button((std::string(trK(Str::KeAbPrefabSpeichern)) + "##abprefab").c_str())) { (void)prefabSpeichern(g_a.prefabName); }
        }
        g_a.zellen.clear();
        if (ImGui::BeginChild("##abraster", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
            const float zelle = 96.0F, rand = 8.0F;
            const int spalten = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + rand) / (zelle + rand)));
            ImGuiListClipper clip;
            const int zeilen = (static_cast<int>(treffer.size()) + spalten - 1) / spalten;
            clip.Begin(zeilen, zelle + 22.0F);
            while (clip.Step()) {
                for (int z = clip.DisplayStart; z < clip.DisplayEnd; ++z) {
                    for (int sp = 0; sp < spalten; ++sp) {
                        const std::size_t k = static_cast<std::size_t>(z * spalten + sp);
                        if (k >= treffer.size()) { break; }
                        if (sp > 0) { ImGui::SameLine(0, rand); }
                        const karte::Asset& a = g_a.liste[treffer[k]];
                        ImGui::BeginGroup();
                        const ImVec2 o = ImGui::GetCursorScreenPos();
                        ImGui::InvisibleButton(("##ab" + a.pfad).c_str(), ImVec2(zelle, zelle));
                        g_a.zellen[treffer[k]] = ImVec2(o.x + zelle * 0.5F, o.y + zelle * 0.5F);
                        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", a.pfad.c_str()); }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) { (void)assetAbgelegt(treffer[k], ImVec2(-1, -1)); }
                        if (ImGui::BeginDragDropSource()) {
                            const std::size_t i = treffer[k];
                            ImGui::SetDragDropPayload("JKS_ASSET", &i, sizeof(i));
                            ImGui::TextUnformatted(a.name.c_str());
                            ImGui::EndDragDropSource();
                        }
                        vorschau(ImGui::GetWindowDrawList(), a, o, zelle);
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + zelle);
                        ImGui::TextUnformatted(a.name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndGroup();
                    }
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!offen) { g_a.offen = false; }
}

}  // namespace bhed::gui::karteneditor
