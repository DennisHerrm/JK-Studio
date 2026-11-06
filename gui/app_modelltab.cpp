// app_modelltab.cpp - der neue Model-Tab (Modell-Block 1a-1): Dokument, Anzeige,
// Bereiche, Szenen-Explorer, Befehlsfeld, Tasten wie 3ds Max
//
// Die Ansichten selbst (Zeichnen, Ueberlagerung, Maus, Gizmo, Foto) stehen in
// gui/app_modelltab_ansicht.cpp. Vorbild fuer den Bereich Ansehen/Pruefen ist
// ModView (Raven, JACoders/OpenJK-tools ModView/, GPL v2) - uebernommen sind
// Gliederung und Funktionen, gezeichnet wird wie im Spiel ueber den
// Ghoul2-Weg des Mission-Tabs; die Regeln dafuer stehen in bhed/modell/g2.h.
#include "app_internal.h"
#include "gpumap.h"

#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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

bool passt(const std::string& name, const std::string& such) {
    return such.empty() || klein(name).find(such) != std::string::npos;
}

}  // namespace

// Knochen, die der Spielcode sucht (OpenJK code/game und code/cgame:
// G2API_GetBoneIndex und G2API_AddBolt mit festem Knochennamen).
bool modellSpielKnochen(const std::string& n) {
    static const char* const kNamen[] = {"model_root", "pelvis",   "lower_lumbar", "upper_lumbar", "thoracic", "cervical",
                                         "cranium",    "Motion",   "face",         "rhand",        "lhand",    "rtalus",
                                         "ltalus",     "rhumerus", "rhang_tag_bone", "reye",       "leye",     "f_eye",
                                         "body",       "neck_bone"};
    for (const char* k : kNamen) {
        if (n == k) { return true; }
    }
    return false;
}

void modellKlickPruefen() {
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
         ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
        g_app->mtab.klickDrinBild = ImGui::GetFrameCount();
    }
}

void modellFokusAktualisieren() {
    ModellTab& t = g_app->mtab;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        t.fokus = t.klickDrinBild == ImGui::GetFrameCount();
    }
}

namespace {

// Die Stufe des Modell-Blocks, die einen spaeteren Bereich bringt (stufenplan.md).
const char* bereichStufe(ModellBereich b) {
    switch (b) {
        case ModellBereich::Modellieren: return "1d";
        case ModellBereich::Uv: return "1b";
        case ModellBereich::Weights: return "1a-2";
        case ModellBereich::Painting: return "1c";
        case ModellBereich::Animation: return "2-7";
        case ModellBereich::Ansehen: break;
    }
    return "";
}

Str bereichText(int i) {
    static const Str kTexte[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv,
                                                Str::MbBereichWeights, Str::MbBereichPainting,    Str::MbBereichAnimation};
    return kTexte[std::clamp(i, 0, kModellBereiche - 1)];
}

// Ein Umschaltknopf in der Art der Bereichsreiter (gewaehlt = Knopffarbe "aktiv").
bool reiterKnopf(const char* text, bool gewaehlt) {
    if (gewaehlt) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
    const bool k = ImGui::Button(text);
    if (gewaehlt) { ImGui::PopStyleColor(); }
    return k;
}

// Die Stufe "automatisch wie im Spiel" fuer die Perspektive (G2_ComputeLOD).
int automatischeStufe(const modell::Inhalt& in) {
    const modell::Grenzen& g = modell::grenzen(static_cast<modell::Zielspiel>(mt().zielspiel));
    float vorn[3];
    g_app->modelCam.forward(vorn);
    modell::LodLage l;
    l.stufenZahl = in.stufenZahl;
    l.radius = g.figurRadius;
    l.lodscale = g.lodscale;
    // Abstand des Ursprungs der Figur (0,0,0) entlang der Blickachse (ProjectRadius).
    l.abstand = -(vorn[0] * g_app->modelCam.pos[0] + vorn[1] * g_app->modelCam.pos[1] + vorn[2] * g_app->modelCam.pos[2]);
    l.projektion5 = 1.0F / std::tan(g_app->modelCam.fovY * 3.14159265F / 360.0F);
    return modell::lodWieImSpiel(l);
}

// Das Zielspiel nach dem aktiven Regelwerk (Vorgabe der Pruefung).
int zielspielVorgabe() {
    if (regelwerkJediOutcast()) { return static_cast<int>(modell::Zielspiel::Jk2Sp); }
    return static_cast<int>(regelwerkMovieDuels() ? modell::Zielspiel::MdSp : modell::Zielspiel::JkaSp);
}

}  // namespace

// --- Dokument und Anzeige ------------------------------------------------------------------

void modellDokumentLaden(const std::string& bytes) {
    ModellTab& t = mt();
    modell::Inhalt in;
    std::string fehler;
    if (!modell::ausGlm(bytes, in, &fehler)) {
        diag::detail("Modelldokument: " + fehler);
        modell::ausGlmModell(g_app->model, in);
    }
    t.dok.laden(std::move(in));
    t.dokModell = g_app->modelPath;
    t.g2 = modell::G2Zustand{};
    t.hautAus.clear();
    t.anzeigeSchluessel.clear();
    t.auswahl.clear();
    t.knochenWahl = -1;
    t.tagWahl = -1;
    t.anbauten.clear();
    t.sekundaer = -1;
    t.zielspiel = zielspielVorgabe();
    diag::detail("Modelldokument: \"" + t.dok.inhalt().name + "\", " + std::to_string(t.dok.inhalt().teile.size()) + " Teile, " +
                 std::to_string(t.dok.inhalt().stufenZahl) + " Stufen");
}

