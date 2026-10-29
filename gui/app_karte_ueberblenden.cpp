// app_karte_ueberblenden.cpp - Karten-Editor, Stufe 9 Punkt 9: Texturen ueberblenden (eigene
// Funktion, nicht in NetRadiant-Custom).
//
// View > Map tools > Texture blending: fuer die func_group der Auswahl (Gelaende aus Pinseln oder
// Patches) die Schluessel von q3map2 setzen (_indexmap, _layers, _shader; map.cpp
// LoadEntityIndexMap), die Index-Karte (32-Bit-TGA) und die Shader (Grund-Shader mit
// q3map_terrain, je Schicht und je Paar) in den Import-Ordner schreiben und allen Flaechen der
// Gruppe den Grund-Shader geben - ein Undo-Schritt. Danach malt man die Schichten mit der Maus in
// eine Draufsicht (oben = groesstes Y wie GetShaderIndexForPoint); q3map2 macht beim Bauen daraus
// die ueberblendeten Flaechen. Der Kern steht in src/karte/ueberblenden.cpp.
#include "karteneditor.h"

#include "app_internal.h"
#include "platform.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/ueberblenden.h"

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

struct BlendZustand {
    bool offen = false;
    int schichten = 2;
    char texturen[4][128] = {};
    int aufloesung = 1;   // 64, 128, 256
    int schicht = 1;      // die gemalte
    float radius = 64.0F;
    std::weak_ptr<karte::Entitaet> ent;   // die eingerichtete Gruppe
    karte::IndexKarte im;
    float mins[3] = {0, 0, 0};
    float maxs[3] = {0, 0, 0};
    std::string tgaPfad;   // Datei im Import-Ordner
    ImVec2 leinwandMin{0, 0}, leinwandMax{0, 0};
    bool malt = false;
};
BlendZustand& g_b = *new BlendZustand;

std::filesystem::path pfadVon(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// die func_group der Auswahl (das erste gewaehlte Element)
std::shared_ptr<karte::Entitaet> gruppeDerAuswahl() {
    for (const auto& w : e().karte.auswahl.elemente()) {
        auto en = w.entitaet.lock();
        if (en && en->wert("classname") == "func_group") { return en; }
    }
    return nullptr;
}

std::string kartenStamm() {
    std::string n = std::filesystem::path(pfadVon(e().name)).stem().string();
    if (n.empty()) { n = "unbenannt"; }
    for (char& c : n) {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_' && c != '-') { c = '_'; }
    }
    return n;
}

bool tgaSchreiben() {
    if (g_b.tgaPfad.empty()) { return false; }
    std::error_code ec;
    std::filesystem::create_directories(pfadVon(g_b.tgaPfad).parent_path(), ec);
    const std::vector<unsigned char> d = karte::blendTga(g_b.im);
    std::ofstream f(pfadVon(g_b.tgaPfad), std::ios::binary);
    f.write(reinterpret_cast<const char*>(d.data()), static_cast<std::streamsize>(d.size()));
    return static_cast<bool>(f);
}

// Die eingerichtete Gruppe wieder aufnehmen (Schluessel vorhanden): Grenzen und Index-Karte lesen
bool aufnehmen(const std::shared_ptr<karte::Entitaet>& en) {
    if (!en || en->wert("_indexmap").empty()) { return false; }
    const std::string imp = platform::settingsDirectory() + "\\import\\";
    std::string rel = en->wert("_indexmap");
    std::replace(rel.begin(), rel.end(), '/', '\\');
    const int n = std::clamp(std::atoi(en->wert("_layers").c_str()), 1, 4);
    std::ifstream f(pfadVon(imp + rel), std::ios::binary);
    const std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    karte::IndexKarte im;
    if (!karte::blendTgaLesen(d, n, im) || !karte::blendGrenzen(*en, e().karte.kontext, g_b.mins, g_b.maxs)) { return false; }
    g_b.im = std::move(im);
    g_b.ent = en;
    g_b.tgaPfad = imp + rel;
    g_b.schichten = n;
    g_b.schicht = std::min(g_b.schicht, n - 1);
    return true;
}

}  // namespace

bool ueberblendenOffen() { return g_b.offen; }
void ueberblendenUmschalten() {
    g_b.offen = !g_b.offen;
    if (g_b.offen && g_b.texturen[0][0] == 0) {   // Vorgabe: der Shader des Texturbrowsers
        std::snprintf(g_b.texturen[0], sizeof(g_b.texturen[0]), "%s", e().texBro.shader.c_str());
    }
}

