# Notizen MERGE3: fix2 nach karte1, dann Reste (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 443b64d5b)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `m3_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\mg3\` (Rotlaeufe `rot\`, Gruenlaeufe `gruen\`, `g\`).

## Teil 1: Merge fix2 bis 0e7235969 (6288e069e)

- Konflikte nur in den erzeugten `handbuch/*/suche.js`; neu erzeugt mit gen_i18n, gen_handbuch, gen_hilfe_orte.
- Stiller Konflikt in der Figurenstufe (b3): MERGE2 legte den Massstab der Entity in `gStufeTurbQ4.zw` (sd[74], sd[75]), FIX4 hat
  gStufeTurbQ4 aufgeloest (kStufeFloats 72 -> 68). Nach dem Merge uebersetzte der Figuren-Vertex-Shader nicht mehr und
  `entityWerte` schrieb hinter die 68 float. Rot `rot/merge_ctest2.txt`: hlsl "error X3004: undeclared identifier
  'gStufeTurbQ4'" je Figurvariante (defgpu ebenfalls rot). Behebung: eigenes `gEntSkala` (float 68..70, kStufeFloats 72, weiter
  zwei Ring-Eintraege), defgpu-Puffer 72. Gruen `rot/rot1_ctest.txt`: hlsl und defgpu bestanden.
- Uebrige Registerlagen: gpuconst, gpushader, turbgpu, defgpu, hlsl gruen (JeStapel unveraendert, nur JeStufe waechst).

## Teil 2

### P45 und Punkt 5: Wellenname und Zahlen wie im Spiel (f7783497a)

- NameToGenFunc (tr_shader.cpp:404-437 OpenJK, MD9 gleich): Q_stricmp, unbekannter Name -> Warnung und GF_SIN; nur ein fehlendes
  Wort laesst GF_NONE (ParseWaveForm :450-455). Vorher None (Welle aus) und Grossschreibung traf nicht. Kommentar in wave.h
  berichtigt.
- Zahlen der Shader wie `(float)atof` (strtod in double, dann float; das Programm setzt keine Locale): vorher from_chars<float>
  (eine Rundung, kein "+").
- Rot `rot/rot1_ctest.txt`: wavetest 2 (sine, SawTooth), bilder 4 (deformVertexes/rgbGen wave sine, 1.0000001788139343261718749 ->
  1+2^-22, "+0.5"); gruen `gruen/g1_ctest.txt`.
- P48: wavetest vergleicht s_noise_table/s_noise_perm mit dem echten rand() nach srand(1001): 0/0 von 256 abweichend; Gegenprobe
  srand(1002) weicht ab (der Vergleich greift). Kein Rotlauf moeglich - die Tafel war richtig.

### Punkt 2, Punkt 3, P53: Haeute wie RE_RegisterSkin (2c93a3dd3)

- Punkt 2: das Streichen von "_off" am Hautnamen steht unter `#ifndef JK2_MODE` (tr_skin.cpp:308-316 OpenJK); unter JK2 bleibt der
  Name, "x_off,*off" schaltet x_off ab.
- Punkt 3: "Skin oeffnen" (und die Hautliste) ueber `wendeHautNeuAn`: Texturen, Abschaltungen und hautZeile zurueck wie G2API_SetSkin
  (neues mCustomSkin, G2_API.cpp:839-858) und mSlist.clear() (tr_ghoul2.cpp:2215).
- P53: 128 Eintraege je Haut (skin_t::surfaces[128], tr_skin.cpp:318-323), doppelte zaehlen mit, "tag_"-Zeilen nicht (:303-305),
  leerer Flaechenname beendet die Datei; dreiteilige Haeute zaehlen zusammen (applySkin nimmt und liefert die Zahl), hat die Haut nach
  einem Teil keinen Eintrag, ist sie keine (hSkin 0, :331-334, :413-424).
- Rot `rot/rot1_ctest.txt`: modell 7 FEHL (Punkt 3: 2, P53: 3, Punkt 2: 2); gruen `gruen/g1_ctest.txt`.
- Nicht nachgebildet: die Wortfolge von CommaParse bei "tag_"-Zeilen (das Spiel liest danach den Shadernamen als naechsten
  Flaechennamen) und bei Zeilen ohne Komma.

### P44 (Teil Parser): ParseDeform wie im Spiel

- Hoechstens MAX_SHADER_DEFORMS (3) Eintraege; jede Zeile mit Art zaehlt (auch projectionShadow, text0..7, unbekannte Arten),
  autosprite/autosprite2 nur innerhalb der Grenze; fehlende Werte bei bulge/normal/move-Richtung/wave-div -> DEFORM_NONE;
  ParseWaveForm hoert beim ersten fehlenden Wert auf.
- Rot `rot/rot1_ctest.txt`: bilder 4 (vierter Eintrag move, bulge 3 10, normal 0.1, text/projectionShadow/unbekannt zaehlen); gruen
  `gruen/g2_bilder.txt` (aa74576c9).

### P49: spiel.py (Werkzeug `Y:\werkzeuge\f2\f5\spiel.py`, nicht im Repo)

- Launcher nur an der Eltern-PID (Toolhelp32, `kinderVon(p.pid)`) erkannt und sofort beendet (Wache je 50 ms, auch nach dem Ende von
  p), kein Beenden nach dem Namen mehr; am Ende nur p samt Baum und seine Kinder; die mp3 im TEMP wird entfernt, wenn sie vorher nicht
  da war; der Schluessel ist eine Konstante und wird vor dem Start geprueft. Vorher-Stand `Y:\werkzeuge\mg3\spiel_f5_vorher.py`.
  Geprueft: Syntax und `kinderVon` (findet das Kind der eigenen Shell); kein Spielstart (kein Rot-Test, Werkzeug).

