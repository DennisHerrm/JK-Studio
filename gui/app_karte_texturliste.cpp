// app_karte_texturliste.cpp - Karten-Editor (rc609 K): das Fenster "Texturen der Karte".
//
// Eigene Funktion (nicht in NetRadiant-Custom): alle benutzten Shader und Texturen
// mit Vorschaubild und Anzahl (Brush-Seiten, Brushes, Patches), sortier- und
// filterbar; ein Klick waehlt alle Flaechen damit; "Ersetzen durch ..." (die im
// Texturbrowser gewaehlte Textur aus einem beliebigen Ordner) und "Ordner
// ersetzen" mit der Liste der Zuordnungen und Luecken - auf Brushes und Patches
// in einem Rueckgaengig-Schritt, fuer die ganze Karte, nur die Auswahl oder nur
// das Sichtbare. Die Logik steht in src/karte/texturliste.cpp; "Find / Replace"
// (app_karte_texturen.cpp) bleibt wie in NRC.
//
//   This file is part of JK Studio, which is free software; you can
//   redistribute it and/or modify it under the terms of the GNU General
//   Public License as published by the Free Software Foundation; either
//   version 2 of the License, or (at your option) any later version.
//   It is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//   more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/texturliste.h"
#include "fenster_dock.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

struct Zustand {
    karte::TexturUmfang umfang = karte::TexturUmfang::Alle;
    std::string filter;
    std::string gewaehlt;   // die angeklickte Zeile
    std::string neu;        // Ersetzen durch
    bool weltGroesse = false;
    std::string altOrdner;
    std::string neuOrdner;
    std::vector<karte::OrdnerZuordnung> zuordnung;
    bool zuordnungDa = false;
    std::string meldung;
    bool meldungFehler = false;
    std::size_t stand = static_cast<std::size_t>(-1);
    karte::TexturUmfang standUmfang = karte::TexturUmfang::Alle;
    std::vector<karte::TexturZeile> liste;
    std::vector<std::pair<std::string, ImVec2>> zeilenOrte;   // fuer den Selbsttest: Mitte jeder sichtbaren Zeile
    std::vector<std::pair<std::string, ImVec2>> knopfOrte;
    float untenHoehe = 0.0F;   // Ersetzen, Ordner und Meldung im letzten Bild
};
Zustand& z() {
    static Zustand s;
    return s;
}

Editor& e() { return editor(); }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Gibt es den Shader im Spiel (Definition oder Bild)? Wie der Texturbrowser: Shader_ForName / CreateDefault.
bool vorhanden(const std::string& name) {
    const Editor& ed = e();
    if (!ed.karteSpiel) { return false; }
    const karte::Texturinventar& inv = ed.karteSpiel->inventar;
    return inv.shaderFinden(name) != nullptr || inv.bildMass(name).gefunden;
}

void listeAktualisieren() {
    Zustand& s = z();
    const std::size_t stand = szeneStand();
    if (stand == s.stand && s.umfang == s.standUmfang) { return; }
    s.stand = stand;
    s.standUmfang = s.umfang;
    s.liste = karte::texturListe(e().karte, s.umfang);
}

const karte::TexturZeile* zeile(const std::string& shader) {
    for (const karte::TexturZeile& t : z().liste) {
        if (karte::shaderNamenGleich(t.shader, shader)) { return &t; }
    }
    return nullptr;
}

std::string ordnerVon(const std::string& shader) {
    const std::size_t p = shader.rfind('/');
    return p == std::string::npos ? std::string() : shader.substr(0, p + 1);
}

void merkeKnopf(const char* id) {
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    z().knopfOrte.emplace_back(id, ImVec2{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F});
}

void meldenErsetzt(const karte::ErsetzenZahl& n) {
    Zustand& s = z();
    char t[256];
    if (n.seiten == 0 && n.patches == 0) {
        std::snprintf(t, sizeof(t), "%s", trK(Str::TxlNichts));
    } else {
        std::snprintf(t, sizeof(t), trKN(Str::TxlErsetzt, n.seiten, n.patches), n.seiten, n.patches);
    }
    s.meldung = t;
    s.meldungFehler = false;
}

