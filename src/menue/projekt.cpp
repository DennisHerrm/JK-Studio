// projekt.cpp - Menue-Editor M5b Punkt 3: Projektdatei *.jksmenu, Sicherung, Dateizuordnung
// (siehe projekt.h).
//
// Eigener Code.
#include "bhed/menue/projekt.h"

#include <cctype>
#include <cstdlib>

namespace bhed::menue {

namespace {

// Ein Wert: ohne Leerzeichen roh, sonst in "" (" und \ mit \ davor)
std::string wert(const std::string& s) {
    if (!s.empty() && s.find_first_of(" \t\"\\") == std::string::npos) { return s; }
    std::string o = "\"";
    for (const char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; }
        o += c;
    }
    return o + "\"";
}

// Woerter einer Zeile (roh oder in "")
std::vector<std::string> woerter(const std::string& z) {
    std::vector<std::string> v;
    std::size_t p = 0;
    while (p < z.size()) {
        while (p < z.size() && (z[p] == ' ' || z[p] == '\t')) { ++p; }
        if (p >= z.size()) { break; }
        std::string w;
        if (z[p] == '"') {
            ++p;
            while (p < z.size() && z[p] != '"') {
                if (z[p] == '\\' && p + 1 < z.size()) { ++p; }
                w += z[p++];
            }
            if (p < z.size()) { ++p; }
        } else {
            while (p < z.size() && z[p] != ' ' && z[p] != '\t') { w += z[p++]; }
        }
        v.push_back(w);
    }
    return v;
}

std::vector<std::string> zeilen(const std::string& t) {
    std::vector<std::string> v;
    std::size_t a = 0;
    while (a <= t.size()) {
        std::size_t e = t.find('\n', a);
        if (e == std::string::npos) { e = t.size(); }
        std::string z = t.substr(a, e - a);
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        v.push_back(z);
        a = e + 1;
    }
    return v;
}

const char* spielName(Spiel s) { return s == Spiel::JK2 ? "jk2" : s == Spiel::MD ? "md" : "ja"; }

// Spielpfad wie in der pk3: / statt \, ohne ./ und ohne fuehrendes /
bool sichererPfad(const std::string& p) {
    if (p.empty() || p[0] == '/' || p[0] == '\\' || p.find(':') != std::string::npos) { return false; }
    std::size_t a = 0;
    for (;;) {
        const std::size_t e = p.find_first_of("/\\", a);
        const std::string teil = p.substr(a, e == std::string::npos ? std::string::npos : e - a);
        if (teil == "..") { return false; }
        if (e == std::string::npos) { break; }
        a = e + 1;
    }
    return true;
}

}  // namespace

std::string projektText(const Projekt& p) {
    std::string s = "// JK Studio menu project - open it with JK Studio (Menu editor)\n";
    s += "jksmenu 1\n";
    s += std::string("game ") + spielName(p.spiel) + "\n";
    s += std::string("dialect ") + (p.dialekt == Dialekt::MP ? "mp" : "sp") + "\n";
    if (!p.profil.empty()) { s += "profile " + wert(p.profil) + "\n"; }
    for (const std::string& m : p.menues) { s += "menu " + wert(m) + "\n"; }
    for (const auto& [datei, menue] : p.dateien) { s += "asset " + wert(menue) + " " + wert(datei) + "\n"; }
    for (const std::string& o : p.offen) { s += "open " + wert(o) + "\n"; }
    s += "active " + std::to_string(p.aktiv) + "\n";
    s += "preview.resolution " + std::to_string(p.aufloesung) + "\n";
    s += std::string("preview.letterbox ") + (p.balken ? "1" : "0") + "\n";
    s += "preview.language " + wert(p.sprache) + "\n";
    for (const auto& [n, w] : p.cvars) { s += "cvar " + wert(n) + " " + wert(w) + "\n"; }
    s += "export.pk3 " + wert(p.exportPk3) + "\n";
    s += std::string("export.changesOnly ") + (p.nurAenderungen ? "1" : "0") + "\n";
    return s;
}

