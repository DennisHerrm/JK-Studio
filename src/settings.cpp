#include "bhed/settings.h"
#include "bhed/regelwerk.h"

#include "bhed/num.h"
#include "bhed/sicherschreiben.h"
#include <algorithm>

#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>

namespace bhed {
namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    auto sp = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (a < b && sp(s[a])) { ++a; }
    while (b > a && sp(s[b - 1])) { --b; }
    return s.substr(a, b - a);
}

void put(std::string& o, const char* key, const std::string& value) {
    o += key;
    o += '=';
    o += value;
    o += '\n';
}

void put(std::string& o, const char* key, bool value) {
    put(o, key, value ? std::string("1") : std::string("0"));
}

bool toBool(const std::string& v) { return v == "1" || v == "true" || v == "yes"; }

}  // namespace

void Settings::noteRecent(const std::string& path) {
    if (path.empty()) {
        return;
    }
    for (std::size_t i = 0; i < recent.size(); ++i) {
        if (recent[i] == path) {
            recent.erase(recent.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
    recent.insert(recent.begin(), path);
    while (recent.size() > kMaxRecent) {
        recent.pop_back();
    }
}

void Settings::noteRecentMission(const std::string& eintrag) {
    if (eintrag.empty()) {
        return;
    }
    recentMissions.erase(std::remove(recentMissions.begin(), recentMissions.end(), eintrag), recentMissions.end());
    recentMissions.insert(recentMissions.begin(), eintrag);
    while (recentMissions.size() > kMaxRecentMissions) {
        recentMissions.pop_back();
    }
}

std::string writeSettings(const Settings& s) {
    std::string o = "# behaved\n";
    put(o, "scriptPath", s.scriptPath);
    put(o, "ibizePath", s.ibizePath);
    put(o, "commandFile", s.commandFile);
    put(o, "sourcePath", s.sourcePath);
    put(o, "reopenLastFile", s.reopenLastFile);
    put(o, "legalExit", s.legalExit);
    put(o, "alphaSortPulldowns", s.alphaSortPulldowns);
    put(o, "alternativeIcons", s.alternativeIcons);
    put(o, "queryOnDiscard", s.queryOnDiscard);
    put(o, "dialogUsesLastDir", s.dialogUsesLastDir);
    put(o, "lastDir", s.lastDir);
    put(o, "mapPath", s.mapPath);
    put(o, "language", s.language);
    put(o, "theme", s.theme);
    put(o, "windowPlacement", s.windowPlacement);
    // Die Helferzeilen, eine Zeile je Befehl und Feld.
    for (const auto& kv : s.helfer) {
        put(o, ("helfer." + kv.first).c_str(), kv.second);
    }
    for (const auto& kv : s.lesezeichen) {
        put(o, ("lesezeichen." + kv.first).c_str(), kv.second);
    }
    // rc601 tour: wie weit die Tutorials im Programm sind.
    for (const auto& kv : s.tourStand) {
        put(o, ("tour." + kv.first).c_str(), kv.second);
    }
    put(o, "ersteSchritteGezeigt", s.ersteSchritteGezeigt);   // H1
    put(o, "kiChat", s.kiChat);
    put(o, "kiModell", s.kiModell);
    put(o, "kiOhneNachfrage", s.kiOhneNachfrage);
    put(o, "mcpHttp", s.mcpHttp);
    put(o, "mcpPort", std::to_string(s.mcpPort));
    put(o, "changeHistory", s.changeHistory);
    put(o, "updateCheck", s.updateCheck);
    put(o, "grafikD3d12", s.grafikD3d12);
    put(o, "grafikRueckfallFassung", s.grafikRueckfallFassung);
    put(o, "updateUebersprungen", s.updateUebersprungen);
    put(o, "highlightSame", s.highlightSame);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(s.uiScale));
    put(o, "uiScale", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitEvents));
    put(o, "splitEvents", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitButtons));
    put(o, "splitButtons", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitMap));
    put(o, "splitMap", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitModel));
    put(o, "splitModel", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitStatus));
    put(o, "splitStatus", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.timelineFrac));
    put(o, "timelineFrac", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitFracX));
    put(o, "splitFracX", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.splitFracY));
    put(o, "splitFracY", std::string(buf));
    put(o, "timelineFrames", s.timelineFrames);
    put(o, "mapSidebar", s.mapSidebar);
    put(o, "schattenArt", std::to_string(s.schattenArt));
    put(o, "nebel", s.nebel);
    put(o, "dynLicht", s.dynLicht);
    put(o, "spielerInCutscene", s.spielerInCutscene);
    put(o, "cgMotionBoneComp", std::to_string(s.cgMotionBoneComp));
    put(o, "cgTurnAnims", s.cgTurnAnims);
    put(o, "gActivateAnimationStyle", s.gActivateAnimationStyle);
    std::snprintf(buf, sizeof(buf), "%.6f", static_cast<double>(s.cgSaberLightIntensity));
    put(o, "cgSaberLightIntensity", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.6f", static_cast<double>(s.cgSaberLightRadius));
    put(o, "cgSaberLightRadius", std::string(buf));
    put(o, "mpZiel", s.mpZiel);
    // Paket kamera, rc601: das Spielfenster.
    std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(s.spielSeiten));
    put(o, "spielSeiten", std::string(buf));
    put(o, "fovAspectAdjust", std::to_string(s.fovAspectAdjust));
    put(o, "spielRahmenFrei", s.spielRahmenFrei);
    put(o, "difficulty", std::to_string(s.schwierigkeit));   // rc601 (bewegung, Durchsicht 2): g_spskill
    put(o, "r_picmip", std::to_string(s.bildPicmip));
    put(o, "modelControls", s.modellBedienung == 1 ? "blender" : "max");   // mb1a1
    put(o, "modelTimelineTracks", s.modellZeitSpuren);                      // AP1
    put(o, "modelTimelineAutoKey", s.modellZeitAutoKey);                    // ENTSCHEID1
    put(o, "modelBlenderToolNames", s.modellBlWerkzeugnamen);               // ENTSCHEID1
    std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(s.modellBlLeisteBreite));
    put(o, "modelBlenderToolbarWidth", std::string(buf));                   // ENTSCHEID3A (B8)
    std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(s.modellZeitSpurHoeheMax));
    put(o, "modelTimelineTracksMax", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(s.modellZeitSpurHoeheBlender));
    put(o, "modelTimelineTracksBlender", std::string(buf));
    put(o, "modelTimelineDisplay", std::to_string(s.modellZeitAnzeige));
    put(o, "modelBoneFins", s.modellFlossen ? "1" : "0");
    put(o, "modelBoneDisplay", std::to_string(s.modellKnochenArt));   // AP13K
    put(o, "modelRigStyle", std::to_string(s.modellRigStil));
    put(o, "modelBonesInFront", s.modellKnochenVorn ? "1" : "0");
    put(o, "r_ext_multisample", std::to_string(s.rExtMultisample));
    put(o, "regelwerk", std::string(regelwerkSchluessel(s.regelwerk)));   // rc601 integ
    put(o, "flussAnsicht", std::to_string(s.flussAnsicht));
    put(o, "menueReiter", std::to_string(s.menueReiter));
    std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(s.menueRechtsBreite));
    put(o, "menueRechtsBreite", std::string(buf));
    put(o, "menueTextUnten", s.menueTextUnten);
    put(o, "menueGanzeDatei", s.menueGanzeDatei);
    put(o, "menueDateiSchluessel", s.menueDateiSchluessel);
    put(o, "menueTeileHinweisWeg", s.menueTeileHinweisWeg);
    put(o, "menueTextZoom", std::to_string(s.menueTextZoom));
    put(o, "menueLetzteFarben", s.menueLetzteFarben);
    put(o, "menueTextUmbruch", s.menueUmbruch);
    put(o, "menueTon", s.menueTon);
    put(o, "menueLeisteText", s.menueLeisteText);
    put(o, "pk3SpeicherWeg", std::to_string(s.pk3SpeicherWeg));
    put(o, "missionsordnerAblauf", s.missionsordnerAblauf);
    put(o, "skriptVorlage", s.skriptVorlage);
    put(o, "fs_dirbeforepak", s.fsDirBeforePak);
    put(o, "entTextOffen", s.entTextOffen);
    put(o, "insSkriptOffen", s.insSkriptOffen);                        // mb1a2
    put(o, "modelBackground", std::to_string(s.modellHintergrund));   // mb1a2
    put(o, "modelViewCube", s.modellNavigation);                       // MBKOPF
    put(o, "modelViewCubeAll", s.modellNavigationAlle);
    put(o, "modelCompass", s.modellKompass);
    put(o, "modelCompassNorth", std::to_string(s.modellKompassNord));
    put(o, "modelOrbit", std::to_string(s.modellOrbitArt));               // MBNAV
    put(o, "modelZoomMouseOrtho", s.modellZoomMausOrtho);
    put(o, "modelZoomMousePersp", s.modellZoomMausPersp);
    put(o, "modelBlenderOrbitSelection", s.modellBlOrbitAuswahl);
    put(o, "modelBlenderZoomToMouse", s.modellBlZoomMaus);
    put(o, "modelGridSpacing", std::to_string(s.modellRasterAbstand));   // MBRASTER
    put(o, "modelGridMajor", std::to_string(s.modellRasterHaupt));
    put(o, "modelGridExtent", std::to_string(s.modellRasterAusdehnung));
    put(o, "modelDisplayMovedSeen", s.modellAnzeigeUmzugGesehen);
    put(o, "modelPanelWidth", std::to_string(s.modellFeldBreite));   // AP3
    for (const std::string& h : s.modellHome) { put(o, "modelHome", h); }   // KLEIN: gesetztes Home je Figur
    put(o, "lumpEditOffen", s.lumpEditOffen);
    put(o, "lumpEditKarteOffen", s.lumpEditKarteOffen);
    put(o, "texListeOffen", s.texListeOffen);
    put(o, "showTypes", s.showTypes);
    put(o, "gFloats", s.gFloats);
    put(o, "foldMacros", s.foldMacros);
    // rc601 ton: die Mischpegel der Vorschau (s_volume, s_volumeVoice,
    // s_musicvolume).
    std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(s.tonEffekte));
    put(o, "tonEffekte", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(s.tonStimmen));
    put(o, "tonStimmen", std::string(buf));
    std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(s.tonMusik));
    put(o, "tonMusik", std::string(buf));
    for (const std::string& r : s.recent) {
        put(o, "recent", r);
    }
    for (const std::string& r : s.recentMissions) {
        put(o, "recentMission", r);
    }
    for (const std::string& g : s.gamePaths) {
        put(o, "gamePath", g);
    }
    for (const std::string& g : s.gamePathsJk2) {   // rc604
        put(o, "gamePathJk2", g);
    }
    // Karten-Editor (Stufe 1b)
    for (const std::string& k : s.karteZuletzt) {
        put(o, "karteZuletzt", k);
    }
    for (const auto& [name, wert] : s.karte) {
        if (name.empty() || name.find('=') != std::string::npos) { continue; }
        put(o, "karte", name + "=" + wert);
    }
    for (const auto& [name, taste] : s.karteTasten) {
        if (name.empty() || name.find('=') != std::string::npos) { continue; }
        put(o, "karteTaste", name + "=" + taste);
    }
    // Stufe dock1: die Anordnung der Fenster je Bereich (bhed/fensteranordnung.h).
    for (const auto& [name, wert] : s.fenster) {
        if (name.empty() || name.find('=') != std::string::npos || wert.find('\n') != std::string::npos) { continue; }
        put(o, "fenster", name + "=" + wert);
    }
    // "key=MoveUp 4194643" - Aktionsname und ImGuiKeyChord als Zahl.
    // Warum die Zahl und nicht der Name der Taste, steht bei
    // Settings::KeyBinding.
    for (const Settings::KeyBinding& b : s.keyBindings) {
        if (b.action.empty() || b.action.find(' ') != std::string::npos) {
            continue;   // kaputte Kennung nicht schreiben
        }
        put(o, "key", b.action + " " + std::to_string(b.chord));
    }
    return o;
}

