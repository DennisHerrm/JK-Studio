// app_modelltab_cwskelett.cpp - CW-MODELL (15.11.2026, Clone-Wars-Kette Teil 3): Bolts fuer Sitze und Muendungen, eigenes Skelett
//
// Liste des Bruders, Abschnitt 7: Bolts (*name) fuer Fahrer, Schuetzen und Muendungen, alle benannt und mit ausgegeben; eigene Skelette
// fuer Fahrzeuge, Laeufer (AT-TE, AT-RT) und Droiden mit eigener animation.cfg. Das Rechnen steht in bhed/modell/bolts.h und
// bhed/modell/skelett.h.
//
//   Bolts      wie 3ds Max ein Point-Helfer, an einen Knochen gelinkt (Reiter Erstellen), wie Blender ein Empty mit Parent Bone
//              (Reiter Objekt) - Bereich Modellieren: Vorlage mit den Namen, die das Spiel sucht (Quelle im Tooltip), Knochen, Ort und
//              Winkel, Anlegen / Uebernehmen / Entfernen, die Liste der Bolts des Modells. Hinweis "kein Fahrersitz / keine Muendung"
//              nur, wenn das Modell als Fahrzeug markiert ist oder die Clone-Wars-Kette an ist.
//   Skelett    wie 3ds Max Bone Tools (Reiter Erstellen, Systems > Bones), wie Blender Armature im Bearbeitungsmodus (Reiter Knochen) -
//              Bereich Animation: Neues Skelett (models/players/<name>/<name>), Knochen anlegen / verschieben / umhaengen / entfernen,
//              gewaehlte Flaechen starr an einen Knochen, Sequenz anlegen (Bilder fuer das Keyen, Zeile der animation.cfg). Das Skelett
//              steht im Dokument (Inhalt::skelett): jeder Schritt ist ein Rueckgaengig-Schritt; der Model-Tab zeigt es statt der GLA.
//              Ein Skelett aus einer Datei (_humanoid und alle anderen) bleibt unberuehrt - Knochen gibt es nur am eigenen.
#include "app_internal.h"
#include "ui_menue.h"
#include "textumbruch.h"

#include "bhed/modell/animnamen.h"
#include "bhed/modell/bolts.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/skelett.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellCw g_cw;

ModellTab& mt() { return g_app->mtab; }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
constexpr float kGrad = 3.14159265F / 180.0F;

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

modell::Zielspiel zielspiel() { return static_cast<modell::Zielspiel>(mt().zielspiel); }

bool ghoul2() { return mt().dok.inhalt().herkunft == modell::Herkunft::Glm && !g_app->model.empty(); }

// Drehung aus Winkeln wie 3ds Max (Euler XYZ: erst um X, dann Y, dann Z): R = Rz(gier) Ry(neigen) Rx(rollen)
BoneMatrix lageAus(const float ort[3], const float winkel[3]) {
    const float cx = std::cos(winkel[0] * kGrad), sx = std::sin(winkel[0] * kGrad);
    const float cy = std::cos(winkel[1] * kGrad), sy = std::sin(winkel[1] * kGrad);
    const float cz = std::cos(winkel[2] * kGrad), sz = std::sin(winkel[2] * kGrad);
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][0] = cz * cy;
    m.m[0][1] = cz * sy * sx - sz * cx;
    m.m[0][2] = cz * sy * cx + sz * sx;
    m.m[1][0] = sz * cy;
    m.m[1][1] = sz * sy * sx + cz * cx;
    m.m[1][2] = sz * sy * cx - cz * sx;
    m.m[2][0] = -sy;
    m.m[2][1] = cy * sx;
    m.m[2][2] = cy * cx;
    for (std::size_t k = 0; k < 3; ++k) { m.m[k][3] = ort[k]; }
    return m;
}

void winkelAus(const BoneMatrix& m, float ort[3], float winkel[3]) {
    for (std::size_t k = 0; k < 3; ++k) { ort[k] = m.m[k][3]; }
    winkel[1] = std::asin(std::clamp(-m.m[2][0], -1.0F, 1.0F)) / kGrad;
    winkel[0] = std::atan2(m.m[2][1], m.m[2][2]) / kGrad;
    winkel[2] = std::atan2(m.m[1][0], m.m[0][0]) / kGrad;
}

const char* knochenName(int k) {
    return k >= 0 && uz(k) < g_app->anim.bones.size() ? g_app->anim.bones[uz(k)].name.c_str() : "-";
}

void knochenOrt(int k, float ort[3]) {
    if (k >= 0 && uz(k) < g_app->anim.bones.size()) {
        for (std::size_t i = 0; i < 3; ++i) {
            const float v = g_app->anim.bones[uz(k)].basePose.m[i][3];
            ort[i] = std::abs(v) < 0.005F ? 0.0F : v;   // keine "-0.00" im Feld
        }
    } else {
        ort[0] = ort[1] = ort[2] = 0.0F;
    }
}

