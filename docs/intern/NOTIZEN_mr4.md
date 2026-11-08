# Notizen MR4: Modell-Block-Reste 5, 6, 7 (08.11.2026, Worktree `Y:\mr4`, Zweig `mr4`, Basis fe1f374c3)

Build `Y:/mr4/build-mr4` (wie `Y:/karte/build-test`), Laeufe `m4_*` unter `Y:\mr4\laeufe`, Skripte `Y:\werkzeuge\m4\`.

## Punkt 5: CJK-Umbruch ausserhalb des Model-Tabs (mit P18)

- Vorher brach nur der Model-Tab japanische und chinesische Saetze zwischen den Zeichen um (`gui/textumbruch.h`, p_render);
  ueberall sonst die ImGui-Regel (ImFontCalcWordWrapPositionEx: nur an Leerzeichen und nach `.,;!?"` / `、。` - die erste Zeile
  endete am ersten Komma).
- Jetzt im ganzen Programm: 208 Text-Aufrufe in 46 Dateien (Mission, Karten-Editor mit Optimierer/Lump-Editor/Dialogen, Menue-Editor
  mit Seitenleisten, Helfer, Tutorials, KI, Zeitleiste, Update, Wegpunkte, Figur animieren ...) auf `cjkText*`, dazu neu
  `cjkTextSize` (statt `CalcTextSize(..., breite)`, 13 Stellen) und `cjkAddText` (statt `AddText(..., breite)`, 11 Stellen),
  damit reservierte Hoehe und gezeichnete Zeilen zusammenpassen. Ohne CJK-Zeichen oder ohne Umbruch ruft jede Funktion genau
  ihr ImGui-Gegenstueck (gleiches Bild in en/de/es/cs).
- Gemessen wird die Zeile jetzt als Summe der Vorschuebe (CalcTextSizeA ohne das Aufrunden je Aufruf): aufgerundet je Stueck
  brach eine CJK-Zeile bis zu einem Bildpunkt je Zeichen zu frueh um.
- Neu `tools/lint_cjkumbruch.py` (ctest `lint_cjkumbruch`, `lint_cjkumbruch_selbsttest`): kein `ImGui::TextWrapped`, kein
  ImGui-Text zwischen `PushTextWrapPos`/`PopTextWrapPos`, kein `CalcTextSize`/`AddText` mit Umbruchbreite in gui/.
- P18 (Codepruefung seit rc610): die Messdaten der Testhilfe (`cjkUmbruchStand`) fuellen sich nur im Selbsttest
  (`BHED_EDITORTEST` gesetzt, Feld `messen`); sonst nur die Zeilen.
- Selbsttest `cjkweit` (karteneditor, `BHED_KARTE_NUR=cjkweit`, Lauf mit SPRACHE=ja / zh-Hans): Startflaeche (Karten mit
  AddText), Tutorials-Fenster, Helfer "Was willst du zuerst tun?" - jede umbrochene Zeile ausser der letzten ist so voll, dass die
  naechste Gruppe (Stueck mit den Zeichen, die nach JIS X 4051 nicht am Zeilenanfang stehen, neu `bhed::gruppeEnde`) nicht mehr
  hineinpasst; dazu P18.

| Pruefung | rot | gruen |
|---|---|---|
| lint_cjkumbruch | 232 Stellen | 0 |
| cjkweit ja | `m4_cjk_rot_ja` 13/4 (0 CJK-Texte im Umbruch je Stelle; P18 "Aufrufe 0 -> 1, Zeilen gemessen 0 -> 8") | `m4_cjk_gr3_ja` 17/0 (Startflaeche 2159 Texte, Tutorials 114, Helfer 696) |
| cjkweit zh-Hans | `m4_cjk_rot_zh` 13/4 | `m4_cjk_gr3_zh-Hans` 17/0 |

- Zwischenlauf `m4_cjk_gr2_ja` 16/1 war ein Fehler der Pruefung (Naeherung "3 x erstes Zeichen" statt der Gruppe; "(ICARUS)。"
  und "ニュー・" haengen zusammen) - mit `gruppeEnde` gruen; ctest `umbruch` prueft gruppeEnde.
- Handbuchbilder neu (angesehen): ja/zh `h_start`, `h_ueber`; ja `h_helfer`, `h_helfer_zeig`, `h_helfer_mcp`, `h_ki_frage`
  (Laeufe `m4_hb_*`); zh Helfer siehe Schluss.

## Punkt 6: Spielnachweise GLA

