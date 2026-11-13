// app_modelltab_gewichte.cpp - AP8 (MBENTWURF Teil B, rc612): der Bereich Weights des Model-Tabs nach dem freigegebenen Entwurf
//
// Das rechte Feld (Baustein gui/ui_rollout.h) wie 3ds Max im Reiter "Aendern": Modifier-Stapel, Gewichts-Werkzeug, Spiegeln,
// Automatisch (Auto-Skin), Knochen, JKA-Grenzen - und wie Blender im Reiter "Daten": Pinsel, Optionen, Bereinigen, Spiegeln,
// Automatisch, Knochen, JKA-Grenzen (Abgleich W0a-W9). Dazu die Befehle der Werkzeuge mit je einem Rueckgaengig-Schritt, das
// Sperren von Knochen (W12, wie Blender K), die Anzeige des Spiegelmodus (W5: links blau, rechts gruen, ohne Gegenstueck gelb)
// und die Knochen, die im Bereich Weights ab Werk zu sehen sind (W10).
//
// Vorbilder (nur gelesen):
//   3ds Max   Autodesk-Hilfe "Weight Tool Dialog (Skin Modifier)", "Parameters Rollout (Skin Modifier)", "Mirror Parameters
//             Rollout" (help.autodesk.com) - Festwerte 0/.1/.25/.5/.75/.9/1 setzen das Gewicht der gewaehlten Ecken, Scale
//             Weight (Vorgabe 0,95), Shrink/Grow/Ring/Loop, Blend, Mirror Mode (links blau, rechts gruen), Paste Blue to Green
//             Verts / Green to Blue Verts, Mirror Paste (Auswahl), Mirror Thresh.
//   Blender   Quelltext 5.0 (paint_weight.cc, paint_vertex_weight_ops.cc, object_vgroup.cc, space_view3d.py): Werkzeuge
//             Draw/Blur/Smear/Gradient/Sample Weight, Auto Normalize, Weights > Normalize All/Clean/Smooth/Limit Total/Mirror,
//             K = Menue "Vertex Group Locks".
// Die Rechnung steht in bhed/modell/gewichte.h; der Pinsel in der Ansicht in app_modelltab_ansicht.cpp (modellTupfer).
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/gewichte.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }
ModellGewichte& gw() { return modellGewichte(); }

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

