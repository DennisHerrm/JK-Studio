// ui_rollout.h - der Rollout-Baustein: das rechte Feld des Model-Tabs als Befehlsfeld (wie 3ds Max) bzw. Eigenschaften-Editor
// (wie Blender) - Rahmen mit Reitern, aufklappbare Gruppen und ihre Bausteine (AP3 von MBENTWURF Teil B, rc612)
//
// shank 04.11. zum rechten Feld (Bild 1): "kann man das noch schoener machen? einfach benutzerfreundlicher und
// uebersichtlicher" - freigegeben ist der Entwurf plaene/modellblock_stufe0/entwurf_modelltab.html (Bausteine roll, wk,
// raster, wert, regler, tipp): aufklappbare Gruppen mit Ueberschrift und Kurzinfo rechts, nummerierte Arbeitsschritte,
// gleich breite Knoepfe in zwei Spalten mit Symbol und Taste, der Hauptknopf blau, Regler statt Zahlen im Text, Werte als
// Tabelle, Tipps als blauer Kasten - in ALLEN Bereichen einheitlich. shank 10.11.: "es geht ja nicht nur um controls like
// blender oder 3ds max sondern das aussehen": wie 3ds Max flache Ueberschriftsbalken mit +/-, wie Blender abgerundete
// Kaesten mit Pfeil und rechtsbuendiger Beschriftung vor dem Feld.
//
// Der Baustein steht auf dem Formular-Baustein (ui_formular.h): dieselbe Regel fuer Formularzeilen (eine Beschriftungsspalte,
// alle Felder mit gleicher linker Kante und gleicher Breite bis zum Rand, Beschriftung auf der Feldmitte), dieselbe
// Betonungsfarbe des Themas fuer den Hauptknopf, neue Fenster ueber ui::fensterBegin.
//
// SCHNITTSTELLE (stabil seit dem ersten Commit von AP3; danach nur Ergaenzungen):
//
//   Rahmen       feldBeginn / feldEnde            Titelzeile, Reiter (Max: Reihe mit Text; Blender: senkrechte Symbolspalte),
//                                                 darunter der rollende Bereich der Gruppen
//   Gruppe       rolloutBeginn / rolloutEnde      Ueberschrift mit +/- bzw. Pfeil, Schrittnummer, Kurzinfo rechts; Kasten um
//                (oder die Klasse Rollout)        den Inhalt. Der Inhalt ist gewoehnliches ImGui im selben Fenster - jede
//                                                 vorhandene Zeichenfunktion (z. B. drawModellAutoSkinGruppe) passt hinein.
//   Knoepfe      rasterBeginn / rasterEnde        gleich breite Zellen in Spalten (schmal: weniger Spalten)
//                werkzeugKnopf                    Symbol, Text (bricht um), Taste als kleines Tastenfeld; haupt = betont
//                schalter                         Haken mit umbrechender Beschriftung (im Raster: Schalterliste in Spalten)
//                segment                          eine Wahl als zusammenhaengende Knopfreihe ueber die Breite
//   Werte        wertTabelle, grenzBalken, balken Beschriftung links, Wert rechtsbuendig in der Farbe des Zustands
//   Eingaben     zeile / zeileOhne                Formularzeile: Beschriftungsspalte der Gruppe, danach ein beliebiges Feld
//                regler / reglerInt               Beschriftung | Schieber | Wert
//   Text         tipp, absatz, eintrag            blauer Kasten mit Info-Symbol; umbrochener Text; Listenzeile mit "x"
//   Unterteil    mehrBeginn / mehrEnde            eingeklappter Teil "Weitere ..." innerhalb einer Gruppe
//
// Regeln fuer alle Bausteine:
//   - Jedes bedienbare Element meldet sich mit seiner Beschriftung im Element-Register (elemente.h) wie ein Knopf von ImGui:
//     Selbsttests finden es ueber finde(text, fenster); eine Gruppe traegt die Fahnen Openable/Opened wie CollapsingHeader,
//     ihr Zustand steht unter derselben Kennung (GetID(titel)) im StateStorage des Fensters.
//   - Nichts wird abgeschnitten: Text bricht um (CJK ueber textumbruch.h), ein Raster nimmt weniger Spalten, eine zu lange
//     Beschriftung einer Formularzeile rueckt ueber ihr Feld.
//   - Alle Masse in Zeichenhoehen der laufenden Schrift, Farben aus dem Thema (hell und dunkel).
//   - `hinweis` ist der Tooltip (nach der ueblichen Verzoegerung); eine Taste steht dort in einer zweiten Zeile.
//
// BEISPIEL (eine Gruppe mit Kurzinfo, Werten, Raster, Formularzeile, Regler, Balken und Tipp; `t` ist der Zustand des Aufrufers):
//
//   char info[64];
//   std::snprintf(info, sizeof info, trN(Str::MfInfoDreiecke, t.dreiecke), t.dreiecke);
//   if (ui::rolloutBeginn(tr(Str::MbRModell), {.kurzinfo = info, .schritt = 2, .hinweis = tr(Str::MfAnzeigeHint)})) {
//       ui::wertTabelle({{tr(Str::MbDatei), t.datei, ui::Ton::Normal, t.pfad}, {tr(Str::MfDreiecke), t.zahl, ui::Ton::Gut, {}}});
//       if (ui::werkzeugKnopf(tr(Str::MbFoto), {.icon = UiIcon::Kamera, .taste = "F9", .hinweis = tr(Str::MbFotoHint),
//                                               .haupt = true})) { foto(); }             // Hauptknopf: ueber die ganze Breite
//       ui::rasterBeginn(2);                                                             // zwei gleich breite Spalten
//       if (ui::werkzeugKnopf(tr(Str::MbAndereGla), {.icon = UiIcon::Tauschen})) { andereGla(); }
//       if (ui::werkzeugKnopf(tr(Str::MbTexNeu), {.icon = UiIcon::Neuladen, .taste = "Alt+R"})) { neuLaden(); }
//       ui::rasterEnde();
//       ui::zeile(tr(Str::MbZielspiel));                    // danach ein beliebiges Feld von ImGui, die Breite ist gesetzt
//       ImGui::Combo("##zielspiel", &t.zielspiel, t.spiele, t.spielZahl);
//       ui::regler(tr(Str::MbPicmip), &t.picmip, 0.0F, 3.0F, "%.0f");
//       ui::grenzBalken(tr(Str::MfPrKnochen), 29, 32);      // "29 / 32" gelb, Balken zu 91 %
//       ui::tipp(tr(Str::MfAnbauTipp));
//       ui::rolloutEnde();                                  // nur, wenn rolloutBeginn true lieferte
//   }
//
// Im rechten Feld des Model-Tabs gehoert jede Gruppe zu einem oder mehreren Reitern (modelltab.h: modellReiter); ohne Angabe
// steht sie im Reiter, den der Bereich vorgibt.
#ifndef BHED_GUI_UI_ROLLOUT_H
#define BHED_GUI_UI_ROLLOUT_H

