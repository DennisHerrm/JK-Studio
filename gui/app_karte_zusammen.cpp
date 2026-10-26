// app_karte_zusammen.cpp - Karten-Editor Stufe 8a Punkt 5c: "Zur fertigen Karte
// hinzufuegen" (eigene Funktion, nicht in NetRadiant-Custom).
//
// Eine fertige .bsp ist die Vorlage (gezeichnet, nicht bearbeitbar); was in einer
// Ebene der Karte liegt, wird allein gebaut (bhed_q3map2 -meta, -light) und mit
// -mergebsp -world -fixnames in eine Kopie der Vorlage eingefuegt. Davor und danach
// die Pruefungen aus bhed/karte/zusammenfuehren.h und die Grenzen des Optimierers.
//
// Licht des Teils: q3map2 beleuchtet den Teil allein (die Geometrie der Hauptkarte
// ist beim Bauen nicht da, -mergebsp rechnet kein Licht). Angenaehert wird mit dem
// Lichtgitter der Hauptkarte im Kasten des Teils (als _ambient/_color) und mit den
// Lichtern ohne targetname, die in der Hauptkarte stehen geblieben sind (-keeplights) -
// beides abschaltbar; die eigenen Lichter der Ebene leuchten wie immer.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_bau_plattform.h"
#include "platform.h"

#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/zusammenfuehren.h"
#include "bhed/regelwerk.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

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

bool schreiben(const std::string& pfad, const std::string& d) {
    std::ofstream f(pfadVon(pfad), std::ios::binary | std::ios::trunc);
    return f && f.write(d.data(), static_cast<std::streamsize>(d.size()));
}

struct Dialog {
    bool offen = false;
    int ebene = 0;
    bool lichtGitter = true;
    bool lichtLichter = true;
    std::string vorlageDaten;         // die Bytes der Vorlage
    std::string vorlageName;          // Anzeige
    std::vector<karte::BspEintrag> liste;
    int listenWahl = -1;
    std::string ordner;               // Arbeitsordner
    std::string stamm;
};
Dialog g_d;
ZusammenStand g_st;

std::string arbeitsOrdner() { return mitSchraeg(platform::settingsDirectory()) + "/karte_zusammen"; }

karte::Q3map2Aufruf aufruf() {
    karte::Q3map2Aufruf a;
    std::string programm = mitSchraeg(platform::executableDirectory());
    if (!programm.empty() && programm.back() != '/') { programm += '/'; }
    a.programm = programm + "bhed_q3map2.exe";
    a.spielTyp = kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja";
    a.enginePfad = enginePfadAnzeige();
    a.fsGame = bauSpielName();
    return a;
}

std::string q3(const std::string& rest) {
    const karte::Q3map2Aufruf a = aufruf();
    std::string pak;
    for (const std::string& p : e().zusatzPfade) {
        if (!p.empty()) { pak += " -fs_pakpath \"" + p + "\""; }
    }
    return "\"" + a.programm + "\" -v -game " + a.spielTyp + " -fs_basepath \"" + a.enginePfad + "\" -fs_homepath \"\" -fs_game " + a.fsGame + pak + " " + rest;
}

// Ein Ergebnis ohne Befund (Licht, Kollision): in die Konsole und in den Dialog
void notiz(const std::string& t) {
    g_st.befunde.push_back(t);
    ausgabe(t);
}

void befund(const std::string& t, bool fehlerArt) {
    g_st.befunde.push_back(t);
    if (fehlerArt) {
        ++g_st.fehler;
        fehler(t);
    } else {
        warnung(t);
    }
}

