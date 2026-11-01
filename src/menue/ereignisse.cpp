// ereignisse.cpp - Menue-Editor M5 Punkt 2: Ereignisse und Aktions-Bausteine (siehe ereignisse.h).
//
// Eigener Code. Die Wortzahlen der Befehle folgen den Script_*-Handlern von ui_shared
// (OpenJK code/ui/ui_shared.cpp, codemp/ui/ui_shared.c, Movie Duels code/ui/ui_shared.cpp).
#include "bhed/menue/ereignisse.h"

#include "bhed/menue/skript.h"

#include <algorithm>
#include <cctype>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// String_Parse (COM_ParseExt): Leerraum ueberspringen, "..." als ein Wort, sonst bis Leerraum;
// ";" gehoert zum Wort davor, wenn es ohne Leerzeichen folgt - hier wie in den Menues meist
// getrennt; ein angehaengtes ";" wird abgetrennt.
struct Wort {
    std::string s;
    bool zitiert = false;
};
std::vector<Wort> woerter(const std::string& t) {
    std::vector<Wort> v;
    std::size_t p = 0;
    while (p < t.size()) {
        while (p < t.size() && static_cast<unsigned char>(t[p]) <= ' ') { ++p; }
        if (p >= t.size()) { break; }
        Wort w;
        if (t[p] == '"') {
            ++p;
            while (p < t.size() && t[p] != '"') { w.s += t[p++]; }
            if (p < t.size()) { ++p; }
            w.zitiert = true;
        } else {
            while (p < t.size() && static_cast<unsigned char>(t[p]) > ' ') { w.s += t[p++]; }
            if (w.s.size() > 1 && w.s.back() == ';') {
                w.s.pop_back();
                v.push_back(w);
                v.push_back(Wort{";", false});
                continue;
            }
        }
        v.push_back(w);
    }
    return v;
}

std::string zahl(float f) { return zahlText(f); }

const Token* klammer(const Eigenschaft& e, const char* s, bool letzte) {
    const Token* r = nullptr;
    for (const Token& t : e.args) {
        if (t.s == s && t.datei == 0) {
            r = &t;
            if (!letzte) { return r; }
        }
    }
    return r;
}

std::size_t zeilenAnfang(const std::string& t, std::size_t p) {
    while (p > 0 && t[p - 1] != '\n') { --p; }
    return p;
}

std::string zeilenende(const std::string& t) {
    const std::size_t n = t.find('\n');
    return (n != std::string::npos && n > 0 && t[n - 1] == '\r') ? "\r\n" : "\n";
}

// Einrueckung der Schluessel eines Items: die der ersten Eigenschaft, sonst die des itemDef + Tab
std::string einrueckung(const Datei& d, const Item& it) {
    const std::string& t = d.text;
    for (const Eigenschaft& e : it.eigenschaften) {
        if (e.kopf.datei != 0) { continue; }
        const std::size_t za = zeilenAnfang(t, e.kopf.anfang);
        return t.substr(za, e.kopf.anfang - za);
    }
    const std::size_t za = zeilenAnfang(t, it.anfang);
    return t.substr(za, it.anfang - za) + "\t";
}

}  // namespace

std::vector<Ereignis> ereignisseFuer(Dialekt d, Spiel s) {
    static const Ereignis alle[] = {{"mouseEnter", 0}, {"mouseExit", 1}, {"action", 2}, {"onFocus", 3}, {"leaveFocus", 4}, {"accept", 5}};
    const std::vector<std::string> sch = itemSchluessel(d, s);
    std::vector<Ereignis> v;
    for (const Ereignis& e : alle) {
        if (std::any_of(sch.begin(), sch.end(), [&](const std::string& k) { return klein(k) == klein(e.schluessel); })) { v.push_back(e); }
    }
    return v;
}

