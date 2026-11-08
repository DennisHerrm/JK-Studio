// modell_dokument.cpp - Modell-Block 1a-1: das Modelldokument und die Ghoul2-Regeln
//
//   Rundlauf      jede LOD-Stufe: readGlm == zuGlm(ausGlm) (Ecken bitgleich,
//                 Dreiecke, Flaggen, Grenzen, groebste Stufe)
//   Halbkanten    naechste^3 = selbst, gegen(gegen) = selbst, Gegenkante
//                 umgekehrt; Naht (gleicher Ort) verbindet, dritte Flaeche an
//                 einer Kante zaehlt als nicht mannigfaltig
//   Undo          aendern / rueckgaengig / wiederholen bitgleich, Tiefe 32
//   G2            RenderSurfaces mit G2_SetSurfaceOnOff, Wurzel, Haut
//   LOD           G2_ComputeLOD mit ProjectRadius (SP Radius 0, MP 64)
//   Grenzen       knapp unter und ueber jeder Grenze je Zielspiel
//   MD3           starr, jede Flaeche gezeichnet
//
// Aufruf: modelldokumenttest <model.glm>
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

using namespace bhed;
using namespace bhed::modell;

bool vertsGleich(const std::vector<GlmVertex>& a, const std::vector<GlmVertex>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::memcmp(a[i].xyz, b[i].xyz, sizeof a[i].xyz) != 0 || std::memcmp(a[i].normal, b[i].normal, sizeof a[i].normal) != 0 ||
            std::memcmp(a[i].st, b[i].st, sizeof a[i].st) != 0 || std::memcmp(a[i].bones, b[i].bones, sizeof a[i].bones) != 0 ||
            std::memcmp(a[i].weights, b[i].weights, sizeof a[i].weights) != 0) {
            return false;
        }
    }
    return true;
}

bool modellGleich(const GlmModel& a, const GlmModel& b, std::string& wo) {
    if (a.name != b.name || a.animFile != b.animFile || a.numBones != b.numBones || a.numLods != b.numLods ||
        a.surfaces.size() != b.surfaces.size()) {
        wo = "Kopf";
        return false;
    }
    if (std::memcmp(a.mins, b.mins, sizeof a.mins) != 0 || std::memcmp(a.maxs, b.maxs, sizeof a.maxs) != 0) {
        wo = "Grenzen";
        return false;
    }
    for (std::size_t i = 0; i < a.surfaces.size(); ++i) {
        const GlmSurface& x = a.surfaces[i];
        const GlmSurface& y = b.surfaces[i];
        if (x.name != y.name || x.shader != y.shader || x.flags != y.flags || x.parent != y.parent || x.indexes != y.indexes ||
            !vertsGleich(x.verts, y.verts) || x.grobIndexes != y.grobIndexes || !vertsGleich(x.grobVerts, y.grobVerts)) {
            wo = "Flaeche " + std::to_string(i) + " " + x.name;
            return false;
        }
    }
    return true;
}

Ecke ecke(float x, float y, float z, float u = 0.0F) {
    Ecke e;
    e.ort[0] = x;
    e.ort[1] = y;
    e.ort[2] = z;
    e.uv[0] = u;
    e.gewicht[0] = 1.0F;
    return e;
}

// Ein Teil mit einem Dreieck (fuer die Hierarchie-Proben).
Teil teil(const std::string& name, int eltern, std::uint32_t flaggen) {
    Teil t;
    t.name = name;
    t.eltern = eltern;
    t.flaggen = flaggen;
    t.stufen.resize(1);
    t.stufen[0].aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(0, 1, 0)}, {0, 1, 2});
    return t;
}

std::string an(const std::vector<char>& v) {
    std::string s;
    for (const char c : v) { s += c != 0 ? '1' : '0'; }
    return s;
}

}  // namespace

