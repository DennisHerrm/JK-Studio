// tour_messwerte.cpp - Messwerte fuer die Tutorials aus allen Bereichen (H1)
//
// Stufe H1 (der Helfer). Ein Schritt einer Tour ist erledigt, wenn ein
// ZUSTAND eintritt (include/bhed/tour.h, "zahl:<name>"). Die Skript-Touren
// lesen ihren Zustand in gui/tour.cpp (kontextBauen); hier stehen die Werte
// der anderen Bereiche - zuerst der Karten-Editor (Tutorial "Deine erste
// Karte", tut-karte):
//
//   karte.spiel          1, wenn ein Spiel geladen ist (Shader im Inventar)
//   karte.leer           1 bei einer leeren Karte (kein Pinsel, hoechstens worldspawn)
//   karte.pinsel         Pinsel in der Welt
//   karte.hoehe          Oberkante aller Pinsel
//   karte.texturen       Pruefsumme der Flaechentexturen ("~" = eine Textur wechselte)
//   karte.caulk          Flaechen mit caulk
//   karte.keineauswahl   1, wenn nichts gewaehlt ist
//   karte.ent            Entities (ohne worldspawn)
//   karte.ent.<klasse>   Entities dieser Klasse (light, info_player_start ...)
//   karte.gespeichert    Aenderungszeit der .map  (ohne Haken im Editor: die
//   karte.gebaut         ... der .bsp              Datei selbst zeigt, dass
//   karte.pk3            ... der .pk3              gespeichert/gebaut/gepackt wurde)
//   karte.spielen        1, solange "Im Editor spielen" laeuft
//   vorschau.bereit      1 im Modus Map (Vorschau)
//   vorschau.karte       1, wenn dort eine Karte geladen ist (vorschau.ohnekarte: nicht)
//   vorschau.blick       Pruefsumme von Ort und Winkel der freien Kamera ("~" = umgesehen)
//   vorschau.durchkamera 1, solange "Durch die Kamera" an ist
//   vorschau.kameragewaehlt  1, wenn die Kamera gewaehlt ist (ihr Inspektor steht links)
//   vorschau.ohnekamera      1 in der Vorschau, wenn das Skript keinen camera-Befehl hat
//   vorschau.nichtgewaehlt   1 in der Vorschau, wenn es Kameras gibt, aber keine gewaehlt ist
//   vorschau.einstellungen   Kameraeinstellungen im Skript (ein einzelner camera-Befehl ist noch keine)
//   modell.figur         Pruefsumme von Modell und NPC im Modell-Reiter ("~" = andere Figur)
//   modell.blick         Pruefsumme des Blicks auf das Modell (und Drehteller)
//   modell.details       Skin, Skelett, Texturen ("~" = eins davon umgeschaltet)
//   menue.projekt        1, wenn im Menue-Editor ein Projekt offen ist (M5b, Tutorial tut-menue-eigen)
//   menue.elemente       Items im Menue der Seite
//   menue.layouts        Layouts in diesem Menue
//   menue.ereignisse     Ereignis-Skripte seiner Items
//   menue.gespeichert    erfolgreiche Projekt-Speicherungen seit dem Start
//
// H2 (mehr interaktive Touren):
//   vorschau.laeuft      1, solange die Zeitleiste abspielt
//   vorschau.startseite  1 im Modus Map ohne Karte (die Startseite steht)
//   vorschau.ents        Entities der Karte in der Vorschau ("-" = eine geloescht)
//   vorschau.figurgewaehlt   1, wenn eine Figur gewaehlt ist (ihr Weg steht)
//   vorschau.ohnefigur       1 in der Vorschau mit Karte, aber ohne Figur in der Szene
//   vorschau.ohnelaufziel    1, wenn die gewaehlte Figur kein Laufziel zeigt
//   vorschau.tlschritte      Bildschritte mit den Pfeiltasten in der Zeitleiste
//   vorschau.gangartmenue    1, solange das Menue eines Laufziels offen ist
//   vorschau.wegfenster / .wegpunkte / .weggeprueft
//                        das Fenster NPC-Wegpunkte, Zahl der Wegpunkte, gepruefte Befunde
//   mission.name         Pruefsumme von Name und Ordner der Mission ("~" = angelegt/geoeffnet)
//   modell.da            1, wenn ein Modell geladen ist
//   modell.skin          vom Anwender gewaehlte Haeute ("+" = eine gewaehlt; das Laden einer Figur zaehlt nicht)
//   modell.caps          1, wenn die _off-Flaechen gezeigt werden
//   modell.flaechenliste Klicks auf den Flaechenzaehler (Liste im Detailprotokoll)
//   modell.skelett       Pruefsumme der Animationsliste ("~" = anderes Skelett)
//   programm.pfade       1, solange Edit > Paths offen ist
//   programm.spielordner eingerichtete Spielordner ("+" = einer dazu)
//   programm.regelwerk   Regelwerk der Vorschau (0 auto, 1 OpenJK, 2 Movie Duels ...)
//   update.fenster       1, solange das Update-Fenster offen ist
//   update.ergebnis      1, wenn die Suche fertig ist (aktuell, neu da oder Fehler)
//   update.auto          1, wenn beim Start nach Updates gesucht wird
//   karte.schluessel     Pruefsumme aller Schluessel/Werte der Entities ("~" = geaendert)
//   karte.verbindungen   target -> targetname-Verbindungen zwischen Entities
//   karte.einstellungen  1, solange die Einstellungen des Karten-Editors offen sind
//   menue.dokumente      offene Menues im Menue-Editor ("+" = eins geoeffnet)
//   menue.text           Pruefsumme des Menuetexts ("~" = etwas verschoben/geaendert)
//   menue.bedienen       1 im Bedienmodus
//   menue.exporte        erfolgreiche .pk3-Exporte ("+" = einer dazu)
//
// Die Dateizeiten liest es hoechstens zweimal je Sekunde; alles andere nur,
// wenn der laufende Schritt einen karte.-Wert braucht (tourMesswerte, Argument).
#include "tour.h"
#include "app_internal.h"
#include "karteneditor.h"
#include "menueeditor.h"
#include "update.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace bhed::gui {
namespace {

namespace ke = karteneditor;

double dateiZeit(const std::filesystem::path& p) {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(p, ec);
    if (ec) { return 0.0; }
    return std::chrono::duration<double>(t.time_since_epoch()).count();
}

struct Zeiten {
    double gelesen = -1.0;
    std::string name;
    double map = 0.0;
    double bsp = 0.0;
    double pk3 = 0.0;
};

int befehlZahl(const std::vector<Node>& ns, const std::string& name) {
    int n = 0;
    for (const Node& k : ns) { n += (k.name == name ? 1 : 0) + befehlZahl(k.children, name); }
    return n;
}

std::uint32_t fnv(std::uint32_t h, const std::string& s) {
    for (const char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 16777619U;
    }
    return h;
}

void karteMesswerte(std::map<std::string, double>& w, double jetzt) {
    const ke::Editor& e = ke::editor();
    w["karte.spiel"] = (e.karteSpiel && !e.karteSpiel->inventar.shader().empty()) ? 1.0 : 0.0;
    int pinsel = 0;
    int caulk = 0;
    float oben = -1e9F;
    std::uint32_t h = 2166136261U;
    if (const auto welt = e.karte.welt()) {
        for (const auto& p : welt->kinder()) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const auto pin = std::static_pointer_cast<karte::Pinsel>(p);
            ++pinsel;
            const karte::Kasten& b = pin->grenzen(e.karte.kontext);
            oben = std::max(oben, b.origin[2] + b.extents[2]);
            for (const auto& f : pin->flaechen()) {
                const std::string& sh = f->shader();
                h = fnv(h, sh);
                h = fnv(h, "|");
                if (sh.size() >= 5 && sh.compare(sh.size() - 5, 5, "caulk") == 0) { ++caulk; }
            }
        }
    }
    int ents = 0;
    std::map<std::string, int> klassen;
    for (const auto& ent : e.karte.entitaeten()) {
        const std::string k = ent->klassenname();
        if (k == "worldspawn") { continue; }
        ++ents;
        ++klassen[k];
    }
    w["karte.pinsel"] = pinsel;
    w["karte.caulk"] = caulk;
    w["karte.hoehe"] = pinsel > 0 ? static_cast<double>(oben) : 0.0;
    w["karte.texturen"] = static_cast<double>(h);
    w["karte.leer"] = (pinsel == 0 && ents == 0) ? 1.0 : 0.0;
    w["karte.keineauswahl"] = e.karte.auswahl.anzahl() == 0 ? 1.0 : 0.0;
    w["karte.ent"] = ents;
    for (const auto& [k, n] : klassen) {
        bool ok = !k.empty();
        for (const char c : k) { ok = ok && ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'); }
        if (ok) { w["karte.ent." + k] = n; }
    }
    w["karte.spielen"] = e.spielen.aktiv ? 1.0 : 0.0;
    // H2: Schluessel der Entities und ihre Verbindungen (target -> targetname).
    std::uint32_t hs = 2166136261U;
    std::map<std::string, int> namen;
    for (const auto& ent : e.karte.entitaeten()) {
        for (const auto& [k, v] : ent->schluessel()) {
            hs = fnv(fnv(fnv(hs, k), "="), v);
            if (k == "targetname" && !v.empty()) { ++namen[v]; }
        }
        hs = fnv(hs, ";");
    }
    int verbindungen = 0;
    for (const auto& ent : e.karte.entitaeten()) {
        for (const auto& [k, v] : ent->schluessel()) {
            if ((k == "target" || k == "target2") && !v.empty()) {
                const auto it = namen.find(v);
                verbindungen += it != namen.end() ? it->second : 0;
            }
        }
    }
    w["karte.schluessel"] = static_cast<double>(hs);
    w["karte.verbindungen"] = verbindungen;
    // Dateizeiten (Speichern, Bauen, Packen) - gedrosselt.
    static Zeiten z;
    if (z.name != e.name || jetzt - z.gelesen > 0.5) {
        z.name = e.name;
        z.gelesen = jetzt;
        z.map = z.bsp = z.pk3 = 0.0;
        if (!e.name.empty() && e.name != "unnamed.map") {
            const std::filesystem::path m(std::u8string(e.name.begin(), e.name.end()));
            z.map = dateiZeit(m);
            std::filesystem::path b = m;
            z.bsp = dateiZeit(b.replace_extension(".bsp"));
            z.pk3 = dateiZeit(b.replace_extension(".pk3"));
        }
    }
    w["karte.gespeichert"] = z.map;
    w["karte.gebaut"] = ke::bauLaeuft() ? 0.0 : z.bsp;
    w["karte.pk3"] = z.pk3;
}

}  // namespace

