# Notizen MBMAL: Malen auf dem Modell wird langsam (rc612)

Auftrag: `JKStudio_Uebergabe/auftraege/auftrag_mbmal_1011.md` (shank 10.11.: "wenn ich lange painte, dann wird es laggy und die
fps geht runter" - "das laggen beim painten kommt davon, dass ich am 3d model selbst zeichne"), dazu Punkt 5 (shank 10.11.
~14:20: "okay ja mach das" - Tupfer im Bildschirm verteilen wie Substance Painter und Blender). Begonnen von Fable (WIP 1-7),
fertig gestellt von Opus. Zweig `p_mbmal` auf a00f6cf90.

## Nachstellen

Selbsttest `BHED_EDITORTEST=modellblock BHED_MODELL_NUR=ptlang` (allein, `BHED_D3D_DEBUG=0`): wie shanks Bild - kyle, body.jpg
2048 x 2048, Painting, 2D-Bild mit "Show UVs", eine 3D-Ansicht, Animation BOTH_A2_TR_BL laeuft, Pinsel Radius 24, Strength 1,
Spacing 10 %, Pressure: strength. Viele Striche ueber die echte Maus nach der Uhr (ein Strich 0,6 s), reihum ueber alle Flaechen
mit dem Bild body. Schalter `BHED_PTLANG_STRICHE` (Vorgabe 2000), `BHED_PTLANG_WO=2d`, `BHED_PTLANG_DRUCK=1`,
`BHED_PTLANG_ANIM=0`, `BHED_PTLANG_FEST=1` (feste Tupfer, Pruefsummen von Bild und gebauter Datei, Rueckgaengig/Wiederholen,
Grafikkarte gegen Bild). Je 100 Striche eine Zeile und `ptlang_<wo>.csv`; Zaehler und Stoppuhren in
`include/bhed/modell/malen_mess.h` (nur waehrend der Messung).

Selbsttest `BHED_MODELL_NUR=ptsprung` (Punkt 5): kyle in der Pose BOTH_A2_TR_BL (Mitte, steht), Strich r_hand -> l_hand in einem
Mausereignis; je geaendertem Texel die Stelle in der Pose am Schirm (`modellMalenPfadPruefen`) gegen den gezogenen Weg. Dazu ein
Strich ueber den torso ohne Sprung, seine Texel in `ptsprung_flaeche.bin` (Vergleich vorher/nachher mit
`Y:\werkzeuge\mbm\flaeche_vergleich.py`).

Testfehler, unterwegs gefunden: ptlang passte mit der Auswahl nur eine Flaeche (hips) ein - die Mitten der anderen Flaechen lagen
ausserhalb der Ansicht, ihre Klicks trafen das 2D-Bild oder Knoepfe des Befehlsfelds (1330 Fehlversuche in 2000 Strichen, darunter
"Bake to new UVs": daher die 42 Maskenlesungen im ersten langen Lauf). Seit WIP 9 passt der Test die ganze Figur ein und klickt nur
in der Ansicht.

## Ursachen (belegt, Zaehler gelten auch unter Last)

1. **Je Tupfer in 3D zu viel Arbeit** (`src/modell/malen.cpp`): jeder Tupfer pruefte alle Dreiecke (41 % der Leinwand), lief alle
   Texel jedes Dreiecks in Reichweite durch und sicherte, blutete, setzte zusammen und lud hoch im Rechteck um ALLE Treffer (ueber
   eine UV-Naht bis zum ganzen Bild, je Textur der Vorschau samt Pyramide). Behoben: Suchgitter ueber die Kugeln der Dreiecke,
   Texelbereich je Dreieck (Ellipse um den Tupfer), Tupfer je Kachel, Hochladen nur der geaenderten Flaechen
   (`schmutzRechtecke`). Bild und gebaute Datei bitgleich.
2. **Maske je Malbild neu gelesen** (`gui/app_modelltab_malen.cpp`): die 2D-Ansicht bestimmte die Maskenflaechen je Stand des
   Bilds neu, also nach jedem Tupfer (4 Mio. Texel). Behoben: eigener Stand der Maske (`Leinwand::maskeStand`).
