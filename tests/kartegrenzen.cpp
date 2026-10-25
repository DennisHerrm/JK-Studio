// kartegrenzen.cpp - Karten-Editor Stufe 5 Punkt 1: die Grenzen-Tabelle
// (bhed/karte/grenzen.h) gegen die Quelltexte der Engines.
//
// Fuer jede Stelle der Tabelle wird die Datei gelesen, Kommentare entfernt,
// die Zweige von JK2_MODE beachtet (#ifdef / #ifndef / #if JK2_MODE, #else,
// #endif) und das #define bzw. constexpr an genau dieser Zeile gesucht; sein
// Ausdruck wird ausgerechnet (Zahlen, Klammern, + - * / << >> & ~ und Namen
// aus den anderen Dateien desselben Baums) und mit dem Wert der Tabelle
// verglichen. Eine Stelle ohne Namen (q3map2 games.cpp:752) muss die Zahl in
// der Zeile tragen. Dazu: jede Pruefstelle "datei:zeile" zeigt auf eine Zeile
// mit einer Fehlermeldung oder dem Feldzugriff, und die Tabelle ist in sich
// stimmig (erlaubt <= strengere Grenze, Ids eindeutig, jede Zeile erklaert).
//
// Aufruf: kartegrenzen <OpenJK-Wurzel|-> <MovieDuels-Wurzel|-> <q3map2-Wurzel>
//   OpenJK-Wurzel: Ordner mit code/, codemp/, codeJK2/ (SP\ojk\oj)
//   MovieDuels-Wurzel: Ordner mit code/ (SP\mdgit)
//   q3map2-Wurzel: third_party/q3map2
// Fehlt eine Wurzel, werden ihre Stellen uebersprungen (und gezaehlt).
#include "bhed/karte/grenzen.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
int g_uebersprungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { return {}; }
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// Block- und Zeilenkommentare durch Leerzeichen ersetzen (Zeilen bleiben).
std::string ohneKommentare(const std::string& t) {
    std::string o;
    o.reserve(t.size());
    std::size_t i = 0;
    while (i < t.size()) {
        if (t.compare(i, 2, "/*") == 0) {
            std::size_t j = t.find("*/", i + 2);
            j = j == std::string::npos ? t.size() : j + 2;
            for (std::size_t k = i; k < j; ++k) { o += t[k] == '\n' ? '\n' : ' '; }
            i = j;
        } else if (t.compare(i, 2, "//") == 0) {
            std::size_t j = t.find('\n', i);
            j = j == std::string::npos ? t.size() : j;
            o.append(j - i, ' ');
            i = j;
        } else if (t[i] == '"') {   // Zeichenketten unveraendert ueberspringen
            std::size_t j = i + 1;
            while (j < t.size() && t[j] != '"' && t[j] != '\n') { j += t[j] == '\\' ? 2 : 1; }
            j = std::min(t.size(), j + 1);
            o.append(t, i, j - i);
            i = j;
        } else {
            o += t[i++];
        }
    }
    return o;
}

std::string trim(const std::string& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a])) != 0) { ++a; }
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])) != 0) { --b; }
    return s.substr(a, b - a);
}

struct Def {
    int zeile = 0;
    std::string ausdruck;
};
using Defs = std::map<std::string, Def>;   // Name -> erste Definition