bool liesProjekt(const std::string& text, Projekt& out, ProjektFehler& fehler) {
    Projekt p;
    bool kopf = false;
    int nr = 0;
    for (const std::string& z : zeilen(text)) {
        ++nr;
        const std::vector<std::string> w = woerter(z);
        if (w.empty() || w[0].compare(0, 2, "//") == 0) { continue; }
        const std::string& k = w[0];
        const std::string v = w.size() > 1 ? w[1] : std::string();
        if (!kopf) {
            if (k != "jksmenu") {
                fehler = ProjektFehler{ProjektFehler::Art::KeinProjekt, nr, k};
                return false;
            }
            if (std::atoi(v.c_str()) > 1) {
                fehler = ProjektFehler{ProjektFehler::Art::ZuNeu, nr, v};
                return false;
            }
            kopf = true;
            continue;
        }
        if (k == "game") { p.spiel = v == "jk2" ? Spiel::JK2 : v == "md" ? Spiel::MD : Spiel::JA; }
        else if (k == "dialect") { p.dialekt = v == "mp" ? Dialekt::MP : Dialekt::SP; }
        else if (k == "profile") { p.profil = v; }
        else if (k == "menu" || k == "open") {
            if (!sichererPfad(v)) {
                fehler = ProjektFehler{ProjektFehler::Art::PfadAusserhalb, nr, v};
                return false;
            }
            (k == "menu" ? p.menues : p.offen).push_back(v);
        } else if (k == "asset" && w.size() > 2) {
            if (!sichererPfad(v) || !sichererPfad(w[2])) {
                fehler = ProjektFehler{ProjektFehler::Art::PfadAusserhalb, nr, sichererPfad(v) ? w[2] : v};
                return false;
            }
            p.dateien[w[2]] = v;
        } else if (k == "active") { p.aktiv = std::atoi(v.c_str()); }
        else if (k == "preview.resolution") { p.aufloesung = std::atoi(v.c_str()); }
        else if (k == "preview.letterbox") { p.balken = v == "1"; }
        else if (k == "preview.language") { p.sprache = v; }
        else if (k == "cvar" && w.size() > 2) { p.cvars[v] = w[2]; }
        else if (k == "export.pk3") { p.exportPk3 = v; }
        else if (k == "export.changesOnly") { p.nurAenderungen = v == "1"; }
        // Unbekanntes (neuere Fassung) wird ueberlesen
    }
    if (!kopf) {
        fehler = ProjektFehler{ProjektFehler::Art::KeinProjekt, 0, ""};
        return false;
    }
    out = p;
    return true;
}

const std::vector<std::string>& projektUnterordner() {
    static const std::vector<std::string> v = {"ui", "gfx", "video", "shaders", "strings"};
    return v;
}

bool paketPfad(const std::string& rel) {
    if (!sichererPfad(rel)) { return false; }
    std::size_t a = 0;
    for (;;) {   // kein Teil mit . am Anfang (Sicherung, .git ...)
        if (a < rel.size() && rel[a] == '.') { return false; }
        const std::size_t e = rel.find_first_of("/\\", a);
        if (e == std::string::npos) { break; }
        a = e + 1;
    }
    const std::string end = kProjektEndung;
    if (rel.size() >= end.size() && rel.compare(rel.size() - end.size(), end.size(), end) == 0) { return false; }
    // keine pk3: das Spiel liest keine pk3 in einer pk3 (FS_AddGameDirectory, OJ code/qcommon/files.cpp:2730,
    // nimmt nur die *.pk3 des Spielordners), und die zz_menu.pk3 des letzten Exports liegt im Projektordner
    std::string endung = rel.size() >= 4 ? rel.substr(rel.size() - 4) : std::string();
    for (char& c : endung) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return endung != ".pk3";
}

std::string sicherungText(const std::vector<SicherungsEintrag>& e, std::int64_t zeit) {
    std::string s = "jksautosave 1\ntime " + std::to_string(zeit) + "\n";
    for (const SicherungsEintrag& x : e) { s += "doc " + wert(x.name) + " " + wert(x.platte) + " " + wert(x.datei) + "\n"; }
    return s;
}

bool liesSicherung(const std::string& text, std::vector<SicherungsEintrag>& out, std::int64_t& zeit) {
    out.clear();
    zeit = 0;
    bool kopf = false;
    for (const std::string& z : zeilen(text)) {
        const std::vector<std::string> w = woerter(z);
        if (w.empty()) { continue; }
        if (w[0] == "jksautosave") { kopf = true; }
        else if (w[0] == "time" && w.size() > 1) { zeit = std::atoll(w[1].c_str()); }
        else if (w[0] == "doc" && w.size() > 3 && sichererPfad(w[3])) { out.push_back(SicherungsEintrag{w[1], w[2], w[3]}); }
    }
    return kopf && !out.empty();
}

std::vector<RegEintrag> dateizuordnung(const std::string& exePfad, const std::string& beschreibung) {
    const std::string klasse = "JKStudio.MenuProject";
    const std::string basis = "Software\\Classes\\";
    return {
        {basis + kProjektEndung, "", klasse},
        {basis + klasse, "", beschreibung},
        {basis + klasse + "\\DefaultIcon", "", "\"" + exePfad + "\",0"},
        {basis + klasse + "\\shell\\open\\command", "", "\"" + exePfad + "\" \"%1\""},
    };
}

}  // namespace bhed::menue
