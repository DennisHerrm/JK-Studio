// starter_win32.cpp - REL-TEMPO (15.11.): JK-Studio-Starter.exe, der eine Download-Link
//
// shank 14.11.: "lassen wir die aktuelle exe einfach weg in Zukunft und bieten eine zentrale an und
// der Rest wird automatisch geupdated". Der Starter ist klein (kein ImGui, keine Uebersetzungstabelle
// des Programms) und tut bei jedem Start dasselbe:
//
//   1. Ordner   neben dem Starter liegt jkstudio.exe -> dieser Ordner; sonst der Unterordner
//               "JK-Studio" neben dem Starter (wie das entpackte .zip). Keine Administratorrechte:
//               ist der Ordner nicht beschreibbar, sagt der Starter das (Starter in einen eigenen
//               Ordner legen).
//   2. Fassung  version.txt im Ordner (kommt mit jedem Paket); ohne sie oder ohne jkstudio.exe:
//               nichts installiert.
//   3. GitHub   /releases/latest wie der Updater des Programms (gui/update_win32.cpp); neuer als
//               installiert -> bhed::update::aktualisieren: erst das Delta von der installierten
//               Fassung, sonst (oder wenn es nicht passt) das volle Paket; beides gegen SHA256SUMS.txt
//               und die Signatur geprueft, eingespielt ueber den Zwischenordner (alles oder nichts).
//   4. Starten  jkstudio.exe im Ordner, dann endet der Starter.
//
// Ohne Verbindung startet er die installierte Fassung; ist keine da, sagt er es (Noch einmal,
// Schliessen). Abbrechen beim Laden aendert nichts (und startet die installierte Fassung, wenn es
// eine gibt).
//
// Nur fuer Tests (tools/karte/starter_test.py, ohne GitHub, ohne Fenster auf dem Schirm):
//   BHED_UPDATE_API=http://127.0.0.1:<port>/repos/   der lokale Testserver statt api.github.com
//                                                    (nur 127.0.0.1; ab Werk GitHub)
//   BHED_STARTER_TEST=<protokoll>   kein sichtbares Fenster, keine Rueckfragen; Ende mit 0 (gestartet),
//                                   1 (Fehler), 2 (abgebrochen), 3 (Ordner nicht beschreibbar)
//   BHED_STARTER_ABBRUCH_NACH=<n>   wie "Abbrechen", sobald n Bytes geladen sind
//   BHED_STARTER_SPRACHE=en|de|zh|ja|es|cs, BHED_STARTER_SKALA=<f>, BHED_STARTER_FOTO=<bmp>,
//   BHED_STARTER_FOTO_BEI=<zustand>[:<prozent>]   fuer die Handbuchbilder: das Fenster ausserhalb des
//                                   Schirms, mit PrintWindow als BMP abgelegt
//
//   Copyright (C) 2026 JK Studio contributors. GPL v2 (LICENSE).
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "bhed/diag.h"
#include "bhed/fassung.h"
#include "bhed/update.h"

