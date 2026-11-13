# NOTIZEN DXR (12.11.2026): Raytracing-Schatten im Echtzeit-Licht des Karten-Editors

Auftrag: `JKStudio_Uebergabe/auftraege/welle_1211.md` Abschnitt 2. Zweig `p_dxr`, Worktree `Y:\p_ap8`, Werkzeuge
`Y:\werkzeuge\dxr`. Begonnen von DXR (Fable 5.2), fortgesetzt von DXR (Opus 5.5) nach dem Modellwechsel.

## Was sich aendert

- **Build > Ray-traced shadows (DXR)** (nach "Patch shadows"): zweite Schattenart des Echtzeit-Lichts. Vorgabe aus =
  Schattenkarten wie bisher; gespeichert als `karte=BhedRaytracedShadows`. An: je Licht und Bildpunkt ein Schattenstrahl
  (RayQuery, DXR 1.1, Shader Model 6.5) gegen Beschleunigungsstrukturen der Schattengeometrie.
- Geht nur unter Direct3D 12 mit DXR 1.1. Sonst ist der Punkt ausgegraut, der Tooltip nennt den Grund (Direct3D 11,
  Grafikkarte ohne DXR 1.1, Bau ohne dxc, Fehler beim Bau). Die Einstellung bleibt gemerkt.
- Strahlen wie q3map2 -light: Ursprung = Stelle + Normale x 1,0 (`DEFAULT_LIGHTMAP_SAMPLE_OFFSET`, q3map2.h:169), TMin 1,5
  (`DEFAULT_INHIBIT_RADIUS`, q3map2.h:161), Sonnenstrahl 524288 lang (light.cpp:116), hell nur bei Treffer einer
  Himmelsflaeche von vorn. alphashadow x (255 - Alpha)/255, lightfilter x Farbe, Schichten multipliziert
  (light_trace.cpp:1325-1364). Sonnenproben (`q3map_sunExt` samples) in ihrer eigenen Richtung.
- Strukturen: je Eimer der Schattengeometrie (512er Zelle x Art x Shader) eine untere Struktur, Neubau nur bei anderer
  Pruefsumme; ein Eimer, der sich binnen 0,5 s wieder aendert, wird schnell gebaut (PREFER_FAST_BUILD) und nach 0,5 s Ruhe
  einmal auf schnelle Strahlen nachgezogen. Seiten zu 4 MiB.
- Mit Strahlen gibt es keine Schattenwuerfel und keine Sonnenkarten (Grafikspeicher frei, kein Licht wartet).
- Ein Shadertext: `kShader4` bleibt die Quelle; `tools/dxr/dxr_quelle.cpp` setzt `gui/karte_dxr.hlsl` davor, dxc (Windows
  SDK) baut DXIL. Die uebrigen Shader bleiben, wie sie sind.

## Eingriffsstellen ausserhalb der eigenen Dateien

- Abgeber: nur `abgeber::strukturSicht(D3D12_GPU_VIRTUAL_ADDRESS)` (`gui/abgeber12.h`, `gui/abgeber12_geraet_win32.cpp`).
- `gui/karte_gpu_win32.cpp`: Include; `#ifdef DXR_SCHATTEN` in `lichtVon`; `dxr::lichtBinden` nach `PSSetSamplers` im Zweig
  Echtzeit-Licht; `dxr::loesen()` in `echtLichtLoesen`.
- `gui/app_karte_echtlicht.cpp` (TEMPO ordnet hier um): zwei Felder in `SchattenGeo`, Block am Ende von `schattenGeoBauen`,
  `wuerfelAus`, in `echtLichtBildVorbereiten` Wahl/Neubau-Bedingung, `bildSetzen`/`wuerfelAus`, Budgets, zwei Bedingungen;
  Hinweis in `echtLichtHinweis`; eine Zeile in `echtLichtSchritt`.
