// hilfe_sprache_cs.inc - der Helfer auf Tschechisch (Sprache cs, tools/sprachen.py)
//
// Von Hand gepflegt, eingebunden nur von src/hilfe.cpp (kSprachDaten). Aufbau
// wie hilfe_sprache_es.inc: alles klein und gefaltet (ohne Hacek und Akzente:
// "cestina" fuer "čeština"), Stoppwoerter, Endungen (laengste zuerst, Stamm
// mindestens vier Buchstaben), Begriffe (Name wie in hilfe_daten.inc) und
// haeufige Ziele (Kennung wie in kZiele).

const char* const kStoppCs[] = {
    nullptr};

const char* const kEndungenCs[] = {
    nullptr};

const BegriffRoh kBegriffeCs[] = {
    {nullptr, nullptr},
};

const ZielRoh kZieleCs[] = {
    {nullptr, nullptr, nullptr},
};
