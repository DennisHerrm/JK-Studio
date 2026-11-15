// app_karte_cwimport.cpp - Karten-Editor, CW-IMPORT: Teil 2 der Clone-Wars-Kette (eigene Funktionen, nicht in NetRadiant-Custom;
// plaene/clonewars_kette.md). Alles ab Werk aus bzw. nur auf Aufruf (shank 14.11.: "zwar drin, aber nicht standard").
//
// Punkt 1: die Testfigur (View > Testfigur, Werkzeugleiste, F7; Shift+F7 setzt sie unter den Zeiger, im Rechtsklickmenue der
// Ansichten "Testfigur hierher"): der Kasten der Engine (karte::testfigurZeichnung) in den 2D-Ansichten und der Kamera, dazu das
// Spielermodell in der Pose von PM_Footsteps ohne Bewegung (BOTH_STAND1 / geduckt BOTH_CROUCH1), am Ursprung der Figur wie
// CG_Player (OpenJK code/cgame/cg_players.cpp:7178-7184: ent.origin = lerpOrigin). Gezeichnet ueber kartegpu (Direct3D 12 und 11).
//
// Punkte 3 bis 6 (Modify > Clone-Wars-Werkzeuge, oben im Rechtsklickmenue der Ansichten): Raum skalieren (Dialog), Treppe zu
// Clip-Rampe, glatte Clip-Wand (Ebene/Huelle), Ebene Collision ein/aus - die Rechnung in src/karte/cwwerkzeuge.cpp.
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_gpu.h"
#include "textumbruch.h"

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/cwimport.h"
#include "bhed/karte/cwwerkzeuge.h"
#include "bhed/karte/ebenen.h"
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <cstdio>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

karte::Spielfigur figur() { return karte::spielfigur(optimiererRegelwerk()); }

karte::FigurLage lage() {
    const TestfigurZustand& t = e().testfigur;
    karte::FigurLage l;
    l.ursprung = t.ursprung;
    l.geduckt = t.geduckt;
    l.quer = t.quer;
    l.marken = t.marken;
    return l;
}

// Farben je Art (2D und Kamera gleich): Kasten gelb-orange wie eine Figur, Hoehe hellblau, Stufe gruen, Tuer violett, Gang grau
karte::Farbe farbeVon(karte::FigurArt a) {
    switch (a) {
    case karte::FigurArt::Kasten: return karte::farbeAus(1.0F, 0.62F, 0.15F);
    case karte::FigurArt::Hoehe: return karte::farbeAus(0.45F, 0.80F, 1.0F);
    case karte::FigurArt::Stufe: return karte::farbeAus(0.40F, 0.95F, 0.45F);
    case karte::FigurArt::Tuer: return karte::farbeAus(0.85F, 0.55F, 1.0F);
    case karte::FigurArt::Gang: return karte::farbeAus(0.70F, 0.70F, 0.70F);
    case karte::FigurArt::Ursprung: return karte::farbeAus(1.0F, 1.0F, 1.0F);
    }
    return karte::farbeAus(1, 1, 1);
}

// Das Spielermodell in der Pose (einmal je Modell und Pose geladen; Dreiecke im Raum der Figur)
struct FigurModell {
    std::string schluessel;   // Pfad|Pose|Spiel
    std::vector<kartegpu::Punkt3> dreiecke;
    std::string fehler;
};
FigurModell g_modell;

