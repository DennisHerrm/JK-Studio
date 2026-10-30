// diagnose.cpp - Menue-Editor M2: Live-Diagnose (siehe diagnose.h).
// Portiert aus OpenJK (die Regeln der Skriptbefehle, code/ui/ui_shared.cpp; code/ui/menudef.h); sonst eigener Code.
//
// Was die Engine liest, steht in menue::lies (parser.cpp); hier wird nur
// ausgewertet. Script-Befehle und ihre Suche nach Items: OpenJK
// code/ui/ui_shared.cpp Script_Show/Script_Hide/Script_FadeIn/
// Script_FadeOut/Script_SetItemColor/Script_Transition/Script_Orbit ->
// Menu_ItemsMatchingGroup (Name oder Gruppe ohne Gross/Klein, "name*" =
// Anfang mit Gross/Klein), Script_SetFocus -> Menu_FindItemByName, Script_Open/
// Script_Close -> Menus_FindByName.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/menue/diagnose.h"

#include "bhed/menue/skript.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>

namespace bhed::menue {

namespace {
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

struct Zeilen {
    std::vector<std::uint32_t> anfang;   // Byte, an dem jede Zeile beginnt
    explicit Zeilen(const std::string& t) {
        anfang.push_back(0);
        for (std::size_t i = 0; i < t.size(); ++i) {
            if (t[i] == '\n') { anfang.push_back(static_cast<std::uint32_t>(i + 1)); }
        }
    }
    [[nodiscard]] int zeileVon(std::uint32_t off) const {   // 1-basiert
        return static_cast<int>(std::upper_bound(anfang.begin(), anfang.end(), off) - anfang.begin());
    }
};

// Einrueckung einer Zeile (Tab = 4)
int einrueckung(const std::string& t, std::uint32_t zeilenAnfang) {
    int n = 0;
    for (std::size_t i = zeilenAnfang; i < t.size(); ++i) {
        if (t[i] == ' ') { ++n; }
        else if (t[i] == '\t') { n = (n / 4 + 1) * 4; }
        else { break; }
    }
    return n;
}
// leer oder nur Kommentar?
bool leereZeile(const std::string& t, std::uint32_t a) {
    std::size_t i = a;
    while (i < t.size() && (t[i] == ' ' || t[i] == '\t' || t[i] == '\r')) { ++i; }
    return i >= t.size() || t[i] == '\n' || t.compare(i, 2, "//") == 0;
}

// OpenJK code/ui/menudef.h (SP): was ein Name bedeuten soll, den SP als 0 liest
struct Define {
    const char* name;
    long wert;
};
const Define kMenudefSp[] = {
#include "menudef_sp.inc"
};
const Define* spDefine(const std::string& n) {
    for (const Define& d : kMenudefSp) {
        if (n == d.name) { return &d; }
    }
    return nullptr;
}

bool istZahl(const std::string& s) {
    if (s.empty()) { return false; }
    char* ende = nullptr;
    (void)std::strtod(s.c_str(), &ende);
    return ende != nullptr && *ende == '\0';
}

// --- Klammern und Anfuehrungszeichen -------------------------------------------------
struct Klammer {
    char c = 0;
    std::uint32_t pos = 0;
    int partner = -1;   // Index der Gegenklammer
};

void klammern(const std::string& t, const Zeilen& z, bool mp, std::vector<Diagnose>& aus) {
    std::vector<Klammer> k;
    std::vector<int> stapel;
    std::vector<Diagnose> falsch;
    for (std::size_t i = 0; i < t.size(); ++i) {
        const char c = t[i];
        if (c == '/' && i + 1 < t.size() && t[i + 1] == '/') {
            while (i < t.size() && t[i] != '\n') { ++i; }
            continue;
        }
        if (c == '/' && i + 1 < t.size() && t[i + 1] == '*') {
            const std::size_t e = t.find("*/", i + 2);
            i = e == std::string::npos ? t.size() : e + 1;
            continue;
        }
        if (c == '"') {
            // SP (COM_ParseExt) liest bis zum naechsten " auch ueber Zeilen; MP (botlib) bricht
            // am Zeilenende ab - beides fast immer ein vergessenes Zeichen.
            std::size_t e = i + 1;
            while (e < t.size() && t[e] != '"') {
                if (mp && t[e] == '\\' && e + 1 < t.size()) { ++e; }   // botlib: \" im Text
                ++e;
            }
            const std::size_t nl = t.find('\n', i);
            if (e >= t.size() || (nl != std::string::npos && nl < e)) {
                Diagnose d;
                d.art = DiagArt::AnfuehrungOffen;
                d.schwere = (e >= t.size() || mp) ? Schwere::Fehler : Schwere::Warnung;
                d.anfang = static_cast<std::uint32_t>(i);
                d.ende = d.anfang + 1;
                d.zeile = z.zeileVon(d.anfang);
                if (e < t.size()) {
                    d.partner = true;
                    d.partnerAnfang = static_cast<std::uint32_t>(e);
                    d.partnerEnde = d.partnerAnfang + 1;
                    d.wert = std::to_string(z.zeileVon(d.partnerAnfang));
                }
                // Korrektur: das " am Ende dieser Zeile ergaenzen
                std::size_t zeilenEnde = nl == std::string::npos ? t.size() : nl;
                if (zeilenEnde > i && t[zeilenEnde - 1] == '\r') { --zeilenEnde; }
                d.korrekturen.push_back(Korrektur{static_cast<std::uint32_t>(zeilenEnde), static_cast<std::uint32_t>(zeilenEnde), "\""});
                aus.push_back(d);
                if (e >= t.size()) { return; }
            }
            i = e;
            continue;
        }
        if (c == '{' || c == '(') {
            stapel.push_back(static_cast<int>(k.size()));
            k.push_back(Klammer{c, static_cast<std::uint32_t>(i), -1});
        } else if (c == '}' || c == ')') {
            const int me = static_cast<int>(k.size());
            k.push_back(Klammer{c, static_cast<std::uint32_t>(i), -1});
            if (stapel.empty()) { continue; }   // unten als ueberzaehlig gemeldet
            const int o = stapel.back();
            stapel.pop_back();
            k[static_cast<std::size_t>(o)].partner = me;
            k[static_cast<std::size_t>(me)].partner = o;
            if ((c == '}') != (k[static_cast<std::size_t>(o)].c == '{')) {
                Diagnose d;
                d.art = DiagArt::KlammerFalsch;
                d.schwere = Schwere::Fehler;
                d.anfang = static_cast<std::uint32_t>(i);
                d.ende = d.anfang + 1;
                d.zeile = z.zeileVon(d.anfang);
                d.partner = true;
                d.partnerAnfang = k[static_cast<std::size_t>(o)].pos;
                d.partnerEnde = d.partnerAnfang + 1;
                d.text = std::string(1, k[static_cast<std::size_t>(o)].c);
                d.wert = std::string(1, c);
                falsch.push_back(d);
            }
        }
    }
    aus.insert(aus.end(), falsch.begin(), falsch.end());
    int offen = 0, zu = 0;
    for (const Klammer& x : k) {
        if (x.partner < 0) { ((x.c == '{' || x.c == '(') ? offen : zu) += 1; }
    }
    if (offen == 0 && zu == 0) { return; }
    auto zeilenAnfang = [&](std::uint32_t off) { return z.anfang[static_cast<std::size_t>(z.zeileVon(off) - 1)]; };
    auto ein = [&](std::uint32_t off) { return einrueckung(t, zeilenAnfang(off)); };
    // Wo die Einrueckung einer Klammer widerspricht, liegt der Fehler meist dort - nicht an
    // der Stelle, an der der Stapel am Ende uebrig bleibt.
    // Fehlt eine schliessende, ist es die letzte oeffnende mit falsch eingerueckter Gegenklammer
    // (alles danach ist richtig gepaart); ist eine zu viel, die erste schliessende.
    auto schief = [&](std::size_t i) {
        const Klammer& x = k[i];
        if (x.partner < 0 || z.zeileVon(x.pos) == z.zeileVon(k[static_cast<std::size_t>(x.partner)].pos)) { return false; }
        return ein(x.pos) != ein(k[static_cast<std::size_t>(x.partner)].pos);
    };
    int verdacht = -1;
    if (offen > 0) {
        for (std::size_t i = k.size(); i-- > 0;) {
            if ((k[i].c == '{' || k[i].c == '(') && schief(i)) {
                verdacht = static_cast<int>(i);
                break;
            }
        }
    }
    int verdachtZu = -1;
    for (std::size_t i = 0; i < k.size(); ++i) {
        if ((k[i].c == '}' || k[i].c == ')') && schief(i)) {
            verdachtZu = static_cast<int>(i);
            break;
        }
    }
    if (offen > 0) {
        // die verdaechtige oeffnende (sonst die uebrig gebliebenen)
        std::vector<int> melden;
        if (verdacht >= 0 && (k[static_cast<std::size_t>(verdacht)].c == '{' || k[static_cast<std::size_t>(verdacht)].c == '(') && offen == 1) {
            melden.push_back(verdacht);
        } else {
            for (std::size_t i = 0; i < k.size(); ++i) {
                if (k[i].partner < 0 && (k[i].c == '{' || k[i].c == '(')) { melden.push_back(static_cast<int>(i)); }
            }
        }
        for (const int i : melden) {
            const Klammer& x = k[static_cast<std::size_t>(i)];
            Diagnose d;
            d.art = DiagArt::KlammerOffen;
            d.schwere = Schwere::Fehler;
            d.anfang = x.pos;
            d.ende = x.pos + 1;
            d.zeile = z.zeileVon(x.pos);
            d.text = std::string(1, x.c);
            // Wo endet der Block nach der Einrueckung? Erste nicht leere Zeile danach, die nicht
            // tiefer steht als die Zeile der Klammer (und nicht selbst schliesst).
            const int e0 = ein(x.pos);
            const std::size_t zl = static_cast<std::size_t>(d.zeile);   // Index der naechsten Zeile
            bool tiefer = false;
            std::uint32_t ziel = static_cast<std::uint32_t>(t.size());
            bool gefunden = false;
            for (std::size_t q = zl; q < z.anfang.size(); ++q) {
                const std::uint32_t a = z.anfang[q];
                if (leereZeile(t, a)) { continue; }
                const int e = einrueckung(t, a);
                if (!tiefer) {
                    tiefer = e > e0;
                    if (!tiefer) { break; }
                    continue;
                }
                if (e <= e0) {
                    std::size_t p = a;
                    while (p < t.size() && (t[p] == ' ' || t[p] == '\t')) { ++p; }
                    if (p < t.size() && (t[p] == '}' || t[p] == ')') && e == e0) { break; }   // dort schliesst sie schon
                    ziel = a;
                    gefunden = true;
                    break;
                }
            }
            if (tiefer) {
                d.partner = true;
                d.partnerAnfang = ziel;
                d.partnerEnde = ziel;
                const std::string einzug = t.substr(zeilenAnfang(x.pos), static_cast<std::size_t>(x.pos - zeilenAnfang(x.pos)));
                std::string nurEinzug;
                for (const char ch : einzug) {
                    if (ch != ' ' && ch != '\t') { break; }
                    nurEinzug += ch;
                }
                const std::string zu1 = x.c == '{' ? "}" : ")";
                if (gefunden) { d.korrekturen.push_back(Korrektur{ziel, ziel, nurEinzug + zu1 + "\n"}); }
                else if (offen == 1) { d.korrekturen.push_back(Korrektur{ziel, ziel, (t.empty() || t.back() == '\n' ? "" : "\n") + nurEinzug + zu1 + "\n"}); }
            }
            aus.push_back(d);
        }
    }
    if (zu > 0) {
        std::vector<int> melden;
        if (verdachtZu >= 0 && zu == 1) {
            melden.push_back(verdachtZu);
        } else {
            for (std::size_t i = 0; i < k.size(); ++i) {
                if (k[i].partner < 0 && (k[i].c == '}' || k[i].c == ')')) { melden.push_back(static_cast<int>(i)); }
            }
        }
        for (const int i : melden) {
            const Klammer& x = k[static_cast<std::size_t>(i)];
            Diagnose d;
            d.art = DiagArt::KlammerZuViel;
            d.schwere = Schwere::Fehler;
            d.anfang = x.pos;
            d.ende = x.pos + 1;
            d.zeile = z.zeileVon(x.pos);
            d.text = std::string(1, x.c);
            if (zu == 1) { d.korrekturen.push_back(Korrektur{x.pos, x.pos + 1, ""}); }
            aus.push_back(d);
        }
    }
}

// Die Spanne einer Eigenschaft (Schluessel bis letztes Argument), nur in der Hauptdatei
bool spanne(const Eigenschaft& e, std::uint32_t& a, std::uint32_t& b) {
    if (e.kopf.datei != 0 || e.kopf.ausMakro) { return false; }
    a = e.kopf.anfang;
    b = e.kopf.ende;
    return b > a;
}
bool spanne(const Token& t, std::uint32_t& a, std::uint32_t& b) {
    if (t.datei != 0 || t.ausMakro) { return false; }
    a = t.anfang;
    b = t.ende;
    return b > a;
}

const Eigenschaft* eigenschaft(const std::vector<Eigenschaft>& v, const char* kanon) {
    for (const Eigenschaft& e : v) {
        if (klein(e.kanon) == klein(kanon)) { return &e; }
    }
    return nullptr;
}

// Item/Gruppe wie Menu_ItemsMatchingGroup: "abc*" = Anfang mit Gross/Klein, sonst ohne
bool passtGruppe(const Menu& m, const std::string& ziel, bool nurName) {
    const std::size_t stern = ziel.find('*');
    for (const Item& it : m.items) {
        if (stern != std::string::npos && !nurName) {
            if (it.window.name.compare(0, stern, ziel, 0, stern) == 0 && it.window.name.size() >= stern) { return true; }
            if (it.window.group.compare(0, stern, ziel, 0, stern) == 0 && it.window.group.size() >= stern) { return true; }
            continue;
        }
        if (klein(it.window.name) == klein(ziel)) { return true; }
        if (!nurName && !it.window.group.empty() && klein(it.window.group) == klein(ziel)) { return true; }
    }
    return false;
}
}  // namespace

int abstand(const std::string& a0, const std::string& b0) {
    const std::string a = klein(a0), b = klein(b0);
    std::vector<int> v(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) { v[j] = static_cast<int>(j); }
    for (std::size_t i = 1; i <= a.size(); ++i) {
        int diag = v[0];
        v[0] = static_cast<int>(i);
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const int alt = v[j];
            v[j] = std::min({v[j] + 1, v[j - 1] + 1, diag + (a[i - 1] == b[j - 1] ? 0 : 1)});
            diag = alt;
        }
    }
    return v[b.size()];
}