namespace {

namespace fs = std::filesystem;

#include "starter_texte.inc"

// --- Texte -------------------------------------------------------------------
int g_sprache = 0;

const char* tx(T t) {
    const char* s = kTexte[static_cast<int>(t)][g_sprache];
    return s != nullptr ? s : kTexte[static_cast<int>(t)][0];
}

std::string fmt(T t, const std::string& a = {}, long long n = 0) {
    char z[1024];
    std::snprintf(z, sizeof(z), tx(t), a.c_str(), n);
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

std::wstring umgebung(const wchar_t* name) {
    wchar_t wert[1024] = {};
    const DWORD n = GetEnvironmentVariableW(name, wert, 1024);
    return (n > 0 && n < 1024) ? std::wstring(wert, n) : std::wstring();
}

// Die Sprache der Windows-Oberflaeche (wie i18n::fromSystemLocale: Chinesisch nur vereinfacht).
int spracheWaehlen() {
    const std::wstring test = umgebung(L"BHED_STARTER_SPRACHE");
    const wchar_t* const kurz[] = {L"en", L"de", L"zh", L"ja", L"es", L"cs"};
    for (int i = 0; i < kSprachen; ++i) {
        if (test == kurz[i]) { return i; }
    }
    const LANGID l = GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(l)) {
    case LANG_GERMAN: return 1;
    case LANG_CHINESE:
        return (SUBLANGID(l) == SUBLANG_CHINESE_SIMPLIFIED || SUBLANGID(l) == SUBLANG_CHINESE_SINGAPORE) ? 2 : 0;
    case LANG_JAPANESE: return 3;
    case LANG_SPANISH: return 4;
    case LANG_CZECH: return 5;
    default: return 0;
    }
}

// Die Meldungen des Einspielens (src/update_einspielen.cpp) aus der kleinen Tabelle.
const char* meldungstext(bhed::update::Meldung m) {
    using bhed::update::Meldung;
    switch (m) {
    case Meldung::ZipUnlesbar: return tx(T::UpdErrZip);
    case Meldung::OhneSummen: return tx(T::UpdErrNoSums);
    case Meldung::NichtGelistet: return tx(T::UpdErrNotListed);
    case Meldung::Pruefsumme: return tx(T::UpdErrChecksum);
    case Meldung::Signatur: return tx(T::UpdErrSignature);
    case Meldung::Abbruch: return tx(T::UpdErrAbort);
    case Meldung::Schreiben: return tx(T::UpdErrWrite);
    case Meldung::Ersetzen: return tx(T::UpdErrReplace);
    }
    return nullptr;
}

std::string mb(std::uint64_t n) {
    char z[64];
    std::snprintf(z, sizeof(z), "%.1f", static_cast<double>(n) / (1024.0 * 1024.0));
    std::string s = z;
    if (g_sprache == 1 || g_sprache == 4 || g_sprache == 5) { std::replace(s.begin(), s.end(), '.', ','); }
    return s;
}

// --- Protokoll (nur Test) ---------------------------------------------------------
std::mutex g_protokollSperre;
std::wstring g_protokoll;

void protokoll(const std::string& zeile) {
    bhed::diag::detail("Starter: " + zeile);
    std::lock_guard<std::mutex> l(g_protokollSperre);
    if (g_protokoll.empty()) { return; }
    std::ofstream f(fs::path(g_protokoll), std::ios::binary | std::ios::app);
    f << zeile << "\n";
}

// --- Netz (WinHTTP) ------------------------------------------------------------
std::atomic<bool> g_abbruch{false};

struct Antwort {
    DWORD status = 0;
    std::string body;
    std::string fehler;
};

// GET `url`; mit `ziel` in diese Datei (sonst nach body). WinHTTP folgt Umleitungen selbst
// (GitHub: /releases/download -> Speicherort).
Antwort holen(const std::wstring& url, const std::vector<std::wstring>& kopf, const std::wstring* ziel,
              const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt) {
    Antwort a;
    URL_COMPONENTSW teile{};
    teile.dwStructSize = sizeof(teile);
    wchar_t host[256] = {};
    wchar_t pfad[4096] = {};
    wchar_t extra[4096] = {};
    teile.lpszHostName = host;
    teile.dwHostNameLength = 256;
    teile.lpszUrlPath = pfad;
    teile.dwUrlPathLength = 4096;
    teile.lpszExtraInfo = extra;
    teile.dwExtraInfoLength = 4096;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &teile)) {
        a.fehler = fmt(T::UpdErrConnect, schmal(url));
        return a;
    }
    const std::wstring agent = L"JK-Studio-Starter/" + breit(bhed::kFassung);
    HINTERNET sitzung = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (sitzung == nullptr) {
        a.fehler = fmt(T::UpdErrConnect, schmal(host));
        return a;
    }
    WinHttpSetTimeouts(sitzung, 10000, 10000, 15000, 30000);
    HINTERNET verbindung = WinHttpConnect(sitzung, host, teile.nPort, 0);
    const std::wstring weg = std::wstring(pfad) + extra;
    HINTERNET anfrage = verbindung == nullptr
                            ? nullptr
                            : WinHttpOpenRequest(verbindung, L"GET", weg.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 teile.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (anfrage == nullptr) {
        a.fehler = fmt(T::UpdErrConnect, schmal(host));
    } else {
        std::wstring zeilen;
        for (const std::wstring& k : kopf) { zeilen += k + L"\r\n"; }
        const BOOL ok = WinHttpSendRequest(anfrage, zeilen.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : zeilen.c_str(),
                                           zeilen.empty() ? 0 : static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                        WinHttpReceiveResponse(anfrage, nullptr);
        if (!ok) {
            a.fehler = fmt(T::UpdErrNoAnswer, schmal(host), static_cast<long long>(GetLastError()));
        } else {
            DWORD groesse = sizeof(a.status);
            WinHttpQueryHeaders(anfrage, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &a.status, &groesse, WINHTTP_NO_HEADER_INDEX);
            wchar_t laenge[32] = {};
            DWORD lg = sizeof(laenge);
            std::uint64_t gesamt = 0;
            if (WinHttpQueryHeaders(anfrage, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, laenge, &lg,
                                    WINHTTP_NO_HEADER_INDEX)) {
                gesamt = std::wcstoull(laenge, nullptr, 10);
            }
            std::ofstream datei;
            if (ziel != nullptr && a.status == 200) { datei.open(fs::path(*ziel), std::ios::binary | std::ios::trunc); }
            std::uint64_t da = 0;
            std::string stueck;
            for (;;) {
                if (g_abbruch.load()) {
                    a.fehler = fmt(T::UpdErrAbort);
                    break;
                }
                DWORD verfuegbar = 0;
                if (!WinHttpQueryDataAvailable(anfrage, &verfuegbar)) {
                    a.fehler = fmt(T::UpdErrAbort);
                    break;
                }
                if (verfuegbar == 0) { break; }
                stueck.resize(verfuegbar);
                DWORD gelesen = 0;
                if (!WinHttpReadData(anfrage, stueck.data(), verfuegbar, &gelesen)) {
                    a.fehler = fmt(T::UpdErrAbort);
                    break;
                }
                if (datei.is_open()) {
                    datei.write(stueck.data(), gelesen);
                } else {
                    a.body.append(stueck.data(), gelesen);
                }
                da += gelesen;
                if (fortschritt) { fortschritt(da, gesamt); }
            }
            if (datei.is_open()) {
                datei.close();
                if (!datei && a.fehler.empty()) { a.fehler = fmt(T::UpdErrWrite, schmal(*ziel)); }
            }
        }
        WinHttpCloseHandle(anfrage);
    }
    if (verbindung != nullptr) { WinHttpCloseHandle(verbindung); }
    WinHttpCloseHandle(sitzung);
    return a;
}

// Wie apiBasis in gui/update_win32.cpp: ab Werk GitHub, zum Testen nur 127.0.0.1.
std::wstring apiBasis() {
    const std::wstring s = umgebung(L"BHED_UPDATE_API");
    if (s.rfind(L"http://127.0.0.1:", 0) == 0 && s.back() == L'/') { return s; }
    return L"https://api.github.com/repos/";
}

std::vector<std::wstring> apiKopf(const std::string& token) {
    std::vector<std::wstring> k{L"Accept: application/vnd.github+json", L"X-GitHub-Api-Version: 2022-11-28"};
    if (!token.empty()) { k.push_back(L"Authorization: Bearer " + breit(token)); }
    return k;
}

// Die Dateien des Releases ueber den oeffentlichen Download-Link, direkt in die Datei.
class StarterQuelle final : public bhed::update::Quelle {
public:
    bool laden(const bhed::update::Asset& asset, const std::wstring& ziel,
               const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt, std::string& fehler) override {
        protokoll("laden " + asset.name);
        const Antwort a = holen(breit(asset.downloadUrl), {}, &ziel, fortschritt);
        if (!a.fehler.empty() || a.status != 200) {
            fehler = a.fehler.empty() ? "HTTP " + std::to_string(a.status) : a.fehler;
            return false;
        }
        return true;
    }
};

// --- Zustand zwischen Arbeitsfaden und Fenster -------------------------------------
enum class Art { Suche, Laden, Einspielen, Startet, Fehler, OfflineLeer, NichtSchreibbar };
enum class Knopf { Keiner, Abbrechen, OhneUpdate, Nochmal, Starten, Schliessen };

struct Anzeige {
    Art art = Art::Suche;
    std::string text;
    std::string detail;
    double anteil = -1.0;   // < 0: laeuft (ohne Anteil)
    Knopf links = Knopf::Keiner, rechts = Knopf::Abbrechen;
};

std::mutex g_sperre;
Anzeige g_anzeige;
HWND g_fenster = nullptr;
constexpr UINT kNeu = WM_APP + 1;
constexpr UINT kEnde = WM_APP + 2;
int g_ende = 0;
bool g_test = false;
std::wstring g_fotoPfad;   // nur Handbuchbilder

void zeige(const std::function<void(Anzeige&)>& f) {
    {
        std::lock_guard<std::mutex> l(g_sperre);
        f(g_anzeige);
    }
    if (g_fenster != nullptr) { PostMessageW(g_fenster, kNeu, 0, 0); }
}

Anzeige anzeige() {
    std::lock_guard<std::mutex> l(g_sperre);
    return g_anzeige;
}

// --- Der Ablauf (im Arbeitsfaden) ----------------------------------------------------
struct Ort {
    fs::path ordner;   // der Programmordner
    fs::path exe;      // ordner/jkstudio.exe
};

Ort ortWaehlen() {
    std::wstring self(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, self.data(), static_cast<DWORD>(self.size()));
        if (n < self.size()) {
            self.resize(n);
            break;
        }
        self.resize(self.size() * 2);
    }
    const fs::path neben = fs::path(self).parent_path();
    Ort o;
    std::error_code ec;
    o.ordner = fs::exists(neben / bhed::update::kProgramm, ec) ? neben : neben / L"JK-Studio";
    o.exe = o.ordner / bhed::update::kProgramm;
    return o;
}

