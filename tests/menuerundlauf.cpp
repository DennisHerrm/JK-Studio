// menuerundlauf.cpp - Menue-Editor M1 Punkt 1: alle Originalmenues lesen.
//
// Aufruf: menuerundlauf <JA base> <JK2 base> [<MD base> <MD>]
// (leere Pfade werden uebersprungen; ohne Spielordner nur ein Hinweis).
//
// Je Spiel alle ui/**.menu und ui/*.txt aus den pk3 (nur lesen):
//   - lesen wie die Engine (Dialekt nach dem Pfad, MP mit menudef.h),
//   - Schreiben ohne Aenderung byte-gleich,
//   - rect des ersten freien Items um 1 verschieben: nur diese Zahl aendert
//     sich, neu gelesen stimmt der Wert, Anzahl der Menues/Items bleibt,
//   - Zaehler: Menues, Items, Meldungen der Engine, @-Texte ohne Eintrag.
#include "bhed/menue/diagnose.h"
#include "bhed/menue/feeder.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/spieldaten.h"
#include "bhed/shaderscript.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace bhed::menue;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

void spiel(const char* name, Spiel s, const std::vector<std::string>& ordner) {
    Spieldaten daten;
    for (const std::string& o : ordner) { daten.ordnerDazu(o); }
    const std::vector<std::string> dateien = daten.liste("ui/", {".menu", ".txt"});
    if (dateien.empty()) {
        std::printf("%s: keine Menues gefunden (%zu Ordner)\n", name, ordner.size());
        return;
    }
    const Texte texte = ladeTexte(daten, s);
    int menus = 0, items = 0, fehler = 0, warnungen = 0, gescheitert = 0, ohneText = 0, verschoben = 0;
    std::map<std::string, int> unbekannt;
    int gezeigt = 0;
    // M2 Live-Diagnose ueber alle Originalmenues (Mass fuer Fehlalarme): erst alle Menuenamen
    std::set<std::string> alleMenues;
    for (const std::string& pfad : dateien) {
        std::string text;
        if (!daten.lies(pfad, text)) { continue; }
        for (const Menu& m : lies(pfad, text, optionenFuer(pfad, s, daten.lader())).menus) {
            std::string n = m.window.name;
            for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            alleMenues.insert(n);
        }
    }
    DiagKontext dk;
    dk.menueBekannt = [&](const std::string& n0) {
        std::string n = n0;
        for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return alleMenues.count(n) != 0;
    };
    dk.pruefung.gibt = [&](const std::string& p) { return daten.gibt(p); };
    bhed::ShaderMap shader;   // Bild oder Shader (R_RegisterShaderNoMip)
    for (const std::string& sp : daten.liste("shaders/", {".shader"})) {
        std::string st;
        if (daten.lies(sp, st)) { bhed::parseShaderScript(st, shader); }
    }
    dk.pruefung.gibtBild = [&](const std::string& p0) {
        std::string p = p0;
        for (char& c : p) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        const std::size_t pt = p.rfind('.');
        if (shader.count(p) != 0 || (pt != std::string::npos && shader.count(p.substr(0, pt)) != 0)) { return true; }
        for (const char* e : {"", ".tga", ".jpg", ".png"}) {
            if (daten.gibt(p + e)) { return true; }
        }
        return false;
    };
    std::map<std::string, int> diagArten;
    int diagGezeigt = 0;
    for (const std::string& pfad : dateien) {
        std::string text;
        if (!daten.lies(pfad, text)) {
            pruefe(false, std::string(name) + ": nicht lesbar " + pfad);
            continue;
        }
        const LeseOptionen o = optionenFuer(pfad, s, daten.lader());
        const Datei d = lies(pfad, text, o);
        pruefe(schreib(d) == text, std::string(name) + ": byte-gleich " + pfad);
        for (const Diagnose& x : diagnose(d, dk)) {
            static const char* const kArt[] = {"Engine", "KlammerOffen", "KlammerZuViel", "KlammerFalsch", "AnfuehrungOffen", "KeineZahl",
                                               "NameDoppelt", "ZielFehlt", "CvarAehnlich", "Pruefung"};
            std::string a = kArt[static_cast<int>(x.art)];
            if (x.art == DiagArt::Pruefung) { a += "/" + std::to_string(static_cast<int>(x.befund.art)); }
            ++diagArten[a];
            if (std::getenv("MENUE_DIAG_ZEIGE") != nullptr && a.find(std::getenv("MENUE_DIAG_ZEIGE")) == 0 && diagGezeigt < 40) {
                std::printf("   diag %s %s:%d %s %s\n", a.c_str(), pfad.c_str(), x.zeile, x.text.c_str(), x.wert.c_str());
                ++diagGezeigt;
            }
        }
        menus += static_cast<int>(d.menus.size());
        for (const Menu& m : d.menus) {
            items += static_cast<int>(m.items.size());
            if (!m.ok) { ++gescheitert; }
            for (const Item& it : m.items) {
                for (const std::string* t : {&it.text, &it.descText}) {
                    if (t->size() > 1 && (*t)[0] == '@' && !texte.kennt(*t)) {
                        ++ohneText;
                        ++unbekannt[*t];
                    }
                }
            }
        }
        for (const Meldung& m : d.meldungen) {
            (m.fehler ? fehler : warnungen) += 1;
            if (gezeigt < 60) {
                std::printf("   %s\n", meldungText(d, m).c_str());
                ++gezeigt;
            }
        }
        // rect des ersten freien Items verschieben
        bool fertig = false;
        for (std::size_t mi = 0; mi < d.menus.size() && !fertig; ++mi) {
            const Menu& m = d.menus[mi];
            for (std::size_t ii = 0; ii < m.items.size() && !fertig; ++ii) {
                const Item& it = m.items[ii];
                if (finde(it, "rect") == nullptr || !it.ok) { continue; }
                Rechteck r = it.window.rectClient;
                r.x += 1.0F;
                std::vector<Ersetzung> e;
                if (!setzeRect(d, static_cast<int>(mi), static_cast<int>(ii), r, e)) { continue; }
                fertig = true;
                const std::string neu = anwenden(d.text, e);
                const Datei d2 = lies(pfad, neu, o);
                const bool gleichViele = d2.menus.size() == d.menus.size() && d2.menus[mi].items.size() == m.items.size();
                pruefe(gleichViele, std::string(name) + ": nach rect gleich viele Menues/Items " + pfad);
                if (gleichViele) {
                    pruefe(d2.menus[mi].items[ii].window.rectClient.x == r.x,
                           std::string(name) + ": rect.x neu gelesen " + pfad);
                }
                pruefe(e.size() == 1, std::string(name) + ": nur eine Zahl ersetzt " + pfad);
                ++verschoben;
            }
        }
    }
    std::printf("%s: %zu Dateien, %d Menues, %d Items, %d gescheitert (wie im Spiel), %d Fehler / %d Warnungen der Engine, "
                "%d @-Texte ohne Eintrag (%zu verschiedene), %d rect-Proben\n",
                name, dateien.size(), menus, items, gescheitert, fehler, warnungen, ohneText, unbekannt.size(), verschoben);
    int n = 0;
    for (const auto& [k, v] : unbekannt) {
        if (n++ < 6) { std::printf("   ohne Text: %s (%d)\n", k.c_str(), v); }
    }
    pruefe(menus > 0, std::string(name) + ": Menues gelesen");
    std::string ds;
    for (const auto& [a, zahl] : diagArten) { ds += " " + a + "=" + std::to_string(zahl); }
    std::printf("%s: Diagnose%s\n", name, ds.c_str());

    // M2: Feeder aus den Spieldaten (UI_BuildPlayerModel_List, Sprachen, Bewegungen)
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        FeederDaten f;
        f.daten = &daten;
        f.dialekt = dl;
        f.spiel = s;
        f.ordner = ordner;
        f.baue();
        const bool sp = dl == Dialekt::SP;
        const auto spezies = f.eintraege(static_cast<float>(sp ? feeder_sp::PLAYER_SPECIES : feeder_mp::PLAYER_SPECIES));
        const auto koepfe = f.eintraege(static_cast<float>(sp ? feeder_sp::PLAYER_SKIN_HEAD : feeder_mp::PLAYER_SKIN_HEAD));
        const auto titel = f.eintraege(static_cast<float>(sp ? feeder_sp::MOVES_TITLES : feeder_mp::MOVES_TITLES));
        std::printf("%s %s: Feeder %zu Spezies (erste %s), %zu Koepfe, %zu Sprachen, %zu Karten, %zu Videos\n", name, sp ? "SP" : "MP",
                    spezies.size(), spezies.empty() ? "-" : spezies[0].spalten[0].c_str(), koepfe.size(), f.sprachen.size(), f.karten.size(),
                    f.filme.size());
        if (s != Spiel::JK2) {   // JK2 kennt kein PlayerChoice.txt
            pruefe(!spezies.empty(), std::string(name) + ": Feeder PLAYER_SPECIES aus models/players/*/PlayerChoice.txt");
            pruefe(!koepfe.empty() && !koepfe[0].bild.empty(), std::string(name) + ": Feeder PLAYER_SKIN_HEAD mit icon_*.jpg");
        }
        pruefe(titel.size() == 6 && titel[0].spalten[0] == "@MENUS_ACROBATICS", std::string(name) + ": Feeder MOVES_TITLES (datapadMoveTitleData)");
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string ja = argc > 1 ? argv[1] : "";
    const std::string jk2 = argc > 2 ? argv[2] : "";
    std::vector<std::string> md;
    for (int i = 3; i < argc; ++i) {
        if (argv[i][0] != '\0') { md.emplace_back(argv[i]); }
    }
    if (ja.empty() && jk2.empty() && md.empty()) {
        std::printf("menuerundlauf: keine Spielordner (-DBEHAVED_KARTE_JA/JK2/MD) - uebersprungen\n");
        return 0;
    }
    if (!ja.empty()) { spiel("JA", Spiel::JA, {ja}); }
    if (!jk2.empty()) { spiel("JK2", Spiel::JK2, {jk2}); }
    if (!md.empty()) { spiel("MD", Spiel::MD, md); }
    std::printf("menuerundlauf: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
