// selbsttest_karte6.inc - Stufe 6 des Karten-Editors: die Pruefstufe.
//
// Eingebunden in gui/selbsttest.cpp (namenloser Namensraum, nach Stufe 5).
// Auftrag shank (25.10.2026): "dass er bitte schaut das alle buttons gehen
// und wir keine bugs uebernommen haben von radiant". Jeder registrierte
// Befehl, jeder Menuepunkt, jeder Werkzeugknopf und jedes Tastenkuerzel wird
// mit einer WIRKUNGSPRUEFUNG ausgeloest: der Zustand des Editors (Karte als
// .map-Text, Auswahl, Ansichten, Kamera, Raster, Filter, Dialoge,
// Einstellungen, Dateien ...) wird vorher und nachher erfasst, und die in der
// Tabelle erwartete Art der Wirkung muss eintreten. "Kein Absturz" allein
// zaehlt nicht. Aendert ein Befehl die Karte, stellt Rueckgaengig den .map-Text
// byte-gleich wieder her und Wiederholen ebenso. Jeder Befehl laeuft ausserdem
// mit leerer Auswahl, leerer Karte und gemischter Auswahl (Pinsel + Patch +
// Entitaet).
//
// Die Pruefnamen sind fest ("Wirkung <Befehl>: ...", "Rueckgaengig <Befehl>:
// ...", "Robust <Befehl>: ...", "Taste <Text> = <Befehl>: ...", "Ereignis
// <Befehl>: ...", "Menue <Weg> = <Befehl>: ...", "Knopf <Befehl>: ...");
// tools/karte/nrc_inventar.py abdeckung ordnet sie den Eintraegen der
// NRC-Checkliste zu (ctest karte_abdeckung).
//
// BHED_KARTE_NUR=stufe6 laesst nur diese Schritte laufen; BHED_STUFE6_NUR=<Teil>
// (befehle, robust, tasten, ereignisse, menues, knoepfe) nur einen Teil.

namespace stufe6 {

namespace ke = karteneditor;
namespace kb = karte::befehle;

// --- Arten der Wirkung --------------------------------------------------------
enum Wirkung : unsigned {
    wKarte = 1U << 0,        // K: der .map-Text (karteSchreiben)
    wAuswahl = 1U << 1,      // A: gewaehlte Objekte, Flaechen, Komponenten
    wAnsicht = 1U << 2,      // V: die drei 2D-Ansichten (Sicht, Massstab, Ursprung, Hintergrund), aktive Ansicht
    wKamera = 1U << 3,       // C: Ort und Winkel der Kamera
    wRaster = 1U << 4,       // R: Rastergroesse und Einrasten
    wFilter = 1U << 5,       // F: Filtermaske, gefilterte Objekte, Filter der Plugins
    wDialog = 1U << 6,       // D: offene Fenster und Dialoge
    wHaken = 1U << 7,        // H: der Haken des Befehls (Menue/Knopf)
    wAblage = 1U << 8,       // Z: Zwischenablage (Pinsel und Flaechentextur)
    wTexturen = 1U << 9,     // T: Texturbrowser (Textur, Ordner, Filter, Tags)
    wModus = 1U << 10,       // M: Werkzeug, Auswahl- und Komponentenmodus, Clipper, Sperren, wiederholbare Transformation
    wEinstellung = 1U << 11, // E: gespeicherte Einstellungen (behaved.cfg, Abschnitt karte)
    wExtern = 1U << 12,      // X: was im Browser/als Programm geoeffnet wuerde (im Test ersetzt)
    wSicht = 1U << 13,       // S: versteckte Objekte und Ebenen
    wAnzeige = 1U << 14,     // G: Anzeige der 2D-Ansichten (Schalter, Linien der Plugins)
    wFenster = 1U << 15,     // W: Fensteranordnung, Gruppendialog, maximiert, Vollbild
    wLeck = 1U << 16,        // L: Leck-Linie, Portale
    wKonsole = 1U << 17,     // N: neue Zeilen in der Konsole
    wDatei = 1U << 18,       // P: Dateien im Testordner (Groesse, Zeit)
    wRegion = 1U << 19,      // O: Region
    wBau = 1U << 20,         // B: ein Bau laeuft / ist gelaufen
    wKlassen = 1U << 21,     // Q: Entitaetsklassen, Shader oder Modelle neu geladen
};

struct WirkungsName {
    unsigned bit;
    char kurz;
    const char* name;
};
const WirkungsName kWirkungsNamen[] = {
    {wKarte, 'K', "Karte"},       {wAuswahl, 'A', "Auswahl"},  {wAnsicht, 'V', "2D-Ansicht"}, {wKamera, 'C', "Kamera"},
    {wRaster, 'R', "Raster"},     {wFilter, 'F', "Filter"},    {wDialog, 'D', "Dialog"},      {wHaken, 'H', "Haken"},
    {wAblage, 'Z', "Ablage"},     {wTexturen, 'T', "Texturen"}, {wModus, 'M', "Modus"},       {wEinstellung, 'E', "Einstellung"},
    {wExtern, 'X', "Extern"},     {wSicht, 'S', "Sicht"},      {wAnzeige, 'G', "Anzeige"},    {wFenster, 'W', "Fenster"},
    {wLeck, 'L', "Leck/Portale"}, {wKonsole, 'N', "Konsole"},  {wDatei, 'P', "Datei"},        {wRegion, 'O', "Region"},
    {wBau, 'B', "Bau"},           {wKlassen, 'Q', "Neu geladen"},
};

unsigned maskeAus(const char* kurz) {
    unsigned m = 0;
    for (const char* c = kurz; *c != 0; ++c) {
        for (const WirkungsName& w : kWirkungsNamen) {
            if (w.kurz == *c) { m |= w.bit; }
        }
    }
    return m;
}

std::string maskeText(unsigned m) {
    std::string s;
    for (const WirkungsName& w : kWirkungsNamen) {
        if ((m & w.bit) != 0U) { s += (s.empty() ? "" : "+") + std::string(w.name); }
    }
    return s.empty() ? std::string("nichts") : s;
}

std::string maskeKurz(unsigned m) {
    std::string s;
    for (const WirkungsName& w : kWirkungsNamen) {
        if ((m & w.bit) != 0U) { s += w.kurz; }
    }
    return s.empty() ? std::string("-") : s;
}

// --- Testkarten ------------------------------------------------------------------

std::string zahl(double f) {
    char z[32];
    std::snprintf(z, sizeof(z), "%.3f", f);
    return z;
}

std::string g(double v) {
    char z[32];
    std::snprintf(z, sizeof(z), "%g", v);
    return z;
}

// Ein Quader im Quake-3-Format (Ebenen wie stufe4::kastenText), mit Inhalts-
// und Flaechen-Flags.
std::string quader(double x0, double y0, double z0, double x1, double y1, double z1, const char* tex, int inhalt = 0, int flaeche = 0) {
    const std::string t = std::string(" ") + tex + " 0 0 0 0.5 0.5 " + std::to_string(inhalt) + " " + std::to_string(flaeche) + " 0\n";
    const auto p = [](double a, double b, double c) { return "( " + g(a) + " " + g(b) + " " + g(c) + " ) "; };
    return "{\n" + p(x1, y1, z1) + p(x1, y0, z1) + p(x0, y1, z1) + t + p(x1, y1, z1) + p(x0, y1, z1) + p(x1, y1, z0) + t + p(x1, y1, z1) +
           p(x1, y1, z0) + p(x1, y0, z1) + t + p(x0, y0, z0) + p(x1, y0, z0) + p(x0, y1, z0) + t + p(x0, y0, z0) + p(x0, y0, z1) +
           p(x1, y0, z0) + t + p(x0, y0, z0) + p(x0, y1, z0) + p(x0, y0, z1) + t + "}\n";
}

// patchDef2 n x n ab (x0, y0) mit Abstand d; schief: die inneren Punkte versetzt
std::string ebene(double x0, double y0, double d, int n, bool schief) {
    std::string t = "{\npatchDef2\n{\ncommon/caulk\n( " + std::to_string(n) + " " + std::to_string(n) + " 0 0 0 )\n(\n";
    for (int i = 0; i < n; ++i) {
        t += "( ";
        for (int j = 0; j < n; ++j) {
            double x = x0 + (i * d);
            double y = y0 + (j * d);
            double z = 0;
            if (schief && i > 0 && i < n - 1) {
                x += (i * i * 3.0) - 6.0;
                z = j * 4.0;
            }
            if (schief && j > 0 && j < n - 1) { y += (j * j * 3.0) - 6.0; }
            t += "( " + g(x) + " " + g(y) + " " + g(z) + " " + g(i * 0.25) + " " + g(j * 0.25) + " ) ";
        }
        t += ")\n";
    }
    return t + ")\n}\n}\n";
}

// Die Testkarte (s6_vorlage.map, je Befehl frisch nach s6_wirkung.map kopiert):
// worldspawn (Reihenfolge = Index in welt()->kinder()):
//   0 A  -64..64 x -64..64 x 0..64        6 G  400..464 x 0..64 x 0..64 (Merge mit H)
//   1 B  32..128 x 32..128 x 32..96        7 H  464..528 x 0..64 x 0..64
//   2 C  neben dem Raster                  8 P3 5 x 5-Patch, schief (Redisperse, Smooth, Delete)
//   3 D  Detail                            9..12 Pinsel mit den UFO:AI-Flags stepon, actorclip, weaponclip, nodraw
//   4 P1 3 x 3-Ebene                       13 K  128 x 128 x 256 (PitOMatic)
//   5 P2 3 x 3-Ebene neben P1
// dazu zwei Lichter (das erste mit targetname/target/_color 2 1 1), ein
// target_position (p1), info_player_start, func_group (zwei Pinsel), func_door,
// eine Kette aus info_train_spline_main/_control (Plot Splines).
std::string vorlageText() {
    const char* c = "common/caulk";   // im .map-Text ohne "textures/" (Quake-3-Format)
    std::string w = "{\n\"classname\" \"worldspawn\"\n\"message\" \"stufe6\"\n";
    w += quader(-64, -64, 0, 64, 64, 64, c);
    w += quader(32, 32, 32, 128, 128, 96, c);
    w += quader(200.25, 0, 0, 264.5, 64, 64, c);
    w += quader(300, 0, 0, 364, 64, 64, c, 0x8000000);
    w += ebene(-64, -200, 64, 3, false);
    w += ebene(64, -200, 64, 3, false);
    w += quader(400, 0, 0, 464, 64, 64, c);
    w += quader(464, 0, 0, 528, 64, 64, c);
    w += ebene(-64, -600, 32, 5, true);
    w += quader(600, 0, 0, 632, 32, 32, c, karte::plugins::kContentsStepon);
    w += quader(640, 0, 0, 672, 32, 32, c, karte::plugins::kContentsActorclip);
    w += quader(680, 0, 0, 712, 32, 32, c, karte::plugins::kContentsWeaponclip);
    w += quader(720, 0, 0, 752, 32, 32, c, 0, karte::plugins::kSurfNodraw);
    w += quader(800, 0, 0, 928, 128, 256, c);
    w += "}\n";
    w += "{\n\"classname\" \"light\"\n\"origin\" \"0 0 128\"\n\"light\" \"300\"\n\"_color\" \"2 1 1\"\n\"targetname\" \"t1\"\n\"target\" \"p1\"\n}\n";
    w += "{\n\"classname\" \"light\"\n\"origin\" \"0 0 400\"\n\"light\" \"200\"\n}\n";
    w += "{\n\"classname\" \"target_position\"\n\"origin\" \"0 0 256\"\n\"targetname\" \"p1\"\n}\n";
    w += "{\n\"classname\" \"info_player_start\"\n\"origin\" \"-160 0 24\"\n}\n";
    w += "{\n\"classname\" \"func_group\"\n" + quader(-300, 0, 0, -236, 64, 64, c) + quader(-300, 80, 0, -236, 144, 64, c) + "}\n";
    w += "{\n\"classname\" \"func_door\"\n" + quader(-300, 200, 0, -236, 264, 64, c) + "}\n";
    w += "{\n\"classname\" \"info_train_spline_main\"\n\"origin\" \"0 600 0\"\n\"targetname\" \"s6_pt0\"\n\"target\" \"s6_pt1\"\n\"control\" "
         "\"s6_ctl0\"\n}\n";
    w += "{\n\"classname\" \"info_train_spline_main\"\n\"origin\" \"0 664 0\"\n\"targetname\" \"s6_pt1\"\n}\n";
    w += "{\n\"classname\" \"info_train_spline_control\"\n\"origin\" \"32 632 0\"\n\"targetname\" \"s6_ctl0\"\n}\n";
    return w;
}

std::string fixturePfad() { return karteTestPfad("s6_wirkung.map"); }
std::string vorlagePfad() { return karteTestPfad("s6_vorlage.map"); }
std::string zweitePfad() { return karteTestPfad("s6_zweite.map"); }

bool textSchreiben(const std::string& pfad, const std::string& text) {
    std::ofstream f(std::filesystem::u8path(pfad), std::ios::binary | std::ios::trunc);
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(f);
}

// Antwort des ersetzten Dateidialogs (kartenDateiDialog) fuer den naechsten Befehl
std::string& dialogAntwort() {
    static std::string s;
    return s;
}
// Was im Test "geoeffnet" worden waere (kartenUrlOeffnen, kartenBefehlStarten)
std::string& externProtokoll() {
    static std::string s;
    return s;
}

void fixtureSchreiben() {
    (void)textSchreiben(vorlagePfad(), vorlageText());
    (void)textSchreiben(zweitePfad(), "{\n\"classname\" \"worldspawn\"\n" + quader(1024, 1024, 0, 1088, 1088, 64, "common/caulk") + "}\n");
    for (int i = 1; i <= 9; ++i) {   // fuer File > 1..9 (MRU)
        (void)textSchreiben(karteTestPfad(("s6_mru" + std::to_string(i) + ".map").c_str()),
                            "{\n\"classname\" \"worldspawn\"\n\"message\" \"mru" + std::to_string(i) + "\"\n" +
                                quader(1024, 1024, 0, 1088, 1088, 64 * i, "common/caulk") + "}\n");
    }
    // Ein Bild fuer Set 2D Background image (TGA 2 x 2, 24 Bit, unkomprimiert)
    std::string bild(18, '\0');
    bild[2] = 2;
    bild[12] = 2;
    bild[14] = 2;
    bild[16] = 24;
    bild += std::string("\0\0\xff\0\xff\0\xff\0\0\xff\xff\xff", 12);
    (void)textSchreiben(karteTestPfad("s6_bild.tga"), bild);
}

std::shared_ptr<karte::Entitaet> entMit(const std::string& klasse, std::size_t nr = 0) {
    for (const auto& ent : ke::editor().karte.entitaeten()) {
        if (ent->klassenname() == klasse && nr-- == 0) { return ent; }
    }
    return nullptr;
}

std::shared_ptr<karte::Primitiv> weltKind(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    return (w && i < w->kinder().size()) ? w->kinder()[i] : nullptr;
}

// --- Vorbedingungen ---------------------------------------------------------------
enum class Vor {
    Leer,           // nichts gewaehlt
    Pinsel,         // A
    NurB,           // B (ueberlappt A: CSG Subtract)
    Zwei,           // A und B
    Paar,           // G und H (nebeneinander: CSG Merge)
    Patch,          // P1
    Patch5,         // P3 (5 x 5, schief)
    PatchVersetzt,  // P1 mit verschobener und gestreckter Textur (MeshTex)
    ZweiPatch,      // P1 und P2
    Licht,          // das erste Licht
    Flaeche,        // die obere Flaeche von A (Flaechenmodus)
    Misch,          // A, P1 und das erste Licht
    Gruppe,         // ein Pinsel der func_group
    GruppeEnt,      // die func_group (Entitaet)
    TuerPinsel,     // der Pinsel der func_door
    PinselTuer,     // A, dann die func_door (Move Primitives to Entity: letzte)
    TuerPinsel2,    // die func_door, dann A (Move Primitives to Entity: erste)
    ZweiEnt,        // target_position und info_player_start (nicht verbunden)
    Aenderung,      // A gewaehlt und verschoben (ein Rueckgaengig-Schritt)
    Rueck,          // wie Aenderung, danach Rueckgaengig
    Ablage,         // A gewaehlt und kopiert
    Schief,         // C (neben dem Raster)
    Detail,         // D (detail)
    PinselVersetzt, // A mit verschobener Textur (Reset)
    Lin,            // die Leck-Linie (s6_wirkung.lin) ist geladen und sichtbar
    LinWeiter,      // wie Lin, einen Punkt weiter (Prev)
    Clipper,        // Clipper an, zwei Punkte durch A
    Versteckt,      // A versteckt
    Filter,         // Filter "World" an
    Region,         // Region um A
    Transformiert,  // A gewaehlt und um 90 Grad gedreht (Repeat/Reset Transforms)
    Ecke,           // Eckenmodus, eine Ecke von A gewaehlt
    LeereKarte,     // File > New Map
    Geaendert,      // die Karte ist geaendert und hat einen Namen (Save)
    Raster16,       // Raster 16 (SetGrid8)
    Gezoomt,        // XY-Ansicht im Massstab 2 (Zoom100)
    KameraUnten,    // Kamera unter der Karte (UpFloor)
    Verschieben,    // Werkzeug Translate (MouseDrag)
    TexturGewaehlt, // im Texturbrowser common/caulk gewaehlt, nichts in der Karte (Select Textured)
    Tags,           // common/caulk gewaehlt, Tag "s6tag" daran und markiert
    TagsKopiert,    // wie Tags, dazu "s6neu" kopiert (Paste)
    Prt,            // eine Portaldatei geladen (s6_wirkung.prt)
    Pit,            // der hohe Kasten K (PitOMatic)
    Terrain,        // zwei Terrain-Dreiecke (Flip terrain, wie Stufe 4)
    Freelook,       // Freelook an
    UfoFilter,      // UFO:AI: StepOn, ActorClip, WeaponClip und NoDraw an (zum Zurueckschalten)
    Doppelt,        // A und eine Kopie von A (Find Duplicates)
    LichtPlanter,   // das erste Licht gewaehlt, Tree Planter an (Drop Entity)
    OhneTagDatei,   // die Tag-Datei geloescht (ShaderPlug Create tag file)
    TexturSeite,    // der Gruppendialog zeigt den Texturbrowser (seine Knoepfe)
};

const char* vorName(Vor v) {
    switch (v) {
    case Vor::Leer: return "leer";
    case Vor::Pinsel: return "Pinsel";
    case Vor::NurB: return "Pinsel B";
    case Vor::Zwei: return "zwei Pinsel";
    case Vor::Paar: return "zwei Pinsel nebeneinander";
    case Vor::Patch: return "Patch";
    case Vor::Patch5: return "Patch 5x5";
    case Vor::PatchVersetzt: return "Patch, Textur versetzt";
    case Vor::ZweiPatch: return "zwei Patches";
    case Vor::Licht: return "Licht";
    case Vor::Flaeche: return "Flaeche";
    case Vor::Misch: return "Pinsel+Patch+Licht";
    case Vor::Gruppe: return "Pinsel der func_group";
    case Vor::GruppeEnt: return "func_group";
    case Vor::TuerPinsel: return "Pinsel der func_door";
    case Vor::PinselTuer: return "Pinsel, dann func_door";
    case Vor::TuerPinsel2: return "func_door, dann Pinsel";
    case Vor::ZweiEnt: return "zwei Entitaeten";
    case Vor::Aenderung: return "nach einer Aenderung";
    case Vor::Rueck: return "nach Rueckgaengig";
    case Vor::Ablage: return "Pinsel kopiert";
    case Vor::Schief: return "Pinsel neben dem Raster";
    case Vor::Detail: return "Detail-Pinsel";
    case Vor::PinselVersetzt: return "Pinsel, Textur versetzt";
    case Vor::Lin: return "Leck-Linie geladen";
    case Vor::LinWeiter: return "Leck-Linie, ein Punkt weiter";
    case Vor::Clipper: return "Clipper mit zwei Punkten";
    case Vor::Versteckt: return "Pinsel versteckt";
    case Vor::Filter: return "Filter an";
    case Vor::Region: return "Region an";
    case Vor::Transformiert: return "Pinsel gedreht";
    case Vor::Ecke: return "Ecke gewaehlt";
    case Vor::LeereKarte: return "leere Karte";
    case Vor::Geaendert: return "Karte geaendert";
    case Vor::Raster16: return "Raster 16";
    case Vor::Gezoomt: return "2D gezoomt";
    case Vor::KameraUnten: return "Kamera unten";
    case Vor::Verschieben: return "Werkzeug Translate";
    case Vor::TexturGewaehlt: return "Textur gewaehlt";
    case Vor::Tags: return "Tag markiert";
    case Vor::TagsKopiert: return "Tag kopiert";
    case Vor::Prt: return "Portale geladen";
    case Vor::Pit: return "hoher Kasten";
    case Vor::Terrain: return "Terrain";
    case Vor::Freelook: return "Freelook";
    case Vor::UfoFilter: return "UFO:AI-Filter an";
    case Vor::Doppelt: return "Pinsel doppelt";
    case Vor::LichtPlanter: return "Licht, Tree Planter an";
    case Vor::OhneTagDatei: return "ohne Tag-Datei";
    case Vor::TexturSeite: return "Texturbrowser offen";
    }
    return "?";
}

// Die Kamera-Einstellungen, die Anzeige und die Werkzeugleiste vom Anfang (am Ende zurueck)
struct Anfang {
    karte::KameraEinstellungen kameraE;
    std::set<std::string> versteckteKnoepfe;
    std::array<bool, 4> schwebend{};
    bool gemerkt = false;
};
Anfang& anfang() {
    static Anfang a;
    return a;
}

// Grundzustand: die Testkarte frisch kopiert und geladen, alles Uebrige auf Vorgabe.
void grundzustand() {
    ke::Editor& e = ke::editor();
    karteDialogeZu();
    e.siOffen = false;
    e.csgWerkzeug.offen = false;
    if (ke::optimiererOffen()) { ke::optimiererSchliessen(); }
    if (e.kamFenster.frei) { ke::freelookBeenden(); }
    if (platform::istVollbild()) { (void)platform::vollbildUmschalten(); }
    if (e.maximiert) { ke::maximierenUmschalten(); }   // vor dem Neuaufbau der Teiler (teilerAufbauen setzt maximiert zurueck)
    e.anordnung = karte::befehle::Anordnung::Regular;
    e.teilerFuer = -1;
    e.anzeige = ke::Anzeige{};
    if (anfang().gemerkt) {
        e.kameraE = anfang().kameraE;
        e.schwebendSichtbar = anfang().schwebend;   // ToggleCamera/ToggleView ... schalten die Fenster
    }
    e.gruppenDialogOffen = false;
    e.gruppenSeite = 0;
    e.filterMaske = 0;
    e.ufoai = karte::plugins::UfoaiZustand{};
    e.bob.splines = karte::bobtoolz::Splines{};
    if (e.bob.planter.aktiv) { (void)ke::ausfuehren("bobToolz::TreePlanter"); }
    e.portale.clear();
    e.texBro.shader = karte::kTexturPraefix;
    e.tags.markiert.clear();
    e.tags.kopiert.clear();
    e.zwischenablage.clear();
    e.letzteHilfe.clear();
    e.letzteUrl.clear();
    e.drehDialog.offen = e.drehDialog.zeigen = false;
    e.skalierDialog.offen = e.skalierDialog.zeigen = false;
    e.seitenDialog.offen = e.seitenDialog.oeffnen = false;
    e.patchDialog.art = 0;
    e.patchDialog.oeffnen = false;
    e.pinselSuche.offen = e.pinselSuche.oeffnen = false;
    e.kartenInfo.offen = e.kartenInfo.oeffnen = false;
    e.schriftDialogOffen = false;
    e.brushExport.offen = e.brushExport.oeffnen = false;
    e.bob.dialog = 0;
    e.bob.oeffnen = e.bob.visOeffnen = false;
    e.meshtex.getInfoOffen = e.meshtex.setScaleOffen = e.meshtex.genFuncOffen = false;
    for (int i = 0; i < 3; ++i) { ke::hintergrundBildFrei(i); }
    (void)textSchreiben(fixturePfad(), dateiInhalt(vorlagePfad()));
    (void)textSchreiben(karteTestPfad("s6_wirkung.lin"), "0 0 32\n200 0 32\n200 200 32\n");
    (void)ke::karteOeffnen(fixturePfad());
    e.karte.auswahl.leeren();
    e.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;
    e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
    if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
    e.bearbeitung.transformZuruecksetzen(-1);
    e.aktivXy = 0;
    e.xy[0].ansicht.typ = karte::Sicht::XY;
    e.xy[0].ansicht.setzeMassstab(1);
    e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
    e.raster.setzePotenz(3);
    if (!e.raster.einrasten()) { e.raster.einrastenUmschalten(); }
    e.kamera.setzeUrsprung(karte::Vektor3f(0, -400, 160));
    e.kamera.setzeWinkel(karte::Vektor3f(-15, 90, 0));
    e.kamera.projektionAktualisieren(e.kameraE);
    ke::tastaturErsatz = nullptr;
    ke::auswahlGeaendert();
}

void vorbereiten(Vor v) {
    ke::Editor& e = ke::editor();
    grundzustand();
    const auto welt = e.karte.welt();
    const auto waehle = [&](std::size_t i) { e.karte.auswahl.waehlen(welt, weltKind(i)); };
    switch (v) {
    case Vor::Leer: break;
    case Vor::Pinsel: waehle(0); break;
    case Vor::NurB: waehle(1); break;
    case Vor::Zwei:
        waehle(0);
        waehle(1);
        break;
    case Vor::Paar:
        waehle(6);
        waehle(7);
        break;
    case Vor::Patch: waehle(4); break;
    case Vor::Patch5: waehle(8); break;
    case Vor::PatchVersetzt: {
        waehle(4);
        if (const auto p = std::dynamic_pointer_cast<karte::Patch>(weltKind(4))) {
            p->texturVerschieben(0.3F, 0.4F, e.karte.kontext);
            p->texturSkalieren(1.15F, 1.15F);
        }
        break;
    }
    case Vor::ZweiPatch:
        waehle(4);
        waehle(5);
        break;
    case Vor::Licht: e.karte.auswahl.waehlen(entMit("light")); break;
    case Vor::Flaeche: {
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Komponente;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Flaeche;
        if (const auto p = std::dynamic_pointer_cast<karte::Pinsel>(weltKind(0))) {
            for (const auto& f : p->flaechen()) {
                if (f->ebene().normal().z() > 0.5) { e.karte.auswahl.flaecheWaehlen(p, f); }
            }
        }
        break;
    }
    case Vor::Misch:
        waehle(0);
        waehle(4);
        e.karte.auswahl.waehlen(entMit("light"));
        break;
    case Vor::Gruppe: {
        const auto t = entMit("func_group");
        if (t && !t->kinder().empty()) { e.karte.auswahl.waehlen(t, t->kinder().front()); }
        break;
    }
    case Vor::GruppeEnt: e.karte.auswahl.waehlen(entMit("func_group")); break;
    case Vor::TuerPinsel: {
        const auto t = entMit("func_door");
        if (t && !t->kinder().empty()) { e.karte.auswahl.waehlen(t, t->kinder().front()); }
        break;
    }
    case Vor::PinselTuer:
        waehle(0);
        e.karte.auswahl.waehlen(entMit("func_door"));
        break;
    case Vor::TuerPinsel2:
        e.karte.auswahl.waehlen(entMit("func_door"));
        waehle(0);
        break;
    case Vor::ZweiEnt:
        e.karte.auswahl.waehlen(entMit("target_position"));
        e.karte.auswahl.waehlen(entMit("info_player_start"));
        break;
    case Vor::Aenderung:
    case Vor::Geaendert:
        waehle(0);
        (void)ke::ausfuehren("SelectNudgeRight");
        break;
    case Vor::Rueck:
        waehle(0);
        (void)ke::ausfuehren("SelectNudgeRight");
        (void)ke::ausfuehren("Undo");
        break;
    case Vor::Ablage:
        waehle(0);
        (void)ke::ausfuehren("Copy");
        break;
    case Vor::Schief: waehle(2); break;
    case Vor::Detail: waehle(3); break;
    case Vor::PinselVersetzt:
        waehle(0);
        (void)ke::ausfuehren("TexShiftLeft");
        (void)ke::ausfuehren("TexRotateClock");
        break;
    case Vor::Lin: ke::punktdateiUmschalten(); break;
    case Vor::LinWeiter:
        ke::punktdateiUmschalten();
        ke::leckWeiter();
        break;
    case Vor::Clipper:
        waehle(0);
        (void)ke::ausfuehren("ToggleClipper");
        e.bearbeitung.clipperPunkte.punkte[0] = karte::Vektor3d(-80, 0, 0);
        e.bearbeitung.clipperPunkte.punkte[1] = karte::Vektor3d(80, 16, 0);
        e.bearbeitung.clipperPunkte.anzahl = 2;
        break;
    case Vor::Versteckt:
        waehle(0);
        (void)ke::ausfuehren("HideSelected");
        break;
    case Vor::Filter: (void)ke::ausfuehren("FilterWorldBrushes"); break;
    case Vor::Region:
        waehle(0);
        (void)ke::ausfuehren("RegionSetSelection");
        e.karte.auswahl.leeren();
        break;
    case Vor::Transformiert:
        waehle(0);
        (void)ke::ausfuehren("RotateSelectionZ");
        break;
    case Vor::Ecke: {
        waehle(0);
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Komponente;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Ecke;
        if (const auto p = std::dynamic_pointer_cast<karte::Pinsel>(weltKind(0))) {
            (void)p->ecken(e.karte.kontext);
            karte::Waehlbar w;
            w.art = karte::Waehlbar::Art::Ecke;
            w.entitaet = welt;
            w.primitiv = p;
            w.index = 0;
            karte::setzeGewaehlt(e.karte, w, true);
        }
        break;
    }
    case Vor::LeereKarte: ke::neueKarte(); break;
    case Vor::Raster16: e.raster.setzePotenz(4); break;
    case Vor::Gezoomt: e.xy[0].ansicht.setzeMassstab(2); break;
    case Vor::KameraUnten: e.kamera.setzeUrsprung(karte::Vektor3f(0, 0, -300)); break;
    case Vor::Verschieben: (void)ke::ausfuehren("MouseTranslate"); break;
    case Vor::TexturGewaehlt: ke::texturShaderWaehlen("textures/common/caulk"); break;   // wie ein Klick im Texturbrowser
    case Vor::Tags:
    case Vor::TagsKopiert:
        ke::texturShaderWaehlen("textures/common/caulk");
        e.tags.kopiert = {"s6tag"};
        ke::tagBefehl("TagPaste");
        e.tags.alle.insert("s6tag");
        e.tags.markiert = {"s6tag"};
        e.tags.kopiert.clear();
        if (v == Vor::TagsKopiert) { e.tags.kopiert = {"s6neu"}; }
        break;
    case Vor::Prt:
        (void)textSchreiben(karteTestPfad("s6_wirkung.prt"),
                            "PRT1\n2\n1\n4 0 1 (0 0 0 ) (0 64 0 ) (0 64 64 ) (0 0 64 )\n");
        {
            e.portalDatei = karteTestPfad("s6_wirkung.prt");
            const std::string text = dateiInhalt(e.portalDatei);
            karte::PortalZufall zufall;
            (void)karte::portaleLaden(e.portalDatei, &text, e.portale, e.portalHintFlags, zufall);   // CPortals::Load
        }
        break;
    case Vor::Pit: waehle(13); break;
    case Vor::Terrain: {
        using P3 = stufe4::P3;
        const P3 a{0, 0, 0};
        const P3 b{64, 0, 16};
        const P3 c{64, 64, 0};
        const P3 d{0, 64, 16};
        const std::string p1 = stufe4::ebenenPinsel({{P3{0, 0, -16}, P3{64, 0, -16}, P3{0, 64, -16}},
                                                     {P3{0, 0, 0}, P3{64, 0, 0}, P3{0, 0, 16}},
                                                     {P3{64, 0, 0}, P3{64, 64, 0}, P3{64, 0, 16}},
                                                     {P3{0, 0, 0}, P3{64, 64, 0}, P3{0, 0, 16}},
                                                     {a, b, c}},
                                                    P3{40, 20, -8}, "test/erde");
        const std::string p2 = stufe4::ebenenPinsel({{P3{0, 0, -16}, P3{64, 0, -16}, P3{0, 64, -16}},
                                                     {P3{0, 64, 0}, P3{64, 64, 0}, P3{0, 64, 16}},
                                                     {P3{0, 0, 0}, P3{0, 64, 0}, P3{0, 0, 16}},
                                                     {P3{0, 0, 0}, P3{64, 64, 0}, P3{0, 0, 16}},
                                                     {a, c, d}},
                                                    P3{20, 40, -8}, "test/erde");
        (void)textSchreiben(fixturePfad(), stufe4::weltText(p1 + p2));
        (void)ke::karteOeffnen(fixturePfad());
        e.karte.auswahl.waehlen(e.karte.welt(), weltKind(0));
        e.karte.auswahl.waehlen(e.karte.welt(), weltKind(1));
        break;
    }
    case Vor::Freelook: e.kamFenster.frei = true; break;
    case Vor::UfoFilter:
        e.ufoai.stepon = e.ufoai.actorclip = e.ufoai.weaponclip = e.ufoai.nodraw = true;
        break;
    case Vor::LichtPlanter:   // Tree Planter laedt die gewaehlten Pinsel als Boden (DTreePlanter::LoadSelectedBrushes)
        waehle(0);
        (void)ke::ausfuehren("bobToolz::TreePlanter");
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(entMit("light"));
        break;
    case Vor::OhneTagDatei: {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::u8path(ke::tagDatei()), ec);
        break;
    }
    case Vor::TexturSeite:
        e.gruppenSeite = 2;
        e.gruppenSeiteWechseln = true;
        e.gruppenDialogOffen = true;
        break;
    case Vor::Doppelt: {
        std::string t = vorlageText();
        const std::size_t p = t.find("{\n(");
        const std::size_t q = t.find("}\n", p) + 2;
        t.insert(q, t.substr(p, q - p));
        (void)textSchreiben(fixturePfad(), t);
        (void)ke::karteOeffnen(fixturePfad());
        break;
    }
    }
    ke::auswahlGeaendert();
}

