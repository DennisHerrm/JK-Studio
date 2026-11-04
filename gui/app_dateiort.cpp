// app_dateiort.cpp - rc609 W2a: Dateien an Ort und Stelle speichern
//
// Mike (03.11.): "If I open a bsp or ent file, I don't see why those files
// can't be modified rather than making copies and pk3 files. ... Everything
// should have an option. Open .bsp, open .ent, open script. When I make
// changes, I expect the files I opened to be affected. Not be copied to the
// documents folder and a .pk3 file (which gives 3 versions of the same map and
// scripts.) Then when saving, allow me to save scripts individually, save
// entities and then compile entities to bsp." shank: "das man halt alles
// anbietet entweder direkt in der Pk3 file oder die bsp files oeffnen".
//
//   Punkt 1  Herkunft je Datei (DateiHerkunft): lose Datei oder .pk3 + Eintrag
//   Punkt 2  getrennte Befehle: Skript speichern (Strg+S je Reiter), Entities
//            speichern (.ent), Entities in die BSP uebernehmen (wie -onlyents)
//   Punkt 3  Datei aus einer .pk3: in der .pk3 aendern, lose im Mod-Ordner,
//            Speichern unter, bisheriger Missionsordner-Weg - waehlbar, merkbar
//   Punkt 5  jedes Schreiben ueber sicherSchreiben (Temp-Datei, atomar), bei
//            einem Fehler keine Meldung "gespeichert"
//   F3/F4    Karte neu laden fragt bei ungespeicherten Entities und leert den
//            Verlauf; ".ent oeffnen" raeumt Verlauf, Auswahl und Skriptliste
#include "app_internal.h"

#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/bsplumps.h"
#include "bhed/missionbau.h"
#include "bhed/pk3.h"
#include "bhed/sicherschreiben.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include "bhed/utf8pfad.h"

