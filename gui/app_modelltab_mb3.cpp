// app_modelltab_mb3.cpp - Model-Tab, Modell-Block Stufe 2: Modell speichern (.glm), Bereich Animation mit GLA,
// animation.cfg, Pruefungen, Karten-GLA und Plaetzen, Marker der Zeitleiste
//
//   Strg+S / Strg+Umschalt+S   .glm aus dem Modelldokument (bhed/modell/glmdatei.h); vorher die Grenzen des Zielspiels
//                              (modellBefunde) - liegt etwas darueber, fragt das Programm selbst (kein Windows-Dialog)
//   Bereich Animation          GLA speichern (schreibGla), animation.cfg mit Befunden je Zielspiel (cfgPruefen), Karten-GLA
//                              _humanoid_<karte> aus Plaetzen BOTH_CIN_n (Ziehen aus der Animationsliste, Rechtsklick)
//   Marker                     die Zeile der animation.cfg zur Sequenz in der Zeitleiste: Zahlen und Balkenenden, ueber das
//                              Dokument (ein Rueckgaengig-Schritt je Aenderung)
#include "app_internal.h"
#include "textumbruch.h"
#include "platform.h"

#include "bhed/modell/animcfg.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/xsiaufnahme.h"
#include "bhed/sicherschreiben.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

std::string kleinMb3(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Eine Datei, die neben dem Programm auf der Platte liegt (nicht in einem Archiv): dorthin speichert Strg+S selbst.
bool aufPlatte(const std::string& p) {
    std::error_code ec;
    return !p.empty() && std::filesystem::path(p).is_absolute() && std::filesystem::is_regular_file(p, ec);
}

modell::Zielspiel zielspiel() { return static_cast<modell::Zielspiel>(mt().zielspiel); }

void melden(const std::string& m) {
    mt().meldung = m;
    addStatus(m);
}

std::string format(Str s, auto... a) {
    char b[600];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

// --- Modell speichern --------------------------------------------------------------------------------------------

std::string befundText(const modell::Befund& x) {
    const modell::Grenzen& g = modell::grenzen(zielspiel());
    switch (x.art) {
        case modell::BefundArt::Ecken: return format(Str::MbPrEcken, x.name.c_str(), x.stufe + 1, x.wert, g.name, x.grenze);
        case modell::BefundArt::Dreiecke: return format(Str::MbPrDreiecke, x.name.c_str(), x.stufe + 1, x.wert, g.name, x.grenze);
        case modell::BefundArt::Knochen: return format(Str::MbPrKnochen, x.name.c_str(), x.stufe + 1, x.wert, x.grenze);
        case modell::BefundArt::Gewichte: return format(Str::MbPrGewichte, x.name.c_str(), x.stufe + 1, x.wert, x.grenze);
        case modell::BefundArt::G2Modelle: return format(Str::MbPrG2, x.wert, g.name, x.grenze);
        case modell::BefundArt::AnimHinterGla:
            return format(Str::MbPrAnim, x.name.c_str(), x.wert, x.grenze, tr(g.tPoseHinterGla ? Str::MbPrAnimTPose : Str::MbPrAnimStand));
        case modell::BefundArt::ShaderFehlt: return format(Str::MbPrShader, x.name.c_str());
    }
    return {};
}

std::string cfgBefundText(const modell::CfgBefund& x) { return modellCfgBefundText(x, zielspiel()); }
}  // namespace

// Stufe 7: auch fuer "Fuer das Spiel bauen" im Mission-Tab (Zielspiel aus dem Regelwerk)
std::string modellCfgBefundText(const modell::CfgBefund& x, modell::Zielspiel s) {
    const modell::Grenzen& g = modell::grenzen(s);
    switch (x.art) {
        case modell::CfgBefundArt::HinterGla:
            return format(Str::MbCfgHinterGla, x.name.c_str(), x.wert, x.grenze, tr(g.tPoseHinterGla ? Str::MbPrAnimTPose : Str::MbPrAnimStand));
        case modell::CfgBefundArt::ErsterFrame: return format(Str::MbCfgErsterFrame, x.name.c_str(), x.wert);
        case modell::CfgBefundArt::Anzahl: return format(Str::MbCfgAnzahl, x.name.c_str(), x.wert);
        case modell::CfgBefundArt::Schleife: return format(Str::MbCfgSchleife, x.name.c_str(), x.wert);
        case modell::CfgBefundArt::ZuGross: return format(Str::MbCfgZuGross, x.wert, g.name, x.grenze);
        case modell::CfgBefundArt::Knochen: return format(Str::MbCfgKnochen, x.name.c_str(), x.wert, x.grenze);
        case modell::CfgBefundArt::NameKopf: return format(Str::MbCfgNameKopf, x.name.c_str());
        case modell::CfgBefundArt::AnimDateien: return format(Str::MbCfgAnimDateien, x.wert, g.name, x.grenze);
        case modell::CfgBefundArt::KeineKartenGla: return format(Str::MbCfgKeineKartenGla, g.name);
        case modell::CfgBefundArt::Unbekannt: return format(Str::MbCfgUnbekannt, x.name.c_str());
        case modell::CfgBefundArt::NurSp: return format(Str::MbCfgNurSp, x.name.c_str(), g.name);
        case modell::CfgBefundArt::MpHinweis: return format(Str::MbCfgMpHinweis, g.name);
    }
    return {};
}

namespace {

bool schreibenNach(const std::string& ziel) {
    ModellTab& t = mt();
    std::string bytes;
    modell::GlmSchreibBericht br;
    if (t.dok.inhalt().herkunft == modell::Herkunft::Md3) {
        // Modell-Block 1b: eine .md3 (Bilder, Tags und unveraenderte Flaechen wie gelesen, siehe bhed/modell/md3datei.h)
        modell::Md3SchreibBericht mb;
        if (!modell::md3Schreiben(t.dok.inhalt(), bytes, modell::grenzen(static_cast<modell::Zielspiel>(t.zielspiel)).eckenJeFlaeche, &mb)) {
            melden(format(Str::MbSpFehler, modellFehlerText(mb.fehlerArt).c_str()));
            return false;
        }
        std::string fehler;
        if (!sicherSchreiben(ziel, bytes, &fehler)) {
            melden(format(Str::MbSpFehler, fehler.c_str()));
            return false;
        }
        t.speicherPfad = ziel;
        t.gespeichertStand = t.dok.stand();
        melden(format(Str::UvMd3Gespeichert, ziel.c_str(), static_cast<int>(bytes.size()), mb.flaechenRoh, mb.flaechenNeu));
        return true;
    }
    if (!modell::glmSchreiben(t.dok.inhalt(), bytes, &br)) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehlerArt).c_str()));
        return false;
    }
    std::string fehler;
    if (!sicherSchreiben(ziel, bytes, &fehler)) {
        melden(format(Str::MbSpFehler, fehler.c_str()));
        return false;
    }
    t.speicherPfad = ziel;
    t.gespeichertStand = t.dok.stand();
    std::string m = format(Str::MbSpGespeichert, ziel.c_str(), static_cast<int>(bytes.size()), br.flaechenRoh, br.flaechenNeu);
    if (br.umgelegt) { m += " " + format(Str::MbSpUmgelegt, br.knochen); }
    melden(m);
    diag::detail("Model-Tab: .glm geschrieben " + ziel + ", " + std::to_string(bytes.size()) + " Bytes, roh " + std::to_string(br.flaechenRoh) +
                 ", neu " + std::to_string(br.flaechenNeu) + " (" + std::to_string(br.eckenNeu) + " Ecken)");
    return true;
}

}  // namespace