// --- Zustand -------------------------------------------------------------------

std::string vek(const karte::Vektor3f& v) { return zahl(v[0]) + " " + zahl(v[1]) + " " + zahl(v[2]); }

struct Zustand {
    std::string karte, auswahl, ansicht, kamera, raster, filter, dialog, ablage, texturen, modus, einst, extern_, sicht, anzeige,
        fenster, leck, datei, region, bau, klassen;
    std::size_t konsole = 0;
};

std::size_t entIndex(const karte::Entitaet* x) {
    std::size_t i = 0;
    for (const auto& ent : ke::editor().karte.entitaeten()) {
        if (ent.get() == x) { return i; }
        ++i;
    }
    return static_cast<std::size_t>(-1);
}

std::size_t primIndex(const karte::Entitaet* ent, const karte::Primitiv* p) {
    if (ent == nullptr || p == nullptr) { return static_cast<std::size_t>(-1); }
    std::size_t i = 0;
    for (const auto& k : ent->kinder()) {
        if (k.get() == p) { return i; }
        ++i;
    }
    return static_cast<std::size_t>(-1);
}

std::string offeneFenster() {
    ke::Editor& e = ke::editor();
    std::vector<std::string> v;
    for (ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive || w->Hidden) { continue; }
        if ((w->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) != 0) { continue; }
        const std::string n = w->Name;
        if (n.find("##Menu_") != std::string::npos || n.find("Debug##") != std::string::npos) { continue; }
        v.push_back(n.substr(n.find("###") != std::string::npos ? n.find("###") : 0));
    }
    std::sort(v.begin(), v.end());
    std::string s;
    for (const std::string& n : v) { s += n + "|"; }
    const auto f = [&s](const char* n, bool b) {
        if (b) { s += std::string(n) + "|"; }
    };
    f("einstellungen", e.einstellungenOffen);
    f("tasten", e.tastenDialogOffen);
    f("finden", e.findenOffen);
    f("shaderinfo", e.shaderInfoOffen);
    f("shadertext", e.shaderText.offen);
    f("ueber", e.ueberOffen);
    f("entliste", e.ent.listeOffen);
    f("farbe", e.ent.farbDialog.offen);
    f("licht", e.ent.lichtOffen);
    f("baudialog", e.bauDialogOffen);
    f("prtladen", e.portalLadenOffen);
    f("prtkonfig", e.portalKonfigOffen);
    f("rueckmeldung", e.bauRueckmeldungOffen);
    f("meldung", e.meldung.offen);
    f("frage", e.frageOffen);
    f("pinselsuche", e.pinselSuche.offen || e.pinselSuche.oeffnen);
    f("karteninfo", e.kartenInfo.offen || e.kartenInfo.oeffnen);
    f("seiten", e.seitenDialog.offen || e.seitenDialog.oeffnen);
    f("drehen", e.drehDialog.offen || e.drehDialog.zeigen);
    f("skalieren", e.skalierDialog.offen || e.skalierDialog.zeigen);
    f("patchdialog", e.patchDialog.art != 0 || e.patchDialog.oeffnen);
    f("si", e.siOffen);
    f("csg", e.csgWerkzeug.offen);
    f("schrift", e.schriftDialogOffen);
    f("enginepfad", e.enginePfadDialogOffen);
    f("eingabe", e.eingabe.offen || e.eingabe.oeffnen);
    f("optimierer", ke::optimiererOffen());
    f("bsp", ke::bspDialogOffen());
    f("ebenenmenue", e.ebenenBro.menueOffen || e.ebenenBro.menueOeffnen);
    f("bob", e.bob.dialog != 0 || e.bob.oeffnen || e.bob.visOeffnen);
    f("meshtex", e.meshtex.getInfoOffen || e.meshtex.setScaleOffen || e.meshtex.genFuncOffen);
    f("popup", GImGui->OpenPopupStack.Size > 0);
    // Zustand in den Dialogen (Sortierung, Werte, Haken)
    const ke::Editor::CsgWerkzeug& c = e.csgWerkzeug;
    s += "csg " + zahl(c.dicke) + " " + std::to_string(c.ausschluss) + (c.caulk ? "c" : "-") + (c.innenWeg ? "i" : "-") + "|info " +
         std::to_string(e.kartenInfo.sortSpalte) + (e.kartenInfo.absteigend ? "a" : "-") + "|taste " + e.tastenDialogBearbeitet + "|finden " +
         (e.findenNurAuswahl ? "a" : "-") + "|meldung " + e.meldung.titel + "|seiten " + (e.seitenDialog.stutzen ? "s" : "-") + "|patch " +
         (e.patchDialog.quadrat ? "q" : "-") + (e.patchDialog.waende ? "w" : "-") + "|liste " + (e.ent.listeAutoFokus ? "f" : "-") + "|licht " +
         (e.ent.lichtNichtZeigen ? "n" : "-") + "|si " + (e.siWerte.rasterAnpassen ? "r" : "-");
    return s;
}

std::string dateienImTestordner() {
    std::vector<std::string> v;
    std::error_code ec;
    const auto eintrag = [&v](const std::filesystem::path& p) {
        std::error_code ec2;
        if (!std::filesystem::is_regular_file(p, ec2)) { return; }
        const auto groesse = std::filesystem::file_size(p, ec2);
        const auto zeit = std::filesystem::last_write_time(p, ec2).time_since_epoch().count();
        v.push_back(p.filename().string() + ":" + std::to_string(groesse) + ":" + std::to_string(zeit));
    };
    for (const auto& d : std::filesystem::directory_iterator(std::filesystem::current_path(), ec)) {
        const std::string n = d.path().filename().string();
        if ((n.rfind("s6_", 0) == 0 && n != "s6_wirkung.lin" && n != "s6_wirkung.prt") || n.find(".obj") != std::string::npos ||
            n.find(".mtl") != std::string::npos) {
            eintrag(d.path());
        }
    }
    eintrag(std::filesystem::u8path(ke::tagDatei()));
    std::sort(v.begin(), v.end());
    std::string s;
    for (const std::string& x : v) { s += x + "|"; }
    return s;
}

