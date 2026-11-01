// spieldaten.cpp - Menue-Editor M1: Dateien aus den Spielordnern lesen.
//
// Eigener Code.
#include "bhed/menue/spieldaten.h"
#include "bhed/utf8pfad.h"

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
        s += (c == '\\') ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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
        for (auto a = o->archives.rbegin(); a != o->archives.rend(); ++a) {
            const Pk3Entry* e = a->find(n);
            if (e != nullptr && readPk3File(*a, *e, out, nullptr)) { return true; }
        }
        std::error_code ec;
        // rc608 Teil 3b: Namen aus Menues sind Latin-1, das Programm laeuft mit UTF-8 als Systemseite: Ein schmaler Pfad warf hier eine Ausnahme
        const std::filesystem::path lose = utf8Pfad(o->directory) / utf8Pfad(pfad);
        if (!o->directory.empty() && std::filesystem::is_regular_file(lose, ec)) {
            std::ifstream f(lose, std::ios::binary);
            std::ostringstream s;
            s << f.rdbuf();
            out = s.str();
            return true;
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
        std::error_code ec;
        if (!o.directory.empty() && std::filesystem::is_regular_file(utf8Pfad(o.directory) / utf8Pfad(pfad), ec)) { return true; }
    }
    return false;
}

std::vector<std::string> Spieldaten::liste(const std::string& praefix, const std::vector<std::string>& endungen) const {
    std::vector<std::string> v = listeWieSpiel(praefix, endungen);
    std::sort(v.begin(), v.end(), [](const std::string& a, const std::string& b) { return norm(a) < norm(b); });
    return v;
}

std::vector<std::string> Spieldaten::listeWieSpiel(const std::string& praefix, const std::vector<std::string>& endungen) const {
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
        // Die losen Dateien des Ordners (FS_ListFiles liest auch sie). Ganze Spielordner (leeres praefix) laeuft niemand durch.
        if (p.empty() || o->directory.empty()) { continue; }
        std::error_code ec;
        const std::filesystem::path wurzel = utf8Pfad(o->directory);
        for (std::filesystem::recursive_directory_iterator it(wurzel / utf8Pfad(praefix), std::filesystem::directory_options::skip_permission_denied, ec), ende;
             !ec && it != ende; it.increment(ec)) {
            if (!it->is_regular_file(ec)) { continue; }
            const std::string name = it->path().lexically_relative(wurzel).generic_string();
            const std::string n = norm(name);
            bool passt = endungen.empty();
            for (const std::string& en : endungen) { passt = passt || endetMit(n, en); }
            if (passt && gesehen.insert(n).second) { v.push_back(name); }
        }
    }
    return v;
}

Lader Spieldaten::lader() const {
    return [this](const std::string& pfad, std::string& inhalt) { return lies(pfad, inhalt); };
}

// rc608: Jedi Outcast legt seine Texte anders ab als Jedi Academy: die Menue-Texte in Pakete MENUS0..3, die der
// Oberflaeche in INGAMETEXT (vm/ui.qvm: "INGAMETEXT_SERVER_REFRESHTIME"). Die Vorschau fragt nach den Namen von Jedi
// Academy; ohne das stand hinter "Lightsaber Only:" oder "Type:" nichts. Was jk2mp als festen Text zeichnet, steht unten.
void jk2Namen(Texte& t) {
    std::vector<std::pair<std::string, std::string>> dazu;
    for (const auto& [k, wert] : t.werte) {
        if (k.size() > 7 && k.compare(0, 5, "MENUS") == 0 && k[5] >= '0' && k[5] <= '9' && k[6] == '_') {
            dazu.emplace_back("MENUS_" + k.substr(7), wert);
        } else if (k.compare(0, 11, "INGAMETEXT_") == 0) {
            dazu.emplace_back("MP_INGAME_" + k.substr(11), wert);
        }
    }
    dazu.emplace_back("MENUS_HUMAN", "HUMAN");
    dazu.emplace_back("MENUS_CLOSED", "CLOSED");
    for (const auto& [k, wert] : dazu) { t.werte.emplace(k, wert); }
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
        jk2Namen(t);
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
