// app_modelltab_pruefen.cpp - AP5 (MBENTWURF Teil B, 12.11.): der Bereich Ansehen/Pruefen des Model-Tabs vollstaendig nach dem
// freigegebenen Entwurf (plaene/modellblock_stufe0/entwurf_modelltab.html, feldInhalt "pruefen"; Abgleichtabelle P1-P11, X18, X19,
// R7, R10, Z20)
//
//   Gruppen         Detailstufe und Skin: die Skins zweispaltig (Z20, bis rc611 in der Zeitleiste); Boden und Kamera (P4);
//                   Auswahl (X18: die Info der Zeile als Tabelle); Pruefung fuer das Spiel: Balken und Zeilen (P6); Szene (P7)
//   Anzeige         die fuenf Schalter von ModView, die bis AP5 fehlten (P3a): Alpha, Texturfilter, Knochengewicht-Schwelle,
//                   Flaeche hervorheben (+ Knochenbezuege), nicht schattenwerfende Flaechen - Schalter in den Menues der
//                   Kopfzeile (gui/app_modelltab_kopf.cpp), Wirkung hier und im Zeichner (gui/gpumap_win32.cpp)
//   Ansicht         der Hinweis "LOD - Dreiecke - wie im Spiel gezeichnet" unten rechts (R10); Overlays an/aus wie Blender
//                   (Umschalt+Alt+Z) und die Schattierungs-Auswahl (Z) (P10)
//   vier Ansichten  wie 3ds Max ab Werk in Modellieren, Weights, Painting, Animation; Alt+W merkt sich die Wahl je Bereich (R7)
//
// Vorbild ModView (Raven, JACoders/OpenJK-tools ModView/, GPL v2): model.h (AppVars), r_glm.cpp (Schwelle), r_surface.cpp
// (Hervorheben, Unshadowable), model.cpp (Knochenbezuege), textures.cpp (Filter), script.cpp (Szene, bhed/modell/szene.h).
#include "app_internal.h"
#include "platform.h"
#include "textumbruch.h"
#include "tour.h"

#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/szene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

ModellPruefen g_pr;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Dreiecke der gezeichneten Stufe ueber alle Flaechen (wie die Kurzinfo der Gruppe Modell) bzw. nur ueber die gezeichneten
int dreieckeJetzt(bool nurGezeichnete) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    int n = 0;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || static_cast<std::size_t>(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
        if (nurGezeichnete && !modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        n += in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].dreiecke();
    }
    return n;
}

// R7: wie 3ds Max ab Werk vier Ansichten in den Bereichen, die der Entwurf so zeigt (Modellieren, Weights, Painting, Animation);
// Ansehen/Pruefen eine maximierte Perspektive, UV das UV-Fenster neben einer Ansicht.
bool vierVorgabe(ModellBereich b) { return b == ModellBereich::Modellieren || b == ModellBereich::Weights || b == ModellBereich::Painting || b == ModellBereich::Animation; }

// Gestrichelte Linie in Bildschirmpunkten (ModView: glLineStipple(8, 0xAAAA) - Striche und Luecken gleich lang)
void strichLinie(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 farbe, float strich, float dicke) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float laenge = std::sqrt(dx * dx + dy * dy);
    if (laenge < 0.5F) { return; }
    const float ux = dx / laenge;
    const float uy = dy / laenge;
    for (float s = 0.0F; s < laenge; s += strich * 2.0F) {
        const float e = std::min(s + strich, laenge);
        dl->AddLine(ImVec2{a.x + ux * s, a.y + uy * s}, ImVec2{a.x + ux * e, a.y + uy * e}, farbe, dicke);
    }
}

// Die Knochen, auf die sich eine Flaeche bezieht (Gewicht > 0) - ModView Model_SurfaceContainsBoneReference
std::vector<int> knochenBezuege(const GlmSurface& s) {
    std::vector<int> b;
    for (const GlmVertex& v : s.verts) {
        for (int k = 0; k < 4; ++k) {
            if (v.weights[k] > 0.0F && std::find(b.begin(), b.end(), static_cast<int>(v.bones[k])) == b.end()) { b.push_back(static_cast<int>(v.bones[k])); }
        }
    }
    std::sort(b.begin(), b.end());
    return b;
}

std::string pfadImSpiel(const std::string& p) {
    // models/... ab der letzten Stelle, an der der Spielpfad beginnt (eine .glm von der Platte liegt unter .../base/models/...)
    std::string g = p;
    std::replace(g.begin(), g.end(), '\\', '/');
    const std::string k = klein(g);
    const std::size_t m = k.rfind("models/");
    return m == std::string::npos ? g : g.substr(m);
}

}  // namespace

ModellPruefen& modellPr() { return g_pr; }

