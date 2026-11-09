// q3map2shader.cpp - REST8: welcher Eintrag eines Shadernamens beim Kompilieren gilt (siehe bhed/karte/q3map2shader.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, third_party/q3map2/tools/quake3): common/vfs.cpp, common/scriplib.cpp,
// q3map2/path_init.cpp und q3map2/shaders.cpp (LoadShaderInfo, ParseShaderFile, Parse1DMatrixAppend).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant. GtkRadiant is free software; you can redistribute it and/or modify it under the terms of
//   the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
//   option) any later version.
#include "bhed/karte/q3map2shader.h"

#include "bhed/pk3.h"
#include "bhed/shaderscript.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace bhed::karte {

namespace {

namespace fs = std::filesystem;

char gross(char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
char klein(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

std::string kleinText(std::string s) {
    for (char& c : s) { c = klein(c); }
    return s;
}

// striEqual (qstringops.h)
bool gleichOhneFall(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (klein(a[i]) != klein(b[i])) { return false; }
    }
    return true;
}

// DirectoryCleaned (libs/os/path.h:255-283): '\' als '/', ein '/' am Ende
std::string ordnerBereinigt(const std::string& p) {
    std::string s = p;
    for (char& c : s) {
        if (c == '\\') { c = '/'; }
    }
    if (!s.empty() && s.back() != '/') { s += '/'; }
    return s;
}

// path_get_extension + extension_equal (libs/os/path.h:166-191): die Endung hinter dem letzten Punkt nach dem letzten
// Trenner, ohne Gross/klein
bool endungIst(std::string_view pfad, std::string_view endung) {
    std::size_t i = pfad.size();
    while (i > 0) {
        --i;
        if (pfad[i] == '/' || pfad[i] == '\\') { break; }
        if (pfad[i] == '.') { return gleichOhneFall(pfad.substr(i + 1), endung); }
    }
    return endung.empty();
}

// PathExtensionless (qpathops.h): ohne die Endung hinter dem letzten Punkt nach dem letzten Trenner
std::string ohneEndung(const std::string& p) {
    std::size_t i = p.size();
    while (i > 0) {
        --i;
        if (p[i] == '/' || p[i] == '\\') { break; }
        if (p[i] == '.') { return p.substr(0, i); }
    }
    return p;
}

// string_compare_nocase_upper (libs/string/string.h:146-159)
bool vorOhneFallGross(const std::string& a, const std::string& b) {
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const char x = gross(a[i]);
        const char y = gross(b[i]);
        if (x != y) { return static_cast<int>(static_cast<unsigned char>(x)) < static_cast<int>(static_cast<unsigned char>(y)); }
    }
    return a.size() < b.size();
}

bool istDatei(const std::string& pfad) {
    std::error_code ec;
    return fs::is_regular_file(utf8Pfad(pfad), ec);
}

bool dateiLesen(const std::string& pfad, std::string& aus) {
    std::ifstream f(utf8Pfad(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    aus = s.str();
    return true;
}

// Die Namen eines Ordners in der Folge von FindFirstFile/FindNextFile (g_dir_read_name, src/q3map2/kompat/glib.h)
std::vector<std::string> ordnerNamen(const std::string& ordner) {
    std::vector<std::string> v;
    std::error_code ec;
    for (fs::directory_iterator it(utf8Pfad(ordner), ec), ende; !ec && it != ende; it.increment(ec)) {
        const std::u8string u = it->path().filename().u8string();
        v.emplace_back(u.begin(), u.end());
    }
    return v;
}

// --- vfs.cpp ---------------------------------------------------------------------------------------------------------

struct PakDatei {
    std::string name;   // klein, mit '/'
    std::shared_ptr<const Pk3> pak;
    std::size_t eintrag = 0;
};

class Vfs {
public:
    explicit Vfs(const std::vector<std::string>& verzeichnisse) {
        std::vector<PakDatei> folge;   // in der Folge des Einfuegens; g_pakFiles ist sie umgekehrt (emplace_front)
        for (const std::string& v : verzeichnisse) {   // vfsInitDirectory
            const std::string pfad = ordnerBereinigt(v);
            m_ordner.push_back(pfad);
            std::vector<std::string> paks;
            for (const std::string& n : ordnerNamen(pfad)) {
                if (endungIst(n, "pk3")) {
                    paks.push_back(pfad + n);
                } else if (endungIst(n, "pk3dir")) {
                    m_ordner.push_back(pfad + n + "/");
                }
            }
            std::sort(paks.begin(), paks.end(), vorOhneFallGross);   // "sort paks in ascending order"
            for (const std::string& p : paks) {   // vfsInitPakFile
                auto pak = std::make_shared<Pk3>();
                if (!readPk3Directory(p, *pak)) { continue; }
                for (std::size_t i = 0; i < pak->entries.size(); ++i) {
                    if (pak->entries[i].name.size() >= 256) { continue; }   // file_info.size_filename < std::size( filename_inzip )
                    folge.push_back({kleinText(pak->entries[i].name), pak, i});   // FixDOSName, strLower
                }
            }
        }
        m_pakDateien.assign(folge.rbegin(), folge.rend());
    }

    // vfsListShaderFiles (vfs.cpp:194-228)
    [[nodiscard]] std::vector<std::string> shaderDateien(const std::string& shaderPfad) const {
        std::vector<std::string> liste;
        const auto dazu = [&liste](const std::string& name) {
            for (const std::string& s : liste) {
                if (gleichOhneFall(s, name)) { return; }
            }
            liste.push_back(name);
        };
        for (const std::string& o : m_ordner) {
            for (const std::string& n : ordnerNamen(o + shaderPfad + "/")) {
                if (endungIst(n, "shader")) { dazu(n); }
            }
        }
        for (const PakDatei& d : m_pakDateien) {
            if (!endungIst(d.name, "shader")) { continue; }
            // strniEqual( name, shaderPath, path_get_last_separator( name ) - name ): genau <shaderPath>/x.shader
            const std::size_t strich = d.name.find_last_of('/');
            const std::string vorne = d.name.substr(0, strich == std::string::npos ? d.name.size() : strich);
            if (gleichOhneFall(vorne, shaderPfad)) { dazu(d.name.substr(strich + 1)); }
        }
        return liste;
    }

    // vfsGetFileCount (vfs.cpp:237-258)
    [[nodiscard]] int anzahl(const std::string& name) const {
        int n = 0;
        for (const std::string& o : m_ordner) {
            if (istDatei(o + name)) { ++n; }
        }
        const std::string k = kleinText(name);
        for (const PakDatei& d : m_pakDateien) {
            if (d.name == k) { ++n; }
        }
        return n;
    }

    // vfsLoadFile (vfs.cpp:261-330): erst die losen Dateien, dann g_pakFiles; index zaehlt die Treffer
    [[nodiscard]] bool laden(const std::string& name, int index, std::string& aus, std::string& quelle) const {
        for (const std::string& o : m_ordner) {
            if (istDatei(o + name) && index-- == 0) {
                quelle = o;
                return dateiLesen(o + name, aus);
            }
        }
        const std::string k = kleinText(name);
        for (const PakDatei& d : m_pakDateien) {
            if (d.name == k && index-- == 0) {
                quelle = d.pak->path;
                aus.clear();
                (void)readPk3File(*d.pak, d.pak->entries[d.eintrag], aus);
                return true;
            }
        }
        return false;
    }

private:
    std::vector<std::string> m_ordner;     // g_strDirs
    std::vector<PakDatei> m_pakDateien;    // g_pakFiles
};

// --- scriplib.cpp ----------------------------------------------------------------------------------------------------

struct Abbruch {
    std::string grund;
};

constexpr std::size_t kMaxToken = 1024;   // MAXTOKEN (scriplib.h:26)

// *script.it <= 32 mit "char" mit Vorzeichen (MSVC, clang-cl): auch jedes Byte ab 0x80
bool leer(char c) { return static_cast<signed char>(c) <= 32; }

class Zerleger {
public:
    using Lader = std::function<bool(const std::string&, std::string&)>;
    explicit Zerleger(Lader lader) : m_lader(std::move(lader)) {}

    // LoadScriptFile: der Stapel nur mit dieser Datei
    void beginnen(std::string text) {
        m_stapel.clear();
        m_bereit = false;
        schieben(std::move(text));
    }

    // GetToken (scriplib.cpp:162-286)
    bool wortHolen(bool zeilen) {
        if (m_stapel.empty()) { return false; }
        if (m_bereit) {
            m_bereit = false;
            return true;
        }
        if (m_stapel.back().it >= text().size()) { return skriptEnde(zeilen); }
        for (;;) {
            Skript& s = m_stapel.back();
            const std::string& t = *s.daten;
            const std::size_t n = t.size();
            while (s.it < n && leer(t[s.it])) {
                if (t[s.it++] == '\n') {
                    if (!zeilen) { unvollstaendig(); }
                    m_zeile = ++s.zeile;
                }
            }
            if (s.it >= n) { return skriptEnde(zeilen); }
            if (t[s.it] == ';' || t[s.it] == '#' || (t[s.it] == '/' && zeichen(t, s.it + 1) == '/')) {   // ; # // comments
                if (!zeilen) { unvollstaendig(); }
                for (;;) {
                    const char c = t[s.it++];
                    if (c == '\n') { break; }
                    if (s.it >= n) { return skriptEnde(zeilen); }
                }
                m_zeile = ++s.zeile;
                continue;
            }
            if (t[s.it] == '/' && zeichen(t, s.it + 1) == '*') {   // /* */ comments
                s.it += 2;
                while (zeichen(t, s.it) != '*' || zeichen(t, s.it + 1) != '/') {
                    if (zeichen(t, s.it) == '\n') {
                        if (!zeilen) { unvollstaendig(); }
                        m_zeile = ++s.zeile;
                    }
                    ++s.it;
                    if (s.it >= n) { return skriptEnde(zeilen); }
                }
                s.it += 2;
                continue;
            }
            break;
        }
        Skript& s = m_stapel.back();
        const std::string& t = *s.daten;
        const std::size_t n = t.size();
        m_wort.clear();
        m_wortSkript = s.nummer;
        m_wortAnfang = s.it;
        if (t[s.it] == '"') {   // quoted token
            ++s.it;
            while (s.it != n && t[s.it] != '"') {
                m_wort += t[s.it++];
                if (s.it == n) { break; }
                if (m_wort.size() == kMaxToken) { throw Abbruch{"Token too large on line " + std::to_string(m_zeile)}; }
            }
            if (s.it != n) { ++s.it; }
        } else {   // regular token
            while (!leer(t[s.it]) && t[s.it] != ';') {
                m_wort += t[s.it++];
                if (s.it == n) { break; }
                if (m_wort.size() == kMaxToken) { throw Abbruch{"Token too large on line " + std::to_string(m_zeile)}; }
            }
        }
        m_wortEnde = s.it;
        m_wort.resize(std::min(m_wort.size(), m_wort.find('\0')));   // token ist ein C-Text
        if (m_wort == "$include") {
            (void)wortHolen(false);
            std::string inhalt;
            if (m_lader && m_lader(m_wort, inhalt)) { schieben(std::move(inhalt)); }   // AddScriptToStack
            return wortHolen(zeilen);
        }
        return true;
    }

    // UnGetToken
    void zurueck() { m_bereit = true; }

    // TokenAvailable (scriplib.cpp:296-314)
    bool aufDerZeile() {
        const int alt = m_zeile;
        if (!wortHolen(true)) { return false; }
        zurueck();
        return alt == m_zeile;
    }

    [[nodiscard]] const std::string& wort() const { return m_wort; }
    [[nodiscard]] std::size_t wortSkript() const { return m_wortSkript; }
    [[nodiscard]] std::size_t wortAnfang() const { return m_wortAnfang; }
    [[nodiscard]] std::size_t wortEnde() const { return m_wortEnde; }
    [[nodiscard]] const std::string& skriptText(std::size_t nummer) const { return *m_texte[nummer]; }

private:
    struct Skript {
        std::shared_ptr<const std::string> daten;
        std::size_t it = 0;
        int zeile = 1;
        std::size_t nummer = 0;
    };

    static char zeichen(const std::string& t, std::size_t i) { return i < t.size() ? t[i] : '\0'; }   // MemBuffer endet mit '\0'
    [[nodiscard]] const std::string& text() const { return *m_stapel.back().daten; }

    void schieben(std::string inhalt) {
        Skript s;
        s.daten = std::make_shared<const std::string>(std::move(inhalt));
        s.nummer = m_texte.size();
        m_texte.push_back(s.daten);
        m_stapel.push_back(std::move(s));
        m_zeile = 1;
    }

    [[noreturn]] void unvollstaendig() const { throw Abbruch{"Line " + std::to_string(m_zeile) + " is incomplete"}; }

    // EndOfScript (scriplib.cpp:140-157)
    bool skriptEnde(bool zeilen) {
        if (!zeilen) { unvollstaendig(); }
        m_stapel.pop_back();
        if (m_stapel.empty()) { return false; }
        m_zeile = m_stapel.back().zeile;
        return wortHolen(zeilen);
    }

    Lader m_lader;
    std::vector<Skript> m_stapel;
    std::vector<std::shared_ptr<const std::string>> m_texte;   // jede gelesene Datei, fuer die Texte der Eintraege
    bool m_bereit = false;
    int m_zeile = 0;   // scriptline
    std::string m_wort;
    std::size_t m_wortSkript = 0;
    std::size_t m_wortAnfang = 0;
    std::size_t m_wortEnde = 0;
};

// --- shaders.cpp -----------------------------------------------------------------------------------------------------

// Parse1DMatrixAppend (shaders.cpp:799-815)
void matrix(Zerleger& z, int x) {
    if (!z.wortHolen(true) || z.wort() != "(") { throw Abbruch{"Parse1DMatrixAppend(): ( not found"}; }
    for (int i = 0; i < x; ++i) {
        if (!z.wortHolen(false)) { throw Abbruch{"Parse1DMatrixAppend(): Number not found"}; }
    }
    if (!z.wortHolen(true) || z.wort() != ")") { throw Abbruch{"Parse1DMatrixAppend(): ) not found"}; }
}

bool ist(const Zerleger& z, std::string_view w) { return gleichOhneFall(z.wort(), w); }

struct Stelle {
    std::size_t skript = 0;
    std::size_t anfang = 0;
};

// ParseShaderFile (shaders.cpp:823-1735): nur, was entscheidet, wo ein Eintrag endet
void dateiParsen(Zerleger& z, const std::string& datei, const std::string& quelle, Q3map2Shader& aus) {
    while (z.wortHolen(true)) {
        std::string name = z.wort();
        if (name.size() >= 6 && gleichOhneFall(std::string_view(name).substr(name.size() - 6), ":q3map")) { name.resize(name.size() - 6); }
        if (!(z.wortHolen(true) && z.wort() == "{")) {
            throw Abbruch{"ParseShaderFile(): " + datei + ": { not found! Found instead: " + z.wort() + " Last known shader: " + name};
        }
        const Stelle auf{z.wortSkript(), z.wortAnfang()};
        std::string editorBild;   // si.editorImagePath
        std::string lichtBild;    // si.lightImagePath
        std::string implizit;     // si.implicitImagePath
        while (z.wortHolen(true) && z.wort() != "}") {
            if (z.wort() == "{") {   // shader stages (passes)
                while (z.wortHolen(true) && z.wort() != "}") {
                    if (!editorBild.empty() || !lichtBild.empty() || !implizit.empty()) { continue; }
                    if (ist(z, "map") || ist(z, "clampMap") || ist(z, "animMap") || ist(z, "clampAnimMap") || ist(z, "mapComp") || ist(z, "mapNoComp")) {
                        if (ist(z, "animMap") || ist(z, "clampAnimMap")) { (void)z.wortHolen(false); }
                        (void)z.wortHolen(false);
                        if (z.wort().empty() || (z.wort()[0] != '*' && z.wort()[0] != '$')) { lichtBild = ohneEndung(z.wort()); }
                    }
                }
            } else if (ist(z, "implicitMap") || ist(z, "implicitMask") || ist(z, "implicitBlend")) {
                (void)z.wortHolen(false);
                implizit = z.wort() == "-" ? name : ohneEndung(z.wort());
            } else if (ist(z, "qer_editorImage")) {
                (void)z.wortHolen(false);
                editorBild = ohneEndung(z.wort());
            } else if (ist(z, "q3map_lightImage")) {
                (void)z.wortHolen(false);
                lichtBild = ohneEndung(z.wort());
            } else if (ist(z, "skyParms")) {
                (void)z.wortHolen(false);
                if (z.wort() != "-" && !ist(z, "full") && lichtBild.empty()) { lichtBild = z.wort() + "_up"; }
                (void)z.wortHolen(false);
                (void)z.wortHolen(false);
            } else if (ist(z, "q3map_tcGen")) {
                (void)z.wortHolen(false);
                if (ist(z, "vector") || ist(z, "ivector")) {
                    matrix(z, 3);
                    matrix(z, 3);
                }
            } else if (ist(z, "q3map_colorGen") || ist(z, "q3map_colorMod") || ist(z, "q3map_rgbGen") || ist(z, "q3map_rgbMod") || ist(z, "q3map_alphaGen") ||
                       ist(z, "q3map_alphaMod")) {
                const bool alpha = ist(z, "q3map_alphaGen") || ist(z, "q3map_alphaMod");
                (void)z.wortHolen(false);
                if (alpha && (ist(z, "set") || ist(z, "const"))) {
                    (void)z.wortHolen(false);
                } else if (ist(z, "set") || ist(z, "const")) {
                    matrix(z, 3);
                } else if (alpha && ist(z, "scale")) {
                    (void)z.wortHolen(false);
                } else if (ist(z, "scale") || ist(z, "dotProduct") || ist(z, "dotProduct2")) {
                    matrix(z, 3);
                } else if (ist(z, "dotProductScale") || ist(z, "dotProduct2scale")) {
                    matrix(z, 5);
                }
            } else if (ist(z, "q3map_fogDir")) {
                matrix(z, 3);
            }
            while (z.aufDerZeile()) { (void)z.wortHolen(false); }   // "ignore all other tokens on the line"
        }
        const std::string k = kleinText(name);
        if (aus.eintraege.count(k) != 0) {   // AllocShaderInfo legt ihn an, find_first findet den ersten
            ++aus.verdeckt;
            continue;
        }
        Q3map2ShaderEintrag e;
        e.datei = datei;
        e.quelle = quelle;
        const std::string& t = z.skriptText(auf.skript);
        const std::size_t ende = z.wortSkript() == auf.skript ? z.wortEnde() : t.size();   // ein Block ueber ein $include hinweg endet dort
        e.text = shaderEintragText(k, std::string_view(t).substr(auf.anfang, ende - auf.anfang));
        aus.eintraege.emplace(k, std::move(e));
    }
}

}  // namespace

std::vector<std::string> q3map2Verzeichnisse(const std::string& basisPfad, const std::string& fsGame, const std::string& basisSpiel,
                                             const std::vector<std::string>& pakPfade) {
    std::vector<std::string> spiele;   // gamePaths: -fs_game, dann der Standard (insert_unique, ohne Gross/klein)
    const auto einmal = [](std::vector<std::string>& v, const std::string& s) {
        for (const std::string& x : v) {
            if (gleichOhneFall(x, s)) { return; }
        }
        v.push_back(s);
    };
    if (!fsGame.empty()) { einmal(spiele, ordnerBereinigt(fsGame)); }
    if (!basisSpiel.empty()) { einmal(spiele, ordnerBereinigt(basisSpiel)); }
    std::vector<std::string> v;
    if (!basisPfad.empty()) {
        for (const std::string& s : spiele) { v.push_back(ordnerBereinigt(basisPfad) + s); }
    }
    std::vector<std::string> paks;
    for (const std::string& p : pakPfade) {
        if (!p.empty()) { einmal(paks, ordnerBereinigt(p)); }
    }
    v.insert(v.end(), paks.begin(), paks.end());
    return v;
}

namespace {
std::vector<std::string> woerterOhneKommentar(const std::string& t) {
    std::vector<std::string> v;
    std::size_t i = 0;
    const std::size_t n = t.size();
    while (i < n) {
        const char c = t[i];
        if (c == '/' && i + 1 < n && t[i + 1] == '/') {
            while (i < n && t[i] != '\n') { ++i; }
        } else if (c == '/' && i + 1 < n && t[i + 1] == '*') {
            const std::size_t e = t.find("*/", i + 2);
            i = e == std::string::npos ? n : e + 2;
        } else if (static_cast<unsigned char>(c) <= ' ') {
            ++i;
        } else {
            std::string w;
            while (i < n && static_cast<unsigned char>(t[i]) > ' ' && !(t[i] == '/' && i + 1 < n && (t[i + 1] == '/' || t[i + 1] == '*'))) { w += klein(t[i++]); }
            v.push_back(std::move(w));
        }
    }
    return v;
}
}  // namespace

bool shaderEintraegeGleich(const std::string& a, const std::string& b) { return woerterOhneKommentar(a) == woerterOhneKommentar(b); }

std::string pfadBlatt(const std::string& pfad) {
    std::string p = pfad;
    for (char& c : p) {
        if (c == '\\') { c = '/'; }
    }
    const bool ordner = !p.empty() && p.back() == '/';
    if (ordner) { p.pop_back(); }
    const std::size_t s = p.find_last_of('/');
    return (s == std::string::npos ? p : p.substr(s + 1)) + (ordner ? "/" : "");
}

Q3map2Shader q3map2ShaderLesen(const std::vector<std::string>& verzeichnisse, const std::string& shaderPfad) {
    Q3map2Shader aus;
    const Vfs vfs(verzeichnisse);
    Zerleger z([&vfs](const std::string& name, std::string& inhalt) {
        std::string quelle;
        return vfs.laden(name, 0, inhalt, quelle);
    });
    try {
        // shaderlist.txt aus allen Verzeichnissen und Archiven
        const std::string liste = shaderPfad + "/shaderlist.txt";
        const int n = vfs.anzahl(liste);
        for (int i = 0; i < n; ++i) {
            std::string text;
            std::string quelle;
            if (!vfs.laden(liste, i, text, quelle)) { continue; }
            z.beginnen(std::move(text));
            while (z.wortHolen(true)) {
                std::string datei = z.wort();
                if (!endungIst(datei, "shader")) { datei += ".shader"; }   // strcatQ( token, ".shader" )
                bool schon = false;
                for (const std::string& d : aus.dateien) { schon = schon || gleichOhneFall(d, datei); }
                if (!schon) { aus.dateien.push_back(datei); }
            }
        }
        aus.shaderliste = !aus.dateien.empty();
        if (aus.dateien.empty()) { aus.dateien = vfs.shaderDateien(shaderPfad); }   // "No shaderlist.txt found: loading all shaders"
        for (const std::string& d : aus.dateien) {
            const std::string pfad = shaderPfad + "/" + d;
            std::string text;
            std::string quelle;
            if (!vfs.laden(pfad, 0, text, quelle)) { continue; }   // "Script file ... was not found": GetToken findet nichts
            z.beginnen(std::move(text));
            dateiParsen(z, pfad, quelle, aus);
        }
    } catch (const Abbruch& a) {
        aus.abbruch = a.grund;
    }
    return aus;
}

}  // namespace bhed::karte
