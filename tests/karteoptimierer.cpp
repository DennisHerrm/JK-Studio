// karteoptimierer.cpp - Karten-Editor Stufe 5: der Optimierer
// (bhed/karte/optimierer.h, patchkollision.h).
//
// Punkt 2 (Zaehlen): Testkarten, die jede Grenze knapp unter- und
// ueberschreiten (die Ampel muss von "ok"/"knapp" auf "ueber" springen), eine
// erzeugte RBSP fuer die Lumps (Lichtgitter, Lightmaps, Nebel, Flaechen,
// Modelle) und der Abgleich gegen die echten Karten der drei Spiele: keine
// Originalkarte darf an einer harten Grenze "ueber" sein.
//
// Aufruf: karteoptimierer [<JA base> <JK2 base> <MD base;MD> <dekomp-Wurzel>]
//   (jeder Ordner "-" oder leer = uebersprungen)
#include "bhed/karte/grenzen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/patchkollision.h"
#include "bhed/pk3.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
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

}  // namespace

int main(int argc, char** argv) {
    testeKartenGrenzen();
    testePatchKollision();
    testeBsp();
    // echte Karten (nur lesen)
    auto arg = [&](int i) { return argc > i ? std::string(argv[i]) : std::string(); };
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
    std::printf("karteoptimierer: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
