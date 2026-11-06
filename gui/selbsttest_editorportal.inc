// selbsttest_editorportal.inc - rc609 P (BHED_EDITORTEST=editorportal): Spiegel und Portalkameras im Karten-Editor.
//
//   1. Vorschau wie die Engine: duel_executor (Spiegelboden) und duel_wbw (misc_portal_camera mit target, Richtung ueber
//      DirToByte): die Sicht der Vorschau ist die von bhed/portalsicht (R_MirrorViewBySurface, portalPlane genau auf der
//      Ebene, pvsOrigin = oldorigin).
//   2. Editor gegen Vorschau: dieselbe .bsp als Vorlage im Spielbild ("Built map as in the game"), dieselbe Kamera, dieselbe
//      Bildgroesse und Shaderzeit - die Bildpunkte wie in der Vorschau. Gegenprobe duel_calodan.
//   3. Eigene Testkarte mit Spiegelboden (textures/system/mirror_yavin, alphaGen portal 512) und Bildschirm (textures/system/portal,
//      alphaGen portal 256) einer misc_portal_camera: Echtzeit-Licht live (misc_portal_surface verschieben, Kamera ausser
//      Reichweite), danach Schnell-Backen und Spielbild mit derselben Sicht.
namespace editorportal {
namespace ke = karteneditor;

struct Bild {
    std::vector<std::uint8_t> rgba;
    int w = 0;
    int h = 0;
    Camera cam;
    float mitte[3]{};   // Mitte der Portalflaeche
};

// Mittlere Abweichung je Kanal (0..255) in einem Fenster um (x, y); -1: Bilder passen nicht
double abweichung(const Bild& a, const Bild& b, int x, int y, int halb) {
    if (a.w != b.w || a.h != b.h || a.rgba.size() != b.rgba.size() || a.w <= 0) { return -1.0; }
    double s = 0.0;
    long n = 0;
    for (int yy = std::max(0, y - halb); yy < std::min(a.h, y + halb); ++yy) {
        for (int xx = std::max(0, x - halb); xx < std::min(a.w, x + halb); ++xx) {
            const std::size_t i = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(a.w) + static_cast<std::size_t>(xx)) * 4U;
            for (std::size_t c = 0; c < 3; ++c) { s += std::fabs(static_cast<double>(a.rgba[i + c]) - static_cast<double>(b.rgba[i + c])); }
            n += 3;
        }
    }
    return n > 0 ? s / static_cast<double>(n) : -1.0;
}

// Der Bildpunkt eines Weltpunkts in einer Vorschaukamera
bool aufBild(const Bild& b, const float p[3], int& x, int& y) {
    float v[3];
    float r[3];
    float o[3];
    b.cam.forward(v);
    b.cam.right(r);
    b.cam.up(o);
    const float d[3] = {p[0] - b.cam.pos[0], p[1] - b.cam.pos[1], p[2] - b.cam.pos[2]};
    const float ez = d[0] * v[0] + d[1] * v[1] + d[2] * v[2];
    if (ez <= 1.0F) { return false; }
    const float tanY = std::tan(b.cam.fovY * 3.14159265F / 360.0F);
    const float tanX = tanY * static_cast<float>(b.w) / static_cast<float>(b.h);
    x = static_cast<int>((1.0F + (d[0] * r[0] + d[1] * r[1] + d[2] * r[2]) / ez / tanX) * 0.5F * static_cast<float>(b.w));
    y = static_cast<int>((1.0F - (d[0] * o[0] + d[1] * o[1] + d[2] * o[2]) / ez / tanY) * 0.5F * static_cast<float>(b.h));
    return x >= 0 && y >= 0 && x < b.w && y < b.h;
}

void bmp(const std::string& name, const std::vector<std::uint8_t>& rgba, int w, int h) {
    const char* o = std::getenv("BHED_FOTOS");
    if (o == nullptr || o[0] == '\0') { return; }
    stufe9c::Bild b;
    b.rgba.assign(rgba.begin(), rgba.end());
    b.breite = w;
    b.hoehe = h;
    stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b);
}

// Die Engine-Sicht fuer die Vorschaukamera (bhed/portalsicht mit den Flaechen und Schirmen der geladenen Karte)
bool engineSicht(const Bild& b, PortalSicht& p) {
    return portalSichtFuer(portalKarteAus(g_app->geo, g_app->textures, g_app->map.entities), g_app->geo, g_app->areaPortale, b.cam, b.w, b.h, g_app->playMs, p);
}

// Vorschau: Karte laden, die Kamera `abstand` Einheiten vor die Portalflaeche, deren Mitte dem ersten Schirm am naechsten liegt,
// Blick auf ihre Mitte
// (`schraeg` Grad aus der Senkrechten), Bild lesen.
void vorschau(std::vector<Schritt>& s, const std::string& karte, float abstand, float schraeg, const std::shared_ptr<Bild>& bild) {
    s.push_back(tu("editorportal: " + karte + " in der Vorschau", [karte] {
        g_app->leftMode = 1;   // Mission-Tab: dort zeichnet die Vorschau
        if (!loadMapFromArchive("maps/" + karte + ".bsp")) { meldeFehler("editorportal: maps/" + karte + ".bsp nicht geladen"); }
    }));
    s.push_back(warteBis("editorportal: Vorschau " + karte + " bereit", [] { return g_app->mapBereit; }, 900));
    s.push_back(tu("editorportal: Kamera vor die Portalflaeche", [karte, abstand, schraeg, bild] {
        const PortalKarte pk = portalKarteAus(g_app->geo, g_app->textures, g_app->map.entities);
        const PortalFlaeche* ziel = nullptr;
        float beste = 1e30F;
        for (const PortalFlaeche& x : pk.flaechen) {
            if (pk.schirme.empty()) { break; }
            const PortalSchirm& y = pk.schirme.front();
            const float d = x.ebene[0] * y.ort[0] + x.ebene[1] * y.ort[1] + x.ebene[2] * y.ort[2] - x.ebene[3];
            float m[3] = {0.0F, 0.0F, 0.0F};
            for (const auto& e : x.ecken) {
                for (int k = 0; k < 3; ++k) { m[k] += e[static_cast<std::size_t>(k)] / static_cast<float>(x.ecken.size()); }
            }
            const float w = (m[0] - y.ort[0]) * (m[0] - y.ort[0]) + (m[1] - y.ort[1]) * (m[1] - y.ort[1]) + (m[2] - y.ort[2]) * (m[2] - y.ort[2]);
            if (std::fabs(d) <= 64.0F && w < beste) {
                beste = w;
                ziel = &x;
            }
        }
        if (ziel == nullptr) {
            meldeFehler("editorportal: " + karte + " hat keine Portalflaeche mit misc_portal_surface");
            return;
        }
        float m[3] = {0.0F, 0.0F, 0.0F};
        for (const auto& e : ziel->ecken) {
            for (int k = 0; k < 3; ++k) { m[k] += e[static_cast<std::size_t>(k)] / static_cast<float>(ziel->ecken.size()); }
        }
        // Blickrichtung -n, um `schraeg` Grad gekippt (bei waagerechter Flaeche nach +x, sonst nach oben)
        const float* n = ziel->ebene;
        const bool waagerecht = std::fabs(n[2]) > 0.7F;
        const float kipp[3] = {waagerecht ? 1.0F : 0.0F, 0.0F, waagerecht ? 0.0F : 1.0F};
        const float w = schraeg * 3.14159265F / 180.0F;
        float dir[3];
        for (int k = 0; k < 3; ++k) { dir[k] = -n[k] * std::cos(w) + kipp[k] * std::sin(w); }
        for (int k = 0; k < 3; ++k) {
            g_app->cam.pos[k] = m[k] - dir[k] * abstand;
            bild->mitte[k] = m[k];
        }
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->draufsicht = false;
        g_app->cam.angles[0] = -std::asin(std::clamp(dir[2], -1.0F, 1.0F)) * 180.0F / 3.14159265F;   // Nicken nach unten positiv
        g_app->cam.angles[1] = std::atan2(dir[1], dir[0]) * 180.0F / 3.14159265F;
        g_app->cam.angles[2] = 0.0F;
        g_app->playing = false;
        g_app->camViewActive = false;
        g_app->followCam = false;
        g_app->playMs = 0.0;
        g_app->mapDirty = true;
    }));
    s.push_back(stufe9c::langeWarten("editorportal: Vorschau " + karte + " gezeichnet", [] { return g_app->mapBereit && !g_app->mapDirty; }, 20000));
    s.push_back(pause(1.0));
    s.push_back(tu("editorportal: Vorschaubild " + karte, [karte, bild] {
        if (!gpu::leseFarbe(bild->rgba, bild->w, bild->h)) { meldeFehler("editorportal: Vorschaubild " + karte + " nicht lesbar"); }
        bild->cam = g_app->cam;
        bmp("p_vorschau_" + karte, bild->rgba, bild->w, bild->h);
        diag::detail("Knopftest: editorportal Vorschau " + karte + " " + std::to_string(bild->w) + " x " + std::to_string(bild->h) + ", Spiegel " +
                     (g_app->spiegelGezeichnet ? "ja" : "nein") + ", Portalkamera " + (g_app->nebenSicht.portal ? "ja" : "nein"));
    }));
}