std::vector<std::string> bausteinBefehle(Dialekt d, Spiel s) {
    // Jedi Outcast SP (jk2sp.exe 1.04, Steam): die commandList hat kein setitemrect, setitemtext,
    // setitembackground, transition2 und delay (Namen im Programm gelesen; OpenJKs JK2_MODE nimmt
    // die JA-Liste und ist darin nicht wie das Original)
    static const std::vector<std::string> jk2 = {"show", "hide", "fadein", "fadeout", "setitemcolor", "setcolor", "setfocus", "setbackground",
                                                 "transition", "play", "playlooped", "open", "close", "setcvar", "exec", "orbit", "uiScript"};
    if (s == Spiel::JK2 && d == Dialekt::SP) { return jk2; }
    // commandList der beiden ui_shared-Fassungen (Movie Duels SP wie OpenJK SP; dort ist playlooped auskommentiert)
    static const std::vector<std::string> sp = {"show", "hide", "fadein", "fadeout", "setitemcolor", "setcolor", "setfocus", "setitemrect",
                                                "setitemtext", "setitembackground", "setbackground", "transition", "transition2",
                                                "play", "open", "close", "setcvar", "exec", "orbit", "delay", "uiScript"};
    static const std::vector<std::string> mp = {"show", "hide", "fadein", "fadeout", "setitemcolor", "setcolor", "setfocus", "setitemrect",
                                                "setitemtext", "setitembackground", "setbackground", "transition", "transition2",
                                                "play", "playlooped", "open", "close", "setcvar", "setcvartocvar", "exec", "orbit", "uiScript"};
    return d == Dialekt::MP ? mp : sp;
}

int bausteinWoerter(const std::string& befehl, Dialekt d) {
    (void)d;   // die Handler lesen in SP und MP gleich viele Woerter
    const std::string c = klein(befehl);
    if (c == "show" || c == "hide" || c == "fadein" || c == "fadeout" || c == "open" || c == "close" || c == "play" || c == "playlooped" ||
        c == "exec" || c == "setfocus" || c == "setbackground" || c == "setasset" || c == "showmenu") {
        return 1;
    }
    if (c == "setcvar" || c == "setitemtext" || c == "setitembackground" || c == "setcvartocvar") { return 2; }
    if (c == "setcolor") { return 5; }                 // Script_SetColor: backcolor|forecolor|bordercolor r g b a
    if (c == "setitemcolor") { return 6; }             // Name, Art, r g b a
    if (c == "setitemrect") { return 5; }              // Name x y w h
    if (c == "transition") { return 11; }              // Name, von (4), nach (4), Zeit, Schritt
    if (c == "orbit") { return 6; }                    // Name, x y cx cy Zeit
    return -1;
}

std::vector<Baustein> zerlege(const std::string& skript, Dialekt d, Spiel s) {
    std::vector<Baustein> v;
    const std::vector<Wort> w = woerter(skript);
    for (std::size_t i = 0; i < w.size();) {
        if (w[i].s == ";" && !w[i].zitiert) {
            ++i;
            continue;
        }
        Baustein b;
        b.befehl = w[i++].s;
        int n = klein(b.befehl) == "uiscript" ? -2 : bausteinWoerter(b.befehl, d);
        if (n == -2 && i < w.size() && w[i].s != ";") {   // uiScript <name> + Woerter des Skripts
            b.args.push_back(w[i].s);
            b.zitiert.push_back(w[i].zitiert);
            n = uiSkriptWortzahl(w[i++].s, d, s);
            if (n <= 0) { n = 0; }
        }
        while (i < w.size() && !(w[i].s == ";" && !w[i].zitiert) && (n < 0 || static_cast<int>(b.args.size()) < n + (klein(b.befehl) == "uiscript" ? 1 : 0))) {
            b.args.push_back(w[i].s);
            b.zitiert.push_back(w[i].zitiert);
            ++i;
        }
        v.push_back(b);
    }
    return v;
}

std::string zusammensetzen(const std::vector<Baustein>& b) {
    std::string s;
    for (const Baustein& x : b) {
        if (!s.empty()) { s += " ; "; }
        s += x.befehl;
        for (std::size_t i = 0; i < x.args.size(); ++i) {
            const bool zit = (i < x.zitiert.size() && x.zitiert[i]) || x.args[i].empty() ||
                             x.args[i].find_first_of(" \t;") != std::string::npos;
            s += zit ? " \"" + x.args[i] + "\"" : " " + x.args[i];
        }
    }
    return s;
}

