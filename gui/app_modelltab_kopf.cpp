// gui/app_modelltab_kopf.cpp - MBKOPF (rc612): die Kopfzeile jeder Ansicht des Model-Tabs zum Bedienen
//
// shank 10.11. mit Bild der Kopfzeile "[+] [Perspective] [Standard]": "ich kann das hier nicht bedienen? wieso?" - bis
// rc611 war sie nur Text (dl->AddText), sah aber aus wie die Viewport Label Menus von 3ds Max. Jetzt:
//   wie 3ds Max  [+] (General: maximieren, Zoom Grenzen, was die Ansicht zeigt, "Knochen und Pruefen", ViewCube,
//                Sichtfeld), [Ansicht] (Perspektive, Orthographisch, Oben ... Rechts mit P/U/T/B/F/L) und [Schattierung]
//                (Standard, Drahtgitter F3, Kanten F4, zweiseitig, weisse Texturen, Gewichte, Silhouette); jedes Feld mit
//                Hervorhebung unter der Maus, Tooltip, Klick links und rechts.
//   wie Blender  links das Menue View (Frame Selected/All, Perspektive/Orthographisch, Viewpoint, Navigation, Quad View),
//                rechts "Overlays" und "Viewport Shading" (Wireframe / Solid / Material Preview und das Ausklappmenue) -
//                an der Stelle der Kopfzeile von Blenders 3D-Ansicht.
// Jede Ansicht der vier Ansichten hat ihre Kopfzeile und wirkt auf sich selbst (ModellSchattierung je Ansicht). Die
// Anzeige-Schalter, die bis rc611 im rechten Feld standen (Gruppe "Anzeige"), stehen hier (shank 10.11.: "die display
// optionen koennen dann oben rein in das aufklappbare menu bei perspective und standard usw"); dieselben Menues hat das
// Rechtsklick-Menue der Ansicht, und "Alle Aktionen suchen" (X / F3) findet jeden Schalter mit seinem Ort im Tooltip.
#include "app_internal.h"
#include "textumbruch.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// --- Der Katalog der Schalter --------------------------------------------------------------------------------------
//
// Ort in 3ds Max: Schattierung = Viewport Shading Label Menu, Allgemein = General Label Menu ("Show ..."), Knochen =
// dessen Untermenue "Knochen und Pruefen". In Blender: Shading = Popover "Viewport Shading", sonst Popover "Overlays".
enum class Ort : int { Schattierung, Allgemein, Knochen };

struct Schalter {
    Str text;
    Str hinweis;
    const char* tasteMax;
    const char* tasteBl;
    Ort ort;
    bool blShading;   // Blender: im Popover "Viewport Shading" (sonst Overlays)
    std::function<bool&(int)> wert;
    std::function<bool()> geht;   // nullptr: immer
    Str grund;                    // Tooltip, wenn !geht()
    // MBRASTER: 0 in beiden Bedienmodi, 1 nur wie 3ds Max, 2 nur wie Blender; Blender: im Block "Hilfslinien" der Overlays;
    // `wirkt`: zeigt der Schalter in dieser Ansicht etwas (Blender: Grid nur in den Achsenansichten, Floor in den freien)
    int modus = 0;
    bool hilfslinie = false;
    std::function<bool(int)> wirkt;
};

// MBRASTER: die freien Ansichten (Perspektive, Orthographisch) zeigen den Boden, die Achsenansichten ihre Ebene
bool freieAnsicht(int n) {
    const AnsichtArt a = mt().ansichten[n].art;
    return a == AnsichtArt::Perspektive || a == AnsichtArt::Orthographisch;
}

// Liegt die Weltachse k in dem, was die Ansicht vom Raster zeigt (Boden: X und Y, dazu Z senkrecht; Oben/Unten: X, Y;
// Vorne/Hinten: X, Z; Links/Rechts: Y, Z)?
bool achseSichtbar(int n, int k) {
    if (freieAnsicht(n)) { return true; }
    const AnsichtArt a = mt().ansichten[n].art;
    if (a == AnsichtArt::Oben || a == AnsichtArt::Unten) { return k != 2; }
    if (a == AnsichtArt::Vorne || a == AnsichtArt::Hinten) { return k != 1; }
    return k != 0;
}

bool imModus(const Schalter& s) { return s.modus == 0 || s.modus == (modellBlender() ? 2 : 1); }

bool& schDraht(int n) { return mt().ansichten[n].sch.drahtgitter; }