// Die Vorschau zeichnet die Sicht der Engine: dieselbe Abbildung und dieselbe Schnittebene wie bhed/portalsicht.
Schritt vorschauWieEngine(const std::string& karte, bool portalKamera, const std::shared_ptr<Bild>& bild) {
    return pruefe("editorportal: Vorschau " + karte + " - " + (portalKamera ? "Portalkamera" : "Spiegel") +
                      " wie die Engine (R_GetPortalOrientations: Abbildung, portalPlane genau auf der Ebene)",
                  [karte, portalKamera, bild] {
                      PortalSicht p;
                      if (!engineSicht(*bild, p)) {
                          diag::detail("Knopftest: editorportal " + karte + " - die Engine sieht keine Portalflaeche");
                          return false;
                      }
                      const App::NebenSicht& ns = g_app->nebenSicht;
                      double groesste = 0.0;
                      for (int i = 0; i < 16; ++i) { groesste = std::max(groesste, static_cast<double>(std::fabs(ns.welt[i] - p.welt[i]))); }
                      for (int i = 0; i < 4; ++i) { groesste = std::max(groesste, static_cast<double>(std::fabs(ns.schnitt[i] - p.schnitt[i]))); }
                      diag::detail("Knopftest: editorportal " + karte + " - Vorschau gezeichnet " + std::to_string(g_app->spiegelGezeichnet) +
                                   ", groesste Abweichung " + std::to_string(groesste) + ", Schnitt d Vorschau " + std::to_string(ns.schnitt[3]) +
                                   " Engine " + std::to_string(p.schnitt[3]));
                      // 0,01: die Abbildung hat Verschiebungen bis 12440 Einheiten (duel_wbw), float hat dort 0,001 Aufloesung
                      return g_app->spiegelGezeichnet && p.portal == portalKamera && groesste <= 0.01;
                  });
}

