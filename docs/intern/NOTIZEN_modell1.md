# Notizen Model-Tab, Leistung der Figurendarstellung (Zweig modell1)

Auftrag: SP\auftrag_modelltab.md mit SP\auftrag_gemeinsam.md; Worktree Y:\sprache_es, Zweig modell1
ab karte1 fdb3b348. shank: der Model-Tab (Modus 2) wirkt „leicht laggy“, wie vorher der
Menü-Editor (82 ms → 1–2 ms nach dem Umstieg auf den GPU-Weg).

## Commits

| Commit | Inhalt |
|---|---|
| bb6951be | Selbsttest `modellleistung`: Messung je Figur und Lage, Zeitmarken des Modellfensters, Fotos, Ruhe-Prüfungen (rot) |
| fabb0f6c | Ruhe: Animation und Drehteller laufen flüssig weiter, der Model-Tab ruht, wenn nichts sich bewegt (grün) |
| d97e71f7 | Animationsliste und NPC-Browser nur mit den sichtbaren Zeilen (ImGuiListClipper), Knochenansicht ohne Neuberechnung |
| 12acb6d7 | Vorschau-Figuren: Flächenliste des Berichts je Bild nur zählen, Zeilen einmal je Art |

## Befund

Die Figur selbst ging schon über die Grafikkarte: Ecken- und Indexpuffer je Modell einmal hochgeladen
(`holeFigurPuffer`), Bilder je Inhalt geteilt (`holeTexBild`), Skinning im Vertexshader
(Knochenmatrizen im Konstantenpuffer), Neuzeichnen des Modellfensters nur bei `modelDirty`. Ein
Neuzeichnen kostet 0,2–0,4 ms CPU und 0,02–0,05 ms Grafikkarte (21–72 Aufrufe, keine Uploads außer
den Konstantenpuffern). Das „laggy“ kam von woanders:

1. **Ruhe (rc596) im Model-Tab falsch.** `darfRuhen` kannte weder Animation noch Drehteller: nach einer
   Sekunde ohne Eingabe wartete die Hauptschleife bis zu 250 ms je Bild – Animation und Drehteller
   liefen mit vier Bildern je Sekunde, sobald die Maus stillstand (sofern vorher eine Karte gezeichnet
   war). Umgekehrt ruhte der Model-Tab nie, solange `mapDirty` stand: die Karte wird dort nicht
   gezeichnet, also wurde die Marke nie gelöscht, und die Oberfläche lief ohne Eingabe mit voller
   Bildrate (hier 240 Hz).
2. **Listen ohne Clipper.** Die Animationsliste (_humanoid: 1837 Einträge) und der NPC-Browser (1323
   Typen) bauten jedes Bild jede Zeile; das Schleifen-Icon ging auch für verdeckte Zeilen ins
   Zeichenbuch. Das war fast die ganze CPU-Zeit des Bildes im Model-Tab.
3. Kleinere Arbeit je Bild für Unverändertes: die Knochenansicht rechnete die Weltmatrizen jedes Bild
   neu; in der Vorschau baute die Flächenliste des Berichts je Fläche und Figur eine Zeichenkette.

## Messverfahren

`BHED_EDITORTEST=modellleistung` (gui/selbsttest_modell1.inc). Je Figur und Lage 12 Bilder Anlauf,
120 gemessen. Bild-CPU = Bildanfang (`selbsttestVorBild`, vor `ImGui::NewFrame`) bis Selbsttest am
Ende von `draw()`, ohne Render/Present; während der Messung ist das Element-Register des Selbsttests
aus (im Alltag gibt es den Haken nicht). Modellfenster: `App::modellCpuMsSumme`/`modellNeuGezeichnet`/
`modellAufrufe`, Grafikkarte über eigene Zeitmarken (`gpu::modellGpuMs`, das Modellfenster liegt
außerhalb des Kartenrings). Figuren: stormtrooper (JKA), jedi_hf (JKA, 214 Flächen), jedi_hm (MD,
236 Flächen), clonetrooper_p2 (MD, 28460 Ecken in LOD 0) – ausgesucht mit einem Blick in alle
model.glm der Spielordner. `BHED_MODELL1_NUR=ruhe`: nur die Ruhe-Prüfungen.

