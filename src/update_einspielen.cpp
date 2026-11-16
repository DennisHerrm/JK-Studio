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
//
// REL-TEMPO: der Tausch steht als intern::tauschen fuer einspielen und das Delta
// (src/update_delta.cpp) bereit; die Meldungen kommen ueber meldungText (das Programm setzt die
// Uebersetzungen, der Starter seine kleine Tabelle); die Summe des .zip wird in Stuecken
// gerechnet (das Paket von rc612 hat 470 MB - bisher lag es dafuer ganz im Speicher).
#include "bhed/update.h"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "bhed/diag.h"
#include "bhed/pk3.h"
#include "update_intern.h"

namespace bhed::update {
namespace {

namespace fs = std::filesystem;
using intern::klein;
using intern::schmal;
using intern::utf8Pfad;

// Im Zwischenordner: die Ziele, die es vor dem Tausch nicht gab (eines je Zeile).
constexpr const wchar_t* kNeuAngelegt = L"neu_angelegt.txt";
// REL-TEMPO: im Zwischenordner, sobald alles getauscht ist (fuer resteAufraeumen).
constexpr const wchar_t* kFertig = L"fertig";

std::mutex g_hakenSperre;
std::function<void(TestPhase, const std::string&)> g_haken;

// REL-TEMPO: die Meldungen. Ohne Tabelle Englisch - wortgleich mit den englischen Texten von
// Str::UpdErrZip ... (tools/gen_i18n.py), die das Programm bis rc612 hier zeigte.
std::atomic<Meldungstexte> g_texte{nullptr};

const char* englisch(Meldung m) {
    switch (m) {
    case Meldung::ZipUnlesbar: return "The downloaded .zip cannot be read: %s";
    case Meldung::OhneSummen: return "The release has no SHA256SUMS.txt%s - the download cannot be checked, nothing was changed";
    case Meldung::NichtGelistet: return "%s is not listed in SHA256SUMS.txt - nothing was changed";
    case Meldung::Pruefsumme:
        return "The checksum of %s does not match SHA256SUMS.txt - the download is damaged or was altered, nothing was changed";
    case Meldung::Signatur: return "The signature of SHA256SUMS.txt is missing or invalid%s - nothing was changed";
    case Meldung::Abbruch: return "Download interrupted%s";
    case Meldung::Schreiben: return "%s cannot be written";
    case Meldung::Ersetzen: return "%s cannot be replaced (error %lld)";
    }
    return "%s";
}

// SHA-256 (32 Byte roh) mit BCrypt, in Stuecken aus `lies` (liefert false am Ende); leer bei einem Fehler.
template <class Lies>
std::string sha256Stuecke(Lies lies) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) { return {}; }
    std::string roh(32, '\0');
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = BCRYPT_SUCCESS(BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0));
    const char* daten = nullptr;
    std::size_t n = 0;
    while (ok && lies(daten, n)) {
        ok = BCRYPT_SUCCESS(BCryptHashData(h, reinterpret_cast<PUCHAR>(const_cast<char*>(daten)), static_cast<ULONG>(n), 0));
    }
    ok = ok && BCRYPT_SUCCESS(BCryptFinishHash(h, reinterpret_cast<PUCHAR>(roh.data()), 32, 0));
    if (h != nullptr) { BCryptDestroyHash(h); }
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok ? roh : std::string();
}

// SHA-256 (32 Byte roh) mit BCrypt; leer bei einem Fehler.
std::string sha256Roh(const std::string& daten) {
    // In Stuecken: ULONG reicht fuer ein .zip ueber 4 GB nicht.
    std::size_t pos = 0;
    return sha256Stuecke([&](const char*& p, std::size_t& n) {
        if (pos >= daten.size()) { return false; }
        n = std::min<std::size_t>(daten.size() - pos, 1u << 30);
        p = daten.data() + pos;
        pos += n;
        return true;
    });
}

