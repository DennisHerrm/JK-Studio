// app_modelltab_autoskin.cpp - AP12: Auto-Skin im Model-Tab (Abgleich W7, Gruppe "Automatisch (Auto-Skin)")
//
// shank 04.11.: "einen guten algorythmus zum auto skinning". Die Rechnung steht in bhed/modell/autoskin.h. Hier: die
// Einstellungen, der Lauf als EIN Rueckgaengig-Schritt, das Ergebnis als Tabelle (Grenzen des Spiels, Abstand zu den
// Gewichten davor in Einheiten) und der Inhalt der Gruppe. Bis das Paket AP8 die Gruppe ins rechte Feld des Bereichs
// Weights setzt, zeigt sie ein eigenes Fenster, das "Auto-Skin: Einstellungen" oeffnet (Alle Aktionen suchen,
// Rechtsklick > Weights).
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/autoskin.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

// So viele Bilder der GLA (in gleichem Abstand ueber alle Animationen) misst der Vergleich mit den Gewichten davor:
// bei 21376 Bildern der _humanoid.gla jedes 107., bei einer Figur mit 5000 Ecken unter einer Zehntelsekunde.
constexpr int kVergleichsBilder = 200;

std::vector<int> vergleichsBilder(const GlaAnimation& g) {
    std::vector<int> b;
    const int n = g.bildZahl();
    const int k = std::min(n, kVergleichsBilder);
    for (int i = 0; i < k; ++i) { b.push_back(k > 1 ? static_cast<int>(static_cast<long long>(i) * (n - 1) / (k - 1)) : 0); }
    return b;
}

template <typename... A>
std::string text(Str s, A... a) {
    char z[600];
    std::snprintf(z, sizeof z, tr(s), a...);
    return z;
}

int gewaehlteFlaechen() {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    int n = 0;
    for (const int i : t.auswahl) {
        if (i >= 0 && static_cast<std::size_t>(i) < in.teile.size() && !in.teile[static_cast<std::size_t>(i)].istTag()) { ++n; }
    }
    return n;
}

void scheitern(const std::string& warum) {
    ModellAutoSkin& z = modellAutoSkinZustand();
    z.gelaufen = false;
    z.fehler = warum;
    mt().meldung = warum;
}

void zeile(Str name, const std::string& wert, Str hinweis = Str::Count) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    cjkTextDisabled("%s", tr(name));
    if (hinweis != Str::Count && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(hinweis)); }
    ImGui::TableNextColumn();
    cjkTextUnformatted(wert.c_str());
}

}  // namespace

ModellAutoSkin& modellAutoSkinZustand() {
    static ModellAutoSkin z;
    return z;
}

void modellAutoSkinOeffnen() { modellAutoSkinZustand().fensterOffen = true; }

