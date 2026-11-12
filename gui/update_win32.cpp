// update_win32.cpp - der Auto-Updater: GitHub fragen, laden, installieren
//
// Siehe update.h. Alles Netz laeuft in einem Hintergrundfaden; die
// Oberflaeche liest nur eine Kopie des Zustands.
//
// ZUGRIFF: Das Repository (kRepo) ist oeffentlich - gefragt wird OHNE
// Anmeldung, jeder bekommt die Updates ohne GitHub-Konto. Nur wenn GitHub
// das ablehnt (403: zu viele Anfragen ohne Anmeldung, 60 je Stunde), hilft ein
// Schluessel von aussen, der NIE gespeichert wird: GH_TOKEN bzw.
// GITHUB_TOKEN, sonst "gh auth token" (GitHub CLI, falls angemeldet).
//
// INSTALLATION (seit Stufe 8d in src/update_einspielen.cpp, bhed::update::einspielen):
// erst das .zip gegen SHA256SUMS.txt pruefen (und gegen die Signatur, sobald ein
// Schluessel im Programm steht), dann alles in einen Zwischenordner neben der .exe
// entpacken, erst dann tauschen. Die laufende .exe laesst sich unter Windows nicht
// ueberschreiben, wohl aber umbenennen: sie wird zu "<exe>.alt", die neue kommt an
// ihren Platz; beim naechsten Start wird die alte geloescht.

#include "textumbruch.h"
#include "update.h"

#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "app_internal.h"
#include "bhed/fassung.h"
#include "tour.h"

