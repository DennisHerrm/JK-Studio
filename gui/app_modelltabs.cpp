// app_modelltabs.cpp - AP11 (MBENTWURF Teil B, 13.11.): mehrere Modelle in Reitern ueber der Ansicht, Kopieren und Einfuegen von
// Flaechen zwischen den Modellen (Abgleich R5, X19, P1; shank 04.11.: "Kopieren/Einfuegen, Modelle in Tabs")
//
// Entwurf (plaene/modellblock_stufe0/entwurf_modelltab.html, Bilder e_max_pruefen, e_blender_pruefen): ueber der Ansicht je
// geoeffnetem Modell ein Reiter ("kyle/model.glm"), x schliesst, "+" oeffnet (Strg+O); rechts der Hinweis "Ablage: <Flaeche> aus
// <Modell> - Strg+V fuegt ein"; im Rechtsklick einer Flaeche Kopieren (Strg+C) und "Einfuegen als Kind hiervon" (Strg+V).
//
// Der Arbeitsstand: das aktive Modell steht wie bisher in App (model, modelPath, anim, mtab ...) und in den Zustaenden der Module,
// alles andere Code des Model-Tabs bleibt unveraendert. Jeder andere Reiter haelt seinen Teil in einem AppTeil und einem Fach je
// Modul (gui/modelltab.h, ModellFach). Ein Wechsel tauscht beides mit dem Arbeitsstand - std::swap, nichts wird kopiert, nichts neu
// geladen: Datei, GLA und Animation, Auswahl, Rueckgaengig-Verlauf, Modifier-Stapel und Begleitdatei, Rig und Control Rig, Zeitleiste
// und Ansichten bleiben je Modell. Danach rechnet jeder Zwischenspeicher, der am alten Modell hing, neu (die Staende der Dokumente
// zaehlen seit AP11 fuer alle gemeinsam, bhed/modell/dokument.cpp; was sich etwas nach Adresse merkt, verwirft es im Tausch).
//
// Gewechselt, geschlossen und in einem neuen Reiter geladen wird nur am Anfang eines Bilds (modellReiterSchritt aus gui::draw) -
// mitten in einem Bild haetten Zeichner und Ereignisse noch Indizes und Verweise auf das alte Modell in der Hand (die Codepruefung
// fand diese Fehlerart mehrfach: R2, P2-3, P3-1).
#include "app_internal.h"
#include "ui_menue.h"   // KLEIN2: ui::menue/popup - Menues bleiben im Fenster

#include "bhed/modell/ablage.h"

#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

