// update_delta.cpp - REL-TEMPO (15.11.): Manifest, Delta-Pakete und der ganze Ablauf (siehe update.h)
//
// shank 14.11. (Idee "nur der neue Code"): ein Update soll nicht jedes Mal das ganze Paket laden.
// rc611 -> rc612 waren von 5980 Dateien (587 MB, als .zip 470 MB) 4183 gleich geblieben; das
// Delta traegt nur die Inhalte, die die alte Fassung nicht hat (dort etwa 161 MB gepackt).
//
//   Manifest   jede Datei des vollen Pakets mit SHA-256 und Groesse (tools/release_paket.py manifest)
//   Delta      ein Zip mit delta.txt (von, nach, weg) und inhalt/<sha256> fuer jeden neuen Inhalt
//              (tools/release_paket.py delta) - gleiche Inhalte nur einmal (jkstudio.exe = behaved.exe)
//
// Beide stehen in SHA256SUMS.txt (Signatur wie beim .zip). Eingespielt wird ueber denselben
// Zwischenordner wie das volle Paket (intern::tauschen), und jeder Inhalt wird vor dem Tausch
// gegen das Manifest geprueft - auch der, der aus einer gleichen Datei im Ordner kommt.
#include "bhed/update.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "bhed/diag.h"
#include "bhed/pk3.h"
#include "update_intern.h"

namespace bhed::update {
namespace {

namespace fs = std::filesystem;
using intern::schmal;
using intern::utf8Pfad;

// Die Zeilen eines Textes ohne CR am Ende.
std::vector<std::string> zeilenVon(const std::string& text) {
    std::vector<std::string> z;
    std::size_t a = 0;
    while (a < text.size()) {
        std::size_t b = text.find('\n', a);
        if (b == std::string::npos) { b = text.size(); }
        std::string s = text.substr(a, b - a);
        if (!s.empty() && s.back() == '\r') { s.pop_back(); }
        z.push_back(std::move(s));
        a = b + 1;
    }
    return z;
}

bool istHex64(const std::string& s) {
    if (s.size() != 64) { return false; }
    for (const char c : s) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) { return false; }
    }
    return true;
}

bool istFassung(const std::string& s) { return rcVon(s) == s && !s.empty(); }

void setzeFehler(std::string* fehler, const std::string& text) {
    if (fehler != nullptr) { *fehler = text; }
}

}  // namespace

std::string rcVon(const std::string& tag) {
    const int n = rcNummer(tag);
    return n < 0 ? std::string() : "rc" + std::to_string(n);
}

std::string zipName(const std::string& rc) { return std::string(kPaketName) + "-" + rc + "-windows-x64.zip"; }

std::string manifestName(const std::string& rc) { return std::string(kPaketName) + "-" + rc + "-manifest.txt"; }

std::string deltaName(const std::string& neu, const std::string& alt) {
    return std::string(kPaketName) + "-" + neu + "-from-" + alt + ".delta";
}

bool parseManifest(const std::string& text, Manifest& out, std::string* fehler) {
    out = Manifest{};
    const std::vector<std::string> z = zeilenVon(text);
    if (z.empty() || z[0] != "# JK Studio manifest 1") {
        setzeFehler(fehler, "Manifest: unbekannter Kopf");
        return false;
    }
    std::set<std::string> gesehen;
    for (std::size_t i = 1; i < z.size(); ++i) {
        const std::string& s = z[i];
        if (s.empty() || s[0] == '#') { continue; }
        if (s.rfind("fassung ", 0) == 0) {
            out.fassung = s.substr(8);
            continue;
        }
        // "<64 hex> <groesse> <pfad>" - der Pfad darf Leerzeichen enthalten
        const std::size_t a = s.find(' ');
        const std::size_t b = a == std::string::npos ? std::string::npos : s.find(' ', a + 1);
        if (a != 64 || b == std::string::npos || b == a + 1 || b + 1 >= s.size()) {
            setzeFehler(fehler, "Manifest: Zeile " + std::to_string(i + 1) + " kaputt");
            return false;
        }
        ManifestDatei d;
        d.sha256 = s.substr(0, 64);
        const std::string g = s.substr(a + 1, b - a - 1);
        d.pfad = s.substr(b + 1);
        if (!istHex64(d.sha256) || g.find_first_not_of("0123456789") != std::string::npos || g.size() > 15) {
            setzeFehler(fehler, "Manifest: Zeile " + std::to_string(i + 1) + " kaputt");
            return false;
        }
        d.groesse = std::strtoll(g.c_str(), nullptr, 10);
        // Nur Pfade, die zielImOrdner unveraendert durchlaesst (kein "..", nicht absolut, kein "\").
        if (d.pfad.find('\\') != std::string::npos || zielImOrdner(d.pfad, "") != d.pfad) {
            setzeFehler(fehler, "Manifest: unsicherer Pfad in Zeile " + std::to_string(i + 1));
            return false;
        }
        if (!gesehen.insert(intern::klein(d.pfad)).second) {
            setzeFehler(fehler, "Manifest: " + d.pfad + " doppelt");
            return false;
        }
        out.dateien.push_back(std::move(d));
    }
    if (!istFassung(out.fassung) || out.dateien.empty()) {
        setzeFehler(fehler, "Manifest: ohne Fassung oder ohne Dateien");
        return false;
    }
    return true;
}

