// textpuffer.h - Menue-Editor M1 Punkt 3: der Textpuffer hinter dem
// Code-Editor (Mehrfachcursor, Rueckgaengig, Suchen/Ersetzen mit Regex,
// Klammerpaare, Falten, Syntaxfarben, Aenderungsrand).
//
// Eigener Code. Der Text bleibt ein einziger std::string, damit Speichern
// ohne Aenderung byte-gleich ist (CRLF, Tabs, fehlendes Zeilenende am Schluss).
#ifndef BHED_MENUE_TEXTPUFFER_H
#define BHED_MENUE_TEXTPUFFER_H

#include "bhed/menue/menue.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bhed::menue {

struct Pos {
    int zeile = 0;    // 0-basiert
    int spalte = 0;   // Byte in der Zeile
};

// Ein Cursor mit Auswahl zwischen anker und pos (Byte-Offsets).
struct Cursor {
    std::uint32_t pos = 0;
    std::uint32_t anker = 0;
    [[nodiscard]] std::uint32_t von() const { return pos < anker ? pos : anker; }
    [[nodiscard]] std::uint32_t bis() const { return pos < anker ? anker : pos; }
    [[nodiscard]] bool auswahl() const { return pos != anker; }
};

struct Treffer {
    std::uint32_t anfang = 0, ende = 0;
};

struct Falte {
    int von = 0, bis = 0;   // Zeilen der { und der }
    std::string kopf;       // Wort vor der { (menuDef, itemDef, onOpen ...)
    std::string name;       // name des Blocks, wenn er einen hat
};

struct SuchOptionen {
    bool regex = false;
    bool gross = false;     // Gross/Klein beachten
    bool wort = false;      // ganzes Wort
};

class Textpuffer {
public:
    explicit Textpuffer(std::string text = {});

    void setzeText(std::string text);   // ohne Rueckgaengig, Cursor an den Anfang
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] int version() const noexcept { return version_; }

    [[nodiscard]] int zeilen() const noexcept { return static_cast<int>(anfang_.size()); }
    [[nodiscard]] std::uint32_t zeilenAnfang(int z) const;
    [[nodiscard]] std::uint32_t zeilenEnde(int z) const;   // ohne \r\n
    [[nodiscard]] std::string_view zeile(int z) const;
    [[nodiscard]] Pos pos(std::uint32_t off) const;
    [[nodiscard]] std::uint32_t off(Pos p) const;
    [[nodiscard]] std::string zeilenUmbruch() const;   // "\r\n" wenn die Datei CRLF hat

    // --- Cursor -----------------------------------------------------------
    std::vector<Cursor> cursor{Cursor{}};
    void einCursor(std::uint32_t off, bool auswahlBehalten = false);
    void cursorDazu(std::uint32_t off);          // Alt+Klick
    void bewege(int dz, int ds, bool auswahl);   // Pfeiltasten fuer alle Cursor
    void zeilenAnfangEnde(bool ende, bool auswahl);
    void wortSprung(bool vor, bool auswahl);
    void allesWaehlen();
    // Strg+D: naechstes Vorkommen der Auswahl (oder des Worts) als neuer Cursor.
    bool naechstesVorkommenDazu();
    // Alle Vorkommen des Worts unter dem Cursor als Cursor (Strg+Umschalt+L).
    void alleVorkommen();
    [[nodiscard]] std::string auswahlText() const;

    // --- Aendern (mit Rueckgaengig) ----------------------------------------
    void ersetze(std::uint32_t a, std::uint32_t e, const std::string& neu);
    // Mehrere Ersetzungen auf einmal (ein Rueckgaengig-Schritt).
    void ersetzeViele(std::vector<Ersetzung> e);
    void tippe(const std::string& s);   // an allen Cursorn, Auswahl wird ersetzt
    void loescheZurueck();
    void loescheVor();
    void neueZeile();                   // mit der Einrueckung der Zeile
    void einruecken(bool aus);          // Tab / Umschalt+Tab auf den ausgewaehlten Zeilen
    void kommentar();                   // Strg+/: // an/aus
    bool rueckgaengig();
    bool wiederholen();
    [[nodiscard]] bool kannRueckgaengig() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool kannWiederholen() const noexcept { return !redo_.empty(); }

    // --- Suchen -------------------------------------------------------------
    // Treffer im ganzen Text; bei ungueltigem Regex leer und fehler gesetzt.
    [[nodiscard]] std::vector<Treffer> suche(const std::string& muster, const SuchOptionen& o,
                                             std::string* fehler = nullptr) const;
    // Ersetzt alle Treffer (Regex: $1 ... im Ersatz). Rueckgabe: Anzahl.
    int ersetzeAlle(const std::string& muster, const std::string& ersatz, const SuchOptionen& o);
    // Ersetzt den Treffer an der Auswahl und waehlt den naechsten.
    bool ersetzeEinen(const std::string& muster, const std::string& ersatz, const SuchOptionen& o);

    // --- Struktur -----------------------------------------------------------
    [[nodiscard]] std::optional<std::uint32_t> partnerKlammer(std::uint32_t off) const;
    [[nodiscard]] std::vector<Falte> falten() const;
    [[nodiscard]] Treffer wortBei(std::uint32_t off) const;

