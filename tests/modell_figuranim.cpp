// modell_figuranim.cpp - Modell-Block Stufe 7: Figuren im Mission-Tab animieren (bhed/modell/figuranim.h)
//
//   Zeilen         die Schicht (FK-Schluessel mit freien Henkeln, IK-Spur, fremder Raum) als Kommentarzeilen und zurueck:
//                  dieselben Zeilen, jedes Bild dieselbe Welt Bit fuer Bit
//   Grundanimation Bild f zeigt die Grundanimation zur Zeit f * frameLerp: Schleife (loopFrames 0) bzw. letztes Bild
//                  gehalten (-1), wie frameForAnimationIn (bg_panimate.cpp:4723)
//   Wurzelweg      SET_ORIGIN = Start + Drehung * (Becken am Ende - Becken im ersten Bild der Folgeanimation), waagrecht
//   ICARUS         der Block mit SET_ANIM_BOTH, Haltezeit, wait, SET_ORIGIN und Folgeanimation, waitsignal/signal; ohne
//                  SET_ORIGIN (Rot-Probe aus Stufe 0) fehlt er; Ersetzen haelt einen Block je Figur
//   Plaetze        der gemerkte Platz bleibt, belegte der vorhandenen Karten-GLA nicht; alle voll: Fehler
//   Bauen          Schicht leer: die Bilder des Platzes sind die der Grundanimation (14 Bytes gleich); zweimal gebaut:
//                  dieselbe Datei Byte fuer Byte; die vorhandene Karten-GLA behaelt ihre Zeilen
//   Fremder Raum   die Hand der einen Figur folgt dem Knochen der anderen in der Welt
//   Dopesheet      IK-Schluessel als Zeile: mit den FK-Schluesseln verschoben, allein verschoben, geloescht
//
// Aufruf: modellfiguranimtest
#include "bhed/gla.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/figuranim.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"
#include "bhed/script.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "rig_handtabelle.inc"

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}

using namespace bhed;
using namespace bhed::modell;

const RigHandSkelett& tabelle(const char* name) {
    for (const RigHandSkelett& s : kRigHandSkelette) {
        if (std::strcmp(s.name, name) == 0) { return s; }
    }
    return kRigHandSkelette[0];
}

void eintrag(std::vector<std::uint8_t>& pool, float tx) {
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, static_cast<std::uint16_t>((tx + 512.0F) * 64.0F), 512 * 64, 512 * 64};
    for (const std::uint16_t v : e) {
        pool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        pool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
}

// Das JKA-Skelett der Hand-Tabelle; das Becken geht in Bild f um 2 f Einheiten entlang X (Wurzelbewegung)
GlaAnimation skelett(int bilder, int becken) {
    const RigHandSkelett& h = tabelle("jka");
    GlaAnimation g;
    g.name = "_humanoid";
    g.skeletonName = "models/players/_humanoid/_humanoid";
    g.scale = 0.64F;
    for (int i = 0; i < h.zahl; ++i) {
        GlaBone b;
        b.name = h.knochen[i].name;
        b.parent = h.knochen[i].eltern;
        b.basePose = BoneMatrix::identity();
        b.basePose.m[0][3] = h.knochen[i].x;
        b.basePose.m[1][3] = h.knochen[i].y;
        b.basePose.m[2][3] = h.knochen[i].z;
        b.basePoseInv = BoneMatrix::identity();
        b.basePoseInv.m[0][3] = -h.knochen[i].x;
        b.basePoseInv.m[1][3] = -h.knochen[i].y;
        b.basePoseInv.m[2][3] = -h.knochen[i].z;
        g.bones.push_back(b);
    }
    for (int i = 0; i < h.zahl; ++i) {
        if (g.bones[static_cast<std::size_t>(i)].parent >= 0) { g.bones[static_cast<std::size_t>(g.bones[static_cast<std::size_t>(i)].parent)].kinder.push_back(i); }
    }
    eintrag(g.bonePool, 0.0F);
    for (int f = 0; f < bilder; ++f) { eintrag(g.bonePool, 2.0F * static_cast<float>(f)); }
    g.numFrames = bilder;
    g.indexes.assign(static_cast<std::size_t>(bilder) * g.bones.size(), 0U);
    for (int f = 0; f < bilder && becken >= 0; ++f) {
        g.indexes[static_cast<std::size_t>(f) * g.bones.size() + static_cast<std::size_t>(becken)] = static_cast<std::uint32_t>(f + 1);
    }
    return g;
}