void modellAnzeigeAktualisieren() {
    ModellTab& t = mt();
    if (g_app->model.empty()) {
        if (!t.anzeige.empty()) {
            t.anzeige = GlmModel{};
            t.anzeigeSchluessel.clear();
        }
        return;
    }
    // Ein Modell, das nicht ueber modellDokumentLaden kam (oder eines mit
    // anderer Flaechenzahl): das Dokument aus dem gelesenen Modell.
    if (t.dokModell != g_app->modelPath || t.dok.inhalt().teile.size() != g_app->model.surfaces.size()) {
        modell::Inhalt in;
        modell::ausGlmModell(g_app->model, in);
        t.dok.laden(std::move(in));
        t.dokModell = g_app->modelPath;
        t.g2 = modell::G2Zustand{};
        t.hautAus.clear();
        t.auswahl.clear();
    }
    const modell::Inhalt& in = t.dok.inhalt();
    // Die Haut: G2API_SetSurfaceOnOffFromSkin, sobald sie wechselt.
    std::vector<char> haut(g_app->model.surfaces.size(), 0);
    for (std::size_t i = 0; i < haut.size(); ++i) { haut[i] = g_app->model.surfaces[i].skinnedOff ? 1 : 0; }
    if (haut != t.hautAus) {
        std::vector<std::string> aus;
        for (std::size_t i = 0; i < haut.size(); ++i) {
            if (haut[i] != 0) { aus.push_back(g_app->model.surfaces[i].name); }
        }
        modell::hautAnwenden(in, t.g2, aus);
        t.hautAus = haut;
    }
    const int lod = std::clamp(t.lodWahl > 0 ? t.lodWahl - 1 : automatischeStufe(in), 0, std::max(in.stufenZahl - 1, 0));
    // "Kappen" (_off-Flaechen) der Leiste: G2_SetSurfaceOnOff(..., 0) fuer jede
    // Flaeche mit OFF in der Datei, die der Anwender nicht selbst geschaltet hat.
    modell::G2Zustand z = t.g2;
    if (g_app->showCaps) {
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if ((in.teile[i].flaggen & modell::kG2Aus) == 0U) { continue; }
            const bool eigen = std::any_of(t.g2.slist.begin(), t.g2.slist.end(),
                                           [&](const modell::FlaechenSchalter& s) { return s.flaeche == static_cast<int>(i); });
            if (!eigen) { (void)modell::setzeFlaecheAnAus(in, z, in.teile[i].name, 0U); }
        }
    }
    std::string schluessel = g_app->modelPath + "|" + std::to_string(t.dok.stand()) + "|" + std::to_string(lod) + "|" +
                             std::to_string(z.wurzel) + "|";
    for (const modell::FlaechenSchalter& s : z.slist) { schluessel += std::to_string(s.flaeche) + ":" + std::to_string(s.offFlags) + ","; }
    for (const GlmSurface& s : g_app->model.surfaces) { schluessel += s.texture + ";"; }
    if (schluessel == t.anzeigeSchluessel) { return; }
    GlmModel neu = modell::zuGlm(in, lod);
    t.gezeichnet = modell::gezeichnet(in, z);
    for (std::size_t i = 0; i < neu.surfaces.size() && i < g_app->model.surfaces.size(); ++i) {
        neu.surfaces[i].texture = g_app->model.surfaces[i].texture;
        // Gezeichnet wird nach den Ghoul2-Regeln: alles andere gilt dem
        // Zeichner als von der Haut abgeschaltet (er zeichnet mit kappen).
        neu.surfaces[i].skinnedOff = t.gezeichnet[i] == 0;
    }
    t.anzeige = std::move(neu);
    t.lodJetzt = lod;
    t.anzeigeSchluessel = schluessel;
    ++t.neuGebaut;
    g_app->modelDirty = true;
}

void modellFlaecheSchalten(int flaeche, std::uint32_t offFlags) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size()) { return; }
    (void)modell::setzeFlaecheAnAus(in, t.g2, in.teile[static_cast<std::size_t>(flaeche)].name, offFlags);
}

void modellWurzelSetzen(int flaeche) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size()) { return; }
    (void)modell::setzeWurzel(in, t.g2, in.teile[static_cast<std::size_t>(flaeche)].name);
}

bool modellRueckgaengig() {
    if (!mt().dok.rueckgaengig()) { return false; }
    g_app->modelDirty = true;
    return true;
}

bool modellWiederholen() {
    if (!mt().dok.wiederholen()) { return false; }
    g_app->modelDirty = true;
    return true;
}

// --- Anbauen (G2API_AttachG2Model, G2API_AddBolt) --------------------------------------------

bool modellAnbauen(const std::string& pfad, const std::string& ziel, bool knochen, bool ersetzen) {
    std::string bytes;
    if (!readFromArchives(pfad, bytes)) {
        mt().meldung = pfad + ": " + tr(Str::MapScriptMissing);
        return false;
    }
    ModellAnbau a;
    std::string fehler;
    if (!readGlm(bytes, a.modell, &fehler)) {
        mt().meldung = pfad + ": " + fehler;
        return false;
    }
    for (const char* which : {"model_default.skin", "model.skin"}) {
        std::string skin;
        if (readFromArchives(directoryOf(pfad) + "/" + which, skin)) {
            applySkin(skin, a.modell);
            break;
        }
    }
    a.texturen = texturesFor(a.modell);
    a.pfad = pfad;
    a.ziel = ziel;
    a.knochen = knochen;
    std::vector<ModellAnbau>& liste = mt().anbauten;
    if (ersetzen) {
        liste.erase(std::remove_if(liste.begin(), liste.end(), [&](const ModellAnbau& x) { return x.ziel == ziel; }), liste.end());
    }
    liste.push_back(std::move(a));
    g_app->modelDirty = true;
    return true;
}

void modellAnbautenLoesen() {
    mt().anbauten.clear();
    g_app->modelDirty = true;
}

// --- Texturen, GLA -----------------------------------------------------------------------------

void modellTexturenNeuLaden() {
    // ModView "Refresh textures" (Alt+R): die Bilder der Modelle neu von der
    // Platte bzw. aus den Archiven - nach einem Bearbeiten im Malprogramm.
    g_app->modelTextures = ModelTextures{};
    const auto modellBild = [](const std::string& k) { return k.rfind("models/", 0) == 0; };
    for (auto it = g_app->texCache.begin(); it != g_app->texCache.end();) {
        it = modellBild(it->first) ? g_app->texCache.erase(it) : std::next(it);
    }
    for (auto it = g_app->figurStufenBilder.begin(); it != g_app->figurStufenBilder.end();) {
        it = modellBild(it->first) ? g_app->figurStufenBilder.erase(it) : std::next(it);
    }
    loadModelTextures();
    for (ModellAnbau& a : mt().anbauten) { a.texturen = texturesFor(a.modell); }
    g_app->modelDirty = true;
}

