// selbsttest_mb3.inc - Modell-Block Stufe 3: Knochen-Erkennung, Control Rig, Posen, Ghosts, Silhouette, Pfade, Backen
// (BHED_EDITORTEST=modellblock)
//
//   BHED_MODELL_NUR=rig          Model-Tab mit dem Sturmtruppler: Rig ueber die Aktionssuche, Griff im Explorer und in der
//                                Ansicht anklicken, mit dem Ring (E) drehen (Schluessel, Netz nicht neu gebaut, Strg+Z),
//                                Pose kopieren / gespiegelt einfuegen gegen die Erwartung je Knochenpaar, Bindepose,
//                                Handpose, Auswahl Eltern/Kind/Gegenseite mit den Tasten wie 3ds Max und wie Blender, Ghosts
//                                und Silhouette im Bild, Bewegungspfad, Backen auf einen Platz (gegen die Welt der Schicht),
//                                Uebersicht und Zuordnung von Hand; jede Aktion in Menue, Suche, Rechtsklick, mit Tooltip
//   BHED_MODELL_NUR=rigrundlauf  jede .gla des Spiels (BHED_MB_SPIEL=jka|jk2|md): Erkennung (mit der .glm einer Figur, wenn es
//                                eine gibt; Hand-Tabellen tests/rig_handtabelle.inc), Rig je Figur, Backen ohne Aenderung
//                                Byte fuer Byte und ganz neu durch compressBone innerhalb der Stufen; Bericht
//                                mb4_rig_<spiel>.txt im Laufordner
// Eingebunden in selbsttest.cpp (nach selbsttest_mb2.inc).
namespace rig3 {

#include "../tests/rig_handtabelle.inc"

std::uint16_t u16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }

char seiteZeichen(modell::Seite s) { return s == modell::Seite::Links ? 'L' : (s == modell::Seite::Rechts ? 'R' : 'M'); }

// 14 Bytes wie MC_UnCompressQuat (OpenJK rd-common/mdx_format.h, GLA-Leser gla.cpp localMatrix): ohne Normieren.
BoneMatrix entpacke(const std::uint8_t* c);

float ortAbstand(const BoneMatrix& a, const BoneMatrix& b) {
    return std::sqrt((a.m[0][3] - b.m[0][3]) * (a.m[0][3] - b.m[0][3]) + (a.m[1][3] - b.m[1][3]) * (a.m[1][3] - b.m[1][3]) +
                     (a.m[2][3] - b.m[2][3]) * (a.m[2][3] - b.m[2][3]));
}

float drehDiff(const BoneMatrix& a, const BoneMatrix& b) {
    float d = 0.0F;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { d = std::max(d, std::fabs(a.m[i][j] - b.m[i][j])); }
    }
    return d;
}

BoneMatrix entpacke(const std::uint8_t* c) {
    const float w = static_cast<float>(u16(c)) / 16383.0F - 2.0F;
    const float x = static_cast<float>(u16(c + 2)) / 16383.0F - 2.0F;
    const float y = static_cast<float>(u16(c + 4)) / 16383.0F - 2.0F;
    const float z = static_cast<float>(u16(c + 6)) / 16383.0F - 2.0F;
    BoneMatrix m;
    m.m[0][0] = 1.0F - 2.0F * (y * y + z * z);
    m.m[0][1] = 2.0F * (x * y - w * z);
    m.m[0][2] = 2.0F * (x * z + w * y);
    m.m[1][0] = 2.0F * (x * y + w * z);
    m.m[1][1] = 1.0F - 2.0F * (x * x + z * z);
    m.m[1][2] = 2.0F * (y * z - w * x);
    m.m[2][0] = 2.0F * (x * z - w * y);
    m.m[2][1] = 2.0F * (y * z + w * x);
    m.m[2][2] = 1.0F - 2.0F * (x * x + y * y);
    for (int k = 0; k < 3; ++k) { m.m[k][3] = static_cast<float>(u16(c + 8 + 2 * k)) / 64.0F - 512.0F; }
    return m;
}

BoneMatrix nurDrehung(BoneMatrix m) {
    m.m[0][3] = m.m[1][3] = m.m[2][3] = 0.0F;
    return m;
}

int griffNach(modell::GriffArt art, modell::Seite seite) {
    const auto& g = g_app->mtab.rig.griffe;
    for (std::size_t i = 0; i < g.size(); ++i) {
        if (g[i].art == art && g[i].seite == seite) { return static_cast<int>(i); }
    }
    return -1;
}

const modell::RigSchicht* schicht() { return modellRigSchichtJetzt(); }

// Ein Element in einem Popup/Menue (Fenster "##Popup_..." / "##Menu_...") - die Befehlsfeld-Knoepfe tragen dieselben Texte.
const Element* imMenue(const std::string& label) {
    for (const Element& e : g_elemente) {
        // ImGui: Popups "##Popup_<id>", Menues "<Titel>###Menu_<n>"
        if (e.label == label && (e.fenster.find("##Popup_") != std::string::npos || e.fenster.find("###Menu_") != std::string::npos ||
                                 e.fenster.rfind("##Menu", 0) == 0)) {
            return &e;
        }
    }
    return nullptr;
}

// Ein Eintrag der Menueleiste: im obersten Fenster selbst (nicht in einem Kindfenster wie die Reiter der Bereiche).
const Element* menueLeiste(const std::string& label) {
    for (const Element& e : g_elemente) {
        if (e.label == label && e.innen == e.fenster && e.fenster.rfind("##", 0) != 0) { return &e; }
    }
    return nullptr;
}

// Wo steht ein Text? (fuer die Meldung, wenn ein Menue nicht aufging)
std::string woIst(const std::string& label) {
    std::string w;
    for (const Element& e : g_elemente) {
        if (e.label == label) { w += "[" + e.fenster + " / " + e.innen + "] "; }
    }
    return w.empty() ? std::string("nirgends") : w;
}

// Ein Kindfenster so rollen, dass das Element oben steht.
Schritt rollenZu(const std::string& label, const char* fensterTeil) {
    return tu("MB4: " + label + " ins Bild rollen", [=] {
        const Element* e = finde(label, "", 0, false);
        for (ImGuiWindow* w : GImGui->Windows) {
            if (e != nullptr && std::strstr(w->Name, fensterTeil) != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 2.0F, 0.0F, w->ScrollMax.y));
            }
        }
    });
}

// Eine Aktion ueber "Alle Aktionen suchen" (X) ausloesen.
void ueberSuche(std::vector<Schritt>& s, Str text) {
    for (int k = 0; k < 2; ++k) {   // ein offenes Menue nimmt den ersten Klick
        s.push_back(klickAn("in die Ansicht (Fokus)", [] {
            const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
            return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
        }));
        s.push_back(pause(0.1));
    }
    s.push_back(taste(ImGuiKey_X, "X (Alle Aktionen suchen)"));
    s.push_back(pause(0.2));
    s.push_back(tippe(tr(text)));
    s.push_back(pause(0.2));
    s.push_back(klickAn(std::string("\"") + tr(text) + "\" in der Suche", [text] {
        const Element* e = imMenue(tr(text));
        if (e == nullptr) {
            meldeFehler(std::string("MB4: ") + tr(text) + " nicht in der Suche: " + woIst(tr(text)));
            return ImVec2{};
        }
        return e->rect.GetCenter();
    }));
    s.push_back(pause(0.3));
}

