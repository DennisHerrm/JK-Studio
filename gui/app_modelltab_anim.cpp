// app_modelltab_anim.cpp - das rechte Feld des Model-Tabs im Bereich Animation nach dem freigegebenen Entwurf (AP10 von
// MBENTWURF Teil B, rc612)
//
// Massstab: plaene/modellblock_stufe0/entwurf_modelltab.html (feldInhalt, Bereich Animation) und die Abgleichtabelle
// abgleich_programm_entwurf.md, Zeilen A0a-A11 und A14. Der Entwurf zeigt im Reiter "Bewegung" (wie 3ds Max) bzw. "Knochen"
// (wie Blender) acht Gruppen untereinander:
//
//   Animation                          Name der Sequenz, Ziel-GLA, Plaetze belegt / gesamt            app_modelltab_mb3.cpp
//   Rig erstellen                      Skelett, Massstab, erkannte Vorlage, Auswahl der Vorlage,      app_modelltab_rig.cpp
//                                      Control Rig erstellen, Animation-Rig (nur FK), Auf Rig legen   (Vorlagen: hier)
//   Presets                            Bibliothek nach Kategorie, Ziel-Platz, Generator               app_modelltab_retarget.cpp
//   Vorhandene Animation bearbeiten    In Schluessel umwandeln, zurueck in die GLA backen             app_modelltab_kurven.cpp
//   Uebertragen (Retargeting)          Quelle und Ziel, Ketten, Grundpose, Schritte, Fussrutschen     app_modelltab_retarget.cpp
//   IK/FK                              je Glied der Zustand und der Wechsel                           app_modelltab_ik.cpp
//   Anbauten und Vorschau              Anbauten, Primaer- und Sekundaersequenz                        hier
//   Fuer das Spiel bauen               Wurzelweg, Bauen                                               app_modelltab_mb3.cpp
//
// Dahinter steht, was das Programm mehr kann als der Entwurf (Zeile A14): Auto-Physik, Pose, Ghosts, Bewegungspfade, Kurven und
// Animationswerkzeuge, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plaetze. Jede Gruppe ist ein ui::rolloutBeginn des
// Bausteins gui/ui_rollout.h und nennt ihren Reiter; im Reiter, den der Bereich vorgibt, stehen wie im Entwurf alle.
//
// Rig-Vorlagen: modell::rtVorlagen() (retarget.h) kennt die 34 Skelette des Basisspiels und die Rigs anderer Programme. Die
// passende wird ueber Dateiname und Knochennamen gefunden ("Erkannt als"); die Auswahl stellt eine andere oder "Eigenes
// Skelett" ein. Die Vorlage bestimmt die Ketten des Ziels beim Uebertragen (rtCharakter) und gibt beim Erstellen des Rigs den
// Knochen eine Rolle, die die Erkennung (rig.h) offen laesst oder unklar nennt.
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/retarget.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellAnim g_anim;

ModellTab& mt() { return g_app->mtab; }

void melden(const std::string& m) {
    mt().meldung = m;
    mt().rigMeldung = m;
    addStatus(m);
}

std::string format(Str s, auto... a) {
    char b[600];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

const AnimEntry* sequenz() {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return nullptr; }
    return &g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
}

const std::vector<modell::RtVorlageInfo>& vorlagen() {
    static const std::vector<modell::RtVorlageInfo> kListe = modell::rtVorlagen();
    return kListe;
}

const std::vector<std::string>& gruppen() {
    static const std::vector<std::string> kListe = modell::rtVorlagenGruppen();
    return kListe;
}

// Der Name einer Gruppe von Vorlagen: die Sammelgruppen uebersetzt, sonst der Eigenname des Spiels (Rancor, Wampa ...).
const char* gruppeText(const std::string& g) {
    if (g == "human") { return tr(Str::AzVorMensch); }
    if (g == "droid") { return tr(Str::AzVorDroiden); }
    if (g == "vehicle") { return tr(Str::AzVorFahrzeuge); }
    if (g == "object") { return tr(Str::AzVorObjekte); }
    if (g == "sand") { return tr(Str::AzVorSand); }
    return g.c_str();
}

