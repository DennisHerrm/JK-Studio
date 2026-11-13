// app_modelltab_ent3a.cpp - ENTSCHEID3A (12.11., verbindliche Entscheidungen von shank B7-B13 und C4 aus
// plaene/entscheidungen_1211.md) im Model-Tab
//
//   B7   wie 3ds Max (Scene Explorer) laesst sich eine eingefrorene Zeile in der Liste waehlen - etwa um die Auswahl aufzutauen
//        (Autodesk-Hilfe "Scene Explorer Right-Click Menus": "Unfreeze Scene Explorer Selection ... as it's not possible to select
//        frozen objects in the viewports"). In der Ansicht geht der Klick weiter hindurch (ENTSCHEID1).
//   B8   wie Blender zeigt die Werkzeugleiste die Werkzeugnamen, wenn sie breiter als 120 Punkte ist (Blender 5.0
//        space_toolsystem_common.py _layout_generator_detect_from_region: region.width * view2d_scale / ui_scale > 120) - die
//        Kante der Leiste laesst sich ziehen; die Breite bleibt in den Einstellungen (modelBlenderToolbarWidth)
//   B11  Auge fuer Edit Poly (je Flaeche: die Flaeche, wie sie geladen wurde) und Skin (die Figur in der Bindepose, die Knochen
//        behalten ihre Pose wie in 3ds Max) - nur die Anzeige: kein Rueckgaengig-Schritt, Modell und Speichern bleiben
//   B12  der Modifier-Stapel in einer Begleitdatei neben der .glm (bhed/modell/stapelbegleit.h)
//   C4   Esc schliesst das Rechtsklick-Menue der Ansicht (wie 3ds Max und Blender)
// B9 (Seitenleiste bei Knochen) steht in gui/app_modelltab_blender.cpp (seiteObjekt), B10 in den Tastenlisten
// (gui/app_modelltab.cpp, gui/app_modelltab_mb2.cpp), B13 in der Gruppe des Stapels (gui/app_modelltab_stapel.cpp).
#include "app_internal.h"

#include "bhed/modell/gewichte.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/stapelbegleit.h"
#include "bhed/sicherschreiben.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

namespace bhed::gui {
namespace {

ModellEnt3a g_e;

ModellTab& mt() { return g_app->mtab; }

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

// Eine Datei auf der Platte (nicht in einem Archiv) - nur dort liegt eine Begleitdatei
bool aufPlatte(const std::string& p) {
    std::error_code ec;
    return !p.empty() && std::filesystem::path(p).is_absolute() && std::filesystem::is_regular_file(p, ec);
}

std::string grundText(modell::BegleitFehler f) {
    switch (f) {
        case modell::BegleitFehler::Alt: return tr(Str::EaBegleitGrundAlt);
        case modell::BegleitFehler::Fassung: return tr(Str::EaBegleitGrundFassung);
        case modell::BegleitFehler::Flaechen: return tr(Str::EaBegleitGrundFlaechen);
        default: break;
    }
    return tr(Str::EaBegleitGrundKaputt);
}

// Punkte wie Blender (region.width / ui_scale): Bildpunkte geteilt durch Bildschirm- und Oberflaechengroesse
float skala() { return std::max(g_app->dpiScale * g_app->uiScale, 0.01F); }

// Blender: ueber 120 Punkte zeigt die Werkzeugleiste die Namen
constexpr float kNamenAb = 120.0F;

// B8: das Ziehen der Kante - Breite und Maus beim Anfassen (so springt die Leiste beim Anfassen nicht), nach einem Doppelklick
// zaehlt das Halten der zweiten Taste nicht als Ziehen
float g_kanteStartBreite = 0.0F;
float g_kanteStartMaus = 0.0F;
bool g_kanteDoppel = false;

}  // namespace

ModellEnt3a& modellEnt3a() { return g_e; }

// --- B12: Begleitdatei ------------------------------------------------------------------------------------------------------

void modellEnt3aVorLaden(const std::string& glmBytes, modell::Inhalt& in) {
    g_e.begleitPfad.clear();
    g_e.begleitEintraege = -1;
    g_e.begleitFehler = 0;
    g_e.meldungNachLaden.clear();
    g_e.drehModus.clear();   // E3: ein neues Modell beginnt mit Blenders Vorgabe, die Begleitdatei bringt seine Drehmodi
    const std::string pfad = g_app->modelPath;
    if (!aufPlatte(pfad) || (in.herkunft != modell::Herkunft::Glm && in.herkunft != modell::Herkunft::Md3)) { return; }
    const std::string begleit = modell::stapelBegleitName(pfad);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(begleit, ec)) { return; }
    g_e.begleitPfad = begleit;
    modell::BegleitDaten d;
    modell::BegleitFehler f = modell::stapelBegleitLesen(slurp(begleit), glmBytes, d);
    if (f == modell::BegleitFehler::Keiner) { f = modell::stapelBegleitAnwenden(d, in); }
    if (f != modell::BegleitFehler::Keiner) {
        // das Modell laedt trotzdem - ohne Stapel, mit Meldung
        g_e.begleitFehler = static_cast<int>(f);
        g_e.meldungNachLaden = format(Str::EaBegleitFehler, fileName(begleit).c_str(), grundText(f).c_str());
        diag::detail("ENT3A Begleitdatei " + begleit + ": nicht gelesen (" + std::to_string(static_cast<int>(f)) + ")");
        return;
    }
    g_e.begleitEintraege = d.eintraege();
    for (const auto& [knochen, modus] : d.drehModi) { g_e.drehModus[knochen] = modus; }
    g_e.meldungNachLaden = format(Str::EaBegleitGelesen, fileName(begleit).c_str(), d.eintraege());
    diag::detail("ENT3A Begleitdatei " + begleit + ": " + std::to_string(d.eintraege()) + " Eintraege" + (d.basisGlm.empty() ? "" : ", mit Basis"));
}

