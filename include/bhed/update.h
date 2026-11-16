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

#include <atomic>
#include <cstdint>
#include <functional>
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
//      ein Tausch, kommen die alten Dateien zurueck. Das Programm wird
//      zuletzt getauscht (rc610 F78).
// `zipPfad`: das geladene .zip auf der Platte, `exe`: die laufende .exe.
// `abbrechen` (rc610 F78, Fund B8-4): wird es waehrend des Entpackens true (das
// Programm endet), hoert einspielen auf, bevor etwas getauscht ist; den Tausch
// selbst bricht es nie ab.
inline constexpr const char* kZwischenOrdner = "jkstudio-update.tmp";
struct Einspielergebnis {
    Echtheit echtheit = Echtheit::Ok;
    int dateien = 0;            // ersetzte bzw. neue Dateien
    std::string problem;        // leer = alles gut (uebersetzte Meldung)
    // rc610 F78 (Fund B8-5): nicht jede alte Datei liess sich zurueckstellen; ihre
    // Sicherung bleibt im Zwischenordner, resteAufraeumen stellt sie beim
    // naechsten Start zurueck.
    bool sicherungBleibt = false;
    int geloescht = 0;          // REL-TEMPO: im Delta weggefallene Dateien (in den Zwischenordner)
};
// REL-TEMPO: was eingespielt wird. WieDaneben (das Programm): wie bisher - ohne dataase neben
// der .exe nur das Programm. Alles (der Starter, auch in einen leeren Ordner): jede Datei.
enum class Umfang { WieDaneben, Alles };
[[nodiscard]] Einspielergebnis einspielen(const std::wstring& zipPfad, const std::string& zipName,
                                          const std::string* summen, const std::string* signatur,
                                          const std::string& schluesselHex, const std::wstring& exe,
                                          const std::atomic<bool>* abbrechen = nullptr, Umfang umfang = Umfang::WieDaneben);

// Beim Programmstart (rc610 F78): ein Zwischenordner neben `exe` ist der Rest
// eines Updates, das nicht zu Ende kam (Rueckstellen gescheitert, Programm
// mitten im Tausch beendet). Ohne "<exe>.alt" war der Tausch nicht fertig: die
// Sicherungen kommen zurueck an ihren Platz, die dabei neu angelegten Dateien
// weg. Mit "<exe>.alt" war er fertig, nur der Ordner bleibt uebrig. Der
// Zwischenordner verschwindet erst, wenn alles zurueckgestellt ist.
// REL-TEMPO: ebenso fertig ist er mit der Marke "fertig" im Zwischenordner (ein Delta, das das
// Programm nicht aendert, und der Starter beim ersten Einspielen haben kein "<exe>.alt").
// Rueckgabe: so viele Dateien wurden zurueckgestellt.
int resteAufraeumen(const std::wstring& exe);

// Nur fuer ctest und Selbsttest (rc610 F78): einspielen ruft den Haken nach
// jedem entpackten und nach jedem getauschten Eintrag (`rel` wie zielImOrdner),
// im Faden des Einspielens. Leer = aus.
enum class TestPhase { Entpackt, Getauscht };
void testHakenSetzen(std::function<void(TestPhase, const std::string& rel)> haken);

// --- REL-TEMPO (15.11.): Meldungen, Manifest, Delta-Pakete, der ganze Ablauf ----
//
// shank 14.11.: "lassen wir die aktuelle exe einfach weg in Zukunft und bieten eine zentrale an
// und der Rest wird automatisch geupdated". Dazu: ein kleiner Starter (starter/) und Updates,
// die nur laden, was sich geaendert hat.
//
// Die Meldungen des Einspielens (Einspielergebnis::problem): das Programm setzt seine
// Uebersetzungen (gui/update_win32.cpp), der Starter seine kleine Tabelle - so braucht der
// Starter die grosse Uebersetzungstabelle nicht. Ohne Tabelle: Englisch. Jeder Text bekommt wie
// bisher (const char* name, long long zahl) fuer printf.
enum class Meldung { ZipUnlesbar, OhneSummen, NichtGelistet, Pruefsumme, Signatur, Abbruch, Schreiben, Ersetzen };
using Meldungstexte = const char* (*)(Meldung);
void meldungstexteSetzen(Meldungstexte texte);
[[nodiscard]] std::string meldungText(Meldung m, const std::string& name = {}, long long zahl = 0);

// Ein Release ab REL-TEMPO traegt neben dem vollen Paket (das ERSTE und einzige .zip - die
// Updater bis rc612 nehmen es) und SHA256SUMS.txt:
//   JK-Studio-<rc>-manifest.txt         jede Datei des Pakets: SHA-256, Groesse, Pfad
//   JK-Studio-<rc>-from-<alt>.delta     je eine der letzten Fassungen: nur die Inhalte, die <alt>
//                                       nicht hat, und welche Dateien wegfallen (ein Zip, aber
//                                       ohne die Endung .zip)
// Beide stehen in SHA256SUMS.txt, sind also wie das .zip geprueft und unterschrieben. Scheitert
// das Delta an irgendetwas (Pruefung, fehlender Inhalt, geaenderte Datei), ist nichts geaendert,
// und das volle Paket kommt (aktualisieren).
inline constexpr const char* kPaketName = "JK-Studio";
inline constexpr const char* kDeltaKopf = "delta.txt";        // im .delta: von, nach, weg
inline constexpr const char* kDeltaInhalt = "inhalt/";        // im .delta: inhalt/<sha256>
// "v1.0.0-rc613" -> "rc613" (leer ohne Nummer).
[[nodiscard]] std::string rcVon(const std::string& tag);
[[nodiscard]] std::string zipName(const std::string& rc);                              // JK-Studio-rc613-windows-x64.zip
[[nodiscard]] std::string manifestName(const std::string& rc);                         // JK-Studio-rc613-manifest.txt
[[nodiscard]] std::string deltaName(const std::string& neu, const std::string& alt);   // JK-Studio-rc613-from-rc612.delta

