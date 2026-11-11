# -*- coding: utf-8 -*-
"""Oberflaechentexte von GDPFAD (rc612: ein eingerichteter Spielordner, den es nicht mehr gibt).

Statuszeile nach dem Einlesen der Spielordner (gui/app_ablage.inc rescanGamePaths), das Fenster Edit > Game folders
(gui/app.cpp drawGamePaths) und das Fenster "Files in .pk3" (gui/app.cpp). Je Zeile: Kennung, English, Deutsch, 中文, 日本語,
Español, Čeština.
"""

ZEILEN = [
    # Statuszeile, einmal je Ordner; %s = der Ordner aus den Einstellungen
    ("GdOrdnerFehlt", "Game folder not found: %s - check Edit > Game folders",
     "Spielordner nicht gefunden: %s - siehe Bearbeiten > Spielordner",
     "未找到游戏目录：%s — 请检查 编辑 > 游戏目录",
     "ゲームフォルダーが見つかりません: %s — 編集 > ゲームフォルダー を確認してください",
     "No se encontró la carpeta del juego: %s - revisa Edición > Carpetas del juego",
     "Složka hry nenalezena: %s - zkontrolujte Úpravy > Složky hry"),
    # Hinter dem Ordner im Fenster Game folders (statt "no archives found - is the path right?", wenn es den Ordner nicht gibt)
    ("GdOrdnerWeg", "folder not found - was it renamed or moved?", "Ordner nicht gefunden - umbenannt oder verschoben?",
     "未找到文件夹——是否已重命名或移动？", "フォルダーが見つかりません — 名前の変更または移動をしましたか？",
     "no se encontró la carpeta - ¿se renombró o se movió?", "složka nenalezena - byla přejmenována nebo přesunuta?"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