// --- Z20: die Skins in "Detailstufe und Skin" ----------------------------------------------------------------------------
void modellPrHaeute() {
    if (g_app->model.empty() || g_app->modelSkins.empty()) { return; }
    ImGuiContext& g = *GImGui;
    if (ImGui::GetCurrentWindow()->SkipItems) { return; }
    const float fs = ImGui::GetFontSize();
    int gezeigt = 0;
    int alle = 0;
    for (const GlmSurface& sf : g_app->model.surfaces) {
        if (sf.isTag()) { continue; }
        ++alle;
        if (sf.isVisible(g_app->showCaps)) { ++gezeigt; }
    }
    char zahl[160];
    std::snprintf(zahl, sizeof zahl, trN(Str::ModelSurfaces, gezeigt, alle), gezeigt, alle);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Zeile "Skin" (Tooltip, im Element-Register fuer Helfer und Touren) und rechts der Flaechenzaehler: Klick schreibt alle Flaechen
    // ins Detailprotokoll (bis rc611 in der Zeitleiste)
    const char* name = tr(Str::ModelSkin);
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const ImVec2 ng = ImGui::CalcTextSize(name);
    ImGui::InvisibleButton("##pqhautname", ImVec2{std::max(ng.x, 1.0F), ImGui::GetTextLineHeight()});
    if (g.TestEngineHookItems) { selbsttestMerkeText(&g, g.LastItemData.ID, name); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", tr(Str::ModelSkinHint)); }
    dl->AddText(o, ImGui::GetColorU32(ImGuiCol_TextDisabled), name);
    const ImVec2 zg = ImGui::CalcTextSize(zahl);
    const float rest = ImGui::GetContentRegionAvail().x;
    ImGui::SameLine(0.0F, std::max(fs, rest - ng.x - zg.x));
    const ImVec2 zo = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##pqflaechen", ImVec2{std::max(zg.x, 1.0F), ImGui::GetTextLineHeight()});
    if (g.TestEngineHookItems) { selbsttestMerkeText(&g, g.LastItemData.ID, zahl); }
    const bool ueber = ImGui::IsItemHovered();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", tr(Str::ModelSurfaceDump)); }
    tourZielMelden("modell.flaechen", ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()});   // H2: Ziel der Tour k-skins
    if (ImGui::IsItemClicked()) {
        modellFlaechenProtokoll();
        ++g_app->modellFlaechenListen;
    }
    dl->AddText(zo, ImGui::GetColorU32(ueber ? ImGuiCol_Text : ImGuiCol_TextDisabled), zahl);
    // die Skins als zweispaltige Auswahl (Entwurf: model_default.skin | model_blue.skin ...)
    std::vector<const char*> texte;
    for (const std::string& h : g_app->modelSkins) { texte.push_back(h.c_str()); }
    int wahl = g_app->modelSkinIndex;
    if (ui::segment("##pqhaeute", texte.data(), static_cast<int>(texte.size()), &wahl, 2, -1, nullptr, 0.0F, true) && wahl >= 0) {
        applySkinByIndex(wahl);
        ++g_app->modellHautWechsel;
    }
}

// --- X18: Gruppe "Auswahl" -------------------------------------------------------------------------------------------------
void modellPrRolloutAuswahl() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    ExplorerArt art = ExplorerArt::Flaeche;
    int index = -1;
    std::string name;
    if (!t.auswahl.empty() && t.auswahl.front() >= 0 && static_cast<std::size_t>(t.auswahl.front()) < in.teile.size()) {
        index = t.auswahl.front();
        name = in.teile[static_cast<std::size_t>(index)].name;
    } else if (t.knochenWahl >= 0 && static_cast<std::size_t>(t.knochenWahl) < g_app->anim.bones.size()) {
        art = ExplorerArt::Knochen;
        index = t.knochenWahl;
        name = g_app->anim.bones[static_cast<std::size_t>(index)].name;
    } else if (t.tagWahl >= 0 && static_cast<std::size_t>(t.tagWahl) < in.teile.size()) {
        art = ExplorerArt::Tag;
        index = t.tagWahl;
        name = in.teile[static_cast<std::size_t>(index)].name;
    }
    if (!ui::rolloutBeginn(tr(Str::PqRAuswahl), {.kurzinfo = name.empty() ? nullptr : name.c_str(), .reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Objekt)})) { return; }
    if (index < 0) {
        ui::tipp(tr(Str::PqAuswahlKeine));
        ui::rolloutEnde();
        return;
    }
    // die Info der Zeile (Tooltip und "Info" im Szenen-Explorer): "Name: Wert" je Zeile, sonst ein Satz
    const std::string info = modellExplorerInfo(art, index);
    std::vector<ui::WertZeile> werte;
    std::vector<std::string> saetze;
    std::size_t a = 0;
    while (a < info.size()) {
        std::size_t b = info.find('\n', a);
        if (b == std::string::npos) { b = info.size(); }
        const std::string z = info.substr(a, b - a);
        a = b + 1;
        if (z.empty()) { continue; }
        const std::size_t p = z.find(": ");
        if (p == std::string::npos) {
            saetze.push_back(z);
        } else {
            werte.push_back({z.substr(0, p), z.substr(p + 2), ui::Ton::Normal, {}});
        }
    }
    ui::wertTabelle(werte.data(), static_cast<int>(werte.size()));
    for (const std::string& s : saetze) { ui::absatz(s.c_str()); }
    if (t.auswahl.size() > 1) {
        char mehr[96];
        std::snprintf(mehr, sizeof mehr, tr(Str::PqAuswahlMehr), static_cast<int>(t.auswahl.size()) - 1);
        ui::absatz(mehr);
    }
    ui::rolloutEnde();
}

