// Modell-Block 1c: Texture Painting - Malen auf die Bilder hinter den JKA-Shadern (neu geschrieben, nicht aus
// WhiteoutFlakes: dort gibt es kein Texture Painting; die Begriffe Abstand/Druck/Abfall entsprechen jedem Malprogramm).
//
// Ziel ist das Bild, das die Engine fuer die Flaeche laedt: die Haut (.skin) bzw. der Shader der Flaeche, sonst die erste
// "map"-Stufe des gleichnamigen Shaderskripts (R_FindShader -> ParseStage, tr_shader.cpp), in GROESSE UND FORMAT DES
// ORIGINALS (keine Verkleinerung, Regel "Tempo ohne Qualitaetsverlust"). Eine Leinwand je Bild:
//   - Ebenen (die unterste ist das Original), je Ebene Deckkraft, Mischart, sichtbar; das Ergebnis ist ihr Stapel.
//   - Maske je Leinwand (255 = geschuetzt; jeder Pinsel ausser "Maske" malt nur, wo sie frei ist).
//   - Rueckgaengig in Kacheln (64 x 64): ein Strich sichert vor der ersten Aenderung jede beruehrte Kachel einmal.
//   - Ausbluten ueber die UV-Grenzen: jedes Texel ausserhalb aller UV-Dreiecke, das hoechstens `ausbluten` Schritte von
//     einem bedeckten liegt, bekommt beim Malen die Farbe des naechsten bedeckten (die Mipmaps ziehen sonst den Grund in
//     die Naehte, R_MipMap in tr_image.cpp mittelt je vier Nachbarn).
// Malen in 3D: der Tupfer ist eine Kugel um den Treffpunkt in der Ruhelage des Modells (Ecken der Datei), jedes Texel
// eines Dreiecks in seiner Reichweite bekommt den Abfall nach seinem Abstand im Raum - so gehen Striche ueber Naehte und
// Inselgrenzen hinweg ohne Bruch, und eine laufende Animation aendert die Form des Tupfers nicht.
// Malen in 2D: der Tupfer ist ein Kreis in Texeln.
#ifndef BHED_MODELL_MALEN_H
#define BHED_MODELL_MALEN_H

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::modell::texmalen {

enum class Mischart : int { Normal = 0, Multiplizieren, Addieren, Aufhellen /* screen */, Ineinander /* overlay */ };
inline constexpr int kMischarten = 5;

enum class Werkzeug : int { Pinsel = 0, Klonen, Verwischen, Weichzeichnen, Fuellen, Maske, Radierer };
inline constexpr int kWerkzeuge = 7;

// Abfall des Tupfers: 1 in der Mitte, 0 am Rand. haerte 1 = harte Kante, 0 = glatt bis in die Mitte (smoothstep).
[[nodiscard]] float abfall(float abstand, float radius, float haerte) noexcept;

struct Ebene {
    std::string name;
    std::vector<std::uint8_t> rgba;   // b*h*4, nicht vormultipliziert
    float deckkraft = 1.0F;
    Mischart misch = Mischart::Normal;
    bool sichtbar = true;
};

// Ein UV-Dreieck mit seinen Ecken in der Ruhelage (fuer 3D) - in UV der Datei (0..1 = ein Bild, oben links 0,0).
struct Dreieck {
    float uv[3][2]{};
    float ort[3][3]{};
};

struct Rechteck {
    int x0 = 0, y0 = 0, x1 = -1, y1 = -1;   // eingeschlossen; leer, wenn x1 < x0
    [[nodiscard]] bool leer() const noexcept { return x1 < x0 || y1 < y0; }
    void dazu(int x, int y) noexcept;
    void dazu(const Rechteck& r) noexcept;
};

struct Leinwand {
    std::string datei;                       // VFS-Pfad des Originals (models/players/.../x.jpg)
    std::string original;                    // Bytes des Originals (bytegleich zurueck, solange nichts bemalt ist)
    int format = 0;                          // image::Format
    int b = 0;
    int h = 0;
    bool alpha = false;                      // Original hatte einen Alphakanal
    int tgaBits = 0;                         // 24/32, 0 kein TGA
    bool tgaRle = false;
    std::vector<Ebene> ebenen;               // [0] = das Original (Name leer: die GUI zeigt ihn in ihrer Sprache)
    int aktiv = 0;
    std::vector<std::uint8_t> grund;         // die Bildpunkte des Originals (Radierer auf der untersten Ebene)
    std::vector<std::uint8_t> maske;         // b*h
    std::vector<std::uint8_t> bild;          // Ergebnis b*h*4
    std::vector<std::int32_t> blutQuelle;    // je Texel: -1 bedeckt oder zu weit, sonst das naechste bedeckte
    std::vector<std::uint8_t> bedeckt;       // je Texel 1, wenn ein UV-Dreieck es bedeckt
    std::vector<Dreieck> dreiecke;           // die Dreiecke, die dieses Bild zeichnen
    std::vector<Dreieck> dreieckeBeimLaden;  // fuer "Auf neue UV umbacken"
    std::vector<std::array<float, 4>> kugeln; // P15: je Dreieck Mitte und Umkreis (dreieckeSetzen), fuer das schnelle Verwerfen
    // MBMAL: Suchgitter ueber die Kugeln der Dreiecke (dreieckeSetzen) - ein Tupfer in 3D prueft nur die Dreiecke der Zellen
    // in seiner Reichweite statt alle (in der Luft: keines). Passt es nicht zu den Dreiecken, prueft der Tupfer alle.
    struct Gitter {
        float ursprung[3]{};
        float zelle = 0.0F;
        int n[3]{};
        std::size_t dreieckZahl = 0;             // fuer so viele Dreiecke gebaut
        std::vector<std::uint32_t> anfang;       // je Zelle der erste Eintrag (n[0] * n[1] * n[2] + 1 Werte)
        std::vector<std::uint32_t> eintraege;    // die Dreiecke der Zellen hintereinander
        std::vector<std::uint32_t> gross;        // Dreiecke ueber sehr viele Zellen: bei jedem Tupfer geprueft
    } gitter;
    Rechteck schmutzig;                      // Ergebnis neu hochladen: die Huelle aller Aenderungen seit dem letzten Hochladen
    // MBMAL: dasselbe genau - je Kachel (kKachel) das geaenderte Rechteck und die Kacheln mit einem. Ein Tupfer ueber eine
    // UV-Naht aendert zwei kleine Flaechen; ihre Huelle reichte bis zum ganzen Bild. Nur ueber schmutzigDazu/schmutzigLeeren.
    std::vector<Rechteck> schmutzKachel;
    std::vector<std::int32_t> schmutzListe;
    int stand = 0;                           // zaehlt jede Aenderung
    int maskeStand = 0;                      // MBMAL: zaehlt jede Aenderung der Maske (die 2D-Ansicht liest sie nur dann neu)
    int gespeichert = 0;                     // stand beim Laden/letzten Bauen
    int ausbluten = 4;
    // Zwischenspeicher eines Tupfers (je Texel der staerkste Abfall, einmal je Tupfer)
    std::vector<float> tupfWert;
    std::vector<std::int32_t> tupfMarke;
    std::int32_t tupfNr = 0;
    // MBMAL: Zwischenspeicher eines Tupfers - Dreiecke aus dem Gitter (einmal je Tupfer), die Kacheln mit Treffern und die
    // Kacheln, in die er schreibt (Marke = tupfNr, Platz in der Liste des Tupfers)
    std::vector<std::int32_t> gitterMarke;
    std::vector<std::uint32_t> gitterFund;
    std::vector<std::int32_t> kachelMarke[2];
    std::vector<std::int32_t> kachelPlatz[2];

    [[nodiscard]] bool bemalt() const noexcept { return stand != 0; }
    [[nodiscard]] std::size_t texel(int x, int y) const noexcept {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x);
    }
};