template <typename... A>
std::string text(Str s, A... a) {
    char z[600];
    std::snprintf(z, sizeof z, tr(s), a...);
    return z;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::vector<std::string> knochenNamen() {
    std::vector<std::string> namen;
    namen.reserve(g_app->anim.bones.size());
    for (const GlaBone& b : g_app->anim.bones) { namen.push_back(b.name); }
    return namen;
}

bool hatKnochen() {
    const ModellTab& t = mt();
    return t.knochenWahl >= 0 && uz(t.knochenWahl) < g_app->anim.bones.size();
}

const char* knochenName(int k) { return k >= 0 && uz(k) < g_app->anim.bones.size() ? g_app->anim.bones[uz(k)].name.c_str() : "-"; }

void melde(const std::string& z) {
    mt().meldung = z;
    g_app->modelDirty = true;
}

// Ein Befehl am Dokument als EIN Rueckgaengig-Schritt; aendert er nichts (Rueckgabe <= 0), bleibt kein leerer Schritt stehen.
template <typename F>
int befehl(Str name, F f) {
    ModellTab& t = mt();
    int n = 0;
    const std::uint64_t schritt = t.dok.aendern(tr(name), [&](modell::Inhalt& x) { n = f(x); });
    if (n <= 0) { (void)t.dok.abbrechen(schritt); }
    gw().letzteZahl = n;
    g_app->modelDirty = true;
    return n;
}

// Ohne gewaehlten Knochen oder an einem gesperrten geht kein Befehl am Gewicht eines Knochens; die Meldung sagt, warum.
bool knochenBereit() {
    if (!hatKnochen()) {
        melde(tr(Str::MbGwKnochenWaehlen));
        return false;
    }
    if (modellGewichtGesperrt(mt().knochenWahl)) {
        melde(tr(Str::MgGesperrtAktiv));
        return false;
    }
    return true;
}

// Nach einem Befehl, der Knochen auf andere Ecken bringt: die Grenze der Knochen je Flaeche des Zielspiels (W5: "Danach wird
// die Grenze von 32 Knochen je Flaeche geprueft"). Ein Befund haengt an der Meldung.
void grenzeMelden(std::string meldung) {
    const ModellTab& t = mt();
    const modell::Grenzen& gr = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    // was das Spiel aus den Mallisten bekaeme: eine Kopie uebernehmen und messen
    modell::Inhalt probe = t.dok.inhalt();
    (void)modell::uebernehmen(probe);
    for (const modell::Teil& teil : probe.teile) {
        if (teil.istTag() || uz(t.lodJetzt) >= teil.stufen.size()) { continue; }
        const int knochen = modell::mass(teil.stufen[uz(t.lodJetzt)]).knochen;
        if (knochen > gr.knochenJeFlaeche) {
            meldung += " " + text(Str::MgSpiegelGrenze, teil.name.c_str(), knochen, gr.knochenJeFlaeche);
            break;
        }
    }
    melde(meldung);
}

// --- Sperren wie Blender (object.vertex_group_lock, vgroup_lock_all) -----------------------------------------------------
void sperrListeAnpassen() {
    ModellGewichte& g = gw();
    // FIX612B P2-7: die Sperren gelten dem geladenen Modell - ein anderes (auch mit gleicher Knochenzahl) beginnt ohne. Vorher
    // blieben sie stehen, wenn nur die Laenge gleich war.
    static int geladen = -1;
    const int stand = mt().dok.ladeStand();
    if (g.gesperrt.size() != g_app->anim.bones.size() || stand != geladen) { g.gesperrt.assign(g_app->anim.bones.size(), 0); }
    geladen = stand;
}

int gesperrteZahl() {
    const ModellGewichte& g = gw();
    return static_cast<int>(std::count_if(g.gesperrt.begin(), g.gesperrt.end(), [](char c) { return c != 0; }));
}

// --- Aktionen: Suche, Rechtsklick, Selbsttest --------------------------------------------------------------------------------
struct GwAktion {
    Str text;
    Str hinweis;
    const char* maxTaste;
    const char* blenderTaste;
    int gruppe;   // 0 Werkzeug, 1 Bereinigen, 2 Spiegeln, 3 Sperren
    std::function<void()> tun;
    std::function<bool()> an;   // Umschalter: der Haken
};

void zuWeights() {
    mt().bereich = ModellBereich::Weights;
    g_app->modelDirty = true;
}

const std::vector<GwAktion>& aktionen() {
    static const std::vector<GwAktion> kListe = {
        {Str::MgMalenMax, Str::MgMalenHint, "", "", 0,
         [] {
             zuWeights();
             modellGewichteMalenSetzen(!gw().malen);
         },
         [] { return gw().malen; }},
        {Str::MgVerwischen, Str::MgVerwischenHint, "", "", 0,
         [] {
             zuWeights();
             modellGewichteWerkzeug(gw().werkzeug == 1 ? 0 : 1);
         },
         [] { return gw().werkzeug == 1; }},
        {Str::MgVerlauf, Str::MgVerlaufHint, "", "", 0,
         [] {
             zuWeights();
             modellGewichteWerkzeug(gw().werkzeug == 2 ? 0 : 2);
         },
         [] { return gw().werkzeug == 2; }},
        {Str::MgSkalieren, Str::MgSkalierenHint, "", "", 0, [] { (void)modellGewichteSkalieren(); }, nullptr},
        {Str::MgMischen, Str::MgMischenHint, "", "", 0, [] { (void)modellGewichteMischen(); }, nullptr},
        {Str::MgWachsen, Str::MgWachsenHint, "", "", 0, [] { (void)modellGewichteWahl(0); }, nullptr},
        {Str::MgSchrumpfen, Str::MgSchrumpfenHint, "", "", 0, [] { (void)modellGewichteWahl(1); }, nullptr},
        {Str::MgRing, Str::MgRingHint, "", "", 0, [] { (void)modellGewichteWahl(2); }, nullptr},
        {Str::MgSchleife, Str::MgSchleifeHint, "", "", 0, [] { (void)modellGewichteWahl(3); }, nullptr},
        {Str::MgAutoNorm, Str::MgAutoNormHint, "", "", 0, [] { gw().autoNormalisieren = !gw().autoNormalisieren; }, [] { return gw().autoNormalisieren; }},
        {Str::MgNormAlle, Str::MgNormAlleHint, "", "", 1, [] { (void)modellGewichteBereinigen(0); }, nullptr},
        {Str::MgSaeubern, Str::MgSaeubernHint, "", "", 1, [] { (void)modellGewichteBereinigen(1); }, nullptr},
        {Str::MgGlaettenAlle, Str::MgGlaettenAlleHint, "", "", 1, [] { (void)modellGewichteBereinigen(2); }, nullptr},
        {Str::MgBegrenzen, Str::MgBegrenzenHint, "", "", 1, [] { (void)modellGewichteBereinigen(3); }, nullptr},
        {Str::MgSpiegelModus, Str::MgSpiegelModusHint, "", "", 2,
         [] {
             zuWeights();
             gw().spiegelModus = !gw().spiegelModus;
         },
         [] { return gw().spiegelModus; }},
        {Str::MgBlauGruen, Str::MgBlauGruenHint, "", "", 2, [] { (void)modellGewichteSpiegelnSeite(1); }, nullptr},
        {Str::MgGruenBlau, Str::MgGruenBlauHint, "", "", 2, [] { (void)modellGewichteSpiegelnSeite(-1); }, nullptr},
        {Str::MgTauschen, Str::MgTauschenHint, "", "", 2, [] { (void)modellGewichteSpiegelnSeite(0); }, nullptr},
        {Str::MgNurAuswahl, Str::MgNurAuswahlHint, "", "", 2, [] { gw().nurAuswahl = !gw().nurAuswahl; }, [] { return gw().nurAuswahl; }},
        {Str::MgGruppeSpiegeln, Str::MgGruppeSpiegelnHint, "", "", 2, [] { (void)modellGewichteSpiegelnBlender(false); }, nullptr},
        {Str::MgAlleGruppen, Str::MgAlleGruppenHint, "", "", 2, [] { (void)modellGewichteSpiegelnBlender(true); }, nullptr},
        {Str::MgSperre, Str::MgSperreHint, "", "", 3, [] { modellGewichteSperren(0, 1); }, [] { return modellGewichtGesperrt(mt().knochenWahl); }},
        {Str::MgUnlockAlle, Str::MgSperreHint, "", "", 3, [] { modellGewichteSperren(2, 0); }, nullptr},
        {Str::MgSperrMenue, Str::MgSperrMenueHint, "", "K", 3,
         [] {
             zuWeights();
             gw().sperrMenueOeffnen = true;
         },
         nullptr},
    };
    return kListe;
}

const char* tasteVon(const GwAktion& a) { return modellBlender() ? a.blenderTaste : a.maxTaste; }

void ausloesen(const GwAktion& a) {
    a.tun();
    ++mt().tastenZaehler;
    mt().letzteTaste = tr(a.text);
    g_app->modelDirty = true;
}

std::string tooltip(const GwAktion& a) {
    std::string s = tr(a.hinweis);
    const char* k = tasteVon(a);
    if (k[0] != '\0') { s += "\n" + text(Str::UvTasteHint, k); }
    return s;
}

// --- Das rechte Feld -------------------------------------------------------------------------------------------------------

// Die Festwerte des Weight Tools von 3ds Max (Autodesk-Hilfe: 0, .1, .25, .5, .75, .9, 1).
constexpr float kFest[7] = {0.0F, 0.1F, 0.25F, 0.5F, 0.75F, 0.9F, 1.0F};

// Ein Festwert: das Gewicht des Pinsels; ist das Malen aus und sind Ecken gewaehlt, steht es sofort an ihnen (wie in Max).
void festwertSetzen(float w) {
    ModellTab& t = mt();
    t.pinselGewicht = w;
    bool da = false;
    (void)modellGewichteAuswahl(&da);
    if (!gw().malen && da && knochenBereit()) { melde(text(Str::MgGeaendert, modellAufAuswahlSetzen())); }
}

void festwerte() {
    ModellTab& t = mt();
    static const char* const kTexte[7] = {"0", ".1", ".25", ".5", ".75", ".9", "1"};
    const char* hinweis = tr(Str::MgFestHint);
    const char* hinweise[7] = {hinweis, hinweis, hinweis, hinweis, hinweis, hinweis, hinweis};
    int wahl = -1;
    for (int k = 0; k < 7; ++k) {
        if (std::fabs(t.pinselGewicht - kFest[k]) < 1e-6F) { wahl = k; }
    }
    if (ui::segment("##mgfest", kTexte, 7, &wahl, 7, -1, hinweise) && wahl >= 0) { festwertSetzen(kFest[wahl]); }
}

// Der Pinsel: die Art als Wahl. Wie Blender steht Glaetten als eigenes Werkzeug "Weichzeichnen" im Raster, die Wahl hier ist
// die Mischung des Pinsels (Add, Subtract, Mix).
void pinselArtSegment(bool mitGlaetten) {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    static const modell::PinselArt kMit[4] = {modell::PinselArt::Hinzufuegen, modell::PinselArt::Abziehen, modell::PinselArt::Glaetten, modell::PinselArt::Setzen};
    static const modell::PinselArt kOhne[3] = {modell::PinselArt::Hinzufuegen, modell::PinselArt::Abziehen, modell::PinselArt::Setzen};
    static const Str kText[4] = {Str::MbGwHinzu, Str::MbGwAbziehen, Str::MbGwGlaetten, Str::MbGwSetzen};
    static const Str kHint[4] = {Str::MbGwHinzuHint, Str::MbGwAbziehenHint, Str::MbGwGlaettenHint, Str::MbGwSetzenHint};
    const modell::PinselArt* arten = mitGlaetten ? kMit : kOhne;
    const int zahl = mitGlaetten ? 4 : 3;
    const char* texte[4]{};
    const char* hinweise[4]{};
    int wahl = -1;
    for (int k = 0; k < zahl; ++k) {
        texte[k] = tr(kText[static_cast<int>(arten[k])]);
        hinweise[k] = tr(kHint[static_cast<int>(arten[k])]);
        if (g.werkzeug == 0 && t.pinselArt == static_cast<int>(arten[k])) { wahl = k; }
    }
    if (ui::segment("##mgart", texte, zahl, &wahl, zahl, -1, hinweise) && wahl >= 0) {
        t.pinselArt = static_cast<int>(arten[wahl]);
        g.werkzeug = 0;
    }
}

void verlaufArtSegment() {
    ModellGewichte& g = gw();
    if (g.werkzeug != 2) { return; }
    const char* texte[2] = {tr(Str::MgVerlaufLinear), tr(Str::MgVerlaufRadial)};
    (void)ui::segment("##mgverlaufart", texte, 2, &g.verlaufArt, 2);
}

void hauptKnopf(Str name) {
    ModellGewichte& g = gw();
    // der Hauptknopf ist gefuellt, solange gemalt wird (hoechstens ein Hauptknopf je Gruppe)
    // AP14: der Hauptknopf schaltet um (KnopfOpt::umschalter) - er behaelt Platz und Hoehe, aus zeigt er nur den Rahmen in der
    // Betonungsfarbe (bis dahin war er aus ein gewoehnlicher, niedrigerer Knopf und der Inhalt darunter sprang)
    const std::string hinweis = std::string(tr(Str::MgMalenHint)) + " " + tr(Str::FxStrichStrgZ);   // FIX612B P2-10
    if (ui::werkzeugKnopf(tr(name), {.icon = UiIcon::GwPinsel, .hinweis = hinweis.c_str(), .haupt = true, .an = g.malen, .umschalter = true})) {
        modellGewichteMalenSetzen(!g.malen);
    }
    if (!hatKnochen()) { ui::absatz(tr(Str::MbGwKnochenWaehlen), ui::Ton::Warnung); }
}

// wie 3ds Max: Gewichts-Werkzeug (Paint Weights, Weight Tool)
void rolloutWerkzeugMax() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRWerkzeug), {.kurzinfo = hatKnochen() ? knochenName(t.knochenWahl) : nullptr})) { return; }
    hauptKnopf(Str::MgMalenMax);
    festwerte();
    (void)ui::regler(tr(Str::MbGwGewicht), &t.pinselGewicht, 0.0F, 1.0F, "%.3f", tr(Str::MgFestHint));
    ui::rasterBeginn(2);
    const bool bereit = hatKnochen();
    if (ui::werkzeugKnopf(tr(Str::MbGwSetzen), {.icon = UiIcon::GwSetzen, .hinweis = tr(Str::MbGwAufAuswahlHint), .aktiv = bereit})) {
        if (knochenBereit()) { melde(text(Str::MgGeaendert, modellAufAuswahlSetzen())); }
    }
    if (ui::werkzeugKnopf(tr(Str::MgSkalieren), {.icon = UiIcon::GwSkalieren, .hinweis = tr(Str::MgSkalierenHint), .aktiv = bereit})) { (void)modellGewichteSkalieren(); }
    if (ui::werkzeugKnopf(tr(Str::MgRing), {.icon = UiIcon::GwRing, .hinweis = tr(Str::MgRingHint)})) { (void)modellGewichteWahl(2); }
    if (ui::werkzeugKnopf(tr(Str::MgSchleife), {.icon = UiIcon::GwSchleife, .hinweis = tr(Str::MgSchleifeHint)})) { (void)modellGewichteWahl(3); }
    if (ui::werkzeugKnopf(tr(Str::MgWachsen), {.icon = UiIcon::GwWachsen, .hinweis = tr(Str::MgWachsenHint)})) { (void)modellGewichteWahl(0); }
    if (ui::werkzeugKnopf(tr(Str::MgSchrumpfen), {.icon = UiIcon::GwSchrumpfen, .hinweis = tr(Str::MgSchrumpfenHint)})) { (void)modellGewichteWahl(1); }
    if (ui::werkzeugKnopf(tr(Str::MgMischen), {.icon = UiIcon::GwMischen, .hinweis = tr(Str::MgMischenHint)})) { (void)modellGewichteMischen(); }
    ui::rasterEnde();
    // die Optionen des Pinsels (3ds Max: Painter Options) und der Faktor von "Scale Weight"
    if (ui::mehrBeginn(tr(Str::MgMalOptionen))) {
        pinselArtSegment(true);
        ui::rasterBeginn(2);
        if (ui::werkzeugKnopf(tr(Str::MgVerwischen), {.icon = UiIcon::GwVerwischen, .hinweis = tr(Str::MgVerwischenHint), .an = g.werkzeug == 1})) {
            modellGewichteWerkzeug(g.werkzeug == 1 ? 0 : 1);
        }
        if (ui::werkzeugKnopf(tr(Str::MgVerlauf), {.icon = UiIcon::GwVerlauf, .hinweis = tr(Str::MgVerlaufHint), .an = g.werkzeug == 2})) {
            modellGewichteWerkzeug(g.werkzeug == 2 ? 0 : 2);
        }
        ui::rasterEnde();
        verlaufArtSegment();
        (void)ui::regler(tr(Str::MbGwRadius), &t.pinselRadius, 2.0F, 400.0F, "%.0f px");
        (void)ui::regler(tr(Str::MbGwStaerke), &t.pinselStaerke, 0.0F, 1.0F, "%.2f");
        (void)ui::schalter(tr(Str::MbGwSpiegelMalen), &t.pinselSpiegeln, tr(Str::MbGwSpiegelHint));
        (void)ui::regler(tr(Str::MgSkalierFaktor), &g.skalieren, 0.5F, 1.5F, "%.2f", tr(Str::MgSkalierenHint));
        ui::mehrEnde();
    }
    ui::tipp(tr(Str::MbGwTastenMax));
    ui::rolloutEnde();
}

