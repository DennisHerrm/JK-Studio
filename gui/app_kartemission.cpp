// app_kartemission.cpp - rc609 B2: Mission <-> Map ohne Verlust
//
// shank (03.11.): "wenn ich merke in der Mission es fehlt einen Bereich in der
// map. das ich rueber Switchen kann zu map den kurz adden und danach weiter
// machen kann an der Mission."
//
//   Punkt 2  "Karte der Mission bearbeiten" im Map-Tab (File): oeffnet die Quelle
//            (<karte>.map neben einer losen .bsp), sonst die .bsp ueber "Open .bsp"
//            (mit Editor-Daten .bhed, sonst zurueckgewandelt). Der Bau schreibt in
//            genau die Datei bzw. den Eintrag der .pk3 der Mission (Herkunft, W2a).
//   Punkt 3  nach dem Bau laedt der Mission-Tab nach: Geometrie, Licht, Kollision,
//            Navigation, Entities (Abgleich nach bhed/kartemission.h), Vorschau. Er
//            behaelt Skripte, eigene Entities, Kamera, Zeitleiste und Auswahl; was an
//            Bezuegen bricht, steht in einer Liste.
#include "app_internal.h"
#include "tabfenster.h"   // rc609 T1
#include "karteneditor.h"

#include "bhed/kartemission.h"
#include "bhed/karte/editordaten.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <string>
#include <system_error>
#include <vector>
#include "bhed/utf8pfad.h"

namespace bhed::gui {

namespace ke = karteneditor;

namespace {

// Die Verbindung Map-Tab -> Mission (eine zur Zeit).
struct Verbindung {
    bool aktiv = false;
    std::string kennung;        // MapData::path der Mission
    DateiHerkunft herkunft;     // wo ihre .bsp liegt (W2a)
    std::string karteDatei;     // die Karte im Map-Tab (Editor::name)
    std::string alteBytes;      // die .bsp der Mission vor dem Bau
    std::string bauBsp;         // was q3map2 baut
};
Verbindung g_v;
KarteMissionStand g_stand;

std::string fmt(const char* muster, const std::string& a, const std::string& b = std::string()) {
    char t[1600];
    std::snprintf(t, sizeof(t), muster, a.c_str(), b.c_str());
    return t;
}

std::string andereEndung(const std::string& p, const char* neu) {
    const std::size_t punkt = p.find_last_of('.');
    const std::size_t strich = p.find_last_of("/\\");
    const bool hat = punkt != std::string::npos && (strich == std::string::npos || punkt > strich);
    return (hat ? p.substr(0, punkt) : p) + neu;
}

bool gibtEs(const std::string& p) {
    std::error_code ec;
    return !p.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(p), ec);
}

bool gleicheDatei(const std::string& a, const std::string& b) {
    std::error_code ec;
    return gibtEs(a) && gibtEs(b) && std::filesystem::equivalent(::bhed::utf8Pfad(a), ::bhed::utf8Pfad(b), ec);
}

// Die .bsp der Mission, so wie sie auf der Platte bzw. in der .pk3 liegt.
bool missionsBsp(const DateiHerkunft& k, const std::string& kennung, std::string& out) {
    out.clear();
    if (k.art == DateiHerkunft::Art::Lose) {
        out = slurp(k.pfad);
    } else if (k.art == DateiHerkunft::Art::Pk3) {
        (void)ausPk3Lesen(k.pfad, k.eintrag, out);
    } else {
        (void)kartenBytes(kennung, out);
    }
    return !out.empty();
}

// Die Namen, die die offenen Skripte ansprechen (alle Reiter).
std::vector<SkriptVerweis> skriptVerweise() {
    std::vector<SkriptVerweis> v;
    const auto name = [](const std::string& pfad, const std::string& skript) { return !skript.empty() ? skript : fileName(pfad); };
    skriptVerweiseSammeln(g_app->doc.script(), name(g_app->path, g_app->skriptPfad), v);
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        if (static_cast<int>(i) == g_app->activeTab) { continue; }
        const App::Parked& t = g_app->tabs[i];
        skriptVerweiseSammeln(t.doc.script(), name(t.path, t.skriptPfad), v);
    }
    return v;
}

bool gleicheEntities(const std::vector<MapEntity>& a, const std::vector<MapEntity>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].keys != b[i].keys) { return false; }
    }
    return true;
}

