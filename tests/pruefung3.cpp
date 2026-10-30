// pruefung3.cpp - Pruefung 3: der Rest gegen das Spiel, je Punkt gegen die
// Formel der Engine (OpenJK, Movie Duels, Jedi Outcast) mit eigenen Daten.
//
// 1. Rumpfwinkel aus dem Skelett: chestBolt (G_SetG2PlayerModelInfo),
//    G_StandardHumanoid, NEGATIVE_Z -> vectoangles (cg_players.cpp).
// 3. Gemeinsamer Markenspeicher: Decals der Effekte (CG_ImpactMark) und
//    Brandspuren der Schwerter in cg_markPolys (256, CG_AllocMark,
//    CG_AddMarks), Reihenfolge im Bild wie CG_DrawActiveFrame.
// 2. Tod: TossClientItems, Klaenge (EV_DEATH, DeathFX), Abwurf mit G_RunItem/
//    G_BounceItem bzw. WP_SaberDrop/G_BounceMissile - je Regelwerk.
// 4. Gehen in der Hocke: SCF_CROUCHED -> ucmd.upmove -127, PM_CheckDuck,
//    pm_duckScale, BOTH_CROUCH1WALK(BACK) mit moveSpeedOfAnim 75; die Beine am
//    Hang (PM_AdjustStandAnimForSlope, PM_FootSlopeTrace).
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
#include <numbers>
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
    std::string kartenPfad = "maps/eigene_karte.bsp";
    bool gegenstaende = false;                 // items.dat: jede Waffe -16 -16 -2 / 16 16 16
    bool hand = false;                         // handRPoint (0 0 64), muzzleDir +x
    // Pruefung 4a: nach dem Tod geht die Hand je 50 ms 20 in +x (die
    // Todesanimation) - sonst bleibt sie, und das Schwert haengt an ihr
    bool handWandert = true;
    int saberWurf = 0;                         // .npc "FP_SABERTHROW"
    std::string rang;                          // .npc "rank" (Pruefung 4a: PM_TryRoll)
    bool fuesse = false;                       // footL/RPoint: +-8 in x unter der Figur
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
    l.karte.path = w.kartenPfad;
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
    def.saber = "eigenes_schwert";
    def.snd = "eigene_stimme";
    if (w.saberWurf > 0) { def.kraefte["fp_saberthrow"] = w.saberWurf; }
    def.rang = w.rang;
    npcs["eigen_typ"] = def;
    NpcDef droide = def;
    droide.klasse = "CLASS_R2D2";
    npcs["eigen_droide"] = droide;
    std::map<std::string, SaberDef> schwerter;
    schwerter["eigenes_schwert"].model = "models/eigen/griff.glm";
    schwerter["eigenes_schwert"].soundOff = "sound/eigen/aus.wav";
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
    h.sabers = &schwerter;
    h.nav = nav;
    h.kartenFiguren = true;
    if (w.gegenstaende) {
        h.waffenGegenstand = [](const std::string&, NavVec& mi, NavVec& ma) {
            mi = {-16.0F, -16.0F, -2.0F};
            ma = {16.0F, 16.0F, 16.0F};
            return true;
        };
    }
    if (w.fuesse) {
        h.fussPunkte = [](const Actor& a, double ms, float lf[3], float rf[3]) {
            const ActorState st = a.at(ms);
            lf[0] = st.pos[0] + 8.0F;
            rf[0] = st.pos[0] - 8.0F;
            lf[1] = rf[1] = st.pos[1];
            lf[2] = rf[2] = st.pos[2] - 24.0F;
            return true;
        };
    }
    if (w.hand) {
        const bool wandert = w.handWandert;
        h.handPunkt = [wandert](const Actor& a, double ms, float hp[3], float r[3]) {
            const ActorState st = a.at(ms);
            hp[0] = st.pos[0];
            if (wandert && a.getoetetMs >= 0.0 && ms > a.getoetetMs) { hp[0] += static_cast<float>((ms - a.getoetetMs) * 0.4); }
            hp[1] = st.pos[1];
            hp[2] = st.pos[2] + 40.0F;
            r[0] = 1.0F;
            r[1] = 0.0F;
            r[2] = 0.0F;
            return true;
        };
    }
    h.hatAnim = [anims](const std::string&, const std::string& a) { return anims.count(a) != 0; };
    h.animDauer = welt.animDauer;
    h.bildDauer = [anims](const std::string&, const std::string& a) { return anims.count(a) != 0 ? kBildMs : -1.0; };
    const Ablauf erst = simuliereAblauf(sc, "start", welt);
    h.erscheint = &erst.erscheint;
    h.fremdeDarsteller = &erst.fremdeDarsteller;
    const Scene ersteSzene = buildScene(erst.flach, l.karte, &h);
    welt.tempo = ablaufTempo(ersteSzene);
    // wie die Vorschau: PM_TryRoll je Figur aus Klasse und Rang (Pruefung 4a)
    std::map<std::string, std::pair<std::string, std::string>> rollKlassen;
    for (const Actor& a : ersteSzene.actors) { rollKlassen[a.name] = {a.klasse, a.rang}; }
    welt.darfRollen = [rollKlassen](const std::string& f, const std::string& waffe) {
        const auto it = rollKlassen.find(f);
        return it != rollKlassen.end() && npcDarfRollen(navMovieDuelsRegeln(), navJk2Regeln(), waffe, it->second.first, it->second.second);
    };
    std::map<std::string, float> hocken;
    for (const Actor& a : ersteSzene.actors) { hocken[a.name] = a.hockHoehe; }
    welt.hockHoehe = [hocken](const std::string& f) {
        const auto it = hocken.find(f);
        return it == hocken.end() ? 16.0F : it->second;
    };
    h.darfRollen = welt.darfRollen;
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
        t.endpos = {s[0] + (e[0] - s[0]) * t.fraction, s[1] + (e[1] - s[1]) * t.fraction, s[2] - unten};
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

