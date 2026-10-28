// karteoptimierer.cpp - Karten-Editor Stufe 5: der Optimierer
// (bhed/karte/optimierer.h, patchkollision.h).
//
// Punkt 2 (Zaehlen): Testkarten, die jede Grenze knapp unter- und
// ueberschreiten (die Ampel muss von "ok"/"knapp" auf "ueber" springen), eine
// erzeugte RBSP fuer die Lumps (Lichtgitter, Lightmaps, Nebel, Flaechen,
// Modelle) und der Abgleich gegen die echten Karten der drei Spiele: keine
// Originalkarte darf an einer harten Grenze "ueber" sein.
//
// Punkt 3 (Pruefungen): je Pruefung eine Karte mit dem Problem und eine ohne.
// Punkt 4 (Beheben): je Behebung die Wirkung (neu gepruefter Zaehler),
// Rueckgaengig und Wiederholen; "Alles Sichere beheben" in einem Schritt.
//
// Aufruf: karteoptimierer [<JA base> <JK2 base> <MD base;MD> <tests/karte/q3map2>]
//   (jeder Ordner "-" oder leer = uebersprungen)
#include "bhed/karte/grenzen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/patchkollision.h"
#include "bhed/pk3.h"
#include "bhed/i18n.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

// --- Bausteine fuer Testkarten (ohne Karte, direkt als Momentaufnahme) ------

AbbildEntitaet ent(const std::string& klasse, std::vector<std::pair<std::string, std::string>> mehr = {}) {
    AbbildEntitaet e;
    e.klasse = klasse;
    e.schluessel.emplace_back("classname", klasse);
    for (auto& kv : mehr) { e.schluessel.push_back(std::move(kv)); }
    return e;
}
AbbildPrimitiv quader(float x0, float y0, float z0, float x1, float y1, float z1, const std::string& shader = "textures/common/caulk") {
    AbbildPrimitiv p;
    p.mins = Vektor3f(x0, y0, z0);
    p.maxs = Vektor3f(x1, y1, z1);
    for (int i = 0; i < 6; ++i) {
        AbbildFlaeche f;
        f.shader = shader;
        p.flaechen.push_back(f);
    }
    return p;
}
KartenAbbild mitWelt() {
    KartenAbbild a;
    AbbildEntitaet w = ent("worldspawn");
    w.prims.push_back(quader(-64, -64, -64, 64, 64, 64));
    a.ents.push_back(w);
    return a;
}

ZaehlOptionen optionen(GrenzSpiel s) {
    ZaehlOptionen o;
    o.spiel = s;
    o.npcModell = [](const std::string& typ) { return "m_" + typ; };
    return o;
}

// Bewertung einer Grenze aus der Karte
Ampel ampelKarte(const KartenAbbild& a, GrenzSpiel s, const char* id, long long* wert = nullptr) {
    const std::vector<Bewertung> b = bewerten(s, zaehleKarte(a, optionen(s)), 0.9);
    for (const Bewertung& x : b) {
        if (x.zaehlung.id == id && x.zaehlung.quelle == ZaehlQuelle::Karte) {
            if (wert != nullptr) { *wert = x.zaehlung.wert; }
            return x.ampel;
        }
    }
    if (wert != nullptr) { *wert = -1; }
    return Ampel::Hinweis;
}

const char* name(Ampel a) { return ampelName(a); }

// Prueft "knapp unter" (nicht ueber) und "knapp ueber" (ueber) einer Grenze.
void grenzePaar(const char* was, GrenzSpiel s, const char* id, const KartenAbbild& unter, const KartenAbbild& ueber, long long erwartetUnter) {
    long long wu = 0;
    long long wo = 0;
    const Ampel au = ampelKarte(unter, s, id, &wu);
    const Ampel ao = ampelKarte(ueber, s, id, &wo);
    const Grenze* g = grenzeSuchen(s, id);
    const long long erlaubt = g != nullptr ? g->erlaubt : -1;
    pruefe(wu == erwartetUnter, std::string(was) + ": knapp unter zaehlt " + std::to_string(wu) + " (erwartet " + std::to_string(erwartetUnter) + ")");
    pruefe(wu <= erlaubt && (au == Ampel::Knapp || au == Ampel::Ok), std::string(was) + ": knapp unter (" + std::to_string(wu) + " / " +
                                                                         std::to_string(erlaubt) + ") ist " + name(au));
    pruefe(wo == erlaubt + 1 && ao == Ampel::Ueber, std::string(was) + ": knapp ueber (" + std::to_string(wo) + " / " + std::to_string(erlaubt) +
                                                       ") ist " + name(ao) + ", erwartet over");
}

// --- eine erzeugte RBSP ------------------------------------------------------

