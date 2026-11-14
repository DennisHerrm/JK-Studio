// kartecwpruef.cpp - Karten-Editor, Optimierer: die Pruefungen der Clone-Wars-Kette (CW-PRUEF, src/karte/optimierer_cw.cpp).
//
// Je Pruefung eine Testkarte, auf der sie anschlaegt, und eine, auf der sie schweigt - als Momentaufnahme (KartenAbbild) mit
// echten Ebenen und Wicklungen. Dazu die Inhalte nach der surfaceparm-Tafel von q3map2 (shaderSkriptLesen), die Wahl des
// Inhalts-Shaders eines Pinsels (SetBrushContents), die Masse der Spielfigur je Regelwerk und der Kopf von .tga/.png/.jpg.
//
// Aufruf: kartecwpruef [<.map> [<.bsp>]]  - mit einer Karte: die Befunde der neuen Pruefungen und ihre Zeiten (nur lesen)
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
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

AbbildEntitaet ent(const std::string& klasse, std::vector<std::pair<std::string, std::string>> mehr = {}) {
    AbbildEntitaet e;
    e.klasse = klasse;
    e.schluessel.emplace_back("classname", klasse);
    for (auto& kv : mehr) { e.schluessel.push_back(std::move(kv)); }
    return e;
}

// Ein Quader mit echten Ebenen (Normalen nach aussen) und Wicklungen; oben/unten/seite: die Shader
AbbildPrimitiv kasten(double x0, double y0, double z0, double x1, double y1, double z1, const std::string& seite, const std::string& oben = "",
                      const std::string& unten = "") {
    AbbildPrimitiv p;
    p.mins = Vektor3f(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(z0));
    p.maxs = Vektor3f(static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(z1));
    const auto flaeche = [&](const Vektor3d& n, double d, std::vector<Vektor3d> pts, const std::string& sh) {
        AbbildFlaeche f;
        f.shader = sh.empty() ? seite : sh;
        f.ebene = Ebene3d(n, d);
        f.punkte = std::move(pts);
        p.flaechen.push_back(f);
    };
    using V = Vektor3d;
    flaeche(V(0, 0, 1), z1, {V(x0, y0, z1), V(x1, y0, z1), V(x1, y1, z1), V(x0, y1, z1)}, oben);
    flaeche(V(0, 0, -1), -z0, {V(x0, y0, z0), V(x0, y1, z0), V(x1, y1, z0), V(x1, y0, z0)}, unten);
    flaeche(V(1, 0, 0), x1, {V(x1, y0, z0), V(x1, y1, z0), V(x1, y1, z1), V(x1, y0, z1)}, "");
    flaeche(V(-1, 0, 0), -x0, {V(x0, y0, z0), V(x0, y0, z1), V(x0, y1, z1), V(x0, y1, z0)}, "");
    flaeche(V(0, 1, 0), y1, {V(x0, y1, z0), V(x0, y1, z1), V(x1, y1, z1), V(x1, y1, z0)}, "");
    flaeche(V(0, -1, 0), -y0, {V(x0, y0, z0), V(x1, y0, z0), V(x1, y0, z1), V(x0, y0, z1)}, "");
    return p;
}

const std::string kFest = "textures/test/fest";         // gezeichnet, fest (ohne Skript: "default")
const std::string kNichtFest = "textures/test/luft";    // gezeichnet, surfaceparm nonsolid
const std::string kPlatte = "textures/test/platte";     // gezeichnet, fest, ein anderes Bild als kFest
const std::string kClip = "textures/common/clip";       // nach dem Namen: playerclip, nicht gezeichnet
const std::string kCaulk = "textures/common/caulk";
const std::string kTrigger = "textures/common/trigger";

PruefOptionen optionen() {
    PruefOptionen o;
    o.spiel = GrenzSpiel::JkaSp;
    ShaderArt luft;
    luft.cSolid = false;
    luft.inhalt = 0x8000;   // nonsolid: SOLID weg
    o.shader[kNichtFest] = luft;
    return o;
}

std::vector<Befund> pruefen(const KartenAbbild& a, PruefOptionen o) { return pruefeKarte(a, o); }
long long anzahl(const std::vector<Befund>& b, const std::string& id) {
    for (const Befund& x : b) {
        if (x.pruefung == id) { return x.anzahl; }
    }
    return 0;
}
const Befund* befund(const std::vector<Befund>& b, const std::string& id) {
    for (const Befund& x : b) {
        if (x.pruefung == id) { return &x; }
    }
    return nullptr;
}