namespace bhed::gui::updater {
namespace {

namespace fs = std::filesystem;

// Absichtlich nie freigegeben: der Hintergrundfaden kann beim Beenden noch
// laufen, und dann darf sein Zustand nicht schon zerstoert sein.
struct Geteilt {
    std::mutex m;
    Zustand z;
    bool fensterOffen = false;
    bool hinweisWeg = false;   // "Spaeter" - der Hinweis unten verschwindet bis zum naechsten Fund
    // REST9: "Spaeter"/x in der Leiste oben - fuer dieses Release bis zum naechsten Start
    // (ein noch neueres, in dieser Sitzung gefunden, zeigt sie wieder).
    std::string leisteWegTag;
    // Nur Selbsttest: das Netz (testNetzSetzen).
    TestNetz testNetz;
    std::atomic<bool> laeuft{false};
    // rc610 F78 (Fund B8-4): das Programm endet - Laden und Entpacken hoeren auf,
    // ein begonnener Tausch laeuft zu Ende (beimBeenden wartet darauf).
    std::atomic<bool> beenden{false};
    // REST10: [Abbrechen] beim Laden - oder das Programm endet (beimBeenden setzt
    // beides). Laden und Entpacken hoeren auf, ein begonnener Tausch laeuft zu Ende.
    std::atomic<bool> abbruch{false};
    // Nur Selbsttest (REST10): Pause je Stueck der simulierten Antwort, die zuletzt
    // "geoeffnete" Seite.
    std::atomic<int> testPauseMs{0};
    std::string testSeite;
};
Geteilt& g() {
    static Geteilt* x = new Geteilt();
    return *x;
}

void setze(const std::function<void(Zustand&)>& f) {
    std::lock_guard<std::mutex> l(g().m);
    f(g().z);
}

// Eine uebersetzte Meldung mit Zahl/Text darin (alle Meldungen in den vier
// Sprachen, wie der Rest der Oberflaeche).
std::string text(Str id, const std::string& a = {}, long long n = 0) {
    char z[512];
    std::snprintf(z, sizeof(z), tr(id), a.c_str(), n);
    return z;
}

std::wstring breit(const std::string& s) {
    if (s.empty()) { return {}; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string schmal(const std::wstring& w) {
    if (w.empty()) { return {}; }
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring exePfad() {
    std::wstring p(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, p.data(), static_cast<DWORD>(p.size()));
        if (n < p.size()) {
            p.resize(n);
            return p;
        }
        p.resize(p.size() * 2);
    }
}

// --- Der Schluessel fuer ein privates Repository -------------------------
std::string ausgabeVon(const std::wstring& programm, const std::wstring& argumente) {
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE lesen = nullptr;
    HANDLE schreiben = nullptr;
    if (!CreatePipe(&lesen, &schreiben, &sa, 0)) { return {}; }
    SetHandleInformation(lesen, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = schreiben;
    si.hStdError = schreiben;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::wstring zeile = L"\"" + programm + L"\" " + argumente;
    const BOOL ok = CreateProcessW(programm.c_str(), zeile.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                   nullptr, &si, &pi);
    CloseHandle(schreiben);
    std::string out;
    if (ok) {
        char puffer[512];
        DWORD n = 0;
        while (ReadFile(lesen, puffer, sizeof(puffer), &n, nullptr) && n > 0 && out.size() < 4096) {
            out.append(puffer, n);
        }
        WaitForSingleObject(pi.hProcess, 10000);
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (code != 0) { out.clear(); }
    }
    CloseHandle(lesen);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) { out.pop_back(); }
    return out;
}

std::string schluessel() {
    for (const char* name : {"GH_TOKEN", "GITHUB_TOKEN"}) {
        char* wert = nullptr;
        std::size_t laenge = 0;
        if (_dupenv_s(&wert, &laenge, name) == 0 && wert != nullptr) {
            std::string s = wert;
            std::free(wert);
            if (!s.empty()) { return s; }
        }
    }
    // GitHub CLI: im PATH oder am ueblichen Ort.
    wchar_t gefunden[MAX_PATH] = {};
    std::wstring gh;
    if (SearchPathW(nullptr, L"gh.exe", nullptr, MAX_PATH, gefunden, nullptr) > 0) {
        gh = gefunden;
    } else {
        for (const wchar_t* k : {L"C:\\Program Files\\GitHub CLI\\gh.exe", L"C:\\Program Files (x86)\\GitHub CLI\\gh.exe"}) {
            if (GetFileAttributesW(k) != INVALID_FILE_ATTRIBUTES) { gh = k; break; }
        }
    }
    if (gh.empty()) { return {}; }
    const std::string t = ausgabeVon(gh, L"auth token");
    // Ein Schluessel hat keine Leerzeichen; alles andere ist eine Fehlermeldung.
    return (t.find(' ') == std::string::npos && t.size() >= 20) ? t : std::string();
}

// --- HTTP ----------------------------------------------------------------
struct Antwort {
    DWORD status = 0;
    std::string body;
    std::wstring weiter;   // Location bei einer Umleitung
    std::string fehler;
};

Antwort holen(const std::wstring& url, const std::vector<std::wstring>& kopf, bool folgen,
              const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt = {}) {
    Antwort a;
    // Nur Selbsttest (REST9): die simulierte Antwort statt GitHub - danach derselbe
    // Weg wie mit dem Netz (Fortschritt in Stuecken, Abbruch beim Beenden).
    TestNetz netz;
    {
        std::lock_guard<std::mutex> l(g().m);
        netz = g().testNetz;
    }
    if (netz) {
        int status = 0;
        std::string inhalt;
        if (!netz(schmal(url), status, inhalt)) {
            a.fehler = text(Str::UpdErrConnect, schmal(url));
            return a;
        }
        a.status = static_cast<DWORD>(status);
        const std::size_t stueck = std::max<std::size_t>(1, inhalt.size() / 16);
        for (std::size_t i = 0; i < inhalt.size(); i += stueck) {
            if (const int ms = g().testPauseMs.load(); ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
            if (g().abbruch.load()) {
                a.fehler = text(Str::UpdErrAbort);
                return a;
            }
            a.body.append(inhalt, i, stueck);
            if (fortschritt) { fortschritt(a.body.size(), inhalt.size()); }
        }
        return a;
    }
    URL_COMPONENTSW teile{};
    teile.dwStructSize = sizeof(teile);
    wchar_t host[256] = {};
    wchar_t pfad[4096] = {};
    teile.lpszHostName = host;
    teile.dwHostNameLength = 256;
    teile.lpszUrlPath = pfad;
    teile.dwUrlPathLength = 4096;
    wchar_t extra[4096] = {};
    teile.lpszExtraInfo = extra;
    teile.dwExtraInfoLength = 4096;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &teile)) {
        a.fehler = text(Str::UpdErrUrl);
        return a;
    }
    const std::wstring agent = L"JK-Studio/" + breit(bhed::kFassung);
    HINTERNET sitzung = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (sitzung == nullptr) {
        // Vor Windows 8.1 gibt es AUTOMATIC_PROXY nicht.
        sitzung = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
    }
    if (sitzung == nullptr) {
        a.fehler = text(Str::UpdErrHttp);
        return a;
    }
    WinHttpSetTimeouts(sitzung, 10000, 10000, 15000, 30000);
    HINTERNET verbindung = WinHttpConnect(sitzung, host, teile.nPort, 0);
    const std::wstring ziel = std::wstring(pfad) + extra;
    HINTERNET anfrage = (verbindung == nullptr)
                            ? nullptr
                            : WinHttpOpenRequest(verbindung, L"GET", ziel.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 teile.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (anfrage == nullptr) {
        a.fehler = text(Str::UpdErrConnect, schmal(host));
    } else {
        if (!folgen) {
            DWORD aus = WINHTTP_DISABLE_REDIRECTS;
            WinHttpSetOption(anfrage, WINHTTP_OPTION_DISABLE_FEATURE, &aus, sizeof(aus));
        }
        std::wstring kopfzeilen;
        for (const std::wstring& k : kopf) { kopfzeilen += k + L"\r\n"; }
        const BOOL gesendet =
            WinHttpSendRequest(anfrage, kopfzeilen.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : kopfzeilen.c_str(),
                               kopfzeilen.empty() ? 0 : static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(anfrage, nullptr);
        if (!gesendet) {
            a.fehler = text(Str::UpdErrNoAnswer, schmal(host), static_cast<long long>(GetLastError()));
        } else {
            DWORD groesse = sizeof(a.status);
            WinHttpQueryHeaders(anfrage, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &a.status, &groesse, WINHTTP_NO_HEADER_INDEX);
            wchar_t ort[4096] = {};
            DWORD ortGroesse = sizeof(ort);
            if (WinHttpQueryHeaders(anfrage, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, ort, &ortGroesse,
                                    WINHTTP_NO_HEADER_INDEX)) {
                a.weiter = ort;
            }
            DWORD laengeWert = 0;
            DWORD laengeGroesse = sizeof(laengeWert);
            std::uint64_t gesamt = 0;
            if (WinHttpQueryHeaders(anfrage, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &laengeWert, &laengeGroesse, WINHTTP_NO_HEADER_INDEX)) {
                gesamt = laengeWert;
            }
            for (;;) {
                if (g().abbruch.load()) {
                    a.fehler = text(Str::UpdErrAbort);
                    break;
                }
                DWORD verfuegbar = 0;
                if (!WinHttpQueryDataAvailable(anfrage, &verfuegbar)) {
                    a.fehler = text(Str::UpdErrAbort);
                    break;
                }
                if (verfuegbar == 0) { break; }
                const std::size_t alt = a.body.size();
                a.body.resize(alt + verfuegbar);
                DWORD gelesen = 0;
                if (!WinHttpReadData(anfrage, a.body.data() + alt, verfuegbar, &gelesen)) {
                    a.fehler = text(Str::UpdErrAbort);
                    break;
                }
                a.body.resize(alt + gelesen);
                if (fortschritt) { fortschritt(a.body.size(), gesamt); }
            }
        }
        WinHttpCloseHandle(anfrage);
    }
    if (verbindung != nullptr) { WinHttpCloseHandle(verbindung); }
    WinHttpCloseHandle(sitzung);
    return a;
}

std::vector<std::wstring> apiKopf(const std::string& token, const wchar_t* accept) {
    std::vector<std::wstring> k{std::wstring(L"Accept: ") + accept, L"X-GitHub-Api-Version: 2022-11-28"};
    if (!token.empty()) { k.push_back(L"Authorization: Bearer " + breit(token)); }
    return k;
}

// rc610 F78 (Fund B8-9): die Ablage des geladenen .zip, je laufendem Prozess ein
// Unterordner <pid>. Bis rc609 lag das .zip fuer alle Installationen unter
// demselben Namen, und jeder Start (auch der einer zweiten Installation) loeschte
// den ganzen Ordner - mitten aus dem Entpacken einer anderen heraus.
fs::path ablageOrdner() {
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    return fs::path(temp) / L"behaved_update";
}

bool prozessLaeuft(const std::wstring& name) {
    if (name.empty() || name.find_first_not_of(L"0123456789") != std::wstring::npos) { return false; }
    const DWORD pid = static_cast<DWORD>(std::wcstoul(name.c_str(), nullptr, 10));
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (h == nullptr) { return false; }
    const bool laeuft = WaitForSingleObject(h, 0) == WAIT_TIMEOUT;
    CloseHandle(h);
    return laeuft;
}

// --- Die beiden Hintergrundauftraege --------------------------------------
void pruefAuftrag() {
    const std::wstring url = L"https://api.github.com/repos/" + breit(bhed::update::kRepo) + L"/releases/latest";
    // Erst ohne Anmeldung - das Release-Repository ist oeffentlich.
    Antwort a = holen(url, apiKopf({}, L"application/vnd.github+json"), true);
    bool angemeldet = false;
    if (a.fehler.empty() && (a.status == 403 || a.status == 429)) {
        // Zu viele Anfragen ohne Anmeldung: mit Schluessel, falls es einen gibt.
        const std::string token = schluessel();
        if (!token.empty()) {
            a = holen(url, apiKopf(token, L"application/vnd.github+json"), true);
            angemeldet = true;
        }
    }
    diag::detail("Update: GitHub (" + std::string(bhed::update::kRepo) + ") antwortet " + std::to_string(a.status) +
                 (angemeldet ? " (angemeldet)" : " (ohne Anmeldung)") + (a.fehler.empty() ? "" : " - " + a.fehler));
    if (!a.fehler.empty()) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = a.fehler; });
        return;
    }
    if (a.status == 404) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = tr(Str::UpdNoRelease); });
        return;
    }
    if (a.status == 401 || a.status == 403 || a.status == 429) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = tr(Str::UpdNoAccess); });
        return;
    }
    bhed::update::Release r;
    std::string fehler;
    if (a.status != 200 || !bhed::update::parseRelease(a.body, r, &fehler)) {
        setze([&](Zustand& z) {
            z.stand = Stand::Fehler;
            z.meldung = text(Str::UpdErrBadAnswer, fehler, static_cast<long long>(a.status));
        });
        return;
    }
    const bool neuer = bhed::update::istNeuer(r.tag, lokaleFassung());
    diag::detail("Update: neuestes Release " + r.tag + ", installiert " + lokaleFassung() + (neuer ? " -> NEUER" : " -> aktuell"));
    setze([&](Zustand& z) {
        z.release = r;
        z.stand = neuer ? Stand::Verfuegbar : Stand::Aktuell;
    });
    if (neuer) {
        std::lock_guard<std::mutex> l(g().m);
        g().hinweisWeg = false;
    }
}

