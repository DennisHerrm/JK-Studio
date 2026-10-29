// Welche Sprache gewinnt beim Start?
//
// shank arbeitet auf `Gebiet de-DE`, und in seinem rc539-Protokoll stand
// die Statuszeile trotzdem englisch:
//
//     Statuszeile: Dropped where it already was - nothing to do
//
// Die Kette beim Start:
//     setLanguage(fromSystemLocale(systemLocale()))   <- Gebietsschema
//     loadSettingsFile(...)                           <- kann ueberschreiben
//     if (findLanguage(settings.language)) setLanguage(...)
//
// `Settings::language` hatte die Vorgabe "en". Damit war "noch nichts
// gewaehlt" nicht von "Englisch gewaehlt" zu unterscheiden - und fehlte
// der Schluessel in der Einstellungsdatei, gewann die Vorgabe und machte
// die Gebietserkennung wirkungslos.
#include "bhed/changelog.h"
#include "bhed/i18n.h"
#include "bhed/settings.h"

#include <cstddef>
#include <cstdio>
#include <string>

static int g_fehl = 0;

static void erwarte(const char* was, bool ist, bool soll) {
    if (ist == soll) { std::printf("  ok    %s\n", was); return; }
    std::printf("  FEHL  %s (erwartet %s)\n", was, soll ? "ja" : "nein");
    ++g_fehl;
}

// Genau die Kette aus main_win32.cpp und loadSettings().
static bhed::i18n::Language starte(const std::string& gebiet,
                                   const std::string& ausDerDatei) {
    bhed::i18n::setLanguage(bhed::i18n::fromSystemLocale(gebiet));
    bhed::Settings s;                    // Vorgaben
    if (!ausDerDatei.empty()) {
        s.language = ausDerDatei;        // stand so in der Datei
    }
    if (const bhed::i18n::LanguageInfo* l = bhed::i18n::findLanguage(s.language)) {
        bhed::i18n::setLanguage(l->language);
    }
    return bhed::i18n::currentLanguage();
}