// wie Blender: Pinsel (die Werkzeuge der Werkzeugleiste des Weight Paint)
void rolloutPinselBl() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRPinsel), {.kurzinfo = hatKnochen() ? knochenName(t.knochenWahl) : nullptr, .reiter = modellReiter(MbReiter::Aendern, MbReiterBl::Werkzeug)})) {
        return;
    }
    hauptKnopf(Str::MgMalenBl);
    (void)ui::regler(tr(Str::MbGwGewicht), &t.pinselGewicht, 0.0F, 1.0F, "%.3f", tr(Str::MgFestHint));
    (void)ui::regler(tr(Str::MbGwRadius), &t.pinselRadius, 2.0F, 400.0F, "%.0f px");
    (void)ui::regler(tr(Str::MbGwStaerke), &t.pinselStaerke, 0.0F, 1.0F, "%.2f");
    pinselArtSegment(false);
    ui::rasterBeginn(2);
    const bool weich = g.werkzeug == 0 && t.pinselArt == static_cast<int>(modell::PinselArt::Glaetten);
    if (ui::werkzeugKnopf(tr(Str::MgWeich), {.icon = UiIcon::GwMischen, .hinweis = tr(Str::MbGwGlaettenHint), .an = weich})) {
        // noch einmal: zurueck zum Malen (Setzen = die Mischung "Mix" von Blender)
        t.pinselArt = static_cast<int>(weich ? modell::PinselArt::Setzen : modell::PinselArt::Glaetten);
        g.werkzeug = 0;
    }
    if (ui::werkzeugKnopf(tr(Str::MgVerlauf), {.icon = UiIcon::GwVerlauf, .hinweis = tr(Str::MgVerlaufHint), .an = g.werkzeug == 2})) { modellGewichteWerkzeug(g.werkzeug == 2 ? 0 : 2); }
    if (ui::werkzeugKnopf(tr(Str::MrGwAufnehmen), {.icon = UiIcon::GwTropfen, .taste = "Shift+X", .hinweis = tr(Str::MrGwAufnehmenHint), .an = t.gwAufnehmenKlick, .tasteHalten = true})) {
        t.gwAufnehmenKlick = !t.gwAufnehmenKlick;
        if (t.gwAufnehmenKlick) { melde(tr(Str::MrGwAufnehmenKlick)); }
    }
    if (ui::werkzeugKnopf(tr(Str::MgVerwischen), {.icon = UiIcon::GwVerwischen, .hinweis = tr(Str::MgVerwischenHint), .an = g.werkzeug == 1})) { modellGewichteWerkzeug(g.werkzeug == 1 ? 0 : 1); }
    ui::rasterEnde();
    verlaufArtSegment();
    ui::tipp(tr(Str::MrGwTastenBlender));
    ui::rolloutEnde();
}

void vierFest() {
    // "Hoechstens 4 je Ecke (JKA)" ist fest an: die Grenze des Spiels laesst sich nicht abwaehlen
    bool an = true;
    (void)ui::schalter(tr(Str::MgVier), &an, tr(Str::MgVierHint), false);
}

void rolloutOptionenBl() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgROptionen), {.reiter = modellReiter(MbReiter::Aendern, MbReiterBl::Werkzeug)})) { return; }
    ui::rasterBeginn(2);
    (void)ui::schalter(tr(Str::MgAutoNorm), &g.autoNormalisieren, tr(Str::MgAutoNormHint));
    (void)ui::schalter(tr(Str::MgXSpiegeln), &t.pinselSpiegeln, tr(Str::MbGwSpiegelHint));
    vierFest();
    ui::rasterEnde();
    ui::rolloutEnde();
}

void bereinigenKnoepfe(bool blender) {
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MgNormAlle), {.icon = UiIcon::GwSumme, .hinweis = tr(Str::MgNormAlleHint)})) { (void)modellGewichteBereinigen(0); }
    if (ui::werkzeugKnopf(tr(blender ? Str::MgSaeubern : Str::MgSaeubernMax), {.icon = UiIcon::GwBesen, .hinweis = tr(Str::MgSaeubernHint)})) { (void)modellGewichteBereinigen(1); }
    if (blender && ui::werkzeugKnopf(tr(Str::MbGwGlaetten), {.icon = UiIcon::GwMischen, .hinweis = tr(Str::MgGlaettenAlleHint)})) {
        (void)modellGewichteBereinigen(2);
    }
    if (ui::werkzeugKnopf(tr(blender ? Str::MgBegrenzen : Str::MgBegrenzenMax), {.icon = UiIcon::GwGrenze, .hinweis = tr(Str::MgBegrenzenHint)})) { (void)modellGewichteBereinigen(3); }
    ui::rasterEnde();
}

void rolloutBereinigenBl() {
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRBereinigen), {})) { return; }
    bereinigenKnoepfe(true);
    if (ui::mehrBeginn((std::string(tr(Str::MgROptionen)) + "###mgbereinigenmehr").c_str())) {
        (void)ui::regler(tr(Str::MgSaeubernGrenze), &g.saeubernGrenze, 0.0F, 0.5F, "%.3f", tr(Str::MgSaeubernGrenzeHint));
        (void)ui::regler(tr(Str::MgGlaettenFaktor), &g.glaettenFaktor, 0.0F, 1.0F, "%.2f", tr(Str::MgGlaettenAlleHint));
        (void)ui::reglerInt(tr(Str::MgGlaettenRunden), &g.glaettenRunden, 1, 10, "%d", tr(Str::MgGlaettenAlleHint));
        ui::mehrEnde();
    }
    ui::rolloutEnde();
}

// wie 3ds Max: was im Rollout "Advanced Parameters" des Skin-Modifiers steht (Normalize, Remove Zero Weights, Bone Affect Limit)
void rolloutErweitertMax() {
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRErweitert), {.zu = true})) { return; }
    ui::rasterBeginn(2);
    (void)ui::schalter(tr(Str::MgNormMax), &g.autoNormalisieren, tr(Str::MgAutoNormHint));
    vierFest();
    ui::rasterEnde();
    bereinigenKnoepfe(false);
    (void)ui::regler(tr(Str::MgSaeubernGrenzeMax), &g.saeubernGrenze, 0.0F, 0.5F, "%.3f", tr(Str::MgSaeubernGrenzeHint));
    ui::rolloutEnde();
}

