# -*- coding: utf-8 -*-
"""Die Sprachen von JK Studio - eine Liste fuer alle Erzeuger.

Gelesen von tools/gen_i18n.py (Aufzaehlung Language, Texttabelle),
tools/gen_changelog.py, tools/handbuch/ (Seiten, Suche, Touren),
tools/lint_i18n.py und tools/release_paket.py.

Die ersten vier Sprachen (ALT) stehen in jeder Zeile der Texttabelle
(`(Kennung, English, Deutsch, 中文, 日本語)` in gen_i18n.py und
tools/i18n_*.py). Jede weitere Sprache hat ihre Texte in einer eigenen
Datei, tools/i18n_sprache_<kurz>.py (`TEXTE = {Kennung: Text}`), und fuer das
Handbuch tools/handbuch/inhalt_<kurz>.py (`TEXTE = {englischer Text: Text}`).
So bleiben neue Texte anderer Zweige Ergaenzungen in vier Sprachen; was in
einer weiteren Sprache noch fehlt, zeigt das Programm englisch (nie leer),
und tools/lint_i18n.py zaehlt es.

Reihenfolge = i18n::Language (neue Sprachen nur am Ende anhaengen: die
Einstellungsdatei speichert den Code, der Changelog die Spalten in dieser
Reihenfolge).
"""
import importlib
import os
import sys

# (kurz, Code in der Einstellungsdatei / BHED_SPRACHE, Name in der eigenen
#  Sprache, Wert der Aufzaehlung i18n::Language, braucht eine CJK-Schrift)
SPRACHEN = [
    ("en", "en", "English", "English", False),
    ("de", "de", "Deutsch", "German", False),
    ("zh", "zh-Hans", "中文", "ChineseSimplified", True),
    ("ja", "ja", "日本語", "Japanese", True),
    ("es", "es", "Español", "Spanish", False),
    ("cs", "cs", "Čeština", "Czech", False),
]
ALT = 4

KURZ = tuple(s[0] for s in SPRACHEN)
NEU = KURZ[ALT:]
NAMEN = {s[0]: s[2] for s in SPRACHEN}

# Pluralformen nach CLDR (Unicode, plurals.xml; ganze Zahlen, Betrag): die
# Kategorien einer Sprache in der Reihenfolge, in der ein Zaehltext seine
# Formen schreibt ("{Datei|Dateien}", cs "{soubor|soubory|souborů}"):
#   one   n = 1          (en, de, es "1 file"; cs "1 soubor")
#   few   n = 2..4       (cs "2 soubory")
#   other alles andere   (0, 5, 11, 21 ...; zh/ja kennen nur other)
# tools/gen_i18n.py erzeugt daraus i18n::pluralForm.
MEHRZAHL = {
    "en": ("one", "other"),
    "de": ("one", "other"),
    "zh": ("other",),
    "ja": ("other",),
    "es": ("one", "other"),
    "cs": ("one", "few", "other"),
}

_HIER = os.path.dirname(os.path.abspath(__file__))


def _modul(name, ordner):
    if ordner not in sys.path:
        sys.path.insert(0, ordner)
    try:
        return importlib.import_module(name)
    except ModuleNotFoundError as e:
        if e.name != name:
            raise
        return None


def texte(kurz):
    """Die Programmtexte einer weiteren Sprache: {Kennung: Text} aus
    tools/i18n_sprache_<kurz>.py ({} solange es die Datei nicht gibt)."""
    m = _modul("i18n_sprache_" + kurz, _HIER)
    return dict(m.TEXTE) if m is not None else {}


def handbuch_texte(kurz):
    """Die Handbuchtexte einer weiteren Sprache: {englischer Text: Text} aus
    tools/handbuch/inhalt_<kurz>.py."""
    m = _modul("inhalt_" + kurz, os.path.join(_HIER, "handbuch"))
    return dict(m.TEXTE) if m is not None else {}


def handbuch_tasten(kurz):
    """Tastennamen wie auf der Tastatur des Landes ({"Shift": ...}) aus
    TASTEN in tools/handbuch/inhalt_<kurz>.py ({} = wie Englisch)."""
    m = _modul("inhalt_" + kurz, os.path.join(_HIER, "handbuch"))
    return dict(getattr(m, "TASTEN", {})) if m is not None else {}