// --- P4: Gruppe "Boden und Kamera" -------------------------------------------------------------------------------------------
void modellPrRolloutBoden() {
    ModellTab& t = mt();
    if (!ui::rolloutBeginn(tr(Str::PqRBoden), {.reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Objekt)})) { return; }
    // Boden: -24 wie ModView (DT85: grauer Boden auf -24) oder unter dem tiefsten Punkt der Figur in der Pose (MBRASTER)
    {
        const char* texte[2] = {"-24", tr(Str::MrTiefster)};
        const char* hinweise[2] = {tr(Str::PqBodenHint), tr(Str::MrTiefsterHint)};
        int wahl = !t.an.boden ? -1 : (t.an.bodenZ == -24.0F ? 0 : 1);
        ui::zeile(tr(Str::PqBoden), tr(Str::PqBodenHint));
        if (ui::segment("##pqboden", texte, 2, &wahl, 2, -1, hinweise, ui::feldBreite())) {
            t.an.boden = true;
            if (wahl == 0) {
                t.an.bodenZ = -24.0F;
            } else {
                modellBodenAufTiefstenPunkt();
            }
            g_app->modelDirty = true;
        }
    }
    // FOV: ModView kennt 10, 80 und 90 Grad (der Regler 10-120 steht im Menue der Kopfzeile)
    {
        static const float kWinkel[3] = {10.0F, 80.0F, 90.0F};
        const char* texte[3] = {"10\xc2\xb0", "80\xc2\xb0", "90\xc2\xb0"};
        int wahl = -1;
        for (int i = 0; i < 3; ++i) {
            if (std::fabs(g_app->modelCam.fovY - kWinkel[i]) < 0.5F) { wahl = i; }
        }
        ui::zeile("FOV", tr(Str::PqFovHint));
        if (ui::segment("##pqfov", texte, 3, &wahl, 3, -1, nullptr, ui::feldBreite())) {
            g_app->modelCam.fovY = kWinkel[wahl];
            g_app->modelDirty = true;
        }
    }
    // Hintergrund: einfarbig wie ModView (Clear colour), ohne Haken der Verlauf
    {
        ui::zeile(tr(Str::PqHintergrund), tr(Str::PqHintergrundHint));
        bool eigen = g_app->settings.modellHintergrund >= 0;
        if (ImGui::Checkbox("##pqhgan", &eigen)) {
            g_app->settings.modellHintergrund = eigen ? 0x333333 : -1;
            g_app->modelDirty = true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::PqHintergrundHint));
        ImGui::SameLine();
        const int h = g_app->settings.modellHintergrund >= 0 ? g_app->settings.modellHintergrund : 0x333333;   // ohne Haken: die Farbe, die der Haken setzt
        float f[3] = {static_cast<float>((h >> 16) & 0xFF) / 255.0F, static_cast<float>((h >> 8) & 0xFF) / 255.0F, static_cast<float>(h & 0xFF) / 255.0F};
        if (ImGui::ColorEdit3("##pqhg", f, ImGuiColorEditFlags_NoInputs)) {
            const auto b = [](float x) { return static_cast<int>(std::lround(std::clamp(x, 0.0F, 1.0F) * 255.0F)); };
            g_app->settings.modellHintergrund = (b(f[0]) << 16) | (b(f[1]) << 8) | b(f[2]);
            g_app->modelDirty = true;
        }
    }
    // Gewicht-Schwelle: die Zahl zur Knochengewicht-Schwelle (P3a)
    if (ui::regler(tr(Str::PqSchwelleWert), &g_pr.schwelle, 0.0F, 1.0F, "%.2f", tr(Str::PqSchwelleWertHint), ImGuiSliderFlags_AlwaysClamp)) {
        g_pr.schwelleAn = true;
        g_app->modelDirty = true;
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::PqAnsichtZurueck), {.icon = UiIcon::Einpassen, .hinweis = tr(Str::PqAnsichtZurueckHint)})) { modellAnsichtEinpassen(); }
    if (ui::werkzeugKnopf(tr(Str::PqFoto), {.icon = UiIcon::Kamera, .hinweis = tr(Str::MbFotoHint)})) {
        const std::string p = platform::saveFileDialog(tr(Str::MbFoto), "Bitmap (*.bmp)|*.bmp", g_app->settings.lastDir,
                                                       fileName(directoryOf(g_app->modelPath)) + ".bmp");
        if (!p.empty()) { (void)modellFotoSpeichern(p, 1920); }
    }
    ui::rasterEnde();
    ui::rolloutEnde();
}

// --- P7: Gruppe "Szene" ------------------------------------------------------------------------------------------------------
void modellPrRolloutSzene() {
    if (!ui::rolloutBeginn(tr(Str::PqRSzene), {.reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Objekt)})) { return; }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::PqSzeneSpeichern), {.icon = UiIcon::Speichern, .hinweis = tr(Str::PqSzeneSpeichernHint)})) {
        const std::string p = platform::saveFileDialog(tr(Str::PqSzeneSpeichern), "ModView script files (*.mvs)|*.mvs", g_app->settings.lastDir,
                                                       fileName(directoryOf(g_app->modelPath)) + ".mvs");
        if (!p.empty()) { (void)modellSzeneSpeichern(p); }
    }
    if (ui::werkzeugKnopf(tr(Str::PqMvsLesen), {.icon = UiIcon::Oeffnen, .hinweis = tr(Str::PqMvsLesenHint)})) {
        const std::string p = platform::openFileDialog(tr(Str::PqMvsLesen), "ModView script files (*.mvs)|*.mvs|All files (*.*)|*.*", g_app->settings.lastDir);
        if (!p.empty()) {
            modellVerwerfenDann([p] { (void)modellSzeneLesen(p); });   // pfix H3: ungespeicherte Aenderungen zuerst
        }
    }
    if (ui::werkzeugKnopf(tr(Str::PqNpc), {.icon = UiIcon::Figur, .hinweis = tr(Str::NpcBrowserHint), .an = g_app->npcBrowserOffen})) {
        g_app->npcBrowserOffen = !g_app->npcBrowserOffen;
    }
    ui::rasterEnde();
    ui::rolloutEnde();
}

// --- P7: Szene speichern / lesen ---------------------------------------------------------------------------------------------
bool modellSzeneSpeichern(const std::string& pfad) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    modell::Szene s;
    s.name = modell::szenenKennung(g_app->modelPath, {});
    s.modell = pfadImSpiel(g_app->modelPath);
    if (t.g2.wurzel > 0 && static_cast<std::size_t>(t.g2.wurzel) < in.teile.size()) { s.wurzel = in.teile[static_cast<std::size_t>(t.g2.wurzel)].name; }
    // ModView schreibt den Sekundaer-Start nur mit gesperrter Sekundaersequenz (Model_SecondaryAnimLockingActive)
    const auto sequenz = [](int i) { return i >= 0 && static_cast<std::size_t>(i) < g_app->animList.size() ? g_app->animList[static_cast<std::size_t>(i)].name : std::string(); };
    s.primaer = sequenz(g_app->animIndex);
    s.sekundaer = sequenz(t.sekundaer);
    if (!s.sekundaer.empty()) { s.sekundaerStart = t.sekundaerKnochen; }
    if (g_app->modelSkinIndex >= 0 && static_cast<std::size_t>(g_app->modelSkinIndex) < g_app->modelSkins.size()) {
        s.haut = directoryOf(s.modell) + "/" + g_app->modelSkins[static_cast<std::size_t>(g_app->modelSkinIndex)];
    }
    // die vom Anwender geschalteten Flaechen (G2API_SetSurfaceOnOff - ModView: Model_GetSurfaceDifferentFromDefault): was die Haut
    // schaltet (G2API_SetSurfaceOnOffFromSkin), gehoert zur Vorgabe und steht nicht in der Datei
    modell::G2Zustand vorgabe;
    modell::hautAnwenden(in, vorgabe, modell::hautAusModell(in, g_app->model));
    for (const modell::FlaechenSchalter& f : t.g2.slist) {
        if (f.flaeche < 0 || static_cast<std::size_t>(f.flaeche) >= in.teile.size()) { continue; }
        const bool ausHaut = std::any_of(vorgabe.slist.begin(), vorgabe.slist.end(),
                                         [&](const modell::FlaechenSchalter& v) { return v.flaeche == f.flaeche && v.offFlags == f.offFlags; });
        if (ausHaut) { continue; }
        const std::string& n = in.teile[static_cast<std::size_t>(f.flaeche)].name;
        if ((f.offFlags & modell::kG2OhneKinder) != 0U) {
            s.flaechenOhneKinder.push_back(n);
        } else if ((f.offFlags & modell::kG2Aus) != 0U) {
            s.flaechenAus.push_back(n);
        } else {
            s.flaechenAn.push_back(n);
        }
    }
    std::vector<std::string> kennungen{s.name};
    for (const ModellAnbau& a : t.anbauten) {
        modell::SzenenAnbau b;
        b.name = modell::szenenKennung(a.pfad, kennungen);
        kennungen.push_back(b.name);
        b.modell = pfadImSpiel(a.pfad);
        b.eltern = s.name;
        b.ziel = a.ziel;
        b.knochen = a.knochen;
        s.anbauten.push_back(std::move(b));
    }
    s.lod = t.lodWahl;
    s.sichtfeld = g_app->modelCam.fovY;
    s.boden = t.an.boden ? 1 : 0;
    s.bodenZ = t.an.bodenZ;
    std::ofstream f(pfad, std::ios::binary);
    const std::string text = modell::szeneSchreiben(s);
    f << text;
    char z[600];
    if (!f.good()) {
        std::snprintf(z, sizeof z, tr(Str::PqSzeneSpeichernFehler), pfad.c_str());
        t.meldung = z;
        return false;
    }
    std::snprintf(z, sizeof z, tr(Str::PqSzeneGespeichert), pfad.c_str());
    t.meldung = z;
    g_app->settings.lastDir = directoryOf(pfad);
    return true;
}

