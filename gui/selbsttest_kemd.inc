// selbsttest_kemd.inc - rc609 KEMD: Karten-Editor mit Movie-Duels-Daten (BHED_EDITORTEST=kemd)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach den Karten-Editor-Teilen, mit allen Helfern davor).
// BHED_KEMD_NUR=shader|assets waehlt einen Teil (sonst beide).
//
//   K2  "SHADER IMAGE MISSING": die Karte BHED_KEMD_KARTE (.map, z. B. die zurueckgewandelte mp/bm_duel1) im Karten-Editor
//       mit den Movie-Duels-Daten und mit OpenJK (nur base). Je Shader der Brush-Seiten und Patches: zeigt die Kamera das
//       Ersatzbild (notex/shadernotex)? Soll = was die Vorschau (Mission-Tab, R_FindShader-Weg loadTextureFor) fuer
//       denselben Shader laedt: kein Shader ohne Bild im Editor, fuer den die Vorschau ein Bild hat. Dazu die
//       Kamera wie im Befund (48 ueber dem ersten info_player_*) und eine Pixelprobe des Bodens, Textured und mit
//       Echtzeit-Licht, je mit Entities (gemeldet) und ohne sie (nicht violett).
//   K3  Asset-Browser: Oeffnen mit den Movie-Duels-Daten; laengste Bildzeit waehrend des Sammelns, Zeit bis zur vollen
//       Liste; die Liste (Anzahl, Reihenfolge) nach kemd_assets.txt im Laufordner (Vergleich zwischen zwei Laeufen).
namespace kemd {
namespace ke = karteneditor;

std::string kKlein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

struct ShaderZaehlung {
    std::set<std::string> shader;   // Shader der Brush-Seiten und Patches (klein)
    bool offen = false;             // ein Bild laedt noch
};

ShaderZaehlung shaderDerKarte() {
    ShaderZaehlung z;
    const ke::Editor& e = ke::editor();
    for (const auto& ent : e.karte.entitaeten()) {
        for (const auto& kind : ent->kinder()) {
            if (const auto p = std::dynamic_pointer_cast<karte::Pinsel>(kind)) {
                for (const auto& f : p->flaechen()) { z.shader.insert(f->shader()); }
            } else if (const auto pa = std::dynamic_pointer_cast<karte::Patch>(kind)) {
                z.shader.insert(pa->shader());
            }
        }
    }
    for (const std::string& s : z.shader) {
        if (ke::kameraBildErsatz(s) < 0) { z.offen = true; }
    }
    return z;
}

// Die Kamera wie im Befund (B13 triggerBauen): 48 ueber dem ersten Spielerstart, in seiner Blickrichtung; 45 Grad nach unten
// (statt 10), damit die Probe unten im Bild die Oberseite seines Kastens (Klassenfarbe, bis 32 ueber dem Ursprung) trifft.
bool kameraWieBefund() {
    ke::Editor& e = ke::editor();
    for (const auto& ent : e.karte.entitaeten()) {
        if (ent->wert("classname").rfind("info_player_", 0) != 0) { continue; }
        float o[3] = {0, 0, 0};
        if (!leseVektor(ent->wert("origin"), o)) { continue; }
        e.kamera.setzeUrsprung(karte::Vektor3f(o[0], o[1], o[2] + 48.0F));
        e.kamera.setzeWinkel(karte::Vektor3f(-45.0F, static_cast<float>(std::atof(ent->wert("angle").c_str())), 0.0F));
        ++e.sichtStand;
        return true;
    }
    return false;
}

// Mittelwert eines Feldes unten in der Mitte der Kamera.
bool bodenFarbe(float rgb[3]) {
    std::vector<unsigned char> bild;
    int b = 0;
    int h = 0;
    if (!ke::kameraBild(bild, b, h) || b < 16 || h < 16) { return false; }
    double s[3] = {0, 0, 0};
    int n = 0;
    for (int y = h * 17 / 20; y < h * 19 / 20; ++y) {   // unten: 40 bis 50 Grad unter dem Blick
        for (int x = b * 3 / 8; x < b * 5 / 8; ++x) {
            const unsigned char* p = &bild[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x)) * 4];
            for (int k = 0; k < 3; ++k) { s[k] += p[k]; }
            ++n;
        }
    }
    for (int k = 0; k < 3; ++k) { rgb[k] = static_cast<float>(s[k] / n); }
    return true;
}

