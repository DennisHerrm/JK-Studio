// app_modelltab_feld.cpp - das rechte Feld des Model-Tabs als Befehlsfeld (wie 3ds Max) bzw. Eigenschaften-Editor (wie
// Blender) und die Kopfzeile des Tabs (AP3 von MBENTWURF Teil B, rc612)
//
// Massstab: der freigegebene Entwurf plaene/modellblock_stufe0/entwurf_modelltab.html und shank 10.11.: "es geht ja nicht
// nur um controls like blender oder 3ds max sondern das aussehen" - wie 3ds Max rechts das Befehlsfeld mit den Reitern
// Erstellen, Aendern, Hierarchie, Bewegung, Anzeige, Dienstprogramme; wie Blender der Eigenschaften-Editor mit senkrechter
// Reiterspalte (Werkzeug, Objekt, Modifier, Daten, Knochen, Constraints, Material). Eigene Symbole (uiicons.cpp).
//
// Der Baustein fuer Rahmen und Gruppen steht in gui/ui_rollout.h; hier steht, was den Model-Tab betrifft: welche Reiter es
// gibt, welchen ein Bereich vorgibt, die Breite des Felds mit ihrer Trennkante und die Reihe der Bereiche ueber die ganze
// Breite des Tabs mit dem Hinweis des Bedienmodus daneben.
#include "app_internal.h"
#include "textumbruch.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace bhed::gui {

namespace {

ModellFeld g_feld;

// Vorgabe der Breite in Zeichenhoehen (Entwurf: 330 von 1840 Punkten; Abgleich F13) und die Grenzen beim Ziehen.
constexpr float kBreiteVorgabe = 22.0F;
constexpr float kAnteilVorgabe = 0.42F;
constexpr float kBreiteMin = 12.0F;
constexpr float kBreiteMax = 44.0F;
// So viel bleibt der Ansicht mindestens.
constexpr float kAnsichtMin = 8.0F;

struct ReiterText {
    Str titel;
    Str hinweis;
    UiIcon icon;
};

// Reihenfolge wie MbReiter (3ds Max: Create, Modify, Hierarchy, Motion, Display, Utilities).
const ReiterText kMax[static_cast<int>(MbReiter::Zahl)] = {
    {Str::MfErstellen, Str::MfErstellenHint, UiIcon::Plus},          {Str::MfAendern, Str::MfAendernHint, UiIcon::FeldModifier},
    {Str::MfHierarchie, Str::MfHierarchieHint, UiIcon::FeldConstraints}, {Str::MfBewegung, Str::MfBewegungHint, UiIcon::Abspielen},
    {Str::MfAnzeige, Str::MfAnzeigeHint, UiIcon::Sichtbar},          {Str::MfDienste, Str::MfDiensteHint, UiIcon::FeldWerkzeug},
};

// Reihenfolge wie MbReiterBl (Blender: Tool, Object, Modifiers, Data, Bone, Constraints, Material).
const ReiterText kBlender[static_cast<int>(MbReiterBl::Zahl)] = {
    {Str::MfWerkzeug, Str::MfWerkzeugHint, UiIcon::FeldWerkzeug},   {Str::MfObjekt, Str::MfObjektHint, UiIcon::FeldObjekt},
    {Str::MfModifier, Str::MfModifierHint, UiIcon::FeldModifier},   {Str::MfDaten, Str::MfDatenHint, UiIcon::FeldDaten},
    {Str::MfKnochen, Str::MfKnochenHint, UiIcon::MbKnochen},        {Str::MfConstraints, Str::MfConstraintsHint, UiIcon::FeldConstraints},
    {Str::MfMaterial, Str::MfMaterialHint, UiIcon::FeldMaterial},
};

Str bereichName(int i) {
    static const Str kTexte[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv,
                                                Str::MbBereichWeights, Str::MbBereichPainting,    Str::MbBereichAnimation};
    return kTexte[std::clamp(i, 0, kModellBereiche - 1)];
}

// Der Hinweis neben den Bereichsreitern: je Bedienmodus und Bereich (die Tasten des Bereichs stehen in der Statuszeile).
Str bereichTipp(bool blender, ModellBereich b) {
    if (b == ModellBereich::Ansehen) { return blender ? Str::MfTippBlAnsehen : Str::MfTippMaxAnsehen; }
    if (b == ModellBereich::Animation) { return blender ? Str::MfTippBlAnim : Str::MfTippMaxAnim; }
    return blender ? Str::MfTippBlBearbeiten : Str::MfTippMaxBearbeiten;
}

// Einen Text auf ganze Abschnitte zwischen " · " kuerzen, bis er in die Breite passt (der Rest steht im Tooltip).
std::string aufAbschnitte(const std::string& ganz, float frei) {
    if (ImGui::CalcTextSize(ganz.c_str()).x <= frei) { return ganz; }
    const std::string trenner = " \xc2\xb7 ";
    std::string teil;
    std::size_t ab = 0;
    while (ab < ganz.size()) {
        const std::size_t bis = ganz.find(trenner, ab);
        const std::string stueck = ganz.substr(ab, bis == std::string::npos ? std::string::npos : bis - ab);
        const std::string probe = (teil.empty() ? stueck : teil + trenner + stueck) + trenner + "...";
        if (ImGui::CalcTextSize(probe.c_str()).x > frei) { break; }
        teil = teil.empty() ? stueck : teil + trenner + stueck;
        if (bis == std::string::npos) { break; }
        ab = bis + trenner.size();
    }
    if (teil.empty()) { return std::string(); }
    return teil + trenner + "...";
}

}  // namespace