// Das Original lesen (Bytes der Datei, wie die Engine sie findet). false mit dem Grund des Bildlesers (technisch, kann leer
// sein; den Satz in der Sprache der Oberflaeche setzt die GUI, P13).
bool leinwandAusDatei(Leinwand& l, const std::string& vfsPfad, const std::string& bytes, std::string* fehler);
// Leinwand ohne Datei (Tests): einfarbig
void leinwandLeer(Leinwand& l, int b, int h, std::array<std::uint8_t, 4> farbe);
// Dreiecke setzen: Bedeckung und Ausblutkarte neu
void dreieckeSetzen(Leinwand& l, const std::vector<Dreieck>& d, bool alsLadestand);
// Ebenen-Stapel -> Ergebnis im Rechteck (leer: alles)
void zusammensetzen(Leinwand& l, Rechteck r = {});
// MBMAL: eine Aenderung des Ergebnisses im Rechteck melden (Huelle und Kacheln; auf das Bild begrenzt)
void schmutzigDazu(Leinwand& l, const Rechteck& r);
// die gemeldeten Flaechen als Rechtecke, die sich nicht ueberdecken - benachbarte Kacheln mit gleichen Kanten sind
// zusammengefasst (ein Tupfer in einer Insel: ein Rechteck). Leer, wenn nichts gemeldet ist.
[[nodiscard]] std::vector<Rechteck> schmutzRechtecke(const Leinwand& l);
void schmutzigLeeren(Leinwand& l);

