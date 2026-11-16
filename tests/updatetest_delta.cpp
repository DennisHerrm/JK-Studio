// updatetest_delta.cpp - REL-TEMPO: Manifest, Delta-Pakete und der ganze Ablauf (src/update_delta.cpp),
// ohne Netz. Teil von updatetest (ctest update).
#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "bhed/pk3.h"
#include "bhed/update.h"

namespace {

namespace fs = std::filesystem;
using Expect = void (*)(const char*, bool);
using namespace bhed::update;

using Bild = std::map<std::string, std::string>;
Bild bildVon(const fs::path& o) {
    Bild b;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(o, ec), ende; it != ende; it.increment(ec)) {
        if (!it->is_regular_file(ec)) { continue; }
        std::ifstream f(it->path(), std::ios::binary);
        b[fs::relative(it->path(), o, ec).generic_string()] =
            std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    return b;
}

void schreibe(const fs::path& p, const std::string& inhalt) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f << inhalt;
}

std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

std::string ausHex(const std::string& h) {
    std::string b;
    for (std::size_t i = 0; i + 1 < h.size(); i += 2) { b.push_back(static_cast<char>(std::stoi(h.substr(i, 2), nullptr, 16))); }
    return b;
}

// Ein Testschluessel nur im Speicher (wie updatetest_echtheit.cpp).
struct TestSchluessel {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    std::string punktHex;
    TestSchluessel() {
        BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0);
        BCryptGenerateKeyPair(alg, &key, 256, 0);
        BCryptFinalizeKeyPair(key, 0);
        unsigned char blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64] = {};
        ULONG n = 0;
        BCryptExportKey(key, nullptr, BCRYPT_ECCPUBLIC_BLOB, blob, sizeof(blob), &n, 0);
        static const char* const z = "0123456789abcdef";
        punktHex = "04";
        for (std::size_t i = sizeof(BCRYPT_ECCKEY_BLOB); i < sizeof(blob); ++i) {
            punktHex.push_back(z[blob[i] >> 4]);
            punktHex.push_back(z[blob[i] & 15]);
        }
    }
    ~TestSchluessel() {
        if (key != nullptr) { BCryptDestroyKey(key); }
        if (alg != nullptr) { BCryptCloseAlgorithmProvider(alg, 0); }
    }
    TestSchluessel(const TestSchluessel&) = delete;
    TestSchluessel& operator=(const TestSchluessel&) = delete;
    [[nodiscard]] std::string signiere(const std::string& n) const {
        const std::string h = ausHex(sha256Hex(n));
        unsigned char roh[64] = {};
        ULONG len = 0;
        BCryptSignHash(key, nullptr, reinterpret_cast<PUCHAR>(const_cast<char*>(h.data())), 32, roh, 64, &len, 0);
        const auto ganz = [](const unsigned char* p) {
            std::string v(reinterpret_cast<const char*>(p), 32);
            while (v.size() > 1 && v[0] == 0 && (static_cast<unsigned char>(v[1]) & 0x80) == 0) { v.erase(0, 1); }
            if ((static_cast<unsigned char>(v[0]) & 0x80) != 0) { v.insert(0, 1, '\0'); }
            return std::string("\x02", 1) + static_cast<char>(v.size()) + v;
        };
        const std::string inhalt = ganz(roh) + ganz(roh + 32);
        return std::string("\x30", 1) + static_cast<char>(inhalt.size()) + inhalt;
    }
};

std::string manifestText(const std::string& rc, const std::map<std::string, std::string>& dateien) {
    std::string t = "# JK Studio manifest 1\nfassung " + rc + "\n";
    for (const auto& [p, inhalt] : dateien) { t += sha256Hex(inhalt) + " " + std::to_string(inhalt.size()) + " " + p + "\n"; }
    return t;
}

// Eine Quelle aus dem Speicher: Name -> Bytes; merkt die Reihenfolge der Anfragen.
class SpeicherQuelle final : public Quelle {
public:
    std::map<std::string, std::string> dateien;
    std::vector<std::string> anfragen;
    std::atomic<bool>* abbruchBei = nullptr;   // beim Laden dieser Datei "Abbrechen" druecken
    std::string abbruchName;
    bool laden(const Asset& a, const std::wstring& ziel, const std::function<void(std::uint64_t, std::uint64_t)>& f,
               std::string& fehler) override {
        anfragen.push_back(a.name);
        const auto it = dateien.find(a.name);
        if (it == dateien.end()) {
            fehler = "HTTP 404";
            return false;
        }
        if (abbruchBei != nullptr && a.name == abbruchName) {
            *abbruchBei = true;
            fehler = "abgebrochen";
            return false;
        }
        schreibe(fs::path(ziel), it->second);
        if (f) { f(it->second.size(), it->second.size()); }
        return true;
    }
};