#include "imgui.h"
#include "imgui_internal.h"

#include <initializer_list>
#include <string>

namespace bhed::gui {
enum class UiIcon : int;
}

namespace bhed::gui::ui {

// --- Stil und Farben ------------------------------------------------------------------------------------
// Der Stil aller Bausteine: wie 3ds Max (flache Balken mit +/-, eckig, Beschriftung links) oder wie Blender (abgerundete
// Kaesten, Pfeil, Beschriftung rechtsbuendig vor dem Feld). feldBeginn setzt ihn; ausserhalb eines Felds gilt der zuletzt
// gesetzte (Vorgabe Max).
enum class RolloutStil : int { Max = 0, Blender = 1 };
void rolloutStilSetzen(RolloutStil stil);
[[nodiscard]] RolloutStil rolloutStil();

// Der Zustand eines Werts bzw. einer Kurzinfo: Normal (Textfarbe), Leise (gedaempft), Gut (gruen), Warnung (gelb), Fehler (rot).
enum class Ton : int { Normal = 0, Leise, Gut, Warnung, Fehler };
[[nodiscard]] ImVec4 tonFarbe(Ton ton);
// Die Betonungsfarbe des Themas (Hauptknopf, gewaehlter Reiter, gewaehltes Segment), deckend.
[[nodiscard]] ImVec4 betonungsFarbe();

// Kein Symbol (KnopfOpt::icon, FeldReiter::icon).
inline constexpr UiIcon kKeinIcon = static_cast<UiIcon>(-1);

// --- Rahmen des Felds: Titel, Reiter, rollender Bereich ---------------------------------------------------
struct FeldReiter {
    const char* titel = "";            // Beschriftung (Max) bzw. Tooltip-Titel (Blender, dort nur das Symbol)
    UiIcon icon = kKeinIcon;           // Symbol der senkrechten Spalte (Blender)
    const char* hinweis = nullptr;     // Tooltip
};
struct FeldOpt {
    const char* titel = "";            // links in der Titelzeile ("Command panel" / "Properties")
    const char* rechts = nullptr;      // rechts in der Titelzeile (der Bereich)
    const FeldReiter* reiter = nullptr;
    int reiterZahl = 0;
    int* aktiv = nullptr;              // der gewaehlte Reiter (Zustand des Aufrufers)
    int standard = 0;                  // der Reiter, den der Bereich vorgibt: dort stehen die Gruppen ohne eigene Angabe
    RolloutStil stil = RolloutStil::Max;
    int kontext = 0;                   // wechselt der Inhalt des Felds (der Bereich): welche Reiter Gruppen haben, merkt es je Kontext
    const char* leerHinweis = nullptr; // Tooltip-Zusatz eines Reiters, der in diesem Kontext keine Gruppe hat
};
// Zeichnet Titelzeile und Reiter und beginnt den rollenden Bereich (Kindfenster "<id>"). Immer mit feldEnde abschliessen.
// Ein Reiter, zu dem im letzten Bild keine Gruppe gehoerte, ist gedaempft und nicht waehlbar (der vorgegebene bleibt es immer).
void feldBeginn(const char* id, const FeldOpt& opt);
void feldEnde();
// Der gewaehlte Reiter des laufenden Felds (-1: kein Feld) und ob ein Reiter im letzten Bild Gruppen hatte.
[[nodiscard]] int feldReiter();
[[nodiscard]] bool feldReiterBelegt(int reiter);
// Ist der vorgegebene Reiter gewaehlt (oder kein Feld offen)? Fuer Inhalt ausserhalb von Gruppen (Hinweise, alte
// CollapsingHeader): er gehoert nur in den vorgegebenen Reiter.
[[nodiscard]] bool feldReiterStandard();

// --- Gruppe (Rollout / Panel) --------------------------------------------------------------------------
// Die Reiter einer Gruppe als Bits (1U << reiter); kReiterStandard = der Reiter, den der Bereich vorgibt (FeldOpt::standard).
inline constexpr unsigned kReiterStandard = 1U << 31;
[[nodiscard]] constexpr unsigned reiterBit(int reiter) { return 1U << static_cast<unsigned>(reiter); }

struct RolloutOpt {
    const char* kurzinfo = nullptr;    // rechts in der Ueberschrift ("3412 triangles", "LOD 1", "1 error")
    Ton ton = Ton::Leise;              // Farbe der Kurzinfo
    int schritt = 0;                   // 1..9: nummerierter Arbeitsschritt vor dem Titel (UV: 1 Naehte, 2 Abwickeln, 3 Aufraeumen)
    bool zu = false;                   // ab Werk zugeklappt
    unsigned reiter = kReiterStandard; // in welchen Reitern des Felds die Gruppe steht (ausserhalb eines Felds ohne Wirkung)
    const char* hinweis = nullptr;     // Tooltip der Ueberschrift
};
// true: die Gruppe ist offen und gehoert zum gewaehlten Reiter - Inhalt zeichnen, dann rolloutEnde(). Die Kennung ist
// GetID(titel) im laufenden Fenster ("Titel###kennung" haelt sie bei wechselndem Titel fest).
[[nodiscard]] bool rolloutBeginn(const char* titel, const RolloutOpt& opt = {});
void rolloutEnde();
// Dasselbe als Klasse - schliesst die Gruppe auch bei einem fruehen return:  if (ui::Rollout r(titel, {...}); r) { ... }
class Rollout {
public:
    explicit Rollout(const char* titel, const RolloutOpt& opt = {}) : offen_(rolloutBeginn(titel, opt)) {}
    ~Rollout() {
        if (offen_) { rolloutEnde(); }
    }
    Rollout(const Rollout&) = delete;
    Rollout& operator=(const Rollout&) = delete;
    explicit operator bool() const { return offen_; }

private:
    bool offen_;
};
// Eine Gruppe des laufenden Fensters auf- oder zuklappen (Aktionssuche, Selbsttest); titel wie bei rolloutBeginn.
void rolloutSetzen(const char* titel, bool offen);
// Die Gruppe mit dieser Kennung (ImHashStr des Titels bzw. des Teils nach "###") beim naechsten Zeichnen aufklappen und in
// den sichtbaren Bereich rollen - in welchem Fenster sie auch steht (den Reiter waehlt der Aufrufer).
void rolloutZeigen(const char* titel);
// Ein eingeklappter Unterteil in einer Gruppe ("More ...", "Advanced"): Pfeil und gedaempfter Titel, ohne Kasten.
[[nodiscard]] bool mehrBeginn(const char* titel, bool offen = false);
void mehrEnde();

// --- Knoepfe ----------------------------------------------------------------------------------------------
// Ein Raster gleich breiter Zellen. werkzeugKnopf, schalter und rasterZelle belegen der Reihe nach je eine Zelle; die Knoepfe
// einer Reihe sind gleich hoch. Reicht die Breite nicht (Zelle unter mindestZelle Zeichenhoehen), werden es weniger Spalten.
void rasterBeginn(int spalten = 2, float mindestZelle = 6.0F);
void rasterEnde();
// Die naechste Zelle fuer ein eigenes Element von ImGui (Cursor steht in der Zelle, die Breite ist gesetzt).
void rasterZelle();

struct KnopfOpt {
    UiIcon icon = kKeinIcon;           // eigenes Symbol links (uiicons.cpp)
    const char* taste = nullptr;       // die Taste des Bedienmodus als kleines Tastenfeld rechts ("Alt+R")
    const char* hinweis = nullptr;     // Tooltip (die Taste steht dort in der zweiten Zeile)
    bool haupt = false;                // Hauptknopf der Gruppe: Betonungsfarbe des Themas, etwas hoeher (hoechstens einer je Gruppe)
    bool an = false;                   // gedrueckt dargestellt (Umschalter, gewaehltes Werkzeug)
    bool aktiv = true;                 // false: gesperrt (der Tooltip erscheint trotzdem)
    // Ergaenzung AP7: die Taste behalten, solange jedes Wort des Texts in die Zeile neben ihr passt - der Text bricht dann um
    // (Entwurf UV: "Dichte angleichen" zweizeilig mit Strg+A). Ohne: die Taste faellt weg, sobald dem Text weniger als
    // min(Textbreite, 4 Zeichenhoehen) bleibt. CJK bricht zwischen den Zeichen: dort muss das breiteste Zeichenpaar passen.
    bool tasteHalten = false;
};
// Ein Knopf mit Symbol, Text und Taste. Im Raster so breit wie die Zelle, sonst ueber die ganze Breite. true: geklickt.
bool werkzeugKnopf(const char* text, const KnopfOpt& opt = {});
// Ein Haken mit umbrechender Beschriftung; im Raster eine Zelle (Schalterlisten in zwei Spalten). true: geaendert.
bool schalter(const char* text, bool* wert, const char* hinweis = nullptr);
// Eine Wahl als zusammenhaengende Knopfreihe ueber die ganze Breite (LOD 1-10, Picmip 0-3, Verfahren). spalten 0: alle in
// einer Reihe, solange die Texte passen, sonst gleichmaessig umbrochen. gesperrtAb: Eintraege ab diesem Index sind gesperrt
// (-1: keiner). hinweise: je Eintrag ein Tooltip oder nullptr. breite: so breit statt der ganzen Breite (0: ganz).
// true: *wahl geaendert.
bool segment(const char* id, const char* const* texte, int zahl, int* wahl, int spalten = 0, int gesperrtAb = -1,
             const char* const* hinweise = nullptr, float breite = 0.0F);

// --- Werte ------------------------------------------------------------------------------------------------
struct WertZeile {
    std::string name;                  // links, gedaempft
    std::string wert;                  // rechtsbuendig
    Ton ton = Ton::Normal;             // Farbe des Werts
    std::string hinweis;               // Tooltip der Zeile
};
// Zwei Spalten: Beschriftung links, Wert rechtsbuendig (beide brechen um, wenn die Breite nicht reicht).
void wertTabelle(const WertZeile* zeilen, int zahl);
void wertTabelle(std::initializer_list<WertZeile> zeilen);
// Eine Grenze des Spiels als Zeile "name ... ist / grenze" mit Balken: gruen bis zur Grenze, gelb ab 90 % (unter der Grenze),
// rot darueber.
void grenzBalken(const char* name, int ist, int grenze, const char* hinweis = nullptr);
// Die allgemeine Form: eigener Werttext, Anteil 0..1 und Farbe.
void balken(const char* name, const char* wert, float anteil, Ton ton, const char* hinweis = nullptr);

// --- Eingaben ---------------------------------------------------------------------------------------------
// Formularzeile: die Beschriftung in der gemeinsamen Spalte der Gruppe (so breit wie die laengste Beschriftung der Gruppe,
// hoechstens die halbe Breite; Max linksbuendig, Blender rechtsbuendig), danach steht der Cursor an der Feldkante und die
// Breite des naechsten Elements ist gesetzt. Passt die Beschriftung nicht in die Spalte, steht sie ueber dem Feld.
void zeile(const char* beschriftung, const char* hinweis = nullptr);
// Ein Feld ohne Beschriftung an der Feldkante (z. B. ein zweites Feld unter dem ersten).
void zeileOhne();
// Die Feldbreite der letzten zeile()/zeileOhne() (fuer mehrere Elemente nebeneinander im Feld).
[[nodiscard]] float feldBreite();
// Regler: Beschriftung | Schieber | Wert rechtsbuendig. Die Kennung ist die Beschriftung ("Text###kennung"). Strg+Klick auf den
// Schieber tippt den Wert ein. true: geaendert.
bool regler(const char* text, float* wert, float von, float bis, const char* format = "%.2f", const char* hinweis = nullptr,
            ImGuiSliderFlags flags = 0);
bool reglerInt(const char* text, int* wert, int von, int bis, const char* format = "%d", const char* hinweis = nullptr,
               ImGuiSliderFlags flags = 0);

// --- Text -------------------------------------------------------------------------------------------------
// Tipp: Kasten in der Betonungsfarbe mit Info-Symbol und umbrochenem Text (am Ende einer Gruppe).
void tipp(const char* text);
// Ein umbrochener Absatz in der Farbe des Zustands (Vorgabe gedaempft).
void absatz(const char* text, Ton ton = Ton::Leise);
// Eine Listenzeile mit "x" rechts (Anbauten: "saber_1 -> *r_hand"). true: das "x" wurde geklickt. entfernen ist der Name der
// Aktion ("Detach"): Tooltip des "x" und seine Beschriftung im Element-Register. Die Kennung kommt aus dem laufenden
// ID-Stapel (PushID je Zeile).
bool eintrag(const char* text, const char* entfernen = nullptr);

// --- Selbsttest -------------------------------------------------------------------------------------------
// Was der Baustein im laufenden bzw. letzten Bild gezeichnet hat (gui/selbsttest_ap3.inc prueft Ort und Form).
struct RolloutProbe {
    ImGuiID id = 0;
    std::string titel;                 // sichtbarer Titel (ohne "##...")
    std::string kurzinfo;
    ImRect kopf;                       // der Ueberschriftsbalken
    ImRect kasten;                     // Balken und Inhalt (zugeklappt: nur der Balken)
    ImRect zeichen;                    // das +/- bzw. der Pfeil
    ImRect info;                       // die Kurzinfo (leer: keine)
    int schritt = 0;
    bool offen = false;
    RolloutStil stil = RolloutStil::Max;
    float rundung = 0.0F;              // Eckenrundung des Kastens
    int knoepfe = 0;                   // werkzeugKnopf in der Gruppe
    int hauptKnoepfe = 0;
    int symbole = 0;                   // davon mit Symbol
    int tasten = 0;                    // davon mit gezeichnetem Tastenfeld
    int rasterSpalten = 0;             // Spalten des letzten Rasters der Gruppe (0: keins)
    float beschriftungRechts = 0.0F;   // rechte Kante der letzten Beschriftung einer Formularzeile bzw. eines Reglers
    float feldLinks = 0.0F;            // linke Kante des Felds dieser Zeile
};
struct FeldProbe {
    int bild = -1;                     // ImGui-Bild
    RolloutStil stil = RolloutStil::Max;
    ImRect titel;                      // die Titelzeile
    ImRect reiter;                     // der Bereich der Reiter
    ImRect inhalt;                     // der rollende Bereich
    bool reiterSenkrecht = false;
    int reiterZahl = 0;
    int reiterAktiv = -1;
    unsigned reiterBelegt = 0;         // Bits: Reiter mit Gruppen
};
[[nodiscard]] const FeldProbe& feldProbe();
// Die Gruppen des letzten vollstaendigen Bildes (Reihenfolge wie gezeichnet; auch die anderer Reiter fehlen - nur gezeichnete).
[[nodiscard]] int rolloutProbeZahl();
[[nodiscard]] const RolloutProbe& rolloutProbe(int nr);
// Zu Beginn jedes Bildes (app.cpp, neben ui::bildAnfang): die Proben des letzten Bildes festhalten.
void rolloutBildAnfang();

}  // namespace bhed::gui::ui

#endif  // BHED_GUI_UI_ROLLOUT_H