Schritt klickMenue(const std::string& label, bool leiste) {
    return klickAn("\"" + label + "\"" + (leiste ? " (Menueleiste)" : " (Menue)"), [=] {
        const Element* e = leiste ? menueLeiste(label) : imMenue(label);
        if (e == nullptr) {
            meldeFehler("MB4: " + label + " nicht gefunden");
            return ImVec2{};
        }
        return e->rect.GetCenter();
    });
}

// --- Rundlauf ueber alle Skelette eines Spiels ------------------------------------------------------------------------
Schritt rundlauf(const std::string& spiel) {
    struct Zustand {
        std::vector<std::string> glas;
        std::map<std::string, std::vector<std::string>> figuren;   // GLA (klein, ohne Endung) -> .glm
        std::size_t i = 0;
        int skelette = 0, figurenZahl = 0, figurenRig = 0, figurenOhne = 0;
        long long knochenBilder = 0, gleich = 0;
        int maxStufe = 0, maxVerschiebung = 0;
        double maxNormStufe = 0.0;   // ganz neu gegen das Original: Matrixabstand / Grenze (<= 1)
        double maxBetrag = 0.0;
        std::string normSchlimmste;      // | |q| - 1 | der Originale, in Stufen (Carcass rundet nicht auf Einheitslaenge)
        double maxOrt = 0.0;
        std::string schlechtester;
        std::map<std::string, std::pair<int, int>> rollen;   // Rolle -> (richtig, gesamt) ueber die Hand-Tabellen
        std::string bericht;
        bool fehler = false;
        std::chrono::steady_clock::time_point anfang{};
    };
    auto z = std::make_shared<Zustand>();
    Schritt s{"MB4 Rig-Rundlauf (" + spiel + "): Erkennung, Rig je Figur, Backen je GLA", [=](int b) {
                  if (b == 0) {
                      std::vector<std::string> glms;
                      for (const GamePath& gp : g_app->gamePaths) {
                          for (const FoundFile& f : findByExtension(gp, {".gla"})) { z->glas.push_back(f.name); }
                          for (const FoundFile& f : findByExtension(gp, {".glm"})) { glms.push_back(f.name); }
                      }
                      std::sort(z->glas.begin(), z->glas.end());
                      z->glas.erase(std::unique(z->glas.begin(), z->glas.end()), z->glas.end());
                      testtempo::anteil(z->glas);   // TESTTEMPO: BHED_TEST_ANTEIL=i/n
                      std::sort(glms.begin(), glms.end());
                      glms.erase(std::unique(glms.begin(), glms.end()), glms.end());
                      // je .glm die GLA aus dem Kopf (animFile)
                      for (const std::string& m : glms) {
                          std::string bytes;
                          GlmModel g;
                          if (!readFromArchives(m, bytes) || !readGlm(bytes, g) || g.animFile.empty()) { continue; }
                          std::string k = g.animFile;
                          for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                          z->figuren[k].push_back(m);
                      }
                      z->anfang = std::chrono::steady_clock::now();
                      diag::detail("Knopftest: MB4 Rig-Rundlauf " + spiel + " - " + std::to_string(z->glas.size()) + " GLA, " + std::to_string(glms.size()) + " GLM");
                      return false;
                  }
                  if (z->i < z->glas.size()) {
                      const std::string pfad = z->glas[z->i++];
                      std::string bytes;
                      GlaAnimation g;
                      if (!readFromArchives(pfad, bytes) || !readGla(bytes, g) || g.bones.empty()) {
                          z->bericht += "nicht lesbar: " + pfad + "\n";
                          return false;
                      }
                      ++z->skelette;
                      std::string schluessel = pfad.substr(0, pfad.size() - 4);
                      for (char& c : schluessel) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                      const auto it = z->figuren.find(schluessel);
                      const std::vector<std::string> figuren = it != z->figuren.end() ? it->second : std::vector<std::string>{};
                      // Erkennung mit der ersten Figur (Ecken fuer Knochen in der Mitte)
                      modell::Inhalt in;
                      if (!figuren.empty()) {
                          std::string mb;
                          if (readFromArchives(figuren.front(), mb)) { (void)modell::ausGlm(mb, in); }
                      }
                      const modell::Erkennung e = modell::erkenneKnochen(g, {}, in.leer() ? nullptr : &in, 0);
                      char zeile[300];
                      std::snprintf(zeile, sizeof zeile, "%s: %d Knochen, %d Bilder, erkannt %d, unklar %d, Figuren %d", pfad.c_str(), static_cast<int>(g.bones.size()),
                                    g.numFrames, e.erkannt, e.unklar, static_cast<int>(figuren.size()));
                      z->bericht += zeile;
                      if (e.unklar > 0) {
                          z->bericht += " - unklar:";
                          for (std::size_t k = 0; k < e.knochen.size(); ++k) {
                              if (e.knochen[k].unklar) {
                                  z->bericht += " " + g.bones[k].name + "=" + modell::rollenSchluessel(e.knochen[k].rolle) + "/" + seiteZeichen(e.knochen[k].seite) + "(" +
                                                e.knochen[k].grund + ")";
                              }
                          }
                      }
                      // Hand-Tabelle
                      for (const RigHandSkelett& h : kRigHandSkelette) {
                          std::string hg = h.gla;
                          const bool passt = pfad == hg && ((spiel == "jk2") == (std::string(h.name) == "jk2")) &&
                                             (h.zahl == static_cast<int>(g.bones.size()));
                          if (!passt) { continue; }
                          int richtig = 0;
                          std::string falsch;
                          for (int k = 0; k < h.zahl; ++k) {
                              const int bi = boneIndex(g, h.knochen[k].name);
                              if (bi < 0) { continue; }
                              const modell::KnochenErkennung& ke = e.knochen[static_cast<std::size_t>(bi)];
                              const bool ok = std::strcmp(modell::rollenSchluessel(ke.rolle), h.knochen[k].rolle) == 0 &&
                                              (h.knochen[k].seite == '?' || h.knochen[k].seite == seiteZeichen(ke.seite));
                              auto& r = z->rollen[std::string(h.name) + " " + h.knochen[k].rolle];
                              r.second += 1;
                              r.first += ok ? 1 : 0;
                              richtig += ok ? 1 : 0;
                              if (!ok) { falsch += std::string(h.knochen[k].name) + "=" + modell::rollenSchluessel(ke.rolle) + "/" + seiteZeichen(ke.seite) + " "; }
                          }
                          z->bericht += "  Hand-Tabelle " + std::string(h.name) + ": " + std::to_string(richtig) + "/" + std::to_string(h.zahl) + " " + falsch;
                          if (richtig != h.zahl) { z->fehler = true; }
                      }
                      z->bericht += "\n";
                      // Rig je Figur
                      for (const std::string& f : figuren) {
                          std::string mb;
                          modell::Inhalt fi;
                          if (!readFromArchives(f, mb) || !modell::ausGlm(mb, fi)) { continue; }
                          ++z->figurenZahl;
                          const modell::Rig r = modell::rigAnlegen(g, fi, 0, {});
                          // ohne einen Knochen mit FK-Rolle (Fahrzeuge, Geschuetze: model_root und Bauteile) gibt es keinen Griff
                          bool fk = false;
                          for (const modell::KnochenErkennung& k : r.erkennung.knochen) {
                              fk = fk || k.rolle == modell::Rolle::Becken || k.rolle == modell::Rolle::Wirbel || k.rolle == modell::Rolle::Hals ||
                                   k.rolle == modell::Rolle::Kopf || modell::armRolle(k.rolle) || modell::beinRolle(k.rolle);
                          }
                          if (!fk) {
                              ++z->figurenOhne;
                              z->bericht += "  ohne Griff-Rolle: " + f + "\n";
                          } else if (r.leer()) {
                              z->bericht += "  Rig leer trotz Griff-Rolle: " + f + "\n";
                          }
                          z->figurenRig += r.leer() ? 0 : 1;
                      }
                      // Backen: ohne Aenderung Byte fuer Byte, ganz neu innerhalb der Stufen
                      const modell::RigSchicht leer;
                      GlaAnimation aus;
                      GlaAnimation neu;
                      modell::BackBericht br;
                      if (!modell::rigBacken(g, leer, 0, g.numFrames, false, g.name, aus, &br) || !modell::rigBacken(g, leer, 0, g.numFrames, true, g.name, neu, &br)) {
                          z->bericht += "  Backen: " + br.fehler + "\n";
                          z->fehler = true;
                          return false;
                      }
                      const std::size_t n = g.indexes.size();
                      for (std::size_t at = 0; at < n; ++at) {
                          const std::uint8_t* a = g.bonePool.data() + static_cast<std::size_t>(g.indexes[at]) * 14U;
                          const std::uint8_t* c = aus.bonePool.data() + static_cast<std::size_t>(aus.indexes[at]) * 14U;
                          const std::uint8_t* d = neu.bonePool.data() + static_cast<std::size_t>(neu.indexes[at]) * 14U;
                          z->gleich += std::memcmp(a, c, 14U) == 0 ? 1 : 0;
                          int plus = 0;
                          int minus = 0;
                          for (int k = 0; k < 4; ++k) {
                              plus = std::max(plus, std::abs(static_cast<int>(u16(a + 2 * k)) - static_cast<int>(u16(d + 2 * k))));
                              minus = std::max(minus, std::abs(static_cast<int>(u16(a + 2 * k)) - (4 * 16383 - static_cast<int>(u16(d + 2 * k)))));
                          }
                          z->maxStufe = std::max(z->maxStufe, std::min(plus, minus));
                          // Was das Spiel zeigt: MC_UnCompressQuat normiert nicht (tr_ghoul2.cpp), aus |q|^2 = s != 1 wird
                          // M = (1 - s) I + s R mit R = R(q/|q|). compressBone zielt auf die naechste Drehung P zu M (Frobenius:
                          // |M - P| <= |M - R| = |1 - s| |I - R|) und quantisiert sie: je Anteil hoechstens 1,5 Stufen (Runden + Suche
                          // optimizeQuat), |dq| <= 3/16383, die Drehmatrix aendert sich um hoechstens 4 |dq|, dazu die Laenge des
                          // quantisierten q (2 |dq| * |I - R| <= 2 * 3/16383 * 2 sqrt 2). Grenze in Frobenius: die Summe.
                          double q[4];
                          double l2 = 0.0;
                          for (int k = 0; k < 4; ++k) {
                              q[k] = static_cast<double>(u16(a + 2 * k)) / 16383.0 - 2.0;
                              l2 += q[k] * q[k];
                          }
                          z->maxBetrag = std::max(z->maxBetrag, std::fabs(std::sqrt(l2) - 1.0) * 16383.0);
                          const BoneMatrix mo = entpacke(a);
                          const BoneMatrix mn = entpacke(d);
                          double ir = 0.0;   // |I - R|_F mit R aus dem normierten q
                          {
                              const double l = std::sqrt(std::max(l2, 1e-12));
                              const double w = q[0] / l, x = q[1] / l, y = q[2] / l, zz = q[3] / l;
                              const double r[3][3] = {{1 - 2 * (y * y + zz * zz), 2 * (x * y - w * zz), 2 * (x * zz + w * y)},
                                                      {2 * (x * y + w * zz), 1 - 2 * (x * x + zz * zz), 2 * (y * zz - w * x)},
                                                      {2 * (x * zz - w * y), 2 * (y * zz + w * x), 1 - 2 * (x * x + y * y)}};
                              for (int i = 0; i < 3; ++i) {
                                  for (int j = 0; j < 3; ++j) { ir += ((i == j ? 1.0 : 0.0) - r[i][j]) * ((i == j ? 1.0 : 0.0) - r[i][j]); }
                              }
                              ir = std::sqrt(ir);
                          }
                          const double grenze = std::fabs(1.0 - l2) * ir + (4.0 * 3.0 + 2.0 * 3.0 * 2.0 * std::sqrt(2.0)) / 16383.0;
                          double ab = 0.0;
                          for (int i = 0; i < 3; ++i) {
                              for (int j = 0; j < 3; ++j) {
                                  const double dd = static_cast<double>(mo.m[i][j]) - static_cast<double>(mn.m[i][j]);
                                  ab += dd * dd;
                              }
                          }
                          ab = std::sqrt(ab);
                          if (ab / grenze > z->maxNormStufe) {
                              z->maxNormStufe = ab / grenze;
                              char w[260];
                              std::snprintf(w, sizeof w, "%s Knochen %d Bild %d: Matrixabstand %.6f, Grenze %.6f (|q|^2 %.5f)", pfad.c_str(),
                                            static_cast<int>(at % g.bones.size()), static_cast<int>(at / g.bones.size()), ab, grenze, l2);
                              z->normSchlimmste = w;
                          }
                          for (int k = 4; k < 7; ++k) { z->maxVerschiebung = std::max(z->maxVerschiebung, std::abs(static_cast<int>(u16(a + 2 * k)) - static_cast<int>(u16(d + 2 * k)))); }
                      }
                      z->knochenBilder += static_cast<long long>(n);
                      // die Welt an jedem 7. Bild (alle Knochen): Ort neu gegen Original
                      std::vector<BoneMatrix> w0;
                      std::vector<BoneMatrix> w1;
                      for (int f = 0; f < g.numFrames; f += 7) {
                          g.worldMatrices(f, w0);
                          neu.worldMatrices(f, w1);
                          for (std::size_t k = 0; k < w0.size(); ++k) {
                              const double d = static_cast<double>(ortAbstand(w0[k], w1[k]) * (g.scale > 0.0F ? g.scale : 1.0F));
                              if (d > z->maxOrt) {
                                  z->maxOrt = d;
                                  z->schlechtester = pfad + " " + g.bones[k].name + " Bild " + std::to_string(f);
                              }
                          }
                      }
                      return false;
                  }
                  const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - z->anfang).count();
                  char zeile[600];
                  std::snprintf(zeile, sizeof zeile,
                                "MB4 Rig-Rundlauf %s: %d Skelette, %d Figuren (Rig angelegt %d, ohne Rolle fuer einen Griff %d); Backen ohne Aenderung %lld "
                                "von %lld Knochen-Bildern Byte fuer Byte; ganz neu durch compressBone: Matrixabstand zum Original hoechstens %.3f der Grenze "
                                "(gegen die Bytes bis %d Stufen, die Originale weichen bis %.1f Stufen von der Einheitslaenge ab), Verschiebung %d "
                                "Stufen, Ort hoechstens %.5f Einheiten (mit fScale; %s); %.0f s",
                                spiel.c_str(), z->skelette, z->figurenZahl, z->figurenRig, z->figurenOhne, z->gleich, z->knochenBilder, z->maxNormStufe,
                                z->maxStufe, z->maxBetrag, z->maxVerschiebung, z->maxOrt, z->schlechtester.c_str(), sek);
                  diag::detail(std::string("Knopftest: ") + zeile);
                  std::string rollen;
                  for (const auto& [k, v] : z->rollen) { rollen += k + ": " + std::to_string(v.first) + "/" + std::to_string(v.second) + "\n"; }
                  {
                      std::ofstream f("mb4_rig_" + spiel + ".txt", std::ios::binary);
                      f << zeile << "\nGroesster Anteil an der Grenze: " << z->normSchlimmste << "\n\nTrefferquote je Rolle (Hand-Tabellen):\n" << rollen << "\nJe GLA:\n" << z->bericht;
                  }
                  pruefeWahr(z->skelette > 0 && !z->fehler, "MB4 Rig-Rundlauf " + spiel + ": die Hand-Tabellen voll getroffen, jede GLA gebacken");
                  pruefeWahr(z->knochenBilder > 0 && z->gleich == z->knochenBilder, "MB4 Rig-Rundlauf " + spiel + ": ohne Aenderung jedes Knochen-Bild Byte fuer Byte");
                  // Die Grenze je Eintrag steht oben beim Messen (Scherung der Raven-Daten + Quantisierung von compressBone). Gegen die
                  // Bytes allein galt sie nicht (mb4_rl_jka: 47 Stufen), weil die Raven-Quaternionen bis 35 Stufen neben der
                  // Einheitslaenge liegen.
                  pruefeWahr(z->maxNormStufe <= 1.0 && z->maxVerschiebung == 0,
                             "MB4 Rig-Rundlauf " + spiel + ": ganz neu komprimiert innerhalb der Grenze aus compressBone, Verschiebung gleich");
                  pruefeWahr(z->figurenZahl == 0 || z->figurenRig + z->figurenOhne == z->figurenZahl,
                             "MB4 Rig-Rundlauf " + spiel + ": fuer jede Figur mit einer Griff-Rolle ein Rig");
                  return true;
              }};
    s.eigeneZeitgrenze = true;
    return s;
}

