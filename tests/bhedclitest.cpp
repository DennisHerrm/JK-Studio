// bhedclitest.cpp - rc610 F78 (Funde B8-6, B8-7, B8-8): bhed (Befehlszeile)
//
//   B8-6  Dateinamen ausserhalb der ANSI-Codepage (<japanisch>.icarus): "bhed check
//         <ordner>" nennt die Fehler jeder Datei und endet mit 1 statt mit
//         abort; "bhed compile <<japanisch>.icarus>" findet die Datei.
//   B8-7  "bhed compile -o <ziel>": ein Ziel, das die Bytes nie bekommt (eine
//         Pipe, deren Gegenseite nicht liest und schliesst), meldet einen
//         Fehler statt "N Bloecke, M Byte" mit Rueckgabe 0.
//   B8-8  "bhed info" ueber den PATH (ohne Ordner aufgerufen, argv[0] = "bhed")
//         findet das Befehlsmodell neben der .exe.
//
// Aufruf: bhedclitest <bhed.exe> <Arbeitsordner>
#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const char* was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}

void schreibe(const fs::path& p, const std::string& t) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}

std::string utf8(const std::wstring& w) {
    if (w.empty()) { return {}; }
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

struct Lauf {
    DWORD rueckgabe = 0xFFFFFFFF;
    std::string ausgabe;   // stdout und stderr
};

// `programm` leer: die Befehlszeile allein entscheidet (Suche ueber den PATH).
// `pfad` nicht leer: PATH des Kindes.
Lauf starte(const std::wstring& programm, std::wstring zeile, const fs::path& ordner, const std::wstring& pfad = {}) {
    Lauf l;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE lesen = nullptr;
    HANDLE schreiben = nullptr;
    if (!CreatePipe(&lesen, &schreiben, &sa, 0)) { return l; }
    SetHandleInformation(lesen, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = schreiben;
    si.hStdError = schreiben;
    si.hStdInput = nullptr;
    std::wstring umgebung;
    if (!pfad.empty()) {
        // Die eigene Umgebung, nur PATH ersetzt.
        wchar_t* alle = GetEnvironmentStringsW();
        for (const wchar_t* z = alle; *z != L'\0'; z += wcslen(z) + 1) {
            const std::wstring e = z;
            if (_wcsnicmp(e.c_str(), L"PATH=", 5) == 0) { continue; }
            umgebung += e;
            umgebung.push_back(L'\0');
        }
        FreeEnvironmentStringsW(alle);
        umgebung += L"PATH=" + pfad;
        umgebung.push_back(L'\0');
        umgebung.push_back(L'\0');
    }
    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessW(programm.empty() ? nullptr : programm.c_str(), zeile.data(), nullptr, nullptr, TRUE,
                                   CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, umgebung.empty() ? nullptr : umgebung.data(),
                                   ordner.wstring().c_str(), &si, &pi);
    CloseHandle(schreiben);
    if (ok) {
        char puffer[4096];
        DWORD n = 0;
        while (ReadFile(lesen, puffer, sizeof(puffer), &n, nullptr) && n > 0) { l.ausgabe.append(puffer, n); }
        WaitForSingleObject(pi.hProcess, 60000);
        GetExitCodeProcess(pi.hProcess, &l.rueckgabe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    CloseHandle(lesen);
    return l;
}

void zeige(const char* was, const Lauf& l) {
    std::printf("      %s: Rueckgabe %ld, Ausgabe \"%s\"\n", was, static_cast<long>(l.rueckgabe),
                l.ausgabe.substr(0, 300).c_str());
}

const char* const kGut = "affect ( \"kyle\", FLUSH )\n{\n\tset ( \"SET_ANIM_BOTH\", \"BOTH_STAND1\" );\n}\n";
const char* const kSchlecht = "affect ( \"kyle\", FLUSH )\n{\n\tgibtesnicht ( 1 );\n}\n";

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fputs("Aufruf: bhedclitest <bhed.exe> <Arbeitsordner>\n", stderr);
        return 2;
    }
    const fs::path bhed = fs::absolute(argv[1]);
    const fs::path wurzel = fs::absolute(fs::path(argv[2]) / "bhedclitest");
    std::error_code ec;
    fs::remove_all(wurzel, ec);
    const fs::path skripte = wurzel / L"skripte";
    fs::create_directories(skripte, ec);
    const std::wstring jp = L"\u30C6\u30B9\u30C8";   // "tesuto" in Katakana
    schreibe(skripte / (jp + L".icarus"), kSchlecht);
    schreibe(skripte / L"a_schlecht.icarus", kSchlecht);
    const std::wstring exe = bhed.wstring();

    // --- B8-6: check ueber einen Ordner mit <japanisch>.icarus -----------------------
    {
        const Lauf l = starte(exe, L"\"" + exe + L"\" check \"" + skripte.wstring() + L"\"", wurzel);
        zeige("check <ordner>", l);
        const std::string name = utf8(jp) + ".icarus: Fehler";
        pruefe(l.rueckgabe == 1, "B8-6 check <ordner mit <japanisch>.icarus>: Rueckgabe 1 (kein Abbruch)");
        pruefe(l.ausgabe.find(name) != std::string::npos && l.ausgabe.find("a_schlecht.icarus: Fehler") != std::string::npos &&
                   l.ausgabe.find("2 Datei(en): 2 Fehler") != std::string::npos,
               "B8-6 check <ordner>: beide Dateien mit Namen und Fehler, Summe 2 Dateien, 2 Fehler");
    }
    // --- B8-6: compile <<japanisch>.icarus> ----------------------------------------
    {
        const fs::path gut = skripte / (jp + L"_gut.icarus");
        schreibe(gut, kGut);
        const Lauf l = starte(exe, L"\"" + exe + L"\" compile \"" + gut.wstring() + L"\"", wurzel);
        zeige("compile <<japanisch>_gut.icarus>", l);
        fs::path ibi = gut;
        ibi.replace_extension(L".ibi");
        pruefe(l.rueckgabe == 0 && fs::exists(ibi, ec) && fs::file_size(ibi, ec) > 0,
               "B8-6 compile <<japanisch>_gut.icarus>: Rueckgabe 0, .ibi daneben");
    }
    // --- B8-7: compile -o <Ziel, das die Bytes nie bekommt> ---------------------
    {
        const fs::path gut = wurzel / L"gut.icarus";
        schreibe(gut, kGut);
        const std::wstring pipeName = L"\\\\.\\pipe\\bhedclitest_" + std::to_wstring(GetCurrentProcessId());
        // Ohne Puffer: ein Schreiben wartet, bis gelesen wird. Die Gegenseite liest
        // nie und schliesst nach einer halben Sekunde.
        HANDLE p = CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr);
        std::thread gegenseite([p] {
            ConnectNamedPipe(p, nullptr);
            Sleep(500);
            CloseHandle(p);
        });
        const Lauf l = starte(exe, L"\"" + exe + L"\" compile \"" + gut.wstring() + L"\" -o " + pipeName, wurzel);
        gegenseite.join();
        zeige("compile -o <pipe>", l);
        pruefe(l.rueckgabe != 0 && l.ausgabe.find("Bloecke") == std::string::npos,
               "B8-7 compile -o <Ziel ohne Bytes>: Rueckgabe ungleich 0, keine Erfolgsmeldung");
    }
    // --- B8-8: info ueber den PATH, in einem leeren Ordner ------------------------
    {
        const fs::path leer = wurzel / L"leer";
        fs::create_directories(leer, ec);
        const Lauf l = starte({}, L"bhed info", leer, bhed.parent_path().wstring());
        zeige("bhed info (PATH)", l);
        pruefe(l.rueckgabe == 0 && l.ausgabe.find("Befehlsmodell aus") != std::string::npos,
               "B8-8 bhed info ueber den PATH: Rueckgabe 0, Befehlsmodell neben der .exe gefunden");
    }
    fs::remove_all(wurzel, ec);
    std::printf("bhed_cli: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
