# Notizen FIX6: PRUEF7-Funde P50, P51, P52, P54 (08.11.2026, `Y:\fix2`, Zweig `fix6`, Basis 443b64d5b)

Build `Y:/fix2/build-fix2` (`--parallel 8 -- /nodeReuse:false`), Laeufe `f6_*` unter `Y:\fix2\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\f2\f6\`. Engine-Quellen: OpenJK `Y:\werkzeuge\sp_654alt\ojk` (mit vollem Verlauf, darin die Raven-Quellen),
Movie Duels `Y:\werkzeuge\sp_654alt\md9`.

## P51: largestScale der Detailstufe aus s.modelScale

- Befund PRUEF7 ("nicht gesetzte Achsen zaehlen im Spiel als 0") trifft fuer Jedi Academy und Movie Duels NICHT zu:
  NPC_ParseParms setzt vor dem Lesen alle drei Achsen auf 1 (NPC_stats.cpp:2041 OpenJK, :2386 MD9). Eine .npc mit nur
  `scaleX 50` hat dort largestScale 1 wie die Vorschau.
- Abweichungen gibt es trotzdem: Jedi Outcast (codeJK2/game/NPC_stats.cpp NPC_ParseParms) setzt keine Vorgabe, s.modelScale
  bleibt aus G_Spawn (memset, g_utils.cpp:831) 0 - nur `scaleX 50` ergibt 0,5 statt 1. In allen Spielen bleibt eine genannte 0
  eine 0 (nur n < 0 und n == 100 werden uebergangen): `scaleX 0`, `scaleY/Z 50` ergibt 0,5. Die Vorschau nahm den gezeichneten
  Massstab (0 und nicht gesetzt = 1).
- Behebung: `NpcDef::skalaSpiel` (s.modelScale; Vorgabe 1, unter Jedi Outcast 0), `npcGroessteSkala` fuer `figurNetzImBild`;
  der gezeichnete Massstab `NpcDef::skala` bleibt (CG_Player skaliert nur Achsen ungleich 0, cg_players.cpp:5671-5690 JK2).
- ctest `pruefung`: rot 3 FEHL (JKA scaleX 0 + Y/Z 50, JK2 nur scaleX 50, JK2 ohne scale) -> gruen.

## P50: Glanz/Umgebung im Ersatzweg ohne entSkala

- Ohne `FigurLicht::entSkala` sind gAchse0..2 die normierten Spalten 0, 1, 2 von welt, die Massstaebe kamen aber als
  `achsLaenge.yxz`. Heute latent (alle Aufrufer ohne entSkala haben gleichen Massstab je Achse).
- Behebung: `achsLaenge` in derselben Reihenfolge (src/gpushader.cpp).
- Neuer ctest `figurglanz`: der echte Figur-Vertex-Shader (vs_4_0, Fahnen 0 wie gpumap_win32.cpp) auf WARP ueber Stream-Output,
  Massstab (2, 1, 1) mit gedrehten Achsen, tcGen environment mit und ohne entSkala: rot 0 von 5 Ecken gleich -> gruen 5 von 5
  bitgleich. `tests/vsstrom.h` ist bytegleich aus dem Zweig fix2 uebernommen (FIX5), damit der Merge kein Konflikt wird; der
  CMake-Eintrag steht absichtlich vor dem hlsl-Block (fix2 haengt seine Proben dahinter an).

## P52: Detailstufen ohne Kopie des Stufe-0-Netzes

- `glmOhneNetz` (glm.h/.cpp): eine Huelle von Stufe 0 ohne verts/indexes/grobVerts/grobIndexes (die Netze werden kurz
  verschoben, nicht kopiert); jede Stufe kopiert die Huelle und nimmt ihr eigenes Netz. Alle uebrigen Felder bleiben aus Stufe 0
  wie vorher (auch mins/maxs).
- Gleiches Bild: p27g-Vorschaufoto (Selbsttest geschuetze, 1920 x 1440, Skript `p27g.sh`) vorher/nachher gleich bis auf die
  fps-Anzeige der Statuszeile (unten rechts, 20 x 15 Bildpunkte). Zwei Laeufe desselben Baus unterscheiden sich dort ebenso
  (`bmpdiff.py` nimmt das Feld aus).
- Speicher (vorlaeufige Messzeile, nicht committet: Bytes der Netze aller Detailstufen nach jedem Laden): p27g 584680 -> 375532
  Bytes (-36 %); mission1 (1222 Ladungen, 1212 mit gleichen Assets/Stufen vergleichbar): groesste Ladung 5263988 -> 3443516
  Bytes (15 Assets, 11 Stufen), Summe ueber alle Ladungen -37,5 %. mission1 vorher und nachher 98/0.
- Ladezeit (".glm" der Zeile "Zeit:" im Protokoll, 51 Ladungen mit Modellen in mission1; vorheriger und neuer Bau gleichzeitig
  gestartet, damit beide dieselbe Last sehen): Summe 1757 -> 1742 ms, 18 Ladungen schneller, 12 gleich, 21 langsamer - im
  Rauschen der Millisekunden-Aufloesung, kein messbarer Unterschied (die Stufen werden weiter einzeln aus der Datei gelesen).
  Keine Bildzeit gemessen (laut Auftrag).
- ctest `modell`: glmOhneNetz behaelt Kopf und Flaechenzustand, laesst die Netze weg, das Original bleibt unveraendert.

## P54: Bilddatei-Reihenfolge unter Original-JKA/Jedi Outcast

- Belegt aus den Raven-Quellen im OpenJK-Verlauf:
  - Jedi Outcast 1.04 (a09d4827, codeJK2/renderer/tr_image.cpp R_LoadImage): nur `.jpg`, dann `.tga` - kein png, die genannte
    Endung nicht zuerst. COM_StripExtension (codeJK2/game/q_shared.cpp:43) schneidet am ERSTEN Punkt ab (auch in einem
    Ordnernamen), COM_DefaultExtension haengt danach immer an.
  - Original-JKA (14cea156, code/renderer/tr_image.cpp:2016-2103 SP, codemp/renderer/tr_image.cpp:2228-2252 MP): `.jpg`,
    `.png`, `.tga` - die genannte Endung nie zuerst, ebenfalls am ersten Punkt abgeschnitten.
  - OpenJKs JK2-Ziel (rd-vanilla mit JK2_MODE) nimmt die OpenJK-Folge (rd-common/tr_image_load.cpp hat keinen JK2-Zweig).
- Umgesetzt fuer das Regelwerk Jedi Outcast (Vorbild ist dort jk2sp.exe 1.04, eine OpenJK-JO-exe gibt es nicht):
  `textureCandidates` liefert jpg, tga am Namen bis zum ersten Punkt.
- Original-JKA: kein eigenes Regelwerk (OpenJK steht fuer Jedi Academy) - nicht umgesetzt, siehe Offen.
- ctest `bilder`: rot 4 FEHL -> gruen; OpenJK und Movie Duels unveraendert (y.tga: tga, jpg, png).

## Schluss

- Rot (Bau mit den neuen Pruefzeilen, Code wie die Basis): ctest figurglanz 1 FEHL, pruefung 3 FEHL, bilder 4 FEHL. Gruen nach
  den Behebungen: pruefung, bilder, figurglanz, modell, gpushader, hlsl 6/6.
- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) 0 Warnungen, 20 min; danach parallel: ctest 241/241,
  figurlicht 8/0, modellblock funktionen 61/0, Modus 1 2635/0 (die Zeile "Klang abspielen ... s3_kick.mp3 FEHLER: nicht
  gefunden" steht ebenso im MERGE2-Lauf, keine Pruefzeile). mission1 98/0 vorher und nachher (Bau mit der vorlaeufigen Messzeile,
  sonst gleicher Quelltext).
- Lints wie die Basis: portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, alle anderen rc 0.
- Handbuch: keine sichtbare Aenderung der Oberflaeche, nichts nachzuziehen. Keine Bildzeit-Messung (laut Auftrag).

## Offen

- Original-JKA (jasp.exe/jamp.exe) laedt Bilder in anderer Folge als OpenJK (jpg, png, tga; genannte Endung nie zuerst; erster
  Punkt). Ohne Regelwerk "Original-JKA" bleibt die Vorschau bei OpenJK - Entscheidung fuer shank, ob es eins geben soll.
- Jedi Outcast MP (jk2mp.exe): R_LoadImage nicht in den Quellen (OpenJK hat nur codeJK2 SP) - die Regel gilt fuer das Regelwerk
  insgesamt.
- JK2 NPC_BuildRandom (codeJK2 NPC_stats.cpp:1054) setzt s.modelScale zufaellig 0,87..1,02 - nicht nachgebildet (Zufall).
- Ein Wechsel des Regelwerks waehrend der Sitzung: Bildzwischenspeicher, die nach dem Shadernamen schluesseln, halten die
  frueher gefundene Datei bis zum naechsten Neuladen (nicht geprueft, Altbestand).

## Changelog

## English

- Mission preview: the detail level of a character uses the game's largest model scale - under Jedi Outcast unset scale axes of a .npc count as 0, and a scale of 0 stays 0 as in the game.
- Preview: shine and chrome of models with an unequal scale per axis are computed with the right axis when the caller gives no entity scale.
- Mission preview: detail levels of characters no longer keep a copy of the full model - less memory, same picture.
- Preview under Jedi Outcast: texture files are looked up like jk2sp.exe (jpg, then tga, no png).

## Deutsch

- Mission-Vorschau: Die Detailstufe einer Figur nimmt den größten Modellmaßstab des Spiels - unter Jedi Outcast zählen nicht gesetzte Achsen einer .npc als 0, und ein Maßstab 0 bleibt wie im Spiel 0.
- Vorschau: Glanz und Chrom von Modellen mit ungleichem Maßstab je Achse werden mit der richtigen Achse gerechnet, wenn der Aufrufer keinen Entity-Maßstab mitgibt.
- Mission-Vorschau: Die Detailstufen der Figuren behalten keine Kopie des ganzen Modells mehr - weniger Speicher, gleiches Bild.
- Vorschau unter Jedi Outcast: Bilddateien werden wie in jk2sp.exe gesucht (jpg, dann tga, kein png).

## 中文

- 任务预览: 角色的细节级别使用游戏中最大的模型缩放 - 在 Jedi Outcast 下 .npc 中未设置的缩放轴按 0 计算, 缩放为 0 时与游戏一样保持 0。
- 预览: 当调用方未提供实体缩放时, 各轴缩放不同的模型的高光和镀铬按正确的轴计算。
- 任务预览: 角色的细节级别不再保留整个模型的副本 - 内存更少, 画面相同。
- Jedi Outcast 下的预览: 纹理文件按 jk2sp.exe 的方式查找 (先 jpg, 再 tga, 不用 png)。

## 日本語

- ミッションプレビュー: キャラクターの詳細レベルはゲームと同じ最大のモデルスケールを使います。Jedi Outcast では .npc で設定されていないスケール軸を 0 として数え、スケール 0 はゲームと同じく 0 のままです。
- プレビュー: 呼び出し側がエンティティのスケールを渡さない場合でも、軸ごとにスケールが異なるモデルの光沢とクロームを正しい軸で計算します。
- ミッションプレビュー: キャラクターの詳細レベルはモデル全体のコピーを保持しなくなりました。メモリが減り、画像は同じです。
- Jedi Outcast でのプレビュー: テクスチャファイルを jk2sp.exe と同じく探します (jpg、次に tga、png なし)。

## Español

- Vista previa de misión: el nivel de detalle de un personaje usa la mayor escala de modelo del juego; en Jedi Outcast los ejes de escala no indicados en un .npc cuentan como 0, y una escala 0 sigue siendo 0 como en el juego.
- Vista previa: el brillo y el cromo de los modelos con escala distinta por eje se calculan con el eje correcto cuando quien llama no da la escala de la entidad.
- Vista previa de misión: los niveles de detalle de los personajes ya no guardan una copia del modelo entero; menos memoria, misma imagen.
- Vista previa en Jedi Outcast: los archivos de textura se buscan como en jk2sp.exe (jpg, luego tga, sin png).

## Čeština

- Náhled mise: úroveň detailu postavy používá největší měřítko modelu jako hra - v Jedi Outcast se nenastavené osy měřítka v .npc počítají jako 0 a měřítko 0 zůstává jako ve hře 0.
- Náhled: lesk a chrom modelů s různým měřítkem v jednotlivých osách se počítají se správnou osou, i když volající nepředá měřítko entity.
- Náhled mise: úrovně detailu postav už neuchovávají kopii celého modelu - méně paměti, stejný obraz.
- Náhled v Jedi Outcast: soubory textur se hledají jako v jk2sp.exe (jpg, pak tga, žádné png).
