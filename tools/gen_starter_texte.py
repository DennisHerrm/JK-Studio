# -*- coding: utf-8 -*-
"""REL-TEMPO: die Texte des Starters (starter/starter_win32.cpp) in den sechs Sprachen.

shank 14.11.: "lassen wir die aktuelle exe einfach weg in Zukunft und bieten eine zentrale an und der
Rest wird automatisch geupdated" - JK-Studio-Starter.exe ist der eine Download-Link. Er soll klein
bleiben, darum bindet er die grosse Uebersetzungstabelle des Programms (src/i18n.cpp, ueber 7000
Texte) nicht ein, sondern nur diese Texte:

  - die eigenen (ZEILEN unten: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština) und
  - die Meldungen des Updaters, die der Starter mit dem Programm teilt (UEBERNOMMEN: Kennungen aus
    tools/gen_i18n.py) - wortgleich aus dem erzeugten src/i18n.cpp kopiert.

  python tools/gen_starter_texte.py           -> starter/starter_texte.inc
  python tools/gen_starter_texte.py --check   nur pruefen, ob die Datei aktuell ist (ctest starter_texte)
"""
import os
import re
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(HIER)
ZIEL = os.path.join(WURZEL, "starter", "starter_texte.inc")
SPRACHEN = ["en", "de", "zh", "ja", "es", "cs"]   # Reihenfolge wie tools/sprachen.py (Language)