bool modellSzeneLesen(const std::string& pfad) {
    ModellTab& t = mt();
    std::ifstream f(pfad, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    modell::Szene s;
    std::string fehler;
    char z[900];
    // FIX612 R6: der Grund in der Sprache der Oberflaeche (bis rc612 "File not found" und die englischen Texte von ModView)
    std::string grund;
    modell::SzeneFehler was;
    if (!f.good() && !f.eof()) {
        grund = tr(Str::PqSzeneNichtOffen);
    } else if (!modell::szeneLesen(ss.str(), s, &fehler, &was)) {
        char g[400] = {};
        switch (was.art) {
            case modell::SzeneFehlerArt::Klammer: std::snprintf(g, sizeof g, tr(Str::PqSzeneKlammer), was.zeile); break;
            case modell::SzeneFehlerArt::KeinBlock: std::snprintf(g, sizeof g, tr(Str::PqSzeneKeinBlock), was.block.c_str()); break;
            case modell::SzeneFehlerArt::KeinSchluessel: std::snprintf(g, sizeof g, tr(Str::PqSzeneKeinSchluessel), was.block.c_str(), was.schluessel.c_str()); break;
            case modell::SzeneFehlerArt::ElternUnbekannt: std::snprintf(g, sizeof g, tr(Str::PqSzeneEltern), was.block.c_str(), was.schluessel.c_str()); break;
            case modell::SzeneFehlerArt::Keiner: std::snprintf(g, sizeof g, "%s", fehler.c_str()); break;
        }
        grund = g;
        diag::detail("Model-Tab: Szene " + pfad + " nicht gelesen - " + fehler);
    }
    if (!grund.empty()) {
        std::snprintf(z, sizeof z, tr(Str::PqSzeneFehler), (pfad + ": " + grund).c_str());
        t.meldung = z;
        return false;
    }
    // das Modell: ModView liest gamedir + modelfile; JK Studio sucht es in den Archiven und losen Dateien des Spiels
    if (!oeffneModellAusArchiv(s.modell, &fehler)) {
        std::snprintf(z, sizeof z, tr(Str::PqSzeneFehler), (s.modell + ": " + fehler).c_str());
        t.meldung = z;
        return false;
    }
    modellAnzeigeAktualisieren();   // das Dokument zum neuen Modell (die Flaechen werden ueber ihren Namen geschaltet)
    const modell::Inhalt& in = t.dok.inhalt();
    std::vector<std::string> ohne = s.nichtUebernommen;
    // Haut (oldskinfile): der Dateiname unter den Haeuten des Modells
    if (!s.haut.empty()) {
        const std::string h = klein(fileName(s.haut));
        bool gefunden = false;
        for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
            if (klein(g_app->modelSkins[i]) != h) { continue; }
            applySkinByIndex(static_cast<int>(i));
            gefunden = true;
        }
        if (!gefunden) { ohne.push_back("oldskinfile " + s.haut); }
        modellAnzeigeAktualisieren();   // G2API_SetSurfaceOnOffFromSkin jetzt - es leert die Ueberschreibungen, die danach kommen
    }
    const auto flaeche = [&](const std::string& n) {
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (klein(in.teile[i].name) == klein(n)) { return static_cast<int>(i); }
        }
        return -1;
    };
    // Reihenfolge wie Script_Parse_ReadOptionalArgs: Wurzel, Sekundaer-Start, Sequenzen, Haut, Flaechen an / aus / aus ohne Kinder
    if (!s.wurzel.empty()) {
        const int w = flaeche(s.wurzel);
        if (w >= 0) {
            modellWurzelSetzen(w);
        } else {
            ohne.push_back("surfacename_rootoverride " + s.wurzel);
        }
    }
    if (!s.sekundaerStart.empty()) { t.sekundaerKnochen = s.sekundaerStart; }
    const auto sequenz = [](const std::string& n) {
        for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
            if (klein(g_app->animList[i].name) == klein(n)) { return static_cast<int>(i); }
        }
        return -1;
    };
    if (!s.primaer.empty()) {
        const int i = sequenz(s.primaer);
        if (i >= 0) {
            animWaehlen(i, true);
        } else {
            ohne.push_back("locksequence " + s.primaer);
        }
    }
    if (!s.sekundaer.empty()) {
        const int i = sequenz(s.sekundaer);
        if (i >= 0) {
            t.sekundaer = i;
        } else {
            ohne.push_back("locksequence_secondary " + s.sekundaer);
        }
    }
    const auto schalten = [&](const std::vector<std::string>& namen, std::uint32_t flags) {
        for (const std::string& n : namen) {
            const int i = flaeche(n);
            if (i >= 0) {
                modellFlaecheSchalten(i, flags);
            } else {
                ohne.push_back(n);
            }
        }
    };
    schalten(s.flaechenAn, 0U);
    schalten(s.flaechenAus, modell::kG2Aus);
    schalten(s.flaechenOhneKinder, modell::kG2Aus | modell::kG2OhneKinder);
    // Anbauten am Hauptmodell (Anbauten an Anbauten nennt nichtUebernommen)
    for (const modell::SzenenAnbau& a : s.anbauten) {
        if (a.eltern != s.name) { continue; }
        if (!modellAnbauen(a.modell, a.ziel, a.knochen, false)) { ohne.push_back(a.name + " " + a.modell); }
    }
    if (s.lod >= 0) { t.lodWahl = std::clamp(s.lod, 0, 10); }
    if (s.sichtfeld > 0.0F) { g_app->modelCam.fovY = std::clamp(s.sichtfeld, 10.0F, 120.0F); }
    if (s.boden >= 0) {
        t.an.boden = s.boden != 0;
        t.an.bodenZ = s.bodenZ;
    }
    t.anzeigeSchluessel.clear();
    g_app->modelDirty = true;
    g_app->settings.lastDir = directoryOf(pfad);
    std::snprintf(z, sizeof z, tr(Str::PqSzeneGelesen), pfad.c_str());
    t.meldung = z;
    if (!ohne.empty()) {
        std::string liste;
        for (const std::string& o : ohne) { liste += (liste.empty() ? "" : ", ") + o; }
        std::snprintf(z, sizeof z, tr(Str::PqSzeneOhne), liste.c_str());
        t.meldung += std::string("\n") + z;
    }
    diag::detail("Szene gelesen: " + pfad + ", " + std::to_string(s.anbauten.size()) + " Anbauten, nicht uebernommen " + std::to_string(ohne.size()));
    return true;
}

