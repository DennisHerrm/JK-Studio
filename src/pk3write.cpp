// pk3write.cpp - ein .pk3 schreiben (siehe pk3.h, writePk3)
#include "bhed/pk3.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>
#include "bhed/utf8pfad.h"

namespace bhed {
namespace {

std::uint32_t crc32(const std::string& d) {
    static const std::array<std::uint32_t, 256> tabelle = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1U) != 0U ? 0xEDB88320U ^ (c >> 1U) : c >> 1U;
            }
            t[i] = c;
        }
        return t;
    }();
    std::uint32_t c = 0xFFFFFFFFU;
    for (const char ch : d) {
        c = tabelle[(c ^ static_cast<unsigned char>(ch)) & 0xFFU] ^ (c >> 8U);
    }
    return c ^ 0xFFFFFFFFU;
}

void u16(std::string& o, std::uint32_t v) {
    o += static_cast<char>(v & 0xFFU);
    o += static_cast<char>((v >> 8U) & 0xFFU);
}
void u32(std::string& o, std::uint32_t v) {
    u16(o, v & 0xFFFFU);
    u16(o, v >> 16U);
}

}  // namespace

bool writePk3(const std::string& path, const std::vector<Pk3Datei>& files, std::string* error) {
    const auto fehler = [error](const std::string& t) {
        if (error != nullptr) { *error = t; }
        return false;
    };
    // Zip ohne Zip64: hoechstens 65535 Eintraege und 4 GB.
    if (files.size() > 0xFFFFU) {
        return fehler("too many files");
    }
    // Fester Zeitstempel 1.1.2026 - gleicher Inhalt gibt dasselbe Archiv.
    constexpr std::uint32_t kZeit = 0;
    constexpr std::uint32_t kDatum = ((2026U - 1980U) << 9U) | (1U << 5U) | 1U;
    std::string lokal;
    std::string verzeichnis;
    for (const Pk3Datei& f : files) {
        if (f.name.empty() || f.name.size() > 0xFFFFU) {
            return fehler("bad file name");
        }
        if (lokal.size() + f.inhalt.size() + f.name.size() + 64 > 0xFFFFFFF0ULL) {
            return fehler("archive too large");
        }
        const std::uint32_t crc = crc32(f.inhalt);
        const auto groesse = static_cast<std::uint32_t>(f.inhalt.size());
        const auto versatz = static_cast<std::uint32_t>(lokal.size());
        u32(lokal, 0x04034B50U);
        u16(lokal, 10);          // noetige Fassung 1.0
        u16(lokal, 0x0800U);     // Namen in UTF-8
        u16(lokal, 0);           // ohne Packung
        u16(lokal, kZeit);
        u16(lokal, kDatum);
        u32(lokal, crc);
        u32(lokal, groesse);
        u32(lokal, groesse);
        u16(lokal, static_cast<std::uint32_t>(f.name.size()));
        u16(lokal, 0);
        lokal += f.name;
        lokal += f.inhalt;

        u32(verzeichnis, 0x02014B50U);
        u16(verzeichnis, 20);
        u16(verzeichnis, 10);
        u16(verzeichnis, 0x0800U);
        u16(verzeichnis, 0);
        u16(verzeichnis, kZeit);
        u16(verzeichnis, kDatum);
        u32(verzeichnis, crc);
        u32(verzeichnis, groesse);
        u32(verzeichnis, groesse);
        u16(verzeichnis, static_cast<std::uint32_t>(f.name.size()));
        u16(verzeichnis, 0);
        u16(verzeichnis, 0);
        u16(verzeichnis, 0);
        u16(verzeichnis, 0);
        u32(verzeichnis, 0);
        u32(verzeichnis, versatz);
        verzeichnis += f.name;
    }
    std::string ende;
    u32(ende, 0x06054B50U);
    u16(ende, 0);
    u16(ende, 0);
    u16(ende, static_cast<std::uint32_t>(files.size()));
    u16(ende, static_cast<std::uint32_t>(files.size()));
    u32(ende, static_cast<std::uint32_t>(verzeichnis.size()));
    u32(ende, static_cast<std::uint32_t>(lokal.size()));
    u16(ende, 0);

    const std::filesystem::path ziel = ::bhed::utf8Pfad(path);
    std::filesystem::path tmp = ziel;
    tmp += ".tmp";
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) {
            return fehler("cannot write " + path + ".tmp");
        }
        o.write(lokal.data(), static_cast<std::streamsize>(lokal.size()));
        o.write(verzeichnis.data(), static_cast<std::streamsize>(verzeichnis.size()));
        o.write(ende.data(), static_cast<std::streamsize>(ende.size()));
        if (!o.flush()) {
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
            return fehler("cannot write " + path + ".tmp");
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, ziel, ec);
    if (ec) {
        std::error_code ec2;
        std::filesystem::remove(tmp, ec2);
        return fehler("cannot replace " + path + " (" + ec.message() + ") - is the game running?");
    }
    return true;
}

