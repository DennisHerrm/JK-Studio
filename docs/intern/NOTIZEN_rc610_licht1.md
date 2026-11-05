# Notizen rc610 / LICHT1: Echtzeit-Licht zeigt die ganze Karte, voll optimiert

Auftrag `auftrag_rc610_licht1.md` (Meldung 05.11. abends: „when I go back for example to the start of the hangar I do not see the light at the end of the hangar“, dazu „geht das mit guter Leistung? voll optimiert“). Direkt in `Y:\karte` auf `karte1` ab f550bdad (F6b), Build `build-test`, Laeufe `laeufe/li1_*` (rot `li1_rot`, `li1_h576_rot`, `li1_taris_rot`; gruen `li1_g1` bis `li1_g5`, `li1_9c_g1`; Bildzeiten `li1_zvor_*`, `li1_znach_*`, `li1_zg5_*`; Schluss `li1e_*`), Laufskript `lauf.py` wie F6b. Die rote exe liegt als Kopie in `laeufe/li1_exe_rot` (f550bdad plus die Zaehler und der neue Selbsttest). kFassung bleibt rc609. Kein Spiel gestartet.

Neue Pruefungen: Selbsttest `karteneditor` mit `BHED_KARTE_NUR=licht1` (`gui/selbsttest_kartelicht1.inc`, auch im vollen Lauf nach Stufe 9d): Testkarte „langer Hangar“ (4096 x 1024 x 512, Saeulen in zwei Reihen, 576 Punktlichter in neun Reihen), Kamera am Anfang, in der Mitte und am Ende; fern (die Wand am Ende) und nah (Boden) gegen das gebackene Licht mit den Grenzen der Stufe 9c, die Zaehler des ersten vollstaendigen Bilds und eines Rundgangs (240 Bilder vom Anfang zum Ende und zurueck); mit `BHED_LICHT1_KARTEN` dazu der Vergleich Wuerfelgroesse nach der Huelle gegen lauter 512er. ctest `schattenatlas` (neu, `tests/schattenatlas.cpp`). Stufe 9c: die Messung wartet je Punkt auf die Schattenwuerfel, das Oeffnen mit geschaetzten Lichtern ist eine Funktion (`karteMitLichternOeffnen`), die auch licht1 nutzt.

## Ursache (bestaetigt)

- `gui/app_karte_echtlicht.cpp` bis rc609: je Bild hoechstens `kLichtMax` = 512 Lichter, gereiht nach dem Beitrag am Auge (photonen / Abstand^2); Schattenwuerfel nur fuer die ersten 128 (32 zu 512, 96 zu 256), hoechstens 8 neue je Bild und 4 Mio. Dreiecke, Plaetze nach Rang neu vergeben (LRU). Wer auf seinen Wuerfel wartete, fehlte im Bild; ab Rang 128 leuchteten die Lichter ohne Schatten; ab Rang 512 fehlten sie ganz.
- Nachgestellt (rot, alte Darstellung): Hangar mit 320 Lichtern - im ersten vollstaendigen Bild 192 Lichter ohne Schatten; Rundgang: 705 Wuerfel neu gerechnet, bis zu 88 Lichter fehlend, 192 ohne Schatten (`li1_rot`, 13 OK / 2 FEHLER). Hangar mit 576 Lichtern (`li1_h576_rot`, Messung Stufe 9c): vom Anfang aus ist die Wand am Ende dunkel - Helligkeit echt 17,7 gegen gebacken 43,3 (Block-Mittel 25,56, Max 30,74; ganzes Bild Max 31,23). Das ist die Meldung: die 64 fernsten Lichter lagen hinter Rang 512.
- Gegenprobe auf einer Karte, die nicht zum Finden diente: duel_mos_espa (172 Lichter der Shader plus 1030 aus der .bsp geschaetzte = 1202): rot Mittel 30,37, P0 39,56 / 196,74, P1 50,39 / 201,91 - Grenze (15 / 200) verfehlt (`li1_taris_rot`).

## Umsetzung

