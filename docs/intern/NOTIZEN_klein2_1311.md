# Notizen KLEIN2 (13.11.): Esc schließt jedes Menü des Model-Tabs, Untermenüs bleiben im Fenster - rc612

Auftrag KLEIN2 (Opus 5.5): zwei kleine Fehler, die AP14b beim Aufnehmen der Handbuchbilder sah (`NOTIZEN_ap14b_1311.md`, Beobachtung am
Ende). Worktree `Y:\p_ap6`, Zweig `p_klein2`, auf karte1 `1f397932c`. Selbsttest `gui/selbsttest_klein2.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=klein2`; `BHED_KLEIN2_PUNKT=esc,unter` wählt die Teile), Lint `tools/lint_menue.py`,
Werkzeuge, Läufe und Fotos `Y:\werkzeuge\klein2\`.

## 1. Esc schließt jedes Rechtsklick-Menü des Model-Tabs

Befund (Foto `Y:\werkzeuge\ap14b\fotos_befund\h_mb1_vier_explorermenue_offen.bmp`): Esc schloss das Rechtsklick-Menü des Szenen-Explorers
nicht. ENTSCHEID3A C4 galt nur dem Menü der Ansicht; sieben weitere Menüs (Kopfzeile der Ansicht, Viewports/Kuchen der Navigation,
Schattierung, ViewCube, Modus- und Gruppenmenü der Blender-Leiste) hatten je eine eigene Esc-Zeile, alle anderen keine. ImGui selbst
schließt Popups mit Esc nur mit Tastaturnavigation (`NavUpdateCancelRequest`, `ImGuiConfigFlags_NavEnableKeyboard`), die das Programm
nicht einschaltet.

Änderung - eine Stelle statt je Menü:

- `ui::menueEsc()` (`gui/ui_menue.h/.cpp`): Esc schließt alle offenen Menüs und Popups über dem obersten modalen Dialog (wie
  `ImGui::ClosePopupsExceptModals`), auch aus einem Untermenü, und verbraucht die Taste bis zum Loslassen (`SetKeyOwner(...,
  ImGuiInputFlags_LockUntilRelease)` - `IsKeyPressed`/`Shortcut` aller anderen sehen sie nicht, auch nicht die Wiederholung beim Halten).
  Nicht, während ein Textfeld schreibt (die Suchfenster "Alle Aktionen suchen" schließen dann wie bisher selbst mit dem ersten Esc) und
  nicht ohne offenes Menü.
- `modellMenueEscBildAnfang()` (`gui/app_modelltab_ent3a.cpp`, bei C4): hat der Model-Tab den Fokus (`tabfenster::fokusModus() == 2`,
  auch in einem eigenen Tab-Fenster), ruft es `ui::menueEsc()`; Aufruf in `draw()` (`gui/app.cpp`) gleich nach `tabfenster::bildAnfang()`,
  vor allem, was Esc liest. Der Zähler von C4 (`kontextEscZahl`) zählt weiter, wenn das Menü der Ansicht dabei war.
- Weg: der eigene Esc-Block in `drawModellKontextMenue`, `modellEnt3aKontextEsc()` in `modellTasten` und die sieben einzelnen Esc-Zeilen
  (`app_modelltab_blender.cpp` 2, `app_modelltab_kopf.cpp`, `app_modelltab_nav.cpp` 2, `app_modelltab_pruefen.cpp`,
  `wof/app_modelltab_viewcube.cpp`). Geblieben: die zwei Suchfenster mit Textfeld (`app_modelltab.cpp` "Alle Aktionen", und das der
  Figurenanimation im Mission-Tab) und Popups mit eigener Bedeutung von Esc in modalen Dialogen (Speichernfrage, "Abbrechen").

Keine andere Wirkung von Esc geht verloren:

- Ohne offenes Menü kommt Esc wie bisher bei `modellTasten` an (Navigationsmodus beenden, G/R/S und Werkzeuge abbrechen, Pinsel-Regler,
  Zug im UV-Editor, Schneiden ...). Der Selbsttest prüft es am Navigationsmodus: mit offenem Menü schließt das erste Esc nur das Menü (Zoom
  bleibt), das zweite beendet den Zoom.
- Mit offenem Menü tut Esc nur das (C4: "sonst geschieht nichts") - geprüft je Fall an Auswahl, Bereich, Navigationsmodus,
  Tasten-Zähler, Knochen, Bild und Abspielen.
- Der Regler der Animationswerkzeuge (`###kvregler`, kein modales Popup): zu geschlossen wirkt wie "Abbrechen" (`reglerZu(false)`, wie
  ein Klick daneben) - wie vorher sein eigenes Esc.
