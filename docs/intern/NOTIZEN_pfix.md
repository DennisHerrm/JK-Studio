# Notizen pfix: Funde der Code-Pruefung Modell-Block beheben

Stand 07.11.2026 abends (paralleles Fenster bis 23 Uhr), Worktree `Y:\fix`, Zweig `fix1`, Basis e6daa5ca8 (1d). Ein Agent,
Laufnamen `fx_*`, Bau `Y:/fix/build-fix` (`--parallel 4`, `/nodeReuse:false`), Laufskript `ab/lauf_fix.py`. kFassung bleibt rc610.
Grundlage: `plaene/codepruefung_modellblock.md` (H1-H4, M1-M6, N1-N10) und `plaene/codepruefung_modellblock_2.md` (H5, H6,
M7-M9, N11-N14).

## Tests

- ctest `modell_pfix` (neu, `tests/modell_pfix.cpp`): selbst erzeugte kaputte Dateien und Faelle der Bibliothek - M1, M2, M3, M4,
  M5, N2, N3, N9, H5, H1 (Dokument).
- ctest `ablauf` (`tests/ablauftest.cpp`, `spawnBlockiert`): N8 nullDriver.
- Selbsttest `BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=pfix` (H1, H2, M5, H6/H5, H3) und `pfixkarte` (H4),
  `gui/selbsttest_pfix.inc`.

## Ergebnisse je Fund

Erster Agent (07.11.): H1, H2, H4, H5, H6, M1-M5, N2, N3, N8, N9 rot gesehen und behoben, H3 ohne Rotlauf behoben, N4/N11/N12 klein.
Nachtest ZUS (08.11., Agent ZUS, Laeufe `zs_*` in `Y:\fix\laeufe`):

- Gruen-Lauf der WIP-Behebungen: ctest 232/232 mit `modell_pfix` und `ablauf` (`zs_ctest_fix`), pfixkarte 5/0 (H4), pfix 15/1 - nur
  M6 rot, Modus 1 2635/0 (`zs_fx_m1`).
- H3 rot nachgestellt: Behebung voruebergehend aus (Frage uebersprungen), Lauf `zs_fx_pfix_rot`: das Kreuz und "Neues Requisit"
  fragen nicht (3 Pruefungen + Folgeklicks rot); gruen in `zs_fx_pfix_rot2` und `zs_fx_pfix_gruen`.
- M6: der Selbsttest-Schritt war selbst falsch - er setzte nur die Netz-Auswahl, G nimmt aber die Ecken des Model-Tabs
  (`eckenWahl`), G startete nie ("rot" im ersten Lauf ohne Bewegung). Test berichtigt (6 Ecken, G laeuft), Behebung
  (`normalenNachUmformen`, Normalen der Ecken an Dreiecken, die ihre Richtung geaendert haben, neu aus den Dreiecksnormalen samt
  harter Kanten der Datei; starr bewegte behalten die Normale) voruebergehend aus: `zs_fx_pfix_rot2` Normale 0/-1/0 (rot); gruen
  `zs_fx_pfix_gruen`.
- M7: neuer Selbsttest-Schritt zieht den Regler IK-FK (Blender-Modus) mit der Maus ueber zehn Bilder (`reglerZiehen`), dann ein
  Rueckgaengig: rot `zs_fx_pfix_rot` (IK-Anteil 0,01 statt 1,00, oben lag noch ein Regler-Schritt); Behebung wie die Physik-Regler:
  erster Wert `aendern`, weitere `vorschau`, Ende bei `IsItemDeactivated` - fuer IK-FK, Rolle, Spitze und die Retarget-Regler
  (Reach Fuss/Hand, Beinlaenge); Strg+Z beendet ein laufendes Ziehen (`modellOffenesAbbrechen`). Gruen `zs_fx_pfix_rot2`/`_gruen`.
  Die Retarget-Regler haben keinen eigenen Rotlauf (Rollout-Unterknoten im Test nicht erreichbar), gleiche Ursache und Behebung.