namespace {
void cfgAbgleich();
}  // namespace

void modellSpeichern(bool unter) {
    ModellTab& t = mt();
    const bool md3 = t.dok.inhalt().herkunft == modell::Herkunft::Md3 && t.dok.inhalt().md3Datei;   // Modell-Block 1b
    if (t.dok.inhalt().herkunft != modell::Herkunft::Glm && !md3) {
        melden(tr(Str::MbSpNichtGlm));
        return;
    }
    std::string ziel;
    if (!unter && !t.speicherPfad.empty()) {
        ziel = t.speicherPfad;
    } else if (!unter && aufPlatte(g_app->modelPath)) {
        ziel = g_app->modelPath;
    } else {
        const std::string endung = md3 ? ".md3" : ".glm";
        ziel = platform::saveFileDialog(tr(Str::MbDateiSpeichernUnter), md3 ? "MD3 models (*.md3)|*.md3" : "Ghoul2 models (*.glm)|*.glm", g_app->settings.lastDir,
                                        fileName(g_app->modelPath.empty() ? "model" + endung : g_app->modelPath));
        if (ziel.empty()) { return; }
        if (kleinMb3(ziel).size() < 4 || kleinMb3(ziel).compare(ziel.size() - 4, 4, endung) != 0) { ziel += endung; }
    }
    // Vor dem Schreiben die Grenzen des Zielspiels (g2.h): darueber fragt das Programm, Speichern bleibt moeglich. Die
    // Animationsliste erst nach dem Dokument (nach Rueckgaengig ohne gezeichnetes Bild nannte die Frage eine alte Zeile:
    // Handbuchfoto mb3_hb, "ROOT endet bei 60871" nach dem Zuruecknehmen).
    cfgAbgleich();
    t.frageZeilen.clear();
    for (const modell::Befund& b : modellBefunde()) {
        if (b.art != modell::BefundArt::ShaderFehlt) { t.frageZeilen.push_back(befundText(b)); }
    }
    if (!t.frageZeilen.empty()) {
        t.frageArt = 1;
        t.frageZiel = ziel;
        return;
    }
    (void)schreibenNach(ziel);
}

bool modellUngespeichert() {
    const ModellTab& t = mt();
    // P1: auch bemalte, nicht gebaute Bilder (Beenden und anderes Modell fragen dann wie beim Dokument)
    return (t.gespeichertStand >= 0 ? t.dok.stand() != t.gespeichertStand : t.dok.kannRueckgaengig()) || modellMalenUngespeichert();
}

namespace {
std::string modellFrageName() {
    const ModellTab& t = mt();
    if (!t.speicherPfad.empty()) { return fileName(t.speicherPfad); }
    return g_app->modelPath.empty() ? t.dok.inhalt().name : fileName(g_app->modelPath);
}
}  // namespace

// pfix H3: wie missionBeendenFrage (Ja: weiter und verwerfen, Nein: zurueck) - vor dem Laden eines anderen Modells
void modellVerwerfenDann(std::function<void()> danach) {
    if (!modellUngespeichert()) {
        danach();
        return;
    }
    if (g_app->frageOffen) { return; }
    g_app->leftMode = 2;   // man soll sehen, worueber man entscheidet
    frage(format(Str::MbVerwerfenFrage, modellFrageName().c_str()), std::move(danach), {});
}

// pfix H3: beim Beenden (confirmQuit) - eine spaeter abgebrochene Frage vergisst die Antwort (beendenAbgebrochen)
bool modellBeendenFrage() {
    if (mt().beendenBeantwortet || !modellUngespeichert()) { return true; }
    if (g_app->frageOffen) { return false; }
    g_app->leftMode = 2;
    frage(format(Str::MbBeendenFrage, modellFrageName().c_str()),
          [] {
              mt().beendenBeantwortet = true;
              if (confirmQuit()) {
                  g_app->quitConfirmed = true;
                  g_app->wantQuit = true;
              }
          },
          [] { beendenAbgebrochen(); });
    return false;
}

void modellBeendenVergessen() { mt().beendenBeantwortet = false; }

// --- Bereich Animation ---------------------------------------------------------------------------------------------
namespace {

// Die animation.cfg der aktuellen GLA ins Dokument (ohne Rueckgaengig-Schritt), wenn eine andere GLA da ist; nach
// Rueckgaengig/Wiederholen die Animationsliste aus den Zeilen des Dokuments.
void cfgAbgleich() {
    ModellTab& t = mt();
    const std::string fuer = g_app->anim.skeletonName + "|" + std::to_string(g_app->anim.numFrames) + "|" + t.dokModell;
    if (fuer != t.cfgFuer) {
        t.cfgFuer = fuer;
        std::string pfad;
        std::string text;
        for (const std::string& k : animCfgCandidates(g_app->anim.skeletonName)) {
            if (readFromArchives(k, text)) {
                pfad = k;
                break;
            }
        }
        t.dok.vorschau([&](modell::Inhalt& in) {
            in.cfgPfad = pfad;
            in.cfg = pfad.empty() ? modell::AnimCfg{} : modell::animCfgLesen(text);
        });
        t.cfgStand = t.dok.stand();
        return;
    }
    if (t.cfgStand == t.dok.stand()) { return; }
    t.cfgStand = t.dok.stand();
    const modell::Inhalt& in = t.dok.inhalt();
    if (in.cfgPfad.empty()) { return; }
    const std::string name = g_app->animIndex >= 0 && static_cast<std::size_t>(g_app->animIndex) < g_app->animList.size()
                                 ? g_app->animList[static_cast<std::size_t>(g_app->animIndex)].name
                                 : std::string();
    g_app->animList = in.cfg.eintraege();
    g_app->animIndex = -1;
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        if (g_app->animList[i].name == name) { g_app->animIndex = static_cast<int>(i); }
    }
    g_app->modelDirty = true;
}

// Die GLA, aus der ein Platz seine Frames nimmt, als feste Kopie (die Plaetze zeigen auf sie).
const GlaAnimation* quelleFuer(const GlaAnimation& a, const std::string& name) {
    ModellTab& t = mt();
    for (const ModellGlaQuelle& q : t.kgQuellen) {
        if (q.name == name && q.gla.numFrames == a.numFrames && q.gla.bones.size() == a.bones.size()) { return &q.gla; }
    }
    t.kgQuellen.push_back(ModellGlaQuelle{name, a});
    return &t.kgQuellen.back().gla;
}