const FigurModell& modellHolen() {
    Editor& ed = e();
    TestfigurZustand& t = ed.testfigur;
    const std::shared_ptr<Spielkontext> sp = ed.karteSpiel;
    const std::string schluessel = t.modellPfad + "|" + (t.geduckt ? "1" : "0") + "|" + std::to_string(reinterpret_cast<std::uintptr_t>(sp.get()));
    if (g_modell.schluessel == schluessel) { return g_modell; }
    g_modell = FigurModell{};
    g_modell.schluessel = schluessel;
    if (!sp || t.modellPfad.empty()) { return g_modell; }
    std::string bytes;
    if (!sp->fs.dateiLesen(t.modellPfad, bytes)) {
        g_modell.fehler = "missing " + t.modellPfad;
        return g_modell;
    }
    karte::Modell m;
    const karte::ModellDateiLeser leser = [sp](const std::string& p, std::string& b) { return sp->fs.dateiLesen(p, b); };
    int bild = -1;
    if (!karte::glmLesenInPose(bytes, leser, karte::testfigurSequenz(t.geduckt), m, &g_modell.fehler, &bild)) { return g_modell; }
    t.modellBild = bild;
    for (const karte::ModellFlaeche& f : m.flaechen) {
        for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
            // Q3-Dreiecke laufen im Uhrzeigersinn (die Vorderseite der Kamera ist hier gegen den Uhrzeiger): umgedreht
            for (const std::size_t k : {i, i + 2, i + 1}) {
                const karte::ModellPunkt& q = f.punkte[f.indizes[k]];
                kartegpu::Punkt3 p{};
                for (int a = 0; a < 3; ++a) {
                    p.pos[a] = q.ort[a];
                    p.normale[a] = q.normale[a];
                }
                p.farbe = 0xFFFFFFFFU;
                g_modell.dreiecke.push_back(p);
            }
        }
    }
    return g_modell;
}

}  // namespace

// --- Punkt 1: Testfigur ------------------------------------------------------------------------------------------------------------

void testfigurUmschalten() {
    Editor& ed = e();
    TestfigurZustand& t = ed.testfigur;
    t.an = !t.an;
    if (t.an && !t.gesetzt) {   // das erste Mal: 128 vor der Kamera, die Fuesse auf der Flaeche darunter (sonst auf Augenhoehe - 64)
        const karte::Kamera& k = ed.kamera;
        const karte::Vektor3f vor = k.ursprung + k.vpn * 128.0F;
        t.ursprung = karte::Vektor3f(vor.x(), vor.y(), k.ursprung.z() - 64.0F - static_cast<float>(figur().unten));
        t.gesetzt = true;
        // Boden unter dem Punkt (Szene_Intersect der Kamera geht nur unter dem Zeiger; hier die Mitte des Bilds)
        const KameraFenster& kf = ed.kamFenster;
        if (kf.breite > 0 && kf.hoehe > 0) {
            const karte::GeraeteVektor eps(8.F / static_cast<float>(k.breite), 8.F / static_cast<float>(k.hoehe));
            const karte::Vektor3f treffer = karte::szeneSchnitt(ed.karte, ed.blicke[3], k.geraetePunkt(static_cast<float>(kf.breite) * 0.5F,
                                                                                                       static_cast<float>(kf.hoehe) * 0.75F), eps);
            t.ursprung = karte::testfigurAufBoden(treffer, figur());
        }
    }
    einstellungenSpeichern();
    ++ed.sichtStand;
}

void testfigurGeduckt(bool an) {
    e().testfigur.geduckt = an;
    einstellungenSpeichern();
    ++e().sichtStand;
}

bool testfigurHierher(ImVec2 punkt) {
    Editor& ed = e();
    TestfigurZustand& t = ed.testfigur;
    for (std::size_t i = 0; i < ed.xy.size(); ++i) {
        XyFenster& f = ed.xy[i];
        const float bx = punkt.x - f.bildX;
        const float by = punkt.y - f.bildY;
        if (bx < 0 || by < 0 || bx >= static_cast<float>(f.ansicht.breite) || by >= static_cast<float>(f.ansicht.hoehe)) { continue; }
        karte::Vektor3f ort = f.ansicht.zuPunkt(static_cast<int>(bx), static_cast<int>(by), ed.raster.einrastGroesse());
        const auto achse = static_cast<std::size_t>(karte::blickachse(f.ansicht.typ));
        ort[achse] = t.ursprung[achse];   // die Tiefe der Ansicht bleibt
        t.ursprung = ort;
        t.gesetzt = true;
        t.an = true;
        ++ed.sichtStand;
        return true;
    }
    const KameraFenster& kf = ed.kamFenster;
    const float kx = punkt.x - kf.bildX, ky = punkt.y - kf.bildY;
    if (kx >= 0 && ky >= 0 && kx < static_cast<float>(kf.breite) && ky < static_cast<float>(kf.hoehe)) {
        karte::Kamera& k = ed.kamera;
        const karte::GeraeteVektor eps(8.F / static_cast<float>(k.breite), 8.F / static_cast<float>(k.hoehe));
        t.ursprung = karte::testfigurAufBoden(karte::szeneSchnitt(ed.karte, ed.blicke[3], k.geraetePunkt(kx, ky), eps), figur());
        t.gesetzt = true;
        t.an = true;
        ++ed.sichtStand;
        return true;
    }
    return false;
}