struct BspBau {
    std::vector<std::string> lumps = std::vector<std::string>(18);
    std::string bytes() const {
        std::string d = "RBSP";
        const int ver = 1;
        d.append(reinterpret_cast<const char*>(&ver), 4);
        std::size_t of = 8 + 18 * 8;
        std::string kopf;
        std::string inhalt;
        for (const std::string& l : lumps) {
            const auto o = static_cast<std::int32_t>(of + inhalt.size());
            const auto n = static_cast<std::int32_t>(l.size());
            kopf.append(reinterpret_cast<const char*>(&o), 4);
            kopf.append(reinterpret_cast<const char*>(&n), 4);
            inhalt += l;
            while (inhalt.size() % 4 != 0) { inhalt += '\0'; }
        }
        return d + kopf + inhalt;
    }
};
void i32an(std::string& s, std::int32_t v) { s.append(reinterpret_cast<const char*>(&v), 4); }
void f32an(std::string& s, float v) { s.append(reinterpret_cast<const char*>(&v), 4); }
std::string flaeche(int typ, int numVerts, int numIdx, int firstVert = 0, int w = 0, int h = 0, int lm = -1) {
    std::string s;
    i32an(s, 0);         // shaderNum
    i32an(s, -1);        // fogNum
    i32an(s, typ);
    i32an(s, firstVert);
    i32an(s, numVerts);
    i32an(s, 0);         // firstIndex
    i32an(s, numIdx);
    s.append(8, '\0');   // styles
    for (int k = 0; k < 4; ++k) { i32an(s, k == 0 ? lm : -1); }
    s.append(140 - s.size(), '\0');
    i32an(s, w);
    i32an(s, h);
    return s;
}
BspBau grundBsp() {
    BspBau b;
    b.lumps[0] = std::string("{\n\"classname\" \"worldspawn\"\n}\n") + '\0';
    std::string sh(72, '\0');
    std::memcpy(sh.data(), "textures/common/caulk", 21);
    b.lumps[1] = sh;
    std::string m;   // Modell 0: -64..64
    for (int k = 0; k < 3; ++k) { f32an(m, -64); }
    for (int k = 0; k < 3; ++k) { f32an(m, 64); }
    for (int k = 0; k < 4; ++k) { i32an(m, 0); }
    b.lumps[7] = m;
    return b;
}
long long bspWert(const BspAnalyse& a, const char* id) {
    for (const Zaehlung& z : a.werte) {
        if (z.id == id) { return z.wert; }
    }
    return -1;
}
Ampel bspAmpel(const BspAnalyse& a, GrenzSpiel s, const char* id) {
    for (const Bewertung& b : bewerten(s, a.werte, 0.9)) {
        if (b.zaehlung.id == id) { return b.ampel; }
    }
    return Ampel::Hinweis;
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// --- Punkt 2: Testkarten an jeder Grenze ----------------------------------------

void testeKartenGrenzen() {
    const GrenzSpiel sp = GrenzSpiel::JkaSp;
    // Entities nach dem Spawn: 1021 (MAX_GENTITIES 1024 - Welt - Leer - Spieler);
    // 300 Wegpunkte und 50 info_null geben sich frei und zaehlen nicht
    auto entKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < 300; ++i) { a.ents.push_back(ent("waypoint")); }
        for (int i = 0; i < 50; ++i) { a.ents.push_back(ent("info_null", {{"spawnflags", "1"}})); }
        for (int i = 0; i < 20; ++i) { a.ents.push_back(ent("light")); }                                  // ohne targetname: frei
        for (int i = 0; i < 5; ++i) { a.ents.push_back(ent("target_relay", {{"notsingle", "1"}})); }       // notsingle: frei
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("target_relay", {{"targetname", "t" + std::to_string(i)}})); }
        return a;
    };
    grenzePaar("Entities (JKA SP)", sp, "laufzeit.entitaeten", entKarte(1021), entKarte(1022), 1021);
    grenzePaar("Entities (MD)", GrenzSpiel::MovieDuels, "laufzeit.entitaeten", entKarte(4093), entKarte(4094), 4093);
    {   // info_null ohne spawnflags 1: OpenJK gibt es frei (g_misc.cpp:57), Movie Duels nicht (g_misc.cpp:52-56)
        KartenAbbild a = mitWelt();
        a.ents.push_back(ent("info_null"));
        long long sp1 = -1;
        long long md1 = -1;
        (void)ampelKarte(a, sp, "laufzeit.entitaeten", &sp1);
        (void)ampelKarte(a, GrenzSpiel::MovieDuels, "laufzeit.entitaeten", &md1);
        pruefe(sp1 == 0 && md1 == 1, "info_null ohne spawnflags: OpenJK 0, Movie Duels 1 (" + std::to_string(sp1) + "/" + std::to_string(md1) + ")");
    }

    // Schluessel je Entitaet: 64 Paare / 2048 Zeichen
    auto varKarte = [](int paare) {
        KartenAbbild a = mitWelt();
        AbbildEntitaet e = ent("target_relay");
        for (int i = 1; i < paare; ++i) { e.schluessel.emplace_back("k" + std::to_string(i), "v"); }
        a.ents.push_back(e);
        return a;
    };
    grenzePaar("Schluessel je Entity", sp, "spawn.variablen", varKarte(64), varKarte(65), 64);
    auto zeichenKarte = [](int zeichen) {
        KartenAbbild a = mitWelt();
        // "classname"+1 + "target_relay"+1 = 23; Rest in einem Wert
        AbbildEntitaet e = ent("target_relay");
        const int rest = zeichen - 23 - 2;   // Schluessel "x" + 1, Wert + 1
        e.schluessel.emplace_back("x", std::string(static_cast<std::size_t>(rest - 1), 'a'));
        a.ents.push_back(e);
        return a;
    };
    grenzePaar("Schluesseltext je Entity", sp, "spawn.zeichen", zeichenKarte(2048), zeichenKarte(2049), 2048);

    // Modelle als Configstrings: 255 (Index 1..255)
    auto modellKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("misc_model_breakable", {{"model", "models/map_objects/m" + std::to_string(i) + ".md3"}})); }
        a.ents.push_back(ent("misc_model_breakable", {{"model", "models/map_objects/m0.md3"}}));   // doppelt: einmal
        a.ents.push_back(ent("misc_model", {{"model", "models/map_objects/eingebacken.md3"}}));    // eingebacken: frei
        return a;
    };
    grenzePaar("Modelle (Configstrings)", sp, "laufzeit.modelle", modellKarte(255), modellKarte(256), 255);
    // Klaenge: JKA 379, JK2 255; Endung zaehlt nicht (G_SoundIndex: COM_StripExtension)
    auto klangKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("target_speaker", {{"noise", "sound/k" + std::to_string(i) + ".wav"}})); }
        a.ents.push_back(ent("target_speaker", {{"noise", "sound/k0"}}));   // = sound/k0.wav
        return a;
    };
    grenzePaar("Klaenge (JKA)", sp, "laufzeit.klaenge", klangKarte(379), klangKarte(380), 379);
    grenzePaar("Klaenge (JK2)", GrenzSpiel::Jk2, "laufzeit.klaenge", klangKarte(255), klangKarte(256), 255);
    // Effekte: 127
    auto fxKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("fx_runner", {{"fxFile", "env/e" + std::to_string(i)}})); }
        return a;
    };
    grenzePaar("Effekte", sp, "laufzeit.effekte", fxKarte(127), fxKarte(128), 127);
    // Welteffekte: 65 (JKA), 3 (JK2) - misc_weather_zone je eigener Kasten
    auto weltKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) {
            AbbildEntitaet e = ent("misc_weather_zone");
            e.prims.push_back(quader(static_cast<float>(i * 100), 0, 0, static_cast<float>(i * 100 + 64), 64, 64));
            a.ents.push_back(e);
        }
        return a;
    };
    grenzePaar("Welteffekte (JKA)", sp, "laufzeit.welteffekte", weltKarte(65), weltKarte(66), 65);
    grenzePaar("Welteffekte (JK2)", GrenzSpiel::Jk2, "laufzeit.welteffekte", weltKarte(3), weltKarte(4), 3);
    {   // fx_rain HEAVY: heavyrain + heavyrainfog + Blitz (6 Klaenge, 1 Effekt) - wie SP_CreateRain
        KartenAbbild a = mitWelt();
        a.ents.push_back(ent("fx_rain", {{"spawnflags", "4"}}));
        const std::vector<Zaehlung> z = zaehleKarte(a, optionen(sp));
        long long w = -1;
        long long k = -1;
        long long e = -1;
        for (const Zaehlung& x : z) {
            if (x.id == "laufzeit.welteffekte") { w = x.wert; }
            if (x.id == "laufzeit.klaenge") { k = x.wert; }
            if (x.id == "laufzeit.effekte") { e = x.wert; }
        }
        pruefe(w == 2 && k == 6 && e == 1, "fx_rain HEAVY: 2 Welteffekte, 6 Donner, 1 Blitz (" + std::to_string(w) + "/" + std::to_string(k) + "/" +
                                               std::to_string(e) + ")");
    }
    // ROFFs: 128
    auto roffKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("func_static", {{"roff", "roffs/r" + std::to_string(i) + ".rof"}})); }
        return a;
    };
    grenzePaar("ROFFs", sp, "laufzeit.roffs", roffKarte(128), roffKarte(129), 128);
    // NPC-Modelle je Level: 60 samt Spieler (JKA), 120 (MD)
    auto npcKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("NPC_spawner", {{"NPC_type", "typ" + std::to_string(i)}})); }
        a.ents.push_back(ent("NPC_spawner", {{"NPC_type", "typ0"}}));   // dasselbe Modell
        return a;
    };
    grenzePaar("NPC-Modelle (JKA)", sp, "laufzeit.npcmodelle", npcKarte(59), npcKarte(60), 60);
    grenzePaar("NPC-Modelle (MD)", GrenzSpiel::MovieDuels, "laufzeit.npcmodelle", npcKarte(119), npcKarte(120), 120);
    // Brush-Modelle: 511 samt Welt (BOX_MODEL_HANDLE), func_group zaehlt nicht
    auto bmodelKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < 30; ++i) {
            AbbildEntitaet g = ent("func_group");
            g.prims.push_back(quader(0, 0, 0, 8, 8, 8));
            a.ents.push_back(g);
        }
        for (int i = 0; i < n; ++i) {
            AbbildEntitaet e = ent("func_wall");
            e.prims.push_back(quader(0, 0, 0, 8, 8, 8));
            a.ents.push_back(e);
        }
        return a;
    };
    grenzePaar("Brush-Modelle (JKA SP)", sp, "kollision.submodelle", bmodelKarte(510), bmodelKarte(511), 511);
    grenzePaar("Brush-Modelle (MP)", GrenzSpiel::JkaMp, "kollision.submodelle", bmodelKarte(509), bmodelKarte(510), 510);
    // Weltgrenze: 65534
    auto weltgrenze = [](float x) {
        KartenAbbild a = mitWelt();
        a.ents.front().prims.push_back(quader(x - 16, 0, 0, x, 16, 16));
        return a;
    };
    grenzePaar("Weltgrenze", sp, "kollision.weltgrenze", weltgrenze(65534), weltgrenze(65535), 65534);
    // schaltbare Lichter: 32 (q3map2 MAX_SWITCHED_LIGHTS)
    auto lichtKarte = [](int n) {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < n; ++i) { a.ents.push_back(ent("light", {{"targetname", "l" + std::to_string(i)}})); }
        a.ents.push_back(ent("light", {{"targetname", "l0"}}));   // gleicher Stil
        return a;
    };
    grenzePaar("Lichtschalter", sp, "compiler.lichtschalter", lichtKarte(32), lichtKarte(33), 32);
    // Patch-Groesse: 31 (q3map2)
    auto patchKarte = [](int w) {
        KartenAbbild a = mitWelt();
        AbbildEntitaet e = ent("func_static");
        AbbildPrimitiv p;
        p.patch = true;
        p.breite = w;
        p.hoehe = 3;
        p.shader = "textures/x";
        for (int j = 0; j < 3; ++j) {
            for (int i = 0; i < w; ++i) { p.kontrollpunkte.push_back({static_cast<float>(i * 8), static_cast<float>(j * 8), 0}); }
        }
        e.prims.push_back(p);
        a.ents.push_back(e);
        return a;
    };
    grenzePaar("Patch-Groesse", sp, "renderer.patchgroesse", patchKarte(31), patchKarte(32), 31);
}

// Ein Patch mit Wellen: viele verschiedene Ebenen (Hoehe aus sin/cos).
std::vector<std::array<float, 3>> wellenPatch(int w, int h, float amp) {
    std::vector<std::array<float, 3>> p;
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            p.push_back({static_cast<float>(i * 64), static_cast<float>(j * 64),
                         amp * std::sin(static_cast<float>(i) * 1.3F) * std::cos(static_cast<float>(j) * 0.7F) + amp * 0.5F * std::sin(static_cast<float>(i * j) * 0.37F)});
        }
    }
    return p;
}

void testePatchKollision() {
    // Ein ebener 3x3-Patch: eine Flaechen-Ebene, eine Facette, Rand- und
    // Bevel-Ebenen (4 Raender + 4 achsparallele Bevels + Gegen-Ebene)
    std::vector<std::array<float, 3>> eben;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) { eben.push_back({static_cast<float>(i * 64), static_cast<float>(j * 64), 0}); }
    }
    const PatchKollision k = patchKollision(3, 3, eben);
    pruefe(k.gueltig, "3x3 eben: gueltig");
    pruefe(k.gitterBreite == 2 && k.gitterHoehe == 2, "3x3 eben: Gitter 2x2 nach dem Entfernen der Naeherungsspalten (" + std::to_string(k.gitterBreite) +
                                                          "x" + std::to_string(k.gitterHoehe) + ")");
    pruefe(k.facetten == 1, "3x3 eben: eine Facette (" + std::to_string(k.facetten) + ")");
    // Die Achsen-Bevels fallen mit den Raendern zusammen (CM_PlaneEqual), die
    // Bevels +-z mit der Flaeche selbst: 5 Ebenen, 4 Raender + Gegen-Ebene
    pruefe(k.ebenen == 5, "3x3 eben: Flaeche + 4 Raender = 5 Ebenen (" + std::to_string(k.ebenen) + ")");
    pruefe(k.meisteRaender == 5, "3x3 eben: 4 Raender + Gegen-Ebene (" + std::to_string(k.meisteRaender) + ")");
    pruefe(!patchKollision(4, 3, std::vector<std::array<float, 3>>(12)).gueltig, "gerade Breite: ungueltig (even sizes are invalid)");
    // Ein gebogener Patch (Halbzylinder) wird unterteilt (SUBDIVIDE_DISTANCE 16)
    std::vector<std::array<float, 3>> bogen;
    for (int j = 0; j < 3; ++j) {
        const float y = static_cast<float>(j * 128);
        bogen.push_back({-256, y, 0});
        bogen.push_back({-256, y, 256});
        bogen.push_back({0, y, 256});
        bogen.push_back({256, y, 256});
        bogen.push_back({256, y, 0});
    }
    const PatchKollision b = patchKollision(5, 3, bogen);
    // nach dem Transponieren ist die Bogenrichtung die Hoehe; die geraden Zeilen fallen auf 2
    pruefe(b.gueltig && b.gitterHoehe > 5 && b.gitterBreite == 2, "Bogen 5x3: Bogen unterteilt, Gerade auf 2 (" + std::to_string(b.gitterBreite) + "x" +
                                                                     std::to_string(b.gitterHoehe) + ")");
    // Wellen: an der Grenze 2048 (Vanilla) - die Ampel springt
    int letzte = -1;
    int unter = -1;
    int ueber = -1;
    for (int w = 3; w <= 31; w += 2) {
        const PatchKollision x = patchKollision(w, 31, wellenPatch(w, 31, 96));
        if (x.ebenen <= 2048) { unter = w; }
        if (x.ebenen > 2048 && ueber < 0) { ueber = w; }
        pruefe(x.ebenen >= letzte, "Wellen " + std::to_string(w) + "x31: Ebenen wachsen mit der Breite (" + std::to_string(x.ebenen) + ")");
        letzte = x.ebenen;
    }
    pruefe(unter > 0 && ueber > 0, "Wellen: ein Patch knapp unter und einer ueber 2048 Ebenen (Breite " + std::to_string(unter) + " / " + std::to_string(ueber) + ")");
    if (unter > 0 && ueber > 0) {
        auto karte = [](int w) {
            KartenAbbild a = mitWelt();
            AbbildEntitaet e = ent("func_static");
            AbbildPrimitiv p;
            p.patch = true;
            p.breite = w;
            p.hoehe = 31;
            p.shader = "textures/x";
            p.kontrollpunkte = wellenPatch(w, 31, 96);
            e.prims.push_back(p);
            a.ents.push_back(e);
            return a;
        };
        long long wu = 0;
        long long wo = 0;
        const Ampel au = ampelKarte(karte(unter), GrenzSpiel::JkaSp, "kollision.patchebenen", &wu);
        const Ampel ao = ampelKarte(karte(ueber), GrenzSpiel::JkaSp, "kollision.patchebenen", &wo);
        pruefe(au != Ampel::Ueber && ao == Ampel::Ueber, "Patch-Ebenen: " + std::to_string(wu) + " " + name(au) + ", " + std::to_string(wo) + " " + name(ao));
        // Movie Duels erlaubt 8192: derselbe Patch ist dort nicht ueber
        const Ampel md = ampelKarte(karte(ueber), GrenzSpiel::MovieDuels, "kollision.patchebenen");
        pruefe(md != Ampel::Ueber, std::string("Patch-Ebenen in Movie Duels (8192): ") + name(md));
    }
}