bool modellGlaLaden(const std::string& pfad) {
    std::string bytes;
    if (!readFromArchives(pfad, bytes)) { return false; }
    GlaAnimation a;
    std::string fehler;
    if (!readGla(bytes, a, &fehler)) {
        mt().meldung = pfad + ": " + fehler;
        return false;
    }
    const std::string ohne = pfad.size() > 4 && klein(pfad.substr(pfad.size() - 4)) == ".gla" ? pfad.substr(0, pfad.size() - 4) : pfad;
    g_app->animList = animSectionsFor(ohne);
    g_app->anim = std::move(a);
    g_app->animIndex = -1;
    g_app->animFrame = 0;
    mt().sekundaer = -1;
    g_app->modelDirty = true;
    diag::detail("GLA ueberschrieben: " + pfad + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen, " +
                 std::to_string(g_app->anim.numFrames) + " Bilder, " + std::to_string(g_app->animList.size()) + " Abschnitte");
    return true;
}

// --- Pruefen -------------------------------------------------------------------------------------

std::vector<modell::Befund> modellBefunde() {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    std::vector<char> hatBild(in.teile.size(), 0);
    for (std::size_t i = 0; i < hatBild.size(); ++i) {
        const bool bild = i < g_app->modelTextures.bySurface.size() && !g_app->modelTextures.bySurface[i].rgba.empty();
        const bool stufen = i < g_app->modelTextures.stufen.size() && !g_app->modelTextures.stufen[i].empty();
        hatBild[i] = (bild || stufen) ? 1 : 0;
    }
    // Ohne gefundene Bilder (Texturen aus) keine Aussage ueber fehlende.
    if (!g_app->showModelTextures || g_app->modelTextures.found == 0) { hatBild.clear(); }
    // Eine Ghoul2-Instanz: die Figur samt Anbauten (ein CGhoul2Info_v).
    return modell::pruefeModell(in, static_cast<modell::Zielspiel>(t.zielspiel), g_app->animList, g_app->anim.bildZahl(), 1, hatBild,
                                t.gezeichnet);
}

// --- Bereiche ------------------------------------------------------------------------------------

void drawModellBereiche() {
    ModellTab& t = mt();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, ImGui::GetStyle().ItemSpacing.y});
    for (int i = 0; i < kModellBereiche; ++i) {
        if (i > 0) { ImGui::SameLine(); }
        ImGui::PushID(i);
        if (reiterKnopf(tr(bereichText(i)), static_cast<int>(t.bereich) == i)) { t.bereich = static_cast<ModellBereich>(i); }
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
}

// --- Szenen-Explorer -------------------------------------------------------------------------------
namespace {

int g_explorerArt = 0;      // 0 nach Hierarchie, 1 nach Ebenen
bool g_sucheFokus = false;  // H: in das Suchfeld
std::string g_anbauZiel;    // offenes Anbau-Fenster: Tag oder Knochen
bool g_anbauKnochen = false;
bool g_anbauOeffnen = false;

void waehleFlaeche(int i) {
    ModellTab& t = mt();
    if (ImGui::GetIO().KeyCtrl) {
        const auto it = std::find(t.auswahl.begin(), t.auswahl.end(), i);
        if (it != t.auswahl.end()) {
            t.auswahl.erase(it);
        } else {
            t.auswahl.push_back(i);
        }
    } else {
        t.auswahl = {i};
    }
    t.tagWahl = -1;
    g_app->modelDirty = true;
}

void flaechenMenue(int i) {
    ModellTab& t = mt();
    if (!ImGui::BeginPopupContextItem("##flmenue")) { return; }
    if (ImGui::MenuItem(tr(Str::MbFlAn))) { modellFlaecheSchalten(i, 0U); }
    if (ImGui::MenuItem(tr(Str::MbFlAus))) { modellFlaecheSchalten(i, modell::kG2Aus); }
    if (ImGui::MenuItem(tr(Str::MbFlAusOhneKinder))) { modellFlaecheSchalten(i, modell::kG2Aus | modell::kG2OhneKinder); }
    if (ImGui::MenuItem(tr(Str::MbFlWurzel), nullptr, t.g2.wurzel == i)) { modellWurzelSetzen(i); }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MbFlUrsprung))) {
        t.g2 = modell::G2Zustand{};
        t.hautAus.clear();   // die Haut gilt wieder
    }
    if (ImGui::MenuItem(tr(Str::MbFlKappenAn))) { g_app->showCaps = true; }
    if (ImGui::MenuItem(tr(Str::MbFlKappenAus))) { g_app->showCaps = false; }
    if (ImGui::MenuItem(tr(Str::MbFlAlleAn))) {
        const modell::Inhalt& in = t.dok.inhalt();
        for (std::size_t k = 0; k < in.teile.size(); ++k) {
            if (!in.teile[k].istTag()) { modellFlaecheSchalten(static_cast<int>(k), 0U); }
        }
    }
    ImGui::EndPopup();
}

void flaechenZeile(int i, bool kinder, const std::string& such, int tiefe) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const modell::Teil& teil = in.teile[static_cast<std::size_t>(i)];
    if (teil.istTag() || tiefe > static_cast<int>(in.teile.size())) { return; }
    std::vector<int> unter;
    if (kinder) {
        for (const int k : teil.kinder) {
            if (!in.teile[static_cast<std::size_t>(k)].istTag()) { unter.push_back(k); }
        }
    }
    const bool gezeichnet = static_cast<std::size_t>(i) < t.gezeichnet.size() && t.gezeichnet[static_cast<std::size_t>(i)] != 0;
    const bool gewaehlt = std::find(t.auswahl.begin(), t.auswahl.end(), i) != t.auswahl.end();
    // Ohne SpanAvailWidth: die Zeile ist so breit wie ihr Text (der Baum rollt waagerecht).
    ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    if (unter.empty()) { f |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; }
    if (gewaehlt) { f |= ImGuiTreeNodeFlags_Selected; }
    std::string text = teil.name;
    if (t.g2.wurzel == i) { text += std::string("  (") + tr(Str::MbWurzel) + ")"; }
    if (!gezeichnet) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    ImGui::PushID(i);
    const bool offen = ImGui::TreeNodeEx("##fl", f, "%s", text.c_str());
    if (!gezeichnet) { ImGui::PopStyleColor(); }
    if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), teil.name.c_str()); }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) { waehleFlaeche(i); }
    flaechenMenue(i);
    ImGui::PopID();
    if (offen && !unter.empty()) {
        for (const int k : unter) { flaechenZeile(k, true, such, tiefe + 1); }
        ImGui::TreePop();
    }
}

