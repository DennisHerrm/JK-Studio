// enttext.h - rc609 W2b: Entities als Text (Mikes "I like raw text")
//
// Der Entity-Texteditor des Mission-Tabs zeigt die ganze Entity-Liste im
// .ent-Format. Der Text ist mehr als eine Ausgabe von writeEntities: Reihenfolge,
// Leerzeilen und Kommentare bleiben stehen. Aendert der Inspektor oder das Gizmo
// etwas, schreibt entTextAbgleichen nur die betroffenen Zeilen um (die Spannen
// je Entity und Paar kommen aus parseEntitiesGenau, bsp.h).
//
// Kodierung (F13): Das Spiel liest die Bytes der .ent und zeichnet sie mit seiner
// Schrift als Latin-1 (je Byte ein Zeichen). Die Oberflaeche (ImGui) arbeitet mit
// UTF-8. Gespeichert wird immer Latin-1; ein Zeichen jenseits von U+00FF gibt
// eine Meldung, nichts wird still ersetzt.
//
// Eigener Code.
#ifndef BHED_ENTTEXT_H
#define BHED_ENTTEXT_H

#include "bhed/bsp.h"

#include <cstddef>
#include <string>
#include <vector>

namespace bhed {

// Latin-1 (Bytes der Datei) -> UTF-8 (Oberflaeche). Jedes Byte wird ein Zeichen.
[[nodiscard]] std::string latin1NachUtf8(const std::string& bytes);
// UTF-8 -> Latin-1. false, wenn ein Zeichen nicht in Latin-1 liegt (oder das
// UTF-8 kaputt ist); stelle = Byte im UTF-8-Text, zeichen = das Zeichen (UTF-8).
[[nodiscard]] bool utf8NachLatin1(const std::string& utf8, std::string& out, std::size_t* stelle = nullptr,
                                  std::string* zeichen = nullptr);

// Die Schluessel, die beim Tippen sofort wirken (BSP-Entity-Edit: origin, angle(s),
// modelscale(_vec), light, _color).
[[nodiscard]] bool entTextLiveSchluessel(const std::string& key);
// Gleiche Schluessel in gleicher Reihenfolge, Unterschiede nur in Werten von
// Live-Schluesseln (und mindestens einer)?
[[nodiscard]] bool entTextNurLive(const MapEntity& alt, const MapEntity& neu);
[[nodiscard]] bool entGleich(const MapEntity& a, const MapEntity& b);
[[nodiscard]] bool entListeGleich(const std::vector<MapEntity>& a, const std::vector<MapEntity>& b);

// Text an neue Entities angleichen. Unveraenderte Entities und Paare bleiben Byte
// fuer Byte, samt allem dazwischen; ein geaenderter Wert ersetzt nur sein Token,
// ein entferntes Paar seine Zeile, ein neues kommt als eigene Zeile hinter das
// vorige Paar (fehlt es ganz, hinter die '{'). Neue Entities im Format von
// writeEntities. Ist der Text nicht lesbar, kommt writeEntities(neu).
[[nodiscard]] std::string entTextAbgleichen(const std::string& text, const std::vector<MapEntity>& neu);

// Text pruefen wie das Spiel beim Laden: parseEntitiesGenau und dazu
// SP_worldspawn - die erste Entity muss worldspawn sein (OJK
// code/game/g_spawn.cpp:1479-1481, MD9 :1686-1688, JK2 codeJK2 g_spawn.cpp:1249-1251).
// Strenger als das Spiel nur an einer Stelle: ein Token in Anfuehrungszeichen ueber
// ein Zeilenende gilt als offenes Anfuehrungszeichen (seine Zeile wird gemeldet).
[[nodiscard]] bool entTextPruefen(const std::string& text, std::vector<MapEntity>& out, std::vector<EntBlockSpanne>* spannen,
                                  EntTextFehler* fehler);

// Zeile (ab 0) eines Byte-Versatzes.
[[nodiscard]] int entTextZeile(const std::string& text, std::size_t pos);

}  // namespace bhed

#endif  // BHED_ENTTEXT_H
