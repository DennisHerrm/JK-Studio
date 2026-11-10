# Notizen NUMPAD: der Ziffernblock setzt im Karten-Editor das Raster (10.11.2026, `Y:\p_numpad`, Zweig `p_numpad`, Basis a00f6cf90)

shank 09.11. (Wunsch eines Mappers): "btw when working in the editor, the NUMPAD numbers should change grids in mapping".
Entscheidung shank: "Raster + Schalter". Vorbild fuer den Schalter: NetRadiant-Custom, `radiant/xywindow.cpp:2273-2275`
(ViewTop Num+7, ViewFront Num+1, ViewSide Num+3), `radiant/grid.cpp:192-200` (SetGrid1 .. SetGrid256 auf 1 .. 9),
`radiant/xywindow.cpp:2216` (ToggleGrid auf 0). Abweichung von NRC in der Vorgabe, mit Schalter wieder genau NRC.

## Punkt 1: Vorgabe - der Ziffernblock tut, was die Ziffernreihe tut

- Vorher: Num 1 / 3 / 7 schalteten die 2D-Ansicht (Vorne, Seite, Oben), die uebrigen Ziffern des Ziffernblocks taten nichts.
- Jetzt: Num 1 .. Num 9 setzen das Raster 1 .. 256, Num 0 blendet das Raster ein und aus - wie 1 .. 9 und 0 der oberen Reihe.
- Umsetzung in `karte::befehle::Belegung` (`include/bhed/karte/befehle.h`, `src/karte/befehle.cpp`): jeder Befehl hat neben
  seiner Taste eine Zweittaste. Die Regel nennt keine Befehle: eine NRC-Vorgabe "Ziffer ohne Zusatztaste" bekommt
  `Num+<Ziffer>` dazu, eine NRC-Vorgabe `Num+<Ziffer>` entfaellt. `treffer` (QShortcutMap::find), `belegtVon`
  (VerifyAcceleratorNotTaken), Reset und Reset All kennen beide Tasten.
- Gespeichert: `BhedNumpad=grid|views` bei den Einstellungen des Karten-Editors; eine geaenderte Zweittaste als
  `karteTaste=<Befehl>#Num=<Taste>` (leer = keine).
- Eigene Tasten gewinnen: kaeme eine Vorgabe beim Umschalten neu auf die eigene Taste eines anderen Befehls, bleibt sie leer
  (die Taste waere sonst mehrdeutig und loeste nichts mehr aus). Einstellungen von vor rc612 (ohne `BhedNumpad`) werden
  darum wie NRC gelesen und danach auf "Raster" geschaltet (`belegungLaden`, `gui/app_karte_menue.cpp`): wer in rc611 einen
  Befehl auf Num 5 gelegt hat, behaelt ihn dort, die uebrigen Ziffern setzen das Raster.

## Punkt 2: Schalter "Numpad sets the grid"

- Menue **Grid**, letzter Eintrag (eigene Liste `ProgrammGrid`, abhakbar, mit Hinweis), und **Edit > Preferences > Grid**
  (Haken, mit Hinweis; gilt mit OK). Aus = NRC: Num 1 Vorne, Num 3 Seite, Num 7 Oben, die uebrigen Num-Ziffern ohne Befehl.
- Der Helfer ("Was willst du tun?") findet den Schalter: er steht im Inhalt der Seite Preferences > Grid
  (`src/hilfe_orte.inc`, erzeugt) und im Handbuch.
- **Edit > Shortcuts**: die Spalte Key und die Suche nach Tasten zeigen beide Tasten ("5 / Num+5"), ebenso
  `karte_commandlist.txt`. Eine vergebene Num-Taste loest die Rueckfrage "Key already used" aus; Yes nimmt dem anderen
  Befehl nur diese Taste (seine andere bleibt). Clear leert beide Tasten, Reset holt beide Vorgaben zurueck und fragt, wenn
  eine davon vergeben ist. Die Markierung "(x)" nennt als Hinweis die Vorgabe; wo sie vom Ziffernblock kommt, heisst es
  "Differs from the default (the numpad sets the grid)" statt "... NetRadiant-Custom's default".

