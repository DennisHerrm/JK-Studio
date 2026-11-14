// selbsttest_mb2.inc - Modell-Block Stufe 2: GLM-/GLA-Schreiber, animation.cfg, Karten-GLA, Plaetze
// (BHED_EDITORTEST=modellblock)
//
//   BHED_MODELL_NUR=rundlauf    jede .glm und .gla des Spiels (BHED_MB_SPIEL=jka|jk2|md, Vorgabe md), wie das Spiel sie
//                               findet: lesen -> Dokument -> schreiben -> Byte fuer Byte gegen die Datei; was abweicht,
//                               mit dem ersten verschiedenen Feld und neu gelesen gegen das Original (alles, was readGlm
//                               zum Zeichnen liefert). Bericht in mb3_rundlauf_<spiel>.txt im Laufordner. Dazu die
//                               Frame-Zahl von models/players/_humanoid/_humanoid.gla und ihrer animation.cfg.
//   BHED_MODELL_NUR=mb3         Model-Tab: Speichern/Speichern unter (Strg+S), Grenzpruefung mit Bestaetigung im
//                               Programm, Bereich Animation (GLA, animation.cfg, Pruefungen, Karten-GLA, Plaetze, Ziehen),
//                               Marker in der Zeitleiste bearbeiten; erreichbar ueber Menue, Suche, Rechtsklick, Tooltips.
// Eingebunden in selbsttest.cpp (nach selbsttest_mb1a2.inc).
namespace mb3 {

// Alles, was readGlm zum Zeichnen liefert, gleich?
bool glmDatenGleich(const GlmModel& a, const GlmModel& b, std::string& wo) {
    const auto ecken = [](const std::vector<GlmVertex>& x, const std::vector<GlmVertex>& y) {
        if (x.size() != y.size()) { return false; }
        for (std::size_t v = 0; v < x.size(); ++v) {
            if (std::memcmp(x[v].xyz, y[v].xyz, sizeof x[v].xyz) != 0 || std::memcmp(x[v].normal, y[v].normal, sizeof x[v].normal) != 0 ||
                std::memcmp(x[v].st, y[v].st, sizeof x[v].st) != 0 || std::memcmp(x[v].bones, y[v].bones, sizeof x[v].bones) != 0 ||
                std::memcmp(x[v].weights, y[v].weights, sizeof x[v].weights) != 0) {
                return false;
            }
        }
        return true;
    };
    if (a.name != b.name || a.animFile != b.animFile || a.numBones != b.numBones || a.numLods != b.numLods || a.surfaces.size() != b.surfaces.size()) {
        wo = "Kopf";
        return false;
    }
    for (std::size_t s = 0; s < a.surfaces.size(); ++s) {
        const GlmSurface& x = a.surfaces[s];
        const GlmSurface& y = b.surfaces[s];
        if (x.name != y.name || x.shader != y.shader || x.flags != y.flags || x.parent != y.parent || x.indexes != y.indexes ||
            x.grobIndexes != y.grobIndexes || !ecken(x.verts, y.verts) || !ecken(x.grobVerts, y.grobVerts)) {
            wo = "Flaeche " + x.name;
            return false;
        }
    }
    return true;
}

Schritt rundlauf(const std::string& spiel) {
    struct Zustand {
        std::vector<std::string> liste;
        std::size_t i = 0;
        int glm = 0, glmGleich = 0, glmDaten = 0, glmAnders = 0, glmNichtLesbar = 0;
        int gla = 0, glaGleich = 0, glaAnders = 0, glaNichtLesbar = 0;
        std::string bericht;
        std::chrono::steady_clock::time_point anfang{};
    };
    auto z = std::make_shared<Zustand>();
    Schritt s{"MB3 Rundlauf: jede .glm und .gla (" + spiel + ") lesen, schreiben, vergleichen", [=](int b) {
                  if (b == 0) {
                      for (const GamePath& gp : g_app->gamePaths) {
                          for (const FoundFile& f : findByExtension(gp, {".glm", ".gla"})) { z->liste.push_back(f.name); }
                      }
                      std::sort(z->liste.begin(), z->liste.end());
                      z->liste.erase(std::unique(z->liste.begin(), z->liste.end()), z->liste.end());
                      z->anfang = std::chrono::steady_clock::now();
                      diag::detail("Knopftest: MB3 Rundlauf " + spiel + " - " + std::to_string(z->liste.size()) + " Dateien");
                      return false;
                  }
                  // je Bild einige Dateien, damit das Fenster lebt (die grossen GLAs von Movie Duels haben 26 MB)
                  for (int n = 0; n < 4 && z->i < z->liste.size(); ++n, ++z->i) {
                      const std::string& pfad = z->liste[z->i];
                      std::string bytes;
                      if (!readFromArchives(pfad, bytes)) { continue; }
                      const bool istGla = pfad.size() > 4 && (pfad.compare(pfad.size() - 4, 4, ".gla") == 0 || pfad.compare(pfad.size() - 4, 4, ".GLA") == 0);
                      std::string aus;
                      std::string fehler;
                      if (istGla) {
                          ++z->gla;
                          GlaAnimation a;
                          if (!readGla(bytes, a, &fehler) || !schreibGla(a, aus, &fehler)) {
                              ++z->glaNichtLesbar;
                              z->bericht += "GLA nicht lesbar/schreibbar: " + pfad + " (" + fehler + ")\n";
                              continue;
                          }
                          if (aus == bytes) {
                              ++z->glaGleich;
                          } else {
                              ++z->glaAnders;
                              std::size_t d = 0;
                              while (d < aus.size() && d < bytes.size() && aus[d] == bytes[d]) { ++d; }
                              z->bericht += "GLA anders: " + pfad + " (Byte " + std::to_string(d) + ", Groesse " + std::to_string(bytes.size()) +
                                            " -> " + std::to_string(aus.size()) + ")\n";
                          }
                          continue;
                      }
                      ++z->glm;
                      modell::Inhalt in;
                      modell::GlmSchreibBericht br;
                      if (!modell::ausGlm(bytes, in, &fehler) || !modell::glmSchreiben(in, aus, &br)) {
                          ++z->glmNichtLesbar;
                          z->bericht += "GLM nicht lesbar/schreibbar: " + pfad + " (" + fehler + br.fehler + ")\n";
                          continue;
                      }
                      if (aus == bytes) {
                          ++z->glmGleich;
                          continue;
                      }
                      // anders: neu gelesen gegen das Original, Stufe fuer Stufe
                      bool daten = true;
                      std::string wo;
                      for (int st = 0; st < std::max(in.stufenZahl, 1) && daten; ++st) {
                          GlmModel x;
                          GlmModel y;
                          daten = readGlm(bytes, x, nullptr, st) && readGlm(aus, y, nullptr, st) && glmDatenGleich(x, y, wo);
                      }
                      (daten ? z->glmDaten : z->glmAnders) += 1;
                      z->bericht += std::string(daten ? "GLM anders, neu gelesen gleich: " : "GLM anders: ") + pfad + " - " +
                                    modell::glmUnterschied(bytes, aus) + (daten ? "" : ", neu gelesen anders: " + wo) + "\n";
                  }
                  if (z->i < z->liste.size()) { return false; }
                  const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - z->anfang).count();
                  // die Frame-Zahl der humanoiden GLA (G2C-README: 21.376 gegen 30.384)
                  std::string hum;
                  std::string cfg;
                  GlaAnimation h;
                  if (readFromArchives("models/players/_humanoid/_humanoid.gla", hum) && readGla(hum, h)) {
                      const bool hatCfg = readFromArchives("models/players/_humanoid/animation.cfg", cfg);
                      int bisEnde = 0;
                      for (const AnimEntry& e : parseAnimationCfg(cfg)) { bisEnde = std::max(bisEnde, e.firstFrame + e.numFrames); }
                      z->bericht += "_humanoid.gla: " + std::to_string(h.numFrames) + " Frames, " + std::to_string(h.bones.size()) +
                                    " Knochen; animation.cfg " + (hatCfg ? std::to_string(parseAnimationCfg(cfg).size()) + " Sequenzen bis Frame " +
                                                                               std::to_string(bisEnde) + ", erste Zeile: " + cfg.substr(0, cfg.find('\n'))
                                                                         : std::string("fehlt")) + "\n";
                  }
                  char zeile[400];
                  std::snprintf(zeile, sizeof zeile,
                                "MB3 Rundlauf %s: GLM %d (gleich %d, anders mit gleichen Daten %d, anders %d, nicht lesbar %d), GLA %d (gleich %d, "
                                "anders %d, nicht lesbar %d), %.0f s",
                                spiel.c_str(), z->glm, z->glmGleich, z->glmDaten, z->glmAnders, z->glmNichtLesbar, z->gla, z->glaGleich,
                                z->glaAnders, z->glaNichtLesbar, sek);
                  diag::detail(std::string("Knopftest: ") + zeile);
                  {
                      std::ofstream f("mb3_rundlauf_" + spiel + ".txt", std::ios::binary);
                      f << zeile << "\n" << z->bericht;
                  }
                  pruefeWahr(z->glm > 0 && z->glmAnders == 0, "MB3 Rundlauf " + spiel + ": jede lesbare .glm neu geschrieben gleich (Bytes oder Daten)");
                  pruefeWahr(z->gla > 0 && z->glaAnders == 0, "MB3 Rundlauf " + spiel + ": jede lesbare .gla Byte fuer Byte gleich");
                  return true;
              }};
    s.eigeneZeitgrenze = true;
    return s;
}