// --- Rueckgaengig in Kacheln -------------------------------------------------------------------------------------
inline constexpr int kKachel = 64;
struct Kachel {
    int ebene = 0;    // -1 die Maske
    int kx = 0;
    int ky = 0;
    std::vector<std::uint8_t> vorher;
    std::vector<std::uint8_t> nachher;
};
struct UndoSchritt {
    std::string name;
    std::vector<Kachel> kacheln;
    // ganze Ebenen (neu, geloescht, Eigenschaften, Umbacken): der Stapel vorher/nachher mit Bildpunkten, wenn struktur
    bool struktur = false;
    // Pruefung seit rc610 P2: nur Eigenschaften (Sichtbar, Mischart, Deckkraft, Name, Reihenfolge gleich): die Stapel ohne
    // Bildpunkte, Anwenden behaelt die Bildpunkte der Ebenen
    bool nurEigenschaften = false;
    // P4: fortlaufende Nummer ueber alle Leinwaende (das Rueckgaengig des Bereichs nimmt den juengsten Schritt aller Bilder)
    long long nummer = 0;
    // TEST1411 O: die Kennung des Strichs, zu dem der Schritt gehoert (0 keiner) - ein Strich ueber mehrere Bilder legt auf jedem
    // Bild einen Schritt mit derselben Kennung ab, Rueckgaengig und Wiederholen nehmen sie zusammen
    long long strich = 0;
    std::vector<Ebene> stapelVorher;
    std::vector<Ebene> stapelNachher;
    int aktivVorher = 0;
    int aktivNachher = 0;
    std::vector<std::uint8_t> maskeVorher;
    std::vector<std::uint8_t> maskeNachher;
};
struct Verlauf {
    std::vector<UndoSchritt> zurueck;
    std::vector<UndoSchritt> vor;
    UndoSchritt offen;
    bool laeuft = false;
    std::vector<std::uint8_t> gesichert;   // je Kachel und Ebene im offenen Schritt
    int gesichertEbenen = 0;
    std::size_t grenzeBytes = 512U * 1024U * 1024U;   // aelteste Schritte fallen danach weg
};
// Bytes der Bildpunkte eines Schritts (Kacheln, Masken, Stapel)
[[nodiscard]] std::size_t schrittBytes(const UndoSchritt& s);
void schrittBeginnen(Verlauf& v, const Leinwand& l, const std::string& name);
// vor einer Aenderung im Rechteck der Ebene (-1 Maske) aufrufen
void sichern(Verlauf& v, const Leinwand& l, int ebene, const Rechteck& r);
void schrittEnde(Verlauf& v, Leinwand& l);
// ganze Ebenen aendern (neu, loeschen, Reihenfolge, Deckkraft ...): ein Schritt mit dem Stapel vorher und nachher
void stapelAendern(Verlauf& v, Leinwand& l, const std::string& name, const std::vector<Ebene>& neu, int aktivNeu);
bool rueckgaengig(Verlauf& v, Leinwand& l);
bool wiederholen(Verlauf& v, Leinwand& l);

// --- Pinsel --------------------------------------------------------------------------------------------------------
struct Pinsel {
    Werkzeug werkzeug = Werkzeug::Pinsel;
    float radius = 4.0F;        // 3D: Einheiten der Welt; 2D: Texel
    float staerke = 1.0F;       // 0..1
    float haerte = 0.5F;        // 0..1
    float abstand = 0.1F;       // Anteil des Radius zwischen zwei Tupfern (Blender spacing 10 %)
    bool druckGroesse = false;  // Druck aendert die Groesse
    bool druckStaerke = true;   // Druck aendert die Staerke
    std::array<float, 4> farbe{1.0F, 1.0F, 1.0F, 1.0F};
    float fuellToleranz = 0.1F;  // 0..1 (Abstand im RGBA-Wuerfel)
    bool maskeLoeschen = false;  // Maske: freigeben statt schuetzen
    bool rueckseiten = false;    // 3D: auch Dreiecke, die von der Kamera wegzeigen
    float klonVersatz[2]{};      // Klonen: Quelle - Ziel in UV
};

