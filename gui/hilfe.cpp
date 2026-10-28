// hilfe.cpp - der Helfer im Programm (Stufe H1, siehe hilfe.h)
//
// Leitlinien (docs/intern/NOTIZEN_hilfe1.md): Hilfe auf Abruf statt Vortrag
// (NN/g), ein Treffer = eine kurze Erklaerung + eine Handlung, nichts wird
// ohne Klick veraendert, alles jederzeit abbrechbar.
#include "hilfe.h"
#include "app_internal.h"
#include "ki.h"
#include "tour.h"
#include "bhed/hilfe.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

struct Zustand {
    bool offen = false;
    bool fokus = false;
    char frage[256] = "";
    std::string letzte = "\x01";
    hilfe::Index index;
    std::string indexSprache;
    const tour::Touren* indexTouren = nullptr;
    std::size_t indexAbschnitte = 0;
    std::vector<hilfe::Treffer> treffer;
    bool ersteSchritte = false;
    bool ersteGeprueft = false;
};

Zustand& z() {
    static Zustand s;
    return s;
}

const char* sprachCode() {
    switch (i18n::currentLanguage()) {
        case i18n::Language::German: return "de";
        case i18n::Language::ChineseSimplified: return "zh";
        case i18n::Language::Japanese: return "ja";
        default: return "en";
    }
}

// Der Index in der eingestellten Sprache - neu nach einem Sprachwechsel.
const hilfe::Index& index() {
    Zustand& s = z();
    const tour::Touren& t = tourDaten();
    const std::vector<tour::Abschnitt>& hb = tourHandbuch();
    if (s.indexSprache != sprachCode() || s.indexTouren != &t || s.indexAbschnitte != hb.size() || s.index.eintraege().empty()) {
        s.index = hilfe::baue(t, hb, i18n::currentLanguage());
        s.indexSprache = sprachCode();
        s.indexTouren = &t;
        s.indexAbschnitte = hb.size();
        s.letzte = "\x01";
        diag::detail("Helfer: Index " + std::to_string(s.index.eintraege().size()) + " Eintraege (" + s.indexSprache + ")");
    }
    return s.index;
}

const char* artName(hilfe::Art a) {
    switch (a) {
        case hilfe::Art::Tour: return "tour";
        case hilfe::Art::Seite: return "seite";
        default: return "ort";
    }
}

std::string ersterSatz(const std::string& t) {
    std::string s = t;
    for (const char* ende : {". ", "\xE3\x80\x82", "! ", "? "}) {
        const std::size_t p = s.find(ende);
        if (p != std::string::npos && p > 10) { s.resize(p + std::strlen(ende)); }
    }
    if (s.size() > 200) {
        std::size_t p = 200;
        while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0U) == 0x80U) { --p; }
        s = s.substr(0, p) + "...";
    }
    return s;
}

bool zeige(const hilfe::Eintrag& e) {
    switch (e.art) {
        case hilfe::Art::Tour:
            return tourStarten(e.tour, e.ab);
        case hilfe::Art::Ort: {
            char text[600];
            std::snprintf(text, sizeof(text), tr(Str::HlfHere), e.pfad.c_str());
            return tourZeigeOrt(e.titel, text, e.ziel, e.voraus);
        }
        case hilfe::Art::Seite:
            handbuchOeffnen(e.seite.c_str());
            return true;
    }
    return false;
}

void trefferZeichnen(const hilfe::Index& ix, float breite) {
    Zustand& s = z();
    const float fs = ImGui::GetFontSize();
    int nr = 0;
    for (const hilfe::Treffer& t : s.treffer) {
        const hilfe::Eintrag& e = ix.eintraege()[t.eintrag];
        ImGui::PushID(nr++);
        ImGui::Separator();
        const char* art = e.art == hilfe::Art::Tour ? tr(Str::HlfKindTour) : e.art == hilfe::Art::Seite ? tr(Str::HlfKindPage) : tr(Str::HlfKindPlace);
        ImGui::TextDisabled("%s", art);
        ImGui::SameLine();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
        ImGui::TextUnformatted(e.titel.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
        if (e.art == hilfe::Art::Ort && !e.pfad.empty()) { ImGui::TextDisabled(tr(Str::HlfWhere), e.pfad.c_str()); }
        if (e.art == hilfe::Art::Tour && e.ab > 0) { ImGui::TextDisabled(tr(Str::HlfFromStep), e.ab + 1); }
        const std::string satz = ersterSatz(e.text);
        if (!satz.empty()) { ImGui::TextWrapped("%s", satz.c_str()); }
        ImGui::PopTextWrapPos();
        if (e.art != hilfe::Art::Seite) {
            if (ImGui::Button(tr(Str::HlfShowMe))) {
                diag::detail("Helfer: Zeig es mir " + e.id);
                (void)zeige(e);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::HlfShowMeTip)); }
            ImGui::SameLine();
        }
        if (!e.seite.empty()) {
            if (ImGui::Button(tr(Str::HlfGuide))) { handbuchOeffnen(e.seite.c_str()); }
        }
        (void)fs;
        ImGui::PopID();
    }
}