bool beschreibbar(const fs::path& ordner) {
    std::error_code ec;
    fs::create_directories(ordner, ec);
    const fs::path probe = ordner / (L".jkstudio-starter-" + std::to_wstring(GetCurrentProcessId()));
    HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return false; }
    CloseHandle(h);
    return true;
}

std::string installiert(const Ort& o) {
    std::error_code ec;
    if (!fs::exists(o.exe, ec)) { return {}; }
    std::ifstream f(o.ordner / L"version.txt", std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) { s.pop_back(); }
    return bhed::update::rcVon(s) == s ? s : std::string();
}

bool programmStarten(const Ort& o, const std::string& fassung) {
    if (!g_fotoPfad.empty()) {   // Handbuchbilder: nie etwas starten
        protokoll("Foto: kein Start");
        return true;
    }
    zeige([&](Anzeige& a) {
        a.art = Art::Startet;
        a.text = fmt(T::StStartet, fassung);
        a.anteil = 1.0;
        a.links = Knopf::Keiner;
        a.rechts = Knopf::Keiner;
    });
    std::wstring zeile = L"\"" + o.exe.wstring() + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(o.exe.c_str(), zeile.data(), nullptr, nullptr, FALSE, 0, nullptr, o.ordner.c_str(), &si, &pi)) {
        const DWORD fehler = GetLastError();
        protokoll("Start fehlgeschlagen " + std::to_string(fehler));
        zeige([&](Anzeige& a) {
            a.art = Art::Fehler;
            a.text = fmt(T::StStartFehler, {}, static_cast<long long>(fehler));
            a.links = Knopf::Keiner;
            a.rechts = Knopf::Schliessen;
        });
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    protokoll("gestartet " + schmal(o.exe.wstring()) + " (" + fassung + ")");
    return true;
}

