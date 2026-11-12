# Notizen FIX612B (12.11.): Funde der Code-Prüfung P2 - Gewichte, Malen, Auto-Skin, rc612

Auftrag FIX612B aus `auftraege/welle_1211.md` (Kopf): die Funde **P2-1 bis P2-10** und die Vermutung V1 aus
`plaene/codepruefung_seit_rc611_p2.md`. Begonnen von Fable 5.2 (Selbsttest und Entwurf, nie gebaut), beendet von Opus 5.5
(Nachfolger ab `Y:\werkzeuge\fix612b\UEBERGABE.md`). Worktree `Y:\p_ap9`, Zweig `p_fix612b`, zuletzt auf karte1 `82848b14e`
(AP13S Modifier-Stapel, AP13K Knochen-Darstellung). Selbsttest `gui/selbsttest_fix612b.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=fix612b` oder `fix612babsturz`; `BHED_FIX612B=p22,p23,...` wählt Abschnitte;
`BHED_FIX612B_MODELL`, `BHED_FIX612B_STUFEN` die Modelle). Läufe und Skripte `Y:\werkzeuge\fix612b\`.

Entscheidungen von shank (Hauptchat, `plaene/entscheidungen_1211.md` A13-A17), alle wie gebaut: Speichern schreibt, was man
sieht, die Mallisten bleiben offen; Weights gelten der gezeigten Stufe; die Auswahl wird je Stufe gemerkt; „Nur Auswahl“ +
„Spiegeln“ ändert die Gegenseite nur, wenn sie mitgewählt ist; Strg+Z im Strich bricht ab und nimmt zurück, Strg+Y bringt ihn
wieder.

## Funde: rot -> grün

Rot = Stand ohne Behebung (Test + Deklarationen, Läufe `rot_fix612b`, `rot2_*`, `rot3_*`), grün = Lauf `gruen2_*` bzw.
`gruen5_fix612b` (P2-3). Protokolle `Y:\p_ap9\build\laeufe\<lauf>\jkstudio-detail.log`.

| Fund | Rot (im laufenden Programm) | Behebung | Grün |
|---|---|---|---|
| **P2-2 [H]** Speichern mit offenen Mallisten | gemalt (rhumerus 0 -> 0,999 an torso-Ecke 120), Speichern: Datei **bytegleich mit dem Original**, 657 von 5567 Ecken anders als die Anzeige; Prüfung vor dem Speichern mit 40 Knochen nur in Mallisten: 0 Befunde | `modellInhaltGezeigt()` (gui/app_modelltab.cpp): auf dem Inhalt für die Datei (Modifier-Stapel von AP13S) eine Kopie mit übernommenen Mallisten (`modell::wieGezeigt`), je Dokumentstand gemerkt. Schreiben (`schreibenNach`), Prüfung vor dem Speichern (`modellBefunde`) und JKA-Grenzen (`grenzListe`) nehmen sie; das Dokument behält die Mallisten; Meldung nach dem Speichern und Tooltip von „Übernehmen“ sagen es | 0 von 5567 Ecken anders als die Anzeige, an der Ecke 0,999022 in der Datei; Prüfung 1 Befund; Dokument gilt als gespeichert, Mallisten offen |
| P2-2: Format des Spiels **an den Bytes** | - | - | eigene Lesung der Bytes (`dateiMass`): höchstens 4 Gewichte je Ecke (Bit 30-31), Rest des letzten > 0 (Summe der ersten n-1 Zehn-Bit-Werte < 1023) an 0 Ecken verletzt, 0 Verweise hinter der Liste, 0 Flächen über 32 Verweise, 0 Gewichte 0 vor dem letzten - wie das Original; nach Modellieren + Malen (P2-4) ebenso, 0 von 5586 Ecken anders als die Anzeige |
| **P2-3 [M]** X-Spiegeln, Stufe wechselt im Strich | stormtrooper (4 Stufen; kyle hat unter Movie Duels nur eine), Strich in Stufe 1, Rad zoomt hinein: Stufe wechselt mitten im Strich, Änderungen in Stufe 0 (2210 Ecken) und 1, in Stufe 0 539 gespiegelte Punkte ohne gemaltes Gegenstück. Absturzrichtung (`fix612babsturz`, Schweißnetz der Stufe 0 an Stufe 3): **Zugriffsverletzung 0xC0000005** | `modellStufeGehalten()`: solange Strich, Verlauf, Gizmo-Zug, G/R/S, Werkzeugkästchen oder UV-Zug offen ist, bleibt die gezeichnete Stufe; im Kern `netzPasst` in `spiegelnSeite`, `spiegelnBlender`, `spiegelMalen` (ein Schweißnetz einer anderen Stufe tut nichts) | Stufe bleibt im Strich, nur Stufe 1 geändert (894 Ecken), 390 Punkte gespiegelt, 0 ohne gemaltes Gegenstück, nach dem Loslassen die nahe Stufe; Absturzrichtung: `spiegelMalen` gibt 0 zurück, nichts geändert |
| **P2-9 [M]** Auswahl beim Stufenwechsel | fünf Ecken von head in Stufe 0 gewählt, Stufe 1: Auswahl hat weiter 5 Nummern, „Setzen“ ändert 6 Ecken der Stufe 1; drei Vielecke im Modellieren bleiben gewählt; offenes Kästchen: Stufe wechselt (1) | `stufeGewechselt`: die Auswahl (Ecken, Arbeitsnetz, Punktfolge) gehört zu ihrer Stufe und wird je Stufe gemerkt; zurück kommt sie bei gleichem Modell, gleicher Gestalt der Netze dieser Stufe (Ecken, Kanten, Punkte je Fläche - Gewichte malen ändert sie nicht) und gleicher Auswahl-Ebene | Auswahl nach dem Wechsel 0, „Setzen“ 0 Ecken, Arbeitsnetz 0 Vielecke, zurück in Stufe 0 wieder die fünf Ecken; Kästchen hält Stufe 0, danach 1 |
| **P2-4 [M]** Modellieren verwirft Mallisten | Punkt an der Grenze torso/hips auf 0,5 gesetzt, Extrudieren auf torso: torso-Ecke wieder 0,0, hips 0,5; Datei 62 Ecken anders als die Anzeige | `polyAus` nimmt die gezeigten Gewichte; `polyZurueck` trägt die Mallisten mit (unberührte Ecke gleichen Orts und gleicher Gewichte behält ihre Liste, jede andere bekommt die Liste ihrer Gewichte) | torso 0,5 wie hips, Liste gleich der von hips; Datei 0 von 5586 anders |
| **P2-1 [M]** Auto-Skin „Nur Auswahl“ + „Spiegeln“ | nur l_arm gewählt: 22 nicht gewählte Flächen geändert (hips, r_leg, torso, head ...), offene Malliste von head übernommen | `spiegelnSeite(..., nurTeile)` schreibt nur in die gewählten Flächen (gelesen wird überall); übernommen werden nur die Mallisten, die das Spiegeln anlegt; Tooltip von „Danach spiegeln“ nennt es | 0 Flächen geändert, head-Malliste offen; mit beiden Armen gewählt 46 Ecken gespiegelt, Arm bei x < 0 trägt seinen Oberarm (26,16), fremder 0 |
| **P2-10 [M]** Strg+Z im Strich/Zug | Pinsel, Verlauf, Gizmo: nach dem Zug geändert, oberster Schritt leer (der Schritt lag auf Wiederholen), die Taste malte weiter; Painting: 59176 Texel nach allem Rückgängig anders | Strg+Z beendet Strich, Verlauf oder Gizmo-Zug und nimmt ihn zurück (`modellAnsichtOffenesAbbrechen(true)`, gemerkte Kennung, `Dokument::obenIst`), Strg+Y beendet nur; Painting: `modellMalenRueckgaengig/Wiederholen` beenden zuerst den Strich; `fuerAuswahl` prüft auch die Teile von `x`; Tooltip des Mal-Knopfs nennt es | unverändert, Strich endet (0 Tupfer danach), Wiederholen = der Strich, Strg+Y bringt ihn (Pinsel, Verlauf, Gizmo ja); Painting 0 Texel anders, Strg+Y bringt 38165 Texel wieder |
| **P2-5 [N]** Befehl ohne Wirkung löscht Wiederholen | „Alles normalisieren“ mit 0 Änderungen: Wiederholen danach 0 | `Dokument`: `aendern` legt das Wiederholen beiseite (`vorWeg_`), `abbrechen` des jüngsten Schritts legt es zurück | Wiederholen bleibt (1), Verlauf-Klick ohne Strecke lässt es stehen, Wiederholen bringt die Änderung |
| **P2-6 [N]** Pinselart eines Strichs neben der Figur | Art des Strichs 1 (Abziehen), gewählt 0, Gewicht blieb 0 | `strichArt` beim Strichbeginn gesetzt, vor der Rückkehr bei 0 Ecken | Art 0, Gewicht 0,2498 |
| **P2-7 [N]** Sperren nach Modellwechsel | luke (53 Knochen wie kyle): 1 Knochen gesperrt | `sperrListeAnpassen` leert bei neuem `ladeStand`; `modellGewichtGesperrt` passt an | 0 gesperrt |
| **P2-8 [N]** Painting mit Radius 0 | 59992 Tupfer auf 6 Bildpunkten, 16,2 s | Regler klemmt 1..500 (wie Tasten [ ] und F), Schritt mit Radius mindestens 1 | 61 Tupfer, 19 ms |
| V1 (Vermutung) Auto-Skin, Knochen weit außerhalb | **nicht bestätigt**: Kopf allein 256: 1,4 s gegen 1,1 s ganze Figur; 512: 13,3 s gegen 10,5 s (unter Last) - keine Minuten | nichts geändert (Auftrag: nicht Nachstellbares belegen, nichts ändern) | Prüfzeile „höchstens doppelt so lang“ grün |

Testanpassungen nach dem ersten roten Lauf (kein Abschwächen): P2-3/P2-9/Absturz auf stormtrooper (kyle hat unter MD eine Stufe);
P2-5 normalisiert zuerst bis 0 Änderungen mit dem späteren aktiven Knochen („Alles normalisieren“ hält ihn fest); P2-3: kleiner
Pinsel, mehr Rad hinein als heraus (das Rad zoomt zur Maus), Warten auf das wirkliche Ende des Strichs (ImGui verteilt Rad- und
Mausereignisse auf Bilder; nach 8 Bildern war die Taste noch nicht losgelassen), und die Spiegelprüfung zählt nur Änderungen am
Spiegelknochen (stormtrooper hält das Gewehr vor dem Bauch, der Pinsel malt dort selbst über die Mitte); P2-10 prüft nach
Entscheidung A17 „zurückgenommen, Strg+Y bringt ihn“ statt „verworfen“. Die P2-3-Zeilen „Stufe wechselt nicht“ und „nur die
Stufe des Strichs“ waren mit dieser Strichführung rot (Stufe 0 geändert); die Spiegelzeile wurde nach der Änderung nur grün gesehen.

## Zusammenführen mit AP13S

AP13S hat Speichern, Prüfung und JKA-Grenzen auf `modellStapelDateiInhalt()` umgestellt. `modellInhaltGezeigt()` baut darauf
auf: erst der Stapel (Flächen mit Stapel übernehmen ihre Mallisten in `stapelNetz` schon selbst), dann die übrigen Mallisten.
Ohne Stapel und ohne Mallisten ist es das Dokument selbst (dieselbe Referenz, dieselben Bytes). Symmetrisieren (AP13S) läuft
über `polyZurueck`: mit offenen Mallisten tragen die neuen Ecken jetzt Listen ihrer gespiegelten Gewichte (P2-4).

## Schlussrunde des Zweigs

- Bau ALLES (`--parallel 2`, /W4 /WX): 0 Fehler, 0 Warnungen.
- Selbsttest `fix612b` und `fix612babsturz`: siehe Tabelle.
- Weitere Gruppen und ctest: siehe Abschnitt „Läufe“.
- Handbuch: `tools/handbuch/inhalt_fix612b.py` (Seite modell-weights, Abschnitt „Speichern, Detailstufe und Strg+Z beim Malen“,
  6 Sprachen), `gen_handbuch.py` 178 Seiten, es/cs vollständig; keine neuen Bilder (reiner Text zu Verhalten, das man nicht
  sieht: Datei, Stufe, Rückgängig).
- Texte `tools/i18n_fix612b.py` (5, 6 Sprachen): Satz nach dem Speichern, Zusätze zu den Tooltips „Übernehmen“, „Danach
  spiegeln“, „Auto“ (Stufe), Mal-Knopf (Strg+Z).

## Offen

- P2-5 Verlauf-Klick: rot nur zusammen mit dem Normalisieren gesehen (derselbe Mechanismus `aendern`/`abbrechen`).
- `mischen` (mesh_netz) normiert gemischte Gewichte ohne Runden auf 10 Bit; die Bytes nach Extrude zeigen 0 Ecken mit Rest <= 0
  (der Schreiber rechnet über `fuersSpiel`) - kein Fund.

## Läufe

Last-Regel des Hauptchats (12.11. abends): Bau `--parallel 2` auf Leerlauf-Priorität, höchstens zwei jkstudio gleichzeitig.

| Lauf | Ergebnis |
|---|---|
| Bau ALLES (Stand vor dem Rebase auf FIX612C) | 0 Fehler, 0 Warnungen |
| ctest | 260/260 |
| `fix612b` (gruen2) | 37 OK, 2 FEHLER (beide P2-3, Test; danach gruen5 6/0) |
| `fix612babsturz` (gruen2) | 3/0 |
| ap8 (Weights) | 105/0 |
| ap12 (Auto-Skin) | 36/0 |
| mbv (Bearbeiten in der Pose) | 217/0 |
| ap6 (Modellieren) | 95/0 |
| pt (Painting) | 43/0 |
| ap13s (Modifier-Stapel, Speichern) | 123/0 |
| md (Modellieren 1d) | 88/0 |
| uv | 52/0 |
| mb2 | 38/1: „die Farbe wie Blender weight_to_rgb … die Füße blau“ - die Probe in der Mitte von r_leg trifft einen Knochen der neuen Knochen-Darstellung von AP13K (Foto `k_mb2/fotos/mb2_weights_kopf.bmp`: grüne/orange Stäbe über dem Bein, Farbe 108/116/71); der Kopf ist rot wie verlangt. Nicht von FIX612B (kein Code dieses Zweigs zeichnet), an den Hauptchat gemeldet |
| Lints | alle ok bis auf die bekannten pruefcodes 11 und portability 101 (keine Datei dieses Zweigs) |
| Handbuch | `gen_handbuch.py` 178 Seiten, es/cs 0 englisch |

Nach dem Rebase auf karte1 `133c47078` (FIX612C, LCFIX): Bau ALLES 0 Fehler, 0 Warnungen; `fix612b` 39/0, `fix612babsturz` 3/0,
`fix612c` 36/0 (FIX612C hat `modellOffenesAbbrechen` und das Ende des Gizmo-Zugs geaendert - beides vertraegt sich mit P2-10).

## Changelog

## English

- Model tab, Weights: saving writes the weights the view shows, also while paint lists are open (after painting or a weight command, before Apply). The file gets at most four weights per vertex, normalized like the game reads them; the paint lists stay open. The check before saving and the JKA limits measure the same. Until now saving wrote the weights from before painting and said "saved".
- Weights: while a stroke, gradient, gizmo drag or tool box is open, the level of detail stays, even if the wheel zooms or Auto would change it. Mirror painting could write to wrong vertices or crash when the level changed during a stroke.
- Weights and Modeling: a selection of vertices, edges or polygons belongs to its level of detail; after a change of level it is empty, and it comes back with that level as long as its mesh was not changed. Until now the numbers stayed and commands changed other vertices.
- Modeling: a tool on a surface with painted, not yet applied weights keeps them (until now they jumped back).
- Auto-Skin: "Selected surfaces only" with "Mirror afterwards" mirrors only into the selected surfaces and leaves open paint lists of other surfaces open.
- Ctrl+Z during a weight stroke, gradient, gizmo drag or texture paint stroke ends it and takes it back; Ctrl+Y brings it back. Until now the held button kept writing without an undo step.
- Smaller fixes: a command that changes nothing keeps Redo; a stroke that starts beside the figure paints with the chosen mode; locked bones are unlocked for a newly loaded model; a typed brush radius of 0 in Painting counts as 1.

## Deutsch

- Model-Tab, Weights: Speichern schreibt die Gewichte, die die Ansicht zeigt, auch solange Mallisten offen sind (nach dem Malen oder einem Gewichtsbefehl, vor „Übernehmen“). Die Datei bekommt höchstens vier Gewichte je Ecke, normiert, wie das Spiel sie liest; die Mallisten bleiben offen. Die Prüfung vor dem Speichern und die JKA-Grenzen messen dasselbe. Bisher schrieb Speichern die Gewichte von vor dem Malen und meldete „gespeichert“.
- Weights: Solange ein Strich, Verlauf, Gizmo-Zug oder Werkzeugkästchen offen ist, bleibt die Detailstufe stehen, auch wenn das Rad zoomt oder „Auto“ sie wechseln würde. Spiegeln beim Malen konnte bei einem Stufenwechsel im Strich falsche Ecken beschreiben oder abstürzen.
- Weights und Modellieren: Eine Auswahl von Ecken, Kanten oder Vielecken gehört zu ihrer Detailstufe; nach einem Stufenwechsel ist sie leer und kommt mit der Stufe zurück, solange deren Netz nicht geändert wurde. Bisher blieben die Nummern stehen, und Befehle änderten andere Ecken.
- Modellieren: Ein Werkzeug auf einer Fläche mit gemalten, noch nicht übernommenen Gewichten behält sie (bisher sprangen sie zurück).
- Auto-Skin: „Nur ausgewählte Flächen“ mit „Danach spiegeln“ spiegelt nur in die gewählten Flächen und lässt offene Mallisten anderer Flächen offen.
- Strg+Z während eines Gewichtsstrichs, Verlaufs, Gizmo-Zugs oder Strichs im Painting beendet ihn und nimmt ihn zurück; Strg+Y bringt ihn wieder. Bisher schrieb die gehaltene Taste ohne Rückgängig-Schritt weiter.
- Kleinere Korrekturen: Ein Befehl, der nichts ändert, lässt Wiederholen stehen; ein Strich, der neben der Figur beginnt, malt mit der gewählten Art; gesperrte Knochen sind bei einem neu geladenen Modell entsperrt; ein getippter Pinselradius 0 im Painting zählt als 1.

## 中文

- 模型选项卡, Weights: 即使绘制列表仍处于打开状态(绘制或执行权重命令之后、“应用”之前), 保存也会写入视图显示的权重。文件中每个顶点最多四个权重, 按游戏读取的方式归一化; 绘制列表保持打开。保存前的检查和 JKA 限制测量同样的内容。此前保存写入的是绘制前的权重, 却提示“已保存”。
- Weights: 当笔划、渐变、Gizmo 拖动或工具框处于打开状态时, 细节级别保持不变, 即使滚轮缩放或“自动”本应切换。此前笔划中切换级别时, 镜像绘制可能写到错误的顶点或导致崩溃。
- Weights 和建模: 顶点、边或多边形的选择属于其细节级别; 切换级别后选择为空, 只要该级别的网格未被更改, 回到该级别时选择会恢复。此前编号保持不变, 命令会修改其他顶点。
- 建模: 在带有已绘制但尚未应用权重的面上使用工具时, 这些权重会保留(此前会跳回)。
- 自动蒙皮: “仅所选表面”配合“之后镜像”时只镜像到所选的面, 其他面的绘制列表保持打开。
- 在权重笔划、渐变、Gizmo 拖动或纹理绘制笔划过程中按 Ctrl+Z 会结束并撤销它; Ctrl+Y 可恢复。此前按住的鼠标键会在没有撤销步骤的情况下继续写入。
- 较小的修正: 不做任何更改的命令会保留“重做”; 从人物旁边开始的笔划使用所选模式绘制; 新加载的模型中锁定的骨骼会解锁; 在 Painting 中输入的画笔半径 0 按 1 处理。

## 日本語

- モデルタブ、Weights: ペイントリストが開いたまま(ペイントやウェイトのコマンドの後、「適用」の前)でも、保存するとビューに表示されているウェイトが書き込まれます。ファイルには頂点ごとに最大 4 つのウェイトが、ゲームが読む形に正規化されて入り、ペイントリストは開いたままです。保存前のチェックと JKA の上限も同じものを測ります。これまでは保存するとペイント前のウェイトが書き込まれ、「保存しました」と表示されていました。
- Weights: ストローク、グラデーション、ギズモのドラッグ、ツールボックスが開いている間は、ホイールでズームしたり「自動」が切り替えようとしても詳細レベルは変わりません。これまではストローク中にレベルが変わると、ミラーペイントが別の頂点に書き込んだりクラッシュしたりすることがありました。
- Weights とモデリング: 頂点・エッジ・ポリゴンの選択はその詳細レベルに属します。レベルが変わると選択は空になり、そのレベルのメッシュが変更されていなければ戻ったときに元に戻ります。これまでは番号が残り、コマンドが別の頂点を変更していました。
- モデリング: ペイント済みで未適用のウェイトを持つサーフェスにツールを使っても、ウェイトは保たれます(これまでは元に戻っていました)。
- オートスキン: 「選択したサーフェスのみ」と「あとでミラー」では、選択したサーフェスにだけミラーし、ほかのサーフェスの開いているペイントリストはそのままです。
- ウェイトのストローク、グラデーション、ギズモのドラッグ、テクスチャペイントのストロークの最中に Ctrl+Z を押すと、それを終えて元に戻します。Ctrl+Y でやり直せます。これまでは押したままのボタンが元に戻すステップなしで書き込みを続けていました。
- 細かな修正: 何も変えないコマンドはやり直しを残します。人物の横から始めたストロークも選んだモードで塗ります。新しく読み込んだモデルではロックされたボーンが解除されます。Painting で入力したブラシ半径 0 は 1 として扱います。

## Español

- Pestaña Modelo, Weights: guardar escribe los pesos que muestra la vista, también mientras hay listas de pintura abiertas (tras pintar o un comando de pesos, antes de Aplicar). El archivo recibe como máximo cuatro pesos por vértice, normalizados como los lee el juego; las listas de pintura siguen abiertas. La comprobación antes de guardar y los límites de JKA miden lo mismo. Hasta ahora guardar escribía los pesos de antes de pintar y decía «guardado».
- Weights: mientras hay un trazo, degradado, arrastre del gizmo o cuadro de herramienta abierto, el nivel de detalle se mantiene, aunque la rueda haga zoom o «Auto» fuera a cambiarlo. El espejo al pintar podía escribir en vértices equivocados o cerrarse con un error cuando el nivel cambiaba durante un trazo.
- Weights y Modelado: una selección de vértices, aristas o polígonos pertenece a su nivel de detalle; tras un cambio de nivel está vacía y vuelve con ese nivel mientras su malla no haya cambiado. Hasta ahora los números se quedaban y los comandos cambiaban otros vértices.
- Modelado: una herramienta sobre una superficie con pesos pintados y aún no aplicados los conserva (hasta ahora volvían atrás).
- Auto-Skin: «Solo las superficies seleccionadas» con «Reflejar después» refleja solo en las superficies seleccionadas y deja abiertas las listas de pintura de las demás.
- Ctrl+Z durante un trazo de pesos, un degradado, un arrastre del gizmo o un trazo de Painting lo termina y lo deshace; Ctrl+Y lo rehace. Hasta ahora el botón pulsado seguía escribiendo sin paso de deshacer.
- Correcciones menores: un comando que no cambia nada conserva Rehacer; un trazo que empieza junto a la figura pinta con el modo elegido; los huesos bloqueados se desbloquean en un modelo recién cargado; un radio de pincel 0 escrito en Painting cuenta como 1.

## Čeština

- Karta Model, Weights: uložení zapíše váhy, které ukazuje pohled, i když jsou seznamy malování otevřené (po malování nebo příkazu vah, před Použít). Soubor dostane nejvýše čtyři váhy na vrchol, normalizované tak, jak je čte hra; seznamy malování zůstanou otevřené. Kontrola před uložením a limity JKA měří totéž. Dosud uložení zapsalo váhy z doby před malováním a hlásilo „uloženo“.
- Weights: dokud je otevřený tah, přechod, tažení gizma nebo okénko nástroje, úroveň detailu zůstává, i když kolečko přibližuje nebo by ji „Auto“ změnilo. Zrcadlení při malování mohlo při změně úrovně během tahu zapsat do špatných vrcholů nebo spadnout.
- Weights a modelování: výběr vrcholů, hran nebo mnohoúhelníků patří ke své úrovni detailu; po změně úrovně je prázdný a vrátí se s touto úrovní, dokud se její síť nezměnila. Dosud čísla zůstala a příkazy měnily jiné vrcholy.
- Modelování: nástroj na ploše s namalovanými, dosud nepoužitými váhami je zachová (dosud se vracely zpět).
- Auto-Skin: „Jen vybrané povrchy“ s „Potom zrcadlit“ zrcadlí jen do vybraných ploch a otevřené seznamy malování ostatních ploch nechá otevřené.
- Ctrl+Z během tahu vah, přechodu, tažení gizma nebo tahu v Painting ho ukončí a vrátí; Ctrl+Y ho obnoví. Dosud držené tlačítko psalo dál bez kroku zpět.
- Drobné opravy: příkaz, který nic nezmění, zachová Znovu; tah začínající vedle postavy maluje zvoleným režimem; uzamčené kosti jsou u nově načteného modelu odemčené; zadaný poloměr štětce 0 v Painting se počítá jako 1.
