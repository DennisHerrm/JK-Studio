// layout.cpp - Menue-Editor M5 Punkt 1: Layouts wie in Qt, gespeichert als
// Kommentarzeile "// jkstudio:layout ..." im menuDef (siehe layout.h).
//
// Eigener Code.
#include "bhed/menue/layout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace bhed::menue {

namespace {

constexpr const char* kKennung = "// jkstudio:layout";

// Namen in der Zeile: Leerzeichen, Komma, Gleich und % als %XX (Item-Namen duerfen im .menu in
// Anfuehrungszeichen stehen und damit Leerzeichen enthalten)
std::string kodiere(const std::string& s) {
    std::string o;
    for (const char c : s) {
        if (c == ' ' || c == ',' || c == '=' || c == '%' || c == '\t' || c == '@' || c == '*') {
            char b[4];
            std::snprintf(b, sizeof(b), "%%%02X", static_cast<unsigned>(static_cast<unsigned char>(c)));
            o += b;
        } else {
            o += c;
        }
    }
    return o;
}

std::string dekodiere(const std::string& s) {
    std::string o;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            o += static_cast<char>(std::strtol(s.substr(i + 1, 2).c_str(), nullptr, 16));
            i += 2;
        } else {
            o += s[i];
        }
    }
    return o;
}

std::vector<std::string> teile(const std::string& s, char t) {
    std::vector<std::string> v;
    std::size_t a = 0;
    for (;;) {
        const std::size_t e = s.find(t, a);
        v.push_back(s.substr(a, e == std::string::npos ? std::string::npos : e - a));
        if (e == std::string::npos) { break; }
        a = e + 1;
    }
    return v;
}

const char* artName(LayoutArt a) {
    switch (a) {
        case LayoutArt::Waagrecht: return "hbox";
        case LayoutArt::Senkrecht: return "vbox";
        case LayoutArt::Raster: return "grid";
        case LayoutArt::Teiler: return "split";
    }
    return "hbox";
}

const char* ausrName(LayoutAusrichtung a) {
    switch (a) {
        case LayoutAusrichtung::Anfang: return "start";
        case LayoutAusrichtung::Mitte: return "center";
        case LayoutAusrichtung::Ende: return "end";
        case LayoutAusrichtung::Fuellen: return "fill";
    }
    return "start";
}

LayoutAusrichtung ausrAus(const std::string& s) {
    if (s == "center") { return LayoutAusrichtung::Mitte; }
    if (s == "end") { return LayoutAusrichtung::Ende; }
    if (s == "fill") { return LayoutAusrichtung::Fuellen; }
    return LayoutAusrichtung::Anfang;
}

float zahl(const std::string& s) { return static_cast<float>(std::atof(s.c_str())); }

// Ganze Punkte wie in den Original-Menues; Kanten kumulativ gerundet, damit sich nichts aufsummiert
float rund(float v) { return std::floor(v + 0.5F); }

struct Rechner {
    const std::vector<Layout>& ls;
    const LayoutGroessen& g;
    LayoutErgebnis& e;
    std::set<std::string> fertig;

    int finde(const std::string& name) const {
        for (std::size_t i = 0; i < ls.size(); ++i) {
            if (ls[i].name == name) { return static_cast<int>(i); }
        }
        return -1;
    }

    bool itemRect(const std::string& n, Rechteck& r) const {
        const auto it = e.rects.find(n);
        if (it != e.rects.end()) {
            r = it->second;
            return true;
        }
        return g.rect && g.rect(n, r);
    }

    // Wunschgroesse eines Kindes (Item: sein rect, mindestens der breiteste Text; Layout: seine Flaeche)
    Rechteck wunsch(const LayoutKind& k) const {
        Rechteck r;
        if (k.spacer) { return r; }
        if (k.layout) {
            const int li = finde(k.name);
            if (li >= 0) { r = flaeche(ls[static_cast<std::size_t>(li)]); }
            return r;
        }
        (void)itemRect(k.name, r);
        return r;
    }

    float text(const LayoutKind& k) const { return (k.spacer || k.layout || !g.textBreite) ? 0.0F : g.textBreite(k.name); }

    Rechteck flaeche(const Layout& l) const {
        Rechteck r = l.rahmen;
        if (!l.rahmenItem.empty()) { (void)itemRect(l.rahmenItem, r); }
        return r;
    }

