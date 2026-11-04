// fensteranordnung.cpp - die Anordnung der Fenster je Bereich (Stufe dock1)
//
// Zeilenform:  dock1|d=<id>,<id>|v=<id>|i=<Ini-Text>
//   d  Panels in eigenem Fenster, v ausgeblendete Panels, i der Auszug aus
//   ImGuis Ini-Text mit "\n" fuer Zeilenumbrueche und "\\" fuer "\". Der
//   Ini-Text steht zuletzt und reicht bis zum Zeilenende - ein '|' darin
//   (Fenstertitel) braucht deshalb kein Fluchtzeichen.
#include "bhed/fensteranordnung.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace bhed::fenster {

namespace {

constexpr const char* kKopf = "dock1";

bool kennungGueltig(const std::string& id) {
    if (id.empty() || id.size() > 64) { return false; }
    return std::all_of(id.begin(), id.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    });
}

std::string liste(const std::vector<std::string>& l) {
    std::string s;
    for (const std::string& id : l) {
        if (!kennungGueltig(id)) { continue; }
        if (!s.empty()) { s += ','; }
        s += id;
    }
    return s;
}

bool listeLesen(const std::string& s, std::vector<std::string>& out) {
    out.clear();
    std::size_t a = 0;
    while (a < s.size()) {
        std::size_t e = s.find(',', a);
        if (e == std::string::npos) { e = s.size(); }
        const std::string id = s.substr(a, e - a);
        if (!kennungGueltig(id)) { return false; }
        einfuegen(out, id);
        a = e + 1;
    }
    return true;
}

std::string iniEinpacken(const std::string& ini) {
    std::string s;
    s.reserve(ini.size());
    for (const char c : ini) {
        if (c == '\\') {
            s += "\\\\";
        } else if (c == '\n') {
            s += "\\n";
        } else if (c != '\r') {
            s += c;
        }
    }
    return s;
}

bool iniAuspacken(const std::string& s, std::string& out) {
    out.clear();
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '\\') {
            out += s[i];
            continue;
        }
        if (i + 1 >= s.size()) { return false; }
        const char n = s[++i];
        if (n == 'n') {
            out += '\n';
        } else if (n == '\\') {
            out += '\\';
        } else {
            return false;
        }
    }
    return true;
}

}  // namespace

void einfuegen(std::vector<std::string>& l, const std::string& id) {
    const auto it = std::lower_bound(l.begin(), l.end(), id);
    if (it == l.end() || *it != id) { l.insert(it, id); }
}

void entfernen(std::vector<std::string>& l, const std::string& id) {
    const auto it = std::lower_bound(l.begin(), l.end(), id);
    if (it != l.end() && *it == id) { l.erase(it); }
}

bool enthaelt(const std::vector<std::string>& l, const std::string& id) { return std::binary_search(l.begin(), l.end(), id); }

std::string kodieren(const Anordnung& a) {
    return std::string(kKopf) + "|d=" + liste(a.draussen) + "|v=" + liste(a.versteckt) + "|i=" + iniEinpacken(a.ini);
}

bool dekodieren(const std::string& zeile, Anordnung& a) {
    a = Anordnung{};
    const std::string kopf = std::string(kKopf) + "|d=";
    if (zeile.compare(0, kopf.size(), kopf) != 0) { return false; }
    const std::size_t v = zeile.find("|v=", kopf.size());
    if (v == std::string::npos) { return false; }
    const std::size_t i = zeile.find("|i=", v + 3);
    if (i == std::string::npos) { return false; }
    Anordnung neu;
    if (!listeLesen(zeile.substr(kopf.size(), v - kopf.size()), neu.draussen) ||
        !listeLesen(zeile.substr(v + 3, i - v - 3), neu.versteckt) || !iniAuspacken(zeile.substr(i + 3), neu.ini)) {
        return false;
    }
    a = std::move(neu);
    return true;
}