- Andere Tabs unverändert: der Karten-Editor schließt seine Menüs wie Qt eine Ebene je Esc (`app_karte_qtmenue.cpp`), der Rest wie
  bisher.

| Selbsttest klein2 (esc) | rot (Bau `bau_rot`: karte1 + Selbsttest) | grün (Bau `bau_esc`) |
|---|---|---|
| Szenen-Explorer wie 3ds Max / wie Blender | Menü bleibt offen (2 FEHLER) | Esc schließt, Stand gleich |
| Zeitleiste wie 3ds Max (Skala) | bleibt offen | schließt |
| Zeitleiste wie Blender, Untermenü Playback offen (2 Ebenen) | im ersten Lauf ging das Menü im Bereich Animation nicht auf (dort liegen auf der Skala die Spuren) - Fall auf den Bereich Weights gestellt wie AP1 Z9; dasselbe Popup `##mzmenue` war im Fall wie 3ds Max rot | beide Ebenen zu |
| Dopesheet | bleibt offen | schließt |
| Befehlsfeld (Zeile TurboSmooth im Stapel) | bleibt offen | schließt |
| Werkzeugleiste wie Blender | bleibt offen | schließt |
| Ansicht, Untermenü Weights offen (C4) | schon grün (C4), Zähler 1 | grün, Zähler 1 (zählt jetzt an der zentralen Stelle) |
| Menüleiste Animation > Animation tools (2 Ebenen) | bleibt offen | beide Ebenen zu |
| Navigationsmodus Zoom mit offenem Menü, dann ohne | Menü bleibt offen, Esc beendet den Zoom | erstes Esc: Menü zu, Zoom bleibt; zweites: Zoom aus |
| Summe | 15 OK / 12 FEHLER (mit Teil unter) | esc 23 / 0 |

## 2. Untermenüs bleiben im Fenster

Befund (Foto `h_mb5_werkzeuge_untermenue_zu.bmp`): das Untermenü Animation > Animation tools ragte im Hauptfenster 2538 × 1472 unten
hinaus. Mit `ImGuiConfigFlags_ViewportsEnable` rechnet ImGui die Lage eines Popups im Rechteck des Bildschirms
(`GetPopupAllowedExtentRect`: `ViewportAllowPlatformMonitorExtend`, die Arbeitsfläche des Monitors), und was über das Hauptfenster
hinausgeht, wird ein eigenes Windows-Fenster (`Begin`: "Late create viewport if we don't fit within our current host viewport"). Im
Selbsttest ist die Innenfläche 56 Bildpunkte niedriger als die Arbeitsfläche (2560 × 1528) - das Untermenü (348 hoch) stand bei
411,1145..657,1493, ein eigenes Fenster, auf dem Foto fehlte es. Neben einem kleineren Hauptfenster hing es frei auf dem Desktop.

Änderung - eine Stelle für alle Menüs:

