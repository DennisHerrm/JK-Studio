// app_karte_terrain.cpp - Karten-Editor, Stufe 8b Punkt 1: die Oberflaeche des
// Terrain Generators (die Logik: src/karte/terraingen.cpp).
//
// Portiert aus NetRadiant-Custom master (origin/master 8216133, nicht im
// Release 20260114), contrib/terrain_generator/terrain_generator.cpp:
// dispatch "About" (Kasten) und "Generate Terrain": der nicht modale Dialog
// (QFormLayout, setMinimumWidth 420) mit Target, den Groessen (Auswahl live
// bzw. SpinBox), Sub-square Size + Advanced (Step X/Y), Irregular Grid, Base
// Shape, Axis, der Formhoehe mit Beschriftung je Form, Tunnel Height, Terrace
// Step, Grid Step, Noise Type, Variance, Frequency, Texture + Pick, Reset to
// Default, Generate / Close; die Sichtbarkeit je Form (update_shape), Ziel
// (update_target_mode) und Advanced (update_advanced); die Abfrage der Auswahl
// (250 ms) und des Texturbrowsers (100 ms) bei jedem Bild; die Werte bleiben
// beim Schliessen (static).
//
// Abweichungen (eigene, "nicht in NetRadiant-Custom"): die Zeile "Seed:" (fester
// Startwert der Zufallsfolge, vorgegeben aus = NRC); der Dialog wird beim
// Ausblenden von Zeilen wieder kleiner (Qt behaelt die groessere Hoehe); der
// Texturbrowser ueberschreibt das Feld nicht mit einer leeren Wahl.
//
//   Terrain Generator for NetRadiant-Custom
//   Copyright (C) 2026 vallz and vld ("Developed by vallz and vld").
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; version 2 of the License. This program is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
//   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_qtdialog.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace bhed::gui::karteneditor {

namespace {

namespace tg = karte::terraingen;
using i18n::Str;

Editor& e() { return editor(); }

constexpr float kRand = 11.0F;
constexpr float kAbstand = 6.0F;

const tg::Werte kVorgabe{};

// Die Beschriftung der Formhoehe je Form (shape_height_label)
Str formHoeheText(int form) {
    static const Str t[9] = {Str::KeTgPeak, Str::KeTgPeak, Str::KeTgCraterDepth, Str::KeTgRidgeH, Str::KeTgSlopeH,
                             Str::KeTgVolcanoH, Str::KeTgValleyD, Str::KeTgTunnelH, Str::KeTgSlopeH};
    return t[std::clamp(form, 0, 8)];
}

// update_target_mode beim Wechsel auf "Use Selection": die Formhoehe aus der
// gespeicherten Groesse bzw. der Auswahl (nur Slope / Slope Tunnel)
void zielGewechselt() {
    TerrainZustand& z = e().terrain;
    double h = 0;
    if (tg::schraegeAusAuswahl(e().karte, z.w, h)) { z.w.formHoehe = h; }
}

void kastenZeigen(const tg::Meldekasten& k) {
    struct Paar {
        const char* titel;
        Str t, x;
    };
    static const Paar paare[] = {{"Terrain Generator - No Selection", Str::KeTgNoSelTitel, Str::KeTgNoSel},
                                 {"Terrain Generator - Brush Too Small", Str::KeTgSmallTitel, Str::KeTgSmall},
                                 {"Terrain Generator - No Texture", Str::KeTgNoTexTitel, Str::KeTgNoTex}};
    for (const Paar& p : paare) {
        if (k.titel == p.titel) {
            qtMeldung(trK(p.t), trK(p.x), MeldungArt::Fehler, false, nullptr);
            return;
        }
    }
    qtMeldung(k.titel, k.text, MeldungArt::Fehler, false, nullptr);
}

}  // namespace

