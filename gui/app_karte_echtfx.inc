// app_karte_echtfx.inc - Karten-Editor Stufe 9d: fx_runner in der Echtzeit-Ansicht (eigene
// Funktion, nicht in NetRadiant-Custom). Eingebunden in app_karte_cam.cpp (braucht dessen
// Bildlager).
//
// Was gezeichnet wird, kommt aus dem Effektsystem der Zwischensequenz-Vorschau (bhed/efxdraw.h:
// Zeitplanung, Bahnen und Kurven aus efxed, gegen FxScheduler/FxPrimitives gebaut) - hier nur die
// fx_runner der Karte als EffectInstance nach den Regeln von g_fx.cpp (wie loadMapEffects der
// Vorschau): fxFile -> "effects/<Name ohne Endung>.efx"; Richtung zum target, sonst "angle", sonst
// nach oben (SP_fx_runner: -90 0 0); delay = Wiederholabstand (Vorgabe 200 ms), random streut ihn
// (Mitte); der erste Durchgang 600 ms nach dem Start; STARTOFF (1) und ONESHOT (2) warten auf ein
// use und laufen hier nicht. Die Shader der Effekte wie die Engine sie registriert
// (RE_RegisterShader): mit Skript die erste Stufe (blendFunc, sonst deckend), ohne Skript der
// implizite 2D-Shader (Alpha, ohne Tiefentest). Grenze: der Editor hat keine .bsp - Teilchen mit
// usePhysics prallen nicht ab (efx::sim spurt ohne Geometrie).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).

namespace {

struct EchtFx {
    std::size_t stand = static_cast<std::size_t>(-1);
    std::map<std::string, std::unique_ptr<efx::Effect>> effekte;   // Pfad -> Effekt (feste Adressen)
    std::set<std::string> fehlt;                                    // nicht lesbar (nicht jedes Bild neu suchen)
    EffectShaderSlots plaetze;                                      // Shadername (klein) -> Platz 1..
    std::vector<std::string> namen{""};                             // Platz -> Shadername
    std::vector<EffectInstance> laeufer;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
};
EchtFx g_fx;

const efx::Effect* fxLaden(const std::string& pfad) {
    const auto it = g_fx.effekte.find(pfad);
    if (it != g_fx.effekte.end()) { return it->second.get(); }
    if (g_fx.fehlt.count(pfad) != 0U || !e().karteSpiel) { return nullptr; }
    std::string text;
    if (!e().karteSpiel->fs.textLesen(pfad, text)) {
        g_fx.fehlt.insert(pfad);
        return nullptr;
    }
    efx::ReadResult r = efx::read(text);
    auto eff = std::make_unique<efx::Effect>(std::move(r.effect));
    for (const efx::Primitive& pr : eff->primitives) {
        for (const std::string& sh : pr.shaders) {
            std::string k = sh;
            for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (g_fx.plaetze.count(k) != 0U) { continue; }
            g_fx.plaetze[k] = static_cast<int>(g_fx.namen.size());
            g_fx.namen.push_back(sh);
        }
    }
    const efx::Effect* z = eff.get();
    g_fx.effekte.emplace(pfad, std::move(eff));
    return z;
}

// loadMapEffects (app_view3d_spiel.inc) fuer die Entitaeten des Editors
void fxLaeuferBauen() {
    g_fx.laeufer.clear();
    const karte::Karte& k = e().karte;
    for (const auto& ep : k.entitaeten()) {
        const karte::Entitaet& en = *ep;
        if (en.klassenname() != "fx_runner" || en.wert("notsingle") == "1") { continue; }
        std::string name = en.wert("fxFile");
        if (name.empty()) { name = en.wert("fxfile"); }
        const std::string ursprung = en.wert("origin");
        if (name.empty() || ursprung.empty()) { continue; }
        const std::size_t punkt = name.find_last_of('.');
        const std::size_t strich = name.find_last_of('/');
        if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) { name = name.substr(0, punkt); }
        const efx::Effect* eff = fxLaden("effects/" + name + ".efx");
        if (eff == nullptr) { continue; }
        const int flags = std::atoi(en.wert("spawnflags").c_str());
        if ((flags & 3) != 0) { continue; }   // STARTOFF / ONESHOT: wartet auf ein use
        EffectInstance inst;
        inst.effect = eff;
        float o[3] = {0, 0, 0};
        (void)std::sscanf(ursprung.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        for (int i = 0; i < 3; ++i) { inst.origin[i] = o[i]; }
        inst.normal[0] = 0;
        inst.normal[1] = 0;
        inst.normal[2] = 1;
        const std::string ziel = en.wert("target");
        if (!ziel.empty()) {
            for (const auto& zp : k.entitaeten()) {
                if (zp->wert("targetname") != ziel) { continue; }
                float p2[3] = {0, 0, 0};
                (void)std::sscanf(zp->wert("origin").c_str(), "%f %f %f", &p2[0], &p2[1], &p2[2]);
                const float d[3] = {p2[0] - o[0], p2[1] - o[1], p2[2] - o[2]};
                const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                if (l > 0.0001F) {
                    for (int i = 0; i < 3; ++i) { inst.normal[i] = d[i] / l; }
                }
                break;
            }
        } else if (!en.wert("angle").empty()) {
            const float gier = static_cast<float>(std::atof(en.wert("angle").c_str())) * 3.14159265F / 180.0F;
            inst.normal[0] = std::cos(gier);
            inst.normal[1] = std::sin(gier);
            inst.normal[2] = 0;
        }
        inst.loops = true;
        inst.intervalMs = 200.0F;
        if (const float v = static_cast<float>(std::atof(en.wert("delay").c_str())); v > 0.0F) { inst.intervalMs = v; }
        inst.intervalMs += static_cast<float>(std::atof(en.wert("random").c_str())) * 0.5F;
        inst.startMs = 600.0;
        inst.seed = static_cast<unsigned>(std::abs(static_cast<int>(o[0]) * 73856093 ^ static_cast<int>(o[1]) * 19349663 ^ static_cast<int>(o[2]) * 83492791));
        g_fx.laeufer.push_back(inst);
    }
    e().echtLicht.effektLaeufer = g_fx.laeufer.size();
}

}  // namespace