void modellEnt3aNachLaden() {
    const ModellTab& t = mt();
    g_e.geladen = std::make_shared<const modell::Inhalt>(t.dok.inhalt());
    g_e.geladenFuer = t.dok.ladeStand();
    g_e.polyAus.clear();
    g_e.skinAus = false;   // ein neues Modell zeigt seine Haut wie geladen
    modellStapel().zeichnungFuer = -1;
    if (!g_e.meldungNachLaden.empty()) {
        melden(g_e.meldungNachLaden);
        g_e.meldungNachLaden.clear();
    }
}

std::string modellEnt3aBegleitSchreiben(const std::string& ziel, const std::string& glmBytes) {
    const modell::Inhalt& in = mt().dok.inhalt();
    const std::string pfad = modell::stapelBegleitName(ziel);
    std::error_code ec;
    // E3: die Drehmodi der Knochen (nur wer nicht Quaternion hat; ein .md3-Requisit hat keine Knochen)
    std::vector<std::pair<std::string, int>> dreh;
    if (in.herkunft == modell::Herkunft::Glm) {
        for (const auto& [knochen, modus] : g_e.drehModus) {
            if (modus != 0) { dreh.emplace_back(knochen, modus); }
        }
    }
    if (modell::stapelLeer(in) && dreh.empty()) {
        // eine alte eigene Begleitdatei gehoert nicht mehr zu dieser .glm (sonst kaeme beim Oeffnen ein Stapel, den es nicht mehr gibt)
        if (std::filesystem::is_regular_file(pfad, ec) && slurp(pfad).rfind("JKSTUDIO-STACK ", 0) == 0) {
            std::filesystem::remove(pfad, ec);
            ++g_e.begleitEntfernt;
        }
        return {};
    }
    std::string basis;
    const bool mitBasis = modell::stapelWirkt(in, modell::StapelZweck::Datei);
    if (mitBasis) {
        // die Flaechen unter dem Stapel, wie die Ansicht sie zeigt (offene Mallisten uebernommen wie beim Speichern, FIX612B P2-2)
        modell::Inhalt ohne = modell::malt(in) ? modell::wieGezeigt(in) : in;
        for (modell::Teil& x : ohne.teile) { x.stapel.clear(); }
        // E3: ein .md3-Requisit bekommt die .md3 ohne Stapel als Basis (wie Speichern sie schreibt, mit derselben Grenze)
        const bool ok = in.herkunft == modell::Herkunft::Md3
                            ? modell::md3Schreiben(ohne, basis, modell::grenzen(static_cast<modell::Zielspiel>(mt().zielspiel)).eckenJeFlaeche)
                            : modell::glmSchreiben(ohne, basis);
        if (!ok) { return format(Str::EaBegleitSchreibFehler, in.herkunft == modell::Herkunft::Md3 ? "md3" : "glm"); }
    }
    std::string fehler;
    if (!sicherSchreiben(pfad, modell::stapelBegleitSchreiben(in, glmBytes, mitBasis ? &basis : nullptr, dreh.empty() ? nullptr : &dreh), &fehler)) {
        return format(Str::EaBegleitSchreibFehler, fehler.c_str());
    }
    ++g_e.begleitGeschrieben;
    diag::detail("ENT3A Begleitdatei geschrieben: " + pfad + (mitBasis ? " (mit Basis)" : ""));
    return format(Str::EaBegleitGeschrieben, fileName(pfad).c_str());
}

// --- B11: Auge fuer Edit Poly und Skin --------------------------------------------------------------------------------------