**Wichtig für alle Messungen:** Im Selbsttest läuft die D3D-Debugschicht (backend_win32.cpp,
`BHED_EDITORTEST` ohne `BHED_D3D_DEBUG=0`). Sie kostet je Aufruf ein Vielfaches – md_am_sith: Figuren
19,7 ms mit, 1,1 ms ohne. Die Endmessungen laufen deshalb mit `BHED_D3D_DEBUG=0`.

## Messtabelle Model-Tab

Ohne D3D-Debugschicht, **unter Last** (17 MSBuild, 3 fremde jkstudio), Normalpriorität, je zwei Läufe
abwechselnd vorher (bb6951be) / nachher (12acb6d7), Mittel der Läufe. Bild-CPU in ms (Mittel / p99),
Modellfenster je Neuzeichnen: CPU ms / Grafikkarte ms / Aufrufe. Bildabstand in allen Lagen 4,2 ms
(240 Hz, Vsync). Hochgeladene Bilder je Messblock: 0.

| Figur | Lage | Bild-CPU vorher | Bild-CPU nachher | Modell vorher | Modell nachher |
|---|---|---|---|---|---|
| ohne Figur | ruhig | 0,17 / 0,33 | 0,21 / 0,32 | – | – |
| stormtrooper | ruhig | 3,71 / 15,89 | 0,35 / 0,77 | – | – |
| stormtrooper | Animation | 3,73 / 16,64 | 0,36 / 0,79 | 0,199 / 0,061 / 72 | 0,072 / 0,033 / 72 |
| stormtrooper | Drehteller + Animation | 3,42 / 8,39 | 0,42 / 0,96 | 0,157 / 0,079 / 72 | 0,070 / 0,035 / 72 |
| stormtrooper | Knochen + Animation | 3,35 / 8,11 | 0,32 / 0,55 | 0,175 / 0,079 / 72 | 0,073 / 0,034 / 72 |
| stormtrooper | NPC-Browser offen | 2,99 / 6,32 | 0,35 / 0,61 | – | – |
| jedi_hf | ruhig | 3,08 / 11,45 | 0,35 / 0,85 | – | – |
| jedi_hf | Animation | 3,09 / 8,31 | 0,36 / 0,85 | 0,171 / 0,077 / 21 | 0,073 / 0,041 / 21 |
| jedi_hf | Drehteller + Animation | 3,05 / 7,54 | 0,40 / 0,88 | 0,092 / 0,058 / 21 | 0,063 / 0,040 / 21 |
| jedi_hf | Knochen + Animation | 2,90 / 7,58 | 0,39 / 0,66 | 0,103 / 0,071 / 21 | 0,073 / 0,043 / 21 |
| jedi_hf | NPC-Browser offen | 3,72 / 9,50 | 0,41 / 0,90 | – | – |
| jedi_hm | ruhig | 1,71 / 3,95 | 0,34 / 0,46 | – | – |
| jedi_hm | Animation | 1,58 / 3,45 | 0,36 / 0,78 | 0,068 / 0,038 / 28 | 0,078 / 0,015 / 28 |
| jedi_hm | Drehteller + Animation | 1,53 / 2,46 | 0,44 / 0,62 | 0,061 / 0,034 / 28 | 0,071 / 0,021 / 28 |
| jedi_hm | Knochen + Animation | 1,50 / 2,47 | 0,36 / 0,79 | 0,067 / 0,031 / 28 | 0,071 / 0,017 / 28 |
| jedi_hm | NPC-Browser offen | 2,69 / 6,28 | 0,39 / 0,87 | – | – |
| clonetrooper_p2 | ruhig | 1,65 / 2,58 | 0,38 / 2,50 | – | – |
| clonetrooper_p2 | Animation | 1,96 / 3,58 | 0,56 / 7,39 | 0,089 / 0,025 / 42 | 0,091 / 0,016 / 42 |
| clonetrooper_p2 | Drehteller + Animation | 2,10 / 3,96 | 0,51 / 2,27 | 0,083 / 0,026 / 42 | 0,104 / 0,016 / 42 |
| clonetrooper_p2 | Knochen + Animation | 2,04 / 3,81 | 0,35 / 0,61 | 0,084 / 0,024 / 42 | 0,069 / 0,017 / 42 |
| clonetrooper_p2 | NPC-Browser offen | 3,28 / 6,60 | 0,39 / 0,83 | – | – |

