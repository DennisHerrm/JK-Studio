// menuezufall.cpp - Menue-Editor M2: zufaellige, selbst gebaute Menues (Grundsatz
// "nichts auf die Originalmenues zuschneiden").
//
// Je Fassung (JA SP, JA MP, Movie Duels SP) einige hundert Menues aus allen
// Schluesselwoertern der Fassung mit Argumenten der passenden Art, Skripten
// auf vorhandene und erfundene Ziele, Gruppen und Cvars. Je Menue:
//   - Lesen und unveraendert Schreiben ist byte-gleich;
//   - die Vorschau zeichnet ohne Absturz, jeder Zeichenbefehl gehoert zu einem Item;
//   - die Diagnose liegt mit allen Stellen im Text, jede Schnellkorrektur laesst
//     sich anwenden und das Ergebnis wieder lesen;
//   - eine zufaellig entfernte } bzw. zusaetzliche } meldet die Diagnose.
#include "bhed/menue/diagnose.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/vorschau.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <random>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        if (g_fehler < 30) { std::printf("FEHL  %s\n", was.c_str()); }
    }
}

struct Bauer {
    std::mt19937 r;
    Dialekt d;
    Spiel s;
    std::vector<std::string> itemWoerter, menuWoerter;
    explicit Bauer(unsigned saat, Dialekt d0, Spiel s0) : r(saat), d(d0), s(s0) {
        itemWoerter = itemSchluessel(d, s);
        menuWoerter = menuSchluessel(d, s);
    }
    int zahl(int a, int b) { return std::uniform_int_distribution<int>(a, b)(r); }
    std::string name(const char* vor) { return std::string(vor) + std::to_string(zahl(0, 9)); }
    std::string argumente(bool item, const std::string& w, const std::vector<std::string>& namen) {
        const int n = zahlArgumente(d, s, item, w);
        std::string a;
        if (n > 0) {
            for (int i = 0; i < n; ++i) { a += " " + std::to_string(zahl(-20, 700)); }
            return a;
        }
        const std::string k = [&] {
            std::string x = w;
            for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            return x;
        }();
        if (k == "rect") { return " 10 10 100 20"; }
        if (k == "style") { return d == Dialekt::SP ? " WINDOW_STYLE_FILLED" : " 1"; }
        if (k == "type") { return d == Dialekt::SP ? " ITEM_TYPE_BUTTON" : " 1"; }
        if (k == "visible" || k == "decoration" || k == "autowrapped" || k == "wrapped" || k == "notselectable" || k == "noscrollbar") { return ""; }
        // Skripte: Befehle auf vorhandene und erfundene Ziele
        static const char* const befehle[] = {"show", "hide", "fadein", "fadeout", "setfocus", "open", "close", "setcvar ui_zufall 1", "play"};
        if (zahl(0, 3) == 0) {
            std::string sk = " {";
            const int m = zahl(1, 3);
            for (int i = 0; i < m; ++i) {
                const std::string b = befehle[zahl(0, 8)];
                sk += " " + b;
                if (b == "play") { sk += " \"sound/zufall.wav\""; }
                else if (b.find(' ') == std::string::npos) { sk += " " + (namen.empty() || zahl(0, 2) == 0 ? name("fremd") : namen[static_cast<std::size_t>(zahl(0, static_cast<int>(namen.size()) - 1))]); }
                sk += " ;";
            }
            return sk + " }";
        }
        return " \"" + name("wert") + "\"";
    }
    std::string menue() {
        std::string t = "{\n\tmenuDef\n\t{\n\t\tname " + name("menue") + "\n\t\trect 0 0 640 480\n";
        std::vector<std::string> namen;
        const int items = zahl(0, 8);
        for (int i = 0; i < items; ++i) { namen.push_back("item" + std::to_string(i)); }
        for (int k = zahl(0, 3); k > 0; --k) {
            const std::string w = menuWoerter[static_cast<std::size_t>(zahl(0, static_cast<int>(menuWoerter.size()) - 1))];
            if (w == "name" || w == "rect" || w == "itemDef") { continue; }
            t += "\t\t" + w + argumente(false, w, namen) + "\n";
        }
        for (int i = 0; i < items; ++i) {
            t += "\t\titemDef\n\t\t{\n\t\t\tname " + namen[static_cast<std::size_t>(i)] + "\n";
            if (zahl(0, 2) == 0) { t += "\t\t\tgroup " + name("gruppe") + "\n"; }
            t += "\t\t\trect " + std::to_string(zahl(-50, 650)) + " " + std::to_string(zahl(-50, 500)) + " " + std::to_string(zahl(0, 200)) + " " +
                 std::to_string(zahl(0, 100)) + "\n";
            for (int k = zahl(0, 6); k > 0; --k) {
                const std::string w = itemWoerter[static_cast<std::size_t>(zahl(0, static_cast<int>(itemWoerter.size()) - 1))];
                if (w == "name" || w == "rect" || w == "group") { continue; }
                t += "\t\t\t" + w + argumente(true, w, namen) + "\n";
            }
            t += "\t\t}\n";
        }
        return t + "\t}\n}\n";
    }
};