bool modellEnt3aSkinAus() {
    return g_e.skinAus && !modellBlender() && mt().dok.inhalt().herkunft == modell::Herkunft::Glm && !g_app->anim.bones.empty();
}

const modell::Teil* modellEnt3aGeladen(int teil) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (!g_e.geladen || g_e.geladenFuer != t.dok.ladeStand() || teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size()) { return nullptr; }
    const std::vector<modell::Teil>& alt = g_e.geladen->teile;
    const std::string& name = in.teile[static_cast<std::size_t>(teil)].name;
    if (static_cast<std::size_t>(teil) < alt.size() && alt[static_cast<std::size_t>(teil)].name == name) { return &alt[static_cast<std::size_t>(teil)]; }
    // umgeordnet (eine Flaeche kam dazu oder ging): derselbe Name, wenn er eindeutig ist
    const modell::Teil* gefunden = nullptr;
    for (const modell::Teil& x : alt) {
        if (x.name != name) { continue; }
        if (gefunden != nullptr) { return nullptr; }
        gefunden = &x;
    }
    return gefunden;
}

bool modellEnt3aPolyAus(int teil) {
    return !modellBlender() && teil >= 0 && static_cast<std::size_t>(teil) < g_e.polyAus.size() && g_e.polyAus[static_cast<std::size_t>(teil)] != 0 &&
           modellEnt3aGeladen(teil) != nullptr;
}

void modellEnt3aPolySchalten(int teil) {
    const modell::Inhalt& in = mt().dok.inhalt();
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size() || modellEnt3aGeladen(teil) == nullptr) { return; }
    if (g_e.polyAus.size() < in.teile.size()) { g_e.polyAus.resize(in.teile.size(), 0); }
    char& aus = g_e.polyAus[static_cast<std::size_t>(teil)];
    aus = aus != 0 ? 0 : 1;
    melden(format(aus != 0 ? Str::EaMeldPolyAus : Str::EaMeldPolyAn, in.teile[static_cast<std::size_t>(teil)].name.c_str()));
    ++g_e.ansichtStand;   // modelDirty allein reicht nicht: es geht am Ende des Bilds aus, das Feld steht hinter der Ansicht
    modellStapel().zeichnungFuer = -1;   // die Zeichnung neu (die Flaeche kommt aus dem geladenen Netz bzw. wieder aus dem Dokument)
    g_app->modelDirty = true;
}

void modellEnt3aSkinSchalten() {
    g_e.skinAus = !g_e.skinAus;
    ++g_e.ansichtStand;
    melden(tr(g_e.skinAus ? Str::EaMeldSkinAus : Str::EaMeldSkinAn));
    g_app->modelDirty = true;   // posieren() rechnet die Knochenmatrizen der Haut neu
}

// --- B7: eingefrorene Zeile im Explorer ---------------------------------------------------------------------------------------

bool modellEnt3aFrostInListeWaehlbar() { return !modellBlender(); }

namespace {
// Gewaehltes, das eingefroren ist (Flaechen, der Tag, der Knochen)
int gewaehltEingefroren() {
    const ModellTab& t = mt();
    int n = 0;
    for (const int i : t.auswahl) { n += modellTeilEingefroren(i) ? 1 : 0; }
    n += modellTeilEingefroren(t.tagWahl) ? 1 : 0;
    n += modellKnochenEingefroren(t.knochenWahl) ? 1 : 0;
    return n;
}
}  // namespace

void modellEnt3aAuswahlAuftauen() {
    const ModellTab& t = mt();
    const int n = gewaehltEingefroren();
    if (n == 0) {
        melden(tr(Str::EaMeldNichtsGefroren));
        return;
    }
    const std::vector<int> wahl = t.auswahl;
    for (const int i : wahl) {
        if (modellTeilEingefroren(i)) { modellTeilEinfrieren(i, false, false); }
    }
    if (modellTeilEingefroren(t.tagWahl)) { modellTeilEinfrieren(t.tagWahl, false, false); }
    if (modellKnochenEingefroren(t.knochenWahl)) { modellKnochenEinfrieren(t.knochenWahl, false, false); }
    melden(format(Str::EaMeldAufgetaut, n));
    g_app->modelDirty = true;
}

void modellEnt3aAuftauenPunkt() {
    if (modellBlender() || gewaehltEingefroren() == 0) { return; }
    if (ImGui::MenuItem(tr(Str::EaAuswahlAuftauen))) { modellEnt3aAuswahlAuftauen(); }
    ImGui::SetItemTooltip("%s", tr(Str::EaAuswahlAuftauenHint));
}

// --- B8: Werkzeugleiste breiter ziehen ------------------------------------------------------------------------------------------