- M8: Selbsttest ph zieht im Fenster "Figur animieren" Staerke und Bilder mit der Maus: rot `zs_fx_ph_rot` (Staerke 1,00 nach dem
  Ziehen auf 0,3) und `zs_fx_ph_rot2` (Bilder 50 -> 50); Behebung `gehalten()` (Wert ueber die Bilder des Ziehens, neu aus dem
  Dokument nur, solange das Element nicht aktiv ist) fuer Staerke, Kontaktabstand, Frequenz, Daempfung, Bilder. Gruen `zs_fx_ph_gruen`.
- N13: Generator `tools/modell/rig_vorlagen_gen.py` mit seinen Eingaben (`tools/modell/rig_vorlagen/`: 33 Kettenvorschlaege,
  `gla/uebersicht.json`) im Repo; erzeugt `src/modell/rig_vorlagen.inc` bytegleich (nur der Kopf nennt den neuen Ort).

## Offen

- M9 (vorn +Y gegen -Y; erst per Lauf klaeren, welche Richtung stimmt), N1 (negative Versaetze, Rotfall braucht eine erzeugte
  .glm), N5/N14 (doppelte Logik, ohne Fehler - Aufraeumen), N6 (Fehlertexte der Bibliothek ueber Str::), N7 (Posen-Datei mit
  sicherSchreiben - Absturz beim Schreiben nicht nachstellbar), N10 (Belegung der Karten-GLA ins Dokument): nicht bearbeitet
  (Kontext/Zeit des Agenten ZUS).

## Changelog

## English

- Model tab: Ctrl+Z while a tool box (Extrude, Inset ...), G/R/S, a slider or a drag is open cancels that tool first, and Esc no longer removes the step before it; a command started while a tool box is open applies the box first.
- Model tab: "Detach" or a primitive into a new part on a .glm no longer resets the model and loses all changes and undo.
- Model tab: closing the program or opening another model (file, archive browser, NPC browser, New prop, Edit in the Model tab) asks before unsaved changes of the model are discarded.
- Model tab: a second new prop sent "Into the map" under the same name gets its own file (prop_2 ...) instead of overwriting the first one.
- Model tab: an .md3 with empty surfaces can be saved again, and parts made with Detach are written to the right surface.
- Model tab: broken .md3, .gla and animation files from other tools are refused with a message instead of crashing (out-of-range offsets, parents, frame counts, key counts).
- Model tab: an .md3 surface with more than 2000 triangles is refused like the game does (R_LoadMD3), not written.
- Model tab: Reverse, Mirror at cursor, Interpolation, Tangents and Copy with an IK key in the Dope Sheet selection no longer crash.
- Retarget: undoing "Load source" also goes back to the previous source.
- Animate figure: quotation marks or line breaks in signal names are refused with a message; the figure's block in the script is no longer deleted when it cannot be written.
- Preview: an NPC_spawner with NPC_type nullDriver appears at once, without the telefrag check, as in the game.
- animation.cfg check: a frame range that ends far behind the GLA is reported even with huge numbers.
- Model tab: moving or scaling vertices (gizmo and G/S) recomputes the normals of the faces that changed their direction, like 3ds Max and Blender - before, the old normals were kept and saved.
- Model tab and Retarget: dragging the IK-FK, roll, tip, reach and leg-length sliders is one undo step instead of one per frame (the history was gone after half a second).
- Animate figure: sliders and drag fields (strength, contact distance, frequency, damping, frames) keep the value you set when you let go.

## Deutsch