// --- 2. Tod ------------------------------------------------------------------

const TodesFolge* folgeVon(const Lauf& l, const std::string& figur) {
    for (const TodesFolge& f : l.szene.todesFolgen) {
        if (f.figur == figur) { return &f; }
    }
    return nullptr;
}
const TodesAbwurf* abwurfVon(const Lauf& l, const std::string& figur) {
    for (const TodesAbwurf& a : l.szene.abwuerfe) {
        if (a.figur == figur) { return &a; }
    }
    return nullptr;
}

void tod() {
    std::printf("--- 2. Tod: Waffe, Schwert, Klaenge, DeathFX ---\n");
    const std::string karte =
        "{\n\"classname\" \"worldspawn\"\n}\n"
        "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"held\"\n\"NPC_type\" \"eigen_typ\"\n\"origin\" \"0 0 24\"\n\"angle\" \"0\"\n}\n";
    const std::string totBei500 = "\twait ( 500.000 );\r\n\tkill ( \"held\" );\r\n";
    for (const Spiel& sp : kSpiele) {
        const std::string r = std::string(" (") + sp.name + ")";
        const TodRegelwerk regel = sp.nr == kRegelwerkJediOutcast ? TodRegelwerk::JediOutcast
                                   : sp.nr == kRegelwerkMovieDuels ? TodRegelwerk::MovieDuels : TodRegelwerk::OpenJK;
        // Eine Waffe in der Hand: TossClientItems -> Drop_Item (zwei Zuege Q_flrand),
        // dann PM_PickAnim (Q_irand(1, 25), bis die Figur sie hat), dann
        // EV_DEATH (Q_irand(1, 3)) -> "*deathN.wav" auf CHAN_VOICE.
        {
            Welt w;
            w.karte = karte;
            w.skript = affect4("\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_BLASTER\" );\r\n" + totBei500);
            w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
            w.spur = bodenUeberall;
            w.boden = true;
            w.gegenstaende = true;
            Lauf l;
            fahre(w, sp.nr, l);
            const Actor* f = l.szene.find("held");
            const double tot = f != nullptr ? f->getoetetMs : -1.0;
            EngineZufall z(todesSaat(f != nullptr ? f->kartenEntity : -1, tot));
            const float r1 = z.flrand(-1.0F, 1.0F);
            const float r2 = z.flrand(-1.0F, 1.0F);
            while (z.irand(1, 25) != 12) {}
            const std::string klang = "*death" + std::to_string(z.irand(1, 3)) + ".wav";
            const TodesFolge* tf = folgeVon(l, "held");
            pruefe(tf != nullptr && !tf->klaenge.empty() && tf->klaenge.front().datei == klang &&
                       tf->klaenge.front().kanal == TodKlang::Kanal::Voice,
                   "Waffe in der Hand: EV_DEATH nach Drop_Item und PM_PickAnim - " + klang + r + " - " +
                       (tf != nullptr && !tf->klaenge.empty() ? tf->klaenge.front().datei : std::string("keiner")));
            const ActorState nach = f != nullptr ? f->at(tot + 50.0) : ActorState{};
            pruefe(nach.waffe == "WP_NONE", "die Waffe ist aus der Hand (s.weapon = WP_NONE)" + r + " - " + nach.waffe);
            const TodesAbwurf* ab = abwurfVon(l, "held");
            // Die Bahn nach EvaluateTrajectory (TR_GRAVITY, g_gravity 800) bis
            // die Unterkante (mins z -2) den Boden z = 0 erreicht: G_BounceItem
            // mit physicsBounce 0 -> G_SetOrigin(trace.endpos).
            const float vz = 200.0F + r1 * 50.0F;
            double ruhe = -1.0;
            float xRuhe = 0.0F;
            float zVor = 24.0F;
            float xVor = 0.0F;
            for (double t = tot + 50.0; t < tot + 3000.0; t += 50.0) {
                const auto dt = static_cast<float>((t - tot) * 0.001);
                const float zt = 24.0F + vz * dt - 400.0F * dt * dt;
                const float xt = 150.0F * dt;
                if (zt < 2.0F) {
                    const float frac = (zVor - 2.0F) / (zVor - zt);
                    xRuhe = xVor + (xt - xVor) * frac;
                    ruhe = t;
                    break;
                }
                zVor = zt;
                xVor = xt;
            }
            NavVec o{};
            NavVec wi{};
            const bool liegt = ab != nullptr && ab->art == TodesAbwurf::Art::Waffe && ab->bahn.bei(ab->bahn.ruheMs + 1.0, o, wi);
            pruefe(liegt && std::fabs(ab->bahn.ruheMs - ruhe) < 0.5 && std::fabs(o[2] - 2.0F) < 0.01F && std::fabs(o[0] - xRuhe) < 0.05F,
                   "Drop_Item + G_RunItem: liegt ab " + std::to_string(static_cast<int>(ruhe)) + " ms bei x " + std::to_string(xRuhe) + ", z 2" + r +
                       " - " + std::to_string(ab != nullptr ? static_cast<int>(ab->bahn.ruheMs) : -1) + " ms, " + std::to_string(o[0]) + " / " +
                       std::to_string(o[2]));
            // LaunchItem: (0, r2 * 180, 90); beim Liegenbleiben OJ/MD ("droppedSaber =
            // qtrue" fuer jeden Gegenstand) PITCH 90, ROLL 0, pitch_roll_for_slope mit
            // keepPitch auf dem flachen Boden -> (0, Gier, +-90); JK2 laesst sie.
            const float gier = r2 * 180.0F;
            const float roll = std::sin(gier * 3.14159265F / 180.0F) < 0.0F ? -90.0F : 90.0F;
            const bool winkelOk = regel == TodRegelwerk::JediOutcast
                                      ? (nah(wi[0], 0.0F) && nah(wi[1], gier) && nah(wi[2], 90.0F))
                                      : (nah(wi[0], 0.0F, 0.01F) && nah(wi[1], gier) && nah(wi[2], roll, 0.01F));
            pruefe(liegt && winkelOk, "liegende Waffe: Winkel" + r + " - " + std::to_string(wi[0]) + " " + std::to_string(wi[1]) + " " +
                                          std::to_string(wi[2]) + " (Gier " + std::to_string(gier) + ")");
            const bool klangOk = regel == TodRegelwerk::JediOutcast ? (ab != nullptr && ab->bahn.klaenge.empty())
                                                                    : (ab != nullptr && ab->bahn.klaenge.size() == 1U &&
                                                                       ab->bahn.klaenge[0].datei.rfind("sound/weapons/saber/bounce", 0) == 0);
            pruefe(klangOk, "G_BounceItem: OJ/MD WP_SaberFallSound (bounceN), JK2 still" + r);
        }
        // Movie Duels laesst auf md_ga_jedi nichts fallen (TossClientItems
        // "reduce memory use"): die Waffe verschwindet nur.
        {
            Welt w;
            w.karte = karte;
            w.kartenPfad = "maps/md_ga_jedi.bsp";
            w.skript = affect4("\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_BLASTER\" );\r\n" + totBei500);
            w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
            w.gegenstaende = true;
            Lauf l;
            fahre(w, sp.nr, l);
            const bool faellt = abwurfVon(l, "held") != nullptr;
            const Actor* f = l.szene.find("held");
            const std::string danach = f != nullptr ? f->at(f->getoetetMs + 50.0).waffe : std::string();
            pruefe(faellt == (regel != TodRegelwerk::MovieDuels) && danach == "WP_NONE",
                   std::string("md_ga_jedi: ") + (regel == TodRegelwerk::MovieDuels ? "nichts faellt, die Waffe verschwindet" : "die Waffe faellt") + r);
        }
        // DeathFX: CLASS_R2D2 - env/med_explode 10 unter dem Ursprung, mark2_explo
        // nach dem Todesklang.
        {
            Welt w;
            w.karte = karte;
            w.karte.replace(w.karte.find("eigen_typ"), 9, "eigen_droide");
            w.skript = affect4(totBei500);
            w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
            Lauf l;
            fahre(w, sp.nr, l);
            const TodesFolge* tf = folgeVon(l, "held");
            const bool fx = tf != nullptr && tf->effekte.size() == 1U && tf->effekte[0].datei == "env/med_explode" &&
                            nah(tf->effekte[0].ort[2], 14.0F);
            const bool klang = tf != nullptr && tf->klaenge.size() == 2U && tf->klaenge[0].datei.rfind("*death", 0) == 0 &&
                               tf->klaenge[1].datei == "sound/chars/mark2/misc/mark2_explo";
            pruefe(fx && klang, "DeathFX R2D2: env/med_explode bei z 14, *deathN dann mark2_explo" + r);
        }
        // Das Schwert in der Hand: Q_irand(0, 1) entscheidet. 1: WP_SaberLose ->
        // WP_SaberDrop (soundOff am Schwert, faellt von der Hand, verschwindet,
        // sobald es liegt); 0: es bleibt in der Hand, die Klinge geht aus (soundOff).
        bool gesehen[2] = {false, false};
        for (int wann = 500; wann < 900 && !(gesehen[0] && gesehen[1]); wann += 50) {
            Welt w;
            w.karte = karte;
            w.skript = affect4("\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\r\n"
                               "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", /*@BOOL_TYPES*/ \"true\" );\r\n"
                               "\twait ( " + std::to_string(wann) + ".000 );\r\n\tkill ( \"held\" );\r\n");
            w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
            w.spur = bodenUeberall;
            w.boden = true;
            w.hand = true;
            Lauf l;
            fahre(w, sp.nr, l);
            const Actor* f = l.szene.find("held");
            if (f == nullptr) { continue; }
            EngineZufall z(todesSaat(f->kartenEntity, f->getoetetMs));
            const int wurf = z.irand(0, 1);
            if (gesehen[wurf]) { continue; }
            gesehen[wurf] = true;
            const TodesFolge* tf = folgeVon(l, "held");
            const TodesAbwurf* ab = abwurfVon(l, "held");
            const std::string aus = regel == TodRegelwerk::JediOutcast ? "sound/weapons/saber/enemy_saber_off.wav" : "sound/eigen/aus.wav";
            const bool ausKlang = tf != nullptr && !tf->klaenge.empty() && tf->klaenge.front().datei == aus;
            if (wurf == 1) {
                // vom Ort der Hand (z 64) faellt es (TR_GRAVITY); der Kasten 3 trifft
                // den Boden im ersten Bild mit z < 3 - G_BounceMissile (EF_BOUNCE_HALF)
                // wirft es mit der halben gespiegelten Geschwindigkeit zurueck
                // (WP_SaberFallSound), bis trDelta[2] < 40; dann liegt es und ist
                // fort, weil sein Besitzer tot ist.
                double erster = -1.0;
                for (double t = f->getoetetMs + 50.0; t < f->getoetetMs + 3000.0; t += 50.0) {
                    const auto dt = static_cast<float>((t - f->getoetetMs) * 0.001);
                    if (64.0F - 400.0F * dt * dt < 3.0F) {
                        erster = t;
                        break;
                    }
                }
                // Jedi Outcast kennt keine .sab - der Griff ist der des Spiels
                const std::string griff = regel == TodRegelwerk::JediOutcast ? "models/weapons2/saber_reborn/saber_w.glm" : "models/eigen/griff.glm";
                // fort: im Bild, in dem es liegt (Jedi Outcast: erst nach
                // level.time > s.time + 5000, s.time = Zeit von WP_SaberDrop)
                const double fort = regel == TodRegelwerk::JediOutcast ? f->getoetetMs + 5050.0 : ab != nullptr ? ab->bahn.ruheMs : -1.0;
                const bool ok = ab != nullptr && ab->art == TodesAbwurf::Art::Schwert && ab->waffe == griff &&
                                !ab->bahn.klaenge.empty() && std::fabs(ab->bahn.klaenge.front().ms - erster) < 0.5 && ab->bahn.klaenge.size() >= 2U &&
                                ab->bahn.ruheMs > erster && std::fabs(ab->bahn.wegMs - fort) < 0.5 &&
                                ab->bahn.klaenge.back().ms == ab->bahn.ruheMs;
                pruefe(ok && f->at(f->getoetetMs + 50.0).waffe == "WP_NONE",
                       "Schwert faellt (Q_irand 1): aus der Hand, erster Aufprall " + std::to_string(static_cast<int>(erster)) +
                           " ms, springt, liegt und ist fort" + r + " - " +
                           std::to_string(ab != nullptr && !ab->bahn.klaenge.empty() ? static_cast<int>(ab->bahn.klaenge.front().ms) : -1) + ", " +
                           std::to_string(ab != nullptr ? ab->bahn.klaenge.size() : 0U) + " Aufpraelle, liegt " +
                           std::to_string(ab != nullptr ? static_cast<int>(ab->bahn.ruheMs) : -1) + " / fort " +
                           std::to_string(ab != nullptr ? static_cast<int>(ab->bahn.wegMs) : -1));
                pruefe(ausKlang, "WP_SaberDrop: der Ausschaltklang zuerst (" + aus + ")" + r);
            } else {
                pruefe(ab == nullptr && ausKlang && f->at(f->getoetetMs + 50.0).waffe == "WP_SABER",
                       "Schwert bleibt (Q_irand 0): in der Hand, Klinge aus mit " + aus + r);
            }
        }
        pruefe(gesehen[0] && gesehen[1], "beide Wuerfe des Schwerts gesehen" + r);
        // Pruefung 4a: WP_SaberThrow mit totem Besitzer - nach WP_SaberDrop
        // (SES_RETURNING) bleibt das Schwert stehen, solange es hoechstens 16 von
        // handRPoint ist (WP_SaberCatch faengt nicht, return): bleibt die Hand,
        // haengt es (cgame zeichnet die Bahn TR_GRAVITY weiter).
        // Mit ausgeschalteter Klinge wirft WP_SaberLose es nur (WP_SaberLaunch,
        // TR_LINEAR ohne Tempo); im naechsten Denkbild zielt WP_RunSaber es
        // (saberEntityDist), dann laesst WP_SaberThrow es fallen (WP_SaberDrop,
        // soundOff). Gesucht ist je Fall ein Tod mit Q_irand(0, 1) == 1.
        for (int fall = 0; fall < 3; ++fall) {
            bool fertig = false;
            for (int wann = 500; wann < 900 && !fertig; wann += 50) {
                Welt w;
                w.karte = karte;
                w.skript = affect4(std::string("\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\r\n") +
                                   "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", /*@BOOL_TYPES*/ \"" + (fall == 0 ? "true" : "false") +
                                   "\" );\r\n\twait ( " + std::to_string(wann) + ".000 );\r\n\tkill ( \"held\" );\r\n");
                w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
                w.spur = bodenUeberall;
                w.boden = true;
                w.hand = true;
                w.handWandert = fall != 0;
                w.saberWurf = fall == 2 ? 2 : 0;
                Lauf l;
                fahre(w, sp.nr, l);
                const Actor* f = l.szene.find("held");
                if (f == nullptr) { continue; }
                EngineZufall z(todesSaat(f->kartenEntity, f->getoetetMs));
                if (z.irand(0, 1) != 1) { continue; }
                fertig = true;
                const TodesAbwurf* ab = abwurfVon(l, "held");
                const double tod = f->getoetetMs;
                if (fall == 0) {
                    const bool ok = ab != nullptr && ab->bahn.punkte.size() == 1U && ab->bahn.ruheMs < 0.0 && ab->bahn.wegMs < 0.0 &&
                                    ab->bahn.klaenge.empty() && ab->bahn.punkte[0].pos.art == AbwurfTrajektorie::Art::Schwer;
                    pruefe(ok, "Schwert haengt an der stehenden Hand (SES_RETURNING, <= 16): keine neue Bahn, nie fort" + r);
                    continue;
                }
                // Jedi Outcast: FORCE_LEVEL_1 (WP_SaberLose); sonst die .npc
                const int stufe = regel == TodRegelwerk::JediOutcast ? 1 : w.saberWurf;
                const float weite = stufe == 0 ? 0.0F : stufe == 1 ? 256.0F : 400.0F;
                const double t1 = tod + 50.0;
                // die Hand in t1: (20, 0, 64); das Auge ohne augenPunkt der Ursprung (0 0 24)
                const NavVec heim = weite < 100.0F ? NavVec{20.0F, 0.0F, 64.0F} : NavVec{0.0F, 0.0F, 24.0F};
                const float nick = -8.0F * (std::numbers::pi_v<float> / 180.0F);   // fwdangles[PITCH] - 8, Gier 0
                const NavVec ziel{heim[0] + weite * std::cos(nick), 0.0F, heim[2] - weite * std::sin(nick)};
                NavVec d{ziel[0], ziel[1], ziel[2] - 64.0F};
                const float dist = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                const float tempo = dist < 50.0F ? dist * 2.0F + 30.0F : 500.0F;
                for (float& x : d) { x = x / dist * tempo; }
                const float dreh = stufe == 2 ? 800.0F : 600.0F;
                bool ok = ab != nullptr && ab->start.klingeAus && ab->bahn.punkte.size() >= 2U &&
                          ab->bahn.punkte[0].pos.art == AbwurfTrajektorie::Art::Linear && ab->bahn.punkte[0].apos.delta[1] == dreh;
                std::string ist = "-";
                if (ok) {
                    const AbwurfBahn::Punkt& p1 = ab->bahn.punkte[1];
                    ist = std::to_string(p1.ms - tod) + " ms, Tempo " + std::to_string(p1.pos.delta[0]) + " " + std::to_string(p1.pos.delta[2]);
                    ok = p1.ms == t1 && p1.pos.art == AbwurfTrajektorie::Art::Schwer && p1.pos.zeit == t1 &&
                         std::fabs(p1.pos.delta[0] - d[0]) < 0.05F && std::fabs(p1.pos.delta[1]) < 0.05F && std::fabs(p1.pos.delta[2] - d[2]) < 0.05F &&
                         p1.apos.delta[1] == dreh && !ab->bahn.klaenge.empty() && ab->bahn.klaenge.front().ms == t1 &&
                         ab->bahn.klaenge.front().datei == ab->start.ausKlang && !ab->start.ausKlang.empty() && ab->bahn.wegMs > t1;
                }
                pruefe(ok, "Klinge aus, FP_SABERTHROW " + std::to_string(stufe) + ": WP_SaberLaunch, nach 50 ms WP_RunSaber (Tempo " +
                               std::to_string(d[0]) + " " + std::to_string(d[2]) + ") und WP_SaberDrop mit soundOff - ist " + ist + r);
            }
            pruefe(fertig, "Fall " + std::to_string(fall) + " des Schwerts gesehen" + r);
        }
    }
}