void installAuftrag(bhed::update::Release r) {
    const bhed::update::Asset* asset = bhed::update::zipAsset(r);
    if (asset == nullptr) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = tr(Str::UpdNoZip); });
        return;
    }
    const auto fortschritt = [&](std::uint64_t n, std::uint64_t gesamt) {
        const std::uint64_t alles = gesamt > 0 ? gesamt : (asset->size > 0 ? static_cast<std::uint64_t>(asset->size) : 0U);
        const double teil = alles > 0 ? static_cast<double>(n) / static_cast<double>(alles) : 0.0;
        setze([&](Zustand& z) {
            z.fortschritt = std::min(1.0, teil);
            z.geladen = n;
            z.gesamt = alles;
        });
    };
    // REST10: [Abbrechen] (nicht das Ende des Programms) - zurueck auf "verfuegbar",
    // nichts eingespielt, die Ablage weg.
    const auto abgebrochen = [] { return g().abbruch.load() && !g().beenden.load(); };
    const auto zurueck = [] {
        diag::detail("Update: Laden abgebrochen - nichts eingespielt");
        setze([](Zustand& z) {
            z.stand = Stand::Verfuegbar;
            z.fortschritt = 0.0;
            z.geladen = 0;
            z.gesamt = 0;
            z.meldung.clear();
            z.beimInstallieren = false;
        });
    };
    // Oeffentlich: der normale Download-Link, ohne Anmeldung.
    Antwort a;
    if (!asset->downloadUrl.empty()) {
        a = holen(breit(asset->downloadUrl), {}, true, fortschritt);
    }
    // Sonst (oder wenn das abgelehnt wird) ueber die API mit Schluessel: sie
    // leitet auf einen signierten Speicherort um, und DORT darf der
    // Schluessel nicht mitgehen - deshalb von Hand folgen.
    if ((asset->downloadUrl.empty() || a.status == 401 || a.status == 403 || a.status == 404) && !asset->apiUrl.empty()) {
        const std::string token = schluessel();
        if (!token.empty()) {
            a = holen(breit(asset->apiUrl), apiKopf(token, L"application/octet-stream"), false);
            if (a.fehler.empty() && (a.status == 301 || a.status == 302 || a.status == 307) && !a.weiter.empty()) {
                a = holen(a.weiter, {}, true, fortschritt);
            }
        }
    }
    if (abgebrochen()) {
        zurueck();
        return;
    }
    if (!a.fehler.empty() || a.status != 200 || a.body.size() < 22) {
        setze([&](Zustand& z) {
            z.stand = Stand::Fehler;
            z.meldung = a.fehler.empty() ? "HTTP " + std::to_string(a.status) : a.fehler;
        });
        return;
    }
    setze([&](Zustand& z) { z.fortschritt = 1.0; });
    // Stufe 8d: SHA256SUMS.txt (Pflicht) und SHA256SUMS.txt.sig (Pflicht, sobald ein
    // Schluessel im Programm steht) des Releases - klein, ohne Fortschritt.
    const auto kleineDatei = [&](const char* name, std::string& inhalt) {
        const bhed::update::Asset* d = bhed::update::assetNamens(r, name);
        if (d == nullptr || d->downloadUrl.empty()) { return false; }
        const Antwort k = holen(breit(d->downloadUrl), {}, true);
        if (!k.fehler.empty() || k.status != 200) { return false; }
        inhalt = k.body;
        return true;
    };
    std::string summen;
    std::string signatur;
    const bool mitSummen = kleineDatei(bhed::update::kSummenDatei, summen);
    const bool mitSignatur = kleineDatei(bhed::update::kSignaturDatei, signatur);
    if (abgebrochen()) {
        zurueck();
        return;
    }
    diag::detail(std::string("Update: ") + bhed::update::kSummenDatei + (mitSummen ? " geladen" : " FEHLT") + ", " +
                 bhed::update::kSignaturDatei + (mitSignatur ? " geladen" : " fehlt"));
    // Das .zip ablegen; einspielen (src/update_einspielen.cpp) prueft es, entpackt
    // in den Zwischenordner und tauscht erst dann.
    std::error_code ec;
    const fs::path ablage = ablageOrdner() / std::to_wstring(GetCurrentProcessId());
    fs::create_directories(ablage, ec);
    const fs::path zipDatei = ablage / breit(asset->name);
    {
        std::ofstream f(zipDatei, std::ios::binary);
        f.write(a.body.data(), static_cast<std::streamsize>(a.body.size()));
    }
    const bhed::update::Einspielergebnis erg =
        bhed::update::einspielen(zipDatei.wstring(), asset->name, mitSummen ? &summen : nullptr,
                                 mitSignatur ? &signatur : nullptr, bhed::update::kSignaturSchluessel, exePfad(), &g().abbruch);
    fs::remove_all(ablage, ec);
    if (!erg.problem.empty() && erg.dateien == 0 && abgebrochen()) {
        zurueck();
        return;
    }
    diag::detail("Update: " + std::to_string(erg.dateien) + " Dateien ersetzt" + (erg.problem.empty() ? "" : " - " + erg.problem));
    setze([&](Zustand& z) {
        z.dateien = erg.dateien;
        if (!erg.problem.empty()) {
            z.stand = Stand::Fehler;
            z.meldung = erg.problem;
        } else {
            z.stand = Stand::Fertig;
        }
    });
}