namespace {

// Ein [Window]-Abschnitt: Kopfzeile, Wertzeilen, Dock-Knoten (0 = keiner).
struct IniFenster {
    std::string kopf;
    std::vector<std::string> zeilen;
    unsigned dock = 0;
};
// Ein Dock-Baum aus [Docking][Data]: die Wurzelzeile (ohne Parent=) und die
// eingerueckten Zeilen ihrer Kinder, wie DockSettingsHandler_WriteAll sie
// schreibt (Tiefe zuerst).
struct IniBaum {
    std::vector<std::string> zeilen;
    std::vector<unsigned> knoten;
};
struct IniTeile {
    std::vector<IniFenster> fenster;
    std::vector<IniBaum> baeume;
};

// "0x0000002A" an der Stelle `at` lesen; false ohne Zahl.
bool hexLesen(const std::string& s, std::size_t at, unsigned& wert, std::size_t& ende) {
    if (s.compare(at, 2, "0x") != 0) { return false; }
    std::size_t i = at + 2;
    unsigned v = 0;
    while (i < s.size() && std::isxdigit(static_cast<unsigned char>(s[i])) != 0) {
        const int c = std::tolower(static_cast<unsigned char>(s[i]));
        v = v * 16U + static_cast<unsigned>(c <= '9' ? c - '0' : c - 'a' + 10);
        ++i;
    }
    if (i == at + 2) { return false; }
    wert = v;
    ende = i;
    return true;
}

std::string hex(unsigned v) {
    char z[16];
    std::snprintf(z, sizeof(z), "0x%08X", v);
    return z;
}

// Der Wert hinter `schluessel` ("ID=", "Parent=") in einer Knotenzeile, nur an
// einer Wortgrenze.
bool knotenWert(const std::string& zeile, const char* schluessel, unsigned& wert, std::size_t& at, std::size_t& ende) {
    const std::string k = schluessel;
    for (std::size_t p = zeile.find(k); p != std::string::npos; p = zeile.find(k, p + 1)) {
        if (p > 0 && zeile[p - 1] != ' ') { continue; }
        if (hexLesen(zeile, p + k.size(), wert, ende)) {
            at = p + k.size();
            return true;
        }
    }
    return false;
}

IniTeile zerlegen(const std::string& ini) {
    IniTeile t;
    std::istringstream in(ini);
    std::string zeile;
    enum class Wo { Sonst, Fenster, Dock };
    Wo wo = Wo::Sonst;
    while (std::getline(in, zeile)) {
        if (!zeile.empty() && zeile.back() == '\r') { zeile.pop_back(); }
        if (!zeile.empty() && zeile.front() == '[') {
            if (zeile.rfind("[Window][", 0) == 0) {
                wo = Wo::Fenster;
                t.fenster.push_back(IniFenster{zeile, {}, 0});
            } else {
                wo = zeile == "[Docking][Data]" ? Wo::Dock : Wo::Sonst;
            }
            continue;
        }
        if (zeile.find_first_not_of(' ') == std::string::npos) { continue; }
        if (wo == Wo::Fenster) {
            IniFenster& f = t.fenster.back();
            std::size_t ende = 0;
            if (zeile.rfind("DockId=", 0) == 0) { (void)hexLesen(zeile, 7, f.dock, ende); }
            f.zeilen.push_back(zeile);
        } else if (wo == Wo::Dock) {
            unsigned id = 0;
            unsigned eltern = 0;
            std::size_t at = 0;
            std::size_t ende = 0;
            if (!knotenWert(zeile, "ID=", id, at, ende)) { continue; }
            const bool kind = knotenWert(zeile, "Parent=", eltern, at, ende);
            if (!kind || t.baeume.empty()) { t.baeume.push_back(IniBaum{}); }
            t.baeume.back().zeilen.push_back(zeile);
            t.baeume.back().knoten.push_back(id);
        }
    }
    return t;
}

std::string zusammensetzen(const IniTeile& t) {
    std::string out;
    for (const IniFenster& f : t.fenster) {
        out += f.kopf + "\n";
        for (const std::string& z : f.zeilen) { out += z + "\n"; }
        out += "\n";
    }
    if (!t.baeume.empty()) {
        out += "[Docking][Data]\n";
        for (const IniBaum& b : t.baeume) {
            for (const std::string& z : b.zeilen) { out += z + "\n"; }
        }
        out += "\n";
    }
    return out;
}

// Gehoert der Abschnitt zu einem Panel des Bereichs? ImGui schreibt den Namen
// ab "###" (ImHashSkipUncontributingPrefix: "[Window][dock.mission.fluss]"),
// mit io.ConfigDebugIniSettings den ganzen ("[Window][Titel###dock...]").
bool zumBereich(const IniFenster& f, const std::string& praefix) {
    const std::string kopf = "[Window][";
    if (f.kopf.size() < kopf.size() + 1 || f.kopf.back() != ']') { return false; }
    std::string name = f.kopf.substr(kopf.size(), f.kopf.size() - kopf.size() - 1);
    const std::size_t marke = name.rfind("###");
    if (marke != std::string::npos) { name = name.substr(marke + 3); }
    return name.rfind(std::string(kIniKennung) + praefix, 0) == 0;
}

bool baumMitKnoten(const IniBaum& b, const std::vector<unsigned>& knoten) {
    return std::any_of(b.knoten.begin(), b.knoten.end(),
                       [&](unsigned k) { return std::find(knoten.begin(), knoten.end(), k) != knoten.end(); });
}

}  // namespace