void anbauMenue(const std::string& ziel, bool knochen) {
    if (!ImGui::BeginPopupContextItem("##anbaumenue")) { return; }
    if (ImGui::MenuItem(tr(Str::MbAnbauen))) {
        g_anbauZiel = ziel;
        g_anbauKnochen = knochen;
        g_anbauOeffnen = true;
    }
    const bool hat = std::any_of(mt().anbauten.begin(), mt().anbauten.end(), [&](const ModellAnbau& a) { return a.ziel == ziel; });
    if (ImGui::MenuItem(tr(Str::MbLoesen), nullptr, false, hat)) {
        auto& l = mt().anbauten;
        l.erase(std::remove_if(l.begin(), l.end(), [&](const ModellAnbau& a) { return a.ziel == ziel; }), l.end());
        g_app->modelDirty = true;
    }
    if (knochen) {
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::MbSekStartSetzen), nullptr, mt().sekundaerKnochen == ziel)) {
            mt().sekundaerKnochen = ziel;
            g_app->modelDirty = true;
        }
    }
    ImGui::EndPopup();
}

void anbautenUnter(const std::string& ziel) {
    for (const ModellAnbau& a : mt().anbauten) {
        if (a.ziel != ziel) { continue; }
        ImGui::Indent();
        ImGui::TextDisabled("%s", fileName(a.pfad).c_str());
        ImGui::Unindent();
    }
}

void knochenZeile(int b, const std::vector<std::vector<int>>& kinder, const std::string& such, int tiefe) {
    ModellTab& t = mt();
    const GlaAnimation& a = g_app->anim;
    if (tiefe > static_cast<int>(a.bones.size())) { return; }
    const GlaBone& k = a.bones[static_cast<std::size_t>(b)];
    const bool suche = !such.empty();
    if (suche && !passt(k.name, such)) {
        for (const int c : kinder[static_cast<std::size_t>(b)]) { knochenZeile(c, kinder, such, tiefe + 1); }
        return;
    }
    ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_OpenOnArrow;
    if (kinder[static_cast<std::size_t>(b)].empty() || suche) { f |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; }
    if (tiefe < 3) { f |= ImGuiTreeNodeFlags_DefaultOpen; }
    if (t.knochenWahl == b) { f |= ImGuiTreeNodeFlags_Selected; }
    ImGui::PushID(1000 + b);
    std::string text = k.name;
    if (k.name == t.sekundaerKnochen) { text += std::string("  (") + tr(Str::MbSekStart) + ")"; }
    const bool offen = ImGui::TreeNodeEx("##kn", f, "%s", text.c_str());
    if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), k.name.c_str()); }
    if (modellSpielKnochen(k.name)) {
        // Markierung am Zeilenende: dieser Knochen wird vom Spielcode gesucht.
        const ImVec2 hi = ImGui::GetItemRectMax();
        const float r = ImGui::GetFontSize() * 0.18F;
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{hi.x - r * 3.0F, (ImGui::GetItemRectMin().y + hi.y) * 0.5F}, r,
                                                    ImGui::GetColorU32(ImGuiCol_PlotHistogram));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbSpielcode)); }
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
        t.knochenWahl = b;
        g_app->modelDirty = true;
    }
    anbauMenue(k.name, true);
    ImGui::PopID();
    anbautenUnter(k.name);
    if (offen && (f & ImGuiTreeNodeFlags_NoTreePushOnOpen) == 0) {
        for (const int c : kinder[static_cast<std::size_t>(b)]) { knochenZeile(c, kinder, such, tiefe + 1); }
        ImGui::TreePop();
    } else if (suche) {
        for (const int c : kinder[static_cast<std::size_t>(b)]) { knochenZeile(c, kinder, such, tiefe + 1); }
    }
}

// Das Fenster "Modell anbauen": alle .glm der Spielarchive mit Suche.
void anbauFenster() {
    if (g_anbauOeffnen) {
        ImGui::OpenPopup("###mtanbau");
        g_anbauOeffnen = false;
    }
    char titel[200];
    std::snprintf(titel, sizeof titel, tr(Str::MbAnbauTitel), g_anbauZiel.c_str());
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 26.0F, fs * 26.0F}, ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal((std::string(titel) + "###mtanbau").c_str(), nullptr)) { return; }
    static std::vector<std::string> modelle;
    if (modelle.empty()) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".glm"})) { modelle.push_back(f.name); }
        }
        std::sort(modelle.begin(), modelle.end());
        modelle.erase(std::unique(modelle.begin(), modelle.end()), modelle.end());
    }
    static char suche[96] = "weapons2/saber";
    static bool ersetzen = true;
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##anbausuche", tr(Str::MbSuche), suche, sizeof suche);
    ImGui::Checkbox(tr(Str::MbAnbauErsetzen), &ersetzen);
    const std::string such = klein(suche);
    ImGui::BeginChild("##anbauliste", ImVec2{-1.0F, -ImGui::GetFrameHeightWithSpacing()}, ImGuiChildFlags_Borders);
    std::vector<const std::string*> treffer;
    for (const std::string& m : modelle) {
        if (passt(m, such)) { treffer.push_back(&m); }
    }
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(treffer.size()));
    while (clipper.Step()) {
        for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r) {
            const std::string& m = *treffer[static_cast<std::size_t>(r)];
            if (ImGui::Selectable(m.c_str())) {
                (void)modellAnbauen(m, g_anbauZiel, g_anbauKnochen, ersetzen);
                ImGui::CloseCurrentPopup();
            }
        }
    }
    ImGui::EndChild();
    if (ImGui::Button(tr(Str::EditorCancel))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

}  // namespace

