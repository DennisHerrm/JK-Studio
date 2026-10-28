// app_view3d.cpp - Karte, Modell und Zeitleiste
//
// Herausgeloest aus app.cpp, das 5138 Zeilen lang geworden war. Diese Datei
// haelt alles, was mit den beiden 3D-Ansichten und der Zeitleiste zu tun
// hat: Karte zeichnen, Kamera-Gizmos, Modelle samt Skelett, die Kamerabahn
// und ihre Bedienung.
//
// Die Trennung folgt der Zustaendigkeit, nicht der Zeilenzahl: was hier
// steht, braucht die Grafikschnittstelle und die Formatleser; was in
// app.cpp bleibt, ist Baum, Dialoge und Dateiverwaltung.
#include "app_internal.h"

#include "gpumap.h"

#include "bhed/messpunkt.h"
#include "bhed/kartenpruefung.h"   // rc604 fix604b (B3): imSpiel
#include "bhed/regelwerk.h"        // rc604 (A2 Rest): roffNotizenOpenJk
#include "bhed/arbeiter.h"
#include "bhed/gpucam.h"
#include "bhed/grasflaechen.h"   // rc604 surfaceSprites
#include "bhed/gpustate.h"
#include "bhed/gpuskin.h"
#include "bhed/num.h"

#include <chrono>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <cstdint>
#include <cstdio>
#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <set>
#include <string>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>
#include "bhed/utf8pfad.h"

namespace bhed::gui {

// KEINE anonyme Namensgruppe um das Ganze: die Zeichenfunktionen werden aus
// app.cpp aufgerufen und brauchen aeussere Bindung. Was nur hier gebraucht
// wird, steht weiter unten in einer eigenen anonymen Gruppe.

// --- Die MOVE-Schluessel des Skripts ------------------------------------
//
// EINE Liste fuer Anzeige, Anklicken und Zurueckschreiben. Vorher war
// App::selectedKey mal eine Nummer in dieser Liste (Zeichnen) und mal eine in
// App::keyMarks (Zurueckschreiben, Anklicken) - und keyMarks enthielt nur die
// Schluessel, die gerade IM BILD lagen. Lag ein frueherer hinter der Kamera,
// verrutschten die Nummern: "write to script" schrieb in die falsche Zeile
// oder gar nicht, und das Gizmo sass an einem anderen Schluessel als dem
// angeklickten. Beim Abspielen und beim Blick durch die Kamera war keyMarks
// ganz leer - dann ging Zurueckschreiben ueberhaupt nicht.
//
// Die Winkel: das erste PAN nach dem MOVE, sonst das zuletzt gesehene.
std::vector<KameraSchluessel> kameraSchluessel(const Script& script) {
    std::vector<KameraSchluessel> aus;
    KameraSchluessel current;
    bool hatPos = false;
    float letzteAng[3] = {0.0F, 0.0F, 0.0F};
    bool hatteAng = false;
    const auto abschliessen = [&]() {
        if (!hatPos) {
            return;
        }
        if (!current.hasAng && hatteAng) {
            for (int k = 0; k < 3; ++k) {
                current.ang[k] = letzteAng[k];
            }
            current.hasAng = true;
        }
        aus.push_back(current);
    };
    std::function<void(const std::vector<Node>&, Path&)> walk =
        [&](const std::vector<Node>& nodes, Path& prefix) {
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                const Node& n = nodes[i];
                prefix.push_back(i);
                if (n.name == "camera" && n.args.size() >= 2) {
                    const std::string& type = n.args[0].text;
                    float v[3] = {0, 0, 0};
                    // Eine Marke - tag( "cam1b", ORIGIN ) - steht dort, wo
                    // der ref_tag in der Karte steht (md_ga_jedi: jede
                    // Einstellung). Vorher las das hier 0 0 0, und Gizmo und
                    // Inspektor sassen im Boden.
                    std::string markenName;
                    bool markeWinkel = false;
                    const bool istMarke = kameraMarke(n.args[1].text, markenName, markeWinkel);
                    if (istMarke) {
                        float o[3] = {0, 0, 0};
                        float a[3] = {0, 0, 0};
                        if (kameraMarkeOrt(markenName, o, a)) {
                            for (int k = 0; k < 3; ++k) { v[k] = markeWinkel ? a[k] : o[k]; }
                        }
                    } else {
                        std::istringstream is(n.args[1].text);
                        is >> v[0] >> v[1] >> v[2];
                    }
                    if (type == "MOVE") {
                        abschliessen();
                        current = KameraSchluessel{};
                        for (int k = 0; k < 3; ++k) { current.pos[k] = v[k]; }
                        hatPos = true;
                        current.path = prefix;
                        if (istMarke && !markeWinkel) { current.marke = markenName; }
                    } else if (type == "PAN") {
                        if (hatPos && !current.hasAng) {
                            for (int k = 0; k < 3; ++k) {
                                current.ang[k] = v[k];
                            }
                            current.hasAng = true;
                            current.panPath = prefix;
                            if (istMarke && markeWinkel) { current.panMarke = markenName; }
                        }
                        for (int k = 0; k < 3; ++k) {
                            letzteAng[k] = v[k];
                        }
                        hatteAng = true;
                    }
                }
                walk(n.children, prefix);
                prefix.pop_back();
            }
        };
    Path prefix;
    walk(script.nodes, prefix);
    abschliessen();
    return aus;
}

// Der Schluessel, der zur Zeit `ms` gilt: der letzte MOVE, der bis dahin
// begonnen hat - dorthin faehrt die Kamera gerade, oder dort steht sie.
// Vor dem ersten MOVE der erste. -1, wenn es keinen gibt.
int schluesselFuerZeit(double ms) {
    const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
    if (ks.empty()) {
        return -1;
    }
    const CamSegment* bester = nullptr;
    for (const CamSegment& s : g_app->camTrack.segments) {
        if (s.kind != CamSegment::Kind::Move || s.startMs > ms + 0.5) {
            continue;
        }
        if (bester == nullptr || s.startMs >= bester->startMs) {
            bester = &s;
        }
    }
    if (bester == nullptr) {
        return 0;
    }
    for (std::size_t i = 0; i < ks.size(); ++i) {
        if (ks[i].path == bester->path) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

// Die Kameras des Skripts als Gizmo einzeichnen.
//
// Aufgebaut wie eine Zielkamera in 3ds Max: ein Koerper mit Objektiv, davor
// ein Ziel, dazwischen eine Linie, und der Sichtkegel als Pyramide. Ohne das
// sieht man zwar die Karte, aber nicht, was das Skript daraus macht.
//
// Warum ein Ziel und nicht nur eine Richtung: in ICARUS bewegt
// camera ( MOVE, ... ) den Ort und camera ( PAN, ... ) den Blick - zwei
// getrennte Befehle mit eigenen Dauern. Genau das ist eine Zielkamera:
// Koerper und Ziel lassen sich unabhaengig bewegen.
//
// Gezeichnet wird direkt ins fertige Bild, mit derselben Projektion wie die
// Karte. Ein eigener Zeichenweg waere doppelte Arbeit fuer ein paar Linien.
// Die Tiefe der Ansicht, waehrend die Gizmos gezeichnet werden (rc594,
// gpu::tiefeAbbilden). Ohne Daten: alles obenauf.
gpu::TiefenSicht g_gizmoTiefe;

void drawCameraGizmos(MapImage& img) {
    BHED_MESS("Gizmos");
    // Gizmo und Schluesselmarken gelten nur fuer DIESES Bild. Gesetzt werden
    // sie nur, wenn die Kamera gewaehlt ist - abgewaehlt blieben sie bis rc587
    // als unsichtbare Griffe stehen, und mit "Verschieben" als Vorgabe (rc588)
    // fing ein solcher Geistergriff den naechsten Klick in die Karte ab
    // (Kartentest: "Klick auf die Kamera waehlt sie").
    g_app->gizmoShown = false;
    if (!g_app->cameraSelected) {
        g_app->keyMarks.clear();
    }
    // --- Auch den PUFFER pruefen, nicht nur die Groesse ------------------
    //
    // Hier stand nur `width <= 0 || height <= 0`. Auf dem GPU-Weg fuellt
    // niemand `mapImage.rgba` - renderMap wird uebersprungen -, und dann
    // sind die Masse gesetzt, aber der Puffer ist leer. Diese Funktion
    // schreibt trotzdem hinein.
    //
    // Gemeldet: Absturz NUR beim Bewegen der Kamera oder beim Vollbild.
    // Genau dann werden die Gizmos gezeichnet beziehungsweise die Groesse
    // geaendert. Die Adresse 0x47B810 blieb ueber Programmstarts gleich -
    // ein fester Versatz auf einen Nullzeiger.
    //
    // Dieselbe Luecke wie bei createTexture in rc412: die Masse sagen
    // nichts darueber, ob Speicher dahintersteht.
    const std::size_t noetig = static_cast<std::size_t>(img.width) *
                               static_cast<std::size_t>(img.height) * 4U;
    if (img.width <= 0 || img.height <= 0 || img.rgba.size() < noetig) {
        return;
    }

    float fwd[3];
    float rgt[3];
    float upv[3];
    g_app->cam.forward(fwd);
    g_app->cam.right(rgt);
    g_app->cam.up(upv);
    const float focal = 1.0F / std::tan(g_app->cam.fovY * 3.14159265F / 360.0F);
    const float halfW = static_cast<float>(img.width) * 0.5F;
    const float halfH = static_cast<float>(img.height) * 0.5F;

    struct P2 {
        float x = 0;
        float y = 0;
        // Der Abstand zur Kamera. Gebraucht, damit eine Linie hinter einer
        // Figur auch dahinter bleibt - siehe plot().
        float z = 0;
        bool ok = false;
    };
    auto project = [&](const float p[3]) {
        P2 out;
        const float d[3] = {p[0] - g_app->cam.pos[0], p[1] - g_app->cam.pos[1],
                            p[2] - g_app->cam.pos[2]};
        const float z = d[0] * fwd[0] + d[1] * fwd[1] + d[2] * fwd[2];
        if (z < 1.0F) {
            return out;
        }
        out.z = z;
        const float x = d[0] * rgt[0] + d[1] * rgt[1] + d[2] * rgt[2];
        const float y = d[0] * upv[0] + d[1] * upv[1] + d[2] * upv[2];
        out.x = halfW + x * focal / z * halfH;
        out.y = halfH - y * focal / z * halfH;
        out.ok = true;
        return out;
    };

    // --- Ein Punkt, aber nur wenn nichts davor steht ----------------------
    //
    // Gefragt: "warum werden die Kamera und Entities immer davor gerendert
    // statt dahinter?"
    //
    // Weil hier bisher direkt ins fertige Bild geschrieben wurde, ohne den
    // Tiefenpuffer je anzusehen. Karte und Figuren fuellen ihn beim
    // Zeichnen; die Gizmos kamen danach und uebermalten alles.
    //
    // Sichtbar war das als Kamerakaesten, die durch Waende scheinen, und
    // als Entitykreuze auf Windus Robe.
    //
    // `z` ist der Abstand zur Kamera. Steht dort schon etwas NAEHERES,
    // wird nicht gezeichnet - genau die Regel, nach der auch Karte und
    // Figuren gegeneinander pruefen.
    //
    // GESCHRIEBEN wird die Tiefe nicht: eine Linie ist ein Strich, kein
    // Koerper. Wer sie einträgt, laesst die naechste Linie an ihr
    // abprallen, und zwei sich kreuzende Gizmos loeschen einander Loecher.
    auto plot = [&](int x, int y, float z, std::uint8_t r, std::uint8_t g,
                    std::uint8_t b) {
        if (x < 0 || y < 0 || x >= img.width || y >= img.height) {
            return;
        }
        const std::size_t at = (static_cast<std::size_t>(y) *
                                static_cast<std::size_t>(img.width) +
                                static_cast<std::size_t>(x));
        if (g_gizmoTiefe.daten != nullptr && x < g_gizmoTiefe.breite && y < g_gizmoTiefe.hoehe &&
            z >= gpu::abstandAusTiefe(g_gizmoTiefe.bei(x, y), g_app->gizmoTiefeNah, g_app->gizmoTiefeFern)) {
            return;   // etwas Naeheres steht davor
        }
        img.rgba[at * 4U + 0] = r;
        img.rgba[at * 4U + 1] = g;
        img.rgba[at * 4U + 2] = b;
        img.rgba[at * 4U + 3] = 255;   // fuer die durchsichtige GPU-Ueberlagerung
        int* box = g_app->gizmoBox;
        if (box[2] <= box[0]) {
            box[0] = x;
            box[1] = y;
            box[2] = x + 1;
            box[3] = y + 1;
        } else {
            box[0] = std::min(box[0], x);
            box[1] = std::min(box[1], y);
            box[2] = std::max(box[2], x + 1);
            box[3] = std::max(box[3], y + 1);
        }
    };

    // Die Linie ZUERST auf das Bild beschneiden.
    //
    // Vorher lief die Schleife ueber die ganze Laenge und liess plot() die
    // Punkte ausserhalb wegwerfen. Das ist genau die falsche Reihenfolge:
    // faehrt man auf eine Entity oder eine Kamera zu, werden ihre Linien
    // auf dem Bildschirm riesig, und dann werden bis zu 6000 Punkte je
    // Linie gerechnet, von denen die meisten gar nicht im Bild liegen.
    // Bei rund hundert Entities mit je drei Linien und den Kamerarahmen
    // dazu ist das die Hauptarbeit - und sie faellt genau dann an, wenn
    // man nah herankommt. Deshalb brach die Bildrate beim Zufahren ein.
    //
    // Cohen-Sutherland, das aelteste Verfahren dafuer: jedem Endpunkt
    // einen Vier-Bit-Code geben, der sagt, auf welcher Seite des Fensters
    // er liegt.
    //
    //   beide Codes 0            ganz drin, nichts zu tun
    //   Codes haben ein Bit gemeinsam   beide auf DERSELBEN Seite
    //                            draussen - die Linie kann das Fenster
    //                            nicht schneiden, ganz verwerfen
    //   sonst                    den aeusseren Punkt auf den Rand ziehen
    //                            und von vorn
    //
    // Der billige Teil ist der zweite Fall: er wirft die meisten Linien
    // ohne jede Rechnung weg.
    constexpr int kInside = 0;
    constexpr int kLeft = 1;
    constexpr int kRight = 2;
    constexpr int kBottom = 4;
    constexpr int kTop = 8;
    const auto outcode = [&img](float x, float y) {
        int c = kInside;
        if (x < 0.0F) { c |= kLeft; }
        else if (x > static_cast<float>(img.width - 1)) { c |= kRight; }
        if (y < 0.0F) { c |= kBottom; }
        else if (y > static_cast<float>(img.height - 1)) { c |= kTop; }
        return c;
    };

    auto line = [&](const P2& a0, const P2& b0, std::uint8_t r, std::uint8_t g,
                    std::uint8_t bl, bool thick) {
        if (!a0.ok || !b0.ok) {
            return;
        }
        float x0 = a0.x;
        float y0 = a0.y;
        float x1 = b0.x;
        float y1 = b0.y;
        const auto xmax = static_cast<float>(img.width - 1);
        const auto ymax = static_cast<float>(img.height - 1);
        int c0 = outcode(x0, y0);
        int c1 = outcode(x1, y1);
        bool sichtbar = false;
        for (int guard = 0; guard < 8; ++guard) {
            if ((c0 | c1) == 0) {
                sichtbar = true;
                break;
            }
            if ((c0 & c1) != 0) {
                break;   // beide auf derselben Seite draussen
            }
            const int aus = (c0 != 0) ? c0 : c1;
            float x = 0.0F;
            float y = 0.0F;
            if ((aus & kTop) != 0) {
                x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
                y = ymax;
            } else if ((aus & kBottom) != 0) {
                x = x0 + (x1 - x0) * (0.0F - y0) / (y1 - y0);
                y = 0.0F;
            } else if ((aus & kRight) != 0) {
                y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
                x = xmax;
            } else {
                y = y0 + (y1 - y0) * (0.0F - x0) / (x1 - x0);
                x = 0.0F;
            }
            if (!std::isfinite(x) || !std::isfinite(y)) {
                return;
            }
            if (aus == c0) {
                x0 = x;
                y0 = y;
                c0 = outcode(x0, y0);
            } else {
                x1 = x;
                y1 = y;
                c1 = outcode(x1, y1);
            }
        }
        if (!sichtbar) {
            return;
        }
        const float dx = x1 - x0;
        const float dy = y1 - y0;
        const int steps = static_cast<int>(std::max(std::fabs(dx), std::fabs(dy)));
        if (steps <= 0) {
            plot(static_cast<int>(x0), static_cast<int>(y0), a0.z, r, g, bl);
            return;
        }
        for (int i = 0; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const int px = static_cast<int>(x0 + dx * t);
            const int py = static_cast<int>(y0 + dy * t);
            // Die Tiefe laeuft mit. Ohne sie waere eine Linie entweder ganz
            // sichtbar oder ganz verdeckt - eine, die halb hinter einer
            // Saeule verschwindet, gibt es dann nicht.
            //
            // Linear zwischen den Endpunkten, nicht perspektivisch korrekt:
            // fuer eine Linie, die auf ein paar Bildpunkte genau stimmen
            // soll, waere das Aufwand ohne sichtbaren Gewinn.
            const float z = a0.z + (b0.z - a0.z) * t;
            plot(px, py, z, r, g, bl);
            if (thick) {
                plot(px + 1, py, z, r, g, bl);
                plot(px, py + 1, z, r, g, bl);
            }
        }
    };

    // Alle camera-Befehle einsammeln. MOVE gibt den Ort, das folgende PAN
    // die Blickrichtung.
    // Die MOVE-Schluessel des Skripts - dieselbe Liste und Reihenfolge, die
    // auch das Zurueckschreiben benutzt (kameraSchluessel).
    struct Shot {
        float pos[3]{};
        float ang[3]{};
        bool hasPos = false;
        bool hasAng = false;
        Path path;
        // Paket kamera, rc601: der Bildwinkel der Einstellung (ZOOM) fuer
        // die Sichtpyramide - CAMERA_DEFAULT_FOV, wenn die Bahn nichts weiss.
        float fovX = 90.0F;
    };
    std::vector<Shot> shots;
    for (const KameraSchluessel& ks : kameraSchluessel(g_app->doc.script())) {
        Shot sh;
        for (int k = 0; k < 3; ++k) {
            sh.pos[k] = ks.pos[k];
            sh.ang[k] = ks.ang[k];
        }
        sh.hasPos = true;
        sh.hasAng = ks.hasAng;
        sh.path = ks.path;
        if (g_app->camTrackValid) {
            const double t = einstellungsZeit(g_app->camTrack, ks.path);
            if (t >= 0.0) { sh.fovX = g_app->camTrack.at(t).fovX; }
        }
        shots.push_back(sh);
    }
    // Das Spielfenster fuer die Sichtpyramiden (Paket kamera, rc601).
    const SpielFenster pyramidenSpiel = spielFensterJetzt();

    // Die Groesse haengt vom Abstand ab, sonst ist das Gizmo aus der Ferne
    // ein Punkt und aus der Naehe bildfuellend.
    auto sizeAt = [&](const float p[3]) {
        float d = 0.0F;
        for (int k = 0; k < 3; ++k) {
            const float t = p[k] - g_app->cam.pos[k];
            d += t * t;
        }
        return std::clamp(std::sqrt(d) * 0.035F, 6.0F, 220.0F);
    };

    // --- Das Kameramodell an einem Ort mit einer Blickrichtung ---------
    //
    // Herausgeloest, weil es an ZWEI Stellen gebraucht wird: fuer die
    // laufende Kamera, und fuer die Vorschau am Gizmo. Vorher stand es nur
    // an der ersten - deshalb blieb die Kamera beim Ziehen und Drehen
    // stehen ("wenn ich sie bewege und rotiere bewegt und rotiert sich ja
    // die kamera nicht wirklich"). Das Gizmo wanderte, das Modell nicht.
    //
    // mitAngaben: Zielkreuz und Sichtkegel dazu. Die laufende Kamera zeigt
    // sie nur, wenn sie ueberhaupt eine Blickrichtung hat.
    auto zeichneKamera = [&](const float pos[3], const float ang[3],
                             std::uint8_t r, std::uint8_t g, std::uint8_t b,
                             bool betont, bool mitAngaben, float fovX) {
        Camera shot;
        for (int i = 0; i < 3; ++i) {
            shot.pos[i] = pos[i];
            shot.angles[i] = ang[i];
        }
        float f[3];
        float rr[3];
        float uu[3];
        shot.forward(f);
        shot.right(rr);
        shot.up(uu);

        const float S = sizeAt(pos);
        auto at = [&](float df, float dr, float du, float out[3]) {
            for (int i = 0; i < 3; ++i) {
                out[i] = pos[i] + f[i] * df + rr[i] * dr + uu[i] * du;
            }
        };

        // --- Koerper: ein Quader hinter dem Objektiv --------------------
        const float bw = S * 0.45F;
        const float bh = S * 0.38F;
        const float bl = S * 0.75F;
        P2 c[8];
        int idx = 0;
        for (int zi = 0; zi < 2; ++zi) {
            for (int yi = 0; yi < 2; ++yi) {
                for (int xi = 0; xi < 2; ++xi) {
                    float pp[3];
                    at(zi == 0 ? 0.0F : -bl, (xi == 0 ? -bw : bw),
                       (yi == 0 ? -bh : bh), pp);
                    c[idx++] = project(pp);
                }
            }
        }
        const int edges[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0},
                                  {4, 5}, {5, 7}, {7, 6}, {6, 4},
                                  {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto& e : edges) {
            line(c[e[0]], c[e[1]], r, g, b, betont);
        }

        // --- Objektiv: ein kurzer Kegel nach vorn ------------------------
        float tipP[3];
        at(S * 0.5F, 0.0F, 0.0F, tipP);
        const P2 tip = project(tipP);
        for (int e = 0; e < 4; ++e) {
            line(c[e], tip, r, g, b, false);
        }
        if (!mitAngaben) {
            return;
        }

        // --- Ziel und Verbindungslinie ----------------------------------
        const float kTargetDist = S * 9.0F;
        const float kConeDist = S * 5.0F;
        float tg[3];
        for (int i = 0; i < 3; ++i) {
            tg[i] = pos[i] + f[i] * kTargetDist;
        }
        const P2 target = project(tg);
        const P2 centre = project(pos);
        line(centre, target, r, g, b, false);

        const float ts = sizeAt(tg) * 0.35F;
        float t1[3];
        float t2[3];
        float t3[3];
        float t4[3];
        for (int i = 0; i < 3; ++i) {
            t1[i] = tg[i] - rr[i] * ts;
            t2[i] = tg[i] + rr[i] * ts;
            t3[i] = tg[i] - uu[i] * ts;
            t4[i] = tg[i] + uu[i] * ts;
        }
        line(project(t1), project(t2), r, g, b, betont);
        line(project(t3), project(t4), r, g, b, betont);

        // --- Sichtkegel als Pyramide ------------------------------------
        //
        // Paket kamera, rc601: so weit offen wie das Spielbild dieser
        // Einstellung - halbe Breite tan(fov_x/2), halbe Hoehe tan(fov_y/2)
        // im Spielfenster (CG_CalcFOVFromX, R_SetupProjection). Vorher eine
        // feste Pyramide (0,36 und 0,27, also 39,6 x 30,2 Grad, "etwa 80
        // Grad" stand daneben), unabhaengig vom ZOOM.
        const float halbX = spielTanHalbX(fovX, pyramidenSpiel);
        const float halbY = spielTanHalbY(fovX, pyramidenSpiel);
        P2 corner[4];
        for (int q = 0; q < 4; ++q) {
            const float sxp = (q == 0 || q == 3) ? -1.0F : 1.0F;
            const float syp = (q < 2) ? 1.0F : -1.0F;
            float pp[3];
            for (int i = 0; i < 3; ++i) {
                pp[i] = pos[i] + f[i] * kConeDist +
                        rr[i] * (sxp * kConeDist * halbX) +
                        uu[i] * (syp * kConeDist * halbY);
            }
            corner[q] = project(pp);
            line(centre, corner[q], r, g, b, false);
        }
        for (int q = 0; q < 4; ++q) {
            line(corner[q], corner[(q + 1) % 4], r, g, b, false);
        }
    };

    // --- Entities der Karte -------------------------------------------
    //
    // Nicht alle, sondern die, deren Namen im Skript auftauchen:
    //   waypoint_navgoal   -> SET_NAVGOAL
    //   target_position    -> SET_LOOK_TARGET
    //   info_null          -> Blickziele und Lichtrichtungen
    //   target_scriptrunner-> startet ein Skript, per usescript
    // In md_twj_jedi.ent sind das 26 Wegpunkte, drei Blickziele und sechs
    // Skriptstarter - genau die Werte, die man beim Schreiben braucht.
    // Die Marken werden beim Zeichnen gemerkt, damit der Klick sie
    // wiederfindet. Neu aufbauen, sobald gezeichnet wird - und AUSSERHALB
    // des Schalters leeren: mit "Entities" aus blieben sonst die alten
    // Stellen stehen, und ein Klick ins scheinbar Leere waehlte eine
    // unsichtbare Entity (Kartentest 27.09.).
    g_app->markers.clear();
    // Die Verdeckt-Strahlen erst nach der Schleife, auf alle Kerne verteilt
    // (rc594): je sichtbarer Entity ein Strahl quer durch die Karte, das war
    // beim Fliegen ueber die Karte bis zur Haelfte der Bildzeit. Das
    // Ergebnis ist dasselbe - `verdeckt` liest erst entityUeberlagerung.
    struct VerdecktStrahl {
        std::size_t marke;
        float ziel[3];
    };
    std::vector<VerdecktStrahl> strahlen;
    if (g_app->showEntities) {
        for (std::size_t ei = 0; ei < g_app->map.entities.size(); ++ei) {
            const MapEntity& e = g_app->map.entities[ei];
            if (e.origin.empty()) {
                continue;
            }
            // Dieselben Gruppen und Farben wie in der Entity-Liste links.
            const int kat = entityKategorie(e);
            if (kat < 0) {
                continue;
            }
            const EntityKat& ek = entityKategorieInfo(kat);
            const std::uint8_t er = ek.r;
            const std::uint8_t eg = ek.g;
            const std::uint8_t eb = ek.b;

            // Ausgeblendete werden nicht gezeichnet - und damit auch nicht
            // angeklickt, denn die Marken entstehen hier.
            if (!g_app->entityVisible(ei, e.classname)) {
                continue;
            }

            // Der Ursprung, je Entity gemerkt und nur neu gelesen, wenn sich
            // sein Text geaendert hat (rc594) - ein istringstream je Entity
            // und Bild kostete beim Fliegen messbar.
            struct Ursprung {
                bool gelesen = false;
                std::string text;
                float p[3] = {0.0F, 0.0F, 0.0F};
            };
            static std::vector<Ursprung> ursprung;
            if (ursprung.size() != g_app->map.entities.size()) {
                ursprung.assign(g_app->map.entities.size(), Ursprung{});
            }
            Ursprung& merk = ursprung[ei];
            if (!merk.gelesen || merk.text != e.origin) {
                float q[3] = {0, 0, 0};
                std::istringstream is(e.origin);
                is >> q[0] >> q[1] >> q[2];
                merk.gelesen = true;
                merk.text = e.origin;
                for (int k = 0; k < 3; ++k) { merk.p[k] = q[k]; }
            }
            float p[3] = {merk.p[0], merk.p[1], merk.p[2]};
            {
                float v[3];
                entityVersatz(static_cast<int>(ei), v);
                for (int k = 0; k < 3; ++k) { p[k] += v[k]; }
            }
            // Draufsicht: was ueber dem Schnitt liegt, ist nicht zu sehen.
            if (g_app->draufsicht && p[2] - 24.0F > g_app->draufSchnitt) {
                continue;
            }
            // Wegpunkte gibt es zu Hunderten - nur mit sichtbarem Wegnetz.
            if (kat == 9 && !g_app->zeigeWegnetz) {
                continue;
            }
            const P2 c0 = project(p);
            if (!c0.ok) {
                continue;
            }
            App::PickedMarker marke{c0.x, c0.y, ei};
            // Wegpunkte (9) bekommen kein Schild, `verdeckt` liest dort
            // niemand - der Strahl je Wegpunkt war vergebens.
            if (!g_app->geo.empty() && kat != 9) {
                strahlen.push_back({g_app->markers.size(), {p[0], p[1], p[2] + 8.0F}});
            }
            g_app->markers.push_back(marke);
            const float es = sizeAt(p) * 0.30F;
            float a[3];
            float bq[3];
            for (int i = 0; i < 3; ++i) {
                a[i] = p[i];
                bq[i] = p[i];
            }
            a[2] -= es;
            bq[2] += es;
            line(project(a), project(bq), er, eg, eb, false);
            // Ein liegendes Kreuz dazu, damit es von oben sichtbar bleibt.
            float l1[3] = {p[0] - es, p[1], p[2]};
            float l2[3] = {p[0] + es, p[1], p[2]};
            float l3[3] = {p[0], p[1] - es, p[2]};
            float l4[3] = {p[0], p[1] + es, p[2]};
            line(project(l1), project(l2), er, eg, eb, false);
            line(project(l3), project(l4), er, eg, eb, false);
        }
    }
    if (!strahlen.empty()) {
        const float* kp = g_app->gezeigteKamera.pos;
        spurenVerteilt(strahlen.size(), [&](std::size_t i) {
            const TraceTreffer tt = traceRay(g_app->geo, kp, strahlen[i].ziel);
            g_app->markers[strahlen[i].marke].verdeckt = tt.hit && tt.fraction < 0.97F;
        });
    }

    // --- Wo die Figuren gerade stehen (fuer ihre Namen) ------------------
    g_app->figurNamen.clear();
    if (g_app->zeigeNamen && g_app->showActors) {
        for (const Actor& a : g_app->scene.actors) {
            if (!a.haveStart || a.brushModel > 0) {
                continue;
            }
            const ActorState st = a.at(g_app->playMs);
            if (!st.visible) {
                continue;
            }
            float kopf[3] = {st.pos[0], st.pos[1], st.pos[2] + 48.0F};
            const P2 c0 = project(kopf);
            if (c0.ok) {
                g_app->figurNamen.push_back(App::NamensMarke{c0.x, c0.y, a.name});
            }
        }
    }

    // --- Die BAHN der Kamera ---------------------------------------------
    //
    // Die magenta Marken sind Stuetzstellen: dort steht im Skript ein MOVE.
    // Was DAZWISCHEN passiert, sagt keine von ihnen - die Kamera faehrt
    // eine Strecke ab, und wie sie das tut, entscheidet die Dauer im
    // Befehl, nicht der Abstand der Punkte.
    //
    // Deshalb eine duenne Linie entlang der ganzen Fahrt, in kleinen
    // Schritten abgetastet. Dieselbe Rechnung wie bei "Durch die Kamera"
    // und bei der gruenen Marke - drei Darstellungen, eine Quelle.
    //
    // Zweihundert Schritte: genug, dass eine Kurve rund aussieht, und
    // wenig genug, dass es bei einer Minute Sequenz nicht ins Gewicht
    // faellt. Die Linien werden ohnehin am Bildrand beschnitten (rc137).
    // --- Wo stehen die Figuren auf dem Bildschirm? ------------------------
    //
    // Hier, weil hier project() zu Hause ist. Der Klick liest die Liste
    // spaeter ab - dieselbe Arbeitsteilung wie bei der Kamera (cameraSx)
    // und den Ringen des Gizmos.
    //
    // Gezielt wird auf Brusthoehe, nicht auf die Fuesse: dorthin klickt
    // man, wenn man eine Figur meint.
    g_app->actorMarks.clear();
    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        const Actor& fa = g_app->scene.actors[ai];
        if (!fa.haveStart) {
            continue;
        }
        const ActorState fs = fa.at(g_app->playMs);
        if (!fs.visible) {
            continue;
        }
        float brust[3] = {fs.pos[0], fs.pos[1], fs.pos[2] + 40.0F};
        if (g_app->entZug.aktiv) {
            float zv[3];
            float zg = 0.0F;
            if (zugVersatz(entityFuerFigur(static_cast<int>(ai)), zv, zg)) {
                for (int k = 0; k < 3; ++k) { brust[k] += zv[k]; }
            }
        }
        const P2 mk = project(brust);
        if (mk.ok) {
            g_app->actorMarks.push_back(
                App::ActorMark{static_cast<int>(ai), mk.x, mk.y});
        }
    }

    // --- Der Laufweg der angewaehlten Figur -------------------------------
    //
    // Dieselbe Rechnung wie bei der Kamerabahn, nur mit Actor::at() statt
    // CameraTrack::at(): in kleinen Schritten abtasten und verbinden. Damit
    // sieht man, WO eine Figur entlanggeht - und weil die Quelle dieselbe
    // ist wie beim Abspielen, stimmt die Linie mit der Bewegung ueberein.
    //
    // Nur bei ausgewaehlter Figur, genau wie in 3ds Max: dort erscheint
    // die Trajektorie erst, wenn das Objekt gewaehlt ist.
    //
    // rc597: auch fuer die gewaehlte ENTITY (Spawner, Schiff, Tuer) - die
    // ganze Bahn grau, was bis zur eingestellten Zeit schon gefahren ist,
    // gelb. "es sollte auch die end linie zeigen nur grau und die wo es
    // bereits geflogen ist in gelb".
    g_app->bahnFarbig = 0;
    g_app->bahnGrau = 0;
    {
        const int gewEnt = g_app->pickedEntity;
        int bahnFig = g_app->selectedActor;
        if ((bahnFig < 0 || static_cast<std::size_t>(bahnFig) >= g_app->scene.actors.size()) && gewEnt >= 0) {
            bahnFig = figurFuerEntity(gewEnt);
        }
        // rc604: eine gewaehlte Figur, die es nach dem Neubau der Szene nicht
        // mehr gibt (Kartenwechsel: 11 Figuren -> 1), und keine Entity
        // gewaehlt - dann keine Bahn. Vorher las scene.actors[bahnFig] hinter
        // dem Ende (Absturz im Selbsttest missionsbau, Zufall 153).
        if (bahnFig >= 0 && static_cast<std::size_t>(bahnFig) >= g_app->scene.actors.size()) { bahnFig = -1; }
        // Eine Tuer o. a. ohne Figur: die Bahn der Mover-Simulation.
        int moverNr = -1;
        if (bahnFig < 0 && gewEnt >= 0 && static_cast<std::size_t>(gewEnt) < g_app->map.entities.size()) {
            if (const std::string* mdl = g_app->map.entities[static_cast<std::size_t>(gewEnt)].find("model");
                mdl != nullptr && mdl->size() > 1 && (*mdl)[0] == '*') {
                const int nr = std::atoi(mdl->c_str() + 1);
                if (g_app->moverSim.at(nr, 0.0).bekannt) { moverNr = nr; }
            }
        }
        float zv[3] = {0.0F, 0.0F, 0.0F};
        float zg = 0.0F;
        const int bahnEnt = bahnFig >= 0 ? (g_app->scene.actors[static_cast<std::size_t>(bahnFig)].brushModel > 0
                                                ? gewEnt
                                                : entityFuerFigur(bahnFig))
                                         : gewEnt;
        if (g_app->entZug.aktiv) { (void)zugVersatz(bahnEnt, zv, zg); }
        const auto ortBei = [&](double ms, float out[3]) {
            if (bahnFig >= 0) {
                const ActorState fst = g_app->scene.actors[static_cast<std::size_t>(bahnFig)].at(ms);
                for (int k = 0; k < 3; ++k) { out[k] = fst.pos[k] + zv[k]; }
                return fst.visible;
            }
            const MoverSim::Stand ms2 = g_app->moverSim.at(moverNr, ms);
            for (int k = 0; k < 3; ++k) { out[k] = ms2.origin[k] + zv[k]; }
            return ms2.bekannt && ms2.sichtbar;
        };
        if (bahnFig >= 0 || moverNr > 0) {
            const double dauer = std::max({g_app->scene.durationMs, g_app->camTrack.durationMs,
                                           g_app->timeline.practicalEndMs, 1.0});
            constexpr int kFigSchritte = 400;
            P2 vorFig;
            bool hatVorFig = false;
            double vorMs = 0.0;
            for (int i = 0; i <= kFigSchritte; ++i) {
                const double ms =
                    dauer * static_cast<double>(i) / static_cast<double>(kFigSchritte);
                float p[3];
                if (!ortBei(ms, p)) {
                    hatVorFig = false;   // Luecke: die Figur ist gerade weg
                    continue;
                }
                const P2 jetzt = project(p);
                if (hatVorFig) {
                    // Gelb, was schon gefahren ist - abgesetzt von der gruenen
                    // Kamerabahn; grau, was noch kommt.
                    if (vorMs < g_app->playMs) {
                        line(vorFig, jetzt, 230, 190, 60, false);
                        ++g_app->bahnFarbig;
                    } else {
                        line(vorFig, jetzt, 140, 144, 155, false);
                        ++g_app->bahnGrau;
                    }
                }
                vorFig = jetzt;
                vorMs = ms;
                hatVorFig = true;
            }
        }
        // Und die STUETZSTELLEN: dort steht ein Befehl im Skript. In Max
        // sind das die weissen Quadrate auf der Bahn.
        if (bahnFig >= 0) {
            const Actor& fig = g_app->scene.actors[static_cast<std::size_t>(bahnFig)];
            for (const ActorStep& sp : fig.steps) {
                float p[3];
                if (!ortBei(sp.startMs, p)) {
                    continue;
                }
                const P2 mp = project(p);
                if (!mp.ok) {
                    continue;
                }
                const int mx = static_cast<int>(mp.x);
                const int my = static_cast<int>(mp.y);
                for (int d = -3; d <= 3; ++d) {
                    plot(mx + d, my - 3, mp.z, 255, 230, 120);
                    plot(mx + d, my + 3, mp.z, 255, 230, 120);
                    plot(mx - 3, my + d, mp.z, 255, 230, 120);
                    plot(mx + 3, my + d, mp.z, 255, 230, 120);
                }
            }
        }
    }

    if (g_app->cameraSelected && g_app->camTrack.durationMs > 1.0) {
        constexpr int kSchritte = 200;
        P2 vorherP;
        bool hatVorher = false;
        for (int i = 0; i <= kSchritte; ++i) {
            const double ms = g_app->camTrack.durationMs *
                              static_cast<double>(i) /
                              static_cast<double>(kSchritte);
            const CameraState st = camStateAt(ms);
            if (!st.enabled) {
                hatVorher = false;   // Luecke: die Kamera ist gerade aus
                continue;
            }
            const P2 jetzt = project(st.pos);
            if (hatVorher) {
                // Gedaempftes Gruen - die Bahn soll die Marken nicht
                // uebertoenen, sondern sie verbinden. Seit rc597 zeichnet
                // kameraBahnZeichnen sie kraeftig obenauf (Alle/Aktiv) -
                // hier nicht mehr doppelt.
                if (false) { line(vorherP, jetzt, 40, 130, 70, false); }
            }
            vorherP = jetzt;
            hatVorher = true;

            // Zwischenpunkte: kleine Punkte in gleichen ZEITabstaenden.
            //
            // Ihr Abstand auf der Linie zeigt die Geschwindigkeit - eng
            // beieinander heisst langsam, weit auseinander heisst schnell.
            // Genau dafuer hat Max sie ("the white dots are in-betweens",
            // "ticks give you an idea of the animation timing").
            if (false && jetzt.ok && (i % 5) == 0) {
                plot(static_cast<int>(jetzt.x), static_cast<int>(jetzt.y),
                     jetzt.z, 170, 255, 190);
            }
        }

        // Die Schluesselstellen: kleine Quadrate, dort steht ein MOVE.
        //
        // In Max sind es weisse Quadrate auf der Bahn. Sie sagen, WO im
        // Skript etwas steht - die Punkte dazwischen sagen, WIE schnell es
        // dorthin geht.
        // Liegt gerade eine Verschiebung als Vorschau an?
        const bool verschobenNun =
            g_app->gizmoOffset[0] != 0.0F || g_app->gizmoOffset[1] != 0.0F ||
            g_app->gizmoOffset[2] != 0.0F;
        g_app->keyMarks.clear();
        // Auto-Key (rc597): mitten in einer Fahrt sitzt der Gizmo an der
        // LAUFENDEN Kamera - Anfassen setzt dort einen neuen Schluessel.
        const int zwischenNr = g_app->cameraSelected ? kameraZwischenSchluesselNr(g_app->playMs) : -1;
        const CameraState zwischenStand = zwischenNr >= 0 ? camStateAt(g_app->playMs) : CameraState{};
        g_app->kameraGizmoZwischen = false;
        // Der Gizmo selbst wird seit rc597 obenauf gezeichnet (wie der der
        // Figuren, app_kamera.cpp kameraBahnZeichnen) - hier nur noch die
        // Lage fuer Ueberfahren und Ziehen.
        constexpr bool kRasterGizmo = false;
        if (g_app->gizmoMode != App::GizmoMode::Move) { g_app->kamYawRingInMove = false; }
        for (std::size_t si2 = 0; si2 < shots.size(); ++si2) {
            const Shot& sh2 = shots[si2];
            const P2 kp = project(sh2.pos);
            // Fuer den Klick merken - samt Skriptzeile. JEDER Schluessel
            // bekommt seinen Platz, auch einer ausserhalb des Bildes: die
            // Nummer in keyMarks muss die in kameraSchluessel sein.
            App::KeyMark mk;
            mk.sx = kp.ok ? kp.x : -1.0e9F;
            mk.sy = kp.ok ? kp.y : -1.0e9F;
            mk.imBild = kp.ok;
            mk.path = sh2.path;
            g_app->keyMarks.push_back(mk);
            if (!kp.ok) {
                continue;
            }

            const bool ausgewaehlt =
                (static_cast<int>(si2) == g_app->selectedKey);
            Shot gz = sh2;
            if (ausgewaehlt && zwischenNr == static_cast<int>(si2)) {
                for (int k = 0; k < 3; ++k) {
                    gz.pos[k] = zwischenStand.pos[k];
                    gz.ang[k] = zwischenStand.angles[k];
                }
                gz.hasAng = true;
                g_app->kameraGizmoZwischen = true;
            }
            const int kx = static_cast<int>(kp.x);
            const int ky = static_cast<int>(kp.y);
            // Der ausgewaehlte Schluessel groesser und gelb - wie in Max
            // die hervorgehobene Stuetzstelle.
            const int rad = ausgewaehlt ? 5 : 3;
            const std::uint8_t kr = 255;
            const std::uint8_t kg = ausgewaehlt ? 210 : 255;
            const std::uint8_t kb = ausgewaehlt ? 60 : 255;
            for (int d = -rad; d <= rad; ++d) {
                plot(kx + d, ky - rad, kp.z, kr, kg, kb);
                plot(kx + d, ky + rad, kp.z, kr, kg, kb);
                plot(kx - rad, ky + d, kp.z, kr, kg, kb);
                plot(kx + rad, ky + d, kp.z, kr, kg, kb);
            }

            // --- Das Gizmo am ausgewaehlten Schluessel -------------------
            //
            // Drei Achsen, wie in 3ds Max: X rot, Y gruen, Z blau. Die
            // Laenge richtet sich nach der Entfernung, damit es aus jedem
            // Abstand gleich gross aussieht - dieselbe Regel wie bei den
            // Kamerarahmen (sizeAt).
            //
            // Die Bildschirmenden werden gemerkt: der Griff braucht sie zum
            // Treffen, und das Ziehen braucht sie, um einen Mausweg in
            // Welteinheiten umzurechnen.
            // Die Kamera geht MIT.
            //
            // Der Einwand war: "dann bewegt sich die kamera nicht wirklich
            // mit". Richtig - in rc153 wanderte nur das Gizmo.
            //
            // Vorwegnehmen laesst sich das genau, weil ein camera(MOVE) in
            // JKA eine GERADE ist (CGCam_MoveUpdate rechnet linear, siehe
            // rc143). Die beiden Abschnitte links und rechts des
            // verschobenen Schluessels sind also einfach zwei Strecken zum
            // neuen Ort - kein Naeherungsverfahren noetig.
            //
            // Gezeichnet in hellem Gruen, damit man die Vorschau von der
            // bestehenden Bahn unterscheiden kann.
            if (ausgewaehlt && verschobenNun) {
                float neu[3];
                for (int k = 0; k < 3; ++k) {
                    neu[k] = gz.pos[k] + g_app->gizmoOffset[k];
                }
                const P2 np = project(neu);
                // Der Weg vom vorigen Schluessel hierher ...
                for (std::size_t vor = si2; vor > 0; --vor) {
                    if (!shots[vor - 1].hasPos) { continue; }
                    line(project(shots[vor - 1].pos), np, 150, 255, 180, false);
                    break;
                }
                // ... und von hier zum naechsten.
                for (std::size_t nach = si2 + 1; nach < shots.size(); ++nach) {
                    if (!shots[nach].hasPos) { continue; }
                    line(np, project(shots[nach].pos), 150, 255, 180, false);
                    break;
                }
                // Und die KAMERA am neuen Ort - nicht nur ein Kaestchen.
                //
                // Ein Quadrat sagt, wo sie steht; das Modell sagt auch,
                // wohin sie blickt. Genau das fehlte: das Gizmo wanderte,
                // die Kamera nicht.
                float vang[3] = {0.0F, 0.0F, 0.0F};
                if (gz.hasAng) {
                    for (int k = 0; k < 3; ++k) {
                        vang[k] = gz.ang[k] + g_app->gizmoAngles[k];
                    }
                }
                // Betont (der vierte Wert), damit die Vorschau vor der weissen
                // Kamera aus dem Skript steht - sie ist das, was man gerade
                // einstellt.
                zeichneKamera(neu, vang, 120, 255, 140, true, gz.hasAng, gz.fovX);
            }

            // --- Drehen: ein Ring je Achse ------------------------------
            //
            // 3ds Max zeichnet drei Kreise um den Ursprung, einen je Achse.
            // Hier dasselbe, nur ohne den freien Trackball - in ICARUS hat
            // eine Kamera drei Winkel (PAN), und die dreht man einzeln.
            //
            // Gezogen wird waagerecht: nach rechts dreht im Uhrzeigersinn.
            // Das ist die Vereinbarung aus jedem Werkzeug, das einen
            // Drehgriff hat, und sie ist unabhaengig vom Blickwinkel - bei
            // einem Ring, der fast von der Seite zu sehen ist, koennte man
            // sonst nicht zielen.
            // Die drei Achsenrichtungen des gewaehlten Bezugssystems.
            //
            // Welt: X, Y, Z der Karte. Lokal: rechts, oben, vorwaerts der
            // Kamera - "as we rotate the object, the rotate gizmo is stuck
            // to the object", wie es in Max heisst.
            auto achsenRichtungen = [&](App::GizmoSpace raum,
                                        float dir[3][3]) {
                if (raum == App::GizmoSpace::World) {
                    for (int a = 0; a < 3; ++a) {
                        for (int k = 0; k < 3; ++k) {
                            dir[a][k] = (a == k) ? 1.0F : 0.0F;
                        }
                    }
                    return;
                }
                Camera hilfL;
                for (int k = 0; k < 3; ++k) {
                    hilfL.pos[k] = gz.pos[k];
                    hilfL.angles[k] =
                        (gz.hasAng ? gz.ang[k] : 0.0F) + g_app->gizmoAngles[k];
                }
                float fL[3];
                float rL[3];
                float uL[3];
                hilfL.forward(fL);
                hilfL.right(rL);
                hilfL.up(uL);
                for (int k = 0; k < 3; ++k) {
                    dir[0][k] = rL[k];
                    dir[1][k] = uL[k];
                    dir[2][k] = fL[k];
                }
            };

            // Beim reinen Drehen gibt es keinen Versatz - der Zweig oben
            // greift also nicht. Trotzdem soll man sehen, wohin sie nach
            // der Drehung blickt.
            const bool gedrehtNun =
                g_app->gizmoAngles[0] != 0.0F ||
                g_app->gizmoAngles[1] != 0.0F ||
                g_app->gizmoAngles[2] != 0.0F;
            if (ausgewaehlt && gedrehtNun && !verschobenNun && gz.hasAng) {
                float vang2[3];
                for (int k = 0; k < 3; ++k) {
                    vang2[k] = gz.ang[k] + g_app->gizmoAngles[k];
                }
                zeichneKamera(gz.pos, vang2, 120, 255, 140, true, true, gz.fovX);
            }

            if (ausgewaehlt) {
                // Fuer den Griff bereitlegen.
                float dnow[3][3];
                achsenRichtungen(g_app->gizmoMode == App::GizmoMode::Rotate
                                     ? g_app->gizmoRotSpace
                                     : g_app->gizmoMoveSpace,
                                 dnow);
                for (int a = 0; a < 3; ++a) {
                    for (int k = 0; k < 3; ++k) {
                        g_app->gizmoDirNow[a][k] = dnow[a][k];
                    }
                }
                for (int k = 0; k < 3; ++k) {
                    g_app->gizmoAngNow[k] = gz.hasAng ? gz.ang[k] : 0.0F;
                }
            }

            if (ausgewaehlt && g_app->gizmoMode == App::GizmoMode::Rotate) {
                // Auch die Ringe sind ein Gizmo im Bild. Vorher setzte nur
                // der Verschieben-Zweig gizmoShown - der Drehmodus hing
                // also davon ab, ob man ZUFAELLIG vorher im Verschieben
                // war. War man es nie, liess sich kein Ring anfassen.
                g_app->gizmoShown = true;
                const float ringRad = sizeAt(gz.pos) * 3.0F;
                float rdir[3][3];
                // WAEHREND DES ZIEHENS bleiben die Ringe stehen.
                //
                // Vorher wurden sie jedes Bild aus gz.ang + gizmoAngles neu
                // gebaut - sie drehten sich also mit der Vorschau mit.
                // Gemeldet als "jetzt rotiert beides mit".
                //
                // In 3ds Max bleibt das Gizmo stehen und ein Tortenstueck
                // zeigt den Betrag: "As you select an axis and drag, an arc
                // is highlighted that shows the distance of the rotation".
                // Genau dafuer braucht es einen STEHENDEN Bezug - ein
                // mitdrehender Ring misst gegen sich selbst und zeigt
                // nichts an.
                //
                // Dasselbe Einfrieren wie in rc156 beim Verschieben, nur
                // dort fuer die Rechnung und hier fuer die Zeichnung.
                if (g_app->gizmoAxis >= 0 && g_app->gizmoAxis < 3) {
                    for (int a = 0; a < 3; ++a) {
                        for (int k = 0; k < 3; ++k) {
                            rdir[a][k] = g_app->gizmoAxisDir[a][k];
                        }
                    }
                } else {
                    achsenRichtungen(g_app->gizmoRotSpace, rdir);
                }
                const float* achsen[3] = {rdir[0], rdir[1], rdir[2]};
                for (int ax = 0; ax < 3; ++ax) {
                    // Der Ring liegt in der Ebene der beiden ANDEREN Achsen.
                    const float* a1 = achsen[(ax + 1) % 3];
                    const float* a2 = achsen[(ax + 2) % 3];
                    const bool aktivR =
                        (g_app->gizmoRotating && g_app->gizmoAxis == ax) ||
                        (!g_app->gizmoRotating && g_app->gizmoHover == ax);
                    const std::uint8_t hell = aktivR ? 255U : 190U;
                    const std::uint8_t dunkel = aktivR ? 140U : 30U;
                    const std::uint8_t rr2 = (ax == 0) ? hell : dunkel;
                    const std::uint8_t gg2 = (ax == 1) ? hell : dunkel;
                    const std::uint8_t bb2 = (ax == 2) ? hell : dunkel;
                    P2 vorigP;
                    bool hatVorig = false;
                    constexpr int kTeile = 48;
                    for (int t = 0; t <= kTeile; ++t) {
                        const float w2 = 6.2831853F *
                                         static_cast<float>(t) /
                                         static_cast<float>(kTeile);
                        float pt[3];
                        for (int k = 0; k < 3; ++k) {
                            pt[k] = gz.pos[k] + g_app->gizmoOffset[k] +
                                    (a1[k] * std::cos(w2) + a2[k] * std::sin(w2)) *
                                        ringRad;
                        }
                        const P2 jetzt2 = project(pt);
                        // Den Punkt fuer den Ueberfahr-Test merken - der
                        // Test laeuft jede Runde, gezeichnet wird nur bei
                        // mapDirty. Dieselbe Arbeitsteilung wie bei den
                        // Achsenstrecken des Verschiebens.
                        if (t < App::kRingPts) {
                            g_app->gizmoRingSx[ax][t] =
                                jetzt2.ok ? jetzt2.x : -1.0F;
                            g_app->gizmoRingSy[ax][t] =
                                jetzt2.ok ? jetzt2.y : -1.0F;
                        }
                        if (hatVorig && kRasterGizmo) {
                            line(vorigP, jetzt2, rr2, gg2, bb2, aktivR);
                        }
                        vorigP = jetzt2;
                        hatVorig = true;
                    }

                    // --- Das Tortenstueck --------------------------------
                    //
                    // Nur an der Achse, an der gezogen wird, und nur dann.
                    // Es zeigt, WIE WEIT gedreht wurde - in Max heisst das
                    // "Show Pie Slice", und ohne es ist ein stehender Ring
                    // nur ein Ring.
                    //
                    // Gezeichnet als Speichen vom Mittelpunkt zum Rand:
                    // gefuellte Flaechen kann der Zeichner nicht, aber ein
                    // dichter Faecher liest sich genauso.
                    if (aktivR && g_app->gizmoAxis == ax) {
                        const float grad = g_app->gizmoAngles[ax] -
                                           g_app->gizmoRotStart[ax];
                        const float bogen = grad * 3.14159265F / 180.0F;
                        const int speichen =
                            std::clamp(static_cast<int>(std::fabs(grad) / 4.0F),
                                       1, 90);
                        float mitte[3];
                        for (int k = 0; k < 3; ++k) {
                            mitte[k] = gz.pos[k] + g_app->gizmoOffset[k];
                        }
                        const P2 mp2 = project(mitte);
                        for (int q = 0; q <= speichen; ++q) {
                            const float w3 = bogen * static_cast<float>(q) /
                                             static_cast<float>(speichen);
                            float pt2[3];
                            for (int k = 0; k < 3; ++k) {
                                pt2[k] = mitte[k] +
                                         (a1[k] * std::cos(w3) +
                                          a2[k] * std::sin(w3)) * ringRad;
                            }
                            if (kRasterGizmo) { line(mp2, project(pt2), 255, 230, 120, false); }
                        }
                    }
                }
            }

            if (ausgewaehlt && g_app->gizmoMode == App::GizmoMode::Move) {
                // Deutlich groesser als in rc153. Ein Gizmo, das man suchen
                // muss, ist keines - in Max nimmt es einen guten Teil des
                // Bildes ein, und das ist der Grund.
                const float len = sizeAt(gz.pos) * 5.5F;
                g_app->gizmoWorldLen = len;
                g_app->gizmoShown = true;
                float mdir[3][3];
                achsenRichtungen(g_app->gizmoMoveSpace, mdir);
                for (int ax = 0; ax < 3; ++ax) {
                    float ende[3];
                    for (int k = 0; k < 3; ++k) {
                        ende[k] = gz.pos[k] + g_app->gizmoOffset[k];
                    }
                    float anfang[3];
                    for (int k = 0; k < 3; ++k) {
                        anfang[k] = ende[k];
                    }
                    // Im Weltsystem ist mdir die Einheitsmatrix, das ist
                    // also genau das bisherige ende[ax] += len.
                    for (int k = 0; k < 3; ++k) {
                        ende[k] += mdir[ax][k] * len;
                    }
                    const P2 a2 = project(anfang);
                    const P2 b2 = project(ende);
                    g_app->gizmoSx[ax][0] = a2.ok ? a2.x : -1.0F;
                    g_app->gizmoSy[ax][0] = a2.ok ? a2.y : -1.0F;
                    g_app->gizmoSx[ax][1] = b2.ok ? b2.x : -1.0F;
                    g_app->gizmoSy[ax][1] = b2.ok ? b2.y : -1.0F;
                    // Angefasst ODER ueberfahren: die Achse leuchtet.
                    //
                    // Ueberfahren, damit man vor dem Klick weiss, ob man
                    // trifft. Angefasst, damit man waehrend des Ziehens
                    // sieht, welche wirkt - auch wenn die Maus laengst
                    // daneben steht.
                    const bool aktiv = (g_app->gizmoAxis == ax) ||
                                       (g_app->gizmoAxis < 0 &&
                                        g_app->gizmoHover == ax);
                    const std::uint8_t hell = aktiv ? 255U : 200U;
                    const std::uint8_t dunkel = aktiv ? 140U : 30U;
                    const std::uint8_t ar = (ax == 0) ? hell : dunkel;
                    const std::uint8_t ag = (ax == 1) ? hell : dunkel;
                    const std::uint8_t ab = (ax == 2) ? hell : dunkel;
                    if (kRasterGizmo) { line(a2, b2, ar, ag, ab, true); }
                    // Ein Kloetzchen an der Spitze - da faellt das Zielen
                    // leichter als auf einen Strich.
                    if (kRasterGizmo && b2.ok) {
                        const int bx = static_cast<int>(b2.x);
                        const int by = static_cast<int>(b2.y);
                        const int r2 = aktiv ? 7 : 5;
                        for (int dy2 = -r2; dy2 <= r2; ++dy2) {
                            for (int dx2 = -r2; dx2 <= r2; ++dx2) {
                                plot(bx + dx2, by + dy2, b2.z, ar, ag, ab);
                            }
                        }
                    }
                }

                // --- Das Kaestchen in der Mitte ----------------------
                //
                // 3ds Max nennt es "center box": "You can constrain
                // translation to the viewport plane by dragging the center
                // box." Damit bewegt man in der BILDEBENE - also in alle
                // Richtungen zugleich, so wie man es gerade sieht.
                //
                // Es ist kein Ersatz fuer die Achsen, sondern ihr
                // Gegenstueck: die Achsen sind genau, das Kaestchen ist
                // schnell.
                float mittePos[3];
                for (int k = 0; k < 3; ++k) {
                    mittePos[k] = gz.pos[k] + g_app->gizmoOffset[k];
                }
                const P2 mitte = project(mittePos);
                // Der Drehring um die Hochachse auch hier (rc597) - nur die
                // Punkte fuer Ueberfahren und Zeichnen der Ueberlagerung.
                {
                    float rdir[3][3];
                    achsenRichtungen(g_app->gizmoRotSpace, rdir);
                    const float ringRad = len * 0.6F;
                    // rc599: im Verschieben nur die Pfeile - gedreht wird im
                    // Drehmodus ("bei Move ein Move-Target, bei Drehen ein
                    // Rotation-Target"). Der Ring bleibt aus.
                    g_app->kamYawRingInMove = false;
                    for (int t = 0; t < App::kRingPts && g_app->kamYawRingInMove; ++t) {
                        const float w2 = 6.2831853F * static_cast<float>(t) / static_cast<float>(App::kRingPts - 1);
                        float pt[3];
                        for (int k = 0; k < 3; ++k) {
                            pt[k] = mittePos[k] + (rdir[0][k] * std::cos(w2) + rdir[1][k] * std::sin(w2)) * ringRad;
                        }
                        const P2 rp = project(pt);
                        g_app->gizmoRingSx[2][t] = rp.ok ? rp.x : -1.0F;
                        g_app->gizmoRingSy[2][t] = rp.ok ? rp.y : -1.0F;
                    }
                }
                if (kRasterGizmo && mitte.ok) {
                    const bool aktivM =
                        (g_app->gizmoAxis == App::kGizmoFree) ||
                        (g_app->gizmoAxis < 0 &&
                         g_app->gizmoHover == App::kGizmoFree);
                    const std::uint8_t mv = aktivM ? 255U : 190U;
                    const int mr = aktivM ? 9 : 7;
                    const int cx = static_cast<int>(mitte.x);
                    const int cy = static_cast<int>(mitte.y);
                    for (int d = -mr; d <= mr; ++d) {
                        plot(cx + d, cy - mr, mitte.z, mv, mv, mv);
                        plot(cx + d, cy + mr, mitte.z, mv, mv, mv);
                        plot(cx - mr, cy + d, mitte.z, mv, mv, mv);
                        plot(cx + mr, cy + d, mitte.z, mv, mv, mv);
                    }
                }
            }
        }
    }

    // --- Die Kamera, WIE SIE JETZT STEHT ---------------------------------
    //
    // Die Marken oben zeigen, wo im Skript ein MOVE oder PAN steht - also
    // die Stuetzstellen. Was dazwischen passiert, sah man nicht: schiebt man
    // den Zeitregler, blieben sie stehen.
    //
    // Diese eine Marke bewegt sich mit. Sie sitzt genau da, wo die Kamera
    // zum eingestellten Zeitpunkt ist, und zeigt dorthin, wo sie hinsieht -
    // dieselbe Rechnung, die auch "Durch die Kamera" benutzt (camTrack.at).
    // So sieht man die Fahrt von aussen, waehrend man den Regler zieht.
    g_app->geisterKamera = false;
    Shot laufend;
    bool zeigeLaufend = false;
    {
        const CameraState st = camStateAt(g_app->playMs);
        if (st.enabled) {
            for (int i = 0; i < 3; ++i) {
                laufend.pos[i] = st.pos[i];
                laufend.ang[i] = st.angles[i];
            }
            laufend.hasPos = true;
            laufend.hasAng = true;
            laufend.fovX = st.fovX;   // Paket kamera, rc601: der laufende ZOOM
            zeigeLaufend = true;
        }
    }

    // Nur noch EINE Kamera.
    //
    // Vorher stand fuer jeden MOVE-Befehl ein eigener Rahmen im Bild - bei
    // zehn Befehlen zehn Kameras, obwohl es im Skript nur eine gibt, die
    // sich bewegt. Das war irrefuehrend.
    //
    // Jetzt steht dort die Kamera, wie sie zur eingestellten Zeit steht.
    // Wo im Skript ihre Stuetzstellen liegen, sieht man, sobald man sie
    // ANKLICKT - dann erscheint die Bahn mit den Quadraten. So halten es
    // Max und Maya auch.
    // --- Welche Kamera ist der AUSGANGSPUNKT? -----------------------------
    //
    // Ist ein Schluessel angewaehlt, dann SEINE - nicht die laufende.
    //
    // Vorher zeigte die laufende Kamera den Stand zur aktuellen Zeit, und
    // die gruene Vorschau den des Schluessels. Zwei richtige Werte zu
    // verschiedenen Zeitpunkten, und im Bild sah es aus, als zeige die
    // Vorschau falsch.
    //
    // In rc208/rc209 habe ich versucht, das ueber die ZEIT zu loesen: den
    // Zeiger zum Schluessel springen lassen. Das war der falsche Hebel und
    // hat mehr kaputt gemacht, als es half - landet der Zeiger hinter
    // einem camera(DISABLE), ist gar keine Kamera mehr da, und nach einem
    // Rueckgaengig erst recht nicht.
    //
    // Richtig ist, das BILD zu aendern statt den Zustand: beim Bearbeiten
    // eines Schluessels wird er selbst gezeigt. Die Zeit bleibt, wo sie
    // ist, und nichts anderes im Programm merkt etwas davon.
    const Shot* schluesselShot = nullptr;
    if (g_app->selectedKey >= 0 &&
        static_cast<std::size_t>(g_app->selectedKey) < shots.size()) {
        schluesselShot = &shots[static_cast<std::size_t>(g_app->selectedKey)];
    }

    // Das Protokoll beim Anwaehlen ist wieder heraus - es hat seine Arbeit
    // getan (rc213) und faellt sonst bei jedem Klick an.
    for (std::size_t k = 0; k <= shots.size(); ++k) {
        const bool istLaufend = (k == shots.size());
        if (!istLaufend) {
            continue;   // die Stuetzstellen zeichnet die Bahn, nicht hier
        }
        if (schluesselShot == nullptr && !zeigeLaufend) {
            break;
        }
        // Die Kamera steht, wo sie zur eingestellten ZEIT steht - auch mit
        // angewaehltem Schluessel. Gemeldet: "wenn ich sie selected habe und
        // die Timeline bewege, dann geht das nicht" - bis hierher hatte der
        // Schluessel Vorrang, und die Kamera klebte an ihm. Welcher
        // Schluessel bearbeitet wird, zeigen das gelbe Quadrat und das Gizmo
        // daran; beim Ziehen der Zeitleiste geht er mit (schluesselMitZeit).
        // Nur wenn die Kamera zu dieser Zeit aus ist, steht sie am Schluessel.
        const Shot& sh = zeigeLaufend                ? laufend
                         : (schluesselShot != nullptr) ? *schluesselShot
                                                       : laufend;
        if (istLaufend) {
            // Wo sitzt sie auf dem Bildschirm? Der Klick braucht das.
            const P2 mitte = project(sh.pos);
            g_app->cameraSx = mitte.ok ? mitte.x : -1.0F;
            g_app->cameraSy = mitte.ok ? mitte.y : -1.0F;
        }
        const bool selected = !istLaufend && (sh.path == g_app->selectedPath);
        // Die laufende Kamera in Gruen: sie gehoert zu keiner Zeile, sie
        // ist der Zustand zwischen den Zeilen.
        // Angeklickt: kraeftiges Weiss, wie eine Auswahl in Max. Sonst
        // gruen - sie gehoert zu keiner Skriptzeile, sondern ist der
        // Zustand zwischen ihnen.
        const bool camSel = istLaufend && g_app->cameraSelected;
        // --- Farben: GRUEN gehoert der Vorschau, sonst niemandem ---------
        //
        // Die laufende Kamera war gruen (90, 235, 120), solange sie nicht
        // selbst angewaehlt war - und die Vorschau am Gizmo ist ebenfalls
        // gruen. Wer einen SCHLUESSEL anwaehlt (nicht die Kamera), bekam
        // also zwei gruene Kameras dicht beieinander, und die eine
        // verschwand optisch in der anderen. Gemeldet als "die weisse
        // Kamera wird unsichtbar gemacht oder geloescht".
        //
        // Jetzt ist die Aufteilung eindeutig:
        //   angewaehlt      - kraeftiges Weiss
        //   sonst           - gedecktes Hellgrau
        //   die VORSCHAU    - gruen, und nur sie
        //
        // Damit sagt die Farbe, was das Ding IST, statt was gerade
        // angeklickt wurde.
        // Ueberfahren (rc588): hell und warm, wie der Umriss einer Entity.
        const bool camHover = istLaufend && !camSel && g_app->kameraHover;
        const std::uint8_t r = camSel ? 255 : camHover ? 255 : (istLaufend ? 200 : 255);
        const std::uint8_t g =
            camSel ? 255 : camHover ? 228 : (istLaufend ? 205 : (selected ? 210 : 110));
        const std::uint8_t b =
            camSel ? 255 : camHover ? 150 : (istLaufend ? 215 : (selected ? 50 : 210));

        // Steht gerade eine Vorschau am Gizmo an, wird die Kamera DAMIT
        // gezeichnet - sonst zieht man am Gizmo und die Kamera bleibt
        // stehen. Der Versatz gilt nur, wenn die Kamera auch angewaehlt
        // ist; sonst wanderte sie, waehrend man einen anderen Schluessel
        // bearbeitet.
        // Die laufende Kamera zeigt den Stand AUS DEM SKRIPT - ohne die
        // Vorschau am Gizmo.
        //
        // In rc168 bekam sie den Versatz dazu, damit sie sich beim Ziehen
        // ueberhaupt bewegt. Inzwischen zeichnet der Gizmo-Zweig aber eine
        // eigene, gruene Vorschaukamera - und damit standen ZWEI Modelle
        // am selben Ort mit derselben Stellung uebereinander. Gemeldet als
        // "die 2. Kamera die dabei erstellt wird ist etwas komisch".
        //
        // Jetzt ist es ein Vorher-Nachher-Paar: weiss, wo sie laut Skript
        // steht, gruen, wo sie nach "Ins Skript schreiben" staende. Genau
        // das will man beim Einstellen sehen.
        //
        // Der Durchblick nimmt die Vorschau weiter mit - dort gibt es kein
        // Paar, sondern nur das eine Bild, das man beurteilt.
        zeichneKamera(sh.pos, sh.ang, r, g, b, selected, sh.hasAng, sh.fovX);
        // Fuer den Selbsttest (Paket kamera, rc601): die Pyramide dieser Kamera.
        g_app->kamPyramideTan[0] = spielTanHalbX(sh.fovX, pyramidenSpiel);
        g_app->kamPyramideTan[1] = spielTanHalbY(sh.fovX, pyramidenSpiel);

        // --- Unterwegs: wohin die Kamera faehrt ---------------------------
        //
        // shank: "sobald sich die Kamera bewegt, ist der Gizmo nicht mehr
        // dran - soll das so sein?" Ja: bearbeitet wird der ZIELPUNKT des
        // MOVE, und das Gizmo sitzt dort. Damit man das sieht, steht dort
        // eine gelbe Geisterkamera mit der Blickrichtung am Ziel, und eine
        // gestrichelte Linie fuehrt von der fahrenden Kamera hin. Steht die
        // Kamera auf ihrem Punkt, faellt beides weg.
        if (zeigeLaufend && schluesselShot != nullptr && g_app->cameraSelected &&
            &sh == &laufend) {
            const float dx = laufend.pos[0] - schluesselShot->pos[0];
            const float dy = laufend.pos[1] - schluesselShot->pos[1];
            const float dz = laufend.pos[2] - schluesselShot->pos[2];
            if (dx * dx + dy * dy + dz * dz > 4.0F) {
                zeichneKamera(schluesselShot->pos, schluesselShot->ang, 200, 170, 60, false,
                              schluesselShot->hasAng, schluesselShot->fovX);
                constexpr int kStriche = 24;
                for (int q = 0; q < kStriche; q += 2) {
                    float a1[3];
                    float b1[3];
                    for (int k2 = 0; k2 < 3; ++k2) {
                        const float von = laufend.pos[k2];
                        const float nach = schluesselShot->pos[k2];
                        a1[k2] = von + (nach - von) * static_cast<float>(q) / kStriche;
                        b1[k2] = von + (nach - von) * static_cast<float>(q + 1) / kStriche;
                    }
                    line(project(a1), project(b1), 220, 190, 80, false);
                }
                g_app->geisterKamera = true;
            }
        }
    }
}

// Was der gewaehlte camera-Befehl sieht.
//
// Ort, Blickrichtung UND Zoom. Der Zoom ist dabei nicht kosmetisch: in
// Ravens 1510 Skripten stehen 461 ZOOM-Befehle mit Werten von 6,3 bis 110
// Grad. Eine Einstellung mit 30 Grad zeigt etwas voellig anderes als eine
// mit 80, und ohne den richtigen Blickwinkel beurteilt man das falsche Bild.
//
// Gesucht wird ab der gewaehlten Zeile RUECKWAERTS: die Engine arbeitet die
// Befehle nacheinander ab, also gilt der letzte MOVE, der letzte PAN und der
// letzte ZOOM vor dieser Stelle. Wer nur die gewaehlte Zeile ansieht,
// bekommt bei einem einzelnen ZOOM keine Position.
struct ShotView {
    float pos[3]{};
    float ang[3]{};
    float fovX = 90.0F;      // CAMERA_DEFAULT_FOV aus cg_camera.h
    bool havePos = false;
    bool haveAng = false;
    std::string label;
};

bool shotAtSelection(ShotView& out);

// --- Spiegel (rc593) -------------------------------------------------------
//
// R_GetPortalOrientations: eine Flaeche mit einem `portal`-Shader braucht
// ein misc_portal_surface in der Naehe ihrer Ebene. Ohne target ist es ein
// Spiegel (die Kamera wird an der Ebene gespiegelt); mit target zeigt es den
// Blick einer misc_portal_camera - das ist hier (noch) nicht gebaut, die
// Flaeche zeigt dann ihre Stufen wie bisher.
namespace {
struct SpiegelFlaeche {
    float n[3]{};
    float d = 0.0F;
    float mitte[3]{};
    float radius = 0.0F;
    // Portal mit misc_portal_camera (rc593): Ort der Flaeche (surface->origin)
    // und die Kamera aus CG_Portal.
    bool portal = false;
    float flOrt[3]{};
    float kamOrt[3]{};
    float kamAchse0[3]{};
    float roll = 0.0F;
    int drehTempo = 0;      // frame: 25 / 75 (spawnflags 1 / 2)
    bool bewegt = false;    // oldframe / powerups: ohne spawnflags 4
};

// PerpendicularVector (q_math.cpp): auf die kleinste Achse projiziert.
void senkrecht(const float src[3], float dst[3]) {
    int pos = 0;
    float minelem = 1.0F;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(src[i]) < minelem) {
            pos = i;
            minelem = std::fabs(src[i]);
        }
    }
    float t[3] = {0.0F, 0.0F, 0.0F};
    t[pos] = 1.0F;
    const float nn = src[0] * src[0] + src[1] * src[1] + src[2] * src[2];
    const float dd = (src[0] * t[0] + src[1] * t[1] + src[2] * t[2]) / (nn > 0.0F ? nn : 1.0F);
    for (int i = 0; i < 3; ++i) { dst[i] = t[i] - dd * src[i]; }
    const float l = std::sqrt(dst[0] * dst[0] + dst[1] * dst[1] + dst[2] * dst[2]);
    if (l > 0.0F) { for (int i = 0; i < 3; ++i) { dst[i] /= l; } }
}

// RotatePointAroundVector: v um die Achse a (Einheit) um grad drehen.
void drehe(const float a[3], float v[3], float grad) {
    const float w = grad * 3.14159265F / 180.0F;
    const float c = std::cos(w);
    const float s = std::sin(w);
    const float d = a[0] * v[0] + a[1] * v[1] + a[2] * v[2];
    const float k[3] = {a[1] * v[2] - a[2] * v[1], a[2] * v[0] - a[0] * v[2], a[0] * v[1] - a[1] * v[0]};
    for (int i = 0; i < 3; ++i) { v[i] = v[i] * c + k[i] * s + a[i] * d * (1.0F - c); }
}
const std::vector<SpiegelFlaeche>& spiegelKandidaten() {
    static std::vector<SpiegelFlaeche> liste;
    static const void* geoStand = nullptr;
    static int entStand = -1;
    static std::size_t texStand = 0;
    if (geoStand == g_app->geo.surfaces.data() && entStand == g_app->entStand &&
        texStand == g_app->textures.byShader.size()) {
        return liste;
    }
    geoStand = g_app->geo.surfaces.data();
    entStand = g_app->entStand;
    texStand = g_app->textures.byShader.size();
    liste.clear();
    // misc_portal_surface: ohne target ein Spiegel, mit target eine Portal-
    // kamera (locateCamera, g_misc.cpp).
    struct Ziel {
        std::array<float, 3> ort{};
        bool portal = false;
        float kamOrt[3]{};
        float achse0[3]{};
        float roll = 0.0F;
        int drehTempo = 0;
        bool bewegt = false;
    };
    std::vector<Ziel> orte;
    const auto ortVon = [](const MapEntity& e, float o[3]) {
        return !e.origin.empty() && std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]) == 3;
    };
    const auto mitName = [](const std::string& n) -> const MapEntity* {
        if (n.empty()) { return nullptr; }
        for (const MapEntity& x : g_app->map.entities) {
            if (x.targetname == n) { return &x; }
        }
        return nullptr;
    };
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname != "misc_portal_surface") { continue; }
        Ziel z;
        if (!ortVon(e, z.ort.data())) { continue; }
        if (const std::string* t = e.find("target"); t != nullptr) {
            const MapEntity* kam = mitName(*t);
            if (kam == nullptr || !ortVon(*kam, z.kamOrt)) { continue; }
            z.portal = true;
            const std::string* sf = kam->find("spawnflags");
            const int flags = sf != nullptr ? std::atoi(sf->c_str()) : 0;
            z.drehTempo = (flags & 1) != 0 ? 25 : ((flags & 2) != 0 ? 75 : 0);
            z.bewegt = (flags & 4) == 0;
            if (const std::string* r = kam->find("roll")) {
                // clientNum = roll/360*256, skinNum = clientNum/256*360 (gekuerzt)
                z.roll = static_cast<float>(static_cast<int>(std::atof(r->c_str()) / 360.0 * 256.0)) / 256.0F * 360.0F;
            }
            float dir[3] = {1.0F, 0.0F, 0.0F};
            float zielOrt[3];
            const MapEntity* ziel = kam->find("target") != nullptr ? mitName(*kam->find("target")) : nullptr;
            if (ziel != nullptr && ortVon(*ziel, zielOrt)) {
                for (int k = 0; k < 3; ++k) { dir[k] = zielOrt[k] - z.kamOrt[k]; }
            } else {
                // G_SetMovedir aus angle/angles
                float ang[3] = {0.0F, 0.0F, 0.0F};
                if (const std::string* a = kam->find("angles")) {
                    (void)std::sscanf(a->c_str(), "%f %f %f", &ang[0], &ang[1], &ang[2]);
                } else if (const std::string* a1 = kam->find("angle")) {
                    ang[1] = static_cast<float>(std::atof(a1->c_str()));
                }
                const float pr = ang[0] * 3.14159265F / 180.0F;
                const float yr = ang[1] * 3.14159265F / 180.0F;
                dir[0] = std::cos(pr) * std::cos(yr);
                dir[1] = std::cos(pr) * std::sin(yr);
                dir[2] = -std::sin(pr);
            }
            const float l = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            if (l < 1e-6F) { continue; }
            for (int k = 0; k < 3; ++k) { z.achse0[k] = dir[k] / l; }
        }
        orte.push_back(z);
    }
    if (orte.empty()) { return liste; }
    for (const BspSurface& s : g_app->geo.surfaces) {
        if ((s.type != BspSurface::Type::Planar && s.type != BspSurface::Type::TriangleSoup) || s.numVerts < 3 ||
            s.shader < 0 || static_cast<std::size_t>(s.shader) >= g_app->textures.byShader.size() ||
            !g_app->textures.byShader[static_cast<std::size_t>(s.shader)].istPortal) {
            continue;
        }
        const BspVertex* v = &g_app->geo.verts[static_cast<std::size_t>(s.firstVert)];
        float a[3];
        float b[3];
        for (int k = 0; k < 3; ++k) {
            a[k] = v[1].xyz[k] - v[0].xyz[k];
            b[k] = v[2].xyz[k] - v[0].xyz[k];
        }
        float n[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
        const float l = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (l < 1e-6F) { continue; }
        for (float& q : n) { q /= l; }
        // Zur sichtbaren Seite: wie die Normale der Ecke.
        if (n[0] * v[0].normal[0] + n[1] * v[0].normal[1] + n[2] * v[0].normal[2] < 0.0F) {
            for (float& q : n) { q = -q; }
        }
        const float d = n[0] * v[0].xyz[0] + n[1] * v[0].xyz[1] + n[2] * v[0].xyz[2];
        const Ziel* naechstes = nullptr;
        float besteAbst = 64.0F;
        for (const Ziel& o : orte) {
            const float abst = std::fabs(n[0] * o.ort[0] + n[1] * o.ort[1] + n[2] * o.ort[2] - d);
            if (abst <= besteAbst) {
                besteAbst = abst;
                naechstes = &o;
            }
        }
        if (naechstes == nullptr) { continue; }
        SpiegelFlaeche f;
        if (naechstes->portal) {
            f.portal = true;
            const float ad = n[0] * naechstes->ort[0] + n[1] * naechstes->ort[1] + n[2] * naechstes->ort[2] - d;
            for (int k = 0; k < 3; ++k) {
                f.flOrt[k] = naechstes->ort[k] - ad * n[k];
                f.kamOrt[k] = naechstes->kamOrt[k];
                f.kamAchse0[k] = naechstes->achse0[k];
            }
            f.roll = naechstes->roll;
            f.drehTempo = naechstes->drehTempo;
            f.bewegt = naechstes->bewegt;
        }
        float lo[3] = {1e30F, 1e30F, 1e30F};
        float hi[3] = {-1e30F, -1e30F, -1e30F};
        for (int q = 0; q < s.numVerts; ++q) {
            for (int k = 0; k < 3; ++k) {
                lo[k] = std::min(lo[k], v[q].xyz[k]);
                hi[k] = std::max(hi[k], v[q].xyz[k]);
            }
        }
        for (int k = 0; k < 3; ++k) {
            f.n[k] = n[k];
            f.mitte[k] = 0.5F * (lo[k] + hi[k]);
        }
        f.d = d;
        f.radius = 0.5F * std::sqrt((hi[0] - lo[0]) * (hi[0] - lo[0]) + (hi[1] - lo[1]) * (hi[1] - lo[1]) +
                                    (hi[2] - lo[2]) * (hi[2] - lo[2]));
        liste.push_back(f);
    }
    return liste;
}
}  // namespace

bool spiegelFuer(const Camera& cam, float ebene[4]) {
    float vorn[3];
    cam.forward(vorn);
    float besteWeite = 1e30F;
    const SpiegelFlaeche* beste = nullptr;
    for (const SpiegelFlaeche& f : spiegelKandidaten()) {
        // Die Kamera muss vor der Flaeche stehen ...
        const float seite = f.n[0] * cam.pos[0] + f.n[1] * cam.pos[1] + f.n[2] * cam.pos[2] - f.d;
        if (seite <= 1.0F) { continue; }
        // ... und die Flaeche vor der Kamera liegen.
        const float zu[3] = {f.mitte[0] - cam.pos[0], f.mitte[1] - cam.pos[1], f.mitte[2] - cam.pos[2]};
        const float weite = std::sqrt(zu[0] * zu[0] + zu[1] * zu[1] + zu[2] * zu[2]);
        if (zu[0] * vorn[0] + zu[1] * vorn[1] + zu[2] * vorn[2] < -f.radius) { continue; }
        if (weite - f.radius < besteWeite) {
            besteWeite = weite - f.radius;
            beste = &f;
        }
    }
    if (beste == nullptr) { return false; }
    const SpiegelFlaeche& f = *beste;
    for (int k = 0; k < 3; ++k) { ebene[k] = f.n[k]; }
    ebene[3] = f.d;
    App::NebenSicht& ns = g_app->nebenSicht;
    ns = App::NebenSicht{};
    if (!f.portal) {
        // Spiegel: R = I - 2nn^T, Verschiebung 2dn; die Wicklung kehrt sich um.
        for (int z = 0; z < 3; ++z) {
            for (int sp = 0; sp < 3; ++sp) {
                const float wert = (z == sp ? 1.0F : 0.0F) - 2.0F * f.n[z] * f.n[sp];
                ns.welt[z * 4 + sp] = wert;
                ns.achsen[z * 3 + sp] = wert;
            }
            ns.welt[z * 4 + 3] = 2.0F * f.d * f.n[z];
        }
        ns.welt[15] = 1.0F;
        // Eine Einheit vor dem Spiegel schneiden: was genau auf der Ebene
        // liegt (der Boden um den Spiegel), gehoert nicht ins Bild.
        for (int k = 0; k < 3; ++k) { ns.schnitt[k] = f.n[k]; }
        ns.schnitt[3] = -f.d - 1.0F;
        ns.umkehren = true;
        return true;
    }
    // --- Portalkamera (R_GetPortalOrientations, CG_Portal) -----------------
    //
    // Flaeche: Achse 0 die Normale, 1 senkrecht dazu, 2 das Kreuzprodukt.
    // Kamera: Achse 0 zum Ziel, 1 die NEGIERTE Senkrechte ("negating this
    // tends to get the directions like they want"), 2 das Kreuzprodukt; dann
    // 0 und 1 umgedreht, und gegebenenfalls um Achse 0 gedreht. Die Ansicht
    // ist M(x) = C_o + sum_i <x - S_o, S_i> C_i - hier wird die Welt mit der
    // Umkehrung transformiert und von der echten Kamera aus gezeichnet.
    float sa[3][3];
    for (int k = 0; k < 3; ++k) { sa[0][k] = f.n[k]; }
    senkrecht(sa[0], sa[1]);
    sa[2][0] = sa[0][1] * sa[1][2] - sa[0][2] * sa[1][1];
    sa[2][1] = sa[0][2] * sa[1][0] - sa[0][0] * sa[1][2];
    sa[2][2] = sa[0][0] * sa[1][1] - sa[0][1] * sa[1][0];
    float ea[3][3];
    for (int k = 0; k < 3; ++k) { ea[0][k] = f.kamAchse0[k]; }
    senkrecht(ea[0], ea[1]);
    for (float& q : ea[1]) { q = -q; }
    ea[2][0] = ea[0][1] * ea[1][2] - ea[0][2] * ea[1][1];
    ea[2][1] = ea[0][2] * ea[1][0] - ea[0][0] * ea[1][2];
    ea[2][2] = ea[0][0] * ea[1][1] - ea[0][1] * ea[1][0];
    float ca[3][3];
    for (int k = 0; k < 3; ++k) {
        ca[0][k] = -ea[0][k];
        ca[1][k] = -ea[1][k];
        ca[2][k] = ea[2][k];
    }
    const auto neueAchse2 = [&]() {
        ca[2][0] = ca[0][1] * ca[1][2] - ca[0][2] * ca[1][1];
        ca[2][1] = ca[0][2] * ca[1][0] - ca[0][0] * ca[1][2];
        ca[2][2] = ca[0][0] * ca[1][1] - ca[0][1] * ca[1][0];
    };
    const double t = g_app->playMs;
    if (f.bewegt) {
        if (f.drehTempo != 0) {
            drehe(ca[0], ca[1], static_cast<float>(t / 1000.0 * f.drehTempo));
        } else {
            drehe(ca[0], ca[1], f.roll + static_cast<float>(std::sin(t * 0.003)) * 4.0F);
        }
        neueAchse2();
    } else if (f.roll != 0.0F) {
        drehe(ca[0], ca[1], f.roll);
        neueAchse2();
    }
    // Umkehrung: L = sum_i S_i C_i^T, t = S_o - L C_o.
    float lin[3][3];
    for (int z = 0; z < 3; ++z) {
        for (int sp = 0; sp < 3; ++sp) {
            lin[z][sp] = sa[0][z] * ca[0][sp] + sa[1][z] * ca[1][sp] + sa[2][z] * ca[2][sp];
        }
    }
    for (int z = 0; z < 3; ++z) {
        for (int sp = 0; sp < 3; ++sp) { ns.welt[z * 4 + sp] = lin[z][sp]; }
        ns.welt[z * 4 + 3] = f.flOrt[z] - (lin[z][0] * f.kamOrt[0] + lin[z][1] * f.kamOrt[1] + lin[z][2] * f.kamOrt[2]);
        // Richtungen der Ansicht: M_lin(v) = sum_i <v, S_i> C_i = L^T v.
        for (int sp = 0; sp < 3; ++sp) { ns.achsen[z * 3 + sp] = lin[sp][z]; }
    }
    ns.welt[15] = 1.0F;
    // Schnittebene: vor der Portalkamera (portalPlane = -C_0).
    const float pn[3] = {-ca[0][0], -ca[0][1], -ca[0][2]};
    for (int k = 0; k < 3; ++k) { ns.schnitt[k] = pn[k]; }
    ns.schnitt[3] = -(pn[0] * f.kamOrt[0] + pn[1] * f.kamOrt[1] + pn[2] * f.kamOrt[2]);
    ns.umkehren = false;
    ns.portal = true;
    return true;
}

bool himmelPortalOrt(float ort[3]) {
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname != "misc_skyportal" || e.origin.empty()) { continue; }
        return std::sscanf(e.origin.c_str(), "%f %f %f", &ort[0], &ort[1], &ort[2]) == 3;
    }
    return false;
}

void lichtStileZuZeit() {
    if (g_app->geo.stilSchichten.empty()) { return; }
    // Die Schaltungen kommen aus dem Nachbau des Skripts (use auf ein light,
    // auch ueber Relais und Ausloeser - MoverSim::lichtSchaltungen).
    const int seiten = stileAnwenden(g_app->geo,
                                     lichtStilGewichteZuZeit(g_app->map.entities, g_app->moverSim.lichtSchaltungen(),
                                                             g_app->playMs));
    if (seiten > 0) {
        g_app->mapDirty = true;
    }
}

bool vorschauKamera(Camera& vc, float aspekt) {
    const bool spielt = g_app->playing && g_app->followCam && g_app->camTrackValid &&
                        !g_app->camTrack.segments.empty();
    if (!g_app->kameraBildImBild || !g_app->cameraSelected || g_app->camViewActive || spielt ||
        g_app->kameraPilot || !g_app->camTrackValid || g_app->camTrack.segments.empty()) {
        return false;
    }
    vc = g_app->cam;
    // --- Shot oder Zeitleiste? (rc597) -----------------------------------
    //
    // shank: "wenn ich die Kamera anklicke und die Timeline bewege, bleibt das
    // Bild rechts unten einfach stehen - es sollte im Fenster in real time
    // updaten". Bis rc596 zeigte die Vorschau bei gewaehltem Shot immer
    // dessen feste Stellung. Jetzt: wurde die Zeitleiste seit der Auswahl
    // bewegt, folgt sie der Kamera zu dieser Zeit (auch mitten in einer
    // Fahrt); direkt nach der Auswahl und waehrend der Gizmo gezogen wird,
    // zeigt sie wie bisher den Shot.
    static Path auswahlVorher;
    static double msBeiAuswahl = -1.0;
    if (g_app->selectedPath != auswahlVorher) {
        auswahlVorher = g_app->selectedPath;
        msBeiAuswahl = g_app->playMs;
    }
    bool gizmoBewegt = false;
    for (int k = 0; k < 3; ++k) {
        gizmoBewegt = gizmoBewegt || g_app->gizmoOffset[k] != 0.0F || g_app->gizmoAngles[k] != 0.0F;
    }
    const bool zeitBewegt = g_app->playMs != msBeiAuswahl;
    ShotView shot;
    if ((gizmoBewegt || !zeitBewegt) && shotAtSelection(shot)) {
        // Wie der Durchblick: mit der Vorschau des Gizmos, damit das Bild
        // beim Ziehen mitlaeuft.
        for (int k = 0; k < 3; ++k) {
            vc.pos[k] = shot.pos[k] + g_app->gizmoOffset[k];
            vc.angles[k] = (shot.haveAng ? shot.ang[k] : 0.0F) + g_app->gizmoAngles[k];
        }
        // Paket kamera, rc601: der Bildwinkel wie im Spielfenster (das Bild
        // hat dessen Seitenverhaeltnis, der Rahmen ist also das ganze Bild).
        (void)setzeSpielFov(vc, shot.fovX, aspekt, spielFensterJetzt());
        return true;
    }
    const CameraState st = camStateAt(g_app->playMs);
    for (int k = 0; k < 3; ++k) {
        vc.pos[k] = st.pos[k];
        vc.angles[k] = st.angles[k];
    }
    // Paket kamera, rc601 (Review): wie die grosse Ansicht an der Zeitmarke
    // (drawMapView) - vor ENABLE und nach DISABLE der Spieler mit cg_fov 80
    // (sichtFovX). Gemeldet: "The preview inset ignores the cg_fov 80 rule" -
    // nach DISABLE zeigte sie den letzten ZOOM.
    (void)setzeSpielFov(vc, sichtFovX(st), aspekt, spielFensterJetzt());
    return true;
}

bool shotAtSelection(ShotView& out) {
    const Path& sel = g_app->selectedPath;
    if (sel.empty()) {
        return false;
    }
    // Nur auf der Ebene der Auswahl suchen - ein camera-Befehl in einem
    // anderen Block gehoert nicht zu dieser Einstellung.
    Path p = sel;
    bool any = false;
    Path movePfad;   // Paket kamera, rc601: der MOVE dieser Einstellung
    for (;;) {
        const Node* n = nodeAt(g_app->doc.script(), p);
        if (n != nullptr && n->name == "camera" && !n->args.empty()) {
            const std::string& type = n->args[0].text;
            float v[3] = {0, 0, 0};
            if (n->args.size() >= 2) {
                std::istringstream is(n->args[1].text);
                is >> v[0] >> v[1] >> v[2];
            }
            if (type == "MOVE" && !out.havePos) {
                for (int k = 0; k < 3; ++k) { out.pos[k] = v[k]; }
                out.havePos = true;
                movePfad = p;
                any = true;
            } else if (type == "PAN" && !out.haveAng) {
                for (int k = 0; k < 3; ++k) { out.ang[k] = v[k]; }
                out.haveAng = true;
                any = true;
            } else if (type == "ZOOM" && out.fovX == 90.0F && n->args.size() >= 2) {
                try {
                    out.fovX = std::stof(n->args[1].text);
                    any = true;
                } catch (...) {
                    // kein Zahlenwert - dann bleibt die Vorgabe
                }
            }
            if (out.label.empty()) {
                out.label = type;
            }
        }
        if (out.havePos && out.haveAng) {
            break;
        }
        if (p.back() == 0) {
            break;
        }
        --p.back();
    }
    // --- Der Stand der Einstellung aus der Kamerabahn (Paket kamera, rc601) ---
    //
    // Gemeldet: "I think camera FOV is slightly off". Die Suche oben laeuft
    // vom MOVE RUECKWAERTS - Ravens Reihenfolge ist aber MOVE; PAN; ZOOM
    // (cin2_jedi: "Behind Barriss" mit ZOOM 30 und PAN -1 5 0). Wer eine
    // Einstellung anwaehlte (die Auswahl ist ihr MOVE), sah ZOOM und PAN der
    // VORIGEN: dort 60 Grad und 0 90 0, bei der ersten 90 Grad und 0 0 0.
    // Dazu liess die Pruefung "fovX == 90" einen aelteren ZOOM einen
    // naeheren ZOOM 90 ueberschreiben, ENABLE (setzt auf 90 zurueck,
    // cg_camera.cpp:91) und Zoomdauern fehlten ganz, und ein MOVE auf eine
    // Marke - tag( "cam1", ORIGIN ) - las hier 0 0 0.
    //
    // Jetzt kommt alles aus der Kamerabahn, die die Befehle in der
    // Reihenfolge des Spiels abarbeitet (camStateAt, samt FOLLOW): Stand am
    // Ende der Fahrt dieses MOVE - dieselbe Zeit, auf die das Anwaehlen des
    // Schluessels die Zeitleiste stellt (kameraSchluesselWaehlen). Kennt die
    // Bahn den MOVE nicht (noch nicht gebaut), bleibt es bei der Suche oben.
    if (out.havePos && g_app->camTrackValid) {
        const double t = einstellungsZeit(g_app->camTrack, movePfad);
        if (t >= 0.0) {
            const CameraState st = camStateAt(t);
            for (int k = 0; k < 3; ++k) {
                out.pos[k] = st.pos[k];
                out.ang[k] = st.angles[k];
            }
            out.haveAng = true;
            out.fovX = st.fovX;
        }
    }
    // --- Paket kamera, rc601 (Review): Vorschau des Gizmos wie das Schreiben ---
    //
    // Gemeldet: "Gizmo preview and write-back use different starting angles".
    // Durchblick und Vorschau zeigen den Stand von oben plus gizmoOffset/
    // gizmoAngles; geschrieben wird aber "Schluessel + Zusatz"
    // (gizmoWriteAngles: Winkel des PAN dieser Einstellung, gizmoWriteBack/
    // markeOrt: Ort des MOVE). Bei einem PAN mit Dauer steht die Kamera am
    // Ende des MOVE noch am ANFANG des Schwenks - die Vorschau zeigte beim
    // Ziehen also nicht, was danach im Skript stand. "Geschrieben wird, WAS
    // DIE VORSCHAU ZEIGT": solange am Gizmo etwas ungeschrieben ist, rechnet
    // die Vorschau von denselben Werten aus wie das Schreiben (wie das
    // Kameramodell am Gizmo, drawCameraGizmos). Nach dem Schreiben gilt
    // wieder der Stand der Kamerabahn zur Zeit der Einstellung - bei einem
    // PAN mit Dauer also wieder dessen Anfang, wie im Spiel.
    if (out.havePos && g_app->cameraSelected) {
        bool verschoben = false;
        bool gedreht = false;
        for (int k = 0; k < 3; ++k) {
            verschoben = verschoben || g_app->gizmoOffset[k] != 0.0F;
            gedreht = gedreht || g_app->gizmoAngles[k] != 0.0F;
        }
        if (verschoben || gedreht) {
            const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
            if (g_app->selectedKey >= 0 && static_cast<std::size_t>(g_app->selectedKey) < ks.size() &&
                ks[static_cast<std::size_t>(g_app->selectedKey)].path == movePfad) {
                const KameraSchluessel& k = ks[static_cast<std::size_t>(g_app->selectedKey)];
                for (int a = 0; a < 3; ++a) {
                    if (verschoben) { out.pos[a] = k.pos[a]; }
                    if (gedreht) { out.ang[a] = k.hasAng ? k.ang[a] : 0.0F; }
                }
                if (gedreht) { out.haveAng = true; }
            }
        }
    }
    return any && out.havePos;
}

// Aus der Kamerastellung einen Befehl bauen und einfuegen.
//
// Zwei Befehle, weil es im Skript auch zwei sind: MOVE setzt den Ort, PAN
// die Blickrichtung. Die Reihenfolge der Winkel ist die der Engine -
// pitch, yaw, roll -, genau wie camera ( PAN, < p y r >, ... ) sie erwartet.
void insertCameraHere() {
    const std::vector<const Command*> ov = g_app->db.overloads("camera");
    if (ov.empty()) {
        return;
    }
    auto make = [&](const char* type, const std::string& vec) {
        Node n;
        n.kind = Node::Kind::Command;
        n.name = "camera";
        Arg a;
        a.kind = Arg::Kind::Ident;
        a.text = type;
        a.typeset = "CAMERA_COMMANDS";
        n.args.push_back(a);
        Arg v;
        v.kind = Arg::Kind::Vector;
        v.text = vec;
        n.args.push_back(v);
        if (std::string(type) == "PAN") {
            Arg zero;
            zero.kind = Arg::Kind::Vector;
            zero.text = "0.000 0.000 0.000";
            n.args.push_back(zero);
        }
        Arg t;
        t.kind = Arg::Kind::Number;
        t.text = "0.000";
        n.args.push_back(t);
        return n;
    };
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%.3f %.3f %.3f",
                  static_cast<double>(g_app->cam.pos[0]),
                  static_cast<double>(g_app->cam.pos[1]),
                  static_cast<double>(g_app->cam.pos[2]));
    const std::string origin = buf;
    std::snprintf(buf, sizeof(buf), "%.3f %.3f %.3f",
                  static_cast<double>(g_app->cam.angles[0]),
                  static_cast<double>(g_app->cam.angles[1]),
                  static_cast<double>(g_app->cam.angles[2]));
    const std::string angles = buf;

    // Beide Befehle als EIN Rueckgaengig-Schritt (insertAfterAll). Vorher
    // zwei insertAfter: ein Strg+Z nahm nur das PAN zurueck, das MOVE blieb
    // stehen (Kartentest 27.09.).
    Path at;
    if (!g_app->doc.insertAfterAll(g_app->selectedPath,
                                   {make("MOVE", origin), make("PAN", angles)}, &at)) {
        return;
    }
    g_app->selectedPath = at;
    rebuildTree();
}

// Die Kartenansicht. Sie steht dort, wo sonst die Ereignisliste ist:
// waehrend man Kameras setzt, braucht man die Befehlsliste nicht, und der
// Skriptablauf bleibt daneben sichtbar - dort landet ja der neue Befehl.
// --- Dateien fuer Modell und Karte -------------------------------------
//
// Jede Ansicht bekommt ihre eigenen Knoepfe. Der Grundsatz dahinter: beim
// Laden wird das Uebliche automatisch mitgesucht - zur .glm die .skin und
// die im Modell vermerkte .gla, zur .bsp die Entities darin. Wer etwas
// anderes will, waehlt es danach von Hand.
void openModelFile() {
    const std::string p = platform::openFileDialog(
        tr(Str::OpenGlm), "Ghoul2 models (*.glm)|*.glm|All files (*.*)|*.*",
        g_app->settings.lastDir);
    if (p.empty()) {
        return;
    }
    diag::Step step("Modell laden: " + p);
    GlmModel gm;
    std::string err;
    if (!readGlm(slurp(p), gm, &err)) {
        step.fail(err);
        platform::showError(p + "\n" + err, tr(Str::AppTitle));
        return;
    }
    g_app->settings.lastDir = directoryOf(p);

    g_app->model = std::move(gm);
    g_app->modelPath = p;
    g_app->modelDir = directoryOf(p);

    // ALLE Haeute daneben sammeln, nicht nur die erste.
    //
    // Ein Modell bringt oft mehrere mit - model_default, model_red,
    // model_blue. Sie entscheiden nicht nur ueber Texturen, sondern auch
    // darueber, WELCHE Flaechen sichtbar sind: eine Zeile "kopf_b,*off"
    // schaltet eine Variante ab. Ohne Auswahl zeichnet man alle Varianten
    // uebereinander.
    g_app->modelSkins.clear();
    g_app->modelSkinIndex = -1;
    for (const std::string& f : platform::listDirectory(g_app->modelDir)) {
        if (f.size() > 5 && f.compare(f.size() - 5, 5, ".skin") == 0) {
            g_app->modelSkins.push_back(f);
        }
    }
    std::sort(g_app->modelSkins.begin(), g_app->modelSkins.end());
    // Die Vorgabehaut zuerst, wenn es sie gibt.
    for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
        if (g_app->modelSkins[i] == "model_default.skin") {
            applySkinByIndex(static_cast<int>(i));
            break;
        }
    }
    if (g_app->modelSkinIndex < 0 && !g_app->modelSkins.empty()) {
        applySkinByIndex(0);
    }
    diag::info(std::to_string(g_app->modelSkins.size()) + " Haeute gefunden");
    g_app->anim = GlaAnimation{};
    g_app->animList.clear();
    g_app->animIndex = -1;
    g_app->animFrame = 0;
    for (int k = 0; k < 3; ++k) {
        g_app->modelCam.pos[k] =
            (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F;
    }
    float size = 0.0F;
    for (int k = 0; k < 3; ++k) {
        size = std::max(size, g_app->model.maxs[k] - g_app->model.mins[k]);
    }
    // Abstand so, dass die Figur das Bild fuellt. 2.2 war zu weit - eine
    // 66 Einheiten hohe Figur stand als Streichholz in der Mitte.
    g_app->modelOrbit.distance = std::max(size * 1.15F, 24.0F);
    g_app->modelCam.angles[0] = 0.0F;
    g_app->modelCam.angles[1] = 180.0F;
    // Die Kamera VOR die Figur setzen: der Drehpunkt liegt in ihrer Mitte.
    {
        float f[3];
        g_app->modelCam.forward(f);
        for (int k = 0; k < 3; ++k) {
            g_app->modelCam.pos[k] =
                (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F -
                f[k] * g_app->modelOrbit.distance;
        }
    }
    g_app->modelDirty = true;
    g_app->leftMode = 2;

    loadModelTextures();
    // Das Skelett gleich mitsuchen - der Pfad steht im Modell.
    loadSkeletonForModel();
}

// Eine Haut aus der Liste anwenden.
//
// Wichtig: das Modell wird VORHER zurueckgesetzt. Eine Haut schaltet Flaechen
// ab, und die bleiben sonst aus, wenn man zu einer Haut wechselt, die sie
// nicht erwaehnt.
void applySkinByIndex(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= g_app->modelSkins.size()) {
        return;
    }
    const std::string path =
        g_app->modelDir + "/" + g_app->modelSkins[static_cast<std::size_t>(index)];
    // Erst von der Platte, dann aus den Archiven.
    //
    // Ein Modell aus einer .pk3 hat keinen Pfad auf der Platte; slurp allein
    // faende die Haut nie, und der Wechsel taete stillschweigend nichts.
    std::string text = slurp(path);
    if (text.empty() && !readFromArchives(path, text)) {
        diag::info("Haut nicht lesbar: " + path);
        return;
    }
    for (GlmSurface& s : g_app->model.surfaces) {
        s.skinnedOff = false;
        s.texture.clear();
    }
    applySkin(text, g_app->model);
    g_app->modelSkinIndex = index;
    loadModelTextures();
    diag::info("Haut: " + g_app->modelSkins[static_cast<std::size_t>(index)]);
}

void openSkinFile() {
    const std::string p = platform::openFileDialog(
        tr(Str::OpenSkin), "Skins (*.skin)|*.skin|All files (*.*)|*.*",
        g_app->modelPath.empty() ? g_app->settings.lastDir
                                 : directoryOf(g_app->modelPath));
    if (p.empty()) {
        return;
    }
    const std::string skin = slurp(p);
    if (skin.empty()) {
        platform::showError(p, tr(Str::AppTitle));
        return;
    }
    applySkin(skin, g_app->model);
    loadModelTextures();
    diag::info("Haut gewechselt: " + p);
}

// Eine .gla von Hand waehlen.
//
// Normalerweise steht der Pfad im Modell, und wir nehmen ihn. Aber ein Mod
// kann eine eigene .gla mitbringen, und beim Bauen einer neuen Animation
// will man sie ausprobieren, bevor sie am gewohnten Ort liegt.
void openSkeletonFile() {
    const std::string p = platform::openFileDialog(
        tr(Str::ModelLoadGla), "Ghoul2 skeleton (*.gla)|*.gla|All files (*.*)|*.*",
        g_app->settings.lastDir);
    if (p.empty()) {
        return;
    }
    diag::Step step("Skelett laden: " + p);
    GlaAnimation a;
    std::string err;
    if (!readGla(slurp(p), a, &err)) {
        step.fail(err);
        platform::showError(p + "\n" + err, tr(Str::AppTitle));
        return;
    }
    // Die animation.cfg liegt daneben und nennt die Abschnitte.
    const std::string cfg = slurp(directoryOf(p) + "/animation.cfg");
    g_app->animList = cfg.empty() ? std::vector<AnimEntry>{}
                                  : parseAnimationCfg(cfg);
    diag::info(std::to_string(a.bones.size()) + " Knochen, " +
               std::to_string(a.numFrames) + " Bilder, " +
               std::to_string(g_app->animList.size()) + " Abschnitte");
    g_app->anim = std::move(a);
    g_app->animIndex = -1;
    g_app->animFrame = 0;
    g_app->modelDirty = true;
}

// Ein Bild auf eine handliche Groesse bringen.
//
// Eine Editoransicht braucht keine 1024er-Textur; bei siebzig Shadern spart
// das ein Vielfaches an Speicher. Der Code stand dreimal woertlich da -
// einmal fuer die Karte, zweimal fuer das Modell.
TextureSet::Tex shrink(const image::Image& im, int maxSize) {
    // Die Rechnung selbst steht im Bildmodul - sie wird an drei Stellen
    // gebraucht und muss ueberall dieselbe sein.
    TextureSet::Tex t;
    image::shrinkTo(im, maxSize, t.width, t.height, t.rgba);
    return t;
}

// Die Texturen des Modells aus den .pk3 holen.
//
// Die .skin nennt je Flaeche eine Datei. Der Weg ist derselbe wie bei der
// Karte, nur dass hier kein Shaderskript dazwischensteht - die .skin zeigt
// direkt auf ein Bild.
// Die Texturen zu einem Modell beschaffen.
//
// Herausgezogen, weil es jetzt zwei Aufrufer gibt: die Modellansicht und die
// Figuren in der Karte.
ModelTextures texturesFor(const GlmModel& model);

void loadModelTextures() {
    diag::Step step("Modelltexturen suchen");
    g_app->modelTextures = texturesFor(g_app->model);
    g_app->modelDirty = true;
}

ModelTextures texturesFor(const GlmModel& model) {
    ModelTextures out;
    if (model.empty()) {
        return out;
    }
    if (g_app->gamePaths.empty()) {
        rescanGamePaths();
    }

    // Die Shaderskripte einmal einlesen. Eine .skin zeigt meistens direkt
    // auf eine Bilddatei - model_blue.skin tut das bei allen 34 Eintraegen -
    // aber bei Lichtschwertern und Glaseffekten steht dort ein Shadername.
    // Dann liegt das Bild in einer "map"-Zeile des Skripts.
    //
    // Erst wenn die direkte Suche nichts ergibt, wird nachgesehen: bei den
    // meisten Modellen ist der Umweg unnoetig.
    // Die Tabelle lebt im Programmzustand, nicht hier.
    //
    // Vorher stand sie als oertliche Veraenderliche genau an dieser Stelle -
    // also wurde sie JE MODELL neu aufgebaut. Bei 40 Archiven, 9014
    // Eintraegen und 64 Figuren stand im Protokoll dutzendfach "9014
    // Shadereintraege gelesen", und das Programm schien zu haengen.
    ShaderMap& shaderMap = g_app->shaderMap;
    auto readShaders = [&]() {
        if (g_app->shaderMapRead) {
            return;
        }
        g_app->shaderMapRead = true;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".shader"})) {
                std::string text;
                if (readFromArchives(f.name, text)) {
                    parseShaderScript(text, shaderMap);
                    parseFogParms(text, g_app->nebelMap);
                }
            }
        }
        diag::info(std::to_string(shaderMap.size()) + " Shadereintraege gelesen");
    };
    constexpr int kMaxSize = 256;
    out.bySurface.resize(model.surfaces.size());
    // Gezaehlt statt je Flaeche gemeldet. Bei einem Sturmtruppler stand
    // sechzehnmal dieselbe Zeile im Protokoll - jede kostet einen Schreib-
    // vorgang, und zusammen machten sie das Protokoll unlesbar.
    int viaShader = 0;

    for (std::size_t i = 0; i < model.surfaces.size(); ++i) {
        const GlmSurface& s = model.surfaces[i];
        if (s.isTag()) {
            continue;
        }
        // --- Wo der Texturname steht -----------------------------------
        //
        // Gemeldet: "die Lichtschwerter sehen nicht richtig aus, da fehlt
        // glaub die Textur?"
        //
        // Bis rc259 wurde NUR s.texture gelesen - das Feld, das eine
        // .skin-Datei fuellt. Figuren haben eine, Waffen nicht: bei
        // models/weapons2/saber/saber_w.glm steht der Name in der
        // HIERARCHIE der Flaeche, also in s.shader:
        //
        //     w_saber   shader="models/weapons2/saber/saber.tga"  texture=""
        //     w_handle  shader="models/weapons2/saber/saber.tga"  texture=""
        //
        // Deshalb stand im Protokoll zehnmal "0 gefunden, 0 nicht
        // gefunden" - es wurde gar nicht erst gesucht, und der Griff kam
        // in der Ersatzfarbe heraus.
        //
        // Die .skin geht VOR: sie ist die Auswahl fuer diese eine Figur,
        // der Shader nur das, was beim Ausleiten des Modells dranstand.
        //
        // rc602 kartenpruefung: Rueckwaertsstriche sind Pfadtrenner. Das .glm
        // von Movie Duels' blaster_w nennt "models\weapons2\blaster_r\blaster_w.jpg" -
        // die Engine liest das wie mit "/" (R_FindShader: generateHashValue
        // "if (letter == '\\') letter = '/'", tr_shader.cpp; FS_ReplaceSeparators
        // beim Oeffnen der Datei). Vorher galt es hier als Werkzeugrest ohne
        // Schraegstrich, und die Waffe am Boden blieb unsichtbar.
        std::string name = s.texture.empty() ? s.shader : s.texture;
        std::replace(name.begin(), name.end(), '\\', '/');
        // --- Was KEIN Dateiname ist ------------------------------------
        //
        // Aus einem Protokoll: 244 nicht gefundene Texturen, davon **240
        // mit dem Namen `[nomaterial]`**. rc260 hat auf `[NoMaterial]`
        // verglichen - buchstabengenau, und die Dateien schreiben es
        // klein. Der Filter griff also nie.
        //
        // Der Rest waren `08 - default` und Aehnliches: Materialnamen aus
        // dem Modellierwerkzeug, die beim Ausleiten im Shaderfeld
        // stehenbleiben.
        //
        // Zwei Regeln statt einer Liste:
        //
        //   1. `[nomaterial]` ohne Ruecksicht auf Gross- und
        //      Kleinschreibung.
        //   2. Ein Texturname hat einen SCHRAEGSTRICH. "models/weapons2/
        //      saber/saber.tga" hat drei, "08 - default" keinen. Wer keinen
        //      hat, ist ein Werkzeugrest und keine Datei.
        //
        // Die zweite Regel gilt nur fuer den Rueckfall auf den Shader. Was
        // in einer .skin steht, wird nicht angezweifelt - dort hat jemand
        // bewusst etwas eingetragen.
        if (name.empty()) {
            continue;
        }
        {
            std::string klein = name;
            for (char& c : klein) {
                c = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
            if (klein == "[nomaterial]") {
                continue;
            }
            if (s.texture.empty() && name.find('/') == std::string::npos) {
                continue;
            }
        }
        std::string data;
        bool got = false;
        std::vector<std::string> tries = textureCandidates(name);

        // Schon einmal geladen? Dann NICHT noch einmal entpacken und
        // entziffern. Der Schluessel wird wie in der Engine gebildet:
        // klein, ohne Endung, nur Schraegstriche.
        // Die GROESSE gehoert in den Schluessel: die Karte verkleinert auf
        // 512, die Figuren auf 256. Ohne sie bekaeme, wer zuerst kommt,
        // recht - und die Lava saehe kloetzchenhaft aus oder eine
        // Figurentextur waere unnoetig gross.
        const std::string schluessel =
            image::mappingName(name) + "@" + std::to_string(kMaxSize);
        const auto imCache = g_app->texCache.find(schluessel);
        if (imCache != g_app->texCache.end()) {
            out.bySurface[i] = imCache->second;
            ++out.found;
            continue;
        }

        for (const std::string& kandidat : tries) {
            if (!readFromArchives(kandidat, data)) {
                continue;
            }
            const image::Image im = image::decode(
                reinterpret_cast<const unsigned char*>(data.data()), data.size());
            if (!im.ok || im.width <= 0 || im.height <= 0) {
                continue;
            }
            TextureSet::Tex t = shrink(im, kMaxSize);
            g_app->texCache[schluessel] = t;
            out.bySurface[i] = std::move(t);
            ++out.found;
            got = true;
            break;
        }
        // Nichts gefunden? Dann ueber das Shaderskript.
        if (!got) {
            readShaders();
            std::string lowered = name;
            for (char& c : lowered) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            // Auch ohne Endung nachsehen: in der .skin steht oft
            // "models/weapons2/saber/blade.tga", im Skript aber
            // "models/weapons2/saber/blade".
            const std::size_t dot = lowered.find_last_of('.');
            const std::size_t slash = lowered.find_last_of('/');
            std::vector<std::string> keys{lowered};
            if (dot != std::string::npos &&
                (slash == std::string::npos || dot > slash)) {
                keys.push_back(lowered.substr(0, dot));
            }
            for (const std::string& key : keys) {
                const auto it = shaderMap.find(key);
                if (it == shaderMap.end()) {
                    continue;
                }
                for (const std::string& kandidat : textureCandidates(it->second.image)) {
                    if (!readFromArchives(kandidat, data)) {
                        continue;
                    }
                    const image::Image im = image::decode(
                        reinterpret_cast<const unsigned char*>(data.data()),
                        data.size());
                    if (!im.ok || im.width <= 0 || im.height <= 0) {
                        continue;
                    }
                    // Auch dieser Weg endet in der Tabelle - unter DEM
                    // Namen, mit dem gesucht wurde. Sonst laeuft jede
                    // Flaeche, deren Bild nur ueber ein Shaderskript zu
                    // finden ist, jedes Mal neu durch die Suche; im
                    // Protokoll sind das die "ueber Shaderskripte"
                    // gezaehlten, und davon gibt es reichlich.
                    TextureSet::Tex t2 = shrink(im, kMaxSize);
                    g_app->texCache[schluessel] = t2;
                    // --- alphaFunc gehoert mit -------------------------
                    //
                    // Der Figurenzeichner prueft den Alphakanal nur noch,
                    // wenn ein Shader es verlangt (mapview.cpp, Notiz bei
                    // `alphaTestet`). Ohne diese Zeile waere er ueberall
                    // aus - und der Geonosianer haette wieder ein schwarzes
                    // Rechteck statt Fluegeln.
                    //
                    // NACH dem Zwischenspeicher gesetzt: der speichert nur
                    // Bildpunkte, und derselbe Bildinhalt kann unter zwei
                    // Shadern verschieden getestet werden.
                    out.bySurface[i] = std::move(t2);
                    out.bySurface[i].alphaTest = it->second.alphaTest;
                    ++out.found;
                    got = true;
                    ++viaShader;
                    break;
                }
                if (got) {
                    break;
                }
            }
        }
        if (!got) {
            ++out.missing;
            diag::info("keine Textur fuer " + s.name + " (" + name + ")");
        }
    }
    // --- rc602: die Leuchtstufen aus dem Shaderskript ----------------------
    //
    // Gemeldet: "es fuehlt sich so an als wuerde etwas fehlen" (md_am_sith).
    // Die Suche oben nimmt das BILD, wenn es eines mit dem Hautnamen gibt.
    // Die Engine fragt zuerst nach dem Shader (R_FindShader: erst
    // ScanAndLoadShaderFiles-Eintrag, dann R_FindImageFile) - und bei
    // "models/players/magnaguard/mangahead" oder ".../sbd/body_cortosis"
    // gibt es einen, dessen letzte Stufe ein Leuchtbild ist:
    //
    //     map models/players/magnaguard/mangahead_glow
    //     blendFunc GL_ONE GL_ONE
    //     glow
    //
    // Diese Stufen werden hier dazugeladen; zeichneFigur legt sie additiv
    // ueber die Grundstufe und gibt sie dem Gluehen. Ohne rgbGen gilt
    // identityLighting (ParseStage) - am Schirm die Bildfarbe selbst, wie
    // bei den Kartenstufen (lichtFuer, gpustate.h).
    out.gluehStufen.assign(model.surfaces.size(), {});
    int leuchtStufen = 0;
    for (std::size_t i = 0; i < model.surfaces.size(); ++i) {
        const GlmSurface& s = model.surfaces[i];
        if (s.isTag()) {
            continue;
        }
        std::string name = s.texture.empty() ? s.shader : s.texture;
        std::replace(name.begin(), name.end(), '\\', '/');   // rc602, wie oben
        if (name.empty() || out.bySurface[i].rgba.empty()) {
            continue;
        }
        std::string key = name;
        for (char& c : key) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const std::size_t dot = key.find_last_of('.');
        const std::size_t slash = key.find_last_of('/');
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
            key = key.substr(0, dot);
        }
        readShaders();
        const auto it = shaderMap.find(key);
        if (it == shaderMap.end()) {
            continue;
        }
        for (const ShaderInfo::Stage& st : it->second.stages) {
            if (!st.glow || st.lightmap || st.image.empty()) {
                continue;
            }
            const std::string sk = image::mappingName(st.image) + "@" + std::to_string(kMaxSize);
            TextureSet::Tex z;
            const auto hit = g_app->texCache.find(sk);
            if (hit != g_app->texCache.end()) {
                z = hit->second;
            } else {
                std::string data;
                for (const std::string& kandidat : textureCandidates(st.image)) {
                    if (!readFromArchives(kandidat, data)) {
                        continue;
                    }
                    const image::Image im = image::decode(
                        reinterpret_cast<const unsigned char*>(data.data()), data.size());
                    if (!im.ok || im.width <= 0 || im.height <= 0) {
                        continue;
                    }
                    z = shrink(im, kMaxSize);
                    g_app->texCache[sk] = z;
                    break;
                }
            }
            if (z.rgba.empty()) {
                continue;
            }
            z.blend = st.blend;
            z.srcFactor = st.srcFactor;
            z.dstFactor = st.dstFactor;
            z.alphaTest = st.alphaTest;
            z.alphaConst = st.alphaConst;
            z.glow = true;
            z.ausSkript = true;
            z.rgbVertex = st.rgbVertex;
            z.cull = it->second.cull;
            z.depthWrite = st.depthWrite;
            z.depthFunc = st.depthFunc;
            out.gluehStufen[i].push_back(std::move(z));
            ++leuchtStufen;
        }
    }
    // --- rc602 kartenpruefung: ALLE Stufen des Shaderskripts ----------------
    //
    // Auf 161 von 278 Karten tragen Figuren Shader mit mehreren Stufen
    // (Glanz, Chrom, getoente Grundstufe der Kampfdroiden, rgbGen wave,
    // animMap). Die Engine nimmt fuer den Hautnamen ZUERST das Skript
    // (R_FindShader) und zeichnet seine Stufen nacheinander
    // (RB_IterateStagesGeneric) - die Grundstufe ist dann die erste Stufe des
    // Skripts, auch wenn sie ein anderes Bild zeigt als der Hautname. Was
    // davon an einer Figur gilt, steht in bhed/figurstufen.h; gezeichnet wird
    // es in gpu::zeichneFigur. Die glow-Stufen sind dabei - die Liste
    // gluehStufen bleibt fuer diese Flaeche leer.
    out.stufen.assign(model.surfaces.size(), {});
    int stufenFlaechen = 0;
    // rc603: die Bildpunkte einmal je Bild (App::figurStufenBilder), nicht je
    // Figur und Stufe kopiert - siehe ModelTextures::FigurStufe::pixel.
    auto ladeStufenBild = [&](const std::string& bildName) -> std::shared_ptr<const TextureSet::Tex> {
        // rc603 Durchsicht: tr.whiteImage - R_CreateBuiltinImages legt "*white"
        // als 8x8 Bild mit allen Bytes 255 an (tr_image.cpp:1273-1276 MD,
        // 1328 OJ). ParseStage bindet es fuer "map $whiteimage" und, an Figuren
        // (lightmapsNone), fuer "map $lightmap" (tr_shader.cpp:1202-1220 MD,
        // 1239-1250 OJ);
        // R_FindImageFile("*white") findet es unter seinem Namen.
        if (bildName == "*white") {
            static const std::shared_ptr<const TextureSet::Tex> kWeiss = [] {
                auto w = std::make_shared<TextureSet::Tex>();
                w->width = 8;
                w->height = 8;
                w->rgba.assign(8U * 8U * 4U, 255U);
                return std::shared_ptr<const TextureSet::Tex>(std::move(w));
            }();
            return kWeiss;
        }
        const std::string sk = image::mappingName(bildName) + "@" + std::to_string(kMaxSize);
        if (const auto da = g_app->figurStufenBilder.find(sk); da != g_app->figurStufenBilder.end()) {
            if (auto lebt = da->second.lock()) { return lebt; }
        }
        std::shared_ptr<TextureSet::Tex> neu;
        const auto hit = g_app->texCache.find(sk);
        if (hit != g_app->texCache.end()) {
            if (hit->second.rgba.empty()) { return nullptr; }
            neu = std::make_shared<TextureSet::Tex>();
            neu->width = hit->second.width;
            neu->height = hit->second.height;
            neu->rgba = hit->second.rgba;
        } else {
            std::string data;
            for (const std::string& kandidat : textureCandidates(bildName)) {
                if (!readFromArchives(kandidat, data)) { continue; }
                const image::Image im =
                    image::decode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
                if (!im.ok || im.width <= 0 || im.height <= 0) { continue; }
                TextureSet::Tex t = shrink(im, kMaxSize);
                g_app->texCache[sk] = t;
                neu = std::make_shared<TextureSet::Tex>(std::move(t));
                break;
            }
        }
        if (!neu) { return nullptr; }
        g_app->figurStufenBilder[sk] = neu;
        return neu;
    };
    for (std::size_t i = 0; i < model.surfaces.size(); ++i) {
        const GlmSurface& s = model.surfaces[i];
        if (s.isTag()) { continue; }
        std::string name = s.texture.empty() ? s.shader : s.texture;
        std::replace(name.begin(), name.end(), '\\', '/');
        if (name.empty()) { continue; }
        std::string key = name;
        for (char& c : key) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        const std::size_t dot = key.find_last_of('.');
        const std::size_t slash = key.find_last_of('/');
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) { key = key.substr(0, dot); }
        readShaders();
        const auto it = shaderMap.find(key);
        if (it == shaderMap.end()) { continue; }
        std::vector<ModelTextures::FigurStufe> liste;
        bool vollstaendig = true;
        // rc604: das erste Stufenbild, das fehlt, entscheidet wie im Renderer
        // (figurShaderMitFehlendemBild, bhed/figurstufen.h): die Stufen davor
        // bleiben; ohne Stufe nichts; ohne Haut tr.defaultShader. Bis rc604
        // zeigte behaved dann nur das Bild des Hautnamens.
        std::size_t ersteFehlende = it->second.stages.size();
        for (std::size_t k = 0; k < it->second.stages.size() && ersteFehlende == it->second.stages.size(); ++k) {
            const ShaderInfo::Stage& st = it->second.stages[k];
            if (st.lightmap || st.weiss) { continue; }   // tr.whiteImage gibt es immer
            const std::vector<std::string> bl = st.animImages.empty() ? std::vector<std::string>{st.image} : st.animImages;
            for (const std::string& b : bl) {
                if (!b.empty() && !ladeStufenBild(b)) {
                    ersteFehlende = k;
                    break;
                }
            }
        }
        const FigurShaderErsatz ersatz =
            figurShaderMitFehlendemBild(ersteFehlende, it->second.stages.size(), !s.texture.empty());
        if (ersatz.ersatzbild || ersatz.unsichtbar) {
            diag::detail("Figurstufen: Bild fehlt in " + key +
                         (ersatz.ersatzbild ? " - tr.defaultShader (Kasten)" : " - keine Stufe, unsichtbar"));
            TextureSet::Tex z;
            z.ausSkript = true;
            z.cull = it->second.cull;
            if (ersatz.ersatzbild) {
                z.width = 16;
                z.height = 16;
                z.rgba = figurErsatzbild();
                z.depthWrite = true;
                out.bySurface[i] = std::move(z);
                out.gluehStufen[i].clear();
                out.stufen[i].clear();
                continue;
            }
            // Keine Stufe (SS_FOG, numUnfoggedPasses 0): eine Stufe, die nichts
            // aendert - blendFunc GL_ZERO GL_ONE, ohne Tiefe.
            z.width = 1;
            z.height = 1;
            z.blend = BlendMode::Alpha;   // wie der Parser fuer GL_ZERO GL_ONE
            z.srcFactor = BlendFactor::Zero;
            z.dstFactor = BlendFactor::One;
            z.depthWrite = false;
            ModelTextures::FigurStufe fs;
            const std::vector<FigurStufenArt> arten = figurStufen(it->second);
            if (!arten.empty()) { fs.art = arten.front(); }
            auto leer = std::make_shared<TextureSet::Tex>(z);
            leer->rgba.assign(4U, 0U);
            fs.bilder.push_back(z);
            fs.pixel.push_back(leer);
            TextureSet::Tex basis = z;
            basis.rgba.assign(4U, 0U);
            out.bySurface[i] = std::move(basis);
            out.gluehStufen[i].clear();
            out.stufen[i].clear();
            out.stufen[i].push_back(std::move(fs));
            ++stufenFlaechen;
            continue;
        }
        for (const FigurStufenArt& art : figurStufen(it->second)) {
            if (art.stufe >= ersatz.stufen) { break; }   // rc604: nur die Stufen vor dem fehlenden Bild
            const ShaderInfo::Stage& st = it->second.stages[art.stufe];
            ModelTextures::FigurStufe fs;
            fs.art = art;
            fs.animFreq = st.animFreq;
            fs.animEinmal = st.animOneShot;
            // rc603 Durchsicht: $whiteimage/$lightmap - tr.whiteImage (siehe oben).
            const std::vector<std::string> bilder = art.weiss                ? std::vector<std::string>{"*white"}
                                                    : st.animImages.empty() ? std::vector<std::string>{st.image}
                                                                            : st.animImages;
            for (const std::string& b : bilder) {
                std::shared_ptr<const TextureSet::Tex> px = ladeStufenBild(b);
                if (!px) {
                    vollstaendig = false;
                    break;
                }
                TextureSet::Tex z;   // Groesse und Zustand, die Bildpunkte in fs.pixel
                z.width = px->width;
                z.height = px->height;
                z.blend = st.blend;
                z.srcFactor = st.srcFactor;
                z.dstFactor = st.dstFactor;
                z.alphaTest = st.alphaTest;
                z.alphaConst = st.alphaConst;
                z.glow = st.glow;
                z.ausSkript = true;
                z.rgbVertex = st.rgbVertex;
                z.cull = it->second.cull;
                z.depthWrite = st.depthWrite;
                z.depthFunc = st.depthFunc;
                // rc604 fix604b (B4): tcMod der Stufe (turb, rotate, scroll ...) -
                // gpumap_win32.cpp packt die Kette je Bild wie bei der Karte.
                z.numTexMods = st.numTexMods;
                for (int k = 0; k < st.numTexMods; ++k) { z.texMods[k] = st.texMods[k]; }
                // rc604: deformVertexes gilt fuer den ganzen Shader (RB_DeformTessGeometry
                // vor allen Stufen) - gpumap_win32.cpp packt es je Stufe in b3.
                z.deformWave = it->second.deformWave;
                z.deformSpread = it->second.deformSpread;
                z.deform2 = it->second.deform2;
                fs.bilder.push_back(std::move(z));
                fs.pixel.push_back(std::move(px));
            }
            if (!vollstaendig) { break; }
            liste.push_back(std::move(fs));
        }
        if (ersatz.stufen < it->second.stages.size()) {
            diag::detail("Figurstufen: Bild fehlt in " + key + " - " + std::to_string(ersatz.stufen) +
                         " Stufe(n) davor bleiben");
        }
        if (!vollstaendig || liste.empty()) {
            continue;
        }
        // Skript vor Bild: die erste Stufe mit ihren Bildpunkten (eine Kopie
        // je Figur wie bisher - bySurface traegt Bildpunkte).
        {
            TextureSet::Tex basis = liste.front().bilder.front();
            basis.rgba = liste.front().pixel.front()->rgba;
            out.bySurface[i] = std::move(basis);
        }
        out.gluehStufen[i].clear();
        out.stufen[i] = std::move(liste);
        ++stufenFlaechen;
    }
    std::string line = std::to_string(out.found) + " gefunden, " +
                       std::to_string(out.missing) + " nicht gefunden";
    if (stufenFlaechen != 0) {
        line += ", " + std::to_string(stufenFlaechen) + " Flaechen mit Shaderstufen";
    }
    if (leuchtStufen != 0) {
        line += ", " + std::to_string(leuchtStufen) + " Leuchtstufen";
    }
    if (viaShader != 0) {
        line += " (" + std::to_string(viaShader) + " ueber Shaderskripte)";
    }
    diag::info(line);
    return out;
}

// Zu einem Animationsnamen die Bildnummer suchen.
//
// Die animation.cfg nennt je Abschnitt Anfang, Laenge und Tempo. Beim
// Abspielen laeuft die Figur durch ihren Abschnitt - deshalb haengt die
// Bildnummer an der Zeit auf der Zeitleiste, nicht an einer festen Zahl.
// Die Abschnittsliste kommt als Argument: jede Figur hat ihre eigene, denn
// ein Modell darf ein anderes Skelett tragen als die Modellansicht gerade
// zeigt. Vorher nahm diese Funktion immer g_app->animList - also die von
// Hand geladene - und die ist bei einer Mission leer. Deshalb stand jede
// Figur regungslos in ihrer Ruhelage.
// Die Modellansicht benutzt weiterhin die von Hand geladene Liste.
int frameForAnimation(const std::string& name, double sinceMs, bool hold) {
    return frameForAnimationIn(g_app->animList, name, sinceMs, hold);
}

// Die Modelle der Figuren beschaffen.
//
// Der Weg ist dreistufig, und jede Stufe kann fehlschlagen:
//   NPC_type          "md_ani_tcwa"      aus dem NPC_spawner der Karte
//   -> playerModel    "anakin_tcw"       aus ext_data/npcs/*.npc
//   -> Modell         models/players/anakin_tcw/model.glm
//
// Fehlt eine Stufe, bleibt die Figur ohne Modell und wird nicht gezeichnet.
// Geraten wird nichts - eine falsche Figur waere schlimmer als keine.
// Zu einer .gla die animation.cfg finden - in der Reihenfolge der Engine.
//
// code/game/NPC_stats.cpp, G_ParseAnimationFile():
//
//     Com_sprintf(skeletonPath, "models/players/%s/%s.cfg", skeletonName, skeletonName);
//     len = gi.RE_GetAnimationCFG(skeletonPath, ...);
//     if ( len <= 0 ) {
//         Com_sprintf(skeletonPath, "models/players/%s/animation.cfg", skeletonName);
//
// Also ERST <name>/<name>.cfg, DANN <name>/animation.cfg. Der erste Weg
// fehlte hier ganz; bei einem Skelett mit eigener .cfg wurden dadurch die
// Abschnitte des _humanoid genommen oder gar keine.
//
// skeletonName ist der LETZTE Teil des .gla-Ordners, nicht der ganze Pfad:
// aus "models/players/_humanoid/_humanoid" wird "_humanoid" (ebenda, ueber
// COM_SkipPath auf dem abgeschnittenen Pfad).
std::vector<AnimEntry> animSectionsFor(const std::string& animFile) {
    // Die Namensregel liegt im Kern (bhed/gla.h) und wird dort von Proben
    // festgehalten - hier bleibt nur das Nachsehen in den Archiven.
    std::string cfg;
    for (const std::string& name : animCfgCandidates(animFile)) {
        if (readFromArchives(name, cfg)) {
            return parseAnimationCfg(cfg);
        }
    }
    return {};
}

// Das Skelett zu einem Modell holen, hoechstens einmal je .gla.
// Wohin zielt camera(FOLLOW, "gruppe", ...) zum Zeitpunkt ms?
//
// Der MITTELPUNKT aller Entities mit diesem cameraGroup -
// CGCam_FollowUpdate() in cg_camera.cpp sammelt alle Treffer und mittelt
// sie. Bewegt das Skript eine davon gerade, gilt ihr AKTUELLER Ort, nicht
// der aus der Karte: sonst zielt die Kamera auf den Startpunkt, waehrend
// die Figur schon woanders steht.
//
// Herausgezogen, weil es an drei Stellen gebraucht wird - beim Abspielen,
// beim Zeichnen der Kamera und beim Abtasten der Bahn. Vorher stand es nur
// an der ersten, und die gezeichnete Kamera zeigte deshalb woandershin als
// die abgespielte.
bool followCentre(const std::string& gruppe, double ms, float out[3]) {
    if (gruppe.empty()) {
        return false;
    }
    float mitte[3] = {0.0F, 0.0F, 0.0F};
    int wieviele = 0;
    const auto klein = [](std::string x) {
        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return x;
    };
    // Die Gruppe einer Entity zu diesem Zeitpunkt: aus der Karte, bis ein
    // SET_CAMERA_GROUP im Ablauf sie aendert (Q3_SetCameraGroup setzt
    // ent->cameraGroup, CGCam_FollowUpdate liest es jedes Bild).
    const auto gruppeVon = [&](const std::string& name, const std::string& ausKarte) -> std::string {
        const auto it = g_app->kameraGruppen.find(klein(name));
        std::string g = ausKarte;
        if (it != g_app->kameraGruppen.end()) {
            for (const auto& [ab, neu] : it->second) {
                if (ab <= ms) { g = neu; }
            }
        }
        return g;
    };
    std::set<std::string> gesehen;
    for (const MapEntity& e : g_app->map.entities) {
        // Eine Figur heisst wie ihr Spawner sie nennt (NPC_targetname), und
        // sie erbt dessen cameraGroup.
        const std::string* npc = e.find("NPC_targetname");
        const std::string name = (npc != nullptr && !npc->empty()) ? *npc : e.targetname;
        if (e.origin.empty() || gruppeVon(name, e.cameraGroup) != gruppe) {
            continue;
        }
        gesehen.insert(klein(name));
        float o[3] = {0.0F, 0.0F, 0.0F};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        for (const Actor& a : g_app->scene.actors) {
            if (a.haveStart && klein(a.name) == klein(name)) {
                const ActorState as = a.at(ms);
                for (int k = 0; k < 3; ++k) {
                    o[k] = as.pos[k];
                }
                // Figuren zielt die Engine auf die AUGEN: + ps.viewheight
                // ("Track to their eyes"). Stehend standheight + offset =
                // DEFAULT_MAXS_2 40 + STANDARD_VIEWHEIGHT_OFFSET -4 = 36.
                o[2] += 36.0F;
                break;
            }
        }
        for (int k = 0; k < 3; ++k) {
            mitte[k] += o[k];
        }
        ++wieviele;
    }
    // Figuren, die erst das Skript in die Gruppe nimmt und die in der Karte
    // keinen Eintrag unter diesem Namen haben (etwa der Spieler).
    for (const Actor& a : g_app->scene.actors) {
        if (!a.haveStart || gesehen.count(klein(a.name)) != 0 || gruppeVon(a.name, "") != gruppe) {
            continue;
        }
        const ActorState as = a.at(ms);
        for (int k = 0; k < 3; ++k) {
            mitte[k] += as.pos[k];
        }
        mitte[2] += (a.brushModel > 0) ? 0.0F : 36.0F;
        ++wieviele;
    }
    if (wieviele == 0) {
        return false;
    }
    for (int k = 0; k < 3; ++k) {
        out[k] = mitte[k] / static_cast<float>(wieviele);
    }
    return true;
}

// Die Kamerastellung MIT FOLLOW (OpenJK/Movie Duels: CGCam_Update mit
// AnglesToAxis nach FollowUpdate und UpdateShake, code/cgame/cg_camera.cpp:
// 1306-1307).
namespace {
constexpr double kFolgeSchritt = 10.0;   // das Bild der Vorschau fuer FOLLOW
CameraState camStateAtOpenJK(double ms) {
    CameraState st = g_app->camTrack.at(ms);
    // Waehrend eines Schwenks zielt FOLLOW nicht (check_follow).
    if (st.followGroup.empty() || st.panning) {
        return st;
    }
    float ziel[3];
    if (!followCentre(st.followGroup, ms, ziel)) {
        return st;
    }
    // --- Nachziehen wie CGCam_FollowUpdate --------------------------------
    //
    // Je Bild: frac = frametime / 100 * followSpeed / 100, dann
    // angles += frac * AngleNormalize180(ziel - angles) - ein Nachziehen mit
    // der Zeitkonstante 100 ms * 100 / speed. Ohne initLerp springt das erste
    // Bild aufs Ziel, danach wird nachgezogen.
    //
    // Nachgerechnet wird in 10-ms-Schritten, aber nur ueber die letzten sechs
    // Zeitkonstanten: was davor lag, ist auf unter ein Prozent abgeklungen.
    const double tau = 100.0 * 100.0 / std::max(1.0F, std::fabs(st.followSpeed));
    const double fenster = 6.0 * tau;
    double t0 = std::max(st.followSinceMs, ms - fenster);
    float blick[3];
    {
        const CameraState anfang = g_app->camTrack.at(t0);
        float z0[3];
        if (t0 <= st.followSinceMs && st.followInitLerp) {
            for (int k = 0; k < 3; ++k) { blick[k] = anfang.angles[k] - ((k < 2) ? anfang.shakeAngles[k] : 0.0F); }
        } else if (followCentre(st.followGroup, t0, z0)) {
            aimAngles(anfang.pos, z0, blick);
        } else {
            for (int k = 0; k < 3; ++k) { blick[k] = anfang.angles[k]; }
        }
    }
    constexpr double kSchritt = kFolgeSchritt;
    const auto frac = static_cast<float>(std::min(1.0, kSchritt / 100.0 * std::fabs(st.followSpeed) / 100.0));
    for (double t = t0 + kSchritt; t <= ms + 1.0e-6; t += kSchritt) {
        const CameraState jetzt = g_app->camTrack.at(t);
        float z[3];
        if (!followCentre(st.followGroup, t, z)) {
            break;
        }
        float soll[3];
        aimAngles(jetzt.pos, z, soll);
        for (int k = 0; k < 3; ++k) {
            const float d = std::fmod(soll[k] - blick[k] + 540.0F, 360.0F) - 180.0F;
            blick[k] += frac * d;
        }
    }
    for (int k = 0; k < 3; ++k) {
        st.angles[k] = blick[k];
    }
    // Das Zittern kommt NACH dem Zielen (CGCam_UpdateShake zuletzt).
    st.angles[0] += st.shakeAngles[0];
    st.angles[1] += st.shakeAngles[1];
    (void)ziel;
    return st;
}
}  // namespace

// Ueberall zu benutzen, wo camTrack.at() gefragt wird - sonst zeigt eine der
// drei Ansichten woandershin.
CameraState camStateAt(double ms) {
    CameraState st = camStateAtOpenJK(ms);
    if (!regelwerkJediOutcast()) { return st; }
    // rc604 fix604 (JK2 Stufe 3): CGCam_Update von Jedi Outcast rechnet die
    // Blickachsen (AnglesToAxis, codeJK2/cgame/cg_camera.cpp:1121) VOR
    // CGCam_FollowUpdate (:1146-1153) und CGCam_UpdateShake (:1176), und
    // danach nicht mehr. Schwenkt die Kamera (PAN, ROFF-Winkel), zeigt das Bild
    // die frischen Winkel ohne Zittern; sonst die cg.refdefViewAngles des
    // vorigen Bilds - FOLLOW und das Zittern der Winkel ein Bild spaeter.
    if (st.panning) {
        st.angles[0] -= st.shakeAngles[0];
        st.angles[1] -= st.shakeAngles[1];
        return st;
    }
    const CameraState vorher = camStateAtOpenJK(ms - kFolgeSchritt);
    for (int k = 0; k < 3; ++k) { st.angles[k] = vorher.angles[k]; }
    return st;
}

// Was jede Figur GERADE tut, in einer Zeile.
//
// Gefragt: "ist das alles dokumentiert im Debug-Output?" - Nein, war es
// nicht. Das Protokoll sagte, WELCHES Skelett eine Figur bekommt, aber
// nichts darueber, was sie damit anstellt. Damit liess sich keine der
// Meldungen dieser Sitzung nachpruefen: "der Kopf bewegt sich nicht
// richtig", "die Transition geht nicht richtig, hab ich das Gefuehl".
//
// Ein Gefuehl ist kein Befund. Diese Zeile macht daraus Zahlen: Animation
// und Bild, der laufende Uebergang samt Anteil, der Kopfwinkel, das Blick-
// und das Watchtarget.
//
// Auf Zuruf, nicht dauernd: bei 62 Figuren und sechzig Bildern je Sekunde
// waeren das 3720 Zeilen in der Sekunde.
// Vorwaerts angekuendigt: der Auszug nennt den Mund mit, und die
// Umsetzung steht weiter unten bei den anderen Helfern der Ansicht.
std::string faceAnimFor(const std::string& figur, double ms);
const App::ActorAssets* hiltFor(const std::string& pfad);
// Griffe und Klingenbilder vorab holen - siehe die Fassung weiter unten.
void preloadSaberAssets();
const Md3Model* weaponModelFor(const std::string& pfad);

void dumpActorsAtPlayhead() {
    char kopf[160];
    std::snprintf(kopf, sizeof(kopf),
                  "--- Figuren bei %.0f ms (%.2f s) ---",
                  g_app->playMs, g_app->playMs / 1000.0);
    // Auch ins gewoehnliche Protokoll - siehe die Notiz beim Zeitknopf in
    // gui/app.cpp. Ein Knopf, dessen Wirkung nur im Detailprotokoll steht,
    // wirkt kaputt.
    diag::detail(kopf);
    diag::info(kopf);
    int gezeigt = 0;
    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        const Actor& a = g_app->scene.actors[ai];
        const ActorState st = a.at(g_app->playMs);
        const App::Skeleton* sk =
            (ai < g_app->actorAssets.size()) ? g_app->actorAssets[ai]->skeleton
                                             : nullptr;

        // Die Bildnummern ueber DENSELBEN Weg wie beim Zeichnen holen -
        // sonst protokolliert man etwas anderes, als man sieht, und der
        // Auszug wuerde eher in die Irre fuehren als helfen.
        int bild = -1;
        int vorher = -1;
        if (sk != nullptr) {
            bild = frameForAnimationIn(sk->sections, st.animation,
                                       g_app->playMs - st.animStartMs,
                                       st.holdAnim, st.animSpeed);
            vorher = frameForAnimationIn(sk->sections, st.prevAnimation,
                                         st.prevAnimSinceMs, st.prevHoldAnim);
        }
        const float l = blendFraction(st, g_app->playMs);

        std::string z = "  " + a.name;
        z += st.visible ? "" : (st.removed ? "  [entfernt]" : "  [unsichtbar]");
        z += "  Anim \"" + st.animation + "\" Bild " + std::to_string(bild);
        if (bild < 0) {
            z += " (dem Skelett unbekannt)";
        }
        // --- Woran man ein Stottern erkennt ---------------------------
        //
        // Gemeldet: "das Einlaufen von Mace stottert ein wenig."
        //
        // Ort und Animationswahl sind nachgemessen sauber. Was fehlt, ist
        // ein Blick auf den ABSCHNITT: wie lang er ist, wie schnell er
        // laeuft und WO im Zyklus die Figur gerade steht.
        //
        // Damit laesst sich der Verdacht pruefen, ohne zu raten. Bei Mace
        // hat BOTH_WALK1 im _humanoid_mace **30 Bilder bei 20 fps**, also
        // 1500 ms je Zyklus - er geht aber 2133 ms. Die Schleife springt
        // also EINMAL mitten im Gehen um, und genau dort waere ein Ruckeln
        // zu erwarten.
        if (sk != nullptr && !st.animation.empty()) {
            for (const AnimEntry& e : sk->sections) {
                if (e.name != st.animation) {
                    continue;
                }
                const double seit = g_app->playMs - st.animStartMs;
                const double zyklus =
                    static_cast<double>(e.numFrames) * 1000.0 /
                    static_cast<double>(std::max(e.fps, 1));
                char az[160];
                std::snprintf(az, sizeof(az),
                              " [%d Bilder ab %d, %d fps, Zyklus %.0f ms;"
                              " laeuft seit %.0f ms = %.2f Zyklen%s]",
                              e.numFrames, e.firstFrame, e.fps, zyklus,
                              seit, (zyklus > 0.0) ? seit / zyklus : 0.0,
                              (e.loopFrame < 0) ? ", EINMALIG" : "");
                z += az;
                break;
            }
        }
        if (l < 1.0F && !st.prevAnimation.empty()) {
            char u[128];
            std::snprintf(u, sizeof(u),
                          "  Uebergang von \"%s\" Bild %d, %.0f %% von %.0f ms",
                          st.prevAnimation.c_str(), vorher,
                          static_cast<double>(l) * 100.0, st.blendMs);
            z += u;
        } else {
            z += "  kein Uebergang";
        }
        char rest[192];
        std::snprintf(rest, sizeof(rest), "  Gier %.1f", st.angles[1]);
        z += rest;
        {
            const auto itn = g_app->npcMap.find(a.npcType);
            if (itn != g_app->npcMap.end() && !itn->second.saber.empty()) {
                z += "  Klinge \"" + itn->second.saber + "\"";
                std::string k = itn->second.saber;
                for (char& c : k) {
                    c = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(c)));
                }
                const auto its = g_app->saberMap.find(k);
                if (its == g_app->saberMap.end()) {
                    z += " (nicht in den .sab-Dateien)";
                } else {
                    z += " -> " + its->second.model;
                    z += (hiltFor(its->second.model) != nullptr)
                             ? " (Griff geladen)"
                             : " (GRIFF NICHT GELADEN)";
                    z += st.saberActive ? "  Klinge AN" : "  Klinge aus";
                    // --- Wo die Klinge WIRKLICH sitzt --------------------
                    //
                    // Gemeldet: "das Lichtschwert sollte vor ihm sein."
                    //
                    // Die Kette ist gegen den Quelltext geprueft - Bolzen
                    // (tr_ghoul2.cpp:2080 ff.), Richtung (-x, ebenda:6114),
                    // kein Versatz bei SABER_SINGLE (ebenda:6731). Sie
                    // stimmt, soweit sich das lesen laesst.
                    //
                    // Was fehlt, sind ZAHLEN: wo landet der Bolzen, und wo
                    // steht die Figur? Erst der Vergleich der beiden sagt,
                    // ob das Schwert an der Hand haengt oder daneben.
                    //
                    // Alles in Figurenkoordinaten - VOR der Drehung um den
                    // Gierwinkel und der Verschiebung an den Ort. Der
                    // Bolzen soll bei einer stehenden Figur ungefaehr auf
                    // Huefthoehe und seitlich vom Rumpf liegen.
                    const App::ActorAssets* hg = hiltFor(its->second.model);
                    if (sk != nullptr && hg != nullptr && bild >= 0) {
                        std::vector<BoneMatrix> welt;
                        sk->anim.worldMatrices(
                            std::clamp(bild, 0, sk->anim.numFrames - 1), welt);
                        BoneMatrix hand{};
                        const GlmModel& figMdl = g_app->actorAssets[ai]->model;
                        const int hs = surfaceIndex(figMdl, "*r_hand");
                        if (hs < 0) {
                            z += "  [KEIN *r_hand im Modell]";
                        } else if (!boltMatrix(figMdl, hs, welt,
                                               sk->anim, hand)) {
                            z += "  [Bolzen *r_hand nicht berechenbar]";
                        } else {
                            char bz[220];
                            std::snprintf(bz, sizeof(bz),
                                          "  Hand bei %.1f/%.1f/%.1f",
                                          hand.m[0][3], hand.m[1][3],
                                          hand.m[2][3]);
                            z += bz;
                            BoneMatrix kl{};
                            int bsx = surfaceIndex(hg->model, "*blade1");
                            if (bsx < 0) {
                                bsx = surfaceIndex(hg->model, "*flash");
                            }
                            if (boltMatrixRigid(hg->model, bsx, kl)) {
                                float wu[3] = {kl.m[0][3], kl.m[1][3],
                                               kl.m[2][3]};
                                float sp[3];
                                // Dieselbe Vorgabe wie beim Zeichnen:
                                // 32 (wp_saberLoad.cpp:364).
                                for (int c = 0; c < 3; ++c) {
                                    sp[c] = wu[c] - kl.m[c][0] * 32.0F;
                                }
                                float a1[3];
                                float b1[3];
                                hand.transform(wu, a1);
                                hand.transform(sp, b1);
                                std::snprintf(
                                    bz, sizeof(bz),
                                    "  Klinge von %.1f/%.1f/%.1f nach "
                                    "%.1f/%.1f/%.1f",
                                    a1[0], a1[1], a1[2], b1[0], b1[1], b1[2]);
                                z += bz;
                            } else {
                                z += "  [kein *blade1 im Griff]";
                            }
                        }
                    }
                }
            }
        }
        // --- Was die Figur gerade in den Haenden haelt ---------------
        //
        // Aus dem Zustand (src/ausruestung.cpp) - damit sich nachlesen
        // laesst, warum ein Griff fehlt oder eine Klinge nicht brennt.
        if (!st.waffe.empty()) {
            z += "  Waffe " + st.waffe;
            for (std::size_t n = 0; n < 2; ++n) {
                const HandBelegung& h = st.hand[n];
                if (h.modell >= 0 && static_cast<std::size_t>(h.modell) < a.modelle.size()) {
                    z += (n == 0) ? "  rechts " : "  links ";
                    z += a.modelle[static_cast<std::size_t>(h.modell)];
                    if (hiltFor(a.modelle[static_cast<std::size_t>(h.modell)]) == nullptr) {
                        z += " (NICHT GELADEN)";
                    }
                }
            }
            for (std::size_t n = 0; n < 2; ++n) {
                const SaberZustand& sz = st.saber[n];
                if (n == 1 && !st.dualSabers) {
                    break;
                }
                char kz[96];
                int an = 0;
                for (int b = 0; b < sz.numBlades && b < kMaxKlingen; ++b) {
                    if (sz.klinge[static_cast<std::size_t>(b)].an) { ++an; }
                }
                const std::string nm =
                    (sz.art >= 0 && static_cast<std::size_t>(sz.art) < a.saberArten.size())
                        ? a.saberArten[static_cast<std::size_t>(sz.art)].name
                        : std::string("-");
                std::snprintf(kz, sizeof(kz), "  Schwert%d \"%s\" %d/%d Klingen an",
                              static_cast<int>(n) + 1, nm.c_str(), an, sz.numBlades);
                z += kz;
            }
        }
        if (st.firing) {
            char fz[110];
            std::snprintf(fz, sizeof(fz),
                          "  SCHIESST (alle %.0f ms, BOTH_ATTACK3)",
                          static_cast<double>(st.shotSpacingMs));
            z += fz;
            // --- Und WOMIT, und wo die Muendung sitzt --------------------
            //
            // Gefragt: "wo bekommen wir das Projektil her?"
            //
            // Die Kette hat vier Glieder, und diese Zeile zeigt sie alle -
            // damit sich beantworten laesst, wo sie reisst, BEVOR
            // irgendetwas gezeichnet wird:
            //
            //   NPC_type -> .npc `weapon` -> weapons.dat -> Modell/Blitz
            //   -> Tag "tag_flash" im Modell
            const auto itw = g_app->npcMap.find(a.npcType);
            if (itw == g_app->npcMap.end() || itw->second.weapon.empty()) {
                z += "  [keine Waffe in der .npc]";
            } else {
                z += "  Waffe " + itw->second.weapon;
                const auto itd = g_app->weaponMap.find(itw->second.weapon);
                if (itd == g_app->weaponMap.end()) {
                    z += " (NICHT in weapons.dat)";
                } else {
                    z += itd->second.muzzleEffect.empty()
                             ? " (ohne Muendungsblitz)"
                             : (" -> " + itd->second.muzzleEffect);
                    // --- Zwei Wege zur Muendung, und welcher gilt -----
                    //
                    // Berichtigt: "SBD schiesst es aus der Hand, deshalb
                    // gibt es kein eigenes Waffenmodell."
                    //
                    // Also nicht immer tag_flash. Wird ein Waffenmodell
                    // gezeichnet, sitzt die Muendung in dessen Tag; sonst
                    // rechnet die Engine sie aus dem Entityursprung
                    // (CalcMuzzlePoint, g_weapon.cpp:449 ff.).
                    //
                    // Der Auszug nennt BEIDE, samt dem Weg - so laesst sich
                    // sehen, welcher greift und ob er plausibel liegt.
                    const Md3Model* wm = weaponModelFor(itd->second.model);
                    const Md3Tag* tf =
                        (wm != nullptr) ? wm->tag("tag_flash") : nullptr;
                    if (tf != nullptr) {
                        char tz[110];
                        std::snprintf(tz, sizeof(tz),
                                      "  Muendung am Modell %.1f/%.1f/%.1f",
                                      static_cast<double>(tf->origin[0]),
                                      static_cast<double>(tf->origin[1]),
                                      static_cast<double>(tf->origin[2]));
                        z += tz;
                    }
                    float mp[3] = {};
                    muzzlePoint(st.pos, st.angles[1], itw->second.weapon, mp);
                    char mz[130];
                    std::snprintf(mz, sizeof(mz),
                                  "  Muendung aus dem Ursprung "
                                  "%.0f/%.0f/%.0f%s",
                                  static_cast<double>(mp[0]),
                                  static_cast<double>(mp[1]),
                                  static_cast<double>(mp[2]),
                                  (tf == nullptr) ? " (gilt)" : "");
                    z += mz;
                }
            }
        }
        {
            const std::string mund = faceAnimFor(a.name, g_app->playMs);
            z += mund.empty() ? "  Mund still" : ("  Mund \"" + mund + "\"");
        }
        if (!st.torsoAnimation.empty()) {
            int tb = -1;
            if (sk != nullptr) {
                tb = frameForAnimationIn(sk->sections, st.torsoAnimation,
                                         g_app->playMs - st.torsoStartMs,
                                         st.torsoHold);
            }
            z += "  Oberkoerper \"" + st.torsoAnimation + "\" Bild " +
                 std::to_string(tb);
        }
        if (st.hasWatchTarget) {
            std::snprintf(rest, sizeof(rest), "  Watchtarget an (%.0f/%.0f)",
                          st.watchAt[0], st.watchAt[1]);
            z += rest;
        }
        if (st.hasLookTarget) {
            const float dx = st.lookAt[0] - st.pos[0];
            const float dy = st.lookAt[1] - st.pos[1];
            const float ziel =
                std::atan2(dy, dx) * 180.0F / 3.14159265F;
            const float roh = shortestAngleDelta(st.angles[1], ziel);
            const float kw =
                std::clamp(roh, kHeadYawClampMin, kHeadYawClampMax);
            std::snprintf(rest, sizeof(rest),
                          "  Blickziel (%.0f/%.0f), Ausschlag %.1f%s "
                          "-> Brust %.1f Hals %.1f Kopf %.1f",
                          st.lookAt[0], st.lookAt[1], kw,
                          (roh < kHeadYawClampMin || roh > kHeadYawClampMax)
                              ? " (am Anschlag)"
                              : "",
                          // Dieselbe Verteilung wie im Zeichner
                          // (mapview.cpp, kette[]) - der Auszug soll
                          // zeigen, was WIRKLICH auf die Knochen geht.
                          kw * 0.10F, kw * 0.30F, kw * 0.60F);
            z += rest;
        } else {
            z += "  kein Blickziel";
        }
        if (sk == nullptr) {
            z += "  [KEIN Skelett]";
        }
        diag::detail(z);
        ++gezeigt;
    }
    if (gezeigt == 0) {
        diag::detail("  (keine Figuren)");
    }
}

// Welche Mundanimation eine Figur gerade spielt, oder leer.
//
// Gemeldet: "face animationen fehlen noch."
//
// Der Weg, nachgelesen und nicht geraten:
//
//   1. Welcher Klang laeuft an DIESER Figur? Die Zeitleiste fuehrt je
//      affect eine Spur, und ihr Name ist der Figurenname. Der SPAETESTE
//      begonnene gewinnt - dieselbe Regel, nach der auch das Abspielen
//      beim Einschalten des Tons einsteigt.
//   2. Wie laut ist er an dieser Stelle? bhed::sound::voiceLevel, Zeile
//      fuer Zeile nach snd_dma.cpp:2333 ff.
//   3. Welche Animation gehoert zu dieser Stufe? cg_players.cpp:5191:
//          anim = FACE_TALK1 + VoiceVolume - 1;
//      und bei -1 (spricht, aber gerade still) FACE_TALK0
//      (ebenda:5200).
//
// Nicht nachgebaut: das Blinzeln und die Leerlaufmimik. Beide haengen in
// der Engine an Zufallszeitgebern (Q_flrand(4000,8000) fuer den Lidschlag,
// cg_players.cpp:5159) - etwas Zufaelliges in einer Vorschau, die man
// anhaelt und zurueckspult, waere kein Gewinn, sondern Unruhe.
std::string faceAnimFor(const std::string& figur, double ms) {
    const TimelineEvent* laufend = nullptr;
    for (const TimelineTrack& track : g_app->timeline.tracks) {
        if (track.entity != figur) {
            continue;
        }
        for (const TimelineEvent& e : track.events) {
            if (e.kind != TimelineEvent::Kind::Sound || e.sound.empty()) {
                continue;
            }
            if (e.startMs <= ms &&
                (laufend == nullptr || e.startMs > laufend->startMs)) {
                laufend = &e;
            }
        }
    }
    if (laufend == nullptr) {
        return {};
    }
    const sound::Sound* s = soundFor(laufend->sound);
    if (s == nullptr) {
        return {};
    }
    const int stufe = sound::voiceLevel(*s, ms - laufend->startMs);
    if (stufe == 0) {
        return {};        // spricht nicht (mehr)
    }
    if (stufe < 0) {
        return "FACE_TALK0";
    }
    return "FACE_TALK" + std::to_string(stufe);
}

// Was spielt eine Entity, die per `use` angesprochen wird?
//
// Gemeldet: "mir fehlen noch Sounds, Lichtschwerter und Musik. Nur die
// Stimmen gehen bisher."
//
// Die Stimmen sind die einzigen, die als `sound`-Befehl IM SKRIPT stehen -
// 22 Stueck in der ganzen Mission. Alles andere haengt an Entities der
// KARTE, die das Skript mit `use` anspricht:
//
//   target_play_music   Schluessel "music"  -> g_target.cpp:1225 f.
//                       target_play_music_use setzt CS_MUSIC, und
//                       CG_StartMusic (cg_main.cpp:2953) spielt es ab.
//   target_speaker      Schluessel "noise"  -> g_target.cpp:243
//
// In md_ga_jedi sind das sieben Musikstuecke (mus1..mus5, mus_boss,
// mus_bossintro) und zwei Lautsprecher (hammer_sound, march1). Die
// Skripte rufen mus1, mus2, mus3, mus5 und march1 auf.
//
// Andere Ziele - func_door, trigger_once, target_autosave, fx_runner -
// geben keinen Klang her und liefern hier leer.
// Welchen Klang macht die Klinge dieser Figur beim Ein- oder Ausschalten?
//
// Die ganze Kette an einer Stelle, damit sie nachvollziehbar bleibt:
//
//   Figur          -> NPC_type aus dem NPC_spawner der Karte
//   -> .npc-Datei  -> Schluessel `saber`
//   -> .sab-Eintrag-> soundOn / soundOff
//
// Reisst die Kette irgendwo, gelten die Vorgaben aus
// wp_saberLoad.cpp:368 ff. Das ist kein Notbehelf, sondern das, was die
// Engine tut: Saber_SetDefaults setzt sie, BEVOR die .sab-Datei gelesen
// wird, und was dort fehlt, bleibt stehen. Gemessen an den 28 .sab-Dateien
// aus MD_Hilts und assets1: 254 Eintraege, 9 davon ohne eigenes soundOn.
// Die Farbe einer Klinge aus dem Namen in der .sab-Datei.
//
// Gezaehlt ueber die 31 .sab-Dateien: rot 68, random 64, blau 52, gruen 31,
// lila 13, gelb 12, dazu je zwei bis drei orange, weiss, schwarz und
// unstable_red.
//
// "random" waehlt die Engine beim Spawnen zufaellig (SABER_RED bis
// SABER_PURPLE). In einer Vorschau, die man anhaelt und zurueckspult, waere
// Zufall Unruhe statt Hilfe - deshalb blau, die haeufigste feste Farbe.
// Die beiden Bilder einer Klinge, nach ihrer Farbe.
//
// Gemeldet: "besser, aber noch nicht perfekt."
//
// Die Klinge war ein Rechteck in einer Farbe. In der Engine sind es zwei
// TEXTUREN, und welche, entscheidet die Farbe (cg_players.cpp:5763 ff.):
//
//     case SABER_BLUE:
//         glow  = cgs.media.blueSaberGlowShader;
//         blade = cgs.media.blueSaberCoreShader;
//
// Die Shader dahinter zeigen auf gfx/effects/sabers/<farbe>_glow2 und
// <farbe>_line. Nachgemessen an den echten Dateien aus assets1:
//
//   blue_glow2  128x128, quer 0 .. 140 .. 0 - ein weicher Verlauf, und der
//               Spitzenwert ist nur 140 von 255
//   blue_line    64x256, quer 0 .. 255 .. 0 mit breitem Plateau, LAENGS
//               durchgehend 252 - ein Band mit weichen Raendern
//
// Diese Verlaeufe nachzurechnen hiesse raten. Die Bilder liegen vor.
//
// Vorhanden sind sie fuer rot, orange, gelb, gruen, blau und lila - genau
// die sechs, die SABER_RED..SABER_PURPLE aufzaehlen. Alles andere (weiss,
// schwarz, unstable_red) faellt auf blau zurueck, so wie es auch in
// CG_DoSaber keinen eigenen Zweig bekaeme.
void meldeNachladen(const char* was, const std::string& name, double ms);

// Die Fernebene aus der Kartengroesse, nicht fest.
//
// 20000 war zu knapp: duel_kamino_lp misst 34816 Einheiten in der Breite und
// hat eine Diagonale von 48229. Wer von einer Ecke zur anderen schaut,
// verliert alles dahinter. Der GPU-Weg hatte die feste Zahl noch.
float kartenFern() {
    float diag = 0.0F;
    for (int k = 0; k < 3; ++k) {
        const float d = g_app->geo.maxs[k] - g_app->geo.mins[k];
        diag += d * d;
    }
    return std::max(std::sqrt(diag) * 1.2F, 8000.0F);
}

const TextureSet::Tex* bladeTexture(const std::string& farbe, bool glow) {
    static const char* const kBekannt[] = {"red",  "orange", "yellow",
                                           "green", "blue",  "purple"};
    std::string k = farbe;
    for (char& c : k) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    std::string gewaehlt = "blue";
    for (const char* n : kBekannt) {
        if (k.find(n) != std::string::npos) {
            gewaehlt = n;
            break;
        }
    }
    // Eine RGB-Farbe ("xff3f00"): die Engine faerbt dafuer ein eigenes Bild
    // ein (rgbSaberGlowShader, cg_players.cpp:7691). Das gibt es hier nicht;
    // das NAECHSTE der sechs Bilder ist naeher an der Wahrheit als blau.
    float rgb[3];
    if (saberFarbeRgb(farbe, rgb)) {
        static constexpr float kBezug[6][3] = {{1.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 0.0F},
                                               {1.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F},
                                               {0.0F, 0.0F, 1.0F}, {0.5F, 0.0F, 1.0F}};
        float best = 1.0e9F;
        for (int i = 0; i < 6; ++i) {
            float d = 0.0F;
            for (int c = 0; c < 3; ++c) {
                const float x = rgb[c] - kBezug[i][c];
                d += x * x;
            }
            if (d < best) {
                best = d;
                gewaehlt = kBekannt[i];
            }
        }
    }
    const std::string pfad = "gfx/effects/sabers/" + gewaehlt +
                             (glow ? "_glow2" : "_line");
    const auto have = g_app->texCache.find(pfad);
    if (have != g_app->texCache.end()) {
        return have->second.empty() ? nullptr : &have->second;
    }
    // Ab hier wird MITTEN IM BILD geladen - siehe meldeNachladen.
    const auto ladeBeginn = std::chrono::steady_clock::now();
    TextureSet::Tex t;
    for (const std::string& versuch : textureCandidates(pfad)) {
        std::string data;
        if (!readFromArchives(versuch, data)) {
            continue;
        }
        const image::Image im = image::decode(
            reinterpret_cast<const unsigned char*>(data.data()), data.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) {
            continue;
        }
        t = shrink(im, 256);
        break;
    }
    if (t.empty()) {
        diag::detail("Klingenbild " + pfad + " nicht gefunden");
    }
    // AUCH den Fehlschlag merken - sonst wird je Bild erneut gesucht.
    const auto put = g_app->texCache.emplace(pfad, std::move(t));
    meldeNachladen(
        "Klingenbild", pfad,
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - ladeBeginn).count());
    return put.first->second.empty() ? nullptr : &put.first->second;
}

// --- Schwertspur (rc592) -----------------------------------------------------------
//
// cg_players.cpp (CG_AddSaberBlade): solange das Schwert "in Aktion" ist,
// entsteht je Bild ein Band vom alten zum neuen Stand der Klinge (Spitze 3
// Einheiten verlaengert), mit gfx/effects/sabers/saberBlur in der Farbe der
// Klinge; es lebt Dauer/5 (die Dauer kommt aus dem Schwertzug, 75 bis 300 ms -
// hier 300/5 = 60). "In Aktion" setzt das Spiel bei Schwertzuegen; hier gilt:
// die Spitze bewegt sich schnell (mehr als 150 Einheiten je Sekunde).
namespace {
struct SpurProbe {
    double ms = 0.0;
    float a[3]{};
    float b[3]{};
};
std::map<std::uint64_t, std::deque<SpurProbe>> g_spuren;
constexpr double kSpurLeben = 60.0;

const TextureSet::Tex* spurBild() {
    static const TextureSet::Tex* bild = nullptr;
    static bool gesucht = false;
    static int stand = -1;
    if (gesucht && stand == g_app->texCacheStand) { return bild; }
    gesucht = true;
    stand = g_app->texCacheStand;
    const std::string pfad = "gfx/effects/sabers/saberBlur";
    const auto have = g_app->texCache.find(pfad);
    if (have != g_app->texCache.end()) {
        bild = have->second.empty() ? nullptr : &have->second;
        return bild;
    }
    TextureSet::Tex t;
    for (const std::string& versuch : textureCandidates(pfad)) {
        std::string data;
        if (!readFromArchives(versuch, data)) { continue; }
        const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) { continue; }
        t = shrink(im, 256);
        break;
    }
    const auto put = g_app->texCache.emplace(pfad, std::move(t));
    bild = put.first->second.empty() ? nullptr : &put.first->second;
    return bild;
}

void schwertSpur(std::uint64_t schluessel, const float wurzel[3], const float spitze[3], const KlingeDraw& kd,
                 std::vector<gpu::KlingenQuad>& aus) {
    if (!g_app->zeigeSpuren) { return; }
    const double ms = g_app->playMs;
    float dir[3] = {spitze[0] - wurzel[0], spitze[1] - wurzel[1], spitze[2] - wurzel[2]};
    const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len < 0.5F) { return; }
    std::deque<SpurProbe>& d = g_spuren[schluessel];
    // Zurueckgespult oder lange nichts: von vorn (cg_players.cpp: "lastTime >
    // cg.time" und der 300-ms-Schnitt fuer alte Stuecke).
    if (!d.empty() && (ms < d.back().ms - 0.5 || ms - d.back().ms > 300.0)) { d.clear(); }
    if (d.empty() || ms - d.back().ms > 2.0) {
        SpurProbe p;
        p.ms = ms;
        for (int k = 0; k < 3; ++k) {
            p.a[k] = wurzel[k];
            p.b[k] = spitze[k] + dir[k] / len * 3.0F;
        }
        d.push_back(p);
    }
    while (d.size() > 2U && ms - d[1].ms > kSpurLeben) { d.pop_front(); }
    if (d.size() < 2U) { return; }
    const TextureSet::Tex* bild = spurBild();
    for (std::size_t i = 1; i < d.size(); ++i) {
        const SpurProbe& alt = d[i - 1];
        const SpurProbe& neu = d[i];
        const double alter = ms - alt.ms;
        if (alter > kSpurLeben || neu.ms - alt.ms <= 0.0) { continue; }
        // In Aktion? Die Spitze muss sich schnell genug bewegen.
        const float dx = neu.b[0] - alt.b[0];
        const float dy = neu.b[1] - alt.b[1];
        const float dz = neu.b[2] - alt.b[2];
        const double tempo = std::sqrt(dx * dx + dy * dy + dz * dz) / (neu.ms - alt.ms);
        if (tempo < 0.15) { continue; }
        gpu::KlingenQuad q;
        for (int k = 0; k < 3; ++k) {
            q.ecke[0][k] = neu.b[k];
            q.ecke[1][k] = alt.b[k];
            q.ecke[2][k] = alt.a[k];
            q.ecke[3][k] = neu.a[k];
            // Die Farbe der Spur: wie CG_RGBForSaberColor (das Licht), nicht
            // das Klingenbild.
            q.farbe[k] = std::clamp(kd.licht[k] * 255.0F, 0.0F, 255.0F);
        }
        q.tex = bild;
        q.spur = true;
        q.staerke = static_cast<float>(1.0 - alter / kSpurLeben);
        aus.push_back(q);
        ++g_app->spurBaender;
    }
}
}  // namespace

void bladeColorFor(const std::string& name, std::uint8_t out[3]) {
    struct Eintrag {
        const char* name;
        std::uint8_t rgb[3];
    };
    static constexpr Eintrag kFarben[] = {
        {"red", {255, 70, 60}},      {"orange", {255, 150, 60}},
        {"yellow", {255, 235, 90}},  {"green", {90, 255, 110}},
        {"blue", {110, 180, 255}},   {"purple", {200, 120, 255}},
        {"white", {245, 245, 255}},  {"black", {90, 60, 130}},
    };
    std::string k = name;
    for (char& c : k) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    // RGB-Farben ("xff3f00") und Farbtoene (Q_parseSaberColor).
    float rgb[3];
    if (saberFarbeRgb(name, rgb)) {
        for (int c = 0; c < 3; ++c) {
            out[c] = static_cast<std::uint8_t>(std::clamp(rgb[c], 0.0F, 1.0F) * 255.0F + 0.5F);
        }
        return;
    }
    // "unstable_red" und Aehnliches: der Farbname steht vorn.
    for (const Eintrag& e : kFarben) {
        if (k.find(e.name) != std::string::npos) {
            out[0] = e.rgb[0];
            out[1] = e.rgb[1];
            out[2] = e.rgb[2];
            return;
        }
    }
    out[0] = 110;
    out[1] = 180;
    out[2] = 255;
}

// Die Farbe des Schwertlichts - CG_RGBForSaberColor (cg_players.cpp der
// Mod), Wort fuer Wort. Eine RGB-Farbe ("xff3f00") leuchtet in genau dieser
// Farbe; `custom` nimmt saberDLightColor aus der .sab, das hier nicht
// gelesen wird - weiss ist die Vorgabe der Engine dafuer.
void saberLichtFarbe(const std::string& name, float out[3]) {
    struct Eintrag {
        const char* name;
        float rgb[3];
    };
    static constexpr Eintrag kFarben[] = {
        {"unstable_red", {1.0F, 0.2F, 0.2F}}, {"red", {1.0F, 0.2F, 0.2F}},
        {"orange", {1.0F, 0.5F, 0.1F}},       {"yellow", {1.0F, 1.0F, 0.2F}},
        {"green", {0.2F, 1.0F, 0.2F}},        {"blue", {0.2F, 0.4F, 1.0F}},
        {"purple", {0.9F, 0.2F, 1.0F}},       {"black", {1.0F, 1.0F, 1.0F}},
        {"white", {1.0F, 1.0F, 1.0F}},
    };
    if (saberFarbeRgb(name, out)) {
        return;
    }
    std::string k = name;
    for (char& c : k) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    for (const Eintrag& e : kFarben) {
        if (k.find(e.name) != std::string::npos) {
            out[0] = e.rgb[0];
            out[1] = e.rgb[1];
            out[2] = e.rgb[2];
            return;
        }
    }
    out[0] = out[1] = out[2] = 1.0F;
}

// Ein Bild aus gfx/, das kein Shader der Karte nennt - der Kreis fuer das
// dynamische Licht (gfx/2d/dlight, R_CreateDlightImage) und der fuer den
// Blobschatten (gfx/damage/shadow, Shader markShadow). Zwischengespeichert
// wie die Klingenbilder, auch der Fehlschlag.
const TextureSet::Tex* gfxBild(const std::string& pfad) {
    const auto have = g_app->texCache.find(pfad);
    if (have != g_app->texCache.end()) {
        return have->second.empty() ? nullptr : &have->second;
    }
    TextureSet::Tex t;
    for (const std::string& versuch : textureCandidates(pfad)) {
        std::string data;
        if (!readFromArchives(versuch, data)) {
            continue;
        }
        const image::Image im = image::decode(
            reinterpret_cast<const unsigned char*>(data.data()), data.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) {
            continue;
        }
        t = shrink(im, 256);
        break;
    }
    if (t.empty()) {
        diag::detail("Bild " + pfad + " nicht gefunden");
    }
    const auto put = g_app->texCache.emplace(pfad, std::move(t));
    return put.first->second.empty() ? nullptr : &put.first->second;
}

// --- Der Blobschatten einer Figur -------------------------------------------
//
// _PlayerShadow (cg_players.cpp der Mod):
//
//     trace: Kasten -7 -7 0 .. 7 7 2 vom Ursprung 128 Einheiten nach unten,
//            MASK_PLAYERSOLID
//     kein Treffer (fraction 1) oder ganz im Festen: kein Schatten
//     CG_ImpactMark(markShadow, endpos, plane.normal, yaw,
//                   1, 1, 1, 1 - fraction, qfalse, radius, qtrue)
//
// behaved spurt gegen die Dreiecke der Karte, nicht gegen Brushes: der
// Kasten wird zu fuenf Strahlen (Mitte und vier Ecken), der hoechste
// Treffer gilt wie beim Kasten. Nicht feste Flaechen (Wasser, Gras) laesst
// MASK_PLAYERSOLID durchfallen - hier ebenso.
bool blobSchattenFuer(const float ursprung[3], float radius, gpu::BlobSchatten& aus) {
    constexpr float kSchattenWeg = 128.0F;            // SHADOW_DISTANCE
    constexpr std::uint32_t kContentsSolid = 0x1U;    // CONTENTS_SOLID
    const BspGeometry& geo = g_app->geo;
    const float versatz[5][2] = {{0, 0}, {-7, -7}, {7, -7}, {-7, 7}, {7, 7}};
    float besteFraktion = 1.0F;
    float normale[3] = {0.0F, 0.0F, 1.0F};
    for (const auto& v : versatz) {
        float start[3] = {ursprung[0] + v[0], ursprung[1] + v[1], ursprung[2]};
        const float ende[3] = {start[0], start[1], ursprung[2] - kSchattenWeg};
        // Durch nicht Festes hindurch weiterspuren - hoechstens ein paar Mal.
        for (int versuch = 0; versuch < 4; ++versuch) {
            if (start[2] <= ende[2]) {
                break;
            }
            const TraceTreffer t = traceRay(geo, start, ende);
            if (!t.hit) {
                break;
            }
            bool fest = true;
            if (t.surface >= 0 && static_cast<std::size_t>(t.surface) < geo.surfaces.size()) {
                const int si = geo.surfaces[static_cast<std::size_t>(t.surface)].shader;
                if (si >= 0 && static_cast<std::size_t>(si) < geo.shaders.size()) {
                    fest = (geo.shaders[static_cast<std::size_t>(si)].contentFlags &
                            kContentsSolid) != 0U;
                }
            }
            if (fest) {
                const float f = (ursprung[2] - t.point[2]) / kSchattenWeg;
                if (f < besteFraktion) {
                    besteFraktion = f;
                    for (int k = 0; k < 3; ++k) { normale[k] = t.normal[k]; }
                }
                break;
            }
            start[2] = t.point[2] - 0.5F;
        }
    }
    if (besteFraktion >= 1.0F) {
        return false;
    }
    besteFraktion = std::max(besteFraktion, 0.0F);
    aus.ort[0] = ursprung[0];
    aus.ort[1] = ursprung[1];
    aus.ort[2] = ursprung[2] - besteFraktion * kSchattenWeg;
    for (int k = 0; k < 3; ++k) { aus.normale[k] = normale[k]; }
    aus.radius = radius;
    aus.alpha = 1.0F - besteFraktion;
    return true;
}

// Die Groesse eines NPC-Typs aus seiner .npc (scale / scaleX/Y/Z).
void npcSkalaFuer(const std::string& npcType, float aus[3]) {
    aus[0] = aus[1] = aus[2] = 1.0F;
    auto it = g_app->npcMap.find(npcType);
    if (it == g_app->npcMap.end()) {
        std::string k = npcType;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        it = g_app->npcMap.find(k);
    }
    if (it != g_app->npcMap.end()) {
        for (int i = 0; i < 3; ++i) { aus[i] = it->second.skala[i]; }
    }
}

// In welchem Nebel steht dieser Punkt? R_GComputeFogNum (tr_ghoul2.cpp):
// der erste, dessen Kasten die Figur ganz enthaelt - der globale Nebel hat
// einen Kasten ueber die ganze Welt. Ohne Radius (refEntity_t.radius setzt
// cgame fuer Figuren nicht) bleibt davon der Ursprung.
int nebelFuer(const float ort[3]) {
    const std::vector<BspGeometry::Nebel>& n = g_app->geo.nebel;
    for (std::size_t i = 0; i < n.size(); ++i) {
        if (n[i].gueltig && ort[0] >= n[i].mins[0] && ort[0] <= n[i].maxs[0] && ort[1] >= n[i].mins[1] &&
            ort[1] <= n[i].maxs[1] && ort[2] >= n[i].mins[2] && ort[2] <= n[i].maxs[2]) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Die Nebel der Karte mit Farbe und Dichte aus `fogparms` (R_LoadFogs).
// Fehlt der Shader, nimmt die Engine Rot und 250 ("bad shader!!") - so auch
// hier, damit ein fehlender Shader auffaellt statt still zu fehlen.
std::vector<gpu::NebelGpu> nebelListe() {
    std::vector<gpu::NebelGpu> aus;
    for (const BspGeometry::Nebel& n : g_app->geo.nebel) {
        gpu::NebelGpu g;
        g.gueltig = n.gueltig;
        std::string k = n.shader;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        float tiefe = 250.0F;
        const auto it = g_app->nebelMap.find(k);
        if (it != g_app->nebelMap.end()) {
            for (int c = 0; c < 3; ++c) { g.farbe[c] = it->second.farbe[c]; }
            tiefe = it->second.tiefe;
        }
        g.tcScale = 1.0F / (std::max(tiefe, 1.0F) * 8.0F);
        g.hatFlaeche = n.hatFlaeche;
        for (int c = 0; c < 4; ++c) { g.ebene[c] = n.ebene[c]; }
        aus.push_back(g);
    }
    return aus;
}

// Der Radius des Blobschattens nach der Klasse aus der .npc
// (CG_PlayerShadow): Rancor 64, AT-ST 64 (dazu zwei kleine unter den
// Fuessen, `fuesse`), Sandwurm keiner.
float schattenRadiusFuer(const std::string& npcType, bool* fuesse = nullptr) {
    if (fuesse != nullptr) { *fuesse = false; }
    auto it = g_app->npcMap.find(npcType);
    if (it == g_app->npcMap.end()) {
        std::string k = npcType;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        it = g_app->npcMap.find(k);
    }
    if (it == g_app->npcMap.end()) {
        return 16.0F;
    }
    std::string kl = it->second.klasse;
    for (char& c : kl) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    if (kl == "CLASS_SAND_CREATURE") {
        return 0.0F;
    }
    if (kl == "CLASS_ATST" && fuesse != nullptr) {
        *fuesse = true;
    }
    if (kl == "CLASS_RANCOR" || kl == "CLASS_ATST") {
        return 64.0F;
    }
    return 16.0F;
}

std::string saberSoundFor(const Actor& a, bool an, double ms) {
    // Aus dem ZUSTAND zur Zeit des Befehls, nicht mehr aus der .npc: ein
    // SET_SABER1 vorher hat das Schwert - und damit seine Klaenge - schon
    // getauscht. Die Vorgaben aus wp_saberLoad.cpp:368 ff. stecken bereits
    // in der SaberArt (saberArtIndex).
    const ActorState st = a.at(ms);
    // Haelt die Figur kein Schwert, laeuft SET_SABERACTIVE in der Engine
    // ins Leere (Q3_SetSaberActive: "is not using a saber!") - also auch
    // kein Klang.
    if (!haeltSaber(st)) {
        return {};
    }
    const int art = st.saber[0].art;
    if (art >= 0 && static_cast<std::size_t>(art) < a.saberArten.size()) {
        const SaberArt& d = a.saberArten[static_cast<std::size_t>(art)];
        return an ? d.soundOn : d.soundOff;
    }
    // Die Nullstruktur (kein `saber` in der .npc) hat keine Klinge, die
    // zuenden koennte - und keinen Klang.
    return {};
}

std::string useSoundFor(const std::string& ziel, bool* istMusik) {
    if (istMusik != nullptr) {
        *istMusik = false;
    }
    if (ziel.empty()) {
        return {};
    }
    const auto gleich = [](const std::string& a, const std::string& b) {
        return a.size() == b.size() &&
               std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                   return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
               });
    };
    for (const MapEntity& e : g_app->map.entities) {
        // G_Find vergleicht mit Q_stricmp - ohne Gross/klein.
        if (!gleich(e.targetname, ziel)) {
            continue;
        }
        // Der Schluessel entscheidet, nicht der Klassenname: eine Karte
        // darf beides tragen, und wer nach der Klasse geht, verpasst
        // Sonderfaelle wie target_speaker mit music-Schluessel.
        //
        // Und der Schluessel sagt auch, WOHIN: Musik ist ein eigener Strom
        // und gehoert auf das Musikgeraet, ein Lautsprecher nicht.
        if (const std::string* m = e.find("music")) {
            if (!m->empty()) {
                if (istMusik != nullptr) { *istMusik = true; }
                return *m;
            }
        }
        if (const std::string* n = e.find("noise")) {
            if (!n->empty() && *n != "*NOSOUND*") { return *n; }
        }
    }
    return {};
}

// --- Klaenge am Ort, wie die Engine sie hoert ------------------------------
//
// rc601 ton: Der Hoerer ist die Kamera, die die Ansicht ZEIGT.
//
// Gemeldet: "You can hear everything on the map". Bis rc600 hoerte die
// Vorschau von g_app->cam, der freien Editorkamera - auch waehrend des
// Abspielens, wo die Ansicht die Skriptkamera zeigt (drawMapView, useCam).
// Nach dem Laden einer Karte steht die freie Kamera am info_player_start;
// jede Szene klang also von dort aus. Die Engine hoert von cg.refdef.vieworg
// mit cg.refdef.viewaxis (cgi_S_Respatialize, cg_view.cpp:3307 MD, OpenJK :2243), im
// Kameramodus also von der Skriptkamera (cg_camera.cpp, CGCam_RenderScene),
// MIT Roll (AnglesToAxis) - vorher kam "rechts" nur aus der Gier.
//
// Dieselbe Wahl wie drawMapView: beim Abspielen mit Mitfahren die Bahn
// (camStateAt, mit FOLLOW und Wackeln), sonst "Through camera" (die gewaehlte
// Einstellung vor der Zeitmarke), sonst die freie Kamera.
void tonHoerer(float ort[3], float rechts[3]) {
    // camStateAt rechnet FOLLOW in 10-ms-Schritten nach - je Bild und
    // Zeitpunkt nur einmal, nicht fuer jeden Klang.
    struct Merk {
        int bild = -1;
        double ms = -1.0;
        bool spielt = false;
        bool durch = false;
        float ort[3]{};
        float rechts[3]{};
    };
    static Merk merk;
    const bool bahn = g_app->camTrackValid && !g_app->camTrack.segments.empty();
    const bool spielt = g_app->playing && g_app->followCam && bahn;
    const int bild = ImGui::GetFrameCount();
    if (merk.bild == bild && merk.ms == g_app->playMs && merk.spielt == spielt && merk.durch == g_app->throughCamera) {
        for (int k = 0; k < 3; ++k) {
            ort[k] = merk.ort[k];
            rechts[k] = merk.rechts[k];
        }
        return;
    }
    Camera c = g_app->cam;
    bool amZeiger = false;
    if (!spielt && g_app->throughCamera) {
        ShotView shot;
        if (shotAtSelection(shot)) {
            const bool vorschau = g_app->cameraSelected;
            for (int k = 0; k < 3; ++k) {
                c.pos[k] = shot.pos[k] + (vorschau ? g_app->gizmoOffset[k] : 0.0F);
                c.angles[k] = (shot.haveAng ? shot.ang[k] : 0.0F) + (vorschau ? g_app->gizmoAngles[k] : 0.0F);
            }
        } else {
            amZeiger = bahn;
        }
    }
    if (spielt || amZeiger) {
        const CameraState st = camStateAt(g_app->playMs);
        for (int k = 0; k < 3; ++k) {
            c.pos[k] = st.pos[k];
            c.angles[k] = st.angles[k];
        }
    }
    for (int k = 0; k < 3; ++k) {
        ort[k] = c.pos[k];
    }
    sound::rechtsAusWinkeln(c.angles, rechts);
    merk.bild = bild;
    merk.ms = g_app->playMs;
    merk.spielt = spielt;
    merk.durch = g_app->throughCamera;
    for (int k = 0; k < 3; ++k) {
        merk.ort[k] = ort[k];
        merk.rechts[k] = rechts[k];
    }
}

namespace {
std::uint64_t kanalSchluessel(const std::string& wer, sound::Kanal k) {
    if (k == sound::Kanal::Auto || k == sound::Kanal::LessAtten) {
        return 0;   // CHAN_AUTO sucht sich einen freien Kanal
    }
    // Die drei Stimmkanaele schneiden einander ab (S_CheckChannelStomp).
    const int gruppe = (k == sound::Kanal::Voice || k == sound::Kanal::VoiceAtten ||
                        k == sound::Kanal::VoiceGlobal)
                           ? 1
                           : 10 + static_cast<int>(k);
    std::uint64_t h = 1469598103934665603ULL;
    for (const char c : wer) {
        h = (h ^ static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(c)))) * 1099511628211ULL;
    }
    return (h ^ static_cast<std::uint64_t>(gruppe) * 0x9E3779B97F4A7C15ULL) | 1ULL;
}
}  // namespace

// rc601 saber review: derselbe Stomp-Schluessel fuer cgame-Klaenge auf einer
// Entity (cgi_S_StartSound( org, clientNum, CHAN_ITEM, ... ) des Wandtreffers).
std::uint64_t tonKanalSchluessel(const std::string& wer, sound::Kanal k) { return kanalSchluessel(wer, k); }

// Ein sound-Befehl aus dem Skript: auf der Entity `wer` (leer = der Traeger
// des Skripts) im Kanal `kanal`.
//
// rc601 ton: die Regeln von PlayIcarusSound stehen jetzt in tonSkriptKlang
// (app_ton.cpp) - Stimmen folgen der Figur ohne PVS, CHAN_ANNOUNCER und ein
// target_scriptrunner sind EV_GLOBAL_SOUND (am Hoerer, 0.75 * s_volume),
// alles andere ein G_Sound am Ort, nur im Snapshot. Vorher: der Spieler und
// jede Entity ohne bekannten Ort klangen voll und ohne Raum.
void spieleSkriptKlang(const std::string& werSpur, const std::string& datei, const std::string& kanalName,
                       double ms) {
    spieleSkriptKlang(werSpur, datei, kanalName, ms, 0.0);
}

void spieleSkriptKlang(const std::string& werSpur, const std::string& datei, const std::string& kanalName,
                       double ms, double abMs) {
    if (datei.empty()) {
        return;
    }
    const sound::Kanal k = sound::kanalAusName(kanalName);
    const std::string wer = werSpur.empty() ? g_app->ablauf.traeger : werSpur;
    tonSkriptKlang(wer, datei, k, kanalSchluessel(wer, k), ms, abMs);
}

std::string weltMusik() {
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname == "worldspawn") {
            const std::string* m = e.find("music");
            return (m == nullptr) ? std::string{} : *m;
        }
    }
    return {};
}

// Ein target_speaker, benutzt (Use_Target_Speaker, g_target.cpp:194 ff. MD).
//
// rc601 ton: das Abspielen nimmt die Einmalklaenge jetzt aus
// MoverSim::lautsprecher (tonEreignisse) - dort stimmen soundSet,
// spawnflags 3, wait, %d und der Ausloeser schon. Diese Funktion bleibt fuer
// andere Aufrufer, mit denselben Regeln und OHNE den alten Ersatz "voll und
// ohne Ort" (der traf jede Brush-Entity ohne origin, etwa ein func_breakable
// mit demselben Namen): nur target_speaker dieses Namens, keiner mit
// soundSet (SP_target_speaker kehrt dann zurueck), keine Schleife (die
// schaltet nur s.loopSound um), spawnflags 4 am Hoerer (EV_GLOBAL_SOUND),
// sonst am Lautsprecher, wenn er im Snapshot ist.
void spieleLautsprecher(const std::string& name, const std::string& datei) {
    const auto gleich = [](const std::string& a, const std::string& b) {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                   return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
               });
    };
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname != "target_speaker" || !gleich(e.targetname, name)) {
            continue;
        }
        if (const std::string* ss = e.find("soundSet"); ss != nullptr && !ss->empty()) {
            continue;
        }
        const std::string* sf = e.find("spawnflags");
        const int flags = (sf == nullptr) ? 0 : std::atoi(sf->c_str());
        if ((flags & 3) != 0) {
            continue;
        }
        KlangWunsch w;
        w.datei = datei;
        w.quelle = "target_speaker " + name;
        if (!e.origin.empty()) {
            std::istringstream is(e.origin);
            is >> w.ort[0] >> w.ort[1] >> w.ort[2];
        }
        const float punkt[3] = {0.0F, 0.0F, 0.0F};
        if ((flags & 4) != 0) {
            w.amHoerer = true;
        } else if (!tonImSnapshot(w.ort, punkt, punkt)) {
            continue;
        }
        (void)tonStarte(w);
    }
}

// Das Modell eines Schwertgriffs, gepuffert.
//
// Eigener Puffer neben modelCache: dort liegen Figuren, die immer unter
// models/players/<name>/model.glm stehen. Ein Griff wird in der .sab-Datei
// mit vollem Pfad genannt (models/weapons2/saber_plasma/saber_aayla.glm),
// und viele Figuren teilen sich denselben.
// Ein Waffenmodell gepuffert holen - wegen seines Tags "tag_flash".
//
// Gefragt: "wo bekommen wir das Projektil her?" Von dort: die Muendung ist
// ein Tag im .md3 der Waffe. An models/weapons2/blaster_r/blaster.md3
// nachgemessen: zwei Tags, "tag_flash" bei -1.3/-11.7/-3.9.
//
// Ueber die 74 .md3 unter models/weapons2 in assets1 gezaehlt: 54 haben
// einen tag_flash, 20 nicht - letztere sind Halterungen und Einzelteile.
// Die Effekte einer Waffe: Muendungsblitz und Geschoss.
//
// Der BLITZ steht in weapons.dat (`muzzleEffect`). Das GESCHOSS nicht - es
// steht in C++, je Waffe ein RegisterEffect (cg_weapons.cpp:401). Aus den
// Daten allein ist es also nicht zu erfahren.
//
// Was sich ableiten laesst, ist der VORSATZ: aus "blaster/muzzle_flash"
// wird "blaster". Nachgezaehlt ueber die 22 Waffen mit Muendungsblitz und
// die 376 .efx aus assets1: bei 16 gibt es unter demselben Vorsatz ein
// shot oder npcshot, bei 6 nicht (Disruptor, Repeater, DEMP2, Thermal,
// Stolperdraht, Sprengpaket - deren Geschosse werden anders gezeichnet
// oder sind gar keine).
//
// NPCs bekommen dabei ein ANDERES als der Spieler. FX_Blaster.cpp:71 ff.:
//
//     if ( cent->gent->owner->s.number > 0 )
//         theFxScheduler.PlayEffect( "blaster/NPCshot", ... );
//     else
//         theFxScheduler.PlayEffect( cgs.effects.blasterShotEffect, ... );
//
// Also erst npcshot, dann shot. Findet sich keines, wird nichts gezeichnet
// - und das Protokoll sagt es.
struct WeaponFx {
    const efx::Effect* muzzle = nullptr;
    const efx::Effect* shot = nullptr;
    const efx::Effect* impact = nullptr;
};

void ladeFolgeeffekte(std::vector<const efx::Effect*> offen, int* nachgeladenAus, int* ohneDateiAus);

const efx::Effect* effectByName(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    const std::string pfad = "effects/" + name + ".efx";
    const auto have = g_app->effects.find(pfad);
    if (have != g_app->effects.end()) {
        return have->second.primitives.empty() ? nullptr : &have->second;
    }
    // Ab hier wird MITTEN IM BILD geladen - siehe meldeNachladen.
    const auto ladeBeginn = std::chrono::steady_clock::now();
    std::string text;
    efx::Effect leer;
    if (readFromArchives(pfad, text)) {
        const efx::ReadResult r = efx::read(text);
        if (!r.hasErrors()) {
            leer = r.effect;
        } else {
            diag::detail("Effekt " + pfad + " nicht lesbar");
        }
    }
    // AUCH den Fehlschlag merken, sonst wird je Bild erneut gesucht.
    const auto put = g_app->effects.emplace(pfad, std::move(leer));
    if (!put.first->second.primitives.empty()) {
        ladeEffektShader(put.first->second);
        ladeFolgeeffekte({&put.first->second}, nullptr, nullptr);
    }
    meldeNachladen(
        "Effektdatei", pfad,
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - ladeBeginn).count());
    return put.first->second.primitives.empty() ? nullptr
                                                : &put.first->second;
}

// Alle Schuesse, die zu diesem Zeitpunkt zu sehen sind.
//
// Zwei Dinge je Schuss: der Muendungsblitz, der nur wenige Bilder lebt und
// an der Muendung bleibt, und das GESCHOSS, das fliegt.
//
// Ein EffectInstance hat einen festen Ort. Die Engine spielt den
// Schusseffekt aber in JEDEM Bild neu, an der aktuellen Stelle des
// Geschosses (FX_BlasterProjectileThink, FX_Blaster.cpp:37 ff.) - der
// "Strahl" ist gar kein bleibendes Ding, sondern ein Line-Primitiv, das
// mitwandert. Genau so wird es hier gemacht: die Stelle wird aus der
// Flugzeit gerechnet, und der Effekt beginnt JETZT.
//
// BLASTER_VELOCITY ist 2300 (weapons.h:180).
std::vector<EffectInstance> shotEffectsAt(double nowMs);

WeaponFx weaponFxFor(const std::string& typ) {
    WeaponFx out;
    const auto it = g_app->weaponMap.find(typ);
    if (it == g_app->weaponMap.end() || it->second.muzzleEffect.empty()) {
        return out;
    }
    out.muzzle = effectByName(it->second.muzzleEffect);
    const std::size_t slash = it->second.muzzleEffect.find('/');
    if (slash != std::string::npos) {
        const std::string vorsatz = it->second.muzzleEffect.substr(0, slash);
        out.shot = effectByName(vorsatz + "/npcshot");
        if (out.shot == nullptr) {
            out.shot = effectByName(vorsatz + "/shot");
        }
        // Der EINSCHLAG. In der Engine haengt er am Aufschlag des
        // Geschosses (FX_BlasterProjectileThink -> die Wand), und der
        // Effekt heisst durchweg "<vorsatz>/wall_impact".
        out.impact = effectByName(vorsatz + "/wall_impact");
    }
    return out;
}

std::vector<EffectInstance> shotEffectsAt(double nowMs) {
    std::vector<EffectInstance> out;
    if (!g_app->showEffects) {
        return out;
    }
    constexpr float kBlasterVelocity = 2300.0F;   // weapons.h:180
    // Wie lange ein Muendungsblitz zu sehen ist. Aus muzzle_flash.efx:
    // "life 30" - dreissig Millisekunden, also ein bis zwei Bilder.
    constexpr double kBlitzMs = 30.0;
    // Wie weit ein Geschoss verfolgt wird. Bei 2300 Einheiten je Sekunde
    // sind zwei Sekunden 4600 Einheiten - laenger als jede Sichtlinie in
    // diesen Karten, und eine Grenze muss es geben.
    constexpr double kFlugMs = 2000.0;

    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        const Actor& a = g_app->scene.actors[ai];
        const ActorState st = a.at(nowMs);
        if (!st.firing || st.removed) {
            continue;
        }
        // Die AKTUELLE Waffe (SET_WEAPON), nicht die aus der .npc - ohne
        // .npc bleibt es wie bisher bei keiner.
        const std::string& waffe = st.waffe;
        if (waffe.empty()) {
            continue;
        }
        const WeaponFx fx = weaponFxFor(waffe);
        if (fx.muzzle == nullptr && fx.shot == nullptr) {
            continue;
        }

        // Die Muendung und die Richtung.
        float mp[3] = {};
        muzzlePoint(st.pos, st.angles[1], waffe, mp);
        const float r = st.angles[1] * 3.14159265358979F / 180.0F;
        float vorn[3] = {std::cos(r), std::sin(r), 0.0F};
        // Zielt die Figur auf etwas, fliegt es dorthin. SET_WATCHTARGET
        // steht bei den Droiden dieser Mission auf "win2" - ohne das floege
        // jeder Schuss geradeaus, egal wohin die Figur schaut.
        if (st.hasWatchTarget) {
            const float dx = st.watchAt[0] - mp[0];
            const float dy = st.watchAt[1] - mp[1];
            const float dz = st.watchAt[2] + 30.0F - mp[2];
            const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (len > 1.0F) {
                vorn[0] = dx / len;
                vorn[1] = dy / len;
                vorn[2] = dz / len;
            }
        }

        // Welche Schuesse liegen zurueck? Der erste faellt auf den Beginn
        // des Feuerns, die weiteren im Abstand von shotSpacingMs.
        const double takt = std::max(static_cast<double>(st.shotSpacingMs),
                                     1.0);
        const double seit = nowMs - st.firingSinceMs;
        if (seit < 0.0) {
            continue;
        }
        // --- Wo trifft der Schuss auf? ------------------------------
        //
        // EINMAL je Figur, nicht je Schuss: alle Schuesse dieser Figur
        // fliegen dieselbe Bahn, solange sie steht. Je Schuss waere es bei
        // drei Droiden und drei Schuessen in der Luft neunmal derselbe
        // Strahl durch 980 Blaetter.
        //
        // Die Strecke ist so lang wie die Flugzeit reicht: bei 2300
        // Einheiten je Sekunde und zwei Sekunden sind das 4600.
        float treffer[3] = {mp[0] + vorn[0] * 4600.0F,
                            mp[1] + vorn[1] * 4600.0F,
                            mp[2] + vorn[2] * 4600.0F};
        float wegBisWand = 4600.0F;
        bool trifft = false;
        if (!g_app->geo.empty()) {
            const TraceTreffer t = traceRay(g_app->geo, mp, treffer);
            if (t.hit) {
                trifft = true;
                for (int k = 0; k < 3; ++k) {
                    // Ein Stueck VOR die Wand, sonst steckt der Effekt zur
                    // Haelfte darin und die halben Teilchen verschwinden.
                    treffer[k] = t.point[k] + t.normal[k] * 2.0F;
                }
                wegBisWand = 4600.0F * t.fraction;
            }
        }
        // Wie lange fliegt das Geschoss bis dahin?
        const double flugMs =
            static_cast<double>(wegBisWand) /
            static_cast<double>(kBlasterVelocity) * 1000.0;

        const auto letzter = static_cast<long long>(seit / takt);
        for (long long n = letzter; n >= 0; --n) {
            const double alter = seit - static_cast<double>(n) * takt;
            if (alter > kFlugMs) {
                break;
            }
            if (fx.muzzle != nullptr && alter <= kBlitzMs) {
                EffectInstance b;
                b.effect = fx.muzzle;
                for (int k = 0; k < 3; ++k) {
                    b.origin[k] = mp[k];
                }
                b.startMs = nowMs - alter;
                b.seed = static_cast<unsigned>(ai * 31 + n);
                out.push_back(b);
            }
            // --- Der EINSCHLAG ------------------------------------
            //
            // Sobald das Geschoss die Wand erreicht hat, hoert es auf zu
            // fliegen und wall_impact.efx spielt an der Trefferstelle.
            //
            // Das Fenster ist die Lebensdauer des Einschlageffekts, grob
            // gegriffen: laenger als ein paar hundert Millisekunden lebt
            // keiner von ihnen.
            if (trifft && alter >= flugMs && alter < flugMs + 400.0) {
                if (fx.impact != nullptr) {
                    EffectInstance ei;
                    ei.effect = fx.impact;
                    for (int k = 0; k < 3; ++k) {
                        ei.origin[k] = treffer[k];
                    }
                    ei.startMs = nowMs - (alter - flugMs);
                    ei.seed = static_cast<unsigned>(ai * 31 + n);
                    out.push_back(ei);
                }
                continue;   // das Geschoss selbst ist weg
            }
            if (trifft && alter >= flugMs) {
                continue;   // laengst eingeschlagen
            }
            if (fx.shot != nullptr) {
                EffectInstance g;
                g.effect = fx.shot;
                const auto weg = static_cast<float>(
                    alter / 1000.0 * static_cast<double>(kBlasterVelocity));
                for (int k = 0; k < 3; ++k) {
                    g.origin[k] = mp[k] + vorn[k] * weg;
                }
                // Der Effekt beginnt JETZT: er ist kein bleibendes Ding,
                // sondern wird je Bild neu an der Stelle des Geschosses
                // gespielt (FX_Blaster.cpp:37 ff.).
                g.startMs = nowMs;
                g.seed = static_cast<unsigned>(ai * 31 + n);
                out.push_back(g);
            }
        }
    }
    return out;
}

const Md3Model* weaponModelFor(const std::string& pfad) {
    if (pfad.empty()) {
        return nullptr;
    }
    const auto have = g_app->weaponModels.find(pfad);
    if (have != g_app->weaponModels.end()) {
        return have->second.empty() && have->second.numTags == 0
                   ? nullptr
                   : &have->second;
    }
    Md3Model m;
    std::string data;
    if (readFromArchives(pfad, data)) {
        std::string err;
        if (!readMd3(data, m, &err)) {
            diag::detail("Waffenmodell " + pfad + " nicht lesbar: " + err);
            m = Md3Model{};
        }
    } else {
        diag::detail("Waffenmodell " + pfad + " nicht gefunden");
    }
    // AUCH den Fehlschlag merken, sonst wird je Bild erneut gesucht.
    const auto put = g_app->weaponModels.emplace(pfad, std::move(m));
    return put.first->second.empty() && put.first->second.numTags == 0
               ? nullptr
               : &put.first->second;
}

// --- Wenn mitten im Bild geladen wird, soll es dastehen ------------------
//
// Aus deinem Protokoll:
//
//     Zeit je Bild 51.6 ms: ... Figuren 23.1, Szene 13.3 ... spielt
//     Zeit je Bild 35.4 ms: ... Figuren  9.2, Szene 13.9 ... spielt
//
// Zweimal waehrend des Abspielens kostet der Szeneabschnitt dreizehn
// Millisekunden statt einem Zehntel. Dort stehen die verzoegerten
// Ladevorgaenge: Schwertgriffe, Klingentexturen, Effektdateien. Sie werden
// beim ERSTEN Gebrauch geholt - und der faellt mitten in ein Bild.
//
// Das ist kein Fehler, sondern eine Entscheidung: alles vorab zu laden
// kostet beim Missionsstart Zeit fuer Dinge, die vielleicht nie
// vorkommen. Aber es soll im Protokoll STEHEN, sonst sucht man den
// Ausreisser bei der Bildrate statt beim Laden.
void meldeNachladen(const char* was, const std::string& name, double ms) {
    if (ms < 1.0) {
        return;   // unter einer Millisekunde ist es kein Ausreisser
    }
    char z[220];
    std::snprintf(z, sizeof(z),
                  "Nachgeladen mitten im Bild: %s \"%s\" - %.1f ms%s",
                  was, name.c_str(), ms,
                  g_app->playing ? " (waehrend des Abspielens)" : "");
    diag::detail(z);
}

const App::ActorAssets* hiltFor(const std::string& pfad) {
    if (pfad.empty()) {
        return nullptr;
    }
    const auto have = g_app->hiltCache.find(pfad);
    if (have != g_app->hiltCache.end()) {
        return have->second.model.empty() ? nullptr : &have->second;
    }
    // Ab hier wird MITTEN IM BILD geladen - siehe meldeNachladen.
    const auto ladeBeginn = std::chrono::steady_clock::now();
    App::ActorAssets a;
    std::string data;
    if (readFromArchives(pfad, data)) {
        std::string err;
        if (!readGlm(data, a.model, &err)) {
            diag::detail("Griff " + pfad + " nicht lesbar: " + err);
            a.model = GlmModel{};
        } else {
            // Die Haut liegt neben dem Modell, nicht darin.
            const std::size_t slash = pfad.find_last_of('/');
            if (slash != std::string::npos) {
                const std::string dir = pfad.substr(0, slash);
                std::string skin;
                if (readFromArchives(dir + "/model_default.skin", skin)) {
                    applySkin(skin, a.model);
                }
            }
            a.textures = texturesFor(a.model);
        }
    } else {
        diag::detail("Griff " + pfad + " nicht gefunden");
    }
    // AUCH den Fehlschlag merken, sonst wird je Bild erneut gesucht.
    const auto put = g_app->hiltCache.emplace(pfad, std::move(a));
    meldeNachladen(
        "Schwertgriff", pfad,
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - ladeBeginn).count());
    return put.first->second.model.empty() ? nullptr : &put.first->second;
}

// Was die Figur in den Haenden haelt, fuer dieses Bild.
//
// Ohne bekannte Waffe (keine .npc zur Figur) bleibt die Hand leer - so war
// es vorher auch: ohne .npc gab es keinen Griff.
// --- rc601 saber: die Bilder einer Figur zu einem Zeitpunkt ---------------
//
// Aus der Figurenschleife von drawMapView herausgezogen, unveraendert bis auf
// den Zeitpunkt `ms` statt g_app->playMs: die Schwertproben (app_saber.cpp)
// brauchen die Lage der Haende zu beliebigen Takten - mit derselben Rechnung,
// sonst laege die geprobte Klinge woanders als die gezeichnete.
void figurBilderSetzen(const Actor& a, const App::ActorAssets& assets, const ActorState& st, double ms, ActorDraw& d) {
    // Welche NPC-Typen schon gemeldet wurden, dass ihnen die
    // Schussanimation fehlt. Ohne das kaeme die Zeile in jedem Bild.
    static std::set<std::string> schussFehlt;
    if (assets.skeleton != nullptr) {
        d.anim = &assets.skeleton->anim;
        // --- Kennt dieses Skelett den Namen? ----------------
        //
        // Kennt es ihn nicht, laesst die Engine die Figur in
        // der Animation, die sie schon hatte (siehe den Kopf
        // von frameForAnimationIn). Genau das tun wir: erst
        // die gewuenschte, dann die vorige, zuletzt der
        // Grundzustand.
        //
        // Bleibt auch der unbekannt, ist das Skelett zu
        // fremd - dann Bild 0, und die Meldung sagt warum.
        // st.animSpeed: die Animation laeuft MIT dem Gehtempo,
        // sonst rutschen die Fuesse - siehe kAnimSpeedWalk in
        // scene.h.
        d.frame = frameForAnimationIn(
            assets.skeleton->sections, st.animation,
            ms - st.animStartMs, st.holdAnim,
            st.animSpeed, &d.frameFraction, &d.frameNext);
        if (d.frame < 0) {
            // EINMAL je Skelett und Name melden, nicht je Bild.
            // Sonst stehen sechzig Zeilen je Sekunde im
            // Protokoll und die Datei laeuft voll.
            const std::string schluessel =
                assets.model.animFile + "|" + st.animation;
            if (g_app->unknownAnims.insert(schluessel).second) {
                diag::detail(
                    "Figur " + a.name + ": Animation \"" +
                    st.animation + "\" steht nicht in der .cfg "
                    "von \"" + assets.model.animFile +
                    "\" - die Engine laesst die Figur in diesem "
                    "Fall stehen, wo sie war.");
            }
            d.frame = frameForAnimationIn(
                assets.skeleton->sections, st.prevAnimation,
                st.prevAnimSinceMs, st.prevHoldAnim);
        }
        if (d.frame < 0) {
            d.frame = frameForAnimationIn(
                assets.skeleton->sections, "BOTH_STAND1", 0.0,
                false);
        }
        if (d.frame < 0) {
            d.frame = 0;
        }
        // --- Die Schussanimation --------------------------
        //
        // Sie liegt auf dem OBERKOERPER (SETANIM_TORSO) und
        // faengt bei JEDEM Schuss neu an
        // (SETANIM_FLAG_RESTART, bg_pmove.cpp:13810). Der
        // Abstand kommt aus SET_SHOT_SPACING.
        //
        // Sie geht VOR einer Oberkoerperanimation aus dem
        // Skript: die Engine setzt sie im Bewegungsschritt,
        // also nach allem, was ICARUS gesetzt hat.
        if (st.firing && assets.skeleton != nullptr) {
            // Seit dem FEUERBEGINN, nicht seit dem
            // Animationsbeginn - siehe firingSinceMs.
            const double seit =
                ms - st.firingSinceMs;
            const double takt =
                std::fmod(std::max(seit, 0.0),
                          static_cast<double>(
                              std::max(st.shotSpacingMs, 1.0F)));
            const int bild = frameForAnimationIn(
                assets.skeleton->sections, "BOTH_ATTACK3", takt,
                false);
            if (bild >= 0) {
                d.torsoFrame = bild;
            } else if (!schussFehlt.count(a.npcType)) {
                // EINMAL je NPC-Typ melden, nicht je Bild.
                schussFehlt.insert(a.npcType);
                diag::detail(
                    "Figur " + a.name + " schiesst, aber \"" +
                    "BOTH_ATTACK3\" steht nicht in der .cfg " +
                    "seines Skeletts (NPC-Typ " + a.npcType + ")");
            }
        }

        // Der MUND, wenn die Figur gerade spricht.
        const std::string mund =
            faceAnimFor(a.name, ms);
        if (!mund.empty()) {
            d.faceFrame = frameForAnimationIn(
                assets.skeleton->sections, mund, 0.0, false);
        }
        // Der OBERKOERPER, wenn er eine eigene Animation hat.
        //
        // Kennt das Skelett den Namen nicht, bleibt torsoFrame
        // bei -1 und der Oberkoerper folgt dem Koerper - genau
        // wie beim Koerper selbst: die Engine tut in dem Fall
        // gar nichts.
        //
        // Schiesst die Figur, gewinnt der Schuss: die Engine
        // setzt BOTH_ATTACK3 mit SETANIM_FLAG_OVERRIDE
        // (bg_pmove.cpp:13810). Vorher ueberschrieb die
        // Skriptanimation das Schussbild wieder.
        if (!st.torsoAnimation.empty() && d.torsoFrame < 0) {
            d.torsoFrame = frameForAnimationIn(
                assets.skeleton->sections, st.torsoAnimation,
                ms - st.torsoStartMs, st.torsoHold, 1.0F,
                &d.torsoFraction, &d.torsoNext);
            if (d.torsoFrame < 0) {
                const std::string schluessel =
                    assets.model.animFile + "|" + st.torsoAnimation;
                if (g_app->unknownAnims.insert(schluessel).second) {
                    diag::detail(
                        "Figur " + a.name + ": Oberkoerperanimation \"" +
                        st.torsoAnimation +
                        "\" steht nicht in der .cfg von \"" +
                        assets.model.animFile + "\".");
                }
            }
        }
        // Laeuft gerade ein Uebergang? Dann die EINGEFRORENE
        // Frame der vorigen Animation dazu.
        //
        // prevAnimSinceMs steht fest - es ist der Stand vom
        // Moment des Umschaltens, nicht "jetzt minus damals".
        // Genau daran erkennt man, ob es richtig gebaut ist:
        // die alte Seite darf sich waehrend des Uebergangs
        // nicht bewegen.
        // Der eigene Uebergang des Oberkoerpers.
        const float lt = torsoBlendFraction(st, ms);
        if (lt < 1.0F) {
            d.torsoPrevFrame = frameForAnimationIn(
                assets.skeleton->sections, st.torsoPrevAnimation,
                st.torsoPrevSinceMs, false);
            d.torsoBlendLerp = (d.torsoPrevFrame < 0) ? 1.0F : lt;
        }
        const float l = blendFraction(st, ms);
        if (l < 1.0F) {
            d.prevFrame =
                frameForAnimationIn(assets.skeleton->sections,
                                    st.prevAnimation,
                                    st.prevAnimSinceMs,
                                    st.prevHoldAnim);
            // Kennt das Skelett die vorige nicht, gibt es
            // nichts zu ueberblenden - dann lieber hart
            // umschalten als gegen ein falsches Bild mischen.
            d.blendLerp = (d.prevFrame < 0) ? 1.0F : l;
        }
    }
}

// rc601 saber: die Weltmatrix einer Figur samt der Groesse aus der .npc -
// aus der Figurenschleife herausgezogen (dieselbe Rechnung fuer Zeichnung
// und Schwertproben).
void figurWeltMatrix(const ActorDraw& a, float fw[16]) {
    gpu::baueFigurWelt(a.yaw + a.yawOffset, a.pos, fw);
    if (a.skala[0] != 1.0F || a.skala[1] != 1.0F || a.skala[2] != 1.0F) {
        const float spalte[3] = {a.skala[1], a.skala[0], a.skala[2]};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) { fw[r * 4 + c] *= spalte[c]; }
        }
        if (a.skala[2] != 0.0F) {
            fw[11] += 24.0F * (a.skala[2] - 1.0F);
        }
    }
}

// rc601 saber: der Kopfausschlag zum Blickziel - aus der Figurenschleife
// herausgezogen (er dreht drei Knochen und damit auch die Arme mit).
void figurKopfGier(const ActorState& st, ActorDraw& d) {
    if (st.hasLookTarget && g_app->lookRotation) {
        const float dx = st.lookAt[0] - st.pos[0];
        const float dy = st.lookAt[1] - st.pos[1];
        if (dx * dx + dy * dy > 1.0F) {
            const float ziel =
                std::atan2(dy, dx) * 180.0F / 3.14159265F;
            // Der Ghoul2-Weg der Engine dreht den Oberkoerper
            // NICHT dem Blick nach - siehe kHeadYawClampMin in
            // scene.h. Es gibt nur den Kopfausschlag, und der
            // verteilt sich im Zeichner auf drei Knochen.
            d.headYaw = std::clamp(
                shortestAngleDelta(st.angles[1], ziel),
                kHeadYawClampMin, kHeadYawClampMax);
        }
    }
}

void anbautenFuer(const Actor& a, const ActorState& st, double ms, ActorDraw& d) {
    static_assert(kAnbauKlingen == kMaxKlingen,
                  "mapview.h und scene.h muessen dieselbe Klingenzahl haben");
    static_assert(kAnbauMax >= 2 + kMaxKinoModelle,
                  "zwei Hand-Faecher und alle Kinomodelle muessen passen");
    d.anbauten = 0;
    const auto pfad = [&a](int m) -> const std::string* {
        if (m < 0 || static_cast<std::size_t>(m) >= a.modelle.size()) {
            return nullptr;
        }
        return &a.modelle[static_cast<std::size_t>(m)];
    };
    const auto neu = [&d](const App::ActorAssets* hg, const char* bolzen) -> AnbauDraw* {
        if (hg == nullptr || d.anbauten >= kAnbauMax) {
            return nullptr;
        }
        AnbauDraw& an = d.anbau[static_cast<std::size_t>(d.anbauten)];
        ++d.anbauten;
        an = AnbauDraw{};
        an.model = &hg->model;
        an.textures = &hg->textures;
        an.bolzen = bolzen;
        return &an;
    };
    if (!st.waffe.empty()) {
        for (std::size_t n = 0; n < 2; ++n) {
            const HandBelegung& h = st.hand[n];
            const std::string* p = pfad(h.modell);
            if (p == nullptr) {
                continue;
            }
            const SaberZustand& s = st.saber[n];
            const SaberArt* art =
                (h.istSaber && s.art >= 0 &&
                 static_cast<std::size_t>(s.art) < a.saberArten.size())
                    ? &a.saberArten[static_cast<std::size_t>(s.art)]
                    : nullptr;
            // boltToWrist haengt den Griff ans Handgelenk
            // (wp_saber.cpp:858 ff.).
            const bool gelenk = art != nullptr && art->handgelenk;
            const char* bolzen = (n == 0) ? (gelenk ? "*r_hand_cap_r_arm" : "*r_hand")
                                          : (gelenk ? "*l_hand_cap_l_arm" : "*l_hand");
            AnbauDraw* an = neu(hiltFor(*p), bolzen);
            if (an == nullptr || !h.istSaber) {
                continue;
            }
            // Klingen nur mit dem Schwert in der Hand, saber[1] nur mit
            // dualSabers (cg_players.cpp:16200 ff.), und keine beim
            // Sith-Schwert (ebenda:14556).
            if (!haeltSaber(st) || (n == 1 && !st.dualSabers)) {
                continue;
            }
            if (art != nullptr && art->typ == "SABER_SITH_SWORD") {
                continue;
            }
            an->lage = klingenLageFuer(art != nullptr ? art->typ : std::string());
            an->saberFach = static_cast<int>(n);   // rc601 saber
            for (int b = 0; b < s.numBlades && b < kMaxKlingen; ++b) {
                // noBlade / noBlade2, je nach bladeStyle2Start
                // (ebenda:14562 ff.) - der Griff eines Elektrostabs hat
                // keine Klinge, nur das Licht.
                if (art != nullptr) {
                    const bool zweiter =
                        art->bladeStyle2Start > 0 && b >= art->bladeStyle2Start;
                    if (zweiter ? art->ohneKlinge2 : art->ohneKlinge) {
                        continue;
                    }
                }
                // rc601 saber review: die Laenge, mit der CG_DoSaber zeichnet
                // (L+8 beim Wachsen, Zuenden nach Kampfstil, an Waenden
                // gekuerzt) - vorher galt hier die Szenenlaenge, und eine Klinge
                // mit L < 0,5 fehlte, obwohl das Spiel L+8 zeichnet.
                const float laenge =
                    saberZeichenLaengeBei(d.figur, static_cast<int>(n), b, ms, klingenLaenge(st, static_cast<int>(n), b, ms));
                // Unter MIN_SABERBLADE_DRAW_LENGTH (0.5, wp_saber.h:122)
                // zeichnet die Engine gar nichts.
                if (laenge < 0.5F) {
                    continue;
                }
                const KlingenZustand& kz = s.klinge[static_cast<std::size_t>(b)];
                KlingeDraw& kd = an->klinge[static_cast<std::size_t>(an->klingen)];
                ++an->klingen;
                kd.nummer = b;
                kd.laenge = laenge;
                kd.radius = (kz.radius > 0.0F) ? kz.radius : 3.0F;
                kd.laengeMax = kz.max;
                bladeColorFor(kz.farbe, kd.farbe);
                saberLichtFarbe(kz.farbe, kd.licht);
                kd.glowTex = bladeTexture(kz.farbe, true);
                kd.coreTex = bladeTexture(kz.farbe, false);
            }
        }
    }
    // Kinomodelle haengen unabhaengig von der Waffe
    // (Q3_AddRHandModel & Co., Q3_Interface.cpp:6336 ff.).
    for (const KinoModell& km : st.kino) {
        const std::string* p = pfad(km.modell);
        if (p != nullptr) {
            (void)neu(hiltFor(*p), km.links ? "*l_hand" : "*r_hand");
        }
    }
}

const App::Skeleton* skeletonFor(const GlmModel& model) {
    // Kein Pfad im Kopf? Die Engine raet dann _humanoid:
    // "take a guess, maybe it's right?" (NPC_stats.cpp). Wir tun dasselbe.
    std::string animFile = model.animFile;
    if (animFile.empty()) {
        animFile = "models/players/_humanoid/_humanoid";
    }
    const auto have = g_app->skeletons.find(animFile);
    if (have != g_app->skeletons.end()) {
        return have->second.anim.empty() ? nullptr : &have->second;
    }
    App::Skeleton sk;
    std::string data;
    // Das Archiv MIT ins Protokoll.
    //
    // Movie Duels ersetzt models/players/_humanoid/_humanoid.gla durch eine
    // Fassung mit 30384 statt 21376 Bildern - gleicher Name, gleicher
    // Ordner. Ohne diese Angabe steht im Protokoll nur die Bilderzahl, und
    // man muss sie auswendig kennen, um zu merken, dass die falsche Datei
    // geladen wurde. Mit ihr sieht man es auf einen Blick.
    std::string ausArchiv;
    if (readFromArchives(animFile + ".gla", data, &ausArchiv)) {
        std::string err;
        if (readGla(data, sk.anim, &err)) {
            sk.sections = animSectionsFor(animFile);
            diag::info("Skelett " + animFile + ": " +
                       std::to_string(sk.anim.bones.size()) + " Knochen, " +
                       std::to_string(sk.anim.numFrames) + " Bilder, " +
                       std::to_string(sk.sections.size()) + " Abschnitte, aus " +
                       fileName(ausArchiv));
        } else {
            diag::info("Skelett nicht lesbar: " + animFile + ".gla - " + err);
        }
    } else {
        diag::info("keine .gla gefunden: " + animFile + ".gla");
    }
    const auto put = g_app->skeletons.emplace(animFile, std::move(sk));
    return put.first->second.anim.empty() ? nullptr : &put.first->second;
}

// Die fx_runner der Karte einlesen.
//
// 374 Stueck ueber die 23 Karten von MD_Maps_Ep1/2/4/6 - nach waypoint und
// info_null die haeufigste Entity. Rauch, Funken, Dampf; ohne sie sieht ein
// Hangar aus wie ein leerer Raum.
//
// Der Pfad wird so gebildet, wie es die Engine tut
// (FxScheduler.cpp, CFxScheduler::RegisterEffect):
//
//     COM_StripExtension( path, filenameNoExt, ... );
//     Com_sprintf( buf, ..., "%s/%s.efx", FX_FILE_PATH, filenameNoExt );
//
// Also Endung abschneiden, dann "effects/" davor und ".efx" dahinter. Der
// Schluessel "fxfile" steht in der Karte oft schon mit Endung, oft ohne -
// beides muss zum selben Ergebnis fuehren.
// rc604: G_SpawnGEntityFromSpawnVars entfernt jede Entity mit "notsingle"
// oder dem Bit der Schwierigkeit (spawnflags & (1 << (8 + g_spskill)),
// code/game/g_spawn.cpp:1064-1075, 1096-1101 OpenJK, :1260-1265 MD) - vor
// ihrer Spawnfunktion. Die Vorschau zeigt sie dann nicht.
bool gespawntImSpiel(const MapEntity& e);

// Stufe 6 (OpenJK#1358): CG_DrawMiscEnts (OpenJK code/cgame/cg_main.cpp, MD
// cg_main.cpp:3224-3227) zeichnet ein misc_model_static nur bei gi.inPVS(vieworg,
// origin + (0 0 zoffset+1)) - zoffset aus SP_misc_model_static (g_misc.cpp:417).
// Liegt der Punkt im Festen (Blatt mit Cluster -1, Area -1), sagt SV_inPVS ueber
// CM_AreasConnected (cm_test.cpp:607) immer nein: das Spiel zeigt es nie.
bool miscModelStaticImFesten(const MapEntity& e) {
    if (e.classname != "misc_model_static" || g_app->geo.empty()) { return false; }
    float pruef[3] = {0.0F, 0.0F, 0.0F};
    (void)std::sscanf(e.origin.c_str(), "%f %f %f", &pruef[0], &pruef[1], &pruef[2]);
    const std::string* zo = e.find("zoffset");
    pruef[2] += (zo != nullptr ? static_cast<float>(std::atof(zo->c_str())) : 0.0F) + 1.0F;
    const int blatt = g_app->geo.leafAt(pruef);
    return blatt >= 0 && static_cast<std::size_t>(blatt) < g_app->geo.leafs.size() && g_app->geo.leafs[static_cast<std::size_t>(blatt)].cluster == -1;
}

bool gespawntImSpiel(const MapEntity& e) {
    return e.classname == "worldspawn" || spawntBeiSchwierigkeit(e, regelwerkSpielstufe());
}
void loadMapEffects();
void fallenEffekteSetzen();   // rc604: misc_trip_mine (laserfalleLauf)
void loadMapModels();

// rc604 fix604: der Block eines Kartenmodells in textures.byShader - beim
// ersten Mal legt `anlegen` die Bilder an (hinten, zusammenhaengend), danach
// kommt derselbe Block zurueck (TextureSet::modellBloecke). Der Schluessel
// nennt alles, was die Bilder bestimmt (Datei, Haut, Sonderfall); dazu der
// Stand von texCache (rescanGamePaths liest die Bilder neu).
// Nur fuer Vorher-Proben (Selbsttest "Bugs erst nachstellen"):
// BHED_MODELLE_JE_AUFRUF=1 legt die Bilder wie bis rc603 bei jedem Aufruf neu
// an (Modus tuerlauf: "ein zweites prepareScene legt keine Bilder an").
template <class F>
int modellBlock(const std::string& schluessel, F&& anlegen) {
    static const bool jeAufruf = std::getenv("BHED_MODELLE_JE_AUFRUF") != nullptr;
    TextureSet& ts = g_app->textures;
    const std::string k = std::to_string(g_app->texCacheStand) + "|" + schluessel;
    const auto it = ts.modellBloecke.find(k);
    if (it != ts.modellBloecke.end() && !jeAufruf) { return it->second; }
    const int basis = static_cast<int>(ts.byShader.size());
    anlegen();
    ts.modellBloecke.emplace(k, basis);
    return basis;
}

// --- Fuer den ICARUS-Nachbau ------------------------------------------------

// Wie heisst das offene Skript fuer ICARUS? "md_ga/intro_jedi" - aus dem
// Archivnamen, sonst aus dem Pfad auf der Platte (alles hinter "scripts/").
std::string aktiverSkriptPfad() {
    if (!g_app->skriptPfad.empty()) {
        return g_app->skriptPfad;
    }
    std::string p = g_app->path;
    for (char& c : p) {
        if (c == '\\') { c = '/'; }
    }
    std::string k = p;
    for (char& c : k) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const auto s = k.rfind("/scripts/");
    std::string r = (s == std::string::npos) ? p.substr(p.find_last_of('/') + 1) : p.substr(s + 9);
    const auto punkt = r.find_last_of('.');
    const auto strich = r.find_last_of('/');
    if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) {
        r.resize(punkt);
    }
    return r;
}

// Ein Skript fuer run / use / spawnscript. Zuerst ein offener Reiter (dann
// wirken ungespeicherte Aenderungen sofort), sonst aus den Archiven.
const Script* skriptFuerAblauf(const std::string& pfad) {
    const auto klein = [](std::string x) {
        for (char& c : x) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (c == '\\') { c = '/'; }
        }
        return x;
    };
    const std::string k = klein(pfad);
    for (const App::Parked& t : g_app->tabs) {
        if (!t.skriptPfad.empty() && klein(t.skriptPfad) == k) {
            return &t.doc.script();
        }
        std::string tp = klein(t.path);
        const auto s = tp.rfind("/scripts/");
        if (s != std::string::npos) {
            tp = tp.substr(s + 9);
            const auto punkt = tp.find_last_of('.');
            if (punkt != std::string::npos) { tp.resize(punkt); }
            if (tp == k) {
                return &t.doc.script();
            }
        }
    }
    const auto it = g_app->ablaufSkripte.find(k);
    if (it != g_app->ablaufSkripte.end()) {
        return it->second.get();
    }
    std::unique_ptr<Script> s;
    std::string data;
    for (const char* ext : {".txt", ".icarus", ".ibi", ".IBI"}) {
        if (!readFromArchives("scripts/" + pfad + ext, data)) {
            continue;
        }
        auto sc = std::make_unique<Script>();
        std::vector<Diag> d;
        if (data.size() > 4 && data.compare(0, 3, "IBI") == 0) {
            std::vector<IbiBlock> bl;
            if (readIbi(data, bl, d)) {
                (void)decompile(bl, g_app->db, *sc, d);
            }
        } else {
            (void)readScript(data, *sc, d);
        }
        s = std::move(sc);
        break;
    }
    const Script* r = s.get();
    g_app->ablaufSkripte[k] = std::move(s);   // auch "nicht gefunden" merken
    return r;
}

// Eine ROFF-Bahn aus den Archiven, gemerkt (bhed/roff.h). Der Name wie im
// Skript: play ( "PLAY_ROFF", "kor2/roffs/boulder" ) sucht
// scripts/kor2/roffs/boulder.rof, genau wie G_LoadRoff (g_roff.cpp:437).
std::shared_ptr<const Roff> roffFuer(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    // Eine eigene Bahn aus einem offenen Skript geht vor (rc599) - sie ist
    // neuer als jede Datei.
    if (std::shared_ptr<const Roff> eigen = roffAusMarken(name)) {
        return eigen;
    }
    const std::string pfad = roffPfad(name);
    std::string k = pfad;
    for (char& c : k) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const auto it = g_app->roffs.find(k);
    if (it != g_app->roffs.end()) {
        return it->second;
    }
    std::shared_ptr<const Roff> r;
    std::string data;
    // Archive, dann lose Dateien (rc599: so liegen die gebauten neben den
    // Skripten). Wie in der Engine je Spielordner erst die Archive, dann
    // der Ordner selbst - und der spaetere Spielordner gewinnt.
    bool gefunden = false;
    for (auto gp = g_app->gamePaths.rbegin(); gp != g_app->gamePaths.rend() && !gefunden; ++gp) {
        for (auto a = gp->archives.rbegin(); a != gp->archives.rend() && !gefunden; ++a) {
            if (const Pk3Entry* e = a->find(pfad)) {
                gefunden = readPk3File(*a, *e, data);
            }
        }
        if (!gefunden && !gp->directory.empty()) {
            std::error_code ec;
            const std::filesystem::path lose = ::bhed::utf8Pfad(gp->directory + "/" + pfad);
            if (std::filesystem::is_regular_file(lose, ec)) {
                data = slurp(gp->directory + "/" + pfad);
                gefunden = !data.empty();
            }
        }
    }
    if (gefunden) {
        auto neu = std::make_shared<Roff>();
        std::string grund;
        if (leseRoff(data, *neu, &grund)) {
            diag::detail("ROFF " + pfad + ": Fassung " + std::to_string(neu->fassung) + ", " +
                         std::to_string(neu->bilder.size()) + " Bilder zu " + std::to_string(neu->bildMs) +
                         " ms, " + std::to_string(neu->notizen.size()) + " Notizen");
            for (const std::string& h : neu->hinweise) {
                diag::detail("ROFF " + pfad + ": " + h);
            }
            r = std::move(neu);
        } else {
            // G_ValidRoff: "Invalid roff format" - im Spiel bewegt sich dann
            // nichts, und die Aufgabe bleibt offen.
            diag::warn("ROFF " + pfad + " ungueltig: " + grund);
        }
    } else if (!g_app->gamePaths.empty()) {
        diag::warn("ROFF " + pfad + " nicht in den Archiven");
    }
    g_app->roffs[k] = r;   // auch "nicht gefunden" merken
    return r;
}

// rc605 (Ende): trigger_hurt toetet Figuren - aus der fertigen Szene (Wege,
// Schwerkraft), im Takt der Spiellogik (50 ms). Nachgebildet:
//  - SP_trigger_hurt (g_trigger.cpp:1422-1475 MD; code/game/g_trigger.cpp
//    OpenJK, codeJK2/game/g_trigger.cpp gleich): dmg 0 -> 5; Movie Duels dmg < 0
//    -> 9999999 mit spawnflags 32|16; delay und wait * 1000; START_OFF (1)
//    nicht gelinkt; InitTrigger: spawnflags 128 -> SVF_INACTIVE.
//  - hurt_use (ebenda:1215-1228 MD, nur mit targetname): jeder use schaltet
//    gelinkt/ungelinkt um; target_activate / target_deactivate setzen
//    SVF_INACTIVE (wie Nachbau::benutze).
//  - G_TouchTriggersLerped (g_active.cpp:1689-1810 MD, nach Pmove, nicht im
//    noclip, nicht tot): vom letzten zum jetzigen Ort in Schritten von
//    maxs[1] / 2 (ueber 1024 nur der jetzige Ort), je Ort der Kasten der Figur
//    gegen das Brush-Modell (gi.EntityContact = CM_TransformedBoxTrace,
//    startsolid), je Bewegung ein Treffer je Feld.
//  - hurt_touch (g_trigger.cpp:1243-1345 MD): SVF_INACTIVE; wait gegen
//    painDebounceTime (MULTIPLE 2048); PLAYERONLY 2; NPC_targetname; NO_PROTECTION
//    8; delay: dmg * attackDebounceTime / delay, attackDebounceTime += FRAMETIME
//    (100), zurueck auf 0 zwei FRAMETIME nach der letzten Beruehrung
//    (trigger_hurt_reset); FALLING 32: OpenJK CLASS_BOBAFETT und
//    CLASS_ROCKETTROOPER fliegen los statt Schaden (JET_FlyStart), Movie Duels
//    vergleicht NPC_class mit dem ODER der fuenf Klassen (59|66|67|81|60 = 127,
//    keine Klasse - also nimmt jeder Schaden), Jedi Outcast hat keine Ausnahme.
//  - G_Damage, soweit es hier zaehlt (g_combat.cpp:8203-8210, 8636-8650,
//    9276-9292 MD; code/game/g_combat.cpp:5661, 6065, 6507-6523 OpenJK;
//    codeJK2/game/g_combat.cpp:4889, 5002, 5143-5157): ein NPC mit FL_GODMODE
//    oder FL_UNDYING verliert NO_PROTECTION; FL_GODMODE nimmt keinen Schaden;
//    health -= take, nicht unter 0; FL_UNDYING unter 1: BSET_DEATH, health 1;
//    health <= 0: die.
// Nicht nachgebildet (vermerkt): der Spieler (dessen Tod beendet die Mission),
// Ruestung (CheckArmor, nur nach SET_ARMOR auf einem NPC), PM_InForceFall
// (Movie Duels), die Reihenfolge der Entities innerhalb eines Bilds (die
// Figuren in der Reihenfolge der Szene, das Feld denkt am Anfang des Bilds) und
// ein deathscript, das FL_UNDYING bei jeder weiteren Beruehrung neu startet
// (hier einmal).
namespace {
void fallenWeltBauen();
bool fallenKannTreffenBei(double ms, const float von[3], const float mitte[3]);
}  // namespace

std::vector<AblaufWelt::HurtTod> hurtTodeBerechnen() {
    std::vector<AblaufWelt::HurtTod> aus;
    const MapData& karte = g_app->map;
    if (karte.empty() || g_app->scene.actors.empty()) { return aus; }
    struct Feld {
        int entity = -1;
        int modell = -1;
        int sf = 0;
        int dmg = 0;
        double delay = 0.0;
        double wait = 0.0;
        bool gelinkt = true;
        bool inaktiv = false;
        std::string name;
        std::string npcName;
        float ursprung[3]{};
        float lo[3]{};
        float hi[3]{};
        double pain = 0.0;      // painDebounceTime
        double angriff = 0.0;   // attackDebounceTime
        double zurueck = -1.0;  // nextthink trigger_hurt_reset
    };
    const bool md = regelwerkMovieDuels();
    const bool jk2 = regelwerkJediOutcast();
    const int stufe = regelwerkSpielstufe();
    std::vector<Feld> felder;
    std::shared_ptr<const KollisionsKarte> k;
    for (std::size_t i = 0; i < karte.entities.size(); ++i) {
        const MapEntity& e = karte.entities[i];
        if (e.classname != "trigger_hurt") { continue; }
        if (stufe >= 0 && !spawntBeiSchwierigkeit(e, stufe)) { continue; }
        const std::string* mdl = e.find("model");
        if (mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*') { continue; }
        if (!k) {
            k = geladeneKollision(karte.path);
            if (!k || k->leer()) { return aus; }
        }
        Feld f;
        f.entity = static_cast<int>(i);
        f.modell = std::atoi(mdl->c_str() + 1);
        if (f.modell <= 0 || f.modell >= static_cast<int>(k->modelle.size())) { continue; }
        const auto zahlVon = [&e](const char* key) {
            const std::string* v = e.find(key);
            return v != nullptr ? std::atof(v->c_str()) : 0.0;
        };
        f.sf = static_cast<int>(zahlVon("spawnflags"));
        f.dmg = static_cast<int>(zahlVon("dmg"));   // F_INT
        if (f.dmg == 0) {
            f.dmg = 5;
        } else if (f.dmg < 0 && md) {
            f.dmg = 9999999;
            f.sf |= 32 | 16;
        }
        f.delay = static_cast<double>(static_cast<float>(zahlVon("delay"))) * 1000.0;
        f.wait = static_cast<double>(static_cast<float>(zahlVon("wait"))) * 1000.0;
        f.gelinkt = (f.sf & 1) == 0;
        f.inaktiv = (f.sf & 128) != 0;
        f.name = e.targetname;
        if (const std::string* n = e.find("NPC_targetname")) { f.npcName = *n; }
        if (const std::string* o = e.find("origin")) {
            std::istringstream is(*o);
            is >> f.ursprung[0] >> f.ursprung[1] >> f.ursprung[2];
        }
        const KollisionsKarte::Modell& m = k->modelle[static_cast<std::size_t>(f.modell)];
        for (int a = 0; a < 3; ++a) {
            f.lo[a] = m.mins[a] + f.ursprung[a];
            f.hi[a] = m.maxs[a] + f.ursprung[a];
        }
        felder.push_back(std::move(f));
    }
    // rc605 (Ende): die Explosionen der Laserfallen aus dem letzten Durchgang
    // (fallenEffekteSetzen) - G_RadiusDamage trifft auch Figuren.
    std::vector<App::FallenKnall> knalle;
    if (g_app->fallenKnalleKarte == karte.path) { knalle = g_app->fallenKnalle; }
    std::stable_sort(knalle.begin(), knalle.end(), [](const auto& x, const auto& y) { return x.ms < y.ms; });
    // rc605 (Ende): blockierte Mover schaden der Figur (MoverSim::quetschungen).
    std::vector<MoverSim::Quetschung> quetsch = g_app->moverSim.quetschungen();
    const std::vector<std::string> quetschNamen = g_app->quetschFiguren;
    if (felder.empty() && knalle.empty() && quetsch.empty()) { return aus; }
    if (!knalle.empty()) { fallenWeltBauen(); }   // die Spuren von CanDamage mit dieser Szene
    // Wer schaltet: hurt_use und target_(de)activate aus den Benutzungen.
    std::vector<const Ablauf::Benutzung*> uses;
    for (const Ablauf::Benutzung& b : g_app->ablauf.benutzt) { uses.push_back(&b); }
    std::stable_sort(uses.begin(), uses.end(), [](const auto* a, const auto* b) { return a->ms < b->ms; });
    const auto gleichName = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) { return false; }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
        }
        return true;
    };
    struct Figur {
        const Actor* a = nullptr;
        int gesundheit = 100;
        std::size_t gesetzt = 0;
        int ruestung = 0;          // rc605 (Ende): ps.stats[STAT_ARMOR]
        std::size_t rGesetzt = 0;
        bool tot = false;
        bool unsterblichGemeldet = false;
        bool hatLetzten = false;
        float letzt[3]{};
        std::vector<std::pair<double, bool>> noclip;
    };
    std::vector<Figur> figuren;
    for (const Actor& a : g_app->scene.actors) {
        if (a.brushModel > 0 || !a.haveStart || a.echterSpieler) { continue; }
        Figur f;
        f.a = &a;
        f.gesundheit = a.gesundheitStart;
        f.ruestung = a.ruestungStart;
        // Q3_SetBState: BS_NOCLIP setzt client->noclip, jeder andere Zustand
        // loescht es (Q3_Interface.cpp:2667-2674); JSF_AMBUSH haengt im noclip.
        if (a.haengtAnfangs) { f.noclip.emplace_back(-1.0, true); }
        if (a.haengtBisMs >= 0.0) { f.noclip.emplace_back(a.haengtBisMs, false); }
        for (const ActorStep& st : a.steps) {
            if (st.kind == ActorStep::Kind::Verhalten) { f.noclip.emplace_back(st.startMs, st.text == "BS_NOCLIP"); }
        }
        std::stable_sort(f.noclip.begin(), f.noclip.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
        figuren.push_back(std::move(f));
    }
    // rc605 (Ende): G_RunFrame laesst die Figuren nach ihrer Entity-Nummer denken
    // (g_main.cpp:1077-1086 / 2038-2042 OpenJK) - in ClientThink beruehren sie die
    // trigger_hurt in dieser Folge, und ein Feld teilt painDebounceTime und
    // attackDebounceTime (hurt_touch): wer zuerst kommt, nimmt den Schaden dieses
    // Bilds. Die Folge kommt aus dem Nachbau (NavFigurKoerper::reihe).
    {
        std::map<std::string, int> reiheNach;
        const Ablauf& ab = g_app->ablauf;
        for (std::size_t i = 0; i < ab.laufFiguren.size() && i < ab.laufFigurNamen.size(); ++i) {
            reiheNach[ab.laufFigurNamen[i]] = ab.laufFiguren[i].reihe;
        }
        std::stable_sort(figuren.begin(), figuren.end(), [&reiheNach](const Figur& x, const Figur& y) {
            const auto ix = reiheNach.find(x.a->name);
            const auto iy = reiheNach.find(y.a->name);
            const int rx = ix == reiheNach.end() ? std::numeric_limits<int>::max() : ix->second;
            const int ry = iy == reiheNach.end() ? std::numeric_limits<int>::max() : iy->second;
            return rx < ry;
        });
    }
    const auto zurZeit = [](const std::vector<std::pair<double, bool>>& v, double t) {
        bool an = false;
        for (const auto& [ms, w] : v) {
            if (ms > t) { break; }
            an = w;
        }
        return an;
    };
    // rc605 (Ende): G_Damage, soweit es fuer eine Figur zaehlt (code/game/g_combat.cpp:
    // 5487-6600 OpenJK, g_combat.cpp:7901 ff. MD, codeJK2/game/g_combat.cpp:4789-5390).
    // Ohne Rueckstoss (G_ApplyKnockback/G_CheckKnockdown) und ohne Schmerz-Reaktion.
    constexpr int kDamageRadius = 0x00000001;      // DAMAGE_RADIUS
    constexpr int kDamageNoArmor = 0x00000002;     // DAMAGE_NO_ARMOR
    enum class Art : std::uint8_t { TriggerHurt, Falling, ExplosiveSplash, Crush };
    // CheckArmor (g_combat.cpp:4832-4973 OpenJK, 6020-6185 MD, codeJK2 4298-4372).
    const auto checkArmor = [&](Figur& fi, int damage, int dflags) -> int {
        const Actor& a = *fi.a;
        if (damage == 0 || (dflags & kDamageNoArmor) != 0) { return 0; }
        if (!jk2 && (gleichName(a.klasse, "CLASS_ASSASSIN_DROID") || (md && gleichName(a.klasse, "CLASS_DROIDEKA")))) {
            // Explosion, trigger_hurt und Sturz stehen in keiner der Listen: ohne
            // Schild (FL_SHIELDED setzt erst die Kampf-KI) "No Additional Protection".
            return 0;
        }
        if (gleichName(a.klasse, "CLASS_GALAKMECH")) {
            if (fi.ruestung <= 0) { return 0; }
            fi.ruestung = std::max(0, fi.ruestung - damage);   // "shields take all the damage"
            return damage;
        }
        const int count = fi.ruestung;
        int save = count > a.gesundheitStart / 2 ? damage : static_cast<int>(std::ceil(static_cast<float>(damage) * 0.40F));   // ARMOR_PROTECTION
        if (damage == 1) {
            if (fi.ruestung > 0) { fi.ruestung -= save; }
            return 0;   // "WTF? returns false 0 if armor absorbs only 1 point of damage"
        }
        if (save >= count) { save = count; }
        if (save == 0) { return 0; }
        fi.ruestung -= save;
        return save;
    };
    const auto gDamage = [&](Figur& fi, int damage, int dflags, Art art, double t, int entity) {
        const Actor& a = *fi.a;
        // Rancor: von einem Nicht-Rancor unter 10 nichts, sonst 10 (OpenJK :5560-5572, MD).
        if (!jk2 && gleichName(a.klasse, "CLASS_RANCOR")) { damage = damage < 10 ? 0 : 10; }
        // Galak: Schild an - DAMAGE_NO_ARMOR gilt nicht; aus - Explosionen ein Viertel
        // (OpenJK :5718-5755, codeJK2 :4905-4940).
        if (gleichName(a.klasse, "CLASS_GALAKMECH")) {
            if (fi.ruestung > 0) {
                dflags &= ~kDamageNoArmor;
            } else if ((dflags & kDamageRadius) != 0 || art == Art::ExplosiveSplash) {
                damage = static_cast<int>(std::ceil(static_cast<float>(damage) / 4.0F));
            }
        }
        if (zurZeit(a.gottmodus, t)) { return; }   // FL_GODMODE
        int take = damage;
        take -= checkArmor(fi, take, dflags);
        // hitLoc HL_NONE: damageModifier 1.0 (MOD_EXPLOSIVE_SPLASH zaehlt ohnehin nicht).
        if (take == 0) { return; }   // "if ( take || (dflags&DAMAGE_NO_DAMAGE) )"
        fi.gesundheit = std::max(0, fi.gesundheit - take);
        // FL_UNDYING: "if(targ->health < 1) { G_ActivateBehavior( targ, BSET_DEATH );
        // targ->health = 1; }" (g_combat.cpp:6510-6518 OpenJK) - bei JEDEM Treffer, der
        // health unter 1 bringt, laeuft das deathscript erneut (rc605 Ende: bis dahin
        // nur beim ersten).
        if (zurZeit(a.unsterblich, t) && fi.gesundheit < 1) {
            aus.push_back(AblaufWelt::HurtTod{t, a.name, entity, true});
            fi.unsterblichGemeldet = true;
            fi.gesundheit = 1;
        }
        if (fi.gesundheit <= 0) {
            aus.push_back(AblaufWelt::HurtTod{t, a.name, entity, false});
            fi.tot = true;
        }
    };
    const double ende = std::max(g_app->camTrack.durationMs, g_app->timeline.practicalEndMs) + 5000.0;
    constexpr double kBild = 50.0;
    constexpr double kFrametime = 100.0;   // FRAMETIME (g_local.h:48)
    std::size_t ui = 0;
    std::size_t ki = 0;   // rc605 (Ende): naechste Explosion
    std::size_t qi = 0;   // rc605 (Ende): naechste Quetschung
    const float alle[3] = {0.0F, 0.0F, 0.0F};
    for (double t = 0.0; t <= ende; t += kBild) {
        while (ui < uses.size() && uses[ui]->ms <= t + 1e-6) {
            const std::string& n = uses[ui]->name;
            for (Feld& f : felder) {
                if (!f.name.empty() && gleichName(f.name, n)) { f.gelinkt = !f.gelinkt; }
            }
            for (const MapEntity& e : karte.entities) {
                if ((e.classname == "target_activate" || e.classname == "target_deactivate") && gleichName(e.targetname, n)) {
                    const std::string* ziel = e.find("target");
                    for (Feld& f : felder) {
                        if (ziel != nullptr && !f.name.empty() && gleichName(f.name, *ziel)) {
                            f.inaktiv = e.classname == "target_deactivate";
                        }
                    }
                }
            }
            ++ui;
        }
        for (Feld& f : felder) {
            if (f.zurueck >= 0.0 && t >= f.zurueck - 1e-6) {
                f.angriff = 0.0;
                f.zurueck = -1.0;
            }
        }
        // rc605 (Ende): SET_HEALTH / SET_ARMOR bis zu diesem Bild (Q3_SetHealth und
        // Q3_SetArmor gelten auch fuer eine unsichtbare Figur).
        for (Figur& fi : figuren) {
            const Actor& a = *fi.a;
            while (fi.gesetzt < a.gesundheitGesetzt.size() && a.gesundheitGesetzt[fi.gesetzt].first <= t + 1e-6) {
                fi.gesundheit = a.gesundheitGesetzt[fi.gesetzt].second;
                ++fi.gesetzt;
            }
            while (fi.rGesetzt < a.ruestungGesetzt.size() && a.ruestungGesetzt[fi.rGesetzt].first <= t + 1e-6) {
                fi.ruestung = a.ruestungGesetzt[fi.rGesetzt].second;
                ++fi.rGesetzt;
            }
        }
        // rc605 (Ende): eine Laserfalle explodiert in diesem Bild (ihr Denken vor den
        // spaeter gespawnten NPCs): WP_Explode -> G_RadiusDamage(currentOrigin, attacker,
        // 90, 256, NULL, MOD_EXPLOSIVE_SPLASH) (g_weapon.cpp:185-230 OpenJK;
        // G_RadiusDamage g_combat.cpp:6850-6980 OpenJK, 9704 ff. MD, codeJK2 5451-5530).
        while (ki < knalle.size() && knalle[ki].ms <= t + 1e-6) {
            const App::FallenKnall& kn = knalle[ki++];
            if (kn.ms <= t - kBild + 1e-6) { continue; }
            for (Figur& fi : figuren) {
                const Actor& a = *fi.a;
                if (fi.tot || fi.gesundheit <= 0) { continue; }
                if (a.getoetetMs >= 0.0 && t > a.getoetetMs + 1e-6) { continue; }
                const ActorState st = a.at(t);
                if (!st.visible || st.removed) { continue; }   // contents 0 / G_FreeEntity
                const float hb = a.halbBreite;
                // absmin/absmax: SV_LinkEntity, je 1 weiter
                const float lo[3] = {st.pos[0] - hb - 1.0F, st.pos[1] - hb - 1.0F, st.pos[2] - 24.0F - 1.0F};
                const float hi[3] = {st.pos[0] + hb + 1.0F, st.pos[1] + hb + 1.0F, st.pos[2] + a.hoeheOben + 1.0F};
                float v[3];
                for (int q = 0; q < 3; ++q) {
                    v[q] = kn.ort[q] < lo[q] ? lo[q] - kn.ort[q] : (kn.ort[q] > hi[q] ? kn.ort[q] - hi[q] : 0.0F);
                }
                const float dist = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
                if (dist >= 256.0F) { continue; }
                const float points = 90.0F * (1.0F - dist / 256.0F);
                const float mitte[3] = {(lo[0] + hi[0]) * 0.5F, (lo[1] + hi[1]) * 0.5F, (lo[2] + hi[2]) * 0.5F};
                if (!fallenKannTreffenBei(t, kn.ort, mitte)) { continue; }
                const int vorher = fi.gesundheit;
                gDamage(fi, static_cast<int>(points), kDamageRadius, Art::ExplosiveSplash, t, kn.entity);
                diag::detail("misc_trip_mine #" + std::to_string(kn.entity) + " explodiert bei " + std::to_string(static_cast<long>(kn.ms)) +
                             " ms: " + a.name + " Abstand " + std::to_string(static_cast<int>(dist)) + ", Schaden " +
                             std::to_string(static_cast<int>(points)) + ", Lebenspunkte " + std::to_string(vorher) + " -> " +
                             std::to_string(fi.gesundheit) + (fi.tot ? " (tot)" : ""));
            }
        }
        // rc605 (Ende): ein Mover haelt in diesem Bild an einer Figur (G_MoverTeam vor den
        // NPCs): G_TryPushingEntity bzw. Blocked_Door -> G_Damage(..., damage, 0, MOD_CRUSH).
        while (qi < quetsch.size() && quetsch[qi].ms <= t + 1e-6) {
            const MoverSim::Quetschung& q = quetsch[qi++];
            if (q.ms <= t - kBild + 1e-6 || q.schaden <= 0 || q.figur < 0 || static_cast<std::size_t>(q.figur) >= quetschNamen.size()) {
                continue;
            }
            for (Figur& fi : figuren) {
                if (fi.tot || fi.gesundheit <= 0 || fi.a->name != quetschNamen[static_cast<std::size_t>(q.figur)]) { continue; }
                const int vorher = fi.gesundheit;
                gDamage(fi, q.schaden, 0, Art::Crush, t, static_cast<int>(q.entity));
                diag::detail("Mover #" + std::to_string(q.entity) + " quetscht " + fi.a->name + " bei " + std::to_string(static_cast<long>(t)) +
                             " ms: Schaden " + std::to_string(q.schaden) + ", Lebenspunkte " + std::to_string(vorher) + " -> " +
                             std::to_string(fi.gesundheit) + (fi.tot ? " (tot)" : ""));
            }
        }
        for (Figur& fi : figuren) {
            const Actor& a = *fi.a;
            if (fi.tot) { continue; }
            if (a.getoetetMs >= 0.0 && t > a.getoetetMs + 1e-6) {
                fi.tot = true;   // im Skript getoetet: tote Clients beruehren nichts
                continue;
            }
            const ActorState st = a.at(t);
            if (!st.visible || st.removed) {
                fi.hatLetzten = false;
                continue;
            }
            // rc605 (Ende): ein SET_ORIGIN dieses Bilds laeuft am Ende von NPC_Think
            // (taskManager->Update nach ClientThink, NPC.cpp:2412-2415 MD) - die
            // Beruehrung dieses Bilds (G_TouchTriggersLerped in ClientThink) hat noch den
            // alten Ort; im naechsten Bild beginnt die Spur am neuen (client->oldOrigin).
            bool versetzt = false;
            for (const ActorStep& stp : a.steps) {
                if (stp.kind == ActorStep::Kind::Teleport && std::fabs(stp.startMs - t) < 0.5) { versetzt = true; }
            }
            const ActorState vorher = versetzt ? a.at(t - 1.0) : st;
            const float jetzt[3] = {vorher.pos[0], vorher.pos[1], vorher.pos[2]};
            float letzt[3] = {jetzt[0], jetzt[1], jetzt[2]};
            if (fi.hatLetzten) {
                for (int i = 0; i < 3; ++i) { letzt[i] = fi.letzt[i]; }
            }
            for (int i = 0; i < 3; ++i) { fi.letzt[i] = st.pos[i]; }
            fi.hatLetzten = true;
            if (zurZeit(fi.noclip, t) || fi.gesundheit <= 0) { continue; }
            const float hb = a.halbBreite;
            const float mins[3] = {-hb, -hb, -24.0F};
            const float maxs[3] = {hb, hb, a.hoeheOben};
            float diff[3] = {jetzt[0] - letzt[0], jetzt[1] - letzt[1], jetzt[2] - letzt[2]};
            float dist = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
            if (dist > 0.0F) {
                for (float& d : diff) { d /= dist; }
            }
            if (dist > 1024.0F) { dist = 0.0F; }
            // grob: der ganze Weg gegen die Kaesten der Felder
            float wlo[3];
            float whi[3];
            for (int i = 0; i < 3; ++i) {
                wlo[i] = std::min(jetzt[i], letzt[i]) + mins[i];
                whi[i] = std::max(jetzt[i], letzt[i]) + maxs[i];
            }
            for (Feld& f : felder) {
                if (fi.tot || fi.gesundheit <= 0) { break; }
                if (!f.gelinkt) { continue; }
                bool nah = true;
                for (int i = 0; i < 3; ++i) {
                    if (whi[i] < f.lo[i] - 1.0F || wlo[i] > f.hi[i] + 1.0F) { nah = false; }
                }
                if (!nah) { continue; }
                bool trifft = false;
                bool fertig = false;
                for (float cur = 0.0F; !fertig && !trifft && maxs[1] > 0.0F; cur += maxs[1] / 2.0F) {
                    float p[3];
                    if (cur >= dist) {
                        for (int i = 0; i < 3; ++i) { p[i] = jetzt[i]; }
                        fertig = true;
                    } else {
                        for (int i = 0; i < 3; ++i) { p[i] = letzt[i] + diff[i] * cur; }
                    }
                    const KollisionsSpur sp = kollisionsSpur(*k, p, mins, maxs, p, 0xFFFFFFFFU, f.modell, f.ursprung, alle);
                    trifft = sp.startsolid;
                }
                if (!trifft) { continue; }
                // hurt_touch
                if (f.inaktiv) { continue; }
                if (t < f.pain + f.wait - 1e-6) {
                    if ((f.sf & 2048) != 0) {
                        if (f.pain != 0.0 && t > f.pain + 1e-6) { continue; }
                    } else {
                        continue;
                    }
                }
                if ((f.sf & 2) != 0) { continue; }   // nur der Spieler
                if (!f.npcName.empty() && !gleichName(f.npcName, a.name)) { continue; }
                int dmg = f.dmg;
                if (f.delay != 0.0) {
                    if (f.angriff < f.delay) {
                        dmg = static_cast<int>(std::floor(static_cast<double>(static_cast<float>(f.dmg) * static_cast<float>(f.angriff) /
                                                                               static_cast<float>(f.delay))));
                    }
                    f.angriff += kFrametime;
                    f.zurueck = t + kFrametime * 2.0;
                }
                if (dmg == 0) { continue; }
                bool schaden = true;
                if ((f.sf & 32) != 0 && !md && !jk2 &&
                    (gleichName(a.klasse, "CLASS_BOBAFETT") || gleichName(a.klasse, "CLASS_ROCKETTROOPER"))) {
                    schaden = false;   // JET_FlyStart (code/game/g_trigger.cpp OpenJK)
                }
                // rc605 (Ende): G_Damage wie oben - Sturz (spawnflags 32) mit
                // DAMAGE_NO_ARMOR und MOD_FALLING, sonst MOD_TRIGGER_HURT (Ruestung zaehlt).
                if (schaden) {
                    const bool sturz = (f.sf & 32) != 0;
                    gDamage(fi, dmg, sturz ? kDamageNoArmor : 0, sturz ? Art::Falling : Art::TriggerHurt, t, f.entity);
                }
                f.pain = t;
            }
        }
    }
    return aus;
}

AblaufWelt ablaufWelt() {
    AblaufWelt w;
    w.karte = g_app->map.empty() ? nullptr : &g_app->map;
    // rc605: die Ziele explodierter misc_trip_mine aus dem letzten Durchgang
    // (fallenEffekteSetzen) - der Nachbau benutzt sie zur Zeit der Explosion.
    if (g_app->fallenUsesKarte == g_app->map.path) { w.fallenUses = g_app->fallenUses; }
    w.skript = [](const std::string& p) { return skriptFuerAblauf(p); };
    w.klangDauer = [](const std::string& f) { return soundLengthMs(f); };
    // Wie lange play ( "PLAY_ROFF", ... ) die Aufgabe haelt. Ohne
    // Spielordner weiss niemand, ob es die Datei gibt: dann "unbekannt" und
    // sofort fertig. Mit Spielordnern und ohne Datei bleibt die Aufgabe
    // offen - wie im Spiel.
    w.roffDauer = [](const std::string& name) {
        const std::shared_ptr<const Roff> r = roffFuer(name);
        if (r) {
            // rc604 (A2 Rest): "loop rof" unter OpenJK - die Bahn endet nie.
            if (roffNotizenOpenJk()) {
                bool endlos = false;
                const Roff aus = roffMitSchleife(*r, false, 12000, &endlos);
                return endlos ? kRoffEndlos : roffLaufzeitMs(aus);
            }
            return roffLaufzeitMs(*r);
        }
        return g_app->gamePaths.empty() ? -1.0 : kRoffFehlt;
    };
    // rc604 (A2 Rest): Notizen "USE <skript>" (nur OpenJK) - jede Notiz eines
    // Bilds (roffAusloesungen mit alleNotizen), auch die wiederholten einer
    // Schleife.
    w.roffSkripte = [](const std::string& name) {
        std::vector<std::pair<double, std::string>> out;
        const std::shared_ptr<const Roff> r = roffFuer(name);
        if (!r || !roffNotizenOpenJk()) {
            return out;
        }
        const Roff aus = roffMitSchleife(*r, false, 12000);
        for (const RoffAusloesung& a : roffAusloesungen(aus, true)) {
            const RoffNotizBefehl b = zerlegeRoffNotiz(aus.notizen[a.notiz], true);
            if (b.art == RoffNotizBefehl::Art::Skript && !b.datei.empty()) {
                out.emplace_back(a.ms, b.datei);
            }
        }
        return out;
    };
    // rc601 (bewegung, Wege wie im Spiel): dieselbe Welt wie die Szene.
    w.nav = navWeltVorschau();
    return w;
}

// Pfade aus dem flachen Skript zurueck auf das offene Skript. Befehle aus
// anderen Skripten behalten ihren Skriptnamen in `fremd` und bekommen einen
// leeren Pfad (dann waehlt ein Klick nichts an, und keine Kante ist
// ziehbar).
void pfadeAufsSkript() {
    const Ablauf& a = g_app->ablauf;
    const auto um = [&a](Path& p, std::string& fremd) {
        const AblaufHerkunft* h = a.woher(p);
        if (h == nullptr || h->skript < 0) {
            p.clear();
            fremd.clear();
            return;
        }
        if (h->skript == 0) {
            p = h->path;
            fremd.clear();
            return;
        }
        fremd = a.skripte[static_cast<std::size_t>(h->skript)];
        p.clear();
    };
    for (CamSegment& s : g_app->camTrack.segments) {
        um(s.path, s.fremd);
    }
    for (TimelineTrack& tr : g_app->timeline.tracks) {
        for (TimelineEvent& e : tr.events) {
            um(e.path, e.fremd);
            // Ein wait traegt den Wert aus dem Skript, nicht die Dauer des
            // Nachbaus - die ist einen Takt laenger (Selbsttest rc601: "wait
            // ( 3050 )" auf dem Block fuer ein wait ( 3000 ) im Skript). Die
            // Laenge des Blocks bleibt die nachgebaute.
            if (e.kind == TimelineEvent::Kind::Wait && !e.path.empty()) {
                const Node* n = nodeAt(g_app->doc.script(), e.path);
                if (n != nullptr && n->kind == Node::Kind::Command) {
                    std::string l = n->name + " (";
                    for (std::size_t i = 0; i < n->args.size(); ++i) { l += (i == 0 ? " " : ", ") + n->args[i].text; }
                    e.label = l + " )";
                }
            }
        }
    }
    for (Diag& d : g_app->timeline.notes) {
        std::string f;
        um(d.path, f);
    }
    for (Actor& ac : g_app->scene.actors) {
        for (ActorStep& st : ac.steps) {
            um(st.path, st.fremd);
        }
    }
}

// --- Marken aus der Karte aufloesen ----------------------------------
//
// Viele Missionen setzen die Kamera nicht auf Zahlen, sondern auf eine
// Entity: camera( MOVE, tag( "cam1b", ORIGIN ), 0 ). In md_ga_jedi ist
// JEDER Kamerapunkt so gesetzt - 507 Entities, darunter ref_tag "cam1b"
// bei 208 1796 1224.
//
// Ohne Aufloesung las der Vektorleser dort null Zahlen, und die Kamera
// stand auf 0 0 0. Genau so gemeldet: "sie ist immer im Boden."
//
// Die Auskunft steht hier und nicht im Kern, weil nur die Oberflaeche
// die Karte hat.
bool kameraMarkeOrt(const std::string& name, float* o, float* a) {
    // Welche Entities die Engine als Marke kennt: nur ref_tag
    // (g_ref.cpp, ref_link -> TAG_Add) und waypoint_navgoal (g_nav.cpp).
    // Gesucht wird ohne Gross/klein (TAG_Add: Q_strlwr "for case
    // insensitive searches on a map"). Eine andere Entity gleichen
    // Namens findet das Spiel NICHT - der Kamerabefehl scheitert dort.
    const auto klein = [](std::string x) {
        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return x;
    };
    const std::string gesucht = klein(name);
    for (const MapEntity& e : g_app->map.entities) {
        if (klein(e.targetname) != gesucht) {
            continue;
        }
        if (e.classname != "ref_tag" && e.classname.rfind("waypoint_navgoal", 0) != 0) {
            diag::detail("Kameramarke \"" + name + "\": " + e.classname +
                         " ist im Spiel keine Marke (nur ref_tag, waypoint_navgoal)");
            continue;
        }
        if (!e.origin.empty()) {
            std::istringstream is(e.origin);
            is >> o[0] >> o[1] >> o[2];
        }
        // "angle" setzt nur die Gier (F_ANGLEHACK), "angles" alle drei.
        if (const std::string* an1 = e.find("angle")) {
            a[0] = 0.0F;
            a[1] = static_cast<float>(std::atof(an1->c_str()));
            a[2] = 0.0F;
        }
        if (const std::string* an = e.find("angles")) {
            std::istringstream is(*an);
            is >> a[0] >> a[1] >> a[2];
        }
        // --- Ein ref_tag mit "target" ZEIGT auf etwas ----------------
        //
        // Gemeldet: "die Kamera von md_ga_jedi, die erste ja, aber alle
        // anderen sind falsch."
        //
        // Hier stand bisher der Satz, ein ref_tag ohne "angles" schaue
        // geradeaus. Das ist falsch, und der Quelltext sagt es
        // ausdruecklich. code/game/g_ref.cpp, ref_link():
        //
        //     if ( ent->target ) {
        //         target = G_Find( NULL, FOFS(targetname), ent->target );
        //         VectorSubtract( target->s.origin, ent->s.origin, dir );
        //         VectorNormalize( dir );
        //         vectoangles( dir, ent->s.angles );
        //     }
        //     TAG_Add( ent->targetname, ..., ent->s.angles, ... );
        //
        // Der Kommentar ueber SP_reference_tag sagt es in Worten:
        // "If you target a ref_tag at an entity, that will set the
        // ref_tag's angles toward that entity."
        //
        // In md_ga_jedi hat JEDER Kamerapunkt ein target - cam1b auf
        // t2, cam2 auf t5, cam3 auf t6 und so fort. Ohne diese Regel
        // blieben alle Winkel null.
        //
        // Warum die ERSTE trotzdem stimmte: cam1b liegt bei
        // 208 1796 1224 und t2 genau in +x davor. Die richtige Antwort
        // ist dort tatsaechlich 0 0 0 - der eine Fall, in dem der
        // Fehler nicht auffaellt. Bei cam2 sind es 273 Grad Yaw, bei
        // cam3 153, bei cam13 233.
        //
        // Das TARGET GEWINNT gegen einen vorhandenen "angles"-Schluessel:
        // ref_link schreibt s.angles ohne Bedingung, wenn es das Ziel
        // findet. Nur wenn das Ziel fehlt, bleibt es beim Schluessel -
        // deshalb steht die Aufloesung NACH dem Lesen von "angles" und
        // nicht davor.
        if (const std::string* tgt = e.find("target")) {
            if (!tgt->empty()) {
                for (const MapEntity& z : g_app->map.entities) {
                    if (z.targetname != *tgt || z.origin.empty()) {
                        continue;
                    }
                    float ziel[3] = {0.0F, 0.0F, 0.0F};
                    std::istringstream is(z.origin);
                    is >> ziel[0] >> ziel[1] >> ziel[2];
                    // aimAngles statt einer eigenen Rechnung: dieselbe
                    // Funktion, die camera(FOLLOW) und die gezeichnete
                    // Kamera benutzen. Eine vierte Abschrift von
                    // vectoangles waere genau die Sorte Doppelung, die
                    // hier schon einmal auseinandergelaufen ist.
                    aimAngles(o, ziel, a);
                    break;
                }
            }
        }
        return true;
    }
    return false;
}

void prepareScene() {
    BHED_MESS("Szene vorbereiten");
    diag::Step step("Ansicht vorbereiten");
    // rc601 (bewegung, Durchsicht 2): was ein Neuaufbau kostet und wie viele
    // Wege dabei neu gerechnet werden (navPlanZaehler) - die Zeile am Ende.
    const auto uhrAb = std::chrono::steady_clock::now();
    const NavPlanZaehler planAb = navPlanZaehler();
    struct ZeitZeile {
        std::chrono::steady_clock::time_point ab;
        NavPlanZaehler vorher;
        ~ZeitZeile() {
            const NavPlanZaehler jetzt = navPlanZaehler();
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab).count();
            char z[200];
            std::snprintf(z, sizeof(z), "Szene vorbereitet: %.1f ms, Wege %ld neu (%.1f ms) / %ld aus dem Zwischenspeicher", ms,
                          jetzt.kalt - vorher.kalt, jetzt.msKalt - vorher.msKalt, jetzt.warm - vorher.warm);
            diag::detail(z);
        }
    } zeitZeile{uhrAb, planAb};
    // Dieselbe Reihenfolge wie bisher im Zeichnen - sie ist begruendet:
    // die Szene braucht die Karte, die Effekte haengen an den Entities,
    // und die .md3-Modelle haengen ihre Texturen hinten an
    // textures.byShader an, also muss die Karte vorher stehen.
    // --- Der ICARUS-Nachbau (bhed/ablauf.h) -------------------------------
    //
    // Das Skript wird nicht mehr nur durchgezaehlt, sondern ausgefuehrt: im
    // Takt der Spiellogik, mit run, do/dowait, affect in affect, use auf
    // Skriptstarter und Spawner. Heraus kommt ein flaches Skript, aus dem
    // Kamerabahn, Zeitleiste und Szene entstehen - wie vorher, nur dass die
    // Zeiten jetzt die des Spiels sind.
    AblaufWelt welt = ablaufWelt();
    // rc605 (Ende): die Tode durch trigger_hurt aus dem letzten Durchgang
    // dieses Skripts (hurtTodeBerechnen weiter unten).
    const std::string hurtSchluessel = g_app->map.path + "|" + aktiverSkriptPfad();
    if (g_app->hurtTodeSchluessel == hurtSchluessel) { welt.hurtTode = g_app->hurtTode; }
    bool moverStandNeu = false;   // rc601 (bewegung, teil 3)
    g_app->ablauf = simuliereAblauf(g_app->doc.script(), aktiverSkriptPfad(), welt);
    {
        const Ablauf& ab = g_app->ablauf;
        diag::detail("Ablauf: Traeger \"" + ab.traeger + "\", " + std::to_string(ab.befehle) + " Befehle aus " +
                     std::to_string(ab.skripte.size()) + " Skript(en), Ende " +
                     std::to_string(static_cast<int>(ab.endeMs)) + " ms");
        for (const std::string& h : ab.hinweise) {
            diag::detail("Ablauf: " + h);
        }
    }
    const Script& flach = g_app->ablauf.flach;
    // Paket kamera, rc601: camera ( PATH, "datei" ) faehrt die ROFF-Datei -
    // gesucht wie bei PLAY_ROFF (roffFuer: Marke im Skript, Archive, lose).
    const RoffLookup kameraRoffs = [](const std::string& n) { return roffFuer(n); };
    g_app->camTrack = buildCameraTrack(flach, kameraMarkeOrt, kameraRoffs);
    g_app->timeline = buildTimeline(flach, g_app->db);
    // rc605 (Ende): nach einem Kartenwechsel gibt es diese Karte nicht mehr -
    // Zeitleiste und Kamerafahrt enden dort (der Nachbau hoerte in diesem Bild
    // auf; was vorher anfing - Klaenge, Wege, Fahrten - laeuft nicht weiter).
    if (g_app->ablauf.kartenwechselMs >= 0.0) {
        const double kw = g_app->ablauf.kartenwechselMs;
        g_app->timeline.practicalEndMs = std::min(g_app->timeline.practicalEndMs, kw);
        g_app->timeline.durationMs = std::min(g_app->timeline.durationMs, kw);
        g_app->camTrack.durationMs = std::min(g_app->camTrack.durationMs, kw);
    }
    // Die ROFF-Bahnen schon im ersten Aufbau: eine Entity, die nur per
    // PLAY_ROFF fliegt, soll auch ohne zweiten Durchgang fliegen.
    SzenenHilfe nurRoff;
    nurRoff.roff = [](const std::string& n) { return roffFuer(n); };
    nurRoff.nav = welt.nav;   // rc601 (bewegung, Wege)
    nurRoff.schwierigkeit = schwierigkeitJetzt();   // rc601 (bewegung, Durchsicht 2; integ: -1 nach Regelwerk)
    // rc602: auch die Figuren, die beim Laden der Karte spawnen (ohne affect) -
    // der erste Aufbau sagt, welche Figuren Modelle brauchen.
    nurRoff.kartenFiguren = true;
    nurRoff.fremdeDarsteller = &g_app->ablauf.fremdeDarsteller;   // rc604 fix604
    g_app->scene = buildScene(flach, g_app->map, &nurRoff);
    g_app->camTrackValid = true;
    ++g_app->szeneBauten;
    // --- Zweiter Durchgang: mit dem, was die Figuren mitbringen ---------
    //
    // Der erste Aufbau sagt, WELCHE Figuren es gibt. Mit ihren Modellen
    // kommen die .npc (Geh- und Lauftempo) und die animation.cfg (Laenge
    // jeder Animation). Beides aendert die Zeiten: ein wait( "task" )
    // wartet, bis die Animation darin abgelaufen ist, und ein Weg dauert
    // so lange, wie das Tempo der Figur es sagt.
    if (!g_app->scene.actors.empty()) {
        loadActorModels();
        kinoSkeletteAbgleichen();
        std::map<std::string, std::size_t> index;
        for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
            index[g_app->scene.actors[i].name] = i;
        }
        const auto eintrag = [index](const std::string& figur,
                                     const std::string& anim) -> const AnimEntry* {
            const auto it = index.find(figur);
            if (it == index.end() || it->second >= g_app->actorAssets.size()) {
                return nullptr;
            }
            const App::Skeleton* sk = g_app->actorAssets[it->second]->skeleton;
            if (sk == nullptr) {
                return nullptr;
            }
            for (const AnimEntry& e : sk->sections) {
                if (e.name == anim) {
                    return &e;
                }
            }
            return nullptr;
        };
        // frameLerp wie die Engine: ceil(1000/fps), negativ floor.
        const auto lerpVon = [](const AnimEntry& e) {
            return (e.fps > 0) ? std::ceil(1000.0 / e.fps)
                               : (e.fps < 0) ? std::fabs(std::floor(1000.0 / e.fps)) : 1000.0;
        };
        SzenenHilfe hilfe;
        hilfe.npcs = &g_app->npcMap;
        // Fuer Waffe und Schwerter (src/ausruestung.cpp). Beide sind hier
        // geladen: loadActorModels liest sie zusammen mit den .npc.
        hilfe.sabers = &g_app->saberMap;
        hilfe.waffen = &g_app->weaponMap;
        hilfe.roff = [](const std::string& n) { return roffFuer(n); };
        hilfe.nav = welt.nav;   // rc601 (bewegung, Wege)
        hilfe.erscheint = &g_app->ablauf.erscheint;   // rc601 (bewegung, teil 3)
        // rc601 (bewegung, Durchsicht 2): g_spskill aus den Einstellungen.
        hilfe.schwierigkeit = schwierigkeitJetzt();   // rc601 integ: -1 nach Regelwerk
        hilfe.kartenFiguren = true;   // rc602: Figuren vom Kartenstart
        hilfe.fremdeDarsteller = &g_app->ablauf.fremdeDarsteller;   // rc604 fix604
        hilfe.animDauer = [eintrag, lerpVon](const std::string& f, const std::string& a) {
            const AnimEntry* e = eintrag(f, a);
            return (e == nullptr) ? -1.0
                                  : std::max(0, e->numFrames - 1) * lerpVon(*e);
        };
        hilfe.bildDauer = [eintrag, lerpVon](const std::string& f, const std::string& a) {
            const AnimEntry* e = eintrag(f, a);
            return (e == nullptr) ? -1.0 : lerpVon(*e);
        };
        // Mit den Laengen der Animationen und den Tempi der Figuren noch
        // einmal ausfuehren: dowait wartet auf beides, also verschieben
        // sich die Zeiten ALLER Spuren, auch die der Kamera.
        const Scene ersteSzene = buildScene(g_app->ablauf.flach, g_app->map, &hilfe);
        welt.animDauer = hilfe.animDauer;
        // rc601 (bewegung): die .npc-Tempi beim Spawnen, nicht das letzte
        // SET_WALKSPEED im Skript (scene.h, ablaufTempo).
        welt.tempo = ablaufTempo(ersteSzene);
        // rc601 (bewegung, Durchsicht 2): die Drehrate je Waffe in der Hand.
        welt.drehrateNpc = ablaufDrehrate(ersteSzene);
        welt.drehrateYaw = ablaufDrehrateYaw(ersteSzene);
        welt.verhaltenTakt = ablaufVerhaltenTakt(ersteSzene, schwierigkeitJetzt());   // rc601 integ
        // rc601 (bewegung, teil 3): Fallen und Vorlauf ab dem Spawn je Figur.
        welt.vorlauf = ablaufVorlauf(ersteSzene);
        welt.bleibtFest = ablaufBleibtFest(ersteSzene);   // rc605 fix605: Mark1/Interrogator
        // rc601 (bewegung, Wege): der Kasten je Figur wie in der Szene -
        // STEER::Reached sieht die Ankunft am Kasten (absmin/absmax).
        {
            auto kaesten = std::make_shared<std::map<std::string, std::pair<float, float>>>();
            for (const Actor& a : ersteSzene.actors) { (*kaesten)[a.name] = {a.halbBreite, a.hoeheOben}; }
            // rc601 (bewegung, R5/R6): Drehrate und Stufe je Figur.
            auto rest = std::make_shared<std::map<std::string, std::array<float, 3>>>();
            for (const Actor& a : ersteSzene.actors) {
                (*rest)[a.name] = {a.festeDrehrate > 0.0F ? a.festeDrehrate : a.startDrehrate,
                                   a.festeDrehrate > 0.0F ? 1.0F : 0.0F, a.stufe};
            }
            welt.drehrate = [rest](const std::string& f) {
                const auto it = rest->find(f);
                return it == rest->end() ? -1.0F : it->second[0];
            };
            welt.drehrateFest = [rest](const std::string& f) {
                const auto it = rest->find(f);
                return it != rest->end() && it->second[1] > 0.5F;
            };
            welt.stufe = [rest](const std::string& f) {
                const auto it = rest->find(f);
                return it == rest->end() ? 18.0F : it->second[2];
            };
            // rc605: SCF_NAV_CAN_JUMP je Figur (NPC-Spruenge).
            auto springer = std::make_shared<std::set<std::string>>();
            for (const Actor& a : ersteSzene.actors) {
                if (a.kannSpringen) { springer->insert(a.name); }
            }
            welt.kannSpringen = [springer](const std::string& f) { return springer->count(f) != 0; };
            // rc605 (Rest): PM_CrashLand je Figur (FP_LEVITATION, npcLandKlasse).
            auto landWerte = std::make_shared<std::map<std::string, std::pair<int, int>>>();
            for (const Actor& a : ersteSzene.actors) {
                (*landWerte)[a.name] = {a.levitation, npcLandKlasse(navMovieDuelsRegeln(), navJk2Regeln(), a.klasse, a.rang)};
            }
            welt.levitation = [landWerte](const std::string& f) {
                const auto it = landWerte->find(f);
                return it == landWerte->end() ? 0 : it->second.first;
            };
            welt.landKlasse = [landWerte](const std::string& f) {
                const auto it = landWerte->find(f);
                return it == landWerte->end() ? 0 : it->second.second;
            };
            welt.halbBreite = [kaesten](const std::string& f) {
                const auto it = kaesten->find(f);
                return it == kaesten->end() ? 16.0F : it->second.first;
            };
            welt.hoeheOben = [kaesten](const std::string& f) {
                const auto it = kaesten->find(f);
                return it == kaesten->end() ? 40.0F : it->second.second;
            };
        }
        // rc601 (bewegung, teil 3): alle Figuren im selben Takt (andere Figuren
        // sind Hindernisse) - Ablauf und Szene nehmen dieselben Plaene.
        {
            int durchgaenge = 1;
            g_app->ablauf = simuliereAblaufMitKoerpern(g_app->doc.script(), aktiverSkriptPfad(), welt, &durchgaenge);
            hilfe.koerperPlaene = welt.koerperPlaene;
            hilfe.schubWege = welt.schubWege;   // rc605 (Ende, Jedi Outcast)
            g_app->koerperPlaene = welt.koerperPlaene;   // rc604 fix604
            if (durchgaenge > 1) {
                diag::detail("Wege: andere Figuren im Weg - " + std::to_string(welt.koerperPlaene ? welt.koerperPlaene->size() : 0U) +
                             " Plaene im gemeinsamen Takt, " + std::to_string(durchgaenge) + " Ablauf-Rechnungen");
            }
        }
        g_app->camTrack = buildCameraTrack(g_app->ablauf.flach, kameraMarkeOrt, kameraRoffs);
        g_app->timeline = buildTimeline(g_app->ablauf.flach, g_app->db);
        g_app->scene = buildScene(g_app->ablauf.flach, g_app->map, &hilfe);
        ++g_app->szeneBauten;
    }
    // rc605 (Ende): nach dem Kartenwechsel gibt es die Karte nicht mehr - auch
    // nach dem zweiten Aufbau enden Zeitleiste, Kamerafahrt und Szene dort.
    {
        static std::string letzterWechsel;
        std::string wechsel;
        if (g_app->ablauf.kartenwechselMs >= 0.0) {
            const double kw = g_app->ablauf.kartenwechselMs;
            g_app->timeline.practicalEndMs = std::min(g_app->timeline.practicalEndMs, kw);
            g_app->timeline.durationMs = std::min(g_app->timeline.durationMs, kw);
            g_app->camTrack.durationMs = std::min(g_app->camTrack.durationMs, kw);
            szeneEndetBei(g_app->scene, kw);
            wechsel = std::to_string(static_cast<long>(kw)) + "|" + g_app->ablauf.kartenwechselKarte;
            if (wechsel != letzterWechsel) {
                char zeile[300];
                std::snprintf(zeile, sizeof(zeile), tr(Str::BewKartenwechsel), kw / 1000.0, g_app->ablauf.kartenwechselKarte.c_str());
                addStatus(zeile);
            }
        }
        letzterWechsel = wechsel;
    }
    // rc605 (Schlussrunde): ein '+'-Menue (G_ChangeMap: uimenu) haelt das Spiel an,
    // bis der Spieler es schliesst - die Karte endet nicht.
    {
        static std::string letztesMenue;
        std::string menue;
        if (g_app->ablauf.menueMs >= 0.0) {
            menue = std::to_string(static_cast<long>(g_app->ablauf.menueMs)) + "|" + g_app->ablauf.menueName;
            if (menue != letztesMenue) {
                char zeile[300];
                std::snprintf(zeile, sizeof(zeile), tr(Str::BewMenue), g_app->ablauf.menueName.c_str(), g_app->ablauf.menueMs / 1000.0);
                addStatus(zeile);
            }
        }
        letztesMenue = menue;
    }
    // rc605 (Ende): SET_MISSIONFAILED - der Bildschirm "Mission gescheitert",
    // stop_icarus haelt alle Skripte an (der Nachbau hoerte dort auf); Wege und
    // Mover laufen weiter wie im Spiel (G_RunFrame laeuft weiter).
    {
        static std::string letzteMission;
        std::string mission;
        if (g_app->ablauf.missionFailedMs >= 0.0 && g_app->ablauf.skripteStoppMs >= 0.0) {
            mission = std::to_string(static_cast<long>(g_app->ablauf.missionFailedMs)) + "|" + g_app->ablauf.missionFailedText +
                      "|" + std::to_string(static_cast<long>(g_app->ablauf.skripteStoppMs));
            if (mission != letzteMission) {
                char zeile[400];
                std::snprintf(zeile, sizeof(zeile), tr(Str::BewMissionGescheitert), g_app->ablauf.missionFailedMs / 1000.0,
                              g_app->ablauf.missionFailedText.c_str(),
                              g_app->ablauf.skripteStoppMs / 1000.0);
                addStatus(zeile);
            }
        }
        letzteMission = mission;
    }
    // Figuren, die erst waehrend der Szene gespawnt werden (NPC_spawner per
    // use), gibt es vorher nicht: unsichtbar bis NPC_Begin.
    for (Actor& ac : g_app->scene.actors) {
        const auto it = g_app->ablauf.erscheint.find(ac.name);
        if (it == g_app->ablauf.erscheint.end()) {
            continue;
        }
        ActorStep weg;
        weg.kind = ActorStep::Kind::Hide;
        ActorStep da;
        da.kind = ActorStep::Kind::Show;
        da.startMs = it->second;
        da.endMs = it->second;
        ac.steps.insert(ac.steps.begin(), weg);
        // rc601 (bewegung): ab hier faellt sie (NPC_Begin), vorher nicht.
        ac.erscheintMs = it->second;
        ac.steps.push_back(da);
        std::stable_sort(ac.steps.begin(), ac.steps.end(),
                         [](const ActorStep& x, const ActorStep& y) { return x.startMs < y.startMs; });
    }
    // Der Spieler ist unsichtbar, solange die Skriptkamera an ist: CG_Player
    // bricht fuer clientNum 0 ab, wenn in_camera gesetzt ist
    // (cg_players.cpp:15602). In md_am/end_cin stand sonst die Vorgabefigur
    // mitten auf der Plattform - im Spiel spielt dort ani3 die Rolle, und
    // erst nach der Szene uebernimmt der Spieler (SET_COPY_ORIGIN).
    for (Actor& ac : g_app->scene.actors) {
        std::string n = ac.name;
        for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        // rc604 fix604 (JK2): in Jedi Outcast heisst der Spieler "kyle"; CG_Player
        // laesst ihn in der Kamera ebenso weg (codeJK2/cgame/cg_players.cpp:4809,
        // "in_camera && !(eFlags & EF_NPC)").
        const bool spieler = n == "player" || (regelwerkJediOutcast() && n == "kyle" && !ac.kartenFigur);
        if (!spieler) {
            continue;
        }
        ac.kameraVerborgen.clear();
        std::vector<const CamSegment*> schalter;
        for (const CamSegment& s : g_app->camTrack.segments) {
            if (s.kind == CamSegment::Kind::Enable || s.kind == CamSegment::Kind::Disable) {
                schalter.push_back(&s);
            }
        }
        std::stable_sort(schalter.begin(), schalter.end(),
                         [](const CamSegment* x, const CamSegment* y) { return x->startMs < y->startMs; });
        double an = -1.0;
        for (const CamSegment* s : schalter) {
            if (s->kind == CamSegment::Kind::Enable) {
                if (an < 0.0) { an = s->startMs; }
            } else if (an >= 0.0) {
                ac.kameraVerborgen.emplace_back(an, s->startMs);
                an = -1.0;
            }
        }
        if (an >= 0.0) {
            ac.kameraVerborgen.emplace_back(an, -1.0);
        }
    }
    // rc605 (Rest): Jedi Outcast - der Kyle-NPC gleicht ausserhalb der Kamera
    // die Waffe des Spielers ab (munroWaffenAbgleich, src/ausruestung.cpp).
    if (regelwerkJediOutcast()) {
        SzenenHilfe wh;
        wh.npcs = &g_app->npcMap;
        wh.sabers = &g_app->saberMap;
        wh.waffen = &g_app->weaponMap;
        munroWaffenAbgleich(g_app->scene, &wh);
    }
    // Ein NPC_type, der in keiner .npc steht, wird im Spiel NIE gespawnt:
    // NPC_ParseParms schlaegt fehl, NPC_Spawn_Do meldet "Couldn't spawn
    // NPC" und gibt die Entity frei (NPC_spawn.cpp:2577). So Tavion am Ende
    // von yavin1 ("tavion_new_dark" gibt es in keinem Archiv).
    if (!g_app->npcMap.empty()) {
        for (Actor& ac : g_app->scene.actors) {
            if (ac.npcType.empty() || ac.brushModel > 0) {
                continue;
            }
            std::string typ = ac.npcType;
            for (char& c : typ) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (g_app->npcMap.count(typ) != 0) {
                continue;
            }
            diag::info("Figur " + ac.name + ": NPC_type \"" + ac.npcType +
                       "\" steht in keiner .npc - im Spiel wird sie nicht gespawnt");
            std::erase_if(ac.steps, [](const ActorStep& s) { return s.kind == ActorStep::Kind::Show; });
            ActorStep weg;
            weg.kind = ActorStep::Kind::Hide;
            ac.steps.insert(ac.steps.begin(), weg);
        }
    }
    // SET_CAMERA_GROUP aus dem Ablauf einsammeln (fuer camera FOLLOW).
    g_app->kameraGruppen.clear();
    for (const Node& n : g_app->ablauf.flach.nodes) {
        if (n.name != "affect" || n.args.empty()) {
            continue;
        }
        std::string wer = n.args[0].text;
        for (char& c : wer) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        double uhr = 0.0;
        for (const Node& k : n.children) {
            if (k.name == "wait" && !k.args.empty()) {
                uhr += std::atof(k.args[0].text.c_str());
            } else if (k.name == "set" && k.args.size() >= 2 &&
                       (k.args[0].text == "SET_CAMERA_GROUP" || k.args[0].text == "set_camera_group")) {
                g_app->kameraGruppen[wer].emplace_back(uhr, k.args[1].text);
            }
        }
    }
    // Die Pfade zeigen jetzt ins flache Skript - zurueck auf die Zeilen des
    // offenen Skripts, damit Anklicken, Kantenziehen und "write to script"
    // die richtige Zeile treffen.
    pfadeAufsSkript();
    // --- Paket bewegung (rc601): Schwerkraft ---------------------------
    //
    // Gemeldet: "NPCs aren't affected by gravity." Erst jetzt, an der
    // FERTIGEN Szene (alle Schritte, Spawnzeiten und Verstecke sind drin),
    // und noch vor der Abtastung fuer die Tuerausloeser - die sollen die
    // Figuren dort sehen, wo sie im Spiel stehen.
    schwerkraftVorschau(false);
    // --- Paket bewegung (rc601): Laufziele ohne Weg im Wegnetz ---------
    //
    // Gemeldet: "NPCs don't walk to waypoints correctly." Findet ein NPC
    // keinen Weg (STEER darf nicht gerade hin, NAV::GoTo findet im Wegnetz
    // nichts), bleibt er im Spiel stehen, und ein dowait auf den Lauf endet
    // nie. rc601 (Wege): die Vorschau tut jetzt genau das (planeLauf) - der
    // Hinweis sagt nur noch, warum.
    {
        // In die Statusliste nur, wenn sich die Meldungen geaendert haben -
        // prepareScene laeuft nach jeder Aenderung am Skript.
        static std::string letzteNavMeldungen;
        std::string navMeldungen;
        const WegNetz netz = baueWegNetz(g_app->map.entities);
        const std::vector<NavOhneWeg> ohneWeg = navZieleOhneWeg(g_app->scene, netz);
        for (const NavOhneWeg& w : ohneWeg) {
            navMeldungen += g_app->scene.actors[w.figur].name + "|" + std::to_string(static_cast<int>(w.ziel[0])) + " " +
                            std::to_string(static_cast<int>(w.ziel[1])) + "|" + std::to_string(static_cast<int>(w.ms)) + ";";
        }
        const bool insStatus = navMeldungen != letzteNavMeldungen;
        letzteNavMeldungen = navMeldungen;
        for (const NavOhneWeg& w : ohneWeg) {
            std::string ziel;
            // Der Name des Ziels: die Entity an diesem Ort (waypoint_navgoal,
            // ref_tag ...) - auch ohne Netz, dann gibt es keinen Zielknoten.
            for (const MapEntity& e : g_app->map.entities) {
                float o[3];
                if (!e.targetname.empty() && entityOrt(e, o) && std::fabs(o[0] - w.ziel[0]) < 1.0F &&
                    std::fabs(o[1] - w.ziel[1]) < 1.0F && std::fabs(o[2] - w.ziel[2]) < 48.0F) {
                    ziel = "\"" + e.targetname + "\"";
                    break;
                }
            }
            if (ziel.empty()) {
                char t[96];
                std::snprintf(t, sizeof(t), "(%.0f %.0f %.0f)", static_cast<double>(w.ziel[0]),
                              static_cast<double>(w.ziel[1]), static_cast<double>(w.ziel[2]));
                ziel = t;
            }
            diag::info("Figur " + g_app->scene.actors[w.figur].name + ": Laufziel " + ziel + " ist " +
                       std::to_string(static_cast<int>(w.abstand)) + " Einheiten entfernt (ab " +
                       std::to_string(static_cast<int>(w.ms)) +
                       " ms) und findet keinen Weg - sie bleibt stehen wie im Spiel (STEER::Stop), "
                       "ein dowait auf den Lauf endet nie.");
            if (insStatus) {
                char zeile[600];
                std::snprintf(zeile, sizeof(zeile), tr(Str::BewNavOhneWeg), g_app->scene.actors[w.figur].name.c_str(),
                              ziel.c_str(), static_cast<int>(w.abstand), w.ms / 1000.0);
                addStatus(zeile);
            }
        }
    }
    // --- Die Karte selbst: Tueren, func_wall, fx_runner auf use --------
    //
    // Aus jedem `use` der Zeitleiste und den Wegen der Figuren (fuer die
    // Ausloesefelder der Tueren). Die Figurenorte einmal im Takt der
    // Spiellogik abgetastet - die Simulation fragt sie tausendfach ab.
    {
        // Aus dem Nachbau: jede Benutzung, auch ueber target_relay,
        // target_delay, target_counter und trigger_* hinweg - die Kette ist
        // dort schon aufgeloest (MoverUse::aufgeloest).
        std::vector<MoverUse> uses;
        for (const Ablauf::Benutzung& b : g_app->ablauf.benutzt) {
            // Mit dem Ausloeser: target_print zeigt nur, was der Spieler
            // ausloest (Use_Target_Print, activator->client).
            uses.push_back(MoverUse{b.ms, b.name, true, b.ausloeser});
        }
        // rc605: die Ziele explodierender misc_trip_mine (WP_Explode ->
        // G_UseTargets) rechnet der Nachbau (ablaufWelt, AblaufWelt::
        // fallenUses) - Relais, Zaehler und usescript; sie stehen schon oben
        // in ablauf.benutzt. Bis rc605 (fix605) gingen sie unaufgeloest hierher,
        // und Nachbau und MoverSim zaehlten getrennt.
        const double ende = std::max(g_app->camTrack.durationMs,
                                     g_app->timeline.practicalEndMs) + 5000.0;
        constexpr double kTakt = 50.0;
        struct Ort { float p[3]; };
        std::vector<std::vector<Ort>> figuren;   // [schritt][figur]
        const auto schritte = static_cast<std::size_t>(ende / kTakt) + 2U;
        figuren.resize(schritte);
        for (const Actor& a : g_app->scene.actors) {
            if (a.brushModel > 0 || !a.haveStart) {
                continue;   // nur Figuren loesen Tueren aus
            }
            for (std::size_t s = 0; s < schritte; ++s) {
                const ActorState st = a.at(static_cast<double>(s) * kTakt);
                if (!st.visible) {
                    continue;
                }
                figuren[s].push_back(Ort{{st.pos[0], st.pos[1], st.pos[2]}});
            }
        }
        const auto figur = [&figuren](double ms, const float mn[3], const float mx[3]) {
            const auto s = static_cast<std::size_t>(std::max(0.0, ms) / kTakt);
            if (s >= figuren.size()) {
                return false;
            }
            // Der Kasten eines NPC (-16 -16 -24 .. 16 16 40) gegen das Feld.
            for (const Ort& o : figuren[s]) {
                if (o.p[0] + 16.0F >= mn[0] && o.p[0] - 16.0F <= mx[0] &&
                    o.p[1] + 16.0F >= mn[1] && o.p[1] - 16.0F <= mx[1] &&
                    o.p[2] + 40.0F >= mn[2] && o.p[2] - 24.0F <= mx[2]) {
                    return true;
                }
            }
            return false;
        };
        // Das eigene Bruchstueckmodell "_c1" nimmt CG_Chunks nur, wenn es
        // geladen werden konnte - also: liegt es in einem Archiv?
        g_app->moverSim.dateiDa = [](const std::string& pfad) {
            for (const auto& gp : g_app->gamePaths) {
                for (const auto& a : gp.archives) {
                    if (a.find(pfad) != nullptr) {
                        return true;
                    }
                }
            }
            return false;
        };
        // rc604: was G_SpawnGEntityFromSpawnVars wegen notsingle/Schwierigkeit
        // entfernt, gibt es auch fuer Tueren, Plattformen und Lautsprecher nicht.
        g_app->moverSim.spielstufe = regelwerkSpielstufe();
        // rc605 (Ende): G_MoverPush (g_mover.cpp:284-500 MD) - haelt eine Figur den
        // Brush auf? Die Figuren im letzten Bild (die Mover denken vor den spaeter
        // gespawnten NPCs): ein Mitfahrer (PM_GroundTrace: 0,25 hinab trifft den
        // Brush, wo er stand) oder einer, der im Brush an dessen neuem Ort steckt
        // (G_TestEntityPosition(check) == pusher), wird um move verschoben
        // (G_TryPushingEntity); ist dort etwas (Welt, der Brush, ein anderer Koerper)
        // und auch der alte Ort nicht frei, ist der Mover blockiert.
        struct KoerperOrt {
            float p[3];
            float hb;
            float hoch;
            int nr;
        };
        std::vector<std::vector<KoerperOrt>> koerper(schritte);
        g_app->quetschFiguren.clear();
        for (const Actor& a : g_app->scene.actors) {
            if (a.brushModel > 0 || !a.haveStart || a.echterSpieler) { continue; }
            const int nr = static_cast<int>(g_app->quetschFiguren.size());
            g_app->quetschFiguren.push_back(a.name);
            for (std::size_t si = 0; si < schritte; ++si) {
                const double ms = static_cast<double>(si) * kTakt;
                if (a.getoetetMs >= 0.0 && ms > a.getoetetMs + 0.5) { break; }   // eine Leiche raeumt Blocked_Door weg
                const ActorState st = a.at(ms);
                if (!st.visible || st.removed) { continue; }
                koerper[si].push_back(KoerperOrt{{st.pos[0], st.pos[1], st.pos[2]}, a.halbBreite, a.hoeheOben, nr});
            }
        }
        const std::shared_ptr<const KollisionsKarte> kk = geladeneKollision(g_app->map.path);
        // Die Welt mit den anderen Brush-Entities zur Zeit (der Stand der Mover aus dem
        // letzten Durchgang, navWeltVorschau) - der schiebende Brush selbst steht an `nach`.
        const std::shared_ptr<const NavWelt> nwelt = navWeltVorschau();
        std::map<int, int> entityVonModell;
        for (std::size_t ei = 0; ei < g_app->map.entities.size(); ++ei) {
            const std::string* mdl = g_app->map.entities[ei].find("model");
            if (mdl != nullptr && mdl->size() > 1 && (*mdl)[0] == '*') { entityVonModell[std::atoi(mdl->c_str() + 1)] = static_cast<int>(ei); }
        }
        g_app->moverSim.blockiert = [&koerper, kk, schritte, nwelt, entityVonModell](int modell, double ms, const float von[3],
                                                                                     const float nach[3]) -> int {
            if (!kk || kk->leer()) { return -1; }
            const auto ie = entityVonModell.find(modell);
            const int pusherEnt = ie == entityVonModell.end() ? kNavKeineEntity : ie->second;
            const auto si = static_cast<std::size_t>(std::max(0.0, ms - kTakt) / kTakt);
            if (si >= schritte) { return -1; }
            const std::uint32_t maske = kNavSolid | kNavMonsterClip | kNavTerrain;   // MASK_NPCSOLID ohne CONTENTS_BODY
            const float null3[3] = {0.0F, 0.0F, 0.0F};
            const float move[3] = {nach[0] - von[0], nach[1] - von[1], nach[2] - von[2]};
            for (const KoerperOrt& f : koerper[si]) {
                const float mi[3] = {-f.hb, -f.hb, -24.0F};
                const float ma[3] = {f.hb, f.hb, f.hoch};
                const float unten[3] = {f.p[0], f.p[1], f.p[2] - 0.25F};
                const KollisionsSpur g = kollisionsSpur(*kk, f.p, mi, ma, unten, maske, modell, von, null3);
                const bool reiter = !g.startsolid && g.fraction < 1.0F;
                if (!reiter && !kollisionsSpur(*kk, f.p, mi, ma, f.p, maske, modell, nach, null3).startsolid) { continue; }
                const auto frei = [&](const float q[3]) {
                    if (nwelt && (nwelt->spurZeit || nwelt->spur)) {
                        const NavVec qv{q[0], q[1], q[2]};
                        const NavVec miv{mi[0], mi[1], mi[2]};
                        const NavVec mav{ma[0], ma[1], ma[2]};
                        const NavSpur w = nwelt->spurZeit ? nwelt->spurZeit(qv, miv, mav, qv, pusherEnt, maske, ms)
                                                          : nwelt->spur(qv, miv, mav, qv, pusherEnt, maske);
                        if (w.startsolid || w.allsolid) { return false; }
                    } else if (kollisionsSpur(*kk, q, mi, ma, q, maske).startsolid) {
                        return false;
                    }
                    if (kollisionsSpur(*kk, q, mi, ma, q, maske, modell, nach, null3).startsolid) { return false; }
                    for (const KoerperOrt& o : koerper[si]) {
                        if (o.nr == f.nr) { continue; }
                        bool drin = true;
                        const float omi[3] = {-o.hb, -o.hb, -24.0F};
                        const float oma[3] = {o.hb, o.hb, o.hoch};
                        for (int q2 = 0; q2 < 3; ++q2) {
                            if (q[q2] + ma[q2] <= o.p[q2] + omi[q2] || q[q2] + mi[q2] >= o.p[q2] + oma[q2]) { drin = false; }
                        }
                        if (drin) { return false; }   // CONTENTS_BODY
                    }
                    return true;
                };
                const float neu[3] = {f.p[0] + move[0], f.p[1] + move[1], f.p[2] + move[2]};
                if (frei(neu)) { continue; }   // geschoben (bzw. mitgenommen)
                if (frei(f.p)) { continue; }   // "ok to leave in the old position"
                return f.nr;
            }
            return -1;
        };
        g_app->moverSim.baue(g_app->map, g_app->geo, std::move(uses), figur, ende);
        g_app->moverSim.blockiert = nullptr;
        for (const MoverSim::Quetschung& q : g_app->moverSim.quetschungen()) {
            diag::detail("Mover *" + std::to_string(q.modell) + " blockiert bei " + std::to_string(static_cast<long>(q.ms)) + " ms von " +
                         (q.figur >= 0 && static_cast<std::size_t>(q.figur) < g_app->quetschFiguren.size() ? g_app->quetschFiguren[static_cast<std::size_t>(q.figur)] : std::string("?")) +
                         " (Schaden " + std::to_string(q.schaden) + (q.umgekehrt ? ", kehrt um" : "") + ")");
        }
        // rc601 (bewegung): faehrt ein Aufzug oder dreht sich eine Plattform,
        // traegt sie die Figuren jetzt mit - ein zweiter Durchgang mit der
        // Mover-Simulation als Boden.
        if (schwerkraftMitMovernNoetig()) {
            schwerkraftVorschau(true);
        }
        // rc601 (bewegung, teil 3): "Doors and movers at their state at that
        // time" - die Wege spuren gegen die Mover dort, wo diese Simulation
        // sie zu jeder Zeit hat, und is_valid liest den moverState daraus.
        // Die Engine rechnet beides im selben Bild; hier haengen die Tueren
        // an den Figurenorten (Ausloesefelder) und die Wege an den Tueren.
        // Darum: hat sich der Stand der Mover geaendert, noch ein Durchgang
        // mit dem neuen Stand, bis er gleich bleibt. Weil ein Mover zur Zeit t
        // nur an den Figuren VOR t haengt und die Figuren zur Zeit t nur an
        // den Movern bis t, ist der Stand danach genau der des Spiels.
        moverStandNeu = navMoverPlanAusSim(ende);
        for (const Bruch& b : g_app->moverSim.brueche()) {
            diag::detail(b.klasse + " #" + std::to_string(b.entity) + " zerbricht bei " +
                         std::to_string(static_cast<int>(b.ms)) + " ms (Material " +
                         std::to_string(b.material) + ", " + std::to_string(b.stuecke) + " Stuecke)");
        }
        for (const Bildschirmtext& bt : g_app->moverSim.bildschirmtexte()) {
            diag::detail("target_print #" + std::to_string(bt.entity) + " bei " +
                         std::to_string(static_cast<int>(bt.ms)) + " ms" +
                         (bt.gezeigt ? "" : " (nicht vom Spieler ausgeloest - kommt im Spiel nicht an)") +
                         ": " + bt.text);
        }
        for (const MoverSim::Oeffnung& o : g_app->moverSim.oeffnungen()) {
            diag::detail("Tuer *" + std::to_string(o.modell) + " oeffnet ab " +
                         std::to_string(static_cast<int>(o.abMs)) + " ms, offen ab " +
                         std::to_string(static_cast<int>(o.offenMs)) + " ms");
        }
        for (const FxRunnerStart& f : g_app->moverSim.fxStarts()) {
            diag::detail("fx_runner #" + std::to_string(f.entity) + " startet bei " +
                         std::to_string(static_cast<int>(f.ms)) + " ms" +
                         (f.oneShot ? " (einmal)" : ""));
        }
        // Die Fahrgeraeusche (soundSet -> sound/sound.txt, bmodelSet).
        if (!g_app->bmodelSetsGelesen) {
            g_app->bmodelSetsGelesen = true;
            std::string text;
            if (readFromArchives("sound/sound.txt", text)) {
                g_app->bmodelSets = leseBmodelSets(text);
            }
        }
        g_app->moverKlaenge = g_app->moverSim.klaenge(ende);
        if (!g_app->moverKlaenge.empty()) {
            diag::detail(std::to_string(g_app->moverKlaenge.size()) + " Moverklaenge, " +
                         std::to_string(g_app->bmodelSets.size()) + " bmodelSets bekannt");
        }
    }
    // rc601 (bewegung, teil 3): siehe oben (navMoverPlanAusSim).
    {
        static int moverDurchgang = 0;
        if (moverStandNeu) {
            if (moverDurchgang < 8) {
                ++moverDurchgang;
                diag::detail("Wege: der Stand der Mover hat sich geaendert - Durchgang " +
                             std::to_string(moverDurchgang + 1) + " mit den Movern zur Zeit");
                prepareScene();
                --moverDurchgang;
                return;
            }
            diag::info("Wege: der Stand der Mover kommt nach 9 Durchgaengen nicht zur Ruhe");
        }
    }
    // rc605 (Ende): trigger_hurt - toetet ein Feld eine Figur, laeuft der
    // Nachbau noch einmal mit diesem Tod (deathscript, Leiche, die Szene zeigt
    // den Tod), bis die Liste gleich bleibt.
    {
        static int hurtDurchgang = 0;
        std::vector<AblaufWelt::HurtTod> neu = hurtTodeBerechnen();
        for (const AblaufWelt::HurtTod& h : neu) {
            diag::detail("trigger_hurt: " + h.figur + (h.unsterblich ? " (SET_UNDYING) faellt auf 1 bei " : " stirbt bei ") +
                         std::to_string(static_cast<long>(h.ms)) + " ms (Entity " + std::to_string(h.entity) + ")");
        }
        if (g_app->hurtTodeSchluessel != hurtSchluessel || neu != g_app->hurtTode) {
            const bool vorher = g_app->hurtTodeSchluessel == hurtSchluessel && !g_app->hurtTode.empty();
            g_app->hurtTode = std::move(neu);
            g_app->hurtTodeSchluessel = hurtSchluessel;
            if ((vorher || !g_app->hurtTode.empty()) && hurtDurchgang < 3) {
                ++hurtDurchgang;
                diag::detail("trigger_hurt: " + std::to_string(g_app->hurtTode.size()) + " Tode - Durchgang mit ihnen im Nachbau");
                prepareScene();
                --hurtDurchgang;
                return;
            }
        }
    }
    // Die LAENGE der Klaenge nachtragen.
    //
    // Das Skript sagt nur, WANN ein sound anfaengt - wie lange er dauert,
    // steht in der Datei. Ohne diese Zahl blieb endMs gleich startMs, und
    // die Frage "laeuft dieser Klang gerade?" war nie mit ja zu
    // beantworten. Genau daran scheiterte der Wiedereinstieg beim
    // Einschalten des Tons: er suchte einen laufenden Klang und fand nie
    // einen, also blieb es still bis zum naechsten - beim Anwender fuenf
    // Sekunden.
    //
    // Hier und nicht beim Zeichnen: die Dateien werden dafuer gelesen, und
    // das gehoert in die Ladephase.
    for (TimelineTrack& tr2 : g_app->timeline.tracks) {
        for (TimelineEvent& e : tr2.events) {
            if (e.kind != TimelineEvent::Kind::Sound || e.sound.empty()) {
                continue;
            }
            const double len = soundLengthMs(e.sound);
            if (len > 0.0) {
                e.endMs = e.startMs + len;
            }
        }
    }
    // Auf das Ende des GANZEN Ablaufs, nicht nur der Kamerabahn (rc592).
    // Vorher sprang der Zeiger nach jeder Skriptaenderung auf das Ende der
    // Kamerafahrten zurueck - ohne Kamerabefehle auf 0 -, obwohl das Lineal
    // weiterreicht, sobald Figuren oder Mover laenger laufen (Animationstest:
    // Schluessel bei 4 s, danach stand der Zeiger auf 0).
    g_app->playMs = std::min(g_app->playMs, std::max({g_app->camTrack.durationMs, g_app->timeline.practicalEndMs,
                                                      g_app->timeline.durationMs}));
    g_app->actorAssets.clear();
    // Effekte (fx_runner) und Kartenmodelle (misc_model) gehoeren zur
    // KARTE, nicht zu den Figuren. Sie standen hinter der Abfrage unten und
    // wurden fuer ein Skript ohne Figuren nie geladen (Kartentest 27.09.).
    loadMapEffects();
    loadMapModels();
    // --- Kartenmodelle, die ein Skript bewegt ---------------------------
    //
    // Ein misc_model mit script_targetname kann per affect bewegt werden
    // (move, rotate, PLAY_ROFF - t1_rail: der Greifer "script_introClaw"
    // und die Jaeger fliegen so durch die Stadt). Die Szene rechnet die
    // Bahn; gezeichnet wurde das Modell aber an seinem festen Platz.
    g_app->modellFigur.assign(g_app->mapModels.size(), -1);
    for (std::size_t m = 0; m < g_app->mapModels.size(); ++m) {
        const std::size_t ei = g_app->mapModels[m].entity;
        if (ei >= g_app->map.entities.size()) {
            continue;
        }
        const std::string name = entitySkriptName(g_app->map.entities[ei]);
        if (name.empty()) {
            continue;
        }
        for (std::size_t a = 0; a < g_app->scene.actors.size(); ++a) {
            const std::string& an = g_app->scene.actors[a].name;
            if (an.size() == name.size() &&
                std::equal(an.begin(), an.end(), name.begin(), [](char x, char y) {
                    return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
                })) {
                g_app->modellFigur[m] = static_cast<int>(a);
                break;
            }
        }
    }
    // --- Figuren aus alten MD3-Teilen ------------------------------------
    //
    // Ein NPC-Typ mit legsmodel und ohne playerModel (remote_sp: "headmodel
    // none, torsomodel none, legsmodel remote_sp") bekommt im Spiel kein
    // Ghoul2-Modell; gezeichnet wird models/players/<legsmodel>/lower.md3
    // (CG_RegisterClientModelname, cg_main.cpp:1255), mit
    // lower_<skin>.skin, falls es sie gibt. Hier reist die Figur als
    // Kartenmodell, das ihrer Bahn folgt - wie ein bewegtes misc_model.
    for (std::size_t a = 0; a < g_app->scene.actors.size(); ++a) {
        const Actor& fig = g_app->scene.actors[a];
        if (fig.npcType.empty() || !fig.haveStart) {
            continue;
        }
        std::string typ = fig.npcType;
        for (char& c : typ) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const auto def = g_app->npcMap.find(typ);
        if (def == g_app->npcMap.end() || !def->second.playerModel.empty() || def->second.legsModel.empty()) {
            continue;
        }
        std::string pfad = "models/players/" + def->second.legsModel + "/lower.md3";
        for (char& c : pfad) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (g_app->md3Files.find(pfad) == g_app->md3Files.end()) {
            std::string data;
            Md3Model mm;
            std::string err;
            if (!readFromArchives(pfad, data) || !readMd3(data, mm, &err)) {
                diag::info("ohne Modell: " + fig.name + " - " + pfad + " fehlt oder ist nicht lesbar");
                continue;
            }
            g_app->md3Files[pfad] = std::move(mm);
        }
        const Md3Model& mm = g_app->md3Files[pfad];
        if (mm.empty()) {
            continue;
        }
        // Die Haut: "Flaeche,Shader" je Zeile (lower_default.skin).
        std::map<std::string, std::string> haut;
        {
            const std::string skin = def->second.customSkin.empty() ? "default" : def->second.customSkin;
            std::string text;
            const std::string ordner = "models/players/" + def->second.legsModel + "/";
            if (readFromArchives(ordner + "lower_" + skin + ".skin", text) ||
                readFromArchives(ordner + "lower_default.skin", text)) {
                std::istringstream is(text);
                std::string zeile;
                while (std::getline(is, zeile)) {
                    const auto komma = zeile.find(',');
                    if (komma == std::string::npos) {
                        continue;
                    }
                    std::string f = zeile.substr(0, komma);
                    std::string s = zeile.substr(komma + 1);
                    const auto rand = [](std::string& x) {
                        while (!x.empty() && std::isspace(static_cast<unsigned char>(x.back()))) { x.pop_back(); }
                        while (!x.empty() && std::isspace(static_cast<unsigned char>(x.front()))) { x.erase(0, 1); }
                        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                    };
                    rand(f);
                    rand(s);
                    if (!f.empty() && !s.empty()) {
                        haut[f] = s;
                    }
                }
            }
        }
        App::MapModel item;
        item.path = pfad;
        item.frames = mm.numFrames;
        item.entity = static_cast<std::size_t>(-1);
        item.standbild = true;
        for (int k = 0; k < 3; ++k) {
            item.origin[k] = fig.start[k];
            item.angles[k] = fig.startAngles[k];
        }
        std::vector<std::string> bildNamen;
        bildNamen.reserve(mm.surfaces.size());
        std::string blockName = "f|" + pfad;
        for (const Md3Surface& sf : mm.surfaces) {
            std::string name = sf.name;
            for (char& c : name) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            const auto h = haut.find(name);
            bildNamen.push_back((h != haut.end()) ? h->second : sf.shader);
            blockName += "|" + bildNamen.back();
        }
        item.shaderBase = modellBlock(blockName, [&bildNamen] {
            for (const std::string& bn : bildNamen) {
                TextureSet::Tex t;
                loadTextureFor(bn, t);
                g_app->textures.byShader.push_back(std::move(t));
            }
        });
        g_app->mapModels.push_back(std::move(item));
        g_app->modellFigur.push_back(static_cast<int>(a));
    }
    if (g_app->scene.actors.empty()) {
        return;
    }
    loadActorModels();
    kinoSkeletteAbgleichen();
    // rc604: misc_trip_mine noch einmal - jetzt mit den Netzen der Figuren
    // (G2_RadiusTracePolys); loadMapEffects lief oben ohne sie.
    fallenEffekteSetzen();
    // rc605 (fix605): explodierte eine Falle mit target, schalten ihre Ziele
    // (G_UseTargets in WP_Explode, g_weapon.cpp:221-224 OpenJK) Mover - noch ein
    // Durchgang mit diesen uses in der Mover-Simulation, bis sie gleich bleiben.
    {
        static int fallenDurchgang = 0;
        // rc605 (Ende): ebenso, wenn die Explosionen (G_RadiusDamage auf Figuren) andere sind.
        const bool knalleAnders = g_app->fallenKnalleKarte != g_app->map.path || g_app->fallenKnalleNeu != g_app->fallenKnalle;
        if (g_app->fallenUsesKarte != g_app->map.path || g_app->fallenUsesNeu != g_app->fallenUses || knalleAnders) {
            const bool vorher = (g_app->fallenUsesKarte == g_app->map.path && !g_app->fallenUses.empty()) ||
                                (g_app->fallenKnalleKarte == g_app->map.path && !g_app->fallenKnalle.empty());
            g_app->fallenUses = g_app->fallenUsesNeu;
            g_app->fallenUsesKarte = g_app->map.path;
            g_app->fallenKnalle = g_app->fallenKnalleNeu;
            g_app->fallenKnalleKarte = g_app->map.path;
            if ((vorher || !g_app->fallenUses.empty() || !g_app->fallenKnalle.empty()) && fallenDurchgang < 3) {
                ++fallenDurchgang;
                diag::detail("misc_trip_mine: " + std::to_string(g_app->fallenUses.size()) +
                             " Ziele explodierter Fallen - Durchgang mit ihnen im Nachbau und in der Mover-Simulation");
                prepareScene();
                --fallenDurchgang;
                return;
            }
        }
    }
    // --- UND HIER AUCH ------------------------------------------------
    //
    // Zweimal habe ich das Vorladen fuer behoben erklaert (rc307, rc309),
    // und zweimal stand es danach unveraendert im Protokoll:
    //
    //     Nachgeladen: Klingenbild "...purple_glow2" - 10.7 ms (spielt)
    //
    // Der Grund war banal: `loadActorModels()` wird an ZWEI Stellen
    // gerufen, und ich hatte das Vorladen nur an die zweite gehaengt.
    // Diese hier - beim Aufbereiten der Ansicht - laeuft zuerst; danach
    // stimmen die Groessen, und die zweite Stelle kommt nie an die Reihe.
    //
    // Die Lehre: bei einer Behebung, die nicht wirkt, ist die erste Frage
    // nicht "rechnet sie richtig", sondern "laeuft sie ueberhaupt". Ich
    // habe zweimal an der Rechnung gebessert, ohne das zu pruefen.
    preloadSaberAssets();
}

// Die Shader eines Effekts laden - EINMAL je Name.
//
// Stand bis rc568 mitten in loadMapEffects (danach in ladeEffektDatei)
// und galt damit nur fuer Effekte, die dort gelesen wurden. Folgeeffekte
// (Aufprall, Tod, Fahne, playfx) kamen spaeter ueber effectByName und
// bekamen nie ein Bild - Slot 0, die erste Textur der Karte. Seit rc569 gehen sie denselben Weg (siehe die
// Vorabladung in loadMapEffects).
void ladeEffektShader(const efx::Effect& ef) {
    for (const efx::Primitive& pr : ef.primitives) {
        if (pr.shaders.empty()) {
            continue;
        }
        // --- ALLE Texturen des Bausteins, nicht nur die erste ----
        //
        // Bis rc367 bekam nur `shaders[0]` einen Platz. Das war
        // stimmig, solange der Zeichner auch nur die erste nahm.
        //
        // Mit rc367 wuerfelt er (CMediaHandles::GetHandle,
        // FxScheduler.h:92) - und traf damit Namen, die hier nie
        // eingetragen wurden. Ohne Platz bleibt `slot` auf 0, und
        // 0 ist nicht "ohne Bild", sondern die ERSTE Textur der
        // Karte. In md_am_sith ist das die Lava.
        //
        // Auf shanks Bildschirm: riesige Lavavierecke mitten im
        // Bild, gestapelt. Ein Fehler, den ich selbst eingebaut und
        // nicht nachgemessen habe - der Effektvergleich zeigt nur
        // Bytes, und dass die Bytes sich aendern, war ja gerade der
        // Zweck der Aenderung.
        for (const std::string& shName : pr.shaders) {
        std::string low = shName;
        for (char& ch : low) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }
        if (g_app->effectShaders.count(low) != 0U) {
            continue;
        }
        TextureSet::Tex t;
        if (!loadTextureFor(shName, t)) {
            // Kein Bild: Nummer 0 heisst "ohne", dann bleibt es
            // beim farbigen Viereck wie bisher.
            g_app->effectShaders[low] = 0;
            diag::detail("  kein Bild fuer Effektshader " + shName);
            continue;
        }
        // Ein Effektpartikel DECKEND zu zeichnen ist nie richtig.
        //
        // Genau das ergab die weissen Flecken: kein blendFunc im
        // Shaderskript heisst bei einer Wand "deckend", und bei
        // einer Wand stimmt das auch. Bei einem Partikel nicht -
        // ein deckendes Viereck mit weisser Textur ist ein weisser
        // Fleck, egal wie das Bild aussieht.
        //
        // Woran unterscheiden? Am Alphakanal des Bildes:
        //
        //   hat er Werte unter voll   -> durchscheinend. Rauch und
        //                                Wolken sind so gebaut: die
        //                                Form steckt im Alpha.
        //   ist er ueberall voll      -> additiv. Leuchten sind so
        //                                gebaut: die Form steckt in
        //                                der Helligkeit, Schwarz
        //                                ist unsichtbar.
        //
        // rc605 (Rest): die Faustregel ist ersetzt durch das, was die Engine
        // tut. Das FX-System registriert seine Shader mit RE_RegisterShader
        // (SFxHelper::RegisterShader -> cgi_R_RegisterShader, code/cgame/
        // FxUtil.cpp:136-140 MD), also R_FindShader mit lightmaps2d
        // (tr_shader.cpp:3535-3536 MD):
        //  - MIT Shaderskript gilt das Skript - auch ohne blendFunc: dann
        //    zeichnet die Stufe deckend (GLS_DEFAULT), wie jede Flaeche;
        //  - OHNE Skript der implizite Shader fuer LIGHTMAP_2D (:3482-3491):
        //    rgbGen vertex, alphaGen vertex, GLS_DEPTHTEST_DISABLE |
        //    GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA, das
        //    Bild ohne Mip mit GL_CLAMP (R_FindImageFile, :3450).
        if (g_app->shaderMap.count(low) == 0U && !t.empty()) {
            t.srcFactor = BlendFactor::SrcAlpha;
            t.dstFactor = BlendFactor::OneMinusSrcAlpha;
            t.blend = BlendMode::Alpha;
            t.rgbVertex = true;
            for (float& c : t.rgbConst) { c = -1.0F; }
            t.depthFunc = DepthFunc::Disable;
            t.depthWrite = false;
            t.clamp = true;
            diag::detail("  Effektshader " + shName + " ohne Skript -> impliziter 2D-Shader (Alpha, ohne Tiefentest)");
        }
        // --- Der Alphawert der ECKE gilt hier ausdruecklich ----
        //
        // Ein Effekt legt seine Blende in den Eckenalphawert
        // (`useAlpha`, 278 Vorkommen, FxPrimitives.cpp:599).
        //
        // Bei der KARTE ist die Vorgabe der Engine eine andere:
        // ohne alphaGen gilt dort AGEN_IDENTITY, die Ecke zaehlt
        // nicht. behaved nimmt bisher fuer beides den Eckenwert -
        // gemessen betrifft der Unterschied in den 43 Karten null
        // Flaechen, aber richtig ist es nicht.
        //
        // Damit die Vorgabe spaeter in EINER Zeile umgestellt
        // werden kann, ohne die Effekte mitzureissen, tragen die
        // Effekttexturen ihr `Vertex` hier ausdruecklich. Heute
        // aendert das nichts; es ist die Vorarbeit, die den
        // spaeteren Schritt ungefaehrlich macht.
        const AlphaGen alphaAusSkript = t.alphaGen;
        t.alphaGen = AlphaGen::Vertex;
        // rc604: `alphaGen entity` der Grundstufe ist der Alphawert des
        // Teilchens (RB_CalcAlphaFromEntity) - sonst bliebe die feste 1 aus
        // alphaConst stehen und die Blende ginge verloren.
        // rc604: und ueberhaupt gilt fuer die Grundstufe eines GESKRIPTETEN
        // Effektshaders ComputeColors wie fuer jede weitere Stufe
        // (RB_IterateStagesGeneric, tr_shade.cpp): ohne rgbGen identity bzw.
        // identityLighting (ParseStage, tr_shader.cpp:1713-1722 MD) - die Farbe
        // des Teilchens zaehlt dann nicht -, ohne alphaGen AGEN_IDENTITY (rgbGen
        // vertex setzt AGEN_VERTEX, :1471-1481) - siehe effektStufeFarbe. Ohne
        // Skript bleibt die Eckenfarbe (die Faustregel oben).
        if (const auto si0 = g_app->shaderMap.find(low); si0 != g_app->shaderMap.end()) {
            const ShaderInfo::Stage* erste = nullptr;
            for (const ShaderInfo::Stage& st : si0->second.stages) {
                if (!st.lightmap && !st.image.empty()) {
                    erste = &st;
                    break;
                }
            }
            if (erste != nullptr) {
                t.alphaGen = alphaAusSkript;
                effektStufeFarbe(*erste, t);
            } else {
                effektAlphaEntity(si0->second.alphaEntity, t);
            }
        }
        // --- rc604 fix604b (B1): ALLE Stufen des Effektshaders ---------------
        //
        // Ein Teilchen ist im Spiel eine Flaeche wie jede andere: RB_StageIterator-
        // Generic zeichnet JEDE Stufe seines Shaders (RB_IterateStagesGeneric,
        // code/rd-vanilla/tr_shade.cpp:1535 ff. OpenJK), je Stufe mit eigenem
        // blendFunc, ComputeColors, ComputeTexCoords (tcMod) und R_BindAnimatedImage
        // (animMap nach tess.shaderTime). Bis hier zeichnete behaved nur die erste
        // Stufe - gfx/exp/explosion1 (21 Karten) ist zwei ueberblendete
        // oneshotanimmap-Stufen (rgbGen wave inversesawtooth/sawtooth), gezeigt
        // wurde das erste Bild der ersten. Das Effektnetz geht durch dieselbe
        // zeichenliste wie die Karte - die Zusatzstufen zeichnet sie schon.
        const int basisPlatz = static_cast<int>(g_app->textures.byShader.size());
        g_app->textures.byShader.emplace_back();   // Platz der Grundstufe
        if (const auto si = g_app->shaderMap.find(low); si != g_app->shaderMap.end()) {
            basisBildfolgeLaden(si->second, basisPlatz, shName, t);
            bool ersteEchte = true;
            for (const ShaderInfo::Stage& st : si->second.stages) {
                if (st.lightmap || st.image.empty()) { continue; }
                if (ersteEchte) {
                    ersteEchte = false;
                    continue;   // die steckt schon in `t`
                }
                // rc604: eine surfaceSprites-Stufe ueberspringt auch das
                // Teilchen (tr_shade.cpp:1888 MD) - Halme darauf zeichnet die
                // Vorschau nicht (kartenpruefung: "effekt: surfacesprites").
                if (st.ss.aktiv()) { continue; }
                const std::size_t platzVorher = g_app->textures.byShader.size();
                zusatzstufeLaden(si->second, st, shName, t);
                for (std::size_t k = platzVorher; k < g_app->textures.byShader.size(); ++k) {
                    effektStufeFarbe(st, g_app->textures.byShader[k]);
                }
            }
        }
        g_app->effectShaders[low] = basisPlatz;
        g_app->textures.byShader[static_cast<std::size_t>(basisPlatz)] = std::move(t);
        }   // Ende: alle Texturen dieses Bausteins
    }
}

// rc604 fix604b (B1): Farbe und Alpha einer weiteren Stufe eines
// Effektshaders. Die Ecken eines Teilchens tragen shaderRGBA (RB_AddQuadStamp
// aus RB_SurfaceSprite, code/rd-vanilla/tr_surface.cpp OpenJK); nur rgbGen
// vertex/exactVertex/entity und alphaGen vertex lesen sie (ComputeColors,
// tr_shade.cpp). Ohne rgbGen/alphaGen gilt CGEN_IDENTITY bzw. AGEN_IDENTITY
// (ParseStage, tr_shader.cpp) - weiss, Alpha 1, das Teilchen faerbt die Stufe
// dann nicht. (Der Effektweg zeichnet die Ecken roh, siehe lichtFuer - darum
// hier die feste Farbe statt der Ecke.) rc604: alphaGen entity liest
// shaderRGBA[3] des Teilchens (RB_CalcAlphaFromEntity) - effektAlphaEntity.
void effektStufeFarbe(const ShaderInfo::Stage& st, TextureSet::Tex& z) {
    switch (st.rgbArt) {
        case RgbGenArt::Keins:
        case RgbGenArt::Identity:
        case RgbGenArt::IdentityLighting:
            if (!z.rgbWave.active()) {
                for (float& c : z.rgbConst) { c = 1.0F; }
            }
            z.rgbVertex = false;
            break;
        case RgbGenArt::Entity:
            // shaderRGBA des Teilchens = seine Eckenfarbe.
            for (float& c : z.rgbConst) { c = -1.0F; }
            z.rgbVertex = true;
            break;
        default:
            break;
    }
    if (z.alphaGen == AlphaGen::FromImage && z.alphaConst < 0.0F && !z.alphaWave.active()) {
        // rc604: rgbGen vertex ohne alphaGen setzt AGEN_VERTEX (ParseStage,
        // tr_shader.cpp:1471-1481 MD: "if (stage->alphaGen == 0) stage->alphaGen
        // = AGEN_VERTEX"), sonst bleibt AGEN_IDENTITY.
        z.alphaGen = st.rgbArt == RgbGenArt::Vertex ? AlphaGen::Vertex : AlphaGen::Identity;
    }
    // rc604: alphaGen entity = shaderRGBA[3] des Teilchens = sein Eckenalpha.
    effektAlphaEntity(st.alphaEntity, z);
}

// Eine Effektdatei EINMAL lesen und ihre Shader eintragen.
//
// Herausgezogen aus loadMapEffects, weil nicht nur fx_runner Effekte
// brauchen: auch was zerbricht (CG_MiscModelExplosion, MoverSim::
// bruchEffekte) spielt "chunks/metalexplode" & Co. - und das ohne Shader
// ueber effectByName zu laden, gaebe Partikel ohne Bild (Platz 0 ist die
// erste Textur der Karte, siehe efxdraw.cpp).
//
// `herkunft` steht nur im Protokoll (was in der Karte stand). false: die
// Datei fehlt oder ist nicht lesbar - schon gemeldet.
bool ladeEffektDatei(const std::string& path, const std::string& herkunft) {
    if (g_app->effects.find(path) != g_app->effects.end()) {
        return true;
    }
    {
        std::string text;
        if (!readFromArchives(path, text)) {
            diag::info("keine Effektdatei: " + path);
            diag::detail("Effekt " + path + "  (fxFile stand als \"" +
                         herkunft + "\")");
            // Dieselbe Frage wie bei den Texturen: liegt die Datei
            // woanders, oder gibt es sie nirgends?
            reportMissingTexture(path);
            return false;
        }
        const efx::ReadResult r = efx::read(text);
        // Die Shader der Primitive laden - EINMAL je Name.
        //
        // Damit bekommen Partikel endlich ihr Bild. Sie gehen denselben
        // Weg wie die Flaechen der Karte und die .md3-Modelle
        // (loadTextureFor), also auch mit blendFunc: fast alle
        // Effektshader sind additiv, und erst dadurch werden aus
        // schwarzen Kaesten weiche Wolken.
        ladeEffektShader(r.effect);
        if (diag::detailOn()) {
            diag::detail("Effekt " + path);
            for (const efx::Diagnostic& d : r.diagnostics) {
                diag::detail("  " +
                             std::string(d.severity == efx::Severity::Error
                                             ? "Fehler: "
                                             : "Anmerkung: ") +
                             d.message);
            }
            for (const efx::Primitive& pr : r.effect.primitives) {
                // Was zeichnen wir davon wirklich - und womit?
                const bool zeichenbar =
                    pr.type == efx::PrimitiveType::Particle ||
                    pr.type == efx::PrimitiveType::OrientedParticle ||
                    pr.type == efx::PrimitiveType::Tail ||
                    pr.type == efx::PrimitiveType::Line ||
                    pr.type == efx::PrimitiveType::Electricity ||
                    pr.type == efx::PrimitiveType::Cylinder ||
                    pr.type == efx::PrimitiveType::Decal ||
                    pr.type == efx::PrimitiveType::ScreenFlash ||
                    (pr.type == efx::PrimitiveType::Emitter &&
                     (pr.flags & efx::kFlagAttachedModel) != 0U);
                std::string zeile = std::string("  ") +
                                    efx::typeName(pr.type) +
                                    (zeichenbar ? "  gezeichnet"
                                                : "  NICHT gezeichnet");
                if (!pr.shaders.empty()) {
                    zeile += "  Shader: " + pr.shaders[0];
                } else {
                    zeile += "  ohne Shader";
                }
                if (!pr.alpha.present) { zeile += "  (kein alpha-Block)"; }
                if (!pr.rgb.present) { zeile += "  (kein rgb-Block)"; }
                if (!pr.size.present) { zeile += "  (kein size-Block)"; }
                diag::detail(zeile);
            }
        }
        if (r.hasErrors()) {
            std::string why = "unlesbar";
            for (const efx::Diagnostic& d : r.diagnostics) {
                if (d.severity == efx::Severity::Error) { why = d.message; }
            }
            diag::info("Effekt nicht lesbar: " + path + " - " + why);
            return false;
        }
        g_app->effects[path] = r.effect;
    }
    return true;
}

#include "app_view3d_spiel.inc"   // Stufe 8d: das Spiel in der Vorschau - gi.trace, Laserfallen, Geschuetze, Gegenstaende, Klingen
#include "app_view3d_bedienung.inc"   // Stufe 8d: Bedienung der Vorschau - Entity-Gruppen, Platzieren, Ansichten, Zeitleiste, Spielfenster
// --- Die Ansicht einstellen: Kopfzeile statt Seitenleiste -------------------
//
// Bis rc585 stand rechts neben dem Bild eine Leiste mit Reglern, Haekchen und
// Zaehlern. Sie nahm dem Bild ein Viertel der Breite, und die Haelfte davon
// waren Werte fuer Entwickler ("37 Aufrufe"). shank (11.10.): "moderner und
// intuitiv". Vorbild ist Unreals Viewport: oben eine schmale Leiste mit
// Umschaltern als Icons, Einstellungen in einem Klappfenster, Zahlen in einem
// eigenen Statistikfenster.

// Netz neu bauen, wenn Himmel oder Detail sich aendern.
void kartenNetzNeu() {
    g_app->mapBereit = false;
    g_app->mesh = g_app->geo.models.empty()
                      ? buildMesh(g_app->geo, g_app->meshDetail, !g_app->showSky)
                      : buildModelMesh(g_app->geo, 0, g_app->meshDetail, !g_app->showSky);
    g_app->mapBereit = !g_app->mesh.batches.empty();
    g_app->brushMeshes.clear();
    gpu::vergissMoverNetze();   // die Puffer hingen an den alten Netzen
    g_app->mapDirty = true;
}

// Im Klappfenster der Ansicht: Licht, Detail, Schatten, Figuren.
void ansichtEinstellungen() {
    ImGui::BeginDisabled(g_app->geo.empty());
    const float feld = ImGui::GetFontSize() * 8.0F;
    ImGui::SetNextItemWidth(feld);
    if (ImGui::SliderFloat(tr(Str::MapBrightness), &g_app->mapBrightness, 0.5F, 6.0F, "%.1fx")) {
        g_app->mapDirty = true;
    }
    ImGui::SetNextItemWidth(feld);
    if (ImGui::SliderFloat(tr(Str::MapMinLight), &g_app->mapMinLight, 0.0F, 0.5F, "%.2f")) {
        g_app->mapDirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tr(Str::MapMinLightHint));
    }
    ImGui::SetNextItemWidth(feld);
    if (ImGui::SliderInt(tr(Str::MapDetail), &g_app->meshDetail, 1, 10)) {
        // Die Welt ist Untermodell 0 - OHNE die beweglichen Teile (die zeichnet
        // die Engine an ihrer Entity, R_LoadSubmodels).
        kartenNetzNeu();
    }
    {
        const char* arten[4] = {tr(Str::MapShadowOff), tr(Str::MapShadowBlob), tr(Str::MapShadowStencil),
                                tr(Str::MapShadowPlanar)};
        const int art = std::clamp(g_app->schattenArt, 0, 3);
        ImGui::SetNextItemWidth(feld);
        if (ImGui::BeginCombo(tr(Str::MapShadows), arten[art])) {
            for (int i = 0; i < 4; ++i) {
                if (ImGui::Selectable(arten[i], i == art)) {
                    g_app->schattenArt = i;
                    g_app->mapDirty = true;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tr(Str::MapShadowsHint));
        }
    }
    if (ImGui::Checkbox(tr(Str::MapScriptFocus), &g_app->skriptFokus)) { g_app->mapDirty = true; }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MapScriptFocusHint)); }
    if (g_app->showActors) {
        // Figuren um 90 Grad drehen - fuer Modelle, die anders herum stehen.
        char lbl[64];
        std::snprintf(lbl, sizeof(lbl), tr(Str::ActorYaw), g_app->actorYaw);
        if (ImGui::SmallButton(lbl)) {
            g_app->actorYaw = (g_app->actorYaw + 90) % 360;
            g_app->mapDirty = true;
        }
    }
    ImGui::EndDisabled();
    // Paket kamera, rc601: das Spielfenster (auch ohne Karte einstellbar).
    spielFensterEinstellungen();
}

// Die Umschalter der Ansicht (rechts in der Kopfzeile). Mit nurMessen nur die
// Breite, ohne zu zeichnen.
float ansichtsSchalter(bool nurMessen) {
    struct Schalter {
        Str text;
        Str hilfe;
        UiIcon icon;
        bool* wert;
    };
    const Schalter schalter[] = {
        {Str::MapEntities, Str::MapEntitiesHint, UiIcon::Entity, &g_app->showEntities},
        {Str::MapNames, Str::MapNamesHint, UiIcon::Namen, &g_app->zeigeNamen},
        {Str::MapActors, Str::MapActorsHint, UiIcon::Figur, &g_app->showActors},
        {Str::MapEffects, Str::MapEffectsHint, UiIcon::Effekt, &g_app->showEffects},
        {Str::MapGlow, Str::MapGlowHint, UiIcon::Leuchten, &g_app->showGlow},
        {Str::MapDynLights, Str::MapDynLightsHint, UiIcon::Licht, &g_app->showDynLights},
        {Str::MapFog, Str::MapFogHint, UiIcon::Nebel, &g_app->showFog},
        {Str::MapSky, Str::MapSkyHint, UiIcon::Himmel, &g_app->showSky},
    };
    const float h = ImGui::GetFrameHeight();
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    const int anzahl = static_cast<int>(sizeof(schalter) / sizeof(schalter[0]));
    // Acht Schalter, dann Kamera, Draufsicht, Wegpunkte (rc589, je mit vollem
    // Abstand), Einstellungen, Statistik. Fehlt hier ein Knopf, laeuft die
    // Leiste rechts ueber den Rand (Wegpunkttest: "nicht ganz sichtbar").
    const float breite = static_cast<float>(anzahl) * h + static_cast<float>(anzahl - 1) * sp * 0.5F +
                         sp + 3.0F * h + 2.0F * sp * 0.5F + 3.0F * (h + sp);
    if (nurMessen) {
        return breite;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{sp * 0.5F, ImGui::GetStyle().ItemSpacing.y});
    for (int i = 0; i < anzahl; ++i) {
        const Schalter& s = schalter[i];
        if (i > 0) { ImGui::SameLine(); }
        if (iconKnopf(tr(s.text), s.icon, tr(s.hilfe), *s.wert, true, true)) {
            *s.wert = !*s.wert;
            if (s.wert == &g_app->showSky) {
                kartenNetzNeu();
            }
            g_app->mapDirty = true;
        }
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.0F, sp);
    // Durch die Kamera schauen - der wichtigste Schalter fuer Zwischensequenzen.
    {
        char hilfe[400];
        if (g_app->throughCamera && g_app->camViewActive) {
            char info[200];
            std::snprintf(info, sizeof(info), tr(Str::MapCamInfo), g_app->camViewLabel.c_str(),
                          static_cast<double>(g_app->camViewFov));
            std::snprintf(hilfe, sizeof(hilfe), "%s\n%s", tr(Str::MapThroughCamHint), info);
        } else {
            std::snprintf(hilfe, sizeof(hilfe), "%s", tr(Str::MapThroughCamHint));
        }
        if (iconKnopf(tr(Str::MapThroughCam), UiIcon::Kamera, hilfe, g_app->throughCamera, true, true)) {
            g_app->throughCamera = !g_app->throughCamera;
            g_app->mapDirty = true;
        }
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::MapTopView), UiIcon::Oben, tr(Str::MapTopViewHint), g_app->draufsicht, true, true)) {
        draufsichtSchalten(!g_app->draufsicht);
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::WegTitel), UiIcon::Pfad, tr(Str::WegZeigenHint), g_app->wegFensterOffen, true, true)) {
        g_app->wegFensterOffen = !g_app->wegFensterOffen;
        if (g_app->wegFensterOffen) { g_app->zeigeWegnetz = true; }
        g_app->mapDirty = true;
    }
    // Raster (rc590): fuer Ziehen und Setzen von Entities.
    ImGui::SameLine();
    {
        char hilfe[300];
        std::snprintf(hilfe, sizeof(hilfe), "%s\n%s", tr(Str::GridHint),
                      g_app->rasterSchritt > 0 ? std::to_string(g_app->rasterSchritt).c_str() : tr(Str::GridOff));
        if (iconKnopf(tr(Str::Grid), UiIcon::Raster, hilfe, g_app->rasterSchritt > 0, true, true)) {
            ImGui::OpenPopup("##raster");
        }
        if (ImGui::BeginPopup("##raster")) {
            if (ImGui::MenuItem(tr(Str::GridOff), nullptr, g_app->rasterSchritt == 0)) { g_app->rasterSchritt = 0; }
            for (const int r : {8, 16, 32, 64}) {
                if (ImGui::MenuItem(std::to_string(r).c_str(), nullptr, g_app->rasterSchritt == r)) { g_app->rasterSchritt = r; }
            }
            ImGui::EndPopup();
        }
    }
    ImGui::SameLine(0.0F, sp * 0.5F);
    if (iconKnopf(tr(Str::MapViewSettings), UiIcon::Einstellungen, tr(Str::MapViewSettingsHint)) ||
        g_app->ansichtEinstellungenOeffnen) {
        g_app->ansichtEinstellungenOeffnen = false;
        ImGui::OpenPopup("##ansichteinstellungen");
    }
    if (ImGui::BeginPopup("##ansichteinstellungen")) {
        ansichtEinstellungen();
        ImGui::EndPopup();
    }
    ImGui::SameLine(0.0F, sp * 0.5F);
    if (iconKnopf(tr(Str::MapStats), UiIcon::Statistik, tr(Str::MapStatsHint), g_app->kartenStatistikOffen, true, true)) {
        g_app->kartenStatistikOffen = !g_app->kartenStatistikOffen;
    }
    return breite;
}

// Die Zahlen fuer Entwickler und Fehlersuche - eigenes Fenster.
void drawKartenStatistik() {
    if (!g_app->kartenStatistikOffen) {
        return;
    }
    if (!ImGui::Begin((std::string(tr(Str::MapStats)) + "###kartenstatistik").c_str(), &g_app->kartenStatistikOffen,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    // Wo die freie Kamera steht, und wie man fliegt - bis rc587 in einer
    // eigenen Zeile ueber der Zeitleiste.
    ImGui::Text(tr(Str::MapPos), static_cast<int>(g_app->cam.pos[0]), static_cast<int>(g_app->cam.pos[1]),
                static_cast<int>(g_app->cam.pos[2]));
    ImGui::TextDisabled("%s", tr(Str::MapFly));
    // Bildrate: die Zeit fuers Zeichnen der Karte, nicht des ganzen Fensters.
    if (g_app->mapFrameMs > 0.0F) {
        ImGui::Text(tr(Str::MapFps), 1000.0F / g_app->mapFrameMs);
        ImGui::SameLine();
        ImGui::TextDisabled(tr(Str::MapFrameMs), static_cast<double>(g_app->mapFrameMs));
    }
    if (g_app->textures.found + g_app->textures.missing != 0) {
        ImGui::Text(tr(Str::MapTexFound), g_app->textures.found, g_app->textures.found + g_app->textures.missing);
    } else if (g_app->gamePaths.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MapTexNone));
    }
    ImGui::Text(tr(Str::MapTriangles), static_cast<int>(g_app->mesh.indexes.size() / 3),
                static_cast<int>(g_app->mesh.batches.size()));
    if (g_app->showActors && !g_app->scene.actors.empty()) {
        int withModel = 0;
        for (const auto& zeiger : g_app->actorAssets) {
            const App::ActorAssets& a = *zeiger;
            if (!a.model.empty()) { ++withModel; }
        }
        if (withModel == 0) {
            ImGui::TextDisabled("%s", tr(Str::ActorsNoModels));
        } else {
            ImGui::Text(tr(Str::ActorsFound), withModel, static_cast<int>(g_app->scene.actors.size()));
        }
    }
    if (g_app->showGlow) { ImGui::TextDisabled("%s: %d", tr(Str::MapGlow), g_app->gpuGluehen); }
    if (g_app->schattenArt != 0) { ImGui::TextDisabled("%s: %d", tr(Str::MapShadows), g_app->schattenAnzahl); }
    if (g_app->showFog) { ImGui::TextDisabled("%s: %d", tr(Str::MapFog), static_cast<int>(g_app->geo.nebel.size())); }
    if (g_app->flareZahl > 0) { ImGui::TextDisabled("%s: %d", tr(Str::MapFlares), g_app->flareZahl); }
    if (g_app->wetterTeilchenZahl > 0) { ImGui::TextDisabled("%s: %d", tr(Str::MapWeather), g_app->wetterTeilchenZahl); }
    if (g_app->showDynLights) { ImGui::TextDisabled("%s: %d", tr(Str::MapDynLights), g_app->lichtAufrufe); }
    if (g_app->throughCamera) {
        if (g_app->camViewActive) {
            ImGui::TextDisabled(tr(Str::MapCamInfo), g_app->camViewLabel.c_str(),
                                static_cast<double>(g_app->camViewFov));
        } else {
            ImGui::TextDisabled("%s", tr(Str::MapCamNone));
        }
    }
    ImGui::Separator();
    ImGui::TextDisabled("GPU: %d Aufrufe, %d uebersprungen", g_app->gpuAufrufe, g_app->gpuUebersprungen);
    ImGui::TextDisabled("%s", gpu::zustandsLage());
    const struct {
        const char* name;
        int aufrufe;
        gpu::Abschnitt a;
        gpu::Abschnitt b;
    } zeilen[] = {
        {"Karte", g_app->gpuKarte, gpu::Abschnitt::KarteDeckend, gpu::Abschnitt::KarteGemischt},
        {"Effekte", g_app->gpuEffekte, gpu::Abschnitt::EffekteDeckend, gpu::Abschnitt::EffekteGemischt},
        {"Mover", g_app->gpuMover, gpu::Abschnitt::MoverDeckend, gpu::Abschnitt::MoverGemischt},
        {"Figuren", g_app->gpuFiguren2, gpu::Abschnitt::Figuren, gpu::Abschnitt::Figuren},
        {"Gluehen", g_app->gpuGluehen, gpu::Abschnitt::Gluehen, gpu::Abschnitt::Gluehen},
    };
    for (const auto& z : zeilen) {
        const float ms = (z.a == z.b) ? gpu::millisekunden(z.a) : gpu::millisekundenBeide(z.a, z.b);
        if (ms >= 0.0F) {
            ImGui::TextDisabled("  %-8s %5d Aufrufe  %6.2f ms", z.name, z.aufrufe, static_cast<double>(ms));
        } else {
            ImGui::TextDisabled("  %-8s %5d Aufrufe", z.name, z.aufrufe);
        }
    }
    if (!g_app->gpuFehler.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
        ImGui::TextDisabled("GPU: %s", g_app->gpuFehler.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
}


}  // namespace bhed::gui