KartenAbbild welt(std::vector<AbbildPrimitiv> prims) {
    KartenAbbild a;
    AbbildEntitaet w = ent("worldspawn");
    w.prims = std::move(prims);
    a.ents.push_back(w);
    return a;
}

void testeShaderInhalte() {
    const auto inhalt = [](const std::string& text) { return shaderSkriptLesen("textures/x/y\n{\n" + text + "\n}\n", "textures/x/y"); };
    {
        const ShaderSkript s = inhalt("qer_editorimage textures/x/y.tga");
        pruefe(s.inhalt == (0x1 | 0x8000) && s.cSolid && !s.cFluessig, "Inhalte: ohne surfaceparm \"default\" SOLID | OPAQUE, C_SOLID (games.cpp:794)");
    }
    {
        const ShaderSkript s = inhalt("surfaceparm nodraw\nsurfaceparm playerclip");
        pruefe(s.inhalt == 0x10 && !s.cSolid, "Inhalte: playerclip loescht SOLID | OPAQUE und setzt PLAYERCLIP (games.cpp:827)");
        ShaderArt a;
        a.inhalt = s.inhalt;
        pruefe(spielerKollision(a), "playerclip hindert die Spielfigur (MASK_PLAYERSOLID)");
    }
    {
        const ShaderSkript s = inhalt("surfaceparm nonsolid\nsurfaceparm trans");
        ShaderArt a;
        a.inhalt = s.inhalt;
        pruefe(!s.cSolid && !spielerKollision(a), "Inhalte: nonsolid loescht SOLID, die Spielfigur geht hindurch (games.cpp:816)");
    }
    {
        const ShaderSkript s = inhalt("surfaceparm water\nsurfaceparm trans");
        pruefe(s.cFluessig && (s.inhalt & 0x4) != 0 && (s.inhalt & 0x1) == 0, "Inhalte: water ist C_LIQUID, nicht SOLID (games.cpp:821)");
    }
    {
        const ShaderSkript s = inhalt("surfaceparm terrain");
        ShaderArt a;
        a.inhalt = s.inhalt;
        pruefe(spielerKollision(a) && !s.cSolid, "Inhalte: terrain ist CONTENTS_TERRAIN (in MASK_PLAYERSOLID), nicht C_SOLID");
    }
    {
        const ShaderSkript s = inhalt("polygonOffset\nq3map_clipModel\n{\nmap textures/x/a.tga\n}\n{\nanimMap 5 textures/x/b1 textures/x/b2\n}\n{\nmap $lightmap\n}");
        const bool bilder = s.bilder.size() == 3 && s.bilder[0] == "textures/x/a.tga" && s.bilder[1] == "textures/x/b1" && s.bilder[2] == "textures/x/b2";
        pruefe(s.versatz && s.clipModell && bilder, "Shader: polygonOffset, q3map_clipModel und die Bilder jeder Stufe ohne $lightmap");
    }
    {
        const ShaderSkript s = inhalt("skyParms env/himmel 512 -");
        pruefe(s.bilder.size() == 6 && s.bilder.front() == "env/himmel_rt" && s.bilder.back() == "env/himmel_dn",
               "Shader: skyParms <farbox> ergibt die sechs Bilder _rt _lf _bk _ft _up _dn (tr_shader.cpp ParseSkyParms)");
    }
}

void testeSpielfigur() {
    const Spielfigur sp = spielfigur(GrenzSpiel::JkaSp);
    const Spielfigur mp = spielfigur(GrenzSpiel::JkaMp);
    pruefe(sp.halb == 16 && mp.halb == 15, "Spielfigur: SP 16 (g_client.cpp:55-58), MP 15 (codemp g_client.c:31)");
    pruefe(sp.hoehe() == 64 && sp.hoeheGeduckt() == 40 && sp.stufe == 18, "Spielfigur: 64 hoch (-24 ... 40), geduckt 40, STEPSIZE 18");
    pruefe(sp.sprung > 31.6 && sp.sprung < 31.7, "Sprunghoehe ohne Macht: 225^2 / (2 * 800) = 31.6");
}