Mit Debugschicht (wie jeder Selbsttest, ebenfalls unter Last, je zwei Läufe): Bild-CPU vorher 1,5–3,0 ms,
nachher 0,2–0,8 ms; ein Neuzeichnen des Modellfensters 0,16–0,43 ms CPU.

### Endmessung, ruhige Maschine

Kein cl.exe, kein fremdes jkstudio, CPU-Last 3–23 % vor den Läufen; sonst wie oben (ohne Debugschicht,
Normalpriorität, je zwei Läufe abwechselnd, vorher bb6951be / nachher 12acb6d7 sauber neu gebaut).
Bildabstand 4,16 ms (p99 4,5–4,7). Die zehn Fotos auch hier Bildpunkt für Bildpunkt gleich.

| Figur | Lage | Bild-CPU vorher | Bild-CPU nachher | Modell vorher | Modell nachher |
|---|---|---|---|---|---|
| ohne Figur | ruhig | 0,11 / 0,23 | 0,12 / 0,32 | – | – |
| stormtrooper | ruhig | 1,02 / 2,96 | 0,17 / 0,35 | – | – |
| stormtrooper | Animation | 1,06 / 2,63 | 0,18 / 0,43 | 0,043 / 0,038 / 72 | 0,040 / 0,013 / 72 |
| stormtrooper | Drehteller + Animation | 1,11 / 2,77 | 0,22 / 0,52 | 0,038 / 0,036 / 72 | 0,032 / 0,019 / 72 |
| stormtrooper | Knochen + Animation | 1,12 / 3,11 | 0,20 / 0,53 | 0,040 / 0,024 / 72 | 0,047 / 0,017 / 72 |
| stormtrooper | NPC-Browser offen | 1,81 / 4,50 | 0,21 / 0,50 | – | – |
| jedi_hf | ruhig | 1,26 / 2,90 | 0,23 / 0,48 | – | – |
| jedi_hf | Animation | 1,09 / 2,36 | 0,21 / 0,46 | 0,043 / 0,042 / 21 | 0,045 / 0,026 / 21 |
| jedi_hf | Drehteller + Animation | 1,32 / 4,49 | 0,28 / 0,47 | 0,036 / 0,060 / 21 | 0,040 / 0,044 / 21 |
| jedi_hf | Knochen + Animation | 1,16 / 2,50 | 0,21 / 0,46 | 0,042 / 0,059 / 21 | 0,044 / 0,030 / 21 |
| jedi_hf | NPC-Browser offen | 1,65 / 3,08 | 0,20 / 0,47 | – | – |
| jedi_hm | ruhig | 1,00 / 2,37 | 0,21 / 0,50 | – | – |
| jedi_hm | Animation | 1,52 / 5,82 | 0,21 / 0,55 | 0,061 / 0,033 / 28 | 0,045 / 0,034 / 28 |
| jedi_hm | Drehteller + Animation | 1,18 / 2,65 | 0,23 / 0,54 | 0,034 / 0,036 / 28 | 0,035 / 0,018 / 28 |
| jedi_hm | Knochen + Animation | 1,12 / 2,56 | 0,20 / 0,45 | 0,044 / 0,043 / 28 | 0,046 / 0,019 / 28 |
| jedi_hm | NPC-Browser offen | 1,54 / 3,35 | 0,22 / 0,43 | – | – |
| clonetrooper_p2 | ruhig | 1,01 / 3,31 | 0,24 / 0,58 | – | – |
| clonetrooper_p2 | Animation | 0,93 / 2,27 | 0,24 / 0,56 | 0,038 / 0,055 / 42 | 0,051 / 0,019 / 42 |
| clonetrooper_p2 | Drehteller + Animation | 1,24 / 2,66 | 0,23 / 0,57 | 0,042 / 0,041 / 42 | 0,037 / 0,017 / 42 |
| clonetrooper_p2 | Knochen + Animation | 1,19 / 2,79 | 0,20 / 0,46 | 0,057 / 0,041 / 42 | 0,042 / 0,018 / 42 |
| clonetrooper_p2 | NPC-Browser offen | 1,75 / 3,62 | 0,24 / 0,60 | – | – |