// --- Model-Tab ------------------------------------------------------------------------------------------------------------
std::vector<Schritt> uiSchritte(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string pfad = "models/players/stormtrooper/model.glm";
    add(tu("MB4: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb4.txt"); }));
    for (Schritt& x : mb1::laden(pfad)) { add(std::move(x)); }
    add(tu("MB4: Bereich Animation, BOTH_WALK1 angehalten, wie 3ds Max", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animFrame = g_app->animList[static_cast<std::size_t>(g_app->animIndex)].firstFrame + 10;
        g_app->animTime = 0.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(klickAn("in die Ansicht (Fokus)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    // --- Rig mit einem Klick, ueber die Suche (X) ------------------------------------------------------------------------
    add(taste(ImGuiKey_X, "X (Alle Aktionen suchen)"));
    add(pause(0.2));
    add(tippe(tr(Str::MbRgAnlegen)));
    add(pause(0.2));
    add(klickMenue(tr(Str::MbRgAnlegen), false));
    add(pause(0.3));
    add(pruefe("MB4 Rig: ueber die Suche angelegt - 53 Knochen erkannt, keiner unklar, Griffe und IK-Ziele", [] {
        const ModellTab& t = g_app->mtab;
        if (t.letzteTaste != tr(Str::MbRgAnlegen)) { return false; }
        int ik = 0;
        for (const modell::Griff& g : t.rig.griffe) { ik += g.ik ? 1 : 0; }
        diag::detail("Knopftest: MB4 Rig - " + t.rigMeldung);
        return modellRigDa() && t.rig.erkennung.erkannt == 53 && t.rig.erkennung.unklar == 0 && ik >= 8 &&
               griffNach(modell::GriffArt::Hand, modell::Seite::Rechts) >= 0 && griffNach(modell::GriffArt::Saebelgriff, modell::Seite::Rechts) >= 0;
    }));
    add(pause(0.3));
    add(pruefe("MB4 Rig: Griffe in der Ansicht und im Explorer (Symbol und Auge je Griff)", [] {
        const ModellTab& t = g_app->mtab;
        return t.griffeGezeichnet > 0 && finde(std::string(tr(Str::MbRgHand)) + " " + tr(Str::MbRgR), "", 0, false) != nullptr;
    }));
    // --- Griff im Explorer und in der Ansicht waehlen ----------------------------------------------------------------
    add(rollenZu(std::string(tr(Str::MbRgHand)) + " " + tr(Str::MbRgR), "mtbaum"));
    add(pause(0.3));
    add(klick(std::string(tr(Str::MbRgHand)) + " " + tr(Str::MbRgR)));
    add(pause(0.2));
    add(pruefe("MB4 Explorer: Klick waehlt den Griff Hand R (Knochen rhand)", [] {
        const int g = griffNach(modell::GriffArt::Hand, modell::Seite::Rechts);
        return g_app->mtab.griffWahl == g && g_app->mtab.knochenWahl == boneIndex(g_app->anim, "rhand");
    }));
    add(klickAn("auf den Griff Kopf in der Ansicht", [] {
        ImVec2 p;
        (void)modellRigGriffAufSchirm(griffNach(modell::GriffArt::Kopf, modell::Seite::Mitte), p);
        return p;
    }));
    add(pause(0.2));
    add(pruefe("MB4 Ansicht: Klick auf den Griff waehlt ihn (Kopf, cranium)", [] {
        return g_app->mtab.griffWahl == griffNach(modell::GriffArt::Kopf, modell::Seite::Mitte) && g_app->mtab.knochenWahl == boneIndex(g_app->anim, "cranium");
    }));
    // --- Drehen mit dem Ring (E): rechte Hand um Z ------------------------------------------------------------------
    auto vorher = std::make_shared<std::vector<BoneMatrix>>();
    auto netz = std::make_shared<int>(0);
    add(tu("MB4 Drehen: Griff Hand R, E", [=] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        *vorher = g_app->modellWelt;
        *netz = g_app->mtab.neuGebaut;
    }));
    add(taste(ImGuiKey_E, "E"));
    add(pause(0.3));
    add(mb3::ziehenVonNach("am Ring Z 60 Punkte nach rechts", [] {
        ImVec2 p;
        (void)modellGizmoRingOrt(2, 45.0F, p);
        return p;
    }, [] {
        ImVec2 p;
        (void)modellGizmoRingOrt(2, 45.0F, p);
        return ImVec2{p.x + 60.0F, p.y};
    }));
    add(pause(0.3));
    add(pruefe("MB4 Drehen: rhand um Welt-Z gedreht (30 Grad), Finger mit, rradiusX mit Anteil, Schluessel, Netz nicht neu gebaut", [=] {
        const int h = boneIndex(g_app->anim, "rhand");
        const int f = boneIndex(g_app->anim, "r_d2_j1");
        const auto& w = g_app->modellWelt;
        const modell::RigSchicht* sch = schicht();
        if (sch == nullptr || w.size() != vorher->size()) { return false; }
        // Drehung um Z durch den Ursprung der Hand: R_neu = Rz(a) * R_alt; a aus dem Ergebnis, dann gegen 30 Grad
        const BoneMatrix d = nurDrehung(w[static_cast<std::size_t>(h)]) * modell::starrInvers(nurDrehung((*vorher)[static_cast<std::size_t>(h)]));
        const float grad = std::atan2(d.m[1][0], d.m[0][0]) * 57.29578F;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        const bool umZ = drehDiff(d, modell::drehungUm(z, grad)) < 2e-3F;
        const bool fingerMit = drehDiff(nurDrehung(w[static_cast<std::size_t>(f)]) * modell::starrInvers(nurDrehung((*vorher)[static_cast<std::size_t>(f)])), d) < 2e-3F;
        char zeile[200];
        std::snprintf(zeile, sizeof zeile, "Knopftest: MB4 Drehen - %.2f Grad um Z (%d), Finger %d, Schluessel %d, neu gebaut %d -> %d", static_cast<double>(grad),
                      umZ ? 1 : 0, fingerMit ? 1 : 0, sch->schluesselZahl(), *netz, g_app->mtab.neuGebaut);
        diag::detail(zeile);
        return umZ && std::fabs(grad - 30.0F) < 1.0F && fingerMit && sch->schluesselZahl() >= 8 && g_app->mtab.neuGebaut == *netz &&
               ortAbstand(w[static_cast<std::size_t>(h)], (*vorher)[static_cast<std::size_t>(h)]) < 1e-3F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB4 Drehen: Strg+Z nimmt den Schluessel zurueck (Pose wie vorher)", [=] {
        const int h = boneIndex(g_app->anim, "rhand");
        const modell::RigSchicht* sch = schicht();
        return (sch == nullptr || sch->schluesselZahl() == 0) && drehDiff(g_app->modellWelt[static_cast<std::size_t>(h)], (*vorher)[static_cast<std::size_t>(h)]) < 1e-5F;
    }));
    // --- Pose: rechter Oberarm um Y, kopieren, gespiegelt einfuegen (Blender Strg+Umschalt+V) -------------------------------
    auto pose = std::make_shared<std::vector<BoneMatrix>>();
    add(tu("MB4 Pose: Oberarm R um Welt-Y 30 Grad (Gizmo-Weg), Pose kopieren (Menue)", [=] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Oberarm, modell::Seite::Rechts));
        g_app->mtab.gizmo = 2;
        const float w[3] = {0.0F, 30.0F, 0.0F};
        modellRigZiehen(2, w);
        modellRigZiehenEnde();
    }));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.2));
    add(klickMenue(tr(Str::MbRgKopieren), false));
    add(pause(0.2));
    add(tu("MB4 Pose: die kopierte Welt merken, 5 Bilder weiter, Bedienung wie Blender", [=] {
        *pose = g_app->modellWelt;
        g_app->animFrame += 5;
        g_app->modelDirty = true;
        modellBedienungSetzen(1);
    }));
    add(pause(0.3));
    add(klickAn("in die Ansicht (Fokus)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V, "Strg+Umschalt+V (gespiegelt einfuegen)"));
    add(pause(0.3));
    add(pruefe("MB4 Gespiegelt: je Knochenpaar D(links) = S D(rechts) S (Seitenachse X), Mitte gespiegelt auf sich", [=] {
        const GlaAnimation& a = g_app->anim;
        const auto& w = g_app->modellWelt;
        const modell::Rig& r = g_app->mtab.rig;
        BoneMatrix S = BoneMatrix::identity();
        S.m[0][0] = -1.0F;
        float maxD = 0.0F;
        int paare = 0;
        for (std::size_t b = 0; b < a.bones.size(); ++b) {
            const int p = r.partner[b];
            if (p < 0) { continue; }
            const BoneMatrix dQuelle = nurDrehung((*pose)[static_cast<std::size_t>(p)] * a.bones[static_cast<std::size_t>(p)].basePoseInv);
            const BoneMatrix dZiel = nurDrehung(w[b] * a.bones[b].basePoseInv);
            maxD = std::max(maxD, drehDiff(dZiel, S * dQuelle * S));
            paare += p != static_cast<int>(b) ? 1 : 0;
        }
        // die Hand-Erwartung am Oberarm: Rechts um Y +30 -> links um Y -30 (die Animation darunter gespiegelt dazu)
        const int lh = boneIndex(a, "lhumerus");
        const int rh = boneIndex(a, "rhumerus");
        const BoneMatrix dl = nurDrehung(w[static_cast<std::size_t>(lh)] * a.bones[static_cast<std::size_t>(lh)].basePoseInv);
        const BoneMatrix dr = nurDrehung((*pose)[static_cast<std::size_t>(rh)] * a.bones[static_cast<std::size_t>(rh)].basePoseInv);
        char zeile[200];
        std::snprintf(zeile, sizeof zeile, "Knopftest: MB4 Gespiegelt - %d Paare, groesste Abweichung %.6f", paare, static_cast<double>(maxD));
        diag::detail(zeile);
        return paare >= 40 && maxD < 1e-3F && drehDiff(dl, S * dr * S) < 1e-3F;
    }));
    // --- R wie Blender auf dem Griff Kopf: Achse X, Maus 40 Punkte nach rechts, Enter ------------------------------------
    auto kopfVorher = std::make_shared<BoneMatrix>();
    add(tu("MB4 Blender R: Griff Kopf, Maus in die Mitte der Ansicht", [=] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Kopf, modell::Seite::Mitte));
        *kopfVorher = g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))];
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F});
    }));
    add(pause(0.1));
    add(taste(ImGuiKey_R, "R"));
    add(taste(ImGuiKey_X, "X"));
    add(tu("MB4 Blender R: Maus 40 Punkte nach rechts", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.5F + 40.0F, a.lo[1] + a.groesse[1] * 0.5F});
    }));
    add(pause(0.2));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("MB4 Blender R: cranium um Welt-X gedreht (40 Punkte = 20 Grad), Modal beendet, ein Rueckgaengig-Schritt", [=] {
        const BoneMatrix& n = g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))];
        const BoneMatrix d = nurDrehung(n) * modell::starrInvers(nurDrehung(*kopfVorher));
        const float x[3] = {1.0F, 0.0F, 0.0F};
        const float grad = std::atan2(d.m[2][1], d.m[1][1]) * 57.29578F;
        diag::detail("Knopftest: MB4 Blender R - " + std::to_string(grad) + " Grad um X");
        return g_app->mtab.rigModal == 0 && std::fabs(grad - 20.0F) < 0.5F && drehDiff(d, modell::drehungUm(x, grad)) < 2e-3F &&
               g_app->mtab.dok.rueckgaengigName() == tr(Str::MbRgUndoDrehen);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    // --- Bindepose, Handpose ----------------------------------------------------------------------------------------
    add(tu("MB4 Bindepose: ganze Figur (Aktion)", [] {
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (std::string(modellRigAktionText(i)) == tr(Str::MbRgBindeAlle)) { modellRigAktionAusloesen(i); }
        }
    }));
    add(pause(0.3));
    add(pruefe("MB4 Bindepose: jede Weltmatrix = Grundstellung", [] {
        float d = 0.0F;
        for (std::size_t b = 0; b < g_app->modellWelt.size(); ++b) {
            d = std::max(d, drehDiff(g_app->modellWelt[b], g_app->anim.bones[b].basePose));
            d = std::max(d, ortAbstand(g_app->modellWelt[b], g_app->anim.bones[b].basePose));
        }
        return d < 2e-3F;
    }));
    add(tu("MB4 Handpose: rechte Hand Faust (aus BOTH_MELEE1)", [] { (void)modellHandpose(modell::Seite::Rechts, 1); }));
    add(pause(0.3));
    add(pruefe("MB4 Handpose: die Finger relativ zur Hand wie im ersten Bild von BOTH_MELEE1", [] {
        const GlaAnimation& a = g_app->anim;
        std::vector<BoneMatrix> q;
        a.worldMatrices(g_app->animList[static_cast<std::size_t>(mb1::animNach("BOTH_MELEE1"))].firstFrame, q);
        const int h = boneIndex(a, "rhand");
        float d = 0.0F;
        for (const char* n : {"r_d1_j1", "r_d1_j2", "r_d2_j1", "r_d2_j2", "r_d4_j1", "r_d4_j2"}) {
            const int f = boneIndex(a, n);
            const BoneMatrix soll = modell::starrInvers(q[static_cast<std::size_t>(h)]) * q[static_cast<std::size_t>(f)];
            const BoneMatrix ist = modell::starrInvers(g_app->modellWelt[static_cast<std::size_t>(h)]) * g_app->modellWelt[static_cast<std::size_t>(f)];
            d = std::max(d, drehDiff(soll, ist));
        }
        return d < 2e-3F;
    }));
    // --- Auswahl: Blender [ ] Strg+Umschalt+M, Max Bild auf/ab -----------------------------------------------------------
    add(tu("MB4 Auswahl: Knochen rradius", [] {
        g_app->mtab.griffWahl = -1;
        g_app->mtab.knochenWahl = boneIndex(g_app->anim, "rradius");
    }));
    add(taste(ImGuiKey_LeftBracket, "[ (Blender: Eltern)"));
    add(pause(0.1));
    add(pruefe("MB4 Auswahl Blender [: rhumerus", [] { return g_app->mtab.knochenWahl == boneIndex(g_app->anim, "rhumerus"); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_M, "Strg+Umschalt+M (Gegenseite)"));
    add(pause(0.1));
    add(pruefe("MB4 Auswahl Blender Strg+Umschalt+M: lhumerus", [] { return g_app->mtab.knochenWahl == boneIndex(g_app->anim, "lhumerus"); }));
    add(taste(ImGuiKey_RightBracket, "] (Blender: Kind)"));
    add(pause(0.1));
    add(pruefe("MB4 Auswahl Blender ]: ein Kind von lhumerus", [] {
        return g_app->mtab.knochenWahl >= 0 && g_app->anim.bones[static_cast<std::size_t>(g_app->mtab.knochenWahl)].parent == boneIndex(g_app->anim, "lhumerus");
    }));
    add(tu("MB4 Auswahl: Bedienung wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(taste(ImGuiKey_PageUp, "Bild auf (Max: Eltern)"));
    add(pause(0.1));
    add(pruefe("MB4 Auswahl Max Bild auf: lhumerus", [] { return g_app->mtab.knochenWahl == boneIndex(g_app->anim, "lhumerus"); }));
    add(taste(ImGuiKey_PageDown, "Bild ab (Max: Kind)"));
    add(pause(0.1));
    add(pruefe("MB4 Auswahl Max Bild ab: ein Kind von lhumerus", [] {
        return g_app->mtab.knochenWahl >= 0 && g_app->anim.bones[static_cast<std::size_t>(g_app->mtab.knochenWahl)].parent == boneIndex(g_app->anim, "lhumerus");
    }));
    add(tu("MB4: alle Schluessel weg (Strg+Z bis zum Anlegen)", [] {
        while (g_app->mtab.dok.kannRueckgaengig()) { (void)modellRueckgaengig(); }
        g_app->animFrame = g_app->animList[static_cast<std::size_t>(g_app->animIndex)].firstFrame + 10;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    // --- Ghosts und Silhouette im Bild ----------------------------------------------------------------------------------
    auto ohne = std::make_shared<mb1::AnsichtBild>();
    add(tu("MB4 Ghosts: Bild ohne", [=] { (void)mb1::ansichtLesen(*ohne); }));
    add(tu("MB4 Ghosts: 3 davor (blau, Abstand 3), keiner danach", [] {
        g_app->mtab.geister = ModellGeisterEinst{};
        g_app->mtab.geister.an = true;
        g_app->mtab.geister.davor = 3;
        g_app->mtab.geister.danach = 0;
        g_app->mtab.geister.abstand = 3;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("MB4 Ghosts: 3 gezeichnet; die geaenderten Bildpunkte liegen bei den Posen davor und sind blauer", [=] {
        mb1::AnsichtBild mit;
        if (!mb1::ansichtLesen(mit) || mit.w != ohne->w || mit.h != ohne->h) { return false; }
        // der Schirmbereich der Ghost-Posen: alle Knochen der drei Bilder davor (Rand: die Haut reicht ueber die Knochen)
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        float x0 = 1e9F, y0 = 1e9F, x1 = -1e9F, y1 = -1e9F;
        std::vector<BoneMatrix> w;
        for (int k = 1; k <= 3; ++k) {
            g_app->anim.worldMatrices(g_app->animFrame - 3 * k, w);
            for (const BoneMatrix& m : w) {
                const float p[3] = {m.m[0][3], m.m[1][3], m.m[2][3]};
                ImVec2 sp;
                if (!modellAufSchirm(a, p, sp)) { continue; }
                x0 = std::min(x0, sp.x - a.lo[0]);
                y0 = std::min(y0, sp.y - a.lo[1]);
                x1 = std::max(x1, sp.x - a.lo[0]);
                y1 = std::max(y1, sp.y - a.lo[1]);
            }
        }
        const float rand = (x1 - x0) * 0.35F;
        int anders = 0;
        int drin = 0;
        long long blauer = 0;
        for (int y = 0; y < mit.h; ++y) {
            for (int x = 0; x < mit.w; ++x) {
                const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(mit.w) + static_cast<std::size_t>(x)) * 4U;
                const int d = std::abs(mit.px[i] - ohne->px[i]) + std::abs(mit.px[i + 1] - ohne->px[i + 1]) + std::abs(mit.px[i + 2] - ohne->px[i + 2]);
                if (d < 12) { continue; }
                ++anders;
                if (static_cast<float>(x) >= x0 - rand && static_cast<float>(x) <= x1 + rand && static_cast<float>(y) >= y0 - rand && static_cast<float>(y) <= y1 + rand) { ++drin; }
                blauer += (mit.px[i + 2] - ohne->px[i + 2]) - (mit.px[i] - ohne->px[i]);
            }
        }
        char zeile[200];
        std::snprintf(zeile, sizeof zeile, "Knopftest: MB4 Ghosts - %d gezeichnet, %d Bildpunkte anders, %d im Bereich, Blau-Rot %lld", g_app->mtab.geisterGezeichnet, anders,
                      drin, blauer);
        diag::detail(zeile);
        return g_app->mtab.geisterGezeichnet == 3 && anders > 500 && drin >= anders * 95 / 100 && blauer > 0;
    }));
    add(tu("MB4 Silhouette an, Ghosts aus", [] {
        g_app->mtab.geister.an = false;
        g_app->mtab.silhouette = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("MB4 Silhouette: am Becken schwarz, in der Ecke hell (232)", [] {
        mb1::AnsichtBild b;
        if (!mb1::ansichtLesen(b)) { return false; }
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        const BoneMatrix& m = g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "lower_lumbar"))];
        const float p[3] = {m.m[0][3], m.m[1][3], m.m[2][3]};
        ImVec2 sp;
        if (!modellAufSchirm(a, p, sp)) { return false; }
        const auto px = [&](int x, int y) {
            const std::size_t i = (static_cast<std::size_t>(std::clamp(y, 0, b.h - 1)) * static_cast<std::size_t>(b.w) + static_cast<std::size_t>(std::clamp(x, 0, b.w - 1))) * 4U;
            return std::array<int, 3>{b.px[i], b.px[i + 1], b.px[i + 2]};
        };
        const auto k = px(static_cast<int>(sp.x - a.lo[0]), static_cast<int>(sp.y - a.lo[1]));
        const auto e = px(3, 3);
        diag::detail("Knopftest: MB4 Silhouette - Becken " + std::to_string(k[0]) + "/" + std::to_string(k[1]) + "/" + std::to_string(k[2]) + ", Ecke " +
                     std::to_string(e[0]));
        return k[0] < 30 && k[1] < 30 && k[2] < 30 && std::abs(e[0] - 232) <= 2 && std::abs(e[2] - 232) <= 2;
    }));
    add(tu("MB4 Silhouette aus", [] {
        g_app->mtab.silhouette = false;
        g_app->modelDirty = true;
    }));
    // --- Bewegungspfad -------------------------------------------------------------------------------------------------
    add(tu("MB4 Pfad: rhand waehlen, Pfad fuer die Auswahl, 10 davor / 10 danach", [] {
        g_app->mtab.griffWahl = -1;
        g_app->mtab.tagWahl = -1;
        g_app->mtab.knochenWahl = boneIndex(g_app->anim, "rhand");
        g_app->mtab.pfadDavor = 10;
        g_app->mtab.pfadDanach = 10;
        (void)modellPfadHinzu();
        // ein Schluessel am aktuellen Bild: Hand R um Z 20 Grad
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        const float w[3] = {0.0F, 0.0F, 20.0F};
        modellRigZiehen(2, w);
        modellRigZiehenEnde();
        g_app->mtab.pfadPunkte = 0;
        g_app->mtab.pfadSchluessel = 0;
    }));
    add(pause(0.3));
    add(pruefe("MB4 Pfad: 21 Punkte je Bild, der Schluessel hervorgehoben, Punkt des Bilds = Ort der Hand in der Pose", [] {
        const int h = boneIndex(g_app->anim, "rhand");
        const modell::RigSchicht* sch = schicht();
        const std::vector<std::array<float, 3>> p = modell::bewegungsPfad(g_app->anim, sch != nullptr ? *sch : modell::RigSchicht{}, h, g_app->animFrame, g_app->animFrame);
        const BoneMatrix& m = g_app->modellWelt[static_cast<std::size_t>(h)];
        const bool ort = p.size() == 1U && std::fabs(p[0][0] - m.m[0][3]) < 1e-3F && std::fabs(p[0][1] - m.m[1][3]) < 1e-3F && std::fabs(p[0][2] - m.m[2][3]) < 1e-3F;
        diag::detail("Knopftest: MB4 Pfad - Punkte " + std::to_string(g_app->mtab.pfadPunkte) + ", Schluessel " + std::to_string(g_app->mtab.pfadSchluessel));
        return g_app->mtab.pfadPunkte == 21 && g_app->mtab.pfadSchluessel == 1 && ort;
    }));
    if (handbuch) {
        add(tu("MB4 Handbuch: Ansicht von schraeg vorn eingepasst, kein Gizmo", [] {
            g_app->mtab.gizmo = 0;
            g_app->modelOrbit.turn(g_app->modelCam, 35.0F, 10.0F);
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(tu("MB4 Handbuch: Ghosts 3/3, Pfade rhand, rtalus, ltalus, Uebersicht zu", [] {
            g_app->mtab.geister = ModellGeisterEinst{};
            g_app->mtab.geister.an = true;
            g_app->mtab.geister.abstand = 3;
            for (const char* n : {"rtalus", "ltalus"}) {
                g_app->mtab.knochenWahl = boneIndex(g_app->anim, n);
                (void)modellPfadHinzu();
            }
            g_app->mtab.pfadDavor = 20;
            g_app->mtab.pfadDanach = 20;
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(hilfe2::schrittFoto("h_mb4_ghosts"));
        add(tu("MB4 Handbuch: Ghosts aus, Rig-Uebersicht auf", [] {
            g_app->mtab.geister.an = false;
            g_app->mtab.uebersichtOffen = true;
            g_app->mtab.uebersichtRolle = static_cast<int>(modell::Rolle::Hand) * 3 + static_cast<int>(modell::Seite::Rechts);
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(hilfe2::schrittFoto("h_mb4_uebersicht"));
        add(tu("MB4 Handbuch: Uebersicht zu, Animation-Menue offen", [] { g_app->mtab.uebersichtOffen = false; }));
        add(klickMenue(tr(Str::MbBereichAnimation), true));
        add(pause(0.4));
        add(hilfe2::schrittFoto("h_mb4_menue"));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.2));
    }
    // --- Backen auf einen Platz, gegen die Welt der Schicht ---------------------------------------------------------------
    add(tu("MB4 Backen: Karte mb4test, Ziel Karten-GLA", [] {
        std::snprintf(g_app->mtab.kgKarte, sizeof g_app->mtab.kgKarte, "%s", "mb4test");
        modellGlaZielSetzen(0);
    }));
    add(pause(0.3));
    ueberSuche(s, Str::MbRgBacken);
    add(pruefe("MB4 Backen: BOTH_CIN_1 traegt BOTH_WALK1_rig; jedes Bild gegen die Welt der Schicht innerhalb der Kompression", [] {
        const ModellTab& t = g_app->mtab;
        if (t.plaetze.empty() || !t.plaetze[0].belegt || t.plaetze[0].quelle.sequenz.name != "BOTH_WALK1_rig" || t.plaetze[0].quelle.gla == nullptr) { return false; }
        const GlaAnimation& g = *t.plaetze[0].quelle.gla;
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        const modell::RigSchicht* sch = schicht();
        float maxOrt = 0.0F;
        std::vector<BoneMatrix> soll;
        std::vector<BoneMatrix> ist;
        for (int f = 0; f < e.numFrames; ++f) {
            modell::rigWelt(g_app->anim, sch != nullptr ? *sch : modell::RigSchicht{}, e.firstFrame + f, e.firstFrame + f, 0.0F, soll);
            g.worldMatrices(f, ist);
            for (std::size_t b = 0; b < ist.size(); ++b) { maxOrt = std::max(maxOrt, ortAbstand(ist[b], soll[b])); }
        }
        // je Knochen der Kette (bis 8) eine halbe Verschiebungsstufe je Achse und 2 Quaternionstufen am Hebel der Figur (65)
        const float grenze = 8.0F * (std::sqrt(3.0F) / 128.0F + 4.0F * 2.0F / 16383.0F * 65.0F);
        char zeile[160];
        std::snprintf(zeile, sizeof zeile, "Knopftest: MB4 Backen - %d Bilder, groesste Ortsabweichung %.5f (Grenze %.3f)", g.numFrames, static_cast<double>(maxOrt),
                      static_cast<double>(grenze));
        diag::detail(zeile);
        return g.numFrames == e.numFrames && maxOrt < grenze;
    }));
    // --- Uebersicht und Zuordnung von Hand ------------------------------------------------------------------------------
    ueberSuche(s, Str::MbRgUebersicht);
    add(pruefe("MB4 Uebersicht: das Fenster ist offen", [] { return g_app->mtab.uebersichtOffen; }));
    add(tu("MB4 Uebersicht: rtail als Hand R zuordnen (von Hand)", [] { modellRigZuordnen(boneIndex(g_app->anim, "rtail"), modell::Rolle::Hand, modell::Seite::Rechts); }));
    add(pause(0.2));
    add(pruefe("MB4 Uebersicht: die Zuordnung gilt (Rig neu angelegt), Strg+Z-Schritt", [] {
        const auto& k = g_app->mtab.rig.erkennung.knochen[static_cast<std::size_t>(boneIndex(g_app->anim, "rtail"))];
        return k.vonHand && k.rolle == modell::Rolle::Hand && g_app->mtab.dok.rueckgaengigName() == tr(Str::MbRgUndoZuordnung);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB4 Uebersicht: nach Strg+Z ist rtail wieder Anhang", [] {
        const auto& k = g_app->mtab.rig.erkennung.knochen[static_cast<std::size_t>(boneIndex(g_app->anim, "rtail"))];
        return !k.vonHand && k.rolle == modell::Rolle::Anhang;
    }));
    add(tu("MB4 Uebersicht zu", [] { g_app->mtab.uebersichtOffen = false; }));
    // --- Leicht zu finden: jede Aktion in Menue, Rechtsklick, Suche, mit Tooltip ---------------------------------------
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(pruefe("MB4 Finden: das Menue Animation zeigt jede Rig-Aktion", [] {
        int da = 0;
        int alle = 0;   // Stufe 4: die Gruppe IK/FK steht im eigenen Untermenue (selbsttest_mb6.inc prueft sie)
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (modellRigAktionGruppe(i) >= 5) { continue; }   // Stufe 6: Auto-Physik ebenso (selbsttest_ph.inc)
            ++alle;
            da += imMenue(modellRigAktionText(i)) != nullptr ? 1 : 0;
        }
        diag::detail("Knopftest: MB4 Menue - " + std::to_string(da) + " von " + std::to_string(alle) + "; " +
                     tr(Str::MbRgKopieren) + " steht in " + woIst(tr(Str::MbRgKopieren)));
        return da == alle && alle > 0;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(klickAn("rechts in die Ansicht", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + 20.0F};
    }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbRgGruppeRig), false));
    add(pause(0.3));
    add(pruefe("MB4 Finden: der Rechtsklick hat das Untermenue Control Rig mit jeder Aktion", [] {
        int da = 0;
        int alle = 0;   // Stufe 4: die Gruppe IK/FK steht im eigenen Untermenue (selbsttest_mb6.inc prueft sie)
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (modellRigAktionGruppe(i) >= 5) { continue; }   // Stufe 6: Auto-Physik ebenso (selbsttest_ph.inc)
            ++alle;
            da += imMenue(modellRigAktionText(i)) != nullptr ? 1 : 0;
        }
        diag::detail("Knopftest: MB4 Rechtsklick - " + std::to_string(da) + " von " + std::to_string(alle) + "; " +
                     tr(Str::MbRgKopieren) + " steht in " + woIst(tr(Str::MbRgKopieren)) + ", Kontext offen " + std::to_string(g_app->mtab.kontextSichtbar ? 1 : 0));
        return da == alle && alle > 0;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MB4 Finden: jede Rig-Aktion hat einen Tooltip; Tasten je Bedienung mit Quelle", [] {
        int ok = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) { ok += modellRigAktionHinweis(i)[0] != '\0' ? 1 : 0; }
        int max = 0;
        int blender = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            max += modellRigAktionTaste(i, false)[0] != '\0' ? 1 : 0;
            blender += modellRigAktionTaste(i, true)[0] != '\0' ? 1 : 0;
        }
        diag::detail("Knopftest: MB4 Tasten - wie 3ds Max " + std::to_string(max) + ", wie Blender " + std::to_string(blender));
        return ok == modellRigAktionZahl() && max == 2 && blender == 7;
    }));
    add(klickAn("in die Ansicht (Fokus)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    add(pause(0.2));
    add(klickAn("noch einmal in die Ansicht (das Rechtsklick-Menue nimmt den ersten Klick)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
    }));
    add(taste(ImGuiKey_X, "X (Alle Aktionen suchen)"));
    add(pause(0.2));
    add(tippe(tr(Str::MbRgGhosts)));
    add(pause(0.2));
    add(pruefe("MB4 Finden: die Suche findet die Ghosts", [] {
        diag::detail(std::string("Knopftest: MB4 Suche - ") + tr(Str::MbRgGhosts) + " steht in " + woIst(tr(Str::MbRgGhosts)) + ", Suche offen " +
                     std::to_string(g_app->mtab.aktionenSichtbar ? 1 : 0));
        return imMenue(tr(Str::MbRgGhosts)) != nullptr;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("MB4: zurueck", [] {
        while (g_app->mtab.dok.kannRueckgaengig()) { (void)modellRueckgaengig(); }
        g_app->mtab.pfade.clear();
        g_app->mtab.geister.an = false;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBedienungSetzen(0);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    std::vector<Schritt> s;
    if (n == "rigrundlauf") {
        const char* sp = std::getenv("BHED_MB_SPIEL");
        const std::string spiel = sp != nullptr ? sp : "md";
        s.push_back(tu("MB4 Rig-Rundlauf: Spiel " + spiel, [=] {
            if (spiel != "md") { mb1::spielSetzen(spiel); }
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        }));
        s.push_back(pause(0.3));
        s.push_back(rundlauf(spiel));
        return s;
    }
    if (n == "rig") { return uiSchritte(false); }
    return s;
}

std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = uiSchritte(true);
    return s;
}

}  // namespace rig3