// Die Wahl gilt fuer ein Skelett: mit einem anderen wieder automatisch, die erkannte Vorlage neu gesucht.
void vorlageAbgleich() {
    const std::string k = g_app->anim.name + "|" + g_app->anim.skeletonName + "|" + std::to_string(g_app->anim.bones.size());
    if (g_anim.vorlageFuer == k) { return; }
    g_anim.vorlageFuer = k;
    g_anim.vorlage = -2;
    g_anim.erkannt = g_app->anim.bones.empty() ? -1 : modell::rtVorlageFuer(g_app->anim, g_app->anim.skeletonName + ".gla");
}

int gruppeVon(int vorlage) {
    const auto& v = vorlagen();
    const auto& gr = gruppen();
    if (vorlage < 0 || static_cast<std::size_t>(vorlage) >= v.size()) { return -1; }
    const auto it = std::find(gr.begin(), gr.end(), v[static_cast<std::size_t>(vorlage)].gruppe);
    return it == gr.end() ? -1 : static_cast<int>(it - gr.begin());
}

}  // namespace

ModellAnim& modellAnim() { return g_anim; }

bool modellRigNurFk() { return g_anim.nurFk; }
void modellRigNurFkSetzen(bool an) { g_anim.nurFk = an; }

int modellAnimVorlageWahl() {
    vorlageAbgleich();
    return g_anim.vorlage;
}

int modellAnimVorlageErkannt() {
    vorlageAbgleich();
    return g_anim.erkannt;
}

int modellAnimVorlage() {
    vorlageAbgleich();
    return g_anim.vorlage == -2 ? g_anim.erkannt : g_anim.vorlage;
}

void modellAnimVorlageSetzen(int wahl) {
    vorlageAbgleich();
    wahl = std::clamp(wahl, -2, static_cast<int>(vorlagen().size()) - 1);
    // die erkannte Vorlage von Hand gewaehlt ist "automatisch"
    if (wahl >= 0 && wahl == g_anim.erkannt) { wahl = -2; }
    if (wahl == g_anim.vorlage) { return; }
    g_anim.vorlage = wahl;
    const int w = modellAnimVorlage();
    melden(format(Str::AzVorlageGesetzt, w >= 0 ? vorlagen()[static_cast<std::size_t>(w)].name.c_str() : tr(Str::AzVorEigen)));
    // ein vorhandenes Rig bekommt die Rollen der neuen Vorlage
    if (modellRigDa()) { (void)modellRigAnlegen(); }
    g_app->modelDirty = true;
}

std::vector<modell::Zuordnung> modellAnimVorlageZuordnung() {
    std::vector<modell::Zuordnung> aus;
    const int v = modellAnimVorlage();
    if (v < 0 || g_app->anim.bones.empty()) { return aus; }
    const std::vector<modell::Zuordnung> rollen = modell::rtVorlageRollen(g_app->anim, v);
    if (rollen.empty()) { return aus; }
    // nur, was die Erkennung offen laesst oder unklar nennt - was sie sicher erkennt und was von Hand zugeordnet ist, bleibt
    const modell::Inhalt& in = mt().dok.inhalt();
    const modell::Erkennung e = modell::erkenneKnochen(g_app->anim, in.rig.zuordnung, &in, 0);
    for (const modell::Zuordnung& z : rollen) {
        if (z.knochen < 0 || static_cast<std::size_t>(z.knochen) >= e.knochen.size()) { continue; }
        const modell::KnochenErkennung& k = e.knochen[static_cast<std::size_t>(z.knochen)];
        if (!k.vonHand && (k.rolle == modell::Rolle::Keine || k.unklar)) { aus.push_back(z); }
    }
    return aus;
}

