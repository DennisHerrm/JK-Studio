// karterundlauf.cpp - Karten-Editor Stufe 1a, Punkt 2e: .map lesen und
// schreiben wie NetRadiant-Custom (bhed/karte/kartenformat.h).
//
// 1. Von NRC geschriebene Karten kommen nach lesen -> schreiben Byte fuer Byte
//    gleich heraus (tests/karte/oa_fan.map, GPL v2 aus dem OpenArena-Patch;
//    weitere Karten aus einem Ordner, wenn als zweites Argument angegeben).
// 2. tests/karte/nrc_tiny_structural_brush.map (aelteres NetRadiant, CR LF,
//    noch ohne Ebenen): NRC schreibt sie mit der Zeile
//    //@$& layerdef "0" -1 0 0 0 - sonst unveraendert.
// 3. Selbst gebaute Faelle mit erwarteter Ausgabe: Ebenen, classname zuerst,
//    leerer Wert faellt weg, Schluessel nach dem ersten Pinsel gehen verloren,
//    nicht beitragende Flaechen fallen weg, leere Gruppen werden verworfen,
//    Pinsel in Punktentitaeten verworfen, Valve 220, Lesefehler, Textmodus,
//    Zahlenformat (Decimal).
// 4. Optional (drittes Argument): Ordner mit Karten in fremdem Format (z. B.
//    Ravens JA-Beispielkarten) - lesen ohne Fehler, Schreiben im NRC-Format,
//    erneutes Lesen und Schreiben ist dann stabil.
//
// Aufruf: karterundlauf <tests/karte> [<NRC-Karten-Ordner>] [<fremde Karten>]
#include "bhed/karte/kartenformat.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// erste abweichende Zeile zeigen
void unterschied(const std::string& a, const std::string& b) {
    std::size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i]) { ++i; }
    const std::size_t za = a.rfind('\n', i == 0 ? 0 : i - 1);
    const std::size_t anfang = za == std::string::npos ? 0 : za + 1;
    std::size_t zeile = 1;
    for (std::size_t k = 0; k < anfang; ++k) { zeile += a[k] == '\n'; }
    std::printf("      Zeile %zu, Byte %zu:\n      erwartet: %s\n      ist     : %s\n", zeile, i,
                a.substr(anfang, a.find('\n', anfang) - anfang).c_str(), b.substr(anfang, b.find('\n', anfang) - anfang).c_str());
}

bool rundlauf(const std::string& text, std::string* aus = nullptr, std::vector<std::string>* meldungen = nullptr,
              bool zeigen = true) {
    Karte k;
    const LeseErgebnis e = karteLesen(text, k);
    const std::string geschrieben = karteSchreiben(k);
    if (aus) { *aus = geschrieben; }
    if (meldungen) { *meldungen = e.meldungen; }
    if (!e.vollstaendig) {
        for (const auto& m : e.meldungen) { std::printf("      %s\n", m.c_str()); }
        return false;
    }
    if (geschrieben != text) {
        if (zeigen) { unterschied(text, geschrieben); }
        return false;
    }
    return true;
}

