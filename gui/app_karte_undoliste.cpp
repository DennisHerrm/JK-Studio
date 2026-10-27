// app_karte_undoliste.cpp - Karten-Editor, Stufe 8b Punkt 7: der Undo-Verlauf
// als Liste (eigenes Fenster, nicht in NetRadiant-Custom). View > "Undo
// history...": oben der Stand vor dem ersten Schritt, darunter alle
// Rueckgaengig-Schritte mit den Befehlsnamen wie NRC sie fuehrt (aeltester
// zuerst, der aktuelle Stand hervorgehoben), dann die Wiederholen-Schritte
// (grau). Ein Klick springt zu diesem Stand - mit so vielen Undo bzw. Redo
// hintereinander, wie noetig (jedes wie Strg+Z/Strg+Y, mit der Meldung in der
// Konsole). Die Liste reicht so weit zurueck wie der Undo-Stapel (Preferences:
// Undo Queue Size, Vorgabe 512).
#include "karteneditor.h"

#include "app_internal.h"

#include <algorithm>
#include <cstdio>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

}  // namespace

void undoListeZeigen() { e().undoListeOffen = true; }

std::string undoListeBeschriftung(std::size_t i) {
    const karte::Rueckgaengig& u = e().karte.undo;
    std::string text;
    if (i == 0) {
        text = trK(Str::KeUlAnfang);
    } else if (i <= u.undoAnzahl()) {
        text = u.undoNamen()[i - 1];
    } else if (i <= u.undoAnzahl() + u.redoAnzahl()) {
        text = u.redoNamen()[i - 1 - u.undoAnzahl()];
    }
    char z[32];
    std::snprintf(z, sizeof(z), "%4zu  ", i);
    return z + text + "###uv" + std::to_string(i);
}

void undoListeSpringen(std::size_t ziel) {
    Editor& ed = e();
    const std::size_t gesamt = ed.karte.undo.undoAnzahl() + ed.karte.undo.redoAnzahl();
    if (ziel > gesamt) { return; }
    while (ed.karte.undo.undoAnzahl() > ziel) {
        const std::size_t vorher = ed.karte.undo.undoAnzahl();
        (void)ausfuehren("Undo");
        if (ed.karte.undo.undoAnzahl() >= vorher) { break; }
    }
    while (ed.karte.undo.undoAnzahl() < ziel) {
        const std::size_t vorher = ed.karte.undo.undoAnzahl();
        (void)ausfuehren("Redo");
        if (ed.karte.undo.undoAnzahl() <= vorher) { break; }
    }
}

void undoListeZeichnen() {
    Editor& ed = e();
    if (!ed.undoListeOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{30.0F * fs, 26.0F * fs}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeUlTitel)) + "###undoverlauf";
    if (!ImGui::Begin(titel.c_str(), &ed.undoListeOffen)) {
        ImGui::End();
        return;
    }
    const karte::Rueckgaengig& u = ed.karte.undo;
    const std::size_t nU = u.undoAnzahl();
    const std::size_t nR = u.redoAnzahl();
    char kopf[200];
    std::snprintf(kopf, sizeof(kopf), trK(Str::KeUlKopf), nU, nR, u.stufen());
    ImGui::TextUnformatted(kopf);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", trK(Str::KeLgNichtNrc));
    ImGui::Separator();
    std::size_t sprung = static_cast<std::size_t>(-1);
    if (ImGui::BeginChild("##uvliste", ImVec2{0, 0}, false)) {
        // Die Namen einmal holen (nicht je Zeile)
        const std::vector<std::string> undoN = u.undoNamen();
        const std::vector<std::string> redoN = u.redoNamen();
        const std::size_t n = 1 + nU + nR;
        ImGuiListClipper clip;
        clip.Begin(static_cast<int>(n));
        while (clip.Step()) {
            for (int zi = clip.DisplayStart; zi < clip.DisplayEnd; ++zi) {
                const auto i = static_cast<std::size_t>(zi);
                std::string text = i == 0 ? std::string(trK(Str::KeUlAnfang)) : (i <= nU ? undoN[i - 1] : redoN[i - 1 - nU]);
                char z[32];
                std::snprintf(z, sizeof(z), "%4zu  ", i);
                const std::string label = z + text + "###uv" + std::to_string(i);
                const bool grau = i > nU;
                if (grau) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
                if (ImGui::Selectable(label.c_str(), i == nU)) { sprung = i; }
                if (grau) { ImGui::PopStyleColor(); }
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeUlZeileTip)); }
            }
        }
        if (ed.undoListeRollen) {   // nach dem Oeffnen und nach jedem neuen Befehl: der aktuelle Stand sichtbar
            ImGui::SetScrollY(std::max(0.0F, (static_cast<float>(nU) * ImGui::GetTextLineHeightWithSpacing()) - (ImGui::GetWindowHeight() * 0.5F)));
            ed.undoListeRollen = false;
        }
    }
    ImGui::EndChild();
    ImGui::End();
    if (ed.undoListeStand != u.abschluesse()) {
        ed.undoListeStand = u.abschluesse();
        ed.undoListeRollen = true;
    }
    if (sprung != static_cast<std::size_t>(-1)) { undoListeSpringen(sprung); }
}

}  // namespace bhed::gui::karteneditor
