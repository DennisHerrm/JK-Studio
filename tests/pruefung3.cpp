// pruefung3.cpp - Pruefung 3: der Rest gegen das Spiel, je Punkt gegen die
// Formel der Engine (OpenJK, Movie Duels, Jedi Outcast) mit eigenen Daten.
//
// 1. Rumpfwinkel aus dem Skelett: chestBolt (G_SetG2PlayerModelInfo),
//    G_StandardHumanoid, NEGATIVE_Z -> vectoangles (cg_players.cpp).
// 3. Gemeinsamer Markenspeicher: Decals der Effekte (CG_ImpactMark) und
//    Brandspuren der Schwerter in cg_markPolys (256, CG_AllocMark,
//    CG_AddMarks), Reihenfolge im Bild wie CG_DrawActiveFrame.
// 4. Gehen in der Hocke: SCF_CROUCHED -> ucmd.upmove -127, PM_CheckDuck,
//    pm_duckScale, BOTH_CROUCH1WALK(BACK) mit moveSpeedOfAnim 75.
#include "bhed/ablauf.h"
#include "bhed/bspgeo.h"
#include "bhed/efx/io.h"
#include "bhed/efxdraw.h"
#include "bhed/navlauf.h"
#include "bhed/npcklasse.h"
#include "bhed/regelwerk.h"
#include "bhed/scene.h"
#include "bhed/script.h"
#include "bhed/todesanim.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int g_fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

bool nah(float a, float b, float d = 0.001F) { return std::fabs(a - b) <= d; }

std::string w3(const float w[3]) {
    char b[96];
    std::snprintf(b, sizeof b, "%.3f %.3f %.3f", static_cast<double>(w[0]), static_cast<double>(w[1]),
                  static_cast<double>(w[2]));
    return b;
}

struct Spiel {
    int nr;
    const char* name;
};
constexpr Spiel kSpiele[] = {{kRegelwerkOpenJK, "OpenJK"}, {kRegelwerkMovieDuels, "Movie Duels"}, {kRegelwerkJediOutcast, "Jedi Outcast"}};

// --- 1. Rumpfwinkel ----------------------------------------------------------