// Erst den Platz belegen, dann den Zustand setzen, dann starten (REST10): bis REST9
// setzte installieren() "Laedt" vor dem Belegen - endete der vorige Auftrag (etwa
// ein abgebrochenes Laden) erst danach, blieb "Laedt" ohne Auftrag stehen.
bool belegen() {
    bool erwartet = false;
    return g().laeuft.compare_exchange_strong(erwartet, true);
}

void starten(std::function<void()> auftrag) {
    std::thread([auftrag = std::move(auftrag)] {
        auftrag();
        g().laeuft = false;
    }).detach();
}

// Ein neuer Auftrag faengt ohne Abbruch an (ausser das Programm endet).
void abbruchZuruecksetzen() {
    if (!g().beenden.load()) { g().abbruch = false; }
}

}  // namespace

std::string lokaleFassung() {
    char* wert = nullptr;
    std::size_t laenge = 0;
    if (_dupenv_s(&wert, &laenge, "BHED_UPDATE_LOKAL") == 0 && wert != nullptr) {
        std::string s = wert;
        std::free(wert);
        if (!s.empty()) { return s; }
    }
    return bhed::kFassung;
}

Zustand zustand() {
    std::lock_guard<std::mutex> l(g().m);
    return g().z;
}

void pruefen(bool stumm) {
    // REST10 (wie G2C): von Hand steht das Ergebnis in der Leiste oben, nicht mehr im
    // Fenster; auch nach "Spaeter" zeigt sie sich wieder - man hat ja gefragt.
    if (!stumm) {
        std::lock_guard<std::mutex> l(g().m);
        g().leisteWegTag.clear();
    }
    // rc610 F78 (Fund B8-3): nach einer fertigen Installation gilt das installierte
    // Release als aktuell - es bleibt bei "Neu starten" (bis rc609 bot die naechste
    // Suche dasselbe Release wieder an, ein zweites Installieren scheiterte am
    // laufenden Programm, und "Neu starten" war weg). Waehrend des Ladens ebenso:
    // der Zustand gehoert dem laufenden Auftrag.
    const Stand jetzt = zustand().stand;
    if (jetzt == Stand::Fertig || jetzt == Stand::Laedt || !belegen()) {
        // Laeuft schon eine Suche (die beim Start), zeigt die Leiste sie jetzt.
        if (!stumm && jetzt != Stand::Ruhe) { setze([](Zustand& z) { z.vonHand = true; }); }
        return;
    }
    abbruchZuruecksetzen();
    setze([stumm](Zustand& z) {
        z.stand = Stand::Pruefe;
        z.meldung.clear();
        z.beimInstallieren = false;
        z.vonHand = !stumm;
    });
    starten(pruefAuftrag);
}

void installieren() {
    const Zustand z = zustand();
    if (z.stand != Stand::Verfuegbar && z.stand != Stand::Fehler) { return; }
    if (z.release.tag.empty() || !belegen()) { return; }
    abbruchZuruecksetzen();
    setze([](Zustand& x) {
        x.stand = Stand::Laedt;
        x.fortschritt = 0.0;
        x.geladen = 0;
        x.gesamt = 0;
        x.meldung.clear();
        x.beimInstallieren = true;
    });
    starten([r = z.release] { installAuftrag(r); });
}

void abbrechen() {
    if (zustand().stand != Stand::Laedt) { return; }
    diag::detail("Update: Abbrechen gedrueckt");
    g().abbruch = true;
}

void beimStart() {
    // rc610 F78 (Funde B8-4, B8-5): zuerst ein Update, das nicht zu Ende kam -
    // vor dem Loeschen von "<exe>.alt", daran erkennt resteAufraeumen, ob der
    // Tausch fertig wurde. Die Sicherungen verschwinden erst, wenn sie zurueck sind.
    (void)bhed::update::resteAufraeumen(exePfad());
    // Reste eines frueheren Updates: die alte .exe und das geladene .zip.
    std::wstring alt = exePfad() + L".alt";
    DeleteFileW(alt.c_str());
    // Stufe 8c: der Updater bis rc605 hat die laufende behaved.exe in behaved.exe.alt
    // umbenannt; laeuft jetzt jkstudio.exe daneben, raeumt sie den Rest weg.
    const std::wstring altesProgramm = (fs::path(exePfad()).parent_path() / (breit(bhed::update::kAltesProgramm) + L".alt")).wstring();
    if (altesProgramm != alt) { DeleteFileW(altesProgramm.c_str()); }
    // Die Ablagen beendeter Prozesse (und lose .zip der Fassungen bis rc609); die
    // eines laufenden - einer zweiten Installation mitten im Update - bleibt.
    std::error_code ec;
    for (fs::directory_iterator it(ablageOrdner(), ec), ende; !ec && it != ende; it.increment(ec)) {
        if (!(it->is_directory(ec) && prozessLaeuft(it->path().filename().wstring()))) {
            std::error_code ec2;
            fs::remove_all(it->path(), ec2);
        }
    }
    // Im Selbsttest nie von selbst ins Netz.
    char* test = nullptr;
    std::size_t laenge = 0;
    const bool selbsttest = _dupenv_s(&test, &laenge, "BHED_EDITORTEST") == 0 && test != nullptr;
    std::free(test);
    if (!selbsttest && g_app != nullptr && g_app->settings.updateCheck) {
        pruefen(true);
    }
}