bool modellAnimRigErstellen(bool nurFk) {
    g_anim.nurFk = nurFk;
    if (!modellRigAnlegen()) { return false; }
    // ENT2 (shank 12.11.: "bei Control Rig Erstellung von FK auf IK uebertragen ... wie das CAT Rig"): eine Animation der Sequenz
    // geht beim Erstellen des Control Rigs von FK auf IK auf das Rig (wie "Auf Rig legen"), danach arbeitet man an den Griffen.
    // Eine Schicht mit Schluesseln - schon aufgelegt oder bearbeitet - bleibt (Neu erkennen); "Auf Rig legen" legt neu auf.
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const bool leer = s == nullptr || (s->schluesselZahl() == 0 && std::all_of(s->ik.begin(), s->ik.end(), [](const modell::IkSpur& x) { return x.ziel.empty(); }));
    if (!nurFk && sequenz() != nullptr && leer) { (void)modellAnimAufRigLegen(); }
    return true;
}

void drawModellAnimVorlage() {
    vorlageAbgleich();
    const auto& v = vorlagen();
    const auto& gr = gruppen();
    const GlaAnimation& a = g_app->anim;
    const int wirksam = modellAnimVorlage();
    const int knochen = static_cast<int>(a.bones.size());
    char skelett[240];
    std::snprintf(skelett, sizeof skelett, trN(Str::AzSkelettWert, knochen), (fileName(a.skeletonName) + ".gla").c_str(), knochen);
    char massstab[32];
    std::snprintf(massstab, sizeof massstab, "%.3g", static_cast<double>(a.scale));
    std::string erkannt = tr(Str::AzErkanntEigen);
    ui::Ton ton = g_anim.vorlage == -1 ? ui::Ton::Normal : ui::Ton::Warnung;
    if (wirksam >= 0) {
        const bool vonHand = g_anim.vorlage >= 0;
        erkannt = format(vonHand ? Str::AzErkanntHand : Str::AzErkanntVorlage, v[static_cast<std::size_t>(wirksam)].name.c_str());
        ton = vonHand ? ui::Ton::Normal : ui::Ton::Gut;
    }
    ui::wertTabelle({{tr(Str::AzSkelett), skelett, ui::Ton::Normal, a.skeletonName + ".gla"},
                     {tr(Str::AzMassstab), massstab, ui::Ton::Normal, {}},
                     {tr(Str::AzErkannt), erkannt, ton, tr(Str::AzVorlageHint)}});
    // die Gruppen der Vorlagen und "Eigenes Skelett" als eine Wahl in zwei Spalten
    std::vector<const char*> texte;
    std::vector<const char*> hinweise;
    for (const std::string& x : gr) {
        texte.push_back(gruppeText(x));
        hinweise.push_back(tr(Str::AzVorlageHint));
    }
    texte.push_back(tr(Str::AzVorEigen));
    hinweise.push_back(tr(Str::AzVorlageHint));
    const int eigen = static_cast<int>(gr.size());
    const int gruppeJetzt = wirksam >= 0 ? gruppeVon(wirksam) : eigen;
    int wahl = gruppeJetzt;
    if (ui::segment("##azvorlage", texte.data(), static_cast<int>(texte.size()), &wahl, 2, -1, hinweise.data(), 0.0F, true) && wahl != gruppeJetzt) {
        if (wahl == eigen) {
            modellAnimVorlageSetzen(-1);
        } else if (gruppeVon(g_anim.erkannt) == wahl) {
            modellAnimVorlageSetzen(-2);   // die Gruppe der erkannten Vorlage: wieder automatisch
        } else {
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (v[i].gruppe == gr[static_cast<std::size_t>(wahl)]) {
                    modellAnimVorlageSetzen(static_cast<int>(i));
                    break;
                }
            }
        }
    }
    // mehrere Vorlagen in der Gruppe: welche
    const int jetzt = modellAnimVorlage();
    if (jetzt < 0) { return; }
    const std::string& gruppe = v[static_cast<std::size_t>(jetzt)].gruppe;
    const auto inGruppe = std::count_if(v.begin(), v.end(), [&](const modell::RtVorlageInfo& x) { return x.gruppe == gruppe; });
    if (inGruppe < 2) { return; }
    ui::zeile(tr(Str::AzVorlage), tr(Str::AzVorlageHint));
    if (ImGui::BeginCombo("##azvorlagewahl", v[static_cast<std::size_t>(jetzt)].name.c_str())) {
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (v[i].gruppe != gruppe) { continue; }
            if (ImGui::Selectable(v[i].name.c_str(), static_cast<int>(i) == jetzt)) { modellAnimVorlageSetzen(static_cast<int>(i)); }
            if (!v[i].datei.empty()) { ImGui::SetItemTooltip("%s", v[i].datei.c_str()); }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(Str::AzVorlageHint));
}