// Nachtrag shank 06.11.: das Ziel waehlt man erst beim Bauen und kann es jederzeit wechseln. Die Belegung (Platzname ->
// Quelle) gilt fuer alle Ziele; die Platzliste kommt aus dem Ziel: Karten-GLA BOTH_CIN_1..n, _humanoid.gla dieselben und
// alle Zeilen ihrer animation.cfg, eigene GLA die belegten Namen.
void plaetzeAnpassen() {
    ModellTab& t = mt();
    const std::string schluessel = std::to_string(t.glaZiel) + "|" + std::to_string(t.zielspiel) + "|" + std::to_string(t.kgStand) + "|" +
                                   std::to_string(t.dok.stand()) + "|" + g_app->anim.skeletonName;
    if (schluessel == t.plaetzeFuer) { return; }
    t.plaetzeFuer = schluessel;
    std::vector<modell::Platz> neu;
    if (t.glaZiel != 2) { neu = modell::kinoPlaetze(zielspiel(), {}); }
    if (t.glaZiel == 1) {
        for (const AnimEntry& e : t.dok.inhalt().cfg.eintraege()) {
            if (std::none_of(neu.begin(), neu.end(), [&](const modell::Platz& p) { return kleinMb3(p.name) == kleinMb3(e.name); })) {
                neu.push_back(modell::Platz{e.name, false, {}});
            }
        }
    }
    if (t.glaZiel == 2) {
        for (const auto& [name, q] : t.dok.inhalt().kinoBelegung) { neu.push_back(modell::Platz{name, false, {}}); }
    }
    for (modell::Platz& p : neu) {
        const auto it = t.dok.inhalt().kinoBelegung.find(p.name);
        if (it != t.dok.inhalt().kinoBelegung.end()) {
            p.belegt = true;
            p.quelle = it->second;
        }
    }
    t.plaetze = std::move(neu);
}

// REST N10: die Belegung aendert sich ueber das Dokument (ein Rueckgaengig-Schritt je Aenderung)
void belegen(Str name, const std::function<void(std::map<std::string, modell::PlatzQuelle>&)>& f) {
    ModellTab& t = mt();
    (void)t.dok.aendern(tr(name), [&](modell::Inhalt& x) { f(x.kinoBelegung); }, false);
    ++t.kgStand;
    plaetzeAnpassen();
}

std::string kartenPfad() {
    const ModellTab& t = mt();
    return modell::kartenGlaPfad("_humanoid", t.kgKarte[0] != '\0' ? t.kgKarte : "karte");
}

// Der Pfad im Kopf der Ziel-GLA (ohne Endung): Karten-GLA, die GLA des Modells, eigenes Skelett models/players/<n>/<n>.
std::string zielKopf() {
    const ModellTab& t = mt();
    if (t.glaZiel == 1) { return g_app->anim.skeletonName; }
    if (t.glaZiel == 2) {
        const std::string n = t.eigenName[0] != '\0' ? t.eigenName : "_eigen";
        return "models/players/" + n + "/" + n;
    }
    return kartenPfad();
}

modell::PlatzQuelle quelleVon(int anim) {
    const std::string name = g_app->anim.skeletonName;
    return modell::PlatzQuelle{quelleFuer(g_app->anim, name), name, g_app->animList[static_cast<std::size_t>(anim)]};
}

}  // namespace

// REST N6: ein Fehler der Modell-Bibliothek in der Sprache der Oberflaeche (bhed/modell/fehler.h). Ohne Art der englische
// Satz zurueck zur Art (fehlerAusText); ein fremder Text (Datei-Schreiber, sicherSchreiben) bleibt, wie er ist.
std::string modellFehlerText(const modell::ModellFehler& f) {
    using A = modell::FehlerArt;
    const char* n = f.name.c_str();
    const int a = static_cast<int>(f.a);
    const int b = static_cast<int>(f.b);
    switch (f.art) {
        case A::Keiner: return {};
        case A::KeinMd3: return tr(Str::MbFeKeinMd3);
        case A::Md3PasstNicht: return tr(Str::MbFeMd3PasstNicht);
        case A::Md3HinterFlaechen: return tr(Str::MbFeMd3HinterFlaechen);
        case A::FlaecheAbgeschnitten: return format(Str::MbFeFlaecheAbgeschnitten, a);
        case A::FlaecheUnbrauchbar: return format(Str::MbFeFlaecheUnbrauchbar, a);
        case A::FlaecheOhneNetz: return format(Str::MbFeFlaecheOhneNetz, a);
        case A::ZuVieleEcken: return format(Str::MbFeZuVieleEcken, n, a, b);
        case A::ZuVieleDreiecke: return format(Str::MbFeZuVieleDreiecke, n, a, b);
        case A::KeinGlm: return tr(Str::MbFeKeinGlm);
        case A::GlaPfadZuLang: return tr(Str::MbFeGlaPfadZuLang);
        case A::TeilNameZuLang: return format(Str::MbFeTeilNameZuLang, n);
        case A::OhneLods: return format(Str::MbFeOhneLods, n);
        case A::ZuVieleKnochen: return format(Str::MbFeZuVieleKnochen, n, a, b);
        case A::QuelleOhneBecken: return tr(Str::MbFeQuelleOhneBecken);
        case A::ZielOhneBecken: return tr(Str::MbFeZielOhneBecken);
        case A::BilderAusserhalb: return tr(Str::MbFeBilderAusserhalb);
        case A::NichtsZuBacken: return tr(Str::MbFeNichtsZuBacken);
        case A::FremdesSkelett: return tr(Str::MbFeFremdesSkelett);
        case A::VorratZuGross: return tr(Str::MbFeVorratZuGross);
        case A::OhneBeckenBeine: return tr(Str::MbFeOhneBeckenBeine);
        case A::KeinPlatzBelegt: return tr(Str::MbFeKeinPlatzBelegt);
        case A::AnderesSkelett: return tr(Str::MbFeAnderesSkelett);
        case A::KnochenAnders: return format(Str::MbFeKnochenAnders, n);
        case A::KeinSkelett: return tr(Str::MbFeKeinSkelett);
        case A::GrundanimFehlt: return format(Str::MbFeGrundanimFehlt, n);
        case A::GrundbildFehlt: return tr(Str::MbFeGrundbildFehlt);
        case A::VerweisHinterVorrat: return tr(Str::MbFeVerweisHinterVorrat);
        case A::KeinKinoPlatz: return tr(Str::MbFeKeinKinoPlatz);
        case A::PlaetzeFiguren: return tr(Str::MbFePlaetzeFiguren);
        case A::FigurOhneGrund: return tr(Str::MbFeFigurOhneGrund);
        case A::Fehlt: return format(Str::MbFeFehlt, n);
        case A::NichtUnterBecken: return format(Str::MbFeNichtUnterBecken, n);
        case A::QuellKnochenFehlt: return format(Str::MbFeQuellKnochenFehlt, n, f.name2.c_str());
        case A::QuellGrundstellung: return format(Str::MbFeQuellGrundstellung, n, f.name2.c_str());
        case A::SequenzNichtInQuelle: return format(Str::MbFeSequenzNichtInQuelle, n, a, b, f.name2.c_str(), static_cast<int>(f.c));
        case A::QuellVerweis: return format(Str::MbFeQuellVerweis, n);
        case A::BereichAusserhalb: return tr(Str::MbFeBereichAusserhalb);
        case A::KeineAnimation: return tr(Str::MbFeKeineAnimation);
        case A::AchsenUnbestimmt: return format(Str::MbFeAchsenUnbestimmt, n, f.name2.c_str());
        case A::AnimZuLang: return format(Str::MbFeAnimZuLang, static_cast<double>(f.a), static_cast<double>(f.b), static_cast<double>(f.c));
        case A::AnimZuLangGla: return format(Str::MbFeAnimZuLangGla, static_cast<double>(f.a), static_cast<double>(f.b), static_cast<double>(f.c));
        case A::BvhOhneSeiten: return tr(Str::MbHwBvhOhneSeiten);
        case A::BvhBoden: return format(Str::MbHwBvhBoden, static_cast<double>(f.a) / 10.0);
        case A::UngleicheSkalierung: return tr(Str::MbHwUngleicheSkalierung);
        case A::Anzahl: break;
    }
    return modell::fehlerText(f);
}