// Vorher: erst confirmQuit(), und nur wenn das SOFORT true war, den neuen
// Prozess starten. Mit einem ungesicherten Reiter (oder der Frage "Exit?")
// ist es das nie - die Fragen kamen, und ihr "Nein"/"Ja" beendete das
// Programm, OHNE neu zu starten (Code-Pruefung 03.10., im Selbsttest
// "pruefung" nachgestellt). Jetzt laeuft "Neu starten" durch denselben
// Ablauf wie "Exit" und startet erst, wenn wirklich beendet wird.
namespace {
bool g_neustartVorgemerkt = false;
}

void neuStarten() {
    g_neustartVorgemerkt = true;
    g_app->wantQuit = true;
}

void neustartVergessen() { g_neustartVorgemerkt = false; }

bool neustartVorgemerkt() { return g_neustartVorgemerkt; }

void neustartWennVorgemerkt() {
    if (!g_neustartVorgemerkt) { return; }
    g_neustartVorgemerkt = false;
    // rc601 release: exePfad() ist auch nach dem Update genau die Datei, die
    // installAuftrag geschrieben hat - auch bei einer eigenstaendigen .exe mit
    // anderem Namen. GetModuleFileNameW meldet den Pfad, unter dem der Lader
    // das Programm geladen hat, und behaelt ihn, wenn die laufende Datei in
    // ".alt" umbenannt wird (nachgeprueft am 17.10.2026 mit einem kleinen
    // Programm, das sich selbst umbenennt; beimStart verlaesst sich mit
    // exePfad() + ".alt" auf dasselbe). An diesen Pfad legt installAuftrag
    // das neue Programm.
    const std::wstring exe = exePfad();
    std::wstring zeile = L"\"" + exe + L"\" --nach-update=" + std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(exe.c_str(), zeile.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        diag::detail("Update: Neustart angestossen");
    } else {
        const std::string m = text(Str::UpdErrRestart, {}, static_cast<long long>(GetLastError()));
        diag::detail("Update: " + m);
        platform::showError(m, tr(Str::AppTitle));
    }
}

void beimBeenden() {
    g().beenden = true;
    g().abbruch = true;
    if (!g().laeuft.load()) { return; }
    diag::detail("Update: Beenden wartet auf den laufenden Auftrag (Laden abgebrochen, ein begonnener Tausch laeuft zu Ende)");
    while (g().laeuft.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
    diag::detail("Update: Auftrag beendet - " + zustand().meldung);
}

bool warteAufVorgaenger(const std::string& argument) {
    const std::string vorsatz = "--nach-update=";
    if (argument.compare(0, vorsatz.size(), vorsatz) != 0) { return false; }
    const DWORD pid = static_cast<DWORD>(std::strtoul(argument.c_str() + vorsatz.size(), nullptr, 10));
    if (HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid)) {
        WaitForSingleObject(h, 15000);
        CloseHandle(h);
    }
    return true;
}

void oeffneFenster() {
    std::lock_guard<std::mutex> l(g().m);
    g().fensterOffen = true;
}

bool fensterOffen() {
    std::lock_guard<std::mutex> l(g().m);
    return g().fensterOffen;
}

void zeichneStatus() {
    const Zustand z = zustand();
    bool weg = false;
    {
        std::lock_guard<std::mutex> l(g().m);
        weg = g().hinweisWeg;
    }
    if ((z.stand != Stand::Verfuegbar && z.stand != Stand::Fertig) || (weg && z.stand == Stand::Verfuegbar)) {
        return;
    }
    char text[200];
    if (z.stand == Stand::Verfuegbar) {
        std::snprintf(text, sizeof(text), tr(Str::UpdStatusAvail), z.release.tag.c_str());
    } else {
        std::snprintf(text, sizeof(text), "%s", tr(Str::UpdStatusDone));
    }
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.45F, 0.85F, 0.45F, 1.0F});
    if (ImGui::SmallButton(text)) {
        oeffneFenster();
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
}

namespace {
void seiteOeffnen(const std::string& url);
}  // namespace

void zeichneFenster() {
    bool offen = fensterOffen();
    if (!offen) { return; }
    const Zustand z = zustand();
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 34.0F, 0.0F}, ImGuiCond_Appearing);
    if (ui::fensterBegin((std::string(tr(Str::UpdTitle)) + "###update").c_str(), &offen,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        char z1[256];
        std::snprintf(z1, sizeof(z1), tr(Str::UpdInstalled), lokaleFassung().c_str());
        ImGui::TextDisabled("%s", z1);
        switch (z.stand) {
            case Stand::Ruhe:
            case Stand::Pruefe:
                ImGui::TextUnformatted(tr(Str::UpdChecking));
                break;
            case Stand::Aktuell:
                ImGui::TextUnformatted(tr(Str::UpdCurrent));
                break;
            case Stand::Verfuegbar:
            case Stand::Laedt:
            case Stand::Fertig: {
                char z2[256];
                std::snprintf(z2, sizeof(z2), tr(Str::UpdAvailable), z.release.tag.c_str());
                ImGui::TextUnformatted(z2);
                if (!z.release.body.empty()) {
                    ImGui::TextDisabled("%s", tr(Str::UpdNotes));
                    if (ImGui::BeginChild("##notizen", ImVec2{ImGui::GetFontSize() * 32.0F, ImGui::GetFontSize() * 12.0F},
                                          ImGuiChildFlags_Borders)) {
                        ImGui::PushTextWrapPos(0.0F);
                        cjkTextUnformatted(z.release.body.c_str());
                        ImGui::PopTextWrapPos();
                    }
                    ImGui::EndChild();
                }
                if (z.stand == Stand::Laedt) {
                    ImGui::ProgressBar(static_cast<float>(z.fortschritt), ImVec2{-FLT_MIN, 0.0F});
                } else if (z.stand == Stand::Fertig) {
                    char z3[200];
                    std::snprintf(z3, sizeof(z3), trN(Str::UpdDone, z.dateien), z.dateien);
                    ImGui::TextUnformatted(z3);
                }
                break;
            }
            case Stand::Fehler: {
                char z4[600];
                std::snprintf(z4, sizeof(z4), tr(Str::UpdError), z.meldung.c_str());
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0F);
                cjkTextUnformatted(z4);
                ImGui::PopTextWrapPos();
                break;
            }
        }
        ImGui::Separator();
        if (z.stand == Stand::Verfuegbar) {
            if (ImGui::Button(tr(Str::UpdInstall))) { installieren(); }
            { if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::UpdInstallHint)); } }   // H2
            ImGui::SameLine();
        } else if (z.stand == Stand::Fertig) {
            if (ImGui::Button(tr(Str::UpdRestart))) { neuStarten(); }
            ImGui::SameLine();
        } else if (z.stand == Stand::Fehler || z.stand == Stand::Aktuell) {
            if (ImGui::Button(tr(Str::UpdRetry))) { pruefen(false); }
            { if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::UpdRetryHint)); } }   // H2
            ImGui::SameLine();
        }
        if (!z.release.htmlUrl.empty() && ImGui::Button(tr(Str::UpdOpenPage))) { seiteOeffnen(z.release.htmlUrl); }
        if (!z.release.htmlUrl.empty()) { ImGui::SameLine(); }
        if (ImGui::Button(tr(Str::ChangelogMenu))) {
            changelogZeigen();
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::UpdLater))) {
            offen = false;
            std::lock_guard<std::mutex> l(g().m);
            g().hinweisWeg = (z.stand == Stand::Verfuegbar);
        }
    }
    ImGui::End();
    std::lock_guard<std::mutex> l(g().m);
    g().fensterOffen = offen;
}