namespace {

// REST612 (rc612): ein Leser je Schluessel statt der langen else-if-Kette. Die Kette stand an der Grenze des
// Compilers (C1061: hoechstens 128 verschachtelte Bloecke, jedes "else if" ist einer). Ein neuer Schluessel ist eine
// Zeile in kLeser (und ein put(...) in writeSettings). Steht ein Schluessel zweimal in kLeser, gilt der erste - wie in
// der Kette, in der der zweite Zweig nie erreicht wurde. Schluessel mit Vorsatz ("helfer.<befehl>") stehen in
// kVorsatzLeser; kein Schluessel aus kLeser beginnt mit einem dieser Vorsaetze. Gleich zu lesen wie die Kette belegt
// ctest einstellungen_alle.
struct Zeile {
    Settings& out;
    const std::string& key;     // getrimmt, vor dem ersten '='
    const std::string& value;   // getrimmt, nach dem ersten '='
    const std::string& line;    // die ganze getrimmte Zeile
};
using Leser = void (*)(const Zeile& z);
struct SchluesselLeser {
    const char* key;
    Leser lesen;
};

// rc601 ton: die Mischpegel der Vorschau, nur 0..1 uebernehmen.
void lies_tonEffekte(const Zeile& z) {
    float v = 0.0F;
    if (parseFloat(z.value, v) && v >= 0.0F && v <= 1.0F) {
        (z.key == "tonEffekte" ? z.out.tonEffekte : z.key == "tonStimmen" ? z.out.tonStimmen : z.out.tonMusik) = v;
    }
}

// Paket kamera, rc601: das Spielfenster. 0 = wie der Bildschirm;
// sonst ein Seitenverhaeltnis, wie es Bildschirme haben (9:16 hochkant
// bis 32:9) - eine von Hand verdorbene Zahl bleibt auf der Vorgabe.
void lies_cgSaberLightIntensity(const Zeile& z) {
    float v = 0.0F;
    if (parseFloat(z.value, v) && v >= 0.0F && v <= 8.0F) {
        (z.key == "cgSaberLightIntensity" ? z.out.cgSaberLightIntensity : z.out.cgSaberLightRadius) = v;
    }
}

void lies_karte(const Zeile& z) {
    const std::size_t gleich = z.value.find('=');
    if (gleich != std::string::npos && gleich > 0) {
        (z.key == "karte" ? z.out.karte : z.out.karteTasten)[z.value.substr(0, gleich)] = z.value.substr(gleich + 1);
    }
}

const SchluesselLeser kVorsatzLeser[] = {
    {"breite.", [](const Zeile&) {
        // Gelernte Breiten des Ereignisfensters (rc564-rc567). Seit
        // rc568 wird die Breite ausgerechnet; alte Zeilen werden
        // ueberlesen und beim naechsten Speichern nicht mehr geschrieben.
    }},
    {"helfer.", [](const Zeile& z) {
        z.out.helfer[z.key.substr(7)] = z.value;
    }},
    {"lesezeichen.", [](const Zeile& z) {
        // Der Schluessel ist ein Dateipfad und darf '=' enthalten
        // ("C:/a=b/skript.txt"); der Wert ist nur eine Zahlenliste. Also
        // am LETZTEN '=' trennen - sonst ging das Lesezeichen verloren.
        const std::size_t le = z.line.rfind('=');
        z.out.lesezeichen[trim(z.line.substr(12, le - 12))] = trim(z.line.substr(le + 1));
    }},
    {"tour.", [](const Zeile& z) {   // rc601 tour
        z.out.tourStand[z.key.substr(5)] = z.value;
    }},
};

const SchluesselLeser kLeser[] = {
    {"grafikRueckfallFassung", [](const Zeile& z) { z.out.grafikRueckfallFassung = z.value; }},   // D3D12-3 B1
    {"windowPlacement", [](const Zeile& z) { z.out.windowPlacement = z.value; }},
    {"tonEffekte", lies_tonEffekte},
    {"tonStimmen", lies_tonEffekte},
    {"tonMusik", lies_tonEffekte},
    {"ersteSchritteGezeigt", [](const Zeile& z) { z.out.ersteSchritteGezeigt = toBool(z.value); }},   // H1
    {"kiChat", [](const Zeile& z) { z.out.kiChat = toBool(z.value); }},
    {"kiModell", [](const Zeile& z) {
        // nur bekannte Modelle (die Datei ist kein Weg, etwas anderes anzusprechen)
        if (z.value == "claude-sonnet-5-5" || z.value == "claude-haiku-5-5" || z.value == "claude-opus-5-5") { z.out.kiModell = z.value; }
    }},
    {"kiOhneNachfrage", [](const Zeile& z) { z.out.kiOhneNachfrage = toBool(z.value); }},
    {"mcpHttp", [](const Zeile& z) { z.out.mcpHttp = toBool(z.value); }},
    {"mcpPort", [](const Zeile& z) {
        const int p = std::atoi(z.value.c_str());
        if (p >= 1024 && p <= 65535) { z.out.mcpPort = p; }
    }},
    {"changeHistory", [](const Zeile& z) { z.out.changeHistory = toBool(z.value); }},
    {"updateCheck", [](const Zeile& z) { z.out.updateCheck = toBool(z.value); }},
    {"grafikD3d12", [](const Zeile& z) { z.out.grafikD3d12 = toBool(z.value); }},
    {"updateUebersprungen", [](const Zeile& z) { z.out.updateUebersprungen = z.value; }},
    {"highlightSame", [](const Zeile& z) { z.out.highlightSame = toBool(z.value); }},
    {"scriptPath", [](const Zeile& z) { z.out.scriptPath = z.value; }},
    {"ibizePath", [](const Zeile& z) { z.out.ibizePath = z.value; }},
    {"commandFile", [](const Zeile& z) { z.out.commandFile = z.value; }},
    {"sourcePath", [](const Zeile& z) { z.out.sourcePath = z.value; }},
    {"reopenLastFile", [](const Zeile& z) { z.out.reopenLastFile = toBool(z.value); }},
    {"legalExit", [](const Zeile& z) { z.out.legalExit = toBool(z.value); }},
    {"alphaSortPulldowns", [](const Zeile& z) { z.out.alphaSortPulldowns = toBool(z.value); }},
    {"alternativeIcons", [](const Zeile& z) { z.out.alternativeIcons = toBool(z.value); }},
    {"queryOnDiscard", [](const Zeile& z) { z.out.queryOnDiscard = toBool(z.value); }},
    {"dialogUsesLastDir", [](const Zeile& z) { z.out.dialogUsesLastDir = toBool(z.value); }},
    {"lastDir", [](const Zeile& z) { z.out.lastDir = z.value; }},
    {"mapPath", [](const Zeile& z) { z.out.mapPath = z.value; }},
    {"language", [](const Zeile& z) { z.out.language = z.value; }},
    {"theme", [](const Zeile& z) { z.out.theme = z.value; }},
    {"showTypes", [](const Zeile& z) { z.out.showTypes = toBool(z.value); }},
    {"menueReiter", [](const Zeile& z) {
        const int v = std::atoi(z.value.c_str());
        if (v >= 0 && v <= 3) { z.out.menueReiter = v; }
    }},
    {"menueRechtsBreite", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v) && v >= 0.0F && v <= 200.0F) { z.out.menueRechtsBreite = v; }
    }},
    {"menueTextUnten", [](const Zeile& z) { z.out.menueTextUnten = toBool(z.value); }},
    {"menueGanzeDatei", [](const Zeile& z) { z.out.menueGanzeDatei = toBool(z.value); }},
    {"menueDateiSchluessel", [](const Zeile& z) { z.out.menueDateiSchluessel = toBool(z.value); }},
    {"menueTeileHinweisWeg", [](const Zeile& z) { z.out.menueTeileHinweisWeg = toBool(z.value); }},
    {"menueTextZoom", [](const Zeile& z) {
        const int v = std::atoi(z.value.c_str());
        if (v >= 50 && v <= 300) { z.out.menueTextZoom = v; }
    }},
    {"menueLetzteFarben", [](const Zeile& z) { z.out.menueLetzteFarben = z.value; }},
    {"menueTextUmbruch", [](const Zeile& z) { z.out.menueUmbruch = toBool(z.value); }},
    {"menueTon", [](const Zeile& z) { z.out.menueTon = toBool(z.value); }},
    {"menueLeisteText", [](const Zeile& z) { z.out.menueLeisteText = toBool(z.value); }},
    {"pk3SpeicherWeg", [](const Zeile& z) {
        const int v = std::atoi(z.value.c_str());
        if (v >= 0 && v <= 4) { z.out.pk3SpeicherWeg = v; }
    }},
    {"missionsordnerAblauf", [](const Zeile& z) { z.out.missionsordnerAblauf = toBool(z.value); }},
    {"skriptVorlage", [](const Zeile& z) { z.out.skriptVorlage = toBool(z.value); }},
    {"fs_dirbeforepak", [](const Zeile& z) { z.out.fsDirBeforePak = toBool(z.value); }},
    {"entTextOffen", [](const Zeile& z) { z.out.entTextOffen = toBool(z.value); }},
    {"insSkriptOffen", [](const Zeile& z) { z.out.insSkriptOffen = toBool(z.value); }},   // mb1a2
    {"modelBackground", [](const Zeile& z) { z.out.modellHintergrund = std::atoi(z.value.c_str()); }},   // mb1a2
    {"modelViewCube", [](const Zeile& z) { z.out.modellNavigation = toBool(z.value); }},   // MBKOPF
    {"modelViewCubeAll", [](const Zeile& z) { z.out.modellNavigationAlle = toBool(z.value); }},
    {"modelCompass", [](const Zeile& z) { z.out.modellKompass = toBool(z.value); }},
    {"modelCompassNorth", [](const Zeile& z) { z.out.modellKompassNord = ((std::atoi(z.value.c_str()) % 360) + 360) % 360; }},
    {"modelOrbit", [](const Zeile& z) { z.out.modellOrbitArt = std::clamp(std::atoi(z.value.c_str()), 0, 3); }},   // MBNAV
    {"modelZoomMouseOrtho", [](const Zeile& z) { z.out.modellZoomMausOrtho = toBool(z.value); }},
    {"modelZoomMousePersp", [](const Zeile& z) { z.out.modellZoomMausPersp = toBool(z.value); }},
    {"modelBlenderOrbitSelection", [](const Zeile& z) { z.out.modellBlOrbitAuswahl = toBool(z.value); }},
    {"modelBlenderZoomToMouse", [](const Zeile& z) { z.out.modellBlZoomMaus = toBool(z.value); }},
    {"modelGridSpacing", [](const Zeile& z) { z.out.modellRasterAbstand = std::clamp(static_cast<float>(std::atof(z.value.c_str())), 0.125F, 1024.0F); }},   // MBRASTER
    {"modelGridMajor", [](const Zeile& z) { z.out.modellRasterHaupt = std::clamp(std::atoi(z.value.c_str()), 2, 100); }},
    {"modelGridExtent", [](const Zeile& z) { z.out.modellRasterAusdehnung = std::clamp(std::atoi(z.value.c_str()), 1, 1000); }},
    {"modelDisplayMovedSeen", [](const Zeile& z) { z.out.modellAnzeigeUmzugGesehen = toBool(z.value); }},
    {"modelPanelWidth", [](const Zeile& z) {   // AP3: 0 = Vorgabe, sonst 12..44 Zeichenhoehen
        const float v = static_cast<float>(std::atof(z.value.c_str()));
        z.out.modellFeldBreite = v <= 0.0F ? 0.0F : std::clamp(v, 12.0F, 44.0F);
    }},
    {"modelHome", [](const Zeile& z) {   // KLEIN: gesetztes Home je Figur
        if (z.out.modellHome.size() < Settings::kMaxModellHome) { z.out.modellHome.push_back(z.value); }
    }},
    {"lumpEditOffen", [](const Zeile& z) { z.out.lumpEditOffen = toBool(z.value); }},
    {"lumpEditKarteOffen", [](const Zeile& z) { z.out.lumpEditKarteOffen = toBool(z.value); }},
    {"texListeOffen", [](const Zeile& z) { z.out.texListeOffen = toBool(z.value); }},
    {"gFloats", [](const Zeile& z) { z.out.gFloats = toBool(z.value); }},
    {"foldMacros", [](const Zeile& z) { z.out.foldMacros = toBool(z.value); }},
    {"uiScale", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            // Unsinnige Werte nicht uebernehmen: eine kaputte Datei darf
            // das Fenster nicht unbedienbar machen.
            if (v >= 0.5F && v <= 4.0F) { z.out.uiScale = v; }
        }
    }},
    {"splitEvents", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.05F && v <= 0.90F) { z.out.splitEvents = v; }
        }
    }},
    // --- splitMap wurde GESCHRIEBEN, aber nie gelesen ---------------
    //
    // Gemeldet: "jedes Mal wenn ich das Programm starte ist die Leiste
    // rechts wieder ausgestreckt, statt so wie ich es eingestellt habe."
    //
    // Im Protokoll stand es deutlich, man musste nur beide Zeilen
    // nebeneinanderhalten:
    //
    //     geladen:   Karte 0.550   <- der VORGABEwert (settings.h:50)
    //     gesichert: Karte 0.943
    //
    // Jede Sitzung sicherte 0.943 und lud wieder 0.550. Der Schreiber
    // kannte den Schluessel (settings.cpp:77), der Leser nicht - also
    // fiel der Wert bei jedem Start auf die Vorgabe zurueck.
    //
    // Die Grenzen wie beim Schieber selbst (gui/app.cpp: 0.12 bis 0.98).
    {"splitMap", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            // Untergrenze 0.35, nicht 0.12.
            //
            // In dieser Spalte liegt die GANZE Kartenansicht - Ebenen,
            // Bild und die Einstellungsspalte. Bei 0.12 ist davon
            // nichts zu erkennen.
            //
            // Und es steht wirklich so in shanks Datei: die
            // Rueckkopplung aus rc484 hat den Wert bis zur Untergrenze
            // heruntergeschrieben, und beim Start galt er wieder.
            // "Das ist immer, wenn ich es starte, und dann muss ich es
            // nach rechts ziehen."
            //
            // Ein gespeicherter Wert, der unbrauchbar ist, wird
            // verworfen und nicht uebernommen.
            if (v >= 0.35F && v <= 0.98F) { z.out.splitMap = v; }
        }
    }},
    // Die Modellansicht hat ihren eigenen Wert - dieselben Grenzen,
    // denn in ihrer Spalte steckt ebenfalls die ganze Ansicht.
    {"splitModel", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.35F && v <= 0.98F) { z.out.splitModel = v; }
        }
    }},
    {"splitStatus", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.05F && v <= 0.60F) { z.out.splitStatus = v; }
        }
    }},
    {"timelineFrac", [](const Zeile& z) {
        float v = 0.0F;
        // Dieselben Grenzen wie beim Ziehen - eine Datei von Hand
        // bearbeitet, und die Leiste waere sonst weg.
        if (parseFloat(z.value, v) && v >= 0.08F && v <= 0.60F) {
            z.out.timelineFrac = v;
        }
    }},
    {"splitFracX", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v) && v >= 0.15F && v <= 0.85F) {
            z.out.splitFracX = v;
        }
    }},
    {"splitFracY", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v) && v >= 0.15F && v <= 0.85F) {
            z.out.splitFracY = v;
        }
    }},
    {"timelineFrames", [](const Zeile& z) {
        z.out.timelineFrames = (z.value == "true" || z.value == "1");
    }},
    {"schattenArt", [](const Zeile& z) {
        const int n = std::atoi(z.value.c_str());
        z.out.schattenArt = (n >= 0 && n <= 3) ? n : 1;
    }},
    {"nebel", [](const Zeile& z) { z.out.nebel = toBool(z.value); }},
    {"dynLicht", [](const Zeile& z) { z.out.dynLicht = toBool(z.value); }},
    {"spielerInCutscene", [](const Zeile& z) { z.out.spielerInCutscene = toBool(z.value); }},
    {"cgMotionBoneComp", [](const Zeile& z) { z.out.cgMotionBoneComp = std::clamp(std::atoi(z.value.c_str()), 0, 3); }},
    {"cgTurnAnims", [](const Zeile& z) { z.out.cgTurnAnims = toBool(z.value); }},
    {"gActivateAnimationStyle", [](const Zeile& z) { z.out.gActivateAnimationStyle = toBool(z.value); }},
    {"mpZiel", [](const Zeile& z) { z.out.mpZiel = toBool(z.value); }},
    {"cgSaberLightIntensity", lies_cgSaberLightIntensity},
    {"cgSaberLightRadius", lies_cgSaberLightIntensity},
    {"spielSeiten", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v) && (v == 0.0F || (v >= 0.25F && v <= 8.0F))) { z.out.spielSeiten = v; }
    }},
    {"fovAspectAdjust", [](const Zeile& z) {
        if (z.value == "-1" || z.value == "0" || z.value == "1") { z.out.fovAspectAdjust = std::atoi(z.value.c_str()); }
    }},
    {"spielRahmenFrei", [](const Zeile& z) { z.out.spielRahmenFrei = toBool(z.value); }},
    {"difficulty", [](const Zeile& z) {
        // rc601 (bewegung, Durchsicht 2): g_spskill 0-3. rc601 integ: -1 =
        // nie gewaehlt (die Vorgabe des Regelwerks), Unsinn ebenso.
        const int n = std::atoi(z.value.c_str());
        z.out.schwierigkeit = (n >= -1 && n <= 3) ? n : -1;
    }},
    {"r_picmip", [](const Zeile& z) { z.out.bildPicmip = std::clamp(std::atoi(z.value.c_str()), 0, 3); }},
    {"modelControls", [](const Zeile& z) { z.out.modellBedienung = z.value == "blender" ? 1 : 0; }},   // mb1a1
    {"modelTimelineTracks", [](const Zeile& z) { z.out.modellZeitSpuren = toBool(z.value); }},   // AP1
    {"modelTimelineAutoKey", [](const Zeile& z) { z.out.modellZeitAutoKey = toBool(z.value); }},   // ENTSCHEID1
    {"modelBlenderToolNames", [](const Zeile& z) { z.out.modellBlWerkzeugnamen = toBool(z.value); }},   // ENTSCHEID1
    {"modelBlenderToolbarWidth", [](const Zeile& z) { z.out.modellBlLeisteBreite = std::clamp(static_cast<float>(std::atof(z.value.c_str())), 0.0F, 2000.0F); }},   // ENTSCHEID3A (B8)
    {"modelTimelineTracksMax", [](const Zeile& z) { z.out.modellZeitSpurHoeheMax = std::clamp(static_cast<float>(std::atof(z.value.c_str())), 2.5F, 26.0F); }},
    {"modelTimelineTracksBlender", [](const Zeile& z) { z.out.modellZeitSpurHoeheBlender = std::clamp(static_cast<float>(std::atof(z.value.c_str())), 2.5F, 26.0F); }},
    {"modelTimelineDisplay", [](const Zeile& z) { z.out.modellZeitAnzeige = std::atoi(z.value.c_str()) == 1 ? 1 : 0; }},
    {"modelBoneFins", [](const Zeile& z) { z.out.modellFlossen = z.value == "1"; }},
    {"modelBoneDisplay", [](const Zeile& z) { z.out.modellKnochenArt = std::clamp(std::atoi(z.value.c_str()), 0, 4); }},   // AP13K
    {"modelRigStyle", [](const Zeile& z) { z.out.modellRigStil = std::clamp(std::atoi(z.value.c_str()), 0, 3); }},
    {"modelBonesInFront", [](const Zeile& z) { z.out.modellKnochenVorn = z.value != "0"; }},
    {"r_ext_multisample", [](const Zeile& z) {
        const int n = std::atoi(z.value.c_str());
        z.out.rExtMultisample = (n == 0 || n == 2 || n == 4 || n == 8) ? n : -1;
    }},
    {"regelwerk", [](const Zeile& z) {
        // rc601 integ: auto/openjk/movieduels, sonst automatisch.
        int w = kRegelwerkAuto;
        z.out.regelwerk = regelwerkAusSchluessel(z.value, w) ? w : kRegelwerkAuto;
    }},
    {"flussAnsicht", [](const Zeile& z) {
        const int n = std::atoi(z.value.c_str());
        z.out.flussAnsicht = (n >= 0 && n <= 2) ? n : 0;
    }},
    {"mapSidebar", [](const Zeile& z) {
        z.out.mapSidebar = (z.value == "true" || z.value == "1");
    }},
    // Doppelt: wird nie gelesen, es gilt der erste Eintrag "splitMap" weiter oben (so war es schon in der
    // else-if-Kette).
    {"splitMap", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.05F && v <= 0.90F) { z.out.splitMap = v; }
        }
    }},
    // Doppelt: wird nie gelesen, es gilt der erste Eintrag "splitModel" weiter oben (so war es schon in der
    // else-if-Kette).
    {"splitModel", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.05F && v <= 0.90F) { z.out.splitModel = v; }
        }
    }},
    {"splitButtons", [](const Zeile& z) {
        float v = 0.0F;
        if (parseFloat(z.value, v)) {
            if (v >= 0.03F && v <= 0.60F) { z.out.splitButtons = v; }
        }
    }},
    {"gamePath", [](const Zeile& z) {
        // Keine Obergrenze: der Dialog nimmt beliebig viele Ordner, und
        // was gespeichert wurde, muss auch wieder geladen werden.
        z.out.gamePaths.push_back(z.value);
    }},
    {"gamePathJk2", [](const Zeile& z) {   // rc604: Spielordner von Jedi Outcast
        z.out.gamePathsJk2.push_back(z.value);
    }},
    {"key", [](const Zeile& z) {
        const std::size_t sp = z.value.find(' ');
        if (sp != std::string::npos && sp != 0) {
            Settings::KeyBinding b;
            b.action = z.value.substr(0, sp);
            try {
                b.chord = std::stoi(z.value.substr(sp + 1));
            } catch (...) {
                return;   // keine Zahl - Eintrag verwerfen, Vorgabe gilt
            }
            z.out.keyBindings.push_back(std::move(b));
        }
    }},
    {"recent", [](const Zeile& z) {
        if (z.out.recent.size() < Settings::kMaxRecent) { z.out.recent.push_back(z.value); }
    }},
    {"recentMission", [](const Zeile& z) {
        if (z.out.recentMissions.size() < Settings::kMaxRecentMissions) { z.out.recentMissions.push_back(z.value); }
    }},
    // Karten-Editor (Stufe 1b): zuletzt geoeffnete Karten (MRU_MAX 9),
    // Einstellungen und Tastenbelegung als "<name>=<wert>".
    {"karteZuletzt", [](const Zeile& z) {
        if (z.out.karteZuletzt.size() < 9) { z.out.karteZuletzt.push_back(z.value); }
    }},
    {"karte", lies_karte},
    {"karteTaste", lies_karte},
    {"fenster", [](const Zeile& z) {   // Stufe dock1
        const std::size_t gleich = z.value.find('=');
        if (gleich != std::string::npos && gleich > 0) { z.out.fenster[z.value.substr(0, gleich)] = z.value.substr(gleich + 1); }
    }},
};