// Ein Tupfer in 3D um `mitte` (Ruhelage). zugewandt: je Dreieck 1, wenn es in der Pose zur Kamera zeigt (nullptr: alle;
// Pinsel::rueckseiten malt auch die anderen), richtung: Weg seit dem letzten Tupfer in UV (Verwischen). Gibt die Zahl der geaenderten Texel zurueck.
int tupfer3d(Leinwand& l, Verlauf& v, const Pinsel& p, const float mitte[3], float druck, const std::vector<std::uint8_t>* zugewandt,
             const float richtung[2]);
// Ein Tupfer in 2D um (u, v) in UV
int tupfer2d(Leinwand& l, Verlauf& v, const Pinsel& p, float u, float vv, float druck, const float richtung[2]);
// Fuellen ab Texel (x, y): zusammenhaengend, Farbe des Ergebnisses innerhalb der Toleranz - im ganzen Bild, auch ueber die
// UV-Inseln hinaus (Pruefung seit rc610 P12: der Kopf sagte vorher "nur bedeckte Texel"); die Maske schuetzt wie bei jedem Werkzeug
int fuellen(Leinwand& l, Verlauf& v, const Pinsel& p, int x, int y);

// Abstand im Strich: aus der Folge der Mauspositionen (Welt oder UV) die Tupferstellen mit festem Abstand
struct Strich {
    bool laeuft = false;
    float letzter[3]{};
    float rest = 0.0F;
    // neue Stelle; `schritt` = Abstand zwischen Tupfern; gibt die Tupferstellen bis hier (der erste Aufruf: der Punkt)
    std::vector<std::array<float, 3>> weiter(const float p[3], float schritt);
};

// Spiegeln: der gespiegelte Punkt (Achse 0 x, 1 y, 2 z) in der Ruhelage
void spiegelPunkt(const float p[3], int achse, float out[3]) noexcept;

// Treffer im Dreieck -> UV und Ort in der Ruhelage (Schwerpunktkoordinaten l0, l1, l2)
void punktIm(const Dreieck& d, float l0, float l1, float l2, float uv[2], float ort[3]) noexcept;
// Die Texel eines UV-Dreiecks mit ihren Schwerpunktkoordinaten - dieselbe Regel wie beim Malen (Mitte im Dreieck; x, y im
// Bild gewickelt wie GL_REPEAT)
void texelJeDreieck(const Dreieck& d, int b, int h, const std::function<void(int x, int y, float l0, float l1, float l2)>& f);

// --- Umbacken alt -> neu (Modell-Block 1b, bisher nur im Test) -----------------------------------------------------
// Je Texel jedes neuen Dreiecks die Stelle des alten (bilinear, GL_REPEAT wie die Engine), danach `ausbluten` Schritte
// in die unbedeckten Texel. alt/neu: gleich viele Dreiecke in gleicher Reihenfolge.
std::vector<std::uint8_t> umbacken(const std::vector<std::uint8_t>& rgba, int b, int h, const std::vector<Dreieck>& alt,
                                   const std::vector<Dreieck>& neu, int ausbluten);
// Jede Ebene und die Maske einer Leinwand von dreieckeBeimLaden auf dreiecke umbacken (ein Rueckgaengig-Schritt)
bool leinwandUmbacken(Leinwand& l, Verlauf& v, const std::string& name);

// --- Schreiben ------------------------------------------------------------------------------------------------------
// Das Ergebnis im Format des Originals (TGA mit gleicher Tiefe und Kompression, JPEG mit den Quantisierungstabellen des
// Originals, PNG als TGA nicht - siehe malen.cpp). Unbemalt: die Bytes des Originals. false (kein Bild): fehler leer.
bool leinwandBytes(const Leinwand& l, std::string& out, std::string* fehler);
std::string tgaSchreiben(const std::uint8_t* rgba, int b, int h, int bits, bool rle);
// Baseline-JPEG 4:4:4 (8 Bit, Huffman-Tabellen aus ITU T.81 Anhang K); quant: 2 x 64 Werte in Zickzack-Folge wie im
// DQT-Abschnitt (leer: Anhang K mit Qualitaet 90)
std::string jpegSchreiben(const std::uint8_t* rgba, int b, int h, const std::vector<std::uint16_t>& quant);
// PNG mit ungepackten Deflate-Bloecken (das Projekt hat keinen Packer; jeder PNG-Leser liest sie), RGBA oder RGB
std::string pngSchreiben(const std::uint8_t* rgba, int b, int h, bool alpha);
// Die DQT-Tabellen einer JPEG-Datei (Luma, Chroma; je 64 in Zickzack-Folge). Leer, wenn keine.
std::vector<std::uint16_t> jpegQuant(const std::string& bytes);

}  // namespace bhed::modell::texmalen
#endif
