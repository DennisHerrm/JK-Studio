"""Grundbausteine des Handbuchs: Sprachen, Texte, feste Beschriftungen."""

import glob
import json
import os
import re
import sys

_HIER = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(_HIER))
import sprachen as _sprachen  # noqa: E402

# Alle Sprachen (tools/sprachen.py). Die ersten vier (ALT) hat jeder Text -
# im Aufruf T(en, de, zh, ja) oder in uebersetzung/<sprache>/*.json; fehlt
# einer, bricht der Bau ab. Die weiteren (NEU: es, cs) kommen aus
# inhalt_<sprache>.py (TEXTE = {englischer Text: Text}); fehlt dort einer,
# gilt vorerst der englische Text, und der Bau nennt ihn (T.fehlend_neu).
SPRACHEN = _sprachen.KURZ
ALT = SPRACHEN[:_sprachen.ALT]
NEU = _sprachen.NEU
# rc601 (shank: "mach es fuer alle 4 sprachen"): Texte, die aus Daten oder in
# Schleifen entstehen (Befehlsseiten, Knopftabellen, Tutorial-Schritte, die
# Beschreibungen aus Q3_Interface.h/behaved.bhc ...), stehen nicht als
# T(en, de, zh, ja) im Quelltext. Ihre Uebersetzungen liegen in
# uebersetzung/<sprache>/*.json - {englischer Text: Uebersetzung}. Aendert sich
# der englische Text, fehlt die Uebersetzung, und der Bau meldet es.
_TABELLE = None
benutzt = set()


def tabelle():
    global _TABELLE
    if _TABELLE is None:
        _TABELLE = {}
        for sp in SPRACHEN[1:]:
            t = {}
            for datei in sorted(glob.glob(os.path.join(_HIER, "uebersetzung", sp, "*.json"))):
                with open(datei, encoding="utf-8") as f:
                    for en, wert in json.load(f).items():
                        if en in t and t[en] != wert:
                            raise ValueError("%s: zwei Uebersetzungen fuer %r" % (datei, en))
                        if not isinstance(wert, str) or not wert.strip():
                            raise ValueError("%s: leere Uebersetzung fuer %r" % (datei, en))
                        t[en] = wert
            if sp in NEU:
                for en, wert in _sprachen.handbuch_texte(sp).items():
                    if not isinstance(wert, str) or not wert.strip():
                        raise ValueError("inhalt_%s.py: leere Uebersetzung fuer %r" % (sp, en))
                    t[en] = wert
            _TABELLE[sp] = t
    return _TABELLE


def neutral(en):
    """Ohne Woerter - nur `Code`, {{Kennung}}, [[Seite]], Zahlen, Zeichen:
    in jeder Sprache gleich."""
    rest = re.sub(r"`[^`]*`|\{\{[^}]*\}\}|\[\[[^\]]*\]\]", "", en)
    return not re.search(r"[A-Za-z]", rest)


class T:
    """Ein Text in allen Sprachen. Fehlt eine der vier ersten im Aufruf, gilt
    die aus uebersetzung/<sprache>/*.json; fehlt sie auch dort, vorerst der
    englische Text - der Bau zaehlt sie (T.fehlend, T.fehlende_texte),
    gen_handbuch.py bricht dann ab. Die weiteren Sprachen kommen immer aus
    inhalt_<sprache>.py; fehlt dort einer, gilt der englische (T.fehlend_neu,
    T.fehlende_texte_neu - kein Abbruch)."""

    fehlend = {}
    fehlende_texte = {}
    fehlend_neu = {}
    fehlende_texte_neu = {}

    def __init__(self, en, de=None, zh=None, ja=None):
        if not isinstance(en, str) or not en.strip():
            raise ValueError("Text ohne Englisch: %r" % (en,))
        self.t = {"en": en}
        for name, wert in (("de", de), ("zh", zh), ("ja", ja)):
            if wert is None:
                if neutral(en):
                    wert = en
                elif en in tabelle()[name]:
                    wert = tabelle()[name][en]
                    benutzt.add((name, en))
                else:
                    T.fehlend[name] = T.fehlend.get(name, 0) + 1
                    T.fehlende_texte.setdefault(en, set()).add(name)
                    wert = en
            elif not isinstance(wert, str) or not wert.strip():
                raise ValueError("Text ohne %s: %r" % (name, en))
            self.t[name] = wert
        for name in NEU:
            if neutral(en):
                wert = en
            elif en in tabelle()[name]:
                wert = tabelle()[name][en]
                benutzt.add((name, en))
            else:
                T.fehlend_neu[name] = T.fehlend_neu.get(name, 0) + 1
                T.fehlende_texte_neu.setdefault(name, []).append(en)
                wert = en
            self.t[name] = wert

    def get(self, sp):
        return self.t[sp]