// Eine Knochenwahl als Klappliste
bool knochenWahl(const char* id, int* wahl, bool keiner) {
    bool geaendert = false;
    const char* jetzt = *wahl < 0 ? tr(Str::CwKeinKnochen) : knochenName(*wahl);
    if (ImGui::BeginCombo(id, jetzt)) {
        if (keiner && ImGui::Selectable(tr(Str::CwKeinKnochen), *wahl < 0)) {
            *wahl = -1;
            geaendert = true;
        }
        for (std::size_t k = 0; k < g_app->anim.bones.size(); ++k) {
            if (ImGui::Selectable(g_app->anim.bones[k].name.c_str(), *wahl == static_cast<int>(k))) {
                *wahl = static_cast<int>(k);
                geaendert = true;
            }
        }
        ImGui::EndCombo();
    }
    return geaendert;
}

void boltWaehlen(int teil) {
    ModellCw& c = g_cw;
    c.boltGewaehlt = teil;
    const modell::Inhalt& in = mt().dok.inhalt();
    if (teil < 0 || uz(teil) >= in.teile.size()) { return; }
    const modell::Teil& t = in.teile[uz(teil)];
    std::snprintf(c.boltName, sizeof c.boltName, "%s", t.name.c_str());
    c.boltKnochen = modell::boltKnochen(t);
    BoneMatrix lage;
    if (modell::boltLage(t, lage)) { winkelAus(lage, c.boltOrt, c.boltWinkel); }
}

}  // namespace

ModellCw& modellCw() { return g_cw; }

// Jedes Bild (aus modellAnzeigeAktualisieren): das eigene Skelett des Dokuments in die Anzeige; ohne (Rueckgaengig bis vor "Neues
// Skelett") wieder die GLA, die davor galt.
void modellCwSkelettAbgleich() {
    ModellCw& c = g_cw;
    const ModellTab& t = mt();
    if (c.ladeStand != t.dok.ladeStand()) {
        c.ladeStand = t.dok.ladeStand();
        c.angewendet = nullptr;
        c.vorherDa = false;
        c.boltGewaehlt = -1;
        c.knochenGewaehlt = -1;
        if (c.fahrzeugFuer != g_app->modelPath) {
            c.fahrzeugFuer = g_app->modelPath;
            c.fahrzeug = false;
        }
    }
    const GlaAnimation* s = t.dok.inhalt().skelett.get();
    if (s == c.angewendet) { return; }
    if (s != nullptr) {
        if (!c.vorherDa) {
            c.glaVorher = g_app->anim;
            c.listeVorher = g_app->animList;
            c.vorherDa = true;
        }
        g_app->anim = *s;
    } else if (c.vorherDa) {
        g_app->anim = c.glaVorher;
        g_app->animList = c.listeVorher;
        c.vorherDa = false;
    }
    c.angewendet = s;
    if (g_app->animIndex >= 0 && uz(g_app->animIndex) >= g_app->animList.size()) { g_app->animIndex = -1; }
    g_app->animFrame = std::clamp(g_app->animFrame, 0, std::max(g_app->anim.numFrames - 1, 0));
    g_app->modelDirty = true;
}

bool modellCwEigenesSkelett() { return mt().dok.inhalt().skelett != nullptr; }

// --- Bolts --------------------------------------------------------------------------------------------------------------------

bool modellBoltAnlegen() {
    ModellCw& c = g_cw;
    if (!ghoul2()) { return false; }
    const std::string name = c.boltName;
    if (!modell::boltNameFrei(mt().dok.inhalt(), name)) {
        melden(format(Str::CwBoltNameBelegt, name.c_str()));
        return false;
    }
    const BoneMatrix lage = lageAus(c.boltOrt, c.boltWinkel);
    const int knochen = std::max(c.boltKnochen, 0);
    int neu = -1;
    (void)mt().dok.aendern(tr(Str::CwBoltSchritt), [&](modell::Inhalt& x) { neu = modell::boltAnlegen(x, name, knochen, lage); });
    if (neu < 0) {
        (void)mt().dok.abbrechen();
        return false;
    }
    c.boltGewaehlt = neu;
    c.boltRollen = true;
    g_app->modelDirty = true;
    melden(format(Str::CwBoltAngelegt, name.c_str(), knochenName(knochen)));
    return true;
}