// Ein Element nach dem Anfang seiner Beschriftung (die Plaetze heissen "BOTH_CIN_3   frei###platz").
const Element* findeAnfang(const std::string& anfang) {
    for (const Element& e : g_elemente) {
        if (e.label.compare(0, anfang.size(), anfang) == 0) { return &e; }
    }
    return nullptr;
}

// Mit der linken Taste von a nach b ziehen (Ziehen und Ablegen von ImGui: erst ab der Schwelle, dann Ziel).
Schritt ziehenVonNach(const std::string& was, std::function<ImVec2()> a, std::function<ImVec2()> b) {
    auto von = std::make_shared<ImVec2>();
    auto nach = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int bild) {
                ImGuiIO& io = ImGui::GetIO();
                if (bild == 0) {
                    *von = a();
                    *nach = b();
                    setzeMaus(*von);
                    return false;
                }
                if (bild == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (bild >= 3 && bild <= 20) {
                    const float t = static_cast<float>(bild - 2) / 18.0F;
                    setzeMaus(ImVec2{von->x + (nach->x - von->x) * t, von->y + (nach->y - von->y) * t});
                    return false;
                }
                if (bild == 24) { io.AddMouseButtonEvent(0, false); return false; }
                return bild >= 30;
            }};
}

std::string lauf(const std::string& n) { return (std::filesystem::current_path() / n).string(); }

