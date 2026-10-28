// updatetest_echtheit.cpp - Stufe 8d: Echtheit des Downloads und das Einspielen
// (src/update_einspielen.cpp), ohne Netz. Teil von updatetest (ctest update).
#include <windows.h>
#include <bcrypt.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "bhed/pk3.h"
#include "bhed/update.h"

namespace {

namespace fs = std::filesystem;
using Expect = void (*)(const char*, bool);

std::string hex(const std::string& b) {
    static const char* const z = "0123456789abcdef";
    std::string h;
    for (const char c : b) {
        h.push_back(z[static_cast<unsigned char>(c) >> 4]);
        h.push_back(z[static_cast<unsigned char>(c) & 15]);
    }
    return h;
}

std::string ausHex(const std::string& h) {
    std::string b;
    for (std::size_t i = 0; i + 1 < h.size(); i += 2) { b.push_back(static_cast<char>(std::stoi(h.substr(i, 2), nullptr, 16))); }
    return b;
}

// Ein Testschluessel, nur im Speicher (BCrypt erzeugt ihn bei jedem Lauf neu).
struct TestSchluessel {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    std::string punktHex;   // "04" + X + Y
    TestSchluessel() {
        BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0);
        BCryptGenerateKeyPair(alg, &key, 256, 0);
        BCryptFinalizeKeyPair(key, 0);
        unsigned char blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64] = {};
        ULONG n = 0;
        BCryptExportKey(key, nullptr, BCRYPT_ECCPUBLIC_BLOB, blob, sizeof(blob), &n, 0);
        punktHex = "04" + hex(std::string(reinterpret_cast<char*>(blob) + sizeof(BCRYPT_ECCKEY_BLOB), 64));
    }
    ~TestSchluessel() {
        if (key != nullptr) { BCryptDestroyKey(key); }
        if (alg != nullptr) { BCryptCloseAlgorithmProvider(alg, 0); }
    }
    TestSchluessel(const TestSchluessel&) = delete;
    TestSchluessel& operator=(const TestSchluessel&) = delete;
    // Signatur ueber `n` als DER, wie "openssl dgst -sha256 -sign" sie schreibt.
    [[nodiscard]] std::string signiere(const std::string& n) const {
        const std::string h = ausHex(bhed::update::sha256Hex(n));
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

using Bild = std::map<std::string, std::string>;
// Alle Dateien unter `o` mit Inhalt - "nichts geaendert" heisst: dasselbe Bild.
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

}  // namespace