- Klein: `gui/karteneditor.h`, `gui/karte_echtlicht.h`, `gui/app_karte_menue.cpp`, `gui/app_karte_bau.cpp`,
  `gui/selbsttest_handbuch.inc`, `src/hilfe.cpp`, `CMakeLists.txt` (Block am Ende), `tools/gen_i18n.py`,
  `tools/i18n_sprache_es.py`/`_cs.py`, `tools/handbuch/gen_handbuch.py`, `inhalt_karte7.py`, `inhalt_es.py`, `inhalt_cs.py`.
- Eigene Dateien: `gui/karte_dxr.h`, `gui/karte_dxr_win32.cpp`, `gui/karte_dxr.hlsl`, `gui/app_karte_dxr.cpp`,
  `gui/selbsttest_dxr.inc`, `tools/dxr/dxr_quelle.cpp`, `tools/i18n_dxr.py`, `tools/handbuch/inhalt_dxr.py`.

## Zusaetzlich (rc612, aus der Arbeit an DXR)

- **Filterbild fehlte beim Bau (beide Schattenarten):** Ein alphashadow-/lightfilter-Shader, dessen Bild beim Bau der
  Schattengeometrie noch nicht geladen war, wurde deckend gebaut, und nichts baute neu, wenn das Bild kam
  (`kameraTexturBild` liefert bis dahin nullptr). Jetzt merkt sich der Bau diese Shader (`SchattenGeo::bildFehlt`) und baut neu,
  sobald eines geladen ist (`schattenBildDa`). Das Bild laedt auch, wenn die Kamera die Flaeche nie sieht (die Schattengeometrie
  fordert es an).
- **B3 - `q3map_<surfaceparm>` im Echtzeit-Licht:** q3map2 nimmt jedes `q3map_<x>`, das `ParseShaderFile` nicht selbst kennt,
  als `surfaceparm <x>` (shaders.cpp:1706-1714, `ApplySurfaceParm( &token[ 6 ] )`). 53 Shader von Jedi Academy schreiben
  `q3map_alphashadow` (z. B. `textures/kejim/grate02_broke`). `shaderSkriptLesen` (src/karte/optimierer_pruefen.cpp), aus dem
  das Echtzeit-Licht liest, kannte die Regel nicht: Diese Gitter warfen im Echtzeit-Licht keinen Schatten, im gebauten Licht
  schon. Jetzt gilt dieselbe Regel, mit der Liste der ausdruecklich behandelten Woerter aus shaders.cpp (ausgenommen, z. B.
  `q3map_terrain`). Eine eigene Schalter-Stelle mit LCFIX gibt es nicht mehr (Hauptchat 12.11.: kein NRC-Schalter noetig).
  - Meine erste Aussage "q3map2 kennt das Wort nicht" war falsch (Suche nach der Zeichenfolge statt nach der Regel). LCFIX hat
    sie widerlegt; nachgemessen mit `Y:\werkzeuge\dxr\b3lm.py` (bhed_q3map2 -meta/-light -fast, Testraum mit Gitter):
    Lightmap-Mittel Testgitter (`surfaceparm alphashadow`) 4,451, JKA-Gitter (`q3map_alphashadow`) 4,451, deckend 4,081 -
    unter Movie Duels und unter Original-JKA gleich.
  - **Doppelte Definition:** `textures/kejim/grate02_broke` steht in bespin.shader (ohne surfaceparm, ohne alphashadow) und
    in imperial.shader (nonopaque, trans, q3map_alphashadow). q3map2 nimmt die zuerst gelesene (`find_first`), die Dateien in
    der Reihenfolge der shaderlist.txt bzw. von `vfsListShaderFiles`. Editor (`q3map2ShaderLesen`, dieselbe Regel) und
    Compiler nehmen hier dieselbe Fassung: Der Editor liest die imperial-Fassung (Selbsttest B3), der Compiler macht aus dem
    Gitter einen Detail-Pinsel (`-meta -v`: 1 detail brush; mit der bespin-Fassung waere er strukturell) und filtert das Licht.
    Kein Fund. Nicht geprueft: alle doppelten Shader einzeln.
  - Welche Fassung Ravens q3map bei doppelten Shadern nimmt, ist offen (nicht Auftrag, nur notiert).
  - **Berichtigt (GITTER 13.11., `NOTIZEN_gitter_1311.md`):** `textures/kejim/grate02_broke` ist nicht doppelt definiert
    (JKA- und Movie-Duels-Daten). `bespin.shader` definiert `textures/bespin/breakable_grate1`, der nur dasselbe Bild
    benutzt; der Absatz oben beruht auf einer Suche nach dem Bildnamen statt nach dem Shadernamen.