int main() {
    using L = bhed::i18n::Language;

    // --- Das ist der Fall, der schiefging -----------------------------
    erwarte("de-DE ohne Eintrag in der Datei -> Deutsch",
            starte("de-DE", "") == L::German, true);
    erwarte("ja-JP ohne Eintrag -> Japanisch",
            starte("ja-JP", "") == L::Japanese, true);
    erwarte("fr-FR ohne Eintrag -> Englisch (haben wir nicht)",
            starte("fr-FR", "") == L::English, true);

    // --- Eine ausdrueckliche Wahl gewinnt weiterhin -------------------
    erwarte("de-DE, aber \"en\" gewaehlt -> Englisch",
            starte("de-DE", "en") == L::English, true);
    erwarte("en-US, aber \"de\" gewaehlt -> Deutsch",
            starte("en-US", "de") == L::German, true);

    // --- Die Vorgabe selbst -------------------------------------------
    {
        bhed::Settings s;
        erwarte("Settings::language ist ab Werk LEER", s.language.empty(), true);
        erwarte("und leer findet keine Sprache",
                bhed::i18n::findLanguage(s.language) == nullptr, true);
    }

    // --- Unsinn in der Datei darf das Gebietsschema nicht kippen ------
    erwarte("de-DE mit unbekanntem Code -> Deutsch bleibt",
            starte("de-DE", "kl-KL") == L::German, true);

    // --- Weitere Sprachen (Spanisch, Tschechisch) -----------------------
    //
    // Am Ende der Aufzaehlung angehaengt: die bisherigen Werte bleiben (die
    // Einstellungsdatei speichert den Code, der Changelog die Spalte).
    erwarte("Englisch, Deutsch, Chinesisch, Japanisch behalten 0..3",
            static_cast<int>(L::English) == 0 && static_cast<int>(L::German) == 1 &&
                static_cast<int>(L::ChineseSimplified) == 2 && static_cast<int>(L::Japanese) == 3,
            true);
    erwarte("es-ES ohne Eintrag -> Spanisch", starte("es-ES", "") == L::Spanish, true);
    erwarte("es-MX ohne Eintrag -> Spanisch", starte("es-MX", "") == L::Spanish, true);
    erwarte("es-419 ohne Eintrag -> Spanisch", starte("es-419", "") == L::Spanish, true);
    erwarte("cs-CZ ohne Eintrag -> Tschechisch", starte("cs-CZ", "") == L::Czech, true);
    erwarte("sk-SK ohne Eintrag -> Englisch (Slowakisch haben wir nicht)", starte("sk-SK", "") == L::English, true);
    erwarte("de-DE, aber \"es\" gewaehlt -> Spanisch", starte("de-DE", "es") == L::Spanish, true);
    erwarte("es-ES, aber \"cs\" gewaehlt -> Tschechisch", starte("es-ES", "cs") == L::Czech, true);
    erwarte("es-ES, aber \"en\" gewaehlt -> Englisch", starte("es-ES", "en") == L::English, true);
    {
        const char* const soll[] = {"en", "de", "zh", "ja", "es", "cs"};
        const auto& alle = bhed::i18n::languages();
        bool kurzOk = alle.size() == 6;
        for (std::size_t i = 0; kurzOk && i < alle.size(); ++i) {
            kurzOk = static_cast<std::size_t>(alle[i].language) == i && std::string(alle[i].kurz) == soll[i] &&
                     std::string(bhed::i18n::kurzCode(alle[i].language)) == soll[i] && alle[i].nativeName[0] != 0;
        }
        erwarte("sechs Sprachen in Reihenfolge der Aufzaehlung, Kurzformen en de zh ja es cs", kurzOk, true);
        erwarte("Spanisch heisst im Menue Espanol (mit Tilde)",
                std::string(bhed::i18n::findLanguage("es")->nativeName) == "Espa\xc3\xb1ol", true);
        erwarte("Tschechisch heisst im Menue Cestina (mit Hacek)",
                std::string(bhed::i18n::findLanguage("cs")->nativeName) == "\xc4\x8c" "e\xc5\xa1tina", true);
    }
    // Fehlt einer weiteren Sprache ein Text (neue Texte anderer Zweige kommen
    // nur in vier Sprachen), zeigt das Programm den englischen - nie einen
    // leeren.
    {
        using bhed::i18n::Str;
        int eigen = 0;
        int englisch = 0;
        bool ok = true;
        for (const L l : {L::Spanish, L::Czech}) {
            for (int i = 0; i < static_cast<int>(Str::Count); ++i) {
                const auto id = static_cast<Str>(i);
                const std::string t = bhed::i18n::trIn(l, id);
                if (t.empty()) { ok = false; }
                if (bhed::i18n::hatEigenenText(l, id)) {
                    ++eigen;
                } else {
                    ++englisch;
                    if (t != bhed::i18n::trIn(L::English, id)) { ok = false; }
                }
            }
        }
        std::printf("  (es/cs: %d eigene Texte, %d englisch)\n", eigen, englisch);
        erwarte("es/cs: jeder Text da, ohne eigenen genau der englische", ok, true);
        erwarte("Englisch hat immer einen eigenen Text, eine Sprache ausserhalb der Liste nie",
                bhed::i18n::hatEigenenText(L::English, Str::AppTitle) &&
                    !bhed::i18n::hatEigenenText(static_cast<L>(99), Str::AppTitle) &&
                    std::string(bhed::i18n::trIn(static_cast<L>(99), Str::FileSave)) ==
                        bhed::i18n::trIn(L::English, Str::FileSave),
                true);
        bhed::i18n::setLanguage(L::Spanish);
        erwarte("tr() in Spanisch liefert nie leer", std::string(bhed::i18n::tr(Str::FileSave)).empty(), false);
        bhed::i18n::setLanguage(L::English);
    }
    // Changelog: alte Fassungen haben nur die vier ersten Sprachen - das
    // Fenster zeigt fuer Spanisch und Tschechisch dann den englischen Text.
    {
        bool ok = bhed::changelog::anzahl() > 0;
        for (std::size_t i = 0; ok && i < bhed::changelog::anzahl(); ++i) {
            const bhed::changelog::Eintrag& e = bhed::changelog::eintraege()[i];
            for (const bhed::i18n::LanguageInfo& l : bhed::i18n::languages()) {
                const int s = static_cast<int>(l.language);
                const char* t = bhed::changelog::text(e, s);
                const bool soll = e.text[s] != nullptr ? t == e.text[s] : t == e.text[0];
                ok = ok && t != nullptr && t[0] != 0 && soll;
            }
            ok = ok && bhed::changelog::text(e, 99) == e.text[0] && bhed::changelog::text(e, -1) == e.text[0];
        }
        erwarte("Changelog: jede Fassung in jeder Sprache, ohne eigene Spalte englisch", ok, true);
    }

    if (g_fehl != 0) {
        std::printf("Sprach-Proben: %d Fehlschlag(e)\n", g_fehl);
        return 1;
    }
    std::printf("Sprach-Proben bestanden (0 Fehlschlaege)\n");
    return 0;
}