3. **Ein Strich aus dem 2D-Bild malte in der 3D-Ansicht weiter**: die Ansicht rief bei jeder Mausbewegung mit Taste den 3D-Tupfer,
   auch wenn der Strich im Bild begann; `Strich::weiter` verband die Stelle im Bild (Texel) mit dem Treffpunkt (Ruhelage):
   zehntausende Tupfer in einem Bild, Streifen quer ueber das Bild. Behoben: der Strich gehoert der Ansicht, in der er beginnt.
4. **Punkt 5: Spruenge zwischen Koerperteilen** (r_hand -> l_hand: in der Pose 53 Bildpunkte, in der Ruhelage 40 Einheiten) legten
   eine Tupferlinie durch die Ruhelage (bei kyle durch die Huefte): Streifen, wo der Pinsel nie war, und die restlichen Ruckler.
   Behoben wie Substance Painter und Blender: die Tupfer liegen auf der Linie zwischen den Mauspositionen am Schirm (Spacing in
   Bildpunkten des Pinselradius), jeder mit einem eigenen Strahl gegen das gezeigte, posierte Netz (vorderste Flaeche, wie die
   Ansicht sie zeigt); ueber Hintergrund, einem anderen Bild oder ausserhalb der Ansicht malt ein Tupfer nichts. Radius,
   zugewandte Dreiecke und Spiegeln je Tupfer; Verwischen nimmt den Weg in UV seit dem vorigen Tupfer (wie im 2D-Bild). Die Ecken
   der Pose am Schirm einmal je Mausereignis (`SchirmNetz`), ein Strahl prueft nur Dreiecke, deren Rechteck die Stelle enthaelt.
   Quelle: Blender-Handbuch "Stroke" (https://docs.blender.org/manual/en/latest/sculpt_paint/brush/stroke.html): Space "creates brush
   stroke as a series of dots, whose distance (spacing) is determined by the Spacing setting", Spacing "percentage of the brush radius",
   Spacing Distance View "relative to the view" (Scene nur im Sculpt-Modus). Substance Painter: Antwort des Hauptchats an shank (die
   Adobe-Hilfeseite lieferte beim Nachlesen HTTP 403).
- Widerlegt: Verlauf als Bremse (Grenze 512 MB greift, schrittEnde 0,3-2 ms je Strich), Vergleich bemalt gegen Original.

## Messung

Ruhefenster 10.11. 21:09-21:42 (kein anderer Agent, Fremdlast laut tasklist "nichts"), `Y:\werkzeuge\mbm\messung_final.sh`,
vorher = `vorher_bin` (Malen vor allen Behebungen, Test wie nachher), nachher = `nachher_bin` (WIP 11), abwechselnd, normale
Prioritaet, `BHED_D3D_DEBUG=0`. Volle Tabellen: `Y:\werkzeuge\mbm\messung_final.txt`.

| Lauf | Bildzeit beim Malen vorher / nachher (ms) | p99 | Tupfer je Strich (in der Luft) | hochgeladen je Ereignis (Texel) | Verlauf-Kacheln je Strich |
|---|---|---|---|---|---|
| 3D, Animation laeuft, 600 Striche | 4,53 / 1,36 | 76,7 / 8,1 | 179 (27 %) / 27 (0 %) | 90978 / 15341 | 145,5 / 53,4 |
| 3D, Animation steht, 200 | 3,45 / 1,68 | 30,5 / 7,5 | 52 (3 %) / 40 (0 %) | 26529 / 18388 | 69,1 / 33,3 |
| 2D-Bild, 200 | 9,54 / 4,83 | 17,1 / 7,7 | 204 / 208 | - | 182,9 / 103,2 |
| 3D mit Stiftdruck, 200 | 4,14 / 1,19 | 63,1 / 6,8 | 190 (28 %) / 28 (0 %) | 88752 / 15196 | 156,0 / 55,9 |

Je Tupfer in 3D: gepruefte Dreiecke 1354 -> 57 (von 3278), bearbeitete Flaeche 53785 -> 15416 Texel; Maske je 600 Striche
50,8 ms -> 0. Bildzeit in Ruhe gleich (0,68 ms).

Langer Lauf (2000 Striche, Animation laeuft, nachher_bin, allein, 21:09 fertig): Bildzeit beim Malen erste 100 Striche 1,40 ms,
letzte 1,38 ms (Bloecke 1,09-1,42), p99 5,8-7,7 ms, Tupfer in der Luft 0 %, Verlauf waechst bis zur Grenze (512 MB je Leinwand;
537 MB mit head und belt), privat 1877 -> 2292 MB (= Verlauf), Pruefungen "Bildzeit wie am Anfang" und "Speicher begrenzt" gruen.
Vor der Behebung (erster langer Lauf mit WIP 7, noch mit dem Testfehler "Auswahl eingepasst"): 1,93 / 1,95 ms - schon ohne Anstieg
ueber die Striche; das Ruckeln kam aus den teuren Tupfern und Spruengen, nicht aus einem Anstieg ueber die Zeit.

## Punkt 5: Nachweis

Selbsttest ptsprung (kyle, BOTH_A2_TR_BL Mitte, r_hand 1343/570 -> l_hand 1396/566 am Schirm, 53 Bildpunkte; Grenze 1,5 x
Pinselradius = 36 Bildpunkte):

| | vorher (vorher_bin) | Stand nach Ursache 1-3 (p5v_bin) | nachher (nachher_bin) |
|---|---|---|---|
| Texel ausserhalb des Wegs (weitester) | 10688 (283 px), alle auf hips | 10688 (283 px) | 0 (27 px) |
| Sprung-Ereignis: Zeit | 416,6 ms (Hochladen 351,6) | 39,8 ms | 48,2 ms (Hochladen 27,7) |
| Tupfer im Ereignis (ohne Texel) | 216 (164) | 216 (164) | 20 (0), 22 Strahlen, 2 Stellen ohne Flaeche |
| bearbeitete Texel im Ereignis | 5656690 | - | 808038 |

(a) rot -> gruen: der Streifen durch die Huefte (Ruhelage) ist weg. (c) Ruckler beim Sprung 416,6 -> 48,2 ms (gegen den Stand nach
Ursache 1-3 etwas mehr, weil jetzt die Flaeche unter dem gezogenen Weg wirklich bemalt wird: 81226 statt 42792 Texel).
(b) Strich ueber den torso ohne Sprung (50 Bildpunkte, 19-23 Tupfer) gegen den Stand nach Ursache 1-3: 26443 / 26234 Texel
geaendert, 26219 in beiden, davon 16320 gleich (61,7 % der beruehrten Texel bitgleich); Abweichung in 9899 Texeln hoechstens 16,
im Mittel 4,1 (je Kanal 0..255), 224 Texel nur vorher, 15 nur nachher - nur an den Raendern der Tupfer (Abstand und Radius je
Tupfer am Schirm statt auf der Flaeche). Beide Staende malen dort Texel bis 68 Bildpunkte vom Weg (alle torso: Kugel in der
Ruhelage bei verdrehter Pose, siehe Offen).

## Bitgleichheit und geaenderte Testerwartungen

- Ursachen 1-3 bitgleich (ptlang fest, 100 Striche, endgueltiger Test): vorher_bin und p5v_bin Bild b2af3bd83e067fd0, gebaute Datei
  73957e9f9d49c132 (3628633 Bytes), 538874 Texel, 4751 Tupfer; Verlauf 6660 -> 3097 Kacheln (218 -> 101 MB). Rueckgaengig bis zum
  Original und Wiederholen bis zum selben Bild in beiden gruen.
- Punkt 5 aendert das Bild (gewollt): nachher_bin Bild 37957a600de6947d, Datei 8bf9572a425514c9, 575322 Texel, 3923 Tupfer.
- ctest modell_malen (nahtTest): Pruefsumme e952f4b1ee707493 wie im Rotlauf (WIP 6).
- Geaenderte Erwartungen: ptlang "liest die Maske nicht neu" zaehlt jetzt Lesungen einer schon gelesenen Leinwand (vorher hoechstens
  eine Lesung insgesamt - ein Strich auf head/belt zeigt deren Leinwand, die ihre Maske einmal liest); ptlang/ptsprung passen die
  ganze Figur ein und klicken nur in der Ansicht (vorher nur die gewaehlte Flaeche eingepasst); "fest"-Pruefsummen gelten je Teststand
  (keine festen Werte im Test).

## Handbuch

Painting, Absatz "Werkzeuge und Pinsel", 6 Sprachen: ein Satz, dass die Tupfer in der Ansicht der gezogenen Linie am Bildschirm
folgen wie in Substance Painter und Blender (schneller Strich von einer Hand zur anderen malt nur, wo der Pinsel ueber das Modell
ging). Bilder unveraendert (der Handbuchstrich liegt auf einer Flaeche ohne Sprung).

## Offen

- Der Tupfer ist eine Kugel in der Ruhelage (Modell-Block 1c): bei stark verdrehter Pose erreicht er auf derselben Flaeche Texel
  bis 68 Bildpunkte vom Weg (Pinsel 24, torso in BOTH_A2_TR_BL). Blender projiziert den Pinsel am Schirm auf die Pose; ein Umbau
  waere eine eigene Entscheidung (Form des Tupfers aendert sich mit der Animation).
- Hochladen bleibt der groesste Posten je Mausereignis (11 Texturen der Vorschau, je Flaeche eine, je Stufe der Pyramide); eine
  gemeinsame Textur je Leinwand waere ein Umbau in `gpumap_win32.cpp` (D3D12-Agent).
- 2D-Striche unterhalb des Bilds (v > 1) malen gewickelt weiter oben (tupfer2d wickelt, GL_REPEAT) - Bestand.

## Schlussrunde

11.11. vormittags, Stand WIP 12 (a25303d5e), Protokolle in `Y:\werkzeuge\mbm\`:

- Bau nach WIP 12 (nur Test) 0 Warnungen (`bau15.log`); sauberer Neubau aller Ziele (`--clean-first`) 0 Warnungen (`bau16.log`),
  gesichert als `nachher_bin` 08:30.
- ptlang mit der berichtigten Maskenpruefung (300 Striche, 3D, Animation laeuft): 7 OK, 0 FEHLER - Maske 1 mal gelesen
  (head.jpg bei Strich 60, 1 Leinwand, 0 mal wieder); die FEHLER-Zeile des langen Laufs war die Testerwartung (eine Leinwand liest
  ihre Maske beim ersten Zeigen einmal, gemerkt je Leinwand), kein Programmfehler. ptlang fest (100 Striche): Bild weiter
  37957a600de6947d, Datei 8bf9572a425514c9, 6 OK.
- ctest 245/246; rot nur `q3map2_teilbau` (zweiraum -bounce 2: Brush 9/24, 7/24, 7/24 und Shader 18/18 aus dem Lichtcache),
  im Einzellauf wiederholt gleich. Stammt aus der Basis a00f6cf90 (WIP lcb2 1, Lichtcache); MBMAL aendert keine q3map2-Datei.
- modellblock alle Gruppen (ohne NUR 104 OK, funktionen, tasten, mb2, blender, haut, bild, bild2, mb3, rundlauf, rig,
  rigrundlauf, mb5, kurvenrundlauf, mb6, ikrundlauf, mb7, mb7textur, ph, rt, pr, ai, uv, uvmd3, uvrundlauf, md, mdkarte, pt,
  ptfix, ptfix2 (Japanisch), ptfix3, ptstufen, ptsprung, pfix, pfix2, pfixkarte) und Modus 1 (2635 OK): 0 FEHLER.
  Ausnahme `rtjka`: Absturz 0xC0000005 nach "wampa/model.glm nicht in den Archiven" - der Ordner "GameData original" (JKA)
  ist nicht mehr vorhanden (0 Spielordner); mit der exe von p_mbkopf (ohne MBMAL, am 10.11. gruen) derselbe Absturz.
  Umgebung, nicht MBMAL; der Absturz bei fehlendem Modell ist ein eigener Fehler (offen).
- Lints: Ausgaben aller `tools/lint_*.py` gleich wie in der Basis a00f6cf90 (efxflags, fremdprogramm, portability 99,
  pruefcodes 11 dort ebenso) - keine neue Beanstandung.

## Changelog

## English

- Painting on the model in the 3D view stays smooth however long you paint: a dab only works on the triangles and texels within its reach, only the changed areas are uploaded, the mask is no longer re-read after every dab, and a stroke started in the 2D image no longer paints on in the 3D view.
- Painting in the 3D view spaces the dabs along the line you draw on the screen, like Substance Painter and Blender: each dab lands on the surface visible there, nothing is painted over the background, and a fast stroke from one hand to the other no longer leaves a stripe across the hips.

## Deutsch

- Malen auf dem Modell in der 3D-Ansicht bleibt flüssig, egal wie lange man malt: ein Tupfer bearbeitet nur die Dreiecke und Texel in seiner Reichweite, hochgeladen werden nur die geänderten Flächen, die Maske wird nicht mehr nach jedem Tupfer neu gelesen, und ein Strich, der im 2D-Bild beginnt, malt nicht mehr in der 3D-Ansicht weiter.
- Malen in der 3D-Ansicht verteilt die Tupfer entlang der Linie, die man auf dem Bildschirm zieht, wie Substance Painter und Blender: jeder Tupfer landet auf der dort sichtbaren Fläche, über dem Hintergrund wird nichts gemalt, und ein schneller Strich von einer Hand zur anderen hinterlässt keinen Streifen mehr quer über die Hüfte.

## 中文

- 在 3D 视图中直接在模型上绘制, 无论画多久都保持流畅: 每次点按只处理其范围内的三角形和纹素, 只上传更改的区域, 不再在每次点按后重新读取遮罩, 在 2D 图像中开始的笔画也不再在 3D 视图中继续绘制。
- 在 3D 视图中绘制时, 点按沿着你在屏幕上画出的线分布, 与 Substance Painter 和 Blender 相同: 每个点按都落在那里可见的表面上, 背景上不会绘制任何内容, 从一只手快速划到另一只手也不再在臀部留下条纹。

## 日本語

- 3D ビューでモデルに直接塗っても、どれだけ長く塗ってもなめらかなままです: ダブは届く範囲の三角形とテクセルだけを処理し、変更した領域だけをアップロードし、ダブのたびにマスクを読み直すことはなくなり、2D 画像で始めたストロークが 3D ビューで塗り続けることもなくなりました。
- 3D ビューでのペイントは、画面上に描いた線に沿ってダブを並べます (Substance Painter や Blender と同じ): 各ダブはそこに見えるサーフェスに置かれ、背景の上には何も塗らず、片手からもう一方の手への素早いストロークで腰に筋が残ることもなくなりました。

## Español

- Pintar sobre el modelo en la vista 3D sigue fluido por mucho que pintes: cada toque solo trabaja con los triángulos y téxeles a su alcance, solo se suben las zonas cambiadas, la máscara ya no se vuelve a leer tras cada toque y un trazo que empieza en la imagen 2D ya no sigue pintando en la vista 3D.
- Pintar en la vista 3D reparte los toques a lo largo de la línea que trazas en la pantalla, como Substance Painter y Blender: cada toque cae sobre la superficie visible allí, no se pinta nada sobre el fondo y un trazo rápido de una mano a la otra ya no deja una franja sobre la cadera.

## Čeština

- Malování na model ve 3D pohledu zůstává plynulé, ať malujete jakkoli dlouho: dotek zpracuje jen trojúhelníky a texely ve svém dosahu, nahrávají se jen změněné oblasti, maska se už nečte znovu po každém doteku a tah začatý ve 2D obrázku už nepokračuje v malování ve 3D pohledu.
- Malování ve 3D pohledu rozmisťuje doteky podél čáry, kterou táhnete na obrazovce, jako Substance Painter a Blender: každý dotek dopadne na plochu, která je tam vidět, přes pozadí se nemaluje nic a rychlý tah z jedné ruky do druhé už nenechá pruh přes boky.
