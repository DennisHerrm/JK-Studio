// app_modelltab.cpp - der neue Model-Tab (Modell-Block 1a-1): Dokument, Anzeige,
// Bereiche, Szenen-Explorer, Befehlsfeld, Tasten wie 3ds Max
//
// Die Ansichten selbst (Zeichnen, Ueberlagerung, Maus, Gizmo, Foto) stehen in
// gui/app_modelltab_ansicht.cpp. Vorbild fuer den Bereich Ansehen/Pruefen ist
// ModView (Raven, JACoders/OpenJK-tools ModView/, GPL v2) - uebernommen sind
// Gliederung und Funktionen, gezeichnet wird wie im Spiel ueber den
// Ghoul2-Weg des Mission-Tabs; die Regeln dafuer stehen in bhed/modell/g2.h.
#include "app_internal.h"
#include "ui_menue.h"   // KLEIN2: ui::menue - Menues bleiben im Fenster
#include "app_modelltab_knochen.h"   // AP13K: Alt+X / Alt+Z, Suche
#include "textumbruch.h"
#include "gpumap.h"
#include "fenster_dock.h"   // AP4: Fensteranordnung je Bedienmodus

#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>
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
    // mb1a2: Strg+Tab ist in Blender das Modus-Menue - ImGui nimmt es sonst fuer den Fensterwechsel (NavUpdateWindowing,
    // ConfigNavWindowingKeyNext); nur solange der Model-Tab wie Blender den Fokus hat.
    ImGuiContext& g = *GImGui;
    const bool blender = g_app->leftMode == 2 && t.fokus && g_app->settings.modellBedienung == 1;
    g.ConfigNavWindowingKeyNext = blender ? 0 : (g.IO.ConfigMacOSXBehaviors ? (ImGuiMod_Super | ImGuiKey_Tab) : (ImGuiMod_Ctrl | ImGuiKey_Tab));
    g.ConfigNavWindowingKeyPrev = blender ? 0 : (g.IO.ConfigMacOSXBehaviors ? (ImGuiMod_Super | ImGuiMod_Shift | ImGuiKey_Tab) : (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab));
}

namespace {

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

// FIX612B P2-9: die Auswahl von Ecken (ModellTab::eckenWahl) und die des Arbeitsnetzes (ModellMesh::auswahl) sind Nummern in
// den Netzen EINER Stufe. Wechselt die gezeichnete Stufe (ab Werk automatisch nach dem Abstand der Kamera), gilt die Auswahl
// der neuen Stufe: die der alten wird gemerkt und steht wieder da, wenn die Ansicht zu ihr zurueckkehrt - solange das
// Modell dasselbe ist, die Netze dieser Stufe dieselbe Gestalt haben (Ecken, Kanten, Punkte je Flaeche: Gewichte malen aendert
// sie nicht, Modellieren schon) und die Ebene der Auswahl dieselbe ist. Vorher blieben die Nummern stehen und meinten in der anderen
// Stufe andere Ecken: G/R/S, Gizmo, "Setzen", Bereinigen, UV- und Modellier-Befehle aenderten die falschen.
struct StufenWahl {
    int ladeStand = -1;
    std::vector<std::size_t> gestalt;   // netzGestalt der Stufe beim Merken
    AuswahlArt art = AuswahlArt::Objekt;
    std::vector<modell::EckOrt> ecken;
    modell::wof::PAuswahl mesh;
    std::vector<int> punktFolge;
};
// Ecken, Kanten (3 je Dreieck) und Punkte jeder Flaeche in der Stufe - die Nummern einer Auswahl gelten, solange das gleich ist
std::vector<std::size_t> netzGestalt(const modell::Inhalt& in, int stufe) {
    std::vector<std::size_t> g;
    g.reserve(in.teile.size() * 3U);
    for (const modell::Teil& teil : in.teile) {
        const bool da = stufe >= 0 && static_cast<std::size_t>(stufe) < teil.stufen.size();
        const modell::Netz* n = da ? &teil.stufen[static_cast<std::size_t>(stufe)] : nullptr;
        g.push_back(n != nullptr ? n->ecken.size() : 0U);
        g.push_back(n != nullptr ? n->kanten.size() : 0U);
        g.push_back(n != nullptr ? static_cast<std::size_t>(n->punktZahl) : 0U);
    }
    return g;
}

std::map<int, StufenWahl>& stufenWahl() {
    static std::map<int, StufenWahl> merk;
    return merk;
}

void stufeGewechselt(int alt, int neu) {
    ModellTab& t = mt();
    std::map<int, StufenWahl>& merk = stufenWahl();
    const modell::Inhalt& in = t.dok.inhalt();
    for (auto it = merk.begin(); it != merk.end();) {   // was zu einem anderen Modell, einer anderen Gestalt oder Ebene gehoert
        const StufenWahl& w = it->second;
        it = (w.ladeStand != t.dok.ladeStand() || w.art != t.auswahlArt || w.gestalt != netzGestalt(in, it->first)) ? merk.erase(it) : std::next(it);
    }
    StufenWahl jetzt;
    jetzt.ladeStand = t.dok.ladeStand();
    jetzt.gestalt = netzGestalt(in, alt);
    jetzt.art = t.auswahlArt;
    jetzt.ecken = std::move(t.eckenWahl);
    jetzt.mesh = std::move(t.mesh.auswahl);
    jetzt.punktFolge = std::move(t.mesh.punktFolge);
    t.eckenWahl.clear();
    t.mesh.auswahl = modell::wof::PAuswahl{};
    t.mesh.punktFolge.clear();
    t.mesh.cutHatVon = false;   // ein begonnener Schnitt und ein Ziel-Verschweissen zeigen auf Punkte der alten Stufe
    t.mesh.zielQuelle = -1;
    const auto it = merk.find(neu);
    if (it != merk.end()) {
        t.eckenWahl = std::move(it->second.ecken);
        t.mesh.auswahl = std::move(it->second.mesh);
        t.mesh.punktFolge = std::move(it->second.punktFolge);
        merk.erase(it);
    }
    if (!jetzt.ecken.empty() || !jetzt.mesh.leer()) { merk[alt] = std::move(jetzt); }
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
    modellEnt3aVorLaden(bytes, in);   // ENTSCHEID3A (B12): der Stapel aus der Begleitdatei neben der .glm (Fehler: ohne, mit Meldung)
    in.kinoBelegung = t.dok.inhalt().kinoBelegung;   // REST N10: die Belegung der Ziel-GLA bleibt
    t.dok.laden(std::move(in));
    modellEnt3aNachLaden();   // ENTSCHEID3A (B11, B12): der Inhalt wie geladen, Meldung zur Begleitdatei
    t.dokModell = g_app->modelPath;
    t.gespeichertStand = -1;   // pfix H3: ein neues Modell ist ungeaendert
    t.g2 = modell::G2Zustand{};
    t.verstecktTeil.clear();   // mb1a2
    t.verstecktKnochen.clear();
    ++t.verstecktStand;
    t.hautAus.clear();
    t.anzeigeSchluessel.clear();
    t.auswahl.clear();
    t.knochenWahl = -1;
    t.tagWahl = -1;
    t.anbauten.clear();
    t.sekundaer = -1;
    t.zielspiel = zielspielVorgabe();
    t.bereich = ModellBereich::Ansehen;   // AP5 P11: Startbereich beim Oeffnen einer .glm (Entwurf, wie ModView)
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
    // pfix H2: eigene neue oder zurueckgenommene Teile (Detach/Grundkoerper in einen neuen Teil, Rueckgaengig davon) zieht
    // das gezeichnete Modell nach - das Dokument bleibt samt Rueckgaengig (vorher baute es die .glm aus der Datei neu)
    if (t.dokModell == g_app->modelPath && t.dok.inhalt().teile.size() != g_app->model.surfaces.size()) { modellFlaechenAbgleichen(); }
    // Ein Modell, das nicht ueber modellDokumentLaden kam (oder eines mit
    // anderer Flaechenzahl): das Dokument aus dem gelesenen Modell.
    if (t.dokModell != g_app->modelPath || t.dok.inhalt().teile.size() != g_app->model.surfaces.size()) {
        modell::Inhalt in;
        modell::ausGlmModell(g_app->model, in);
        in.kinoBelegung = t.dok.inhalt().kinoBelegung;   // REST N10: die Belegung der Ziel-GLA bleibt
    t.dok.laden(std::move(in));
        modellEnt3aNachLaden();   // ENTSCHEID3A (B11)
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
        modell::hautAnwenden(in, t.g2, modell::hautAusModell(in, g_app->model));
        t.hautAus = haut;
    }
    int lod = std::clamp(t.lodWahl > 0 ? t.lodWahl - 1 : automatischeStufe(in), 0, std::max(in.stufenZahl - 1, 0));
    // FIX612B P2-3 / P2-9: solange ein Strich, ein Zug oder ein Kaestchen offen ist, bleibt die gezeichnete Stufe stehen (das
    // Rad zoomt auch bei gedrueckter linker Taste, und "automatisch" folgt dem Abstand). Was offen ist, haelt Nummern der
    // Stufe, in der es begann (Schweissnetz und Partner des Spiegelns, die Mallisten des Verlaufs, der Stand vom Anfassen,
    // das Arbeitsnetz des Kaestchens); die neue Stufe gilt nach dem Loslassen.
    const bool gleichesModell = !t.anzeigeSchluessel.empty() && t.anzeigeSchluessel.compare(0, g_app->modelPath.size() + 1U, g_app->modelPath + "|") == 0;
    if (lod != t.lodJetzt && gleichesModell && t.lodJetzt >= 0 && t.lodJetzt < std::max(in.stufenZahl, 1) && modellStufeGehalten()) { lod = t.lodJetzt; }
    // "Kappen" (_off-Flaechen) der Leiste: G2_SetSurfaceOnOff(..., 0) fuer jede
    // Flaeche mit OFF in der Datei, die der Anwender nicht selbst geschaltet hat.
    const modell::G2Zustand z = g_app->showCaps ? modell::mitKappen(in, t.g2) : t.g2;
    // Stufe 3: der Netz-Stand des Dokuments (Rig-Schluessel aendern das Netz nicht und bauen nichts neu)
    std::string schluessel = g_app->modelPath + "|" + std::to_string(in.netzStand) + "|" + std::to_string(lod) + "|" +
                             std::to_string(z.wurzel) + "|";
    for (const modell::FlaechenSchalter& s : z.slist) { schluessel += std::to_string(s.flaeche) + ":" + std::to_string(s.offFlags) + ","; }
    for (const GlmSurface& s : g_app->model.surfaces) { schluessel += s.texture + ";"; }
    schluessel += "|v" + std::to_string(t.verstecktStand);   // mb1a2: das Auge
    schluessel += modellPrSchwelleSchluessel();   // AP5 P3a: Knochengewicht-Schwelle
    if (schluessel == t.anzeigeSchluessel) { return; }
    GlmModel neu = modell::zuGlm(in, lod);
    modellPrSchwelleAnwenden(neu);   // AP5 P3a (ModView r_glm.cpp: kleine Gewichte fallen weg, ohne Umverteilen)
    t.gezeichnet = modell::gezeichnet(in, z);
    for (std::size_t i = 0; i < neu.surfaces.size() && i < g_app->model.surfaces.size(); ++i) {
        neu.surfaces[i].texture = g_app->model.surfaces[i].texture;
        // Gezeichnet wird nach den Ghoul2-Regeln: alles andere gilt dem
        // Zeichner als von der Haut abgeschaltet (er zeichnet mit kappen).
        // mb1a2: ein im Explorer ausgeblendetes Teil zeichnet die Ansicht nicht (nur die Ansicht - gezeichnet bleibt die Regel)
        neu.surfaces[i].skinnedOff = t.gezeichnet[i] == 0 || modellTeilVersteckt(static_cast<int>(i));
        // p_render: bleibt eine Flaeche nach den Ghoul2-Regeln an, obwohl die Haut sie mit "*off" belegt (Movie Duels imperial:
        // zwei Flaechen "hips", G2_IsSurfaceLegal findet die erste, schon OFF), sucht RenderSurfaces ihren Shader in der Haut
        // (tr_ghoul2.cpp:2276-2288) - "*off": R_FindShader findet kein Bild und gibt einen Shader ohne Stufe zurueck
        // (tr_shader.cpp:3502-3509, MD9 :3457-3466), RB_StageIteratorGeneric zeichnet nichts. Das Spiel zeigt sie nicht.
        if (g_app->model.surfaces[i].skinnedOff) { neu.surfaces[i].skinnedOff = true; }
    }
    t.anzeige = std::move(neu);
    const int alteStufe = t.lodJetzt;
    t.lodJetzt = lod;
    if (alteStufe != lod && gleichesModell) { stufeGewechselt(alteStufe, lod); }   // FIX612B P2-9
    t.anzeigeSchluessel = schluessel;
    ++t.neuGebaut;
    g_app->modelDirty = true;
}

void modellFlaecheSchalten(int flaeche, std::uint32_t offFlags) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size()) { return; }
    (void)modell::setzeFlaecheAnAus(in, t.g2, modell::spielName(in, flaeche), offFlags);
}

void modellWurzelSetzen(int flaeche) {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size()) { return; }
    (void)modell::setzeWurzel(in, t.g2, modell::spielName(in, flaeche));
}