Zustand erfassen() {
    ke::Editor& e = ke::editor();
    Zustand z;
    z.karte = karte::karteSchreiben(e.karte);
    {
        std::string a;
        for (const karte::AuswahlElement& el : e.karte.auswahl.elemente()) {
            const auto ent = el.entitaet.lock();
            const auto p = el.primitiv.lock();
            a += std::to_string(entIndex(ent.get())) + "/" + std::to_string(primIndex(ent.get(), p.get())) + ";";
        }
        a += "|";
        for (const karte::FlaechenWahl& f : e.karte.auswahl.flaechen()) {
            const auto p = f.pinsel.lock();
            const auto fl = f.flaeche.lock();
            std::size_t i = 0;
            if (p && fl) {
                for (const auto& x : p->flaechen()) {
                    if (x == fl) { break; }
                    ++i;
                }
            }
            a += std::to_string(i) + ";";
        }
        a += "|" + std::to_string(ke::komponentenZeichnung(e.blicke[0]).gewaehlt.size());
        z.auswahl = a;
    }
    for (const ke::XyFenster& f : e.xy) {
        z.ansicht += std::to_string(static_cast<int>(f.ansicht.typ)) + " " + zahl(f.ansicht.massstab) + " " + vek(f.ansicht.ursprung) + " " +
                     f.hintergrund.datei + "|";
    }
    z.ansicht += std::to_string(e.aktivXy);
    z.kamera = vek(e.kamera.ursprung) + "|" + vek(e.kamera.winkel);
    z.raster = std::to_string(e.raster.potenz()) + (e.raster.einrasten() ? "e" : "-");
    z.filter = std::to_string(e.filterMaske) + " " + (e.ufoai.stepon ? "s" : "-") + (e.ufoai.actorclip ? "a" : "-") + (e.ufoai.weaponclip ? "w" : "-") +
               (e.ufoai.nodraw ? "n" : "-") + std::to_string(e.ufoai.level) + " ";
    for (const auto& ent : e.karte.entitaeten()) {
        for (const auto& p : ent->kinder()) { z.filter += p->gefiltert ? "1" : "0"; }
    }
    z.dialog = offeneFenster();
    {
        const karte::FlaechenTextur& t = e.texturAblage;
        const karte::Texdef& d = t.projektion.texdef;
        z.ablage = std::to_string(std::hash<std::string>{}(e.zwischenablage)) + "|" + zahl(t.ebene.a) + " " + zahl(t.ebene.b) + " " + zahl(t.ebene.c) +
                   " " + zahl(t.ebene.d) + "|" + zahl(d.shift[0]) + " " + zahl(d.shift[1]) + " " + zahl(d.rotate) + " " + zahl(d.scale[0]) + " " +
                   zahl(d.scale[1]) + "|" + std::to_string(t.breite) + "x" + std::to_string(t.hoehe) + "|" + std::to_string(t.patchPunkte.size());
    }
    {
        const karte::TexturBrowser& tb = e.texBro;
        z.texturen = tb.shader + "|" + tb.verzeichnis + "|" + tb.filter + "|" + (tb.unbenutzteVerstecken ? "u" : "-") + (tb.tagSuche ? "t" : "-") +
                     std::to_string(tb.gefundeneShader.size()) + "|" + e.texFilterText + "|";
        const ke::TagZustand& t = e.tags;
        z.texturen += std::string(t.gui ? "g" : "-") + std::to_string(t.alle.size()) + ":";
        for (const std::string& x : t.markiert) { z.texturen += x + ","; }
        z.texturen += ":";
        for (const std::string& x : t.kopiert) { z.texturen += x + ","; }
        z.texturen += ":" + t.bearbeitet + ":" + std::to_string(t.gefunden.size()) + ":";
        for (const std::string& x : t.haken) { z.texturen += x + ","; }
    }
    z.modus = std::to_string(static_cast<int>(e.bearbeitung.modus())) + " " + std::to_string(static_cast<int>(e.bearbeitung.einstellung.modus)) + " " +
              std::to_string(static_cast<int>(e.bearbeitung.einstellung.komponente)) + " " + (e.bearbeitung.clipperUmgedreht ? "u" : "-") + " " +
              std::to_string(e.bearbeitung.clipperPunkte.anzahl) + " " + (e.karte.kontext.texturLock ? "t" : "-") +
              (e.bearbeitung.texturEckenSperre ? "v" : "-") + (e.bob.planter.aktiv ? "p" : "-") + (e.kamFenster.frei ? "f" : "-") + "|";
    for (int i = 0; i < 4; ++i) { z.modus += e.bearbeitung.transformText(i) + "|"; }
    ke::einstellungenSpeichern();
    for (const auto& [schluessel, wert] : g_app->settings.karte) {
        if (schluessel == "LastMap") { continue; }   // folgt der offenen Karte
        z.einst += schluessel + "=" + wert + "\n";
    }
    for (const auto& [schluessel, wert] : g_app->settings.karteTasten) { z.einst += "Taste " + schluessel + "=" + wert + "\n"; }
    z.extern_ = e.letzteUrl + "|" + e.letzteHilfe + "|" + externProtokoll();
    for (const auto& ent : e.karte.entitaeten()) {
        z.sicht += ent->versteckt ? "v" : "-";
        z.sicht += ent->ebeneVersteckt ? "e" : "-";
        for (const auto& p : ent->kinder()) {
            z.sicht += p->versteckt ? "v" : "-";
            z.sicht += p->ebeneVersteckt ? "e" : "-";
        }
    }
    {
        const ke::Anzeige& a = e.anzeige;
        z.anzeige = std::string(a.groesse2d ? "1" : "0") + (a.fadenkreuz ? "1" : "0") + (a.raster ? "1" : "0") + (a.winkel ? "1" : "0") +
                    (a.namen ? "1" : "0") + (a.kaesten ? "1" : "0") + (a.verbindungen ? "1" : "0") + (a.lichtRadien ? "1" : "0") +
                    (a.bloecke ? "1" : "0") + (a.koordinaten ? "1" : "0") + (a.umriss ? "1" : "0") + (a.achsen ? "1" : "0") +
                    (a.workzone2d ? "1" : "0") + "|";
        std::size_t linien = 0;
        for (const ke::PluginLinien& l : ke::pluginLinien2d()) { linien += l.linien.size(); }
        z.anzeige += std::to_string(linien);
    }
    z.fenster = std::to_string(static_cast<int>(e.anordnung)) + (e.gruppenDialogOffen ? "g" + std::to_string(e.gruppenSeite) : "-") +
                (e.maximiert ? "m" : "-") + (platform::istVollbild() ? "v" : "-");
    for (const bool b : e.schwebendSichtbar) { z.fenster += b ? "1" : "0"; }
    z.leck = std::string(e.punktdateiSichtbar ? "s" : "-") + std::to_string(e.punktdatei.size()) + "@" + std::to_string(e.punktdateiStelle) + "|" +
             std::to_string(e.portale.size()) + "|" + (e.portalE.zeige2d ? "2" : "-") + (e.portalE.zeige3d ? "3" : "-");
    z.konsole = e.konsole.size();
    z.datei = dateienImTestordner();
    z.region = std::string(e.region.aktiv ? "a " : "- ") + vek(e.region.min) + " " + vek(e.region.max);
    for (const auto& ent : e.karte.entitaeten()) {
        z.region += ent->ausgeschlossen ? "x" : "-";
        for (const auto& p : ent->kinder()) { z.region += p->ausgeschlossen ? "x" : "-"; }
    }
    z.bau = std::string(ke::bauLaeuft() ? "l" : "-") + std::to_string(e.bauZuletzt) + "|" + std::to_string(e.bauRueckmeldungen.size()) + "|" +
            std::to_string(e.bauGestartet);
    z.klassen = std::to_string(e.neuGeladen);
    return z;
}

unsigned unterschied(const Zustand& a, const Zustand& b) {
    unsigned m = 0;
    const auto u = [&m](bool anders, unsigned bit) {
        if (anders) { m |= bit; }
    };
    u(a.karte != b.karte, wKarte);
    u(a.auswahl != b.auswahl, wAuswahl);
    u(a.ansicht != b.ansicht, wAnsicht);
    u(a.kamera != b.kamera, wKamera);
    u(a.raster != b.raster, wRaster);
    u(a.filter != b.filter, wFilter);
    u(a.dialog != b.dialog, wDialog);
    u(a.ablage != b.ablage, wAblage);
    u(a.texturen != b.texturen, wTexturen);
    u(a.modus != b.modus, wModus);
    u(a.einst != b.einst, wEinstellung);
    u(a.extern_ != b.extern_, wExtern);
    u(a.sicht != b.sicht, wSicht);
    u(a.anzeige != b.anzeige, wAnzeige);
    u(a.fenster != b.fenster, wFenster);
    u(a.leck != b.leck, wLeck);
    u(a.konsole != b.konsole, wKonsole);
    u(a.datei != b.datei, wDatei);
    u(a.region != b.region, wRegion);
    u(a.bau != b.bau, wBau);
    u(a.klassen != b.klassen, wKlassen);
    return m;
}

// --- die Tabelle -----------------------------------------------------------------
//
// Je registrierter Befehl (Command/Toggle, befehle_liste.inc) und Menue-Ruf: die
// Vorbedingung und die Arten der Wirkung, die eintreten MUESSEN (Buchstaben wie
// oben). Eine Aenderung der Karte, die nicht erwartet ist, ist ein Fehler.
// ohneUndo: die Karte aendert sich, ist aber kein Rueckgaengig-Schritt (neue
// Karte, Laden - wie NRC). datei: Antwort des ersetzten Dateidialogs.
struct Erwartung {
    const char* name;
    Vor vor;
    const char* wirkung;
    bool ohneUndo = false;
    const char* datei = "";
};

const std::vector<Erwartung>& tabelle() {
    static const std::vector<Erwartung> t = {
#include "selbsttest_karte6_tabelle.inc"
    };
    return t;
}

const Erwartung* erwartung(const std::string& name) {
    for (const Erwartung& x : tabelle()) {
        if (name == x.name) { return &x; }
    }
    return nullptr;
}

// Befehle, die die Wirkungspruefung nicht ueber ausfuehren() ausloest, mit Grund
// (die Pruefung der Wirkung steht dort, wo der Grund es sagt).
const char* ausnahme(const std::string& name) {
    if (name == "Exit") { return "beendet das Programm - Stufe 4 prueft den Weg davor (ConfirmModified, Save/Discard/Cancel)"; }
    return nullptr;
}

// Nach dem Befehl: Dialoge zu, Bauen abbrechen, gefangene Maus los, Vollbild aus.
void aufraeumen() {
    ke::Editor& e = ke::editor();
    karteDialogeZu();
    e.siOffen = false;
    e.csgWerkzeug.offen = false;
    e.eingabe.offen = false;
    e.eingabe.oeffnen = false;
    e.bob.dialog = 0;
    e.bob.oeffnen = false;
    e.bob.visOeffnen = false;
    e.tags.bearbeitet.clear();
    e.drehDialog.offen = e.drehDialog.zeigen = false;
    e.skalierDialog.offen = e.skalierDialog.zeigen = false;
    e.schriftDialogOffen = false;
    e.brushExport.offen = e.brushExport.oeffnen = false;
    if (ke::optimiererOffen()) { ke::optimiererSchliessen(); }
    if (e.kamFenster.frei) { ke::freelookBeenden(); }
    if (platform::istVollbild()) { (void)platform::vollbildUmschalten(); }
    ke::kameraTastenLoslassen();
}

// --- Ausloeser -------------------------------------------------------------------

// Klicks nacheinander auf die Elemente mit diesen Kennungen (Endung der
// Beschriftung "###..." bzw. "##..."), wie ein Mensch: Maus hin, druecken,
// loslassen - je Klick sechs Bilder. Ein fehlendes Element meldet FEHLER.
std::function<bool(int)> klickFolge(std::vector<std::function<std::string()>> ziele, std::string fenster, std::string* fehler) {
    return [ziele, fenster, fehler](int b) {
        const std::size_t i = static_cast<std::size_t>(b / 6);
        if (i >= ziele.size()) { return true; }
        const int f = b % 6;
        ImGuiIO& io = ImGui::GetIO();
        const std::string roh = ziele[i]();
        const bool rechts = roh.rfind("@R", 0) == 0;   // "@R<Kennung>": Rechtsklick
        const int knopf = rechts ? 1 : 0;
        if (f == 0) {
            const std::string ziel = rechts ? roh.substr(2) : roh;
            const Element* el = findeEndung(ziel, i == 0 ? fenster : std::string());
            if (el == nullptr) {
                if (fehler != nullptr && fehler->empty()) { *fehler = "nicht gefunden: " + ziel; }
                return true;
            }
            setzeMaus(el->rect.GetCenter());
        }
        if (f == 2) {
            io.AddMouseButtonEvent(knopf, true);
            if (fehler != nullptr && i + 1 == ziele.size()) { diag::detail("Knopftest: Stufe 6 - Klick " + roh + ": " + wasImGuiSieht()); }
        }
        if (f == 3) { io.AddMouseButtonEvent(knopf, false); }
        return false;
    };
}

// --- US-Tastatur --------------------------------------------------------------------
//
// Die Kuerzel von NRC sind fuer die US-Belegung geschrieben ("[", "]", "`",
// "\\", "/"); mit einer festen US-Belegung (statt der von Windows) ist jedes
// Kuerzel mit seiner Taste erreichbar - die deutsche Belegung prueft
// karteneditorTastenSchritte.
bool usTastatur(int k, karte::befehle::TastenBelegung& out, bool& ziffernblock, bool& erweitert) {
    ziffernblock = false;
    erweitert = false;
    out = karte::befehle::TastenBelegung{};
    const auto setze = [&out](std::uint32_t ohne, std::uint32_t mit) {
        out.qtKey = {ohne, mit, ohne, mit, ohne, mit, ohne, mit, ohne};
        out.vorhanden = true;
        return true;
    };
    if (k >= ImGuiKey_A && k <= ImGuiKey_Z) {
        const auto l = static_cast<std::uint32_t>('A' + (k - ImGuiKey_A));
        return setze(l, l);
    }
    static const char kUmschalt[] = ")!@#$%^&*(";
    if (k >= ImGuiKey_0 && k <= ImGuiKey_9) {
        return setze(static_cast<std::uint32_t>('0' + (k - ImGuiKey_0)), static_cast<std::uint32_t>(kUmschalt[k - ImGuiKey_0]));
    }
    switch (k) {
    case ImGuiKey_LeftBracket: return setze('[', '{');
    case ImGuiKey_RightBracket: return setze(']', '}');
    case ImGuiKey_GraveAccent: return setze('`', '~');
    case ImGuiKey_Backslash: return setze('\\', '|');
    case ImGuiKey_Slash: return setze('/', '?');
    case ImGuiKey_Minus: return setze('-', '_');
    case ImGuiKey_Equal: return setze('=', '+');
    case ImGuiKey_Space: return setze(' ', ' ');
    default: break;
    }
    if (k >= ImGuiKey_Keypad0 && k <= ImGuiKey_Keypad9) {
        ziffernblock = true;
        const auto d = static_cast<std::uint32_t>('0' + (k - ImGuiKey_Keypad0));
        return setze(d, d);
    }
    if (k == ImGuiKey_KeypadSubtract || k == ImGuiKey_KeypadAdd) {
        ziffernblock = true;
        const std::uint32_t c = k == ImGuiKey_KeypadSubtract ? '-' : '+';
        return setze(c, c);
    }
    return false;   // benannte Tasten (F1, Pfeile, Tab, Enter ...): wie Windows
}

// Qt-Taste (QKeySequence) -> ImGui-Taste mit Zusatztasten; false, wenn es keine gibt.
bool imguiTaste(kb::Taste t, ImGuiKeyChord& chord) {
    const std::uint32_t taste = t & ~kb::kModMaske;
    const bool num = (t & kb::kNum) != 0U;
    ImGuiKey k = ImGuiKey_None;
    if (taste >= 'A' && taste <= 'Z') { k = static_cast<ImGuiKey>(ImGuiKey_A + static_cast<int>(taste - 'A')); }
    if (taste >= '0' && taste <= '9') {
        k = static_cast<ImGuiKey>((num ? ImGuiKey_Keypad0 : ImGuiKey_0) + static_cast<int>(taste - '0'));
    }
    if (taste >= kb::kKeyF1 && taste < kb::kKeyF1 + 12) { k = static_cast<ImGuiKey>(ImGuiKey_F1 + static_cast<int>(taste - kb::kKeyF1)); }
    switch (taste) {
    case '[': k = ImGuiKey_LeftBracket; break;
    case ']': k = ImGuiKey_RightBracket; break;
    case '`': k = ImGuiKey_GraveAccent; break;
    case '\\': k = ImGuiKey_Backslash; break;
    case '/': k = ImGuiKey_Slash; break;
    case '-': k = num ? ImGuiKey_KeypadSubtract : ImGuiKey_Minus; break;
    case '+': k = num ? ImGuiKey_KeypadAdd : ImGuiKey_Equal; break;
    case ' ': k = ImGuiKey_Space; break;
    case kb::kKeyEscape: k = ImGuiKey_Escape; break;
    case kb::kKeyTab: k = ImGuiKey_Tab; break;
    case kb::kKeyBackspace: k = ImGuiKey_Backspace; break;
    case kb::kKeyReturn: k = ImGuiKey_Enter; break;
    case kb::kKeyEnter: k = ImGuiKey_KeypadEnter; break;
    case kb::kKeyInsert: k = ImGuiKey_Insert; break;
    case kb::kKeyDelete: k = ImGuiKey_Delete; break;
    case kb::kKeyHome: k = ImGuiKey_Home; break;
    case kb::kKeyEnd: k = ImGuiKey_End; break;
    case kb::kKeyLeft: k = ImGuiKey_LeftArrow; break;
    case kb::kKeyUp: k = ImGuiKey_UpArrow; break;
    case kb::kKeyRight: k = ImGuiKey_RightArrow; break;
    case kb::kKeyDown: k = ImGuiKey_DownArrow; break;
    case kb::kKeyPageUp: k = ImGuiKey_PageUp; break;
    case kb::kKeyPageDown: k = ImGuiKey_PageDown; break;
    default: break;
    }
    if (k == ImGuiKey_None) { return false; }
    chord = k;
    if ((t & kb::kCtrl) != 0U) { chord |= ImGuiMod_Ctrl; }
    if ((t & kb::kShift) != 0U) { chord |= ImGuiMod_Shift; }
    if ((t & kb::kAlt) != 0U) { chord |= ImGuiMod_Alt; }
    return true;
}

// Eine Taste mit Zusatztasten druecken (sechs Bilder, wie tasteKombi), die Maus
// ueber der XY-Ansicht.
std::function<bool(int)> tastenDruck(ImGuiKeyChord chord) {
    return [chord](int b) {
        ImGuiIO& io = ImGui::GetIO();
        const auto key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
        const bool strg = (chord & ImGuiMod_Ctrl) != 0;
        const bool umsch = (chord & ImGuiMod_Shift) != 0;
        const bool alt = (chord & ImGuiMod_Alt) != 0;
        if (b == 0) {
            ke::tastaturErsatz = usTastatur;
            const ke::XyFenster& f = ke::editor().xy[0];
            setzeMaus(ImVec2{f.bildX + 40.0F, f.bildY + 40.0F});
            if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
            if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, true); }
            if (alt) { io.AddKeyEvent(ImGuiMod_Alt, true); }
        }
        if (b == 1) { io.AddKeyEvent(key, true); }
        if (b == 2) {
            io.AddKeyEvent(key, false);
            const ke::Editor& e = ke::editor();
            diag::detail(std::string("Knopftest: Stufe 6 - Tastenweg: Popup ") + std::to_string(GImGui->OpenPopupStack.Size) + ", Texteingabe " +
                         (io.WantTextInput ? "ja" : "nein") + ", Aufnahme \"" + e.tastenDialogBearbeitet + "\", Maus " +
                         (io.MouseDown[0] || io.MouseDown[1] || io.MouseDown[2] ? "gedrueckt" : "frei") + ", gedrueckt " +
                         (ImGui::IsKeyDown(key) ? "ja" : "nein") + ", Fokus " + (GImGui->NavWindow != nullptr ? GImGui->NavWindow->Name : "-") +
                         ", aktiv " + (GImGui->ActiveId != 0 ? "ja" : "nein"));
        }
        if (b == 3) {
            if (strg) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
            if (umsch) { io.AddKeyEvent(ImGuiMod_Shift, false); }
            if (alt) { io.AddKeyEvent(ImGuiMod_Alt, false); }
        }
        if (b >= 5) {
            ke::tastaturErsatz = nullptr;
            return true;
        }
        return false;
    };
}

// Ein Bild aus dem Dateisystem des Spiels als voller Pfad unter seinem ersten
// Ordner (Set 2D Background image nimmt den Pfad relativ zum Spiel, das Bild
// selbst darf in einer .pk3 liegen - wie bei NRC ueber GlobalFileSystem).
std::string bildImSpiel() {
    ke::Editor& e = ke::editor();
    if (!e.karteSpiel || e.karteSpiel->ordner.empty()) { return {}; }
    for (const char* kandidat : {"textures/system/caulk", "textures/common/caulk", "textures/system/clip", "gfx/2d/colorbar"}) {
        std::vector<unsigned char> rgba;
        int b = 0;
        int h = 0;
        if (ke::kartenBildRgba(kandidat, rgba, b, h)) { return karte::ordnerBereinigt(e.karteSpiel->ordner.front()) + kandidat + ".tga"; }
    }
    return {};
}

// --- der Wirkungsschritt -------------------------------------------------------------

struct Lauf {
    Zustand vorher;
    bool hakenVorher = false;
    int start = 0;   // >0: Bild, in dem das Ausloesen begann; <0: Bild, in dem es fertig war
    std::string fehler;
};

struct Art {
    Vor vor = Vor::Leer;
    unsigned soll = 0;
    bool ohneUndo = false;
    std::string datei;
    bool mitUndo = false;
    std::function<void()> vorher;   // nach vorbereiten() (z. B. eine andere Fensteranordnung)
    int warten = 0;                 // Bilder mehr bis zum Erfassen (ein Dialog geht auf)
};

Art artAus(const Erwartung& x) {
    Art a;
    a.vor = x.vor;
    a.soll = maskeAus(x.wirkung);
    a.ohneUndo = x.ohneUndo;
    a.datei = x.datei;
    return a;
}