void testeBildKopf() {
    int b = 0;
    int h = 0;
    std::string tga(18, '\0');
    tga[2] = 2;
    tga[12] = static_cast<char>(100);
    tga[14] = 64;
    pruefe(bildKopfMass(tga, b, h) && b == 100 && h == 64, "Bildkopf: TGA 100 x 64");
    std::string png = std::string("\x89PNG\r\n\x1a\n", 8) + std::string("\0\0\0\x0dIHDR", 8) + std::string("\0\0\x01\0\0\0\0\x80", 8);
    pruefe(bildKopfMass(png, b, h) && b == 256 && h == 128, "Bildkopf: PNG 256 x 128");
    std::string jpg = std::string("\xFF\xD8\xFF\xE0\0\x04\0\0\xFF\xC0\0\x11\x08\x01\x00\x02\x00", 17) + std::string(8, '\0');
    pruefe(bildKopfMass(jpg, b, h) && b == 512 && h == 256, "Bildkopf: JPEG 512 x 256 (SOF0)");
    pruefe(!bildKopfMass("kein Bild", b, h), "Bildkopf: kein Bild");
}

void testeSichtGegenKollision() {
    // a) sichtbarer Boden ohne Kollision darunter
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kNichtFest)}), optionen());
        pruefe(anzahl(b, "laufohnekollision") == 1, "laufohnekollision: Boden mit surfaceparm nonsolid, darunter nichts (rot)");
        const Befund* x = befund(b, "laufohnekollision");
        pruefe(x != nullptr && !x->objekte.empty() && x->objekte[0].hatPunkt && x->objekte[0].flaeche == 0, "laufohnekollision: Objekt mit Flaeche und Messpunkt");
        const auto g = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest)}), optionen());
        pruefe(anzahl(g, "laufohnekollision") == 0 && anzahl(g, "kollisionsabstand") == 0, "laufohnekollision: fester Boden (schweigt)");
    }
    // SetBrushContents: eine trigger-Seite macht den Pinsel in der BSP nicht fest (der nicht feste Shader gewinnt vor dem festen)
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kCaulk, kFest, kTrigger)}), optionen());
        pruefe(anzahl(b, "laufohnekollision") == 1, "SetBrushContents: Pinsel mit einer trigger-Seite traegt die Inhalte von trigger - kein Boden (rot)");
        const auto c = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kCaulk, kFest, kClip)}), optionen());
        pruefe(anzahl(c, "laufohnekollision") == 0, "SetBrushContents: Pinsel mit einer clip-Seite traegt playerclip - fest (schweigt)");
    }
    // b) Abstand: sichtbare Platte 4 ueber der Kollision / unsichtbarer Clip 8 auf dem sichtbaren Boden
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(0, 0, 0, 128, 256, 4, kNichtFest)}), optionen());
        const Befund* x = befund(b, "kollisionsabstand");
        pruefe(x != nullptr && x->anzahl == 1 && x->detail.find("largest gap 4") != std::string::npos, "kollisionsabstand: Platte 4 Einheiten ueber der Kollision (rot)");
        const auto c = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(0, 0, 0, 128, 256, 8, kClip)}), optionen());
        const Befund* y = befund(c, "kollisionsabstand");
        pruefe(y != nullptr && y->detail.find("largest gap 8") != std::string::npos, "kollisionsabstand: unsichtbarer Clip 8 hoch auf dem Boden (rot)");
        const auto g = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(0, 0, 0, 128, 256, 0.5, kNichtFest)}), optionen());
        pruefe(anzahl(g, "kollisionsabstand") == 0, "kollisionsabstand: Platte 0,5 ueber der Kollision (schweigt, Toleranz 1)");
        PruefOptionen streng = optionen();
        streng.masse.toleranz = 0.25;
        pruefe(anzahl(pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(0, 0, 0, 128, 256, 0.5, kNichtFest)}), streng), "kollisionsabstand") == 1,
               "kollisionsabstand: mit Toleranz 0,25 meldet dieselbe Platte (einstellbar)");
    }
    // Clip-Rampe ueber einer Treppe (die Liste verlangt sie): schraege Oberseite hoechstens eine Stufe ueber den Tritten - schweigt
    {
        AbbildPrimitiv rampe;
        rampe.mins = Vektor3f(0, 0, 0);
        rampe.maxs = Vektor3f(64, 64, 16);
        using V = Vektor3d;
        const double l = std::sqrt(17.0);
        const auto seite = [&](const V& n, double d, std::vector<V> pts) {
            AbbildFlaeche f;
            f.shader = kClip;
            f.ebene = Ebene3d(n, d);
            f.punkte = std::move(pts);
            rampe.flaechen.push_back(f);
        };
        seite(V(-1 / l, 0, 4 / l), 0, {V(0, 0, 0), V(64, 0, 16), V(64, 64, 16), V(0, 64, 0)});
        seite(V(0, 0, -1), 0, {V(0, 0, 0), V(0, 64, 0), V(64, 64, 0), V(64, 0, 0)});
        seite(V(1, 0, 0), 64, {V(64, 0, 0), V(64, 64, 0), V(64, 64, 16), V(64, 0, 16)});
        seite(V(0, 1, 0), 64, {V(0, 64, 0), V(64, 64, 16), V(64, 64, 0)});
        seite(V(0, -1, 0), 0, {V(0, 0, 0), V(64, 0, 0), V(64, 0, 16)});
        const auto b = pruefen(welt({kasten(-64, 0, -16, 128, 64, 0, kFest), kasten(32, 0, 0, 64, 64, 8, kFest), rampe}), optionen());
        pruefe(anzahl(b, "kollisionsabstand") == 0, "kollisionsabstand: Clip-Rampe ueber einer Treppe, hoechstens STEPSIZE ueber den Tritten (schweigt)");
    }
    // c) unsichtbare Kollision braucht die .bsp des letzten Baus
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(64, 64, 0, 128, 128, 64, kClip)}), optionen());
        pruefe(anzahl(b, "unsichtbarekollision") == 0, "unsichtbarekollision: ohne .bsp keine Aussage (wo die Spielfigur hinkommt, sagt erst der Bau)");
    }
}

