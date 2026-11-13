# NOTIZEN LCFIX (12.11.2026): Funde P4-1 bis P4-4 der Codeprüfung seit rc611

Auftrag: `JKStudio_Uebergabe\plaene\codepruefung_seit_rc611_p4.md`, Funde P4-1 bis P4-4 (Licht-Cache, VIS-Wiederverwendung,
VIS-Zwischenstand). Maßstab: ein Bau mit Cache bzw. Wiederverwendung liefert Byte für Byte dieselbe .bsp wie der Vollbau.
Je Fund erst rot (mit dem Compiler von 73c18113f, `Y:\werkzeuge\lcfix\exe_rot`), dann behoben, dann im selben Test grün.
Begonnen von LCFIX (Fable 5.2), fortgesetzt von LCFIX (Opus 5.5).

Kurzwege: LC = `src/q3map2/bhed_lichtcache.cpp`, LCH = `.h`, Q = `third_party/q3map2/tools/quake3/q3map2`.

## Neue Tests

- ctest `q3map2_lichthash` (`tests/lichthash.cpp`): der Hash des Licht-Caches ohne Karte (jedes gekippte Bit jeder Länge 0-40,
  angehängte Nullbytes, Teilstücke) und der Schalter-Schlüssel (`BhedLichtSchalterSig`: `-subdivisions`, Spiel).
- ctest `q3map2_lichtabdruck` (`tools/karte/q3map2_lichtabdruck.py`): eigene kleine Karte mit Modellen (ASE), Texturen (TGA)
  und Shadern im Arbeitsordner. Je Fall: erster Bau mit Cache, EINE Eingabe ändern, Teilbau mit Cache gegen den Vollbau der
  geänderten Karte ohne Cache (die .bsp ohne Uhrzeit, jede externe Lightmap) und die Gegenprobe "die Änderung wirkt" (der
  Vollbau weicht in den genannten Lumps vom ersten Bau ab - sonst bewiese der Vergleich nichts). 13 Fälle, `BHED_LICHTABDRUCK_NUR`
  wählt aus. Rund 12 Minuten unter Last.
- ctest `q3map2_visschluessel` (`tools/karte/q3map2_visschluessel.py`): Gitterkarte 14 x 14 von `q3map2_visgleich.py`; die
  Regel im Schlüssel von `-bhedvisreuse`, die Fassung in den Schlüsseln von Wiederverwendung und Zwischenstand.
- `tests/kartebau.cpp`: die Meldung "older cache format (made by ..., this is ...)" fällt im Editor unter "älteres Format".

## Rot und Grün je Fund