// --- P6: Zeilen der Pruefung fuer das Spiel -----------------------------------------------------------------------------------
void modellPrPruefZeilen() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const auto spiel = static_cast<modell::Zielspiel>(t.zielspiel);
    const modell::Grenzen& g = modell::grenzen(spiel);
    const std::size_t stufe = static_cast<std::size_t>(std::max(t.lodJetzt, 0));
    // Balken der gewaehlten Flaeche, ohne Auswahl die, die einer Grenze am naechsten ist (Entwurf: "Vertices je Flaeche (torso)")
    int wahl = !t.auswahl.empty() ? t.auswahl.front() : -1;
    if (wahl < 0) {
        float best = -1.0F;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].istTag() || stufe >= in.teile[i].stufen.size()) { continue; }
            const modell::FlaechenMass m = modell::mass(in.teile[i].stufen[stufe]);
            const float anteil = std::max({static_cast<float>(m.ecken) / static_cast<float>(g.eckenJeFlaeche),
                                           static_cast<float>(m.knochen) / static_cast<float>(g.knochenJeFlaeche),
                                           static_cast<float>(m.gewichteMax) / static_cast<float>(g.gewichteJeEcke)});
            if (anteil > best) {
                best = anteil;
                wahl = static_cast<int>(i);
            }
        }
    }
    if (wahl >= 0 && static_cast<std::size_t>(wahl) < in.teile.size() && stufe < in.teile[static_cast<std::size_t>(wahl)].stufen.size()) {
        const modell::Teil& x = in.teile[static_cast<std::size_t>(wahl)];
        const modell::FlaechenMass m = modell::mass(x.stufen[stufe]);
        const std::string von = " (" + x.name + ")";
        ui::grenzBalken((std::string(tr(Str::MfPrEcken)) + von).c_str(), m.ecken, g.eckenJeFlaeche, tr(Str::PqPrBalkenHint));
        ui::grenzBalken((std::string(tr(Str::MfPrKnochen)) + von).c_str(), m.knochen, g.knochenJeFlaeche, tr(Str::PqPrBalkenHint));
        ui::grenzBalken((std::string(tr(Str::MfPrGewichte)) + von).c_str(), m.gewichteMax, g.gewichteJeEcke, tr(Str::PqPrBalkenHint));
    }
    // die Zeilen des Entwurfs
    int flaechen = 0;
    int ohneSchatten = 0;
    std::vector<std::string> tags;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag()) {
            tags.push_back(klein(in.teile[i].name));
            continue;
        }
        ++flaechen;
        if (stufe < in.teile[i].stufen.size() && static_cast<int>(in.teile[i].stufen[stufe].ecken.size()) >= modell::schattenEckenGrenze(spiel)) { ++ohneSchatten; }
    }
    int innerhalb = 0;
    std::string draussen;
    for (const AnimEntry& e : g_app->animList) {
        if (g_app->anim.bildZahl() <= 0 || modell::animInnerhalb(e, g_app->anim.bildZahl())) {
            ++innerhalb;
        } else if (draussen.size() < 200) {
            draussen += (draussen.empty() ? "" : ", ") + e.name;
        }
    }
    const std::vector<std::string> gesucht = modell::gesuchteTags(spiel, in.glaPfad, g_app->modelPath);
    int da = 0;
    std::string fehlt;
    for (const std::string& n : gesucht) {
        if (std::find(tags.begin(), tags.end(), klein(n)) != tags.end()) {
            ++da;
        } else {
            fehlt += (fehlt.empty() ? "" : ", ") + n;
        }
    }
    int shaderOhne = 0;
    for (const modell::Befund& b : modellBefunde()) { shaderOhne += b.art == modell::BefundArt::ShaderFehlt ? 1 : 0; }
    char z[600];
    std::vector<ui::WertZeile> w;
    w.push_back({tr(Str::PqPrFlaechen), std::to_string(flaechen), ui::Ton::Normal, {}});
    const int alleSeq = static_cast<int>(g_app->animList.size());
    w.push_back({tr(Str::PqPrSequenzen), std::to_string(innerhalb) + " / " + std::to_string(alleSeq), innerhalb == alleSeq ? ui::Ton::Gut : ui::Ton::Fehler,
                 draussen});
    if (gesucht.empty()) {
        w.push_back({tr(Str::PqPrTags), tr(Str::PqPrTagsKeine), ui::Ton::Leise, {}});
    } else {
        std::snprintf(z, sizeof z, tr(Str::PqPrTagsHint), g.name, fehlt.empty() ? "-" : fehlt.c_str());
        w.push_back({tr(Str::PqPrTags), std::to_string(da) + " / " + std::to_string(gesucht.size()), da == static_cast<int>(gesucht.size()) ? ui::Ton::Gut : ui::Ton::Warnung, z});
    }
    w.push_back({tr(Str::PqPrShader), std::to_string(shaderOhne), shaderOhne == 0 ? ui::Ton::Gut : ui::Ton::Warnung, {}});
    std::snprintf(z, sizeof z, tr(Str::PqPrLodsWert), in.stufenZahl);
    w.push_back({tr(Str::PqPrLods), z, ui::Ton::Gut, {}});
    char schatten[400];
    std::snprintf(schatten, sizeof schatten, tr(Str::PqSchattenHint), modell::schattenEckenGrenze(spiel), g.name);
    w.push_back({tr(Str::PqPrSchatten), std::to_string(ohneSchatten), ui::Ton::Normal, schatten});
    ui::wertTabelle(w.data(), static_cast<int>(w.size()));
    g_pr.pruefWerte.clear();   // Selbsttest ap5
    for (const ui::WertZeile& x : w) { g_pr.pruefWerte += (g_pr.pruefWerte.empty() ? "" : "|") + x.name + "=" + x.wert; }
}

