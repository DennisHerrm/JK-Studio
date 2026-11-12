// kartebefehle.cpp - Karten-Editor Stufe 1b, Punkte 3 und 4: die Befehlstabelle
// (bhed/karte/befehle.h) gegen die Checkliste aus dem NRC-Quelltext
// (tools/karte/nrc_inventar.md) und die Tastenkuerzel wie Qt.
//
// Geprueft wird:
//   - jeder registrierte Befehl der Checkliste steht in der Tabelle, mit NRCs
//     Standardtaste (auch die Qt::-Ausdruecke fuer den Ziffernblock);
//   - jeder Menuepunkt und jeder Werkzeugknopf der Checkliste steht in den
//     Menues/Leisten der Tabelle, mit derselben Beschriftung und demselben
//     Befehl;
//   - die englische Uebersetzung jedes Eintrags ist NRCs Text ohne '&'
//     ("Begriffe wie NetRadiant");
//   - QKeySequence: Text <-> Taste wie Qt ("Escape" liest sich, "Esc" wird
//     geschrieben; "Shift+Num++"; "Ctrl+\");
//   - possibleKeys/QShortcutMap: auf einer deutschen Tastatur ist "[" AltGr+8,
//     Shift+AltGr+8 ist "Shift+[", Ctrl+AltGr+8 bleibt "[" (wie in NRC); auf
//     einer US-Tastatur ist Ctrl+[ "Ctrl+["; zwei Befehle mit derselben Taste
//     sind mehrdeutig.
#include "bhed/karte/befehle.h"
#include "bhed/karte/zuletzt.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;
using namespace bhed::karte::befehle;
using bhed::i18n::Language;
using bhed::i18n::Str;
using bhed::i18n::trIn;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string qtAusdruck(const std::string& a) {
    // "+Qt::SHIFT + Qt::Key_Plus + Qt::KeypadModifier" -> "Shift+Num++"
    std::string taste;
    std::stringstream s(a);
    std::string teil;
    bool ctrl = false, shift = false, num = false;
    while (std::getline(s, teil, '+')) {
        const auto a0 = teil.find_first_not_of(' ');
        if (a0 == std::string::npos) { continue; }
        teil = teil.substr(a0, teil.find_last_not_of(' ') - a0 + 1);
        if (teil == "Qt::CTRL") { ctrl = true; }
        else if (teil == "Qt::SHIFT") { shift = true; }
        else if (teil == "Qt::KeypadModifier") { num = true; }
        else if (teil == "Qt::Key_Plus") { taste = "+"; }
        else if (teil == "Qt::Key_Minus") { taste = "-"; }
        else if (teil.rfind("Qt::Key_", 0) == 0) { taste = teil.substr(8); }
    }
    return std::string(ctrl ? "Ctrl+" : "") + (shift ? "Shift+" : "") + (num ? "Num+" : "") + taste;
}

std::string cZeichenkette(const std::string& s) {   // "Ctrl+\\" aus dem Quelltext -> "Ctrl+\"
    std::string o;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) { ++i; }
        o.push_back(s[i]);
    }
    return o;
}