// Die Tabelle des Spiegelns (beide Modi): Knochenpaare, Ecken ohne Gegenstueck, flaechenuebergreifende Paare; der Tipp.
void spiegelTabelle() {
    const ModellTab& t = mt();
    const ModellGewichte& g = gw();
    const modell::Inhalt& in = t.dok.inhalt();
    const modell::SpiegelBefund& b = g.befund;
    char paare[48];
    std::snprintf(paare, sizeof paare, "%d / %d", b.knochenPaare, b.knochenPaareSoll);
    const std::string ohne = b.eckenOhne > 0 ? text(Str::MgOhneWert, b.eckenOhne) : std::string("0");
    // die Flaechenpaare, deren Namen sich nur in der Seite unterscheiden (l_arm <-> r_arm): das erste, dahinter die Zahl der uebrigen
    std::vector<std::string> echte;
    for (const std::pair<int, int>& p : b.teilPaare) {
        if (uz(p.first) >= in.teile.size() || uz(p.second) >= in.teile.size()) { continue; }
        const std::string& a = in.teile[uz(p.first)].name;
        const std::string& c = in.teile[uz(p.second)].name;
        int anders = a.size() == c.size() ? 0 : 99;
        for (std::size_t i = 0; i < a.size() && anders < 2; ++i) {
            if (a[i] == c[i]) { continue; }
            const int x = std::tolower(static_cast<unsigned char>(a[i]));
            const int y = std::tolower(static_cast<unsigned char>(c[i]));
            anders += ((x == 'l' && y == 'r') || (x == 'r' && y == 'l')) ? 1 : 99;
        }
        if (anders == 1) { echte.push_back(a + " <-> " + c); }
    }
    std::string ueber = "-";
    if (echte.size() == 1U) { ueber = echte.front(); }
    if (echte.size() > 1U) { ueber = text(Str::MgUeberMehr, echte.front().c_str(), static_cast<int>(echte.size()) - 1); }
    // im schmalen Feld nur die Zahl (die Namen brachen sonst mitten im Wort um); alle Paare stehen im Tooltip der Zeile
    if (!echte.empty() && ImGui::CalcTextSize(ueber.c_str()).x > ImGui::GetContentRegionAvail().x * 0.5F) { ueber = text(Str::MgUeberZahl, static_cast<int>(echte.size())); }
    std::string paarListe;
    for (std::size_t i = 0; i < echte.size() && i < 16U; ++i) { paarListe += (i > 0 ? "\n" : "") + echte[i]; }
    ui::wertTabelle({{tr(Str::MgPaare), paare, b.knochenPaare == b.knochenPaareSoll ? ui::Ton::Gut : ui::Ton::Warnung, {}},
                     {tr(Str::MgOhne), ohne, b.eckenOhne > 0 ? ui::Ton::Warnung : ui::Ton::Gut, tr(Str::MgSpiegelModusHint)},
                     {tr(Str::MgUeberFlaechen), ueber, echte.empty() ? ui::Ton::Leise : ui::Ton::Gut, paarListe}});
    const modell::Grenzen& gr = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    ui::tipp(text(Str::MgSpiegelTipp, gr.knochenJeFlaeche).c_str());
}

void rolloutSpiegelnMax() {
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRSpiegeln), {.kurzinfo = tr(Str::MgInfoLR)})) { return; }
    if (!mt().malStrich) { modellGewichteSpiegelStand(); }
    (void)ui::schalter(tr(Str::MgSpiegelModus), &g.spiegelModus, tr(Str::MgSpiegelModusHint));
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MgBlauGruen), {.icon = UiIcon::GwNachRechts, .hinweis = tr(Str::MgBlauGruenHint)})) { (void)modellGewichteSpiegelnSeite(1); }
    if (ui::werkzeugKnopf(tr(Str::MgGruenBlau), {.icon = UiIcon::GwNachLinks, .hinweis = tr(Str::MgGruenBlauHint)})) { (void)modellGewichteSpiegelnSeite(-1); }
    if (ui::werkzeugKnopf(tr(Str::MgTauschen), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MgTauschenHint)})) { (void)modellGewichteSpiegelnSeite(0); }
    if (ui::werkzeugKnopf(tr(Str::MgNurAuswahl), {.icon = UiIcon::Zielen, .hinweis = tr(Str::MgNurAuswahlHint), .an = g.nurAuswahl})) { g.nurAuswahl = !g.nurAuswahl; }
    ui::rasterEnde();
    (void)ui::regler(tr(Str::MgToleranz), &g.toleranz, 0.01F, 2.0F, "%.2f", tr(Str::MgToleranzHint));
    spiegelTabelle();
    ui::rolloutEnde();
}

void rolloutSpiegelnBl() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (!ui::rolloutBeginn(tr(Str::MgRSpiegeln), {.kurzinfo = tr(Str::MgInfoLR)})) { return; }
    if (!t.malStrich) { modellGewichteSpiegelStand(); }
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::MgGruppeSpiegeln), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MgGruppeSpiegelnHint), .aktiv = hatKnochen()})) { (void)modellGewichteSpiegelnBlender(false); }
    if (ui::werkzeugKnopf(tr(Str::MgAlleGruppen), {.icon = UiIcon::Tauschen, .hinweis = tr(Str::MgAlleGruppenHint)})) { (void)modellGewichteSpiegelnBlender(true); }
    ui::rasterEnde();
    ui::rasterBeginn(2);
    (void)ui::schalter(tr(Str::MgBlGewichte), &g.blGewichte, tr(Str::MgBlGewichteHint));
    (void)ui::schalter(tr(Str::MgBlNamen), &g.blNamen, tr(Str::MgBlNamenHint));
    (void)ui::schalter(tr(Str::MgBlTopologie), &g.blTopologie, tr(Str::MgBlTopologieHint));
    (void)ui::schalter(tr(Str::MbGwSpiegelMalen), &t.pinselSpiegeln, tr(Str::MbGwSpiegelHint));
    ui::rasterEnde();
    // nicht im Entwurf: ohne Toleranz findet eine Figur des Spiels kaum Gegenstuecke (Blender sucht auf 0,00002 genau)
    (void)ui::regler(tr(Str::MgToleranz), &g.toleranz, 0.01F, 2.0F, "%.2f", tr(Str::MgToleranzHint), 0, !g.blTopologie);
    spiegelTabelle();
    ui::rolloutEnde();
}

void rolloutAutoSkin() {
    if (!ui::rolloutBeginn(tr(Str::AsGruppe), {})) { return; }
    drawModellAutoSkinGruppe();
    ui::rolloutEnde();
}

void rolloutKnochen() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    const GlaAnimation& a = g_app->anim;
    const bool da = hatKnochen();
    if (!ui::rolloutBeginn(tr(Str::MgRKnochen), {.kurzinfo = da ? knochenName(t.knochenWahl) : nullptr, .reiter = modellReiter(MbReiter::Hierarchie, MbReiterBl::Knochen)})) { return; }
    ui::zeile(tr(Str::MgGewaehlt));
    if (ImGui::BeginCombo("##gwknochen", da ? knochenName(t.knochenWahl) : "-")) {
        for (std::size_t b = 0; b < a.bones.size(); ++b) {
            const std::string zeile = a.bones[b].name + (modellGewichtGesperrt(static_cast<int>(b)) ? " *" : "");
            if (ImGui::Selectable(zeile.c_str(), static_cast<int>(b) == t.knochenWahl)) {
                t.knochenWahl = static_cast<int>(b);
                g_app->modelDirty = true;
            }
        }
        ImGui::EndCombo();
    }
    // die Pose: Sequenz und Bild (gezaehlt wie die Zeitleiste des Bedienmodus: ab 0 wie 3ds Max, ab 1 wie Blender)
    std::string pose = "-";
    if (g_app->animIndex >= 0 && uz(g_app->animIndex) < g_app->animList.size()) {
        const AnimEntry& e = g_app->animList[uz(g_app->animIndex)];
        pose = text(Str::MgPoseWert, e.name.c_str(), g_app->animFrame - e.firstFrame + (modellBlender() ? 1 : 0));
    }
    ui::wertTabelle({{tr(Str::MgPose), pose, ui::Ton::Normal, {}}});
    sperrListeAnpassen();
    ui::rasterBeginn(2);
    bool gesperrt = modellGewichtGesperrt(t.knochenWahl);
    ImGui::BeginDisabled(!da);
    if (ui::schalter(tr(Str::MgSperre), &gesperrt, tr(Str::MgSperreHint)) && da) { g.gesperrt[uz(t.knochenWahl)] = gesperrt ? 1 : 0; }
    ImGui::EndDisabled();
    if (ui::schalter(tr(Str::MgKnochenZeigen), &g_app->modellKnochen, tr(Str::MgKnochenZeigenHint))) { g_app->modelDirty = true; }
    ui::rasterEnde();
    const int zahl = gesperrteZahl();
    if (zahl > 0) {
        ui::absatz(text(Str::MgGesperrtZahl, zahl).c_str());
        for (std::size_t b = 0; b < g.gesperrt.size() && b < a.bones.size(); ++b) {
            if (g.gesperrt[b] == 0) { continue; }
            ImGui::PushID(static_cast<int>(b));
            const bool weg = ui::eintrag(a.bones[b].name.c_str(), tr(Str::MgEntsperren));
            ImGui::PopID();
            if (weg) { g.gesperrt[b] = 0; }
        }
    }
    if (ui::werkzeugKnopf(tr(Str::MgSperrMenue), {.icon = zahl > 0 ? UiIcon::Schloss : UiIcon::SchlossOffen, .taste = modellBlender() ? "K" : nullptr, .hinweis = tr(Str::MgSperrMenueHint), .tasteHalten = true})) {
        g.sperrMenueOeffnen = true;
    }
    ui::tipp(tr(Str::MgKnochenTipp));
    ui::rolloutEnde();
}

