# Notizen REST (08.11.2026): offene Punkte vor rc611

Ein Agent (Auftrag `auftraege/auftrag_rest_0811.md`), Laeufe `rs_*`, Skripte und Protokolle unter `Y:\werkzeuge\rs\`.
Basis 57733c73d (karte1). kFassung bleibt rc610, nichts gepusht.

## Punkt 1: Painting schnell

- Ursache gemessen (Zeitstempel, Lauf `rs_ptzeit_mess1`): mit offenem 2D-Bild kosteten 130 Bilder 4029 ms, davon 3946 ms in
  `bildStufen` - die Bildliste eines Shaders las jede Kandidatendatei ganz aus den Archiven (readFromArchives entpackt), und
  der Bereich, das 2D-Bild und jeder Tupfer fragten je Bild danach (drei Fragen, je 10 ms). Jetzt gemerkt je Name und Shader
  (`dateiVon`, `bildStufen`), geleert mit `texCacheStand` (wie die Bildtabelle), beim Einlesen der Shadertabelle und bei einem
  anderen Modell.
- Ein Tupfer kopiert nur das geaenderte Rechteck in die Bildpunkte der Vorschau und laedt nur dieses Rechteck hoch
  (`gpu::figurBildBereich`: Stufe 0 und die Pyramide darueber mit derselben Rechnung wie `Tex::buildMips`, UpdateSubresource
  mit Box) in eine eigene, beschreibbare Textur (nicht mehr nach Inhalt geteilt). Bis hier: ganzes Bild kopiert, Pruefsumme,
  ganze Pyramide auf der CPU, neue Textur.
- Gefunden beim Messen: Die 3D-Ansicht zeigte das Malen nie. (a) Die Ansichten fragten `modelDirty`, bevor gemalt wurde, und am
  Ende des Bilds ging es aus - `ptzeit` "neu gezeichnet 0 von 120"; (b) Flaechen mit Shader-Stufen (der Sturmtruppler-Rumpf)
  zeichnen die Bilder der Stufen (`ModelTextures::stufen`, geteilt zwischen den Figuren), nicht `bySurface`. Jetzt: der Stand
  der bemalten Textur gehoert zum Zeichenstand jeder Ansicht (`ModellMalen::ansichtStand`), und die Stufen, deren Bild die
  bemalte Datei ist, bekommen fuer die Vorschau eine eigene Kopie (`StufenBindung`); neu geladene Texturen bekommen das
  Bemalte wieder.
- Rot `rs_pt_rot` 36 OK / 2 FEHLER ("PT 3D Malen: die Ansicht zeigt den Strich" 0 Bildpunkte anders, "PT 2D Malen: die
  3D-Ansicht zeigt den Strich" 0), gruen `rs_pt_gruen4` 41/0 (3017 bzw. 673 Bildpunkte anders; drei neue Pruefungen
  "PT Grafikkarte ...": jede hochgeladene Textur der Flaeche torso, auch die zwei Shader-Stufen, Stufe fuer Stufe
  zurueckgelesen gleich `Tex::buildMips` ihrer Bildpunkte, 4 von 4).
- Bildgleich und dateigleich: `ptspiel` mit der alten exe (57733c73d) und der neuen: die gebaute trooper-armor.jpg ist
  bytegleich (SHA-256 3afe1403...), das Vorschau-Foto vorher bytegleich; nachher zeigt die neue den Streifen
  (`Y:\werkzeuge\rs\pt_vorschau_alt_neu.png`).

| ptzeit (allein, BHED_D3D_DEBUG=0, CPU Mittel / p99 ms) | vorher `rs_ptzeit_alt` | nachher `rs_ptzeit_neu` |
|---|---|---|
| Ansehen | 0.45 / 0.91 | 0.48 / 1.60 |
| Painting, 2D-Bild offen | 30.45 / 44.20 | 0.84 / 2.91 |
| Painting, je Bild ein Tupfer | 41.25 / 49.55 | 2.04 / 3.59 |

  Der Tupfer-Wert nachher enthaelt das Neuzeichnen der Ansicht in jedem Bild (vorher "neu gezeichnet 0 von 120", nachher 120).

## Punkt 2: Codepruefung Rest (plaene/codepruefung_modellblock.md, _2.md)

Neue Pruefgruppe `BHED_MODELL_NUR=pfix2` (mit `BHED_POSEN=<Ordner>`), ctest `modell_pfix` mit der Probe `data/fixtures/model.glm`.

- N1 (GLM-Versaetze): ein Versatz vor den Anfang der Datei lief ueber size_t um; `glmRohfelder` nahm die Flaeche als "aus der
  Datei", `readGlm` las 3 Ecken vor dem Puffer. Jetzt vorzeichenbehaftet und nur innerhalb der Datei (negativ erlaubt wie
  R_LoadMDXM `(byte *)surf + ofs`). ctest rot 2 FEHL -> gruen (b3297302f).
- N7 (Posen-Datei): `sicherSchreiben` statt ofstream. Rot `rs_pfix2_rot` (Fehler mitten im Schreiben eingebaut:
  alt 8653 Bytes -> 17272, Ergebnis "gespeichert"), gruen `rs_g_pfix2` (8653 Bytes bleiben, Meldung, keine .tmp).
- N10 (Belegung der Karten-GLA): `Inhalt::kinoBelegung` im Dokument, Aenderungen ueber `belegen()` als Schritte; beim Laden
  eines anderen Modells uebernommen. Rot `rs_pfix2_rot` (BOTH_CIN_1 nach Strg+Z belegt), gruen (frei, Strg+Y wieder belegt).
- N6 (Fehlertexte): `bhed/modell/fehler.h` (FehlerArt + Werte, englisch fuer das Protokoll), die Berichte von md3datei,
  glmdatei und animcfg tragen `fehlerArt`, die Text-Rueckgaben (figuranim, xsiaufnahme, physik, retarget, rig) kommen ueber
  `fehlerAusText` zurueck zur Art; die GUI zeigt `modellFehlerText` (33 Texte, `tools/i18n_rest.py`). Rot `rs_pfix2_c`
  ("Name oder Shader von ... laenger als 63 Zeichen" in der englischen Oberflaeche), gruen `rs_g_pfix2`.
- M9 (vorn): Lauf am Sturmtruppler: die Gesichtsknochen liegen bei (0, -3,23, 2,98) vom cranium, die Figur schaut nach -Y.
  Die UV-Naehte rechneten mit +Y (die "verdeckten" Naehte lagen vorn), und die Ansicht "Vorne" blickte entlang -Y und zeigte
  den Ruecken (Foto `Y:\werkzeuge\rs\pt_kopf.png`: Helm-Hinterkopf, Rueckenkasten). Jetzt: Vorne/Hinten blicken entlang
  +Y/-Y wie 3ds Max und Blender (Ziffernblock 1), UV nimmt vorn aus `modell::figurVorn` (aus `rigAnlegen` herausgezogen,
  dieselbe Rechnung fuer Rig, IK, Retarget und UV). Rot `rs_pfix2_b`, gruen `rs_g_pfix2`. Handbuchbilder der Model-Tab-Gruppen
  neu (6 Sprachen).
- N5 (doppelte Logik): `bhed/modell/glavorrat.h` (GlaVorrat) fuer die sechs Vorrat-Schleifen; `basisGla` hat jetzt die
  24-Bit-Grenze. Nicht rot nachstellbar (16,7 Mio. verschiedene 14-Byte-Eintraege); bytegleich belegt durch mb2 37/0, mb5
  159/0, mb7 24/0, rig 26/0, rt 19/0, ctest modell_* 20/20. `klein()` je Datei bleibt (verschiedene Regeln: mit/ohne
  Schraegstrich-Tausch).
- N14 (Rig-Mathematik): `nurDrehung` hiess in ik.cpp "Ort genullt" und in retarget.cpp "starr" - jetzt `ohneOrt` und
  `nurDrehungStarr`; `ausSchluessel` (wortgleich in rig.cpp und ik.cpp) einmal als `schluesselMatrix` (rig.h). Die vier
  Slerp-Fassungen bleiben: sie unterscheiden sich gewollt (Normieren, Blender-Interpolation); `umPunkt`/`verschiebung`
  haben verschiedene Typen (float[3] / V3). Ohne Rotlauf (Aufraeumen), gleiche Ergebnisse in rig, mb6, rt, ph.
- Abweichung: Nach Aenderungen an Kopfdateien waehrend eines laufenden Baus (selbst verursacht) stuerzten sechs Gruppen in
  `spuren.cpp` ab (veraltete Objektdateien mit altem `App`-Aufbau). Nach dem sauberen Neubau alle gruen (rs_h_*).

## Punkt 5: uvrundlauf zeigte 0 Pruefungen

- Der Lauf verglich jede .glm, meldete aber nur Abweichungen (meldeFehler) - die Zusammenfassung zaehlte "0 OK, 0 FEHLER".
  Jetzt eine Pruefung je Spiel. Rot `rs_uvrl_alt_md` 0 OK, gruen `rs_uvrl_md` 924 .glm, `rs_uvrl_jka` 127, `rs_uvrl_jk2` 71
  (je 1 OK, Byte fuer Byte gleich) (e3ee8fb3c).

## Punkt 3: Paarbild Painting Spiel gegen Vorschau

- Nach M9 malt `ptspiel` den Streifen wirklich ueber die Brust (vorher lag er auf dem Ruecken - die Ansicht "Vorne" zeigte den
  Ruecken, daher sah man im Spiel von vorn nur Kanten am Hals). Das Foto der Vorschau ohne Auswahlrahmen.
- Movie Duels per Popen (eigener fs_homepath `Y:\werkzeuge\rs\ptspiel_lauf\home`, `sp_654neu\pt\ptspiel.py`, duel_deathstar,
  dritte Person von vorn, 1920x1440), Mod aus `ptspiel` (`Y:\werkzeuge\rs\ptmod`, zweimal gebaut bytegleich). Spielordner
  vorher/nachher gleich (3772 Dateien, Groesse und Zeit).
- Paarbild `Y:\werkzeuge\rs\pt_paar_spiel_vorschau.png` (1920x2920, oben Spiel, unten Vorschau ungeskaliert): beide zeigen
  den roten Streifen quer ueber die Brust und die roten Stellen an der Huefte. Abweichung: Haltung (Spiel: Schwert-Stand,
  Oberkoerper vorgebeugt; Vorschau: Grundstellung) und Kamera (Spiel: dritte Person, Abstand 90; Vorschau: Ansicht Vorne,
  eingepasst) - gleiche Seite und Stelle, nicht derselbe Bildausschnitt.

## Punkt 4: karteneditor-Volllauf haengt bei "Schnell-Backen fertig"

- Verdacht Port 39000 widerlegt: zwei kr-Laeufe gleichzeitig (`rs_ke_kr1`, `rs_ke_kr2`, je 39/0) - der zweite Bau lauscht auf
  39001 ("Port 39000 is in use (another build), monitoring on 127.0.0.1:39001", reste1 Punkt 3); das Schnell-Backen selbst
  startet q3map2 ohne -connect.
- Nachgestellt im vollen Lauf allein (`rs_ke_voll`, 3275 OK bis dahin): nach dem Ende von aseb (leftMode 0, Events) liefen
  kreste, r2a, r3, rd und reste1 in Events - der Karten-Editor zeichnete nicht. Darum rechnete das Teile-Fenster seine
  Vorschau nicht (KR truhe/punkt "Art/Brushes 1/1 2/1", jetzt "Vorschau mit der neuen Wahl: nicht nach 600 Bildern"), und
  `echtLichtSchritt` lief nicht: der Bau war fertig, der neue begann nie (Stapel des Prozesses: kein Bau-Faden, kein
  q3map2; Zustand "bauStand 3, Faden 0, gebacken 1, gescheitert 0, abgebrochen 1"). Danach scheiterten auch die vier R2a-Gras-
  Pruefungen (so in der Basis e6daa5ca8). Einzeln liefen alle Gruppen gruen, weil ihr Einzellauf mit
  `stufe9c::einrichten()` (Klick auf "Map Editor") beginnt.
- Behoben: im vollen Lauf beginnt jede dieser Gruppen ebenso mit `stufe9c::einrichten()`.
- Dazu, damit ein Stillstand nie wieder stundenlang stumm bleibt: `langeWarten` zaehlte Bilder (400000 = Stunden unter Last;
  die laengste gemessene Wartezeit waren 974 Bilder in 62 Wartezeiten). Jetzt zusaetzlich 600 s Uhrzeit, danach FEHLER mit dem
  Zustand des Baus (`echtLichtBauZustand`: Stand, laufender Schritt -meta/-vis/-light und seit wann). Die Teile-Vorschau wird
  abgewartet statt fester 0,4 s.
- Gruen: `rs_ke_voll2` allein 3373 OK, 0 FEHLER (ZUS-Runde: 3275 OK, 2 FEHLER, dann Stillstand).

## Schluss (Laeufe `rs_s_*`, Summe `Y:\werkzeuge\rs\schluss_summe.txt`)

- Sauberer Neubau `build-test` 0 Warnungen (zweimal: nach Punkt 2 und am Ende); ctest 235/235.
- Modus 1 2635/0; karteneditor voll allein 3373/0; Modell-Gruppen: pt 41/0 (`rs_pt_end`; im Schluss zuerst 39/3 - der Strich in
  der Animation lief noch mit dem Radierer, Testfehler, behoben), pfix 17/0, pfix2 5/0, pfixkarte 5/0, md 84/0, mdkarte 7/0,
  funktionen 60/0, tasten 43/0, blender 44/0, uvmd3 3/0, ai 29/0, pr 15/0; nach dem Neubau von Punkt 2 auch mb2 37/0, mb3 21/0,
  mb5 159/0, mb6 37/0, mb7 24/0, ph 29/0, rig 26/0, rt 19/0, uv 50/0; uvrundlauf md/jka/jk2 je 1/0.
- Lints wie die Basis (portability 99, msvclink 110 Dateien ok, fremdprogramm 1 bekannt); Handbuch `--pruefen --pflicht=es
  --pflicht=cs` ok, 36 Model-Tab-Bilder je Sprache neu (13 Gruppen x 6 Sprachen, 0 FEHLER).
- Spielordner Movie Duels vorher/nachher gleich; kein eigener Prozess mehr.

## Offen

- Punkt 3: Paarbild mit gleicher Seite und Stelle, aber anderer Haltung und Kamera (siehe oben).
- N5: `klein()` je Datei nicht zusammengelegt (verschiedene Regeln); N14: Slerp-Fassungen und `umPunkt`/`verschiebung` bleiben
  getrennt (Begruendung oben).
- Painting auf der GPU (Hinweis des Hauptstrangs): nicht gebaut - die Ziele (< 2 ms mit offenem 2D-Bild, Tupfer nur das
  Rechteck) sind auf der CPU erreicht, bild- und dateigleich.

## Changelog

## English

- Painting: viewing the 2D image or painting no longer slows the Model tab down (30 ms -> 0.8 ms per frame with the 2D image open, 41 ms -> 2 ms with a dab every frame); a dab only uploads the changed rectangle, the result is the same pixel for pixel.
- Painting: strokes now show at once in the 3D view, also on surfaces whose shader has several stages (like the stormtrooper armour) and when painting in the 2D image.
- Model tab: the Front view shows the front of the figure (it showed the back) and Back the back, like 3ds Max and Blender; UV seams "on the hidden side" now really go to the back.
- Model tab: saving the pose library can no longer leave a half file behind when writing fails.
- Model tab: assigning a sequence to a slot of the target GLA (map GLA BOTH_CIN_n, _humanoid, own GLA) can be undone and redone with Ctrl+Z / Ctrl+Y.
- Model tab: error messages of the model writers, GLA merging, figure baking, XSI import, physics and retarget appear in the language of the program.
- Model tab: broken .glm files with offsets pointing before the start of the file are no longer read in front of the data.

## Deutsch

- Painting: das 2D-Bild ansehen oder malen bremst den Model-Tab nicht mehr (30 ms -> 0,8 ms je Bild mit offenem 2D-Bild, 41 ms -> 2 ms mit einem Tupfer je Bild); ein Tupfer lädt nur das geänderte Rechteck hoch, das Ergebnis ist Bildpunkt für Bildpunkt gleich.
- Painting: Striche erscheinen sofort in der 3D-Ansicht, auch auf Flächen, deren Shader mehrere Stufen hat (wie die Sturmtruppler-Rüstung), und beim Malen im 2D-Bild.
- Model-Tab: die Ansicht „Vorne“ zeigt die Vorderseite der Figur (sie zeigte den Rücken) und „Hinten“ den Rücken, wie 3ds Max und Blender; UV-Nähte „auf der verdeckten Seite“ kommen jetzt wirklich nach hinten.
- Model-Tab: das Speichern der Posen-Bibliothek hinterlässt keine halbe Datei mehr, wenn das Schreiben scheitert.
- Model-Tab: eine Sequenz auf einen Platz der Ziel-GLA legen (Karten-GLA BOTH_CIN_n, _humanoid, eigene GLA) lässt sich mit Strg+Z / Strg+Y zurücknehmen und wiederholen.
- Model-Tab: Fehlermeldungen der Modell-Schreiber, des GLA-Zusammenführens, des Figurenbackens, des XSI-Imports, der Physik und des Retargetings erscheinen in der Sprache des Programms.
- Model-Tab: kaputte .glm-Dateien mit Versätzen vor den Anfang der Datei werden nicht mehr vor den Daten gelesen.

## 中文

- 绘制:查看 2D 图像或绘制时 Model 标签不再变慢(打开 2D 图像时每帧 30 毫秒 -> 0.8 毫秒,每帧一笔时 41 毫秒 -> 2 毫秒);每一笔只上传改动的矩形,结果逐像素相同。
- 绘制:笔画立即显示在 3D 视图中,包括着色器有多个阶段的表面(如冲锋队盔甲)以及在 2D 图像中绘制时。
- Model 标签:"前"视图显示角色的正面(之前显示背面),"后"显示背面,与 3ds Max 和 Blender 一致;"在隐藏面"的 UV 接缝现在真正位于背面。
- Model 标签:写入失败时,保存姿势库不再留下半个文件。
- Model 标签:将序列放到目标 GLA 的槽位(地图 GLA BOTH_CIN_n、_humanoid、自定义 GLA)可以用 Ctrl+Z / Ctrl+Y 撤销和重做。
- Model 标签:模型写入、GLA 合并、角色烘焙、XSI 导入、物理和重定向的错误消息以程序的语言显示。
- Model 标签:偏移指向文件开头之前的损坏 .glm 文件不再在数据之前读取。

## 日本語

- ペイント: 2D 画像の表示やペイントで Model タブが遅くならなくなりました(2D 画像を開いた状態で 1 フレーム 30 ms -> 0.8 ms、毎フレーム 1 回のストロークで 41 ms -> 2 ms)。ストロークは変更された矩形だけをアップロードし、結果はピクセル単位で同じです。
- ペイント: ストロークがすぐに 3D ビューに表示されます。シェーダーに複数のステージがあるサーフェス(ストームトルーパーの鎧など)や 2D 画像でのペイントでも同様です。
- Model タブ: 「前」ビューはフィギュアの正面を表示し(以前は背中を表示)、「後」は背中を表示します。3ds Max や Blender と同じです。「見えない側」の UV シームは本当に背面に置かれます。
- Model タブ: 書き込みに失敗しても、ポーズライブラリの保存で半端なファイルが残らなくなりました。
- Model タブ: ターゲット GLA のスロット(マップ GLA BOTH_CIN_n、_humanoid、独自 GLA)へのシーケンス割り当てを Ctrl+Z / Ctrl+Y で取り消し・やり直しできます。
- Model タブ: モデルの書き出し、GLA の統合、フィギュアのベイク、XSI インポート、物理、リターゲットのエラーメッセージがプログラムの言語で表示されます。
- Model タブ: ファイルの先頭より前を指すオフセットを持つ壊れた .glm ファイルを、データの前から読まなくなりました。

## Español

- Pintura: ver la imagen 2D o pintar ya no ralentiza la pestaña Model (30 ms -> 0,8 ms por fotograma con la imagen 2D abierta, 41 ms -> 2 ms con un toque por fotograma); un toque solo sube el rectángulo cambiado, el resultado es igual píxel a píxel.
- Pintura: los trazos aparecen al instante en la vista 3D, también en superficies cuyo shader tiene varias etapas (como la armadura del soldado de asalto) y al pintar en la imagen 2D.
- Pestaña Model: la vista «Frente» muestra el frente de la figura (mostraba la espalda) y «Atrás» la espalda, como 3ds Max y Blender; las costuras UV «en el lado oculto» van ahora de verdad atrás.
- Pestaña Model: guardar la biblioteca de poses ya no deja un archivo a medias si falla la escritura.
- Pestaña Model: asignar una secuencia a una ranura del GLA de destino (GLA de mapa BOTH_CIN_n, _humanoid, GLA propio) se puede deshacer y rehacer con Ctrl+Z / Ctrl+Y.
- Pestaña Model: los mensajes de error de los escritores de modelos, la fusión de GLA, el horneado de figuras, la importación XSI, la física y el retargeting aparecen en el idioma del programa.
- Pestaña Model: los archivos .glm dañados con desplazamientos antes del inicio del archivo ya no se leen por delante de los datos.

## Čeština

- Malování: prohlížení 2D obrázku ani malování už nezpomaluje kartu Model (30 ms -> 0,8 ms na snímek s otevřeným 2D obrázkem, 41 ms -> 2 ms s jedním tahem na snímek); tah nahraje jen změněný obdélník, výsledek je stejný pixel po pixelu.
- Malování: tahy se hned zobrazí ve 3D pohledu, i na plochách, jejichž shader má více fází (jako zbroj stormtroopera), a při malování ve 2D obrázku.
- Karta Model: pohled „Zepředu“ ukazuje přední stranu postavy (ukazoval záda) a „Zezadu“ záda, jako 3ds Max a Blender; UV švy „na skryté straně“ jsou teď opravdu vzadu.
- Karta Model: uložení knihovny póz už nenechá poloviční soubor, když zápis selže.
- Karta Model: přiřazení sekvence ke slotu cílového GLA (mapové GLA BOTH_CIN_n, _humanoid, vlastní GLA) lze vrátit a zopakovat pomocí Ctrl+Z / Ctrl+Y.
- Karta Model: chybové zprávy zapisovačů modelů, slučování GLA, pečení postav, importu XSI, fyziky a retargetingu se zobrazují v jazyce programu.
- Karta Model: poškozené soubory .glm s posuny před začátek souboru se už nečtou před daty.