- `ui::menue(text, an)` statt `ImGui::BeginMenu` (`gui/ui_menue.h/.cpp`): das Menü bekommt den Viewport des Fensters, aus dem es aufgeht
  (`SetNextWindowViewport`) - Hauptfenster, eigenes Tab-Fenster oder schwebendes Panel. Damit bleibt `ViewportAllowPlatformMonitorExtend`
  bei -1, `FindBestWindowPosForPopup` rechnet im Rechteck des Fensters, und ein Untermenü, das unten nicht passt, rückt nach oben, bis es
  ganz im Fenster steht (wie ein Windows-Menü am Bildschirmrand); es wird kein eigenes Fenster. Ein Menü, das höher oder breiter als das
  ganze Fenster ist (Größe aus dem letzten Bild, ImGui zeichnet ein neues Menü im ersten Bild verborgen), geht wie bisher darüber hinaus,
  sonst wäre es abgeschnitten; ebenso Untermenüs eines Popups, das selbst schon ein eigenes Fenster ist.
- Alle 70 `ImGui::BeginMenu` in `gui/` (18 Dateien: Menüleiste, Rechtsklick-Menüs von Ansicht, Zeitleiste, Dopesheet, Stapel, Gewichte,
  Malen, Modellieren, UV, Rig, Knochen, Kopfzeile, Navigation, Figurenanimation und Entities im Mission-Tab, Docking) gehen über
  `ui::menue`, dazu die Menüs des Karten-Editors im Stil des Programms. `tools/lint_menue.py` (in `tools/verify.sh`, mit `--gegentest`)
  meldet jedes neue direkte `ImGui::BeginMenu`/`BeginMenuEx`.
- Hat der Aufrufer den Viewport schon gewählt, bleibt es dabei: `dock::eigenesFensterWennVerdeckt` legt ein Menü der Menüleiste in ein
  eigenes Fenster, wenn ein schwebendes Panel oder Tab-Fenster über dem Hauptfenster liegt (dock1) - das bleibt so.
- Dasselbe für die Rechtsklick-Menüs der ersten Ebene: `ui::popup`, `ui::popupKontext`, `ui::popupKontextFenster` statt
  `ImGui::BeginPopup`, `BeginPopupContextItem`, `BeginPopupContextWindow` (70 Stellen in 31 Dateien). Nachgestellt nach dem ersten Schritt
  (nur Untermenüs): das Menü der Zeitleiste (wie 3ds Max eine lange Spalte) ging unten an der Skala bei 1414,668..1850,1524 auf, ein eigenes
  Fenster, und sein Untermenü Animation tools (1844,1176..2090,1524) mit ihm - ein Untermenü, das aus dem Fenster ragte, weil sein Menü es
  tat. Ein Popup mit gesetzter Lage (`SetNextWindowPos`, z. B. die Menüs der Kopfzeile der Ansicht) bleibt wie bisher: ImGui rückt eine
  gesetzte Lage nicht ins Fenster, im festen Viewport wäre der Rest abgeschnitten.
- Der Karten-Editor im Stil von NetRadiant öffnet seine Menüs und Popups wie Qt bis zum Bildschirmrand (`app_karte_qtmenue.cpp`, Höhe wie
  `UseFullScreenForPopupMenu`) und bleibt so (Menüs freigegeben mit `lint-menue:`, Popups von `gui/app_karte*`/`gui/karte_*` im Lint
  ausgenommen). Kombinationsfelder (`BeginCombo`) gehen wie bisher nach unten oder oben auf - unverändert.
- Werkzeuge, die Menüs im Quelltext lesen, kennen `ui::menue(`: `tools/gen_hilfe_orte.py` (die Orte der Menüleiste für den Helfer;
  `src/hilfe_orte.inc` neu erzeugt bytegleich) und `tools/lint_i18n.py` (feste Texte in `ui::menue("...")`).

