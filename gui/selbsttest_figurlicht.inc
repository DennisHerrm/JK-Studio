// selbsttest_figurlicht.inc - rc608: das Licht der Figuren auf der Karte (Modus "figurlicht").
//
// Sollwerte aus dem laufenden Movie Duels (r_debuglight 1, LogLight in tr_light.cpp): amb und dir
// sind VectorLength von ambientLight und directedLight, ganzzahlig abgeschnitten, und die Richtung.
// Je Karte ein info_player_start, der Spieler steht am gemessenen Ort (x y aus dem Start, z aus viewpos).
namespace figurlicht_probe {

struct Soll {
    const char* karte;
    float ort[3];
    float amb;
    float dir;
    float richtung[3];
};

const Soll kSoll[] = {
    {"md_am_sith", {336.0F, 6872.0F, 24.0F}, 40.0F, 161.0F, {-0.25F, 0.24F, 0.94F}},
    {"duel_kamino", {1382.0F, 3336.0F, 216.0F}, 35.0F, 97.0F, {0.25F, -0.01F, 0.97F}},
    {"duel_deathstar", {32.0F, -5032.0F, 32.0F}, 48.0F, 212.0F, {-0.17F, 0.05F, 0.98F}},
    {"duel_mustafar", {-1024.0F, 7320.0F, 24.0F}, 48.0F, 207.0F, {-0.62F, -0.08F, -0.78F}},
    {"academy1", {-1408.0F, 272.0F, 728.0F}, 52.0F, 97.0F, {0.02F, -0.64F, 0.77F}},
    {"duel_hoth", {-281.0F, -348.0F, -213.0F}, 81.0F, 42.0F, {-0.23F, -0.50F, 0.84F}},
    {"t3_byss", {736.0F, 112.0F, 24.0F}, 74.0F, 385.0F, {-0.05F, 0.01F, 1.00F}},
};

float laenge(const float v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

}   // namespace figurlicht_probe

std::vector<Schritt> figurLichtSchritte() {
    namespace fp = figurlicht_probe;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto archiv = std::make_shared<std::string>();
    add(tu("Figurenlicht: Ausgangslage (Movie Duels)", [=] {
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (archiv->empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                *archiv = o + "\\assets1.pk3";
            }
        }
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
    }));
    add(pruefe("Figurenlicht: die Vorgaben der Vorschau sind die des Spiels (r_mapOverBrightBits 0, kein Grundlicht)", [] {
        diag::detail("Knopftest: Figurenlicht - Helligkeit " + std::to_string(g_app->mapBrightness) + ", Grundlicht " + std::to_string(g_app->mapMinLight));
        return g_app->mapBrightness == 1.0F && g_app->mapMinLight == 0.0F;
    }));
    for (const fp::Soll& soll : fp::kSoll) {
        const fp::Soll* const so = &soll;
        const std::string karte = soll.karte;
        add(tu("Figurenlicht: Karte " + karte, [=] {
            mission1::ungespeicherteEntitiesVergessen();
            missionenAusArchiv(*archiv, karte);
        }));
        add(warteBis("Figurenlicht: " + karte + " geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp"; }, 4000));
        add(pruefe("Figurenlicht: " + karte + " wie r_debuglight im Spiel", [=] {
            const GridLight l = sampleLightGrid(g_app->geo, so->ort);
            const float amb = fp::laenge(l.ambient);
            const float dir = fp::laenge(l.directed);
            char z[220];
            std::snprintf(z, sizeof(z), "Knopftest: Figurenlicht %s - amb %.1f (Spiel %.0f), dir %.1f (Spiel %.0f), Richtung %.2f %.2f %.2f (Spiel %.2f %.2f %.2f)", so->karte, amb, so->amb, dir,
                          so->dir, l.dir[0], l.dir[1], l.dir[2], so->richtung[0], so->richtung[1], so->richtung[2]);
            diag::detail(z);
            bool richtungGleich = true;
            for (int k = 0; k < 3; ++k) { richtungGleich = richtungGleich && std::fabs(l.dir[k] - so->richtung[k]) < 0.011F; }
            // Das Spiel schneidet ab, der Ort aus viewpos ist ganzzahlig: 1,5 Spiel.
            return l.ok && std::fabs(amb - (so->amb + 0.5F)) < 1.5F && std::fabs(dir - (so->dir + 0.5F)) < 1.5F && richtungGleich;
        }));
    }
    return s;
}