// pfix H1: ein offenes Werkzeugkaestchen, G/R/S, ein Regler oder Zug haelt einen eigenen Schritt oben auf dem Stapel - Strg+Z
// (und Strg+Y) brechen zuerst ihn ab (wie Esc), statt ihn auf Wiederholen zu legen; sonst naehme das spaetere Esc einen
// fremden Schritt weg
bool modellOffenesAbbrechen() {
    ModellTab& t = mt();
    bool war = false;
    // FIX612C (P3-10): auch der Gang-Regler der Presets - der Rest des Zugs legt danach einen eigenen Schritt an
    t.rt.presetZug = false;
    // pfix M7: ein gezogener Regler (Physik, IK, Retarget) hat seinen Schritt schon - Strg+Z nimmt ihn, das Ziehen endet
    t.physikZug = false;
    t.ikZug = false;
    t.rt.reglerZug = false;
    if (t.mesh.kaestchenOffen) {
        modellMeshAbbrechen();
        war = true;
    }
    if (t.modal.art != 0) {
        modellModalEnde(false);
        war = true;
    }
    if (t.kvRegler != 0 || t.kvModal != 0) {
        modellKvOffenesAbbrechen();
        war = true;
    }
    if (t.uv.zug != 0) {
        modellUvZugAbbrechen();
        war = true;
    }
    if (t.rigZieht || t.rigModal != 0) {
        if (t.rigZieht) { (void)t.dok.abbrechen(t.rigSchritt); }
        t.rigZieht = false;
        t.rigModal = 0;
        war = true;
    }
    if (war) { g_app->modelDirty = true; }
    return war;
}

bool modellRueckgaengig() {
    if (modellOffenesAbbrechen()) { return true; }
    if (modellAnsichtOffenesAbbrechen(true)) { return true; }   // FIX612B P2-10: Strich, Verlauf, Gizmo-Zug: beenden und zuruecknehmen
    // Modell-Block 1c: im Bereich Painting gilt Strg+Z zuerst dem Bild (Rueckgaengig in Kacheln)
    // P4: im Bereich Painting nie auf das Modelldokument durchfallen (das Netz aenderte sich sonst unter den Leinwaenden)
    if (mt().bereich == ModellBereich::Painting) { return modellMalenRueckgaengig(); }
    if (!mt().dok.rueckgaengig()) { return false; }
    g_app->modelDirty = true;
    return true;
}

bool modellWiederholen() {
    if (modellOffenesAbbrechen()) { return true; }
    if (modellAnsichtOffenesAbbrechen(false)) { return true; }   // FIX612B P2-10: Strich, Verlauf, Gizmo-Zug nur beenden
    if (mt().bereich == ModellBereich::Painting) { return modellMalenWiederholen(); }   // Modell-Block 1c, P4 wie Rueckgaengig
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

// FIX612B P2-2: der Inhalt, wie die Ansicht ihn zeigt und wie die Datei ihn traegt - mit offenen Mallisten eine Kopie, in der
// sie uebernommen sind (gemerkt je Stand des Dokuments), sonst das Dokument selbst.
const modell::Inhalt& modellInhaltGezeigt() {
    const ModellTab& t = mt();
    // AP13S: der Inhalt mit den Eintraegen des Modifier-Stapels, die in die Datei gehen (ohne: das Dokument selbst); dessen
    // Flaechen mit Stapel tragen keine Mallisten mehr (stapelNetz uebernimmt sie)
    const modell::Inhalt& in = modellStapelDateiInhalt();
    if (!modell::malt(in)) { return in; }
    struct Merk {
        int ladeStand = -1;
        int stand = -1;
        modell::Inhalt inhalt;
    };
    static Merk m;
    if (m.ladeStand != t.dok.ladeStand() || m.stand != t.dok.stand()) {
        m.inhalt = modell::wieGezeigt(in);
        m.ladeStand = t.dok.ladeStand();
        m.stand = t.dok.stand();
    }
    return m.inhalt;
}

// FIX612B P2-3 / P2-9: haelt etwas Offenes die gezeichnete Stufe? (Strich und Verlauf im Bereich Weights, Zug am Gizmo,
// G/R/S, Werkzeugkaestchen, Zug im UV-Editor)
bool modellStufeGehalten() {
    const ModellTab& t = mt();
    return t.malStrich || modellGewichte().verlaufZieht || (t.griff != 0 && !t.ziehVorher.empty()) || t.modal.art != 0 || t.mesh.kaestchenOffen || t.uv.zug != 0;
}

std::vector<modell::Befund> modellBefunde() {
    const ModellTab& t = mt();
    // AP13S: geprueft wird, was die Datei bekommt - mit den Eintraegen des Modifier-Stapels, die in die Datei gehen.
    // FIX612B P2-2: und mit offenen Mallisten das, was das Spiel aus ihnen bekommt (vorher die Ecken von vor dem Malen: die
    // Frage vor dem Speichern kannte die gemalten Gewichte nicht)
    const modell::Inhalt& in = modellInhaltGezeigt();
    std::vector<char> hatBild(in.teile.size(), 0);
    for (std::size_t i = 0; i < hatBild.size(); ++i) {
        const bool bild = i < g_app->modelTextures.bySurface.size() && !g_app->modelTextures.bySurface[i].rgba.empty() &&
                          !g_app->modelTextures.bySurface[i].ohneStufe;   // REST4 Punkt 3: Hautshader ohne Bild zaehlt als fehlend
        const bool stufen = i < g_app->modelTextures.stufen.size() && !g_app->modelTextures.stufen[i].empty();
        hatBild[i] = (bild || stufen) ? 1 : 0;
    }
    // Ohne gefundene Bilder (Texturen aus) keine Aussage ueber fehlende.
    if (!g_app->showModelTextures || g_app->modelTextures.found == 0) { hatBild.clear(); }
    // Eine Ghoul2-Instanz: die Figur samt Anbauten (ein CGhoul2Info_v).
    return modell::pruefeModell(in, static_cast<modell::Zielspiel>(t.zielspiel), g_app->animList, g_app->anim.bildZahl(), 1, hatBild,
                                t.gezeichnet);
}

// --- Bereiche: seit AP3 als Reihe in der Kopfzeile des Tabs (gui/app_modelltab_feld.cpp, drawModellBereiche) ---

// --- Szenen-Explorer: seit AP2 in gui/app_modelltab_explorer.cpp (ein Baum mit Sequenzen, Ebenen, Einfrieren) ---

// --- Befehlsfeld ------------------------------------------------------------------------------
namespace {

// TEST1411 C (shank 14.11.: "kann er auch GLAS laden von einer PK3?"): "Load .gla..." und "Other GLA..." oeffnen dieselbe Liste; sie steht
// ausserhalb des Befehlsfelds (drawModellGlaListe), damit Werkzeugknopf und Menue sie aus jedem Bereich oeffnen
bool g_glaOeffnen = false;

void glaFenster() {
    if (g_glaOeffnen) {
        ImGui::OpenPopup("###mtgla");
        g_glaOeffnen = false;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 24.0F, fs * 22.0F}, ImGuiCond_Appearing);
    if (!ui::modalBegin((std::string(tr(Str::MbAndereGla)) + "###mtgla").c_str(), nullptr)) { return; }
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
    if (ImGui::Button(tr(Str::MeDateiOeffnen))) {   // eine lose .gla (Windows-Dialog), wie "Archiv oeffnen..." bei Load mission
        ImGui::CloseCurrentPopup();
        openSkeletonFile();
    }
    ImGui::SetItemTooltip("%s", tr(Str::ModelLoadGlaHint));
    ImGui::EndPopup();
}

}  // namespace

void modellGlaListeOeffnen() {
    g_app->leftMode = 2;
    g_glaOeffnen = true;
}

void drawModellGlaListe() { glaFenster(); }

namespace {

// AP3: die Gruppen von Ansehen/Pruefen in der Form des Entwurfs (Baustein gui/ui_rollout.h, shank 04.11. Bild 1): Kurzinfo in der
// Ueberschrift, Werte als Tabelle, Knoepfe im Raster mit Symbol und Taste, Wahl als Segment, Tipp als Kasten. Der Inhalt ist der
// von rc611; jede Gruppe nennt ihren Reiter im Befehlsfeld (wie 3ds Max) bzw. im Eigenschaften-Editor (wie Blender) - im Reiter,
// den der Bereich vorgibt (Anzeige bzw. Objekt), stehen wie im Entwurf alle untereinander.
void rolloutModell() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    int flaechen = 0;
    int dreiecke = 0;
    int ecken = 0;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || static_cast<std::size_t>(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
        ++flaechen;
        dreiecke += in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].dreiecke();
        ecken += static_cast<int>(in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].ecken.size());
    }
    char info[96];
    std::snprintf(info, sizeof info, trN(Str::MfInfoDreiecke, dreiecke), dreiecke);
    if (!ui::rolloutBeginn(tr(Str::MbRModell), {.kurzinfo = info, .reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Daten)})) { return; }
    std::vector<ui::WertZeile> werte;
    // AP11: Ordner und Datei wie der Reiter und der Entwurf ("kyle/model.glm") - mit mehreren Modellen heissen fast alle model.glm
    werte.push_back({tr(Str::MbDatei), modellReiterTitel(g_app->modelPath, in.name), ui::Ton::Normal, g_app->modelPath});
    werte.push_back({tr(Str::MbGla), g_app->anim.name.empty() ? in.glaPfad : g_app->anim.name, ui::Ton::Normal, {}});
    werte.push_back({tr(Str::MbXArtFlaechen), std::to_string(flaechen), ui::Ton::Normal, {}});
    werte.push_back({tr(Str::MfDreiecke), std::to_string(dreiecke), ui::Ton::Normal, {}});
    werte.push_back({tr(Str::MfEcken), std::to_string(ecken), ui::Ton::Normal, {}});
    werte.push_back({tr(Str::MbXArtKnochen), std::to_string(in.knochenZahl), ui::Ton::Normal, {}});
    ui::wertTabelle(werte.data(), static_cast<int>(werte.size()));
    if (!t.auswahl.empty()) {
        const int n = static_cast<int>(t.auswahl.size());
        char gewaehlt[96];
        std::snprintf(gewaehlt, sizeof gewaehlt, trN(Str::MbAuswahl, n), n);
        ui::absatz(gewaehlt);
    }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MbAndereGla), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MbAndereGlaHint)})) { modellGlaListeOeffnen(); }
    // AP5 P1: Alt+R (ModView "Refresh textures") in beiden Bedienmodi wie im Entwurf
    if (ui::werkzeugKnopf(tr(Str::MbTexNeu), {.icon = UiIcon::Neuladen, .taste = "Alt+R", .tasteHalten = true})) { modellTexturenNeuLaden(); }
    // AP5 P1: Importieren = das heutige Oeffnen (.glm, .md3 von der Platte) und jedes Modell des Spiels aus den .pk3
    if (ui::werkzeugKnopf(tr(Str::PqImport), {.icon = UiIcon::Oeffnen, .hinweis = tr(Str::PqImportHint)})) { ImGui::OpenPopup("###pqimport"); }
    // AP11 (P1): wie im Entwurf neben "Importieren ..." - ein weiteres Modell in einem eigenen Reiter (Strg+O)
    if (ui::werkzeugKnopf(tr(Str::MxNeuerTab), {.icon = UiIcon::Plus, .taste = "Ctrl+O", .hinweis = tr(Str::MxNeuerTabHint), .tasteHalten = true})) {
        ImGui::OpenPopup("###mxneutab");
    }
    ui::rasterEnde();
    if (ui::popup("###mxneutab")) {
        modellNeuerReiterMenue();
        ImGui::EndPopup();
    }
    if (ui::popup("###pqimport")) {
        if (ImGui::MenuItem(tr(Str::PqImportDatei))) { openModelFile(); }
        ImGui::SetItemTooltip("%s", tr(Str::OpenGlmHint));
        if (ImGui::MenuItem(tr(Str::PqImportPk3))) {
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
            g_app->pk3Kind = 2;
            refreshPk3List();
            g_app->pk3Open = true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::OpenGlmPk3Hint));
        ImGui::EndPopup();
    }
    ui::rolloutEnde();
}

