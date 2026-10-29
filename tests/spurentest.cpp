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

#include <cmath>
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