std::string format(Str s, auto... a) {
    char b[1200];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

// --- Der Teil von App, der zu einem Modell gehoert ---------------------------------------------------------------------------------
// Dieselben Felder mit denselben Vorgaben wie in App (app_internal.h): ein neuer Reiter beginnt wie das Programm nach dem Start.
struct AppTeil {
    GlmModel model;
    std::string modelPath;
    Camera modelCam;
    Orbit modelOrbit;
    bool modelDirty = true;
    bool showCaps = false;
    GlaAnimation anim;
    std::vector<AnimEntry> animList;
    int animIndex = -1;
    int animFrame = 0;
    bool animPlaying = false;
    float animTime = 0.0F;
    char animFilter[48] = {};
    std::string animText;
    int loopMode = 0;
    float animSpeed = 1.0F;
    ModelTextures modelTextures;
    std::vector<std::string> modelSkins;
    int modelSkinIndex = -1;
    std::string modelDir;
    std::string npcGeladen;
    std::vector<BoneMatrix> modellWelt;
    ModellTab mtab;
};

void appTauschen(AppTeil& p) {
    App& a = *g_app;
    using std::swap;
    swap(a.model, p.model);
    swap(a.modelPath, p.modelPath);
    swap(a.modelCam, p.modelCam);
    swap(a.modelOrbit, p.modelOrbit);
    swap(a.modelDirty, p.modelDirty);
    swap(a.showCaps, p.showCaps);
    swap(a.anim, p.anim);
    swap(a.animList, p.animList);
    swap(a.animIndex, p.animIndex);
    swap(a.animFrame, p.animFrame);
    swap(a.animPlaying, p.animPlaying);
    swap(a.animTime, p.animTime);
    swap(a.animFilter, p.animFilter);
    swap(a.animText, p.animText);
    swap(a.loopMode, p.loopMode);
    swap(a.animSpeed, p.animSpeed);
    swap(a.modelTextures, p.modelTextures);
    swap(a.modelSkins, p.modelSkins);
    swap(a.modelSkinIndex, p.modelSkinIndex);
    swap(a.modelDir, p.modelDir);
    swap(a.npcGeladen, p.npcGeladen);
    swap(a.modellWelt, p.modellWelt);
    swap(a.mtab, p.mtab);
}

// Die Module mit Zustand je Modell (gui/modelltab.h); die Reihenfolge ist fest, das Fach i gehoert immer dem Modul i.
using Tausch = void (*)(ModellFachZeiger&);
constexpr Tausch kTausch[] = {&modellTabTauschen,     &modellAnsichtTauschen, &modellExplorerTauschen, &modellMalenTauschen,
                              &modellGewichteTauschen, &modellStapelTauschen,  &modellEnt3aTauschen,    &modellAutoSkinTauschen,
                              &modellZeitTauschen,     &modellNavTauschen,     &modellPruefenTauschen,  &modellRetargetTauschen,
                              &modellNebenTauschen};
constexpr std::size_t kFaecher = sizeof kTausch / sizeof kTausch[0];

struct Reiter {
    int kennung = 0;
    std::unique_ptr<AppTeil> app;                 // nullptr, solange der Reiter aktiv ist (sein Stand steht in App)
    std::array<ModellFachZeiger, kFaecher> faecher;
    // beim Wegtauschen festgehalten (ein abgelegter Reiter aendert sich nicht)
    bool ungespeichert = false;
    bool leer = true;
    std::string pfad;
    std::string name;   // Name des Dokuments (ohne Pfad: neues Requisit)
};

std::vector<Reiter> g_reiter;
int g_aktiv = 0;
int g_naechsteKennung = 1;
int g_stand = 0;
ModellReiterProbe g_probe;
// So viele Bilder lang waehlt die Leiste den aktiven Reiter selbst (nach einem Wechsel oder Schliessen aus dem Programm): ImGui
// uebernimmt eine Wahl erst im Bild danach und meldet bis dahin noch den vorigen Reiter als sichtbar.
int g_auswahlBilder = 2;
bool g_naechstesNeu = false;      // der pk3-Browser wurde ueber "In neuem Tab oeffnen" geoeffnet

enum class Art { Wechseln, Schliessen, SchliessenOhneFrage, AndereSchliessen, Laden, Dialog };
struct Auftrag {
    Art art = Art::Wechseln;
    int kennung = 0;
    std::string pfad;
    std::function<void()> laden;
};
std::vector<Auftrag> g_auftraege;

modell::FlaechenAblage g_ablage;

void sicherstellen() {
    if (!g_reiter.empty()) { return; }
    Reiter r;
    r.kennung = g_naechsteKennung++;
    g_reiter.push_back(std::move(r));
    g_aktiv = 0;
}

int indexVon(int kennung) {
    for (std::size_t i = 0; i < g_reiter.size(); ++i) {
        if (g_reiter[i].kennung == kennung) { return static_cast<int>(i); }
    }
    return -1;
}

bool aktivLeer() { return g_app->model.empty() && mt().dok.inhalt().teile.empty(); }

// Den laufenden Stand des aktiven Reiters festhalten (Titel, Frage beim Schliessen).
void aktivFesthalten() {
    Reiter& r = g_reiter[static_cast<std::size_t>(g_aktiv)];
    r.ungespeichert = modellUngespeichert();
    r.leer = aktivLeer();
    r.pfad = g_app->modelPath;
    r.name = mt().dok.inhalt().name;
}

// Was mitten in einem Zug offen ist, gehoert dem alten Modell: Strich, Verlauf und Gizmo-Zug uebernehmen (wie Loslassen),
// Werkzeugkaestchen, G/R/S, Regler und Griff-Zuege abbrechen (wie Esc).
void offenesBeenden() {
    (void)modellAnsichtOffenesAbbrechen(false);
    (void)modellOffenesAbbrechen();
    mt().griff = 0;
    mt().ziehVorher.clear();
    mt().rahmen = false;
    mt().kontextOeffnen = false;
}

void wechselnZu(int neu) {
    sicherstellen();
    if (neu < 0 || neu >= static_cast<int>(g_reiter.size()) || neu == g_aktiv) { return; }
    offenesBeenden();
    aktivFesthalten();
    Reiter& z = g_reiter[static_cast<std::size_t>(neu)];
    if (!z.app) { z.app = std::make_unique<AppTeil>(); }
    appTauschen(*z.app);
    for (std::size_t i = 0; i < kFaecher; ++i) { kTausch[i](z.faecher[i]); }
    // z haelt jetzt den alten Stand: er gehoert dem bisher aktiven Reiter
    Reiter& alt = g_reiter[static_cast<std::size_t>(g_aktiv)];
    alt.app = std::move(z.app);
    for (std::size_t i = 0; i < kFaecher; ++i) { alt.faecher[i] = std::move(z.faecher[i]); }
    z.app.reset();
    g_aktiv = neu;
    ++g_stand;
    ++g_probe.wechsel;
    g_auswahlBilder = 2;
    g_app->modelDirty = true;
    modellAnsichtNachWechsel();
    diag::detail("Model-Tab: Reiter " + std::to_string(neu + 1) + " von " + std::to_string(g_reiter.size()) + " (" +
                 (g_app->modelPath.empty() ? std::string("ohne Modell") : g_app->modelPath) + ")");
}

int neuerReiter() {
    sicherstellen();
    Reiter r;
    r.kennung = g_naechsteKennung++;
    r.app = std::make_unique<AppTeil>();
    g_reiter.push_back(std::move(r));
    const int i = static_cast<int>(g_reiter.size()) - 1;
    wechselnZu(i);
    return i;
}

void entfernen(int i) {
    // nur abgelegte Reiter: der aktive steht in App
    if (i < 0 || i >= static_cast<int>(g_reiter.size()) || i == g_aktiv) { return; }
    g_reiter.erase(g_reiter.begin() + i);
    if (g_aktiv > i) { --g_aktiv; }
    ++g_stand;
    ++g_probe.geschlossen;
    g_auswahlBilder = 2;
}

void schliessenJetzt(int i) {
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    const int kennung = g_reiter[static_cast<std::size_t>(i)].kennung;
    if (g_reiter.size() == 1U) {
        // der letzte Reiter verschwindet nicht: an seine Stelle tritt ein leerer (wie nach dem Start)
        (void)neuerReiter();
        entfernen(indexVon(kennung));
        return;
    }
    if (i == g_aktiv) { wechselnZu(i + 1 < static_cast<int>(g_reiter.size()) ? i + 1 : i - 1); }
    entfernen(indexVon(kennung));
}

std::string titelVon(const std::string& pfad, const std::string& name) { return modellReiterTitel(pfad, name); }

void schliessen(int i, bool ohneFrage) {
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    if (i == g_aktiv) { aktivFesthalten(); }
    const Reiter& r = g_reiter[static_cast<std::size_t>(i)];
    if (ohneFrage || !r.ungespeichert) {
        schliessenJetzt(i);
        return;
    }
    if (g_app->frageOffen) {
        // eine andere Frage ist offen ("Andere Reiter schliessen": die naechste nach der Antwort)
        g_auftraege.push_back(Auftrag{Art::Schliessen, r.kennung, {}, {}});
        return;
    }
    // wie heute (pfix H3): man sieht, worueber man entscheidet
    wechselnZu(i);
    const int kennung = r.kennung;
    g_app->leftMode = 2;
    frage(format(Str::MxSchliessenFrage, titelVon(g_app->modelPath, mt().dok.inhalt().name).c_str()),
          [kennung] { g_auftraege.push_back(Auftrag{Art::SchliessenOhneFrage, kennung, {}, {}}); }, {});
}

int offenIn(const std::string& pfad) {
    if (pfad.empty()) { return -1; }
    for (std::size_t i = 0; i < g_reiter.size(); ++i) {
        const std::string& p = static_cast<int>(i) == g_aktiv ? g_app->modelPath : g_reiter[i].pfad;
        if (p == pfad) { return static_cast<int>(i); }
    }
    return -1;
}

void ladenInNeuem(const std::string& pfad, const std::function<void()>& laden) {
    const int da = offenIn(pfad);
    if (da >= 0) {
        wechselnZu(da);
        melden(format(Str::MxSchonOffen, titelVon(pfad, {}).c_str()));
        return;
    }
    aktivFesthalten();
    const bool hier = aktivLeer() && !modellUngespeichert();
    const int vorher = g_aktiv;
    const int vorherKennung = g_reiter[static_cast<std::size_t>(vorher)].kennung;
    if (!hier) { (void)neuerReiter(); }
    const int kennung = g_reiter[static_cast<std::size_t>(g_aktiv)].kennung;
    if (laden) { laden(); }
    if (!hier && aktivLeer()) {
        // Fehler oder abgebrochen: der neue Reiter geht wieder, der vorige ist wieder aktiv
        wechselnZu(indexVon(vorherKennung));
        entfernen(indexVon(kennung));
    }
}

void andereSchliessen(int i) {
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    const int bleibt = g_reiter[static_cast<std::size_t>(i)].kennung;
    std::vector<int> weg;
    for (const Reiter& r : g_reiter) {
        if (r.kennung != bleibt) { weg.push_back(r.kennung); }
    }
    // jeder fuer sich (ungespeichert: die Frage je Modell); die Fragen kommen nacheinander
    for (const int k : weg) { g_auftraege.push_back(Auftrag{Art::Schliessen, k, {}, {}}); }
}

// --- Ablage -----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> knochenNamen(const GlaAnimation& a) {
    std::vector<std::string> n;
    n.reserve(a.bones.size());
    for (const GlaBone& b : a.bones) { n.push_back(b.name); }
    return n;
}

