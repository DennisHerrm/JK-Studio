// selbsttest_optsicher.inc - "Fix all safe" darf nichts Sichtbares aendern (Fund des Anwenders,
// md_rev_TarisUpper2: Flaechen hinter Fenstern und Streifen zwischen Pfeilern wurden caulk,
// Kanten verschoben).
//
// Modus karteneditor mit BHED_KARTE_NUR=optsicher und BHED_OPTSICHER_KARTEN="<a.bsp|a.map>;...":
// je Karte: oeffnen (.bsp wie File > Open .bsp); Optimizer pruefen, "Fix all safe"; aus dem
// Unterschied die Blicke: je geaenderte Flaeche (hoechstens 24) eine Kamera 64 vor der Flaeche
// auf der anderen Seite des deckenden Brushes, auf die Flaeche gerichtet (sieht man sie durch ein
// Fenster, aendert sich das Bild), dazu die Spielerstarts in vier Richtungen. Rueckgaengig ->
// Stichproben (48 x 36, kameraPixel); Wiederholen -> dieselben Blicke. Geprueft: keine Stichprobe
// anders (Farbabstand > 40), kein Eckpunkt um mehr als 0,05 gewandert.
namespace optsicher {
namespace ke = karteneditor;

struct Blick {
    karte::Vektor3f ort;
    float nick = 0;
    float gier = 0;
    std::string was;   // Blick auf eine caulk-Flaeche: welche
};
struct Lauf {
    std::vector<Blick> blicke;
    // vorher zweimal (unruhige Pixel: Animation), dazu vorher und nachher mit dem Filter "Caulk" (View >
    // Filter): wo sich das Bild durch den Filter aendert, ist caulk zu sehen
    std::vector<std::vector<unsigned char>> vorher, vorher2, vorherOhne, nachher, nachherOhne;
    std::string text;   // die Karte vorher
    long long caulk = 0;
    int behoben = 0;
    std::vector<std::size_t> anders;   // Blicke mit Unterschied (fuer die Bilder mit BHED_OPTSICHER_BILDER)
    double wandern = 0;
};

std::vector<unsigned char> stichproben() {
    const ke::KameraFenster& f = ke::editor().kamFenster;
    std::vector<unsigned char> v;
    for (int y = 0; y < 36; ++y) {
        for (int x = 0; x < 48; ++x) {
            unsigned char c[4] = {0, 0, 0, 0};
            (void)ke::kameraPixel((f.breite * (2 * x + 1)) / 96, (f.hoehe * (2 * y + 1)) / 72, c);
            v.insert(v.end(), c, c + 3);
        }
    }
    return v;
}

constexpr int kBlicke = 900;

// Unterschied der Karte vorher/jetzt: neue caulk-Flaechen (mit Blick darauf) und gewanderte Eckpunkte
void unterschied(Lauf& l) {
    karte::Karte alt;
    (void)karte::karteLesen(l.text, alt);
    const karte::KartenAbbild a = karte::kartenAbbild(alt);
    const karte::KartenAbbild b = karte::kartenAbbild(ke::editor().karte);
    std::vector<Blick> flaechen;
    // Blicke nur aus dem freien Raum (im Festen sieht die Editor-Kamera auch Flaechen, die im Spiel nie
    // jemand sieht): Kollision aus den Brushes, ein Kasten +-8 darf nicht im Festen stecken
    KollisionsKarte koll;
    (void)karte::kollisionAusKarte(ke::editor().karte, [](const std::string&) { return std::vector<std::string>{}; }, true, koll);
    const auto frei = [&koll](const karte::Vektor3d& q) {
        const float p0[3] = {static_cast<float>(q[0]), static_cast<float>(q[1]), static_cast<float>(q[2])};
        const float mi[3] = {-8, -8, -8}, ma[3] = {8, 8, 8};
        return !kollisionsSpur(koll, p0, mi, ma, p0, 1U, 0).startsolid;
    };
    // gleiche Primitive suchen: geloeschte (entartete) Pinsel fehlen nachher, die Reihenfolge bleibt
    const auto gleich = [](const karte::AbbildPrimitiv& x, const karte::AbbildPrimitiv& y) {
        if (x.patch != y.patch || x.flaechen.size() != y.flaechen.size()) { return false; }
        for (std::size_t f = 0; f < x.flaechen.size(); ++f) {
            const karte::Vektor3d nx = x.flaechen[f].ebene.normal();
            const karte::Vektor3d ny = y.flaechen[f].ebene.normal();
            if (std::fabs(nx[0] - ny[0]) + std::fabs(nx[1] - ny[1]) + std::fabs(nx[2] - ny[2]) > 0.01 || std::fabs(x.flaechen[f].ebene.d - y.flaechen[f].ebene.d) > 0.6) {
                return false;
            }
        }
        return true;
    };
    for (std::size_t e = 0; e < a.ents.size() && e < b.ents.size(); ++e) {
        std::size_t q2 = 0;
        for (std::size_t p = 0; p < a.ents[e].prims.size() && q2 < b.ents[e].prims.size(); ++p) {
            const auto& pa = a.ents[e].prims[p];
            if (!gleich(pa, b.ents[e].prims[q2])) { continue; }   // nachher geloescht
            const auto& pb = b.ents[e].prims[q2++];
            for (std::size_t f = 0; f < pa.flaechen.size() && f < pb.flaechen.size(); ++f) {
                const bool vorCaulk = pa.flaechen[f].shader.find("caulk") != std::string::npos;
                const bool nachCaulk = pb.flaechen[f].shader.find("caulk") != std::string::npos;
                if (vorCaulk || !nachCaulk || pa.flaechen[f].punkte.empty()) { continue; }
                ++l.caulk;
                karte::Vektor3d m(0, 0, 0);
                for (const auto& q : pa.flaechen[f].punkte) { m = m + q; }
                m = m * (1.0 / static_cast<double>(pa.flaechen[f].punkte.size()));
                const karte::Vektor3d n = pa.flaechen[f].ebene.normal();   // zeigt in den deckenden Brush
                Blick bl;
                bool gefunden = false;
                for (const double weite : {48.0, 96.0, 192.0, 384.0}) {   // hinter dem deckenden Brush, im Freien
                    const karte::Vektor3d q(m[0] + n[0] * weite, m[1] + n[1] * weite, m[2] + n[2] * weite);
                    if (!frei(q)) { continue; }
                    bl.ort = karte::Vektor3f(static_cast<float>(q[0]), static_cast<float>(q[1]), static_cast<float>(q[2]));
                    gefunden = true;
                    break;
                }
                if (!gefunden) { continue; }
                bl.was = "ent " + std::to_string(e) + " prim " + std::to_string(p) + " face " + std::to_string(f) + " " + pa.flaechen[f].shader;
                bl.gier = static_cast<float>(std::atan2(-n[1], -n[0]) * 180.0 / 3.14159265);
                bl.nick = static_cast<float>(std::asin(std::clamp(-n[2], -1.0, 1.0)) * 180.0 / 3.14159265);
                flaechen.push_back(bl);
            }
            for (const auto& fa : pa.flaechen) {
                for (const auto& va : fa.punkte) {
                    double best = 1e30;
                    for (const auto& fb : pb.flaechen) {
                        for (const auto& vb : fb.punkte) {
                            best = std::min(best, std::fabs(va[0] - vb[0]) + std::fabs(va[1] - vb[1]) + std::fabs(va[2] - vb[2]));
                        }
                    }
                    if (!pb.flaechen.empty()) { l.wandern = std::max(l.wandern, best); }
                }
            }
        }
    }
    // gleichmaessig hoechstens 860 Flaechen, dazu die Spielerstarts
    if (std::getenv("BHED_OPTSICHER_NURFEST") != nullptr) { flaechen.clear(); }   // Untersuchung: nur die festen Blicke
    const char* maxFl = std::getenv("BHED_OPTSICHER_MAXFL");   // Blicke auf caulk-Flaechen je Karte (Vorgabe 860)
    const std::size_t m = std::min<std::size_t>(flaechen.size(), maxFl != nullptr ? std::clamp<std::size_t>(std::strtoul(maxFl, nullptr, 10), 1, 860) : 860);
    for (std::size_t i = 0; i < m; ++i) { l.blicke.push_back(flaechen[i * flaechen.size() / m]); }
    // feste Punkte im Spielraum: Spielerstarts und alle Punkt-Entities (Lichter, Gegenstaende, Ziele) im
    // Freien, mindestens 128 auseinander, je 4 Richtungen leicht nach unten
    std::vector<karte::Vektor3f> orte;
    for (const auto& en : ke::editor().karte.entitaeten()) {
        if (!en->hat("origin") || en->wert("classname") == "worldspawn" || en->hat("model")) { continue; }
        const karte::Vektor3f o = karte::entitaetUrsprung(*en);
        if (!frei(karte::Vektor3d(o[0], o[1], o[2]))) { continue; }
        bool nah = false;
        for (const auto& q : orte) { nah = nah || std::fabs(q[0] - o[0]) + std::fabs(q[1] - o[1]) + std::fabs(q[2] - o[2]) < 128.0F; }
        if (!nah) { orte.push_back(o); }
    }
    if (const char* fest = std::getenv("BHED_OPTSICHER_ORTE")) {   // Untersuchung: feste Blicke "x y z nick gier;..."
        std::string t = fest;
        for (std::size_t i = 0; i < t.size();) {
            std::size_t j = t.find(';', i);
            if (j == std::string::npos) { j = t.size(); }
            Blick bl;
            float x = 0, y = 0, zz = 0;
            if (std::sscanf(t.substr(i, j - i).c_str(), "%f %f %f %f %f", &x, &y, &zz, &bl.nick, &bl.gier) == 5) {
                bl.ort = karte::Vektor3f(x, y, zz);
                bl.was = "fest";
                l.blicke.push_back(bl);
            }
            i = j + 1;
        }
    }
    const std::size_t no = std::min<std::size_t>(orte.size(), static_cast<std::size_t>(kBlicke - 860) / 4);
    diag::detail("Knopftest: Optsicher - Blicke auf " + std::to_string(flaechen.size()) + " caulk-Flaechen, " + std::to_string(orte.size()) + " freie Punkte");
    for (std::size_t i = 0; i < no; ++i) {
        for (int r = 0; r < 4; ++r) { l.blicke.push_back({orte[i * orte.size() / no], -15.0F, 90.0F * static_cast<float>(r) + 45.0F * static_cast<float>(i % 2)}); }
    }
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    std::vector<std::string> karten;
    if (const char* k = std::getenv("BHED_OPTSICHER_KARTEN")) {
        std::string t = k;
        for (std::size_t a = 0; a < t.size();) {
            std::size_t e = t.find(';', a);
            if (e == std::string::npos) { e = t.size(); }
            if (e > a) { karten.push_back(t.substr(a, e - a)); }
            a = e + 1;
        }
    }
    for (const std::string& pfad : karten) {
        auto l = std::make_shared<Lauf>();
        add(tu("Optsicher: " + pfad + " oeffnen", [pfad] {
            karteDialogeZu();
            ke::konsoleLeeren();
            stufe3::q3map2Bereitstellen();   // bhed_q3map2.exe neben das Programm des Laufs
            // eine .bsp wie der Anwender: File > Open .bsp (dekompilieren, Modelle und Flaechen zurueckgewinnen)
            const bool bsp = pfad.size() > 4 && pfad.substr(pfad.size() - 4) == ".bsp";
            if (!(bsp ? ke::bspOeffnenDatei(pfad) : ke::karteOeffnen(pfad))) { meldeFehler("Optsicher: nicht lesbar: " + pfad); }
            ke::editor().optimierer.nachBau = false;
        }));
        add(pause(1.0));
        add(tu("Optsicher: Karte merken, Optimizer pruefen", [l] {
            l->text = karte::karteSchreiben(ke::editor().karte);
            ke::optimiererStarten(false);
        }));
        add(stufe8::warteSek("Optsicher: Pruefung fertig", [] { return !ke::optimiererLaeuft(); }, 900));
        add(tu("Optsicher: Fix all safe, Unterschied, Rueckgaengig", [l] {
            (void)ke::optimiererBerichtSchreiben(platform::settingsDirectory() + "\\optsicher_bericht.txt");
            const char* nur = std::getenv("BHED_OPTSICHER_NUR");   // Untersuchung: nur diese eine Behebung
            const int n = nur != nullptr ? (ke::optimiererBefundBeheben(nur) ? 1 : 0) : ke::optimiererAllesSichere();
            unterschied(*l);
            diag::detail("Knopftest: Optsicher - behoben: " + std::to_string(n) + ", " + std::to_string(l->caulk) + " Flaechen caulk, " +
                         std::to_string(l->blicke.size()) + " Blicke");
            l->behoben = n;
            // vorher und nachher je frisch geladen (nicht Undo/Redo: nach dem Undo dieses grossen Schritts
            // fehlten im Editor Modelle bis zum Redo - das waere ein Unterschied des Editors, nicht der Karte)
            const std::string nach = karte::karteSchreiben(ke::editor().karte);
            for (int i = 0; i < 2; ++i) {
                const std::string pf = platform::settingsDirectory() + (i == 0 ? "\\optsicher_vor.map" : "\\optsicher_nach.map");
                std::ofstream(std::filesystem::path(std::u8string(pf.begin(), pf.end())), std::ios::binary) << (i == 0 ? l->text : nach);
            }
        }));
        add(tu("Optsicher: vorher frisch laden", [] { (void)ke::karteOeffnen(platform::settingsDirectory() + "\\optsicher_vor.map"); }));
        add(pause(1.0));
        const auto caulkFilter = [](bool an) {
            ke::Editor& ed = ke::editor();
            ed.filterMaske = an ? (ed.filterMaske | karte::EXCLUDE_CAULK) : (ed.filterMaske & ~static_cast<unsigned>(karte::EXCLUDE_CAULK));
            karte::filterAnwenden(ed.karte, ed.filterMaske);
            ++ed.sichtStand;
        };
        for (int phase = 0; phase < 5; ++phase) {
            if (phase == 2 || phase == 4) {
                add(tu("Optsicher: Filter Caulk an", [caulkFilter] { caulkFilter(true); }));
                add(pause(0.5));
            }
            if (phase == 3) {
                add(tu("Optsicher: Filter Caulk aus, nachher frisch laden", [caulkFilter] {
                    caulkFilter(false);
                    (void)ke::karteOeffnen(platform::settingsDirectory() + "\\optsicher_nach.map");
                }));
                add(pause(1.0));
            }
            // ein Schritt je Durchgang: je Blick Kamera setzen, warten bis alle Texturen geladen sind
            // (keine Platzhalter) und noch zwei Bilder, dann abtasten
            auto z = std::make_shared<std::array<int, 3>>();   // Blick, Bilder im Blick, Bild "fertig"
            Schritt d{"Optsicher: Blicke, Durchgang " + std::to_string(phase), [l, phase, z](int b) {
                          std::array<int, 3>& q = *z;
                          if (b == 0) { q = {0, 0, -1}; }
                          if (q[0] >= static_cast<int>(l->blicke.size())) { return true; }
                          if (q[1] == 0) {
                              ke::Editor& ed = ke::editor();
                              const Blick& bl = l->blicke[static_cast<std::size_t>(q[0])];
                              ed.kamera.ursprung = bl.ort;
                              ed.kamera.winkel = karte::Vektor3f(bl.nick, bl.gier, 0.0F);
                          }
                          ++q[1];
                          if (q[2] < 0 && q[1] >= 3 && ke::kameraBilderFertig()) { q[2] = q[1]; }
                          if (q[1] < 400 && (q[2] < 0 || q[1] < q[2] + 3)) { return false; }
                          (phase == 0 ? l->vorher : phase == 1 ? l->vorher2 : phase == 2 ? l->vorherOhne : phase == 3 ? l->nachher : l->nachherOhne).push_back(stichproben());
                          q = {q[0] + 1, 0, -1};
                          return false;
                      }};
            d.eigeneZeitgrenze = true;
            add(d);
        }
        add(tu("Optsicher: Filter Caulk aus", [caulkFilter] { caulkFilter(false); }));
        // Mit dem Filter "Caulk" sieht die Kamera wie das Spiel (caulk ist nodraw): unterscheiden sich die
        // Bilder vorher/nachher dort an einer ruhigen Stichprobe, ist eine sichtbare Flaeche verschwunden.
        // Daneben zur Auskunft: Stichproben, die sich ohne Filter unterscheiden.
        add(pruefe("Optsicher: " + pfad + " - Fix all safe aendert nichts Sichtbares (Kamera, Eckpunkte)", [l, pfad] {
            int anders = 0, blickeAnders = 0, unruhig = 0, sonst = 0;
            const std::size_t nb = std::min({l->vorher.size(), l->vorher2.size(), l->vorherOhne.size(), l->nachher.size(), l->nachherOhne.size()});
            for (std::size_t b = 0; b < nb; ++b) {
                int n = 0;
                for (std::size_t i = 0; i + 2 < l->vorher[b].size(); i += 3) {
                    const auto abst = [i](const std::vector<unsigned char>& x, const std::vector<unsigned char>& y) {
                        return std::abs(x[i] - y[i]) + std::abs(x[i + 1] - y[i + 1]) + std::abs(x[i + 2] - y[i + 2]);
                    };
                    if (abst(l->vorher[b], l->vorher2[b]) > 40) {   // schon ohne Aenderung unruhig (Animation)
                        ++unruhig;
                        continue;
                    }
                    if (abst(l->vorher[b], l->nachher[b]) > 40) { ++sonst; }
                    if (abst(l->vorherOhne[b], l->nachherOhne[b]) > 40) { ++n; }
                }
                anders += n;
                blickeAnders += n > 0 ? 1 : 0;
                if (n > 0 && b < l->blicke.size()) {
                    l->anders.push_back(b);
                    const Blick& bl = l->blicke[b];
                    char w[400];
                    std::snprintf(w, sizeof(w), "Optsicher: Blick %zu wie im Spiel anders (%d) %s: %.0f %.0f %.0f nick %.0f gier %.0f", b, n, bl.was.c_str(), static_cast<double>(bl.ort[0]),
                                  static_cast<double>(bl.ort[1]), static_cast<double>(bl.ort[2]), static_cast<double>(bl.nick), static_cast<double>(bl.gier));
                    diag::detail(w);
                }
            }
            int verschieden = 0;   // Gegenprobe: zeigt die Kamera die Karte?
            for (std::size_t v = 1; v < l->vorher.size(); ++v) { verschieden += l->vorher[v] != l->vorher[v - 1] ? 1 : 0; }
            char z[600];
            std::snprintf(z, sizeof(z), "Knopftest: Optsicher - %s: %zu Blicke (%d verschieden, %d Stichproben unruhig, %d sonst anders), %d Stichproben wie im Spiel anders in %d Blicken, %lld Flaechen neu caulk, Eckpunkte bis %.3f gewandert",
                          pfad.c_str(), l->vorher.size(), verschieden, unruhig, sonst, anders, blickeAnders, l->caulk, l->wandern);
            diag::detail(z);
            return (l->vorher.size() <= 1 || verschieden > 0) && anders == 0 && l->wandern <= 0.051;
        }));
        if (std::getenv("BHED_OPTSICHER_BILDER") != nullptr) {   // Untersuchung: die ersten 6 abweichenden Blicke als Bild
            add(tu("Optsicher: Bilder - Filter Caulk an (wie im Spiel)", [caulkFilter] { caulkFilter(true); }));
            for (int phase = 0; phase < 2; ++phase) {
                if (phase == 1) {
                    add(tu("Optsicher: Bilder - vorher laden", [caulkFilter] {
                        (void)ke::karteOeffnen(platform::settingsDirectory() + "\\optsicher_vor.map");
                        caulkFilter(true);
                    }));
                    add(pause(1.0));
                }
                auto z = std::make_shared<std::array<int, 3>>();
                Schritt d{"Optsicher: Bilder " + std::to_string(phase), [l, phase, z](int b) {
                              std::array<int, 3>& q = *z;
                              if (b == 0) { q = {0, 0, -1}; }
                              if (q[0] >= static_cast<int>(std::min<std::size_t>(l->anders.size(), 6))) { return true; }
                              const std::size_t v = l->anders[static_cast<std::size_t>(q[0])];
                              if (q[1] == 0) {
                                  ke::Editor& ed = ke::editor();
                                  ed.kamera.ursprung = l->blicke[v].ort;
                                  ed.kamera.winkel = karte::Vektor3f(l->blicke[v].nick, l->blicke[v].gier, 0.0F);
                              }
                              ++q[1];
                              if (q[2] < 0 && q[1] >= 3 && ke::kameraBilderFertig()) { q[2] = q[1]; }
                              if (q[1] < 400 && (q[2] < 0 || q[1] < q[2] + 3)) { return false; }
                              const ke::KameraFenster& f = ke::editor().kamFenster;
                              const std::string pf = platform::settingsDirectory() + "\\optsicher_" + std::to_string(v) + (phase == 0 ? "_nach" : "_vor") + ".ppm";
                              std::ofstream o(std::filesystem::path(std::u8string(pf.begin(), pf.end())), std::ios::binary);
                              o << "P6\n240 180\n255\n";
                              for (int y = 0; y < 180; ++y) {
                                  for (int x = 0; x < 240; ++x) {
                                      unsigned char c[4] = {0, 0, 0, 0};
                                      (void)ke::kameraPixel((f.breite * (2 * x + 1)) / 480, (f.hoehe * (2 * y + 1)) / 360, c);
                                      o.write(reinterpret_cast<const char*>(c), 3);
                                  }
                              }
                              q = {q[0] + 1, 0, -1};
                              return false;
                          }};
                d.eigeneZeitgrenze = true;
                add(d);
            }
        }
    }
    return s;
}

}  // namespace optsicher
