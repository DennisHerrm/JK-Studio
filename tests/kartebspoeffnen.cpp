// kartebspoeffnen.cpp - Karten-Editor Stufe 4 Punkte 1 und 2: fertige Karten
// oeffnen (q3map2 -convert wie Map_ImportFile tryDecompile) und das
// Originallicht behalten (q3map2 -onlyents).
//
// Geprueft wird:
//   - der Befehl wie NRC (map.cpp:1583-1594), "<stamm>_converted.map",
//     -bsp -onlyents; misc_model/_decal/_skybox als eingebackene Entitaeten;
//   - der Fingerabdruck der Geometrie: gleich bei Schluesselaenderungen,
//     verschobenen und neuen Punkt-Entities, anders bei verschobenen Pinseln,
//     geaenderten Texturen, neuen Pinseln und neuen misc_model;
//   - mit bhed_q3map2.exe an tests/karte/q3map2/raum.nrc.bsp (RBSP von NRCs
//     q3map2): -convert, laden, eine Entity aendern, speichern, -onlyents auf
//     eine Kopie - jeder Lump ausser den Entities Byte-gleich, der
//     Entity-Lump traegt die Aenderung;
//   - mit den Original-BSPs (<dekomp>/<spiel>/bsp, nur wenn angegeben): je
//     Spiel zwei Karten, Entitaeten/Pinsel/Patches wie die Massenlast
//     (<dekomp>/<spiel>/bp/<name>.map, von NRCs q3map2 umgewandelt), und
//     -onlyents ohne Aenderung laesst jeden Lump ausser den Entities gleich.
//
// Aufruf: kartebspoeffnen <bhed_q3map2.exe|-> <tests/karte/q3map2> [<dekomp> <JA GameData> <JK2 GameData> <MD GameData>]
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/pinsel.h"

#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;
namespace fs = std::filesystem;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

struct Zahlen {
    std::size_t entitaeten = 0, pinsel = 0, patches = 0;
};
Zahlen zaehlen(const Karte& k) {
    Zahlen z;
    z.entitaeten = k.entitaeten().size();
    for (const auto& en : k.entitaeten()) {
        for (const auto& pr : en->kinder()) {
            if (pr->art() == Primitiv::Art::Patch) {
                ++z.patches;
            } else if (pr->art() == Primitiv::Art::Pinsel) {
                ++z.pinsel;
            }
        }
    }
    return z;
}

bool laden(const fs::path& p, Karte& k) {
    const LeseErgebnis e = karteLesen(textmodusLesen(lies(p)), k);
    return e.vollstaendig;
}

int ausfuehren(const std::string& befehl, const fs::path& log) {
    // cmd /c: ein Befehl, der mit " beginnt, braucht ein aeusseres Paar
    const std::string zeile = "\"" + befehl + " > \"" + log.string() + "\" 2>&1\"";
    return std::system(zeile.c_str());
}

std::string mitSchraeg(std::string p) {
    for (char& c : p) {
        if (c == '\\') { c = '/'; }
    }
    return p;
}

// -onlyents auf eine frische Kopie der Original-.bsp; true, wenn jeder Lump
// ausser den Entities gleich ist. text = Entity-Lump danach.
bool nurEntitaeten(const Q3map2Aufruf& a, const fs::path& ordner, const std::string& stamm, const std::string& originalDaten,
                   std::string& text, std::string& bericht) {
    {
        std::ofstream f(ordner / (stamm + ".bsp"), std::ios::binary | std::ios::trunc);
        f.write(originalDaten.data(), static_cast<std::streamsize>(originalDaten.size()));
    }
    BspDatei vorher;
    if (!bspLesen(originalDaten, vorher)) {
        bericht = "Original unlesbar";
        return false;
    }
    const bool lichter = bspHatLichter(bspEntitaetenText(vorher));
    const int rc = ausfuehren(onlyentsBefehl(a, mitSchraeg((ordner / (stamm + ".map")).string()), lichter), ordner / (stamm + "_onlyents.log"));
    // q3map2 hat den Entity-Lump geschrieben; die uebrigen Lumps kommen Byte
    // fuer Byte aus dem Original (bspEntitaetenEinsetzen)
    const std::string fertig = rc == 0 ? bspEntitaetenEinsetzen(originalDaten, lies(ordner / (stamm + ".bsp"))) : std::string();
    BspDatei nachher;
    if (rc != 0 || fertig.empty() || !bspLesen(fertig, nachher)) {
        bericht = "rc " + std::to_string(rc);
        return false;
    }
    bool gleich = vorher.ident == nachher.ident && vorher.version == nachher.version && vorher.lumps.size() == nachher.lumps.size();
    bericht.clear();
    for (std::size_t i = 1; gleich && i < vorher.lumps.size(); ++i) {
        if (vorher.lumps[i] != nachher.lumps[i]) {
            gleich = false;
            bericht = "Lump " + std::to_string(i) + " anders";
        }
    }
    text = bspEntitaetenText(nachher);
    return gleich;
}

