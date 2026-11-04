// bsplumpstest.cpp - rc609 J: Shader-, Fog- und Flaechen-Lump fertiger Karten (ctest bsplumps).
//
// An tests/karte/lumps/lumps_ibsp.bsp (IBSP 46, bhed_q3map2 -game quake3 -meta + -light -fast aus
// tests/karte/q3map2/markensonder.map) und lumps_rbsp.bsp (RBSP 1, dasselbe mit -game ja), je 9
// Shader, 3 Nebel, 87 Flaechen, 57 mit Lightmap:
//   - unveraendert geschrieben: jeder Lump Byte fuer Byte gleich;
//   - Text hin und zurueck gleich;
//   - Shader einer Flaeche tauschen (neuer Shader angehaengt) und ein Flag aendern, einen
//     Nebel aendern: alle anderen Lumps Byte-gleich, im Flaechen-Lump nur shaderNum/fogNum
//     der Flaeche, die Lumps lesen sich wieder, die Datei laedt im eigenen BSP-Leser
//     (RBSP: readBspGeometry und readBsp; IBSP liest readBspGeometry nicht - 80-Byte-Ecken
//     und 148-Byte-Flaechen fest -, dort bspLesen und bspLumpsLesen);
//   - Grenzen: 63/64 Zeichen, 1024/1025 Shader, Shader entfernen, shaderNum, fogNum,
//     brushNum, visibleSide, globaler Nebel, "funny lump size"; Textfehler mit Zeile.
// Gegenprobe (wenn die Spielordner angegeben sind) an Karten, die nicht zum Entwickeln dienten:
// JKA maps/academy3.bsp, JK2 maps/duel_hangar.bsp, Movie Duels maps/duel_mandalore.bsp.
//
// Aufruf: bsplumpstest <tests/karte/lumps> [<JA base> <JK2 base> <MD-Ordner;...>]
#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/karte/bsplumps.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/pk3.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;
namespace fs = std::filesystem;

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

std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

bool gleicheLumps(const BspLumps& a, const BspLumps& b) {
    if (a.shader.size() != b.shader.size() || a.nebel.size() != b.nebel.size() || a.flaechen.size() != b.flaechen.size()) { return false; }
    for (std::size_t i = 0; i < a.shader.size(); ++i) {
        if (a.shader[i].name != b.shader[i].name || a.shader[i].surfaceFlags != b.shader[i].surfaceFlags ||
            a.shader[i].contentFlags != b.shader[i].contentFlags) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.nebel.size(); ++i) {
        if (a.nebel[i].shader != b.nebel[i].shader || a.nebel[i].brushNum != b.nebel[i].brushNum || a.nebel[i].visibleSide != b.nebel[i].visibleSide) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.flaechen.size(); ++i) {
        if (a.flaechen[i].shaderNum != b.flaechen[i].shaderNum || a.flaechen[i].fogNum != b.flaechen[i].fogNum) { return false; }
    }
    return true;
}

// Welche Lumps unterscheiden sich (Nummern)?
std::set<std::size_t> andereLumps(const std::string& a, const std::string& b) {
    BspDatei x;
    BspDatei y;
    std::set<std::size_t> s;
    if (!bspLesen(a, x) || !bspLesen(b, y) || x.lumps.size() != y.lumps.size()) {
        s.insert(999);
        return s;
    }
    for (std::size_t i = 0; i < x.lumps.size(); ++i) {
        if (x.lumps[i] != y.lumps[i]) { s.insert(i); }
    }
    return s;
}

// Eine Datei laedt im eigenen Leser (RBSP: Geometrie und Entities wie die Vorschau).
bool laedt(const std::string& bsp, bool rbsp, std::size_t& shaderZahl) {
    BspLumps l;
    BspDatei d;
    if (!bspLesen(bsp, d) || !bspLumpsLesen(bsp, l)) { return false; }
    shaderZahl = l.shader.size();
    if (!rbsp) { return true; }
    bhed::BspGeometry g;
    bhed::MapData m;
    std::string f;
    if (!bhed::readBspGeometry(bsp, g, &f) || !bhed::readBsp(bsp, m, &f)) { return false; }
    shaderZahl = g.shaders.size();
    return g.surfaces.size() == l.flaechen.size();
}