bool parseDeltaKopf(const std::string& text, DeltaKopf& out, std::string* fehler) {
    out = DeltaKopf{};
    const std::vector<std::string> z = zeilenVon(text);
    if (z.empty() || z[0] != "# JK Studio delta 1") {
        setzeFehler(fehler, "Delta: unbekannter Kopf");
        return false;
    }
    for (std::size_t i = 1; i < z.size(); ++i) {
        const std::string& s = z[i];
        if (s.empty() || s[0] == '#') { continue; }
        if (s.rfind("von ", 0) == 0) {
            out.von = s.substr(4);
        } else if (s.rfind("nach ", 0) == 0) {
            out.nach = s.substr(5);
        } else if (s.rfind("weg ", 0) == 0) {
            const std::string p = s.substr(4);
            if (p.find('\\') != std::string::npos || zielImOrdner(p, "") != p) {
                setzeFehler(fehler, "Delta: unsicherer Pfad in Zeile " + std::to_string(i + 1));
                return false;
            }
            out.weg.push_back(p);
        } else {
            setzeFehler(fehler, "Delta: Zeile " + std::to_string(i + 1) + " unbekannt");
            return false;
        }
    }
    if (!istFassung(out.von) || !istFassung(out.nach)) {
        setzeFehler(fehler, "Delta: ohne von/nach");
        return false;
    }
    return true;
}