// Ein Befehl mit Wirkungspruefung (und Rueckgaengig/Wiederholen, wenn die Karte
// sich aendert). ausloesen(b): wie der Befehl ausgeloest wird, ueber mehrere
// Bilder; true = fertig. Danach vier Bilder warten (Fenster da, Popups offen).
Schritt wirkungsSchritt(const std::string& pruefung, const std::string& befehl, Art art,
                        std::function<std::function<bool(int)>(std::string*)> ausloeserBauen) {
    auto l = std::make_shared<Lauf>();
    auto ausloesen = std::make_shared<std::function<bool(int)>>();
    return {pruefung, [=](int b) {
                ke::Editor& e = ke::editor();
                if (b == 0) {
                    vorbereiten(art.vor);
                    if (art.vorher) { art.vorher(); }
                    dialogAntwort() = art.datei.empty() ? std::string() : art.datei == "@bild" ? bildImSpiel() : karteTestPfad(art.datei.c_str());
                    l->fehler.clear();
                    *ausloesen = ausloeserBauen(&l->fehler);
                    l->start = 0;
                    return false;
                }
                if (b < 3 + art.warten) { return false; }
                if (b == 3 + art.warten) {
                    l->vorher = erfassen();
                    l->hakenVorher = ke::angehakt(befehl);
                    e.letzterBefehl.clear();
                    l->start = b;
                }
                if (l->start > 0) {
                    if (!(*ausloesen)(b - l->start)) { return false; }
                    l->start = -b;
                    return false;
                }
                if (b < 4 - l->start) { return false; }
                const Zustand n = erfassen();
                unsigned ist = unterschied(l->vorher, n);
                if (ke::angehakt(befehl) != l->hakenVorher) { ist |= wHaken; }
                const bool ausgeloest = befehl.empty() || e.letzterBefehl == befehl;
                const bool ok = l->fehler.empty() && ausgeloest && (ist & art.soll) == art.soll && ((art.soll & wKarte) != 0U || (ist & wKarte) == 0U);
                diag::detail("Knopftest: Stufe 6 - " + pruefung + " | " + vorName(art.vor) + " | soll " + maskeKurz(art.soll) + " ist " + maskeKurz(ist) +
                             (ausgeloest ? "" : " (nicht ausgeloest: zuletzt \"" + e.letzterBefehl + "\")") + (l->fehler.empty() ? "" : " " + l->fehler) + " [Kamera-Bilder " + std::to_string(e.kamFenster.bilder) + "]");
                pruefeWahr(ok, pruefung);
                if (art.mitUndo && (art.soll & wKarte) != 0U && !art.ohneUndo) {
                    const std::string nachher = n.karte;
                    (void)ke::ausfuehren("Undo");
                    const std::string zurueck = karte::karteSchreiben(e.karte);
                    (void)ke::ausfuehren("Redo");
                    const std::string wieder = karte::karteSchreiben(e.karte);
                    const bool gleich = zurueck == l->vorher.karte && wieder == nachher;
                    if (!gleich) {
                        diag::detail("Knopftest: Stufe 6 - Rueckgaengig " + befehl + ": vorher " + std::to_string(l->vorher.karte.size()) + " Byte, zurueck " +
                                     std::to_string(zurueck.size()) + ", nachher " + std::to_string(nachher.size()) + ", wieder " +
                                     std::to_string(wieder.size()));
                    }
                    pruefeWahr(gleich, "Rueckgaengig " + (befehl.empty() ? pruefung.substr(0, pruefung.find(':')) : befehl) +
                                           ": .map-Text wie vorher, Wiederholen wie nachher");
                }
                aufraeumen();
                return true;
            }};
}

std::function<std::function<bool(int)>(std::string*)> perAusfuehren(const std::string& name) {
    return [name](std::string*) {
        return [name](int) {
            (void)ke::ausfuehren(name);
            return true;
        };
    };
}

// BHED_STUFE6_BEFEHL=<Name>[,<Name>...]: nur diese Befehle (gezielter Rot/Gruen-Test)
bool gewuenscht(const std::string& name) {
    const char* nur = std::getenv("BHED_STUFE6_BEFEHL");
    if (nur == nullptr || *nur == 0) { return true; }
    const std::string liste = std::string(",") + nur + ",";
    return liste.find("," + name + ",") != std::string::npos;
}

// --- Teil "befehle": jeder Befehl ueber ausfuehren() ----------------------------------

std::vector<Schritt> befehle() {
    std::vector<Schritt> s;
    s.push_back(tu("Stufe 6: Testkarten schreiben, Dateidialog und Aussenwelt ersetzen", [] {
        fixtureSchreiben();
        ke::Editor& e = ke::editor();
        e.dateiDialog = [](const std::string&) { return dialogAntwort(); };
        e.urlOeffnen = [](const std::string& u) { externProtokoll() += "url " + u + ";"; };
        e.befehlStarten = [](const std::string& z) {
            externProtokoll() += "exec " + z + ";";
            return true;
        };
        if (!anfang().gemerkt) {
            anfang().kameraE = e.kameraE;
            anfang().versteckteKnoepfe = e.versteckteKnoepfe;
            anfang().schwebend = e.schwebendSichtbar;
            anfang().gemerkt = true;
        }
    }));
    // Jeder registrierte Befehl steht in der Tabelle - und kein anderer.
    s.push_back(pruefe("Stufe 6: jeder Befehl (Command/Toggle) und jeder Menue-Ruf hat eine Erwartung, die Tabelle nur echte Befehle", [] {
        std::string fehlt;
        std::string fremd;
        for (const kb::Befehl& b : kb::alle()) {
            if ((b.art & (kb::kCommand | kb::kToggle)) == 0U) { continue; }
            if (erwartung(b.name) == nullptr && ausnahme(b.name) == nullptr) { fehlt += std::string(" ") + b.name; }
        }
        for (const kb::MenueEintrag& m : kb::menues()) {
            if (m.art == kb::MenueArt::Ruf && erwartung(m.befehl) == nullptr) { fehlt += std::string(" ") + m.befehl; }
        }
        for (const Erwartung& x : tabelle()) {
            const kb::Befehl* b = kb::finden(x.name);
            const bool ruf = std::any_of(kb::menues().begin(), kb::menues().end(),
                                         [&x](const kb::MenueEintrag& m) { return m.art == kb::MenueArt::Ruf && std::strcmp(m.befehl, x.name) == 0; });
            if (b == nullptr && !ruf) { fremd += std::string(" ") + x.name; }
            if (x.wirkung[0] == 0) { fremd += std::string(" ") + x.name + "(ohne Wirkung)"; }
        }
        diag::detail("Knopftest: Stufe 6 - Tabelle " + std::to_string(tabelle().size()) + " Eintraege; ohne Erwartung:" +
                     (fehlt.empty() ? std::string(" -") : fehlt) + "; unbekannt/leer:" + (fremd.empty() ? std::string(" -") : fremd));
        return fehlt.empty() && fremd.empty();
    }));
    for (const Erwartung& x : tabelle()) {
        if (!gewuenscht(x.name)) { continue; }
        Art a = artAus(x);
        a.mitUndo = true;
        s.push_back(wirkungsSchritt("Wirkung " + std::string(x.name) + ": " + maskeText(a.soll) + " (" + vorName(x.vor) + ")", x.name, a,
                                    perAusfuehren(x.name)));
    }
    return s;
}

// --- Teil "robust": leere Auswahl, leere Karte, gemischte Auswahl ----------------------
//
// Kein Absturz und keine Zusicherung (IM_ASSERT bricht ab, der Lauf endete
// dann ohne "Knopftest fertig"); hat der Befehl die Karte geaendert, nimmt
// Rueckgaengig die Aenderung byte-gleich zurueck (ausser bei Laden/Neu); die
// Konsole bekommt hoechstens Meldungen. Je Befehl ein Schritt mit drei Laeufen.
std::vector<Schritt> robust() {
    std::vector<Schritt> s;
    for (const Erwartung& x : tabelle()) {
        if (!gewuenscht(x.name)) { continue; }
        const std::string n = x.name;
        const bool ohneUndo = x.ohneUndo;
        struct R {
            Zustand vorher;
            std::string bericht;
            bool ok = true;
        };
        auto r = std::make_shared<R>();
        s.push_back({"Robust " + n, [n, ohneUndo, r](int b) {
                         static const Vor kFaelle[3] = {Vor::Leer, Vor::LeereKarte, Vor::Misch};
                         const int fall = b / 10;
                         const int f = b % 10;
                         if (fall >= 3) {
                             pruefeWahr(r->ok, "Robust " + n + ": leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck");
                             if (!r->bericht.empty()) { diag::detail("Knopftest: Stufe 6 - Robust " + n + ":" + r->bericht); }
                             r->ok = true;
                             r->bericht.clear();
                             return true;
                         }
                         ke::Editor& e = ke::editor();
                         if (f == 0) {
                             vorbereiten(kFaelle[fall]);
                             dialogAntwort().clear();
                         }
                         if (f == 2) {
                             r->vorher = erfassen();
                             (void)ke::ausfuehren(n);
                         }
                         if (f == 6) {
                             const Zustand nach = erfassen();
                             const unsigned ist = unterschied(r->vorher, nach);
                             std::string meldung;
                             for (std::size_t i = r->vorher.konsole; i < e.konsole.size(); ++i) { meldung += " \"" + e.konsole[i].text + "\""; }
                             r->bericht += std::string(" [") + vorName(kFaelle[fall]) + ": " + maskeKurz(ist) + meldung.substr(0, 160) + "]";
                             if ((ist & wKarte) != 0U && !ohneUndo && n != "Undo" && n != "Redo" && n != "Redo2") {
                                 (void)ke::ausfuehren("Undo");
                                 if (karte::karteSchreiben(e.karte) != r->vorher.karte) {
                                     r->ok = false;
                                     r->bericht += " Rueckgaengig nicht byte-gleich";
                                 }
                             }
                             aufraeumen();
                         }
                         return false;
                     }});
    }
    return s;
}

// --- Teil "tasten": jedes Standardkuerzel ueber echte Tastenereignisse ------------------
std::vector<Schritt> tasten() {
    std::vector<Schritt> s;
    for (const kb::Befehl& b : kb::alle()) {
        if (b.taste[0] == 0 || !gewuenscht(b.name)) { continue; }
        const std::string n = b.name;
        const kb::Taste t = kb::tasteAusText(b.taste);
        ImGuiKeyChord chord = 0;
        if (!imguiTaste(t, chord)) {
            s.push_back(pruefe("Taste " + std::string(b.taste) + " = " + n + ": keine ImGui-Taste", [] { return false; }));
            continue;
        }
        const Erwartung* x = erwartung(n);
        Art a;
        if (x != nullptr) {
            a = artAus(*x);
        } else if ((b.art & kb::kKeyEvent) != 0U) {
            // reine KeyEvents (Freelook): die Taste haelt die Bewegung (Teil "ereignisse")
            a.vor = Vor::Freelook;
            a.soll = wKamera;
            if (n == "CameraFreeFocus") { a.vor = Vor::Freelook; }
        } else {
            continue;
        }
        std::string name = "Taste " + std::string(b.taste) + " = " + n + ": " + maskeText(a.soll) + " (" + vorName(a.vor) + ")";
        if (x == nullptr) {
            // KeyEvent im Freelook: druecken, halten, loslassen (zwoelf Bilder)
            s.push_back(wirkungsSchritt(name, "", a, [chord](std::string*) {
                return [chord](int b2) {
                    ImGuiIO& io = ImGui::GetIO();
                    const auto key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
                    if (b2 == 0) {
                        ke::tastaturErsatz = usTastatur;
                        if (ke::editor().karte.auswahl.anzahl() == 0) {
                            ke::editor().karte.auswahl.waehlen(ke::editor().karte.welt(), weltKind(0));
                            ke::auswahlGeaendert();
                        }
                        io.AddKeyEvent(key, true);
                    }
                    if (b2 == 12) { io.AddKeyEvent(key, false); }
                    if (b2 >= 14) {
                        ke::tastaturErsatz = nullptr;
                        return true;
                    }
                    return false;
                };
            }));
            continue;
        }
        s.push_back(wirkungsSchritt(name, n, a, [chord](std::string*) { return tastenDruck(chord); }));
    }
    return s;
}

// --- Teil "ereignisse": KeyEvents (Taste halten) ----------------------------------------
//
// CameraForward ... CameraAngleDown haengen bei "Discrete movement" aus an
// ihrer Taste, CameraFreeMove* und CameraFreeFocus im Freelook. Ohne
// Standardtaste (CameraUp, CameraDown, CameraAngleUp/Down, CameraFreeMoveUp/
// Down) belegt der Test F9 und setzt die Belegung danach zurueck.
std::vector<Schritt> ereignisse() {
    std::vector<Schritt> s;
    for (const kb::Befehl& b : kb::alle()) {
        if ((b.art & kb::kKeyEvent) == 0U) { continue; }
        const std::string n = b.name;
        const bool frei = n.rfind("CameraFree", 0) == 0;
        Art a;
        a.vor = frei ? Vor::Freelook : Vor::Leer;
        a.soll = wKamera;
        const bool eigeneTaste = b.taste[0] == 0;
        a.vorher = [n, frei, eigeneTaste] {
            ke::Editor& e = ke::editor();
            if (!frei) { e.kameraE.diskret = false; }
            if (eigeneTaste) { e.belegung.setzen(n, kb::kKeyF1 + 8); }
            if (n == "CameraFreeFocus") {
                e.karte.auswahl.waehlen(e.karte.welt(), weltKind(0));
                ke::auswahlGeaendert();
            }
        };
        ImGuiKeyChord chord = ImGuiKey_F9;
        if (!eigeneTaste && !imguiTaste(kb::tasteAusText(b.taste), chord)) { continue; }
        s.push_back(wirkungsSchritt("Ereignis " + n + ": Kamera bewegt, solange die Taste gehalten wird (" + (frei ? "Freelook" : "nicht diskret") + ")", "", a,
                                    [chord, n, eigeneTaste](std::string*) {
                                        return [chord, n, eigeneTaste](int b2) {
                                            ImGuiIO& io = ImGui::GetIO();
                                            const auto key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
                                            if (b2 == 0) {
                                                ke::tastaturErsatz = usTastatur;
                                                const ke::XyFenster& f = ke::editor().xy[0];
                                                setzeMaus(ImVec2{f.bildX + 40.0F, f.bildY + 40.0F});
                                                io.AddKeyEvent(key, true);
                                            }
                                            if (b2 == 8) {
                                                const ke::Editor& e = ke::editor();
                                                diag::detail("Knopftest: Stufe 6 - Ereignisweg " + n + ": gedrueckt " + (ImGui::IsKeyDown(key) ? "ja" : "nein") +
                                                             ", Tasten " + std::to_string(e.kamFenster.tasten.size()) + ", Bewegung " +
                                                             std::to_string(e.kamera.bewegung) + ", frei " + (e.kamFenster.frei ? "ja" : "nein") +
                                                             ", diskret " + (e.kameraE.diskret ? "ja" : "nein") + ", Taste " +
                                                             kb::tasteAlsText(e.belegung.taste(n)) + ", Kamera " + std::to_string(e.kamFenster.breite) + "x" +
                                                             std::to_string(e.kamFenster.hoehe) + ", Popup " + std::to_string(GImGui->OpenPopupStack.Size) +
                                                             ", Ersatz " + (ke::tastaturErsatz ? "ja" : "nein") + ", Ort " + vek(e.kamera.ursprung) +
                                                             ", Winkel " + vek(e.kamera.winkel) + ", Kamera-Bilder " + std::to_string(e.kamFenster.bilder) + ", Uhr " + zahl(e.kamFenster.tastenUhr) + ", jetzt " +
                                                             zahl(ImGui::GetTime()) + ", Empfindlichkeit " + zahl(e.kameraE.winkelTempo) + ", Tempo " +
                                                             std::to_string(e.kameraE.tempo));
                                            }
                                            if (b2 == 15) { io.AddKeyEvent(key, false); }
                                            if (b2 >= 17) {
                                                ke::tastaturErsatz = nullptr;
                                                if (eigeneTaste) { ke::editor().belegung.zuruecksetzen(n); }
                                                if (anfang().gemerkt) { ke::editor().kameraE.diskret = anfang().kameraE.diskret; }
                                                return true;
                                            }
                                            return false;
                                        };
                                    }));
    }
    return s;
}

// --- Teil "menues": jeder Menuepunkt ueber echte Mausklicks -----------------------------

// Die Anordnung, unter der ein Eintrag mit dieser Bedingung sichtbar ist.
karte::befehle::Anordnung anordnungFuer(kb::Bedingung b) {
    using A = karte::befehle::Anordnung;
    switch (b) {
    case kb::kNurFloating:
    case kb::kNichtRegular:
    case kb::kTexturen: return A::Floating;
    case kb::kNurSplit: return A::Split;
    default: return A::Regular;
    }
}

std::vector<Schritt> menues() {
    std::vector<Schritt> s;
    const auto& liste = kb::menues();
    std::vector<std::size_t> weg;   // Menue/Unter-Indizes bis hier
    const auto beschriftung = [](const kb::MenueEintrag& m) { return karte::ohneMnemonik(m.text); };
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const kb::MenueEintrag& m = liste[i];
        if (m.art == kb::MenueArt::Menue || m.art == kb::MenueArt::Unter) {
            weg.push_back(i);
            continue;
        }
        if (m.art == kb::MenueArt::Ende) {
            if (!weg.empty()) { weg.pop_back(); }
            continue;
        }
        // Die Kennungen der Menues auf dem Weg (zur Laufzeit: Sprache, Begriffe)
        std::vector<std::function<std::string()>> ziele;
        std::string wegText;
        for (std::size_t k = 0; k < weg.size(); ++k) {
            const std::size_t j = weg[k];
            ziele.emplace_back([j, k] { return (k == 0 ? std::string("###menue") : std::string("###unter")) + std::to_string(j); });
            wegText += beschriftung(liste[j]) + " > ";
        }
        const auto menuePunkt = [&](const std::string& befehl, const std::string& anzeige, const std::string& kennung, Art a, const std::string& pruefBefehl) {
            auto z = ziele;
            z.emplace_back([kennung] { return kennung; });
            s.push_back(wirkungsSchritt("Menue " + wegText + anzeige + " = " + befehl + ": " + maskeText(a.soll) + " (" + vorName(a.vor) + ")", pruefBefehl, a,
                                        [z](std::string* fehler) { return klickFolge(z, "###main", fehler); }));
        };
        if (m.art == kb::MenueArt::Punkt || m.art == kb::MenueArt::Haken || m.art == kb::MenueArt::Ruf) {
            const std::string befehl = m.befehl;
            if (befehl == "Exit" || !gewuenscht(befehl)) { continue; }   // Stufe 4 prueft File > Exit samt Rueckfrage
            const Erwartung* x = erwartung(befehl);
            if (x == nullptr) {
                s.push_back(pruefe("Menue " + wegText + beschriftung(m) + " = " + befehl + ": keine Erwartung", [] { return false; }));
                continue;
            }
            Art a = artAus(*x);
            const auto anordnung = anordnungFuer(m.bedingung);
            if (anordnung != karte::befehle::Anordnung::Regular) {
                a.vorher = [anordnung] {
                    ke::editor().anordnung = anordnung;
                    ke::editor().teilerFuer = -1;
                };
            }
            const std::string kennung = (m.art == kb::MenueArt::Ruf ? std::string("###ruf") : "###" + befehl) + std::to_string(i);
            menuePunkt(befehl, beschriftung(m), kennung, a, befehl);
            continue;
        }
        if (m.art != kb::MenueArt::Liste) { continue; }
        const std::string lname = m.befehl;
        if (lname == "MRU") {
            for (int nr = 1; nr <= 9; ++nr) {
                Art a;
                a.vor = Vor::Pinsel;
                a.soll = wKarte;
                a.ohneUndo = true;
                a.vorher = [] {
                    std::vector<std::string> pfade;
                    for (int k = 1; k <= 9; ++k) { pfade.push_back(karteTestPfad(("s6_mru" + std::to_string(k) + ".map").c_str())); }
                    ke::editor().zuletzt.setzen(pfade);
                    ke::editor().zuletztGeladen = true;
                };
                menuePunkt("MRU" + std::to_string(nr), "&" + std::to_string(nr), "###karteMru" + std::to_string(nr - 1), a, "");
            }
        } else if (lname == "Transforms") {
            static const char* const kTeile[4] = {"Translate", "Rotate", "Scale", "Skew"};
            for (int t = 0; t < 4; ++t) {
                Art a;
                a.vor = Vor::Transformiert;
                a.soll = t == 1 ? wModus : 0U;   // gedreht ist nur die Drehung: die anderen setzen zurueck, was schon zurueck ist
                a.vorher = [t] {
                    // jede der vier wiederholbaren Transformationen setzen (Manipulator-Werte)
                    ke::Editor& e = ke::editor();
                    if (t == 0) { e.bearbeitung.verschiebeAuswahl(karte::Vektor3f(8, 0, 0)); }
                    if (t == 2) { e.bearbeitung.skaliereAuswahl(karte::Vektor3f(2, 1, 1)); }
                };
                if (t == 0 || t == 2) { a.soll = wModus; }
                menuePunkt(std::string("Transform") + kTeile[t], kTeile[t], "###karteTransform" + std::to_string(t), a, "");
            }
        } else if (lname == "Plugins") {
            for (const kb::PluginEintrag& p : kb::pluginEintraege()) {
                if (p.befehl[0] == 0) { continue; }
                const std::string befehl = p.befehl;
                const Erwartung* x = erwartung(befehl);
                if (x == nullptr) {
                    s.push_back(pruefe("Menue Plugins > " + std::string(p.menue) + " > " + p.titel + " = " + befehl + ": keine Erwartung", [] { return false; }));
                    continue;
                }
                auto z = ziele;
                const std::string menue = p.menue;
                z.emplace_back([menue] { return menue + "##plugin"; });
                z.emplace_back([befehl] { return "##pl" + befehl; });
                const Art a = artAus(*x);
                s.push_back(wirkungsSchritt("Menue Plugins > " + menue + " > " + karte::ohneMnemonik(p.titel) + " = " + befehl + ": " + maskeText(a.soll) + " (" +
                                                vorName(a.vor) + ")",
                                            befehl, a, [z](std::string* fehler) { return klickFolge(z, "###main", fehler); }));
            }
        }
    }
    // Die Menues des Texturbrowsers (texwindow.cpp:1238 View ueber den Knopf, :1281 Tags ueber das Kontextmenue der Tag-Liste)
    const auto& tm = kb::texturMenues();
    std::string tmenue;
    for (std::size_t i = 0; i < tm.size(); ++i) {
        const kb::MenueEintrag& m = tm[i];
        if (m.art == kb::MenueArt::Menue) {
            tmenue = m.text;
            continue;
        }
        if ((m.art != kb::MenueArt::Punkt && m.art != kb::MenueArt::Haken) || !gewuenscht(m.befehl)) { continue; }
        const std::string befehl = m.befehl;
        const Erwartung* x = erwartung(befehl);
        if (x == nullptr) {
            s.push_back(pruefe("Menue Texturbrowser " + tmenue + " > " + karte::ohneMnemonik(m.text) + " = " + befehl + ": keine Erwartung", [] { return false; }));
            continue;
        }
        Art a = artAus(*x);
        const bool tags = tmenue == "Tags";
        const Vor vor = tags ? (befehl == "TagPaste" ? Vor::TagsKopiert : Vor::Tags) : a.vor;
        a.vor = vor;
        a.vorher = [tags] {
            ke::Editor& e = ke::editor();
            e.gruppenSeite = 2;
            e.gruppenSeiteWechseln = true;
            e.gruppenDialogOffen = true;
            if (tags) {
                e.tags.gui = true;
                e.tags.reiter = 1;
            }
        };
        a.warten = 4;
        std::vector<std::function<std::string()>> z;
        if (tags) {
            z.emplace_back([] { return std::string("@R##tags6tag"); });
        } else {
            z.emplace_back([] { return std::string("##texknopftexbro_view"); });
        }
        const std::string kennung = "###" + befehl + std::to_string(i);
        z.emplace_back([kennung] { return kennung; });
        s.push_back(wirkungsSchritt("Menue Texturbrowser " + tmenue + " > " + karte::ohneMnemonik(m.text) + " = " + befehl + ": " + maskeText(a.soll) + " (" +
                                        vorName(vor) + ")",
                                    befehl, a, [z](std::string* fehler) { return klickFolge(z, "", fehler); }));
    }
    return s;
}

