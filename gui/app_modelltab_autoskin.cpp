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

// AP8: der Inhalt in der Form der Gruppen des rechten Felds (Baustein gui/ui_rollout.h, Entwurf W7) - die Verfahren als Wahl,
// Regler, Schalter in zwei Spalten, der Hauptknopf, das Ergebnis als Tabelle. Er steht in der Gruppe des Bereichs Weights
// (gui/app_modelltab_gewichte.cpp) und im Fenster "Auto-Skin: Einstellungen".
void drawModellAutoSkinGruppe() {
    ModellTab& t = mt();
    ModellAutoSkin& z = modellAutoSkinZustand();
    static const Str kVerfahren[modell::kAutoSkinVerfahren] = {Str::AsVerfUebertragen, Str::AsVerfVoxel, Str::AsVerfHuelle};
    static const Str kVerfahrenHint[modell::kAutoSkinVerfahren] = {Str::AsVerfUebertragenHint, Str::AsVerfVoxelHint, Str::AsVerfHuelleHint};
    const char* texte[modell::kAutoSkinVerfahren]{};
    const char* hinweise[modell::kAutoSkinVerfahren]{};
    for (int k = 0; k < modell::kAutoSkinVerfahren; ++k) {
        texte[k] = tr(kVerfahren[k]);
        hinweise[k] = tr(kVerfahrenHint[k]);
    }
    // drei Verfahren nebeneinander, solange sie in die Zeile passen (Entwurf: zwei oben, einer darunter)
    int verfahren = static_cast<int>(z.o.verfahren);
    if (ui::segment("##asverfahren", texte, modell::kAutoSkinVerfahren, &verfahren, 0, -1, hinweise)) { z.o.verfahren = static_cast<modell::AutoSkinVerfahren>(verfahren); }
    if (z.o.verfahren == modell::AutoSkinVerfahren::Uebertragen) {
        ui::zeile(tr(Str::AsVorlage), tr(Str::AsVorlageHint));
        ImGui::InputText("##asvorlage", z.vorlage, sizeof z.vorlage);
        ImGui::SetItemTooltip("%s", tr(Str::AsVorlageHint));
        (void)ui::regler(tr(Str::AsAbstand), &z.o.uebertragenAbstand, 0.1F, 10.0F, "%.1f", tr(Str::AsAbstandHint), ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::BeginDisabled(z.o.verfahren == modell::AutoSkinVerfahren::Huelle);
    (void)ui::reglerInt(tr(Str::AsAufloesung), &z.o.aufloesung, modell::kAutoSkinAufloesungMin, modell::kAutoSkinAufloesungMax, "%d", tr(Str::AsAufloesungHint),
                        ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();
    (void)ui::regler(tr(Str::AsSteifigkeit), &z.o.steifigkeit, 0.0F, 1.0F, "%.2f", tr(Str::AsSteifigkeitHint), ImGuiSliderFlags_AlwaysClamp);
    (void)ui::reglerInt(tr(Str::AsGlaetten), &z.o.glaetten, 0, modell::kAutoSkinGlaettenMax, "%d", tr(Str::AsGlaettenHint), ImGuiSliderFlags_AlwaysClamp);
    ui::rasterBeginn(2);
    (void)ui::schalter(tr(Str::AsNaehte), &z.o.naehte, tr(Str::AsNaehteHint));
    (void)ui::schalter(tr(Str::AsDreh), &z.o.drehknochen, tr(Str::AsDrehHint));
    (void)ui::schalter(tr(Str::AsDetail), &z.o.detailknochen, tr(Str::AsDetailHint));
    // FIX612B P2-1 (Entscheidung A16): der Hinweis nennt, dass mit "Nur Auswahl" nur in die gewaehlten Flaechen gespiegelt wird
    const std::string spiegelnHinweis = std::string(tr(Str::AsSpiegelnHint)) + " " + tr(Str::FxAsSpiegelnAuswahl);
    (void)ui::schalter(tr(Str::AsSpiegeln), &z.o.spiegeln, spiegelnHinweis.c_str());
    // die Zahl der gewaehlten Flaechen steht dahinter (der Text selbst traegt keinen Platzhalter: das Handbuch nennt ihn)
    (void)ui::schalter((std::string(tr(Str::AsNurAuswahl)) + " (" + std::to_string(gewaehlteFlaechen()) + ")###asnurauswahl").c_str(), &z.nurAuswahl,
                       tr(Str::AsNurAuswahlHint));
    ui::rasterEnde();
    if (ui::werkzeugKnopf(tr(Str::AsAusfuehren), {.icon = UiIcon::Erzeugen, .hinweis = tr(Str::AsAusfuehrenHint), .haupt = true})) { (void)modellAutoSkin(); }
    ui::absatz(tr(Str::AsErgebnis));
    if (!z.fehler.empty()) {
        ui::absatz(z.fehler.c_str(), ui::Ton::Warnung);
    } else if (!z.gelaufen || z.ladeStand != t.dok.ladeStand()) {
        ui::absatz(tr(Str::AsErgKeins));
        ui::tipp(tr(Str::AsTipp));   // nur vor dem ersten Lauf: danach steht hier das Ergebnis
    } else {
        const modell::AutoSkinBericht& b = z.bericht;
        const std::string abweichungHint = tr(Str::AsErgAbweichungHint);
        std::vector<ui::WertZeile> werte;
        werte.push_back({tr(Str::AsErgEcken), std::to_string(b.ecken), ui::Ton::Normal, {}});
        werte.push_back({tr(Str::AsErgGewichte), text(Str::AsErgVon, b.gewichteMax, b.gewichteGrenze), b.gewichteMax <= b.gewichteGrenze ? ui::Ton::Gut : ui::Ton::Fehler, {}});
        werte.push_back({tr(Str::AsErgKnochen), text(Str::AsErgKnochenWert, b.knochenJeFlaecheMax, b.knochenGrenze, b.knochenJeFlaecheName.c_str()),
                         b.knochenJeFlaecheMax <= b.knochenGrenze ? ui::Ton::Gut : ui::Ton::Fehler, {}});
        werte.push_back({tr(Str::AsErgBit), tr(Str::AsErgBitWert), ui::Ton::Gut, {}});
        werte.push_back({tr(Str::AsErgAbweichung), text(Str::AsErgAbweichungWert, z.verformung.mittel, z.verformung.p95, z.verformung.max), ui::Ton::Normal, abweichungHint});
        werte.push_back({tr(Str::AsErgHaupt),
                         text(Str::AsErgHauptWert, z.vergleich.ecken > 0 ? 100.0 * static_cast<double>(z.vergleich.hauptGleich) / z.vergleich.ecken : 0.0), ui::Ton::Normal,
                         abweichungHint});
        if (b.uebertragen > 0 || b.fern > 0) { werte.push_back({tr(Str::AsErgUebertragen), text(Str::AsErgUebertragenWert, b.uebertragen, b.fern), ui::Ton::Normal, {}}); }
        if (b.voxelFest > 0) { werte.push_back({tr(Str::AsErgVoxel), text(Str::AsErgVoxelWert, b.voxel[0], b.voxel[1], b.voxel[2], b.voxelFest), ui::Ton::Normal, {}}); }
        werte.push_back({tr(Str::AsErgZeit), text(Str::AsErgZeitWert, z.ms), ui::Ton::Normal, {}});
        ui::wertTabelle(werte.data(), static_cast<int>(werte.size()));
        if (b.eckenGekuerzt > 0 || b.flaechenGekuerzt > 0) {
            ui::absatz(text(Str::AsWarnGekuerzt, b.eckenGekuerzt, b.gewichteGrenze, b.flaechenGekuerzt, b.knochenGrenze).c_str());
        }
        if (b.knochenJeFlaecheMax > b.knochenGrenze) {
            ui::absatz(text(Str::AsWarnKnochenGrenze, b.knochenJeFlaecheName.c_str(), b.knochenJeFlaecheMax, b.knochenGrenze).c_str(), ui::Ton::Warnung);
        }
        if (b.knochenAussen > 0) { ui::absatz(text(Str::AsWarnAussen, b.knochenAussen).c_str(), ui::Ton::Warnung); }
        if (b.punkteUnerreicht > 0) { ui::absatz(text(Str::AsWarnUnerreicht, b.punkteUnerreicht).c_str(), ui::Ton::Warnung); }
    }
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
    // UIFORM: jedes Fenster beginnt ueber den Baustein; dieses passt sich dem Inhalt an und behaelt seine eigene Grenze
    if (ui::fensterBegin((std::string(tr(Str::AsGruppe)) + "###mtautoskin").c_str(), &z.fensterOffen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        z.fensterSichtbar = true;
        drawModellAutoSkinGruppe();
    }
    ImGui::End();
}

}  // namespace bhed::gui
