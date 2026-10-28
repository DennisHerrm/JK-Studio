// update_einspielen.cpp - Stufe 8d: Echtheit des Downloads und das Einspielen (siehe update.h)
//
// Bis rc605 stand das Einspielen in gui/update_win32.cpp: jede Datei des .zip
// wurde als "<name>.neu" neben ihr Ziel geschrieben und sofort getauscht - ein
// kaputter Eintrag mitten im Archiv liess eine halbe Installation zurueck, und
// geprueft wurde nur der Pfad (zielImOrdner), nicht der Inhalt. Jetzt hier
// (ohne Netz und Fenster pruefbar, tests/updatetest.cpp):
//
//   pruefen   SHA-256 des .zip gegen SHA256SUMS.txt, mit Schluessel im Programm
//             zusaetzlich ECDSA P-256 ueber SHA256SUMS.txt (BCrypt, Windows-Bordmittel)
//   entpacken alles in den Zwischenordner neben der .exe
//   tauschen  erst wenn alles da ist; scheitert ein Tausch, kommen die alten
//             Dateien zurueck
#include "bhed/update.h"

#include <windows.h>
#include <bcrypt.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "bhed/diag.h"
#include "bhed/i18n.h"
#include "bhed/pk3.h"

namespace bhed::update {
namespace {

namespace fs = std::filesystem;
using ::bhed::i18n::Str;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// SHA-256 (32 Byte roh) mit BCrypt; leer bei einem Fehler.
std::string sha256Roh(const std::string& daten) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) { return {}; }
    std::string roh(32, '\0');
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = BCRYPT_SUCCESS(BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0));
    // In Stuecken: ULONG reicht fuer ein .zip ueber 4 GB nicht.
    std::size_t pos = 0;
    while (ok && pos < daten.size()) {
        const std::size_t n = std::min<std::size_t>(daten.size() - pos, 1u << 30);
        ok = BCRYPT_SUCCESS(BCryptHashData(h, reinterpret_cast<PUCHAR>(const_cast<char*>(daten.data() + pos)),
                                           static_cast<ULONG>(n), 0));
        pos += n;
    }
    ok = ok && BCRYPT_SUCCESS(BCryptFinishHash(h, reinterpret_cast<PUCHAR>(roh.data()), 32, 0));
    if (h != nullptr) { BCryptDestroyHash(h); }
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok ? roh : std::string();
}

bool hexZuBytes(const std::string& hex, std::string& out) {
    out.clear();
    if (hex.size() % 2 != 0) { return false; }
    const auto wert = [](char c) -> int {
        if (c >= '0' && c <= '9') { return c - '0'; }
        if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
        if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
        return -1;
    };
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int a = wert(hex[i]);
        const int b = wert(hex[i + 1]);
        if (a < 0 || b < 0) { return false; }
        out.push_back(static_cast<char>((a << 4) | b));
    }
    return true;
}

// ASN.1 DER "SEQUENCE { INTEGER r, INTEGER s }" -> r||s mit je 32 Byte (BCrypt).
// Streng: kurze Laengen, nichts dahinter, r und s hoechstens 32 Byte (plus
// einer fuehrenden 0).
bool derZuRoh(const std::string& der, std::string& roh) {
    const auto* p = reinterpret_cast<const unsigned char*>(der.data());
    const std::size_t n = der.size();
    if (n < 8 || p[0] != 0x30 || p[1] >= 0x80 || static_cast<std::size_t>(p[1]) + 2 != n) { return false; }
    roh.assign(64, '\0');
    std::size_t i = 2;
    for (int teil = 0; teil < 2; ++teil) {
        if (i + 2 > n || p[i] != 0x02 || p[i + 1] >= 0x80) { return false; }
        std::size_t len = p[i + 1];
        i += 2;
        if (len == 0 || i + len > n) { return false; }
        const unsigned char* z = p + i;
        i += len;
        while (len > 0 && *z == 0) { ++z; --len; }   // fuehrende Nullen
        if (len > 32) { return false; }
        std::memcpy(roh.data() + teil * 32 + (32 - len), z, len);
    }
    return i == n;
}

// Liest eine Datei ganz; false, wenn sie fehlt.
bool liesDatei(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { return false; }
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !f.bad();
}

std::string schmal(const std::wstring& w) {
    if (w.empty()) { return {}; }
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

// UTF-8 -> Pfad (fs::u8path ist seit C++20 veraltet).
fs::path utf8Pfad(const std::string& s) {
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
}

std::string meldung(Str id, const std::string& a = {}, long long n = 0) {
    char z[512];
    std::snprintf(z, sizeof(z), ::bhed::i18n::tr(id), a.c_str(), n);
    return z;
}

}  // namespace

const Asset* assetNamens(const Release& r, const char* name) {
    const std::string gesucht = klein(name);
    for (const Asset& a : r.assets) {
        if (klein(a.name) == gesucht) { return &a; }
    }
    return nullptr;
}