// --- Punkt 2: erzeugte RBSP -----------------------------------------------------

void testeBsp() {
    const GrenzSpiel sp = GrenzSpiel::JkaSp;
    const ZaehlOptionen o = optionen(sp);
    {   // Lichtgitter 65535 Punkte ok, 65536 ueber; Index 65535 hinter dem Feld
        BspBau b = grundBsp();
        b.lumps[15].assign(65535 * 30, '\0');
        b.lumps[17].assign(4, '\0');
        const std::uint16_t hinten = 65535;
        std::memcpy(b.lumps[17].data() + 2, &hinten, 2);
        const BspAnalyse a = analysiereBsp(b.bytes(), o);
        pruefe(a.ok, "RBSP gelesen: " + a.fehler);
        pruefe(bspWert(a, "bsp.lichtgitter") == 65535 && bspAmpel(a, sp, "bsp.lichtgitter") != Ampel::Ueber, "Lichtgitter 65535: nicht ueber");
        pruefe(a.lichtgitterUeberlauf == 1, "Lichtgitter: eine Zelle zeigt auf Index 65535 (" + std::to_string(a.lichtgitterUeberlauf) + ")");
        b.lumps[15].assign(65536 * 30, '\0');
        const BspAnalyse c = analysiereBsp(b.bytes(), o);
        pruefe(bspAmpel(c, sp, "bsp.lichtgitter") == Ampel::Ueber, "Lichtgitter 65536: ueber");
    }
    {   // Lichtgitter-Feld passt nicht zu den Grenzen (light grid array mismatch)
        BspBau b = grundBsp();
        b.lumps[15].assign(30, '\0');
        b.lumps[17].assign(2 * 10, '\0');   // -64..64 mit 64 64 128: 3 * 3 * 1 = 9 Zellen
        const BspAnalyse a = analysiereBsp(b.bytes(), o);
        pruefe(a.lichtgitterErwartet == 9 && a.lichtgitterFehlpassung, "Lichtgitter-Feld: 9 Zellen erwartet, 10 da (" + std::to_string(a.lichtgitterErwartet) + ")");
        b.lumps[17].assign(2 * 9, '\0');
        pruefe(!analysiereBsp(b.bytes(), o).lichtgitterFehlpassung, "Lichtgitter-Feld: 9 Zellen passen");
    }
    {   // Lightmaps 256 ok, 257 ueber
        BspBau b = grundBsp();
        b.lumps[14].assign(256 * 128 * 128 * 3, '\0');
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "renderer.lightmaps") != Ampel::Ueber, "256 Lightmaps: nicht ueber");
        b.lumps[14].assign(257 * 128 * 128 * 3, '\0');
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "renderer.lightmaps") == Ampel::Ueber, "257 Lightmaps: ueber");
        // ueber MAX_MAP_LIGHTING (8 MB), aber Formatgrenze: nur Hinweis
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "bsp.licht") == Ampel::Hinweis, "Lightmap-Bytes ueber 8 MB: Hinweis (weiche Grenze)");
    }
    {   // Nebel 30 ok, 31 ueber
        BspBau b = grundBsp();
        b.lumps[12].assign(30 * 72, '\0');
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "renderer.nebel") != Ampel::Ueber, "30 Nebel: nicht ueber");
        b.lumps[12].assign(31 * 72, '\0');
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "renderer.nebel") == Ampel::Ueber, "31 Nebel: ueber");
    }
    {   // Punkte je Flaeche 999 ok, 1000 ueber; Patches zaehlen dort nicht
        BspBau b = grundBsp();
        b.lumps[13] = flaeche(1, 999, 3) + flaeche(2, 1500, 0);
        b.lumps[10].assign(80 * 1500, '\0');
        const BspAnalyse a = analysiereBsp(b.bytes(), o);
        pruefe(bspWert(a, "renderer.flaechenpunkte") == 999 && bspAmpel(a, sp, "renderer.flaechenpunkte") != Ampel::Ueber, "Flaeche 999 Punkte: nicht ueber");
        b.lumps[13] = flaeche(3, 1000, 6000);
        const BspAnalyse c = analysiereBsp(b.bytes(), o);
        pruefe(bspAmpel(c, sp, "renderer.flaechenpunkte") == Ampel::Ueber && bspAmpel(c, sp, "renderer.flaechenindizes") == Ampel::Ueber,
               "Flaeche 1000 Punkte / 6000 Indizes: ueber");
    }
    {   // Modelle 511 ok, 512 ueber
        BspBau b = grundBsp();
        const std::string m0 = b.lumps[7];
        std::string m;
        for (int i = 0; i < 511; ++i) { m += m0; }
        b.lumps[7] = m;
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "kollision.submodelle") != Ampel::Ueber, "511 Modelle: nicht ueber");
        b.lumps[7] = m + m0;
        pruefe(bspAmpel(analysiereBsp(b.bytes(), o), sp, "kollision.submodelle") == Ampel::Ueber, "512 Modelle: ueber");
    }
    {   // Entity-Lump: Schluessel je Entity wie G_ParseSpawnVars (auch doppelte)
        BspBau b = grundBsp();
        std::string t = "{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"target_relay\"\n";
        for (int i = 0; i < 64; ++i) { t += "\"k\" \"v\"\n"; }
        t += "}\n";
        b.lumps[0] = t + '\0';
        const BspAnalyse a = analysiereBsp(b.bytes(), o);
        pruefe(bspWert(a, "spawn.variablen") == 65 && bspAmpel(a, sp, "spawn.variablen") == Ampel::Ueber, "Entity-Lump: 65 Paare (64 doppelte k) ueber");
    }
}

// --- Abgleich gegen die echten Karten der Spiele --------------------------------------

struct Abgleich {
    int karten = 0;
    int ueber = 0;
    std::map<std::string, std::pair<long long, std::string>> hoechstes;   // id -> (wert, karte)
};

void bspPruefen(const std::string& daten, const std::string& name, GrenzSpiel s, Abgleich& ab) {
    const BspAnalyse a = analysiereBsp(daten, optionen(s));
    if (!a.ok) { return; }
    ++ab.karten;
    for (const Bewertung& b : bewerten(s, a.werte, 0.9)) {
        auto& h = ab.hoechstes[b.zaehlung.id];
        if (b.zaehlung.wert > h.first) { h = {b.zaehlung.wert, name}; }
        if (b.ampel == Ampel::Ueber) {
            ++ab.ueber;
            std::printf("UEBER %s: %s = %lld (erlaubt %lld)\n", name.c_str(), b.zaehlung.id.c_str(), b.zaehlung.wert, b.grenze->erlaubt);
        }
    }
}

void spielAbgleich(const std::vector<std::string>& ordner, GrenzSpiel s, const char* spiel) {
    Abgleich ab;
    const auto t0 = std::chrono::steady_clock::now();
    std::map<std::string, bool> gesehen;
    for (const std::string& o : ordner) {
        if (o.empty() || o == "-") { continue; }
        bhed::GamePath gp;
        if (!bhed::scanGamePath(o, gp)) { continue; }
        for (const bhed::Pk3& p : gp.archives) {
            for (const bhed::Pk3Entry& e : p.entries) {
                std::string n = e.name;
                for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                if (n.rfind("maps/", 0) != 0 || n.size() < 4 || n.compare(n.size() - 4, 4, ".bsp") != 0) { continue; }
                if (gesehen[n]) { continue; }
                gesehen[n] = true;
                std::string daten;
                if (!bhed::readPk3File(p, e, daten)) { continue; }
                bspPruefen(daten, std::filesystem::path(p.path).filename().string() + ":" + e.name, s, ab);
            }
        }
    }
    if (ab.karten == 0) {
        std::printf("Abgleich %s: uebersprungen (keine Karten)\n", spiel);
        return;
    }
    const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("Abgleich %s: %d Karten, %d harte Grenzen ueber, %.1f s\n", spiel, ab.karten, ab.ueber, sek);
    for (const auto& [id, h] : ab.hoechstes) {
        const Grenze* g = grenzeSuchen(s, id);
        std::printf("  %-26s %10lld / %-10lld %5.1f %%  %s\n", id.c_str(), h.first, g != nullptr ? g->erlaubt : 0LL,
                    g != nullptr && g->erlaubt > 0 ? 100.0 * static_cast<double>(h.first) / static_cast<double>(g->erlaubt) : 0.0, h.second.c_str());
    }
    pruefe(ab.ueber == 0, std::string("Abgleich ") + spiel + ": keine Originalkarte ueber einer harten Grenze");
}


// --- Punkt 3: Pruefungen -------------------------------------------------------

std::string kastenText(double x0, double y0, double z0, double x1, double y1, double z1, const char* tex, int inhalt = 0) {
    auto f = [](double v) {
        char z[32];
        std::snprintf(z, sizeof(z), "%g", v);
        return std::string(z);
    };
    const std::string t = std::string(" ") + tex + " 0 0 0 0.5 0.5 " + std::to_string(inhalt) + " 0 0\n";
    auto p = [&](double a, double b, double c) { return "( " + f(a) + " " + f(b) + " " + f(c) + " ) "; };
    return "{\n" + p(x1, y1, z1) + p(x1, y0, z1) + p(x0, y1, z1) + t + p(x1, y1, z1) + p(x0, y1, z1) + p(x1, y1, z0) + t + p(x1, y1, z1) + p(x1, y1, z0) +
           p(x1, y0, z1) + t + p(x0, y0, z0) + p(x1, y0, z0) + p(x0, y1, z0) + t + p(x0, y0, z0) + p(x0, y0, z1) + p(x1, y0, z0) + t + p(x0, y0, z0) +
           p(x0, y1, z0) + p(x0, y0, z1) + t + "}\n";
}
std::string entText(const std::string& klasse, const std::string& schluessel, const std::string& pinsel) {
    return "{\n\"classname\" \"" + klasse + "\"\n" + schluessel + pinsel + "}\n";
}
KartenAbbild abbildAus(const std::string& text) {
    Karte k;
    (void)karteLesen(text, k);
    k.anmelden();
    return kartenAbbild(k);
}
const Befund* befund(const std::vector<Befund>& b, const char* id) {
    for (const Befund& x : b) {
        if (x.pruefung == id) { return &x; }
    }
    return nullptr;
}
long long anzahlVon(const std::vector<Befund>& b, const char* id) {
    const Befund* x = befund(b, id);
    return x != nullptr ? x->anzahl : 0;
}
PruefOptionen pruefOpt() {
    PruefOptionen o;
    o.spiel = GrenzSpiel::JkaSp;
    return o;
}
constexpr int kDetail = 1 << 27;

