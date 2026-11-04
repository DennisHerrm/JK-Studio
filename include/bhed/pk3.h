// pk3.h - in .pk3-Archiven blaettern
//
// Ein .pk3 ist eine Zip-Datei. Gelesen wird nur das Inhaltsverzeichnis am
// Ende (End of Central Directory + Central Directory), nicht die einzelnen
// Dateikoepfe - das ist derselbe Weg, den auch die Engine geht, und er
// braucht kein Durchsuchen des ganzen Archivs.
//
// Wozu: die Skripte einer Mod liegen fast nie ausgepackt herum. Sie stecken
// in `assets*.pk3` oder in der `.pk3` der Mod, zusammen mit den Karten. Ohne
// diesen Leser muesste man vorher von Hand auspacken.
//
// Nur lesend. Ein Archiv aus dem Spielordner ist keine vertrauenswuerdige
// Eingabe: jede Groesse und jeder Versatz wird geprueft, bevor er benutzt
// wird, und die im Verzeichnis angegebene Groesse ist eine harte Obergrenze
// beim Auspacken.
#ifndef BHED_PK3_H
#define BHED_PK3_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "bhed/sicherschreiben.h"

namespace bhed {

struct Pk3Entry {
    std::string name;              // Pfad im Archiv, mit /
    std::uint32_t compressedSize = 0;
    std::uint32_t size = 0;
    std::uint32_t localHeaderOffset = 0;
    std::uint16_t method = 0;      // 0 = ohne Packung, 8 = deflate
};

struct Pk3 {
    std::string path;
    std::vector<Pk3Entry> entries;
    // Kleingeschriebener Name -> Index. Einmal beim Lesen des Verzeichnisses
    // aufgebaut. Ohne ihn kostete jede Namenssuche einen Durchlauf ueber alle
    // Eintraege samt einer Kleinschreibung je Eintrag - bei 5240 Eintraegen
    // rund eine halbe Millisekunde, und gesucht wird oft.
    std::map<std::string, std::size_t> index;
    // Menue-Editor M2: die Ordnereintraege ("models/players/kyle/") in
    // Archivreihenfolge - die Engine listet Ordner nur ueber sie
    // (FS_ListFilteredFiles mit Endung "/"). entries enthaelt sie nicht.
    std::vector<std::string> directories;
    [[nodiscard]] bool empty() const { return entries.empty(); }
    // Gross- und Kleinschreibung wird ignoriert: Zip unterscheidet sie, die
    // Engine nicht, und Ravens Dateien sind darin uneinheitlich.
    [[nodiscard]] const Pk3Entry* find(const std::string& name) const;
};

// Inhaltsverzeichnis lesen. false, wenn es keine Zip-Datei ist.
[[nodiscard]] bool readPk3Directory(const std::string& path, Pk3& out,
                                    std::string* error = nullptr);

// Eine einzelne Datei auspacken.
[[nodiscard]] bool readPk3File(const Pk3& archive, const Pk3Entry& entry,
                               std::string& out, std::string* error = nullptr);

// Alle .pk3 eines Ordners, in der Reihenfolge, die die Engine benutzt:
// alphabetisch, spaetere ueberdecken fruehere.
struct GamePath {
    std::string directory;
    std::vector<Pk3> archives;
    int fileCount = 0;
};

[[nodiscard]] bool scanGamePath(const std::string& directory, GamePath& out);

// rc609 W2b F17: eine Datei so suchen wie FS_FOpenFileRead (OJK
// code/qcommon/files.cpp:1309-1380) in der Suchliste von FS_AddGameDirectory
// (:2730-2805): spaetere Spielordner vor frueheren, je Ordner die .pk3 (die
// alphabetisch letzte zuerst) und die losen Dateien des Ordners - mit
// fs_dirbeforepak 0 (Vorgabe, :2895; Movie Duels 9 gleich, files.cpp:2861) die
// .pk3 vor den losen Dateien, mit 1 die losen zuerst. ordner: niedrigste
// Prioritaet zuerst. herkunft: das Archiv bzw. der volle Pfad der losen Datei.
// Namen mit ".." oder "::" findet das Spiel nicht (FS_FOpenFileRead :1283), ein
// fuehrender / oder \ faellt weg (:1276).
[[nodiscard]] bool liesWieSpiel(const std::vector<GamePath>& ordner, const std::string& name, bool dirVorPak, std::string& out,
                                std::string* herkunft = nullptr, bool* lose = nullptr);
// rc608: FS_Startup liest base immer mit. Wer nur den Mod-Ordner einrichtet (".../MD"), bekommt den Ordner base
// daneben als niedrigsten Rang dazu, wie im Spiel. Die Reihenfolge der uebrigen bleibt.
[[nodiscard]] std::vector<std::string> spielordnerMitBase(const std::vector<std::string>& ordner);

// rc604: FS_PathCmp (code/qcommon/files.cpp:2475-2508 OpenJK, auch Movie Duels
// und Jedi Outcast): Buchstaben a-z gross, '\' und ':' als '/'. paksort
// (:2713-2720) sortiert die .pk3-Namen eines Ordners damit
// (FS_AddGameDirectory :2773) - ohne Ruecksicht auf Gross/klein.
[[nodiscard]] int fsPathCmp(const std::string& a, const std::string& b) noexcept;
void sortierePk3WieEngine(std::vector<std::string>& namen);

// Alle Eintraege eines Suchpfads, die auf eine der Endungen passen -
// zusammengefasst und ohne Dubletten, spaetere Archive gewinnen.
struct FoundFile {
    std::string name;        // Pfad im Archiv
    std::string archive;     // welches .pk3
};
[[nodiscard]] std::vector<FoundFile> findByExtension(
    const GamePath& path, const std::vector<std::string>& extensions);

// --- Schreiben ("Save mission...") ----------------------------------------
struct Pk3Datei {
    std::string name;     // Pfad im Archiv, mit /
    std::string inhalt;
};

// Ein Zip-Archiv ohne Packung ("stored", Methode 0) - die Engine liest es wie
// jedes andere. Erst nach "<path>.tmp", dann umbenennen: ein vorhandenes
// Archiv wird nie halb ueberschrieben, und ist es gesperrt (das Spiel laeuft),
// bleibt es unberuehrt und die Meldung sagt es.
[[nodiscard]] bool writePk3(const std::string& path, const std::vector<Pk3Datei>& files,
                            std::string* error = nullptr);

// rc609 W2a Punkt 4: EINEN Eintrag einer vorhandenen .pk3 ersetzen (fehlt er,
// wird er hinten angehaengt). Alle anderen Eintraege bleiben Byte fuer Byte:
// lokaler Kopf, komprimierte Daten und Datenbeschreibung roh kopiert (nicht neu
// gepackt), ihr Zentralverzeichnis-Eintrag roh mit dem neuen Versatz. Der neue
// Eintrag ohne Packung (Methode 0) mit CRC-32. Vorher "<pk3>.bak" (der Stand vor
// diesem Speichern), geschrieben ueber "<pk3>.tmp" und atomares Umbenennen. Ist
// das Archiv gesperrt (das Spiel hat es offen), bleibt alles unveraendert und
// art sagt Gesperrt. Zip64 und verschluesselte Archive lehnt es ab.
[[nodiscard]] bool pk3EintragErsetzen(const std::string& pk3, const std::string& eintrag, const std::string& inhalt,
                                      std::string* fehler = nullptr, SchreibFehler* art = nullptr);
// Mehrere Eintraege in einem Durchgang (eine Sicherung, ein Umbenennen) - ein
// Skript, dessen .txt und .IBI beide in der .pk3 stehen.
[[nodiscard]] bool pk3EintraegeErsetzen(const std::string& pk3, const std::vector<Pk3Datei>& neu, std::string* fehler = nullptr,
                                        SchreibFehler* art = nullptr);

}  // namespace bhed
#endif
