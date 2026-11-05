// pruefmission.cpp - pruefung1: selbst gebaute Mini-Missionen fuer den
// Missions-Nachbau (Ablauf, Szene, Kamera, Mover), je ICARUS-Befehlsgruppe und
// Spielverhalten, fuer das OpenJK- und das Movie-Duels-Regelwerk.
//
// Grundsatz (shank): "Alles im Mission-Tab und in der Vorschau muss fuer neue,
// selbst gebaute Missionen genauso funktionieren wie fuer die Originale."
// Jede Mini-Mission ist deshalb eine Vorlage mit Platzhaltern (@FIG1@ ...)
// und laeuft zweimal:
//   A  mit Namen, wie sie in den Original-Missionen stehen (obi1, cam1b,
//      md_am/intro2_sith, kor2/roffs/boulder ...),
//   B  mit frischen Namen anderer Laenge und Schreibweise - in der KARTE gross
//      geschrieben, im Skript klein (die Engine vergleicht Entity-Namen ohne
//      Gross/klein: GetByName mit Q_strupr, G_Find mit Q_stricmp, TAG_Find
//      mit Q_strlwr).
// Beide Laeufe muessen Bild fuer Bild dasselbe ergeben - Ort, Winkel,
// Animation und Sichtbarkeit jeder Figur, die Kamera, die Mover, die
// Effektstarts, die Skriptstarts, die Benutzungen und der Kartenwechsel. Dazu
// prueft jede Vorlage, dass ihr Verhalten ueberhaupt eintritt.
//
// Keine Spieldaten: Karten, Skripte und die Laengen von Klaengen, Animationen
// und ROFF-Bahnen stehen hier im Text.
#include "bhed/ablauf.h"
#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/camtrack.h"
#include "bhed/mover.h"
#include "bhed/navlauf.h"
#include "bhed/regelwerk.h"
#include "bhed/scene.h"
#include "bhed/script.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace bhed;