void testePruefungen(const std::string& q3dir) {
    const PruefOptionen o = pruefOpt();
    {   // Ebenen zeigen nach aussen: der Mittelpunkt liegt hinter jeder Flaeche
        const KartenAbbild a = abbildAus(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x")));
        bool innen = !a.ents.front().prims.empty();
        for (const AbbildFlaeche& f : a.ents.front().prims.front().flaechen) { innen = innen && abstandZuPunkt(f.ebene, Vektor3d(32, 32, 32)) < 0; }
        pruefe(innen, "Ebenen der Pinsel zeigen nach aussen (Mittelpunkt innen)");
    }
    // 1. Leck und Detail an der Leck-Linie
    {
        PruefOptionen p = pruefOpt();
        const std::string welt = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(100, 0, 0, 164, 64, 64, "test/x", kDetail));
        pruefe(befund(pruefeKarte(abbildAus(welt), p), "leck") == nullptr, "ohne .lin: kein Leck");
        p.linDaten = "0 -50 32\n132.000000 32.000000 32.000000\n132 200 32\n";
        const std::vector<Befund> b = pruefeKarte(abbildAus(welt), p);
        pruefe(befund(b, "leck") != nullptr && befund(b, "leck")->anzahl == 3, "Leck: 3 Punkte der .lin");
        pruefe(anzahlVon(b, "detailamleck") == 1, "Leck-Linie durch den Detail-Pinsel: 1 (" + std::to_string(anzahlVon(b, "detailamleck")) + ")");
    }
    // 2. verdeckte Flaechen
    {
        const std::string zwei = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 64, 64, 64, 128, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(zwei), o), "verdeckt") == 2, "zwei gleich grosse Kaesten aufeinander: Deckel und Boden verdeckt (2)");
        const std::string kleiner = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(16, 16, 64, 48, 48, 128, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(kleiner), o), "verdeckt") == 1, "kleiner Kasten auf grossem: nur sein Boden verdeckt (1)");
        const std::string luecke = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 65, 64, 64, 128, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(luecke), o), "verdeckt") == 0, "Abstand 1: nichts verdeckt");
        const std::string glas = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 64, 64, 64, 128, "test/glass"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(glas), o), "verdeckt") == 1, "Glas deckt nicht: nur der Glasboden ist verdeckt (1)");
        const std::string caulk = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "common/caulk") + kastenText(0, 0, 64, 64, 64, 128, "common/caulk"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(caulk), o), "verdeckt") == 0, "schon gecaulkt: nichts");
        const std::string raum = lies(std::filesystem::path(q3dir) / "raum.map");
        if (!raum.empty()) {
            const long long n = anzahlVon(pruefeKarte(abbildAus(raum), o), "verdeckt");
            pruefe(n > 0, "raum.map: Waende auf dem Boden - verdeckte Flaechen gefunden (" + std::to_string(n) + ")");
        }
    }
    // 3. entartet, Mikro, Off-Grid, Mischinhalt, Areaportal in Entity
    {
        KartenAbbild a = mitWelt();
        AbbildPrimitiv kaputt = quader(0, 0, 0, 8, 8, 8);
        kaputt.entartet = true;
        a.ents.front().prims.push_back(kaputt);
        pruefe(anzahlVon(pruefeKarte(a, o), "entartet") == 1, "entarteter Pinsel gefunden");
        const std::string mikro = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 0.5, "test/x"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(mikro), o), "mikro") == 1, "Pinsel 0.5 dick: Mikropinsel");
        const std::string raster = entText("worldspawn", "", kastenText(0, 0, 0, 64.02, 64, 64, "test/x"));
        const std::vector<Befund> br = pruefeKarte(abbildAus(raster), o);
        pruefe(anzahlVon(br, "offgrid") == 1 && befund(br, "offgrid")->sicher, "Ecke bei 64.02: neben dem Raster, sicher einzurasten");
        const std::string halb = entText("worldspawn", "", kastenText(0, 0, 0, 64.5, 64, 64, "test/x"));
        const std::vector<Befund> bh = pruefeKarte(abbildAus(halb), o);
        pruefe(anzahlVon(bh, "offgrid") == 1 && !befund(bh, "offgrid")->sicher, "Ecke bei 64.5: neben dem Raster, nicht 'sicher'");
        pruefe(anzahlVon(pruefeKarte(abbildAus(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x"))), o), "offgrid") == 0, "ganze Zahlen: kein Befund");
        const std::string misch = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "common/hint", kDetail));
        pruefe(anzahlVon(pruefeKarte(abbildAus(misch), o), "mischinhalt") == 1, "Hint als detail: Mischinhalt (q3map2 map.cpp:509)");
        const std::string portal = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x")) +
                                   entText("func_wall", "", kastenText(0, 0, 100, 64, 64, 164, "common/areaportal"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(portal), o), "areaportalentity") == 1, "Areaportal in func_wall (q3map2 map.cpp:810)");
    }
    // 4. Deko als structural
    {
        const std::string deko = entText("worldspawn", "", kastenText(0, 0, 0, 512, 512, 16, "test/boden") + kastenText(0, 0, 16, 32, 32, 48, "test/kiste"));
        const std::vector<Befund> b = pruefeKarte(abbildAus(deko), o);
        pruefe(anzahlVon(b, "deko") == 1 && befund(b, "deko")->schaetzung, "kleine Kiste structural: Vorschlag detail (1, Schaetzung)");
        const std::string detail = entText("worldspawn", "", kastenText(0, 0, 0, 512, 512, 16, "test/boden") + kastenText(0, 0, 16, 32, 32, 48, "test/kiste", kDetail));
        pruefe(anzahlVon(pruefeKarte(abbildAus(detail), o), "deko") == 0, "kleine Kiste als detail: kein Vorschlag");
    }
    // 5. Blockgitter und Ueberlappung
    {
        const std::string block = entText("worldspawn", "", kastenText(1000, 0, 0, 1040, 512, 256, "test/wand"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(block), o), "blockgitter") == 1, "Wand 1000..1040 ragt 16 ueber die Blockgrenze 1024");
        PruefOptionen o2 = pruefOpt();
        o2.blockGroesse = 0;
        pruefe(anzahlVon(pruefeKarte(abbildAus(block), o2), "blockgitter") == 0, "_blocksize 0: kein Blockgitter");
        const std::string ueber = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(32, 0, 0, 96, 64, 64, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(ueber), o), "ueberlappung") == 1, "Kaesten 0..64 und 32..96: Ueberlappung");
        const std::string buendig = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(64, 0, 0, 128, 64, 64, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(buendig), o), "ueberlappung") == 0, "Kaesten buendig: keine Ueberlappung");
    }
    // 6. T-Verbindungen
    {
        const std::string t = entText("worldspawn", "", kastenText(0, 0, 0, 128, 128, 64, "test/x") + kastenText(0, 0, 64, 64, 128, 128, "test/y"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(t), o), "tjunction") >= 1, "kleiner Kasten auf grossem: T-Verbindung an x=64");
        pruefe(anzahlVon(pruefeKarte(abbildAus(entText("worldspawn", "", kastenText(0, 0, 0, 128, 128, 64, "test/x"))), o), "tjunction") == 0, "ein Kasten: keine T-Verbindung");
    }
    // 7. Patches und Clip-Modelle
    {
        KartenAbbild a = mitWelt();
        AbbildEntitaet e = ent("func_static");
        AbbildPrimitiv p;
        p.patch = true;
        p.breite = 31;
        p.hoehe = 31;
        p.shader = "textures/x";
        p.kontrollpunkte = wellenPatch(31, 31, 96);
        e.prims.push_back(p);
        a.ents.push_back(e);
        const std::vector<Befund> b = pruefeKarte(a, o);
        pruefe(befund(b, "patchgross") != nullptr && befund(b, "patchgross")->schwere == Schwere::Fehler && befund(b, "patchgross")->behebung == Behebung::PatchTeilen,
               "Wellen-Patch 31x31: zu viele Kollisionsebenen (Fehler, Patch teilen)");
        KartenAbbild c = mitWelt();
        AbbildEntitaet e2 = ent("func_static");
        AbbildPrimitiv spitz;
        spitz.patch = true;
        spitz.breite = 3;
        spitz.hoehe = 3;
        for (int j = 0; j < 3; ++j) {
            for (int i = 0; i < 3; ++i) { spitz.kontrollpunkte.push_back({static_cast<float>(i * 64), static_cast<float>(j * 64), i == 1 ? 1000000.0F : 0.0F}); }
        }
        e2.prims.push_back(spitz);
        c.ents.push_back(e2);
        pruefe(anzahlVon(pruefeKarte(c, o), "patchgitter") == 1, "Patch mit Spitze 1e6: mehr als 129 Gitterspalten (CM_SubdivideGridColumns)");
        KartenAbbild m = mitWelt();
        m.ents.push_back(ent("misc_model", {{"model", "models/a.md3"}, {"spawnflags", "2"}}));
        m.ents.push_back(ent("misc_model", {{"model", "models/b.md3"}}));
        pruefe(anzahlVon(pruefeKarte(m, o), "clipmodell") == 1, "misc_model spawnflags 2: Clip-Modell");
    }
    // 8. fehlende Texturen
    {
        PruefOptionen p = pruefOpt();
        p.shaderBekanntGeprueft = true;
        ShaderArt fehlt;
        fehlt.bekannt = false;
        p.shader["textures/test/fehlt"] = fehlt;
        const std::string t = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/fehlt") + kastenText(100, 0, 0, 164, 64, 64, "test/da"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(t), p), "textur") == 1, "eine fehlende Textur (die bekannte zaehlt nicht)");
    }
    // 9. Brush-Entity ohne Pinsel
    {
        KartenAbbild a = mitWelt();
        AbbildEntitaet tuer = ent("func_door");
        tuer.behaelter = true;
        a.ents.push_back(tuer);
        AbbildEntitaet tuer2 = ent("func_door");
        tuer2.behaelter = true;
        tuer2.prims.push_back(quader(0, 0, 0, 8, 64, 64, "textures/tuer"));
        a.ents.push_back(tuer2);
        AbbildEntitaet nurUrsprung = ent("func_rotating");
        nurUrsprung.behaelter = true;
        nurUrsprung.prims.push_back(quader(0, 0, 0, 8, 8, 8, "textures/common/origin"));
        a.ents.push_back(nurUrsprung);
        pruefe(anzahlVon(pruefeKarte(a, o), "leereentity") == 2, "func_door ohne Pinsel und func_rotating nur mit Ursprung: SV_SetBrushModel NULL (2)");
    }
    // 10. aus der BSP: Lichtgitter, Lightmaps, VIS, Dreiecke, Spawnpunkt
    {
        BspAnalyse an;
        an.ok = true;
        an.lichtgitterUeberlauf = 21752;
        an.gitterGroesse = {64, 64, 128};
        an.portale = 9000;
        an.cluster = 1200;
        an.dreieckeJeAnsicht = 25000;
        PruefOptionen p = pruefOpt();
        p.bsp = &an;
        Bewertung lm;
        lm.zaehlung.id = "renderer.lightmaps";
        lm.zaehlung.quelle = ZaehlQuelle::Bsp;
        lm.zaehlung.wert = 242;
        lm.anteil = 242.0 / 256.0;
        lm.ampel = Ampel::Knapp;
        p.bewertungen.push_back(lm);
        const std::vector<Befund> b = pruefeKarte(mitWelt(), p);
        const Befund* lg = befund(b, "lichtgitter");
        pruefe(lg != nullptr && lg->schluessel == "gridsize" && lg->wert == "128 128 128" && lg->anzahl == 21752, "volles Lichtgitter: gridsize 128 128 128 vorgeschlagen");
        const Befund* ls = befund(b, "lightmapscale");
        pruefe(ls != nullptr && ls->schluessel == "_lightmapscale" && ls->wert == "2", "Lightmaps knapp: _lightmapscale 2 vorgeschlagen");
        pruefe(befund(b, "vis") != nullptr && befund(b, "dreiecke") != nullptr, "9000 Portale und 25000 Dreiecke je Ansicht: Hinweise");
        an.lichtgitterUeberlauf = 0;
        an.portale = 100;
        an.dreieckeJeAnsicht = 1000;
        p.bewertungen.clear();
        const std::vector<Befund> c = pruefeKarte(mitWelt(), p);
        pruefe(befund(c, "lichtgitter") == nullptr && befund(c, "lightmapscale") == nullptr && befund(c, "vis") == nullptr && befund(c, "dreiecke") == nullptr,
               "ohne Ueberlauf und kleine Werte: keine Befunde");
        const std::string bsp = lies(std::filesystem::path(q3dir) / "raum.nrc.bsp");
        const std::string raum = lies(std::filesystem::path(q3dir) / "raum.map");
        if (!bsp.empty() && !raum.empty()) {
            pruefe(bspCluster(bsp, Vektor3f(-384, 128, 32)) >= 0, "raum.nrc.bsp: der Spawnpunkt liegt in einem Cluster");
            pruefe(bspCluster(bsp, Vektor3f(2000, 0, 100)) == -1, "raum.nrc.bsp: ausserhalb ist Cluster -1");
            p.bspDaten = &bsp;
            pruefe(befund(pruefeKarte(abbildAus(raum), p), "spawnimfesten") == nullptr, "raum.map: Spawnpunkt im Raum - kein Befund");
            std::string draussen = raum;
            const std::size_t i = draussen.find("\"-384 128 24\"");
            if (i != std::string::npos) { draussen.replace(i, 13, "\"2000 0 24\""); }
            pruefe(befund(pruefeKarte(abbildAus(draussen), p), "spawnimfesten") != nullptr, "Spawnpunkt nach 2000 0 24 verschoben: im Festen");
        }
    }
    // 11. Brush-Modelle zusammenlegen
    {
        KartenAbbild a = mitWelt();
        for (int i = 0; i < 3; ++i) {
            AbbildEntitaet w = ent("func_wall", {{"spawnflags", "1"}});
            w.prims.push_back(quader(static_cast<float>(i * 100), 0, 0, static_cast<float>(i * 100 + 8), 8, 8));
            a.ents.push_back(w);
        }
        AbbildEntitaet t = ent("func_wall", {{"spawnflags", "1"}, {"targetname", "an"}});
        t.prims.push_back(quader(500, 0, 0, 508, 8, 8));
        a.ents.push_back(t);
        PruefOptionen p = pruefOpt();
        Bewertung bm;
        bm.zaehlung.id = "kollision.submodelle";
        bm.zaehlung.wert = 480;
        bm.ampel = Ampel::Knapp;
        p.bewertungen.push_back(bm);
        pruefe(anzahlVon(pruefeKarte(a, p), "modellezusammen") == 3, "drei gleiche func_wall ohne targetname zusammenzulegen (3)");
        pruefe(anzahlVon(pruefeKarte(a, o), "modellezusammen") == 0, "ohne knappe Brush-Modelle: kein Vorschlag");
    }
}