void terrainErzeugenKnopf() {   // do_generate
    Editor& ed = e();
    TerrainZustand& z = ed.terrain;
    if (z.w.startwertFest) { std::srand(static_cast<unsigned>(z.w.startwert)); }   // eigene Zeile "Seed:"
    const tg::Ergebnis erg = tg::erzeugen(ed.karte, z.w, [] { return std::rand(); });
    z.letztes = erg;
    if (!erg.ok) {
        kastenZeigen(erg.fehler);
        return;
    }
    for (std::string t : erg.ausgabe) {
        while (!t.empty() && t.back() == '\n') { t.pop_back(); }
        ausgabe(t);
    }
    for (std::string t : erg.fehlerStrom) {
        while (!t.empty() && t.back() == '\n') { t.pop_back(); }
        fehler(t);
    }
    ++z.erzeugt;
    ++ed.sichtStand;   // SceneChangeNotify
    auswahlGeaendert();
}

void terrainSchliessen() {
    TerrainZustand& z = e().terrain;
    if (!z.offen) { return; }
    z.gespeichert = z.w;   // s_* = ... nach dem Ende der Ereignisschleife
    z.offen = false;
}

std::vector<PluginUmsetzung> terrainUmsetzungen() {
    std::vector<PluginUmsetzung> v;
    v.push_back({"terrainGenerator::About",
                 [] { qtMeldung(trK(Str::KeTgAboutTitel), tg::kAbout, MeldungArt::Info, false, nullptr); }, nullptr});
    v.push_back({"terrainGenerator::GenerateTerrain", [] {
                     Editor& ed = e();
                     TerrainZustand& z = ed.terrain;
                     if (z.offen) {   // s_active_dialog: nur nach vorn holen
                         ImGui::SetWindowFocus("###terraingenerator");
                         return;
                     }
                     z.w = z.gespeichert;
                     // initial_texture: Wahl des Browsers, sonst die gemerkte, sonst "textures/"
                     const std::string browser = ed.texBro.shader;
                     z.w.textur = !browser.empty() ? browser : (!z.gespeichert.textur.empty() ? z.gespeichert.textur : std::string(tg::kStandardTextur));
                     z.letzteTexturAbfrage = z.w.textur;
                     z.offen = true;
                     z.oeffnen = true;
                 },
                 nullptr});
    return v;
}