std::string alsHex(const std::string& roh) {
    std::string hex;
    static const char* const ziffern = "0123456789abcdef";
    for (const char c : roh) {
        const auto b = static_cast<unsigned char>(c);
        hex.push_back(ziffern[b >> 4]);
        hex.push_back(ziffern[b & 15]);
    }
    return hex;
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

}  // namespace

// --- intern (update_intern.h) ---------------------------------------------------
namespace intern {

std::function<void(TestPhase, const std::string&)> haken() {
    std::lock_guard<std::mutex> l(g_hakenSperre);
    return g_haken;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
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

Tausch tauschFuer(const std::string& rel, bool programm, bool weg, const fs::path& zwischen, const fs::path& exe) {
    Tausch t;
    t.rel = rel;
    t.programm = programm;
    t.weg = weg;
    if (!weg) { t.neu = programm ? zwischen / L"neu.exe" : zwischen / L"neu" / utf8Pfad(rel); }
    t.ziel = programm ? exe : exe.parent_path() / utf8Pfad(rel);
    if (programm) {
        t.alt = exe;
        t.alt += L".alt";
    } else {
        t.alt = zwischen / L"alt" / utf8Pfad(rel);
    }
    return t;
}

void tauschen(std::vector<Tausch>& liste, const fs::path& zwischen, const fs::path& exe, Einspielergebnis& erg) {
    const auto testHaken = haken();
    std::error_code ec;
    // rc610 F78: das Programm zuletzt (auch wenn das .zip es frueher nennt). Dann
    // heisst "<exe>.alt ist da": alles ist getauscht - darauf verlaesst sich
    // resteAufraeumen beim naechsten Start.
    std::stable_partition(liste.begin(), liste.end(), [](const Tausch& t) { return !t.programm; });
    // Was es noch nicht gibt, steht in neu_angelegt.txt: bricht der Tausch ohne
    // Rueckstellen ab (das Programm endet), entfernt resteAufraeumen es wieder.
    {
        std::ofstream liste_(zwischen / kNeuAngelegt, std::ios::binary | std::ios::trunc);
        for (const Tausch& t : liste) {
            if (!t.programm && !t.weg && !fs::exists(t.ziel, ec)) { liste_ << t.rel << '\n'; }
        }
    }

    // 3. Tauschen. Die laufende .exe laesst sich nur umbenennen, nicht ueberschreiben.
    bool zurueckGescheitert = false;
    for (Tausch& t : liste) {
        t.hatteAlt = fs::exists(t.ziel, ec);
        if (t.weg && !t.hatteAlt) { continue; }   // REL-TEMPO: schon weg
        if (t.hatteAlt) {
            fs::create_directories(t.alt.parent_path(), ec);
            if (t.programm) { DeleteFileW(t.alt.c_str()); }
            if (!MoveFileExW(t.ziel.c_str(), t.alt.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                erg.problem = meldungText(Meldung::Ersetzen, t.rel, static_cast<long long>(GetLastError()));
                break;
            }
        } else {
            fs::create_directories(t.ziel.parent_path(), ec);
        }
        if (!t.weg && !MoveFileExW(t.neu.c_str(), t.ziel.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            erg.problem = meldungText(Meldung::Ersetzen, t.rel, static_cast<long long>(GetLastError()));
            if (t.hatteAlt && !MoveFileExW(t.alt.c_str(), t.ziel.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                diag::detail("Update: " + t.rel + " nicht zurueckgestellt (Fehler " + std::to_string(GetLastError()) + ")");
                zurueckGescheitert = true;
            }
            break;
        }
        t.getauscht = true;
        if (testHaken) { testHaken(TestPhase::Getauscht, t.rel); }
        if (t.programm) { diag::detail("Update: neues Programm an " + schmal(exe.wstring()) + " (aus " + t.rel + ")"); }
        if (t.weg) {
            ++erg.geloescht;
        } else {
            ++erg.dateien;
        }
    }
    if (!erg.problem.empty()) {
        // Zurueck: in umgekehrter Reihenfolge die alten Dateien an ihren Platz.
        // rc610 F78 (Fund B8-5): jeder Schritt geprueft. Was sich nicht
        // zurueckstellen laesst (meist dieselbe Sperre, an der der Tausch
        // scheiterte), bleibt als Sicherung im Zwischenordner liegen; der naechste
        // Start stellt es zurueck (resteAufraeumen). Bis rc609 loeschte aufraeumen
        // sie in jedem Fall.
        int zurueck = 0;
        for (auto it = liste.rbegin(); it != liste.rend(); ++it) {
            if (!it->getauscht) { continue; }
            const bool ok = it->hatteAlt ? MoveFileExW(it->alt.c_str(), it->ziel.c_str(), MOVEFILE_REPLACE_EXISTING) != 0
                                         : DeleteFileW(it->ziel.c_str()) != 0;
            if (ok) {
                ++zurueck;
            } else {
                diag::detail("Update: " + it->rel + " nicht zurueckgestellt (Fehler " + std::to_string(GetLastError()) +
                             "), die Sicherung bleibt in " + kZwischenOrdner);
                zurueckGescheitert = true;
            }
        }
        diag::detail("Update: Tausch abgebrochen, " + std::to_string(zurueck) + " von " +
                     std::to_string(erg.dateien + erg.geloescht) + " Dateien zurueckgestellt - " + erg.problem);
        erg.dateien = 0;
        erg.geloescht = 0;
        erg.sicherungBleibt = zurueckGescheitert;
    } else {
        // REL-TEMPO: alles getauscht - auch ohne "<exe>.alt" (Delta ohne neues Programm, erster
        // Start des Starters) weiss resteAufraeumen das, falls das Wegraeumen nicht mehr gelingt.
        std::ofstream(zwischen / kFertig, std::ios::binary | std::ios::trunc) << "ok\n";
    }
    if (!zurueckGescheitert) { fs::remove_all(zwischen, ec); }
}

}  // namespace intern

void meldungstexteSetzen(Meldungstexte texte) { g_texte = texte; }

std::string meldungText(Meldung m, const std::string& name, long long zahl) {
    const Meldungstexte t = g_texte.load();
    const char* format = t != nullptr ? t(m) : nullptr;
    if (format == nullptr) { format = englisch(m); }
    char z[512];
    std::snprintf(z, sizeof(z), format, name.c_str(), zahl);
    return z;
}

const Asset* assetNamens(const Release& r, const char* name) {
    const std::string gesucht = klein(name);
    for (const Asset& a : r.assets) {
        if (klein(a.name) == gesucht) { return &a; }
    }
    return nullptr;
}

std::string sha256Hex(const std::string& daten) { return alsHex(sha256Roh(daten)); }

std::string sha256Datei(const std::wstring& pfad) {
    HANDLE f = CreateFileW(pfad.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (f == INVALID_HANDLE_VALUE) { return {}; }
    std::string puffer(1u << 20, '\0');
    bool fehler = false;
    const std::string roh = sha256Stuecke([&](const char*& p, std::size_t& n) {
        DWORD gelesen = 0;
        if (!ReadFile(f, puffer.data(), static_cast<DWORD>(puffer.size()), &gelesen, nullptr)) {
            fehler = true;
            return false;
        }
        p = puffer.data();
        n = gelesen;
        return gelesen > 0;
    });
    CloseHandle(f);
    return fehler ? std::string() : alsHex(roh);
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

Echtheit pruefeSumme(const std::string& name, const std::string& istHex, const std::string* summen, const std::string* signatur,
                     const std::string& schluesselHex) {
    if (summen == nullptr) { return Echtheit::OhneSummen; }
    // Erst die Signatur ueber die Summen-Datei: ohne sie waeren die Summen
    // nur so echt wie der Download selbst.
    if (!schluesselHex.empty()) {
        if (signatur == nullptr) { return Echtheit::OhneSignatur; }
        if (!signaturGueltig(*summen, *signatur, schluesselHex)) { return Echtheit::SignaturFalsch; }
    }
    const std::string soll = summeFuer(*summen, name);
    if (soll.empty()) { return Echtheit::NichtGelistet; }
    return (!istHex.empty() && istHex == soll) ? Echtheit::Ok : Echtheit::SummeFalsch;
}

Echtheit pruefeEchtheit(const std::string& zipName, const std::string& zip, const std::string* summen,
                        const std::string* signatur, const std::string& schluesselHex) {
    return pruefeSumme(zipName, sha256Hex(zip), summen, signatur, schluesselHex);
}

void testHakenSetzen(std::function<void(TestPhase, const std::string& rel)> h) {
    std::lock_guard<std::mutex> l(g_hakenSperre);
    g_haken = std::move(h);
}

Einspielergebnis einspielen(const std::wstring& zipPfad, const std::string& zipName, const std::string* summen,
                            const std::string* signatur, const std::string& schluesselHex, const std::wstring& exeW,
                            const std::atomic<bool>* abbrechen, Umfang umfang) {
    Einspielergebnis erg;
    const auto testHaken = intern::haken();
    const fs::path exe(exeW);
    const fs::path ordner = exe.parent_path();
    std::error_code ec;

    // 1. Pruefen - die Bytes, die gleich der pk3-Leser oeffnet (REL-TEMPO: in Stuecken gelesen).
    if (!fs::exists(fs::path(zipPfad), ec)) {
        erg.problem = meldungText(Meldung::ZipUnlesbar, schmal(zipPfad));
        return erg;
    }
    const std::string ist = sha256Datei(zipPfad);
    if (ist.empty()) {
        erg.problem = meldungText(Meldung::ZipUnlesbar, schmal(zipPfad));
        return erg;
    }
    erg.echtheit = pruefeSumme(zipName, ist, summen, signatur, schluesselHex);
    diag::detail("Update: Pruefung von " + zipName + ": " +
                 (erg.echtheit == Echtheit::Ok ? std::string("echt (SHA-256") + (schluesselHex.empty() ? ")" : " und Signatur)")
                                               : "ABGELEHNT (" + std::to_string(static_cast<int>(erg.echtheit)) + ")"));
    switch (erg.echtheit) {
    case Echtheit::Ok: break;
    case Echtheit::OhneSummen: erg.problem = meldungText(Meldung::OhneSummen); return erg;
    case Echtheit::NichtGelistet: erg.problem = meldungText(Meldung::NichtGelistet, zipName); return erg;
    case Echtheit::SummeFalsch: erg.problem = meldungText(Meldung::Pruefsumme, zipName); return erg;
    case Echtheit::OhneSignatur: erg.problem = meldungText(Meldung::Signatur); return erg;
    case Echtheit::SignaturFalsch: erg.problem = meldungText(Meldung::Signatur); return erg;
    }

    Pk3 archiv;
    std::string fehler;
    if (!readPk3Directory(schmal(zipPfad), archiv, &fehler)) {
        erg.problem = meldungText(Meldung::ZipUnlesbar, fehler);
        return erg;
    }
    std::vector<std::string> namen;
    for (const Pk3Entry& e : archiv.entries) { namen.push_back(e.name); }
    const std::string ober = gemeinsamerOrdner(namen);
    // rc601 release: WAS mit einem Eintrag geschieht, entscheidet einspielenAls.
    // Ohne data\base neben der .exe laeuft das Programm eigenstaendig.
    // REL-TEMPO: der Starter (Umfang::Alles) spielt immer alles ein, auch in einen leeren Ordner.
    const bool nurProgramm = umfang == Umfang::WieDaneben && !fs::exists(ordner / L"data" / L"base" / L"behaved.bhc", ec);
    const std::string laufenderName = schmal(exe.filename().wstring());

    // 2. Entpacken in den Zwischenordner: neu/<rel> fuer Dateien, das Programm als
    //    neu.exe, die alten Dateien spaeter nach alt/<rel>.
    const fs::path zwischen = ordner / kZwischenOrdner;
    fs::remove_all(zwischen, ec);
    std::vector<intern::Tausch> liste;
    const auto aufraeumen = [&] { fs::remove_all(zwischen, ec); };
    for (const Pk3Entry& e : archiv.entries) {
        const std::string rel = zielImOrdner(e.name, ober);
        const Einspielen art = einspielenAls(rel, laufenderName, nurProgramm);
        if (art == Einspielen::Ueberspringen) { continue; }
        // rc610 F78 (Fund B8-4): das Programm wird beendet - bis hierher ist nichts getauscht.
        if (abbrechen != nullptr && abbrechen->load()) {
            erg.problem = meldungText(Meldung::Abbruch);
            aufraeumen();
            return erg;
        }
        std::string inhalt;
        if (!readPk3File(archiv, e, inhalt, &fehler)) {
            erg.problem = e.name + ": " + fehler;
            aufraeumen();
            return erg;
        }
        intern::Tausch t = intern::tauschFuer(rel, art == Einspielen::Programm, false, zwischen, exe);
        fs::create_directories(t.neu.parent_path(), ec);
        std::ofstream f(t.neu, std::ios::binary | std::ios::trunc);
        f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
        f.close();
        if (!f) {
            erg.problem = meldungText(Meldung::Schreiben, rel);
            aufraeumen();
            return erg;
        }
        if (testHaken) { testHaken(TestPhase::Entpackt, t.rel); }
        liste.push_back(std::move(t));
    }

    // 3. Tauschen (intern::tauschen: Programm zuletzt, Rueckstellen, Zwischenordner weg).
    intern::tauschen(liste, zwischen, exe, erg);
    return erg;
}

int resteAufraeumen(const std::wstring& exeW) {
    const fs::path exe(exeW);
    const fs::path zwischen = exe.parent_path() / kZwischenOrdner;
    std::error_code ec;
    if (!fs::exists(zwischen, ec)) { return 0; }
    fs::path exeAlt = exe;
    exeAlt += L".alt";
    int zurueck = 0;
    bool alles = true;
    // "<exe>.alt" da: das Programm (zuletzt) ist getauscht, also alles - die
    // Reste sind nur noch wegzuraeumen. Sonst ist der Tausch nicht fertig
    // geworden: die Sicherungen zurueck, die neu angelegten Dateien weg.
    // REL-TEMPO: ebenso mit der Marke "fertig" (tauschen legt sie nach dem letzten Tausch an).
    if (!fs::exists(exeAlt, ec) && !fs::exists(zwischen / kFertig, ec)) {
        const fs::path alt = zwischen / L"alt";
        for (fs::recursive_directory_iterator it(alt, ec), ende; !ec && it != ende; it.increment(ec)) {
            if (!it->is_regular_file(ec)) { continue; }
            const fs::path ziel = exe.parent_path() / fs::relative(it->path(), alt, ec);
            fs::create_directories(ziel.parent_path(), ec);
            if (MoveFileExW(it->path().c_str(), ziel.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                ++zurueck;
            } else {
                alles = false;
            }
        }
        std::ifstream neu(zwischen / kNeuAngelegt, std::ios::binary);
        std::string rel;
        while (std::getline(neu, rel)) {
            if (!rel.empty()) { fs::remove(exe.parent_path() / utf8Pfad(rel), ec); }
        }
    }
    diag::detail("Update: Reste eines unterbrochenen Updates - " + std::to_string(zurueck) + " Dateien zurueckgestellt" +
                 (alles ? "" : ", nicht alle (die Sicherung bleibt fuer den naechsten Start)"));
    if (alles) { fs::remove_all(zwischen, ec); }
    return zurueck;
}

}  // namespace bhed::update