void rolloutGrenzen() {
    ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const modell::Grenzen& gr = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel));
    const int stufe = t.lodJetzt;
    // je Flaeche, was das Spiel bekaeme (die Anzeige traegt die Gewichte des Spiels, auch waehrend des Malens)
    int flaechen = 0;
    int ueber4 = 0;
    int malend = 0;
    int gewichteMax = 0;
    int meisteKnochen = -1;   // die Flaeche mit den meisten Knochen
    int meisteEcken = -1;     // ... mit den meisten Ecken
    std::vector<int> knochenJe(in.teile.size(), 0);
    std::vector<std::string> ueber32;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (in.teile[i].istTag() || uz(stufe) >= in.teile[i].stufen.size()) { continue; }
        const modell::Netz& n = in.teile[i].stufen[uz(stufe)];
        ++flaechen;
        ueber4 += modell::eckenUeberVier(n);
        malend += modell::malt(n) ? 1 : 0;
        if (modell::malt(n)) {
            for (const modell::GewichtListe& l : n.mal) { gewichteMax = std::max(gewichteMax, static_cast<int>(l.size())); }
        } else {
            gewichteMax = std::max(gewichteMax, modell::mass(n).gewichteMax);
        }
        if (i < t.anzeige.surfaces.size()) {
            std::vector<char> benutzt(256, 0);
            for (const GlmVertex& v : t.anzeige.surfaces[i].verts) {
                for (int j = 0; j < 4; ++j) {
                    if (v.weights[j] > 0.0F) { benutzt[v.bones[j]] = 1; }
                }
            }
            knochenJe[i] = static_cast<int>(std::count(benutzt.begin(), benutzt.end(), static_cast<char>(1)));
        }
        if (meisteKnochen < 0 || knochenJe[i] > knochenJe[uz(meisteKnochen)]) { meisteKnochen = static_cast<int>(i); }
        if (meisteEcken < 0 || n.ecken.size() > in.teile[uz(meisteEcken)].stufen[uz(stufe)].ecken.size()) { meisteEcken = static_cast<int>(i); }
        if (knochenJe[i] > gr.knochenJeFlaeche) { ueber32.push_back(in.teile[i].name + " (" + std::to_string(knochenJe[i]) + ")"); }
    }
    // die Balken gelten der gewaehlten Flaeche (Entwurf: "torso"), ohne Auswahl der, die der Grenze am naechsten kommt
    int gewaehlt = -1;
    for (const int i : t.auswahl) {
        if (i >= 0 && uz(i) < in.teile.size() && !in.teile[uz(i)].istTag() && uz(stufe) < in.teile[uz(i)].stufen.size()) {
            gewaehlt = i;
            break;
        }
    }
    const int fuerKnochen = gewaehlt >= 0 ? gewaehlt : meisteKnochen;
    const int fuerEcken = gewaehlt >= 0 ? gewaehlt : meisteEcken;
    const bool fehler = ueber4 > 0 || !ueber32.empty();
    const int knochenHier = fuerKnochen >= 0 ? knochenJe[uz(fuerKnochen)] : 0;
    const std::string info = fuerKnochen >= 0 ? text(Str::MgGrInfo, knochenHier, gr.knochenJeFlaeche) : std::string(tr(Str::MfInfoGut));
    const ui::Ton ton = fehler ? ui::Ton::Fehler : (knochenHier * 10 >= gr.knochenJeFlaeche * 9 && knochenHier < gr.knochenJeFlaeche ? ui::Ton::Warnung : ui::Ton::Gut);
    const std::string pruef = text(Str::MbGwPruef, gr.name, gr.gewichteJeEcke, gr.knochenJeFlaeche);
    if (!ui::rolloutBeginn(tr(Str::MgRGrenzen), {.kurzinfo = info.c_str(), .ton = ton, .reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Objekt), .hinweis = pruef.c_str()})) { return; }
    if (fuerEcken >= 0) {
        ui::grenzBalken(text(Str::MgGrEcken, in.teile[uz(fuerEcken)].name.c_str()).c_str(), static_cast<int>(in.teile[uz(fuerEcken)].stufen[uz(stufe)].ecken.size()),
                        gr.eckenJeFlaeche, pruef.c_str());
    }
    if (fuerKnochen >= 0) { ui::grenzBalken(text(Str::MgGrKnochen, in.teile[uz(fuerKnochen)].name.c_str()).c_str(), knochenHier, gr.knochenJeFlaeche, pruef.c_str()); }
    ui::grenzBalken(tr(Str::MgGrGewichte), gewichteMax, gr.gewichteJeEcke, tr(Str::MgVierHint));
    ui::wertTabelle({{tr(Str::MgGrFlaechen), std::to_string(flaechen), ui::Ton::Normal, {}}});
    if (ueber4 > 0) { ui::absatz(text(Str::MbGwUeber4, ueber4).c_str(), ui::Ton::Warnung); }
    for (const std::string& n : ueber32) { ui::absatz(text(Str::MbGwUeber32, n.c_str(), gr.knochenJeFlaeche).c_str(), ui::Ton::Fehler); }
    if (!fehler) { ui::absatz(tr(Str::MbGwPruefOk), ui::Ton::Gut); }
    // FIX612B P2-2: der Hinweis sagt, dass Speichern das Ergebnis auch ohne Uebernehmen schreibt
    const std::string uebernehmenHinweis = std::string(tr(Str::MbGwUebernehmenHint)) + " " + tr(Str::FxUebernehmenSpeichern);
    if (ui::werkzeugKnopf(tr(Str::MbGwUebernehmen), {.icon = UiIcon::Haken, .hinweis = uebernehmenHinweis.c_str(), .aktiv = malend > 0})) { (void)modellUebernehmen(); }
    ui::rolloutEnde();
}

// Das Sperren-Menue (Blender: K, VIEW3D_MT_wpaint_vgroup_lock_pie) - hier eine Liste an der Maus
void sperrMenue() {
    ModellGewichte& g = gw();
    if (g.sperrMenueOeffnen) {
        ImGui::OpenPopup("###mgsperren");
        g.sperrMenueOeffnen = false;
    }
    g.sperrMenueSichtbar = ImGui::BeginPopup("###mgsperren");
    if (!g.sperrMenueSichtbar) { return; }
    struct Zeile {
        Str text;
        int aktion;
        int maske;
        bool brauchtKnochen;
    };
    // die acht Eintraege des Menues von Blender: alle, der gewaehlte, die nicht gewaehlten, nur ..., umkehren
    static const Zeile kZeilen[8] = {{Str::MgLockAlle, 1, 0, false},     {Str::MgUnlockAlle, 2, 0, false}, {Str::MgLockGew, 1, 1, true},
                                     {Str::MgUnlockGew, 2, 1, true},     {Str::MgLockAndere, 1, 2, true},  {Str::MgLockNurGew, 1, 3, true},
                                     {Str::MgLockNurAndere, 2, 3, true}, {Str::MgLockUmkehren, 3, 0, false}};
    cjkTextDisabled("%s", tr(Str::MgGrSperren));
    for (const Zeile& z : kZeilen) {
        if (ImGui::MenuItem(tr(z.text), nullptr, false, !z.brauchtKnochen || hatKnochen())) { modellGewichteSperren(z.aktion, z.maske); }
        ImGui::SetItemTooltip("%s", tr(Str::MgSperreHint));
    }
    ImGui::EndPopup();
}

void alsMenue() {
    static const Str kGruppen[4] = {Str::MgRWerkzeug, Str::MgRBereinigen, Str::MgRSpiegeln, Str::MgGrSperren};
    const auto& liste = aktionen();
    int gruppe = -1;
    bool unter = false;
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const GwAktion& a = liste[i];
        if (a.gruppe != gruppe) {
            if (unter) { ImGui::EndMenu(); }
            gruppe = a.gruppe;
            unter = ImGui::BeginMenu(tr(kGruppen[uz(gruppe)]));
            if (!unter) {
                while (i + 1 < liste.size() && liste[i + 1].gruppe == gruppe) { ++i; }
                continue;
            }
        }
        ImGui::PushID(static_cast<int>(i));
        const char* k = tasteVon(a);
        if (ImGui::MenuItem(tr(a.text), k[0] != '\0' ? k : nullptr, a.an ? a.an() : false)) { ausloesen(a); }
        ImGui::SetItemTooltip("%s", tooltip(a).c_str());
        ImGui::PopID();
    }
    if (unter) { ImGui::EndMenu(); }
}

}  // namespace