## Rot / Gruen je Punkt

Laeufe in `Y:\werkzeuge\dxr\laeufe`, Selbsttest `karteneditor` mit `BHED_KARTE_NUR=dxr`, unter Leerlauf-Prioritaet.

| Punkt | Rot | Gruen |
|---|---|---|
| Schattengrenze gleiche Stelle, schaerfer | - | g2/g3/g6: Karten Ort 542,61 Breite 11,33 Zeilen, Strahlen 540,54 / 0,74; gemessener Massstab 5,437 Zeilen je Einheit -> Abstand 0,38 Einheiten, Breite 2,08 gegen 0,14 Einheiten. Gegen gebacken: halbe Helligkeit 7,56 Einheiten daneben (unter einem halben Luxel) |
| Uebersicht Strahlen = Karten | g2 rot: geratene Schwelle (Helligkeit > 10 bei 7,90) | Block-Mittel 0,01, Block-Max 0,78; Schwelle aus der Messung begruendet |
| Sonne und Himmelslicht | g1: Testfehler (Schwelle) | Block-Mittel 0,05; gegen gebacken gleich gut (2,88 / 2,88) |
| Filterbild fehlte beim Bau | g3 (ohne p07): Boden unter dem Gitter mit Strahlen 0,0 bei frei 52,2, 0 Filter-Teile | g4/g6: 4 Filter-Teile, Strahlen 4,90, Schattenkarten 5,05 |
| B3 q3map_alphashadow | g6: Boden unter dem JKA-Gitter 52,15 = frei, 0 Filter-Teile (Strahlen und Karten) | g8 (Endstand n8): JKA-Gitter Strahlen 4,90 (4 Filter-Teile), Karten 5,05 - gleich dem Testgitter mit surfaceparm alphashadow |
| Bewegte Teile | g4 rot: Testfehler unter Leerlauf-Prioritaet (Schritte > 0,5 s auseinander) | Erwartung aus gemessenen Abstaenden; g6: schnell +3 (Abstaende 0,363 / 0,021 / 0,016 s), nach der Ruhe +1 nachgezogen; neuer Pinsel baut 1 von 40 Strukturen |
| Debugschicht | g2: "Ignoring InitialState UNORDERED_ACCESS" (Kratzpuffer) | g6: Warnung weg (Puffer in COMMON), Fehler der Debugschicht 0 |
| Rueckfall | frot (Geraet gespielt, ohne Erwartung): 22 OK / 12 FEHLER - wie gewollt | f11 (Direct3D 11) 20/0, fger (kein DXR) 21/0, fsha (ohne Shader) 21/0, fbau (Bau scheitert) 21/0: jeweils 0 verschiedene Bildpunkte gegen die Vorgabe, ausgegraut, Grund im Tooltip |
| Aktionssuche | - | Helfer findet `menue:KeLBuild>KeStrahlenMenue` (g6) |

Rueckfall-Laeufe auf exe n3 (vor dem Rebase); der Rueckfall-Code hat sich danach nicht geaendert.

## Messwerte (Richtwerte unter Last, Leerlauf-Prioritaet)

- Grafikspeicher Testraum: Strahlen 4372 KB (davon 4116 KB Strukturen, fast nur die erste 4-MiB-Seite) gegen 12288 KB
  Schattenwuerfel; mit Strahlen 0 Wuerfel.
