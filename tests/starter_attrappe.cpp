// starter_attrappe.cpp - REL-TEMPO: steht im Testpaket von tools/karte/starter_test.py an der Stelle von
// jkstudio.exe. Der Starter startet sie wie das Programm; sie schreibt nur auf, dass sie lief (Ordner,
// Argumente, ihre Fassung aus version.txt), in gestartet.txt neben sich, und endet - kein Fenster.
#include <windows.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

int wmain(int argc, wchar_t** argv) {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring ordner = self;
    ordner = ordner.substr(0, ordner.find_last_of(L"\\/"));
    wchar_t cwd[MAX_PATH] = {};
    GetCurrentDirectoryW(MAX_PATH, cwd);
    std::ifstream v(ordner + L"\\version.txt", std::ios::binary);
    std::string fassung((std::istreambuf_iterator<char>(v)), std::istreambuf_iterator<char>());
    while (!fassung.empty() && (fassung.back() == '\n' || fassung.back() == '\r')) { fassung.pop_back(); }
    FILE* f = _wfopen((ordner + L"\\gestartet.txt").c_str(), L"ab");
    if (f == nullptr) { return 1; }
    std::fprintf(f, "gestartet %s argumente %d arbeitsordner_gleich %d\n", fassung.c_str(), argc - 1,
                 _wcsicmp(cwd, ordner.c_str()) == 0 ? 1 : 0);
    (void)argv;
    std::fclose(f);
    return 0;
}