std::string datei(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

int platzVon(const std::string& name) {
    for (std::size_t i = 0; i < g_app->mtab.plaetze.size(); ++i) {
        if (g_app->mtab.plaetze[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

std::vector<Schritt> uiSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string pfad = "models/players/stormtrooper/model.glm";
    add(tu("MB3: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb3.txt"); }));
    for (Schritt& x : mb1::laden(pfad)) { add(std::move(x)); }
    add(klickAn("in die Ansicht (Fokus)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    // --- Speichern unter: unveraendert Byte fuer Byte die Datei der Archive -----------------------------------------
    add(tu("MB3 Speichern: Antwort des Dialogs mb3_sturm.glm", [] { platform::dialogAntwortImSelbsttest(lauf("mb3_sturm.glm")); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, "Strg+Umschalt+S"));
    add(pause(0.3));
    add(pruefe("MB3 Speichern unter: das unveraenderte Modell ist Byte fuer Byte die Datei der Archive", [=] {
        std::string orig;
        const bool ok = readFromArchives(pfad, orig) && datei(lauf("mb3_sturm.glm")) == orig;
        diag::detail("Knopftest: MB3 Speichern - " + g_app->mtab.meldung);
        return ok && !modellUngespeichert() && g_app->mtab.speicherPfad == lauf("mb3_sturm.glm");
    }));
    // --- Gewicht aendern, Strg+S ohne Dialog, neu gelesen wie das Dokument --------------------------------------------
    add(tu("MB3 Speichern: an hips Ecke 0 die Gewichte 0.6/0.4 (Uebernehmen-Werte)", [] {
        g_app->mtab.dok.aendern("Test", [](modell::Inhalt& in) {
            const int t = in.teilIndex("hips");
            if (t < 0) { return; }
            modell::Ecke& e = in.teile[static_cast<std::size_t>(t)].stufen[0].ecken[0];
            modell::GewichtListe l{{e.knochen[0], 0.6F}, {e.knochen[0] == 1 ? 2 : 1, 0.4F}};
            (void)modell::aufEcke(l, e);
        });
    }));
    add(pruefe("MB3: geaendert ist ungespeichert", [] { return modellUngespeichert(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_S, "Strg+S"));
    add(pause(0.3));
    add(pruefe("MB3 Strg+S: in dieselbe Datei, neu gelesen traegt hips Ecke 0 die Gewichte des Dokuments, sonst alles gleich", [] {
        GlmModel neu;
        if (!readGlm(datei(lauf("mb3_sturm.glm")), neu)) { return false; }
        const GlmModel soll = modell::zuGlm(g_app->mtab.dok.inhalt(), 0);
        std::string wo;
        const bool gleich = glmDatenGleich(soll, neu, wo);
        if (!gleich) { diag::detail("Knopftest: MB3 Strg+S - anders: " + wo); }
        return gleich && !modellUngespeichert();
    }));
    // --- Grenzen: eine Flaeche mit 1001 Ecken fuer JKA SP -> Frage im Programm ------------------------------------------
    auto vorher = std::make_shared<std::string>();
    add(tu("MB3 Grenzen: Zielspiel JKA SP, hips mit 1001 Ecken", [=] {
        *vorher = datei(lauf("mb3_sturm.glm"));
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        g_app->mtab.dok.aendern("Test", [](modell::Inhalt& in) {
            const int t = in.teilIndex("hips");
            modell::Netz& n = in.teile[static_cast<std::size_t>(t)].stufen[0];
            std::vector<modell::Ecke> e = n.ecken;
            std::vector<std::uint32_t> idx = n.indizes();
            while (e.size() < 1001U) { e.push_back(e.front()); }
            n.aufbauen(std::move(e), idx);
        });
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_S, "Strg+S"));
    add(warteBis("MB3 Grenzen: die Frage im Programm ist offen", [] { return g_app->mtab.frageSichtbar; }, 300));
    add(pruefe("MB3 Grenzen: vor der Antwort nichts geschrieben, die Frage nennt hips und 1000", [=] {
        bool hips = false;
        for (const std::string& z : g_app->mtab.frageZeilen) { hips = hips || (z.find("hips") != std::string::npos && z.find("1000") != std::string::npos); }
        return g_app->mtab.frageArt == 1 && hips && datei(lauf("mb3_sturm.glm")) == *vorher;
    }));
    add(klick(tr(Str::MbSpTrotzdem), "###mbfrage"));
    add(pause(0.3));
    add(pruefe("MB3 Grenzen: Trotzdem speichern schreibt die 1001 Ecken", [] {
        GlmModel neu;
        if (!readGlm(datei(lauf("mb3_sturm.glm")), neu)) { return false; }
        const int t = surfaceIndex(neu, "hips");
        return t >= 0 && neu.surfaces[static_cast<std::size_t>(t)].verts.size() == 1001U && !g_app->mtab.frageSichtbar;
    }));
    add(tu("MB3: Rueckgaengig bis zum Laden, Zielspiel Movie Duels SP", [] {
        while (g_app->mtab.dok.rueckgaengig()) {}
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::MdSp);
    }));
    // --- Leicht zu finden: Menue, Suche, Rechtsklick, Tooltips ---------------------------------------------------------
    add(pruefe("MB3 Finden: Speichern, Speichern unter, GLA, animation.cfg, Karten-GLA, freier Platz in Suche und Rechtsklick, je mit Tooltip", [] {
        const Str soll[] = {Str::MbDateiSpeichern, Str::MbDateiSpeichernUnter, Str::MbGlaSpeichern, Str::MbCfgSpeichern, Str::MbKgBauen,
                            Str::MbKgVorhanden, Str::MbKgAufFreien};
        int da = 0;
        for (const Str x : soll) {
            for (int i = 0; i < modellZusatzZahl(); ++i) {
                if (std::string(modellZusatzText(i)) == tr(x) && modellZusatzHinweis(i)[0] != '\0') { ++da; }
            }
        }
        return da == 7;
    }));
    // --- Bereich Animation: GLA, animation.cfg ohne Befund, Marker --------------------------------------------------------
    add(tu("MB3 Animation: Bereich Animation, BOTH_WALK1", [] {
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1"), true);
    }));
    add(pause(0.2));
    // TEST1411 N: GLA, animation.cfg, Ziel-GLA und Plaetze stehen im Animationsmodus wie in 3ds Max im Reiter Dienstprogramme
    add(tu("MB3 Animation: Reiter Dienstprogramme", [] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten); }));
    add(pause(0.6));
    add(pruefe("MB3 Animation: die animation.cfg der GLA im Dokument, keine Befunde fuer Movie Duels SP", [] {
        diag::detail("Knopftest: MB3 cfg " + g_app->mtab.dok.inhalt().cfgPfad + ", Befunde " + std::to_string(g_app->mtab.cfgBefunde));
        return !g_app->mtab.dok.inhalt().cfgPfad.empty() && g_app->mtab.cfgBefunde == 0 &&
               g_app->mtab.dok.inhalt().cfg.eintraege().size() == g_app->animList.size();
    }));
    auto walk = std::make_shared<AnimEntry>();
    add(tu("MB3 Marker: BOTH_WALK1 eine Zahl mehr", [=] {
        *walk = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        AnimEntry w = *walk;
        w.numFrames += 1;
        (void)modellMarkerAendern(w);
    }));
    add(pause(0.2));
    add(pruefe("MB3 Marker: Animationsliste, Zeile der cfg und Rueckgaengig-Schritt", [=] {
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        const int z = g_app->mtab.dok.inhalt().cfg.zeileVon("BOTH_WALK1");
        return e.numFrames == walk->numFrames + 1 && z >= 0 && g_app->mtab.dok.inhalt().cfg.zeilen[static_cast<std::size_t>(z)].geaendert &&
               g_app->mtab.dok.rueckgaengigName().find("BOTH_WALK1") != std::string::npos;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("MB3 Marker: Strg+Z stellt die Zeile zurueck", [=] {
        return g_app->animList[static_cast<std::size_t>(g_app->animIndex)].numFrames == walk->numFrames;
    }));
    // AP1: die Zeile der animation.cfg (Anfang, Anzahl, Schleife, fps, Balken) steht im Fenster Zeitkonfiguration
    add(tu("MB3 Marker: Zeitkonfiguration oeffnen", [] { modellZeit().konfigOffen = true; }));
    add(pause(0.4));
    add(ziehenVonNach("das rechte Ende des Markerbalkens 40 Punkte nach rechts", [] {
        const Element* e = findeEndung("##mkbalken", "");
        if (e == nullptr) { return ImVec2{}; }
        // das Ende bei firstFrame + numFrames: der Balken reicht von firstFrame - rand bis Ende + rand
        const AnimEntry& w = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        const float rand = static_cast<float>(std::max(w.numFrames / 2, 10));
        const float von = std::max(static_cast<float>(w.firstFrame) - rand, 0.0F);
        const float bis = static_cast<float>(w.firstFrame + w.numFrames) + rand;
        const float x = e->rect.Min.x + (static_cast<float>(w.firstFrame + w.numFrames) - von) / (bis - von) * e->rect.GetWidth();
        return ImVec2{x, e->rect.GetCenter().y};
    }, [] {
        const Element* e = findeEndung("##mkbalken", "");
        return e != nullptr ? ImVec2{e->rect.Max.x - 2.0F, e->rect.GetCenter().y} : ImVec2{};
    }));
    add(pause(0.2));
    add(pruefe("MB3 Marker: das Ende gezogen macht die Sequenz laenger (ein Schritt)", [=] {
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        diag::detail("Knopftest: MB3 Marker gezogen - Anzahl " + std::to_string(walk->numFrames) + " -> " + std::to_string(e.numFrames));
        return e.numFrames > walk->numFrames && e.firstFrame == walk->firstFrame;
    }));
    add(tu("MB3 Marker: Zeitkonfiguration zu", [] { modellZeit().konfigOffen = false; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Rot: eine Zeile hinter dem Ende der GLA muss die Pruefung melden (T-Pose in OpenJK-Fassungen).
    add(tu("MB3 Pruefen: die letzte Sequenz zehn Frames hinter das Ende der GLA", [] {
        const std::vector<AnimEntry> e = g_app->mtab.dok.inhalt().cfg.eintraege();
        AnimEntry w = e.back();
        w.numFrames = g_app->anim.numFrames - w.firstFrame + 10;
        (void)modellMarkerAendern(w);
    }));
    add(pause(0.3));
    add(pruefe("MB3 Pruefen: animation.cfg mit einem Frame hinter dem Ende wird gemeldet", [] { return g_app->mtab.cfgBefunde >= 1; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB3 Pruefen: nach Strg+Z kein Befund", [] { return g_app->mtab.cfgBefunde == 0; }));
    // --- Plaetze: Ziehen aus der Animationsliste, Rechtsklick-Weg, Karten-GLA bauen ------------------------------------------
    add(tu("MB3 Plaetze: Karte mb3test, die Zeile BOTH_WALK1 ins Bild", [] {
        std::snprintf(g_app->mtab.kgKarte, sizeof g_app->mtab.kgKarte, "%s", "mb3test");
        animWaehlen(mb1::animNach("BOTH_WALK1"), true);
        g_app->animScrollen = true;
    }));
    add(pause(0.5));
    add(tu("MB3 Plaetze: das Befehlsfeld zu den Plaetzen rollen (darueber stehen seit Stufe 3 die Rig-Rollouts)", [] {
        const Element* e = finde(tr(Str::MbRKartenGla), "", 0, false);
        for (ImGuiWindow* w : GImGui->Windows) {
            if (e != nullptr && std::strstr(w->Name, "mtrollouts") != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y, 0.0F, w->ScrollMax.y));
            }
        }
    }));
    add(pause(0.5));
    add(ziehenVonNach("BOTH_WALK1 auf den Platz BOTH_CIN_3", [] {
        const Element* e = finde("BOTH_WALK1", "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, [] {
        const Element* e = findeAnfang("BOTH_CIN_3 ");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }));
    add(pause(0.2));
    add(pruefe("MB3 Plaetze: BOTH_CIN_3 traegt BOTH_WALK1 (gezogen), die anderen frei", [] {
        const int p = platzVon("BOTH_CIN_3");
        int belegt = 0;
        for (const modell::Platz& x : g_app->mtab.plaetze) { belegt += x.belegt ? 1 : 0; }
        return p >= 0 && g_app->mtab.plaetze[static_cast<std::size_t>(p)].belegt &&
               g_app->mtab.plaetze[static_cast<std::size_t>(p)].quelle.sequenz.name == "BOTH_WALK1" && belegt == 1 &&
               g_app->mtab.plaetze.size() == static_cast<std::size_t>(modell::cfgGrenzen(modell::Zielspiel::MdSp).kinoPlaetze);
    }));
    add(tu("MB3 Plaetze: BOTH_RUN1 auf den naechsten freien Platz (Rechtsklick-Weg)", [] { (void)modellAufFreienPlatz(mb1::animNach("BOTH_RUN1")); }));
    add(pruefe("MB3 Plaetze: BOTH_CIN_1 traegt BOTH_RUN1", [] {
        return g_app->mtab.plaetze[0].belegt && g_app->mtab.plaetze[0].quelle.sequenz.name == "BOTH_RUN1";
    }));
    add(tu("MB3 Karten-GLA: Antwort des Dialogs", [] {
        std::filesystem::create_directories(lauf("mb3_karte"));
        (void)modellKartenGlaBauen(lauf("mb3_karte\\_humanoid_mb3test.gla"), false);
    }));
    add(pause(0.3));
    add(pruefe("MB3 Karten-GLA: geschrieben, Frames = beide Sequenzen, Name im Kopf = Ordner, Zeilen BOTH_CIN_1 und _3, kein Befund", [] {
        GlaAnimation k;
        const std::string cfg = datei(lauf("mb3_karte\\animation.cfg"));
        if (!readGla(datei(lauf("mb3_karte\\_humanoid_mb3test.gla")), k)) { return false; }
        const std::vector<AnimEntry> z = parseAnimationCfg(cfg);
        const int walk = mb1::animNach("BOTH_WALK1");
        const int run = mb1::animNach("BOTH_RUN1");
        const int soll = g_app->animList[static_cast<std::size_t>(walk)].numFrames + g_app->animList[static_cast<std::size_t>(run)].numFrames;
        diag::detail("Knopftest: MB3 Karten-GLA - " + g_app->mtab.meldung);
        return k.numFrames == soll && k.skeletonName == "models/players/_humanoid_mb3test/_humanoid_mb3test" && k.bones.size() == 53U &&
               z.size() == 2U && z[0].name == "BOTH_CIN_1" && z[0].firstFrame == 0 && z[1].name == "BOTH_CIN_3" &&
               modell::kartenGlaPruefen(k, modell::kartenGlaPfad("_humanoid", "mb3test"), modell::Zielspiel::MdSp).empty() && !g_app->mtab.frageSichtbar;
    }));
    add(pruefe("MB3 Karten-GLA: BOTH_CIN_3 traegt Bild fuer Bild die Eintraege von BOTH_WALK1", [] {
        GlaAnimation k;
        if (!readGla(datei(lauf("mb3_karte\\_humanoid_mb3test.gla")), k)) { return false; }
        const AnimEntry& w = g_app->animList[static_cast<std::size_t>(mb1::animNach("BOTH_WALK1"))];
        const std::vector<AnimEntry> z = parseAnimationCfg(datei(lauf("mb3_karte\\animation.cfg")));
        const GlaAnimation& q = g_app->anim;
        bool gleich = z.size() == 2U && z[1].numFrames == w.numFrames;
        for (int f = 0; f < w.numFrames && gleich; ++f) {
            for (std::size_t b = 0; b < k.bones.size() && gleich; ++b) {
                const auto qi = q.indexes[static_cast<std::size_t>(w.firstFrame + f) * q.bones.size() + b];
                const auto ki = k.indexes[static_cast<std::size_t>(z[1].firstFrame + f) * k.bones.size() + b];
                gleich = std::memcmp(q.bonePool.data() + qi * 14U, k.bonePool.data() + ki * 14U, 14) == 0;
            }
        }
        return gleich;
    }));
    // --- Nachtrag shank: dieselbe Animation in alle drei Ziele, neu laden, gleich abspielen ------------------------------------
    add(tu("MB3 Ziele: nur BOTH_WALK1 auf BOTH_CIN_3, eigener Name mb3skelett", [] {
        modellPlatzLeeren(0);
        std::snprintf(g_app->mtab.eigenName, sizeof g_app->mtab.eigenName, "%s", "mb3skelett");
    }));
    for (int ziel = 0; ziel < 3; ++ziel) {
        const std::string ordner = "mb3_ziel" + std::to_string(ziel);
        add(tu("MB3 Ziel " + std::to_string(ziel) + ": bauen", [=] {
            modellGlaZielSetzen(ziel);
            std::filesystem::create_directories(lauf(ordner));
            const std::string n = ziel == 0 ? "_humanoid_mb3test.gla" : ziel == 1 ? "_humanoid.gla" : "mb3skelett.gla";
            (void)modellKartenGlaBauen(lauf(ordner + "\\" + n), false);
            diag::detail("Knopftest: MB3 Ziel " + std::to_string(ziel) + " - " + g_app->mtab.meldung);
        }));
        add(pause(0.2));
        add(pruefe("MB3 Ziel " + std::to_string(ziel) + ": neu geladen spielt BOTH_CIN_3 Bild fuer Bild die Pose von BOTH_WALK1", [=] {
            const std::string n = ziel == 0 ? "_humanoid_mb3test.gla" : ziel == 1 ? "_humanoid.gla" : "mb3skelett.gla";
            GlaAnimation k;
            if (!readGla(datei(lauf(ordner + "\\" + n)), k)) { return false; }
            const std::vector<AnimEntry> z = parseAnimationCfg(datei(lauf(ordner + "\\animation.cfg")));
            const AnimEntry* c = nullptr;
            for (const AnimEntry& e : z) {
                if (e.name == "BOTH_CIN_3") { c = &e; }
            }
            const AnimEntry& w = g_app->animList[static_cast<std::size_t>(mb1::animNach("BOTH_WALK1"))];
            if (c == nullptr || c->numFrames != w.numFrames || c->loopFrame != w.loopFrame || c->fps != w.fps) { return false; }
            std::vector<BoneMatrix> a;
            std::vector<BoneMatrix> b;
            bool gleich = true;
            for (int f = 0; f < w.numFrames && gleich; ++f) {
                g_app->anim.worldMatrices(w.firstFrame + f, a);
                k.worldMatrices(c->firstFrame + f, b);
                gleich = a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(BoneMatrix)) == 0;
            }
            // je Ziel: Kopf und Rest
            if (ziel == 0) { gleich = gleich && k.skeletonName == "models/players/_humanoid_mb3test/_humanoid_mb3test"; }
            if (ziel == 1) {
                // die ganze GLA bleibt: alle alten Zeilen, alte Frames unveraendert vorne
                gleich = gleich && k.skeletonName == g_app->anim.skeletonName && z.size() >= g_app->animList.size() &&
                         k.numFrames == g_app->anim.numFrames + w.numFrames &&
                         std::memcmp(k.indexes.data(), g_app->anim.indexes.data(), g_app->anim.indexes.size() * 4U) == 0;
            }
            if (ziel == 2) {
                gleich = gleich && k.skeletonName == "models/players/mb3skelett/mb3skelett" &&
                         g_app->mtab.dok.inhalt().glaPfad == "models/players/mb3skelett/mb3skelett";
            }
            return gleich;
        }));
    }
    add(pruefe("MB3 Ziele: _humanoid.gla nie ins Spielverzeichnis (Pfad im Spielordner erkannt, im Laufordner nicht)", [] {
        const bool spiel = !g_app->gamePaths.empty() &&
                           modellImSpielordner((std::filesystem::path(g_app->gamePaths.front().directory) / "models/players/_humanoid/_humanoid.gla").string());
        return spiel && !modellImSpielordner(lauf("mb3_ziel1\\_humanoid.gla"));
    }));
    add(pruefe("MB3 Ziele: Pruefungen je Ziel - eigene GLA im Mehrspieler gemeldet, _humanoid.gla im Mehrspieler mit Hinweis", [] {
        GlaAnimation k;
        k.skeletonName = "models/players/mb3skelett/mb3skelett";
        return !modell::eigeneGlaPruefen(k, k.skeletonName, modell::Zielspiel::MdMp).empty() &&
               modell::eigeneGlaPruefen(k, k.skeletonName, modell::Zielspiel::MdSp).empty() &&
               !modell::humanoidGlaPruefen(modell::Zielspiel::JkaMp).empty() && modell::humanoidGlaPruefen(modell::Zielspiel::JkaSp).empty();
    }));
    add(tu("MB3: zurueck", [] {
        modellGlaZielSetzen(0);
        while (g_app->mtab.dok.rueckgaengig()) {}
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    return s;
}

std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    add(tu("MB3 Handbuch: Bereich Animation, Karte mb3test, zwei Plaetze, BOTH_WALK1", [=] {
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = 0.66F;
        g_app->mtab.bereich = ModellBereich::Animation;
        std::snprintf(g_app->mtab.kgKarte, sizeof g_app->mtab.kgKarte, "%s", "mb3test");
        animWaehlen(mb1::animNach("BOTH_RUN1"), false);
        (void)modellPlatzSetzen(0, mb1::animNach("BOTH_RUN1"));
        animWaehlen(mb1::animNach("BOTH_WALK1"), true);
        (void)modellPlatzSetzen(2, mb1::animNach("BOTH_WALK1"));
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("MB3 Handbuch: Reiter Dienstprogramme (TEST1411 N: Ziel-GLA und Plaetze)", [] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten); }));
    add(pause(0.5));
    add(tu("MB3 Handbuch: das Befehlsfeld zu den Plaetzen rollen", [] {
        // die Ueberschrift Ziel-GLA und Plaetze oben ins Befehlsfeld
        const Element* e = finde(tr(Str::MbRKartenGla), "", 0, false);
        for (ImGuiWindow* w : GImGui->Windows) {
            if (e != nullptr && std::strstr(w->Name, "mtrollouts") != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y, 0.0F, w->ScrollMax.y));
            }
        }
    }));
    add(pause(0.5));
    add(hilfe2::schrittFoto("h_mb3_animation"));
    add(tu("MB3 Handbuch: die letzte Sequenz hinter das Ende der GLA (Befund)", [] {
        const std::vector<AnimEntry> e = g_app->mtab.dok.inhalt().cfg.eintraege();
        AnimEntry w = e.back();
        w.numFrames = g_app->anim.numFrames - w.firstFrame + 10;
        (void)modellMarkerAendern(w);
        animWaehlen(mb1::animNach(w.name), true);
    }));
    add(pause(0.6));
    // AP14b: seit AP1 steht die Zeile nicht mehr als Marker unter der Zeitleiste, sondern in der Zeitkonfiguration; der Befund steht in
    // der Gruppe animation.cfg - die Gruppe oben ins Befehlsfeld, die Zeitkonfiguration auf
    add(tu("MB3 Handbuch: Gruppe animation.cfg oben ins Befehlsfeld, Zeitkonfiguration auf", [] {
        const Element* e = finde(tr(Str::MbRCfg), "", 0, false);
        for (ImGuiWindow* w : GImGui->Windows) {
            if (e != nullptr && std::strstr(w->Name, "mtrollouts") != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y, 0.0F, w->ScrollMax.y));
            }
        }
        modellZeit().konfigOffen = true;
    }));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mb3_befund"));
    add(tu("MB3 Handbuch: Zeitkonfiguration zu", [] { modellZeit().konfigOffen = false; }));
    add(tu("MB3 Handbuch: zurueck, hips mit 1001 Ecken fuer JKA SP, Strg+S", [] {
        while (g_app->mtab.dok.rueckgaengig()) {}
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        g_app->mtab.dok.aendern("Test", [](modell::Inhalt& in) {
            const int t = in.teilIndex("hips");
            modell::Netz& n = in.teile[static_cast<std::size_t>(t)].stufen[0];
            std::vector<modell::Ecke> e = n.ecken;
            std::vector<std::uint32_t> idx = n.indizes();
            while (e.size() < 1001U) { e.push_back(e.front()); }
            n.aufbauen(std::move(e), idx);
        });
        platform::dialogAntwortImSelbsttest(lauf("mb3_hb.glm"));
        modellSpeichern(true);
    }));
    add(warteBis("MB3 Handbuch: die Frage ist offen", [] { return g_app->mtab.frageSichtbar; }, 300));
    add(pause(0.3));
    add(hilfe2::schrittFoto("h_mb3_speichern"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MB3 Handbuch: zurueck", [=] {
        while (g_app->mtab.dok.rueckgaengig()) {}
        g_app->mtab.zielspiel = static_cast<int>(modell::Zielspiel::MdSp);
        g_app->settings.splitModel = *teiler;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    std::vector<Schritt> s;
    if (n == "rundlauf") {
        const char* sp = std::getenv("BHED_MB_SPIEL");
        const std::string spiel = sp != nullptr ? sp : "md";
        s.push_back(tu("MB3 Rundlauf: Spiel " + spiel, [=] {
            if (spiel != "md") { mb1::spielSetzen(spiel); }
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        }));
        s.push_back(pause(0.3));
        s.push_back(rundlauf(spiel));
        return s;
    }
    if (n == "mb3") { return uiSchritte(); }
    return s;
}

}  // namespace mb3