bool skriptSetzen(const Datei& d, int menu, int item, const char* schluessel, const std::string& skript, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item >= 0 && static_cast<std::size_t>(item) >= m.items.size()) { return false; }
    const std::vector<Eigenschaft>& eig = item >= 0 ? m.items[static_cast<std::size_t>(item)].eigenschaften : m.eigenschaften;
    const std::string& t = d.text;
    const std::string nl = zeilenende(t);
    for (const Eigenschaft& e : eig) {
        if (klein(e.schluessel) != klein(schluessel)) { continue; }
        if (e.kopf.datei != 0 || e.kopf.ausMakro) { return false; }
        const Token* auf = klammer(e, "{", false);
        const Token* zu = klammer(e, "}", true);
        if (auf == nullptr || zu == nullptr || zu->anfang < auf->ende) { return false; }
        if (skript.empty()) {   // Schluessel samt Zeile entfernen
            std::size_t a = zeilenAnfang(t, e.kopf.anfang);
            std::size_t z = zu->ende;
            if (z < t.size() && t[z] == '\r') { ++z; }
            if (z < t.size() && t[z] == '\n') { ++z; }
            out.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(z), ""});
            return true;
        }
        out.push_back(Ersetzung{auf->ende, zu->anfang, " " + skript + " "});
        return true;
    }
    if (skript.empty()) { return true; }
    // neu: vor die schliessende } des itemDef bzw. menuDef, eigene Zeile. Beim Menue vor seine Items: dort stehen seine Skripte auch in den Originalen.
    if (item < 0) {
        if (m.ende == 0 || m.ende > t.size()) { return false; }
        const std::size_t ziel = m.items.empty() ? zeilenAnfang(t, m.ende - 1) : zeilenAnfang(t, m.items.front().anfang);
        std::string einr;
        for (const Eigenschaft& e : m.eigenschaften) {
            if (e.kopf.datei != 0) { continue; }
            const std::size_t za = zeilenAnfang(t, e.kopf.anfang);
            einr = t.substr(za, e.kopf.anfang - za);
            break;
        }
        if (einr.empty()) { einr = "\t"; }
        out.push_back(Ersetzung{static_cast<std::uint32_t>(ziel), static_cast<std::uint32_t>(ziel), einr + schluessel + " { " + skript + " }" + nl});
        return true;
    }
    const Item& it = m.items[static_cast<std::size_t>(item)];
    if (it.ende == 0 || it.ende > t.size()) { return false; }
    const std::size_t klZu = it.ende - 1;
    const std::size_t za = zeilenAnfang(t, klZu);
    const std::string einr = einrueckung(d, it);
    out.push_back(Ersetzung{static_cast<std::uint32_t>(za), static_cast<std::uint32_t>(za),
                            einr + schluessel + " { " + skript + " }" + nl});
    return true;
}

bool befehlMitZiel(const std::string& befehl) {
    static const char* const mitZiel[] = {"show", "hide", "fadein", "fadeout", "setfocus", "setitemcolor", "setitemrect", "setitemtext",
                                          "setitembackground", "transition", "transition2", "orbit"};
    const std::string c = klein(befehl);
    return std::any_of(std::begin(mitZiel), std::end(mitZiel), [&c](const char* m) { return c == m; });
}

bool zielDa(const Menu& m, const std::string& befehl, const std::string& ziel) {
    const bool nurName = klein(befehl) == "setfocus";
    const std::size_t stern = ziel.find('*');
    const std::string kz = klein(ziel);
    for (const Item& it : m.items) {
        if (stern != std::string::npos && !nurName) {
            if (it.window.name.compare(0, stern, ziel, 0, stern) == 0 && it.window.name.size() >= stern) { return true; }
            if (it.window.group.compare(0, stern, ziel, 0, stern) == 0 && it.window.group.size() >= stern) { return true; }
            continue;
        }
        if (klein(it.window.name) == kz) { return true; }
        if (!nurName && !it.window.group.empty() && klein(it.window.group) == kz) { return true; }
    }
    return false;
}