void terrainDialogZeichnen() {
    Editor& ed = e();
    TerrainZustand& z = ed.terrain;
    if (!z.offen) { return; }
    tg::Werte& w = z.w;

    // Abfrage des Texturbrowsers (pick_timer, 100 ms)
    {
        const std::string aktuell = ed.texBro.shader;
        if (aktuell != z.letzteTexturAbfrage && !aktuell.empty()) {
            z.letzteTexturAbfrage = aktuell;
            w.textur = aktuell;
        }
    }
    const tg::ShapeType st = static_cast<tg::ShapeType>(w.form);
    const bool useSel = w.ziel == 0;
    const bool isFlat = st == tg::ShapeType::Flat;
    const bool isSlopeTunnel = st == tg::ShapeType::SlopeTunnel;
    const bool isTunnel = st == tg::ShapeType::Tunnel || isSlopeTunnel;
    const bool hasAxis = st == tg::ShapeType::Slope || st == tg::ShapeType::Ridge || st == tg::ShapeType::Valley || isTunnel;

    // Die Zeilen des QFormLayout (ausgeblendete fallen weg)
    struct Zeile {
        std::string label;
        float h;
    };
    const std::string lTarget = trK(Str::KeTgTarget);
    const std::string lW = trK(Str::KeTgWidth);
    const std::string lL = trK(Str::KeTgLength);
    const std::string lH = trK(Str::KeTgHeight);
    const std::string lSq = trK(Str::KeTgSubSquare);
    const std::string lSx = trK(Str::KeTgStepX);
    const std::string lSy = trK(Str::KeTgStepY);
    const std::string lShape = trK(Str::KeTgBaseShape);
    const std::string lAxis = trK(Str::KeTgAxis);
    const std::string lFormH = trK(formHoeheText(w.form));
    const std::string lTunnelH = trK(Str::KeTgTunnelH);
    const std::string lTerr = trK(Str::KeTgTerrace);
    const std::string lGrid = trK(Str::KeTgGridStep);
    const std::string lNoise = trK(Str::KeTgNoiseType);
    const std::string lVar = trK(Str::KeTgVariance);
    const std::string lFreq = trK(Str::KeTgFrequency);
    const std::string lSeed = trK(Str::KeTgSeed);
    const std::string lTex = trK(Str::KeTgTexture);
    std::vector<std::string> labels{lTarget, lW, lL, lH, lSq, lShape, lGrid, lNoise, lVar, lFreq, lSeed, lTex};
    if (w.erweitert) {
        labels.push_back(lSx);
        labels.push_back(lSy);
    }
    if (hasAxis) { labels.push_back(lAxis); }
    if (!isFlat) { labels.push_back(lFormH); }
    if (isSlopeTunnel) { labels.push_back(lTunnelH); }
    float labelB = 0.0F;
    for (const std::string& l : labels) { labelB = std::max(labelB, qtd::textBreite(l)); }
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = 420.0F;   // dialog.setMinimumWidth( 420 )
    const float feldB = innenB - kRand - x1;
    int zeilen22 = 1 + 3 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1;   // Target, Groessen, Sub-square, Shape, Grid, Noise, Variance, Frequency, Seed, Texture, Reset
    int zeilen20 = 0;
    int zeilen16 = 0;
    if (useSel) {   // die drei Zeilen sind QLabel
        zeilen22 -= 3;
        zeilen16 += 3;
    }
    if (w.erweitert) { zeilen22 += 2; }
    if (!isTunnel) { ++zeilen20; }
    if (hasAxis) { ++zeilen22; }
    if (!isFlat) { ++zeilen22; }
    if (isSlopeTunnel) { ++zeilen22; }
    if (!isFlat && st != tg::ShapeType::Tunnel) { ++zeilen22; }
    const int n = zeilen22 + zeilen20 + zeilen16;
    const float innenH = kRand + static_cast<float>(zeilen22) * 22.0F + static_cast<float>(zeilen20) * 20.0F + static_cast<float>(zeilen16) * 16.0F +
                         static_cast<float>(n) * kAbstand + 12.0F + 22.0F + kRand;
    if (!qtd::beginnen(std::string(trK(Str::KeTgTitel)) + "###terraingenerator", z.oeffnen, innenB, innenH, false)) {
        if (qtd::geschlossen()) { terrainSchliessen(); }
        return;
    }
    qtd::merke("TgDialog", 0, 0, innenB, innenH);
    float y = kRand;
    const auto label = [&](const std::string& t, float h) { qtd::text(t, kRand, y, h); };
    // Target
    label(lTarget, 22);
    {
        const int vorher = w.ziel;
        (void)qtd::wahlFeld("tgziel", w.ziel, {trK(Str::KeTgUseSel), trK(Str::KeTgManual)}, x1, feldB, y);
        if (w.ziel != vorher && w.ziel == 0) { zielGewechselt(); }
    }
    y += 22 + kAbstand;
    // Groessen
    if (useSel) {
        const tg::AuswahlGrenzen s = tg::auswahlGrenzen(ed.karte);
        const std::string strich = "\xE2\x80\x94";   // "—"
        const std::string wt = s.valid ? std::to_string(static_cast<int>(s.x1 - s.x0)) : strich;
        const std::string lt = s.valid ? std::to_string(static_cast<int>(s.y1 - s.y0)) : strich;
        const std::string ht = s.valid ? std::to_string(static_cast<int>(std::max(s.z1 - s.z0, 64.0))) : strich;
        for (const auto& [l, v, id] : {std::tuple{lW, wt, "tgselw"}, std::tuple{lL, lt, "tgsell"}, std::tuple{lH, ht, "tgselh"}}) {
            label(l, 16);
            qtd::text(v, x1, y, 16);
            qtd::merke(id, x1, y, feldB, 16);
            y += 16 + kAbstand;
        }
    } else {
        (void)qtd::zahlFeld(lW, "tgmanw", w.manuellB, 64, 131072, 0, 64, kRand, x1, feldB, y);
        y += 22 + kAbstand;
        (void)qtd::zahlFeld(lL, "tgmanl", w.manuellL, 64, 131072, 0, 64, kRand, x1, feldB, y);
        y += 22 + kAbstand;
        (void)qtd::zahlFeld(lH, "tgmanh", w.manuellH, 64, 131072, 0, 64, kRand, x1, feldB, y);
        y += 22 + kAbstand;
    }
    // Sub-square Size: Kombi (ausgeblendet mit Advanced) + Advanced
    label(lSq, 22);
    {
        std::vector<std::string> q;
        for (const int v : tg::kQuadrate) { q.push_back(std::to_string(v)); }
        const std::string adv = trK(Str::KeTgAdvanced);
        const float hakenB = qtd::hakenHinweis(adv);
        const float kombiB = std::max(qtd::kombiHinweis(q), feldB - hakenB - kAbstand);
        if (!w.erweitert) { (void)qtd::wahlFeld("tgquadrat", w.quadratIndex, q, x1, kombiB, y); }
        (void)qtd::haken(adv, "tgerweitert", w.erweitert, w.erweitert ? x1 : x1 + kombiB + kAbstand, y, 22);
    }
    y += 22 + kAbstand;
    if (w.erweitert) {
        (void)qtd::ganzFeld(lSx, "tgschrittx", w.schrittX, 8, 512, 8, kRand, x1, feldB, y);
        y += 22 + kAbstand;
        (void)qtd::ganzFeld(lSy, "tgschritty", w.schrittY, 8, 512, 8, kRand, x1, feldB, y);
        y += 22 + kAbstand;
    }
    if (!isTunnel) {
        (void)qtd::haken(trK(Str::KeTgIrregular), "tgjitter", w.unregelmaessig, x1, y, 20);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", trK(Str::KeTgIrregularTip)); }
        y += 20 + kAbstand;
    }
    label(lShape, 22);
    (void)qtd::wahlFeld("tgform", w.form,
                        {trK(Str::KeTgShFlat), trK(Str::KeTgShHill), trK(Str::KeTgShCrater), trK(Str::KeTgShRidge), trK(Str::KeTgShSlope),
                         trK(Str::KeTgShVolcano), trK(Str::KeTgShValley), trK(Str::KeTgShTunnel), trK(Str::KeTgShSlopeTunnel)},
                        x1, feldB, y);
    y += 22 + kAbstand;
    if (hasAxis) {
        label(lAxis, 22);
        (void)qtd::wahlFeld("tgachse", w.achse, {trK(Str::KeTgXAxis), trK(Str::KeTgYAxis)}, x1, feldB, y);
        y += 22 + kAbstand;
    }
    if (!isFlat) {
        (void)qtd::zahlFeld(lFormH, "tgformhoehe", w.formHoehe, 0, 4096, 2, 8, kRand, x1, feldB, y);
        y += 22 + kAbstand;
    }
    if (isSlopeTunnel) {
        (void)qtd::zahlFeld(lTunnelH, "tgtunnelhoehe", w.tunnelHoehe, 0, 4096, 2, 8, kRand, x1, feldB, y);
        y += 22 + kAbstand;
    }
    if (!isFlat && st != tg::ShapeType::Tunnel) {
        (void)qtd::zahlFeld(lTerr, "tgterrasse", w.terrasse, 0, 512, 2, 8, kRand, x1, feldB, y);
        y += 22 + kAbstand;
    }
    label(lGrid, 22);
    {
        std::vector<std::string> r;
        for (const int v : tg::kRasterSchritte) { r.push_back(std::to_string(v)); }
        (void)qtd::wahlFeld("tgraster", w.rasterIndex, r, x1, feldB, y);
    }
    y += 22 + kAbstand;
    label(lNoise, 22);
    (void)qtd::wahlFeld("tgrauschen", w.rauschen, {trK(Str::KeTgPerlin), trK(Str::KeTgSimplex), trK(Str::KeTgRandom)}, x1, feldB, y);
    y += 22 + kAbstand;
    (void)qtd::zahlFeld(lVar, "tgvarianz", w.varianz, 0, 1024, 2, 1, kRand, x1, feldB, y);
    y += 22 + kAbstand;
    (void)qtd::zahlFeld(lFreq, "tgfrequenz", w.frequenz, 0.0001, 1.0, 4, 0.001, kRand, x1, feldB, y);
    y += 22 + kAbstand;
    // eigene Zeile: Seed (nicht in NetRadiant-Custom)
    label(lSeed, 22);
    {
        const std::string fest = trK(Str::KeTgSeedFix);
        const float hb = qtd::hakenHinweis(fest);
        (void)qtd::haken(fest, "tgseedfest", w.startwertFest, x1, y, 22);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", trK(Str::KeTgSeedTip)); }
        (void)qtd::ganzFeld("", "tgseed", w.startwert, 0, 999999, 1, -1.0F, x1 + hb + kAbstand, feldB - hb - kAbstand, y, w.startwertFest,
                            trK(Str::KeTgSeedTip));
    }
    y += 22 + kAbstand;
    // Texture: QLineEdit + Pick (48)
    label(lTex, 22);
    (void)qtd::zeileFeld("tgtextur", w.textur, x1, feldB - 48.0F - kAbstand, y, 1023);
    if (qtd::knopf(trK(Str::KeTgPick), "tgpick", x1 + feldB - 48.0F, 48.0F, y, trK(Str::KeTgPickTip))) {
        // TextureBrowser_show: nur wenn der Browser eine Seite des Gruppendialogs ist
        // (GroupDialog_showPage: zeigen, nicht umschalten wie ToggleTextures)
        if (ed.anordnung == karte::befehle::Anordnung::Floating || ed.anordnung == karte::befehle::Anordnung::Split || ed.gruppenDialogEingebaut) {
            if (ed.gruppenSeite != 2) {
                ed.gruppenSeite = 2;
                ed.gruppenSeiteWechseln = true;
            }
            ed.gruppenDialogOffen = true;
        }
    }
    y += 22 + kAbstand;
    // Reset to Default (Feldspalte, Breite des Knopfs)
    {
        const std::string t = trK(Str::KeTgReset);
        if (qtd::knopf(t, "tgreset", x1, qtd::knopfHinweis(t), y)) {
            w.manuellB = kVorgabe.manuellB;
            w.manuellL = kVorgabe.manuellL;
            w.manuellH = kVorgabe.manuellH;
            w.quadratIndex = kVorgabe.quadratIndex;
            w.erweitert = false;
            w.schrittX = 64;
            w.schrittY = 64;
            w.unregelmaessig = false;
            w.form = 0;
            w.achse = 0;
            w.formHoehe = 256.0;
            w.tunnelHoehe = 256.0;
            w.terrasse = 0.0;
            w.rasterIndex = 0;
            w.rauschen = 0;
            w.varianz = 32.0;
            w.frequenz = 0.005;
            w.textur = tg::kStandardTextur;
            w.startwertFest = false;   // eigene Zeile
            w.startwert = 1;
        }
    }
    y += 22 + kAbstand;
    // Generate / Close mittig (addStretch, 8 Abstand), 12 Abstand nach oben
    y += 12.0F;
    const std::string tGen = trK(Str::KeTgGenerate);
    const std::string tClose = trK(Str::KeTgClose);
    const float bGen = qtd::knopfHinweis(tGen);
    const float bClose = qtd::knopfHinweis(tClose);
    const float xs = (innenB - (bGen + 8.0F + bClose)) * 0.5F;
    const bool erzeugen = qtd::knopf(tGen, "tggenerate", xs, bGen, y);
    const bool esc = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    const bool schliessen = qtd::knopf(tClose, "tgclose", xs + bGen + 8.0F, bClose, y) || esc;
    qtd::beenden(false);
    if (erzeugen) { terrainErzeugenKnopf(); }
    if (schliessen) { terrainSchliessen(); }
}

}  // namespace bhed::gui::karteneditor
