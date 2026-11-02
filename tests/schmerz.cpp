// rc608 Teil 6: G_Damage auf Figuren, Portale, Waffenhaltung, Leiche am Hang, Holster, Gamma - gegen die Formeln der Engine.
#include "bhed/areaportal.h"
#include "bhed/bsp.h"
#include "bhed/gpushader.h"
#include "bhed/holster.h"
#include "bhed/kartenpruefung.h"
#include "bhed/navlauf.h"
#include "bhed/regelwerk.h"
#include "bhed/scene.h"
#include "bhed/schmerz.h"
#include "bhed/shaderscript.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using namespace bhed;

int g_fehler = 0;

void erwarte(const char* name, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", name);
    if (!ok) { ++g_fehler; }
}

bool nah(float a, float b, float d = 0.01F) { return std::fabs(a - b) <= d; }

// Ein ebener Boden bei z = 0.
NavWelt ebene() {
    NavWelt w;
    w.boden = [](float, float, float, float zUnten, float& b) {
        b = 0.0F;
        return b >= zUnten;
    };
    w.spur = [](const NavVec& a, const NavVec& mi, const NavVec&, const NavVec& e, int, std::uint32_t) {
        NavSpur s;
        s.endpos = e;
        const float za = a[2] + mi[2];
        const float ze = e[2] + mi[2];
        if (za < 0.0F) {
            s.fraction = 0.0F;
            s.startsolid = true;
            s.endpos = a;
            s.normal = NavVec{0.0F, 0.0F, 1.0F};
            return s;
        }
        if (ze < 0.0F) {
            const float f = za / (za - ze);
            s.fraction = f;
            s.endpos = NavVec{a[0] + (e[0] - a[0]) * f, a[1] + (e[1] - a[1]) * f, a[2] + (e[2] - a[2]) * f};
            s.normal = NavVec{0.0F, 0.0F, 1.0F};
            s.entity = kNavWeltEntity;
            return s;
        }
        s.fraction = 1.0F;
        return s;
    };
    return w;
}

