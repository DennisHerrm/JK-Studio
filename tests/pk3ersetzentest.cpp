// pk3ersetzentest.cpp - rc609 W2a Punkt 4: einen Eintrag einer vorhandenen .pk3 ersetzen
//
// Ein Archiv mit 5 Eintraegen, gemischt ohne Packung und deflate (einer mit
// Zusatzfeld im lokalen Kopf, einer mit Datenbeschreibung hinter den Daten wie
// von Java-Packern), dazu ein Ordnereintrag und ein Archivkommentar. Einen
// Eintrag ersetzen:
//   - die anderen 4 sind roh Byte-gleich (lokaler Kopf + komprimierte Daten +
//     Datenbeschreibung; der Zentraleintrag bis auf den Versatz),
//   - readPk3File liest alle, die CRC-32 im lokalen Kopf und im Verzeichnis
//     stimmt, <pk3>.bak ist der Stand davor, keine Temp-Datei,
//   - ein fehlender Eintrag wird angehaengt,
//   - ein gesperrtes Archiv (offen wie im Spiel) ergibt eine Meldung, das
//     Original bleibt Byte-gleich.
//
// Aufruf: pk3ersetzentest <Arbeitsordner>
#include "bhed/pk3.h"
#include "bhed/sicherschreiben.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
void schreibeRoh(const fs::path& p, const std::string& t) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}
std::uint32_t crc(const std::string& d) {
    return static_cast<std::uint32_t>(crc32(0L, reinterpret_cast<const Bytef*>(d.data()), static_cast<uInt>(d.size())));
}
void u16(std::string& o, std::uint32_t v) {
    o += static_cast<char>(v & 0xFFU);
    o += static_cast<char>((v >> 8U) & 0xFFU);
}
void u32(std::string& o, std::uint32_t v) {
    u16(o, v & 0xFFFFU);
    u16(o, v >> 16U);
}
std::uint32_t l16(const std::string& b, std::size_t at) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(b[at])) | (static_cast<std::uint32_t>(static_cast<unsigned char>(b[at + 1])) << 8U);
}
std::uint32_t l32(const std::string& b, std::size_t at) { return l16(b, at) | (l16(b, at + 2) << 16U); }

// Rohes deflate (ohne zlib-Kopf), wie Zip es will.
std::string packen(const std::string& d) {
    z_stream z{};
    deflateInit2(&z, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    std::string out(deflateBound(&z, static_cast<uLong>(d.size())) + 16, '\0');
    z.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(d.data()));
    z.avail_in = static_cast<uInt>(d.size());
    z.next_out = reinterpret_cast<Bytef*>(out.data());
    z.avail_out = static_cast<uInt>(out.size());
    deflate(&z, Z_FINISH);
    out.resize(z.total_out);
    deflateEnd(&z);
    return out;
}

struct Eintrag {
    std::string name;
    std::string inhalt;
    bool deflate = false;
    bool zusatz = false;          // Zusatzfeld im lokalen Kopf
    bool beschreibung = false;    // Bit 3: Groessen/CRC hinter den Daten
};