// Die Definitionen einer Datei im Zweig mit oder ohne JK2_MODE.
Defs dateiDefs(const std::string& text, bool jk2, std::vector<std::string>* zeilen) {
    Defs d;
    std::istringstream in(ohneKommentare(text));
    std::string l;
    struct Ebene {
        bool jk;
        bool an;
    };
    std::vector<Ebene> stapel;
    int nr = 0;
    while (std::getline(in, l)) {
        ++nr;
        if (zeilen != nullptr) { zeilen->push_back(l); }
        const std::string s = trim(l);
        if (s.empty()) { continue; }
        if (s[0] == '#') {
            std::string r = trim(s.substr(1));
            auto beginnt = [&](const char* w) { return r.rfind(w, 0) == 0; };
            if (beginnt("ifdef") || beginnt("ifndef") || (beginnt("if") && !beginnt("include"))) {
                const bool nicht = beginnt("ifndef");
                if (r.find("JK2_MODE") != std::string::npos) {
                    stapel.push_back({true, nicht ? !jk2 : jk2});
                } else {
                    stapel.push_back({false, true});
                }
                continue;
            }
            if (beginnt("else")) {
                if (!stapel.empty() && stapel.back().jk) { stapel.back().an = !stapel.back().an; }
                continue;
            }
            if (beginnt("endif")) {
                if (!stapel.empty()) { stapel.pop_back(); }
                continue;
            }
        }
        bool aus = false;
        for (const Ebene& e : stapel) {
            if (e.jk && !e.an) { aus = true; }
        }
        if (aus) { continue; }
        if (s[0] == '#') {
            std::string r = trim(s.substr(1));
            if (r.rfind("define", 0) == 0) {
                r = trim(r.substr(6));
                std::size_t i = 0;
                while (i < r.size() && (std::isalnum(static_cast<unsigned char>(r[i])) != 0 || r[i] == '_')) { ++i; }
                const std::string name = r.substr(0, i);
                if (i < r.size() && r[i] == '(') { continue; }   // Makro mit Parametern
                const std::string ausdruck = trim(r.substr(i));
                if (!name.empty() && !ausdruck.empty() && d.find(name) == d.end()) { d[name] = Def{nr, ausdruck}; }
            }
            continue;
        }
        // constexpr <typ> NAME = ausdruck;
        if (s.rfind("constexpr", 0) == 0) {
            const std::size_t gleich = s.find('=');
            const std::size_t semi = s.find(';');
            if (gleich == std::string::npos || semi == std::string::npos || semi < gleich) { continue; }
            std::string links = trim(s.substr(0, gleich));
            const std::size_t leer = links.find_last_of(" \t");
            const std::string name = leer == std::string::npos ? links : links.substr(leer + 1);
            const std::string ausdruck = trim(s.substr(gleich + 1, semi - gleich - 1));
            if (!name.empty() && d.find(name) == d.end()) { d[name] = Def{nr, ausdruck}; }
        }
    }
    return d;
}

