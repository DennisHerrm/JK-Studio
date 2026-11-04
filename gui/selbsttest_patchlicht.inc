// selbsttest_patchlicht.inc - rc609 L (Modus "patchlicht"): Licht geht durch Patches (Tester: "Sometimes light
// goes through patches").
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// Testkarte im Laufordner: sechs geschlossene Kisten (innen 512 x 512 x 384) nebeneinander, je ein Licht 64 unter
// der Decke ueber der Mitte; zwischen Licht und Boden: Bogen-Patch, flacher Patch, Brush, nichts (Bezug), Bogen in
// einer func_group, flacher Patch in einer func_group mit "_cs" "0". Gemessen wird der Boden unter der Mitte, als
// Anteil am Boden der leeren Kiste: Lightmap-Werte der .bsp (Schnell-Backen, voller Bau) und Bildpunkte der Kamera
// (Echtzeit-Licht, Schnell-Backen, "Gebaute Karte wie im Spiel" aus Schnell-Backen und vollem Bau). Je einmal mit
// dem Schalter "Patch shadows" aus und an. Erwartet wie q3map2 (light_trace.cpp PopulateWithBSPModel):
//   aus  Patches ohne Schatten, der Brush mit
//   an   Patches in der Welt und in func_group mit Schatten; "_cs" "0" nie (GetEntityShadowFlags)
// Der volle Bau nimmt "[Test] Quick and Crude" (ohne -patchshadows in NRCs Vorgabe). Dazu: der Schalter steht in der
// Karte (Rueckgaengig), der Hinweis im Bau, das Schattennetz der Patches wie TessellatedMesh von -light.
// Gegenprobe (BHED_PATCHLICHT_GEGEN=0 schaltet sie ab): maps/mp/duel4.bsp aus den Spieldaten, rueckgewonnen, zweimal
// gebaut (Schalter aus/an).
namespace patchlicht {
namespace ke = karteneditor;

enum class Inhalt { Bogen, Flach, Pinsel, Leer, GruppeBogen, GruppeFlachCs0 };
struct Kiste {
    const char* name;
    Inhalt inhalt;
    bool schattenAn;    // Schatten mit dem Schalter (q3map2 -patchshadows)
    bool schattenAus;   // Schatten ohne
};
constexpr Kiste kKisten[] = {
    {"bogen", Inhalt::Bogen, true, false},           {"flach", Inhalt::Flach, true, false},
    {"brush", Inhalt::Pinsel, true, true},           {"leer", Inhalt::Leer, false, false},
    {"gruppe", Inhalt::GruppeBogen, true, false},    {"gruppe_cs0", Inhalt::GruppeFlachCs0, false, false},
};
constexpr std::size_t kN = std::size(kKisten);
constexpr std::size_t kLeer = 3;
constexpr float kAbstand = 768.0F;
const char* const kStein = "textures/yavin/temple_stone2";
// Grenzen des Anteils am Boden der leeren Kiste: Schatten hoechstens, Licht durch mindestens
constexpr double kSchatten = 0.7;
constexpr double kDurch = 0.9;
// TessellatedMesh von -light fuer die Patches der Karte (aus q3map2, von Hand nachgerechnet): Bogen longestCurve 384
// (ExpandLongestCurve: 160 + 7 x 32) -> IterationsForCurve(384, 8) = 2 -> 9 x 9, RemoveLinearMeshColumnsRows -> 9 x 2
// = 16 Dreiecke; flacher Patch longestCurve 0 -> 0 Iterationen, 3 x 3 -> 2 x 2 = 2 Dreiecke; "_cs" "0" wirft nicht
constexpr std::size_t kPatchDreieckeAn = 16 + 2 + 16;

float mitteX(std::size_t i) { return static_cast<float>(i) * kAbstand; }
std::string kartenPfad() { return karteTestPfad("patchlicht.map"); }

// Messwerte: [Schalter 0/1][Kiste]
using Reihe = std::array<double, kN>;
struct Messung {
    std::array<Reihe, 2> echt{};
    std::array<Reihe, 2> gebacken{};
    std::array<Reihe, 2> spielSchnell{};
    std::array<Reihe, 2> spielBau{};
    std::array<Reihe, 2> lmSchnell{};
    std::array<Reihe, 2> lmBau{};
    std::array<std::size_t, 2> patchDreiecke{};
    std::array<std::string, 2> hinweis;
    std::array<std::string, 2> lichtBefehl;
    // Gegenprobe
    std::array<double, 2> gegenSumme{};
    std::array<std::size_t, 2> gegenDreiecke{};
    std::array<std::string, 2> gegenHinweis;
    std::array<std::string, 2> gegenBefehl;
    std::size_t gegenPatches = 0;
    std::size_t gegenDunkler = 0;
    std::size_t gegenTexel = 0;
    bool schluesselAn = false;
    bool schluesselUndo = false;
    bool schluesselDatei = false;
};

std::shared_ptr<karte::Patch> patch(float cx, bool bogen) {
    auto p = std::make_shared<karte::Patch>();
    p->setzeGroesse(3, 3);
    std::vector<karte::PatchPunkt> pp(9);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            const float z = bogen ? (c == 1 ? 256.0F : 128.0F) : 160.0F;
            karte::PatchPunkt& q = pp[static_cast<std::size_t>(r * 3 + c)];
            q.vertex = karte::Vektor3f(cx - 96.0F + 96.0F * static_cast<float>(c), -96.0F + 96.0F * static_cast<float>(r), z);
            q.texcoord = karte::Vektor2f(0.5F * static_cast<float>(c), 0.5F * static_cast<float>(r));
        }
    }
    p->ladeShader(kStein);
    p->punkteSetzen(pp);
    return p;
}

void karteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](karte::Entitaet& ziel, float x, float y, float z, float hx, float hy, float hz) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(karte::Vektor3f(x, y, z), karte::Vektor3f(hx, hy, hz)), kStein, proj, k.kontext);
        ziel.kindEinfuegen(p);
    };
    for (std::size_t i = 0; i < kN; ++i) {
        const float x = mitteX(i);
        quader(*welt, x, 0, -8, 272, 272, 8);    // Boden
        quader(*welt, x, 0, 392, 272, 272, 8);   // Decke
        quader(*welt, x - 264, 0, 192, 8, 272, 192);
        quader(*welt, x + 264, 0, 192, 8, 272, 192);
        quader(*welt, x, -264, 192, 256, 8, 192);
        quader(*welt, x, 264, 192, 256, 8, 192);
        auto licht = k.neueEntitaet("light", false);
        licht->setzen("origin", std::to_string(static_cast<int>(x)) + " 0 320");
        licht->setzen("light", "2500");
        k.entitaetEinfuegen(licht);
        switch (kKisten[i].inhalt) {
        case Inhalt::Bogen: welt->kindEinfuegen(patch(x, true)); break;
        case Inhalt::Flach: welt->kindEinfuegen(patch(x, false)); break;
        case Inhalt::Pinsel: quader(*welt, x, 0, 160, 96, 96, 8); break;
        case Inhalt::Leer: break;
        case Inhalt::GruppeBogen:
        case Inhalt::GruppeFlachCs0: {
            auto g = k.neueEntitaet("func_group", false);
            if (kKisten[i].inhalt == Inhalt::GruppeFlachCs0) { g->setzen("_cs", "0"); }
            g->kindEinfuegen(patch(x, kKisten[i].inhalt == Inhalt::GruppeBogen));
            k.entitaetEinfuegen(g);
            break;
        }
        }
    }
    (void)karte::karteSpeichern(kartenPfad(), k, false);
}

