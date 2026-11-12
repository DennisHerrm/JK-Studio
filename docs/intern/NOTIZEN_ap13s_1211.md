# Notizen AP13S (12.11.): Model-Tab, Modifier-Stapel und Glätten-Vorschau - MBENTWURF Teil B, rc612

Paket AP13S aus `auftraege/welle_1211.md` Abschnitt 4 (Grundlage `auftrag_mbentwurf_b_1011.md`): die Zeilen **M1, M2, W1** der
Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md`. Maßstab: der freigegebene Entwurf (Bilder
`Y:\werkzeuge\mbe\fotos\entwurf\e_max_modell.png`, `e_blender_modell.png`, `e_max_weights.png`). Begonnen von Fable 5.2, beendet von
Opus 5.5 (Nachfolger ab `Y:\werkzeuge\ap13s\UEBERGABE.md`). Worktree `Y:\p_ap6`, Zweig `p_ap13s` auf karte1 `73c18113f`. Selbsttest
`gui/selbsttest_ap13s.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap13s | ap13sfoto | ap13shb`), ctest `modell_stapel`,
Skripte und Läufe `Y:\werkzeuge\ap13s\`.

## Was sich ändert

Neue Dateien: `include/bhed/modell/stapeldaten.h` (Eintrag `Modifikator`, im Dokument je Fläche `modell::Teil::stapel`),
`include/bhed/modell/stapel.h` und `src/modell/stapel.cpp` (Rechnen: Spiegeln über `wof::symmetrisieren`, Glätten als Catmull-Clark,
neu geschrieben), `tests/modell_stapel.cpp`, `gui/app_modelltab_stapel.cpp` (Gruppe, Menüs, Suche, Ansicht/Datei-Anschluss; Zustand
`ModellStapel` im eigenen Block am Ende von `gui/modelltab.h`), `tools/i18n_ap13s.py` (Präfix `Stp`, 6 Sprachen),
`tools/handbuch/inhalt_ap13s.py`, `gui/selbsttest_ap13s.inc`.

| Zeile | Was | Datei |
|---|---|---|
| M1 (wie 3ds Max, Modellieren) | Gruppe „Modifier stack“ zuoberst im Reiter Modify, Kurzinfo die Fläche; „Modifier List“ darüber; Zeilen von oben: Skin · Einträge (der letzte oben) · Edit Poly · „GLM surface torso“; Auge je Eintrag; darunter die Einstellungen des gewählten Eintrags, Ecken/Dreiecke davor → danach, Move up / Move down / Collapse To / Remove | `drawModellStapelGruppe()` |
| M2 (wie Blender, Modellieren) | Gruppe „Modifiers“ im Reiter Modifiers; „Add Modifier“; nur die Einträge, von oben nach unten in der Reihenfolge des Rechnens, je mit Auge und Datei-Schalter; Apply wie Blender (nur dieser Eintrag, Blenders Meldung, wenn er nicht der erste war) | dieselbe |
| W1 (Weights, Vorschau = angewendet) | derselbe Stapel im Bereich Weights (wie 3ds Max Skin gewählt; Klick auf Edit Poly/Skin wechselt den Bereich); die Ansicht zeigt das Netz, das Anwenden ergibt (eine Funktion für Ansicht, Datei und Anwenden); Gewichte des gezeichneten Netzes aus seinen eigenen Ecken | `stapelNetz()`, `stapelAnwenden()`, `gewichtsModell()` |
| Datei | leerer Stapel bzw. nur „preview only“: Speichern, „In die Karte“, JKA-Grenzen und die Prüfung vor dem Speichern nehmen das Dokument selbst (dieselbe Referenz, dieselben Bytes); Einträge mit „Write to the file“ gehen hinein | `modellStapelDateiInhalt()`; `app_modelltab_mb3.cpp` (`schreibenNach`), `app_modelltab_mesh.cpp` (`grenzListe`, `modellRequisitInKarte`), `app_modelltab.cpp` (`modellBefunde`) |
| Finden | zehn Zeilen in „Search all actions“, Untermenü im Rechtsklick der Ansicht und im Menü „Edit Mesh“, Rechtsklick auf die Zeile, Ziehen zum Umordnen, Tooltip überall | `modellStapelMenue()`, `modellStapelAktionenSuche()` |
| Rückgängig | jede Stapel-Aktion (Hinzufügen, Auge, Datei, Umordnen, Einstellung, Anwenden, Entfernen) ist ein Schritt des Dokuments; Wiederholen bringt sie zurück | `Dokument::aendern` |

AP8s zweizeiliger Stapel (`rolloutStapel` in `app_modelltab_gewichte.cpp`) ist durch die gemeinsame Gruppe ersetzt; seine Texte
(`MgRStapel`, `MgStapelSkin`, `MgStapelPoly` und Hinweise) benutzt die neue Gruppe weiter.

## Funde (rot nachgestellt, behoben, grün)

| Fund | Rot | Grün |
|---|---|---|
| **Symmetrisieren** (Werkzeug Modellieren) legte die Gewichte der gespiegelten Seite auf die Knochen der Quellseite: `knochenSpiegel()` tauschte nur `l_`/`r_`, die Figuren des Spiels heißen `lhumerus`, `rtibia` … | nach dem Werkzeug hängen 50 von 94 seitigen Gewichten von torso an der falschen Seite (vorher 0 von 88) | 0 von 94; `knochenSpiegel()` nimmt `modell::spiegelKnochen` (dieselbe Regel wie das Spiegeln der Gewichte) |
| Foto gegen den Entwurf: mit gewähltem Eintrag war die Gruppe viel höher als im Entwurf (vierzeiliger Tipp unter den Knöpfen), wie 3ds Max rutschten Tools und JKA limits aus dem Bild | Foto `ap13s_f_en/ap13s_max_modell` (Fable-Lauf) | Tipp und Hinweis zum leeren Stapel stehen im Tooltip des Knopfs und der Zeilen; Foto `ap13s_f_en/ap13s_max_modell` (Opus-Lauf): Selection und Tools wieder im Bild |
| Selbsttest ap6 (wie Blender, Requisit): mit dem Absatz „No modifier yet …“ und dem Tipp schob die Gruppe die Werkzeuge unter die Zeitleiste (Klick „Extrude“ kam nicht an) | ap6 89 OK / 11 FEHLER (Lauf g4) | ap6 95 / 0 (Lauf g5) |
| Foto: der Tooltip der Zeile lief ohne Umbruch über die halbe Ansicht (`SetItemTooltip`) | Foto `ap13s_f_de/ap13s_max_tooltip` (Lauf g5) | umbrochen wie die Knöpfe des Bausteins (24 Zeichenhöhen, CJK über `textumbruch.h`); Foto derselbe Name (Lauf g6) |
| Lint `lint_imgui_context`: Aufruf direkt nach `SetCursorPosY` ohne Anker (`app_modelltab_stapel.cpp`) | 1 Beanstandung | `Dummy({0,0})` als Anker, Abstand unverändert; 0 Beanstandungen |
| Deutscher Text „Ausblenden bis“ / „Alle ausblenden“ für Collapse To / Collapse All las sich wie Verstecken | - | „Reduzieren bis“, „Alle reduzieren“ (wie das deutsche 3ds Max) |

## Selbsttest (rot -> grün)

Roter Lauf = Selbsttest ohne die Anschlüsse (Bibliothek und Gruppe vorhanden, nicht eingehängt), grüner Lauf = Stand der Abgabe.

| Zeile | rot | grün (Direct3D 12) | grün (Direct3D 11) |
|---|---|---|---|
| M1 | 2 OK / 39 FEHLER | 41 / 0 | 41 / 0 |
| M2 | 2 / 40 | 42 / 0 | 42 / 0 |
| W1 | 4 / 9 | 13 / 0 | 13 / 0 |
| Finden | 4 / 4 | 8 / 0 | 8 / 0 |
| Datei | 5 / 1 | 6 / 0 | 6 / 0 |
| Requisit (.md3) | 5 / 2 | 7 / 0 | 7 / 0 |
| Fund Symmetrisieren | 1 / 1 | 2 / 0 | 2 / 0 |
| Summe | 27 / 123 | 123 / 0 | 122 / 0 (ohne die Zeile zum Abgeber von Direct3D 12) |

Messzeilen (grüner Lauf, kyle, Fläche torso, LOD 1):
- Dokument 434 Dreiecke, gezeichnet mit Glätten 2604 (× 6); die Prüfung mit 3 Unterteilungen (× 96) grün.
- W1: Netz der Vorschau Ecke für Ecke gleich dem angewendeten; Prüfsumme der gezeichneten Fläche gleich (`e4f677c5798fa027` wie
  3ds Max, `034875094c90271e` wie Blender), 0 Bildpunkte anders.
- Auge aus: 0 Bildpunkte anders als ohne Eintrag. Gespiegelt: 1534 Ecken, 0 ohne Gegenstück.
- Wie 3ds Max „Collapse To“ über Symmetry + TurboSmooth: 1553 Ecken, 2644 Dreiecke, Netz gleich der Vorschau, Rest im Stapel 0.
- Wie Blender „Apply“ auf Mirror: 318 Ecken, 442 Dreiecke, Netz gleich der Vorschau, Subdivision bleibt.
- Datei, leerer Stapel: 330284 Bytes, Prüfsumme `85daff658bf5ca1e` = die Datei des Spiels (im roten Lauf, also mit dem Speicherweg vor
  AP13S, dieselbe); geändertes Modell `97d96ed01b5b4442`, mit einem Eintrag „preview only“ dieselbe. Mit Spiegeln in die Datei:
  332780 Bytes, torso 318 Ecken (Soll 318, Dokument 258).
- Suche „modifier“: 10 von 10 Aktionen.

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

- `gui/selbsttest_ap6.inc`: die Gruppenliste des rechten Felds (M0a/M0b und M11 Requisit) erwartet den Stapel über Auswahl, Werkzeuge,
  JKA-Grenzen.

## Fotos

Foto-Selbsttest `ap13sfoto` (je Bedienmodus leer, Spiegeln, Hinzufügen, Rechtsklick, Tooltip, Grenze, Auge aus, Weights, dazu das
Requisit), Läufe `Y:\werkzeuge\ap13s\fotos\ap13s_f_<variante>`, Vergleiche neben dem Entwurf in `Y:\werkzeuge\ap13s\vergleich\`:
- en, de (Direct3D 12) und d3d11: 21 Bilder je Lauf, 3818 × 2104; ja, hell: dto.; klein: Fenster 1280 × 720 bei Oberfläche 1,0
  (1328 × 844 - die Mindestgröße selbst, nicht verkleinert). Alle Läufe 0 FEHLER. Angesehen: max/blender Modellieren und max Weights
  neben dem Entwurf (en, de, ja, hell), klein, Tooltip (de), Rechtsklick (Blender), leerer Stapel (3ds Max), die zwei Handbuchbilder
  (de, ja). Lange Texte (de „In die Datei schreiben“) brechen um, CJK passt.
- Direct3D 12 gegen 11 (`bildgleich.py`, en gegen d3d11, 19 gemeinsame Bilder): 2 gleich, 17 mit 1-2 Bildpunkten um 1/255 (dieselbe
  Abweichung an Kanten der Mehrfachabtastung, die D3D12-3 als Punkt 5 untersucht); nichts über 8/255.

## Handbuch

`tools/handbuch/inhalt_ap13s.py`: Abschnitt „Der Modifier-Stapel“ (drei Absätze, zwei Bilder, eine Notiz) vor „grenzen“ der Seite
modell-modellieren, 6 Sprachen. Nicht eingehängt (wie AP6-AP10; AP14 führt zusammen) - die vier Zeilen zum Einhängen stehen im Kopf der
Datei, dazu `i18n_ap8` und `i18n_ap13s` in `_lade_ui_texte`. Geprüft durch vorübergehendes Einhängen: `gen_handbuch.py --pruefen`
(dazu `i18n_ap8`, `i18n_ap13s` geladen): `handbuch_sprachen: ok`, 0 Texte und 0 Bilder noch englisch (es, cs); danach
zurückgesetzt. Bilder `handbuch/bilder/<sprache>/h_ap13s_max.webp`, `h_ap13s_blender.webp` (Selbsttest `ap13shb`).

## Schlussrunde des Zweigs

Alles auf dem Stand der Abgabe (Bau g6; Last gesenkt: `--parallel 2`, Läufe mit Leerlauf-Priorität, höchstens 2 gleichzeitig):
- Bau `behaved` 0 Fehler, 0 Warnungen (/W4 /WX).
- ctest `-C Release -j 2`: 259 / 0 (darin `modell_stapel` 36 / 0).
- Selbsttest `ap13s`: 123 / 0 (Direct3D 12), 122 / 0 (Direct3D 11; ohne die Zeile zum Abgeber von Direct3D 12).
- Gruppen, die der Code berührt (Bau g5, nur das Feld seither geändert): ap6 95 / 0, ap3 106 / 0, ap4 106 / 0, md 88 / 0, mdkarte
  9 / 0, mb3 23 / 0, funktionen 63 / 0; ap8 104 / 1 (Bau g4) - die eine Zeile W12 wie Blender ist auch ohne AP13S rot (siehe Offen).
- Bewusst nicht gelaufen: ap5, ap7, ap9, ap10, mbv, tasten, blender, pfix, pfix2, nav, kopf, mb2 (berühren weder Stapel, Ansicht noch
  Speichern; Gruppenlisten des rechten Felds prüfen nur ap3/ap4/ap6/ap8, alle gelaufen), Modus 1, volle Runde (Hauptchat).
- Lints (Arbeitskopie): i18n, i18n_erzeugt, gapp, headers, feldinit, sources, abstand, breitenabzug, cjkumbruch, doppelanzeige, hoehe,
  imgui_alpha, imgui_context, layout, tabellenid, endtable, settings ok; portability 99 und pruefcodes 11 Beanstandungen, keine davon
  in einer Datei von AP13S.
- Private Adressen: 0 Treffer in Log und Diff.

## Entscheidungen (mit Grund)

1. **Stapel je Fläche** (nicht je Modell): der Entwurf zeigt „GLM-Fläche torso“ als Grundzeile und die Kurzinfo „torso“.
2. **Glätten ab Werk „preview only“**, Spiegeln geht in die Datei: so beschriftet es der Entwurf; ein Schritt versechsfacht die
   Dreiecke (torso 434 → 2604) und sprengt die 1000 Ecken je Fläche.
3. **Skin steht über den Einträgen**: sie rechnen in der Bindepose, die Figur wird danach verformt. Der Entwurf M1 zeigt Skin im Bereich
   Modellieren nicht; die Tabelle verlangt aber einen gemeinsamen Stapel (W1).
4. **Edit Poly, Skin und die Fläche haben kein Auge** (der Entwurf zeigt eines): es gäbe nichts auszuschalten, ein Auge ohne Wirkung wäre
   eine Attrappe. Leicht umstellbar (`stufenZeile` in `drawModellStapelGruppe()`, `ZeileOpt::auge`) - Frage 1.
5. **Anwenden:** wie Blender nur der gewählte Eintrag; wie 3ds Max „Collapse To“ alle eingeschalteten bis hierher. Ein ausgeschalteter
   Eintrag wird nicht angewendet (Meldung). Angewendet wird in jeder LOD-Stufe.
6. **„Keep borders“ ab Werk an:** die Flächen einer .glm stoßen an ihren Rändern aneinander; glättet der Rand mit, reißt es auf.
7. **JKA-Grenzen und die Prüfung vor dem Speichern zählen, was in die Datei geht.**
8. **Platz nur, wenn gebraucht:** kein Absatz unter dem Stapel; Hinweis und Tipp im Tooltip.

## Offen / Fragen an shank

Fragen (nicht blockierend; bis zur Antwort gilt der heutige Stand):
1. Sollen Edit Poly und Skin ein Auge bekommen (Edit Poly aus = Fläche wie geladen zeigen, Skin aus = Bindepose)? Heute ohne Auge.
2. Der Stapel steht nicht in der `.glm`: nach Schließen und Öffnen sind die Einträge weg (was in die Datei ging, ist dann echtes Netz).
   Soll er in einer Begleitdatei bleiben?
3. Eine Zeile „Unwrap UVW“ im Stapel wie 3ds Max (wechselt in den Bereich UV)? Der Entwurf zeigt sie nicht; heute nicht gebaut.

Offen:
- Deutscher Gruppentitel wie Blender ist „Modifier“ (so der Entwurf); der Reiter von AP3 heißt „Modifikatoren“. Nicht angeglichen.
- Das Datei-Symbol der Zeile ist eine Diskette (`UiIcon::Speichern`), der Entwurf zeigt ▣.
- Selbsttest ap8, Zeile „W12 wie Blender: ein anderer Knochen daneben“ ist rot - auch auf dem Zweig von D3D12-3 ohne AP13S
  (`Y:\werkzeuge\dx12\laeufe\sw1_12`, `sw2_12`: 104 / 1, `sw3_12`: 105 / 0): schwankt, nicht von AP13S.

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP13S nicht angefasst)

| Zeile | Stand |
|---|---|
| M1 | gleich (Gruppe, Kurzinfo, Reihenfolge, Auge an den Einträgen, TurboSmooth „preview only“); bewusst anders: Skin als oberste Zeile (W1), Edit Poly/Fläche ohne Auge (Entscheidung 4, Frage 1), Einstellungen und Knöpfe des gewählten Eintrags unter der Liste |
| M2 | gleich (Gruppe Modifiers im Reiter Modifiers, Mirror (X) mit Auge und Datei-Schalter, Subdivision „preview only“, Reihenfolge); Datei-Symbol Diskette statt ▣ |
| W1 | gleich (gemeinsamer Stapel, Skin gewählt über Edit Poly, Klick wechselt den Bereich); Glätten-Vorschau = angewendetes Ergebnis belegt |

## Changelog

## English

- Model tab: every surface has a modifier stack. In 3ds Max mode the group "Modifier stack" stands at the top of the tab Modify, in the Modeling and in the Weights area, and reads from the bottom up: the surface of the file, Edit Poly, the modifiers, Skin - a click on Edit Poly or Skin switches the area. In Blender mode the group "Modifiers" stands on the tab Modifiers and lists the modifiers from the top down.
- Two modifiers: Symmetry (Blender mode: Mirror) cuts the surface at an axis plane and mirrors one side onto the other, including the bone weights; TurboSmooth (Blender mode: Subdivision) subdivides and rounds the surface one to three times and can leave open borders in place.
- Each modifier has the eye (on/off) and "Write to the file"; smoothing is a preview only at first. Modifiers can be moved up and down (also by dragging), applied (Blender mode: Apply, 3ds Max mode: Collapse To and Collapse All) and removed; each of these is one undo step. The commands are also in the right-click menu of a row, in the right-click menu of the view and in "Search all actions".
- What the view shows for a modifier is exactly the mesh that applying it produces. As long as a surface has no modifier that goes into the file, saving writes the same file as before.
- JKA limits and the check before saving count what goes into the file, including modifiers that are written to it.
- Modeling, Symmetrize: the mirrored side now hangs on the bones of its own side for every bone pair of the skeleton (lhumerus and rhumerus, ltibia and rtibia ...); until now only names beginning with l_ and r_ were swapped.

## Deutsch

- Model-Tab: jede Fläche hat einen Modifier-Stapel. Im 3ds-Max-Modus steht die Gruppe „Modifier-Stapel“ zuoberst im Reiter Ändern, im Bereich Modellieren und im Bereich Weights, und liest sich von unten nach oben: die Fläche der Datei, Edit Poly, die Modifier, Skin - ein Klick auf Edit Poly oder Skin wechselt den Bereich. Im Blender-Modus steht die Gruppe „Modifier“ im Reiter Modifikatoren und nennt die Modifier von oben nach unten.
- Zwei Modifier: Symmetrie (Blender-Modus: Spiegeln) schneidet die Fläche an einer Achsenebene und spiegelt eine Seite auf die andere, samt den Knochengewichten; TurboSmooth (Blender-Modus: Unterteilung) unterteilt und rundet die Fläche ein- bis dreimal und kann offene Ränder stehen lassen.
- Jeder Modifier hat das Auge (an/aus) und „In die Datei schreiben“; Glätten ist zunächst nur Vorschau. Modifier lassen sich nach oben und unten schieben (auch durch Ziehen), anwenden (Blender-Modus: Anwenden, 3ds-Max-Modus: Reduzieren bis und Alle reduzieren) und entfernen; jedes davon ist ein Rückgängig-Schritt. Die Befehle stehen auch im Rechtsklick-Menü einer Zeile, im Rechtsklick-Menü der Ansicht und in „Alle Aktionen suchen“.
- Was die Ansicht für einen Modifier zeigt, ist genau das Netz, das Anwenden ergibt. Solange eine Fläche keinen Modifier hat, der in die Datei geht, schreibt Speichern dieselbe Datei wie bisher.
- JKA-Grenzen und die Prüfung vor dem Speichern zählen, was in die Datei geht, samt den Modifiern, die hineingeschrieben werden.
- Modellieren, Symmetrisieren: die gespiegelte Seite hängt jetzt bei jedem Knochenpaar des Skeletts an den Knochen ihrer Seite (lhumerus und rhumerus, ltibia und rtibia ...); bisher wurden nur Namen getauscht, die mit l_ und r_ beginnen.

## 中文

- 模型选项卡: 每个面都有一个修改器堆栈。3ds Max 模式下, “修改器堆栈”组位于“修改”选项卡的最上方 (建模区域和 Weights 区域), 自下而上读: 文件中的面、Edit Poly、各修改器、Skin - 单击 Edit Poly 或 Skin 可切换区域。Blender 模式下, “修改器”组位于“修改器”选项卡, 自上而下列出修改器。
- 两种修改器: 对称 (Blender 模式: 镜像) 在轴平面处切开面并把一侧镜像到另一侧, 骨骼权重也一并镜像; 涡轮平滑 (Blender 模式: 细分) 把面细分并圆滑一到三次, 并可让开放边界保持原位。
- 每个修改器都有眼睛图标 (开/关) 和“写入文件”; 平滑起初仅作预览。修改器可以上移和下移 (也可拖动)、应用 (Blender 模式: 应用; 3ds Max 模式: 塌陷到、全部塌陷) 和移除; 每一项都是一步撤销。这些命令也在行的右键菜单、视图的右键菜单和“搜索所有操作”中。
- 视图为修改器显示的内容, 正是应用后得到的网格。只要一个面没有写入文件的修改器, 保存时写出的文件就与以前相同。
- JKA 限制和保存前的检查按写入文件的内容计数, 包括写入文件的修改器。
- 建模, 对称: 对骨架中的每一对骨骼 (lhumerus 与 rhumerus、ltibia 与 rtibia 等), 镜像出的一侧现在都绑定到本侧的骨骼; 此前只交换以 l_ 和 r_ 开头的名称。

## 日本語

- モデルタブ: サーフェスごとにモディファイアスタックがあります。3ds Max 風では「モディファイアスタック」グループが「修正」タブの一番上 (モデリングエリアと Weights エリア) にあり、下から上へ読みます: ファイルのサーフェス、Edit Poly、モディファイア、Skin - Edit Poly または Skin をクリックするとエリアが切り替わります。Blender 風では「モディファイア」グループが「モディファイアー」タブにあり、モディファイアを上から下へ並べます。
- モディファイアは 2 種類: シンメトリ (Blender 風: ミラー) は軸平面でサーフェスを切り、ボーンウェイトも含めて片側をもう片側にミラーします。ターボスムーズ (Blender 風: サブディビジョン) はサーフェスを 1〜3 回細分して丸め、開いた境界をその場に残すこともできます。
- 各モディファイアには目のアイコン (オン/オフ) と「ファイルに書き出す」があります。スムーズは最初プレビューのみです。モディファイアは上下に移動 (ドラッグも可)、適用 (Blender 風: 適用、3ds Max 風: ここまで集約・すべて集約)、削除ができ、どれも元に戻す 1 ステップです。コマンドは行の右クリックメニュー、ビューの右クリックメニュー、「すべての操作を検索」にもあります。
- ビューがモディファイアについて表示するものは、適用して得られるメッシュそのものです。ファイルに書き出すモディファイアがサーフェスにない間は、保存で書かれるファイルはこれまでと同じです。
- JKA の上限と保存前のチェックは、ファイルに書き出すモディファイアを含め、ファイルに入る内容を数えます。
- モデリングの対称化: ミラーされた側は、スケルトンのすべてのボーンの対 (lhumerus と rhumerus、ltibia と rtibia など) で自分の側のボーンに付くようになりました。これまでは l_ と r_ で始まる名前だけを入れ替えていました。

## Español

- Pestaña Model: cada superficie tiene una pila de modificadores. En modo 3ds Max el grupo «Pila de modificadores» está arriba del todo en la pestaña Modificar, en el área de modelado y en el área Weights, y se lee de abajo arriba: la superficie del archivo, Edit Poly, los modificadores, Skin; un clic en Edit Poly o Skin cambia de área. En modo Blender el grupo «Modificadores» está en la pestaña Modificadores y enumera los modificadores de arriba abajo.
- Dos modificadores: Simetría (modo Blender: Espejo) corta la superficie en un plano de eje y refleja un lado sobre el otro, pesos de hueso incluidos; TurboSmooth (modo Blender: Subdivisión) subdivide y redondea la superficie de una a tres veces y puede dejar los bordes abiertos en su sitio.
- Cada modificador tiene el ojo (activado/desactivado) y «Escribir en el archivo»; el suavizado es al principio solo vista previa. Los modificadores se pueden subir y bajar (también arrastrando), aplicar (modo Blender: Aplicar; modo 3ds Max: Contraer hasta y Contraer todo) y quitar; cada una de estas acciones es un paso de deshacer. Los comandos están también en el menú contextual de una fila, en el menú contextual de la vista y en «Buscar todas las acciones».
- Lo que la vista muestra para un modificador es exactamente la malla que resulta de aplicarlo. Mientras una superficie no tenga un modificador que vaya al archivo, guardar escribe el mismo archivo que antes.
- Los límites de JKA y la comprobación antes de guardar cuentan lo que va al archivo, incluidos los modificadores que se escriben en él.
- Modelado, Simetrizar: el lado reflejado queda ahora unido a los huesos de su propio lado para cada par de huesos del esqueleto (lhumerus y rhumerus, ltibia y rtibia...); hasta ahora solo se intercambiaban los nombres que empiezan por l_ y r_.

## Čeština

- Karta Model: každý povrch má zásobník modifikátorů. V režimu 3ds Max je skupina „Zásobník modifikátorů“ úplně nahoře na kartě Upravit, v oblasti modelování i v oblasti Weights, a čte se zdola nahoru: povrch ze souboru, Edit Poly, modifikátory, Skin - kliknutí na Edit Poly nebo Skin přepne oblast. V režimu Blender je skupina „Modifikátory“ na kartě Modifikátory a uvádí modifikátory shora dolů.
- Dva modifikátory: Symetrie (režim Blender: Zrcadlení) rozřízne povrch v osové rovině a zrcadlí jednu stranu na druhou včetně vah kostí; TurboSmooth (režim Blender: Dělení povrchu) povrch jednou až třikrát rozdělí a zaoblí a může nechat otevřené okraje na místě.
- Každý modifikátor má oko (zapnuto/vypnuto) a „Zapsat do souboru“; vyhlazení je zpočátku jen náhled. Modifikátory lze posouvat nahoru a dolů (i tažením), použít (režim Blender: Použít, režim 3ds Max: Sbalit po a Sbalit vše) a odebrat; každá z těchto akcí je jeden krok zpět. Příkazy jsou také v nabídce pravého tlačítka na řádku, v nabídce pravého tlačítka v pohledu a v „Hledat všechny akce“.
- To, co pohled pro modifikátor ukazuje, je přesně síť, která vznikne jeho použitím. Dokud povrch nemá modifikátor, který jde do souboru, zapíše uložení stejný soubor jako dosud.
- Limity JKA a kontrola před uložením počítají to, co jde do souboru, včetně modifikátorů, které se do něj zapisují.
- Modelování, Symetrizovat: zrcadlená strana je teď u každého páru kostí kostry (lhumerus a rhumerus, ltibia a rtibia ...) navázána na kosti své strany; dosud se zaměňovala jen jména začínající na l_ a r_.
