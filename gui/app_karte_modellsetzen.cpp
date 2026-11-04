// app_karte_modellsetzen.cpp - Karten-Editor, rc609 M0: vorhandene Modelle mit einem Klick in die Karte (eigene
// Funktion, nicht in NetRadiant-Custom).
//
// Der Asset-Browser (Stufe 9 Punkt 11) setzt ein Modell beim Ziehen auf die Kamera oder eine 2D-Ansicht. Hier dazu:
//   Klasse      misc_model (q3map2 backt es ein, Lightmaps) oder misc_model_static (das Spiel zeichnet es zur Laufzeit,
//               nur .md3); je Spiel nur, was es gibt (bhed/modellsetzen.h: Jedi Outcast ohne misc_model_static)
//   Boden       der Strahl unter der Maus trifft die Spielkollision der Karte (kollisionAusKarte, CONTENTS_SOLID);
//               an einer Wand ein Stueck davor, dann senkrecht auf den Boden; der Ursprung so, dass die Unterkante des
//               Modells (localAABB, wie q3map2 es einsetzt: ModelGetTransform) darauf steht. Von oben (2D-Ansicht XY)
//               der oberste Boden.
//   Kollision   misc_model: spawnflags 2 (q3map2 model.cpp:359 eClipModel, :969-1000 clipflags_doClip);
//               misc_model_static: ein Clip-Brush (textures/common/clip) um den Kasten des Modells
//   Live        je Sekunde der Stand der Modelldateien der Karte (dort, wo das Dateisystem des Spiels sie findet); eine
//               neuere Fassung laedt der Editor neu, die Entities behalten ihre Schluessel.
#include "karteneditor.h"

#include "app_internal.h"

#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/karte/texdef.h"
#include "bhed/kollision.h"
#include "bhed/modellsetzen.h"
#include "bhed/regelwerk.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

struct SetzZustand {
    int klasse = 0;          // 0 misc_model, 1 misc_model_static
    bool kollision = false;
    std::map<std::string, std::pair<std::string, modellsetzen::DateiStand>> staende;   // Live: Fundort und Stand
    double uhr = -1.0;
};
SetzZustand& z() {
    static SetzZustand s;
    return s;
}

const std::shared_ptr<Spielkontext>& spielKontext() {
    const Editor& ed = e();
    return ed.karteSpiel ? ed.karteSpiel : ed.spiel;
}

int spielNr() {
    const auto& sp = spielKontext();
    return sp ? sp->spiel : kRegelwerkOpenJK;
}

const char* klassenName(int k) { return k == 1 ? "misc_model_static" : "misc_model"; }

bool endetMd3(const std::string& p) {
    if (p.size() < 4) { return false; }
    std::string t = p.substr(p.size() - 4);
    for (char& c : t) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return t == ".md3";
}