// Lightmap des Bodens (z 0, Normale nach oben) bei (x, y): das Dreieck einer ebenen Flaeche, die Lightmap-Koordinate
// baryzentrisch, das Texel darunter; Mittel (r + g + b) / 3 aus 3 x 3 Punkten im Abstand 8. -1: nichts gefunden.
double lightmapBoden(const BspGeometry& g, float x, float y) {
    double summe = 0;
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const float px = x + 8.0F * static_cast<float>(dx);
            const float py = y + 8.0F * static_cast<float>(dy);
            bool gefunden = false;
            for (const BspSurface& s : g.surfaces) {
                if (gefunden) { break; }
                if (s.type != BspSurface::Type::Planar || s.lightmap < 0 || static_cast<std::size_t>(s.lightmap) >= g.lightmaps.size()) { continue; }
                for (int k = 0; k + 2 < s.numIndexes && !gefunden; k += 3) {
                    const BspVertex* v[3];
                    bool boden = true;
                    for (int j = 0; j < 3; ++j) {
                        const std::size_t idx = static_cast<std::size_t>(s.firstVert) + g.indexes[static_cast<std::size_t>(s.firstIndex + k + j)];
                        v[j] = &g.verts[idx];
                        boden = boden && std::fabs(v[j]->xyz[2]) < 0.5F && v[j]->normal[2] > 0.9F;
                    }
                    if (!boden) { continue; }
                    const double d = (v[1]->xyz[1] - v[2]->xyz[1]) * (v[0]->xyz[0] - v[2]->xyz[0]) + (v[2]->xyz[0] - v[1]->xyz[0]) * (v[0]->xyz[1] - v[2]->xyz[1]);
                    if (std::fabs(d) < 1e-6) { continue; }
                    const double a = ((v[1]->xyz[1] - v[2]->xyz[1]) * (px - v[2]->xyz[0]) + (v[2]->xyz[0] - v[1]->xyz[0]) * (py - v[2]->xyz[1])) / d;
                    const double b = ((v[2]->xyz[1] - v[0]->xyz[1]) * (px - v[2]->xyz[0]) + (v[0]->xyz[0] - v[2]->xyz[0]) * (py - v[2]->xyz[1])) / d;
                    const double c = 1.0 - a - b;
                    if (a < -1e-4 || b < -1e-4 || c < -1e-4) { continue; }
                    const double u = a * v[0]->lightmap[0] + b * v[1]->lightmap[0] + c * v[2]->lightmap[0];
                    const double w = a * v[0]->lightmap[1] + b * v[1]->lightmap[1] + c * v[2]->lightmap[1];
                    const int tx = std::clamp(static_cast<int>(u * BspGeometry::kLightmapSize), 0, BspGeometry::kLightmapSize - 1);
                    const int ty = std::clamp(static_cast<int>(w * BspGeometry::kLightmapSize), 0, BspGeometry::kLightmapSize - 1);
                    const std::vector<std::uint8_t>& lm = g.lightmaps[static_cast<std::size_t>(s.lightmap)];
                    const std::size_t o = (static_cast<std::size_t>(ty) * BspGeometry::kLightmapSize + static_cast<std::size_t>(tx)) * 3U;
                    if (o + 2 >= lm.size()) { continue; }
                    summe += (lm[o] + lm[o + 1] + lm[o + 2]) / 3.0;
                    ++n;
                    gefunden = true;
                }
            }
        }
    }
    return n > 0 ? summe / n : -1.0;
}

void lightmapsMessen(const std::string& bsp, Reihe& aus, const char* was) {
    BspGeometry g;
    std::string f;
    if (!readBspGeometry(dateiInhalt(bsp), g, &f)) {
        meldeFehler(std::string("patchlicht: ") + was + " - .bsp nicht lesbar (" + bsp + "): " + f);
        return;
    }
    for (std::size_t i = 0; i < kN; ++i) { aus[i] = lightmapBoden(g, mitteX(i), 0.0F); }
}