std::string ablageWas() {
    if (g_ablage.leer()) { return {}; }
    std::string was = g_ablage.teile.front().name;
    if (g_ablage.teile.size() > 1U) { was = format(Str::MxAblageMehr, was.c_str(), static_cast<int>(g_ablage.teile.size()) - 1); }
    return was;
}

std::string liste(const std::vector<std::string>& z) {
    std::string s;
    for (std::size_t i = 0; i < z.size(); ++i) {
        if (i > 0) { s += ", "; }
        if (i == 8 && z.size() > 9U) {
            s += "...";
            break;
        }
        s += z[i];
    }
    return s;
}

}  // namespace

// --- Reiter -------------------------------------------------------------------------------------------------------------------------

// Die Module, die nur aus dem Modell abgeleitete Zwischenspeicher halten: kein Fach, sie rechnen neu.
void modellNebenTauschen(ModellFachZeiger& /*f*/) {
    modellAnimWechsel();
    modellBlWechsel();
    modellKdWechsel();
    modellKurvenWechsel();
    modellMeshWechsel();
}

std::string modellReiterTitel(const std::string& pfad, const std::string& name) {
    if (pfad.empty()) { return name.empty() ? std::string(tr(Str::MxKeinModell)) : name; }
    std::string p = pfad;
    for (char& c : p) {
        if (c == '\\') { c = '/'; }
    }
    const std::size_t s = p.rfind('/');
    if (s == std::string::npos) { return p; }
    const std::size_t s2 = s > 0 ? p.rfind('/', s - 1) : std::string::npos;
    return s2 == std::string::npos ? p : p.substr(s2 + 1);
}

