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

    // uebersetzung1: Pluralformen nach CLDR. Tschechisch hat drei (1 soubor,
    // 2-4 soubory, 5+ souboru), Englisch, Deutsch, Spanisch zwei, Chinesisch
    // und Japanisch eine; vorher stand "%d souboru" auch fuer 2 bis 4.
    {
        using bhed::i18n::Str;
        using bhed::i18n::pluralForm;
        erwarte("CLDR cs: 1 one, 2..4 few, 0/5/11/21/22/100 other, -3 few",
                pluralForm(L::Czech, 1) == 0 && pluralForm(L::Czech, 2) == 1 && pluralForm(L::Czech, 4) == 1 &&
                    pluralForm(L::Czech, 0) == 2 && pluralForm(L::Czech, 5) == 2 && pluralForm(L::Czech, 11) == 2 &&
                    pluralForm(L::Czech, 21) == 2 && pluralForm(L::Czech, 22) == 2 && pluralForm(L::Czech, 100) == 2 &&
                    pluralForm(L::Czech, -3) == 1,
                true);
        bool zwei = true;
        for (const L l : {L::English, L::German, L::Spanish}) {
            zwei = zwei && bhed::i18n::pluralFormen(l) == 2 && pluralForm(l, 1) == 0 && pluralForm(l, 0) == 1 &&
                   pluralForm(l, 2) == 1 && pluralForm(l, 21) == 1 && pluralForm(l, -1) == 0;
        }
        erwarte("CLDR en/de/es: 1 one, sonst other", zwei, true);
        erwarte("CLDR zh/ja: nur other",
                bhed::i18n::pluralFormen(L::ChineseSimplified) == 1 && bhed::i18n::pluralFormen(L::Japanese) == 1 &&
                    pluralForm(L::ChineseSimplified, 1) == 0 && pluralForm(L::Japanese, 3) == 0 &&
                    bhed::i18n::pluralFormen(L::Czech) == 3,
                true);

        auto mit = [](L l, Str id, long long n) {
            char b[256];
            std::snprintf(b, sizeof(b), bhed::i18n::trNIn(l, id, n), static_cast<int>(n));
            return std::string(b);
        };
        erwarte("en: 1 command copied / 2 commands copied",
                mit(L::English, Str::MsgCopied, 1) == "1 command copied" &&
                    mit(L::English, Str::MsgCopied, 2) == "2 commands copied",
                true);
        erwarte("de: 1 Befehl kopiert / 5 Befehle kopiert",
                mit(L::German, Str::MsgCopied, 1) == "1 Befehl kopiert" &&
                    mit(L::German, Str::MsgCopied, 5) == "5 Befehle kopiert",
                true);
        erwarte("es: 1 comando copiado / 0 comandos copiados",
                mit(L::Spanish, Str::MsgCopied, 1) == "1 comando copiado" &&
                    mit(L::Spanish, Str::MsgCopied, 0) == "0 comandos copiados",
                true);
        erwarte("cs: 1 prikaz, 3 prikazy, 5/0/22 prikazu (mit Verbform)",
                mit(L::Czech, Str::MsgCopied, 1) == "Zkop\xc3\xadrov\xc3\xa1n 1 p\xc5\x99\xc3\xadkaz" &&
                    mit(L::Czech, Str::MsgCopied, 3) == "Zkop\xc3\xadrov\xc3\xa1ny 3 p\xc5\x99\xc3\xadkazy" &&
                    mit(L::Czech, Str::MsgCopied, 5) == "Zkop\xc3\xadrov\xc3\xa1no 5 p\xc5\x99\xc3\xadkaz\xc5\xaf" &&
                    mit(L::Czech, Str::MsgCopied, 0) == "Zkop\xc3\xadrov\xc3\xa1no 0 p\xc5\x99\xc3\xadkaz\xc5\xaf" &&
                    mit(L::Czech, Str::MsgCopied, 22) == "Zkop\xc3\xadrov\xc3\xa1no 22 p\xc5\x99\xc3\xadkaz\xc5\xaf",
                true);
        erwarte("zh/ja: eine Form, trN == tr",
                std::string(bhed::i18n::trNIn(L::ChineseSimplified, Str::MsgCopied, 1)) ==
                        bhed::i18n::trIn(L::ChineseSimplified, Str::MsgCopied) &&
                    std::string(bhed::i18n::trNIn(L::Japanese, Str::MsgCopied, 7)) ==
                        bhed::i18n::trIn(L::Japanese, Str::MsgCopied),
                true);
        {
            char b[256];
            std::snprintf(b, sizeof(b), bhed::i18n::trNIn(L::English, Str::CompileOk, 1, 2), 1, 2);
            const std::string en = b;
            std::snprintf(b, sizeof(b), bhed::i18n::trNIn(L::Czech, Str::CompileOk, 2, 5), 2, 5);
            const std::string cs = b;
            erwarte("zwei Zahlen: en 1 block, 2 bytes; cs 2 bloky, 5 bajtu",
                    en == "Compiled: 1 block, 2 bytes" && cs == "Zkompilov\xc3\xa1no: 2 bloky, 5 bajt\xc5\xaf", true);
        }
        // Jeder Zaehltext in jeder Sprache und fuer jede Form: keine Klammer
        // bleibt stehen, dieselben Platzhalter wie Englisch, tr() = other.
        auto platzhalter = [](const std::string& t) {
            std::string p;
            for (std::size_t i = 0; i + 1 < t.size(); ++i) {
                if (t[i] != '%') { continue; }
                std::size_t j = i + 1;
                while (j < t.size() && std::string("-+ #0123456789.hlLzjt").find(t[j]) != std::string::npos) { ++j; }
                if (j < t.size()) { p += t[j]; }
                i = j;
            }
            return p;
        };
        int zaehl = 0;
        bool ok = true;
        for (int i = 0; i < static_cast<int>(Str::Count); ++i) {
            const auto id = static_cast<Str>(i);
            if (!bhed::i18n::istZaehltext(id)) { continue; }
            ++zaehl;
            const std::string soll = platzhalter(bhed::i18n::trIn(L::English, id));
            for (const bhed::i18n::LanguageInfo& l : bhed::i18n::languages()) {
                for (const long long n : {0LL, 1LL, 2LL, 3LL, 5LL, 21LL}) {
                    const std::string t = bhed::i18n::trNIn(l.language, id, n, n, n, n, n);
                    const std::size_t a = t.find('{');
                    if ((a != std::string::npos && t.find('|', a) != std::string::npos) || platzhalter(t) != soll) {
                        std::printf("  Zaehltext %s (%s, %lld): %s\n", bhed::i18n::strName(id), l.kurz, n, t.c_str());
                        ok = false;
                    }
                }
                ok = ok && std::string(bhed::i18n::trIn(l.language, id)) == bhed::i18n::trNIn(l.language, id, 5, 5, 5, 5, 5);
            }
        }
        std::printf("  (%d Zaehltexte)\n", zaehl);
        erwarte("jeder Zaehltext: alle Formen ohne Klammerrest, Platzhalter wie Englisch, tr() = other",
                ok && zaehl > 0 && !bhed::i18n::istZaehltext(Str::FileSave), true);
    }

    if (g_fehl != 0) {
        std::printf("Sprach-Proben: %d Fehlschlag(e)\n", g_fehl);
        return 1;
    }
    std::printf("Sprach-Proben bestanden (0 Fehlschlaege)\n");
    return 0;
}
