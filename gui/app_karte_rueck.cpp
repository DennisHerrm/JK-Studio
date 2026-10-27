// app_karte_rueck.cpp - Karten-Editor Stufe 8a Punkt 3 (und 4): was eine fertige
// .bsp verloren hat, beim Oeffnen zurueckgewinnen (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Nach "Open .bsp" (Dekompilat von q3map2 -convert) laeuft im Hintergrund:
// bhed_q3map2 -bhedmodels laedt die Modelle, die in Frage kommen, wie
// InsertModel sie sieht; karte::modelleZurueckgewinnen sucht sie in der
// Geometrie. In Frage kommen (Stufe 8b Punkt 8) die Modelle aus den Ordnern der
// Shader der .bsp und jedes Modell aus ALLEN Modellordnern, das laut Modell-Index
// ein Netz mit derselben Eckenzahl und Dreiecksmenge wie eine Dreiecksflaeche der
// .bsp hat oder dessen Shader die .bsp nutzt. Der Index (Eckenzahl, Signatur,
// Shader je Netz) liegt im Einstellungsordner (karte_modellindex_<Spiel>.txt);
// beim ersten Mal - und fuer neue oder geaenderte Archive - laedt bhed_q3map2 die
// fehlenden Modelle einmal. Danach - im Hauptfaden, ein Rueckgaengig-Schritt -
// kommen die misc_model in die Karte und, auf Wunsch, die nicht erkannten
// Flaechen als duenne Pinsel in die Ebene "Nicht erkannte Modellflaechen".
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_bau_plattform.h"
#include "platform.h"

#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/ebenen.h"
#include "bhed/karte/lichtschaetzung.h"
#include "bhed/karte/modellrueck.h"
#include "bhed/karte/pinsel.h"
#include "bhed/regelwerk.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;

Editor& e() { return editor(); }

fs::path pfadVon(const std::string& p) { return fs::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())); }