- Bildzeit: nicht gemessen (Lastregel 12.11. abends). `Y:\werkzeuge\dxr\messung_final.sh` misst im Ruhefenster
  Schattenkarten gegen Strahlen auf Taris (leistung, Abschnitt e, BHED_DXR=0/1, 3 Runden) und vergleicht die Fotos der
  Schattenkarten-Runde pixelgenau mit `basis\f_l12` ("kein Bild aendert sich mit der Vorgabe"). Ergebnis in
  `messung_final.txt`. Laufzeit etwa 9 Minuten.

## Gelaufen am Schluss

- Bau n11 (`alle`, nach dem Rebase auf karte1 fe0cbece7, `--parallel 2`, Leerlauf): 0 Fehler, 0 Warnungen (/W4 /WX); ebenso
  vorher n8 (Basis 499527165) und n9 (133c47078).
- Selbsttest `karteneditor`, `BHED_KARTE_NUR=dxr` (Direct3D 12): g11 auf n11 35 OK / 0 FEHLER (g8 auf n8, g9 auf n9 ebenso). Rueckfall (Direct3D 11, ohne DXR, ohne
  Shader, Bau scheitert) auf n3: je 0 FEHLER, Gegenprobe rot (s. o.).
- Gruppen, die das Echtzeit-Licht beruehren, unter Direct3D 12 (`BHED_KARTE_GRUPPEN=stufe9c,stufe9d,licht1`, grp8): 44 OK /
  0 FEHLER (grp11 auf n11, vorher grp8 auf n8 ebenso). Unter Direct3D 11 nicht gelaufen: der Vorgabeweg zeichnet dort wie vorher (DXR gibt es nur unter Direct3D 12), f11
  belegt Bildpunkt-Gleichheit mit der Vorgabe.
- Bewusst nicht: r3, rd (Spielbild/Rendern, nicht vom Echtzeit-Licht-Schattenweg beruehrt), volle Runde (macht der Hauptchat).
- Handbuch (auf n8): `BHED_HB_NUR=dxr` in 6 Sprachen je 2 OK / 0 FEHLER, 18 Bilder 2538 x 1472 (angesehen: en Karten/Strahlen/Menue,
  de und ja Menue - nichts abgeschnitten), gen_handbuch, gen_hilfe_orte.
- Lints: wie der Grundstand (efxflags, fremdprogramm 1, portability 101, pruefcodes 11 - alle in fremden Dateien); i18n,
  d3dbesitz, settings, headers ok.
- ctest (`-j 4`, Leerlauf): n9 265 von 265 bestanden (n8 ebenso; auf n11 nicht wiederholt), darunter dxr_quelle, dxr_quelle_fehlt (WILL_FAIL), hilfe, hilfe_orte.

## Offen

- **Gitter gegen gebacken - erledigt (GITTER 13.11., `NOTIZEN_gitter_1311.md`):** Ravens q3map liess das Licht ebenso
  durch die Staebe von `alphaFunc LT128` (Original vjun2: 61 dunkle Inseln am Boden, 1 helle; erwartet 134/0 fuer q3map2s
  Regel, 0/134 fuer die umgekehrte). q3map2 und beide Echtzeit-Arten haben dieselbe Richtung, also nichts zu aendern.
  Die 24 bis 26 % sind ein Zufall der Mittelwerte: Die Lightmap ist unter dem Gitter im Blickfeld ueberall 0 (ein Strahl je
  Luxel von 16 Einheiten trifft die 3 bis 4 Einheiten breiten Lichtlinien nicht), und ihr Mittel kommt aus der bilinearen
  Rampe des naechsten freien Luxels am oberen Bildrand. Die Lightmap "verwischt" das Muster also nicht.
- Pixelvergleich der Vorgabe gegen einen Bau von karte1: nur im Ruhefenster (`messung_final.sh`). Belegt bisher: der
  Vorgabeweg ist im Code unveraendert (Strahlen nur bei Wahl), Rueckfall-Laeufe Bildpunkt fuer Bildpunkt gleich,
  `leistung_editor_vorlage_spiel` von n4 gleich der Basis.
