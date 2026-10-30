# Notizen uebersetzung2: Spanisch und Tschechisch für die Texte nach Integration 4

Zweig `uebersetzung2` ab karte1 b6309500 (Integration 4), Worktree `Y:\sprache_es`, Bau `build-sprache`.
Auftrag: SP\auftrag_uebersetzung2.md. Skripte: `SP/u2/` (`fehlend.py` fehlende Programmtexte mit Quelle,
`hb_fehlend.py` fehlende/unbenutzte Handbuchtexte, `einfuegen.py`/`hb_merge.py` Blöcke eintragen,
`idle.sh` + `start_idle.ps1` Selbsttest mit Idle-Priorität über CreateProcess, `bilder.sh` Bildsätze),
Laufordner `Y:/es_arbeit/u2/laeufe`. Parallel arbeiteten andere Agenten (alle Zeiten unter Last).

| Commit | Inhalt |
|---|---|
| fcc10b4c | Punkt 1+2: 354 Programmtexte je Sprache, Zähltexte, lint_i18n 5 → 0 |
| 78e322b2 | Punkt 1: 148 Handbuchtexte je Sprache, 33 unbenutzte Einträge entfernt |
| 64737870 | Punkt 3: Suchfeld der Zeitleiste, Hinweis der Aufnahme, Ränder im Menü-Editor; zwei feste Texte übersetzt (rot/grün) |
| 65084442 | Punkt 4: 25 Bilder je Sprache, dazu h_zeitleiste und h_karte8_menue neu |
| 5e49f021 | Notizen vor dem Merge |
| 51775cb9 | Merge karte1 (d7dd6b28, Integration 5 bis dahin) + es/cs der M6-Texte (74 Programm-, 23 Handbuchtexte), 3 geänderte Handbuchtexte |
| 0c5eb52b | Bilder nach dem Merge: 7 M6-Bilder je Sprache, Zeitleisten-Satz neu |

## Zahlen: noch englisch je Sprache

| | es vorher | es nachher | cs vorher | cs nachher |
|---|---|---|---|---|
| Programmtexte (`gen_i18n.py --fehlend`) | 354 | **0** | 354 | **0** |
| Handbuchtexte (`gen_handbuch.py --pflicht`) | 149 (148 verschiedene) | **0** | 149 | **0** |
| Handbuch: unbenutzte Einträge | 33 | **0** | 33 | **0** |
| Handbuchbilder englisch | 25 | **0** | 25 | **0** |
| veraltete Handbuchbilder (h_zeitleiste, h_karte8_menue) | 2 | **0** | 2 | **0** |

## Punkt 1: Programmtexte und Handbuch

- 354 Texte je Sprache (zeitleiste1 154, menue5 111, menue3 63, dock1 24, vis1 2) als Block „uebersetzung2“ in
  `tools/i18n_sprache_es.py` / `_cs.py`. Begriffe wie im Bestand (es: línea de tiempo, plano, giro, fundido,
  cabezal, destino, disposición, lienzo; cs: časová osa, záběr, švenk, prolínání, cíl chůze, rozvržení, plátno).
  Spielbegriffe nach den spanischen Texten von Jedi Academy (assets0.pk3, strings/spanish, nur gelesen):
  g_subtitles „Ninguno / En secuencias / Todas las voces en off“, Salud, Blindaje, cruz filar, Munición,
  Estilo del sable; Tschechisch ohne offizielle Fassung: Zdraví, Brnění, Síla, Munice, Zaměřovač.
- **Zähltexte**: acht Texte nannten eine Anzahl ohne Mehrzahlform („1 walk targets“, „1 channels“,
  „1 menus“): AufHinweis, AufGeschrieben, MeVideoTonQuelle, MeVideoErgebnis, MeLayoutZustaende,
  MePrjGeoeffnet, MePrjSicherungText, MePrjWiederhergestellt → `{..|..}` in Englisch/Deutsch
  (`i18n_zeitleiste1.py`, `i18n_menue3.py`, `i18n_menue5.py`), es/cs mit ihren Formen (cs drei, „menu“ ist
  unveränderlich), Aufrufe auf `trN`. Test `sprache` (jeder Zähltext jeder Sprache für 0/1/2/3/5/21) grün.
  **Beim Zusammenführen:** diese acht Zeilen der Zweigdateien und die acht Aufrufe in `gui/app_mission.cpp`,
  `gui/app_menue.cpp`, `gui/app_menue_layout.inc`, `gui/app_menue_projekt.inc` sind geändert.
