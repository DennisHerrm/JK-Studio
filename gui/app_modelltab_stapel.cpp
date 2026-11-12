// app_modelltab_stapel.cpp - AP13S (MBENTWURF Teil B, rc612): der Modifier-Stapel im rechten Feld des Model-Tabs und die
// Vorschau seiner Eintraege in der Ansicht
//
// Abgleichtabelle plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Zeilen M1, M2, W1; Entwurf entwurf_modelltab.html
// (Gruppe "Modifier-Stapel" wie 3ds Max: von unten nach oben GLM-Flaeche -> Edit Poly -> TurboSmooth (nur Vorschau), je mit
// Auge; Gruppe "Modifier" wie Blender: von oben nach unten Spiegeln (X) mit Auge und Datei-Schalter, Unterteilung (nur
// Vorschau)); vorbilder_max_blender.md, Zeile "Modifier-Stapel": ein kleiner, JKA-eigener Stapel, Reihenfolge und Darstellung
// nach dem Bedienmodus, das Ergebnis beim Export gleich.
//
//   Daten      je Flaeche (modell::Teil::stapel, bhed/modell/stapeldaten.h) im Dokument: jede Aktion am Stapel ist ein
//              Rueckgaengig-Schritt (hinzufuegen, an/aus, in die Datei, Einstellungen, umordnen, anwenden, entfernen)
//   Rechnen    bhed/modell/stapel.h - dieselbe Funktion fuer die Vorschau der Ansicht, fuer Speichern/Export und fuer
//              "Anwenden": was die Ansicht zeigt, ist Ecke fuer Ecke das, was Anwenden ins Dokument legt
//   Ansicht    modellStapelZeichnung(): das gezeichnete Modell mit den Eintraegen, die rechnen (gui/app_modelltab_ansicht.cpp
//              zeichnet es statt ModellTab::anzeige). Auswahl, Ueberlagerung und alle Werkzeuge arbeiten weiter auf dem Netz
//              des Dokuments (wie 3ds Max "Show End Result" und Blender im Edit-Modus: der Kaefig darunter bleibt, was man
//              bearbeitet)
//   Datei      modellStapelDateiInhalt(): was Speichern und Export schreiben - ohne Eintrag, der in die Datei geht, das
//              Dokument selbst (dieselbe Referenz, dieselben Bytes wie ohne Stapel)
//   Feld       drawModellStapelGruppe(): wie 3ds Max die Gruppe "Modifier-Stapel" (Reiter Aendern, Bereiche Modellieren und
//              Weights) mit den Stufen des Modells - Flaeche der Datei, Edit Poly, Skin - als Zeilen, die den Bereich
//              wechseln; wie Blender die Gruppe "Modifier" (Reiter Modifier) nur mit den Eintraegen. Zeilen wie die Ebenen
//              des Paintings (dieselbe Form im Entwurf), Einstellungen des gewaehlten Eintrags darunter.
#include "app_internal.h"
#include "textumbruch.h"