const char* kQuader =
    "( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
    "( 64 32 128 ) ( -64 32 128 ) ( 64 32 0 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
    "( 64 32 128 ) ( 64 32 0 ) ( 64 -32 128 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
    "( -64 -32 0 ) ( 64 -32 0 ) ( -64 32 0 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
    "( -64 -32 0 ) ( -64 -32 128 ) ( 64 -32 0 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
    "( -64 -32 0 ) ( -64 32 0 ) ( -64 -32 128 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n";

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: karterundlauf <tests/karte> [<NRC-Karten>] [<fremde Karten>]\n");
        return 2;
    }
    const std::filesystem::path fixtures = argv[1];

    // --- Zahlenformat (Decimal, textstream.h:254-282) ---------------------
    pruefe(dezimal(1024.1) == "1024.1", "Decimal 1024.1");
    pruefe(dezimal(-0.0) == "-0", "Decimal -0.0 -> \"-0\" (wie NRC)");
    pruefe(dezimal(static_cast<double>(0.1f)) == "0.1000000015", "Decimal float 0.1 -> 0.1000000015");
    pruefe(dezimal(0.00048828125) == "0.0004882812", "Decimal 2^-11 rundet auf gerade (0.0004882812)");
    pruefe(dezimal(64) == "64" && dezimal(-21396.48046875) == "-21396.48046875", "Decimal ganze und lange Zahlen");
    pruefe(dezimal(1e20) == "1", "Decimal 1e20: 21 Zeichen, Nullen weg -> \"1\" (wie NRC)");
    pruefe(dezimal(12345678901.25) == "12345678901.25", "Decimal 12345678901.25 (21 Zeichen)");
    pruefe(dezimal(123456789012.125) == "123456789012.125", "Decimal 123456789012.125: 21 Zeichen, dann Nullen weg");
    pruefe(dezimal(1234567890123.0625) == "1234567890123.0625", "Decimal 1234567890123.0625: 21 Zeichen");
    pruefe(dezimal(12345678901234.03125) == "12345678901234.03125", "Decimal 12345678901234.03125: 21 Zeichen reichen");
    pruefe(dezimal(1e-11) == "0", "Decimal 1e-11 -> 0");

    // --- 1. NRC-geschriebene Karten: bytegleich --------------------------
    {
        const std::string fan = lies(fixtures / "oa_fan.map");
        pruefe(!fan.empty(), "oa_fan.map vorhanden");
        Karte k;
        const LeseErgebnis e = karteLesen(fan, k);
        std::size_t patches = 0;
        for (const auto& en : k.entitaeten()) {
            for (const auto& p : en->kinder()) { patches += p->art() == Primitiv::Art::Patch; }
        }
        pruefe(e.vollstaendig && k.kontext.format == PinselFormat::Quake3 && patches == 44,
               "oa_fan.map: gelesen, Quake3-Format, 44 Patches");
        pruefe(rundlauf(fan), "oa_fan.map: lesen -> schreiben bytegleich");
    }
    std::size_t nrcKarten = 0;
    if (argc >= 3 && std::filesystem::is_directory(argv[2])) {
        for (const auto& d : std::filesystem::directory_iterator(argv[2])) {
            if (d.path().extension() != ".map") { continue; }
            ++nrcKarten;
            const std::string text = textmodusLesen(lies(d.path()));
            pruefe(rundlauf(text), d.path().filename().string() + ": lesen -> schreiben bytegleich");
        }
        std::printf("NRC-Karten im Ordner: %zu\n", nrcKarten);
    }

    // --- 2. aelteres NetRadiant, CR LF, ohne Ebenen ----------------------
    {
        const std::string roh = lies(fixtures / "nrc_tiny_structural_brush.map");
        pruefe(roh.size() == 5106 && roh.substr(0, 2) == "\r\n", "nrc_tiny_structural_brush.map vorhanden (CR LF)");
        Karte k;
        const LeseErgebnis e = karteLesen(textmodusLesen(roh), k);
        pruefe(e.vollstaendig && k.kontext.format == PinselFormat::Quake3BP, "tiny_structural_brush: Brush Primitives erkannt");
        const std::string erwartet = "\r\n//@$& layerdef \"0\" -1 0 0 0" + roh;
        const std::string ist = textmodusSchreiben(karteSchreiben(k));
        if (ist != erwartet) { unterschied(erwartet, ist); }
        pruefe(ist == erwartet, "tiny_structural_brush: wie NRC mit Zeile layerdef \"0\", sonst bytegleich (CR LF)");
    }

    // --- 3a. Ebenen, Patch, Punktentitaet mit Ebene ----------------------
    {
        const std::string text = std::string("\n//@$& layerdef \"0\" -1 0 0 0\n//@$& layerdef \"Raeume\" -1 255 0 0\n"
                                             "//@$& layerdef \"Kinder\" 1 0 128 0\n// entity 0\n{\n\"classname\" \"worldspawn\"\n"
                                             "// brush 0\n{\n") + kQuader +
                                 "}\n// brush 1\n//@$& layer 2\n{\npatchDef2\n{\nbase/wall\n( 3 3 0 0 0 )\n(\n"
                                 "( ( 0 0 0 0 0 ) ( 0 64 0 0 0.5 ) ( 0 128 0 0 1 ) )\n"
                                 "( ( 64 0 0 0.5 0 ) ( 64 64 0 0.5 0.5 ) ( 64 128 0 0.5 1 ) )\n"
                                 "( ( 128 0 0 1 0 ) ( 128 64 0 1 0.5 ) ( 128 128 0 1 1 ) )\n)\n}\n}\n}\n"
                                 "// entity 1\n//@$& layer 1\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 0 24\"\n}\n";
        Karte k;
        (void)karteLesen(text, k);
        pruefe(k.schichten.size() == 3 && k.schichten[2].name == "Kinder" && k.schichten[2].eltern == 1
                   && k.schichten[1].farbe[0] == 255,
               "Ebenen: Baum aus layerdef (Kinder unter Raeume)");
        pruefe(k.entitaeten().size() == 2 && k.entitaeten()[0]->kinder()[1]->schicht == 2 && k.entitaeten()[1]->schicht == 1,
               "Ebenen: Patch in Ebene 2, Punktentitaet in Ebene 1");
        pruefe(rundlauf(text), "Ebenen und Patch: bytegleich");
    }
    // Ebenen in anderer Reihenfolge: NRC haengt sie um und schreibt in Tiefensuche
    {
        const std::string text = "//@$& layerdef \"A\" -1 0 0 0\n//@$& layerdef \"B\" -1 0 0 0\n//@$& layerdef \"C\" 0 0 0 0\n"
                                 "{\n\"classname\" \"info_null\"\n}\n//@$& layer 1\n{\n\"classname\" \"info_null\"\n}\n";
        std::string aus;
        (void)rundlauf(text, &aus, nullptr, false);
        const std::string erwartet = "\n//@$& layerdef \"A\" -1 0 0 0\n//@$& layerdef \"C\" 0 0 0 0\n//@$& layerdef \"B\" -1 0 0 0\n"
                                     "// entity 0\n{\n\"classname\" \"info_null\"\n}\n// entity 1\n//@$& layer 2\n{\n"
                                     "\"classname\" \"info_null\"\n}\n";
        if (aus != erwartet) { unterschied(erwartet, aus); }
        pruefe(aus == erwartet, "Ebenen: Kind C hinter A geschrieben, Ebene B wird Index 2");
    }

    // --- 3b. Reihenfolge, leere Werte, spaete Schluessel, Flaechen -------
    {
        const std::string text = std::string("{\n\"message\" \"test\"\n\"classname\" \"worldspawn\"\n\"leer\" \"\"\n{\n") + kQuader +
                                 "( 0 32 200 ) ( 0 -32 200 ) ( -64 32 200 ) system/caulk 0 0 0 0.25 0.25 0 0 0\n"
                                 "}\n\"spaet\" \"verloren\"\n}\n"
                                 "{\n\"classname\" \"func_group\"\n}\n";
        std::string aus;
        (void)rundlauf(text, &aus, nullptr, false);
        const std::string erwartet = std::string("\n//@$& layerdef \"0\" -1 0 0 0\n// entity 0\n{\n\"classname\" \"worldspawn\"\n"
                                                 "\"message\" \"test\"\n// brush 0\n{\n") + kQuader +
                                     "}\n}\n// entity 1\n{\n\"classname\" \"func_group\"\n}\n";
        if (aus != erwartet) { unterschied(erwartet, aus); }
        pruefe(aus == erwartet,
               "classname zuerst, leerer Wert weg, Schluessel nach dem Pinsel verloren, nicht beitragende Flaeche weg, "
               "func_group ohne Definition = Punktentitaet");
    }
    // mit Klassen: func_group ist Gruppe -> leer verworfen; info_null fest -> Pinsel verworfen
    {
        EntitaetsKlassen kl;
        auto g = std::make_unique<EntitaetsKlasse>();
        g->setzeName("func_group");
        kl.einfuegen(std::move(g));
        auto n = std::make_unique<EntitaetsKlasse>();
        n->setzeName("info_null");
        n->fixedsize = true;
        kl.einfuegen(std::move(n));
        const std::string text = std::string("{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"func_group\"\n}\n"
                                             "{\n\"classname\" \"info_null\"\n{\n") + kQuader + "}\n}\n";
        Karte k;
        k.klassen = &kl;
        const LeseErgebnis e = karteLesen(text, k);
        std::vector<std::string> m;
        const std::string aus = karteSchreiben(k, &m);
        const std::string erwartet = "\n//@$& layerdef \"0\" -1 0 0 0\n// entity 0\n{\n\"classname\" \"worldspawn\"\n}\n"
                                     "// entity 1\n{\n\"classname\" \"info_null\"\n}\n";
        bool verworfen = false;
        for (const auto& s : e.meldungen) { verworfen = verworfen || s == "entity 2: type info_null: discarding brush 0"; }
        if (aus != erwartet) { unterschied(erwartet, aus); }
        pruefe(aus == erwartet && verworfen && m.size() == 1 && m[0] == "discarding empty group entity: # = 1; classname = func_group",
               "Klassen: leere Gruppe verworfen, Pinsel in Punktentitaet verworfen (Meldungen wie NRC)");
    }

    // --- 3c. Brush Primitives mit fehlenden Flags, Valve 220 -------------
    {
        const std::string text = "{\n\"classname\" \"worldspawn\"\n{\nbrushDef\n{\n"
                                 "( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) ( ( 0.015625 0 -0 ) ( 0 0.015625 0 ) ) system/caulk\n"
                                 "( 64 32 128 ) ( -64 32 128 ) ( 64 32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk 134217728 0 0\n"
                                 "( 64 32 128 ) ( 64 32 0 ) ( 64 -32 128 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) NULL\n"
                                 "( -64 -32 0 ) ( 64 -32 0 ) ( -64 32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk\n"
                                 "( -64 -32 0 ) ( -64 -32 128 ) ( 64 -32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk\n"
                                 "( -64 -32 0 ) ( -64 32 0 ) ( -64 -32 128 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk\n"
                                 "}\n}\n}\n";
        std::string aus;
        (void)rundlauf(text, &aus, nullptr, false);
        const std::string erwartet = "\n//@$& layerdef \"0\" -1 0 0 0\n// entity 0\n{\n\"classname\" \"worldspawn\"\n// brush 0\n{\nbrushDef\n{\n"
                                     "( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) ( ( 0.015625 0 -0 ) ( 0 0.015625 0 ) ) system/caulk 0 0 0\n"
                                     "( 64 32 128 ) ( -64 32 128 ) ( 64 32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk 134217728 0 0\n"
                                     "( 64 32 128 ) ( 64 32 0 ) ( 64 -32 128 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) NULL 0 0 0\n"
                                     "( -64 -32 0 ) ( 64 -32 0 ) ( -64 32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk 0 0 0\n"
                                     "( -64 -32 0 ) ( -64 -32 128 ) ( 64 -32 0 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk 0 0 0\n"
                                     "( -64 -32 0 ) ( -64 32 0 ) ( -64 -32 128 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) system/caulk 0 0 0\n"
                                     "}\n}\n}\n";
        if (aus != erwartet) { unterschied(erwartet, aus); }
        pruefe(aus == erwartet, "BP: fehlende Flags -> 0 0 0, Detail-Flag bleibt, NULL bleibt NULL, -0 bleibt -0");
    }
    {
        const std::string text = "{\n\"classname\" \"worldspawn\"\n{\n"
                                 "( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) base/a [ 1 0 0 8 ] [ 0 -1 0 -4 ] 15 0.5 0.5 0 0 0\n"
                                 "( 64 32 128 ) ( -64 32 128 ) ( 64 32 0 ) base/a [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 0 0\n"
                                 "( 64 32 128 ) ( 64 32 0 ) ( 64 -32 128 ) base/a [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 0 0\n"
                                 "( -64 -32 0 ) ( 64 -32 0 ) ( -64 32 0 ) base/a [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 0 0\n"
                                 "( -64 -32 0 ) ( -64 -32 128 ) ( 64 -32 0 ) base/a [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 0 0\n"
                                 "( -64 -32 0 ) ( -64 32 0 ) ( -64 -32 128 ) base/a [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 0 0\n"
                                 "}\n}\n";
        Karte k;
        (void)karteLesen(text, k);
        const auto& f = static_cast<const Pinsel&>(*k.entitaeten()[0]->kinder()[0])[0];
        pruefe(k.kontext.format == PinselFormat::Quake3Valve220 && f.projektion().texdef.rotate == -15
                   && f.projektion().texdef.shift[0] == 8 && f.projektion().basisT == Vektor3f(0, -1, 0),
               "Valve 220 erkannt (\" [ \" und \" ] \" im Puffer), Drehung negiert gespeichert");
        std::string aus = karteSchreiben(k);
        const std::string erwartet = "\n//@$& layerdef \"0\" -1 0 0 0\n// entity 0" + text.substr(0, 0);
        pruefe(aus.find("( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) base/a [ 1 0 0 8 ] [ 0 -1 0 -4 ] 15 0.5 0.5 0 0 0\n") != std::string::npos,
               "Valve 220 geschrieben wie gelesen");
        (void)erwartet;
    }

    // --- 3d. Lesefehler: was davor kam, bleibt ---------------------------
    {
        const std::string text = "{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"info_null\"\n{\n( 1 2 3 ) ( 4 5\n}\n}\n";
        Karte k;
        const LeseErgebnis e = karteLesen(text, k);
        bool brush = false;
        bool entity = false;
        for (const auto& m : e.meldungen) {
            brush = brush || m == "brush 0: parse error";
            entity = entity || m == "entity 1: parse error";
        }
        pruefe(!e.vollstaendig && k.entitaeten().size() == 1 && brush && entity,
               "Lesefehler: erste Entitaet bleibt, Meldungen 'brush 0: parse error' / 'entity 1: parse error'");
        bool genau = false;
        for (const auto& m : e.meldungen) { genau = genau || m == "8:2: parse error at '}': expected '#number'"; }
        pruefe(genau, "Lesefehler: Zeile:Spalte und Wortlaut wie Tokeniser_unexpectedError");
    }
    // negative Flags kann NRC nicht lesen (Tokeniser_nextTokenIsDigit)
    {
        const std::string text = "{\n\"classname\" \"worldspawn\"\n{\n( 64 32 128 ) ( 64 -32 128 ) ( -64 32 128 ) a 0 0 0 1 1 -1 0 0\n}\n}\n";
        Karte k;
        const LeseErgebnis e = karteLesen(text, k);
        pruefe(!e.vollstaendig, "negative Flags: Lesefehler wie NRC");
    }

    // --- 3e. Textmodus: CR LF und Strg+Z ---------------------------------
    {
        const std::string roh = "{\r\n\"classname\" \"worldspawn\"\r\n}\r\n\x1a{\r\n\"classname\" \"x\"\r\n}\r\n";
        Karte k;
        (void)karteLesen(textmodusLesen(roh), k);
        pruefe(k.entitaeten().size() == 1, "Textmodus: Strg+Z beendet die Datei");
        pruefe(textmodusSchreiben("a\nb\n") == "a\r\nb\r\n", "Textmodus Schreiben: LF -> CR LF");
    }
    // Sonderkommentare: nur "//@$&" wird gelesen, "//@$" ist ein Kommentar
    {
        const std::string text = "//@$ layerdef \"x\" -1 0 0 0\n{\n\"classname\" \"worldspawn\" /* Block */\n}\n";
        Karte k;
        const LeseErgebnis e = karteLesen(text, k);
        pruefe(e.vollstaendig && k.schichten.size() == 1 && k.schichten[0].name == "0" && k.entitaeten().size() == 1,
               "Tokenizer: //@$ ist Kommentar, /* */ Blockkommentar");
    }

    // --- 4. fremde Karten: lesen, im NRC-Format schreiben, stabil ---------
    if (argc >= 4 && std::filesystem::is_directory(argv[3])) {
        std::size_t anzahl = 0;
        for (const auto& d : std::filesystem::recursive_directory_iterator(argv[3])) {
            if (d.path().extension() != ".map") { continue; }
            ++anzahl;
            Karte k;
            const LeseErgebnis e = karteLesen(textmodusLesen(lies(d.path())), k);
            const std::string einmal = karteSchreiben(k);
            const bool stabil = rundlauf(einmal);
            pruefe(e.vollstaendig && stabil, d.path().filename().string() + ": gelesen, NRC-Format stabil");
        }
        std::printf("fremde Karten: %zu\n", anzahl);
    }

    std::printf("karterundlauf: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