void umfangZeichnen() {
    Zustand& s = z();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(trK(Str::TxlUmfang));
    const Str namen[3] = {Str::TxlAlle, Str::TxlAuswahl, Str::TxlSichtbar};
    for (int i = 0; i < 3; ++i) {
        ImGui::SameLine();
        if (ImGui::RadioButton((std::string(trK(namen[i])) + "##tlumfang" + std::to_string(i)).c_str(), static_cast<int>(s.umfang) == i)) {
            s.umfang = static_cast<karte::TexturUmfang>(i);
            s.zuordnung.clear();
            s.zuordnungDa = false;
        }
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    char puffer[256];
    std::snprintf(puffer, sizeof(puffer), "%s", s.filter.c_str());
    if (ImGui::InputTextWithHint("##tlfilter", trK(Str::TxlFilter), puffer, sizeof(puffer))) { s.filter = puffer; }
}

void tabelleZeichnen(float hoehe) {
    Zustand& s = z();
    Editor& ed = e();
    s.zeilenOrte.clear();
    const float bild = std::round(ImGui::GetFontSize() * 2.0F);
    const ImGuiTableFlags flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    if (!ImGui::BeginTable("##tltabelle", 6, flags, ImVec2{0.0F, hoehe})) { return; }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(trK(Str::TxlSpBild), ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, bild);
    ImGui::TableSetupColumn(trK(Str::TxlSpShader), ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(trK(Str::TxlSpSeiten), ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn(trK(Str::TxlSpBrushes), ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn(trK(Str::TxlSpPatches), ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableSetupColumn(trK(Str::TxlSpGroesse), ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::TableHeadersRow();
    // Gefiltert und sortiert (Name ohne Gross/klein; Zahlen, bei Gleichstand der Name)
    std::vector<const karte::TexturZeile*> v;
    const std::string f = klein(s.filter);
    for (const karte::TexturZeile& t : s.liste) {
        if (f.empty() || klein(t.shader).find(f) != std::string::npos) { v.push_back(&t); }
    }
    int spalte = 1;
    bool ab = false;
    if (const ImGuiTableSortSpecs* sp = ImGui::TableGetSortSpecs(); sp != nullptr && sp->SpecsCount > 0) {
        spalte = sp->Specs[0].ColumnIndex;
        ab = sp->Specs[0].SortDirection == ImGuiSortDirection_Descending;
    }
    const auto flaeche = [&ed](const karte::TexturZeile& t) {
        const auto [w, h] = ed.karte.kontext.groesse(t.shader);
        return w * h;
    };
    std::stable_sort(v.begin(), v.end(), [&](const karte::TexturZeile* a, const karte::TexturZeile* b) {
        long long x = 0;
        long long y = 0;
        switch (spalte) {
        case 2: x = a->seiten; y = b->seiten; break;
        case 3: x = a->pinsel; y = b->pinsel; break;
        case 4: x = a->patches; y = b->patches; break;
        case 5: x = static_cast<long long>(flaeche(*a)); y = static_cast<long long>(flaeche(*b)); break;
        default: break;
        }
        if (x == y) { return ab ? klein(b->shader) < klein(a->shader) : klein(a->shader) < klein(b->shader); }
        return ab ? x > y : x < y;
    });
    texturBilderAbholen();   // auch ohne sichtbaren Texturbrowser
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(v.size()), bild + ImGui::GetStyle().CellPadding.y * 2.0F);
    while (clip.Step()) {
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
            const karte::TexturZeile& t = *v[static_cast<std::size_t>(i)];
            ImGui::TableNextRow(ImGuiTableRowFlags_None, bild);
            ImGui::TableSetColumnIndex(0);
            if (void* b = texturBild(t.shader)) {
                ImGui::Image(reinterpret_cast<ImTextureID>(b), ImVec2{bild, bild});
            } else {
                ImGui::Dummy(ImVec2{bild, bild});
            }
            ImGui::TableSetColumnIndex(1);
            const bool aus = karte::shaderNamenGleich(t.shader, s.gewaehlt);
            const std::string id = t.shader + "##tlz";
            if (ImGui::Selectable(id.c_str(), aus, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2{0.0F, bild})) {
                texturListeKlick(t.shader, ImGui::GetIO().KeyCtrl);
            }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::TxlKlickHint)); }
            const ImVec2 a = ImGui::GetItemRectMin();
            const ImVec2 c = ImGui::GetItemRectMax();
            s.zeilenOrte.emplace_back(t.shader, ImVec2{(a.x + c.x) * 0.5F, (a.y + c.y) * 0.5F});
            if (kameraBildErsatz(t.shader) == 1) {   // notex / shadernotex wie in der Kamera
                ImGui::SameLine();
                ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.35F, 1.0F}, "%s", trK(Str::TxlBildFehlt));
            }
            const auto zahl = [](int n) {
                if (n > 0) {
                    ImGui::Text("%d", n);
                } else {
                    ImGui::TextDisabled("-");
                }
            };
            ImGui::TableSetColumnIndex(2);
            zahl(t.seiten);
            ImGui::TableSetColumnIndex(3);
            zahl(t.pinsel);
            ImGui::TableSetColumnIndex(4);
            zahl(t.patches);
            ImGui::TableSetColumnIndex(5);
            const auto [w, h] = ed.karte.kontext.groesse(t.shader);
            ImGui::Text("%zu x %zu", w, h);
        }
    }
    ImGui::EndTable();
}