ZEILEN = [
    ("StTitel", "JK Studio", "JK Studio", "JK Studio", "JK Studio", "JK Studio", "JK Studio"),
    ("StSuche",
     "Looking for the newest version of JK Studio...",
     "Suche die neueste Version von JK Studio...",
     "正在查找 JK Studio 的最新版本...",
     "JK Studio の最新バージョンを確認しています...",
     "Buscando la versión más reciente de JK Studio...",
     "Hledám nejnovější verzi JK Studia..."),
    ("StLaedtVoll",
     "Downloading JK Studio %s...",
     "Lade JK Studio %s...",
     "正在下载 JK Studio %s...",
     "JK Studio %s をダウンロードしています...",
     "Descargando JK Studio %s...",
     "Stahuji JK Studio %s..."),
    ("StLaedtDelta",
     "Downloading the update to %s - only what has changed...",
     "Lade das Update auf %s - nur, was sich geändert hat...",
     "正在下载 %s 的更新 - 仅下载有变化的部分...",
     "%s へのアップデートをダウンロードしています - 変更された部分だけ...",
     "Descargando la actualización a %s - solo lo que ha cambiado...",
     "Stahuji aktualizaci na %s - jen to, co se změnilo..."),
    ("StEinspielen",
     "Checking and installing...",
     "Prüfe und installiere...",
     "正在校验并安装...",
     "確認してインストールしています...",
     "Comprobando e instalando...",
     "Kontroluji a instaluji..."),
    ("StStartet",
     "Starting JK Studio %s...",
     "Starte JK Studio %s...",
     "正在启动 JK Studio %s...",
     "JK Studio %s を起動しています...",
     "Iniciando JK Studio %s...",
     "Spouštím JK Studio %s..."),
    ("StOrdner",
     "Folder: %s",
     "Ordner: %s",
     "文件夹: %s",
     "フォルダー: %s",
     "Carpeta: %s",
     "Složka: %s"),
    ("StMB",
     "%s / %s MB",
     "%s / %s MB",
     "%s / %s MB",
     "%s / %s MB",
     "%s / %s MB",
     "%s / %s MB"),
    ("StOffline",
     "No connection to GitHub (%s) - starting the installed version.",
     "Keine Verbindung zu GitHub (%s) - starte die installierte Version.",
     "无法连接 GitHub (%s) - 启动已安装的版本。",
     "GitHub に接続できません (%s) - インストール済みのバージョンを起動します。",
     "Sin conexión con GitHub (%s) - se inicia la versión instalada.",
     "Bez připojení ke GitHubu (%s) - spouštím nainstalovanou verzi."),
    ("StOfflineLeer",
     "No connection to GitHub: %s\nJK Studio is not installed yet. Check the internet connection and try again.",
     "Keine Verbindung zu GitHub: %s\nJK Studio ist noch nicht installiert. Prüfe die Internetverbindung und versuche es noch einmal.",
     "无法连接 GitHub: %s\nJK Studio 尚未安装。请检查网络连接后重试。",
     "GitHub に接続できません: %s\nJK Studio はまだインストールされていません。インターネット接続を確認して、もう一度お試しください。",
     "Sin conexión con GitHub: %s\nJK Studio aún no está instalado. Comprueba la conexión a internet y vuelve a intentarlo.",
     "Bez připojení ke GitHubu: %s\nJK Studio ještě není nainstalované. Zkontrolujte připojení k internetu a zkuste to znovu."),
    ("StFehler",
     "The update did not work: %s",
     "Das Update hat nicht geklappt: %s",
     "更新未成功: %s",
     "アップデートできませんでした: %s",
     "La actualización no funcionó: %s",
     "Aktualizace se nepovedla: %s"),
    ("StNichtSchreibbar",
     "JK Studio cannot be installed in %s (no write access). Move JK-Studio-Starter.exe into a folder of your own, "
     "for example Documents, and start it again.",
     "JK Studio lässt sich nicht in %s installieren (kein Schreibzugriff). Lege JK-Studio-Starter.exe in einen eigenen "
     "Ordner, zum Beispiel Dokumente, und starte ihn dort noch einmal.",
     "无法将 JK Studio 安装到 %s (没有写入权限)。请把 JK-Studio-Starter.exe 移到自己的文件夹(例如“文档”)中, 然后再次启动。",
     "%s に JK Studio をインストールできません (書き込み権限がありません)。JK-Studio-Starter.exe を自分のフォルダー "
     "(例: ドキュメント) に移動して、もう一度起動してください。",
     "JK Studio no se puede instalar en %s (sin permiso de escritura). Mueve JK-Studio-Starter.exe a una carpeta propia, "
     "por ejemplo Documentos, y vuelve a iniciarlo.",
     "JK Studio nelze nainstalovat do %s (chybí oprávnění k zápisu). Přesuňte JK-Studio-Starter.exe do vlastní složky, "
     "například Dokumenty, a spusťte ho znovu."),
    ("StStartFehler",
     "JK Studio could not be started (error %d).",
     "JK Studio ließ sich nicht starten (Fehler %d).",
     "无法启动 JK Studio (错误 %d)。",
     "JK Studio を起動できませんでした (エラー %d)。",
     "No se pudo iniciar JK Studio (error %d).",
     "JK Studio se nepodařilo spustit (chyba %d)."),
    ("StAbgebrochen",
     "Cancelled - nothing was changed.",
     "Abgebrochen - nichts wurde geändert.",
     "已取消 - 未做任何更改。",
     "キャンセルしました - 何も変更していません。",
     "Cancelado - no se cambió nada.",
     "Zrušeno - nic nebylo změněno."),
    ("StKnopfAbbrechen", "Cancel", "Abbrechen", "取消", "キャンセル", "Cancelar", "Zrušit"),
    ("StKnopfOhneUpdate",
     "Start without updating",
     "Ohne Update starten",
     "不更新直接启动",
     "アップデートせずに起動",
     "Iniciar sin actualizar",
     "Spustit bez aktualizace"),
    ("StKnopfNochmal", "Try again", "Noch einmal", "重试", "再試行", "Reintentar", "Zkusit znovu"),
    ("StKnopfStarten",
     "Start JK Studio",
     "JK Studio starten",
     "启动 JK Studio",
     "JK Studio を起動",
     "Iniciar JK Studio",
     "Spustit JK Studio"),
    ("StKnopfSchliessen", "Close", "Schließen", "关闭", "閉じる", "Cerrar", "Zavřít"),
]