#include "bhed/modell/gewichte.h"
#include "bhed/modell/stapel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::gui {
namespace wof = modell::wof;
namespace {

using modell::ModArt;
using modell::Modifikator;
using modell::StapelZweck;

ModellStapel g_stapel;

ModellTab& mt() { return g_app->mtab; }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
int zi(std::size_t i) { return static_cast<int>(i); }

std::string format(Str s, auto... a) {
    char b[900];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Tooltip des zuletzt gezeichneten Elements, umbrochen wie die Knoepfe des Bausteins (ui::tooltipText: 24 Zeichenhoehen, CJK
// ueber textumbruch.h) - ohne Umbruch lief der Tipp der Zeile ueber die halbe Ansicht (Foto ap13s_f_de/ap13s_max_tooltip)
void tippUmbrochen(const std::string& s) {
    if (s.empty() || !ImGui::BeginItemTooltip()) { return; }
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
    cjkTextUnformatted(s.c_str(), s.c_str() + s.size());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// --- Namen ------------------------------------------------------------------------------------------------------------

// Der Name der Art im Bedienmodus: wie 3ds Max "Symmetry" und "TurboSmooth", wie Blender "Mirror" und "Subdivision"
Str artStr(ModArt art) {
    const bool bl = modellBlender();
    if (art == ModArt::Spiegeln) { return bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax; }
    return bl ? Str::StpGlaettenBl : Str::StpGlaettenMax;
}

std::string eintragName(const Modifikator& m) {
    std::string n = tr(artStr(m.art));
    if (m.art == ModArt::Spiegeln) {
        static const char* const kAchsen[3] = {"X", "Y", "Z"};
        n = format(Str::StpMitAchse, n.c_str(), kAchsen[std::min<int>(m.achse, 2)]);
    }
    if (!m.datei) { n = format(Str::StpNurVorschau, n.c_str()); }
    return n;
}

std::string flaechenName(int teil) {
    const modell::Inhalt& in = mt().dok.inhalt();
    return teil >= 0 && uz(teil) < in.teile.size() ? in.teile[uz(teil)].name : std::string();
}

// Warum ein Eintrag nicht rechnen konnte - die Saetze des Modellierens (tools/i18n_md1d.py)
std::string grundText(wof::Ablehnung g) {
    switch (g) {
        case wof::Ablehnung::KeineQuelle: return tr(Str::MdAblQuelle);
        case wof::Ablehnung::Tag: return tr(Str::MdAblTag);
        default: break;
    }
    return tr(Str::MdAblLeer);
}

// --- Dokument ---------------------------------------------------------------------------------------------------------

const modell::Teil* teilVon(int teil) {
    const modell::Inhalt& in = mt().dok.inhalt();
    return teil >= 0 && uz(teil) < in.teile.size() ? &in.teile[uz(teil)] : nullptr;
}

// Die Flaeche, die den Eintrag traegt (-1: keine), und sein Index dort
int teilMit(std::uint32_t kennung, int* index = nullptr) {
    if (kennung == 0U) { return -1; }
    const modell::Inhalt& in = mt().dok.inhalt();
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const int i = modell::stapelIndex(in.teile[t], kennung);
        if (i >= 0) {
            if (index != nullptr) { *index = i; }
            return zi(t);
        }
    }
    return -1;
}

// Vor jedem Befehl am Stapel: ein offenes Werkzeugkaestchen zuerst uebernehmen (wie jeder Befehl des Modellierens, pfix H1 -
// sonst laege sein Schritt unter dem neuen)
modell::Dokument& vorBefehl() {
    if (mt().mesh.kaestchenOffen) { modellMeshUebernehmen(false); }
    g_stapel.reglerAktiv = 0;
    return mt().dok;
}

void nachBefehl() {
    ++g_stapel.befehle;
    g_app->modelDirty = true;
}

// Eine Einstellung des Eintrags aendern. `wer`: die Kennung des gezogenen Reglers (0: ein Klick) - solange derselbe Regler
// gezogen wird und nichts dazwischenkam, aendert die Vorschau den offenen Schritt statt je Wert einen neuen anzulegen.
void einstellung(ImGuiID wer, std::uint32_t kennung, const std::function<void(Modifikator&)>& f) {
    ModellStapel& z = g_stapel;
    int index = -1;
    const int teil = teilMit(kennung, &index);
    if (teil < 0) { return; }
    const auto tun = [&](modell::Inhalt& x) { f(x.teile[uz(teil)].stapel[uz(index)]); };
    modell::Dokument& d = mt().dok;
    if (wer != 0U && z.reglerAktiv == wer && z.reglerStand == d.stand()) {
        d.vorschau(tun);
    } else {
        (void)vorBefehl().aendern(tr(Str::StpUndoEinstellung), tun);
    }
    z.reglerAktiv = wer;
    z.reglerStand = d.stand();
    nachBefehl();
}

// --- die Zeichnung der Ansicht ---------------------------------------------------------------------------------------

void zeichnungBauen() {
    ModellTab& t = mt();
    ModellStapel& z = g_stapel;
    z.zeichnungFuer = t.neuGebaut;
    z.zeichnungAn = false;
    z.ersetzt.assign(t.anzeige.surfaces.size(), 0);
    z.stand.clear();
    const modell::Inhalt& in = t.dok.inhalt();
    if (in.teile.size() != t.anzeige.surfaces.size() || !modell::stapelWirkt(in, StapelZweck::Ansicht)) {
        if (!z.zeichnung.empty()) { z.zeichnung = GlmModel{}; }
        return;
    }
    const modell::StapelUmgebung u = modellStapelUmgebung();
    GlmModel m = t.anzeige;
    z.stand.resize(in.teile.size());
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const modell::Teil& teil = in.teile[i];
        if (!modell::stapelWirkt(teil, StapelZweck::Ansicht) || uz(t.lodJetzt) >= teil.stufen.size()) { continue; }
        ModellStapel::TeilStand& s = z.stand[i];
        std::vector<wof::Ablehnung> status;
        const modell::Netz netz = modell::stapelNetz(teil, t.lodJetzt, StapelZweck::Ansicht, u, &status, &s.nach);
        s.vor = modell::netzZahlen(teil.stufen[uz(t.lodJetzt)]);
        s.status.assign(status.size(), 0);
        for (std::size_t k = 0; k < status.size(); ++k) { s.status[k] = static_cast<int>(status[k]); }
        GlmSurface& g = m.surfaces[i];
        g.verts.clear();
        g.verts.reserve(netz.ecken.size());
        for (std::size_t e = 0; e < netz.ecken.size(); ++e) { g.verts.push_back(modell::anzeigeVertex(netz, e)); }
        g.indexes = netz.indizes();
        z.ersetzt[i] = 1;
    }
    z.zeichnung = std::move(m);
    z.zeichnungAn = true;
    ++z.zeichnungen;
}

void zeichnungAbgleichen() {
    if (g_stapel.zeichnungFuer != mt().neuGebaut) { zeichnungBauen(); }
}

// --- Befehle ------------------------------------------------------------------------------------------------------------

// Den Eintrag in der Liste waehlen (seine Einstellungen stehen darunter)
void waehlen(std::uint32_t kennung) { g_stapel.gewaehlt = kennung; }

// Der gewaehlte Eintrag, wenn er zur gezeigten Flaeche gehoert (sonst 0)
std::uint32_t gewaehlterEintrag() {
    const int teil = modellStapelTeil();
    const modell::Teil* t = teilVon(teil);
    return t != nullptr && modell::stapelIndex(*t, g_stapel.gewaehlt) >= 0 ? g_stapel.gewaehlt : 0U;
}

// Nach dem Anwenden hat die Flaeche ein anderes Netz: die Auswahl des Modellierens zeigte auf das alte
void auswahlLeeren() {
    ModellTab& t = mt();
    t.mesh.auswahl = wof::PAuswahl{};
    t.mesh.punktFolge.clear();
    t.eckenWahl.clear();
    modellMeshNetzAktualisieren();
}

bool anwenden(int teil, const std::vector<int>& eintraege, Str name, const std::string& was) {
    const modell::Teil* t = teilVon(teil);
    if (t == nullptr || eintraege.empty()) { return false; }
    modell::Dokument& d = vorBefehl();
    const modell::StapelUmgebung u = modellStapelUmgebung();
    wof::Ablehnung grund = wof::Ablehnung::Keine;
    bool ok = false;
    const std::uint64_t schritt = d.aendern(tr(name), [&](modell::Inhalt& x) { ok = modell::stapelAnwenden(x, teil, eintraege, u, &grund); });
    if (!ok) {
        (void)d.abbrechen(schritt);
        melden(format(Str::MdAbgelehnt, was.c_str(), grundText(grund).c_str()));
        g_app->modelDirty = true;
        return false;
    }
    auswahlLeeren();
    const modell::Teil* neu = teilVon(teil);
    const modell::NetzZahlen zahlen = neu != nullptr && uz(mt().lodJetzt) < neu->stufen.size() ? modell::netzZahlen(neu->stufen[uz(mt().lodJetzt)]) : modell::NetzZahlen{};
    if (eintraege.size() == 1U) {
        melden(format(Str::StpMeldAngewendet, was.c_str(), flaechenName(teil).c_str(), zahlen.ecken, zahlen.dreiecke));
    } else {
        melden(format(Str::StpMeldAlle, zi(eintraege.size()), flaechenName(teil).c_str(), zahlen.ecken, zahlen.dreiecke));
    }
    nachBefehl();
    return true;
}

}  // namespace

// --- Schnittstelle ------------------------------------------------------------------------------------------------------

ModellStapel& modellStapel() { return g_stapel; }

modell::StapelUmgebung modellStapelUmgebung() {
    modell::StapelUmgebung u;
    // ein Modell ohne Skelett (.md3, Requisit): die Ecken haengen nur der Form halber an Knochen 0 - nichts zu tauschen
    if (mt().dok.inhalt().herkunft != modell::Herkunft::Glm) { return u; }
    std::vector<std::string> namen;
    namen.reserve(g_app->anim.bones.size());
    for (const auto& b : g_app->anim.bones) { namen.push_back(b.name); }
    u.knochenSpiegel.resize(namen.size(), -1);
    for (std::size_t i = 0; i < namen.size(); ++i) { u.knochenSpiegel[i] = modell::spiegelKnochen(namen, zi(i)); }
    return u;
}

int modellStapelTeil() {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    const auto gut = [&](int i) { return i >= 0 && uz(i) < in.teile.size() && !in.teile[uz(i)].istTag(); };
    // 1. die im Baum oder in der Ansicht gewaehlte Flaeche
    for (const int i : t.auswahl) {
        if (gut(i)) { return i; }
    }
    // 2. die Flaeche der Auswahl des Modellierens
    const ModellMesh& m = t.mesh;
    if (!m.auswahl.flaechen.empty() && m.auswahl.flaechen.front() >= 0 && uz(m.auswahl.flaechen.front()) < m.netz.flaechen.size() &&
        gut(m.netz.flaechen[uz(m.auswahl.flaechen.front())].teil)) {
        return m.netz.flaechen[uz(m.auswahl.flaechen.front())].teil;
    }
    const int punkt = !m.auswahl.punkte.empty() ? m.auswahl.punkte.front() : (!m.auswahl.kanten.empty() ? m.auswahl.kanten.front().first : -1);
    if (punkt >= 0 && uz(punkt) < m.netz.punkte.size() && gut(m.netz.punkte[uz(punkt)].teil)) { return m.netz.punkte[uz(punkt)].teil; }
    // 3. die einzige Flaeche (ein Requisit), sonst die erste mit einem Eintrag
    int einzige = -1;
    int zahl = 0;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (gut(zi(i))) {
            einzige = zi(i);
            ++zahl;
        }
    }
    if (zahl == 1) { return einzige; }
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (gut(zi(i)) && !in.teile[i].stapel.empty()) { return zi(i); }
    }
    return -1;
}