// Die Kamera unter den Patch: 200 vor der Mitte, 40 hoch, Blick auf den Boden unter der Mitte (Bildmitte)
void kameraAufBoden(std::size_t i) {
    ke::Editor& e = ke::editor();
    e.kamera.ursprung = karte::Vektor3f(mitteX(i) - 200.0F, 0.0F, 40.0F);
    e.kamera.winkel = karte::Vektor3f(-static_cast<float>(std::atan2(40.0, 200.0) * 180.0 / 3.14159265358979323846), 0.0F, 0.0F);
}

std::string reiheText(const Reihe& r) {
    std::string t;
    for (std::size_t i = 0; i < kN; ++i) {
        char z[64];
        const double anteil = r[kLeer] > 0 ? r[i] / r[kLeer] : -1.0;
        std::snprintf(z, sizeof(z), "%s%s %.1f (%.2f)", i == 0 ? "" : ", ", kKisten[i].name, r[i], anteil);
        t += z;
    }
    return t;
}

// Je Kiste wie erwartet? an: Schalter an
bool wieQ3map2(const Reihe& r, bool an) {
    if (r[kLeer] <= 0) { return false; }
    bool gut = true;
    for (std::size_t i = 0; i < kN; ++i) {
        if (i == kLeer) { continue; }
        const double anteil = r[i] / r[kLeer];
        const bool schatten = an ? kKisten[i].schattenAn : kKisten[i].schattenAus;
        gut = gut && (schatten ? anteil >= 0.0 && anteil <= kSchatten : anteil >= kDurch);
    }
    return gut;
}

// Bildpunkte aller Kisten in r (je Kiste: Kamera, warten, Mitte messen)
void bilderMessen(std::vector<Schritt>& s, const std::string& was, Reihe* r, double warten) {
    for (std::size_t i = 0; i < kN; ++i) {
        s.push_back(tu("patchlicht: " + was + " Kiste " + kKisten[i].name, [i] { kameraAufBoden(i); }));
        s.push_back(pause(warten));
        s.push_back(tu("patchlicht: " + was + " Kiste " + kKisten[i].name + " messen", [r, i] { (*r)[i] = stufe9c::mittelHelligkeit(); }));
    }
}

std::size_t eintragNummer(const char* name) {
    ke::bauMenueSicherstellen();
    const auto& l = ke::editor().bauProjekt.eintraege;
    for (std::size_t i = 0; i < l.size(); ++i) {
        if (karte::ohneMnemonik(l[i].name) == name) { return i; }
    }
    return l.size();
}

std::string lichtZeile(const std::string& konsole) {
    std::istringstream in(konsole);
    std::string letzte;
    for (std::string z; std::getline(in, z);) {
        if (z.find(" -light ") != std::string::npos && z.find("bhed_q3map2") != std::string::npos) { letzte = z; }
    }
    return letzte;
}

void fotoSichern(const std::string& name) {
    const char* o = std::getenv("BHED_FOTOS");
    if (o == nullptr || o[0] == '\0') { return; }
    stufe9c::Bild b;
    if (!ke::kameraBild(b.rgba, b.breite, b.hoehe)) {
        meldeFehler("patchlicht: Kamerabild nicht lesbar (" + name + ")");
        return;
    }
    stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b);
}