// Die Ebene als eigene Karte: worldspawn (mit Licht der Vorlage) und was in der Ebene liegt
bool teilKarteSchreiben(int ebene, const std::string& karte) {
    Editor& ed = e();
    const karte::Karte& k = ed.karte;
    std::string text = karte::karteSchreiben(
        k, [ebene](const karte::Entitaet& en) {
            if (en.klassenname() == "worldspawn") { return true; }
            if (!en.kinder().empty()) {
                for (const auto& p : en.kinder()) {
                    if (p->schicht == ebene) { return true; }
                }
                return false;
            }
            return en.schicht == ebene;
        },
        [ebene](const karte::Entitaet&, const karte::Primitiv& p) { return p.schicht == ebene; });
    // Kasten der Ebene
    double mins[3] = {1e30, 1e30, 1e30};
    double maxs[3] = {-1e30, -1e30, -1e30};
    for (const auto& en : k.entitaeten()) {
        for (const auto& p : en->kinder()) {
            if (p->schicht != ebene) { continue; }
            const karte::Kasten b = p->art() == karte::Primitiv::Art::Pinsel ? static_cast<const karte::Pinsel&>(*p).grenzen(k.kontext)
                                                                               : static_cast<const karte::Patch&>(*p).grenzen();
            for (int i = 0; i < 3; ++i) {
                mins[i] = std::min(mins[i], static_cast<double>(b.origin[static_cast<std::size_t>(i)] - b.extents[static_cast<std::size_t>(i)]));
                maxs[i] = std::max(maxs[i], static_cast<double>(b.origin[static_cast<std::size_t>(i)] + b.extents[static_cast<std::size_t>(i)]));
            }
        }
    }
    if (mins[0] > maxs[0]) { return false; }   // nichts in der Ebene
    if (g_d.lichtGitter || g_d.lichtLichter) {
        const karte::MergeLicht l = karte::mergeLichtAusHauptkarte(g_d.vorlageDaten, mins, maxs, 512.0);
        std::string welt;
        if (g_d.lichtGitter && l.gitterPunkte > 0) {
            char z[160];
            std::snprintf(z, sizeof(z), "\"_ambient\" \"%.1f\"\n\"_color\" \"%.3f %.3f %.3f\"\n", l.ambient, l.farbe[0], l.farbe[1], l.farbe[2]);
            welt = z;
        }
        const std::size_t p = text.find("\"classname\" \"worldspawn\"\n");
        if (p != std::string::npos && !welt.empty()) { text.insert(p + 25, welt); }
        if (g_d.lichtLichter) {
            for (const auto& kv : l.lichter) {
                text += "{\n";
                for (const auto& [a, b] : kv) { text += "\"" + a + "\" \"" + b + "\"\n"; }
                text += "}\n";
            }
        }
        char z[200];
        std::snprintf(z, sizeof(z), trK(Str::KeZfLicht), l.ambient, l.gitterPunkte, static_cast<int>(l.lichter.size()));
        notiz(z);
    }
    return schreiben(karte, karte::textmodusSchreiben(text));
}

void nachZusammenfuehren(int art);

void nachTeilBau(int art, bool zusammen) {
    if (art != 0) {
        g_st.stand = 3;
        befund(trK(Str::KeZfBauFehler), true);
        return;
    }
    const std::string teil = g_d.ordner + "/" + g_d.stamm + "_teil.bsp";
    std::string teilDaten;
    if (!lesen(teil, teilDaten)) {
        g_st.stand = 3;
        befund(trK(Str::KeZfBauFehler), true);
        return;
    }
    bool sperre = false;
    for (const karte::MergeBefund& b : karte::mergeVorPruefen(g_d.vorlageDaten, teilDaten)) {
        befund(b.text, b.schwere == karte::MergeBefund::Schwere::Fehler);
        sperre = sperre || b.schwere == karte::MergeBefund::Schwere::Fehler;
    }
    if (!zusammen) {
        g_st.stand = 2;
        ausgabe(trK(Str::KeZfGeprueft));
        return;
    }
    if (sperre) {
        g_st.stand = 3;
        befund(trK(Str::KeZfGesperrt), true);
        return;
    }
    const std::string haupt = g_d.ordner + "/" + g_d.stamm + ".bsp";
    bauEigeneBefehle(trK(Str::KeZfTitel), {karte::mergeBefehl(aufruf(), haupt, teil)}, [](int a) { nachZusammenfuehren(a); });
}