Release releaseAus(const std::string& tag, const std::map<std::string, std::string>& dateien) {
    Release r;
    r.tag = tag;
    for (const auto& [n, b] : dateien) { r.assets.push_back({n, "", "https://example.invalid/" + n, static_cast<long long>(b.size())}); }
    return r;
}

}  // namespace

void deltaproben(Expect expect) {
    std::printf("\n--- REL-TEMPO: Manifest und Delta ---\n");
    expect("rcVon / zipName / manifestName / deltaName",
           rcVon("v1.0.0-rc613") == "rc613" && rcVon("rc612") == "rc612" && rcVon("v1.0.0").empty() &&
               zipName("rc613") == "JK-Studio-rc613-windows-x64.zip" && manifestName("rc613") == "JK-Studio-rc613-manifest.txt" &&
               deltaName("rc613", "rc612") == "JK-Studio-rc613-from-rc612.delta");
    // Die Updater bis rc612 nehmen das erste .zip - ein Delta darf keines sein.
    Release ra = releaseAus("v1.0.0-rc613", {{"JK-Studio-Starter.exe", "s"}, {"JK-Studio-rc613-from-rc612.delta", "d"},
                                              {"JK-Studio-rc613-manifest.txt", "m"}, {"JK-Studio-rc613-windows-x64.zip", "z"}});
    expect("zipAsset nimmt bei Starter, Delta und Manifest weiter das volle Paket",
           zipAsset(ra) != nullptr && zipAsset(ra)->name == "JK-Studio-rc613-windows-x64.zip");

    Manifest m;
    std::string f;
    const std::string h = sha256Hex("x");
    expect("parseManifest: Kopf, Fassung, Pfade mit Leerzeichen und Unterordnern",
           parseManifest("# JK Studio manifest 1\nfassung rc613\n" + h + " 1 jkstudio.exe\r\n" + h + " 1 handbuch/en/a b.html\n", m, &f) &&
               m.fassung == "rc613" && m.dateien.size() == 2 && m.dateien[1].pfad == "handbuch/en/a b.html" && m.dateien[1].groesse == 1);
    const auto falsch = [&](const std::string& t) { return !parseManifest(t, m, &f) && !f.empty(); };
    expect("parseManifest weist ab: fremder Kopf, ohne Fassung, kaputte Summe, '..', absolut, Rueckstrich, doppelt (gross/klein)",
           falsch("# anderes 1\nfassung rc613\n" + h + " 1 a\n") && falsch("# JK Studio manifest 1\n" + h + " 1 a\n") &&
               falsch("# JK Studio manifest 1\nfassung rc613\n" + h.substr(1) + "q 1 a\n") &&
               falsch("# JK Studio manifest 1\nfassung rc613\n" + h + " 1 ../a\n") &&
               falsch("# JK Studio manifest 1\nfassung rc613\n" + h + " 1 C:/a\n") &&
               falsch("# JK Studio manifest 1\nfassung rc613\n" + h + " 1 a\\b\n") &&
               falsch("# JK Studio manifest 1\nfassung rc613\n" + h + " 1 A.txt\n" + h + " 1 a.txt\n"));
    DeltaKopf k;
    expect("parseDeltaKopf: von, nach, weg",
           parseDeltaKopf("# JK Studio delta 1\nvon rc612\nnach rc613\nweg handbuch/alt.html\n", k, &f) && k.von == "rc612" &&
               k.nach == "rc613" && k.weg.size() == 1);
    expect("parseDeltaKopf weist ab: fremde Zeile, '..' in weg, ohne von",
           !parseDeltaKopf("# JK Studio delta 1\nvon rc612\nnach rc613\nloesche a\n", k, &f) &&
               !parseDeltaKopf("# JK Studio delta 1\nvon rc612\nnach rc613\nweg ../a\n", k, &f) &&
               !parseDeltaKopf("# JK Studio delta 1\nnach rc613\n", k, &f));

    // --- einspielenDelta an einer Ordner-Installation --------------------------------
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const fs::path wurzel = fs::path(temp) / (L"jkstudio_deltatest_" + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    fs::remove_all(wurzel, ec);
    const fs::path inst = wurzel / L"inst";
    const fs::path exe = inst / L"jkstudio.exe";
    expect("sha256Datei = sha256Hex des Inhalts, fehlende Datei leer",
           (schreibe(wurzel / L"probe.bin", std::string(3000000, 'a') + "b"), sha256Datei((wurzel / L"probe.bin").wstring()) ==
                                                                               sha256Hex(std::string(3000000, 'a') + "b")) &&
               sha256Datei((wurzel / L"fehlt.bin").wstring()).empty());
    const std::map<std::string, std::string> alt = {
        {"jkstudio.exe", "ALTES PROGRAMM"}, {"behaved.exe", "ALTES PROGRAMM"}, {"data/base/behaved.bhc", "alte bhc"},
        {"data/x.txt", "x bleibt"}, {"handbuch/alt.html", "verschoben"}, {"version.txt", "rc900\n"}};
    std::map<std::string, std::string> neu = {
        {"jkstudio.exe", "NEUES PROGRAMM"}, {"behaved.exe", "NEUES PROGRAMM"}, {"data/base/behaved.bhc", "neue bhc"},
        {"data/x.txt", "x bleibt"}, {"handbuch/neu.html", "<p>neu</p>"}, {"data/y.txt", "verschoben"}, {"version.txt", "rc901\n"}};
    const auto aufbauen = [&] {
        fs::remove_all(inst, ec);
        for (const auto& [p, b] : alt) { schreibe(inst / fs::path(std::u8string(reinterpret_cast<const char8_t*>(p.c_str()))), b); }
    };
    // Das Delta: nur die Inhalte, die rc900 nicht hat (das neue Programm einmal, obwohl es zwei Dateien sind;
    // "verschoben" liegt schon da), dazu was wegfaellt.
    const auto deltaBauen = [&](const fs::path& ziel, const std::string& von, const std::map<std::string, std::string>& n,
                                const std::string& verfaelscht) {
        std::vector<bhed::Pk3Datei> d{{kDeltaKopf, "# JK Studio delta 1\nvon " + von + "\nnach rc901\nweg handbuch/alt.html\n"}};
        std::map<std::string, std::string> inhalte;
        std::map<std::string, bool> altHat;
        for (const auto& [p, b] : alt) { altHat[sha256Hex(b)] = true; }
        for (const auto& [p, b] : n) {
            if (!altHat.count(sha256Hex(b))) { inhalte[sha256Hex(b)] = b; }
        }
        for (const auto& [hx, b] : inhalte) { d.push_back({std::string(kDeltaInhalt) + hx, hx == verfaelscht ? b + "!" : b}); }
        std::string fe;
        return bhed::writePk3(ziel.string(), d, &fe);
    };
    const fs::path dPfad = wurzel / L"JK-Studio-rc901-from-rc900.delta";
    const fs::path mPfad = wurzel / L"JK-Studio-rc901-manifest.txt";
    const std::string dName = "JK-Studio-rc901-from-rc900.delta";
    const std::string mName = "JK-Studio-rc901-manifest.txt";
    expect("Testdelta geschrieben", deltaBauen(dPfad, "rc900", neu, ""));
    schreibe(mPfad, manifestText("rc901", neu));
    const auto summenFuer = [&] {
        return sha256Hex(lies(dPfad)) + "  " + dName + "\n" + sha256Hex(lies(mPfad)) + "  " + mName + "\n";
    };
    std::string summen = summenFuer();
    const TestSchluessel ts;
    const auto abgelehnt = [&](const char* was, const std::string* su, const std::string* si, const std::string& schl,
                               const std::string& von) {
        aufbauen();
        const Bild vorher = bildVon(inst);
        const Einspielergebnis e = einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, su, si, schl, exe.wstring(), von, "rc901");
        std::printf("      (%s)\n", e.problem.c_str());
        expect(was, !e.problem.empty() && e.dateien == 0 && bildVon(inst) == vorher && !fs::exists(inst / kZwischenOrdner, ec));
    };
    {
        aufbauen();
        const std::string sig = ts.signiere(summen);
        const Einspielergebnis e =
            einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, &sig, ts.punktHex, exe.wstring(), "rc900", "rc901");
        Bild soll;
        for (const auto& [p, b] : neu) { soll[p] = b; }
        soll["jkstudio.exe.alt"] = "ALTES PROGRAMM";
        std::printf("      (%s) %d Dateien, %d weg\n", e.problem.c_str(), e.dateien, e.geloescht);
        expect("einspielenDelta: echt (Summe und Signatur) -> genau die neue Fassung, Programm getauscht, handbuch/alt.html weg, "
               "data/y.txt aus der verschobenen Datei, kein Zwischenordner",
               e.problem.empty() && e.dateien == 6 && e.geloescht == 1 && bildVon(inst) == soll && !fs::exists(inst / kZwischenOrdner, ec));
        expect("einspielenDelta: noch einmal (schon alles neu) -> nichts zu tun",
               einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, &sig, ts.punktHex, exe.wstring(), "rc900", "rc901")
                       .problem.empty() &&
                   bildVon(inst) == soll);
    }
    std::string summenFalsch = summen;
    summenFalsch[0] = summenFalsch[0] == 'a' ? 'b' : 'a';
    abgelehnt("einspielenDelta: Delta passt nicht zu SHA256SUMS.txt -> nichts geaendert", &summenFalsch, nullptr, "", "rc900");
    const std::string ohneManifest = sha256Hex(lies(dPfad)) + "  " + dName + "\n";
    abgelehnt("einspielenDelta: Manifest nicht in SHA256SUMS.txt -> nichts geaendert", &ohneManifest, nullptr, "", "rc900");
    abgelehnt("einspielenDelta: ohne SHA256SUMS.txt -> nichts geaendert", nullptr, nullptr, "", "rc900");
    abgelehnt("einspielenDelta: Schluessel im Programm, aber keine Signatur -> nichts geaendert", &summen, nullptr, ts.punktHex, "rc900");
    abgelehnt("einspielenDelta: Delta fuer eine andere Fassung (installiert rc899) -> nichts geaendert", &summen, nullptr, "", "rc899");
    {
        // Ein Inhalt im Delta ist falsch, das Delta selbst aber gelistet: die Pruefung je Datei faengt es.
        expect("Testdelta mit falschem Inhalt", deltaBauen(dPfad, "rc900", neu, sha256Hex("neue bhc")));
        const std::string s2 = summenFuer();
        abgelehnt("einspielenDelta: ein Inhalt passt nicht zum Manifest -> nichts geaendert", &s2, nullptr, "", "rc900");
        expect("Testdelta wieder richtig", deltaBauen(dPfad, "rc900", neu, ""));
        summen = summenFuer();
    }
    {
        // Eine Datei im Ordner wurde von Hand geaendert und kommt im neuen Manifest unveraendert vor - das Delta
        // hat ihren Inhalt nicht (rc900 hatte ihn ja): nichts geaendert, der Aufrufer nimmt das volle Paket.
        aufbauen();
        schreibe(inst / L"data" / L"x.txt", "von Hand geaendert");
        const Bild vorher = bildVon(inst);
        const Einspielergebnis e =
            einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, nullptr, "", exe.wstring(), "rc900", "rc901");
        std::printf("      (%s)\n", e.problem.c_str());
        expect("einspielenDelta: geaenderte Datei ohne Inhalt im Delta -> nichts geaendert (dann das volle Paket)",
               !e.problem.empty() && e.dateien == 0 && bildVon(inst) == vorher && !fs::exists(inst / kZwischenOrdner, ec));
    }
    {
        // Ein Tausch scheitert (data/base/behaved.bhc gesperrt): alles zurueck.
        aufbauen();
        const Bild vorher = bildVon(inst);
        HANDLE sperre = CreateFileW((inst / L"data" / L"base" / L"behaved.bhc").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, 0, nullptr);
        const Einspielergebnis e =
            einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, nullptr, "", exe.wstring(), "rc900", "rc901");
        CloseHandle(sperre);
        std::printf("      (%s)\n", e.problem.c_str());
        expect("einspielenDelta: Tausch scheitert -> alle alten Dateien zurueck, auch die weggefallene",
               !e.problem.empty() && e.dateien == 0 && bildVon(inst) == vorher && !fs::exists(inst / kZwischenOrdner, ec));
    }
    {
        // Das Programm endet mitten im Tausch: der naechste Start stellt alles zurueck.
        aufbauen();
        const Bild vorher = bildVon(inst);
        testHakenSetzen([](TestPhase p, const std::string& rel) {
            if (p == TestPhase::Getauscht && rel == "data/base/behaved.bhc") { throw std::runtime_error("Ende"); }
        });
        bool ab = false;
        try {
            (void)einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, nullptr, "", exe.wstring(), "rc900", "rc901");
        } catch (const std::runtime_error&) {
            ab = true;
        }
        testHakenSetzen({});
        const int n = resteAufraeumen(exe.wstring());
        expect("einspielenDelta mitten im Tausch beendet -> resteAufraeumen: alles wie vorher",
               ab && n >= 1 && bildVon(inst) == vorher && !fs::exists(inst / kZwischenOrdner, ec));
    }
    {
        // REL-TEMPO: ein fertiger Tausch ohne "<exe>.alt" (das Programm blieb gleich) - nur der Zwischenordner
        // blieb liegen, mit der Marke "fertig": nichts zurueckstellen.
        aufbauen();
        fs::create_directories(inst / kZwischenOrdner / L"alt" / L"data", ec);
        schreibe(inst / kZwischenOrdner / L"alt" / L"data" / L"x.txt", "uralt");
        schreibe(inst / kZwischenOrdner / L"fertig", "ok\n");
        const int n = resteAufraeumen(exe.wstring());
        expect("resteAufraeumen mit der Marke fertig: nichts zurueck, nur aufgeraeumt",
               n == 0 && lies(inst / L"data" / L"x.txt") == "x bleibt" && !fs::exists(inst / kZwischenOrdner, ec));
    }
    {
        // Eigenstaendige .exe (kein data\base daneben): auch aus dem Delta nur das Programm.
        fs::remove_all(inst, ec);
        const fs::path eigen = inst / L"JK-Studio-rc605-windows-x64.exe";
        schreibe(eigen, "ALTES PROGRAMM");
        const Einspielergebnis e =
            einspielenDelta(dPfad.wstring(), dName, mPfad.wstring(), mName, &summen, nullptr, "", eigen.wstring(), "rc900", "rc901");
        const Bild b = bildVon(inst);
        expect("einspielenDelta: eigenstaendige .exe -> nur sie wird neu, nichts daneben",
               e.problem.empty() && e.dateien == 1 && b.size() == 2 && lies(eigen) == "NEUES PROGRAMM" &&
                   b.count("JK-Studio-rc605-windows-x64.exe.alt") == 1);
    }

    // --- Umfang::Alles (der Starter): das volle Paket in einen leeren Ordner ------------------------
    {
        std::vector<bhed::Pk3Datei> paket;
        for (const auto& [p, b] : neu) { paket.push_back({"JK-Studio/" + p, b}); }
        const fs::path zPfad = wurzel / L"JK-Studio-rc901-windows-x64.zip";
        std::string fe;
        expect("Testpaket (voll) geschrieben", bhed::writePk3(zPfad.string(), paket, &fe));
        const std::string zs = sha256Hex(lies(zPfad)) + "  JK-Studio-rc901-windows-x64.zip\n";
        const fs::path leer = wurzel / L"leer" / L"JK-Studio";
        const Einspielergebnis e1 = einspielen(zPfad.wstring(), "JK-Studio-rc901-windows-x64.zip", &zs, nullptr, "",
                                               (leer / L"jkstudio.exe").wstring(), nullptr, Umfang::WieDaneben);
        const std::size_t nurProgramm = bildVon(leer).size();
        fs::remove_all(leer, ec);
        const Einspielergebnis e2 = einspielen(zPfad.wstring(), "JK-Studio-rc901-windows-x64.zip", &zs, nullptr, "",
                                               (leer / L"jkstudio.exe").wstring(), nullptr, Umfang::Alles);
        Bild soll;
        for (const auto& [p, b] : neu) { soll[p] = b; }
        expect("einspielen Umfang::Alles in einen leeren Ordner: jede Datei (WieDaneben: nur das Programm)",
               e1.problem.empty() && nurProgramm == 1 && e2.problem.empty() && bildVon(leer) == soll &&
                   !fs::exists(leer / kZwischenOrdner, ec));

        // --- aktualisieren: Delta zuerst, sonst das volle Paket -----------------------------------------
        const std::string sigUnbenutzt;
        SpeicherQuelle q;
        q.dateien[dName] = lies(dPfad);
        q.dateien[mName] = lies(mPfad);
        q.dateien["JK-Studio-rc901-windows-x64.zip"] = lies(zPfad);
        q.dateien[kSummenDatei] = summen + zs;
        const Release r = releaseAus("v1.0.0-rc901", q.dateien);
        const fs::path ablage = wurzel / L"ablage";
        aufbauen();
        std::vector<std::uint64_t> fort;
        const auto fortschritt = [&](std::uint64_t n, std::uint64_t g) {
            (void)g;
            fort.push_back(n);
        };
        Ablaufergebnis a = aktualisieren(r, "rc900", exe.wstring(), q, ablage.wstring(), "", nullptr, fortschritt);
        std::printf("      (Anfragen: %zu, zuerst %s)\n", q.anfragen.size(), q.anfragen.empty() ? "-" : q.anfragen[0].c_str());
        expect("aktualisieren: Delta fuer die installierte Fassung -> nur Delta, Manifest und SHA256SUMS.txt (das .zip nie)",
               a.mitDelta && a.einspielen.problem.empty() && q.anfragen.size() == 3 && q.anfragen[0] == dName &&
                   q.anfragen[1] == mName && q.anfragen[2] == kSummenDatei && lies(exe) == "NEUES PROGRAMM" && !fort.empty());
        // Das Delta ist kaputt: dann das volle Paket, und es wird richtig.
        aufbauen();
        q.anfragen.clear();
        q.dateien[dName] = lies(dPfad) + "kaputt";
        a = aktualisieren(r, "rc900", exe.wstring(), q, ablage.wstring(), "", nullptr, fortschritt);
        std::printf("      (Delta: %s)\n", a.deltaGrund.c_str());
        expect("aktualisieren: Delta passt nicht zu SHA256SUMS.txt -> das volle Paket, die Installation ist trotzdem neu",
               !a.mitDelta && !a.deltaGrund.empty() && a.einspielen.problem.empty() &&
                   std::find(q.anfragen.begin(), q.anfragen.end(), "JK-Studio-rc901-windows-x64.zip") != q.anfragen.end() &&
                   // das volle Paket laesst wie bisher Dateien liegen, die es nicht mehr hat
                   lies(exe) == "NEUES PROGRAMM" && lies(inst / L"version.txt") == "rc901\n" &&
                   fs::exists(inst / L"handbuch" / L"alt.html", ec));
        q.dateien[dName] = lies(dPfad);
        // Kein Delta fuer diese Fassung, oder die Fassung unbekannt (Starter ohne version.txt): gleich das .zip.
        aufbauen();
        q.anfragen.clear();
        a = aktualisieren(r, "rc899", exe.wstring(), q, ablage.wstring(), "", nullptr, fortschritt);
        expect("aktualisieren: kein Delta von rc899 -> das .zip zuerst, dann SHA256SUMS.txt",
               !a.mitDelta && a.einspielen.problem.empty() && q.anfragen.size() == 2 &&
                   q.anfragen[0] == "JK-Studio-rc901-windows-x64.zip" && q.anfragen[1] == kSummenDatei);
        aufbauen();
        q.anfragen.clear();
        a = aktualisieren(r, "", exe.wstring(), q, ablage.wstring(), "", nullptr, fortschritt);
        expect("aktualisieren: Fassung unbekannt -> das .zip", !a.mitDelta && a.einspielen.problem.empty() && q.anfragen.size() == 2);
        // Abbrechen beim Laden des Deltas: nichts weiter geladen, nichts geaendert.
        aufbauen();
        const Bild vorher = bildVon(inst);
        q.anfragen.clear();
        std::atomic<bool> abbruch{false};
        q.abbruchBei = &abbruch;
        q.abbruchName = dName;
        a = aktualisieren(r, "rc900", exe.wstring(), q, ablage.wstring(), "", &abbruch, fortschritt);
        q.abbruchBei = nullptr;
        expect("aktualisieren: Abbrechen beim Delta -> nichts weiter geladen (kein .zip, keine Summen), nichts geaendert",
               !a.ladeFehler.empty() && q.anfragen.size() == 1 && bildVon(inst) == vorher);
    }
    fs::remove_all(wurzel, ec);
}
