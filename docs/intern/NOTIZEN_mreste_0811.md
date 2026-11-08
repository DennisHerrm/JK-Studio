# Notizen MRESTE (08.11.2026): Reste des Modell-Blocks vor rc611

Ein Agent (Auftrag `auftraege/auftrag_mreste_0811.md`), Laeufe `mr_*`, Skripte und Protokolle unter `Y:\werkzeuge\mr\`.
Basis 174d7e5b3 (karte1). kFassung bleibt rc610, nichts gepusht. Neue Texte in `tools/i18n_mreste.py`, Selbsttest `gui/selbsttest_mr.inc`.

## Punkt 1: Blender-Modus in allen Stufen

Abgleich der Aktionstabellen je Stufe (Skript `Y:\werkzeuge\mr\tastenvergleich.py`): Kurven, Rig, Painting hatten fuer jede
Max-Taste eine Blender-Taste; Modellieren hatte 12, UV 14, Figur-Animation 1 Aktion nur mit Max-Taste; Weights hatte im
Blender-Modus keine einzige Taste von Blenders Weight Paint. Quelle der Blender-Tasten: Blender 5.0
`scripts/presets/keyconfig/keymap_data/blender_default.py` (nur gelesen, Blender nicht gestartet).

- Modellieren (km_edit_mesh): Alt+Klick Kantenschleife (mesh.loop_select), Strg+Alt+Klick Kantenring (mesh.edgering_select),
  auf Vielecken Alt+Klick die Flaechenschleife, Strg+L Verbundene waehlen (mesh.select_linked, neue Aktion auch in Menue,
  Rechtsklick, Suche), J Connect (mesh.vert_connect_path), Strg+X Remove (mesh.dissolve_mode). Ohne Blender-Taste bleiben
  (Blender hat dort keine): Rand, Element, Make Planar, Collapse (M > Collapse), QuickSlice (Bisect), Target Weld, Clean Remove,
  Bevel der Vielecke - Menue, Rechtsklick, Suche und Tooltip mit Weg.
- UV (km_uv_editor): Strg+E Naht markieren (uv.mark_seam), P Pin (uv.pin), Umschalt+W Menue Ausrichten mit Begradigen und
  Ausrichten (IMAGE_MT_uvs_align). Packen, Entspannen, Dichte haben in Blender 5.0 nur in der Legacy-Belegung Tasten.
- Weights (km_weight_paint): Strg+Strich zieht ab (INVERT), Umschalt+Strich glaettet (SMOOTH), Umschalt+X Gewicht aufnehmen
  (paint.weight_sample; auch "Gewicht aufnehmen" in Suche/Rechtsklick, dann ein Klick in die Ansicht), Strg+X Gewicht auf die
  Auswahl (paint.weight_set), F / Umschalt+F / Strg+F Radius / Staerke / Gewicht ziehen (wm.radial_control: Klick uebernimmt,
  Esc/Rechtsklick bricht ab). Das Ziehen folgt der Maus in der Ansicht (der bestaetigende Klick gilt sonst nicht den Tasten).
- Zaehlung ab 1 wie die Zeitleiste: Flugphase und Warnung der Auto-Physik (Model-Tab und Mission), Spanne von "Reduzieren".
- Figur-Animation Q (Max: Auswahl-Werkzeug) bleibt ohne Blender-Taste: im Blender-Modus gibt es kein Gizmo, ein Klick waehlt immer.
- Rot `mr_bl_rot` 47 OK / 31 FEHLER (Testcode mit altem Verhalten gebaut), gruen `mr_bl_g3` 67 / 0.
  Dazwischen zwei eigene Fehler gefunden und behoben: das Ziehen F endete nicht beim Klick (der Klick setzt den Fokus erst im
  Bild), Gewicht aufnehmen traf ueber das Dreieck nicht - jetzt die Ecken wie ein Tupfer.

## Punkt 3: Chamfer mit Segmenten

- `chamferKanten(n, kanten, segmente, profil)`: jeder Streifen wird in `segmente` Vierecke geteilt, die Bogenpunkte liegen auf
  Blenders Profil (Superellipse, r = -2 ln 2 / ln profil; 0,5 Kreis, 0,25 gerade), linear im Betrag wie alle Bewegungen des Plans;
  Endflaechen, Flicken und Nachbarstreifen bekommen dieselben Bogenpunkte. Kaestchen: Segmente und Max "Tension" / Blender "Shape".
- ctest `modell_mesh`: rot (Segmentieren abgeschaltet) 4 FEHL, gruen alle ok - eine Kante 3 Segmente 14/21/9 mit 4 Punkten auf dem
  Kreis Radius = Betrag, Profil 0,25 Bogenpunkt auf der Geraden, vier Deckelkanten 12/20/10 bzw. 16/28/14, alle zwoelf Kanten
  48/84/38. Selbsttest md 86/0 (neu: Chamfer 3 Segmente ueber das Kaestchen 14/21/9, Rueckgaengig bytegleich).
- Abweichung: Ecken mit drei und mehr gewaehlten Kanten schliesst ein Vieleck ueber die Bogenpunkte (Blender und Max fuellen dort
  ein Gitter); die Punkte stehen bei gleichen Parameterwinkeln (beim Kreis wie Blender, sonst nicht Blenders gleiche Sehnen).
  Chamfer an Punkten hat keine Segmente (wie Max).

## Punkt 8: teil6 3 FEHLER

- Ursache (nachgestellt `mr_teil6_rot` 13/3, Protokoll `mr_teil6_d2`): die Zielfigur steht am Startpunkt der ersten laufenden
  Figur der Grundmission md_am_sith; seit dem Wechsel der Movie-Duels-Daten ist das ani1, und ihr NPC_spawner steht genau dort.
  Das Wachgeschuetz nimmt den naechsten Feind und traf ani1 (Schmerzen bei ani1), der Knall der Kiste ebenso. Kein Fehler der
  Vorschau, ein Fehler des Tests.
- Behoben: der Test entfernt NPC_* der Grundkarte naeher als 600 Einheiten am Zielort (ani1, R2-D2) und schreibt Ziel und
  Schmerzen ins Protokoll. Gruen `mr_teil6_g3` 16 / 0.

## Schluss fuer die Punkte 1, 3, 8 (Laeufe `mr_s_*`)

- Sauberer Neubau `build-test` (--clean-first) 0 Warnungen, 12 min 24 s; ctest 235/235.
- Modell-Block: blender 67/0, md 86/0, uv 50/0 (die alte Erwartung "Naht markieren ohne Blender-Taste" auf Strg+E nachgezogen),
  mb2 37/0, mb5 159/0, mb7 24/0, ph 29/0, rig 26/0, rt 19/0, tasten 43/0, funktionen 60/0, pfix 17/0; modell 14/0, kuerzel 202/0;
  teil6 16/0. Lints wie die Basis (portability 99, pruefcodes 11, fremdprogramm 1 bekannt), i18n ok (der verwaiste Text
  MbGwTastenBlender entfernt); Handbuch `--pruefen --pflicht=es --pflicht=cs` ok.
- Nicht gelaufen (gehoeren in die Schlussrunde des ganzen Auftrags): Modus 1, karteneditor voll, mission1, Handbuchbilder.

| modellleistung (allein, BHED_D3D_DEBUG=0, Bild-CPU Mittel / p99 ms) | vorher (exe des Rotlaufs) | nachher |
|---|---|---|
| ohne Figur ruhig | 0.13 / 0.32 | 0.11 / 0.27 |
| stormtrooper Animation | 0.42 / 0.82 | 0.40 / 0.82 |
| stormtrooper Knochen und Animation | 0.48 / 0.91 | 0.43 / 0.73 |
| jedi_hf Animation | 1.00 / 2.31 | 0.96 / 1.30 |

## Offen (nicht bearbeitet, Kontext)

- Punkt 2 Painting: 3D-Vorschau weiterer Shader-Stufen, Paarbild mit gleicher Haltung und Kamera.
- Punkt 4 model2 als .glm: Quellen gelesen (OpenJK `g_mover.cpp:1066` InitMover: .glm -> G2API_InitGhoul2Model, `cg_ents.cpp:1341`
  CG_Mover zeichnet modelindex2 mit der Ghoul2-Instanz, `tr_ghoul2.cpp:1394` ohne Animation Bild 0 der GLA, ohne Haut die Shader
  der .glm); Vorschau `app_view3d_spiel.inc` (nur .md3) und Kamera `kameraszene.cpp:652` (.glm ausgelassen) noch nicht geaendert.
- Punkt 5 CJK-Umbruch ausserhalb des Model-Tabs, Punkt 6 offene Spielnachweise, Punkt 7 Helligkeit Original-JKA.

## Changelog

## English

- Model tab, Blender mode: modelling takes Blender's keys Alt+click (edge loop), Ctrl+Alt+click (edge ring), Ctrl+L (select linked), J (connect) and Ctrl+X (dissolve).
- Model tab, Blender mode: the UV editor takes Ctrl+E (mark seam), P (pin) and Shift+W (align menu).
- Model tab, Blender mode: weight painting takes Blender's keys - Ctrl and Shift strokes subtract and smooth, Shift+X samples the weight, Ctrl+X sets it on the selection, F, Shift+F and Ctrl+F drag radius, strength and weight.
- Model tab: Chamfer has Segments and Tension (Blender: Shape) for rounded edges.

## Deutsch

- Model-Tab, Blender-Modus: Modellieren nimmt Blenders Tasten Alt+Klick (Kantenschleife), Strg+Alt+Klick (Kantenring), Strg+L (Verbundene wählen), J (Verbinden) und Strg+X (Auflösen).
- Model-Tab, Blender-Modus: der UV-Editor nimmt Strg+E (Naht markieren), P (Pin) und Umschalt+W (Menü Ausrichten).
- Model-Tab, Blender-Modus: Weights nimmt Blenders Tasten - Striche mit Strg und Umschalt ziehen ab und glätten, Umschalt+X nimmt das Gewicht auf, Strg+X setzt es auf die Auswahl, F, Umschalt+F und Strg+F ziehen Radius, Stärke und Gewicht.
- Model-Tab: Chamfer hat Segmente und Spannung (Blender: Form) für runde Kanten.

## 中文

- 模型选项卡, Blender 模式: 建模接受 Blender 的按键 Alt+单击 (边循环)、Ctrl+Alt+单击 (边环)、Ctrl+L (选择相连)、J (连接) 和 Ctrl+X (融并)。
- 模型选项卡, Blender 模式: UV 编辑器接受 Ctrl+E (标记接缝)、P (固定) 和 Shift+W (对齐菜单)。
- 模型选项卡, Blender 模式: 权重绘制接受 Blender 的按键——按住 Ctrl 和 Shift 绘制为减去和平滑, Shift+X 采样权重, Ctrl+X 设置到所选, F、Shift+F 和 Ctrl+F 拖动半径、强度和权重。
- 模型选项卡: 倒角有分段和张力 (Blender: 形状), 可做圆角边。

## 日本語

- モデルタブ、Blender 風: モデリングで Blender のキー Alt+クリック (辺ループ)、Ctrl+Alt+クリック (辺リング)、Ctrl+L (リンク選択)、J (接続)、Ctrl+X (溶解) が使えます。
- モデルタブ、Blender 風: UV エディターで Ctrl+E (シーム)、P (ピン)、Shift+W (整列メニュー) が使えます。
- モデルタブ、Blender 風: ウェイトペイントで Blender のキーが使えます - Ctrl と Shift のストロークで減算とスムーズ、Shift+X でウェイト取得、Ctrl+X で選択に設定、F・Shift+F・Ctrl+F で半径・強さ・ウェイトをドラッグ。
- モデルタブ: 面取りにセグメントとテンション (Blender: シェイプ) が付き、丸い辺を作れます。

## Español

- Pestaña Model, modo Blender: el modelado acepta las teclas de Blender Alt+clic (bucle de aristas), Ctrl+Alt+clic (anillo de aristas), Ctrl+L (seleccionar vinculado), J (unir) y Ctrl+X (disolver).
- Pestaña Model, modo Blender: el editor UV acepta Ctrl+E (marcar costura), P (fijar) y Mayús+W (menú Alinear).
- Pestaña Model, modo Blender: los pesos aceptan las teclas de Blender: trazos con Ctrl y Mayús restan y suavizan, Mayús+X muestrea el peso, Ctrl+X lo aplica a la selección, F, Mayús+F y Ctrl+F arrastran radio, intensidad y peso.
- Pestaña Model: Chamfer tiene Segmentos y Tensión (Blender: Forma) para aristas redondeadas.

## Čeština

- Karta Model, režim Blender: modelování přijímá klávesy Blenderu Alt+klepnutí (smyčka hran), Ctrl+Alt+klepnutí (prstenec hran), Ctrl+L (vybrat propojené), J (spojit) a Ctrl+X (rozpustit).
- Karta Model, režim Blender: editor UV přijímá Ctrl+E (označit šev), P (připíchnout) a Shift+W (nabídka Zarovnat).
- Karta Model, režim Blender: váhy přijímají klávesy Blenderu - tahy s Ctrl a Shift odečítají a vyhlazují, Shift+X odebere váhu, Ctrl+X ji nastaví na výběr, F, Shift+F a Ctrl+F táhnou poloměr, sílu a váhu.
- Karta Model: Chamfer má segmenty a napětí (Blender: tvar) pro zaoblené hrany.