// Ein kleiner Rechner fuer die Ausdruecke der #defines.
class Rechner {
public:
    Rechner(const std::string& t, const Defs& sym, int tiefe) : m_t(t), m_sym(sym), m_tiefe(tiefe) {}
    bool rechne(long long& out) {
        m_ok = true;
        out = oder();
        leer();
        return m_ok && m_i == m_t.size();
    }

private:
    void leer() {
        while (m_i < m_t.size() && std::isspace(static_cast<unsigned char>(m_t[m_i])) != 0) { ++m_i; }
    }
    bool ist(const char* op) {
        leer();
        const std::size_t n = std::char_traits<char>::length(op);
        if (m_t.compare(m_i, n, op) == 0) {
            m_i += n;
            return true;
        }
        return false;
    }
    long long oder() {
        long long a = schieb();
        while (true) {
            leer();
            if (m_i < m_t.size() && m_t[m_i] == '&' && (m_i + 1 >= m_t.size() || m_t[m_i + 1] != '&')) {
                ++m_i;
                a &= schieb();
            } else {
                return a;
            }
        }
    }
    long long schieb() {
        long long a = summe();
        while (true) {
            if (ist("<<")) { a <<= summe(); }
            else if (ist(">>")) { a >>= summe(); }
            else { return a; }
        }
    }
    long long summe() {
        long long a = produkt();
        while (true) {
            leer();
            if (m_i < m_t.size() && m_t[m_i] == '+') { ++m_i; a += produkt(); }
            else if (m_i < m_t.size() && m_t[m_i] == '-') { ++m_i; a -= produkt(); }
            else { return a; }
        }
    }
    long long produkt() {
        long long a = einzeln();
        while (true) {
            leer();
            if (m_i < m_t.size() && m_t[m_i] == '*') { ++m_i; a *= einzeln(); }
            else if (m_i < m_t.size() && m_t[m_i] == '/') {
                ++m_i;
                const long long b = einzeln();
                if (b == 0) { m_ok = false; return 0; }
                a /= b;
            } else { return a; }
        }
    }
    long long einzeln() {
        leer();
        if (m_i >= m_t.size()) { m_ok = false; return 0; }
        const char c = m_t[m_i];
        if (c == '(') {
            ++m_i;
            const long long a = oder();
            if (!ist(")")) { m_ok = false; }
            return a;
        }
        if (c == '-') { ++m_i; return -einzeln(); }
        if (c == '~') { ++m_i; return ~einzeln(); }
        if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
            std::size_t j = m_i;
            while (j < m_t.size() && (std::isalnum(static_cast<unsigned char>(m_t[j])) != 0)) { ++j; }
            std::string zahl = m_t.substr(m_i, j - m_i);
            m_i = j;
            while (!zahl.empty() && (zahl.back() == 'u' || zahl.back() == 'U' || zahl.back() == 'l' || zahl.back() == 'L')) { zahl.pop_back(); }
            char* ende = nullptr;
            const long long v = std::strtoll(zahl.c_str(), &ende, 0);
            if (ende == nullptr || *ende != 0) { m_ok = false; }
            return v;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_') {
            std::size_t j = m_i;
            while (j < m_t.size() && (std::isalnum(static_cast<unsigned char>(m_t[j])) != 0 || m_t[j] == '_')) { ++j; }
            const std::string name = m_t.substr(m_i, j - m_i);
            m_i = j;
            const auto it = m_sym.find(name);
            if (it == m_sym.end() || m_tiefe > 16) { m_ok = false; return 0; }
            long long v = 0;
            Rechner r(it->second.ausdruck, m_sym, m_tiefe + 1);
            if (!r.rechne(v)) { m_ok = false; }
            return v;
        }
        m_ok = false;
        return 0;
    }
    const std::string& m_t;
    const Defs& m_sym;
    int m_tiefe;
    std::size_t m_i = 0;
    bool m_ok = true;
};

struct Datei {
    Defs defs;
    std::vector<std::string> zeilen;   // ohne Kommentare
    std::vector<std::string> roh;      // wie in der Datei
    bool da = false;
};

std::filesystem::path g_wurzel[3];   // OpenJK, MovieDuels, q3map2
int baumIndex(const std::string& baum) {
    if (baum == "OpenJK") { return 0; }
    if (baum == "MovieDuels") { return 1; }
    return 2;
}

std::map<std::string, Datei> g_dateien;   // Schluessel: baum|jk2|datei
Datei& datei(const std::string& baum, bool jk2, const std::string& pfad) {
    const std::string schluessel = baum + "|" + (jk2 ? "1" : "0") + "|" + pfad;
    auto it = g_dateien.find(schluessel);
    if (it != g_dateien.end()) { return it->second; }
    Datei d;
    const std::filesystem::path p = g_wurzel[baumIndex(baum)] / pfad;
    const std::string text = lies(p);
    if (!text.empty()) {
        d.da = true;
        d.defs = dateiDefs(text, jk2, &d.zeilen);
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l)) { d.roh.push_back(l); }
    }
    return g_dateien.emplace(schluessel, std::move(d)).first->second;
}

bool wurzelDa(const std::string& baum) {
    const std::filesystem::path& w = g_wurzel[baumIndex(baum)];
    std::error_code ec;
    return !w.empty() && w != "-" && std::filesystem::is_directory(w, ec);
}

// Alle Namen der Dateien eines Baums (fuer Namen aus anderen Dateien).
Defs symbole(const std::string& baum, bool jk2, const std::vector<std::string>& pfade) {
    Defs s;
    for (const std::string& p : pfade) {
        const Datei& d = datei(baum, jk2, p);
        for (const auto& [n, def] : d.defs) { s.emplace(n, def); }
    }
    return s;
}

