# NOTIZEN GITTER (13.11.2026): alphashadow-Gitter mit alphaFunc LT128 - q3map2, Ravens q3map, Spiel, Echtzeit-Licht

Auftrag GITTER (Hauptchat 13.11.). Zweig `p_gitter`, Worktree `Y:\p_gd`, Werkzeuge und Belege `Y:\werkzeuge\gitter`.
Ausloeser: `NOTIZEN_dxr_1211.md`, Abschnitt "Offen" - Strahlen und Schattenkarten lagen unter dem Testgitter 24 bis 26 %
unter der gebackenen Lightmap, und q3map2 laesst bei `alphaFunc LT128` das Licht durch die Staebe und sperrt es an den
Loechern.

## Ergebnis

**Nichts zu aendern - weder im Compiler noch im Echtzeit-Licht.** q3map2 macht es wie Ravens q3map: Das Licht geht dort
durch, wo das Bild wenig Alpha hat, egal welche `alphaFunc` die Stufe hat. Bei `alphaFunc LT128` zeichnet das Spiel dort aber
die Staebe. Physikalisch ist das verkehrt herum, in den Originalkarten ist es aber genau so gebacken (vjun2). Nach der Regel
"wie das Spiel" ist das Verhalten richtig, es ist kein NRC-Fehler. Einen Schalter gibt es deshalb nicht, und
Licht-Cache-Abdruck, Echtzeit-Licht und Handbuch bleiben, wie sie sind.

Die 24 bis 26 % kommen nicht von der Regel, sondern von der Abtastung der Lightmap (ein Strahl je Luxel, Luxel 16 Einheiten,
bilinear im Renderer). Die Mittelwerte von gebacken und Echtzeit liegen nur zufaellig so nah beieinander (Abschnitt 4).

## 1. Was q3map2 tut (NRC, `third_party/q3map2/tools/quake3/q3map2`)

- `light_trace.cpp:1312-1356` (`TraceTriangle`): Ein Dreieck ohne `C_ALPHASHADOW`/`C_LIGHTFILTER` oder ohne Bild deckt ganz.
  Mit alphashadow nimmt die Funktion den Texel bei (s, t) ohne Filter (`int( s * width )`, wiederholt) und rechnet
  `trace->color *= ( 255 - pixel[3] ) / 255` (Zeile 1350-1356), stufenlos. Unter 0,001 je Kanal gilt der Strahl als gedeckt.
- Das Bild ist `si->lightImage`, und das ist das Editorbild (`qer_editorimage`), sonst der Shadername, die implizite Map oder
  die erste `map` (`shaders.cpp:663-706`). Welche Stufe die `alphaFunc` traegt, spielt keine Rolle.
- **`alphaFunc` liest q3map2 nirgends.** Das Wort steht nur in einem erzeugten Text fuer eigene Shader (`shaders.cpp:392`).
  GT0, LT128 und GE128 backen deshalb gleich: Der Durchlass ist immer 255 - Alpha.
- `forceSubsampling` (Zeile 1321) wirkt nur mit `-samples` > 1 oder `-randomsamples` (`light_ydnar.cpp:4710`, `4780`). Ohne
  diese Schalter bekommt jedes Luxel genau einen Strahl, und zwar aus seiner Mitte (`DEFAULT_LIGHTMAP_SAMPLE_SIZE` 16,
  `q3map2.h:167`).

**Testkarte mit drei Gittern** (`Y:\werkzeuge\gitter\drei_gitter.py`): drei gleiche, getrennte Raeume mit je einem Gitter
128 x 128. Das Bild ist ueberall `textures/kejim/grate02_broke`, die Shader unterscheiden sich nur in `alphaFunc GT0 / LT128 /
GE128`, alle mit `surfaceparm alphashadow`. Das Licht steht 64 ueber dem Gitter, das Gitter 64 ueber dem Boden, Luxel 4
(`_lightmapscale 0.25`), Spieldaten Movie Duels.
- `-light -fast` und `-light`: Das Mittel unter jedem Gitter ist 30,36. GT0 im linken Raum und GE128 im rechten sind
  Bildpunkt fuer Bildpunkt gleich (max. Abstand 0,0000).
- Der mittlere Raum weicht an einzelnen Luxeln um bis zu 5,27 ab. Das kommt von der Lage und nicht von alphaFunc: Mit
  vertauschter Reihenfolge (LT128 links, GT0 in der Mitte) ist links = rechts und die Mitte weicht genauso ab
  (`lauf_drei_tausch`).