def formatiere(t, *werte):
    """Ein T mit %s/%d, je Sprache mit denselben Werten gefuellt."""
    aus = T.__new__(T)
    aus.t = {sp: t.get(sp) % werte for sp in SPRACHEN}
    return aus


def verbinde(*teile):
    """Ein T aus Teilen (T oder feste Zeichenkette), je Sprache aneinander
    gehaengt - statt T(a.get("en") + x, a.get("de") + x, ...), das die
    weiteren Sprachen nicht kennt."""
    aus = T.__new__(T)
    aus.t = {sp: "".join(x.get(sp) if isinstance(x, T) else x for x in teile) for sp in SPRACHEN}
    return aus


def Tf(vorlage, *werte):
    """T aus einer Vorlage mit %s/%d - uebersetzt wird die Vorlage."""
    return formatiere(T(vorlage), *werte)


def tasten_namen(sp):
    """Tastennamen wie auf der Tastatur des Landes (Deutsch: Strg, Umschalt
    ...; die weiteren Sprachen: TASTEN in inhalt_<sprache>.py)."""
    if sp == "de":
        return {"Ctrl": "Strg", "Shift": "Umschalt", "Space": "Leertaste", "Home": "Pos1", "End": "Ende",
                "Delete": "Entf", "Insert": "Einfg", "Backspace": "Rücktaste", "Up": "Oben", "Down": "Unten"}
    if sp in NEU:
        return _sprachen.handbuch_tasten(sp)
    return {}


_UI = {
    "guide": T("Guide", "Handbuch", "手册", "ガイド"),
    "suchen": T("Search the guide...  (/)", "Im Handbuch suchen ...  (/)", "搜索手册...  (/)", "ガイドを検索...  (/)"),
    "thema": T("Light or dark", "Hell oder dunkel", "浅色或深色", "ライト / ダーク"),
    "zurueck": T("Previous", "Zurück", "上一页", "前へ"),
    "weiter": T("Next", "Weiter", "下一页", "次へ"),
    "animation": T("Animation", "Animation", "动画", "アニメーション"),
    "pause": T("Pause", "Anhalten", "暂停", "一時停止"),
    "ansicht": T("View:", "Ansicht:", "视图:", "ビュー:"),
    "keine_treffer": T("Nothing found.", "Nichts gefunden.", "未找到结果。", "見つかりませんでした。"),
    "treffer_auf": T("on", "auf", "位于", "ページ:"),
    "bild_englisch": T("Picture still in English", "Bild noch auf Englisch", "图片仍为英文", "画像はまだ英語です"),
    "legende_nr": T("No.", "Nr.", "编号", "番号"),
    "legende_name": T("Name", "Name", "名称", "名前"),
    "legende_was": T("What it does", "Was es macht", "作用", "機能"),
    "fuss": T("JK Studio guide - mod tools for Jedi Knight: Jedi Academy and Jedi Outcast. The pictures show the program itself; they are made by its self-test. Not affiliated with Lucasfilm, Disney or Raven Software.",
              "Handbuch zu JK Studio - Mod-Werkzeuge für Jedi Knight: Jedi Academy und Jedi Outcast. Die Bilder zeigen das Programm selbst; sein Selbsttest nimmt sie auf. Nicht verbunden mit Lucasfilm, Disney oder Raven Software.",
              "JK Studio 手册——Jedi Knight: Jedi Academy 与 Jedi Outcast 的模组工具。图片展示的是程序本身,由其自检功能截取。与 Lucasfilm、Disney 或 Raven Software 无关联。",
              "JK Studio ガイド - Jedi Knight: Jedi Academy と Jedi Outcast 向けの MOD ツール。画像はプログラム自身の画面で、セルフテストが撮影しています。Lucasfilm、Disney、Raven Software とは関係ありません。"),
}


def ui(schluessel, sp):
    return _UI[schluessel].get(sp)