// Ein Durchgang mit dem Schalter an (1) bzw. aus (0)
void durchgang(std::vector<Schritt>& s, const std::shared_ptr<Messung>& m, int an) {
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string sw = an != 0 ? "an" : "aus";
    add(tu("patchlicht: Schalter " + sw + ", Echtzeit-Licht", [an] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = true;
        ke::vorlageEntfernen();
        ke::patchSchattenSetzen(an != 0);
        kameraAufBoden(0);
    }));
    add(warteBis("patchlicht: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    bilderMessen(s, "Echtzeit " + sw, &m->echt[static_cast<std::size_t>(an)], 1.2);
    add(tu("patchlicht: Echtzeit " + sw + " - Ergebnis", [m, an] {
        m->patchDreiecke[static_cast<std::size_t>(an)] = ke::editor().echtLicht.schattenPatchDreiecke;
        diag::detail("Knopftest: patchlicht Echtzeit " + std::string(an != 0 ? "an" : "aus") + ": " + reiheText(m->echt[static_cast<std::size_t>(an)]) +
                     "; Patch-Dreiecke im Schatten " + std::to_string(m->patchDreiecke[static_cast<std::size_t>(an)]));
    }));
    // Schnell-Backen
    add(tu("patchlicht: Schalter " + sw + ", Schnell-Backen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    add(stufe9c::langeWarten("patchlicht: gebacken (" + sw + ")", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("patchlicht: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 60000));
    add(tu("patchlicht: Schnell-Backen " + sw + " - Lightmaps", [m, an] {
        lightmapsMessen(ke::editor().echtLicht.bauOrdner + "\\maps\\schnell.bsp", m->lmSchnell[static_cast<std::size_t>(an)], "Schnell-Backen");
        diag::detail("Knopftest: patchlicht Schnell-Backen " + std::string(an != 0 ? "an" : "aus") + " Lightmap: " +
                     reiheText(m->lmSchnell[static_cast<std::size_t>(an)]));
    }));
    bilderMessen(s, "Schnell-Backen " + sw, &m->gebacken[static_cast<std::size_t>(an)], 0.5);
    add(tu("patchlicht: Schnell-Backen " + sw + ", wie im Spiel", [] { ke::editor().echtLicht.spielbild = true; }));
    add(stufe9c::langeWarten("patchlicht: Spielbild (Schnell-Backen " + sw + ")", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    bilderMessen(s, "Spielbild Schnell-Backen " + sw, &m->spielSchnell[static_cast<std::size_t>(an)], 0.5);
    add(tu("patchlicht: Schnell-Backen " + sw + " - Ergebnis", [m, an] {
        diag::detail("Knopftest: patchlicht Schnell-Backen " + std::string(an != 0 ? "an" : "aus") + " Bild: " + reiheText(m->gebacken[static_cast<std::size_t>(an)]) +
                     "; wie im Spiel: " + reiheText(m->spielSchnell[static_cast<std::size_t>(an)]));
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
    }));
    // voller Bau: [Test] Quick and Crude
    add(tu("patchlicht: Schalter " + sw + ", voller Bau [Test] Quick and Crude", [] {
        ke::konsoleLeeren();
        const std::size_t nr = eintragNummer("[Test] Quick and Crude");
        if (nr >= ke::editor().bauProjekt.eintraege.size()) {
            meldeFehler("patchlicht: Eintrag [Test] Quick and Crude fehlt");
            return;
        }
        ke::bauStarten(nr);
    }));
    add(pause(0.5));
    add(stufe9c::langeWarten("patchlicht: Bau fertig (" + sw + ")", [] { return !ke::bauLaeuft(); }, 60000));
    add(tu("patchlicht: voller Bau " + sw + " - Lightmaps, Hinweis, Vorlage", [m, an] {
        const std::size_t a = static_cast<std::size_t>(an);
        m->hinweis[a] = ke::bauPatchHinweis();
        m->lichtBefehl[a] = lichtZeile(stufe3::konsoleText());
        const std::string bsp = ke::bauVariablen().at("BspFile");
        lightmapsMessen(bsp, m->lmBau[a], "voller Bau");
        diag::detail("Knopftest: patchlicht voller Bau " + std::string(an != 0 ? "an" : "aus") + " Lightmap: " + reiheText(m->lmBau[a]) + "; Licht-Schritt: " +
                     m->lichtBefehl[a] + "; Hinweis: " + m->hinweis[a]);
        // die gebaute .bsp allein durch den Zeichner der Vorschau (wie Teil 11): ohne die Pinsel der Karte, die sonst mit
        // dem Echtzeit-Licht darueber liegen
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        std::string f;
        if (!ke::vorlageLaden(dateiInhalt(bsp), bsp, &f)) { meldeFehler("patchlicht: Vorlage: " + f); }
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("patchlicht: Spielbild (voller Bau " + sw + ")", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    bilderMessen(s, "Spielbild voller Bau " + sw, &m->spielBau[static_cast<std::size_t>(an)], 0.5);
    // Handbuch: der Bogen im gebauten Bild, schraeg von oben (ohne Oberflaeche: ein Bild fuer alle Sprachen)
    add(tu("patchlicht: Foto Bogen (" + sw + ")", [] {
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(mitteX(0) - 230.0F, -230.0F, 300.0F);
        e.kamera.winkel = karte::Vektor3f(-38.0F, 45.0F, 0.0F);
    }));
    add(pause(0.8));
    add(tu("patchlicht: Foto Bogen (" + sw + ") sichern", [an] { fotoSichern(an != 0 ? "h_patchlicht_an" : "h_patchlicht_aus"); }));
    add(tu("patchlicht: voller Bau " + sw + " - Ergebnis", [m, an] {
        diag::detail("Knopftest: patchlicht voller Bau " + std::string(an != 0 ? "an" : "aus") + " wie im Spiel: " + reiheText(m->spielBau[static_cast<std::size_t>(an)]));
        ke::vorlageEntfernen();
        ke::editor().echtLicht.spielbild = false;
        (void)ke::karteOeffnen(kartenPfad());
    }));
}

// Gegenprobe: eine echte Karte mit Patches, rueckgewonnen, zweimal gebaut
void gegenprobe(std::vector<Schritt>& s, const std::shared_ptr<Messung>& m) {
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string bsp = karteTestPfad("gegen_duel4.bsp");
    add(tu("patchlicht: Gegenprobe maps/mp/duel4.bsp rueckgewinnen (mit geschaetzten Lichtern)", [bsp] {
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.karte.undo.alsGespeichertMarkieren();
        std::string daten;
        if (!e.karteSpiel || !e.karteSpiel->fs.dateiLesen("maps/mp/duel4.bsp", daten)) {
            meldeFehler("patchlicht: maps/mp/duel4.bsp nicht in den Spieldaten");
            return;
        }
        std::ofstream(bhed::utf8Pfad(bsp), std::ios::binary).write(daten.data(), static_cast<std::streamsize>(daten.size()));
        e.bspRueck.modelle = false;
        e.bspRueck.lichter = true;
        e.bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        (void)ke::bspOeffnenDatei(bsp);
    }));
    add(stufe9c::langeWarten("patchlicht: Gegenprobe rueckgewonnen", [] { return ke::bspRueckStand().stand == 0 && ke::editor().bspRueckLetzt.sekunden > 0; },
                             120000));
    add(tu("patchlicht: Gegenprobe als .map speichern", [m] {
        (void)ke::karteSpeichernUnter(karteTestPfad("gegen_duel4.map"));
        m->gegenPatches = ke::kartePatchAnzahl();
    }));
    for (int an = 0; an < 2; ++an) {
        const std::string sw = an != 0 ? "an" : "aus";
        add(tu("patchlicht: Gegenprobe Schalter " + sw + ", Echtzeit-Netz, Bau [Test] Quick and Crude", [an] {
            ke::Editor& e = ke::editor();
            ke::patchSchattenSetzen(an != 0);
            e.echtLicht.an = true;
            ke::konsoleLeeren();
            ke::bauStarten(eintragNummer("[Test] Quick and Crude"));
        }));
        add(pause(1.0));
        add(stufe9c::langeWarten("patchlicht: Gegenprobe Bau fertig (" + sw + ")", [] { return !ke::bauLaeuft(); }, 200000));
        add(tu("patchlicht: Gegenprobe " + sw + " - Lightmaps", [m, an] {
            const std::size_t a = static_cast<std::size_t>(an);
            m->gegenDreiecke[a] = ke::editor().echtLicht.schattenPatchDreiecke;
            m->gegenHinweis[a] = ke::bauPatchHinweis();
            m->gegenBefehl[a] = lichtZeile(stufe3::konsoleText());
            BspGeometry g;
            const std::string bsp2 = ke::bauVariablen().at("BspFile");
            if (!readBspGeometry(dateiInhalt(bsp2), g)) {
                meldeFehler("patchlicht: Gegenprobe - .bsp nicht lesbar: " + bsp2);
                return;
            }
            double summe = 0;
            for (const auto& lm : g.lightmaps) {
                for (const std::uint8_t v : lm) { summe += v; }
            }
            m->gegenSumme[a] = summe;
            if (an == 0) {
                ke::editor().echtLicht.an = false;
                std::filesystem::copy_file(bhed::utf8Pfad(bsp2), bhed::utf8Pfad(karteTestPfad("gegen_aus.bsp")), std::filesystem::copy_options::overwrite_existing);
            } else {
                BspGeometry g0;
                if (readBspGeometry(dateiInhalt(karteTestPfad("gegen_aus.bsp")), g0) && g0.lightmaps.size() == g.lightmaps.size()) {
                    for (std::size_t l = 0; l < g.lightmaps.size(); ++l) {
                        for (std::size_t t = 0; t + 2 < g.lightmaps[l].size() && t + 2 < g0.lightmaps[l].size(); t += 3) {
                            const int v0 = g0.lightmaps[l][t] + g0.lightmaps[l][t + 1] + g0.lightmaps[l][t + 2];
                            const int v1 = g.lightmaps[l][t] + g.lightmaps[l][t + 1] + g.lightmaps[l][t + 2];
                            ++m->gegenTexel;
                            if (v1 * 10 < v0 * 9) { ++m->gegenDunkler; }
                        }
                    }
                }
            }
            diag::detail("Knopftest: patchlicht Gegenprobe duel4 " + std::string(an != 0 ? "an" : "aus") + ": Patches " + std::to_string(m->gegenPatches) +
                         ", Lightmap-Summe " + std::to_string(static_cast<long long>(summe)) + ", Patch-Dreiecke im Echtzeit-Schatten " +
                         std::to_string(m->gegenDreiecke[a]) + ", dunkler um > 10 %: " + std::to_string(m->gegenDunkler) + " von " +
                         std::to_string(m->gegenTexel) + "; Licht-Schritt: " + m->gegenBefehl[a] + "; Hinweis: " + m->gegenHinweis[a]);
        }));
    }
    add(pruefe("patchlicht: Gegenprobe duel4 (Patches) - aus: Hinweis, kein -patchshadows, keine Patch-Schatten in Echtzeit; an: -patchshadows im "
               "Licht-Schritt, Patch-Schatten in Echtzeit, die Lightmaps werden dunkler",
               [m] {
                   return m->gegenPatches > 0 && !m->gegenHinweis[0].empty() && m->gegenHinweis[1].empty() &&
                          m->gegenBefehl[0].find("-patchshadows") == std::string::npos && m->gegenBefehl[1].find("-patchshadows") != std::string::npos &&
                          m->gegenDreiecke[0] == 0 && m->gegenDreiecke[1] > 0 && m->gegenSumme[1] < m->gegenSumme[0] && m->gegenDunkler > 0;
               }));
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto m = std::make_shared<Messung>();
    add(tu("patchlicht: Testkarte schreiben und oeffnen", [] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.patchSchatten = false;
        e.bau.gpuBacken = karte::GpuBacken::Aus;   // die Lichtrechnung des Prozessors (die GPU ist Byte-gleich)
        karteSchreiben();
        (void)ke::karteOeffnen(kartenPfad());
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
    }));
    // Schalter in der Karte: Rueckgaengig, gespeicherte Datei
    add(tu("patchlicht: Schalter an (Build > Patch shadows), dann Rueckgaengig", [m] {
        ke::Editor& e = ke::editor();
        ke::patchSchattenSetzen(true);
        m->schluesselAn = e.karte.welt() != nullptr && e.karte.welt()->wert(ke::kPatchSchattenSchluessel) == "1" && ke::patchSchattenAn();
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.2));
    add(tu("patchlicht: nach Rueckgaengig, Speichern", [m] {
        ke::Editor& e = ke::editor();
        m->schluesselUndo = e.karte.welt() != nullptr && e.karte.welt()->wert(ke::kPatchSchattenSchluessel).empty();
        ke::patchSchattenSetzen(true);
        (void)ke::karteSpeichern();
        m->schluesselDatei = dateiInhalt(kartenPfad()).find("\"_bhed_patchshadows\" \"1\"") != std::string::npos;
        diag::detail("Knopftest: patchlicht Schalter in der Karte: gesetzt " + std::to_string(m->schluesselAn) + ", nach Undo weg " +
                     std::to_string(m->schluesselUndo) + ", in der Datei " + std::to_string(m->schluesselDatei));
    }));
    add(pruefe("patchlicht: Patch shadows ist eine Einstellung der Karte (worldspawn _bhed_patchshadows), ein Rueckgaengig-Schritt, gespeichert",
               [m] { return m->schluesselAn && m->schluesselUndo && m->schluesselDatei; }));
    for (int an = 0; an < 2; ++an) { durchgang(s, m, an); }
    add(pruefe("patchlicht: Echtzeit-Licht aus - Patches ohne Schatten, der Brush mit (q3map2 ohne -patchshadows)", [m] { return wieQ3map2(m->echt[0], false); }));
    add(pruefe("patchlicht: Echtzeit-Licht an - Patches in Welt und func_group mit Schatten, func_group mit _cs 0 ohne (GetEntityShadowFlags)",
               [m] { return wieQ3map2(m->echt[1], true); }));
    add(pruefe("patchlicht: Echtzeit-Licht - Schattennetz der Patches wie TessellatedMesh von -light (an 34 Dreiecke, aus 0)", [m] {
        return m->patchDreiecke[0] == 0 && m->patchDreiecke[1] == kPatchDreieckeAn;
    }));
    add(pruefe("patchlicht: Schnell-Backen aus/an - Lightmaps wie q3map2", [m] { return wieQ3map2(m->lmSchnell[0], false) && wieQ3map2(m->lmSchnell[1], true); }));
    add(pruefe("patchlicht: Schnell-Backen aus/an - Bild wie q3map2", [m] { return wieQ3map2(m->gebacken[0], false) && wieQ3map2(m->gebacken[1], true); }));
    add(pruefe("patchlicht: Gebaute Karte wie im Spiel (Schnell-Backen) aus/an", [m] {
        return wieQ3map2(m->spielSchnell[0], false) && wieQ3map2(m->spielSchnell[1], true);
    }));
    add(pruefe("patchlicht: voller Bau aus - Patches ohne Schatten wie q3map2, Licht-Schritt ohne -patchshadows, Hinweis im Bau", [m] {
        return wieQ3map2(m->lmBau[0], false) && m->lichtBefehl[0].find("-patchshadows") == std::string::npos && !m->hinweis[0].empty();
    }));
    add(pruefe("patchlicht: voller Bau an - der Licht-Schritt bekommt -patchshadows, Patches mit Schatten, kein Hinweis", [m] {
        return wieQ3map2(m->lmBau[1], true) && m->lichtBefehl[1].find("-patchshadows") != std::string::npos && m->hinweis[1].empty();
    }));
    add(pruefe("patchlicht: Gebaute Karte wie im Spiel (voller Bau) aus/an", [m] { return wieQ3map2(m->spielBau[0], false) && wieQ3map2(m->spielBau[1], true); }));
    const char* gegen = std::getenv("BHED_PATCHLICHT_GEGEN");
    if (gegen == nullptr || std::strcmp(gegen, "0") != 0) { gegenprobe(s, m); }
    add(tu("patchlicht: aufraeumen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.patchSchatten = false;
        e.echtLicht.pauseSekunden = 2.0;
        e.bau.gpuBacken = karte::kGpuBackenVorgabe;
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

}  // namespace patchlicht
