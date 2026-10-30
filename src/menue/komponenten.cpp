// komponenten.cpp - Menue-Editor M7 Punkt 3: Komponenten (verknuepfte Kopien). Eigener Code.
#include "bhed/menue/komponenten.h"

#include <algorithm>

namespace bhed::menue {

namespace {

std::string ohneRand(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) { s.pop_back(); }
    std::size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t')) { ++a; }
    return s.substr(a);
}

// Anfang der Zeile des itemDef und dessen Einrueckung; false, wenn vor "itemDef" in der Zeile etwas steht
bool zeilenAnfang(const std::string& t, std::uint32_t pos, std::size_t& anfang, std::string& einzug) {
    anfang = pos == 0 ? 0 : t.rfind('\n', pos - 1);
    anfang = anfang == std::string::npos ? 0 : anfang + 1;
    einzug = t.substr(anfang, pos - anfang);
    return std::all_of(einzug.begin(), einzug.end(), [](char c) { return c == ' ' || c == '\t'; });
}

// Die Kennungszeile direkt vor dem itemDef: 'k' / 'i' und der Name
char kennung(const std::string& t, const Item& it, std::string& name) {
    if (it.kopf.datei != 0) { return 0; }
    std::size_t ls = 0;
    std::string einzug;
    if (!zeilenAnfang(t, it.anfang, ls, einzug) || ls == 0) { return 0; }
    const std::size_t pe = ls - 1;   // '\n' der Zeile davor
    std::size_t ps = pe == 0 ? std::string::npos : t.rfind('\n', pe - 1);
    ps = ps == std::string::npos ? 0 : ps + 1;
    const std::string zeile = ohneRand(t.substr(ps, pe - ps));
    for (const auto& [k, c] : {std::pair<const char*, char>{kKomponenteKennung, 'k'}, {kInstanzKennung, 'i'}}) {
        const std::string kk = std::string(k) + " ";
        if (zeile.compare(0, kk.size(), kk) == 0) {
            name = ohneRand(zeile.substr(kk.size()));
            return name.empty() ? 0 : c;
        }
    }
    return 0;
}

// Der Text des Originals mit eigener Lage (x, y) und eigenem Namen (leer = der des Originals)
bool kopieText(const Datei& d, const std::string& t, int menu, int original, float x, float y, const std::string& name, std::string& out) {
    const Item& o = d.menus[static_cast<std::size_t>(menu)].items[static_cast<std::size_t>(original)];
    Rechteck r = o.window.rect;
    r.x = x;
    r.y = y;
    std::vector<Ersetzung> e;
    if (!setzeRect(d, menu, original, r, e)) { return false; }
    if (!name.empty() && name != o.window.name && !setzeEigenschaft(d, menu, original, "name", {name}, e)) { return false; }
    std::vector<Ersetzung> innen;
    for (Ersetzung x2 : e) {
        if (x2.anfang < o.anfang || x2.ende > o.ende) { return false; }
        x2.anfang -= o.anfang;
        x2.ende -= o.anfang;
        innen.push_back(std::move(x2));
    }
    out = anwenden(t.substr(o.anfang, o.ende - o.anfang), std::move(innen));
    return true;
}

}  // namespace

std::vector<Komponente> komponenten(const Datei& d, const std::string& t) {
    std::vector<Komponente> r;
    for (std::size_t m = 0; m < d.menus.size(); ++m) {
        const Menu& me = d.menus[m];
        std::vector<std::pair<std::string, int>> kopien;
        for (std::size_t i = 0; i < me.items.size(); ++i) {
            std::string name;
            const char c = kennung(t, me.items[i], name);
            if (c == 'k') {
                Komponente k;
                k.name = name;
                k.menu = static_cast<int>(m);
                k.original = static_cast<int>(i);
                r.push_back(std::move(k));
            } else if (c == 'i') {
                kopien.emplace_back(name, static_cast<int>(i));
            }
        }
        for (const auto& [name, i] : kopien) {
            for (Komponente& k : r) {
                if (k.menu == static_cast<int>(m) && k.name == name) {
                    k.instanzen.push_back(i);
                    break;
                }
            }
        }
    }
    return r;
}

char komponentenRolle(const Datei& d, const std::string& t, int menu, int item, std::string& name) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size() || item < 0 ||
        static_cast<std::size_t>(item) >= d.menus[static_cast<std::size_t>(menu)].items.size()) {
        return 0;
    }
    return kennung(t, d.menus[static_cast<std::size_t>(menu)].items[static_cast<std::size_t>(item)], name);
}

bool alsKomponente(const Datei& d, const std::string& t, int menu, int item, const std::string& name, std::vector<Ersetzung>& out) {
    std::string alt;
    if (name.empty() || name.find_first_of(" \t\r\n") != std::string::npos || komponentenRolle(d, t, menu, item, alt) != 0) { return false; }
    const Item& it = d.menus[static_cast<std::size_t>(menu)].items[static_cast<std::size_t>(item)];
    std::size_t ls = 0;
    std::string einzug;
    if (it.kopf.datei != 0 || !zeilenAnfang(t, it.anfang, ls, einzug)) { return false; }
    out.push_back({static_cast<std::uint32_t>(ls), static_cast<std::uint32_t>(ls), einzug + kKomponenteKennung + " " + name + "\n"});
    return true;
}

bool kopieEinfuegen(const Datei& d, const std::string& t, const Komponente& k, float x, float y, const std::string& instanzName,
                    std::vector<Ersetzung>& out) {
    if (k.menu < 0 || k.original < 0) { return false; }
    std::string kopie;
    if (!kopieText(d, t, k.menu, k.original, x, y, instanzName, kopie)) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(k.menu)];
    std::uint32_t hinter = m.items[static_cast<std::size_t>(k.original)].ende;
    for (const int i : k.instanzen) { hinter = std::max(hinter, m.items[static_cast<std::size_t>(i)].ende); }
    std::size_t ls = 0;
    std::string einzug;
    (void)zeilenAnfang(t, m.items[static_cast<std::size_t>(k.original)].anfang, ls, einzug);
    out.push_back({hinter, hinter, "\n" + einzug + kInstanzKennung + " " + k.name + "\n" + einzug + kopie});
    return true;
}

std::vector<Ersetzung> kopienAbgleichen(const Datei& d, const std::string& t) {
    std::vector<Ersetzung> r;
    for (const Komponente& k : komponenten(d, t)) {
        const Menu& m = d.menus[static_cast<std::size_t>(k.menu)];
        for (const int i : k.instanzen) {
            const Item& it = m.items[static_cast<std::size_t>(i)];
            std::string soll;
            if (!kopieText(d, t, k.menu, k.original, it.window.rect.x, it.window.rect.y, it.window.name, soll)) { continue; }
            if (t.compare(it.anfang, it.ende - it.anfang, soll) != 0) { r.push_back({it.anfang, it.ende, soll}); }
        }
    }
    return r;
}

std::vector<Ersetzung> alsEineErsetzung(const std::string& alt, const std::string& neu) {
    if (alt == neu) { return {}; }
    std::size_t a = 0;
    while (a < alt.size() && a < neu.size() && alt[a] == neu[a]) { ++a; }
    std::size_t e = 0;
    while (e < alt.size() - a && e < neu.size() - a && alt[alt.size() - 1 - e] == neu[neu.size() - 1 - e]) { ++e; }
    return {{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(alt.size() - e), neu.substr(a, neu.size() - e - a)}};
}

}  // namespace bhed::menue
