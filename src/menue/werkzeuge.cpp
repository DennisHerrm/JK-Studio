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

Rechteck ohneDeckung(Rechteck r, const std::vector<Rechteck>& andere, float schritt) {
    const auto belegt = [&] {
        return std::any_of(andere.begin(), andere.end(), [&](const Rechteck& o) {
            return std::fabs(o.x - r.x) < 1.0F && std::fabs(o.y - r.y) < 1.0F && std::fabs(o.w - r.w) < 1.0F && std::fabs(o.h - r.h) < 1.0F;
        });
    };
    for (int i = 0; i < 40 && belegt(); ++i) {
        r.x += schritt;
        r.y += schritt;
    }
    return r;
}

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

float textXFuer(int ausrichtung, float breite) {
    if (ausrichtung == 1) { return std::round(breite * 0.5F); }
    if (ausrichtung == 2) { return breite - kTextRand; }
    return kTextRand;
}

float textYFuer(int senkrecht, float hoehe, float oben, float unten) {
    if (senkrecht == 1) { return std::round((hoehe - oben - unten) * 0.5F); }
    if (senkrecht == 2) { return std::round(hoehe - unten - 2.0F); }
    return std::round(2.0F - oben);
}

bool textXFolgt(int ausrichtung, float x, float breite) {
    if (ausrichtung != 1 && ausrichtung != 2) { return false; }
    return std::fabs(x - textXFuer(ausrichtung, breite)) <= 0.5F;
}

int textSenkrecht(float y, float hoehe, float oben, float unten) {
    // Mitte zuerst: bei niedrigen Elementen fallen die Stufen zusammen, und die Mitte ist die haeufige Wahl
    for (const int s : {1, 2, 0}) {
        if (std::fabs(y - textYFuer(s, hoehe, oben, unten)) <= 0.5F) { return s; }
    }
    return -1;
}

bool istBildRahmen(const Item& it) { return it.type == ITEM_TYPE_TEXT && it.text.empty() && it.cvar.empty() && it.window.style == WINDOW_STYLE_SHADER; }

bool feldGilt(const std::string& schluessel, int typ, int style, Spiel spiel) {
    std::string k = schluessel;
    for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    const bool modell = typ == ITEM_TYPE_MODEL;
    const bool liste = typ == ITEM_TYPE_LISTBOX;
    const bool scroll = typ == ITEM_TYPE_TEXTSCROLL;
    if (k == "text" || k == "text2" || k == "text2alignx" || k == "text2aligny") { return !modell && !liste; }
    // Item_ListBox_Paint: Zeilen in textscale, textStyle und iMenuFont; Item_TextScroll_Paint: textalignx/y, aber keine Ausrichtung
    if (k == "textscale" || k == "textstyle" || k == "font") { return !modell; }
    if (k == "textalign") { return !modell && !liste && !scroll; }
    if (k == "textalignx" || k == "textaligny") { return !modell && !liste; }
    // Window_Paint nimmt foreColor nur bei WINDOW_STYLE_SHADER; beim Modell gibt es sonst keinen Text, der sie nutzt
    if (k == "forecolor") { return !modell || style == WINDOW_STYLE_SHADER; }
    if (k == "cvar") {
        return typ == ITEM_TYPE_TEXT || scroll || typ == ITEM_TYPE_YESNO || typ == ITEM_TYPE_MULTI || typ == ITEM_TYPE_SLIDER || typ == ITEM_TYPE_EDITFIELD ||
               typ == ITEM_TYPE_NUMERICFIELD || typ == ITEM_TYPE_RADIOBUTTON || typ == ITEM_TYPE_CHECKBOX || typ == ITEM_TYPE_BIND;
    }
    if (k == "asset_model" || k == "model_angle" || k == "model_rotation" || k == "model_fovx" || k == "model_fovy") { return modell; }
    if (k == "model_origin") { return modell && spiel == Spiel::JK2; }
    if (k == "model_g2mins" || k == "model_g2maxs" || k == "model_g2skin" || k == "model_g2anim" || k == "ischaracter" || k == "issaber" || k == "issaber2") {
        return modell && spiel != Spiel::JK2;
    }
    if (k == "feeder" || k == "elementwidth" || k == "elementheight" || k == "elementtype" || k == "columns") { return liste; }
    if (k == "lineheight") { return scroll; }
    if (k == "ownerdraw" || k == "ownerdrawflag") { return typ == ITEM_TYPE_OWNERDRAW; }
    if (k == "maxchars" || k == "maxpaintchars") { return typ == ITEM_TYPE_EDITFIELD || typ == ITEM_TYPE_NUMERICFIELD; }
    return true;
}