Einspielergebnis einspielenDelta(const std::wstring& deltaPfad, const std::string& deltaDatei, const std::wstring& manifestPfad,
                                 const std::string& manifestDatei, const std::string* summen, const std::string* signatur,
                                 const std::string& schluesselHex, const std::wstring& exeW, const std::string& vonFassung,
                                 const std::string& nachFassung, const std::atomic<bool>* abbrechen, Umfang umfang) {
    Einspielergebnis erg;
    const auto testHaken = intern::haken();
    const fs::path exe(exeW);
    const fs::path ordner = exe.parent_path();
    std::error_code ec;
    const auto abgebrochen = [&] { return abbrechen != nullptr && abbrechen->load(); };

    // 1. Pruefen: Manifest und Delta gegen SHA256SUMS.txt (und die Signatur ueber sie).
    for (const auto& [pfad, name] : {std::pair<std::wstring, std::string>{manifestPfad, manifestDatei}, {deltaPfad, deltaDatei}}) {
        const std::string ist = sha256Datei(pfad);
        erg.echtheit = pruefeSumme(name, ist, summen, signatur, schluesselHex);
        if (ist.empty()) {
            erg.problem = meldungText(Meldung::ZipUnlesbar, name);
            return erg;
        }
        switch (erg.echtheit) {
        case Echtheit::Ok: break;
        case Echtheit::OhneSummen: erg.problem = meldungText(Meldung::OhneSummen); return erg;
        case Echtheit::NichtGelistet: erg.problem = meldungText(Meldung::NichtGelistet, name); return erg;
        case Echtheit::SummeFalsch: erg.problem = meldungText(Meldung::Pruefsumme, name); return erg;
        case Echtheit::OhneSignatur: erg.problem = meldungText(Meldung::Signatur); return erg;
        case Echtheit::SignaturFalsch: erg.problem = meldungText(Meldung::Signatur); return erg;
        }
    }
    diag::detail("Update: " + manifestDatei + " und " + deltaDatei + " echt (SHA-256" +
                 (schluesselHex.empty() ? ")" : " und Signatur)"));

    std::string text;
    Manifest m;
    std::string fehler;
    if (!intern::liesDatei(fs::path(manifestPfad), text) || !parseManifest(text, m, &fehler) || m.fassung != nachFassung) {
        erg.problem = "Delta: " + manifestDatei + " passt nicht (" + (fehler.empty() ? "Fassung " + m.fassung : fehler) + ")";
        return erg;
    }
    Pk3 archiv;
    if (!readPk3Directory(schmal(deltaPfad), archiv, &fehler)) {
        erg.problem = meldungText(Meldung::ZipUnlesbar, fehler);
        return erg;
    }
    std::map<std::string, const Pk3Entry*> inhalte;   // sha256 -> Eintrag
    const Pk3Entry* kopfEintrag = nullptr;
    const std::string praefix = kDeltaInhalt;
    for (const Pk3Entry& e : archiv.entries) {
        if (e.name == kDeltaKopf) {
            kopfEintrag = &e;
        } else if (e.name.rfind(praefix, 0) == 0 && istHex64(e.name.substr(praefix.size()))) {
            inhalte[e.name.substr(praefix.size())] = &e;
        }
    }
    DeltaKopf kopf;
    if (kopfEintrag == nullptr || !readPk3File(archiv, *kopfEintrag, text, &fehler) || !parseDeltaKopf(text, kopf, &fehler) ||
        kopf.von != vonFassung || kopf.nach != nachFassung) {
        erg.problem = "Delta: " + deltaDatei + " passt nicht zu " + vonFassung + " -> " + nachFassung +
                      (fehler.empty() ? "" : " (" + fehler + ")");
        return erg;
    }

    // 2. Was sich aendert. Wie beim vollen Paket: ohne data\base nur das Programm.
    const bool nurProgramm = umfang == Umfang::WieDaneben && !fs::exists(ordner / L"data" / L"base" / L"behaved.bhc", ec);
    const std::string laufenderName = schmal(exe.filename().wstring());
    const fs::path zwischen = ordner / kZwischenOrdner;
    struct Bedarf {
        const ManifestDatei* d;
        bool programm;
    };
    std::vector<Bedarf> bedarf;
    std::map<std::string, fs::path> vorhanden;   // sha256 -> eine Datei im Ordner mit genau diesem Inhalt
    int gleich = 0;
    for (const ManifestDatei& d : m.dateien) {
        if (abgebrochen()) {
            erg.problem = meldungText(Meldung::Abbruch);
            return erg;
        }
        const Einspielen art = einspielenAls(d.pfad, laufenderName, nurProgramm);
        if (art == Einspielen::Ueberspringen) { continue; }
        const bool programm = art == Einspielen::Programm;
        const fs::path ziel = programm ? exe : ordner / utf8Pfad(d.pfad);
        const std::string ist = fs::is_regular_file(ziel, ec) ? sha256Datei(ziel.wstring()) : std::string();
        if (!ist.empty()) { vorhanden.emplace(ist, ziel); }
        if (ist == d.sha256) {
            ++gleich;
            continue;
        }
        bedarf.push_back({&d, programm});
    }
    std::vector<intern::Tausch> liste;
    for (const std::string& p : kopf.weg) {
        const Einspielen art = einspielenAls(p, laufenderName, nurProgramm);
        // Nie das laufende Programm; und eine Datei, die das neue Manifest wieder nennt, bleibt.
        if (art != Einspielen::Datei) { continue; }
        bool bleibt = false;
        for (const ManifestDatei& d : m.dateien) { bleibt = bleibt || intern::klein(d.pfad) == intern::klein(p); }
        const fs::path ziel = ordner / utf8Pfad(p);
        if (bleibt || !fs::is_regular_file(ziel, ec)) { continue; }
        const std::string ist = sha256Datei(ziel.wstring());
        if (!ist.empty()) { vorhanden.emplace(ist, ziel); }
        liste.push_back(intern::tauschFuer(p, false, true, zwischen, exe));
    }

    // 3. Die neuen Inhalte in den Zwischenordner, jeder gegen das Manifest geprueft.
    fs::remove_all(zwischen, ec);
    const auto aufraeumen = [&] { fs::remove_all(zwischen, ec); };
    for (const Bedarf& b : bedarf) {
        if (abgebrochen()) {
            erg.problem = meldungText(Meldung::Abbruch);
            aufraeumen();
            return erg;
        }
        intern::Tausch t = intern::tauschFuer(b.d->pfad, b.programm, false, zwischen, exe);
        fs::create_directories(t.neu.parent_path(), ec);
        bool da = false;
        if (const auto it = inhalte.find(b.d->sha256); it != inhalte.end()) {
            std::string inhalt;
            if (!readPk3File(archiv, *it->second, inhalt, &fehler)) {
                erg.problem = "Delta: " + it->second->name + ": " + fehler;
                aufraeumen();
                return erg;
            }
            std::ofstream f(t.neu, std::ios::binary | std::ios::trunc);
            f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
            f.close();
            if (!f) {
                erg.problem = meldungText(Meldung::Schreiben, b.d->pfad);
                aufraeumen();
                return erg;
            }
            da = true;
        } else if (const auto v = vorhanden.find(b.d->sha256); v != vorhanden.end()) {
            // Derselbe Inhalt liegt schon im Ordner (verschoben, oder jkstudio.exe = behaved.exe).
            da = CopyFileW(v->second.c_str(), t.neu.c_str(), FALSE) != 0;
        }
        if (!da) {
            erg.problem = "Delta: kein Inhalt fuer " + b.d->pfad + " (die Datei im Ordner weicht von " + vonFassung + " ab)";
            aufraeumen();
            return erg;
        }
        if (sha256Datei(t.neu.wstring()) != b.d->sha256 ||
            static_cast<long long>(fs::file_size(t.neu, ec)) != b.d->groesse) {
            erg.problem = meldungText(Meldung::Pruefsumme, b.d->pfad);
            aufraeumen();
            return erg;
        }
        if (testHaken) { testHaken(TestPhase::Entpackt, t.rel); }
        liste.push_back(std::move(t));
    }
    diag::detail("Update: Delta " + vonFassung + " -> " + nachFassung + ": " + std::to_string(gleich) + " Dateien gleich, " +
                 std::to_string(bedarf.size()) + " neu oder geaendert, " +
                 std::to_string(liste.size() - bedarf.size()) + " fallen weg");
    if (liste.empty()) { return erg; }   // schon alles wie im Manifest

    // 4. Tauschen wie beim vollen Paket.
    fs::create_directories(zwischen, ec);
    intern::tauschen(liste, zwischen, exe, erg);
    return erg;
}