Ergebnis: Bild-CPU im Model-Tab 0,9–1,8 ms (p99 bis 5,8) → 0,17–0,28 ms (p99 bis 0,60), also auf
ein Fünftel bis ein Achtel; das Neuzeichnen der Figur selbst war schon billig (0,03–0,06 ms CPU,
0,01–0,06 ms Grafikkarte) und bleibt es.

### Ruhe

Ruhe (`BHED_MODELL1_NUR=ruhe`, Idle-Priorität, mit Debugschicht):

| Lage | vorher | nachher |
|---|---|---|
| Animation, Karte gezeichnet | 11 Bilder in 2,86 s (rot) | 342 in 1,53 s |
| Drehteller, Karte gezeichnet | 11 in 2,88 s (rot) | 363 in 1,52 s |
| Figur steht, `mapDirty` | 361 in 1,52 s (rot) | 11 in 2,87 s |

## Vorschau / Mission-Tab (Punkt 3)

Derselbe Code ist `zeichneFigurIn` (Puffer, Bilder, Skinning). Gemessen mit `figurleistung`
(md_am_sith, intro2_sith bei 26,5 s, 1752 Figurenaufrufe, schneller Stufenweg), ohne Debugschicht:
Bild 7,7 ms, Figuren-CPU 1,07 ms, Grafikkarte 0,60 ms. Profil mit vorübergehenden Zeitmarken (nicht
eingecheckt): Knochen 0,10–0,18 ms, Flächenliste des Berichts 0,12–0,21 ms, `zeichneFigur` 0,19–0,53
ms je Bild; in `zeichneFigurIn` fast alles in der Schleife über die Teile (Puffer, Knochenpuffer und
Aufbau zusammen unter 3 µs je Figur). Die Flächenliste wird jetzt je Bild nur gezählt (Schlüssel: die
Zeichenketten der Fläche), die Zeilen entstehen einmal je Art; der Berichtsabschnitt „Flaechen der
Figuren“ ist alt und neu gleich (42 Zeilen, gleiche Prüfsumme). Die ganze Figurenzeit streut unter
Last stärker als der Gewinn (Median alt 1,07/1,83 ms, neu 1,60/1,00 ms) – der Gewinn ist nur im Profil
zu sehen.

Nicht geändert, weil schon so: Puffer/Bilder einmal hochladen, Skinning auf der Grafikkarte,
Zustände nur bei Wechsel setzen, Stufenwerte je Fläche in einem Map (Ring).

## Bildvergleich

Fotos aus `modellleistung` in fester Lage (Bild 5 von BOTH_RUN1, eingepasste Kamera; mit und ohne
Knochen; Gruppe TORSO; NPC-Browser), vorher (bb6951be) gegen nachher: alle zehn Bildpunkt für
Bildpunkt gleich (2538 × 1472). Das Foto `modell_knochen` des Selbsttests `modell` streut schon
zwischen zwei Läufen desselben Programms (Drehteller-Winkel hängt an der Bildzeit) und taugt nicht
zum Vergleich.

## Tests

Schlussrunde mit dem sauber neu gebauten Stand 12acb6d7 (alle Ziele, `--clean-first`, 526 Übersetzungen,
**0 Warnungen**), Läufe mit Idle-Priorität außer den Messungen:

