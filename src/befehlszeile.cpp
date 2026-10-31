// befehlszeile.cpp - siehe include/bhed/befehlszeile.h
#include "bhed/befehlszeile.h"

namespace bhed {

namespace {

bool bekannt(const std::string& a) {
    return a == "--mcp" || a == "--pruefe-daten" || a == "--version" || a.rfind("--nach-update=", 0) == 0;
}

}  // namespace

Befehl werteBefehlszeileAus(const std::vector<std::string>& argumente) {
    Befehl b;
    for (const std::string& a : argumente) { b.mcp = b.mcp || a == "--mcp"; }
    // Was mit -- anfaengt und unbekannt ist, wird nie als Datei geoeffnet. Eine Datei mit so einem
    // Namen geht mit einem Pfad (.\\--name); der faengt nicht mit -- an.
    for (const std::string& a : argumente) {
        if (a.rfind("--", 0) == 0 && !bekannt(a)) {
            b.art = Befehl::Art::Unbekannt;
            b.wert = a;
            return b;
        }
    }
    if (argumente.empty() || argumente[0] == "--mcp") { return b; }
    const std::string& a = argumente[0];
    if (a == "--pruefe-daten") {
        b.art = Befehl::Art::PruefeDaten;
    } else if (a == "--version") {
        b.art = Befehl::Art::Version;
    } else if (a.rfind("--nach-update=", 0) == 0) {
        b.art = Befehl::Art::NachUpdate;
        b.wert = a;
    } else {
        b.art = Befehl::Art::Datei;
        b.wert = a;
    }
    return b;
}

}  // namespace bhed