bool modellAnimWurzelweg(float& weg) {
    const AnimEntry* e = sequenz();
    if (e == nullptr || g_app->anim.empty()) { return false; }
    const std::string k = g_app->anim.name + "|" + e->name + "|" + std::to_string(e->firstFrame) + "|" + std::to_string(e->numFrames) + "|" +
                          std::to_string(mt().dok.stand()) + "|" + std::to_string(modellRigDa() ? 1 : 0);
    if (g_anim.wegFuer != k) {
        g_anim.wegFuer = k;
        g_anim.wegDa = false;
        g_anim.weg = 0.0F;
        // das Becken: aus dem Rig, ohne Rig aus der Erkennung des Skeletts
        modell::Erkennung eigene;
        const modell::Erkennung* er = &mt().rig.erkennung;
        if (!modellRigDa()) {
            eigene = modell::erkenneKnochen(g_app->anim);
            er = &eigene;
        }
        int becken = -1;
        for (std::size_t b = 0; b < er->knochen.size() && becken < 0; ++b) {
            if (er->knochen[b].rolle == modell::Rolle::Becken) { becken = static_cast<int>(b); }
        }
        if (becken >= 0 && static_cast<std::size_t>(becken) < g_app->anim.bones.size()) {
            // wie Stufe 7 (modell::wurzelPfad, FaWurzelWeg): Ort des Beckens am ersten und am letzten Bild mit der Schicht
            static const modell::RigSchicht kLeer;
            const modell::RigSchicht* s = modellRigSchichtJetzt();
            const int erstes = std::clamp(e->firstFrame, 0, std::max(g_app->anim.bildZahl() - 1, 0));
            const int letztes = std::clamp(e->firstFrame + std::max(e->numFrames, 1) - 1, erstes, std::max(g_app->anim.bildZahl() - 1, 0));
            std::vector<BoneMatrix> w0;
            std::vector<BoneMatrix> w1;
            modell::rigWelt(g_app->anim, s != nullptr ? *s : kLeer, erstes, erstes, 0.0F, w0);
            modell::rigWelt(g_app->anim, s != nullptr ? *s : kLeer, letztes, letztes, 0.0F, w1);
            if (static_cast<std::size_t>(becken) < w0.size() && static_cast<std::size_t>(becken) < w1.size()) {
                const BoneMatrix& m0 = w0[static_cast<std::size_t>(becken)];
                const BoneMatrix& m1 = w1[static_cast<std::size_t>(becken)];
                g_anim.weg = std::hypot(m1.m[0][3] - m0.m[0][3], m1.m[1][3] - m0.m[1][3]);
                g_anim.wegDa = true;
            }
        }
    }
    weg = g_anim.weg;
    return g_anim.wegDa;
}

