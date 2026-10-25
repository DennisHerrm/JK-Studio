// app_karte_optimierer.cpp - Karten-Editor, Stufe 5 Punkt 5: das Fenster des
// Optimierers (Build > Optimizer...).
//
// NRC hat keinen Optimierer - eigenes Fenster im Stil der NRC-Dialoge (Qt-
// Regeln: Rand 11, Abstand 6, Knoepfe 80 x 22, Tahoma 8 pt), Farben und
// Knoepfe aus dem Farbschema des Programms. Oben Regelwerk, Filter nach Art,
// "Nur Probleme" und "Pruefen"; zwei Reiter: die Grenzen mit Ampel (Wert,
// erlaubt, Anteil, aus Karte oder BSP) und die Pruefungen mit ihren
// Behebungen; darunter die Einzelheiten der gewaehlten Zeile (Quellen,
// Beitraege bzw. Objekte - ein Klick waehlt sie aus und zentriert die
// Ansichten) und die Knoepfe Beheben / Alles Sichere beheben / Bericht
// exportieren / Schliessen. Gezaehlt und geprueft wird in einem eigenen Faden
// auf einer Momentaufnahme der Karte (karte::kartenAbbild): das Programm
// bleibt bedienbar, auch bei der groessten Karte.
//
// Portiert aus NetRadiant-Custom 20260114 (der Dialogstil: radiant/
// gtkutil/dialog.cpp, die Befehle der Behebungen siehe
// src/karte/optimierer_beheben.cpp) und OpenJK (die Grenzen, siehe
// include/bhed/karte/grenzen.h).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/texturen.h"
#include "platform.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;
using karte::Ampel;
using karte::Befund;
using karte::Behebung;
using karte::Bewertung;
using karte::GrenzArt;
using karte::GrenzSpiel;
using karte::Schwere;

Editor& e() { return editor(); }

fs::path pfadVon(const std::string& p) {
    const std::u8string u(p.begin(), p.end());
    return fs::path(u);
}