bool modellAutoSkin() {
    ModellTab& t = mt();
    ModellAutoSkin& z = modellAutoSkinZustand();
    const GlaAnimation& a = g_app->anim;
    const modell::Inhalt vorher = t.dok.inhalt();
    if (a.bones.empty() || vorher.herkunft != modell::Herkunft::Glm || vorher.knochenZahl != static_cast<int>(a.bones.size())) {
        scheitern(tr(Str::AsFehlerSkelett));
        return false;
    }
    modell::AutoSkinOptionen o = z.o;
    o.ziel = static_cast<modell::Zielspiel>(t.zielspiel);
    o.teile.clear();
    if (z.nurAuswahl) {
        o.teile.assign(vorher.teile.size(), 0);
        for (const int i : t.auswahl) {
            if (i >= 0 && static_cast<std::size_t>(i) < o.teile.size()) { o.teile[static_cast<std::size_t>(i)] = 1; }
        }
    }
    modell::Inhalt vorlage;
    const modell::Inhalt* vp = nullptr;
    if (o.verfahren == modell::AutoSkinVerfahren::Uebertragen) {
        std::string bytes;
        if (!readFromArchives(z.vorlage, bytes) || !modell::ausGlm(bytes, vorlage) || vorlage.knochenZahl != vorher.knochenZahl) {
            scheitern(text(Str::AsFehlerVorlage, z.vorlage, vorlage.knochenZahl, vorher.knochenZahl));
            return false;
        }
        vp = &vorlage;
    }
    modell::AutoSkinBericht b;
    bool ok = false;
    const auto start = std::chrono::steady_clock::now();
    const std::uint64_t schritt = t.dok.aendern(tr(Str::AsUndo), [&](modell::Inhalt& x) { ok = modell::autoSkin(x, a, o, b, vp); });
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (!ok) {
        (void)t.dok.abbrechen(schritt);
        scheitern(b.fehler == "flaechen" ? tr(Str::AsFehlerFlaechen)
                  : b.fehler == "knochen" ? tr(Str::AsFehlerKnochen)
                  : b.fehler == "vorlage" ? text(Str::AsFehlerVorlage, z.vorlage, vorlage.knochenZahl, vorher.knochenZahl)
                                          : std::string(tr(Str::AsFehlerSkelett)));
        return false;
    }
    z.bericht = b;
    z.ms = ms;
    z.gelaufen = true;
    z.fehler.clear();
    z.ladeStand = t.dok.ladeStand();
    z.vergleich = modell::gewichteVergleichen(vorher, t.dok.inhalt(), 0, static_cast<int>(a.bones.size()), o.teile);
    z.verformung = modell::verformungsFehler(vorher, t.dok.inhalt(), a, vergleichsBilder(a), 0, o.teile);
    t.meldung = text(Str::AsMeldung, b.ecken, b.knochenBenutzt, ms);
    g_app->modelDirty = true;
    return true;
}