void testfigurMenueEintraege() {
    Editor& ed = e();
    TestfigurZustand& t = ed.testfigur;
    {
        const std::string id = std::string(trK(Str::KeCwFigur)) + "###cwfigur";
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Pruefbar;
        p.haken = t.an;
        p.taste = "F7";
        p.tooltip = trK(Str::KeCwFigurTip);
        if (qtPunkt(p)) { testfigurUmschalten(); }
    }
    {
        const std::string id = std::string(trK(Str::KeCwFigurGeduckt)) + "###cwfigurgeduckt";
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Pruefbar;
        p.haken = t.geduckt;
        p.tooltip = trK(Str::KeCwFigurGeducktTip);
        if (qtPunkt(p)) { testfigurGeduckt(!t.geduckt); }
    }
    {
        const std::string id = std::string(trK(Str::KeCwFigurMarken)) + "###cwfigurmarken";
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Pruefbar;
        p.haken = t.marken;
        p.tooltip = trK(Str::KeCwFigurMarkenTip);
        if (qtPunkt(p)) {
            t.marken = !t.marken;
            einstellungenSpeichern();
            ++ed.sichtStand;
        }
    }
    {
        const std::string id = std::string(trK(Str::KeCwFigurQuer)) + "###cwfigurquer";
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Pruefbar;
        p.haken = t.quer;
        p.tooltip = trK(Str::KeCwFigurQuerTip);
        if (qtPunkt(p)) {
            t.quer = !t.quer;
            ++ed.sichtStand;
        }
    }
    {
        const std::string id = std::string(trK(Str::KeCwFigurModell)) + "###cwfigurmodell";
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Pruefbar;
        p.haken = t.modellZeigen;
        p.tooltip = trK(Str::KeCwFigurModellTip);
        if (qtPunkt(p)) {
            t.modellZeigen = !t.modellZeigen;
            einstellungenSpeichern();
            ++ed.sichtStand;
        }
    }
    {
        const std::string id = std::string(trK(Str::KeCwFigurHier)) + "###cwfigurhier";
        QtPunkt p;
        p.id = id.c_str();
        p.taste = "Shift+F7";
        p.tooltip = trK(Str::KeCwFigurHierTip);
        if (qtPunkt(p)) {   // aus dem Menue: vor die Kamera (der Zeiger steht auf dem Menue)
            t.gesetzt = false;
            t.an = false;
            testfigurUmschalten();
        }
    }
}

void testfigurMenue() {
    const std::string id = std::string(trK(Str::KeCwFigurMenue)) + "###cwfigurmenue";
    if (qtMenueAnfang(id.c_str())) {
        testfigurMenueEintraege();
        qtMenueEnde();
    }
}

void testfigurPopupOeffnen() { e().testfigur.popupOeffnen = true; }

void testfigurPopupZeichnen() {
    TestfigurZustand& t = e().testfigur;
    if (t.popupOeffnen) {
        t.popupOeffnen = false;
        ImGui::OpenPopup("##cwfigurpopup");
    }
    if (qtPopupAnfang("##cwfigurpopup")) {
        testfigurMenueEintraege();
        qtPopupEnde();
    }
}

std::vector<karte::Linie> testfigurLinien2d(std::vector<std::pair<karte::Vektor3f, std::string>>* texte, std::vector<karte::Farbe>* textFarben) {
    std::vector<karte::Linie> v;
    ++e().testfigur.xyAufrufe;
    const TestfigurZustand& t = e().testfigur;
    if (!t.an) { return v; }
    const karte::FigurZeichnung z = karte::testfigurZeichnung(lage(), figur(), e().optimierer.masse);
    for (const karte::FigurLinie& l : z.linien) { v.push_back(karte::Linie{l.a, l.b, farbeVon(l.art)}); }
    if (texte != nullptr) {
        for (const karte::FigurText& x : z.texte) {
            texte->emplace_back(x.ort, x.text);
            if (textFarben != nullptr) { textFarben->push_back(farbeVon(x.art)); }
        }
    }
    ++e().testfigur.gezeichnet2d;
    return v;
}