void rumpfwinkel() {
    std::printf("--- 1. Rumpfwinkel aus dem chestBolt ---\n");
    // vectoangles (q_math.c:616 OJ): PITCH = -atan2(z, |xy|) (negativ = hoch),
    // YAW 0..360, gerade hoch 90 -> -90, gerade runter 270 -> -270.
    struct Fall {
        float v[3];
        float soll[3];
    };
    const Fall faelle[] = {{{1, 0, 0}, {0, 0, 0}},     {{0, 1, 0}, {0, 90, 0}},     {{-1, 0, 0}, {0, 180, 0}},
                           {{0, -1, 0}, {0, 270, 0}},  {{1, 1, 0}, {0, 45, 0}},     {{1, 0, 1}, {-45, 0, 0}},
                           {{1, 0, -1}, {-315, 0, 0}}, {{0, 0, 1}, {-90, 0, 0}},    {{0, 0, -1}, {-270, 0, 0}}};
    for (const Fall& f : faelle) {
        float w[3];
        engineVectoangles(f.v, w);
        pruefe(nah(w[0], f.soll[0]) && nah(w[1], f.soll[1]) && nah(w[2], f.soll[2]),
               "vectoangles(" + w3(f.v) + ") = " + w3(f.soll) + " - " + w3(w));
    }
    // NEGATIVE_Z der Bolzenmatrix: die dritte Spalte verneint. Eine Figur auf
    // dem Ruecken: die Brust (-Z des Knochens) zeigt nach oben -> PITCH -90,
    // AngleNormalize180 < 0 -> BOTH_DEATH_LYING_UP; auf dem Bauch nach unten.
    {
        const float rueckenOben[3][4] = {{1, 0, 0, 10}, {0, 1, 0, 20}, {0, 0, -1, 30}};
        float w[3];
        rumpfWinkelAusBolzen(rueckenOben, w);
        pruefe(nah(w[0], -90.0F), "Bolzen mit -Z nach oben: PITCH -90 (Ruecken) - " + w3(w));
        const float bauch[3][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
        rumpfWinkelAusBolzen(bauch, w);
        pruefe(nah(w[0], -270.0F), "Bolzen mit -Z nach unten: PITCH -270 = +90 (Bauch) - " + w3(w));
        // stehend, Brust nach +Y: -Z = (0, 1, 0) -> YAW 90
        const float stehend[3][4] = {{1, 0, 0, 0}, {0, 0, -1, 0}, {0, 1, 0, 0}};
        rumpfWinkelAusBolzen(stehend, w);
        pruefe(nah(w[0], 0.0F) && nah(w[1], 90.0F), "Bolzen mit -Z nach +Y: YAW 90, PITCH 0 - " + w3(w));
    }
    // Der Knochen des chestBolt je Regelwerk (G_SetG2PlayerModelInfo).
    struct Knochen {
        const char* modell;
        const char* gla;
        const char* klasse;
        const char* soll[3];   // OpenJK, Movie Duels, Jedi Outcast
    };
    const Knochen knochen[] = {
        // ein eigenes Modell auf dem Grundskelett
        {"eigene_figur", "models/players/_humanoid/_humanoid", "CLASS_REBEL", {"thoracic", "thoracic", "thoracic"}},
        // MD-Skelett: OpenJK zaehlt jedes "models/players/_humanoid*" (Q_stricmpn 24)
        {"eigene_figur", "models/players/_humanoid_md/_humanoid_md", "CLASS_JEDI", {"thoracic", "thoracic", "thoracic"}},
        // protocol: "no thoracic bone" -> upper_lumbar; JK2 nach dem Namen
        {"protocol", "models/players/protocol/protocol", "CLASS_PROTOCOL", {"upper_lumbar", "upper_lumbar", "upper_lumbar"}},
        // Droiden ohne chestBolt
        {"mark1", "models/players/mark1/mark1", "CLASS_MARK1", {"", "", ""}},
        {"r2d2_eigen", "models/players/r2d2/r2d2", "CLASS_R2D2", {"", "", ""}},
        {"atst", "models/players/atst/atst", "CLASS_ATST", {"", "", ""}},
        // CLASS_HOWLER: OpenJK/MD nach der Klasse, JK2 nach dem Modell "howler"
        {"eigenes_tier", "models/players/eigenes_tier/eigenes_tier", "CLASS_HOWLER", {"", "", "thoracic"}},
        {"howler", "models/players/howler/howler", "CLASS_HOWLER", {"", "", ""}},
        // wampa: OpenJK und MD zaehlen sein Skelett zu den Humanoiden -> thoracic;
        // auf einem fremden Skelett MD/OJ upper_spine, JK2 kennt keinen wampa
        {"wampa", "models/players/wampa/wampa", "CLASS_WAMPA", {"thoracic", "thoracic", "thoracic"}},
        {"wampa", "models/players/eigen/eigen", "CLASS_WAMPA", {"upper_spine", "upper_spine", "thoracic"}},
        // galak_mech: MD Humanoid, OpenJK sonst-Zweig (thoracic), JK2 eigener Zweig ohne
        {"galak_mech", "models/players/galak_mech/galak_mech", "CLASS_GALAKMECH", {"thoracic", "thoracic", ""}},
        // kotor_monster: nur MD kennt es (Humanoid-Liste), sonst thoracic
        {"kotor_monster", "models/players/eigen/eigen", "CLASS_MONSTER", {"thoracic", "upper_spine", "thoracic"}},
        {"rancor", "models/players/rancor/rancor", "CLASS_RANCOR", {"", "", "thoracic"}},
    };
    for (const Knochen& k : knochen) {
        for (int i = 0; i < 3; ++i) {
            const std::string ist = rumpfBolzenKnochen(k.modell, k.gla, k.klasse, kSpiele[i].nr);
            pruefe(ist == k.soll[i], std::string("chestBolt ") + k.modell + " (" + k.gla + ", " + kSpiele[i].name + "): \"" + k.soll[i] +
                                         "\" - \"" + ist + "\"");
        }
    }
    // G_StandardHumanoid nach dem Namen (JK2): "st" ist eine Vorsilbe (Q_strncmp)
    pruefe(standardHumanoid("stormtrooper", "", kRegelwerkJediOutcast) && !standardHumanoid("eigene_figur", "", kRegelwerkJediOutcast),
           "Jedi Outcast: G_StandardHumanoid nach dem Modellnamen (Vorsilbe \"st\")");
    pruefe(standardHumanoid("x", "models/players/JK2anims/JK2anims", kRegelwerkMovieDuels) &&
               !standardHumanoid("x", "models/players/JK2anims/JK2anims", kRegelwerkOpenJK),
           "JK2anims: nur Movie Duels zaehlt es zu den Humanoiden");
}

// --- 3. Gemeinsamer Markenspeicher ------------------------------------------

// Eine Wand x = 20 (Normale -x), y -50..50, z 0..50 (wie pruefung2).
BspGeometry wandKarte() {
    BspGeometry g;
    g.planes.push_back(BspGeometry::Plane{{1.0F, 0.0F, 0.0F}, 1000.0F});
    BspGeometry::Node n;
    n.plane = 0;
    n.children[0] = -1;
    n.children[1] = -1;
    g.nodes.push_back(n);
    BspGeometry::Leaf blatt;
    blatt.firstSurface = 0;
    blatt.numSurfaces = 1;
    g.leafs.push_back(blatt);
    g.leafSurfaces = {0};
    BspShader wand;
    wand.name = "textures/eigen/wand";
    g.shaders = {wand};
    for (int i = 0; i < 4; ++i) {
        BspVertex v;
        v.xyz[0] = 20.0F;
        v.xyz[1] = (i == 1 || i == 2) ? 50.0F : -50.0F;
        v.xyz[2] = (i >= 2) ? 50.0F : 0.0F;
        g.verts.push_back(v);
    }
    g.indexes = {0, 1, 2, 0, 2, 3};
    BspSurface f;
    f.type = BspSurface::Type::Planar;
    f.firstVert = 0;
    f.numVerts = 4;
    f.firstIndex = 0;
    f.numIndexes = 6;
    f.ebenenNormale[0] = -1.0F;
    g.surfaces = {f};
    return g;
}

void markenspeicher() {
    std::printf("--- 3. Gemeinsamer Markenspeicher ---\n");
    const BspGeometry g = wandKarte();
    const auto lies = [](const char* text) { return efx::read(text).effect; };
    const efx::Effect sofortFx = lies("Decal\n{\n\tlife\t\t1000 1000\n\tshaders\n\t[\n\t\tgfx/eigen/brand\n\t]\n"
                                      "\tsize\n\t{\n\t\tstart 10\n\t}\n\talpha\n\t{\n\t\tstart 0.5\n\t}\n}\n");
    const efx::Effect spaeterFx = lies("Decal\n{\n\tdelay\t\t100 100\n\tlife\t\t1000 1000\n\tshaders\n\t[\n\t\tgfx/eigen/brand2\n\t]\n"
                                       "\tsize\n\t{\n\t\tstart 10\n\t}\n}\n");
    const auto runner = [](const efx::Effect& e, double ab) {
        EffectInstance in;
        in.effect = &e;
        in.startMs = ab;
        in.seed = 7U;
        in.origin[0] = 20.0F;
        in.origin[2] = 25.0F;
        in.normal[0] = -1.0F;
        in.normal[2] = 0.0F;
        return in;
    };
    // buildEffectMesh baut keine Decals mehr - sie liegen im Markenspeicher.
    {
        const float rechts[3] = {0.0F, 1.0F, 0.0F};
        const float hoch[3] = {0.0F, 0.0F, 1.0F};
        const BspMesh m = buildEffectMesh({runner(sofortFx, 0.0)}, 300.0, rechts, hoch, nullptr, &g);
        pruefe(m.verts.empty(), "Decal: kein Viereck im Effektnetz (CG_ImpactMark legt eine Marke an)");
    }
    // Ohne delay entsteht das Decal im Ereignis (sofort), mit delay im Zeitplaner.
    const std::vector<EffektMarke> a = effektMarkenIn({runner(sofortFx, 1000.0)}, 0.0, 2000.0, &g);
    const std::vector<EffektMarke> b = effektMarkenIn({runner(spaeterFx, 1000.0)}, 0.0, 2000.0, &g);
    pruefe(a.size() == 1 && a[0].sofort && std::fabs(a[0].ms - 1000.0) < 0.01 && !a[0].polys.empty(),
           "Decal ohne delay: bei 1000 ms, sofort (CreateEffect im Ereignis)");
    pruefe(b.size() == 1 && !b[0].sofort && std::fabs(b[0].ms - 1100.0) < 0.01, "Decal mit delay 100: bei 1100 ms, im Zeitplaner");
    if (a.empty() || b.empty()) { return; }
    pruefe(a[0].polys[0].shader >= kSpurShaderEffekt && spurShaderName(a[0].polys[0].shader) == "gfx/eigen/brand" &&
               a[0].polys[0].alphaFade && a[0].polys[0].eckenAlpha == static_cast<std::uint8_t>(0.5F * 255.0F),
           "Decal-Marke: Shader des Teilchens, alphaFade, Alpha 0,5 in den Ecken (CG_ImpactMark)");
    // Brandspuren eines Bildes (cg.time 1000) - synthetische Marken.
    const auto brand = [](int zeit, int n) {
        std::vector<SpurPoly> v;
        for (int i = 0; i < n; ++i) {
            SpurPoly p;
            p.zeit = zeit;
            p.zahl = 3;
            p.shader = 0;
            v.push_back(p);
        }
        return v;
    };
    // Reihenfolge in einem Bild: sofortiges Decal, Brandspuren, CG_AddMarks,
    // verzoegertes Decal - CG_AddMarks zeichnet die neueste zuerst.
    {
        SpurSpeicher s;
        std::size_t k = 0;
        std::vector<EffektMarke> beide = a;
        EffektMarke spaet = b[0];
        spaet.ms = 1000.0;   // im selben Bild
        beide.push_back(spaet);
        const std::vector<SpurPoly> klingen = brand(1000, 2);
        markenBild(s, 1000, beide, k, &klingen);
        const std::vector<const SpurPoly*> l = s.lebende();
        const std::size_t na = a[0].polys.size();
        const std::size_t nb = spaet.polys.size();
        bool ok = l.size() == na + nb + 2;
        for (std::size_t i = 0; ok && i < l.size(); ++i) {
            const int soll = i < nb ? spaet.polys[0].shader : (i < nb + 2 ? 0 : a[0].polys[0].shader);
            ok = l[i]->shader == soll && l[i]->zeit == 1000;
        }
        pruefe(ok, "ein Bild: verzoegertes Decal, Brandspuren, sofortiges Decal (neueste zuerst), alle mit mark->time 1000");
    }
    // 256 Plaetze fuer beide: 250 Brandspuren bei 1000 ms, dann das Decal bei
    // 2000 ms - CG_AllocMark wirft die aelteste Zeitgruppe ganz ab.
    {
        SpurSpeicher s;
        std::size_t k = 0;
        const std::vector<SpurPoly> viele = brand(1000, kSpurMaxPolys - 6);
        const std::vector<EffektMarke> keine;
        markenBild(s, 1000, keine, k, &viele);
        std::vector<EffektMarke> d = a;
        d[0].ms = 2000.0;
        while (d[0].polys.size() < 10U) { d[0].polys.push_back(d[0].polys[0]); }
        k = 0;
        markenBild(s, 2000, d, k, nullptr);
        pruefe(s.zahl() == static_cast<int>(d[0].polys.size()),
               "250 Brandspuren + Decal mit 10 Marken: die Brandspuren (eine Zeitgruppe) fallen alle (CG_AllocMark) - " +
                   std::to_string(s.zahl()) + " Marken");
    }
    // Ausblenden: alphaFade -> modulate[3] = 255 * t / 1000 in der letzten Sekunde.
    {
        SpurSpeicher s;
        std::size_t k = 0;
        markenBild(s, 1000, a, k, nullptr);
        std::uint8_t f[4]{};
        spurFarbe(*s.lebende().front(), 10500, f);
        const std::uint8_t erwartet = static_cast<std::uint8_t>(255 * 500 / 1000);
        pruefe(f[3] == erwartet, "Decal-Marke 500 ms vor Ablauf: Alpha 255 * 500 / 1000 = " + std::to_string(erwartet) + " - " + std::to_string(f[3]));
        s.bild(11001);
        pruefe(s.zahl() == 0, "Decal-Marke nach MARK_TOTAL_TIME fort (CG_AddMarks)");
    }
}

// --- Die Welt einer Mini-Mission (wie tests/pruefung2.cpp) -----------------

struct Welt {
    std::string karte;
    std::string skript;                        // ohne Kopfzeile
    std::set<std::string> anims;               // was die Animationsdatei der Figur hat
    std::string klasse = "CLASS_REBEL";
    float hockHoehe = -1.0F;                   // .npc "crouchheight"
    NavSpurFn spur;                            // leer: keine Kartengeometrie
    bool boden = false;                        // ein Boden z = 0 ueberall
};

constexpr double kAnimMs = 1000.0;   // jede Animation: 21 Bilder zu 50 ms
constexpr double kBildMs = 50.0;

struct Lauf {
    MapData karte;
    Scene szene;
};

// Wie die Vorschau (gui/app_view3d.cpp, prepareScene): erst der Ablauf, dann
// eine erste Szene fuer Tempo und Kaesten, dann der gemeinsame Takt aller
// Figuren und die Szene daraus.
void fahre(const Welt& w, int spiel, Lauf& l) {
    regelwerkAnwendenSpiel(spiel, 1);
    parseEntities(w.karte, l.karte.entities);
    Script sc;
    std::vector<Diag> d;
    (void)readScript("//Generated by BehavEd\r\n\r\n" + w.skript, sc, d);
    NpcMap npcs;
    NpcDef def;
    def.playerModel = "eigenes_modell";
    def.walkSpeed = 55.0F;
    def.runSpeed = 200.0F;
    def.yawSpeed = 90.0F;
    def.klasse = w.klasse;
    def.team = "TEAM_NEUTRAL";
    def.hockHoehe = w.hockHoehe;
    npcs["eigen_typ"] = def;
    const std::set<std::string> anims = w.anims;
    AblaufWelt welt;
    welt.karte = &l.karte;
    welt.skript = [&sc](const std::string& p) -> const Script* { return p == "start" ? &sc : nullptr; };
    welt.animDauer = [anims](const std::string&, const std::string& a) { return anims.count(a) != 0 ? kAnimMs : -1.0; };
    auto nav = std::make_shared<NavWelt>(*navWeltAusEntities(l.karte.entities));
    nav->spur = w.spur;
    if (w.boden) {
        nav->boden = [](float, float, float zOben, float zUnten, float& b) {
            if (zUnten <= 0.0F && zOben >= 0.0F) {
                b = 0.0F;
                return true;
            }
            return false;
        };
    }
    welt.nav = nav;
    SzenenHilfe h;
    h.npcs = &npcs;
    h.nav = nav;
    h.kartenFiguren = true;
    h.hatAnim = [anims](const std::string&, const std::string& a) { return anims.count(a) != 0; };
    h.animDauer = welt.animDauer;
    h.bildDauer = [anims](const std::string&, const std::string& a) { return anims.count(a) != 0 ? kBildMs : -1.0; };
    const Ablauf erst = simuliereAblauf(sc, "start", welt);
    h.erscheint = &erst.erscheint;
    h.fremdeDarsteller = &erst.fremdeDarsteller;
    const Scene ersteSzene = buildScene(erst.flach, l.karte, &h);
    welt.tempo = ablaufTempo(ersteSzene);
    std::map<std::string, float> hocken;
    for (const Actor& a : ersteSzene.actors) { hocken[a.name] = a.hockHoehe; }
    welt.hockHoehe = [hocken](const std::string& f) {
        const auto it = hocken.find(f);
        return it == hocken.end() ? 16.0F : it->second;
    };
    const Ablauf ablauf = simuliereAblaufMitKoerpern(sc, "start", welt);
    h.koerperPlaene = welt.koerperPlaene;
    h.erscheint = &ablauf.erscheint;
    h.fremdeDarsteller = &ablauf.fremdeDarsteller;
    l.szene = buildScene(ablauf.flach, l.karte, &h);
}

// Ein Boden z = 0 ueberall: Spuren abwaerts treffen ihn mit der Unterkante.
NavSpur bodenUeberall(const NavVec& s, const NavVec& mins, const NavVec&, const NavVec& e, int, std::uint32_t) {
    NavSpur t;
    t.endpos = e;
    const float unten = s[2] + mins[2];
    if (e[2] < s[2] && unten >= 0.0F && e[2] + mins[2] < 0.0F) {
        t.fraction = unten / (s[2] - e[2]);
        t.endpos = {e[0], e[1], s[2] - unten};
        t.normal = {0.0F, 0.0F, 1.0F};
        t.entity = kNavWeltEntity;
    }
    return t;
}

const char* const kKarte4 =
    "{\n\"classname\" \"worldspawn\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"held\"\n\"NPC_type\" \"eigen_typ\"\n\"origin\" \"0 0 24\"\n\"angle\" \"0\"\n}\n"
    "{\n\"classname\" \"waypoint_navgoal\"\n\"targetname\" \"ziel\"\n\"origin\" \"390 0 24\"\n}\n"
    "{\n\"classname\" \"info_notnull\"\n\"targetname\" \"hinten\"\n\"origin\" \"-400 0 24\"\n}\n";

std::string affect4(const std::string& zeilen) {
    return "affect ( \"held\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
           "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n" +
           zeilen + "\twait ( 8000.000 );\r\n}\r\n";
}
std::string setze(const char* typ, const char* wert) {
    return std::string("\tset ( /*@SET_TYPES*/ \"") + typ + "\", \"" + wert + "\" );\r\n";
}

// Tempo in der Ebene zwischen zwei Zeiten (Einheiten je Sekunde).
float tempoXY(const Lauf& l, double a, double b) {
    const Actor* f = l.szene.find("held");
    if (f == nullptr) { return -1.0F; }
    const ActorState s0 = f->at(a);
    const ActorState s1 = f->at(b);
    const float dx = s1.pos[0] - s0.pos[0];
    const float dy = s1.pos[1] - s0.pos[1];
    return std::sqrt(dx * dx + dy * dy) / static_cast<float>((b - a) / 1000.0);
}
std::string animBei(const Lauf& l, double ms) {
    const Actor* f = l.szene.find("held");
    return f == nullptr ? std::string("(keine Figur)") : f->at(ms).animation;
}

void hocke() {
    std::printf("--- 4. Gehen in der Hocke ---\n");
    for (const Spiel& sp : kSpiele) {
        const std::string r = std::string(" (") + sp.name + ")";
        Welt w;
        w.karte = kKarte4;
        w.anims = {"BOTH_STAND1", "BOTH_RUN1", "BOTH_WALK1", "BOTH_CROUCH1", "BOTH_CROUCH1WALK", "BOTH_CROUCH1WALKBACK"};
        w.spur = bodenUeberall;
        w.boden = true;
        // Ohne Hocke: PM_WalkMove mit moveDir, wishspeed = ps.speed (runSpeed 200).
        Welt stehend = w;
        stehend.skript = affect4(setze("SET_NAVGOAL", "ziel"));
        Lauf l0;
        fahre(stehend, sp.nr, l0);
        const float v0 = tempoXY(l0, 800.0, 1400.0);
        // Geduckt: NPC_ApplyScriptFlags upmove -127, PM_CheckDuck, wishspeed
        // ps.speed * pm_duckScale (0,5) - im Gleichgewicht von PM_Friction
        // (pm_friction 6, pm_stopspeed 100) und PM_Accelerate (12) genau
        // wishspeed, also die Haelfte.
        w.skript = affect4(setze("SET_CROUCHED", "true") + setze("SET_NAVGOAL", "ziel"));
        Lauf l1;
        fahre(w, sp.nr, l1);
        const float v1 = tempoXY(l1, 800.0, 1400.0);
        pruefe(std::fabs(v0 - 200.0F) < 2.0F && std::fabs(v1 - 100.0F) < 2.0F,
               "geduckt: ps.speed * pm_duckScale - stehend " + std::to_string(v0) + ", geduckt " + std::to_string(v1) +
                   " (Soll 200 / 100)" + r);
        const Actor* f = l1.szene.find("held");
        const ActorState st = f != nullptr ? f->at(1200.0) : ActorState{};
        pruefe(st.animation == "BOTH_CROUCH1WALK", "geduckt unterwegs: PM_Footsteps BOTH_CROUCH1WALK" + r + " - " + st.animation);
        // bg_panimate.cpp:4778-4824 OJ: animSpeed *= resultspeed / 75
        pruefe(std::fabs(st.animSpeed - 100.0F / 75.0F) < 0.02F,
               "BOTH_CROUCH1WALK mit moveSpeedOfAnim 75: animSpeed 1,333" + r + " - " + std::to_string(st.animSpeed));
        // Rueckwaerts (Blick nach hinten): PMF_BACKWARDS_RUN -> BOTH_CROUCH1WALKBACK
        Welt zurueck = w;
        zurueck.skript = affect4(setze("SET_CROUCHED", "true") + setze("SET_WATCHTARGET", "hinten") + setze("SET_NAVGOAL", "ziel"));
        Lauf l2;
        fahre(zurueck, sp.nr, l2);
        const std::string a2 = animBei(l2, 1200.0);
        pruefe(a2 == "BOTH_CROUCH1WALKBACK", "geduckt rueckwaerts: BOTH_CROUCH1WALKBACK" + r + " - " + a2);
        // Aufstehen mitten im Weg (SET_CROUCHED false): wieder volles Tempo
        Welt auf = w;
        auf.skript = affect4(setze("SET_CROUCHED", "true") + setze("SET_NAVGOAL", "ziel") + "\twait ( 1000.000 );\r\n" +
                             setze("SET_CROUCHED", "false"));
        Lauf l3;
        fahre(auf, sp.nr, l3);
        const float v3a = tempoXY(l3, 500.0, 950.0);
        const float v3b = tempoXY(l3, 1250.0, 1550.0);
        const std::string a3 = animBei(l3, 1400.0);
        pruefe(std::fabs(v3a - 100.0F) < 2.0F && std::fabs(v3b - 200.0F) < 2.0F && a3 == "BOTH_RUN1",
               "SET_CROUCHED false unterwegs: wieder 200 und BOTH_RUN1 - " + std::to_string(v3a) + " / " + std::to_string(v3b) + " " +
                   a3 + r);
    }
}

}  // namespace

int main() {
    rumpfwinkel();
    markenspeicher();
    hocke();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