std::string modellFehlerText(const std::string& englisch) {
    modell::ModellFehler f;
    return modell::fehlerAusText(englisch, f) ? modellFehlerText(f) : englisch;
}

// Ein Ort im Spielordner? (_humanoid.gla gehoert in einen Mod-Ordner, nie ins Spielverzeichnis: es ersetzte das Skelett
// aller Figuren auf jeder Karte.) Verglichen wird mit den Spielordnern der Einstellungen und deren Elternordner.
bool modellImSpielordner(const std::string& datei) {
    std::error_code ec;
    const std::string d = kleinMb3(std::filesystem::weakly_canonical(std::filesystem::path(datei), ec).generic_string());
    for (const GamePath& gp : g_app->gamePaths) {
        std::filesystem::path wurzel = std::filesystem::weakly_canonical(std::filesystem::path(gp.directory), ec);
        // ...\GameData\base und ...\GameData\MD: der Spielordner ist der darueber
        if (wurzel.has_parent_path()) { wurzel = wurzel.parent_path(); }
        const std::string w = kleinMb3(wurzel.generic_string());
        if (!w.empty() && d.compare(0, w.size(), w) == 0) { return true; }
    }
    return false;
}

bool modellPlatzSetzen(int platz, int anim) {
    ModellTab& t = mt();
    plaetzeAnpassen();
    if (platz < 0 || static_cast<std::size_t>(platz) >= t.plaetze.size() || anim < 0 || static_cast<std::size_t>(anim) >= g_app->animList.size() ||
        g_app->anim.empty()) {
        return false;
    }
    const std::string name = t.plaetze[static_cast<std::size_t>(platz)].name;
    const modell::PlatzQuelle q = quelleVon(anim);
    belegen(Str::MbKgHierher, [&](std::map<std::string, modell::PlatzQuelle>& b) { b[name] = q; });
    diag::detail("Model-Tab: Platz " + name + " <- " + g_app->animList[static_cast<std::size_t>(anim)].name + " (Ziel " + std::to_string(t.glaZiel) + ")");
    return true;
}

int modellAufFreienPlatz(int anim) {
    ModellTab& t = mt();
    plaetzeAnpassen();
    if (anim < 0 || static_cast<std::size_t>(anim) >= g_app->animList.size() || g_app->anim.empty()) { return -1; }
    if (t.glaZiel == 2) {
        // eigene GLA: der Platz heisst wie die Sequenz
        const modell::PlatzQuelle q = quelleVon(anim);
        belegen(Str::MbKgAufFreien, [&](std::map<std::string, modell::PlatzQuelle>& b) { b[g_app->animList[static_cast<std::size_t>(anim)].name] = q; });
        return static_cast<int>(t.plaetze.size()) - 1;
    }
    for (std::size_t i = 0; i < t.plaetze.size(); ++i) {
        if (!t.plaetze[i].belegt) { return modellPlatzSetzen(static_cast<int>(i), anim) ? static_cast<int>(i) : -1; }
    }
    return -1;
}

void modellPlatzLeeren(int platz) {
    ModellTab& t = mt();
    if (platz < 0 || static_cast<std::size_t>(platz) >= t.plaetze.size()) { return; }
    const std::string name = t.plaetze[static_cast<std::size_t>(platz)].name;
    belegen(Str::MbKgLeeren, [&](std::map<std::string, modell::PlatzQuelle>& b) { b.erase(name); });
}

void modellGlaZielSetzen(int ziel) {
    ModellTab& t = mt();
    t.glaZiel = std::clamp(ziel, 0, 2);
    plaetzeAnpassen();
}

namespace {
// Die vorhandene Karten-GLA aus den Spieldaten: je Platz BOTH_CIN_n ihre Zeile (Quelle: sie selbst). belegtLassen: nur
// Plaetze nehmen, die noch frei sind (Bauen fuehrt zusammen, statt zu ueberschreiben).
int vorhandeneUebernehmen(bool belegtLassen) {
    const std::string pfad = kartenPfad();
    std::string bytes;
    GlaAnimation a;
    if (!readFromArchives(pfad + ".gla", bytes) || !readGla(bytes, a)) { return -1; }
    std::string text;
    std::vector<AnimEntry> zeilen;
    for (const std::string& k : animCfgCandidates(pfad)) {
        if (readFromArchives(k, text)) {
            zeilen = parseAnimationCfg(text);
            break;
        }
    }
    const GlaAnimation* q = quelleFuer(a, pfad);
    int n = 0;
    belegen(Str::MbKgVorhanden, [&](std::map<std::string, modell::PlatzQuelle>& b) {
        for (const modell::Platz& p : modell::kinoPlaetze(zielspiel(), {})) {
            for (const AnimEntry& e : zeilen) {
                if (kleinMb3(e.name) != kleinMb3(p.name) || (belegtLassen && b.count(p.name) != 0U)) { continue; }
                b[p.name] = modell::PlatzQuelle{q, pfad, e};
                ++n;
            }
        }
    });
    return n;
}
}  // namespace

bool modellVorhandeneKartenGla() {
    const int n = vorhandeneUebernehmen(false);
    if (n < 0) {
        melden(format(Str::MbKgNichtGefunden, (kartenPfad() + ".gla").c_str()));
        return false;
    }
    diag::detail("Model-Tab: vorhandene Karten-GLA " + kartenPfad() + ", " + std::to_string(n) + " Plaetze");
    return true;
}

