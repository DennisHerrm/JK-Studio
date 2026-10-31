// spurentest.cpp - das Modell der Zeitleiste (bhed/spuren.h) an einer
// eigenen Mini-Szene: eigene Karte, eigenes Skript, eigene Namen, jede
// Befehlsart einmal. Die Erwartungswerte kommen aus den Regeln der Engine
// (TaskManager-Takt 50 ms, cg_camera.cpp linear, Klanglaenge), nicht aus
// dem Ergebnis - nichts ist auf diese Szene zugeschnitten.
#include "bhed/ablauf.h"
#include "bhed/bsp.h"
#include "bhed/camtrack.h"
#include "bhed/commands.h"
#include "bhed/scene.h"
#include "bhed/script.h"
#include "bhed/spuren.h"
#include "bhed/timeline.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int fehler = 0;

void pruefe(bool ok, const char* was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

bool nah(double a, double b, double eps = 0.5) { return std::fabs(a - b) <= eps; }

const char* const kKarte =
    "{\n\"classname\" \"NPC_spawner\"\n\"npc_targetname\" \"brakk\"\n\"NPC_type\" \"md_tusken_chief\"\n"
    "\"origin\" \"0 0 100\"\n\"angle\" \"0\"\n}\n"
    "{\n\"classname\" \"waypoint_navgoal\"\n\"targetname\" \"duene\"\n\"origin\" \"380 0 100\"\n}\n"
    "{\n\"classname\" \"ref_tag\"\n\"targetname\" \"blick_a\"\n\"origin\" \"0 -300 160\"\n\"angles\" \"0 90 0\"\n}\n"
    "{\n\"classname\" \"ref_tag\"\n\"targetname\" \"blick_b\"\n\"origin\" \"200 -300 160\"\n\"angles\" \"0 90 0\"\n}\n"
    "{\n\"classname\" \"target_play_music\"\n\"targetname\" \"lied\"\n\"music\" \"music/mini/lied.mp3\"\n}\n"
    "{\n\"classname\" \"target_level_change\"\n\"targetname\" \"weiter\"\n\"mapname\" \"mini2\"\n}\n";

// Jede Befehlsart einmal: Kamera aller Arten mit einer Ueberlappung, ein
// Satz mit bekannter Laenge und Untertitel, eine Animation, ein Lauf,
// signal/waitsignal, dowait, use auf Musik, ein Befehl, den die Vorschau
// nicht nachbildet (print, camera TRACK), und der Kartenwechsel.
const char* const kSkript =
    "camera ( ENABLE );\n"
    "camera ( MOVE, tag ( \"blick_a\", ORIGIN ), 0 );\n"
    "camera ( PAN, < 0 90 0 >, < 0 0 0 >, 0 );\n"
    "camera ( FADE, < 0 0 0 >, 1, < 0 0 0 >, 0, 1000 );\n"
    "camera ( ZOOM, 60, 2000 );\n"
    "affect ( \"brakk\", FLUSH )\n{\n"
    "\tset ( \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\n"
    "\tset ( \"SET_WALKING\", \"true\" );\n"
    "\tset ( \"SET_ANIM_BOTH\", \"BOTH_GESTURE1\" );\n"
    "\ttask ( \"rede\" )\n\t{\n\t\tsound ( CHAN_VOICE, \"sound/chars/mini/brakk01.mp3\" );\n\t}\n"
    "\tdowait ( \"rede\" );\n"
    "\tset ( \"SET_NAVGOAL\", \"duene\" );\n"
    "\tsignal ( \"losgelaufen\" );\n"
    "}\n"
    "wait ( 1000 );\n"
    "camera ( MOVE, < 100 0 150 >, 3000 );\n"
    "wait ( 1000 );\n"
    "camera ( MOVE, tag ( \"blick_b\", ORIGIN ), 1000 );\n"
    "waitsignal ( \"losgelaufen\" );\n"
    "use ( \"lied\" );\n"
    "print ( \"Ende\" );\n"
    "camera ( TRACK, \"pfad1\", 100, 0 );\n"
    "wait ( 500 );\n"
    "camera ( DISABLE );\n"
    "use ( \"weiter\" );\n";

constexpr double kKlangMs = 2345.0;
constexpr double kGesteMs = 1500.0;

}  // namespace

