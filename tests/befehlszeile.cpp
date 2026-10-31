// befehlszeile.cpp - die Auswertung der Befehlszeile, ohne Fenster (rc608)
#include "bhed/befehlszeile.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

using bhed::Befehl;

int g_fehler = 0;

void pruefe(const std::vector<std::string>& args, Befehl::Art art, const std::string& wert, bool mcp = false) {
    const Befehl b = bhed::werteBefehlszeileAus(args);
    if (b.art != art || b.wert != wert || b.mcp != mcp) {
        std::string zeile;
        for (const std::string& a : args) { zeile += a + " "; }
        std::printf("FEHLER: [%s] -> Art %d, Wert '%s', mcp %d\n", zeile.c_str(), static_cast<int>(b.art), b.wert.c_str(), b.mcp ? 1 : 0);
        ++g_fehler;
    }
}

}  // namespace

int main() {
    pruefe({}, Befehl::Art::Normal, "");
    // Dateien, wie bisher
    pruefe({"foo.txt"}, Befehl::Art::Datei, "foo.txt");
    pruefe({"C:\\Spiel\\a b.menu"}, Befehl::Art::Datei, "C:\\Spiel\\a b.menu");
    pruefe({"-x.txt"}, Befehl::Art::Datei, "-x.txt");
    // Eine Datei, deren Name wie ein Schalter aussieht, geht mit einem Pfad
    pruefe({".\\--version"}, Befehl::Art::Datei, ".\\--version");
    pruefe({"C:\\x\\--komisch.menu"}, Befehl::Art::Datei, "C:\\x\\--komisch.menu");
    // Die bekannten Schalter
    pruefe({"--pruefe-daten"}, Befehl::Art::PruefeDaten, "");
    pruefe({"--nach-update=1234"}, Befehl::Art::NachUpdate, "--nach-update=1234");
    pruefe({"--mcp"}, Befehl::Art::Normal, "", true);
    pruefe({"foo.menu", "--mcp"}, Befehl::Art::Datei, "foo.menu", true);
    pruefe({"--version"}, Befehl::Art::Version, "");
    // Unbekannt: nie eine Datei, auch nicht hinter einer Datei
    pruefe({"--foo"}, Befehl::Art::Unbekannt, "--foo");
    pruefe({"--foo=bar"}, Befehl::Art::Unbekannt, "--foo=bar");
    pruefe({"--"}, Befehl::Art::Unbekannt, "--");
    pruefe({"--VERSION"}, Befehl::Art::Unbekannt, "--VERSION");
    pruefe({"--nach-update"}, Befehl::Art::Unbekannt, "--nach-update");
    pruefe({"foo.menu", "--bar"}, Befehl::Art::Unbekannt, "--bar");
    pruefe({"--mcp", "--bar"}, Befehl::Art::Unbekannt, "--bar", true);
    if (g_fehler != 0) {
        std::printf("%d Fehler\n", g_fehler);
        return 1;
    }
    std::puts("befehlszeile: alles richtig");
    return 0;
}
