// ki_win32.cpp - Windows-Teil der KI-Anbindung (Stufe H1, siehe bhed/ki.h):
// DPAPI, Zufall (BCryptGenRandom), WinHTTP (TLS mit Zertifikatspruefung), der
// lokale MCP-Server (nur 127.0.0.1) und stdio.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <winhttp.h>

#include "bhed/ki.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace bhed::ki {
namespace {

std::wstring breit(const std::string& s) {
    if (s.empty()) { return {}; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

// Bindet die Blobs zusaetzlich an dieses Programm.
constexpr char kEntropie[] = "jkstudio-h1-ki";

double sekunden() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

bool schuetzen(const std::string& klar, std::string& blob) {
    DATA_BLOB ein{static_cast<DWORD>(klar.size()), reinterpret_cast<BYTE*>(const_cast<char*>(klar.data()))};
    DATA_BLOB ent{static_cast<DWORD>(sizeof(kEntropie) - 1), reinterpret_cast<BYTE*>(const_cast<char*>(kEntropie))};
    DATA_BLOB aus{};
    if (CryptProtectData(&ein, L"JK Studio", &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &aus) == FALSE) { return false; }
    blob.assign(reinterpret_cast<const char*>(aus.pbData), aus.cbData);
    SecureZeroMemory(aus.pbData, aus.cbData);
    LocalFree(aus.pbData);
    return true;
}

bool oeffnen(const std::string& blob, std::string& klar) {
    DATA_BLOB ein{static_cast<DWORD>(blob.size()), reinterpret_cast<BYTE*>(const_cast<char*>(blob.data()))};
    DATA_BLOB ent{static_cast<DWORD>(sizeof(kEntropie) - 1), reinterpret_cast<BYTE*>(const_cast<char*>(kEntropie))};
    DATA_BLOB aus{};
    if (CryptUnprotectData(&ein, nullptr, &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &aus) == FALSE) { return false; }
    klar.assign(reinterpret_cast<const char*>(aus.pbData), aus.cbData);
    SecureZeroMemory(aus.pbData, aus.cbData);
    LocalFree(aus.pbData);
    return true;
}

std::string zufallsToken() {
    unsigned char b[32] = {};
    if (BCryptGenRandom(nullptr, b, sizeof(b), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) { return {}; }
    static const char* const hex = "0123456789abcdef";
    std::string t;
    for (const unsigned char c : b) {
        t += hex[c >> 4U];
        t += hex[c & 15U];
    }
    SecureZeroMemory(b, sizeof(b));
    return t;
}

bool httpPost(const std::string& host, int port, bool sicher, const std::string& pfad, const std::vector<std::pair<std::string, std::string>>& kopf,
              const std::string& koerper, int& status, std::string& antwort, std::string& fehler, int zeitlimitMs) {
    status = 0;
    antwort.clear();
    fehler.clear();
    const bool lokal = host == "127.0.0.1" || host == "localhost";
    HINTERNET s = WinHttpOpen(L"JKStudio-Helper/1", lokal ? WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (s == nullptr && !lokal) {
        s = WinHttpOpen(L"JKStudio-Helper/1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }
    if (s == nullptr) {
        fehler = "WinHttpOpen " + std::to_string(GetLastError());
        return false;
    }
    WinHttpSetTimeouts(s, zeitlimitMs, zeitlimitMs, zeitlimitMs, zeitlimitMs);
    if (sicher) {
        DWORD prot = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
        prot |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
        WinHttpSetOption(s, WINHTTP_OPTION_SECURE_PROTOCOLS, &prot, sizeof(prot));
    }
    bool ok = false;
    HINTERNET c = WinHttpConnect(s, breit(host).c_str(), static_cast<INTERNET_PORT>(port), 0);
    HINTERNET r = c != nullptr ? WinHttpOpenRequest(c, L"POST", breit(pfad).c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    sicher ? WINHTTP_FLAG_SECURE : 0)
                               : nullptr;
    if (r != nullptr) {
        // Keine Umleitungen (der Schluessel soll nirgendwo anders hin).
        DWORD nie = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(r, WINHTTP_OPTION_REDIRECT_POLICY, &nie, sizeof(nie));
        std::wstring k;
        for (const auto& [n, v] : kopf) { k += breit(n) + L": " + breit(v) + L"\r\n"; }
        if (WinHttpSendRequest(r, k.c_str(), static_cast<DWORD>(-1L), const_cast<char*>(koerper.data()), static_cast<DWORD>(koerper.size()),
                               static_cast<DWORD>(koerper.size()), 0) != FALSE &&
            WinHttpReceiveResponse(r, nullptr) != FALSE) {
            DWORD st = 0;
            DWORD groesse = sizeof(st);
            WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &st, &groesse,
                                WINHTTP_NO_HEADER_INDEX);
            status = static_cast<int>(st);
            for (;;) {
                DWORD da = 0;
                if (WinHttpQueryDataAvailable(r, &da) == FALSE || da == 0) { break; }
                std::string teil(da, '\0');
                DWORD gelesen = 0;
                if (WinHttpReadData(r, teil.data(), da, &gelesen) == FALSE) { break; }
                antwort.append(teil.data(), gelesen);
                if (antwort.size() > (8u << 20)) {
                    fehler = "answer too large";
                    break;
                }
            }
            ok = fehler.empty();
        } else {
            fehler = "WinHTTP " + std::to_string(GetLastError());
        }
        WinHttpCloseHandle(r);
    } else {
        fehler = "WinHttpConnect/OpenRequest " + std::to_string(GetLastError());
    }
    if (c != nullptr) { WinHttpCloseHandle(c); }
    WinHttpCloseHandle(s);
    return ok;
}

// --- Lokaler MCP-Server ------------------------------------------------------------

struct HttpServer::Innen {
    SOCKET lauscher = INVALID_SOCKET;
    std::thread faden;
    std::atomic<bool> stopp{false};
    std::atomic<int> abgewiesen{0};
    std::string token;
    int port = 0;
    Bearbeiten bearbeiten;
    Bremse bremse{30};
    bool wsa = false;

    void verbindung(SOCKET k) {
        DWORD zeit = 5000;
        setsockopt(k, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&zeit), sizeof(zeit));
        setsockopt(k, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&zeit), sizeof(zeit));
        std::string roh;
        HttpAnfrage a;
        int st = 1;
        char puffer[8192];
        while (st == 1) {
            const int n = recv(k, puffer, sizeof(puffer), 0);
            if (n <= 0) { break; }
            roh.append(puffer, static_cast<std::size_t>(n));
            st = leseHttp(roh, a, kMaxAnfrage);
        }
        std::string antwort;
        if (st == 1) {
            closesocket(k);
            return;
        }
        if (st != 0) {
            ++abgewiesen;
            antwort = httpAntwort(st, R"({"error":"bad request"})");
        } else if (const Urteil u = pruefeHttp(a, port, token); u.status != 0) {
            ++abgewiesen;
            antwort = httpAntwort(u.status, R"({"error":")" + u.grund + "\"}");
        } else if (!bremse.darf(sekunden())) {
            ++abgewiesen;
            antwort = httpAntwort(429, R"({"error":"too many requests"})");
        } else {
            const std::string r = bearbeiten ? bearbeiten(a.koerper) : std::string();
            antwort = r.empty() ? httpAntwort(202, "") : httpAntwort(200, r);
        }
        std::size_t gesendet = 0;
        while (gesendet < antwort.size()) {
            const int n = send(k, antwort.data() + gesendet, static_cast<int>(antwort.size() - gesendet), 0);
            if (n <= 0) { break; }
            gesendet += static_cast<std::size_t>(n);
        }
        shutdown(k, SD_SEND);
        closesocket(k);
    }

    void lauf() {
        while (!stopp) {
            fd_set m;
            FD_ZERO(&m);
            FD_SET(lauscher, &m);
            timeval t{0, 200000};
            const int n = select(0, &m, nullptr, nullptr, &t);
            if (stopp) { break; }
            if (n <= 0) { continue; }
            sockaddr_in von{};
            int laenge = sizeof(von);
            const SOCKET k = accept(lauscher, reinterpret_cast<sockaddr*>(&von), &laenge);
            if (k == INVALID_SOCKET) { continue; }
            // Nur Verbindungen von diesem PC (gebunden ist ohnehin nur 127.0.0.1).
            if (von.sin_family != AF_INET || von.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                ++abgewiesen;
                closesocket(k);
                continue;
            }
            verbindung(k);
        }
    }
};

HttpServer::~HttpServer() { halte(); }

bool HttpServer::starte(int port, const std::string& token, Bearbeiten b, std::string* fehler) {
    halte();
    if (token.size() < 32) {
        if (fehler != nullptr) { *fehler = "no token"; }
        return false;
    }
    auto* in = new Innen();
    WSADATA w{};
    in->wsa = WSAStartup(MAKEWORD(2, 2), &w) == 0;
    in->token = token;
    in->port = port;
    in->bearbeiten = std::move(b);
    in->lauscher = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BOOL ja = TRUE;
    sockaddr_in adr{};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(static_cast<u_short>(port));
    adr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (in->lauscher == INVALID_SOCKET ||
        setsockopt(in->lauscher, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&ja), sizeof(ja)) != 0 ||
        bind(in->lauscher, reinterpret_cast<const sockaddr*>(&adr), sizeof(adr)) != 0 || listen(in->lauscher, 8) != 0) {
        if (fehler != nullptr) { *fehler = "port " + std::to_string(port) + ": " + std::to_string(WSAGetLastError()); }
        if (in->lauscher != INVALID_SOCKET) { closesocket(in->lauscher); }
        if (in->wsa) { WSACleanup(); }
        delete in;
        return false;
    }
    innen_ = in;
    port_ = port;
    in->faden = std::thread([in] { in->lauf(); });
    return true;
}

void HttpServer::halte() {
    if (innen_ == nullptr) { return; }
    innen_->stopp = true;
    if (innen_->faden.joinable()) { innen_->faden.join(); }
    closesocket(innen_->lauscher);
    if (innen_->wsa) { WSACleanup(); }
    delete innen_;
    innen_ = nullptr;
}

bool HttpServer::laeuft() const { return innen_ != nullptr && !innen_->stopp; }

int HttpServer::abgewiesen() const { return innen_ != nullptr ? innen_->abgewiesen.load() : 0; }

// --- stdio ---------------------------------------------------------------------------

struct StdioServer::Innen {
    std::thread faden;
    std::atomic<bool> laeuft{false};
    Bearbeiten bearbeiten;
    HANDLE ein = INVALID_HANDLE_VALUE;
    HANDLE aus = INVALID_HANDLE_VALUE;

    void schreibe(const std::string& z) {
        const std::string t = z + "\n";
        DWORD n = 0;
        WriteFile(aus, t.data(), static_cast<DWORD>(t.size()), &n, nullptr);
        FlushFileBuffers(aus);
    }

    void lauf() {
        std::string zeile;
        bool zuLang = false;
        char puffer[8192];
        while (laeuft) {
            DWORD n = 0;
            if (ReadFile(ein, puffer, sizeof(puffer), &n, nullptr) == FALSE || n == 0) { break; }   // Client weg
            for (DWORD i = 0; i < n; ++i) {
                const char c = puffer[i];
                if (c == '\n') {
                    if (!zuLang && !zeile.empty()) {
                        if (zeile.back() == '\r') { zeile.pop_back(); }
                        const std::string r = bearbeiten ? bearbeiten(zeile) : std::string();
                        if (!r.empty()) { schreibe(r); }
                    }
                    zeile.clear();
                    zuLang = false;
                } else if (!zuLang) {
                    zeile += c;
                    if (zeile.size() > kMaxAnfrage) {
                        zuLang = true;
                        zeile.clear();
                    }
                }
            }
        }
        laeuft = false;
    }
};

StdioServer::~StdioServer() { halte(); }

bool StdioServer::starte(Bearbeiten b) {
    halte();
    auto* in = new Innen();
    in->ein = GetStdHandle(STD_INPUT_HANDLE);
    in->aus = GetStdHandle(STD_OUTPUT_HANDLE);
    if (in->ein == nullptr || in->ein == INVALID_HANDLE_VALUE || in->aus == nullptr || in->aus == INVALID_HANDLE_VALUE) {
        delete in;
        return false;
    }
    in->bearbeiten = std::move(b);
    in->laeuft = true;
    innen_ = in;
    in->faden = std::thread([in] { in->lauf(); });
    return true;
}

void StdioServer::halte() {
    if (innen_ == nullptr) { return; }
    innen_->laeuft = false;
    if (innen_->faden.joinable()) {
        CancelSynchronousIo(innen_->faden.native_handle());
        innen_->faden.join();
    }
    delete innen_;
    innen_ = nullptr;
}

bool StdioServer::laeuft() const { return innen_ != nullptr && innen_->laeuft; }

}  // namespace bhed::ki