void testeDeckung() {
    // dicke Platte (Struktur) auf einem Boden, deren Oberseite 1 ueber dem Boden liegt: die BSP schneidet den Boden darunter weg
    {
        const auto g = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), kasten(32, -8, -15, 96, 72, 1, kPlatte)}), optionen());
        pruefe(anzahl(g, "fastdeckungsgleich") == 0, "fastdeckungsgleich: Strukturpinsel schneidet die Flaeche darunter weg (schweigt)");
    }
    // duenne Platte 0,5 ... 1 ueber dem Boden: beide sichtbar, 1 Einheit auseinander
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), kasten(32, 0, 0.5, 96, 64, 1, kPlatte)}), optionen());
        const Befund* x = befund(b, "fastdeckungsgleich");
        pruefe(x != nullptr && x->anzahl == 1 && x->objekte.size() == 2, "fastdeckungsgleich: Platte 1 Einheit ueber dem Boden (rot, ein Paar)");
        const auto g = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), kasten(32, 0, 2.5, 96, 64, 3, kPlatte)}), optionen());
        pruefe(anzahl(g, "fastdeckungsgleich") == 0, "fastdeckungsgleich: 3 Einheiten auseinander (schweigt, Grenze 2)");
    }
    // gleiche Ebene, halb ueberlappend (zwei Pinsel nebeneinander, Oberseiten gleich hoch): Z-Fighting
    {
        AbbildPrimitiv d = kasten(32, -8, -15, 96, 72, 0, kPlatte);   // nur die Oberseiten liegen in einer Ebene
        d.detail = true;
        const auto b = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), d}), optionen());
        pruefe(anzahl(b, "fastdeckungsgleich") == 1, "fastdeckungsgleich: Detailpinsel ueberlappt mit gleicher Oberseite (rot)");
        const auto g = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), kasten(0, 0, -16, 64, 64, 0, kPlatte)}), optionen());
        pruefe(anzahl(g, "fastdeckungsgleich") == 0, "fastdeckungsgleich: genau gleiche Pinsel - CullSides entfernt eine Seite (schweigt)");
        AbbildPrimitiv gleich = kasten(32, -8, -15, 96, 72, 0, kFest);
        gleich.detail = true;
        const auto s = pruefen(welt({kasten(0, 0, -16, 64, 64, 0, kFest), gleich}), optionen());
        pruefe(anzahl(s, "fastdeckungsgleich") == 0, "fastdeckungsgleich: derselbe Shader auf beiden - man sieht kein Flackern (schweigt)");
    }
}