private:
    struct Schritt {
        std::vector<Ersetzung> hin;     // angewandt auf den Text davor
        std::vector<Ersetzung> zurueck; // angewandt auf den Text danach
        std::vector<Cursor> cursorVor, cursorNach;
        bool tippen = false;
    };
    void zeilenNeu();
    void anwenden(std::vector<Ersetzung> e, bool tippen);
    std::string text_;
    std::vector<std::uint32_t> anfang_;
    std::vector<Schritt> undo_, redo_;
    int version_ = 0;
};

// --- Syntaxfarben -----------------------------------------------------------
enum class Farbklasse {
    Text,
    Kommentar,
    String,
    Zahl,
    MenuWort,      // menuDef-Schluesselwort
    ItemWort,      // itemDef-Schluesselwort
    Block,         // menuDef, itemDef, assetGlobalDef
    Konstante,     // ITEM_TYPE_*, WINDOW_STYLE_* ..., Defines
    Praeprozessor, // #include, #define ...
    Skript,        // Skriptbefehle in { } (open, close, setcvar ...)
    MenuText,      // @MENUS_...
    Klammer,
};
struct Farbstueck {
    std::uint32_t anfang = 0, ende = 0;
    Farbklasse klasse = Farbklasse::Text;
};
// Faerbt den ganzen Text (die Woerter wie COM_ParseExt sie trennt).
[[nodiscard]] std::vector<Farbstueck> faerbe(const std::string& text, Dialekt d, Spiel s,
                                             const std::map<std::string, long>* defines = nullptr);

// Die Skriptbefehle von ui_shared (commandList), fuer Farben und Vervollstaendigung.
[[nodiscard]] const std::vector<std::string>& skriptBefehle();

// --- Aenderungsrand (wie im Skript-Editor: laengste gemeinsame Teilfolge) ----
// Je Zeile von neu: 0 = unveraendert, 1 = geaendert oder neu. geloeschtVor:
// Zeilen von neu, vor denen alte Zeilen weggefallen sind.
struct ZeilenDiff {
    std::vector<std::uint8_t> geaendert;
    std::vector<int> geloeschtVor;
};
[[nodiscard]] ZeilenDiff zeilenDiff(const std::string& alt, const std::string& neu);

// --- Vervollstaendigung ------------------------------------------------------
// Kandidaten, die mit praefix beginnen (Gross/Klein egal), dann solche, die es
// enthalten; ohne Doppelte, hoechstens max.
[[nodiscard]] std::vector<std::string> vorschlaege(const std::string& praefix, const std::vector<std::string>& kandidaten,
                                                   std::size_t max = 50);

}  // namespace bhed::menue

#endif  // BHED_MENUE_TEXTPUFFER_H