## Gruenlauf (`Y:\werkzeuge\mg3\gruen.sh`, parallel, Summe `gruen_summe.txt`)

- ctest 243/243, Modus 1 2635/0, mission1 98/0, menueeditor en 2787/0, figurlicht 8/0, editorportal 19/0, karteneditor rd 21/0,
  r3 22/0, modellblock pt 41/0, ptfix3 12/0; Lints wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung
  bekannt, cjkumbruch 0); Handbuch `--pruefen` ok.
- Vorschaufotos gegen MERGE2 (`m2_*_vor_gruen`): p27e und p27g bildpunktgleich bis auf die Leiste unten rechts (2898..3251 x
  2009..2026, Zeitangabe). Der erste p27e-Lauf (`m3_p27e_vor`) war ungueltig: im pk3-Ordner von MERGE2 liegt auch zz_p27f.pk3 (scale
  300) - neu mit eigenem Ordner `Y:\werkzeuge\mg3\p27e_pk3` (`m3_p27e_vor2`).

## Offen

- P44 Rest: zwei Eintraege derselben Art wirken beide (heute je Art ein Satz Werte, der letzte); autosprite an seiner Stelle der
  Folge (heute wirken die Verformungen vor dem autosprite des Vertex-Shaders). Plan: drei allgemeine Plaetze je Eintrag in den
  16 float von `gpu::verformung` (d0..d2 Werte, d3.xyz spreads, d3.w Folge zu je 4 Bit mit Markern fuer autosprite), Karten-VS
  `verformeEcke(.., von, bis)` vor und nach dem autosprite-Schritt (nachher n = -gVorn, st = Ecke des Plaettchens wie
  RB_AddQuadStampExt).
- P45 Rest: Kartenpruefung meldet Shader, bei denen das Spiel mit ERR_DROP abbricht (TableForFunc: deformVertexes wave mit
  frequency != 0 und noise/random, move mit noise/random/ohne Form, rgbGen/alphaGen/stretch wave ohne Form).
- P46 (Wolkenstufe ps_5_0 bitgleich auf Intel), P47 (`if (art == 0u) break;`, normal-Zweig nur mit kDeformNormal, Uebersetzungszeit),
  Chamfer-Feinheiten (NOTIZEN_ptfix3_0811.md "Punkt 3") - Abgabe wegen Kontext.
- Haut unter JK2_MODE: G2API_SetSkin setzt dort nur mCustomSkin (G2_API.cpp:842-848), ohne die An/Aus-Liste aus der Haut -
  `wendeHautNeuAn` setzt die Abschaltungen auch unter JK2 zurueck und applySkin schaltet "*off" ab; nicht gegen jk2sp geprueft.
- p27f nicht neu fotografiert (Kamera der MERGE2-Aufnahme nicht festgehalten).

## Changelog

## English

- Shaders: an unknown wave name (e.g. "sine") now waves as a sine like in the game instead of standing still, and shader numbers are read exactly like the game reads them.
- Shaders: at most three deformVertexes lines count like in the game, and an incomplete line has no effect.
- Characters: skins follow the limits of the game (128 entries, tag_ lines, Jedi Outcast keeps "_off"), and opening another skin starts from the plain model.

## Deutsch

- Shader: Ein unbekannter Wellenname (z. B. „sine“) wogt jetzt wie im Spiel als Sinus statt stillzustehen, und Zahlen in Shadern werden genau wie im Spiel gelesen.
- Shader: Wie im Spiel zählen höchstens drei deformVertexes-Zeilen, eine unvollständige Zeile wirkt nicht.
- Figuren: Häute folgen den Grenzen des Spiels (128 Einträge, tag_-Zeilen, Jedi Outcast behält „_off“), und eine andere Haut zu öffnen beginnt beim unveränderten Modell.

## 中文

- 着色器: 未知的波形名称 (例如 "sine") 现在与游戏一样按正弦波动, 不再静止; 着色器中的数字与游戏的读取方式完全相同。
- 着色器: 与游戏一样最多计算三行 deformVertexes, 不完整的行不起作用。
- 角色: 皮肤遵循游戏的限制 (128 个条目、tag_ 行、Jedi Outcast 保留 "_off"), 打开另一个皮肤时从原始模型开始。

## 日本語

- シェーダー: 未知の波形名 (例: 「sine」) はゲームと同じくサイン波として揺れるようになり、止まったままではなくなりました。シェーダー内の数値もゲームとまったく同じように読み取ります。
- シェーダー: ゲームと同じく deformVertexes は最大 3 行まで有効で、不完全な行は効果がありません。
- キャラクター: スキンはゲームの制限に従います (128 エントリ、tag_ 行、Jedi Outcast では「_off」を保持)。別のスキンを開くと元のモデルから始まります。

## Español

- Shaders: un nombre de onda desconocido (p. ej. "sine") ahora ondula como un seno igual que en el juego en lugar de quedarse quieto, y los números de los shaders se leen exactamente como los lee el juego.
- Shaders: como en el juego, cuentan como máximo tres líneas deformVertexes y una línea incompleta no tiene efecto.
- Personajes: las skins siguen los límites del juego (128 entradas, líneas tag_, Jedi Outcast conserva "_off"), y al abrir otra skin se parte del modelo sin cambios.

## Čeština

- Shadery: neznámý název vlny (např. „sine“) se nyní vlní jako sinus stejně jako ve hře, místo aby stál, a čísla v shaderech se čtou přesně jako ve hře.
- Shadery: stejně jako ve hře se počítají nejvýše tři řádky deformVertexes a neúplný řádek nemá žádný účinek.
- Postavy: skiny dodržují limity hry (128 položek, řádky tag_, Jedi Outcast ponechává „_off“) a otevření jiného skinu začíná od nezměněného modelu.