void ueberblendenTextur(int schicht, const std::string& shader) {
    if (schicht >= 0 && schicht < 4) { std::snprintf(g_b.texturen[schicht], sizeof(g_b.texturen[schicht]), "%s", shader.c_str()); }
}
void ueberblendenSchichten(int n) { g_b.schichten = std::clamp(n, 2, 4); }
void ueberblendenPinsel(int schicht, float radius) {
    g_b.schicht = schicht;
    g_b.radius = radius;
}
const karte::IndexKarte& ueberblendenKarte() { return g_b.im; }
bool ueberblendenLeinwand(ImVec2& mi, ImVec2& ma) {
    mi = g_b.leinwandMin;
    ma = g_b.leinwandMax;
    return g_b.leinwandMax.x > g_b.leinwandMin.x;
}
std::string ueberblendenDatei() { return g_b.tgaPfad; }

// "Set up": Schluessel, Dateien, Grund-Shader auf alle Flaechen der Gruppe - ein Undo-Schritt
bool ueberblendenEinrichten() {
    Editor& ed = e();
    const auto en = gruppeDerAuswahl();
    if (!en) {
        fehler(trK(Str::KeUbKeineGruppe));
        return false;
    }
    std::vector<std::string> tex;
    for (int i = 0; i < g_b.schichten; ++i) {
        std::string t = g_b.texturen[i];
        if (t.empty()) {
            fehler(trK(Str::KeUbTexturFehlt));
            return false;
        }
        if (t.rfind("textures/", 0) != 0) { t = "textures/" + t; }
        tex.push_back(t);
    }
    std::size_t nr = 0;
    for (std::size_t i = 0; i < ed.karte.entitaeten().size(); ++i) {
        if (ed.karte.entitaeten()[i] == en) { nr = i; }
    }
    const std::string name = kartenStamm() + "_" + std::to_string(nr);
    const std::string basis = "jks_blend/" + name;   // _shader ohne "textures/" (GetIndexedShader setzt es davor)
    const std::string imp = platform::settingsDirectory() + "\\import";
    // die Shader-Datei
    {
        std::error_code ec;
        std::filesystem::create_directories(pfadVon(imp + "\\shaders"), ec);
        std::ofstream f(pfadVon(imp + "\\shaders\\jks_blend_" + name + ".shader"), std::ios::binary);
        f << karte::blendShaderText(basis, tex);
    }
    const int seiten[3] = {64, 128, 256};
    karte::IndexKarte im;
    im.breite = im.hoehe = seiten[std::clamp(g_b.aufloesung, 0, 2)];
    im.schichten = g_b.schichten;
    im.index.assign(static_cast<std::size_t>(im.breite * im.hoehe), 0);
    {
        karte::Befehl undo(ed.karte.undo, "textureBlendSetup");
        en->setzen("_indexmap", "maps/" + name + "_blend.tga");
        en->setzen("_layers", std::to_string(g_b.schichten));
        en->setzen("_shader", basis);
        for (const auto& c : en->kinder()) {
            if (!c) { continue; }
            if (c->art() == karte::Primitiv::Art::Pinsel) {
                auto& p = static_cast<karte::Pinsel&>(*c);
                for (std::size_t i = 0; i < p.anzahl(); ++i) {
                    const std::string& s = p[i].shader();
                    if (s.find("/caulk") != std::string::npos || s.find("/nodraw") != std::string::npos) { continue; }
                    p[i].setzeShader("textures/" + basis, ed.karte.kontext);
                }
            } else if (c->art() == karte::Primitiv::Art::Patch) {
                static_cast<karte::Patch&>(*c).setzeShader("textures/" + basis);
            }
        }
    }
    g_b.im = std::move(im);
    g_b.ent = en;
    g_b.tgaPfad = imp + "\\maps\\" + name + "_blend.tga";
    (void)karte::blendGrenzen(*en, ed.karte.kontext, g_b.mins, g_b.maxs);
    (void)tgaSchreiben();
    texturBefehl("RefreshShaders");   // der neue Grund-Shader im Editor (qer_editorimage)
    ++ed.sichtStand;
    char t[700];
    std::snprintf(t, sizeof(t), trK(Str::KeUbEingerichtet), basis.c_str(), g_b.tgaPfad.c_str());
    ausgabe(t);
    return true;
}

// Malen an einem Weltpunkt (x, y) mit Schicht und Radius des Fensters; true, wenn sich etwas aendert
bool ueberblendenMalen(float x, float y) {
    if (g_b.ent.expired() || g_b.im.index.empty()) { return false; }
    return karte::blendMalen(g_b.im, g_b.mins, g_b.maxs, x, y, g_b.radius, g_b.schicht) > 0;
}