int main() {
    MapData karte;
    parseEntities(kKarte, karte.entities);
    Script skript;
    std::vector<Diag> diag;
    (void)readScript(kSkript, skript, diag);

    AblaufWelt welt;
    welt.karte = &karte;
    welt.klangDauer = [](const std::string&) { return kKlangMs; };
    welt.animDauer = [](const std::string&, const std::string& a) { return a == "BOTH_GESTURE1" ? kGesteMs : 0.0; };
    const Ablauf ab = simuliereAblauf(skript, "", welt);

    const TagLookup marken = [&karte](const std::string& name, float o[3], float a[3]) {
        for (const MapEntity& e : karte.entities) {
            if (e.classname != "ref_tag" || e.targetname != name) { continue; }
            (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            a[0] = 0.0F;
            a[1] = 90.0F;
            a[2] = 0.0F;
            return true;
        }
        return false;
    };
    CameraTrack kamera = buildCameraTrack(ab.flach, marken);
    const CommandDb db;
    Timeline zeit = buildTimeline(ab.flach, db);
    SzenenHilfe hilfe;
    hilfe.animDauer = welt.animDauer;
    hilfe.erscheint = &ab.erscheint;
    Scene szene = buildScene(ab.flach, karte, &hilfe);
    if (ab.kartenwechselMs >= 0.0) {
        zeit.practicalEndMs = std::min(zeit.practicalEndMs, ab.kartenwechselMs);
        kamera.durationMs = std::min(kamera.durationMs, ab.kartenwechselMs);
    }
    // Die Pfade zeigen ins flache Skript - zurueck aufs offene (wie
    // pfadeAufsSkript in der Oberflaeche).
    const auto um = [&ab](Path& p) {
        const AblaufHerkunft* h = ab.woher(p);
        if (h == nullptr || h->skript != 0) {
            p.clear();
        } else {
            p = h->path;
        }
    };
    for (CamSegment& s : kamera.segments) { um(s.path); }
    for (TimelineTrack& t : zeit.tracks) {
        for (TimelineEvent& e : t.events) { um(e.path); }
    }
    for (Actor& a : szene.actors) {
        for (ActorStep& st : a.steps) { um(st.path); }
    }

    spuren::SpurenQuellen q;
    q.zeitleiste = &zeit;
    q.kamera = &kamera;
    q.szene = &szene;
    q.ablauf = &ab;
    q.skript = &skript;
    q.klangMs = [](const std::string&) { return kKlangMs; };
    q.untertitel = [](const std::string& d, std::string* k) {
        if (k != nullptr) { *k = "MINI_BRAKK01"; }
        return d == "sound/chars/mini/brakk01.mp3" ? std::string("Die Duene ruft.") : std::string();
    };
    q.musikVon = [&karte](const std::string& ziel) {
        for (const MapEntity& e : karte.entities) {
            if (e.classname == "target_play_music" && e.targetname == ziel) {
                const std::string* m = e.find("music");
                return m != nullptr ? *m : std::string();
            }
        }
        return std::string();
    };
    const spuren::Spuren s = spuren::baue(q);

    std::printf("Kamera\n");
    // wait ( 1000 ) ist im Bild NACH seinem Ablauf fertig: 1050 ms (TaskManager.cpp:1100).
    pruefe(s.fahrt.clips.size() == 5, "Fahrt-Zeile: drei MOVE und Kamera an/aus");
    std::vector<const spuren::Clip*> moves;
    for (const spuren::Clip& c : s.fahrt.clips) {
        if (c.art == spuren::Art::Fahrt) { moves.push_back(&c); }
    }
    pruefe(moves.size() == 3, "drei Kamerafahrten");
    if (moves.size() == 3) {
        pruefe(nah(moves[1]->startMs, 1050.0) && nah(moves[1]->endMs, 4050.0), "MOVE nach wait 1000: 1050 bis 4050 ms");
        pruefe(nah(moves[2]->startMs, 2100.0), "zweites wait 1000: 2100 ms");
        pruefe(moves[2]->springt && nah(moves[2]->vorigerStartMs, 1050.0), "der dritte MOVE springt zurueck (voriger ab 1050)");
        pruefe(!moves[1]->springt && !moves[0]->springt, "die ersten beiden springen nicht");
        pruefe(moves[0]->text == "blick_a" && moves[1]->text == "(100, 0, 150)", "Beschriftung: Marke bzw. (x, y, z)");
        pruefe(moves[1]->laengeZiehbar && moves[1]->verschiebbar, "Fahrt mit Dauer: ziehbar");
        pruefe(moves[1]->bahn != moves[2]->bahn, "Ueberlappung auf eigener Unterbahn");
    }
    pruefe(s.ueberlappungen == 1, "genau eine Ueberlappung");
    pruefe(s.ort.clips.size() == 5, "Uebersicht Ort: drei Einstellungen und Kamera an/aus");
    {
        std::vector<const spuren::Clip*> e;
        for (const spuren::Clip& c : s.ort.clips) {
            if (c.art == spuren::Art::Einstellung) { e.push_back(&c); }
        }
        if (e.size() == 3) {
            pruefe(nah(e[0]->endMs, 1050.0) && nah(e[1]->endMs, 2100.0), "Einstellung bis zum naechsten MOVE");
            pruefe(e[2]->problem && !e[1]->problem, "die Einstellung mit dem Sprung ist markiert");
            double aus = -1.0;
            for (const spuren::Clip& c : s.ort.clips) {
                if (c.art == spuren::Art::KameraAus) { aus = c.startMs; }
            }
            pruefe(aus > 0.0 && nah(e[2]->endMs, aus), "die letzte endet bei camera DISABLE");
        } else {
            pruefe(false, "drei Einstellungen");
        }
    }
    pruefe(s.zoom.clips.size() == 1 && s.zoom.clips[0].text == "60" && nah(s.zoom.clips[0].endMs, 2000.0), "ZOOM 60 ueber 2000 ms");
    // CAMERA_DEFAULT_FOV 90 (cg_camera.h), linear (CGCam_Update).
    pruefe(nah(spuren::kurveBei(s.sichtfeld, 0.0), 90.0, 0.01), "Bildwinkel am Anfang 90");
    pruefe(nah(spuren::kurveBei(s.sichtfeld, 1000.0), 75.0, 0.01), "Bildwinkel linear: 75 zur halben Zeit");
    pruefe(nah(spuren::kurveBei(s.sichtfeld, 2000.0), 60.0, 0.01), "Bildwinkel 60 am Ende des ZOOM");
    pruefe(nah(spuren::kurveBei(s.schwarz, 0.0), 1.0, 0.001) && nah(spuren::kurveBei(s.schwarz, 500.0), 0.5, 0.001) &&
               nah(spuren::kurveBei(s.schwarz, 1000.0), 0.0, 0.001),
           "Blende linear von schwarz auf klar (CGCam_UpdateFade)");
    pruefe(s.blende.clips.size() == 1 && nah(s.blende.clips[0].wert, 0.0, 0.001), "FADE: Ziel-Deckkraft 0");
    pruefe(s.drehung.clips.size() == 1 && nah(s.drehung.clips[0].endMs - s.drehung.clips[0].startMs, 0.0),
           "PAN ohne Dauer (Schnitt)");

    std::printf("Ton\n");
    pruefe(s.sprache.clips.size() == 1, "ein Satz");
    if (!s.sprache.clips.empty()) {
        const spuren::Clip& c = s.sprache.clips[0];
        pruefe(nah(c.endMs - c.startMs, kKlangMs), "Laenge = Laenge der Klangdatei");
        pruefe(c.untertitel == "Die Duene ruft." && c.schluessel == "MINI_BRAKK01", "Untertitel und Schluessel");
        pruefe(c.wer == "brakk" && c.kanal == "CHAN_VOICE", "Sprecher und Kanal");
    }
    pruefe(s.musik.clips.size() == 1 && s.musik.clips[0].datei == "music/mini/lied.mp3", "Musik aus target_play_music");
    if (!s.musik.clips.empty()) {
        pruefe(nah(s.musik.clips[0].endMs, s.dauerMs) && !s.musik.clips[0].pfad.empty(), "Musik bis zum Ende, mit Skriptzeile");
    }

    std::printf("Ablauf\n");
    const spuren::Clip* warteSignal = nullptr;
    int nicht = 0;
    bool wechsel = false;
    for (const spuren::Clip& c : s.skript.clips) {
        if (c.art == spuren::Art::Warten && c.warteArt == spuren::WarteArt::Signal) { warteSignal = &c; }
        nicht += c.art == spuren::Art::NichtNachgebildet ? 1 : 0;
        wechsel = wechsel || (c.art == spuren::Art::Kartenwechsel && c.text == "mini2");
    }
    pruefe(warteSignal != nullptr && warteSignal->wartetAuf == "losgelaufen", "waitsignal: worauf");
    if (warteSignal != nullptr) {
        pruefe(warteSignal->sender.size() == 1 && warteSignal->sender[0].second == "brakk" &&
                   nah(warteSignal->sender[0].first, warteSignal->endMs, 60.0),
               "waitsignal: Sender und Zeit (das Signal beendet das Warten)");
    }
    pruefe(nicht == 2 && s.nichtNachgebildet == 2, "print und camera TRACK: nicht nachgebildet");
    pruefe(wechsel && s.kartenwechselMs >= 0.0, "Kartenwechsel-Marke nach mini2");
    int waitsMitKante = 0;
    for (const spuren::Clip& c : s.skript.clips) {
        if (c.art == spuren::Art::Warten && c.laengeZiehbar) {
            ++waitsMitKante;
            pruefe(nah(c.endMs - c.startMs, c.befehl.find("1000") != std::string::npos ? 1050.0 : 550.0),
                   "wait-Block: Wert + ein Spieltakt");
        }
    }
    pruefe(waitsMitKante == 3, "drei wait mit Zahl haben eine Kante");
    {
        int doppelt = 0;
        for (std::size_t i = 0; i < s.skript.clips.size(); ++i) {
            for (std::size_t j = i + 1; j < s.skript.clips.size(); ++j) {
                const spuren::Clip& a = s.skript.clips[i];
                const spuren::Clip& b = s.skript.clips[j];
                if (a.art == spuren::Art::Warten && b.art == spuren::Art::Warten && b.startMs < a.endMs - 0.5 && a.startMs < b.endMs - 0.5) {
                    ++doppelt;
                    std::printf("    ueberlappende Wartezeiten: %.0f-%.0f \"%s\" / %.0f-%.0f \"%s\"\n", a.startMs, a.endMs, a.befehl.c_str(),
                                b.startMs, b.endMs, b.befehl.c_str());
                }
            }
        }
        pruefe(doppelt == 0, "keine zwei Wartezeiten uebereinander (ein Ablauf wartet nur einmal)");
    }

    std::printf("Darsteller\n");
    const spuren::Darsteller* d = nullptr;
    for (const spuren::Darsteller& x : s.darsteller) {
        if (x.name == "brakk") { d = &x; }
    }
    pruefe(d != nullptr, "brakk ist ein Darsteller");
    if (d != nullptr) {
        pruefe(d->anzeige == "Tusken chief", "lesbarer Name aus NPC_type");
        pruefe(d->sichtbarIrgendwann && d->unsichtbar.clips.empty(), "die ganze Zeit sichtbar");
        pruefe(d->laufen.clips.size() == 1 && d->laufen.clips[0].art == spuren::Art::Laufen && d->laufen.clips[0].nummer == 1,
               "ein Lauf, Nummer 1");
        double dowaitEnde = -1.0;
        for (const spuren::Clip& c : d->befehle.clips) {
            if (c.art == spuren::Art::Warten && c.warteArt == spuren::WarteArt::Aufgabe && c.wartetAuf == "rede") {
                dowaitEnde = c.endMs;
            }
        }
        pruefe(dowaitEnde > 0.0, "dowait wartet auf die Aufgabe \"rede\"");
        if (!d->laufen.clips.empty() && !s.sprache.clips.empty()) {
            const spuren::Clip& l = d->laufen.clips[0];
            // dowait wartet auf CHAN_VOICE (TID_CHAN_VOICE) - der Lauf beginnt nach dem Satz.
            pruefe(l.startMs >= s.sprache.clips[0].endMs - 0.5, "der Lauf beginnt nach dem Satz (dowait)");
            pruefe(l.text == "duene" && nah(l.nach[0], 380.0, 40.0), "Lauf zur Marke duene");
            pruefe(!l.laengeZiehbar && l.verschiebbar, "Lauf: verschiebbar, nicht in der Laenge");
            bool vorher = false;
            for (const spuren::Clip& c : d->stehen.clips) { vorher = vorher || (nah(c.startMs, 0.0) && nah(c.endMs, l.startMs)); }
            pruefe(vorher, "steht still bis zum Lauf");
        }
        pruefe(d->animation.clips.size() == 1 && d->animation.clips[0].text == "BOTH_GESTURE1", "Animation in der Detailzeile");
        if (!d->animation.clips.empty()) {
            pruefe(nah(d->animation.clips[0].endMs - d->animation.clips[0].startMs, kGesteMs, 51.0), "Animation so lang wie ihre Haltezeit");
        }
        pruefe(d->oben.clips.size() == 1, "aussagekraeftige Animation in der Uebersicht");
    }

    std::printf("Schnitt in einer Fahrt\n");
    {
        Script sk;
        std::vector<Diag> dg;
        (void)readScript("camera ( ENABLE );\ncamera ( MOVE, < 0 0 100 >, 3000 );\nwait ( 500 );\n"
                         "camera ( MOVE, < 200 0 100 >, 0 );\nwait ( 500 );\ncamera ( MOVE, < 400 0 100 >, 2000 );\n"
                         "camera ( PAN, < 0 90 0 >, < 0 0 0 >, 2000 );\nwait ( 500 );\ncamera ( PAN, < 0 0 0 >, < 0 0 0 >, 0 );\n",
                         sk, dg);
        const Ablauf a2 = simuliereAblauf(sk, "", AblaufWelt{});
        const CameraTrack k2 = buildCameraTrack(a2.flach);
        const Timeline t2 = buildTimeline(a2.flach, db);
        const Scene sz2 = buildScene(a2.flach, MapData{});
        spuren::SpurenQuellen q2;
        q2.zeitleiste = &t2;
        q2.kamera = &k2;
        q2.szene = &sz2;
        q2.ablauf = &a2;
        const spuren::Spuren s2 = spuren::baue(q2);
        int springt = 0;
        for (const spuren::Clip& c : s2.fahrt.clips) { springt += c.springt ? 1 : 0; }
        for (const spuren::Clip& c : s2.schwenk.clips) { springt += c.springt ? 1 : 0; }
        // Der Schnitt bei 550 ms bricht die Fahrt ab (CGCam_Move: info_state &= ~CAMERA_MOVING,
        // CGCam_SetPosition) - die Fahrt danach (1100 ms) beginnt vom Schnitt aus. Ebenso PAN mit 0.
        pruefe(springt == 0 && s2.ueberlappungen == 0, "ein Schnitt mitten in einer Fahrt springt nicht zurueck");
    }

    std::printf("Fremdes Skript\n");
    {
        // Zeitleiste 2 Punkt 3: ein Befehl aus einem Skript, das per run laeuft,
        // traegt Skriptname und Zeile dort, der Text kommt aus jenem Skript.
        Script haupt;
        Script fremd;
        std::vector<Diag> dg;
        (void)readScript("camera ( ENABLE );\nrun ( \"mini/fremd\" );\nwait ( 2000 );\n", haupt, dg);
        (void)readScript("wait ( 500 );\ncamera ( MOVE, < 200 0 100 >, 1000 );\n", fremd, dg);
        const auto finde = [&fremd](const std::string& n) -> const Script* { return n == "mini/fremd" ? &fremd : nullptr; };
        AblaufWelt w3;
        w3.skript = finde;
        const Ablauf a3 = simuliereAblauf(haupt, "", w3);
        CameraTrack k3 = buildCameraTrack(a3.flach);
        const Timeline t3 = buildTimeline(a3.flach, db);
        const Scene sz3 = buildScene(a3.flach, MapData{});
        for (CamSegment& sg : k3.segments) {   // wie pfadeAufsSkript in der Oberflaeche
            const AblaufHerkunft* h = a3.woher(sg.path);
            if (h != nullptr && h->skript > 0) {
                sg.fremd = a3.skripte[static_cast<std::size_t>(h->skript)];
                sg.fremdPfad = h->path;
                sg.path.clear();
            }
        }
        spuren::SpurenQuellen q3;
        q3.zeitleiste = &t3;
        q3.kamera = &k3;
        q3.szene = &sz3;
        q3.ablauf = &a3;
        q3.skript = &haupt;
        q3.fremdesSkript = finde;
        const spuren::Spuren s3 = spuren::baue(q3);
        bool gefunden = false;
        for (const spuren::Clip& c : s3.fahrt.clips) {
            std::printf("  fahrt %.0f-%.0f fremd \"%s\" Pfadlaenge %zu Befehl \"%s\"\n", c.startMs, c.endMs, c.fremd.c_str(), c.fremdPfad.size(),
                        c.befehl.c_str());
            gefunden = gefunden || (c.fremd == "mini/fremd" && c.fremdPfad == Path{1} && c.pfad.empty() && !c.verschiebbar &&
                                    c.befehl.find("MOVE") != std::string::npos);
        }
        pruefe(gefunden, "Befehl aus einem fremden Skript: Skript, Zeile dort, Befehlstext, nicht ziehbar");
    }

    std::printf("Untertitel: SET_USE_SUBTITLES und SET_CAPTIONTEXTCOLOR\n");
    {
        // Zeitleiste 2 Punkt 7: der NPC brakk mit SET_USE_SUBTITLES "true" fuer
        // den zweiten Satz, nicht fuer den ersten; die Farbe ab 500 ms gelb.
        Script sk;
        std::vector<Diag> dg;
        (void)readScript("affect ( \"brakk\", FLUSH )\n{\n"
                         "\tsound ( CHAN_VOICE, \"sound/chars/mini/brakk01.mp3\" );\n"
                         "\twait ( 1000 );\n"
                         "\tset ( \"SET_USE_SUBTITLES\", \"true\" );\n"
                         "\tsound ( CHAN_VOICE, \"sound/chars/mini/brakk02.mp3\" );\n"
                         "}\n"
                         "wait ( 500 );\n"
                         "set ( \"SET_CAPTIONTEXTCOLOR\", \"YELLOW\" );\n",
                         sk, dg);
        const Ablauf a4 = simuliereAblauf(sk, "", welt);
        const CameraTrack k4 = buildCameraTrack(a4.flach);
        const Timeline t4 = buildTimeline(a4.flach, db);
        const Scene sz4 = buildScene(a4.flach, karte, &hilfe);
        spuren::SpurenQuellen q4;
        q4.zeitleiste = &t4;
        q4.kamera = &k4;
        q4.szene = &sz4;
        q4.ablauf = &a4;
        const spuren::Spuren s4 = spuren::baue(q4);
        std::vector<bool> immer;
        for (const spuren::Clip& c : s4.sprache.clips) {
            std::printf("  Satz %s bei %.0f ms, immer %d\n", c.datei.c_str(), c.startMs, c.untertitelImmer ? 1 : 0);
            immer.push_back(c.untertitelImmer);
        }
        pruefe(immer.size() == 2 && !immer[0] && immer[1], "SET_USE_SUBTITLES gilt ab dem set, nur fuer den NPC");
        pruefe(s4.untertitelFarben.size() == 1 && s4.untertitelFarben[0].second == "YELLOW" && nah(s4.untertitelFarben[0].first, 550.0, 1.0),
               "SET_CAPTIONTEXTCOLOR mit Zeit und Farbe");
    }

    std::printf("wait mit Namen\n");
    {
        // Zeitleiste 4 Punkt 4: wait ( "name" ) wartet auf die Taskgruppe (CTaskManager::Wait) -
        // wie dowait, nicht auf ein Signal.
        Script sk;
        std::vector<Diag> dg;
        (void)readScript("affect ( \"brakk\", FLUSH )\n{\n\ttask ( \"rede\" )\n\t{\n\t\tsound ( CHAN_VOICE, \"sound/chars/mini/brakk01.mp3\" );\n\t}\n"
                         "\tdo ( \"rede\" );\n\twait ( \"rede\" );\n\tprint ( \"da\" );\n}\n",
                         sk, dg);
        AblaufWelt welt5;
        welt5.karte = &karte;
        welt5.klangDauer = [](const std::string&) { return kKlangMs; };
        const Ablauf a5 = simuliereAblauf(sk, "", welt5);
        const CameraTrack k5 = buildCameraTrack(a5.flach);
        Timeline t5 = buildTimeline(a5.flach, db);
        const Scene sz5 = buildScene(a5.flach, karte);
        for (TimelineTrack& tr : t5.tracks) {
            for (TimelineEvent& e : tr.events) {
                const AblaufHerkunft* h = a5.woher(e.path);
                if (h == nullptr || h->skript != 0) {
                    e.path.clear();
                } else {
                    e.path = h->path;
                }
            }
        }
        spuren::SpurenQuellen q5;
        q5.zeitleiste = &t5;
        q5.kamera = &k5;
        q5.szene = &sz5;
        q5.ablauf = &a5;
        q5.skript = &sk;
        const spuren::Spuren s5 = spuren::baue(q5);
        const spuren::Clip* w5 = nullptr;
        for (const spuren::Darsteller& x : s5.darsteller) {
            for (const spuren::Clip& c : x.befehle.clips) {
                if (c.art == spuren::Art::Warten && c.befehl.rfind("wait", 0) == 0) { w5 = &c; }
            }
        }
        if (w5 != nullptr) {
            std::printf("  %s: Art %d auf \"%s\", %.0f-%.0f\n", w5->befehl.c_str(), static_cast<int>(w5->warteArt), w5->wartetAuf.c_str(),
                        w5->startMs, w5->endMs);
        }
        pruefe(w5 != nullptr && w5->warteArt == spuren::WarteArt::Aufgabe && w5->wartetAuf == "rede" && w5->sender.empty(),
               "wait ( \"rede\" ) wartet auf die Aufgabe rede, kein Signal");
        pruefe(w5 != nullptr && nah(w5->endMs, kKlangMs, 60.0), "wait ( \"rede\" ) endet mit der Stimme");
    }

    std::printf("Hilfen\n");
    pruefe(spuren::anzeigeName("md_jedi_male", "x") == "Jedi male" && spuren::anzeigeName("", "kyle") == "kyle", "anzeigeName");
    {
        spuren::Zeile z;
        for (const double a : {0.0, 500.0, 1000.0}) {
            spuren::Clip c;
            c.startMs = a;
            c.endMs = a + 800.0;
            z.clips.push_back(c);
        }
        spuren::bahnenVerteilen(z, 0.0);
        pruefe(z.bahnen == 2 && z.clips[0].bahn == 0 && z.clips[1].bahn == 1 && z.clips[2].bahn == 0, "Unterbahnen verteilt");
    }
    {
        const std::vector<spuren::Knick> k{{0.0, 0.0F}, {1000.0, 10.0F}, {1000.0, 20.0F}, {2000.0, 20.0F}};
        pruefe(nah(spuren::kurveBei(k, 500.0), 5.0, 1e-4) && nah(spuren::kurveBei(k, 1000.0), 20.0, 1e-4), "kurveBei: linear und Sprung");
    }

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
