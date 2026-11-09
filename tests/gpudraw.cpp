// Probe fuer die Zeichenliste.
//
// Datenfrei: das Netz wird hier von Hand gebaut, damit die Probe ohne .bsp
// und ohne pk3 laeuft. Was sie prueft, ist die REIHENFOLGE und das
// Zusammenfassen - genau die zwei Dinge, die im Bild als "Rauch hinter dem
// Felsen" oder "Lava durch die Wand" auffallen.

#include "bhed/gpudraw.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace bhed;
using namespace bhed::gpu;

namespace {

int fehler = 0;

void erwarte(const char* was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was);
    if (!ok) {
        ++fehler;
    }
}

// Ein Texturensatz mit ein paar Eintraegen: deckend, additiv, alpha.
TextureSet baueTexturen() {
    TextureSet ts;
    ts.byShader.resize(4);
    for (auto& t : ts.byShader) {
        // Ein Eintrag gilt nur als brauchbar, wenn er Bildpunkte hat.
        t.width = 2;
        t.height = 2;
        t.rgba.assign(16, 255);
        t.buildMips();
    }
    // 0: deckend
    ts.byShader[0].srcFactor = BlendFactor::One;
    ts.byShader[0].dstFactor = BlendFactor::Zero;
    // 1: additiv
    ts.byShader[1].blend = BlendMode::Add;
    ts.byShader[1].srcFactor = BlendFactor::One;
    ts.byShader[1].dstFactor = BlendFactor::One;
    // 2: alpha
    ts.byShader[2].blend = BlendMode::Alpha;
    ts.byShader[2].srcFactor = BlendFactor::SrcAlpha;
    ts.byShader[2].dstFactor = BlendFactor::OneMinusSrcAlpha;
    // 3: deckend, aber mit tcMod - also ein ANDERER Zustand
    ts.byShader[3].srcFactor = BlendFactor::One;
    ts.byShader[3].dstFactor = BlendFactor::Zero;
    ts.byShader[3].numTexMods = 1;
    ts.byShader[3].texMods[0].kind = TexModKind::Scroll;
    return ts;
}

// Derselbe Satz, aber Shader 0 traegt eine ZUSATZSTUFE: das additive
// Leuchten (Platz 1). So sehen die Bildschirme in md_am_sith aus.
TextureSet baueTexturenMitStufe() {
    TextureSet ts = baueTexturen();
    ts.byShader[0].extraStages.push_back(1);
    return ts;
}

BspMesh baueNetz(const std::vector<int>& shaderJeStapel) {
    BspMesh m;
    std::uint32_t at = 0;
    for (const int sh : shaderJeStapel) {
        BspMesh::Batch b;
        b.shader = sh;
        b.lightmap = -1;
        b.firstIndex = at;
        b.numIndexes = 3;
        at += 3;
        m.batches.push_back(b);
    }
    m.indexes.assign(at, 0);
    return m;
}

}  // namespace