const std::vector<Schalter>& schalter() {
    static const std::vector<Schalter> kListe = {
        {Str::MkKanten, Str::MkKantenHint, "F4", "", Ort::Schattierung, false, [](int n) -> bool& { return mt().ansichten[n].sch.kanten; }, nullptr,
         Str::MkKantenHint},
        {Str::MkZweiseitig, Str::MkZweiseitigHint, "", "", Ort::Schattierung, true, [](int n) -> bool& { return mt().ansichten[n].sch.zweiseitig; },
         nullptr, Str::MkZweiseitigHint},
        {Str::MkWeiss, Str::MkWeissHint, "", "", Ort::Schattierung, true, [](int n) -> bool& { return mt().ansichten[n].sch.weiss; }, nullptr,
         Str::MkWeissHint},
        {Str::MkGewichte, Str::MkGewichteHint, "", "", Ort::Schattierung, false, [](int) -> bool& { return mt().an.gewichte; }, nullptr,
         Str::MkGewichteHint},
        {Str::MkSilhouette, Str::MkSilhouetteHint, "", "", Ort::Schattierung, true, [](int) -> bool& { return mt().silhouette; }, nullptr,
         Str::MkSilhouetteHint},
        {Str::MkRaster, Str::MrRasterHint, "G", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.raster; }, nullptr,
         Str::MrRasterHint, 1},
        {Str::MrGrauBoden, Str::MkBodenHint, "", "", Ort::Allgemein, false, [](int) -> bool& { return mt().an.boden; }, nullptr, Str::MkBodenHint},
        {Str::MbAnUrsprung, Str::MkUrsprungHint, "", "", Ort::Allgemein, false, [](int) -> bool& { return mt().an.ursprung; }, nullptr,
         Str::MkUrsprungHint},
        {Str::MbAnHuelle, Str::MkHuelleHint, "", "", Ort::Allgemein, false, [](int) -> bool& { return mt().an.huelle; }, nullptr, Str::MkHuelleHint},
        {Str::MkKlammer, Str::MkKlammerHint, "J", "", Ort::Allgemein, false, [](int) -> bool& { return mt().an.klammer; }, nullptr,
         Str::MkKlammerHint},
        {Str::MkStatistik, Str::MkStatistikHint, "7", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.statistik; },
         nullptr, Str::MkStatistikHint},
        {Str::MbAnLineal, Str::MbAnLinealHint, "", "", Ort::Allgemein, false, [](int) -> bool& { return mt().an.lineal; }, nullptr,
         Str::MbAnLinealHint},
        {Str::MbAnKnochen, Str::MkKnochenHint, "", "", Ort::Knochen, false, [](int) -> bool& { return g_app->modellKnochen; }, nullptr,
         Str::MkKnochenHint},
        {Str::MkFlossen, Str::MkFlossenHint, "", "", Ort::Knochen, false, [](int) -> bool& { return g_app->settings.modellFlossen; },
         [] { return g_app->modellKnochen && !modellBlender(); }, Str::MkFlossenBraucht},
        {Str::MbAnNormalen, Str::MkNormalenHint, "", "", Ort::Knochen, false, [](int) -> bool& { return mt().an.normalen; }, nullptr,
         Str::MkNormalenHint},
        {Str::MbAnTags, Str::MkTagsHint, "", "", Ort::Knochen, false, [](int) -> bool& { return mt().an.tags; }, nullptr, Str::MkTagsHint},
        {Str::MkRisse, Str::MkRisseHint, "", "", Ort::Knochen, false, [](int) -> bool& { return mt().an.risse; }, nullptr, Str::MkRisseHint},
        {Str::MkEckNummern, Str::MbAnEckNummernHint, "", "", Ort::Knochen, false, [](int) -> bool& { return mt().an.eckNummern; }, nullptr,
         Str::MbAnEckNummernHint},
        {Str::MkDreieckNummern, Str::MbAnEckNummernHint, "", "", Ort::Knochen, false, [](int) -> bool& { return mt().an.dreieckNummern; },
         nullptr, Str::MbAnEckNummernHint},
        // MBRASTER: Blender Overlays > Guides - Grid, Floor, Axes X Y Z (je Ansicht)
        {Str::MkRaster, Str::MrRasterHintBl, "", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.raster; }, nullptr,
         Str::MrRasterHintBl, 2, true, [](int n) { return !freieAnsicht(n); }},
        {Str::MrBoden, Str::MrBodenHint, "", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.rasterBoden; }, nullptr,
         Str::MrBodenHint, 2, true, [](int n) { return freieAnsicht(n); }},
        {Str::MrAchseX, Str::MrAchsenHint, "", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.achsen[0]; }, nullptr,
         Str::MrAchsenHint, 2, true, [](int n) { return achseSichtbar(n, 0); }},
        {Str::MrAchseY, Str::MrAchsenHint, "", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.achsen[1]; }, nullptr,
         Str::MrAchsenHint, 2, true, [](int n) { return achseSichtbar(n, 1); }},
        {Str::MrAchseZ, Str::MrAchsenHint, "", "", Ort::Allgemein, false, [](int n) -> bool& { return mt().ansichten[n].sch.achsen[2]; }, nullptr,
         Str::MrAchsenHint, 2, true, [](int n) { return achseSichtbar(n, 2); }},
        // AP5 (Abgleich P3a): die fuenf Anzeige-Schalter von ModView, die fehlten (gui/app_modelltab_pruefen.cpp, Zeichner gpumap_win32.cpp)
        {Str::PqAlpha, Str::PqAlphaHint, "", "", Ort::Schattierung, true, [](int) -> bool& { return modellPr().alpha; }, nullptr, Str::PqAlphaHint},
        {Str::PqFilter, Str::PqFilterHint, "", "", Ort::Schattierung, true, [](int) -> bool& { return modellPr().filter; }, nullptr, Str::PqFilterHint},
        {Str::PqSchwelle, Str::PqSchwelleHint, "", "", Ort::Knochen, false, [](int) -> bool& { return modellPr().schwelleAn; }, nullptr, Str::PqSchwelleHint},
        {Str::PqHervor, Str::PqHervorHint, "", "", Ort::Knochen, false, [](int) -> bool& { return modellPr().hervorheben; }, nullptr, Str::PqHervorHint},
        // zeigt nur etwas, wenn eine gezeichnete Flaeche SHADER_MAX_VERTEXES / 2 Ecken hat (im Zielspiel)
        {Str::PqSchatten, Str::PqSchattenMenueHint, "", "", Ort::Knochen, false, [](int) -> bool& { return modellPr().schatten; }, nullptr,
         Str::PqSchattenMenueHint, 0, false, [](int) { return modellPrSchattenZahl() > 0; }},
    };
    return kListe;
}

const char* tasteVon(const Schalter& s) { return modellBlender() ? s.tasteBl : s.tasteMax; }

// Der Ort im Tooltip der Suche und des alten Platzes ("In der Kopfzeile der Ansicht: [+] > Knochen und Pruefen").
std::string ortText(const Schalter& s) {
    if (modellBlender()) { return tr(s.blShading ? Str::MkOrtShadingBl : Str::MkOrtOverlays); }
    if (s.ort == Ort::Schattierung) { return tr(Str::MkOrtSchatt); }
    return tr(s.ort == Ort::Knochen ? Str::MkOrtKnochen : Str::MkOrtAllg);
}

void geaendert() {
    g_app->modelDirty = true;
    ++mt().tastenZaehler;
}

void schalterZeile(const Schalter& s, int n) {
    const bool geht = !s.geht || s.geht();
    bool& w = s.wert(n);
    const char* taste = tasteVon(s);
    if (ImGui::MenuItem(tr(s.text), taste[0] != '\0' ? taste : nullptr, w, geht)) {
        w = !w;
        mt().letzteTaste = tr(s.text);
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(geht ? s.hinweis : s.grund));
}

void schalterDerArt(Ort ort, int n) {
    for (const Schalter& s : schalter()) {
        if (s.ort == ort && imModus(s)) { schalterZeile(s, n); }
        if (s.ort == ort && s.text == Str::PqSchwelle) { modellPrSchwelleZeile(); }   // AP5: die Zahl zur Schwelle
    }
}

// MBRASTER: die Bodenhoehe (Raster und grauer Boden) und "Tiefster Punkt"
void bodenZeile() {
    ModellTab& t = mt();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0F);
    if (ImGui::DragFloat(tr(Str::MbAnBodenZ), &t.an.bodenZ, 0.5F, -512.0F, 512.0F, "%.1f")) { g_app->modelDirty = true; }
    ImGui::SetItemTooltip("%s", tr(Str::MrBodenZHint));
    ImGui::SameLine();
    if (ImGui::SmallButton(tr(Str::MrTiefster))) {
        modellBodenAufTiefstenPunkt();
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MrTiefsterHint));
}

