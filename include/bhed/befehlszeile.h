// befehlszeile.h - was die Befehlszeile von jkstudio.exe verlangt (rc608). Ohne Fenster, damit ein ctest es prueft.
#pragma once

#include <string>
#include <vector>

namespace bhed {

struct Befehl {
    enum class Art {
        Normal,        // nichts Besonderes
        Datei,         // wert: die Datei zum Oeffnen
        NachUpdate,    // wert: das ganze Argument, --nach-update=<pid>
        PruefeDaten,   // --pruefe-daten (Release-Pruefung)
        Version,       // --version: die Fassung nennen, das Programm nicht oeffnen
        Unbekannt,     // wert: der Schalter; wird nie als Datei geoeffnet
    };
    Art art = Art::Normal;
    std::string wert;
    bool mcp = false;   // --mcp: eine MCP-App spricht ueber stdin/stdout mit dem Programm
};

// argumente: ohne den Programmnamen, UTF-8.
[[nodiscard]] Befehl werteBefehlszeileAus(const std::vector<std::string>& argumente);

}  // namespace bhed