- Bild `Y:\werkzeuge\gitter\lauf_drei\drei_sichtbar_und_lightmap.png`: Links steht je Zeile, was das Spiel zeichnet, rechts
  die Boden-Lightmap. Bei GT0 und GE128 zeichnet das Spiel die Felder (Alpha 255) und das Licht faellt durch die Schlitze
  (Alpha 0). Bei LT128 zeichnet es die duennen Staebe, und genau dort faellt das Licht durch.
- Falle beim Nachbauen: Traegt der Gitterpinsel auf den uebrigen Seiten `common/caulk`, ist der ganze Pinsel deckend und der
  Boden schwarz. Ebenso schwarz wird er mit `GameData\base`, weil eine lose `shaderlist.txt` dort den Testshader ausschliesst.

## 2. Was Ravens q3map und das Spiel tun

- **Spiel** (OpenJK, `code/rd-vanilla/tr_shader.cpp:291-293`, `tr_backend.cpp:378-380`): `LT128` wird zu `GLS_ATEST_LT_80`,
  also `glAlphaFunc( GL_LESS, 0.5 )`. Gezeichnet wird, wo Alpha < 128 ist. Bei `grate02_broke` sind das die Staebe (Alpha 0,
  39 % der Bildpunkte), die Felder (Alpha 255) sind Loecher.
- **Quellcode von Ravens q3map:** nicht veroeffentlicht. Die GPL-Quellen von JK2/JKA (2013) enthalten nur das Spiel, und im
  Netz ist nichts zu finden (Suchen nach Ravens q3map, sof2map und den Werkzeugen der SDK). Auf der Platte liegt kein Ravens
  q3map.exe. Nach ydnar hat q3map2 Ravens Schreibweise `q3map_<surfaceparm>` bewusst als surfaceparm uebernommen
  (forums.splashdamage.com, "Alphashadow - what do i wrong?").
- **ids q3map**, die Grundlage von Ravens q3map (GPL, github.com/id-Software/Quake-III-Arena, `q3map/light_trace.c`,
  `SetFacetFilter`): `b = image[3]; b = b < 128 ? 0 : 255; filter *= (255 - b) / 255`. Die Richtung ist dieselbe wie bei
  q3map2 (viel Alpha sperrt), nur hart an der Schwelle 128 statt stufenlos. Auch dort wird alphaFunc nirgends gelesen.
- **Original-Karten (nur gelesen):** In den JKA-Daten hat genau ein alphashadow-Shader `alphaFunc LT128`, naemlich
  `textures/kejim/grate02_broke` (`imperial.shader`, assets1). Er kommt in t1_fatal, t2_rancor und vjun2 vor
  (`alphafunc_suche.py`, Liste `alphafunc_suche.txt`). In JK2 gibt es keinen einzigen alphashadow-Shader
  (`alphafunc_suche_jk2.txt` ist leer).
  - **vjun2:** Das Gitter liegt bei z 624 (Flaechen 13601-13613) ueber einem Boden bei z 496 (Flaeche 6276) mit Luxeln von
    nur 4 Einheiten. Die Lichter sind aus der .bsp entfernt. Am Boden liegt das vergroesserte Gittermuster mit einer Periode
    von 32,7 / 31,7 Einheiten (FFT, `vjun2_fit.py`). Das sind die 8er-Felder mal 4, also ein Licht etwa 43 ueber dem Gitter.
  - **Topologie** (`vjun2_topologie.py`, unabhaengig von Lichtort, Abfall und Linienbreite): Im Bild bilden die Staebe ein
    zusammenhaengendes Netz und die Felder einzelne Inseln. Am Original-Boden gibt es **61 abgeschlossene dunkle Inseln und 1
    helle**. Das Gitterbild selbst, vergroessert und unscharf wie die Luxel, ergibt 134 / 0 fuer "Durchlass 255 - Alpha" und
    0 / 134 fuer "Durchlass wie das Spiel zeichnet". Ravens q3map liess also das Licht durch die Staebe und sperrte es an den
    Loechern, wie q3map2. Bilder: `topologie_vjun2.png`, `lm_vjun2_boden_zoom.png` (helle Linien, dunkle Felder), Decke
    `vjun2_decke_TA_aus.png`.
  - Die ersten Versuche (Punktlicht frei anpassen, `vjun2_fit.py`, Kreuzkorrelation `vjun2_ncc.py`, Schiefe
    `vjun2_schiefe.py`) waren nicht eindeutig: Bei der Grundwelle ist "Regel B" gleich "Regel A um eine halbe Periode
    verschoben". Sie stehen nur zur Nachvollziehbarkeit hier. Entschieden hat die Topologie.
  - Gegenprobe auf einer anderen Karte: nicht moeglich. Bei t1_fatal und t2_rancor haben die Flaechen hinter dem Gitter
    Luxel von 16 bis 64 Einheiten, die Felder sind dort nur 8 bis 16 gross. Die zweite und dritte Stelle in vjun2 (Flaechen
    13614, 14499) haben Luxel von 32. Die 61 Inseln verteilen sich aber auf mehrere Bereiche des Bodens mit verschiedenen
    Lichtern und Vergroesserungen.