void fensterZeichnen() {
    Zustand& s = z();
    if (!s.offen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 30.0F, fs * 30.0F}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(tr(Str::HlfTitle)) + "###hilfe";
    if (!ImGui::Begin(titel.c_str(), &s.offen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    const hilfe::Index& ix = index();
    const float breite = ImGui::GetContentRegionAvail().x;
    ImGui::TextUnformatted(tr(Str::HlfAsk));
    if (s.fokus) {
        ImGui::SetKeyboardFocusHere();
        s.fokus = false;
    }
    ImGui::SetNextItemWidth(breite);
    ImGui::InputTextWithHint("##hilfefrage", tr(Str::HlfExample), s.frage, sizeof(s.frage));
    if (s.letzte != s.frage) {
        s.letzte = s.frage;
        s.treffer = ix.frage(s.frage, 5);
    }
    if (s.frage[0] == '\0') {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + breite);
        ImGui::TextDisabled("%s", tr(Str::HlfMenuHint));
        ImGui::PopTextWrapPos();
        if (ImGui::Button(tr(Str::HlfAllTutorials))) { tourFensterOeffnen(); }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::HlfGuide))) { handbuchOeffnen(); }
    } else if (s.treffer.empty()) {
        ImGui::TextDisabled("%s", tr(Str::HlfNone));
        if (ImGui::Button(tr(Str::HlfGuide))) { handbuchOeffnen(); }
    } else {
        trefferZeichnen(ix, breite);
    }
    // Freiwillig: Claude fragen (ab Werk aus, gui/ki.cpp).
    ImGui::Separator();
    kiAbschnitt(s.frage, breite);
    ImGui::End();
}

void ersteSchritteZeichnen() {
    Zustand& s = z();
    if (!s.ersteGeprueft && g_app != nullptr) {
        s.ersteGeprueft = true;
        s.ersteSchritte = !g_app->settings.ersteSchritteGezeigt && std::getenv("BHED_EDITORTEST") == nullptr;
    }
    if (!s.ersteSchritte) { return; }
    const float fs = ImGui::GetFontSize();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::SetNextWindowSize(ImVec2{fs * 26.0F, 0.0F}, ImGuiCond_Appearing);
    bool offen = true;
    const std::string titel = std::string(tr(Str::EsTitle)) + "###erstestart";
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushTextWrapPos(fs * 25.0F);
        ImGui::TextUnformatted(tr(Str::EsIntro));
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        int wahl = -1;
        const auto knopf = [&](int nr, Str text, Str tip) {
            if (ImGui::Button(tr(text), ImVec2{fs * 25.0F, 0.0F})) { wahl = nr; }
            ImGui::PushTextWrapPos(fs * 25.0F);
            ImGui::TextDisabled("%s", tr(tip));
            ImGui::PopTextWrapPos();
        };
        knopf(0, Str::EsScript, Str::EsScriptTip);
        knopf(1, Str::EsMap, Str::EsMapTip);
        knopf(2, Str::EsScene, Str::EsSceneTip);
        knopf(3, Str::EsModel, Str::EsModelTip);
        if (ImGui::Button(tr(Str::EsLook), ImVec2{fs * 25.0F, 0.0F})) { wahl = 4; }
        ImGui::Spacing();
        ImGui::TextDisabled("%s", tr(Str::EsF1));
        if (wahl >= 0) {
            offen = false;
            diag::detail("Erste Schritte: Wahl " + std::to_string(wahl));
            if (wahl == 0) { (void)tourStarten("k-neues-skript", 0); }
            if (wahl == 1) { (void)tourStarten("tut-karte", 0); }
            if (wahl == 2) { handbuchOeffnen("tut-projekt.html"); }
            if (wahl == 3) { (void)hilfeZeige("ort:knopf:ViewModel@modebar"); }
        }
    }
    ImGui::End();
    if (!offen) {
        s.ersteSchritte = false;
        if (g_app != nullptr) { g_app->settings.ersteSchritteGezeigt = true; }
    }
}

}  // namespace

void hilfeBild() {
    if (g_app == nullptr) { return; }
    ersteSchritteZeichnen();
    fensterZeichnen();
    kiBild();
}

void hilfeOeffnen(const std::string& frage) {
    Zustand& s = z();
    s.offen = true;
    s.fokus = true;
    if (!frage.empty()) {
        std::snprintf(s.frage, sizeof(s.frage), "%s", frage.c_str());
    }
    ImGui::SetWindowFocus("###hilfe");
}

bool hilfeOffen() { return z().offen; }

bool hilfeZeige(const std::string& id) {
    const hilfe::Index& ix = index();
    const hilfe::Eintrag* e = ix.finde(id);
    if (e == nullptr) {
        diag::detail("Helfer: unbekannte Kennung " + id);
        return false;
    }
    return zeige(*e);
}

std::vector<HilfeTreffer> hilfeFrage(const std::string& frage, std::size_t hoechstens) {
    const hilfe::Index& ix = index();
    std::vector<HilfeTreffer> aus;
    for (const hilfe::Treffer& t : ix.frage(frage, hoechstens)) {
        const hilfe::Eintrag& e = ix.eintraege()[t.eintrag];
        aus.push_back(HilfeTreffer{e.id, artName(e.art), e.titel, ersterSatz(e.text), e.pfad, e.seite});
    }
    return aus;
}

HilfeSicht hilfeSicht() {
    const Zustand& s = z();
    HilfeSicht v;
    v.offen = s.offen;
    v.ersteSchritte = s.ersteSchritte;
    v.frage = s.frage;
    for (const hilfe::Treffer& t : s.treffer) {
        if (t.eintrag < s.index.eintraege().size()) { v.treffer.push_back(s.index.eintraege()[t.eintrag].id); }
    }
    v.eintraege = s.index.eintraege().size();
    return v;
}

void hilfeFrageSetzen(const std::string& frage) {
    Zustand& s = z();
    s.offen = true;
    std::snprintf(s.frage, sizeof(s.frage), "%s", frage.c_str());
}

void hilfeErsteSchritte(bool zeigen) {
    Zustand& s = z();
    s.ersteGeprueft = true;
    s.ersteSchritte = zeigen;
}

}  // namespace bhed::gui