// Der Boden unter dem Bildschirmpunkt nach der Spielkollision (siehe oben). false: kein Boden.
bool bodenPunkt(ImVec2 punkt, const KollisionsKarte& k, float aus[3]) {
    Editor& ed = e();
    const float null[3] = {0.0F, 0.0F, 0.0F};
    float q[3] = {0.0F, 0.0F, 0.0F};
    bool gefunden = false;
    const KameraFenster& kf = ed.kamFenster;
    bool inAnsicht = punkt.x >= kf.bildX && punkt.y >= kf.bildY && punkt.x < kf.bildX + static_cast<float>(kf.breite) &&
                     punkt.y < kf.bildY + static_cast<float>(kf.hoehe);
    for (const XyFenster& f : ed.xy) {
        inAnsicht = inAnsicht || (punkt.x >= f.bildX && punkt.y >= f.bildY && punkt.x < f.bildX + static_cast<float>(f.ansicht.breite) &&
                                  punkt.y < f.bildY + static_cast<float>(f.ansicht.hoehe));
    }
    if (!inAnsicht) {   // Doppelklick im Browser: die Mitte der Kamera
        punkt = ImVec2{kf.bildX + static_cast<float>(kf.breite) * 0.5F, kf.bildY + static_cast<float>(kf.hoehe) * 0.5F};
    }
    const float kx = punkt.x - kf.bildX;
    const float ky = punkt.y - kf.bildY;
    if (kx >= 0 && ky >= 0 && kx < static_cast<float>(kf.breite) && ky < static_cast<float>(kf.hoehe)) {
        karte::Kamera& kam = ed.kamera;
        const karte::Vektor2f g = kam.geraetePunkt(kx, ky);
        const karte::Vektor3f p = kam.strahlOhneTreffer(g[0], g[1]);
        karte::Vektor3f r = p - kam.ursprung;
        const float l = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
        if (l <= 0.0F) { return false; }
        const float a[3] = {kam.ursprung[0], kam.ursprung[1], kam.ursprung[2]};
        const float b[3] = {a[0] + r[0] / l * 131072.0F, a[1] + r[1] / l * 131072.0F, a[2] + r[2] / l * 131072.0F};
        const KollisionsSpur s = kollisionsSpur(k, a, null, null, b, 1U);
        if (s.startsolid || s.fraction >= 1.0F) { return false; }
        for (int i = 0; i < 3; ++i) { q[i] = s.endpos[i]; }
        if (s.normale[2] <= 0.7F) {   // MIN_WALK_NORMAL: eine Wand - davor
            for (int i = 0; i < 3; ++i) { q[i] += s.normale[i] * 16.0F; }
        }
        gefunden = true;
    }
    for (std::size_t i = 0; i < ed.xy.size() && !gefunden; ++i) {
        const XyFenster& f = ed.xy[i];
        const float bx = punkt.x - f.bildX;
        const float by = punkt.y - f.bildY;
        if (bx < 0 || by < 0 || bx >= static_cast<float>(f.ansicht.breite) || by >= static_cast<float>(f.ansicht.hoehe)) { continue; }
        const karte::Vektor3f o = ablageOrt(punkt);
        for (int j = 0; j < 3; ++j) { q[j] = o[static_cast<std::size_t>(j)]; }
        if (f.ansicht.typ == karte::Sicht::XY) { q[2] = 65536.0F; }   // von oben: der oberste Boden
        gefunden = true;
    }
    if (!gefunden) { return false; }
    const float raster = ed.raster.einrastGroesse();
    if (raster > 0.0F) {
        q[0] = karte::zahlEingerastet(q[0], raster);
        q[1] = karte::zahlEingerastet(q[1], raster);
    }
    const float a[3] = {q[0], q[1], q[2] + 2.0F};
    const float b[3] = {q[0], q[1], q[2] - 131072.0F};
    const KollisionsSpur s = kollisionsSpur(k, a, null, null, b, 1U);
    if (s.startsolid || s.fraction >= 1.0F) { return false; }
    for (int i = 0; i < 3; ++i) { aus[i] = s.endpos[i]; }
    aus[2] = modellsetzen::bodenHoehe(s.endpos[2], s.normale[2]);
    return true;
}

}  // namespace

int& modellSetzKlasse() { return z().klasse; }
bool& modellSetzKollision() { return z().kollision; }

void modellSetzOptionen() {
    const std::vector<modellsetzen::KlassenAngebot> k = modellsetzen::modellKlassen(spielNr(), true);
    if (z().klasse == 1 && !k[1].da) { z().klasse = 0; }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 11.0F);
    if (ImGui::BeginCombo((std::string(trK(Str::KeM0Klasse)) + "##m0klasse").c_str(), klassenName(z().klasse))) {
        for (int i = 0; i < 2; ++i) {
            const bool da = k[static_cast<std::size_t>(i)].da;
            if (!da) { ImGui::BeginDisabled(); }
            if (ImGui::Selectable(klassenName(i), z().klasse == i)) { z().klasse = i; }
            if (!da) {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", k[static_cast<std::size_t>(i)].grund.c_str()); }
            } else if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", k[static_cast<std::size_t>(i)].quelle.c_str());
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeM0KlasseHint)); }
    ImGui::SameLine();
    ImGui::Checkbox((std::string(trK(Str::KeM0Kollision)) + "##m0kollision").c_str(), &z().kollision);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeM0KollisionHint)); }
}