void ende(int code) {
    g_ende = code;
    if (g_fenster != nullptr) { PostMessageW(g_fenster, kEnde, 0, 0); }
}

void ablauf() {
    g_abbruch = false;
    const Ort o = ortWaehlen();
    protokoll("Ordner " + schmal(o.ordner.wstring()));
    zeige([&](Anzeige& a) {
        a = Anzeige{};
        a.text = tx(T::StSuche);
        a.detail = fmt(T::StOrdner, schmal(o.ordner.wstring()));
    });
    if (!beschreibbar(o.ordner)) {
        protokoll("nicht beschreibbar");
        zeige([&](Anzeige& a) {
            a.art = Art::NichtSchreibbar;
            a.text = fmt(T::StNichtSchreibbar, schmal(o.ordner.wstring()));
            a.anteil = 0.0;
            a.links = Knopf::Keiner;
            a.rechts = Knopf::Schliessen;
        });
        if (g_test) { ende(3); }
        return;
    }
    bhed::diag::openDetail(schmal((o.ordner / L"starter.log").wstring()));
    (void)bhed::update::resteAufraeumen(o.exe.wstring());
    const std::string lokal = installiert(o);
    protokoll("installiert " + (lokal.empty() ? std::string("-") : lokal));

    // GitHub fragen (bei 403/429 mit GH_TOKEN/GITHUB_TOKEN, falls gesetzt).
    const std::wstring url = apiBasis() + breit(bhed::update::kRepo) + L"/releases/latest";
    Antwort a = holen(url, apiKopf({}), nullptr, {});
    if (a.fehler.empty() && (a.status == 403 || a.status == 429)) {
        for (const wchar_t* name : {L"GH_TOKEN", L"GITHUB_TOKEN"}) {
            const std::wstring t = umgebung(name);
            if (!t.empty()) {
                a = holen(url, apiKopf(schmal(t)), nullptr, {});
                break;
            }
        }
    }
    bhed::update::Release r;
    std::string grund = a.fehler;
    if (grund.empty() && a.status == 404) { grund = tx(T::UpdNoRelease); }
    if (grund.empty() && (a.status == 401 || a.status == 403 || a.status == 429)) { grund = tx(T::UpdNoAccess); }
    std::string jsonFehler;
    if (grund.empty() && (a.status != 200 || !bhed::update::parseRelease(a.body, r, &jsonFehler))) {
        grund = fmt(T::UpdErrBadAnswer, jsonFehler, static_cast<long long>(a.status));
    }
    if (!grund.empty()) {
        protokoll("offline: " + grund);
        if (!lokal.empty() || fs::exists(o.exe)) {
            zeige([&](Anzeige& x) { x.text = fmt(T::StOffline, grund); });
            ende(programmStarten(o, lokal) ? 0 : 1);
            return;
        }
        zeige([&](Anzeige& x) {
            x.art = Art::OfflineLeer;
            x.text = fmt(T::StOfflineLeer, grund);
            x.anteil = 0.0;
            x.links = Knopf::Nochmal;
            x.rechts = Knopf::Schliessen;
        });
        if (g_test) { ende(1); }
        return;
    }
    const std::string neu = bhed::update::rcVon(r.tag);
    protokoll("neuestes " + r.tag);
    std::error_code ec;
    if (!lokal.empty() && fs::exists(o.exe, ec) && !bhed::update::istNeuer(r.tag, lokal)) {
        ende(programmStarten(o, lokal) ? 0 : 1);
        return;
    }

    // Laden und einspielen.
    const bool delta = !lokal.empty() && bhed::update::assetNamens(r, bhed::update::deltaName(neu, lokal).c_str()) != nullptr;
    zeige([&](Anzeige& x) {
        x.art = Art::Laden;
        x.text = fmt(delta ? T::StLaedtDelta : T::StLaedtVoll, neu);
        x.anteil = 0.0;
        x.links = Knopf::Keiner;
        x.rechts = lokal.empty() ? Knopf::Abbrechen : Knopf::OhneUpdate;
    });
    const std::wstring abbruchNach = umgebung(L"BHED_STARTER_ABBRUCH_NACH");
    const std::uint64_t grenze = abbruchNach.empty() ? 0 : std::wcstoull(abbruchNach.c_str(), nullptr, 10);
    DWORD letzte = 0;
    const auto fortschritt = [&](std::uint64_t n, std::uint64_t gesamt) {
        if (grenze > 0 && n >= grenze && !g_abbruch.load()) {
            protokoll("Abbrechen (Test) bei " + std::to_string(n));
            g_abbruch = true;
        }
        const DWORD jetzt = GetTickCount();
        if (jetzt - letzte < 50 && n < gesamt) { return; }
        letzte = jetzt;
        zeige([&](Anzeige& x) {
            // Ganz geladen: pruefen und einspielen; kommt danach noch etwas (das volle Paket, weil das
            // Delta nicht passte), wieder "laden".
            if (gesamt > 0 && n >= gesamt) {
                x.art = Art::Einspielen;
                x.text = tx(T::StEinspielen);
                x.anteil = -1.0;
                x.detail.clear();
                return;
            }
            if (x.art == Art::Einspielen) {
                x.art = Art::Laden;
                x.text = fmt(T::StLaedtVoll, neu);
            }
            x.anteil = gesamt > 0 ? static_cast<double>(n) / static_cast<double>(gesamt) : -1.0;
            char z[200];
            std::snprintf(z, sizeof(z), tx(T::StMB), mb(n).c_str(), mb(gesamt).c_str());
            x.detail = gesamt > 0 ? z : mb(n) + " MB";
        });
    };
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    // Dieselbe Ablage wie das Programm (gui/update_win32.cpp): je Prozess ein Unterordner.
    const fs::path ablage = fs::path(temp) / L"behaved_update" / std::to_wstring(GetCurrentProcessId());
    StarterQuelle quelle;
    bhed::update::meldungstexteSetzen(&meldungstext);
    const bhed::update::Ablaufergebnis e = bhed::update::aktualisieren(
        r, lokal, o.exe.wstring(), quelle, ablage.wstring(), bhed::update::kSignaturSchluessel, &g_abbruch, fortschritt,
        bhed::update::Umfang::Alles);
    fs::remove_all(ablage, ec);
    protokoll(std::string("Ergebnis: ") + (e.mitDelta ? "Delta" : "voll") + ", " + std::to_string(e.einspielen.dateien) +
              " Dateien, " + std::to_string(e.einspielen.geloescht) + " weg" +
              (e.deltaGrund.empty() ? "" : ", Delta ging nicht: " + e.deltaGrund) +
              (e.ladeFehler.empty() ? "" : ", Laden: " + e.ladeFehler) +
              (e.einspielen.problem.empty() ? "" : ", Problem: " + e.einspielen.problem));
    const bool fertig = e.ladeFehler.empty() && e.einspielen.problem.empty();
    if (!fertig && g_abbruch.load() && e.einspielen.dateien == 0) {
        protokoll("abgebrochen");
        if (!lokal.empty()) {
            ende(programmStarten(o, lokal) ? 2 : 1);
            return;
        }
        zeige([&](Anzeige& x) {
            x.art = Art::Fehler;
            x.text = tx(T::StAbgebrochen);
            x.anteil = 0.0;
            x.links = Knopf::Nochmal;
            x.rechts = Knopf::Schliessen;
        });
        if (g_test) { ende(2); }
        return;
    }
    if (!fertig) {
        const std::string warum = !e.ladeFehler.empty() ? e.ladeFehler : e.einspielen.problem;
        zeige([&](Anzeige& x) {
            x.art = Art::Fehler;
            x.text = fmt(T::StFehler, warum);
            x.anteil = 0.0;
            x.links = Knopf::Nochmal;
            x.rechts = (!lokal.empty() && fs::exists(o.exe)) ? Knopf::Starten : Knopf::Schliessen;
        });
        if (g_test) { ende(1); }
        return;
    }
    ende(programmStarten(o, neu) ? 0 : 1);
}