void testfigurKameraZeichnen(const float* matrix, const float auge[3], const float vpn[3]) {
    Editor& ed = e();
    TestfigurZustand& t = ed.testfigur;
    if (!t.an) { return; }
    const karte::FigurZeichnung z = karte::testfigurZeichnung(lage(), figur(), ed.optimierer.masse);
    kartegpu::Zustand3 st;
    st.matrix = matrix;
    for (int a = 0; a < 3; ++a) {
        st.auge[a] = auge[a];
        st.vpn[a] = vpn[a];
    }
    // das Spielermodell, schattiert (Licht aus der Blickrichtung wie NRCs Kamera), in Grau
    t.modellDreiecke = 0;
    if (t.modellZeigen) {
        const FigurModell& m = modellHolen();
        if (!m.dreiecke.empty()) {
            std::vector<kartegpu::Punkt3> welt = m.dreiecke;
            for (kartegpu::Punkt3& p : welt) {
                p.pos[0] += t.ursprung.x();
                p.pos[1] += t.ursprung.y();
                p.pos[2] += t.ursprung.z();
            }
            kartegpu::Zustand3 d = st;
            d.licht = true;
            d.farbe[0] = 0.78F;
            d.farbe[1] = 0.80F;
            d.farbe[2] = 0.84F;
            d.tiefe = kartegpu::TiefenTest::KleinerGleich;
            d.tiefeSchreiben = true;
            d.wegschneiden = true;
            kartegpu::dreiecke(welt, d);
            t.modellDreiecke = welt.size() / 3;
        }
    }
    // die Linien: sichtbar voll, verdeckt gestrichelt (wie NRCs Overlay der Auswahl)
    std::vector<kartegpu::LinienPunkt> l;
    for (const karte::FigurLinie& x : z.linien) {
        for (const karte::Vektor3f& p : {x.a, x.b}) {
            kartegpu::LinienPunkt q{};
            q.pos[0] = p.x();
            q.pos[1] = p.y();
            q.pos[2] = p.z();
            q.farbe = farbeVon(x.art);
            l.push_back(q);
        }
    }
    kartegpu::Zustand3 sichtbar = st;
    sichtbar.tiefe = kartegpu::TiefenTest::KleinerGleich;
    sichtbar.tiefeSchreiben = false;
    sichtbar.wegschneiden = false;
    sichtbar.versatz = true;
    kartegpu::linien(l, sichtbar);
    kartegpu::Zustand3 verdeckt = sichtbar;
    verdeckt.tiefe = kartegpu::TiefenTest::Groesser;
    verdeckt.muster = 0xAAAAU;
    verdeckt.musterFaktor = 2;
    kartegpu::linien(l, verdeckt);
    ++t.gezeichnetKamera;
}

std::vector<float> testfigurLage() {
    const TestfigurZustand& t = e().testfigur;
    return {t.an ? 1.0F : 0.0F, t.geduckt ? 1.0F : 0.0F, t.marken ? 1.0F : 0.0F, t.quer ? 1.0F : 0.0F, t.modellZeigen ? 1.0F : 0.0F,
            t.ursprung.x(), t.ursprung.y(), t.ursprung.z(), static_cast<float>(g_modell.dreiecke.size())};
}

// --- Punkte 3 bis 6: Werkzeuge -------------------------------------------------------------------------------------------------------

namespace {

struct RaumDialog {
    bool offen = false;
    bool oeffnen = false;
    float faktor = 1.2F;
    char name[64] = "";
    karte::Vektor3d pivot{0, 0, 0};
};
RaumDialog g_raum;

void cwMeldung(const std::string& fehlerEnglisch) {
    char t[512];
    std::snprintf(t, sizeof(t), trK(Str::KeCwFehler), fehlerEnglisch.c_str());
    fehler(t);
    cwWerkzeugStand().meldung = t;
}

bool auswahlDa() { return e().karte.auswahl.anzahl() != 0; }

}  // namespace

CwWerkzeugStand& cwWerkzeugStand() {
    static CwWerkzeugStand s;
    return s;
}