// Eine Stufe: Boden z = 0 fuer x < 0, z = 8 ab x = 0 (ein Kasten trifft die
// hoehere, sobald sein Grundriss x >= 0 beruehrt).
NavSpur stufenBoden(const NavVec& s, const NavVec& mins, const NavVec& maxs, const NavVec& e, int, std::uint32_t) {
    NavSpur t;
    t.endpos = e;
    const float h = (std::max(s[0], e[0]) + maxs[0] >= 0.0F) ? 8.0F : 0.0F;
    const float unten = s[2] + mins[2];
    if (unten < h) {
        t.startsolid = true;
        t.fraction = 0.0F;
        t.endpos = s;
        return t;
    }
    if (e[2] < s[2] && e[2] + mins[2] < h) {
        t.fraction = (unten - h) / (s[2] - e[2]);
        t.endpos = {s[0] + (e[0] - s[0]) * t.fraction, s[1] + (e[1] - s[1]) * t.fraction, h - mins[2]};
        t.normal = {0.0F, 0.0F, 1.0F};
        t.entity = kNavWeltEntity;
    }
    return t;
}

void hang() {
    std::printf("--- 4b. Beine am Hang ---\n");
    // Die Schritte 4-7 gegen die Tabelle der Engine (interval 4, ATST 10).
    struct Fall {
        const char* beine;
        float diff;
        bool atst;
        double neu;      // slopeRecalcTime vorher
        const char* soll;
    };
    const Fall faelle[] = {
        {"BOTH_STAND1", 3.9F, false, -1.0, ""},                    // unter interval: qfalse
        {"BOTH_STAND1", 4.0F, false, -1.0, "LEGS_S1_LUP1"},        // aus dem Stand erst Stufe 1
        {"BOTH_STAND1", -20.0F, false, -1.0, "LEGS_S1_RUP1"},
        {"BOTH_STAND2", 12.0F, false, -1.0, "LEGS_LEFTUP1"},       // STAND2: die allgemeine Familie
        {"BOTH_SABERDUAL_STANCE", 8.0F, false, -1.0, "LEGS_S6_LUP1"},
        {"BOTH_STAND6", 20.0F, false, -1.0, ""},                    // keine Familie
        {"BOTH_CROUCH1", 12.0F, false, -1.0, "LEGS_LEFTUP3"},      // kein Fall im Stand: gleich das Ziel
        {"LEGS_S1_LUP1", 12.0F, false, -1.0, "LEGS_S1_LUP2"},       // eine Stufe hinauf
        {"LEGS_S1_LUP1", 12.0F, false, 1000.0, "LEGS_S1_LUP1"},     // slopeRecalcTime noch nicht vorbei
        {"LEGS_S1_LUP3", -8.0F, false, -1.0, "LEGS_S1_LUP4"},       // zur rechten Seite ueber die Nummern: L3 -> L4
        {"LEGS_S1_LUP5", -8.0F, false, -1.0, "LEGS_S1_RUP1"},
        {"LEGS_S1_RUP3", 4.0F, false, -1.0, "LEGS_S1_RUP2"},
        {"LEGS_S1_RUP1", 4.0F, false, -1.0, "LEGS_S1_LUP5"},
        {"LEGS_S3_LUP2", 8.0F, false, -1.0, "LEGS_S3_LUP2"},        // am Ziel
        {"BOTH_STAND1", 9.9F, true, -1.0, ""},                      // ATST: interval 10
        {"BOTH_STAND1", 30.0F, true, -1.0, "LEGS_LEFTUP1"},
        {"BOTH_RUN1", 30.0F, true, -1.0, "LEGS_LEFTUP3"},           // ATST ohne Stand: gleich das Ziel
    };
    for (const Fall& f : faelle) {
        double neu = f.neu;
        const std::string ist = hangBeinSchritt(f.beine, f.diff, f.atst, 500.0, neu);
        pruefe(ist == f.soll, std::string("PM_AdjustStandAnimForSlope ") + f.beine + " diff " + std::to_string(f.diff) + (f.atst ? " (ATST)" : "") +
                                  ": \"" + f.soll + "\" - \"" + ist + "\"");
    }
    // In der Szene: eine Figur steht so auf der Stufe, dass der linke Fuss oben
    // (z 8) und der rechte unten (z 0) steht: diff 8 -> Ziel LUP2, erst LUP1,
    // eine Stufe je SLOPE_RECALC_INT (100 ms, "slopeRecalcTime < level.time").
    for (const Spiel& sp : kSpiele) {
        const std::string r = std::string(" (") + sp.name + ")";
        Welt w;
        w.karte = "{\n\"classname\" \"worldspawn\"\n}\n"
                  "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"held\"\n\"NPC_type\" \"eigen_typ\"\n\"origin\" \"-4 0 32\"\n\"angle\" \"0\"\n}\n";
        w.skript = affect4("");
        w.anims = {"BOTH_STAND1", "LEGS_S1_LUP1", "LEGS_S1_LUP2"};
        w.spur = stufenBoden;
        w.fuesse = true;
        Lauf l;
        fahre(w, sp.nr, l);
        const Actor* a = l.szene.find("held");
        if (a == nullptr || a->hangBeine.size() < 2U) {
            pruefe(false, "Hang: Beinanimationen gerechnet" + r + " - " + std::to_string(a != nullptr ? a->hangBeine.size() : 0U));
            continue;
        }
        const double t0 = a->hangBeine[0].first;
        const double t1 = a->hangBeine[1].first;
        const ActorState st = a->at(t1 + 10.0);
        pruefe(a->hangBeine[0].second == "LEGS_S1_LUP1" && a->hangBeine[1].second == "LEGS_S1_LUP2" && std::fabs(t1 - t0 - 150.0) < 0.5 &&
                   st.animation == "LEGS_S1_LUP2" && st.torsoAnimation == "BOTH_STAND1",
               "Hang: LUP1 bei " + std::to_string(static_cast<int>(t0)) + ", LUP2 150 ms spaeter, Oberkoerper BOTH_STAND1" + r + " - " +
                   a->hangBeine[0].second + "/" + a->hangBeine[1].second + " nach " + std::to_string(static_cast<int>(t1 - t0)) + " ms, " +
                   st.animation + " / " + st.torsoAnimation);
        // Auf ebenem Boden: keine Hanganimation.
        Welt eben = w;
        eben.spur = bodenUeberall;
        eben.karte.replace(eben.karte.find("-4 0 32"), 7, "-4 0 24");
        Lauf l2;
        fahre(eben, sp.nr, l2);
        const Actor* a2 = l2.szene.find("held");
        pruefe(a2 != nullptr && a2->hangBeine.empty(), "ebener Boden: keine Hanganimation" + r);
    }
}

