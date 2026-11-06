// app_karte_echtgras.inc - rc610 R2a: Gras, Buesche und Ranken (surfaceSprites) in der Echtzeit-Ansicht (eigene Funktion,
// nicht in NetRadiant-Custom). Eingebunden in app_karte_cam.cpp nach app_karte_echtfx.inc (braucht Bildlager und kugelNeben).
//
// Wunsch Tester/shank (05.11.): "sprites ... like when u are on Yavin, those long grasses on the grass textures". Bis rc609
// zeigte sie nur "Built map as in the game" (rc608 Teil 13c, app_gras.cpp). Hier dieselbe Rechnung (bhed/grasflaechen.h,
// RB_DrawSurfaceSprites aus tr_surfacesprites.cpp) live auf den Flaechen des Editors:
//   - Flaechen: jede Pinselseite, jeder Patch und jedes misc_model (q3map2 backt es in die Welt) mit einem Shader, dessen
//     Stufe 1 .. n surfaceSprites hat (tr_shade.cpp:2149 MD: Stufe 0 zeichnet keine Halme). Die Dreiecke wie in der .bsp:
//     Pinselseiten wie -meta (StripFaceSurface, echtFlaechenStreifen), Patches wie die Engine (R_SubdividePatchToGrid und
//     je Bild RB_SurfaceGrid mit LOD, aus den Kontrollpunkten mit den Normalen von q3map2), misc_model wie AddTriangleModels.
//     Die Stellen der Halme haengen nur an den Ecken der Dreiecke (randomchart); q3map2 zerschneidet Pinselseiten noch an
//     den Ebenen des BSP-Baums - dort stehen die Halme im Spiel an anderen Stellen, ihre Zahl je Flaeche bleibt (Dichte je
//     Flaeche, je Dreieck auf ein Gitter gerundet).
//   - Die festen Halme einer Flaeche (gras::kandidaten) entstehen EINMAL und bleiben, bis sich die Flaeche oder ihr Shader
//     aendert (Schluessel: Shader und Ecken; ed.echtLicht.grasNeu zaehlt die Neubauten). Je Bild nur Abstand (alpha, fade),
//     Blickrichtung und Wind (gras::zeichneKandidaten), wie die Engine je Bild - fuer die Kacheln (256 Halme, Kasten samt
//     Ueberstand) in Reichweite (ssFademax) und im Sichtkegel.
//   - Das Netz eines Bilds bleibt auf der Grafikkarte, solange Kamera, Karte und (bei Wind/Effekt) die Zeit gleich sind; je
//     Stufe ein Zeichenaufruf (SQuickSprite je Stufe).
//   - Licht: im Spiel die BLAUE Eckenfarbe der Flaeche am Fuss des Halms (input->vertexColors[..][2], :649), grau fuer alle
//     drei Kanaele. Die Echtzeit-Ansicht hat keine Eckenfarben - an ihre Stelle tritt ihr Licht der Flaeche an der Stelle des
//     Halms mit der Normale der Flaeche, davon der blaue Anteil (kartegpu Zustand3::grauLicht). Additive Stufen (Regenspritzer)
//     wie im Spiel mit (128 + Licht/2) * alpha, das Licht der Ecke als 255.
//   - Wind: die Wetterzonen (fx_wind) wirken nur mit Teilchenwolken, deren Innen/Aussen die gebaute Karte braucht
//     (wetterFuerGrasIn) - hier windstill; ssWindIdle (ssWind setzt es mit, tr_shader.cpp) schwingt wie im Spiel nach der Zeit
//     des Spielbilds (spielbildZeit) - dann zeichnet die Kamera jedes Bild neu.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).