void rueckstoss() {
    // knockback = damage, hoechstens 200; NO_KNOCKBACK, MOD_CRUSH 0.
    erwarte("knockback = damage", nah(knockbackWert(30, 0, false), 30.0F));
    erwarte("knockback hoechstens 200", nah(knockbackWert(500, 0, false), 200.0F));
    erwarte("DAMAGE_EXTRA_KNOCKBACK verdoppelt", nah(knockbackWert(30, 0x40, false), 60.0F));
    erwarte("DAMAGE_NO_KNOCKBACK: die Bolzen der Geschuetze", knockbackWert(30, kSchadenOhneRueckstoss, false) == 0.0F);
    erwarte("MOD_CRUSH: kein Stoss", knockbackWert(30, 0, true) == 0.0F);
    // G_ApplyKnockback: knockback*2 (hoechstens 120), g_knockback 1000, Masse 200, Schwerkraft > 0.
    const float flach[3] = {1.0F, 0.0F, 0.0F};
    StossErgebnis st;
    erwarte("Stoss 30: moeglich", stossTempo(30.0F, flach, "CLASS_REBORN", TodRegelwerk::OpenJK, 0.0F, 800.0F, st));
    erwarte("Stoss 30: x = 1000 * 60 / 200 * 0.8 = 240", nah(st.tempo[0], 240.0F));
    erwarte("Stoss 30: z = 0 + 20", nah(st.tempo[2], 20.0F));
    erwarte("Stoss 30: pm_time = 120", st.pmZeit == 120);
    StossErgebnis gross;
    erwarte("Stoss 200: hoechstens 120 -> x = 480", stossTempo(200.0F, flach, "CLASS_REBORN", TodRegelwerk::MovieDuels, 0.0F, 800.0F, gross) && nah(gross.tempo[0], 480.0F));
    erwarte("Stoss 200: pm_time hoechstens 200", gross.pmZeit == 200);
    const float runter[3] = {0.0F, 0.0F, -1.0F};
    StossErgebnis r;
    (void)stossTempo(50.0F, runter, "CLASS_REBORN", TodRegelwerk::OpenJK, 0.0F, 800.0F, r);
    // newDir[2] += (0 - newDir[2]) * 1.2 = -1 + 1.2 = 0.2; kvel[2] = 0.2 * 1000 * 100 / 300 + 20
    erwarte("nach unten wird nach oben: z = 86,67", nah(r.tempo[2], 0.2F * 1000.0F * 100.0F / 300.0F + 20.0F, 0.05F));
    StossErgebnis atst;
    erwarte("CLASS_ATST: much too large", !stossTempo(50.0F, flach, "CLASS_ATST", TodRegelwerk::MovieDuels, 0.0F, 800.0F, atst));
    erwarte("Jedi Outcast kennt die Grenze nicht", stossTempo(50.0F, flach, "CLASS_ATST", TodRegelwerk::JediOutcast, 0.0F, 800.0F, atst));

    // planeStosslauf: Pmove mit leerem ucmd, ohne Reibung bis pm_time abgelaufen ist.
    const NavWelt w = ebene();
    LaufZustand z;
    z.ort = NavVec{0.0F, 0.0F, 24.0F};
    z.amBoden = true;
    LaufWerte lw;
    lw.halbBreite = 15.0F;
    lw.hoeheOben = 40.0F;
    lw.hockHoehe = 16.0F;
    lw.stufe = 18.0F;
    lw.schwerkraft = 800.0F;
    lw.weltSchwerkraft = 800.0F;
    LaufStoss s;
    s.ms = 1000.0;
    s.tempo = NavVec{240.0F, 0.0F, 20.0F};
    s.pmZeit = 120;
    const LaufPlan p = planeStosslauf(w, z, lw, {s}, 61000.0);
    erwarte("Stosslauf: Takte ab dem Treffer", !p.takte.empty() && nah(static_cast<float>(p.abMs), 1000.0F));
    const NavVec ende = p.takte.empty() ? NavVec{} : p.takte.back().ort;
    std::printf("  Stosslauf: Ende x %.2f nach %zu Takten\n", static_cast<double>(ende[0]), p.takte.size());
    erwarte("Stosslauf: weggestossen und zum Stehen gekommen", ende[0] > 10.0F && p.takte.size() < 60U && p.zustaende.back().amBoden);
    erwarte("Stosslauf: bleibt in der Spur y", nah(ende[1], 0.0F));
}

void schmerz() {
    EngineZufall zufall(7U);
    erwarte("G_PickPainAnim: Kopf BOTH_PAIN4", schmerzAnimNachOrt(kOrtKopf, zufall) == "BOTH_PAIN4");
    erwarte("G_PickPainAnim: rechter Fuss BOTH_PAIN12", schmerzAnimNachOrt(kOrtFussR, zufall) == "BOTH_PAIN12");
    erwarte("G_PickPainAnim: linker Fuss -1", schmerzAnimNachOrt(kOrtFussL, zufall).empty());
    erwarte("G_PickPainAnim: Brust rechts BOTH_PAIN3", schmerzAnimNachOrt(kOrtBrustR, zufall) == "BOTH_PAIN3");
    erwarte("G_PickPainAnim: Arm links BOTH_PAIN10", schmerzAnimNachOrt(kOrtArmL, zufall) == "BOTH_PAIN10");
    bool breite[20] = {};
    for (int i = 0; i < 200; ++i) {
        const std::string a = schmerzAnimNachOrt(kOrtBrust, zufall);
        breite[std::stoi(a.substr(9))] = true;
    }
    erwarte("G_PickPainAnim: Brust 6, 8, 17, 19", breite[6] && breite[8] && breite[17] && breite[19] && !breite[1]);
    erwarte("EV_PAIN: unter 25 *pain100", schmerzKlang(24) == "*pain100.wav");
    erwarte("EV_PAIN: ab 75 *pain25", schmerzKlang(75) == "*pain25.wav");
    erwarte("NPC_ChoosePainAnimation: ohne Feind immer", nah(npcSchmerzChance("CLASS_REBORN", true), 1.0F));
    erwarte("NPC_ChoosePainAnimation: Vader halb, nur Movie Duels", nah(npcSchmerzChance("CLASS_VADER", true), 0.5F) && nah(npcSchmerzChance("CLASS_VADER", false), 1.0F));
    erwarte("NPC_ChoosePainAnimation: Desann halb", nah(npcSchmerzChance("CLASS_DESANN", false), 0.5F));
}