// --- Das Fenster -------------------------------------------------------------------
float g_skala = 1.0F;
HFONT g_titelSchrift = nullptr;
HFONT g_schrift = nullptr;
HFONT g_kleinSchrift = nullptr;
HWND g_knopf[2] = {nullptr, nullptr};
Knopf g_knopfArt[2] = {Knopf::Keiner, Knopf::Keiner};
HICON g_symbol = nullptr;
int g_marquee = 0;
std::thread g_faden;
std::string g_fotoBei;
bool g_fotoGemacht = false;

int px(float v) { return static_cast<int>(v * g_skala + 0.5F); }

float skalaFuer(HWND h) {
    const std::wstring s = umgebung(L"BHED_STARTER_SKALA");
    if (!s.empty()) { return std::max(0.5F, std::wcstof(s.c_str(), nullptr)); }
    const UINT dpi = h != nullptr ? GetDpiForWindow(h) : GetDpiForSystem();
    return static_cast<float>(dpi) / 96.0F;
}

void schriftenSetzen() {
    for (HFONT* f : {&g_titelSchrift, &g_schrift, &g_kleinSchrift}) {
        if (*f != nullptr) { DeleteObject(*f); }
    }
    NONCLIENTMETRICSW m{};
    m.cbSize = sizeof(m);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(m), &m, 0);
    LOGFONTW lf = m.lfMessageFont;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfHeight = -px(15.0F);
    lf.lfWeight = FW_NORMAL;
    g_schrift = CreateFontIndirectW(&lf);
    lf.lfHeight = -px(12.5F);
    g_kleinSchrift = CreateFontIndirectW(&lf);
    lf.lfHeight = -px(24.0F);
    lf.lfWeight = FW_SEMIBOLD;
    g_titelSchrift = CreateFontIndirectW(&lf);
    for (HWND k : g_knopf) {
        if (k != nullptr) { SendMessageW(k, WM_SETFONT, reinterpret_cast<WPARAM>(g_schrift), TRUE); }
    }
}