std::string text(const GrenzStelle& s) {
    return s.baum + " " + s.datei + ":" + std::to_string(s.zeile) + " " + s.name + (s.jk2 ? " [JK2_MODE]" : "");
}

void stellePruefen(const GrenzStelle& s, const std::vector<std::string>& pfadeDesBaums, const std::string& wo) {
    if (!wurzelDa(s.baum)) {
        ++g_uebersprungen;
        return;
    }
    const Datei& d = datei(s.baum, s.jk2, s.datei);
    pruefe(d.da, wo + ": " + text(s) + " - Datei fehlt");
    if (!d.da) { return; }
    if (s.name.empty()) {
        // die Zahl muss in der Zeile stehen
        bool gefunden = false;
        if (s.zeile >= 1 && s.zeile <= static_cast<int>(d.roh.size())) {
            const std::string& l = d.roh[static_cast<std::size_t>(s.zeile - 1)];
            const std::string zahl = std::to_string(s.wert);
            std::size_t p = l.find(zahl);
            while (p != std::string::npos && !gefunden) {
                const bool links = p == 0 || std::isdigit(static_cast<unsigned char>(l[p - 1])) == 0;
                const bool rechts = p + zahl.size() >= l.size() || std::isdigit(static_cast<unsigned char>(l[p + zahl.size()])) == 0;
                gefunden = links && rechts;
                p = l.find(zahl, p + 1);
            }
        }
        pruefe(gefunden, wo + ": " + text(s) + " - die Zahl " + std::to_string(s.wert) + " steht nicht in der Zeile");
        return;
    }
    const auto it = d.defs.find(s.name);
    pruefe(it != d.defs.end(), wo + ": " + text(s) + " - Name nicht definiert");
    if (it == d.defs.end()) { return; }
    pruefe(it->second.zeile == s.zeile, wo + ": " + text(s) + " - steht in Zeile " + std::to_string(it->second.zeile));
    const Defs sym = symbole(s.baum, s.jk2, pfadeDesBaums);
    long long v = 0;
    Rechner r(it->second.ausdruck, sym, 0);
    const bool ok = r.rechne(v);
    pruefe(ok, wo + ": " + text(s) + " - Ausdruck \"" + it->second.ausdruck + "\" nicht ausrechenbar");
    if (ok) {
        pruefe(v == s.wert, wo + ": " + text(s) + " = " + std::to_string(v) + ", Tabelle " + std::to_string(s.wert));
    }
}