int main(int argc, char** argv) {
    // --- Halbkanten an kleinen Netzen ------------------------------------------
    {
        Netz n;   // Viereck aus zwei Dreiecken, die Diagonale geteilt
        n.aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(1, 1, 0), ecke(0, 1, 0)}, {0, 1, 2, 0, 2, 3});
        std::string f;
        expect("Viereck: Invarianten", n.pruefen(&f));
        expect("Viereck: 4 Randkanten, Diagonale 2-0 / 0-2 gepaart", n.randKanten() == 4 && n.kanten[2].gegen == 3 && n.kanten[3].gegen == 2);
        // Naht: dieselben Orte als eigene Ecken (andere UV) - gleicher Punkt, also verbunden.
        Netz naht;
        naht.aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(1, 1, 0), ecke(0, 0, 0, 0.5F), ecke(1, 1, 0, 0.5F), ecke(0, 1, 0)}, {0, 1, 2, 3, 4, 5});
        expect("Naht: 6 Ecken, 4 Punkte, Diagonale gepaart", naht.punktZahl == 4 && naht.randKanten() == 4 && naht.pruefen(&f));
        const std::vector<char> rand = naht.randEcken();
        expect("Naht: alle Ecken am Rand (offenes Viereck)", an(rand) == "111111");
        // Tetraeder: geschlossen, keine Randkante, keine Randecke.
        Netz tet;
        tet.aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(0, 1, 0), ecke(0, 0, 1)}, {0, 2, 1, 0, 1, 3, 1, 2, 3, 2, 0, 3});
        expect("Tetraeder: geschlossen", tet.randKanten() == 0 && tet.pruefen(&f) && an(tet.randEcken()) == "0000");
        // Drei Dreiecke an einer Kante: eine Kante bleibt offen und zaehlt.
        Netz drei;
        drei.aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(0, 1, 0), ecke(0, -1, 0), ecke(0, 0, 1)}, {0, 1, 2, 1, 0, 3, 0, 1, 4});
        expect("drei Dreiecke an einer Kante: 1 nicht mannigfaltig", drei.nichtMannigfaltig == 1 && drei.pruefen(&f));
        // Ungueltiger Index wird 0 (wie readGlm).
        Netz kaputt;
        kaputt.aufbauen({ecke(0, 0, 0), ecke(1, 0, 0), ecke(0, 1, 0)}, {0, 1, 7});
        expect("ungueltiger Index -> 0, entartetes Dreieck ohne Nachbarn", kaputt.indizes() == std::vector<std::uint32_t>({0, 1, 0}) && kaputt.pruefen(&f) && kaputt.randKanten() == 3);
        // Eine kaputte Halbkante faellt auf.
        Netz falsch = n;
        falsch.kanten[1].gegen = 0;
        expect("Invariantenpruefung meldet eine falsche Gegenkante", !falsch.pruefen(&f));
    }

    // --- Rundlauf mit einer echten .glm ---------------------------------------------
    if (argc < 2) {
        std::printf("  FEHL  kein model.glm angegeben\n");
        return 1;
    }
    const std::string bytes = slurp(argv[1]);
    Inhalt in;
    std::string fehler;
    expect("ausGlm liest " + std::string(argv[1]), ausGlm(bytes, in, &fehler));
    expect("vier Stufen, 80 Flaechen", in.stufenZahl == 4 && in.teile.size() == 80U);
    for (int s = 0; s < in.stufenZahl; ++s) {
        GlmModel direkt;
        (void)readGlm(bytes, direkt, nullptr, s);
        const GlmModel ueber = zuGlm(in, s);
        std::string wo;
        expect("Rundlauf Stufe " + std::to_string(s) + " bitgleich", modellGleich(direkt, ueber, wo));
        if (!wo.empty()) { std::printf("        Abweichung: %s\n", wo.c_str()); }
    }
    GlmModel jenseits;
    expect("Stufe hinter der letzten wird abgewiesen", !readGlm(bytes, jenseits, nullptr, in.stufenZahl));
    int netze = 0;
    int heil = 0;
    int nichtM = 0;
    int stufe0Dreiecke = 0;
    int stufe3Dreiecke = 0;
    for (const Teil& t : in.teile) {
        for (std::size_t s = 0; s < t.stufen.size(); ++s) {
            ++netze;
            heil += t.stufen[s].pruefen(&fehler) ? 1 : 0;
            nichtM += t.stufen[s].nichtMannigfaltig;
            if (s == 0) { stufe0Dreiecke += t.stufen[s].dreiecke(); }
            if (s == 3) { stufe3Dreiecke += t.stufen[s].dreiecke(); }
        }
    }
    std::printf("        %d Netze, Dreiecke Stufe 0: %d, Stufe 3: %d, nicht mannigfaltig: %d\n", netze, stufe0Dreiecke, stufe3Dreiecke, nichtM);
    expect("Halbkanten-Invarianten in allen " + std::to_string(netze) + " Netzen", heil == netze);
    expect("groebere Stufe hat weniger Dreiecke", stufe3Dreiecke > 0 && stufe3Dreiecke < stufe0Dreiecke);
    {
        int kinder = 0;
        for (const Teil& t : in.teile) { kinder += static_cast<int>(t.kinder.size()); }
        int mitEltern = 0;
        for (const Teil& t : in.teile) { mitEltern += t.eltern >= 0 ? 1 : 0; }
        expect("Kinderlisten passen zu den Eltern", kinder == mitEltern);
    }

    // --- Undo ---------------------------------------------------------------------------
    {
        Dokument d;
        d.laden(in);
        const int st0 = d.stand();
        const GlmModel vorher = zuGlm(d.inhalt(), 0);
        d.aendern("verschieben", [](Inhalt& x) {
            for (Ecke& e : x.teile[1].stufen[0].ecken) { e.ort[2] += 10.0F; }
        });
        const float z1 = d.inhalt().teile[1].stufen[0].ecken[0].ort[2];
        expect("aendern: Stand gezaehlt, Name gemerkt", d.stand() == st0 + 1 && d.rueckgaengigName() == "verschieben");
        expect("rueckgaengig", d.rueckgaengig());
        std::string wo;
        expect("rueckgaengig ergibt bitgleich den Stand vorher", modellGleich(vorher, zuGlm(d.inhalt(), 0), wo));
        expect("wiederholen", d.wiederholen() && d.inhalt().teile[1].stufen[0].ecken[0].ort[2] == z1);
        expect("nichts mehr zu wiederholen", !d.wiederholen() && !d.kannWiederholen());
        for (int i = 0; i < 40; ++i) {
            d.aendern("Schritt", [](Inhalt& x) { x.teile[0].stufen[0].ecken[0].ort[0] += 1.0F; });
        }
        int zurueck = 0;
        while (d.rueckgaengig()) { ++zurueck; }
        expect("Tiefe: hoechstens 32 Schritte zurueck", zurueck == static_cast<int>(Dokument::kTiefe));
        d.aendern("neu", [](Inhalt& x) { x.name = "neu"; });
        expect("neue Aenderung leert Wiederholen", !d.kannWiederholen());
    }

    // --- G2: welche Flaechen gezeichnet werden ---------------------------------------
    {
        Inhalt h;
        h.herkunft = Herkunft::Glm;
        h.stufenZahl = 1;
        h.teile.push_back(teil("hips", -1, 0));             // 0 Wurzel
        h.teile.push_back(teil("torso", 0, 0));             // 1
        h.teile.push_back(teil("head", 1, 0));              // 2
        h.teile.push_back(teil("torso_cap_off", 1, kG2Aus)); // 3
        h.teile.push_back(teil("*r_hand", 1, kG2Bolzen));   // 4
        h.teile.push_back(teil("zweite_wurzel", -1, 0));    // 5 (eigene Wurzel: nicht unter 0)
        for (std::size_t i = 0; i < h.teile.size(); ++i) {
            const int p = h.teile[i].eltern;
            if (p >= 0) { h.teile[static_cast<std::size_t>(p)].kinder.push_back(static_cast<int>(i)); }
        }
        G2Zustand z;
        expect("Vorgabe: Kappe aus, Tag nie, zweite Wurzel nicht (RenderSurfaces ab 0)", an(gezeichnet(h, z)) == "111000");
        expect("Flaeche aus", setzeFlaecheAnAus(h, z, "torso", kG2Aus) && an(gezeichnet(h, z)) == "101000");
        expect("aus ohne Kinder", setzeFlaecheAnAus(h, z, "TORSO", kG2Aus | kG2OhneKinder) && an(gezeichnet(h, z)) == "100000" && z.slist.size() == 1U);
        expect("wieder an (Eintrag bleibt, Flaggen 0)", setzeFlaecheAnAus(h, z, "torso", 0) && an(gezeichnet(h, z)) == "111000" && z.slist.size() == 1U);
        // mb1a2: G2_IsSurfaceLegal sucht den Namen aus R_LoadMDXM - "_off" ist dort gestrichen
        expect("Name mit _off findet die Flaeche nicht (R_LoadMDXM streicht es)", !setzeFlaecheAnAus(h, z, "torso_cap_off", 0) && spielName(h, 3) == "torso_cap");
        expect("Kappe an", setzeFlaecheAnAus(h, z, "torso_cap", 0) && an(gezeichnet(h, z)) == "111100");
        expect("Tag an bleibt ungezeichnet (ISBOLT bleibt)", setzeFlaecheAnAus(h, z, "*r_hand", 0) && an(gezeichnet(h, z)) == "111100");
        expect("unbekannte Flaeche", !setzeFlaecheAnAus(h, z, "gibtsnicht", kG2Aus));
        expect("Wurzel torso", setzeWurzel(h, z, "torso") && an(gezeichnet(h, z)) == "011100");
        expect("Wurzel zweite_wurzel", setzeWurzel(h, z, "zweite_wurzel") && an(gezeichnet(h, z)) == "000001");
        expect("unbekannte Wurzel aendert nichts", !setzeWurzel(h, z, "nichts") && z.wurzel == 5);
        z.wurzel = 0;
        hautAnwenden(h, z, {"head", "torso_cap"});
        expect("Haut: Ueberschreibungen weg, head aus, Kappe bleibt aus ohne Eintrag", an(gezeichnet(h, z)) == "110000" && z.slist.size() == 1U);
        // mb1a2: was der Spielcode beim Erzeugen schaltet
        Inhalt d = h;
        d.teile.push_back(teil("force_shield_off", 0, 0));   // 6: Droideka (Movie Duels), Flagge 0
        d.teile[0].kinder.push_back(6);
        SpawnRegeln r;
        expect("force_shield_off mit Flagge 0: RenderSurfaces zeichnet es (OpenJK)", an(gezeichnet(d, spawnZustand(d, {}, r))) == "1110001");
        r.movieDuelsSp = true;
        expect("Movie Duels SP: g_client.cpp:2381 schaltet force_shield mit 0x100 ab", an(gezeichnet(d, spawnZustand(d, {}, r))) == "1110000");
        r.movieDuelsSp = false;
        r.surfOff = npcFlaechenListe("head torso_cap");
        r.surfOn = npcFlaechenListe("torso_cap");
        expect(".npc surfOff head, dann surfOn torso_cap", an(gezeichnet(d, spawnZustand(d, {}, r))) == "1101001");
        expect("zwei surfOff-Zeilen (mit Komma verbunden) sind ein Wort wie COM_ParseExt", npcFlaechenListe("a b,c	d") == std::vector<std::string>({"a", "b,c", "d"}));
        SpawnRegeln schl;
        d.teile.push_back(teil("l_arm_key", 0, kG2Aus));     // 7
        d.teile[0].kinder.push_back(7);
        expect("l_arm_key ohne Schluessel aus", an(gezeichnet(d, spawnZustand(d, {}, schl))).back() == '0');
        schl.imperialMitSchluessel = true;
        expect("CLASS_IMPERIAL mit message: l_arm_key an", an(gezeichnet(d, spawnZustand(d, {}, schl))).back() == '1');
        expect("Haut vor surfOn: surfOn head schaltet die Haut-Flaeche wieder an", [&] {
            SpawnRegeln o;
            o.surfOn = {"head"};
            return an(gezeichnet(d, spawnZustand(d, {"head"}, o)))[2] == '1';
        }());
    }

    // --- LOD wie G2_ComputeLOD ---------------------------------------------------------
    {
        LodLage l;
        l.stufenZahl = 4;
        l.projektion5 = 1.0F;   // fovY 90
        l.lodscale = 5.0F;
        l.radius = 64.0F;
        const auto bei = [&](float d, int bias = 0) {
            LodLage x = l;
            x.abstand = d;
            x.lodbias = bias;
            return lodWieImSpiel(x);
        };
        expect("MP: 240 -> 0, 480 -> 2, 960 -> 3, 10000 -> 3", bei(240) == 0 && bei(480) == 2 && bei(960) == 3 && bei(10000) == 3);
        expect("r_lodbias 1 bei 480 -> 3, Bias ueber der Zahl -> letzte", bei(480, 1) == 3 && bei(100, 5) == 3);
        expect("hinter der Kamera: Stufe 0", bei(-50) == 0);
        LodLage sp = l;
        sp.radius = 0.0F;   // SP: CG_Player legt den refEntity mit memset an
        sp.abstand = 5000.0F;
        expect("SP-Figur (Radius 0): immer Stufe 0", lodWieImSpiel(sp) == 0);
        LodLage eins = l;
        eins.stufenZahl = 1;
        eins.lodbias = 3;
        expect("eine Stufe: 0 auch mit Bias", lodWieImSpiel(eins) == 0);
        expect("ProjectRadius: hoechstens 1", std::fabs(projiziereRadius(1000.0F, 10.0F, 1.0F) - 1.0F) < 1e-6F);
    }

    // --- Grenzen knapp unter und ueber ----------------------------------------------
    {
        const Grenzen& jka = grenzen(Zielspiel::JkaSp);
        const Grenzen& md = grenzen(Zielspiel::MdSp);
        const auto befunde = [](int ecken, int dreiecke, int knochen, int gewichte, const Grenzen& g) {
            FlaechenMass m;
            m.ecken = ecken;
            m.dreiecke = dreiecke;
            m.knochen = knochen;
            m.gewichteMax = gewichte;
            return pruefeMass(m, g);
        };
        expect("JKA: 999 Ecken ok, 1000 nicht", befunde(999, 1, 1, 1, jka).empty() && befunde(1000, 1, 1, 1, jka).size() == 1U &&
                                                    befunde(1000, 1, 1, 1, jka)[0].art == BefundArt::Ecken);
        expect("MD: 3999 Ecken ok, 4000 nicht", befunde(3999, 1, 1, 1, md).empty() && befunde(4000, 1, 1, 1, md).size() == 1U);
        expect("JKA: 5999 Dreiecke ok, 6000 nicht", befunde(1, 5999, 1, 1, jka).empty() && befunde(1, 6000, 1, 1, jka)[0].art == BefundArt::Dreiecke);
        expect("32 Knochen ok, 33 nicht", befunde(1, 1, 32, 1, jka).empty() && befunde(1, 1, 33, 1, jka)[0].art == BefundArt::Knochen);
        expect("4 Gewichte ok, 5 nicht", befunde(1, 1, 1, 4, jka).empty() && befunde(1, 1, 1, 5, jka)[0].art == BefundArt::Gewichte);
        // Das Mass eines Netzes: 33 verschiedene Knochen, Ecken mit 4 Gewichten.
        Netz n;
        std::vector<Ecke> e;
        for (int i = 0; i < 33; ++i) {
            Ecke x = ecke(static_cast<float>(i), 0, 0);
            x.knochen[0] = static_cast<std::uint8_t>(i);
            x.knochen[1] = static_cast<std::uint8_t>((i + 1) % 33);
            x.gewicht[0] = 0.25F;
            x.gewicht[1] = 0.25F;
            x.gewicht[2] = 0.25F;
            x.gewicht[3] = 0.25F;
            e.push_back(x);
        }
        n.aufbauen(e, {0, 1, 2});
        const FlaechenMass m = mass(n);
        expect("Mass: 33 Ecken, 1 Dreieck, 33 Knochen, 4 Gewichte", m.ecken == 33 && m.dreiecke == 1 && m.knochen == 33 && m.gewichteMax == 4);
        Inhalt in2;
        in2.herkunft = Herkunft::Glm;
        in2.stufenZahl = 1;
        Teil t;
        t.name = "viele_knochen";
        t.stufen.push_back(n);
        in2.teile.push_back(t);
        const std::vector<AnimEntry> anims = {{"BOTH_STAND1", 90, 10, -1, 20}, {"BOTH_CIN_16", 90, 11, -1, 20}, {"HINTEN", 100, 1, -1, 20}};
        const std::vector<Befund> b = pruefeModell(in2, Zielspiel::JkaSp, anims, 100, 513, {0}, {1});
        int knochen = 0, anim = 0, g2 = 0, shader = 0;
        for (const Befund& x : b) {
            knochen += x.art == BefundArt::Knochen ? 1 : 0;
            anim += x.art == BefundArt::AnimHinterGla ? 1 : 0;
            g2 += x.art == BefundArt::G2Modelle ? 1 : 0;
            shader += x.art == BefundArt::ShaderFehlt ? 1 : 0;
        }
        expect("Modell: 33 Knochen, 2 Abschnitte hinter der GLA (100 Bilder), 513 Instanzen, fehlendes Bild", knochen == 1 && anim == 2 && g2 == 1 && shader == 1);
        expect("MAX_G2_MODELS: JKA SP 512 ok, MP 1024 ok, 1025 nicht",
               pruefeModell(in2, Zielspiel::JkaSp, {}, 0, 512, {}, {}).size() == 1U &&
                   pruefeModell(in2, Zielspiel::JkaMp, {}, 0, 1024, {}, {}).size() == 1U &&
                   pruefeModell(in2, Zielspiel::JkaMp, {}, 0, 1025, {}, {}).size() == 2U);
        expect("animInnerhalb: 90+10 von 100 ja, 90+11 nein", animInnerhalb({"a", 90, 10, -1, 20}, 100) && !animInnerhalb({"a", 90, 11, -1, 20}, 100));
    }

    // --- MD3 -------------------------------------------------------------------------------
    {
        Md3Model m;
        m.name = "kiste";
        m.numFrames = 1;
        Md3Surface s;
        s.name = "seite";
        s.shader = "textures/kiste";
        s.numVerts = 3;
        s.numFrames = 1;
        s.verts.resize(3);
        s.verts[1].xyz[0] = 8.0F;
        s.verts[2].xyz[1] = 8.0F;
        s.indexes = {0, 1, 2};
        m.surfaces.push_back(s);
        m.surfaces.push_back(s);
        m.surfaces[1].name = "deckel";
        Inhalt md;
        ausMd3(m, md);
        const GlmModel g = zuGlm(md, 0);
        expect("MD3: zwei Teile, starr an Knochen 0", md.herkunft == Herkunft::Md3 && g.surfaces.size() == 2U && g.surfaces[0].verts.size() == 3U &&
                                                         g.surfaces[0].verts[1].xyz[0] == 8.0F && g.surfaces[0].verts[1].weights[0] == 1.0F &&
                                                         g.surfaces[0].shader == "textures/kiste" && g.numLods == 0);
        G2Zustand z;
        z.wurzel = 1;
        expect("MD3: jede Flaeche gezeichnet (kein Ghoul2)", an(gezeichnet(md, z)) == "11");
    }

    // --- MRESTE 4: Ghoul2-Kartenmodell ohne Animation (model2, misc_model_ghoul) -------
    // Zwei Knochen: 0 (Wurzel, Bild 0 verschiebt um +10 x) und 1 (Kind, Grundstellung +4 z, Bild 0 dreht 90 Grad um z).
    // Erwartet nach RB_SurfaceGhoul: Ort = Summe Gewicht * (Bild-0-Knochen * Grundstellung^-1 * Ecke), Normale nur mit
    // dem ersten Knochen, danach rootMatrix (x, y, z) -> (-y, x, z); Flaeche mit G2SURFACEFLAG_OFF und Tag fehlen.
    {
        GlaAnimation gla;
        gla.numFrames = 2;
        gla.bones.resize(2);
        gla.bones[0].name = "model_root";
        gla.bones[0].basePose = BoneMatrix::identity();
        gla.bones[0].basePoseInv = BoneMatrix::identity();
        gla.bones[1].name = "arm";
        gla.bones[1].parent = 0;
        gla.bones[1].basePose = BoneMatrix::identity();
        gla.bones[1].basePose.m[2][3] = 4.0F;
        gla.bones[1].basePoseInv = BoneMatrix::identity();
        gla.bones[1].basePoseInv.m[2][3] = -4.0F;
        gla.roh.assign(4, BoneMatrix::identity());
        gla.roh[0].m[0][3] = 10.0F;   // Bild 0, Knochen 0: +10 x
        gla.roh[1].m[0][0] = 0.0F;    // Bild 0, Knochen 1: 90 Grad um z, am Ort +4 z
        gla.roh[1].m[0][1] = -1.0F;
        gla.roh[1].m[1][0] = 1.0F;
        gla.roh[1].m[1][1] = 0.0F;
        gla.roh[1].m[2][3] = 4.0F;
        gla.roh[2].m[0][3] = 99.0F;   // Bild 1 darf nicht gelten
        GlmModel g;
        g.numBones = 2;
        GlmSurface teil;
        teil.name = "kiste";
        teil.shader = "models/map_objects/test/kiste";
        teil.verts.resize(3);
        teil.verts[0].xyz[0] = 1.0F;
        teil.verts[0].weights[0] = 1.0F;
        teil.verts[0].normal[0] = 1.0F;
        teil.verts[1].xyz[1] = 2.0F;
        teil.verts[1].xyz[2] = 4.0F;
        teil.verts[1].bones[0] = 1;
        teil.verts[1].weights[0] = 1.0F;
        teil.verts[1].normal[0] = 1.0F;
        teil.verts[2].xyz[2] = 4.0F;   // halb und halb
        teil.verts[2].bones[0] = 0;
        teil.verts[2].bones[1] = 1;
        teil.verts[2].weights[0] = 0.5F;
        teil.verts[2].weights[1] = 0.5F;
        teil.verts[2].normal[1] = 1.0F;
        teil.indexes = {0, 1, 2};
        g.surfaces.push_back(teil);
        GlmSurface aus = teil;
        aus.name = "kappe_off";
        aus.parent = 0;
        aus.flags = GlmSurface::kFlagOff;
        g.surfaces.push_back(aus);
        GlmSurface bolzen = teil;
        bolzen.name = "*flash";
        bolzen.parent = 0;
        bolzen.flags = GlmSurface::kFlagBolt;
        g.surfaces.push_back(bolzen);
        Md3Model m;
        const bool ok = glmStandbild(g, gla, m);
        const auto nah = [](const float* a, float x, float y, float z) {
            return std::fabs(a[0] - x) < 1e-5F && std::fabs(a[1] - y) < 1e-5F && std::fabs(a[2] - z) < 1e-5F;
        };
        expect("Standbild: ein Bild, nur die gezeichnete Flaeche (OFF und Tag fehlen), Shader der Datei",
               ok && m.numFrames == 1 && m.surfaces.size() == 1U && m.surfaces[0].name == "kiste" &&
                   m.surfaces[0].shader == teil.shader && m.surfaces[0].numVerts == 3 && m.surfaces[0].indexes == teil.indexes &&
                   m.surfaces[0].dateiFlaeche == 0);
        if (ok && m.surfaces.size() == 1U && m.surfaces[0].verts.size() == 3U) {
            const std::vector<Md3Vertex>& v = m.surfaces[0].verts;
            // Ecke 0 an Knochen 0: (1,0,0) + 10 x = (11,0,0), Wurzel -> (0,11,0)
            expect("Standbild: Bild 0 der GLA und Wurzel ((1,0,0) -> (0,11,0))", nah(v[0].xyz, 0.0F, 11.0F, 0.0F));
            // Ecke 1 an Knochen 1: X(1) = X(0) * B(0)^-1 * A(1) * B(1), Haut = X(1) * B(1)^-1 = (+10 x) * A(1):
            // (0,2,4) gedreht (-2,0,4), +4 z, +10 x = (8,0,8), Wurzel -> (0,8,8)
            expect("Standbild: Kindknochen ueber die Kette ((0,2,4) -> (0,8,8))", nah(v[1].xyz, 0.0F, 8.0F, 8.0F));
            // Ecke 2: 0,5 * (10,0,4) + 0,5 * (10,0,8) = (10,0,6), Wurzel -> (0,10,6)
            expect("Standbild: zwei Gewichte gemischt ((0,0,4) -> (0,10,6))", nah(v[2].xyz, 0.0F, 10.0F, 6.0F));
            // Normalen: Ecke 0 (1,0,0) -> (0,1,0); Ecke 1 nur Knochen 1: (1,0,0) -> (0,1,0) -> (-1,0,0); Ecke 2 nur Knochen 0: (0,1,0) -> (-1,0,0)
            expect("Standbild: Normale mit dem ersten Knochen und der Wurzel", nah(v[0].normal, 0.0F, 1.0F, 0.0F) &&
                                                                                    nah(v[1].normal, -1.0F, 0.0F, 0.0F) &&
                                                                                    nah(v[2].normal, -1.0F, 0.0F, 0.0F));
        }
        GlmModel falsch = g;
        falsch.numBones = 3;
        expect("Standbild: andere Knochenzahl als die GLA -> nichts (R_LoadMDXM)", !glmStandbild(falsch, gla, m) && m.surfaces.empty());
        expect("Standbild: ohne GLA -> nichts (missing animation file)", !glmStandbild(g, GlaAnimation{}, m));
        // "*default": die eingebaute FakeGLAFile - ein Knochen, ein Bild, Bild 0 (fast) die Einheit
        const GlaAnimation& st = standardGla();
        GlmModel eins;
        eins.numBones = 1;
        GlmSurface e1 = teil;
        e1.verts.resize(1);
        e1.verts[0] = GlmVertex{};
        e1.verts[0].xyz[0] = 1.0F;
        e1.verts[0].xyz[1] = 2.0F;
        e1.verts[0].xyz[2] = 3.0F;
        e1.verts[0].weights[0] = 1.0F;
        e1.indexes = {0, 0, 0};
        eins.surfaces.push_back(e1);
        const bool okSt = glmStandbild(eins, st, m);
        expect("*default: FakeGLAFile mit einem Knochen und einem Bild, (1,2,3) -> (-2,1,3)",
               st.bones.size() == 1U && st.numFrames == 1 && okSt && m.surfaces.size() == 1U &&
                   std::fabs(m.surfaces[0].verts[0].xyz[0] + 2.0F) < 1e-3F && std::fabs(m.surfaces[0].verts[0].xyz[1] - 1.0F) < 1e-3F &&
                   std::fabs(m.surfaces[0].verts[0].xyz[2] - 3.0F) < 1e-3F);
    }

    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