bool modellBoltUebernehmen() {
    ModellCw& c = g_cw;
    const modell::Inhalt& in = mt().dok.inhalt();
    if (!ghoul2() || c.boltGewaehlt < 0 || uz(c.boltGewaehlt) >= in.teile.size()) { return false; }
    const int teil = c.boltGewaehlt;
    const std::string name = c.boltName;
    const BoneMatrix lage = lageAus(c.boltOrt, c.boltWinkel);
    const int knochen = std::max(c.boltKnochen, 0);
    bool ok = true;
    (void)mt().dok.aendern(tr(Str::CwBoltAendernSchritt), [&](modell::Inhalt& x) {
        if (x.teile[uz(teil)].name != name) { ok = modell::boltUmbenennen(x, teil, name); }
        ok = ok && modell::boltKnochenSetzen(x, teil, knochen) && modell::boltLageSetzen(x, teil, lage);
    });
    if (!ok) {
        (void)mt().dok.abbrechen();
        melden(format(Str::CwBoltNameBelegt, name.c_str()));
        return false;
    }
    g_app->modelDirty = true;
    return true;
}

bool modellBoltEntfernen() {
    ModellCw& c = g_cw;
    const modell::Inhalt& in = mt().dok.inhalt();
    if (c.boltGewaehlt < 0 || uz(c.boltGewaehlt) >= in.teile.size() || !in.teile[uz(c.boltGewaehlt)].istTag()) { return false; }
    const int teil = c.boltGewaehlt;
    (void)mt().dok.aendern(tr(Str::CwBoltEntfernenSchritt), [&](modell::Inhalt& x) {
        x.teile.erase(x.teile.begin() + teil);
        for (modell::Teil& t : x.teile) {
            if (t.eltern == teil) { t.eltern = -1; }
            if (t.eltern > teil) { --t.eltern; }
            t.kinder.clear();
            t.dateiKinder.clear();
        }
        for (std::size_t i = 0; i < x.teile.size(); ++i) {
            const int p = x.teile[i].eltern;
            if (p >= 0 && uz(p) < x.teile.size()) { x.teile[uz(p)].kinder.push_back(static_cast<int>(i)); }
        }
        ++x.netzStand;
    });
    c.boltGewaehlt = -1;
    g_app->modelDirty = true;
    return true;
}

std::vector<std::string> modellBoltHinweise() {
    std::vector<std::string> h;
    if (!ghoul2() || !(g_cw.fahrzeug || modellCloneWarsAn())) { return h; }
    for (const modell::BoltBefundArt b : modell::fahrzeugBoltsPruefen(mt().dok.inhalt())) {
        h.emplace_back(tr(b == modell::BoltBefundArt::KeinFahrer ? Str::CwBoltKeinFahrer : Str::CwBoltKeineMuendung));
    }
    return h;
}