float modellEnt3aLeisteBreite() {
    const float pt = g_app->settings.modellBlLeisteBreite;
    if (pt <= 0.0F) { return 0.0F; }
    return pt * skala() / std::max(ImGui::GetFontSize(), 1.0F);
}

bool modellEnt3aLeisteNamen() { return g_app->settings.modellBlLeisteBreite > kNamenAb; }

void modellEnt3aLeisteKante(float schmal) {
    // im Fenster der Leiste VOR den Knoepfen: wo sie sich ueberdecken, gehoert die Maus der Kante (ImGui gibt das Schweben dem
    // ersten Element, das es beansprucht)
    const float fs = ImGui::GetFontSize();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 gr = ImGui::GetWindowSize();
    const float b = std::max(std::floor(fs * 0.35F), 3.0F);
    const ImVec2 vorher = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2{pos.x + gr.x - b, pos.y});
    ImGui::InvisibleButton("##ent3aleistekante", ImVec2{b, std::max(gr.y, 1.0F)});
    const bool ueber = ImGui::IsItemHovered();
    const bool zieht = ImGui::IsItemActive();
    g_e.leisteKante = ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
    g_e.leisteZieht = zieht;
    if (ueber || zieht) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        const float x = std::floor(pos.x + gr.x - 1.0F);
        ImGui::GetWindowDrawList()->AddLine(ImVec2{x, pos.y}, ImVec2{x, pos.y + gr.y}, ImGui::GetColorU32(zieht ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered), 2.0F);
    }
    if (ueber && !zieht) { ImGui::SetItemTooltip("%s", tr(Str::EaLeisteKanteHint)); }
    const float maus = ImGui::GetIO().MousePos.x;
    if (ImGui::IsItemActivated()) {
        // die eingestellte Breite der Spalte (nicht die des Kindfensters, das um den Zellrand schmaler ist)
        g_kanteStartBreite = std::max(modellEnt3aLeisteBreite(), schmal) * fs;
        g_kanteStartMaus = maus;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { g_kanteDoppel = false; }
    if (ueber && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        g_app->settings.modellBlLeisteBreite = 0.0F;   // zurueck zur schmalen Leiste
        g_kanteDoppel = true;
    } else if (zieht && !g_kanteDoppel && std::fabs(maus - g_kanteStartMaus) >= 1.0F) {
        // die neue Breite: die beim Anfassen plus der Weg der Maus, nicht schmaler als die Vorgabe, hoechstens ein Drittel des Fensters
        const float neu = std::clamp(g_kanteStartBreite + maus - g_kanteStartMaus, schmal * fs, std::max(ImGui::GetIO().DisplaySize.x / 3.0F, schmal * fs));
        g_app->settings.modellBlLeisteBreite = neu <= schmal * fs + 0.5F ? 0.0F : neu / skala();
    }
    ImGui::SetCursorScreenPos(vorher);
}

// --- C4: Esc schliesst das Rechtsklick-Menue der Ansicht -------------------------------------------------------------------------

bool modellEnt3aKontextEsc() {
    if (!mt().kontextSichtbar || !ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { return false; }
    // verbraucht: Esc gilt dem Menue (es schliesst in drawModellKontextMenue), nicht einem Werkzeug, der Navigation oder der Auswahl
    return true;
}


// AP11 (Modelle in Tabs): die Augen von Edit Poly und Skin, der Inhalt wie geladen, die Begleitdatei und der Drehmodus je Knochen
// gehoeren dem Modell. Die Breite der Werkzeugleiste und die Zaehler fuer den Selbsttest gelten fuer alle Reiter.
void modellEnt3aTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        std::vector<char> polyAus;
        bool skinAus = false;
        std::shared_ptr<const modell::Inhalt> geladen;
        int geladenFuer = -1;
        std::string begleitPfad;
        int begleitEintraege = -1;
        int begleitFehler = 0;
        std::string meldungNachLaden;
        std::map<std::string, int> drehModus;
    };
    if (!f) { f = std::make_unique<Fach>(); }
    Fach& x = static_cast<Fach&>(*f);
    std::swap(g_e.polyAus, x.polyAus);
    std::swap(g_e.skinAus, x.skinAus);
    std::swap(g_e.geladen, x.geladen);
    std::swap(g_e.geladenFuer, x.geladenFuer);
    std::swap(g_e.begleitPfad, x.begleitPfad);
    std::swap(g_e.begleitEintraege, x.begleitEintraege);
    std::swap(g_e.begleitFehler, x.begleitFehler);
    std::swap(g_e.meldungNachLaden, x.meldungNachLaden);
    std::swap(g_e.drehModus, x.drehModus);
    ++g_e.ansichtStand;   // die Ansicht posiert und zeichnet neu
    g_e.leisteZieht = false;
}

}  // namespace bhed::gui