// --- Punkt 4: Beheben (mit Wirkung, Rueckgaengig und Wiederholen) ---------------

std::string patchText(int w, int h, const std::vector<std::array<float, 3>>& p) {
    std::string t = "{\npatchDef2\n{\ntextures/test/x\n( " + std::to_string(w) + " " + std::to_string(h) + " 0 0 0 )\n(\n";
    for (int i = 0; i < w; ++i) {   // Spalten (patchDef2: je Zeile der Datei eine Spalte)
        t += "( ";
        for (int j = 0; j < h; ++j) {
            const auto& q = p[static_cast<std::size_t>(j) * static_cast<std::size_t>(w) + static_cast<std::size_t>(i)];
            char z[128];
            std::snprintf(z, sizeof(z), "( %g %g %g 0 0 ) ", static_cast<double>(q[0]), static_cast<double>(q[1]), static_cast<double>(q[2]));
            t += z;
        }
        t += ")\n";
    }
    return t + ")\n}\n}\n";
}

struct Fall {
    Karte k;
    KartenAbbild a;
    std::vector<Befund> b;
    explicit Fall(const std::string& text, const PruefOptionen& o = pruefOpt()) {
        (void)karteLesen(text, k);
        k.anmelden();
        neu(o);
    }
    void neu(const PruefOptionen& o = pruefOpt()) {
        a = kartenAbbild(k);
        b = pruefeKarte(a, o);
    }
    long long n(const char* id) const { return anzahlVon(b, id); }
};

// Behebt den Befund id, prueft die Wirkung (neu gepruefter Zaehler), dann
// Rueckgaengig (wieder wie vorher) und Wiederholen (wieder behoben).
void behebenPruefen(const char* was, Fall& f, const char* id, long long nachher, const PruefOptionen& o = pruefOpt()) {
    const long long vorher = f.n(id);
    const Befund* b = befund(f.b, id);
    pruefe(b != nullptr && vorher > 0, std::string(was) + ": Befund " + id + " vorhanden (" + std::to_string(vorher) + ")");
    if (b == nullptr) { return; }
    pruefe(!behebungsVorschau(*b).empty(), std::string(was) + ": Vorschau \"" + behebungsVorschau(*b) + "\"");
    const BehebungsErgebnis e = beheben(f.k, f.a, *b);
    pruefe(e.ok && e.geaendert > 0 && e.undoName.rfind("Optimizer: ", 0) == 0,
           std::string(was) + ": behoben (" + std::to_string(e.geaendert) + ", \"" + e.undoName + "\") " + e.meldung);
    pruefe(f.k.undo.undoName() == e.undoName, std::string(was) + ": ein Rueckgaengig-Schritt \"" + f.k.undo.undoName() + "\"");
    f.neu(o);
    pruefe(f.n(id) == nachher, std::string(was) + ": danach " + std::to_string(f.n(id)) + " (erwartet " + std::to_string(nachher) + ")");
    (void)f.k.undo.undo();
    f.neu(o);
    pruefe(f.n(id) == vorher, std::string(was) + ": Rueckgaengig stellt " + std::to_string(vorher) + " wieder her (" + std::to_string(f.n(id)) + ")");
    (void)f.k.undo.redo();
    f.neu(o);
    pruefe(f.n(id) == nachher, std::string(was) + ": Wiederholen behebt wieder (" + std::to_string(f.n(id)) + ")");
}