void leiche() {
    float nick = 1.0F;
    float roll = 1.0F;
    leichenNeigung(NavVec{0.0F, 0.0F, 1.0F}, 45.0F, nick, roll);
    erwarte("pitch_roll_for_slope: flacher Boden", nah(nick, 0.0F) && nah(roll, 0.0F));
    // Hang nach +x um 30 Grad: Normale (sin30, 0, cos30), vectoangles Nick = -60, pitch = 30.
    const float n = 0.5F;
    const float h = std::sqrt(3.0F) * 0.5F;
    leichenNeigung(NavVec{n, 0.0F, h}, 0.0F, nick, roll);
    erwarte("Blick zum Hang hinab (Gier 0): dot 1, Nick 30, Roll 0", nah(nick, 30.0F, 0.05F) && nah(roll, 0.0F));
    leichenNeigung(NavVec{n, 0.0F, h}, 90.0F, nick, roll);
    // Gier 90: dot 0, ovr = (1, 0), mod 1
    erwarte("Quer zum Hang (Gier 90): Nick 0, Roll 30", nah(nick, 0.0F, 0.05F) && nah(roll, 30.0F, 0.05F));
    leichenNeigung(NavVec{n, 0.0F, h}, 270.0F, nick, roll);
    erwarte("Andere Seite (Gier 270): Roll -30", nah(nick, 0.0F, 0.05F) && nah(roll, -30.0F, 0.05F));
}