namespace {

struct EchtGrasStufe {
    SurfaceSprite ss;
    std::size_t stufenNr = 0;
    bool additiv = false;   // :1461 GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE
    SpielStufe darstellung;
};

constexpr std::uint32_t kGrasKachel = 256;   // Halme je Kachel (Sichtpruefung)

struct EchtGrasKachel {
    std::uint32_t von = 0;
    std::uint32_t bis = 0;
    float mn[3]{};
    float mx[3]{};
};

struct EchtGrasFlaeche {
    std::uint64_t schluessel = 0;
    std::string shader;
    bool eckLicht = false;
    bool patch = false;
    std::vector<gras::Ecke> dreiecke;   // je drei Ecken
    float mn[3]{};
    float mx[3]{};
    gras::PatchGitter gitter;           // Patch: das Gitter der Engine, Dreiecke je Bild nach LOD
    std::vector<int> lodSpalten;
    std::vector<int> lodZeilen;
    std::vector<std::vector<gras::Kandidat>> kandidaten;   // je Stufe
    std::vector<std::vector<float>> normalen;              // je Stufe, je Halm die Normale der Flaeche (3)
    std::vector<std::vector<EchtGrasKachel>> kacheln;      // je Stufe
    std::vector<char> gebaut;
};

struct EchtGras {
    std::size_t stand = static_cast<std::size_t>(-1);
    std::size_t neuGeladen = static_cast<std::size_t>(-1);
    std::map<std::string, std::vector<EchtGrasStufe>> stufen;   // Shader (klein) -> Halm-Stufen (leer: keine)
    std::vector<EchtGrasFlaeche> flaechen;
    // das Netz des letzten Bilds
    struct Gruppe {
        const EchtGrasStufe* stufe = nullptr;
        bool eckLicht = false;
        std::size_t erstes = 0;
        std::size_t anzahl = 0;
    };
    std::vector<float> lage;   // wofuer das Netz gilt
    std::vector<Gruppe> gruppen;
    kartegpu::Puffer3 puffer;
    std::size_t vierecke = 0;
    std::size_t flaechenImBild = 0;
    std::size_t patchesImBild = 0;
    std::size_t bytes = 0;
    bool zeitAbhaengig = false;
};
EchtGras g_gras;

std::string grasKlein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Die Halm-Stufen eines Shaders (Stufe 1 .. n mit surfaceSprites, tr_shade.cpp:2149 MD), einmal je Shader
const std::vector<EchtGrasStufe>& grasStufen(const std::string& shader) {
    const std::string k = grasKlein(shader);
    const auto it = g_gras.stufen.find(k);
    if (it != g_gras.stufen.end()) { return it->second; }
    std::vector<EchtGrasStufe> v;
    if (const ShaderInfo* si = echtShaderInfo(shader)) {
        for (std::size_t i = 1; i < si->stages.size(); ++i) {
            const ShaderInfo::Stage& st = si->stages[i];
            if (!st.ss.aktiv() || st.image.empty()) { continue; }
            EchtGrasStufe g;
            g.ss = st.ss;
            g.stufenNr = i;
            g.additiv = st.srcFactor == BlendFactor::One && st.dstFactor == BlendFactor::One;
            g.darstellung = spielStufe(st);
            v.push_back(std::move(g));
        }
    }
    return g_gras.stufen.emplace(k, std::move(v)).first->second;
}

std::uint64_t grasHash(std::uint64_t h, const void* p, std::size_t n) {
    const auto* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) {
        h ^= b[i];
        h *= 1099511628211ULL;
    }
    return h;
}

void kastenUm(EchtGrasFlaeche& f, const float p[3]) {
    for (int k = 0; k < 3; ++k) {
        f.mn[k] = std::min(f.mn[k], p[k]);
        f.mx[k] = std::max(f.mx[k], p[k]);
    }
}

// Wie weit ein Halm hoechstens ueber den Kasten seiner Flaeche ragt (wie app_gras.cpp halmUeberstand)
float halmRand(const SurfaceSprite& ss) {
    const float h = ss.height * (1.0F + ss.variance[1]) * (1.0F + ss.fxGrow[1]) * (1.0F + std::fabs(ss.vertSkew) * 2.0F);
    const float w = ss.width * (1.0F + ss.variance[0]) * (1.0F + ss.fxGrow[0]) * (1.0F + std::fabs(ss.fadeScale));
    return h * (1.0F + ss.wind + ss.windIdle) + w + 2.0F;
}