namespace {

int g_fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

// --- Die Namen --------------------------------------------------------------

struct Satz {
    const char* name;
    std::map<std::string, std::string> skript;   // Platzhalter -> Name im Skript
    std::map<std::string, std::string> karte;    // ... in der Karte (Entity-Namen)
};

// Entity-Namen: in Satz B steht die Karte gross geschrieben.
const char* const kEntityNamen[] = {"@FIG1@", "@FIG2@", "@ZIEL@", "@WPA@",    "@WPB@", "@TUER@", "@REL@",   "@DEL@",
                                    "@ZAEHL@", "@SR@",  "@FX@",   "@KAM1@",   "@KAM2@", "@LEVEL@", "@NEU@", "@GITTER@"};

Satz satzOriginal() {
    Satz s;
    s.name = "Namen wie in den Originalen";
    s.skript = {{"@FIG1@", "obi1"},
                {"@FIG2@", "vader"},
                {"@TYP1@", "md_obi_tcw"},
                {"@TYP2@", "md_vader"},
                {"@ZIEL@", "obi1_nav3"},
                {"@WPA@", "wp_obi1"},
                {"@WPB@", "wp_obi2"},
                {"@TASK@", "walk_obi"},
                {"@TASK2@", "roff_door"},
                {"@SIG@", "intro_done"},
                {"@SKRIPT1@", "md_am/intro2_sith"},
                {"@SKRIPT2@", "md_am/cin1_sith"},
                {"@SKRIPT3@", "md_am/ani3_spawn"},
                {"@SND@", "sound/md_am/s3_kick.mp3"},
                {"@TUER@", "script_bigdoor_l"},
                {"@REL@", "relay1"},
                {"@DEL@", "delay1"},
                {"@ZAEHL@", "counter1"},
                {"@SR@", "runner1"},
                {"@FX@", "fx_explode"},
                {"@EFX@", "explosions/fighter_explosion2"},
                {"@KAM1@", "cam1"},
                {"@KAM2@", "cam1b"},
                {"@ROFF@", "kor2/roffs/boulder"},
                {"@LEVEL@", "end_level"},
                {"@MAP@", "md_am_sith2"},
                {"@NEU@", "ani3"},
                {"@GITTER@", "jediroom_doors"}};
    s.karte = s.skript;
    return s;
}

Satz satzNeu() {
    Satz s;
    s.name = "frische Namen, Karte gross geschrieben";
    s.skript = {{"@FIG1@", "zx_held7"},
                {"@FIG2@", "qm9x"},
                {"@TYP1@", "eigen_typ_a"},
                {"@TYP2@", "eigentypb"},
                {"@ZIEL@", "ziel_ost_01"},
                {"@WPA@", "wq_a"},
                {"@WPB@", "wq_bbb"},
                {"@TASK@", "lauf_t"},
                {"@TASK2@", "bahn_t2"},
                {"@SIG@", "SigFertig"},
                {"@SKRIPT1@", "meins/start"},
                {"@SKRIPT2@", "meins/teil_zwei"},
                {"@SKRIPT3@", "meins/neuling_da"},
                {"@SND@", "sound/eigen/stimme_a.wav"},
                {"@TUER@", "gitter_x"},
                {"@REL@", "rq_y"},
                {"@DEL@", "vq_2y"},
                {"@ZAEHL@", "zq_3y"},
                {"@SR@", "sr_01y"},
                {"@FX@", "knall_ay"},
                {"@EFX@", "eigen/knall"},
                {"@KAM1@", "blick_1y"},
                {"@KAM2@", "blick_zweiy"},
                {"@ROFF@", "eigen/roffs/bahn_a"},
                {"@LEVEL@", "ende_xy"},
                {"@MAP@", "eigene_karte2"},
                {"@NEU@", "neulingy"},
                {"@GITTER@", "torweg_q"}};
    s.karte = s.skript;
    for (const char* p : kEntityNamen) { s.karte[p] = gross(s.karte[p]); }
    return s;
}

std::string einsetzen(std::string text, const std::map<std::string, std::string>& n) {
    for (const auto& [platz, wert] : n) {
        for (std::size_t at = text.find(platz); at != std::string::npos; at = text.find(platz, at + wert.size())) {
            text.replace(at, platz.size(), wert);
        }
    }
    return text;
}

// Die Namen in einer Ausgabe wieder durch ihre Platzhalter ersetzen (ohne
// Gross/klein, laengste zuerst), damit A und B vergleichbar werden.
std::string zurueck(const std::string& text, const Satz& s) {
    std::vector<std::pair<std::string, std::string>> paare;
    for (const auto& [platz, wert] : s.skript) { paare.emplace_back(klein(wert), platz); }
    std::stable_sort(paare.begin(), paare.end(),
                     [](const auto& x, const auto& y) { return x.first.size() > y.first.size(); });
    std::string t = klein(text);
    for (const auto& [wert, platz] : paare) {
        for (std::size_t at = t.find(wert); at != std::string::npos; at = t.find(wert, at + platz.size())) {
            t.replace(at, wert.size(), platz);
        }
    }
    return t;
}

// --- Eine Mini-Mission --------------------------------------------------------

struct Vorlage {
    const char* name;
    std::string karte;
    std::vector<std::pair<std::string, std::string>> skripte;   // Pfad -> Text (ohne Kopfzeile)
    int brushModelle = 0;
};

struct Lauf {
    MapData karte;
    Ablauf ablauf;
    Scene szene;
    CameraTrack kamera;
    MoverSim mover;
    double ende = 0.0;
    std::string spur;   // die Ausgabe mit Platzhaltern statt Namen
};

std::string zahl(double v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.1f", std::fabs(v) < 0.05 ? 0.0 : v);
    return b;
}

// Laengen ohne Spieldaten: je Platzhalter eine feste Zahl.
constexpr double kStimmeMs = 2400.0;
constexpr double kRoffMs = 1500.0;
double animLaenge(const std::string& anim) {
    if (anim == "BOTH_CIN_1") { return 1500.0; }
    if (anim == "BOTH_CIN_2") { return 900.0; }
    return -1.0;
}

void fahre(const Vorlage& v, const Satz& s, bool md, Lauf& l) {
    regelwerkAnwenden(md, 1);
    parseEntities(einsetzen(v.karte, s.karte), l.karte.entities);
    std::map<std::string, Script> skripte;
    std::string startPfad;
    for (const auto& [pfad, text] : v.skripte) {
        Script sc;
        std::vector<Diag> d;
        (void)readScript("//Generated by BehavEd\r\n\r\n" + einsetzen(text, s.skript), sc, d);
        const std::string p = einsetzen(pfad, s.skript);
        if (startPfad.empty()) { startPfad = p; }
        skripte[klein(p)] = std::move(sc);
    }
    const std::string stimme = klein(s.skript.at("@SND@"));
    const std::string roff = klein(s.skript.at("@ROFF@"));
    AblaufWelt w;
    w.karte = &l.karte;
    w.skript = [&skripte](const std::string& p) -> const Script* {
        const auto it = skripte.find(klein(p));
        return it == skripte.end() ? nullptr : &it->second;
    };
    w.klangDauer = [stimme](const std::string& d) { return klein(d) == stimme ? kStimmeMs : -1.0; };
    w.roffDauer = [roff](const std::string& n) { return klein(n) == roff ? kRoffMs : kRoffFehlt; };
    w.animDauer = [](const std::string&, const std::string& a) { return animLaenge(a); };
    w.nav = navWeltAusEntities(l.karte.entities);
    // Eigene NPC-Beschreibungen statt der .npc des Spiels (Schluessel klein
    // wie parseNpcFile): Tempo 55/200, Drehrate 90.
    NpcMap npcs;
    for (const char* typ : {"@TYP1@", "@TYP2@"}) {
        NpcDef d;
        d.playerModel = "eigenes_modell";
        d.walkSpeed = 55.0F;
        d.runSpeed = 200.0F;
        d.yawSpeed = 90.0F;
        d.klasse = "CLASS_REBEL";
        d.team = "TEAM_NEUTRAL";
        npcs[klein(s.skript.at(typ))] = d;
    }
    const Script& start = skripte.at(klein(startPfad));
    // Wie die Vorschau (gui/app_view3d.cpp, prepareScene): erst der Ablauf,
    // dann eine erste Szene fuer Tempo und Kaesten, dann der gemeinsame Takt.
    const Ablauf erst = simuliereAblauf(start, startPfad, w);
    SzenenHilfe h;
    h.npcs = &npcs;
    h.nav = w.nav;
    h.kartenFiguren = true;
    h.animDauer = w.animDauer;
    h.erscheint = &erst.erscheint;
    h.fremdeDarsteller = &erst.fremdeDarsteller;
    const Scene ersteSzene = buildScene(erst.flach, l.karte, &h);
    w.tempo = ablaufTempo(ersteSzene);
    l.ablauf = simuliereAblaufMitKoerpern(start, startPfad, w);
    h.koerperPlaene = w.koerperPlaene;
    h.erscheint = &l.ablauf.erscheint;
    h.fremdeDarsteller = &l.ablauf.fremdeDarsteller;
    l.szene = buildScene(l.ablauf.flach, l.karte, &h);
    const MapData* karte = &l.karte;
    l.kamera = buildCameraTrack(l.ablauf.flach, [karte](const std::string& n, float o[3], float a[3]) {
        for (const MapEntity& e : karte->entities) {
            if (e.classname != "ref_tag" || klein(e.targetname) != klein(n)) { continue; }
            o[0] = o[1] = o[2] = 0.0F;
            a[0] = a[1] = a[2] = 0.0F;
            (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            if (const std::string* w2 = e.find("angles")) { (void)std::sscanf(w2->c_str(), "%f %f %f", &a[0], &a[1], &a[2]); }
            return true;
        }
        return false;
    });
    l.ende = std::max({l.szene.durationMs, l.kamera.durationMs, l.ablauf.endeMs, 1000.0}) + 1500.0;
    l.ende = std::min(l.ende, 40000.0);
    BspGeometry geo;
    geo.models.resize(static_cast<std::size_t>(v.brushModelle) + 1U);
    for (std::size_t i = 1; i < geo.models.size(); ++i) {
        geo.models[i].mins[0] = geo.models[i].mins[1] = geo.models[i].mins[2] = 0.0F;
        geo.models[i].maxs[0] = 8.0F;
        geo.models[i].maxs[1] = 64.0F;
        geo.models[i].maxs[2] = 128.0F;
    }
    std::vector<MoverUse> uses;
    for (const Ablauf::Benutzung& b : l.ablauf.benutzt) {
        uses.push_back(MoverUse{b.ms, b.name, true, b.ausloeser, std::numeric_limits<int>::max(), b.entity, b.inaktiv});
    }
    l.mover.baue(l.karte, geo, uses, nullptr, l.ende);

    // --- Die Spur ----------------------------------------------------------
    std::string t;
    t += "ende " + zahl(l.ablauf.endeMs) + " wechsel " + zahl(l.ablauf.kartenwechselMs) + " " + l.ablauf.kartenwechselKarte + "\n";
    for (std::size_t i = 0; i < l.ablauf.skripte.size(); ++i) {
        t += "skript " + l.ablauf.skripte[i] + " ab " + zahl(i < l.ablauf.skriptBeginn.size() ? l.ablauf.skriptBeginn[i] : -9.0) + "\n";
    }
    for (const auto& [n, ms] : l.ablauf.erscheint) { t += "erscheint " + n + " " + zahl(ms) + "\n"; }
    for (const Ablauf::Benutzung& b : l.ablauf.benutzt) {
        if (b.inaktiv >= 0) {   // F4b: SVF_INACTIVE einer Entity
            t += "inaktiv " + std::to_string(b.entity) + " " + std::to_string(b.inaktiv) + " " + zahl(b.ms) + "\n";
            continue;
        }
        t += "use " + b.name + " " + zahl(b.ms) + " von " + b.ausloeser + (b.entity >= 0 ? " nur " + std::to_string(b.entity) : "") + "\n";
    }
    for (const std::string& hw : l.ablauf.hinweise) { t += "hinweis " + hw + "\n"; }
    t += writeScript(l.ablauf.flach);
    std::vector<const Actor*> figuren;
    for (const Actor& a : l.szene.actors) { figuren.push_back(&a); }
    std::stable_sort(figuren.begin(), figuren.end(),
                     [&s](const Actor* x, const Actor* y) { return zurueck(x->name, s) < zurueck(y->name, s); });
    for (const Actor* a : figuren) {
        t += "figur " + a->name + " typ " + a->npcType + "\n";
        for (const ActorStep& st : a->steps) { t += "schritt " + std::to_string(static_cast<int>(st.kind)) + " " + zahl(st.startMs) + " " + zahl(st.endMs) + " " + zahl(st.ankunftMs) + " " + st.text + "\n"; }
        for (double ms = 0.0; ms <= l.ende; ms += 100.0) {
            const ActorState st = a->at(ms);
            t += zahl(ms) + ": " + zahl(st.pos[0]) + " " + zahl(st.pos[1]) + " " + zahl(st.pos[2]) + " / " + zahl(st.angles[0]) + " " +
                 zahl(st.angles[1]) + " " + st.animation + " " + st.torsoAnimation + (st.visible ? " v" : " -") +
                 (st.removed ? " r" : "") + "\n";
        }
    }
    for (double ms = 0.0; ms <= l.ende; ms += 100.0) {
        const CameraState c = l.kamera.at(ms);
        t += "kamera " + zahl(ms) + ": " + (c.enabled ? "an " : "aus ") + zahl(c.pos[0]) + " " + zahl(c.pos[1]) + " " + zahl(c.pos[2]) +
             " / " + zahl(c.angles[0]) + " " + zahl(c.angles[1]) + " " + zahl(c.angles[2]) + " fov " + zahl(c.fovX) + " blende " +
             zahl(c.fadeColor[3]) + "\n";
    }
    for (int m = 1; m <= v.brushModelle; ++m) {
        for (double ms = 0.0; ms <= l.ende; ms += 100.0) {
            const MoverSim::Stand st = l.mover.at(m, ms);
            t += "mover " + std::to_string(m) + " " + zahl(ms) + ": " + zahl(st.origin[0]) + " " + zahl(st.origin[1]) + " " +
                 zahl(st.origin[2]) + " / " + zahl(st.angles[1]) + (st.sichtbar ? " v" : " -") + "\n";
        }
    }
    for (const FxRunnerStart& f : l.mover.fxStarts()) {
        t += "fx " + std::to_string(f.entity) + " " + zahl(f.ms) + (f.oneShot ? " einmal" : "") + "\n";
    }
    l.spur = zurueck(t, s);
}

const Actor* figur(const Lauf& l, const std::string& name) {
    for (const Actor& a : l.szene.actors) {
        if (klein(a.name) == klein(name)) { return &a; }
    }
    return nullptr;
}

double skriptAb(const Lauf& l, const std::string& pfad) {
    for (std::size_t i = 0; i < l.ablauf.skripte.size() && i < l.ablauf.skriptBeginn.size(); ++i) {
        if (klein(l.ablauf.skripte[i]) == klein(pfad)) { return l.ablauf.skriptBeginn[i]; }
    }
    return -1.0;
}

double abstand2(const float* p, float x, float y) { return std::hypot(static_cast<double>(p[0] - x), static_cast<double>(p[1] - y)); }

// Erste Abweichung zweier Spuren, fuer die Meldung.
std::string ersteAbweichung(const std::string& a, const std::string& b) {
    std::size_t za = 0;
    std::size_t zb = 0;
    while (za < a.size() || zb < b.size()) {
        const std::size_t ea = std::min(a.find('\n', za), a.size());
        const std::size_t eb = std::min(b.find('\n', zb), b.size());
        const std::string la = a.substr(za, ea - za);
        const std::string lb = b.substr(zb, eb - zb);
        if (la != lb) { return "A: " + la + " | B: " + lb; }
        za = ea + 1;
        zb = eb + 1;
    }
    return {};
}

using Probe = std::function<void(const Lauf&, const Satz&, const std::string&)>;

void beide(const Vorlage& v, const Probe& probe) {
    for (const bool md : {false, true}) {
        const std::string regel = md ? "Movie Duels" : "OpenJK";
        const Satz a = satzOriginal();
        const Satz b = satzNeu();
        Lauf la;
        Lauf lb;
        fahre(v, a, md, la);
        fahre(v, b, md, lb);
        const std::string kopf = std::string(v.name) + " (" + regel + ")";
        probe(la, a, kopf + ", " + a.name);
        probe(lb, b, kopf + ", " + b.name);
        const std::string ab = ersteAbweichung(la.spur, lb.spur);
        pruefe(ab.empty(), kopf + ": beide Namenssaetze ergeben Bild fuer Bild dasselbe" + (ab.empty() ? "" : " - " + ab));
    }
}

// --- Die Vorlagen -------------------------------------------------------------

// 1. Laufen und Wegnetz, Animation mit Haltezeit, task/dowait, signal/waitsignal,
//    run, Stimme mit dowait (CHAN_VOICE), Blickziel, SET_ORIGIN/SET_ANGLES.
const Vorlage kLauf = {
    "Laufen/Wegnetz, Animation, Signale, Stimme",
    "{\n\"classname\" \"worldspawn\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@FIG1@\"\n\"NPC_type\" \"@TYP1@\"\n\"origin\" \"0 0 100\"\n\"angle\" \"0\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@FIG2@\"\n\"NPC_type\" \"@TYP2@\"\n\"origin\" \"0 300 100\"\n\"angle\" \"270\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@NEU@\"\n\"NPC_type\" \"@TYP2@\"\n\"origin\" \"300 300 100\"\n}\n"
    "{\n\"classname\" \"waypoint\"\n\"targetname\" \"@WPA@\"\n\"origin\" \"190 0 100\"\n}\n"
    "{\n\"classname\" \"waypoint_navgoal\"\n\"targetname\" \"@ZIEL@\"\n\"origin\" \"380 0 100\"\n}\n",
    {{"@SKRIPT1@",
      "affect ( \"@FIG1@\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", \"BOTH_CIN_1\" );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_BOTH\", 1200 );\r\n"
      "\twait ( 1200.000 );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_WALKING\", /*@BOOL_TYPES*/ \"true\" );\r\n"
      "\ttask ( \"@TASK@\" )\r\n\t{\r\n"
      "\t\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"@ZIEL@\" );\r\n"
      "\t}\r\n\r\n"
      "\tdowait ( \"@TASK@\" );\r\n"
      "\tsignal ( \"@SIG@\" );\r\n"
      "}\r\n\r\n"
      "waitsignal ( \"@SIG@\" );\r\n"
      "run ( \"@SKRIPT2@\" );\r\n"},
     {"@SKRIPT2@",
      "affect ( \"@FIG2@\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_WATCHTARGET\", \"@NEU@\" );\r\n"
      "\ttask ( \"@TASK@\" )\r\n\t{\r\n"
      "\t\tsound ( /*@CHANNELS*/ CHAN_VOICE, \"@SND@\" );\r\n"
      "\t}\r\n\r\n"
      "\tdowait ( \"@TASK@\" );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ANIM_UPPER\", \"BOTH_CIN_2\" );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_UPPER\", 800 );\r\n"
      "\twait ( 1000.000 );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ORIGIN\", < 100.000 300.000 24.000 > );\r\n"
      "\tset ( /*@SET_TYPES*/ \"SET_ANGLES\", < 0.000 90.000 0.000 > );\r\n"
      "}\r\n"}},
    0};

// 2. Kamera: ENABLE, MOVE/PAN auf ref_tag-Marken, ZOOM, FADE, SHAKE, DISABLE.
const Vorlage kKamera = {
    "Kamera",
    "{\n\"classname\" \"worldspawn\"\n}\n"
    "{\n\"classname\" \"ref_tag\"\n\"targetname\" \"@KAM1@\"\n\"origin\" \"0 -200 100\"\n\"angles\" \"10 90 0\"\n}\n"
    "{\n\"classname\" \"ref_tag\"\n\"targetname\" \"@KAM2@\"\n\"origin\" \"300 -200 150\"\n\"angles\" \"20 120 0\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@FIG1@\"\n\"NPC_type\" \"@TYP1@\"\n\"origin\" \"0 0 24\"\n}\n",
    {{"@SKRIPT1@",
      "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ MOVE, $tag( \"@KAM1@\", ORIGIN )$, 0 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ PAN, $tag( \"@KAM1@\", ANGLES )$, < 0.000 0.000 0.000 >, 0 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ ZOOM, 60, 0 );\r\n"
      "wait ( 500.000 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ MOVE, $tag( \"@KAM2@\", ORIGIN )$, 2000 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ PAN, $tag( \"@KAM2@\", ANGLES )$, < 0.000 0.000 0.000 >, 2000 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ FADE, < 0.000 0.000 0.000 >, 0.000, < 0.000 0.000 0.000 >, 1.000, 1000 );\r\n"
      "wait ( 2500.000 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ SHAKE, 4, 500 );\r\n"
      "wait ( 1000.000 );\r\n"
      "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\r\n"}},
    0};

// 3. Mover und ROFF: move/rotate auf einem Brush-Modell mit
//    script_targetname, dowait auf die Fahrt, PLAY_ROFF mit Bahnlaenge.
const Vorlage kMover = {
    "Mover und ROFF",
    "{\n\"classname\" \"worldspawn\"\n}\n"
    "{\n\"classname\" \"func_static\"\n\"model\" \"*1\"\n\"script_targetname\" \"@TUER@\"\n\"origin\" \"0 0 0\"\n}\n",
    {{"@SKRIPT1@",
      "affect ( \"@TUER@\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
      "\ttask ( \"@TASK@\" )\r\n\t{\r\n"
      "\t\tmove ( < 0.000 0.000 200.000 >, 1500.000 );\r\n"
      "\t}\r\n\r\n"
      "\tdowait ( \"@TASK@\" );\r\n"
      "\trotate ( < 0.000 90.000 0.000 >, 1000.000 );\r\n"
      "\twait ( 1000.000 );\r\n"
      "\ttask ( \"@TASK2@\" )\r\n\t{\r\n"
      "\t\tplay ( /*@PLAY_TYPES*/ \"PLAY_ROFF\", \"@ROFF@\" );\r\n"
      "\t}\r\n\r\n"
      "\tdowait ( \"@TASK2@\" );\r\n"
      "\tsignal ( \"@SIG@\" );\r\n"
      "}\r\n\r\n"
      "waitsignal ( \"@SIG@\" );\r\n"
      "wait ( 500.000 );\r\n"}},
    1};

// 4. Entity-Logik: use ueber target_relay, target_delay und target_counter auf
//    einen target_scriptrunner (usescript), ein NPC_spawner auf use mit
//    spawnscript, ein fx_runner und eine Tuer auf use, kill/remove und der
//    Kartenwechsel (target_level_change).
const Vorlage kLogik = {
    "Entity-Logik, Effekte, Tuer, Kartenwechsel",
    "{\n\"classname\" \"worldspawn\"\n}\n"
    "{\n\"classname\" \"target_relay\"\n\"targetname\" \"@REL@\"\n\"target\" \"@DEL@\"\n}\n"
    "{\n\"classname\" \"target_delay\"\n\"targetname\" \"@DEL@\"\n\"target\" \"@ZAEHL@\"\n\"wait\" \"1.5\"\n}\n"
    "{\n\"classname\" \"target_counter\"\n\"targetname\" \"@ZAEHL@\"\n\"target\" \"@SR@\"\n\"count\" \"2\"\n}\n"
    "{\n\"classname\" \"target_scriptrunner\"\n\"targetname\" \"@SR@\"\n\"usescript\" \"@SKRIPT2@\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@NEU@\"\n\"targetname\" \"@NEU@\"\n\"NPC_type\" \"@TYP2@\"\n"
    "\"spawnscript\" \"@SKRIPT3@\"\n\"origin\" \"200 0 24\"\n}\n"
    "{\n\"classname\" \"NPC_spawner\"\n\"NPC_targetname\" \"@FIG1@\"\n\"NPC_type\" \"@TYP1@\"\n\"origin\" \"-200 0 24\"\n}\n"
    "{\n\"classname\" \"fx_runner\"\n\"targetname\" \"@FX@\"\n\"fxFile\" \"@EFX@\"\n\"spawnflags\" \"1\"\n\"origin\" \"0 200 24\"\n}\n"
    "{\n\"classname\" \"func_door\"\n\"model\" \"*1\"\n\"targetname\" \"@GITTER@\"\n\"angle\" \"90\"\n}\n"
    "{\n\"classname\" \"target_level_change\"\n\"targetname\" \"@LEVEL@\"\n\"mapname\" \"@MAP@\"\n}\n",
    {{"@SKRIPT1@",
      "use ( \"@REL@\" );\r\n"
      "wait ( 500.000 );\r\n"
      "use ( \"@ZAEHL@\" );\r\n"
      "wait ( 3000.000 );\r\n"
      "kill ( \"@FIG1@\" );\r\n"
      "wait ( 1000.000 );\r\n"
      "remove ( \"@FIG1@\" );\r\n"
      "wait ( 6000.000 );\r\n"
      "use ( \"@LEVEL@\" );\r\n"},
     {"@SKRIPT2@",
      "use ( \"@NEU@\" );\r\n"
      "wait ( 300.000 );\r\n"
      "use ( \"@FX@\" );\r\n"
      "use ( \"@GITTER@\" );\r\n"},
     {"@SKRIPT3@",
      "set ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n"
      "set ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", \"BOTH_CIN_1\" );\r\n"
      "set ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_BOTH\", 1500 );\r\n"}},
    1};

}  // namespace

int main() {
    std::printf("pruefmission\n");

    beide(kLauf, [](const Lauf& l, const Satz& s, const std::string& k) {
        const Actor* f1 = figur(l, s.skript.at("@FIG1@"));
        const Actor* f2 = figur(l, s.skript.at("@FIG2@"));
        pruefe(f1 != nullptr && f2 != nullptr, k + ": beide Figuren aus den Spawnern");
        if (f1 == nullptr || f2 == nullptr) { return; }
        pruefe(f1->at(600.0).animation == "BOTH_CIN_1", k + ": SET_ANIM_BOTH mit Haltezeit (" + f1->at(600.0).animation + ")");
        const double zwei = skriptAb(l, s.skript.at("@SKRIPT2@"));
        pruefe(zwei > 1200.0, k + ": run nach waitsignal erst nach dem Lauf (" + zahl(zwei) + " ms)");
        pruefe(abstand2(f1->at(zwei).pos, 380.0F, 0.0F) < 40.0, k + ": die Figur ist am Laufziel, als das Signal kommt");
        pruefe(abstand2(f1->at(1000.0).pos, 0.0F, 0.0F) < 1.0, k + ": vor dem Ende der Haltezeit steht sie");
        // Nach SET_ANGLES dreht sie sich wieder zum Blickziel (BS_CINEMATIC,
        // NPC_BSCinematic) - die Winkel also im Bild des Sprungs.
        double sprung = -1.0;
        for (const ActorStep& st : f2->steps) {
            if (st.kind == ActorStep::Kind::Teleport) { sprung = st.startMs; }
        }
        const ActorState dort = f2->at(sprung);
        pruefe(sprung > zwei + kStimmeMs && abstand2(f2->at(l.ende).pos, 100.0F, 300.0F) < 1.0 && std::fabs(dort.angles[1] - 90.0F) < 1.0F,
               k + ": SET_ORIGIN/SET_ANGLES nach Stimme und Animation (" + zahl(sprung) + " ms)");
        const ActorState mitte = f2->at(zwei + kStimmeMs + 300.0);
        pruefe(mitte.torsoAnimation == "BOTH_CIN_2", k + ": dowait wartet auf die Stimme, dann SET_ANIM_UPPER (" + mitte.torsoAnimation + ")");
        pruefe(abstand2(f2->at(zwei + kStimmeMs - 200.0).pos, 0.0F, 300.0F) < 1.0, k + ": waehrend der Stimme noch am Spawner");
        // Das Blickziel ist eine Figur, die nur in der Karte steht (kein
        // affect): G_Find findet sie ohne Gross/klein (Q3_SetWatchTarget).
        const float gier = f2->at(zwei + kStimmeMs - 200.0).angles[1];
        pruefe(std::fabs(gier) < 2.0F, k + ": SET_WATCHTARGET dreht zur Figur aus der Karte (" + zahl(gier) + ")");
    });

    beide(kKamera, [](const Lauf& l, const Satz&, const std::string& k) {
        const CameraState a = l.kamera.at(100.0);
        pruefe(a.enabled && std::fabs(a.pos[1] + 200.0F) < 0.5F && std::fabs(a.pos[2] - 100.0F) < 0.5F,
               k + ": MOVE auf die erste Marke");
        pruefe(std::fabs(a.angles[1] - 90.0F) < 0.5F && std::fabs(a.fovX - 60.0F) < 0.5F, k + ": PAN auf ihre Winkel, ZOOM 60");
        const CameraState b = l.kamera.at(2600.0);
        pruefe(std::fabs(b.pos[0] - 300.0F) < 0.5F && std::fabs(b.pos[2] - 150.0F) < 0.5F, k + ": nach 2 s an der zweiten Marke");
        pruefe(l.kamera.at(1000.0).fadeColor[3] > 0.2F && l.kamera.at(1000.0).fadeColor[3] < 0.8F, k + ": FADE blendet ueber 1 s ein");
        pruefe(!l.kamera.at(l.ende).enabled, k + ": DISABLE am Ende");
    });

    beide(kMover, [](const Lauf& l, const Satz& s, const std::string& k) {
        const Actor* t = figur(l, s.skript.at("@TUER@"));
        pruefe(t != nullptr && t->brushModel == 1, k + ": das Brush-Modell ist ueber script_targetname ansprechbar");
        if (t == nullptr) { return; }
        pruefe(std::fabs(t->at(1600.0).pos[2] - 200.0F) < 1.0F, k + ": move faehrt in 1,5 s 200 nach oben");
        pruefe(std::fabs(t->at(2700.0).angles[1] - 90.0F) < 1.0F, k + ": rotate dreht nach dowait");
        const double ende = l.ablauf.endeMs;
        pruefe(ende >= 1500.0 + 1000.0 + kRoffMs + 400.0, k + ": dowait auf PLAY_ROFF wartet die Bahn ab (Ende " + zahl(ende) + " ms)");
    });

    beide(kLogik, [](const Lauf& l, const Satz& s, const std::string& k) {
        const double zwei = skriptAb(l, s.skript.at("@SKRIPT2@"));
        pruefe(zwei >= 1500.0 && zwei < 2200.0, k + ": relay -> delay (1,5 s) -> counter (2. use) -> usescript (" + zahl(zwei) + " ms)");
        const auto er = l.ablauf.erscheint.find(s.karte.at("@NEU@"));
        const bool neuDa = er != l.ablauf.erscheint.end() ||
                           l.ablauf.erscheint.find(klein(s.karte.at("@NEU@"))) != l.ablauf.erscheint.end() ||
                           l.ablauf.erscheint.find(s.skript.at("@NEU@")) != l.ablauf.erscheint.end();
        pruefe(neuDa, k + ": der NPC_spawner auf use laesst seine Figur erscheinen");
        pruefe(skriptAb(l, s.skript.at("@SKRIPT3@")) >= zwei, k + ": sein spawnscript laeuft");
        pruefe(!l.mover.fxStarts().empty(), k + ": der fx_runner startet auf use");
        pruefe(l.mover.at(1, l.ende).origin[1] > 1.0F || !l.mover.oeffnungen().empty(), k + ": die Tuer oeffnet auf use");
        pruefe(l.ablauf.kartenwechselMs > 10000.0 && klein(l.ablauf.kartenwechselKarte) == klein(s.karte.at("@MAP@")),
               k + ": target_level_change wechselt die Karte (" + zahl(l.ablauf.kartenwechselMs) + " ms)");
        const Actor* f1 = figur(l, s.skript.at("@FIG1@"));
        pruefe(f1 != nullptr && f1->at(l.ende).removed, k + ": kill/remove nimmt die Figur weg");
    });

    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "pruefmission: alles gut" : "pruefmission: FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