std::string mitSchraeg(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

bool lesen(const std::string& pfad, std::string& d) {
    std::ifstream f(pfadVon(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    d = s.str();
    return true;
}

struct Rueck {
    std::thread faden;
    std::atomic<int> stand{0};          // 0 frei, 1 Modelle laden, 2 suchen, 3 fertig, 4 Fehler, 5 abgebrochen
    std::atomic<double> anteil{0.0};
    std::atomic<bool> abbrechen{false};
    std::mutex sperre;
    karte::RueckErgebnis erg;           // unter sperre, gueltig bei stand 3
    std::vector<karte::GeschaetztesLicht> lichter;   // Punkt 4 (experimentell), unter sperre
    bool mitModellen = true;
    bool mitLichtern = false;
    std::string fehlerText;
    std::string karteDatei;             // die geoeffnete Karte (Ergebnis nur fuer sie)
    std::size_t kandidaten = 0;
    std::size_t modelleGeladen = 0;
    double sekunden = 0;
    // Stufe 8b Punkt 8: der Modell-Index
    std::size_t indexModelle = 0;       // Eintraege im Index
    std::size_t indexNeu = 0;           // in diesem Lauf neu geladen
    double indexSekunden = 0;           // Zeit fuer die neuen Eintraege
    std::size_t kandidatenOrdner = 0;   // davon nach der Ordnerregel
    std::chrono::steady_clock::time_point start;
};
Rueck g_r;

// Stufe 8b Punkt 8: der Schluessel eines Modells im Index - Archiv, Groesse und Zeit
// (bei einem Ordner die der Datei selbst)
std::string dateiStempel(const fs::path& p) {
    std::error_code ec;
    const auto groesse = fs::file_size(p, ec);
    if (ec) { return "?"; }
    const auto zeit = fs::last_write_time(p, ec);
    if (ec) { return "?"; }
    return std::to_string(groesse) + "|" + std::to_string(zeit.time_since_epoch().count());
}

std::string indexDatei() {
    // je Spiel, Engine-Pfad, Mod und Zusatzpfaden eine Datei
    std::string s = std::string(kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja") + "|" + enginePfadAnzeige() + "|" + bauSpielName();
    for (const std::string& z : e().zusatzPfade) { s += "|" + z; }
    std::uint64_t h = 1469598103934665603ULL;
    for (const char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    char z[32];
    std::snprintf(z, sizeof(z), "%016llx", static_cast<unsigned long long>(h));
    return mitSchraeg(platform::settingsDirectory()) + "/karte_modellindex_" + z + ".txt";
}

void fadenWeg() {
    if (g_r.faden.joinable()) {
        g_r.abbrechen = true;
        g_r.faden.join();
    }
}

}  // namespace

std::string modellIndexPfad() { return indexDatei(); }

BspRueckStand bspRueckStand() {
    BspRueckStand s;
    s.stand = g_r.stand.load();
    s.anteil = g_r.anteil.load();
    s.kandidaten = g_r.kandidaten;
    return s;
}

void bspRueckStarten(const std::string& bspDaten, const std::string& ordner, const std::string& stamm, const std::string& karteDatei) {
    Editor& ed = e();
    if ((!ed.bspRueck.modelle && !ed.bspRueck.lichter) || !ed.karteSpiel) { return; }
    fadenWeg();
    g_r.mitModellen = ed.bspRueck.modelle;
    g_r.mitLichtern = ed.bspRueck.lichter;
    g_r.stand = 1;
    g_r.anteil = 0.0;
    g_r.abbrechen = false;
    g_r.karteDatei = karteDatei;
    g_r.fehlerText.clear();
    g_r.start = std::chrono::steady_clock::now();
    // Kandidaten (Hauptfaden: das Dateisystem des Editors)
    std::vector<std::string> pfade;
    for (const char* endung : {"md3", "ase", "obj", "lwo", "3ds", "ms3d"}) {
        for (const std::string& p : ed.karteSpiel->fs.dateien("models/", endung, 16)) { pfade.push_back("models/" + p); }
    }
    // Stufe 8b Punkt 8: die Schluessel aller Modelle (Hauptfaden: das Dateisystem des
    // Editors), der Index aus dem Einstellungsordner, was darin fehlt oder veraltet ist
    std::vector<std::pair<std::string, std::string>> schluessel;   // Pfad -> Schluessel
    {
        std::map<std::string, std::string> archivStempel;
        for (const std::string& p : pfade) {
            const std::string archiv = ed.karteSpiel->fs.finden(p);
            std::string st;
            if (!archiv.empty() && archiv.back() == '/') {
                st = archiv + "|" + dateiStempel(pfadVon(archiv + p));
            } else {
                auto it = archivStempel.find(archiv);
                if (it == archivStempel.end()) { it = archivStempel.emplace(archiv, archiv + "|" + dateiStempel(pfadVon(archiv))).first; }
                st = it->second;
            }
            schluessel.emplace_back(p, st);
        }
    }
    const std::string indexPfad = indexDatei();
    std::vector<karte::ModellIndexEintrag> altIndex;
    {
        std::string text;
        if (!lesen(indexPfad, text) || !karte::modellIndexLesen(text, altIndex)) { altIndex.clear(); }
    }
    std::vector<karte::ModellIndexEintrag> index;   // in der Reihenfolge der Pfade, nur gueltige
    std::vector<std::pair<std::string, std::string>> fehlend;
    {
        std::map<std::string, const karte::ModellIndexEintrag*> nachPfad;
        for (const auto& x : altIndex) { nachPfad[x.pfad] = &x; }
        for (const auto& [p, st] : schluessel) {
            const auto it = nachPfad.find(p);
            if (it != nachPfad.end() && it->second->schluessel == st) {
                index.push_back(*it->second);
            } else {
                fehlend.emplace_back(p, st);
            }
        }
    }
    const std::string liste = mitSchraeg(ordner) + "/" + stamm + "_modelle.txt";
    const std::string aus = mitSchraeg(ordner) + "/" + stamm + "_modelle.bhmd";
    const std::string indexListe = mitSchraeg(ordner) + "/" + stamm + "_modellindex.txt";
    const std::string indexAus = mitSchraeg(ordner) + "/" + stamm + "_modellindex.bhmd";
    if (!fehlend.empty()) {
        std::ofstream f(pfadVon(indexListe), std::ios::binary | std::ios::trunc);
        for (const auto& fe : fehlend) { f << fe.first << "\t0\n"; }
    }
    karte::Q3map2Aufruf a;
    std::string programm = mitSchraeg(platform::executableDirectory());
    if (!programm.empty() && programm.back() != '/') { programm += '/'; }
    a.programm = programm + "bhed_q3map2.exe";
    a.spielTyp = kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja";
    a.enginePfad = enginePfadAnzeige();
    a.fsGame = bauSpielName();
    const std::vector<std::string> zusatz(ed.zusatzPfade.begin(), ed.zusatzPfade.end());
    const std::string befehl = karte::bhedModelleBefehl(a, zusatz, liste, aus);
    const std::string indexBefehl = fehlend.empty() ? std::string() : karte::bhedModelleBefehl(a, zusatz, indexListe, indexAus);
    if (!indexBefehl.empty()) { ausgabe(indexBefehl); }
    ausgabe(befehl);
    g_r.indexModelle = index.size() + fehlend.size();
    g_r.indexNeu = fehlend.size();
    g_r.indexSekunden = 0;
    g_r.kandidaten = 0;
    g_r.kandidatenOrdner = 0;
    g_r.faden = std::thread([befehl, aus, bspDaten, indexBefehl, indexAus, indexPfad, liste, pfade, index, fehlend]() mutable {
        // Punkt 4: Lichter schaetzen (vor den Modellen - braucht keine Spieldateien)
        if (g_r.mitLichtern) {
            karte::LichtSchaetzOptionen lo;
            lo.fortschritt = [](double x) {
                g_r.anteil = x * (g_r.mitModellen ? 0.3 : 1.0);
                return !g_r.abbrechen.load();
            };
            std::vector<karte::GeschaetztesLicht> l = karte::lichterSchaetzen(bspDaten, lo);
            if (g_r.abbrechen) {
                g_r.stand = 5;
                return;
            }
            std::lock_guard<std::mutex> lk(g_r.sperre);
            g_r.lichter = std::move(l);
        }
        if (!g_r.mitModellen) {
            g_r.stand = 3;
            return;
        }
        std::string grund;
        std::error_code ec;
        // Stufe 8b Punkt 8: fehlende Index-Eintraege einmal laden, Index speichern
        if (!indexBefehl.empty()) {
            const auto t0 = std::chrono::steady_clock::now();
            bauplattform::Prozess ip;
            ip.niedrigePrioritaet(true);
            fs::remove(pfadVon(indexAus), ec);
            if (!ip.starten(indexBefehl, std::string(), false, grund)) {
                std::lock_guard<std::mutex> l(g_r.sperre);
                g_r.fehlerText = "bhed_q3map2 -bhedmodels (index): " + grund;
                g_r.stand = 4;
                return;
            }
            while (!ip.warten(100)) {
                if (g_r.abbrechen) {
                    ip.beenden();
                    g_r.stand = 5;
                    return;
                }
            }
            std::string dump;
            std::vector<karte::ModellDatei> neu;
            if (!lesen(indexAus, dump) || !karte::modellDumpLesen(dump, neu)) {
                std::lock_guard<std::mutex> l(g_r.sperre);
                g_r.fehlerText = "bhed_q3map2 -bhedmodels (index): no readable result (" + std::to_string(ip.rueckgabe()) + ")";
                g_r.stand = 4;
                return;
            }
            fs::remove(pfadVon(indexAus), ec);
            std::map<std::string, std::string> stempel(fehlend.begin(), fehlend.end());
            for (const karte::ModellDatei& m : neu) {
                const auto it = stempel.find(m.pfad);
                index.push_back(karte::modellIndexEintrag(m, it != stempel.end() ? it->second : std::string("?")));
            }
            std::ofstream f(pfadVon(indexPfad), std::ios::binary | std::ios::trunc);
            f << karte::modellIndexSchreiben(index);
            g_r.indexSekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        }
        // Die Kandidaten: Ordnerregel und Index
        {
            const karte::BspSuchmerkmale mk = karte::bspSuchmerkmale(bspDaten);
            const std::vector<std::string> kand = karte::modellKandidatenMitIndex(pfade, mk, index);
            g_r.kandidatenOrdner = karte::modellKandidaten(pfade, mk.shader).size();
            g_r.kandidaten = kand.size();
            std::ofstream f(pfadVon(liste), std::ios::binary | std::ios::trunc);
            for (const std::string& k : kand) { f << k << "\t0\n"; }
        }
        bauplattform::Prozess p;
        p.niedrigePrioritaet(true);
        fs::remove(pfadVon(aus), ec);
        if (!p.starten(befehl, std::string(), false, grund)) {
            std::lock_guard<std::mutex> l(g_r.sperre);
            g_r.fehlerText = "bhed_q3map2 -bhedmodels: " + grund;
            g_r.stand = 4;
            return;
        }
        while (!p.warten(100)) {
            if (g_r.abbrechen) {
                p.beenden();
                g_r.stand = 5;
                return;
            }
        }
        std::string dump;
        std::vector<karte::ModellDatei> modelle;
        if (!lesen(aus, dump) || !karte::modellDumpLesen(dump, modelle)) {
            std::lock_guard<std::mutex> l(g_r.sperre);
            g_r.fehlerText = "bhed_q3map2 -bhedmodels: no readable result (" + std::to_string(p.rueckgabe()) + ")";
            g_r.stand = 4;
            return;
        }
        std::size_t geladen = 0;
        for (const auto& m : modelle) { geladen += m.geladen ? 1U : 0U; }
        g_r.modelleGeladen = geladen;
        g_r.stand = 2;
        karte::RueckOptionen o;
        o.fortschritt = [](double x) {
            g_r.anteil = g_r.mitLichtern ? 0.3 + 0.7 * x : x;
            return !g_r.abbrechen.load();
        };
        karte::RueckErgebnis erg = karte::modelleZurueckgewinnen(bspDaten, modelle, o);
        if (erg.abgebrochen) {
            g_r.stand = 5;
            return;
        }
        {
            std::lock_guard<std::mutex> l(g_r.sperre);
            g_r.erg = std::move(erg);
        }
        g_r.stand = 3;
    });
}

void bspRueckAbbrechen() {
    if (g_r.stand == 1 || g_r.stand == 2) { g_r.abbrechen = true; }
}

void bspRueckBeenden() { fadenWeg(); }

namespace {

// Im Hauptfaden: das Ergebnis in die Karte (ein Rueckgaengig-Schritt)
void ergebnisAnwenden() {
    Editor& ed = e();
    if (g_r.faden.joinable()) { g_r.faden.join(); }
    g_r.sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - g_r.start).count();
    karte::RueckErgebnis erg;
    std::vector<karte::GeschaetztesLicht> lichter;
    {
        std::lock_guard<std::mutex> l(g_r.sperre);
        erg = std::move(g_r.erg);
        lichter = std::move(g_r.lichter);
        g_r.lichter.clear();
    }
    g_r.stand = 0;
    if (mitSchraeg(ed.name) != mitSchraeg(g_r.karteDatei)) {
        warnung(trK(Str::KeRkAndereKarte));
        return;
    }
    // Hat sich die Geometrie seit dem Oeffnen geaendert (File > Import, ein verschobener Pinsel),
    // waere der neue Fingerabdruck unten falsch: "Update entities only" naehme die Aenderung als
    // Teil der .bsp. Das Ergebnis verwerfen - Entity-Aenderungen stoeren nicht.
    if (ed.bspHerkunft.aktiv && karte::geometrieFingerabdruck(ed.karte) != ed.bspHerkunft.fingerabdruck) {
        warnung(trK(Str::KeRkGeaendert));
        return;
    }
    karte::Karte& k = ed.karte;
    std::size_t forceMeta = 0;
    double maxFehler = 0;
    k.undo.start();
    for (const karte::ModellTreffer& t : erg.treffer) {
        auto en = k.neueEntitaet("misc_model", false);
        for (const auto& [kk, vv] : karte::trefferSchluessel(t)) {
            if (kk != "classname") { en->setzen(kk, vv); }
        }
        k.entitaetEinfuegen(en);
        forceMeta += t.forceMeta ? 1U : 0U;
        maxFehler = std::max(maxFehler, t.maxFehler);
    }
    std::size_t pinsel = 0;
    std::vector<karte::ModellVieleck> vielecke;
    if (ed.bspRueck.flaechen && !erg.unerkannteDreiecke.empty()) {
        vielecke = karte::unerkannteVielecke(erg.unerkannteDreiecke);
        int ebene = -1;
        const std::size_t vorher = k.aktuelleSchicht;
        (void)karte::ebeneNeu(k, -1, trK(Str::KeRkEbene), &ebene);
        k.aktuelleSchicht = vorher;
        auto welt = k.weltHolenOderAnlegen();
        for (const karte::ModellVieleck& v : vielecke) {
            auto p = karte::vieleckPinsel(v, 1.0, "textures/system/nodraw", k.kontext);
            if (!p) { continue; }
            p->schicht = ebene;
            welt->kindEinfuegen(p);
            ++pinsel;
        }
    }
    // Punkt 4 (experimentell): geschaetzte Lichter in der Ebene "Geschaetzte Lichter",
    // je mit "_bhed_geschaetzt" "1" (q3map2 und das Spiel lesen den Schluessel nicht)
    std::size_t lichtZahl = 0;
    if (g_r.mitLichtern && !lichter.empty()) {
        int ebene = -1;
        const std::size_t vorher = k.aktuelleSchicht;
        (void)karte::ebeneNeu(k, -1, trK(Str::KeLsEbene), &ebene);
        k.aktuelleSchicht = vorher;
        for (const karte::GeschaetztesLicht& l : lichter) {
            auto en = k.neueEntitaet("light", false);
            for (const auto& [kk, vv] : karte::lichtSchluessel(l)) {
                if (kk != "classname") { en->setzen(kk, vv); }
            }
            en->schicht = ebene;
            k.entitaetEinfuegen(en);
            ++lichtZahl;
        }
    }
    k.undo.finish(g_r.mitModellen ? "Recover misc_model (" + std::to_string(erg.treffer.size()) + ")" : "Estimate lights (" + std::to_string(lichtZahl) + ")");
    ed.bspRueckLetzt.lichter = lichtZahl;
    // die zurueckgewonnenen Modelle und Flaechen stecken schon in der .bsp: "Update entities
    // only" bleibt moeglich
    if (ed.bspHerkunft.aktiv) { ed.bspHerkunft.fingerabdruck = karte::geometrieFingerabdruck(k); }
    ed.bspRueckLetzt.treffer = erg.treffer.size();
    ed.bspRueckLetzt.forceMeta = forceMeta;
    ed.bspRueckLetzt.pinsel = pinsel;
    ed.bspRueckLetzt.dreiecke = erg.unerkannteDreiecke.size();
    ed.bspRueckLetzt.maxFehler = maxFehler;
    ed.bspRueckLetzt.sekunden = g_r.sekunden;
    char z[512];
    if (g_r.mitLichtern) {
        std::snprintf(z, sizeof(z), trK(Str::KeLsFertig), static_cast<int>(lichtZahl));
        warnung(z);
    }
    if (!g_r.mitModellen) {
        ++ed.sichtStand;
        auswahlGeaendert();
        return;
    }
    std::snprintf(z, sizeof(z), trK(Str::KeRkFertig), static_cast<int>(erg.treffer.size()), static_cast<int>(forceMeta), maxFehler,
                  static_cast<int>(g_r.modelleGeladen), static_cast<int>(g_r.kandidaten), g_r.sekunden);
    ausgabe(z);
    std::snprintf(z, sizeof(z), trK(Str::KeRkIndex), static_cast<int>(g_r.indexModelle), static_cast<int>(g_r.indexNeu), g_r.indexSekunden,
                  static_cast<int>(g_r.kandidatenOrdner), static_cast<int>(g_r.kandidaten));
    ausgabe(z);
    ed.bspRueckLetzt.indexModelle = g_r.indexModelle;
    ed.bspRueckLetzt.indexNeu = g_r.indexNeu;
    ed.bspRueckLetzt.kandidatenOrdner = g_r.kandidatenOrdner;
    ed.bspRueckLetzt.kandidaten = g_r.kandidaten;
    if (!erg.unerkannteDreiecke.empty()) {
        std::snprintf(z, sizeof(z), trK(ed.bspRueck.flaechen ? Str::KeRkFlaechen : Str::KeRkFlaechenAus), static_cast<int>(erg.unerkannteDreiecke.size()),
                      static_cast<int>(pinsel));
        warnung(z);
    }
    ++ed.sichtStand;
    auswahlGeaendert();
}

}  // namespace

// Jedes Bild: Ergebnis uebernehmen, Fortschritt zeigen
void bspRueckSchritt() {
    const int st = g_r.stand.load();
    if (st == 3) {
        ergebnisAnwenden();
        return;
    }
    if (st == 4 || st == 5) {
        if (g_r.faden.joinable()) { g_r.faden.join(); }
        if (st == 4) {
            std::lock_guard<std::mutex> l(g_r.sperre);
            fehler(g_r.fehlerText);
        } else {
            warnung(trK(Str::KeRkAbgebrochen));
        }
        g_r.stand = 0;
        return;
    }
    if (st != 1 && st != 2) { return; }
    const float s = massstab();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x - 16.0F * s, vp->WorkPos.y + vp->WorkSize.y - 220.0F * s}, ImGuiCond_FirstUseEver,
                            ImVec2{1.0F, 1.0F});
    ImGui::SetNextWindowSize(ImVec2{std::round(420.0F * s), 0.0F}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeRkTitel)) + "###kartebsprueck";
    if (ImGui::Begin(titel.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        char z[256];
        if (st == 1) {
            std::snprintf(z, sizeof(z), trK(Str::KeRkLaden), static_cast<int>(g_r.kandidaten));
            ImGui::TextUnformatted(z);
            ImGui::ProgressBar(-1.0F * static_cast<float>(ImGui::GetTime()), ImVec2{ImGui::GetContentRegionAvail().x, 0.0F}, "");
        } else {
            ImGui::TextUnformatted(trK(Str::KeRkSuchen));
            std::snprintf(z, sizeof(z), "%d %%", static_cast<int>(std::lround(g_r.anteil.load() * 100.0)));
            ImGui::ProgressBar(static_cast<float>(g_r.anteil.load()), ImVec2{ImGui::GetContentRegionAvail().x, 0.0F}, z);
        }
        if (ImGui::Button((std::string(trK(Str::KeBmAbbrechen)) + "##bsprueckabbrechen").c_str())) { bspRueckAbbrechen(); }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", trK(Str::KeRkHinweis));
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

}  // namespace bhed::gui::karteneditor