// R7: die Zahl der Ansichten je Bereich wie 3ds Max - beim Wechsel des Bereichs merkt sich der alte seine Wahl (Alt+W), der
// neue nimmt seine (ab Werk die Vorgabe des Entwurfs); wie Blender eine grosse Ansicht (Strg+Alt+Q teilt, der Bereich nicht).
// Jedes Bild (modellPrSchritt) und beim Umschalten des Bedienmodus (modellBedienungSetzen, FIX612 P3-4: dort VOR dem Loeschen
// von vierAnsichten - sonst merkte sich der Bereich fuer wie 3ds Max die schon geloeschte Wahl).
void modellPrAnsichtenAbgleich() {
    ModellTab& t = mt();
    ModellPruefen& p = g_pr;
    const int b = static_cast<int>(t.bereich);
    const int bed = g_app->settings.modellBedienung;
    if (p.letzterBereich >= 0 && (b != p.letzterBereich || bed != p.letzteBedienung)) {
        if (p.letzteBedienung == 0) { p.vierJeBereich[p.letzterBereich] = t.vierAnsichten ? 1 : 0; }
        if (bed == 0) {
            const int v = p.vierJeBereich[b];
            t.vierAnsichten = v < 0 ? vierVorgabe(t.bereich) : v != 0;
        } else if (bed != p.letzteBedienung) {
            t.vierAnsichten = false;
        }
        g_app->modelDirty = true;
    }
    p.letzterBereich = b;
    p.letzteBedienung = bed;
}