// violett: Rot und Blau deutlich ueber Gruen (der Boden n_floor02 ist braun-grau, Blau <= Gruen)
bool violett(const float rgb[3]) { return rgb[2] > rgb[1] + 25.0F && rgb[0] > rgb[1] + 25.0F; }

// Bodenprobe wie im Befund: erst mit den Entities (gemeldet, der Kasten des Spielerstarts liegt 16 unter der Kamera),
// dann ohne sie (Filter > Entities) - dann muss der Boden zu sehen und nicht violett sein.
void bodenProben(std::vector<Schritt>& s, const std::string& name, const std::string& wie) {
    auto messen = [name, wie](const char* mitOhne, bool pruefen) {
        return pruefe("K2 " + name + ": Boden " + wie + " " + mitOhne + " Entities" + (pruefen ? " nicht violett" : " (nur gemeldet)"),
                      [name, wie, mitOhne, pruefen] {
                          float c[3] = {0, 0, 0};
                          const bool ok = bodenFarbe(c);
                          char t[200];
                          std::snprintf(t, sizeof t, "Knopftest: K2 %s - Boden %s %s Entities %.0f %.0f %.0f", name.c_str(), wie.c_str(), mitOhne,
                                        static_cast<double>(c[0]), static_cast<double>(c[1]), static_cast<double>(c[2]));
                          diag::detail(t);
                          return ok && (!pruefen || !violett(c));
                      });
    };
    s.push_back(messen("mit", false));
    s.push_back(tu("K2 " + name + ": Filter > Entities an", [] { ke::filterBefehl("FilterEntities"); }));
    s.push_back(pause(1.0));
    s.push_back(foto("kemd_k2_" + name + "_" + wie + "_ohne_entities"));
    s.push_back(messen("ohne", true));
    s.push_back(tu("K2 " + name + ": Filter > Entities aus", [] { ke::filterBefehl("FilterEntities"); }));
}