void rolloutLod() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    char info[48];
    std::snprintf(info, sizeof info, tr(Str::MfInfoLod), t.lodJetzt + 1);
    if (!ui::rolloutBeginn(tr(Str::MbRLod), {.kurzinfo = info, .reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Daten)})) { return; }
    // "Auto" wie im Spiel (G2_ComputeLOD) als Haken, darunter LOD 1-10 wie ModView (F1-F10 dort); nur die vorhandenen sind waehlbar.
    bool automatisch = t.lodWahl == 0;
    // FIX612B (Entscheidungen A14, A15): der Hinweis nennt, dass Malen, Gewichtsbefehle und Auswahl der gezeigten Stufe gelten
    const std::string lodHinweis = std::string(tr(Str::MbLodAutoHint)) + " " + tr(Str::FxLodGilt);
    if (ui::schalter(tr(Str::MbLodAuto), &automatisch, lodHinweis.c_str())) { t.lodWahl = automatisch ? 0 : t.lodJetzt + 1; }
    static const char* const kStufen[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    // mit "Auto" ist keine Stufe gewaehlt (die gezeichnete steht in der Kurzinfo und in der Zeile darunter); ein Klick stellt sie fest.
    // AP5 P2: alle zehn waehlbar wie ModView und r_lodbias im Spiel - G2_ComputeLOD: "lodBias >= numLods -> numLods - 1"
    // (tr_ghoul2.cpp:868-872 Movie Duels Update 9, gleich in OpenJK rd-vanilla); modellAnzeigeAktualisieren klemmt genauso.
    int stufe = t.lodWahl > 0 ? t.lodWahl - 1 : -1;
    const char* hinweise[10];
    for (const char*& h : hinweise) { h = tr(Str::PqLodStufeHint); }
    if (ui::segment("##mtlod", kStufen, 10, &stufe, 10, -1, hinweise)) { t.lodWahl = stufe + 1; }
    int dreiecke = 0;
    for (const modell::Teil& x : in.teile) {
        if (!x.istTag() && static_cast<std::size_t>(t.lodJetzt) < x.stufen.size()) { dreiecke += x.stufen[static_cast<std::size_t>(t.lodJetzt)].dreiecke(); }
    }
    char text[200];
    std::snprintf(text, sizeof text, trN(Str::PqLodZahl, in.stufenZahl, dreiecke), in.stufenZahl, dreiecke);
    std::string absatzText = text;
    if (t.lodWahl > in.stufenZahl && in.stufenZahl > 0) {
        std::snprintf(text, sizeof text, tr(Str::PqLodWieSpiel), t.lodWahl, in.stufenZahl);
        absatzText = std::string(text) + " " + absatzText;
    }
    ui::absatz(absatzText.c_str());
    modellPr().lodText = absatzText;   // Selbsttest ap5
    // Picmip wie die Vorschau (r_picmip, Einstellung).
    static const char* const kPicmip[4] = {"0##pm", "1##pm", "2##pm", "3##pm"};
    int picmip = std::clamp(g_app->settings.bildPicmip, 0, 3);
    ui::zeile(tr(Str::MbPicmip));
    if (ui::segment("##mtpicmip", kPicmip, 4, &picmip, 4, -1, nullptr, ImGui::GetFontSize() * 8.0F)) {
        g_app->settings.bildPicmip = picmip;
        modellTexturenNeuLaden();
    }
    // Die Haeute des Modells ("off" blendet Flaechen aus) und der Flaechenzaehler - AP1 Z20: bis rc611 in der Zeitleiste;
    // AP5: als zweispaltige Auswahl wie im Entwurf (gui/app_modelltab_pruefen.cpp)
    modellPrHaeute();
    ui::rolloutEnde();
}

// MBKOPF: die Gruppe "Anzeige" ist in die Kopfzeile der Ansicht gewandert (shank 10.11.: "weil das sieht da rechts einfach
// nicht gut aus") - fuer bisherige Nutzer einmal der Hinweis, wo sie jetzt steht.
void anzeigeUmzugHinweis() {
    if (g_app->settings.modellAnzeigeUmzugGesehen) { return; }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##mkumzug", ImVec2{-1.0F, 0.0F}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
    ImGui::PushTextWrapPos(0.0F);
    cjkTextUnformatted(tr(modellBlender() ? Str::MkUmzugBl : Str::MkUmzug));
    ImGui::PopTextWrapPos();
    if (ImGui::SmallButton(tr(Str::MkVerstanden))) { g_app->settings.modellAnzeigeUmzugGesehen = true; }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void rolloutAnbauten() {
    ModellTab& t = mt();
    char info[16];
    std::snprintf(info, sizeof info, "%d", static_cast<int>(t.anbauten.size()));
    if (!ui::rolloutBeginn(tr(Str::MbRAnbauten), {.kurzinfo = t.anbauten.empty() ? nullptr : info, .reiter = modellReiter(MbReiter::Hierarchie, MbReiterBl::Constraints)})) { return; }
    if (t.anbauten.empty()) {
        ui::tipp(tr(Str::MbAnbauKeine));
        ui::rolloutEnde();
        return;
    }
    for (std::size_t i = 0; i < t.anbauten.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const std::string zeile = fileName(t.anbauten[i].pfad) + " \xe2\x86\x92 " + t.anbauten[i].ziel;
        const bool loesen = ui::eintrag(zeile.c_str(), tr(Str::MbLoesen));
        ImGui::PopID();
        if (loesen) {
            t.anbauten.erase(t.anbauten.begin() + static_cast<std::ptrdiff_t>(i));
            g_app->modelDirty = true;
            break;
        }
    }
    if (!t.anbauten.empty() && ui::werkzeugKnopf(tr(Str::MbAlleLoesen), {.icon = UiIcon::Loeschen})) { modellAnbautenLoesen(); }
    ui::tipp(tr(Str::MfAnbauTipp));
    ui::rolloutEnde();
}

void rolloutPruefen() {
    ModellTab& t = mt();
    const modell::Grenzen& g = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    const std::vector<modell::Befund> b = modellBefunde();
    const int n = static_cast<int>(b.size());
    char info[64];
    if (b.empty()) {
        std::snprintf(info, sizeof info, "%s", tr(Str::MfInfoGut));
    } else {
        std::snprintf(info, sizeof info, trN(Str::MfInfoFehler, n), n);
    }
    if (!ui::rolloutBeginn(tr(Str::MbRPruefen), {.kurzinfo = info, .ton = b.empty() ? ui::Ton::Gut : ui::Ton::Fehler, .reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Objekt)})) { return; }
    ui::zeile(tr(Str::MbZielspiel));
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
    modellPrPruefZeilen();   // AP5 P6: Balken der Flaeche, Flaechen, Sequenzen, gesuchte Tags, Shader, LODs, Schatten
    // die Grenzen des Zielspiels als Tabelle (bis rc611 ein Satz)
    char titel[160];
    std::snprintf(titel, sizeof titel, tr(Str::MfPrGrenzenVon), g.name);
    ui::absatz(titel);
    const auto zahl = [](Str form, int wert) {
        char z[64];
        std::snprintf(z, sizeof z, tr(form), wert);
        return std::string(z);
    };
    char instanzen[48];
    std::snprintf(instanzen, sizeof instanzen, "%d / %d", 1, g.g2Modelle);
    ui::wertTabelle({{tr(Str::MfPrEcken), zahl(Str::MfPrUnter, g.eckenJeFlaeche), ui::Ton::Normal, {}},
                     {tr(Str::MfPrDreiecke), zahl(Str::MfPrUnter, g.dreieckeJeFlaeche), ui::Ton::Normal, {}},
                     {tr(Str::MfPrKnochen), zahl(Str::MfPrHoechstens, g.knochenJeFlaeche), ui::Ton::Normal, {}},
                     {tr(Str::MfPrGewichte), zahl(Str::MfPrHoechstens, g.gewichteJeEcke), ui::Ton::Normal, {}},
                     {tr(Str::MfPrInstanzen), instanzen, ui::Ton::Normal, {}}});
    if (b.empty()) {
        char ok[200];
        std::snprintf(ok, sizeof ok, tr(Str::MbPrOk), g.name);
        ui::absatz(ok, ui::Ton::Gut);
    }
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
        ui::absatz(z, ui::Ton::Warnung);
    }
    ui::rolloutEnde();
}

// TEST1411 B (shank 14.11.: "wenn er leer ist sieht er nicht so aus wie wenn ein model drin ist. es sollte leer auch schon so aussehen"):
// ohne Modell stehen im Befehlsfeld dieselben Gruppen in denselben Reitern wie mit Modell - zugeklappt und gesperrt, darueber ein kurzer
// Hinweis (wie 3ds Max in einer leeren Szene: Reiter und Rollouts stehen, was ein Objekt braucht, ist grau). Je Bereich die Gruppen in der
// Reihenfolge, in der drawModellBefehlsfeld sie mit Modell zeichnet, mit derselben Angabe der Reiter; gui/selbsttest_test1411.inc (B)
// vergleicht Titel und belegte Reiter je Bereich und Bedienmodus mit und ohne Modell. Modellieren zeichnet seine Gruppen selbst (Neues
// Requisit, auch ohne Modell).
struct LeereGruppe {
    Str titel;
    Str titelBl;            // wie Blender (sonst wie titel)
    unsigned (*reiter)();   // wie die Gruppe mit Modell (modellReiter, modellAnimReiter)
    int schritt = 0;        // nummerierter Schritt (UV)
    int modus = 0;          // 0 beide Bedienmodi, 1 nur wie 3ds Max, 2 nur wie Blender
};

unsigned reiterStandard() { return ui::kReiterStandard; }

const std::vector<LeereGruppe>& leereGruppenListe(ModellBereich b) {
    using R = MbReiter;
    using B = MbReiterBl;
    static const std::vector<LeereGruppe> kAnsehen = {
        {Str::MbRModell, Str::MbRModell, [] { return modellReiter(R::Anzeige, B::Daten); }},
        {Str::PqRAuswahl, Str::PqRAuswahl, [] { return modellReiter(R::Anzeige, B::Objekt); }},
        {Str::MbRLod, Str::MbRLod, [] { return modellReiter(R::Anzeige, B::Daten); }},
        {Str::PqRBoden, Str::PqRBoden, [] { return modellReiter(R::Anzeige, B::Objekt); }},
        {Str::MbRAnbauten, Str::MbRAnbauten, [] { return modellReiter(R::Hierarchie, B::Constraints); }},
        {Str::MbRPruefen, Str::MbRPruefen, [] { return modellReiter(R::Dienste, B::Objekt); }},
        {Str::PqRSzene, Str::PqRSzene, [] { return modellReiter(R::Dienste, B::Objekt); }},
    };
    static const std::vector<LeereGruppe> kUv = {
        {Str::A7RAuswahl, Str::A7RAuswahl, reiterStandard},
        {Str::A7RNaehte, Str::A7RNaehte, reiterStandard, 1},
        {Str::UvGrAbwickeln, Str::UvGrAbwickeln, reiterStandard, 2},
        {Str::A7RAufraeumen, Str::A7RAufraeumen, reiterStandard, 3},
        {Str::UvGrPruefen, Str::UvGrPruefen, reiterStandard},
    };
    static const std::vector<LeereGruppe> kWeights = {
        // der Modifier-Stapel wie 3ds Max im Reiter Aendern; wie Blender im Reiter Modifier, nicht im vorgegebenen Reiter Daten
        {Str::MgRStapel, Str::StpGruppeBl, [] { return modellReiter(R::Aendern, B::Modifier, !modellBlender()); }},
        {Str::T1pBlVertexGewichte, Str::T1pBlVertexGewichte, [] { return modellReiter(R::Aendern, B::Daten); }, 0, 2},
        {Str::MgRPinsel, Str::MgRPinsel, [] { return modellReiter(R::Aendern, B::Werkzeug); }, 0, 2},
        {Str::MgROptionen, Str::MgROptionen, [] { return modellReiter(R::Aendern, B::Werkzeug); }, 0, 2},
        {Str::MgRBereinigen, Str::MgRBereinigen, reiterStandard, 0, 2},
        {Str::MgRSpiegeln, Str::MgRSpiegeln, reiterStandard, 0, 2},
        {Str::MgRWerkzeug, Str::MgRWerkzeug, reiterStandard, 0, 1},
        {Str::MgRSpiegeln, Str::MgRSpiegeln, reiterStandard, 0, 1},
        {Str::AsGruppe, Str::AsGruppe, reiterStandard},
        {Str::MgRKnochen, Str::MgRKnochen, [] { return modellReiter(R::Hierarchie, B::Knochen); }},
        {Str::MgRGrenzen, Str::MgRGrenzen, [] { return modellReiter(R::Dienste, B::Objekt); }},
        {Str::MgRErweitert, Str::MgRErweitert, reiterStandard, 0, 1},
    };
    static const std::vector<LeereGruppe> kPainting = {
        {Str::PtGrZiel, Str::PtGrZiel, [] { return modellReiter(R::Aendern, B::Material); }},
        {Str::PtGrWerkzeuge, Str::PtGrWerkzeuge, reiterStandard},
        {Str::PtGrPinsel, Str::PtGrPinsel, reiterStandard},
        {Str::PtGrEbenen, Str::PtGrEbenen, reiterStandard},
        {Str::PtGrAusgabe, Str::PtGrAusgabe, [] { return modellReiter(R::Dienste, B::Werkzeug); }},
    };
    // Animation: die Gruppen des Motion-Felds je Modus (TEST1411 N: modellAnimReiter), dann Knochen-Darstellung, Kurven, Bibliothek, GLA;
    // Auto-Physik nicht - sie steht erst mit einem Rig da (drawModellPhysikBereich), und die leere Szene hat keins
    static const std::vector<LeereGruppe> kAnimation = {
        {Str::AzRAnimation, Str::AzRAnimation, [] { return modellAnimReiter(false, ui::kReiterStandard); }},
        {Str::AzRRig, Str::AzRRig, [] { return modellAnimReiter(true, modellReiter(R::Hierarchie, B::Knochen)); }},
        {Str::PrRollout, Str::PrRollout, [] { return modellAnimReiter(false, ui::kReiterStandard); }},
        {Str::AzRBearbeiten, Str::AzRBearbeiten, [] { return modellAnimReiter(false, ui::kReiterStandard); }},
        {Str::AzRUebertragen, Str::AzRUebertragen, [] { return modellAnimReiter(false, ui::kReiterStandard); }},
        {Str::MbIkRollout, Str::MbIkRollout, [] { return modellAnimReiter(false, modellReiter(R::Hierarchie, B::Constraints)); }},
        {Str::AzRAnbauten, Str::AzRAnbauten, [] { return modellAnimReiter(true, modellReiter(R::Hierarchie, B::Constraints)); }},
        {Str::AzRBauen, Str::AzRBauen, [] { return modellAnimReiter(true, modellReiter(R::Dienste, B::Objekt)); }},
        {Str::MbRgGruppePose, Str::MbRgGruppePose, [] { return modellAnimReiter(false, ui::kReiterStandard); }},
        {Str::MbRgGhosts, Str::MbRgGhosts, [] { return modellReiter(R::Anzeige, B::Daten, false); }},
        {Str::MbRgPfade, Str::MbRgPfade, [] { return modellAnimReiter(false, modellReiter(R::Bewegung, B::Daten)); }},
        {Str::KdGruppe, Str::KdGruppe, [] { return modellReiter(R::Anzeige, B::Daten, false); }},
        {Str::MbKvRollout, Str::MbKvRollout, [] { return modellAnimReiter(false, modellReiter(R::Bewegung, B::Werkzeug)); }},
        {Str::MbKvGruppeBibliothek, Str::MbKvGruppeBibliothek, [] { return modellAnimReiter(false, modellReiter(R::Dienste, B::Daten)); }},
        {Str::MbRGla, Str::MbRGla, [] { return modellAnimReiter(true, modellReiter(R::Dienste, B::Daten)); }},
        {Str::MbRCfg, Str::MbRCfg, [] { return modellAnimReiter(true, modellReiter(R::Dienste, B::Daten)); }},
        {Str::MbRKartenGla, Str::MbRKartenGla, [] { return modellAnimReiter(true, modellReiter(R::Dienste, B::Daten)); }},
    };
    static const std::vector<LeereGruppe> kKeine;
    switch (b) {
        case ModellBereich::Ansehen: return kAnsehen;
        case ModellBereich::Uv: return kUv;
        case ModellBereich::Weights: return kWeights;
        case ModellBereich::Painting: return kPainting;
        case ModellBereich::Animation: return kAnimation;
        case ModellBereich::Modellieren: break;
    }
    return kKeine;
}