std::string itemVorlage(int typ, Dialekt d, const std::string& name, const Rechteck& r, const std::string& einr,
                        const std::string& nl, Spiel spiel) {
    const std::string e = einr + "\t";
    std::string s = einr + "itemDef" + nl + einr + "{" + nl;
    auto z = [&](const std::string& k, const std::string& v) {
        // rc608 Teil 3a: Die Vorgaben rechnen mit ergoec. In Movie Duels MP heisst sie font 3; ohne Nummer stuende dort albertus_bold, fast viermal so gross (Spielfoto)
        if (k == "textscale" && d == Dialekt::MP && spiel == Spiel::MD) { s += e + "font\t\t\t3" + nl; }
        s += e + k + "\t\t\t" + v + nl;
    };
    z("name", name);
    if (typ != ITEM_TYPE_TEXT || d == Dialekt::MP) { z("type", typName(typ)); }
    else { z("type", "ITEM_TYPE_TEXT"); }
    z("rect", zahlText(r.x) + " " + zahlText(r.y) + " " + zahlText(r.w) + " " + zahlText(r.h));
    switch (typ) {
        case ITEM_TYPE_BUTTON:
            // rc608 Teil 3a: mit sichtbarer Flaeche; Farbe wie die Knoepfe der fertigen Teile
            z("style", "WINDOW_STYLE_FILLED");
            z("backcolor", "0.15 0.18 0.26 0.9");
            z("text", "\"Button\"");
            z("textscale", "0.7");
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
            // rc608 Punkt 12: aus den echten Menues statt geschaetzt. JK2 kennt in Menues nur Modelle ohne Ghoul2 (ui/credits.menu, logomodel).
            // JKA und MD: Figur ganz im Bild, von vorn, Standpose wie ui/jamp/ingame_player2.menu (Grenzen, Blickwinkel, model_angle 180)
            // und ui/md_mp/ingame_player.menu (BOTH_MENUIDLE1).
            if (spiel == Spiel::JK2) {
                z("asset_model", "\"models/map_objects/bespin/jk2logo.md3\"");
                z("model_fovx", "37");
                z("model_fovy", "34");
                z("model_origin", "100 100 100");
                z("model_angle", "90");
                break;
            }
            // Die Reihenfolge zaehlt: ItemParse_asset_model_go setzt die Animation, die zu dem Zeitpunkt schon gelesen ist; steht
            // model_g2anim dahinter, zeigt das Spiel die T-Pose (OpenJK code/ui, MD; im Spiel fotografiert). Nur isCharacter stellt sie spaeter.
            // Das MP von OpenJK spielt model_g2anim nur mit isCharacter (dann die Figur aus ui_char_model).
            if (d == Dialekt::MP && spiel != Spiel::MD) {
                z("asset_model", "\"ui_char_model\"");
                z("model_g2anim", "\"BOTH_WALK1\"");
                z("isCharacter", "1");
            } else {
                z("model_g2anim", "\"BOTH_MENUIDLE1\"");
                z("asset_model", "\"models/players/kyle/model.glm\"");
                z("model_g2skin", "\"models/players/kyle/model_default.skin\"");
            }
            z("model_fovx", "50");
            z("model_fovy", "50");
            z("model_angle", "180");
            z("model_g2mins", "-30 -15 -14");
            z("model_g2maxs", "20 15 30");
            break;
        case ITEM_TYPE_BIND:
            z("text", "\"Attack:\"");
            z("cvar", "\"+attack\"");
            z("textscale", "1");
            z("forecolor", "1 1 1 1");
            break;
        default:   // Text / Bild
            if (typ == -1 || typ == -3) {
                s.clear();
                s = einr + "itemDef" + nl + einr + "{" + nl;
                z("name", name);
                z("style", "WINDOW_STYLE_SHADER");
                z("rect", zahlText(r.x) + " " + zahlText(r.y) + " " + zahlText(r.w) + " " + zahlText(r.h));
                if (typ == -3) { s += e + kVideoRahmenKennung + nl; }
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

namespace {
bool menuSpanne(const Datei& d, int menu, std::size_t& a, std::size_t& e) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (m.kopf.datei != 0 || m.ende <= m.anfang) { return false; }
    a = m.anfang;
    e = m.ende;
    if (nurLeerDavor(d.text, a)) { a = zeilenAnfang(d.text, a); }
    const std::size_t n = nachZeile(d.text, e);
    if (n > e && (n >= d.text.size() || d.text[n - 1] == '\n')) { e = n; }
    return true;
}
}  // namespace

bool menuLoeschen(const Datei& d, int menu, std::vector<Ersetzung>& out) {
    std::size_t a = 0, e = 0;
    if (!menuSpanne(d, menu, a, e)) { return false; }
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(e), ""});
    return true;
}