void bodenHoehe() {
    if (!mt().an.boden) { return; }
    ImGui::Indent();
    bodenZeile();
    ImGui::Unindent();
}

// MBRASTER: Abstand und Hauptlinien (3ds Max: Grid Spacing, Major Lines every Nth Grid Line, Perspective View Grid Extent;
// Blender: Scale, Subdivisions) - gelten fuer alle Ansichten und stehen in den Einstellungen
void rasterZahlen() {
    Settings& st = g_app->settings;
    const bool bl = modellBlender();
    const float b = ImGui::GetFontSize() * 6.0F;
    ImGui::SetNextItemWidth(b);
    if (ImGui::DragFloat(tr(bl ? Str::MrMassstab : Str::MrAbstand), &st.modellRasterAbstand, 0.05F, 0.125F, 1024.0F, "%.3g", ImGuiSliderFlags_AlwaysClamp)) {
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(bl ? Str::MrMassstabHint : Str::MrAbstandHint));
    ImGui::SetNextItemWidth(b);
    if (ImGui::DragInt(tr(bl ? Str::MrUnterteil : Str::MrHaupt), &st.modellRasterHaupt, 0.05F, 2, 100, "%d", ImGuiSliderFlags_AlwaysClamp)) { geaendert(); }
    ImGui::SetItemTooltip("%s", tr(bl ? Str::MrUnterteilHint : Str::MrHauptHint));
    if (bl) { return; }
    ImGui::SetNextItemWidth(b);
    if (ImGui::DragInt(tr(Str::MrAusdehnung), &st.modellRasterAusdehnung, 0.1F, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp)) { geaendert(); }
    ImGui::SetItemTooltip("%s", tr(Str::MrAusdehnungHint));
}

// MBRASTER: 3ds Max - das Untermenue unter "Raster (G)" (in Max: Grid and Snap Settings > Home Grid)
void rasterEinstellungenMax() {
    const bool offen = ImGui::BeginMenu(tr(Str::MrRasterEinst));
    ImGui::SetItemTooltip("%s", tr(Str::MrRasterEinstHint));
    if (!offen) { return; }
    ++mt().kopfMenueZaehler;
    rasterZahlen();
    bodenZeile();
    ImGui::EndMenu();
}

// MBRASTER: Blender Overlays > Guides - Grid, Floor, die Achsen X Y Z in einer Zeile, Scale, Subdivisions (dazu die Bodenhoehe)
void hilfslinienBlender(int n) {
    ImGui::SeparatorText(tr(Str::MrHilfslinien));
    for (const Schalter& s : schalter()) {
        if (s.hilfslinie && s.text != Str::MrAchseX && s.text != Str::MrAchseY && s.text != Str::MrAchseZ) { schalterZeile(s, n); }
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(tr(Str::MrAchsen));
    ImGui::SetItemTooltip("%s", tr(Str::MrAchsenHint));
    // die Farben der Achsen wie Blenders Thema (TH_AXIS_X/Y/Z)
    static const ImU32 kFarbe[3] = {IM_COL32(255, 51, 82, 255), IM_COL32(139, 220, 0, 255), IM_COL32(40, 144, 255, 255)};
    static const char* const kName[3] = {"X", "Y", "Z"};
    bool* achsen = mt().ansichten[n].sch.achsen;
    for (int k = 0; k < 3; ++k) {
        ImGui::SameLine();
        ImGui::PushID(k);
        ImGui::PushStyleColor(ImGuiCol_Text, kFarbe[k]);
        if (ImGui::Selectable(kName[k], achsen[k], ImGuiSelectableFlags_NoAutoClosePopups, ImVec2{ImGui::GetFontSize() * 1.4F, 0.0F})) {
            achsen[k] = !achsen[k];
            mt().aktiv = n;
            geaendert();
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::SetItemTooltip("%s", tr(Str::MrAchsenHint));
    }
    rasterZahlen();
    bodenZeile();
}

void hintergrundZeile() {
    bool eigen = g_app->settings.modellHintergrund >= 0;
    if (ImGui::MenuItem(tr(Str::MbAnHintergrund), nullptr, eigen)) {
        g_app->settings.modellHintergrund = eigen ? -1 : 0x333333;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MbAnHintergrundHint));
    if (g_app->settings.modellHintergrund >= 0) {
        ImGui::Indent();
        const int h = g_app->settings.modellHintergrund;
        float f[3] = {static_cast<float>((h >> 16) & 0xFF) / 255.0F, static_cast<float>((h >> 8) & 0xFF) / 255.0F, static_cast<float>(h & 0xFF) / 255.0F};
        if (ImGui::ColorEdit3("##mkhg", f, ImGuiColorEditFlags_NoInputs)) {
            const auto b = [](float x) { return static_cast<int>(std::lround(std::clamp(x, 0.0F, 1.0F) * 255.0F)); };
            g_app->settings.modellHintergrund = (b(f[0]) << 16) | (b(f[1]) << 8) | b(f[2]);
            g_app->modelDirty = true;
        }
        ImGui::Unindent();
    }
}

// Blickwinkel: ModView kennt 10, 80 und 90 Grad (die Knoepfe), der Regler 10 bis 120.
void sichtfeldZeile() {
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0F);
    if (ImGui::SliderFloat(tr(Str::MbAnFov), &g_app->modelCam.fovY, 10.0F, 120.0F, "%.0f")) { g_app->modelDirty = true; }
    ImGui::SetItemTooltip("%s", tr(Str::MkSichtfeldHint));
    for (const float f : {10.0F, 80.0F, 90.0F}) {
        ImGui::SameLine();
        char z[8];
        std::snprintf(z, sizeof z, "%.0f", static_cast<double>(f));
        if (ImGui::SmallButton(z)) {
            g_app->modelCam.fovY = f;
            geaendert();
        }
    }
}

void navigationZeilen() {
    Settings& st = g_app->settings;
    const bool bl = modellBlender();
    if (ImGui::MenuItem(tr(bl ? Str::MkGizmoZeigen : Str::MkViewCubeZeigen), bl ? "Ctrl+`" : nullptr, st.modellNavigation)) {
        st.modellNavigation = !st.modellNavigation;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(bl ? Str::MkGizmoHint : Str::MkViewCubeHint));
    if (ImGui::MenuItem(tr(Str::MkNurAktiv), nullptr, !st.modellNavigationAlle, st.modellNavigation)) {
        st.modellNavigationAlle = !st.modellNavigationAlle;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkNurAktivHint));
    if (bl) { return; }   // Blender hat keinen Kompass
    // wie die ViewCube-Konfiguration von 3ds Max: "Show the compass below the ViewCube", "Angle of north (degrees)"
    if (ImGui::MenuItem(tr(Str::MkKompass), nullptr, st.modellKompass, st.modellNavigation)) {
        st.modellKompass = !st.modellKompass;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkKompassHint));
    ImGui::BeginDisabled(!st.modellNavigation || !st.modellKompass);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0F);
    if (ImGui::DragInt(tr(Str::MkKompassNord), &st.modellKompassNord, 1.0F, 0, 359, "%d\xC2\xB0", ImGuiSliderFlags_AlwaysClamp)) { geaendert(); }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("%s", tr(Str::MkKompassNordHint));
}