void drawModellExplorer(float hoehe) {
    ModellTab& t = mt();
    ImGui::BeginChild("mtexplorer", ImVec2{-1.0F, hoehe}, ImGuiChildFlags_Borders);
    if (g_app->model.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::ModelNoneH2));
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        return;
    }
    modellAnzeigeAktualisieren();
    modellKlickPruefen();
    const modell::Inhalt& in = t.dok.inhalt();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ImGui::GetFontSize() * 0.35F, 2.0F});
    if (reiterKnopf(tr(Str::MbNachHierarchie), g_explorerArt == 0)) { g_explorerArt = 0; }
    ImGui::SameLine(0.0F, 2.0F);
    if (reiterKnopf(tr(Str::MbNachEbenen), g_explorerArt == 1)) { g_explorerArt = 1; }
    ImGui::PopStyleVar();
    if (g_sucheFokus) {
        ImGui::SetKeyboardFocusHere();
        g_sucheFokus = false;
    }
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##mtsuche", tr(Str::MbSuche), t.suche, sizeof t.suche);
    const std::string such = klein(t.suche);
    ImGui::BeginChild("mtbaum", ImVec2{-1.0F, -1.0F}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    int gezeichnet = 0;
    for (const char g : t.gezeichnet) { gezeichnet += g != 0 ? 1 : 0; }
    const std::string modellText = fileName(g_app->modelPath) + "  " + tr(Str::MbRModell);
    if (ImGui::TreeNodeEx("##mtmodell", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", modellText.c_str())) {
        char kopf[160];
        std::snprintf(kopf, sizeof kopf, tr(Str::MbFlaechen), t.lodJetzt + 1, gezeichnet);
        if (ImGui::TreeNodeEx("##mtflaechen", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", kopf)) {
            if (!such.empty()) {
                for (std::size_t i = 0; i < in.teile.size(); ++i) {
                    if (passt(in.teile[i].name, such)) { flaechenZeile(static_cast<int>(i), false, such, 0); }
                }
            } else if (g_explorerArt == 0) {
                for (std::size_t i = 0; i < in.teile.size(); ++i) {
                    if (in.teile[i].eltern < 0) { flaechenZeile(static_cast<int>(i), true, such, 0); }
                }
            } else {
                // Nach Ebenen: gezeichnet, aus, Kappen (Flagge OFF in der Datei).
                const Str kEbenen[3] = {Str::MbEbeneGezeichnet, Str::MbEbeneAus, Str::MbEbeneKappen};
                for (int e = 0; e < 3; ++e) {
                    if (!ImGui::TreeNodeEx(tr(kEbenen[e]), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) { continue; }
                    for (std::size_t i = 0; i < in.teile.size(); ++i) {
                        const bool kappe = (in.teile[i].flaggen & modell::kG2Aus) != 0U;
                        const bool an = i < t.gezeichnet.size() && t.gezeichnet[i] != 0;
                        const int ebene = kappe ? 2 : (an ? 0 : 1);
                        if (ebene == e) { flaechenZeile(static_cast<int>(i), false, such, 0); }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        int tags = 0;
        for (const modell::Teil& x : in.teile) { tags += x.istTag() ? 1 : 0; }
        std::snprintf(kopf, sizeof kopf, tr(Str::MbTags), tags);
        if (ImGui::TreeNodeEx("##mttags", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", kopf)) {
            for (std::size_t i = 0; i < in.teile.size(); ++i) {
                const modell::Teil& x = in.teile[i];
                if (!x.istTag() || !passt(x.name, such)) { continue; }
                ImGui::PushID(static_cast<int>(5000 + i));
                ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (t.tagWahl == static_cast<int>(i)) { f |= ImGuiTreeNodeFlags_Selected; }
                ImGui::TreeNodeEx("##tag", f, "%s", x.name.c_str());
                if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), x.name.c_str()); }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    t.tagWahl = static_cast<int>(i);
                    g_app->modelDirty = true;
                }
                anbauMenue(x.name, false);
                ImGui::PopID();
                anbautenUnter(x.name);
            }
            ImGui::TreePop();
        }
        if (!g_app->anim.bones.empty()) {
            std::snprintf(kopf, sizeof kopf, tr(Str::MbKnochen), fileName(g_app->anim.name).c_str(), static_cast<int>(g_app->anim.bones.size()));
            if (ImGui::TreeNodeEx("##mtknochen", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", kopf)) {
                std::vector<std::vector<int>> kinder(g_app->anim.bones.size());
                for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
                    const int p = g_app->anim.bones[b].parent;
                    if (p >= 0 && static_cast<std::size_t>(p) < kinder.size() && static_cast<std::size_t>(p) != b) {
                        kinder[static_cast<std::size_t>(p)].push_back(static_cast<int>(b));
                    }
                }
                for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
                    if (g_app->anim.bones[b].parent < 0) { knochenZeile(static_cast<int>(b), kinder, such, 0); }
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();
    anbauFenster();
    ImGui::EndChild();
}

// --- Befehlsfeld ------------------------------------------------------------------------------
namespace {

void glaFenster() {
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 24.0F, fs * 22.0F}, ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal((std::string(tr(Str::MbAndereGla)) + "###mtgla").c_str(), nullptr)) { return; }
    static std::vector<std::string> glas;
    if (glas.empty()) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".gla"})) { glas.push_back(f.name); }
        }
        std::sort(glas.begin(), glas.end());
        glas.erase(std::unique(glas.begin(), glas.end()), glas.end());
    }
    static char suche[96] = "";
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##glasuche", tr(Str::MbSuche), suche, sizeof suche);
    const std::string such = klein(suche);
    ImGui::BeginChild("##glaliste", ImVec2{-1.0F, -ImGui::GetFrameHeightWithSpacing()}, ImGuiChildFlags_Borders);
    for (const std::string& g : glas) {
        if (!passt(g, such)) { continue; }
        if (ImGui::Selectable(g.c_str())) {
            (void)modellGlaLaden(g);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();
    if (ImGui::Button(tr(Str::EditorCancel))) { ImGui::CloseCurrentPopup(); }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::ModelLoadGla))) {
        ImGui::CloseCurrentPopup();
        openSkeletonFile();
    }
    ImGui::EndPopup();
}

void rolloutModell() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    ImGui::TextDisabled("%s", tr(Str::MbDatei));
    ImGui::SameLine();
    ImGui::TextUnformatted(fileName(g_app->modelPath).c_str());
    ImGui::TextDisabled("%s", tr(Str::MbGla));
    ImGui::SameLine();
    ImGui::TextUnformatted(g_app->anim.name.empty() ? in.glaPfad.c_str() : g_app->anim.name.c_str());
    int flaechen = 0;
    int dreiecke = 0;
    int ecken = 0;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || static_cast<std::size_t>(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
        ++flaechen;
        dreiecke += in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].dreiecke();
        ecken += static_cast<int>(in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].ecken.size());
    }
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled(trN(Str::MbZahlen, flaechen, dreiecke, ecken), flaechen, dreiecke, ecken, t.lodJetzt + 1);
    ImGui::TextDisabled(trN(Str::MbKnochenZahl, in.knochenZahl, in.stufenZahl), in.knochenZahl, in.stufenZahl);
    if (!t.auswahl.empty()) {
        const int n = static_cast<int>(t.auswahl.size());
        ImGui::TextDisabled(trN(Str::MbAuswahl, n), n);
    }
    ImGui::PopTextWrapPos();
    if (ImGui::Button(tr(Str::MbAndereGla))) { ImGui::OpenPopup("###mtgla"); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbAndereGlaHint)); }
    glaFenster();
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MbFoto))) {
        const std::string p = platform::saveFileDialog(tr(Str::MbFoto), "Bitmap (*.bmp)|*.bmp", g_app->settings.lastDir,
                                                       fileName(directoryOf(g_app->modelPath)) + ".bmp");
        if (!p.empty()) { (void)modellFotoSpeichern(p, 1920); }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbFotoHint)); }
    if (ImGui::Button(tr(Str::MbTexNeu))) { modellTexturenNeuLaden(); }
}

