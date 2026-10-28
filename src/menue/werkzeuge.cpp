// werkzeuge.cpp - Menue-Editor M1 Punkt 4: Werkzeuge der Arbeitsflaeche.
//
// Eigener Code.
#include "bhed/menue/werkzeuge.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bhed::menue {

namespace {

std::size_t zeilenAnfang(const std::string& t, std::size_t p) {
    while (p > 0 && t[p - 1] != '\n') { --p; }
    return p;
}
std::size_t nachZeile(const std::string& t, std::size_t p) {
    while (p < t.size() && (t[p] == ' ' || t[p] == '\t')) { ++p; }
    if (p < t.size() && t[p] == '\r') { ++p; }
    if (p < t.size() && t[p] == '\n') { ++p; }
    return p;
}
bool nurLeerDavor(const std::string& t, std::size_t p) {
    for (std::size_t i = zeilenAnfang(t, p); i < p; ++i) {
        if (t[i] != ' ' && t[i] != '\t') { return false; }
    }
    return true;
}
std::string nlVon(const std::string& t) { return t.find("\r\n") != std::string::npos ? "\r\n" : "\n"; }

// Spanne eines Items mit ganzen Zeilen (wenn es allein auf seinen Zeilen steht)
void itemSpanne(const Datei& d, const Item& it, std::size_t& a, std::size_t& e) {
    a = it.anfang;
    e = it.ende;
    if (nurLeerDavor(d.text, a)) { a = zeilenAnfang(d.text, a); }
    const std::size_t n = nachZeile(d.text, e);
    if (n > e && (n >= d.text.size() || d.text[n - 1] == '\n')) { e = n; }
}

}  // namespace

Einrasten einrasten(const Rechteck& r, const std::vector<Rechteck>& andere, float raster, float toleranz, bool anRaster,
                    bool anKanten) {
    Einrasten e;
    float bestX = toleranz + 1, bestY = toleranz + 1;
    const float xs[3] = {r.x, r.x + r.w * 0.5F, r.x + r.w};
    const float ys[3] = {r.y, r.y + r.h * 0.5F, r.y + r.h};
    auto probeX = [&](float ziel, float von, float bis) {
        for (const float x : xs) {
            const float d = ziel - x;
            if (std::fabs(d) <= toleranz && std::fabs(d) < bestX) {
                bestX = std::fabs(d);
                e.dx = d;
                e.linien.erase(std::remove_if(e.linien.begin(), e.linien.end(), [](const Hilfslinie& h) { return h.senkrecht; }),
                               e.linien.end());
                e.linien.push_back(Hilfslinie{true, ziel, std::min(von, r.y), std::max(bis, r.y + r.h)});
            }
        }
    };
    auto probeY = [&](float ziel, float von, float bis) {
        for (const float y : ys) {
            const float d = ziel - y;
            if (std::fabs(d) <= toleranz && std::fabs(d) < bestY) {
                bestY = std::fabs(d);
                e.dy = d;
                e.linien.erase(std::remove_if(e.linien.begin(), e.linien.end(), [](const Hilfslinie& h) { return !h.senkrecht; }),
                               e.linien.end());
                e.linien.push_back(Hilfslinie{false, ziel, std::min(von, r.x), std::max(bis, r.x + r.w)});
            }
        }
    };
    if (anKanten) {
        for (const Rechteck& o : andere) {
            for (const float x : {o.x, o.x + o.w * 0.5F, o.x + o.w}) { probeX(x, o.y, o.y + o.h); }
            for (const float y : {o.y, o.y + o.h * 0.5F, o.y + o.h}) { probeY(y, o.x, o.x + o.w); }
        }
        for (const float x : {0.0F, 320.0F, 640.0F}) { probeX(x, 0, 480); }
        for (const float y : {0.0F, 240.0F, 480.0F}) { probeY(y, 0, 640); }
    }
    if (anRaster && raster > 0) {
        if (bestX > toleranz) { e.dx = std::round(r.x / raster) * raster - r.x; }
        if (bestY > toleranz) { e.dy = std::round(r.y / raster) * raster - r.y; }
    }
    return e;
}

void ausrichten(std::vector<Rechteck>& r, Ausrichtung a) {
    if (r.size() < 2) { return; }
    const Rechteck k = r.front();
    for (std::size_t i = 1; i < r.size(); ++i) {
        switch (a) {
            case Ausrichtung::Links: r[i].x = k.x; break;
            case Ausrichtung::Rechts: r[i].x = k.x + k.w - r[i].w; break;
            case Ausrichtung::Oben: r[i].y = k.y; break;
            case Ausrichtung::Unten: r[i].y = k.y + k.h - r[i].h; break;
            case Ausrichtung::MitteH: r[i].x = k.x + (k.w - r[i].w) * 0.5F; break;
            case Ausrichtung::MitteV: r[i].y = k.y + (k.h - r[i].h) * 0.5F; break;
        }
    }
}

