// alt_install.inc - ERZEUGT aus gui/update_win32.cpp (setze ... installAuftrag) des Updaters von rc604 (git 8cc7bd1e, Datei
// unveraendert seit rc601 513d0afb; so steckt er in jeder installierten Fassung bis
// rc605). Nur fuer tests/uebergangtest.cpp (Stufe 8c: Uebergang zu JK Studio).
// Ersetzt wurde nur, was der Test umlenken muss - alles andere ist Zeile fuer Zeile das
// Original:
//   - exePfad() -> g_simExe (die laufende .exe der simulierten Installation)
//   - schluessel() -> leer (der Test braucht keine Anmeldung)
//   - "https://api.github.com/repos/" -> g_simApi (der lokale Testserver)
//   - bhed::update::kRepo -> kAltesRepo ("DennisHerrm/BehavEd-Remake", wie in rc604)
//   - bhed::update:: -> alt_update:: (die Funktionen von rc604)
//   - bhed::kFassung -> kAlteFassung ("rc604")
//   - %TEMP%\\behaved_update -> g_simTemp
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

std::wstring exePfad() { return g_simExe; }   // ERSETZT: die simulierte laufende .exe

std::string schluessel() { return {}; }   // ERSETZT: im Test ohne Anmeldung

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
    const std::wstring agent = L"BehavEd-Remake/" + breit(kAlteFassung);
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

// --- Die beiden Hintergrundauftraege --------------------------------------
void pruefAuftrag() {
    const std::wstring url = g_simApi + breit(kAltesRepo) + L"/releases/latest";
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
    diag::detail("Update: GitHub (" + std::string(kAltesRepo) + ") antwortet " + std::to_string(a.status) +
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
    alt_update::Release r;
    std::string fehler;
    if (a.status != 200 || !alt_update::parseRelease(a.body, r, &fehler)) {
        setze([&](Zustand& z) {
            z.stand = Stand::Fehler;
            z.meldung = text(Str::UpdErrBadAnswer, fehler, static_cast<long long>(a.status));
        });
        return;
    }
    const bool neuer = alt_update::istNeuer(r.tag, lokaleFassung());
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

void installAuftrag(alt_update::Release r) {
    const alt_update::Asset* asset = alt_update::zipAsset(r);
    if (asset == nullptr) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = tr(Str::UpdNoZip); });
        return;
    }
    const auto fortschritt = [&](std::uint64_t n, std::uint64_t gesamt) {
        const double teil = gesamt > 0 ? static_cast<double>(n) / static_cast<double>(gesamt)
                                       : (asset->size > 0 ? static_cast<double>(n) / static_cast<double>(asset->size) : 0.0);
        setze([&](Zustand& z) { z.fortschritt = std::min(1.0, teil); });
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
    if (!a.fehler.empty() || a.status != 200 || a.body.size() < 22) {
        setze([&](Zustand& z) {
            z.stand = Stand::Fehler;
            z.meldung = a.fehler.empty() ? "HTTP " + std::to_string(a.status) : a.fehler;
        });
        return;
    }
    setze([&](Zustand& z) { z.fortschritt = 1.0; });
    // Das .zip ablegen und mit dem pk3-Leser oeffnen (dasselbe Format).
    std::error_code ec;
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const fs::path ablage = g_simTemp;
    fs::create_directories(ablage, ec);
    const fs::path zipDatei = ablage / breit(asset->name);
    {
        std::ofstream f(zipDatei, std::ios::binary);
        f.write(a.body.data(), static_cast<std::streamsize>(a.body.size()));
    }
    Pk3 archiv;
    std::string fehler;
    if (!readPk3Directory(schmal(zipDatei.wstring()), archiv, &fehler)) {
        setze([&](Zustand& z) { z.stand = Stand::Fehler; z.meldung = text(Str::UpdErrZip, fehler); });
        return;
    }
    std::vector<std::string> namen;
    for (const Pk3Entry& e : archiv.entries) { namen.push_back(e.name); }
    const std::string ober = alt_update::gemeinsamerOrdner(namen);
    const fs::path exe = exePfad();
    const fs::path ordner = exe.parent_path();
    int ersetzt = 0;
    std::string problem;
    // rc601 release: WAS mit einem Eintrag geschieht (an die Stelle der
    // laufenden .exe, daneben, gar nicht), entscheidet einspielenAls in
    // src/update.cpp - dort laesst es sich ohne Netz pruefen
    // (tests/updatetest.cpp). Ohne data\base neben der .exe laeuft das
    // Programm eigenstaendig mit seinen eingebauten Daten.
    const bool nurProgramm = !fs::exists(ordner / L"data" / L"base" / L"behaved.bhc", ec);
    const std::string laufenderName = schmal(exe.filename().wstring());
    for (const Pk3Entry& e : archiv.entries) {
        const std::string rel = alt_update::zielImOrdner(e.name, ober);
        const alt_update::Einspielen art = alt_update::einspielenAls(rel, laufenderName, nurProgramm);
        if (art == alt_update::Einspielen::Ueberspringen) { continue; }
        std::string inhalt;
        if (!readPk3File(archiv, e, inhalt, &fehler)) {
            problem = e.name + ": " + fehler;
            break;
        }
        const bool istExe = art == alt_update::Einspielen::Programm;
        const fs::path ziel = istExe ? exe : ordner / fs::u8path(rel);
        fs::create_directories(ziel.parent_path(), ec);
        fs::path neu = ziel;
        neu += L".neu";
        {
            std::ofstream f(neu, std::ios::binary | std::ios::trunc);
            f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
            if (!f) {
                problem = text(Str::UpdErrWrite, rel);
                break;
            }
        }
        if (istExe) {
            // Die laufende .exe nur umbenennen - ueberschreiben geht nicht.
            fs::path alt = exe;
            alt += L".alt";
            DeleteFileW(alt.c_str());
            if (!MoveFileExW(exe.c_str(), alt.c_str(), MOVEFILE_REPLACE_EXISTING) ||
                !MoveFileExW(neu.c_str(), exe.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                MoveFileExW(alt.c_str(), exe.c_str(), MOVEFILE_REPLACE_EXISTING);   // zurueck
                problem = text(Str::UpdErrReplace, rel, static_cast<long long>(GetLastError()));
                break;
            }
            diag::detail("Update: neues Programm an " + schmal(exe.wstring()) + " (aus " + rel + ")");
        } else if (!MoveFileExW(neu.c_str(), ziel.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            problem = text(Str::UpdErrReplace, rel, static_cast<long long>(GetLastError()));
            break;
        }
        ++ersetzt;
    }
    diag::detail("Update: " + std::to_string(ersetzt) + " Dateien ersetzt" + (problem.empty() ? "" : " - " + problem));
    setze([&](Zustand& z) {
        z.dateien = ersetzt;
        if (!problem.empty()) {
            z.stand = Stand::Fehler;
            z.meldung = problem;
        } else {
            z.stand = Stand::Fertig;
        }
    });
}