bool itemUmbenennen(const Datei& d, int menu, int item, const std::string& neuZitiert, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item < 0 || static_cast<std::size_t>(item) >= m.items.size()) { return false; }
    const std::string alt = m.items[static_cast<std::size_t>(item)].window.name;
    if (!setzeEigenschaft(d, menu, item, "name", {neuZitiert}, out)) { return false; }
    if (alt.empty()) { return true; }
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (static_cast<int>(i) != item && klein(m.items[i].window.name) == klein(alt)) { return true; }
    }
    std::string neu = neuZitiert;
    if (neu.size() >= 2 && neu.front() == '"' && neu.back() == '"') { neu = neu.substr(1, neu.size() - 2); }
    const auto skripte = [&](const std::vector<Eigenschaft>& eig) {
        for (const Eigenschaft& e : eig) {
            static const char* const kSkripte[] = {"action", "mouseenter", "mouseexit", "mouseentertext", "mouseexittext", "onfocus", "leavefocus", "accept",
                                                   "selectnext", "selectprev", "doubleclick", "onopen", "onclose", "onaccept", "onesc"};
            const std::string k = klein(e.schluessel);
            if (std::none_of(std::begin(kSkripte), std::end(kSkripte), [&k](const char* s) { return k == s; })) { continue; }
            bool amAnfang = true;
            for (std::size_t a = 0; a + 1 < e.args.size(); ++a) {
                const Token& t = e.args[a];
                if (t.s == "{" || t.s == ";") {
                    amAnfang = true;
                    continue;
                }
                if (!amAnfang) { continue; }
                amAnfang = false;
                if (!befehlMitZiel(t.s)) { continue; }
                const Token& ziel = e.args[a + 1];
                std::string s = ziel.s;
                if (s.size() > 1 && s.back() == ';') { s.pop_back(); }   // "show x;": das Semikolon haengt am Wort
                if (klein(s) != klein(alt) || ziel.datei != 0 || ziel.ausMakro) { continue; }
                const std::size_t p = d.text.find(s, ziel.anfang);
                if (p == std::string::npos || p + s.size() > ziel.ende) { continue; }
                out.push_back(Ersetzung{static_cast<std::uint32_t>(p), static_cast<std::uint32_t>(p + s.size()), neu});
            }
        }
    };
    skripte(m.eigenschaften);
    for (const Item& it : m.items) { skripte(it.eigenschaften); }
    return true;
}

