// texturen.cpp - Karten-Editor: Dateisystem des Spiels und Textur-Inventar.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): plugins/vfspk3/vfs.cpp,
// radiant/qe3.cpp (QE_InitVFS), plugins/shaders/shaders.cpp,
// radiant/texwindow.cpp, radiant/image.cpp - Einzelheiten in texturen.h.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/texturen.h"

#include "bhed/image.h"
#include "bhed/karte/kartenformat.h"
#include "tokenizer.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace bhed::karte {

using namespace detail;

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool gleichOhneFall(const std::string& a, const std::string& b) { return klein(a) == klein(b); }   // path_equal

bool praefixOhneFall(const std::string& s, const std::string& p) {   // shader_equal_prefix / path_equal_n
    if (p.size() > s.size()) { return false; }
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(p[i]))) { return false; }
    }
    return true;
}

int vergleichOhneFall(const std::string& a, const std::string& b) {   // string_compare_nocase (beide klein)
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return ca < cb ? -1 : 1; }
    }
    return a.size() < b.size() ? -1 : (a.size() > b.size() ? 1 : 0);
}

int vergleichOhneFallGross(const std::string& a, const std::string& b) {   // string_compare_nocase_upper
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::toupper(static_cast<unsigned char>(a[i]));
        const int cb = std::toupper(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return ca < cb ? -1 : 1; }
    }
    return a.size() < b.size() ? -1 : (a.size() > b.size() ? 1 : 0);
}

std::filesystem::path pfadAus(const std::string& p) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size()));
}

std::string endung(const std::string& name) {   // path_get_extension
    const std::size_t s = name.find_last_of('/');
    const std::size_t p = name.find_last_of('.');
    if (p == std::string::npos || (s != std::string::npos && p < s)) { return {}; }
    return name.substr(p + 1);
}

std::string ohneEndung(const std::string& name) {   // PathExtensionless
    const std::size_t s = name.find_last_of('/');
    const std::size_t p = name.find_last_of('.');
    if (p == std::string::npos || (s != std::string::npos && p < s)) { return name; }
    return name.substr(0, p);
}