void leereGruppen() {
    const ModellTab& t = mt();
    const bool bl = modellBlender();
    if (ui::feldReiterStandard()) {
        if (t.bereich == ModellBereich::Ansehen) { anzeigeUmzugHinweis(); }
        if (t.bereich == ModellBereich::Animation) { modellAnimModusSchalterLeer(); }
        ui::tipp(tr(Str::T1bFeldHinweis));
    }
    ImGui::BeginDisabled();
    int nr = 0;
    for (const LeereGruppe& g : leereGruppenListe(t.bereich)) {
        if ((g.modus == 1 && bl) || (g.modus == 2 && !bl)) { continue; }
        // eigene Kennung (###): die Gruppe mit Modell behaelt ihren Zustand (auf/zu)
        const std::string titel = std::string(tr(bl ? g.titelBl : g.titel)) + "###t1bleer" + std::to_string(nr++);
        if (ui::rolloutBeginn(titel.c_str(), {.schritt = g.schritt, .zu = true, .reiter = g.reiter()})) { ui::rolloutEnde(); }
    }
    ImGui::EndDisabled();
}

}  // namespace

// TEST1411 B: ein Editor ohne Modell - der Rahmen an seinem Platz, oben sein Titel, mittig "Kein Modell geladen"
void modellEditorLeer(const char* id, const char* titel, ImVec2 groesse) {
    ImGui::BeginChild(id, groesse, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextUnformatted(titel);
    const char* hinweis = tr(Str::T1bLeerTitel);
    const ImVec2 g = ImGui::CalcTextSize(hinweis);
    const ImVec2 o = ImGui::GetWindowPos();
    const ImVec2 f = ImGui::GetWindowSize();
    ImGui::SetCursorScreenPos(ImVec2{std::floor(o.x + (f.x - g.x) * 0.5F), std::floor(o.y + (f.y - g.y) * 0.5F)});
    cjkTextDisabled("%s", hinweis);
    ImGui::EndChild();
}

void drawModellBefehlsfeld() {
    ModellTab& t = mt();
    // AP3: der Rahmen - Titelzeile (Befehlsfeld / Eigenschaften | Bereich), die Reiter des Bedienmodus und der rollende
    // Bereich "mtrollouts" (gui/app_modelltab_feld.cpp). Der Schalter des Bedienmodus steht in der Statuszeile (Klick auf
    // "wie 3ds Max" / "wie Blender"), im Menue der Kopfzeile der Ansicht und in den Einstellungen.
    modellFeldBeginn();
    if (g_app->model.empty() && t.bereich != ModellBereich::Modellieren) { leereGruppen(); }   // TEST1411 B
    if (t.bereich == ModellBereich::Weights && !g_app->model.empty()) { drawModellGewichte(); }   // AP8: gui/app_modelltab_gewichte.cpp
    if (t.bereich == ModellBereich::Uv && !g_app->model.empty()) { drawModellUvBereich(); }   // Modell-Block 1b
    if (t.bereich == ModellBereich::Modellieren) { drawModellMeshBereich(); }   // Modell-Block 1d (auch ohne Modell: Neues Requisit)
    if (t.bereich == ModellBereich::Painting && !g_app->model.empty()) { drawModellMalenBereich(); }   // Modell-Block 1c
    if (t.bereich == ModellBereich::Animation && !g_app->model.empty()) {
        drawModellRigBereich();       // Stufe 3: Rig, Posen, Ghosts, Pfade, Backen
        drawModellKurvenBereich();    // Stufe 3b: Editoren, Reduzieren, Spiegeln, Posen-Bibliothek
        drawModellAnimationBereich();   // Stufe 2
    }
    if (!g_app->model.empty()) {
        // MBKOPF: die Anzeige-Schalter stehen in der Kopfzeile der Ansicht - der Hinweis darauf nur im Reiter, den der Bereich vorgibt
        // (AP5: vor den Gruppen von Ansehen/Pruefen, sonst stuende er unter dem Rand des Felds)
        if (ui::feldReiterStandard()) { anzeigeUmzugHinweis(); }
        if (t.bereich == ModellBereich::Ansehen) {
            rolloutModell();
            modellPrRolloutAuswahl();   // AP5 X18
            rolloutLod();
            modellPrRolloutBoden();   // AP5 P4
            rolloutAnbauten();
            rolloutPruefen();
            modellPrRolloutSzene();   // AP5 P7
        }
    }
    if (!t.meldung.empty()) {
        ImGui::Separator();
        ImGui::PushTextWrapPos(0.0F);
        cjkTextDisabled("%s", t.meldung.c_str());
        ImGui::PopTextWrapPos();
    }
    modellFeldEnde();
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
    t.ansichten[t.aktiv].animStart = -1.0;
    t.ansichten[t.aktiv].art = a;
    modellAnsichtEinpassenAktiv();
}

// MBKOPF: U - Isometrische Benutzeransicht (max_hotkeys_de.txt), die Richtung bleibt
void orthoBenutzer() {
    ModellTab& t = mt();
    float w[3];
    modellAnsichtWinkel(t.ansichten[t.aktiv], w);
    modellBlickSetzen(t.aktiv, AnsichtArt::Orthographisch, w, false);
}

void achseSetzen(int a) {
    ModellTab& t = mt();
    t.achse = t.achse == a ? -1 : a;
}

void blenderAlle(bool an);   // mb1a2: unten bei den Tasten wie Blender
void blenderUmkehren();

// FIX612C (P3-6): Q/W/E/R wie 3ds Max beenden auch "Knochen waehlen" (wie die Grundwerkzeuge der Leiste wie Blender)
void maxWerkzeug(int gizmo) {
    mt().gizmo = gizmo;
    if (modellBl().werkzeug == 2) {
        modellBl().werkzeug = 0;
        mt().meldung = tr(Str::BzKnochenAus);
    }
}

const std::vector<MaxTaste>& maxTasten() {
    static const std::vector<MaxTaste> kListe = {
        {ImGuiKey_Q, Str::MbAktAuswaehlen, [] { maxWerkzeug(0); }},
        {ImGuiKey_W, Str::MbAktVerschieben, [] { maxWerkzeug(1); }},
        {ImGuiKey_E, Str::MbAktDrehen, [] { maxWerkzeug(2); }},
        {ImGuiKey_R, Str::MbAktSkalieren, [] { maxWerkzeug(3); }},
        {ImGuiMod_Alt | ImGuiKey_W, Str::MbAktMax, [] { mt().vierAnsichten = !mt().vierAnsichten; }},
        {ImGuiKey_Z, Str::MbAktZoom, [] { modellNavGrenzen(0); }},   // MBNAV: Zoom Extents All Selected - alle Ansichten
        {ImGuiKey_F3, Str::MbAnDraht, [] { modellSch().drahtgitter = !modellSch().drahtgitter; }},   // MBKOPF: die aktive Ansicht
        {ImGuiKey_F4, Str::MbAnKanten, [] { modellSch().kanten = !modellSch().kanten; }},
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
        {ImGuiKey_U, Str::MkAktOrtho, [] { orthoBenutzer(); }},
        {ImGuiMod_Ctrl | ImGuiKey_A, Str::MbAktAlle, [] { blenderAlle(true); }},
        {ImGuiMod_Ctrl | ImGuiKey_D, Str::MbAktNichts, [] { blenderAlle(false); }},
        {ImGuiMod_Ctrl | ImGuiKey_I, Str::MbAktUmkehren, [] { blenderUmkehren(); }},
        {ImGuiKey_Comma, Str::MbAktZurueck, [] { bildSchritt(-1); }},
        {ImGuiKey_Period, Str::MbAktVor, [] { bildSchritt(1); }},
        {ImGuiKey_Slash, Str::MbAktAbspielen, [] {
             if (g_app->animIndex >= 0) { g_app->animPlaying = !g_app->animPlaying; }
         }},
        {ImGuiKey_Home, Str::MbAktAnfang, [] { bildRand(false); }},
        {ImGuiKey_End, Str::MbAktEnde, [] { bildRand(true); }},
        {ImGuiKey_G, Str::MbAnRaster, [] { modellSch().raster = !modellSch().raster; }},
        {ImGuiKey_J, Str::MbAnKlammer, [] { mt().an.klammer = !mt().an.klammer; }},
        {ImGuiKey_7, Str::MbAnStatistik, [] { modellSch().statistik = !modellSch().statistik; }},
        // mb1a2: Unterobjekt-Ebenen (Autodesk-Karte: 1 Scheitelpunkt, 2 Kante, 3 Rand, 4 Polygon, 5 Element, 6 Objekt;
        // vorbilder_max_blender.md) - Polygon ist in der .glm ein Dreieck
        {ImGuiKey_1, Str::MbMaxEcke, [] { modellAuswahlArtSetzen(AuswahlArt::Ecke); }},
        {ImGuiKey_2, Str::MbMaxKante, [] { modellAuswahlArtSetzen(AuswahlArt::Kante); }},
        {ImGuiKey_3, Str::MbMaxRand, [] { modellAuswahlArtSetzen(AuswahlArt::Rand); }},
        {ImGuiKey_4, Str::MbMaxPolygon, [] { modellAuswahlArtSetzen(AuswahlArt::Dreieck); }},
        {ImGuiKey_5, Str::MbMaxElement, [] { modellAuswahlArtSetzen(AuswahlArt::Element); }},
        {ImGuiKey_6, Str::MbMaxObjekt, [] { modellAuswahlArtSetzen(AuswahlArt::Objekt); }},
        {ImGuiKey_H, Str::MbAktName, [] { modellExplorerSucheFokus(); }},
        {ImGuiKey_X, Str::MbAktionen, [] { mt().aktionenOffen = true; }},
        {ImGuiMod_Ctrl | ImGuiKey_Z, Str::MbAktUndo, [] { (void)modellRueckgaengig(); }},
        {ImGuiMod_Ctrl | ImGuiKey_Y, Str::MbAktRedo, [] { (void)modellWiederholen(); }},
        {ImGuiMod_Alt | ImGuiKey_R, Str::MbAktTexNeu, [] { modellTexturenNeuLaden(); }},
        // MBNAV: Navigation wie 3ds Max (Hotkey-Tabelle der Autodesk-Hilfe 2026, Main UI > Tools / Views; gui/app_modelltab_nav.cpp).
        // ENTSCHEID3A (B10): Strg+Umschalt+Z ist wie in 3ds Max Zoom Extents All (alle Ansichten, ganze Figur); Wiederholen ist Strg+Y
        {ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, Str::MnGrenzenAlle, [] { modellNavGrenzen(2); }},
        {ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_Z, Str::MnGrenzen, [] { modellNavGrenzen(1); }},
        {ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_Z, Str::MnZoom2xHeran, [] { modellNavZoom2x(true); }},
        {ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_Z, Str::MnZoom2xHinaus, [] { modellNavZoom2x(false); }},
        {ImGuiKey_LeftBracket, Str::MnZoomHeran, [] { modellNavZoomSchritt(1.0F); }},
        {ImGuiKey_RightBracket, Str::MnZoomHinaus, [] { modellNavZoomSchritt(-1.0F); }},
        {ImGuiMod_Ctrl | ImGuiKey_Equal, Str::MnZoomHeran2, [] { modellNavZoomSchritt(1.0F); }},
        {ImGuiMod_Ctrl | ImGuiKey_Minus, Str::MnZoomHinaus2, [] { modellNavZoomSchritt(-1.0F); }},
        {ImGuiMod_Alt | ImGuiKey_Z, Str::MnModusZoom, [] { modellNavModusSetzen(1); }},
        {ImGuiMod_Ctrl | ImGuiKey_P, Str::MnModusPan, [] { modellNavModusSetzen(2); }},
        {ImGuiMod_Ctrl | ImGuiKey_R, Str::MnModusOrbit, [] { modellNavModusSetzen(3); }},
        {ImGuiMod_Ctrl | ImGuiKey_W, Str::MnModusBereich, [] { modellNavModusSetzen(4); }},
        {ImGuiKey_I, Str::MnZumZeiger, [] { modellNavZumZeiger(); }},
        {ImGuiMod_Shift | ImGuiKey_Z, Str::MnRueck, [] { (void)modellNavRueck(false); }},
        {ImGuiMod_Shift | ImGuiKey_Y, Str::MnWieder, [] { (void)modellNavRueck(true); }},
        {ImGuiKey_V, Str::MnAnsichtenMenue, [] { modellNavAnsichtenMenue(); }},
        {ImGuiMod_Alt | ImGuiKey_Home, Str::MnHomeTaste, [] { modellHomeFlug(mt().aktiv); }},
        {ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_H, Str::MnHomeTaste2, [] { modellHomeFlug(mt().aktiv); }},
        {ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_V, Str::MnCubeTaste, [] { g_app->settings.modellNavigation = !g_app->settings.modellNavigation; }},
    };
    return kListe;
}

// --- mb1a2: Tasten wie Blender ------------------------------------------------------------------------
//
// Die Belegung "Blender" (Vorgabe) aus belege/blender_keys.txt (Blender 5.0.0, keymap_data/blender_default.py,
// ausgelesen 04.11.) - je Taste die Zeile als Quelle. Die Ziffernblock-Ansichten und Pos1 stehen nicht im Auszug
// (er traegt nur die Gruppen Window, Screen, Frames, Object/Mesh/Pose, UV); sie folgen blender_default.py
// (view3d.view_axis, view_persportho, view_orbit, view_all) und sind als "Doku" markiert. Die Leertaste spielt ab
// (Vorgabe "Spacebar Action = Play", vorbilder_max_blender.md). Die Liste ist zugleich die Pruefliste des Selbsttests.
struct BlenderTaste {
    ImGuiKeyChord chord;
    Str text;
    std::function<void()> tun;
    const char* quelle;
};

void blenderModal(int art) {
    if (modellRigModalStarten(art)) { return; }   // Stufe 3: im Bereich Animation dreht R den gewaehlten Griff
    (void)modellModalStarten(art);
}

void blenderAlle(bool an) {
    ModellTab& t = mt();
    if (t.auswahlArt == AuswahlArt::Objekt) {
        t.auswahl.clear();
        if (an) {
            for (std::size_t i = 0; i < t.gezeichnet.size(); ++i) {
                if (t.gezeichnet[i] != 0) { t.auswahl.push_back(static_cast<int>(i)); }
            }
        }
        return;
    }
    t.eckenWahl.clear();
    if (!an) { return; }
    const modell::Inhalt& in = t.dok.inhalt();
    for (std::size_t i = 0; i < in.teile.size() && i < t.gezeichnet.size(); ++i) {
        if (t.gezeichnet[i] == 0 || static_cast<std::size_t>(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
        const std::size_t n = in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].ecken.size();
        for (std::size_t e = 0; e < n; ++e) { t.eckenWahl.push_back(modell::EckOrt{static_cast<int>(i), static_cast<int>(e)}); }
    }
}

void blenderUmkehren() {
    ModellTab& t = mt();
    if (t.auswahlArt == AuswahlArt::Objekt) {
        std::vector<int> neu;
        for (std::size_t i = 0; i < t.gezeichnet.size(); ++i) {
            if (t.gezeichnet[i] != 0 && std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) == t.auswahl.end()) {
                neu.push_back(static_cast<int>(i));
            }
        }
        t.auswahl = neu;
        return;
    }
    const std::vector<modell::EckOrt> alt = t.eckenWahl;
    blenderAlle(true);
    std::vector<modell::EckOrt> neu;
    for (const modell::EckOrt& o : t.eckenWahl) {
        const bool war = std::any_of(alt.begin(), alt.end(), [&](const modell::EckOrt& x) { return x.teil == o.teil && x.ecke == o.ecke; });
        if (!war) { neu.push_back(o); }
    }
    t.eckenWahl = neu;
}

void spielenUmschalten() {
    if (g_app->animIndex >= 0) { g_app->animPlaying = !g_app->animPlaying; }
}

const std::vector<BlenderTaste>& blenderTasten() {
    static const std::vector<BlenderTaste> kListe = {
        {ImGuiKey_G, Str::MbBlGreifen, [] { blenderModal(1); }, "Object Mode | transform.translate | G PRESS"},
        {ImGuiKey_R, Str::MbBlDrehen, [] { blenderModal(2); }, "Object Mode | transform.rotate | R PRESS"},
        {ImGuiKey_S, Str::MbBlSkalieren, [] { blenderModal(3); }, "Object Mode | transform.resize | S PRESS"},
        {ImGuiKey_Tab, Str::MbBlTab, [] {
             ModellTab& t = mt();
             modellAuswahlArtSetzen(t.auswahlArt == AuswahlArt::Objekt ? t.editArt : AuswahlArt::Objekt);
         },
         "Object Non-modal | object.mode_set | TAB PRESS mode=EDIT, toggle=True"},
        {ImGuiMod_Ctrl | ImGuiKey_Tab, Str::MbBlModusMenue, [] { mt().bereichMenue = true; },
         "Object Non-modal | view3d.object_mode_pie_or_toggle | Strg+TAB PRESS"},
        {ImGuiKey_1, Str::MbBlEcke, [] { modellAuswahlArtSetzen(AuswahlArt::Ecke); }, "Mesh | mesh.select_mode | ONE PRESS type=VERT"},
        {ImGuiKey_2, Str::MbBlKante, [] { modellAuswahlArtSetzen(AuswahlArt::Kante); }, "Mesh | mesh.select_mode | TWO PRESS type=EDGE"},
        {ImGuiKey_3, Str::MbBlFlaeche, [] { modellAuswahlArtSetzen(AuswahlArt::Dreieck); }, "Mesh | mesh.select_mode | THREE PRESS type=FACE"},
        {ImGuiKey_A, Str::MbBlAlle, [] { blenderAlle(true); }, "Object Mode | object.select_all | A PRESS action=SELECT"},
        {ImGuiMod_Alt | ImGuiKey_A, Str::MbBlNichts, [] { blenderAlle(false); }, "Object Mode | object.select_all | Alt+A PRESS action=DESELECT"},
        {ImGuiMod_Ctrl | ImGuiKey_I, Str::MbBlUmkehren, [] { blenderUmkehren(); }, "Object Mode | object.select_all | Strg+I PRESS action=INVERT"},
        {ImGuiKey_B, Str::MbBlRahmen, [] { mt().gizmo = 0; }, "3D View | view3d.select_box | B PRESS"},
        {ImGuiKey_Keypad1, Str::MbAnsichtVorne, [] { modellNavBlAchse(AnsichtArt::Vorne); }, "Doku | view3d.view_axis | NUMPAD_1 type=FRONT"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad1, Str::MbAnsichtHinten, [] { modellNavBlAchse(AnsichtArt::Hinten); },
         "Doku | view3d.view_axis | Strg+NUMPAD_1 type=BACK"},
        {ImGuiKey_Keypad3, Str::MbAnsichtRechts, [] { modellNavBlAchse(AnsichtArt::Rechts); }, "Doku | view3d.view_axis | NUMPAD_3 type=RIGHT"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad3, Str::MbAnsichtLinks, [] { modellNavBlAchse(AnsichtArt::Links); },
         "Doku | view3d.view_axis | Strg+NUMPAD_3 type=LEFT"},
        {ImGuiKey_Keypad7, Str::MbAnsichtOben, [] { modellNavBlAchse(AnsichtArt::Oben); }, "Doku | view3d.view_axis | NUMPAD_7 type=TOP"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad7, Str::MbAnsichtUnten, [] { modellNavBlAchse(AnsichtArt::Unten); },
         "Doku | view3d.view_axis | Strg+NUMPAD_7 type=BOTTOM"},
        {ImGuiKey_Keypad5, Str::MbBlPersp, [] { modellPerspektiveUmschalten(); }, "Doku | view3d.view_persportho | NUMPAD_5"},
        {ImGuiKey_Keypad4, Str::MbBlKreisLinks, [] { modellKreisenSchritt(15.0F, 0.0F); }, "Doku | view3d.view_orbit | NUMPAD_4 ORBITLEFT"},
        {ImGuiKey_Keypad6, Str::MbBlKreisRechts, [] { modellKreisenSchritt(-15.0F, 0.0F); }, "Doku | view3d.view_orbit | NUMPAD_6 ORBITRIGHT"},
        {ImGuiKey_Keypad8, Str::MbBlKreisOben, [] { modellKreisenSchritt(0.0F, 15.0F); }, "Doku | view3d.view_orbit | NUMPAD_8 ORBITUP"},
        {ImGuiKey_Keypad2, Str::MbBlKreisUnten, [] { modellKreisenSchritt(0.0F, -15.0F); }, "Doku | view3d.view_orbit | NUMPAD_2 ORBITDOWN"},
        // MBKOPF: Gegenseite, Rollen, Drahtgitter, Gizmos (blender_default.py; Strg+` aus blender_keys.txt)
        {ImGuiKey_Keypad9, Str::MkBlGegenseite, [] { modellGegenseite(); }, "Doku | view3d.view_orbit | NUMPAD_9 angle=pi ORBITRIGHT"},
        {ImGuiMod_Shift | ImGuiKey_Keypad4, Str::MkBlRollLinks, [] { modellRollenSchritt(-15.0F); }, "Doku | view3d.view_roll | Umschalt+NUMPAD_4 LEFT"},
        {ImGuiMod_Shift | ImGuiKey_Keypad6, Str::MkBlRollRechts, [] { modellRollenSchritt(15.0F); }, "Doku | view3d.view_roll | Umschalt+NUMPAD_6 RIGHT"},
        {ImGuiMod_Shift | ImGuiKey_Z, Str::MkBlDrahtTaste, [] { modellSch().drahtgitter = !modellSch().drahtgitter; },
         "Doku | view3d.toggle_shading | Umschalt+Z type=WIREFRAME"},
        {ImGuiMod_Ctrl | ImGuiKey_GraveAccent, Str::MkBlGizmoTaste, [] { g_app->settings.modellNavigation = !g_app->settings.modellNavigation; },
         "3D View | wm.context_toggle | Strg+ACCENT_GRAVE PRESS data_path=space_data.show_gizmo"},
        {ImGuiKey_KeypadDecimal, Str::MbBlGewaehlte, [] { (void)modellNavBlEinpassen(true, false); }, "3D View | view3d.view_selected | NUMPAD_PERIOD PRESS"},
        {ImGuiKey_Home, Str::MbBlAlles, [] { (void)modellNavBlEinpassen(false, false); }, "3D View | view3d.view_all | HOME center=False"},
        {ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_Q, Str::MbBlVier, [] { modellNavBlVier(); }, "Doku | screen.region_quadview | Strg+Alt+Q"},
        // MBNAV: blender_default.py km_view3d (Blender 5.0)
        {ImGuiMod_Shift | ImGuiKey_C, Str::MnBlAllesMitte, [] { (void)modellNavBlEinpassen(false, true); }, "3D View | view3d.view_all | Umschalt+C center=True"},
        {ImGuiKey_KeypadAdd, Str::MnBlZoomHeran, [] { modellNavZoomSchritt(1.0F); }, "3D View | view3d.zoom | NUMPAD_PLUS delta=1"},
        {ImGuiKey_KeypadSubtract, Str::MnBlZoomHinaus, [] { modellNavZoomSchritt(-1.0F); }, "3D View | view3d.zoom | NUMPAD_MINUS delta=-1"},
        {ImGuiMod_Ctrl | ImGuiKey_Equal, Str::MnBlZoomHeran2, [] { modellNavZoomSchritt(1.0F); }, "3D View | view3d.zoom | Strg+EQUAL delta=1"},
        {ImGuiMod_Ctrl | ImGuiKey_Minus, Str::MnBlZoomHinaus2, [] { modellNavZoomSchritt(-1.0F); }, "3D View | view3d.zoom | Strg+MINUS delta=-1"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad4, Str::MnBlPanLinks, [] { modellNavBlSchieben(0); }, "3D View | view3d.view_pan | Strg+NUMPAD_4 PANLEFT"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad6, Str::MnBlPanRechts, [] { modellNavBlSchieben(1); }, "3D View | view3d.view_pan | Strg+NUMPAD_6 PANRIGHT"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad8, Str::MnBlPanOben, [] { modellNavBlSchieben(2); }, "3D View | view3d.view_pan | Strg+NUMPAD_8 PANUP"},
        {ImGuiMod_Ctrl | ImGuiKey_Keypad2, Str::MnBlPanUnten, [] { modellNavBlSchieben(3); }, "3D View | view3d.view_pan | Strg+NUMPAD_2 PANDOWN"},
        {ImGuiKey_GraveAccent, Str::MnBlKuchen, [] { modellNavKuchen(); }, "3D View | wm.call_menu_pie VIEW3D_MT_view_pie | ACCENT_GRAVE PRESS"},
        {ImGuiMod_Shift | ImGuiKey_B, Str::MnBlBereich, [] { modellNavBlBereich(); }, "3D View | view3d.zoom_border | Umschalt+B PRESS"},
        {ImGuiKey_H, Str::MbAuswahlAusblenden, [] { modellAuswahlAusblenden(false); }, "Doku | object.hide_view_set | H"},
        {ImGuiMod_Shift | ImGuiKey_H, Str::MbAndereAusblenden, [] { modellAuswahlAusblenden(true); },
         "Doku | object.hide_view_set unselected=True | Umschalt+H"},
        {ImGuiMod_Alt | ImGuiKey_H, Str::MbAllesEinblenden, [] { modellAllesEinblenden(); }, "Doku | object.hide_view_clear | Alt+H"},
        {ImGuiKey_Space, Str::MbAktAbspielen, [] { spielenUmschalten(); }, "vorbilder: Spacebar Action = Play (Blender.py)"},
        {ImGuiMod_Shift | ImGuiKey_Space, Str::MbAktAbspielen, [] { spielenUmschalten(); }, "Frames | screen.animation_play | Umschalt+SPACE PRESS"},
        {ImGuiKey_LeftArrow, Str::MbAktZurueck, [] { bildSchritt(-1); }, "Frames | screen.frame_offset | LEFT_ARROW PRESS delta=-1"},
        {ImGuiKey_RightArrow, Str::MbAktVor, [] { bildSchritt(1); }, "Frames | screen.frame_offset | RIGHT_ARROW PRESS delta=1"},
        {ImGuiMod_Shift | ImGuiKey_LeftArrow, Str::MbAktAnfang, [] { bildRand(false); }, "Frames | screen.frame_jump | Umschalt+LEFT_ARROW PRESS end=False"},
        {ImGuiMod_Shift | ImGuiKey_RightArrow, Str::MbAktEnde, [] { bildRand(true); }, "Frames | screen.frame_jump | Umschalt+RIGHT_ARROW PRESS end=True"},
        {ImGuiKey_F3, Str::MbAktionen, [] { mt().aktionenOffen = true; }, "Window | wm.search_menu | F3 PRESS"},
        // AP5: Texturen neu laden (ModView Alt+R, Entwurf auch wie Blender - im Bereich Animation gilt vorher Alt+R des Rigs: pose.rot_clear),
        // Z Schattierung und Umschalt+Alt+Z Overlays (P10)
        {ImGuiMod_Alt | ImGuiKey_R, Str::MbAktTexNeu, [] { modellTexturenNeuLaden(); }, "Entwurf | Texturen neu laden | Alt+R (ModView: Refresh textures)"},
        {ImGuiKey_Z, Str::PqSchattierungMenue, [] { modellPr().schattierungOeffnen = true; }, "3D View | wm.call_menu_pie | Z PRESS name=VIEW3D_MT_shading_pie"},
        {ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_Z, Str::PqOverlays, [] {
             bool& aus = modellPr().overlaysAus[std::clamp(mt().aktiv, 0, 3)];
             aus = !aus;
         },
         "3D View | wm.context_toggle | Umschalt+Alt+Z PRESS data_path=space_data.overlay.show_overlays"},
        {ImGuiMod_Ctrl | ImGuiKey_Z, Str::MbAktUndo, [] { (void)modellRueckgaengig(); }, "Screen | ed.undo | Strg+Z PRESS"},
        {ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, Str::EaRedoBl, [] { (void)modellWiederholen(); }, "Screen | ed.redo | Strg+Umschalt+Z PRESS"},
    };
    return kListe;
}

// --- mb1a2: die Funktionen ohne eigene Taste des Vorbilds - in "Alle Aktionen suchen" (X / F3) und im Rechtsklick-
// Menue der Ansicht, mit Tooltip (shank 06.11.: "dass man es leicht und gut findet"). `taste` ist die eigene Taste von
// JK Studio, wo es eine gibt.
struct ZusatzAktion {
    Str text;
    Str hinweis;
    const char* taste;
    std::function<void()> tun;
    int gruppe;   // 0 Weights, 1 Auswahl, 3 Skript, 4 Animation (die Anzeige: MBKOPF, modellAnzeigeKontext)
};

void weights(int art) {
    ModellTab& t = mt();
    t.bereich = ModellBereich::Weights;
    if (art >= 0) { t.pinselArt = art; }
    g_app->modelDirty = true;
}

const std::vector<ZusatzAktion>& zusatzAktionen() {
    static const std::vector<ZusatzAktion> kListe = {
        {Str::MbBereichWeights, Str::MbGwKnochenWaehlen, "", [] { weights(-1); }, 0},
        {Str::MbGwHinzu, Str::MbGwHinzuHint, "", [] { weights(static_cast<int>(modell::PinselArt::Hinzufuegen)); }, 0},
        {Str::MbGwAbziehen, Str::MbGwAbziehenHint, "", [] { weights(static_cast<int>(modell::PinselArt::Abziehen)); }, 0},
        {Str::MbGwGlaetten, Str::MbGwGlaettenHint, "", [] { weights(static_cast<int>(modell::PinselArt::Glaetten)); }, 0},
        {Str::MbGwSetzen, Str::MbGwSetzenHint, "", [] { weights(static_cast<int>(modell::PinselArt::Setzen)); }, 0},
        {Str::MbGwAufAuswahl, Str::MbGwAufAuswahlHint, "", [] { (void)modellAufAuswahlSetzen(); }, 0},
        {Str::MrGwAufnehmen, Str::MrGwAufnehmenHint, "", [] {
             weights(-1);
             mt().gwAufnehmenKlick = true;   // der naechste Klick in die Ansicht nimmt auf (aus dem Menue steht die Maus nicht auf dem Modell)
             mt().meldung = tr(Str::MrGwAufnehmenKlick);
         },
         0},
        {Str::MbGwSpiegelnPlus, Str::MbGwSpiegelHint, "", [] { (void)modellSpiegeln(1); }, 0},
        {Str::MbGwSpiegelnMinus, Str::MbGwSpiegelHint, "", [] { (void)modellSpiegeln(-1); }, 0},
        {Str::MbGwUebernehmen, Str::MbGwUebernehmenHint, "", [] { (void)modellUebernehmen(); }, 0},
        {Str::AsAusfuehren, Str::AsAusfuehrenHint, "", [] { (void)modellAutoSkin(); }, 0},   // AP12
        {Str::AsOeffnen, Str::AsOeffnenHint, "", [] { modellAutoSkinOeffnen(); }, 0},
        // AP3 R4: der Bedienmodus (bis rc611 zwei Knoepfe im rechten Feld; sonst Statuszeile, Kopfzeile der Ansicht, Einstellungen)
        {Str::MbWieMax, Str::MzModusHint, "", [] { modellBedienungSetzen(0); }, 1},
        {Str::MbWieBlender, Str::MzModusHint, "", [] { modellBedienungSetzen(1); }, 1},
        {Str::MbAuswahlAusblenden, Str::MbAugeHint, "", [] { modellAuswahlAusblenden(false); }, 1},
        {Str::MbAndereAusblenden, Str::MbAugeHint, "", [] { modellAuswahlAusblenden(true); }, 1},
        {Str::MbAllesEinblenden, Str::MbAugeHint, "", [] { modellAllesEinblenden(); }, 1},
        {Str::MbInsSkriptFenster, Str::MbInsSkriptHint, "Ctrl+Shift+I", [] { modellInsSkriptOeffnen(g_app->animIndex); }, 3},
        {Str::MbDateiSpeichern, Str::MbDateiSpeichernHint, "Ctrl+S", [] { modellSpeichern(false); }, 3},
        {Str::MbDateiSpeichernUnter, Str::MbDateiSpeichernUnterHint, "Ctrl+Shift+S", [] { modellSpeichern(true); }, 3},
        {Str::MbGlaSpeichern, Str::MbGlaSpeichernHint, "", [] { modellGlaSpeichernFrage(); }, 4},
        {Str::MbCfgSpeichern, Str::MbCfgSpeichernHint, "", [] { modellCfgSpeichernFrage(); }, 4},
        {Str::MbKgBauen, Str::MbKgBauenHint, "", [] { modellKartenGlaBauenFrage(); }, 4},
        {Str::MbKgVorhanden, Str::MbKgVorhandenHint, "", [] { (void)modellVorhandeneKartenGla(); }, 4},
        {Str::MbKgAufFreien, Str::MbKgZiehen, "", [] { (void)modellAufFreienPlatz(g_app->animIndex); }, 4},
        {Str::MbXsiAufnehmen, Str::MbXsiAufnehmenHint, "", [] { modellXsiAufnehmenFrage(); }, 4},
    };
    return kListe;
}

std::string zusatzText(const ZusatzAktion& a) {
    std::string t = tr(a.text);
    if (a.taste[0] != '\0') { t += std::string(" (") + a.taste + ")"; }
    return t;
}

void zusatzAusloesen(const ZusatzAktion& a) {
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(a.text);
    g_app->modelDirty = true;
}

}  // namespace