bool hoverEffekt(const Datei& d, int menu, int item, HoverEffekt e, const Farbe& farbe, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item < 0 || static_cast<std::size_t>(item) >= m.items.size()) { return false; }
    const Item& it = m.items[static_cast<std::size_t>(item)];
    const std::string& name = it.window.name;
    if (name.empty()) { return false; }
    const std::vector<Ereignis> ev = ereignisseFuer(d.dialekt, d.spiel);
    const bool hover = std::any_of(ev.begin(), ev.end(), [](const Ereignis& x) { return x.art == 0; }) &&
                       std::any_of(ev.begin(), ev.end(), [](const Ereignis& x) { return x.art == 1; });
    if (!hover) { return false; }
    const std::string f = zahl(farbe.r) + " " + zahl(farbe.g) + " " + zahl(farbe.b) + " " + zahl(farbe.a);
    const Farbe& vf = it.window.foreColor;
    const std::string alt = zahl(vf.r) + " " + zahl(vf.g) + " " + zahl(vf.b) + " " + zahl(vf.a);
    // an vorhandene Hover-Skripte anhaengen
    auto mit = [&](const std::string& vorher, const std::string& neu) { return vorher.empty() ? neu : vorher + " ; " + neu; };
    std::string ein, aus;
    const Rechteck& r = it.window.rectClient;
    switch (e) {
        case HoverEffekt::Leuchten: {
            const std::string glow = name + "_glow";
            ein = "show " + glow;
            aus = "hide " + glow;
            // neues Element vor dem Item (darunter gezeichnet), gleiche Einrueckung wie das itemDef
            const std::string& t = d.text;
            const std::size_t za = zeilenAnfang(t, it.anfang);
            const std::string e0 = t.substr(za, it.anfang - za);
            const std::string nl = zeilenende(t);
            const std::string e1 = e0 + "\t";
            const std::string stil = d.dialekt == Dialekt::MP && d.defines.count("WINDOW_STYLE_SHADER") == 0 ? "3" : "WINDOW_STYLE_SHADER";
            const std::string block = e0 + "itemDef" + nl + e0 + "{" + nl + e1 + "name " + glow + nl + e1 + "style " + stil + nl + e1 + "rect " +
                                      zahl(r.x - 4) + " " + zahl(r.y - 4) + " " + zahl(r.w + 8) + " " + zahl(r.h + 8) + nl + e1 +
                                      "background \"gfx/menus/menu_buttonback\"" + nl + e1 + "forecolor " + f + nl + e1 + "visible 0" + nl + e1 +
                                      "decoration" + nl + e0 + "}" + nl;
            out.push_back(Ersetzung{static_cast<std::uint32_t>(za), static_cast<std::uint32_t>(za), block});
            break;
        }
        case HoverEffekt::Farbe:
            ein = "setitemcolor " + name + " forecolor " + f;
            aus = "setitemcolor " + name + " forecolor " + alt;
            break;
        case HoverEffekt::Rahmen: {
            ein = "setitemcolor " + name + " bordercolor " + f;
            aus = "setitemcolor " + name + " bordercolor " + zahl(farbe.r) + " " + zahl(farbe.g) + " " + zahl(farbe.b) + " 0";
            break;
        }
        case HoverEffekt::Groesser: {
            const std::string klein0 = zahl(r.x) + " " + zahl(r.y) + " " + zahl(r.w) + " " + zahl(r.h);
            const std::string gross = zahl(r.x - 4) + " " + zahl(r.y - 4) + " " + zahl(r.w + 8) + " " + zahl(r.h + 8);
            ein = "transition " + name + " " + klein0 + " " + gross + " 10 1";
            aus = "transition " + name + " " + gross + " " + klein0 + " 10 1";
            break;
        }
    }
    // vorhandene Schluessel an Ort und Stelle, fehlende zusammen in einer Zeilenfolge vor die }
    // (mehrere Einfuegungen an derselben Stelle waeren nicht eindeutig geordnet)
    std::vector<std::pair<std::string, std::string>> neu;   // Schluessel, Rest der Zeile
    auto hat = [&](const char* k) {
        return std::any_of(it.eigenschaften.begin(), it.eigenschaften.end(), [&](const Eigenschaft& x) { return klein(x.schluessel) == klein(k); });
    };
    for (const auto& [k, s] : {std::pair<const char*, std::string>{"mouseEnter", mit(it.mouseEnter, ein)}, {"mouseExit", mit(it.mouseExit, aus)}}) {
        if (hat(k)) {
            if (!skriptSetzen(d, menu, item, k, s, out)) { return false; }
        } else {
            neu.emplace_back(k, "{ " + s + " }");
        }
    }
    if (e == HoverEffekt::Rahmen) {
        if (it.window.border == 0 && !hat("border")) { neu.emplace_back("border", "1"); }
        const std::vector<std::string> bc = {zahl(farbe.r), zahl(farbe.g), zahl(farbe.b), "0"};
        if (hat("bordercolor")) {
            if (!setzeEigenschaft(d, menu, item, "bordercolor", bc, out)) { return false; }
        } else {
            neu.emplace_back("bordercolor", bc[0] + " " + bc[1] + " " + bc[2] + " " + bc[3]);
        }
    }
    if (!neu.empty()) {
        const std::string& t = d.text;
        if (it.ende == 0 || it.ende > t.size()) { return false; }
        const std::size_t za = zeilenAnfang(t, it.ende - 1);
        const std::string einr = einrueckung(d, it), nl = zeilenende(t);
        std::string z;
        for (const auto& [k, s] : neu) { z += einr + k + " " + s + nl; }
        out.push_back(Ersetzung{static_cast<std::uint32_t>(za), static_cast<std::uint32_t>(za), z});
    }
    return true;
}

}  // namespace bhed::menue