- Model-Tab: Strg+Z bei offenem Werkzeugkästchen (Extrude, Inset ...), G/R/S, Regler oder Zug bricht zuerst dieses ab, und Esc nimmt nicht mehr den Schritt davor weg; ein Befehl bei offenem Kästchen übernimmt zuerst das Kästchen.
- Model-Tab: „Detach“ oder ein Grundkörper in einen neuen Teil setzt eine .glm nicht mehr zurück (alle Änderungen und Rückgängig blieben weg).
- Model-Tab: Beenden und das Öffnen eines anderen Modells (Datei, Archiv-Browser, NPC-Browser, Neues Requisit, Im Model-Tab bearbeiten) fragen, bevor ungespeicherte Änderungen am Modell verworfen werden.
- Model-Tab: ein zweites neues Requisit mit demselben Namen „In die Karte“ bekommt eine eigene Datei (prop_2 ...), statt das erste zu überschreiben.
- Model-Tab: eine .md3 mit leeren Flächen lässt sich wieder speichern, und per Detach angelegte Teile landen in der richtigen Fläche.
- Model-Tab: kaputte .md3-, .gla- und Animationsdateien anderer Werkzeuge werden mit einer Meldung abgelehnt statt abzustürzen (Versätze, Elternteile, Bildzahlen, Schlüsselzahlen außerhalb).
- Model-Tab: eine .md3-Fläche mit mehr als 2000 Dreiecken wird wie im Spiel (R_LoadMD3) abgelehnt statt geschrieben.
- Model-Tab: Umkehren, Spiegeln am Cursor, Interpolation, Tangenten und Kopieren mit einem IK-Schlüssel in der Wahl des Dope Sheets stürzen nicht mehr ab.
- Retarget: Rückgängig von „Quelle laden“ geht auch zur vorigen Quelle zurück.
- Figur animieren: Anführungszeichen oder Zeilenumbrüche in Signalnamen werden mit Meldung abgelehnt; der Block der Figur im Skript wird nicht mehr gelöscht, wenn er nicht zu schreiben ist.
- Vorschau: ein NPC_spawner mit NPC_type nullDriver erscheint sofort, ohne Telefrag-Prüfung, wie im Spiel.
- Prüfung der animation.cfg: ein Bereich, der weit hinter der GLA endet, wird auch bei riesigen Zahlen gemeldet.
- Model-Tab: Verschieben oder Skalieren von Ecken (Gizmo und G/S) rechnet die Normalen der Flächen neu, die ihre Richtung geändert haben, wie 3ds Max und Blender - vorher blieben die alten stehen und wurden gespeichert.
- Model-Tab und Retarget: das Ziehen der Regler IK-FK, Rolle, Spitze, Reach und Beinlänge ist ein Rückgängig-Schritt statt einer je Bild (der Verlauf war nach einer halben Sekunde weg).
- Figur animieren: Regler und Zugfelder (Stärke, Kontaktabstand, Frequenz, Dämpfung, Bilder) behalten beim Loslassen den eingestellten Wert.

## 中文

- Model 标签: 在工具框 (Extrude、Inset ...)、G/R/S、滑块或拖动打开时按 Ctrl+Z 会先取消该工具, Esc 不再删除之前的步骤; 工具框打开时执行其他命令会先应用工具框。
- Model 标签: 对 .glm 执行 "Detach" 或把基本体放入新部件不再重置模型 (此前所有更改和撤销都会丢失)。
- Model 标签: 退出程序或打开其他模型 (文件、归档浏览器、NPC 浏览器、新道具、在 Model 标签中编辑) 前, 会询问是否放弃模型的未保存更改。
- Model 标签: 同名的第二个新道具 "放入地图" 时获得自己的文件 (prop_2 ...), 不再覆盖第一个。
- Model 标签: 含空面的 .md3 可以再次保存, Detach 生成的部件写入正确的面。
- Model 标签: 其他工具生成的损坏的 .md3、.gla 和动画文件会给出提示并被拒绝, 不再崩溃 (偏移、父骨骼、帧数、关键帧数越界)。
- Model 标签: 与游戏 (R_LoadMD3) 一样, 拒绝写入超过 2000 个三角形的 .md3 面。
- Model 标签: Dope Sheet 选择中含 IK 关键帧时, 反转、在光标处镜像、插值、切线和复制不再崩溃。
- Retarget: 撤销 "加载源" 时也会回到之前的源。
- 角色动画: 信号名中的引号或换行会被拒绝并提示; 无法写入时, 脚本中该角色的块不再被删除。
- 预览: NPC_type 为 nullDriver 的 NPC_spawner 与游戏中一样立即出现, 不做 telefrag 检查。
- animation.cfg 检查: 远超出 GLA 的帧范围即使数字极大也会报告。
- Model 标签: 移动或缩放顶点 (Gizmo 和 G/S) 时, 方向改变的面的法线会重新计算, 与 3ds Max 和 Blender 相同 - 此前保留并保存旧法线。
- Model 标签和 Retarget: 拖动 IK-FK、滚动、脚尖、Reach 和腿长滑块只算一个撤销步骤, 而不是每帧一个 (此前半秒后历史就没了)。
- 角色动画: 滑块和拖动字段 (强度、接触距离、频率、阻尼、帧数) 松开时保留设定的值。