void echtheitsproben(Expect expect) {
    using namespace bhed::update;
    std::printf("\n--- Stufe 8d: Echtheit des Downloads ---\n");
    expect("sha256Hex(\"abc\") (FIPS 180-2)",
           sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string z = "JK-Studio-rc9999-windows-x64.zip";
    const std::string h = sha256Hex("x");
    expect("summeFuer: sha256sum-Format mit zwei Leerzeichen, LF", summeFuer(h + "  a.exe\n" + h + "  " + z + "\n", z) == h);
    expect("summeFuer: Binaer-Stern, CRLF, Grossbuchstaben",
           summeFuer("BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD *" + z + "\r\n", z) ==
               "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    expect("summeFuer: fehlender Name, Teilname, kaputte Zeile -> leer",
           summeFuer(h + "  a.zip\n", z).empty() && summeFuer(h + "  " + z + ".alt\n", z).empty() &&
               summeFuer("zz" + h.substr(2) + "  " + z + "\n", z).empty() && summeFuer("", z).empty());

    // Ein Vektor aus openssl ("openssl dgst -sha256 -sign", Wegwerf-Schluessel, der
    // private Teil ist geloescht): DER-Signaturen von openssl passen.
    const std::string osPunkt =
        "046108b9d4da1c691808575c71409e7a4c61001e05d200e0892535bc33ec017eb8bb4d5a3b5f9056a1faa2cfcc8ae4873212c3822c962c8889f9c4d98693e9baa3";
    const std::string osSig = ausHex(
        "3044022068d33cfb1b8c0d2f4a202cccc6ddde7eaf0be15ffe8401ea2eea4b5f858efd08022068c754b3df606d2a09fc39610cca06770838b7a74822a7ffee60206bc572846f");
    const std::string osText = "abc123  JK-Studio-rc9999-windows-x64.zip\n";
    expect("signaturGueltig: openssl-Signatur wird erkannt", signaturGueltig(osText, osSig, osPunkt));
    std::string anders = osText;
    anders[0] = 'b';
    std::string sigKaputt = osSig;
    sigKaputt[20] = static_cast<char>(sigKaputt[20] ^ 1);
    std::string punktFalsch = osPunkt;
    punktFalsch[10] = punktFalsch[10] == '0' ? '1' : '0';
    expect("signaturGueltig: geaenderter Text / Signatur / Schluessel -> falsch",
           !signaturGueltig(anders, osSig, osPunkt) && !signaturGueltig(osText, sigKaputt, osPunkt) &&
               !signaturGueltig(osText, osSig, punktFalsch));
    expect("signaturGueltig: kaputtes DER (abgeschnitten, Muell dahinter, leer) -> falsch",
           !signaturGueltig(osText, osSig.substr(0, osSig.size() - 1), osPunkt) &&
               !signaturGueltig(osText, osSig + "x", osPunkt) && !signaturGueltig(osText, "", osPunkt));
    const std::string eingebaut = kSignaturSchluessel;
    expect("kSignaturSchluessel ist leer (der Koordinator traegt ihn ein) oder ein P-256-Punkt",
           eingebaut.empty() || (eingebaut.size() == 130 && eingebaut.rfind("04", 0) == 0));

    const TestSchluessel ts;
    const std::string zipBytes = "PK-Inhalt";
    const std::string summen =
        sha256Hex("anderes") + "  JK-Studio-rc9999-windows-x64.exe\n" + sha256Hex(zipBytes) + "  " + z + "\n";
    const std::string sig = ts.signiere(summen);
    expect("pruefeEchtheit: ohne Schluessel reicht die Summe", pruefeEchtheit(z, zipBytes, &summen, nullptr, "") == Echtheit::Ok);
    expect("pruefeEchtheit: mit Schluessel und guter Signatur (BCrypt-Testschluessel)",
           pruefeEchtheit(z, zipBytes, &summen, &sig, ts.punktHex) == Echtheit::Ok);
    std::string summen2 = summen;
    summen2[0] = summen2[0] == 'a' ? 'b' : 'a';
    expect("pruefeEchtheit: Summen-Datei nach dem Signieren geaendert -> SignaturFalsch",
           pruefeEchtheit(z, zipBytes, &summen2, &sig, ts.punktHex) == Echtheit::SignaturFalsch);
    expect("pruefeEchtheit: mit Schluessel ohne .sig -> OhneSignatur",
           pruefeEchtheit(z, zipBytes, &summen, nullptr, ts.punktHex) == Echtheit::OhneSignatur);
    expect("pruefeEchtheit: ohne SHA256SUMS.txt -> OhneSummen",
           pruefeEchtheit(z, zipBytes, nullptr, nullptr, "") == Echtheit::OhneSummen);
    expect("pruefeEchtheit: anderes .zip -> NichtGelistet / SummeFalsch",
           pruefeEchtheit("x.zip", zipBytes, &summen, nullptr, "") == Echtheit::NichtGelistet &&
               pruefeEchtheit(z, zipBytes + " ", &summen, nullptr, "") == Echtheit::SummeFalsch);

    Release r;
    r.assets = {{"JK-Studio-rc9999-windows-x64.zip", "", "", 1}, {"sha256sums.txt", "", "", 1}, {"SHA256SUMS.txt.sig", "", "", 1}};
    expect("assetNamens: SHA256SUMS.txt (gross/klein egal) und .sig; zipAsset bleibt das .zip",
           assetNamens(r, kSummenDatei) == &r.assets[1] && assetNamens(r, kSignaturDatei) == &r.assets[2] &&
               zipAsset(r) == &r.assets[0] && assetNamens(r, "fehlt.txt") == nullptr);

    // --- einspielen: eine Ordner-Installation in einem Temp-Ordner ----------
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const fs::path wurzel = fs::path(temp) / (L"jkstudio_updatetest_" + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    fs::remove_all(wurzel, ec);
    const fs::path inst = wurzel / L"inst";
    const fs::path exe = inst / L"jkstudio.exe";
    const auto aufbauen = [&] {
        fs::remove_all(inst, ec);
        schreibe(exe, "ALTES PROGRAMM");
        schreibe(inst / L"behaved.exe", "ALTES PROGRAMM");
        schreibe(inst / L"data" / L"base" / L"behaved.bhc", "alte bhc");
        schreibe(inst / L"data" / L"x.txt", "altes x");
    };
    const std::vector<bhed::Pk3Datei> paket = {
        {"JK-Studio/jkstudio.exe", "NEUES PROGRAMM"},
        {"JK-Studio/behaved.exe", "NEUES PROGRAMM"},
        {"JK-Studio/data/base/behaved.bhc", "neue bhc"},
        {"JK-Studio/data/x.txt", "neues x"},
        {"JK-Studio/handbuch/index.html", "<p>neu</p>"},
    };
    const fs::path zipPfad = wurzel / z;
    fs::create_directories(wurzel, ec);
    std::string fehlerText;
    expect("Testpaket geschrieben", bhed::writePk3(zipPfad.string(), paket, &fehlerText));
    const std::string zipEcht = lies(zipPfad);
    const std::string summenEcht = sha256Hex(zipEcht) + "  " + z + "\n";
    const std::string sigEcht = ts.signiere(summenEcht);

    // Jede Ablehnung: dasselbe Bild des Programmordners, kein Zwischenordner.
    const auto abgelehnt = [&](const char* was, const std::string& zipInhalt, const std::string* su, const std::string* si,
                               const std::string& schluessel, Echtheit erwartet) {
        aufbauen();
        schreibe(zipPfad, zipInhalt);
        const Bild vorher = bildVon(inst);
        const Einspielergebnis e = einspielen(zipPfad.wstring(), z, su, si, schluessel, exe.wstring());
        const bool gleich = bildVon(inst) == vorher;
        std::printf("      (%s)\n", e.problem.c_str());
        expect(was, e.echtheit == erwartet && !e.problem.empty() && e.dateien == 0 && gleich &&
                        !fs::exists(inst / kZwischenOrdner, ec));
    };
    std::string zipManipuliert = zipEcht;
    zipManipuliert[zipManipuliert.find("neue bhc")] = 'N';   // ein Byte im Inhalt, das Verzeichnis bleibt lesbar
    const std::string summenFalsch = sha256Hex("etwas anderes") + "  " + z + "\n";
    const std::string summenAnderes = sha256Hex(zipEcht) + "  anderes.zip\n";
    std::string sigFalsch = sigEcht;
    sigFalsch[10] = static_cast<char>(sigFalsch[10] ^ 0x40);
    abgelehnt("einspielen: manipuliertes .zip (ein Byte) wird abgelehnt, nichts geaendert", zipManipuliert, &summenEcht,
              nullptr, "", Echtheit::SummeFalsch);
    abgelehnt("einspielen: falsche Summe wird abgelehnt, nichts geaendert", zipEcht, &summenFalsch, nullptr, "",
              Echtheit::SummeFalsch);
    abgelehnt("einspielen: Release ohne SHA256SUMS.txt wird abgelehnt, nichts geaendert", zipEcht, nullptr, nullptr, "",
              Echtheit::OhneSummen);
    abgelehnt("einspielen: .zip nicht in SHA256SUMS.txt wird abgelehnt, nichts geaendert", zipEcht, &summenAnderes, nullptr,
              "", Echtheit::NichtGelistet);
    abgelehnt("einspielen: falsche Signatur wird abgelehnt, nichts geaendert", zipEcht, &summenEcht, &sigFalsch, ts.punktHex,
              Echtheit::SignaturFalsch);
    abgelehnt("einspielen: fehlende Signatur (Schluessel im Programm) wird abgelehnt, nichts geaendert", zipEcht,
              &summenEcht, nullptr, ts.punktHex, Echtheit::OhneSignatur);
    {
        // Echt (die Summe passt), aber ein Eintrag weiter hinten ist unlesbar: der
        // Zwischenordner faengt es ab, bevor irgendetwas getauscht ist.
        std::string zipHalb = zipEcht;
        std::size_t p = 0;
        for (int i = 0; i < 3; ++i) { p = zipHalb.find(std::string("PK\x03\x04", 4), p + 1); }
        zipHalb[p + 3] = '\x05';
        const std::string summenHalb = sha256Hex(zipHalb) + "  " + z + "\n";
        abgelehnt("einspielen: unlesbarer 4. Eintrag (Summe passt) -> nichts geaendert", zipHalb, &summenHalb, nullptr, "",
                  Echtheit::Ok);
    }
    {
        // Ein Tausch scheitert (data/x.txt ist gesperrt): die schon getauschten
        // Dateien - auch das Programm - kommen zurueck.
        aufbauen();
        schreibe(zipPfad, zipEcht);
        const Bild vorher = bildVon(inst);
        const fs::path gesperrt = inst / L"data" / L"x.txt";
        HANDLE sperre = CreateFileW(gesperrt.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        const Einspielergebnis e = einspielen(zipPfad.wstring(), z, &summenEcht, nullptr, "", exe.wstring());
        CloseHandle(sperre);
        std::printf("      (%s)\n", e.problem.c_str());
        expect("einspielen: Tausch scheitert -> alle alten Dateien zurueck, kein Zwischenordner",
               !e.problem.empty() && e.dateien == 0 && bildVon(inst) == vorher && !fs::exists(inst / kZwischenOrdner, ec));
    }
    {
        aufbauen();
        schreibe(zipPfad, zipEcht);
        const Einspielergebnis e = einspielen(zipPfad.wstring(), z, &summenEcht, &sigEcht, ts.punktHex, exe.wstring());
        expect("einspielen: echtes .zip (Summe und Signatur) -> 5 Dateien, Programm getauscht, alte .exe als .alt",
               e.problem.empty() && e.echtheit == Echtheit::Ok && e.dateien == 5 && lies(exe) == "NEUES PROGRAMM" &&
                   lies(inst / L"jkstudio.exe.alt") == "ALTES PROGRAMM" && lies(inst / L"behaved.exe") == "NEUES PROGRAMM" &&
                   lies(inst / L"data" / L"base" / L"behaved.bhc") == "neue bhc" &&
                   lies(inst / L"handbuch" / L"index.html") == "<p>neu</p>" && !fs::exists(inst / kZwischenOrdner, ec));
        // Eigenstaendige .exe (kein data\base daneben): nur das Programm.
        fs::remove_all(inst, ec);
        const fs::path eigen = inst / L"JK-Studio-rc605-windows-x64.exe";
        schreibe(eigen, "ALT");
        const Einspielergebnis e2 = einspielen(zipPfad.wstring(), z, &summenEcht, nullptr, "", eigen.wstring());
        const Bild b = bildVon(inst);
        expect("einspielen: eigenstaendige .exe -> nur sie wird JK Studio, nichts daneben",
               e2.problem.empty() && e2.dateien == 1 && b.size() == 2 && lies(eigen) == "NEUES PROGRAMM" &&
                   b.count("JK-Studio-rc605-windows-x64.exe.alt") == 1);
    }
    fs::remove_all(wurzel, ec);
}