bool menuTauschen(const Datei& d, int menu, std::vector<Ersetzung>& out) {
    std::size_t a0 = 0, e0 = 0, a1 = 0, e1 = 0;
    if (!menuSpanne(d, menu, a0, e0) || !menuSpanne(d, menu + 1, a1, e1)) { return false; }
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a0), static_cast<std::uint32_t>(e0), d.text.substr(a1, e1 - a1)});
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a1), static_cast<std::uint32_t>(e1), d.text.substr(a0, e0 - a0)});
    return true;
}

bool menuUmbenennen(const Datei& d, int menu, const std::string& neu, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const std::string alt = d.menus[static_cast<std::size_t>(menu)].window.name;
    if (!setzeEigenschaft(d, menu, -1, "name", {"\"" + neu + "\""}, out)) { return false; }
    const auto gleich = [](const std::string& x, const std::string& y) {
        return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), [](char p, char q) { return std::tolower(static_cast<unsigned char>(p)) == std::tolower(static_cast<unsigned char>(q)); });
    };
    const auto skripte = [&](const std::vector<Eigenschaft>& eig) {
        for (const Eigenschaft& s : eig) {
            bool amAnfang = false;
            for (std::size_t k = 0; k + 1 < s.args.size(); ++k) {
                const Token& t = s.args[k];
                if (t.s == "{" || t.s == ";") {
                    amAnfang = true;
                    continue;
                }
                if (!amAnfang) { continue; }
                amAnfang = false;
                if (!gleich(t.s, "open") && !gleich(t.s, "close")) { continue; }
                const Token& ziel = s.args[k + 1];
                std::string w = ziel.s;
                if (w.size() > 1 && w.back() == ';') { w.pop_back(); }
                if (!gleich(w, alt) || ziel.datei != 0 || ziel.ausMakro) { continue; }
                const std::size_t p = d.text.find(w, ziel.anfang);
                if (p == std::string::npos || p + w.size() > ziel.ende) { continue; }
                out.push_back(Ersetzung{static_cast<std::uint32_t>(p), static_cast<std::uint32_t>(p + w.size()), neu});
            }
        }
    };
    for (const Menu& m : d.menus) {
        skripte(m.eigenschaften);
        for (const Item& it : m.items) { skripte(it.eigenschaften); }
    }
    return true;
}

std::string menuTextMitName(const Datei& d, int menu, const std::string& name) {
    std::size_t a = 0, e = 0;
    if (!menuSpanne(d, menu, a, e)) { return {}; }
    std::vector<Ersetzung> r;
    if (!setzeEigenschaft(d, menu, -1, "name", {"\"" + name + "\""}, r)) { return d.text.substr(a, e - a); }
    std::string t = d.text.substr(a, e - a);
    std::sort(r.begin(), r.end(), [](const Ersetzung& x, const Ersetzung& y) { return x.anfang > y.anfang; });
    for (const Ersetzung& x : r) {
        if (x.anfang >= a && x.ende <= e) { t.replace(x.anfang - a, x.ende - x.anfang, x.neu); }
    }
    return t;
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

bool itemAnStelle(const Datei& d, int menu, int item, int ziel, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const int n = static_cast<int>(m.items.size());
    if (item < 0 || item >= n || ziel < 0 || ziel >= n || ziel == item) { return false; }
    for (const Item& it : m.items) {
        if (it.kopf.datei != 0) { return false; }   // aus einem #include: nicht in diesem Text
    }
    std::size_t a = 0, e = 0, za = 0, ze = 0;
    itemSpanne(d, m.items[static_cast<std::size_t>(item)], a, e);
    itemSpanne(d, m.items[static_cast<std::size_t>(ziel)], za, ze);
    const std::string block = d.text.substr(a, e - a);
    // nach hinten: vor den Block des Ziels; nach vorn: hinter ihn
    if (ziel < item) {
        const std::string zwischen = d.text.substr(za, a - za);
        out.push_back(Ersetzung{static_cast<std::uint32_t>(za), static_cast<std::uint32_t>(e), block + zwischen});
    } else {
        const std::string zwischen = d.text.substr(e, ze - e);
        out.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(ze), zwischen + block});
    }
    return true;
}