int main() {
    std::printf("gpudraw\n");
    const TextureSet ts = baueTexturen();

    // --- Deckend zuerst, gemischt danach ---------------------------------
    //
    // Absichtlich in der falschen Reihenfolge eingespeist.
    {
        const BspMesh m = baueNetz({1, 0, 2, 0});
        const auto calls = buildDrawCalls(m, &ts, 0.0F);
        erwarte("alle vier Stapel kommen an", calls.size() == 4);
        bool gemischtGesehen = false;
        bool deckendNachGemischt = false;
        for (const auto& c : calls) {
            if (c.state.blend != Blend::Opaque) {
                gemischtGesehen = true;
            } else if (gemischtGesehen) {
                deckendNachGemischt = true;
            }
        }
        erwarte("kein deckender Stapel nach einem gemischten",
                !deckendNachGemischt);
    }

    // --- Unter den GEMISCHTEN bleibt die Reihenfolge der Karte ------------
    //
    // Additiv ist vertauschbar, Alpha nicht. Wer hier umsortiert, aendert
    // das Bild - und zwar an genau den Stellen, an denen es am meisten
    // auffaellt.
    {
        const BspMesh m = baueNetz({2, 1, 2});
        const auto calls = buildDrawCalls(m, &ts, 0.0F);
        erwarte("drei gemischte Stapel", calls.size() == 3);
        erwarte("Reihenfolge der Karte bleibt",
                calls[0].batch == 0 && calls[1].batch == 1 &&
                    calls[2].batch == 2);
    }

    // --- Deckende werden nach Zustand gruppiert ---------------------------
    {
        const BspMesh m = baueNetz({0, 3, 0, 3});
        const auto calls = buildDrawCalls(m, &ts, 0.0F);
        erwarte("vier deckende Stapel", calls.size() == 4);
        // 0,0,3,3 statt 0,3,0,3 - sonst vier Zustandswechsel statt zwei.
        erwarte("gleiche Zustaende liegen beieinander",
                calls[0].state.gleichWie(calls[1].state) &&
                    calls[2].state.gleichWie(calls[3].state) &&
                    !calls[1].state.gleichWie(calls[2].state));
        erwarte("also zwei Zustandswechsel statt vier",
                countStateChanges(calls) == 2);
    }

    // --- Ein Stapel ohne Textur wird UEBERSPRUNGEN ------------------------
    //
    // Nicht durch einen Ersatz gezeichnet. In rc367 hat ein Rueckfall auf
    // "Platz 0" riesige Lavavierecke ins Bild gemalt, weil Platz 0 zufaellig
    // die Lavatextur war.
    {
        TextureSet leer = ts;
        leer.byShader[1].rgba.clear();
        leer.byShader[1].width = 0;
        leer.byShader[1].height = 0;
        const BspMesh m = baueNetz({0, 1, 0});
        const auto calls = buildDrawCalls(m, &leer, 0.0F);
        erwarte("der Stapel ohne Bild faellt weg", calls.size() == 2);
        for (const auto& c : calls) {
            erwarte("und keiner zeigt auf den fehlenden Platz", c.bild != 1);
        }
    }

    // --- Leere Stapel kommen gar nicht erst in die Liste -------------------
    {
        BspMesh m = baueNetz({0, 0});
        m.batches[0].numIndexes = 0;
        const auto calls = buildDrawCalls(m, &ts, 0.0F);
        erwarte("ein Stapel ohne Indizes faellt weg", calls.size() == 1);
    }

    // --- Ohne Texturensatz gibt es nichts zu zeichnen ----------------------
    {
        const BspMesh m = baueNetz({0, 1});
        erwarte("kein Texturensatz -> leere Liste",
                buildDrawCalls(m, nullptr, 0.0F).empty());
    }

    // --- Der Gluehdurchgang ------------------------------------------------
    //
    // Er nimmt dieselben Stapel, aber nur die leuchtenden - und in
    // DERSELBEN Reihenfolge. Wuerde er umsortieren, leuchtete etwas an einer
    // Stelle, an der es im Hauptbild verdeckt ist.
    {
        TextureSet mitGlow = ts;
        mitGlow.byShader[0].glow = true;   // deckend UND leuchtend
        mitGlow.byShader[2].glow = true;   // alpha UND leuchtend
        const BspMesh m = baueNetz({0, 1, 2, 0});
        const auto alle = buildDrawCalls(m, &mitGlow, 0.0F);
        const auto gluehend = nurGluehende(alle);
        erwarte("drei von vier gluehen", gluehend.size() == 3);
        for (const auto& c : gluehend) {
            erwarte("  und jeder traegt das Bit",
                    (c.state.features & kGlow) != 0U);
        }
        // Die Reihenfolge muss die der vollen Liste sein.
        std::size_t j = 0;
        bool inOrdnung = true;
        for (const auto& c : alle) {
            if ((c.state.features & kGlow) != 0U) {
                if (j >= gluehend.size() || gluehend[j].batch != c.batch) {
                    inOrdnung = false;
                }
                ++j;
            }
        }
        erwarte("die Reihenfolge bleibt die der vollen Liste", inOrdnung);
    }
    {
        // Ohne leuchtende Stapel ist der Durchgang leer - und darf nicht
        // etwa alle zurueckgeben.
        const BspMesh m = baueNetz({0, 1, 2});
        erwarte("nichts glueht -> leere Liste",
                nurGluehende(buildDrawCalls(m, &ts, 0.0F)).empty());
    }

    // --- Die Lage: deckend gegen gemischt ---------------------------------
    //
    // Der Fehler, den das beheben soll: der GPU-Weg zeichnet Karte, dann
    // Mover. Beide Listen sind IN SICH richtig sortiert - und trotzdem
    // stehen die gemischten Kartenflaechen vor den deckenden Tueren.
    {
        // 0 deckend, 2 alpha, 3 deckend-mit-tcMod, 1 additiv
        const BspMesh m = baueNetz({0, 2, 3, 1});
        const std::vector<DrawCall> alle = buildDrawCalls(m, &ts, 0.0F);
        const std::vector<DrawCall> d = nurLage(alle, Lage::Deckend);
        const std::vector<DrawCall> g = nurLage(alle, Lage::Gemischt);

        erwarte("zwei deckende", d.size() == 2);
        erwarte("zwei gemischte", g.size() == 2);
        erwarte("zusammen wieder alle", d.size() + g.size() == alle.size());

        bool nurDeckend = true;
        for (const DrawCall& c : d) {
            if (c.state.blend != Blend::Opaque) { nurDeckend = false; }
        }
        erwarte("in der deckenden Lage ist nichts Gemischtes", nurDeckend);
        bool nurGemischt = true;
        for (const DrawCall& c : g) {
            if (c.state.blend == Blend::Opaque) { nurGemischt = false; }
        }
        erwarte("in der gemischten Lage ist nichts Deckendes", nurGemischt);

        // Die Reihenfolge INNERHALB einer Lage muss die der vollen Liste
        // sein. Bei den gemischten haengt das Bild daran.
        bool folge = true;
        std::size_t j = 0;
        for (const DrawCall& c : alle) {
            if (c.state.blend != Blend::Opaque) {
                if (j >= g.size() || g[j].batch != c.batch) { folge = false; }
                ++j;
            }
        }
        erwarte("gemischte behalten ihre Reihenfolge", folge && j == g.size());
    }
    {
        // Zwei Netze nacheinander - Karte und Mover. Die Lage muss die
        // Reihenfolge ueber BEIDE richtigstellen.
        const BspMesh karte = baueNetz({0, 2});     // deckend, alpha
        const BspMesh mover = baueNetz({0});        // deckend (die Tuer)
        const std::vector<DrawCall> ck = buildDrawCalls(karte, &ts, 0.0F);
        const std::vector<DrawCall> cm = buildDrawCalls(mover, &ts, 0.0F);

        // So lief es bisher: erst die ganze Karte, dann der Mover.
        std::vector<DrawCall> alt;
        alt.insert(alt.end(), ck.begin(), ck.end());
        alt.insert(alt.end(), cm.begin(), cm.end());
        bool deckendNachGemischt = false;
        bool schonGemischt = false;
        for (const DrawCall& c : alt) {
            if (c.state.blend != Blend::Opaque) { schonGemischt = true; }
            else if (schonGemischt) { deckendNachGemischt = true; }
        }
        erwarte("der alte Ablauf zeichnet deckend NACH gemischt (der Fehler)",
                deckendNachGemischt);

        // Und so soll es laufen: zwei Durchgaenge ueber beide Netze.
        std::vector<DrawCall> neu;
        for (const auto* l : {&ck, &cm}) {
            const std::vector<DrawCall> t = nurLage(*l, Lage::Deckend);
            neu.insert(neu.end(), t.begin(), t.end());
        }
        for (const auto* l : {&ck, &cm}) {
            const std::vector<DrawCall> t = nurLage(*l, Lage::Gemischt);
            neu.insert(neu.end(), t.begin(), t.end());
        }
        erwarte("gleich viele Aufrufe wie vorher", neu.size() == alt.size());
        bool sauber = true;
        schonGemischt = false;
        for (const DrawCall& c : neu) {
            if (c.state.blend != Blend::Opaque) { schonGemischt = true; }
            else if (schonGemischt) { sauber = false; }
        }
        erwarte("in zwei Lagen steht kein deckender mehr hinter einem "
                "gemischten", sauber);
    }
    {
        // Lage::Alles muss die Liste unveraendert lassen - der
        // Gluehdurchgang und der Vergleich haengen daran.
        const BspMesh m = baueNetz({0, 2, 1});
        const std::vector<DrawCall> alle = buildDrawCalls(m, &ts, 0.0F);
        const std::vector<DrawCall> a2 = nurLage(alle, Lage::Alles);
        bool gleich = (a2.size() == alle.size());
        for (std::size_t i = 0; gleich && i < alle.size(); ++i) {
            if (a2[i].batch != alle[i].batch) { gleich = false; }
        }
        erwarte("Lage::Alles aendert nichts", gleich);
        erwarte("leere Liste bleibt leer",
                nurLage({}, Lage::Deckend).empty());
    }

    // --- Die Stufen UEBER der ersten --------------------------------------
    //
    // Der GPU-Weg zeichnete bis rc435 nur die Grundstufe. Das additive
    // Leuchten der Bildschirme sitzt darueber - deshalb fand der
    // Gluehdurchgang nur zwei Aufrufe, waehrend der Rasterer 61529
    // Bildpunkte gluehen liess.
    {
        const TextureSet ts2 = baueTexturenMitStufe();
        const BspMesh m = baueNetz({0, 0});
        const std::vector<DrawCall> calls = buildDrawCalls(m, &ts2, 0.0F);
        erwarte("zwei Stapel mit je einer Zusatzstufe ergeben vier Aufrufe",
                calls.size() == 4);

        int zusatz = 0;
        for (const DrawCall& c : calls) {
            if (c.zusatzstufe) { ++zusatz; }
        }
        erwarte("zwei davon sind Zusatzstufen", zusatz == 2);

        // Die Zusatzstufe zeigt auf ihren EIGENEN Platz, nicht auf den des
        // Stapels - sonst zeichnete sie dasselbe Bild noch einmal.
        bool eigenesBild = true;
        for (const DrawCall& c : calls) {
            if (c.zusatzstufe && c.bild != 1) { eigenesBild = false; }
            if (!c.zusatzstufe && c.bild != 0) { eigenesBild = false; }
        }
        erwarte("die Zusatzstufe hat ihr eigenes Bild", eigenesBild);

        // Und sie liegt auf DERSELBEN Geometrie.
        bool gleicheEcken = true;
        for (const DrawCall& c : calls) {
            if (!c.zusatzstufe) { continue; }
            bool gefunden = false;
            for (const DrawCall& g : calls) {
                if (!g.zusatzstufe && g.batch == c.batch &&
                    g.firstIndex == c.firstIndex &&
                    g.numIndexes == c.numIndexes) {
                    gefunden = true;
                }
            }
            if (!gefunden) { gleicheEcken = false; }
        }
        erwarte("die Zusatzstufe liegt auf derselben Geometrie", gleicheEcken);

        // Das Wichtigste: die Grundstufe kommt VOR ihrer Zusatzstufe.
        bool reihenfolge = true;
        for (std::size_t i = 0; i < calls.size(); ++i) {
            if (!calls[i].zusatzstufe) { continue; }
            bool grundstufeDavor = false;
            for (std::size_t j = 0; j < i; ++j) {
                if (!calls[j].zusatzstufe && calls[j].batch == calls[i].batch) {
                    grundstufeDavor = true;
                }
            }
            if (!grundstufeDavor) { reihenfolge = false; }
        }
        erwarte("die Grundstufe kommt VOR ihrer Zusatzstufe", reihenfolge);
    }
    {
        // Die Zusatzstufe glueht (Platz 1 ist additiv, aber nicht als
        // gluehend gekennzeichnet) - hier zaehlt nur, dass der
        // Gluehdurchgang die Stufen ueberhaupt sehen KANN.
        TextureSet ts2 = baueTexturenMitStufe();
        ts2.byShader[1].glow = true;
        const BspMesh m = baueNetz({0});
        const std::vector<DrawCall> alle = buildDrawCalls(m, &ts2, 0.0F);
        const std::vector<DrawCall> g = nurGluehende(alle);
        erwarte("der Gluehdurchgang findet die gluehende ZUSATZSTUFE",
                g.size() == 1 && g[0].zusatzstufe);
    }
    {
        // rc608 Teil 11: Ein deckender Shader mit einer gemischten Zusatzstufe (wie u_metalorange02:
        // env, dann GL_SRC_ALPHA GL_SRC_ALPHA) und davor in der Karte eine additive Flaeche (env_glass).
        // Im Spiel ist der deckende Shader mit allen Stufen fertig, bevor das Glas addiert
        // (R_RadixSort nach Sortierstufe, RB_IterateStagesGeneric je Shader).
        TextureSet ts2 = baueTexturen();
        ts2.byShader[0].extraStages.push_back(2);
        const BspMesh m = baueNetz({1, 0});
        const std::vector<DrawCall> calls = buildDrawCalls(m, &ts2, 0.0F);
        erwarte("Glas vor Metall in der Karte: Grundstufe, Zusatzstufe, dann das Glas",
                calls.size() == 3 && !calls[0].zusatzstufe && calls[0].bild == 0 && calls[1].zusatzstufe && calls[1].bild == 2 &&
                    calls[2].bild == 1);
        const std::vector<DrawCall> d = nurLage(calls, Lage::Deckend);
        const std::vector<DrawCall> g = nurLage(calls, Lage::Gemischt);
        erwarte("die gemischte Zusatzstufe des deckenden Shaders zeichnet in der deckenden Lage",
                d.size() == 2 && d[1].zusatzstufe && g.size() == 1 && g[0].bild == 1);
    }
    {
        // Ohne Zusatzstufen aendert sich nichts - der alte Weg bleibt.
        const BspMesh m = baueNetz({0, 2, 1});
        erwarte("ohne Zusatzstufen bleibt die Liste wie bisher",
                buildDrawCalls(m, &ts, 0.0F).size() == 3);
    }

    // --- Die Zaehlung selbst ----------------------------------------------
    {
        erwarte("leere Liste hat null Wechsel",
                countStateChanges({}) == 0U);
        const BspMesh m = baueNetz({0});
        erwarte("ein Aufruf ist ein Zustand",
                countStateChanges(buildDrawCalls(m, &ts, 0.0F)) == 1U);
    }

    // --- REST3: autosprite auf der CPU (verformeTess): Mitte, Radius, Richtung, Wicklung ------------------------------------------
    //
    // Ein Viereck 40 breit, 100 hoch, stehend in der x-z-Ebene um (10,20,30) - wie das Palpatine-Hologramm. AutospriteDeform: Mitte,
    // Radius = |Ecke 0 - Mitte| * 0,707, Ecken mid +- left +- up entlang der Kameraachsen, Wicklung 0,1,3 / 3,1,2, st und Lightmap-st
    // 0..1, Normale -vorn der Welt, Farbe der ersten Ecke (RB_AddQuadStampExt).
    {
        DeformZusatz z;
        DeformEintrag e;
        e.art = kFolgeSprite;
        z.dazu(e);
        const float ecken[4][3] = {{-10, 20, -20}, {30, 20, -20}, {30, 20, 80}, {-10, 20, 80}};
        std::vector<TessEcke> te(4);
        for (int w = 0; w < 4; ++w) {
            for (int k = 0; k < 3; ++k) { te[static_cast<std::size_t>(w)].xyz[k] = ecken[w][k]; }
            te[static_cast<std::size_t>(w)].farbe[0] = static_cast<std::uint8_t>(10 * (w + 1));
        }
        std::vector<std::uint32_t> idx = {0, 1, 2, 0, 2, 3};
        SpriteSicht sicht;
        sicht.links[0] = 1.0F;   // links = +x, hoch = +z, vorn = +y (Mover: vornWelt anders als vorn)
        sicht.hoch[2] = 1.0F;
        sicht.vorn[1] = 1.0F;
        sicht.vornWelt[0] = 1.0F;
        erwarte("Autosprite: verformeTess rechnet", verformeTess(z, 0.0F, sicht, te, idx));
        const float r = std::sqrt(20.0F * 20.0F + 50.0F * 50.0F) * 0.707F;
        erwarte("Mitte des Vierecks, Radius = halbe Diagonale x 0,707, +links+hoch zuerst",
                te[0].xyz[0] == 10.0F + r && te[0].xyz[1] == 20.0F && te[0].xyz[2] == 30.0F + r && te[2].xyz[0] == 10.0F - r &&
                    te[2].xyz[2] == 30.0F - r);
        erwarte("st und Lightmap-st wie RB_AddQuadStampExt, Normale -vorn der Welt, Farbe der ersten Ecke",
                te[2].st[0] == 1.0F && te[2].st[1] == 1.0F && te[3].lm[0] == 0.0F && te[3].lm[1] == 1.0F && te[1].normal[0] == -1.0F &&
                    te[1].normal[1] == 0.0F && te[3].farbe[0] == 10U);
        erwarte("Wicklung 0,1,3 / 3,1,2", idx == std::vector<std::uint32_t>({0, 1, 3, 3, 1, 2}));
        sicht.spiegel = true;
        std::vector<TessEcke> te2(4);
        for (int w = 0; w < 4; ++w) {
            for (int k = 0; k < 3; ++k) { te2[static_cast<std::size_t>(w)].xyz[k] = ecken[w][k]; }
        }
        std::vector<std::uint32_t> idx2 = {0, 1, 2, 0, 2, 3};
        erwarte("im Spiegel left = -left (AutospriteDeform :400)", verformeTess(z, 0.0F, sicht, te2, idx2) && te2[0].xyz[0] == 10.0F - r);
        std::vector<TessEcke> drei(3);
        std::vector<std::uint32_t> idx3 = {0, 1, 2};
        erwarte("eine Flaeche, die nicht in Vierecke passt: das Spiel bricht ab (ERR_DROP), nichts veraendert",
                !verformeTess(z, 0.0F, sicht, drei, idx3) && idx3 == std::vector<std::uint32_t>({0, 1, 2}));
    }

    // --- autosprite2: Drehung um die LANGE Achse (REST3: auf der CPU wie Autosprite2Deform) ---------------------------------------
    //
    // Eine Kette aus kor1 (textures/korriban/chain), genau so in der .bsp: 8 breit, 32 hoch, Indizes 3,0,2 / 2,0,1 - die Reihenfolge
    // von q3map2. Mit einer anderen Indexfolge verdreht die Rechnung der Engine das Viereck - sie verlaesst sich auf diese. Mit dem
    // Blick von -y (vorn = 0,1,0) liegen die Ecken wieder wie gebaut.
    {
        DeformZusatz z;
        DeformEintrag e;
        e.art = kFolgeSprite2;
        z.dazu(e);
        const float ecken[4][3] = {{644, -32, -800}, {636, -32, -800}, {636, -32, -768}, {644, -32, -768}};
        std::vector<TessEcke> te(4);
        for (int w = 0; w < 4; ++w) {
            for (int k = 0; k < 3; ++k) { te[static_cast<std::size_t>(w)].xyz[k] = ecken[w][k]; }
        }
        std::vector<std::uint32_t> idx = {3, 0, 2, 2, 0, 1};
        SpriteSicht sicht;
        sicht.vorn[1] = 1.0F;
        erwarte("autosprite2: verformeTess rechnet", verformeTess(z, 0.0F, sicht, te, idx));
        bool lage = true;
        for (int w = 0; w < 4; ++w) {
            for (int k = 0; k < 3; ++k) { lage = lage && te[static_cast<std::size_t>(w)].xyz[k] == ecken[w][k]; }
        }
        erwarte("autosprite2: von vorn gesehen liegen die Ecken wie gebaut, Wicklung bleibt", lage && idx == std::vector<std::uint32_t>({3, 0, 2, 2, 0, 1}));
    }

    // rc608: Stapel eines .md3: rgbGen lightingDiffuse rechnet das Licht der Entity
    {
        BspMesh m;
        BspMesh::Batch b;
        b.numIndexes = 3U;
        b.shader = 0;
        b.lightmap = -1;
        b.modellLicht = true;
        m.batches.push_back(b);
        const auto merkmale = [&m](const TextureSet& satz) {
            const std::vector<DrawCall> c = buildDrawCalls(m, &satz, 0.0F);
            return c.empty() ? 0U : c[0].state.features;
        };
        TextureSet tx = baueTexturen();
        erwarte("md3 ohne Skript: kModellLicht, ohne Helligkeitsfaktor",
                (merkmale(tx) & kModellLicht) != 0U && (merkmale(tx) & kRohEcke) != 0U);
        tx.byShader[0].ausSkript = true;
        erwarte("md3 mit Skript ohne rgbGen: identityLighting, kein Licht der Entity", (merkmale(tx) & kModellLicht) == 0U);
        tx.byShader[0].rgbVertex = true;
        erwarte("md3 mit rgbGen lightingDiffuse: kModellLicht", (merkmale(tx) & kModellLicht) != 0U);
        tx.byShader[0].rgbVertex = false;
        tx.byShader[0].lightmapStufe = true;
        erwarte("md3 mit $lightmap: tr.whiteImage (lightmapsNone), kein Licht der Entity und keine Lightmap",
                (merkmale(tx) & (kModellLicht | kLightmap)) == 0U);
    }
    // rc608: $lightmap nach mehreren Bildstufen ist eine eigene letzte Stufe
    {
        BspMesh m;
        BspMesh::Batch b;
        b.numIndexes = 3U;
        b.shader = 0;
        b.lightmap = 0;
        m.batches.push_back(b);
        TextureSet tx = baueTexturen();
        tx.byShader[0].ausSkript = true;
        tx.byShader[1].ausSkript = true;
        tx.byShader[1].lightmapStufe = true;
        tx.byShader[1].lightmapFilter = true;
        tx.byShader[1].srcFactor = BlendFactor::DstColor;
        tx.byShader[1].dstFactor = BlendFactor::Zero;
        tx.byShader[0].extraStages.push_back(1);
        const std::vector<DrawCall> c = buildDrawCalls(m, &tx, 0.0F);
        erwarte("Lightmap als Filter: Grundstufe ohne, letzte Stufe mit Lightmap",
                c.size() == 2U && (c[0].state.features & kLightmap) == 0U && (c[1].state.features & kLightmap) != 0U);
        tx.byShader[1].lightmapFilter = false;
        const std::vector<DrawCall> z = buildDrawCalls(m, &tx, 0.0F);
        erwarte("eine gewoehnliche Zusatzstufe bleibt ohne Lightmap", z.size() == 2U && (z[1].state.features & kLightmap) == 0U);
    }

    // REST6: rgbGen lightingDiffuse auf der Welt (tr.worldEntity ohne Licht, weltDiffus): Farbe 0, bei lightingDiffuse mit alphaGen
    // identity auch Alpha 0 (AGEN_SKIP), bei lightingDiffuseEntity Alpha 1; ein Brush-Modell und rgbGen vertex bleiben bei der Eckenfarbe.
    {
        BspMesh m;
        BspMesh::Batch b;
        b.numIndexes = 3U;
        b.shader = 0;
        b.lightmap = -3;
        m.batches.push_back(b);
        m.indexes.assign(3, 0);
        TextureSet tx = baueTexturen();
        tx.byShader[0].ausSkript = true;
        tx.byShader[0].rgbVertex = true;
        tx.byShader[0].rgbArt = RgbGenArt::LightingDiffuse;
        tx.byShader[1].ausSkript = true;
        tx.byShader[1].rgbVertex = true;
        tx.byShader[1].rgbArt = RgbGenArt::LightingDiffuseEntity;
        tx.byShader[1].srcFactor = BlendFactor::SrcAlpha;   // GL_SRC_ALPHA GL_ONE: das Alpha der Stufe wirkt
        tx.byShader[0].extraStages.push_back(1);
        const auto stufe = [](const std::vector<DrawCall>& v, bool zusatz) {
            for (const DrawCall& c : v) {
                if (c.zusatzstufe == zusatz) { return c; }
            }
            return DrawCall{};
        };
        const std::vector<DrawCall> brush = buildDrawCalls(m, &tx, 0.0F);
        erwarte("lightingDiffuse an einem Brush-Modell: Eckenfarbe wie bisher",
                brush.size() == 2U && !stufe(brush, false).weltDiffus && !stufe(brush, true).weltDiffus &&
                    (stufe(brush, false).state.features & kRgbConst) == 0U);
        m.weltEntity = true;
        const std::vector<DrawCall> welt = buildDrawCalls(m, &tx, 0.0F);
        const std::uint32_t schwarz = kRgbConst | kRohEcke;
        const DrawCall g = stufe(welt, false);
        const DrawCall z = stufe(welt, true);
        erwarte("lightingDiffuse auf der Welt, deckend (GL_ONE GL_ZERO): feste Farbe, das Alpha wirkt nicht und bleibt",
                welt.size() == 2U && g.weltDiffus && (g.state.features & schwarz) == schwarz &&
                    (g.state.features & (kAlphaKonst | kAlphaEins)) == 0U);
        erwarte("lightingDiffuseEntity auf der Welt (Zusatzstufe): feste Farbe, Alpha 1",
                z.weltDiffus && (z.state.features & schwarz) == schwarz && (z.state.features & (kAlphaKonst | kAlphaEins)) == kAlphaEins);
        tx.byShader[0].srcFactor = BlendFactor::SrcAlpha;
        tx.byShader[0].dstFactor = BlendFactor::OneMinusSrcAlpha;
        const DrawCall ga = stufe(buildDrawCalls(m, &tx, 0.0F), false);
        erwarte("lightingDiffuse auf der Welt mit GL_SRC_ALPHA: Alpha 0 aus gRgb (AGEN_SKIP)",
                ga.weltDiffus && (ga.state.features & (kAlphaKonst | kAlphaEins)) == kAlphaKonst);
        BatchState bs = batchStateFor(&tx, 0, 0.0F);
        weltDiffusSetzen(bs);
        erwarte("weltDiffusSetzen: rgbConst 0 0 0, alphaConst 0",
                bs.haveRgbConst && bs.rgbConst[0] == 0.0F && bs.rgbConst[1] == 0.0F && bs.rgbConst[2] == 0.0F && bs.alphaConst == 0.0F);
        tx.byShader[0].alphaGen = AlphaGen::Vertex;
        const DrawCall e = stufe(buildDrawCalls(m, &tx, 0.0F), false);
        erwarte("lightingDiffuse auf der Welt mit alphaGen vertex: das Alpha der Ecke bleibt",
                e.weltDiffus && (e.state.features & (kAlphaKonst | kAlphaEins)) == 0U);
        tx.byShader[0].alphaGen = AlphaGen::FromImage;
        tx.byShader[0].rgbArt = RgbGenArt::Vertex;
        const DrawCall v = stufe(buildDrawCalls(m, &tx, 0.0F), false);
        erwarte("rgbGen vertex auf der Welt: Eckenfarbe wie bisher", !v.weltDiffus && (v.state.features & kRgbConst) == 0U);
    }

    if (fehler != 0) {
        std::printf("FEHLGESCHLAGEN (%d)\n", fehler);
        return 1;
    }
    std::printf("alle Gegenproben bestanden (0 Fehlschlaege)\n");
    return 0;
}