int modellReiterStand() { return g_stand; }

int modellReiterZahl() {
    sicherstellen();
    return static_cast<int>(g_reiter.size());
}

int modellReiterAktiv() {
    sicherstellen();
    return g_aktiv;
}

ModellReiterInfo modellReiterInfo(int i) {
    sicherstellen();
    ModellReiterInfo x;
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return x; }
    const Reiter& r = g_reiter[static_cast<std::size_t>(i)];
    x.kennung = r.kennung;
    x.aktiv = i == g_aktiv;
    if (x.aktiv) {
        x.pfad = g_app->modelPath;
        x.titel = titelVon(g_app->modelPath, mt().dok.inhalt().name);
        x.ungespeichert = modellUngespeichert();
        x.leer = aktivLeer();
    } else {
        x.pfad = r.pfad;
        x.titel = titelVon(r.pfad, r.name);
        x.ungespeichert = r.ungespeichert;
        x.leer = r.leer;
    }
    return x;
}

void modellReiterWechseln(int i) {
    sicherstellen();
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    g_auftraege.push_back(Auftrag{Art::Wechseln, g_reiter[static_cast<std::size_t>(i)].kennung, {}, {}});
}

void modellReiterSchliessen(int i) {
    sicherstellen();
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    g_auftraege.push_back(Auftrag{Art::Schliessen, g_reiter[static_cast<std::size_t>(i)].kennung, {}, {}});
}

void modellReiterAndereSchliessen(int i) {
    sicherstellen();
    if (i < 0 || i >= static_cast<int>(g_reiter.size())) { return; }
    g_auftraege.push_back(Auftrag{Art::AndereSchliessen, g_reiter[static_cast<std::size_t>(i)].kennung, {}, {}});
}

void modellInNeuemReiter(const std::string& pfad, std::function<void()> laden) {
    g_auftraege.push_back(Auftrag{Art::Laden, 0, pfad, std::move(laden)});
}

void modellInNeuemReiterOeffnen() { g_auftraege.push_back(Auftrag{Art::Dialog, 0, {}, {}}); }

void modellInNeuemReiterAusArchiv(const std::string& name) {
    modellInNeuemReiter(name, [name] {
        std::string fehler;
        if (!oeffneModellAusArchiv(name, &fehler)) { platform::showError(name + "\n" + fehler, tr(Str::AppTitle)); }
    });
}

bool modellNaechstesInNeuemReiter() { return g_naechstesNeu && g_app->pk3Open; }

void modellOeffnenWie(const std::string& pfad, std::function<void()> laden) {
    sicherstellen();
    if (modellNaechstesInNeuemReiter()) {
        modellInNeuemReiter(pfad, std::move(laden));
        return;
    }
    const int da = offenIn(pfad);
    if (da >= 0 && da != g_aktiv) {
        modellReiterWechseln(da);
        melden(format(Str::MxSchonOffen, titelVon(pfad, {}).c_str()));
        return;
    }
    modellVerwerfenDann(std::move(laden));   // pfix H3: wie bisher in den aktiven Reiter
}