void testeBeheben() {
    {   // Caulk fuer verdeckte Flaechen
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 64, 64, 64, 128, "test/y")));
        behebenPruefen("Caulk", f, "verdeckt", 0);
        long long caulk = 0;
        for (const auto& p : f.k.entitaeten().front()->kinder()) {
            const auto& pin = static_cast<const Pinsel&>(*p);
            for (std::size_t i = 0; i < pin.anzahl(); ++i) { caulk += pin[i].shader() == "textures/system/caulk" ? 1 : 0; }
        }
        pruefe(caulk == 2, "Caulk: genau die zwei verdeckten Flaechen tragen system/caulk (der Caulk von JKA, JK2, Movie Duels) (" + std::to_string(caulk) + ")");
    }
    {   // Make Detail fuer Deko
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 512, 512, 16, "test/boden") + kastenText(0, 0, 16, 32, 32, 48, "test/kiste")));
        behebenPruefen("Detail", f, "deko", 0);
        const auto& kinder = f.k.entitaeten().front()->kinder();
        pruefe(kinder.size() == 2 && !static_cast<const Pinsel&>(*kinder[0]).istDetail() && static_cast<const Pinsel&>(*kinder[1]).istDetail(),
               "Detail: nur die Kiste ist detail, der Boden bleibt structural");
    }
    {   // Make Structural fuer Hint als detail
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "common/hint", kDetail)));
        behebenPruefen("Struktur", f, "mischinhalt", 0);
    }
    {   // Snap to Grid
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64.02, 64, 64, "test/x")));
        behebenPruefen("Einrasten", f, "offgrid", 0);
        const Kasten& g = static_cast<const Pinsel&>(*f.k.entitaeten().front()->kinder().front()).grenzen(f.k.kontext);
        pruefe(std::fabs(g.origin[0] + g.extents[0] - 64.0F) < 0.001F, "Einrasten: die Ecke liegt danach bei 64");
    }
    {   // Snap to Grid mit einem 0.5 dicken Pinsel daneben (Selbsttest Stufe 5/7:
        // danach blieb ein Befund "offgrid")
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64.02, 64, 64, "test/x") + kastenText(100, 0, 0, 164, 64, 0.5, "test/x")));
        pruefe(f.n("offgrid") == 1 && f.n("mikro") == 1, "0.5 dick: Mikropinsel, nicht \"offgrid\" (Einrasten kann ihn nur zerdruecken)");
        behebenPruefen("Einrasten mit Mikropinsel daneben", f, "offgrid", 0);
    }
    {   // Mikropinsel entfernen
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(100, 0, 0, 164, 64, 0.5, "test/x")));
        behebenPruefen("Mikropinsel", f, "mikro", 0);
        pruefe(f.k.entitaeten().front()->kinder().size() == 1, "Mikropinsel: einer von zwei Pinseln bleibt");
    }
    {   // Patch teilen (bobToolz Split patch)
        Fall f(entText("worldspawn", "", kastenText(-64, -64, -64, 64, 64, 0, "test/x")) + entText("func_static", "", patchText(31, 31, wellenPatch(31, 31, 96))));
        behebenPruefen("Patch teilen", f, "patchgross", 0);
        pruefe(f.k.entitaeten().back()->kinder().size() == 225, "Patch teilen: 31x31 wird zu 15 x 15 = 225 Teilen (" +
                                                                       std::to_string(f.k.entitaeten().back()->kinder().size()) + ")");
    }
    {   // Brush-Modelle zusammenlegen; danach wird neu gezaehlt
        std::string t = entText("worldspawn", "", kastenText(-64, -64, -64, 64, 64, 0, "test/x"));
        for (int i = 0; i < 3; ++i) { t += entText("func_wall", "\"spawnflags\" \"1\"\n", kastenText(i * 100, 0, 0, i * 100 + 8, 8, 8, "test/w")); }
        t += entText("func_wall", "\"spawnflags\" \"1\"\n\"targetname\" \"an\"\n", kastenText(500, 0, 0, 508, 8, 8, "test/w"));
        PruefOptionen o = pruefOpt();
        Bewertung bm;
        bm.zaehlung.id = "kollision.submodelle";
        bm.zaehlung.wert = 480;
        bm.ampel = Ampel::Knapp;
        o.bewertungen.push_back(bm);
        Fall f(t, o);
        auto modelle = [&]() {
            for (const Zaehlung& z : zaehleKarte(kartenAbbild(f.k), optionen(GrenzSpiel::JkaSp))) {
                if (z.id == "kollision.submodelle") { return z.wert; }
            }
            return -1LL;
        };
        const long long vorher = modelle();
        behebenPruefen("Modelle zusammenlegen", f, "modellezusammen", 0, o);
        pruefe(vorher == 5 && modelle() == 3, "Modelle zusammenlegen: Brush-Modelle 5 -> 3 (" + std::to_string(vorher) + " -> " + std::to_string(modelle()) + ")");
        long long pinsel = 0;
        for (const auto& e : f.k.entitaeten()) { pinsel += static_cast<long long>(e->kinder().size()); }
        pruefe(pinsel == 5 && f.k.entitaeten()[1]->kinder().size() == 3, "Modelle zusammenlegen: kein Pinsel geht verloren, der erste func_wall traegt alle drei");
    }
    {   // worldspawn gridsize (volles Lichtgitter)
        BspAnalyse an;
        an.ok = true;
        an.lichtgitterUeberlauf = 100;
        PruefOptionen o = pruefOpt();
        o.bsp = &an;
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x")), o);
        const Befund* b = befund(f.b, "lichtgitter");
        pruefe(b != nullptr, "gridsize: Befund vorhanden");
        if (b != nullptr) {
            const BehebungsErgebnis e = beheben(f.k, f.a, *b);
            pruefe(e.ok && f.k.welt()->wert("gridsize") == "128 128 128", "gridsize: worldspawn \"gridsize\" \"128 128 128\" (" + f.k.welt()->wert("gridsize") + ")");
            (void)f.k.undo.undo();
            pruefe(f.k.welt()->wert("gridsize").empty(), "gridsize: Rueckgaengig entfernt den Schluessel");
        }
    }
    {   // Brush-Entity ohne Pinsel entfernen
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x")) + entText("func_door", "\"angle\" \"90\"\n", ""));
        f.a.ents.back().behaelter = true;   // ohne Entitaets-Definitionen kennt die Karte func_door nicht als Brush-Entity
        f.b = pruefeKarte(f.a, pruefOpt());
        const Befund* b = befund(f.b, "leereentity");
        pruefe(b != nullptr, "leere Entity: Befund vorhanden");
        if (b != nullptr) {
            const BehebungsErgebnis e = beheben(f.k, f.a, *b);
            pruefe(e.ok && f.k.entitaeten().size() == 1, "leere Entity: geloescht (" + std::to_string(f.k.entitaeten().size()) + " Entitaeten)");
            (void)f.k.undo.undo();
            pruefe(f.k.entitaeten().size() == 2, "leere Entity: Rueckgaengig bringt sie zurueck");
        }
    }
    {   // Alles Sichere beheben: Caulk, Struktur, Einrasten - nicht die Deko
        const std::string t = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 64, 64, 64, 128, "test/y") +
                                                            kastenText(200, 0, 0, 264, 64, 64, "common/hint", kDetail) +
                                                            kastenText(300, 0, 0, 428.02, 128, 128, "test/z") + kastenText(500, 0, 0, 532, 32, 32, "test/kiste"));
        Fall f(t);
        const long long deko = f.n("deko");
        const auto r = allesSichereBeheben(f.k, f.a, f.b);
        std::set<std::string> welche;
        for (const auto& [id, e] : r) {
            if (e.geaendert > 0) { welche.insert(id); }
        }
        pruefe(welche == std::set<std::string>{"verdeckt", "mischinhalt", "offgrid"}, "Alles Sichere: verdeckt, mischinhalt, offgrid (" + std::to_string(welche.size()) + ")");
        pruefe(f.k.undo.undoName().rfind("Optimizer: fix all safe", 0) == 0, "Alles Sichere: ein Rueckgaengig-Schritt \"" + f.k.undo.undoName() + "\"");
        f.neu();
        pruefe(f.n("verdeckt") == 0 && f.n("mischinhalt") == 0 && f.n("offgrid") == 0 && f.n("deko") == deko && deko > 0,
               "Alles Sichere: die drei behoben, die Deko-Vorschlaege bleiben");
        (void)f.k.undo.undo();
        f.neu();
        pruefe(f.n("verdeckt") == 2 && f.n("mischinhalt") == 1 && f.n("offgrid") == 1, "Alles Sichere: ein Rueckgaengig stellt alles wieder her");
    }
    {   // veraltete Momentaufnahme: nicht beheben
        Fall f(entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x") + kastenText(0, 0, 64, 64, 64, 128, "test/y")));
        {
            Befehl b(f.k.undo, "andere Aenderung");
            f.k.welt()->setzen("message", "x");
        }
        const BehebungsErgebnis e = beheben(f.k, f.a, *befund(f.b, "verdeckt"));
        pruefe(!e.ok && e.geaendert == 0, "geaenderte Karte: Behebung abgelehnt (" + e.meldung + ")");
    }
}
}  // namespace

// --- Nachtrag: Pruefungen aus bekannten Fehlern (NRC-/q3map2-/OpenJK-Meldungen) ------