// --- 5. SET_ANIM einer Animation, die das Modell nicht hat --------------------
void animOhneBilder() {
    std::printf("--- 5. SET_ANIM ohne Bilder ---\n");
    for (const Spiel& sp : kSpiele) {
        const std::string r = std::string(" (") + sp.name + ")";
        Welt w;
        w.karte = kKarte4;
        w.anims = {"BOTH_STAND1", "BOTH_DEATH12"};
        w.skript = affect4("\tset ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", \"BOTH_DEADFORWARD1\" );\r\n"
                           "\tset ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_BOTH\", -1 );\r\n"
                           "\twait ( 500.000 );\r\n"
                           "\tset ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", \"BOTH_DEATH12\" );\r\n"
                           "\tset ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_BOTH\", -1 );\r\n");
        Lauf l;
        fahre(w, sp.nr, l);
        const std::string a0 = animBei(l, 300.0);
        const std::string a1 = animBei(l, 800.0);
        pruefe(a0 == "BOTH_STAND1" && a1 == "BOTH_DEATH12",
               "PM_HasAnimation: BOTH_DEADFORWARD1 (nicht im Modell) setzt nichts, BOTH_DEATH12 schon" + r + " - " + a0 + " / " + a1);
    }
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
        // Pruefung 4a: PM_TryRoll - aus vollem Lauf (BOTH_RUN1) geduckt rollt die
        // Figur (G_StartRoll BOTH_ROLL_F, HOLDLESS 999 ms; ClientThink: ucmd 127
        // vorwaerts, ps.speed 400, mal pm_duckScale), danach BOTH_CROUCH1WALK.
        // Mit Schwert nur ab RANK_LT_JG (oder RANK_CREWMAN) - Movie Duels fragt
        // weder Waffe noch Rang.
        for (int fall = 0; fall < 3; ++fall) {
            Welt roll = w;
            roll.anims.insert("BOTH_ROLL_F");
            roll.rang = fall == 2 ? "ltjg" : "";
            roll.skript = affect4(setze("SET_WEAPON", fall == 0 ? "WP_NONE" : "WP_SABER") + setze("SET_NAVGOAL", "ziel") +
                                  "\twait ( 400.000 );\r\n" + setze("SET_CROUCHED", "true"));
            Lauf l4;
            fahre(roll, sp.nr, l4);
            double ab = -1.0;
            double bis = -1.0;
            for (double t = 400.0; t < 3000.0; t += 50.0) {
                const bool rollt = animBei(l4, t) == "BOTH_ROLL_F";
                if (rollt && ab < 0.0) { ab = t; }
                if (!rollt && ab >= 0.0 && bis < 0.0) { bis = t; }
            }
            // Jedi Outcast prueft den Rang fuer jeden NPC (auch ohne Waffe)
            const bool soll = sp.nr == kRegelwerkMovieDuels || fall == 2 || (fall == 0 && sp.nr != kRegelwerkJediOutcast);
            const std::string nachher = bis > 0.0 ? animBei(l4, bis + 100.0) : std::string("-");
            const float v = ab > 0.0 ? tempoXY(l4, ab + 200.0, ab + 800.0) : 0.0F;
            const bool ok = soll ? (ab > 400.0 && ab <= 600.0 && bis - ab >= 950.0 && bis - ab <= 1050.0 &&
                                    nachher == "BOTH_CROUCH1WALK" && std::fabs(v - 200.0F) < 2.0F)
                                 : ab < 0.0;
            const char* was[] = {"WP_NONE", "WP_SABER ohne Rang", "WP_SABER ltjg"};
            pruefe(ok, std::string("PM_TryRoll aus vollem Lauf, ") + was[fall] + (soll ? ": BOTH_ROLL_F" : ": keine Rolle") + " - ab " +
                           std::to_string(static_cast<int>(ab)) + " bis " + std::to_string(static_cast<int>(bis)) + ", " + nachher +
                           ", Tempo " + std::to_string(v) + r);
        }
    }
}