void modellReiterSchritt() {
    sicherstellen();
    if (!g_app->pk3Open) { g_naechstesNeu = false; }
    if (g_auftraege.empty()) { return; }
    std::vector<Auftrag> jetzt;
    jetzt.swap(g_auftraege);
    // ein offenes Menue oder Fenster zeigt sonst noch Zeilen des alten Modells (Modale - Fragen - bleiben)
    if (ImGui::GetCurrentContext() != nullptr) { ImGui::ClosePopupsExceptModals(); }
    for (Auftrag& a : jetzt) {
        switch (a.art) {
            case Art::Wechseln: wechselnZu(indexVon(a.kennung)); break;
            case Art::Schliessen: schliessen(indexVon(a.kennung), false); break;
            case Art::SchliessenOhneFrage: schliessen(indexVon(a.kennung), true); break;
            case Art::AndereSchliessen: andereSchliessen(indexVon(a.kennung)); break;
            case Art::Laden: ladenInNeuem(a.pfad, a.laden); break;
            case Art::Dialog: {
                const std::string p = platform::openFileDialog(tr(Str::MxNeuerTab), "Ghoul2 models (*.glm)|*.glm|MD3 models (*.md3)|*.md3|All files (*.*)|*.*",
                                                               g_app->settings.lastDir);
                if (!p.empty()) { ladenInNeuem(p, [p] { modellVonPlatteOeffnen(p); }); }
                break;
            }
        }
    }
}

bool modellReiterZeileSichtbar() {
    sicherstellen();
    return g_reiter.size() > 1U || modellAblageDa();
}

float modellReiterZeileHoehe() { return ImGui::GetFrameHeight() + 2.0F; }

void drawModellReiterZeile(float breite) {
    sicherstellen();
    ModellReiterProbe& p = g_probe;
    p.bild = ImGui::GetFrameCount();
    p.reiter.clear();
    p.plus = ImRect{};
    p.hinweis = ImRect{};
    p.hinweisText.clear();
    p.hinweisGekuerzt = false;
    const float h = modellReiterZeileHoehe();
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    p.zeile = ImRect{lo, ImVec2{lo.x + breite, lo.y + h}};
    // rechts der Hinweis der Ablage: die Reiter gehen vor (ihre Namen sollen lesbar bleiben), der Hinweis nimmt, was sie uebrig
    // lassen, hoechstens 45 % der Zeile; gekuerzt steht er ganz im Tooltip, bleibt kein Platz (Mindestgroesse), im Tooltip von "+"
    const std::string hinweis = modellAblageHinweis();
    const ImGuiStyle& stil = ImGui::GetStyle();
    const float sp = stil.ItemSpacing.x;
    float bedarf = ImGui::CalcTextSize("+").x + stil.FramePadding.x * 2.0F + sp * 2.0F;
    for (int i = 0; i < static_cast<int>(g_reiter.size()); ++i) {
        // wie ImGui::TabItemCalcSize: Text, Rand, x mit Abstand; dazu der Abstand zwischen zwei Reitern
        bedarf += ImGui::CalcTextSize(modellReiterInfo(i).titel.c_str()).x + stil.FramePadding.x * 2.0F + stil.ItemInnerSpacing.x * 2.0F + ImGui::GetFontSize();
    }
    float hinweisB = 0.0F;
    std::string gezeigt = hinweis;
    if (!hinweis.empty()) {
        const float max = std::floor(std::min(breite * 0.45F, breite - bedarf));
        hinweisB = ImGui::CalcTextSize(hinweis.c_str()).x;
        if (max < ImGui::GetFontSize() * 6.0F) {
            p.hinweisGekuerzt = true;
            gezeigt.clear();
            hinweisB = 0.0F;
        } else if (hinweisB > max) {
            p.hinweisGekuerzt = true;
            const char* const punkte = "...";
            const float pb = ImGui::CalcTextSize(punkte).x;
            while (!gezeigt.empty() && ImGui::CalcTextSize(gezeigt.c_str()).x + pb > max) {
                gezeigt.pop_back();
                while (!gezeigt.empty() && (static_cast<unsigned char>(gezeigt.back()) & 0xC0U) == 0x80U) { gezeigt.pop_back(); }   // UTF-8
                if (!gezeigt.empty() && (static_cast<unsigned char>(gezeigt.back()) & 0x80U) != 0U) { gezeigt.pop_back(); }
            }
            gezeigt += punkte;
            hinweisB = ImGui::CalcTextSize(gezeigt.c_str()).x;
        }
    }
    const float reiterB = std::max(breite - (hinweisB > 0.0F ? hinweisB + sp * 2.0F : 0.0F), ImGui::GetFontSize() * 4.0F);
    ImGui::BeginChild("##mtreiter", ImVec2{reiterB, h}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGuiStyle& st = ImGui::GetStyle();
    const float schliessenAlt = st.TabCloseButtonMinWidthUnselected;
    st.TabCloseButtonMinWidthUnselected = -1.0F;   // wie im Entwurf: jedes x sichtbar
    if (ImGui::BeginTabBar("##mtreiterleiste", ImGuiTabBarFlags_FittingPolicyShrink | ImGuiTabBarFlags_NoTabListScrollingButtons)) {
        for (int i = 0; i < static_cast<int>(g_reiter.size()); ++i) {
            const ModellReiterInfo info = modellReiterInfo(i);
            // NoAssumedClosure: das x schliesst erst, wenn der Auftrag ausgefuehrt ist (ungespeichert: nach der Frage)
            ImGuiTabItemFlags f = ImGuiTabItemFlags_NoTooltip | ImGuiTabItemFlags_NoReorder | ImGuiTabItemFlags_NoAssumedClosure;
            if (info.ungespeichert) { f |= ImGuiTabItemFlags_UnsavedDocument; }
            if (info.aktiv && g_auswahlBilder > 0) { f |= ImGuiTabItemFlags_SetSelected; }
            bool offen = true;
            const std::string label = info.titel + "###mxreiter" + std::to_string(info.kennung);
            const bool gewaehlt = ImGui::BeginTabItem(label.c_str(), &offen, f);
            if (gewaehlt) { ImGui::EndTabItem(); }
            p.reiter.emplace_back(info.titel, ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()});
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
                std::string tipp = info.pfad.empty() ? info.titel : info.pfad;
                if (info.ungespeichert) { tipp += "\n" + std::string(tr(Str::MxUngespeichert)); }
                ImGui::SetTooltip("%s", tipp.c_str());
                p.tooltip = tipp;
            }
            if (ui::popupKontext("##mxreitermenue")) {
                if (ImGui::MenuItem(tr(Str::MxSchliessen))) { modellReiterSchliessen(i); }
                ImGui::SetItemTooltip("%s", tr(Str::MxSchliessenHint));
                if (ImGui::MenuItem(tr(Str::MxAndereSchliessen), nullptr, false, g_reiter.size() > 1U)) { modellReiterAndereSchliessen(i); }
                ImGui::Separator();
                if (ImGui::MenuItem(tr(Str::MxNeuerTab), "Ctrl+O")) { modellInNeuemReiterOeffnen(); }
                ImGui::SetItemTooltip("%s", tr(Str::MxNeuerTabHint));
                ImGui::EndPopup();
            }
            if (!offen) {
                modellReiterSchliessen(i);
            } else if (gewaehlt && !info.aktiv && g_auswahlBilder == 0) {
                modellReiterWechseln(i);
            }
        }
        if (ImGui::TabItemButton("+##mxneu", ImGuiTabItemFlags_NoTooltip)) { ImGui::OpenPopup("###mxneumenue"); }
        p.plus = ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            // ohne Platz fuer den Hinweis der Ablage (Mindestgroesse) steht er hier
            p.tooltip = gezeigt.empty() && !hinweis.empty() ? std::string(tr(Str::MxPlusHint)) + "\n" + hinweis : std::string(tr(Str::MxPlusHint));
            ImGui::SetTooltip("%s", p.tooltip.c_str());
        }
        if (ui::popup("###mxneumenue")) {
            modellNeuerReiterMenue();
            ImGui::EndPopup();
        }
        ImGui::EndTabBar();
    }
    st.TabCloseButtonMinWidthUnselected = schliessenAlt;
    if (g_auswahlBilder > 0) { --g_auswahlBilder; }
    ImGui::EndChild();
    if (!gezeigt.empty()) {
        // auf derselben Zeile rechtsbuendig, Grundlinie wie der Text der Reiter
        ImGui::SameLine(0.0F, std::max(breite - reiterB - hinweisB - sp, 0.0F));
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", gezeigt.c_str());
        p.hinweis = ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
        p.hinweisText = gezeigt;
        if (p.hinweisGekuerzt && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", hinweis.c_str()); }
    }
}