// Entwurf A8: was an der Figur haengt und was sie spielt - die Anbauten (Tag oder Knochen), die Primaersequenz und die
// Sekundaersequenz ab ihrem Startknochen. Die Liste zum Loesen steht im Bereich Ansehen/Pruefen (Gruppe Anbauten).
void modellAnimGruppeAnbauten() {
    const ModellTab& t = mt();
    char info[16];
    std::snprintf(info, sizeof info, "%d", static_cast<int>(t.anbauten.size()));
    if (!ui::rolloutBeginn(tr(Str::AzRAnbauten),
                           {.kurzinfo = t.anbauten.empty() ? nullptr : info, .reiter = modellReiter(MbReiter::Hierarchie, MbReiterBl::Constraints)})) {
        return;
    }
    std::string an;
    for (const ModellAnbau& x : t.anbauten) { an += (an.empty() ? "" : ", ") + fileName(x.pfad) + " \xe2\x86\x92 " + x.ziel; }
    const AnimEntry* e = sequenz();
    const bool hatSek = t.sekundaer >= 0 && static_cast<std::size_t>(t.sekundaer) < g_app->animList.size();
    const std::string sek = format(Str::AzSekWert, hatSek ? g_app->animList[static_cast<std::size_t>(t.sekundaer)].name.c_str() : "\xe2\x80\x94", t.sekundaerKnochen.c_str());
    ui::wertTabelle({{tr(Str::MbRAnbauten), an.empty() ? std::string(tr(Str::AzKeine)) : an, an.empty() ? ui::Ton::Leise : ui::Ton::Normal, {}},
                     {tr(Str::MbPrimaer), e != nullptr ? e->name : std::string(tr(Str::AzKeineSequenz)), e != nullptr ? ui::Ton::Normal : ui::Ton::Leise, {}},
                     {tr(Str::AzSekundaer), sek, hatSek ? ui::Ton::Normal : ui::Ton::Leise, tr(Str::MzOberkoerper)}});
    ui::tipp(tr(Str::AzAnbauTipp));
    ui::rolloutEnde();
}

// --- Werkzeuge fuer die Werkzeugleiste wie Blender (Entwurf A11) -----------------------------------------------------------
namespace {

struct Werkzeug {
    UiIcon icon;
    Str text;
    Str hinweis;
};

// Pose kopieren, Spiegeln (die kopierte Pose gespiegelt einfuegen), Ausduennen (Frames zu Schluesseln reduzieren)
const Werkzeug kWerkzeuge[3] = {
    {UiIcon::Duplizieren, Str::MbRgKopieren, Str::MbRgKopierenHint},
    {UiIcon::Tauschen, Str::MbRgGespiegelt, Str::MbRgGespiegeltHint},
    {UiIcon::Ruhig, Str::MbKvReduzieren, Str::MbKvReduzierenHint},
};

// die Taste der Aktion im laufenden Bedienmodus aus der Tabelle der Rig-Aktionen (dort stehen sie mit ihrer Quelle)
const char* tasteVon(Str text) {
    const std::string name = tr(text);
    for (int i = 0; i < modellRigAktionZahl(); ++i) {
        if (name == modellRigAktionText(i)) { return modellRigAktionTaste(i, modellBlender()); }
    }
    return "";
}

}  // namespace

int modellAnimWerkzeugZahl() { return static_cast<int>(std::size(kWerkzeuge)); }

ModellAnimWerkzeug modellAnimWerkzeug(int i) {
    const Werkzeug& w = kWerkzeuge[std::clamp(i, 0, modellAnimWerkzeugZahl() - 1)];
    ModellAnimWerkzeug aus;
    aus.icon = w.icon;
    aus.text = tr(w.text);
    aus.hinweis = tr(w.hinweis);
    aus.taste = tasteVon(w.text);
    aus.aktiv = !g_app->anim.bones.empty() && (i != 1 || mt().kopieDa) && (i != 2 || sequenz() != nullptr);
    return aus;
}

bool modellAnimWerkzeugAusloesen(int i) {
    switch (i) {
        case 0: return modellPoseKopieren();
        case 1: return modellPoseEinfuegen(true);
        case 2: return modellAnimReduzieren();
        default: return false;
    }
}

// --- Die Reihenfolge der Gruppen -------------------------------------------------------------------------------------------

void drawModellAnimFeld() {
    // wie im Entwurf (feldInhalt, Bereich Animation)
    modellAnimGruppeAnimation();
    modellAnimGruppeRig();
    drawModellPresetBereich();
    modellAnimGruppeBearbeiten();
    drawModellRetargetBereich();
    drawModellIkBereich();
    modellAnimGruppeAnbauten();
    modellAnimGruppeBauen();
    // was das Programm mehr kann (ab Werk zugeklappt - Platz nur, wenn gebraucht)
    drawModellPhysikBereich();
    modellAnimGruppePose();
    modellAnimGruppeGeister();
    modellAnimGruppePfade();
}

}  // namespace bhed::gui