void ersetzenZeichnen() {
    Zustand& s = z();
    const karte::TexturZeile* t = zeile(s.gewaehlt);
    if (t == nullptr) {
        ImGui::TextDisabled("%s", trK(Str::TxlKeineZeile));
        return;
    }
    char text[512];
    std::snprintf(text, sizeof(text), trKN(Str::TxlGewaehlt, t->seiten, t->patches), t->shader.c_str(), t->seiten, t->patches);
    ImGui::TextUnformatted(text);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(trK(Str::TxlErsetzenDurch));
    ImGui::SameLine();
    const std::string browser = std::string(trK(Str::TxlAusBrowser)) + "##tlbrowser";
    const float knopf = ImGui::CalcTextSize(trK(Str::TxlAusBrowser)).x + ImGui::GetStyle().FramePadding.x * 2.0F;
    ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize() * 6.0F, ImGui::GetContentRegionAvail().x - knopf - ImGui::GetStyle().ItemSpacing.x));
    char puffer[256];
    std::snprintf(puffer, sizeof(puffer), "%s", s.neu.c_str());
    if (ImGui::InputText("##tlneu", puffer, sizeof(puffer))) { s.neu = puffer; }
    ImGui::SameLine();
    if (ImGui::Button(browser.c_str())) { s.neu = browserShader(); }   // TextureBrowser_GetSelectedShader: aus jedem Ordner
    merkeKnopf("browser");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::TxlAusBrowserHint)); }
    (void)ImGui::Checkbox((std::string(trK(Str::TxlWeltgroesse)) + "##tlwelt").c_str(), &s.weltGroesse);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::TxlWeltgroesseHint)); }
    std::string neu = s.neu;
    std::replace(neu.begin(), neu.end(), '\\', '/');
    const bool gueltig = karte::texturNameGueltig(neu) && !karte::shaderNamenGleich(neu, t->shader);
    if (gueltig && !vorhanden(neu)) {
        std::snprintf(text, sizeof(text), trK(Str::TxlFehltNeu), neu.c_str());
        ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", text);
    }
    ImGui::BeginDisabled(!gueltig);
    if (ImGui::Button((std::string(trK(Str::TxlErsetzen)) + "##tlersetzen").c_str())) { (void)texturListeErsetzenDurch(neu, s.weltGroesse); }
    merkeKnopf("ersetzen");
    ImGui::EndDisabled();
}