void testeStufen() {
    {
        const auto b = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(256, 0, -16, 512, 256, 24, kFest)}), optionen());
        const Befund* x = befund(b, "stufehoch");
        pruefe(x != nullptr && x->anzahl == 1 && x->detail.find("highest 24") != std::string::npos, "stufehoch: Stufe 24 (rot)");
        const auto g = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(256, 0, -16, 512, 256, 18, kFest)}), optionen());
        pruefe(anzahl(g, "stufehoch") == 0, "stufehoch: Stufe 18 = STEPSIZE (schweigt)");
        const auto w = pruefen(welt({kasten(0, 0, -16, 256, 256, 0, kFest), kasten(256, 0, -16, 512, 256, 48, kFest)}), optionen());
        pruefe(anzahl(w, "stufehoch") == 0, "stufehoch: 48 hoch ist eine Wand, keine Stufe (schweigt)");
    }
    // Kisten und Pulte: die obere Flaeche ist klein und zu Fuss mit nichts verbunden - ein Hindernis, keine Stufe
    {
        const auto kiste = [](double breite, PruefOptionen o) {
            return pruefen(welt({kasten(-512, -512, -16, 512, 512, 0, kFest), kasten(0, 0, 0, breite, breite, 24, kFest)}), o);
        };
        pruefe(anzahl(kiste(48, optionen()), "stufehoch") == 0, "stufehoch: Kiste 48 x 48 x 24 auf dem Boden ist ein Hindernis (schweigt)");
        PruefOptionen alt = optionen();
        alt.masse.stufenFlaeche = 0;   // wie vor der Regel: jede Stufe zaehlt
        pruefe(anzahl(kiste(48, alt), "stufehoch") == 1, "stufehoch: dieselbe Kiste ohne Mindestflaeche (0) meldet - so war es vorher (rot)");
        pruefe(anzahl(kiste(160, optionen()), "stufehoch") == 1, "stufehoch: Podest 160 x 160 x 24 ist eine Stufe auf eine Flaeche ueber 96 x 96 (rot)");
        PruefOptionen gross = optionen();
        gross.masse.stufenFlaeche = 192;
        pruefe(anzahl(kiste(160, gross), "stufehoch") == 0, "stufehoch: Mindestflaeche 192 (einstellbar) - dasselbe Podest schweigt");
    }
}

void testeMasse() {
    // Tueren
    {
        KartenAbbild a = welt({kasten(-512, -512, -16, 512, 512, 0, kFest)});
        AbbildEntitaet t = ent("func_door");
        t.behaelter = true;
        t.prims.push_back(kasten(0, 0, 0, 8, 48, 96, kFest));
        a.ents.push_back(t);
        AbbildEntitaet gut = ent("func_door");
        gut.behaelter = true;
        gut.prims.push_back(kasten(100, 0, 0, 108, 64, 112, kFest));
        a.ents.push_back(gut);
        const auto b = pruefen(a, optionen());
        const Befund* x = befund(b, "tuerklein");
        pruefe(x != nullptr && x->anzahl == 1 && x->objekte[0].entitaet == 1, "tuerklein: Tuer 96 x 48 (rot), Tuer 112 x 64 (schweigt)");
    }
    // ein Gang 48 breit, 100 hoch, 512 lang (Waende und Decke aus festen Pinseln)
    {
        const auto gang = [](double breite, double hoehe, double laenge) {
            return welt({kasten(0, -256, -16, laenge, 256, 0, kFest), kasten(0, -256, 0, laenge, -breite / 2, hoehe, kFest),
                         kasten(0, breite / 2, 0, laenge, 256, hoehe, kFest), kasten(0, -256, hoehe, laenge, 256, hoehe + 16, kFest)});
        };
        const auto b = pruefen(gang(80, 120, 1024), optionen());
        pruefe(anzahl(b, "gangeng") >= 1 && anzahl(b, "durchgangeng") == 0, "gangeng: Gang 120 x 80 ueber 1024 Laenge (rot als Gang)");
        const auto g = pruefen(gang(128, 160, 1024), optionen());
        pruefe(anzahl(g, "gangeng") == 0 && anzahl(g, "durchgangeng") == 0, "gangeng: Gang 160 x 128 (schweigt)");
        const auto d = pruefen(gang(48, 100, 64), optionen());
        pruefe(anzahl(d, "durchgangeng") >= 1, "durchgangeng: kurzer Durchgang 100 x 48 (rot als Durchgang)");
    }
}