bool itemsNeuOrdnen(const Datei& d, int menu, const std::vector<int>& plaetze, const std::vector<int>& neu, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size() || plaetze.size() != neu.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    const int n = static_cast<int>(m.items.size());
    std::vector<int> a(plaetze), b(neu);
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    if (a != b || std::adjacent_find(a.begin(), a.end()) != a.end() || (!a.empty() && (a.front() < 0 || a.back() >= n))) { return false; }
    for (const Item& it : m.items) {
        if (it.kopf.datei != 0) { return false; }   // aus einem #include: nicht in diesem Text
    }
    for (std::size_t k = 0; k < plaetze.size(); ++k) {
        if (plaetze[k] == neu[k]) { continue; }
        std::size_t pa = 0, pe = 0, na = 0, ne = 0;
        itemSpanne(d, m.items[static_cast<std::size_t>(plaetze[k])], pa, pe);
        itemSpanne(d, m.items[static_cast<std::size_t>(neu[k])], na, ne);
        out.push_back(Ersetzung{static_cast<std::uint32_t>(pa), static_cast<std::uint32_t>(pe), d.text.substr(na, ne - na)});
    }
    return true;
}

namespace {
// Der Text der Argumente einer Eigenschaft (wie er in der Datei steht, Leerraum vereinheitlicht)
std::string argText(const Datei& d, const Eigenschaft& e) {
    std::string r;
    for (const Token& t : e.args) {
        const std::string& q = t.datei == 0 ? d.text : (static_cast<std::size_t>(t.datei) < d.quellen.size() ? d.quellen[static_cast<std::size_t>(t.datei)].text : d.text);
        r += (r.empty() ? "" : " ") + (t.ende <= q.size() && t.anfang < t.ende ? q.substr(t.anfang, t.ende - t.anfang) : t.s);
    }
    return r;
}
}  // namespace

std::vector<std::string> itemUnterschiede(const Datei& a, const Item& ia, const Datei& b, const Item& ib) {
    std::vector<std::string> r;
    auto klein = [](std::string s) {
        for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return s;
    };
    auto werte = [&](const Datei& d, const Item& it) {
        std::vector<std::pair<std::string, std::string>> v;
        for (const Eigenschaft& e : it.eigenschaften) { v.emplace_back(klein(e.kanon.empty() ? e.schluessel : e.kanon), argText(d, e)); }
        return v;
    };
    const auto va = werte(a, ia), vb = werte(b, ib);
    auto dazu = [&](const std::string& k) {
        if (std::find(r.begin(), r.end(), k) == r.end()) { r.push_back(k); }
    };
    for (const auto& [k, w] : va) {
        if (std::count(va.begin(), va.end(), std::make_pair(k, w)) != std::count(vb.begin(), vb.end(), std::make_pair(k, w))) { dazu(k); }
    }
    for (const auto& [k, w] : vb) {
        if (std::count(va.begin(), va.end(), std::make_pair(k, w)) != std::count(vb.begin(), vb.end(), std::make_pair(k, w))) { dazu(k); }
    }
    return r;
}

bool itemErsetzen(const Datei& d, int menu, int item, const std::string& neuerText, std::vector<Ersetzung>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return false; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if (item < 0 || static_cast<std::size_t>(item) >= m.items.size() || m.items[static_cast<std::size_t>(item)].kopf.datei != 0) { return false; }
    std::size_t a = 0, e = 0;
    itemSpanne(d, m.items[static_cast<std::size_t>(item)], a, e);
    std::string t = neuerText;
    if (!t.empty() && t.back() != '\n' && e > 0 && d.text[e - 1] == '\n') { t += nlVon(d.text); }
    out.push_back(Ersetzung{static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(e), t});
    return true;
}

std::string freierName(const Menu& m, const std::string& basis) {
    std::string b = basis.empty() ? std::string("item") : basis;
    // rc608 Teil 3b: Ist der Name hier frei (Einfuegen auf einer anderen Seite), bleibt er
    if (std::none_of(m.items.begin(), m.items.end(), [&b](const Item& it) { return it.window.name == b; })) { return b; }
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
