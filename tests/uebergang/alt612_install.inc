// alt612_install.inc - ERZEUGT von tools/karte/gen_uebergang612.py aus gui/update_win32.cpp (Geteilt ... installAuftrag) des Updaters von rc612 (git 348f0e345; so steckt er in
// den installierten Fassungen rc606 bis rc612). Nur fuer tests/uebergangtest612.cpp (REL-TEMPO: alte Updater und das
// neue Release mit Starter, Manifest und Deltas). Ersetzt wurde nur, was der Test umlenken muss - alles andere ist
// Zeile fuer Zeile das Original:
//   - exePfad() -> g_simExe (die laufende .exe der simulierten Installation)
//   - schluessel() -> leer (der Test braucht keine Anmeldung), ausgabeVon entfaellt mit ihm
//   - prozessLaeuft entfaellt (nur beimStart, nicht im Test)
//   - ablageOrdner() (%TEMP%\\behaved_update) -> g_simTemp
//   - "https://api.github.com/repos/" -> g_simApi (der lokale Testserver)
//   - bhed::kFassung -> kAlteFassung ("rc612")
//   - bhed::update:: -> alt612_update:: (die Funktionen von rc612)
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

std::wstring exePfad() { return g_simExe; }   // ERSETZT: die simulierte laufende .exe

// ERSETZT: kein Schluessel (der Test braucht keine Anmeldung)
std::string schluessel() { return {}; }

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
    const std::wstring agent = L"JK-Studio/" + breit(kAlteFassung);
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
fs::path ablageOrdner() { return g_simTemp; }   // ERSETZT: die Ablage des Tests

// --- Die beiden Hintergrundauftraege --------------------------------------
void pruefAuftrag() {
    const std::wstring url = g_simApi + breit(alt612_update::kRepo) + L"/releases/latest";
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
    diag::detail("Update: GitHub (" + std::string(alt612_update::kRepo) + ") antwortet " + std::to_string(a.status) +
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
    alt612_update::Release r;
    std::string fehler;
    if (a.status != 200 || !alt612_update::parseRelease(a.body, r, &fehler)) {
        setze([&](Zustand& z) {
            z.stand = Stand::Fehler;
            z.meldung = text(Str::UpdErrBadAnswer, fehler, static_cast<long long>(a.status));
        });
        return;
    }
    const bool neuer = alt612_update::istNeuer(r.tag, lokaleFassung());
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

void installAuftrag(alt612_update::Release r) {
    const alt612_update::Asset* asset = alt612_update::zipAsset(r);
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
        const alt612_update::Asset* d = alt612_update::assetNamens(r, name);
        if (d == nullptr || d->downloadUrl.empty()) { return false; }
        const Antwort k = holen(breit(d->downloadUrl), {}, true);
        if (!k.fehler.empty() || k.status != 200) { return false; }
        inhalt = k.body;
        return true;
    };
    std::string summen;
    std::string signatur;
    const bool mitSummen = kleineDatei(alt612_update::kSummenDatei, summen);
    const bool mitSignatur = kleineDatei(alt612_update::kSignaturDatei, signatur);
    if (abgebrochen()) {
        zurueck();
        return;
    }
    diag::detail(std::string("Update: ") + alt612_update::kSummenDatei + (mitSummen ? " geladen" : " FEHLT") + ", " +
                 alt612_update::kSignaturDatei + (mitSignatur ? " geladen" : " fehlt"));
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
    const alt612_update::Einspielergebnis erg =
        alt612_update::einspielen(zipDatei.wstring(), asset->name, mitSummen ? &summen : nullptr,
                                 mitSignatur ? &signatur : nullptr, alt612_update::kSignaturSchluessel, exePfad(), &g().abbruch);
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