- lightfilter (Farbe des Texels) und mehrere Filterbilder im Atlas: kein eigener Test.
- Rechtsklick: Die Build-Schalter haben keinen; Aktionssuche = Ort im Helfer.
- Patches mit "Patch shadows": Das gezeichnete Netz weicht vom Netz von -light ab; bei Versatz 1,0 sind Selbstschatten moeglich.
- Lehnt der Abgeber den Pipeline-Zustand ab, merkt die Schattenart es nicht (nur im Protokoll).
- "Pruefung beim Start" geschieht beim ersten Gebrauch (Build-Menue offen oder Einstellung an).
- Im Testfoto `dxr_menue_grund` (f11) ist das Build-Menue nicht offen (foto() statt hb::fotoTeil); die Pruefung findet den
  Punkt trotzdem. Im Handbuchbild h_dxr_menue ist es offen (hb::fotoTeil).

## Fragen an shank

- Keine offenen (die Frage zu q3map_alphashadow hat sich erledigt: q3map2 wertet es schon).

## Changelog

## English

- Map editor, real-time light: **Build > Ray-traced shadows (DXR)** - a second shadow type next to the shadow maps. With Direct3D 12 and a graphics card with DirectX Raytracing 1.1 every light sends one shadow ray per pixel: the shadows lie at the same place as with the shadow maps and as in the built lightmap, but with a sharp edge and without steps, no light waits for its shadow map, and alphashadow/lightfilter shaders let light through as in q3map2. Off by default; without DXR the item is greyed out and says why.
- Real-time light: a grate or leaf shader whose texture had not loaded yet when the shadow geometry was built stayed a solid shadow until the next edit - it is rebuilt as soon as the texture is there (both shadow types).
- Real-time light: grates of Jedi Academy that write `q3map_alphashadow` (instead of `surfaceparm alphashadow`) cast no shadow in the real-time light, while the built light filtered them - the real-time light now reads every `q3map_<surfaceparm>` like q3map2 does, with both shadow types.

## Deutsch

- Karten-Editor, Echtzeit-Licht: **Build > Raytracing-Schatten (DXR)** - eine zweite Schattenart neben den Schattenkarten. Mit Direct3D 12 und einer Grafikkarte mit DirectX Raytracing 1.1 schickt jedes Licht je Bildpunkt einen Schattenstrahl: Die Schatten liegen an derselben Stelle wie mit den Schattenkarten und wie in der gebauten Lightmap, aber mit scharfer Kante und ohne Stufen, kein Licht wartet auf seine Schattenkarte, und alphashadow-/lightfilter-Shader lassen Licht durch wie in q3map2. Ab Werk aus; ohne DXR ist der Punkt ausgegraut und nennt den Grund.
- Echtzeit-Licht: Ein Gitter- oder Blätter-Shader, dessen Textur beim Bau der Schattengeometrie noch nicht geladen war, blieb bis zur nächsten Änderung ein voller Schatten - er wird neu gebaut, sobald die Textur da ist (beide Schattenarten).
- Echtzeit-Licht: Gitter von Jedi Academy, die `q3map_alphashadow` schreiben (statt `surfaceparm alphashadow`), warfen im Echtzeit-Licht keinen Schatten, während das gebaute Licht sie filterte - das Echtzeit-Licht liest jetzt jedes `q3map_<surfaceparm>` wie q3map2, mit beiden Schattenarten.

## 中文

- 地图编辑器实时光照:**Build > 光线追踪阴影(DXR)**——阴影贴图之外的第二种阴影类型。在 Direct3D 12 和支持 DirectX Raytracing 1.1 的显卡上,每个光源为每个像素发出一条阴影光线:阴影位置与阴影贴图及烘焙光照贴图相同,但边缘锐利、没有锯齿,没有光源需要等待阴影贴图,alphashadow/lightfilter 着色器像 q3map2 那样透光。默认关闭;没有 DXR 时该项变灰并说明原因。
- 实时光照:构建阴影几何时纹理尚未加载的格栅或树叶着色器,在下次编辑前一直是实心阴影——现在纹理一到就会重新构建(两种阴影类型)。
- 实时光照:写 `q3map_alphashadow`(而不是 `surfaceparm alphashadow`)的 Jedi Academy 格栅在实时光照中不投射阴影,而烘焙光照会过滤它们——现在实时光照像 q3map2 一样读取每个 `q3map_<surfaceparm>`,两种阴影类型都适用。

