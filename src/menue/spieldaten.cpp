// spieldaten.cpp - Menue-Editor M1: Dateien aus den Spielordnern lesen.
//
// Eigener Code.
#include "bhed/menue/spieldaten.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace bhed::menue {

namespace {
std::string norm(const std::string& p) {
    std::string s;
    for (const char c : p) {
        s += (c == '\') ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    while (!s.empty() && s[0] == '/') { s.erase(0, 1); }
    return s;
}
bool endetMit(const std::string& s, const std::string& e) {
    return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0;
}
}  // namespace

bool Spieldaten::ordnerDazu(const std::string& pfad) {
    GamePath g;
    const bool ok = scanGamePath(pfad, g);
    g.directory = pfad;
    ordner.push_back(std::move(g));
    return ok;
}

bool Spieldaten::lies(const std::string& pfad, std::string& out) const {
    const std::string n = norm(pfad);
    for (auto o = ordner.rbegin(); o != ordner.rend(); ++o) {
        std::error_code ec;
        const std::filesystem::path lose = std::filesystem::path(o->directory) / pfad;
        if (!o->directory.empty() && std::filesystem::is_regular_file(lose, ec)) {
            std::ifstream f(lose, std::ios::binary);
            std::ostringstream s;
            s << f.rdbuf();
            out = s.str();
            return true;
        }
        for (auto a = o->archives.rbegin(); a != o->archives.rend(); ++a) {
            const Pk3Entry* e = a->find(n);
            if (e != nullptr && readPk3File(*a, *e, out, nullptr)) { return true; }
        }
    }
    return false;
}

bool Spieldaten::gibt(const std::string& pfad) const {
    const std::string n = norm(pfad);
    for (const GamePath& o : ordner) {
        for (const Pk3& a : o.archives) {
            if (a.find(n) != nullptr) { return true; }
        }
    }
    return false;
}

std::vector<std::string> Spieldaten::liste(const std::string& praefix, const std::vector<std::string>& endungen) const {
    std::vector<std::string> v;
    std::set<std::string> gesehen;
    const std::string p = norm(praefix);
    for (auto o = ordner.rbegin(); o != ordner.rend(); ++o) {
        for (auto a = o->archives.rbegin(); a != o->archives.rend(); ++a) {
            for (const Pk3Entry& e : a->entries) {
                const std::string n = norm(e.name);
                if (n.compare(0, p.size(), p) != 0) { continue; }
                bool passt = endungen.empty();
                for (const std::string& en : endungen) { passt = passt || endetMit(n, en); }
                if (passt && gesehen.insert(n).second) { v.push_back(e.name); }
            }
        }
    }
    std::sort(v.begin(), v.end(), [](const std::string& a, const std::string& b) { return norm(a) < norm(b); });
    return v;
}

Lader Spieldaten::lader() const {
    return [this](const std::string& pfad, std::string& inhalt) { return lies(pfad, inhalt); };
}

Texte ladeTexte(const Spieldaten& d, Spiel spiel, const std::string& sprache) {
    Texte t;
    if (spiel == Spiel::JK2) {
        int nr = 1;
        std::string s = norm(sprache);
        if (s == "french") { nr = 2; }
        if (s == "german") { nr = 3; }
        for (const std::string& f : d.liste("strip/", {".sp"})) {
            std::string inhalt;
            if (d.lies(f, inhalt)) { t.ladeSp(inhalt, nr); }
        }
        return t;
    }
    std::string up = sprache;
    for (char& c : up) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    for (const std::string& f : d.liste("strings/english/", {".str"})) {
        std::string inhalt;
        if (d.lies(f, inhalt)) { t.ladeStr(f, inhalt, "ENGLISH"); }
    }
    if (up != "ENGLISH") {
        for (const std::string& f : d.liste("strings/" + norm(sprache) + "/", {".str"})) {
            std::string inhalt;
            if (d.lies(f, inhalt)) { t.ladeStr(f, inhalt, up); }
        }
    }
    return t;
}

}  // namespace bhed::menue