bool cwTreppeZuRampe() {
    Editor& ed = e();
    if (bspRueckSperre()) { return false; }
    karte::TreppeOptionen o;
    o.nichtFest = karte::gemeinsamerShader("nodrawnonsolid");   // Jedi Academy: textures/system/nodraw (nodraw + nonsolid)
    o.gehen = karte::spielfigur(optimiererRegelwerk()).gehen;
    const karte::TreppeErgebnis r = karte::treppeZuRampe(ed.karte, o);
    if (!r.ok) {
        cwMeldung(r.fehler);
        return false;
    }
    char t[512];
    std::snprintf(t, sizeof(t), trK(Str::KeCwRampeFertig), static_cast<int>(r.tritte), karte::zahlText(static_cast<float>(r.steigung)).c_str(),
                  static_cast<int>(r.pinselNichtFest), static_cast<int>(r.modelleOhneClip), static_cast<int>(r.kollisionEntfernt));
    ausgabe(t);
    cwWerkzeugStand().meldung = t;
    if (!r.begehbar) { warnung(trK(Str::KeCwRampeSteil)); }
    ++cwWerkzeugStand().rampen;
    ++ed.sichtStand;
    auswahlGeaendert();
    return true;
}

bool cwClipWand(bool huelle) {
    Editor& ed = e();
    if (bspRueckSperre()) { return false; }
    const karte::Vektor3f& k = ed.kamera.ursprung;
    const karte::WandErgebnis r = karte::glatteClipWand(ed.karte, huelle ? karte::WandArt::Huelle : karte::WandArt::Ebene, karte::Vektor3d(k.x(), k.y(), k.z()),
                                                        karte::kClipSpieler);
    if (!r.ok) {
        cwMeldung(r.fehler);
        return false;
    }
    ++cwWerkzeugStand().waende;
    ++ed.sichtStand;
    auswahlGeaendert();
    return true;
}

bool cwKollisionSichtbar() {
    const Editor& ed = e();
    const int ebene = karte::kollisionsEbeneFinden(ed.karte);
    if (ebene < 0) { return true; }
    for (const auto& p : ed.karte.welt() ? ed.karte.welt()->kinder() : karte::Entitaet::Kinder{}) {
        if (p->schicht == ebene) { return !p->ebeneVersteckt; }
    }
    return true;
}

void cwKollisionZeigen(bool an) {
    Editor& ed = e();
    const int ebene = karte::kollisionsEbeneFinden(ed.karte);
    if (ebene < 0) { return; }
    karte::ebenenSichtbar(ed.karte, karte::ebenenUnterbaum(ed.karte, ebene), an);   // LayersSetVisible
    ++ed.sichtStand;
}

void cwRaumDialogOeffnen() {
    Editor& ed = e();
    if (!auswahlDa()) {
        cwMeldung("nothing selected");
        return;
    }
    // um die Mitte der Auswahl, unten (der Boden des Raums bleibt, wo er ist)
    const karte::Kasten b = karte::auswahlGrenzenMitEntitaeten(ed.karte);
    g_raum.pivot = karte::Vektor3d(b.origin.x(), b.origin.y(), b.origin.z() - b.extents.z());
    g_raum.offen = true;
    g_raum.oeffnen = true;
}