// --- Teil "knoepfe": jeder Werkzeugknopf ueber echte Mausklicks -------------------------
std::vector<Schritt> knoepfe() {
    std::vector<Schritt> s;
    s.push_back(tu("Stufe 6: alle Knoepfe der Leisten zeigen (g_toolbarHiddenButtons leer)", [] { ke::editor().versteckteKnoepfe.clear(); }));
    // leisteId: die Kennung der QToolBar (leisteZeichnen: "haupt", "filter", "plugin"; "" = Texturbrowser)
    const auto leiste = [&](const std::vector<kb::Knopf>& knoepfe, const std::string& leisteId) {
        for (const kb::Knopf& k : knoepfe) {
            if (k.trenner || k.befehl[0] == 0 || !gewuenscht(k.befehl)) { continue; }
            const std::string befehl = k.befehl;
            const Erwartung* x = erwartung(befehl);
            if (x == nullptr) {
                s.push_back(pruefe("Knopf " + befehl + ": keine Erwartung", [] { return false; }));
                continue;
            }
            Art a = artAus(*x);
            const auto anordnung = anordnungFuer(k.bedingung);
            a.vorher = [anordnung] {
                ke::editor().versteckteKnoepfe.clear();
                ke::editor().anordnung = anordnung;
                ke::editor().teilerFuer = -1;
            };
            s.push_back(wirkungsSchritt("Knopf " + befehl + ": " + maskeText(a.soll) + " (" + vorName(a.vor) + ")", befehl, a,
                                        [befehl, leisteId](std::string* fehler) {
                                            // sichtbar: anklicken; sonst ueber das Erweiterungsmenue (">>") der Leiste
                                            auto folge = std::make_shared<std::function<bool(int)>>();
                                            return std::function<bool(int)>([befehl, leisteId, fehler, folge](int b) {
                                                if (b == 0) {
                                                    const std::string direkt = (leisteId.empty() ? "##texknopf" : "##knopf") + befehl;
                                                    const Element* el = findeEndung(direkt, "");
                                                    std::vector<std::function<std::string()>> ziel;
                                                    if (el != nullptr && ganzSichtbar(*el)) {
                                                        ziel = {[direkt] { return direkt; }};
                                                    } else {
                                                        ziel = {[leisteId] { return "##erweiterung" + leisteId; }, [befehl] { return "##em" + befehl; }};
                                                    }
                                                    *folge = klickFolge(ziel, "", fehler);
                                                }
                                                return (*folge)(b);
                                            });
                                        }));
        }
    };
    leiste(kb::werkzeugleiste(), "haupt");
    leiste(kb::filterleiste(), "filter");
    leiste(kb::pluginleiste(), "plugin");
    leiste(kb::texturKnoepfe(), "");
    s.push_back(tu("Stufe 6: versteckte Knoepfe wie vorher", [] {
        if (anfang().gemerkt) { ke::editor().versteckteKnoepfe = anfang().versteckteKnoepfe; }
    }));
    return s;
}

// --- Teil "felder": jede Einstellungsseite und jedes Feld ueber die echte Oberflaeche ---
//
// Je Seite: Edit > Preferences..., die Seite im Baum anklicken, jedes Feld wie
// ein Mensch aendern (Haken anklicken, "+" am Spinner, Eintrag der Klappliste
// waehlen, Radioknopf, Text tippen), OK - danach muss sich der gespeicherte
// Schluessel jedes Felds geaendert haben (behaved.cfg, Abschnitt karte). Dann
// noch einmal: ein Feld aendern und Cancel - der Schluessel bleibt. Am Ende
// kommen alle Einstellungen zurueck.
enum class FeldArt { Haken, Plus, Klappe, Radio, Text };

struct Feld {
    const char* quelle;   // NRC-Datei:Zeile des Felds (Checkliste)
    const char* name;
    FeldArt art;
    std::function<std::string()> ziel;   // Beschriftung des Elements (Haken, Radio, Klappliste, Textfeld, Spinner)
    std::function<std::string()> wahl;   // Klappliste: der Eintrag; Text: was getippt wird
    const char* schluessel;
};

struct Seite6 {
    int index;              // ##seite<index> im Baum
    const char* titel;      // NRC-Titel (Checkliste "Einstellungsseiten")
    std::vector<Feld> felder;
    std::function<void()> vorbereiten;   // vor dem Oeffnen (abhaengige Felder freischalten)
};

std::string trS(Str s) { return ke::trK(s); }

std::map<std::string, std::string>& einstellungenVorher() {
    static std::map<std::string, std::string> m;
    return m;
}
std::map<std::string, std::string>& einstellungenAnfang() {
    static std::map<std::string, std::string> m;
    return m;
}

std::string einstellungJetzt(const char* schluessel) {
    ke::einstellungenSpeichern();
    const auto it = g_app->settings.karte.find(schluessel);
    return it != g_app->settings.karte.end() ? it->second : std::string("<fehlt>");
}

// Ein Feld aendern: Schritte ueber mehrere Bilder (sechs je Klick).
Schritt feldAendern(const Feld& f) {
    const Feld feld = f;
    return {std::string("Feld aendern: ") + f.name, [feld](int b) {
                ImGuiIO& io = ImGui::GetIO();
                const auto ort = [&]() -> const Element* {
                    if (feld.art == FeldArt::Plus) { return finde("+", "###karteeinstellungen", 0, false, feld.ziel()); }
                    return finde(feld.ziel(), "###karteeinstellungen", 0, false);
                };
                if (b == 0 && feld.art == FeldArt::Klappe) {   // QComboBox: die Lage aus klappOrte (ImGui meldet sie nicht als Element)
                    for (const auto& k : ke::editor().klappOrte) {
                        if (k.id == feld.ziel()) {
                            setzeMaus(ImVec2{(k.x0 + k.x1) * 0.5F, (k.y0 + k.y1) * 0.5F});
                            return false;
                        }
                    }
                    meldeFehler(std::string("Feld ") + feld.quelle + " " + feld.name + ": Klappliste \"" + feld.ziel() + "\" nicht gefunden");
                    return true;
                }
                if (b == 0) {
                    const Element* el = ort();
                    if (el == nullptr) {
                        meldeFehler(std::string("Feld ") + feld.quelle + " " + feld.name + ": Element \"" + feld.ziel() + "\" nicht gefunden");
                        return true;
                    }
                    setzeMaus(el->rect.GetCenter());
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); }
                if (b == 3) { io.AddMouseButtonEvent(0, false); }
                if (b < 6) { return false; }
                if (feld.art == FeldArt::Klappe) {
                    if (b == 6) {
                        const Element* item = finde(feld.wahl(), "##Combo_", 0, false);
                        if (item == nullptr) {
                            meldeFehler(std::string("Feld ") + feld.quelle + " " + feld.name + ": Eintrag \"" + feld.wahl() + "\" nicht gefunden");
                            return true;
                        }
                        setzeMaus(item->rect.GetCenter());
                    }
                    if (b == 8) { io.AddMouseButtonEvent(0, true); }
                    if (b == 9) { io.AddMouseButtonEvent(0, false); }
                    return b >= 12;
                }
                if (feld.art == FeldArt::Text) {
                    if (b == 6) {
                        io.AddKeyEvent(ImGuiMod_Ctrl, true);
                        io.AddKeyEvent(ImGuiKey_A, true);
                    }
                    if (b == 7) {
                        io.AddKeyEvent(ImGuiKey_A, false);
                        io.AddKeyEvent(ImGuiMod_Ctrl, false);
                    }
                    if (b == 8) { io.AddKeyEvent(ImGuiKey_Backspace, true); }
                    if (b == 9) { io.AddKeyEvent(ImGuiKey_Backspace, false); }
                    if (b == 10) { io.AddInputCharactersUTF8(feld.wahl().c_str()); }
                    if (b == 12) {   // aus dem Feld heraus (ImGui uebernimmt den Text sofort)
                        const ke::Editor& e = ke::editor();
                        (void)e;
                        io.AddKeyEvent(ImGuiKey_Tab, true);
                    }
                    if (b == 13) { io.AddKeyEvent(ImGuiKey_Tab, false); }
                    return b >= 15;
                }
                return true;
            }};
}

std::vector<Seite6> seiten6() {
    using A = FeldArt;
    const auto haken = [](Str s, const char* endung = "") { return [s, endung] { return trS(s) + endung; }; };
    const auto fest = [](const char* t) { return [t] { return std::string(t); }; };
    const auto kamera = [](Str s) { return [s] { return "##" + trS(s); }; };
    ke::Editor& e0 = ke::editor();
    (void)e0;
    std::vector<Seite6> v;
    v.push_back({22, "Autosave Preferences",
                 {{"radiant/autosave.cpp:168", "Autosave Frequency (minutes)", A::Plus, fest("##autosaveminuten"), nullptr, "AutosaveMinutes"},
                  {"radiant/autosave.cpp:170", "Save Snapshots", A::Haken, haken(Str::KePrAutosaveSnapshots, "##schnappschuesse"), nullptr, "Snapshots"},
                  {"radiant/autosave.cpp:167", "Enable Autosave", A::Haken, haken(Str::KePrAutosaveEnable, "##autosavean"), nullptr, "Autosave"}},
                 [] { ke::editor().autosave.an = true; }});
    v.push_back({13, "Brush Settings",
                 {{"radiant/brushmodule.cpp:86", "Default texture scale", A::Plus, fest("##wert"), nullptr, "TexdefDefaultScale"},
                  {"radiant/brushmodule.cpp:95", "New map Brush Type", A::Klappe, fest("##pinseltyp"),
                   [] { return trS(ke::editor().pinselTyp == 0 ? Str::KePrBrushBp : Str::KePrBrushAxial); }, "BrushType"},
                  {"radiant/brushmodule.cpp:102", "Always use caulk for new brushes", A::Haken, haken(Str::KePrBrushCaulk, "##immercaulk"), nullptr,
                   "BrushAlwaysCaulk"},
                  {"radiant/brushmodule.cpp:106", "Snap plane points to integer grid", A::Haken, haken(Str::KePrBrushSnap, "##ganzzahlig"), nullptr,
                   "BrushSnapPlanes"}},
                 nullptr});
    v.push_back({17, "Camera View Preferences",
                 {{"radiant/camwindow.cpp:2399", "Movement Speed", A::Plus, kamera(Str::KePrCamMoveSpeed), nullptr, "CamMoveSpeed"},
                  {"radiant/camwindow.cpp:2400", "Time to Max Speed", A::Plus, kamera(Str::KePrCamTimeMax), nullptr, "CamMoveTimeToMaxSpeed"},
                  {"radiant/camwindow.cpp:2401", "Scroll Move Speed", A::Plus, kamera(Str::KePrCamScrollSpeed), nullptr, "ScrollMoveSpeed"},
                  {"radiant/camwindow.cpp:2402", "Strafe Speed", A::Plus, kamera(Str::KePrCamStrafeSpeed), nullptr, "CamStrafeSpeed"},
                  {"radiant/camwindow.cpp:2403", "Mouse Sensitivity", A::Plus, kamera(Str::KePrCamSensitivity), nullptr, "Sensitivity"},
                  {"radiant/camwindow.cpp:2404", "Invert mouse vertical axis", A::Haken, haken(Str::KePrCamInvert), nullptr, "CamInverseMouse"},
                  {"radiant/camwindow.cpp:2405", "Zoom to Mouse pointer", A::Haken, haken(Str::KePrCamZoomPointer), nullptr, "3DZoomInToPointer"},
                  {"radiant/camwindow.cpp:2406", "Discrete movement", A::Haken, haken(Str::KePrCamDiscrete), nullptr, "CamDiscrete"},
                  {"radiant/camwindow.cpp:2411", "Enable far-clip plane", A::Haken, haken(Str::KePrCamFarClip), nullptr, "CubicClipping"},
                  {"radiant/camwindow.cpp:2416", "Colorize selection", A::Haken, haken(Str::KePrCamColorize), nullptr, "CameraFaceFill"},
                  {"radiant/camwindow.cpp:2421", "Selected faces wire", A::Haken, haken(Str::KePrCamFacesWire), nullptr, "CameraFaceWire"},
                  {"radiant/camwindow.cpp:2428", "Render Mode", A::Klappe, kamera(Str::KePrCamRenderMode),
                   [] { return trS(ke::editor().kameraE.modus == karte::KameraModus::Flach ? Str::KePrCamTextured : Str::KePrCamFlatshade); },
                   "CameraRenderMode"},
                  {"radiant/camwindow.cpp:2438", "MSAA", A::Klappe, kamera(Str::KePrCamMsaa),
                   [] { return std::string(ke::editor().kameraE.msaa == 4 ? "2" : "4"); }, "CameraMSAA"},
                  {"radiant/camwindow.cpp:2448", "Strafe Mode", A::Klappe, kamera(Str::KePrCamStrafeMode),
                   [] { return trS(ke::editor().kameraE.strafeModus == 0 ? Str::KePrCamStrafeUp : Str::KePrCamStrafeNone); }, "StrafeMode"},
                  {"radiant/camwindow.cpp:2454", "Field Of View", A::Plus, kamera(Str::KePrCamFov), nullptr, "fieldOfView"}},
                 nullptr});
    v.push_back({14, "Clipper Tool Settings",
                 {{"radiant/clippertool.cpp:170", "Caulk Clipper Cuts", A::Haken, haken(Str::KePrClipCaulk), nullptr, "ClipperCaulk"},
                  {"radiant/clippertool.cpp:171", "Reset Flipped State", A::Haken, haken(Str::KePrClipResetFlip), nullptr, "ClipperResetFlip"},
                  {"radiant/clippertool.cpp:172", "Reset Points on Split", A::Haken, haken(Str::KePrClipResetPoints), nullptr, "ClipperResetPoints"},
                  {"radiant/clippertool.cpp:174", "2 Points in 2D Views", A::Haken, haken(Str::KePrClip2Points), nullptr, "Clipper2PointsIn2D"},
                  {"radiant/clippertool.cpp:177", "On DoubleClick do:", A::Radio,
                   [] {
                       return ke::editor().bearbeitung.clipperDoppelklickTeilen == 1 ? trS(Str::KePrClipClip) + "##clipperdoppelt0"
                                                                                      : trS(Str::KePrClipSplit) + "##clipperdoppelt1";
                   },
                   nullptr, "ClipperDoubleclickedSplit"}},
                 [] { ke::editor().bearbeitung.clipperPunkteZuruecksetzen = true; }});
    v.push_back({9, "Entity Display Preferences",
                 {{"radiant/entity.cpp:669", "Names Display Distance (3D)", A::Plus, fest("##namenabstand"), nullptr, "SI_ShowNamesDist"},
                  {"radiant/entity.cpp:673", "Names Display Ratio (2D)", A::Plus, fest("##namenverhaeltnis"), nullptr, "SI_ShowNamesRatio"},
                  {"radiant/entity.cpp:677", "Entity Names = Targetnames", A::Haken, haken(Str::KePrEnTargetnames), nullptr, "SI_ShowTargetNames"}},
                 nullptr});
    v.push_back({15, "Grid Settings",
                 {{"radiant/grid.cpp:241", "Default grid spacing", A::Klappe, fest("##rasterstandard"),
                   [] { return std::string(karte::Raster::name(ke::editor().rasterStandard == 3 ? 4 : 3)); }, "GridDefault"},
                  {"radiant/grid.cpp:249", "Max grid coordinate", A::Klappe, fest("##rastermax"),
                   [] { return std::string(ke::editor().rasterMaxPotenz == 1 ? "16384" : "8192"); }, "GridMaxCoordPower"}},
                 nullptr});
    v.push_back({5, "Layout Preferences",
                 {{"radiant/mainframe.cpp:2031", "Window Layout", A::Radio,
                   [] { return std::string(ke::editor().anordnung == karte::befehle::Anordnung::RegularLeft ? "##layout0" : "##layout3"); }, nullptr,
                   "QE4StyleWindows"},
                  {"radiant/mainframe.cpp:2038", "Detachable Menus", A::Haken, haken(Str::KePrDetachableMenus, "##abreissbar"), nullptr, "DetachableMenus"},
                  {"radiant/mainframe.cpp:2043", "Built-In Group Dialog", A::Haken, haken(Str::KePrBuiltInGroupDialog), nullptr, "BuiltInGroupDialog"}},
                 nullptr});
    v.push_back({10, "Settings",
                 {{"radiant/select.cpp:1921", "Nudge selected after duplication", A::Haken, haken(Str::KePrNudgeClone, "##nachklonen"), nullptr,
                   "NudgeAfterClone"},
                  {"radiant/map.cpp:2370", "Load last map at startup", A::Haken, haken(Str::KePrLoadLastMap, "##letztekarte"), nullptr, "LoadLastMap"}},
                 nullptr});
    v.push_back({21, "Model Browser Preferences",
                 {{"radiant/modelwindow.cpp:1366", "Model View Size", A::Plus, fest("##modzelle"), nullptr, "ModelBrowserCellSize"},
                  {"radiant/modelwindow.cpp:1369", "List of *folderToLoad/depth*", A::Text, fest("##modordner"), fest("*models/s6*"), "ModelBrowserFolders"}},
                 nullptr});
    v.push_back({8, "Patch Display Preferences",
                 {{"radiant/patchmanip.cpp:673", "Patch Subdivide Threshold", A::Plus, fest("##patchschwelle"), nullptr, "Subdivisions"}}, nullptr});
    v.push_back({4, "Interface Preferences",
                 {{"radiant/preferences.cpp:64", "Shader Editor Command", A::Text, fest("##shadereditor"), fest("s6editor.exe"), "CustomShaderEditorCommand"}},
                 nullptr});
    v.push_back({1, "Global Game",
                 {{"radiant/preferences.cpp:267", "Startup / Show Global Preferences", A::Haken, haken(Str::KePrShowGlobal, "##spielfrage"), nullptr,
                   "gamePrompt"}},
                 nullptr});
    v.push_back({12, "Selection System Settings",
                 {{"radiant/selection.cpp:2314", "Selector size (pixels)", A::Plus, fest("##waehlergroesse"), nullptr, "SELECT_EPSILON"},
                  {"radiant/selection.cpp:2315", "Prefer point entities in 2D", A::Haken, haken(Str::KePrSelPreferPoint), nullptr, "PreferPointEntsIn2D"},
                  {"radiant/selection.cpp:2316", "Create brushes in 3D", A::Haken, haken(Str::KePrSel3dBrushes), nullptr, "3DCreateBrushes"},
                  {"radiant/selection.cpp:2319", "Move style in 3D", A::Klappe, fest("##bewegstil"),
                   [] { return trS(ke::editor().bearbeitung.bewegStil3d == 0 ? Str::KePrSelMoveView : Str::KePrSelMoveXY); }, "3DMoveStyle"}},
                 nullptr});
    v.push_back({16, "Surface Inspector Preferences",
                 {{"radiant/surfacedialog.cpp:1735", "Surface Inspector Increments Match Grid", A::Haken, haken(Str::KePrSiMatchGrid), nullptr, "SnapTToGrid"}},
                 nullptr});
    v.push_back({7, "Texture Settings",
                 {{"radiant/textures.cpp:764", "Texture Quality", A::Radio,
                   [] { return ke::editor().texQual.mipStufe == 1 ? std::string("100%##texqual0") : std::string("50%##texqual1"); }, nullptr,
                   "TextureMipLevel"},
                  {"radiant/textures.cpp:771", "Texture Gamma", A::Plus, fest("##texgamma"), nullptr, "SI_Gamma"},
                  {"radiant/textures.cpp:780", "Texture Render Mode", A::Klappe, fest("##texmodus"),
                   [] { return trS(karte::texturModusZuListe(ke::editor().texQual.modus) == 0 ? Str::KePrTexTrilinear : Str::KePrTexNearest); },
                   "TextureFiltering"},
                  {"radiant/textures.cpp:826", "Anisotropy", A::Haken, haken(Str::KePrTexAnisotropy), nullptr, "TextureAnisotropy"}},
                 nullptr});
    v.push_back({20, "Texture Browser Preferences",
                 {{"radiant/texwindow.cpp:2045", "Texture scrollbar", A::Haken, haken(Str::KePrTbScrollbar), nullptr, "TextureScrollbar"},
                  {"radiant/texwindow.cpp:2052", "Texture Thumbnail Scale", A::Klappe, fest("##texskala"),
                   [] { return std::string(ke::editor().texBro.e.skala == 50 ? "100%" : "50%"); }, "TextureScale"},
                  {"radiant/texwindow.cpp:2059", "Thumbnails Max Size", A::Plus, fest("##texmax"), nullptr, "UniformTextureSize"},
                  {"radiant/texwindow.cpp:2060", "Thumbnails Min Size", A::Plus, fest("##texmin"), nullptr, "UniformTextureMinSize"},
                  {"radiant/texwindow.cpp:2061", "Mousewheel Increment", A::Plus, fest("##texrad"), nullptr, "WheelMouseInc"},
                  {"radiant/texwindow.cpp:2064", "Load Shaders at Startup", A::Klappe, fest("##texstart"),
                   [] { return ke::editor().texBro.e.startShader == 0 ? std::string("System") : trS(Str::KePrTbNone); }, "LoadShaders"},
                  {"radiant/texwindow.cpp:2068", "Hide nonShaders in common folder", A::Haken,
                   [] {
                       char text[160];
                       std::snprintf(text, sizeof(text), ke::trK(Str::KePrTbHideCommon), ke::editor().texBro.commonVerzeichnis.c_str());
                       return std::string(text);
                   },
                   nullptr, "HideNonShadersInCommon"}},
                 nullptr});
    v.push_back({11, "Undo Queue Settings",
                 {{"radiant/undo.cpp:400", "Undo Queue Size", A::Plus, fest("##undostufen"), nullptr, "UndoLevels"}}, nullptr});
    v.push_back({19, "Build Preferences",
                 {{"radiant/watchbsp.cpp:222", "Stop Compilation on Leak", A::Haken, haken(Str::KePrBuildLeak, "##bauleck"), nullptr, "BuildLeakStop"},
                  {"radiant/watchbsp.cpp:227", "Engine to Run", A::Text, fest("##bauengine"), fest("s6engine.exe"), "BuildEngineExecutable"},
                  {"radiant/watchbsp.cpp:229", "Engine Arguments", A::Text, fest("##bauargumente"), fest("+set s6 1"), "BuildEngineArgs"},
                  {"radiant/watchbsp.cpp:232", "MP Engine to Run", A::Text, fest("##bauenginemp"), fest("s6mp.exe"), "BuildEngineExecutableMP"},
                  {"radiant/watchbsp.cpp:234", "MP Engine Arguments", A::Text, fest("##bauargumentemp"), fest("+set s6mp 1"), "BuildEngineArgsMP"},
                  {"radiant/watchbsp.cpp:238", "Dump non Monitored Builds Log", A::Haken, haken(Str::KePrBuildDump, "##baulog"), nullptr, "BuildDumpLog"},
                  {"radiant/watchbsp.cpp:240", "Region Box Shader", A::Text, fest("##regionshader"), fest("textures/common/s6"), "RegionBoxShader"},
                  {"radiant/watchbsp.cpp:221", "Enable Build Process Monitoring", A::Haken, haken(Str::KePrBuildMonitor, "##bauueberwachen"), nullptr,
                   "BuildMonitor"},
                  {"radiant/watchbsp.cpp:223", "Run Engine After Compile", A::Haken, haken(Str::KePrBuildRun, "##baustarten"), nullptr, "BuildRunGame"}},
                 [] {
                     ke::editor().bau.spielStarten = true;
                     ke::editor().bau.ueberwachen = true;
                 }});
    v.push_back({18, "Orthographic View Preferences",
                 {{"radiant/xywindow.cpp:2237", "Solid selection boxes ( no stipple )", A::Haken, haken(Str::KePrOrthoStipple, "##keinestrichelung"), nullptr,
                   "NoStipple"},
                  {"radiant/xywindow.cpp:2239", "Chase mouse during drags", A::Haken, haken(Str::KePrOrthoChase, "##jagen"), nullptr, "ChaseMouse"},
                  {"radiant/xywindow.cpp:2240", "Zoom to Mouse pointer", A::Haken, haken(Str::KePrCamZoomPointer, "##zumzeiger2d"), nullptr, "2DZoomInToPointer"},
                  {"radiant/xywindow.cpp:2245", "MSAA", A::Klappe, fest("##xymsaa"), [] { return std::string(ke::editor().anzeige.msaa == 4 ? "2" : "4"); },
                   "XYMSAA"}},
                 nullptr});
    return v;
}