const GlmModel& modellStapelZeichnung() {
    zeichnungAbgleichen();
    return g_stapel.zeichnungAn ? g_stapel.zeichnung : mt().anzeige;
}

bool modellStapelErsetzt(int flaeche) {
    zeichnungAbgleichen();
    return g_stapel.zeichnungAn && flaeche >= 0 && uz(flaeche) < g_stapel.ersetzt.size() && g_stapel.ersetzt[uz(flaeche)] != 0;
}

const modell::Inhalt& modellStapelDateiInhalt() {
    const modell::Dokument& d = mt().dok;
    const modell::Inhalt& in = d.inhalt();
    if (!modell::stapelWirkt(in, StapelZweck::Datei)) { return in; }
    ModellStapel& z = g_stapel;
    // einmal je Stand des Dokuments (die Pruefung fuer das Spiel fragt in jedem Bild)
    if (z.dateiFuer != d.stand() || z.dateiVon != &in) {
        z.dateiInhalt = modell::stapelFuerDatei(in, modellStapelUmgebung());
        z.dateiFuer = d.stand();
        z.dateiVon = &in;
        ++z.dateiAuswertungen;
    }
    return z.dateiInhalt;
}

modell::Netz modellStapelVorschauNetz(int teil) {
    const modell::Teil* t = teilVon(teil);
    if (t == nullptr) { return modell::Netz{}; }
    return modell::stapelNetz(*t, mt().lodJetzt, StapelZweck::Ansicht, modellStapelUmgebung());
}

bool modellStapelHinzufuegen(ModArt art) {
    const int teil = modellStapelTeil();
    if (teil < 0) {
        melden(tr(Str::StpMeldKeineFlaeche));
        return false;
    }
    modell::Dokument& d = vorBefehl();
    Modifikator m;
    m.art = art;
    m.kennung = modell::stapelNeueKennung(d.inhalt());
    // Glaetten ist ab Werk "nur Vorschau" (Entwurf: "TurboSmooth (nur Vorschau)", "Unterteilung (nur Vorschau)") - schon eine
    // Unterteilung versechsfacht die Dreiecke und sprengt bei einer Figur die Grenze des Spiels je Flaeche
    m.datei = art == ModArt::Spiegeln;
    (void)d.aendern(format(Str::StpAktHinzu, tr(artStr(art))), [&](modell::Inhalt& x) { x.teile[uz(teil)].stapel.push_back(m); });
    waehlen(m.kennung);
    melden(format(Str::StpMeldHinzu, eintragName(m).c_str(), flaechenName(teil).c_str()));
    nachBefehl();
    return true;
}

bool modellStapelEntfernen(std::uint32_t kennung) {
    int index = -1;
    const int teil = teilMit(kennung, &index);
    if (teil < 0) {
        melden(tr(Str::StpMeldKeiner));
        return false;
    }
    const std::string name = eintragName(teilVon(teil)->stapel[uz(index)]);
    (void)vorBefehl().aendern(tr(Str::StpAktEntfernen), [&](modell::Inhalt& x) {
        auto& s = x.teile[uz(teil)].stapel;
        s.erase(s.begin() + index);
    });
    if (g_stapel.gewaehlt == kennung) { waehlen(0); }
    melden(format(Str::StpMeldEntfernt, name.c_str(), flaechenName(teil).c_str()));
    nachBefehl();
    return true;
}

bool modellStapelSchalten(std::uint32_t kennung, bool datei) {
    int index = -1;
    const int teil = teilMit(kennung, &index);
    if (teil < 0) {
        melden(tr(Str::StpMeldKeiner));
        return false;
    }
    (void)vorBefehl().aendern(tr(datei ? Str::StpAktDatei : Str::StpAktAnAus), [&](modell::Inhalt& x) {
        Modifikator& m = x.teile[uz(teil)].stapel[uz(index)];
        (datei ? m.datei : m.an) = !(datei ? m.datei : m.an);
    });
    const Modifikator& m = teilVon(teil)->stapel[uz(index)];
    // der Name ohne den Zusatz "nur Vorschau" - die Meldung nennt den neuen Zustand selbst
    Modifikator ohne = m;
    ohne.datei = true;
    const std::string name = eintragName(ohne);
    if (datei) {
        melden(format(m.datei ? Str::StpMeldDateiAn : Str::StpMeldDateiAus, name.c_str()));
    } else {
        melden(format(m.an ? Str::StpMeldAn : Str::StpMeldAus, name.c_str()));
    }
    nachBefehl();
    return true;
}