// Die Pruefungen an einer Karte. tief: auch die Grenzen und der Text.
void karte(const std::string& name, const std::string& orig, bool tief) {
    BspLumps l;
    std::string fl;
    const bool ok = bspLumpsLesen(orig, l, &fl);
    pruefe(ok, name + ": lesen (" + fl + ")");
    if (!ok) { return; }
    std::printf("  %s: %s, %zu Shader, %zu Nebel, %zu Flaechen, %zu Brushes, %zu Brush-Seiten\n", name.c_str(), l.rbsp ? "RBSP 1" : "IBSP 46",
                l.shader.size(), l.nebel.size(), l.flaechen.size(), l.brushes.size(), l.seitenShader.size());
    // Unveraendert geschrieben: jeder Lump gleich
    const std::string gleich = bspLumpsSchreiben(orig, l, &fl);
    pruefe(!gleich.empty() && andereLumps(orig, gleich).empty(), name + ": unveraendert geschrieben - alle Lumps Byte-gleich (" + fl + ")");
    // Text hin und zurueck
    BspLumps t;
    LumpTextFehler tf;
    pruefe(lumpsAusText(lumpsAlsText(l), l, t, &tf) && gleicheLumps(l, t), name + ": Text hin und zurueck (" + tf.text + ")");

    // Shader einer Flaeche mit Lightmap tauschen, ein Flag aendern
    std::size_t k = 0;
    while (k < l.flaechen.size() && l.flaechen[k].lightmap < 0) { ++k; }
    if (k == l.flaechen.size()) { k = 0; }
    BspLumps n = l;
    const LumpShader alt = l.shader[static_cast<std::size_t>(l.flaechen[k].shaderNum)];
    const int neu = shaderEintrag(n, "textures/jkstudio/getauscht", alt.surfaceFlags, alt.contentFlags);
    pruefe(neu == static_cast<int>(l.shader.size()), name + ": neuer Shader angehaengt");
    n.flaechen[k].shaderNum = neu;
    n.shader[0].contentFlags ^= 0x00000004U;   // CONTENTS_WATER
    if (!n.nebel.empty()) {
        // Nebel 0: ein anderer Shader, die sichtbare Seite wechseln (eine Seite des Brushes)
        n.nebel[0].shader = "textures/jkstudio/nebel";
        if (n.nebel[0].brushNum >= 0) {
            const LumpBrush& b = n.brushes[static_cast<std::size_t>(n.nebel[0].brushNum)];
            n.nebel[0].visibleSide = n.nebel[0].visibleSide == b.seiten - 1 ? 0 : b.seiten - 1;
        }
    }
    const std::string bsp = bspLumpsSchreiben(orig, n, &fl);
    pruefe(!bsp.empty(), name + ": tauschen geschrieben (" + fl + ")");
    if (bsp.empty()) { return; }
    std::set<std::size_t> soll = {1, 13};
    if (!n.nebel.empty()) { soll.insert(12); }
    pruefe(andereLumps(orig, bsp) == soll, name + ": nur Shader-, Fog- und Flaechen-Lump anders");
    BspDatei a;
    BspDatei b;
    (void)bspLesen(orig, a);
    (void)bspLesen(bsp, b);
    // Flaechen-Lump: nur die 4 Bytes shaderNum der Flaeche k
    const std::size_t fg = l.rbsp ? 148 : 104;
    std::size_t andereBytes = 0;
    bool nurDort = a.lumps[13].size() == b.lumps[13].size();
    for (std::size_t i = 0; nurDort && i < a.lumps[13].size(); ++i) {
        if (a.lumps[13][i] != b.lumps[13][i]) {
            ++andereBytes;
            nurDort = i >= k * fg && i < k * fg + 4;
        }
    }
    pruefe(nurDort && andereBytes >= 1, name + ": im Flaechen-Lump nur shaderNum der Flaeche " + std::to_string(k));
    // Shader-Lump: die alten Eintraege bis auf die contentFlags von 0 gleich, einer angehaengt
    bool shaderAlt = b.lumps[1].size() == a.lumps[1].size() + 72;
    for (std::size_t i = 0; shaderAlt && i < a.lumps[1].size(); ++i) {
        const bool erlaubt = i >= 68 && i < 72;
        if (a.lumps[1][i] != b.lumps[1][i] && !erlaubt) { shaderAlt = false; }
    }
    pruefe(shaderAlt, name + ": Shader-Lump: alte Eintraege Byte-gleich ausser den geaenderten Flags, einer angehaengt");
    BspLumps w;
    pruefe(bspLumpsLesen(bsp, w) && gleicheLumps(w, n), name + ": das Lump liest sich wieder");
    std::size_t zahl = 0;
    pruefe(laedt(bsp, l.rbsp, zahl) && zahl == l.shader.size() + 1, name + ": laedt im eigenen BSP-Leser, " + std::to_string(zahl) + " Shader");
    if (l.rbsp) {
        bhed::BspGeometry g;
        pruefe(bhed::readBspGeometry(bsp, g, nullptr) && g.surfaces[k].shader == neu && g.shaders[static_cast<std::size_t>(neu)].name == "textures/jkstudio/getauscht",
               name + ": readBspGeometry sieht den neuen Shader an der Flaeche");
    }
    // Nutzung: jede Flaeche, jeder Brush, jede Seite genau einmal gezaehlt
    int fl2 = 0;
    int br = 0;
    int se = 0;
    for (std::size_t i = 0; i < w.shader.size(); ++i) {
        const ShaderNutzung u = shaderNutzung(w, static_cast<int>(i));
        fl2 += u.flaechen;
        br += u.brushes;
        se += u.seiten;
    }
    pruefe(fl2 == static_cast<int>(w.flaechen.size()) && br == static_cast<int>(w.brushes.size()) && se == static_cast<int>(w.seitenShader.size()),
           name + ": Nutzung zaehlt jede Flaeche, jeden Brush, jede Seite einmal");
    if (!tief) { return; }

    // --- Grenzen ----------------------------------------------------------------------------
    const auto abgelehnt = [&](BspLumps x, const std::string& was) {
        std::string f;
        const bool nein = bspLumpsSchreiben(orig, x, &f).empty() && !f.empty();
        pruefe(nein, name + ": abgelehnt - " + was + " (" + f + ")");
    };
    const auto angenommen = [&](BspLumps x, const std::string& was) {
        std::string f;
        pruefe(!bspLumpsSchreiben(orig, x, &f).empty(), name + ": angenommen - " + was + " (" + f + ")");
    };
    {
        BspLumps x = l;
        x.shader[1].name = std::string(63, 'a');
        angenommen(x, "Name mit 63 Zeichen");
        x.shader[1].name = std::string(64, 'a');
        abgelehnt(x, "Name mit 64 Zeichen");
        x = l;
        x.shader[1].name.clear();
        abgelehnt(x, "leerer Name");
    }
    {
        BspLumps x = l;
        while (x.shader.size() < kMaxMapShaders) { x.shader.push_back({"textures/x/" + std::to_string(x.shader.size()), 0, 1}); }
        angenommen(x, "1024 Shader (MAX_MAP_SHADERS)");
        x.shader.push_back({"textures/x/zuviel", 0, 1});
        abgelehnt(x, "1025 Shader");
    }
    {
        BspLumps x = l;
        x.shader.pop_back();
        abgelehnt(x, "einen Shader entfernen");
        x = l;
        x.flaechen[0].shaderNum = static_cast<std::int32_t>(l.shader.size());
        abgelehnt(x, "shaderNum = Anzahl (ShaderForShaderNum: bad num)");
        x = l;
        x.flaechen[0].fogNum = static_cast<std::int32_t>(l.nebel.size());
        abgelehnt(x, "fogNum = Anzahl der Nebel");
        x = l;
        x.flaechen[0].fogNum = -2;
        abgelehnt(x, "fogNum -2");
    }
    if (!l.nebel.empty()) {
        BspLumps x = l;
        x.nebel[0].brushNum = static_cast<std::int32_t>(l.brushes.size());
        abgelehnt(x, "fog brushNumber out of range");
        x = l;
        x.nebel[0].brushNum = -1;
        x.nebel[0].visibleSide = 0;
        abgelehnt(x, "globaler Nebel mit sichtbarer Seite");
        x.nebel[0].visibleSide = -1;
        angenommen(x, "globaler Nebel ohne Seite");
        x = l;
        if (x.nebel[0].brushNum >= 0) {
            x.nebel[0].visibleSide = x.brushes[static_cast<std::size_t>(x.nebel[0].brushNum)].seiten;
            abgelehnt(x, "visibleSide = Seitenzahl des Brushes");
        }
        x = l;
        x.nebel.pop_back();
        abgelehnt(x, "einen Nebel entfernen");
    }
    {
        // funny lump size: das Shader-Lump um 4 Bytes laenger
        BspDatei d;
        (void)bspLesen(orig, d);
        const std::string kaputt = bspMitLumps(orig, {{1, d.lumps[1] + std::string(4, '\0')}});
        BspLumps x;
        std::string f;
        pruefe(!bspLumpsLesen(kaputt, x, &f) && f == "funny lump size", name + ": funny lump size (" + f + ")");
    }
    // --- Text: Fehler mit Zeile ------------------------------------------------------------
    const std::string text = lumpsAlsText(l);
    const int z2 = lumpTextZeile(text, false, 2);
    const auto textFehler = [&](const std::string& tx, int zeile, const std::string& was) {
        BspLumps x;
        LumpTextFehler f;
        const bool nein = !lumpsAusText(tx, l, x, &f);
        pruefe(nein && f.zeile == zeile, name + ": Text " + was + " -> Zeile " + std::to_string(f.zeile) + " (soll " + std::to_string(zeile) + ", " + f.text + ")");
    };
    const auto zeileErsetzen = [&](int zeile, const std::string& neueZeile) {
        std::istringstream is(text);
        std::string s;
        std::string out;
        int nr = 0;
        while (std::getline(is, s)) {
            ++nr;
            out += (nr == zeile ? neueZeile : s) + "\n";
        }
        return out;
    };
    pruefe(z2 > 0, name + ": Zeile von Shader 2 gefunden");
    bool nebelTeil = true;
    pruefe(lumpTextEintrag(text, z2, nebelTeil) == 2 && !nebelTeil, name + ": Zeile -> Shader 2");
    textFehler(zeileErsetzen(z2, "3 \"textures/x\" 0 1"), z2, "falsche Nummer");
    textFehler(zeileErsetzen(z2, "2 textures/x 0 1"), z2, "ohne Anfuehrungszeichen");
    textFehler(zeileErsetzen(z2, "2 \"textures/x\" 0 1 7"), z2, "Text dahinter");
    textFehler(zeileErsetzen(z2, "2 \"textures/x\" zwei 1"), z2, "Flag keine Zahl");
    textFehler(zeileErsetzen(z2, "2 \"" + std::string(64, 'b') + "\" 0 1"), z2, "Name mit 64 Zeichen");
    if (!l.nebel.empty()) {
        const int zn = lumpTextZeile(text, true, 0);
        pruefe(lumpTextEintrag(text, zn, nebelTeil) == 0 && nebelTeil, name + ": Zeile -> Nebel 0");
        textFehler(zeileErsetzen(zn, "0 \"" + l.nebel[0].shader + "\" " + std::to_string(l.brushes.size()) + " -1"), zn, "brushNum ausserhalb");
    }
    {
        // ein Shader angehaengt, Flags hexadezimal und negativ dezimal
        BspLumps x;
        LumpTextFehler f;
        std::string tx = text;
        const std::size_t p = tx.find("\n\n// fogs");
        tx.insert(p, "\n" + std::to_string(l.shader.size()) + " \"textures/jkstudio/text\" 0x00200000 -2147483648");
        const bool ja = lumpsAusText(tx, l, x, &f);
        pruefe(ja && x.shader.size() == l.shader.size() + 1 && x.shader.back().surfaceFlags == 0x00200000U && x.shader.back().contentFlags == 0x80000000U,
               name + ": Text haengt einen Shader an (" + f.text + ")");
    }
}