| Selbsttest klein2 (unter) | rot | grün |
|---|---|---|
| Animation > Animation tools (unten, passt nicht nach unten) ganz im Fenster, kein eigenes Fenster | 411,1145..657,1493, eigenes Fenster | 411,1120..657,1468 im Hauptfenster |
| dritte Ebene Key tools ganz im Fenster, kein eigenes Fenster | 651,1265..860,1463, eigenes Fenster (erbt den Viewport) | 651,1240..860,1438 im Hauptfenster |
| Gegenprobe Ansicht > Thema (Platz nach unten): geht wie bisher rechts neben der Zeile auf | grün | grün |
| Rechtsklick-Menü der Zeitleiste und sein Untermenü Animation tools ganz im Fenster | 1414,668..1850,1524 und 1844,1176..2090,1524, eigene Fenster (nach Schritt 1) | 1414,612..1850,1468 und 1844,1120..2090,1468 im Hauptfenster |

Handbuchbild `h_mb5_werkzeuge` in sechs Sprachen neu: wieder das Menü Animation > Animation tools > Key tools mit dem Tooltip von
Breakdown (AP14b hatte ersatzweise die Suche aufgenommen); Bildunterschrift in sechs Sprachen (`inhalt_mb3b.py`), Aufnahmeschritte
`gui/selbsttest_mb5.inc` mit Prüfzeile (beide Untermenüs ganz im Hauptfenster). Der Absatz zu Esc (`inhalt_ent3a.py` "esc", Seite
modell-bedienung) nennt jetzt jedes Menü des Model-Tabs.

## Läufe

Bau `Y:/p_ap6/build` (Release, `--parallel 2 -- /nodeReuse:false`, /W4 /WX, Leerlauf-Priorität): `bau_rot`, `bau_esc` (voller Bau der
Oberfläche: `modelltab.h` geändert), `bau_unter`, `bau_popup2`, zuletzt alle Ziele (`bau_alles`) - jeder 0 Warnungen. Läufe mit
`Y:\werkzeuge\klein2\lauf.sh` (eigener Laufordner `Y:\p_ap6\build\laeufe\<name>`, Leerlauf-Priorität), höchstens zwei Programme
gleichzeitig.

| Lauf | Ergebnis |
|---|---|
| klein2 (esc + unter), Endstand | 27 OK / 0 FEHLER |
| ctest (273) | 272 / 1: `hilfe_orte` - `tools/gen_hilfe_orte.py` las die Menüs der Menüleiste an `ImGui::BeginMenu(tr(Str::...))` und fand nach dem Ersetzen 67 Orte weniger (454 → 387). Der Erzeuger kennt jetzt auch `ui::menue(`; neu erzeugt ist `src/hilfe_orte.inc` bytegleich (454 Orte). Danach hilfe_orte, hilfe, tour, handbuch_sprachen, handbuch_suche 5 / 5 |
| Gruppen modellblock ap2 / ap1 / ent3a / mb5 | 68 / 0, 143 / 0, 56 / 0 (C4 grün über die zentrale Stelle), 161 / 0 |
| menueeditor Kurzlauf (`BHED_MENUE_NUR=kontext+leiste+datei`: Rechtsklick-Menüs, Menüleiste, Datei - Popups über `ui::popup`) | 168 / 0 |
| karteneditor tasten (`BHED_KARTE_GRUPPEN=tasten`) | 37 / 0 |
| Handbuch mb5 (`BHED_HB_NUR=mb5`) in en, de, zh-Hans, ja, es, cs | je 162 / 0 (die neue Prüfzeile: Animation tools und Key tools ganz im Hauptfenster) |
| `gen_handbuch.py` / `--pruefen` | 184 Seiten × 6 Sprachen, es/cs 0 Texte englisch, handbuch_sprachen ok (der Hinweis „2 Einträge in uebersetzung/ werden nicht mehr gebraucht“ stand schon auf karte1); neu erzeugt: modell-bedienung, modell-kurven, suche.js je Sprache |
| Lints | lint_menue ok (und `--gegentest` 11 / 11), sources, headers, imgui_context, imgui_alpha, i18n (erkennt jetzt auch feste Texte in `ui::menue("...")`), gapp, feldinit ok; portability 101, pruefcodes 11 (wie karte1) |