bool modellKartenGlaBauen(const std::string& datei, bool trotzdem) {
    ModellTab& t = mt();
    if (t.glaZiel == 0) { (void)vorhandeneUebernehmen(true); }   // eine vorhandene Karten-GLA wird zusammengefuehrt
    plaetzeAnpassen();
    if (std::none_of(t.plaetze.begin(), t.plaetze.end(), [](const modell::Platz& p) { return p.belegt; })) {
        melden(tr(Str::MbKgNichts));
        return false;
    }
    if (t.glaZiel == 1 && modellImSpielordner(datei)) {
        melden(tr(Str::MbKgImSpielordner));
        return false;
    }
    GlaAnimation k;
    std::vector<AnimEntry> zeilen;
    modell::ZusammenBericht br;
    const std::string kopf = zielKopf();
    const bool ok = t.glaZiel == 1 ? modell::anhaengen(g_app->anim, t.plaetze, k, zeilen, &br) : modell::zusammenfuehren(t.plaetze, kopf, k, zeilen, &br);
    if (!ok) {
        melden(format(Str::MbSpFehler, modellFehlerText(br.fehlerArt).c_str()));
        return false;
    }
    // animation.cfg: fuer die GLA des Modells ihre Zeilen mit den geaenderten (unveraenderte Byte fuer Byte), sonst neu.
    modell::AnimCfg cfg;
    if (t.glaZiel == 1) {
        cfg = t.dok.inhalt().cfg;
    } else {
        cfg.zeilen.push_back(modell::CfgZeile{"// " + kopf + ".gla " + std::to_string(k.numFrames) + " frames; " + std::to_string(zeilen.size()) +
                                                  " sequences; written by JK Studio\r",
                                              false, {}, false});
        cfg.zeilen.push_back(modell::CfgZeile{"//\r", false, {}, false});
        cfg.zeilen.push_back(modell::CfgZeile{"// Format:  enum, targetFrame, frameCount, loopFrame, frameSpeed\r", false, {}, false});
        cfg.zeilen.push_back(modell::CfgZeile{"//\r", false, {}, false});
    }
    for (const AnimEntry& e : zeilen) { cfg.setzen(e); }
    const std::string cfgText = modell::animCfgSchreiben(cfg);
    if (!trotzdem) {
        t.frageZeilen.clear();
        std::vector<modell::CfgBefund> b;
        if (t.glaZiel == 0) { b = modell::kartenGlaPruefen(k, kopf, zielspiel()); }
        if (t.glaZiel == 1) { b = modell::humanoidGlaPruefen(zielspiel()); }
        if (t.glaZiel == 2) {
            b = modell::eigeneGlaPruefen(k, kopf, zielspiel());
            // ein Satz mehr neben _humanoid (G_ParseAnimFileSet)
            for (const modell::CfgBefund& x : modell::animDateienPruefen(2, zielspiel())) { b.push_back(x); }
        }
        for (const modell::CfgBefund& x : modell::cfgPruefen(cfg.eintraege(), k.numFrames, cfgText.size(), zielspiel())) { b.push_back(x); }
        for (const modell::CfgBefund& x : b) { t.frageZeilen.push_back(cfgBefundText(x)); }
        if (!t.frageZeilen.empty()) {
            t.frageArt = 2;
            t.frageZiel = datei;
            return false;
        }
    }
    std::string bytes;
    std::string fehler;
    if (!schreibGla(k, bytes, &fehler) || !sicherSchreiben(datei, bytes, &fehler) ||
        !sicherSchreiben((std::filesystem::path(datei).parent_path() / "animation.cfg").string(), cfgText, &fehler)) {
        melden(format(Str::MbSpFehler, fehler.c_str()));
        return false;
    }
    // eigene GLA: die .glm nennt sie (mdxmHeader_t animName) - beim naechsten Speichern des Modells
    if (t.glaZiel == 2 && t.dok.inhalt().glaPfad != kopf) {
        t.dok.aendern(kopf, [&](modell::Inhalt& in) { in.glaPfad = kopf; });
    }
    int belegt = 0;
    for (const modell::Platz& p : t.plaetze) { belegt += p.belegt ? 1 : 0; }
    melden(format(Str::MbKgGebaut, datei.c_str(), k.numFrames, br.vorrat, belegt));
    return true;
}

// Eine dotXSI-Animation gegen das Skelett der GLA aufnehmen (Carcass-Weg, G2C) und auf den naechsten freien Platz legen.
bool modellXsiAufnehmen(const std::string& datei) {
    plaetzeAnpassen();
    std::string text;
    if (g_app->anim.empty() || !readFromArchives(datei, text)) {
        text = slurp(datei);
    }
    GlaAnimation g;
    modell::XsiBericht br;
    const std::string name = fileName(datei);
    if (g_app->anim.empty() || !modell::xsiAufnehmen(text, g_app->anim, zielKopf(), g, &br)) {
        melden(format(Str::MbSpFehler, br.fehler.empty() ? tr(Str::MbCfgKeine) : modellFehlerText(br.fehler).c_str()));
        return false;
    }
    const int bilder = g.numFrames;
    const std::string platz = modellGlaAufPlatz(std::move(g), name, -1, 20);
    if (platz.empty()) { return false; }
    melden(format(Str::MbXsiAufgenommen, name.c_str(), bilder, platz.c_str(), static_cast<int>(br.fehlendeKnochen.size())));
    return true;
}

// Eine neue Quelle (XSI aufgenommen, Rig gebacken) auf den naechsten freien Platz des Ziels: die GLA als feste Kopie,
// die Zeile mit allen Bildern, loopFrames und fps. Leer: kein Platz frei (gemeldet).
std::string modellGlaAufPlatz(GlaAnimation g, const std::string& name, int loopFrames, int fps, const std::string& wunsch) {
    ModellTab& t = mt();
    plaetzeAnpassen();
    t.kgQuellen.push_back(ModellGlaQuelle{name, std::move(g)});
    const GlaAnimation* q = &t.kgQuellen.back().gla;
    const modell::PlatzQuelle pq{q, name, AnimEntry{name, 0, q->numFrames, loopFrames, fps}};
    // Stufe 5: ein gewuenschter Platz (BOTH_WALK1 der _humanoid.gla, ein Name der eigenen GLA), auch wenn er belegt ist
    if (!wunsch.empty()) {
        const bool da = t.glaZiel == 2 ||
                        std::any_of(t.plaetze.begin(), t.plaetze.end(), [&](const modell::Platz& p) { return kleinMb3(p.name) == kleinMb3(wunsch); });
        if (da) {
            std::string platz = wunsch;
            for (const modell::Platz& p : t.plaetze) {
                if (kleinMb3(p.name) == kleinMb3(wunsch)) { platz = p.name; }
            }
            belegen(Str::MbKgHierher, [&](std::map<std::string, modell::PlatzQuelle>& b) { b[platz] = pq; });
            return platz;
        }
    }
    for (const modell::Platz& p : t.plaetze) {
        if (p.belegt && t.glaZiel != 2) { continue; }
        const std::string platz = t.glaZiel == 2 ? name : p.name;
        belegen(Str::MbKgAufFreien, [&](std::map<std::string, modell::PlatzQuelle>& b) { b[platz] = pq; });
        return platz;
    }
    melden(format(Str::MbKgKeinePlaetze, modell::grenzen(zielspiel()).name));
    return {};
}

std::string modellZielKopf() { return zielKopf(); }

void modellXsiAufnehmenFrage() {
    const std::string p = platform::openFileDialog(tr(Str::MbXsiAufnehmen), "dotXSI (*.xsi)|*.xsi", g_app->settings.lastDir);
    if (!p.empty()) { (void)modellXsiAufnehmen(p); }
}

bool modellGlaSpeichern(const std::string& datei) {
    std::string bytes;
    std::string fehler;
    if (!schreibGla(g_app->anim, bytes, &fehler) || !sicherSchreiben(datei, bytes, &fehler)) {
        melden(format(Str::MbSpFehler, fehler.c_str()));
        return false;
    }
    melden(format(Str::MbGlaGespeichert, datei.c_str(), static_cast<int>(bytes.size())));
    return true;
}