Leser leserFuer(const std::string& key) {
    for (const SchluesselLeser& v : kVorsatzLeser) {
        if (key.rfind(v.key, 0) == 0) { return v.lesen; }
    }
    static const std::unordered_map<std::string_view, Leser> tabelle = [] {
        std::unordered_map<std::string_view, Leser> t;
        for (const SchluesselLeser& l : kLeser) { t.emplace(l.key, l.lesen); }   // emplace: der erste Eintrag gilt
        return t;
    }();
    const auto it = tabelle.find(key);
    return it == tabelle.end() ? nullptr : it->second;
}

}  // namespace

bool readSettings(const std::string& text, Settings& out) {
    out.recent.clear();
    out.recentMissions.clear();
    out.modellHome.clear();
    out.gamePaths.clear();
    out.gamePathsJk2.clear();
    out.keyBindings.clear();
    out.karteZuletzt.clear();
    out.karte.clear();
    out.karteTasten.clear();
    out.fenster.clear();
    std::string line;
    std::istringstream in(text);
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (const Leser lesen = leserFuer(key)) { lesen(Zeile{out, key, value, line}); }
        // Unbekannte Schluessel bleiben unbeachtet.
    }
    return true;
}

bool loadSettingsFile(const std::string& path, Settings& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return readSettings(ss.str(), out);
}

// rc610 F78 (Fund B8-2): bis rc609 mit std::ofstream (trunc) und dem Ergebnis
// vor dem Schliessen - die alte behaved.cfg war leer, bevor ein Byte stand, und
// ein Fehler beim Leeren des Puffers blieb unbemerkt (Spielordner und Tasten
// stehen am Ende der Datei). Jetzt wie F5: Temp-Datei, Schliessen geprueft,
// atomar ersetzt; bei einem Fehler bleibt die alte Datei.
bool saveSettingsFile(const std::string& path, const Settings& s) { return sicherSchreiben(path, writeSettings(s)); }

}  // namespace bhed
