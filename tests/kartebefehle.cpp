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
    std::string mods;
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