std::string befundArt(const KarteMissionBefund& b) {
    switch (b.art) {
    case KarteMissionBefund::Art::NameFehlt: return tr(Str::KmArtNameFehlt);
    case KarteMissionBefund::Art::ModellFehlt: return tr(Str::KmArtModellFehlt);
    case KarteMissionBefund::Art::ModellVerschoben: return tr(Str::KmArtModellVerschoben);
    case KarteMissionBefund::Art::ModellAngepasst: return fmt(tr(Str::KmArtModellAngepasst), b.neu);
    case KarteMissionBefund::Art::EntityNichtInKarte: return tr(Str::KmArtEntityWeg);
    }
    return {};
}

// Punkt 3: die neue .bsp in den Mission-Tab, ohne seinen Stand zu verlieren.
void missionNachladen(const std::string& neu) {
    const std::string kennung = g_v.kennung;
    MapData nd;
    MapData od;
    if (!readBsp(neu, nd)) { return; }
    if (!readBsp(g_v.alteBytes, od)) { od.entities.clear(); }
    const bool angezeigt = g_app->map.path == kennung;
    const auto gemerkt = g_app->entitiesJeKarte.find(kennung);
    // Entities der Mission: die angezeigten, sonst die gemerkten, sonst die der alten Karte
    const std::vector<MapEntity> mission = angezeigt ? g_app->map.entities : gemerkt != g_app->entitiesJeKarte.end() ? gemerkt->second : od.entities;
    KarteMissionErgebnis erg = karteMissionAbgleich(od.entities, bspModellGrenzen(g_v.alteBytes), mission, nd.entities, bspModellGrenzen(neu), skriptVerweise());
    const bool eigene = !gleicheEntities(erg.entities, nd.entities);
    const bool warUngespeichert = g_app->entitiesUngespeichert.count(kennung) != 0U;
    // rc609 J: bearbeitete Lumps galten den Flaechen der alten Fassung
    if (lumpEditUngespeichert(kennung)) { meldung(fmt(tr(Str::KmLumpsVerworfen), kennung), std::string(), true); }
    lumpEditVergessen(kennung);
    if (eigene) {
        g_app->entitiesJeKarte[kennung] = erg.entities;   // loadMapFromBytes nimmt sie (wie beim Reiterwechsel)
    } else {
        g_app->entitiesJeKarte.erase(kennung);
    }
    if (angezeigt) {
        // Was der Mission-Tab behaelt: Kamera, Zeitleiste, Auswahl, Herkunft der Dateien
        const Camera cam = g_app->cam;
        const Orbit orbit = g_app->orbit;
        const double playMs = g_app->playMs;
        const double tlStart = g_app->timelineViewStartMs;
        const double tlZoom = g_app->timelineZoom;
        const std::vector<int> auswahl = entityAuswahl();
        const bool kameraGewaehlt = g_app->cameraSelected;
        const int schluessel = g_app->selectedKey;
        const std::set<std::size_t> versteckt = g_app->hiddenEntities;
        const int anim = g_app->animEntity;
        const bool animOffen = g_app->animFensterOffen;
        const std::string mapPfad = g_app->settings.mapPath;
        const DateiHerkunft entHerkunft = g_app->entHerkunft;
        const std::string entHerkunftKarte = g_app->entHerkunftKarte;
        loadMapFromBytes(neu, kennung);
        g_app->kartenHerkunft = g_v.herkunft;
        g_app->settings.mapPath = mapPfad;
        g_app->entHerkunft = entHerkunft;
        g_app->entHerkunftKarte = entHerkunftKarte;
        g_app->cam = cam;
        g_app->orbit = orbit;
        g_app->playMs = playMs;
        g_app->timelineViewStartMs = tlStart;
        g_app->timelineZoom = tlZoom;
        const auto neuNr = [&erg](int alt) {
            return alt >= 0 && static_cast<std::size_t>(alt) < erg.neuVonMission.size() ? erg.neuVonMission[static_cast<std::size_t>(alt)] : -1;
        };
        g_app->pickedEntity = auswahl.empty() ? -1 : neuNr(auswahl.front());
        for (std::size_t i = 1; i < auswahl.size(); ++i) {
            if (const int n = neuNr(auswahl[i]); n >= 0) { g_app->entAuswahl.push_back(n); }
        }
        g_app->cameraSelected = kameraGewaehlt;
        g_app->selectedKey = g_app->pickedEntity >= 0 ? schluessel : -1;
        for (const std::size_t h : versteckt) {
            if (const int n = neuNr(static_cast<int>(h)); n >= 0) { g_app->hiddenEntities.insert(static_cast<std::size_t>(n)); }
        }
        g_app->animEntity = neuNr(anim);
        g_app->animFensterOffen = animOffen && g_app->animEntity >= 0;
        // Wie "Karte neu laden" (W2a F3): der Verlauf zaehlte die Nummern der alten Fassung
        entVerlaufLeeren();
    }
    if (eigene || warUngespeichert) {
        g_app->entitiesUngespeichert.insert(kennung);
    } else {
        g_app->entitiesUngespeichert.erase(kennung);
    }
    g_v.alteBytes = neu;
    std::string gebrochen;
    for (const KarteMissionBefund& b : erg.befunde) {
        if (b.art == KarteMissionBefund::Art::ModellAngepasst) { continue; }
        if (gebrochen.find(b.was) == std::string::npos) { gebrochen += (gebrochen.empty() ? "" : ", ") + b.was; }
    }
    diag::info("Karte der Mission nachgeladen: " + kennung + " - " + std::to_string(erg.entities.size()) + " Entities, unveraendert " +
               std::to_string(erg.unveraendert) + ", geaendert " + std::to_string(erg.geaendert) + ", eigene " + std::to_string(erg.eigeneAngehaengt) +
               " angehaengt / " + std::to_string(erg.eigeneSchonDa) + " schon da, entfernt " + std::to_string(erg.entfernt) + ", Befunde " +
               std::to_string(erg.befunde.size()) + (gebrochen.empty() ? std::string() : " (" + gebrochen + ")"));
    for (const KarteMissionBefund& b : erg.befunde) { diag::detail("Karte der Mission: " + befundArt(b) + " - " + b.was + " - " + b.wo); }
    meldung(fmt(tr(Str::KmNachgeladen), fileName(kennung)), std::string());
    if (!gebrochen.empty()) { meldung(fmt(tr(Str::KmGebrochen), gebrochen), std::string(), true); }
    g_stand.befunde = erg.befunde;
    g_stand.zusammenfassung = {erg.unveraendert, erg.geaendert, erg.eigeneAngehaengt, erg.eigeneSchonDa, erg.entfernt};
    g_stand.fensterOffen = !erg.befunde.empty();
    ++g_stand.nachgeladen;
}

}  // namespace