// Die Effekte dieses Bilds zeichnen (nach dem Deckenden, im durchscheinenden Teil)
void echtEffekteZeichnen(const karte::Kamera& k, std::size_t geoStand, const kartegpu::Zustand3& basis) {
    Editor& ed = e();
    const auto t0 = std::chrono::steady_clock::now();
    if (g_fx.stand != geoStand) {
        g_fx.stand = geoStand;
        fxLaeuferBauen();
    }
    ed.echtLicht.effektEcken = 0;
    ed.echtLicht.effektMs = 0;
    if (g_fx.laeufer.empty()) { return; }
    const double jetzt = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - g_fx.start).count();
    // die Achsen der Kamera (RB_SurfaceSprite: viewParms.ori.axis[1], [2])
    const float rechts[3] = {k.vright[0], k.vright[1], k.vright[2]};
    const float oben[3] = {k.vup[0], k.vup[1], k.vup[2]};
    EffectCull auge;
    for (int i = 0; i < 3; ++i) { auge.viewer[i] = k.ursprung[i]; }
    auge.haveViewer = true;
    auge.sichtTabelle = false;
    std::vector<EffectInstance> alle = g_fx.laeufer;
    const std::vector<EffectInstance> folge = effectFollowUpsAt(alle, jetzt, nullptr, [](const std::string& n) { return fxLaden(n); });
    alle.insert(alle.end(), folge.begin(), folge.end());
    const BspMesh netz = buildEffectMesh(alle, jetzt, rechts, oben, &g_fx.plaetze, nullptr, &auge);
    ed.echtLicht.effektEcken = netz.verts.size();
    for (const BspMesh::Batch& b : netz.batches) {
        if (b.numIndexes == 0) { continue; }
        const std::string name = b.shader > 0 && static_cast<std::size_t>(b.shader) < g_fx.namen.size() ? g_fx.namen[static_cast<std::size_t>(b.shader)] : std::string();
        kartegpu::Zustand3 z = basis;
        z.licht = false;
        z.echtLicht = false;
        z.wegschneiden = false;
        z.tiefe = kartegpu::TiefenTest::KleinerGleich;
        z.tiefeSchreiben = false;
        z.mischen = kartegpu::Mischen::Alpha;
        const karte::ShaderEintrag* se = ed.karteSpiel && !name.empty() ? ed.karteSpiel->inventar.shaderFinden(name) : nullptr;
        if (se != nullptr && !se->istStandard()) {
            const SpielDarstellung sd = spielDarstellung(name);
            if (sd.unsichtbar) { continue; }
            if (sd.mischQuelle != 0) {
                z.mischen = kartegpu::Mischen::Spiel;
                z.mischQuelle = sd.mischQuelle;
                z.mischZiel = sd.mischZiel;
            } else {   // GLS_DEFAULT: deckend
                z.mischen = kartegpu::Mischen::Aus;
                z.tiefeSchreiben = true;
            }
            z.alphaFunktion = sd.alphaFunktion;
            z.alphaRef = sd.alphaRef;
            const Bild* bild = lager().holen(!sd.bild.empty() ? sd.bild : name);
            if (bild == nullptr || !bild->fertig) { continue; }
            z.bild = bild->srv;
        } else if (!name.empty()) {
            // LIGHTMAP_2D ohne Skript: GLS_DEPTHTEST_DISABLE | SRC_ALPHA ONE_MINUS_SRC_ALPHA
            z.tiefe = kartegpu::TiefenTest::Aus;
            const Bild* bild = lager().holen("?" + name);
            if (bild == nullptr || !bild->fertig) { continue; }
            z.bild = bild->srv;
        }
        z.farbe[0] = z.farbe[1] = z.farbe[2] = z.farbe[3] = 1.0F;
        std::vector<kartegpu::Punkt3> d;
        d.reserve(b.numIndexes);
        for (std::uint32_t i = 0; i < b.numIndexes; ++i) {
            const std::uint32_t ix = netz.indexes[b.firstIndex + i];
            if (ix >= netz.verts.size()) { continue; }
            const BspVertex& v = netz.verts[ix];
            kartegpu::Punkt3 p{};
            for (int c = 0; c < 3; ++c) {
                p.pos[c] = v.xyz[c];
                p.normale[c] = v.normal[c];
            }
            p.st[0] = v.st[0];
            p.st[1] = v.st[1];
            p.farbe = static_cast<std::uint32_t>(v.colour[0]) | (static_cast<std::uint32_t>(v.colour[1]) << 8) |
                      (static_cast<std::uint32_t>(v.colour[2]) << 16) | (static_cast<std::uint32_t>(v.colour[3]) << 24);
            d.push_back(p);
        }
        d.resize(d.size() - d.size() % 3);
        if (!d.empty()) { kartegpu::dreiecke(d, z); }
    }
    ed.echtLicht.effektMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