Ablaufergebnis aktualisieren(const Release& r, const std::string& lokaleFassung, const std::wstring& exe, Quelle& quelle,
                             const std::wstring& ablageW, const std::string& schluesselHex, const std::atomic<bool>* abbrechen,
                             const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt, Umfang umfang) {
    Ablaufergebnis a;
    const fs::path ablage(ablageW);
    std::error_code ec;
    fs::create_directories(ablage, ec);
    const auto abgebrochen = [&] { return abbrechen != nullptr && abbrechen->load(); };
    const std::string neu = rcVon(r.tag);
    // Eine Datei des Releases laden; mit `gross` zaehlt der Fortschritt (sonst still).
    const auto laden = [&](const Asset& as, fs::path& ziel, bool gross, std::string& fehler) {
        ziel = ablage / utf8Pfad(as.name);
        const auto f = [&](std::uint64_t n, std::uint64_t gesamt) {
            if (gross && fortschritt) {
                fortschritt(n, gesamt > 0 ? gesamt : static_cast<std::uint64_t>(std::max<long long>(as.size, 0)));
            }
        };
        return quelle.laden(as, ziel.wstring(), f, fehler);
    };
    std::string summen;
    std::string signatur;
    bool mitSummen = false;
    bool mitSignatur = false;
    bool kleineGeladen = false;
    // Stufe 8d: SHA256SUMS.txt (Pflicht) und SHA256SUMS.txt.sig (Pflicht, sobald ein Schluessel im
    // Programm steht) - klein, ohne Fortschritt, erst nach der grossen Datei.
    const auto kleine = [&] {
        if (kleineGeladen) { return; }
        kleineGeladen = true;
        for (const auto& [name, inhalt, mit] : {std::tuple<const char*, std::string*, bool*>{kSummenDatei, &summen, &mitSummen},
                                                 {kSignaturDatei, &signatur, &mitSignatur}}) {
            const Asset* d = assetNamens(r, name);
            fs::path p;
            std::string fehler;
            if (d != nullptr && laden(*d, p, false, fehler)) { *mit = intern::liesDatei(p, *inhalt); }
        }
        diag::detail(std::string("Update: ") + kSummenDatei + (mitSummen ? " geladen" : " FEHLT") + ", " + kSignaturDatei +
                     (mitSignatur ? " geladen" : " fehlt"));
    };

    // 1. Das Delta fuer diese Fassung, wenn es eins gibt (und ein Manifest).
    const Asset* delta = (neu.empty() || lokaleFassung.empty()) ? nullptr : assetNamens(r, deltaName(neu, lokaleFassung).c_str());
    const Asset* manifest = neu.empty() ? nullptr : assetNamens(r, manifestName(neu).c_str());
    if (delta != nullptr && manifest != nullptr) {
        fs::path dPfad;
        fs::path mPfad;
        std::string fehler;
        if (!laden(*delta, dPfad, true, fehler)) {
            a.deltaGrund = delta->name + ": " + fehler;
        } else if (abgebrochen()) {
            a.ladeFehler = meldungText(Meldung::Abbruch);
            return a;
        } else if (!laden(*manifest, mPfad, false, fehler)) {
            a.deltaGrund = manifest->name + ": " + fehler;
        } else {
            kleine();
            if (abgebrochen()) {
                a.ladeFehler = meldungText(Meldung::Abbruch);
                return a;
            }
            a.einspielen = einspielenDelta(dPfad.wstring(), delta->name, mPfad.wstring(), manifest->name, mitSummen ? &summen : nullptr,
                                           mitSignatur ? &signatur : nullptr, schluesselHex, exe, lokaleFassung, neu, abbrechen, umfang);
            if (a.einspielen.problem.empty()) {
                a.mitDelta = true;
                return a;
            }
            a.deltaGrund = a.einspielen.problem;
            // Geaendert ist nichts - ausser die Sicherung blieb (dann nicht noch ein Paket darueber).
            if (a.einspielen.sicherungBleibt || a.einspielen.dateien > 0 || abgebrochen()) { return a; }
        }
        fs::remove(dPfad, ec);
        if (abgebrochen()) {
            a.ladeFehler = meldungText(Meldung::Abbruch);
            return a;
        }
        diag::detail("Update: das Delta ging nicht (" + a.deltaGrund + ") - das volle Paket");
        a.einspielen = Einspielergebnis{};
    }

    // 2. Das volle Paket wie bisher.
    const Asset* zip = zipAsset(r);
    if (zip == nullptr) {
        a.ladeFehler = "no .zip";
        return a;
    }
    fs::path zPfad;
    std::string fehler;
    if (!laden(*zip, zPfad, true, fehler)) {
        a.ladeFehler = fehler;
        return a;
    }
    if (fs::file_size(zPfad, ec) < 22) {
        a.ladeFehler = meldungText(Meldung::ZipUnlesbar, zip->name);
        return a;
    }
    if (abgebrochen()) {
        a.ladeFehler = meldungText(Meldung::Abbruch);
        return a;
    }
    kleine();
    if (abgebrochen()) {
        a.ladeFehler = meldungText(Meldung::Abbruch);
        return a;
    }
    a.einspielen = einspielen(zPfad.wstring(), zip->name, mitSummen ? &summen : nullptr, mitSignatur ? &signatur : nullptr,
                              schluesselHex, exe, abbrechen, umfang);
    return a;
}

}  // namespace bhed::update