void verteilen(std::vector<Rechteck>& r, bool waagrecht) {
    if (r.size() < 3) { return; }
    std::vector<std::size_t> idx(r.size());
    for (std::size_t i = 0; i < idx.size(); ++i) { idx[i] = i; }
    std::sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) { return waagrecht ? r[a].x < r[b].x : r[a].y < r[b].y; });
    float summe = 0;
    for (const Rechteck& x : r) { summe += waagrecht ? x.w : x.h; }
    const Rechteck& erstes = r[idx.front()];
    const Rechteck& letztes = r[idx.back()];
    const float anfang = waagrecht ? erstes.x : erstes.y;
    const float ende = waagrecht ? letztes.x + letztes.w : letztes.y + letztes.h;
    const float luecke = (ende - anfang - summe) / static_cast<float>(r.size() - 1);
    float pos = anfang;
    for (const std::size_t i : idx) {
        if (waagrecht) { r[i].x = pos; pos += r[i].w + luecke; }
        else { r[i].y = pos; pos += r[i].h + luecke; }
    }
}

std::string itemVorlage(int typ, Dialekt d, const std::string& name, const Rechteck& r, const std::string& einr,
                        const std::string& nl) {
    const std::string e = einr + "\t";
    std::string s = einr + "itemDef" + nl + einr + "{" + nl;
    auto z = [&](const std::string& k, const std::string& v) { s += e + k + "\t\t\t" + v + nl; };
    z("name", name);
    if (typ != ITEM_TYPE_TEXT || d == Dialekt::MP) { z("type", typName(typ)); }
    else { z("type", "ITEM_TYPE_TEXT"); }
    z("rect", zahlText(r.x) + " " + zahlText(r.y) + " " + zahlText(r.w) + " " + zahlText(r.h));
    switch (typ) {
        case ITEM_TYPE_BUTTON:
            z("style", "WINDOW_STYLE_EMPTY");
            z("text", "\"Button\"");
            z("textscale", "1");
            z("textalign", "ITEM_ALIGN_CENTER");
            z("textalignx", zahlText(r.w * 0.5F));
            z("forecolor", "1 1 1 1");
            z("action", "{ }");
            break;
        case ITEM_TYPE_YESNO:
            z("text", "\"Option:\"");
            z("cvar", "\"ui_option\"");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        case ITEM_TYPE_MULTI:
            z("text", "\"Choice:\"");
            z("cvar", "\"ui_choice\"");
            z("cvarFloatList", "{ \"One\" 0 \"Two\" 1 }");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        case ITEM_TYPE_SLIDER:
            z("text", "\"Value:\"");
            z("cvarFloat", "\"ui_value\" 0.5 0 1");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        case ITEM_TYPE_EDITFIELD:
            z("text", "\"Name:\"");
            z("cvar", "\"ui_name\"");
            z("maxChars", "32");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        case ITEM_TYPE_LISTBOX:
            z("elementwidth", "120");
            z("elementheight", "16");
            z("elementtype", "0");
            z("feeder", "0");
            z("textscale", "0.7");
            z("forecolor", "1 1 1 1");
            z("outlinecolor", "1 1 1 0.25");
            break;
        case ITEM_TYPE_MODEL:
            z("asset_model", "\"models/players/kyle/model.glm\"");
            z("model_fovx", "40");
            z("model_fovy", "40");
            break;
        case ITEM_TYPE_BIND:
            z("text", "\"Attack:\"");
            z("cvar", "\"+attack\"");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        default:   // Text / Bild
            if (typ == -1) {
                s.clear();
                s = einr + "itemDef" + nl + einr + "{" + nl;
                z("name", name);
                z("style", "WINDOW_STYLE_SHADER");
                z("rect", zahlText(r.x) + " " + zahlText(r.y) + " " + zahlText(r.w) + " " + zahlText(r.h));
                z("background", "\"gfx/menus/menu_box1\"");
                z("forecolor", "1 1 1 1");
                z("decoration", "");
                break;
            }
            z("text", "\"Text\"");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            z("decoration", "");
            break;
    }
    z("visible", "1");
    s += einr + "}" + nl;
    // "decoration\t\t\t" ohne Wert: Abstand am Zeilenende weg
    std::string aus;
    std::size_t a = 0;
    while (a < s.size()) {
        std::size_t e2 = s.find('\n', a);
        if (e2 == std::string::npos) { e2 = s.size(); }
        std::string zl = s.substr(a, e2 - a);
        const bool cr = !zl.empty() && zl.back() == '\r';
        if (cr) { zl.pop_back(); }
        while (!zl.empty() && (zl.back() == '\t' || zl.back() == ' ')) { zl.pop_back(); }
        aus += zl + (cr ? "\r" : "") + (e2 < s.size() ? "\n" : "");
        a = e2 + 1;
    }
    return aus;
}