ModellGewichte& modellGewichte() {
    static ModellGewichte z;
    return z;
}

modell::MalRegeln modellGewichteRegeln() {
    sperrListeAnpassen();
    modell::MalRegeln r;
    r.gesperrt = gw().gesperrt;
    r.autoNormalisieren = gw().autoNormalisieren;
    return r;
}

bool modellGewichtGesperrt(int knochen) {
    sperrListeAnpassen();   // FIX612B P2-7: nach dem Laden eines anderen Modells schon beim ersten Tupfer
    const ModellGewichte& g = gw();
    return knochen >= 0 && uz(knochen) < g.gesperrt.size() && g.gesperrt[uz(knochen)] != 0;
}

void modellGewichteSperren(int aktion, int maske) {
    sperrListeAnpassen();
    ModellGewichte& g = gw();
    const int gewaehlt = mt().knochenWahl;
    // VGROUP_MASK_SELECTED (1) / _UNSELECTED (2); _ALL (0) und _INVERT_UNSELECTED (3) gelten in der Schleife jedem
    const auto betrifft = [&](int i, int m) { return m == 1 ? i == gewaehlt : (m == 2 ? i != gewaehlt : true); };
    const int zahl = static_cast<int>(g.gesperrt.size());
    if (aktion == 0) {   // VGROUP_TOGGLE: entsperren, wenn einer der Betroffenen gesperrt ist, sonst sperren
        aktion = 1;
        for (int i = 0; i < zahl; ++i) {
            if (betrifft(i, maske == 3 ? 1 : maske) && g.gesperrt[uz(i)] != 0) {
                aktion = 2;
                break;
            }
        }
    }
    for (int i = 0; i < zahl; ++i) {
        if (!betrifft(i, maske)) { continue; }
        switch (aktion) {
            case 1: g.gesperrt[uz(i)] = 1; break;
            case 2: g.gesperrt[uz(i)] = 0; break;
            case 3: g.gesperrt[uz(i)] = g.gesperrt[uz(i)] != 0 ? 0 : 1; break;
            default: break;
        }
        if (maske == 3 && i != gewaehlt) { g.gesperrt[uz(i)] = g.gesperrt[uz(i)] != 0 ? 0 : 1; }
    }
    melde(text(Str::MgGesperrtZahl, gesperrteZahl()));
}

void modellGewichteBereichSchritt() {
    ModellGewichte& g = gw();
    const bool drin = mt().bereich == ModellBereich::Weights;
    if (drin == g.warDrin) { return; }
    // der Schalter "Bones" merkt sich seinen Stand je Bereich: im Bereich Weights ab Werk an (Entwurf: Knochen sichtbar)
    if (drin) {
        g.knochenAussen = g_app->modellKnochen;
        g_app->modellKnochen = g.knochenInnen;
    } else {
        g.knochenInnen = g_app->modellKnochen;
        g_app->modellKnochen = g.knochenAussen;
    }
    g.warDrin = drin;
    g_app->modelDirty = true;
}

void modellGewichteMalenSetzen(bool an) {
    gw().malen = an;
    g_app->modelDirty = true;
}

void modellGewichteWerkzeug(int werkzeug) {
    ModellGewichte& g = gw();
    g.werkzeug = std::clamp(werkzeug, 0, 2);
    if (g.werkzeug != 0) { g.malen = true; }   // ein Werkzeug waehlen heisst malen
    g_app->modelDirty = true;
}

modell::EckenMaske modellGewichteAuswahl(bool* da) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    std::vector<modell::EckOrt> ecken;
    if (t.auswahlArt == AuswahlArt::Objekt) {
        for (const int i : t.auswahl) {
            if (i < 0 || uz(i) >= in.teile.size() || in.teile[uz(i)].istTag() || uz(t.lodJetzt) >= in.teile[uz(i)].stufen.size()) { continue; }
            const std::size_t zahl = in.teile[uz(i)].stufen[uz(t.lodJetzt)].ecken.size();
            for (std::size_t k = 0; k < zahl; ++k) { ecken.push_back(modell::EckOrt{i, static_cast<int>(k)}); }
        }
    } else {
        ecken = t.eckenWahl;
    }
    if (da != nullptr) { *da = !ecken.empty(); }
    return modell::eckenMaske(in, t.lodJetzt, ecken);
}

int modellGewichteWahl(int art) {
    ModellTab& t = mt();
    if (t.auswahlArt == AuswahlArt::Objekt || t.eckenWahl.empty()) {
        melde(tr(Str::MgKeineAuswahl));
        return 0;
    }
    const modell::Inhalt& in = t.dok.inhalt();
    switch (art) {
        case 0: t.eckenWahl = modell::eckenWachsen(in, t.lodJetzt, t.eckenWahl); break;
        case 1: t.eckenWahl = modell::eckenSchrumpfen(in, t.lodJetzt, t.eckenWahl); break;
        case 2: t.eckenWahl = modell::eckenRing(in, t.lodJetzt, t.eckenWahl); break;
        default: t.eckenWahl = modell::eckenSchleife(in, t.lodJetzt, t.eckenWahl); break;
    }
    const int zahl = static_cast<int>(t.eckenWahl.size());
    gw().letzteZahl = zahl;
    melde(text(Str::MgGewaehltZahl, zahl));
    return zahl;
}

int modellAufAuswahlSetzen() {
    ModellTab& t = mt();
    if (!hatKnochen() || modellGewichtGesperrt(t.knochenWahl)) { return 0; }
    const modell::EckenMaske m = modellGewichteAuswahl();
    const modell::MalRegeln r = modellGewichteRegeln();
    const int stufe = t.lodJetzt;
    const int knochen = t.knochenWahl;
    const float wert = t.pinselGewicht;
    return befehl(Str::MbUndoSetzen, [&](modell::Inhalt& x) { return modell::gewichtSetzen(x, stufe, m, knochen, wert, r); });
}

int modellGewichteSkalieren() {
    ModellTab& t = mt();
    if (!knochenBereit()) { return 0; }
    bool da = false;
    const modell::EckenMaske m = modellGewichteAuswahl(&da);
    if (!da) {
        melde(tr(Str::MgKeineAuswahl));
        return 0;
    }
    const modell::MalRegeln r = modellGewichteRegeln();
    const int stufe = t.lodJetzt;
    const int knochen = t.knochenWahl;
    const float faktor = gw().skalieren;
    const int n = befehl(Str::MgUndoSkalieren, [&](modell::Inhalt& x) { return modell::gewichtSkalieren(x, stufe, m, knochen, faktor, r); });
    melde(text(Str::MgGeaendert, n));
    return n;
}

int modellGewichteMischen() {
    ModellTab& t = mt();
    bool da = false;
    const modell::EckenMaske m = modellGewichteAuswahl(&da);
    if (!da) {
        melde(tr(Str::MgKeineAuswahl));
        return 0;
    }
    // 3ds Max haelt die Summe der Gewichte einer Ecke immer bei 1 ("the total weight ... is always 1.0")
    modell::MalRegeln r = modellGewichteRegeln();
    r.autoNormalisieren = true;
    const int stufe = t.lodJetzt;
    const int n = befehl(Str::MgUndoMischen, [&](modell::Inhalt& x) { return modell::glaettenAlle(x, stufe, &m, 0.5F, 1, r); });
    melde(text(Str::MgGeaendert, n));
    return n;
}

int modellGewichteBereinigen(int art) {
    ModellTab& t = mt();
    const ModellGewichte& g = gw();
    // wie Blender mit Auswahlmaske: mit gewaehlten Ecken nur diese, sonst alle
    const bool mitWahl = t.auswahlArt != AuswahlArt::Objekt && !t.eckenWahl.empty();
    const modell::EckenMaske m = mitWahl ? modell::eckenMaske(t.dok.inhalt(), t.lodJetzt, t.eckenWahl) : modell::EckenMaske{};
    const modell::EckenMaske* wahl = mitWahl ? &m : nullptr;
    const modell::MalRegeln r = modellGewichteRegeln();
    const int stufe = t.lodJetzt;
    const int aktiv = t.knochenWahl;
    const int vier = modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel)).gewichteJeEcke;
    int n = 0;
    switch (art) {
        case 0:
            n = befehl(Str::MgUndoNorm, [&](modell::Inhalt& x) { return modell::normalisierenAlle(x, stufe, wahl, aktiv, true, r); });
            melde(n < 0 ? std::string(tr(Str::MgAlleGesperrt)) : text(Str::MgGeaendert, n));
            break;
        case 1:
            n = befehl(Str::MgUndoSaeubern, [&](modell::Inhalt& x) { return modell::saeubern(x, stufe, wahl, g.saeubernGrenze, true, r); });
            melde(text(Str::MgEntfernt, n));
            break;
        case 2:
            n = befehl(Str::MgUndoGlaetten, [&](modell::Inhalt& x) { return modell::glaettenAlle(x, stufe, wahl, g.glaettenFaktor, g.glaettenRunden, r); });
            melde(text(Str::MgGeaendert, n));
            break;
        default:
            n = befehl(Str::MgUndoBegrenzen, [&](modell::Inhalt& x) { return modell::begrenzen(x, stufe, wahl, vier, r); });
            melde(text(Str::MgEntfernt, n));
            break;
    }
    return n;
}