## 日本語

- Model タブ: ツールボックス (Extrude、Inset ...)、G/R/S、スライダー、ドラッグが開いている間の Ctrl+Z はまずそれを取り消し、Esc が直前のステップを消すことはなくなりました。ツールボックスを開いたまま別のコマンドを実行すると、先にボックスを適用します。
- Model タブ: .glm で「Detach」やプリミティブを新しいパーツに作っても、モデルがリセットされなくなりました (以前はすべての変更と元に戻すが失われました)。
- Model タブ: 終了時や別のモデルを開くとき (ファイル、アーカイブブラウザー、NPC ブラウザー、新しい小道具、Model タブで編集) に、未保存の変更を破棄するか確認します。
- Model タブ: 同じ名前の 2 つ目の新しい小道具を「マップへ」送ると、1 つ目を上書きせず専用のファイル (prop_2 ...) になります。
- Model タブ: 空のサーフェスを含む .md3 を再び保存でき、Detach で作ったパーツは正しいサーフェスに書き込まれます。
- Model タブ: 他のツールで作られた壊れた .md3、.gla、アニメーションファイルはクラッシュせず、メッセージ付きで拒否されます (範囲外のオフセット、親、フレーム数、キー数)。
- Model タブ: 2000 を超える三角形を持つ .md3 サーフェスは、ゲーム (R_LoadMD3) と同様に書き込まず拒否します。
- Model タブ: Dope Sheet の選択に IK キーがあっても、反転、カーソルでミラー、補間、接線、コピーでクラッシュしなくなりました。
- Retarget: 「ソースを読み込む」を元に戻すと、前のソースにも戻ります。
- フィギュアのアニメーション: シグナル名の引用符や改行はメッセージ付きで拒否されます。書き込めない場合でもスクリプト内のフィギュアのブロックは削除されなくなりました。
- プレビュー: NPC_type が nullDriver の NPC_spawner は、ゲームと同様に telefrag チェックなしですぐに現れます。
- animation.cfg のチェック: GLA を大きく超えるフレーム範囲は、巨大な数値でも報告されます。
- Model タブ: 頂点の移動・スケール (ギズモと G/S) で、向きが変わった面の法線を 3ds Max や Blender と同じように再計算します。以前は古い法線のまま保存されていました。
- Model タブと Retarget: IK-FK、ロール、つま先、Reach、脚の長さのスライダーのドラッグは、フレームごとではなく 1 つの元に戻すステップになります (以前は半秒で履歴が消えていました)。
- フィギュアのアニメーション: スライダーとドラッグ欄 (強さ、接触距離、周波数、減衰、フレーム数) は、離したときに設定した値を保ちます。

## Español