const char* knopfText(Knopf k) {
    switch (k) {
    case Knopf::Abbrechen: return tx(T::StKnopfAbbrechen);
    case Knopf::OhneUpdate: return tx(T::StKnopfOhneUpdate);
    case Knopf::Nochmal: return tx(T::StKnopfNochmal);
    case Knopf::Starten: return tx(T::StKnopfStarten);
    case Knopf::Schliessen: return tx(T::StKnopfSchliessen);
    case Knopf::Keiner: break;
    }
    return "";
}

// Feste Breite (480 Punkte), die Hoehe nach dem Text.
constexpr float kBreite = 480.0F;
constexpr float kRand = 20.0F;

int textHoehe(HDC dc, HFONT f, const std::wstring& t, int breite) {
    HGDIOBJ alt = SelectObject(dc, f);
    RECT r{0, 0, breite, 0};
    DrawTextW(dc, t.c_str(), static_cast<int>(t.size()), &r, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, alt);
    return r.bottom - r.top;
}

struct Lage {
    RECT titel, text, balken, detail, knoepfe;
    int hoehe;
};

Lage lageFuer(HDC dc, const Anzeige& a) {
    Lage l{};
    const int rand = px(kRand);
    const int innen = px(kBreite) - 2 * rand;
    const int symbol = px(48.0F);
    int y = rand;
    const int tx0 = rand + symbol + px(14.0F);
    l.titel = {tx0, y, rand + innen, y + px(34.0F)};
    y += px(36.0F);
    const int th = std::max(px(20.0F), textHoehe(dc, g_schrift, breit(a.text), rand + innen - tx0));
    l.text = {tx0, y, rand + innen, y + th};
    y = std::max(y + th, rand + symbol) + px(16.0F);
    l.balken = {rand, y, rand + innen, y + px(8.0F)};
    y += px(8.0F) + px(8.0F);
    const int dh = std::max(px(16.0F), textHoehe(dc, g_kleinSchrift, breit(a.detail), innen));
    l.detail = {rand, y, rand + innen, y + dh};
    y += dh + px(16.0F);
    l.knoepfe = {rand, y, rand + innen, y + px(30.0F)};
    y += px(30.0F) + rand;
    l.hoehe = y;
    return l;
}

void knoepfeSetzen(const Anzeige& a, const Lage& l) {
    const Knopf arten[2] = {a.links, a.rechts};
    HDC dc = GetDC(g_fenster);
    int x = l.knoepfe.right;
    for (int i = 1; i >= 0; --i) {
        g_knopfArt[i] = arten[i];
        if (arten[i] == Knopf::Keiner) {
            ShowWindow(g_knopf[i], SW_HIDE);
            continue;
        }
        const std::wstring t = breit(knopfText(arten[i]));
        HGDIOBJ alt = SelectObject(dc, g_schrift);
        SIZE s{};
        GetTextExtentPoint32W(dc, t.c_str(), static_cast<int>(t.size()), &s);
        SelectObject(dc, alt);
        const int b = std::max(px(96.0F), static_cast<int>(s.cx) + px(28.0F));
        SetWindowTextW(g_knopf[i], t.c_str());
        SetWindowPos(g_knopf[i], nullptr, x - b, l.knoepfe.top, b, l.knoepfe.bottom - l.knoepfe.top, SWP_NOZORDER);
        ShowWindow(g_knopf[i], SW_SHOWNA);
        x -= b + px(8.0F);
    }
    ReleaseDC(g_fenster, dc);
}