void drawModellBoltGruppe() {
    ModellCw& c = g_cw;
    const modell::Inhalt& in = mt().dok.inhalt();
    if (!ghoul2()) { return; }
    const bool bl = modellBlender();
    const char* titel = tr(bl ? Str::CwBoltGruppeBl : Str::CwBoltGruppeMax);
    if (c.zeigenBolts > 0) {
        modellFeldReiter(MbReiter::Erstellen, MbReiterBl::Objekt);
        ui::rolloutZeigen(titel);
        --c.zeigenBolts;
    }
    int zahl = 0;
    for (const modell::Teil& t : in.teile) { zahl += t.istTag() ? 1 : 0; }
    const std::vector<std::string> hinweise = modellBoltHinweise();
    char info[48];
    std::snprintf(info, sizeof info, "%d", zahl);
    c.probeBolts.clear();
    if (!ui::rolloutBeginn(titel, {.kurzinfo = info, .ton = hinweise.empty() ? ui::Ton::Leise : ui::Ton::Warnung, .zu = true,
                                   .reiter = modellReiter(MbReiter::Erstellen, MbReiterBl::Objekt, false), .hinweis = tr(Str::CwBoltGruppeHint)})) {
        return;
    }
    // Vorlage: die Namen, die der Spielcode sucht
    const std::vector<modell::BoltVorlage> vorlagen = modell::boltVorlagen(zielspiel());
    ui::zeile(tr(Str::CwBoltVorlage), tr(Str::CwBoltVorlageHint));
    if (ImGui::BeginCombo("##cwboltvorlage", c.boltName)) {
        static const Str kGruppe[4] = {Str::CwGruppeFahrzeug, Str::CwGruppeGeschuetz, Str::CwGruppeWaffe, Str::CwGruppeFigur};
        int gruppe = -1;
        for (const modell::BoltVorlage& v : vorlagen) {
            if (static_cast<int>(v.gruppe) != gruppe) {
                gruppe = static_cast<int>(v.gruppe);
                ImGui::SeparatorText(tr(kGruppe[gruppe]));
            }
            const bool belegt = !modell::boltNameFrei(in, v.name);
            if (ImGui::Selectable(v.name.c_str(), std::strcmp(c.boltName, v.name.c_str()) == 0, belegt ? ImGuiSelectableFlags_Disabled : 0)) {
                std::snprintf(c.boltName, sizeof c.boltName, "%s", v.name.c_str());
            }
            ImGui::SetItemTooltip("%s", v.quelle);
        }
        ImGui::EndCombo();
    }
    ui::zeile(tr(Str::CwBoltName), tr(Str::CwBoltNameHint));
    ImGui::InputText("##cwboltname", c.boltName, sizeof c.boltName);
    ui::zeile(tr(bl ? Str::CwBoltKnochenBl : Str::CwBoltKnochenMax), tr(Str::CwBoltKnochenHint));
    if (knochenWahl("##cwboltknochen", &c.boltKnochen, false) && c.boltGewaehlt < 0) { knochenOrt(c.boltKnochen, c.boltOrt); }
    ui::zeile(tr(Str::CwBoltOrt), tr(Str::CwBoltOrtHint));
    for (float& v : c.boltOrt) { v = std::abs(v) < 0.005F ? 0.0F : v; }   // keine "-0.00"
    ImGui::DragFloat3("##cwboltort", c.boltOrt, 0.25F, -65536.0F, 65536.0F, "%.2f");
    ui::zeile(tr(Str::CwBoltWinkel), tr(Str::CwBoltWinkelHint));
    ImGui::DragFloat3("##cwboltwinkel", c.boltWinkel, 1.0F, -360.0F, 360.0F, "%.1f");
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(Str::CwBoltAnlegen), {.icon = UiIcon::MbTag, .hinweis = tr(Str::CwBoltAnlegenHint), .haupt = true})) { (void)modellBoltAnlegen(); }
    if (ui::werkzeugKnopf(tr(Str::CwBoltAnKnochen), {.icon = UiIcon::MbKnochen, .hinweis = tr(Str::CwBoltAnKnochenHint)})) { knochenOrt(c.boltKnochen, c.boltOrt); }
    const bool gewaehlt = c.boltGewaehlt >= 0 && uz(c.boltGewaehlt) < in.teile.size() && in.teile[uz(c.boltGewaehlt)].istTag();
    if (ui::werkzeugKnopf(tr(Str::CwBoltUebernehmen), {.icon = UiIcon::Haken, .hinweis = tr(Str::CwBoltUebernehmenHint), .aktiv = gewaehlt})) {
        (void)modellBoltUebernehmen();
    }
    if (ui::werkzeugKnopf(tr(Str::CwBoltEntfernen), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::CwBoltEntfernenHint), .aktiv = gewaehlt})) {
        (void)modellBoltEntfernen();
    }
    ui::rasterEnde();
    // Pruefung fuer Fahrzeuge (markiert oder Clone-Wars-Kette) - vor der Liste, die bei einer Figur lang ist
    (void)ui::schalter(tr(Str::CwFahrzeug), &c.fahrzeug, tr(Str::CwFahrzeugHint));
    for (const std::string& h : hinweise) { ui::absatz(h.c_str(), ui::Ton::Warnung); }
    // die Bolts des Modells (Klick waehlt, die Felder zeigen seine Werte) - hoechstens acht Zeilen hoch, darin rollend; ein neuer Bolt ist
    // gewaehlt und ins Bild gerollt
    const float zeileH = ImGui::GetTextLineHeightWithSpacing();
    const float hoehe = zeileH * static_cast<float>(std::min(std::max(zahl, 1), 8)) + ImGui::GetStyle().WindowPadding.y * 2.0F;
    if (zahl > 0) {
        if (ImGui::BeginChild("##cwboltliste", ImVec2{-1.0F, hoehe}, ImGuiChildFlags_Borders)) {
            for (std::size_t i = 0; i < in.teile.size(); ++i) {
                const modell::Teil& t = in.teile[i];
                if (!t.istTag()) { continue; }
                const std::string zeile = t.name + "  \xE2\x86\x92  " + knochenName(modell::boltKnochen(t));
                ImGui::PushID(static_cast<int>(i));
                const bool gew = c.boltGewaehlt == static_cast<int>(i);
                if (ImGui::Selectable(zeile.c_str(), gew)) { boltWaehlen(static_cast<int>(i)); }
                if (gew && c.boltRollen) {
                    ImGui::SetScrollHereY();
                    c.boltRollen = false;
                }
                ImGui::PopID();
                c.probeBolts.push_back(zeile);
            }
        }
        ImGui::EndChild();
    }
    ui::rolloutEnde();
}

// --- Skelett ------------------------------------------------------------------------------------------------------------------