bool modellStapelBewegen(std::uint32_t kennung, int sichtbar) {
    int index = -1;
    const int teil = teilMit(kennung, &index);
    if (teil < 0) {
        melden(tr(Str::StpMeldKeiner));
        return false;
    }
    // wie 3ds Max steht der letzte Eintrag oben: "nach oben" heisst dort spaeter rechnen, wie Blender frueher
    const int schritt = modellBlender() ? sichtbar : -sichtbar;
    const int ziel = index + (schritt < 0 ? -1 : 1);
    const modell::Teil& t = *teilVon(teil);
    if (ziel < 0 || uz(ziel) >= t.stapel.size()) { return false; }
    const std::string name = eintragName(t.stapel[uz(index)]);
    (void)vorBefehl().aendern(tr(sichtbar < 0 ? Str::StpAktHoch : Str::StpAktRunter), [&](modell::Inhalt& x) {
        auto& s = x.teile[uz(teil)].stapel;
        std::swap(s[uz(index)], s[uz(ziel)]);
    });
    melden(format(Str::StpMeldBewegt, name.c_str()));
    nachBefehl();
    return true;
}

bool modellStapelAnwenden(std::uint32_t kennung) {
    int index = -1;
    const int teil = teilMit(kennung, &index);
    if (teil < 0) {
        melden(tr(Str::StpMeldKeiner));
        return false;
    }
    const modell::Teil& t = *teilVon(teil);
    const Modifikator m = t.stapel[uz(index)];
    Modifikator ohne = m;
    ohne.datei = true;
    const std::string name = eintragName(ohne);
    if (!m.an) {   // Blender: "Modifier is disabled, skipping apply"
        melden(format(Str::StpMeldIstAus, name.c_str()));
        return false;
    }
    std::vector<int> eintraege;
    bool nichtErster = false;
    if (modellBlender()) {
        // wie Blender: nur dieser Eintrag, auf dem Netz der Flaeche - stand ein eingeschalteter davor, sagt Blender dasselbe
        eintraege.push_back(index);
        for (int i = 0; i < index; ++i) { nichtErster = nichtErster || t.stapel[uz(i)].an; }
    } else {
        // wie 3ds Max "Collapse To": alle eingeschalteten bis hierher
        for (int i = 0; i <= index; ++i) {
            if (t.stapel[uz(i)].an) { eintraege.push_back(i); }
        }
    }
    if (!anwenden(teil, eintraege, modellBlender() ? Str::StpAktAnwendenBl : Str::StpAktAnwendenMax, name)) { return false; }
    if (g_stapel.gewaehlt == kennung) { waehlen(0); }
    if (nichtErster) { melden(tr(Str::StpMeldNichtErster)); }
    return true;
}

bool modellStapelAllesAnwenden() {
    const int teil = modellStapelTeil();
    const modell::Teil* t = teilVon(teil);
    if (t == nullptr) {
        melden(tr(Str::StpMeldKeineFlaeche));
        return false;
    }
    std::vector<int> eintraege;
    for (std::size_t i = 0; i < t->stapel.size(); ++i) {
        if (t->stapel[i].an) { eintraege.push_back(zi(i)); }
    }
    if (eintraege.empty()) {
        melden(format(Str::StpMeldNichtsAn, flaechenName(teil).c_str()));
        return false;
    }
    const Str name = modellBlender() ? Str::StpAktAlleBl : Str::StpAktAlleMax;
    if (!anwenden(teil, eintraege, name, tr(name))) { return false; }
    waehlen(0);
    return true;
}

void modellStapelZeigen() {
    ModellTab& t = mt();
    if (t.bereich != ModellBereich::Modellieren && t.bereich != ModellBereich::Weights) { t.bereich = ModellBereich::Modellieren; }
    // wie Blender steht die Gruppe im Reiter Modifier (im Bereich Weights gibt der Bereich den Reiter Daten vor)
    g_stapel.zeigen = 2;
    g_app->modelDirty = true;
}

// --- das Feld -------------------------------------------------------------------------------------------------------------