void drawModellAutoSkinGruppe() {
    ModellTab& t = mt();
    ModellAutoSkin& z = modellAutoSkinZustand();
    ImGui::PushTextWrapPos(0.0F);
    // Verfahren: drei Umschaltknoepfe nebeneinander, solange sie in die Zeile passen (Entwurf: zwei oben, einer darunter)
    static const Str kVerfahren[modell::kAutoSkinVerfahren] = {Str::AsVerfUebertragen, Str::AsVerfVoxel, Str::AsVerfHuelle};
    static const Str kVerfahrenHint[modell::kAutoSkinVerfahren] = {Str::AsVerfUebertragenHint, Str::AsVerfVoxelHint, Str::AsVerfHuelleHint};
    cjkTextDisabled("%s", tr(Str::AsVerfahren));
    const ImGuiStyle& stil = ImGui::GetStyle();
    const float rechts = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    for (int k = 0; k < modell::kAutoSkinVerfahren; ++k) {
        const float breite = ImGui::CalcTextSize(tr(kVerfahren[k])).x + stil.FramePadding.x * 2.0F;
        if (k > 0) {
            const float danach = ImGui::GetItemRectMax().x + 2.0F + breite;
            if (danach <= rechts) { ImGui::SameLine(0.0F, 2.0F); }
        }
        const bool an = static_cast<int>(z.o.verfahren) == k;
        if (an) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
        if (ImGui::Button(tr(kVerfahren[k]))) { z.o.verfahren = static_cast<modell::AutoSkinVerfahren>(k); }
        if (an) { ImGui::PopStyleColor(); }
        ImGui::SetItemTooltip("%s", tr(kVerfahrenHint[k]));
    }
    // Regler: Beschriftung links, Regler rechts ueber die restliche Breite
    float beschriftung = 0.0F;
    for (const Str x : {Str::AsVorlage, Str::AsAbstand, Str::AsAufloesung, Str::AsSteifigkeit, Str::AsGlaetten}) {
        beschriftung = std::max(beschriftung, ImGui::CalcTextSize(tr(x)).x);
    }
    const auto links = [&](Str x, Str hinweis) {
        ImGui::AlignTextToFramePadding();
        cjkTextDisabled("%s", tr(x));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(hinweis)); }
        ImGui::SameLine(beschriftung + stil.ItemSpacing.x * 2.0F);
        ImGui::SetNextItemWidth(-1.0F);
    };
    if (z.o.verfahren == modell::AutoSkinVerfahren::Uebertragen) {
        links(Str::AsVorlage, Str::AsVorlageHint);
        ImGui::InputText("##asvorlage", z.vorlage, sizeof z.vorlage);
        ImGui::SetItemTooltip("%s", tr(Str::AsVorlageHint));
        links(Str::AsAbstand, Str::AsAbstandHint);
        ImGui::SliderFloat("##asabstand", &z.o.uebertragenAbstand, 0.1F, 10.0F, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("%s", tr(Str::AsAbstandHint));
    }
    links(Str::AsAufloesung, Str::AsAufloesungHint);
    ImGui::BeginDisabled(z.o.verfahren == modell::AutoSkinVerfahren::Huelle);
    ImGui::SliderInt("##asaufloesung", &z.o.aufloesung, modell::kAutoSkinAufloesungMin, modell::kAutoSkinAufloesungMax, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(Str::AsAufloesungHint)); }
    links(Str::AsSteifigkeit, Str::AsSteifigkeitHint);
    ImGui::SliderFloat("##assteifigkeit", &z.o.steifigkeit, 0.0F, 1.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("%s", tr(Str::AsSteifigkeitHint));
    links(Str::AsGlaetten, Str::AsGlaettenHint);
    ImGui::SliderInt("##asglaetten", &z.o.glaetten, 0, modell::kAutoSkinGlaettenMax, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("%s", tr(Str::AsGlaettenHint));
    ImGui::Checkbox(tr(Str::AsNaehte), &z.o.naehte);
    ImGui::SetItemTooltip("%s", tr(Str::AsNaehteHint));
    ImGui::Checkbox(tr(Str::AsDreh), &z.o.drehknochen);
    ImGui::SetItemTooltip("%s", tr(Str::AsDrehHint));
    ImGui::Checkbox(tr(Str::AsDetail), &z.o.detailknochen);
    ImGui::SetItemTooltip("%s", tr(Str::AsDetailHint));
    ImGui::Checkbox(tr(Str::AsSpiegeln), &z.o.spiegeln);
    ImGui::SetItemTooltip("%s", tr(Str::AsSpiegelnHint));
    // die Zahl der gewaehlten Flaechen steht dahinter (der Text selbst traegt keinen Platzhalter: das Handbuch nennt ihn)
    ImGui::Checkbox((std::string(tr(Str::AsNurAuswahl)) + " (" + std::to_string(gewaehlteFlaechen()) + ")###asnurauswahl").c_str(), &z.nurAuswahl);
    ImGui::SetItemTooltip("%s", tr(Str::AsNurAuswahlHint));
    // der Hauptknopf in der Akzentfarbe (wie die Kopfzeile einer Gruppe)
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
    const bool los = ImGui::Button(tr(Str::AsAusfuehren), ImVec2{-1.0F, 0.0F});
    ImGui::PopStyleColor(3);
    ImGui::SetItemTooltip("%s", tr(Str::AsAusfuehrenHint));
    if (los) { (void)modellAutoSkin(); }
    ImGui::Separator();
    cjkTextDisabled("%s", tr(Str::AsErgebnis));
    const ImVec4 warn = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
    if (!z.fehler.empty()) {
        cjkTextColored(warn, "%s", z.fehler.c_str());
    } else if (!z.gelaufen || z.ladeStand != t.dok.ladeStand()) {
        cjkTextDisabled("%s", tr(Str::AsErgKeins));
        cjkTextDisabled("%s", tr(Str::AsTipp));   // nur vor dem ersten Lauf: danach steht hier das Ergebnis
    } else {
        const modell::AutoSkinBericht& b = z.bericht;
        if (ImGui::BeginTable("##asergebnis", 2, ImGuiTableFlags_SizingStretchProp)) {
            zeile(Str::AsErgEcken, std::to_string(b.ecken));
            zeile(Str::AsErgGewichte, text(Str::AsErgVon, b.gewichteMax, b.gewichteGrenze));
            zeile(Str::AsErgKnochen, text(Str::AsErgKnochenWert, b.knochenJeFlaecheMax, b.knochenGrenze, b.knochenJeFlaecheName.c_str()));
            zeile(Str::AsErgBit, tr(Str::AsErgBitWert));
            zeile(Str::AsErgAbweichung, text(Str::AsErgAbweichungWert, z.verformung.mittel, z.verformung.p95, z.verformung.max), Str::AsErgAbweichungHint);
            zeile(Str::AsErgHaupt,
                  text(Str::AsErgHauptWert, z.vergleich.ecken > 0 ? 100.0 * static_cast<double>(z.vergleich.hauptGleich) / z.vergleich.ecken : 0.0),
                  Str::AsErgAbweichungHint);
            if (b.uebertragen > 0 || b.fern > 0) { zeile(Str::AsErgUebertragen, text(Str::AsErgUebertragenWert, b.uebertragen, b.fern)); }
            if (b.voxelFest > 0) { zeile(Str::AsErgVoxel, text(Str::AsErgVoxelWert, b.voxel[0], b.voxel[1], b.voxel[2], b.voxelFest)); }
            zeile(Str::AsErgZeit, text(Str::AsErgZeitWert, z.ms));
            ImGui::EndTable();
        }
        if (b.eckenGekuerzt > 0 || b.flaechenGekuerzt > 0) {
            cjkTextDisabled("%s", text(Str::AsWarnGekuerzt, b.eckenGekuerzt, b.gewichteGrenze, b.flaechenGekuerzt, b.knochenGrenze).c_str());
        }
        if (b.knochenJeFlaecheMax > b.knochenGrenze) {
            cjkTextColored(warn, "%s", text(Str::AsWarnKnochenGrenze, b.knochenJeFlaecheName.c_str(), b.knochenJeFlaecheMax, b.knochenGrenze).c_str());
        }
        if (b.knochenAussen > 0) { cjkTextColored(warn, "%s", text(Str::AsWarnAussen, b.knochenAussen).c_str()); }
        if (b.punkteUnerreicht > 0) { cjkTextColored(warn, "%s", text(Str::AsWarnUnerreicht, b.punkteUnerreicht).c_str()); }
    }
    ImGui::PopTextWrapPos();
}

void drawModellAutoSkinFenster() {
    ModellAutoSkin& z = modellAutoSkinZustand();
    z.fensterSichtbar = false;
    if (!z.fensterOffen) { return; }
    const float fs = ImGui::GetFontSize();
    const ImGuiViewport* v = ImGui::GetMainViewport();
    // feste Breite, die Hoehe folgt dem Inhalt (nach dem Lauf kommt die Tabelle dazu) bis 94 % des Hauptfensters; am rechten
    // Rand (dort setzt AP8 die Gruppe ins Befehlsfeld), damit die Ansicht mit dem Modell frei bleibt
    const float breite = fs * 26.0F;
    ImGui::SetNextWindowPos(ImVec2{v->Pos.x + v->Size.x - fs, v->Pos.y + v->Size.y * 0.04F}, ImGuiCond_Appearing, ImVec2{1.0F, 0.0F});
    ImGui::SetNextWindowSizeConstraints(ImVec2{breite, 0.0F}, ImVec2{breite, v->Size.y * 0.94F});
    if (ImGui::Begin((std::string(tr(Str::AsGruppe)) + "###mtautoskin").c_str(), &z.fensterOffen,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        z.fensterSichtbar = true;
        drawModellAutoSkinGruppe();
    }
    ImGui::End();
}

}  // namespace bhed::gui