## Punkt 3: Ansichten bleiben erreichbar, Anzeige im Menue

- Strg+Tab (NextView) und **View > Orthographic** wie bisher. Im Menue stehen bei Grid beide Tasten ("1 / Num+1"), bei den
  Ansichten die Num-Tasten nur mit dem Schalter wie NRC (`Belegung::text` in Menues, Knopf-Hinweisen und im Texturbrowser).

## Punkt 4: Tutorial und Handbuch

- Tutorial "Erste eigene Karte" (`gui/selbsttest_karte8c.inc`, `tools/handbuch/inhalt_karte8c.py`): Schritt "Hoeher machen"
  nimmt Strg+Tab fuer die Vorderansicht und View > Orthographic > XY (Top) zurueck (vorher Num 1 / Num 7).
- Handbuch Karten-Editor (6 Sprachen): Tastentabelle "[ / ]" nennt Num 1 .. Num 9, neuer Absatz "Ziffernblock" im Abschnitt
  Tastenkuerzel. Kein neues Bild: weder das Menue Grid noch die Seite Grid sind im Handbuch abgebildet, das Bild des Menues
  View zeigt Orthographic zugeklappt.

## Nebenbei gefunden

- `src/karte/menues.inc` (erzeugt) war von Hand ergaenzt worden: `ProgrammTexturen` (zweimal, rc609 K) und `ProgrammCurve`
  (Stufe 9d) fehlten in der Quelle `tools/karte_menues.py`; `tools/karte/gen_befehle.py` haette sie beim naechsten Lauf
  entfernt. Quelle nachgezogen, `LISTE` nimmt eine Bemerkung fuer die erzeugte Zeile; die erzeugte Datei aendert sich nur um
  die neue Zeile `ProgrammGrid`.
- `tools/handbuch/gen_handbuch.py` fuehrt eine eigene Liste der i18n-Module (neu: `i18n_numpad`).

## Selbsttest

`gui/selbsttest_numpad.inc` (karteneditor, `BHED_KARTE_NUR=numpad`; im vollen karteneditor-Lauf nach den Tasten der Stufe 1b),
`tests/kartebefehle.cpp` (ctest `karte_befehle`), `tests/hilfetest.cpp` (ctest `hilfe`). Laeufe mit `Y:\werkzeuge\numpad\lauf.py`.

- Rot, alter Code (Stand cae140362): ctest `karte_befehle` 10 Fehlschlaege (Num 0 .. Num 9 treffen nicht ToggleGrid / SetGrid1 ..
  SetGrid256); Selbsttest `np_rot` 13 OK, 10 FEHLER - Num 1 loeste ViewFront aus, Num 3 ViewSide, Num 7 ViewTop, die uebrigen
  Ziffern nichts (Raster blieb 16), Num 0 nichts.
- Rot, erster Entwurf des Kerns (Zweittaste ohne Vorrang eigener Tasten): 5 weitere ctest-Zeilen - Schalter aus und wieder an
  machte eine eigene Num-Taste mehrdeutig; eine eigene Taste auf Num+1 stiess nach dem Umschalten mit ViewFront zusammen; eine
  eigene Num-Taste aus rc611 wurde mit der neuen Vorgabe mehrdeutig; dieselbe Taste zweimal am Befehl stand doppelt da
  ("Num+5 / Num+5"); die Meldung einer unlesbaren Zweittaste nannte nicht den Eintrag. Danach gruen (26 neue Zeilen: 10 + 16).