void ordnerZeichnen() {
    Zustand& s = z();
    if (!ImGui::CollapsingHeader((std::string(trK(Str::TxlOrdner)) + "###tlordner").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    const float beschriftung = std::max(ImGui::CalcTextSize(trK(Str::TxlOrdnerAlt)).x, ImGui::CalcTextSize(trK(Str::TxlOrdnerNeu)).x) +
                               ImGui::GetStyle().ItemSpacing.x;
    const auto feld = [&](Str name, std::string& wert, const char* id) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(trK(name));
        ImGui::SameLine(beschriftung + ImGui::GetStyle().WindowPadding.x);
        ImGui::SetNextItemWidth(-FLT_MIN);
        char puffer[256];
        std::snprintf(puffer, sizeof(puffer), "%s", wert.c_str());
        if (ImGui::InputText(id, puffer, sizeof(puffer))) {
            wert = puffer;
            s.zuordnung.clear();
            s.zuordnungDa = false;
        }
    };
    feld(Str::TxlOrdnerAlt, s.altOrdner, "##tlalt");
    feld(Str::TxlOrdnerNeu, s.neuOrdner, "##tlneuordner");
    if (ImGui::Button((std::string(trK(Str::TxlOrdnerBrowser)) + "##tlordnerbrowser").c_str())) {   // g_TextureBrowser_currentDirectory
        s.neuOrdner = e().texBro.verzeichnis;
        s.zuordnung.clear();
        s.zuordnungDa = false;
    }
    merkeKnopf("ordnerbrowser");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::TxlOrdnerBrowserHint)); }
    ImGui::SameLine();
    if (ImGui::Button((std::string(trK(Str::TxlZuordnung)) + "##tlzuordnung").c_str())) { texturListeOrdnerZeigen(s.altOrdner, s.neuOrdner); }
    merkeKnopf("zuordnung");
    if (!s.zuordnungDa) { return; }
    int luecken = 0;
    for (const karte::OrdnerZuordnung& o : s.zuordnung) { luecken += o.fehlt ? 1 : 0; }
    char text[256];
    std::snprintf(text, sizeof(text), trKN(Str::TxlZuordnungZahl, s.zuordnung.size(), luecken), static_cast<int>(s.zuordnung.size()), luecken);
    ImGui::TextUnformatted(text);
    if (!s.zuordnung.empty() &&
        ImGui::BeginTable("##tlzuordnungen", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY,
                          ImVec2{0.0F, std::min(ImGui::GetTextLineHeightWithSpacing() * (static_cast<float>(s.zuordnung.size()) + 1.5F),
                                                ImGui::GetTextLineHeightWithSpacing() * 9.0F)})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(trK(Str::TxlOrdnerAlt));
        ImGui::TableSetupColumn(trK(Str::TxlOrdnerNeu));
        ImGui::TableHeadersRow();
        for (const karte::OrdnerZuordnung& o : s.zuordnung) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(o.alt.c_str());
            ImGui::TableSetColumnIndex(1);
            if (o.fehlt) {
                ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.35F, 1.0F}, "%s (%s)", o.neu.c_str(), trK(Str::TxlLuecke));
            } else {
                ImGui::TextUnformatted(o.neu.c_str());
            }
        }
        ImGui::EndTable();
    }
    const int treffer = static_cast<int>(s.zuordnung.size()) - luecken;
    ImGui::BeginDisabled(treffer == 0);
    std::snprintf(text, sizeof(text), trKN(Str::TxlOrdnerErsetzen, treffer), treffer);
    if (ImGui::Button((std::string(text) + "###tlordnerersetzen").c_str())) { (void)texturListeOrdnerErsetzen(); }
    merkeKnopf("ordnerersetzen");
    ImGui::EndDisabled();
}

}  // namespace

// --- oeffentlich ------------------------------------------------------------------------------

void texturListeOeffnen() {
    dock::ausblenden("karte.texliste", false);
    ImGui::SetWindowFocus(dock::fensterName("karte.texliste").c_str());
}