bool modellCfgSpeichern(const std::string& datei) {
    const std::string text = modell::animCfgSchreiben(mt().dok.inhalt().cfg);
    std::string fehler;
    if (mt().dok.inhalt().cfgPfad.empty() || !sicherSchreiben(datei, text, &fehler)) {
        melden(format(Str::MbSpFehler, fehler.empty() ? tr(Str::MbCfgKeine) : fehler.c_str()));
        return false;
    }
    melden(format(Str::MbGlaGespeichert, datei.c_str(), static_cast<int>(text.size())));
    return true;
}

void modellGlaSpeichernFrage() {
    const std::string p = platform::saveFileDialog(tr(Str::MbGlaSpeichern), "Ghoul2 animation (*.gla)|*.gla", g_app->settings.lastDir,
                                                   fileName(g_app->anim.skeletonName) + ".gla");
    if (!p.empty()) { (void)modellGlaSpeichern(p); }
}

void modellCfgSpeichernFrage() {
    const std::string p = platform::saveFileDialog(tr(Str::MbCfgSpeichern), "animation.cfg (*.cfg)|*.cfg", g_app->settings.lastDir, "animation.cfg");
    if (!p.empty()) { (void)modellCfgSpeichern(p); }
}

void modellKartenGlaBauenFrage() {
    const std::string n = fileName(zielKopf());
    const std::string p = platform::saveFileDialog(tr(Str::MbKgBauen), "Ghoul2 animation (*.gla)|*.gla", g_app->settings.lastDir, n + ".gla");
    if (!p.empty()) { (void)modellKartenGlaBauen(p, false); }
}

// Die Frage im Programm (kein Windows-Dialog): Befunde, Trotzdem/Abbrechen.
void drawModellSpeichernFrage() {
    ModellTab& t = mt();
    const char* id = "###mbfrage";
    if (t.frageArt != 0 && !ImGui::IsPopupOpen(id)) { ImGui::OpenPopup(id); }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 34.0F, 0.0F}, ImGuiCond_Appearing);
    if (!ui::modalBegin((std::string(tr(Str::MbSpFrageTitel)) + id).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        t.frageSichtbar = false;
        return;
    }
    t.frageSichtbar = true;
    ImGui::PushTextWrapPos(fs * 33.0F);
    cjkTextUnformatted(format(Str::MbSpFrageText, modell::grenzen(zielspiel()).name).c_str());
    const ImVec4 warn = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
    for (std::size_t i = 0; i < t.frageZeilen.size() && i < 12U; ++i) { cjkTextColored(warn, "- %s", t.frageZeilen[i].c_str()); }
    if (t.frageZeilen.size() > 12U) { cjkTextDisabled("... %d", static_cast<int>(t.frageZeilen.size() - 12U)); }
    cjkTextDisabled("%s", t.frageZiel.c_str());
    ImGui::PopTextWrapPos();
    if (ImGui::Button(tr(Str::MbSpTrotzdem))) {
        const int art = t.frageArt;
        t.frageArt = 0;
        ImGui::CloseCurrentPopup();
        if (art == 1) { (void)schreibenNach(t.frageZiel); }
        if (art == 2) { (void)modellKartenGlaBauen(t.frageZiel, true); }
    }
    ImGui::SetItemTooltip("%s", tr(Str::MbSpTrotzdemHint));
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EditorCancel)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        t.frageArt = 0;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// Eine Zeile der Animationsliste als Quelle zum Ziehen auf einen Platz.
void modellSequenzZiehen(int anim) {
    if (anim < 0 || static_cast<std::size_t>(anim) >= g_app->animList.size()) { return; }
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("MB_SEQ", &anim, sizeof anim);
        cjkTextUnformatted(g_app->animList[static_cast<std::size_t>(anim)].name.c_str());
        mt().ziehtSequenz = anim;
        ImGui::EndDragDropSource();
    }
}

// --- Befehlsfeld (AP10: ueber den Baustein gui/ui_rollout.h) ---------------------------------------------------------------

// Entwurf A1: "Animation" - die Sequenz, an der gearbeitet wird, die Ziel-GLA und ihre Plaetze (belegt / alle) als Tabelle.
// Die ausfuehrliche Platzliste steht weiter unten in "Ziel-GLA und Plaetze".
void modellAnimGruppeAnimation() {
    ModellTab& t = mt();
    cfgAbgleich();
    plaetzeAnpassen();
    const AnimEntry* e = g_app->animIndex >= 0 && static_cast<std::size_t>(g_app->animIndex) < g_app->animList.size() ? &g_app->animList[static_cast<std::size_t>(g_app->animIndex)]
                                                                                                                      : nullptr;
    if (!ui::rolloutBeginn(tr(Str::AzRAnimation), {.kurzinfo = e != nullptr ? e->name.c_str() : nullptr})) { return; }
    int belegt = 0;
    for (const modell::Platz& p : t.plaetze) { belegt += p.belegt ? 1 : 0; }
    char plaetze[48];
    std::snprintf(plaetze, sizeof plaetze, "%d / %d", belegt, static_cast<int>(t.plaetze.size()));
    // eine volle Karten-GLA nimmt nichts mehr auf; die eigene GLA waechst mit jeder Sequenz
    const bool voll = t.glaZiel == 0 && !t.plaetze.empty() && belegt == static_cast<int>(t.plaetze.size());
    ui::wertTabelle({{tr(Str::AzName), e != nullptr ? e->name : std::string(tr(Str::AzKeineSequenz)), e != nullptr ? ui::Ton::Normal : ui::Ton::Leise, {}},
                     {tr(Str::AzZielGla), fileName(zielKopf()), ui::Ton::Normal, zielKopf() + ".gla"},
                     {tr(Str::AzPlaetze), plaetze, voll ? ui::Ton::Warnung : ui::Ton::Normal, tr(Str::AzPlaetzeHint)}});
    ui::rolloutEnde();
}

// Entwurf A9: "Fuer das Spiel bauen" - der Wurzelweg der Sequenz, der Hauptknopf (baut die Ziel-GLA mit ihrer animation.cfg) und
// der Hinweis auf SET_ORIGIN (Stufenplan: das Spiel verschiebt den Ursprung nicht mit der Wurzel).
void modellAnimGruppeBauen() {
    if (!ui::rolloutBeginn(tr(Str::AzRBauen), {.reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Objekt)})) { return; }
    float weg = 0.0F;
    const bool da = modellAnimWurzelweg(weg);
    ui::wertTabelle({{tr(Str::AzWurzelweg), da ? format(Str::AzEinheiten, static_cast<double>(weg)) : std::string("-"), da ? ui::Ton::Normal : ui::Ton::Leise, tr(Str::AzWurzelwegHint)},
                     {tr(Str::AzZielGla), fileName(zielKopf()), ui::Ton::Normal, zielKopf() + ".gla"}});
    if (ui::werkzeugKnopf(tr(Str::MbKgBauen), {.icon = UiIcon::Paket, .hinweis = tr(Str::MbKgBauenHint), .haupt = true, .aktiv = !g_app->anim.empty()})) { modellKartenGlaBauenFrage(); }
    ui::tipp(tr(Str::AzBauenTipp));
    ui::rolloutEnde();
}