std::vector<Schritt> shaderTeil(int regelwerk, const std::string& name) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* kk = std::getenv("BHED_KEMD_KARTE");
    const std::string karte = kk != nullptr ? kk : "";
    // OpenJK: die Ordner einer JKA-Installation (die Movie-Duels-Ordner nimmt der Editor fuer OpenJK nicht)
    const char* ja = std::getenv("BHED_KEMD_JA");
    const std::string jaOrdner = ja != nullptr ? ja : "C:/Program Files (x86)/Steam/steamapps/common/Jedi Academy/GameData original/base";
    auto ordnerVorher = std::make_shared<std::vector<std::string>>();
    add(tu("K2 " + name + ": Regelwerk, Spielordner neu, Vorschau laedt mp/duel1 (Soll)", [regelwerk, jaOrdner, ordnerVorher] {
        *ordnerVorher = g_app->settings.gamePaths;
        if (regelwerk == kRegelwerkOpenJK) { g_app->settings.gamePaths = {jaOrdner}; }
        g_app->settings.regelwerk = regelwerk;
        rescanGamePaths();
        g_app->leftMode = 1;
        std::string bytes;
        if (readFromArchives("maps/mp/duel1.bsp", bytes)) { loadMapFromBytes(bytes, "maps/mp/duel1.bsp"); }
    }));
    add(warteBis("K2 " + name + ": Shadertabelle der Vorschau gelesen", [] { return g_app->shaderMapRead; }, 3000));
    add(tu("K2 " + name + ": Karten-Editor, Karte oeffnen", [] { g_app->leftMode = 3; }));
    add(warteBis("K2 " + name + ": Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("K2 " + name + ": " + karte, [karte] {
        karteDialogeZu();
        (void)ke::karteOeffnen(karte);
        ke::editor().echtLicht.an = false;
        (void)kameraWieBefund();
    }));
    add(warteBis("K2 " + name + ": Bilder der Flaechen geladen", [] { return !shaderDerKarte().offen && ke::kameraBilderFertig(); }, 6000));
    add(pruefe("K2 " + name + ": kein Shader ohne Bild im Editor, fuer den die Vorschau ein Bild hat", [name] {
        const ShaderZaehlung z = shaderDerKarte();
        int editorOhne = 0;
        int vorschauOhne = 0;
        std::string falsch;
        std::string beide;
        std::string liste;
        // Eine shaderlist.txt in den Spielordnern: NRC (Shaders_Load) und q3map2 (LoadShaderInfo) lesen nur die dort
        // genannten Shader-Dateien, das Spiel alle. Ein Shader aus einer anderen Datei ist fuer Editor und Bau
        // unbekannt (Bild = Shadername) - das zeigt der Editor wie NRC; gemeldet, kein Fehler.
        const ke::Editor& ed = ke::editor();
        const std::vector<std::string> meldungen = ed.karteSpiel ? ed.karteSpiel->inventar.meldungen() : std::vector<std::string>();
        const bool mitListe = std::any_of(meldungen.begin(), meldungen.end(), [](const std::string& m) { return m.rfind("Found shaderlist.txt", 0) == 0; });
        for (const std::string& sh : z.shader) {
            const bool ersatz = ke::kameraBildErsatz(sh) == 1;
            TextureSet::Tex t;
            const bool vorschau = loadTextureFor(sh, t);
            editorOhne += ersatz ? 1 : 0;
            vorschauOhne += vorschau ? 0 : 1;
            if (ersatz && vorschau && mitListe && ed.karteSpiel->inventar.shaderFinden(sh) == nullptr) {
                liste += " " + sh;
            } else if (ersatz && vorschau) {
                falsch += " " + sh;
            }
            if (ersatz && !vorschau) { beide += " " + sh; }
        }
        diag::detail("Knopftest: K2 " + name + " - " + std::to_string(z.shader.size()) + " Shader, Editor ohne Bild " + std::to_string(editorOhne) +
                     ", Vorschau ohne Bild " + std::to_string(vorschauOhne) + "; nur im Editor ohne Bild:" + (falsch.empty() ? " -" : falsch) +
                     "; nicht in shaderlist.txt (wie NRC und q3map2):" + (liste.empty() ? " -" : liste) +
                     "; in beiden ohne Bild:" + (beide.empty() ? " -" : beide));
        return falsch.empty() && !z.shader.empty();
    }));
    add(pause(0.5));
    add(foto("kemd_k2_" + name + "_editor"));
    bodenProben(s, name, "Textured");
    add(tu("K2 " + name + ": Echtzeit-Licht an", [] { ke::editor().echtLicht.an = true; }));
    add(pause(2.0));
    add(foto("kemd_k2_" + name + "_echtzeit"));
    bodenProben(s, name, "Echtzeit");
    // Endrunde 1 Punkt 2: ist der dunkle Boden richtig? Dieselbe Stelle mit den Lightmaps von q3map2 (Schnell-Backen) und
    // durch den Zeichner der Vorschau (Spielbild). Die zurueckgewandelte Karte hat keine light-Entities (q3map2 nimmt sie
    // nicht in die .bsp, -keeplights), also Licht nur aus _ambient, _minlight und den Flaechenlichtern.
    auto werte = std::make_shared<std::array<std::array<float, 3>, 3>>();
    auto messe = [name, werte](int k, const char* wie) {
        return tu("K2 " + name + ": Boden " + wie + " ohne Entities messen", [name, werte, k, wie] {
            float c[3] = {0, 0, 0};
            (void)bodenFarbe(c);
            for (int i = 0; i < 3; ++i) { (*werte)[static_cast<std::size_t>(k)][static_cast<std::size_t>(i)] = c[i]; }
            char t[200];
            std::snprintf(t, sizeof t, "Knopftest: K2 %s - Boden %s ohne Entities %.0f %.0f %.0f", name.c_str(), wie, static_cast<double>(c[0]),
                          static_cast<double>(c[1]), static_cast<double>(c[2]));
            diag::detail(t);
        });
    };
    add(tu("K2 " + name + ": Filter > Entities an (Vergleich)", [] { ke::filterBefehl("FilterEntities"); }));
    add(pause(1.0));
    add(messe(0, "Echtzeit"));
    add(tu("K2 " + name + ": Schnell-Backen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    add(stufe9c::langeWarten("K2 " + name + ": gebacken", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("K2: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 60000));
    add(pause(1.0));
    add(foto("kemd_k2_" + name + "_gebacken"));
    add(messe(1, "Schnell-Backen"));
    add(tu("K2 " + name + ": wie im Spiel", [] { ke::editor().echtLicht.spielbild = true; }));
    add(stufe9c::langeWarten("K2 " + name + ": Spielbild", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.0));
    add(foto("kemd_k2_" + name + "_spielbild"));
    add(messe(2, "Spielbild"));
    // Grenze 4 von 255: die Vorschau ohne Filter und Supersampling (Kopf von karte_gpu_win32.cpp "Stufe 7/9c")
    add(pruefe("K2 " + name + ": Boden Echtzeit wie Schnell-Backen und Spielbild (q3map2)", [werte] {
        for (std::size_t k = 1; k < 3; ++k) {
            for (std::size_t i = 0; i < 3; ++i) {
                if (std::fabs((*werte)[0][i] - (*werte)[k][i]) > 4.0F) { return false; }
            }
        }
        return true;
    }));
    add(tu("K2 " + name + ": Echtzeit-Licht aus, Filter > Entities aus, Spielordner zurueck", [ordnerVorher] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        ke::filterBefehl("FilterEntities");
        g_app->settings.gamePaths = *ordnerVorher;
    }));
    return s;
}