void modellGewichteSpiegelStand() {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    const bool topologie = g.blTopologie && modellBlender();
    char schluessel[120];
    std::snprintf(schluessel, sizeof schluessel, "%d|%d|%d|%.4f|%d|%zu", t.dok.ladeStand(), t.dok.stand(), t.lodJetzt, static_cast<double>(g.toleranz), topologie ? 1 : 0,
                  g_app->anim.bones.size());
    if (g.spiegelSchluessel == schluessel) { return; }
    const modell::Inhalt& in = t.dok.inhalt();
    const std::vector<std::string> namen = knochenNamen();
    g.netz = modell::schweissen(in, t.lodJetzt);
    g.partner = topologie ? modell::spiegelPartnerTopologie(g.netz) : modell::spiegelPartner(in, t.lodJetzt, g.netz, g.toleranz);
    g.befund = modell::spiegelBefund(namen, g.netz, g.partner);
    g.links = modell::linkeSeite(in, t.lodJetzt, namen);
    g.spiegelSchluessel = schluessel;
}

int modellSpiegeln(int quelle) {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    modellGewichteSpiegelStand();
    const std::vector<std::string> namen = knochenNamen();
    const int stufe = t.lodJetzt;
    const modell::EckenMaske m = g.nurAuswahl ? modellGewichteAuswahl() : modell::EckenMaske{};
    const modell::EckenMaske* wahl = g.nurAuswahl ? &m : nullptr;
    // Seiten nach dem Vorzeichen von x; zur Mitte gehoert ein Punkt nur, wenn er sein eigener Partner ist
    const int n = befehl(Str::MbUndoSpiegeln, [&](modell::Inhalt& x) { return modell::spiegelnSeite(x, stufe, namen, g.netz, g.partner, quelle, 0.0F, wahl); });
    grenzeMelden(text(Str::MgGespiegelt, n, g.befund.eckenOhne));
    return n;
}

int modellGewichteSpiegelnSeite(int richtung) {
    ModellGewichte& g = gw();
    modellGewichteSpiegelStand();
    if (richtung == 0) { return modellSpiegeln(0); }
    // Blau ist die linke Seite der Figur (3ds Max: links blau, rechts gruen); wo sie liegt, sagen die Gewichte
    const int links = g.links != 0 ? g.links : 1;
    const int n = modellSpiegeln(richtung > 0 ? links : -links);
    if (g.links == 0) { melde(mt().meldung + " " + tr(Str::MgLinksUnklar)); }
    return n;
}

int modellGewichteSpiegelnBlender(bool alleGruppen) {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (!alleGruppen && !hatKnochen()) {
        melde(tr(Str::MbGwKnochenWaehlen));
        return 0;
    }
    modellGewichteSpiegelStand();
    const std::vector<std::string> namen = knochenNamen();
    const int stufe = t.lodJetzt;
    // die Auswahlmaske von Blender: gewaehlte Ecken; ohne sie gelten alle als gewaehlt (beide Seiten tauschen)
    const bool mitWahl = t.auswahlArt != AuswahlArt::Objekt && !t.eckenWahl.empty();
    const modell::EckenMaske m = mitWahl ? modell::eckenMaske(t.dok.inhalt(), stufe, t.eckenWahl) : modell::EckenMaske{};
    modell::BlenderSpiegel o;
    o.gewichte = g.blGewichte;
    o.namenTauschen = g.blNamen;
    o.alleGruppen = alleGruppen;
    o.aktiv = t.knochenWahl;
    modell::SpiegelZahl zahl;
    (void)befehl(Str::MbUndoSpiegeln, [&](modell::Inhalt& x) {
        zahl = modell::spiegelnBlender(x, stufe, namen, g.netz, g.partner, o, mitWahl ? &m : nullptr);
        return zahl.gespiegelt;
    });
    grenzeMelden(text(Str::MgGespiegelt, zahl.gespiegelt, zahl.ohnePartner));
    return zahl.gespiegelt;
}

bool modellUebernehmen() {
    ModellTab& t = mt();
    int n = 0;
    t.dok.aendern(tr(Str::MbUndoUebernehmen), [&](modell::Inhalt& x) { n = modell::uebernehmen(x); });
    melde(text(Str::MbGwUebernommen, n));
    return n > 0;
}

void modellGewichteTupferAnwenden(modell::Inhalt& x, const ModellAnsicht& a, const std::vector<std::vector<modell::Tupfer>>& jeTeil, int knochen, int spiegelKn,
                                  int art, float wert) {
    const ModellTab& t = mt();
    const ModellGewichte& g = gw();
    const modell::MalRegeln r = modellGewichteRegeln();
    const int stufe = t.lodJetzt;
    ModellEckenBild bild;
    if (g.werkzeug == 1) { modellEckenBild(a, bild); }
    // die Bewegung des Pinsels in den Achsen der Bildebene (rechts, oben); im Bild zeigt y nach unten
    const float richtung[2] = {g.bewegung[0], -g.bewegung[1]};
    for (std::size_t i = 0; i < jeTeil.size() && i < x.teile.size(); ++i) {
        if (jeTeil[i].empty() || uz(stufe) >= x.teile[i].stufen.size()) { continue; }
        modell::Netz& n = x.teile[i].stufen[uz(stufe)];
        if (g.werkzeug == 1) {
            if (i < bild.ebene.size()) { (void)modell::verwischen(n, jeTeil[i], knochen, bild.ebene[i], richtung, r); }
        } else {
            (void)modell::malen(n, jeTeil[i], knochen, static_cast<modell::PinselArt>(art), wert, r);
        }
    }
    if (t.pinselSpiegeln) { (void)modell::spiegelMalen(x, stufe, g.netz, g.partner, jeTeil, knochen, spiegelKn, r); }
}