std::vector<Schritt> felder() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 6: Einstellungen merken (am Ende zurueck)", [] {
        grundzustand();
        ke::einstellungenSpeichern();
        einstellungenAnfang() = g_app->settings.karte;
    }));
    const auto oeffnen = [&](int index) {
        add(tu("Edit > Preferences...", [] { (void)ke::ausfuehren("Preferences"); }));
        add(pause(0.3));
        add(klickAn("Seite " + std::to_string(index), [index] {
            const Element* el = findeEndung("##seite" + std::to_string(index), "###karteeinstellungen");
            return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
        }));
        add(pause(0.2));
    };
    for (const Seite6& seite : seiten6()) {
        const Seite6 sw = seite;
        add(tu(std::string("Seite ") + seite.titel + ": vorbereiten, Werte merken", [sw] {
            if (sw.vorbereiten) { sw.vorbereiten(); }
            ke::einstellungenSpeichern();
            einstellungenVorher() = g_app->settings.karte;
        }));
        oeffnen(seite.index);
        add(pruefe(std::string("Seite ") + seite.titel + ": im Baum gewaehlt, Felder da", [sw] {
            const Feld& f = sw.felder.front();
            bool da = false;
            if (f.art == FeldArt::Klappe) {
                for (const auto& k : ke::editor().klappOrte) { da = da || k.id == f.ziel(); }
            } else {
                da = (f.art == FeldArt::Plus ? finde("+", "###karteeinstellungen", 0, false, f.ziel()) : finde(f.ziel(), "###karteeinstellungen", 0, false)) != nullptr;
            }
            return ke::editor().einstellungenOffen && da;
        }));
        for (const Feld& f : seite.felder) {
            add(feldAendern(f));
            add(pause(0.1));
        }
        add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
        add(pause(0.3));
        for (const Feld& f : seite.felder) {
            const std::string schluessel = f.schluessel;
            add(pruefe(std::string("Feld ") + f.quelle + ": " + f.name + " - nach OK geaendert und gespeichert (" + schluessel + ")", [schluessel] {
                const auto it = einstellungenVorher().find(schluessel);
                const std::string alt = it != einstellungenVorher().end() ? it->second : std::string("<fehlt>");
                const std::string neu = einstellungJetzt(schluessel.c_str());
                diag::detail("Knopftest: Stufe 6 - Feld " + schluessel + ": " + alt + " -> " + neu);
                return !ke::editor().einstellungenOffen && neu != alt && neu != "<fehlt>";
            }));
        }
        // Cancel verwirft: das erste Feld noch einmal aendern, Cancel
        const Feld erstes = seite.felder.front();
        add(tu(std::string("Seite ") + seite.titel + ": Werte nach OK merken", [] {
            ke::einstellungenSpeichern();
            einstellungenVorher() = g_app->settings.karte;
        }));
        oeffnen(seite.index);
        add(feldAendern(erstes));
        add(klick(ke::trK(Str::KeCancel), "###karteeinstellungen"));
        add(pause(0.3));
        const std::string schluessel = erstes.schluessel;
        add(pruefe(std::string("Seite ") + seite.titel + ": OK uebernimmt, Cancel verwirft (" + schluessel + " bleibt)", [schluessel] {
            return !ke::editor().einstellungenOffen && einstellungJetzt(schluessel.c_str()) == einstellungenVorher()[schluessel];
        }));
    }
    add(tu("Stufe 6: Einstellungen zurueck", [] {
        karteDialogeZu();
        g_app->settings.karte = einstellungenAnfang();
        ke::einstellungenLaden();
        ke::einstellungenSpeichern();
        grundzustand();
    }));
    return s;
}

// --- Teil "dialoge": jeder Knopf der Dialoge --------------------------------------
//
// Je Knopf: die Vorbedingung, der Dialog wird geoeffnet (Befehl oder Rueckruf),
// der Zustand erfasst, dann wird der Knopf wie von einem Menschen angeklickt
// (ggf. mehrere Klicks: Zeile waehlen, dann Knopf) und die erwartete Wirkung
// geprueft. Pruefname "Dialog <NRC-Titel> > <Knopf>: ...".
struct DialogKnopf {
    const char* dialog;     // Titel wie in der NRC-Checkliste
    const char* knopf;      // Anzeige im Pruefnamen
    Vor vor;
    std::function<void()> oeffnen;
    std::vector<std::function<std::string()>> klicks;   // Endungen der Elemente
    const char* wirkung;
    bool oeffnenErfasst = false;   // das Oeffnen gehoert zum Ausloesen (Zustand vor dem Oeffnen erfassen)
};

std::function<std::string()> endung(const char* t) {
    return [t] { return std::string(t); };
}
std::function<std::string()> beschriftet(Str s) {
    return [s] { return trS(s); };
}
std::function<void()> befehl(const char* n) {
    return [n] { (void)ke::ausfuehren(n); };
}

std::vector<DialogKnopf> dialogKnoepfe() {
    std::vector<DialogKnopf> v;
    const auto add = [&v](DialogKnopf d) { v.push_back(std::move(d)); };
    // Build Menu (BuildMenuCustomize, build.cpp:1027)
    add({"Build Menu", "OK", Vor::Leer, befehl("BuildMenuCustomize"), {endung("##bauOK")}, "D"});
    add({"Build Menu", "Cancel", Vor::Leer, befehl("BuildMenuCustomize"), {endung("##bauCancel")}, "D"});
    add({"Build Menu", "Reset", Vor::Leer, befehl("BuildMenuCustomize"), {endung("##bauReset")}, "D"});
    // Mapped Commands (Shortcuts, commands.cpp:333): eine Zeile waehlen, dann der Knopf
    add({"Mapped Commands", "Edit", Vor::Leer, befehl("Shortcuts"), {endung("ArbitraryRotation##tz"), beschriftet(Str::KeEdit)}, "D"});
    add({"Mapped Commands", "Clear", Vor::Leer, befehl("Shortcuts"), {endung("ArbitraryRotation##tz"), beschriftet(Str::KeClear)}, "E"});
    add({"Mapped Commands", "Reset", Vor::Leer,
         [] {
             ke::editor().belegung.setzen("ArbitraryRotation", karte::befehle::kKeyF1 + 6);
             ke::einstellungenSpeichern();
             (void)ke::ausfuehren("Shortcuts");
         },
         {endung("ArbitraryRotation##tz"), beschriftet(Str::KeReset)}, "E"});
    add({"Mapped Commands", "Reset All", Vor::Leer, befehl("Shortcuts"), {beschriftet(Str::KeResetAll)}, "D"});
    add({"Mapped Commands", "Reset All > Yes", Vor::Leer,
         [] {
             ke::editor().belegung.setzen("GridUp", karte::befehle::kKeyF1 + 6);
             ke::einstellungenSpeichern();
             (void)ke::ausfuehren("Shortcuts");
         },
         {beschriftet(Str::KeResetAll), beschriftet(Str::KeYes)}, "E"});
    // CSG Tool (csg.cpp:1486)
    for (const auto& [knopf, kennung, wirkung] : std::vector<std::tuple<const char*, const char*, const char*>>{
             {"Shrink", "##csgschrumpfen", "K"},     {"Expand", "##csgerweitern", "K"},   {"Diagonal", "##csgdiag", "K"},
             {"Wrap", "##csgwrap", "K"},             {"Extrude", "##csgextrude", "K"},    {"Pull", "##csgpull", "K"},
             {"+faces", "##csgplusfaces", "D"},      {"-proj", "##csgminusproj", "D"},    {"-cam", "##csgminuscam", "D"},
             {"Caulk", "##csgcaulk", "D"},           {"Remove inner", "##csginnenweg", "D"}, {"Offset up", "##csgdickehoch", "D"},
             {"Offset down", "##csgdickerunter", "D"}}) {
        add({"CSG Tool", knopf, Vor::Pinsel, befehl("CSGTool"), {endung(kennung)}, wirkung});
    }
    add({"CSG Tool", "-faces", Vor::Pinsel,
         [] {
             (void)ke::ausfuehren("CSGTool");
             ke::editor().csgWerkzeug.ausschluss = 1;
         },
         {endung("##csgminusfaces")}, "D"});
    // Entity List (entitylist.cpp:279): Haken "Auto focus"
    add({"Entity List", "Auto focus", Vor::Leer, befehl("ToggleEntityList"), {endung("##elautofocus")}, "D"});
    // Find / Replace Texture(s) (findtexturedialog.cpp:132)
    add({"Find / Replace Texture(s)", "Apply", Vor::Leer,
         [] {
             (void)ke::ausfuehren("FindReplaceTextures");
             ke::editor().findenText = "common/caulk";
             ke::editor().ersetzenText = "common/s6";
         },
         {beschriftet(Str::KeFrApply)}, "K"});
    add({"Find / Replace Texture(s)", "Close", Vor::Leer, befehl("FindReplaceTextures"), {beschriftet(Str::KeFrClose)}, "D"});
    add({"Find / Replace Texture(s)", "Within selected brushes only", Vor::Leer, befehl("FindReplaceTextures"), {beschriftet(Str::KeFrSelected)}, "D"});
    // Arbitrary sides (gtkdlgs.cpp:274)
    add({"Arbitrary sides", "OK", Vor::Pinsel, befehl("BrushPrism"), {endung("##seitenok")}, "K"});
    add({"Arbitrary sides", "Cancel", Vor::Pinsel, befehl("BrushPrism"), {endung("##seitenabbrechen")}, "D"});
    add({"Arbitrary sides", "Truncate", Vor::Pinsel, befehl("BrushIcosahedron"), {endung("##seitenstutzen")}, "D"});
    // About NetRadiant (gtkdlgs.cpp:333); Credits ist wie bei NRC gesperrt (button->setEnabled( false ))
    add({"About NetRadiant", "OK", Vor::Leer, befehl("DoAbout"), {endung("##ueberok")}, "D"});
    add({"About NetRadiant", "Changelog", Vor::Leer, befehl("DoAbout"), {endung("##uebernotizen")}, "X"});
    add({"About NetRadiant", "About Qt", Vor::Leer, befehl("DoAbout"), {endung("##ueberqt")}, "D"});
    // Set Light intensity (gtkdlgs.cpp:415): ein Licht aus dem Entitaetenmenue
    // Entity_createFromSelection (entity.cpp:474-483): das Licht entsteht mit dem Menue, OK setzt "light",
    // Cancel laesst es ohne - beides ein Rueckgaengig-Schritt "entityCreate"
    add({"Set Light intensity", "OK", Vor::Leer, [] { ke::entitaetMenueGewaehlt("light"); }, {endung("##lichtok")}, "K", true});
    add({"Set Light intensity", "Cancel", Vor::Leer, [] { ke::entitaetMenueGewaehlt("light"); }, {endung("##lichtabbrechen")}, "K", true});
    add({"Set Light intensity", "Don't show", Vor::Leer, [] { ke::entitaetMenueGewaehlt("light"); }, {beschriftet(Str::KeLiDontShow)}, "D"});
    // Choose Color (QColorDialog, EntityColorSet)
    add({"Choose Color", "OK", Vor::Licht,
         [] {
             (void)ke::ausfuehren("EntityColorSet");
             ke::editor().ent.farbDialog.r = 10;
         },
         {endung("##cdok")}, "K"});
    add({"Choose Color", "Cancel", Vor::Licht, befehl("EntityColorSet"), {endung("##cdabbrechen")}, "D"});
    // Engine Path Configuration (mainframe.cpp:421)
    add({"Engine Path Configuration", "OK", Vor::Leer, [] { ke::editor().enginePfadDialogOffen = true; }, {endung("##enginepfadok")}, "D"});
    // Map Info (map.cpp:908): Spaltenkopf sortiert
    add({"Map Info", "Spalte Anzahl", Vor::Leer, befehl("MapInfo"), {endung("##infokopf1")}, "D"});
    // Find Brush (map.cpp:2085)
    add({"Find Brush", "Find", Vor::Leer, befehl("FindBrush"), {endung("##suchefinden")}, "A"});
    add({"Find Brush", "Close", Vor::Leer, befehl("FindBrush"), {endung("##sucheschliessen")}, "D"});
    // Patch density / deform / Cap / thicken (patchmanip.cpp)
    add({"Patch density", "OK", Vor::Leer, befehl("SimplePatchMesh"), {endung("##patchok")}, "K"});
    add({"Patch density", "Cancel", Vor::Leer, befehl("SimplePatchMesh"), {endung("##patchabbrechen")}, "D"});
    add({"Patch deform", "OK", Vor::Patch, befehl("PatchDeform"), {endung("##patchok")}, "K"});
    add({"Patch deform", "Cancel", Vor::Patch, befehl("PatchDeform"), {endung("##patchabbrechen")}, "D"});
    add({"Cap", "OK", Vor::Patch, befehl("CapCurrentCurve"), {endung("##patchok")}, "K"});
    add({"Cap", "Cancel", Vor::Patch, befehl("CapCurrentCurve"), {endung("##patchabbrechen")}, "D"});
    add({"Patch thicken", "OK", Vor::Patch, befehl("PatchThicken"), {endung("##patchok")}, "K"});
    add({"Patch thicken", "Cancel", Vor::Patch, befehl("PatchThicken"), {endung("##patchabbrechen")}, "D"});
    add({"Patch thicken", "Side walls", Vor::Patch, befehl("PatchThicken"), {endung("##patchwaende")}, "D"});
    // Global Preferences (preferences.cpp:271)
    add({"Global Preferences", "OK", Vor::Leer, [] { ke::editor().globaleEinstellungenOffen = true; }, {endung("##globalok")}, "D"});
    // NetRadiant Preferences (preferences.cpp:584): Clean (OK/Cancel pruefen die Seiten im Teil "felder")
    add({"NetRadiant Preferences", "Clean", Vor::Leer, befehl("Preferences"), {beschriftet(Str::KePrefClean)}, "D"});
    add({"NetRadiant Preferences", "Clean > Yes", Vor::Leer,
         [] {
             ke::editor().nachKlonenSchubsen = !ke::editor().nachKlonenSchubsen;
             ke::einstellungenSpeichern();
             (void)ke::ausfuehren("Preferences");
         },
         {beschriftet(Str::KePrefClean), endung("##meldungja")}, "E"});
    // Arbitrary rotation / scale (select.cpp:1589/1681)
    for (const char* knopf : {"OK", "Apply", "Cancel"}) {
        const std::string k = knopf;
        const char* drehEnde = k == "OK" ? "##kartedrehenok" : k == "Apply" ? "##kartedrehenanwenden" : "##kartedrehenabbrechen";
        const char* skalEnde = k == "OK" ? "##karteskalierenok" : k == "Apply" ? "##karteskalierenanwenden" : "##karteskalierenabbrechen";
        add({"Arbitrary rotation", knopf, Vor::Pinsel,
             [] {
                 (void)ke::ausfuehren("ArbitraryRotation");
                 ke::editor().drehDialog.werte = {0.0, 0.0, 45.0};
             },
             {endung(drehEnde)}, k == "Cancel" ? "D" : "K"});
        add({"Arbitrary scale", knopf, Vor::Pinsel,
             [] {
                 (void)ke::ausfuehren("ArbitraryScale");
                 ke::editor().skalierDialog.werte = {2.0, 1.0, 1.0};
             },
             {endung(skalEnde)}, k == "Cancel" ? "D" : "K"});
    }
    // Surface Inspector (surfacedialog.cpp:637)
    add({"Surface Inspector", "Reset", Vor::PinselVersetzt, befehl("SurfaceInspector"), {endung("##sireset")}, "K"});
    add({"Surface Inspector", "Fit", Vor::Pinsel, befehl("SurfaceInspector"), {endung("##sifit")}, "K"});
    add({"Surface Inspector", "Width", Vor::Pinsel, befehl("SurfaceInspector"), {endung("##siwidth")}, "K"});
    add({"Surface Inspector", "Height", Vor::Pinsel, befehl("SurfaceInspector"), {endung("##siheight")}, "K"});
    add({"Surface Inspector", "Axial", Vor::Patch, befehl("SurfaceInspector"), {endung("##siaxial")}, "K"});
    add({"Surface Inspector", "Ortho", Vor::Patch, befehl("SurfaceInspector"), {endung("##siortho")}, "K"});
    add({"Surface Inspector", "Cam", Vor::Patch, befehl("SurfaceInspector"), {endung("##sicam")}, "K"});
    add({"Surface Inspector", "Natural", Vor::Patch, befehl("SurfaceInspector"), {endung("##sinatural")}, "K"});
    add({"Surface Inspector", "Match Grid", Vor::Pinsel, befehl("SurfaceInspector"), {endung("##simatchgrid")}, "E"});
    // OpenGL Font (QFontDialog)
    add({"Select Font", "OK", Vor::Leer, befehl("OpenGLFont"), {endung("##schriftok")}, "D"});
    add({"Select Font", "Cancel", Vor::Leer, befehl("OpenGLFont"), {endung("##schriftabbrechen")}, "D"});
    return v;
}