std::string iniFiltern(const std::string& ini, const std::string& praefix) {
    IniTeile t = zerlegen(ini);
    t.fenster.erase(std::remove_if(t.fenster.begin(), t.fenster.end(), [&](const IniFenster& f) { return !zumBereich(f, praefix); }),
                    t.fenster.end());
    if (!praefix.empty()) {
        std::vector<unsigned> knoten;
        for (const IniFenster& f : t.fenster) {
            if (f.dock != 0) { knoten.push_back(f.dock); }
        }
        t.baeume.erase(std::remove_if(t.baeume.begin(), t.baeume.end(), [&](const IniBaum& b) { return !baumMitKnoten(b, knoten); }),
                       t.baeume.end());
    }
    return zusammensetzen(t);
}

std::string iniErsetzen(const std::string& basis, const std::string& teil, const std::string& praefix) {
    IniTeile b = zerlegen(basis);
    IniTeile t = zerlegen(iniFiltern(teil, praefix));
    // Aus der Basis: die Fenster des Bereichs und die Baeume, in denen sie stecken.
    std::vector<unsigned> weg;
    for (const IniFenster& f : b.fenster) {
        if (zumBereich(f, praefix) && f.dock != 0) { weg.push_back(f.dock); }
    }
    b.fenster.erase(std::remove_if(b.fenster.begin(), b.fenster.end(), [&](const IniFenster& f) { return zumBereich(f, praefix); }),
                    b.fenster.end());
    b.baeume.erase(std::remove_if(b.baeume.begin(), b.baeume.end(), [&](const IniBaum& x) { return baumMitKnoten(x, weg); }),
                   b.baeume.end());
    // Vergebene Nummern der Basis; Kollisionen aus dem Teil neu nummerieren.
    std::vector<unsigned> belegt;
    unsigned hoechste = 0;
    for (const IniBaum& x : b.baeume) {
        for (const unsigned k : x.knoten) {
            belegt.push_back(k);
            hoechste = std::max(hoechste, k);
        }
    }
    for (const IniBaum& x : t.baeume) {
        for (const unsigned k : x.knoten) { hoechste = std::max(hoechste, k); }
    }
    std::map<unsigned, unsigned> neu;
    for (const IniBaum& x : t.baeume) {
        for (const unsigned k : x.knoten) {
            if (std::find(belegt.begin(), belegt.end(), k) != belegt.end() && neu.count(k) == 0U) { neu[k] = ++hoechste; }
        }
    }
    const auto umschreiben = [&](std::string& zeile, const char* schluessel) {
        unsigned w = 0;
        std::size_t at = 0;
        std::size_t ende = 0;
        if (knotenWert(zeile, schluessel, w, at, ende) && neu.count(w) != 0U) { zeile.replace(at, ende - at, hex(neu[w])); }
    };
    for (IniBaum& x : t.baeume) {
        for (std::string& z : x.zeilen) {
            umschreiben(z, "ID=");
            umschreiben(z, "Parent=");
        }
        for (unsigned& k : x.knoten) {
            if (neu.count(k) != 0U) { k = neu[k]; }
        }
        b.baeume.push_back(x);
    }
    for (IniFenster& f : t.fenster) {
        if (f.dock != 0 && neu.count(f.dock) != 0U) {
            f.dock = neu[f.dock];
            for (std::string& z : f.zeilen) {
                unsigned w = 0;
                std::size_t ende = 0;
                if (z.rfind("DockId=", 0) == 0 && hexLesen(z, 7, w, ende)) { z.replace(7, ende - 7, hex(f.dock)); }
            }
        }
        b.fenster.push_back(f);
    }
    return zusammensetzen(b);
}

std::string nameBereinigen(const std::string& name) {
    std::string s;
    for (const char c : name) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20 || c == '=' || c == '/' || c == '|' || c == 0x7F) { continue; }
        s += c;
    }
    const std::size_t a = s.find_first_not_of(' ');
    if (a == std::string::npos) { return {}; }
    s = s.substr(a, s.find_last_not_of(' ') - a + 1);
    if (s.size() > 48) {
        // Nicht mitten in einem UTF-8-Zeichen abschneiden.
        std::size_t n = 48;
        while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0U) == 0x80U) { --n; }
        s.resize(n);
    }
    return s;
}

// --- rc609 T1: Tabs in eigenen Fenstern ----------------------------------------