std::string bereinigt(std::string p) {   // PathCleaned
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

// ShaderTemplate::parseQuake3 (shaders.cpp:1157-1311)
bool quake3Parsen(Tokenizer& t, ShaderEintrag& s) {
    s.texturName = s.name;
    t.nextLine();
    int tiefe = 0;
    for (;;) {
        t.nextLine();
        const char* token = t.getToken();
        if (token == nullptr) { return false; }
        if (std::strcmp(token, "{") == 0) {
            ++tiefe;
            continue;
        }
        if (std::strcmp(token, "}") == 0) {
            --tiefe;
            if (tiefe < 0) { return false; }
            if (tiefe == 0) { break; }
            continue;
        }
        if (tiefe != 1) { continue; }
        const std::string k = klein(token);
        if (k == "qer_nocarve") {
            s.flags |= QER_NOCARVE;
        } else if (k == "qer_trans") {
            if (!floatHolen(t, s.trans)) { return false; }
            s.flags |= QER_TRANS;
        } else if (k == "qer_editorimage") {
            const char* n = t.getToken();
            if (n == nullptr) {
                unerwartet(t, n, "#texture-name");
                return false;
            }
            s.texturName = bereinigt(ohneEndung(n));   // parseTextureName
        } else if (k == "qer_alphafunc") {
            const char* f = t.getToken();
            if (f == nullptr) {
                unerwartet(t, f, "#alphafunc");
                return false;
            }
            const std::string a = klein(f);
            s.alphaFunc = a == "equal" ? 1 : a == "greater" ? 3 : a == "less" ? 2 : a == "gequal" ? 5 : a == "lequal" ? 4 : 0;
            s.flags |= QER_ALPHATEST;
            if (!floatHolen(t, s.alphaRef)) { return false; }
        } else if (k == "q3map_lightmapsamplesize") {
            // Stufe 8b Punkt 5 (Ansicht Lightmap-Dichte): q3map2 shaders.cpp
            // "q3map_lightmapSampleSize N" - NRC liest es nicht; ohne Zahl
            // dahinter bleibt der Strom unveraendert (ungetToken).
            const char* n = t.getToken();
            if (n != nullptr) {
                char* ende = nullptr;
                const long v = std::strtol(n, &ende, 10);
                if (ende != nullptr && ende != n && *ende == '\0') {
                    s.lightmapAbtastung = static_cast<int>(v);
                } else {
                    t.ungetToken();
                }
            }
        } else if (k == "skyparms") {
            const char* h = t.getToken();
            if (h == nullptr) {
                unerwartet(t, h, "#skyparms");
                return false;
            }
            if (std::strcmp(h, "-") != 0) { s.himmel = h; }
            s.flags |= QER_SKY;
        } else if (k == "cull") {
            const char* c = t.getToken();
            if (c == nullptr) {
                unerwartet(t, c, "#cull");
                return false;
            }
            const std::string a = klein(c);
            s.cull = (a == "none" || a == "twosided" || a == "disable") ? 0 : 1;
            s.flags |= QER_CULL;
        } else if (k == "surfaceparm") {
            const char* p = t.getToken();
            if (p == nullptr) {
                unerwartet(t, p, "#surfaceparm");
                return false;
            }
            const std::string a = klein(p);
            s.surfaceparms.push_back(a);   // Stufe 8b Punkt 3
            if (a == "fog") {
                s.flags |= QER_FOG;
                s.flags |= QER_TRANS;
                if (s.trans == 1) { s.trans = 0.35f; }   // nicht schon durch qer_trans gesetzt
            } else if (a == "nodraw") {
                s.flags |= QER_NODRAW;
            } else if (a == "nonsolid") {
                s.flags |= QER_NONSOLID;
            } else if (a == "water" || a == "lava" || a == "slime") {
                s.flags |= QER_LIQUID;
            } else if (a == "areaportal") {
                s.flags |= QER_AREAPORTAL;
            } else if (a == "playerclip") {
                s.flags |= QER_CLIP;
            } else if (a == "botclip") {
                s.flags |= QER_BOTCLIP;
            }
        }
    }
    return true;
}

}  // namespace

// shader_valid (shaderlib.h:46-55)
bool shaderGueltig(const std::string& s) {
    for (const char c : s) {
        if (static_cast<unsigned char>(c) >= 0x80 || c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\\') {
            return false;
        }
    }
    return true;
}

bool texturNameIgnorieren(const std::string& name) {   // texture_name_ignore
    const std::string t = klein(name);
    static const char* const enden[] = {".specular", ".glow",  ".bump",  ".diffuse", ".blend", ".alpha",   "_norm",
                                        "_bump",     "_glow",  "_gloss", "_pants",   "_shirt", "_reflect", "_alpha"};
    for (const char* e : enden) {
        const std::size_t n = std::strlen(e);
        if (t.size() >= n && t.compare(t.size() - n, n, e) == 0) { return true; }
    }
    return false;
}

// ===================== Dateisystem (vfs.cpp) ================================