- **Ein Wuerfel je Licht, behalten** (`wuerfelVerteilen`, `wuerfelPruefen`): Schluessel Ort und Weite (gleiche Lichter teilen sich einen). Die Schattengeometrie haelt je Eimer (Art, 512er-Zelle, Shader) eine Pruefsumme der Ecken samt Art und Bild; nach einem Neubau gelten nur die Wuerfel nicht mehr, deren Huelle die alten oder neuen Grenzen eines geaenderten Eimers trifft. Neues Grafikgeraet: alle neu. Keine Vergabe nach Rang mehr.
- **Vorberechnen**: das erste Bild nach dem Einschalten (oder nach mehr als 0,25 s ohne Bild) rechnet Wuerfel bis 1,5 s lang, danach jedes Bild bis 8 ms, erst die der Lichter im Bild (nach Rang), dann alle anderen. Wer wartet, fehlt (sonst leuchtete es kurz durch die Waende); die Kamera-Ecke zeigt „Schatten n / m“, die Ansicht zeichnet weiter, bis alle fertig sind (R1: Anlass 256). Hangar (576 Lichter): alle im ersten Bild.
- **Atlas statt zweier Wuerfel-Felder** (`bhed/schattenatlas.h`, `kartegpu::schattenAtlasSeiten`, `schattenWuerfel`): Seiten 3072 x 2048 R16_FLOAT (12 MiB), je Seite eine Groesse (4 zu 512, 16 zu 256, 64 zu 128, 256 zu 64), die sechs Seiten eines Wuerfels nebeneinander. Gerendert wird jede Seite in ein 512er-Ziel und in den Atlas kopiert. Der Shader waehlt Seite und st wie TextureCube (groesste Komponente, D3D-Seiten) und liest mit Load (Punktabtastung wie bisher); die Ecke des Wuerfels steht im Licht (`mehr.x`), die Groesse in `zusatz.w`. Der Atlas wird so gross, wie die Lichter brauchen, und schrumpft wieder (halb leer: neu verteilen, leere Seiten am Ende weg) - vorher immer 168 MiB fuer die Felder.
- **Groesse nach der Huelle**: die kleinste Stufe, deren Texel am Rand der Huelle (2 x Huelle / Groesse) hoechstens 3 Einheiten misst (512 ab Huelle 385, 256 ab 193, 128 ab 97, sonst 64). Gemessen gegen lauter 512er (licht1, `echtLichtWuerfelFest`): duel_mos_espa (172 Lichter) an drei Punkten 0,0000 % der Bildpunkte mit mehr als einem Bildpunkt Versatz, Atlas 180 MiB statt 276 MiB. Der Hangar (Huelle 949) bekommt lauter 512er - nah ist das Bild wie vorher: 99,967 % der Bildpunkte im unteren Viertel bytegleich, der Rest +-1 (`li1_h576_rot` gegen `li1_9c_g1`, P0).
- **Budget**: ein Viertel des Budgets der Grafikkarte (DXGI QueryVideoMemoryInfo, lokal), 256 MiB bis 2 GiB, ohne Angabe 1 GiB; hier (RTX 4070 Laptop, 8 GB) 1788 MiB = 149 Seiten. Passt nicht alles, werden die Wuerfel der kleinsten Huellen zuerst eine Stufe kleiner (`wuerfelGroessenVerteilen`); reicht auch 64 nicht, bleiben die kleinsten ohne Wuerfel und leuchten ohne Schatten, die Statuszeile sagt „Echtzeit-Licht: n von m Lichtern ohne Schatten - die Schattenwuerfel nutzen ihr ganzes Budget im Grafikspeicher (x MB)“.
- **Grenze (Zahl der Lichter)**: hier 596 Lichter mit grosser Huelle in voller Groesse (512); darueber werden Wuerfel kleiner (duel_mos_espa, 1202 Lichter: alle mit Wuerfel, Atlas 1788 MiB); ohne Schatten bleiben Lichter erst ab 38 144 (alle zu 64). Mit 2 GiB Budget: 680 / 43 520.
- **Kein Limit 512 je Bild**: der Lichtpuffer waechst. Die Lichter stehen nach der Tiefe ihres Mittelpunkts (Sonnen vorn, eigene Schleife); je Tiefenstufe (128 Stufen, exponentiell ab 8 Einheiten, gleiche Formel in `kartegpu::zbinFuer` und im Shader, eine Stufe Rand je Seite) der Bereich der Lichter, deren Huelle sie trifft (z-binning); ein Bildpunkt prueft nur die Worte dieses Bereichs, und-verknuepft mit der Kachelmaske. Die Kacheln einer Huelle: ausser dem Kasten um die Kugel (`kugelImBild`) je Kachelspalte und -zeile die beiden Ebenen durch das Auge (`kugelKacheln`) - konservativ, das Bild bleibt gleich.
- **Sichtbarkeit ueber vis**: nicht umgesetzt. Die Karte im Editor ist meist nicht gebaut, und die .prt/.bsp des Schnell-Backens gilt nur fuer dessen Stand; das z-binning und die engeren Kacheln nehmen die meisten fernen Lichter schon aus den Bildpunkten (Bildzeit unten).