namespace {

void gruppeGla() {
    const GlaAnimation& a = g_app->anim;
    if (!ui::rolloutBeginn(tr(Str::MbRGla), {.reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Daten)})) { return; }
    ui::absatz(format(Str::MbGlaInfo, a.skeletonName.empty() ? "-" : a.skeletonName.c_str(), a.numFrames, static_cast<int>(a.bones.size()), static_cast<int>(a.bonePool.size() / 14U)).c_str());
    if (ui::werkzeugKnopf(tr(Str::MbGlaSpeichern), {.icon = UiIcon::SpeichernUnter, .hinweis = tr(Str::MbGlaSpeichernHint), .aktiv = !a.empty()})) { modellGlaSpeichernFrage(); }
    ui::rolloutEnde();
}

void gruppeCfg() {
    ModellTab& t = mt();
    const GlaAnimation& a = g_app->anim;
    if (!ui::rolloutBeginn(tr(Str::MbRCfg), {.reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Daten)})) { return; }
    const modell::Inhalt& in = t.dok.inhalt();
    if (in.cfgPfad.empty()) {
        ui::absatz(tr(Str::MbCfgKeine));
    } else {
        const std::string text = modell::animCfgSchreiben(in.cfg);
        const std::vector<AnimEntry> e = in.cfg.eintraege();
        ui::absatz(format(Str::MbCfgInfo, in.cfgPfad.c_str(), static_cast<int>(e.size()), static_cast<int>(text.size())).c_str());
        const std::vector<modell::CfgBefund> b = modell::cfgPruefen(e, a.numFrames, text.size(), zielspiel());
        t.cfgBefunde = static_cast<int>(b.size());
        if (b.empty()) { ui::absatz(format(Str::MbCfgOk, modell::grenzen(zielspiel()).name).c_str(), ui::Ton::Gut); }
        for (std::size_t i = 0; i < b.size() && i < 40U; ++i) { ui::absatz(cfgBefundText(b[i]).c_str(), ui::Ton::Warnung); }
        if (b.size() > 40U) { ui::absatz(("... " + std::to_string(b.size() - 40U)).c_str()); }
    }
    if (ui::werkzeugKnopf(tr(Str::MbCfgSpeichern), {.icon = UiIcon::SpeichernUnter, .hinweis = tr(Str::MbCfgSpeichernHint), .aktiv = !in.cfgPfad.empty()})) { modellCfgSpeichernFrage(); }
    ui::rolloutEnde();
}