- Handbuch: 148 Texte je Sprache in `inhalt_es.py` / `inhalt_cs.py` (Zeitleiste neu, Menü-Editor M3/M4/M5/M5b,
  Tutorial „Dein erstes eigenes Menü“, Fenster anordnen, VIS-Schalter); die 33 Übersetzungen alter englischer
  Fassungen (alte Zeitleiste, VIS, Bau-Menü) entfernt – „Einträge nicht mehr gebraucht“ ist weg. Tasten im
  Fließtext wie im Land: es `<kbd>Mayús</kbd>`, `<kbd>Espacio</kbd>`, `<kbd>Inicio</kbd>`, Pfeile als ←/→.

## Punkt 2: lint_i18n

Rot: **5 Beanstandungen** (unbenutzte Texte). Ergebnis:
- `TlKamera` war eine Falschmeldung: der Helfer holt die Namen der Ketten in `src/hilfe_orte.inc`
  („ViewMap>TlKamera>CamPilotKey“) zur Laufzeit über `strVonName` – gelöscht hieße der Pfad
  „Missionsansicht >  > Schlüssel setzen“. `lint_i18n` liest jetzt Ketten und Hinweise aus `hilfe_orte.inc`.
- `TlKatBlick`, `TlKatAblauf` (alte Zeitleiste, von zeitleiste1 entfernt) und `MeKlaenge`, `MeTipKlaenge` (M4
  ersetzte den Knopf „Klänge“ durch den Ton-Schalter der Leiste, Einstellung `menueTon`) entfernt.
- Fund dabei: die Handbuchseite des Menü-Editors (M2) nannte noch den Knopf **Klänge** – jetzt „der Schalter
  **Ton an** in der Leiste“, in allen sechs Sprachen.
Grün: `lint_i18n` **ok** (0 Beanstandungen, es/cs je 0 fehlend), `lint_i18n_erzeugt` ok.

## Punkt 3: Passt jeder Text?

Durchsicht der neuen es/cs-Bilder gegen die englischen (Ausschnitte nebeneinander, `SP/u2/zeigen.py`). Die neuen
Texte selbst passen; abgeschnitten war Layout, das schon auf Englisch zu knapp ist:
- **Suchfeld der Zeitleiste** fest 11 Schriftbreiten: Englisch „… SET_ANIM .“, Spanisch „Buscar: nombre, marca,
  SET_ANI|“ → so breit wie sein Hinweis. Selbsttest zeitleiste1 (es) „das Suchfeld zeigt seinen Hinweis ganz“:
  rot (Feld 264, Hinweis 301 Punkte), grün nach der Korrektur.