// --- Jedes Bild ------------------------------------------------------------------------------------------------------------
void modellPrSchritt() {
    ModellTab& t = mt();
    ModellPruefen& p = g_pr;
    modellPrAnsichtenAbgleich();
    // P10: Z wie Blender - die Schattierung der aktiven Ansicht zur Wahl (Blender VIEW3D_MT_shading_pie: Wireframe, Solid, Material)
    if (p.schattierungOeffnen) {
        ImGui::OpenPopup("###pqschattierung");
        p.schattierungOeffnen = false;
    }
    p.schattierungSichtbar = ImGui::BeginPopup("###pqschattierung");
    if (p.schattierungSichtbar) {
        ModellSchattierung& sch = t.ansichten[static_cast<std::size_t>(std::clamp(t.aktiv, 0, 3))].sch;
        cjkTextDisabled("%s", tr(Str::PqSchattierungMenue));
        const int art = sch.drahtgitter ? 0 : (sch.weiss ? 1 : 2);
        static const Str kArt[3] = {Str::MkBlDraht, Str::MkBlSolid, Str::MkBlMaterial};
        static const Str kHint[3] = {Str::MkDrahtHint, Str::MkBlSolidHint, Str::MkBlMaterialHint};
        for (int a = 0; a < 3; ++a) {
            if (ImGui::MenuItem(tr(kArt[a]), nullptr, art == a)) {
                sch.drahtgitter = a == 0;
                if (a != 0) { sch.weiss = a == 1; }
                g_app->modelDirty = true;
            }
            ImGui::SetItemTooltip("%s", tr(kHint[a]));
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

// --- Ansicht: rote Striche, Knochenbezuege, Hinweis -----------------------------------------------------------------------------
void modellPrUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    ModellPruefen& p = g_pr;
    const bool aktiv = n >= 0 && n == std::clamp(t.aktiv, 0, 3);
    if (n < 0) { n = std::clamp(t.aktiv, 0, 3); }   // das Foto der aktiven Ansicht
    if (aktiv) {
        p.lodHinweis = ImRect{};
        p.lodHinweisText.clear();
        p.schattenFlaechen = 0;
        p.bezugKnochen = 0;
    }
    if (g_app->model.empty() || p.overlaysAus[std::clamp(n, 0, 3)]) { return; }
    const float fs = ImGui::GetFontSize();
    const ImVec2 lo{a.lo[0], a.lo[1]};
    const ImVec2 hi{a.lo[0] + a.groesse[0], a.lo[1] + a.groesse[1]};
    dl->PushClipRect(lo, hi, true);
    const auto& h = modellHautOrte();
    const GlmModel& m = t.anzeige;
    const auto spiel = static_cast<modell::Zielspiel>(t.zielspiel);
    int schatten = 0;
    // P3a: nicht schattenwerfende Flaechen - rote Striche (ModView r_surface.cpp: glLineStipple(8, 0xAAAA), glColor3f(1,0,0))
    if (p.schatten) {
        const std::size_t grenze = static_cast<std::size_t>(modell::schattenEckenGrenze(spiel));
        for (std::size_t i = 0; i < m.surfaces.size() && i < h.size(); ++i) {
            const GlmSurface& s = m.surfaces[i];
            if (s.isTag() || s.verts.size() < grenze || !modellTeilGezeichnet(static_cast<int>(i))) { continue; }
            ++schatten;
            const bool hervor = p.hervorheben && std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) != t.auswahl.end();
            for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
                for (std::size_t e = 0; e < 3; ++e) {
                    ImVec2 pa;
                    ImVec2 pb;
                    if (!modellAufSchirm(a, h[i][s.indexes[k + e]].data(), pa) || !modellAufSchirm(a, h[i][s.indexes[k + (e + 1) % 3]].data(), pb)) { continue; }
                    strichLinie(dl, pa, pb, IM_COL32(255, 0, 0, 255), std::max(fs * 0.25F, 3.0F), hervor ? 4.0F : 1.0F);
                }
            }
        }
    }
    // P3a: Flaeche hervorheben + Knochenbezuege (ModView: gelbe Linien der Flaeche, glLineWidth 2; DrawTagOrigin je Knochen, auf den
    // sie sich bezieht, mit " ( Nummer )")
    int bezuege = 0;
    if (p.hervorheben && !t.auswahl.empty()) {
        const int f = t.auswahl.front();
        if (f >= 0 && static_cast<std::size_t>(f) < m.surfaces.size() && static_cast<std::size_t>(f) < h.size()) {
            const GlmSurface& s = m.surfaces[static_cast<std::size_t>(f)];
            const auto& o = h[static_cast<std::size_t>(f)];
            for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
                for (std::size_t e = 0; e < 3; ++e) {
                    ImVec2 pa;
                    ImVec2 pb;
                    if (modellAufSchirm(a, o[s.indexes[k + e]].data(), pa) && modellAufSchirm(a, o[s.indexes[k + (e + 1) % 3]].data(), pb)) {
                        dl->AddLine(pa, pb, IM_COL32(255, 255, 0, 255), 2.0F);
                    }
                }
            }
            for (const int b : knochenBezuege(s)) {
                if (b < 0 || static_cast<std::size_t>(b) >= g_app->modellWelt.size()) { continue; }
                const BoneMatrix& w = g_app->modellWelt[static_cast<std::size_t>(b)];
                const float ursprung[3] = {w.m[0][3], w.m[1][3], w.m[2][3]};
                ImVec2 sp;
                if (!modellAufSchirm(a, ursprung, sp)) { continue; }
                // die drei Achsen des Knochens kurz in RGB, daneben Name und Nummer (dimmer als ein gezielt hervorgehobener Knochen)
                const ImU32 farben[3] = {IM_COL32(230, 60, 60, 220), IM_COL32(60, 210, 60, 220), IM_COL32(70, 110, 250, 220)};
                for (int k = 0; k < 3; ++k) {
                    const float q[3] = {ursprung[0] + w.m[0][k] * 2.0F, ursprung[1] + w.m[1][k] * 2.0F, ursprung[2] + w.m[2][k] * 2.0F};
                    ImVec2 qp;
                    if (modellAufSchirm(a, q, qp)) { dl->AddLine(sp, qp, farben[k], 1.5F); }
                }
                const std::string name = (static_cast<std::size_t>(b) < g_app->anim.bones.size() ? g_app->anim.bones[static_cast<std::size_t>(b)].name : std::string("?")) +
                                         " ( " + std::to_string(b) + " )";
                // dunkel hinterlegt wie der Hinweis unten rechts - auf den gelben Linien der Flaeche sonst kaum lesbar (Foto ap5_f_en)
                const ImVec2 textOrt{sp.x + 4.0F, sp.y - fs * 0.5F};
                const ImVec2 textG = ImGui::CalcTextSize(name.c_str());
                dl->AddRectFilled(ImVec2{textOrt.x - 2.0F, textOrt.y}, ImVec2{textOrt.x + textG.x + 2.0F, textOrt.y + textG.y}, IM_COL32(0, 0, 0, 150), 2.0F);
                dl->AddText(textOrt, IM_COL32(255, 255, 160, 235), name.c_str());
                ++bezuege;
            }
        }
    }
    // R10: unten rechts in Ansehen/Pruefen "LOD 1 - 3412 Dreiecke - wie im Spiel gezeichnet (Ghoul2 aus dem Mission-Tab)"
    if (aktiv && t.bereich == ModellBereich::Ansehen) {
        const int dreiecke = dreieckeJetzt(true);
        char z[300];
        std::snprintf(z, sizeof z, trN(Str::PqLodHinweis, t.lodJetzt + 1, dreiecke), t.lodJetzt + 1, dreiecke);
        const float rand = std::floor(fs * 0.4F);
        // passt der Satz nicht in eine Zeile (kleine Ansicht), bleiben LOD und Dreiecke (bis zum zweiten " - ") - er laege sonst ueber der Figur
        if (ImGui::CalcTextSize(z).x > a.groesse[0] - rand * 2.0F) {
            const char* kurz = std::strstr(z, " \xc2\xb7 ");
            if (kurz != nullptr) { kurz = std::strstr(kurz + 1, " \xc2\xb7 "); }
            if (kurz != nullptr) { z[kurz - z] = '\0'; }
        }
        const ImVec2 g = cjkTextSize(z, nullptr, false, a.groesse[0] - rand * 2.0F);
        const ImVec2 o{hi.x - rand - g.x, hi.y - rand - g.y};
        dl->AddRectFilled(ImVec2{o.x - 3.0F, o.y - 1.0F}, ImVec2{o.x + g.x + 3.0F, o.y + g.y + 1.0F}, IM_COL32(0, 0, 0, 110), 3.0F);
        cjkAddText(dl, nullptr, 0.0F, o, IM_COL32(205, 205, 205, 235), z, nullptr, a.groesse[0] - rand * 2.0F);
        p.lodHinweis = ImRect{o, ImVec2{o.x + g.x, o.y + g.y}};
        p.lodHinweisText = z;
    }
    dl->PopClipRect();
    if (aktiv) {
        p.schattenFlaechen = schatten;
        p.bezugKnochen = bezuege;
    }
}