- Gruen `np_gruen` 54 OK, 0 FEHLER, ebenso in de, zh, ja, es, cs (je 54/0): Num 1 .. 9 und Num 0; Menue Grid (Eintrag am
  Ende, Hinweis); Schalter aus (gespeichert `views`, Num 1/3/7 = Ansichten, Num 5 ohne Befehl, obere Reihe weiter Raster,
  View > Orthographic); Preferences > Grid (Hinweis, Haken gilt erst mit OK, gespeichert `grid`); Shortcuts (Suche nach
  "Num+5", Rueckfrage, Yes, gespeichert `Zoom100=Num+5` und `SetGrid16#Num=`, Markierung mit Hinweis, Clear, Reset mit
  Rueckfrage, Reset All); Schalter aus und an mit eigener Num-Taste; Tastenbelegung wie von rc611; Strg+Tab und View >
  Orthographic > XY (Top).
  - Ein Fund im ersten Lauf (53/1): der Hinweis des Schalters war in Preferences > Grid 1277 Punkte breit und ragte ueber
    den rechten Rand des Programms (2538 breit) - jetzt in kurzen Zeilen (en 696, de 850, ja 749, cs 849 breit, ganz im Bild).
  - Fotos angesehen (2538 x 1472): `numpad_menue_grid`, `numpad_menue_ansicht_nrc`, `numpad_menue_ansicht`, `numpad_pref_grid`,
    `numpad_konflikt`, `numpad_tasten` in en; die Hinweise auch in de, ja, cs.
- Angepasste Tests: Stufe 6 `tasten` drueckt NRCs Vorgaben mit dem Schalter wie NRC (`BHED_STUFE6_NUR=tasten` 189/0); die
  Menuegroessen gegen die NRC-Fotos ebenso (`BHED_KARTE_NUR=fotos` 78/0; Grid 205 x 377 statt 163 x 346: Trenner 7 + Eintrag 24,
  der neue Eintrag ist der laengste; View > Orthographic 307 x 240 wie NRC); Stufe 1b nimmt ein drittes Strg+Tab statt Num 7;
  Tutorial `BHED_KARTE_NUR=stufe8c` 35/0.
- ctest `hilfe`: je Sprache (de, en, zh, ja) nennt der Helfer auf "Numpad sets the grid" unter den ersten drei die Seite
  Preferences > Grid oder den Abschnitt Tastenkuerzel des Handbuchs.

## Schluss