const KarteMissionStand& karteMissionStand() { return g_stand; }

bool karteDerMissionMoeglich() { return !g_app->map.empty(); }

std::string karteDerMissionName() { return g_app->map.empty() ? std::string() : fileName(g_app->map.path); }

// Punkt 2
bool karteDerMissionBearbeiten() {
    if (g_app->map.empty()) {
        meldung(tr(Str::KmKeineMission), std::string(), true);
        return false;
    }
    const DateiHerkunft k = g_app->kartenHerkunft;
    const std::string kennung = g_app->map.path;
    std::string bytes;
    if (!missionsBsp(k, kennung, bytes)) {
        meldung(fmt(tr(Str::KmNichtGelesen), kennung), std::string(), true);
        return false;
    }
    // Die Quelle neben einer losen .bsp
    const std::string quelle = k.art == DateiHerkunft::Art::Lose && gibtEs(andereEndung(k.pfad, ".map")) ? andereEndung(k.pfad, ".map") : std::string();
    ke::rueckfrageDann("Open Map", [k, kennung, bytes, quelle] {
        bool ok = false;
        if (!quelle.empty()) {
            ke::zuletztEintragen(quelle);   // wie File > Open (MRU_AddFile vor dem Laden)
            ok = ke::karteOeffnen(quelle);
        } else if (k.art == DateiHerkunft::Art::Lose) {
            ok = ke::bspOeffnenDatei(k.pfad);   // mit <karte>.bhed daneben die genaue Quelle
        } else {
            std::string begleit;
            const std::string eintrag = k.art == DateiHerkunft::Art::Pk3 ? k.eintrag : kennung;
            const std::string begleitName = karte::editorDatenPfad(eintrag);
            if (k.art == DateiHerkunft::Art::Pk3) { (void)ausPk3Lesen(k.pfad, begleitName, begleit); }
            ok = ke::bspOeffnenAus(eintrag, bytes, begleitName, begleit);
        }
        if (!ok) { return; }
        g_v = Verbindung{};
        g_v.aktiv = true;
        g_v.kennung = kennung;
        g_v.herkunft = k;
        g_v.karteDatei = ke::editor().name;
        g_v.alteBytes = bytes;
        const std::string ziel = k.art == DateiHerkunft::Art::Pk3 ? k.eintrag + "  (" + k.pfad + ")" : k.art == DateiHerkunft::Art::Lose ? k.pfad : kennung;
        ke::ausgabe(fmt(ke::trK(Str::KmVerbunden), fileName(kennung), ziel));
        diag::info("Karte der Mission bearbeiten: " + kennung + " -> " + g_v.karteDatei);
    });
    return true;
}