namespace {

std::uint32_t l16(const std::string& b, std::size_t at) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(b[at])) | (static_cast<std::uint32_t>(static_cast<unsigned char>(b[at + 1])) << 8U);
}
std::uint32_t l32(const std::string& b, std::size_t at) { return l16(b, at) | (l16(b, at + 2) << 16U); }
void setze32(std::string& b, std::size_t at, std::uint32_t v) {
    for (std::size_t i = 0; i < 4; ++i) { b[at + i] = static_cast<char>((v >> (8U * i)) & 0xFFU); }
}
// Name wie die Engine vergleicht (FS_FilenameCompare): Gross/klein und Trenner egal.
std::string vergleichsName(std::string s) {
    for (char& c : s) {
        if (c == '\\' || c == ':') { c = '/'; }
        if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c - 'A' + 'a'); }
    }
    return s;
}

}  // namespace

bool pk3EintraegeErsetzen(const std::string& pk3, const std::vector<Pk3Datei>& neu, std::string* fehler, SchreibFehler* art) {
    const auto nein = [fehler, art](SchreibFehler a, const std::string& t) {
        if (fehler != nullptr) { *fehler = t; }
        if (art != nullptr) { *art = a; }
        return false;
    };
    if (art != nullptr) { *art = SchreibFehler::Keiner; }
    if (neu.empty()) { return nein(SchreibFehler::Schreiben, "nothing to write"); }
    for (const Pk3Datei& d : neu) {
        if (d.name.empty() || d.name.size() > 0xFFFFU || d.name.back() == '/') { return nein(SchreibFehler::Schreiben, "bad entry name " + d.name); }
    }
    if (dateiGesperrt(pk3)) {
        return nein(SchreibFehler::Gesperrt, "cannot replace " + pk3 + " - it is open in another program (is the game running?)");
    }
    const std::filesystem::path quelle = ::bhed::utf8Pfad(pk3);
    std::ifstream f(quelle, std::ios::binary);
    if (!f) { return nein(SchreibFehler::Schreiben, "cannot read " + pk3); }
    f.seekg(0, std::ios::end);
    const std::streamoff groesse = f.tellg();
    if (groesse < 22) { return nein(SchreibFehler::Schreiben, pk3 + " is not a zip archive"); }
    // Endverzeichnis (hinten, bis zu 64 KiB Kommentar dahinter)
    const std::streamoff hinten = std::min<std::streamoff>(groesse, 0x10000 + 22);
    std::string schwanz(static_cast<std::size_t>(hinten), '\0');
    f.seekg(groesse - hinten);
    f.read(schwanz.data(), hinten);
    std::size_t eocd = std::string::npos;
    for (std::size_t i = schwanz.size() - 22;; --i) {
        if (l32(schwanz, i) == 0x06054B50U && i + 22 + l16(schwanz, i + 20) == schwanz.size()) {
            eocd = i;
            break;
        }
        if (i == 0) { break; }
    }
    if (eocd == std::string::npos) { return nein(SchreibFehler::Schreiben, pk3 + ": no end of central directory"); }
    const std::uint32_t anzahl = l16(schwanz, eocd + 10);
    const std::uint32_t dirGroesse = l32(schwanz, eocd + 12);
    const std::uint32_t dirVersatz = l32(schwanz, eocd + 16);
    if (l16(schwanz, eocd + 4) != 0 || l16(schwanz, eocd + 6) != 0 || anzahl != l16(schwanz, eocd + 8) || anzahl == 0xFFFFU ||
        dirVersatz == 0xFFFFFFFFU || static_cast<std::streamoff>(dirVersatz) + dirGroesse > groesse - static_cast<std::streamoff>(schwanz.size() - eocd)) {
        return nein(SchreibFehler::Schreiben, pk3 + ": multi-disk or zip64 archive");
    }
    const std::string kommentar = schwanz.substr(eocd + 22);
    std::string dir(dirGroesse, '\0');
    f.seekg(static_cast<std::streamoff>(dirVersatz));
    f.read(dir.data(), static_cast<std::streamsize>(dirGroesse));
    if (f.gcount() != static_cast<std::streamsize>(dirGroesse)) { return nein(SchreibFehler::Schreiben, pk3 + ": central directory cut off"); }
    // Zentraleintraege roh; je Eintrag, welche neue Datei ihn ersetzt (-1: keine)
    struct Zentral {
        std::string roh;
        std::uint32_t versatz = 0;
        std::string name;
        int neu = -1;
    };
    std::vector<Zentral> zentral;
    std::vector<bool> gefunden(neu.size(), false);
    std::size_t p = 0;
    for (std::uint32_t i = 0; i < anzahl; ++i) {
        if (p + 46 > dir.size() || l32(dir, p) != 0x02014B50U) { return nein(SchreibFehler::Schreiben, pk3 + ": central directory damaged"); }
        const std::size_t laenge = 46U + l16(dir, p + 28) + l16(dir, p + 30) + l16(dir, p + 32);
        if (p + laenge > dir.size()) { return nein(SchreibFehler::Schreiben, pk3 + ": central directory damaged"); }
        if ((l16(dir, p + 8) & 0x0001U) != 0U || l32(dir, p + 20) == 0xFFFFFFFFU || l32(dir, p + 42) == 0xFFFFFFFFU) {
            return nein(SchreibFehler::Schreiben, pk3 + ": encrypted or zip64 entry");
        }
        Zentral z{dir.substr(p, laenge), l32(dir, p + 42), dir.substr(p + 46, l16(dir, p + 28)), -1};
        if (z.versatz >= dirVersatz) { return nein(SchreibFehler::Schreiben, pk3 + ": entry outside the archive"); }
        for (std::size_t k = 0; k < neu.size(); ++k) {
            if (!gefunden[k] && vergleichsName(z.name) == vergleichsName(neu[k].name)) {
                gefunden[k] = true;
                z.neu = static_cast<int>(k);
                break;
            }
        }
        zentral.push_back(std::move(z));
        p += laenge;
    }
    // Lokale Bloecke: vom Kopf bis vor den naechsten Kopf (nach Versatz) bzw. das
    // Verzeichnis - so kommen Datenbeschreibungen und Luecken roh mit.
    std::vector<std::uint32_t> anfaenge;
    for (const Zentral& z : zentral) { anfaenge.push_back(z.versatz); }
    std::sort(anfaenge.begin(), anfaenge.end());
    anfaenge.erase(std::unique(anfaenge.begin(), anfaenge.end()), anfaenge.end());
    const auto blockEnde = [&](std::uint32_t von) {
        const auto it = std::upper_bound(anfaenge.begin(), anfaenge.end(), von);
        return it == anfaenge.end() ? dirVersatz : *it;
    };
    // Lokaler Kopf und Zentraleintrag einer neuen Datei (ohne Packung). alt: der
    // ersetzte Zentraleintrag (Zeit, Datum, Namen in UTF-8, Attribute bleiben).
    const auto kopfNeu = [](const Pk3Datei& d, const std::string* alt, std::string& lokal, std::string& zent) {
        const std::uint32_t c = crc32(d.inhalt);
        const auto g = static_cast<std::uint32_t>(d.inhalt.size());
        const std::uint32_t zeit = alt != nullptr ? l16(*alt, 12) : 0U;
        const std::uint32_t datum = alt != nullptr ? l16(*alt, 14) : (((2026U - 1980U) << 9U) | (1U << 5U) | 1U);
        const std::uint32_t flags = alt != nullptr ? (l16(*alt, 8) & 0x0800U) : 0x0800U;
        const std::string name = alt != nullptr ? alt->substr(46, l16(*alt, 28)) : d.name;
        lokal.clear();
        u32(lokal, 0x04034B50U);
        u16(lokal, 10);
        u16(lokal, flags);
        u16(lokal, 0);
        u16(lokal, zeit);
        u16(lokal, datum);
        u32(lokal, c);
        u32(lokal, g);
        u32(lokal, g);
        u16(lokal, static_cast<std::uint32_t>(name.size()));
        u16(lokal, 0);
        lokal += name;
        zent.clear();
        u32(zent, 0x02014B50U);
        u16(zent, alt != nullptr ? l16(*alt, 4) : 20U);
        u16(zent, 10);
        u16(zent, flags);
        u16(zent, 0);
        u16(zent, zeit);
        u16(zent, datum);
        u32(zent, c);
        u32(zent, g);
        u32(zent, g);
        u16(zent, static_cast<std::uint32_t>(name.size()));
        u16(zent, 0);
        u16(zent, 0);
        u16(zent, 0);
        u16(zent, alt != nullptr ? l16(*alt, 36) : 0U);
        u32(zent, alt != nullptr ? l32(*alt, 38) : 0U);
        u32(zent, 0);   // Versatz folgt
        zent += name;
    };
    // Schreiben: <pk3>.tmp, dann .bak, dann atomar umbenennen
    const std::string tmpPfad = pk3 + ".tmp";
    const std::filesystem::path tmp = ::bhed::utf8Pfad(tmpPfad);
    std::uint64_t ort = 0;
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) { return nein(SchreibFehler::Schreiben, "cannot write " + tmpPfad); }
        std::string puffer;
        const auto kopiere = [&](std::uint32_t von, std::uint32_t bis) {
            f.clear();
            f.seekg(static_cast<std::streamoff>(von));
            std::uint32_t rest = bis - von;
            while (rest > 0 && f && o) {
                const std::uint32_t n = std::min<std::uint32_t>(rest, 1U << 20U);
                puffer.resize(n);
                f.read(puffer.data(), n);
                if (f.gcount() != static_cast<std::streamsize>(n)) { return false; }
                o.write(puffer.data(), n);
                rest -= n;
            }
            ort += bis - von;
            return rest == 0 && static_cast<bool>(o);
        };
        std::string lokal;
        std::string zent;
        const auto schreibeNeu = [&](const Pk3Datei& d) {
            o.write(lokal.data(), static_cast<std::streamsize>(lokal.size()));
            o.write(d.inhalt.data(), static_cast<std::streamsize>(d.inhalt.size()));
            ort += lokal.size() + d.inhalt.size();
            return static_cast<bool>(o);
        };
        bool ok = anfaenge.empty() || kopiere(0, anfaenge.front());   // was vor dem ersten Kopf steht
        // In Archivreihenfolge (nach Versatz); ersetzte Bloecke werden neu geschrieben.
        std::vector<std::uint32_t> neuJeAnfang(anfaenge.size(), 0);
        std::string neuDir;
        std::vector<std::string> zentralNeu(zentral.size());
        for (std::size_t k = 0; ok && k < anfaenge.size(); ++k) {
            if (ort > 0xFFFFFFF0ULL) {
                ok = false;
                break;
            }
            neuJeAnfang[k] = static_cast<std::uint32_t>(ort);
            const auto z = std::find_if(zentral.begin(), zentral.end(), [&](const Zentral& x) { return x.versatz == anfaenge[k] && x.neu >= 0; });
            if (z != zentral.end()) {
                kopfNeu(neu[static_cast<std::size_t>(z->neu)], &z->roh, lokal, zent);
                setze32(zent, 42, static_cast<std::uint32_t>(ort));
                zentralNeu[static_cast<std::size_t>(z - zentral.begin())] = zent;
                ok = schreibeNeu(neu[static_cast<std::size_t>(z->neu)]);
            } else {
                ok = kopiere(anfaenge[k], blockEnde(anfaenge[k]));
            }
        }
        std::size_t eintraege = zentral.size();
        for (std::size_t i = 0; ok && i < zentral.size(); ++i) {
            if (!zentralNeu[i].empty()) {
                neuDir += zentralNeu[i];
            } else {
                const auto k = static_cast<std::size_t>(std::lower_bound(anfaenge.begin(), anfaenge.end(), zentral[i].versatz) - anfaenge.begin());
                std::string z = zentral[i].roh;
                setze32(z, 42, neuJeAnfang[k]);
                neuDir += z;
            }
        }
        for (std::size_t k = 0; ok && k < neu.size(); ++k) {   // anhaengen, was es noch nicht gab
            if (gefunden[k]) { continue; }
            kopfNeu(neu[k], nullptr, lokal, zent);
            setze32(zent, 42, static_cast<std::uint32_t>(ort));
            neuDir += zent;
            ++eintraege;
            ok = ort <= 0xFFFFFFF0ULL && schreibeNeu(neu[k]);
        }
        if (ok && (ort + neuDir.size() + 22 + kommentar.size() > 0xFFFFFFF0ULL || eintraege > 0xFFFEU)) { ok = false; }
        if (ok) {
            std::string ende;
            u32(ende, 0x06054B50U);
            u16(ende, 0);
            u16(ende, 0);
            u16(ende, static_cast<std::uint32_t>(eintraege));
            u16(ende, static_cast<std::uint32_t>(eintraege));
            u32(ende, static_cast<std::uint32_t>(neuDir.size()));
            u32(ende, static_cast<std::uint32_t>(ort));
            u16(ende, static_cast<std::uint32_t>(kommentar.size()));
            o.write(neuDir.data(), static_cast<std::streamsize>(neuDir.size()));
            o.write(ende.data(), static_cast<std::streamsize>(ende.size()));
            o.write(kommentar.data(), static_cast<std::streamsize>(kommentar.size()));
        }
        o.close();
        if (!ok || o.fail()) {
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
            return nein(SchreibFehler::Schreiben, "cannot write " + tmpPfad);
        }
    }
    f.close();
    // Sicherung: der Stand vor diesem Speichern
    {
        std::error_code ec;
        std::filesystem::copy_file(quelle, ::bhed::utf8Pfad(pk3 + ".bak"), std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            std::filesystem::remove(tmp, ec);
            return nein(SchreibFehler::Schreiben, "cannot write " + pk3 + ".bak");
        }
    }
    SchreibFehler a = SchreibFehler::Keiner;
    if (!dateiErsetzen(tmpPfad, pk3, &a)) {
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return nein(a, a == SchreibFehler::Gesperrt ? "cannot replace " + pk3 + " - it is open in another program (is the game running?)"
                                                    : "cannot replace " + pk3);
    }
    return true;
}

bool pk3EintragErsetzen(const std::string& pk3, const std::string& eintrag, const std::string& inhalt, std::string* fehler,
                        SchreibFehler* art) {
    return pk3EintraegeErsetzen(pk3, {{eintrag, inhalt}}, fehler, art);
}

}  // namespace bhed