void modellNeuerReiterMenue() {
    if (ImGui::MenuItem(tr(Str::MxNeuerTabDatei), "Ctrl+O")) { modellInNeuemReiterOeffnen(); }
    ImGui::SetItemTooltip("%s", tr(Str::MxNeuerTabHint));
    if (ImGui::MenuItem(tr(Str::MxNeuerTabPk3))) {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->pk3Kind = 2;
        refreshPk3List();
        g_app->pk3Open = true;
        g_naechstesNeu = true;
    }
    ImGui::SetItemTooltip("%s", tr(Str::OpenGlmPk3Hint));
}

bool modellReiterBeendenFrage() {
    sicherstellen();
    for (std::size_t i = 0; i < g_reiter.size(); ++i) {
        Reiter& r = g_reiter[i];
        if (static_cast<int>(i) == g_aktiv || !r.app || !r.ungespeichert || r.app->mtab.beendenBeantwortet) { continue; }
        if (g_app->frageOffen) { return false; }
        // man sieht, worueber man entscheidet: der Reiter wird aktiv (am Anfang des naechsten Bilds)
        modellReiterWechseln(static_cast<int>(i));
        g_app->leftMode = 2;
        const int kennung = r.kennung;
        frage(format(Str::MbBeendenFrage, titelVon(r.pfad, r.name).c_str()),
              [kennung] {
                  const int k = indexVon(kennung);
                  if (k == g_aktiv) {
                      mt().beendenBeantwortet = true;
                  } else if (k >= 0 && g_reiter[static_cast<std::size_t>(k)].app) {
                      g_reiter[static_cast<std::size_t>(k)].app->mtab.beendenBeantwortet = true;
                  }
                  if (confirmQuit()) {
                      g_app->quitConfirmed = true;
                      g_app->wantQuit = true;
                  }
              },
              [] { beendenAbgebrochen(); });
        return false;
    }
    return true;
}