bool dateiLesen(const std::string& pfad, std::string& daten) {
    std::ifstream f(pfadVon(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    daten = s.str();
    return true;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string dateiname(const std::string& pfad) {
    const std::u8string u = pfadVon(pfad).filename().u8string();
    return std::string(u.begin(), u.end());
}
std::string stamm(const std::string& pfad) {
    const std::u8string u = pfadVon(pfad).stem().u8string();
    return std::string(u.begin(), u.end());
}

// --- Zuordnungen fuer die Anzeige ------------------------------------------------

struct PruefText {
    const char* id;
    Str titel;
    Str erklaerung;
};
constexpr PruefText kPruefTexte[] = {
    {"leck", Str::KeOptPrLeck, Str::KeOptPrLeckE},
    {"detailamleck", Str::KeOptPrDetailLeck, Str::KeOptPrDetailLeckE},
    {"verdeckt", Str::KeOptPrVerdeckt, Str::KeOptPrVerdecktE},
    {"entartet", Str::KeOptPrEntartet, Str::KeOptPrEntartetE},
    {"mikro", Str::KeOptPrMikro, Str::KeOptPrMikroE},
    {"offgrid", Str::KeOptPrOffGrid, Str::KeOptPrOffGridE},
    {"mischinhalt", Str::KeOptPrMisch, Str::KeOptPrMischE},
    {"areaportalentity", Str::KeOptPrPortalEnt, Str::KeOptPrPortalEntE},
    {"deko", Str::KeOptPrDeko, Str::KeOptPrDekoE},
    {"blockgitter", Str::KeOptPrBlock, Str::KeOptPrBlockE},
    {"ueberlappung", Str::KeOptPrUeberlapp, Str::KeOptPrUeberlappE},
    {"tjunction", Str::KeOptPrTJunction, Str::KeOptPrTJunctionE},
    {"patchgross", Str::KeOptPrPatch, Str::KeOptPrPatchE},
    {"patchgitter", Str::KeOptPrPatchGitter, Str::KeOptPrPatchGitterE},
    {"clipmodell", Str::KeOptPrClip, Str::KeOptPrClipE},
    {"textur", Str::KeOptPrTextur, Str::KeOptPrTexturE},
    {"leereentity", Str::KeOptPrLeereEnt, Str::KeOptPrLeereEntE},
    {"lichtgitter", Str::KeOptPrLichtgitter, Str::KeOptPrLichtgitterE},
    {"lightmapscale", Str::KeOptPrLightmaps, Str::KeOptPrLightmapsE},
    {"vis", Str::KeOptPrVis, Str::KeOptPrVisE},
    {"dreiecke", Str::KeOptPrDreiecke, Str::KeOptPrDreieckeE},
    {"spawnimfesten", Str::KeOptPrSpawn, Str::KeOptPrSpawnE},
    {"modellezusammen", Str::KeOptPrModelle, Str::KeOptPrModelleE},
};
const PruefText* pruefText(const std::string& id) {
    for (const PruefText& p : kPruefTexte) {
        if (id == p.id) { return &p; }
    }
    return nullptr;
}
const char* befundTitel(const Befund& b) {
    const PruefText* p = pruefText(b.pruefung);
    return p != nullptr ? trK(p->titel) : b.pruefung.c_str();
}
const char* behebungText(Behebung b) {
    switch (b) {
    case Behebung::Keine: return trK(Str::KeOptBhKeine);
    case Behebung::Caulk: return trK(Str::KeOptBhCaulk);
    case Behebung::Detail: return trK(Str::KeOptBhDetail);
    case Behebung::Struktur: return trK(Str::KeOptBhStruktur);
    case Behebung::Einrasten: return trK(Str::KeOptBhEinrasten);
    case Behebung::PinselEntfernen: return trK(Str::KeOptBhPinsel);
    case Behebung::PatchTeilen: return trK(Str::KeOptBhPatch);
    case Behebung::ModelleZusammenlegen: return trK(Str::KeOptBhModelle);
    case Behebung::WeltSchluessel: return trK(Str::KeOptBhWelt);
    case Behebung::EntitaetEntfernen: return trK(Str::KeOptBhEntitaet);
    }
    return "";
}
const char* ampelText(Ampel a) {
    switch (a) {
    case Ampel::Ok: return trK(Str::KeOptOk);
    case Ampel::Knapp: return trK(Str::KeOptKnapp);
    case Ampel::Ueber: return trK(Str::KeOptUeber);
    case Ampel::Hinweis: return trK(Str::KeOptHinweis);
    }
    return "";
}
const char* schwereText(Schwere s) {
    switch (s) {
    case Schwere::Fehler: return trK(Str::KeOptFehler);
    case Schwere::Warnung: return trK(Str::KeOptWarnung);
    case Schwere::Hinweis: return trK(Str::KeOptHinweis);
    }
    return "";
}
// Ampelfarben: auf hellem Grund dunkler (Kontrast), auf dunklem heller
ImU32 ampelFarbe(Ampel a) {
    const bool hell = g_app != nullptr && g_app->activeTheme != nullptr && theme::relativeLuminance(g_app->activeTheme->child) > 0.4F;
    switch (a) {
    case Ampel::Ok: return hell ? IM_COL32(30, 130, 50, 255) : IM_COL32(80, 190, 100, 255);
    case Ampel::Knapp: return hell ? IM_COL32(180, 120, 0, 255) : IM_COL32(240, 180, 40, 255);
    case Ampel::Ueber: return hell ? IM_COL32(190, 20, 20, 255) : IM_COL32(240, 80, 70, 255);
    case Ampel::Hinweis: return hell ? IM_COL32(40, 90, 180, 255) : IM_COL32(110, 160, 240, 255);
    }
    return IM_COL32(128, 128, 128, 255);
}
Ampel ampelVonSchwere(Schwere s) {
    return s == Schwere::Fehler ? Ampel::Ueber : (s == Schwere::Warnung ? Ampel::Knapp : Ampel::Hinweis);
}
int rangAmpel(Ampel a) { return a == Ampel::Ueber ? 0 : (a == Ampel::Knapp ? 1 : (a == Ampel::Hinweis ? 2 : 3)); }

// --- Zustand ---------------------------------------------------------------------

struct Ergebnis {
    std::shared_ptr<karte::KartenAbbild> abbild;
    GrenzSpiel spiel = GrenzSpiel::JkaSp;
    std::vector<Bewertung> bewertungen;
    std::vector<Befund> befunde;
    karte::BspAnalyse bsp;
    std::string bspPfad;
    std::string karteName;
    double ms = 0;
    std::vector<std::pair<std::string, double>> zeiten;
};

struct Zustand {
    bool offen = false;
    bool neuGeoeffnet = false;
    std::thread faden;
    std::atomic<bool> fertig{false};
    bool laeuft = false;
    bool nachBau = false;   // vom Bau gestartet
    std::mutex sperre;
    std::unique_ptr<Ergebnis> neu;   // vom Faden
    std::unique_ptr<Ergebnis> e;     // angezeigt
    int reiter = 0;                  // 0 Grenzen, 1 Pruefungen
    bool reiterSetzen = false;       // der Reiter wurde von aussen gewechselt (Selbsttest, Klick auf einen Verweis)
    int artFilter = 0;               // 0 alle, 1 + GrenzArt
    bool nurProbleme = true;
    std::string wahlGrenze;          // Id der gewaehlten Grenze
    int wahlQuelle = 0;              // 0 Karte, 1 BSP
    std::string wahlBefund;          // Id des gewaehlten Befunds
    bool vorschau = false;           // "Beheben..." gedrueckt: Vorschau mit Anwenden
    int laeufe = 0;
    std::string meldung;             // letzte Meldung (Behebung)
};
Zustand g_z;

std::map<std::string, std::string> g_npcModelle;   // NPC_type (klein) -> playerModel
bool g_npcGelesen = false;

GrenzSpiel spielWahl() {
    const int w = e().optimierer.regelwerk;
    if (w >= 0 && w < karte::kGrenzSpiele) { return static_cast<GrenzSpiel>(w); }
    return karte::grenzSpielFuerRegelwerk(kartenSpielNummer());
}

// Die gebaute BSP zur Karte (neben der gebauten Datei), sonst die
// Original-BSP einer geoeffneten .bsp (Stufe 4: <Arbeitsordner>/<Name>_original.bsp).
std::string bspZurKarte() {
    std::error_code ec;
    const std::string karte = bauKartenPfad();
    const std::size_t punkt = karte.rfind('.');
    if (punkt != std::string::npos) {
        const std::string bsp = karte.substr(0, punkt) + ".bsp";
        if (fs::is_regular_file(pfadVon(bsp), ec)) { return bsp; }
    }
    const BspHerkunft& h = e().bspHerkunft;
    if (h.aktiv) {
        const std::string original = h.ordner + "/" + h.stamm + "_original.bsp";
        if (fs::is_regular_file(pfadVon(original), ec)) { return original; }
    }
    return std::string();
}

std::string nebenDatei(const std::string& bsp, const char* endung) {
    const std::size_t punkt = bsp.rfind('.');
    return punkt == std::string::npos ? std::string() : bsp.substr(0, punkt) + endung;
}

// Die Shader der Momentaufnahme im Texturinventar nachsehen (Hauptfaden).
void shaderArten(const karte::KartenAbbild& a, karte::PruefOptionen& o) {
    const auto& spiel = e().karteSpiel;
    if (!spiel) { return; }
    std::set<std::string> namen;
    for (const auto& en : a.ents) {
        for (const auto& p : en.prims) {
            if (p.patch) { namen.insert(klein(p.shader)); }
            for (const auto& f : p.flaechen) { namen.insert(klein(f.shader)); }
        }
    }
    for (const std::string& n : namen) {
        karte::ShaderArt art = karte::shaderArtNachName(n);
        const karte::ShaderEintrag* s = spiel->inventar.shaderFinden(n);
        if (s != nullptr && !s->istStandard()) {
            using namespace karte;
            const unsigned f = s->flags;
            if ((f & (QER_TRANS | QER_NONSOLID | QER_FOG | QER_LIQUID | QER_ALPHATEST | QER_AREAPORTAL | QER_CLIP | QER_BOTCLIP)) != 0) {
                art.undurchsichtig = false;
            }
            if ((f & QER_NODRAW) != 0) { art.sichtbar = false; }
            if ((f & (QER_CLIP | QER_BOTCLIP | QER_AREAPORTAL)) != 0) {
                art.werkzeug = true;
                art.sichtbar = false;
            }
            art.bekannt = true;
        } else {
            art.bekannt = spiel->inventar.bildMass(n).gefunden;
        }
        o.shader[n] = art;
    }
    o.shaderBekanntGeprueft = true;
}

void npcModelleSicherstellen() {
    if (g_npcGelesen || g_app == nullptr) { return; }
    g_npcGelesen = true;
    NpcMap m;
    if (g_app->npcMapRead) {
        m = g_app->npcMap;
    } else {
        (void)npcDateienLesen(m);
    }
    for (const auto& [typ, def] : m) {
        const std::string modell = !def.playerModel.empty() ? def.playerModel : def.legsModel;
        if (!modell.empty()) { g_npcModelle[klein(typ)] = klein(modell); }
    }
}

void fadenAbwarten() {
    if (g_z.faden.joinable()) { g_z.faden.join(); }
}

// Ergebnis abholen (Hauptfaden)
void abholen() {
    if (!g_z.laeuft || !g_z.fertig.load()) { return; }
    fadenAbwarten();
    g_z.laeuft = false;
    {
        std::lock_guard<std::mutex> l(g_z.sperre);
        g_z.e = std::move(g_z.neu);
    }
    ++g_z.laeufe;
    if (!g_z.e) { return; }
    // Zusammenfassung in die Konsole
    int ueber = 0;
    int knapp = 0;
    int fehler_ = 0;
    int warn = 0;
    for (const Bewertung& b : g_z.e->bewertungen) {
        if (b.ampel == Ampel::Ueber) { ++ueber; }
        if (b.ampel == Ampel::Knapp) { ++knapp; }
    }
    for (const Befund& b : g_z.e->befunde) {
        if (b.schwere == Schwere::Fehler) { ++fehler_; }
        if (b.schwere == Schwere::Warnung) { ++warn; }
    }
    char z[256];
    std::snprintf(z, sizeof(z), trK(Str::KeOptZusammenfassung), ueber, knapp, fehler_, warn);
    if (ueber + fehler_ > 0) {
        warnung(z);
    } else {
        ausgabe(z);
    }
    if (g_z.nachBau && (ueber + fehler_ > 0)) { g_z.offen = true; }   // nach dem Bau: nur bei Problemen von selbst auf
    g_z.nachBau = false;
}

bool veraltet() {
    return g_z.e && g_z.e->abbild && !karte::abbildPasst(e().karte, *g_z.e->abbild);
}

// Die Objekte auswaehlen und die Ansichten zentrieren.
void auswaehlen(const std::vector<karte::ObjektRef>& refs) {
    if (!g_z.e || !g_z.e->abbild || veraltet()) { return; }
    Editor& ed = e();
    const auto& ents = ed.karte.entitaeten();
    ed.karte.auswahl.leeren();
    int n = 0;
    for (const karte::ObjektRef& r : refs) {
        if (r.entitaet < 0 || r.entitaet >= static_cast<int>(ents.size())) { continue; }
        const auto& en = ents[static_cast<std::size_t>(r.entitaet)];
        if (r.primitiv >= 0) {
            if (r.primitiv >= static_cast<int>(en->kinder().size())) { continue; }
            ed.karte.auswahl.waehlen(en, en->kinder()[static_cast<std::size_t>(r.primitiv)]);
        } else {
            ed.karte.auswahl.waehlen(en);
        }
        ++n;
    }
    auswahlGeaendert();
    if (n > 0) {
        (void)ausfuehren("XYFocusOnSelected");      // xywindow.cpp:2278
        (void)ausfuehren("CameraFocusOnSelected");  // camwindow.cpp:2489
    }
}

const Bewertung* gewaehlteGrenze() {
    if (!g_z.e) { return nullptr; }
    for (const Bewertung& b : g_z.e->bewertungen) {
        if (b.zaehlung.id == g_z.wahlGrenze && static_cast<int>(b.zaehlung.quelle) == g_z.wahlQuelle) { return &b; }
    }
    return nullptr;
}
const Befund* gewaehlterBefund() {
    if (!g_z.e) { return nullptr; }
    for (const Befund& b : g_z.e->befunde) {
        if (b.pruefung == g_z.wahlBefund) { return &b; }
    }
    return nullptr;
}

std::vector<const Bewertung*> sichtbareGrenzen() {
    std::vector<const Bewertung*> v;
    if (!g_z.e) { return v; }
    for (const Bewertung& b : g_z.e->bewertungen) {
        if (g_z.nurProbleme && b.ampel == Ampel::Ok) { continue; }
        if (g_z.artFilter > 0 && static_cast<int>(b.grenze->art) != g_z.artFilter - 1) { continue; }
        v.push_back(&b);
    }
    std::stable_sort(v.begin(), v.end(), [](const Bewertung* a, const Bewertung* b) {
        if (rangAmpel(a->ampel) != rangAmpel(b->ampel)) { return rangAmpel(a->ampel) < rangAmpel(b->ampel); }
        return a->anteil > b->anteil;
    });
    return v;
}
std::vector<const Befund*> sichtbareBefunde() {
    std::vector<const Befund*> v;
    if (!g_z.e) { return v; }
    for (const Befund& b : g_z.e->befunde) { v.push_back(&b); }
    std::stable_sort(v.begin(), v.end(), [](const Befund* a, const Befund* b) { return static_cast<int>(a->schwere) > static_cast<int>(b->schwere); });
    return v;
}

void nachBehebung(const karte::BehebungsErgebnis& r, Behebung art) {
    if (!r.undoName.empty() && r.geaendert > 0) {
        ausgabe(r.undoName);
        g_z.meldung = r.undoName;
    } else {
        g_z.meldung = r.meldung;
    }
    if (!r.meldung.empty()) { warnung("Optimizer: " + r.meldung); }
    if (art == Behebung::WeltSchluessel && r.geaendert > 0) { ausgabe(trK(Str::KeOptNeuBauen)); }
    auswahlGeaendert();
    optimiererStarten(false);   // danach wird neu gezaehlt
}

void zeichneAmpelPunkt(ImDrawList* dl, ImVec2 p, float r, Ampel a) {
    dl->AddCircleFilled(ImVec2{p.x + r, p.y + r}, r, ampelFarbe(a));
}

}  // namespace

// --- Starten, Abholen ---------------------------------------------------------------

void optimiererStarten(bool nachBau) {
    abholen();
    if (g_z.laeuft) { return; }   // eine Pruefung zur Zeit
    fadenAbwarten();
    Editor& ed = e();
    auto abbild = std::make_shared<karte::KartenAbbild>(karte::kartenAbbild(ed.karte));
    const GrenzSpiel spiel = spielWahl();
    npcModelleSicherstellen();
    karte::ZaehlOptionen zo;
    zo.spiel = spiel;
    std::map<std::string, std::string> npc = g_npcModelle;
    zo.npcModell = [npc](const std::string& typ) {
        const auto it = npc.find(klein(typ));
        return it != npc.end() ? it->second : std::string();
    };
    karte::PruefOptionen po;
    po.spiel = spiel;
    po.knappAb = std::clamp(ed.optimierer.knappProzent, 1, 100) / 100.0;
    po.portalSchwelle = ed.optimierer.portalSchwelle;
    po.dreieckSchwelle = ed.optimierer.dreieckSchwelle;
    shaderArten(*abbild, po);
    if (!abbild->ents.empty()) {
        const std::string bs = abbild->ents.front().wert("_blocksize");
        if (!bs.empty()) { po.blockGroesse = std::atoi(bs.c_str()); }
    }
    const std::string bspPfad = bspZurKarte();
    // die .lin (Leck) nur, wenn sie zur gebauten Datei gehoert
    const std::string karteGebaut = bauKartenPfad();
    std::string lin;
    if (karteGebaut.rfind('.') != std::string::npos) { (void)dateiLesen(karteGebaut.substr(0, karteGebaut.rfind('.')) + ".lin", lin); }
    po.linDaten = lin;
    const double knapp = po.knappAb;
    const std::string name = ed.name;
    g_z.fertig = false;
    g_z.laeuft = true;
    g_z.nachBau = nachBau;
    g_z.faden = std::thread([abbild, zo, po, bspPfad, knapp, spiel, name]() mutable {
        const auto t0 = std::chrono::steady_clock::now();
        auto r = std::make_unique<Ergebnis>();
        r->abbild = abbild;
        r->spiel = spiel;
        r->karteName = name;
        std::vector<karte::Zaehlung> z = karte::zaehleKarte(*abbild, zo);
        std::string bspDaten;
        if (!bspPfad.empty() && dateiLesen(bspPfad, bspDaten)) {
            std::string prt;
            (void)dateiLesen(nebenDatei(bspPfad, ".prt"), prt);
            r->bsp = karte::analysiereBsp(bspDaten, zo, prt);
            if (r->bsp.ok) {
                r->bspPfad = bspPfad;
                z.insert(z.end(), r->bsp.werte.begin(), r->bsp.werte.end());
            }
        }
        r->bewertungen = karte::bewerten(spiel, z, knapp);
        po.bewertungen = r->bewertungen;
        po.bsp = r->bsp.ok ? &r->bsp : nullptr;
        po.bspDaten = r->bsp.ok ? &bspDaten : nullptr;
        po.zeiten = &r->zeiten;
        r->befunde = karte::pruefeKarte(*abbild, po);
        po.bsp = nullptr;
        po.bspDaten = nullptr;
        po.zeiten = nullptr;
        r->ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        {
            std::lock_guard<std::mutex> l(g_z.sperre);
            g_z.neu = std::move(r);
        }
        g_z.fertig = true;
    });
}

void optimiererOeffnen() {
    g_z.offen = true;
    g_z.neuGeoeffnet = true;
    optimiererStarten(false);
}

bool optimiererLaeuft() { return g_z.laeuft; }
bool optimiererOffen() { return g_z.offen; }
void optimiererSchliessen() { g_z.offen = false; }
void optimiererBeenden() { fadenAbwarten(); }

// --- Bericht ---------------------------------------------------------------------------

std::string optimiererBericht() {
    std::ostringstream o;
    if (!g_z.e) {
        o << trK(Str::KeOptNochNicht) << "\n";
        return o.str();
    }
    const Ergebnis& r = *g_z.e;
    o << trK(Str::KeOptTitel) << " - " << r.karteName << "\n";
    o << trK(Str::KeOptRegelwerk) << " " << karte::grenzSpielName(r.spiel) << "\n";
    o << "BSP: " << (r.bspPfad.empty() ? std::string("-") : r.bspPfad) << "\n";
    char z[128];
    std::snprintf(z, sizeof(z), "%.0f ms", r.ms);
    o << z << "\n\n== " << trK(Str::KeOptReiterGrenzen) << " ==\n";
    std::vector<const Bewertung*> v;
    for (const Bewertung& b : r.bewertungen) { v.push_back(&b); }
    std::stable_sort(v.begin(), v.end(), [](const Bewertung* a, const Bewertung* b) {
        if (rangAmpel(a->ampel) != rangAmpel(b->ampel)) { return rangAmpel(a->ampel) < rangAmpel(b->ampel); }
        return a->anteil > b->anteil;
    });
    for (const Bewertung* b : v) {
        const karte::Grenze& g = *b->grenze;
        std::snprintf(z, sizeof(z), "%5.1f %%", b->anteil * 100.0);
        o << "[" << ampelText(b->ampel) << "] " << trK(g.titel) << " (" << g.name << "): " << b->zaehlung.wert << " / " << g.erlaubt << "  " << z << "  "
          << (b->zaehlung.quelle == karte::ZaehlQuelle::Bsp ? trK(Str::KeOptAusBsp) : trK(Str::KeOptAusKarte))
          << (b->zaehlung.schaetzung ? std::string(", ") + trK(Str::KeOptSchaetzung) : std::string()) << "\n";
        for (const karte::GrenzStelle& s : g.stellen) { o << "    " << s.baum << " " << s.datei << ":" << s.zeile << " " << s.name << " = " << s.wert << "\n"; }
        o << "    " << trK(Str::KeOptPruefstelle) << " " << g.pruefstelle << "\n";
        if (!b->zaehlung.hinweis.empty()) { o << "    " << trK(Str::KeOptGezaehlt) << " " << b->zaehlung.hinweis << "\n"; }
    }
    o << "\n== " << trK(Str::KeOptReiterBefunde) << " ==\n";
    for (const Befund* b : sichtbareBefunde()) {
        o << "[" << schwereText(b->schwere) << "] " << befundTitel(*b) << ": " << b->anzahl << " - " << b->detail << "\n";
        const PruefText* p = pruefText(b->pruefung);
        if (p != nullptr) { o << "    " << trK(p->erklaerung) << "\n"; }
        o << "    " << trK(Str::KeOptQuelle) << " " << b->quelle << "\n";
        o << "    " << trK(Str::KeOptVorschau) << " " << behebungText(b->behebung) << (b->sicher ? std::string(" (") + trK(Str::KeOptSicher) + ")" : std::string())
          << " - " << karte::behebungsVorschau(*b) << "\n";
        std::size_t n = 0;
        for (const karte::ObjektRef& r2 : b->objekte) {
            if (++n > 20) {
                o << "      ...\n";
                break;
            }
            o << "      " << r2.text << "\n";
        }
    }
    return o.str();
}

bool optimiererBerichtSchreiben(const std::string& pfad) {
    std::ofstream f(pfadVon(pfad), std::ios::binary | std::ios::trunc);
    if (!f) { return false; }
    const std::string t = optimiererBericht();
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
    return static_cast<bool>(f);
}

// --- Selbsttest ---------------------------------------------------------------------------

OptimiererBlick optimiererBlick() {
    OptimiererBlick b;
    b.laeufe = g_z.laeufe;
    if (!g_z.e) { return b; }
    b.hatErgebnis = true;
    b.veraltet = veraltet();
    b.ms = g_z.e->ms;
    b.bsp = g_z.e->bspPfad;
    for (const Bewertung& x : g_z.e->bewertungen) {
        if (x.ampel == Ampel::Ueber) { ++b.grenzenUeber; }
        if (x.ampel == Ampel::Knapp) { ++b.grenzenKnapp; }
        if (x.ampel != Ampel::Ok) { b.grenzenIds.push_back(x.zaehlung.id + (x.zaehlung.quelle == karte::ZaehlQuelle::Bsp ? "@bsp" : "@map")); }
    }
    for (const Befund* x : sichtbareBefunde()) {
        b.befundIds.push_back(x->pruefung);
        b.befundAnzahl.push_back(x->anzahl);
    }
    b.befunde = static_cast<int>(b.befundIds.size());
    return b;
}

void optimiererReiter(int reiter) {
    g_z.reiter = std::clamp(reiter, 0, 1);
    g_z.reiterSetzen = true;
}

bool optimiererZeileWaehlen(int reiter, const std::string& id) {
    abholen();
    if (!g_z.e) { return false; }
    if (g_z.reiter != reiter) { optimiererReiter(reiter); }
    g_z.vorschau = false;
    if (reiter == 0) {
        const std::size_t at = id.find('@');
        const std::string gid = id.substr(0, at);
        const int quelle = at != std::string::npos && id.substr(at + 1) == "bsp" ? 1 : 0;
        for (const Bewertung& b : g_z.e->bewertungen) {
            if (b.zaehlung.id == gid && static_cast<int>(b.zaehlung.quelle) == quelle) {
                g_z.wahlGrenze = gid;
                g_z.wahlQuelle = quelle;
                if (quelle == 0) {
                    std::vector<karte::ObjektRef> refs;
                    for (const karte::Beitrag& x : b.zaehlung.beitraege) { refs.push_back(x.ref); }
                    auswaehlen(refs);
                }
                return true;
            }
        }
        return false;
    }
    for (const Befund& b : g_z.e->befunde) {
        if (b.pruefung == id) {
            g_z.wahlBefund = id;
            auswaehlen(b.objekte);
            return true;
        }
    }
    return false;
}

bool optimiererBefundBeheben(const std::string& id) {
    abholen();
    if (!g_z.e || veraltet()) { return false; }
    for (const Befund& b : g_z.e->befunde) {
        if (b.pruefung != id) { continue; }
        const karte::BehebungsErgebnis r = karte::beheben(e().karte, *g_z.e->abbild, b);
        const Behebung art = b.behebung;
        nachBehebung(r, art);
        return r.ok && r.geaendert > 0;
    }
    return false;
}

int optimiererAllesSichere() {
    abholen();
    if (!g_z.e || veraltet()) { return 0; }
    const auto r = karte::allesSichereBeheben(e().karte, *g_z.e->abbild, g_z.e->befunde);
    int n = 0;
    std::string liste;
    for (const auto& [id, x] : r) {
        if (x.geaendert <= 0) { continue; }
        ++n;
        const PruefText* p = pruefText(id);
        liste += (liste.empty() ? "" : ", ") + std::string(p != nullptr ? trK(p->titel) : id.c_str()) + " (" + std::to_string(x.geaendert) + ")";
    }
    if (n == 0) {
        ausgabe(trK(Str::KeOptNichtsSicher));
        g_z.meldung = trK(Str::KeOptNichtsSicher);
        return 0;
    }
    char z[1024];
    std::snprintf(z, sizeof(z), trK(Str::KeOptSichereListe), liste.c_str());
    ausgabe(z);
    g_z.meldung = z;
    auswahlGeaendert();
    optimiererStarten(false);
    return n;
}

// --- Das Fenster ------------------------------------------------------------------------

void zeichneOptimierer() {
    abholen();
    if (!g_z.offen) { return; }
    const float sk = massstab();
    auto px = [sk](float v) { return std::round(v * sk); };
    ImGui::SetNextWindowSize(ImVec2{px(760.0F), px(560.0F)}, ImGuiCond_FirstUseEver);
    if (g_z.neuGeoeffnet) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
        g_z.neuGeoeffnet = false;
    }
    const std::string titel = std::string(trK(Str::KeOptTitel)) + "###karteoptimierer";
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(11.0F), px(11.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{px(6.0F), px(6.0F)});
    if (!ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        if (!offen) { g_z.offen = false; }
        return;
    }
    dialogSchriftAn();
    const ImVec2 innen = ImGui::GetCursorScreenPos();
    auto merke = [&](const char* name) {
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        merkeFeld(std::string("Optimierer:") + name, static_cast<int>(std::lround((a.x - innen.x) / sk)), static_cast<int>(std::lround((a.y - innen.y) / sk)),
                  static_cast<int>(std::lround((b.x - a.x) / sk)), static_cast<int>(std::lround((b.y - a.y) / sk)));
    };
    const float zeileH = px(22.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(4.0F), std::max(0.0F, (zeileH - ImGui::GetTextLineHeight()) * 0.5F)});
    // --- Kopfzeile: Regelwerk, Art, Nur Probleme, Pruefen ---
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(trK(Str::KeOptRegelwerk));
    ImGui::SameLine();
    {
        const int alt = e().optimierer.regelwerk;
        std::string vorschau = alt < 0 ? std::string(trK(Str::KeOptAuto)) + " (" + karte::grenzSpielName(spielWahl()) + ")"
                                       : karte::grenzSpielName(static_cast<GrenzSpiel>(alt));
        ImGui::SetNextItemWidth(px(250.0F));
        if (ImGui::BeginCombo("##optregelwerk", vorschau.c_str())) {
            if (ImGui::Selectable(trK(Str::KeOptAuto), alt < 0)) { e().optimierer.regelwerk = -1; }
            for (int i = 0; i < karte::kGrenzSpiele; ++i) {
                if (ImGui::Selectable(karte::grenzSpielName(static_cast<GrenzSpiel>(i)), alt == i)) { e().optimierer.regelwerk = i; }
            }
            ImGui::EndCombo();
        }
        merke("Regelwerk");
        if (e().optimierer.regelwerk != alt) { optimiererStarten(false); }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(trK(Str::KeOptZeige));
    ImGui::SameLine();
    {
        const char* art = g_z.artFilter == 0 ? trK(Str::KeOptAlle) : trK(karte::grenzArtTitel(static_cast<GrenzArt>(g_z.artFilter - 1)));
        ImGui::SetNextItemWidth(px(150.0F));
        if (ImGui::BeginCombo("##optart", art)) {
            if (ImGui::Selectable(trK(Str::KeOptAlle), g_z.artFilter == 0)) { g_z.artFilter = 0; }
            for (int i = 0; i < karte::kGrenzArten; ++i) {
                if (ImGui::Selectable(trK(karte::grenzArtTitel(static_cast<GrenzArt>(i))), g_z.artFilter == i + 1)) { g_z.artFilter = i + 1; }
            }
            ImGui::EndCombo();
        }
        merke("Art");
    }
    ImGui::SameLine();
    ImGui::Checkbox((std::string(trK(Str::KeOptNurProbleme)) + "##optnurprobleme").c_str(), &g_z.nurProbleme);
    merke("NurProbleme");
    ImGui::SameLine();
    if (g_z.laeuft) { ImGui::BeginDisabled(); }
    if (ImGui::Button((std::string(trK(Str::KeOptAnalysieren)) + "##optpruefen").c_str(), ImVec2{px(80.0F), zeileH})) { optimiererStarten(false); }
    merke("Pruefen");
    if (g_z.laeuft) { ImGui::EndDisabled(); }
    // --- Stand ---
    {
        std::string stand;
        if (g_z.laeuft) {
            stand = trK(Str::KeOptLaeuft);
        } else if (!g_z.e) {
            stand = trK(Str::KeOptNochNicht);
        } else {
            char z[512];
            std::string quelle;
            if (g_z.e->bspPfad.empty()) {
                quelle = trK(Str::KeOptOhneBsp);
            } else {
                char q[600];
                std::snprintf(q, sizeof(q), trK(Str::KeOptMitBsp), dateiname(g_z.e->bspPfad).c_str());
                quelle = q;
            }
            std::snprintf(z, sizeof(z), trK(Str::KeOptFertig), g_z.e->ms, quelle.c_str());
            stand = z;
        }
        if (veraltet()) {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ampelFarbe(Ampel::Knapp)), "%s", trK(Str::KeOptVeraltet));
        } else {
            ImGui::TextDisabled("%s", stand.c_str());
        }
        merke("Stand");
    }
    // --- Reiter ---
    const float knopfZeile = zeileH + px(6.0F);
    const float rest = ImGui::GetContentRegionAvail().y;
    const float detailH = std::max(px(120.0F), rest * 0.36F);
    const float listeH = std::max(px(80.0F), rest - detailH - knopfZeile - px(30.0F));
    if (ImGui::BeginTabBar("##optreiter")) {
        const bool setzen = g_z.reiterSetzen || ImGui::IsWindowAppearing();
        const int soll = g_z.reiter;
        g_z.reiterSetzen = false;
        const bool w0 = ImGui::BeginTabItem((std::string(trK(Str::KeOptReiterGrenzen)) + "###optgrenzen").c_str(), nullptr,
                                            setzen && soll == 0 ? ImGuiTabItemFlags_SetSelected : 0);
        if (w0) {
            g_z.reiter = 0;
            ImGui::EndTabItem();
        }
        const bool w1 = ImGui::BeginTabItem((std::string(trK(Str::KeOptReiterBefunde)) + " (" + std::to_string(g_z.e ? g_z.e->befunde.size() : 0U) + ")###optbefunde").c_str(),
                                            nullptr, setzen && soll == 1 ? ImGuiTabItemFlags_SetSelected : 0);
        if (w1) {
            g_z.reiter = 1;
            ImGui::EndTabItem();
        }
        if (setzen) { g_z.reiter = soll; }
        ImGui::EndTabBar();
    }
    const float punktR = std::round(ImGui::GetTextLineHeight() * 0.32F);
    const ImGuiTableFlags tf = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (g_z.reiter == 0) {
        if (ImGui::BeginTable("##optgrenzentab", 7, tf, ImVec2{0.0F, listeH})) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, px(16.0F));
            ImGui::TableSetupColumn(trK(Str::KeOptSpGrenze), ImGuiTableColumnFlags_WidthStretch, 4.0F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpArt), ImGuiTableColumnFlags_WidthStretch, 1.4F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpWert), ImGuiTableColumnFlags_WidthStretch, 1.3F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpErlaubt), ImGuiTableColumnFlags_WidthStretch, 1.3F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpAnteil), ImGuiTableColumnFlags_WidthStretch, 1.0F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpAus), ImGuiTableColumnFlags_WidthStretch, 1.2F);
            ImGui::TableHeadersRow();
            for (const Bewertung* b : sichtbareGrenzen()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const ImVec2 p = ImGui::GetCursorScreenPos();
                zeichneAmpelPunkt(ImGui::GetWindowDrawList(), ImVec2{p.x + px(2.0F), p.y + (ImGui::GetTextLineHeight() - 2 * punktR) * 0.5F}, punktR, b->ampel);
                ImGui::TableNextColumn();
                const std::string id = b->zaehlung.id + (b->zaehlung.quelle == karte::ZaehlQuelle::Bsp ? "@bsp" : "@map");
                const bool gewaehlt = g_z.wahlGrenze == b->zaehlung.id && g_z.wahlQuelle == static_cast<int>(b->zaehlung.quelle);
                if (ImGui::Selectable((std::string(trK(b->grenze->titel)) + "##" + id).c_str(), gewaehlt, ImGuiSelectableFlags_SpanAllColumns)) {
                    (void)optimiererZeileWaehlen(0, id);
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(trK(karte::grenzArtTitel(b->grenze->art)));
                ImGui::TableNextColumn();
                ImGui::Text("%lld", b->zaehlung.wert);
                ImGui::TableNextColumn();
                ImGui::Text("%lld", b->grenze->erlaubt);
                ImGui::TableNextColumn();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ampelFarbe(b->ampel)), "%.1f %%", b->anteil * 100.0);
                ImGui::TableNextColumn();
                ImGui::Text("%s%s", b->zaehlung.quelle == karte::ZaehlQuelle::Bsp ? trK(Str::KeOptAusBsp) : trK(Str::KeOptAusKarte),
                            b->zaehlung.schaetzung ? " *" : "");
            }
            ImGui::EndTable();
        }
        merke("Liste");
    } else {
        if (ImGui::BeginTable("##optbefundtab", 5, tf, ImVec2{0.0F, listeH})) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, px(16.0F));
            ImGui::TableSetupColumn(trK(Str::KeOptSpPruefung), ImGuiTableColumnFlags_WidthStretch, 4.0F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpAnzahl), ImGuiTableColumnFlags_WidthStretch, 1.0F);
            ImGui::TableSetupColumn(trK(Str::KeOptSpBehebung), ImGuiTableColumnFlags_WidthStretch, 2.0F);
            ImGui::TableSetupColumn(trK(Str::KeOptSicher), ImGuiTableColumnFlags_WidthStretch, 0.8F);
            ImGui::TableHeadersRow();
            for (const Befund* b : sichtbareBefunde()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const ImVec2 p = ImGui::GetCursorScreenPos();
                zeichneAmpelPunkt(ImGui::GetWindowDrawList(), ImVec2{p.x + px(2.0F), p.y + (ImGui::GetTextLineHeight() - 2 * punktR) * 0.5F}, punktR,
                                  ampelVonSchwere(b->schwere));
                ImGui::TableNextColumn();
                if (ImGui::Selectable((std::string(befundTitel(*b)) + (b->schaetzung ? " *" : "") + "##bf" + b->pruefung).c_str(), g_z.wahlBefund == b->pruefung,
                                      ImGuiSelectableFlags_SpanAllColumns)) {
                    (void)optimiererZeileWaehlen(1, b->pruefung);
                }
                ImGui::TableNextColumn();
                ImGui::Text("%lld", b->anzahl);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(behebungText(b->behebung));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(b->sicher ? trK(Str::KeOptSicher) : "");
            }
            ImGui::EndTable();
        }
        merke("Liste");
    }
    // --- Einzelheiten ---
    ImGui::BeginChild("##optdetail", ImVec2{0.0F, detailH}, ImGuiChildFlags_Borders);
    ImGui::PushTextWrapPos(0.0F);
    if (g_z.reiter == 0) {
        if (const Bewertung* b = gewaehlteGrenze()) {
            const karte::Grenze& g = *b->grenze;
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ampelFarbe(b->ampel)), "%s - %s: %lld / %lld", ampelText(b->ampel), trK(g.titel), b->zaehlung.wert, g.erlaubt);
            ImGui::TextDisabled("%s", trK(Str::KeOptDefine));
            for (const karte::GrenzStelle& s : g.stellen) {
                ImGui::BulletText("%s %s:%d  %s = %lld%s", s.baum.c_str(), s.datei.c_str(), s.zeile, s.name.c_str(), s.wert, s.jk2 ? "  (JK2_MODE)" : "");
            }
            ImGui::Text("%s %s", trK(Str::KeOptPruefstelle), g.pruefstelle.c_str());
            ImGui::Text("%s %s", trK(Str::KeOptRechnung), g.rechnung.c_str());
            if (!g.vanillaQuelle.empty()) { ImGui::Text("%s %lld - %s", trK(Str::KeOptVanilla), g.vanilla, g.vanillaQuelle.c_str()); }
            if (!g.hart) { ImGui::TextDisabled("%s", trK(Str::KeOptWeich)); }
            if (!b->zaehlung.hinweis.empty()) { ImGui::Text("%s %s", trK(Str::KeOptGezaehlt), b->zaehlung.hinweis.c_str()); }
            if (!b->zaehlung.beitraege.empty()) {
                ImGui::TextDisabled("%s", trK(Str::KeOptBeitraege));
                for (std::size_t i = 0; i < b->zaehlung.beitraege.size(); ++i) {
                    const karte::Beitrag& x = b->zaehlung.beitraege[i];
                    const std::string t = std::to_string(x.wert) + "  " + x.ref.text + "##beitrag" + std::to_string(i);
                    if (ImGui::Selectable(t.c_str(), false) && b->zaehlung.quelle == karte::ZaehlQuelle::Karte) { auswaehlen({x.ref}); }
                }
            }
        }
    } else if (const Befund* b = gewaehlterBefund()) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ampelFarbe(ampelVonSchwere(b->schwere))), "%s - %s: %lld", schwereText(b->schwere), befundTitel(*b), b->anzahl);
        if (const PruefText* p = pruefText(b->pruefung)) { ImGui::TextUnformatted(trK(p->erklaerung)); }
        ImGui::Text("%s %s", trK(Str::KeOptQuelle), b->quelle.c_str());
        if (!b->detail.empty()) { ImGui::TextDisabled("%s", b->detail.c_str()); }
        ImGui::Text("%s %s - %s%s", trK(Str::KeOptVorschau), behebungText(b->behebung), karte::behebungsVorschau(*b).c_str(),
                    b->sicher ? (std::string(" (") + trK(Str::KeOptSicher) + ")").c_str() : "");
        ImGui::TextDisabled("%s", trK(Str::KeOptObjekte));
        const std::size_t zeigen = std::min<std::size_t>(b->objekte.size(), 300);
        for (std::size_t i = 0; i < zeigen; ++i) {
            const std::string t = b->objekte[i].text + "##obj" + std::to_string(i);
            if (ImGui::Selectable(t.c_str(), false)) { auswaehlen({b->objekte[i]}); }
        }
        if (b->objekte.size() > zeigen) { ImGui::TextDisabled("... (%zu)", b->objekte.size()); }
    }
    if (!g_z.meldung.empty()) {
        ImGui::Separator();
        ImGui::TextDisabled("%s", g_z.meldung.c_str());
    }
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
    merke("Einzelheiten");
    // --- Knoepfe ---
    const Befund* bf = g_z.reiter == 1 ? gewaehlterBefund() : nullptr;
    const bool darf = bf != nullptr && bf->behebung != Behebung::Keine && !veraltet() && !g_z.laeuft;
    if (!g_z.vorschau) {
        if (!darf) { ImGui::BeginDisabled(); }
        if (ImGui::Button((std::string(trK(Str::KeOptBeheben)) + "##optbeheben").c_str(), ImVec2{px(100.0F), zeileH})) { g_z.vorschau = true; }
        merke("Beheben");
        if (!darf) { ImGui::EndDisabled(); }
    } else {
        if (!darf) { g_z.vorschau = false; }
        if (ImGui::Button((std::string(trK(Str::KeOptAnwenden)) + "##optanwenden").c_str(), ImVec2{px(100.0F), zeileH}) && bf != nullptr) {
            const std::string id = bf->pruefung;
            g_z.vorschau = false;
            (void)optimiererBefundBeheben(id);
        }
        merke("Anwenden");
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##optvorschauweg").c_str(), ImVec2{px(80.0F), zeileH})) { g_z.vorschau = false; }
    }
    ImGui::SameLine();
    const bool sichereDa = g_z.e && !veraltet() && !g_z.laeuft &&
                           std::any_of(g_z.e->befunde.begin(), g_z.e->befunde.end(), [](const Befund& b) { return b.sicher && b.behebung != Behebung::Keine; });
    if (!sichereDa) { ImGui::BeginDisabled(); }
    if (ImGui::Button((std::string(trK(Str::KeOptAllesSichere)) + "##optsicher").c_str(), ImVec2{0.0F, zeileH})) { (void)optimiererAllesSichere(); }
    merke("AllesSichere");
    if (!sichereDa) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    if (!g_z.e) { ImGui::BeginDisabled(); }
    if (ImGui::Button((std::string(trK(Str::KeOptExport)) + "##optexport").c_str(), ImVec2{0.0F, zeileH})) {
        std::string vorschlag = "optimizer.txt";
        if (g_z.e && !stamm(g_z.e->karteName).empty()) { vorschlag = stamm(g_z.e->karteName) + "_optimizer.txt"; }
        const std::string pfad = kartenDateiDialog(false, trK(Str::KeOptExportTitel), "*.txt|*.txt", kartenStartOrdner(), vorschlag);
        if (!pfad.empty() && optimiererBerichtSchreiben(pfad)) {
            char z[1024];
            std::snprintf(z, sizeof(z), trK(Str::KeOptExportiert), pfad.c_str());
            ausgabe(z);
            g_z.meldung = z;
        }
    }
    merke("Export");
    if (!g_z.e) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    const float rechts = ImGui::GetContentRegionMax().x - px(80.0F);
    if (ImGui::GetCursorPosX() < rechts) { ImGui::SetCursorPosX(rechts); }
    if (ImGui::Button((std::string(trK(Str::KeOptSchliessen)) + "##optzu").c_str(), ImVec2{px(80.0F), zeileH})) { offen = false; }
    merke("Schliessen");
    ImGui::PopStyleVar();   // FramePadding
    dialogSchriftAus();
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (!offen) { g_z.offen = false; }
}

}  // namespace bhed::gui::karteneditor