| Fund | Fall | Rot (exe_rot, 73c18113f) | Grün (neu) |
|---|---|---|---|
| P4-2 Hash 7 mod 8 | `lichthash` | 4 FEHL ("1 1 0.5" = "1 1 0.2", jedes Bit ab Länge 7) | 0 FEHL |
| | Karte `floodlight` (`_floodlight` "... 1" -> "... 7", 15 Zeichen) | "geometry and switches unchanged", Teilbau weicht in Lumps 10, 15, 17 ab | "worldspawn keys changed", Teilbau = Vollbau |
| P4-1 (a) Alphakanal einer alphashadow-Textur | `alpha` | Vertexlicht und Gitter aus dem Cache, Lumps 10, 15, 17 falsch | Vertexlicht und Gitter neu, Teilbau = Vollbau |
| P4-1 (b) Punkt-Entity mit `model` und `_cs` verschoben | `modell` | Lump 10 falsch | Teilbau = Vollbau |
| P4-1 (b) Modelldatei dieser Entity geändert | `modelldatei` | Lump 10 falsch | Teilbau = Vollbau |
| P4-1 (c) `-subdivisions 32` -> `8` mit `-patchshadows` | `subdivisions` | Lump 10 falsch | "light switches changed", Teilbau = Vollbau |
| P4-1 (d) eigene Lücke: Shader `name:q3map` ohne Stufen | `q3mapshader` | Lump 10 falsch | "geometry, shaders ... changed", Teilbau = Vollbau |
| P4-3 Licht-Cache anderer Fassung | `fassung` (`BHED_Q3_FASSUNG`) | ganz übernommen | "older cache format (made by rc611, this is ...)", nichts übernommen, Teilbau = Vollbau |
| | `altformat` (Kennung BHLICHT5) | übernommen | "older cache format", Teilbau = Vollbau |
| | von Hand: echter BHLICHT5-Cache von exe_rot, Teilbau mit dem neuen exe | - | "older cache format", kein Absturz, .bsp = Vollbau |
| P4-3 VIS-Wiederverwendung anderer Fassung | `visschluessel` | "visibility reused" | "visibility computed", VIS = Bezug |
| P4-3 VIS-Zwischenstand anderer Fassung | `visschluessel` | "vis resumed" | "vis state not used (state of another version)", VIS = Bezug |
| P4-4 `-bhedvisexact` nach NRCs Regel (8 Threads) | `visschluessel` | "visibility reused", VIS = der von NRCs Regel (weicht im 1. Versuch vom Bezug ab) | "visibility computed", VIS = Bezug `-bhedvisexact` |
| P4-4 `-bhedvisfixed` nach NRCs Regel | `visschluessel` | wie oben | "visibility computed", VIS = Bezug `-bhedvisfixed` |
| P4-4 NRCs Regel nach `-bhedvisexact` | `visschluessel` | "visibility reused" | "visibility computed" |

Gegenproben, schon vor dem Fix grün und danach weiter grün: `shaderskript`, `lichtbild`, `sonne` (Teilbau = Vollbau) und
`faeden` (nur `-threads 1` -> `8`: alles kommt weiter aus dem Cache, 11/11 Lightmaps, 1/1 Fläche, Gitter); in
`visschluessel` geht die Wiederverwendung bei gleicher Regel weiter ("visibility reused" für `-bhedvisexact` nach
`-bhedvisexact`, `-threads 1` nach `-bhedvisexact`, `-bhedvisfixed` mit 2 nach 8 Threads).
Protokolle: `Y:\werkzeuge\lcfix\rot1.txt`, `rot2.txt`, `rot3.txt`, `rot_lichthash.txt`, `rot_vis.txt` (rot);
`gruen1.txt`, `gruen2.txt`, `vis_p43.txt` (Stand nach P4-3: Fassung grün, Regel noch rot), `vis_p44.txt` (grün).

## Was geändert ist

- **P4-2** (LCH `BhedHash::daten`): die Länge des Rests steht in Bit 56-58, die Marke in Bit 63, die Daten in Bit 0-55. Vorher lag
  die Länge in Bit 48-50, wo bei 7 Bytes das letzte Byte liegt. Kennung `BHLICHT6`.
- **P4-1**:
  - Vertexlicht (`BhedFlaecheAusCache`) und Gitter (`BhedGitterAusCache`) haben die Signatur des GANZEN Strahlbaums im Abdruck
    (`BhedSpurGanzSig`, Q/light_trace.cpp: Haupt- und Himmelsbaum mit jedem Dreieck, seinem Shader samt Bild bei alphashadow und
    Lichtfilter, Fläche, Schattenflags; Modelle von Punkt-Entities mit `_cs`; Patch-Dreiecke). Ohne Signatur wird gerechnet und
    nichts abgelegt. Folge: jede Änderung an einem Schattenwerfer rechnet Vertexlicht und Gitter ganz neu ("lieber einmal zu viel").
  - Die Signaturen entstehen jetzt am Anfang des ersten Durchgangs (`BhedLichtCacheDurchgang( 0 )`), weil das Gitter vor den
    Lightmaps gerechnet wird; nötig sind nur SetupSurfaceLightmaps und SetupTraceNodes (beide vor LightWorld).
  - `-subdivisions` (vor `-light` genommen, Q/main.cpp:108) steht im Schalter-Schlüssel (`BhedLichtSchalterSig` in LCH).
  - (d) Ein Shader `name:q3map` ohne Stufen bekommt von q3map2 keinen `shaderText` (Q/shaders.cpp, für die Custom-Shader so
    gewollt). Sein Skript steht jetzt in einem eigenen Feld `bhedSkript` (Q/q3map2.h) und geht in den Geometrie-Schlüssel und in
    die Shader-Signatur ein. `shaderText` bleibt wie in NRC leer. Am Testfall: `q3map_shadeAngle` mit `-shade` auf zwei
    vertexbeleuchteten Patches (Modelle taugen nicht: SmoothNormals übergeht Dreieckssuppen).
  - Kennung `BHLICHT7`.