namespace {

constexpr unsigned kKlick = 1U;
constexpr unsigned kAuge = 2U;
constexpr unsigned kDatei = 4U;
constexpr unsigned kRechts = 8U;
constexpr unsigned kZieht = 16U;

struct ZeileOpt {
    bool gewaehlt = false;
    bool gedaempft = false;       // ausgeschaltet
    bool auge = false;            // mit dem Knopf "an/aus"
    bool an = true;
    bool dateiKnopf = false;      // mit dem Knopf "in die Datei" (wie Blender in der Zeile; wie 3ds Max bei den Einstellungen)
    bool datei = true;
    bool warnung = false;         // der Eintrag konnte nicht rechnen
    const char* hinweis = nullptr;
};

// Eine Zeile des Stapels in der Form des Entwurfs (.stapel: Kasten mit Name links und Symbolen rechts, die gewaehlte in der
// Betonungsfarbe) - dieselbe Form wie die Ebenen des Paintings (gui/app_modelltab_malen.cpp ebenenZeile).
unsigned stapelZeile(const std::string& text, const ZeileOpt& o, ImRect* rechteck) {
    ImGuiContext& g = *GImGui;
    const float fs = ImGui::GetFontSize();
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), fs * 4.0F);
    const float hoehe = std::floor(fs * 1.7F);
    const float knopf = std::floor(fs * 1.3F);
    const float randX = std::floor(fs * 0.45F);
    const int knoepfe = (o.auge ? 1 : 0) + (o.dateiKnopf ? 1 : 0);
    const float textB = std::max(breite - randX * 2.0F - knopf * static_cast<float>(knoepfe), fs);
    const ImVec2 textG = ImGui::CalcTextSize(text.c_str());
    unsigned r = 0;
    ImGui::SetNextItemAllowOverlap();   // die Knoepfe liegen auf der Zeile und bekommen ihren Klick
    if (ImGui::InvisibleButton("##zeile", ImVec2{breite, hoehe})) { r |= kKlick; }
    const bool ueber = ImGui::IsItemHovered();
    if (ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { r |= kRechts; }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) { r |= kZieht; }
    const ImRect bb{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
    if (rechteck != nullptr) { *rechteck = bb; }
    IMGUI_TEST_ENGINE_ITEM_INFO(ImGui::GetItemID(), text.c_str(), g.LastItemData.StatusFlags | (o.gewaehlt ? ImGuiItemStatusFlags_Checked : 0));
    if (o.hinweis != nullptr) {
        // der Name gekuerzt: ganz im Tooltip
        tippUmbrochen(textG.x > textB ? text + "\n" + o.hinweis : std::string(o.hinweis));
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rund = ui::rolloutStil() == ui::RolloutStil::Blender ? std::floor(fs * 0.3F) : 2.0F;
    const ImVec4 betont = ui::betonungsFarbe();
    const auto zuWeiss = [](const ImVec4& c, float t) { return ImVec4{c.x + (1.0F - c.x) * t, c.y + (1.0F - c.y) * t, c.z + (1.0F - c.z) * t, 1.0F}; };
    ImVec4 grund = ImGui::GetStyleColorVec4(ueber ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 kante = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 schrift = o.warnung ? ui::tonFarbe(ui::Ton::Warnung) : ImGui::GetStyleColorVec4(o.gedaempft ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    if (o.gewaehlt) {
        grund = ueber ? zuWeiss(betont, 0.15F) : betont;
        kante = zuWeiss(betont, 0.25F);
        const bool hell = betont.x * 0.3F + betont.y * 0.59F + betont.z * 0.11F > 0.6F;
        schrift = hell ? ImVec4{0.0F, 0.0F, 0.0F, 1.0F} : ImVec4{1.0F, 1.0F, 1.0F, 1.0F};
        if (o.gedaempft) { schrift.w = 0.6F; }
    }
    dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(grund), rund);
    dl->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(kante), rund);
    const ImVec2 textOrt{bb.Min.x + randX, std::floor(bb.Min.y + (hoehe - fs) * 0.5F)};
    ImGui::PushStyleColor(ImGuiCol_Text, schrift);
    ImGui::RenderTextEllipsis(dl, textOrt, ImVec2{textOrt.x + textB, bb.Max.y}, textOrt.x + textB, text.c_str(), nullptr, &textG);
    ImGui::PopStyleColor();
    const ImVec2 danach = ImGui::GetCursorScreenPos();
    const float symbol = std::floor(knopf * 0.72F);
    // von rechts: "in die Datei", davor das Auge (Entwurf wie Blender: Auge, dann der Schalter der Ausgabe)
    float x = bb.Max.x - std::floor(randX * 0.5F);
    const auto zeilenKnopf = [&](Str name, Str hinweis, UiIcon icon, bool an) {
        x -= knopf;
        const ImVec2 ort{x, std::floor(bb.Min.y + (hoehe - knopf) * 0.5F)};
        ImGui::SetCursorScreenPos(ort);
        const bool klick = ImGui::InvisibleButton(tr(name), ImVec2{knopf, knopf});
        IMGUI_TEST_ENGINE_ITEM_INFO(ImGui::GetItemID(), tr(name), g.LastItemData.StatusFlags | (an ? ImGuiItemStatusFlags_Checked : 0));
        if (ImGui::IsItemHovered()) { dl->AddRectFilled(ort, ImVec2{ort.x + knopf, ort.y + knopf}, ImGui::GetColorU32(ImGuiCol_ButtonHovered), rund); }
        tippUmbrochen(std::string(tr(name)) + "\n" + tr(hinweis));
        ImVec4 farbe = schrift;
        if (!an) { farbe.w *= 0.4F; }
        uiIcon(dl, icon, ImVec2{std::floor(ort.x + (knopf - symbol) * 0.5F), std::floor(ort.y + (knopf - symbol) * 0.5F)}, symbol, ImGui::GetColorU32(farbe));
        return klick;
    };
    if (o.dateiKnopf && zeilenKnopf(Str::StpDatei, Str::StpDateiHint, UiIcon::Speichern, o.datei)) { r |= kDatei; }
    if (o.auge && zeilenKnopf(Str::StpAn, Str::StpAnHint, o.an ? UiIcon::Sichtbar : UiIcon::Versteckt, o.an)) { r |= kAuge; }
    ImGui::SetCursorScreenPos(danach);
    return r;
}

// Die zwei Arten als Menuezeilen (Knopf "Modifier-Liste" / "Modifier hinzufuegen", Rechtsklick, Menue)
void hinzuZeilen() {
    const bool aktiv = modellStapelTeil() >= 0;
    for (const ModArt art : {ModArt::Spiegeln, ModArt::Glaetten}) {
        if (ImGui::MenuItem(tr(artStr(art)), nullptr, false, aktiv)) { (void)modellStapelHinzufuegen(art); }
        ImGui::SetItemTooltip("%s", tr(art == ModArt::Spiegeln ? Str::StpSpiegelnHint : Str::StpGlaettenHint));
    }
}

// Die Befehle fuer einen Eintrag als Menuezeilen (Rechtsklick auf die Zeile, Rechtsklick der Ansicht, Menue)
void eintragZeilen(std::uint32_t kennung) {
    int index = -1;
    const int teil = teilMit(kennung, &index);
    const modell::Teil* t = teilVon(teil);
    const bool da = t != nullptr;
    const bool bl = modellBlender();
    const Modifikator m = da ? t->stapel[uz(index)] : Modifikator{};
    const int letzter = da ? zi(t->stapel.size()) - 1 : 0;
    // sichtbar nach oben: wie Blender frueher (Index - 1), wie 3ds Max spaeter (Index + 1)
    const bool hoch = da && (bl ? index > 0 : index < letzter);
    const bool runter = da && (bl ? index < letzter : index > 0);
    if (ImGui::MenuItem(tr(Str::StpAn), nullptr, da && m.an, da)) { (void)modellStapelSchalten(kennung, false); }
    ImGui::SetItemTooltip("%s", tr(Str::StpAnHint));
    if (ImGui::MenuItem(tr(Str::StpDatei), nullptr, da && m.datei, da)) { (void)modellStapelSchalten(kennung, true); }
    ImGui::SetItemTooltip("%s", tr(Str::StpDateiHint));
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::StpHoch), nullptr, false, hoch)) { (void)modellStapelBewegen(kennung, -1); }
    ImGui::SetItemTooltip("%s", tr(Str::StpBewegenHint));
    if (ImGui::MenuItem(tr(Str::StpRunter), nullptr, false, runter)) { (void)modellStapelBewegen(kennung, 1); }
    ImGui::SetItemTooltip("%s", tr(Str::StpBewegenHint));
    ImGui::Separator();
    if (ImGui::MenuItem(tr(bl ? Str::StpAnwendenBl : Str::StpAnwendenMax), nullptr, false, da)) { (void)modellStapelAnwenden(kennung); }
    ImGui::SetItemTooltip("%s", tr(bl ? Str::StpAnwendenBlHint : Str::StpAnwendenMaxHint));
    if (ImGui::MenuItem(tr(bl ? Str::StpAlleAnwendenBl : Str::StpAlleAnwendenMax), nullptr, false, modellStapelTeil() >= 0)) { (void)modellStapelAllesAnwenden(); }
    ImGui::SetItemTooltip("%s", tr(Str::StpAlleAnwendenHint));
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::StpEntfernen), nullptr, false, da)) { (void)modellStapelEntfernen(kennung); }
    ImGui::SetItemTooltip("%s", tr(Str::StpEntfernenHint));
}

