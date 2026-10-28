// update.h - Auto-Updater: was ohne Netz und Fenster pruefbar ist
//
// Gewuenscht (shank, 01.10.2026): "eine Auto-Updater-Funktion, dass man in
// Zukunft Aenderungen einfach direkt ueber GitHub runterladen kann - wir
// haben ja das Repo angelegt."
//
// Der Ablauf (gui/update_win32.cpp):
//
//   1. GET api.github.com/repos/<repo>/releases/latest   -> Release (JSON)
//   2. Ist das Release neuer als diese Fassung (kFassung, "rc568")?
//   3. Das .zip des Releases laden, entpacken, die Dateien neben die .exe
//      legen (die laufende .exe wird dabei nur umbenannt), neu starten.
//
// Hier steht der Teil, der sich ohne Netz pruefen laesst
// (tests/updatetest.cpp): JSON lesen, Fassungen vergleichen, Zielpfade
// aus den Archiveintraegen.
#ifndef BHED_UPDATE_H
#define BHED_UPDATE_H

#include <string>
#include <vector>

namespace bhed::update {

// Das Repository, aus dem die Updates kommen - seit rc573 das (jetzt
// oeffentliche) Hauptrepository selbst. Bis rc572 war es ein eigenes
// Download-Repository (DennisHerrm/BehavEd-Remake-Releases, archiviert), weil
// der Quelltext privat war. Stufe 8c: das Projekt heisst JK Studio, das
// Repository DennisHerrm/JK-Studio (umbenannt; GitHub leitet den alten Namen
// DennisHerrm/BehavEd-Remake mit 301 weiter, darum finden auch die alten
// Fassungen mit dem alten Namen das neue Release). Nur HIER aendern.
inline constexpr const char* kRepo = "DennisHerrm/JK-Studio";
// Die Programmdatei: seit Stufe 8c jkstudio.exe. Jedes Paket enthaelt dazu
// behaved.exe (dieselbe Datei): die Updater bis rc605 setzen genau einen
// Eintrag "behaved.exe" ganz oben an die Stelle der laufenden .exe und starten
// ihn neu - so kommen alte Installationen, auch eigenstaendige .exe, zu JK Studio.
inline constexpr const char* kProgramm = "jkstudio.exe";
inline constexpr const char* kAltesProgramm = "behaved.exe";

struct Asset {
    std::string name;          // "BehavEd-Remake-rc568.zip"
    std::string apiUrl;        // api.github.com/.../assets/<id> (mit Anmeldung)
    std::string downloadUrl;   // github.com/.../download/... (oeffentlich)
    long long size = 0;
};

struct Release {
    std::string tag;           // "v1.0.0-rc568"
    std::string name;          // "BehavEd-Remake 1.0.0-rc568"
    std::string body;          // die Notizen (Markdown)
    std::string htmlUrl;       // Seite des Releases
    std::vector<Asset> assets;
};

// Liest die Antwort von /releases/latest. false bei kaputtem JSON oder
// ohne tag_name.
[[nodiscard]] bool parseRelease(const std::string& json, Release& out, std::string* fehler = nullptr);

// Die Nummer hinter "rc": "v1.0.0-rc568" -> 568, "rc568" -> 568, sonst -1.
[[nodiscard]] int rcNummer(const std::string& s);

// Ist `tag` (vom Server) neuer als `lokal` (kFassung)? Ohne erkennbare
// Nummer auf einer Seite: nein - lieber kein Update als ein falsches.
[[nodiscard]] bool istNeuer(const std::string& tag, const std::string& lokal);

// Das erste .zip unter den Dateien des Releases, sonst nullptr.
[[nodiscard]] const Asset* zipAsset(const Release& r);

// Wohin ein Archiveintrag neben der .exe gehoert. Ein gemeinsamer oberster
// Ordner ("BehavEd-Remake/") wird abgestreift - `oberOrdner` ist er, oder
// leer. Leer zurueck heisst: ueberspringen (Ordner selbst, absolute Pfade,
// ".." - ein Archiv soll nichts ausserhalb des Programmordners anfassen).
[[nodiscard]] std::string zielImOrdner(const std::string& eintrag, const std::string& oberOrdner);

// Der gemeinsame oberste Ordner aller Eintraege ("BehavEd-Remake/"), oder
// leer, wenn es keinen gibt.
[[nodiscard]] std::string gemeinsamerOrdner(const std::vector<std::string>& eintraege);

// --- rc601 release: was das Update mit einem Eintrag tut ------------------
//
// `rel` ist zielImOrdner(...) eines Archiveintrags, `laufenderName` der
// Dateiname der laufenden .exe ("jkstudio.exe", "behaved.exe" oder
// "JK-Studio-rc606-windows-x64.exe"). `nurProgramm`: neben der .exe
// liegt kein data\base - das Programm laeuft eigenstaendig mit seinen
// eingebauten Daten (bhed/eingebaut.h).
//
//   Programm       an die Stelle der LAUFENDEN .exe (sie wird dafuer nur
//                  umbenannt, ueberschreiben laesst sie sich nicht)
//   Datei          neben die .exe, an ihren Platz im Programmordner
//   Ueberspringen  gar nicht
//
// Stufe 8c: genau EIN Eintrag wird Programm - der mit dem Namen der laufenden
// .exe, sonst jkstudio.exe (eigenstaendige .exe mit eigenem Namen). Die zweite
// Programmdatei (behaved.exe neben jkstudio.exe oder umgekehrt) wird eine
// gewoehnliche Datei; bei einer eigenstaendigen .exe entfaellt sie.
enum class Einspielen { Ueberspringen, Programm, Datei };
[[nodiscard]] Einspielen einspielenAls(const std::string& rel, const std::string& laufenderName,
                                       bool nurProgramm);

// --- Stufe 8d: Echtheit des Downloads -------------------------------------
//
// Bis rc605 lud der Updater das .zip per HTTPS und schuetzte nur gegen
// Zip-Slip (zielImOrdner). Jetzt muss jedes Release SHA256SUMS.txt tragen
// (release.yml legt sie seit rc601 an, Format von sha256sum): das .zip wird
// VOR dem Entpacken gegen seine Zeile geprueft. Steht im Programm ein
// oeffentlicher Schluessel (kSignaturSchluessel, ECDSA P-256), ist dazu
// SHA256SUMS.txt.sig Pflicht - eine Signatur ueber SHA256SUMS.txt, wie sie
// "openssl dgst -sha256 -sign" schreibt (DER). Ohne Schluessel (so steht es im
// Repo, bis der Koordinator ihn einmalig erzeugt, docs/intern/NOTIZEN_karte8d.md)
// gilt die Pruefsumme als Mindestanforderung.
//
// Die Updater bis rc605 kennen keine Pruefung; sie laden weiter das erste .zip
// (ctest updater_uebergang) - die zusaetzlichen Dateien stoeren sie nicht.
inline constexpr const char* kSummenDatei = "SHA256SUMS.txt";
inline constexpr const char* kSignaturDatei = "SHA256SUMS.txt.sig";
// Der oeffentliche Schluessel der Release-Signatur: der unkomprimierte Punkt
// "04" + X + Y als 130 Hex-Zeichen (tools/release_paket.py schluessel <pub.der>).
// Leer: keine Signaturpflicht.
inline constexpr const char* kSignaturSchluessel = "";

// Die Datei des Releases mit diesem Namen (gross/klein egal), sonst nullptr.
[[nodiscard]] const Asset* assetNamens(const Release& r, const char* name);

// SHA-256 als 64 kleine Hex-Zeichen (BCrypt). Leer, wenn BCrypt versagt.
[[nodiscard]] std::string sha256Hex(const std::string& daten);

// Die Zeile fuer `datei` in einer SHA256SUMS.txt ("<hex>  <name>" oder
// "<hex> *<name>", LF oder CRLF): die Summe klein geschrieben, sonst leer.
[[nodiscard]] std::string summeFuer(const std::string& summen, const std::string& datei);

// ECDSA P-256 mit SHA-256: `signaturDer` (ASN.1 DER, wie openssl sie
// schreibt) ueber `nachricht`, `schluesselHex` wie kSignaturSchluessel.
[[nodiscard]] bool signaturGueltig(const std::string& nachricht, const std::string& signaturDer,
                                   const std::string& schluesselHex);

enum class Echtheit { Ok, OhneSummen, NichtGelistet, SummeFalsch, OhneSignatur, SignaturFalsch };
// Prueft das .zip `zipName` mit den Bytes `zip`. `summen`/`signatur` sind die
// Inhalte der Release-Dateien oder nullptr, wenn das Release sie nicht hat.
[[nodiscard]] Echtheit pruefeEchtheit(const std::string& zipName, const std::string& zip,
                                      const std::string* summen, const std::string* signatur,
                                      const std::string& schluesselHex);

// Das ganze Einspielen nach dem Download (gui/update_win32.cpp ruft nur das):
//   1. das .zip lesen und pruefen (pruefeEchtheit) - scheitert das, bleibt
//      jede Datei, wie sie war;
//   2. alle Eintraege in einen Zwischenordner neben der .exe entpacken
//      (kZwischenOrdner, derselbe Datentraeger: das Tauschen ist ein
//      Umbenennen) - scheitert ein Eintrag, ist ebenfalls nichts geaendert;
//   3. tauschen: jede alte Datei in den Zwischenordner, die neue an ihren
//      Platz; die laufende .exe wird wie bisher zu "<exe>.alt". Scheitert
//      ein Tausch, kommen die alten Dateien zurueck.
// `zipPfad`: das geladene .zip auf der Platte, `exe`: die laufende .exe.
inline constexpr const char* kZwischenOrdner = "jkstudio-update.tmp";
struct Einspielergebnis {
    Echtheit echtheit = Echtheit::Ok;
    int dateien = 0;            // ersetzte bzw. neue Dateien
    std::string problem;        // leer = alles gut (uebersetzte Meldung)
};
[[nodiscard]] Einspielergebnis einspielen(const std::wstring& zipPfad, const std::string& zipName,
                                          const std::string* summen, const std::string* signatur,
                                          const std::string& schluesselHex, const std::wstring& exe);

}  // namespace bhed::update

#endif  // BHED_UPDATE_H