- **P4-3**: `BhedQ3Fassung()` = `bhed::kFassung` (heute "rc611"), für Tests ersetzt durch `BHED_Q3_FASSUNG`.
  - Licht-Cache: Kopf = Kennung (8) + Fassung (32 Bytes, mit Nullen aufgefüllt) + 3 x 16; Kennung `BHLICHT8`. Andere Fassung ->
    `light cache not usable: older cache format (made by <alt>, this is <neu>)`; der Editor liest das Präfix und zeigt den
    vorhandenen Text "der Cache stammt aus einer älteren Version". `tools/karte/q3map2_teilbau.py` liest den Kopf mit der neuen
    Länge (88 statt 56).
  - VIS-Wiederverwendung und VIS-Zwischenstand: der Schlüssel beginnt mit `version=<Fassung>` und einem Zeilenende. Der Leser des
    Zwischenstands meldet bei anderem Fassungsteil "state of another version" (ein Stand von rc611 hat keinen solchen Teil und
    fällt ebenso darunter), bei sonst anderem Schlüssel wie bisher "other vis switches".
  - Für Stände zwischen zwei Releases bleibt die Kennung (`BHLICHTn`, `BHVSTn`) zuständig: kein Baustempel (`__DATE__` ändert
    sich nur, wenn genau diese Datei neu übersetzt wird).
- **P4-4** (Q/vis.cpp, Schlüssel von `<karte>.bhedvis`): `rule=fixed=<Ketten>` bei `-bhedvisfixed`, `rule=exact` bei
  `-bhedvisexact` oder einem Thread (dasselbe Ergebnis), sonst `rule=nrc`. Strenger als der Vorschlag der Prüfung (die
  erlaubte, ein exaktes Ergebnis in NRCs Regel zu übernehmen): ein Regelwechsel rechnet einmal neu, ohne Sonderfall.
  Weitere Schalter geprüft: `-fast -merge -mergeportals -nopassage -passageOnly -nosort -hint`, Fernebene und die beiden
  Ebenen-Schalter stehen schon im Schlüssel; `-saveprt`, `-v`, `-bhedvischeck` ändern das Ergebnis nicht.
- Handbuch (6 Sprachen, `tools/handbuch/inhalt_lcfix.py`, Seite karte_teilbau hinter dem Hinweis zu `-bounce`): nach einem
  Update rechnet der erste Bau VIS und Licht neu; ein VIS wird nur von derselben Regel wiederverwendet; Vertexlicht und Gitter
  nur, wenn sich kein Schattenwerfer geändert hat. Kein neues Bild (kein sichtbarer Unterschied in der Oberfläche).

## Entscheidungen von shank (12.11. spät, `plaene/entscheidungen_1211.md`): B3, B4, B5