// InitDirectory (vfs.cpp:197-286): erst der Ordner selbst, dann die .pk3 -
// Vorrangpakete (sp_ im Modus sp, mp_ im Modus mp) zuerst, je absteigend
// sortiert (PakLess: string_compare_nocase_upper > 0); die des anderen Modus
// werden uebergangen.
void Dateisystem::ordnerHinzufuegen(const std::string& ordner, const std::string& spielmodus) {
    std::string pfad = bereinigt(ordner);   // DirectoryCleaned: '/' am Ende
    if (!pfad.empty() && pfad.back() != '/') { pfad += '/'; }
    Archiv dir;
    dir.name = pfad;
    m_archive.push_back(std::move(dir));
    std::error_code ecDir;
    if (!std::filesystem::is_directory(pfadAus(pfad), ecDir)) {   // g_dir_open schlug fehl
        fehler(protokoll, "vfs directory not found: " + pfad + "\n");
        return;
    }
    ausgabe(protokoll, "vfs directory: " + pfad + "\n");

    std::string ignorieren, vorrang;
    if (spielmodus == "sp") {
        ignorieren = "mp_";
        vorrang = "sp_";
    } else if (spielmodus == "mp") {
        ignorieren = "sp_";
        vorrang = "mp_";
    }
    auto absteigend = [](const std::string& a, const std::string& b) { return vergleichOhneFallGross(a, b) > 0; };
    std::set<std::string, decltype(absteigend)> paks(absteigend), vorrangPaks(absteigend);
    std::error_code ec;
    for (const auto& d : std::filesystem::directory_iterator(pfadAus(pfad), ec)) {
        if (!d.is_regular_file()) { continue; }
        const std::string name = d.path().filename().string();
        if (klein(endung(name)) != "pk3") { continue; }
        if (!ignorieren.empty() && name.compare(0, ignorieren.size(), ignorieren) == 0) { continue; }
        if (!vorrang.empty() && name.compare(0, vorrang.size(), vorrang) == 0) {
            vorrangPaks.insert(name);
            continue;
        }
        paks.insert(name);
    }
    for (const auto* menge : {&vorrangPaks, &paks}) {
        for (const auto& name : *menge) {
            Archiv a;
            a.name = pfad + name;
            a.istPak = true;
            ausgabe(protokoll, "  pak file: " + a.name + "\n");   // InitPakFile
            if (readPk3Directory(a.name, a.pak)) {
                m_archive.push_back(std::move(a));
            }
        }
    }
}