void testeBekannteFehler(const std::string& q3dir) {
    // Shader-Skript: q3map_surfacelight, q3map_backsplash, surfaceparm sky / origin
    {
        const std::string text =
            "// Kommentar mit textures/korriban/kor1_lava\n"
            "textures/korriban/kor1_lava\n{\n\tqer_editorimage textures/x\n\tq3map_surfacelight 1600\n\tsurfaceparm lava\n"
            "\t{\n\t\tmap $lightmap // q3map_surfacelight 9\n\t}\n}\n"
            "textures/a/b { q3map_surfacelight 10 q3map_backsplash 100 5 surfaceparm sky }\n"
            "/* textures/system/origin { surfaceparm nodraw } */\n"
            "textures/system/origin\n{\n\tsurfaceparm origin\n}\n";
        const ShaderSkript lava = shaderSkriptLesen(text, "textures/korriban/kor1_lava");
        pruefe(lava.gefunden && lava.flaechenlicht == 1600.0 && lava.backsplashAnteil == 0.05 && lava.backsplashWeite == 23.0 && !lava.himmel,
               "Shader-Skript: kor1_lava q3map_surfacelight 1600, Backsplash wie q3map2 (5 %, 23), Stufen und Kommentare zaehlen nicht");
        const ShaderSkript ab = shaderSkriptLesen(text, "TEXTURES/A/B");
        pruefe(ab.gefunden && ab.flaechenlicht == 10.0 && ab.backsplashAnteil == 1.0 && ab.backsplashWeite == 5.0 && ab.himmel,
               "Shader-Skript: q3map_backsplash 100 5 und surfaceparm sky (ohne Gross/klein)");
        const ShaderSkript ursprung = shaderSkriptLesen(text, "textures/system/origin");
        pruefe(ursprung.gefunden && ursprung.ursprung, "Shader-Skript: surfaceparm origin (der Block im /* */-Kommentar zaehlt nicht)");
        pruefe(!shaderSkriptLesen(text, "textures/fehlt").gefunden, "Shader-Skript: unbekannter Name nicht gefunden");
    }
    // Stufe 6: die einmal zerlegte Datei (ShaderDatei) ergibt fuer jeden Namen
    // dasselbe wie die alte Suche je Name (hier als Referenz nachgebaut)
    {
        auto alteSuche = [](const std::string& t, const std::string& name) {
            ShaderSkript r;
            const std::vector<std::string> w = shaderDateiZerlegen(t).woerter;   // die Woerter selbst sind nicht Gegenstand
            std::string gesucht = name;
            for (char& c : gesucht) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            auto kl = [](std::string x) {
                for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                return x;
            };
            int tiefe = 0;
            for (std::size_t i = 0; i < w.size(); ++i) {
                if (w[i] == "{") {
                    ++tiefe;
                    continue;
                }
                if (w[i] == "}") {
                    tiefe = std::max(0, tiefe - 1);
                    continue;
                }
                if (tiefe != 0 || kl(w[i]) != gesucht || i + 1 >= w.size() || w[i + 1] != "{") { continue; }
                ShaderDatei einzeln;
                einzeln.woerter = w;
                einzeln.anfang.emplace(gesucht, i);
                return shaderSkriptLesen(einzeln, name);
            }
            return r;
        };
        const std::string text =
            "textures/d/a { q3map_surfacelight 5 }\n"
            "textures/D/A { q3map_surfacelight 7 surfaceparm sky }\n"          // zweiter gleicher Name: der erste zaehlt
            "textures/d/b surfaceparm\n{ q3map_backsplash 50 8 }\n"            // Name ohne { dahinter
            "textures/d/b { { textures/d/c { q3map_surfacelight 9 } } surfaceparm origin }\n"   // verschachtelt: kein Name
            "} } textures/d/c { q3map_lightmapSize 256 64 }\n"                 // ueberzaehlige }
            "/* textures/d/e { surfaceparm sky } */ \"textures/d/e\" { q3map_surfacelight 3 }\n"
            "textures/d/f {";                                                  // offener Block am Ende
        const ShaderDatei d = shaderDateiZerlegen(text);
        bool gleich = true;
        int gefunden = 0;
        for (const char* n : {"textures/d/a", "TEXTURES/D/A", "textures/d/b", "textures/d/c", "textures/d/e", "textures/d/f", "surfaceparm", "{", "}", "textures/fehlt"}) {
            const ShaderSkript x = shaderSkriptLesen(d, n);
            const ShaderSkript y = alteSuche(text, n);
            const ShaderSkript z = shaderSkriptLesen(text, n);
            for (const ShaderSkript* v : {&x, &z}) {
                gleich = gleich && v->gefunden == y.gefunden && v->flaechenlicht == y.flaechenlicht && v->backsplashAnteil == y.backsplashAnteil &&
                         v->backsplashWeite == y.backsplashWeite && v->himmel == y.himmel && v->ursprung == y.ursprung && v->lightmapBreite == y.lightmapBreite &&
                         v->lightmapHoehe == y.lightmapHoehe;
            }
            gefunden += x.gefunden ? 1 : 0;
        }
        const ShaderSkript a = shaderSkriptLesen(d, "textures/d/a");
        const ShaderSkript b = shaderSkriptLesen(d, "textures/d/b");
        pruefe(gleich && gefunden == 7 && a.flaechenlicht == 5.0 && !a.himmel && b.ursprung && b.backsplashWeite == 23.0,
               "Shader-Datei einmal zerlegt: fuer jeden Namen dasselbe wie die Suche je Name (erster Block, ohne Gross/klein, nur oberste Ebene)");
    }
    // Lichtstile (style, _style, targetname) und Schalter im Build-Menue
    {
        const std::string welt = entText("worldspawn", "", kastenText(0, 0, 0, 64, 64, 64, "test/x"));
        pruefe(!lichtstile(abbildAus(welt + entText("light", "\"origin\" \"0 0 100\"\n", ""))), "ein Licht ohne Stil: keine Lichtstile");
        pruefe(lichtstile(abbildAus(welt + entText("light", "\"origin\" \"0 0 100\"\n\"style\" \"1\"\n", ""))), "Licht mit style 1: Lichtstile");
        pruefe(lichtstile(abbildAus(welt + entText("light", "\"origin\" \"0 0 100\"\n\"targetname\" \"an\"\n", ""))),
               "Licht mit targetname (schaltbar): Lichtstile");
        const std::vector<std::pair<std::string, std::string>> ext = {{"A", "[q3map2] -light -fast -external \"[MapFile]\""}};
        const std::vector<std::pair<std::string, std::string>> gross = {{"B", "[q3map2] -light -lightmapsize 256 \"[MapFile]\""}};
        const std::vector<std::pair<std::string, std::string>> normal = {{"C", "[q3map2] -light -lightmapsize 128 \"[MapFile]\""},
                                                                         {"D", "[q3map2] -vis -external \"[MapFile]\""}};
        pruefe(bauBefehlProbleme(GrenzSpiel::Jk2, false, ext).size() == 1, "JK2 mit -external: gesperrt (NRC#98)");
        const auto g = bauBefehlProbleme(GrenzSpiel::Jk2, false, gross);
        pruefe(g.size() == 1 && g.front().schalter == "-lightmapsize 256", "JK2 mit -lightmapsize 256: gesperrt (externe Lightmaps, light.cpp:2544)");
        pruefe(bauBefehlProbleme(GrenzSpiel::Jk2, true, normal).empty(), "JK2 mit -lightmapsize 128 und -external ausserhalb von -light: frei");
        pruefe(bauBefehlProbleme(GrenzSpiel::JkaSp, false, ext).empty(), "JKA ohne Lichtstile mit -external: frei");
        pruefe(bauBefehlProbleme(GrenzSpiel::JkaSp, true, ext).size() == 1 && bauBefehlProbleme(GrenzSpiel::MovieDuels, true, gross).size() == 1,
               "JKA/MD mit Lichtstilen und externen Lightmaps: gesperrt (NRC#48)");
        PruefOptionen o = pruefOpt();
        o.spiel = GrenzSpiel::Jk2;
        o.bauBefehle = ext;
        pruefe(anzahlVon(pruefeKarte(abbildAus(welt), o), "baubefehl") == 1, "Optimierer: Befund baubefehl fuer JK2 mit -external");
    }
    // Stufe 6: Shader mit q3map_lightmapSize (externe Lightmaps) neben Lichtstilen (NRC#48) / in JK2 (NRC#98)
    {
        const std::string welt = entText("worldspawn", "", kastenText(-64, -64, -16, 64, 64, 0, "test/gross") + kastenText(-64, -64, 64, 64, 64, 80, "test/x"));
        const std::string stil = entText("light", "\"origin\" \"0 0 32\"\n\"style\" \"5\"\n", "");
        PruefOptionen o = pruefOpt();
        ShaderArt gross;
        gross.lightmapBreite = 512;
        gross.lightmapHoehe = 512;
        o.shader["textures/test/gross"] = gross;
        pruefe(anzahlVon(pruefeKarte(abbildAus(welt + stil), o), "lightmapextern") == 1, "q3map_lightmapSize 512 und ein Lichtstil (JKA): 1 Pinsel (NRC#48)");
        pruefe(anzahlVon(pruefeKarte(abbildAus(welt), o), "lightmapextern") == 0, "q3map_lightmapSize 512 ohne Lichtstil (JKA): kein Befund");
        o.spiel = GrenzSpiel::Jk2;
        pruefe(anzahlVon(pruefeKarte(abbildAus(welt), o), "lightmapextern") == 1, "q3map_lightmapSize 512 in Jedi Outcast: 1 Pinsel auch ohne Stil (NRC#98)");
        o.spiel = GrenzSpiel::JkaSp;
        gross.lightmapBreite = 128;
        gross.lightmapHoehe = 128;
        o.shader["textures/test/gross"] = gross;
        pruefe(anzahlVon(pruefeKarte(abbildAus(welt + stil), o), "lightmapextern") == 0, "q3map_lightmapSize 128 mit Lichtstil: kein Befund");
        const ShaderSkript sk = shaderSkriptLesen("textures/test/gross\n{\n\tq3map_lightmapSize 256 512 // extern\n}\n", "textures/test/gross");
        pruefe(sk.gefunden && sk.lightmapBreite == 256 && sk.lightmapHoehe == 512, "Shader-Skript: q3map_lightmapSize 256 512 gelesen");
    }
    // Stufe 8a Punkt 2: Entity-Lump gegen die Grenzen ("enttext"), -keeplights in der Zaehlung
    {
        const std::string welt = entText("worldspawn", "", kastenText(-64, -64, -16, 64, 64, 0, "test/boden"));
        std::string lichter;
        for (int i = 0; i < 3; ++i) { lichter += entText("light", "\"origin\" \"" + std::to_string(i * 16) + " 0 32\"\n\"light\" \"300\"\n", ""); }
        const auto enttext = [](const KartenAbbild& a, bool behalten) {
            ZaehlOptionen zo;
            zo.spiel = GrenzSpiel::JkaSp;
            zo.lichterBehalten = behalten;
            for (const Zaehlung& z : zaehleKarte(a, zo)) {
                if (z.id == "bsp.enttext") { return z.wert; }
            }
            return -1LL;
        };
        const KartenAbbild a = abbildAus(welt + lichter);
        const long long ohne = enttext(a, false);
        const long long mit = enttext(a, true);
        // je Licht "{\n" "}\n" (4) + "classname" "light" (9+5+6) + "origin" "x 0 32" + "light" "300"
        long long erwartet = 0;
        for (int i = 0; i < 3; ++i) {
            const std::string o = std::to_string(i * 16) + " 0 32";
            erwartet += 4 + (9 + 5 + 6) + (6 + static_cast<long long>(o.size()) + 6) + (5 + 3 + 6);
        }
        pruefe(ohne > 0 && mit - ohne == erwartet, "-keeplights: der Entity-Text waechst um die drei Lichter (" + std::to_string(mit - ohne) + " = " +
                                                       std::to_string(erwartet) + " Byte)");
        PruefOptionen o = pruefOpt();
        pruefe(anzahlVon(pruefeKarte(a, o), "enttext") == 0, "kleine Karte: kein Befund enttext");
        const std::string lang(1100, 'x');
        // der Leser der .map kuerzt wie NRCs Tokeniser auf 1023 Zeichen - ein so langer Wert
        // entsteht im Entity Inspector; hier direkt in der Momentaufnahme
        KartenAbbild langA = abbildAus(welt + entText("info_null", "\"origin\" \"0 0 0\"\n", ""));
        langA.ents[1].schluessel.emplace_back("message", lang);
        const std::vector<Befund> b = pruefeKarte(langA, o);
        const Befund* lt = befund(b, "enttext");
        pruefe(lt != nullptr && lt->schwere == Schwere::Fehler && lt->anzahl == 1, "Wert mit 1100 Zeichen: Befund enttext (Fehler, q3map2 \"Token too large\")");
        // nahe MAX_MAP_ENTSTRING: viele Lichter; mit -keeplights steht ihr Anteil im Befund
        std::string viele;
        for (int i = 0; i < 2600; ++i) {
            viele += entText("light", "\"origin\" \"" + std::to_string(i) + " 0 32\"\n\"light\" \"300\"\n\"_color\" \"1 0.9 0.8\"\n\"radius\" \"" +
                                          std::string(40, '9') + "\"\n", "");
        }
        const KartenAbbild va = abbildAus(welt + viele);
        o.lichterBehalten = false;
        pruefe(anzahlVon(pruefeKarte(va, o), "enttext") == 0, "2600 Lichter ohne -keeplights: kein Befund (sie kommen nicht in den Lump)");
        o.lichterBehalten = true;
        const std::vector<Befund> vb = pruefeKarte(va, o);
        const Befund* gr = befund(vb, "enttext");
        pruefe(gr != nullptr && gr->schwere == Schwere::Warnung && gr->detailWerte.size() == 3 && std::stoll(gr->detailWerte[0]) > 0x40000 &&
                   std::stoll(gr->detailWerte[2]) > 0x40000 - 1000,
               "2600 Lichter mit -keeplights: ueber MAX_MAP_ENTSTRING, Warnung mit dem Anteil der Lichter (" +
                   (gr != nullptr ? gr->detail : std::string("-")) + ")");
    }
    // common/origin statt system/origin (NRC#270)
    {
        const std::string t = entText("worldspawn", "", kastenText(-64, -64, -16, 64, 64, 0, "test/boden")) +
                              entText("func_door", "\"angle\" \"90\"\n", kastenText(0, 0, 0, 8, 64, 64, "test/tuer") + kastenText(0, 0, 0, 8, 8, 8, "common/origin"));
        PruefOptionen o = pruefOpt();
        const std::vector<Befund> b = pruefeKarte(abbildAus(t), o);
        const Befund* co = befund(b, "commonorigin");
        pruefe(co != nullptr && co->anzahl == 6 && co->behebung == Behebung::TexturErsetzen && co->wert == "textures/system/origin",
               "common/origin im func_door: 6 Flaechen, Behebung system/origin");
        ShaderArt mod;
        mod.ursprung = true;
        o.shader["textures/common/origin"] = mod;
        pruefe(anzahlVon(pruefeKarte(abbildAus(t), o), "commonorigin") == 0, "ein Mod mit common/origin samt surfaceparm origin: kein Befund");
        Fall f(t);
        behebenPruefen("system/origin", f, "commonorigin", 0);
        long long sys = 0;
        for (const auto& p : f.k.entitaeten()[1]->kinder()) {
            const auto& pin = static_cast<const Pinsel&>(*p);
            for (std::size_t i = 0; i < pin.anzahl(); ++i) { sys += pin[i].shader() == "textures/system/origin" ? 1 : 0; }
        }
        pruefe(sys == 6, "system/origin: die 6 Flaechen des Ursprungs-Pinsels (" + std::to_string(sys) + ")");
    }
    // riesige Pinsel (GT#271)
    {
        const std::string riesig = entText("worldspawn", "", kastenText(-10000, 0, 0, 10000, 64, 64, "test/x"));
        const std::string gross = entText("worldspawn", "", kastenText(-4096, 0, 0, 4096, 64, 64, "test/x"));
        pruefe(anzahlVon(pruefeKarte(abbildAus(riesig), pruefOpt()), "riesenpinsel") == 1, "Pinsel 20000 lang: riesig");
        pruefe(anzahlVon(pruefeKarte(abbildAus(gross), pruefOpt()), "riesenpinsel") == 0, "Pinsel 8192 lang: kein Befund");
    }
    // Flaechenlicht unter festen Flaechen: der Backsplash (light_bounce.cpp:544-566)
    {
        PruefOptionen o = pruefOpt();
        ShaderArt lava;
        lava.undurchsichtig = false;
        lava.flaechenlicht = 1600;
        o.shader["textures/korriban/kor1_lava"] = lava;
        const std::string unten = kastenText(0, 0, -16, 256, 256, 0, "korriban/kor1_lava");
        const std::string eng = entText("worldspawn", "", unten + kastenText(0, 0, 8, 256, 256, 24, "test/boden"));
        const std::string weit = entText("worldspawn", "", unten + kastenText(0, 0, 40, 256, 256, 56, "test/boden"));
        const std::string frei = entText("worldspawn", "", unten);
        pruefe(anzahlVon(pruefeKarte(abbildAus(eng), o), "flaechenlichtleck") == 1, "Lava 8 unter einer Platte: der Backsplash (23) liegt im Festen");
        pruefe(anzahlVon(pruefeKarte(abbildAus(weit), o), "flaechenlichtleck") == 0, "Platte 40 ueber der Lava: frei");
        pruefe(anzahlVon(pruefeKarte(abbildAus(frei), o), "flaechenlichtleck") == 0, "Lava ohne Platte: frei");
        lava.backsplashAnteil = 0;
        o.shader["textures/korriban/kor1_lava"] = lava;
        pruefe(anzahlVon(pruefeKarte(abbildAus(eng), o), "flaechenlichtleck") == 0, "q3map_backsplash 0: kein Backsplash, kein Befund");
    }
    // misc_model_static: der Pruefpunkt origin + (0 0 zoffset+1) im Festen (OpenJK#1358)
    {
        const std::string bsp = lies(std::filesystem::path(q3dir) / "raum.nrc.bsp");
        const std::string raum = lies(std::filesystem::path(q3dir) / "raum.map");
        if (!bsp.empty() && !raum.empty()) {
            PruefOptionen o = pruefOpt();
            BspAnalyse an;
            an.ok = true;
            o.bsp = &an;
            o.bspDaten = &bsp;
            const std::string modelle = entText("misc_model_static", "\"origin\" \"-384 128 -100\"\n\"model\" \"models/a.md3\"\n", "") +
                                        entText("misc_model_static", "\"origin\" \"-384 128 24\"\n\"model\" \"models/b.md3\"\n", "");
            Fall f(raum + modelle, o);
            const Befund* m = befund(f.b, "modellunsichtbar");
            float z = 0;
            const bool zahl = m != nullptr && m->objekte.size() == 1 && std::sscanf(m->objekte.front().wert.c_str(), "%f", &z) == 1;
            pruefe(m != nullptr && m->anzahl == 1 && zahl && m->behebung == Behebung::EntitaetSchluessel && m->schluessel == "zoffset",
                   "misc_model_static unter dem Boden: Befund, Vorschlag zoffset " + (zahl ? m->objekte.front().wert : std::string("-")));
            pruefe(zahl && bspCluster(bsp, Vektor3f(-384, 128, -100 + z + 1)) >= 0 && bspCluster(bsp, Vektor3f(-384, 128, -100 + z)) == -1,
                   "der Vorschlag ist der kleinste zoffset, der den Pruefpunkt in einen Cluster bringt");
            behebenPruefen("zoffset", f, "modellunsichtbar", 0, o);
        }
    }
}