float abstandZumKasten(const float* auge, const float* mn, const float* mx) {
    float d2 = 0.0F;
    for (int k = 0; k < 3; ++k) {
        float d = 0.0F;
        if (auge[k] < mn[k]) { d = mn[k] - auge[k]; }
        else if (auge[k] > mx[k]) { d = auge[k] - mx[k]; }
        d2 += d * d;
    }
    return std::sqrt(d2);
}

// Die festen Halme einer Stufe: je Dreieck gras::kandidaten, dazu die Normale des Dreiecks und die Kacheln
void grasKandidaten(EchtGrasFlaeche& f, std::size_t si, const EchtGrasStufe& st) {
    std::vector<gras::Kandidat>& k = f.kandidaten[si];
    std::vector<float>& n = f.normalen[si];
    k.clear();
    n.clear();
    for (std::size_t t = 0; t + 2 < f.dreiecke.size(); t += 3) {
        const std::size_t vorher = k.size();
        gras::kandidaten(st.ss, &f.dreiecke[t], k);
        if (k.size() == vorher) { continue; }
        const float* a = f.dreiecke[t].xyz;
        const float* b = f.dreiecke[t + 1].xyz;
        const float* c = f.dreiecke[t + 2].xyz;
        const float u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const float v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        float q[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const float l = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
        // nach oben wie die Normalen der Ecken (die Halme stehen nur auf Dreiecken, deren Ecken nach oben zeigen, :644)
        const float zeichen = (q[2] < 0.0F) == (f.dreiecke[t].normalZ + f.dreiecke[t + 1].normalZ + f.dreiecke[t + 2].normalZ < 0.0F) ? 1.0F : -1.0F;
        for (float& x : q) { x = l > 0.0F ? x / l * zeichen : 0.0F; }
        if (l <= 0.0F) { q[2] = 1.0F; }
        for (std::size_t i = vorher; i < k.size(); ++i) { n.insert(n.end(), q, q + 3); }
    }
    std::vector<EchtGrasKachel>& kach = f.kacheln[si];
    kach.clear();
    const float rand = halmRand(st.ss);
    for (std::uint32_t von = 0; von < k.size(); von += kGrasKachel) {
        EchtGrasKachel x;
        x.von = von;
        x.bis = std::min<std::uint32_t>(static_cast<std::uint32_t>(k.size()), von + kGrasKachel);
        for (int a = 0; a < 3; ++a) {
            x.mn[a] = 1.0e30F;
            x.mx[a] = -1.0e30F;
        }
        for (std::uint32_t i = x.von; i < x.bis; ++i) {
            for (int a = 0; a < 3; ++a) {
                x.mn[a] = std::min(x.mn[a], k[i].p[a] - rand);
                x.mx[a] = std::max(x.mx[a], k[i].p[a] + rand);
            }
        }
        kach.push_back(x);
    }
}

// Die Flaechen mit Halmen aus der Karte (bei jedem neuen Stand der Geometrie; die Halme bleibender Flaechen bleiben)
void grasFlaechenSammeln(std::size_t geoStand) {
    Editor& ed = e();
    if (g_gras.neuGeladen != ed.neuGeladen) {   // Flush & Reload Shaders: die Skripte neu
        g_gras.neuGeladen = ed.neuGeladen;
        g_gras.stufen.clear();
        g_gras.flaechen.clear();
        g_gras.stand = static_cast<std::size_t>(-1);
    }
    if (g_gras.stand == geoStand) { return; }
    g_gras.stand = geoStand;
    std::map<std::uint64_t, EchtGrasFlaeche> alt;
    for (EchtGrasFlaeche& f : g_gras.flaechen) { alt.emplace(f.schluessel, std::move(f)); }
    g_gras.flaechen.clear();
    const auto aufnehmen = [&alt](EchtGrasFlaeche&& neu) {
        const auto da = alt.find(neu.schluessel);
        if (da != alt.end()) {
            g_gras.flaechen.push_back(std::move(da->second));
            alt.erase(da);
            return;
        }
        const std::size_t n = grasStufen(neu.shader).size();
        neu.kandidaten.resize(n);
        neu.normalen.resize(n);
        neu.kacheln.resize(n);
        neu.gebaut.assign(n, 0);
        ++e().echtLicht.grasNeu;
        g_gras.flaechen.push_back(std::move(neu));
    };
    const auto anfang = [](EchtGrasFlaeche& f, const std::string& shader) {
        f.shader = shader;
        f.eckLicht = spielDarstellung(shader).eckLicht;
        for (int k = 0; k < 3; ++k) {
            f.mn[k] = 1.0e30F;
            f.mx[k] = -1.0e30F;
        }
        f.schluessel = grasHash(1469598103934665603ULL, shader.data(), shader.size());
    };
    for (const auto& en : ed.karte.entitaeten()) {
        if (!en->sichtbar()) { continue; }
        for (const auto& prim : en->kinder()) {
            if (!prim->sichtbar()) { continue; }
            if (prim->art() == karte::Primitiv::Art::Pinsel) {
                auto& pinsel = static_cast<karte::Pinsel&>(*prim);
                pinsel.bRep(ed.karte.kontext);
                for (const auto& fl : pinsel.flaechen()) {
                    if (!fl || !fl->traegtBei() || grasStufen(fl->shader()).empty()) { continue; }
                    const karte::Wicklung& w = fl->wicklung();
                    if (w.numpoints < 3) { continue; }
                    EchtGrasFlaeche f;
                    anfang(f, fl->shader());
                    std::vector<karte::Vektor3f> ecken;
                    for (std::size_t i = 0; i < w.numpoints; ++i) {
                        ecken.emplace_back(static_cast<float>(w[i].vertex[0]), static_cast<float>(w[i].vertex[1]), static_cast<float>(w[i].vertex[2]));
                    }
                    const karte::Vektor3d& nd = fl->ebene().normal();
                    const float nz = static_cast<float>(nd[2]);
                    // -meta: StripFaceSurface (sonst FanFaceSurface mit dem Mittelpunkt an Index w.size())
                    bool mitte = false;
                    const std::vector<int> idx = karte::echtFlaechenStreifen(ecken, &mitte);
                    karte::Vektor3f m(0, 0, 0);
                    for (const karte::Vektor3f& p : ecken) { m = m + p; }
                    m = m * (1.0F / static_cast<float>(ecken.size()));
                    for (const int i : idx) {
                        const karte::Vektor3f& p = static_cast<std::size_t>(i) < ecken.size() ? ecken[static_cast<std::size_t>(i)] : m;
                        gras::Ecke q;
                        for (int k = 0; k < 3; ++k) { q.xyz[k] = p[static_cast<std::size_t>(k)]; }
                        q.normalZ = nz;
                        q.licht = 255.0F;
                        f.dreiecke.push_back(q);
                        kastenUm(f, q.xyz);
                        f.schluessel = grasHash(f.schluessel, q.xyz, sizeof(q.xyz));
                    }
                    f.schluessel = grasHash(f.schluessel, &nz, sizeof(nz));
                    aufnehmen(std::move(f));
                }
            } else if (prim->art() == karte::Primitiv::Art::Patch) {
                const auto& patch = static_cast<const karte::Patch&>(*prim);
                if (grasStufen(patch.shader()).empty()) { continue; }
                const int b = static_cast<int>(patch.breite());
                const int h = static_cast<int>(patch.hoehe());
                std::vector<karte::EchtNetzPunkt> netz;
                for (const karte::PatchPunkt& p : patch.punkte()) {
                    karte::EchtNetzPunkt q;
                    q.ort = p.vertex;
                    netz.push_back(q);
                }
                float kurve = 0;
                if (!karte::echtPatchSteuernetz(netz, b, h, kurve)) { continue; }   // ParsePatch verwirft ihn
                EchtGrasFlaeche f;
                anfang(f, patch.shader());
                f.patch = true;
                // ParseMesh (tr_bsp.cpp MD): die Kontrollpunkte mit den Normalen von q3map2, R_SubdividePatchToGrid; lodOrigin und
                // lodRadius aus den Grenzen der Kontrollpunkte (q3map2 schreibt sie in lightmapVecs[0..1])
                std::vector<gras::PatchPunkt> pp(netz.size());
                float bmn[3] = {1.0e30F, 1.0e30F, 1.0e30F};
                float bmx[3] = {-1.0e30F, -1.0e30F, -1.0e30F};
                for (std::size_t i = 0; i < netz.size(); ++i) {
                    for (int k = 0; k < 3; ++k) {
                        pp[i].xyz[k] = netz[i].ort[static_cast<std::size_t>(k)];
                        pp[i].normal[k] = netz[i].normale[static_cast<std::size_t>(k)];
                        bmn[k] = std::min(bmn[k], pp[i].xyz[k]);
                        bmx[k] = std::max(bmx[k], pp[i].xyz[k]);
                    }
                    for (std::uint8_t& c : pp[i].farbe) { c = 255; }
                    f.schluessel = grasHash(f.schluessel, pp[i].xyz, sizeof(pp[i].xyz));
                }
                f.gitter = gras::unterteilePatch(b, h, pp.data());
                if (f.gitter.breite < 2 || f.gitter.hoehe < 2) { continue; }
                float r2 = 0.0F;
                for (int k = 0; k < 3; ++k) {
                    f.gitter.lodMitte[k] = (bmn[k] + bmx[k]) * 0.5F;
                    r2 += (bmn[k] - f.gitter.lodMitte[k]) * (bmn[k] - f.gitter.lodMitte[k]);
                    f.mn[k] = f.gitter.mins[k];
                    f.mx[k] = f.gitter.maxs[k];
                }
                f.gitter.lodRadius = std::sqrt(r2);
                aufnehmen(std::move(f));
            }
        }
    }
    // misc_model: q3map2 backt ihre Dreiecke in die Welt (AddTriangleModels) - dort zeichnet das Spiel auch ihre Halme
    karte::echtModellFlaechen(ed.karte, [&](const karte::EchtModellFlaeche& mf) {
        if (mf.ort.size() < 3 || mf.normale.size() != mf.ort.size() || grasStufen(mf.shader).empty()) { return; }
        EchtGrasFlaeche f;
        anfang(f, mf.shader);
        for (std::size_t i = 0; i < mf.ort.size(); ++i) {
            gras::Ecke q;
            for (int k = 0; k < 3; ++k) { q.xyz[k] = mf.ort[i][static_cast<std::size_t>(k)]; }
            q.normalZ = mf.normale[i][2];
            q.licht = 255.0F;
            f.dreiecke.push_back(q);
            kastenUm(f, q.xyz);
            f.schluessel = grasHash(f.schluessel, q.xyz, sizeof(q.xyz));
            f.schluessel = grasHash(f.schluessel, &q.normalZ, sizeof(q.normalZ));
        }
        f.dreiecke.resize(f.dreiecke.size() - f.dreiecke.size() % 3U);
        aufnehmen(std::move(f));
    });
}

}  // namespace