bool modellGewichteVerlaufSchritt(int ansicht, ImVec2 maus, bool neu) {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (ansicht < 0 || ansicht >= 4 || !hatKnochen()) { return false; }
    if (neu) {
        if (modellGewichtGesperrt(t.knochenWahl)) {
            melde(tr(Str::MgGesperrtAktiv));
            return false;
        }
        g.verlaufSchritt = t.dok.aendern(tr(Str::MgUndoVerlauf), [](modell::Inhalt&) {});
        t.malStrich = true;
        g.verlaufZieht = true;
        g.verlaufGeaendert = false;
        g.verlaufAnsicht = ansicht;
        g.verlaufStart[0] = g.verlaufEnde[0] = maus.x;
        g.verlaufStart[1] = g.verlaufEnde[1] = maus.y;
        // die Mallisten vom Beginn: jede Bewegung der Maus rechnet den Verlauf von dort neu (Blender: weight_orig) - mit den
        // Bildorten vom Beginn: die Pose zeigt die Ecken mit den Gewichten, die der Verlauf gerade aendert; Blender merkt sich
        // die Orte beim ersten Ereignis (paint_weight.cc, vert_cache->elem[].sco)
        modellEckenBild(t.ansichten[ansicht], g.verlaufBild);
        const modell::Inhalt& in = t.dok.inhalt();
        g.verlaufVorher.assign(in.teile.size(), {});
        g.verlaufHatte.assign(in.teile.size(), 0);
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (uz(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
            const modell::Netz& n = in.teile[i].stufen[uz(t.lodJetzt)];
            g.verlaufHatte[i] = modell::malt(n) ? 1 : 0;
            g.verlaufVorher[i] = n.mal;
        }
        t.letzterTupfer[0] = maus.x;
        t.letzterTupfer[1] = maus.y;
        return true;
    }
    if (!g.verlaufZieht || ansicht != g.verlaufAnsicht) { return false; }
    g.verlaufEnde[0] = maus.x;
    g.verlaufEnde[1] = maus.y;
    t.letzterTupfer[0] = maus.x;
    t.letzterTupfer[1] = maus.y;
    const ModellEckenBild& bild = g.verlaufBild;
    const modell::MalRegeln r = modellGewichteRegeln();
    const int stufe = t.lodJetzt;
    const int knochen = t.knochenWahl;
    const auto mischung = static_cast<modell::PinselArt>(std::clamp(t.pinselArt, 0, modell::kPinselArten - 1));
    const auto va = g.verlaufArt == 1 ? modell::VerlaufArt::Radial : modell::VerlaufArt::Linear;
    int geaendert = 0;
    t.dok.vorschau([&](modell::Inhalt& x) {
        for (std::size_t i = 0; i < x.teile.size() && i < g.verlaufVorher.size(); ++i) {
            if (uz(stufe) >= x.teile[i].stufen.size()) { continue; }
            modell::Netz& n = x.teile[i].stufen[uz(stufe)];
            n.mal = g.verlaufVorher[i];
            if (i >= bild.ort.size() || bild.ort[i].size() < n.ecken.size()) { continue; }
            const int z = modell::verlauf(n, bild.ort[i], bild.imBild[i], g.verlaufStart, g.verlaufEnde, va, knochen, mischung, t.pinselGewicht, t.pinselStaerke, r);
            if (z == 0 && g.verlaufHatte[i] == 0) { n.mal.clear(); }   // nichts geaendert: die Flaeche bleibt ohne Malliste
            geaendert += z;
        }
    });
    g.verlaufGeaendert = geaendert > 0;
    g.letzteZahl = geaendert;
    t.gemalteEcken = geaendert;
    ++t.tupferZahl;
    g_app->modelDirty = true;
    return true;
}

void modellGewichteStrichEnde() {
    ModellGewichte& g = gw();
    if (!g.verlaufZieht) { return; }
    g.verlaufZieht = false;
    g.verlaufVorher.clear();
    g.verlaufHatte.clear();
    g.verlaufBild = ModellEckenBild{};
    if (!g.verlaufGeaendert) { (void)mt().dok.abbrechen(g.verlaufSchritt); }   // ein Klick ohne Strecke: kein leerer Schritt
    g_app->modelDirty = true;
}

void modellGewichteUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    ModellGewichte& g = gw();
    if (g.zaehlBild != ImGui::GetFrameCount()) {
        g.zaehlBild = ImGui::GetFrameCount();
        for (int& z : g.spiegelPunkte) { z = 0; }
        g.verlaufStriche = 0;
    }
    if (t.bereich != ModellBereich::Weights) { return; }
    if (g.verlaufZieht && g.verlaufAnsicht >= 0 && g.verlaufAnsicht < 4 && &a == &t.ansichten[g.verlaufAnsicht]) {
        const ImVec2 von{g.verlaufStart[0], g.verlaufStart[1]};
        const ImVec2 bis{g.verlaufEnde[0], g.verlaufEnde[1]};
        dl->AddLine(von, bis, IM_COL32(0, 0, 0, 200), 3.5F);
        dl->AddLine(von, bis, IM_COL32(255, 255, 255, 235), 1.5F);
        dl->AddCircleFilled(von, 4.0F, IM_COL32(255, 255, 255, 235));
        dl->AddCircle(bis, 4.0F, IM_COL32(255, 255, 255, 235), 16, 1.5F);
        ++g.verlaufStriche;
    }
    if (!g.spiegelModus) { return; }
    if (!t.malStrich) { modellGewichteSpiegelStand(); }
    const modell::Inhalt& in = t.dok.inhalt();
    ModellEckenBild bild;
    modellEckenBild(a, bild);
    const int links = g.links != 0 ? g.links : 1;
    const float halb = std::max(1.5F, std::floor(ImGui::GetFontSize() * 0.1F));
    // wie der Spiegelmodus von 3ds Max: links blau, rechts gruen; ohne Gegenstueck gelb (Entwurf), Punkte der Mitte hell
    static const ImU32 kFarbe[4] = {IM_COL32(70, 130, 255, 235), IM_COL32(70, 215, 95, 235), IM_COL32(255, 215, 40, 255), IM_COL32(215, 215, 215, 200)};
    for (int p = 0; p < g.netz.zahl() && uz(p) < g.partner.size(); ++p) {
        const int q = g.partner[uz(p)];
        const modell::EckOrt erste = g.netz.ecken[uz(p)].front();
        if (uz(erste.teil) >= in.teile.size() || uz(t.lodJetzt) >= in.teile[uz(erste.teil)].stufen.size() ||
            uz(erste.ecke) >= in.teile[uz(erste.teil)].stufen[uz(t.lodJetzt)].ecken.size()) {
            continue;
        }
        const float x = in.teile[uz(erste.teil)].stufen[uz(t.lodJetzt)].ecken[uz(erste.ecke)].ort[0];
        const int art = q < 0 ? 2 : (q == p ? 3 : ((x > 0.0F ? 1 : -1) == links ? 0 : 1));
        for (const modell::EckOrt& o : g.netz.ecken[uz(p)]) {
            if (uz(o.teil) >= bild.ort.size() || uz(o.ecke) >= bild.ort[uz(o.teil)].size() || bild.imBild[uz(o.teil)][uz(o.ecke)] == 0 ||
                bild.vorn[uz(o.teil)][uz(o.ecke)] == 0) {
                continue;
            }
            const ImVec2 sp{bild.ort[uz(o.teil)][uz(o.ecke)][0], bild.ort[uz(o.teil)][uz(o.ecke)][1]};
            const float r = art == 2 ? halb + 1.0F : halb;
            dl->AddRectFilled(ImVec2{sp.x - r, sp.y - r}, ImVec2{sp.x + r, sp.y + r}, kFarbe[art]);
            // dunkler Rand: die blauen Punkte stehen sonst blau auf der blauen Figur (Gewicht 0)
            dl->AddRect(ImVec2{sp.x - r - 1.0F, sp.y - r - 1.0F}, ImVec2{sp.x + r + 1.0F, sp.y + r + 1.0F}, IM_COL32(0, 0, 0, 220));
            ++g.spiegelPunkte[art];
            break;   // ein Punkt je Ort
        }
    }
}

bool modellGewichteEigeneTasten() {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Weights || !modellBlender() || ImGui::GetIO().WantTextInput) { return false; }
    // Blender km_weight_paint: K ruft das Menue "Vertex Group Locks" (wm.call_menu_pie VIEW3D_MT_wpaint_vgroup_lock_pie)
    if (ImGui::IsKeyChordPressed(ImGuiKey_K)) {
        gw().sperrMenueOeffnen = true;
        ++t.tastenZaehler;
        t.letzteTaste = tr(Str::MgSperrMenue);
        g_app->modelDirty = true;
        return true;
    }
    return false;
}

int modellGewichteAktionZahl() { return static_cast<int>(aktionen().size()); }
int modellGewichteAktionGruppe(int i) { return aktionen()[uz(i)].gruppe; }
const char* modellGewichteAktionText(int i) { return tr(aktionen()[uz(i)].text); }
const char* modellGewichteAktionHinweis(int i) { return tr(aktionen()[uz(i)].hinweis); }
const char* modellGewichteAktionTaste(int i, bool blender) { return blender ? aktionen()[uz(i)].blenderTaste : aktionen()[uz(i)].maxTaste; }
void modellGewichteAktionAusloesen(int i) { ausloesen(aktionen()[uz(i)]); }

bool modellGewichteAktionenSuche(const std::string& such) {
    bool ausgeloest = false;
    for (const GwAktion& a : aktionen()) {
        std::string zeile = tr(a.text);
        const char* k = tasteVon(a);
        if (k[0] != '\0') { zeile += std::string(" (") + k + ")"; }
        if (!such.empty() && klein(zeile).find(klein(such)) == std::string::npos) { continue; }
        ImGui::PushID(&a);
        if (modellAktionZeile(zeile.c_str(), static_cast<int>(ModellBereich::Weights)) && !ausgeloest) {
            ausloesen(a);
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", tooltip(a).c_str());
        ImGui::PopID();
    }
    return ausgeloest;
}

void modellGewichteKontextMenue() {
    if (g_app->model.empty()) { return; }
    if (ImGui::BeginMenu(tr(Str::MgMenue))) {
        alsMenue();
        ImGui::EndMenu();
    }
}

void drawModellGewichte() {
    sperrMenue();
    // AP13S (W1): der gemeinsame Modifier-Stapel - wie 3ds Max zuoberst im Reiter Aendern (Skin gewaehlt, ueber Edit Poly), wie
    // Blender im Reiter Modifier (gui/app_modelltab_stapel.cpp)
    drawModellStapelGruppe();
    if (modellBlender()) {
        rolloutPinselBl();
        rolloutOptionenBl();
        rolloutBereinigenBl();
        rolloutSpiegelnBl();
    } else {
        rolloutWerkzeugMax();
        rolloutSpiegelnMax();
    }
    rolloutAutoSkin();
    rolloutKnochen();
    rolloutGrenzen();
    if (!modellBlender()) { rolloutErweitertMax(); }
}

}  // namespace bhed::gui