bool modellSkelettNeu() {
    ModellCw& c = g_cw;
    if (!ghoul2()) { return false; }
    std::string n = c.skelettName;
    for (char& ch : n) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
    if (n.empty() || n.find_first_of("/\\ .") != std::string::npos) {
        melden(tr(Str::CwSkelettNameUngueltig));
        return false;
    }
    const std::string kopf = "models/players/" + n + "/" + n;
    auto a = std::make_shared<GlaAnimation>(modell::skelettNeu(kopf));
    (void)mt().dok.aendern(tr(Str::CwSkelettNeuSchritt), [&](modell::Inhalt& x) {
        std::vector<int> alle;
        for (std::size_t i = 0; i < x.teile.size(); ++i) {
            if (!x.teile[i].istTag()) { alle.push_back(static_cast<int>(i)); }
        }
        modell::flaechenAnKnochen(x, alle, 0);
        modell::modellAnSkelett(x, *a);
        x.skelett = a;
        x.cfg = modell::AnimCfg{};
        x.cfg.crlf = true;
        x.cfgPfad = "models/players/" + n + "/animation.cfg";
    });
    c.knochenGewaehlt = 0;
    melden(format(Str::CwSkelettAngelegt, kopf.c_str()));
    return true;
}

namespace {
// Eine Aenderung am eigenen Skelett als ein Rueckgaengig-Schritt (Kopie, aendern, ins Dokument)
bool skelettAendern(Str schritt, const std::function<bool(GlaAnimation&, modell::Inhalt&)>& f) {
    const modell::Inhalt& in = mt().dok.inhalt();
    if (!in.skelett) {
        melden(tr(Str::CwSkelettNurEigenes));
        return false;
    }
    bool ok = false;
    (void)mt().dok.aendern(tr(schritt), [&](modell::Inhalt& x) {
        auto a = std::make_shared<GlaAnimation>(*x.skelett);
        ok = f(*a, x);
        if (ok) {
            x.knochenZahl = static_cast<int>(a->bones.size());
            x.dateiKnochen = x.knochenZahl;
            x.skelett = a;
        }
    });
    if (!ok) { (void)mt().dok.abbrechen(); }
    return ok;
}
}  // namespace

bool modellKnochenAnlegen() {
    ModellCw& c = g_cw;
    const std::string name = c.knochenName;
    const int eltern = c.knochenEltern;
    float ort[3] = {c.knochenOrt[0], c.knochenOrt[1], c.knochenOrt[2]};
    int neu = -1;
    const bool ok = skelettAendern(Str::CwKnochenSchritt, [&](GlaAnimation& a, modell::Inhalt&) {
        neu = modell::knochenAnlegen(a, name, eltern, ort);
        return neu >= 0;
    });
    if (!ok) {
        if (modellCwEigenesSkelett()) { melden(format(Str::CwKnochenNameBelegt, name.c_str())); }
        return false;
    }
    // wie die Bone Tools von 3ds Max: der naechste Knochen setzt die Kette fort - Eltern der neue, Name mit der naechsten Nummer; gewaehlt
    // ist keiner (die Felder gehoeren dem naechsten Knochen, ein Klick in die Liste waehlt einen zum Aendern)
    c.knochenGewaehlt = -1;
    c.knochenEltern = neu;
    std::string naechster = name;
    std::size_t z = naechster.size();
    while (z > 0 && std::isdigit(static_cast<unsigned char>(naechster[z - 1])) != 0) { --z; }
    const int nummer = z < naechster.size() ? std::atoi(naechster.c_str() + z) + 1 : 1;
    naechster = naechster.substr(0, z) + std::to_string(nummer);
    std::snprintf(c.knochenName, sizeof c.knochenName, "%s", naechster.c_str());
    melden(format(Str::CwKnochenAngelegt, name.c_str()));
    return true;
}

bool modellKnochenUebernehmen() {
    ModellCw& c = g_cw;
    const int k = c.knochenGewaehlt;
    const std::string name = c.knochenName;
    const int eltern = c.knochenEltern;
    float ort[3] = {c.knochenOrt[0], c.knochenOrt[1], c.knochenOrt[2]};
    return skelettAendern(Str::CwKnochenAendernSchritt, [&](GlaAnimation& a, modell::Inhalt&) {
        if (k < 0 || uz(k) >= a.bones.size()) { return false; }
        if (a.bones[uz(k)].name != name && !modell::knochenUmbenennen(a, k, name)) { return false; }
        if (a.bones[uz(k)].parent != eltern && !modell::knochenEltern(a, k, eltern)) { return false; }
        return modell::knochenVerschieben(a, k, ort);
    });
}

