# Notizen REST10: Update-Leiste wie G2C ergaenzt (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 6a4396777)

shank 09.11.: Update-Leiste "wie bei g2c programm". Vorbild: DennisHerrm/G2C-toolkit 0b00bf4, `g2c-toolkit/gui/app.cpp:3535`
`drawUpdateBanner`, Texte `g2c-toolkit/gui/i18n.cpp:783ff` (nur Quelltext gelesen, G2C nicht gestartet). Was G2C mehr hatte als
REST9 (Abgleich in `naechste_auftraege.md`), ist jetzt in `gui/update_win32.cpp` (`zeichneLeiste`, Updater).

## Punkt 1: beim Laden [Abbrechen] und "x.x / y.y MB"

- Vorher: waehrend des Ladens nur Text und Balken mit Prozent, kein Abbrechen (nur das Beenden des Programms brach ab).
- Jetzt: Balken mit "x.x / y.y MB" (wie G2C `"%.1f / %.1f MB"`, Punkt wie alle MB-Angaben im Programm; Gesamt aus
  Content-Length, sonst der Groesse aus der Release-Antwort; unbekannt: Prozent) und [Abbrechen] mit Tooltip.
  - `updater::abbrechen()`: Flagge `abbruch` (beimBeenden setzt sie mit), Laden (WinHTTP-Schleife und Simulation) und Entpacken
    hoeren auf, ein begonnener Tausch laeuft zu Ende (wie seit rc610 F78 beim Beenden). Danach Zustand zurueck auf Verfuegbar,
    nichts eingespielt, die Ablage `%TEMP%\behaved_update\<pid>` wird wie immer geloescht.
  - Nebenbei behoben: `installieren()`/`pruefen()` setzten den Zustand vor dem Belegen des Hintergrundplatzes; endete der vorige
    Auftrag (etwa ein abgebrochenes Laden) erst danach, blieb "Laedt" ohne Auftrag stehen. Jetzt erst belegen, dann setzen, dann starten.

## Punkt 2: nach dem Laden auch [Spaeter]

- [Spaeter] stand schon seit REST9 neben [Jetzt neu starten]; neu ist der passende Tooltip (wird beim naechsten Start wirksam,
  Hinweis unten bleibt) und dass die Suche von Hand die Leiste mit [Jetzt neu starten] zurueckholt (bisher kehrte `pruefen` bei
  "Fertig" ohne Wirkung zurueck, nach Spaeter blieb nur der Hinweis unten).

## Punkt 3: "Nach Updates suchen" von Hand in der Leiste

- Vorher: Hilfe > Nach Updates suchen (und Karten-Editor Help > Check for update) oeffnete das Update-Fenster.
- Jetzt (`Zustand::vonHand`, wie G2C `st.manual`): die Leiste zeigt "Suche nach Updates..." (Info, x), dann "JK Studio ist aktuell
  (rcNNN)." (Haken, [Schliessen]) oder "Suche nach Updates fehlgeschlagen: <Grund>" (Fehler, [Releases-Seite] [Schliessen]); ein
  Fund zeigt die normale Leiste - von Hand auch fuer eine uebersprungene Version und nach Spaeter (wie G2C). Beim Start bleibt eine
  Suche ohne Netz still. Das Fenster bleibt fuer "Was ist neu?" und den Hinweis in der Statuszeile.
  - [Releases-Seite] oeffnet `https://github.com/DennisHerrm/JK-Studio/releases`; im Selbsttest nur gemerkt (kein Browser), ebenso
    der Knopf im Fenster.
  - Tour k-update angepasst: Schritt 1 erledigt, sobald die Leiste steht (`zahl:update.leiste`), Schritt 2 zeigt auf die Leiste
    (`gezeichnet:update.leiste`), Schritt 4 ueber Hilfe > Aenderungen; Tutorialtexte in 6 Sprachen.

## Punkt 4: Symbole

- `UiIcon::Info` (Kreis mit i) und `UiIcon::Fehler` (Kreis mit Kreuz) im Strichstil von `uiicons.cpp`, dazu der vorhandene
  `UiIcon::Haken`. In der Leiste in deren Schriftfarbe (weiss auf dunklem, dunkelblau auf hellem Thema); die Farbe der Leiste folgt
  dem Symbol (blau, gruen, rot).

## Punkt 5: Wortwahl

- de "Diese Version überspringen" (statt "Fassung"), Tooltips "Eine neuere Version ...", "startet die neue Version"; en/zh/ja/es/cs
  sagten schon "version". Handbuchseite "Einstellungen und Updates" mitgezogen.

## Selbsttest

`gui/selbsttest_rest9.inc` erweitert (simulierte GitHub-Antwort, kein Netz; `updater::testStueckPause` macht die Simulation
langsam, das .zip bekommt fuer den Abbruchtest 3 MB Zufallsbytes):