int modellZusatzZahl() { return static_cast<int>(zusatzAktionen().size()); }
const char* modellZusatzText(int i) { return tr(zusatzAktionen()[static_cast<std::size_t>(i)].text); }
const char* modellZusatzHinweis(int i) { return tr(zusatzAktionen()[static_cast<std::size_t>(i)].hinweis); }
void modellZusatzAusloesen(int i) { zusatzAusloesen(zusatzAktionen()[static_cast<std::size_t>(i)]); }

// mb1a2: das Rechtsklick-Menue der Ansicht (3ds Max: Quad-Menue, Blender: Kontextmenue) - Auswahlart, Weights, Anzeige,
// Ins Skript; jede Zeile mit Tooltip.
// --- TEST1411 J: das Quad-Menue wie 3ds Max ----------------------------------------------------------------------------------
//
// shank 14.11.: "mit rechts klick würde ich gerne rotate oder movement auswählen können". 3ds Max oeffnet mit Rechtsklick in die Ansicht
// das Quad-Menue: vier Viertel um die Maus - rechts unten "transform" (Move, Rotate, Scale, Select, Select Similar, Clone, Object
// Properties, Curve Editor, Dope Sheet ...), rechts oben "display" (Isolate Selection, End Isolate, Unfreeze All, Freeze Selection,
// Unhide by Name, Unhide All, Hide Unselected, Hide Selection), links "tools 1" und "tools 2" je nach Objekt (Autodesk-Hilfe "Quad
// Menus"; Reihenfolge und Schreibweise wie dort). Was das Menue bis rc612 in einer Spalte hatte, steht in tools 1 (Unterobjekte, die
// Werkzeuge des Bereichs) und tools 2 (Anzeige, Weights, Animation, Werkzeugleisten).
struct Quad {
    ImVec2 groesse{0.0F, 0.0F};   // TEST1411c: das ganze Quad-Menue im letzten Bild (fuer die Lage im Fenster)
    ImVec2 klick{};
    float w[4]{};   // Breite und Hoehe der Viertel im letzten Bild: 0 links oben, 1 rechts oben, 2 links unten, 3 rechts unten
    float h[4]{};
    bool gemessen = false;
};
Quad g_quad;

