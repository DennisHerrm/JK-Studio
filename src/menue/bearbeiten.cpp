// bearbeiten.cpp - Menue-Editor M1: Aenderungen ueber Quelltext-Spannen.
//
// Eigener Code (kein Engine-Gegenstueck): eine Aenderung am Modell ersetzt
// nur die Woerter, die sie betrifft. Kommentare, Einrueckung und alles
// andere bleiben byte-gleich.
#include "bhed/menue/menue.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bhed::menue {

namespace {

bool gleichOhneFall(const std::string& a, const char* b) noexcept {
    const std::size_t n = std::strlen(b);
    if (a.size() != n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

const Eigenschaft* letzte(const std::vector<Eigenschaft>& v, const char* s) noexcept {
    const Eigenschaft* r = nullptr;
    for (const Eigenschaft& e : v) {
        if (gleichOhneFall(e.schluessel, s)) { r = &e; }   // die Engine wendet die letzte an
    }
    return r;
}

// Die Argumente in Werte gruppieren: MP liest "-5" als "-" und "5".
struct Wert {
    std::uint32_t anfang = 0, ende = 0;
    bool frei = true;   // in der Hauptdatei, nicht aus einem Makro
};
std::vector<Wert> werte(const Eigenschaft& e) {
    std::vector<Wert> v;
    for (std::size_t i = 0; i < e.args.size(); ++i) {
        const Token& t = e.args[i];
        Wert w;
        w.anfang = t.anfang;
        w.ende = t.ende;
        w.frei = t.datei == 0 && !t.ausMakro;
        if (t.s == "-" && t.typ == TT_PUNCTUATION && i + 1 < e.args.size()) {
            const Token& n = e.args[++i];
            w.ende = n.ende;
            w.frei = w.frei && n.datei == 0 && !n.ausMakro;
        }
        v.push_back(w);
    }
    return v;
}

std::string zeilenEnde(const std::string& text) {
    return text.find("\r\n") != std::string::npos ? "\r\n" : "\n";
}

std::size_t zeilenAnfang(const std::string& text, std::size_t p) {
    while (p > 0 && text[p - 1] != '\n') { --p; }
    return p;
}

std::string einrueckungBei(const std::string& text, std::size_t p) {
    const std::size_t a = zeilenAnfang(text, p);
    std::size_t e = a;
    while (e < text.size() && (text[e] == ' ' || text[e] == '\t')) { ++e; }
    return text.substr(a, e - a);
}

// Neue Zeile "schluessel werte" vor die schliessende } des Blocks.
bool einfuegen(const std::string& text, std::uint32_t blockEnde, const std::vector<Eigenschaft>& eig,
               std::uint32_t kopfAnfang, const char* schluessel, const std::string& wert, std::vector<Ersetzung>& out) {
    if (blockEnde == 0 || blockEnde > text.size() || text[blockEnde - 1] != '}') { return false; }
    const std::size_t zu = blockEnde - 1;
    std::string einr;
    std::string abstand = "\t\t\t\t";
    if (!eig.empty() && eig.front().kopf.datei == 0) {
        einr = einrueckungBei(text, eig.front().kopf.anfang);
        // Abstand zwischen Schluessel und Wert wie bei der ersten Eigenschaft
        const Eigenschaft& e0 = eig.front();
        if (!e0.args.empty() && e0.args.front().datei == 0 && e0.args.front().anfang > e0.kopf.ende) {
            const std::string zw = text.substr(e0.kopf.ende, e0.args.front().anfang - e0.kopf.ende);
            if (zw.find('\n') == std::string::npos) {
                // so auffuellen, dass der Wert in derselben Spalte steht (Tabs zu 4)
                const auto spalte = [](const std::string& s) {
                    std::size_t c = 0;
                    for (const char ch : s) { c = (ch == '\t') ? (c / 4 + 1) * 4 : c + 1; }
                    return c;
                };
                const std::size_t ziel = spalte(e0.schluessel + zw);
                std::size_t c = spalte(schluessel);
                abstand.clear();
                while (c < ziel) {
                    abstand += '\t';
                    c = (c / 4 + 1) * 4;
                }
                if (abstand.empty()) { abstand = " "; }
            }
        }
    } else {
        einr = einrueckungBei(text, kopfAnfang) + "\t";
    }
    const std::string nl = zeilenEnde(text);
    const std::size_t za = zeilenAnfang(text, zu);
    bool eigeneZeile = true;
    for (std::size_t i = za; i < zu; ++i) {
        if (text[i] != ' ' && text[i] != '\t') { eigeneZeile = false; }
    }
    Ersetzung r;
    if (eigeneZeile) {
        r.anfang = r.ende = static_cast<std::uint32_t>(za);
        r.neu = einr + schluessel + abstand + wert + nl;
    } else {
        r.anfang = r.ende = static_cast<std::uint32_t>(zu);
        r.neu = std::string(schluessel) + " " + wert + " ";
    }
    out.push_back(r);
    return true;
}

}  // namespace

int zeileVon(const std::string& text, std::uint32_t byte) noexcept {
    int z = 1;
    const std::size_t n = std::min<std::size_t>(byte, text.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (text[i] == '\n') { ++z; }
    }
    return z;
}

const Eigenschaft* finde(const Item& it, const char* schluessel) noexcept { return letzte(it.eigenschaften, schluessel); }
const Eigenschaft* finde(const Menu& m, const char* schluessel) noexcept { return letzte(m.eigenschaften, schluessel); }

std::string zahlText(float v) {
    if (std::fabs(v - std::round(v)) < 1e-4F) {
        char b[32];
        std::snprintf(b, sizeof(b), "%d", static_cast<int>(std::lround(v)));
        return b;
    }
    char b[48];
    std::snprintf(b, sizeof(b), "%.4f", static_cast<double>(v));
    std::string s = b;
    while (!s.empty() && s.back() == '0') { s.pop_back(); }
    if (!s.empty() && s.back() == '.') { s.pop_back(); }
    return s;
}

bool setzeEigenschaft(const Datei& d, int menu, int item, const char* schluessel,
                      const std::vector<std::string>& neu, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const std::vector<Eigenschaft>* eig = &m.eigenschaften;
    std::uint32_t blockEnde = m.ende;
    std::uint32_t kopf = m.anfang;
    if (item >= 0) {
        if (static_cast<std::size_t>(item) >= m.items.size()) { return false; }
        const Item& it = m.items[static_cast<std::size_t>(item)];
        eig = &it.eigenschaften;
        blockEnde = it.ende;
        kopf = it.anfang;
    }
    std::string zusammen;
    for (std::size_t i = 0; i < neu.size(); ++i) {
        if (i != 0) { zusammen += ' '; }
        zusammen += neu[i];
    }
    const Eigenschaft* e = letzte(*eig, schluessel);
    if (e == nullptr) {
        return einfuegen(d.text, blockEnde, *eig, kopf, schluessel, zusammen, out);
    }
    if (e->kopf.datei != 0 || e->kopf.ausMakro) { return false; }
    const std::vector<Wert> w = werte(*e);
    for (const Wert& x : w) {
        if (!x.frei) { return false; }
    }
    if (w.size() == neu.size()) {
        for (std::size_t i = 0; i < w.size(); ++i) {
            if (d.text.compare(w[i].anfang, w[i].ende - w[i].anfang, neu[i]) != 0) {
                out.push_back(Ersetzung{w[i].anfang, w[i].ende, neu[i]});
            }
        }
        return true;
    }
    if (w.empty()) {
        out.push_back(Ersetzung{e->kopf.ende, e->kopf.ende, " " + zusammen});
        return true;
    }
    out.push_back(Ersetzung{w.front().anfang, w.back().ende, zusammen});
    return true;
}

bool setzeRect(const Datei& d, int menu, int item, const Rechteck& r, std::vector<Ersetzung>& out) {
    return setzeEigenschaft(d, menu, item, "rect", {zahlText(r.x), zahlText(r.y), zahlText(r.w), zahlText(r.h)}, out);
}

std::string anwenden(const std::string& text, std::vector<Ersetzung> e) {
    std::sort(e.begin(), e.end(), [](const Ersetzung& a, const Ersetzung& b) {
        return a.anfang != b.anfang ? a.anfang > b.anfang : a.ende > b.ende;
    });
    std::string t = text;
    for (const Ersetzung& x : e) {
        if (x.anfang > x.ende || x.ende > t.size()) { continue; }
        t.replace(x.anfang, x.ende - x.anfang, x.neu);
    }
    return t;
}

std::string meldungText(const Datei& d, const Meldung& m) {
    const std::string datei = (m.datei >= 0 && static_cast<std::size_t>(m.datei) < d.quellen.size())
                                  ? d.quellen[static_cast<std::size_t>(m.datei)].name
                                  : d.pfad;
    if (d.dialekt == Dialekt::MP) {
        return std::string(m.fehler ? "ERROR: " : "WARNING: ") + datei + ", line " + std::to_string(m.zeile) + ": " + m.text;
    }
    std::string t = m.text;
    while (!t.empty() && t.back() == '\n') { t.pop_back(); }
    return "WARNING: " + t + " Line #" + std::to_string(m.zeile) + " of File '" + datei + "'";
}

}  // namespace bhed::menue