// AP3 R4: der Bedienmodus des Model-Tabs als Untermenue (bis rc611 zwei Knoepfe im rechten Feld).
void bedienmodusMenue() {
    if (!ImGui::BeginMenu(tr(Str::MbBedienung))) { return; }
    const bool blender = modellBlender();
    if (ImGui::MenuItem(tr(Str::MbWieMax), nullptr, !blender)) { modellBedienungSetzen(0); }
    if (ImGui::MenuItem(tr(Str::MbWieBlender), nullptr, blender)) { modellBedienungSetzen(1); }
    ImGui::SetItemTooltip("%s", tr(Str::MzModusHint));
    ImGui::EndMenu();
}

// --- 3ds Max: die drei Menues ---------------------------------------------------------------------------------------

void menueAllgemein(int n) {
    ModellTab& t = mt();
    if (ImGui::MenuItem(tr(Str::MkMaximieren), "Alt+W", !t.vierAnsichten)) {
        t.aktiv = n;
        t.vierAnsichten = !t.vierAnsichten;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkMaximierenHint));
    if (ImGui::MenuItem(tr(Str::MkEinpassen), "Z")) {
        modellAnsichtEinpassenIn(n);
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkEinpassenHint));
    ImGui::Separator();
    for (const Schalter& s : schalter()) {
        if (s.ort != Ort::Allgemein || !imModus(s)) { continue; }
        schalterZeile(s, n);
        if (s.text == Str::MkRaster) { rasterEinstellungenMax(); }   // MBRASTER
        if (s.text == Str::MrGrauBoden) { bodenHoehe(); }
    }
    hintergrundZeile();
    ImGui::Separator();
    if (ImGui::BeginMenu(tr(Str::MkKnochenPruefen))) {
        ++t.kopfMenueZaehler;
        schalterDerArt(Ort::Knochen, n);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MkViewCube))) {
        navigationZeilen();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MbAnFov))) {
        sichtfeldZeile();
        ImGui::EndMenu();
    }
    ImGui::Separator();
    bedienmodusMenue();   // AP3 R4
}

struct AnsichtZeile {
    AnsichtArt art;
    Str text;
    const char* tasteMax;
    const char* tasteBl;
};

const AnsichtZeile kAnsichten[] = {
    {AnsichtArt::Perspektive, Str::MbAnsichtPersp, "P", "Numpad 5"}, {AnsichtArt::Orthographisch, Str::MkOrtho, "U", "Numpad 5"},
    {AnsichtArt::Oben, Str::MbAnsichtOben, "T", "Numpad 7"},         {AnsichtArt::Unten, Str::MbAnsichtUnten, "B", "Ctrl+Numpad 7"},
    {AnsichtArt::Vorne, Str::MbAnsichtVorne, "F", "Numpad 1"},       {AnsichtArt::Hinten, Str::MbAnsichtHinten, "", "Ctrl+Numpad 1"},
    {AnsichtArt::Links, Str::MbAnsichtLinks, "L", "Ctrl+Numpad 3"},  {AnsichtArt::Rechts, Str::MbAnsichtRechts, "", "Numpad 3"},
};

// Wie die Tasten P/U/T/B/F/L: 3ds Max passt beim Wechsel ein (ansichtArt), U behaelt die Richtung.
void ansichtWaehlen(int n, AnsichtArt art) {
    ModellTab& t = mt();
    t.aktiv = n;
    if (art == AnsichtArt::Orthographisch) {
        float w[3];
        modellAnsichtWinkel(t.ansichten[n], w);
        modellBlickSetzen(n, AnsichtArt::Orthographisch, w, false);
    } else if (modellBlender()) {
        modellBlickSetzen(n, art, nullptr, true);   // Blender view_axis: weich, Mitte und Groesse bleiben (MBNAV)
    } else {
        t.ansichten[n].animStart = -1.0;
        t.ansichten[n].art = art;
        modellAnsichtEinpassenIn(n);
    }
    geaendert();
}

void menueAnsichtMax(int n) {
    const AnsichtArt jetzt = mt().ansichten[n].art;
    for (const AnsichtZeile& z : kAnsichten) {
        if (ImGui::MenuItem(tr(z.text), z.tasteMax[0] != '\0' ? z.tasteMax : nullptr, jetzt == z.art)) { ansichtWaehlen(n, z.art); }
        if (z.art == AnsichtArt::Orthographisch) { ImGui::Separator(); }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MkStartansicht))) {
        mt().aktiv = n;
        modellHomeFlug(n);
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkHomeHint));
    if (ImGui::MenuItem(tr(Str::MkEinpassen))) {
        modellAnsichtEinpassenIn(n);
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkEinpassenHint));
    // MBNAV: die Befehle der Ansichtssteuerung von 3ds Max (unten rechts) und die Einstellungen der Maus
    if (ImGui::BeginMenu(tr(Str::MkBlNavigation))) {
        ++mt().kopfMenueZaehler;
        modellNavMenue(n);
        ImGui::EndMenu();
    }
}

void schattierungRadio(int n, bool draht) {
    ModellSchattierung& sch = mt().ansichten[n].sch;
    sch.drahtgitter = draht;
    mt().aktiv = n;
    geaendert();
}

void menueSchattierungMax(int n) {
    ModellSchattierung& sch = mt().ansichten[n].sch;
    if (ImGui::MenuItem(tr(Str::MbAnsichtStandard), nullptr, !sch.drahtgitter)) { schattierungRadio(n, false); }
    ImGui::SetItemTooltip("%s", tr(Str::MkStandardHint));
    if (ImGui::MenuItem(tr(Str::MkDraht), "F3", sch.drahtgitter)) { schattierungRadio(n, !sch.drahtgitter); }
    ImGui::SetItemTooltip("%s", tr(Str::MkDrahtHint));
    ImGui::Separator();
    schalterDerArt(Ort::Schattierung, n);
}

// --- Blender ---------------------------------------------------------------------------------------------------------

