# Notizen REST8: doppelte Shadernamen im Karten-Editor nach q3map2, const als Byte, Kartenpruefung (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 580e289f3)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r8_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r8\`. Quellen: q3map2 im Repo (`third_party/q3map2`, NetRadiant-Custom 20260114), OpenJK `Y:\werkzeuge\sp_af9c\m3\ojq\src`,
Movie Duels `Y:\werkzeuge\sp_654alt\md9`, Jedi Outcast 1.04 im OpenJK-Verlauf a09d4827 (`Y:\werkzeuge\sp_654alt\ojk`).

## Punkt 1: Echtzeit-Licht und Karten-Editor nach der Regel von bhed_q3map2

- Entscheidung (Hauptchat): das Echtzeit-Licht zeigt das Kompilierergebnis, also loest es doppelte Shadernamen wie bhed_q3map2 auf;
  "Gebaute Karte wie im Spiel" und die Vorschau bleiben bei der Regel des Spiels (REST7).
- Regel von q3map2, aus dem Quelltext (Port `bhed/karte/q3map2shader.h`, `src/karte/q3map2shader.cpp`):
  - InitPaths (path_init.cpp:290-428): vfsInitDirectory fuer `<fs_basepath>/<fs_game>/`, dann `<fs_basepath>/base/`, dann jedes
    `-fs_pakpath` (das Build-Menue gibt -fs_game MD und die Extra Resource Paths samt Import-Ordner mit).
  - vfsInitDirectory (vfs.cpp:140-191): Verzeichnis und *.pk3dir in g_strDirs, die .pk3 aufsteigend (string_compare_nocase_upper), ihre
    Eintraege per emplace_front in g_pakFiles - am Ende stehen die .pk3 von base VOR denen des Mods, je Ordner die alphabetisch letzte zuerst.
  - LoadShaderInfo (shaders.cpp:1806-1856): mit shaders/shaderlist.txt nur die dort genannten Dateien, sonst vfsListShaderFiles
    (vfs.cpp:194-228: erst die losen shaders/*.shader jedes Verzeichnisses, dann g_pakFiles, eindeutig ohne Gross/klein).
  - vfsLoadFile(name, 0) (vfs.cpp:261-330): jede lose Datei gewinnt gegen jede .pk3, die .pk3 von base gegen die des Mods.
  - ParseShaderFile (shaders.cpp:823-1735) mit dem Zerleger von scriplib.cpp (Kommentare `;` `#` `//` `/* */`, char mit Vorzeichen: Bytes
    ab 0x80 trennen wie Leerraum, `$include`): ":q3map" faellt weg, ohne "{" bricht q3map2 ab; nach jeder Anweisung und jeder Stufe gilt
    der Rest der Zeile nicht ("} }" auf einer Zeile schliesst nur die Stufe), die Matrizen von q3map_tcGen vector/ivector,
    q3map_colorMod & Co. und q3map_fogDir duerfen Zeilen ueberspannen, Bilder von map/animMap & Co. werden nur bis zum ersten
    qer_editorimage/q3map_lightImage/implicit* als Wort genommen. AllocShaderInfo legt jeden Eintrag an, ShaderInfoForShader nimmt den
    ERSTEN (UnsortedSet::find_first, multiset haengt Gleiche hinten an).
- Unterschied zum Spiel (REST7: Dateien umgekehrt verkettet, die .pk3 mit dem niedrigeren Rang und im Ordner die .pk3 vor den losen
  Dateien gewinnen, eine gleichnamige Datei aus dem hoechsten Rang) und zum Inventar von NetRadiant-Custom (Mod-Ordner zuerst, je Ordner
  lose Dateien vor den .pk3 absteigend, erste Definition). Gezaehlt auf den Movie-Duels-Daten (`q3vorrang.py`, eigener Python-Port; das
  Programm im Selbsttest gleich): q3map2 624 Dateien, 9799 Namen, 265 verdeckte Eintraege; 87 Shader loest q3map2 anders als das Spiel
  auf (46 davon unter textures/, z. B. textures/skies/korriban: q3map2 die lose base/shaders/skies.shader, das Spiel MD_Assets.pk3;
  models/map_objects/rift/statue: q3map2 exegol.shader, das Spiel rift_statuefix.shader), 74 zeichnete das Echtzeit-Licht bis REST8 anders
  (Inventar von NRC). Original-JKA (`GameData original/base`, dort liegt eine lose shaders/shaderlist.txt): q3map2 liest nur deren 40
  Dateien (1893 Namen), das Spiel alle (3365), 24 verschieden aufgeloest.
- Programm: Spielkontext::q3map2 und Spielkontext::spielShader (beim Laden des Spiels im Faden und bei "Flush & Reload Shaders",
  shaderVorrangLesen); echtShaderInfo und skript() (Stufen, Flaechenlicht, q3map_sun usw. des Echtzeit-Lichts, Gras, Effekte, Spiegel)
  nehmen den Eintrag von q3map2. Die Kartenpruefung (Optimierer) liest, was der Bau braucht (Flaechenlicht, Backsplash, origin, Himmel,
  Lightmap-Groesse), aus dem Eintrag von q3map2, was das Spiel zeichnet (ERR_DROP, autosprite), aus dem des Spiels. Neue Pruefung
  "shadervorrang" (Warnung, 6 Sprachen, Zeile in der Tabelle der Pruefungen im Handbuch): Shader der Karte, die Bau und Spiel
  verschieden aufloesen. Handbuch: Notiz auf der Seite Tempo und Echtzeit-Licht vor "Gebaute Karte wie im Spiel" (6 Sprachen).
- Gemeinsame Teile statt doppelter Logik: `shaderEintragText` (Name + Block mit Klammern auf eigener Zeile, auch fuer REST7),
  `shaderTextAusOrdnern` (der Spieltext aus beliebigen Spielordnern; die Vorschau ruft es mit ihren Ordnern).
- Ladezeit des Spiels im Karten-Editor (Movie Duels) 0,8 s im Selbsttest.
- Rot/Gruen: Selbsttest karteneditor BHED_KARTE_NUR=rest8 mit dem Echtzeit-Licht und der Kartenpruefung wie vor REST8 rot
  (`r8_rest8_rot`: Echtzeit-Licht anders als q3map2 bei 80 Shadern, Kartenpruefung ohne "shadervorrang"), gruen (`r8_rest8_gruen`
  14 OK 0 FEHLER: 0 anders, "shadervorrang" genau fuer textures/cloudy/lawa, nicht fuer textures/00mj_larshomestead/bswitch). ctest
  shadervorrang um 23 Pruefungen erweitert (kuenstliche .pk3 und lose Dateien: Folge lose/base/Mod, md_z vor md_a, ":q3map",
  "} }", Matrix ueber Zeilen, Byte 0xA0, ';', shaderlist.txt, Abbruch ohne "{", insert_unique; Gegenprobe NRC-Inventar und Spielregel
  entscheiden anders), ctest karte_optimierer (Befund je Shader, ohne vorrang keiner) - gruen.
- Nicht nachgebildet: die Abbrueche von q3map2, wenn einer anderen Anweisung ihr Wert auf derselben Zeile fehlt ("Line is incomplete" -
  dann baut q3map2 gar nicht). Fruehere Modell-Importe unter scripts/ (Stufe 9d) liest q3map2 nicht; das Echtzeit-Licht zeigt sie jetzt
  wie der Bau ohne Skript.

## Punkt 2: rgbGen const / alphaGen const als Byte

- Spiel: stage->constantColor ist ein byte (tr_local.h:388 OpenJK, :376 MD9, JK2 :367). ParseStage: rgbGen const ueber VectorClear +
  ParseVector (tr_shader.cpp:1466-1478 und :108-134 OpenJK, MD9 :1443-1455; erst ein eigenes Wort "(", dann je Wert atof als float,
  ein fehlendes Wort bricht ab, ohne "(" bleibt die Farbe schwarz), `constantColor = 255 * Wert` (float); alphaGen const
  `255 * atof(token)` (double), AGEN_CONST auch ohne Wort. Die Wandlung schneidet zur Null hin ab und behaelt das untere Byte
  (cvttss2si/cvttsd2si): 0,125 -> 31, 0,752941 -> 191, 2 -> 254, -0,5 -> 129. Jedi Outcast laesst die Farbe ohne "(" uninitialisiert
  (kein VectorClear) - die Vorschau nimmt dort 0 wie OpenJK.
- Vorschau bisher: der Gleitwert (0,125 statt 31/255, 2,7 %), Klammern freiwillig, fehlende Werte "nicht gesetzt". Jetzt
  spielKonstantByte in parseShaderScript, damit gelten Vorschau, Figuren, Effekte, Echtzeit-Licht und Spielbild gleich. In den
  Movie-Duels-Daten: 683 rgbGen const (alle mit "("), 819 alphaGen const; 195 verschiedene Werte sind kein genaues Byte.
- Rot/Gruen: ctest gpushader 5 neue Pruefungen rot (Bau mit dem alten Leser: 5 FEHL), gruen (0 Fehlschlaege).

## Punkt 3: Kartenpruefung mit der neuen Shader-Regel

- `kp.sh` (tools/kartenpruefung.py, 282 Karten, Vergleich mit dem letzten vollen Lauf `Y:\werkzeuge\zs\kp_render` vom 07.11.): alle
  Summen gleich bis auf "Karten mit fehlender Textur" 142 -> 117. Es fallen nur Meldungen weg (29 Texturen, 0 neue), alle sind in den
  Spieldaten als Shader definiert (z. B. textures/colors/black_nomarks in assets1.pk3 und base/shaders/system.shader,
  textures/jeditemple_garden/light2-10 in MD_Maps_Canonical.pk3, textures/tatooine_outpost/barsign_paint hinter der Zeile
  "{ map ..." aus REST7) - die alte Regel hatte sie verloren. Nichts zu beheben. Welt-, Figuren- und Effekt-Shader ohne Luecken (0 -> 0).

## Schluss (`Y:\werkzeuge\r8\schluss.sh`, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen (1537 s); ctest 242/245 mit -j 12, q3map2_pr316 1/1. Rot waren drei Tests mit der alten
  Erwartung: figurstufen und bilder (rgbGen/alphaGen const als Gleitwert bzw. ohne Klammern) - auf die Spielregel umgestellt -, karte_lizenz
  (Kopf von q3map2shader.h ohne Copyright/GPL) - ergaenzt; danach 5/5 gruen (gpushader, figurstufen, bilder, karte_lizenz, shadervorrang).
- Parallel: figurlicht 8/0, mission1 98/0, karteneditor voll 3393/0 (mit rest8), editorportal 19/0, laserfalle 9/0, gluehen 9/0, rest605
  moversprite 7/0, modellblock funktionen 61/0 und md 86/0, menueeditor 2787/0, Modus 1 2635/0.
- Lints wie die Basis (portability 99 nach `<cstdint>` in shaderscript.cpp, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt,
  link/tidy nur unter Linux, sonst ok). Handbuch neu erzeugt (6 Sprachen, nur Text, keine Bilder betroffen). Kartenpruefung siehe Punkt 3
  (gleicher Code, nicht wiederholt). Keine Bildzeit-Messung (Auftrag).

## Offen

- Gegen das Spiel/q3map2 selbst ist die Regel nicht im Lauf gemessen (kein Bau einer Karte mit doppeltem Shader und Vergleich der
  Flaechenflags); belegt aus dem Quelltext und dem eigenen Python-Port (gleiche 9799 Namen).
- q3map2 bevorzugt die .pk3 von base vor denen des Mods und jede lose Datei vor jeder .pk3 (Eigenheit von vfs.cpp) - der Bau kann so
  andere Flaechenflags/Flaechenlicht nehmen als das Spiel zeichnet; die Kartenpruefung warnt, geaendert ist bhed_q3map2 nicht.
- Jedi Outcast: rgbGen const ohne "(" ist im Spiel uninitialisiert - die Vorschau nimmt 0.

## Changelog

## English

- Map editor: when a shader name is defined more than once, the real-time light now uses the definition bhed_q3map2 builds with (loose files before every .pk3, base before the mod, first definition) instead of the one in the texture browser; the map check warns when the build and the game resolve a shader of the map differently.
- Preview: rgbGen const and alphaGen const are stored as a byte like in the game (0.125 becomes 31/255).

## Deutsch

- Karten-Editor: Steht ein Shadername mehrfach, nimmt das Echtzeit-Licht jetzt die Definition, mit der bhed_q3map2 baut (lose Dateien vor jeder .pk3, base vor dem Mod, die erste Definition), statt der des Textur-Browsers; die Kartenpruefung warnt, wenn Bau und Spiel einen Shader der Karte verschieden aufloesen.
- Vorschau: rgbGen const und alphaGen const werden wie im Spiel als Byte gespeichert (0,125 wird 31/255).

## 中文

- 地图编辑器：当一个着色器名称被多次定义时，实时光照现在使用 bhed_q3map2 构建时采用的定义（松散文件先于任何 .pk3，base 先于模组，第一个定义生效），而不是纹理浏览器中的定义；当构建与游戏对地图中某个着色器的解析不同时，地图检查会发出警告。
- 预览：rgbGen const 和 alphaGen const 像游戏中一样以字节存储（0.125 变为 31/255）。

## 日本語

- マップエディター：同じシェーダー名が複数回定義されている場合、リアルタイムライトはテクスチャブラウザーの定義ではなく、bhed_q3map2 がビルドに使う定義（ばらのファイルがどの .pk3 よりも先、base が MOD より先、最初の定義）を使うようになりました。マップのシェーダーをビルドとゲームが異なって解決する場合、マップチェックが警告します。
- プレビュー：rgbGen const と alphaGen const をゲームと同じくバイトで保持します（0.125 は 31/255 になります）。

## Español

- Editor de mapas: si un nombre de shader está definido más de una vez, la luz en tiempo real usa ahora la definición con la que compila bhed_q3map2 (archivos sueltos antes que cualquier .pk3, base antes que el mod, la primera definición) en lugar de la del navegador de texturas; la comprobación del mapa avisa cuando la compilación y el juego resuelven distinto un shader del mapa.
- Vista previa: rgbGen const y alphaGen const se guardan como un byte, como en el juego (0,125 pasa a ser 31/255).

## Čeština

- Editor map: pokud je název shaderu definován vícekrát, světlo v reálném čase nyní používá definici, se kterou staví bhed_q3map2 (volné soubory před každým .pk3, base před modem, první definice), místo definice z prohlížeče textur; kontrola mapy varuje, když sestavení a hra řeší shader mapy různě.
- Náhled: rgbGen const a alphaGen const se ukládají jako bajt jako ve hře (0,125 se stane 31/255).