std::string aehnlichstes(const std::string& wort, const std::vector<std::string>& kandidaten, int maxAbstand) {
    int best = maxAbstand + 1;
    std::string wahl;
    bool gleich = false;
    for (const std::string& k : kandidaten) {
        if (klein(k) == klein(wort)) { return {}; }   // gibt es schon
        const int a = abstand(wort, k);
        if (a < best) {
            best = a;
            wahl = k;
            gleich = false;
        } else if (a == best && klein(k) != klein(wahl)) {
            gleich = true;
        }
    }
    return (best <= maxAbstand && !gleich) ? wahl : std::string();
}

std::vector<Diagnose> diagnose(const Datei& d, const DiagKontext& k) {
    std::vector<Diagnose> aus;
    const std::string& t = d.text;
    const Zeilen zl(t);
    const bool mp = d.dialekt == Dialekt::MP;
    klammern(t, zl, mp, aus);

    // --- Meldungen der Engine, mit der Stelle des Wortes, das sie nennen -----------------
    // Unbekannte Schluesselwoerter je Zeile (fuer Stelle und Korrektur)
    struct Unbekannt {
        const Eigenschaft* e;
        bool item;
        int menu, nr;
    };
    std::map<int, std::vector<Unbekannt>> unbekannt;
    for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
        const Menu& m = d.menus[mi];
        for (const Eigenschaft& e : m.eigenschaften) {
            if (!e.ok && e.kanon.empty() && e.kopf.datei == 0) { unbekannt[e.kopf.zeile].push_back({&e, false, static_cast<int>(mi), -1}); }
        }
        for (std::size_t ii = 0; ii < m.items.size(); ++ii) {
            for (const Eigenschaft& e : m.items[ii].eigenschaften) {
                if (!e.ok && e.kanon.empty() && e.kopf.datei == 0) {
                    unbekannt[e.kopf.zeile].push_back({&e, true, static_cast<int>(mi), static_cast<int>(ii)});
                }
            }
        }
    }
    const std::vector<std::string> itemWoerter = itemSchluessel(d.dialekt, d.spiel);
    const std::vector<std::string> menuWoerter = menuSchluessel(d.dialekt, d.spiel);
    for (const Meldung& m : d.meldungen) {
        Diagnose x;
        x.art = DiagArt::Engine;
        x.schwere = m.fehler ? Schwere::Fehler : Schwere::Warnung;
        x.text = m.text;
        x.zeile = m.datei == 0 ? m.zeile : 0;
        if (m.datei == 0 && m.zeile > 0 && static_cast<std::size_t>(m.zeile) <= zl.anfang.size()) {
            const std::uint32_t za = zl.anfang[static_cast<std::size_t>(m.zeile) - 1];
            const std::uint32_t ze = static_cast<std::size_t>(m.zeile) < zl.anfang.size() ? zl.anfang[static_cast<std::size_t>(m.zeile)] : static_cast<std::uint32_t>(t.size());
            // das genannte Wort: 'wort' oder das letzte Wort der Meldung
            std::string wort;
            const std::size_t q1 = m.text.find('\'');
            const std::size_t q2 = q1 == std::string::npos ? std::string::npos : m.text.find('\'', q1 + 1);
            if (q2 != std::string::npos) { wort = m.text.substr(q1 + 1, q2 - q1 - 1); }
            else {
                const std::size_t sp = m.text.find_last_of(' ');
                wort = sp == std::string::npos ? std::string() : m.text.substr(sp + 1);
            }
            const auto u = unbekannt.find(m.zeile);
            const Eigenschaft* treffer = nullptr;
            bool imItem = true;
            if (u != unbekannt.end()) {
                for (const Unbekannt& uu : u->second) {
                    if (wort.empty() || klein(uu.e->schluessel) == klein(wort)) {
                        treffer = uu.e;
                        imItem = uu.item;
                        x.menu = uu.menu;
                        x.item = uu.nr;
                        break;
                    }
                }
            }
            std::uint32_t a = 0, b = 0;
            if (treffer != nullptr && spanne(*treffer, a, b)) {
                x.anfang = a;
                x.ende = b;
                const std::string vorschlag = aehnlichstes(treffer->schluessel, imItem ? itemWoerter : menuWoerter, 2);
                if (!vorschlag.empty()) {
                    x.wert = vorschlag;
                    x.korrekturen.push_back(Korrektur{a, b, vorschlag});
                }
            } else if (!wort.empty()) {
                const std::string zeile = t.substr(za, ze - za);
                std::size_t p = 0;
                while ((p = klein(zeile).find(klein(wort), p)) != std::string::npos) {
                    const bool linksFrei = p == 0 || std::isspace(static_cast<unsigned char>(zeile[p - 1])) || zeile[p - 1] == '"';
                    const bool rechtsFrei = p + wort.size() >= zeile.size() || std::isspace(static_cast<unsigned char>(zeile[p + wort.size()])) ||
                                            zeile[p + wort.size()] == '"';
                    if (linksFrei && rechtsFrei) { break; }
                    ++p;
                }
                if (p != std::string::npos) {
                    x.anfang = za + static_cast<std::uint32_t>(p);
                    x.ende = x.anfang + static_cast<std::uint32_t>(wort.size());
                }
            }
            if (x.ende <= x.anfang) {   // das erste Wort der Zeile
                std::uint32_t p = za;
                while (p < ze && std::isspace(static_cast<unsigned char>(t[p]))) { ++p; }
                std::uint32_t q = p;
                while (q < ze && !std::isspace(static_cast<unsigned char>(t[q]))) { ++q; }
                x.anfang = p;
                x.ende = q;
            }
        }
        aus.push_back(x);
    }

    // --- Zahlen-Argumente (SP liest Nicht-Zahlen still als 0) -------------------------
    if (!mp) {
        auto zahlen = [&](const std::vector<Eigenschaft>& v, bool item, int mi, int ii) {
            for (const Eigenschaft& e : v) {
                if (e.kanon.empty()) { continue; }
                const int n = zahlArgumente(d.dialekt, d.spiel, item, e.kanon);
                for (int i = 0; i < n && static_cast<std::size_t>(i) < e.args.size(); ++i) {
                    const Token& a = e.args[static_cast<std::size_t>(i)];
                    if (istZahl(a.s) || a.ausMakro || a.datei != 0) { continue; }
                    // ein Name aus menudef.h mit dem Wert 0 ergibt zufaellig dasselbe (LISTBOX_TEXT, FEEDER_SAVEGAMES)
                    const Define* def = spDefine(a.s);
                    if (def != nullptr && def->wert == 0) { continue; }
                    Diagnose x;
                    x.art = DiagArt::KeineZahl;
                    x.schwere = Schwere::Warnung;
                    x.anfang = a.anfang;
                    x.ende = a.ende;
                    x.zeile = zl.zeileVon(a.anfang);
                    x.menu = mi;
                    x.item = ii;
                    x.text = e.kanon;
                    x.wert = def != nullptr ? a.s + " = " + std::to_string(def->wert) : a.s;
                    if (def != nullptr) { x.korrekturen.push_back(Korrektur{a.anfang, a.ende, std::to_string(def->wert)}); }
                    aus.push_back(x);
                }
            }
        };
        for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
            zahlen(d.menus[mi].eigenschaften, false, static_cast<int>(mi), -1);
            for (std::size_t ii = 0; ii < d.menus[mi].items.size(); ++ii) {
                zahlen(d.menus[mi].items[ii].eigenschaften, true, static_cast<int>(mi), static_cast<int>(ii));
            }
        }
    }

    // --- doppelte Namen ----------------------------------------------------------------
    auto nameStelle = [&](const std::vector<Eigenschaft>& v, Diagnose& x) {
        if (const Eigenschaft* e = eigenschaft(v, "name")) {
            std::uint32_t a = 0, b = 0;
            if (!e->args.empty() && spanne(e->args.front(), a, b)) {
                x.anfang = a;
                x.ende = b;
                x.zeile = zl.zeileVon(a);
            }
        }
    };
    {
        std::map<std::string, int> menues;
        for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
            const Menu& m = d.menus[mi];
            if (m.window.name.empty()) { continue; }
            if (!menues.emplace(klein(m.window.name), static_cast<int>(mi)).second) {
                Diagnose x;
                x.art = DiagArt::NameDoppelt;
                x.schwere = Schwere::Warnung;   // Menus_FindByName findet nur das erste
                x.menu = static_cast<int>(mi);
                x.text = m.window.name;
                nameStelle(m.eigenschaften, x);
                aus.push_back(x);
            }
            // Doppelte Item-Namen sind fuer show/hide gewollt (sie treffen alle); nur setfocus
            // (Menu_FindItemByName) findet allein das erste.
            std::set<std::string> fokusZiele;
            auto fokus = [&](const std::vector<Eigenschaft>& v) {
                for (const Eigenschaft& e : v) {
                    for (std::size_t i = 1; i + 1 < e.args.size(); ++i) {
                        if (!e.args.empty() && e.args.front().s == "{" && klein(e.args[i].s) == "setfocus") { fokusZiele.insert(klein(e.args[i + 1].s)); }
                    }
                }
            };
            fokus(m.eigenschaften);
            for (const Item& it : m.items) { fokus(it.eigenschaften); }
            std::map<std::string, int> items;
            for (std::size_t ii = 0; ii < m.items.size(); ++ii) {
                const Item& it = m.items[ii];
                if (it.window.name.empty() || fokusZiele.count(klein(it.window.name)) == 0) { continue; }
                if (!items.emplace(klein(it.window.name), static_cast<int>(ii)).second) {
                    Diagnose x;
                    x.art = DiagArt::NameDoppelt;
                    x.schwere = Schwere::Warnung;   // setfocus findet nur das erste
                    x.menu = static_cast<int>(mi);
                    x.item = static_cast<int>(ii);
                    x.text = it.window.name;
                    nameStelle(it.eigenschaften, x);
                    aus.push_back(x);
                }
            }
        }
    }

    // --- Ziele in Skripten -----------------------------------------------------------
    {
        std::set<std::string> menueNamen;
        for (const Menu& m : d.menus) { menueNamen.insert(klein(m.window.name)); }
        auto skripte = [&](const Menu& m, const std::vector<Eigenschaft>& v, int mi, int ii) {
            for (const Eigenschaft& e : v) {
                if (e.args.size() < 2 || e.args.front().s != "{") { continue; }
                bool befehl = true;
                std::string cmd;
                int argNr = 0;
                for (std::size_t i = 1; i + 1 < e.args.size(); ++i) {
                    const Token& a = e.args[i];
                    if (a.s == ";") {
                        befehl = true;
                        continue;
                    }
                    if (befehl) {
                        cmd = klein(a.s);
                        befehl = false;
                        argNr = 0;
                        continue;
                    }
                    ++argNr;
                    if (argNr != 1) { continue; }
                    bool fehlt = false;
                    if (cmd == "close" && klein(a.s) == "all") {   // Script_Close: "all" = Menus_CloseAll
                        fehlt = false;
                    } else if (cmd == "open" || cmd == "close") {
                        fehlt = menueNamen.count(klein(a.s)) == 0 && k.menueBekannt && !k.menueBekannt(a.s);
                    } else if (cmd == "show" || cmd == "hide" || cmd == "fadein" || cmd == "fadeout" || cmd == "setitemcolor" || cmd == "transition" ||
                               cmd == "transition2" || cmd == "orbit") {
                        fehlt = !passtGruppe(m, a.s, false);
                    } else if (cmd == "setfocus") {
                        fehlt = !passtGruppe(m, a.s, true);
                    }
                    if (!fehlt) { continue; }
                    Diagnose x;
                    x.art = DiagArt::ZielFehlt;
                    x.schwere = Schwere::Warnung;
                    x.menu = mi;
                    x.item = ii;
                    x.text = a.s;
                    x.wert = cmd;
                    std::uint32_t p = 0, q = 0;
                    if (spanne(a, p, q)) {
                        x.anfang = p;
                        x.ende = q;
                        x.zeile = zl.zeileVon(p);
                    }
                    aus.push_back(x);
                }
            }
        };
        for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
            const Menu& m = d.menus[mi];
            skripte(m, m.eigenschaften, static_cast<int>(mi), -1);
            for (std::size_t ii = 0; ii < m.items.size(); ++ii) { skripte(m, m.items[ii].eigenschaften, static_cast<int>(mi), static_cast<int>(ii)); }
        }
    }

    // --- Cvar-Tippfehler -----------------------------------------------------------
    if (!k.bekannteCvars.empty()) {
        std::set<std::string> bekannt;
        for (const std::string& c : k.bekannteCvars) { bekannt.insert(klein(c)); }
        for (const CvarFund& c : cvarsImMenue(d)) {
            if (bekannt.count(klein(c.name)) != 0) { continue; }
            const std::string v = aehnlichstes(c.name, k.bekannteCvars, c.name.size() >= 8 ? 2 : 1);
            if (v.empty()) { continue; }
            // jede Stelle, an der der Name als ganzes Wort steht
            const std::string kt = klein(t), kn = klein(c.name);
            std::size_t p = 0;
            while ((p = kt.find(kn, p)) != std::string::npos) {
                auto wortZeichen = [](char ch) { return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_'; };
                const bool frei = (p == 0 || !wortZeichen(t[p - 1])) && (p + kn.size() >= t.size() || !wortZeichen(t[p + kn.size()]));
                if (frei) {
                    Diagnose x;
                    x.art = DiagArt::CvarAehnlich;
                    x.schwere = Schwere::Hinweis;
                    x.anfang = static_cast<std::uint32_t>(p);
                    x.ende = static_cast<std::uint32_t>(p + kn.size());
                    x.zeile = zl.zeileVon(x.anfang);
                    x.text = c.name;
                    x.wert = v;
                    x.korrekturen.push_back(Korrektur{x.anfang, x.ende, v});
                    aus.push_back(x);
                }
                p += kn.size();
            }
        }
    }

    // --- die Pruefungen vor dem Export (gemeinsame Quelle) ------------------------------
    for (const Befund& b : pruefeFuerSpiel(d, k.pruefung)) {
        Diagnose x;
        x.art = DiagArt::Pruefung;
        x.befund = b;
        x.menu = b.menu;
        x.item = b.item;
        x.text = b.name;
        x.wert = b.wert;
        x.schwere = b.fehler ? Schwere::Fehler
                    : (b.art == BefundArt::DateiFehlt) ? Schwere::Warnung
                                                        : Schwere::Hinweis;
        const std::vector<Eigenschaft>* eig = nullptr;
        const Token* kopf = nullptr;
        if (b.menu >= 0 && static_cast<std::size_t>(b.menu) < d.menus.size()) {
            const Menu& m = d.menus[static_cast<std::size_t>(b.menu)];
            eig = &m.eigenschaften;
            kopf = &m.kopf;
            if (b.item >= 0 && static_cast<std::size_t>(b.item) < m.items.size()) {
                eig = &m.items[static_cast<std::size_t>(b.item)].eigenschaften;
                kopf = &m.items[static_cast<std::size_t>(b.item)].kopf;
            }
        }
        std::uint32_t a = 0, e = 0;
        // die genaue Stelle: das Argument mit dem Dateinamen bzw. die betroffene Eigenschaft
        if (eig != nullptr) {
            for (const Eigenschaft& ei : *eig) {
                if (b.art == BefundArt::DateiFehlt) {
                    for (const Token& tk : ei.args) {
                        if (klein(tk.s) == klein(b.name) || (b.wert == "cinematic" && klein(ei.kanon) == "cinematic")) {
                            if (spanne(tk, a, e)) {
                                if (k.pfadVorschlag) {
                                    const std::string v = k.pfadVorschlag(b.name);
                                    if (!v.empty() && b.wert != "cinematic") {
                                        const bool anf = t[a] == '"';
                                        x.korrekturen.push_back(Korrektur{a, e, anf ? "\"" + v + "\"" : v});
                                    }
                                }
                            }
                            break;
                        }
                    }
                } else if (b.art == BefundArt::NieSichtbar && klein(ei.kanon) == klein(b.name)) {
                    (void)spanne(ei, a, e);
                } else if ((b.art == BefundArt::UnbekanntIn || b.art == BefundArt::CinematicSp) && klein(ei.kanon) == klein(b.name)) {
                    (void)spanne(ei, a, e);
                } else if (b.art == BefundArt::SkriptZuLang && ei.schluessel == b.name) {
                    (void)spanne(ei, a, e);
                }
                if (e > a) { break; }
            }
        }
        if (e <= a && kopf != nullptr) { (void)spanne(*kopf, a, e); }
        if (e <= a && b.menu < 0 && !b.name.empty()) {   // z. B. play "x.wav" in einem Skript
            const std::size_t p = klein(t).find(klein(b.name));
            if (p != std::string::npos) {
                a = static_cast<std::uint32_t>(p);
                e = static_cast<std::uint32_t>(p + b.name.size());
            }
        }
        if (e > a) {
            x.anfang = a;
            x.ende = e;
            x.zeile = zl.zeileVon(a);
        }
        aus.push_back(x);
    }
    std::stable_sort(aus.begin(), aus.end(), [](const Diagnose& x, const Diagnose& y) {
        if (x.zeile != y.zeile) { return (x.zeile == 0 ? 1 << 30 : x.zeile) < (y.zeile == 0 ? 1 << 30 : y.zeile); }
        return static_cast<int>(x.schwere) < static_cast<int>(y.schwere);
    });
    return aus;
}

}  // namespace bhed::menue
