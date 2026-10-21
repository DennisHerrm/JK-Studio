// kartelizenz.cpp - Karten-Editor Stufe 1a, Punkt 1: Lizenz GPL v2.
//
// Seit dem Karten-Editor (21.10.2026, Entscheidung shank) stecken 1:1 portierte
// Teile von NetRadiant-Custom, GtkRadiant, q3map2 und OpenJK im Programm. Die
// stehen unter der GNU GPL v2, und damit das ganze Programm. Diese Probe
// schlaegt an, wenn
//   - LICENSE fehlt oder nicht der GPL-v2-Text ist,
//   - die README noch "No license has been chosen yet." sagt oder den
//     Lizenzabschnitt bzw. die Herkunft (NetRadiant-Custom/GtkRadiant/q3map2,
//     OpenJK) nicht nennt,
//   - eine Datei in src/karte/ oder include/bhed/karte/ ohne Herkunftskopf ist
//     (aus welchen NRC-Dateien, Original-Copyright, GPL).
//
// Aufruf: kartelizenz <Quellordner>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_fehler = 0;

void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { return {}; }
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

bool enthaelt(const std::string& text, const char* stueck) {
    return text.find(stueck) != std::string::npos;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: kartelizenz <Quellordner>\n");
        return 2;
    }
    const std::filesystem::path wurzel = argv[1];

    // 1. LICENSE = GPL v2 (Text aus NetRadiant-Custom, Datei GPL).
    const std::string lizenz = lies(wurzel / "LICENSE");
    pruefe(!lizenz.empty(), "LICENSE vorhanden");
    pruefe(enthaelt(lizenz, "GNU GENERAL PUBLIC LICENSE"), "LICENSE ist die GNU GPL");
    pruefe(enthaelt(lizenz, "Version 2, June 1991"), "LICENSE ist Version 2");
    pruefe(enthaelt(lizenz, "END OF TERMS AND CONDITIONS"), "LICENSE ist vollstaendig");
    pruefe(lizenz.size() == 18011U, "LICENSE hat die Groesse des Originaltexts (18011 Bytes)");

    // 2. README: Lizenzabschnitt, Raven-Daten ausgenommen, Herkunft.
    // Zeilenumbrueche und Einrueckung zaehlen nicht (Markdown umbricht frei).
    std::string readme;
    for (const char c : lies(wurzel / "README.md")) {
        const bool leer = c == ' ' || c == '\r' || c == '\n' || c == '\t';
        if (!leer) { readme += c; }
        else if (!readme.empty() && readme.back() != ' ') { readme += ' '; }
    }
    pruefe(!enthaelt(readme, "No license has been chosen yet."), "README sagt nicht mehr 'No license has been chosen yet.'");
    pruefe(enthaelt(readme, "## License"), "README hat einen Abschnitt '## License'");
    pruefe(enthaelt(readme, "GNU General Public License, version 2"), "README nennt GPL v2");
    pruefe(enthaelt(readme, "Raven Software / Activision") && enthaelt(readme, "not covered"),
           "README nimmt die Raven/Activision-Daten unter data/ aus");
    pruefe(enthaelt(readme, "NetRadiant-Custom") && enthaelt(readme, "GtkRadiant") && enthaelt(readme, "q3map2"),
           "README nennt NetRadiant-Custom / GtkRadiant / q3map2");
    pruefe(enthaelt(readme, "OpenJK"), "README nennt OpenJK");

    // 3. Jede portierte Datei traegt den Herkunftskopf.
    std::vector<std::filesystem::path> dateien;
    for (const char* ordner : {"src/karte", "include/bhed/karte"}) {
        const auto o = wurzel / ordner;
        std::error_code ec;
        if (!std::filesystem::is_directory(o, ec)) { continue; }
        for (const auto& e : std::filesystem::directory_iterator(o, ec)) {
            const auto ext = e.path().extension().string();
            if (ext == ".cpp" || ext == ".h") { dateien.push_back(e.path()); }
        }
    }
    for (const auto& d : dateien) {
        const std::string text = lies(d);
        const std::string kopf = text.substr(0, 4000);
        const std::string name = d.filename().string();
        pruefe(enthaelt(kopf, "Portiert aus NetRadiant-Custom") || enthaelt(kopf, "Portiert aus OpenJK")
               || enthaelt(kopf, "Portiert aus q3map2"),
               name + ": Kopf nennt die Herkunft");
        pruefe(enthaelt(kopf, "Copyright (C)"), name + ": Kopf traegt die Original-Copyright-Zeile");
        pruefe(enthaelt(kopf, "GNU General Public License"), name + ": Kopf nennt die GPL");
    }

    std::printf("kartelizenz: %zu portierte Dateien geprueft, %d Fehlschlaege\n", dateien.size(), g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