bool modellKnochenEntfernen() {
    ModellCw& c = g_cw;
    const int k = c.knochenGewaehlt;
    const bool ok = skelettAendern(Str::CwKnochenEntfernenSchritt, [&](GlaAnimation& a, modell::Inhalt& x) { return modell::knochenEntfernen(a, x, k); });
    if (ok) { c.knochenGewaehlt = -1; }
    return ok;
}

bool modellFlaechenAnKnochen() {
    ModellCw& c = g_cw;
    ModellTab& t = mt();
    // die Knochen des eigenen Skeletts aus dem Dokument (die Anzeige nimmt es erst im naechsten Bild), sonst die der GLA
    const std::size_t knochenZahl = t.dok.inhalt().skelett ? t.dok.inhalt().skelett->bones.size() : g_app->anim.bones.size();
    if (!ghoul2() || c.knochenGewaehlt < 0 || uz(c.knochenGewaehlt) >= knochenZahl) { return false; }
    std::vector<int> teile;
    for (const int i : t.auswahl) {
        if (i >= 0 && uz(i) < t.dok.inhalt().teile.size() && !t.dok.inhalt().teile[uz(i)].istTag()) { teile.push_back(i); }
    }
    if (teile.empty()) {
        melden(tr(Str::CwFlaechenKeine));
        return false;
    }
    const int k = c.knochenGewaehlt;
    (void)t.dok.aendern(tr(Str::CwFlaechenSchritt), [&](modell::Inhalt& x) { modell::flaechenAnKnochen(x, teile, k); });
    g_app->modelDirty = true;
    const modell::Inhalt& in = t.dok.inhalt();
    melden(format(Str::CwFlaechenGebunden, static_cast<int>(teile.size()), in.skelett ? in.skelett->bones[uz(k)].name.c_str() : knochenName(k)));
    return true;
}

bool modellSequenzAnlegen() {
    ModellCw& c = g_cw;
    const std::string name = c.sequenzName;
    const int bilder = std::clamp(c.sequenzBilder, 1, 1000);
    const int fps = std::clamp(c.sequenzFps, 1, 1000);
    int erstes = 0;
    const bool ok = skelettAendern(Str::CwSequenzSchritt, [&](GlaAnimation& a, modell::Inhalt& x) {
        if (name.empty() || x.cfg.zeileVon(name) >= 0) { return false; }
        erstes = a.numFrames;
        const std::vector<BoneMatrix> eins(a.bones.size(), BoneMatrix::identity());
        for (int i = 0; i < bilder; ++i) {
            if (!modell::bildAnhaengen(a, eins)) { return false; }
        }
        x.cfg.setzen(AnimEntry{name, erstes, bilder, c.sequenzSchleife ? 0 : -1, fps});
        return true;
    });
    if (!ok) {
        if (modellCwEigenesSkelett()) { melden(format(Str::CwSequenzBelegt, name.c_str())); }
        return false;
    }
    melden(format(Str::CwSequenzAngelegt, name.c_str(), erstes, bilder, fps));
    return true;
}