// --- REST9: die Leiste oben (wie im Programm G2C) -------------------------
//
// shank 09.11.: "wie bei g2c programm das wenn man es startet oben eine Leiste
// erscheint mit neues update verfuegbar und jetzt updaten ... dass man gleich
// gewarnt wird". Bis rc610 stand ein Fund nur als kleiner gruener Text unten in
// der Statuszeile (zeichneStatus) - leicht zu uebersehen.
//
// REST10 (Abgleich mit G2C, g2c-toolkit/gui/app.cpp drawUpdateBanner): beim Laden
// [Abbrechen] und "x.x / y.y MB" im Balken, nach dem Laden auch [Spaeter], die Suche
// von Hand zeigt "Suche ...", "aktuell" oder den Fehler (mit [Releases-Seite]) in der
// Leiste, ein Symbol vor dem Text (Info, Haken, Fehler).
namespace {

LeistenBild g_leiste;
float g_testBreite = 0.0F;   // testLeistenBreite

// "v1.0.0-rc612" -> "rc612" (die Schreibweise von kFassung); ohne Nummer der Tag selbst.
std::string kurzTag(const std::string& tag) {
    const int n = bhed::update::rcNummer(tag);
    return n >= 0 ? "rc" + std::to_string(n) : tag;
}

// "Diese Version ueberspringen": gilt fuer dieses Release und aeltere; ohne
// erkennbare Nummer in der Einstellung gilt nichts als uebersprungen.
bool uebersprungen(const std::string& tag) {
    if (g_app == nullptr) { return false; }
    const std::string& u = g_app->settings.updateUebersprungen;
    return bhed::update::rcNummer(u) >= 0 && !bhed::update::istNeuer(tag, u);
}

bool leisteZeigen(const Zustand& z) {
    // Die Suche von Hand (REST10): "Suche ...", "aktuell" und ein Fehler beim
    // Nachsehen stehen bis [Schliessen] in der Leiste. Beim Start bleibt das still.
    switch (z.stand) {
        case Stand::Pruefe:
        case Stand::Aktuell: return z.vonHand;
        case Stand::Fehler:
            if (!z.beimInstallieren) { return z.vonHand; }
            break;
        default: break;
    }
    if (z.release.tag.empty()) { return false; }
    {
        std::lock_guard<std::mutex> l(g().m);
        if (g().leisteWegTag == z.release.tag) { return false; }
    }
    switch (z.stand) {
        // Von Hand gesucht zeigt auch eine uebersprungene Version die Leiste (wie G2C).
        case Stand::Verfuegbar: return z.vonHand || !uebersprungen(z.release.tag);
        case Stand::Laedt:
        case Stand::Fertig:
        case Stand::Fehler: return true;   // Fehler beim Installieren
        default: return false;
    }
}

std::string mitText(Str id, const std::string& a, const std::string& b = {}) {
    char z[1024];
    std::snprintf(z, sizeof(z), tr(id), a.c_str(), b.c_str());
    return z;
}

void tipp(const std::string& t) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", t.c_str()); }
}

// Breite eines Knopfs mit seiner sichtbaren Beschriftung (bis "##").
float knopfBreite(const std::string& label) {
    const std::string sichtbar = label.substr(0, label.find("##"));
    return ImGui::CalcTextSize(sichtbar.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0F;
}

// Auf `breite` kuerzen (an Zeichengrenzen, mit "..."), wenn es nicht passt.
std::string gekuerzt(const std::string& t, float breite, bool& kurz) {
    kurz = false;
    if (ImGui::CalcTextSize(t.c_str()).x <= breite) { return t; }
    kurz = true;
    std::string s = t;
    while (!s.empty()) {
        // ein ganzes UTF-8-Zeichen weg: Folgebytes, dann das Kopfbyte
        while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0U) == 0x80U) { s.pop_back(); }
        if (!s.empty()) { s.pop_back(); }
        const std::string probe = s + "...";
        if (ImGui::CalcTextSize(probe.c_str()).x <= breite) { return probe; }
    }
    return "...";
}