void neuAnordnen() {
    const Anzeige a = anzeige();
    HDC dc = GetDC(g_fenster);
    const Lage l = lageFuer(dc, a);
    ReleaseDC(g_fenster, dc);
    RECT r{0, 0, px(kBreite), l.hoehe};
    AdjustWindowRectExForDpi(&r, GetWindowLongW(g_fenster, GWL_STYLE), FALSE, 0, static_cast<UINT>(96.0F * g_skala));
    SetWindowPos(g_fenster, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    knoepfeSetzen(a, l);
    InvalidateRect(g_fenster, nullptr, TRUE);
}

void malen(HWND h) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    RECT c;
    GetClientRect(h, &c);
    const Anzeige a = anzeige();
    const Lage l = lageFuer(dc, a);
    FillRect(dc, &c, GetSysColorBrush(COLOR_WINDOW));
    // Unten eine leichte Flaeche fuer die Knoepfe (wie die Dialoge von Windows).
    RECT unten{0, l.knoepfe.top - px(12.0F), c.right, c.bottom};
    HBRUSH grau = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    FillRect(dc, &unten, grau);
    DeleteObject(grau);
    DrawIconEx(dc, px(kRand), px(kRand), g_symbol, px(48.0F), px(48.0F), 0, nullptr, DI_NORMAL);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    HGDIOBJ alt = SelectObject(dc, g_titelSchrift);
    RECT r = l.titel;
    DrawTextW(dc, L"JK Studio", -1, &r, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    SelectObject(dc, g_schrift);
    const bool fehler = a.art == Art::Fehler || a.art == Art::OfflineLeer || a.art == Art::NichtSchreibbar;
    if (fehler) { SetTextColor(dc, RGB(176, 32, 32)); }
    const std::wstring t = breit(a.text);
    r = l.text;
    DrawTextW(dc, t.c_str(), static_cast<int>(t.size()), &r, DT_WORDBREAK | DT_NOPREFIX);
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    SelectObject(dc, g_kleinSchrift);
    const std::wstring d = breit(a.detail);
    r = l.detail;
    DrawTextW(dc, d.c_str(), static_cast<int>(d.size()), &r, DT_WORDBREAK | DT_NOPREFIX | DT_PATH_ELLIPSIS);
    SelectObject(dc, alt);
    // Der Balken: eigene Zeichnung (kein comctl32), mit Anteil oder als laufendes Stueck.
    if (!fehler) {
        HBRUSH spur = CreateSolidBrush(RGB(222, 226, 232));
        FillRect(dc, &l.balken, spur);
        DeleteObject(spur);
        HBRUSH farbe = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
        RECT f = l.balken;
        const int w = l.balken.right - l.balken.left;
        if (a.anteil >= 0.0) {
            f.right = f.left + static_cast<int>(w * std::min(1.0, a.anteil));
        } else {
            const int stueck = w / 4;
            f.left = l.balken.left + (g_marquee * px(6.0F)) % (w + stueck) - stueck;
            f.right = f.left + stueck;
            f.left = std::max(f.left, l.balken.left);
            f.right = std::min(f.right, l.balken.right);
        }
        if (f.right > f.left) { FillRect(dc, &f, farbe); }
        DeleteObject(farbe);
    }
    EndPaint(h, &ps);
}

// Nur fuer die Handbuchbilder: der Inhalt des Fensters per PrintWindow als BMP (ohne Rahmen: den zeichnet Windows in der
// Groesse des Bildschirms, nicht in der Skala des Fotos).
void fotoMachen() {
    RECT r;
    GetClientRect(g_fenster, &r);
    const int w = r.right - r.left;
    const int hh = r.bottom - r.top;
    HDC schirm = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(schirm);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -hh;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ alt = SelectObject(dc, bmp);
    PrintWindow(g_fenster, dc, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    GdiFlush();
    BITMAPFILEHEADER fh{};
    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof(fh) + sizeof(bi.bmiHeader);
    fh.bfSize = fh.bfOffBits + static_cast<DWORD>(w) * static_cast<DWORD>(hh) * 4U;
    std::ofstream f(fs::path(g_fotoPfad), std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    f.write(reinterpret_cast<const char*>(&bi.bmiHeader), sizeof(bi.bmiHeader));
    f.write(static_cast<const char*>(bits), static_cast<std::streamsize>(w) * hh * 4);
    SelectObject(dc, alt);
    DeleteObject(bmp);
    DeleteDC(dc);
    ReleaseDC(nullptr, schirm);
    protokoll("Foto " + schmal(g_fotoPfad) + " " + std::to_string(w) + "x" + std::to_string(hh));
}

bool fotoZeitpunkt(const Anzeige& a) {
    if (g_fotoPfad.empty() || g_fotoGemacht) { return false; }
    const std::size_t p = g_fotoBei.find(':');
    const std::string art = g_fotoBei.substr(0, p);
    const double prozent = p == std::string::npos ? 0.0 : std::atof(g_fotoBei.c_str() + p + 1) / 100.0;
    switch (a.art) {
    case Art::Suche: return art == "suche";
    case Art::Laden: return art == "laden" && a.anteil >= prozent;
    case Art::Fehler: return art == "fehler";
    case Art::OfflineLeer: return art == "offline";
    case Art::NichtSchreibbar: return art == "ordner";
    default: return false;
    }
}

void starteFaden() {
    if (g_faden.joinable()) { g_faden.join(); }
    g_faden = std::thread(ablauf);
}

LRESULT CALLBACK fensterProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        g_fenster = h;
        for (int i = 0; i < 2; ++i) {
            g_knopf[i] = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_TABSTOP | (i == 1 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                                         0, 0, 10, 10, h, reinterpret_cast<HMENU>(static_cast<INT_PTR>(100 + i)), nullptr, nullptr);
        }
        g_skala = skalaFuer(h);
        schriftenSetzen();
        SetTimer(h, 1, 30, nullptr);
        return 0;
    }
    case WM_DPICHANGED: {
        if (umgebung(L"BHED_STARTER_SKALA").empty()) {
            g_skala = static_cast<float>(HIWORD(w)) / 96.0F;
            schriftenSetzen();
            const RECT* r = reinterpret_cast<const RECT*>(l);
            SetWindowPos(h, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            neuAnordnen();
        }
        return 0;
    }
    case WM_TIMER:
        if (anzeige().anteil < 0.0) {
            ++g_marquee;
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    case kNeu: {
        neuAnordnen();
        UpdateWindow(h);
        const Anzeige a = anzeige();
        if (fotoZeitpunkt(a)) {
            g_fotoGemacht = true;
            fotoMachen();
            ende(0);
            g_abbruch = true;
        }
        return 0;
    }
    case kEnde:
        if (g_faden.joinable()) { g_faden.join(); }
        DestroyWindow(h);
        return 0;
    case WM_COMMAND:
        if (HIWORD(w) == BN_CLICKED && (LOWORD(w) == 100 || LOWORD(w) == 101)) {
            switch (g_knopfArt[LOWORD(w) - 100]) {
            case Knopf::Abbrechen:
            case Knopf::OhneUpdate: g_abbruch = true; break;
            case Knopf::Nochmal: starteFaden(); break;
            case Knopf::Starten: {
                const Ort o = ortWaehlen();
                if (g_faden.joinable()) { g_faden.join(); }
                if (programmStarten(o, installiert(o))) { ende(0); }
                break;
            }
            case Knopf::Schliessen: ende(1); break;
            case Knopf::Keiner: break;
            }
        }
        return 0;
    case WM_CLOSE:
        // Waehrend des Suchens, Ladens und Einspielens wie "Abbrechen" (der Tausch selbst laeuft zu Ende); sonst zu.
        if (anzeige().art == Art::Laden || anzeige().art == Art::Suche || anzeige().art == Art::Einspielen) {
            g_abbruch = true;
            return 0;
        }
        ende(1);
        return 0;
    case WM_PAINT: malen(h); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    default: break;
    }
    return DefWindowProcW(h, m, w, l);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int zeigen) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    g_sprache = spracheWaehlen();
    g_protokoll = umgebung(L"BHED_STARTER_TEST");
    g_test = !g_protokoll.empty();
    g_fotoPfad = umgebung(L"BHED_STARTER_FOTO");
    g_fotoBei = schmal(umgebung(L"BHED_STARTER_FOTO_BEI"));
    g_symbol = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = fensterProc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));   // IDC_ARROW
    wc.lpszClassName = L"JKStudioStarter";
    RegisterClassExW(&wc);
    const DWORD stil = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    // Test und Fotos: ausserhalb des Schirms, nie im Vordergrund (PrintWindow braucht kein Bild auf dem Schirm).
    const bool versteckt = g_test || !g_fotoPfad.empty();
    const int x = versteckt ? GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) + 200 : CW_USEDEFAULT;
    HWND h = CreateWindowExW(versteckt ? WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE : 0, wc.lpszClassName, breit(tx(T::StTitel)).c_str(),
                             stil, x, versteckt ? 0 : CW_USEDEFAULT, 480, 200, nullptr, nullptr, inst, nullptr);
    if (h == nullptr) { return 1; }
    neuAnordnen();
    if (!versteckt) {
        // In die Mitte des Schirms, auf dem der Mauszeiger ist.
        POINT maus{};
        GetCursorPos(&maus);
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(MonitorFromPoint(maus, MONITOR_DEFAULTTOPRIMARY), &mi);
        RECT r;
        GetWindowRect(h, &r);
        SetWindowPos(h, nullptr, (mi.rcWork.left + mi.rcWork.right - (r.right - r.left)) / 2,
                     (mi.rcWork.top + mi.rcWork.bottom - (r.bottom - r.top)) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ShowWindow(h, zeigen);
    } else if (!g_fotoPfad.empty()) {
        ShowWindow(h, SW_SHOWNOACTIVATE);
    }
    starteFaden();
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    g_abbruch = true;
    if (g_faden.joinable()) { g_faden.join(); }
    return g_ende;
}