void drawModellSkelettGruppe() {
    ModellCw& c = g_cw;
    const modell::Inhalt& in = mt().dok.inhalt();
    if (!ghoul2()) { return; }
    const bool bl = modellBlender();
    const char* titel = tr(bl ? Str::CwSkelettGruppeBl : Str::CwSkelettGruppeMax);
    if (c.zeigenSkelett > 0) {
        modellFeldReiter(MbReiter::Erstellen, MbReiterBl::Knochen);
        ui::rolloutZeigen(titel);
        --c.zeigenSkelett;
    }
    const bool eigen = in.skelett != nullptr;
    char info[64];
    std::snprintf(info, sizeof info, "%d", static_cast<int>(g_app->anim.bones.size()));
    c.probeKnochen.clear();
    if (!ui::rolloutBeginn(titel, {.kurzinfo = info, .zu = !eigen, .reiter = modellAnimReiter(true, modellReiter(MbReiter::Erstellen, MbReiterBl::Knochen, false)),
                                   .hinweis = tr(Str::CwSkelettGruppeHint)})) {
        return;
    }
    if (!eigen) {
        // ein Skelett aus einer Datei bleibt, wie es ist (_humanoid: alle Figuren und Animationen des Spiels haengen daran)
        ui::absatz(format(Str::CwSkelettAusDatei, g_app->anim.skeletonName.c_str(), static_cast<int>(g_app->anim.bones.size())).c_str());
        ui::zeile(tr(Str::CwSkelettName), tr(Str::CwSkelettNameHint));
        ImGui::InputText("##cwskelettname", c.skelettName, sizeof c.skelettName);
        if (ui::werkzeugKnopf(tr(Str::CwSkelettNeu), {.icon = UiIcon::MbRig, .hinweis = tr(Str::CwSkelettNeuHint), .haupt = true})) { (void)modellSkelettNeu(); }
        ui::rolloutEnde();
        return;
    }
    ui::absatz(format(Str::CwSkelettEigen, in.skelett->skeletonName.c_str(), static_cast<int>(in.skelett->bones.size()), in.skelett->numFrames).c_str());
    // die Knochen (Klick waehlt)
    for (std::size_t k = 0; k < g_app->anim.bones.size(); ++k) {
        const GlaBone& b = g_app->anim.bones[k];
        int tiefe = 0;
        for (int p = b.parent; p >= 0 && tiefe < 32; p = g_app->anim.bones[uz(p)].parent) { ++tiefe; }
        const std::string zeile = std::string(static_cast<std::size_t>(tiefe) * 2U, ' ') + b.name;
        ImGui::PushID(static_cast<int>(k));
        if (ImGui::Selectable(zeile.c_str(), c.knochenGewaehlt == static_cast<int>(k))) {
            c.knochenGewaehlt = static_cast<int>(k);
            std::snprintf(c.knochenName, sizeof c.knochenName, "%s", b.name.c_str());
            c.knochenEltern = b.parent;
            knochenOrt(static_cast<int>(k), c.knochenOrt);
        }
        ImGui::PopID();
        c.probeKnochen.push_back(b.name);
    }
    ui::zeile(tr(Str::CwKnochenName), tr(Str::CwKnochenNameHint));
    ImGui::InputText("##cwknochenname", c.knochenName, sizeof c.knochenName);
    ui::zeile(tr(Str::CwKnochenEltern), tr(Str::CwKnochenElternHint));
    (void)knochenWahl("##cwknocheneltern", &c.knochenEltern, true);
    ui::zeile(tr(Str::CwKnochenOrt), tr(Str::CwKnochenOrtHint));
    for (float& v : c.knochenOrt) { v = std::abs(v) < 0.005F ? 0.0F : v; }
    ImGui::DragFloat3("##cwknochenort", c.knochenOrt, 0.25F, -65536.0F, 65536.0F, "%.2f");
    ui::rasterBeginn(2);
    if (ui::werkzeugKnopf(tr(bl ? Str::CwKnochenAnlegenBl : Str::CwKnochenAnlegenMax), {.icon = UiIcon::MbKnochen, .hinweis = tr(Str::CwKnochenAnlegenHint), .haupt = true})) {
        (void)modellKnochenAnlegen();
    }
    const bool gewaehlt = c.knochenGewaehlt >= 0 && uz(c.knochenGewaehlt) < g_app->anim.bones.size();
    if (ui::werkzeugKnopf(tr(Str::CwKnochenUebernehmen), {.icon = UiIcon::Haken, .hinweis = tr(Str::CwKnochenUebernehmenHint), .aktiv = gewaehlt})) {
        (void)modellKnochenUebernehmen();
    }
    if (ui::werkzeugKnopf(tr(Str::CwKnochenEntfernen), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::CwKnochenEntfernenHint), .aktiv = gewaehlt})) {
        (void)modellKnochenEntfernen();
    }
    if (ui::werkzeugKnopf(tr(bl ? Str::CwFlaechenBl : Str::CwFlaechenMax), {.icon = UiIcon::MbFlaeche, .hinweis = tr(Str::CwFlaechenHint), .aktiv = gewaehlt})) {
        (void)modellFlaechenAnKnochen();
    }
    ui::rasterEnde();
    // Sequenz anlegen: Bilder zum Keyen und die Zeile der animation.cfg (nur Namen, die das Spiel kennt, ohne Warnung)
    ui::zeile(tr(Str::CwSequenz), tr(Str::CwSequenzHint));
    if (ImGui::BeginCombo("##cwsequenz", c.sequenzName)) {
        static const char* const kVorschlag[] = {"BOTH_STAND1", "BOTH_WALK1", "BOTH_RUN1", "BOTH_WALKBACK1", "BOTH_TURN_LEFT1", "BOTH_TURN_RIGHT1",
                                                 "BOTH_DEATH1", "BOTH_PAIN1", "BOTH_ATTACK1", "BOTH_VS_IDLE", "BOTH_VS_MOUNT_L", "BOTH_VS_DISMOUNT_L"};
        for (const char* v : kVorschlag) {
            if (ImGui::Selectable(v, std::strcmp(c.sequenzName, v) == 0)) { std::snprintf(c.sequenzName, sizeof c.sequenzName, "%s", v); }
        }
        ImGui::EndCombo();
    }
    ImGui::InputText("##cwsequenzname", c.sequenzName, sizeof c.sequenzName);
    const std::vector<std::string>& bekannt = modell::animNamenSpiel(zielspiel());
    const bool kennt = std::any_of(bekannt.begin(), bekannt.end(), [&](const std::string& n) {
        return n.size() == std::strlen(c.sequenzName) && std::equal(n.begin(), n.end(), c.sequenzName, [](char a, char b) {
                   return std::toupper(static_cast<unsigned char>(a)) == std::toupper(static_cast<unsigned char>(b));
               });
    });
    if (!kennt && c.sequenzName[0] != '\0') { ui::absatz(format(Str::CwSequenzUnbekannt, c.sequenzName, modell::grenzen(zielspiel()).name).c_str(), ui::Ton::Warnung); }
    (void)ui::reglerInt(tr(Str::CwSequenzBilder), &c.sequenzBilder, 1, 200, "%d", tr(Str::CwSequenzBilderHint));
    (void)ui::reglerInt(tr(Str::CwSequenzFps), &c.sequenzFps, 1, 60, "%d", tr(Str::CwSequenzFpsHint));
    (void)ui::schalter(tr(Str::CwSequenzSchleife), &c.sequenzSchleife, tr(Str::CwSequenzSchleifeHint));
    if (ui::werkzeugKnopf(tr(Str::CwSequenzAnlegen), {.icon = UiIcon::MbSequenz, .hinweis = tr(Str::CwSequenzAnlegenHint)})) { (void)modellSequenzAnlegen(); }
    ui::tipp(tr(Str::CwSkelettTipp));
    ui::rolloutEnde();
}