void portale() {
    AreaPortale p;
    p.anlegen(4);
    erwarte("Am Anfang jedes Gebiet fuer sich", !p.verbunden(0, 1) && p.verbunden(2, 2));
    erwarte("Gebiet -1 ist nie verbunden", !p.verbunden(-1, 0));
    p.setze(0, 1, true);
    erwarte("Tuer auf: verbunden", p.verbunden(0, 1) && p.verbunden(1, 0));
    p.setze(1, 2, true);
    erwarte("Flut: ueber zwei Tueren", p.verbunden(0, 2) && !p.verbunden(0, 3));
    p.setze(0, 1, true);
    p.setze(0, 1, false);
    erwarte("Zaehler: zwei Tueren am selben Paar", p.verbunden(0, 1));
    p.setze(0, 1, false);
    erwarte("Tuer zu: getrennt", !p.verbunden(0, 1) && p.verbunden(1, 2));
    // Eine kleine Karte: zwei Blaetter trennt eine Ebene x = 0.
    BspGeometry g;
    BspGeometry::Plane pl;
    pl.normal[0] = 1.0F;
    pl.dist = 0.0F;
    g.planes.push_back(pl);
    BspGeometry::Node knoten;
    knoten.plane = 0;
    knoten.children[0] = -1;   // Blatt 0: x >= 0
    knoten.children[1] = -2;   // Blatt 1: x < 0
    g.nodes.push_back(knoten);
    BspGeometry::Leaf a;
    a.cluster = 0;
    a.area = 0;
    a.firstSurface = 0;
    a.numSurfaces = 2;
    BspGeometry::Leaf b = a;
    b.area = 1;
    b.firstSurface = 1;
    b.numSurfaces = 2;
    g.leafs = {a, b};
    g.leafSurfaces = {0, 1, 2};
    g.surfaces.resize(3);
    const float punkt[3] = {5.0F, 0.0F, 0.0F};
    erwarte("Gebiet des Punkts", punktGebiet(g, punkt) == 0);
    const float mn[3] = {-1.0F, -1.0F, -1.0F};
    const float mx[3] = {1.0F, 1.0F, 1.0F};
    const AreaPaar paar = kastenGebiete(g, mn, mx);
    erwarte("SV_LinkEntity: der Kasten auf der Grenze: areanum 0, areanum2 1", paar.a == 0 && paar.b == 1);
    AreaPortale q;
    q.anlegen(2);
    std::vector<char> v = flaechenHinterPortalen(g, q, 0);
    erwarte("Zu: Flaeche 2 nur im anderen Gebiet ist verdeckt, Flaeche 1 in beiden nicht", v[0] == 0 && v[1] == 0 && v[2] == 1);
    q.setze(0, 1, true);
    v = flaechenHinterPortalen(g, q, 0);
    erwarte("Auf: nichts verdeckt", v[2] == 0);
    q.setze(0, 1, false);
    v = flaechenHinterPortalen(g, q, -1);
    erwarte("Auge im Festen: nichts verdeckt (CM_WriteAreaBits)", v[2] == 0);
    // portalEntities: Regeln je Klasse.
    MapData karte;
    BspGeometry::SubModel welt;
    g.models.push_back(welt);
    BspGeometry::SubModel t;
    t.mins[0] = -1.0F;
    t.maxs[0] = 1.0F;
    t.inhalt = kInhaltUndurchsichtig;
    for (int i = 0; i < 5; ++i) { g.models.push_back(t); }
    g.models[5].inhalt = 0U;
    const auto ent = [&karte](const char* klasse, const char* modell, const char* team, const char* sf) {
        MapEntity e;
        e.classname = klasse;
        e.keys.emplace_back("model", modell);
        if (team[0] != 0) { e.keys.emplace_back("team", team); }
        if (sf[0] != 0) { e.keys.emplace_back("spawnflags", sf); }
        karte.entities.push_back(e);
    };
    ent("func_door", "*1", "a", "");
    ent("func_door", "*2", "a", "");   // Mannschaft: nur der erste
    ent("func_wall", "*3", "", "1");   // START_OFF: nie
    ent("func_breakable", "*4", "", "");
    ent("func_door", "*5", "", "");    // nicht CONTENTS_OPAQUE
    const std::vector<PortalEntity> ents = portalEntities(karte, g, true, nullptr);
    erwarte("portalEntities: teammaster und Zerbrechliches", ents.size() == 2U && ents[0].modell == 1 && ents[1].modell == 4);
    erwarte("Jedi Outcast fragt nicht nach CONTENTS_OPAQUE", portalEntities(karte, g, false, nullptr).size() == 3U);
}

void waffenhaltung() {
    regelwerkAnwenden(true, 0);
    erwarte("Movie Duels: Gewehr steht in TORSO_WEAPONREST3", npcWaffenTorso("WP_BLASTER", "BOTH_STAND1", "CLASS_PLAYER", false, false, false, true) == "TORSO_WEAPONREST3");
    erwarte("Movie Duels: der Spieler laeuft mit den Beinen", npcWaffenTorso("WP_BLASTER", "BOTH_RUN1", "CLASS_PLAYER", false, false, false, true).empty());
    erwarte("Movie Duels: ein NPC nicht", !npcWaffenTorso("WP_BLASTER", "BOTH_RUN1", "CLASS_STORMTROOPER", false, false, false, false).empty());
    regelwerkAnwenden(false, 0);
    erwarte("OpenJK: Gewehr steht in TORSO_WEAPONIDLE3", npcWaffenTorso("WP_BLASTER", "BOTH_STAND1", "CLASS_PLAYER", false, false, false, true) == "TORSO_WEAPONIDLE3");
    erwarte("OpenJK: der Spieler geht mit den Beinen", npcWaffenTorso("WP_BLASTER", "BOTH_WALK1", "CLASS_PLAYER", false, true, false, true).empty());
    erwarte("OpenJK: ein NPC geht mit dem Oberkoerper", !npcWaffenTorso("WP_BLASTER", "BOTH_WALK1", "CLASS_STORMTROOPER", false, true, false, false).empty());
}