void nachZusammenfuehren(int art) {
    const std::string merged = karte::mergeZiel(g_d.ordner + "/" + g_d.stamm + ".bsp");
    std::string m;
    std::string teilDaten;
    if (art != 0 || !lesen(merged, m) || !lesen(g_d.ordner + "/" + g_d.stamm + "_teil.bsp", teilDaten)) {
        g_st.stand = 3;
        befund(trK(Str::KeZfBauFehler), true);
        return;
    }
    g_st.ergebnis = merged;
    // Grenzen der zusammengefuehrten Karte (die Zaehlung des Optimierers aus der .bsp)
    karte::ZaehlOptionen zo;
    zo.spiel = karte::grenzSpielFuerRegelwerk(kartenSpielNummer());
    const karte::BspAnalyse an = karte::analysiereBsp(m, zo);
    g_st.grenzenUeber = 0;
    for (const karte::Bewertung& b : karte::bewerten(zo.spiel, an.werte, 0.9)) {
        if (b.ampel != karte::Ampel::Ueber || b.grenze == nullptr) { continue; }
        ++g_st.grenzenUeber;
        befund(b.grenze->name + ": " + std::to_string(b.zaehlung.wert) + " > " + std::to_string(b.grenze->erlaubt), true);
    }
    // Kollision der eingefuegten Welt-Brushes im CM-Port
    const karte::MergeKollision k = karte::mergeKollisionPruefen(m, karte::weltBrushes(teilDaten));
    g_st.kollisionGeladen = k.geladen;
    g_st.kollisionGesamt = k.gesamt;
    g_st.kollisionStoppen = k.stoppen;
    char z[512];
    std::snprintf(z, sizeof(z), trK(Str::KeZfKollision), k.stoppen, k.gesamt, k.nichtPruefbar);
    if (!k.geladen || k.stoppen < k.gesamt) {
        befund(k.geladen ? std::string(z) : std::string(trK(Str::KeZfNichtGeladen)), true);
    } else {
        notiz(z);
    }
    std::snprintf(z, sizeof(z), trK(Str::KeZfFertig), mitSchraeg(merged).c_str());
    ausgabe(z);
    g_st.stand = g_st.fehler > 0 ? 3 : 2;
}

}  // namespace

const ZusammenStand& zusammenStand() { return g_st; }

void zusammenDialogOeffnen() {
    g_d.offen = true;
    g_d.ebene = static_cast<int>(e().karte.aktuelleSchicht);   // die Ebene, in der man gerade baut
    if (e().karteSpiel) { g_d.liste = karte::bspListe(e().karteSpiel->fs); }
}

void zusammenVorlageWaehlenDatei(const std::string& pfad) {
    std::string d;
    if (!lesen(pfad, d)) {
        fehler("Open file " + mitSchraeg(pfad) + " for read... failure");
        return;
    }
    std::string f;
    if (!vorlageLaden(d, pfad, &f)) {
        fehler("reading " + mitSchraeg(pfad) + ": " + f);
        return;
    }
    g_d.vorlageDaten = std::move(d);
    g_d.vorlageName = mitSchraeg(pfad);
}

namespace {
void vorlageAusSpiel(const std::string& pfad) {
    std::string d;
    if (!e().karteSpiel || !e().karteSpiel->fs.dateiLesen(pfad, d)) {
        fehler("Open file " + pfad + " for read... failure");
        return;
    }
    std::string f;
    if (!vorlageLaden(d, pfad, &f)) {
        fehler("reading " + pfad + ": " + f);
        return;
    }
    g_d.vorlageDaten = std::move(d);
    g_d.vorlageName = pfad;
}

void starten(int ebene, bool zusammen) {
    g_st = ZusammenStand{};
    if (g_d.vorlageDaten.empty()) {
        befund(trK(Str::KeZfKeineVorlage), true);
        g_st.stand = 3;
        return;
    }
    g_d.ordner = arbeitsOrdner();
    std::error_code ec;
    fs::create_directories(pfadVon(g_d.ordner), ec);
    const std::u8string s = pfadVon(g_d.vorlageName).stem().u8string();
    g_d.stamm = std::string(s.begin(), s.end());
    if (!schreiben(g_d.ordner + "/" + g_d.stamm + ".bsp", g_d.vorlageDaten)) {
        befund("writing " + g_d.ordner + "/" + g_d.stamm + ".bsp failure", true);
        g_st.stand = 3;
        return;
    }
    const std::string teil = g_d.ordner + "/" + g_d.stamm + "_teil.map";
    if (!teilKarteSchreiben(ebene, teil)) {
        befund(trK(Str::KeZfLeereEbene), true);
        g_st.stand = 3;
        return;
    }
    g_st.stand = 1;
    bauEigeneBefehle(trK(Str::KeZfTitel), {q3("-meta \"" + teil + "\""), q3("-light -fast -samples 2 \"" + teil + "\"")},
                     [zusammen](int art) { nachTeilBau(art, zusammen); });
}
}  // namespace