// Editor: dieselbe .bsp als Vorlage im Spielbild, Kamera, Groesse und Shaderzeit wie die Vorschau
void editorWieVorschau(std::vector<Schritt>& s, const std::string& karte, const std::shared_ptr<Bild>& vb, const std::shared_ptr<Bild>& eb) {
    s.push_back(tu("editorportal: " + karte + " als Vorlage", [karte, vb] {
        const std::array<float, 5> kam = {vb->cam.pos[0], vb->cam.pos[1], vb->cam.pos[2], vb->cam.angles[0], vb->cam.angles[1]};
        (void)teil13b::vorlage(karte, kam).tun(0);
        ke::Editor& ed = ke::editor();
        ed.kamFenster.messBreite = vb->w;
        ed.kamFenster.messHoehe = vb->h;
        // alsSpielKamera: das Sichtfeld gilt fuer die laengere Seite
        const double t = std::tan(static_cast<double>(vb->cam.fovY) * 3.14159265358979323846 / 360.0);
        const double lang = vb->w >= vb->h ? t * static_cast<double>(vb->w) / static_cast<double>(vb->h) : t;
        ed.kameraE.sichtfeld = static_cast<float>(2.0 * std::atan(lang) * 180.0 / 3.14159265358979323846);
        ed.kamera.ursprung = karte::Vektor3f(vb->cam.pos[0], vb->cam.pos[1], vb->cam.pos[2]);
        ed.kamera.winkel = karte::Vektor3f(-vb->cam.angles[0], vb->cam.angles[1], 0.0F);
        ed.filterMaske = 0;
        ke::spielbildZeitFest(0.0);
    }));
    s.push_back(warteBis("editorportal: Texturen " + karte, [] { return ke::kameraBilderFertig(); }, 20000));
    s.push_back(pause(1.5));
    s.push_back(stufe9c::langeWarten("editorportal: Spielbild " + karte, [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    s.push_back(pause(0.5));
    s.push_back(tu("editorportal: Editorbild " + karte, [karte, vb, eb] {
        std::vector<unsigned char> px;
        if (!ke::kameraBild(px, eb->w, eb->h)) { meldeFehler("editorportal: Kamerabild " + karte + " nicht lesbar"); }
        eb->rgba.assign(px.begin(), px.end());
        eb->cam = vb->cam;
        std::copy(std::begin(vb->mitte), std::end(vb->mitte), eb->mitte);
        bmp("p_editor_" + karte, eb->rgba, eb->w, eb->h);
    }));
}

// Bildpunkte auf der Portalflaeche: Editor gegen Vorschau
Schritt bildVergleich(const std::string& karte, bool portalKamera, const std::shared_ptr<Bild>& vb, const std::shared_ptr<Bild>& eb) {
    return pruefe("editorportal: " + karte + " - das Spielbild zeigt " + (portalKamera ? "die Portalkamera" : "den Spiegel") +
                      " wie die Vorschau (im ganzen Bild im Mittel hoechstens 1, im Fenster 80 x 80 auf der Flaeche hoechstens 3 von 255 daneben)",
                  [karte, portalKamera, vb, eb] {
                      int x = 0;
                      int y = 0;
                      const bool sichtbar = aufBild(*vb, vb->mitte, x, y);
                      const double fenster = abweichung(*vb, *eb, x, y, 40);
                      const double ganz = abweichung(*vb, *eb, vb->w / 2, vb->h / 2, std::max(vb->w, vb->h));
                      const ke::SpielbildStand st = ke::spielbildStand();
                      diag::detail("Knopftest: editorportal " + karte + " Vorschau " + std::to_string(vb->w) + "x" + std::to_string(vb->h) + ", Editor " +
                                   std::to_string(eb->w) + "x" + std::to_string(eb->h) + ", Flaeche bei " + std::to_string(x) + " " + std::to_string(y) +
                                   ", Abweichung Fenster " + std::to_string(fenster) + ", ganzes Bild " + std::to_string(ganz) + ", Spielbild-Sicht " +
                                   (st.spiegel ? (st.portalKamera ? "Portalkamera" : "Spiegel") : "keine"));
                      // 3 von 255: dieselben Zeichner (gpumap); was bleibt, sind Kanten der Kantenglaettung und Rundung an Bildpunktgrenzen
                      return sichtbar && st.spiegel && st.portalKamera == portalKamera && fenster >= 0.0 && fenster <= 3.0 && ganz >= 0.0 && ganz <= 1.0;
                  });
}

// --- 3. eigene Testkarte -----------------------------------------------------------------
std::string testKarte() { return karteTestPfad("p_portale.map"); }

void testKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
        welt->kindEinfuegen(p);
    };
    const char* stein = "textures/yavin/temple_stone2";
    quader(karte::Vektor3f(0, 0, -8), karte::Vektor3f(528, 528, 8), stein);
    quader(karte::Vektor3f(0, 0, 520), karte::Vektor3f(528, 528, 8), stein);
    quader(karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 512, 256), stein);
    quader(karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 512, 256), stein);
    quader(karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 256), stein);
    quader(karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 256), stein);
    // Spiegelboden: Oberseite z = 2, misc_portal_surface 8 darueber (ohne target). textures/system/mirror_yavin: schwarz mit
    // alphaGen portal 512 darueber; textures/system/mirror1 hat keine Stufe und ist deshalb im Spiel kein Portal (SS_FOG)
    quader(karte::Vektor3f(0, 0, 1), karte::Vektor3f(128, 128, 1), "textures/system/mirror_yavin");
    // eine Saeule neben dem Spiegel, damit er etwas zeigt
    quader(karte::Vektor3f(200, 0, 128), karte::Vektor3f(24, 24, 128), "textures/yavin/temple_stone2");
    // Bildschirm an der Wand x = 512: Vorderseite x = 504 (textures/system/portal: alphaGen portal 256)
    quader(karte::Vektor3f(508, 0, 128), karte::Vektor3f(4, 64, 64), "textures/system/portal");
    welt->setzen("_ambient", "150");
    const auto ent = [&k](const char* klasse, std::vector<std::pair<const char*, const char*>> werte) {
        auto e = k.neueEntitaet(klasse, false);
        for (const auto& [a, b] : werte) { e->setzen(a, b); }
        k.entitaetEinfuegen(e);
    };
    ent("light", {{"origin", "0 0 400"}, {"light", "2500"}});
    ent("info_player_start", {{"origin", "-400 -400 24"}});
    ent("misc_portal_surface", {{"origin", "0 0 10"}});
    ent("misc_portal_surface", {{"origin", "500 0 128"}, {"target", "p_kam"}});
    ent("misc_portal_camera", {{"origin", "-300 0 200"}, {"targetname", "p_kam"}, {"target", "p_blick"}, {"roll", "10"}});
    ent("info_notnull", {{"origin", "200 0 128"}, {"targetname", "p_blick"}});
    (void)karte::karteSpeichern(testKarte(), k, false);
}

std::shared_ptr<karte::Entitaet> entitaet(const std::string& klasse, bool mitTarget) {
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->klassenname() == klasse && e->hat("target") == mitTarget) { return e; }
    }
    return nullptr;
}

void verschieben(const std::string& klasse, bool mitTarget, const char* ort) {
    ke::Editor& ed = ke::editor();
    const auto e = entitaet(klasse, mitTarget);
    if (!e) {
        meldeFehler("editorportal: " + klasse + " fehlt in der Testkarte");
        return;
    }
    ed.karte.auswahl.leeren();
    ed.karte.auswahl.waehlen(e);
    karte::schluesselSetzenRueckgaengig(ed.karte, "origin", ort);
    ed.karte.auswahl.leeren();
}

void kamera(float x, float y, float z, float nicken, float gier) {
    ke::Editor& ed = ke::editor();
    ed.kamera.ursprung = karte::Vektor3f(x, y, z);
    ed.kamera.winkel = karte::Vektor3f(nicken, gier, 0.0F);
}

// Mittlere Farbe im Fenster um die Bildmitte des Kamerabilds; mit Namen auch das Bild nach BHED_FOTOS
std::array<double, 3> mitteJetzt(const std::string& name = std::string()) {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    std::array<double, 3> r{-1.0, -1.0, -1.0};
    if (!ke::kameraBild(px, w, h) || w < 64 || h < 64) { return r; }
    if (!name.empty()) { bmp(name, std::vector<std::uint8_t>(px.begin(), px.end()), w, h); }
    double sum[3] = {0.0, 0.0, 0.0};
    long n = 0;
    for (int y = h / 2 - 24; y < h / 2 + 24; ++y) {
        for (int x = w / 2 - 24; x < w / 2 + 24; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
            for (std::size_t c = 0; c < 3; ++c) { sum[c] += px[i + c]; }
            ++n;
        }
    }
    for (std::size_t c = 0; c < 3; ++c) { r[c] = sum[c] / static_cast<double>(n); }
    return r;
}

// Das ganze Kamerabild (fuer die Live-Pruefungen)
std::vector<std::uint8_t> bildJetzt() {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    if (!ke::kameraBild(px, w, h)) { return {}; }
    return std::vector<std::uint8_t>(px.begin(), px.end());
}

// Mittlere Abweichung je Kanal zweier Kamerabilder (0..255); -1: nicht vergleichbar
double bildAbstand(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    if (a.empty() || a.size() != b.size()) { return -1.0; }
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); i += 4) {
        for (std::size_t c = 0; c < 3; ++c) { s += std::fabs(static_cast<double>(a[i + c]) - static_cast<double>(b[i + c])); }
    }
    return s / static_cast<double>(a.size() / 4U * 3U);
}