bool itemEinfuegen(const Datei& d, int menu, const std::string& itemText, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (m.ende == 0 || m.ende > d.text.size() || d.text[m.ende - 1] != '}') { return false; }
    const std::size_t zu = m.ende - 1;
    std::size_t p = nurLeerDavor(d.text, zu) ? zeilenAnfang(d.text, zu) : zu;
    std::string neu = itemText;
    if (p == zu) { neu = nlVon(d.text) + neu; }
    out.push_back(Ersetzung{static_cast<std::uint32_t>(p), static_cast<std::uint32_t>(p), neu});
    return true;
}

bool itemLoeschen(const Datei& d, int menu, int item, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item < 0 || static_cast<std::size_t>(item) >= m.items.size()) { return false; }
    const Item& it = m.items[static_cast<std::size_t>(item)];
    if (it.kopf.datei != 0 || it.ende <= it.anfang) { return false; }
    std::size_t a = 0, e = 0;
    itemSpanne(d, it, a, e);
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(e), ""});
    return true;
}

std::string itemText(const Datei& d, int menu, int item) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return {}; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item < 0 || static_cast<std::size_t>(item) >= m.items.size()) { return {}; }
    std::size_t a = 0, e = 0;
    itemSpanne(d, m.items[static_cast<std::size_t>(item)], a, e);
    std::string t = d.text.substr(a, e - a);
    if (t.empty() || t.back() != '\n') { t += nlVon(d.text); }
    return t;
}

bool itemVerschieben(const Datei& d, int menu, int item, bool nachVorn, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const int ander = nachVorn ? item + 1 : item - 1;
    if (item < 0 || ander < 0 || static_cast<std::size_t>(item) >= m.items.size() || static_cast<std::size_t>(ander) >= m.items.size()) {
        return false;
    }
    const int erst = std::min(item, ander);
    const int zweit = std::max(item, ander);
    std::size_t a1 = 0, e1 = 0, a2 = 0, e2 = 0;
    itemSpanne(d, m.items[static_cast<std::size_t>(erst)], a1, e1);
    itemSpanne(d, m.items[static_cast<std::size_t>(zweit)], a2, e2);
    if (e1 > a2) { return false; }
    // [a1,e1) zwischen [e1,a2) [a2,e2) -> Bloecke tauschen, Zwischenraum bleibt
    const std::string b1 = d.text.substr(a1, e1 - a1);
    const std::string mitte = d.text.substr(e1, a2 - e1);
    const std::string b2 = d.text.substr(a2, e2 - a2);
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a1), static_cast<std::uint32_t>(e2), b2 + mitte + b1});
    return true;
}

std::string freierName(const Menu& m, const std::string& basis) {
    std::string b = basis.empty() ? std::string("item") : basis;
    // "_<zahl>" am Ende abschneiden
    const std::size_t u = b.find_last_of('_');
    if (u != std::string::npos && u + 1 < b.size() &&
        std::all_of(b.begin() + static_cast<std::ptrdiff_t>(u) + 1, b.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })) {
        b = b.substr(0, u);
    }
    for (int n = 2; n < 10000; ++n) {
        const std::string k = b + "_" + std::to_string(n);
        bool frei = true;
        for (const Item& it : m.items) {
            if (it.window.name == k) { frei = false; }
        }
        if (frei) { return k; }
    }
    return b + "_x";
}

std::string umbenennen(const std::string& t, const std::string& neuerName, Dialekt d) {
    // die erste "name"-Zeile im Block
    std::size_t p = 0;
    while ((p = t.find("name", p)) != std::string::npos) {
        const bool links = p == 0 || static_cast<unsigned char>(t[p - 1]) <= ' ';
        const bool rechts = p + 4 < t.size() && (t[p + 4] == ' ' || t[p + 4] == '\t');
        if (links && rechts) {
            std::size_t a = p + 4;
            while (a < t.size() && (t[a] == ' ' || t[a] == '\t')) { ++a; }
            std::size_t e = a;
            if (e < t.size() && t[e] == '"') {
                e = t.find('"', e + 1);
                e = (e == std::string::npos) ? t.size() : e + 1;
            } else {
                while (e < t.size() && static_cast<unsigned char>(t[e]) > ' ') { ++e; }
            }
            const bool quote = a < t.size() && t[a] == '"';
            return t.substr(0, a) + (quote ? "\"" + neuerName + "\"" : neuerName) + t.substr(e);
        }
        p += 4;
    }
    (void)d;
    const std::size_t auf = t.find('{');
    if (auf == std::string::npos) { return t; }
    return t.substr(0, auf + 1) + " name " + neuerName + t.substr(auf + 1);
}

}  // namespace bhed::menue