std::vector<Schritt> dialoge() {
    std::vector<Schritt> s;
    for (const DialogKnopf& d : dialogKnoepfe()) {
        Art a;
        a.vor = d.vor;
        a.soll = maskeAus(d.wirkung);
        if (!d.oeffnenErfasst) {
            a.vorher = d.oeffnen;
            a.warten = 4;
        }
        a.mitUndo = (a.soll & wKarte) != 0U;
        const auto klicks = d.klicks;
        const auto oeffnen = d.oeffnenErfasst ? d.oeffnen : std::function<void()>();
        s.push_back(wirkungsSchritt(std::string("Dialog ") + d.dialog + " > " + d.knopf + ": " + maskeText(a.soll) + " (" + vorName(d.vor) + ")", "", a,
                                    [klicks, oeffnen](std::string* fehler) {
                                        auto folge = std::make_shared<std::function<bool(int)>>(klickFolge(klicks, "", fehler));
                                        return std::function<bool(int)>([oeffnen, folge](int b) {
                                            if (!oeffnen) { return (*folge)(b); }
                                            if (b == 0) { oeffnen(); }
                                            return b < 5 ? false : (*folge)(b - 5);
                                        });
                                    }));
    }
    return s;
}

// --- Teil "funde": bekannte Fehler aus NRC/GtkRadiant, nachgestellt im laufenden Programm ---

std::size_t komponentenGewaehlt(bool ecken) {
    std::size_t n = 0;
    for (const auto& ent : ke::editor().karte.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            for (const auto& f : static_cast<karte::Pinsel&>(*p).flaechen()) { n += ecken ? f->eckenWahl.size() : f->kantenWahl.size(); }
        }
    }
    return n;
}