- **Doppelte Definition: gibt es nicht.** Den Namen `textures/kejim/grate02_broke` definiert nur `imperial.shader`, in den
  JKA-Daten wie in denen von Movie Duels (`shader_zeigen.py`, `md_doppelt.py`). `bespin.shader` definiert
  `textures/bespin/breakable_grate1`, und der benutzt nur dasselbe Bild. Die Vermutung in `NOTIZEN_lcfix_1211.md` (B3) und
  `NOTIZEN_dxr_1211.md` ist damit hinfaellig; beide sind berichtigt.

## 3. Echtzeit-Licht

Beide Schattenarten haben dieselbe Richtung wie q3map2 und Ravens q3map:
- Strahlen: `gui/karte_dxr.hlsl:66`, `f *= 1.0 - texel.a`, stufenlos wie q3map2.
- Schattenkarten: `gui/karte_gpu_win32.cpp:1757-1760` und `1771-1777`, `if (a < 0.5) discard`. Alpha ab 0,5 deckt, also
  hart wie ids q3map. Das ist bekannt seit 9c ("Alpha-Test mit dem Bild").

Die Fotos des DXR-Selbsttests zeigen dasselbe Muster wie die Lightmap in 1: helle duenne Linien (die Staebe). Einen Fund in
der Regel des Echtzeit-Lichts gibt es nicht.

## 4. Die 24 bis 26 % erklaert

Grundlage sind die Fotos des letzten DXR-Laufs (`Y:\werkzeuge\dxr\laeufe\g11\bilder\dxr_gitter_*.bmp`, 1364 x 780), Boden
unter dem Testgitter (Pinsel +-128, z 38..42, Grate-Shader auf allen Seiten, also zwei Lagen uebereinander), Licht 0 0 96.

| Bild | Mittel | Zeilen 0-100 | Zeilen 100-780 | Bildpunkte > 20 |
|---|---|---|---|---|
| Strahlen | 4,97 | 2,4 | 5,3 | 10,6 % |
| Schattenkarten | 5,12 | 3,2 | 5,4 | 7,1 % |
| gebacken | 6,70 | 34,8 | **0,0** (ab Zeile 240 genau 0) | 14,1 % |

(Zeilenmittel je 40 Bildpunkte, gebacken: 40,2 33,4 24,8 17,7 10,3 3,1 0,0 0,0 ...; `dxr_g11_gitter_drei_x8.png`.)

- **Unten (70 % des Bilds)** zeigen die beiden Echtzeit-Arten je Bildpunkt die duennen Lichtlinien, also die Stellen, an denen
  sich die Staebe beider Lagen decken. Die Lightmap ist dort ueberall 0: Jedes Luxel (16 Einheiten) hat einen einzigen
  Strahl aus seiner Mitte, und der trifft in diesem Ausschnitt nie eine Linie von 3 bis 4 Einheiten Breite.
- **Oben** liegt der Rand des Gitterschattens. Die Echtzeit-Arten zeigen dort Schatten (2,4 / 3,2). In der Lightmap laeuft
  das naechste, schon freie Luxel bilinear als gerade Rampe 16 Einheiten in den Schatten hinein (40 -> 0).
- Die beiden Mittelwerte kommen also aus voellig verschiedenen Bildern und liegen nur zufaellig nahe. An einer anderen
  Kamerastelle oder mit anderer Lage der Luxel zum Muster kommt ein ganz anderes Verhaeltnis heraus.
- Nachgebaut (`lm_abtastung.py`, Testraum wie im Selbsttest mit bhed_q3map2 `-light -fast`): Ueber 70 x 70 Einheiten unter
  dem Gitter ist das Lightmap-Mittel ohne Unterabtastung 21,51, mit `-samples 4` nur 7,61, also ein Faktor 2,8 allein durch
  die Abtastung. (`-samplesize` beim `-light` aendert nichts: Die Luxelgroesse steht schon seit `-meta` in der `.srf`.)
