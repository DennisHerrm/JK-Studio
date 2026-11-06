// selbsttest_r3.inc - rc610 R3 (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=r3): Tester-Bugs vom 06.11.
//
//   Spiegeln: "some models randomly renders inside (... when i flip the model vertically or horizontal)". Ein Wuerfel als .md3
//         (Vorderseite rot, alle anderen Seiten blau) als misc_model ungespiegelt, mit Flip X, mit Flip Z und mit Flip X + Z
//         (zweifach, Determinante wieder positiv), dazu misc_model_static ungespiegelt und mit Flip X. Je Ansicht (Editor-Licht,
//         Echtzeit-Licht, "Built map as in the game") der Anteil Rot (Vorderseite) im Bild jedes Wuerfels. q3map2 dreht die
//         Wicklung eingebackener Modelle bei negativer Determinante um (model.cpp InsertModel), das Spiel zeichnet Laufzeit-Modelle
//         ohne Umkehr (OpenJK tr_backend.cpp GL_Cull: nur viewParms.isMirror) - misc_model_static zeigt gespiegelt sein Inneres,
//         die Kartenpruefung meldet es.
//   Glow: der Shader des Testers ({ $lightmap } { Textur GL_DST_COLOR GL_ZERO } { _glw GL_ONE GL_ONE glow }) auf einer Platte;
//         Echtzeit-Licht gegen "Built map as in the game" an derselben Kamera (Gelb der Platte, Schein daneben).
//         BHED_R3_ZEIT=1: nur die Bildzeit des Echtzeit-Lichts (240 Bilder mit drehender Kamera, allein laufen lassen).
// Eingebunden in gui/selbsttest.cpp nach selbsttest_editorportal.inc (tga32), aufgerufen aus selbsttest_handbuch.inc.
namespace r3 {

namespace ke = karteneditor;

constexpr const char* kWand = "textures/r3/wand";
constexpr const char* kDunkel = "textures/r3/dunkel";
constexpr const char* kLicht = "textures/r3/licht";

std::string quelle() { return stufe2::laufDatei("r3_quelle"); }
std::string spiegelKarte() { return karteTestPfad("r3_spiegel.map"); }
std::string glowKarte() { return karteTestPfad("r3_glow.map"); }

std::string& wuerfelPfad() {
    static std::string p;
    return p;
}

// Der Wuerfel (64 Einheiten, Mitte im Ursprung) als .md3 wie ein Import (md3Paket): je Seite vier Punkte gegen den Uhrzeiger von
// aussen; die Vorderseite (-y) nimmt die linke Haelfte der Textur (rot), alle anderen die rechte (blau).
karte::Md3Paket wuerfel() {
    karte::ImportNetz netz;
    karte::ImportTeil t;
    t.name = "wuerfel";
    t.material = "wuerfel";
    t.textur = "wuerfel.tga";
    t.texturEndung = "tga";
    t.texturDaten = editorportal::tga32(64, 64, [](int x, int) {
        return x < 32 ? std::array<unsigned char, 4>{220, 30, 30, 255} : std::array<unsigned char, 4>{30, 30, 220, 255};
    });
    t.hatUv = true;
    struct Seite {
        double n[3];
        double u[3];
        double v[3];
    };
    const Seite seiten[6] = {{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},  {{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}}, {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
                             {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}}, {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},  {{0, 0, -1}, {1, 0, 0}, {0, -1, 0}}};
    for (std::size_t i = 0; i < 6; ++i) {
        const Seite& s = seiten[i];
        const double s0 = i == 0 ? 0.1 : 0.6;
        const auto basis = static_cast<std::uint32_t>(t.punkte.size());
        const double ecken[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        for (const auto& e : ecken) {
            karte::ImportPunkt p;
            p.x = 32.0 * (s.n[0] + e[0] * s.u[0] + e[1] * s.v[0]);
            p.y = 32.0 * (s.n[1] + e[0] * s.u[1] + e[1] * s.v[1]);
            p.z = 32.0 * (s.n[2] + e[0] * s.u[2] + e[1] * s.v[2]);
            t.punkte.push_back(p);
            t.uv.push_back({s0 + (e[0] + 1.0) * 0.15, 0.1 + (1.0 - e[1]) * 0.4});
        }
        t.dreiecke.push_back({basis, basis + 1, basis + 2});
        t.dreiecke.push_back({basis, basis + 2, basis + 3});
    }
    netz.teile.push_back(std::move(t));
    return karte::md3Paket(netz, "r3wuerfel");
}

// Alles als .pk3 im Ordner des Laufs: der Editor liest ihn als Extra Resource Path, das Spielbild als Spielordner dieser Sitzung
void pk3Schreiben() {
    std::vector<Pk3Datei> pk3;
    const karte::Md3Paket w = wuerfel();
    if (!w.fehler.empty()) { meldeFehler("R3: Wuerfel nicht gebaut: " + w.fehler); }
    wuerfelPfad() = w.modellPfad;
    for (const karte::Md3Paket::Datei& d : w.dateien) { pk3.push_back({d.pfad, d.inhalt}); }
    // der Shader des Testers in derselben Form (textures/md_concept/light gibt es in den Daten nicht)
    pk3.push_back({"shaders/r3.shader", std::string(kLicht) +
                                            "\n{\n\tqer_editorimage textures/r3/licht\n\t{\n\t\tmap $lightmap\n\t\trgbGen identity\n\t}\n"
                                            "\t{\n\t\tmap textures/r3/licht\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\trgbGen identity\n\t}\n"
                                            "\t{\n\t\tmap textures/r3/licht_glw\n\t\tblendFunc GL_ONE GL_ONE\n\t\trgbGen identity\n\t\tglow\n\t}\n}\n"});
    pk3.push_back({"textures/r3/licht.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{40, 40, 40, 255}; })});
    pk3.push_back({"textures/r3/licht_glw.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{255, 210, 40, 255}; })});
    pk3.push_back({"textures/r3/wand.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{150, 150, 150, 255}; })});
    pk3.push_back({"textures/r3/dunkel.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{12, 12, 12, 255}; })});
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(quelle()), ec);
    std::string err;
    if (!writePk3(quelle() + "\\r3.pk3", pk3, &err)) { meldeFehler("R3: .pk3 nicht geschrieben: " + err); }
}

void quader(karte::Karte& k, const std::shared_ptr<karte::Entitaet>& welt, const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    auto p = std::make_shared<karte::Pinsel>();
    p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
    welt->kindEinfuegen(p);
}

void raum(karte::Karte& k, const std::shared_ptr<karte::Entitaet>& welt, const char* shader) {
    quader(k, welt, karte::Vektor3f(0, 250, -8), karte::Vektor3f(544, 666, 8), shader);
    quader(k, welt, karte::Vektor3f(0, 250, 408), karte::Vektor3f(544, 666, 8), shader);
    quader(k, welt, karte::Vektor3f(-536, 250, 200), karte::Vektor3f(8, 650, 200), shader);
    quader(k, welt, karte::Vektor3f(536, 250, 200), karte::Vektor3f(8, 650, 200), shader);
    quader(k, welt, karte::Vektor3f(0, -408, 200), karte::Vektor3f(528, 8, 200), shader);
    quader(k, welt, karte::Vektor3f(0, 908, 200), karte::Vektor3f(528, 8, 200), shader);
}

std::shared_ptr<karte::Entitaet> ent(karte::Karte& k, const char* klasse, const std::vector<std::pair<const char*, std::string>>& werte) {
    auto e = k.neueEntitaet(klasse, false);
    for (const auto& [a, b] : werte) { e->setzen(a, b); }
    k.entitaetEinfuegen(e);
    return e;
}

// Die sechs Wuerfel: Reihe 1 (z 96) misc_model A ohne, B Flip X, C Flip Z, D Flip X + Z; Reihe 2 (z 256) misc_model_static E ohne,
// F Flip X. Gespiegelt wird erst im Editor mit dem Werkzeug (welche Schluessel es schreibt, zeigt das Protokoll). Alle Urspruenge auf
// dem Raster 32: der Drehpunkt des Werkzeugs rastet ein (sonst wandert der Wuerfel um den Rest).
struct Wuerfel {
    const char* name;
    const char* klasse;
    float x;
    float z;
    bool rotErwartet;   // wie im Spiel: Vorderseite sichtbar
};
constexpr Wuerfel kWuerfel[6] = {{"A", "misc_model", -320, 96, true},       {"B", "misc_model", -96, 96, true},
                                 {"C", "misc_model", 96, 96, true},         {"D", "misc_model", 320, 96, true},
                                 {"E", "misc_model_static", -160, 256, true}, {"F", "misc_model_static", 160, 256, false}};
constexpr float kReiheY = 300;

void spiegelKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    raum(k, welt, kWand);
    welt->setzen("_ambient", "80");
    ent(k, "light", {{"origin", "0 0 300"}, {"light", "1500"}});
    ent(k, "light", {{"origin", "0 600 300"}, {"light", "1500"}});
    ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    for (const Wuerfel& w : kWuerfel) {
        char o[64];
        std::snprintf(o, sizeof(o), "%g %g %g", static_cast<double>(w.x), static_cast<double>(kReiheY), static_cast<double>(w.z));
        ent(k, w.klasse, {{"origin", o}, {"model", wuerfelPfad()}, {"targetname", std::string("r3_") + w.name}});
    }
    (void)karte::karteSpeichern(spiegelKarte(), k, false);
}

std::shared_ptr<karte::Entitaet> findeWuerfel(const char* name) {
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->wert("targetname") == std::string("r3_") + name) { return e; }
    }
    return nullptr;
}

void spiegeln(const char* name, int achse) {
    ke::Editor& ed = ke::editor();
    const auto e = findeWuerfel(name);
    if (!e) {
        meldeFehler(std::string("R3 Spiegeln: Wuerfel ") + name + " fehlt");
        return;
    }
    ed.karte.auswahl.leeren();
    ed.karte.auswahl.waehlen(e);
    karte::auswahlSpiegeln(ed.bearbeitung, ed.karte, achse);
    ed.karte.auswahl.leeren();
}

void bmp(const std::string& name, const stufe9c::Bild& b) {
    const char* o = std::getenv("BHED_FOTOS");
    if (o != nullptr && o[0] != '\0') { stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b); }
}

// Bildpunkt eines Weltpunkts (Kamera des letzten Bilds); false hinter dem Auge
bool imBild(const karte::Vektor3f& p, int breite, int hoehe, float& sx, float& sy) {
    const karte::Matrix4 m = ke::editor().kamera.sicht();
    const float* vp = &m[0];
    const float x = vp[0] * p[0] + vp[4] * p[1] + vp[8] * p[2] + vp[12];
    const float y = vp[1] * p[0] + vp[5] * p[1] + vp[9] * p[2] + vp[13];
    const float w = vp[3] * p[0] + vp[7] * p[1] + vp[11] * p[2] + vp[15];
    if (w <= 1.0F) { return false; }
    sx = (x / w * 0.5F + 0.5F) * static_cast<float>(breite);
    sy = (0.5F - y / w * 0.5F) * static_cast<float>(hoehe);
    return true;
}

// Rechteck im Bild um einen Kasten (Mitte, halbe Groesse); innen = um den Anteil rand verkleinert
struct Rechteck {
    int x0 = 0, y0 = 0, x1 = -1, y1 = -1;
};
Rechteck rechteck(const karte::Vektor3f& mitte, const karte::Vektor3f& halb, int breite, int hoehe, float rand) {
    float mnx = 1e30F, mny = 1e30F, mxx = -1e30F, mxy = -1e30F;
    for (int i = 0; i < 8; ++i) {
        const karte::Vektor3f p(mitte[0] + ((i & 1) != 0 ? halb[0] : -halb[0]), mitte[1] + ((i & 2) != 0 ? halb[1] : -halb[1]),
                                mitte[2] + ((i & 4) != 0 ? halb[2] : -halb[2]));
        float sx = 0, sy = 0;
        if (!imBild(p, breite, hoehe, sx, sy)) { return {}; }
        mnx = std::min(mnx, sx);
        mny = std::min(mny, sy);
        mxx = std::max(mxx, sx);
        mxy = std::max(mxy, sy);
    }
    const float dx = (mxx - mnx) * rand;
    const float dy = (mxy - mny) * rand;
    Rechteck r;
    r.x0 = std::clamp(static_cast<int>(mnx + dx), 0, breite - 1);
    r.y0 = std::clamp(static_cast<int>(mny + dy), 0, hoehe - 1);
    r.x1 = std::clamp(static_cast<int>(mxx - dx), 0, breite - 1);
    r.y1 = std::clamp(static_cast<int>(mxy - dy), 0, hoehe - 1);
    return r;
}

struct Anteil {
    long rot = 0;
    long blau = 0;
    [[nodiscard]] double wert() const { return rot + blau > 0 ? static_cast<double>(rot) / static_cast<double>(rot + blau) : -1.0; }
};

Anteil rotAnteil(const stufe9c::Bild& b, const Wuerfel& w) {
    Anteil a;
    // das Rechteck der Vorderseite (y - 32), innen 20 % weg (Raender): von vorn rot; wird sie weggeschnitten, sieht man durch sie
    // die Innenseiten der anderen (blau)
    const Rechteck r = rechteck(karte::Vektor3f(w.x, kReiheY - 32.0F, w.z), karte::Vektor3f(32, 0, 32), b.breite, b.hoehe, 0.2F);
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
            const int R = b.rgba[i], G = b.rgba[i + 1], B = b.rgba[i + 2];
            if (R > G + 30 && R > B + 30) { ++a.rot; }
            if (B > G + 30 && B > R + 30) { ++a.blau; }
        }
    }
    return a;
}

using Messung = std::array<Anteil, 6>;

Schritt messen(const std::string& ansicht, std::shared_ptr<Messung> m) {
    return tu("R3 Spiegeln: " + ansicht + " messen", [ansicht, m] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("R3 Spiegeln: Kamerabild " + ansicht + " nicht lesbar");
            return;
        }
        bmp("r3_spiegel_" + ansicht, b);
        std::string z = "Knopftest: R3 Spiegeln " + ansicht + " - Anteil Rot (Vorderseite):";
        for (std::size_t i = 0; i < 6; ++i) {
            (*m)[i] = rotAnteil(b, kWuerfel[i]);
            char t[96];
            std::snprintf(t, sizeof(t), " %s %s %.3f (%ld)", kWuerfel[i].name, kWuerfel[i].klasse, (*m)[i].wert(), (*m)[i].rot + (*m)[i].blau);
            z += t;
        }
        diag::detail(z);
    });
}

Schritt pruefeAnsicht(const std::string& ansicht, std::shared_ptr<Messung> m) {
    return pruefe("R3 Spiegeln: " + ansicht + " - misc_model mit jeder Spiegelung von vorn (rot, wie q3map2), misc_model_static gespiegelt innen (wie im Spiel)",
                  [ansicht, m] {
                      bool gut = true;
                      for (std::size_t i = 0; i < 6; ++i) {
                          const double w = (*m)[i].wert();
                          const long n = (*m)[i].rot + (*m)[i].blau;
                          const bool ok = n > 200 && (kWuerfel[i].rotErwartet ? w > 0.8 : w < 0.2);
                          if (!ok) { diag::detail(std::string("Knopftest: R3 Spiegeln ") + ansicht + " - " + kWuerfel[i].name + " falsch"); }
                          gut = gut && ok;
                      }
                      return gut;
                  });
}

std::vector<Schritt> spiegelSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto editor = std::make_shared<Messung>();
    auto echt = std::make_shared<Messung>();
    auto spiel = std::make_shared<Messung>();
    auto spielEditor = std::make_shared<Messung>();
    add(tu("R3: Testdaten (.md3, Shader, Bilder) als Extra Resource Path", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        pk3Schreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("R3: Spieldaten mit dem Extra Resource Path", [] {
        ke::spielPruefen();
        const ke::Editor& e = ke::editor();
        return ke::spielBereit() && e.karteSpiel && e.karteSpiel->inventar.shaderFinden(kLicht) != nullptr;
    }, 20000));
    add(tu("R3 Spiegeln: Karte mit sechs Wuerfeln, spiegeln mit dem Werkzeug (Selection_Flipx/z)", [] {
        spiegelKarteSchreiben();
        (void)ke::karteOeffnen(spiegelKarte());
        spiegeln("B", 0);
        spiegeln("C", 2);
        spiegeln("D", 0);
        spiegeln("D", 2);
        spiegeln("F", 0);
        std::string z = "Knopftest: R3 Spiegeln - Schluessel nach dem Werkzeug:";
        for (const Wuerfel& w : kWuerfel) {
            const auto e = findeWuerfel(w.name);
            if (!e) { continue; }
            z += std::string(" ") + w.name + " origin \"" + e->wert("origin") + "\" angles \"" + e->wert("angles") + "\" angle \"" + e->wert("angle") +
                 "\" modelscale_vec \"" + e->wert("modelscale_vec") + "\" modelscale \"" + e->wert("modelscale") + "\";";
        }
        diag::detail(z);
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS;
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.kamera.ursprung = karte::Vektor3f(0.0F, -150.0F, 176.0F);
        e.kamera.winkel = karte::Vektor3f(0.0F, 90.0F, 0.0F);
    }));
    add(pruefe("R3 Spiegeln: das Werkzeug schreibt modelscale_vec mit negativen Achsen (B -1 1 1, C 1 1 -1, D -1 1 -1)", [] {
        const auto b = findeWuerfel("B");
        const auto c = findeWuerfel("C");
        const auto d = findeWuerfel("D");
        return b && c && d && b->wert("modelscale_vec") == "-1 1 1" && c->wert("modelscale_vec") == "1 1 -1" && d->wert("modelscale_vec") == "-1 1 -1";
    }));
    add(warteBis("R3 Spiegeln: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(messen("editor", editor));
    add(pruefeAnsicht("Editor-Licht", editor));
    add(tu("R3 Spiegeln: Echtzeit-Licht", [] { ke::editor().echtLicht.an = true; }));
    add(warteBis("R3 Spiegeln: Texturen (Echtzeit)", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("R3 Spiegeln: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(messen("echtzeit", echt));
    add(pruefeAnsicht("Echtzeit-Licht", echt));
    // Kartenpruefung: das gespiegelte Laufzeit-Modell
    auto laeufe = std::make_shared<int>(0);
    add(tu("R3 Spiegeln: Kartenpruefung (Optimierer)", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("R3 Spiegeln: Optimierer fertig", laeufe));
    add(pruefe("R3 Spiegeln: Kartenpruefung \"modellgespiegelt\" genau fuer F (misc_model_static mit Flip X), nicht fuer die misc_model", [] {
        long long n = 0;
        const bool da = stufe5::hatBefund("modellgespiegelt", &n);
        diag::detail("Knopftest: R3 Spiegeln - Befunde " + stufe5::befundListe());
        return da && n == 1;
    }));
    add(tu("R3 Spiegeln: Optimierer zu", [] { ke::optimiererSchliessen(); }));
    // dieselbe Kamera im Spielbild der gebauten Karte (q3map2 backt die misc_model ein, das Spiel laedt die misc_model_static)
    add(tu("R3 Spiegeln: Schnell-Backen, Spielbild (die .pk3 als Spielordner dieser Sitzung)", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        ordner.push_back(quelle());
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("R3 Spiegeln: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("R3 Spiegeln: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("R3 Spiegeln: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    // q3map2 muss das Modell aus dem Extra Resource Path finden, sonst backt es die misc_model nicht ein (AddTriangleModels, "Failed
    // to load model") - der Shader der Wuerfelflaechen steht dann nicht in der .bsp
    add(pruefe("R3 Spiegeln: das Schnell-Backen findet die Modelle des Extra Resource Path (die misc_model sind eingebacken)", [] {
        const std::string bsp = ke::schnellBspFuer(ke::geometrieStand());
        std::ifstream f(::bhed::utf8Pfad(bsp), std::ios::binary);
        std::ostringstream t;
        t << f.rdbuf();
        const bool drin = t.str().find("models/import/r3wuerfel/wuerfel") != std::string::npos;
        const ke::SpielbildStand st = ke::spielbildStand();
        diag::detail("Knopftest: R3 Spiegeln - .bsp \"" + bsp + "\" " + std::to_string(t.str().size()) + " Bytes, Wuerfelshader " + (drin ? "drin" : "fehlt") +
                     ", Kartenmodelle im Spielbild " + std::to_string(st.modelle) + " (" + std::to_string(st.modellAufrufe) + " Aufrufe)");
        return drin;
    }));
    add(pause(1.5));
    add(messen("spielbild_editor", spielEditor));
    add(pruefeAnsicht("Spielbild mit den Modellen des Editors darueber", spielEditor));
    // nur das Spielbild: die Modelle des Editors (die die Kamera ueber das Spielbild legt) per Filter aus
    add(tu("R3 Spiegeln: Filter Models (nur das Spielbild)", [] { ke::editor().filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_MODELS; }));
    add(pause(1.0));
    // der Filter aendert den Stand der Karte: das Schnell-Backen laeuft noch einmal (dieselbe Karte)
    add(stufe9c::langeWarten("R3 Spiegeln: Schnell-Backen nach dem Filter", [] {
        const ke::Editor& e = ke::editor();
        return e.echtLicht.bauStand == 5 || (e.echtLicht.bauStand != 1 && e.echtLicht.bauStand != 2 && e.echtLicht.gebackenZeigen && e.echtLicht.spielbildGezeigt);
    }, 400000));
    add(pause(1.5));
    add(tu("R3 Spiegeln: Stand des Spielbilds", [] {
        const ke::SpielbildStand st = ke::spielbildStand();
        diag::detail("Knopftest: R3 Spiegeln - Spielbild Nummer " + std::to_string(st.nummer) + ", Kartenmodelle " + std::to_string(st.modelle) + ", Bau " +
                     std::to_string(ke::editor().echtLicht.bauStand) + ", gezeigt " + (ke::editor().echtLicht.spielbildGezeigt ? "ja" : "nein"));
    }));
    add(messen("spielbild", spiel));
    add(pruefeAnsicht("Spielbild", spiel));
    add(tu("R3 Spiegeln: aufraeumen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// --- Glow ----------------------------------------------------------------------------------------

constexpr float kPlatteY = 300;   // Vorderseite der Platte (y 300 .. 316, x -96 .. 96, z 64 .. 256)

void glowKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    raum(k, welt, kDunkel);
    quader(k, welt, karte::Vektor3f(0, kPlatteY + 8, 160), karte::Vektor3f(96, 8, 96), kLicht);
    welt->setzen("_ambient", "40");
    ent(k, "light", {{"origin", "0 100 300"}, {"light", "600"}});
    ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    (void)karte::karteSpeichern(glowKarte(), k, false);
}

struct Glow {
    double gelb = 0;   // Mitte der Platte: (R + G) / 2 - B
    double schein = 0; // Ring 6 .. 40 Bildpunkte neben der Platte (links, rechts, oben): (R + G) / 2
};

Glow glowMessen(const stufe9c::Bild& b) {
    Glow g;
    const Rechteck r = rechteck(karte::Vektor3f(0, kPlatteY, 160), karte::Vektor3f(96, 0, 96), b.breite, b.hoehe, 0.0F);
    const auto px = [&b](int x, int y) {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
        return std::array<int, 3>{b.rgba[i], b.rgba[i + 1], b.rgba[i + 2]};
    };
    long n = 0;
    const int w = r.x1 - r.x0;
    const int h = r.y1 - r.y0;
    for (int y = r.y0 + h / 4; y <= r.y1 - h / 4; ++y) {
        for (int x = r.x0 + w / 4; x <= r.x1 - w / 4; ++x) {
            const auto c = px(x, y);
            g.gelb += (c[0] + c[1]) / 2.0 - c[2];
            ++n;
        }
    }
    if (n > 0) { g.gelb /= static_cast<double>(n); }
    n = 0;
    for (int y = r.y0 - 40; y <= r.y1; ++y) {
        for (int x = r.x0 - 40; x <= r.x1 + 40; ++x) {
            if (x < 0 || y < 0 || x >= b.breite || y >= b.hoehe) { continue; }
            const int dx = x < r.x0 ? r.x0 - x : x > r.x1 ? x - r.x1 : 0;
            const int dy = y < r.y0 ? r.y0 - y : 0;
            const int d = std::max(dx, dy);
            if (d < 6 || d > 40) { continue; }
            const auto c = px(x, y);
            g.schein += (c[0] + c[1]) / 2.0;
            ++n;
        }
    }
    if (n > 0) { g.schein /= static_cast<double>(n); }
    return g;
}

Schritt glowBild(const std::string& name, std::shared_ptr<Glow> g) {
    return tu("R3 Glow: " + name + " messen", [name, g] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("R3 Glow: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("r3_glow_" + name, b);
        *g = glowMessen(b);
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: R3 Glow %s - Gelb der Platte %.1f, Schein daneben %.1f", name.c_str(), g->gelb, g->schein);
        diag::detail(z);
    });
}

std::vector<Schritt> glowSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto editor = std::make_shared<Glow>();
    auto echt = std::make_shared<Glow>();
    auto spiel = std::make_shared<Glow>();
    add(tu("R3 Glow: Platte mit dem Shader des Testers, Editor-Licht", [] {
        glowKarteSchreiben();
        (void)ke::karteOeffnen(glowKarte());
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        ke::spielbildZeitFest(0.0);
        e.kamera.ursprung = karte::Vektor3f(0.0F, -60.0F, 160.0F);
        e.kamera.winkel = karte::Vektor3f(0.0F, 90.0F, 0.0F);
    }));
    add(warteBis("R3 Glow: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(glowBild("editor", editor));
    add(pruefe("R3 Glow: Editor-Licht wie NRC - nur qer_editorimage, kein Glow", [editor] { return editor->gelb < 20.0; }));
    add(tu("R3 Glow: Echtzeit-Licht", [] { ke::editor().echtLicht.an = true; }));
    add(warteBis("R3 Glow: Texturen (Echtzeit)", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("R3 Glow: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(glowBild("echtzeit", echt));
    add(pruefe("R3 Glow: Echtzeit-Licht zeichnet die Glow-Stufe (GL_ONE GL_ONE) und den Schein (RB_BlurGlowTexture)", [echt] {
        diag::detail("Knopftest: R3 Glow Echtzeit - Glow-Aufrufe " + std::to_string(ke::editor().echtLicht.gluehAufrufe) + ", weitere Stufen " +
                     std::to_string(ke::editor().echtLicht.weitereStufen));
        return echt->gelb > 60.0 && ke::editor().echtLicht.gluehAufrufe > 0;
    }));
    add(tu("R3 Glow: Schnell-Backen, Spielbild", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        if (ordner.empty() || ordner.back() != quelle()) {
            ordner.push_back(quelle());
            setzeSitzungsSpielordner(std::move(ordner));
            rescanGamePaths();
        }
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("R3 Glow: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("R3 Glow: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("R3 Glow: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.5));
    add(glowBild("spielbild", spiel));
    add(pruefe("R3 Glow: Echtzeit-Licht gegen Spielbild - Gelb der Platte und Schein daneben gleich bis auf 15 % bzw. 25 %", [echt, spiel] {
        const double gelb = spiel->gelb > 0 ? echt->gelb / spiel->gelb : 0.0;
        const double schein = spiel->schein > 0 ? echt->schein / spiel->schein : 0.0;
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: R3 Glow Vergleich - Gelb %.3f, Schein %.3f (Echtzeit / Spielbild)", gelb, schein);
        diag::detail(z);
        return gelb > 0.85 && gelb < 1.18 && schein > 0.8 && schein < 1.25;
    }));
    add(tu("R3 Glow: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
        std::vector<std::string> ordner = aktiveSpielordner();
        if (!ordner.empty() && ordner.back() == quelle()) { ordner.pop_back(); }
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0].clear(); });
    }));
    add(warteBis("R3: Spieldaten ohne den Extra Resource Path", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    return s;
}

// Bildzeit allein: die Glow-Karte im Echtzeit-Licht, 240 Bilder mit drehender Kamera
std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("R3 Zeit: Testdaten", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        pk3Schreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("R3 Zeit: Spieldaten", [] {
        ke::spielPruefen();
        return ke::spielBereit() && ke::editor().karteSpiel && ke::editor().karteSpiel->inventar.shaderFinden(kLicht) != nullptr;
    }, 20000));
    add(tu("R3 Zeit: Glow-Karte, Echtzeit-Licht", [] {
        glowKarteSchreiben();
        (void)ke::karteOeffnen(glowKarte());
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.kamera.ursprung = karte::Vektor3f(0.0F, -60.0F, 160.0F);
        e.kamera.winkel = karte::Vektor3f(0.0F, 90.0F, 0.0F);
    }));
    add(warteBis("R3 Zeit: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("R3 Zeit: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    auto bilder = std::make_shared<std::vector<double>>();
    add({"R3 Zeit: 240 Bilder", [bilder](int b) {
             ke::Editor& e = ke::editor();
             if (b > 2) { bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0); }
             e.kamera.ursprung = karte::Vektor3f(0.0F, -60.0F, 160.0F);
             e.kamera.winkel = karte::Vektor3f(0.0F, 90.0F + static_cast<float>(b % 40) - 20.0F, 0.0F);
             return b >= 242;
         }});
    add(tu("R3 Zeit: Ergebnis", [bilder] {
        std::vector<double> v = *bilder;
        std::sort(v.begin(), v.end());
        double summe = 0;
        for (const double t : v) { summe += t; }
        char z[300];
        std::snprintf(z, sizeof(z), "R3-Glow-Bildzeit: Mittel %.2f ms, p99 %.2f ms (%zu Bilder)", v.empty() ? 0.0 : summe / static_cast<double>(v.size()),
                      v.empty() ? 0.0 : v[std::min(v.size() - 1, v.size() * 99 / 100)], v.size());
        diag::detail(z);
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.filterMaske = 0;
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0].clear(); });
    }));
    return s;
}

// Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=r3): die Glow-Platte im Echtzeit-Licht, schraeg von vorn
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    s.push_back(tu("Handbuch R3: Testdaten als Extra Resource Path", [] {
        karteDialogeZu();
        ke::setzeNrcBegriffe(false);
        ke::editor().karte.undo.alsGespeichertMarkieren();
        pk3Schreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    s.push_back(warteBis("Handbuch R3: Spieldaten", [] {
        ke::spielPruefen();
        return ke::spielBereit() && ke::editor().karteSpiel && ke::editor().karteSpiel->inventar.shaderFinden(kLicht) != nullptr;
    }, 20000));
    s.push_back(tu("Handbuch R3: Glow-Platte, Echtzeit-Licht", [] {
        glowKarteSchreiben();
        (void)ke::karteOeffnen(glowKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        if (const auto welt = e.karte.welt()) { welt->setzen("_ambient", "160"); }   // fuers Bild heller als der Test
        e.karte.undo.alsGespeichertMarkieren();
        e.kamera.ursprung = karte::Vektor3f(-260.0F, -120.0F, 200.0F);
        e.kamera.winkel = karte::Vektor3f(-8.0F, 50.0F, 0.0F);
        if (ImGuiWindow* w = karteFenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.6F, w->Pos.y + w->Size.y - 4.0F}); }   // kein Tooltip im Bild
    }));
    s.push_back(warteBis("Handbuch R3: Texturen", [] { return ke::kameraBilderFertig(); }, 6000));
    s.push_back(pause(1.5));
    s.push_back(hb::fotoTeil("h_r3_glow", [] { return hb::fensterRect("##feldKamera"); }));
    s.push_back(tu("Handbuch R3: Ende", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0].clear(); });
        g_app->leftMode = 0;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    if (const char* z = std::getenv("BHED_R3_ZEIT"); z != nullptr && z[0] == '1') { return zeitSchritte(); }
    std::vector<Schritt> s = spiegelSchritte();
    for (Schritt& x : glowSchritte()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace r3