struct BlZeile {
    Str text;
    const char* taste;
    std::function<void()> tun;
};

void blenderZeilen(int n, const std::vector<BlZeile>& zeilen) {
    for (const BlZeile& z : zeilen) {
        if (ImGui::MenuItem(tr(z.text), z.taste)) {
            mt().aktiv = n;
            z.tun();
            geaendert();
        }
    }
}

void menueAnsichtBlender(int n) {
    ModellTab& t = mt();
    // Frame All wie die Taste Pos1 (view3d.view_all): die Richtung bleibt; die Startansicht dreht dazu nach vorne
    blenderZeilen(n, {{Str::MkBlGewaehlte, "Numpad .", [] { (void)modellNavBlEinpassen(true, false); }},
                      {Str::MkBlAlles, "Home", [] { (void)modellNavBlEinpassen(false, false); }},
                      {Str::MkStartansicht, nullptr, [n] { modellHomeFlug(n); }}});
    ImGui::SetItemTooltip("%s", tr(Str::MkHomeHint));
    ImGui::Separator();
    blenderZeilen(n, {{Str::MkBlPersp, "Numpad 5", [] { modellPerspektiveUmschalten(); }}});
    if (ImGui::BeginMenu(tr(Str::MkBlBlickpunkt))) {
        ++t.kopfMenueZaehler;
        const AnsichtArt jetzt = t.ansichten[n].art;
        for (const AnsichtZeile& z : kAnsichten) {
            if (z.art == AnsichtArt::Perspektive || z.art == AnsichtArt::Orthographisch) { continue; }
            if (ImGui::MenuItem(tr(z.text), z.tasteBl, jetzt == z.art)) { ansichtWaehlen(n, z.art); }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MkBlNavigation))) {
        ++t.kopfMenueZaehler;
        blenderZeilen(n, {{Str::MkBlKreisLinks, "Numpad 4", [] { modellKreisenSchritt(15.0F, 0.0F); }},
                          {Str::MkBlKreisRechts, "Numpad 6", [] { modellKreisenSchritt(-15.0F, 0.0F); }},
                          {Str::MkBlKreisOben, "Numpad 8", [] { modellKreisenSchritt(0.0F, 15.0F); }},
                          {Str::MkBlKreisUnten, "Numpad 2", [] { modellKreisenSchritt(0.0F, -15.0F); }},
                          {Str::MkBlGegenseite, "Numpad 9", [] { modellGegenseite(); }},
                          {Str::MkBlRollLinks, "Shift+Numpad 4", [] { modellRollenSchritt(-15.0F); }},
                          {Str::MkBlRollRechts, "Shift+Numpad 6", [] { modellRollenSchritt(15.0F); }}});
        ImGui::Separator();
        modellNavMenue(n);   // MBNAV
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MkBlVier), "Ctrl+Alt+Q", t.vierAnsichten)) {
        t.aktiv = n;
        modellNavBlVier();   // MBNAV: Anordnung wie Blender
        geaendert();
    }
    ImGui::Separator();
    bedienmodusMenue();   // AP3 R4
}

void menueOverlays(int n) {
    hilfslinienBlender(n);   // MBRASTER
    ImGui::Separator();
    for (const Schalter& s : schalter()) {
        if (s.blShading || s.hilfslinie || !imModus(s)) { continue; }
        schalterZeile(s, n);
        if (s.text == Str::MrGrauBoden) { bodenHoehe(); }
        if (s.text == Str::PqSchwelle) { modellPrSchwelleZeile(); }   // AP5: die Zahl zur Schwelle
    }
    hintergrundZeile();
    ImGui::Separator();
    navigationZeilen();
    sichtfeldZeile();
}

// Blender "Viewport Shading": Wireframe / Solid / Material Preview. Solid zeigt die Figur weiss (wie Blenders Solid mit
// der Farbe "Material" ohne Bild), Material Preview mit ihren Bildern.
int blShadingArt(int n) {
    const ModellSchattierung& sch = mt().ansichten[n].sch;
    return sch.drahtgitter ? 0 : (sch.weiss ? 1 : 2);
}

void blShadingSetzen(int n, int art) {
    ModellSchattierung& sch = mt().ansichten[n].sch;
    sch.drahtgitter = art == 0;
    if (art != 0) { sch.weiss = art == 1; }
    mt().aktiv = n;
    geaendert();
}

void menueShadingBlender(int n) {
    static const Str kArt[3] = {Str::MkBlDraht, Str::MkBlSolid, Str::MkBlMaterial};
    static const Str kHint[3] = {Str::MkDrahtHint, Str::MkBlSolidHint, Str::MkBlMaterialHint};
    for (int a = 0; a < 3; ++a) {
        if (ImGui::MenuItem(tr(kArt[a]), a == 0 ? "Shift+Z" : nullptr, blShadingArt(n) == a)) { blShadingSetzen(n, a); }
        ImGui::SetItemTooltip("%s", tr(kHint[a]));
    }
    ImGui::Separator();
    ModellSchattierung& sch = mt().ansichten[n].sch;
    bool rueck = !sch.zweiseitig;   // Blender "Backface Culling" = einseitig
    if (ImGui::MenuItem(tr(Str::MkBlRueckseiten), nullptr, rueck)) {
        sch.zweiseitig = rueck;
        geaendert();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MkZweiseitigHint));
    for (const Schalter& s : schalter()) {
        if (s.blShading && imModus(s) && s.text != Str::MkZweiseitig && s.text != Str::MkWeiss) { schalterZeile(s, n); }
    }
}

// --- Die Kopfzeile ---------------------------------------------------------------------------------------------------

constexpr ImU32 kText = IM_COL32(220, 220, 220, 230);
constexpr ImU32 kTextHover = IM_COL32(255, 222, 120, 255);   // wie die Labels von 3ds Max unter der Maus: hell, gelblich

// Wohin das Menue eines Feldes aufklappt: unter das Feld, wie die Viewport Label Menus von 3ds Max und die Menues der
// Kopfzeile von Blender. Ohne Angabe oeffnet ImGui ein Popup an der Maus - es lag dann ueber seinem eigenen Feld.
struct MenueAnker {
    ImVec2 ort{};
    float pivotX = 0.0F;   // 0: an der linken Kante des Feldes, 1: an der rechten (Felder am rechten Rand der Ansicht)
};