- updateleiste: Abbrechen mitten im Laden (Balken "0.4 / 3.0 MB", Gesamt = Groesse des .zip, danach Verfuegbar, keine Datei
  eingespielt, SHA256SUMS nie geholt, kein .zip in der Ablage); Spaeter nach dem Laden (Leiste weg, Hinweis unten, kein Neustart
  vorgemerkt, Suche von Hand holt [Jetzt neu starten] zurueck); Hilfe > Nach Updates suchen ueber das Menue (Suche ..., aktuell mit
  [Schliessen] und ohne x, x waehrend der Suche, Fehler mit [Releases-Seite] - Adresse gemerkt, kein Browser - und [Schliessen], kein
  Fenster); von Hand nach Spaeter und fuer eine uebersprungene Version; Symbol je Zustand mit Kontrast >= 4.5 (gemessen 8.0 bis 13.1,
  helles und dunkles Thema, Fotos `r10_leiste_light/dark`); Wortwahl in 6 Sprachen.
- Rot (`r10_rot`, alte `update_win32.cpp` mit leeren Stubs fuer die neuen Testhaken): 31 FEHLER, alle neuen Pruefungen der Punkte 1-4.
  Punkt 5 ist eine Textaenderung (ohne eigenen Rotlauf, Pruefung liest die Texte).
- Gruen `r10_g1`: 70 OK, 1 FEHLER - "REST9 Map Editor wieder mit Leiste: 2 unruhige Bilder": die Maus stand nach den neuen Klicks
  auf einem Knopf der Leiste, dessen Tooltip mitten in der Ruhepruefung kam (auch im Rotlauf). Testfehler: `lageInTab` nimmt die
  Maus vorher weg. Danach 71/0 (Schluss).
- hilfe2 k-update (`BHED_H2_NUR=update`): 16/0, die Leiste zeigt "JK Studio is up to date (rc610)." (echte Abfrage wie bisher).

## Schluss

`Y:\werkzeuge\r10\schluss.sh`, Summe `schluss_summe.txt`:

- Sauberer Neubau (`--clean-first`) exit 0, 0 Warnungen, 896 s (allein, danach alles parallel).
- ctest 245/245 und q3map2_pr316 1/1 (darin `updater_uebergang`, `updatetest`, `updatetest_echtheit`).
- Selbsttests: karteneditor voll 3393/0, menueeditor 2787/0, editorportal 19/0, Modus 1 2635/0, modellblock funktionen 61/0 und
  md 86/0, updateleiste 71/0, updateleiste2 3/0.
- Handbuch je Sprache parallel: `BHED_HB_NUR=updateleiste` 2/0 und `BHED_HB_NUR=hilfe2` 12/0 (6 Sprachen); uebernommen nur
  `h_update_leiste` (2538 x 618) und `h_tut_update_2..4` (2538 x 1472); de und ja angesehen, nichts gekuerzt.
- Lints wie Basis 6a4396777: portability 99, pruefcodes 11, settings ok (92), i18n ok, uebrige ok bzw. unveraenderte Hinweise
  (efxflags, fremdprogramm, link/tidy nur unter Linux).
- Keine Bildzeit-Messung (Auftrag).

## Offen

- Gegen das laufende G2C nicht verglichen (Auftrag: nur Quelltext).

## Changelog

## English

- Update bar like G2C: Cancel while downloading (nothing is installed) with "x.x / y.y MB" in the progress bar; Later after the installation; Help > Check for updates shows the search, "up to date" or the error (with Releases page) in the bar instead of a window; info, tick and error symbols; "Skip this version".

## Deutsch

- Update-Leiste wie G2C: Abbrechen beim Laden (nichts wird installiert) mit "x.x / y.y MB" im Fortschrittsbalken; Später nach der Installation; Hilfe > Nach Updates suchen zeigt die Suche, "aktuell" oder den Fehler (mit Releases-Seite) in der Leiste statt in einem Fenster; Symbole für Info, Haken und Fehler; "Diese Version überspringen".

## 中文

- 更新栏与 G2C 一致：下载时可“取消”（不会安装任何内容），进度条中显示“x.x / y.y MB”；安装后也可“稍后”；“帮助 > 检查更新”在栏中显示正在检查、“已是最新版本”或错误（带“发布页面”），不再弹出窗口；信息、对勾和错误符号；“跳过此版本”。

## 日本語

- アップデートバーを G2C と同じに：ダウンロード中に「キャンセル」（何もインストールされません）、進行状況バーに「x.x / y.y MB」、インストール後に「後で」。「ヘルプ > アップデートを確認」は確認中、「最新です」、エラー（「リリースページ」付き）をウィンドウではなくバーに表示。情報・チェック・エラーの記号、「このバージョンをスキップ」。

## Español

- Barra de actualización como en G2C: Cancelar durante la descarga (no se instala nada) con "x.x / y.y MB" en la barra de progreso; Más tarde tras la instalación; Ayuda > Buscar actualizaciones muestra la búsqueda, "actualizado" o el error (con Página de versiones) en la barra en lugar de una ventana; símbolos de información, marca y error; "Omitir esta versión".

## Čeština

- Pruh aktualizace jako v G2C: Zrušit během stahování (nic se nenainstaluje) s „x.x / y.y MB“ v ukazateli průběhu; Později po instalaci; Nápověda > Vyhledat aktualizace ukáže hledání, „aktuální“ nebo chybu (se Stránkou vydání) v pruhu místo v okně; symboly informace, fajfky a chyby; „Přeskočit tuto verzi“.