struct ManifestDatei {
    std::string sha256;        // 64 kleine Hex-Zeichen
    long long groesse = 0;
    std::string pfad;          // relativ zum Programmordner, '/' (zielImOrdner-tauglich)
};
struct Manifest {
    std::string fassung;       // "rc613"
    std::vector<ManifestDatei> dateien;
};
// "# JK Studio manifest 1" / "fassung rc613" / "<sha256> <groesse> <pfad>" je Zeile (LF).
// false bei unbekanntem Kopf, kaputter Zeile, unsicherem oder doppeltem Pfad.
[[nodiscard]] bool parseManifest(const std::string& text, Manifest& out, std::string* fehler = nullptr);
struct DeltaKopf {
    std::string von, nach;
    std::vector<std::string> weg;   // Dateien von `von`, die es in `nach` nicht mehr gibt
};
// "# JK Studio delta 1" / "von rc612" / "nach rc613" / "weg <pfad>" je Zeile.
[[nodiscard]] bool parseDeltaKopf(const std::string& text, DeltaKopf& out, std::string* fehler = nullptr);

// SHA-256 einer Datei in Stuecken (64 kleine Hex-Zeichen; leer, wenn sie fehlt).
[[nodiscard]] std::string sha256Datei(const std::wstring& pfad);
// Wie pruefeEchtheit, mit der schon berechneten Summe `istHex` der Datei `name`.
[[nodiscard]] Echtheit pruefeSumme(const std::string& name, const std::string& istHex, const std::string* summen,
                                   const std::string* signatur, const std::string& schluesselHex);

// Das Delta einspielen: `deltaPfad`/`manifestPfad` sind die geladenen Dateien. Prueft beide gegen
// SHA256SUMS.txt (und die Signatur), dann je Datei des Manifests: liegt sie schon so da (SHA-256),
// bleibt sie; sonst kommt ihr Inhalt aus dem Delta oder aus einer gleichen Datei im Ordner (jeder
// Inhalt wieder gegen das Manifest geprueft). Fehlt einer, ist nichts geaendert (problem gesetzt,
// dateien 0). Getauscht wird wie bei einspielen (Zwischenordner, Programm zuletzt, Rueckstellen);
// wegfallende Dateien gehen in den Zwischenordner und mit ihm weg.
[[nodiscard]] Einspielergebnis einspielenDelta(const std::wstring& deltaPfad, const std::string& deltaDatei,
                                               const std::wstring& manifestPfad, const std::string& manifestDatei,
                                               const std::string* summen, const std::string* signatur,
                                               const std::string& schluesselHex, const std::wstring& exe,
                                               const std::string& vonFassung, const std::string& nachFassung,
                                               const std::atomic<bool>* abbrechen = nullptr,
                                               Umfang umfang = Umfang::WieDaneben);

// Woher die Dateien eines Releases kommen: GitHub (Programm, Starter), ein Testserver, ctest.
class Quelle {
public:
    Quelle() = default;
    Quelle(const Quelle&) = delete;
    Quelle& operator=(const Quelle&) = delete;
    virtual ~Quelle() = default;
    // `a` nach `ziel` laden. fortschritt(geladen, gesamt; 0 = unbekannt). false: `fehler`
    // (uebersetzt) - auch beim Abbruch.
    virtual bool laden(const Asset& a, const std::wstring& ziel,
                       const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt, std::string& fehler) = 0;
};
struct Ablaufergebnis {
    Einspielergebnis einspielen;
    bool mitDelta = false;        // aus dem Delta eingespielt
    std::string deltaGrund;       // warum das Delta nicht ging (leer: keins da oder es ging)
    std::string ladeFehler;       // das volle Paket kam nicht an (dann ist nichts eingespielt)
};
// Der ganze Ablauf nach "Jetzt aktualisieren" (Programm) bzw. im Starter: gibt es fuer
// `lokaleFassung` ein Delta und ein Manifest, erst das Delta; sonst - oder wenn es scheitert und
// nichts geaendert ist - das volle Paket (zipAsset) wie bisher. Erst die grosse Datei, dann
// SHA256SUMS.txt und die Signatur (bricht man das Laden ab, kommt nichts weiter). `ablage`: Ordner
// fuer die geladenen Dateien (der Aufrufer raeumt ihn weg). `lokaleFassung` leer: nur das volle Paket.
[[nodiscard]] Ablaufergebnis aktualisieren(const Release& r, const std::string& lokaleFassung, const std::wstring& exe,
                                           Quelle& quelle, const std::wstring& ablage, const std::string& schluesselHex,
                                           const std::atomic<bool>* abbrechen,
                                           const std::function<void(std::uint64_t, std::uint64_t)>& fortschritt,
                                           Umfang umfang = Umfang::WieDaneben);

}  // namespace bhed::update

#endif  // BHED_UPDATE_H