// Ein Feld: Text mit Rand, Hervorhebung unter der Maus und solange sein Menue offen ist; Klick links oder rechts oeffnet
// das Menue. Gibt die rechte Kante zurueck.
float feld(ImDrawList* dl, ImVec2 o, const char* id, const char* text, Str hinweis, const char* popup, int n, float hoehe, MenueAnker& anker,
           bool rechts = false) {
    const ImVec2 g = ImGui::CalcTextSize(text);
    const float rand = 4.0F;
    const ImVec2 groesse{g.x + rand * 2.0F, hoehe};
    anker.ort = ImVec2{rechts ? o.x + groesse.x : o.x, o.y + groesse.y};
    anker.pivotX = rechts ? 1.0F : 0.0F;
    ImGui::SetCursorScreenPos(o);
    ImGui::InvisibleButton(id, groesse, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool klick = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
    const bool ueber = ImGui::IsItemHovered();
    if (ueber) { ImGui::SetItemTooltip("%s", tr(hinweis)); }
    const bool offen = ImGui::IsPopupOpen(popup);
    if (ueber || offen) {
        dl->AddRectFilled(o, ImVec2{o.x + groesse.x, o.y + groesse.y}, offen ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 34), 3.0F);
    }
    dl->AddText(ImVec2{o.x + rand, o.y + (hoehe - g.y) * 0.5F}, ueber || offen ? kTextHover : kText, text);
    if (klick) {
        mt().aktiv = n;
        mt().klickDrinBild = ImGui::GetFrameCount();
        ImGui::OpenPopup(popup);
    }
    return o.x + groesse.x;
}

// Blender: die drei Kugeln Wireframe / Solid / Material Preview als eigene kleine Zeichnungen (keine Symbole von Blender).
float shadingKnopf(ImDrawList* dl, ImVec2 o, int n, int art, float hoehe) {
    static const Str kArt[3] = {Str::MkBlDraht, Str::MkBlSolid, Str::MkBlMaterial};
    char id[16];
    std::snprintf(id, sizeof id, "##mksh%d", art);
    ImGui::SetCursorScreenPos(o);
    ImGui::InvisibleButton(id, ImVec2{hoehe, hoehe});
    const bool ueber = ImGui::IsItemHovered();
    if (ueber) { ImGui::SetItemTooltip("%s", tr(kArt[art])); }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        mt().klickDrinBild = ImGui::GetFrameCount();
        blShadingSetzen(n, art);
    }
    const bool an = blShadingArt(n) == art;
    dl->AddRectFilled(o, ImVec2{o.x + hoehe, o.y + hoehe}, an ? IM_COL32(70, 110, 170, 230) : (ueber ? IM_COL32(255, 255, 255, 50) : IM_COL32(0, 0, 0, 90)),
                      3.0F);
    const ImVec2 m{o.x + hoehe * 0.5F, o.y + hoehe * 0.5F};
    const float r = hoehe * 0.32F;
    const ImU32 f = ueber ? kTextHover : kText;
    if (art == 0) {
        dl->AddCircle(m, r, f, 20, 1.2F);
        dl->AddLine(ImVec2{m.x - r, m.y}, ImVec2{m.x + r, m.y}, f);
        dl->AddEllipse(m, ImVec2{r * 0.45F, r}, f, 0.0F, 16, 1.0F);
    } else if (art == 1) {
        dl->AddCircleFilled(m, r, f, 20);
    } else {
        dl->AddCircleFilled(m, r, IM_COL32(150, 150, 150, 255), 20);
        dl->AddCircleFilled(ImVec2{m.x - r * 0.3F, m.y - r * 0.3F}, r * 0.5F, f, 16);
    }
    return o.x + hoehe;
}

// Blender nennt die freien Ansichten "User Perspective" / "User Orthographic", die Achsenansichten "Front" ...
std::string blenderName(AnsichtArt art) {
    if (art == AnsichtArt::Perspektive) { return tr(Str::MkBlUserPersp); }
    return art == AnsichtArt::Orthographisch ? tr(Str::MkBlUserOrtho) : modellAnsichtText(art);
}

}  // namespace

bool modellKopfTreffer(const ModellAnsicht& a, ImVec2 maus) {
    if (maus.y >= a.lo[1] && maus.y <= a.kopfUnten) {
        if (maus.x >= a.lo[0] && maus.x <= a.kopfRechts) { return true; }
        if (a.kopfLinks2 > 0.0F && maus.x >= a.kopfLinks2 && maus.x <= a.lo[0] + a.groesse[0]) { return true; }
    }
    return a.wuerfel[2] > a.wuerfel[0] && maus.x >= a.wuerfel[0] && maus.x <= a.wuerfel[2] && maus.y >= a.wuerfel[1] && maus.y <= a.wuerfel[3];
}