void rolloutLod() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0F, ImGui::GetStyle().ItemSpacing.y});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ImGui::GetFontSize() * 0.25F, ImGui::GetStyle().FramePadding.y});
    if (reiterKnopf(tr(Str::MbLodAuto), t.lodWahl == 0)) { t.lodWahl = 0; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 24.0F, 0.0F});
        ImGui::SetTooltip("%s", tr(Str::MbLodAutoHint));
    }
    // LOD 1-10 wie ModView (F1-F10 dort); nur die vorhandenen sind waehlbar.
    for (int s = 1; s <= 10; ++s) {
        ImGui::SameLine();
        ImGui::PushID(s);   // eigene Kennung: "10" steht auch beim Blickwinkel
        ImGui::BeginDisabled(s > in.stufenZahl);
        char n[8];
        std::snprintf(n, sizeof n, "%d", s);
        if (reiterKnopf(n, t.lodWahl == s)) { t.lodWahl = s; }
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::PopStyleVar(2);
    ImGui::TextDisabled(tr(Str::MbLodJetzt), t.lodJetzt + 1, in.stufenZahl);
    // Picmip wie die Vorschau (r_picmip, Einstellung).
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(tr(Str::MbPicmip));
    for (int p = 0; p <= 3; ++p) {
        ImGui::SameLine();
        char n[8];
        std::snprintf(n, sizeof n, "%d##pm", p);
        if (reiterKnopf(n, g_app->settings.bildPicmip == p) && g_app->settings.bildPicmip != p) {
            g_app->settings.bildPicmip = p;
            modellTexturenNeuLaden();
        }
    }
    // Die Haeute des Modells (Liste wie ModView; "off" blendet Flaechen aus).
    for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
        if (ImGui::Selectable(g_app->modelSkins[i].c_str(), static_cast<int>(i) == g_app->modelSkinIndex)) {
            applySkinByIndex(static_cast<int>(i));
            ++g_app->modellHautWechsel;
        }
    }
}

void rolloutAnzeige() {
    ModellTab& t = mt();
    ModellAnzeige& a = t.an;
    bool neu = false;
    const auto schalter = [&](Str text, bool& wert) {
        if (ImGui::Checkbox(tr(text), &wert)) { neu = true; }
    };
    schalter(Str::MbAnDraht, a.drahtgitter);
    schalter(Str::MbAnKanten, a.kanten);
    schalter(Str::MbAnKnochen, g_app->modellKnochen);
    if (ImGui::Checkbox(tr(Str::MbAnFlossen), &g_app->settings.modellFlossen)) { neu = true; }
    schalter(Str::MbAnGewichte, a.gewichte);
    schalter(Str::MbAnNormalen, a.normalen);
    schalter(Str::MbAnTags, a.tags);
    schalter(Str::MbAnUrsprung, a.ursprung);
    schalter(Str::MbAnHuelle, a.huelle);
    schalter(Str::MbAnBoden, a.boden);
    if (a.boden) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0F);
        if (ImGui::DragFloat(tr(Str::MbAnBodenZ), &a.bodenZ, 0.5F, -512.0F, 512.0F, "%.1f")) { neu = true; }
    }
    schalter(Str::MbAnRaster, a.raster);
    bool weiss = !g_app->showModelTextures;
    if (ImGui::Checkbox(tr(Str::MbAnWeiss), &weiss)) {
        g_app->showModelTextures = !weiss;
        neu = true;
    }
    schalter(Str::MbAnRisse, a.risse);
    schalter(Str::MbAnZweiseitig, a.zweiseitig);
    schalter(Str::MbAnStatistik, a.statistik);
    schalter(Str::MbAnKlammer, a.klammer);
    // Blickwinkel: ModView kennt 10, 80 und 90 Grad.
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0F);
    if (ImGui::SliderFloat(tr(Str::MbAnFov), &g_app->modelCam.fovY, 10.0F, 120.0F, "%.0f")) { neu = true; }
    for (const float f : {10.0F, 80.0F, 90.0F}) {
        ImGui::SameLine();
        char n[8];
        std::snprintf(n, sizeof n, "%.0f", static_cast<double>(f));
        if (ImGui::SmallButton(n)) {
            g_app->modelCam.fovY = f;
            neu = true;
        }
    }
    if (neu) { g_app->modelDirty = true; }
}

void rolloutAnbauten() {
    ModellTab& t = mt();
    if (t.anbauten.empty()) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::MbAnbauKeine));
        ImGui::PopTextWrapPos();
        return;
    }
    for (std::size_t i = 0; i < t.anbauten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::SmallButton(tr(Str::MbLoesen))) {
            t.anbauten.erase(t.anbauten.begin() + static_cast<std::ptrdiff_t>(i));
            g_app->modelDirty = true;
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();
        ImGui::Text("%s -> %s", fileName(t.anbauten[i].pfad).c_str(), t.anbauten[i].ziel.c_str());
        ImGui::PopID();
    }
    if (ImGui::Button(tr(Str::MbAlleLoesen))) { modellAnbautenLoesen(); }
}