    void setze(const LayoutKind& k, const Rechteck& r) {
        if (k.spacer) { return; }
        if (k.layout) {
            const int li = finde(k.name);
            if (li < 0) {
                e.fehlend.push_back("@" + k.name);
                return;
            }
            const Layout& c = ls[static_cast<std::size_t>(li)];
            if (!c.rahmenItem.empty()) { e.rects[c.rahmenItem] = r; }
            rechne(c, &r);
            return;
        }
        Rechteck alt;
        if (!itemRect(k.name, alt)) {
            e.fehlend.push_back(k.name);
            return;
        }
        e.rects[k.name] = r;
        if (text(k) > r.w + 0.5F) { e.warnungen.push_back(k.name); }
    }

    std::vector<const LayoutKind*> sichtbare(const Layout& l) const {
        std::vector<const LayoutKind*> v;
        for (const LayoutKind& k : l.kinder) {
            if (l.versteckteAuslassen && !k.spacer && !k.layout && g.sichtbar && !g.sichtbar(k.name)) { continue; }
            v.push_back(&k);
        }
        return v;
    }

    // Lage quer zur Hauptachse
    static void quer(LayoutAusrichtung a, float start, float platz, float wunschGr, float& pos, float& gr) {
        gr = a == LayoutAusrichtung::Fuellen ? platz : std::min(wunschGr, platz);
        pos = start;
        if (a == LayoutAusrichtung::Mitte) { pos = start + (platz - gr) / 2; }
        if (a == LayoutAusrichtung::Ende) { pos = start + platz - gr; }
    }

    void reihe(const Layout& l, const Rechteck& in, bool waagrecht) {
        const std::vector<const LayoutKind*> k = sichtbare(l);
        if (k.empty()) { return; }
        const float platz = waagrecht ? in.w : in.h;
        std::vector<float> gr(k.size(), 0.0F);
        float summe = 0, dehnung = 0;
        int dehnbar = 0;
        for (std::size_t i = 0; i < k.size(); ++i) {
            const Rechteck w = wunsch(*k[i]);
            gr[i] = waagrecht ? std::max(w.w, text(*k[i])) : w.h;
            summe += gr[i];
            if (k[i]->spacer) { dehnung += k[i]->dehnung; }
            else { ++dehnbar; }
        }
        const float abst = l.abstand * static_cast<float>(k.size() - 1);
        float frei = platz - summe - abst;
        const LayoutAusrichtung haupt = waagrecht ? l.h : l.v;
        const LayoutAusrichtung querA = waagrecht ? l.v : l.h;
        float start = 0;
        if (frei < 0 && summe > 0) {
            // zu eng: alle Kinder anteilig kleiner (Qt: bis zur Mindestgroesse; der Text meldet sich als Warnung)
            const float f = std::max(0.0F, platz - abst) / summe;
            for (float& x : gr) { x *= f; }
            frei = 0;
        } else if (frei > 0 && dehnung > 0) {
            for (std::size_t i = 0; i < k.size(); ++i) {
                if (k[i]->spacer) { gr[i] += frei * k[i]->dehnung / dehnung; }
            }
            frei = 0;
        } else if (frei > 0 && haupt == LayoutAusrichtung::Fuellen && dehnbar > 0) {
            for (std::size_t i = 0; i < k.size(); ++i) {
                if (!k[i]->spacer) { gr[i] += frei / static_cast<float>(dehnbar); }
            }
            frei = 0;
        }
        if (haupt == LayoutAusrichtung::Mitte) { start = frei / 2; }
        if (haupt == LayoutAusrichtung::Ende) { start = frei; }
        float p = (waagrecht ? in.x : in.y) + start;
        for (std::size_t i = 0; i < k.size(); ++i) {
            const float a = rund(p), b = rund(p + gr[i]);
            p += gr[i] + l.abstand;
            if (k[i]->spacer) { continue; }
            const Rechteck w = wunsch(*k[i]);
            Rechteck r;
            float qp = 0, qg = 0;
            if (waagrecht) {
                quer(querA, in.y, in.h, w.h, qp, qg);
                r = Rechteck{a, rund(qp), b - a, rund(qp + qg) - rund(qp)};
            } else {
                quer(querA, in.x, in.w, std::max(w.w, text(*k[i])), qp, qg);
                r = Rechteck{rund(qp), a, rund(qp + qg) - rund(qp), b - a};
            }
            setze(*k[i], r);
        }
    }