std::string mitKennung(Str text, const char* kennung) { return std::string(tr(text)) + "###" + kennung; }

// Die Einstellungen des gewaehlten Eintrags unter der Liste
void einstellungen(int teil, const modell::Teil& t, int index) {
    const Modifikator m = t.stapel[uz(index)];
    const std::uint32_t k = m.kennung;
    const bool bl = modellBlender();
    if (m.art == ModArt::Spiegeln) {
        static const char* const kAchsen[3] = {"X", "Y", "Z"};
        ui::zeile(tr(Str::StpAchse), tr(Str::StpAchseHint));
        int achse = std::min<int>(m.achse, 2);
        if (ui::segment("##stpachse", kAchsen, 3, &achse, 3, -1, nullptr, ui::feldBreite())) {
            einstellung(0, k, [achse](Modifikator& x) { x.achse = static_cast<std::uint8_t>(achse); });
        }
        ui::zeile(tr(Str::StpRichtung), tr(Str::StpRichtungHint));
        const char* const richtungen[2] = {tr(Str::StpPlusMinus), tr(Str::StpMinusPlus)};
        int richtung = m.vonPlus ? 0 : 1;
        if (ui::segment("##stprichtung", richtungen, 2, &richtung, 2, -1, nullptr, ui::feldBreite())) {
            einstellung(0, k, [richtung](Modifikator& x) { x.vonPlus = richtung == 0; });
        }
    } else {
        int stufen = std::clamp<int>(m.stufen, 1, modell::kGlaettenStufenMax);
        if (ui::reglerInt(mitKennung(bl ? Str::StpStufenBl : Str::StpStufenMax, "stpstufen").c_str(), &stufen, 1, modell::kGlaettenStufenMax, "%d", tr(Str::StpStufenHint))) {
            stufen = std::clamp(stufen, 1, modell::kGlaettenStufenMax);
            einstellung(ImGui::GetActiveID(), k, [stufen](Modifikator& x) { x.stufen = static_cast<std::uint8_t>(stufen); });
        }
    }
    // Schalter: Raender halten (Glaetten), in die Datei (wie 3ds Max hier; wie Blender steht er auch in der Zeile)
    ui::rasterBeginn(2);
    if (m.art == ModArt::Glaetten) {
        bool halten = m.raenderHalten;
        if (ui::schalter(tr(Str::StpRaender), &halten, tr(Str::StpRaenderHint))) {
            einstellung(0, k, [halten](Modifikator& x) { x.raenderHalten = halten; });
        }
    }
    bool datei = m.datei;
    if (ui::schalter(mitKennung(Str::StpDatei, "stpdateischalter").c_str(), &datei, tr(Str::StpDateiHint))) { (void)modellStapelSchalten(k, true); }
    ui::rasterEnde();
    // Ecken und Dreiecke vor und nach dem Eintrag (in der Stufe der Ansicht), gegen die Grenze des Zielspiels
    zeichnungAbgleichen();
    const ModellStapel& z = g_stapel;
    if (z.zeichnungAn && uz(teil) < z.stand.size() && uz(index) < z.stand[uz(teil)].nach.size()) {
        const ModellStapel::TeilStand& s = z.stand[uz(teil)];
        const modell::NetzZahlen vor = index > 0 ? s.nach[uz(index) - 1U] : s.vor;
        const modell::NetzZahlen nach = s.nach[uz(index)];
        const int grund = uz(index) < s.status.size() ? s.status[uz(index)] : 0;
        if (grund != 0) {
            ui::absatz(format(Str::StpNichtGerechnet, grundText(static_cast<wof::Ablehnung>(grund)).c_str()).c_str(), ui::Ton::Warnung);
        } else if (m.an) {
            const modell::Grenzen& gr = modell::grenzen(static_cast<modell::Zielspiel>(mt().zielspiel));
            const bool ueber = nach.ecken >= gr.eckenJeFlaeche;   // SHADER_MAX_VERTEXES: schon die Grenze selbst ist zu viel (RB_CheckOverflow)
            char ecken[64];
            char dreiecke[64];
            std::snprintf(ecken, sizeof ecken, "%d \xE2\x86\x92 %d", vor.ecken, nach.ecken);
            std::snprintf(dreiecke, sizeof dreiecke, "%d \xE2\x86\x92 %d", vor.dreiecke, nach.dreiecke);
            const ui::Ton ton = !ueber ? ui::Ton::Normal : (m.datei ? ui::Ton::Fehler : ui::Ton::Warnung);
            ui::wertTabelle({{tr(Str::StpEcken), ecken, ton, tr(Str::StpZahlenHint)}, {tr(Str::StpDreiecke), dreiecke, ui::Ton::Normal, tr(Str::StpZahlenHint)}});
            if (ueber) {
                ui::absatz(format(m.datei ? Str::StpGrenzeDatei : Str::StpGrenzeVorschau, nach.ecken, gr.name, gr.eckenJeFlaeche - 1).c_str(),
                           m.datei ? ui::Ton::Fehler : ui::Ton::Leise);
            }
        }
    }
}

}  // namespace

