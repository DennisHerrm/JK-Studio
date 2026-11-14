# Notizen CW-PRUEF: Pruefungen der Clone-Wars-Kette im Optimierer, JSON-Bericht (13.11.2026, `Y:\p_gd`, Zweig `p_cwpruef`, Basis karte1 1f397932c)

Auftrag: `JKStudio_Uebergabe\plaene\clonewars_kette.md`, Teil CW-PRUEF (nach rc612). Build `Y:/p_gd/build` (`--parallel 2 -- /nodeReuse:false`,
Leerlauf), Laeufe `cw_*` unter `Y:\p_gd\build\laeufe`, Skripte und Protokolle `Y:\werkzeuge\cwpruef\`. Quellen: OpenJK
`Y:\werkzeuge\sp_af9c\m3\ojq\src`, Movie Duels `Y:\werkzeuge\sp_654alt\md9`, q3map2 im Repo (`third_party/q3map2`, NetRadiant-Custom 20260114).

## Was die Engine vorgibt (Stellen)

| Mass | Wert | Stelle |
|---|---|---|
| Spielfigur x/y | -16 ... 16 (SP, Movie Duels, JK2), -15 ... 15 (MP) | OpenJK code/game/g_client.cpp:55-58, Movie Duels code/game/g_client.cpp:94-96, codeJK2/game/g_client.cpp:40, codemp/game/g_client.c:31 |
| Spielfigur z | -24 ... 40, geduckt Oberkante 16 | code/game/bg_public.h:51-53 (MD9 :76-78) |
| STEPSIZE | 18 | code/game/bg_public.h:71 (MD9 :93, MP bg_public.h:51) |
| begehbar | Normale z >= 0,7 | bg_public.h:69 MIN_WALK_NORMAL |
| Sprunghoehe ohne Macht | 225^2 / (2 * 800) = 31,6 | bg_public.h:70 JUMP_VELOCITY, g_main.cpp:628 g_gravity 800 |
| Kollision der Spielfigur | SOLID, PLAYERCLIP, BODY, TERRAIN | bg_public.h:716 MASK_PLAYERSOLID |
| Bildseiten | Zweierpotenz, sonst ERR_FATAL | rd-vanilla/tr_image.cpp:1002-1004 R_CreateImage (MD9 :1049), R_LoadImage skaliert nicht (rd-common/tr_image_load.cpp:94-145) |
| Teilflaeche eines Laufzeit-Modells | > 1000 Ecken bzw. 6000 Indizes: ERR_DROP | tr_model.cpp:899-906 R_LoadMD3, tr_surface.cpp:53-69 RB_CheckOverflow, qfiles.h:33-34 |
| MD3_MAX_VERTS / _SURFACES / _TRIANGLES | 4096 / 32 + 32 / 8192 | qfiles.h:101-105 - nirgends geprueft (R_LoadMD3 nicht, q3map2 nur Warnung ueber assimp MD3Loader.cpp:405-419) |

Abweichungen von der Liste: die Liste nennt x/y -15 ... 15 - das ist Mehrspieler; Einzelspieler und Movie Duels haben 16. MD3_MAX_VERTS
und MD3_MAX_SURFACES setzt keine Engine durch (src/md3.cpp nennt deka_dispenser.MD3 mit 300 Flaechen, das im Spiel laedt) - die
Pruefung ist deshalb nur ein Hinweis.

SHADER_MAX_VERTEXES je Regelwerk (Entscheidung Hauptchat 14.11.): die Grenzen-Tabelle (src/karte/grenzen.cpp) nimmt den Wert des
gewaehlten Regelwerks - Movie Duels 4000 / SHADER_MAX_INDEXES 24000 (Update 9: code/qcommon/qfiles.h:33-34 "raised from 1000 (cloth /
dense robes)", Pruefstellen code/rd-vanilla/tr_bsp.cpp:541 ParseTriSurf, tr_surface.cpp:60 RB_CheckOverflow), Jedi Academy SP 1000
(OpenJK code/qcommon/qfiles.h:33), MP 1000 (codemp/qcommon/qfiles.h:34), Jedi Outcast 1000 (code/qcommon/qfiles.h:33 mit JK2_MODE).
Die Pruefung "modellteilflaeche" liest die Tabelle. Dafuer liest die Tabelle fuer Movie Duels jetzt die Quelle von Update 9 statt der
alten Kopie mdgit (Gedaechtnis "MD-Quelle Update 9"): ausser den beiden Werten waren nur Zeilen verschoben (25 Stellen: tr_local.h,
cm_load.cpp, q_shared.h, cg_main.cpp, g_local.h, g_utils.cpp, NPC_stats.cpp, cl_cgame.cpp - neu belegt), tools/handbuch/grenzen.json
neu erzeugt. Rot: ctest karte_cwpruef "Movie Duels 4000" und "1500 Ecken laedt Movie Duels, Jedi Academy bricht ab" (2 Fehlschlaege),
kartegrenzen gegen md9 27 Fehlschlaege; gruen: beide 0. **Wichtig fuer andere Bauten:** ctest karte_grenzen braucht
`-DBEHAVED_MD_QUELLEN=Y:/werkzeuge/sp_654alt/md9` (bisher Y:/werkzeuge/sp_af9c/mdgit - gegen mdgit wird es rot).

"misc_model in Weltkoordinaten mit origin 0 0 0, das q3map2 in einer dichten Karte verwirft": q3map2 verwirft das Modell nicht -
InsertModel backt die Dreiecke an Ursprung + Ecke ein (model.cpp:1581), AddReferenceToLeaf laesst nur Dreiecke im Festen weg. Was
wirklich geschieht: FloodEntities (portals.cpp:536-596) setzt JEDE Punkt-Entity an ihrem Ursprung + 1 ins BSP - liegt er im Leeren,
leckt die Karte (keine .prt, kein VIS), im Festen kommt "Entity in solid". Laufzeit-Modelle (misc_model_static) blendet das Spiel nach
dem Ursprung aus (Movie Duels cg_main.cpp:3263-3290: PVS von origin + zoffset + 1, Abstand 8192 vom Ursprung). Nachgestellt mit
bhed_q3map2 (`weltkoord.py`): dichter Raum, misc_model mit Ursprung 4096 ausserhalb: "entity reached from outside -- leak detected", .lin geschrieben, keine
.prt (kein VIS), das Modell trotzdem eingebacken (13 statt 6 Zeichenflaechen); Ursprung 8 unter dem Boden: zweimal "Entity in solid",
.prt da, Modell eingebacken; Ursprung im Raum: alles normal. Die Pruefung "modellweltkoord" meldet deshalb ein eingebackenes misc_model
nur, wenn sein Ursprung ausserhalb aller Pinsel der Welt liegt (dann leckt die Karte), Laufzeit-Modelle immer, wenn die Geometrie
weiter vom Ursprung entfernt ist, als das Modell gross ist.

## Die Pruefungen (src/karte/optimierer_cw.cpp)

Kollision wie im Spiel: der Inhalts-Shader jedes Pinsels nach SetBrushContents (map.cpp:455-503: Fluessigkeit vor Nebel vor playerclip
vor nicht fest, sonst die erste Seite; writebsp.cpp:376 schreibt seine Inhalte), die Inhalte nach der surfaceparm-Tafel (games.cpp
787-848 game_sof2, ApplySurfaceParm shaders.cpp:230-245), Patches mit festem Shader, Dreiecke eines misc_model mit Clip-Spawnflag
(clipflags_doClip, model.cpp:970-999). Sichtbar = gezeichnet und nicht durchsichtig (blendFunc/alphaFunc, qer_trans, Fluessigkeit,
Nebel; "nonsolid" allein nicht - neues Feld ShaderArt::sichtDeckend).

| Kennung | Schwere | Was | Massstab |
|---|---|---|---|
| laufohnekollision | Warnung | sichtbare, begehbare, undurchsichtige Flaeche (Pinsel, Patch, Modell), unter der innerhalb der Groesse der Spielfigur (64) keine Kollision liegt | Spielfigur 64, MIN_WALK_NORMAL |
| kollisionsabstand | Warnung | sichtbarer Boden und Kollision darunter bzw. unsichtbare Kollision darauf mehr als die Toleranz auseinander (bis 64); eine schraege Clip-Rampe hoechstens STEPSIZE ueber Treppenstufen zaehlt nicht (die Liste verlangt sie) | Toleranz 1 |
| unsichtbarekollision | Warnung | nicht gezeichnete Kollisionsflaeche (clip, caulk, nodraw), an die die Spielfigur kommt, ohne sichtbare Flaeche: Boden (nichts Sichtbares +-18), Wand (nichts Sichtbares +-16) | STEPSIZE, halbe Breite |
| fastdeckungsgleich | Warnung | gezeichnete Flaechen zweier Pinsel, gleiche Richtung, Abstand unter 2, Ueberdeckung > 1 Einheit^2, verschiedene Shader; ohne die, die q3map2 entfernt (CullSides: gleiche Wicklung; SideInBrush: ganz im anderen Pinsel; die BSP schneidet in Strukturpinseln weg) und ohne polygonOffset | Abstand 2 |
| stufehoch | Warnung | Stufe zwischen zwei begehbaren Flaechen, hoeher als 18 und hoechstens 31,6 (darueber eine Wand), oben und unten Platz zum Stehen; die obere Flaeche gehoert zu einer zu Fuss verbundenen, erreichten Flaeche von mindestens 96 x 96 (Kasten der Standplaetze; Kisten und Pulte sind Hindernisse) | STEPSIZE, Sprunghoehe, Flaeche 96 |
| tuerklein | Warnung | func_door (je Team zusammen, ohne Ursprungspinsel) niedriger oder schmaler als die Tuer | 110 x 64 |
| durchgangeng | Warnung | Stelle auf begehbarem Boden, niedriger oder schmaler als eine Tuer, die Spielfigur passt noch (geduckt 40, 32 breit); Mover offen | 110 x 64 |
| gangeng | Hinweis | dasselbe in einem Gang (eine Gangbreite weit in beide Richtungen eng) unter dem Gangmass | 128 x 96 |
| bildnpot | Fehler | Bildseite keine Zweierpotenz (Stufen map/clampMap/animMap, skyParms, Bild ohne Skript) | ERR_FATAL |
| bildfehlt | Warnung | Bild eines Shaders der Karte fehlt, Shader einer Modellflaeche ohne Skript und Bild | - |
| bildgroesse | Hinweis | Bildseite unter 64 oder ueber 2048 | 64 ... 2048 |
| modellteilflaeche | Fehler | zur Laufzeit geladenes .md3 mit einer Flaeche ueber der Grenze renderer.flaechenpunkte/-indizes | 999 / 5999 |
| modellweltkoord | Warnung | Geometrie weiter vom Ursprung als das Modell gross (siehe oben) | - |
| md3grenzen | Hinweis | .md3 ueber 64 Flaechen / 4096 Ecken / 8192 Dreiecken | qfiles.h |
| requisitschwebt | Warnung | Modell beruehrt innerhalb der Toleranz weder Kollision noch ein anderes Modell | Toleranz 1 |
| requisitsteckt | Hinweis | unter der Modellmitte liegt der erste Boden mehr als die Toleranz ueber der Unterkante (Mitte in der Kollision oder eigener Clip: ausgenommen) | Toleranz 1 |

Erreichbarkeit (nach den ersten Laeufen auf Originalkarten noetig): Standplaetze im Raster 32 auf jeder begehbaren Kollisionsflaeche mit
Platz fuer die geduckte Spielfigur, geflutet von info_player_*, NPC_*, misc_teleporter_dest, target_position, item_/weapon_/ammo_ ueber
Nachbarn bis 48 waagerecht, hinauf bis 31,6, hinab beliebig, der Weg auf halber Hoehe frei, Tueren und andere Mover offen. Die Sicht-,
Stufen- und Masspruefungen zaehlen nur an erreichten Stellen. Ohne eine dieser Entities (Testkarten) entscheidet die .bsp (Cluster);
unsichtbarekollision braucht dann einen Bau. Nicht verfolgt: Aufzuege (func_plat), Machtspruenge, Skript-Teleports - dort entgehen
Befunde (keine Falsch-Treffer).

## Bericht und Oberflaeche

- JSON (`optimiererBerichtJson`, Knopf "JSON..." mit Tooltip, Rechtsklick auf einen Befund): Format `jkstudio-optimizer-report`
  Version 1, Grenzen, alle Befunde mit allen Objekten und `position` [x, y, z] (der Messpunkt; ohne: Mitte der Flaeche bzw. des
  Primitivs, Ursprung der Entity), Shader und Bilder der Karte mit Groesse. Englisch, damit andere Werkzeuge es lesen.
- Textbericht: Abschnitt "Shader und Bilder".
- Einstellungen (Seite Optimizer, mit Tooltips): OptimizerTolerance 1, OptimizerCoplanar 2, OptimizerDoorHeight/-Width 110/64,
  OptimizerCorridorHeight/-Width 128/96, OptimizerStepArea 96, OptimizerImageMin/-Max 64/2048 - als `karte=`-Zeilen der Einstellungsdatei wie die anderen
  Optimizer-Schluessel (sie laufen ueber den Leser "karte" der REST612-Tabelle in src/settings.cpp; eine eigene Zeile dort waere ein
  zweiter Weg fuer dieselben Werte).
- Der Karten-Editor hat keine Aktionssuche (die gibt es nur im Model-Tab); auffindbar ueber Build > Optimizer..., den Knopf mit
  Tooltip, den Rechtsklick und das Handbuch.

## Tests

- Bau ohne Warnungen (bau1 ... bau5, /W4 /WX), nach dem Rebase auf karte1 348f0e345 bau4/bau5 0 Warnungen; voller Bau bau_alles: bau_alles und bau6 (alle Ziele) 0 Fehler, 0 Warnungen.
- ctest `karte_cwpruef` (neu, tests/kartecwpruef.cpp): 57 Pruefungen, 0 Fehlschlaege - je Pruefung eine Karte, auf der sie anschlaegt,
  und eine, auf der sie schweigt; dazu surfaceparm-Inhalte, SetBrushContents (eine trigger-Seite macht den Pinsel durchlaessig, eine
  clip-Seite fest), Spielfigur je Regelwerk, Bildkoepfe tga/png/jpg, Clip-Rampe ueber einer Treppe, einstellbare Masse.
  `karte_optimierer` 273/0 (Vorlagen 54). ctest gesamt: 274 Tests: erst 3 rot - karte_lizenz (Kopf von optimierer_cw.cpp ohne "Portiert aus": ergaenzt), hilfe_orte (src/hilfe_orte.inc veraltet, weil die Einstellungsseite Optimizer neue Felder hat: tools/gen_hilfe_orte.py), q3map2_baupause (Licht-Cache "3 of 5 passes kept" unter Last neben dem Selbsttest; allein gruen, Code von CW-PRUEF nicht beteiligt) - danach alle drei gruen; Lints i18n, i18n_erzeugt, menue, settings, headers, sources, feldinit, cjkumbruch, doppelanzeige, abstand, layout, gapp, imgui_context, tabellenid ok; portability 97 (kartecwpruef.cpp: <cstddef> ergaenzt, keine Beanstandung aus CW-PRUEF), pruefcodes 11 wie vorher.
- Selbsttest karteneditor BHED_KARTE_NUR=cwpruef: cw_sel3 23 OK, 0 FEHLER (vorher cw_sel1 19/6 und cw_sel2 22/1 - die Fehler waren:
  Stufen nur von der unteren Kante gesucht (eine Stufe mitten auf grosser Bodenflaeche fehlte), sichtDeckend nach dem Namen
  "window" statt nach dem Skript, unerreichbare Platte, zu grosses Modell, Bau-Menue nicht geladen, Ort der Stufe). Sauberer Raum:
  keine der 16 Pruefungen; Fehlerraum: jede; Klick waehlt das Objekt; JSON gueltig (eigener Leser) mit Ort der Stufe; Gangbreite aus
  den Einstellungen; nach dem Bau mit .bsp.
- Gruppen des Optimierers: karteneditor BHED_KARTE_GRUPPEN=stufe3,stufe4,stufe5,rest8,cwpruef (Lauf cw_gruppen): 625 OK, 0 FEHLER; Handbuchbilder BHED_HB_NUR=karte5 in en, de, zh-Hans, ja, es, cs je 2 OK, 0 FEHLER (Bilder angesehen: Fenster mit Knopf JSON..., Einstellungsseite mit den neuen Feldern, nichts abgeschnitten); nicht gelaufen: der volle karteneditor-Lauf und Modus 1 (laufen auf dem zusammengefuehrten Stand).

## Originalkarten (nur gelesen)

Selbsttest mit BHED_CWPRUEF_KARTEN (File > Open .bsp mit Rueckgewinnung der misc_model, Optimierer mit der Original-.bsp, Regelwerk
Auto = Movie Duels; Berichte `Y:\p_gd\build\laeufe\cw_orig3\cw_<karte>.txt/.json`). Zahlen erst ohne Erreichbarkeit (cw_orig1), dann
mit (cw_orig3):

| Befund | t1_sour | duel1 | kor1 |
|---|---|---|---|
| laufohnekollision | 83 -> 9 | 1 -> 0 | 8 -> 4 |
| unsichtbarekollision | 1107 -> 427 | 132 -> 31 | 4612 -> 1215 |
| kollisionsabstand | 350 -> 119 | 40 -> 0 | 449 -> 90 |
| fastdeckungsgleich | 483 -> 273 | 0 -> 0 | 33 -> 18 |
| stufehoch | 22 -> 27 -> 4 | 5 -> 4 -> 4 | 57 -> 87 -> 56 |
| tuerklein | 9 | 0 | 3 |
| durchgangeng / gangeng | 373 / 39 -> 189 / 27 | 14 / 3 -> 3 / 1 | 935 / 181 -> 558 / 116 |
| requisitschwebt / -steckt | 3 / 296 -> 3 / 9 | 0 / 10 -> 0 / 1 | 0 / 293 -> 0 / 0 |
| bildgroesse | 4 | 2 | 1 |

Geaendert nach den ersten Laeufen (jeweils am Befund belegt): Erreichbarkeit statt Cluster (Daecher, Raum hinter Zaeunen und Inneres des
Himmelskastens liegen in Clustern), requisitsteckt von der Modellmitte (in Waende eingelassene Tafeln galten als "im Boden"), gleiche
Shader bei fastdeckungsgleich ausgenommen (gleiches Bild, kein sichtbares Flackern), Clip-Rampen ueber Treppen (duel1: Stufen 8 hoch
unter einer Rampe, 4 Einheiten "Abstand"). stufehoch steigt, weil die Suche jetzt von der oberen Kante ausgeht (findet auch Stufen auf
grossen Bodenflaechen). Danach die Mindestflaeche 96 x 96 (Entscheidung 14.11., Lauf cw_orig4): t1_sour 27 -> 4 (weg: Steuerpulte
kej_control, Kisten, Tuerschwellen, physics_clip-Kanten; bleiben: drei Podeste concrete_floor_dark 24 hoch, eine Clip-Kante 28), duel1
4 -> 4 (die Balkone n_floor02, 28 hoch, grosse Flaeche - echte Stufen), kor1 87 -> 56 (weg: 30 einzelne Saeulenfuesse os_basic_pillar
u. a.; bleiben: Fuesse und Stufen, die zu Fuss mit groesseren Flaechen verbunden sind). Testkarte: Kiste 48 x 48 x 24 schweigt, ohne
Mindestflaeche (0) meldet sie (so war es vorher), Podest 160 x 160 x 24 meldet, mit Mindestflaeche 192 schweigt es.

Falsch-Treffer bzw. Treffer, die bei Raven Absicht sind (die Liste ist strenger als die Originalkarten):
- unsichtbarekollision: physics_clip/clip zum Glaetten und zum Absperren (t1_sour 328 + 56 von 427), caulk/clip als Kollision von
  Felsen aus Patches und Modellen (kor1: 850 caulk, 256 physics_clip) - die Felsflaeche liegt dort mehr als 16 vom Clip entfernt.
- durchgangeng/gangeng: Engstellen zwischen Kisten, Saeulen und Gelaendern, Dachgesimse (t1_sour z 480), Felsspalten (kor1) - echt
  enger als 110 x 64 bzw. 128 x 96, aber keine Tueren oder Gaenge im Sinn der Liste.
- stufehoch: Saeulenfuesse in kor1, die zu Fuss an groessere Flaechen grenzen (23); Kisten und Pulte faellt die Mindestflaeche heraus.
- tuerklein: die Tueren von Jedi Academy sind 96 hoch (func_door 96 x 64) - nach der Liste zu niedrig, im Spiel Standard.
- laufohnekollision: Oberseiten von misc_model_static ohne Kollision in Reichweite (wall_light, switch_locked) - man geht hindurch, steht
  aber nicht darauf.
- requisitschwebt: fliegende Gleiter (podcar, 300 ... 800 ueber dem Boden) - gewollt.
- requisitsteckt: Antennen, die durch das Dach gehen (antenna 24/64), Schrott im Sand (crawler_junk2 20) - gewollt.
- fastdeckungsgleich: echte deckungsgleiche Flaechen mit verschiedenen Shadern (concrete_floor_dark/stucco_top_grime auf z 476) - ob
  es im Spiel flackert, haengt an der Zeichenreihenfolge.
- bildgroesse: Gluehbilder 256 x 32, 128 x 32 und desert_dn - laden im Spiel, nur unter dem Richtwert der Liste.

## Fragen

Entschieden (Hauptchat 14.11., im Auftrag von shank): SHADER_MAX_VERTEXES je Regelwerk (umgesetzt, siehe oben), Stufen nur auf
Flaechen ab 96 x 96 (umgesetzt), Spielfigur +-16 fuer SP/MD, der origin-0-Fund und das Tuermass 110 laut Liste (einstellbar) bleiben.
Offen: keine.

## Changelog

### English

- Map editor, optimizer: new checks from the requirements of a Clone Wars mod - visible floors without collision (the player falls
  through), visible floor and collision more than 1 unit apart, invisible collision (after a build), nearly coincident faces
  (z-fighting), steps higher than STEPSIZE, doors and corridors below 110 x 64 and 128 x 96, image sides that are not a power of two
  (the game quits), missing images and model shaders, models placed away from their origin, model surfaces over SHADER_MAX_VERTEXES,
  MD3 format limits (Movie Duels allows 4000 vertices per surface, the limits now follow its Update 9 source), floating props and props
  stuck in the floor; crates and consoles do not count as steps; the sizes are adjustable in the settings. The report can be exported
  as JSON with the position of every object and lists the shaders and images of the map.

### Deutsch

- Karten-Editor, Optimierer: neue Prüfungen aus den Anforderungen eines Clone-Wars-Mods - sichtbare Böden ohne Kollision (man fällt
  durch), sichtbarer Boden und Kollision mehr als 1 Einheit auseinander, unsichtbare Kollision (nach einem Bau), fast deckungsgleiche
  Flächen (Z-Fighting), Stufen höher als STEPSIZE, Türen und Gänge unter 110 x 64 bzw. 128 x 96, Bildseiten, die keine Zweierpotenz
  sind (das Spiel beendet sich), fehlende Bilder und Modell-Shader, Modelle abseits ihres Ursprungs, Modellflächen über
  SHADER_MAX_VERTEXES, Formatgrenzen von MD3 (Movie Duels erlaubt 4000 Ecken je Fläche, die Grenzen folgen jetzt seiner Quelle von
  Update 9), schwebende und im Boden steckende Requisiten; Kisten und Pulte zählen nicht als Stufen; die Maße sind in den Einstellungen
  änderbar. Der Bericht lässt sich als JSON mit dem Ort jedes Objekts exportieren und nennt die Shader und Bilder der Karte.

### 中文

- 地图编辑器,优化器:根据一个 Clone Wars 模组的需求新增检查——没有碰撞的可见地面(玩家会掉下去)、可见地面与碰撞相距超过 1 单位、不可见的碰撞
  (编译之后)、几乎重合的面(深度冲突)、高于 STEPSIZE 的台阶、小于 110 x 64 的门和小于 128 x 96 的走廊、边长不是 2 的幂的图像(游戏会退出)、
  缺少的图像和模型着色器、远离原点放置的模型、超过 SHADER_MAX_VERTEXES 的模型表面、MD3 格式限制(Movie Duels 每个表面允许 4000 个顶点,限制现在依据其 Update 9 源码)、悬浮和陷入地面的道具;箱子和控制台不算台阶;各项尺寸可在设置中修改。
  报告可以导出为带有每个对象位置的 JSON,并列出地图的着色器和图像。

### 日本語

- マップエディター、最適化ツール: Clone Wars MOD の要件から新しいチェック - コリジョンのない見える床(プレイヤーが落ちる)、見える床と
  コリジョンが 1 ユニットを超えて離れている、見えないコリジョン(ビルド後)、ほぼ重なった面(Z ファイティング)、STEPSIZE より高い段差、
  110 x 64 未満のドアと 128 x 96 未満の廊下、2 のべき乗でない画像の辺(ゲームが終了します)、見つからない画像とモデルのシェーダー、
  原点から離れて置かれたモデル、SHADER_MAX_VERTEXES を超えるモデルの面、MD3 の形式上限(Movie Duels は面あたり 4000 頂点まで。上限は Update 9 のソースに従います)、浮いている小道具と床に埋まった小道具。箱や操作盤は段差に数えません。寸法は
  設定で変更できます。レポートは各オブジェクトの位置付きの JSON で書き出せ、マップのシェーダーと画像を一覧にします。

### Español

- Editor de mapas, optimizador: nuevas comprobaciones a partir de los requisitos de un mod de Clone Wars: suelos visibles sin colisión
  (el jugador cae a través), suelo visible y colisión separados más de 1 unidad, colisión invisible (tras compilar), caras casi
  coincidentes (z-fighting), escalones más altos que STEPSIZE, puertas y pasillos por debajo de 110 x 64 y 128 x 96, lados de imagen
  que no son potencia de dos (el juego se cierra), imágenes y shaders de modelo que faltan, modelos colocados lejos de su origen,
  superficies de modelo por encima de SHADER_MAX_VERTEXES, límites del formato MD3 (Movie Duels admite 4000 vértices por superficie; los límites siguen ahora su código de Update 9), accesorios flotantes y hundidos en el suelo; las cajas y consolas no cuentan como escalones; las
  medidas se pueden cambiar en los ajustes. El informe se puede exportar como JSON con la posición de cada objeto y enumera los shaders
  y las imágenes del mapa.

### Čeština

- Editor map, optimalizátor: nové kontroly podle požadavků módu Clone Wars - viditelné podlahy bez kolize (hráč propadne), viditelná
  podlaha a kolize víc než 1 jednotku od sebe, neviditelná kolize (po sestavení), téměř shodné plochy (z-fighting), schody vyšší než
  STEPSIZE, dveře a chodby pod 110 x 64 a 128 x 96, strany obrázků, které nejsou mocninou dvou (hra se ukončí), chybějící obrázky a
  shadery modelů, modely umístěné mimo svůj počátek, plochy modelů nad SHADER_MAX_VERTEXES, limity formátu MD3 (Movie Duels dovoluje 4000 vrcholů na plochu, limity se nyní řídí jeho zdrojem Update 9), levitující rekvizity a
  rekvizity zapadlé v podlaze; bedny a pulty se nepočítají jako schody; rozměry lze změnit v nastavení. Zprávu lze exportovat jako JSON s polohou každého objektu a uvádí shadery
  a obrázky mapy.