## Rot und gruen

- licht1 (gleicher Test, ab dem Hangar mit 576 Lichtern): erstes Bild 576 gezeichnet, 0 fehlend, 0 ohne Schatten; Rundgang 0 Wuerfel neu, 0 fehlend, 0 ohne Schatten. Fern gegen gebacken P0 0,17 / 0,63 (43,7 gegen 43,7), P1 0,07 / 0,16, P2 0,15 / 0,35; nah hoechstens 0,22 / 4,07 (`li1_g3`, 16 OK / 0 FEHLER).
- Messung Stufe 9c (`li1_9c_g1`, im Schluss `li1e_9c` gleich): Hangar 576 Mittel 0,27 / Max 31,23 -> 0,09 / 2,83, die Wand am Ende 17,7 -> 43,2 (gebacken 43,3); duel_mos_espa 30,37 / 201,91 -> 2,70 / 199,10 (Schluss 2,59 / 199,15; P0 39,56 -> 5,87, P1 50,39 -> 1,14); Taris (10 939 Lichter) 1,08 / 155,13 -> 1,08 / 155,27 (gleich).

## Bildzeit und Grafikspeicher (allein gemessen)

| Messung | vorher (f550bdad) | nachher |
|---|---|---|
| `leistung` duel_jeditemple, Echtzeit-Licht ruhend: Abstand Mittel / p99, GPU Mittel | 4,43 / 8,81 ms, GPU 2,63 ms | 4,22 / 6,55 ms, GPU 2,37 ms |
| `leistung` duel_jeditemple, Kamera dreht: Abstand Mittel / p99, GPU Mittel | 6,56 / 12,97 ms, GPU 5,66 ms | 4,28 / 8,09 ms, GPU 2,09 ms |
| `leistung` duel_jeditemple, Prozessor je Bild ruhend / dreht | 1,82 / 2,54 ms | 2,63 / 2,72 ms |
| Stufe 7 grosse Karte (duel_mos_espa, 172 Lichter) mit Echtzeit-Licht: Mittel / p99 (Median aus drei Laeufen) | 10,03 / 19,18 ms | 10,14 / 18,07 ms |
| duel_mos_espa (Stufe 9c Bildzeit, Median aus drei Laeufen): Mittel / p99 | 9,45 / 16,85 ms | 9,79 / 18,47 ms |
| Hangar 576 Lichter (Stufe 9c Bildzeit, Kamera dreht): Mittel / p99 | 7,38 / 14,59 ms | 6,23 / 9,40 ms |
| Grafikspeicher duel_jeditemple nach dem Laden | 2201 MB | 3162 MB |
| Grafikspeicher Hangar | 400 MB (320 Lichter) | 1963 MB (576 Lichter, 144 Seiten) |

- Laeufe: vorher `li1_zvor_*`, `li1_z2rot*`; nachher `li1_zende_*`, `li1_z2ende*` (die exe des Schlusses), je allein. duel_mos_espa streut stark (166 fx_runner, Teilchen): vorher Mittel 9,64 / 21,16 / 10,03 ms (Stufe 7) und 9,45 / 8,85 / 12,12 ms (9c), nachher 10,14 / 10,31 / 9,86 bzw. 11,18 / 9,78 / 9,79 - gleich innerhalb der Streuung; die Tabelle nennt den Median.
- Der Prozessor rechnet je Bild mehr (duel_jeditemple +0,8 ms ruhend): alle Lichter statt hoechstens 512, ihre Tiefe und die engeren Kacheln. Die Bildzeit (Abstand) ist trotzdem kleiner, weil die Grafikkarte weniger tut.
- Der Grafikspeicher waechst um die Wuerfel, die vorher fehlten (vorher fest 168 MiB fuer 128 Wuerfel); auf kleinen Karten ist er kleiner als vorher (ein Licht: eine Seite, 12 MiB).
- Zwischenstaende (`li1_znach_*`, `li1_zg4_*`): mit z-binning, aber noch ohne die engeren Kacheln war ruhend GPU 3,47 ms (schlechter als vorher, weil jetzt alle Lichter da sind); ohne z-binning 6,39 ms; ohne jede Schattenprobe unveraendert 3,48 ms - die Zeit lag in der Lichtliste, nicht in den Wuerfeln. Erst die engeren Kacheln brachten ruhend unter vorher.

