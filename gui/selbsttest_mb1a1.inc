// selbsttest_mb1a1.inc - Modell-Block 1a-1: der neue Model-Tab (BHED_EDITORTEST=modellblock)
//
//   BHED_MODELL_NUR=bild        jedes .glm unter models/players des Spiels (BHED_MB_SPIEL=jka|jk2|md, Vorgabe md)
//                               laden und unveraendert zeichnen: der Weg bis rc610 (gelesenes Modell, Bild der
//                               Animation, Kappen der Leiste) gegen den Weg ueber das Modelldokument
//                               (Ghoul2-Regeln, Pose mit Mischen) - Bildpunkte gleich; dazu die Halbkanten-
//                               Invarianten jedes Netzes jeder LOD-Stufe.
//   BHED_MODELL_NUR=funktionen  Bereiche, Explorer (an/aus/ohne Kinder/Wurzel, Haut off), LOD, Anbauen,
//                               Primaer/Sekundaer, Interpolation, Anzeige-Schalter, GLA-Override, Texturen neu,
//                               Foto >= 1920, Pruefhilfen mit Grenzfaellen, Gizmo mit Undo, vier Ansichten,
//                               Auswahl mit Klick und Rahmen, Bedienmodus.
//   BHED_MODELL_NUR=tasten      je Taste des Max-Modus eine Pruefzeile gegen die Tabelle (max_hotkeys_de.txt).
//   Die Bilder der Handbuchseite: BHED_EDITORTEST=handbuch, BHED_HB_NUR=mb1a1 (handbuchSchritte).
// Ohne BHED_MODELL_NUR: funktionen und tasten.
// Eingebunden in selbsttest.cpp (nach selbsttest_modell1.inc).
namespace mb1 {

int unterschied(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    if (a.size() != b.size()) { return -1; }
    int n = 0;
    for (std::size_t i = 0; i + 3 < a.size(); i += 4) {
        if (a[i] != b[i] || a[i + 1] != b[i + 1] || a[i + 2] != b[i + 2]) { ++n; }
    }
    return n;
}

struct AnsichtBild {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
};

bool ansichtLesen(AnsichtBild& b) { return modellAnsichtPixel(g_app->mtab.aktiv, b.px, b.w, b.h); }

std::string fotoOrdner() {
    const char* f = std::getenv("BHED_FOTOS");
    return f != nullptr ? std::string(f) : std::string();
}

int teil(const std::string& name) { return g_app->mtab.dok.inhalt().teilIndex(name); }

bool gez(const std::string& name) {
    const int i = teil(name);
    return i >= 0 && static_cast<std::size_t>(i) < g_app->mtab.gezeichnet.size() && g_app->mtab.gezeichnet[static_cast<std::size_t>(i)] != 0;
}

int dreieckeAnzeige() {
    int n = 0;
    for (const GlmSurface& s : g_app->mtab.anzeige.surfaces) { n += static_cast<int>(s.indexes.size() / 3U); }
    return n;
}

int animNach(const std::string& name) {
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        if (g_app->animList[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

bool matrizenGleich(const BoneMatrix& a, const BoneMatrix& b, float tol) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (std::fabs(a.m[r][c] - b.m[r][c]) > tol) { return false; }
        }
    }
    return true;
}

// Das Spiel des Bildvergleichs: Original-JKA (GameData original), Jedi Outcast (Steam) oder die Einstellungen (MD).
void spielSetzen(const std::string& spiel) {
    Settings& st = g_app->settings;
    if (spiel == "jka") {
        const char* e = std::getenv("BHED_MB_JKA");
        st.gamePaths = {e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Academy\\GameData original\\base"};
        st.regelwerk = kRegelwerkOpenJK;
    } else if (spiel == "jk2") {
        const char* e = std::getenv("BHED_JK2_BASIS");
        st.gamePaths.clear();
        st.gamePathsJk2 = {e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Outcast\\GameData\\base"};
        st.regelwerk = kRegelwerkJediOutcast;
    }
    g_app->sitzungsSpielordner.clear();
    (void)regelwerkPruefen();
    rescanGamePaths();
    diag::detail("Knopftest: MB1 - Spiel " + spiel + ": " + regelwerkStatusZeile() + ", " + std::to_string(g_app->gamePaths.size()) +
                 " Spielordner, Ghoul2 JK2 " + std::to_string(ghoul2Jk2()));
}

// Alle Modelle eines Spiels: zwei Bilder je Modell (laden, dann vergleichen).
Schritt bildVergleich(const std::string& spiel) {
    struct Zustand {
        std::vector<std::string> liste;
        std::size_t i = 0;
        int gleich = 0;
        int abweichend = 0;
        int erklaert = 0;   // anders, weil RenderSurfaces eine Flaeche anders zeichnet als der Weg bis rc610
        int ladeFehler = 0;
        int netze = 0;
        int netzeKaputt = 0;
        int nichtMannigfaltig = 0;
        std::string abweichungen;
        std::string erklaerungen;
        std::chrono::steady_clock::time_point anfang{};
    };
    auto z = std::make_shared<Zustand>();
    Schritt s{"MB1 Bild: jedes .glm unter models/players (" + spiel + ") - bis rc610 gegen das Modelldokument", [=](int b) {
                  if (b == 0) {
                      for (const GamePath& gp : g_app->gamePaths) {
                          for (const FoundFile& f : findByExtension(gp, {".glm"})) {
                              std::string k = f.name;
                              for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                              if (k.rfind("models/players/", 0) == 0) { z->liste.push_back(f.name); }
                          }
                      }
                      std::sort(z->liste.begin(), z->liste.end());
                      z->liste.erase(std::unique(z->liste.begin(), z->liste.end()), z->liste.end());
                      const char* grenze = std::getenv("BHED_MB_GRENZE");   // nur zum Nachstellen: die ersten n
                      if (grenze != nullptr && std::atoi(grenze) > 0 && z->liste.size() > static_cast<std::size_t>(std::atoi(grenze))) {
                          z->liste.resize(static_cast<std::size_t>(std::atoi(grenze)));
                      }
                      z->anfang = std::chrono::steady_clock::now();
                      diag::detail("Knopftest: MB1 Bild " + spiel + " - " + std::to_string(z->liste.size()) + " Modelle");
                      return false;
                  }
                  if (z->i >= z->liste.size()) {
                      const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - z->anfang).count();
                      char zeile[600];
                      std::snprintf(zeile, sizeof zeile,
                                    "Knopftest: MB1 Bild %s - %zu Modelle, gleich %d, anders wie im Spiel (RenderSurfaces) %d, abweichend %d, "
                                    "nicht ladbar %d, Netze %d (Invarianten verletzt %d, Kanten nicht mannigfaltig %d), %.0f s",
                                    spiel.c_str(), z->liste.size(), z->gleich, z->erklaert, z->abweichend, z->ladeFehler, z->netze, z->netzeKaputt,
                                    z->nichtMannigfaltig, sek);
                      diag::detail(zeile);
                      if (!z->abweichungen.empty()) { diag::detail("Knopftest: MB1 Bild abweichend: " + z->abweichungen); }
                      if (!z->erklaerungen.empty()) { diag::detail("Knopftest: MB1 Bild wie im Spiel anders: " + z->erklaerungen); }
                      pruefeWahr(!z->liste.empty() && z->abweichend == 0 && z->netzeKaputt == 0,
                                 "MB1 Bild " + spiel + ": jedes Modell ueber das Dokument bildgleich, Halbkanten heil");
                      return true;
                  }
                  const std::string& pfad = z->liste[z->i];
                  if ((b - 1) % 2 == 0) {
                      // Die Bilder aller Modelle im Zwischenspeicher: ueber 500 Figuren mit Texturen bis 2048 brauchen mehr
                      // als 8 GB (mb1_bmd) - der Zwischenspeicher gilt einer Sitzung, nicht einem Lauf durch alle.
                      g_app->texCache.clear();
                      g_app->figurStufenBilder.clear();
                      std::string fehler;
                      if (!oeffneModellAusArchiv(pfad, &fehler)) {
                          ++z->ladeFehler;
                          diag::detail("Knopftest: MB1 Bild - nicht ladbar: " + pfad + " (" + fehler + ")");
                          ++z->i;
                      }
                      // rc611: beide Wege mit Stufe 0 - der Weg bis rc610 zeichnet die gelesene Stufe 0, das Dokument "automatisch wie
                      // im Spiel" seit MERGE2 mit dem Einzelspieler-Radius 40 (G2_ComputeLOD): grosse Fahrzeuge (x-wing, lambdashuttle,
                      // YT-1300) bekamen im Bildausschnitt Stufe 1. Verglichen wird der Zeichenweg, nicht die Stufenwahl (modellblock funktionen).
                      g_app->mtab.lodWahl = 1;
                      return false;
                  }
                  std::vector<unsigned char> alt;
                  std::vector<unsigned char> neu;
                  constexpr int kB = 400;
                  constexpr int kH = 560;
                  const bool ok = modellFotoPixel(kB, kH, alt, false) && modellFotoPixel(kB, kH, neu, true);
                  const int d = ok ? unterschied(alt, neu) : -1;
                  // Welche Flaechen zeichnet der Weg bis rc610 anders als RenderSurfaces (Flagge statt Name, Wurzel 0)?
                  std::string anders;
                  for (std::size_t k = 0; k < g_app->model.surfaces.size() && k < g_app->mtab.gezeichnet.size(); ++k) {
                      if (g_app->model.surfaces[k].isVisible(g_app->showCaps) != (g_app->mtab.gezeichnet[k] != 0)) {
                          anders += g_app->model.surfaces[k].name + (g_app->mtab.gezeichnet[k] != 0 ? "+" : "-") + " ";
                      }
                  }
                  if (d == 0) {
                      ++z->gleich;
                  } else if (!anders.empty()) {
                      ++z->erklaert;
                      z->erklaerungen += pfad + " (" + anders + std::to_string(d) + " Punkte) ";
                  } else {
                      ++z->abweichend;
                      z->abweichungen += pfad + " (" + std::to_string(d) + " Punkte) ";
                      if (!fotoOrdner().empty() && ok) {
                          std::string n = pfad;
                          std::replace(n.begin(), n.end(), '/', '_');
                          (void)modellBmp(fotoOrdner() + "\\mb1_alt_" + n + ".bmp", kB, kH, alt);
                          (void)modellBmp(fotoOrdner() + "\\mb1_neu_" + n + ".bmp", kB, kH, neu);
                      }
                  }
                  for (const modell::Teil& t : g_app->mtab.dok.inhalt().teile) {
                      for (const modell::Netz& n : t.stufen) {
                          ++z->netze;
                          std::string grund;
                          if (!n.pruefen(&grund)) {
                              ++z->netzeKaputt;
                              diag::detail("Knopftest: MB1 Bild - " + pfad + " " + t.name + ": " + grund);
                          }
                          z->nichtMannigfaltig += n.nichtMannigfaltig;
                      }
                  }
                  ++z->i;
                  return false;
              }};
    s.eigeneZeitgrenze = true;
    return s;
}

// Ein Testmodell laden (der Sturmtruppler: vier LOD-Stufen in Movie Duels und JKA).
std::vector<Schritt> laden(const std::string& pfad) {
    std::vector<Schritt> s;
    s.push_back(tu("MB1: Model-Tab, " + pfad, [=] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        std::string fehler;
        if (!oeffneModellAusArchiv(pfad, &fehler)) { meldeFehler("MB1: " + pfad + ": " + fehler); }
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.aktiv = 3;
        g_app->animPlaying = false;
    }));
    s.push_back(pause(0.8));
    return s;
}

// MBKOPF: die Startansicht bis rc611 - von der Seite (Gier 180), vor der Mitte des Huellkastens der Datei im 1,15-fachen
// seiner groessten Seite. Fuer die Tests, deren Klick- und Malstellen an der Figur von der Seite ausgemessen sind; seit
// MBKOPF zeigt die Startansicht die Figur von vorne (das pruefen kopfrot und kopf).
Schritt seitenAnsicht() {
    return tu("MB1: Ansicht von der Seite wie bis rc611", [] {
        Camera& c = g_app->modelCam;
        float groesse = 0.0F;
        for (int k = 0; k < 3; ++k) { groesse = std::max(groesse, g_app->model.maxs[k] - g_app->model.mins[k]); }
        g_app->modelOrbit.distance = std::max(groesse * 1.15F, 24.0F);
        c.angles[0] = 0.0F;
        c.angles[1] = 180.0F;
        c.angles[2] = 0.0F;
        float vorn[3];
        c.forward(vorn);
        for (int k = 0; k < 3; ++k) { c.pos[k] = (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F - vorn[k] * g_app->modelOrbit.distance; }
        g_app->modelDirty = true;
    });
}

std::vector<Schritt> ladenSeite(const std::string& pfad) {
    std::vector<Schritt> s = laden(pfad);
    s.push_back(seitenAnsicht());
    s.push_back(pause(0.3));
    return s;
}

// Der Schirmort einer gezeichneten Flaeche (Mitte ihrer verformten Ecken) in der aktiven Ansicht.
std::function<ImVec2()> flaechenOrt(const std::string& name) {
    return [name] {
        ImVec2 p{0.0F, 0.0F};
        (void)modellFlaecheAufSchirm(teil(name), p);
        return p;
    };
}

std::vector<Schritt> funktionenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MB1: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb1.txt"); }));
    for (Schritt& x : laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("MB1 Einpassen: Ansicht verdreht, dann eingepasst", [] {
        g_app->modelOrbit.turn(g_app->modelCam, 70.0F, 20.0F);
        modellAnsichtEinpassen();
    }));
    add(pause(0.2));
    // MBKOPF: vor der Figur heisst vor ihrem Gesicht - sie schaut in der .glm nach -Y (kopfrot misst es am Skelett); bis
    // rc611 stand die Kamera bei +X (von der Seite). Der Model-Tab passt auf die Figur in der Pose ein: der Drehpunkt
    // der Kreiskamera liegt in ihrem Huellkasten.
    add(pruefe("MB1 Einpassen: die Kamera steht vor der Figur, ausserhalb des Huellkastens, der Drehpunkt der Kreiskamera in ihm", [] {
        const Camera& c = g_app->modelCam;
        float p[3];
        g_app->modelOrbit.pivot(c, p);
        bool drin = true;
        for (int k = 0; k < 3; ++k) { drin = drin && p[k] >= g_app->model.mins[k] && p[k] <= g_app->model.maxs[k]; }
        diag::detail("Knopftest: MB1 Einpassen - Kamera " + std::to_string(c.pos[0]) + " " + std::to_string(c.pos[1]) + " " +
                     std::to_string(c.pos[2]) + ", Drehpunkt " + std::to_string(p[0]) + " " + std::to_string(p[1]) + " " + std::to_string(p[2]) +
                     ", Kreis " + std::to_string(g_app->modelOrbit.distance));
        return c.pos[1] < g_app->model.mins[1] && drin;
    }));
    add(pruefe("MB1: Sturmtruppler geladen, Dokument mit allen LOD-Stufen, Bereich Ansehen/Pruefen", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        diag::detail("Knopftest: MB1 - " + std::to_string(in.teile.size()) + " Teile, " + std::to_string(in.stufenZahl) + " Stufen, " +
                     std::to_string(g_app->animList.size()) + " Animationen");
        return in.teile.size() == g_app->model.surfaces.size() && in.stufenZahl > 1 && g_app->mtab.bereich == ModellBereich::Ansehen &&
               !g_app->animList.empty();
    }));
    // --- Bereiche -------------------------------------------------------------
    add(klick(tr(Str::MbBereichModellieren), "###main"));
    add(pause(0.2));
    add(pruefe("MB1 Bereiche: Modellieren gewaehlt, Hinweis auf Stufe 1d", [] { return g_app->mtab.bereich == ModellBereich::Modellieren; }));
    add(klick(tr(Str::MbBereichAnsehen), "###main"));
    add(pause(0.2));
    add(pruefe("MB1 Bereiche: zurueck in Ansehen/Pruefen", [] { return g_app->mtab.bereich == ModellBereich::Ansehen; }));
    // --- Explorer: Flaechen an/aus, ohne Kinder, Wurzel (Rechtsklick wie ModView) -------
    // Erst suchen (H): der Treffer steht oben in der flachen Liste.
    add(klick("##mtsuche", ""));
    add(tippe("torso"));
    add(pause(0.2));
    add(klick("torso", "", 0, false, "", 1));
    add(klick(tr(Str::MbFlAus), ""));
    add(pause(0.2));
    add(pruefe("MB1 Explorer: torso aus, head bleibt (OFF ohne NODESCENDANTS)", [] { return !gez("torso") && gez("head") && gez("hips"); }));
    add(klick("torso", "", 0, false, "", 1));
    add(klick(tr(Str::MbFlAusOhneKinder), ""));
    add(pause(0.2));
    add(pruefe("MB1 Explorer: torso aus ohne Kinder - head auch aus", [] { return !gez("torso") && !gez("head") && gez("hips"); }));
    add(klick("torso", "", 0, false, "", 1));
    add(klick(tr(Str::MbFlAn), ""));
    add(pause(0.2));
    add(pruefe("MB1 Explorer: torso wieder an, head auch", [] { return gez("torso") && gez("head"); }));
    add(klick("torso", "", 0, false, "", 1));
    add(klick(tr(Str::MbFlWurzel), ""));
    add(pause(0.2));
    add(pruefe("MB1 Explorer: Wurzel torso - hips und Beine aus, Kopf an", [] {
        return g_app->mtab.g2.wurzel == teil("torso") && !gez("hips") && !gez("l_leg") && gez("head") && gez("torso");
    }));
    add(klick("torso", "", 0, false, "", 1));
    add(klick(tr(Str::MbFlUrsprung), ""));
    add(pause(0.2));
    add(pruefe("MB1 Explorer: Ursprungszustand - alles wie geladen", [] { return g_app->mtab.g2.wurzel == 0 && gez("hips") && gez("l_leg"); }));
    add(tu("MB1 Explorer: Suche leeren", [] { g_app->mtab.suche[0] = 0; }));
    // --- Haut mit *off (G2API_SetSurfaceOnOffFromSkin) -----------------------------------
    add(tu("MB1 Haut: head,*off", [] { applySkin("head,*off\n", g_app->model); }));
    add(pause(0.2));
    add(pruefe("MB1 Haut: head aus, torso an", [] { return !gez("head") && gez("torso"); }));
    add(tu("MB1 Haut: Vorgabehaut zurueck", [] {
        for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
            if (g_app->modelSkins[i] == "model_default.skin") { applySkinByIndex(static_cast<int>(i)); }
        }
    }));
    add(pause(0.2));
    add(pruefe("MB1 Haut: head wieder an", [] { return gez("head"); }));
    // --- LOD ----------------------------------------------------------------------
    auto dreiecke0 = std::make_shared<int>(0);
    add(tu("MB1 LOD: Stufe 2 fest", [=] {
        *dreiecke0 = dreieckeAnzeige();
        g_app->mtab.lodWahl = 2;
    }));
    add(pause(0.2));
    add(pruefe("MB1 LOD: Stufe 2 gezeichnet, weniger Dreiecke", [=] {
        diag::detail("Knopftest: MB1 LOD - Dreiecke " + std::to_string(*dreiecke0) + " -> " + std::to_string(dreieckeAnzeige()));
        return g_app->mtab.lodJetzt == 1 && dreieckeAnzeige() < *dreiecke0;
    }));
    add(klick(tr(Str::MbLodAuto), "###main"));
    add(pause(0.2));
    add(pruefe("MB1 LOD: automatisch, SP-Figur eingepasst (nah) zeichnet LOD 1", [] { return g_app->mtab.lodWahl == 0 && g_app->mtab.lodJetzt == 0; }));
    // MERGE2: im Einzelspieler waehlt der Abstand ebenso (Radius 40 = s.radius aus mins/maxs, r_lodscale 10)
    add(tu("MB1 LOD: SP, Kamera 3000 weit", [] {
        g_app->modelOrbit.distance = 3000.0F;
        float v[3];
        g_app->modelCam.forward(v);
        for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = -v[k] * 3000.0F; }
        g_app->mtab.anzeigeSchluessel.clear();
    }));
    add(pause(0.2));
    add(pruefe("MB1 LOD: SP in 3000 Einheiten - letzte Stufe (Radius 40, r_lodscale 10)", [] {
        return g_app->mtab.lodJetzt == g_app->mtab.dok.inhalt().stufenZahl - 1;
    }));
    add(tu("MB1 LOD: Zielspiel JKA MP, Kamera 3000 weit", [] {
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::JkaMp);
        g_app->modelOrbit.distance = 3000.0F;
        float v[3];
        g_app->modelCam.forward(v);
        for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = -v[k] * 3000.0F; }
        g_app->mtab.anzeigeSchluessel.clear();
    }));
    add(pause(0.2));
    add(pruefe("MB1 LOD: MP in 3000 Einheiten - letzte Stufe (G2_ComputeLOD, r_lodscale 5)", [] {
        return g_app->mtab.lodJetzt == g_app->mtab.dok.inhalt().stufenZahl - 1;
    }));
    add(tu("MB1 LOD: zurueck (SP, eingepasst)", [] {
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::MdSp);
        modellAnsichtEinpassen();
        g_app->mtab.anzeigeSchluessel.clear();
    }));
    add(pause(0.3));
    // --- Anbauen --------------------------------------------------------------------
    auto vorher = std::make_shared<AnsichtBild>();
    add(tu("MB1 Anbauen: Bild ohne", [=] { (void)ansichtLesen(*vorher); }));
    add(tu("MB1 Anbauen: Saeble an *r_hand, an den Knochen lhand", [] {
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "lhand", true, false);
    }));
    add(pause(0.3));
    add(pruefe("MB1 Anbauen: zwei Anbauten, beide gestellt, das Bild zeigt sie", [=] {
        AnsichtBild jetzt;
        const bool gelesen = ansichtLesen(jetzt);
        const int d = gelesen ? unterschied(vorher->px, jetzt.px) : -1;
        float w0[16];
        float w1[16];
        const bool gestellt = g_app->mtab.anbauten.size() == 2U && modellAnbauWelt(0, w0) && modellAnbauWelt(1, w1);
        const int lhand = boneIndex(g_app->anim, "lhand");
        const bool amKnochen = gestellt && lhand >= 0 && static_cast<std::size_t>(lhand) < g_app->modellWelt.size() &&
                               std::fabs(w1[3] - g_app->modellWelt[static_cast<std::size_t>(lhand)].m[0][3]) < 1e-3F;
        diag::detail("Knopftest: MB1 Anbauen - " + std::to_string(d) + " Punkte anders, am Knochen " + std::to_string(amKnochen));
        return gestellt && amKnochen && d > 200;
    }));
    add(tu("MB1 Anbauen: alle loesen", [] { modellAnbautenLoesen(); }));
    add(pause(0.2));
    add(pruefe("MB1 Anbauen: alle geloest", [] { return g_app->mtab.anbauten.empty(); }));
    // --- Primaer/Sekundaer und Interpolation --------------------------------------------------
    auto zahlen = std::make_shared<std::array<int, 2>>();
    add(tu("MB1 Sequenzen: Primaer BOTH_STAND1 Bild 0, Sekundaer TORSO_WEAPONREADY3", [=] {
        (*zahlen)[0] = animNach("BOTH_STAND1");
        (*zahlen)[1] = animNach("TORSO_WEAPONREADY3");
        if ((*zahlen)[1] < 0) { (*zahlen)[1] = animNach("BOTH_ATTACK1"); }
        animWaehlen((*zahlen)[0], false);
        g_app->mtab.sekundaer = (*zahlen)[1];
        if ((*zahlen)[1] >= 0) { g_app->mtab.sekFrame = g_app->animList[static_cast<std::size_t>((*zahlen)[1])].firstFrame + 3; }
        g_app->mtab.sekZeit = 0.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("MB1 Sequenzen: Oberkoerper (thoracic) aus der Sekundaeren, Becken (pelvis) aus der Primaeren", [=] {
        if ((*zahlen)[0] < 0 || (*zahlen)[1] < 0) { return false; }
        std::vector<BoneMatrix> erwartet;
        BoneFrameOverride o;
        o.bone = boneIndex(g_app->anim, "lower_lumbar");
        o.frame = g_app->mtab.sekFrame;
        g_app->anim.worldMatrices(g_app->animFrame, {o}, erwartet);
        std::vector<BoneMatrix> nurPrimaer;
        g_app->anim.worldMatrices(g_app->animFrame, nurPrimaer);
        const int th = boneIndex(g_app->anim, "thoracic");
        const int pe = boneIndex(g_app->anim, "pelvis");
        if (th < 0 || pe < 0 || g_app->modellWelt.size() != erwartet.size()) { return false; }
        const auto t = static_cast<std::size_t>(th);
        const auto p = static_cast<std::size_t>(pe);
        return matrizenGleich(g_app->modellWelt[t], erwartet[t], 1e-4F) && matrizenGleich(g_app->modellWelt[p], nurPrimaer[p], 1e-4F) &&
               !matrizenGleich(erwartet[t], nurPrimaer[t], 1e-3F);
    }));
    add(tu("MB1 Interpolation: an, halbes Bild weiter", [] {
        g_app->mtab.sekundaer = -1;
        const int i = animNach("BOTH_WALK1") >= 0 ? animNach("BOTH_WALK1") : 0;
        animWaehlen(i, false);
        g_app->animFrame += 2;
        g_app->animTime = 0.5F;
        g_app->mtab.interpolieren = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe("MB1 Interpolation an: zwischen Bild A und B gemischt (tr_ghoul2.cpp:1531)", [] {
        std::vector<BoneMatrix> mitte;
        g_app->anim.worldMatricesLerp(g_app->animFrame, g_app->animFrame + 1, 0.5F, mitte);
        std::vector<BoneMatrix> a;
        g_app->anim.worldMatrices(g_app->animFrame, a);
        const int k = boneIndex(g_app->anim, "rtibia") >= 0 ? boneIndex(g_app->anim, "rtibia") : 1;
        const auto i = static_cast<std::size_t>(k);
        return g_app->modellWelt.size() == mitte.size() && matrizenGleich(g_app->modellWelt[i], mitte[i], 1e-4F) &&
               !matrizenGleich(mitte[i], a[i], 1e-4F);
    }));
    add(tu("MB1 Interpolation: aus", [] {
        g_app->mtab.interpolieren = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe("MB1 Interpolation aus: genau Bild A", [] {
        std::vector<BoneMatrix> a;
        g_app->anim.worldMatrices(g_app->animFrame, a);
        bool gleich = a.size() == g_app->modellWelt.size();
        for (std::size_t i = 0; gleich && i < a.size(); ++i) { gleich = std::memcmp(&a[i], &g_app->modellWelt[i], sizeof(BoneMatrix)) == 0; }
        return gleich;
    }));
    add(tu("MB1: Interpolation wieder an, Pose Bild 0", [] {
        g_app->mtab.interpolieren = true;
        g_app->animIndex = -1;
        g_app->animFrame = 0;
        g_app->animTime = 0.0F;
        modellAnsichtEinpassen();
    }));
    add(pause(0.3));
    // --- Anzeige-Schalter --------------------------------------------------------------
    auto grund = std::make_shared<AnsichtBild>();
    add(tu("MB1 Anzeige: Grundbild", [=] { (void)ansichtLesen(*grund); }));
    const std::vector<std::pair<std::string, std::function<bool&()>>> gpuSchalter = {
        {"Drahtgitter (F3)", [] () -> bool& { return modellSch().drahtgitter; }},
        {"Kanten (F4)", [] () -> bool& { return modellSch().kanten; }},
        {"Boden", [] () -> bool& { return g_app->mtab.an.boden; }},
        {"Raster (G)", [] () -> bool& { return modellSch().raster; }},
    };
    for (const auto& [name, wert] : gpuSchalter) {
        add(tu("MB1 Anzeige: " + name + " an", [=] {
            wert() = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.2));
        add(pruefe("MB1 Anzeige: " + name + " aendert das Bild", [=] {
            AnsichtBild b;
            const int d = ansichtLesen(b) ? unterschied(grund->px, b.px) : -1;
            diag::detail("Knopftest: MB1 Anzeige " + name + " - " + std::to_string(d) + " Punkte anders");
            return d > 50;
        }));
        add(tu("MB1 Anzeige: " + name + " aus", [=] {
            wert() = false;
            g_app->modelDirty = true;
        }));
        add(pause(0.2));
        add(pruefe("MB1 Anzeige: " + name + " aus - Bild wie vorher", [=] {
            AnsichtBild b;
            return ansichtLesen(b) && unterschied(grund->px, b.px) == 0;
        }));
    }
    add(tu("MB1 Anzeige: weisse Texturen an", [] {
        g_app->showModelTextures = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("MB1 Anzeige: weisse Texturen aendern das Bild", [=] {
        AnsichtBild b;
        return !g_app->showModelTextures && ansichtLesen(b) && unterschied(grund->px, b.px) > 1000;
    }));
    add(tu("MB1 Anzeige: weisse Texturen aus", [] {
        g_app->showModelTextures = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("MB1 Anzeige: Kamera in die Figur, weisse Texturen (Rueckseiten nur zweiseitig; die Shader des Sturmtrupplers sind teils cull none)", [=] {
        g_app->showModelTextures = false;
        for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F; }
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("MB1 Anzeige: Bild innen einseitig", [=] { (void)ansichtLesen(*grund); }));
    add(tu("MB1 Anzeige: zweiseitig", [] {
        modellSch().zweiseitig = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe("MB1 Anzeige: zweiseitig zeigt innen die Rueckseiten", [=] {
        AnsichtBild b;
        const int d = ansichtLesen(b) ? unterschied(grund->px, b.px) : -1;
        diag::detail("Knopftest: MB1 Anzeige zweiseitig - " + std::to_string(d) + " Punkte anders");
        return d > 1000;
    }));
    add(tu("MB1 Anzeige: einseitig, Texturen, eingepasst", [] {
        modellSch().zweiseitig = false;
        g_app->showModelTextures = true;
        modellAnsichtEinpassen();
    }));
    add(pause(0.3));
    // Ueberlagerung: je Art die gezeichneten Striche/Punkte (ModellTab::ueber).
    const std::vector<std::tuple<std::string, int, std::function<bool&()>>> ueber = {
        {"Knochen", 0, [] () -> bool& { return g_app->modellKnochen; }},
        {"Normalen", 1, [] () -> bool& { return g_app->mtab.an.normalen; }},
        {"Gewichte", 2, [] () -> bool& { return g_app->mtab.an.gewichte; }},
        {"Tags", 3, [] () -> bool& { return g_app->mtab.an.tags; }},
        {"Huellkasten", 4, [] () -> bool& { return g_app->mtab.an.huelle; }},
        {"Ursprung", 5, [] () -> bool& { return g_app->mtab.an.ursprung; }},
    };
    for (const auto& [name, art, wert] : ueber) {
        add(tu("MB1 Ueberlagerung: " + name + " an", [=] {
            wert() = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.2));
        add(pruefe("MB1 Ueberlagerung: " + name + " gezeichnet", [=] {
            diag::detail("Knopftest: MB1 Ueberlagerung " + name + " - " + std::to_string(g_app->mtab.ueber[art]));
            return g_app->mtab.ueber[art] >= 3;
        }));
        add(tu("MB1 Ueberlagerung: " + name + " aus", [=] { wert() = false; }));
        add(pause(0.2));
        add(pruefe("MB1 Ueberlagerung: " + name + " weg", [=] { return g_app->mtab.ueber[art] == 0; }));
    }
    add(tu("MB1 Risse: torso gewaehlt, Risse an", [] {
        g_app->mtab.auswahl = {teil("torso")};
        g_app->mtab.an.risse = true;
    }));
    add(pause(0.2));
    add(pruefe("MB1 Risse: offene Kanten am torso (Halsloch, Armloecher) markiert", [] {
        diag::detail("Knopftest: MB1 Risse - " + std::to_string(g_app->mtab.ueber[6]) + " Randecken");
        return g_app->mtab.ueber[6] > 3;
    }));
    add(tu("MB1 Risse: aus, nichts gewaehlt", [] {
        g_app->mtab.an.risse = false;
        g_app->mtab.auswahl.clear();
    }));
    add(tu("MB1 FOV: 90", [=] {
        (void)ansichtLesen(*grund);
        g_app->modelCam.fovY = 90.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe("MB1 FOV: 90 Grad - die Figur kleiner, anderes Bild", [=] {
        AnsichtBild b;
        return ansichtLesen(b) && unterschied(grund->px, b.px) > 500;
    }));
    add(tu("MB1 FOV: zurueck", [] {
        g_app->modelCam.fovY = Camera{}.fovY;
        g_app->modelDirty = true;
    }));
    // --- GLA-Override, Texturen neu, Foto ------------------------------------------------------
    add(pruefe("MB1 GLA-Override: _humanoid.gla aus den Archiven", [] {
        return modellGlaLaden("models/players/_humanoid/_humanoid.gla") && g_app->anim.bones.size() == 53U && !g_app->animList.empty();
    }));
    add(tu("MB1 Texturen neu laden", [] {
        const int vorher = g_app->modelTextures.found;
        modellTexturenNeuLaden();
        diag::detail("Knopftest: MB1 Texturen - vorher " + std::to_string(vorher) + ", nachher " + std::to_string(g_app->modelTextures.found));
    }));
    add(pause(0.3));
    add(pruefe("MB1 Texturen neu: wieder alle gefunden", [] { return g_app->modelTextures.found > 0; }));
    add(pruefe("MB1 Foto: mindestens 1920 breit", [] {
        const std::string pfad = (fotoOrdner().empty() ? std::string(".") : fotoOrdner()) + "\\mb1_foto.bmp";
        if (!modellFotoSpeichern(pfad, 1920)) { return false; }
        std::FILE* f = std::fopen(pfad.c_str(), "rb");
        if (f == nullptr) { return false; }
        unsigned char kopf[54] = {};
        const bool ok = std::fread(kopf, 1, sizeof kopf, f) == sizeof kopf;
        std::fclose(f);
        const int breite = kopf[18] | (kopf[19] << 8) | (kopf[20] << 16) | (kopf[21] << 24);
        diag::detail("Knopftest: MB1 Foto - " + std::to_string(breite) + " breit");
        return ok && kopf[0] == 'B' && breite >= 1920;
    }));
    // --- Pruefhilfen ---------------------------------------------------------------------------
    const auto hat = [](modell::BefundArt art) {
        for (const modell::Befund& b : modellBefunde()) {
            if (b.art == art) { return true; }
        }
        return false;
    };
    add(pruefe("MB1 Pruefen: Sturmtruppler ohne Befund (JKA SP)", [=] {
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        const auto b = modellBefunde();
        for (const modell::Befund& x : b) { diag::detail("Knopftest: MB1 Pruefen - Befund " + x.name + " " + std::to_string(static_cast<int>(x.art))); }
        return b.empty();
    }));
    add(pruefe("MB1 Pruefen: Abschnitt hinter dem Ende der GLA (T-Pose-Fall) gemeldet, genau am Ende nicht", [=] {
        const int n = g_app->anim.bildZahl();
        g_app->animList.push_back({"MB1_AM_ENDE", n - 10, 10, -1, 20});
        const bool amEnde = !hat(modell::BefundArt::AnimHinterGla);
        g_app->animList.push_back({"MB1_HINTER_ENDE", n - 10, 11, -1, 20});
        const bool dahinter = hat(modell::BefundArt::AnimHinterGla);
        g_app->animList.resize(g_app->animList.size() - 2U);
        return amEnde && dahinter;
    }));
    add(pruefe("MB1 Pruefen: 999 Ecken in torso ok, 1000 fuer JKA ein Befund, fuer Movie Duels (4000) nicht", [=] {
        const int t = teil("torso");
        if (t < 0) { return false; }
        const auto auffuellen = [&](int ecken) {
            g_app->mtab.dok.aendern("Test", [=](modell::Inhalt& x) {
                modell::Netz& n = x.teile[static_cast<std::size_t>(t)].stufen[0];
                std::vector<modell::Ecke> e = n.ecken;
                e.resize(static_cast<std::size_t>(ecken), e.empty() ? modell::Ecke{} : e.front());
                n.aufbauen(std::move(e), n.indizes());
            });
        };
        auffuellen(999);
        const bool unter = !hat(modell::BefundArt::Ecken);
        (void)modellRueckgaengig();
        auffuellen(1000);
        const bool ueberJka = hat(modell::BefundArt::Ecken);
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::MdSp);
        const bool md = !hat(modell::BefundArt::Ecken);
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        (void)modellRueckgaengig();
        return unter && ueberJka && md && !hat(modell::BefundArt::Ecken);
    }));
    add(tu("MB1 Pruefen: torso ohne Bild", [] {
        const int t = teil("torso");
        if (t >= 0) {
            g_app->model.surfaces[static_cast<std::size_t>(t)].texture = "models/mb1_gibt_es_nicht/fehlt";
            g_app->model.surfaces[static_cast<std::size_t>(t)].shader.clear();
        }
        loadModelTextures();
    }));
    add(pause(0.2));
    add(pruefe("MB1 Pruefen: fehlendes Bild an torso gemeldet", [=] { return hat(modell::BefundArt::ShaderFehlt); }));
    add(tu("MB1 Pruefen: Vorgabehaut zurueck", [] {
        for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
            if (g_app->modelSkins[i] == "model_default.skin") { applySkinByIndex(static_cast<int>(i)); }
        }
    }));
    add(pause(0.3));
    // --- Auswahl mit Klick und Rahmen, Gizmo mit Undo --------------------------------------
    add(klickAn("in die Ansicht auf torso", flaechenOrt("torso")));
    add(pause(0.2));
    add(pruefe("MB1 Auswahl: Klick waehlt eine gezeichnete Flaeche (vorderstes Dreieck)", [] {
        const auto& a = g_app->mtab.auswahl;
        diag::detail("Knopftest: MB1 Auswahl - " + (a.empty() ? std::string("-") : g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(a[0])].name));
        return a.size() == 1U && gez(g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(a[0])].name) && g_app->mtab.fokus;
    }));
    add(tu("MB1 Auswahl: nichts gewaehlt, Ansicht eingepasst (Z)", [] {
        g_app->mtab.auswahl.clear();
        modellAnsichtEinpassenAktiv();
    }));
    add(pause(0.2));
    add(ziehenAn("Rahmen ueber die ganze Ansicht", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 4.0F, a.kopfUnten + 6.0F};   // MBKOPF: unter der Kopfzeile - ihre Felder oeffnen jetzt Menues
    }, 1500.0F, 1500.0F, 0));
    add(pause(0.2));
    add(pruefe("MB1 Auswahl: Rahmen waehlt alle gezeichneten Flaechen", [] {
        int gezeichnet = 0;
        for (const char g : g_app->mtab.gezeichnet) { gezeichnet += g != 0 ? 1 : 0; }
        diag::detail("Knopftest: MB1 Rahmen - " + std::to_string(g_app->mtab.auswahl.size()) + " von " + std::to_string(gezeichnet));
        return static_cast<int>(g_app->mtab.auswahl.size()) == gezeichnet && gezeichnet > 5;
    }));
    auto ecke = std::make_shared<modell::Ecke>();
    auto grob0 = std::make_shared<modell::Ecke>();
    add(tu("MB1 Gizmo: torso gewaehlt, Ansicht von der Seite, Taste W", [=] {
        g_app->mtab.auswahl = {teil("torso")};
        // MBKOPF: seit der Startansicht von vorne (Blick +Y) zeigt der Y-Griff in den Schirm - fuer den Zug von der Seite
        g_app->modelOrbit.turn(g_app->modelCam, 90.0F, 0.0F);
        g_app->modelDirty = true;
        *ecke = g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(teil("torso"))].stufen[0].ecken[0];
        *grob0 = g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(teil("torso"))].stufen.back().ecken[0];
    }));
    add(tasteKombi(ImGuiKey_W, "W"));
    add(pruefe("MB1 Gizmo: W - verschieben", [] { return g_app->mtab.gizmo == 1; }));
    // Y: in der Ansicht von der Seite (Blick nach -X) waagerecht auf dem Schirm.
    add(ziehenAn("am Y-Griff des Gizmos", [] {
        ImVec2 p{0.0F, 0.0F};
        (void)modellGizmoGriffOrt(2, p);
        return p;
    }, 60.0F, 0.0F, 0));
    add(pause(0.2));
    auto weg = std::make_shared<float>(0.0F);
    add(pruefe("MB1 Gizmo: torso entlang Y verschoben (alle LOD-Stufen), X und Z gleich", [=] {
        const modell::Teil& t = g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(teil("torso"))];
        const modell::Ecke& e = t.stufen[0].ecken[0];
        const modell::Ecke& grob = t.stufen.back().ecken[0];
        *weg = e.ort[1] - ecke->ort[1];
        diag::detail("Knopftest: MB1 Gizmo - Y um " + std::to_string(*weg) + ", Rueckgaengig: " + g_app->mtab.dok.rueckgaengigName());
        return std::fabs(*weg) > 0.5F && e.ort[0] == ecke->ort[0] && e.ort[2] == ecke->ort[2] && g_app->mtab.dok.kannRueckgaengig() &&
               std::fabs(grob.ort[1] - grob0->ort[1] - *weg) < 1e-4F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("MB1 Gizmo: Strg+Z - bitgleich wie vorher", [=] {
        const modell::Ecke& e = g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(teil("torso"))].stufen[0].ecken[0];
        return std::memcmp(&e, ecke.get(), sizeof e) == 0;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pruefe("MB1 Gizmo: Strg+Y - wieder verschoben", [=] {
        const modell::Ecke& e = g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(teil("torso"))].stufen[0].ecken[0];
        return std::fabs(e.ort[1] - ecke->ort[1] - *weg) < 1e-5F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_Q, "Q"));
    add(tu("MB1 Gizmo: Ansicht wieder von vorne", [] {
        g_app->modelOrbit.turn(g_app->modelCam, -90.0F, 0.0F);
        g_app->modelDirty = true;
    }));
    // --- Vier Ansichten -------------------------------------------------------------------
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_W, "Alt+W"));
    add(pause(0.3));
    add(pruefe("MB1 Ansichten: Alt+W - vier Ansichten (Oben, Vorne, Links, Perspektive), die Figur in jeder", [] {
        int sichtbar = 0;
        int mitFigur = 0;
        for (int i = 0; i < 4; ++i) {
            const ModellAnsicht& a = g_app->mtab.ansichten[i];
            sichtbar += a.sichtbar ? 1 : 0;
            ImVec2 p;
            if (modellPunktAufSchirm(i, g_app->model, p) && p.x > a.lo[0] && p.x < a.lo[0] + a.groesse[0] && p.y > a.lo[1] &&
                p.y < a.lo[1] + a.groesse[1]) {
                ++mitFigur;
            }
        }
        return g_app->mtab.vierAnsichten && sichtbar == 4 && mitFigur == 4 && g_app->mtab.ansichten[0].art == AnsichtArt::Oben &&
               g_app->mtab.ansichten[3].art == AnsichtArt::Perspektive;
    }));
    add(foto("mb1_vier_ansichten"));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_W, "Alt+W"));
    add(pause(0.2));
    add(pruefe("MB1 Ansichten: Alt+W - wieder eine", [] { return !g_app->mtab.vierAnsichten; }));
    // --- Bedienmodus --------------------------------------------------------------------
    add(klick(tr(Str::MbWieBlender), "###main"));
    add(pruefe("MB1 Bedienmodus: wie Blender gewaehlt (Einstellung)", [] { return g_app->settings.modellBedienung == 1; }));
    add(klick(tr(Str::MbWieMax), "###main"));
    add(pruefe("MB1 Bedienmodus: wieder wie 3ds Max", [] { return g_app->settings.modellBedienung == 0; }));
    add(tu("MB1: zurueck", [] {
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

// Je Taste eine Pruefzeile: Taste -> Aktion laut Tabelle (max_hotkeys_de.txt bzw. Doku) -> Wirkung.
std::vector<Schritt> tastenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("MB1 Tasten: Animation BOTH_WALK1, angehalten, torso gewaehlt", [] {
        animWaehlen(animNach("BOTH_WALK1") >= 0 ? animNach("BOTH_WALK1") : 0, false);
        g_app->animFrame += 3;
    }));
    add(klickAn("in die Ansicht (Fokus)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    add(tu("MB1 Tasten: torso gewaehlt", [] { g_app->mtab.auswahl = {teil("torso")}; }));
    struct Zeile {
        ImGuiKeyChord chord;
        const char* taste;
        const char* tabelle;   // Bezeichnung in max_hotkeys_de.txt (oder Doku)
        std::function<bool()> wirkung;
    };
    auto vorher = std::make_shared<std::array<int, 4>>();
    const auto merke = [=] {
        (*vorher)[0] = g_app->animFrame;
        (*vorher)[1] = static_cast<int>(g_app->mtab.auswahl.size());
        (*vorher)[2] = g_app->mtab.neuGebaut;
        (*vorher)[3] = g_app->mtab.dok.stand();
    };
    const std::vector<Zeile> zeilen = {
        {ImGuiKey_W, "W", "Auswaehlen und verschieben", [] { return g_app->mtab.gizmo == 1; }},
        {ImGuiKey_E, "E", "Auswaehlen und drehen", [] { return g_app->mtab.gizmo == 2; }},
        {ImGuiKey_R, "R", "Auswaehlen und skalieren", [] { return g_app->mtab.gizmo == 3; }},
        {ImGuiKey_Q, "Q", "Auswaehlen", [] { return g_app->mtab.gizmo == 0; }},
        {ImGuiMod_Alt | ImGuiKey_W, "Alt+W", "Ansichtsfenster maximieren ein/aus", [] { return g_app->mtab.vierAnsichten; }},
        {ImGuiMod_Alt | ImGuiKey_W, "Alt+W", "Ansichtsfenster maximieren ein/aus", [] { return !g_app->mtab.vierAnsichten; }},
        {ImGuiKey_T, "T", "Ansicht von oben", [] { return g_app->mtab.ansichten[g_app->mtab.aktiv].art == AnsichtArt::Oben; }},
        {ImGuiKey_F, "F", "Ansicht von vorn", [] { return g_app->mtab.ansichten[g_app->mtab.aktiv].art == AnsichtArt::Vorne; }},
        {ImGuiKey_L, "L", "Ansicht von links", [] { return g_app->mtab.ansichten[g_app->mtab.aktiv].art == AnsichtArt::Links; }},
        {ImGuiKey_B, "B", "Ansicht von unten", [] { return g_app->mtab.ansichten[g_app->mtab.aktiv].art == AnsichtArt::Unten; }},
        {ImGuiKey_P, "P", "Perspektiv-Benutzeransicht", [] { return g_app->mtab.ansichten[g_app->mtab.aktiv].art == AnsichtArt::Perspektive; }},
        {ImGuiKey_Z, "Z", "Zoom Grenzen: Alle ausgewaehlten", [] {
             // eingepasst auf torso: die Kamera zielt auf die Mitte der Auswahl
             float m[3];
             return modellAuswahlMitte(m) && std::fabs(g_app->modelOrbit.distance) < 200.0F;
         }},
        {ImGuiKey_F3, "F3", "Drahtgittermodell/Glatt ein/aus", [] { return modellSch().drahtgitter; }},
        {ImGuiKey_F3, "F3", "Drahtgittermodell/Glatt ein/aus", [] { return !modellSch().drahtgitter; }},
        {ImGuiKey_F4, "F4", "Flaechen mit Kanten anzeigen ein/aus", [] { return modellSch().kanten; }},
        {ImGuiKey_F4, "F4", "Flaechen mit Kanten anzeigen ein/aus", [] { return !modellSch().kanten; }},
        {ImGuiKey_F5, "F5", "Auf X beschraenken", [] { return g_app->mtab.achse == 0; }},
        {ImGuiKey_F6, "F6", "Auf Y beschraenken", [] { return g_app->mtab.achse == 1; }},
        {ImGuiKey_F7, "F7", "Auf Z beschraenken", [] { return g_app->mtab.achse == 2; }},
        {ImGuiKey_F8, "F8", "Beschraenkungsebenen durchgehen", [] { return g_app->mtab.achse == 3; }},
        {ImGuiKey_F8, "F8", "Beschraenkungsebenen durchgehen", [] { return g_app->mtab.achse == 4; }},
        {ImGuiKey_Equal, "+", "Groesse des Transformations-Gizmo erhoehen", [] { return g_app->mtab.gizmoGroesse > 1.2F; }},
        {ImGuiKey_Minus, "-", "Groesse des Transformations-Gizmo verringern", [] { return std::fabs(g_app->mtab.gizmoGroesse - 1.0F) < 1e-4F; }},
        {ImGuiMod_Ctrl | ImGuiKey_D, "Strg+D", "Nichts auswaehlen", [] { return g_app->mtab.auswahl.empty(); }},
        {ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A", "Alles auswaehlen", [] {
             int n = 0;
             for (const char g : g_app->mtab.gezeichnet) { n += g != 0 ? 1 : 0; }
             return static_cast<int>(g_app->mtab.auswahl.size()) == n && n > 0;
         }},
        {ImGuiMod_Ctrl | ImGuiKey_I, "Strg+I", "Auswahl invertieren (Doku)", [] { return g_app->mtab.auswahl.empty(); }},
        {ImGuiKey_Period, ".", "Zeit um eine Einheit vorspulen", [=] { return g_app->animFrame == (*vorher)[0] + 1; }},
        {ImGuiKey_Comma, ",", "Zeit um eine Einheit zurueckspulen", [=] { return g_app->animFrame == (*vorher)[0] - 1; }},
        {ImGuiKey_End, "Ende", "Gehe zu End-Frame", [] {
             const AnimEntry& a = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
             return g_app->animFrame == a.firstFrame + a.numFrames - 1;
         }},
        {ImGuiKey_Home, "Pos1", "Gehe zu Start-Frame", [] { return g_app->animFrame == g_app->animList[static_cast<std::size_t>(g_app->animIndex)].firstFrame; }},
        {ImGuiKey_Slash, "/", "Abspielen (Doku)", [] { return g_app->animPlaying; }},
        {ImGuiKey_Slash, "/", "Anhalten (Doku)", [] { return !g_app->animPlaying; }},
        {ImGuiKey_G, "G", "Raster ein/aus", [] { return modellSch().raster; }},
        {ImGuiKey_G, "G", "Raster ein/aus", [] { return !modellSch().raster; }},
        {ImGuiKey_J, "J", "Auswahlklammer zeigen ein/aus", [] { return !g_app->mtab.an.klammer; }},
        {ImGuiKey_J, "J", "Auswahlklammer zeigen ein/aus", [] { return g_app->mtab.an.klammer; }},
        {ImGuiKey_7, "7", "Ansichtsfenster-Statistiken anzeigen ein/aus", [] { return modellSch().statistik; }},
        {ImGuiKey_7, "7", "Ansichtsfenster-Statistiken anzeigen ein/aus", [] { return !modellSch().statistik; }},
        {ImGuiMod_Alt | ImGuiKey_R, "Alt+R", "ModView: Texturen neu laden", [] { return g_app->modelTextures.found > 0; }},
        {ImGuiKey_X, "X", "Alle Aktionen suchen", [] { return g_app->mtab.aktionenSichtbar; }},
        {ImGuiKey_Escape, "Esc", "(schliesst die Suche)", [] { return !g_app->mtab.aktionenSichtbar; }},
        {ImGuiKey_H, "H", "Nach Namen auswaehlen", [] { return ImGui::GetIO().WantTextInput; }},
    };
    for (const Zeile& z : zeilen) {
        if (z.chord == ImGuiKey_Equal) {
            add(tu("MB1 Tasten: Gizmogroesse 1", [] { g_app->mtab.gizmoGroesse = 1.0F; }));
        }
        if (z.chord == ImGuiKey_Z) {
            add(tu("MB1 Tasten: Kamera weit weg, torso gewaehlt", [] {
                g_app->mtab.auswahl = {teil("torso")};
                g_app->modelOrbit.distance = 900.0F;
                modellAnsichtEinpassenAktiv();   // nur die Perspektive wieder auf Ausgang
                g_app->modelOrbit.distance = 900.0F;
            }));
        }
        add(tu(std::string("MB1 Tasten: vor ") + z.taste, merke));
        add(tasteKombi(z.chord, z.taste));
        add(pause(0.1));
        const std::string zeile = std::string(z.taste) + " = " + z.tabelle;
        add(pruefe("MB1 Taste " + zeile, [=] {
            const bool ok = z.wirkung();
            diag::detail(std::string("Knopftest: MB1 Taste ") + zeile + (ok ? " - wirkt" : " - wirkt NICHT") + " (letzte: " +
                         g_app->mtab.letzteTaste + ")");
            return ok;
        }));
    }
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe(std::string("MB1 Tasten: Liste des Modus hat ") + "jede gepruefte Taste", [] { return modellTasteZahl() >= 30; }));
    add(tu("MB1 Tasten: zurueck", [] {
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

// Bilder fuer die Handbuchseite (BHED_FOTOS): Ansehen/Pruefen mit Explorer und Befehlsfeld, Rechtsklick-Menue,
// Pruefhilfen, vier Ansichten mit Knochen.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    add(tu("MB1 Handbuch: breite Anordnung (Vorgabe neuer Einstellungen), Saeble an *r_hand, BOTH_STAND1", [=] {
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = 0.85F;
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(animNach("BOTH_STAND1") >= 0 ? animNach("BOTH_STAND1") : 0, false);
        g_app->mtab.auswahl = {teil("torso")};
        g_app->mtab.an.ursprung = true;
    }));
    add(pause(0.5));
    add(tu("MB1 Handbuch: Zoom Grenzen", [] {
        g_app->mtab.auswahl.clear();
        modellAnsichtEinpassenAktiv();
        g_app->mtab.auswahl = {teil("torso")};
    }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_mb1_ansehen"));
    add(tu("MB1 Handbuch: im Explorer nach torso suchen", [] { std::snprintf(g_app->mtab.suche, sizeof g_app->mtab.suche, "%s", "torso"); }));
    add(pause(0.3));
    add(klick("torso", "", 0, false, "", 1));
    add(pause(0.3));
    add(hilfe2::schrittFoto("h_mb1_menue"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MB1 Handbuch: vier Ansichten, Knochen", [] {
        g_app->mtab.suche[0] = 0;
        g_app->mtab.vierAnsichten = true;
        g_app->modellKnochen = true;
        g_app->mtab.an.tags = false;
        g_app->mtab.auswahl.clear();
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_mb1_vier"));
    add(tu("MB1 Handbuch: zurueck", [=] {
        g_app->settings.splitModel = *teiler;
        g_app->mtab.vierAnsichten = false;
        g_app->modellKnochen = false;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte() {
    const char* nur = std::getenv("BHED_MODELL_NUR");
    const std::string n = nur != nullptr ? nur : "";
    std::vector<Schritt> s;
    if (n == "bild") {
        const char* sp = std::getenv("BHED_MB_SPIEL");
        const std::string spiel = sp != nullptr ? sp : "md";
        s.push_back(tu("MB1 Bild: Spiel " + spiel, [=] {
            g_app->leftMode = 2;
            openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb1.txt");
            if (spiel != "md") { spielSetzen(spiel); }
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        }));
        s.push_back(pause(0.5));
        s.push_back(bildVergleich(spiel));
        s.push_back(tu("MB1 Bild: zurueck", [] {
            g_app->leftMode = 1;
            g_app->mtab.lodWahl = 0;   // wieder automatisch
            g_app->doc.markSaved();
        }));
        return s;
    }
    if (n == "mb2" || n == "blender" || n == "bild2") { return {}; }   // mb1a2: selbsttest_mb1a2.inc
    if (n.empty() || n == "funktionen") {
        for (Schritt& x : funktionenSchritte()) { s.push_back(std::move(x)); }
    }
    if (n.empty() || n == "tasten") {
        for (Schritt& x : tastenSchritte()) { s.push_back(std::move(x)); }
    }
    return s;
}

}  // namespace mb1