std::vector<Schritt> testKarteSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto f = std::make_shared<std::array<std::array<double, 3>, 6>>();   // Spiegel, Schirm weg, Spiegel wieder, Portal, Kamera verschoben, ausser Reichweite
    auto echt = std::make_shared<PortalSicht>();
    auto bilder = std::make_shared<std::array<std::vector<std::uint8_t>, 5>>();   // dieselben Zustaende als ganze Bilder
    add(tu("editorportal: Testkarte mit Spiegelboden und Portalkamera, Echtzeit-Licht", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::vorlageEntfernen();
        testKarteSchreiben();
        (void)ke::karteOeffnen(testKarte());
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        ke::spielbildZeitFest(0.0);
        kamera(-60.0F, 0.0F, 220.0F, -60.0F, 0.0F);   // ueber dem Spiegel, schraeg nach unten zur Saeule
    }));
    add(warteBis("editorportal: Texturen der Testkarte", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pruefe("editorportal: textures/system/mirror1 ohne Stufe ist kein Portal (FinishShader: SS_FOG), mirror_yavin und portal sind es", [] {
        const ke::SpielDarstellung a = ke::spielDarstellung("textures/system/mirror1");
        const ke::SpielDarstellung b = ke::spielDarstellung("textures/system/mirror_yavin");
        const ke::SpielDarstellung c = ke::spielDarstellung("textures/system/portal");
        diag::detail("Knopftest: editorportal Portal-Shader mirror1 " + std::to_string(a.portal) + ", mirror_yavin " + std::to_string(b.portal) + " (" +
                     std::to_string(b.portalWeite) + "), portal " + std::to_string(c.portal) + " (" + std::to_string(c.portalWeite) + ")");
        return !a.portal && b.portal && b.portalWeite == 512.0F && c.portal && c.portalWeite == 256.0F;
    }));
    add(pause(1.0));
    add(tu("editorportal: Spiegel im Echtzeit-Licht", [f, bilder] {
        (*f)[0] = mitteJetzt("p_echt_spiegel");
        (*bilder)[0] = bildJetzt();
    }));
    add(pruefe("editorportal: Echtzeit-Licht - der Spiegelboden zeigt die Spiegelsicht (misc_portal_surface ohne target)", [f] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: editorportal Echtzeit Spiegel " + std::string(e.echtLicht.spiegel ? "ja" : "nein") + ", Mitte " + stufe9d::farbText((*f)[0]));
        return e.echtLicht.spiegel && !e.echtLicht.spiegelPortal;
    }));
    // Live: der Schirm 100 ueber den Boden (mehr als 64 von der Ebene) - im naechsten Bild kein Spiegel mehr, zurueck: wieder
    add(tu("editorportal: misc_portal_surface 100 ueber den Boden", [] { verschieben("misc_portal_surface", false, "0 0 102"); }));
    add(tu("editorportal: Bild nach dem Verschieben", [f, bilder] {
        (*f)[1] = mitteJetzt();
        (*bilder)[1] = bildJetzt();
    }));
    add(pruefe("editorportal: live - mehr als 64 Einheiten von der Ebene ist im naechsten Bild kein Spiegel (R_GetPortalOrientations)", [f, bilder] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: editorportal Schirm weg - Spiegel " + std::string(e.echtLicht.spiegel ? "ja" : "nein") + ", Mitte " +
                     stufe9d::farbText((*f)[1]) + ", Bildabstand " + std::to_string(bildAbstand((*bilder)[0], (*bilder)[1])));
        // 0,1: dasselbe Bild ist bitgleich (gemessen 0,0), eine geaenderte Spiegelflaeche hebt das Mittel darueber
        return !e.echtLicht.spiegel && bildAbstand((*bilder)[0], (*bilder)[1]) > 0.1;
    }));
    add(tu("editorportal: misc_portal_surface zurueck", [] { verschieben("misc_portal_surface", false, "0 0 10"); }));
    add(tu("editorportal: Bild nach dem Zurueckschieben", [f, echt, bilder] {
        (*f)[2] = mitteJetzt();
        (*bilder)[2] = bildJetzt();
        const ke::Editor& e = ke::editor();
        if (!ke::echtPortalSicht(e.kamera, e.kamFenster.messBreite, e.kamFenster.messHoehe, e.kameraE.sichtfeld, ke::geometrieStand(), *echt)) {
            *echt = PortalSicht{};
        }
    }));
    add(pruefe("editorportal: live - zurueckgeschoben ist der Spiegel im naechsten Bild wieder da und gleich", [bilder] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: editorportal Schirm zurueck - Spiegel " + std::string(e.echtLicht.spiegel ? "ja" : "nein") + ", Abstand zum ersten Bild " +
                     std::to_string(bildAbstand((*bilder)[0], (*bilder)[2])));
        return e.echtLicht.spiegel && bildAbstand((*bilder)[0], (*bilder)[2]) >= 0.0 && bildAbstand((*bilder)[0], (*bilder)[2]) < 0.5;
    }));
    // Portalkamera: vor dem Bildschirm (124 vor ihm, innerhalb portalRange 256)
    add(tu("editorportal: vor den Bildschirm", [] { kamera(380.0F, 0.0F, 128.0F, 0.0F, 0.0F); }));
    add(pause(0.5));
    add(tu("editorportal: Portalkamera im Echtzeit-Licht", [f, bilder] {
        (*f)[3] = mitteJetzt("p_echt_portal");
        (*bilder)[3] = bildJetzt();
    }));
    add(pruefe("editorportal: Echtzeit-Licht - der Bildschirm zeigt die misc_portal_camera", [f] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: editorportal Echtzeit Portal " + std::string(e.echtLicht.spiegelPortal ? "ja" : "nein") + ", Mitte " + stufe9d::farbText((*f)[3]));
        return e.echtLicht.spiegel && e.echtLicht.spiegelPortal;
    }));
    add(tu("editorportal: misc_portal_camera verschieben", [] { verschieben("misc_portal_camera", true, "-300 200 200"); }));
    add(tu("editorportal: Bild nach dem Verschieben der Kamera", [f, bilder] {
        (*f)[4] = mitteJetzt();
        (*bilder)[4] = bildJetzt();
    }));
    add(pruefe("editorportal: live - die verschobene Kamera aendert den Bildschirm im naechsten Bild", [bilder] {
        diag::detail("Knopftest: editorportal Kamera verschoben - Bildabstand " + std::to_string(bildAbstand((*bilder)[3], (*bilder)[4])));
        return ke::editor().echtLicht.spiegelPortal && bildAbstand((*bilder)[3], (*bilder)[4]) > 0.1;
    }));
    add(tu("editorportal: Kamera zurueck, 268 vor den Bildschirm (die Saeule hinter der Kamera)", [] {
        verschieben("misc_portal_camera", true, "-300 0 200");
        kamera(236.0F, 0.0F, 128.0F, 0.0F, 0.0F);
    }));
    add(pause(0.5));
    add(tu("editorportal: Bildschirm ausser Reichweite", [f] { (*f)[5] = mitteJetzt("p_echt_weit"); }));
    add(pruefe("editorportal: weiter als portalRange 256 keine Portalsicht, der Bildschirm ist schwarz (alphaGen portal 1)", [f] {
        const auto& m = (*f)[5];
        diag::detail("Knopftest: editorportal ausser Reichweite - Sicht " + std::string(ke::editor().echtLicht.spiegel ? "ja" : "nein") + ", Mitte " + stufe9d::farbText(m));
        return !ke::editor().echtLicht.spiegel && m[0] < 8.0 && m[1] < 8.0 && m[2] < 8.0;
    }));
    // Schnell-Backen und Spielbild: dieselbe Sicht aus der gebauten Karte
    add(tu("editorportal: Schnell-Backen, Spielbild", [] {
        kamera(-60.0F, 0.0F, 220.0F, -60.0F, 0.0F);
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("editorportal: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("editorportal: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("editorportal: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(0.5));
    add(tu("editorportal: Spielbild Spiegel", [] { (void)mitteJetzt("p_spielbild_spiegel"); }));
    add(pruefe("editorportal: Spielbild der gebauten Testkarte - dieselbe Spiegelsicht wie das Echtzeit-Licht (Abbildung und Schnitt gleich)", [echt] {
        const ke::SpielbildStand st = ke::spielbildStand();
        double groesste = 0.0;
        for (int i = 0; i < 16; ++i) { groesste = std::max(groesste, static_cast<double>(std::fabs(st.spiegelWelt[i] - echt->welt[i]))); }
        for (int i = 0; i < 4; ++i) { groesste = std::max(groesste, static_cast<double>(std::fabs(st.spiegelSchnitt[i] - echt->schnitt[i]))); }
        diag::detail("Knopftest: editorportal Spielbild Spiegel " + std::string(st.spiegel ? "ja" : "nein") + ", Abweichung zur Echtzeit-Sicht " + std::to_string(groesste));
        // 0,01: q3map2 schreibt die Ebene als float (lightmapVecs[2]), der Editor rechnet sie in double
        return st.spiegel && !st.portalKamera && groesste <= 0.01;
    }));
    add(tu("editorportal: Spielbild vor dem Bildschirm", [] { kamera(380.0F, 0.0F, 128.0F, 0.0F, 0.0F); }));
    add(pause(0.5));
    add(tu("editorportal: Spielbild Portalkamera", [] { (void)mitteJetzt("p_spielbild_portal"); }));
    add(pruefe("editorportal: Spielbild der gebauten Testkarte - der Bildschirm zeigt die Portalkamera", [] {
        const ke::SpielbildStand st = ke::spielbildStand();
        return st.spiegel && st.portalKamera;
    }));
    add(tu("editorportal: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// --- rc610 R2a: der Spiegel-Shader des Testers (05.11.) ------------------------------------------------------------------
//
// Zwei Stufen ueber dem Spiegelbild: qer_mirror (blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA, depthWrite), darueber floor2_alpha
// (blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA, rgbGen identity, alphaGen identity). Das Spiel zeichnet die Portalflaeche
// mit RB_StageIteratorGeneric, also beide Stufen ueber dem Bild der Spiegelsicht. Shader und Bilder schreibt der Test in einen
// Extra Resource Path des Laufs (textures/r2a/...): floor2_alpha ist zur Haelfte deckend gruen, zur Haelfte durchsichtig -
// dort ist der Spiegel zu sehen. Ein zweiter Spiegel 2000 Einheiten weiter mit eigenem misc_portal_surface 16 ueber dem Boden.
std::string testerQuelle() { return stufe2::laufDatei("r2a_spiegel_quelle"); }
std::string testerKarte() { return karteTestPfad("r2a_spiegel.map"); }
// rc610 R2b: dieselbe Halle fuer das Handbuchbild, mit einem hellen Wuerfel ueber dem Spiegel (im Spiegel gut zu sehen)
std::string testerKarteHandbuch() { return karteTestPfad("r2b_spiegel_hb.map"); }
constexpr const char* kTesterShader = "textures/r2a/spiegel_tester";

// TGA 32 Bit, unkomprimiert, von unten nach oben; f(x, y) -> RGBA
std::string tga32(int w, int h, const std::function<std::array<unsigned char, 4>(int, int)>& f) {
    std::string t(18, '\0');
    t[2] = 2;
    t[12] = static_cast<char>(w & 255);
    t[13] = static_cast<char>(w >> 8);
    t[14] = static_cast<char>(h & 255);
    t[15] = static_cast<char>(h >> 8);
    t[16] = 32;
    t[17] = 8;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::array<unsigned char, 4> c = f(x, y);
            t.push_back(static_cast<char>(c[2]));
            t.push_back(static_cast<char>(c[1]));
            t.push_back(static_cast<char>(c[0]));
            t.push_back(static_cast<char>(c[3]));
        }
    }
    return t;
}

void testerSchreiben(bool handbuch = false) {
    // als .pk3 im Ordner des Laufs: der Editor liest ihn als Extra Resource Path, das Spielbild (Zeichner der Vorschau) als
    // Spielordner dieser Sitzung - wie ein Mod-Ordner mit dem Shader des Testers
    std::vector<Pk3Datei> pk3(4);
    pk3[0].name = "shaders/r2a_spiegel.shader";
    pk3[0].inhalt = std::string(kTesterShader) +
                    "\n{\n\tqer_editorimage textures/r2a/floor2_alpha\n\tsurfaceparm trans\n\tsurfaceparm nonsolid\n\tsurfaceparm nolightmap\n"
                    "\tportal\n\tsort portal\n\t{\n\t\tmap textures/r2a/qer_mirror\n\t\tblendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA\n\t\tdepthWrite\n\t}\n"
                    "\t{\n\t\tmap textures/r2a/floor2_alpha\n\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n\t\trgbGen identity\n"
                    "\t\talphaGen identity\n\t}\n}\n";
    pk3[1].name = "textures/r2a/qer_mirror.tga";
    pk3[1].inhalt = tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{20, 20, 40, 64}; });
    pk3[2].name = "textures/r2a/floor2_alpha.tga";
    pk3[2].inhalt = tga32(64, 64, [](int x, int) {
        return x < 32 ? std::array<unsigned char, 4>{40, 200, 40, 255} : std::array<unsigned char, 4>{0, 0, 0, 0};
    });
    pk3[3].name = "textures/r2b/hell.tga";   // rc610 R2b: der helle Wuerfel des Handbuchbilds
    pk3[3].inhalt = tga32(16, 16, [](int x, int y) {
        return ((x / 4 + y / 4) % 2) == 0 ? std::array<unsigned char, 4>{255, 240, 120, 255} : std::array<unsigned char, 4>{255, 255, 255, 255};
    });
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(testerQuelle()), ec);
    std::string err;
    if (!writePk3(testerQuelle() + "\\r2a_spiegel.pk3", pk3, &err)) { meldeFehler("R2a Spiegel: .pk3 nicht geschrieben: " + err); }
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
        welt->kindEinfuegen(p);
    };
    const char* stein = "textures/yavin/temple_stone2";
    // Halle x -512 .. 2512, y -1536 .. 1536, 1024 hoch
    quader(karte::Vektor3f(1000, 0, -8), karte::Vektor3f(1528, 1552, 8), stein);
    quader(karte::Vektor3f(1000, 0, 1032), karte::Vektor3f(1528, 1552, 8), stein);
    quader(karte::Vektor3f(-520, 0, 512), karte::Vektor3f(8, 1536, 512), stein);
    quader(karte::Vektor3f(2520, 0, 512), karte::Vektor3f(8, 1536, 512), stein);
    quader(karte::Vektor3f(1000, -1544, 512), karte::Vektor3f(1512, 8, 512), stein);
    quader(karte::Vektor3f(1000, 1544, 512), karte::Vektor3f(1512, 8, 512), stein);
    for (const float x : {0.0F, 2000.0F}) {
        quader(karte::Vektor3f(x, 0, 1), karte::Vektor3f(128, 128, 1), kTesterShader);       // Spiegel, Oberseite z = 2
        quader(karte::Vektor3f(x + 200.0F, 0, 128), karte::Vektor3f(24, 24, 128), stein);    // eine Saeule daneben
    }
    if (handbuch) { quader(karte::Vektor3f(250, 170, 230), karte::Vektor3f(56, 56, 56), "textures/r2b/hell"); }
    welt->setzen("_ambient", "150");
    const auto ent = [&k](const char* klasse, std::vector<std::pair<const char*, const char*>> werte) {
        auto e = k.neueEntitaet(klasse, false);
        for (const auto& [a, b] : werte) { e->setzen(a, b); }
        k.entitaetEinfuegen(e);
    };
    ent("light", {{"origin", "0 0 600"}, {"light", "3000"}});
    ent("light", {{"origin", "2000 0 600"}, {"light", "3000"}});
    ent("info_player_start", {{"origin", "-400 -400 24"}});
    ent("misc_portal_surface", {{"origin", "0 0 18"}});   // 16 ueber dem Spiegel (Bild des Testers)
    ent("misc_portal_surface", {{"origin", "2000 0 18"}});
    (void)karte::karteSpeichern(handbuch ? testerKarteHandbuch() : testerKarte(), k, false);
}

// Anteil der gruenen Bildpunkte (Stufe floor2_alpha) im Fenster 240 x 240 um die Bildmitte; das Bild nach BHED_FOTOS
double gruenJetzt(const std::string& name) {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    if (!ke::kameraBild(px, w, h) || w < 480 || h < 480) { return -1.0; }
    bmp(name, std::vector<std::uint8_t>(px.begin(), px.end()), w, h);
    long n = 0;
    long gruen = 0;
    for (int y = h / 2 - 120; y < h / 2 + 120; ++y) {
        for (int x = w / 2 - 120; x < w / 2 + 120; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
            gruen += (px[i + 1] > px[i] + 60 && px[i + 1] > px[i + 2] + 60) ? 1 : 0;
            ++n;
        }
    }
    return static_cast<double>(gruen) / static_cast<double>(n);
}

// Die Sicht der Echtzeit-Ansicht fuer die jetzige Kamera: die gewaehlte Portalflaeche (ihre Nummer); -1: keine. Der
// misc_portal_surface ist fuer beide Spiegel derselbe: R_GetPortalOrientations (tr_main.cpp) nimmt den ersten Portal-Eintrag
// in 64 Einheiten der Ebene, und beide liegen auf z = 2.
float sichtX() {
    const ke::Editor& e = ke::editor();
    PortalSicht ps;
    if (!ke::echtPortalSicht(e.kamera, e.kamFenster.messBreite, e.kamFenster.messHoehe, e.kameraE.sichtfeld, ke::geometrieStand(), ps)) { return -1.0F; }
    return static_cast<float>(ps.flaeche);
}

std::vector<Schritt> testerSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto gruen = std::make_shared<std::array<double, 3>>();   // Echtzeit, Echtzeit zweiter Spiegel, Spielbild
    auto x = std::make_shared<std::array<float, 3>>();        // gewaehlte Sicht: Spiegel 1, Spiegel 2, beide im Bild
    add(tu("R2a Spiegel: Shader des Testers und Bilder in einen Extra Resource Path", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        testerSchreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(testerQuelle()); });
    }));
    add(warteBis("R2a Spiegel: Spieldaten mit dem Extra Resource Path", [] {
        ke::spielPruefen();
        const ke::Editor& e = ke::editor();
        return ke::spielBereit() && e.karteSpiel && e.karteSpiel->inventar.shaderFinden(kTesterShader) != nullptr;
    }, 20000));
    add(tu("R2a Spiegel: Karte mit zwei Spiegeln, Echtzeit-Licht ueber dem ersten", [] {
        (void)ke::karteOeffnen(testerKarte());
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        ke::spielbildZeitFest(0.0);
        kamera(-60.0F, 0.0F, 220.0F, -60.0F, 0.0F);
    }));
    add(warteBis("R2a Spiegel: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(tu("R2a Spiegel: Echtzeit-Licht ueber dem ersten Spiegel", [gruen, x] {
        (*gruen)[0] = gruenJetzt("r2a_spiegel_echt");
        (*x)[0] = sichtX();
    }));
    add(pruefe("R2a Spiegel: Echtzeit-Licht - Spiegelsicht da, beide Stufen darueber (die deckende Haelfte von floor2_alpha ist gruen)", [gruen, x] {
        const ke::Editor& e = ke::editor();
        const ke::SpielDarstellung sd = ke::spielDarstellung(kTesterShader);
        diag::detail("Knopftest: R2a Spiegel Echtzeit - Portal " + std::to_string(sd.portal) + ", Sicht " + (e.echtLicht.spiegel ? "ja" : "nein") +
                     " (Flaeche " + std::to_string((*x)[0]) + "), gruen " + std::to_string((*gruen)[0]));
        return sd.portal && e.echtLicht.spiegel && (*x)[0] >= 0.0F && (*gruen)[0] > 0.3 && (*gruen)[0] < 0.7;
    }));
    add(tu("R2a Spiegel: ueber den zweiten Spiegel (der erste ist nicht im Bild)", [] { kamera(1940.0F, 0.0F, 220.0F, -60.0F, 0.0F); }));
    add(pause(0.5));
    add(tu("R2a Spiegel: Echtzeit-Licht ueber dem zweiten Spiegel", [gruen, x] {
        (*gruen)[1] = gruenJetzt("r2a_spiegel_echt2");
        (*x)[1] = sichtX();
    }));
    add(pruefe("R2a Spiegel: der zweite Spiegel wird benutzt, sobald er im Bild ist", [gruen, x] {
        diag::detail("Knopftest: R2a Spiegel 2 - Sicht " + std::string(ke::editor().echtLicht.spiegel ? "ja" : "nein") + " (Flaeche " + std::to_string((*x)[1]) +
                     "), gruen " + std::to_string((*gruen)[1]));
        return ke::editor().echtLicht.spiegel && (*x)[1] >= 0.0F && (*x)[1] != (*x)[0] && (*gruen)[1] > 0.3;
    }));
    add(tu("R2a Spiegel: beide Spiegel im Bild", [x] {
        kamera(1000.0F, -1400.0F, 700.0F, -30.0F, 90.0F);
        (*x)[2] = -1.0F;
    }));
    add(pause(0.5));
    add(tu("R2a Spiegel: welche Sicht", [x] {
        (void)gruenJetzt("r2a_spiegel_beide");
        (*x)[2] = sichtX();
    }));
    add(pruefe("R2a Spiegel: zwei im Bild - genau eine Spiegelsicht (R_SortDrawSurfs: only one mirror view at a time)", [x] {
        diag::detail("Knopftest: R2a Spiegel beide - Sicht " + std::string(ke::editor().echtLicht.spiegel ? "ja" : "nein") + ", Flaeche " +
                     std::to_string((*x)[2]));
        const float v = (*x)[2];
        return ke::editor().echtLicht.spiegel && (v == (*x)[0] || v == (*x)[1]);
    }));
    add(tu("R2a Spiegel: Schnell-Backen, Spielbild ueber dem ersten Spiegel (die .pk3 als Spielordner dieser Sitzung)", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        ordner.push_back(testerQuelle());
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        kamera(-60.0F, 0.0F, 220.0F, -60.0F, 0.0F);
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("R2a Spiegel: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("R2a Spiegel: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("R2a Spiegel: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.0));
    add(tu("R2a Spiegel: Spielbild", [gruen] { (*gruen)[2] = gruenJetzt("r2a_spiegel_spielbild"); }));
    add(pruefe("R2a Spiegel: Spielbild - Spiegelsicht und beide Stufen wie im Echtzeit-Licht (gruen gleich bis auf 0,1)", [gruen] {
        const ke::SpielbildStand st = ke::spielbildStand();
        diag::detail("Knopftest: R2a Spiegel Spielbild - Sicht " + std::string(st.spiegel ? "ja" : "nein") + ", gruen " + std::to_string((*gruen)[2]) +
                     " (Echtzeit " + std::to_string((*gruen)[0]) + ")");
        return st.spiegel && (*gruen)[2] > 0.3 && std::fabs((*gruen)[2] - (*gruen)[0]) < 0.1;
    }));
    // Bildzeit mit und ohne Spiegel (nur mit BHED_P_ZEIT=1, allein laufen lassen): Echtzeit-Licht, die Kamera ueber dem ersten
    // Spiegel dreht sich; ohne Spiegel = der misc_portal_surface 100 hoeher (keine Portal-Entity in 64 Einheiten)
    if (const char* zeit = std::getenv("BHED_P_ZEIT"); zeit != nullptr && zeit[0] == '1') {
        add(tu("R2a Spiegel Zeit: Echtzeit-Licht", [] {
            ke::Editor& e = ke::editor();
            e.echtLicht.backen = false;
            e.echtLicht.spielbild = false;
            ke::spielbildZeitFest(-1.0);
        }));
        for (const bool mit : {true, false}) {
            const std::string name = mit ? "mit Spiegel" : "ohne Spiegel";
            if (!mit) {
                // beide misc_portal_surface 100 hoeher: keine Portal-Entity mehr in 64 Einheiten der Ebene (R_GetPortalOrientations)
                add(tu("R2a Spiegel Zeit: beide misc_portal_surface 100 hoeher", [] {
                    ke::Editor& ed = ke::editor();
                    ed.karte.auswahl.leeren();
                    for (const auto& e : ed.karte.entitaeten()) {
                        if (e->klassenname() == "misc_portal_surface") { ed.karte.auswahl.waehlen(e); }
                    }
                    karte::Befehl b(ed.karte.undo, "translateTool -xyz 0 0 100");
                    for (const auto& e : ed.karte.entitaeten()) {
                        if (e->klassenname() != "misc_portal_surface") { continue; }
                        float o[3] = {0, 0, 0};
                        (void)std::sscanf(e->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
                        char t[64];
                        std::snprintf(t, sizeof(t), "%g %g %g", static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2] + 100.0F));
                        e->setzen("origin", t);
                    }
                    ed.karte.auswahl.leeren();
                }));
            }
            add(pause(1.0));
            auto bilder = std::make_shared<std::vector<double>>();
            add({"R2a Spiegel Zeit: 240 Bilder " + name, [bilder](int b) {
                     ke::Editor& e = ke::editor();
                     if (b > 2) { bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0); }
                     e.kamera.ursprung = karte::Vektor3f(-60.0F, 0.0F, 220.0F);
                     e.kamera.winkel = karte::Vektor3f(-60.0F, static_cast<float>(b % 40) - 20.0F, 0.0F);
                     return b >= 242;
                 }});
            add(tu("R2a Spiegel Zeit: Ergebnis " + name, [bilder, name] {
                std::vector<double> v = *bilder;
                std::sort(v.begin(), v.end());
                double summe = 0;
                for (const double t : v) { summe += t; }
                char z[300];
                std::snprintf(z, sizeof(z), "R2a-Spiegel-Bildzeit: %s Mittel %.2f ms, p99 %.2f ms, Sicht %s", name.c_str(),
                              v.empty() ? 0.0 : summe / static_cast<double>(v.size()), v.empty() ? 0.0 : v[std::min(v.size() - 1, v.size() * 99 / 100)],
                              ke::editor().echtLicht.spiegel ? "ja" : "nein");
                diag::detail(z);
            }));
        }
    }
    add(tu("R2a Spiegel: aufraeumen", [] {
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
        if (!ordner.empty() && ordner.back() == testerQuelle()) { ordner.pop_back(); }
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0].clear(); });
    }));
    add(warteBis("R2a Spiegel: Spieldaten ohne den Extra Resource Path", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    return s;
}

// BHED_P_SPIEL="name|karte|x y z nick gier;...": nur Editorbilder fuer den Spielvergleich - die .bsp als Vorlage im Spielbild,
// Kamera wie viewpos im Spiel (cg_fov 80, 1920 x 1440), Bild nach BHED_FOTOS als p_spiel_<name>.bmp.
std::vector<Schritt> spielvergleich(const char* liste) {
    std::vector<Schritt> s = stufe9c::einrichten();
    std::istringstream in(liste);
    for (std::string z; std::getline(in, z, ';');) {
        std::vector<std::string> teile;
        std::istringstream zz(z);
        for (std::string x; std::getline(zz, x, '|');) { teile.push_back(x); }
        std::array<float, 5> kam{};
        if (teile.size() != 3U || std::sscanf(teile[2].c_str(), "%f %f %f %f %f", &kam[0], &kam[1], &kam[2], &kam[3], &kam[4]) != 5) { continue; }
        const std::string name = teile[0];
        s.push_back(teil13b::vorlage(teile[1], kam));
        s.push_back(tu("editorportal: Shaderzeit fest " + name, [] { ke::spielbildZeitFest(0.0); }));
        s.push_back(warteBis("editorportal: Texturen " + name, [] { return ke::kameraBilderFertig(); }, 20000));
        s.push_back(pause(1.5));
        s.push_back(stufe9c::langeWarten("editorportal: Spielbild " + name, [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
        s.push_back(pause(0.5));
        s.push_back(tu("editorportal: Spielvergleich " + name, [name] {
            std::vector<unsigned char> px;
            int w = 0;
            int h = 0;
            if (!ke::kameraBild(px, w, h)) { meldeFehler("editorportal: Kamerabild " + name + " nicht lesbar"); }
            bmp("p_spiel_" + name, std::vector<std::uint8_t>(px.begin(), px.end()), w, h);
            const ke::SpielbildStand st = ke::spielbildStand();
            diag::detail("Knopftest: editorportal Spielvergleich " + name + " " + std::to_string(w) + " x " + std::to_string(h) + ", Sicht " +
                         (st.spiegel ? (st.portalKamera ? "Portalkamera" : "Spiegel") : "keine"));
        }));
    }
    s.push_back(tu("editorportal: Vorlage weg", [] {
        ke::spielbildZeitFest(-1.0);
        ke::vorlageEntfernen();
    }));
    return s;
}

std::vector<Schritt> schritte() {
    if (const char* liste = std::getenv("BHED_P_SPIEL"); liste != nullptr && liste[0] != '\0') { return spielvergleich(liste); }
    // rc610 R2a: nur der Spiegel-Shader des Testers (BHED_P_NUR=tester)
    if (const char* nur = std::getenv("BHED_P_NUR"); nur != nullptr && std::strcmp(nur, "tester") == 0) {
        std::vector<Schritt> t = stufe9c::einrichten();
        for (Schritt& y : testerSchritte()) { t.push_back(std::move(y)); }
        return t;
    }
    std::vector<Schritt> s;
    // 1. und 2.: Vorschau, dann dieselbe Karte im Editor
    struct Fall {
        const char* karte;
        float abstand;
        float schraeg;
        bool portalKamera;
    };
    // duel_executor: Spiegelboden; duel_wbw: misc_portal_camera mit target; duel_calodan: Gegenprobe Spiegel (Wasser, anderer Shader)
    const Fall faelle[] = {{"duel_executor", 160.0F, 35.0F, false}, {"duel_wbw", 120.0F, 0.0F, true}, {"duel_calodan", 160.0F, 35.0F, false}};
    std::vector<std::pair<Fall, std::pair<std::shared_ptr<Bild>, std::shared_ptr<Bild>>>> bilder;
    for (const Fall& fa : faelle) {
        auto vb = std::make_shared<Bild>();
        vorschau(s, fa.karte, fa.abstand, fa.schraeg, vb);
        s.push_back(vorschauWieEngine(fa.karte, fa.portalKamera, vb));
        bilder.push_back({fa, {vb, std::make_shared<Bild>()}});
    }
    for (Schritt& x : stufe9c::einrichten()) { s.push_back(std::move(x)); }
    for (const auto& [fa, b] : bilder) {
        editorWieVorschau(s, fa.karte, b.first, b.second);
        s.push_back(bildVergleich(fa.karte, fa.portalKamera, b.first, b.second));
    }
    s.push_back(tu("editorportal: Vorlage weg", [] { ke::vorlageEntfernen(); }));
    for (Schritt& x : testKarteSchritte()) { s.push_back(std::move(x)); }
    for (Schritt& x : testerSchritte()) { s.push_back(std::move(x)); }   // rc610 R2a
    return s;
}

}  // namespace editorportal

std::vector<Schritt> editorportalSchritte() { return editorportal::schritte(); }

// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=portale: die Testkarte im Echtzeit-Licht, Spiegelboden und Bildschirm im Bild.
std::vector<Schritt> portaleHandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s = stufe9c::einrichten();
    s.push_back(tu("Handbuch rc609 P: Testkarte mit Spiegelboden und Bildschirm, Echtzeit-Licht", [] {
        karteDialogeZu();
        ke::setzeNrcBegriffe(false);
        editorportal::testKarteSchreiben();
        (void)ke::karteOeffnen(editorportal::testKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        editorportal::kamera(-170.0F, -90.0F, 130.0F, -22.0F, 18.0F);
        if (ImGuiWindow* w = karteFenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.6F, w->Pos.y + w->Size.y - 4.0F}); }   // kein Tooltip im Bild
    }));
    s.push_back(warteBis("Handbuch rc609 P: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    s.push_back(pause(1.5));
    s.push_back(hb::fotoTeil("h_karte_portale", [] { return hb::fensterRect("##feldKamera"); }));
    s.push_back(tu("Handbuch rc609 P: Ende", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        g_app->leftMode = 0;
    }));
    return s;
}

// rc610 R2a, BHED_EDITORTEST=handbuch mit BHED_HB_NUR=r2a: Gras (surfaceSprites) im Echtzeit-Licht und der Spiegel-Shader des
// Testers mit seinen zwei Stufen.
std::vector<Schritt> r2aHandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s = stufe9c::einrichten();
    const auto ohneTooltip = [] {
        if (ImGuiWindow* w = karteFenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.6F, w->Pos.y + w->Size.y - 4.0F}); }
    };
    s.push_back(tu("Handbuch R2a: Testraum mit Gras, Echtzeit-Licht", [ohneTooltip] {
        karteDialogeZu();
        ke::setzeNrcBegriffe(false);
        r2a::grasKarteSchreiben();
        (void)ke::karteOeffnen(r2a::grasKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        g_app->zeigeGras = true;
        if (const auto welt = e.karte.welt()) { welt->setzen("_ambient", "320"); }   // fuers Bild heller als der Test
        e.karte.undo.alsGespeichertMarkieren();
        r2a::kamera(-760.0F, -700.0F, 90.0F, -10.0F, 42.0F);
        ohneTooltip();
    }));
    s.push_back(warteBis("Handbuch R2a: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    s.push_back(pause(1.5));
    s.push_back(hb::fotoTeil("h_r2a_gras", [] { return hb::fensterRect("##feldKamera"); }));
    s.push_back(tu("Handbuch R2a: Spiegel-Shader des Testers in einen Extra Resource Path", [] {
        ke::editor().karte.undo.alsGespeichertMarkieren();
        editorportal::testerSchreiben(true);
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(editorportal::testerQuelle()); });
    }));
    s.push_back(warteBis("Handbuch R2a: Spieldaten mit dem Extra Resource Path", [] {
        ke::spielPruefen();
        const ke::Editor& e = ke::editor();
        return ke::spielBereit() && e.karteSpiel && e.karteSpiel->inventar.shaderFinden(editorportal::kTesterShader) != nullptr;
    }, 20000));
    s.push_back(tu("Handbuch R2a: Karte mit dem Spiegel", [ohneTooltip] {
        (void)ke::karteOeffnen(editorportal::testerKarteHandbuch());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        if (const auto welt = e.karte.welt()) { welt->setzen("_ambient", "320"); }   // fuers Bild heller als der Test
        e.karte.undo.alsGespeichertMarkieren();
        editorportal::kamera(-250.0F, -170.0F, 110.0F, -18.0F, 32.0F);
        ohneTooltip();
    }));
    s.push_back(warteBis("Handbuch R2a: Texturen der Spiegelkarte", [] { return ke::kameraBilderFertig(); }, 6000));
    s.push_back(pause(1.5));
    s.push_back(hb::fotoTeil("h_r2a_spiegel", [] { return hb::fensterRect("##feldKamera"); }));
    s.push_back(tu("Handbuch R2a: Ende", [] {
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
