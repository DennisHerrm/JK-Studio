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
#include "bhed/menue/menue.h"
#include "bhed/menue/spieldaten.h"

#include <cstdio>
#include <map>
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
    for (const std::string& pfad : dateien) {
        std::string text;
        if (!daten.lies(pfad, text)) {
            pruefe(false, std::string(name) + ": nicht lesbar " + pfad);
            continue;
        }
        const LeseOptionen o = optionenFuer(pfad, s, daten.lader());
        const Datei d = lies(pfad, text, o);
        pruefe(schreib(d) == text, std::string(name) + ": byte-gleich " + pfad);
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
            if (gezeigt < 12) {
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
