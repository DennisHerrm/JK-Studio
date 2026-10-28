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
//
// Die Dateizeiten liest es hoechstens zweimal je Sekunde; alles andere nur,
// wenn der laufende Schritt einen karte.-Wert braucht (tourMesswerte, Argument).
#include "tour.h"
#include "app_internal.h"
#include "karteneditor.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>

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
    if (karte) { karteMesswerte(werte, jetzt); }
}

}  // namespace bhed::gui