ModellFeld& modellFeld() { return g_feld; }

MbReiter modellReiterStandard(ModellBereich b) {
    switch (b) {
        case ModellBereich::Ansehen: return MbReiter::Anzeige;
        case ModellBereich::Animation: return MbReiter::Bewegung;
        case ModellBereich::Modellieren:
        case ModellBereich::Uv:
        case ModellBereich::Weights:
        case ModellBereich::Painting: break;
    }
    return MbReiter::Aendern;
}

MbReiterBl modellReiterStandardBl(ModellBereich b) {
    switch (b) {
        case ModellBereich::Ansehen: return MbReiterBl::Objekt;
        case ModellBereich::Modellieren: return MbReiterBl::Modifier;
        case ModellBereich::Uv: return MbReiterBl::Daten;
        case ModellBereich::Weights: return MbReiterBl::Daten;
        case ModellBereich::Painting: return MbReiterBl::Werkzeug;
        case ModellBereich::Animation: return MbReiterBl::Knochen;
    }
    return MbReiterBl::Objekt;
}

unsigned modellReiter(MbReiter max, MbReiterBl blender, bool auchStandard) {
    const unsigned bit = ui::reiterBit(modellBlender() ? static_cast<int>(blender) : static_cast<int>(max));
    return bit | (auchStandard ? ui::kReiterStandard : 0U);
}

void modellFeldBeginn() {
    const ModellTab& t = g_app->mtab;
    const bool blender = modellBlender();
    const int bereich = std::clamp(static_cast<int>(t.bereich), 0, kModellBereiche - 1);
    // Bereichs- oder Moduswechsel waehlt den Reiter, den der Entwurf fuer den Bereich zeigt.
    const int standard = blender ? static_cast<int>(modellReiterStandardBl(t.bereich)) : static_cast<int>(modellReiterStandard(t.bereich));
    if (g_feld.bereich != bereich || g_feld.blender != blender || g_feld.reiter < 0) {
        g_feld.bereich = bereich;
        g_feld.blender = blender;
        g_feld.reiter = standard;
    }
    constexpr int kMeiste = std::max(static_cast<int>(MbReiter::Zahl), static_cast<int>(MbReiterBl::Zahl));
    ui::FeldReiter reiter[kMeiste];
    const int zahl = blender ? static_cast<int>(MbReiterBl::Zahl) : static_cast<int>(MbReiter::Zahl);
    for (int i = 0; i < zahl; ++i) {
        const ReiterText& r = blender ? kBlender[i] : kMax[i];
        reiter[i] = ui::FeldReiter{tr(r.titel), r.icon, tr(r.hinweis)};
    }
    ui::FeldOpt o;
    o.titel = tr(blender ? Str::MfEigenschaften : Str::MbBefehlsfeld);
    o.rechts = tr(bereichName(bereich));
    o.reiter = reiter;
    o.reiterZahl = zahl;
    o.aktiv = &g_feld.reiter;
    o.standard = standard;
    o.stil = blender ? ui::RolloutStil::Blender : ui::RolloutStil::Max;
    o.kontext = bereich * 2 + (blender ? 1 : 0);
    o.leerHinweis = tr(Str::MfReiterLeer);
    ui::feldBeginn("mtrollouts", o);
}

void modellFeldEnde() { ui::feldEnde(); }

void modellFeldReiter(MbReiter max, MbReiterBl blender) {
    const ModellTab& t = g_app->mtab;
    g_feld.bereich = std::clamp(static_cast<int>(t.bereich), 0, kModellBereiche - 1);
    g_feld.blender = modellBlender();
    g_feld.reiter = g_feld.blender ? static_cast<int>(blender) : static_cast<int>(max);
}

float modellFeldBreite(float frei) {
    const float fs = ImGui::GetFontSize();
    // Ab Werk 22 Zeichenhoehen, in einem kleinen Fenster hoechstens 42 % des Platzes (sonst bliebe der Ansicht nur ihr Mindestplatz:
    // 1328 Punkte breites Fenster - Ansicht 8, Feld 19 Zeichenhoehen); eine gezogene Breite gilt, wie sie gezogen wurde.
    const float gewuenscht = g_app->settings.modellFeldBreite > 0.0F ? fs * g_app->settings.modellFeldBreite : std::min(fs * kBreiteVorgabe, std::floor(frei * kAnteilVorgabe));
    // die Ansicht behaelt ihren Mindestplatz, das Feld seine Mindestbreite
    return std::max(std::min(gewuenscht, frei - fs * kAnsichtMin), fs * 8.0F);
}