void drawModellKopfzeile(int n, ModellAnsicht& a) {
    ModellTab& t = mt();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool bl = modellBlender();
    const float fs = ImGui::GetFontSize();
    const float hoehe = fs + 6.0F;
    const ImVec2 lo{a.lo[0], a.lo[1]};
    const float breite = a.groesse[0];
    float x = lo.x + 3.0F;
    const float y = lo.y + 2.0F;
    char text[160];
    std::array<MenueAnker, 5> anker{};   // je Menue ##mkm0 .. ##mkm4
    // --- links ---------------------------------------------------------------------------------------------------
    if (!bl) {
        x = feld(dl, ImVec2{x, y}, "##mkplus", "[+]", Str::MkPlusHint, "##mkm0", n, hoehe, anker[0]) + 2.0F;
        std::snprintf(text, sizeof text, "[%s]", modellAnsichtText(a.art));
        x = feld(dl, ImVec2{x, y}, "##mkansicht", text, Str::MkAnsichtHint, "##mkm1", n, hoehe, anker[1]) + 2.0F;
        const char* art = a.sch.drahtgitter ? tr(Str::MkDraht) : tr(Str::MbAnsichtStandard);
        if (a.sch.kanten) {
            std::snprintf(text, sizeof text, "[%s + %s]", art, tr(Str::MkKanten));
        } else {
            std::snprintf(text, sizeof text, "[%s]", art);
        }
        x = feld(dl, ImVec2{x, y}, "##mkschatt", text, Str::MkSchattHint, "##mkm2", n, hoehe, anker[2]);
    } else {
        std::snprintf(text, sizeof text, "%s", tr(Str::MkBlAnsicht));
        x = feld(dl, ImVec2{x, y}, "##mkansicht", text, Str::MkAnsichtHintBl, "##mkm1", n, hoehe, anker[1]) + 6.0F;
        // AP4 (Entwurf "Benutzer-Perspektive - Objektmodus"): der Name der Ansicht und der Modus des Bereichs als Ausklapper
        x = modellBlModusKopf(dl, n, x, y, hoehe, blenderName(a.art).c_str()) + 6.0F;
    }
    a.kopfRechts = x;
    a.kopfUnten = y + hoehe;
    // --- rechts (Blender): Overlays und Viewport Shading ---------------------------------------------------------
    a.kopfLinks2 = 0.0F;
    float rechtsAb = lo.x + breite;
    if (bl) {
        const float knopf = hoehe;
        const float overlaysB = ImGui::CalcTextSize(tr(Str::MkBlOverlays)).x + 8.0F;
        const float pfeilB = ImGui::CalcTextSize("v").x + 8.0F;
        const float gesamt = overlaysB + 6.0F + knopf * 3.0F + 2.0F * 2.0F + 2.0F + pfeilB + 4.0F;
        float rx = lo.x + breite - gesamt;
        rx = std::max(rx, x + 4.0F);
        a.kopfLinks2 = rx;
        rechtsAb = rx;
        rx = feld(dl, ImVec2{rx, y}, "##mkoverlays", tr(Str::MkBlOverlays), Str::MkOverlaysHint, "##mkm3", n, hoehe, anker[3], true) + 6.0F;
        for (int k = 0; k < 3; ++k) { rx = shadingKnopf(dl, ImVec2{rx, y}, n, k, knopf) + 2.0F; }
        (void)feld(dl, ImVec2{rx, y}, "##mkshading", "v", Str::MkShadingHintBl, "##mkm4", n, hoehe, anker[4], true);
    }
    // --- der Hinweis daneben (nur maximiert, nur wenn er passt) und im Blender-Modus der Name der Ansicht ----------
    {
        std::string hin;   // AP4: wie Blender steht der Name der Ansicht schon vor dem Modus (modellBlModusKopf)
        if (!t.vierAnsichten) { hin = tr(bl ? Str::MbAnsichtMaxBlender : Str::MbAnsichtMax); }
        if (!hin.empty() && x + 6.0F + ImGui::CalcTextSize(hin.c_str()).x <= rechtsAb - 6.0F) {
            dl->AddText(ImVec2{x + 6.0F, y + (hoehe - fs) * 0.5F}, IM_COL32(200, 200, 200, 170), hin.c_str());
        }
    }
    // --- die Menues ------------------------------------------------------------------------------------------------
    const auto menue = [&](const char* id, const MenueAnker& m, auto&& inhalt) {
        if (ImGui::IsPopupOpen(id)) { ImGui::SetNextWindowPos(m.ort, ImGuiCond_Appearing, ImVec2{m.pivotX, 0.0F}); }
        if (ImGui::BeginPopup(id)) {
            ++t.kopfMenueZaehler;
            inhalt();
            // Esc schliesst das Menue (wie die Suche: ohne das blieb ein per Rechtsklick geoeffnetes offen)
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    };
    menue("##mkm0", anker[0], [&] { menueAllgemein(n); });
    menue("##mkm1", anker[1], [&] {
        if (bl) {
            menueAnsichtBlender(n);
        } else {
            menueAnsichtMax(n);
        }
    });
    menue("##mkm2", anker[2], [&] { menueSchattierungMax(n); });
    menue("##mkm3", anker[3], [&] { menueOverlays(n); });
    menue("##mkm4", anker[4], [&] { menueShadingBlender(n); });
}

// Das Rechtsklick-Menue der Ansicht (Untermenue "Anzeige"): dieselben Menues wie in der Kopfzeile der aktiven Ansicht.
void modellAnzeigeKontext() {
    ModellTab& t = mt();
    const int n = std::clamp(t.aktiv, 0, 3);
    if (modellBlender()) {
        if (ImGui::BeginMenu(tr(Str::MkBlOverlays))) {
            menueOverlays(n);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::MkBlShading))) {
            menueShadingBlender(n);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(tr(Str::MkBlNavigation))) {   // MBNAV
            modellNavMenue(n);
            ImGui::EndMenu();
        }
        return;
    }
    if (ImGui::BeginMenu(tr(Str::MkSchattierung))) {
        menueSchattierungMax(n);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MbRAnzeige))) {
        menueAllgemein(n);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::MkBlNavigation))) {   // MBNAV: die Befehle der Ansichtssteuerung auch im Rechtsklick-Menue
        modellNavMenue(n);
        ImGui::EndMenu();
    }
}