void checkliste(const std::string& pfad) {
    std::ifstream f(pfad);
    pruefe(f.good(), "Checkliste lesbar: " + pfad);
    std::string zeile;
    std::string abschnitt;
    std::map<std::string, std::string> tasteSoll;   // Name -> erste nicht leere Taste
    std::set<std::string> namen;
    std::vector<std::pair<std::string, std::string>> menuePunkte;   // Beschriftung, Befehl
    std::vector<std::pair<std::string, std::string>> knoepfe;       // Beschreibung (1. Zeile), Befehl
    const std::regex reBefehl(R"(^- \[.\] `([^`]+)` \| (\w+) \| (.*?) \| \()");
    const std::regex reMenue(R"(^\s*- \[.\] (ITEM|CHECK) (.*?)(?: \[wenn: .*\])?(?: -> `([^`]+)`)?  \()");
    const std::regex reKnopf(R"(^- \[.\] (?:toggle_button|button) (.*?)(?: \[wenn: .*\])? \| [^|]* -> `([^`]+)`)");
    while (std::getline(f, zeile)) {
        if (zeile.rfind("## ", 0) == 0) {
            abschnitt = zeile.substr(3);
            continue;
        }
        std::smatch m;
        if (abschnitt == "Registrierte Befehle" && std::regex_search(zeile, m, reBefehl)) {
            std::string taste = m[3].str() == "-" ? std::string() : cZeichenkette(m[3].str());
            if (taste.find("Qt::") != std::string::npos) { taste = qtAusdruck(taste); }
            namen.insert(m[1].str());
            if (!taste.empty() && tasteSoll.count(m[1].str()) == 0U) { tasteSoll[m[1].str()] = taste; }
        } else if (abschnitt == "Menues" && std::regex_search(zeile, m, reMenue)) {
            menuePunkte.emplace_back(m[2].str(), m[3].str());
        } else if (abschnitt == "Werkzeugleisten" && std::regex_search(zeile, m, reKnopf)) {
            knoepfe.emplace_back(m[1].str(), m[2].str());
        }
    }
    std::printf("Checkliste: %zu Befehle, %zu Menuepunkte, %zu Knoepfe\n", namen.size(), menuePunkte.size(),
                knoepfe.size());
    pruefe(namen.size() > 280 && menuePunkte.size() > 200 && knoepfe.size() > 50, "Checkliste vollstaendig gelesen");
    // 1. jeder Befehl mit seiner Taste
    for (const std::string& n : namen) {
        const Befehl* b = finden(n);
        pruefe(b != nullptr, "Befehl in der Tabelle: " + n);
        if (b == nullptr) { continue; }
        const auto it = tasteSoll.find(n);
        const Taste soll = it == tasteSoll.end() ? 0 : tasteAusText(it->second);
        pruefe(it == tasteSoll.end() || soll != 0, "Taste lesbar: " + n + " " + (it == tasteSoll.end() ? "" : it->second));
        pruefe(tasteAusText(b->taste) == soll, std::string("Standardtaste ") + n + ": Tabelle \"" + b->taste +
                                                   "\", NRC \"" + (it == tasteSoll.end() ? "" : it->second) + "\"");
    }
    pruefe(alle().size() == namen.size(), "Tabelle hat genau die Befehle der Checkliste (" + std::to_string(alle().size()) +
                                               " / " + std::to_string(namen.size()) + ")");
    // 2. jeder Menuepunkt
    std::vector<const MenueEintrag*> tabelle;
    for (const MenueEintrag& e : menues()) { tabelle.push_back(&e); }
    for (const MenueEintrag& e : texturMenues()) { tabelle.push_back(&e); }
    for (const auto& [text, befehl] : menuePunkte) {
        if (text.size() == 2 && text[0] == '&' && text[1] >= '1' && text[1] <= '9' && befehl.empty()) {
            continue;   // MRU-Eintraege "&1" .. "&9" (dynamisch)
        }
        if (text == "(dynamisch)") { continue; }   // Repeat-Eintraege (SelectionSystem_connectTransformsCallbacks)
        bool da = false;
        for (const MenueEintrag* e : tabelle) {
            if (text == e->text && (befehl.empty() ? e->art == MenueArt::Ruf : befehl == e->befehl)) { da = true; }
        }
        pruefe(da, "Menuepunkt in der Tabelle: \"" + text + "\" -> " + befehl);
    }
    // 3. jeder Werkzeugknopf
    std::vector<const Knopf*> leisten;
    for (const Knopf& k : werkzeugleiste()) { leisten.push_back(&k); }
    for (const Knopf& k : filterleiste()) { leisten.push_back(&k); }
    for (const Knopf& k : texturKnoepfe()) { leisten.push_back(&k); }
    for (const auto& [beschr, befehl] : knoepfe) {
        bool da = false;
        for (const Knopf* k : leisten) {
            // Die Checkliste schreibt Zeilenwechsel als " / " ohne Tabulatoren.
            std::string text;
            for (const char c : std::string(k->beschreibung)) {
                if (c == '\n') {
                    text += " / ";
                } else if (c != '\t') {
                    text.push_back(c);
                }
            }
            if (!k->trenner && befehl == k->befehl && beschr == text) { da = true; }
        }
        pruefe(da, "Knopf in der Leiste: \"" + beschr + "\" -> " + befehl);
    }
}