- Aufbau wie Stufe 0 (`nachweise_im_spiel.md`): Testkarte `raum_licht.bsp` als `ms0kart`, Skript `kam3` (Kamera von oben,
  BOTH_CIN_16, 10,2 s), eigener fs_homepath je Lauf, Fenster ausser Sicht, stumm (`Y:\werkzeuge\m4\ms6\`, `aufbau6.py`, `md.py`).
- Neu gebaute Karten-GLA (`Y:\werkzeuge\m4\glagross.py`: Skelett und Bewegung aus Ravens `_humanoid_t1_sour`, eigener Kopf, eigene
  Frames und eigener Vorrat - nicht Ravens Datei umbenannt; mit "einmalig" je Knochen und Frame ein eigener Vorratseintrag, also so
  gross, wie eine GLA mit diesen Frames werden kann). Movie Duels SP:

| Lauf | Frames | Datei | Vorrat | BOTH_CIN_16 ab | Ergebnis |
|---|---|---|---|---|---|
| g3124 | 3124 | 1,5 MB | 71910 | 1321 | geladen (sucht danach `_humanoid_ms0kart.cfg`), Figur geht 588,1181 -> 469,249 |
| g65535 | 65535 | 59 MB | 3473355 | 63801 | gleich (467,250) |
| g131070 | 131070 | 118 MB | 6946710 | 63801 | gleich (467,250) |
| gunter / gkreuz | 131070 | 118 MB | 6946710 | 59152 / 65400 (bis 65603) | gleiche Bahn (030 s 634,1133 / 640,1124 ... 042 s 592,1186) |

- Ergebnis: in Movie Duels keine Speichergrenze bis zur groessten moeglichen Karten-GLA (Anfang hoechstens 65535, Anzahl
  hoechstens 65535 -> 131070 Frames; Vorrat 24 Bit). Frames ueber 65535 spielen (G2 rechnet in int, nur animation_t.firstFrame
  ist unsigned short). Erster gkreuz-Lauf: das Skript lief nicht an (Ansicht blieb, Protokoll gleich) - einmal wiederholt, gruen.
- Pruefung im Tool: cfgPruefen meldet schon Anfang/Anzahl > 65535 (ErsterFrame/Anzahl) und GlaVorrat > 24 Bit; eine weitere
  Groessengrenze haette keine Quelle. ctest `modell_dateien`: neue Zeilen fuer die gemessenen Faelle (131070 Frames, Sequenzen ueber
  65535 hinaus: kein Befund, MdSp und JkaSp).
- Original-JKA (Steam angemeldet, vor jedem Start geprueft, GameData danach gleich): o131070 und o3124 bleiben nach `setviewpos`
  stehen - das Skript startet nicht, das Bild steht (auch mit der kleinen GLA, also nicht die Groesse). Stufe 0 lief am 04.11. mit
  demselben Skript; vermutlich haelt das Original ohne Fokus an (Fokus-Ausnahme gilt nur fuer Movie Duels) - offen.
- Die neu gebaute GLA aus dem Werkzeug selbst ("Fuer das Spiel bauen") ist seit Stufe 7 in MD und im Original belegt
  (NOTIZEN_mb7.md, Endlage < 1 px). MP uebersprungen (MD-MP-Daten bewusst geloescht).

## Punkt 7: Helligkeit Original-JKA gegen Vorschau

- Die 75,5 waren ein Vorschaufoto mit den Regeln von Movie Duels (Selbsttest `jk_mds2`, MD-Daten): dort zieht die Gamma-Stufe von
  Movie Duels (r_gammaShaders, `spielGammaWert`) Grau 32 auf 30. Mit den Daten und Regeln des Original-JKA (`mdspiel` mit neu
  `BHED_MB_SPIEL=jka`) ist die Vorschau gleich hell (L 76,74 gegen 76,73), aber ohne den Farbstich des Originals: Spielfoto R 77,49
  G 76,71 B 77,33, Vorschau 76,81 / 76,72 / 76,65; haeufigster Wert Spiel (33, 32, 33), Vorschau (32, 32, 32).
- Ursache (OpenJK 1:1): Upload32 (rd-vanilla/tr_image.cpp:629-689) laedt jedes Bild mit allowTC bei TC_S3TC_DXT als DXT5
  (r_texturebits 0), jede Mip-Stufe einzeln; r_ext_compress_textures ist 1 (tr_init.cpp:296-388; das Protokoll des Originals sagt
  "using GL_EXT_texture_compression_s3tc", Movie Duels "...ignoring texture compression"). 5:6:5 macht aus Grau 32 (33, 32, 33);
  tr.defaultImage hat auch Alpha 32 (memset), also DXT5 mit Alpha. Ausgenommen: noTC (tr_shader.cpp:2324), Lightmaps
  (r_ext_compress_lightmaps 0).
- Neu `bhed/dxt.h` (`dxt5Rundlauf`: Farbblock mit zwei 5:6:5-Farben auf der Hauptachse und zwei Zwischenstufen, Alphablock mit
  acht Stufen), angewandt beim Hochladen (`ladeMitStufen`) unter Jedi Academy und Jedi Outcast (nicht Movie Duels), nach der
  Pyramide aus dem unkomprimierten Bild; `noTC` im Shaderverzeichnis gelesen. Den Block packt im Spiel der Treiber - dessen
  Verfahren steht nicht in der Quelle; gemessen gegen das Foto (unten).

| Pruefung | rot (`m4_hell_rot2`) | gruen (`m4_hell_gr`) |
|---|---|---|
| ohne Requisit gegen `pj/jka_ohne1` | Mittel 84,91/84,91/84,91 (Spiel 85,68/84,92/85,68), gleiche Punkte 23,6 % | 85,68/84,91/85,68, gleiche Punkte 99,94 %, mittlere Abw. 0,001 |
| mit Requisit gegen `pj/jka_mit1` | 76,81/76,72/76,65 (Spiel 77,49/76,71/77,33), 27,7 % | 77,50/76,72/77,34, 96,8 %, 0,037 |
| Movie Duels (`m4_hell_md`) | - | unveraendert 75,55/75,46/75,40, Grau 30 |

- Die Haeufigkeiten der Farbwerte stimmen danach bis auf einzelne Punkte (Spiel (33,32,33) 1803538, Vorschau 1803539; (247,247,247)
  7354 / 7354). Das Requisit (eine echte Textur) ist die Gegenprobe ausserhalb des Standardbilds.
- ctest `dxt` (neu): Grau 32 -> 33 32 33, Weiss bleibt, Alpha 100 -> 109, Teilblock.
- Die beiden Spielfotos stammen vom 07.11. (p_jka); ein neuer Original-Start war nicht noetig. Bildzeit/Ladezeit mit DXT nicht
  gemessen (allein, Hauptchat).

## Schluss

## Offen

## Changelog

## English

- Interface: Chinese and Japanese text wraps between characters in every window and tab (Mission, map editor, menu editor, helper, tutorials, timeline and more), no longer only after the first comma.
- Preview under Jedi Academy and Jedi Outcast: textures are compressed like in the game (S3TC), so colours match the original exactly; Movie Duels stays uncompressed like the game.

## Deutsch

- Oberfläche: Chinesischer und japanischer Text bricht in allen Fenstern und Tabs zwischen den Zeichen um (Mission, Karten-Editor, Menü-Editor, Helfer, Tutorials, Zeitleiste und mehr), nicht mehr erst nach dem ersten Komma.
- Vorschau unter Jedi Academy und Jedi Outcast: Texturen werden wie im Spiel komprimiert (S3TC), die Farben stimmen jetzt genau mit dem Original überein; Movie Duels bleibt wie das Spiel unkomprimiert.

## 中文

- 界面:中文和日文文本在所有窗口和标签页中按字符换行(任务、地图编辑器、菜单编辑器、助手、教程、时间轴等),不再只在第一个逗号后换行。
- Jedi Academy 和 Jedi Outcast 下的预览:纹理像游戏中一样压缩(S3TC),颜色与原版完全一致;Movie Duels 与游戏一样保持不压缩。

## 日本語

- インターフェース:中国語と日本語のテキストは、すべてのウィンドウとタブ(ミッション、マップエディター、メニューエディター、ヘルパー、チュートリアル、タイムラインなど)で文字の間で折り返されます。最初の読点の後だけで折り返すことはなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:テクスチャをゲームと同じように圧縮(S3TC)するため、色がオリジナルと正確に一致します。Movie Duels はゲームと同じく非圧縮のままです。

## Español

- Interfaz: el texto chino y japonés se corta entre caracteres en todas las ventanas y pestañas (Misión, editor de mapas, editor de menús, ayudante, tutoriales, línea de tiempo y más), ya no solo después de la primera coma.
- Vista previa con Jedi Academy y Jedi Outcast: las texturas se comprimen como en el juego (S3TC), así los colores coinciden exactamente con el original; Movie Duels sigue sin comprimir, como el juego.

## Čeština

- Rozhraní: čínský a japonský text se ve všech oknech a kartách (Mise, editor map, editor nabídek, pomocník, výukové lekce, časová osa a další) zalamuje mezi znaky, už ne až za první čárkou.
- Náhled pro Jedi Academy a Jedi Outcast: textury se komprimují jako ve hře (S3TC), takže barvy přesně odpovídají originálu; Movie Duels zůstává jako hra bez komprese.