Schlussrunde am Endstand (10.11. nachmittags, nach der Abgabe des ersten Agenten fortgesetzt; Skripte
`Y:\werkzeuge\numpad\schluss2.sh`, Ausgaben `Y:\werkzeuge\numpad\s2\`). Bau allein, danach alles parallel.

| Lauf | Ergebnis |
|---|---|
| Sauberer Neubau (`--clean-first`, /W4 /WX) | exit 0, 0 Warnungen, 1209 s |
| ctest alle (ohne `q3map2_pr316`) | 244 von 245; rot nur `q3map2_teilbau` mit 2 Zeilen, gehoert zur Basis (unten) |
| ctest `q3map2_pr316` allein | 1 von 1 |
| karteneditor voll (`np_s2_karteneditor`) | 3438 OK, 0 FEHLER (lcb1 am Morgen 3396: dazu die 42 Pruefungen des Numpad-Teils) |
| Modus 1 (`np_s2_modus1`) | 2635 OK, 0 FEHLER |
| Lints (32) gegen die Basis a00f6cf90 (`lints.py`) | Ausgabe Zeile fuer Zeile gleich (portability 99, pruefcodes 11, efxflags und fremdprogramm 1 wie in der Basis) |
| Zwischenstaende der Commits 1-3 | die betroffenen Dateien je Stand mit den Schaltern des Baus uebersetzt (`cl /Zs`, `schnitt2\zwischen.py`): 0 Warnungen |

- Im vollen Lauf zum ersten Mal: der Numpad-Teil mitten im Lauf (42 OK, er stellt Raster, Ansicht und Tastatur wieder
  her) und Stufe 1b mit dem dritten Strg+Tab ("Ctrl+Tab: YZ -> XY"); ebenso gruen Stufe 6 `tasten` (Num 1/3/7 mit dem
  Schalter wie NRC), das Tutorial (Strg+Tab, View > Orthographic > XY (Top)) und die Menuegroessen (Grid 205 x 377).
- Die gezielten Laeufe oben (numpad in 6 Sprachen 54/0, fotos 78/0) liefen schon auf dem Code des Endstands; stufe6
  `tasten` 189/0 und stufe8c 35/0 auf dem Stand davor (seitdem nur Hinweistext und Grid-Breite) - im vollen Lauf erneut
  gruen.
- `q3map2_teilbau`: "zweiraum, -bounce 2, Brush (Detail in Raum B)" und "... Shader (Wand in Raum A)" erwarten an der Basis
  'geometry' (nichts wiederverwendet), der Licht-Cache dieses WIP-Stands von lcb2 verwendet aber schon wieder (9/24 und
  18/18 Lightmaps je Durchgang). lcb2 hat genau diese zwei Faelle nach der Basis umgestellt (688daa357,
  `tools/karte/q3map2_teilbau.py`: 'treffer'). NUMPAD aendert nichts an q3map2 oder an diesem Test.
- Modus 1 schreibt 19 Diagnosezeilen "FEHLER: Feld 0 zeigt Reiter ..." (Pruefung der geteilten Skriptfelder in
  `gui/app.cpp`, kein Knopftest-Fehler); der Lauf lcb1_m1 vom Morgen (vor NUMPAD) hat dieselben 19.
- Handbuch in 6 Sprachen nachgelesen (Absatz Ziffernblock, Tastentabelle, Tutorial, Tour). Berichtigt: im Absatz
  Tastenkuerzel holte **Reset** "die Taste von NetRadiant-Custom" zurueck - mit dem Ziffernblock gilt das nicht mehr fuer
  jeden Befehl (ViewTop hat keine Vorgabe mehr, SetGrid16 "5 / Num+5"); jetzt "die vorgegebene Taste". Kein Bild sichtbar
  neu: `h_karteneditor_tasten` zeigt nur die Befehle A bis B, Grid-Menue und Seite Grid sind nicht abgebildet.
- Commits: die WIP-Commits sind zu je einem Commit je Punkt und einem fuer die Notizen zusammengefasst, der Baum ist
  derselbe wie vorher (`Y:\werkzeuge\numpad\schnitt2\schnitt2.sh`); `gui/selbsttest_karte3bis5.inc` ist geteilt (Fotos mit
  dem Ziffernblock wie NRC zu Punkt 1, Grid-Menue mit dem Schalter zu Punkt 2).

## Offen

- Die Basis a00f6cf90 ist ein WIP-Stand von lcb2; beim Einordnen in `karte1` auf dessen Stand setzen (dort ist
  `q3map2_teilbau` mit 688daa357 umgestellt). Beruehrt werden dieselben erzeugten Dateien (`include/bhed/i18n.h`,
  `src/i18n.cpp`, `tools/gen_i18n.py`, `tools/i18n_sprache_*.py`, Handbuch): nach dem Einordnen die Generatoren neu laufen lassen.
- Gegen ein laufendes NetRadiant-Custom nicht verglichen (radiant.exe startet kein Agent); NRCs Tasten stehen mit Quelle in
  `src/karte/befehle_liste.inc`.
- Die Zweittaste laesst sich in Edit > Shortcuts nicht einzeln neu belegen (Edit setzt die erste Taste; die zweite weicht
  ueber Yes, Clear leert beide, Reset holt beide zurueck).
- Der Menuepunkt selbst ist kein Ort des Helfers (Listen des Programms stehen nicht im Ortskatalog); der Helfer fuehrt zur
  Seite Preferences > Grid und zum Handbuch. Die Frage an den Helfer ist fuer de, en, zh, ja geprueft, nicht fuer es und cs.

## Changelog

## English

- Map editor: the numpad now sets the grid like the number row (Num 1 to Num 9 = grid 1 to 256, Num 0 shows or hides the grid). The new switch "Numpad sets the grid" (menu Grid and Preferences > Grid) brings back NetRadiant-Custom's numpad views (Num 1 front, Num 3 side, Num 7 top); the views stay on Ctrl+Tab and in View > Orthographic. Menus and the shortcut list show both keys, and your own numpad shortcuts keep working.

## Deutsch

- Karten-Editor: Der Ziffernblock setzt jetzt das Raster wie die Ziffernreihe (Num 1 bis Num 9 = Raster 1 bis 256, Num 0 blendet das Raster ein oder aus). Der neue Schalter „Ziffernblock setzt das Raster“ (Menü Raster und Einstellungen > Raster) holt die Ansichten von NetRadiant-Custom auf den Ziffernblock zurück (Num 1 vorne, Num 3 Seite, Num 7 oben); die Ansichten bleiben auf Strg+Tab und unter Ansicht > 2D-Ansicht. Menüs und die Liste der Tastenkürzel zeigen beide Tasten, eigene Kürzel auf dem Ziffernblock wirken weiter.

## 中文

- 地图编辑器：数字小键盘现在与数字行一样设置网格（Num 1 到 Num 9 = 网格 1 到 256，Num 0 显示或隐藏网格）。新开关“数字小键盘设置网格”（“网格”菜单和 首选项 > 网格）可恢复 NetRadiant-Custom 的小键盘视图（Num 1 前视图、Num 3 侧视图、Num 7 顶视图）；视图仍可用 Ctrl+Tab 和 视图 > 正交视图 切换。菜单和快捷键列表会显示两个按键，你自己设置在小键盘上的快捷键继续有效。

## 日本語

- マップエディター：テンキーが数字キーの列と同じようにグリッドを設定するようになりました（Num 1〜Num 9 = グリッド 1〜256、Num 0 でグリッドの表示/非表示）。新しいスイッチ「テンキーでグリッドを設定」（「グリッド」メニューと 設定 > グリッド）で、NetRadiant-Custom のテンキーによるビュー切り替え（Num 1 正面、Num 3 側面、Num 7 上面）に戻せます。ビューは引き続き Ctrl+Tab と 表示 > 平行投影 で切り替えられます。メニューとショートカットの一覧には両方のキーが表示され、テンキーに自分で割り当てたショートカットはそのまま使えます。

## Español

- Editor de mapas: el teclado numérico ahora ajusta la rejilla como la fila de números (Num 1 a Num 9 = rejilla de 1 a 256, Num 0 muestra u oculta la rejilla). El nuevo interruptor "El teclado numérico ajusta la rejilla" (menú Grid y Preferences > Rejilla) recupera las vistas de NetRadiant-Custom en el teclado numérico (Num 1 frontal, Num 3 lateral, Num 7 superior); las vistas siguen en Ctrl+Tab y en View > Orthographic. Los menús y la lista de atajos muestran ambas teclas, y tus propios atajos del teclado numérico siguen funcionando.

## Čeština

- Editor map: numerická klávesnice teď nastavuje mřížku stejně jako řada číslic (Num 1 až Num 9 = mřížka 1 až 256, Num 0 mřížku zobrazí nebo skryje). Nový přepínač „Numerická klávesnice nastavuje mřížku“ (nabídka Grid a Preferences > Mřížka) vrátí pohledy z NetRadiant-Custom na numerickou klávesnici (Num 1 zepředu, Num 3 z boku, Num 7 shora); pohledy zůstávají na Ctrl+Tab a ve View > Orthographic. Nabídky a seznam zkratek ukazují obě klávesy a vaše vlastní zkratky na numerické klávesnici fungují dál.
