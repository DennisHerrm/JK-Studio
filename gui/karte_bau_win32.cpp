// karte_bau_win32.cpp - Karten-Editor, Stufe 3: q3map2 starten und seinen
// Rueckkanal annehmen (Windows). Siehe karte_bau_plattform.h.
//
// Vorbild: NetRadiant-Custom 20260114 libs/commandlib.cpp (Q_Exec),
// libs/l_net/l_net_wins.c (WINS_OpenReliableSocket, WINS_Accept, WINS_Read).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karte_bau_plattform.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <vector>

namespace bhed::gui::bauplattform {

namespace {

std::wstring breit(const std::string& s) {
    if (s.empty()) { return {}; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string letzterFehler() {
    const DWORD c = GetLastError();
    wchar_t* text = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, c, 0,
                   reinterpret_cast<wchar_t*>(&text), 0, nullptr);
    std::string aus = "Fehler " + std::to_string(c);
    if (text != nullptr) {
        const int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        std::string u(static_cast<std::size_t>(n > 0 ? n - 1 : 0), '\0');
        if (n > 1) { WideCharToMultiByte(CP_UTF8, 0, text, -1, u.data(), n, nullptr, nullptr); }
        while (!u.empty() && (u.back() == '\n' || u.back() == '\r')) { u.pop_back(); }
        aus += ": " + u;
        LocalFree(text);
    }
    return aus;
}

bool winsock() {
    static bool bereit = [] {
        WSADATA d;
        return WSAStartup(MAKEWORD(2, 2), &d) == 0;
    }();
    return bereit;
}

}  // namespace

// --- Prozess ---------------------------------------------------------------------

struct Prozess::Teile {
    HANDLE prozess = nullptr;
    HANDLE job = nullptr;
    HANDLE lesen = nullptr;
    DWORD rueckgabe = 0;
    bool fertig = false;
};

Prozess::Prozess() : m(std::make_unique<Teile>()) {}
Prozess::~Prozess() { loslassen(); }

bool Prozess::gestartet() const { return m->prozess != nullptr; }

bool Prozess::starten(const std::string& befehlszeile, const std::string& ordner, bool ausgabe, std::string& fehler) {
    loslassen();
    m = std::make_unique<Teile>();
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE schreiben = nullptr;
    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
    if (ausgabe) {
        if (!CreatePipe(&m->lesen, &schreiben, &sa, 1 << 16)) {
            fehler = letzterFehler();
            if (nul != INVALID_HANDLE_VALUE) { CloseHandle(nul); }
            return false;
        }
        SetHandleInformation(m->lesen, HANDLE_FLAG_INHERIT, 0);   // nur das Schreibende erbt das Kind
    }
    // Nur diese Handles erbt das Kind (PROC_THREAD_ATTRIBUTE_HANDLE_LIST): sonst
    // erbt q3map2 alle erblichen Handles des Editors, auch den lauschenden Socket -
    // dann haelt es Port 39000, bis es ganz zu Ende ist, und der naechste Schritt
    // bekommt ihn nicht ("Failed to get a listening socket", Selbsttest s3p4a/b).
    std::vector<HANDLE> erben;
    if (nul != INVALID_HANDLE_VALUE) { erben.push_back(nul); }
    if (schreiben != nullptr) { erben.push_back(schreiben); }
    SIZE_T groesse = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &groesse);
    std::vector<unsigned char> attributSpeicher(groesse);
    auto* attribute = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributSpeicher.data());
    const bool mitListe = !erben.empty() && InitializeProcThreadAttributeList(attribute, 1, 0, &groesse) &&
                          UpdateProcThreadAttribute(attribute, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, erben.data(), erben.size() * sizeof(HANDLE),
                                                    nullptr, nullptr);
    STARTUPINFOEXW six{};
    six.StartupInfo.cb = sizeof(six);
    six.lpAttributeList = mitListe ? attribute : nullptr;
    STARTUPINFOW& si = six.StartupInfo;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nul;
    si.hStdOutput = ausgabe ? schreiben : nul;
    si.hStdError = ausgabe ? schreiben : nul;
    PROCESS_INFORMATION pi{};
    std::wstring zeile = breit(befehlszeile);
    while (!zeile.empty() && zeile.front() == L' ') { zeile.erase(zeile.begin()); }   // Q_Exec: fuehrende Leerzeichen weg
    std::vector<wchar_t> puffer(zeile.begin(), zeile.end());
    puffer.push_back(L'\0');
    const std::wstring o = breit(ordner);
    // CREATE_SUSPENDED: erst in den Job, dann laufen lassen
    const BOOL ok = CreateProcessW(nullptr, puffer.data(), nullptr, nullptr, !erben.empty(),
                                   (mitListe ? EXTENDED_STARTUPINFO_PRESENT : 0) | CREATE_NO_WINDOW | CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS, nullptr,
                                   o.empty() ? nullptr : o.c_str(), &si, &pi);
    const std::string grund = ok ? std::string() : letzterFehler();
    if (mitListe) { DeleteProcThreadAttributeList(attribute); }
    if (schreiben != nullptr) { CloseHandle(schreiben); }
    if (nul != INVALID_HANDLE_VALUE) { CloseHandle(nul); }
    if (!ok) {
        fehler = grund;
        if (m->lesen != nullptr) {
            CloseHandle(m->lesen);
            m->lesen = nullptr;
        }
        return false;
    }
    // Ein Job, der beim Schliessen seine Prozesse beendet: q3map2 endet mit dem
    // Editor, auch wenn dieser abstuerzt (kein verwaister, unsichtbarer Prozess).
    m->job = CreateJobObjectW(nullptr, nullptr);
    if (m->job != nullptr) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(m->job, JobObjectExtendedLimitInformation, &info, sizeof(info));
        if (!AssignProcessToJobObject(m->job, pi.hProcess)) {
            CloseHandle(m->job);
            m->job = nullptr;
        }
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    m->prozess = pi.hProcess;
    return true;
}

