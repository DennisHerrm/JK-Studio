// hilfetest.cpp - der Helfer ohne Oberflaeche (Stufe H1, bhed/hilfe.h)
//
//   a) Woerter: klein, Umlaute gefaltet, gestutzt, CJK als Ein- und
//      Doppelzeichen, Begriffe ueber die Sprachen (@kamera)
//   b) Ortskatalog: Hauptmenue, Modusleiste, Einstellungen und die Menues des
//      Karten-Editors; jede Kennung hat einen Text
//   c) die haeufigen Ziele (src/hilfe_daten.inc) stehen in jeder Sprache im Index
//   d) Trefferquote: typische Fragen in vier Sprachen, Top 3 - Ziel >= 85 %
//      (Argument 1 = Handbuchordner mit <sprache>/touren.json und suche.js)
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "bhed/hilfe.h"
#include "bhed/i18n.h"
#include "bhed/tour.h"

namespace {

int fehler = 0;

void expect(const char* was, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

std::string datei(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}

bool hat(const std::vector<std::string>& v, const std::string& w) {
    for (const std::string& x : v) {
        if (x == w) { return true; }
    }
    return false;
}

void woerterProben() {
    std::printf("Woerter\n");
    const std::vector<std::string> a = bhed::hilfe::woerter("Wie mache ich eine Kamerafahrt mit Texturen?");
    expect("Stoppwoerter fallen weg (wie, ich, eine, mit)", !hat(a, "wie") && !hat(a, "ich") && !hat(a, "eine") && !hat(a, "mit"));
    expect("Texturen -> textur", hat(a, "textur"));
    expect("Kamerafahrt traegt @kamera und @fahrt", hat(a, "@kamera") && hat(a, "@fahrt"));
    expect("Texturen traegt @textur", hat(a, "@textur"));
    const std::vector<std::string> b = bhed::hilfe::woerter("Gr\xc3\xb6\xc3\x9f" "e \xc3\xa4ndern f\xc3\xbcr T\xc3\xbcren");
    expect("Umlaute gefaltet und gestutzt: groess, aend, tuer", hat(b, "groess") && hat(b, "aend") && hat(b, "tuer"));
    expect("Tueren -> @tuer", hat(b, "@tuer"));
    const std::vector<std::string> c = bhed::hilfe::woerter("\xe5\x88\xb6\xe4\xbd\x9c\xe9\x95\x9c\xe5\xa4\xb4\xe8\xbf\x90\xe5\x8a\xa8");
    expect("CJK: Einzel- und Doppelzeichen", hat(c, "\xe9\x95\x9c") && hat(c, "\xe9\x95\x9c\xe5\xa4\xb4") && hat(c, "\xe8\xbf\x90\xe5\x8a\xa8"));
    expect("CJK: \xe9\x95\x9c\xe5\xa4\xb4 -> @kamera", hat(c, "@kamera"));
    const std::vector<std::string> d = bhed::hilfe::woerter("\xe3\x82\xab\xe3\x83\xa1\xe3\x83\xa9\xe3\x83\xaf\xe3\x83\xbc\xe3\x82\xaf\xe3\x82\x92\xe4\xbd\x9c\xe3\x82\x8b");
    expect("Katakana: \xe3\x82\xab\xe3\x83\xa1\xe3\x83\xa9 -> @kamera, \xe4\xbd\x9c\xe3\x82\x8b -> @neu", hat(d, "@kamera") && hat(d, "@neu"));
    const std::vector<std::string> e = bhed::hilfe::woerter("camera path");
    expect("camera path -> @fahrt (zwei Woerter)", hat(e, "@fahrt"));
    expect("leer -> nichts", bhed::hilfe::woerter("   ").empty());
    // Weitere Sprachen: Akzente gefaltet (wie die Suche im Handbuch), Stoppwoerter
    // und Endungen der Sprache (src/hilfe_sprache_<kurz>.inc).
    using L = bhed::i18n::Language;
    expect("Akzente: Animaci\xc3\xb3n = animacion (Spanisch)",
           bhed::hilfe::woerter("Animaci\xc3\xb3n", L::Spanish) == bhed::hilfe::woerter("animacion", L::Spanish));
    expect("Akzente: \xc4\x8c" "e\xc5\xa1tina = cestina, \xc4\x9a\xc5\xa1\xc4\x8d\xc5\x99\xc5\xbe\xc3\xbd\xc3\xa1\xc3\xad\xc3\xa9\xc5\xaf\xc3\xba\xc4\x8f\xc5\xa5\xc5\x88 = escrzyaieuudtn (Tschechisch)",
           bhed::hilfe::woerter("\xc4\x8c" "e\xc5\xa1tina", L::Czech) == bhed::hilfe::woerter("cestina", L::Czech) &&
               bhed::hilfe::woerter("\xc4\x9a\xc5\xa1\xc4\x8d\xc5\x99\xc5\xbe\xc3\xbd\xc3\xa1\xc3\xad\xc3\xa9\xc5\xaf\xc3\xba\xc4\x8f\xc5\xa5\xc5\x88", L::Czech) == bhed::hilfe::woerter("escrzyaieuudtn", L::Czech));
    expect("Akzente: \xc3\xb1 -> n, \xc3\xa1 \xc3\xa9 \xc3\xad \xc3\xb3 \xc3\xba -> a e i o u",
           bhed::hilfe::woerter("A\xc3\xb1o c\xc3\xa1mara tr\xc3\xa1nsito", L::Spanish) == bhed::hilfe::woerter("ano camara transito", L::Spanish));
    const std::vector<std::string> es = bhed::hilfe::woerter("\xc2\xbf" "C\xc3\xb3mo hago una animaci\xc3\xb3n con las texturas?", L::Spanish);
    expect("Spanisch: Stoppwoerter fallen weg (como, hago, una, con, las)",
           !hat(es, "como") && !hat(es, "hago") && !hat(es, "una") && !hat(es, "con") && !hat(es, "las"));
    expect("Spanisch: animaci\xc3\xb3n, animaciones, animar, animado -> anim",
           hat(es, "anim") && hat(bhed::hilfe::woerter("animaciones", L::Spanish), "anim") &&
               hat(bhed::hilfe::woerter("animar", L::Spanish), "anim") && hat(bhed::hilfe::woerter("animado", L::Spanish), "anim"));
    expect("Spanisch: textura, texturas -> textur (wie texture)",
           hat(es, "textur") && hat(bhed::hilfe::woerter("textura", L::Spanish), "textur") &&
               hat(bhed::hilfe::woerter("texture", L::Spanish), "textur"));
    expect("die vier ersten Sprachen stutzen wie bisher (Englisch: animation bleibt)",
           bhed::hilfe::woerter("animation", L::German) == bhed::hilfe::woerter("animation") &&
               hat(bhed::hilfe::woerter("animation"), "animation"));
    expect("schrittTitel: der fette Anfang", bhed::hilfe::schrittTitel("**Texture.** Double-click a folder") == "Texture.");
}

void orteProben() {
    std::printf("Ortskatalog\n");
    const std::vector<bhed::hilfe::Ort>& o = bhed::hilfe::orte();
    int menue = 0;
    int karte = 0;
    int einst = 0;
    int ohneText = 0;
    int leisten = 0;      // H2: Knoepfe der Leisten (Vorschau, Kamera, Zeitleiste, Wegpunkte, Modell)
    int keSeiten = 0;     // H2: Seiten der Einstellungen des Karten-Editors
    int menueEd = 0;      // H2: Leiste des Menue-Editors
    int kette2 = 0;       // H2: verkettete Vorbedingungen
    int inhaltOhne = 0;   // H2: Kennungen im Inhalt ohne Text
    bool prefsKette = false;
    for (const bhed::hilfe::Ort& x : o) {
        const std::string z = x.ziel;
        const std::string b = x.bereich;
        if (z.rfind("menue:", 0) == 0 && b == "karte") { ++karte; }
        if (z.rfind("menue:", 0) == 0) { ++menue; }
        if (z.rfind("knopf:", 0) == 0 && (b == "vorschau" || b == "modell")) { ++leisten; }
        if (z.rfind("gezeichnet:karte.einstseite.", 0) == 0) { ++keSeiten; }
        if (z.rfind("knopf:Me", 0) == 0 && b == "menue") { ++menueEd; }
        if (std::string(x.voraus).find("&&") != std::string::npos) { ++kette2; }
        if (z == "knopf:PrefsRegelwerk") {
            prefsKette = std::string(x.voraus) == "zahl:programm.hauptmenue < knopf:ViewEvents@modebar && zahl:programm.einstellungen < menue:MenuEdit>AppPrefs";
        }
        for (std::string in = x.inhalt; !in.empty();) {
            const std::size_t p = in.find(',');
            bhed::i18n::Str id{};
            if (!bhed::i18n::strVonName(in.substr(0, p), &id)) { ++inhaltOhne; }
            if (p == std::string::npos) { break; }
            in.erase(0, p + 1);
        }
        if (std::string(x.bereich) == "einstellungen") { ++einst; }
        std::string k = x.kette;
        std::size_t a = 0;
        while (a <= k.size()) {
            std::size_t e = k.find('>', a);
            if (e == std::string::npos) { e = k.size(); }
            bhed::i18n::Str id{};
            if (!bhed::i18n::strVonName(k.substr(a, e - a), &id)) { ++ohneText; }
            a = e + 1;
        }
    }
    std::printf("    %zu Orte: %d Menuepunkte, %d im Karten-Editor, %d Einstellungen\n", o.size(), menue, karte, einst);
    expect("mindestens 50 Menuepunkte des Hauptprogramms", menue - karte >= 50);
    expect("mindestens 150 Menuepunkte des Karten-Editors", karte >= 150);
    expect("mindestens 10 Einstellungen", einst >= 10);
    expect("jede Kennung der Ketten hat einen Text", ohneText == 0);
    std::printf("    H2: %d Leistenknoepfe, %d Seiten der KE-Einstellungen, %d im Menue-Editor, %d mit verketteter Vorbedingung\n", leisten, keSeiten,
                menueEd, kette2);
    expect("H2: mindestens 30 Leistenknoepfe (Vorschau, Kamera, Zeitleiste, Wegpunkte, Modell)", leisten >= 30);
    expect("H2: mindestens 20 Seiten der Einstellungen des Karten-Editors", keSeiten >= 20);
    expect("H2: mindestens 8 Orte im Menue-Editor", menueEd >= 8);
    expect("H2: Einstellung des Hauptprogramms: erst Events, dann Bearbeiten > Einstellungen (verkettet)", prefsKette);
    expect("H2: jede Kennung im Inhalt hat einen Text", inhaltOhne == 0);
}

struct Frage {
    const char* sprache;
    const char* text;
    const char* erwartet;   // Vorsilben der Kennungen, "|" getrennt
};

// Typische Fragen - wie Leute in Foren und JKHub-Tutorials fragen, nicht nur
// die Wortlisten der Ziele.
// H2: wo eine neue Tour (k-update, k-spielordner, tut-karte-tuer) oder ein neuer
// Ort (Knopf "Modell oeffnen" der Leiste) die Antwort ist, zaehlt sie hier mit -
// diese Liste war nie unabhaengig (die beiden unabhaengigen bleiben unveraendert).
const Frage kFragen[] = {
    // --- Deutsch ---
    {"de", "Wie mache ich meine erste eigene Karte?", "tour:tut-karte"},
    {"de", "Wie bekomme ich eine Textur auf die Wand?", "tour:tut-karte@4|tour:tut-karte@6"},
    {"de", "Licht in den Raum setzen", "tour:tut-karte@7"},
    {"de", "Wo setze ich den Spielerstart?", "tour:tut-karte@8"},
    {"de", "Karte kompilieren", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"de", "Wie packe ich meine Karte als pk3 ins Spiel?", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"de", "Optimierer benutzen", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"de", "Im Editor durch die Karte laufen", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"de", "Erstes Skript schreiben", "tour:k-neues-skript"},
    {"de", "Skript in eine ibi \xc3\xbc" "bersetzen", "tour:k-uebersetzen|seite:k-ibi"},
    {"de", "Befehl einf\xc3\xbcgen", "tour:k-befehl-einfuegen"},
    {"de", "Kamerafahrt f\xc3\xbcr eine Zwischensequenz", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"de", "NPC zu einem Wegpunkt laufen lassen", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"de", "Wo finde ich die Einstellungen?", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen|tour:k-spielordner@3"},
    {"de", "Sprache auf Englisch umstellen", "ort:menue:MenuView>ViewLanguage"},
    {"de", "Gibt es eine neue Version?", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck"},
    {"de", "Modell \xc3\xb6" "ffnen", "ort:menue:MenuFile>OpenGlm|ort:knopf:OpenGlm|ort:knopf:ViewModel@modebar|seite:modell|tour:k-modell@|seite:k-modell"},
    {"de", "Mission laden", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"de", "Tastenk\xc3\xbcrzel \xc3\xa4ndern", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"de", "Raum aush\xc3\xb6hlen", "tour:tut-karte@5"},
    {"de", "wie benutzt man das model zu brush?", "tour:k-geometrie"},   // rc610 TUT-GEO: shanks Frage 05.11.
    {"de", "Dunkles Design einschalten", "ort:menue:MenuView>ViewTheme"},
    {"de", "R\xc3\xbc" "ckg\xc3\xa4ngig machen", "seite:k-rueckgaengig|ort:menue:MenuEdit|ort:menue:KeLEdit>KeLUndo"},
    // --- English ---
    {"en", "How do I make my first map?", "tour:tut-karte"},
    {"en", "apply a texture to a brush", "tour:tut-karte@4|tour:tut-karte@6"},
    {"en", "add a light to my room", "tour:tut-karte@7"},
    {"en", "where does the player spawn", "tour:tut-karte@8"},
    {"en", "compile my map", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"en", "pack the map into a pk3", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"en", "check the map limits with the optimizer", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"en", "walk around my map in the editor", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"en", "write my first script", "tour:k-neues-skript"},
    {"en", "compile a script to ibi", "tour:k-uebersetzen|seite:k-ibi"},
    {"en", "insert a command", "tour:k-befehl-einfuegen"},
    {"en", "make a camera move for a cutscene", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"en", "make an NPC walk to a waypoint", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"en", "open the settings", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"en", "change the language", "ort:menue:MenuView>ViewLanguage"},
    {"en", "check for updates", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|tour:k-update"},
    {"en", "open a model", "ort:menue:MenuFile>OpenGlm|ort:knopf:OpenGlm|ort:knopf:ViewModel@modebar|seite:modell|tour:k-modell@|seite:k-modell"},
    {"en", "load a mission", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"en", "change keyboard shortcuts", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"en", "hollow out a room", "tour:tut-karte@5"},
    {"en", "how do I turn a Blender model into brushes", "tour:k-geometrie"},
    {"en", "the text is too small", "ort:menue:MenuView>ViewDpiScale"},
    // --- Chinesisch ---
    {"zh", "\xe6\x80\x8e\xe4\xb9\x88\xe5\x88\xb6\xe4\xbd\x9c\xe7\xac\xac\xe4\xb8\x80\xe5\xbc\xa0\xe5\x9c\xb0\xe5\x9b\xbe", "tour:tut-karte"},
    {"zh", "\xe7\xbb\x99\xe7\x94\xbb\xe5\x88\xb7\xe5\xba\x94\xe7\x94\xa8\xe7\xba\xb9\xe7\x90\x86", "tour:tut-karte@4|tour:tut-karte@6"},
    {"zh", "\xe6\xb7\xbb\xe5\x8a\xa0\xe7\x81\xaf\xe5\x85\x89", "tour:tut-karte@7"},
    {"zh", "\xe8\xae\xbe\xe7\xbd\xae\xe7\x8e\xa9\xe5\xae\xb6\xe5\x87\xba\xe7\x94\x9f\xe7\x82\xb9", "tour:tut-karte@8"},
    {"zh", "\xe7\xbc\x96\xe8\xaf\x91\xe5\x9c\xb0\xe5\x9b\xbe", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"zh", "\xe6\x8a\x8a\xe5\x9c\xb0\xe5\x9b\xbe\xe6\x89\x93\xe5\x8c\x85\xe6\x88\x90pk3", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"zh", "\xe4\xbd\xbf\xe7\x94\xa8\xe4\xbc\x98\xe5\x8c\x96\xe5\x99\xa8", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"zh", "\xe5\x9c\xa8\xe7\xbc\x96\xe8\xbe\x91\xe5\x99\xa8\xe4\xb8\xad\xe8\xaf\x95\xe7\x8e\xa9\xe5\x9c\xb0\xe5\x9b\xbe", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"zh", "\xe5\x86\x99\xe7\xac\xac\xe4\xb8\x80\xe4\xb8\xaa\xe8\x84\x9a\xe6\x9c\xac", "tour:k-neues-skript"},
    {"zh", "\xe7\xbc\x96\xe8\xaf\x91\xe8\x84\x9a\xe6\x9c\xac", "tour:k-uebersetzen|seite:k-ibi"},
    {"zh", "\xe6\x8f\x92\xe5\x85\xa5\xe5\x91\xbd\xe4\xbb\xa4", "tour:k-befehl-einfuegen"},
    {"zh", "\xe5\x88\xb6\xe4\xbd\x9c\xe8\xbf\x87\xe5\x9c\xba\xe5\x8a\xa8\xe7\x94\xbb\xe7\x9a\x84\xe9\x95\x9c\xe5\xa4\xb4\xe8\xbf\x90\xe5\x8a\xa8", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"zh", "\xe8\xae\xa9NPC\xe8\xb5\xb0\xe5\x88\xb0\xe8\xb7\xaf\xe5\xbe\x84\xe7\x82\xb9", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"zh", "\xe6\x89\x93\xe5\xbc\x80\xe8\xae\xbe\xe7\xbd\xae", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"zh", "\xe6\x9b\xb4\xe6\x94\xb9\xe8\xaf\xad\xe8\xa8\x80", "ort:menue:MenuView>ViewLanguage"},
    {"zh", "\xe6\xa3\x80\xe6\x9f\xa5\xe6\x9b\xb4\xe6\x96\xb0", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|tour:k-update"},
    {"zh", "\xe6\x89\x93\xe5\xbc\x80\xe6\xa8\xa1\xe5\x9e\x8b", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell|tour:k-modell@|seite:k-modell"},
    {"zh", "\xe8\xbd\xbd\xe5\x85\xa5\xe4\xbb\xbb\xe5\x8a\xa1", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"zh", "\xe4\xbf\xae\xe6\x94\xb9\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"zh", "\xe6\x8c\x96\xe7\xa9\xba\xe6\x88\xbf\xe9\x97\xb4", "tour:tut-karte@5"},
    // --- Japanisch ---
    {"ja", "\xe5\x88\x9d\xe3\x82\x81\xe3\x81\xa6\xe3\x81\xae\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97\xe3\x82\x92\xe4\xbd\x9c\xe3\x82\x8b\xe3\x81\xab\xe3\x81\xaf", "tour:tut-karte"},
    {"ja", "\xe3\x83\x96\xe3\x83\xa9\xe3\x82\xb7\xe3\x81\xab\xe3\x83\x86\xe3\x82\xaf\xe3\x82\xb9\xe3\x83\x81\xe3\x83\xa3\xe3\x82\x92\xe8\xb2\xbc\xe3\x82\x8b", "tour:tut-karte@4|tour:tut-karte@6"},
    {"ja", "\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\x88\xe3\x82\x92\xe7\xbd\xae\xe3\x81\x8f", "tour:tut-karte@7"},
    {"ja", "\xe3\x82\xb9\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\xb3\xe5\x9c\xb0\xe7\x82\xb9\xe3\x82\x92\xe8\xa8\xad\xe5\xae\x9a", "tour:tut-karte@8"},
    {"ja", "\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97\xe3\x82\x92\xe3\x83\x93\xe3\x83\xab\xe3\x83\x89\xe3\x81\x99\xe3\x82\x8b", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"ja", "\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97\xe3\x82\x92pk3\xe3\x81\xab\xe3\x83\x91\xe3\x83\x83\xe3\x82\xaf\xe3\x81\x99\xe3\x82\x8b", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"ja", "\xe6\x9c\x80\xe9\x81\xa9\xe5\x8c\x96\xe3\x83\x84\xe3\x83\xbc\xe3\x83\xab\xe3\x82\x92\xe4\xbd\xbf\xe3\x81\x86", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"ja", "\xe3\x82\xa8\xe3\x83\x87\xe3\x82\xa3\xe3\x82\xbf\xe3\x83\xbc\xe3\x81\xa7\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97\xe3\x82\x92\xe6\xad\xa9\xe3\x81\x8d\xe5\x9b\x9e\xe3\x82\x8b", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"ja", "\xe6\x9c\x80\xe5\x88\x9d\xe3\x81\xae\xe3\x82\xb9\xe3\x82\xaf\xe3\x83\xaa\xe3\x83\x97\xe3\x83\x88\xe3\x82\x92\xe6\x9b\xb8\xe3\x81\x8f", "tour:k-neues-skript"},
    {"ja", "\xe3\x82\xb9\xe3\x82\xaf\xe3\x83\xaa\xe3\x83\x97\xe3\x83\x88\xe3\x82\x92\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab", "tour:k-uebersetzen|seite:k-ibi"},
    {"ja", "\xe3\x82\xb3\xe3\x83\x9e\xe3\x83\xb3\xe3\x83\x89\xe3\x82\x92\xe6\x8c\xbf\xe5\x85\xa5\xe3\x81\x99\xe3\x82\x8b", "tour:k-befehl-einfuegen"},
    {"ja", "\xe3\x82\xab\xe3\x83\x83\xe3\x83\x88\xe3\x82\xb7\xe3\x83\xbc\xe3\x83\xb3\xe3\x81\xae\xe3\x82\xab\xe3\x83\xa1\xe3\x83\xa9\xe3\x83\xaf\xe3\x83\xbc\xe3\x82\xaf\xe3\x82\x92\xe4\xbd\x9c\xe3\x82\x8b", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"ja", "NPC\xe3\x82\x92\xe3\x82\xa6\xe3\x82\xa7\xe3\x82\xa4\xe3\x83\x9d\xe3\x82\xa4\xe3\x83\xb3\xe3\x83\x88\xe3\x81\xbe\xe3\x81\xa7\xe6\xad\xa9\xe3\x81\x8b\xe3\x81\x9b\xe3\x82\x8b", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"ja", "\xe8\xa8\xad\xe5\xae\x9a\xe3\x82\x92\xe9\x96\x8b\xe3\x81\x8f", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"ja", "\xe8\xa8\x80\xe8\xaa\x9e\xe3\x82\x92\xe5\xa4\x89\xe6\x9b\xb4\xe3\x81\x99\xe3\x82\x8b", "ort:menue:MenuView>ViewLanguage"},
    {"ja", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe3\x83\x87\xe3\x83\xbc\xe3\x83\x88\xe3\x82\x92\xe7\xa2\xba\xe8\xaa\x8d", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|tour:k-update"},
    {"ja", "\xe3\x83\xa2\xe3\x83\x87\xe3\x83\xab\xe3\x82\x92\xe9\x96\x8b\xe3\x81\x8f", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell|tour:k-modell@|seite:k-modell"},
    {"ja", "\xe3\x83\x9f\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa7\xe3\x83\xb3\xe3\x82\x92\xe8\xaa\xad\xe3\x81\xbf\xe8\xbe\xbc\xe3\x82\x80", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"ja", "\xe3\x82\xb7\xe3\x83\xa7\xe3\x83\xbc\xe3\x83\x88\xe3\x82\xab\xe3\x83\x83\xe3\x83\x88\xe3\x82\x92\xe5\xa4\x89\xe6\x9b\xb4", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"ja", "\xe9\x83\xa8\xe5\xb1\x8b\xe3\x82\x92\xe4\xb8\xad\xe7\xa9\xba\xe3\x81\xab\xe3\x81\x99\xe3\x82\x8b", "tour:tut-karte@5"},
    // --- frei formuliert (keine Woerter aus den Wortlisten der Ziele) ---
    {"de", "Mein Raum ist viel zu dunkel", "tour:tut-karte@7|seite:k-hell"},
    {"de", "Wo stelle ich den Ordner vom Spiel ein?", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen|tour:k-spielordner@|ort:knopf:Pk3AddPath"},
    {"de", "Wie baue ich eine T\xc3\xbcr, die aufgeht?", "seite:k-tuer|tour:tut-karte-tuer"},
    {"de", "Eine Figur soll etwas sagen", "seite:k-dialog"},
    {"en", "my level is way too dark", "tour:tut-karte@7|seite:k-hell"},
    {"en", "where do I set the game folder", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen|tour:k-spielordner@|ort:knopf:Pk3AddPath"},
    {"en", "how to build a door that opens", "seite:k-tuer|tour:tut-karte-tuer"},
    {"en", "a character should say a line", "seite:k-dialog"},
    {"zh", "\xe6\x88\xbf\xe9\x97\xb4\xe5\xa4\xaa\xe6\x9a\x97\xe4\xba\x86", "tour:tut-karte@7|seite:k-hell"},
    {"zh", "\xe8\xae\xbe\xe7\xbd\xae\xe6\xb8\xb8\xe6\x88\x8f\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen|tour:k-spielordner@|ort:knopf:Pk3AddPath"},
    {"zh", "\xe6\x80\x8e\xe4\xb9\x88\xe5\x81\x9a\xe4\xb8\x80\xe6\x89\x87\xe4\xbc\x9a\xe6\x89\x93\xe5\xbc\x80\xe7\x9a\x84\xe9\x97\xa8", "seite:k-tuer"},
    {"zh", "\xe8\xae\xa9\xe8\xa7\x92\xe8\x89\xb2\xe8\xaf\xb4\xe4\xb8\x80\xe5\x8f\xa5\xe5\x8f\xb0\xe8\xaf\x8d", "seite:k-dialog"},
    {"ja", "\xe9\x83\xa8\xe5\xb1\x8b\xe3\x81\x8c\xe6\x9a\x97\xe3\x81\x99\xe3\x81\x8e\xe3\x82\x8b", "tour:tut-karte@7|seite:k-hell"},
    {"ja", "\xe3\x82\xb2\xe3\x83\xbc\xe3\x83\xa0\xe3\x81\xae\xe3\x83\x95\xe3\x82\xa9\xe3\x83\xab\xe3\x83\x80\xe3\x82\x92\xe8\xa8\xad\xe5\xae\x9a\xe3\x81\x99\xe3\x82\x8b", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen"},
    {"ja", "\xe9\x96\x8b\xe3\x81\x8f\xe3\x83\x89\xe3\x82\xa2\xe3\x81\xae\xe4\xbd\x9c\xe3\x82\x8a\xe6\x96\xb9", "seite:k-tuer"},
    {"ja", "\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xa9\xe3\x81\xab\xe3\x82\xbb\xe3\x83\xaa\xe3\x83\x95\xe3\x82\x92\xe8\xa8\x80\xe3\x82\x8f\xe3\x81\x9b\xe3\x82\x8b", "seite:k-dialog"},
};

// Unabhaengige Fragen: nach dem Abstimmen der Wortlisten geschrieben und NICHT
// daran angepasst - die ehrliche Messung (wird gezeigt, ist keine Schranke).
const Frage kUnabhaengig[] = {
    {"de", "Wie stelle ich ein, dass die Figur rennt statt geht?", "seite:k-gangart"},
    {"de", "Ich will dass eine T\xc3\xbcr aufgeht wenn der Spieler kommt", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"de", "Kann man das Programm auf Japanisch stellen?", "ort:menue:MenuView>ViewLanguage"},
    {"de", "Meine Karte hat ein Loch nach drau\xc3\x9f" "en (Leak)", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"de", "Wie schalte ich die KI wieder aus?", "seite:helfer"},
    {"de", "Musik in der Szene abspielen", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"en", "how do I make the character run instead of walk", "seite:k-gangart"},
    {"en", "open a door when the player gets close", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"en", "switch the program to Japanese", "ort:menue:MenuView>ViewLanguage"},
    {"en", "my map leaks into the void", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"en", "how do I turn the AI off again", "seite:helfer"},
    {"en", "play music during the scene", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"zh", "\xe6\x80\x8e\xe4\xb9\x88\xe8\xae\xa9\xe8\xa7\x92\xe8\x89\xb2\xe8\xb7\x91\xe8\x80\x8c\xe4\xb8\x8d\xe6\x98\xaf\xe8\xb5\xb0", "seite:k-gangart"},
    {"zh", "\xe7\x8e\xa9\xe5\xae\xb6\xe9\x9d\xa0\xe8\xbf\x91\xe6\x97\xb6\xe8\xae\xa9\xe9\x97\xa8\xe6\x89\x93\xe5\xbc\x80", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"zh", "\xe6\x8a\x8a\xe7\xa8\x8b\xe5\xba\x8f\xe5\x88\x87\xe6\x8d\xa2\xe6\x88\x90\xe6\x97\xa5\xe8\xaf\xad", "ort:menue:MenuView>ViewLanguage"},
    {"zh", "\xe6\x88\x91\xe7\x9a\x84\xe5\x9c\xb0\xe5\x9b\xbe\xe6\x9c\x89\xe6\xb3\x84\xe6\xbc\x8f", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"zh", "\xe6\x80\x8e\xe4\xb9\x88\xe5\x86\x8d\xe5\x85\xb3\xe9\x97\xad AI", "seite:helfer"},
    {"zh", "\xe5\x9c\xa8\xe5\x9c\xba\xe6\x99\xaf\xe4\xb8\xad\xe6\x92\xad\xe6\x94\xbe\xe9\x9f\xb3\xe4\xb9\x90", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"ja", "\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xa9\xe3\x82\x92\xe6\xad\xa9\xe3\x81\x8d\xe3\x81\xa7\xe3\x81\xaf\xe3\x81\xaa\xe3\x81\x8f\xe8\xb5\xb0\xe3\x82\x89\xe3\x81\x9b\xe3\x81\x9f\xe3\x81\x84", "seite:k-gangart"},
    {"ja", "\xe3\x83\x97\xe3\x83\xac\xe3\x82\xa4\xe3\x83\xa4\xe3\x83\xbc\xe3\x81\x8c\xe8\xbf\x91\xe3\x81\xa5\xe3\x81\x84\xe3\x81\x9f\xe3\x82\x89\xe3\x83\x89\xe3\x82\xa2\xe3\x82\x92\xe9\x96\x8b\xe3\x81\x91\xe3\x82\x8b", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"ja", "\xe3\x83\x97\xe3\x83\xad\xe3\x82\xb0\xe3\x83\xa9\xe3\x83\xa0\xe3\x82\x92\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x81\xab\xe3\x81\x99\xe3\x82\x8b", "ort:menue:MenuView>ViewLanguage"},
    {"ja", "\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97\xe3\x81\x8c\xe3\x83\xaa\xe3\x83\xbc\xe3\x82\xaf\xe3\x81\x97\xe3\x81\xa6\xe3\x81\x84\xe3\x82\x8b", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"ja", "AI \xe3\x82\x92\xe3\x82\x82\xe3\x81\x86\xe4\xb8\x80\xe5\xba\xa6\xe3\x82\xaa\xe3\x83\x95\xe3\x81\xab\xe3\x81\x99\xe3\x82\x8b\xe3\x81\xab\xe3\x81\xaf", "seite:helfer"},
    {"ja", "\xe3\x82\xb7\xe3\x83\xbc\xe3\x83\xb3\xe3\x81\xa7\xe9\x9f\xb3\xe6\xa5\xbd\xe3\x82\x92\xe6\xb5\x81\xe3\x81\x99", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
};

// H2: die neue unabhaengige Liste (eingefroren, siehe dort).
#include "hilfe_fragen_h2.inc"

// Weitere Sprachen (tools/sprachen.py): je Sprache eine eigene, eingefrorene
// Liste (FrageH2, mit sprache = Kurzform; {nullptr, ...} am Ende). Hat eine
// Sprache Fragen, braucht sie mindestens 24 und Top-3 >= 85 %.
#include "hilfe_fragen_es.inc"
#include "hilfe_fragen_cs.inc"

bhed::i18n::Language spracheVon(const std::string& s) {
    for (const bhed::i18n::LanguageInfo& l : bhed::i18n::languages()) {
        if (s == l.kurz) { return l.language; }
    }
    return bhed::i18n::Language::English;
}

bool passt(const std::string& id, const std::string& erwartet) {
    std::size_t a = 0;
    while (a <= erwartet.size()) {
        std::size_t e = erwartet.find('|', a);
        if (e == std::string::npos) { e = erwartet.size(); }
        const std::string v = erwartet.substr(a, e - a);
        // "tour:tut-karte" passt nur auf die Tour selbst oder "@0", nicht auf jeden
        // Schritt; "tour:x@" (mit @ am Ende) auf jeden Schritt der Tour.
        if (!v.empty() && v.back() == '@') {
            if (id == v.substr(0, v.size() - 1) || id.compare(0, v.size(), v) == 0) { return true; }
        } else if (v.find('@') == std::string::npos && v.rfind("tour:", 0) == 0) {
            if (id == v || id == v + "@0") { return true; }
        } else if (id.compare(0, v.size(), v) == 0 && (id.size() == v.size() || v.find('@') == std::string::npos || id[v.size()] < '0' || id[v.size()] > '9')) {
            return true;
        }
        a = e + 1;
    }
    return false;
}

void trefferProben(const std::string& ordner) {
    std::printf("Trefferquote (Top 3)\n");
    int gesamt = 0;
    int gut = 0;
    int unGesamt = 0;
    int unGut = 0;
    int h2Gesamt = 0;
    int h2Gut = 0;
    for (const char* sp : {"de", "en", "zh", "ja"}) {
        bhed::tour::Touren t;
        std::vector<bhed::tour::Abschnitt> hb;
        std::string f;
        const bool tOk = bhed::tour::ladeTouren(datei(ordner + "/" + sp + "/touren.json"), t, &f);
        const bool hOk = bhed::tour::ladeSuchindex(datei(ordner + "/" + sp + "/suche.js"), hb, &f);
        expect((std::string(sp) + ": touren.json und suche.js gelesen").c_str(), tOk && hOk);
        const bhed::hilfe::Index ix = bhed::hilfe::baue(t, hb, spracheVon(sp));
        int fehlend = 0;
        for (const std::string& id : bhed::hilfe::zielKennungen()) {
            if (ix.finde(id) == nullptr) {
                std::printf("    fehlt im Index (%s): %s\n", sp, id.c_str());
                ++fehlend;
            }
        }
        expect((std::string(sp) + ": jedes haeufige Ziel steht im Index").c_str(), fehlend == 0);
        int n = 0;
        int g = 0;
        for (const Frage& q : kFragen) {
            if (std::string(q.sprache) != sp) { continue; }
            ++n;
            const std::vector<bhed::hilfe::Treffer> tr = ix.frage(q.text, 3);
            bool ok = false;
            std::string liste;
            for (const bhed::hilfe::Treffer& x : tr) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q.erwartet);
                liste += " " + id;
            }
            if (ok) {
                ++g;
            } else {
                std::printf("    daneben (%s) \"%s\":%s\n", sp, q.text, liste.c_str());
            }
        }
        std::printf("    %s: %d von %d\n", sp, g, n);
        gesamt += n;
        gut += g;
        int un = 0;
        int ug = 0;
        for (const Frage& q : kUnabhaengig) {
            if (std::string(q.sprache) != sp) { continue; }
            ++un;
            bool ok = false;
            std::string liste;
            for (const bhed::hilfe::Treffer& x : ix.frage(q.text, 3)) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q.erwartet);
                liste += " " + id;
            }
            if (ok) {
                ++ug;
            } else {
                std::printf("    unabhaengig daneben (%s) \"%s\":%s\n", sp, q.text, liste.c_str());
            }
        }
        std::printf("    %s unabhaengig: %d von %d\n", sp, ug, un);
        unGesamt += un;
        unGut += ug;
        int h2 = 0;
        int h2g = 0;
        for (const FrageH2& q : kFragenH2) {
            if (std::string(q.sprache) != sp) { continue; }
            ++h2;
            bool ok = false;
            std::string liste;
            for (const bhed::hilfe::Treffer& x : ix.frage(q.text, 3, q.modus)) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q.erwartet);
                liste += " " + id;
            }
            if (ok) {
                ++h2g;
            } else {
                std::printf("    H2 daneben (%s, Modus %d) \"%s\":%s\n", sp, q.modus, q.text, liste.c_str());
            }
        }
        std::printf("    %s H2-Liste: %d von %d\n", sp, h2g, h2);
        h2Gesamt += h2;
        h2Gut += h2g;
    }
    const double quote = gesamt > 0 ? 100.0 * gut / gesamt : 0.0;
    std::printf("    gesamt: %d von %d = %.1f %%\n", gut, gesamt, quote);
    std::printf("    unabhaengig gesamt: %d von %d = %.1f %%\n", unGut, unGesamt, unGesamt > 0 ? 100.0 * unGut / unGesamt : 0.0);
    const double h2Quote = h2Gesamt > 0 ? 100.0 * h2Gut / h2Gesamt : 0.0;
    std::printf("    H2-Liste gesamt: %d von %d = %.1f %%\n", h2Gut, h2Gesamt, h2Quote);
    for (const FrageH2* liste : {kFragenEs, kFragenCs}) {
        if (liste[0].sprache == nullptr) { continue; }
        const std::string sp = liste[0].sprache;
        bhed::tour::Touren t;
        std::vector<bhed::tour::Abschnitt> hb;
        std::string f;
        const bool tOk = bhed::tour::ladeTouren(datei(ordner + "/" + sp + "/touren.json"), t, &f);
        const bool hOk = bhed::tour::ladeSuchindex(datei(ordner + "/" + sp + "/suche.js"), hb, &f);
        expect((sp + ": touren.json und suche.js gelesen").c_str(), tOk && hOk);
        const bhed::hilfe::Index ix = bhed::hilfe::baue(t, hb, spracheVon(sp));
        int n = 0;
        int g = 0;
        for (const FrageH2* q = liste; q->sprache != nullptr; ++q) {
            ++n;
            bool ok = false;
            std::string ids;
            for (const bhed::hilfe::Treffer& x : ix.frage(q->text, 3, q->modus)) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q->erwartet);
                ids += " " + id;
            }
            if (ok) {
                ++g;
            } else {
                std::printf("    daneben (%s, Modus %d) \"%s\":%s\n", sp.c_str(), q->modus, q->text, ids.c_str());
            }
        }
        const double anteil = 100.0 * g / n;
        std::printf("    %s-Liste: %d von %d = %.1f %%\n", sp.c_str(), g, n, anteil);
        expect((sp + "-Liste: mindestens 24 Fragen").c_str(), n >= 24);
        expect((sp + "-Liste: Top-3-Trefferquote >= 85 %").c_str(), anteil >= 85.0);
    }
    expect("H2-Liste: mindestens 40 Fragen", h2Gesamt >= 40);
    expect("H2-Liste: Top-3-Trefferquote >= 85 %", h2Quote >= 85.0);
    expect("mindestens 60 Fragen", gesamt >= 60);
    expect("Top-3-Trefferquote >= 85 %", quote >= 85.0);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string handbuch = argc > 1 ? argv[1] : "handbuch";
    // H2: zum Nachsehen einer einzelnen Frage: hilfetest <handbuch> --frage <sprache> <modus> "<text>"
    if (argc > 5 && std::string(argv[2]) == "--frage") {
        bhed::tour::Touren t;
        std::vector<bhed::tour::Abschnitt> hb;
        std::string f;
        (void)bhed::tour::ladeTouren(datei(handbuch + "/" + argv[3] + "/touren.json"), t, &f);
        (void)bhed::tour::ladeSuchindex(datei(handbuch + "/" + argv[3] + "/suche.js"), hb, &f);
        const bhed::hilfe::Index ix = bhed::hilfe::baue(t, hb, spracheVon(argv[3]));
        for (const bhed::hilfe::Treffer& x : ix.frage(argv[5], 8, std::atoi(argv[4]))) {
            std::printf("%8.3f  %s\n", x.wert, ix.eintraege()[x.eintrag].id.c_str());
        }
        std::printf("Vorschlag: %s\n", ix.vorschlag(argv[5]).c_str());
        return 0;
    }
    woerterProben();
    orteProben();
    trefferProben(handbuch);
    std::printf(fehler == 0 ? "\nalle Helferproben bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