// Der Spielordner aus der CMake-Variable: "<GameData>/base" (bei Movie Duels
// "<base>;<MD>") -> "<GameData>/" fuer -fs_basepath.
std::string spielBasis(std::string arg) {
    const std::size_t semi = arg.find(';');
    if (semi != std::string::npos) { arg.resize(semi); }
    arg = mitSchraeg(arg);
    while (!arg.empty() && arg.back() == '/') { arg.pop_back(); }
    const std::size_t s = arg.rfind('/');
    std::string letzter = s == std::string::npos ? arg : arg.substr(s + 1);
    for (char& c : letzter) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    if (letzter == "base" && s != std::string::npos) { arg.resize(s); }
    return arg.empty() ? arg : arg + "/";
}

}  // namespace

int main(int argc, char** argv) {
    // --- Stufe 6: schaltbare Lichter (SetLightStyles: Stil 32+j in der Reihenfolge) ---
    {
        const std::string bsp =
            "{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"light\"\n\"targetname\" \"a\"\n\"style\" \"32\"\n\"switch_style\" \"5\"\n}\n"
            "{\n\"classname\" \"light\"\n\"targetname\" \"b\"\n\"style\" \"33\"\n}\n{\n\"classname\" \"light\"\n\"targetname\" \"a\"\n\"style\" \"32\"\n\"switch_style\" \"5\"\n}\n"
            "{\n\"classname\" \"func_door\"\n\"targetname\" \"t\"\n}\n";
        const std::string karte =
            "// entity 0\n{\n\"classname\" \"worldspawn\"\n// brush 0\n{\n( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) a/b 0 0 0 0.5 0.5 0 0 0\n}\n}\n"
            "{\n\"classname\" \"light\"\n\"targetname\" \"a\"\n\"style\" \"5\"\n}\n{\n\"classname\" \"light\"\n\"targetname\" \"b\"\n}\n";
        const auto sb = schaltlichter(bsp, true);
        const auto sk = schaltlichter(karte, false);
        pruefe(sb.size() == 2 && sb[0] == std::make_pair(std::string("a"), 5) && sb[1] == std::make_pair(std::string("b"), 0),
               "schaltlichter (BSP): a/5 und b/0, doppeltes a einmal, func_door nicht");
        pruefe(sb == sk, "schaltlichter: .bsp (switch_style) und .map (style) gleich");
        const std::string vertauscht = "{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"light\"\n\"targetname\" \"b\"\n}\n"
                                       "{\n\"classname\" \"light\"\n\"targetname\" \"a\"\n\"style\" \"5\"\n}\n";
        pruefe(schaltlichter(vertauscht, false) != sb, "schaltlichter: vertauschte Reihenfolge ist anders (Stile wuerden sich verschieben)");
        const std::string mehr = karte + "{\n\"classname\" \"light_spot\"\n\"targetname\" \"c\"\n}\n";
        pruefe(schaltlichter(mehr, false) != sb, "schaltlichter: ein neues schaltbares Licht ist anders");
        const std::string ohne = karte + "{\n\"classname\" \"light\"\n\"light\" \"300\"\n}\n";
        pruefe(schaltlichter(ohne, false) == sb, "schaltlichter: ein Licht ohne targetname zaehlt nicht");
    }
    // --- Entity-Lump einsetzen: Lumps des Originals bleiben, Reihenfolge wie dort
    {
        // RBSP mit 18 Lumps, Lump 0 (Entitaeten) liegt in der Datei hinten
        const auto setze = [](std::string& d, std::size_t o, std::uint32_t v) {
            for (std::size_t i = 0; i < 4; ++i) { d[o + i] = static_cast<char>((v >> (8 * i)) & 0xFFU); }
        };
        std::string orig = "RBSP";
        orig += std::string("\x01\0\0\0", 4);
        orig += std::string(18 * 8, '\0');
        std::string koerper;
        for (std::size_t i = 1; i < 18; ++i) {
            setze(orig, 8 + i * 8, static_cast<std::uint32_t>(8 + 18 * 8 + koerper.size()));
            setze(orig, 12 + i * 8, 4);
            koerper += std::string(4, static_cast<char>('A' + i));
        }
        setze(orig, 8, static_cast<std::uint32_t>(8 + 18 * 8 + koerper.size()));
        setze(orig, 12, 6);
        koerper += std::string("{x}\n\0\0", 6);
        orig += koerper;
        std::string neu = orig.substr(0, 8 + 18 * 8);
        setze(neu, 8, static_cast<std::uint32_t>(neu.size()));
        setze(neu, 12, 10);
        neu += std::string("{neu 1}\n\0\0", 10);
        neu += koerper;   // die anderen Lumps zeigen weiter dorthin (Groesse egal)
        for (std::size_t i = 1; i < 18; ++i) { setze(neu, 8 + i * 8, static_cast<std::uint32_t>(8 + 18 * 8 + 10)); }
        const std::string ein = bspEntitaetenEinsetzen(orig, neu);
        BspDatei a;
        BspDatei b;
        bool ok = bspLesen(orig, a) && bspLesen(ein, b) && b.lumps.size() == 18 && b.lumps[0] == std::string("{neu 1}\n\0\0", 10);
        for (std::size_t i = 1; ok && i < 18; ++i) { ok = a.lumps[i] == b.lumps[i]; }
        pruefe(ok && bspEntitaetenText(b) == "{neu 1}\n", "bspEntitaetenEinsetzen: Entity-Lump neu, alle anderen gleich");
        pruefe(ein.size() % 4 == 0 && ein.compare(8 + 18 * 8, 4, "BBBB") == 0, "bspEntitaetenEinsetzen: Reihenfolge wie im Original, auf 4 Bytes");
        pruefe(bspHatLichter("{\n\"classname\" \"light\"\n}") && bspHatLichter("\"classname\" \"lightJunior\"") &&
                   !bspHatLichter("\"classname\" \"info_null\"\n\"light\" \"300\""),
               "bspHatLichter: classname light...");
        pruefe(bspEntitaetenEinsetzen(orig, "IBSP").empty(), "bspEntitaetenEinsetzen: andere Art -> leer");
    }
    // --- Befehle wie NRC
    {
        Q3map2Aufruf a{"C:/Spiel/bhed_q3map2.exe", "ja", "C:/Spiel/GameData/", "", "MD"};
        pruefe(convertBefehl(a, true, "C:/arbeit/duel1.bsp") ==
                   "\"C:/Spiel/bhed_q3map2.exe\" -v -game ja -fs_basepath \"C:/Spiel/GameData/\" -fs_homepath \"\" -fs_game MD -convert "
                   "-format map_bp \"C:/arbeit/duel1.bsp\"",
               "convertBefehl wie map.cpp:1585-1594 (map_bp)");
        a.spielTyp.clear();
        pruefe(convertBefehl(a, false, "x.bsp").find(" -game quake3 ") != std::string::npos &&
                   convertBefehl(a, false, "x.bsp").find(" -convert -format map \"x.bsp\"") != std::string::npos,
               "convertBefehl: ohne q3map2_type quake3, Quake-3-Format map");
        pruefe(convertZiel("C:/a.b/karte.bsp") == "C:/a.b/karte_converted.map" && convertZiel("C:/a.b/karte") == "C:/a.b/karte_converted.map",
               "convertZiel: PathExtensionless + _converted.map");
        a.spielTyp = "jk2";
        a.fsGame = "base";
        pruefe(onlyentsBefehl(a, "C:/arbeit/k.map").find(" -game jk2 ") != std::string::npos &&
                   onlyentsBefehl(a, "C:/arbeit/k.map").find(" -fs_game base -bsp -onlyents \"C:/arbeit/k.map\"") != std::string::npos,
               "onlyentsBefehl: -bsp -onlyents");
        pruefe(eingebackeneEntitaet("misc_model") && eingebackeneEntitaet("MISC_MODEL") && eingebackeneEntitaet("_decal") &&
                   eingebackeneEntitaet("_skybox") && !eingebackeneEntitaet("misc_model_breakable") && !eingebackeneEntitaet("light"),
               "eingebackene Entitaeten wie UnparseEntities");
    }
    // --- Fingerabdruck
    {
        // Wuerfel 0..64 (Punktreihenfolge wie Brush 1 in nrc_tiny_structural_brush.map)
        const std::string text =
            "{\n\"classname\" \"worldspawn\"\n{\n( 64 0 64 ) ( 64 -64 64 ) ( 0 0 64 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n"
            "( 64 64 32 ) ( 0 64 32 ) ( 64 64 0 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n( 64 0 32 ) ( 64 0 0 ) ( 64 -64 32 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n"
            "( 0 0 0 ) ( 0 0 32 ) ( 64 0 0 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n( 0 -64 0 ) ( 0 0 0 ) ( 0 -64 32 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n"
            "( 0 0 0 ) ( 64 -64 0 ) ( 64 0 0 ) common/caulk 0 0 0 0.5 0.5 0 0 0\n}\n}\n{\n\"classname\" \"light\"\n\"origin\" \"32 32 32\"\n}\n";
        Karte k;
        (void)karteLesen(text, k);
        const std::uint64_t f0 = geometrieFingerabdruck(k);
        pruefe(f0 == geometrieFingerabdruck(k) && k.entitaeten().size() == 2 && k.entitaeten()[0]->kinder().size() == 1,
               "Fingerabdruck: gleich bei gleicher Karte (1 Pinsel, 2 Entitaeten)");
        k.entitaeten()[1]->setzen("light", "500");
        k.entitaeten()[1]->setzen("origin", "16 16 16");
        k.entitaeten()[0]->setzen("message", "neu");
        pruefe(f0 == geometrieFingerabdruck(k), "Fingerabdruck: gleich nach Schluesseln und verschobener Punkt-Entity");
        Karte k2;
        (void)karteLesen(text + "{\n\"classname\" \"info_null\"\n\"origin\" \"0 0 0\"\n}\n", k2);
        pruefe(f0 == geometrieFingerabdruck(k2), "Fingerabdruck: gleich mit neuer Punkt-Entity");
        Karte k3;
        std::string verschoben = text;
        verschoben.replace(verschoben.find("( 64 0 64 ) ( 64 -64 64 ) ( 0 0 64 )"), 36, "( 64 0 72 ) ( 64 -64 72 ) ( 0 0 72 )");
        (void)karteLesen(verschoben, k3);
        pruefe(f0 != geometrieFingerabdruck(k3), "Fingerabdruck: anders mit verschobener Flaeche");
        Karte k4;
        std::string textur = text;
        textur.replace(textur.find("common/caulk"), 12, "common/nodraw");
        (void)karteLesen(textur, k4);
        pruefe(f0 != geometrieFingerabdruck(k4), "Fingerabdruck: anders mit anderer Textur");
        Karte k5;
        (void)karteLesen(text + "{\n\"classname\" \"misc_model\"\n\"model\" \"models/map_objects/x.md3\"\n\"origin\" \"0 0 0\"\n}\n", k5);
        pruefe(f0 != geometrieFingerabdruck(k5), "Fingerabdruck: anders mit neuem misc_model (eingebacken)");
    }
    // --- mit q3map2 an raum.nrc.bsp
    const std::string q3 = argc > 1 ? argv[1] : "-";
    const fs::path daten = argc > 2 ? fs::path(argv[2]) : fs::path("tests/karte/q3map2");
    if (q3 != "-" && fs::is_regular_file(q3)) {
        const fs::path ordner = fs::temp_directory_path() / "bhed_kartebspoeffnen";
        std::error_code ec;
        fs::remove_all(ordner, ec);
        fs::create_directories(ordner, ec);
        const std::string original = lies(daten / "raum.nrc.bsp");
        {
            std::ofstream f(ordner / "raum.bsp", std::ios::binary);
            f.write(original.data(), static_cast<std::streamsize>(original.size()));
        }
        const Q3map2Aufruf a{mitSchraeg(q3), "ja", mitSchraeg((ordner / "leer").string()) + "/", "", "base"};
        fs::create_directories(ordner / "leer" / "base", ec);
        const int rc = ausfuehren(convertBefehl(a, true, mitSchraeg((ordner / "raum.bsp").string())), ordner / "convert.log");
        Karte k;
        const bool geladen = rc == 0 && laden(convertZiel((ordner / "raum.bsp").string()), k);
        const Zahlen z = zaehlen(k);
        pruefe(geladen && z.entitaeten >= 3 && z.pinsel >= 10, "raum: -convert und laden (" + std::to_string(z.entitaeten) + " Entitaeten, " +
                                                                    std::to_string(z.pinsel) + " Pinsel, " + std::to_string(z.patches) + " Patches)");
        const std::uint64_t f0 = geometrieFingerabdruck(k);
        if (geladen) {
            k.entitaeten()[0]->setzen("message", "Stufe 4 bhed_test");
            // Punkt-Entity verschoben (kein light: q3map2 nimmt Lichter ohne
            // targetname nicht in die .bsp - SetLightStyles, writebsp.cpp:246-250)
            for (const auto& en : k.entitaeten()) {
                if (en->klassenname().rfind("info_player", 0) == 0) {
                    en->setzen("origin", "-100 0 100");
                    break;
                }
            }
        }
        pruefe(f0 == geometrieFingerabdruck(k), "raum: nach der Entity-Aenderung derselbe Fingerabdruck");
        std::string fehlerText;
        pruefe(karteSpeichern((ordner / "raum.map").string(), k, false, &fehlerText), "raum: .map gespeichert " + fehlerText);
        std::string ent;
        std::string bericht;
        pruefe(nurEntitaeten(a, ordner, "raum", original, ent, bericht), "raum: -onlyents - jeder Lump ausser den Entities Byte-gleich " + bericht);
        pruefe(ent.find("\"message\" \"Stufe 4 bhed_test\"") != std::string::npos && ent.find("\"origin\" \"-100 0 100\"") != std::string::npos,
               "raum: der Entity-Lump traegt die Aenderung");
    } else {
        std::printf("(ohne bhed_q3map2.exe: nur die Teile ohne q3map2)\n");
    }
    // --- Original-BSPs der drei Spiele gegen die Massenlast
    if (argc > 6 && q3 != "-" && std::string(argv[3]).size() > 0) {
        const fs::path dekomp = argv[3];
        struct Fall {
            const char* spiel;
            const char* typ;
            int basis;   // argv-Index des Spielordners
            const char* fsGame;
            const char* karte;
        };
        const Fall faelle[] = {{"ja", "ja", 4, "base", "t1_inter"},       {"ja", "ja", 4, "base", "mp__duel1"},
                               {"jk2", "jk2", 5, "base", "duel_pit"},     {"jk2", "jk2", 5, "base", "kejim_post"},
                               {"md", "ja", 6, "MD", "duel_twinsuns"},     {"md", "ja", 6, "MD", "md_ch_ben"}};
        for (const Fall& f : faelle) {
            const fs::path bsp = dekomp / f.spiel / "bsp" / (std::string(f.karte) + ".bsp");
            const fs::path ref = dekomp / f.spiel / "bp" / (std::string(f.karte) + ".map");
            if (!fs::is_regular_file(bsp) || !fs::is_regular_file(ref) || std::string(argv[f.basis]).empty()) {
                std::printf("(%s/%s fehlt)\n", f.spiel, f.karte);
                continue;
            }
            const fs::path ordner = fs::temp_directory_path() / "bhed_kartebspoeffnen" / f.karte;
            std::error_code ec;
            fs::create_directories(ordner, ec);
            const std::string original = lies(bsp);
            {
                std::ofstream o(ordner / (std::string(f.karte) + ".bsp"), std::ios::binary | std::ios::trunc);
                o.write(original.data(), static_cast<std::streamsize>(original.size()));
            }
            const Q3map2Aufruf a{mitSchraeg(q3), f.typ, spielBasis(argv[f.basis]), "", f.fsGame};
            const int rc = ausfuehren(convertBefehl(a, true, mitSchraeg((ordner / (std::string(f.karte) + ".bsp")).string())), ordner / "convert.log");
            Karte uns;
            Karte nrc;
            const bool ok = rc == 0 && laden(convertZiel((ordner / (std::string(f.karte) + ".bsp")).string()), uns) && laden(ref, nrc);
            const Zahlen a1 = zaehlen(uns);
            const Zahlen b1 = zaehlen(nrc);
            pruefe(ok && a1.entitaeten == b1.entitaeten && a1.pinsel == b1.pinsel && a1.patches == b1.patches,
                   std::string(f.spiel) + "/" + f.karte + ": Entitaeten/Pinsel/Patches " + std::to_string(a1.entitaeten) + "/" + std::to_string(a1.pinsel) +
                       "/" + std::to_string(a1.patches) + " wie die Massenlast " + std::to_string(b1.entitaeten) + "/" + std::to_string(b1.pinsel) + "/" +
                       std::to_string(b1.patches));
            std::string fehlerText;
            const bool gesp = ok && karteSpeichern((ordner / (std::string(f.karte) + ".map")).string(), uns, false, &fehlerText);
            std::string ent;
            std::string bericht;
            pruefe(gesp && nurEntitaeten(a, ordner, f.karte, original, ent, bericht),
                   std::string(f.spiel) + "/" + f.karte + ": -onlyents ohne Aenderung - jeder Lump ausser den Entities gleich " + bericht);
        }
    }
    std::printf("%d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