void rolloutPruefen() {
    ModellTab& t = mt();
    const modell::Grenzen& g = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##zielspiel", g.name)) {
        for (int s = 0; s < modell::kZielspiele; ++s) {
            if (ImGui::Selectable(modell::grenzen(static_cast<modell::Zielspiel>(s)).name, s == t.zielspiel)) {
                t.zielspiel = s;
                t.anzeigeSchluessel.clear();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbZielspiel)); }
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled(tr(Str::MbPrGrenzen), g.name, g.eckenJeFlaeche, g.dreieckeJeFlaeche, g.knochenJeFlaeche, g.gewichteJeEcke);
    ImGui::TextDisabled(tr(Str::MbPrInstanzen), 1, g.g2Modelle);
    const std::vector<modell::Befund> b = modellBefunde();
    if (b.empty()) {
        ImGui::Text(tr(Str::MbPrOk), g.name);
    }
    const ImVec4 warn = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
    for (const modell::Befund& x : b) {
        char z[400];
        switch (x.art) {
            case modell::BefundArt::Ecken:
                std::snprintf(z, sizeof z, tr(Str::MbPrEcken), x.name.c_str(), x.stufe + 1, x.wert, g.name, x.grenze);
                break;
            case modell::BefundArt::Dreiecke:
                std::snprintf(z, sizeof z, tr(Str::MbPrDreiecke), x.name.c_str(), x.stufe + 1, x.wert, g.name, x.grenze);
                break;
            case modell::BefundArt::Knochen:
                std::snprintf(z, sizeof z, tr(Str::MbPrKnochen), x.name.c_str(), x.stufe + 1, x.wert, x.grenze);
                break;
            case modell::BefundArt::Gewichte:
                std::snprintf(z, sizeof z, tr(Str::MbPrGewichte), x.name.c_str(), x.stufe + 1, x.wert, x.grenze);
                break;
            case modell::BefundArt::G2Modelle:
                std::snprintf(z, sizeof z, tr(Str::MbPrG2), x.wert, g.name, x.grenze);
                break;
            case modell::BefundArt::AnimHinterGla:
                std::snprintf(z, sizeof z, tr(Str::MbPrAnim), x.name.c_str(), x.wert, x.grenze,
                              tr(g.tPoseHinterGla ? Str::MbPrAnimTPose : Str::MbPrAnimStand));
                break;
            case modell::BefundArt::ShaderFehlt:
                std::snprintf(z, sizeof z, tr(Str::MbPrShader), x.name.c_str());
                break;
        }
        ImGui::TextColored(warn, "%s", z);
    }
    ImGui::PopTextWrapPos();
}

}  // namespace

void drawModellBefehlsfeld() {
    ModellTab& t = mt();
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled("%s - %s", tr(Str::MbBefehlsfeld), tr(bereichText(static_cast<int>(t.bereich))));
    ImGui::PopTextWrapPos();
    // Bedienmodus: wie 3ds Max (Vorgabe) oder wie Blender (Belegung in 1a-2).
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", tr(Str::MbBedienung));
    if (reiterKnopf(tr(Str::MbWieMax), g_app->settings.modellBedienung == 0)) { g_app->settings.modellBedienung = 0; }
    ImGui::SameLine(0.0F, 2.0F);
    if (reiterKnopf(tr(Str::MbWieBlender), g_app->settings.modellBedienung == 1)) { g_app->settings.modellBedienung = 1; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tr(Str::MbWieBlenderHint)); }
    ImGui::Separator();
    ImGui::BeginChild("mtrollouts", ImVec2{-1.0F, -1.0F});
    if (t.bereich != ModellBereich::Ansehen) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled(tr(Str::MbBereichSpaeter), bereichStufe(t.bereich));
        ImGui::PopTextWrapPos();
        ImGui::Separator();
    }
    if (!g_app->model.empty()) {
        if (t.bereich == ModellBereich::Ansehen && ImGui::CollapsingHeader(tr(Str::MbRModell), ImGuiTreeNodeFlags_DefaultOpen)) { rolloutModell(); }
        if (t.bereich == ModellBereich::Ansehen && ImGui::CollapsingHeader(tr(Str::MbRLod), ImGuiTreeNodeFlags_DefaultOpen)) { rolloutLod(); }
        // Die Anzeige gilt in jedem Bereich (Entwurf: "Die Anzeige-Schalter gelten auch in den anderen Bereichen").
        if (ImGui::CollapsingHeader(tr(Str::MbRAnzeige), ImGuiTreeNodeFlags_DefaultOpen)) { rolloutAnzeige(); }
        if (t.bereich == ModellBereich::Ansehen && ImGui::CollapsingHeader(tr(Str::MbRAnbauten), ImGuiTreeNodeFlags_DefaultOpen)) { rolloutAnbauten(); }
        if (t.bereich == ModellBereich::Ansehen && ImGui::CollapsingHeader(tr(Str::MbRPruefen), ImGuiTreeNodeFlags_DefaultOpen)) { rolloutPruefen(); }
    }
    if (!t.meldung.empty()) {
        ImGui::Separator();
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", t.meldung.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();
}

// --- Tasten wie 3ds Max --------------------------------------------------------------------
//
// Die Belegung aus der Tastenkarte von Autodesk (belege/max_hotkeys_de.txt,
// keyboard_mapDE-max2019) und der Doku, soweit sie fuer Ansehen/Pruefen gilt.
// Die Liste ist zugleich die Pruefliste des Selbsttests (modellTasteZahl /
// modellTasteAusfuehren) und der Inhalt der Suche "Alle Aktionen suchen" (X).
namespace {

struct MaxTaste {
    ImGuiKeyChord chord;
    Str text;
    std::function<void()> tun;
};

void bildSchritt(int d) {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return; }
    const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
    g_app->animFrame = std::clamp(g_app->animFrame + d, e.firstFrame, e.firstFrame + std::max(e.numFrames - 1, 0));
    g_app->animTime = 0.0F;
    g_app->animPlaying = false;
    g_app->modelDirty = true;
}

void bildRand(bool ende) {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return; }
    const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
    g_app->animFrame = ende ? e.firstFrame + std::max(e.numFrames - 1, 0) : e.firstFrame;
    g_app->animTime = 0.0F;
    g_app->modelDirty = true;
}

void ansichtArt(AnsichtArt a) {
    ModellTab& t = mt();
    t.ansichten[t.aktiv].art = a;
    modellAnsichtEinpassenAktiv();
}

void achseSetzen(int a) {
    ModellTab& t = mt();
    t.achse = t.achse == a ? -1 : a;
}