int modellPrSchattenZahl() {
    const ModellTab& t = mt();
    const std::size_t grenze = static_cast<std::size_t>(modell::schattenEckenGrenze(static_cast<modell::Zielspiel>(t.zielspiel)));
    int n = 0;
    for (std::size_t i = 0; i < t.anzeige.surfaces.size(); ++i) {
        const GlmSurface& s = t.anzeige.surfaces[i];
        if (!s.isTag() && s.verts.size() >= grenze && modellTeilGezeichnet(static_cast<int>(i))) { ++n; }
    }
    return n;
}

std::string modellPrStand(int n) {
    char z[48];
    std::snprintf(z, sizeof z, "|pq%d%d%d", g_pr.alpha ? 1 : 0, g_pr.filter ? 1 : 0, g_pr.overlaysAus[std::clamp(n, 0, 3)] ? 1 : 0);
    return z;
}

// --- P3a: Knochengewicht-Schwelle ------------------------------------------------------------------------------------------
// ModView r_glm.cpp (RB_SurfaceGhoul): ein Gewicht zaehlt nur, wenn fBoneWeight * 100 > fBoneWeightThreshholdPercent; die
// uebrigen fallen weg, ohne dass die anderen mehr bekommen (tempVert summiert nur die genutzten).
void modellPrSchwelleAnwenden(GlmModel& m) {
    g_pr.schwelleEcken = 0;
    if (!g_pr.schwelleAn) { return; }
    for (GlmSurface& s : m.surfaces) {
        for (GlmVertex& v : s.verts) {
            bool weg = false;
            for (int k = 0; k < 4; ++k) {
                if (v.weights[k] > 0.0F && !(v.weights[k] * 100.0F > g_pr.schwelle * 100.0F)) {
                    v.weights[k] = 0.0F;
                    weg = true;
                }
            }
            g_pr.schwelleEcken += weg ? 1 : 0;
        }
    }
}

std::string modellPrSchwelleSchluessel() {
    char z[48];
    std::snprintf(z, sizeof z, "|s%d:%.4f", g_pr.schwelleAn ? 1 : 0, static_cast<double>(g_pr.schwelle));
    return z;
}

void modellPrSchwelleZeile() {
    ImGui::Indent();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0F);
    if (ImGui::DragFloat(tr(Str::PqSchwelleWert), &g_pr.schwelle, 0.005F, 0.0F, 1.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp)) { g_app->modelDirty = true; }
    ImGui::SetItemTooltip("%s", tr(Str::PqSchwelleWertHint));
    ImGui::Unindent();
}

// --- "Alle Aktionen suchen" -----------------------------------------------------------------------------------------------
bool modellPrAktionenSuche(const std::string& such) {
    struct Eintrag {
        Str text;
        Str hinweis;
        const char* taste;
        void (*tun)();
    };
    static const Eintrag kListe[] = {
        {Str::PqFlaechenliste, Str::ModelSurfaceDump, "", [] {
             modellFlaechenProtokoll();
             ++g_app->modellFlaechenListen;
         }},
        {Str::PqOverlays, Str::PqOverlaysHint, "Shift+Alt+Z", [] {
             bool& aus = g_pr.overlaysAus[std::clamp(mt().aktiv, 0, 3)];
             aus = !aus;
         }},
        {Str::PqSchattierungMenue, Str::MkShadingHintBl, "Z", [] { g_pr.schattierungOeffnen = true; }},
        {Str::PqRBoden, Str::PqBodenHint, "", [] {
             mt().bereich = ModellBereich::Ansehen;
             ui::rolloutZeigen(tr(Str::PqRBoden));
         }},
        {Str::PqRAuswahl, Str::PqAuswahlKeine, "", [] {
             mt().bereich = ModellBereich::Ansehen;
             ui::rolloutZeigen(tr(Str::PqRAuswahl));
         }},
        {Str::PqRSzene, Str::PqSzeneSpeichernHint, "", [] {
             mt().bereich = ModellBereich::Ansehen;
             ui::rolloutZeigen(tr(Str::PqRSzene));
         }},
        {Str::PqImport, Str::PqImportHint, "", [] {
             mt().bereich = ModellBereich::Ansehen;
             ui::rolloutZeigen(tr(Str::MbRModell));
         }},
    };
    for (const Eintrag& e : kListe) {
        std::string text = tr(e.text);
        // die Taste nur im Bedienmodus, in dem sie gilt (Z und Umschalt+Alt+Z wie Blender)
        const bool blTaste = e.taste[0] != '\0';
        if (blTaste && modellBlender()) { text += std::string(" (") + e.taste + ")"; }
        if (!such.empty() && klein(text).find(such) == std::string::npos) { continue; }
        if (blTaste && !modellBlender() && e.text == Str::PqSchattierungMenue) { continue; }   // wie 3ds Max: das Menue [Standard]
        if (modellAktionZeile(text.c_str(), static_cast<int>(ModellBereich::Ansehen))) {
            e.tun();
            ++mt().tastenZaehler;
            mt().letzteTaste = tr(e.text);
            g_app->modelDirty = true;
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(e.hinweis));
    }
    return false;
}


// AP11 (Modelle in Tabs): vier Ansichten je Bereich (Alt+W wie 3ds Max) und die Overlays je Ansicht gehoeren zu den Ansichten des
// Modells. Alpha, Filter, Schwelle und die anderen Anzeige-Schalter gelten fuer alle Reiter ("gilt ueberall").
void modellPruefenTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        int vierJeBereich[kModellBereiche] = {-1, -1, -1, -1, -1, -1};
        int letzterBereich = -1;
        int letzteBedienung = -1;
        bool overlaysAus[4]{};
    };
    if (!f) { f = std::make_unique<Fach>(); }
    Fach& x = static_cast<Fach&>(*f);
    std::swap(g_pr.vierJeBereich, x.vierJeBereich);
    std::swap(g_pr.letzterBereich, x.letzterBereich);
    std::swap(g_pr.letzteBedienung, x.letzteBedienung);
    std::swap(g_pr.overlaysAus, x.overlaysAus);
    g_pr.schattierungOeffnen = false;
}

}  // namespace bhed::gui