## 日本語

- マップエディターのリアルタイムライト:**Build > レイトレーシングの影(DXR)**——シャドウマップに並ぶ 2 つ目の影の種類です。Direct3D 12 と DirectX Raytracing 1.1 対応のグラフィックカードでは、各ライトがピクセルごとにシャドウレイを飛ばします。影はシャドウマップや焼き込んだライトマップと同じ位置にできますが、輪郭はくっきりして段差がなく、シャドウマップを待つライトもなく、alphashadow/lightfilter のシェーダーは q3map2 と同じように光を通します。既定ではオフ。DXR がないと項目は灰色になり理由を表示します。
- リアルタイムライト:影のジオメトリを作った時点でテクスチャがまだ読み込まれていなかった格子や葉のシェーダーは、次の編集まで完全な影のままでした。テクスチャが届いた時点で作り直します(両方の影の種類)。
- リアルタイムライト:`surfaceparm alphashadow` の代わりに `q3map_alphashadow` と書かれた Jedi Academy の格子は、焼き込んだライトではフィルターされるのにリアルタイムライトでは影を落としませんでした。リアルタイムライトは q3map2 と同じくすべての `q3map_<surfaceparm>` を読むようになりました(両方の影の種類)。

## Español

- Editor de mapas, luz en tiempo real: **Build > Sombras con trazado de rayos (DXR)**, un segundo tipo de sombra junto a los mapas de sombras. Con Direct3D 12 y una tarjeta gráfica con DirectX Raytracing 1.1, cada luz lanza un rayo de sombra por píxel: las sombras quedan en el mismo sitio que con los mapas de sombras y que en el lightmap compilado, pero con borde nítido y sin escalones, ninguna luz espera a su mapa de sombras y los shaders alphashadow/lightfilter dejan pasar la luz como en q3map2. Desactivado por defecto; sin DXR la opción aparece en gris e indica el motivo.
- Luz en tiempo real: un shader de rejilla u hojas cuya textura aún no estaba cargada al construir la geometría de sombras seguía siendo una sombra sólida hasta la siguiente edición; ahora se reconstruye en cuanto llega la textura (ambos tipos de sombra).
- Luz en tiempo real: las rejillas de Jedi Academy que escriben `q3map_alphashadow` (en lugar de `surfaceparm alphashadow`) no proyectaban sombra en la luz en tiempo real, mientras que la luz compilada las filtraba; ahora la luz en tiempo real lee cada `q3map_<surfaceparm>` como q3map2, con ambos tipos de sombra.

## Čeština

- Editor map, světlo v reálném čase: **Build > Stíny sledováním paprsků (DXR)** - druhý typ stínů vedle stínových map. S Direct3D 12 a grafickou kartou s DirectX Raytracing 1.1 vyšle každé světlo pro každý pixel stínový paprsek: stíny leží na stejném místě jako se stínovými mapami a jako v sestavené lightmapě, ale s ostrou hranou a bez schodů, žádné světlo nečeká na svou stínovou mapu a shadery alphashadow/lightfilter propouštějí světlo jako v q3map2. Ve výchozím stavu vypnuto; bez DXR je položka šedá a uvádí důvod.
- Světlo v reálném čase: shader mřížky nebo listí, jehož textura ještě nebyla načtená při stavbě stínové geometrie, zůstával až do další úpravy plným stínem - nyní se přestaví, jakmile textura dorazí (oba typy stínů).
- Světlo v reálném čase: mřížky Jedi Academy, které píší `q3map_alphashadow` (místo `surfaceparm alphashadow`), ve světle v reálném čase nevrhaly stín, zatímco sestavené světlo je filtrovalo - světlo v reálném čase nyní čte každé `q3map_<surfaceparm>` jako q3map2, s oběma typy stínů.