const std::vector<MaxTaste>& maxTasten() {
    static const std::vector<MaxTaste> kListe = {
        {ImGuiKey_Q, Str::MbAktAuswaehlen, [] { mt().gizmo = 0; }},
        {ImGuiKey_W, Str::MbAktVerschieben, [] { mt().gizmo = 1; }},
        {ImGuiKey_E, Str::MbAktDrehen, [] { mt().gizmo = 2; }},
        {ImGuiKey_R, Str::MbAktSkalieren, [] { mt().gizmo = 3; }},
        {ImGuiMod_Alt | ImGuiKey_W, Str::MbAktMax, [] { mt().vierAnsichten = !mt().vierAnsichten; }},
        {ImGuiKey_Z, Str::MbAktZoom, [] { modellAnsichtEinpassenAktiv(); }},
        {ImGuiKey_F3, Str::MbAnDraht, [] { mt().an.drahtgitter = !mt().an.drahtgitter; }},
        {ImGuiKey_F4, Str::MbAnKanten, [] { mt().an.kanten = !mt().an.kanten; }},
        {ImGuiKey_F5, Str::MbAktX, [] { achseSetzen(0); }},
        {ImGuiKey_F6, Str::MbAktY, [] { achseSetzen(1); }},
        {ImGuiKey_F7, Str::MbAktZ, [] { achseSetzen(2); }},
        {ImGuiKey_F8, Str::MbAktEbenen, [] { mt().achse = mt().achse >= 3 && mt().achse < 5 ? mt().achse + 1 : 3; }},
        {ImGuiKey_Equal, Str::MbAktGizmoPlus, [] { mt().gizmoGroesse = std::min(mt().gizmoGroesse * 1.25F, 4.0F); }},
        {ImGuiKey_Minus, Str::MbAktGizmoMinus, [] { mt().gizmoGroesse = std::max(mt().gizmoGroesse / 1.25F, 0.25F); }},
        {ImGuiKey_P, Str::MbAktPersp, [] { ansichtArt(AnsichtArt::Perspektive); }},
        {ImGuiKey_T, Str::MbAktOben, [] { ansichtArt(AnsichtArt::Oben); }},
        {ImGuiKey_F, Str::MbAktVorne, [] { ansichtArt(AnsichtArt::Vorne); }},
        {ImGuiKey_L, Str::MbAktLinks, [] { ansichtArt(AnsichtArt::Links); }},
        {ImGuiKey_B, Str::MbAktUnten, [] { ansichtArt(AnsichtArt::Unten); }},
        {ImGuiMod_Ctrl | ImGuiKey_A, Str::MbAktAlle, [] {
             ModellTab& t = mt();
             t.auswahl.clear();
             for (std::size_t i = 0; i < t.gezeichnet.size(); ++i) {
                 if (t.gezeichnet[i] != 0) { t.auswahl.push_back(static_cast<int>(i)); }
             }
         }},
        {ImGuiMod_Ctrl | ImGuiKey_D, Str::MbAktNichts, [] { mt().auswahl.clear(); }},
        {ImGuiMod_Ctrl | ImGuiKey_I, Str::MbAktUmkehren, [] {
             ModellTab& t = mt();
             std::vector<int> neu;
             for (std::size_t i = 0; i < t.gezeichnet.size(); ++i) {
                 if (t.gezeichnet[i] != 0 && std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) == t.auswahl.end()) {
                     neu.push_back(static_cast<int>(i));
                 }
             }
             t.auswahl = neu;
         }},
        {ImGuiKey_Comma, Str::MbAktZurueck, [] { bildSchritt(-1); }},
        {ImGuiKey_Period, Str::MbAktVor, [] { bildSchritt(1); }},
        {ImGuiKey_Slash, Str::MbAktAbspielen, [] {
             if (g_app->animIndex >= 0) { g_app->animPlaying = !g_app->animPlaying; }
         }},
        {ImGuiKey_Home, Str::MbAktAnfang, [] { bildRand(false); }},
        {ImGuiKey_End, Str::MbAktEnde, [] { bildRand(true); }},
        {ImGuiKey_G, Str::MbAnRaster, [] { mt().an.raster = !mt().an.raster; }},
        {ImGuiKey_J, Str::MbAnKlammer, [] { mt().an.klammer = !mt().an.klammer; }},
        {ImGuiKey_7, Str::MbAnStatistik, [] { mt().an.statistik = !mt().an.statistik; }},
        {ImGuiKey_H, Str::MbAktName, [] { g_sucheFokus = true; }},
        {ImGuiKey_X, Str::MbAktionen, [] { mt().aktionenOffen = true; }},
        {ImGuiMod_Ctrl | ImGuiKey_Z, Str::MbAktUndo, [] { (void)modellRueckgaengig(); }},
        {ImGuiMod_Ctrl | ImGuiKey_Y, Str::MbAktRedo, [] { (void)modellWiederholen(); }},
        {ImGuiMod_Alt | ImGuiKey_R, Str::MbAktTexNeu, [] { modellTexturenNeuLaden(); }},
    };
    return kListe;
}

void tasteAusloesen(const MaxTaste& k) {
    k.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(k.text);
    g_app->modelDirty = true;
}

}  // namespace

bool modellFokus() {
    return g_app->leftMode == 2 && mt().fokus && !ImGui::GetIO().WantTextInput && !g_app->model.empty();
}

bool modellTasten() {
    if (!modellFokus()) { return false; }
    for (const MaxTaste& k : maxTasten()) {
        if (ImGui::IsKeyChordPressed(k.chord)) {
            tasteAusloesen(k);
            return true;
        }
    }
    return false;
}

int modellTasteZahl() { return static_cast<int>(maxTasten().size()); }

ImGuiKeyChord modellTasteChord(int i) { return maxTasten()[static_cast<std::size_t>(i)].chord; }

const char* modellTasteText(int i) { return tr(maxTasten()[static_cast<std::size_t>(i)].text); }

// "Alle Aktionen suchen" (X): die Liste der Tasten, zum Ausfuehren anklicken.
void drawModellAktionen() {
    ModellTab& t = mt();
    if (t.aktionenOffen) {
        ImGui::OpenPopup("###mtaktionen");
        t.aktionenOffen = false;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 24.0F, fs * 24.0F}, ImGuiCond_Appearing);
    t.aktionenSichtbar = ImGui::BeginPopup("###mtaktionen");
    if (!t.aktionenSichtbar) { return; }
    static char suche[64] = "";
    if (ImGui::IsWindowAppearing()) {
        suche[0] = '\0';
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(-1.0F);
    // Enter fuehrt den ersten Treffer aus, Esc schliesst (auch aus dem Suchfeld).
    const bool enter = ImGui::InputTextWithHint("##aktsuche", tr(Str::MbAktionen), suche, sizeof suche, ImGuiInputTextFlags_EnterReturnsTrue);
    const std::string such = klein(suche);
    bool erster = enter;
    for (const MaxTaste& k : maxTasten()) {
        if (!passt(tr(k.text), such)) { continue; }
        if (ImGui::Selectable(tr(k.text)) || erster) {
            tasteAusloesen(k);
            ImGui::CloseCurrentPopup();
            break;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

}  // namespace bhed::gui