std::string sha256Hex(const std::string& daten) {
    const std::string roh = sha256Roh(daten);
    std::string hex;
    static const char* const ziffern = "0123456789abcdef";
    for (const char c : roh) {
        const auto b = static_cast<unsigned char>(c);
        hex.push_back(ziffern[b >> 4]);
        hex.push_back(ziffern[b & 15]);
    }
    return hex;
}

std::string summeFuer(const std::string& summen, const std::string& datei) {
    std::size_t a = 0;
    while (a < summen.size()) {
        std::size_t b = summen.find('\n', a);
        if (b == std::string::npos) { b = summen.size(); }
        std::string zeile = summen.substr(a, b - a);
        a = b + 1;
        if (!zeile.empty() && zeile.back() == '\r') { zeile.pop_back(); }
        // "<64 hex><Leerzeichen><Leerzeichen oder *><Name>"
        if (zeile.size() < 67 || zeile[64] != ' ' || (zeile[65] != ' ' && zeile[65] != '*')) { continue; }
        std::string hex = klein(zeile.substr(0, 64));
        std::string roh;
        if (!hexZuBytes(hex, roh)) { continue; }
        if (zeile.compare(66, std::string::npos, datei) == 0) { return hex; }
    }
    return {};
}

bool signaturGueltig(const std::string& nachricht, const std::string& signaturDer, const std::string& schluesselHex) {
    std::string punkt;
    std::string sig;
    if (!hexZuBytes(schluesselHex, punkt) || punkt.size() != 65 || punkt[0] != 0x04) { return false; }
    if (!derZuRoh(signaturDer, sig)) { return false; }
    const std::string hash = sha256Roh(nachricht);
    if (hash.size() != 32) { return false; }
    // BCRYPT_ECCPUBLIC_BLOB: Kopf (Magic, 32) + X + Y.
    std::vector<unsigned char> blob(sizeof(BCRYPT_ECCKEY_BLOB) + 64);
    BCRYPT_ECCKEY_BLOB kopf{BCRYPT_ECDSA_PUBLIC_P256_MAGIC, 32};
    std::memcpy(blob.data(), &kopf, sizeof(kopf));
    std::memcpy(blob.data() + sizeof(kopf), punkt.data() + 1, 64);
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0))) { return false; }
    BCRYPT_KEY_HANDLE key = nullptr;
    bool ok = BCRYPT_SUCCESS(BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key, blob.data(),
                                                 static_cast<ULONG>(blob.size()), 0));
    ok = ok && BCRYPT_SUCCESS(BCryptVerifySignature(key, nullptr, reinterpret_cast<PUCHAR>(const_cast<char*>(hash.data())), 32,
                                                    reinterpret_cast<PUCHAR>(sig.data()), 64, 0));
    if (key != nullptr) { BCryptDestroyKey(key); }
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

Echtheit pruefeEchtheit(const std::string& zipName, const std::string& zip, const std::string* summen,
                        const std::string* signatur, const std::string& schluesselHex) {
    if (summen == nullptr) { return Echtheit::OhneSummen; }
    // Erst die Signatur ueber die Summen-Datei: ohne sie waeren die Summen
    // nur so echt wie der Download selbst.
    if (!schluesselHex.empty()) {
        if (signatur == nullptr) { return Echtheit::OhneSignatur; }
        if (!signaturGueltig(*summen, *signatur, schluesselHex)) { return Echtheit::SignaturFalsch; }
    }
    const std::string soll = summeFuer(*summen, zipName);
    if (soll.empty()) { return Echtheit::NichtGelistet; }
    const std::string ist = sha256Hex(zip);
    return (!ist.empty() && ist == soll) ? Echtheit::Ok : Echtheit::SummeFalsch;
}