## Schluss

- Sauberer Neubau (`--clean-first`, `li1e_neubau`) 0 Warnungen, 0 Fehler; danach nur noch das Verschieben des Atlas nach `bhed/schattenatlas.h` (ctest `karte_lizenz` verlangt in `src/karte` einen Herkunftskopf aus NRC/OpenJK/q3map2 - der Atlas ist eigener Code) und die Maus aus dem Handbuchbild: Bau 0 Warnungen.
- ctest -j 16: 220 von 220 (nach dem Verschieben; vorher 218: `karte_lizenz` und `handbuch_sprachen` bis zu den neuen Bildern).
- Selbsttests parallel (`li1e_*`): `karteneditor` licht1 16/0 (mit Groessenvergleich duel_mos_espa), licht9c (Hangar, Taris, duel_mos_espa) 15/0, Stufe 7 78/0, Stufe 9c 23/0, Stufe 9d 28/0, `patchlicht` 11/0, `editorportal` 15/0, `dock1` 99/0, `karteneditor` fotos 78/0, ruhen 32/0, Modus 1 2651/0.
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer ok; portability 98 (wie F6b).
- Handbuch (6 Sprachen): Karten-Editor: Tempo und Echtzeit-Licht - der Schritt zu den Lichtern neu (ganze Karte, Vorberechnen, Budget und Statuszeile), neues Bild `h_karte7_hangar` (der Hangar vom Anfang, ganzes Fenster 2538 breit) aus dem Selbsttest `handbuch` (`BHED_HB_NUR=karte7`, je Sprache); die uebrigen Bilder der Seite unveraendert gelassen. Neue Programmtexte in `tools/i18n_rc610_licht1.py` (Fortschritt, Grenze; 6 Sprachen).

## Offen

- Gegen das Spiel nichts gemessen (kein Spielstart): Vergleich ist das gebackene Licht von bhed_q3map2 (-light -fast) wie in Stufe 9c.
- Sichtbarkeit ueber vis nicht umgesetzt (siehe oben).
- Der Groessenvergleich gegen lauter 512er geht nur, wo alle 512er ins Budget passen (duel_mos_espa ohne geschaetzte Lichter: 172; mit 1202 nicht). Naeher als die Kamerapunkte der Stufe 9c (z. B. dicht vor einer Wand neben einem kleinen Licht) ist ein 256er oder kleinerer Wuerfel weicher als ein 512er; gemessen ist nur an diesen Punkten.
- Ueber dem Budget sind Wuerfel kleiner und damit Schatten weicher als bis rc609 fuer die 32 staerksten Lichter (rc609 gab ihnen 512, egal wie viele Lichter die Karte hat).

## Changelog

### English
- Map editor, real-time light: every light whose reach touches the picture is drawn with its shadows - also far away and however many lights the map has. Standing at the start of a long hangar now shows the light at its far end.
- Map editor, real-time light: each light keeps its shadow cube until something changes within its reach. Switching the light on computes all cubes first (the camera shows the progress on large maps); walking around computes nothing new.
- Map editor, real-time light: shadow cubes take the graphics memory they need, up to a quarter of the card's budget; lights with a small reach get smaller cubes, and if the budget is not enough the status bar says how many lights are without shadows.
- Map editor, real-time light: faster with many lights - each pixel only checks the lights near its depth.