    void raster(const Layout& l, const Rechteck& in) {
        const std::vector<const LayoutKind*> k = sichtbare(l);
        if (k.empty()) { return; }
        const int n = std::max(1, l.spalten);
        const int zeilen = (static_cast<int>(k.size()) + n - 1) / n;
        const float zw = (in.w - l.abstand * static_cast<float>(n - 1)) / static_cast<float>(n);
        const float zh = (in.h - l.abstand * static_cast<float>(zeilen - 1)) / static_cast<float>(zeilen);
        for (std::size_t i = 0; i < k.size(); ++i) {
            if (k[i]->spacer) { continue; }
            const int sp = static_cast<int>(i) % n, ze = static_cast<int>(i) / n;
            const float x0 = in.x + static_cast<float>(sp) * (zw + l.abstand);
            const float y0 = in.y + static_cast<float>(ze) * (zh + l.abstand);
            const Rechteck w = wunsch(*k[i]);
            float xp = 0, xg = 0, yp = 0, yg = 0;
            quer(l.h, x0, zw, std::max(w.w, text(*k[i])), xp, xg);
            quer(l.v, y0, zh, w.h, yp, yg);
            setze(*k[i], Rechteck{rund(xp), rund(yp), rund(xp + xg) - rund(xp), rund(yp + yg) - rund(yp)});
        }
    }

    void teiler(const Layout& l, const Rechteck& in) {
        const std::vector<const LayoutKind*> k = sichtbare(l);
        std::vector<const LayoutKind*> spalten;
        for (const LayoutKind* x : k) {
            if (!x->spacer && spalten.size() < 2) { spalten.push_back(x); }
        }
        const float links = (in.w - l.griff) * std::clamp(l.teilung, 0.0F, 1.0F);
        const float xm = rund(in.x + links), xr = rund(in.x + links + l.griff);
        if (!spalten.empty()) { setze(*spalten[0], Rechteck{rund(in.x), rund(in.y), xm - rund(in.x), rund(in.y + in.h) - rund(in.y)}); }
        if (spalten.size() > 1) { setze(*spalten[1], Rechteck{xr, rund(in.y), rund(in.x + in.w) - xr, rund(in.y + in.h) - rund(in.y)}); }
    }

    void rechne(const Layout& l, const Rechteck* vorgabe) {
        if (!fertig.insert(l.name).second) { return; }   // verschachtelt im Kreis: nur einmal
        const Rechteck a = vorgabe != nullptr ? *vorgabe : flaeche(l);
        e.flaechen[l.name] = a;
        const Rechteck in{a.x + l.rand[0], a.y + l.rand[1], std::max(0.0F, a.w - l.rand[0] - l.rand[2]),
                          std::max(0.0F, a.h - l.rand[1] - l.rand[3])};
        switch (l.art) {
            case LayoutArt::Waagrecht: reihe(l, in, true); break;
            case LayoutArt::Senkrecht: reihe(l, in, false); break;
            case LayoutArt::Raster: raster(l, in); break;
            case LayoutArt::Teiler: teiler(l, in); break;
        }
    }
};

std::size_t zeilenAnfang(const std::string& t, std::size_t p) {
    while (p > 0 && t[p - 1] != '\n') { --p; }
    return p;
}

}  // namespace