void quadZeile(const char* text, const char* taste, bool an, bool geht, const std::function<void()>& tun) {
    if (ImGui::MenuItem(text, taste, an, geht)) { tun(); }
}

void quadTransform() {
    ModellTab& t = mt();
    quadZeile(tr(Str::T1QuadMove), "W", t.gizmo == 1, true, [] { maxWerkzeug(1); });
    quadZeile(tr(Str::T1QuadRotate), "E", t.gizmo == 2, true, [] { maxWerkzeug(2); });
    quadZeile(tr(Str::T1QuadScale), "R", t.gizmo == 3, true, [] { maxWerkzeug(3); });
    quadZeile(tr(Str::T1QuadSelect), "Q", t.gizmo == 0, true, [] { maxWerkzeug(0); });
    ImGui::Separator();
    quadZeile(tr(Str::T1QuadCurveEditor), nullptr, modellZeit().miniKurve && t.kvArt == 1, true, [] {
        ModellTab& m = mt();
        m.bereich = ModellBereich::Animation;
        m.kvArt = 1;
        if (!modellZeit().miniKurve) { modellZeitMiniKurve(true); }
    });
    quadZeile(tr(Str::T1QuadDopeSheet), nullptr, modellZeit().miniKurve && t.kvArt == 0, true, [] {
        ModellTab& m = mt();
        m.bereich = ModellBereich::Animation;
        m.kvArt = 0;
        if (!modellZeit().miniKurve) { modellZeitMiniKurve(true); }
    });
}

void quadDisplay() {
    const ModellTab& t = mt();
    const bool auswahl = !t.auswahl.empty();
    quadZeile(tr(Str::T1QuadIsolate), "Alt+Q", false, auswahl, [] { modellAuswahlAusblenden(true); });
    quadZeile(tr(Str::T1QuadEndIsolate), nullptr, false, true, [] { modellAllesEinblenden(); });
    ImGui::Separator();
    quadZeile(tr(Str::T1QuadUnfreezeAll), nullptr, false, true, [] { modellAllesAuftauen(); });
    quadZeile(tr(Str::T1QuadFreezeSel), nullptr, false, auswahl, [] {
        const std::vector<int> wahl = mt().auswahl;
        for (const int i : wahl) { modellTeilEinfrieren(i, true, false); }
    });
    ImGui::Separator();
    quadZeile(tr(Str::T1QuadUnhideAll), nullptr, false, true, [] { modellAllesEinblenden(); });
    quadZeile(tr(Str::T1QuadHideUnsel), nullptr, false, auswahl, [] { modellAuswahlAusblenden(true); });
    quadZeile(tr(Str::T1QuadHideSel), nullptr, false, auswahl, [] { modellAuswahlAusblenden(false); });
}

void quadTools1() {
    ModellTab& t = mt();
    static const Str kArten[6] = {Str::MbMaxObjekt, Str::MbMaxEcke, Str::MbMaxKante, Str::MbMaxPolygon, Str::MbMaxRand, Str::MbMaxElement};
    for (int a = 0; a < 6; ++a) {
        if (ImGui::MenuItem(tr(kArten[a]), nullptr, static_cast<int>(t.auswahlArt) == a)) { modellAuswahlArtSetzen(static_cast<AuswahlArt>(a)); }
    }
    modellRigKontextMenue();   // Stufe 3
    modellUvKontextMenue();    // Modell-Block 1b
    modellMeshKontextMenue();  // Modell-Block 1d
    modellMalenKontextMenue(); // Modell-Block 1c
    modellGewichteKontextMenue();   // AP8
}