void drawModellStapelGruppe() {
    ModellTab& t = mt();
    ModellStapel& z = g_stapel;
    const modell::Inhalt& in = t.dok.inhalt();
    if (in.leer() || g_app->model.empty()) { return; }
    if (z.reglerAktiv != 0U && ImGui::GetActiveID() != z.reglerAktiv) { z.reglerAktiv = 0; }
    const bool bl = modellBlender();
    const int teil = modellStapelTeil();
    const modell::Teil* tl = teilVon(teil);
    const char* titel = tr(bl ? Str::StpGruppeBl : Str::MgRStapel);
    if (z.zeigen > 0) {   // "Modifier-Stapel zeigen": den Reiter waehlen, aufklappen, ins Bild rollen
        modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier);
        ui::rolloutZeigen(titel);
        --z.zeigen;
    }
    z.probeBild = ImGui::GetFrameCount();
    z.probeTeil = teil;
    z.probeZeilen.clear();
    z.probeRechtecke.clear();
    z.probeGewaehlt = -1;
    // wie 3ds Max im Reiter Aendern (dort gibt ihn der Bereich vor); wie Blender im Reiter Modifier - im Bereich Weights gibt
    // der Bereich den Reiter Daten vor, dort zeigt der Entwurf keinen Stapel
    const bool auchStandard = !(bl && t.bereich != ModellBereich::Modellieren);
    const std::string info = tl != nullptr ? tl->name : std::string();
    if (!ui::rolloutBeginn(titel, {.kurzinfo = info.empty() ? nullptr : info.c_str(), .reiter = modellReiter(MbReiter::Aendern, MbReiterBl::Modifier, auchStandard)})) { return; }
    if (tl == nullptr) {
        ui::absatz(tr(Str::StpKeineFlaeche));
        ui::rolloutEnde();
        return;
    }
    const float fs = ImGui::GetFontSize();
    // Hinzufuegen: wie 3ds Max die "Modifier-Liste" ueber dem Stapel, wie Blender "Modifier hinzufuegen"
    const std::string hinzuHinweis = (tl->stapel.empty() ? std::string(tr(bl ? Str::StpLeerBl : Str::StpLeerMax)) + "\n\n" : std::string()) +
                                     tr(Str::StpHinzuHint) + "\n\n" + tr(Str::StpTipp);
    if (ui::werkzeugKnopf(tr(bl ? Str::StpHinzuBl : Str::StpHinzuMax), {.icon = UiIcon::Plus, .hinweis = hinzuHinweis.c_str()})) { ImGui::OpenPopup("##stpneu"); }
    if (ImGui::BeginPopup("##stpneu")) {
        hinzuZeilen();
        ImGui::EndPopup();
    }
    // die Zeilen von oben nach unten
    const std::uint32_t gewaehlt = modell::stapelIndex(*tl, z.gewaehlt) >= 0 ? z.gewaehlt : 0U;
    const bool skelett = in.herkunft == modell::Herkunft::Glm && in.knochenZahl > 0;
    zeichnungAbgleichen();
    const std::vector<int>* status = z.zeichnungAn && uz(teil) < z.stand.size() ? &z.stand[uz(teil)].status : nullptr;
    std::uint32_t bewegen = 0;
    int bewegenRichtung = 0;
    std::uint32_t kontext = 0;
    std::uint32_t schalten = 0;
    bool schaltenDatei = false;
    const ImGuiStyle& stil = ImGui::GetStyle();
    const float eng = std::floor(fs * 0.18F);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{stil.ItemSpacing.x, eng});
    const auto merken = [&](const std::string& text, const ImRect& r, bool gew) {
        if (gew) { z.probeGewaehlt = zi(z.probeZeilen.size()); }
        z.probeZeilen.push_back(text);
        z.probeRechtecke.push_back(r);
    };
    // der Tipp (Stapel bleibt aenderbar, was in die Datei geht) steht im Tooltip jeder Zeile - als Absatz unter einem vollen
    // Stapel schob er die Gruppen darunter aus dem Bild (Foto gegen den Entwurf M1/M2)
    const std::string zeilenHinweis = std::string(tr(Str::StpZeileHint)) + "\n\n" + tr(Str::StpTipp);
    const auto eintragZeile = [&](int index) {
        const Modifikator& m = tl->stapel[uz(index)];
        const std::string text = eintragName(m);
        ZeileOpt o;
        o.gewaehlt = m.kennung == gewaehlt;
        o.gedaempft = !m.an;
        o.auge = true;
        o.an = m.an;
        o.dateiKnopf = bl;
        o.datei = m.datei;
        o.warnung = status != nullptr && uz(index) < status->size() && (*status)[uz(index)] != 0;
        o.hinweis = zeilenHinweis.c_str();
        ImRect r;
        ImGui::PushID(static_cast<int>(m.kennung));
        const unsigned e = stapelZeile(text, o, &r);
        ImGui::PopID();
        merken(text, r, o.gewaehlt);
        // die Befehle erst nach der Liste (sie aendern das Dokument, aus dem die Liste gerade zeichnet)
        if ((e & kAuge) != 0U) {
            schalten = m.kennung;
            schaltenDatei = false;
        } else if ((e & kDatei) != 0U) {
            schalten = m.kennung;
            schaltenDatei = true;
        } else if ((e & kKlick) != 0U) {
            waehlen(m.kennung);
        }
        if ((e & kRechts) != 0U) { kontext = m.kennung; }
        if ((e & kZieht) != 0U) {   // ziehen: ueber die Kante der Zeile hinaus tauscht sie mit dem Nachbarn
            const float y = ImGui::GetIO().MousePos.y;
            if (y < r.Min.y - eng || y > r.Max.y + eng) {
                bewegen = m.kennung;
                bewegenRichtung = y < r.Min.y ? -1 : 1;
            }
        }
    };
    const auto stufenZeile = [&](const char* kennung, const std::string& text, bool gew, const char* hinweis, ModellBereich ziel, bool wechselt) {
        ZeileOpt o;
        o.gewaehlt = gew;
        o.hinweis = hinweis;
        ImRect r;
        ImGui::PushID(kennung);
        const unsigned e = stapelZeile(text, o, &r);
        ImGui::PopID();
        merken(text, r, gew);
        if ((e & kKlick) != 0U) {
            waehlen(0);
            if (wechselt && t.bereich != ziel) {
                t.bereich = ziel;
                g_app->modelDirty = true;
            }
        }
    };
    if (bl) {
        for (int i = 0; i < zi(tl->stapel.size()); ++i) { eintragZeile(i); }
    } else {
        // wie 3ds Max von unten nach oben: Flaeche der Datei, Edit Poly, die Eintraege, Skin. Die Eintraege rechnen in der
        // Bindepose, die Figur wird danach verformt - deshalb steht Skin ueber ihnen.
        if (skelett) {
            stufenZeile("skin", tr(Str::MgStapelSkin), gewaehlt == 0U && t.bereich == ModellBereich::Weights,
                        tr(t.bereich == ModellBereich::Weights ? Str::MgStapelSkinHint : Str::StpSkinWechsel), ModellBereich::Weights, true);
        }
        for (int i = zi(tl->stapel.size()) - 1; i >= 0; --i) { eintragZeile(i); }
        stufenZeile("poly", tr(Str::MgStapelPoly), gewaehlt == 0U && t.bereich == ModellBereich::Modellieren,
                    tr(t.bereich == ModellBereich::Modellieren ? Str::StpPolyHier : Str::MgStapelPolyHint), ModellBereich::Modellieren, true);
        stufenZeile("basis", format(in.herkunft == modell::Herkunft::Md3 ? Str::StpBasisMd3 : Str::StpBasisGlm, tl->name.c_str()), false, tr(Str::StpBasisHint),
                    t.bereich, false);
    }
    ImGui::PopStyleVar();
    // wieder der volle Abstand nach der Liste: zurueck um den engen, dann der Anker (Dummy rueckt um ItemSpacing.y weiter)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - eng);
    ImGui::Dummy({0.0F, 0.0F});
    if (kontext != 0U) {
        z.kontext = kontext;
        waehlen(kontext);
        ImGui::OpenPopup("##stpkontext");
    }
    if (ImGui::BeginPopup("##stpkontext")) {
        eintragZeilen(z.kontext);
        ImGui::EndPopup();
    }
    if (schalten != 0U) { (void)modellStapelSchalten(schalten, schaltenDatei); }
    if (bewegen != 0U && modellStapelBewegen(bewegen, bewegenRichtung)) { ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left); }
    // nach den Befehlen der Zeilen neu lesen (das Dokument kann sich geaendert haben)
    tl = teilVon(teil);
    if (tl == nullptr) {
        ui::rolloutEnde();
        return;
    }
    const int index = modell::stapelIndex(*tl, z.gewaehlt);
    // Platz nur, wenn gebraucht: ein leerer Stapel ist wie im Vorbild nur der Knopf (Blender) bzw. die Zeilen (3ds Max); der
    // Hinweis dazu und der Tipp stehen im Tooltip des Knopfs und der Zeilen. Als Absaetze schoben sie die Gruppen darunter aus dem
    // Bild (Foto gegen den Entwurf M1/M2; Selbsttest ap6: Werkzeuge des Requisits unter der Zeitleiste)
    if (index >= 0) {
        einstellungen(teil, *tl, index);
        const std::uint32_t k = z.gewaehlt;
        const int letzter = zi(tl->stapel.size()) - 1;
        const bool an = tl->stapel[uz(index)].an;
        // alles, was die Knoepfe brauchen, vorher lesen: ein Befehl aendert das Dokument
        std::string anwendenHinweis = tr(bl ? Str::StpAnwendenBlHint : Str::StpAnwendenMaxHint);
        if (!an) { anwendenHinweis += "\n" + format(Str::StpMeldIstAus, tr(artStr(tl->stapel[uz(index)].art))); }
        ui::rasterBeginn(2);
        if (ui::werkzeugKnopf(tr(Str::StpHoch), {.icon = UiIcon::MalHoch, .hinweis = tr(Str::StpBewegenHint), .aktiv = bl ? index > 0 : index < letzter})) {
            (void)modellStapelBewegen(k, -1);
        }
        if (ui::werkzeugKnopf(tr(Str::StpRunter), {.icon = UiIcon::MalRunter, .hinweis = tr(Str::StpBewegenHint), .aktiv = bl ? index < letzter : index > 0})) {
            (void)modellStapelBewegen(k, 1);
        }
        if (ui::werkzeugKnopf(tr(bl ? Str::StpAnwendenBl : Str::StpAnwendenMax), {.icon = UiIcon::Haken, .hinweis = anwendenHinweis.c_str(), .aktiv = an})) {
            (void)modellStapelAnwenden(k);
        }
        if (ui::werkzeugKnopf(tr(Str::StpEntfernen), {.icon = UiIcon::Loeschen, .hinweis = tr(Str::StpEntfernenHint)})) { (void)modellStapelEntfernen(k); }
        ui::rasterEnde();
    }
    ui::rolloutEnde();
}