void uebersetzungen() {
    std::size_t n = 0;
    for (const auto* liste : {&menues(), &texturMenues()}) {
        for (const MenueEintrag& e : *liste) {
            if (e.str == Str::Count) { continue; }
            ++n;
            pruefe(trIn(Language::English, e.str) == ohneMnemonik(e.text),
                   std::string("Englisch = NRC: \"") + e.text + "\" -> \"" + trIn(Language::English, e.str) + "\"");
            pruefe(std::string(trIn(Language::German, e.str)).size() > 0 &&
                       std::string(trIn(Language::ChineseSimplified, e.str)).size() > 0 &&
                       std::string(trIn(Language::Japanese, e.str)).size() > 0,
                   std::string("alle Sprachen: ") + e.text);
        }
    }
    for (const auto* liste : {&werkzeugleiste(), &filterleiste(), &texturKnoepfe()}) {
        for (const Knopf& k : *liste) {
            if (k.trenner) { continue; }
            std::stringstream s(k.beschreibung);
            std::string z;
            std::size_t i = 0;
            while (std::getline(s, z)) {
                const auto a = z.find_first_not_of(" \t");
                z = a == std::string::npos ? std::string() : z.substr(a);
                if (i < k.zeilen.size() && k.zeilen[i] != Str::Count) {
                    ++n;
                    pruefe(trIn(Language::English, k.zeilen[i]) == z, "Knopftext Englisch = NRC: " + z);
                }
                ++i;
            }
        }
    }
    std::printf("Uebersetzungen: %zu Eintraege geprueft\n", n);
}

