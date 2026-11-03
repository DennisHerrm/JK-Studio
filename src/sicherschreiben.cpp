// sicherschreiben.cpp - siehe sicherschreiben.h
#include "bhed/sicherschreiben.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include "bhed/utf8pfad.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace bhed {
namespace {
int g_fehlerStufe = 0;

void tmpWeg(const std::filesystem::path& tmp) {
    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}
}  // namespace

void sicherSchreibenFehlerEinbauen(int stufe) { g_fehlerStufe = stufe; }

bool dateiGesperrt(const std::string& pfad) {
#ifdef _WIN32
    // Ersetzen braucht DELETE am Ziel. Die Engine oeffnet ihre .pk3 mit fopen
    // (MSVC: _SH_DENYNO = Lesen und Schreiben geteilt, Loeschen nicht) - dann
    // scheitert genau dieses Oeffnen mit ERROR_SHARING_VIOLATION.
    const std::wstring w = utf8Pfad(pfad).wstring();
    HANDLE h = CreateFileW(w.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return GetLastError() == ERROR_SHARING_VIOLATION; }
    CloseHandle(h);
    return false;
#else
    (void)pfad;
    return false;
#endif
}

bool dateiErsetzen(const std::string& von, const std::string& nach, SchreibFehler* art) {
    const auto setze = [art](SchreibFehler a) {
        if (art != nullptr) { *art = a; }
        return false;
    };
#ifdef _WIN32
    const std::wstring a = utf8Pfad(von).wstring();
    const std::wstring b = utf8Pfad(nach).wstring();
    if (MoveFileExW(a.c_str(), b.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        const DWORD e = GetLastError();
        return setze(e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION || dateiGesperrt(nach) ? SchreibFehler::Gesperrt
                                                                                                       : SchreibFehler::Umbenennen);
    }
    return true;
#else
    std::error_code ec;
    std::filesystem::rename(utf8Pfad(von), utf8Pfad(nach), ec);
    return !ec || setze(SchreibFehler::Umbenennen);
#endif
}

bool sicherSchreiben(const std::string& pfad, const std::string& inhalt, std::string* fehler, SchreibFehler* art) {
    const int stufe = g_fehlerStufe;
    g_fehlerStufe = 0;
    const auto nein = [fehler, art](SchreibFehler a, const std::string& t) {
        if (fehler != nullptr) { *fehler = t; }
        if (art != nullptr) { *art = a; }
        return false;
    };
    if (art != nullptr) { *art = SchreibFehler::Keiner; }
    const std::filesystem::path ziel = utf8Pfad(pfad);
    std::error_code ec;
    if (ziel.has_parent_path()) { std::filesystem::create_directories(ziel.parent_path(), ec); }
    // Gesperrt (das Spiel haelt die Datei offen): gar nicht erst anfangen.
    if (dateiGesperrt(pfad)) { return nein(SchreibFehler::Gesperrt, "cannot replace " + pfad + " - it is open in another program (is the game running?)"); }
    const std::string tmpPfad = pfad + ".tmp";
    const std::filesystem::path tmp = utf8Pfad(tmpPfad);
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) { return nein(SchreibFehler::Schreiben, "cannot write " + pfad + ".tmp"); }
        const std::size_t teil = stufe == 1 ? inhalt.size() / 2 : inhalt.size();
        f.write(inhalt.data(), static_cast<std::streamsize>(teil));
        if (stufe == 1) { f.setstate(std::ios::badbit); }   // Selbsttest: Fehler mitten im Schreiben
        f.close();   // leert den Puffer - erst hier zeigt sich ein volles Laufwerk
        if (stufe == 2) { f.setstate(std::ios::failbit); }  // Selbsttest: Fehler beim Schliessen
        if (f.fail()) {
            tmpWeg(tmp);
            return nein(SchreibFehler::Schreiben, std::string("cannot write ") + pfad + (stufe == 2 ? " (closing failed)" : ""));
        }
    }
    SchreibFehler a = SchreibFehler::Keiner;
    if (stufe == 3 || !dateiErsetzen(tmpPfad, pfad, &a)) {
        tmpWeg(tmp);
        if (stufe == 3) { a = SchreibFehler::Umbenennen; }
        return nein(a, a == SchreibFehler::Gesperrt ? "cannot replace " + pfad + " - it is open in another program (is the game running?)"
                                                    : "cannot replace " + pfad);
    }
    return true;
}

}  // namespace bhed