Einspielergebnis einspielen(const std::wstring& zipPfad, const std::string& zipName, const std::string* summen,
                            const std::string* signatur, const std::string& schluesselHex, const std::wstring& exeW) {
    Einspielergebnis erg;
    const fs::path exe(exeW);
    const fs::path ordner = exe.parent_path();
    std::error_code ec;

    // 1. Pruefen - die Bytes, die gleich der pk3-Leser oeffnet.
    std::string zip;
    if (!liesDatei(fs::path(zipPfad), zip)) {
        erg.problem = meldung(Str::UpdErrZip, schmal(zipPfad));
        return erg;
    }
    erg.echtheit = pruefeEchtheit(zipName, zip, summen, signatur, schluesselHex);
    diag::detail("Update: Pruefung von " + zipName + ": " +
                 (erg.echtheit == Echtheit::Ok ? std::string("echt (SHA-256") + (schluesselHex.empty() ? ")" : " und Signatur)")
                                               : "ABGELEHNT (" + std::to_string(static_cast<int>(erg.echtheit)) + ")"));
    switch (erg.echtheit) {
    case Echtheit::Ok: break;
    case Echtheit::OhneSummen: erg.problem = meldung(Str::UpdErrNoSums); return erg;
    case Echtheit::NichtGelistet: erg.problem = meldung(Str::UpdErrNotListed, zipName); return erg;
    case Echtheit::SummeFalsch: erg.problem = meldung(Str::UpdErrChecksum, zipName); return erg;
    case Echtheit::OhneSignatur: erg.problem = meldung(Str::UpdErrSignature); return erg;
    case Echtheit::SignaturFalsch: erg.problem = meldung(Str::UpdErrSignature); return erg;
    }

    Pk3 archiv;
    std::string fehler;
    if (!readPk3Directory(schmal(zipPfad), archiv, &fehler)) {
        erg.problem = meldung(Str::UpdErrZip, fehler);
        return erg;
    }
    std::vector<std::string> namen;
    for (const Pk3Entry& e : archiv.entries) { namen.push_back(e.name); }
    const std::string ober = gemeinsamerOrdner(namen);
    // rc601 release: WAS mit einem Eintrag geschieht, entscheidet einspielenAls.
    // Ohne data\base neben der .exe laeuft das Programm eigenstaendig.
    const bool nurProgramm = !fs::exists(ordner / L"data" / L"base" / L"behaved.bhc", ec);
    const std::string laufenderName = schmal(exe.filename().wstring());

    // 2. Entpacken in den Zwischenordner: neu/<rel> fuer Dateien, das Programm als
    //    neu.exe, die alten Dateien spaeter nach alt/<rel>.
    const fs::path zwischen = ordner / kZwischenOrdner;
    fs::remove_all(zwischen, ec);
    struct Tausch {
        fs::path neu, ziel, alt;
        std::string rel;
        bool programm = false, hatteAlt = false, getauscht = false;
    };
    std::vector<Tausch> liste;
    const auto aufraeumen = [&] { fs::remove_all(zwischen, ec); };
    for (const Pk3Entry& e : archiv.entries) {
        const std::string rel = zielImOrdner(e.name, ober);
        const Einspielen art = einspielenAls(rel, laufenderName, nurProgramm);
        if (art == Einspielen::Ueberspringen) { continue; }
        std::string inhalt;
        if (!readPk3File(archiv, e, inhalt, &fehler)) {
            erg.problem = e.name + ": " + fehler;
            aufraeumen();
            return erg;
        }
        Tausch t;
        t.rel = rel;
        t.programm = art == Einspielen::Programm;
        t.neu = t.programm ? zwischen / L"neu.exe" : zwischen / L"neu" / utf8Pfad(rel);
        t.ziel = t.programm ? exe : ordner / utf8Pfad(rel);
        if (t.programm) {
            t.alt = exe;
            t.alt += L".alt";
        } else {
            t.alt = zwischen / L"alt" / utf8Pfad(rel);
        }
        fs::create_directories(t.neu.parent_path(), ec);
        std::ofstream f(t.neu, std::ios::binary | std::ios::trunc);
        f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
        f.close();
        if (!f) {
            erg.problem = meldung(Str::UpdErrWrite, rel);
            aufraeumen();
            return erg;
        }
        liste.push_back(std::move(t));
    }

    // 3. Tauschen. Die laufende .exe laesst sich nur umbenennen, nicht ueberschreiben.
    for (Tausch& t : liste) {
        t.hatteAlt = fs::exists(t.ziel, ec);
        if (t.hatteAlt) {
            fs::create_directories(t.alt.parent_path(), ec);
            if (t.programm) { DeleteFileW(t.alt.c_str()); }
            if (!MoveFileExW(t.ziel.c_str(), t.alt.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                erg.problem = meldung(Str::UpdErrReplace, t.rel, static_cast<long long>(GetLastError()));
                break;
            }
        } else {
            fs::create_directories(t.ziel.parent_path(), ec);
        }
        if (!MoveFileExW(t.neu.c_str(), t.ziel.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            erg.problem = meldung(Str::UpdErrReplace, t.rel, static_cast<long long>(GetLastError()));
            if (t.hatteAlt) { MoveFileExW(t.alt.c_str(), t.ziel.c_str(), MOVEFILE_REPLACE_EXISTING); }
            break;
        }
        t.getauscht = true;
        if (t.programm) { diag::detail("Update: neues Programm an " + schmal(exe.wstring()) + " (aus " + t.rel + ")"); }
        ++erg.dateien;
    }
    if (!erg.problem.empty()) {
        // Zurueck: in umgekehrter Reihenfolge die alten Dateien an ihren Platz.
        for (auto it = liste.rbegin(); it != liste.rend(); ++it) {
            if (!it->getauscht) { continue; }
            if (it->hatteAlt) {
                MoveFileExW(it->alt.c_str(), it->ziel.c_str(), MOVEFILE_REPLACE_EXISTING);
            } else {
                DeleteFileW(it->ziel.c_str());
            }
        }
        diag::detail("Update: Tausch abgebrochen, " + std::to_string(erg.dateien) + " Dateien zurueckgestellt - " + erg.problem);
        erg.dateien = 0;
    }
    aufraeumen();
    return erg;
}

}  // namespace bhed::update