| Prüfung | Ergebnis |
|---|---|
| ctest | **166/166** bestanden (99 s, `-j 4`) |
| lint_sources, lint_fassung | ok (lint_d3dbesitz 20 / lint_portability 217 Beanstandungen wie vorher, keine in geänderten Stellen) |
| Selbsttest `modellleistung` | 7 OK / 0 FEHLER (vorher 4 OK / 3 FEHLER: die drei Ruhe-Prüfungen) |
| Selbsttest `modell` | 13 OK / 0 FEHLER |
| Selbsttest Modus 1 | **2648 OK / 0 FEHLER** (mit `BHED_HANDBUCH`; ohne findet ein Laufordner außerhalb des Worktrees `touren.json` nicht – erster Versuch abgebrochen) |
| `karteneditor` kurz (`BHED_KARTE_NUR=fotos`) | 78 OK / 0 FEHLER |
| `karte` (wegen `darfRuhen`: rc596-Ruhe, Gerätverlust) | 226 OK / 1 FEHLER unter Last (Idle-Priorität): „Spulen: zurück so flüssig wie vor“ – Bilder 69–103 ms, davon gemessen nur ~5 ms Arbeit, der Rest Warten auf die CPU; Ruhe 13 Bilder in 3,35 s (ok). Allein wiederholt (Modus `spulen`, Normalpriorität): **13 OK / 0 FEHLER** |
| `figurleistung` (Punkt 3) | 1 OK / 0 FEHLER, je zwei Läufe alt/neu ohne Debugschicht |

Handbuch: keine Änderung nötig – es beschreibt weder Ruhe noch Bildrate, Bedienung und Aussehen sind
gleich (Bildvergleich). Keine neuen Programmtexte (kein `tools/i18n_modell1.py`).

## Offen

- Der Map-Tab ohne geladene Karte ruht weiterhin nie (`mapDirty` bleibt stehen, weil
  `gpuBereit` ohne Netz falsch ist) – außerhalb dieses Auftrags, nicht geändert.
- `g_figurBilder` behält die Bilder alter Häute/Modelle (Schlüssel: Adresse) bis zum Neuaufbau – kein
  Bild-für-Bild-Aufwand, aber Speicher; nicht geändert.

## Changelog

## English
- Model tab: a playing animation and the turntable stay smooth when the mouse stands still (they
  dropped to four frames per second); the tab no longer redraws at full rate while nothing moves.
- Model tab: the animation list and the NPC browser only build the visible rows - the frame time in
  the Model tab drops from about 1-2 ms to about 0.2 ms.
- Bone view and preview figures: no more per-frame work for things that did not change.

## Deutsch
- Model-Tab: Eine laufende Animation und der Drehteller bleiben flüssig, wenn die Maus stillsteht
  (vorher vier Bilder je Sekunde); ohne Bewegung zeichnet der Tab nicht mehr mit voller Bildrate.
- Model-Tab: Animationsliste und NPC-Browser bauen nur noch die sichtbaren Zeilen – die Bildzeit im
  Model-Tab sinkt von etwa 1–2 ms auf etwa 0,2 ms.
- Knochenansicht und Figuren der Vorschau: keine Arbeit je Bild mehr für Unverändertes.

## 中文
- 模型选项卡：鼠标静止时，正在播放的动画和转台保持流畅（之前降至每秒四帧）；没有任何变化时不再以全帧率重绘。
- 模型选项卡：动画列表和 NPC 浏览器只构建可见的行——模型选项卡的帧时间从约 1–2 毫秒降至约 0.2 毫秒。
- 骨骼视图和预览中的角色：不再为未改变的内容逐帧计算。

## 日本語
- モデルタブ：マウスを止めても、再生中のアニメーションとターンテーブルが滑らかに動き続けます（以前は毎秒4フレームに低下）。何も動いていないときはフルフレームレートで再描画しなくなりました。
- モデルタブ：アニメーション一覧と NPC ブラウザーは表示中の行だけを構築します。モデルタブのフレーム時間は約 1–2 ms から約 0.2 ms に下がりました。
- ボーン表示とプレビューのキャラクター：変化のないものに対する毎フレームの処理をなくしました。