void karteMissionBauBeginnt(const std::string& karteDatei, const std::string& bsp) {
    if (!g_v.aktiv || karteDatei != g_v.karteDatei) { return; }
    g_v.bauBsp = bsp;
    // Der Stand von jetzt: die Mission kann inzwischen Entities in die .bsp uebernommen haben
    std::string jetzt;
    if (missionsBsp(g_v.herkunft, g_v.kennung, jetzt)) { g_v.alteBytes = jetzt; }
}

void karteMissionBauFertig(const std::string& karteDatei, bool ok) {
    if (!g_v.aktiv || karteDatei != g_v.karteDatei || g_v.bauBsp.empty()) { return; }
    const std::string bsp = g_v.bauBsp;
    g_v.bauBsp.clear();
    const std::string neu = ok ? slurp(bsp) : std::string();
    if (neu.empty() || neu == g_v.alteBytes) {
        if (!ok || neu.empty()) { meldung(fmt(tr(Str::KmBauOhneBsp), bsp), directoryOf(bsp), true); }
        return;
    }
    // In die Datei der Mission (W2a: vorher .bak)
    DateiHerkunft& k = g_v.herkunft;
    bool geschrieben = false;
    if (k.art == DateiHerkunft::Art::Lose) {
        geschrieben = gleicheDatei(k.pfad, bsp) ? dateiSicherSchreiben(k.pfad + ".bak", g_v.alteBytes) : bspSicherungUndSchreiben(k.pfad, neu);
    } else if (k.art == DateiHerkunft::Art::Pk3) {
        geschrieben = aufWegSchreiben(1, k, neu, nullptr);
    } else {
        // Keine eigene Datei (neue Mission auf einer Grundkarte): in ihren Missionsordner, sonst bleibt sie, wo sie gebaut wurde
        const App::MissionsProjekt& m = g_app->mission;
        std::string ziel = bsp;
        if (g_app->missionAktiv && m.karte == g_v.kennung && !m.projektOrdner.empty()) {
            ziel = (std::filesystem::path(::bhed::utf8Pfad(m.projektOrdner)) / "maps" / (m.name + ".bsp")).string();
            std::error_code ec;
            std::filesystem::create_directories(::bhed::utf8Pfad(directoryOf(ziel)), ec);
            geschrieben = bspSicherungUndSchreiben(ziel, neu);
            if (geschrieben) { k = DateiHerkunft::lose(ziel); }
        } else {
            geschrieben = true;
            meldung(fmt(tr(Str::KmArbeitsordner), bsp), directoryOf(bsp));
        }
        g_app->kartenQuelle[g_v.kennung] = ziel;
    }
    if (!geschrieben) { return; }
    missionNachladen(neu);
}

void drawKarteMission() {
    if (!g_stand.fensterOffen || !tabfenster::sichtbar(1)) { return; }   // rc609 T1: auch mit Mission in eigenem Fenster
    const float fs = ImGui::GetFontSize();
    // Mitten im Hauptfenster und hoechstens halb so gross - nie ueber Modusleiste und Menue (dort lag es beim ersten Oeffnen)
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::SetNextWindowSize(ImVec2{std::min(44.0F * fs, vp->WorkSize.x * 0.5F), std::min(18.0F * fs, vp->WorkSize.y * 0.5F)}, ImGuiCond_Appearing);
    const std::string titel = std::string(tr(Str::KmTitel)) + "###kartemission";
    if (!ImGui::Begin(titel.c_str(), &g_stand.fensterOffen)) {
        ImGui::End();
        return;
    }
    const auto& z = g_stand.zusammenfassung;
    char t[512];
    std::snprintf(t, sizeof(t), tr(Str::KmZusammenfassung), z[0], z[1], z[2], z[3], z[4]);
    ImGui::TextWrapped("%s", t);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", tr(Str::KmRegel));   // umbrochen: die Regel ist laenger als das Fenster breit
    ImGui::PopStyleColor();
    if (ImGui::BeginTable("##kmbefunde", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                          ImVec2{0.0F, -ImGui::GetFrameHeightWithSpacing()})) {
        ImGui::TableSetupColumn(tr(Str::KmSpalteArt));
        ImGui::TableSetupColumn(tr(Str::KmSpalteWas));
        ImGui::TableSetupColumn(tr(Str::KmSpalteWo));
        ImGui::TableHeadersRow();
        for (const KarteMissionBefund& b : g_stand.befunde) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(befundArt(b).c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(b.was.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(b.wo.c_str());
        }
        ImGui::EndTable();
    }
    if (ImGui::Button(tr(Str::KmSchliessen))) { g_stand.fensterOffen = false; }
    ImGui::End();
}

}  // namespace bhed::gui