std::string Prozess::lesen() {
    std::string aus;
    if (m->lesen == nullptr) { return aus; }
    for (;;) {
        DWORD verfuegbar = 0;
        if (!PeekNamedPipe(m->lesen, nullptr, 0, nullptr, &verfuegbar, nullptr) || verfuegbar == 0) { break; }
        char puffer[4096];
        DWORD gelesen = 0;
        if (!ReadFile(m->lesen, puffer, std::min<DWORD>(verfuegbar, sizeof(puffer)), &gelesen, nullptr) || gelesen == 0) { break; }
        aus.append(puffer, gelesen);
    }
    return aus;
}

bool Prozess::laeuft() {
    if (m->prozess == nullptr || m->fertig) { return false; }
    if (WaitForSingleObject(m->prozess, 0) == WAIT_OBJECT_0) {
        GetExitCodeProcess(m->prozess, &m->rueckgabe);
        m->fertig = true;
        return false;
    }
    return true;
}

bool Prozess::warten(int millisekunden) {
    if (m->prozess == nullptr) { return false; }
    if (!m->fertig && WaitForSingleObject(m->prozess, static_cast<DWORD>(millisekunden)) == WAIT_OBJECT_0) {
        GetExitCodeProcess(m->prozess, &m->rueckgabe);
        m->fertig = true;
    }
    return m->fertig;
}

int Prozess::rueckgabe() const { return static_cast<int>(m->rueckgabe); }

void Prozess::beenden() {
    if (m->prozess != nullptr && !m->fertig) {
        TerminateProcess(m->prozess, 1);
        WaitForSingleObject(m->prozess, 2000);
        GetExitCodeProcess(m->prozess, &m->rueckgabe);
        m->fertig = true;
    }
}

void Prozess::loslassen() {
    if (!m) { return; }
    if (m->lesen != nullptr) { CloseHandle(m->lesen); }
    if (m->prozess != nullptr) { CloseHandle(m->prozess); }
    if (m->job != nullptr) { CloseHandle(m->job); }   // KILL_ON_JOB_CLOSE: ein noch laufender Prozess endet hier
    m->lesen = nullptr;
    m->prozess = nullptr;
    m->job = nullptr;
}

// --- Horcher ---------------------------------------------------------------------

struct Horcher::Teile {
    SOCKET lauschen = INVALID_SOCKET;
    SOCKET verbindung = INVALID_SOCKET;
};

Horcher::Horcher() : m(std::make_unique<Teile>()) {}
Horcher::~Horcher() { schliessen(); }

bool Horcher::verbunden() const { return m->verbindung != INVALID_SOCKET; }

bool Horcher::lauschen(int port, std::string& fehler) {
    schliessen();
    if (!winsock()) {
        fehler = "WSAStartup";
        return false;
    }
    SOCKET s = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT);   // nicht erblich
    if (s == INVALID_SOCKET) {
        fehler = "socket " + std::to_string(WSAGetLastError());
        return false;
    }
    BOOL exklusiv = TRUE;
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exklusiv), sizeof(exklusiv));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(static_cast<u_short>(port));
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // nur 127.0.0.1 (NRC: alle Adressen)
    if (bind(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == SOCKET_ERROR || listen(s, 1) == SOCKET_ERROR) {
        fehler = "bind/listen " + std::to_string(WSAGetLastError());
        closesocket(s);
        return false;
    }
    u_long ohneWarten = 1;
    ioctlsocket(s, FIONBIO, &ohneWarten);
    m->lauschen = s;
    return true;
}

bool Horcher::annehmen() {
    if (m->lauschen == INVALID_SOCKET) { return false; }
    SOCKET c = accept(m->lauschen, nullptr, nullptr);
    if (c == INVALID_SOCKET) { return false; }
    u_long ohneWarten = 1;
    ioctlsocket(c, FIONBIO, &ohneWarten);
    m->verbindung = c;
    return true;
}

int Horcher::empfangen(std::string& daten) {
    if (m->verbindung == INVALID_SOCKET) { return -1; }
    bool etwas = false;
    for (;;) {
        char puffer[8192];
        const int n = recv(m->verbindung, puffer, sizeof(puffer), 0);
        if (n > 0) {
            daten.append(puffer, static_cast<std::size_t>(n));
            etwas = true;
            continue;
        }
        if (n == 0) { return etwas ? 1 : 2; }   // zu (erst die Daten abgeben)
        const int f = WSAGetLastError();
        if (f == WSAEWOULDBLOCK) { return etwas ? 1 : 0; }
        if (f == WSAECONNRESET || f == WSAECONNABORTED) { return etwas ? 1 : 2; }
        return etwas ? 1 : -1;
    }
}

void Horcher::schliessen() {
    if (!m) { return; }
    if (m->verbindung != INVALID_SOCKET) { closesocket(m->verbindung); }
    if (m->lauschen != INVALID_SOCKET) { closesocket(m->lauschen); }
    m->verbindung = INVALID_SOCKET;
    m->lauschen = INVALID_SOCKET;
}

bool losgeloestStarten(const std::string& befehlszeile, const std::string& ordner) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring zeile = breit(befehlszeile);
    std::vector<wchar_t> puffer(zeile.begin(), zeile.end());
    puffer.push_back(L'\0');
    const std::wstring o = breit(ordner);
    if (!CreateProcessW(nullptr, puffer.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, nullptr,
                        o.empty() ? nullptr : o.c_str(), &si, &pi)) {
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

}  // namespace bhed::gui::bauplattform