// Die Releases-Seite im Browser - im Selbsttest nur gemerkt (kein Browser auf
// shanks Bildschirm).
void seiteOeffnen(const std::string& url) {
    if (std::getenv("BHED_EDITORTEST") != nullptr) {
        std::lock_guard<std::mutex> l(g().m);
        g().testSeite = url;
        diag::detail("Update: (Selbsttest) nicht geoeffnet: " + url);
        return;
    }
    ShellExecuteW(nullptr, L"open", breit(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// "Suche ..." / "aktuell" / Fehler beim Nachsehen: [Schliessen] und x beenden die
// Anzeige; die Suche selbst laeuft (falls noch) still weiter.
void vonHandZu() {
    setze([](Zustand& z) { z.vonHand = false; });
}

}  // namespace

bool leisteSichtbar() { return leisteZeigen(zustand()); }

LeistenBild leisteBild() { return g_leiste; }

float zeichneLeiste() {
    g_leiste = LeistenBild{};
    const Zustand z = zustand();
    if (!leisteZeigen(z)) { return 0.0F; }
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiStyle& st = ImGui::GetStyle();
    const float zeileH = ImGui::GetFrameHeight();
    const std::string neu = kurzTag(z.release.tag);
    const std::string tag = z.release.tag;

    std::string satz;
    struct Knopf {
        std::string label;
        std::string tipp;
        std::function<void()> tun;
        bool haupt = false;
    };
    std::vector<Knopf> knoepfe;
    const auto spaeter = [tag] {
        std::lock_guard<std::mutex> l(g().m);
        g().leisteWegTag = tag;
    };
    const Knopf wasNeu{std::string(tr(Str::UpdBarWhatsNew)) + "###updleiste_neu", tr(Str::UpdBarWhatsNewHint), [] { oeffneFenster(); }};
    const Knopf spaeterKnopf{std::string(tr(Str::UpdLater)) + "###updleiste_spaeter", tr(Str::UpdBarLaterHint), spaeter};
    const Knopf schliessen{std::string(tr(Str::UpdBarClose)) + "###updleiste_schliessen", tr(Str::UpdBarCloseResultHint), vonHandZu};
    LeistenSymbol symbol = LeistenSymbol::Info;
    // x ganz rechts: wie Spaeter bzw. wie Schliessen; nicht neben [Schliessen].
    bool mitZu = true;
    std::function<void()> zuTun = spaeter;
    std::string zuTipp = tr(Str::UpdBarCloseHint);
    const bool sucheVonHand = z.stand == Stand::Pruefe || z.stand == Stand::Aktuell || (z.stand == Stand::Fehler && !z.beimInstallieren);
    switch (z.stand) {
        case Stand::Pruefe:
            satz = tr(Str::UpdBarChecking);
            zuTun = vonHandZu;
            zuTipp = tr(Str::UpdBarCloseCheckHint);
            break;
        case Stand::Aktuell:
            symbol = LeistenSymbol::Haken;
            satz = mitText(Str::UpdBarCurrent, kurzTag(lokaleFassung()));
            knoepfe.push_back(schliessen);
            mitZu = false;
            break;
        case Stand::Verfuegbar:
            satz = mitText(Str::UpdBarAvail, neu, kurzTag(lokaleFassung()));
            knoepfe.push_back({std::string(tr(Str::UpdBarNow)) + "###updleiste_jetzt", mitText(Str::UpdBarNowHint, neu),
                               [] { installieren(); }, true});
            knoepfe.push_back(wasNeu);
            knoepfe.push_back(spaeterKnopf);
            // FIX612 R7: die Version merken UND die Leiste ausblenden (G2C drawUpdateBanner: UpdSkip setzt skippedUpdate und
            // updateBannerHidden_). Nach einer Suche von Hand zeigte die Leiste sonst auch die uebersprungene Version weiter -
            // der Knopf schien nichts zu tun. Die naechste Suche von Hand zeigt sie wieder (pruefen leert leisteWegTag).
            knoepfe.push_back({std::string(tr(Str::UpdBarSkip)) + "###updleiste_skip", mitText(Str::UpdBarSkipHint, neu), [tag, spaeter] {
                                   if (g_app != nullptr) { g_app->settings.updateUebersprungen = tag; }
                                   spaeter();
                               }});
            break;
        case Stand::Laedt:
            satz = mitText(Str::UpdBarLoading, neu);
            knoepfe.push_back({std::string(tr(Str::UpdBarCancel)) + "###updleiste_abbrechen", tr(Str::UpdBarCancelHint), [] { abbrechen(); }});
            break;
        case Stand::Fertig:
            symbol = LeistenSymbol::Haken;
            satz = mitText(Str::UpdBarDone, neu);
            knoepfe.push_back({std::string(tr(Str::UpdRestart)) + "###updleiste_neustart", tr(Str::UpdBarRestartHint),
                               [] { neuStarten(); }, true});
            knoepfe.push_back(wasNeu);
            knoepfe.push_back({spaeterKnopf.label, tr(Str::UpdBarDoneLaterHint), spaeter});
            break;
        default:   // Fehler
            symbol = LeistenSymbol::Fehler;
            if (sucheVonHand) {
                satz = mitText(Str::UpdBarCheckFailed, z.meldung);
                knoepfe.push_back({std::string(tr(Str::UpdBarReleases)) + "###updleiste_seite", tr(Str::UpdBarReleasesHint),
                                   [] { seiteOeffnen(std::string("https://github.com/") + bhed::update::kRepo + "/releases"); }});
                knoepfe.push_back(schliessen);
                mitZu = false;
            } else {
                satz = mitText(Str::UpdBarError, neu, z.meldung);
                knoepfe.push_back({std::string(tr(Str::UpdBarRetry)) + "###updleiste_nochmal", mitText(Str::UpdBarRetryHint, neu),
                                   [] { installieren(); }, true});
                knoepfe.push_back(wasNeu);
                knoepfe.push_back(spaeterKnopf);
            }
            break;
    }

    // Passen Symbol, Text und Knoepfe in eine Zeile? Sonst stehen die Knoepfe darunter
    // (schmales Fenster, lange Uebersetzung); ein zu langer Text (Fehlermeldung)
    // endet mit "..." und steht ganz im Tooltip.
    const float lueckeText = st.ItemSpacing.x * 3.0F;
    const float zuB = mitZu ? zeileH : 0.0F;
    const float symB = ImGui::GetFontSize();
    const float symPlatz = symB + st.ItemSpacing.x;
    const float fortschrittB = z.stand == Stand::Laedt ? ImGui::GetFontSize() * 14.0F : 0.0F;
    float knoepfeB = fortschrittB;
    for (std::size_t i = 0; i < knoepfe.size(); ++i) {
        knoepfeB += knopfBreite(knoepfe[i].label) + ((i > 0 || fortschrittB > 0.0F) ? st.ItemSpacing.x : 0.0F);
    }
    const float innenB = g_testBreite > 0.0F ? g_testBreite : w->Size.x - st.WindowPadding.x * 2.0F;
    const float textB = ImGui::CalcTextSize(satz.c_str()).x;
    const float knoepfeTeil = knoepfeB > 0.0F ? lueckeText + knoepfeB : 0.0F;
    const float zuTeil = mitZu ? lueckeText + zuB : 0.0F;
    const bool eineZeile = symPlatz + textB + knoepfeTeil + zuTeil <= innenB;
    const int zeilen = eineZeile ? 1 : 2;
    const float rand = st.ItemSpacing.y + 1.0F;
    const float h = std::floor(zeileH * static_cast<float>(zeilen) + st.ItemSpacing.y * static_cast<float>(zeilen - 1) + rand * 2.0F + 0.5F);
    const float oben = w->Pos.y + w->TitleBarHeight + w->MenuBarHeight;
    const ImVec2 r0{w->Pos.x, oben};
    const ImVec2 r1{w->Pos.x + w->Size.x, oben + h};

    // Farben: hell auf dunklem Thema, dunkel auf hellem - je Symbol blau (Info:
    // verfuegbar, laedt, Suche), gruen (Haken: installiert, aktuell) oder rot (Fehler).
    const ImVec4 hg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const bool dunkel = 0.299F * hg.x + 0.587F * hg.y + 0.114F * hg.z < 0.5F;
    ImVec4 grund = dunkel ? ImVec4{0.11F, 0.30F, 0.54F, 1.0F} : ImVec4{0.80F, 0.89F, 1.00F, 1.0F};
    ImVec4 kante = dunkel ? ImVec4{0.32F, 0.58F, 0.90F, 1.0F} : ImVec4{0.35F, 0.55F, 0.88F, 1.0F};
    if (symbol == LeistenSymbol::Haken) {
        grund = dunkel ? ImVec4{0.10F, 0.36F, 0.20F, 1.0F} : ImVec4{0.82F, 0.95F, 0.84F, 1.0F};
        kante = dunkel ? ImVec4{0.30F, 0.70F, 0.42F, 1.0F} : ImVec4{0.30F, 0.65F, 0.38F, 1.0F};
    } else if (symbol == LeistenSymbol::Fehler) {
        grund = dunkel ? ImVec4{0.45F, 0.14F, 0.14F, 1.0F} : ImVec4{1.00F, 0.86F, 0.84F, 1.0F};
        kante = ImVec4{0.85F, 0.35F, 0.35F, 1.0F};
    }
    const ImVec4 schrift = dunkel ? ImVec4{1.0F, 1.0F, 1.0F, 1.0F} : ImVec4{0.04F, 0.10F, 0.22F, 1.0F};
    w->DrawList->PushClipRect(r0, r1, false);
    w->DrawList->AddRectFilled(r0, r1, ImGui::GetColorU32(grund));
    w->DrawList->AddLine(ImVec2{r0.x, r1.y - 1.0F}, ImVec2{r1.x, r1.y - 1.0F}, ImGui::GetColorU32(kante), 1.0F);
    w->DrawList->PopClipRect();

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float y = oben + rand;
    // Das Symbol im Strichstil der Bedien-Icons (uiicons.cpp), in der Schriftfarbe der
    // Leiste - lesbar auf jedem ihrer Gruende, im hellen wie im dunklen Thema.
    const ImVec2 symPos{start.x, y + (zeileH - symB) * 0.5F};
    const ImU32 symFarbe = ImGui::GetColorU32(schrift);
    const UiIcon icon = symbol == LeistenSymbol::Haken ? UiIcon::Haken : (symbol == LeistenSymbol::Fehler ? UiIcon::Fehler : UiIcon::Info);
    uiIcon(w->DrawList, icon, symPos, symB, symFarbe);
    ImGui::SetCursorScreenPos(ImVec2{start.x + symPlatz, y});
    bool kurz = false;
    const float textPlatz = innenB - symPlatz - zuTeil - (eineZeile ? knoepfeTeil : 0.0F);
    const std::string gezeigt = gekuerzt(satz, textPlatz, kurz);
    ImGui::PushStyleColor(ImGuiCol_Text, schrift);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(gezeigt.c_str());
    ImGui::PopStyleColor();
    if (kurz) { tipp(satz); }
    if (eineZeile) {
        ImGui::SameLine(0.0F, lueckeText);
    } else {
        ImGui::SetCursorScreenPos(ImVec2{start.x + symPlatz, y + zeileH + st.ItemSpacing.y});
    }
    std::string balken;
    if (z.stand == Stand::Laedt) {
        // Wie G2C: geladen / gesamt in MB (unbekannte Groesse: Prozent).
        char text[64];
        if (z.gesamt > 0) {
            std::snprintf(text, sizeof(text), "%.1f / %.1f MB", static_cast<double>(z.geladen) / (1024.0 * 1024.0),
                          static_cast<double>(z.gesamt) / (1024.0 * 1024.0));
        } else {
            std::snprintf(text, sizeof(text), "%d %%", static_cast<int>(z.fortschritt * 100.0 + 0.5));
        }
        balken = text;
        ImGui::ProgressBar(static_cast<float>(z.fortschritt), ImVec2{fortschrittB, zeileH}, text);
        if (!knoepfe.empty()) { ImGui::SameLine(); }
    }
    for (std::size_t i = 0; i < knoepfe.size(); ++i) {
        if (i > 0) { ImGui::SameLine(); }
        const Knopf& k = knoepfe[i];
        if (k.haupt) {
            // Wie der Compile-Knopf: gruen mit weisser Schrift - der naechste Schritt.
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.18F, 0.55F, 0.30F, 1.0F});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.26F, 0.65F, 0.38F, 1.0F});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.14F, 0.46F, 0.25F, 1.0F});
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 1.0F, 1.0F, 1.0F});
        }
        const bool geklickt = ImGui::Button(k.label.c_str());
        if (k.haupt) { ImGui::PopStyleColor(4); }
        tipp(k.tipp);
        if (geklickt) {
            diag::detail("Update: Leiste - " + k.label.substr(k.label.find("###") + 3) + " (" + tag + ")");
            k.tun();
        }
    }
    if (mitZu) {
        ImGui::SetCursorScreenPos(ImVec2{r1.x - st.WindowPadding.x - zuB, y});
        if (ImGui::Button("x###updleiste_zu", ImVec2{zuB, zeileH})) {
            diag::detail("Update: Leiste - zu (" + tag + ")");
            zuTun();
        }
        tipp(zuTipp);
    }
    // Der Arbeitsbereich darunter beginnt um die Hoehe der Leiste tiefer.
    ImGui::SetCursorScreenPos(start);
    ImGui::Dummy(ImVec2{0.0F, h - st.ItemSpacing.y});
    tourZielMelden("update.leiste", ImRect{r0, r1});   // Tour k-update, Schritt 2
    g_leiste = LeistenBild{r0.x, r0.y, r1.x, r1.y, zeilen, kurz, satz, symbol, symPos.x, symPos.y, symPos.x + symB, symPos.y + symB,
                           symFarbe, ImGui::GetColorU32(grund), balken};
    return h;
}

void testNetzSetzen(TestNetz netz) {
    std::lock_guard<std::mutex> l(g().m);
    g().testNetz = std::move(netz);
}

void testLeistenBreite(float breite) { g_testBreite = breite; }

void testStueckPause(int millisekunden) { g().testPauseMs = millisekunden; }

std::string testLetzteSeite() {
    std::lock_guard<std::mutex> l(g().m);
    return g().testSeite;
}

void testNeuerStart() {
    setze([](Zustand& z) { z = Zustand{}; });
    std::lock_guard<std::mutex> l(g().m);
    g().leisteWegTag.clear();
    g().hinweisWeg = false;
    g().fensterOffen = false;
    g().testSeite.clear();
}

}  // namespace bhed::gui::updater