### Deutsch
- Karten-Editor, Echtzeit-Licht: Jedes Licht, dessen Reichweite das Bild berührt, wird mit seinen Schatten gezeichnet - auch in der Ferne und gleich, wie viele Lichter die Karte hat. Am Anfang eines langen Hangars ist jetzt auch das Licht an seinem Ende zu sehen.
- Karten-Editor, Echtzeit-Licht: Jedes Licht behält seinen Schattenwürfel, bis sich in seiner Reichweite etwas ändert. Beim Einschalten werden zuerst alle Würfel gerechnet (bei großen Karten zeigt die Kamera den Fortschritt); ein Rundgang rechnet nichts neu.
- Karten-Editor, Echtzeit-Licht: Die Schattenwürfel nehmen sich den Grafikspeicher, den sie brauchen, bis zu einem Viertel des Budgets der Grafikkarte; Lichter mit kleiner Reichweite bekommen kleinere Würfel, und reicht das Budget nicht, sagt die Statuszeile, wie viele Lichter ohne Schatten sind.
- Karten-Editor, Echtzeit-Licht: schneller mit vielen Lichtern - jeder Bildpunkt prüft nur die Lichter nahe seiner Tiefe.

### 中文
- 地图编辑器,实时光照:凡是照射范围触及画面的光源都会带着阴影绘制——包括远处的,无论地图有多少光源。站在长机库的起点,现在也能看到机库尽头的光。
- 地图编辑器,实时光照:每个光源保留自己的阴影立方体,直到其照射范围内有东西改变。打开时先计算所有立方体(大地图上摄像机会显示进度);四处走动不会重新计算。
- 地图编辑器,实时光照:阴影立方体按需占用显存,最多为显卡预算的四分之一;照射范围小的光源使用较小的立方体,预算不够时状态栏会显示有多少个光源没有阴影。
- 地图编辑器,实时光照:光源多时更快——每个像素只检查其深度附近的光源。

### 日本語
- マップエディター、リアルタイムライト: 届く範囲が画面に触れるライトはすべて影付きで描画されます。遠くのものも、マップのライトの数にかかわらずです。長い格納庫の入口に立っても、奥の端の光が見えるようになりました。
- マップエディター、リアルタイムライト: 各ライトは届く範囲で何かが変わるまで自分の影キューブを保持します。オンにすると最初にすべてのキューブを計算し(大きなマップではカメラに進行状況が表示されます)、歩き回っても再計算はありません。
- マップエディター、リアルタイムライト: 影キューブはグラフィックカードの予算の 4 分の 1 まで必要なグラフィックメモリを使います。届く範囲が小さいライトは小さいキューブになり、予算が足りない場合は影のないライトの数をステータスバーに表示します。
- マップエディター、リアルタイムライト: ライトが多いときに速くなりました。各ピクセルは自分の深度に近いライトだけを調べます。

### Español
- Editor de mapas, luz en tiempo real: toda luz cuyo alcance toca la imagen se dibuja con sus sombras, también lejos y sin importar cuántas luces tenga el mapa. Desde el principio de un hangar largo ahora se ve también la luz de su extremo.
- Editor de mapas, luz en tiempo real: cada luz conserva su cubo de sombra hasta que algo cambia dentro de su alcance. Al activarla se calculan primero todos los cubos (en mapas grandes la cámara muestra el progreso); recorrer el mapa no recalcula nada.
- Editor de mapas, luz en tiempo real: los cubos de sombra usan la memoria gráfica que necesitan, hasta una cuarta parte del presupuesto de la tarjeta; las luces de alcance pequeño reciben cubos más pequeños y, si el presupuesto no alcanza, la barra de estado indica cuántas luces quedan sin sombras.
- Editor de mapas, luz en tiempo real: más rápida con muchas luces; cada píxel solo comprueba las luces cercanas a su profundidad.

### Čeština
- Editor map, světlo v reálném čase: každé světlo, jehož dosah zasahuje do obrazu, se kreslí se svými stíny - i v dálce a bez ohledu na to, kolik světel mapa má. Na začátku dlouhého hangáru je teď vidět i světlo na jeho konci.
- Editor map, světlo v reálném čase: každé světlo si ponechá svou stínovou krychli, dokud se v jeho dosahu něco nezmění. Při zapnutí se nejprve spočítají všechny krychle (na velkých mapách kamera ukazuje průběh); procházka mapou nic znovu nepočítá.
- Editor map, světlo v reálném čase: stínové krychle si berou grafickou paměť, kterou potřebují, až do čtvrtiny rozpočtu grafické karty; světla s malým dosahem dostanou menší krychle, a pokud rozpočet nestačí, stavový řádek řekne, kolik světel je bez stínů.
- Editor map, světlo v reálném čase: rychlejší s mnoha světly - každý pixel kontroluje jen světla blízko své hloubky.
