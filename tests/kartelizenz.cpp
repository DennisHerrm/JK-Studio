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
//     (aus welchen NRC-Dateien, Original-Copyright, GPL),
//   - (Stufe 3) q3map2 und seine Bibliotheken unter third_party ohne Herkunft,
//     Lizenztext oder .gitattributes-Schutz sind, oder etclib.c (Apache-2.0)
//     doch dabei ist.
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
    for (const char* ordner : {"src/karte", "include/bhed/karte", "src/menue", "include/bhed/menue"}) {
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
        // Menue-Editor: eigene Dateien (ohne Engine-Vorlage) sagen das im Kopf.
        const bool menue = d.parent_path().filename() == "menue";
        if (menue && enthaelt(kopf.substr(0, 600), "Eigener Code")) { continue; }
        pruefe(enthaelt(kopf, "Portiert aus NetRadiant-Custom") || enthaelt(kopf, "Portiert aus OpenJK")
               || enthaelt(kopf, "Portiert aus q3map2"),
               name + ": Kopf nennt die Herkunft");
        pruefe(enthaelt(kopf, "Copyright (C)"), name + ": Kopf traegt die Original-Copyright-Zeile");
        pruefe(enthaelt(kopf, "GNU General Public License"), name + ": Kopf nennt die GPL");
    }

    // 4. Stufe 3 Punkt 1: q3map2 und seine Bibliotheken unveraendert unter
    //    third_party, mit Herkunft und Lizenztext; etclib (Apache-2.0, nicht
    //    GPL-v2-vertraeglich) nicht dabei, sondern eine eigene Fassung.
    const std::string q3readme = lies(wurzel / "third_party/q3map2/README.md");
    pruefe(enthaelt(q3readme, "20260114") && enthaelt(q3readme, "68ecbed"),
           "third_party/q3map2/README.md nennt Tag 20260114 und Commit 68ecbed");
    for (const char* datei : {"third_party/zlib/LICENSE", "third_party/libpng/LICENSE", "third_party/libjpeg-turbo/LICENSE.md",
                              "third_party/libxml2/Copyright", "third_party/assimp/LICENSE",
                              "third_party/q3map2/tools/quake3/q3map2/q3map2.h"}) {
        pruefe(!lies(wurzel / datei).empty(), std::string(datei) + " vorhanden");
    }
    for (const char* bib : {"zlib 1.3.1", "libpng 1.6.53", "libjpeg-turbo 3.1.3", "libxml2 2.15.1", "assimp 6.0.2"}) {
        pruefe(enthaelt(readme, bib), std::string("README nennt ") + bib);
    }
    std::error_code ec;
    pruefe(!std::filesystem::exists(wurzel / "third_party/q3map2/libs/etclib.c", ec),
           "etclib.c (Apache-2.0) liegt nicht in third_party/q3map2");
    pruefe(enthaelt(lies(wurzel / "src/q3map2/kompat/etclib.c"), "GNU General Public License"), "eigene etclib.c steht unter der GPL");
    const std::string attribute = lies(wurzel / ".gitattributes");
    for (const char* regel : {"third_party/q3map2/** -text", "third_party/zlib/** -text", "third_party/libpng/** -text",
                              "third_party/libjpeg-turbo/** -text", "third_party/libxml2/** -text",
                              "third_party/assimp/** -text"}) {
        pruefe(enthaelt(attribute, regel), std::string(".gitattributes: ") + regel);
    }

    std::printf("kartelizenz: %zu portierte Dateien geprueft, %d Fehlschlaege\n", dateien.size(), g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
