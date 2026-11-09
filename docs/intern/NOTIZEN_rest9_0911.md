# Notizen REST9: Update-Leiste oben, wie im Programm G2C (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 0997506cb)

shank 09.11.: "kannst du nicht einbauen wie bei g2c programm das wenn man es startet oben eine Leiste erscheint mit neues update
verfuegbar und jetzt updaten? ... dass man gleich gewarnt wird" - "ja bitte" (in rc611, damit die Leiste beim Erscheinen von rc612
schon beim Nutzer ist).

## Punkt 1: die Leiste

- Vorher: ein Fund der Suche beim Start (`updater::pruefen(true)`) stand nur als kleiner gruener Text unten in der Statuszeile
  (`zeichneStatus`), erst der Klick oeffnete das Fenster.
- Jetzt (`gui/update_win32.cpp`, `zeichneLeiste`, aus `fensterKopf` direkt nach der Menueleiste, nur im Hauptfenster): eine farbige
  Leiste ueber die ganze Fensterbreite, blau (verfuegbar, laedt), gruen (installiert), rot (Fehler beim Installieren); helle Farben
  auf hellem Thema, dunkle auf dunklem (Helligkeit von `ImGuiCol_WindowBg`).
  - Verfuegbar: "Neues Update rcNNN verfuegbar (installiert: rcMMM)", [Jetzt aktualisieren] (gruen wie der Compile-Knopf,
    vorhandenes `installieren()`), [Was ist neu?] (vorhandenes Fenster mit den Release-Notizen und dem Changelog-Knopf), [Spaeter],
    [Diese Fassung ueberspringen], [x].
  - Laedt: Text und Fortschrittsbalken mit Prozent in der Leiste.
  - Fertig: [Jetzt neu starten] (vorhandenes `neuStarten()`, fragt nach ungesicherten Reitern wie Exit), [Was ist neu?], [Spaeter].
  - Fehler beim Installieren (`Zustand::beimInstallieren`): der Grund und [Erneut versuchen]. Ein gescheitertes Nachsehen (keine
    Verbindung) zeigt keine Leiste - nur das Fenster, wenn man selbst gesucht hat.
  - Spaeter/x: bis zum naechsten Start weg (`leisteWegTag`, gilt fuer dieses Release; ein noch neueres in derselben Sitzung zeigt sie
    wieder). Der Hinweis in der Statuszeile bleibt.
  - Diese Fassung ueberspringen: Einstellung `updateUebersprungen` (Tag des Releases, in `behaved.cfg`); gemeldet wird erst wieder
    ein neueres Release (`istNeuer`). Der Hinweis in der Statuszeile bleibt auch hier.
  - Jede Schaltflaeche hat einen Tooltip (was sie tut). Passen Text und Knoepfe nicht in eine Zeile, stehen die Knoepfe in einer
    zweiten; ein zu langer Text (Fehlermeldung) endet mit "..." und steht ganz im Tooltip - abgeschnitten wird nichts.
- Lage: alles darunter rueckt um die Hoehe der Leiste nach unten. Karten- und Menue-Editor setzten sich bisher selbst an die
  Unterkante der Menueleiste (`drawKartenEditorBereich`, `drawMenueEditorBereich`, `Editor::leisteUnten`) und haetten die Leiste
  verdeckt (im Selbsttest so gefunden: Map Editor Modusleiste bei 41 statt unter 78, Menu Editor bei 35) - jetzt
  `unterDerMenueleiste()` bzw. `karteneditor::leisteUntenVerschieben`. Eigene Fenster der Tabs (rc609 T1) haben keine Leiste.
- Texte in 6 Sprachen: `tools/i18n_rest9.py` (15 Kennungen `UpdBar*`).

## Punkt 2: Selbsttest

`gui/selbsttest_rest9.inc`, ohne Netz: `updater::testNetzSetzen` ersetzt die Antworten von GitHub (wie `tests/uebergang` mit
`g_simApi`) - `/releases/latest` als JSON mit Tag nach Wahl, ein echtes .zip (`writePk3`, eine Datei `r9_update_probe.txt`) und
`SHA256SUMS.txt`; danach laeuft derselbe Code wie mit dem Netz (Fortschritt in Stuecken, Pruefung, Entpacken, Tausch).

- `BHED_EDITORTEST=updateleiste` (48 Pruefungen): aelteres/gleiches Release keine Leiste; neueres Leiste unter der Menueleiste mit
  beiden Fassungen, kein Fenster, Hinweis unten; Tooltips aller fuenf Knoepfe; schmal zwei Zeilen, alle Knoepfe ganz sichtbar;
  Was ist neu? oeffnet das Fenster mit Notizen; Spaeter weg, Hinweis bleibt, Arbeitsbereich hoch, nochmal suchen zeigt sie nicht,
  nach neuem Start wieder; x ebenso; ueberspringen haelt ueber einen neuen Start, ein noch neueres zeigt sie wieder; in allen fuenf
  Tabs: Leiste direkt unter der Menueleiste, Inhalt genau um ihre Hoehe tiefer (46 Punkte), nichts verdeckt, nach dem Schliessen
  zurueck (Map Editor zusaetzlich `editorOben`); Jetzt aktualisieren zeigt das Laden, ein kaputtes .zip den Fehler (Pruefsumme) mit
  Erneut versuchen und keine Datei eingespielt, Erneut versuchen installiert (Datei mit Inhalt neben der .exe im Laufordner), Jetzt
  neu starten merkt den Neustart vor und setzt wantQuit (im selben Bild zurueckgenommen); ohne Netz keine Leiste; der Helfer findet
  "Nach Updates suchen".