bool liesLayoutZeile(const std::string& zeile, Layout& out) {
    const std::size_t k = zeile.find(kKennung);
    if (k == std::string::npos) { return false; }
    std::string rest = zeile.substr(k + std::string(kKennung).size());
    while (!rest.empty() && (rest.back() == '\r' || rest.back() == '\n')) { rest.pop_back(); }
    Layout l;
    std::size_t a = 0;
    while (a < rest.size()) {
        while (a < rest.size() && (rest[a] == ' ' || rest[a] == '\t')) { ++a; }
        std::size_t e = a;
        while (e < rest.size() && rest[e] != ' ' && rest[e] != '\t') { ++e; }
        const std::string wort = rest.substr(a, e - a);
        a = e;
        const std::size_t gl = wort.find('=');
        if (gl == std::string::npos) { continue; }
        const std::string s = wort.substr(0, gl), w = wort.substr(gl + 1);
        if (s == "name") { l.name = dekodiere(w); }
        else if (s == "type") {
            l.art = w == "vbox" ? LayoutArt::Senkrecht : w == "grid" ? LayoutArt::Raster : w == "split" ? LayoutArt::Teiler : LayoutArt::Waagrecht;
        } else if (s == "frame") { l.rahmenItem = dekodiere(w); }
        else if (s == "rect") {
            const auto v = teile(w, ',');
            if (v.size() == 4) { l.rahmen = Rechteck{zahl(v[0]), zahl(v[1]), zahl(v[2]), zahl(v[3])}; }
        } else if (s == "spacing") { l.abstand = zahl(w); }
        else if (s == "margins") {
            const auto v = teile(w, ',');
            for (std::size_t i = 0; i < 4 && i < v.size(); ++i) { l.rand[i] = zahl(v[i]); }
            if (v.size() == 1) { l.rand[1] = l.rand[2] = l.rand[3] = l.rand[0]; }
        } else if (s == "halign") { l.h = ausrAus(w); }
        else if (s == "valign") { l.v = ausrAus(w); }
        else if (s == "columns") { l.spalten = std::max(1, std::atoi(w.c_str())); }
        else if (s == "ratio") { l.teilung = std::clamp(zahl(w), 0.0F, 1.0F); }
        else if (s == "handle") { l.griff = std::max(0.0F, zahl(w)); }
        else if (s == "hidden") { l.versteckteAuslassen = w == "skip"; }
        else if (s == "items") {
            for (const std::string& x : teile(w, ',')) {
                if (x.empty()) { continue; }
                LayoutKind kd;
                if (x[0] == '*') {
                    kd.spacer = true;
                    kd.dehnung = x.size() > 1 ? std::max(0.01F, zahl(x.substr(1))) : 1.0F;
                } else if (x[0] == '@') {
                    kd.layout = true;
                    kd.name = dekodiere(x.substr(1));
                } else {
                    kd.name = dekodiere(x);
                }
                l.kinder.push_back(kd);
            }
        }
    }
    if (l.name.empty()) { return false; }
    out = l;
    return true;
}

std::string layoutZeile(const Layout& l) {
    std::string s = std::string(kKennung) + " name=" + kodiere(l.name) + " type=" + artName(l.art);
    if (!l.rahmenItem.empty()) { s += " frame=" + kodiere(l.rahmenItem); }
    else { s += " rect=" + zahlText(l.rahmen.x) + "," + zahlText(l.rahmen.y) + "," + zahlText(l.rahmen.w) + "," + zahlText(l.rahmen.h); }
    if (l.abstand != 0) { s += " spacing=" + zahlText(l.abstand); }
    if (l.rand[0] != 0 || l.rand[1] != 0 || l.rand[2] != 0 || l.rand[3] != 0) {
        s += " margins=" + zahlText(l.rand[0]) + "," + zahlText(l.rand[1]) + "," + zahlText(l.rand[2]) + "," + zahlText(l.rand[3]);
    }
    if (l.h != LayoutAusrichtung::Anfang) { s += std::string(" halign=") + ausrName(l.h); }
    if (l.v != LayoutAusrichtung::Anfang) { s += std::string(" valign=") + ausrName(l.v); }
    if (l.art == LayoutArt::Raster) { s += " columns=" + std::to_string(l.spalten); }
    if (l.art == LayoutArt::Teiler) { s += " ratio=" + zahlText(l.teilung) + " handle=" + zahlText(l.griff); }
    if (l.versteckteAuslassen) { s += " hidden=skip"; }
    s += " items=";
    for (std::size_t i = 0; i < l.kinder.size(); ++i) {
        const LayoutKind& k = l.kinder[i];
        if (i > 0) { s += ","; }
        if (k.spacer) { s += k.dehnung == 1.0F ? std::string("*") : "*" + zahlText(k.dehnung); }
        else if (k.layout) { s += "@" + kodiere(k.name); }
        else { s += kodiere(k.name); }
    }
    return s;
}