struct AssetMessung {
    std::chrono::steady_clock::time_point auf;
    std::chrono::steady_clock::time_point letztes;
    double laengstesBildMs = 0;
    double bisVollMs = 0;
};

std::vector<Schritt> assetTeil() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto m = std::make_shared<AssetMessung>();
    add(tu("K3: Regelwerk Movie Duels, Karten-Editor", [] {
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        rescanGamePaths();
        g_app->leftMode = 3;
    }));
    add(warteBis("K3: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(pause(1.0));
    add(tu("K3: View > Map tools > Asset browser", [m] {
        if (ke::assetBrowserOffen()) { ke::assetBrowserUmschalten(); }
        ke::assetBrowserUmschalten();
        m->auf = std::chrono::steady_clock::now();
        m->letztes = m->auf;
    }));
    add(warteBis("K3: Liste gesammelt (Bildzeiten gemessen)", [m] {
        const auto jetzt = std::chrono::steady_clock::now();
        m->laengstesBildMs = std::max(m->laengstesBildMs, std::chrono::duration<double, std::milli>(jetzt - m->letztes).count());
        m->letztes = jetzt;
        if (ke::assetSammelnLaeuft() || ke::assetListeAlle().empty()) { return false; }
        m->bisVollMs = std::chrono::duration<double, std::milli>(jetzt - m->auf).count();
        return true;
    }, 100000));
    add(pruefe("K3: Fenster bedienbar (laengstes Bild < 100 ms), Liste da", [m] {
        const auto modelle = ke::assetListe(0);
        const auto klaenge = ke::assetListe(2);
        std::ofstream f("kemd_assets.txt", std::ios::binary | std::ios::trunc);
        for (const auto& a : ke::assetListeAlle()) { f << static_cast<int>(a.art) << ' ' << a.pfad << '\n'; }
        char t[300];
        std::snprintf(t, sizeof t, "Knopftest: K3 - laengstes Bild %.0f ms, volle Liste nach %.0f ms, Modelle %d, Klaenge %d, alle %d",
                      m->laengstesBildMs, m->bisVollMs, static_cast<int>(modelle.size()), static_cast<int>(klaenge.size()),
                      static_cast<int>(ke::assetListeAlle().size()));
        diag::detail(t);
        return m->laengstesBildMs < 100.0 && !modelle.empty() && !klaenge.empty();
    }));
    add(foto("kemd_k3_assets"));
    add(tu("K3: Asset-Browser zu", [] {
        if (ke::assetBrowserOffen()) { ke::assetBrowserUmschalten(); }
    }));
    return s;
}

}  // namespace kemd

std::vector<Schritt> kemdSchritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_KEMD_NUR");
    const std::string n = nur != nullptr ? nur : "";
    s.push_back(tu("KEMD: Spielordner", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
    }));
    if (n.empty() || n == "shader") {
        for (Schritt& x : kemd::shaderTeil(kRegelwerkMovieDuels, "md")) { s.push_back(std::move(x)); }
        for (Schritt& x : kemd::shaderTeil(kRegelwerkOpenJK, "ja")) { s.push_back(std::move(x)); }
    }
    if (n.empty() || n == "assets") {
        for (Schritt& x : kemd::assetTeil()) { s.push_back(std::move(x)); }
    }
    return s;
}