- `BHED_EDITORTEST=updateleiste2` mit `CFG=<einst/behaved.cfg des ersten Laufs>` (echter Neustart, Skript
  `Y:\werkzeuge\r9\leiste.sh`): die uebersprungene Fassung steht in den Einstellungen und zeigt keine Leiste (Hinweis unten da),
  eine neuere zeigt sie (3 Pruefungen).
- Rot: dieselbe Fassung, nur ohne Aufruf der Leiste in `fensterKopf` - 35 FEHLER (`r9_rot`). Erster gruener Lauf 3 FEHLER: Pruefung
  "Was ist neu?" (Text statt Fenster gesucht, Test berichtigt) und Map-/Menu-Editor verdecken die Leiste (Programm berichtigt, s. o.).
  Danach 48/0 und 3/0 (`r9_g3`, `r9_g3b`).

## Punkt 3: Handbuch und Changelog

- Seite "Einstellungen und Updates" (`einstellungen`), Abschnitt Updates: Absatz zur Leiste und Bild `h_update_leiste`
  (`tools/handbuch/inhalt_rest9.py`, 6 Sprachen). Bild aus `BHED_EDITORTEST=handbuch BHED_HB_NUR=updateleiste`, je Sprache parallel,
  Fenster 2538 x 1384 bei Oberflaeche x2, Ausschnitt 2538 x 618, Leiste gelb markiert; de, ja, cs angesehen (eine Zeile, nichts
  gekuerzt).

## Schluss (`Y:\werkzeuge\r9\schluss.sh`, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) exit 0, 0 Warnungen, 1550 s (allein, danach alles parallel).
- ctest 245/245 und q3map2_pr316 1/1 (darin `updater_uebergang`, `updatetest`, `updatetest_echtheit`).
- Selbsttests: karteneditor voll 3393/0, menueeditor 2787/0, editorportal 19/0, Modus 1 2635/0, modellblock funktionen 61/0 und
  md 86/0, updateleiste 48/0, updateleiste2 3/0.
- Lints wie Basis 0997506cb: portability 99 (Basis 100), pruefcodes 11 (11), settings ok mit 92 Schluesseln (neu
  `updateUebersprungen`), uebrige ok bzw. unveraenderte Hinweise (efxflags, fremdprogramm, link/tidy nur unter Linux).
- Keine Bildzeit-Messung (Auftrag). Adressen 0 ab 0997506cb, Autor und Trailer in jedem Commit.

## Offen

- Gegen G2C selbst nicht verglichen (das Programm liegt nicht vor); Vorbild ist shanks Beschreibung.
- Die echte Abfrage bei GitHub bleibt im Selbsttest `update` (nicht Teil dieses Auftrags, braucht Netz und eine Kopie des
  Programmordners).

## Changelog

## English

- Update bar: when a newer version is found, a coloured bar appears under the menu bar, above all tabs, with Update now (progress in the bar, then Restart now), What's new?, Later, Skip this version and x; a failed install shows the reason and Try again. The work area moves down while the bar is there.

## Deutsch

- Update-Leiste: Ist eine neuere Fassung gefunden, erscheint unter der Menueleiste, ueber allen Reitern, eine farbige Leiste mit Jetzt aktualisieren (Fortschritt in der Leiste, danach Jetzt neu starten), Was ist neu?, Spaeter, Diese Fassung ueberspringen und x; ein gescheitertes Installieren nennt den Grund und bietet Erneut versuchen. Der Arbeitsbereich rueckt nach unten, solange die Leiste steht.

## 中文

- 更新栏：发现新版本时，菜单栏下方、所有标签页上方会出现一条彩色栏，包含“立即更新”（栏内显示进度，之后“立即重新启动”）、“有什么新内容?”、“稍后”、“跳过此版本”和 x；安装失败时会说明原因并提供“重试”。更新栏显示时工作区下移。

## 日本語

- アップデートバー：新しいバージョンが見つかると、メニューバーの下、すべてのタブの上に色付きのバーが表示されます。「今すぐ更新」（バー内に進行状況、その後「今すぐ再起動」）、「新機能は?」、「後で」、「このバージョンをスキップ」、x があります。インストールに失敗すると理由を示し「再試行」を提示します。バーがある間は作業領域が下にずれます。

## Español

- Barra de actualización: cuando se encuentra una versión más nueva, aparece bajo la barra de menús, encima de todas las pestañas, una barra de color con Actualizar ahora (progreso en la barra y después Reiniciar ahora), ¿Qué hay de nuevo?, Más tarde, Omitir esta versión y x; si la instalación falla, indica el motivo y ofrece Reintentar. El área de trabajo baja mientras la barra está visible.

## Čeština

- Pruh aktualizace: když se najde novější verze, objeví se pod nabídkovým řádkem, nad všemi kartami, barevný pruh s tlačítky Aktualizovat nyní (průběh v pruhu, potom Restartovat nyní), Co je nového?, Později, Přeskočit tuto verzi a x; neúspěšná instalace uvede důvod a nabídne Zkusit znovu. Pracovní plocha se posune dolů, dokud je pruh vidět.