void testeTexturen() {
    KartenAbbild a = welt({kasten(0, 0, -16, 64, 64, 0, kCaulk, "textures/test/npot"), kasten(64, 0, -16, 128, 64, 0, kCaulk, "textures/test/klein"),
                           kasten(128, 0, -16, 192, 64, 0, kCaulk, "textures/test/fehlt"), kasten(192, 0, -16, 256, 64, 0, kCaulk, "textures/test/gut")});
    PruefOptionen o = optionen();
    for (const char* n : {"textures/test/npot", "textures/test/klein", "textures/test/fehlt", "textures/test/gut"}) {
        ShaderArt s;
        s.bilder = {n};
        o.shader[n] = s;
    }
    o.bildMass = [](const std::string& bild, int& b, int& h) {
        if (bild == "textures/test/npot") {
            b = 100;
            h = 64;
            return true;
        }
        if (bild == "textures/test/klein") {
            b = 32;
            h = 32;
            return true;
        }
        if (bild == "textures/test/gut") {
            b = 256;
            h = 512;
            return true;
        }
        return false;
    };
    CwInventar inv;
    o.inventar = &inv;
    const auto b = pruefen(a, o);
    pruefe(anzahl(b, "bildnpot") == 1, "bildnpot: 100 x 64 (rot, ERR_FATAL)");
    pruefe(anzahl(b, "bildgroesse") == 1, "bildgroesse: 32 x 32 unter 64 (rot), 256 x 512 (schweigt)");
    pruefe(anzahl(b, "bildfehlt") == 1, "bildfehlt: Bild nicht da (rot)");
    pruefe(inv.shader.size() == 5 && inv.bilder.size() == 4, "Inventar: 5 Shader (mit caulk), 4 Bilder");
    o.masse.bildMin = 16;
    pruefe(anzahl(pruefen(a, o), "bildgroesse") == 0, "bildgroesse: Untergrenze 16 (einstellbar) - 32 x 32 schweigt");
}

std::string lies(const std::string& pfad) {
    std::ifstream f(pfad, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

}  // namespace

int main(int argc, char** argv) {
    testeShaderInhalte();
    testeSpielfigur();
    testeBildKopf();
    testeSichtGegenKollision();
    testeDeckung();
    testeStufen();
    testeMasse();
    testeTexturen();
    // eine echte Karte (nur lesen): die neuen Befunde und ihre Zeiten
    if (argc > 1) {
        using Uhr = std::chrono::steady_clock;
        const auto t0 = Uhr::now();
        Karte k;
        (void)karteLesen(lies(argv[1]), k);
        k.anmelden();
        const KartenAbbild a = kartenAbbild(k);
        std::string bsp = argc > 2 ? lies(argv[2]) : std::string();
        PruefOptionen o;
        o.spiel = GrenzSpiel::JkaSp;
        o.bspDaten = bsp.empty() ? nullptr : &bsp;
        std::vector<std::pair<std::string, double>> zeiten;
        o.zeiten = &zeiten;
        const auto t1 = Uhr::now();
        const std::vector<Befund> b = pruefeKarte(a, o);
        const auto t2 = Uhr::now();
        static const std::set<std::string> kNeu = {"laufohnekollision", "unsichtbarekollision", "kollisionsabstand", "fastdeckungsgleich", "stufehoch",
                                                   "tuerklein", "durchgangeng", "gangeng", "bildnpot", "bildfehlt", "bildgroesse", "modellteilflaeche",
                                                   "modellweltkoord", "md3grenzen", "requisitschwebt", "requisitsteckt"};
        std::printf("Karte %s: %zu Entitaeten, laden %.0f ms, pruefen %.0f ms\n", argv[1], a.ents.size(),
                    std::chrono::duration<double, std::milli>(t1 - t0).count(), std::chrono::duration<double, std::milli>(t2 - t1).count());
        for (const Befund& x : b) {
            if (kNeu.count(x.pruefung) == 0) { continue; }
            std::printf("  %-8s %-20s %lld  %s\n", schwereName(x.schwere), x.pruefung.c_str(), x.anzahl, x.detail.c_str());
            for (std::size_t i = 0; i < x.objekte.size() && i < 8; ++i) { std::printf("      %s\n", x.objekte[i].text.c_str()); }
        }
        std::printf("  Zeiten:");
        for (const auto& [w, t] : zeiten) {
            if (w.rfind("cw_", 0) == 0) { std::printf(" %s %.0f", w.c_str(), t); }
        }
        std::printf(" ms\n");
    }
    std::printf("kartecwpruef: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