void modellReiterBeendenVergessen() {
    for (Reiter& r : g_reiter) {
        if (r.app) { r.app->mtab.beendenBeantwortet = false; }
    }
}

ModellReiterProbe& modellReiterProbe() { return g_probe; }

// --- Ablage -------------------------------------------------------------------------------------------------------------------------

bool modellAblageDa() { return !g_ablage.leer(); }

std::string modellAblageHinweis() {
    if (g_ablage.leer()) { return {}; }
    return format(Str::MxAblageHinweis, ablageWas().c_str(), g_ablage.quelle.c_str());
}

bool modellFlaechenKopieren() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    std::vector<int> wahl;
    for (const int i : t.auswahl) {
        if (i >= 0 && static_cast<std::size_t>(i) < in.teile.size()) { wahl.push_back(i); }
    }
    if (g_app->model.empty() || wahl.empty()) {
        melden(tr(Str::MxNichtsGewaehlt));
        return false;
    }
    const bool md3 = in.herkunft == modell::Herkunft::Md3;
    if (!md3 && g_app->anim.bones.empty()) {
        melden(tr(Str::MxQuelleOhneGla));
        return false;
    }
    // die Bilder, die die Haut zeigt (je Teil; ohne: der Shader der Datei)
    std::vector<std::string> bilder(in.teile.size());
    for (std::size_t i = 0; i < bilder.size() && i < g_app->model.surfaces.size(); ++i) {
        const GlmSurface& s = g_app->model.surfaces[i];
        bilder[i] = !s.texture.empty() ? s.texture : s.shader;
    }
    std::vector<int> eltern;
    for (const GlaBone& b : g_app->anim.bones) { eltern.push_back(b.parent); }
    // FIX612B P2-2: was man sieht - offene Mallisten uebernommen (eine Kopie nur, wenn gemalt wird)
    const bool malt = modell::malt(in);
    const modell::Inhalt gezeigt = malt ? modell::wieGezeigt(in) : modell::Inhalt{};
    g_ablage = modell::ablageAus(malt ? gezeigt : in, wahl, bilder, md3 ? std::vector<std::string>{} : knochenNamen(g_app->anim),
                                 md3 ? std::vector<int>{} : eltern, modellReiterTitel(g_app->modelPath, in.name));
    melden(format(Str::MxKopiert, ablageWas().c_str(), g_ablage.quelle.c_str()));
    diag::detail("Model-Tab: Ablage " + std::to_string(g_ablage.teile.size()) + " Flaechen aus " + g_ablage.quelle + ", " +
                 std::to_string(g_ablage.knochen.size()) + " Knochen, " + std::to_string(g_ablage.stufen) + " Stufen");
    return true;
}