- **B4 - ein Licht-Cache gilt nur für den Rechenweg, der ihn angelegt hat.** Eine Grafikkarte ohne double rechnet mit
  float-Näherungen und ist nicht bytegleich (`NOTIZEN_karte8g.md`); ein solcher Cache wurde von einem späteren Prozessor-Bau
  ganz übernommen (und umgekehrt). Jetzt steht der Rechenweg im Kopf (`exact` = Prozessor oder Grafikkarte mit double, gleiche
  Bytes; `GPU without double`), Kennung `BHLICHT9`, Kopf 8 + 32 + 32 + 3 x 16 Bytes. Weil der Weg erst nach der GPU-Wahl
  feststeht, liest `BhedLichtCacheOeffnen` (neu, Q/light.cpp direkt vor LightWorld) den alten Cache; `BhedLichtCacheGeometrie`
  rechnet nur noch die Schlüssel. Meldung `light cache not usable: other computing path (cache: GPU without double, this
  build: exact)`, im Fenster Baufortschritt eigener Grund `KeLcGrundWeg` (6 Sprachen, `tools/i18n_lcfix.py`).
  - Rot (exe_p44, Fall `gpuweg`: erster Bau `-bhedgpubake` mit `BHED_GPUBAKE_OHNE_DOUBLE=1` auf der RTX 4070, Teilbau auf dem
    Prozessor): "geometry and switches unchanged", 11/11 Lightmaps, 1/1 Fläche und Gitter aus dem Cache. Auf der kleinen
    Testkarte sind die float-Bytes zufällig gleich - der Fehler ist die Übernahme selbst.
  - Grün: siehe Schlussrunde. Gegenprobe `gpudouble` (erster Bau auf der Grafikkarte mit double): alles kommt weiter aus dem
    Cache, Teilbau = Vollbau (vor und nach dem Fix grün). Ohne Grafikkarte werden beide Fälle übersprungen (SKIP).