void zusammenStarten(int ebene) {
    g_d.ebene = ebene;
    starten(ebene, true);
}

void zusammenDialogZeichnen() {
    if (!g_d.offen) { return; }
    Editor& ed = e();
    const float s = massstab();
    ImGui::SetNextWindowSize(ImVec2{std::round(560.0F * s), std::round(420.0F * s)}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeZfTitel)) + "###kartezusammen";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextWrapped("%s", trK(Str::KeZfErklaerung));
        ImGui::Separator();
        ImGui::Text("%s %s", trK(Str::KeZfVorlage), g_d.vorlageName.empty() ? "-" : g_d.vorlageName.c_str());
        if (ImGui::BeginCombo("##zfspiel", trK(Str::KeZfAusSpiel))) {
            for (int i = 0; i < static_cast<int>(g_d.liste.size()); ++i) {
                if (ImGui::Selectable((g_d.liste[static_cast<std::size_t>(i)].pfad + "##zf" + std::to_string(i)).c_str(), g_d.listenWahl == i)) {
                    g_d.listenWahl = i;
                    vorlageAusSpiel(g_d.liste[static_cast<std::size_t>(i)].pfad);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeBspDatei)) + "##zfdatei").c_str())) {
            const std::string pfad = kartenDateiDialog(true, "Open compiled map", "*.bsp|*.bsp", kartenStartOrdner());
            if (!pfad.empty()) { zusammenVorlageWaehlenDatei(pfad); }
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeZfEntfernen)) + "##zfentfernen").c_str())) {
            vorlageEntfernen();
            g_d.vorlageDaten.clear();
            g_d.vorlageName.clear();
        }
        // Ebene
        const auto& sch = ed.karte.schichten;
        if (g_d.ebene >= static_cast<int>(sch.size())) { g_d.ebene = 0; }
        const std::string aktuell = sch.empty() ? std::string("-") : sch[static_cast<std::size_t>(g_d.ebene)].name;
        if (ImGui::BeginCombo((std::string(trK(Str::KeZfEbene)) + "##zfebene").c_str(), aktuell.c_str())) {
            for (int i = 0; i < static_cast<int>(sch.size()); ++i) {
                if (ImGui::Selectable((sch[static_cast<std::size_t>(i)].name + "##zfe" + std::to_string(i)).c_str(), g_d.ebene == i)) { g_d.ebene = i; }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox((std::string(trK(Str::KeZfLichtGitter)) + "##zflg").c_str(), &g_d.lichtGitter);
        ImGui::Checkbox((std::string(trK(Str::KeZfLichtLichter)) + "##zfll").c_str(), &g_d.lichtLichter);
        const bool baut = bauLaeuft() || g_st.stand == 1;
        if (baut) { ImGui::BeginDisabled(); }
        if (ImGui::Button((std::string(trK(Str::KeZfPruefen)) + "##zfpruefen").c_str())) { starten(g_d.ebene, false); }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeZfBauen)) + "##zfbauen").c_str())) { starten(g_d.ebene, true); }
        if (baut) { ImGui::EndDisabled(); }
        ImGui::Separator();
        for (const std::string& b : g_st.befunde) { ImGui::TextWrapped("%s", b.c_str()); }
        if (!g_st.ergebnis.empty()) {
            char z[1024];
            std::snprintf(z, sizeof(z), trK(Str::KeZfFertig), mitSchraeg(g_st.ergebnis).c_str());
            ImGui::TextWrapped("%s", z);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", trK(Str::KeZfWetter));
        ImGui::PopStyleColor();
    }
    ImGui::End();
    if (!offen) { g_d.offen = false; }
}

}  // namespace bhed::gui::karteneditor