void ueberblendenZeichnen() {
    if (!g_b.offen) { return; }
    const std::string titel = std::string(trK(Str::KeUbMenue)) + "###ueberblenden";
    ImGui::SetNextWindowSize(ImVec2(440, 620), ImGuiCond_FirstUseEver);
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen)) {
        const auto gr = gruppeDerAuswahl();
        if (gr && gr != g_b.ent.lock()) { (void)aufnehmen(gr); }   // schon eingerichtet: weiter malen
        ImGui::TextWrapped("%s", gr ? trK(Str::KeUbGruppe) : trK(Str::KeUbKeineGruppe));
        ImGui::SetNextItemWidth(120);
        ImGui::SliderInt((std::string(trK(Str::KeUbSchichten)) + "##ubsch").c_str(), &g_b.schichten, 2, 4);
        for (int i = 0; i < g_b.schichten; ++i) {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(260);
            ImGui::InputText(("##ubtex" + std::to_string(i)).c_str(), g_b.texturen[i], sizeof(g_b.texturen[i]));
            ImGui::SameLine();
            if (ImGui::Button((std::string(trK(Str::KeUbAktuell)) + "##ubakt").c_str())) { ueberblendenTextur(i, e().texBro.shader); }
            ImGui::PopID();
        }
        const char* aufl[3] = {"64 x 64", "128 x 128", "256 x 256"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo((std::string(trK(Str::KeUbAufloesung)) + "##ubaufl").c_str(), &g_b.aufloesung, aufl, 3);
        if (ImGui::Button((std::string(trK(Str::KeUbEinrichten)) + "##ubein").c_str())) { (void)ueberblendenEinrichten(); }
        ImGui::Separator();
        if (!g_b.ent.expired() && !g_b.im.index.empty()) {
            static const ImU32 farben[4] = {IM_COL32(70, 120, 60, 255), IM_COL32(150, 120, 80, 255), IM_COL32(150, 150, 160, 255), IM_COL32(220, 210, 160, 255)};
            for (int i = 0; i < g_b.im.schichten; ++i) {
                if (i > 0) { ImGui::SameLine(); }
                char z[64];
                std::snprintf(z, sizeof(z), trK(Str::KeUbSchicht), i);
                if (ImGui::RadioButton((std::string(z) + "##ubs" + std::to_string(i)).c_str(), g_b.schicht == i)) { g_b.schicht = i; }
            }
            ImGui::SetNextItemWidth(160);
            ImGui::SliderFloat((std::string(trK(Str::KeUbRadius)) + "##ubrad").c_str(), &g_b.radius, 8.0F, 512.0F, "%.0f");
            // die Draufsicht: Seitenverhaeltnis der Gruppe, oben = groesstes Y
            const float bx = g_b.maxs[0] - g_b.mins[0], by = g_b.maxs[1] - g_b.mins[1];
            const float platz = std::max(64.0F, std::min(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 8.0F));
            const float s = platz / std::max(bx, by);
            const ImVec2 o = ImGui::GetCursorScreenPos();
            const ImVec2 groesse(std::max(16.0F, bx * s), std::max(16.0F, by * s));
            ImGui::InvisibleButton("##ubleinwand", groesse);
            g_b.leinwandMin = o;
            g_b.leinwandMax = ImVec2(o.x + groesse.x, o.y + groesse.y);
            if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) {
                const ImVec2 m = ImGui::GetIO().MousePos;
                const float wx = g_b.mins[0] + (m.x - o.x) / groesse.x * bx;
                const float wy = g_b.maxs[1] - (m.y - o.y) / groesse.y * by;
                (void)ueberblendenMalen(wx, wy);
                g_b.malt = true;
            } else if (g_b.malt) {   // losgelassen: die Datei schreiben
                g_b.malt = false;
                if (tgaSchreiben()) {
                    char t[700];
                    std::snprintf(t, sizeof(t), trK(Str::KeUbGespeichert), g_b.tgaPfad.c_str());
                    ausgabe(t);
                }
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float pw = groesse.x / static_cast<float>(g_b.im.breite), ph = groesse.y / static_cast<float>(g_b.im.hoehe);
            for (int y = 0; y < g_b.im.hoehe; ++y) {   // je Zeile Laeufe gleicher Schicht als ein Rechteck
                const unsigned char* zeile = &g_b.im.index[static_cast<std::size_t>(y * g_b.im.breite)];
                for (int x = 0; x < g_b.im.breite;) {
                    int x2 = x + 1;
                    while (x2 < g_b.im.breite && zeile[x2] == zeile[x]) { ++x2; }
                    dl->AddRectFilled(ImVec2(o.x + static_cast<float>(x) * pw, o.y + static_cast<float>(y) * ph),
                                      ImVec2(o.x + static_cast<float>(x2) * pw, o.y + static_cast<float>(y + 1) * ph), farben[std::min<int>(zeile[x], 3)]);
                    x = x2;
                }
            }
            dl->AddRect(o, g_b.leinwandMax, IM_COL32(255, 255, 255, 160));
        }
    }
    ImGui::End();
    if (!offen) { g_b.offen = false; }
}

}  // namespace bhed::gui::karteneditor