- Pestaña Model: Ctrl+Z con una caja de herramienta (Extrude, Inset ...), G/R/S, un deslizador o un arrastre abiertos cancela primero esa herramienta, y Esc ya no quita el paso anterior; un comando con la caja abierta aplica primero la caja.
- Pestaña Model: «Detach» o una primitiva en una parte nueva ya no reinicia un .glm (se perdían todos los cambios y el deshacer).
- Pestaña Model: al cerrar el programa o abrir otro modelo (archivo, explorador de archivos, explorador de NPC, accesorio nuevo, editar en la pestaña Model) se pregunta antes de descartar los cambios sin guardar del modelo.
- Pestaña Model: un segundo accesorio nuevo con el mismo nombre enviado «Al mapa» recibe su propio archivo (prop_2 ...) en lugar de sobrescribir el primero.
- Pestaña Model: un .md3 con superficies vacías se puede volver a guardar, y las partes creadas con Detach se escriben en la superficie correcta.
- Pestaña Model: los archivos .md3, .gla y de animación rotos de otras herramientas se rechazan con un mensaje en lugar de bloquear el programa (desplazamientos, padres, número de fotogramas o de claves fuera de rango).
- Pestaña Model: una superficie .md3 con más de 2000 triángulos se rechaza como en el juego (R_LoadMD3) en lugar de escribirse.
- Pestaña Model: Invertir, Reflejar en el cursor, Interpolación, Tangentes y Copiar con una clave IK en la selección del Dope Sheet ya no bloquean el programa.
- Retarget: deshacer «Cargar origen» vuelve también al origen anterior.
- Animar figura: las comillas o saltos de línea en los nombres de señal se rechazan con un mensaje; el bloque de la figura en el script ya no se borra cuando no se puede escribir.
- Vista previa: un NPC_spawner con NPC_type nullDriver aparece al instante, sin la comprobación de telefrag, como en el juego.
- Comprobación de animation.cfg: un rango de fotogramas que termina muy detrás de la GLA se notifica también con números enormes.
- Pestaña Model: mover o escalar vértices (gizmo y G/S) recalcula las normales de las caras que cambiaron de dirección, como 3ds Max y Blender; antes se conservaban y guardaban las antiguas.
- Pestaña Model y Retarget: arrastrar los deslizadores IK-FK, giro, punta, alcance y longitud de pierna es un solo paso de deshacer en vez de uno por fotograma (el historial desaparecía en medio segundo).
- Animar figura: los deslizadores y campos de arrastre (fuerza, distancia de contacto, frecuencia, amortiguación, fotogramas) conservan el valor ajustado al soltar.

## Čeština

- Karta Model: Ctrl+Z při otevřeném okénku nástroje (Extrude, Inset ...), G/R/S, posuvníku nebo tažení nejprve zruší tento nástroj a Esc už neodstraní předchozí krok; příkaz při otevřeném okénku nejprve okénko použije.
- Karta Model: „Detach“ nebo těleso do nové části už neresetuje .glm (všechny změny i zpět se ztratily).
- Karta Model: ukončení programu a otevření jiného modelu (soubor, prohlížeč archivů, prohlížeč NPC, nová rekvizita, upravit na kartě Model) se zeptá, než se neuložené změny modelu zahodí.
- Karta Model: druhá nová rekvizita se stejným názvem poslaná „Do mapy“ dostane vlastní soubor (prop_2 ...) místo přepsání první.
- Karta Model: .md3 s prázdnými plochami jde znovu uložit a části vytvořené pomocí Detach se zapíší do správné plochy.
- Karta Model: poškozené soubory .md3, .gla a animací z jiných nástrojů se odmítnou se zprávou místo pádu (posuny, rodiče, počty snímků a klíčů mimo rozsah).
- Karta Model: plocha .md3 s více než 2000 trojúhelníky se odmítne jako ve hře (R_LoadMD3) místo zápisu.
- Karta Model: Obrátit, Zrcadlit u kurzoru, Interpolace, Tečny a Kopírovat s klíčem IK ve výběru Dope Sheetu už nespadnou.
- Retarget: zpět u „Načíst zdroj“ se vrátí i k předchozímu zdroji.
- Animace postavy: uvozovky nebo zalomení řádku v názvech signálů se odmítnou se zprávou; blok postavy ve skriptu se už nesmaže, když ho nelze zapsat.
- Náhled: NPC_spawner s NPC_type nullDriver se objeví hned, bez kontroly telefragu, jako ve hře.
- Kontrola animation.cfg: rozsah snímků končící daleko za GLA se nahlásí i u obrovských čísel.
- Karta Model: posun nebo změna měřítka vrcholů (gizmo a G/S) přepočítá normály ploch, které změnily směr, jako 3ds Max a Blender - dříve zůstaly staré a uložily se.
- Karta Model a Retarget: tažení posuvníků IK-FK, rolování, špička, dosah a délka nohy je jeden krok zpět místo jednoho za snímek (historie byla po půl sekundě pryč).
- Animace postavy: posuvníky a tažná pole (síla, kontaktní vzdálenost, frekvence, tlumení, snímky) si po puštění ponechají nastavenou hodnotu.