// Ein Zip wie von fremden Packern: Zeitstempel je Eintrag verschieden.
std::string baueArchiv(const std::vector<Eintrag>& es) {
    std::string lokal;
    std::string zentral;
    std::uint32_t zeit = 0x6000;
    int anzahl = 0;
    for (const Eintrag& e : es) {
        const std::string daten = e.deflate ? packen(e.inhalt) : e.inhalt;
        const std::uint32_t c = crc(e.inhalt);
        const std::uint32_t flags = e.beschreibung ? 0x0008U : 0U;
        const std::string zus = e.zusatz ? std::string("\x55\x54\x05\x00\x01\x11\x22\x33\x44", 9) : std::string();
        const auto versatz = static_cast<std::uint32_t>(lokal.size());
        u32(lokal, 0x04034B50U);
        u16(lokal, 20);
        u16(lokal, flags);
        u16(lokal, e.deflate ? 8 : 0);
        u16(lokal, zeit);
        u16(lokal, 0x5A21);
        u32(lokal, e.beschreibung ? 0 : c);
        u32(lokal, e.beschreibung ? 0 : static_cast<std::uint32_t>(daten.size()));
        u32(lokal, e.beschreibung ? 0 : static_cast<std::uint32_t>(e.inhalt.size()));
        u16(lokal, static_cast<std::uint32_t>(e.name.size()));
        u16(lokal, static_cast<std::uint32_t>(zus.size()));
        lokal += e.name + zus + daten;
        if (e.beschreibung) {
            u32(lokal, 0x08074B50U);
            u32(lokal, c);
            u32(lokal, static_cast<std::uint32_t>(daten.size()));
            u32(lokal, static_cast<std::uint32_t>(e.inhalt.size()));
        }
        u32(zentral, 0x02014B50U);
        u16(zentral, 0x031E);
        u16(zentral, 20);
        u16(zentral, flags);
        u16(zentral, e.deflate ? 8 : 0);
        u16(zentral, zeit);
        u16(zentral, 0x5A21);
        u32(zentral, c);
        u32(zentral, static_cast<std::uint32_t>(daten.size()));
        u32(zentral, static_cast<std::uint32_t>(e.inhalt.size()));
        u16(zentral, static_cast<std::uint32_t>(e.name.size()));
        u16(zentral, 0);
        u16(zentral, 0);
        u16(zentral, 0);
        u16(zentral, 0);
        u32(zentral, e.name.back() == '/' ? 0x10U : 0x20U);
        u32(zentral, versatz);
        zentral += e.name;
        zeit += 0x21;
        ++anzahl;
    }
    const std::string kommentar = "JK Studio Probe";
    std::string ende;
    u32(ende, 0x06054B50U);
    u16(ende, 0);
    u16(ende, 0);
    u16(ende, static_cast<std::uint32_t>(anzahl));
    u16(ende, static_cast<std::uint32_t>(anzahl));
    u32(ende, static_cast<std::uint32_t>(zentral.size()));
    u32(ende, static_cast<std::uint32_t>(lokal.size()));
    u16(ende, static_cast<std::uint32_t>(kommentar.size()));
    return lokal + zentral + ende + kommentar;
}

// Roh je Eintrag: lokaler Block (Kopf bis vor den naechsten Kopf bzw. das
// Verzeichnis) und Zentraleintrag ohne das Versatzfeld.
struct Roh {
    std::string lokal;
    std::string zentralOhneVersatz;
    std::uint32_t crcZentral = 0;
    std::uint32_t crcLokal = 0;
    bool lokalHatCrc = false;
};
std::map<std::string, Roh> rohLesen(const std::string& a) {
    std::map<std::string, Roh> aus;
    std::size_t eocd = a.size() - 22;
    while (eocd > 0 && l32(a, eocd) != 0x06054B50U) { --eocd; }
    const std::uint32_t n = l16(a, eocd + 10);
    const std::uint32_t dirOff = l32(a, eocd + 16);
    std::vector<std::pair<std::uint32_t, std::string>> folge;
    std::size_t p = dirOff;
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint32_t nl = l16(a, p + 28);
        const std::uint32_t xl = l16(a, p + 30);
        const std::uint32_t kl = l16(a, p + 32);
        const std::string name = a.substr(p + 46, nl);
        std::string z = a.substr(p, 46 + nl + xl + kl);
        z.replace(42, 4, "????");
        Roh& r = aus[name];
        r.zentralOhneVersatz = z;
        r.crcZentral = l32(a, p + 16);
        folge.emplace_back(l32(a, p + 42), name);
        p += 46U + nl + xl + kl;
    }
    std::sort(folge.begin(), folge.end());
    for (std::size_t i = 0; i < folge.size(); ++i) {
        const std::uint32_t von = folge[i].first;
        const std::uint32_t bis = i + 1 < folge.size() ? folge[i + 1].first : dirOff;
        Roh& r = aus[folge[i].second];
        r.lokal = a.substr(von, bis - von);
        r.lokalHatCrc = (l16(a, von + 6) & 0x0008U) == 0U;
        r.crcLokal = l32(a, von + 14);
    }
    return aus;
}

}  // namespace