void gruppePlaetze() {
    ModellTab& t = mt();
    const float fs = ImGui::GetFontSize();
    int belegt = 0;
    for (const modell::Platz& p : t.plaetze) { belegt += p.belegt ? 1 : 0; }
    char info[48];
    std::snprintf(info, sizeof info, "%d / %d", belegt, static_cast<int>(t.plaetze.size()));
    if (!ui::rolloutBeginn(tr(Str::MbRKartenGla), {.kurzinfo = t.plaetze.empty() ? nullptr : info, .reiter = modellReiter(MbReiter::Dienste, MbReiterBl::Daten)})) { return; }
    // Nachtrag shank 06.11.: das Ziel erst beim Bauen waehlen, jederzeit wechselbar - ein klares Feld mit Erklaerung.
    static const Str kZiele[3] = {Str::MbZielKarte, Str::MbZielHumanoid, Str::MbZielEigen};
    static const Str kZielHinweise[3] = {Str::MbZielKarteHint, Str::MbZielHumanoidHint, Str::MbZielEigenHint};
    ui::zeile(tr(Str::MbZiel));
    if (ImGui::BeginCombo("##glaziel", tr(kZiele[t.glaZiel]))) {
        for (int z = 0; z < 3; ++z) {
            if (ImGui::Selectable(tr(kZiele[z]), z == t.glaZiel)) { modellGlaZielSetzen(z); }
            ImGui::SetItemTooltip("%s", tr(kZielHinweise[z]));
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", tr(kZielHinweise[t.glaZiel]));
    ui::absatz(tr(kZielHinweise[t.glaZiel]));
    if (t.glaZiel == 0) {
        ui::zeile(tr(Str::MbKgKarte));
        ImGui::InputText("##kgkarte", t.kgKarte, sizeof t.kgKarte);
        ImGui::SetItemTooltip("%s", tr(Str::MbKgKarteHint));
    }
    if (t.glaZiel == 2) {
        ui::zeile(tr(Str::MbZielEigenName));
        ImGui::InputText("##kgeigen", t.eigenName, sizeof t.eigenName);
        ImGui::SetItemTooltip("%s", tr(Str::MbZielEigenHint));
    }
    ui::absatz(format(Str::MbKgPfad, zielKopf().c_str()).c_str());
    if (t.plaetze.empty() && t.glaZiel == 0) {
        ui::absatz(format(Str::MbKgKeinePlaetze, modell::grenzen(zielspiel()).name).c_str(), ui::Ton::Warnung);
        ui::rolloutEnde();
        return;
    }
    ui::absatz(format(Str::MbKgPlaetze, static_cast<int>(t.plaetze.size()), belegt, static_cast<int>(t.plaetze.size()) - belegt).c_str());
    ui::absatz(tr(Str::MbKgZiehen));
    // "Fuer das Spiel bauen..." steht als Hauptknopf in seiner eigenen Gruppe (Entwurf)
    ui::rasterBeginn(2);
    if (t.glaZiel == 0 && ui::werkzeugKnopf(tr(Str::MbKgVorhanden), {.icon = UiIcon::Oeffnen, .hinweis = tr(Str::MbKgVorhandenHint)})) { (void)modellVorhandeneKartenGla(); }
    if (ui::werkzeugKnopf(tr(Str::MbXsiAufnehmen), {.icon = UiIcon::Plus, .hinweis = tr(Str::MbXsiAufnehmenHint)})) { modellXsiAufnehmenFrage(); }
    ui::rasterEnde();
    ImGui::BeginChild("##kgplaetze", ImVec2{-1.0F, fs * 14.0F}, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(t.plaetze.size()));
    while (clip.Step()) {
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
            modell::Platz& p = t.plaetze[static_cast<std::size_t>(i)];
            ImGui::PushID(i);
            std::string zeile = p.name + "   ";
            zeile += p.belegt ? p.quelle.sequenz.name + "  (" +
                                    format(Str::MbKgQuelle, fileName(p.quelle.glaName).c_str(), p.quelle.sequenz.firstFrame,
                                           p.quelle.sequenz.firstFrame + p.quelle.sequenz.numFrames - 1) +
                                    ")"
                              : std::string(tr(Str::MbKgFrei));
            if (!p.belegt) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
            ImGui::Selectable((zeile + "###platz").c_str(), t.platzWahl == i);
            if (!p.belegt) { ImGui::PopStyleColor(); }
            if (ImGui::IsItemClicked()) { t.platzWahl = i; }
            ImGui::SetItemTooltip("%s", tr(Str::MbKgZiehen));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("MB_SEQ"); pl != nullptr && pl->DataSize == sizeof(int)) {
                    int anim = 0;
                    std::memcpy(&anim, pl->Data, sizeof anim);
                    (void)modellPlatzSetzen(i, anim);
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::BeginPopupContextItem("##platzkontext")) {
                if (ImGui::MenuItem(tr(Str::MbKgHierher), nullptr, false, g_app->animIndex >= 0)) { (void)modellPlatzSetzen(i, g_app->animIndex); }
                if (ImGui::MenuItem(tr(Str::MbKgLeeren), nullptr, false, p.belegt)) { modellPlatzLeeren(i); }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }
    if (t.glaZiel == 2) {
        // eigene GLA: neue Plaetze heissen wie die gezogene Sequenz
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::Selectable((std::string(tr(Str::MbZielEigenNeu)) + "###platzneu").c_str());
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%s", tr(Str::MbKgZiehen));
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("MB_SEQ"); pl != nullptr && pl->DataSize == sizeof(int)) {
                int anim = 0;
                std::memcpy(&anim, pl->Data, sizeof anim);
                (void)modellAufFreienPlatz(anim);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();
    ui::rolloutEnde();
}

}  // namespace

// GLA, animation.cfg, Ziel-GLA und Plaetze: mehr als der Entwurf zeigt (Zeile A14), hinter seinen Gruppen.
void drawModellAnimationBereich() {
    cfgAbgleich();
    plaetzeAnpassen();
    gruppeGla();
    gruppeCfg();
    gruppePlaetze();
}

// --- Marker in der Zeitleiste --------------------------------------------------------------------------------------
namespace {

void markerSetzen(const AnimEntry& neu) {
    ModellTab& t = mt();
    const int z = t.dok.inhalt().cfg.zeileVon(neu.name);
    if (z < 0) { return; }
    const AnimEntry& alt = t.dok.inhalt().cfg.zeilen[static_cast<std::size_t>(z)].e;
    if (alt.firstFrame == neu.firstFrame && alt.numFrames == neu.numFrames && alt.loopFrame == neu.loopFrame && alt.fps == neu.fps) { return; }
    t.dok.aendern(format(Str::MbMarkerSchritt, neu.name.c_str()), [&](modell::Inhalt& in) { in.cfg.setzen(neu); });
    ++t.markerAenderungen;
    diag::detail("Model-Tab: Marker " + neu.name + " " + std::to_string(neu.firstFrame) + " " + std::to_string(neu.numFrames) + " " +
                 std::to_string(neu.loopFrame) + " " + std::to_string(neu.fps));
}

}  // namespace

void modellCfgAbgleich() { cfgAbgleich(); }

bool modellMarkerAendern(const AnimEntry& neu) {
    const int vorher = mt().markerAenderungen;
    markerSetzen(neu);
    cfgAbgleich();
    return mt().markerAenderungen != vorher;
}

// AP1: die Zeile steht im Fenster Zeitkonfiguration (app_modelltab_zeit.cpp), bis rc611 als vierte Zeile in der Leiste
void modellMarkerZeile(const AnimEntry* e) {
    ModellTab& t = mt();
    cfgAbgleich();
    const float fs = ImGui::GetFontSize();
    ImGui::AlignTextToFramePadding();
    cjkTextDisabled("%s", tr(Str::MbMarker));
    ImGui::SetItemTooltip("%s", tr(Str::MbMarkerHint));
    const bool da = e != nullptr && !t.dok.inhalt().cfgPfad.empty() && t.dok.inhalt().cfg.zeileVon(e->name) >= 0;
    ImGui::BeginDisabled(!da);
    AnimEntry w = da ? *e : AnimEntry{};
    bool fertig = false;
    const auto feld = [&](const char* id, Str text, int& wert, int lo, int hi) {
        ImGui::SameLine(0.0F, fs * 0.6F);
        cjkTextUnformatted(tr(text));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(fs * 5.5F);
        int v = wert;
        if (ImGui::InputInt(id, &v, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue)) {
            wert = std::clamp(v, lo, hi);
            fertig = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            wert = std::clamp(v, lo, hi);
            fertig = true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MbMarkerHint));
    };
    feld("##mkanfang", Str::MbMarkerAnfang, w.firstFrame, 0, 65535);
    feld("##mkanzahl", Str::MbMarkerAnzahl, w.numFrames, 1, 65535);
    feld("##mkschleife", Str::MbMarkerSchleife, w.loopFrame, -1, 127);
    feld("##mkfps", Str::MbMarkerFps, w.fps, -1000, 1000);
    // Der Balken: die Frames der GLA um die Sequenz, ihre Enden zum Ziehen (wie die Bereichsmarker der Zeitleisten von
    // 3ds Max und Blender).
    ImGui::SameLine(0.0F, fs * 0.8F);
    const float breite = std::max(ImGui::GetContentRegionAvail().x, fs * 6.0F);
    const float hoehe = ImGui::GetFrameHeight();
    ImGui::InvisibleButton("##mkbalken", ImVec2{breite, hoehe});
    const ImVec2 lo = ImGui::GetItemRectMin();
    const ImVec2 hi = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), 3.0F);
    if (da) {
        const int rand = std::max(w.numFrames / 2, 10);
        const float von = static_cast<float>(std::max(e->firstFrame - rand, 0));
        const float bis = static_cast<float>(e->firstFrame + e->numFrames + rand);
        const auto xVon = [&](float f) { return lo.x + (f - von) / std::max(bis - von, 1.0F) * (hi.x - lo.x); };
        const float x0 = xVon(static_cast<float>(w.firstFrame));
        const float x1 = xVon(static_cast<float>(w.firstFrame + w.numFrames));
        dl->AddRectFilled(ImVec2{x0, lo.y + 2.0F}, ImVec2{x1, hi.y - 2.0F}, ImGui::GetColorU32(ImGuiCol_Header), 2.0F);
        const float xs = xVon(static_cast<float>(g_app->animFrame));
        dl->AddLine(ImVec2{xs, lo.y}, ImVec2{xs, hi.y}, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), 2.0F);
        dl->AddRectFilled(ImVec2{x0 - 3.0F, lo.y}, ImVec2{x0 + 3.0F, hi.y}, ImGui::GetColorU32(ImGuiCol_SliderGrabActive));
        dl->AddRectFilled(ImVec2{x1 - 3.0F, lo.y}, ImVec2{x1 + 3.0F, hi.y}, ImGui::GetColorU32(ImGuiCol_SliderGrabActive));
        const ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsItemActivated()) { t.markerGriff = std::fabs(io.MousePos.x - x0) <= std::fabs(io.MousePos.x - x1) ? 1 : 2; }
        if (ImGui::IsItemActive() && t.markerGriff != 0) {
            const int f = static_cast<int>(std::lround(von + (io.MousePos.x - lo.x) / std::max(hi.x - lo.x, 1.0F) * (bis - von)));
            const int ende = e->firstFrame + e->numFrames;
            if (t.markerGriff == 1) {
                w.firstFrame = std::clamp(f, 0, ende - 1);
                w.numFrames = ende - w.firstFrame;
            } else {
                w.numFrames = std::max(f - e->firstFrame, 1);
            }
            t.markerZug = w;
        }
        if (ImGui::IsItemDeactivated() && t.markerGriff != 0) {
            w = t.markerZug;
            t.markerGriff = 0;
            fertig = true;
        }
        ImGui::SetItemTooltip("%s", tr(Str::MbMarkerHint));
    }
    ImGui::EndDisabled();
    if (da && fertig) { (void)modellMarkerAendern(w); }
}

}  // namespace bhed::gui