// Eine Karte aus den .pk3 eines Ordners (die spaetere .pk3 zuerst, wie das Spiel).
bool ausSpiel(const std::string& ordner, const std::string& eintrag, std::string& out) {
    std::vector<fs::path> pk3;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(bhed::utf8Pfad(ordner), ec)) {
        if (e.path().extension() == ".pk3") { pk3.push_back(e.path()); }
    }
    std::sort(pk3.rbegin(), pk3.rend());
    for (const fs::path& p : pk3) {
        bhed::Pk3 a;
        if (!bhed::readPk3Directory(p.string(), a)) { continue; }
        const bhed::Pk3Entry* e = a.find(eintrag);
        if (e != nullptr && bhed::readPk3File(a, *e, out)) { return true; }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: bsplumpstest <tests/karte/lumps> [<JA base> <JK2 base> <MD-Ordner;...>]\n");
        return 2;
    }
    const fs::path ordner = bhed::utf8Pfad(argv[1]);
    for (const char* n : {"lumps_ibsp.bsp", "lumps_rbsp.bsp"}) {
        const std::string d = lies(ordner / n);
        pruefe(!d.empty(), std::string(n) + " lesbar");
        if (!d.empty()) { karte(n, d, true); }
    }
    // Gegenprobe an Spielkarten
    struct Probe {
        int arg;
        const char* karte;
    };
    for (const Probe p : {Probe{2, "maps/academy3.bsp"}, Probe{3, "maps/duel_hangar.bsp"}, Probe{4, "maps/duel_mandalore.bsp"}}) {
        if (argc <= p.arg || argv[p.arg][0] == '\0') { continue; }
        std::string ordnerListe = argv[p.arg];
        std::string d;
        std::size_t a = 0;
        while (d.empty() && a <= ordnerListe.size()) {
            std::size_t e = ordnerListe.find(';', a);
            if (e == std::string::npos) { e = ordnerListe.size(); }
            (void)ausSpiel(ordnerListe.substr(a, e - a), p.karte, d);
            a = e + 1;
        }
        if (d.empty()) {
            std::printf("  %s nicht gefunden in %s - uebersprungen\n", p.karte, argv[p.arg]);
            continue;
        }
        karte(p.karte, d, false);
    }
    std::printf("%d Pruefungen, %d fehlgeschlagen\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