void tourMesswerte(std::map<std::string, double>& werte, bool karte, double jetzt) {
    if (g_app == nullptr) { return; }
    werte["programm.modus"] = g_app->leftMode;
    // Das Hauptmenue (File, Edit, View ...) steht in den Modi 0-2; im
    // Karten-Editor (3) stehen dessen eigene Menues.
    werte["programm.hauptmenue"] = g_app->leftMode != 3 ? 1.0 : 0.0;
    werte["programm.einstellungen"] = g_app->prefsOpen ? 1.0 : 0.0;
    werte["karte.einstellungen"] = ke::editor().einstellungenOffen ? 1.0 : 0.0;   // H2: Einstellungen des Karten-Editors
    // Die Vorschau (Modus 1): Karte da, Blick der freien Kamera, durch die Kamera.
    const bool karteDa = g_app->mapBereit && !g_app->map.empty();
    werte["vorschau.bereit"] = g_app->leftMode == 1 ? 1.0 : 0.0;
    werte["vorschau.karte"] = karteDa ? 1.0 : 0.0;
    werte["vorschau.ohnekarte"] = (g_app->leftMode == 1 && !karteDa) ? 1.0 : 0.0;
    double blick = 0.0;
    for (int i = 0; i < 3; ++i) {
        blick = blick * 1000.0 + std::floor(static_cast<double>(g_app->cam.pos[i]) / 8.0);
        blick = blick * 1000.0 + std::floor(static_cast<double>(g_app->cam.angles[i]));
    }
    werte["vorschau.blick"] = blick;
    werte["vorschau.durchkamera"] = g_app->throughCamera ? 1.0 : 0.0;
    const bool kameraGewaehlt = g_app->cameraSelected && g_app->pickedEntity < 0 && g_app->selectedActor < 0;
    werte["vorschau.kameragewaehlt"] = kameraGewaehlt ? 1.0 : 0.0;
    const int kameras = befehlZahl(g_app->doc.script().nodes, "camera");
    werte["vorschau.ohnekamera"] = (g_app->leftMode == 1 && kameras == 0) ? 1.0 : 0.0;
    werte["vorschau.nichtgewaehlt"] = (g_app->leftMode == 1 && kameras > 0 && !kameraGewaehlt) ? 1.0 : 0.0;
    // Kameraeinstellungen (MOVE+PAN-Paare) - nur in der Vorschau gerechnet.
    // Die Modellansicht (Modus 2): welche Figur, Blick, Details.
    werte["modell.figur"] = static_cast<double>(fnv(fnv(2166136261U, g_app->modelPath), g_app->npcGeladen));
    double mblick = g_app->modellDrehteller ? 1.0 : 0.0;
    for (int i = 0; i < 3; ++i) {
        mblick = mblick * 1000.0 + std::floor(static_cast<double>(g_app->modelCam.pos[i]));
        mblick = mblick * 1000.0 + std::floor(static_cast<double>(g_app->modelCam.angles[i]));
    }
    werte["modell.blick"] = mblick;
    werte["modell.details"] = g_app->modelSkinIndex * 4.0 + (g_app->modellKnochen ? 2.0 : 0.0) + (g_app->showModelTextures ? 1.0 : 0.0);
    werte["vorschau.einstellungen"] = g_app->leftMode == 1 ? static_cast<double>(kameraSchluessel(g_app->doc.script()).size()) : 0.0;
    // H2: Abspielen, Startseite, Figuren, Wegpunkte, Mission.
    werte["vorschau.laeuft"] = g_app->playing ? 1.0 : 0.0;
    werte["vorschau.startseite"] = (g_app->leftMode == 1 && !karteDa) ? 1.0 : 0.0;
    werte["vorschau.ents"] = static_cast<double>(g_app->map.entities.size());
    werte["vorschau.figurgewaehlt"] = (g_app->leftMode == 1 && g_app->selectedActor >= 0) ? 1.0 : 0.0;
    werte["vorschau.gangartmenue"] = tourGemeldet("vorschau.gangartmenue") ? 1.0 : 0.0;
    werte["vorschau.ohnefigur"] = (g_app->leftMode == 1 && karteDa && g_app->scene.actors.empty()) ? 1.0 : 0.0;
    werte["vorschau.ohnelaufziel"] =
        (g_app->leftMode == 1 && g_app->selectedActor >= 0 && !tourGemeldet("vorschau.laufziel")) ? 1.0 : 0.0;
    werte["vorschau.tlschritte"] = g_app->tlSchritte;
    werte["vorschau.wegfenster"] = g_app->wegFensterOffen ? 1.0 : 0.0;
    int wegpunkte = 0;
    for (const MapEntity& me : g_app->map.entities) { wegpunkte += me.classname.rfind("waypoint", 0) == 0 ? 1 : 0; }
    werte["vorschau.wegpunkte"] = wegpunkte;
    werte["vorschau.weggeprueft"] = g_app->wegGeprueft ? 1.0 : 0.0;
    werte["mission.name"] = static_cast<double>(fnv(fnv(2166136261U, g_app->mission.name), g_app->mission.projektOrdner));
    // Modell: Haut, Kappen, Flaechenliste, Skelett.
    werte["modell.da"] = g_app->model.empty() ? 0.0 : 1.0;
    werte["modell.skin"] = g_app->modellHautWechsel;
    werte["modell.caps"] = g_app->showCaps ? 1.0 : 0.0;
    werte["modell.flaechenliste"] = g_app->modellFlaechenListen;
    std::uint32_t ha = fnv(2166136261U, std::to_string(g_app->anim.numFrames) + "/" + std::to_string(g_app->anim.bones.size()));
    for (const AnimEntry& a : g_app->animList) { ha = fnv(ha, a.name); }
    werte["modell.skelett"] = static_cast<double>(ha);
    // Programm: Spielordner, Regelwerk, Updates.
    werte["programm.pfade"] = g_app->pathsOpen ? 1.0 : 0.0;
    werte["programm.spielordner"] = static_cast<double>(g_app->settings.gamePaths.size() + g_app->settings.gamePathsJk2.size());
    werte["programm.regelwerk"] = g_app->settings.regelwerk;
    werte["update.fenster"] = updater::fensterOffen() ? 1.0 : 0.0;
    const updater::Stand us = updater::zustand().stand;
    werte["update.ergebnis"] = (us == updater::Stand::Aktuell || us == updater::Stand::Verfuegbar || us == updater::Stand::Fehler ||
                                us == updater::Stand::Fertig)
                                   ? 1.0
                                   : 0.0;
    werte["update.auto"] = g_app->settings.updateCheck ? 1.0 : 0.0;
    // Menue-Editor (Modus 4).
    if (g_app->leftMode == 4) {
        const menueeditor::Stand ms = menueeditor::stand();
        werte["menue.dokumente"] = ms.dokumente;
        werte["menue.text"] = static_cast<double>(fnv(2166136261U, ms.text));
        werte["menue.bedienen"] = ms.interaktion ? 1.0 : 0.0;
        werte["menue.exporte"] = ms.exporte;
    }
    if (karte) { karteMesswerte(werte, jetzt); }
    // M5b: Tour "Dein erstes eigenes Menue" (tut-menue-eigen) - nur im Menue-Editor
    if (g_app->leftMode == 4) {
        const menueeditor::TourWerte m = menueeditor::tourWerte();
        werte["menue.projekt"] = m.projekt ? 1.0 : 0.0;
        werte["menue.elemente"] = m.elemente;
        werte["menue.layouts"] = m.layouts;
        werte["menue.ereignisse"] = m.ereignisse;
        werte["menue.gespeichert"] = m.gespeichert;
    }
}

}  // namespace bhed::gui