void kontextZusatz();

// Ein Viertel: Kindfenster in Groesse seines Inhalts; die oberen stehen auf der Mitte (wachsen nach oben), die linken rechtsbuendig
void quadViertel(int nr, const char* titel, const std::function<void()>& inhalt, ImVec2 ort) {
    const float fs = ImGui::GetFontSize();
    ImGui::SetCursorScreenPos(ort);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{fs * 0.3F, fs * 0.2F});
    char id[16];
    std::snprintf(id, sizeof id, "##quad%d", nr);
    // TEST1411c: die Viertel einer Seite gleich breit (links rechtsbuendig, rechts linksbuendig an der Mitte) - ein Untermenue, das nach aussen
    // aufgeht, liegt so neben der ganzen Seite und nicht ueber dem breiteren Nachbarviertel (pt: Maske und Bild unter tools 2)
    if (const float seite = nr % 2 == 0 ? std::max(g_quad.w[0], g_quad.w[2]) : std::max(g_quad.w[1], g_quad.w[3]); seite > 0.0F) {
        ImGui::SetNextWindowSizeConstraints(ImVec2{seite, 0.0F}, ImVec2{FLT_MAX, FLT_MAX});
    }
    ImGui::BeginChild(id, ImVec2{0.0F, 0.0F}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    if (nr >= 2) { cjkTextDisabled("%s", titel); }   // unten: der Name an der Mitte (oben), wie 3ds Max
    // TEST1411c: wie die Quad-Menues von 3ds Max klappen die Untermenues nach aussen auf - die linken Viertel nach links, die rechten nach
    // rechts (bis dahin rechts vom Viertel: ueber dem Nachbarviertel bzw. ueber dem eigenen Eintrag, ap13k S8 und pt)
    ui::menueRichtung(nr % 2 == 0 ? ImGuiDir_Left : ImGuiDir_Right);
    inhalt();
    ui::menueRichtung(ImGuiDir_None);
    // TEST1411d: wie 3ds Max schliesst ein gewaehlter Eintrag das Quad-Menue. MenuItem/Selectable schliessen nur das Popup-Fenster, in dem
    // sie stehen (imgui_widgets.cpp, Selectable: window->Flags & ImGuiWindowFlags_Popup) - die Viertel sind Kindfenster, das Menue blieb
    // offen und nahm den naechsten Klick daneben nur zum Schliessen (mb5: der Knopf Mini Curve Editor nach Quad > Dope Sheet). Losgelassen
    // ueber einem (nicht gesperrten) Eintrag dieses Viertels und kein Untermenue offen (dessen Eintraege schliessen die Kette selbst, ImGuiWindowFlags_ChildMenu)
    const ImGuiContext& g = *GImGui;
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && ImGui::GetHoveredID() != 0 && !g.HoveredIdIsDisabled &&
        g.OpenPopupStack.Size <= g.BeginPopupStack.Size) {
        ImGui::CloseCurrentPopup();
    }
    if (nr < 2) { cjkTextDisabled("%s", titel); }   // oben: unter den Zeilen, an der Mitte
    ImGui::EndChild();
    g_quad.w[nr] = ImGui::GetItemRectSize().x;
    g_quad.h[nr] = ImGui::GetItemRectSize().y;
}

void drawModellKontextMenue() {
    ModellTab& t = mt();
    const bool blender = modellBlender();
    if (!blender) {
        // TEST1411 J: das Quad-Menue - die vier Viertel treffen sich an der Stelle des Rechtsklicks
        if (ImGui::IsPopupOpen("##mtkontext") && g_quad.gemessen) {
            const float jx = std::max(g_quad.w[0], g_quad.w[2]);
            const float jy = std::max(g_quad.h[0], g_quad.h[1]);
            const ImVec2 pad = ImGui::GetStyle().WindowPadding;
            ImVec2 ort{g_quad.klick.x - jx - pad.x - 1.0F, g_quad.klick.y - jy - pad.y - 1.0F};
            // TEST1411c: wie 3ds Max ganz im Fenster - am Rand rueckt das Quad-Menue nach innen (seine Mitte liegt dann neben der Maus); bis
            // dahin lagen die oberen Viertel bei einem Rechtsklick oben in der Ansicht ausserhalb (pt: y -208)
            const ImGuiViewport* vp = ImGui::GetWindowViewport();
            ort.x = std::clamp(ort.x, vp->WorkPos.x, std::max(vp->WorkPos.x, vp->WorkPos.x + vp->WorkSize.x - g_quad.groesse.x));
            ort.y = std::clamp(ort.y, vp->WorkPos.y, std::max(vp->WorkPos.y, vp->WorkPos.y + vp->WorkSize.y - g_quad.groesse.y));
            ImGui::SetNextWindowPos(ort, ImGuiCond_Always);
        }
        // TEST1411c: die leere Flaeche zwischen den Vierteln gehoert nicht zum Menue (NoMouseInputs - die Viertel sind eigene Kindfenster): ein
        // Untermenue links neben einem schmalen Viertel lag sonst unter dieser Flaeche, die Maus traf das Popup statt des Untermenues (pt)
        t.kontextSichtbar = ui::popup("##mtkontext", ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs);
        if (!t.kontextSichtbar) {
            g_quad.gemessen = false;
            return;
        }
        if (ImGui::IsWindowAppearing()) {
            g_quad.klick = ImGui::GetMousePosOnOpeningCurrentPopup();
            g_quad.gemessen = false;
            for (int k = 0; k < 4; ++k) { g_quad.w[k] = 0.0F; }   // neu messen (der Inhalt haengt vom Bereich ab)
        }
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const float jx = std::max(g_quad.w[0], g_quad.w[2]);
        const float jy = std::max(g_quad.h[0], g_quad.h[1]);
        quadViertel(0, tr(Str::T1QuadTools1), quadTools1, ImVec2{o.x + jx - g_quad.w[0], o.y + jy - g_quad.h[0]});
        quadViertel(1, tr(Str::T1QuadDisplay), quadDisplay, ImVec2{o.x + jx + 2.0F, o.y + jy - g_quad.h[1]});
        quadViertel(2, tr(Str::T1QuadTools2), kontextZusatz, ImVec2{o.x + jx - g_quad.w[2], o.y + jy + 2.0F});
        quadViertel(3, tr(Str::T1QuadTransform), quadTransform, ImVec2{o.x + jx + 2.0F, o.y + jy + 2.0F});
        g_quad.gemessen = true;
        g_quad.groesse = ImGui::GetWindowSize();
        ImGui::EndPopup();
        return;
    }
    t.kontextSichtbar = ui::popup("##mtkontext");
    if (!t.kontextSichtbar) { return; }
    // TEST1411 J: wie Blender "Object > Transform" (G/R/S) - aus dem Rechtsklick erreichbar
    if (ui::menue(tr(Str::T1BlTransform))) {
        if (ImGui::MenuItem(tr(Str::MbBlGreifen))) { blenderModal(1); }
        if (ImGui::MenuItem(tr(Str::MbBlDrehen))) { blenderModal(2); }
        if (ImGui::MenuItem(tr(Str::MbBlSkalieren))) { blenderModal(3); }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    // ENTSCHEID3A (C4): Esc schliesst das Menue wie in 3ds Max und Blender, auch aus einem offenen Untermenue - seit KLEIN2 fuer jedes
    // Menue des Model-Tabs an einer Stelle am Anfang des Bildes (modellMenueEscBildAnfang, ui::menueEsc)
    cjkTextDisabled("%s", tr(Str::MbKontextAuswahl));
    static const Str kArten[6] = {Str::MbMaxObjekt, Str::MbMaxEcke, Str::MbMaxKante, Str::MbMaxPolygon, Str::MbMaxRand, Str::MbMaxElement};
    static const Str kArtenBl[4] = {Str::MbBlObjektModus, Str::MbBlEcke, Str::MbBlKante, Str::MbBlFlaeche};
    for (int a = 0; a < (blender ? 4 : 6); ++a) {
        if (ImGui::MenuItem(tr(blender ? kArtenBl[a] : kArten[a]), nullptr, static_cast<int>(t.auswahlArt) == a)) {
            modellAuswahlArtSetzen(static_cast<AuswahlArt>(a));
        }
    }
    // Weights und Anzeige als Untermenues (wie die Quad-Menues von 3ds Max und die Kontextmenues von Blender): alles in
    // einer Spalte war 1151 Bildpunkte hoch (uiScale 1.5) - ueber den Rand des Hauptfensters hinaus wird ein Popup ein
    // eigenes Fenster, und Platz nur, wenn noetig.
    int gruppe = -1;
    bool unter = false;   // ein Untermenue ist offen
    bool zeigen = true;   // die Zeilen der Gruppe stehen da (Untermenue zu: nein)
    const auto& liste = zusatzAktionen();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const ZusatzAktion& a = liste[i];
        if (a.gruppe != gruppe) {
            if (unter) { ImGui::EndMenu(); }
            unter = false;
            if (gruppe == 0) { modellAnzeigeKontext(); }   // MBKOPF: nach Weights dieselben Menues wie die Kopfzeile der Ansicht
            gruppe = a.gruppe;
            if (gruppe == 0 || gruppe == 4) {
                ImGui::Separator();
                unter = ui::menue(tr(gruppe == 0 ? Str::MbBereichWeights : Str::MbBereichAnimation));
                zeigen = unter;
            } else {
                ImGui::Separator();
                zeigen = true;
            }
        }
        if (!zeigen) { continue; }
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::MenuItem(tr(a.text), a.taste[0] != '\0' ? a.taste : nullptr)) { zusatzAusloesen(a); }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
        ImGui::PopID();
    }
    if (unter) { ImGui::EndMenu(); }
    modellRigKontextMenue();   // Stufe 3
    modellUvKontextMenue();    // Modell-Block 1b
    modellMeshKontextMenue();  // Modell-Block 1d
    modellMalenKontextMenue(); // Modell-Block 1c
    modellGewichteKontextMenue();   // AP8: Gewichts-Werkzeuge, Bereinigen, Spiegeln, Sperren
    modellBlKontextMenue();   // AP4: Werkzeugleiste (T), Seitenleiste (N), Abstand messen
    ImGui::EndPopup();
}

// TEST1411 J: tools 2 des Quad-Menues - Weights, Anzeige, Skript, Animation (die Gruppen der Zusatzaktionen) und die Werkzeugleisten
void kontextZusatz() {
    int gruppe = -1;
    bool unter = false;
    bool zeigen = true;
    const auto& liste = zusatzAktionen();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const ZusatzAktion& a = liste[i];
        if (a.gruppe != gruppe) {
            if (unter) { ImGui::EndMenu(); }
            unter = false;
            if (gruppe == 0) { modellAnzeigeKontext(); }
            gruppe = a.gruppe;
            if (gruppe == 0 || gruppe == 4) {
                unter = ui::menue(tr(gruppe == 0 ? Str::MbBereichWeights : Str::MbBereichAnimation));
                zeigen = unter;
            } else {
                ImGui::Separator();
                zeigen = true;
            }
        }
        if (!zeigen) { continue; }
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::MenuItem(tr(a.text), a.taste[0] != '\0' ? a.taste : nullptr)) { zusatzAusloesen(a); }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
        ImGui::PopID();
    }
    if (unter) { ImGui::EndMenu(); }
    modellBlKontextMenue();   // AP4: Abstand messen, Knochen waehlen
}

namespace {

// FIX612 R4 (Entscheidung B2): die Tabellen der Bereiche Modellieren und Animation kommen in modellTasten vor der Haupttabelle
// (wie 3ds Max im Modellieren Alt+R = Ring, wie Blender in der Animation Alt+R = Drehung zuruecksetzen). Nennt der Text einer
// Zeile der Haupttabelle am Ende in Klammern ihre Taste und nimmt der Bereich diese Taste selbst, steht die Zeile in der Suche
// ohne die Klammer - die Taste loest dort etwas anderes aus. Ueber die Suche bleibt die Zeile ausfuehrbar.
std::string tastenZeile(Str text, ImGuiKeyChord c) {
    std::string s = tr(text);
    if (c == 0 || (!modellMeshHatTaste(c) && !modellRigHatTaste(c))) { return s; }
    const std::string name = ImGui::GetKeyChordName(c);
    const std::size_t k = s.rfind(" (");
    if (k != std::string::npos && !s.empty() && s.back() == ')' && s.find(name, k) != std::string::npos) { s.erase(k); }
    return s;
}

void tasteAusloesen(const MaxTaste& k) {
    k.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tastenZeile(k.text, k.chord);
    g_app->modelDirty = true;
}

}  // namespace

bool modellFokus() {
    return g_app->leftMode == 2 && mt().fokus && !ImGui::GetIO().WantTextInput && !g_app->model.empty();
}

bool modellBlender() { return g_app->settings.modellBedienung == 1; }