std::vector<Layout> liesLayouts(const Datei& d, int menu) {
    std::vector<Layout> v;
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return v; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const std::string& t = d.text;
    std::size_t p = m.anfang;
    const std::size_t ende = std::min<std::size_t>(m.ende, t.size());
    while (p < ende) {
        const std::size_t k = t.find(kKennung, p);
        if (k == std::string::npos || k >= ende) { break; }
        std::size_t ze = t.find('\n', k);
        ze = ze == std::string::npos ? t.size() : ze + 1;
        Layout l;
        if (liesLayoutZeile(t.substr(k, ze - k), l)) {
            // eine eigene Zeile: ab Zeilenanfang, wenn davor nur Einrueckung steht
            const std::size_t za = zeilenAnfang(t, k);
            bool nurLeer = true;
            for (std::size_t i = za; i < k; ++i) { nurLeer = nurLeer && (t[i] == ' ' || t[i] == '\t'); }
            l.anfang = static_cast<std::uint32_t>(nurLeer ? za : k);
            l.ende = static_cast<std::uint32_t>(ze);
            v.push_back(l);
        }
        p = ze;
    }
    return v;
}

LayoutErgebnis rechneLayouts(const std::vector<Layout>& ls, const LayoutGroessen& g) {
    LayoutErgebnis e;
    Rechner r{ls, g, e, {}};
    std::set<std::string> innen;
    for (const Layout& l : ls) {
        for (const LayoutKind& k : l.kinder) {
            if (k.layout) { innen.insert(k.name); }
        }
    }
    for (const Layout& l : ls) {
        if (innen.count(l.name) == 0) { r.rechne(l, nullptr); }
    }
    for (const Layout& l : ls) { r.rechne(l, nullptr); }   // nur verschachtelte im Kreis bleiben uebrig
    return e;
}

bool layoutSchreiben(const Datei& d, int menu, const Layout& l, bool loeschen, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const std::string& t = d.text;
    for (const Layout& alt : liesLayouts(d, menu)) {
        if (alt.name != l.name) { continue; }
        if (loeschen) {
            out.push_back(Ersetzung{alt.anfang, alt.ende, ""});
            return true;
        }
        // Einrueckung und Zeilenende der alten Zeile behalten
        const std::size_t k = t.find(kKennung, alt.anfang);
        const std::string einr = t.substr(alt.anfang, k - alt.anfang);
        std::string nl;
        if (alt.ende > 0 && t[alt.ende - 1] == '\n') { nl = (alt.ende > 1 && t[alt.ende - 2] == '\r') ? "\r\n" : "\n"; }
        out.push_back(Ersetzung{alt.anfang, alt.ende, einr + layoutZeile(l) + nl});
        return true;
    }
    if (loeschen) { return false; }
    // neu: vor das erste itemDef (eigene Zeile mit dessen Einrueckung), sonst vor die } des Menues
    std::size_t ziel = m.items.empty() ? (m.ende > 0 ? m.ende - 1 : 0) : m.items[0].anfang;
    if (ziel >= t.size()) { return false; }
    const std::size_t za = zeilenAnfang(t, ziel);
    bool nurLeer = true;
    for (std::size_t i = za; i < ziel; ++i) { nurLeer = nurLeer && (t[i] == ' ' || t[i] == '\t'); }
    const std::size_t nlPos = t.find('\n');
    const std::string nl = (nlPos != std::string::npos && nlPos > 0 && t[nlPos - 1] == '\r') ? "\r\n" : "\n";
    if (nurLeer) {
        out.push_back(Ersetzung{static_cast<std::uint32_t>(za), static_cast<std::uint32_t>(za), t.substr(za, ziel - za) + layoutZeile(l) + nl});
    } else {
        out.push_back(Ersetzung{static_cast<std::uint32_t>(ziel), static_cast<std::uint32_t>(ziel), layoutZeile(l) + nl});
    }
    return true;
}

int layoutVon(const std::vector<Layout>& ls, const std::string& item) {
    for (std::size_t i = 0; i < ls.size(); ++i) {
        if (ls[i].rahmenItem == item) { return static_cast<int>(i); }
        for (const LayoutKind& k : ls[i].kinder) {
            if (!k.spacer && !k.layout && k.name == item) { return static_cast<int>(i); }
        }
    }
    return -1;
}

std::string freierLayoutName(const std::vector<Layout>& ls) {
    for (int n = 1;; ++n) {
        const std::string s = "layout" + std::to_string(n);
        if (std::none_of(ls.begin(), ls.end(), [&](const Layout& l) { return l.name == s; })) { return s; }
    }
}

}  // namespace bhed::menue