void holster() {
    HolsterLage l;
    erwarte("HOLSTER_LHIPF an *hip_fl", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "", "", 0, l) && l.bolzen == std::string("*hip_fl"));
    // angles 180 0 180 -> G2_Generate_Matrix(POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z): Nick 180, Gier 180, Roll 0: diag(1, -1, -1).
    erwarte("Griff nach der Quelle: diag(1 -1 -1)", nah(l.drehung.m[0][0], 1.0F) && nah(l.drehung.m[1][1], -1.0F) && nah(l.drehung.m[2][2], -1.0F));
    erwarte("HOLSTER_LHIPF: Versatz 0 -1 -10 in der vierten Spalte", nah(l.drehung.m[1][3], -1.0F) && nah(l.drehung.m[2][3], -10.0F));
    erwarte("Doppelschwert an der Huefte: 10 hoeher", saberHolsterLage("x", "CLASS_REBORN", "SABER_STAFF", "HOLSTER_LHIPF", "", 0, l) && nah(l.drehung.m[2][3], 10.0F));
    erwarte("HOLSTER_BACK: 0 -2 -4", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_BACK", "", 0, l) && nah(l.drehung.m[1][3], -2.0F) && nah(l.drehung.m[2][3], -4.0F));
    (void)saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "", "", 0, l);
    erwarte("HOLSTER_LHIP an *hip_l: 5 tiefer", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_LHIP", "", 0, l) && nah(l.drehung.m[2][3], -5.0F));
    erwarte("HOLSTER_BACK an *back", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_BACK", "", 0, l) && l.bolzen == std::string("*back"));
    // WP_SaberHolsterAvoidGuns: hat die Waffe die Huefte, nach vorn, 2 weg, 2 vor, 6 tiefer.
    erwarte("AvoidGuns: links belegt -> *hip_fl", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_LHIP", "", 0, l, true, false) && l.bolzen == std::string("*hip_fl") && nah(l.drehung.m[0][3], 2.0F) && nah(l.drehung.m[1][3], -2.0F) && nah(l.drehung.m[2][3], -6.0F));
    erwarte("AvoidGuns: rechts belegt -> *hip_fr", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_RHIP", "", 0, l, false, true) && l.bolzen == std::string("*hip_fr") && nah(l.drehung.m[0][3], -2.0F));
    // holster.cfg
    HolsterCfg cfg;
    leseHolsterCfg("// Kommentar\n{\n holsterType HLR_BLASTER_R\n boneIndex HOLSTER_RIGHTHIP\n posOffset \"1, 2, 3\"\n angOffset \"0, 90, 0\"\n}\n"
                   "{ holsterType HLR_BLASTER_L boneIndex HOLSTER_LEFTHIP }\n{ holsterType HLR_REPEATER boneIndex HOLSTER_UPPERBACK }\n{ holsterType HLR_PISTOL_R boneIndex disabled }\n", cfg);
    erwarte("holster.cfg: Knochen und Versatz", cfg.typ[7].knochen == 4 && nah(cfg.typ[7].pos[2], 3.0F) && nah(cfg.typ[7].ang[1], 90.0F) && cfg.typ[5].knochen == 0);
    WaffenPlaetze pl;
    waffenPlaetzeAendern(pl, cfg, {"WP_SABER", "WP_BLASTER"}, "WP_SABER", false);
    erwarte("Waffe: erst die rechte Huefte", pl[0] == "WP_BLASTER" && pl[1].empty());
    waffenPlaetzeAendern(pl, cfg, {"WP_SABER", "WP_BLASTER", "WP_THEFIRSTORDER"}, "WP_BLASTER", false);
    erwarte("Waffe: behaelt den Platz, auch in der Hand, die neue links", pl[0] == "WP_BLASTER" && pl[1] == "WP_THEFIRSTORDER");
    erwarte("Waffe in der Hand wird nicht gezeichnet", waffenImHolster(pl, cfg, "WP_BLASTER", false).size() == 1U);
    waffenPlaetzeAendern(pl, cfg, {"WP_SABER", "WP_BLASTER", "WP_THEFIRSTORDER", "WP_REPEATER"}, "WP_SABER", false);
    erwarte("Hoechstens zwei", pl[2].empty());
    WaffenPlaetze pl2;
    waffenPlaetzeAendern(pl2, cfg, {"WP_SABER", "WP_REPEATER"}, "WP_SABER", true);
    erwarte("Ruecken belegt: keine Waffe dorthin", pl2[2].empty());
    waffenPlaetzeAendern(pl2, cfg, {"WP_SABER", "WP_REPEATER"}, "WP_SABER", false);
    erwarte("Ruecken frei", pl2[2] == "WP_REPEATER");
    const std::vector<WaffeImHolster> wi = waffenImHolster(pl, cfg, "WP_SABER", false);
    erwarte("Bolzen der Huefte", !wi.empty() && wi[0].bolzen == std::string("rfemurYZ"));
    // posOffset 1 2 3: 1 * NEGATIVE_Y + 2 * NEGATIVE_Z + 3 * POSITIVE_X; ent.axis[2] = POSITIVE_X
    erwarte("Hls_Render: Versatz", !wi.empty() && nah(wi[0].lage.m[0][3], 3.0F) && nah(wi[0].lage.m[1][3], -1.0F) && nah(wi[0].lage.m[2][3], -2.0F));
}

void gamma() {
    // R_SetColorMappings (DLL): (int)(i/63*255 + 0.5); GL_LINEAR, ohne Halbtexel-Ausgleich.
    const auto bytes = [](float b) { return gpu::spielGammaWert(b / 255.0F) * 255.0F; };
    erwarte("13 -> 11", nah(bytes(13.0F), 11.04F, 0.05F));
    erwarte("Tabelle gerundet: i = 7 -> 28", nah(bytes(7.5F * 255.0F / 64.0F), 28.0F, 0.05F));
    erwarte("255 -> 255", nah(bytes(255.0F), 255.0F));
}

void sonne() {
    const std::string text = "textures/skies/s\n{\n\tq3map_sun 1 0.5 0.5 100 90 45\n\tskyParms textures/skies/s 512 -\n}\n"
                             "textures/x/y\n{\n\t{\n\t\tmap textures/x/y\n\t}\n}\n";
    ShaderMap karte;
    parseShaderScript(text, karte);
    const auto it = karte.find("textures/skies/s");
    const bool da = it != karte.end() && it->second.hatSonne;
    erwarte("q3map_sun gelesen", da);
    BspGeometry g;
    erwarte("Vorgabe aus R_LoadWorld", nah(g.sonne[0], 0.42857F, 0.001F) && nah(g.sonne[2], 0.85714F, 0.001F));
    BspShader sh;
    sh.name = "textures/skies/S";
    g.shaders.push_back(sh);
    BspSurface fl;
    fl.shader = 0;
    g.surfaces.push_back(fl);
    sonneAusShadern(g, karte);
    erwarte("tr.sunDirection aus Grad und Hoehe", nah(g.sonne[0], 0.0F, 0.001F) && nah(g.sonne[1], 0.7071F, 0.001F) && nah(g.sonne[2], 0.7071F, 0.001F));
}

}  // namespace

int main() {
    rueckstoss();
    schmerz();
    leiche();
    portale();
    waffenhaltung();
    holster();
    gamma();
    sonne();
    using bhed::kartenpruefung::brushModellImBild;
    erwarte("Untermodell: Tuer im Bild", brushModellImBild("func_door", true));
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