void cwRaumDialogZeichnen() {
    if (!g_raum.offen) { return; }
    Editor& ed = e();
    const std::string titel = std::string(trK(Str::KeCwRaum)) + "###cwraumdialog";
    if (g_raum.oeffnen) {
        g_raum.oeffnen = false;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    }
    bool offen = true;
    if (ui::fensterBegin(titel.c_str(), &offen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0F);
        ImGui::InputFloat((std::string(trK(Str::KeCwRaumFaktor)) + "##cwraumfaktor").c_str(), &g_raum.faktor, 0.05F, 0.1F, "%.3f");
        g_raum.faktor = std::clamp(g_raum.faktor, 0.05F, 20.0F);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0F);
        ImGui::InputText((std::string(trK(Str::KeCwRaumName)) + "##cwraumname").c_str(), g_raum.name, sizeof(g_raum.name));
        ImGui::TextDisabled("%s %s (%g %g %g)", trK(Str::KeCwRaumPunkt), trK(Str::KeCwRaumMitte), g_raum.pivot.x(), g_raum.pivot.y(), g_raum.pivot.z());
        cjkTextWrapped("%s", trK(Str::KeCwRaumTip));   // CJK zwischen den Zeichen umbrechen (gui/textumbruch.h)
        const auto los = [&](bool speichern) {
            karte::RaumOptionen o;
            o.faktor = static_cast<double>(g_raum.faktor);
            o.pivot = g_raum.pivot;
            o.name = g_raum.name;
            const karte::RaumErgebnis r = karte::raumSkalieren(ed.karte, o);
            if (!r.fehler.empty()) {
                cwMeldung(r.fehler);
                return;
            }
            char t[512];
            std::snprintf(t, sizeof(t), trK(Str::KeCwRaumFertig), karte::zahlText(g_raum.faktor).c_str(), static_cast<int>(r.primitive), static_cast<int>(r.entitaeten));
            ausgabe(t);
            cwWerkzeugStand().meldung = t;
            ++cwWerkzeugStand().raeume;
            ++ed.sichtStand;
            auswahlGeaendert();
            g_raum.offen = false;
            if (speichern) { (void)ausfuehren("SaveSelected"); }   // File > Save selected: die Auswahl samt Marker als eigenes Teil
        };
        if (ImGui::Button((std::string(trK(Str::KeCwRaumOk)) + "##cwraumok").c_str())) { los(false); }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeCwRaumSpeichern)) + "##cwraumspeichern").c_str())) { los(true); }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeGiAbbrechen)) + "##cwraumab").c_str())) { g_raum.offen = false; }
    }
    ImGui::End();
    if (!offen) { g_raum.offen = false; }
}

namespace {

// Die Eintraege der Werkzeuge (Modify-Menue und Rechtsklickmenue der Ansichten)
void werkzeugEintraege(bool kontext) {
    const bool da = auswahlDa();
    const auto punkt = [&](Str text, const char* id, Str tip, bool aktiv, const std::function<void()>& tun) {
        const std::string l = std::string(trK(text)) + id;
        QtPunkt p;
        p.id = l.c_str();
        p.aktiv = aktiv;
        p.tooltip = trK(tip);
        if (qtPunkt(p)) { tun(); }
    };
    punkt(Str::KeCwRampe, kontext ? "###cwrampek" : "###cwrampe", Str::KeCwRampeTip, da, [] { (void)cwTreppeZuRampe(); });
    punkt(Str::KeCwWandEbene, kontext ? "###cwwandebenek" : "###cwwandebene", Str::KeCwWandTip, da, [] { (void)cwClipWand(false); });
    punkt(Str::KeCwWandHuelle, kontext ? "###cwwandhuellek" : "###cwwandhuelle", Str::KeCwWandTip, da, [] { (void)cwClipWand(true); });
    punkt(Str::KeCwRaum, kontext ? "###cwraumk" : "###cwraum", Str::KeCwRaumTip, da, [] { cwRaumDialogOeffnen(); });
    if (kontext) { return; }
    qtTrenner();
    const std::string l = std::string(trK(Str::KeCwKollision)) + "###cwkollision";
    QtPunkt p;
    p.id = l.c_str();
    p.art = QtArt::Pruefbar;
    p.haken = cwKollisionSichtbar();
    p.aktiv = karte::kollisionsEbeneFinden(e().karte) >= 0;
    p.tooltip = trK(Str::KeCwKollisionTip);
    if (qtPunkt(p)) { cwKollisionZeigen(!cwKollisionSichtbar()); }
}

}  // namespace

void cwWerkzeugMenue() {
    const std::string id = std::string(trK(Str::KeCwWzMenue)) + "###cwwerkzeuge";
    if (qtMenueAnfang(id.c_str())) {
        werkzeugEintraege(false);
        qtMenueEnde();
    }
}

void cwWerkzeugKontext() {
    if (!auswahlDa()) { return; }
    const std::string id = std::string(trK(Str::KeCwWzMenue)) + "###cwwerkzeugek";
    if (qtMenueAnfang(id.c_str())) {
        werkzeugEintraege(true);
        qtMenueEnde();
    }
    qtTrenner();
}

}  // namespace bhed::gui::karteneditor