void modellStapelMenue() {
    if (mt().dok.inhalt().leer() || g_app->model.empty()) { return; }
    if (!ImGui::BeginMenu(tr(modellBlender() ? Str::StpGruppeBl : Str::MgRStapel))) { return; }
    if (ImGui::BeginMenu(tr(modellBlender() ? Str::StpHinzuBl : Str::StpHinzuMax), modellStapelTeil() >= 0)) {
        hinzuZeilen();
        ImGui::EndMenu();
    }
    ImGui::Separator();
    eintragZeilen(gewaehlterEintrag());
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::StpAktZeigen))) { modellStapelZeigen(); }
    ImGui::SetItemTooltip("%s", tr(Str::StpAktZeigenHint));
    ImGui::EndMenu();
}

bool modellStapelAktionenSuche(const std::string& such) {
    if (mt().dok.inhalt().leer() || g_app->model.empty()) { return false; }
    struct Eintrag {
        std::string text;
        const char* hinweis;
        std::function<void()> tun;
    };
    const bool bl = modellBlender();
    const std::vector<Eintrag> liste = {
        {format(Str::StpAktHinzu, tr(artStr(ModArt::Spiegeln))), tr(Str::StpSpiegelnHint), [] { (void)modellStapelHinzufuegen(ModArt::Spiegeln); }},
        {format(Str::StpAktHinzu, tr(artStr(ModArt::Glaetten))), tr(Str::StpGlaettenHint), [] { (void)modellStapelHinzufuegen(ModArt::Glaetten); }},
        {tr(Str::StpAktAnAus), tr(Str::StpAnHint), [] { (void)modellStapelSchalten(gewaehlterEintrag(), false); }},
        {tr(Str::StpAktDatei), tr(Str::StpDateiHint), [] { (void)modellStapelSchalten(gewaehlterEintrag(), true); }},
        {tr(Str::StpAktHoch), tr(Str::StpBewegenHint), [] { (void)modellStapelBewegen(gewaehlterEintrag(), -1); }},
        {tr(Str::StpAktRunter), tr(Str::StpBewegenHint), [] { (void)modellStapelBewegen(gewaehlterEintrag(), 1); }},
        {tr(bl ? Str::StpAktAnwendenBl : Str::StpAktAnwendenMax), tr(bl ? Str::StpAnwendenBlHint : Str::StpAnwendenMaxHint), [] { (void)modellStapelAnwenden(gewaehlterEintrag()); }},
        {tr(bl ? Str::StpAktAlleBl : Str::StpAktAlleMax), tr(Str::StpAlleAnwendenHint), [] { (void)modellStapelAllesAnwenden(); }},
        {tr(Str::StpAktEntfernen), tr(Str::StpEntfernenHint), [] { (void)modellStapelEntfernen(gewaehlterEintrag()); }},
        {tr(Str::StpAktZeigen), tr(Str::StpAktZeigenHint), [] { modellStapelZeigen(); }},
    };
    bool ausgeloest = false;
    for (const Eintrag& e : liste) {
        if (!such.empty() && klein(e.text).find(such) == std::string::npos) { continue; }
        if (ImGui::Selectable(e.text.c_str())) {
            e.tun();
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