// --- 6. Blinzeln und Leerlaufmimik (Pruefung 4a) ------------------------------
// CG_G2PlayerHeadAnims/CG_G2SetHeadBlink (cg_players.cpp:5046-5232 OJ), Q3_Face.
void gesicht() {
    std::printf("--- 6. Blinzeln und Leerlaufmimik ---\n");
    GesichtsEingabe e;
    e.saat = 4711U;
    e.abMs = 0.0;
    e.bisMs = 60000.0;
    e.kamera = true;
    // die ersten Zuege: facial_blink = cg.time + Q_flrand(4000, 8000), facial_timer
    // = cg.time + Q_flrand(6000, 10000)
    EngineZufall z(e.saat);
    const double blink = static_cast<double>(z.flrand(4000.0F, 8000.0F));
    const double aux = static_cast<double>(z.flrand(6000.0F, 10000.0F));
    const GesichtsPlan p = gesichtPlanen(e);
    // "facial_blink < cg.time": im ersten Bild danach zu, 300 ms spaeter
    // ("-(facial_blink) < cg.time") wieder auf; in einer Kamerafahrt nie nur links
    bool ok = p.lider.size() >= 4U && p.lider[0].ms == std::floor(blink) + 1.0 && p.lider[0].winkel == -38.0F &&
              p.lider[1].ms == p.lider[0].ms + 301.0 && p.lider[1].winkel == 0.0F;
    double abstand = 1.0e9;
    bool links = false;
    for (std::size_t i = 0; i + 1 < p.lider.size(); i += 2) {
        if (i + 2 < p.lider.size()) { abstand = std::min(abstand, p.lider[i + 2].ms - p.lider[i + 1].ms); }
        links = links || p.lider[i].nurLinks;
    }
    pruefe(ok && abstand >= 4000.0 && abstand <= 8001.0 && !links,
           "Lidschlag: erstes Schliessen " + std::to_string(p.lider.empty() ? -1.0 : p.lider[0].ms) + " ms (Soll " +
               std::to_string(std::floor(blink) + 1.0) + "), 300 ms zu, Abstand 4000..8000, in der Kamerafahrt beide Augen");
    // facial_timer: FACE_ALERT + Q_irand(0, 2) fuer 2000 ms
    const bool mimik = !p.mimik.empty() && std::fabs(p.mimik[0].first - (std::floor(aux) + 1.0)) < 0.5 &&
                       (p.mimik[0].second == "FACE_ALERT" || p.mimik[0].second == "FACE_SMILE" || p.mimik[0].second == "FACE_FROWN");
    pruefe(mimik, "Leerlaufmimik: erste nach facial_timer (" + std::to_string(std::floor(aux) + 1.0) + " ms) - " +
                      (p.mimik.empty() ? std::string("-") : p.mimik[0].second + " bei " + std::to_string(p.mimik[0].first)));
    // Sprechen schiebt die Mimik auf: facial_timer = cg.time + Q_flrand(2000, 7000)
    GesichtsEingabe sp = e;
    sp.sprechen = {{1000.0, 20000.0}};
    const GesichtsPlan ps = gesichtPlanen(sp);
    bool still = true;
    for (const auto& [t, n] : ps.mimik) { still = still && !(t >= 1000.0 && t < 22000.0); }
    pruefe(still && !ps.mimik.empty(), "Sprechen 1000..20000: keine Mimik bis 2000 ms nach dem Ende, danach wieder");
    // SET_FACEEYESCLOSED: facial_blink = 1 -> im naechsten Bild zu, bleibt zu
    GesichtsEingabe zu = e;
    zu.befehle.push_back({GesichtsBefehl::Art::AugenZu, 1000.0, 0.0F});
    const GesichtsPlan pz = gesichtPlanen(zu);
    float l = 0.0F;
    float r = 0.0F;
    std::string m;
    double ende = 0.0;
    gesichtBei(pz, 50000.0, l, r, m, ende);
    pruefe(!pz.lider.empty() && pz.lider.back().ms == 1000.0 && l == -38.0F && r == -38.0F,
           "SET_FACEEYESCLOSED (1000 ms): im selben Bild zu und bleibt zu");
    // Tot: FACE_DEAD, kein Lidschlag mehr
    GesichtsEingabe tot = e;
    tot.todMs = 3000.0;
    const GesichtsPlan pt = gesichtPlanen(tot);
    gesichtBei(pt, 40000.0, l, r, m, ende);
    pruefe(m == "FACE_DEAD" && (pt.lider.empty() || pt.lider.back().ms < 3000.0), "tot: FACE_DEAD, keine Lider mehr - " + m);
    // ohne Kamerafahrt zwinkert die Figur in 5 % der Lidschlaege (nur links)
    GesichtsEingabe frei = e;
    frei.kamera = false;
    frei.bisMs = 3600000.0;
    const GesichtsPlan pf = gesichtPlanen(frei);
    int schl = 0;
    int zw = 0;
    for (const GesichtsPlan::Lid& lid : pf.lider) {
        if (lid.winkel != 0.0F) {
            ++schl;
            if (lid.nurLinks) { ++zw; }
        }
    }
    const double anteil = schl > 0 ? static_cast<double>(zw) / schl : 0.0;
    pruefe(schl > 400 && anteil > 0.02 && anteil < 0.08, "ohne Kamera: Zwinkern " + std::to_string(zw) + " von " + std::to_string(schl));
}

}  // namespace

int main() {
    rumpfwinkel();
    tod();
    markenspeicher();
    hocke();
    hang();
    animOhneBilder();
    gesicht();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
