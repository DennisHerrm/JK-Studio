// stile.cpp - Menue-Editor M6 Punkt 4: Stile zentral (siehe stile.h).
//
// Eigener Code.
#include "bhed/menue/stile.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string zahl(const std::string& s) {
    char b[48];
    std::snprintf(b, sizeof(b), "%g", std::atof(s.c_str()));
    return b;
}

// Art und einheitlicher Wert einer Eigenschaft; false = gehoert zu keinem Stil
bool stilVon(const Eigenschaft& e, StilArt& art, std::string& wert) {
    const std::string k = klein(e.kanon.empty() ? e.schluessel : e.kanon);
    if (!e.ok || e.args.empty()) { return false; }
    if (k == "forecolor" || k == "backcolor" || k == "bordercolor" || k == "outlinecolor" || k == "focuscolor" || k == "disablecolor" ||
        k == "desccolor") {
        if (e.args.size() != 4) { return false; }
        art = StilArt::Farbe;
        wert.clear();
        for (const Token& t : e.args) { wert += (wert.empty() ? "" : " ") + zahl(t.s); }
        return true;
    }
    if (e.args.size() != 1) { return false; }
    if (k == "font") {
        art = StilArt::Schrift;
        wert = e.args[0].s;
        return true;
    }
    if (k == "textscale") {
        art = StilArt::Textgroesse;
        wert = zahl(e.args[0].s);
        return true;
    }
    if (k == "background") {
        art = StilArt::Hintergrund;
        wert = e.args[0].s;
        return true;
    }
    return false;
}

bool inHauptdatei(const Eigenschaft& e) {
    return std::all_of(e.args.begin(), e.args.end(), [](const Token& t) { return t.datei == 0 && !t.ausMakro; });
}

template <typename F>
void jedeEigenschaft(const Datei& d, F f) {
    for (const Menu& m : d.menus) {
        for (const Eigenschaft& e : m.eigenschaften) { f(e); }
        for (const Item& it : m.items) {
            for (const Eigenschaft& e : it.eigenschaften) { f(e); }
        }
    }
}

}  // namespace

std::vector<StilWert> stileImMenue(const Datei& d) {
    std::map<std::pair<int, std::string>, StilWert> m;
    jedeEigenschaft(d, [&](const Eigenschaft& e) {
        StilArt art{};
        std::string wert;
        if (!stilVon(e, art, wert)) { return; }
        StilWert& w = m[{static_cast<int>(art), wert}];
        w.art = art;
        w.wert = wert;
        const std::string k = klein(e.kanon.empty() ? e.schluessel : e.kanon);
        if (std::find(w.schluessel.begin(), w.schluessel.end(), k) == w.schluessel.end()) { w.schluessel.push_back(k); }
        if (inHauptdatei(e)) {
            ++w.anzahl;
        } else {
            ++w.gesperrt;
        }
    });
    std::vector<StilWert> v;
    for (auto& [k, w] : m) { v.push_back(std::move(w)); }
    std::stable_sort(v.begin(), v.end(), [](const StilWert& a, const StilWert& b) {
        if (a.art != b.art) { return static_cast<int>(a.art) < static_cast<int>(b.art); }
        return a.anzahl + a.gesperrt > b.anzahl + b.gesperrt;
    });
    return v;
}

bool stilErsetzen(const Datei& d, const StilWert& w, const std::vector<std::string>& neu, std::vector<Ersetzung>& out) {
    bool eine = false;
    jedeEigenschaft(d, [&](const Eigenschaft& e) {
        StilArt art{};
        std::string wert;
        if (!stilVon(e, art, wert) || art != w.art || wert != w.wert || !inHauptdatei(e) || e.args.size() != neu.size()) { return; }
        for (std::size_t i = 0; i < neu.size(); ++i) {
            const Token& t = e.args[i];
            const bool zitiert = t.anfang < d.text.size() && d.text[t.anfang] == '"';
            const bool braucht = neu[i].empty() || neu[i].find_first_of(" \t") != std::string::npos;
            out.push_back(Ersetzung{t.anfang, t.ende, (zitiert || braucht) ? "\"" + neu[i] + "\"" : neu[i]});
        }
        eine = true;
    });
    return eine;
}

}  // namespace bhed::menue