# Die Meldungen des Updaters (src/update_einspielen.cpp ueber bhed::update::Meldung, dazu die der
# Anfrage an GitHub) - wortgleich wie im Programm.
UEBERNOMMEN = ["UpdErrZip", "UpdErrNoSums", "UpdErrNotListed", "UpdErrChecksum", "UpdErrSignature", "UpdErrAbort",
               "UpdErrWrite", "UpdErrReplace", "UpdErrConnect", "UpdErrNoAnswer", "UpdErrBadAnswer", "UpdNoAccess",
               "UpdNoRelease", "UpdNoZip"]


def c_text(s):
    """Ein C-Stringliteral, Nicht-ASCII als \\x-Folgen (wie src/i18n.cpp)."""
    aus = '"'
    vorher_hex = False
    for b in s.encode("utf-8"):
        c = chr(b)
        if b >= 0x80:
            aus += "\\x%02x" % b
            vorher_hex = True
            continue
        if vorher_hex and c in "0123456789abcdefABCDEF":
            aus += '" "'
        vorher_hex = False
        if c == '"':
            aus += '\\"'
        elif c == "\\":
            aus += "\\\\"
        elif c == "\n":
            aus += "\\n"
        else:
            aus += c
    return aus + '"'


def uebernommen():
    """Kennung -> Zeile '{{...}}' aus src/i18n.cpp (Reihenfolge der Spalten wie dort)."""
    zeilen = {}
    with open(os.path.join(WURZEL, "src", "i18n.cpp"), encoding="utf-8") as f:
        for z in f:
            m = re.match(r"^\s*\{\{(.*)\}\},\s*// (\w+)\s*$", z)
            if m and m.group(2) in UEBERNOMMEN:
                zeilen[m.group(2)] = m.group(1)
    fehlt = [k for k in UEBERNOMMEN if k not in zeilen]
    if fehlt:
        raise SystemExit("gen_starter_texte: in src/i18n.cpp fehlt %s" % ", ".join(fehlt))
    return zeilen


def erzeugen():
    for z in ZEILEN:
        if len(z) != 1 + len(SPRACHEN):
            raise SystemExit("gen_starter_texte: %s hat %d statt %d Sprachen" % (z[0], len(z) - 1, len(SPRACHEN)))
    alt = uebernommen()
    t = ("// Erzeugt von tools/gen_starter_texte.py - nicht von Hand aendern.\n"
         "// Die Texte des Starters (starter/starter_win32.cpp) in den Sprachen %s; die Meldungen des\n"
         "// Updaters wortgleich aus src/i18n.cpp.\n\n" % ", ".join(SPRACHEN))
    t += "enum class T {\n"
    for z in ZEILEN:
        t += "    %s,\n" % z[0]
    for k in UEBERNOMMEN:
        t += "    %s,\n" % k
    t += "    Anzahl\n};\n\n"
    t += "constexpr int kSprachen = %d;\n\n" % len(SPRACHEN)
    t += "// Je Text die Sprachen %s.\n" % ", ".join(SPRACHEN)
    t += "const char* const kTexte[static_cast<int>(T::Anzahl)][kSprachen] = {\n"
    for z in ZEILEN:
        t += "    {%s},  // %s\n" % (", ".join(c_text(s) for s in z[1:]), z[0])
    for k in UEBERNOMMEN:
        t += "    {%s},  // %s\n" % (alt[k], k)
    t += "};\n"
    return t


def main():
    text = erzeugen()
    if "--check" in sys.argv:
        try:
            with open(ZIEL, encoding="utf-8", newline="") as f:
                ist = f.read()
        except OSError:
            ist = ""
        if ist != text:
            print("starter/starter_texte.inc ist nicht aktuell - python tools/gen_starter_texte.py")
            return 1
        print("starter/starter_texte.inc aktuell (%d eigene Texte, %d aus dem Programm, %d Sprachen)" %
              (len(ZEILEN), len(UEBERNOMMEN), len(SPRACHEN)))
        return 0
    os.makedirs(os.path.dirname(ZIEL), exist_ok=True)
    with open(ZIEL, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("starter/starter_texte.inc: %d eigene Texte, %d aus dem Programm" % (len(ZEILEN), len(UEBERNOMMEN)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