Bilder angesehen: `h_mb5_werkzeuge` en ganz (Untermenüs Animation tools und Key tools im Fenster, Tooltip von Breakdown), de und ja den
unteren Teil (Texte nicht abgeschnitten). Nicht gelaufen (bewusst): die übrigen Gruppen des Modell-Blocks und der volle karteneditor - sie
laufen in der vollen Runde nach dem Zusammenführen; die Menüs und Popups dort gehen über dieselben zwei Funktionen.

## Changelog

### English

- Model tab: Esc closes every right-click menu (Scene Explorer, timeline and dope sheet, command panel, toolbar, view) and the menus of the menu bar, also from an open submenu, as in 3ds Max and Blender; without an open menu Esc works as before.
- Menus: a menu or right-click menu that does not fit below opens upwards and stays inside the window, like a Windows menu, instead of sticking out as a separate window (also its submenus).

### Deutsch

- Model-Tab: Esc schließt jedes Rechtsklick-Menü (Szenen-Explorer, Zeitleiste und Dopesheet, Befehlsfeld, Werkzeugleiste, Ansicht) und die Menüs der Menüleiste, auch aus einem offenen Untermenü, wie in 3ds Max und Blender; ohne offenes Menü wirkt Esc wie bisher.
- Menüs: ein Menü oder Rechtsklick-Menü, das unten nicht passt, geht nach oben auf und bleibt im Fenster wie ein Windows-Menü, statt als eigenes Fenster hinauszuragen (auch seine Untermenüs).

### 中文

- 模型选项卡: Esc 关闭每个右键菜单 (场景资源管理器、时间轴和摄影表、命令面板、工具栏、视图) 以及菜单栏的菜单, 包括打开的子菜单, 与 3ds Max 和 Blender 相同; 没有打开的菜单时, Esc 的作用与以前相同。
- 菜单: 下方放不下的菜单或右键菜单 (以及其子菜单) 会向上展开并留在窗口内, 与 Windows 菜单相同, 而不再作为单独的窗口伸出。

### 日本語

- モデルタブ: Esc で、すべての右クリック メニュー (シーン エクスプローラ、タイムラインとドープシート、コマンド パネル、ツールバー、ビュー) とメニューバーのメニューが、開いているサブメニューからでも閉じます。3ds Max や Blender と同じです。メニューが開いていないときは、Esc はこれまでどおりに働きます。
- メニュー: 下に収まらないメニューや右クリック メニュー (そのサブメニューも) は、Windows のメニューと同じく上に開いてウィンドウ内に収まり、別ウィンドウとしてはみ出しません。

### Español

- Pestaña Model: Esc cierra cualquier menú contextual (Explorador de escena, línea de tiempo y hoja de claves, panel de comandos, barra de herramientas, vista) y los menús de la barra de menús, también desde un submenú abierto, como en 3ds Max y Blender; sin un menú abierto, Esc funciona como antes.
- Menús: un menú o menú contextual que no cabe hacia abajo se abre hacia arriba y queda dentro de la ventana, como un menú de Windows, en lugar de sobresalir como una ventana aparte (también sus submenús).

### Čeština

- Karta Model: Esc zavře každou nabídku pravého tlačítka (Průzkumník scény, časová osa a dopesheet, panel příkazů, panel nástrojů, pohled) i nabídky panelu nabídek, i z otevřené podnabídky, jako ve 3ds Max a Blenderu; bez otevřené nabídky funguje Esc jako dřív.
- Nabídky: nabídka nebo nabídka pravého tlačítka, která se dolů nevejde, se otevře nahoru a zůstane v okně jako nabídka Windows, místo aby vyčnívala jako samostatné okno (i její podnabídky).