bool modellEinsetzen(const std::string& pfad, ImVec2 punkt) {
    Editor& ed = e();
    const std::vector<modellsetzen::KlassenAngebot> k = modellsetzen::modellKlassen(spielNr(), true);
    const int kl = z().klasse == 1 ? 1 : 0;
    char t[900];
    if (!k[static_cast<std::size_t>(kl)].da) {
        std::snprintf(t, sizeof(t), trK(Str::M0KlasseFehlt), klassenName(kl), k[static_cast<std::size_t>(kl)].grund.c_str());
        fehler(t);
        return false;
    }
    if (kl == 1 && !endetMd3(pfad)) {
        std::snprintf(t, sizeof(t), trK(Str::KeM0NurMd3), pfad.c_str());
        fehler(t);
        return false;
    }
    const karte::ModellAnsicht a = kartenModellPfad(pfad);
    if (!a.modell || a.modell->null) {
        fehler("Model load failed: \"" + pfad + "\"");
        return false;
    }
    const auto& sp = spielKontext();
    auto koll = std::make_shared<KollisionsKarte>();
    (void)karte::kollisionAusKarte(ed.karte, parmsVon, !(sp && sp->spiel == kRegelwerkJediOutcast), *koll);
    float boden[3];
    if (!bodenPunkt(punkt, *koll, boden)) {
        fehler(trK(Str::M0KeinOrt));
        return false;
    }
    const karte::Kasten& g = a.modell->grenzen;   // localAABB: was q3map2 mit ModelGetTransform einsetzt
    const float mn[3] = {g.origin[0] - g.extents[0], g.origin[1] - g.extents[1], g.origin[2] - g.extents[2]};
    const float mx[3] = {g.origin[0] + g.extents[0], g.origin[1] + g.extents[1], g.origin[2] + g.extents[2]};
    const modellsetzen::Lage lage;
    const float ort[3] = {boden[0], boden[1], modellsetzen::ursprungZAufBoden(boden[2], mn, mx, lage)};
    const std::string klasse = klassenName(kl);
    {
        karte::Befehl undo(ed.karte.undo, "entityCreate -class " + klasse);
        ed.karte.auswahl.leeren();
        auto ent = ed.karte.neueEntitaet(klasse, false);
        ent->setzen("origin", karte::zahlText(ort[0]) + ' ' + karte::zahlText(ort[1]) + ' ' + karte::zahlText(ort[2]));
        const karte::EntitaetsKlasse* ek = ent->klasse();
        ent->setzen(ek != nullptr && !ek->miscmodelKey.empty() ? ek->miscmodelKey : std::string("model"), pfad);
        if (z().kollision && kl == 0) { ent->setzen("spawnflags", "2"); }   // q3map2 eClipModel
        ed.karte.entitaetEinfuegen(ent);
        ed.karte.auswahl.waehlen(ent);
        if (z().kollision && kl == 1) {
            // Ein Clip-Brush um den Kasten des Modells (misc_model_static hat im Spiel keine Kollision)
            float w0[3];
            float w1[3];
            modellsetzen::weltKasten(mn, mx, lage, ort, w0, w1);
            auto pinsel = std::make_shared<karte::Pinsel>();
            pinsel->schicht = static_cast<int>(ed.karte.aktuelleSchicht);
            const auto welt = ed.karte.weltHolenOderAnlegen();
            welt->kindEinfuegen(pinsel);
            karte::TexturProjektion proj;
            karte::texdefStandard(ed.karte.kontext.art(), proj);
            pinsel->quaderBauen(karte::kastenAusMinMax(karte::Vektor3f(w0[0], w0[1], w0[2]), karte::Vektor3f(w1[0], w1[1], w1[2])),
                                "textures/common/clip", proj, ed.karte.kontext);
        }
    }
    ++ed.sichtStand;
    auswahlGeaendert();
    std::snprintf(t, sizeof(t), trK(Str::KeM0Gesetzt), klasse.c_str(), pfad.c_str(), static_cast<double>(ort[0]), static_cast<double>(ort[1]),
                  static_cast<double>(ort[2]));
    ausgabe(t);
    return true;
}

void modellLiveWacheKarte() {
    Editor& ed = e();
    const double jetzt = ImGui::GetTime();
    if (z().uhr >= 0.0 && jetzt - z().uhr < 1.0) { return; }
    z().uhr = jetzt;
    const auto& sp = spielKontext();
    if (!sp) { return; }
    std::set<std::string> pfade;
    for (const auto& ent : ed.karte.entitaeten()) {
        const karte::EntitaetsKlasse* kl = ent->klasse();
        if (kl == nullptr || !kl->miscmodelIs) { continue; }
        const std::string p = ent->wert(kl->miscmodelKey.empty() ? std::string("model") : kl->miscmodelKey);
        if (!p.empty()) { pfade.insert(p); }
    }
    for (const std::string& p : pfade) {
        bool ordner = false;
        const std::string wurzel = sp->fs.finden(p, &ordner);   // findFile: Ordner (mit '/') oder .pk3
        const std::string ort = wurzel.empty() ? std::string() : (ordner ? wurzel + p : wurzel);
        const modellsetzen::DateiStand s = modellsetzen::dateiStand(ort);
        const auto alt = z().staende.find(p);
        const bool neu = alt != z().staende.end() && (alt->second.first != ort || !(alt->second.second == s));
        z().staende[p] = {ort, s};
        if (!neu) { continue; }
        modellVergessen(p);
        int exemplare = 0;
        for (const auto& ent : ed.karte.entitaeten()) {
            const karte::EntitaetsKlasse* kl = ent->klasse();
            exemplare += kl != nullptr && kl->miscmodelIs && ent->wert(kl->miscmodelKey.empty() ? std::string("model") : kl->miscmodelKey) == p ? 1 : 0;
        }
        char t[700];
        std::snprintf(t, sizeof(t), trN(Str::M0NeuGeladen, static_cast<std::size_t>(exemplare)), p.c_str(), exemplare);
        ausgabe(t);
    }
}

}  // namespace bhed::gui::karteneditor