std::vector<Schritt> funde() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // NRC#65: nach dem Verlassen des Ecken- bzw. Kantenmodus blieb die Teilauswahl
    // aktiv, der naechste Zug verformte den Pinsel. Garux: "Probably should just do
    // deselection, at least for vertices and edges". Der Flaechenmodus bleibt wie NRC.
    struct Modus {
        const char* name;
        ImGuiKey taste;
        karte::KomponentenModus modus;
        karte::Waehlbar::Art art;
    };
    for (const Modus& m : {Modus{"Ecken", ImGuiKey_V, karte::KomponentenModus::Ecke, karte::Waehlbar::Art::Ecke},
                           Modus{"Kanten", ImGuiKey_E, karte::KomponentenModus::Kante, karte::Waehlbar::Art::Kante}}) {
        const Modus mm = m;
        add(tu(std::string("NRC#65: Pinsel A, ") + m.name + "modus, eine " + m.name + " gewaehlt", [mm] {
            vorbereiten(Vor::Pinsel);
            ke::Editor& e = ke::editor();
            e.bearbeitung.einstellung.modus = karte::AuswahlModus::Komponente;
            e.bearbeitung.einstellung.komponente = mm.modus;
            if (const auto p = std::dynamic_pointer_cast<karte::Pinsel>(weltKind(0))) {
                (void)p->ecken(e.karte.kontext);   // m_ecken / m_kanten aufbauen
                (void)p->kanten(e.karte.kontext);
                karte::Waehlbar w;
                w.art = mm.art;
                w.entitaet = e.karte.welt();
                w.primitiv = p;
                w.index = 0;
                karte::setzeGewaehlt(e.karte, w, true);
            }
            ke::auswahlGeaendert();
        }));
        add(pruefe(std::string("NRC#65: ") + m.name + " gewaehlt", [mm] { return komponentenGewaehlt(mm.modus == karte::KomponentenModus::Ecke) > 0; }));
        add(tasteKombi(m.taste, m.taste == ImGuiKey_V ? "V" : "E"));
        add(pruefe(std::string("NRC#65: ") + m.name + "modus verlassen (Taste noch einmal): Primitivmodus, keine " + m.name + " mehr gewaehlt", [mm] {
            const ke::Editor& e = ke::editor();
            return e.bearbeitung.einstellung.modus == karte::AuswahlModus::Primitiv && komponentenGewaehlt(mm.modus == karte::KomponentenModus::Ecke) == 0 &&
                   e.karte.auswahl.anzahl() == 1;
        }));
    }
    add(tu("NRC#65: Flaechenmodus mit der oberen Flaeche von A", [] { vorbereiten(Vor::Flaeche); }));
    add(tasteKombi(ImGuiKey_F, "F"));
    add(pruefe("NRC#65: Flaechenmodus verlassen: die Flaeche bleibt gewaehlt (wie NRC: Flaechen dienen dem Extrudieren)", [] {
        const ke::Editor& e = ke::editor();
        return e.bearbeitung.einstellung.modus == karte::AuswahlModus::Primitiv && e.karte.auswahl.flaechen().size() == 1;
    }));
    // NRC#164: ein relativer Engine-Pfad (../../Academy/) loeste beim Speichern eine
    // Zusicherung aus; die Werkzeuge bekommen ihn ausserdem relativ zu ihrem eigenen
    // Arbeitsordner. Jetzt wird er beim Setzen absolut (zum Programmordner).
    auto engineAlt = std::make_shared<std::string>();
    add(tu("NRC#164: Grundzustand, Engine-Pfad merken, Edit > Preferences > Paths", [engineAlt] {
        grundzustand();
        *engineAlt = ke::editor().enginePfad;
        (void)ke::ausfuehren("Preferences");
    }));
    add(pause(0.3));
    add(klickAn("Seite Paths", [] {
        const Element* el = findeEndung("##seite3", "###karteeinstellungen");
        return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
    }));
    add(pause(0.2));
    add(feldAendern(Feld{"radiant/mainframe.cpp:398", "Engine Path", FeldArt::Text, [] { return std::string("##enginepfad"); },
                         [] { return std::string("../"); }, "EnginePath"}));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(warteBis("NRC#164: Spiel neu geladen", [] { return ke::spielBereit(); }, 3000));
    add(pause(0.3));
    add(pruefe("NRC#164: der Engine-Pfad ist absolut (Programmordner/..), gespeichert", [] {
        const ke::Editor& e = ke::editor();
        const std::filesystem::path soll = (std::filesystem::path(platform::executableDirectory()) / "..").lexically_normal();
        const std::u8string u = soll.u8string();
        const std::string erwartet = karte::ordnerBereinigt(std::string(u.begin(), u.end()));
        diag::detail("Knopftest: Stufe 6 - NRC#164 Engine-Pfad \"" + e.enginePfad + "\", erwartet \"" + erwartet + "\"");
        return std::filesystem::path(std::u8string(e.enginePfad.begin(), e.enginePfad.end())).is_absolute() && e.enginePfad == erwartet &&
               einstellungJetzt("EnginePath") == erwartet;
    }));
    add(tu("NRC#164: aendern und Save", [] {
        ke::Editor& e = ke::editor();
        (void)ke::karteOeffnen(fixturePfad());
        e.karte.auswahl.waehlen(e.karte.welt(), weltKind(0));
        (void)ke::ausfuehren("SelectNudgeUp");
        (void)ke::ausfuehren("SaveMap");
    }));
    add(pruefe("NRC#164: Save mit diesem Engine-Pfad: gespeichert, keine Zusicherung", [] { return !ke::geaendert(); }));
    add(tu("NRC#164: Engine-Pfad zurueck", [engineAlt] {
        const std::string a = *engineAlt;
        ke::spielNeuRealisieren("Engine Path Configuration", [a] {
            ke::editor().enginePfad = a;
            ke::einstellungenSpeichern();
        });
    }));
    add(warteBis("NRC#164: Spiel wieder geladen", [] { return ke::spielBereit(); }, 3000));
    // NRC#213: AutoCaulk scheiterte nur mit "reading ..._ac.caulk failure" ohne Grund.
    // Hier: bhed_q3map2 fehlt neben behaved.exe - die Konsole muss sagen, warum.
    const std::filesystem::path q3 = std::filesystem::path(platform::executableDirectory()) / "bhed_q3map2.exe";
    const std::filesystem::path q3weg = std::filesystem::path(platform::executableDirectory()) / "bhed_q3map2.s6weg";
    add(tu("NRC#213: bhed_q3map2 beiseite, Pinsel A, AutoCaulk Selected", [q3, q3weg] {
        vorbereiten(Vor::Pinsel);
        stufe3::q3map2Bereitstellen();
        std::error_code ec;
        std::filesystem::rename(q3, q3weg, ec);
        ke::konsoleLeeren();
        ke::editor().kamera.setzeUrsprung(karte::Vektor3f(0, 0, 32));
        (void)ke::ausfuehren("AutoCaulkSelected");
    }));
    add(pause(0.3));
    add(pruefe("NRC#213: die Konsole nennt den Grund (q3map2 nicht gestartet), nicht nur \"reading ..._ac.caulk failure\"", [] {
        bool failure = false;
        bool grund = false;
        for (const ke::KonsolenZeile& z : ke::editor().konsole) {
            failure = failure || z.text.find("_ac.caulk failure") != std::string::npos;
            grund = grund || (z.text.find("bhed_q3map2") != std::string::npos && z.text.find("could not be started") != std::string::npos);
        }
        return failure && grund;
    }));
    add(tu("NRC#213: bhed_q3map2 zurueck", [q3, q3weg] {
        std::error_code ec;
        std::filesystem::rename(q3weg, q3, ec);
        grundzustand();
    }));
    // AutoCaulk mit Wasser (Stufe 6, Fund): NRC ruft "q3map2 -game quake3 -autocaulk" fuer
    // jedes Spiel; bei Jedi Academy liegen die Shader aber in shaders/ (nicht scripts/),
    // q3map2 kennt dann kein Wasser und schreibt caulk auf die verdeckten Flaechen eines
    // Wasserpinsels (fest statt watercaulk). Hier: Raum mit einem Wasserpinsel
    // (textures/common/water2 aus common.shader von JA) im Boden.
    add(tu("AutoCaulk/Wasser: Raum mit common/water2 im Boden, alle Pinsel, AutoCaulk Selected", [] {
        grundzustand();
        stufe3::q3map2Bereitstellen();
        std::string t = stufe3::raumKarte(false);
        const std::string marke = "\"ambient\" \"8\"\n";
        const std::size_t p = t.find(marke);
        if (p != std::string::npos) {
            t.insert(p + marke.size(), quader(-200, -100, -8, -100, 100, 48, "common/water2") +
                                           quader(-400, -100, 0, -300, 100, 96, "byss/static_field"));   // durchsichtig, aber fest
        }
        const std::string pfad = karteTestPfad("s6_wasser.map");
        (void)textSchreiben(pfad, t);
        (void)ke::karteOeffnen(pfad);
        ke::Editor& e = ke::editor();
        for (const auto& pr : e.karte.welt()->kinder()) {
            if (pr->art() == karte::Primitiv::Art::Pinsel) { e.karte.auswahl.waehlen(e.karte.welt(), pr); }
        }
        ke::auswahlGeaendert();
        e.kamera.setzeUrsprung(karte::Vektor3f(-384, 128, 64));   // in Raum A (wie Stufe 3)
        (void)ke::ausfuehren("AutoCaulkSelected");
    }));
    add(warteBis("AutoCaulk/Wasser: fertig", [] { return !ke::bauLaeuft(); }, 6000));
    add(pruefe("AutoCaulk/Wasser: die verdeckte Unterseite des Wasserpinsels wird watercaulk, nicht caulk (q3map2 mit den Shadern des Spiels)", [] {
        const ke::Editor& e = ke::editor();
        int wasserCaulk = 0;
        int caulk = 0;
        for (const auto& pr : e.karte.welt()->kinder()) {
            if (pr->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const auto& pin = static_cast<const karte::Pinsel&>(*pr);
            bool wasser = false;
            for (const auto& f : pin.flaechen()) { wasser = wasser || f->shader() == "textures/common/water2"; }
            if (!wasser) { continue; }
            for (const auto& f : pin.flaechen()) {
                wasserCaulk += f->shader() == "textures/system/caulk_water" ? 1 : 0;   // JA: caulk_water (system.shader)
                caulk += (f->shader().find("caulk") != std::string::npos && f->shader() != "textures/system/caulk_water") ? 1 : 0;
            }
        }
        diag::detail("Knopftest: Stufe 6 - AutoCaulk/Wasser: watercaulk " + std::to_string(wasserCaulk) + ", caulk " + std::to_string(caulk) + ", " +
                     e.karte.undo.undoName());
        return wasserCaulk > 0 && caulk == 0;
    }));
    // NRC#145: AutoCaulk setzte nodraw auf feste (durchsichtige) Pinsel und machte sie damit
    // nicht fest. NRCs q3map2 faellt auf caulk zurueck, wenn common/nodraw nicht fest ist -
    // bei Jedi Academy heisst der feste aber system/nodraw_solid, system/nodraw ist nicht fest.
    add(pruefe("NRC#145: der feste durchsichtige Pinsel (byss/static_field) bekommt unten einen festen nodraw-Shader (system/nodraw_solid)", [] {
        const ke::Editor& e = ke::editor();
        int fest = 0;
        int nichtFest = 0;
        for (const auto& pr : e.karte.welt()->kinder()) {
            if (pr->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const auto& pin = static_cast<const karte::Pinsel&>(*pr);
            bool glas = false;
            for (const auto& f : pin.flaechen()) { glas = glas || f->shader() == "textures/byss/static_field"; }
            if (!glas) { continue; }
            for (const auto& f : pin.flaechen()) {
                fest += f->shader() == "textures/system/nodraw_solid" ? 1 : 0;
                nichtFest += f->shader() == "textures/system/nodraw" ? 1 : 0;
            }
        }
        diag::detail("Knopftest: Stufe 6 - NRC#145: nodraw_solid " + std::to_string(fest) + ", nodraw (nicht fest) " + std::to_string(nichtFest));
        return fest > 0 && nichtFest == 0;
    }));
    add(tu("AutoCaulk/Wasser: Grundzustand", [] { grundzustand(); }));
    // NRC#147: Textur-Kuerzel (Pfeile, Bild hoch/runter mit Umschalt/Strg) wirkten nicht,
    // solange der Surface Inspector den Fokus hatte.
    for (const auto& [taste, name, befehl] : std::vector<std::tuple<ImGuiKeyChord, const char*, const char*>>{
             {ImGuiMod_Shift | ImGuiKey_UpArrow, "Umschalt+Hoch", "TexShiftUp"},
             {ImGuiMod_Shift | ImGuiKey_DownArrow, "Umschalt+Runter", "TexShiftDown"},
             {ImGuiMod_Ctrl | ImGuiKey_UpArrow, "Strg+Hoch", "TexScaleUp"},
             {ImGuiMod_Ctrl | ImGuiKey_DownArrow, "Strg+Runter", "TexScaleDown"},
             {ImGuiMod_Shift | ImGuiKey_PageUp, "Umschalt+Bild hoch", "TexRotateCounter"},
             {ImGuiMod_Shift | ImGuiKey_PageDown, "Umschalt+Bild runter", "TexRotateClock"}}) {
        const ImGuiKeyChord t = taste;
        const std::string b2 = befehl;
        auto vorher = std::make_shared<std::string>();
        add(tu(std::string("NRC#147: Pinsel A, Surface Inspector offen (") + name + ")", [] {
            vorbereiten(Vor::Pinsel);
            ke::editor().siOffen = true;
        }));
        add(pause(0.3));
        add(klickAn("Titel des Surface Inspectors (Fokus)", [] {
            const ImRect r = fensterRahmen("###kartesi");
            return ImVec2{r.Min.x + 60.0F, r.Min.y + 8.0F};
        }));
        add(tu("NRC#147: Karte merken", [vorher] {
            *vorher = karte::karteSchreiben(ke::editor().karte);
            ke::editor().letzterBefehl.clear();
        }));
        add(tasteKombi(t, std::string(name) + " (Fokus im Surface Inspector)"));
        add(pause(0.2));
        add(pruefe(std::string("NRC#147: ") + name + " = " + befehl + " wirkt bei Fokus im Surface Inspector (Textur der Flaechen geaendert)", [vorher, b2] {
            const ke::Editor& e = ke::editor();
            const bool fokus = GImGui->NavWindow != nullptr && std::string(GImGui->NavWindow->Name).find("###kartesi") != std::string::npos;
            diag::detail("Knopftest: Stufe 6 - NRC#147 " + b2 + ": Fokus " + (GImGui->NavWindow != nullptr ? GImGui->NavWindow->Name : "-") + ", zuletzt " +
                         e.letzterBefehl);
            return fokus && e.letzterBefehl == b2 && karte::karteSchreiben(e.karte) != *vorher;
        }));
    }
    add(tu("NRC#147: Surface Inspector zu", [] { aufraeumen(); }));
    // NRC#152: die Standard-Texturskalierung nur mit 2 Nachkommastellen (0.125 ging nicht)
    add(tu("NRC#152: Edit > Preferences > Brush", [] {
        grundzustand();
        (void)ke::ausfuehren("Preferences");
    }));
    add(pause(0.3));
    add(klickAn("Seite Brush", [] {
        const Element* el = findeEndung("##seite13", "###karteeinstellungen");
        return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
    }));
    add(pause(0.2));
    add(feldAendern(Feld{"radiant/brushmodule.cpp:86", "Default texture scale", FeldArt::Text, [] { return std::string("##wert"); },
                         [] { return std::string("0.125"); }, "TexdefDefaultScale"}));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(pause(0.3));
    add(pruefe("NRC#152: Default texture scale 0.125 (vier Nachkommastellen) gespeichert und wirksam", [] {
        return einstellungJetzt("TexdefDefaultScale") == "0.125" && karte::standardTexturSkalierung() == 0.125F;
    }));
    add(tu("NRC#152: Skalierung zurueck", [] {
        karte::setzeStandardTexturSkalierung(0.25F);
        ke::einstellungenSpeichern();
    }));
    // NRC#12: Arbitrary rotation mit dem Knopf OK nur 2 Nachkommastellen genau, mit Enter voll
    auto mitKnopf = std::make_shared<std::string>();
    static auto vorlage12 = []() -> std::string& {
        static std::string v;
        return v;
    };
    for (int weg = 0; weg < 2; ++weg) {
        add(tu(std::string("NRC#12: Pinsel A, Arbitrary rotation, z = 33.333333, ") + (weg == 0 ? "OK" : "Enter"), [] {
            vorbereiten(Vor::Pinsel);
            ke::editor().drehDialog.werte = {0.0, 0.0, 0.0};
            vorlage12() = karte::karteSchreiben(ke::editor().karte);
            (void)ke::ausfuehren("ArbitraryRotation");
        }));
        add(pause(0.3));
        add(klickAn("Feld Z", stufe2::elementOrt("##kartedrehenwert2")));
        add(tippe("33.333333"));
        if (weg == 0) {
            add(klickAn("OK", stufe2::elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##kartedrehenok")));
        } else {
            add(tasteKombi(ImGuiKey_Enter, "Enter"));
        }
        add(pause(0.3));
        if (weg == 0) {
            add(tu("NRC#12: Ergebnis mit OK merken", [mitKnopf] { *mitKnopf = karte::karteSchreiben(ke::editor().karte); }));
        } else {
            add(pruefe("NRC#12: OK und Enter drehen gleich genau (33.333333 Grad, .map-Text gleich)", [mitKnopf] {
                const ke::Editor& e = ke::editor();
                return *mitKnopf == karte::karteSchreiben(e.karte) && *mitKnopf != vorlage12() && std::fabs(e.drehDialog.werte[2] - 33.333333) < 1e-6;
            }));
        }
    }
    add(tu("NRC#12: Dialog zu", [] { aufraeumen(); }));
    // NRC#261: nicht normierte BP-Verschiebungen aus anderen Radiants (Bromine, Xonotic:
    // "( ( 0.015625 0 254.75 ) ( 0 0.0078125 0.875 ) )") - der Surface Inspector zeigte
    // einen Wert ausserhalb des Spinners (+-8192); Enter/Pfeil setzte ihn dann auf 8192 und
    // verschob die Textur.
    struct Lage261 {
        std::vector<karte::Vektor2f> st;
        float breite = 0.0F;
    };
    auto lage261 = std::make_shared<Lage261>();
    const auto flaeche261 = []() -> std::shared_ptr<karte::Flaeche> {
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(ke::editor().karte.welt()->kinder().front());
        if (!p) { return nullptr; }
        for (const auto& f : p->flaechen()) {
            if (f->ebene().normal().x() < -0.5) { return f; }
        }
        return nullptr;
    };
    add(tu("NRC#261: Karte im BP-Format mit der Verschiebung 254.75 laden, die -x-Flaeche waehlen, Surface Inspector", [lage261, flaeche261] {
        grundzustand();
        const std::string pfad = karteTestPfad("s6_nrc261.map");
        (void)textSchreiben(pfad,
                            "// entity 0\n{\n\"classname\" \"worldspawn\"\n// brush 0\n{\nbrushDef\n{\n"
                            "( 1168 240 304 ) ( 1104 240 304 ) ( 1168 240 240 ) ( ( 0.015625 0 0.25 ) ( 0 0.0078125 0.875 ) ) system/caulk 0 0 0\n"
                            "( 1168 240 304 ) ( 1168 240 240 ) ( 1168 176 304 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) system/caulk 0 0 0\n"
                            "( 1104 176 240 ) ( 1168 176 240 ) ( 1104 240 240 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) system/caulk 0 0 0\n"
                            "( 1104 176 240 ) ( 1104 176 304 ) ( 1168 176 240 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) system/caulk 0 0 0\n"
                            "( 1104 176 240 ) ( 1104 240 240 ) ( 1104 176 304 ) ( ( 0.015625 0 254.75 ) ( 0 0.0078125 0.875 ) ) system/caulk 0 0 0\n"
                            "( 1168 176 304 ) ( 1104 176 304 ) ( 1168 240 304 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) system/caulk 0 0 0\n"
                            "}\n}\n}\n");
        (void)ke::karteOeffnen(pfad);
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Komponente;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Flaeche;
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(e.karte.welt()->kinder().front());
        const auto f = flaeche261();
        if (p && f) {
            e.karte.auswahl.flaecheWaehlen(p, f);
            p->bRep(e.karte.kontext);
            lage261->st.clear();
            const karte::Wicklung& w = f->wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) { lage261->st.push_back(w[i].texcoord); }
            lage261->breite = static_cast<float>(e.karte.kontext.groesse(f->shader()).first);
        }
        e.siOffen = true;
    }));
    add(pause(0.4));
    add(pruefe("NRC#261: der Surface Inspector zeigt die Verschiebung im Bereich des Spinners (+-8192), gleichwertig zu 254.75 Kacheln", [lage261] {
        const float s = ke::siAnzeigeWert(0);
        const double soll = -254.75 * lage261->breite;   // BP -> fake: shift = -coords[0][2] * Breite
        const double kacheln = (static_cast<double>(s) - soll) / std::max(1.0F, lage261->breite);
        diag::detail("Knopftest: Stufe 6 - NRC#261: Format " + std::to_string(static_cast<int>(ke::editor().karte.kontext.format)) + ", Breite " +
                     std::to_string(lage261->breite) + ", angezeigt " + std::to_string(s) + ", roh " + std::to_string(soll));
        return lage261->breite > 0.0F && std::fabs(s) <= 8192.0F && std::fabs(kacheln - std::round(kacheln)) < 1e-4;
    }));
    for (const char* pfeil : {"##sihoch0", "##sirunter0"}) {
        const std::string id = pfeil;
        add(klickAn(std::string("Horizontal shift ") + (id == "##sihoch0" ? "Pfeil hoch" : "Pfeil runter"), [id] {
            const Element* el = findeEndung(id, "###kartesi");
            return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
        }));
        add(pause(0.2));
    }
    add(pruefe("NRC#261: Pfeil hoch und wieder runter laesst die Textur der Flaeche liegen (st an jedem Eckpunkt gleich bis auf ganze Kacheln)", [lage261, flaeche261] {
        const auto f = flaeche261();
        if (!f || lage261->st.empty()) { return false; }
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(ke::editor().karte.welt()->kinder().front());
        p->bRep(ke::editor().karte.kontext);
        const karte::Wicklung& w = f->wicklung();
        double abw = 0.0;
        for (std::size_t i = 0; i < w.numpoints && i < lage261->st.size(); ++i) {
            for (int k = 0; k < 2; ++k) {
                double d = static_cast<double>(w[i].texcoord[k]) - lage261->st[i][k];
                d -= std::round(d);
                abw = std::max(abw, std::fabs(d));
            }
        }
        diag::detail("Knopftest: Stufe 6 - NRC#261: st-Abweichung nach hoch/runter " + std::to_string(abw));
        return w.numpoints == lage261->st.size() && abw < 1e-3;
    }));
    add(tu("NRC#261: aufraeumen", [] { grundzustand(); }));
    // NRC#279 (ABSICHT): die Zylinderkappe geht nur fuer Patches mit 9 Punkten Breite oder
    // Hoehe. Unter der englischen Konsolenzeile ein Hinweis in der Programmsprache.
    for (const bool deutsch : {true, false}) {
        auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
        add(tu(std::string("NRC#279: Patch P1 (3x3), Sprache ") + (deutsch ? "Deutsch" : "Englisch") + ", Patch > Cap...", [sprache, deutsch] {
            *sprache = i18n::currentLanguage();
            i18n::setLanguage(deutsch ? i18n::Language::German : i18n::Language::English);
            vorbereiten(Vor::Patch);
            ke::konsoleLeeren();
            (void)ke::ausfuehren("CapCurrentCurve");
        }));
        add(pause(0.3));
        add(tu("NRC#279: Cylinder waehlen", [] { ke::editor().patchDialog.kappe = 4; }));
        add(pause(0.1));
        add(klickAn("OK", [] {
            const Element* el = findeEndung("##patchok", "");
            return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
        }));
        add(pause(0.3));
        add(pruefe(std::string("NRC#279: ") + (deutsch ? "Deutsch: englische Zeile und darunter der Hinweis" : "Englisch: nur die englische Zeile wie NRC"),
                   [sprache, deutsch] {
                       const ke::Editor& e = ke::editor();
                       bool englisch = false;
                       bool hinweis = false;
                       for (const auto& z : e.konsole) {
                           englisch = englisch || z.text == "cannot create cylinder-cap - neither patch width, nor height is 9";
                           hinweis = hinweis || z.text == i18n::trIn(i18n::Language::German, Str::KeCapHintCylinder);
                       }
                       i18n::setLanguage(*sprache);
                       diag::detail(std::string("Knopftest: Stufe 6 - NRC#279: englisch ") + (englisch ? "ja" : "nein") + ", Hinweis " + (hinweis ? "ja" : "nein"));
                       return englisch && hinweis == deutsch;
                   }));
    }
    // NRC#132 (ABSICHT in NRC): der Region-Bau leckt, wenn die Region keine Entity hat und die
    // Kamera in einem Pinsel steht (der Startpunkt der Region-Karte liegt dann im Festen).
    // BehavEd nennt das vor dem Bau; gebaut wird wie in NRC. Gegenprobe: Kamera im Freien.
    for (const bool imPinsel : {true, false}) {
        add(tu(std::string("NRC#132: Raum ohne Entities, Region um den Raum, Kamera ") + (imPinsel ? "in der Wand" : "im Freien") + ", Build > [Final] Fast",
               [imPinsel] {
                   grundzustand();
                   std::string t = stufe3::raumKarte(false);
                   const std::size_t zweite = t.find("// entity 1");
                   if (zweite != std::string::npos) { t.resize(zweite); }
                   const std::string pfad = karteTestPfad("s6_nrc132.map");
                   (void)textSchreiben(pfad, t);
                   (void)ke::karteOeffnen(pfad);
                   ke::Editor& e = ke::editor();
                   e.region.aktiv = true;
                   e.region.min = karte::Vektor3f(-600, -300, -50);
                   e.region.max = karte::Vektor3f(600, 300, 300);
                   karte::regionAnwenden(e.karte, e.region);
                   e.kamera.ursprung = imPinsel ? karte::Vektor3f(0, 264, 128) : karte::Vektor3f(-300, 100, 128);   // Wand y 256..272 / frei
                   ke::konsoleLeeren();
                   e.bau = ke::Editor::BauEinstellungen{};
                   ke::bauMenueSicherstellen();
                   const int i = stufe3::eintragNr("[Final] Fast");
                   if (i >= 0) { ke::bauStarten(static_cast<std::size_t>(i)); }
               }));
        add(warteBis("NRC#132: Bauen fertig", [] { return !ke::bauLaeuft(); }, 6000));
        add(pruefe(std::string("NRC#132: ") + (imPinsel ? "Hinweis vor dem Bau (Kamera in der Wand, keine Entity)" : "kein Hinweis (Kamera im Freien)"), [imPinsel] {
            const ke::Editor& e = ke::editor();
            bool hinweis = false;
            bool leck = false;
            for (const auto& z : e.konsole) {
                hinweis = hinweis || z.text == ke::trK(Str::KeRegionLeckHinweis);
                leck = leck || z.text.find("LEAKED") != std::string::npos || z.text.find("leaked") != std::string::npos;
            }
            diag::detail(std::string("Knopftest: Stufe 6 - NRC#132: Hinweis ") + (hinweis ? "ja" : "nein") + ", q3map2 leaked " + (leck ? "ja" : "nein"));
            return hinweis == imPinsel && leck == imPinsel;
        }));
    }
    add(tu("NRC#132: Region aus", [] {
        ke::Editor& e = ke::editor();
        karte::regionAus(e.karte, e.region, e.raster.maxKoord());
        grundzustand();
    }));
    // NRC-master nach 20260114: cc96c84 (Door Builder: die zweite Ausrichtung hiess auch
    // "North - South") und 7027470 ("Distance Multipler"); mit den Begriffen wie NetRadiant
    add(tu("NRC master: Begriffe wie NetRadiant, Pinsel A, bobToolz Door Builder", [] {
        vorbereiten(Vor::Pinsel);
        ke::setzeNrcBegriffe(true);
        (void)ke::ausfuehren("bobToolz::DoorBuilder");
    }));
    add(pause(0.4));
    add(pruefe("NRC master cc96c84: Door Builder - die zweite Ausrichtung heisst \"East - West\"", [] {
        const Element* ew = finde("East - West", "", 0, true);
        const Element* ns = finde("North - South", "", 1, true);   // ein zweites "North - South" darf es nicht geben
        return ew != nullptr && ns == nullptr;
    }));
    add(tu("Door Builder zu, Path Plotter", [] {
        aufraeumen();
        (void)ke::ausfuehren("bobToolz::PathPlotter");
    }));
    add(pause(0.4));
    add(pruefe("NRC master 7027470: Path Plotter offen, die Beschriftung des Felds heisst \"Distance Multiplier\"", [] {
        return ke::editor().bob.dialog == 6 && std::string(ke::trK(Str::KeBtMultiplier)) == "Distance Multiplier";   // der Text wird gezeichnet (kein Element)
    }));
    add(tu("Path Plotter zu, Begriffe aus", [] {
        aufraeumen();
        ke::setzeNrcBegriffe(false);
    }));
    add(tu("Funde: Grundzustand", [] { grundzustand(); }));
    return s;
}

// --- Teil "pfade": NRC#130, Pfade ausserhalb von ASCII ------------------------------
//
// NRC: "Editor and all the tools are ASCII only" - eine Karte unter
// C:\Users\Joerg\... oeffnete nicht. Hier: ein Ordner mit Umlaut, Leerzeichen,
// chinesischen und japanischen Zeichen; Laden, Speichern, Speichern unter,
// Bauen mit bhed_q3map2 (Licht), Leck-Linie, AutoCaulk und ein Suchpfad in den
// Einstellungen.
std::string utf8Pfad(const std::filesystem::path& p) {
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

std::filesystem::path pfadOrdner() { return std::filesystem::current_path() / std::filesystem::path(u8"J\u00f6rg \u00dc \u6d4b\u8bd5 \u30de\u30c3\u30d7"); }

std::string pfadKarte(const char8_t* name) { return utf8Pfad(pfadOrdner() / std::filesystem::path(name)); }

bool pfadDa(const std::string& utf8) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::path(std::u8string(utf8.begin(), utf8.end())), ec);
}

std::vector<Schritt> pfade() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string karte = pfadKarte(u8"R\u00e4ume \u00fc \u6d4b\u8bd5.map");
    const std::string kopie = pfadKarte(u8"Kopie \u00e4 \u65e5\u672c.map");
    const std::string leck = pfadKarte(u8"Leck \u00f6 \u6d4b\u8bd5.map");
    const auto stamm = [](const std::string& p) { return p.substr(0, p.size() - 4); };
    add(tu("NRC#130: Ordner mit Umlaut, Leerzeichen, CJK; Raumkarte darin, oeffnen", [karte] {
        grundzustand();
        stufe3::q3map2Bereitstellen();
        std::error_code ec;
        std::filesystem::remove_all(pfadOrdner(), ec);
        std::filesystem::create_directories(pfadOrdner(), ec);
        (void)textSchreiben(karte, stufe3::raumKarte(false));
        (void)ke::karteOeffnen(karte);
    }));
    add(pruefe("NRC#130: Karte aus dem Ordner geladen (Name, Pinsel)", [karte] {
        const ke::Editor& e = ke::editor();
        return e.name == karte && e.karte.welt() != nullptr && e.karte.welt()->kinder().size() > 4;
    }));
    add(tu("NRC#130: aendern und Save", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        (void)ke::ausfuehren("SelectNudgeUp");
        (void)ke::ausfuehren("SaveMap");
    }));
    add(pruefe("NRC#130: Save schreibt in den Ordner (die Datei gelesen = die Karte, nicht geaendert)", [karte] {
        karte::Karte k;
        (void)karte::karteLesen(dateiInhalt(karte), k);
        return !ke::geaendert() && karte::karteSchreiben(k) == karte::karteSchreiben(ke::editor().karte);
    }));
    add(tu("NRC#130: Save As in denselben Ordner", [kopie] {
        dialogAntwort() = kopie;
        ke::editor().dateiDialog = [](const std::string&) { return dialogAntwort(); };
        (void)ke::ausfuehren("SaveMapAs");
    }));
    add(pruefe("NRC#130: Save As: Datei da, Name gewechselt, oben in der zuletzt-Liste", [kopie] {
        const ke::Editor& e = ke::editor();
        return pfadDa(kopie) && e.name == kopie && e.zuletzt.anzahl() > 0 && e.zuletzt.pfade()[0] == kopie;
    }));
    add(tu("NRC#130: die Kopie neu oeffnen, Build > [Final] Fast", [kopie] {
        (void)ke::karteOeffnen(kopie);
        ke::editor().bau = ke::Editor::BauEinstellungen{};
        ke::bauMenueSicherstellen();
        const int i = stufe3::eintragNr("[Final] Fast");
        if (i >= 0) { ke::bauStarten(static_cast<std::size_t>(i)); }
    }));
    add(warteBis("NRC#130: Bauen fertig", [] { return !ke::bauLaeuft(); }, 6000));
    add(pruefe("NRC#130: .bsp mit Licht neben der Karte im Ordner (bhed_q3map2 mit UTF-8-Pfad)", [kopie, stamm] {
        const std::string bsp = stamm(kopie) + ".bsp";
        const std::vector<std::string> l = stufe3::bspLumps(utf8Pfad(std::filesystem::path(std::u8string(bsp.begin(), bsp.end()))));
        diag::detail("Knopftest: Stufe 6 - NRC#130 Bau: " + std::to_string(l.size()) + " Lumps, Licht " + (l.size() > 14 ? std::to_string(l[14].size()) : "-"));
        return pfadDa(bsp) && l.size() == 18 && !l[14].empty();
    }));
    add(tu("NRC#130: Karte mit Loch im Ordner, Build > [Final] Fast", [leck] {
        (void)textSchreiben(leck, stufe3::raumKarte(true));
        (void)ke::karteOeffnen(leck);
        const int i = stufe3::eintragNr("[Final] Fast");
        if (i >= 0) { ke::bauStarten(static_cast<std::size_t>(i)); }
    }));
    add(warteBis("NRC#130: Bauen (Leck) fertig", [] { return !ke::bauLaeuft(); }, 6000));
    add(tu("NRC#130: File > Pointfile", [] {
        karteDialogeZu();
        if (ke::editor().punktdateiSichtbar) { ke::punktdateiUmschalten(); }
        (void)ke::ausfuehren("TogglePointfile");
    }));
    add(pruefe("NRC#130: die Leck-Linie aus dem Ordner gelesen (.lin neben der Karte, Punkte sichtbar)", [leck, stamm] {
        const ke::Editor& e = ke::editor();
        return pfadDa(stamm(leck) + ".lin") && e.punktdateiSichtbar && e.punktdatei.size() >= 2;
    }));
    add(tu("NRC#130: dichte Karte, alle Pinsel, AutoCaulk Selected", [karte] {
        karteDialogeZu();
        (void)ke::karteOeffnen(karte);
        ke::Editor& e = ke::editor();
        for (const auto& p : e.karte.welt()->kinder()) {
            if (p->art() == karte::Primitiv::Art::Pinsel) { e.karte.auswahl.waehlen(e.karte.welt(), p); }
        }
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("AutoCaulkSelected");
    }));
    add(warteBis("NRC#130: AutoCaulk fertig", [] { return !ke::bauLaeuft(); }, 6000));
    add(pause(0.3));
    add(pruefe("NRC#130: AutoCaulk im Ordner: _ac.map und _ac.caulk, Rueckgaengig \"AutoCaulk N faces\" mit N > 0", [karte, stamm] {
        const std::string u = ke::editor().karte.undo.undoName();
        diag::detail("Knopftest: Stufe 6 - NRC#130 AutoCaulk: " + u);
        return pfadDa(stamm(karte) + "_ac.map") && pfadDa(stamm(karte) + "_ac.caulk") && u.rfind("AutoCaulk ", 0) == 0 && u != "AutoCaulk 0 faces";
    }));
    // Einstellungen: ein Suchpfad mit Umlaut und CJK (Extra Resource Path), gespeichert und vom Dateisystem des Spiels gelesen
    auto alt = std::make_shared<std::string>();
    add(tu("NRC#130: Extra Resource Path im Ordner (maps/s6pfad.txt)", [alt] {
        grundzustand();   // ungeaenderte Karte: kein ConfirmModified vor dem Neuladen des Spiels
        const std::filesystem::path zusatz = pfadOrdner() / std::filesystem::path(u8"zus\u00e4tz \u6d4b");
        std::error_code ec;
        std::filesystem::create_directories(zusatz / "maps", ec);
        (void)textSchreiben(utf8Pfad(zusatz / "maps" / "s6pfad.txt"), "s6");
        *alt = ke::editor().zusatzPfade[0];
        const std::string neu = karte::ordnerBereinigt(utf8Pfad(zusatz));
        ke::spielNeuRealisieren("Edit Project Settings", [neu] { ke::editor().zusatzPfade[0] = neu; });
        ke::einstellungenSpeichern();
    }));
    add(warteBis("NRC#130: Spiel neu geladen", [] { return ke::spielBereit(); }, 3000));
    add(pause(0.5));
    add(pruefe("NRC#130: der Suchpfad ist gespeichert und das Spiel findet maps/s6pfad.txt darin", [] {
        const ke::Editor& e = ke::editor();
        ke::einstellungenSpeichern();
        const auto it = g_app->settings.karte.find("ExtraResourcePath0");
        const std::string gefunden = e.karteSpiel ? e.karteSpiel->fs.finden("maps/s6pfad.txt") : std::string("-");
        diag::detail("Knopftest: Stufe 6 - NRC#130 Suchpfad \"" + e.zusatzPfade[0] + "\", gespeichert \"" +
                     (it != g_app->settings.karte.end() ? it->second : std::string("-")) + "\", gefunden in \"" + gefunden + "\"");
        return it != g_app->settings.karte.end() && it->second == e.zusatzPfade[0] && !gefunden.empty();
    }));
    add(tu("NRC#130: Suchpfad zurueck, Grundzustand", [alt] {
        const std::string a = *alt;
        ke::spielNeuRealisieren("Edit Project Settings", [a] { ke::editor().zusatzPfade[0] = a; });
        ke::einstellungenSpeichern();
        grundzustand();
    }));
    add(pause(0.5));
    return s;
}

}  // namespace stufe6

std::vector<Schritt> karteneditorStufe6Schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE6_NUR");
    const auto teil = [nur](const char* name) { return nur == nullptr || std::strstr(nur, name) != nullptr; };
    const auto dazu = [&s](std::vector<Schritt> v) {
        for (Schritt& x : v) { s.push_back(std::move(x)); }
    };
    // die Vorbereitung (Testkarten, Ersatz fuer Dateidialog und Aussenwelt) steckt im Teil "befehle"
    std::vector<Schritt> b = stufe6::befehle();
    s.push_back(std::move(b[0]));
    if (teil("befehle")) {
        for (std::size_t i = 1; i < b.size(); ++i) { s.push_back(std::move(b[i])); }
    }
    if (teil("robust")) { dazu(stufe6::robust()); }
    if (teil("tasten")) { dazu(stufe6::tasten()); }
    if (teil("ereignisse")) { dazu(stufe6::ereignisse()); }
    if (teil("menues")) { dazu(stufe6::menues()); }
    if (teil("knoepfe")) { dazu(stufe6::knoepfe()); }
    if (teil("felder")) { dazu(stufe6::felder()); }
    if (teil("dialoge")) { dazu(stufe6::dialoge()); }
    if (teil("funde")) { dazu(stufe6::funde()); }
    if (teil("pfade")) { dazu(stufe6::pfade()); }
    s.push_back(tu("Stufe 6: Grundzustand, Aussenwelt zurueck", [] {
        stufe6::grundzustand();
        karteneditor::Editor& e = karteneditor::editor();
        e.dateiDialog = nullptr;
        e.urlOeffnen = nullptr;
        e.befehlStarten = nullptr;
        if (stufe6::anfang().gemerkt) {
            e.kameraE = stufe6::anfang().kameraE;
            e.versteckteKnoepfe = stufe6::anfang().versteckteKnoepfe;
        }
    }));
    return s;
}