void tasten() {
    pruefe(tasteAusText("Escape") == kKeyEscape && tasteAlsText(kKeyEscape) == "Esc", "Escape liest sich, Esc wird geschrieben");
    pruefe(tasteAlsText(tasteAusText("Delete")) == "Del" && tasteAlsText(tasteAusText("Insert")) == "Ins",
           "Delete/Insert -> Del/Ins (toString)");
    pruefe(tasteAlsText(tasteAusText("PgDown")) == "PgDown" && tasteAusText("Page Down") == kKeyPageDown, "PgDown");
    pruefe(tasteAusText("Ctrl+Shift+P") == (kCtrl | kShift | 'P') && tasteAlsText(kCtrl | kShift | 'P') == "Ctrl+Shift+P",
           "Ctrl+Shift+P");
    pruefe(tasteAusText("Shift+Num++") == (kShift | kNum | '+') && tasteAlsText(kShift | kNum | '+') == "Shift+Num++",
           "Shift+Num++ (Ziffernblock-Plus)");
    pruefe(tasteAusText("Ctrl+\\") == (kCtrl | '\\'), "Ctrl+\\");
    pruefe(tasteAusText("shift+[") == (kShift | '['), "Kleinschreibung der Zusatztasten");
    pruefe(tasteAusText("F11") == kKeyF1 + 10 && tasteAlsText(kKeyF1 + 10) == "F11", "F11");
    pruefe(tasteAusText("Space") == 0x20 && tasteAlsText(0x20) == "Space", "Space");
    pruefe(tasteAusText("Bogus") == 0 && tasteAusText("") == 0, "unbekannt/leer = 0");
    pruefe(tasteAusText("`") == 0x60 && tasteAusText("/") == '/', "` und /");
    for (const Befehl& b : alle()) {
        if (b.taste[0] == 0) { continue; }
        const Taste t = tasteAusText(b.taste);
        pruefe(t != 0 && tasteAusText(tasteAlsText(t)) == t, std::string("Rundlauf Text/Taste: ") + b.name + " " + b.taste);
    }

    // Deutsche Tastatur, Taste "8": 8 ( 8 ( 8 ( [ ( | Rueckfall 8
    TastenBelegung acht;
    acht.vorhanden = true;
    acht.qtKey = {'8', '(', '8', '(', '8', '(', '[', '(', '8'};
    Belegung bel;
    const auto t1 = bel.treffer(moeglicheTasten(acht, kCtrl | kAlt));   // AltGr+8
    pruefe(t1.size() == 1 && t1[0] == "GridDown", "deutsch AltGr+8 = \"[\" = GridDown");
    const auto t2 = bel.treffer(moeglicheTasten(acht, kShift | kCtrl | kAlt));   // Shift+AltGr+8
    pruefe(t2.size() == 1 && t2[0] == "CameraModePrev", "deutsch Shift+AltGr+8 = \"Shift+[\" = CameraModePrev");
    const auto t3 = bel.treffer(moeglicheTasten(acht, 0));
    pruefe(t3.size() == 1 && t3[0] == "SetGrid128", "deutsch 8 = SetGrid128");
    // US-Tastatur, Taste "[": [ { 0 0 [ { [ { 0
    TastenBelegung klammer;
    klammer.vorhanden = true;
    klammer.qtKey = {'[', '{', 0, 0, '[', '{', '[', '{', 0};
    const auto t4 = bel.treffer(moeglicheTasten(klammer, kCtrl));
    pruefe(t4.size() == 1 && t4[0] == "CubicClipZoomIn", "US Ctrl+[ = CubicClipZoomIn");
    const auto t5 = bel.treffer(moeglicheTasten(klammer, kShift));
    pruefe(t5.size() == 1 && t5[0] == "CameraModePrev", "US Shift+[ ({) = CameraModePrev");
    // Deutsch "7": Shift+7 = "/" = ToggleModelBrowser
    TastenBelegung sieben;
    sieben.vorhanden = true;
    sieben.qtKey = {'7', '/', '7', '/', '7', '/', '{', '/', '7'};
    const auto t6 = bel.treffer(moeglicheTasten(sieben, kShift));
    pruefe(t6.size() == 1 && t6[0] == "ToggleModelBrowser", "deutsch Shift+7 = \"/\" = ToggleModelBrowser");
    // Ziffernblock: Shift+Num+ -> CameraSpeedInc
    TastenBelegung plus;
    plus.vorhanden = true;
    plus.qtKey = {'+', '+', '+', '+', '+', '+', '+', '+', '+'};
    const auto t7 = bel.treffer(moeglicheTasten(plus, kShift | kNum));
    pruefe(t7.size() == 1 && t7[0] == "CameraSpeedInc", "Shift+Ziffernblock-Plus = CameraSpeedInc");
    // rc612 NUMPAD (Vorgabe): der Ziffernblock tut, was die obere Ziffernreihe tut - Num 1..9 = SetGrid1..SetGrid256,
    // Num 0 = ToggleGrid (NRC: grid.cpp:192-200, xywindow.cpp:2216)
    {
        static const char* const kSoll[10] = {"ToggleGrid", "SetGrid1",  "SetGrid2",  "SetGrid4",   "SetGrid8",
                                              "SetGrid16",  "SetGrid32", "SetGrid64", "SetGrid128", "SetGrid256"};
        for (int d = 0; d <= 9; ++d) {
            TastenBelegung num;
            num.vorhanden = true;
            num.qtKey.fill(static_cast<std::uint32_t>('0' + d));
            const auto t = bel.treffer(moeglicheTasten(num, kNum));
            pruefe(t.size() == 1 && t[0] == kSoll[d], std::string("Vorgabe: Num ") + std::to_string(d) + " = " + kSoll[d]);
        }
    }
    // rc612 NUMPAD: Zweittaste, Schalter und eigene Belegungen
    {
        const auto num = [](int d) {
            TastenBelegung kb;
            kb.vorhanden = true;
            kb.qtKey.fill(static_cast<std::uint32_t>('0' + d));
            return moeglicheTasten(kb, kNum);
        };
        const auto allein = [&num](const Belegung& b, int d, const char* soll) {
            const auto t = b.treffer(num(d));
            return t.size() == 1 && t[0] == soll;
        };
        Belegung b;
        pruefe(b.ziffernblockRaster() && b.text("SetGrid16") == "5 / Num+5" && b.text("ToggleGrid") == "0 / Num+0" &&
                   b.text("ViewFront").empty() && b.text("ViewSide").empty() && b.text("ViewTop").empty() && b.text("NextView") == "Ctrl+Tab",
               "Vorgabe: Anzeige \"5 / Num+5\", die Ansichten ohne Taste, NextView Ctrl+Tab");
        pruefe(b.belegtVon(kNum | '5', "Zoom100") == "SetGrid16" && b.abweichende().empty() && !b.geaendert("SetGrid16"),
               "Vorgabe: belegtVon kennt die Zweittaste, nichts weicht ab");
        pruefe(b.standardText("SetGrid16") == "5 / Num+5" && !b.vorgabeWieNrc("SetGrid16") && !b.vorgabeWieNrc("ViewTop") &&
                   b.vorgabeWieNrc("NextView"),
               "Vorgabe: SetGrid16 und ViewTop weichen von NRCs Vorgabe ab, NextView nicht");
        // Schalter: wie NetRadiant-Custom (xywindow.cpp:2273-2275)
        b.ziffernblockRaster(false);
        pruefe(!b.ziffernblockRaster() && allein(b, 1, "ViewFront") && allein(b, 3, "ViewSide") && allein(b, 7, "ViewTop") &&
                   b.treffer(num(5)).empty() && b.treffer(num(0)).empty() && b.text("SetGrid16") == "5" && b.text("ViewTop") == "Num+7" &&
                   b.abweichende().empty() && b.vorgabeWieNrc("SetGrid16") && b.vorgabeWieNrc("ViewTop") && b.standardText("ViewTop") == "Num+7",
               "Schalter aus: Num 1/3/7 = ViewFront/ViewSide/ViewTop, Num 5 und Num 0 ohne Befehl, nichts weicht ab, Vorgaben wie NRC");
        b.ziffernblockRaster(true);
        pruefe(allein(b, 1, "SetGrid1") && allein(b, 7, "SetGrid64") && b.abweichende().empty(), "Schalter wieder an: die Vorgabe ist zurueck");
        // "Yes" bei "Key already used": nur die Zweittaste weicht, gespeichert als "<Befehl>#Num"
        b.loesen("SetGrid16", kNum | '5');
        b.setzen("Zoom100", kNum | '5');
        const auto ab = b.abweichende();
        pruefe(allein(b, 5, "Zoom100") && b.text("SetGrid16") == "5" && b.geaendert("SetGrid16") && ab.size() == 2 &&
                   ab[0] == std::pair<std::string, std::string>("SetGrid16#Num", "") &&
                   ab[1] == std::pair<std::string, std::string>("Zoom100", "Num+5"),
               "Zoom100 auf Num+5: SetGrid16 behaelt \"5\", abweichend SetGrid16#Num= und Zoom100=Num+5");
        Belegung c;
        (void)c.laden(ab);
        pruefe(allein(c, 5, "Zoom100") && c.text("SetGrid16") == "5" && c.text("SetGrid32") == "6 / Num+6" && c.abweichende() == ab,
               "Rundlauf: laden(abweichende) stellt beide Tasten her");
        // Schalter hin und zurueck: die eigene Taste bleibt, die Vorgabe des Rasters weicht ihr
        b.ziffernblockRaster(false);
        pruefe(allein(b, 5, "Zoom100") && allein(b, 1, "ViewFront"), "eigene Num-Taste, Schalter aus: Zoom100 bleibt auf Num 5");
        b.ziffernblockRaster(true);
        pruefe(allein(b, 5, "Zoom100") && b.text("SetGrid16") == "5" && allein(b, 6, "SetGrid32"),
               "eigene Num-Taste, Schalter wieder an: Num 5 bleibt Zoom100 (nicht mehrdeutig), SetGrid16 nur \"5\"");
        // Eigene Taste auf Num+1 im Raster-Modus, dann wie NRC: ViewFront weicht der eigenen Taste
        Belegung d;
        d.loesen("SetGrid1", kNum | '1');
        d.setzen("Zoom100", kNum | '1');
        d.ziffernblockRaster(false);
        pruefe(allein(d, 1, "Zoom100") && d.text("ViewFront").empty() && allein(d, 3, "ViewSide"),
               "eigene Taste Num+1, Schalter aus: Num 1 bleibt Zoom100, ViewFront ohne Taste, Num 3 = ViewSide");
        // Einstellungen von vor rc612 (kein Schalter gespeichert): wie NRC laden, dann die Vorgabe - eigene Tasten behalten ihre Wirkung
        Belegung alt;
        alt.ziffernblockRaster(false);
        (void)alt.laden({{"Zoom100", "Num+5"}, {"ViewTop", "F9"}});
        alt.ziffernblockRaster(true);
        pruefe(allein(alt, 5, "Zoom100") && alt.text("SetGrid16") == "5" && alt.text("SetGrid32") == "6 / Num+6" && alt.text("ViewTop") == "F9" &&
                   alt.text("ViewFront").empty() && allein(alt, 1, "SetGrid1"),
               "Belegung von rc611 (Zoom100=Num+5, ViewTop=F9): beide bleiben, die uebrigen Num-Tasten setzen das Raster");
        // Dieselbe Taste zweimal am selben Befehl wird einmal gezeigt; Clear leert beide; Reset stellt beide her
        Belegung e;
        e.setzen("SetGrid16", kNum | '5');
        pruefe(e.text("SetGrid16") == "Num+5" && allein(e, 5, "SetGrid16"), "SetGrid16 auf Num+5: Anzeige \"Num+5\", nicht doppelt");
        e.leeren("SetGrid16");
        pruefe(e.text("SetGrid16").empty() && e.treffer(num(5)).empty() && e.abweichende().size() == 2, "Clear: beide Tasten leer, beide abweichend");
        e.zuruecksetzen("SetGrid16");
        pruefe(e.text("SetGrid16") == "5 / Num+5" && e.abweichende().empty(), "Reset: beide Tasten wieder da");
        e.loesen("ToggleGrid", kNum | '0');
        e.setzen("NextView", 'Q');
        e.alleZuruecksetzen();
        pruefe(e.text("ToggleGrid") == "0 / Num+0" && e.text("NextView") == "Ctrl+Tab" && e.abweichende().empty(), "Reset All: beide Tasten aller Befehle");
        // Eine Zweittaste aus der Einstellung (von Hand eingetragen) wird gelesen; eine unlesbare gemeldet
        Belegung f;
        const auto m = f.laden({{"Zoom100#Num", "Num+5"}, {"SetGrid16#Num", ""}, {"ZoomIn#Num", "Kaputt"}, {"Gibts#Num", "X"}});
        pruefe(allein(f, 5, "Zoom100") && f.zweitTaste("Zoom100") == (kNum | '5') && f.taste("Zoom100") == 0 && m.size() == 1 &&
                   m[0].find("ZoomIn#Num") != std::string::npos,
               "laden: \"<Befehl>#Num\" setzt die Zweittaste, unlesbar wird gemeldet, unbekannt uebergangen");
        // KLEIN (rc612): die Zweittaste einzeln setzen (Edit > Shortcuts, "Edit 2nd") - die erste bleibt, gesichert wird nur "#Num"
        Belegung g;
        g.setzenZweit("SetGrid16", kNum | '8');
        const auto abw = g.abweichende();
        pruefe(g.text("SetGrid16") == "5 / Num+8" && g.taste("SetGrid16") == '5' && g.zweitTaste("SetGrid16") == (kNum | '8') && abw.size() == 1 &&
                   abw[0].first == "SetGrid16#Num" && abw[0].second == "Num+8" && g.belegtVon(kNum | '8', "SetGrid16") == "SetGrid128",
               "setzenZweit: nur die Zweittaste aendert sich, gesichert als \"SetGrid16#Num=Num+8\"; SetGrid128 hat Num+8 noch (Rueckfrage im Dialog)");
        g.setzenZweit("Zoom100", kNum | '5');
        Belegung h;
        (void)h.laden(g.abweichende());
        pruefe(h.text("SetGrid16") == "5 / Num+8" && h.text("Zoom100") == "Num+5" && h.taste("Zoom100") == 0 && h.abweichende() == g.abweichende(),
               "setzenZweit: ein Befehl ohne Taste bekommt nur eine zweite; nach Sichern und Laden dieselbe Belegung");
        g.zuruecksetzen("SetGrid16");
        g.setzenZweit("Gibts", 'X');
        pruefe(g.text("SetGrid16") == "5 / Num+5" && g.abweichende().size() == 1, "setzenZweit: Reset holt die Vorgabe zurueck, ein unbekannter Befehl aendert nichts");
    }
    // Mehrdeutig: zwei Befehle auf derselben Taste
    Belegung zwei;
    zwei.setzen("ZoomIn", kKeyInsert);   // ZoomOut hat schon Insert
    pruefe(zwei.treffer({kKeyInsert}).size() == 2, "zwei Befehle auf Insert: mehrdeutig");
    pruefe(zwei.belegtVon(kKeyInsert, "ZoomIn") == "ZoomOut", "belegtVon nennt den anderen");
    zwei.zuruecksetzen("ZoomIn");
    pruefe(zwei.taste("ZoomIn") == 0 && zwei.treffer({kKeyInsert}).size() == 1, "Reset: wieder die Vorgabe");
    // Eigene Belegung laden/abweichende
    Belegung b3;
    const auto meld = b3.laden({{"MapInfo", "Ctrl+Shift+M"}, {"Preferences", ""}, {"Undo", "Kaputt+Taste"}, {"Gibts", "X"}});
    pruefe(b3.taste("MapInfo") == (kCtrl | kShift | 'M') && b3.taste("Preferences") == 0 && b3.taste("Undo") == (kCtrl | 'Z'),
           "eigene Belegung: gesetzt, geleert, unlesbar bleibt Vorgabe");
    pruefe(meld.size() == 1 && meld[0].find("failed to parse user command \"Undo\"") != std::string::npos,
           "Meldung wie ReadCommandMap");
    pruefe(b3.abweichende().size() == 2, "zwei abweichende Belegungen");
    // Tooltip wie toolbar_action_set_tooltip
    pruefe(knopfTooltip({"Structural", "RightClick: MakeStructural"}, "Ctrl+Shift+D") ==
               "Structural (Ctrl+Shift+D)\nRightClick: MakeStructural",
           "Tooltip: erste Zeile + (Taste) + Rest");
    // Anordnungen
    pruefe(bedingungGilt(kNurFloating, Anordnung::Floating, false) && !bedingungGilt(kNurFloating, Anordnung::Regular, false) &&
               bedingungGilt(kTexturen, Anordnung::Regular, true) && !bedingungGilt(kTexturen, Anordnung::RegularLeft, false) &&
               bedingungGilt(kNurSplit, Anordnung::Split, false) && !bedingungGilt(kNichtSplit, Anordnung::Split, false),
           "Bedingungen der Fensteranordnung");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2) { checkliste(argv[1]); }
    uebersetzungen();
    tasten();
    std::printf("kartebefehle: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