int knochen(const GlaAnimation& g, const char* n) { return boneIndex(g, n); }

bool welteGleich(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b) {
    std::vector<BoneMatrix> wa;
    std::vector<BoneMatrix> wb;
    for (int f = 0; f < g.numFrames; ++f) {
        rigWelt(g, a, f, f, 0.0F, wa);
        rigWelt(g, b, f, f, 0.0F, wb);
        if (wa.size() != wb.size() || std::memcmp(wa.data(), wb.data(), wa.size() * sizeof(BoneMatrix)) != 0) { return false; }
    }
    return true;
}

std::string skriptText(const Script& s) {
    std::string t;
    for (const Node& n : s.nodes) {
        if (n.kind == Node::Kind::LineComment) { continue; }
        t += n.name + "(";
        for (const Arg& a : n.args) { t += a.text + ","; }
        t += ")";
        for (const Node& c : n.children) {
            t += " " + c.name + "(";
            for (const Arg& a : c.args) { t += a.text + ","; }
            t += ")";
        }
        t += "\n";
    }
    return t;
}

}  // namespace

int main() {
    const int beckenIdx = boneIndex(skelett(1, 0), "pelvis");
    const GlaAnimation sk = skelett(40, beckenIdx);
    const Inhalt leer;
    const Rig rig = rigAnlegen(sk, leer, 0, {});
    std::vector<AnimEntry> abschnitte = {AnimEntry{"BOTH_WALK1", 0, 10, 0, 20}, AnimEntry{"BOTH_STAND1", 10, 10, -1, 20}, AnimEntry{"BOTH_CIN_9", 20, 20, -1, 20}};

    // --- Grundanimation ------------------------------------------------------------------------------------------------
    {
        GlaAnimation lauf;
        GlaAnimation steh;
        const bool ok1 = basisGla(sk, abschnitte, "BOTH_WALK1", 25, 20, lauf);
        const bool ok2 = basisGla(sk, abschnitte, "BOTH_STAND1", 25, 20, steh);
        const auto quelleVon = [&](const GlaAnimation& b, int f) {
            const std::uint8_t* p = b.bonePool.data() + static_cast<std::size_t>(b.indexes[static_cast<std::size_t>(f) * b.bones.size() + static_cast<std::size_t>(beckenIdx)]) * 14U;
            for (int q = 0; q < sk.numFrames; ++q) {
                const std::uint8_t* s = sk.bonePool.data() + static_cast<std::size_t>(sk.indexes[static_cast<std::size_t>(q) * sk.bones.size() + static_cast<std::size_t>(beckenIdx)]) * 14U;
                if (std::memcmp(p, s, 14U) == 0) { return q; }
            }
            return -1;
        };
        std::printf("    Grundanimation: Schleife Bild 12 -> %d, 24 -> %d; gehalten Bild 12 -> %d, 24 -> %d\n", quelleVon(lauf, 12), quelleVon(lauf, 24),
                    quelleVon(steh, 12), quelleVon(steh, 24));
        expect("Grundanimation: 25 Bilder, Skelett der Quelle", ok1 && ok2 && lauf.numFrames == 25 && lauf.bones.size() == sk.bones.size());
        expect("Grundanimation: Schleife (loopFrames 0) wie frameForAnimationIn", quelleVon(lauf, 12) == 2 && quelleVon(lauf, 24) == 4);
        expect("Grundanimation: ohne Schleife (-1) das letzte Bild gehalten", quelleVon(steh, 12) == 19 && quelleVon(steh, 24) == 19);
        GlaAnimation x;
        expect("Grundanimation: unbekannter Name gemeldet", !basisGla(sk, abschnitte, "BOTH_GIBTSNICHT", 5, 20, x));
    }

    // --- Zeilen hin und zurueck -----------------------------------------------------------------------------------------
    GlaAnimation basis;
    (void)basisGla(sk, abschnitte, "BOTH_STAND1", 30, 20, basis);
    FigurAnim a;
    a.figur = "mb7_a";
    a.basis = "BOTH_STAND1";
    a.bilder = 30;
    a.startMs = 1250.5;
    a.platz = "BOTH_CIN_4";
    a.warteSignal = "los";
    a.sendeSignal = "fertig";
    a.schicht.knochen.assign(sk.bones.size(), {});
    a.schicht.neuInterp = Interp::Bezier;
    {
        const float achse[3] = {0.3F, -0.7F, 0.2F};
        for (const int f : {0, 11, 29}) {
            BoneMatrix m = drehungUm(achse, 7.5F * static_cast<float>(f + 1));
            m.m[0][3] = 0.25F * static_cast<float>(f);
            rigSchluesselSetzen(a.schicht, knochen(sk, "lower_lumbar"), f, m);
            rigSchluesselSetzen(a.schicht, knochen(sk, "cranium"), f, drehungUm(achse, -3.0F * static_cast<float>(f)));
        }
        // ein freier Henkel (Kurveneditor): seine Werte muessen mit
        auto& v = a.schicht.knochen[static_cast<std::size_t>(knochen(sk, "cranium"))];
        v[1].henkelL[2] = Henkel::Frei;
        v[1].hl[2][0] = -3.25F;
        v[1].hl[2][1] = 0.125F;
        henkelNeu(v);
        IkSpur sp;
        if (ikSpurAnlegen(basis, rig, IkArt::ArmRechts, sp)) { a.schicht.ik.push_back(sp); }
        std::vector<BoneMatrix> w;
        rigWelt(basis, a.schicht, 15, 15, 0.0F, w);
        BoneMatrix z = starr(w[static_cast<std::size_t>(sp.ende.knochen)]);
        z.m[0][3] += 3.0F;
        z.m[2][3] += 4.5F;
        if (!a.schicht.ik.empty()) { ikZielSetzen(basis, a.schicht, 0, 15, z); }
        FremdRaum f;
        f.art = IkArt::ArmLinks;
        f.figur = "mb7_b";
        f.knochen = "rhand";
        f.von = 3;
        f.bis = 9;
        f.versatz.m[1][3] = 1.5F;
        a.fremd.push_back(f);
    }
    const std::vector<std::string> zeilen = figurZeilen(a);
    FigurAnim b;
    std::string fehler;
    const bool gelesen = figurAusZeilen(zeilen, basis, rig, b, &fehler);
    const std::vector<std::string> zeilen2 = figurZeilen(b);
    std::size_t laenge = 0;
    for (const std::string& z : zeilen) { laenge += z.size(); }
    std::printf("    Zeilen: %zu Zeilen, %zu Zeichen, %d Schluessel, %zu IK-Spuren (%s)\n", zeilen.size(), laenge, a.schicht.schluesselZahl(), a.schicht.ik.size(),
                fehler.c_str());
    expect("Zeilen: gelesen, dieselben Zeilen", gelesen && zeilen == zeilen2);
    expect("Zeilen: Einstellungen gleich", b.figur == a.figur && b.basis == a.basis && b.bilder == a.bilder && b.startMs == a.startMs && b.platz == a.platz &&
                                               b.warteSignal == a.warteSignal && b.sendeSignal == a.sendeSignal && b.setOrigin && b.kino &&
                                               b.schicht.neuInterp == Interp::Bezier && b.fremd.size() == 1U && b.fremd[0].versatz.m[1][3] == 1.5F);
    expect("Zeilen: jedes Bild dieselbe Welt Bit fuer Bit (FK, freie Henkel, IK)", !a.schicht.ik.empty() && welteGleich(basis, a.schicht, b.schicht));
    expect("Zeilen: Marke und Daten erkannt", istFigurMarke(zeilen[0]) && zeilen.size() > 1U && istFigurDaten(zeilen[1]) && !istFigurMarke(zeilen[1]));

    // --- Wurzelweg ------------------------------------------------------------------------------------------------------
    {
        GlaAnimation geht;
        (void)basisGla(sk, abschnitte, "BOTH_CIN_9", 20, 20, geht);   // Becken 2 Einheiten je Bild, Bild 20..39
        RigSchicht s0;
        const std::array<float, 3> d = wurzelVersatz(geht, s0, beckenIdx, sk, 10);   // Folge BOTH_STAND1 ab Bild 10 (Becken 20)
        float fw[16]{};
        const float gier = 30.0F * 3.14159265F / 180.0F;
        // wie gpu::baueFigurWelt: Gier + 90 Grad
        const float c = std::cos(gier + 3.14159265F / 2.0F);
        const float sn = std::sin(gier + 3.14159265F / 2.0F);
        fw[0] = c;
        fw[1] = -sn;
        fw[4] = sn;
        fw[5] = c;
        fw[10] = 1.0F;
        fw[15] = 1.0F;
        const float start[3] = {100.0F, -50.0F, 24.0F};
        const std::array<float, 3> o = setOriginOrt(start, fw, d);
        // Becken am Ende 2*39 = 78, in der Folge 2*10 = 20: 58 Einheiten entlang X des Skeletts, X = links (Gier + 90)
        const float soll[3] = {100.0F + c * 58.0F, -50.0F + sn * 58.0F, 24.0F};
        std::printf("    Wurzelweg: Versatz (%.3f %.3f %.3f), SET_ORIGIN (%.3f %.3f %.3f) soll (%.3f %.3f %.3f)\n", static_cast<double>(d[0]), static_cast<double>(d[1]),
                    static_cast<double>(d[2]), static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2]), static_cast<double>(soll[0]),
                    static_cast<double>(soll[1]), static_cast<double>(soll[2]));
        expect("Wurzelweg: Becken am Ende minus erstes Bild der Folge, waagrecht", std::fabs(d[0] - 58.0F) < 0.02F && d[1] == 0.0F && d[2] == 0.0F);
        expect("Wurzelweg: SET_ORIGIN = Start + Drehung der Figur * Versatz, Hoehe des Starts",
               std::fabs(o[0] - soll[0]) < 0.02F && std::fabs(o[1] - soll[1]) < 0.02F && o[2] == 24.0F);
        const std::vector<std::array<float, 3>> pfad = wurzelPfad(geht, s0, beckenIdx, fw);
        expect("Wurzelweg: Pfad je Bild, Abstand Anfang/Ende 38 Einheiten",
               pfad.size() == 20U && std::fabs(std::hypot(pfad.back()[0] - pfad.front()[0], pfad.back()[1] - pfad.front()[1]) - 38.0F) < 0.02F);
    }

    // --- ICARUS ---------------------------------------------------------------------------------------------------------
    {
        const std::array<float, 3> u{10.5F, -20.25F, 24.0F};
        Script s;
        std::vector<Diag> d;
        (void)readScript("//Generated by BehavEd\n\ncamera ( ENABLE );\nwait ( 500.000 );\n", s, d);
        Script mit = mitFigurBlock(s, a.figur, &a, u);
        const std::string t = skriptText(mit);
        std::printf("%s", t.c_str());
        expect("ICARUS: affect mit Kino, wait, waitsignal, SET_ANIM_BOTH, Haltezeit, wait, SET_ORIGIN, Folge, signal",
               t.find("affect(mb7_a,FLUSH,) set(SET_BEHAVIOR_STATE,BS_CINEMATIC,) wait(1250.500,) waitsignal(los,) set(SET_ANIM_BOTH,BOTH_CIN_4,) "
                      "set(SET_ANIM_HOLDTIME_BOTH,1500,) wait(1500.000,) set(SET_ORIGIN,10.500 -20.250 24.000,) set(SET_ANIM_BOTH,BOTH_STAND1,) "
                      "signal(fertig,)") != std::string::npos);
        expect("ICARUS: Marke und Daten im Skript, gefunden", findeFigurBlock(mit, a.figur).has_value() && figurZeilenImSkript(mit, a.figur) == zeilen &&
                                                                  figurenImSkript(mit) == std::vector<std::string>{"mb7_a"});
        FigurAnim a2 = a;
        a2.setOrigin = false;
        a2.warteSignal.clear();
        mit = mitFigurBlock(mit, a.figur, &a2, u);
        const std::string t2 = skriptText(mit);
        expect("ICARUS: ersetzt (ein Block), ohne SET_ORIGIN (Rot-Probe aus Stufe 0) fehlt er",
               std::count(t2.begin(), t2.end(), '\n') == std::count(t.begin(), t.end(), '\n') && t2.find("SET_ORIGIN") == std::string::npos &&
                   t2.find("waitsignal") == std::string::npos);
        FigurAnim c = a;
        c.figur = "mb7_c";
        mit = mitFigurBlock(mit, c.figur, &c, u);
        expect("ICARUS: zweite Figur hinter der ersten", figurenImSkript(mit) == std::vector<std::string>({"mb7_a", "mb7_c"}));
        mit = mitFigurBlock(mit, a.figur, nullptr, u);
        expect("ICARUS: Block entfernt, die andere Figur bleibt", !findeFigurBlock(mit, a.figur).has_value() && findeFigurBlock(mit, c.figur).has_value() &&
                                                                     skriptText(mit).find("affect(mb7_a") == std::string::npos);
        expect("ICARUS: Dauer = Bilder * frameLerp (30 * 50 ms)", figurDauerMs(a) == 1500);
    }

    // --- Plaetze --------------------------------------------------------------------------------------------------------
    {
        FigurAnim p1;
        p1.platz = "BOTH_CIN_7";
        FigurAnim p2;
        FigurAnim p3;
        std::string f;
        const std::vector<std::string> v = plaetzeVerteilen({&p1, &p2, &p3}, {"BOTH_CIN_1", "BOTH_CIN_2", "BOTH_CIN_7"}, Zielspiel::JkaSp, &f);
        std::printf("    Plaetze: %s %s %s\n", v.size() == 3 ? v[0].c_str() : "-", v.size() == 3 ? v[1].c_str() : "-", v.size() == 3 ? v[2].c_str() : "-");
        expect("Plaetze: gemerkter bleibt, belegte der vorhandenen Karten-GLA nicht", v == std::vector<std::string>({"BOTH_CIN_7", "BOTH_CIN_3", "BOTH_CIN_4"}));
        std::vector<std::string> voll;
        for (int i = 1; i <= 50; ++i) { voll.push_back("BOTH_CIN_" + std::to_string(i)); }
        const std::vector<std::string> x = plaetzeVerteilen({&p2}, voll, Zielspiel::JkaSp, &f);
        expect("Plaetze: JKA alle 50 belegt -> Fehler", x.empty() && !f.empty());
        const std::vector<std::string> md = plaetzeVerteilen({&p2}, voll, Zielspiel::MdSp, &f);
        expect("Plaetze: Movie Duels hat 125 (BOTH_CIN_51 frei)", md == std::vector<std::string>({"BOTH_CIN_51"}));
    }

    // --- Bauen ----------------------------------------------------------------------------------------------------------
    {
        // die vorhandene Karten-GLA: BOTH_CIN_1 (Bilder 0..9 des Skeletts) und FACE_ALERT
        GlaAnimation vorh;
        (void)basisGla(sk, abschnitte, "BOTH_WALK1", 10, 20, vorh);
        vorh.skeletonName = "models/players/_humanoid_mb7/_humanoid_mb7";
        const std::vector<AnimEntry> vcfg = {AnimEntry{"BOTH_CIN_1", 0, 10, -1, 20}, AnimEntry{"FACE_ALERT", 3, 1, -1, 20}};
        FigurAnim leerA;
        leerA.figur = "mb7_a";
        leerA.bilder = 30;
        leerA.schicht.knochen.assign(sk.bones.size(), {});
        const std::vector<std::string> pl = plaetzeVerteilen({&leerA, &a}, {"BOTH_CIN_1", "FACE_ALERT"}, Zielspiel::MdSp);
        std::vector<FigurBauEingabe> ein(2);
        ein[0].anim = &leerA;
        ein[0].basis = &basis;
        ein[0].schicht = leerA.schicht;
        ein[1].anim = &a;
        ein[1].basis = &basis;
        ein[1].schicht = a.schicht;
        std::vector<GlaAnimation> geb;
        std::vector<Platz> plaetze;
        FigurBauBericht br;
        const bool ok = figurenBacken(ein, pl, &vorh, vcfg, Zielspiel::MdSp, geb, plaetze, &br);
        GlaAnimation k;
        std::vector<AnimEntry> cfg;
        ZusammenBericht zb;
        const bool ok2 = ok && zusammenfuehren(plaetze, vorh.skeletonName, k, cfg, &zb);
        std::string bytes1;
        std::string bytes2;
        (void)schreibGla(k, bytes1);
        // zweimal gebaut
        std::vector<GlaAnimation> geb2;
        std::vector<Platz> plaetze2;
        GlaAnimation k2;
        std::vector<AnimEntry> cfg2;
        const bool ok3 = figurenBacken(ein, pl, &vorh, vcfg, Zielspiel::MdSp, geb2, plaetze2) && zusammenfuehren(plaetze2, vorh.skeletonName, k2, cfg2);
        (void)schreibGla(k2, bytes2);
        std::string zz;
        for (const AnimEntry& e : cfg) { zz += e.name + "@" + std::to_string(e.firstFrame) + "+" + std::to_string(e.numFrames) + " "; }
        std::printf("    Bauen: Plaetze %s/%s, %d Bilder, Vorrat %d, behalten %d, Zeilen %s(%zu Bytes)\n", pl.size() > 1 ? pl[0].c_str() : "-",
                    pl.size() > 1 ? pl[1].c_str() : "-", k.numFrames, zb.vorrat, br.vorhandenBehalten, zz.c_str(), bytes1.size());
        expect("Bauen: gebaut und zusammengefuehrt", ok2 && !bytes1.empty() && pl == std::vector<std::string>({"BOTH_CIN_2", "BOTH_CIN_4"}));
        expect("Bauen: die vorhandene Karten-GLA behaelt BOTH_CIN_1 und FACE_ALERT", br.vorhandenBehalten == 2 &&
                                                                                       std::any_of(cfg.begin(), cfg.end(), [](const AnimEntry& e) { return e.name == "FACE_ALERT"; }) &&
                                                                                       std::any_of(cfg.begin(), cfg.end(), [](const AnimEntry& e) { return e.name == "BOTH_CIN_1"; }));
        // Schicht leer: der Platz traegt die Bilder der Grundanimation (14 Bytes gleich)
        bool gleich = false;
        for (const AnimEntry& e : cfg) {
            if (e.name != "BOTH_CIN_2" || e.numFrames != basis.numFrames) { continue; }
            gleich = true;
            for (int f = 0; f < e.numFrames && gleich; ++f) {
                for (std::size_t bb = 0; bb < k.bones.size(); ++bb) {
                    const std::uint8_t* p = k.bonePool.data() + static_cast<std::size_t>(k.indexes[static_cast<std::size_t>(e.firstFrame + f) * k.bones.size() + bb]) * 14U;
                    const std::uint8_t* q = basis.bonePool.data() + static_cast<std::size_t>(basis.indexes[static_cast<std::size_t>(f) * basis.bones.size() + bb]) * 14U;
                    if (std::memcmp(p, q, 14U) != 0) { gleich = false; }
                }
            }
        }
        expect("Bauen: Schicht leer -> die Bilder der Grundanimation Byte fuer Byte", gleich);
        expect("Bauen: zweimal gebaut -> dieselbe GLA Byte fuer Byte (Rundlauf)", ok3 && bytes1 == bytes2);
        expect("Bauen: Karten-GLA-Pruefungen ohne Befund (53 Knochen, Name im Kopf)", kartenGlaPruefen(k, vorh.skeletonName, Zielspiel::MdSp).empty() ||
                                                                                          sk.bones.size() != 53U);
    }

    // --- Fremder Raum ---------------------------------------------------------------------------------------------------
    {
        // ohne Wurzelbewegung (beide Figuren stehen, wo ihr Ursprung ist)
        const GlaAnimation still = skelett(30, -1);
        GlaAnimation steht;
        (void)basisGla(still, abschnitte, "BOTH_STAND1", 30, 20, steht);
        FigurAnim fa;
        fa.figur = "mb7_a";
        fa.bilder = 30;
        fa.schicht.knochen.assign(sk.bones.size(), {});
        const float achse[3] = {0.0F, 0.0F, 1.0F};
        rigSchluesselSetzen(fa.schicht, knochen(sk, "rhumerus"), 0, drehungUm(achse, 0.0F));
        rigSchluesselSetzen(fa.schicht, knochen(sk, "rhumerus"), 29, drehungUm(achse, 40.0F));
        FigurAnim fb;
        fb.figur = "mb7_b";
        fb.bilder = 30;
        fb.startMs = 250.0;
        fb.schicht.knochen.assign(sk.bones.size(), {});
        IkSpur sp;
        const bool hat = ikSpurAnlegen(steht, rig, IkArt::ArmLinks, sp);
        fb.schicht.ik.push_back(sp);
        FremdRaum fr;
        fr.art = IkArt::ArmLinks;
        fr.figur = "mb7_a";
        fr.knochen = "rhand";
        fr.von = 5;
        fr.bis = 20;
        fr.versatz.m[0][3] = 2.0F;
        fb.fremd.push_back(fr);
        FigurLage la{&fa, &steht, {}, 0.0};
        FigurLage lb{&fb, &steht, {}, 250.0};
        // zwei Figuren gegenueber: a am Ursprung, b 4 Einheiten davor, um 180 Grad gedreht
        for (FigurLage* l : {&la, &lb}) {
            const float g = (l == &la ? 0.0F : 180.0F) * 3.14159265F / 180.0F + 3.14159265F / 2.0F;
            l->fw[0] = std::cos(g);
            l->fw[1] = -std::sin(g);
            l->fw[4] = std::sin(g);
            l->fw[5] = std::cos(g);
            l->fw[10] = 1.0F;
            l->fw[15] = 1.0F;
        }
        lb.fw[3] = 4.0F;
        RigSchicht sb = fb.schicht;
        const int n = fremdeEinrechnen(steht, lb, {la}, sb);
        // Bild 10 von b ist Zeit 250 + 500 = 750 ms, Bild 15 von a
        std::vector<BoneMatrix> wa;
        std::vector<BoneMatrix> wb;
        rigWelt(steht, fa.schicht, 15, 15, 0.0F, wa);
        rigWelt(steht, sb, 10, 10, 0.0F, wb);
        const int hand = knochen(sk, "lhand");
        const BoneMatrix sollW = BoneMatrix{{{la.fw[0], la.fw[1], la.fw[2], la.fw[3]}, {la.fw[4], la.fw[5], la.fw[6], la.fw[7]}, {la.fw[8], la.fw[9], la.fw[10], la.fw[11]}}} *
                                 starr(wa[static_cast<std::size_t>(knochen(sk, "rhand"))]) * fr.versatz;
        const BoneMatrix istW = BoneMatrix{{{lb.fw[0], lb.fw[1], lb.fw[2], lb.fw[3]}, {lb.fw[4], lb.fw[5], lb.fw[6], lb.fw[7]}, {lb.fw[8], lb.fw[9], lb.fw[10], lb.fw[11]}}} *
                                wb[static_cast<std::size_t>(hand)];
        const float dist = std::hypot(std::hypot(sollW.m[0][3] - istW.m[0][3], sollW.m[1][3] - istW.m[1][3]), sollW.m[2][3] - istW.m[2][3]);
        std::printf("    Fremder Raum: %d Ziele, Hand von b %.4f neben dem Griff an a (soll %.2f %.2f %.2f)\n", n, static_cast<double>(dist),
                    static_cast<double>(sollW.m[0][3]), static_cast<double>(sollW.m[1][3]), static_cast<double>(sollW.m[2][3]));
        expect("Fremder Raum: je Bild der Spanne ein Ziel", hat && n == 16);
        expect("Fremder Raum: die Hand von b folgt dem Knochen von a in der Welt", dist < 0.01F);
    }

    // --- Dopesheet: IK-Schluessel als Zeile ------------------------------------------------------------------------------
    {
        RigSchicht s = a.schicht;
        const IkSpur& sp = s.ik.front();
        const int code = ikWahlCode(0);
        expect("Dopesheet: IK-Schluessel als Zeile vorhanden", s.hatSchluessel(code, 15) && !s.hatSchluessel(code, 14));
        // FK-Schluessel der Kette am Bild 15 (Auto-Einrasten) gewaehlt: der IK-Schluessel geht mit
        const Wahl w{{sp.unten.knochen, 15}};
        const Wahl mit = wahlMitIk(s, w);
        const Wahl neu = schluesselVerschieben(s, w, 4);
        expect("Dopesheet: mit dem FK-Schluessel gewaehlt", mit.size() == 2U && std::find(mit.begin(), mit.end(), SchluesselWahl{code, 15}) != mit.end());
        expect("Dopesheet: mit den FK-Schluesseln verschoben (15 -> 19)", s.hatSchluessel(code, 19) && !s.hatSchluessel(code, 15) &&
                                                                              s.hatSchluessel(sp.unten.knochen, 19) &&
                                                                              std::find(neu.begin(), neu.end(), SchluesselWahl{code, 19}) != neu.end());
        const Wahl allein{{code, 19}};
        (void)schluesselVerschieben(s, allein, -2);
        expect("Dopesheet: IK-Zeile verschoben (19 -> 17), Ziel und Wert zusammen, die FK-Schluessel der Kette gehen mit",
               s.hatSchluessel(code, 17) && s.ik.front().ziel.size() == s.ik.front().wert.size() && s.hatSchluessel(sp.unten.knochen, 17) &&
                   !s.hatSchluessel(sp.unten.knochen, 19));
        rigSchluesselLoeschen(s, code, 17);
        expect("Dopesheet: IK-Schluessel geloescht", !s.hatSchluessel(code, 17) && s.ik.front().ziel.size() == s.ik.front().wert.size());
    }

    std::printf("%s (%d Fehler)\n", fails == 0 ? "alles gut" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
