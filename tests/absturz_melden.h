// absturz_melden.h - Stufe 8d Punkt 4: ein Absturz in einem Fuzzer (auch die Falle von UBSan mit
// -fsanitize-trap, Ausnahme 0xC000001D) nennt die Adresse im Programm und die letzte Station; mit
// einer Map-Datei des Linkers (/MAP) laesst sich daraus die Funktion finden.
#ifndef BHED_TESTS_ABSTURZ_MELDEN_H
#define BHED_TESTS_ABSTURZ_MELDEN_H

#include <windows.h>

#include <cstdint>
#include <cstdio>

namespace absturz {

inline const char*& station() {
    static const char* s = "";
    return s;
}

inline LONG WINAPI melden(EXCEPTION_POINTERS* e) {
    const auto adr = reinterpret_cast<std::uintptr_t>(e->ExceptionRecord->ExceptionAddress);
    const auto basis = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    std::printf("  ABSTURZ bei %s: Ausnahme 0x%08lx bei Programm+0x%llx\n", station(), e->ExceptionRecord->ExceptionCode,
                static_cast<unsigned long long>(adr - basis));
    std::fflush(stdout);
    return EXCEPTION_EXECUTE_HANDLER;
}

inline void einrichten() {
    SetUnhandledExceptionFilter(melden);
    std::setvbuf(stdout, nullptr, _IONBF, 0);   // nichts geht im Puffer verloren
}

}  // namespace absturz

#endif