void fassung(const char* titel, Dialekt d, Spiel s, int anzahl) {
    Bauer b(1234U + static_cast<unsigned>(s) * 7U + static_cast<unsigned>(d), d, s);
    LeseOptionen o;
    o.dialekt = d;
    o.spiel = s;
    const std::string pfad = d == Dialekt::MP ? "ui/jamp/zufall.menu" : "ui/zufall.menu";
    int diagnosen = 0, korrekturen = 0, klammerGemeldet = 0, klammerProben = 0;
    for (int n = 0; n < anzahl; ++n) {
        const std::string t = b.menue();
        const Datei dt = lies(pfad, t, o);
        pruefe(schreib(dt) == t, std::string(titel) + ": byte-gleich #" + std::to_string(n));
        Umgebung u;
        u.dialekt = d;
        for (std::size_t mi = 0; mi < dt.menus.size(); ++mi) {
            std::vector<Prim> prims;
            zeichneMenu(dt, static_cast<int>(mi), u, prims);
            for (const Prim& p : prims) {
                pruefe(p.item < static_cast<int>(dt.menus[mi].items.size()), std::string(titel) + ": Zeichenbefehl gehoert zu einem Item #" + std::to_string(n));
            }
        }
        const std::vector<Diagnose> v = diagnose(dt, DiagKontext{});
        diagnosen += static_cast<int>(v.size());
        for (const Diagnose& x : v) {
            pruefe(x.ende <= t.size() && x.anfang <= x.ende && (!x.partner || x.partnerEnde <= t.size()),
                   std::string(titel) + ": Stelle im Text #" + std::to_string(n));
            for (const Korrektur& k : x.korrekturen) {
                if (k.ende > t.size() || k.anfang > k.ende) {
                    pruefe(false, std::string(titel) + ": Korrektur im Text #" + std::to_string(n));
                    continue;
                }
                const std::string neu = t.substr(0, k.anfang) + k.text + t.substr(k.ende);
                const Datei d2 = lies(pfad, neu, o);
                pruefe(schreib(d2) == neu, std::string(titel) + ": korrigiert wieder lesbar #" + std::to_string(n));
                ++korrekturen;
            }
        }
        // eine } entfernen bzw. eine dazu: die Diagnose meldet eine Klammer
        std::vector<std::size_t> zu;
        for (std::size_t i = 0; i < t.size(); ++i) {
            if (t[i] == '}' && (i + 1 >= t.size() || t[i + 1] == '\n')) { zu.push_back(i); }
        }
        if (!zu.empty()) {
            const std::size_t p = zu[static_cast<std::size_t>(b.zahl(0, static_cast<int>(zu.size()) - 1))];
            for (const bool weg : {true, false}) {
                const std::string kaputt = weg ? t.substr(0, p) + t.substr(p + 1) : t.substr(0, p) + "}" + t.substr(p);
                bool gemeldet = false;
                for (const Diagnose& x : diagnose(lies(pfad, kaputt, o), DiagKontext{})) {
                    gemeldet = gemeldet || x.art == DiagArt::KlammerOffen || x.art == DiagArt::KlammerZuViel || x.art == DiagArt::KlammerFalsch;
                }
                ++klammerProben;
                klammerGemeldet += gemeldet ? 1 : 0;
                pruefe(gemeldet, std::string(titel) + (weg ? ": fehlende } gemeldet #" : ": zusaetzliche } gemeldet #") + std::to_string(n));
            }
        }
    }
    std::printf("%s: %d Menues, %d Diagnosen, %d Korrekturen geprueft, Klammer %d/%d gemeldet\n", titel, anzahl, diagnosen, korrekturen,
                klammerGemeldet, klammerProben);
}
}  // namespace

int main() {
    fassung("JA SP", Dialekt::SP, Spiel::JA, 300);
    fassung("JA MP", Dialekt::MP, Spiel::JA, 300);
    fassung("MD SP", Dialekt::SP, Spiel::MD, 300);
    std::printf("menuezufall: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