int main(int argc, char** argv) {
    const fs::path ordner = fs::path(argc > 1 ? argv[1] : "pk3ersetzen_lauf") / "pk3ersetzen";
    std::error_code ec;
    fs::remove_all(ordner, ec);
    fs::create_directories(ordner, ec);
    const fs::path pk3 = ordner / "zz_probe.pk3";

    std::string gross;
    for (int i = 0; i < 3000; ++i) { gross += "// Zeile " + std::to_string(i) + " affect ( \"luke\", FLUSH ) { wait ( 1000 ); }\n"; }
    const std::vector<Eintrag> anfang = {
        {"maps/", "", false, false, false},
        {"maps/probe.bsp", std::string("RBSP\x01\x00\x00\x00", 8) + std::string(5000, '\x07'), false, true, false},
        {"maps/probe.ent", "{\n\"classname\" \"worldspawn\"\n}\n", false, false, false},
        {"scripts/probe/intro.txt", gross, true, false, false},
        {"scripts/probe/intro.IBI", std::string("IBI\0", 4) + gross.substr(0, 9000), true, false, true},
        {"levelshots/probe.jpg", std::string(777, '\x42'), true, true, false},
    };
    const std::string original = baueArchiv(anfang);
    schreibeRoh(pk3, original);
    const auto vorher = rohLesen(original);

    const std::string neuerText = "{\n\"classname\" \"worldspawn\"\n\"message\" \"geaendert\"\n}\n{\n\"classname\" \"info_player_start\"\n}\n";
    std::string fehler;
    bhed::SchreibFehler art = bhed::SchreibFehler::Keiner;
    const bool ok = bhed::pk3EintragErsetzen(pk3.string(), "maps/probe.ent", neuerText, &fehler, &art);
    pruefe(ok, "Eintrag ersetzt (" + fehler + ")");
    const std::string danach = lies(pk3);
    const auto nachher = rohLesen(danach);
    int rohGleich = 0;
    for (const Eintrag& e : anfang) {
        if (e.name == "maps/probe.ent" || e.name.back() == '/') { continue; }
        const auto a = vorher.find(e.name);
        const auto b = nachher.find(e.name);
        const bool gleich = a != vorher.end() && b != nachher.end() && a->second.lokal == b->second.lokal &&
                            a->second.zentralOhneVersatz == b->second.zentralOhneVersatz;
        rohGleich += gleich ? 1 : 0;
        pruefe(gleich, "roh Byte-gleich (lokal + Daten + Verzeichnis ohne Versatz): " + e.name);
    }
    std::printf("  %d von 4 anderen Eintraegen roh gleich, Archiv %zu -> %zu Bytes\n", rohGleich, original.size(), danach.size());
    pruefe(nachher.count("maps/") == 1U && nachher.at("maps/").zentralOhneVersatz == vorher.at("maps/").zentralOhneVersatz,
           "Ordnereintrag bleibt");
    pruefe(danach.size() > 15 && danach.substr(danach.size() - 15) == "JK Studio Probe", "Archivkommentar bleibt");

    bhed::Pk3 arc;
    pruefe(bhed::readPk3Directory(pk3.string(), arc), "readPk3Directory liest das neue Archiv");
    pruefe(arc.entries.size() == 5U, "5 Eintraege (" + std::to_string(arc.entries.size()) + ")");
    for (const Eintrag& e : anfang) {
        if (e.name.back() == '/') { continue; }
        const bhed::Pk3Entry* x = arc.find(e.name);
        std::string inhalt;
        const bool gelesen = x != nullptr && bhed::readPk3File(arc, *x, inhalt);
        const std::string& soll = e.name == "maps/probe.ent" ? neuerText : e.inhalt;
        pruefe(gelesen && inhalt == soll, "readPk3File: " + e.name);
    }
    if (const auto it = nachher.find("maps/probe.ent"); it != nachher.end()) {
        pruefe(it->second.crcZentral == crc(neuerText) && it->second.lokalHatCrc && it->second.crcLokal == crc(neuerText),
               "CRC-32 des neuen Eintrags im Verzeichnis und im lokalen Kopf");
    } else {
        pruefe(false, "neuer Eintrag im Verzeichnis");
    }
    pruefe(lies(fs::path(pk3.string() + ".bak")) == original, "<pk3>.bak ist der Stand davor");
    pruefe(!fs::exists(fs::path(pk3.string() + ".tmp"), ec), "keine Temp-Datei");

    // Fehlender Eintrag: wird angehaengt, die anderen bleiben roh gleich.
    const std::string zusatz = "neu";
    pruefe(bhed::pk3EintragErsetzen(pk3.string(), "scripts/probe/neu.txt", zusatz, &fehler), "fehlender Eintrag angehaengt");
    {
        const std::string drei = lies(pk3);
        const auto r3 = rohLesen(drei);
        int gleich = 0;
        for (const auto& [name, r] : nachher) { gleich += r3.count(name) != 0U && r3.at(name).lokal == r.lokal ? 1 : 0; }
        bhed::Pk3 a3;
        std::string t3;
        const bhed::Pk3Entry* x3 = bhed::readPk3Directory(pk3.string(), a3) ? a3.find("scripts/probe/neu.txt") : nullptr;
        pruefe(gleich == static_cast<int>(nachher.size()) && a3.entries.size() == 6U && x3 != nullptr && bhed::readPk3File(a3, *x3, t3) &&
                   t3 == zusatz,
               "angehaengt: 6 Eintraege, alle vorigen roh gleich");
    }

    // Gesperrt: das Spiel haelt die .pk3 offen.
    const std::string vorSperre = lies(pk3);
    {
        std::ifstream offen(pk3, std::ios::binary);
        fehler.clear();
        art = bhed::SchreibFehler::Keiner;
        const bool ok2 = bhed::pk3EintragErsetzen(pk3.string(), "maps/probe.ent", "x", &fehler, &art);
        std::printf("  gesperrt: Rueckgabe %s, Meldung \"%s\"\n", ok2 ? "true" : "false", fehler.c_str());
        pruefe(!ok2 && art == bhed::SchreibFehler::Gesperrt && !fehler.empty(), "gesperrtes Archiv: false mit Meldung");
    }
    pruefe(lies(pk3) == vorSperre, "gesperrtes Archiv: Original Byte-gleich");
    pruefe(!fs::exists(fs::path(pk3.string() + ".tmp"), ec), "gesperrt: keine Temp-Datei");

    // Zwei Eintraege in einem Durchgang (Skript als .txt und .IBI): eine Sicherung.
    {
        const std::string vorZwei = lies(pk3);
        const auto rv = rohLesen(vorZwei);
        const bool ok3 = bhed::pk3EintraegeErsetzen(pk3.string(), {{"scripts/probe/intro.txt", "neu txt"}, {"scripts/probe/intro.IBI", "neu ibi"}});
        bhed::Pk3 a4;
        std::string t1;
        std::string t2;
        const bool gelesen = bhed::readPk3Directory(pk3.string(), a4) && a4.find("scripts/probe/intro.txt") != nullptr &&
                             a4.find("scripts/probe/intro.IBI") != nullptr && bhed::readPk3File(a4, *a4.find("scripts/probe/intro.txt"), t1) &&
                             bhed::readPk3File(a4, *a4.find("scripts/probe/intro.IBI"), t2);
        const auto rn = rohLesen(lies(pk3));
        int gleich = 0;
        for (const auto& [name, r] : rv) {
            if (name.rfind("scripts/probe/intro.", 0) == 0) { continue; }
            gleich += rn.count(name) != 0U && rn.at(name).lokal == r.lokal ? 1 : 0;
        }
        pruefe(ok3 && gelesen && t1 == "neu txt" && t2 == "neu ibi" && gleich == static_cast<int>(rv.size()) - 2 &&
                   lies(fs::path(pk3.string() + ".bak")) == vorZwei,
               "zwei Eintraege in einem Durchgang, die anderen roh gleich, .bak = Stand davor");
    }

    fs::remove_all(ordner, ec);
    std::printf("pk3ersetzen: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