bool modellFlaechenEinfuegen(int elternTeil) {
    ModellTab& t = mt();
    if (g_ablage.leer()) {
        melden(tr(Str::MxAblageLeer));
        return false;
    }
    if (g_app->model.empty()) {
        melden(tr(Str::MxZielLeer));
        return false;
    }
    offenesBeenden();
    const modell::Zielspiel ziel = static_cast<modell::Zielspiel>(std::clamp(t.zielspiel, 0, modell::kZielspiele - 1));
    modell::Inhalt neu = t.dok.inhalt();
    modell::EinfuegeBericht b;
    const modell::EinfuegeFehler f = modell::einfuegen(neu, g_ablage, elternTeil, knochenNamen(g_app->anim), modell::grenzen(ziel), b);
    switch (f) {
        case modell::EinfuegeFehler::Keiner: break;
        case modell::EinfuegeFehler::AblageLeer: melden(tr(Str::MxAblageLeer)); return false;
        case modell::EinfuegeFehler::ZielLeer: melden(tr(Str::MxZielLeer)); return false;
        case modell::EinfuegeFehler::ZielOhneSkelett: melden(tr(Str::MxZielOhneGla)); return false;
        case modell::EinfuegeFehler::ElternUngueltig: melden(tr(Str::MxNichtsGewaehlt)); return false;
    }
    (void)t.dok.aendern(tr(Str::MxUndoEinfuegen), [&neu](modell::Inhalt& x) { x = std::move(neu); });
    t.auswahl = b.neu;
    t.knochenWahl = -1;
    t.tagWahl = -1;
    g_app->modelDirty = true;
    // die Meldung: was, woher, wohin - und alles, was das Ziel anders bekommt
    const modell::Inhalt& in = t.dok.inhalt();
    std::string m = elternTeil >= 0 && static_cast<std::size_t>(elternTeil) < in.teile.size()
                        ? format(Str::MxEingefuegt, ablageWas().c_str(), g_ablage.quelle.c_str(), in.teile[static_cast<std::size_t>(elternTeil)].name.c_str())
                        : format(Str::MxEingefuegtOben, ablageWas().c_str(), g_ablage.quelle.c_str());
    if (!b.umbenannt.empty()) {
        std::vector<std::string> z;
        for (const auto& [alt, nn] : b.umbenannt) { z.push_back(alt + " -> " + nn); }
        m += " " + format(Str::MxUmbenannt, liste(z).c_str());
    }
    if (!b.ersatz.empty()) {
        std::vector<std::string> z;
        for (const auto& [alt, nn] : b.ersatz) { z.push_back(alt + " -> " + nn); }
        m += " " + format(Str::MxErsatz, liste(z).c_str());
    }
    if (b.stufenWeg > 0) { m += " " + format(Str::MxStufenWeg, b.stufenWeg, std::max(in.stufenZahl, 1)); }
    if (b.stufenAufgefuellt > 0) { m += " " + format(Str::MxStufenAuf, b.stufenAufgefuellt); }
    if (b.zielOhneSkelett) { m += " " + std::string(tr(Str::MxStarrZiel)); }
    if (b.quelleOhneSkelett) { m += " " + format(Str::MxStarrQuelle, g_app->anim.bones.empty() ? "0" : g_app->anim.bones.front().name.c_str()); }
    const modell::Grenzen& g = modell::grenzen(ziel);
    if (b.befunde.empty()) {
        m += " " + format(Str::MxGrenzenOk, g.name);
    } else {
        std::vector<std::string> z;
        for (const modell::Befund& x : b.befunde) { z.push_back(modellBefundText(x)); }
        m += " " + format(Str::MxGrenzen, g.name, liste(z).c_str());
    }
    melden(m);
    diag::detail("Model-Tab: eingefuegt " + std::to_string(b.neu.size()) + " Flaechen, Ersatzknochen " + std::to_string(b.ersatz.size()) +
                 ", umbenannt " + std::to_string(b.umbenannt.size()) + ", Befunde " + std::to_string(b.befunde.size()));
    return true;
}

void modellAblageMenue(int teil) {
    if (ImGui::MenuItem(tr(Str::MxKopieren), "Ctrl+C")) {
        mt().auswahl = {teil};
        (void)modellFlaechenKopieren();
    }
    ImGui::SetItemTooltip("%s", tr(Str::MxKopierenHint));
    if (ImGui::MenuItem(tr(Str::MxEinfuegenKind), "Ctrl+V", false, modellAblageDa())) { (void)modellFlaechenEinfuegen(teil); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_ForTooltip)) {
        ImGui::SetTooltip("%s", modellAblageDa() ? (modellAblageHinweis() + "\n" + tr(Str::MxEinfuegenKindHint)).c_str() : tr(Str::MxAblageLeer));
    }
}

bool modellReiterTasten() {
    // im Bereich Animation gehoeren Strg+C / Strg+V der Pose (modellRigTasten, wie Blender pose.copy / pose.paste)
    if (mt().bereich == ModellBereich::Animation) { return false; }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C)) {
        (void)modellFlaechenKopieren();
        return true;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_V)) {
        const std::vector<int>& w = mt().auswahl;
        (void)modellFlaechenEinfuegen(w.empty() ? -1 : w.front());
        return true;
    }
    return false;
}

bool modellReiterAktionenSuche(const std::string& such) {
    const auto passt = [&](const char* text) {
        std::string k = text;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return such.empty() || k.find(such) != std::string::npos;
    };
    if (passt(tr(Str::MxNeuerTab))) {
        if (modellAktionZeile(tr(Str::MxNeuerTab))) {
            modellInNeuemReiterOeffnen();
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MxNeuerTabHint));
    }
    if (passt(tr(Str::MxSchliessen))) {
        if (modellAktionZeile(tr(Str::MxSchliessen))) {
            modellReiterSchliessen(modellReiterAktiv());
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MxSchliessenHint));
    }
    if (passt(tr(Str::MxKopieren))) {
        if (modellAktionZeile(tr(Str::MxKopieren))) {
            (void)modellFlaechenKopieren();
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MxKopierenHint));
    }
    if (passt(tr(Str::MxEinfuegen))) {
        if (modellAktionZeile(tr(Str::MxEinfuegen))) {
            const std::vector<int>& w = mt().auswahl;
            (void)modellFlaechenEinfuegen(w.empty() ? -1 : w.front());
            return true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MxEinfuegenKindHint));
    }
    return false;
}

}  // namespace bhed::gui
