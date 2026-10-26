// app_karte_rueck.cpp - Karten-Editor Stufe 8a Punkt 3 (und 4): was eine fertige
// .bsp verloren hat, beim Oeffnen zurueckgewinnen (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Nach "Open .bsp" (Dekompilat von q3map2 -convert) laeuft im Hintergrund:
// bhed_q3map2 -bhedmodels laedt die Modelle, die in Frage kommen (Ordner der
// Shader der .bsp), wie InsertModel sie sieht; karte::modelleZurueckgewinnen
// sucht sie in der Geometrie. Danach - im Hauptfaden, ein Rueckgaengig-Schritt -
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
#include "bhed/karte/modellrueck.h"
#include "bhed/karte/pinsel.h"
#include "bhed/regelwerk.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
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
    std::string fehlerText;
    std::string karteDatei;             // die geoeffnete Karte (Ergebnis nur fuer sie)
    std::size_t kandidaten = 0;
    std::size_t modelleGeladen = 0;
    double sekunden = 0;
    std::chrono::steady_clock::time_point start;
};
Rueck g_r;

void fadenWeg() {
    if (g_r.faden.joinable()) {
        g_r.abbrechen = true;
        g_r.faden.join();
    }
}

}  // namespace

BspRueckStand bspRueckStand() {
    BspRueckStand s;
    s.stand = g_r.stand.load();
    s.anteil = g_r.anteil.load();
    s.kandidaten = g_r.kandidaten;
    return s;
}

void bspRueckStarten(const std::string& bspDaten, const std::string& ordner, const std::string& stamm, const std::string& karteDatei) {
    Editor& ed = e();
    if (!ed.bspRueck.modelle || !ed.karteSpiel) { return; }
    fadenWeg();
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
    karte::BspDatei b;
    std::vector<std::string> shader;
    if (karte::bspLesen(bspDaten, b) && b.lumps.size() > 1) {
        for (std::size_t i = 0; i + 72 <= b.lumps[1].size(); i += 72) { shader.emplace_back(b.lumps[1].data() + i, strnlen(b.lumps[1].data() + i, 64)); }
    }
    const std::vector<std::string> kand = karte::modellKandidaten(pfade, shader);
    g_r.kandidaten = kand.size();
    const std::string liste = mitSchraeg(ordner) + "/" + stamm + "_modelle.txt";
    const std::string aus = mitSchraeg(ordner) + "/" + stamm + "_modelle.bhmd";
    {
        std::ofstream f(pfadVon(liste), std::ios::binary | std::ios::trunc);
        for (const std::string& k : kand) { f << k << "\t0\n"; }
    }
    karte::Q3map2Aufruf a;
    std::string programm = mitSchraeg(platform::executableDirectory());
    if (!programm.empty() && programm.back() != '/') { programm += '/'; }
    a.programm = programm + "bhed_q3map2.exe";
    a.spielTyp = kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja";
    a.enginePfad = enginePfadAnzeige();
    a.fsGame = bauSpielName();
    const std::string befehl = karte::bhedModelleBefehl(a, std::vector<std::string>(ed.zusatzPfade.begin(), ed.zusatzPfade.end()), liste, aus);
    ausgabe(befehl);
    g_r.faden = std::thread([befehl, aus, bspDaten]() {
        bauplattform::Prozess p;
        p.niedrigePrioritaet(true);
        std::string grund;
        std::error_code ec;
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
            g_r.anteil = x;
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
    {
        std::lock_guard<std::mutex> l(g_r.sperre);
        erg = std::move(g_r.erg);
    }
    g_r.stand = 0;
    if (mitSchraeg(ed.name) != mitSchraeg(g_r.karteDatei)) {
        warnung(trK(Str::KeRkAndereKarte));
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
    k.undo.finish("Recover misc_model (" + std::to_string(erg.treffer.size()) + ")");
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
    std::snprintf(z, sizeof(z), trK(Str::KeRkFertig), static_cast<int>(erg.treffer.size()), static_cast<int>(forceMeta), maxFehler,
                  static_cast<int>(g_r.modelleGeladen), static_cast<int>(g_r.kandidaten), g_r.sekunden);
    ausgabe(z);
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