- **Hinweis der Lauf-Aufnahme** in einer Zeile: in der Handbuch-Ansicht lief er in jeder Sprache rechts hinaus,
  Englisch „… 1 walk targets (toleran|“, Deutsch „… Esc bricht ab - 1 Lau|“ → umgebrochen auf die Breite der
  Ansicht. Prüfung im Handbuch-Lauf `BHED_HB_NUR=zeitleiste1` und im Selbsttest zeitleiste1: rot (x bis 2115,
  Ansicht bis 1968), grün danach.
- **Menü-Editor, Abschnitt Layout**: „Margins (left, top, right, botto|“ (es „Márgenes (izquierda, arriba, |“) –
  die Beschriftung steht jetzt über den vier Feldern. `spaltenGanz()` auch mit offenem Layout-Abschnitt: rot
  (`##rechts/##inhalt` 119 Punkte zu schmal), grün danach.
- **Feste Texte** (in keiner Sprache übersetzt, `lint_i18n` lässt Formate mit Wörtern durch, siehe Offen):
  „%zu items“ über den Eigenschaften bei Mehrfachauswahl → Zähltext `MeAuswahlItems` (es „4 elementos“,
  cs „4 prvky“), der deutsche Tooltip „%s von %s“ der Kartenskripte → `MapScriptKeyVon`
  (`tools/i18n_uebersetzung2.py`).
- Nicht abgeschnitten, bewusst: die Legende unter der Zeitleiste (Tooltip zeigt alle Tasten, Englisch ebenso),
  lange Pfade in der Menü-Editor-Leiste.

## Punkt 4: Bilder

`SP/u2/bilder.sh` (Bildsätze `zl` = `BHED_EDITORTEST=handbuch BHED_HB_NUR=zeitleiste1`, `dock` = `dock1` mit
`BHED_DOCK_NUR=fotos`, `menue` = `menueeditor`), je `BHED_SPRACHE` es/cs und `BHED_FOTOS`, dann
`tools/handbuch/bilder.py`. Erste Läufe: zl 7 Bilder (0/0), dock1 ohne `BHED_DOCK_NUR=fotos` 86/0 aber 0 Bilder
(nachgeholt: 5/0, 3 Bilder), menueeditor voll **105 OK / 0 FEHLER** je Sprache, 31 Bilder.
Durchsicht gegen die englischen Bilder:
- Der volle menueeditor-Lauf trifft nicht für jedes Bild den Zustand des englischen Bildsatzes: `menue_hud`
  braucht Jedi-Academy-Daten (mit Movie Duels zeichnet die Vorschau das HUD nicht), `menue_video_dialog` die
  Probevideos (`BHED_MENUE_VIDEOS`). Diese Sätze wie in menue3/M4 (`BHED_MENUE_NUR=m3` mit JKA-Einstellungen,
  `BHED_MENUE_NUR=m4` mit Videos).
- Die übrigen Menü-Editor-Bilder des vollen Laufs (M1/M2) nicht übernommen: die englischen sind aus
  uebersetzung1, die es/cs-Fassungen davon bleiben gleichwertig; drei Bilder, die keine Seite nutzt
  (`menue_hud_munition`, `menue_oberflaeche_100/150`), nicht abgelegt.
- Veraltet, obwohl vorhanden (zählt nicht als „englisch“): `h_zeitleiste` (es/cs zeigten die alte Zeitleiste)
  und `h_karte8_menue` (vis1 hat das Build-Menü mit „Same VIS in every build“ nur in vier Sprachen neu
  aufgenommen) – beide neu.
- Neu aufgenommen nach den Korrekturen von Punkt 3 mit `SP/u2/bilder3.sh` (nur die genannten Bilder gehen nach
  `handbuch/bilder/<sp>`): zl 1/0 (die neue Prüfung), m3 (JKA-Einstellungen) 6/0, m4 (Videos) 18/0, menueeditor
  voll **106/0**, karte8 (nur `h_karte8_menue`) 0/0, je es und cs; dock1 aus dem Lauf davor (5/0). Je Sprache
  25 + 2 Bilder, zusammen 4 MB. `gen_handbuch --pruefen --pflicht=es --pflicht=cs`: **ok**, 0 Texte, 0 Bilder
  englisch.

## Punkt 5: Selbsttests es/cs (Bereiche der neuen Texte)

`SP/u2/tests.sh`, Summe `SP/u2/tests_summe.txt` (unter Last):

| Selbsttest | es | cs |
|---|---|---|
| menueeditor (voll, mit Fotos) | **106/0** | **106/0** |
| zeitleiste | **47/0** | **47/0** |
| zeitleiste1 | 77/4 | 77/4 |
| dock1 | **86/0** | **86/0** |
| karteneditor `hilfe2` | **44/0** | **44/0** |
| Handbuch zeitleiste1 / menueeditor m5 (rot/grün Punkt 3) | 1/0, 10/0 | – |

zeitleiste1: die vier Fehler sind der JK2-Teil (`kejim_post` nicht in der Liste nach „New mission“), genau wie
Integration 4 auf Englisch („73 OK, 4 FEHLER – nur der JK2-Teil im vollen Lauf, allein grün“) – nicht sprachabhängig.

## Merge karte1 (Integration 5)

karte1 stand bei d7dd6b28: alle fünf Zweige des Auftrags gemergt (reste1, vis3, Zeitleiste 3, menue6, dock3),
Notizen von Integration 5 noch nicht da. Gemergt, damit die neuen Texte übersetzt werden; kommt auf karte1 noch
etwas, reicht ein weiterer Merge.

- **Konflikte**: karte1 hatte die fünf lint_i18n-Beanstandungen parallel behoben – `TlKamera` gelöscht und den
  Helferpfad der Kameraleiste auf `ZlOrt` („Kamera – Ort“) gestellt, den Satz zum Ton-Schalter eigen formuliert.
  Übernommen (die lint_i18n-Prüfung der Helferketten bleibt, sie schützt jetzt `ZlOrt`). Den Hinweis der
  Lauf-Aufnahme hat Zeitleiste 3 umgebrochen, mittig und frei von der Platzierleiste, mit eigener Prüfung –
  übernommen, meine Fassung und ihr App-Feld entfernt; die Prüfung im Handbuch-Lauf liest `aufnahmeHinweis()`.
  Generator-Listen: MENUE6, dann UEBERSETZUNG2; erzeugte Dateien neu (i18n, hilfe_orte 195 Orte, Handbuch).
- **Neue Texte**: nur menue6 brachte Programmtexte (74 je Sprache); reste1, vis3, Zeitleiste 3 und dock3 keine.
  Vier M6-Texte nannten Anzahlen ohne Mehrzahl („1 hints“, „Create page with 1 settings“, „1 menus: … 1 dead
  ends … 1 missing“, „1 more … stay“) → Zähltexte in `i18n_menue6.py` und `trN` in `gui/app_menue_m6.inc`; der
  Selbsttest, der den Knopf des Assistenten über seine Beschriftung klickt, holt sie ebenfalls mit `trN`
  (sonst fände er auf Tschechisch „few“ statt „other“). Handbuch: die M6-Seite (23 Texte) und drei von
  Zeitleiste 3/dock3 geänderte englische Sätze (Untertitel kleiner mit 0,66, die Figur läuft als Modell,
  Dateien auf ein Panelfenster ziehen) – es/cs je 0 Texte englisch.
- **Bilder**: die sieben M6-Bilder (`BHED_MENUE_NUR=m6`, je **16/0**) und der Zeitleisten-Satz noch einmal
  (Zeitleiste 3 hat Aufnahme-Hinweis und Untertitel geändert, en/de neu aufgenommen). Durchsicht: passt; im
  Assistenten überdeckt ein langer, in den MD-Daten unübersetzter Schlüssel (`@MD_MENU_SETTINGS_DYNAMIC_HUD`) die
  Spalte rechts daneben – in jeder Sprache, Layout des Assistenten (gemeldet, nicht geändert).
- **Prüfungen nach dem Merge** (Bau inkrementell 0 Warnungen, 17:41 min unter Last): ctest gezielt (sprache,
  hilfe*, tour, zeitleiste, handbuch_suche, changelog_sprachen, alle menue_*) **28/28**; lint_i18n ok (es/cs je 0
  fehlend), lint_i18n_erzeugt ok; gen_handbuch es/cs 0 Texte, 0 Bilder englisch. Danach **ctest gesamt 183/183**
  (4:30 min, -j 2), lint_sources und lint_fassung ok.

| Selbsttest nach dem Merge | es | cs |
|---|---|---|
| menueeditor (mit M6) | **122/0** | **122/0** |
| zeitleiste | **47/0** | **47/0** |
| zeitleiste1 | 80/1, wiederholt 80/1 | 80/1, wiederholt 80/1 |
| dock1 | **86/0** | **86/0** |
| karteneditor `hilfe2` | **44/0** | **44/0** |

zeitleiste1: Die JK2-Fehler sind mit Zeitleiste 3 weg; übrig sind Prüfungen der Lauf-Aufnahme von Zeitleiste 3,
die von der Bildzeit abhängen – Englisch im selben Bau **79/2** (dieselbe Prüfung „das Modell steht am gesteuerten
Ort“, academy1 und mini: 78,6 statt > 100 Einheiten), es wie en (mini 90,6), cs zweimal „die Vorschau läuft den
Weg … bis zum letzten Laufziel“: guard1 lief 489 statt ~365 Einheiten (Start −304 3616 in allen Sprachen), hinter das
Wegnetz – genau der Lastfall, den der Kommentar über `halteTaste` beschreibt („unter Last … 443 statt 331 Einheiten
… finds no path“). Nicht sprachabhängig; gehört zu Zeitleiste 3 (Offen).

## Gegen das Vorbild geprüft / nicht geprüft

- Spanische Spielbegriffe gegen die Texte von Jedi Academy (`assets0.pk3`, `strings/spanish/MENUS.str`,
  `sp_ingame.str`, nur gelesen): g_subtitles „Subtítulos: Ninguno / En secuencias / Todas las voces en off“,
  „Salud“, „Blindaje“ (ESCUDO PERSONAL (BLINDAJE)), „cruz filar“, „Munición“, „Estilo del sable láser“.
- Mehrzahl nach CLDR (Test `sprache`: jeder Zähltext jeder Sprache für 0/1/2/3/5/21, auch die zwölf neuen).
- Platz: alle neuen es/cs-Bilder gegen die englischen durchgesehen; die drei Layout-Funde mit roten/grünen
  Selbsttestprüfungen.
- Nicht geprüft: Tschechisch gegen eine offizielle Fassung (es gibt keine), die Texte im laufenden Spiel; volle
  es/cs-Läufe aller Modi (Schlussrunde laut Auftrag).

## Offen

- **karte1 nach d7dd6b28**: Integration 5 war beim Merge noch nicht abgeschlossen (danach z. B. 3cae6104,
  Untertitel von Jedi Outcast – ohne Texte). Kommt mit den Notizen von Integration 5 noch etwas mit Texten, einmal
  nachmergen; sonst nimmt die Integration beide Stände zusammen.
- Runde 3: M7, dock4, Zeitleiste 4, pruefung3.
- **Beim Zusammenführen**: Zähltexte in fremden Zweigdateien geändert – `i18n_zeitleiste1.py` (AufHinweis,
  AufGeschrieben), `i18n_menue3.py` (MeVideoTonQuelle, MeVideoErgebnis), `i18n_menue5.py` (MeLayoutZustaende,
  MePrj…), `i18n_menue6.py` (MeFlussZusammen, MeStilGesperrt, MeAssistentAnlegen, MeLesbar) und die Aufrufe dazu
  (`trN`); dazu `gui/app_view3d_bedienung.inc` (Suchfeld), `gui/app_menue_layout.inc` (Ränder),
  `gui/app_menue.cpp` („%zu items“), `gui/app_view3d_spiel.inc` (Tooltip).
- `lint_i18n` lässt Formatzeichenketten mit Wörtern durch (`re.fullmatch` mit a-zA-Z in `harmlos`), deshalb
  fielen „%zu items“ und „%s von %s“ nur in den Bildern auf. Schärfer gestellt meldet er 37 Stellen, fast alle im
  absichtlich unübersetzten Messfenster „Layout“ (app.cpp); echt übrig: die GPU-Tabelle im Statistikfenster der
  Karte (`gui/app_view3d.cpp`: „Karte/Effekte/Mover/Figuren/Gluehen … Aufrufe“, „GPU: %d Aufrufe, %d
  uebersprungen“) – in jeder Sprache deutsch, nicht geändert.
- Menü-Editor, Einstellungs-Assistent: ein langer Spieltext überdeckt die Spalte daneben (alle Sprachen).
- zeitleiste1: Aufnahme-Prüfungen von Zeitleiste 3 hängen unter Last von der Bildzeit ab (oben).
- Zeitleiste 3 hat Tutorial-Bilder (a_k_timen, c_tl_*, g_*_zeit, s_*_zeit, …) nur auf Englisch neu aufgenommen;
  de/zh/ja/es/cs zeigen dort noch die alte Zeitleiste (zählt nicht als „englisch“, `gen_handbuch` sieht es nicht).
- Unverändert: 18 Tutorial-Schritte ohne Bild (alle Sprachen).

## Changelog

## English
- Spanish and Czech are complete again: the new timeline, the menu editor (HUD, Asian texts, videos, sound switch, layouts, events, projects, mod profiles, flow, tab order, styles, templates, settings wizard, reference image, comparison with the original, readability), window arrangement and the "Same VIS in every build" switch, with all guide pages and their pictures.
- Counts in the new texts use the right plural form in every language ("1 walk target", "1 menu", "2 channels"; Czech three forms).
- The timeline search field shows its whole hint, the long "Margins" label of the menu editor's Layout section is no longer cut off, and "items" in the menu editor and a script tooltip in the mission view appear in the program language.

## Deutsch
- Spanisch und Tschechisch sind wieder vollständig: die neue Zeitleiste, der Menü-Editor (HUD, asiatische Texte, Videos, Ton-Schalter, Layouts, Ereignisse, Projekte, Mod-Profile, Menüfluss, Tab-Reihenfolge, Stile, Vorlagen, Einstellungs-Assistent, Referenzbild, Vergleich mit dem Original, Lesbarkeit), das Anordnen der Fenster und der Schalter „Dasselbe VIS in jedem Bau“ – samt allen Handbuchseiten und Bildern.
- Zahlen in den neuen Texten stehen in jeder Sprache mit der richtigen Mehrzahl („1 Laufziel“, „1 Menü“, „2 Kanäle“; Tschechisch mit drei Formen).
- Das Suchfeld der Zeitleiste zeigt seinen ganzen Hinweis, die lange Beschriftung „Ränder“ im Layout-Abschnitt des Menü-Editors wird nicht mehr abgeschnitten, und „Items“ im Menü-Editor sowie ein Tooltip der Kartenskripte erscheinen in der Programmsprache.

## 中文
- 西班牙语和捷克语再次完整:新的时间轴、菜单编辑器(HUD、亚洲文字、视频、声音开关、布局、事件、项目、模组配置、菜单流程、Tab 顺序、样式、模板、设置向导、参考图、与原版比较、可读性)、窗口排列以及"每次构建相同的 VIS"开关,连同所有手册页面和图片。
- 新文本中的数字在每种语言中都使用正确的复数形式(捷克语三种形式)。
- 时间轴的搜索框显示完整的提示,菜单编辑器布局部分中较长的"边距"标签不再被截断,菜单编辑器中的"项目"数和地图脚本的提示以程序语言显示。

## 日本語
- スペイン語とチェコ語が再び完全になりました:新しいタイムライン、メニューエディター(HUD、アジアのテキスト、動画、サウンドスイッチ、レイアウト、イベント、プロジェクト、MOD プロファイル、メニューの流れ、Tab の順序、スタイル、テンプレート、設定ウィザード、参考画像、オリジナルとの比較、読みやすさ)、ウィンドウの配置、「ビルドごとに同じ VIS」スイッチと、すべてのガイドページと画像。
- 新しいテキストの数はどの言語でも正しい複数形で表示されます(チェコ語は 3 つの形)。
- タイムラインの検索欄はヒントを最後まで表示し、メニューエディターのレイアウト欄の長い「余白」ラベルが切れなくなりました。メニューエディターの項目数とマップスクリプトのツールチップはプログラムの言語で表示されます。

## Español
- El español y el checo vuelven a estar completos: la nueva línea de tiempo, el editor de menús (HUD, textos asiáticos, vídeos, interruptor de sonido, disposiciones, eventos, proyectos, perfiles de mod, flujo de menús, orden de tabulación, estilos, plantillas, asistente de ajustes, imagen de referencia, comparación con el original, legibilidad), la organización de ventanas y la opción "El mismo VIS en cada compilación", con todas las páginas de la guía y sus imágenes.
- Los números de los textos nuevos llevan el plural correcto en cada idioma ("1 destino", "1 menú", "2 canales"; en checo, tres formas).
- El campo de búsqueda de la línea de tiempo muestra su indicación completa, la etiqueta larga "Márgenes" de la sección Disposición ya no se corta, y "elementos" en el editor de menús y una información sobre herramientas de los scripts del mapa aparecen en el idioma del programa.

## Čeština
- Španělština a čeština jsou opět úplné: nová časová osa, editor menu (HUD, asijské texty, videa, přepínač zvuku, rozvržení, události, projekty, profily modů, tok menu, pořadí tabulátoru, styly, šablony, průvodce nastavením, referenční obrázek, porovnání s originálem, čitelnost), uspořádání oken a přepínač „Stejné VIS při každém sestavení“ včetně všech stránek příručky a obrázků.
- Čísla v nových textech mají v každém jazyce správný tvar množného čísla („1 cíl chůze“, „2 kanály“, „5 cílů chůze“).
- Pole hledání časové osy ukazuje celou nápovědu, dlouhý popisek „Okraje“ v oddílu Rozvržení editoru menu se už neořezává a „prvky“ v editoru menu i popisek skriptů mapy se zobrazují v jazyce programu.