bool modellTasten() {
    if (!modellFokus()) { return false; }
    // ENTSCHEID3A (C4) / KLEIN2: Esc bei offenem Menue hat modellMenueEscBildAnfang am Anfang des Bildes verbraucht - hier kommt es
    // nur ohne offenes Menue an (Navigation, Umformung, Werkzeuge)
    if (modellNavTasten()) { return true; }   // MBNAV: Esc beendet einen Navigationsmodus
    if (modellModalTasten()) {   // mb1a2: waehrend G/R/S gehoeren die Tasten der Umformung
        mt().modalEndeBild = ImGui::GetFrameCount();
        return true;
    }
    if (modellRigModalTasten()) {   // Stufe 3: G/R auf einem Griff
        mt().modalEndeBild = ImGui::GetFrameCount();
        return true;
    }
    if (modellKdTasten()) { return true; }     // AP13K: Durchsicht - Alt+X wie 3ds Max, Alt+Z wie Blender
    if (modellZeitTasten()) { return true; }   // AP1: Zeitleiste (N Auto Key, Key-Modus; wie Blender Pfeil hoch/runter, K)
    if (modellKurvenTasten()) { return true; }   // Stufe 3b: Schluessel, Editor-Tasten (Bereich Animation)
    if (modellRigTasten()) { return true; }   // Stufe 3: Pose, Auswahl (Bereich Animation)
    if (modellReiterTasten()) { return true; }   // AP11: Strg+C / Strg+V kopieren und fuegen Flaechen ein (nicht im Bereich Animation)
    if (modellUvTasten()) { return true; }    // Modell-Block 1b: UV-Editor (Bereich UV)
    if (modellMeshTasten()) { return true; }  // Modell-Block 1d: Werkzeuge (Bereich Modellieren)
    if (modellMalenTasten()) { return true; } // Modell-Block 1c: Pinsel (Bereich Painting)
    if (modellGewichteTasten()) { return true; }   // MRESTE: Bereich Weights wie Blender (km_weight_paint)
    if (modellBlender()) {
        if (modellBlTasten()) { return true; }   // AP4: T Werkzeugleiste, N Seitenleiste
        for (const BlenderTaste& k : blenderTasten()) {
            if (ImGui::IsKeyChordPressed(k.chord)) {
                k.tun();
                ++mt().tastenZaehler;
                mt().letzteTaste = tr(k.text);
                g_app->modelDirty = true;
                return true;
            }
        }
        return false;
    }
    for (const MaxTaste& k : maxTasten()) {
        if (ImGui::IsKeyChordPressed(k.chord)) {
            tasteAusloesen(k);
            return true;
        }
    }
    return false;
}

int modellBlenderTasteZahl() { return static_cast<int>(blenderTasten().size()); }
ImGuiKeyChord modellBlenderTasteChord(int i) { return blenderTasten()[static_cast<std::size_t>(i)].chord; }
const char* modellBlenderTasteText(int i) { return tr(blenderTasten()[static_cast<std::size_t>(i)].text); }
const char* modellBlenderTasteQuelle(int i) { return blenderTasten()[static_cast<std::size_t>(i)].quelle; }

int modellTasteZahl() { return static_cast<int>(maxTasten().size()); }

ImGuiKeyChord modellTasteChord(int i) { return maxTasten()[static_cast<std::size_t>(i)].chord; }

const char* modellTasteText(int i) { return tr(maxTasten()[static_cast<std::size_t>(i)].text); }

// --- AP14: die Zeilen der Suche "Alle Aktionen" ------------------------------------------------------------------------------
namespace {

struct AktionZeilen {
    std::unordered_map<std::string, int> zahl;          // sichtbarer Text -> wie oft in diesem Bild
    std::unordered_map<std::string, int> zahlLetztes;   // ... im letzten Bild der Suche
    std::vector<ModellAktionProbe> proben;
    std::vector<ModellAktionProbe> probenLetztes;
};

AktionZeilen& aktionZeilen() {
    static AktionZeilen z;
    return z;
}

// Zu Beginn jeder gezeichneten Liste: was im letzten Bild stand, entscheidet, welche Texte mehrfach vorkommen.
void aktionZeilenBild() {
    AktionZeilen& z = aktionZeilen();
    z.zahlLetztes.swap(z.zahl);
    z.zahl.clear();
    z.probenLetztes.swap(z.proben);
    z.proben.clear();
}

}  // namespace

bool modellAktionZeile(const char* text, int bereich) {
    static const Str kBereich[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv,
                                                  Str::MbBereichWeights, Str::MbBereichPainting,     Str::MbBereichAnimation};
    AktionZeilen& z = aktionZeilen();
    const char* ende = ImGui::FindRenderedTextEnd(text);
    const std::string sicht(text, ende);
    const int hier = ++z.zahl[sicht];
    const auto war = z.zahlLetztes.find(sicht);
    const bool mehrfach = hier > 1 || (war != z.zahlLetztes.end() && war->second > 1);
    const char* zusatz = mehrfach && bereich >= 0 && bereich < kModellBereiche ? tr(kBereich[bereich]) : "";
    const ImGuiStyle& st = ImGui::GetStyle();
    const float zusatzB = *zusatz != 0 ? ImGui::CalcTextSize(zusatz).x : 0.0F;
    const float textB = ImGui::CalcTextSize(text, ende).x;
    // der Zusatz steht rechts in der Zeile; reicht die Breite nicht fuer beide, in einer zweiten Zeile darunter (nichts wird ueberdeckt)
    const bool darunter = zusatzB > 0.0F && textB + ImGui::GetFontSize() + zusatzB > ImGui::GetContentRegionAvail().x;
    const bool geklickt = ImGui::Selectable(text, false, 0, ImVec2{0.0F, darunter ? ImGui::GetTextLineHeight() * 2.0F + st.ItemSpacing.y : 0.0F});
    z.proben.push_back(ModellAktionProbe{ImGui::GetItemID(), sicht, zusatz});
    if (zusatzB > 0.0F) {
        const ImVec2 lo = ImGui::GetItemRectMin();
        const ImVec2 hi = ImGui::GetItemRectMax();
        const float y = darunter ? lo.y + ImGui::GetTextLineHeight() + st.ItemSpacing.y : lo.y;
        ImGui::GetWindowDrawList()->AddText(ImVec2{std::floor(hi.x - zusatzB), y}, ImGui::GetColorU32(ImGuiCol_TextDisabled), zusatz);
    }
    return geklickt;
}

const std::vector<ModellAktionProbe>& modellAktionProben() { return aktionZeilen().probenLetztes; }

// "Alle Aktionen suchen" (X): die Liste der Tasten, zum Ausfuehren anklicken.
void drawModellAktionen() {
    ModellTab& t = mt();
    if (t.aktionenOffen) {
        ImGui::OpenPopup("###mtaktionen");
        t.aktionenOffen = false;
    }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 24.0F, fs * 24.0F}, ImGuiCond_Appearing);
    t.aktionenSichtbar = ui::popup("###mtaktionen");
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
    aktionZeilenBild();   // AP14: gleiche Namen aus mehreren Bereichen bekommen ihren Bereich als Zusatz (modellAktionZeile)
    if (modellBlender()) {   // mb1a2: F3 sucht in der Belegung wie Blender
        std::vector<Str> gezeigt;   // AP14: eine Aktion mit zwei Tasten (Abspielen: Leertaste, Umschalt+Leertaste) steht einmal
        for (const BlenderTaste& k : blenderTasten()) {
            // MBKOPF: die Tasten der Anzeige-Schalter stehen unten einmal - als Schalter mit Haken und Ort (Enter loest sie weiter aus)
            const std::string zeile = tastenZeile(k.text, k.chord);   // FIX612 R4
            if (!passt(zeile, such) || (modellKopfSucheHatTaste(k.chord) && !erster)) { continue; }
            if (std::find(gezeigt.begin(), gezeigt.end(), k.text) != gezeigt.end()) { continue; }
            gezeigt.push_back(k.text);
            if (modellAktionZeile(zeile.c_str()) || erster) {
                k.tun();
                ++t.tastenZaehler;
                t.letzteTaste = zeile;
                g_app->modelDirty = true;
                ImGui::CloseCurrentPopup();
                break;
            }
        }
    } else {
        // FIX612C (P3-9): im Bereich Painting gehoeren [ und ] dem Pinsel (AP9, Entscheidung shank) - das Zoomen steht dort nur mit
        // Strg+= / Strg+- in der Liste
        const bool malen = mt().bereich == ModellBereich::Painting;
        for (const MaxTaste& k : maxTasten()) {
            const std::string zeile = tastenZeile(k.text, k.chord);   // FIX612 R4
            if (!passt(zeile, such) || (modellKopfSucheHatTaste(k.chord) && !erster)) { continue; }
            if (malen && (k.chord == ImGuiKey_LeftBracket || k.chord == ImGuiKey_RightBracket)) { continue; }
            if (modellAktionZeile(zeile.c_str()) || erster) {
                tasteAusloesen(k);
                ImGui::CloseCurrentPopup();
                break;
            }
        }
    }
    bool rigAusgeloest = false;
    for (const ZusatzAktion& a : zusatzAktionen()) {   // mb1a2: auch die Funktionen ohne Taste des Vorbilds
        const std::string text = zusatzText(a);
        if (!passt(text, such)) { continue; }
        if (modellAktionZeile(text.c_str())) {
            zusatzAusloesen(a);
            ImGui::CloseCurrentPopup();
            rigAusgeloest = true;
            break;
        }
        ImGui::SetItemTooltip("%s", tr(a.hinweis));
    }
    if (!rigAusgeloest && modellRigAktionenSuche(such)) {   // Stufe 3
        ImGui::CloseCurrentPopup();
        rigAusgeloest = true;
    }
    if (!rigAusgeloest && modellKurvenAktionenSuche(such)) {   // Stufe 3b
        ImGui::CloseCurrentPopup();
        rigAusgeloest = true;
    }
    if (!rigAusgeloest && modellUvAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // Modell-Block 1b
    if (!rigAusgeloest && modellMeshAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // Modell-Block 1d
    if (!rigAusgeloest && modellMalenAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // Modell-Block 1c
    if (!rigAusgeloest && modellKopfAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }    // MBKOPF: die Anzeige-Schalter
    if (!rigAusgeloest && modellNavAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }     // MBNAV: Navigation ohne eigene Taste
    if (!rigAusgeloest && modellExplorerAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP2: Szenen-Explorer
    if (!rigAusgeloest && modellGewichteAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP8: Werkzeuge des Bereichs Weights
    if (!rigAusgeloest && modellBlAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP4: Werkzeugleiste, Seitenleiste, Messen
    if (!rigAusgeloest && modellPrAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP5: Flaechenliste, Overlays, Gruppen von Ansehen/Pruefen
    if (!rigAusgeloest && modellStapelAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP13S: Modifier-Stapel
    if (!rigAusgeloest && modellKdAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP13K: Durchsicht, Im Vordergrund, Rig-Stil, Anzeigen als
    if (!rigAusgeloest && modellReiterAktionenSuche(such)) { ImGui::CloseCurrentPopup(); }   // AP11: In neuem Tab oeffnen, Reiter schliessen, Kopieren, Einfuegen
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// mb1a2: Strg+Tab wie Blenders Modus-Menue: Objekt, Bearbeiten (Ecken), Weights.
void drawModellModusMenue() {
    ModellTab& t = mt();
    if (t.bereichMenue) {
        ImGui::OpenPopup("###mtmodus");
        t.bereichMenue = false;
    }
    t.modusMenueSichtbar = ui::popup("###mtmodus");
    if (!t.modusMenueSichtbar) { return; }
    if (ImGui::MenuItem(tr(Str::MbBlObjektModus), nullptr, t.auswahlArt == AuswahlArt::Objekt)) {
        t.bereich = ModellBereich::Ansehen;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
    }
    if (ImGui::MenuItem(tr(Str::MbBlEditModus), nullptr, t.auswahlArt != AuswahlArt::Objekt)) { modellAuswahlArtSetzen(t.editArt); }
    if (ImGui::MenuItem(tr(Str::MbBereichWeights), nullptr, t.bereich == ModellBereich::Weights)) {
        t.bereich = ModellBereich::Weights;
        g_app->modelDirty = true;
    }
    ImGui::EndPopup();
}

// mb1a2: der Schalter in den Einstellungen wirkt sofort - wie Blender eine grosse Ansicht, wie Max die vier.
void modellBedienungSetzen(int b) {
    ModellTab& t = mt();
    if (g_app->settings.modellBedienung == b) { return; }
    dock::modusAnordnung(2, g_app->settings.modellBedienung, b);   // AP4: jeder Bedienmodus behaelt seine Fensteranordnung
    g_app->settings.modellBedienung = b;
    modellModalEnde(false);
    modellNavZuruecksetzen();   // MBNAV
    // FIX612 P3-4: erst merkt sich der Bereich seine Wahl fuer wie 3ds Max (bzw. nimmt sie beim Zurueckschalten wieder), dann
    // gilt wie Blender eine Ansicht - auch vor dem ersten Bild des Model-Tabs
    modellPrAnsichtenAbgleich();
    if (b != 0) { t.vierAnsichten = false; }
    // FIX612C (P3-6): das Werkzeug Knochen waehlen der Leiste wie Blender kommt nicht unsichtbar mit nach wie 3ds Max
    if (b == 0 && modellBl().werkzeug == 2) { modellBl().werkzeug = 0; }
    g_app->modelDirty = true;
}


// AP11 (Modelle in Tabs): die je Detailstufe gemerkte Auswahl (FIX612B P2-9) gehoert dem Modell - sie geht mit in sein Fach.
void modellTabTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        std::map<int, StufenWahl> stufenWahl;
    };
    if (!f) { f = std::make_unique<Fach>(); }
    std::swap(stufenWahl(), static_cast<Fach&>(*f).stufenWahl);
}

}  // namespace bhed::gui