void modellCwMenueZeilen() {
    if (!ghoul2()) { return; }
    if (ImGui::MenuItem(tr(modellBlender() ? Str::CwBoltGruppeBl : Str::CwBoltGruppeMax))) { modellCwZeigen(0); }
    ImGui::SetItemTooltip("%s", tr(Str::CwBoltGruppeHint));
    if (ImGui::MenuItem(tr(modellBlender() ? Str::CwSkelettGruppeBl : Str::CwSkelettGruppeMax))) { modellCwZeigen(1); }
    ImGui::SetItemTooltip("%s", tr(Str::CwSkelettGruppeHint));
}

void modellCwZeigen(int was) {
    if (was == 0) {
        mt().bereich = ModellBereich::Modellieren;
        g_cw.zeigenBolts = 2;
    } else {
        mt().bereich = ModellBereich::Animation;
        g_cw.zeigenSkelett = 2;
    }
}

bool modellCwAktionenSuche(const std::string& such) {
    if (!ghoul2()) { return false; }
    struct Eintrag {
        std::string text;
        const char* hinweis;
        int bereich;
        bool (*tun)();
    };
    const bool bl = modellBlender();
    const std::vector<Eintrag> liste = {
        {tr(bl ? Str::CwBoltGruppeBl : Str::CwBoltGruppeMax), tr(Str::CwBoltGruppeHint), static_cast<int>(ModellBereich::Modellieren), [] {
             modellCwZeigen(0);
             return true;
         }},
        {tr(Str::CwBoltAnlegen), tr(Str::CwBoltAnlegenHint), static_cast<int>(ModellBereich::Modellieren), [] { return modellBoltAnlegen(); }},
        {tr(bl ? Str::CwSkelettGruppeBl : Str::CwSkelettGruppeMax), tr(Str::CwSkelettGruppeHint), static_cast<int>(ModellBereich::Animation), [] {
             modellCwZeigen(1);
             return true;
         }},
        {tr(Str::CwSkelettNeu), tr(Str::CwSkelettNeuHint), static_cast<int>(ModellBereich::Animation), [] { return modellSkelettNeu(); }},
        {tr(bl ? Str::CwKnochenAnlegenBl : Str::CwKnochenAnlegenMax), tr(Str::CwKnochenAnlegenHint), static_cast<int>(ModellBereich::Animation),
         [] { return modellKnochenAnlegen(); }},
        {tr(bl ? Str::CwFlaechenBl : Str::CwFlaechenMax), tr(Str::CwFlaechenHint), static_cast<int>(ModellBereich::Animation), [] { return modellFlaechenAnKnochen(); }},
        {tr(Str::CwSequenzAnlegen), tr(Str::CwSequenzAnlegenHint), static_cast<int>(ModellBereich::Animation), [] { return modellSequenzAnlegen(); }},
    };
    bool ausgeloest = false;
    for (const Eintrag& e : liste) {
        std::string k = e.text;
        for (char& ch : k) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
        if (!such.empty() && k.find(such) == std::string::npos) { continue; }
        if (modellAktionZeile(e.text.c_str(), e.bereich)) {
            (void)e.tun();
            ++mt().tastenZaehler;
            mt().letzteTaste = e.text;
            g_app->modelDirty = true;
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s", e.hinweis);
    }
    return ausgeloest;
}

}  // namespace bhed::gui