- **B5 - VIS-Zwischenstand einer anderen Fassung: eigener Text.** Der Editor las "state of another version" als "die Datei ist
  beschädigt" (`src/karte/baufortschritt.cpp`, Rückfall `Grund::Beschaedigt`). Jetzt `BauVisStand::Grund::Fassung`, Text
  `KeBpGrundFassung` ("er stammt aus einer anderen Programmversion"; die Vorlage `KeBpVisNicht` sagt schon "das VIS wird von
  vorn gerechnet"). Probe in `tests/kartebau.cpp`: rot mit dem alten Leser (Grund Beschaedigt), grün mit dem neuen.
- **B3 - `q3map_alphashadow` wie Ravens q3map: im Compiler schon so, nichts zu bauen.**
  - Am Code: das Wort fällt in q3map2 in den Zweig "q3map_<surfaceparm>" (`Q/shaders.cpp:1706-1714`, ydnar, so auch in
    NetRadiant-Custom) und setzt über `ApplySurfaceParm( "alphashadow" )` dieselben Flags wie `surfaceparm alphashadow`
    (`C_ALPHASHADOW | C_TRANSLUCENT`; `-game ja` erbt die Liste von sof2, `Q/games.cpp:811`).
  - Gemessen (`Y:\werkzeuge\lcfix\b3_probe.py`, exe_rot = 73c18113f): derselbe Schattenwerfer mit ganz durchsichtiger
    Textur, Shader A `surfaceparm alphashadow`, B `q3map_alphashadow`, C ohne beides: B = A in Vertices, Lightmaps und Gitter,
    B != C. Keine Warnung "Unknown q3map_* directive".
  - Neuer Fall `q3mapalpha` im ctest q3map2_lichtabdruck: Alphakanal einer Textur mit `q3map_alphashadow` 255 -> 0 ändert die
    Lumps 10, 14, 15, 17 (das Wort wirkt) und der Teilbau ist gleich dem Vollbau (das Wort steht im Abdruck: Skripttext in
    `g_geo`, `compileFlags` in der Shader-Signatur). Grün schon vor B4 (exe_p44).
  - Ein Schalter für "das NRC-Verhalten" entfällt: NRC und Ravens q3map behandeln das Wort gleich.
  - Woher der Befund des DXR-Agenten kam (Vermutung, nicht gemessen): `textures/kejim/grate02_broke` steht in den JKA-Daten
    ZWEIMAL - in `shaders/bespin.shader` ohne `trans` und ohne `q3map_alphashadow` (deckend) und in `shaders/imperial.shader`
    mit beidem. q3map2 nimmt bei gleichem Namen die zuerst gelesene Fassung (`ShaderInfoForShader`: `find_first` nach der
    Anlegereihenfolge); ohne shaderlist.txt liest es die Dateien in der Reihenfolge des Dateisystems, bespin also vor
    imperial. Welche Fassung Ravens q3map und der Renderer des Spiels nehmen, ist nicht geprüft. Die Stelle, an der der
    Shader-Leser das Flag setzt, ist für den DXR-Agenten `Q/shaders.cpp:1706-1714` (über `ApplySurfaceParm`).
  - **Berichtigt (GITTER 13.11., `NOTIZEN_gitter_1311.md`):** Die Vermutung trifft nicht zu. Den Namen
    `textures/kejim/grate02_broke` definiert nur `imperial.shader`, in den JKA-Daten wie in denen von Movie Duels;
    `bespin.shader` definiert `textures/bespin/breakable_grate1`, der nur dasselbe Bild benutzt.

## Liste der Eingaben eines Lichtbaus (am Code durchgegangen)

Drei Abdrücke: Lightmap (`BhedLmVorab`, nach Inhalt), Vertexlicht (`BhedFlaecheAusCache`), Gitter (`BhedGitterAusCache`); dazu
im Kopf die Schalter (`g_opt`), die worldspawn (`g_welt`) und die Geometrie (`g_geo`).

| Eingabe | Lightmap | Vertexlicht / Gitter |
|---|---|---|
| BSP-Lumps, .srf | Inhalt je Fläche | `g_geo` |
| Shader-Skripte (auch `name:q3map` ohne Stufen, neu) | Shader-Signatur | `g_geo` |
| Texturpixel alphashadow / Lichtfilter | Strahlbaum | Strahlbaum (neu) |
| Texturfarbe von Flächenlicht und Bounce | Licht-Hash | Licht-Hash |
| Lichter (alle Felder von `light_t`) | Licht-Hash | Licht-Hash |
| worldspawn ohne Nachbearbeitung | `g_welt` | `g_welt`, `g_geo` |
| Brush-Entities | Strahlbaum, Inhalt | `g_geo` |
| Punkt-Entities mit Modell und `_cs`, Modelldatei | Strahlbaum | Strahlbaum (neu) |
| Schalter nach `-light` ohne Nachbearbeitung, `-game` | `g_opt` | `g_opt` |
| `-subdivisions` | `g_opt` (neu), Inhalt, Strahlbaum | `g_opt` (neu), Strahlbaum (neu) |
| Fassung des Compilers | Kopf (neu) | Kopf (neu) |
| `-threads`, `-force`, `-v`, `-connect`, `-maxmapdrawsurfs` | ändern das Licht nicht (Fall `faeden`, ctest q3map2_determinismus) | |
| Nachbearbeitung (`-gamma -exposure -compensate -saturation -contrast -gridscale -gridambientscale`, `_minlight` usw.) | wirkt nur beim Speichern | |
| GPU-Backen | bytegleich mit double; GPU OHNE double nicht: Rechenweg im Kopf (B4, neu) | |

## Zeitkosten (unter Last gemessen, Richtwert)

Messfenster 20:53-20:54 (`mess_start.sh "LCFIX: Abdruck alt/neu"`), `Y:\werkzeuge\lcfix\mess_lcfix.sh` (mess4.py von lcb2):
md_ch_ben, `-fast -patchshadows -samples 2 -filter -bounce 2`, Prozessor, ein Licht dazu. Neu = alle Fixes (exe_b4), alt =
73c18113f (exe_rot). **Fremdlast hoch**: andere Agenten hatten 8-9 MSBuild und 3-7 cl.exe laufen (Bauten, die vor der Sperre
begonnen hatten).

| Lauf | Wand | CPU | aus dem Cache |
|---|---|---|---|
| Vollbau mit Cache, neu (N0) | 6,1 s | 41,8 s | - |
| Vollbau mit Cache, alt (R0) | 9,9 s | 37,6 s | - |
| Teilbau, neu (T) | 10,4 s | 41,8 s | 28/144 Lightmaps, 1/30 Flächen, Gitter gerechnet |
| Teilbau, alt (RT) | 5,4 s | 36,6 s | 28/144 Lightmaps, 1/30 Flächen, Gitter gerechnet |

- Die Abschnitte des Licht-Caches (`BHED_Q3_ZEITEN`: Schlüssel, Inhalt/Spurbaum, Vorab, lesen, packen) liegen alt wie neu je bei
  Hundertsteln bis einer Zehntelsekunde (Summe je Bau 0,2-0,35 s); `BhedSpurGanzSig` hasht zwei schon vorhandene Werte. Die
  Unterschiede der Wand- und CPU-Zeiten gehen in beide Richtungen und kommen von der Fremdlast.
- Gleiche Trefferzahl alt und neu: die Fixes nehmen auf dieser Karte keine Übernahme weg (die Gitter-Übernahme fällt hier schon
  alt weg, weil das neue Licht das Gitter erreicht).
- Vollbau neu gegen Vollbau alt (R0 = N0): .bsp Byte für Byte gleich - die Fixes ändern kein Ergebnis eines Vollbaus.
- Für das Ruhefenster: `Y:\werkzeuge\lcfix\messung_final.sh` (rund 1 Minute, Ergebnis `messung_final.txt`).

## Schlussrunde (Zweig p_lcfix auf karte1 82848b14e)

- Ganzer Bau (`--parallel 2`, Leerlauf-Priorität): 0 Warnungen.
- ctest `q3map2_|karte_bau|karte_grenzen|handbuch|sprache|changelog`: 28/28 grün (u. a. q3map2_lichthash, q3map2_lichtabdruck mit
  17 Fällen, 35 OK, kein SKIP auf der RTX 4070; q3map2_visschluessel 13 ok; q3map2_teilbau, _bounce, _geometrie, _offen,
  q3map2_vispause, q3map2_baupause, q3map2_determinismus, q3map2_gpubake, karte_bau 121/0, karte_grenzen).
- Lints der geänderten Dateien: portability (keine Meldung in geänderten Dateien), sources, feldinit, fassung, i18n,
  i18n_erzeugt, msvclink ok.
- Kein Selbsttest gelaufen: an der Oberfläche ändert sich nur die Wahl eines Textes aus einem geprüften Grund (ctest kartebau);
  keine Zeichnung, kein neues Bedienelement.

## Offen

- Die zwei Fragen (GPU ohne double, Text für den VIS-Zwischenstand) hat shank beantwortet (B4, B5) - umgesetzt.
- B3: doppelte Shader-Namen in den JKA-Daten (z. B. `textures/kejim/grate02_broke`) - erledigt (GITTER 13.11.): Dieser Name
  ist nicht doppelt definiert (siehe B3 oben und `NOTIZEN_gitter_1311.md`).

## Changelog

## English

- Map editor, partial compiles: vertex lit surfaces (models without a lightmap) and the light grid are no longer taken from the light cache when something that casts a shadow has changed - the image of an alphashadow texture, a model with `_cs` on a point entity or its model file, `-subdivisions`, or the script of a `:q3map` shader without stages. Such a build now equals a full build byte for byte.
- The light cache no longer mixes up two worldspawn keys or switches that differ only in the last character of a value with 7, 15, 23 ... characters.
- After an update of JK Studio, the first build computes the light and the VIS again instead of reusing what an older version of bhed_q3map2 made; the build progress window says that the cache comes from an older version.
- A light cache made on a graphics card without double precision is no longer used by a build on the processor, and the other way round - the two do not give the same bytes; the build progress window says so.
- The build progress window says when a saved VIS state comes from another program version (it said the file was damaged).
- "Partial compile: reuse VIS" no longer takes the VIS of another rule: after switching "Same VIS in every build" (or `-bhedvisfixed`) on or off, the next VIS is computed once and the .bsp carries the promised result.

## Deutsch

- Karten-Editor, Teil-Kompilierungen: vertexbeleuchtete Flächen (Modelle ohne Lightmap) und das Lichtgitter kommen nicht mehr aus dem Licht-Cache, wenn sich etwas geändert hat, das Schatten wirft - das Bild einer alphashadow-Textur, ein Modell mit `_cs` an einer Punkt-Entity oder seine Modelldatei, `-subdivisions` oder das Skript eines `:q3map`-Shaders ohne Stufen. Ein solcher Bau ist jetzt Byte für Byte gleich einem vollen Bau.
- Der Licht-Cache verwechselt keine zwei worldspawn-Schlüssel oder Schalter mehr, die sich nur im letzten Zeichen eines Werts mit 7, 15, 23 ... Zeichen unterscheiden.
- Nach einem Update von JK Studio rechnet der erste Bau Licht und VIS neu, statt zu übernehmen, was eine ältere Fassung von bhed_q3map2 angelegt hat; das Fenster Baufortschritt sagt, dass der Cache aus einer älteren Version stammt.
- Ein Licht-Cache, der auf einer Grafikkarte ohne double entstand, wird von einem Bau auf dem Prozessor nicht mehr übernommen und umgekehrt - die beiden ergeben nicht dieselben Bytes; das Fenster Baufortschritt sagt das.
- Das Fenster Baufortschritt sagt, wenn ein VIS-Zwischenstand aus einer anderen Programmversion stammt (bisher hieß es, die Datei sei beschädigt).
- „Teil-Kompilierung: VIS wiederverwenden“ nimmt nicht mehr das VIS einer anderen Regel: nachdem „Dasselbe VIS in jedem Bau“ (oder `-bhedvisfixed`) ein- oder ausgeschaltet wurde, wird das nächste VIS einmal gerechnet, und die .bsp trägt das zugesagte Ergebnis.

## 中文

- 地图编辑器,部分编译:当投射阴影的东西发生变化时——alphashadow 贴图的图像、点实体上带 `_cs` 的模型或其模型文件、`-subdivisions`,或没有阶段的 `:q3map` 着色器的脚本——顶点光照面(没有光照贴图的模型)和光照网格不再取自光照缓存。这样的编译现在与完整编译逐字节相同。
- 光照缓存不再混淆两个只在长度为 7、15、23……个字符的值的最后一个字符上不同的 worldspawn 键或开关。
- JK Studio 更新之后,第一次编译会重新计算光照和 VIS,而不是重用旧版本 bhed_q3map2 生成的结果;编译进度窗口会说明缓存来自旧版本。
- 在不支持 double 的显卡上生成的光照缓存不再被处理器上的编译使用,反之亦然——两者得到的字节并不相同;编译进度窗口会说明这一点。
- 当保存的 VIS 状态来自另一个程序版本时,编译进度窗口会说明(以前显示文件已损坏)。
- “重用 VIS”不再采用另一种规则的 VIS:打开或关闭“每次编译得到相同的 VIS”(或 `-bhedvisfixed`)之后,下一次 VIS 会重新计算一次,.bsp 中是承诺的结果。

## 日本語

- マップエディター、部分コンパイル:影を落とすもの(alphashadow テクスチャの画像、ポイントエンティティの `_cs` 付きモデルやそのモデルファイル、`-subdivisions`、ステージのない `:q3map` シェーダーのスクリプト)が変わったとき、頂点ライティングの面(ライトマップのないモデル)とライトグリッドをライトキャッシュから取らなくなりました。このようなビルドは、フルビルドとバイト単位で同じになりました。
- ライトキャッシュは、長さが 7、15、23… 文字の値の最後の 1 文字だけが違う worldspawn のキーやスイッチを取り違えなくなりました。
- JK Studio を更新したあとの最初のビルドは、古いバージョンの bhed_q3map2 が作ったものを再利用せず、ライトと VIS を計算し直します。ビルド進行状況のウィンドウに、キャッシュが古いバージョンのものだと表示されます。
- double 非対応のグラフィックカードで作ったライトキャッシュは、プロセッサーでのビルドでは使わなくなりました(逆も同じ)。両者は同じバイトにならないためです。ビルド進行状況のウィンドウにそう表示されます。
- 保存した VIS の状態が別のプログラムバージョンのものなら、ビルド進行状況のウィンドウにそう表示されます(これまではファイルが壊れていると表示されていました)。
- 「VIS を再利用」は、ほかの規則の VIS を使わなくなりました。「毎回のビルドで同じ VIS」(または `-bhedvisfixed`)をオンまたはオフにしたあとの次の VIS は一度計算し直し、.bsp には約束どおりの結果が入ります。

## Español

- Editor de mapas, compilaciones parciales: las superficies con iluminación por vértice (modelos sin lightmap) y la rejilla de luz ya no se toman de la caché de luz cuando ha cambiado algo que proyecta sombra: la imagen de una textura alphashadow, un modelo con `_cs` en una entidad de punto o su archivo de modelo, `-subdivisions` o el script de un shader `:q3map` sin etapas. Una compilación así es ahora byte a byte igual a una compilación completa.
- La caché de luz ya no confunde dos claves de worldspawn o interruptores que solo se diferencian en el último carácter de un valor de 7, 15, 23... caracteres.
- Después de actualizar JK Studio, la primera compilación vuelve a calcular la luz y el VIS en lugar de reutilizar lo que creó una versión anterior de bhed_q3map2; la ventana de progreso de compilación indica que la caché procede de una versión anterior.
- Una caché de luz creada en una tarjeta gráfica sin doble precisión ya no la usa una compilación en el procesador, ni al revés: las dos no dan los mismos bytes; la ventana de progreso de compilación lo indica.
- La ventana de progreso de compilación indica cuando un estado de VIS guardado procede de otra versión del programa (antes decía que el archivo estaba dañado).
- "Reutilizar VIS" ya no toma el VIS de otra regla: después de activar o desactivar "VIS igual en cada compilación" (o `-bhedvisfixed`), el siguiente VIS se calcula una vez y el .bsp lleva el resultado prometido.

## Čeština

- Editor map, dílčí kompilace: plochy osvětlené po vrcholech (modely bez lightmapy) a mřížka světla se už neberou z mezipaměti světla, když se změnilo něco, co vrhá stín - obrázek textury alphashadow, model s `_cs` u bodové entity nebo jeho soubor modelu, `-subdivisions` nebo skript shaderu `:q3map` bez fází. Takové sestavení je nyní bajt po bajtu stejné jako plné sestavení.
- Mezipaměť světla už nezamění dva klíče worldspawn nebo přepínače, které se liší jen posledním znakem hodnoty o délce 7, 15, 23... znaků.
- Po aktualizaci JK Studia první sestavení znovu spočítá světlo a VIS, místo aby převzalo, co vytvořila starší verze bhed_q3map2; okno průběhu sestavení oznámí, že mezipaměť pochází ze starší verze.
- Mezipaměť světla vytvořenou na grafické kartě bez double už nepoužije sestavení na procesoru a naopak - obě nedávají stejné bajty; okno průběhu sestavení to oznámí.
- Okno průběhu sestavení oznámí, když uložený stav VIS pochází z jiné verze programu (dosud hlásilo poškozený soubor).
- „Znovu použít VIS“ už nepřevezme VIS jiného pravidla: po zapnutí nebo vypnutí „Stejné VIS při každém sestavení“ (nebo `-bhedvisfixed`) se další VIS jednou spočítá a .bsp nese slíbený výsledek.