namespace bhed::gui {

// app_mission.cpp
std::string missionBefundText(const MissionBefund& b);
bool missionsWegFuerDatei(const std::string& eintrag, const std::string& bytes);

namespace {

std::string kleinD(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool endetAufD(const std::string& s, const char* e) {
    const std::string k = kleinD(s);
    const std::string x(e);
    return k.size() >= x.size() && k.compare(k.size() - x.size(), x.size(), x) == 0;
}
std::string andereEndung(const std::string& p, const char* neu) {
    const std::size_t punkt = p.find_last_of('.');
    const std::size_t strich = p.find_last_of("/\\");
    const bool hat = punkt != std::string::npos && (strich == std::string::npos || punkt > strich);
    return (hat ? p.substr(0, punkt) : p) + neu;
}
std::string verbinden(const std::string& ordner, const std::string& eintrag) {
    std::string r = ordner;
    if (!r.empty() && r.back() != '\\' && r.back() != '/') { r += '\\'; }
    for (const char c : eintrag) { r += c == '/' ? '\\' : c; }
    return r;
}
bool gibtEsD(const std::string& p) {
    std::error_code ec;
    return !p.empty() && std::filesystem::exists(::bhed::utf8Pfad(p), ec);
}
std::string fmt(const char* muster, const std::string& a, const std::string& b = std::string(), const std::string& c = std::string()) {
    char t[1600];
    std::snprintf(t, sizeof(t), muster, a.c_str(), b.c_str(), c.c_str());
    return t;
}
std::string anzeigeVon(const DateiHerkunft& h) { return h.eintrag + "  (" + fileName(h.pfad) + ")"; }

// Die alte Datei als <pfad>.bak (Stand vor diesem Speichern).
bool sicherungAnlegenD(const std::string& pfad) {
    if (!gibtEsD(pfad)) { return true; }
    std::error_code ec;
    std::filesystem::copy_file(::bhed::utf8Pfad(pfad), ::bhed::utf8Pfad(pfad + ".bak"), std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        meldung(fmt(tr(Str::DoNichtGeschrieben), pfad + ".bak"), directoryOf(pfad), true);
        return false;
    }
    return true;
}

// F8 vor jedem Schreiben der Entities.
bool grenzenOk(const std::vector<MapEntity>& ents) {
    std::vector<MissionBefund> b;
    entityGrenzenPruefen(ents, b);
    if (b.empty()) { return true; }
    meldung(missionBefundText(b.front()), std::string(), true);
    diag::warn("Entity-Grenze: " + b.front().code + " " + b.front().arg);
    return false;
}

std::vector<karte::EntitaetPaare> paareVon(const std::vector<MapEntity>& ents) {
    std::vector<karte::EntitaetPaare> v;
    v.reserve(ents.size());
    for (const MapEntity& e : ents) { v.push_back(e.keys); }
    return v;
}

// Die Datei, in die "Entities speichern" schreibt (Art::Keine: Speichern unter).
DateiHerkunft entZiel() {
    if (g_app->entHerkunftKarte == g_app->map.path && g_app->entHerkunft.art != DateiHerkunft::Art::Keine) { return g_app->entHerkunft; }
    const DateiHerkunft& k = g_app->kartenHerkunft;
    if (k.art == DateiHerkunft::Art::Lose) { return DateiHerkunft::lose(andereEndung(k.pfad, ".ent")); }
    if (k.art == DateiHerkunft::Art::Pk3) { return DateiHerkunft::ausPk3(k.pfad, andereEndung(k.eintrag, ".ent")); }
    // Neue Mission: ihr Ordner ist der Ort ihrer Dateien.
    if (g_app->missionAktiv && g_app->map.path == g_app->mission.karte && !g_app->mission.projektOrdner.empty()) {
        return DateiHerkunft::lose(verbinden(g_app->mission.projektOrdner, "maps/" + g_app->mission.name + ".ent"));
    }
    DateiHerkunft h;
    h.eintrag = andereEndung(g_app->map.path, ".ent");
    return h;
}

void entGeschrieben(const std::string& karte, const DateiHerkunft& h) {
    if (g_app->map.path == karte) {
        g_app->entHerkunft = h;
        g_app->entHerkunftKarte = karte;
        merkeEntities();
    }
    g_app->entitiesUngespeichert.erase(karte);
}

}  // namespace

// Bytes eines Eintrags aus einer bestimmten .pk3.
bool ausPk3Lesen(const std::string& pk3, const std::string& eintrag, std::string& out) {
    Pk3 a;
    if (!readPk3Directory(pk3, a)) { return false; }
    const Pk3Entry* e = a.find(eintrag);
    return e != nullptr && readPk3File(a, *e, out);
}

void entVerlaufLeeren() {
    g_app->entUndo.clear();
    g_app->entRedo.clear();
    g_app->entUndoStempel.clear();
    g_app->entRedoStempel.clear();
}

bool dateiSicherSchreiben(const std::string& pfad, const std::string& bytes) {
    std::string fehler;
    SchreibFehler art = SchreibFehler::Keiner;
    if (sicherSchreiben(pfad, bytes, &fehler, &art)) {
        diag::detail("Gespeichert: " + pfad + " (" + std::to_string(bytes.size()) + " Bytes)");
        return true;
    }
    diag::warn("Speichern gescheitert: " + fehler);
    meldung(fmt(tr(art == SchreibFehler::Gesperrt ? Str::DoGesperrt : Str::DoNichtGeschrieben), pfad), directoryOf(pfad), true);
    return false;
}

bool bspSicherungUndSchreiben(const std::string& pfad, const std::string& bytes) {
    return sicherungAnlegenD(pfad) && dateiSicherSchreiben(pfad, bytes);
}

bool aufWegSchreiben(int weg, const DateiHerkunft& h, const std::string& bytes, DateiHerkunft* neu) {
    const bool bsp = endetAufD(h.eintrag.empty() ? h.pfad : h.eintrag, ".bsp");
    if (weg == 1 && h.art == DateiHerkunft::Art::Pk3) {
        std::string fehler;
        SchreibFehler art = SchreibFehler::Keiner;
        if (!pk3EintragErsetzen(h.pfad, h.eintrag, bytes, &fehler, &art)) {
            diag::warn("In .pk3 speichern gescheitert: " + fehler);
            meldung(fmt(tr(art == SchreibFehler::Gesperrt ? Str::DoGesperrt : Str::DoNichtGeschrieben), h.pfad), directoryOf(h.pfad), true);
            return false;
        }
        diag::info("In .pk3 gespeichert: " + h.eintrag + " in " + h.pfad);
        meldung(fmt(tr(Str::DoInPk3Gespeichert), h.eintrag, h.pfad, fileName(h.pfad)), directoryOf(h.pfad));
        if (neu != nullptr) { *neu = h; }
        return true;
    }
    std::string ziel;
    if (weg == 2 && h.art == DateiHerkunft::Art::Pk3) {
        ziel = verbinden(directoryOf(h.pfad), h.eintrag);
    } else if (weg == 3) {
        const std::string name = fileName(h.eintrag.empty() ? h.pfad : h.eintrag);
        const std::string start = h.art == DateiHerkunft::Art::Keine ? g_app->settings.lastDir : directoryOf(h.pfad);
        ziel = platform::saveFileDialog(tr(Str::DoWegUnter), bsp ? "JKA maps (*.bsp)|*.bsp|All files (*.*)|*.*"
                                                                 : "Entity files (*.ent)|*.ent|All files (*.*)|*.*",
                                        start, name);
        if (ziel.empty()) { return false; }
    } else {
        return false;
    }
    if (bsp && !sicherungAnlegenD(ziel)) { return false; }
    if (!dateiSicherSchreiben(ziel, bytes)) { return false; }
    meldung(fmt(tr(Str::DoGespeichert), ziel), directoryOf(ziel));
    if (neu != nullptr) { *neu = DateiHerkunft::lose(ziel); }
    return true;
}

void speicherWegAnbieten(App::SpeicherAuftrag auftrag) {
    int weg = g_app->settings.missionsordnerAblauf ? 4 : g_app->settings.pk3SpeicherWeg;
    if (weg == 4 && !auftrag.missionsWeg) { weg = 0; }
    if (weg >= 1 && weg <= 4) {
        diag::detail("Speicherweg " + std::to_string(weg) + " (gemerkt) fuer " + auftrag.anzeige);
        auftrag.ausfuehren(weg);
        return;
    }
    g_app->speicherFragen.push_back(std::move(auftrag));
}

void drawSpeicherWeg() {
    if (g_app->speicherFragen.empty()) { return; }
    const std::string titel = std::string(tr(Str::DoSpeicherWegTitel)) + "###speicherweg";
    if (!ImGui::IsPopupOpen("###speicherweg")) {
        ImGui::OpenPopup("###speicherweg");
        g_app->speicherWegMerken = false;
    }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 36.0F, 0.0F}, ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal(titel.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }
    const App::SpeicherAuftrag& a = g_app->speicherFragen.front();
    const float breite = ImGui::GetFontSize() * 35.0F;
    ImGui::PushTextWrapPos(breite);
    ImGui::TextWrapped("%s", fmt(tr(Str::DoSpeicherWegText), a.anzeige).c_str());
    ImGui::Spacing();
    int gewaehlt = 0;
    const auto weg = [&](int nr, Str knopf, const std::string& hinweis, bool moeglich) {
        ImGui::BeginDisabled(!moeglich);
        if (ImGui::Button(tr(knopf), ImVec2{breite, 0.0F})) { gewaehlt = nr; }
        ImGui::EndDisabled();
        ImGui::Indent();
        ImGui::TextDisabled("%s", hinweis.c_str());
        ImGui::Unindent();
        ImGui::Spacing();
    };
    const std::string lose = verbinden(directoryOf(a.herkunft.pfad), a.herkunft.eintrag);
    weg(1, Str::DoWegPk3, fmt(tr(Str::DoWegPk3Hint), fileName(a.herkunft.pfad)), true);
    weg(2, Str::DoWegLose, fmt(tr(Str::DoWegLoseHint), lose), true);
    weg(3, Str::DoWegUnter, tr(Str::DoWegUnterHint), true);
    weg(4, Str::DoWegMission, tr(a.missionsWeg ? Str::DoWegMissionHint : Str::DoWegMissionNicht), a.missionsWeg);
    ImGui::Checkbox(tr(Str::DoWegMerken), &g_app->speicherWegMerken);
    ImGui::PopTextWrapPos();
    ImGui::Separator();
    const bool abbrechen = ImGui::Button(tr(Str::MissionCancel)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
    if (gewaehlt != 0 || abbrechen) {
        App::SpeicherAuftrag x = std::move(g_app->speicherFragen.front());
        g_app->speicherFragen.erase(g_app->speicherFragen.begin());
        if (g_app->speicherFragen.empty()) { ImGui::CloseCurrentPopup(); }
        if (gewaehlt != 0) {
            if (g_app->speicherWegMerken) { g_app->settings.pk3SpeicherWeg = gewaehlt; }
            diag::detail("Speicherweg " + std::to_string(gewaehlt) + " gewaehlt fuer " + x.anzeige);
            x.ausfuehren(gewaehlt);
        } else {
            diag::detail("Speichern abgebrochen: " + x.anzeige);
        }
    }
    ImGui::EndPopup();
}

// --- Oeffnen mit Herkunft -------------------------------------------------------------

bool karteAusPk3Nachladen(const std::string& pk3, const std::string& eintrag) {
    std::string data;
    if (!ausPk3Lesen(pk3, eintrag, data)) { return false; }
    loadMapFromBytes(data, eintrag);
    // Eine Karte im Archiv hat keinen Pfad auf der Platte.
    g_app->settings.mapPath.clear();
    g_app->kartenHerkunft = DateiHerkunft::ausPk3(pk3, eintrag);
    return true;
}

bool karteAusArchivOeffnen(const std::string& pk3, const std::string& eintrag) {
    diag::Step step("Karte aus Archiv: " + eintrag + " (" + pk3 + ")");
    vergissEntities(eintrag);
    if (!karteAusPk3Nachladen(pk3, eintrag)) {
        meldung(fmt(tr(Str::DoNichtGelesen), eintrag, pk3), directoryOf(pk3), true);
        return false;
    }
    entVerlaufLeeren();
    return true;
}

bool skriptAusArchivOeffnen(const std::string& pk3, const std::string& eintrag) {
    std::string data;
    if (!ausPk3Lesen(pk3, eintrag, data)) {
        meldung(fmt(tr(Str::DoNichtGelesen), eintrag, pk3), directoryOf(pk3), true);
        return false;
    }
    openScriptFromMemory(data, eintrag, DateiHerkunft::ausPk3(pk3, eintrag));
    return true;
}

// --- Punkt 2: getrennte Befehle ----------------------------------------------------------

bool entitiesSpeichern() {
    if (g_app->map.empty()) {
        meldung(tr(Str::DoKeineKarte), std::string(), true);
        return false;
    }
    if (!grenzenOk(g_app->map.entities)) { return false; }
    // rc609 W2b: der Text des Entity-Texteditors - Kommentare, Leerzeilen und Reihenfolge
    // bleiben, nur geaenderte Zeilen sind neu (ohne Quelle wie writeEntities).
    const std::string text = entTextFuerSpeichern();
    const std::string karte = g_app->map.path;
    const DateiHerkunft h = entZiel();
    if (h.art == DateiHerkunft::Art::Lose) {
        if (!dateiSicherSchreiben(h.pfad, text)) { return false; }
        entGeschrieben(karte, h);
        meldung(fmt(tr(Str::DoGespeichert), h.pfad), directoryOf(h.pfad));
        return true;
    }
    if (h.art == DateiHerkunft::Art::Keine) {
        DateiHerkunft neu;
        if (!aufWegSchreiben(3, h, text, &neu)) { return false; }
        entGeschrieben(karte, neu);
        return true;
    }
    App::SpeicherAuftrag a;
    a.anzeige = anzeigeVon(h);
    a.herkunft = h;
    a.missionsWeg = g_app->missionAktiv;
    a.ausfuehren = [text, karte, h](int weg) {
        if (weg == 4) {
            if (missionsWegFuerDatei(h.eintrag, text)) { g_app->entitiesUngespeichert.erase(karte); }
            return;
        }
        DateiHerkunft neu;
        if (aufWegSchreiben(weg, h, text, &neu)) { entGeschrieben(karte, neu); }
    };
    speicherWegAnbieten(std::move(a));
    return false;
}

bool entitiesInBspUebernehmen() {
    if (g_app->map.empty()) {
        meldung(tr(Str::DoKeineKarte), std::string(), true);
        return false;
    }
    if (!grenzenOk(g_app->map.entities)) { return false; }
    const DateiHerkunft k = g_app->kartenHerkunft;
    std::string original;
    if (k.art == DateiHerkunft::Art::Lose) {
        original = slurp(k.pfad);
    } else if (k.art == DateiHerkunft::Art::Pk3) {
        (void)ausPk3Lesen(k.pfad, k.eintrag, original);
    } else {
        (void)kartenBytes(g_app->map.path, original);
    }
    std::string fehler;
    const std::string fertig = original.empty() ? std::string() : karte::bspNurEntitaetenEinsetzen(original, paareVon(g_app->map.entities), &fehler);
    if (fertig.empty()) {
        meldung(fmt(tr(Str::DoBspFehler), g_app->map.path, fehler), std::string(), true);
        return false;
    }
    const std::string karte = g_app->map.path;
    // Liegt eine .ent daneben, nimmt das Spiel deren Entities (CM_LoadMap: OJK
    // code/qcommon/cm_load.cpp:470-483) - sagen.
    const auto nachher = [karte](const DateiHerkunft& ziel) {
        g_app->entitiesUngespeichert.erase(karte);
        const std::string ent = ziel.art == DateiHerkunft::Art::Lose ? andereEndung(ziel.pfad, ".ent") : std::string();
        if (!ent.empty() && gibtEsD(ent)) { meldung(fmt(tr(Str::DoEntDaneben), ent), directoryOf(ent), true); }
        if (ziel.art == DateiHerkunft::Art::Pk3) {
            std::string schon;
            if (ausPk3Lesen(ziel.pfad, andereEndung(ziel.eintrag, ".ent"), schon)) {
                meldung(fmt(tr(Str::DoEntDaneben), andereEndung(ziel.eintrag, ".ent")), directoryOf(ziel.pfad), true);
            }
        }
    };
    if (k.art == DateiHerkunft::Art::Lose) {
        if (!sicherungAnlegenD(k.pfad) || !dateiSicherSchreiben(k.pfad, fertig)) { return false; }
        diag::info("Entities in die BSP uebernommen: " + k.pfad + " (" + std::to_string(fertig.size()) + " Bytes)");
        meldung(fmt(tr(Str::DoBspGespeichert), k.pfad, k.pfad), directoryOf(k.pfad));
        nachher(k);
        return true;
    }
    if (k.art == DateiHerkunft::Art::Keine) {
        DateiHerkunft h;
        h.eintrag = karte;
        DateiHerkunft neu;
        if (!aufWegSchreiben(3, h, fertig, &neu)) { return false; }
        nachher(neu);
        return true;
    }
    App::SpeicherAuftrag a;
    a.anzeige = anzeigeVon(k);
    a.herkunft = k;
    a.missionsWeg = false;   // der Missionsordner kennt keine eigene .bsp
    a.ausfuehren = [fertig, k, nachher](int weg) {
        DateiHerkunft neu;
        if (!aufWegSchreiben(weg, k, fertig, &neu)) { return; }
        if (weg != 1) { g_app->kartenHerkunft = neu; }
        nachher(neu);
    };
    speicherWegAnbieten(std::move(a));
    return false;
}

// rc609 J: die bearbeiteten Shader-, Fog- und Flaechen-Lumps in die .bsp, auf denselben Wegen wie
// "Entities in die BSP uebernehmen": die Datei am Ort der Karte (darin evtl. schon neue Entities)
// bekommt die Lumps, alle anderen bleiben Byte fuer Byte; vorher x.bsp.bak bzw. <pk3>.bak.
bool bspLumpsSpeichern(const karte::BspLumps& lumps, const std::function<void()>& gespeichert) {
    if (g_app->map.empty()) {
        meldung(tr(Str::DoKeineKarte), std::string(), true);
        return false;
    }
    const DateiHerkunft k = g_app->kartenHerkunft;
    std::string original;
    if (k.art == DateiHerkunft::Art::Lose) {
        original = slurp(k.pfad);
    } else if (k.art == DateiHerkunft::Art::Pk3) {
        (void)ausPk3Lesen(k.pfad, k.eintrag, original);
    } else {
        (void)kartenBytes(g_app->map.path, original);
    }
    std::string fehler;
    const std::string fertig = original.empty() ? std::string() : karte::bspLumpsSchreiben(original, lumps, &fehler);
    if (fertig.empty()) {
        meldung(fmt(tr(Str::DoBspFehler), g_app->map.path, fehler), std::string(), true);
        return false;
    }
    if (k.art == DateiHerkunft::Art::Lose) {
        if (!bspSicherungUndSchreiben(k.pfad, fertig)) { return false; }
        diag::info("Lumps in die BSP gespeichert: " + k.pfad + " (" + std::to_string(fertig.size()) + " Bytes)");
        meldung(fmt(tr(Str::DoBspGespeichert), k.pfad, k.pfad), directoryOf(k.pfad));
        gespeichert();
        return true;
    }
    if (k.art == DateiHerkunft::Art::Keine) {
        DateiHerkunft h;
        h.eintrag = g_app->map.path;
        if (!aufWegSchreiben(3, h, fertig, nullptr)) { return false; }
        gespeichert();
        return true;
    }
    App::SpeicherAuftrag a;
    a.anzeige = anzeigeVon(k);
    a.herkunft = k;
    a.missionsWeg = false;   // der Missionsordner kennt keine eigene .bsp
    a.ausfuehren = [fertig, k, gespeichert](int weg) {
        DateiHerkunft neu;
        if (!aufWegSchreiben(weg, k, fertig, &neu)) { return; }
        if (weg != 1) { g_app->kartenHerkunft = neu; }
        diag::info("Lumps in die BSP gespeichert (Weg " + std::to_string(weg) + "): " + anzeigeVon(neu));
        gespeichert();
    };
    speicherWegAnbieten(std::move(a));
    return false;
}

// --- F3 / F4 --------------------------------------------------------------------------

void karteDateiLaden(const std::string& p) {
    const auto laden = [p] {
        vergissEntities(p);
        lumpEditVergessen(p);   // rc609 J: die bearbeiteten Lumps galten der verworfenen Fassung
        loadMap(p);
        // F3: frisch von der Platte - der Verlauf der verworfenen Fassung gilt nicht mehr
        // (Strg+Z setzte sonst ihren halben Stand wieder ein).
        entVerlaufLeeren();
        entityNummernVergessen();
        g_app->entitiesUngespeichert.erase(p);
    };
    if (g_app->entitiesUngespeichert.count(p) != 0U || lumpEditUngespeichert(p)) {
        if (!g_app->frageOffen) { frage(fmt(tr(Str::DoKarteNeuFrage), fileName(p)), laden, [] {}); }
        return;
    }
    laden();
}

bool entDateiLaden(const std::string& p) {
    diag::Step step("Entities laden: " + p);
    const std::string text = slurp(p);
    if (text.empty()) {
        step.fail("nicht lesbar");
        meldung(fmt(tr(Str::DoNichtGeschrieben), p), directoryOf(p), true);
        return false;
    }
    std::vector<MapEntity> ents;
    parseEntities(text, ents);
    if (ents.empty()) {
        step.fail("keine Entities gefunden");
        return false;
    }
    g_app->map.entities = std::move(ents);
    entTextQuelle(g_app->map.path, text);   // rc609 W2b
    // F4: Namen, Kameragruppen UND Skriptverweise neu (collectNames); Nummern, die in
    // die alte Liste zeigten (Mehrfachauswahl, Wegpunkt-Kette, Animation), und der
    // Verlauf der alten Entities gelten nicht mehr.
    collectNames(g_app->map);
    entityNummernVergessen();
    entVerlaufLeeren();
    g_app->entitiesUngespeichert.erase(g_app->map.path);
    merkeEntities();
    g_app->entHerkunft = DateiHerkunft::lose(p);
    g_app->entHerkunftKarte = g_app->map.path;
    g_app->settings.lastDir = directoryOf(p);
    g_app->mapDirty = true;
    // Alles, was an den ALTEN Entities hing, gilt nicht mehr: die gewaehlte
    // und die ausgeblendeten Entities stehen als Index da, und die
    // Kamerabahn loest ref_tags ueber die Entities auf - neu aufbauen
    // (Kartentest 27.09.; vorher blieb alles auf dem alten Stand).
    g_app->pickedEntity = -1;
    g_app->hiddenEntities.clear();
    g_app->camTrackValid = false;
    char msg[200];
    std::snprintf(msg, sizeof(msg), trN(Str::EntLoaded, g_app->map.entities.size()), static_cast<int>(g_app->map.entities.size()));
    addStatus(msg);
    return true;
}

}  // namespace bhed::gui