- Das ist derselbe Unterschied wie an jeder Schattenkante: Das Echtzeit-Licht rechnet je Bildpunkt, die Lightmap je Luxel
  ("Schattengrenze gleiche Stelle, schaerfer" in `NOTIZEN_dxr_1211.md`). Unter einem Gitter, dessen Lichtlinien schmaler als
  ein Luxel sind, faellt er nur besonders gross aus.

## Offen (nicht Auftrag, nur notiert)

- **Hart gegen stufenlos:** ids q3map sperrt hart ab Alpha 128, q3map2 rechnet stufenlos mit 255 - Alpha. Was Ravens q3map
  hier tat, ist ohne seinen Quellcode offen. `grate02_broke` hat fast nur 0 und 255 (0,8 % Zwischenwerte) und entscheidet es
  nicht. Unterscheiden wuerde es z. B. `textures/imperial/floorgrate` (56 % Zwischenwerte, in `textures/h_evil/roundhole_grate`,
  t1_fatal, t3_hevil, taspir1), aber dort fehlen feine Luxel unter dem Gitter. Erst wenn sich das belegen laesst, kaeme ein
  Schalter in Frage.
- **DXR-Selbsttest:** Die Pruefung "|Strahlen - gebacken| <= 35 % von gebacken" unter dem Gitter ist nach Abschnitt 4 ein
  Zufallsvergleich (verschiedene Bilder, gleiches Mittel). Die Kommentare in `gui/selbsttest_dxr.inc` (bei "Massstab ist das
  gebaute Licht" sowie bei B3 "Doppelt definiert ist der Shader auch" und "q3map2 kennt nur surfaceparm alphashadow") sind
  ueberholt. Vorschlag fuer die naechste Aenderung dort: gegen ein Backen mit `-samples` pruefen oder die Topologie des
  Musters zaehlen wie in `vjun2_topologie.py`. In diesem Auftrag nicht geaendert, weil kein Code geaendert werden sollte.

## Gelaufen

- bhed_q3map2 aus `Y:\p_gd\build` (Stand LCFIX; der Compiler ist seither fuer -light unveraendert), `-threads 8`, Prioritaet
  Leerlauf, nur eigene Laufordner unter `Y:\werkzeuge\gitter`. Spieldaten nur gelesen.
- Kein Bau, kein ctest, kein Selbsttest: Am Code hat sich nichts geaendert, nur diese Notizen und zwei Berichtigungen in
  `NOTIZEN_dxr_1211.md` und `NOTIZEN_lcfix_1211.md`.

## Changelog

## English

- Map compiler: checked against the original maps - alphashadow grates with `alphaFunc LT128` let the light through where the texture has little alpha, like Raven's compiler did for Jedi Academy (`vjun2`), even though the game draws the bars there. Nothing changed; the real-time light does the same.

## Deutsch

- Karten-Compiler: gegen die Originalkarten geprüft - alphashadow-Gitter mit `alphaFunc LT128` lassen das Licht dort durch, wo das Bild wenig Alpha hat, so wie Ravens Compiler bei Jedi Academy (`vjun2`), obwohl das Spiel dort die Stäbe zeichnet. Nichts geändert; das Echtzeit-Licht macht es genauso.

## 中文

- 地图编译器:已与原版地图对照检查——带 `alphaFunc LT128` 的 alphashadow 格栅在贴图 alpha 较低处透光,与 Raven 编译器在 Jedi Academy 中的做法(`vjun2`)相同,尽管游戏在这些位置绘制的是栅条。没有改动;实时光照的行为相同。

## 日本語

- マップコンパイラー:オリジナルマップと照合しました。`alphaFunc LT128` の alphashadow 格子は、テクスチャのアルファが低い所で光を通します。ゲームはそこに格子の棒を描きますが、Raven のコンパイラーも Jedi Academy(`vjun2`)で同じでした。変更はありません。リアルタイムライトも同じ動作です。

## Español

- Compilador de mapas: comprobado con los mapas originales: las rejillas alphashadow con `alphaFunc LT128` dejan pasar la luz donde la textura tiene poco alfa, igual que el compilador de Raven en Jedi Academy (`vjun2`), aunque el juego dibuja allí los barrotes. Sin cambios; la luz en tiempo real hace lo mismo.

## Čeština

- Kompilátor map: ověřeno proti původním mapám - mřížky alphashadow s `alphaFunc LT128` propouštějí světlo tam, kde má textura málo alfy, stejně jako Ravenův kompilátor v Jedi Academy (`vjun2`), přestože hra tam kreslí mříže. Beze změny; světlo v reálném čase se chová stejně.