namespace {
constexpr const char* kTabKopf = "tab1";

bool zahlenLesen(const std::string& teil, int (&z)[9]) {
    std::size_t pos = 0;
    for (int i = 0; i < 9; ++i) {
        const std::size_t ende = (i < 8) ? teil.find(',', pos) : teil.size();
        if (ende == std::string::npos || ende <= pos || ende - pos > 8) { return false; }
        int w = 0;
        bool minus = false;
        for (std::size_t k = pos; k < ende; ++k) {
            const char c = teil[k];
            if (k == pos && c == '-' && ende - pos > 1) {
                minus = true;
            } else if (c >= '0' && c <= '9') {
                w = w * 10 + (c - '0');
            } else {
                return false;
            }
        }
        if (w > 1000000) { return false; }   // weit ausserhalb jedes Bildschirms
        z[i] = minus ? -w : w;
        pos = ende + 1;
    }
    return true;
}

bool enthaeltPunkt(const Rechteck& r, int x, int y) { return x >= r.x && y >= r.y && x < r.x + r.b && y < r.y + r.h; }
}  // namespace

std::string schluesselTabBenannt(int bereich, const std::string& name) { return "t" + std::to_string(bereich) + "/" + name; }

std::string tabFensterKodieren(const std::vector<TabFenster>& liste) {
    std::string s = kTabKopf;
    for (const TabFenster& f : liste) {
        char z[160];
        std::snprintf(z, sizeof(z), "|%d,%d,%d,%d,%d,%d,%d,%d,%d", f.modus, f.lage.x, f.lage.y, f.lage.b, f.lage.h, f.bildschirm.x,
                      f.bildschirm.y, f.bildschirm.b, f.bildschirm.h);
        s += z;
    }
    return s;
}

bool tabFensterDekodieren(const std::string& zeile, std::vector<TabFenster>& liste) {
    liste.clear();
    const std::string kopf = kTabKopf;
    if (zeile.compare(0, kopf.size(), kopf) != 0 || (zeile.size() > kopf.size() && zeile[kopf.size()] != '|')) { return false; }
    std::vector<TabFenster> neu;
    std::size_t pos = kopf.size();
    while (pos < zeile.size()) {
        const std::size_t ende = std::min(zeile.find('|', pos + 1), zeile.size());
        int z[9] = {};
        if (!zahlenLesen(zeile.substr(pos + 1, ende - pos - 1), z)) { return false; }
        TabFenster f;
        f.modus = z[0];
        f.lage = Rechteck{z[1], z[2], z[3], z[4]};
        f.bildschirm = Rechteck{z[5], z[6], z[7], z[8]};
        if (f.modus < 0 || f.modus >= kTabBereiche || f.lage.b <= 0 || f.lage.h <= 0 || f.bildschirm.b < 0 || f.bildschirm.h < 0) { return false; }
        if (std::any_of(neu.begin(), neu.end(), [&](const TabFenster& g) { return g.modus == f.modus; })) { return false; }
        neu.push_back(f);
        pos = ende;
    }
    if (neu.size() >= static_cast<std::size_t>(kTabBereiche)) { return false; }
    liste = std::move(neu);
    return true;
}

TabFenster aufBildschirm(const TabFenster& f, const std::vector<Rechteck>& bildschirme, const Rechteck& ersatz) {
    TabFenster n = f;
    const int mx = f.lage.x + f.lage.b / 2;
    const int my = f.lage.y + f.lage.h / 2;
    const Rechteck* ziel = nullptr;
    for (const Rechteck& r : bildschirme) {
        if (ziel == nullptr && r == f.bildschirm && enthaeltPunkt(r, mx, my)) { ziel = &r; }
    }
    for (const Rechteck& r : bildschirme) {
        if (ziel == nullptr && enthaeltPunkt(r, mx, my)) { ziel = &r; }
    }
    const Rechteck z = (ziel != nullptr) ? *ziel : ersatz;
    n.lage.b = std::max(1, std::min(n.lage.b, z.b));
    n.lage.h = std::max(1, std::min(n.lage.h, z.h));
    if (ziel == nullptr) {
        n.lage.x = z.x + (z.b - n.lage.b) / 2;
        n.lage.y = z.y + (z.h - n.lage.h) / 2;
    }
    n.lage.x = std::clamp(n.lage.x, z.x, std::max(z.x, z.x + z.b - n.lage.b));
    n.lage.y = std::clamp(n.lage.y, z.y, std::max(z.y, z.y + z.h - n.lage.h));
    n.bildschirm = z;
    return n;
}

std::string schluesselLaufend(int bereich) { return "b" + std::to_string(bereich); }

std::string schluesselBenannt(int bereich, const std::string& name) { return schluesselLaufend(bereich) + "/" + name; }

std::vector<std::string> benannte(const std::map<std::string, std::string>& fenster, int bereich) {
    const std::string p = schluesselLaufend(bereich) + "/";
    std::vector<std::string> namen;
    for (auto it = fenster.lower_bound(p); it != fenster.end() && it->first.compare(0, p.size(), p) == 0; ++it) {
        namen.push_back(it->first.substr(p.size()));
    }
    return namen;
}

}  // namespace bhed::fenster