int main(int argc, char** argv) {
    {   // Stufe 6 (Punkt 5): Vorlagen der Befund-Angaben und ihre Uebersetzung (KeOptD01..)
        bool gleich = bhed::karte::kOptimiererDetailVorlagenAnzahl == 34;   // Stufe 8a: 33, 34 (enttext)
        for (std::size_t i = 0; i < bhed::karte::kOptimiererDetailVorlagenAnzahl; ++i) {
            char name[16];
            std::snprintf(name, sizeof(name), "KeOptD%02zu", i + 1);
            bhed::i18n::Str id{};
            if (!bhed::i18n::strVonName(name, &id) ||
                std::strcmp(bhed::i18n::trIn(bhed::i18n::Language::English, id), bhed::karte::kOptimiererDetailVorlagen[i]) != 0) {
                std::printf("FEHL  Vorlage %zu (%s) ohne passende Uebersetzung %s\n", i, bhed::karte::kOptimiererDetailVorlagen[i], name);
                gleich = false;
            }
        }
        pruefe(gleich, "Optimierer: jede Vorlage einer Befund-Angabe hat ihre Uebersetzung KeOptD.. in derselben Reihenfolge");
        pruefe(bhed::karte::detailText("%s = %s (%s %)", {"a", "1", "50"}) == "a = 1 (50 %)", "detailText setzt der Reihe nach ein, % bleibt");
    }

    testeKartenGrenzen();
    testePatchKollision();
    testeBsp();
    auto arg = [&](int i) { return argc > i ? std::string(argv[i]) : std::string(); };
    testePruefungen(arg(4));
    testeBeheben();
    testeBekannteFehler(arg(4));
    // echte Karten (nur lesen)
    spielAbgleich({arg(1)}, GrenzSpiel::JkaSp, "Jedi Academy");
    spielAbgleich({arg(2)}, GrenzSpiel::Jk2, "Jedi Outcast");
    {
        std::vector<std::string> md;
        std::string s = arg(3);
        std::size_t p = 0;
        while (!s.empty()) {
            p = s.find(';');
            md.push_back(s.substr(0, p));
            if (p == std::string::npos) { break; }
            s = s.substr(p + 1);
        }
        spielAbgleich(md, GrenzSpiel::MovieDuels, "Movie Duels");
    }
    // Zeitmessung an einer grossen Karte (optional): <map> [<bsp>]
    if (!arg(5).empty() && arg(5) != "-") {
        using Uhr = std::chrono::steady_clock;
        auto ms = [](Uhr::time_point a, Uhr::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
        const auto t0 = Uhr::now();
        const std::string text = lies(arg(5));
        Karte k;
        (void)karteLesen(text, k);
        k.anmelden();
        const auto t1 = Uhr::now();
        const KartenAbbild a = kartenAbbild(k);
        const auto t2 = Uhr::now();
        const ZaehlOptionen zo = optionen(GrenzSpiel::MovieDuels);
        std::vector<Zaehlung> z = zaehleKarte(a, zo);
        const auto t3 = Uhr::now();
        std::string bspDaten = arg(6).empty() || arg(6) == "-" ? std::string() : lies(arg(6));
        BspAnalyse ba;
        if (!bspDaten.empty()) {
            ba = analysiereBsp(bspDaten, zo);
            z.insert(z.end(), ba.werte.begin(), ba.werte.end());
        }
        const auto t4 = Uhr::now();
        PruefOptionen po;
        po.spiel = GrenzSpiel::MovieDuels;
        po.bewertungen = bewerten(GrenzSpiel::MovieDuels, z, 0.9);
        po.bsp = ba.ok ? &ba : nullptr;
        po.bspDaten = ba.ok ? &bspDaten : nullptr;
        std::vector<std::pair<std::string, double>> zeiten;
        po.zeiten = &zeiten;
        const std::vector<Befund> b = pruefeKarte(a, po);
        const auto t5 = Uhr::now();
        long long pinsel = 0;
        for (const AbbildEntitaet& e : a.ents) { pinsel += static_cast<long long>(e.prims.size()); }
        std::printf("Grosse Karte %s: %zu Entitaeten, %lld Primitive\n", arg(5).c_str(), a.ents.size(), pinsel);
        std::printf("  laden %.0f ms, Abbild %.0f ms, Zaehlen (Karte) %.0f ms, BSP %.0f ms, Pruefen %.0f ms\n", ms(t0, t1), ms(t1, t2), ms(t2, t3), ms(t3, t4), ms(t4, t5));
        for (const Bewertung& x : po.bewertungen) {
            if (x.ampel != Ampel::Ok) {
                std::printf("  %-6s %-26s %s %lld / %lld\n", ampelName(x.ampel), x.zaehlung.id.c_str(), x.zaehlung.quelle == ZaehlQuelle::Bsp ? "bsp" : "map", x.zaehlung.wert,
                            x.grenze->erlaubt);
            }
        }
        for (const Befund& x : b) { std::printf("  %-8s %-18s %lld  %s\n", schwereName(x.schwere), x.pruefung.c_str(), x.anzahl, x.detail.c_str()); }
        std::printf("  Zeiten:");
        for (const auto& [w, t] : zeiten) { std::printf(" %s %.0f", w.c_str(), t); }
        std::printf(" ms\n");
    }
    std::printf("karteoptimierer: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