// "Alle Aktionen suchen": jeder Anzeige-Schalter, Haken als [x], Tooltip mit dem Ort in der Kopfzeile.
bool modellKopfAktionenSuche(const std::string& such) {
    ModellTab& t = mt();
    const int n = std::clamp(t.aktiv, 0, 3);
    bool ausgeloest = false;
    // `haken`: 1 an, 0 aus, -1 eine Aktion ohne Zustand
    const auto zeile = [&](const std::string& text, int haken, const std::string& tip, const std::function<void()>& tun, int id) {
        if (!such.empty() && klein(text).find(such) == std::string::npos) { return; }
        ImGui::PushID(id);
        const std::string z = std::string(haken < 0 ? "" : (haken != 0 ? "[x] " : "[  ] ")) + text;
        if (ImGui::Selectable(z.c_str()) && !ausgeloest) {
            tun();
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", tip.c_str());
        ImGui::PopID();
    };
    for (const Schalter& s : schalter()) {
        if (!imModus(s)) { continue; }   // MBRASTER: nur die Schalter des Bedienmodus (Blender: Grid, Floor, Axes)
        std::string text = tr(s.text);
        const char* k = tasteVon(s);
        if (k[0] != '\0') { text += std::string(" (") + k + ")"; }
        const bool geht = !s.geht || s.geht();
        zeile(text, s.wert(n) ? 1 : 0, std::string(tr(geht ? s.hinweis : s.grund)) + "\n" + ortText(s), [&s, n, geht] {
            if (!geht) { return; }
            s.wert(n) = !s.wert(n);
            mt().letzteTaste = tr(s.text);
            geaendert();
        }, static_cast<int>(&s - schalter().data()));
    }
    {
        const std::string ort = tr(modellBlender() ? Str::MkOrtShadingBl : Str::MkOrtSchatt);
        std::string text = tr(Str::MkDraht);
        text += modellBlender() ? " (Shift+Z)" : " (F3)";
        zeile(text, t.ansichten[n].sch.drahtgitter ? 1 : 0, std::string(tr(Str::MkDrahtHint)) + "\n" + ort, [n] {
            schDraht(n) = !schDraht(n);
            geaendert();
        }, 1000);
        zeile(tr(Str::MbAnHintergrund), g_app->settings.modellHintergrund >= 0 ? 1 : 0,
              std::string(tr(Str::MbAnHintergrundHint)) + "\n" + tr(modellBlender() ? Str::MkOrtOverlays : Str::MkOrtAllg), [] {
                  g_app->settings.modellHintergrund = g_app->settings.modellHintergrund >= 0 ? -1 : 0x333333;
                  geaendert();
              }, 1001);
        zeile(tr(modellBlender() ? Str::MkGizmoZeigen : Str::MkViewCubeZeigen), g_app->settings.modellNavigation ? 1 : 0,
              std::string(tr(modellBlender() ? Str::MkGizmoHint : Str::MkViewCubeHint)) + "\n" + tr(modellBlender() ? Str::MkOrtOverlays : Str::MkOrtAllg),
              [] {
                  g_app->settings.modellNavigation = !g_app->settings.modellNavigation;
                  geaendert();
              }, 1002);
        zeile(tr(Str::MkNurAktiv), g_app->settings.modellNavigationAlle ? 0 : 1,
              std::string(tr(Str::MkNurAktivHint)) + "\n" + tr(modellBlender() ? Str::MkOrtOverlays : Str::MkOrtAllg), [] {
                  g_app->settings.modellNavigationAlle = !g_app->settings.modellNavigationAlle;
                  geaendert();
              }, 1004);
        if (!modellBlender()) {
            zeile(tr(Str::MkKompass), g_app->settings.modellKompass ? 1 : 0, std::string(tr(Str::MkKompassHint)) + "\n" + tr(Str::MkOrtAllg), [] {
                g_app->settings.modellKompass = !g_app->settings.modellKompass;
                geaendert();
            }, 1005);
            char nord[160];
            std::snprintf(nord, sizeof nord, "%s (%d\xC2\xB0)", tr(Str::MkKompassNordDrehen), g_app->settings.modellKompassNord);
            zeile(nord, -1, std::string(tr(Str::MkKompassNordHint)) + "\n" + tr(Str::MkOrtAllg), [] {
                g_app->settings.modellKompassNord = (g_app->settings.modellKompassNord + 90) % 360;
                geaendert();
            }, 1006);
        }
        zeile(tr(Str::MkStartansicht), -1, tr(Str::MkHomeHint), [n] {
            modellHomeFlug(n);
            geaendert();
        }, 1003);
    }
    return ausgeloest;
}

// Die Tasten, die die Suche schon als Schalter zeigt (F3/F4/G/J/7, im Blender-Modus Umschalt+Z und Strg+`): die
// Tastenliste der Suche laesst sie aus.
bool modellKopfSucheHatTaste(ImGuiKeyChord chord) {
    if (modellBlender()) { return chord == (ImGuiMod_Shift | ImGuiKey_Z) || chord == (ImGuiMod_Ctrl | ImGuiKey_GraveAccent); }
    return chord == ImGuiKey_F3 || chord == ImGuiKey_F4 || chord == ImGuiKey_G || chord == ImGuiKey_J || chord == ImGuiKey_7;
}

// Selbsttest: der Katalog der Anzeige-Schalter (4a: jeder Schalter mit Wirkung im Bild)
int modellKopfSchalterZahl() { return static_cast<int>(schalter().size()); }
const char* modellKopfSchalterText(int i) { return tr(schalter()[static_cast<std::size_t>(i)].text); }
const char* modellKopfSchalterHinweis(int i) { return tr(schalter()[static_cast<std::size_t>(i)].hinweis); }
bool& modellKopfSchalterWert(int i, int ansicht) { return schalter()[static_cast<std::size_t>(i)].wert(std::clamp(ansicht, 0, 3)); }
bool modellKopfSchalterGeht(int i) {
    const Schalter& s = schalter()[static_cast<std::size_t>(i)];
    return !s.geht || s.geht();
}
// MBRASTER: gehoert der Schalter zum Bedienmodus und zeigt er in dieser Ansicht etwas?
bool modellKopfSchalterHier(int i, int ansicht) {
    const Schalter& s = schalter()[static_cast<std::size_t>(i)];
    return imModus(s) && (!s.wirkt || s.wirkt(std::clamp(ansicht, 0, 3)));
}
bool modellKopfSchalterBrauchtAuswahl(int i) {   // zeigen sich an der gewaehlten Flaeche (ohne: der Hinweis)
    const Str t = schalter()[static_cast<std::size_t>(i)].text;
    return t == Str::MkKlammer || t == Str::PqHervor;   // AP5: Flaeche hervorheben zeigt sich an der gewaehlten Flaeche
}

// Selbsttest: die Mitte eines Felds der Kopfzeile der Ansicht (aus dem letzten Bild).
bool modellKopfFeldOrt(int ansicht, int feldNr, ImVec2& out) {
    if (ansicht < 0 || ansicht > 3) { return false; }
    const ModellAnsicht& a = mt().ansichten[ansicht];
    if (!a.sichtbar || a.kopfUnten <= a.lo[1]) { return false; }
    const float fs = ImGui::GetFontSize();
    const float y = (a.lo[1] + a.kopfUnten) * 0.5F;
    const bool bl = modellBlender();
    const auto breite = [](const std::string& s) { return ImGui::CalcTextSize(s.c_str()).x + 8.0F; };
    if (!bl) {
        const float b0 = breite("[+]");
        const float b1 = breite(std::string("[") + modellAnsichtText(a.art) + "]");
        float x = a.lo[0] + 3.0F;
        if (feldNr == 0) { out = ImVec2{x + b0 * 0.5F, y}; return true; }
        x += b0 + 2.0F;
        if (feldNr == 1) { out = ImVec2{x + b1 * 0.5F, y}; return true; }
        x += b1 + 2.0F;
        if (feldNr == 2) { out = ImVec2{x + fs, y}; return true; }
        return false;
    }
    if (feldNr == 1) { out = ImVec2{a.lo[0] + 3.0F + breite(tr(Str::MkBlAnsicht)) * 0.5F, y}; return true; }
    if (feldNr == 3) { out = ImVec2{a.kopfLinks2 + breite(tr(Str::MkBlOverlays)) * 0.5F, y}; return true; }
    if (feldNr == 4) { out = ImVec2{a.lo[0] + a.groesse[0] - 4.0F - breite("v") * 0.5F, y}; return true; }
    if (feldNr >= 10 && feldNr <= 12) {   // die drei Shading-Kugeln
        const float knopf = fs + 6.0F;
        const float x0 = a.kopfLinks2 + breite(tr(Str::MkBlOverlays)) + 6.0F;
        out = ImVec2{x0 + static_cast<float>(feldNr - 10) * (knopf + 2.0F) + knopf * 0.5F, y};
        return true;
    }
    return false;
}

}  // namespace bhed::gui
