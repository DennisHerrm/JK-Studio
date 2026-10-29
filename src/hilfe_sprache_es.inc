// hilfe_sprache_es.inc - der Helfer auf Spanisch (Sprache es, tools/sprachen.py)
//
// Von Hand gepflegt, eingebunden nur von src/hilfe.cpp (kSprachDaten). Alles
// klein und gefaltet geschrieben (ohne Akzente: "camara" fuer "cámara",
// "animacion" fuer "animación") - so zerlegt src/hilfe.cpp jeden Text.
//
// STOPP: Fuellwoerter von Fragen ("como hago una ..."), zusaetzlich zu den
// deutschen und englischen (kStoppAllgemein).
// ENDUNGEN: vor den allgemeinen gestutzt, laengste zuerst, der Stamm behaelt
// mindestens vier Buchstaben (stutze): animacion, animaciones, animar,
// animado -> anim; textura, texturas -> textur.
// BEGRIFFE: spanische Woerter zu den Begriffen aus hilfe_daten.inc (gleicher
// Name; ein neuer Name ist ein neuer Begriff).
// ZIELE: haeufige Aufgaben, wie Leute auf Spanisch fragen (Kennung wie in
// kZiele).

const char* const kStoppEs[] = {
    "como", "que", "hago", "puedo", "quiero", "necesito", "deseo", "se", "un", "una", "unos", "unas", "el", "la",
    "los", "las", "lo", "de", "del", "al", "a", "en", "con", "para", "por", "mi", "mis", "tu", "tus", "su", "sus",
    "y", "o", "u", "e", "es", "son", "esta", "este", "esto", "estos", "estas", "ese", "esa", "eso", "donde", "cuando",
    "cual", "cuales", "quien", "me", "te", "le", "les", "nos", "hay", "muy", "mas", "pero", "si", "no", "ya", "algo",
    "favor", "puede", "pueden", "debo", "tengo", "hacer", "hace", "hacen", nullptr};

const char* const kEndungenEs[] = {
    "aciones", "amiento", "imiento", "adores", "ciones", "acion", "mente", "ador", "ando", "iendo", "ados", "adas", "idos",
    "idas", "cion", "ado", "ada", "ido", "ida", "ar", "er", "ir", "as", "os", "es", "a", "o", "e", "s", nullptr};

const BegriffRoh kBegriffeEs[] = {
    {nullptr, nullptr},
};

const ZielRoh kZieleEs[] = {
    {nullptr, nullptr, nullptr},
};