bool Dateisystem::lesenAus(const Archiv& a, const std::string& pfad, std::string& inhalt) const {
    if (a.istPak) {
        const Pk3Entry* e = a.pak.find(pfad);
        return e && readPk3File(a.pak, *e, inhalt);
    }
    std::ifstream f(pfadAus(a.name + pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    inhalt = s.str();
    return true;
}

bool Dateisystem::dateiLesen(const std::string& pfad, std::string& inhalt) const {   // OpenFile
    for (const auto& a : m_archive) {
        if (lesenAus(a, pfad, inhalt)) { return true; }
    }
    return false;
}

std::string Dateisystem::finden(const std::string& pfad, bool* istOrdner) const {   // findFile (vfs.cpp)
    for (const auto& a : m_archive) {
        bool da = false;
        if (a.istPak) {
            da = a.pak.find(pfad) != nullptr;
        } else {
            std::error_code ec;
            da = std::filesystem::is_regular_file(pfadAus(a.name + pfad), ec);
        }
        if (da) {
            if (istOrdner != nullptr) { *istOrdner = !a.istPak; }
            return a.name;
        }
    }
    return {};
}

bool Dateisystem::textLesen(const std::string& pfad, std::string& inhalt) const {   // OpenTextFile
    std::string roh;
    if (!dateiLesen(pfad, roh)) { return false; }
    inhalt = textmodusLesen(roh);
    return true;
}

bool Dateisystem::textAusArchiv(const std::string& archiv, const std::string& pfad, std::string& inhalt) const {
    for (const auto& a : m_archive) {
        if (a.name == archiv) {
            std::string roh;
            if (!lesenAus(a, pfad, roh)) { return false; }
            inhalt = textmodusLesen(roh);
            return true;
        }
    }
    return false;
}

std::vector<std::string> Dateisystem::archive(bool umgekehrt) const {   // forEachArchive
    std::vector<std::string> v;
    for (const auto& a : m_archive) { v.push_back(a.name); }
    if (umgekehrt) { std::reverse(v.begin(), v.end()); }
    return v;
}

// Archive::forEachFile (GenericFileSystem): Pfade unter basis bis "tiefe"
// Stufen; Verzeichnisse mit '/' am Ende.
void Dateisystem::pfadeSammeln(const Archiv& a, const std::string& basis, std::size_t tiefe, bool verzeichnisse,
                               std::vector<std::string>& out) const {
    auto stufen = [](const std::string& rest) {
        return static_cast<std::size_t>(std::count(rest.begin(), rest.end(), '/'));
    };
    if (a.istPak) {
        std::set<std::string> gesehen;
        for (const auto& e : a.pak.entries) {
            const std::string n = bereinigt(e.name);
            if (!praefixOhneFall(n, basis) || n.size() == basis.size()) { continue; }
            const std::string rest = n.substr(basis.size());
            if (verzeichnisse) {
                // jede Verzeichnisstufe bis "tiefe" (implizite Verzeichnisse)
                std::size_t pos = 0;
                for (std::size_t s = 0; s < tiefe; ++s) {
                    const std::size_t q = rest.find('/', pos);
                    if (q == std::string::npos) { break; }
                    const std::string d = basis + rest.substr(0, q + 1);
                    if (gesehen.insert(klein(d)).second) { out.push_back(d); }
                    pos = q + 1;
                }
            } else if (!rest.empty() && rest.back() != '/' && stufen(rest) < tiefe) {
                out.push_back(n);
            }
        }
        return;
    }
    std::error_code ec;
    const std::filesystem::path wurzel = pfadAus(a.name + basis);
    if (!std::filesystem::is_directory(wurzel, ec)) { return; }
    for (auto it = std::filesystem::recursive_directory_iterator(wurzel, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) { break; }
        if (static_cast<std::size_t>(it.depth()) + 1 > tiefe) {
            it.disable_recursion_pending();
            continue;
        }
        const std::string rel = std::filesystem::relative(it->path(), pfadAus(a.name), ec).generic_string();
        if (it->is_directory()) {
            if (verzeichnisse) { out.push_back(rel + '/'); }
            if (static_cast<std::size_t>(it.depth()) + 1 >= tiefe) { it.disable_recursion_pending(); }
        } else if (!verzeichnisse) {
            out.push_back(rel);
        }
    }
}

// GetListInternal (vfs.cpp:150-174) mit DirectoryListVisitor / FileListVisitor
std::vector<std::string> Dateisystem::verzeichnisse(const std::string& basis, std::size_t tiefe) const {
    std::vector<std::string> treffer;
    for (const auto& a : m_archive) {
        std::vector<std::string> namen;
        pfadeSammeln(a, basis, tiefe, true, namen);
        for (const auto& n : namen) {
            std::string sub = n.substr(basis.size());
            if (!sub.empty() && sub[0] == '/') { sub.erase(0, 1); }
            if (!sub.empty() && sub.back() == '/') { sub.pop_back(); }
            if (std::none_of(treffer.begin(), treffer.end(), [&](const std::string& t) { return gleichOhneFall(t, sub); })) {
                treffer.push_back(sub);
            }
        }
    }
    return treffer;
}

void Dateisystem::dateienImArchiv(const std::string& archiv, const std::string& basis, std::size_t tiefe,
                                  const std::function<void(const std::string&)>& besuch) const {
    for (const auto& a : m_archive) {
        if (a.name != archiv) { continue; }
        std::vector<std::string> namen;
        pfadeSammeln(a, basis, tiefe, false, namen);
        for (const auto& n : namen) { besuch(n); }
        return;
    }
}

std::vector<std::string> Dateisystem::dateien(const std::string& basis, const std::string& endungWunsch, std::size_t tiefe) const {
    std::vector<std::string> treffer;
    for (const auto& a : m_archive) {
        std::vector<std::string> namen;
        pfadeSammeln(a, basis, tiefe, false, namen);
        for (const auto& n : namen) {
            std::string sub = n.substr(basis.size());
            if (!sub.empty() && sub[0] == '/') { sub.erase(0, 1); }
            if (endungWunsch != "*" && !gleichOhneFall(endung(sub), endungWunsch)) { continue; }   // path_extension_is
            if (std::none_of(treffer.begin(), treffer.end(), [&](const std::string& t) { return gleichOhneFall(t, sub); })) {
                treffer.push_back(sub);
            }
        }
    }
    return treffer;
}

// ===================== Texturinventar (shaders.cpp, texwindow.cpp) ==========

bool Texturinventar::OhneFall::operator()(const std::string& a, const std::string& b) const {
    return vergleichOhneFall(a, b) < 0;
}

// Shaders_Load (shaders.cpp:1642-1687)
void Texturinventar::laden(const Dateisystem& fs, const std::string& standardShaderliste) {
    m_fs = &fs;
    m_shaderDateien.clear();
    m_meldungen.clear();
    protokoll.clear();
    // Meldungen des Tokenizers (globalErrorStream) seit der letzten Stelle
    std::size_t gemeldet = 0;
    const auto tokenizerFehler = [this, &gemeldet]() {
        for (; gemeldet < m_meldungen.size(); ++gemeldet) { fehler(protokoll, m_meldungen[gemeldet] + "\n"); }
    };
    m_shader.clear();
    {
        std::lock_guard<std::mutex> l(m_sperre);
        m_groessen.clear();
    }
    const std::string pfad = "shaders/";   // shaderpath (ja.game, jk2.game)

    // ShaderList_addShaderFile: ohne Gross/klein eindeutig
    auto hinzufuegen = [this](const std::string& datei) {
        for (const auto& s : m_shaderDateien) {
            if (gleichOhneFall(s, datei)) {
                m_meldungen.push_back("duplicate entry \"" + s + "\" in shaderlist.txt");
                ausgabe(protokoll, "duplicate entry \"" + s + "\" in shaderlist.txt\n");
                return;
            }
        }
        m_shaderDateien.push_back(datei);
    };
    // BuildShaderList: Tokens der Liste, ".shader" wenn ohne Endung
    auto listeLesen = [&](const std::string& text) {
        Tokenizer t(text, &m_meldungen, false, false);   // NewSimpleTokeniser
        for (const char* token; t.nextLine(), (token = t.getToken()) != nullptr;) {
            std::string datei = token;
            if (!gleichOhneFall(endung(datei), "shader")) { datei += ".shader"; }
            hinzufuegen(datei);
        }
    };
    // ShaderList_addFromArchive fuer jedes Archiv (umgekehrt)
    for (const auto& archiv : fs.archive(true)) {
        std::string text;
        if (fs.textAusArchiv(archiv, pfad + "shaderlist.txt", text)) {
            m_meldungen.push_back("Found shaderlist.txt in " + archiv);
            gemeldet = m_meldungen.size();
            ausgabe(protokoll, "Found shaderlist.txt in " + archiv + "\n");
            listeLesen(text);
            tokenizerFehler();
        }
    }
    if (m_shaderDateien.empty() && !standardShaderliste.empty()) {
        // was NRC aus dem Gamepack in den Spielordner kopiert haette
        std::ifstream f(pfadAus(standardShaderliste), std::ios::binary);
        if (f) {
            std::ostringstream s;
            s << f.rdbuf();
            listeLesen(textmodusLesen(s.str()));
        }
    }
    tokenizerFehler();
    if (m_shaderDateien.empty()) {
        m_meldungen.push_back("No shaderlist.txt found: loading all shaders");
        gemeldet = m_meldungen.size();
        ausgabe(protokoll, "No shaderlist.txt found: loading all shaders\n");
        for (const auto& d : fs.dateien(pfad, "shader", 1)) { hinzufuegen(d); }
    }
    for (const auto& datei : m_shaderDateien) {   // LoadShaderFile
        std::string text;
        if (fs.textLesen(pfad + datei, text)) {
            ausgabe(protokoll, "Parsing shaderfile " + pfad + datei + "\n");
            const std::size_t vorher = m_meldungen.size();
            shaderDateiParsen(text, pfad + datei);
            // Tokenizer-Fehler, dann "Error parsing shader" (beides globalErrorStream)
            for (std::size_t i = vorher; i < m_meldungen.size(); ++i) { fehler(protokoll, m_meldungen[i] + "\n"); }
            gemeldet = m_meldungen.size();
        } else {
            m_meldungen.push_back("Unable to read shaderfile " + pfad + datei);
            gemeldet = m_meldungen.size();
            warnung(protokoll, "Unable to read shaderfile " + pfad + datei + "\n");
        }
    }
}

// ParseShaderFile (shaders.cpp:1330-1409), Quake3-Zweig
void Texturinventar::shaderDateiParsen(const std::string& text, const std::string& datei) {
    Tokenizer t(text, &m_meldungen, true, false);   // NewScriptTokeniser
    t.nextLine();
    for (;;) {
        const char* token = t.getToken();
        if (token == nullptr) { break; }
        if (std::strcmp(token, "table") == 0) {   // Doom3: Tabelle ueberspringen
            if (t.getToken() == nullptr) {
                unerwartet(t, nullptr, "#table-name");
                return;
            }
            if (!tokenErwarten(t, "{")) { return; }
            for (;;) {
                const char* option = t.getToken();
                if (option != nullptr && std::strcmp(option, "{") == 0) {
                    for (;;) {
                        const char* wert = t.getToken();
                        if (wert == nullptr || std::strcmp(wert, "}") == 0) { break; }
                    }
                    if (!tokenErwarten(t, "}")) { return; }
                    break;
                }
                if (option == nullptr) { return; }
            }
            continue;
        }
        if (std::strcmp(token, "material") != 0 && std::strcmp(token, "particle") != 0 && std::strcmp(token, "skin") != 0) {
            t.ungetToken();
        }
        ShaderEintrag s;
        {
            const char* n = t.getToken();   // Tokeniser_parseShaderName
            if (n == nullptr) {
                unerwartet(t, n, "#shader-name");
            } else {
                s.name = bereinigt(ohneEndung(n));
            }
        }
        s.datei = datei;
        if (quake3Parsen(t, s)) {
            m_shader.emplace(s.name, s);   // erste Definition gewinnt
        } else {
            m_meldungen.push_back("Error parsing shader " + s.name);
            return;
        }
    }
}

std::vector<const ShaderEintrag*> Texturinventar::shader() const {
    std::vector<const ShaderEintrag*> v;
    v.reserve(m_shader.size());
    for (const auto& [n, s] : m_shader) {
        (void)n;
        v.push_back(&s);
    }
    return v;
}

const ShaderEintrag* Texturinventar::shaderFinden(const std::string& name) const {
    auto it = m_shader.find(name);
    return it == m_shader.end() ? nullptr : &it->second;
}

// TextureGroups_constructTreeView (texwindow.cpp:1163-1176)
std::vector<std::string> Texturinventar::verzeichnisse(bool nurShaderliste) const {
    auto vergleich = [](const std::string& a, const std::string& b) {   // TextureGroups_compare
        const int c = vergleichOhneFall(a, b);
        if (c != 0) { return c < 0; }
        return a < b;
    };
    std::set<std::string, decltype(vergleich)> gruppen(vergleich);
    if (m_fs && !nurShaderliste) {
        for (const auto& d : m_fs->verzeichnisse("textures/", 1)) { gruppen.insert(d); }   // TextureGroups_addDirectory
    }
    for (const auto& [name, s] : m_shader) {   // TextureGroups_addShader
        (void)s;
        if (!praefixOhneFall(name, "textures/")) { continue; }
        // path_make_relative + path_remove_directory: erste Verzeichnisstufe,
        // wenn nach dem ersten '/' noch etwas folgt
        const std::string textur = name.substr(9);
        const std::size_t schraeg = textur.find('/');
        if (schraeg != std::string::npos && schraeg + 1 < textur.size()) {
            gruppen.insert(textur.substr(0, schraeg));
        }
    }
    return std::vector<std::string>(gruppen.begin(), gruppen.end());
}

// TextureBrowser_ShowDirectory (texwindow.cpp:708-741) + Texture_IsShown
std::vector<ShaderEintrag> Texturinventar::verzeichnis(const std::string& dirRoh) const {
    std::string dir = dirRoh;
    if (!dir.empty() && dir.back() != '/') { dir += '/'; }   // TreeView_onRowActivated haengt '/' an
    std::map<std::string, ShaderEintrag, OhneFall> aktiv;    // g_ActiveShaders
    // TextureCategoryLoadShader: alle Shader mit Praefix textures/<dir>
    for (const auto& [name, s] : m_shader) {
        if (praefixOhneFall(name, "textures/") && praefixOhneFall(name.substr(9), dir)) {
            aktiv.emplace(name, s);
        }
    }
    // LoadTexturesByTypeVisitor: Bilder je Bildmodul (jpg, png, tga)
    if (m_fs) {
        for (const char* typ : {"jpg", "png", "tga"}) {
            for (const auto& datei : m_fs->dateien("textures/" + dir, typ, 1)) {
                const std::string name = "textures/" + dir + ohneEndung(datei);
                if (texturNameIgnorieren(name)) { continue; }
                if (!shaderGueltig(name)) {
                    continue;   // "Skipping invalid texture name"
                }
                if (aktiv.count(name)) { continue; }
                // Try_Shader_ForName: Definition oder Standardshader (CreateDefault)
                if (const ShaderEintrag* s = shaderFinden(name)) {
                    aktiv.emplace(name, *s);
                } else {
                    ShaderEintrag st;
                    st.name = name;
                    st.texturName = name;   // g_enableDefaultShaders
                    aktiv.emplace(name, st);
                }
            }
        }
    }
    std::vector<ShaderEintrag> v;
    for (auto& [n, s] : aktiv) {
        (void)n;
        v.push_back(s);
    }
    return v;
}

// QERApp_LoadImage (image.cpp:36-60): Module alphabetisch, erstes Archiv gewinnt
bool Texturinventar::bildGroesse(const std::string& texturName, std::size_t& w, std::size_t& h) const {
    if (!m_fs) { return false; }
    for (const char* typ : {"jpg", "png", "tga"}) {
        std::string daten;
        if (m_fs->dateiLesen(texturName + '.' + typ, daten)) {
            const image::Image bild = image::decode(reinterpret_cast<const unsigned char*>(daten.data()), daten.size());
            if (!bild.ok || bild.width <= 0 || bild.height <= 0) { return false; }   // Laden scheitert -> notex
            w = static_cast<std::size_t>(bild.width);
            h = static_cast<std::size_t>(bild.height);
            return true;
        }
    }
    return false;
}

std::pair<std::size_t, std::size_t> Texturinventar::groesse(const std::string& shader) const {
    const BildMass m = bildMass(shader);
    return {m.breite, m.hoehe};
}

Texturinventar::BildMass Texturinventar::bildMass(const std::string& shader) const {
    {
        std::lock_guard<std::mutex> l(m_sperre);
        auto it = m_groessen.find(shader);
        if (it != m_groessen.end()) { return it->second; }
    }
    // Shader -> Editorbild; ohne Definition ist der Name selbst das Bild
    const ShaderEintrag* s = shaderFinden(shader);
    const std::string bild = s ? s->texturName : shader;
    BildMass m;   // notex.png / shadernotex.png: 64 x 64
    std::size_t w = 64;
    std::size_t h = 64;
    if (bildGroesse(bild, w, h)) {
        m.breite = w;
        m.hoehe = h;
        m.gefunden = true;
    }
    std::lock_guard<std::mutex> l(m_sperre);
    m_groessen[shader] = m;
    return m;
}

std::array<unsigned char, 256> gammaTabelle(float gamma) {
    std::array<unsigned char, 256> t{};
    if (gamma == 1) {
        for (int i = 0; i < 256; ++i) { t[static_cast<std::size_t>(i)] = static_cast<unsigned char>(i); }
    } else {
        for (int i = 0; i < 256; ++i) {
            const int v = static_cast<int>(255 * std::pow((i + 0.5) / 255.5, gamma) + 0.5);
            t[static_cast<std::size_t>(i)] = static_cast<unsigned char>(std::clamp(v, 0, 255));
        }
    }
    return t;
}

int mipBasis(int mipStufe, int breite, int hoehe) {
    return std::min(mipStufe, static_cast<int>(std::log2(static_cast<float>(std::max(breite, hoehe)))));
}

int texturModusAusListe(int index) {
    static const int modi[6] = {0, 1, 3, 2, 4, 5};   // NEAREST, NEAREST_MIPMAP_NEAREST, LINEAR, NEAREST_MIPMAP_LINEAR, ...
    return index >= 0 && index < 6 ? modi[index] : 5;
}

int texturModusZuListe(int modus) {
    static const int liste[6] = {0, 1, 3, 2, 4, 5};
    return modus >= 0 && modus < 6 ? liste[modus] : 4;   // default: importer( 4 )
}

}  // namespace bhed::karte