// "code/qcommon/cm_load.cpp:143 ... tr_surface.cpp:61" - jede datei:zeile
// muss auf eine Zeile mit Fehlermeldung, Ausgabe oder Feldzugriff zeigen.
void pruefstellePruefen(const Grenze& g, const std::string& wo) {
    const std::string& t = g.pruefstelle;
    std::string ordner;   // Ordner der letzten Datei mit Pfad
    std::string baum = g.stellen.empty() ? std::string("OpenJK") : g.stellen.front().baum;
    if (baum == "q3map2") { baum = "OpenJK"; }
    const bool jk2 = !g.stellen.empty() && g.stellen.front().jk2;
    std::size_t i = 0;
    while (i < t.size()) {
        // ein Wort aus Pfadzeichen
        std::size_t j = i;
        while (j < t.size() && (std::isalnum(static_cast<unsigned char>(t[j])) != 0 || t[j] == '_' || t[j] == '/' || t[j] == '.' || t[j] == '-')) { ++j; }
        std::string wort = t.substr(i, j - i);
        if (j < t.size() && t[j] == ':' && j + 1 < t.size() && std::isdigit(static_cast<unsigned char>(t[j + 1])) != 0 &&
            (wort.find(".cpp") != std::string::npos || wort.find(".h") != std::string::npos || wort.find(".c") != std::string::npos)) {
            std::size_t k = j + 1;
            while (k < t.size() && std::isdigit(static_cast<unsigned char>(t[k])) != 0) { ++k; }
            const int zeile = std::atoi(t.substr(j + 1, k - j - 1).c_str());
            // der Baum: q3map2 / MovieDuels, wenn das Wort davor so heisst;
            // NetRadiant-Custom selbst liegt nicht vor (uebersprungen)
            const bool q3 = i >= 7 && t.compare(i - 7, 7, "q3map2 ") == 0;
            const bool mdVerweis = i >= 11 && t.compare(i - 11, 11, "MovieDuels ") == 0;
            const bool nrc = i >= 4 && t.compare(i - 4, 4, "NRC ") == 0;
            if (nrc) {
                ++g_uebersprungen;
                i = k;
                continue;
            }
            std::string pfad;
            std::string b = baum;
            if (q3) {
                b = "q3map2";
                pfad = "tools/quake3/q3map2/" + wort;
            } else if (mdVerweis) {
                b = "MovieDuels";
                pfad = wort;
            } else if (wort.find('/') != std::string::npos) {
                pfad = wort;
                ordner = wort.substr(0, wort.rfind('/') + 1);
            } else {
                pfad = ordner + wort;
            }
            if (!q3 && pfad.rfind("client/", 0) == 0) { i = k; continue; }   // nicht im Quellstand
            if (wurzelDa(b) && !pfad.empty()) {
                const Datei& d = datei(b, jk2, pfad);
                bool treffer = false;
                if (d.da && zeile >= 1 && zeile <= static_cast<int>(d.roh.size())) {
                    const std::string& l = d.roh[static_cast<std::size_t>(zeile - 1)];
                    for (const char* w : {"Error", "Printf", "MAX_", "lightmaps[", "Warning", "xml_Select", "gridArray["}) {
                        if (l.find(w) != std::string::npos) { treffer = true; }
                    }
                }
                pruefe(treffer, wo + ": Pruefstelle " + b + " " + pfad + ":" + std::to_string(zeile) + " zeigt nicht auf eine Fehler-/Feldzeile");
            } else {
                ++g_uebersprungen;
            }
            i = k;
            continue;
        }
        i = j > i ? j : i + 1;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Aufruf: kartegrenzen <OpenJK-Wurzel|-> <MovieDuels-Wurzel|-> <q3map2-Wurzel>\n");
        return 2;
    }
    for (int i = 0; i < 3; ++i) { g_wurzel[i] = argv[i + 1]; }

    const char* kernIds[] = {"bsp.lichtgitter", "kollision.submodelle", "kollision.patchebenen", "kollision.patchfacetten",
                             "laufzeit.entitaeten", "laufzeit.modelle", "laufzeit.klaenge", "laufzeit.effekte",
                             "spawn.variablen", "spawn.zeichen", "renderer.flaechenpunkte", "renderer.lightmaps"};
    for (int si = 0; si < kGrenzSpiele; ++si) {
        const auto spiel = static_cast<GrenzSpiel>(si);
        const std::vector<Grenze>& t = grenzen(spiel);
        const std::string sname = grenzSpielName(spiel);
        pruefe(t.size() >= 30, sname + ": mindestens 30 Grenzen (" + std::to_string(t.size()) + ")");
        std::set<std::string> ids;
        // alle Dateien je Baum (fuer Namen aus anderen Dateien)
        std::map<std::string, std::vector<std::string>> pfade;
        for (const Grenze& g : t) {
            for (const GrenzStelle& s : g.stellen) { pfade[s.baum].push_back(s.datei); }
        }
        for (const Grenze& g : t) {
            const std::string wo = sname + " " + g.id;
            pruefe(ids.insert(g.id).second, wo + ": Id eindeutig");
            pruefe(!g.stellen.empty(), wo + ": hat eine Stelle im Quelltext");
            pruefe(!g.pruefstelle.empty() && !g.rechnung.empty(), wo + ": Pruefstelle und Rechnung genannt");
            pruefe(g.titel != bhed::i18n::Str::Count, wo + ": hat einen Titel");
            pruefe(g.erlaubt > 0 && g.erlaubt <= std::min(g.wert, g.vanilla), wo + ": erlaubt (" + std::to_string(g.erlaubt) +
                                                                                  ") <= strengere Grenze");
            pruefe(g.vanilla <= g.wert, wo + ": Vanilla nicht groesser als der Wert");
            if (g.vanilla != g.wert) { pruefe(!g.vanillaQuelle.empty(), wo + ": Vanilla-Quelle genannt"); }
            if (g.offen) { pruefe(g.vanillaQuelle.find("offen") != std::string::npos, wo + ": 'offen' in der Vanilla-Quelle erklaert"); }
            // die erste Stelle traegt den Namen der Grenze und ihren Wert (ausser bei Rechnungen)
            if (!g.stellen.empty() && g.stellen.front().name == g.name) {
                pruefe(g.stellen.front().wert == g.wert || g.id == "renderer.nebel" || g.id == "laufzeit.entitaeten",
                       wo + ": Wert der Tabelle = Wert der ersten Stelle");
            }
            for (const GrenzStelle& s : g.stellen) { stellePruefen(s, pfade[s.baum], wo); }
            pruefstellePruefen(g, wo);
            pruefe(grenzeSuchen(spiel, g.id) == &g, wo + ": grenzeSuchen findet sie");
        }
        for (const char* id : kernIds) { pruefe(ids.count(id) == 1, sname + ": hat " + id); }
    }
    // Werte, die je Spiel verschieden sein muessen (aus den Quellen, siehe NOTIZEN_karte5.md)
    auto erlaubt = [](GrenzSpiel s, const char* id) {
        const Grenze* g = grenzeSuchen(s, id);
        return g != nullptr ? g->erlaubt : -1;
    };
    pruefe(erlaubt(GrenzSpiel::JkaSp, "kollision.patchebenen") == 2048, "JKA SP: Patch-Ebenen gegen Vanilla 2048");
    pruefe(erlaubt(GrenzSpiel::Jk2, "kollision.patchebenen") == 2048, "JK2: Patch-Ebenen gegen 2048 (offen)");
    pruefe(erlaubt(GrenzSpiel::MovieDuels, "kollision.patchebenen") == 8192, "MD: Patch-Ebenen 8192");
    pruefe(erlaubt(GrenzSpiel::JkaSp, "laufzeit.klaenge") == 379 && erlaubt(GrenzSpiel::Jk2, "laufzeit.klaenge") == 255 &&
               erlaubt(GrenzSpiel::MovieDuels, "laufzeit.klaenge") == 511 && erlaubt(GrenzSpiel::JkaMp, "laufzeit.klaenge") == 255,
           "Klaenge: 379 / 255 / 511 / 255");
    pruefe(erlaubt(GrenzSpiel::JkaSp, "laufzeit.entitaeten") == 1021 && erlaubt(GrenzSpiel::MovieDuels, "laufzeit.entitaeten") == 4093 &&
               erlaubt(GrenzSpiel::JkaMp, "laufzeit.entitaeten") == 990,
           "Entities: 1021 / 4093 / 990");
    pruefe(erlaubt(GrenzSpiel::Jk2, "laufzeit.welteffekte") == 3 && erlaubt(GrenzSpiel::JkaSp, "laufzeit.welteffekte") == 65 &&
               erlaubt(GrenzSpiel::MovieDuels, "laufzeit.welteffekte") == 127,
           "Welteffekte: 3 / 65 / 127");
    pruefe(grenzeSuchen(GrenzSpiel::Jk2, "laufzeit.npcmodelle") == nullptr, "JK2: keine NPC-Modell-Grenze (codeJK2 kennt keine)");
    pruefe(erlaubt(GrenzSpiel::JkaMp, "spawn.zeichen") == 4096, "MP: 4096 Zeichen je Entity");

    std::printf("kartegrenzen: %d Pruefungen, %d Fehlschlaege, %d Stellen uebersprungen (Quelltext fehlt)\n", g_pruefungen, g_fehler,
                g_uebersprungen);
    return g_fehler == 0 ? 0 : 1;
}