void texturListeZeichnen() {
    if (g_app->leftMode != 3 || !g_app->settings.texListeOffen) { return; }
    Zustand& s = z();
    s.knopfOrte.clear();
    listeAktualisieren();
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 36.0F, fs * 40.0F}, ImGuiCond_FirstUseEver);
    if (dock::fensterBeginnen("karte.texliste")) {
        umfangZeichnen();
        if (s.liste.empty()) {
            ImGui::TextDisabled("%s", trK(Str::TxlLeer));
        } else {
            // Die Liste nimmt, was unter ihr nicht gebraucht wird (Hoehe des unteren Teils aus dem letzten Bild):
            // Ersetzen und Ordner bleiben im Fenster, solange es hoch genug ist.
            tabelleZeichnen(std::max(fs * 6.0F, ImGui::GetContentRegionAvail().y - s.untenHoehe));
        }
        const float untenAb = ImGui::GetCursorPosY();
        ImGui::Separator();
        ersetzenZeichnen();
        ImGui::Separator();
        ordnerZeichnen();
        if (!s.meldung.empty()) {
            ImGui::Separator();
            if (s.meldungFehler) {
                ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.35F, 1.0F}, "%s", s.meldung.c_str());
            } else {
                ImGui::TextWrapped("%s", s.meldung.c_str());
            }
        }
        s.untenHoehe = ImGui::GetCursorPosY() - untenAb;
    }
    dock::fensterEnde();
}

void texturListeUmfang(karte::TexturUmfang u) { z().umfang = u; }

const std::vector<karte::TexturZeile>& texturListeZeilen() {
    listeAktualisieren();
    return z().liste;
}

void texturListeKlick(const std::string& shader, bool dazu) {
    Zustand& s = z();
    s.gewaehlt = shader;
    s.altOrdner = ordnerVon(shader);
    s.zuordnung.clear();
    s.zuordnungDa = false;
    const auto [seiten, patches] = karte::texturListeWaehlen(e().karte, shader, s.umfang, dazu);
    auswahlGeaendert();
    char t[512];
    std::snprintf(t, sizeof(t), trKN(Str::TxlGewaehltN, seiten, patches), seiten, patches);
    s.meldung = t;
    s.meldungFehler = false;
}

karte::ErsetzenZahl texturListeErsetzenDurch(const std::string& neu, bool weltGroesse) {
    Zustand& s = z();
    s.neu = neu;
    s.weltGroesse = weltGroesse;
    if (!karte::texturNameGueltig(neu)) {
        char t[512];
        std::snprintf(t, sizeof(t), trK(Str::TxlUngueltig), neu.c_str());
        s.meldung = t;
        s.meldungFehler = true;
        return {};
    }
    const karte::ErsetzenZahl n = karte::texturListeErsetzen(e().karte, {{s.gewaehlt, neu}}, s.umfang, weltGroesse);
    auswahlGeaendert();
    meldenErsetzt(n);
    if (n.seiten + n.patches > 0) { s.gewaehlt = neu; }   // die Zeile bleibt die ersetzte Textur
    return n;
}

void texturListeOrdnerZeigen(const std::string& alt, const std::string& neu) {
    Zustand& s = z();
    listeAktualisieren();
    s.altOrdner = karte::texturOrdnerName(alt);
    s.neuOrdner = karte::texturOrdnerName(neu);
    s.zuordnung = karte::texturOrdnerZuordnung(s.liste, s.altOrdner, s.neuOrdner, vorhanden);
    s.zuordnungDa = true;
}

const std::vector<karte::OrdnerZuordnung>& texturListeZuordnung() { return z().zuordnung; }

karte::ErsetzenZahl texturListeOrdnerErsetzen() {
    Zustand& s = z();
    std::vector<karte::TexturErsatz> paare;
    for (const karte::OrdnerZuordnung& o : s.zuordnung) {
        if (!o.fehlt) { paare.push_back({o.alt, o.neu}); }
    }
    const karte::ErsetzenZahl n = karte::texturListeErsetzen(e().karte, paare, s.umfang, s.weltGroesse);
    auswahlGeaendert();
    meldenErsetzt(n);
    s.zuordnung.clear();
    s.zuordnungDa = false;
    return n;
}

const std::string& texturListeMeldung() { return z().meldung; }

ImVec2 texturListeOrt(const std::string& was) {
    for (const auto& [name, p] : z().zeilenOrte) {
        if (karte::shaderNamenGleich(name, was)) { return p; }
    }
    for (const auto& [name, p] : z().knopfOrte) {
        if (name == was) { return p; }
    }
    return ImVec2{-1.0F, -1.0F};
}

}  // namespace bhed::gui::karteneditor