// Die Halme dieses Bilds zeichnen (nach dem Deckenden, vor dem Durchsichtigen)
void echtGrasZeichnen(const karte::Kamera& k, const float* vp, int breite, int hoehe, std::size_t geoStand, const kartegpu::Zustand3& basis) {
    Editor& ed = e();
    const auto t0 = std::chrono::steady_clock::now();
    ed.echtLicht.grasAufrufe = 0;
    if (!g_app->zeigeGras) {
        ed.echtLicht.grasVierecke = 0;
        ed.echtLicht.grasFlaechen = 0;
        ed.echtLicht.grasPatches = 0;
        ed.echtLicht.grasBytes = 0;
        ed.echtLicht.grasZeit = false;
        ed.echtLicht.grasMs = 0;
        return;
    }
    grasFlaechenSammeln(geoStand);
    // Dieselbe Sicht wie das Spielbild (alsSpielKamera): der Editor nickt mit umgekehrtem Vorzeichen
    Camera cam;
    for (int i = 0; i < 3; ++i) { cam.pos[i] = k.ursprung[static_cast<std::size_t>(i)]; }
    cam.angles[0] = -k.winkel[0];
    cam.angles[1] = k.winkel[1];
    cam.angles[2] = k.winkel[2];
    const double zeitMs = spielbildZeit() * 1000.0;   // dieselbe Uhr wie das Spielbild (ssWind setzt auch ssWindIdle: die Halme schwingen)
    std::vector<float> lage(vp, vp + 16);
    for (const float x : {cam.pos[0], cam.pos[1], cam.pos[2], cam.angles[0], cam.angles[1], cam.angles[2], static_cast<float>(breite), static_cast<float>(hoehe),
                          static_cast<float>(geoStand)}) {
        lage.push_back(x);
    }
    if (g_gras.zeitAbhaengig) { lage.push_back(static_cast<float>(zeitMs)); }
    if (lage != g_gras.lage || !g_gras.puffer.gueltig()) {
        g_gras.lage = std::move(lage);
        // R_SurfaceSpriteFrameUpdate (:106) und RB_DrawSurfaceSprites (:1471): Auge und Achsen der Sicht, rangescalefactor 1
        // (die Kamera ist die erste Sicht), windstill (siehe oben)
        gras::Rahmen r;
        float rechts[3];
        float vorn[3];
        cam.right(rechts);
        cam.up(r.oben);
        cam.forward(vorn);
        for (int i = 0; i < 3; ++i) {
            r.auge[i] = cam.pos[i];
            r.links[i] = -rechts[i];   // ori.axis[1] zeigt nach links
        }
        r.reichweite = 1.0F;
        r.zeitMs = static_cast<int>(zeitMs);
        const float x[3] = {1.0F, 0.0F, 0.0F};   // mGlobalWindDirection ohne Wind
        gras::windSetzen(0.0F, x, r);
        gras::rahmenFertig(r);
        // je Stufe (und Mindestlicht der Flaeche) eine Gruppe, in der Reihenfolge der Flaechen
        std::map<std::pair<const EchtGrasStufe*, bool>, std::vector<kartegpu::Punkt3>> jeStufe;
        std::vector<std::pair<const EchtGrasStufe*, bool>> reihe;
        std::vector<gras::Quad> quads;
        std::vector<float> quadNormale;
        std::vector<int> lodSp;
        std::vector<int> lodZe;
        g_gras.zeitAbhaengig = false;
        g_gras.vierecke = 0;
        g_gras.flaechenImBild = 0;
        g_gras.patchesImBild = 0;
        static const float kSt[4][2] = {{1.0F, 1.0F}, {1.0F, 0.0F}, {0.0F, 0.0F}, {0.0F, 1.0F}};   // tr_quicksprite.cpp:38
        for (EchtGrasFlaeche& f : g_gras.flaechen) {
            const std::vector<EchtGrasStufe>& stufen = grasStufen(f.shader);
            bool etwas = false;
            for (std::size_t si = 0; si < stufen.size() && si < f.kandidaten.size(); ++si) {
                const EchtGrasStufe& st = stufen[si];
                if (!gras::stufeZeichnet(st.ss, st.stufenNr, r.wetter)) { continue; }
                const bool effekt = st.ss.typ == 3 || st.ss.typ == 4;
                if (effekt || st.ss.windIdle > 0.0F) { g_gras.zeitAbhaengig = true; }
                // ausser Reichweite: jede Ecke hat alpha <= 0 (:705, :1316); ganz neben dem Sichtkegel: unsichtbar
                const float rand = halmRand(st.ss);
                if (abstandZumKasten(r.auge, f.mn, f.mx) >= gras::reichweite(st.ss, r) + rand) { continue; }
                const float mitte[3] = {(f.mn[0] + f.mx[0]) * 0.5F, (f.mn[1] + f.mx[1]) * 0.5F, (f.mn[2] + f.mx[2]) * 0.5F};
                const float halb = 0.5F * std::sqrt((f.mx[0] - f.mn[0]) * (f.mx[0] - f.mn[0]) + (f.mx[1] - f.mn[1]) * (f.mx[1] - f.mn[1]) +
                                                    (f.mx[2] - f.mn[2]) * (f.mx[2] - f.mn[2]));
                if (kugelNeben(vp, mitte, halb + rand)) { continue; }
                if (f.patch) {
                    // RB_SurfaceGrid: welche Zeilen und Spalten bei dieser Sicht; aendert sich das, liegen die Halme anders (wie im Spiel)
                    gras::gitterLod(f.gitter, r.auge, vorn, lodSp, lodZe);
                    if (lodSp != f.lodSpalten || lodZe != f.lodZeilen || f.dreiecke.empty()) {
                        f.lodSpalten = lodSp;
                        f.lodZeilen = lodZe;
                        f.dreiecke.clear();
                        gras::gitterDreiecke(f.gitter, lodSp, lodZe, f.dreiecke);
                        std::fill(f.gebaut.begin(), f.gebaut.end(), 0);
                    }
                }
                quads.clear();
                quadNormale.clear();
                if (effekt) {
                    // RB_DrawEffectSurfaceSprites: die Stellen wandern mit der Zeit - je Bild ganz
                    for (std::size_t t = 0; t + 2 < f.dreiecke.size(); t += 3) {
                        const std::size_t vorher = quads.size();
                        gras::effektDreieck(st.ss, &f.dreiecke[t], r, st.additiv, quads);
                        for (std::size_t q = vorher; q < quads.size(); ++q) { quadNormale.insert(quadNormale.end(), {0.0F, 0.0F, 1.0F}); }
                    }
                } else {
                    if (f.gebaut[si] == 0) {
                        grasKandidaten(f, si, st);
                        f.gebaut[si] = 1;
                    }
                    for (const EchtGrasKachel& kach : f.kacheln[si]) {
                        if (abstandZumKasten(r.auge, kach.mn, kach.mx) >= gras::reichweite(st.ss, r) + rand) { continue; }
                        const float km[3] = {(kach.mn[0] + kach.mx[0]) * 0.5F, (kach.mn[1] + kach.mx[1]) * 0.5F, (kach.mn[2] + kach.mx[2]) * 0.5F};
                        const float kh = 0.5F * std::sqrt((kach.mx[0] - kach.mn[0]) * (kach.mx[0] - kach.mn[0]) + (kach.mx[1] - kach.mn[1]) * (kach.mx[1] - kach.mn[1]) +
                                                          (kach.mx[2] - kach.mn[2]) * (kach.mx[2] - kach.mn[2]));
                        if (kugelNeben(vp, km, kh)) { continue; }
                        // je Halm hoechstens ein Viereck, in der Reihenfolge der Halme (die Normale gehoert zum Halm)
                        for (std::uint32_t i = kach.von; i < kach.bis; ++i) {
                            const std::size_t vorher = quads.size();
                            gras::zeichneKandidaten(st.ss, &f.kandidaten[si][i], 1, r, st.additiv, quads);
                            if (quads.size() > vorher) { quadNormale.insert(quadNormale.end(), &f.normalen[si][i * 3U], &f.normalen[si][i * 3U] + 3); }
                        }
                    }
                }
                if (quads.empty()) { continue; }
                etwas = true;
                const std::pair<const EchtGrasStufe*, bool> schluessel{&st, f.eckLicht};
                auto [ziel, neu] = jeStufe.try_emplace(schluessel);
                if (neu) { reihe.push_back(schluessel); }
                for (std::size_t q = 0; q < quads.size(); ++q) {
                    const gras::Quad& v = quads[q];
                    // SQuickSprite: (licht, licht, licht, alpha)
                    const std::uint32_t farbe = static_cast<std::uint32_t>(v.licht) | (static_cast<std::uint32_t>(v.licht) << 8) |
                                                (static_cast<std::uint32_t>(v.licht) << 16) | (static_cast<std::uint32_t>(v.alpha) << 24);
                    for (const int i : {0, 1, 2, 0, 2, 3}) {   // GL_QUADS als zwei Dreiecke
                        kartegpu::Punkt3 p{};
                        for (int c = 0; c < 3; ++c) {
                            p.pos[c] = v.ecke[i][c];
                            p.normale[c] = quadNormale[q * 3U + static_cast<std::size_t>(c)];
                        }
                        p.st[0] = kSt[i][0];
                        p.st[1] = kSt[i][1];
                        p.farbe = farbe;
                        ziel->second.push_back(p);
                    }
                }
                g_gras.vierecke += quads.size();
            }
            if (etwas) {
                ++g_gras.flaechenImBild;
                if (f.patch) { ++g_gras.patchesImBild; }
            }
        }
        std::vector<kartegpu::Punkt3> alle;
        g_gras.gruppen.clear();
        for (const auto& s : reihe) {
            const std::vector<kartegpu::Punkt3>& v = jeStufe[s];
            EchtGras::Gruppe gr;
            gr.stufe = s.first;
            gr.eckLicht = s.second;
            gr.erstes = alle.size();
            gr.anzahl = v.size();
            alle.insert(alle.end(), v.begin(), v.end());
            g_gras.gruppen.push_back(gr);
        }
        g_gras.bytes = alle.size() * sizeof(kartegpu::Punkt3);
        ed.echtLicht.grasPruefsumme = grasHash(1469598103934665603ULL, alle.data(), g_gras.bytes);
        if (!alle.empty()) { g_gras.puffer.setzen(alle.data(), g_gras.bytes); }
    }
    for (const EchtGras::Gruppe& gr : g_gras.gruppen) {
        const EchtGrasStufe& st = *gr.stufe;
        const SpielStufe& sd = st.darstellung;
        const Bild* b = lager().holen(sd.bild);
        if (b == nullptr || !b->fertig || b->ersatz || b->srv == nullptr) { continue; }   // fehlt das Bild, ist der Shader im Spiel der Ersatz-Shader (ParseStage)
        kartegpu::Zustand3 z = basis;
        // SQuickSprite (tr_quicksprite.cpp StartGroup): die stateBits der Stufe, ohne Wegschneiden der Rueckseite
        z.wegschneiden = false;
        z.bild = b->srv;
        z.alphaFunktion = sd.alphaFunktion;
        z.alphaRef = sd.alphaRef;
        z.tiefe = kartegpu::TiefenTest::KleinerGleich;
        z.tiefeSchreiben = sd.tiefeSchreiben;
        if (sd.mischQuelle != 0) {
            z.mischen = kartegpu::Mischen::Spiel;
            z.mischQuelle = sd.mischQuelle;
            z.mischZiel = sd.mischZiel;
        } else {
            z.mischen = kartegpu::Mischen::Aus;
        }
        z.farbe[0] = z.farbe[1] = z.farbe[2] = z.farbe[3] = 1.0F;
        // additiv: die Eckenfarbe ist schon die Helligkeit (:784); sonst das Licht der Flaeche an der Stelle, grau aus dem Blau
        z.licht = !st.additiv;
        z.echtLicht = !st.additiv;
        z.grauLicht = !st.additiv;
        z.eckLicht = gr.eckLicht;
        kartegpu::dreiecke(g_gras.puffer, gr.erstes, gr.anzahl, z);
        ++ed.echtLicht.grasAufrufe;
    }
    ed.echtLicht.grasVierecke = g_gras.vierecke;
    ed.echtLicht.grasFlaechen = g_gras.flaechenImBild;
    ed.echtLicht.grasPatches = g_gras.patchesImBild;
    ed.echtLicht.grasBytes = g_gras.bytes;
    ed.echtLicht.grasZeit = g_gras.zeitAbhaengig && g_gras.vierecke > 0;
    ed.echtLicht.grasMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