void modellFeldTrenner(float breite, float hoehe, float frei) {
    const float fs = ImGui::GetFontSize();
    ImGui::InvisibleButton("##mtfeldtrenner", ImVec2{std::max(breite, 2.0F), std::max(hoehe, 1.0F)});
    const bool ueber = ImGui::IsItemHovered();
    const bool zieht = ImGui::IsItemActive();
    g_feld.trennerVon = ImGui::GetItemRectMin().x;
    g_feld.trennerBis = ImGui::GetItemRectMax().x;
    if (ueber || zieht) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        const float x = std::floor((a.x + b.x) * 0.5F);
        ImGui::GetWindowDrawList()->AddLine(ImVec2{x, a.y}, ImVec2{x, b.y}, ImGui::GetColorU32(zieht ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered), 2.0F);
    }
    if (ueber && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        g_app->settings.modellFeldBreite = 0.0F;   // Vorgabe
    } else if (zieht && ImGui::GetIO().MouseDelta.x != 0.0F) {
        // das Feld steht rechts: nach links ziehen macht es breiter
        const float jetzt = modellFeldBreite(frei) / fs;
        const float hoechstens = std::max(std::min(kBreiteMax, frei / fs - kAnsichtMin), kBreiteMin);
        g_app->settings.modellFeldBreite = std::clamp(jetzt - ImGui::GetIO().MouseDelta.x / fs, kBreiteMin, hoechstens);
    }
    if (ueber && !zieht && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", tr(Str::MfBreiteHint)); }
}

// Die Kopfzeile des Model-Tabs ueber Explorer, Ansichten und rechtem Feld: die Bereiche als eine Reihe gleich breiter
// Felder (der gewaehlte in der Betonungsfarbe), daneben der kurze Hinweis des Bedienmodus fuer den Bereich.
void drawModellBereiche() {
    ModellTab& t = g_app->mtab;
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    // Wie im Entwurf rechts neben der Modusleiste beginnen (ihre Breite stammt aus dem letzten Bild).
    if (g_app->modeBarW > 0.0F) { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + g_app->modeBarW + st.CellPadding.x * 2.0F); }
    const float frei = std::max(ImGui::GetContentRegionAvail().x, 1.0F);
    const char* texte[kModellBereiche];
    float breitester = 0.0F;
    for (int i = 0; i < kModellBereiche; ++i) {
        texte[i] = tr(bereichName(i));
        breitester = std::max(breitester, ImGui::CalcTextSize(texte[i]).x);
    }
    // gleich breite Felder: so breit wie der laengste Name mit Rand, wenigstens 7,5 Zeichenhoehen (Entwurf: 108 von 13 Punkten)
    const float feld = std::max(std::ceil(breitester + fs * 1.4F), std::floor(fs * 7.5F));
    const float reihe = std::min(feld * static_cast<float>(kModellBereiche), frei);
    int wahl = std::clamp(static_cast<int>(t.bereich), 0, kModellBereiche - 1);
    const ImVec2 ort = ImGui::GetCursorScreenPos();
    if (ui::segment("##mtbereiche", texte, kModellBereiche, &wahl, kModellBereiche, -1, nullptr, reihe)) {
        t.bereich = static_cast<ModellBereich>(wahl);
        g_app->modelDirty = true;   // mb1a2: Weights zeigt die Farbe des Knochens
    }
    g_feld.kopfVon = ort.x;
    g_feld.kopfBis = ort.x + reihe;
    g_feld.kopfY = ort.y;
    g_feld.kopfFeld = reihe / static_cast<float>(kModellBereiche);
    g_feld.kopfBild = ImGui::GetFrameCount();
    // der Hinweis: Navigation der Ansicht (MBNAV) und die Tasten des Bedienmodus fuer diesen Bereich - so weit Platz ist
    const float luecke = fs;
    const float rest = frei - reihe - luecke;
    const bool blender = modellBlender();
    const std::string ganz = std::string(tr(bereichTipp(blender, t.bereich))) + " \xc2\xb7 " + tr(blender ? Str::MnZeileBlender : Str::MnZeileMax);
    g_feld.tippText = ganz;
    g_feld.tippGezeigt.clear();
    if (rest > fs * 6.0F) {
        const std::string zeigen = aufAbschnitte(ganz, rest);
        if (!zeigen.empty()) {
            ImGui::SameLine(0.0F, luecke);
            ImGui::AlignTextToFramePadding();
            cjkTextDisabled("%s", zeigen.c_str());
            if (zeigen != ganz) { ImGui::SetItemTooltip("%s", ganz.c_str()); }
            g_feld.tippGezeigt = zeigen;
        }
    }
}

}  // namespace bhed::gui
