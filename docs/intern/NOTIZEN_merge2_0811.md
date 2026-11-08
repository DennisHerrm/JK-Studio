# Notizen MERGE2: mr4-Rest nach karte1, dann Reste (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 954f182c3)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `m2_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\mg2\` (Testkarte p27e und Spielfotos in `Y:\werkzeuge\mg2\p27e\`).

## Teil 1: Merge mr4 bis f368e49d2

- `git merge f368e49d2` (Merge-Commit b0681a47f). Konflikte nur in den erzeugten Suchdateien `handbuch/*/suche.js` (6 Sprachen);
  neu erzeugt mit gen_i18n, gen_handbuch, gen_hilfe_orte - die uebrigen erzeugten Dateien waren danach bytegleich zum Merge.
- `gui/app_menue.cpp`, `gui/textumbruch.cpp`: nur mr4 hatte sie geaendert. `gui/gpumap_win32.cpp`: karte1 (FIX2/FIX3 turb-Ring)
  und mr4 (DXT-Upload) an getrennten Stellen, automatisch gemischt. `tools/gen_i18n.py`/`src/i18n.cpp`: karte1 haengte
  i18n_fix2/i18n_ptfix3 an, mr4 aenderte EntDuplicate (zh) - beides behalten.
- Gruen nach dem Merge (inkrementeller Bau 0 Warnungen): ctest 240/240, menueeditor en 2787/0, ja 2787/0, cjkweit ja 18/0,
  modellblock pt 41/0, ptfix3 12/0, figurlicht 8/0, karteneditor rd 21/0, r3 22/0, Modus 1 2635/0 (`m2_t1_*`); Lints wie die
  Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, cjkumbruch 0), Handbuch `--pruefen` ok.

## Teil 2

### Punkt 1: textureCandidates wie R_LoadImage

- `textureCandidates` (Vorschau, Karten-Editor-Bilder, Menue-Editor, Wetter) liefert jetzt genau die Folge von R_LoadImage
  (rd-common/tr_image_load.cpp:106-140 OpenJK, MD9 :678-716 gleich, R_ImageLoader_Init :90-98 jpg/png/tga): die genannte
  Endung nur, wenn es fuer sie einen Leser gibt (FindImageLoader mit Q_stricmp), dann jpg, png, tga ohne die schon versuchte,
  am Namen ohne Endung (COM_GetExtension/COM_StripExtension, q_shared.cpp:53-77). Vorher kam jede Endung zuerst (auch `.jpeg`,
  `.bmp`) und dazu `.jpeg` - die laedt das Spiel nie; `y.tga` stand doppelt in der Liste.
- `bildDateienWieEngine` (Painting, PTFIX3 P29) war dieselbe Regel ein zweites Mal - entfernt, das Painting nimmt
  textureCandidates.
- ctest `bilder` (imagetest): rot 5 FEHL (`ct_rot.txt`: y.tga-Folge, ohne Endung, y.jpeg, y.PNG, Punkt im Ordnernamen) ->
  gruen; Gegenlaeufe modellblock ptfix3 12/0, pt 41/0, karteneditor rd 21/0, r3 22/0, menueeditor en (`m2_g1_*`).

### Punkt 2: applySkin bei doppelten Hautzeilen wie die Engine

- RE_RegisterIndividualSkin haengt jede Zeile an die Haut (tr_skin.cpp:286-334 OpenJK), RenderSurfaces nimmt den ERSTEN
  Eintrag einer Flaeche (tr_ghoul2.cpp:2279-2289, break), G2API_SetSurfaceOnOffFromSkin schaltet bei JEDER Zeile `*off` ab
  (tr_ghoul2.cpp:2207-2234, ohne break), beide Namen klein (Q_strlwr). applySkin nahm die letzte Zeile, ein spaeteres Bild
  schaltete `*off` wieder an, und Grossbuchstaben im Flaechennamen der Haut trafen nicht.
- Dreiteilige Haeute `|kopf|rumpf|beine` (hautAusDreiTeilen): ein applySkin ueber Kopf, Rumpf (wenn anders), Beine (wenn anders)
  wie RE_RegisterSkin (tr_skin.cpp:413-424) statt drei Aufrufen rueckwaerts - so gilt auch `*off` aus jedem Teil.
- In den Spieldaten 32 Haeute mit doppelten Zeilen und anderem Shader (`Y:\werkzeuge\mg2\hautdoppelt.txt`), u. a.
  reborn/model_blue (head_eyes_mouth: reborn/ statt kyle/mouth_eyes), tarados_gon/model_robed (hips erst `*off`, dann ein Bild:
  bleibt aus), malak/model_default (head: headsith statt headmask), jedi_spanki*/torso_g3.
- ctest `modell` (glmtest): rot 3 FEHL (erste Zeile, `*off` vor Bild, Grossschreibung) -> gruen.

### Punkt 3: tcGen environment (und Glanz) skalierter Figuren im Raum der Entity

- RB_CalcEnvironmentTexCoords (tr_shade_calc.cpp:902-931 OpenJK) bekommt tess.xyz/normal im Raum der Entity und
  backEnd.ori.viewOrigin = A^T delta / |axis[0]| (R_RotateForEntity, tr_main.cpp:384-403 MD9); die Achsen tragen den
  Massstab (ScaleModelAxis, cg_ents.cpp:332-351 MD9). Bei einer skalierten Figur ist das nicht dasselbe wie in
  Weltkoordinaten: der Blick ist R^T (Auge - Ursprung) - diag(1/s) R^T (Ecke - Ursprung).
- Figur-VS: Glanz (PTFIX3) und Umgebung rechnen jetzt gemeinsam in den Achsen der Entity (gAchse, ohne den Zeichnerausgleich),
  Ursprung und Massstab je Achse kommen ueber FigurLicht (`entUrsprung`, `entSkala`) in die freien Plaetze von b3 (gAchse0..2.w,
  gAuge.w, gStufeTurbQ4.zw) - damit auch fuer Griffe in der Hand einer skalierten Figur die der Figur. Unskaliert bleibt der
  alte Weg bildpunktgleich (figurlicht 8/0).
- Testkarten `p27e` (Protokolldroide, Shader c3po_* mit `tcGen environment` gfx/effects/chr_inv, scale 100/150) und `p27f`
  (scale 300, Kamera 160 60 150 nah davor), Movie Duels per Popen gegen Selbsttest `geschuetze`, Messung `figur.py`
  (mittlere Abweichung der Bildpunkte der Figur, von 255):

| Szene | Figur | ohne (rot) | mit (gruen) |
|---|---|---|---|
| p27e (500 Einheiten) | A scale 100 | 0,65 (`m2_p27e_vor_rot`, alte exe) | 0,65 |
| p27e | B scale 150 | 1,15 | 1,15 (rot gegen gruen nur 0,04 - der Unterschied (s-1)*Ecke ist bei 500 Einheiten klein) |
| p27f (nah) | B scale 300, ganze Figur | 2,31 (`m2_p27f_vor_rotenv`: Zweig abgeschaltet) | 2,01 (`m2_p27f_vor_gruen`) |
| p27f | die 11051 Bildpunkte, die sich durch die Umgebung aendern | 15,2 | 9,5 |

- Der Rest (9,5 an den Chrom-Bildpunkten) liegt an Kanten und Glanzspitzen; nicht weiter verfolgt. Ein zweiter Rotlauf von p27e
  mit abgeschaltetem Zweig war ungueltig (der Selbsttest bindet alle .pk3 des Ordners ein, zz_p27f.pk3 brachte scale 300 mit).

- ctest `gpushader`: Quelltext-Pruefzeilen fuer Glanz und Umgebung im Raum der Entity (rot 2 FEHL -> gruen).

### Punkt 4: p27g Restabweichung der kleinen Figur - Ursache Detailstufe (G2_ComputeLOD), angeglichen

- Ursache belegt: nicht Mip/Filter, sondern die Detailstufe. G2_ComputeLOD (tr_ghoul2.cpp:853-917 MD9) nimmt
  ProjectRadius(0.75 * largestScale * ent->e.radius) mit r_lodscale 10; ent->e.radius ist s.radius (CG_SetGhoul2Info,
  cg_ents.cpp:287-293 MD9, gleich nach dem memset in CG_Player) = 40 aus mins/maxs. Die Figur mit scale 70 in ~500 Einheiten
  bekommt Stufe 1. Gegenprobe im Spiel mit `r_lodscale 20` (erzwingt Stufe 0): die kleine Figur 48,8 -> 52,7, die Vorschau
  zeigte 53,0; die grosse Figur A blieb 54,1 (Stufe 0 in beiden Laeufen).
- Behebung: die Mission-Vorschau laedt alle Detailstufen der Figurmodelle (Haut und Ghoul2-Zustand aus Stufe 0) und waehlt je
  Bild die Stufe wie G2_ComputeLOD (`figurNetzImBild`, Radius nach G_SetG2PlayerModelInfo/NPC_ParseParms: CLASS_SAND_CREATURE
  256, CLASS_RANCOR 300/150, "height", sonst groesster Betrag von mins/maxs mit "width", 0 -> 60) - Koerper, Umriss, Schatten
  und Nebel nehmen dasselbe Netz.
- p27g (Spiel 1920 x 1440): Figur B (scale 70) Spiel 48,8, Vorschau vorher 53,0 (Abweichung 0,44), jetzt 49,5 (0,13); Figur A
  54,1 / 53,7 unveraendert (0,62).
- Derselbe Irrtum im Model-Tab ("automatisch wie im Spiel"): die Grenzen nahmen fuer SP-Figuren Radius 0 an (MB1 las nur das
  memset, nicht CG_SetGhoul2Info) - im Einzelspieler waere es immer Stufe 1 gewesen. Jetzt Radius 40 fuer JKA SP, JK2 SP
  (codeJK2 cg_ents.cpp:215-219, g_client.cpp:1236-1270) und Movie Duels SP; Tooltip MbLodAutoHint und Handbuch (6 Sprachen)
  berichtigt. ctest `modell_dokument` rot 1 FEHL -> gruen, Selbsttest modellblock funktionen mit neuer Zeile "SP in 3000
  Einheiten - letzte Stufe" 61/0.
- Handbuch "Die Karte": Hinweis zu den Detailstufen der Figuren (6 Sprachen, `tools/handbuch/inhalt_merge2.py`).

### Punkt 5: Chamfer-Feinheiten - nicht bearbeitet

- Abgabe nach Punkt 4 wegen Kontext; die Blender-Stellen stehen in `NOTIZEN_ptfix3_0811.md` Abschnitt "Punkt 3".

## Schluss

- Gruenlauf nach Punkt 1-4 (`m2_g1_*`, parallel): ctest 240/240, mission1 98/0, Modus 1 2635/0, menueeditor en 2787/0,
  figurlicht 8/0, modellblock funktionen 61/0, pt 41/0, ptfix3 12/0, karteneditor rd 21/0, r3 22/0, Vorschaufotos p27e/p27g/p27f.
- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) 0 Warnungen, 19 min 9 s; ctest 240/240; Lints wie die
  Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, cjkumbruch 0, i18n_erzeugt ok); Handbuch
  `--pruefen --pflicht=es --pflicht=cs` ok. Modus 1 und mission1 nicht noch einmal: der Quelltext ist derselbe wie im Gruenlauf
  (der Rotzweig fuer p27f war nur zwischen Gruenlauf und Neubau abgeschaltet und ist bytegleich zurueck).
- Keine Bildzeit-Messung (laut Auftrag). Spielordner unveraendert (spiel.py: "Spielordner MD gleich: True" in allen 4 Laeufen),
  vor jedem Start keine fremde MovieDuels-/jasp-Instanz.

## Offen

- Punkt 5 Chamfer-Feinheiten (siehe oben).
- Umgebung p27f: an den Chrom-Bildpunkten bleiben 9,5 von 255 Abweichung (Kanten, Glanzspitzen) - nicht weiter verfolgt.
- Skalierte Figuren mit ungleichem Massstab je Achse (scaleX/Y/Z): die Normale im Raum der Entity ist R^T n (genau nur bei
  gleichem Massstab); Ursprung und Massstab je Achse sind genau.
- Detailstufe: nur der Koerper der Figur im Hauptbild; Anbauten (Griffe, Waffen), Fahrzeuge, Geschuetze und Bolzen/Tags bleiben
  in Stufe 0. JK2: Spieler im AT-ST hat s.radius 320 (codeJK2 g_client.cpp:1466) - nicht nachgebildet.
- JK2_MODE: RE_RegisterIndividualSkin streicht "_off" am Hautnamen nur ausserhalb von JK2_MODE (tr_skin.cpp:308-316) -
  applySkin streicht immer; nicht Teil dieses Auftrags.
- "Skin oeffnen" (openSkinFile) setzt die Flaechen vorher nicht zurueck (G2API_SetSkin leert mSlist) - Altbestand.

## Changelog

## English

- Preview and menu editor: a texture name is looked up in exactly the order of the game (the named extension only if the game can read it, then jpg, png, tga) - no more .jpeg or doubled extensions.
- Characters: when a skin names the same surface twice, the first line counts as in the game, and a "*off" in any line switches the surface off; three-part skins (|head|torso|legs) follow the same rule.
- Mission preview: environment-mapped (chrome) and shiny surfaces of scaled characters are computed in the character's own space like in the game, also for hilts in their hand.
- Mission preview: characters are drawn with the detail level the game picks for them by distance and size (G2_ComputeLOD) - a small, distant figure uses the coarser mesh as in Movie Duels; the model tab's "automatic as in the game" uses the single player radius of the game.

## Deutsch

- Vorschau und Menü-Editor: Ein Texturname wird genau in der Reihenfolge des Spiels gesucht (die genannte Endung nur, wenn das Spiel sie lesen kann, dann jpg, png, tga) - kein .jpeg und keine doppelten Endungen mehr.
- Figuren: Nennt eine Haut dieselbe Fläche zweimal, gilt wie im Spiel die erste Zeile, und ein „*off“ in irgendeiner Zeile schaltet die Fläche ab; dreiteilige Häute (|Kopf|Rumpf|Beine) folgen derselben Regel.
- Mission-Vorschau: Umgebungs-Abbildung (Chrom) und Glanz skalierter Figuren werden wie im Spiel im Raum der Figur gerechnet, auch für Griffe in ihrer Hand.
- Mission-Vorschau: Figuren werden mit der Detailstufe gezeichnet, die das Spiel nach Abstand und Größe wählt (G2_ComputeLOD) - eine kleine, ferne Figur nimmt wie in Movie Duels das gröbere Netz; „automatisch wie im Spiel“ im Model-Tab nimmt den Einzelspieler-Radius des Spiels.

## 中文

- 预览和菜单编辑器: 纹理名称严格按照游戏的顺序查找 (只有游戏能读取时才先用所写的扩展名, 然后是 jpg、png、tga) - 不再有 .jpeg 或重复的扩展名。
- 角色: 皮肤对同一表面写了两次时, 与游戏一样以第一行为准, 任何一行中的 "*off" 都会关闭该表面; 三部分皮肤 (|头|躯干|腿) 遵循同样的规则。
- 任务预览: 缩放角色的环境映射 (镀铬) 和高光像游戏中一样在角色自身空间中计算, 手中的剑柄也是如此。
- 任务预览: 角色以游戏根据距离和大小选择的细节级别绘制 (G2_ComputeLOD) - 小而远的角色与 Movie Duels 一样使用较粗的网格; 模型标签页的"与游戏相同的自动"使用游戏的单人模式半径。

## 日本語

- プレビューとメニューエディター: テクスチャ名はゲームとまったく同じ順序で探します (書かれた拡張子はゲームが読める場合だけ先に、その後 jpg、png、tga)。.jpeg や重複した拡張子はなくなりました。
- キャラクター: スキンが同じサーフェスを 2 回指定したときはゲームと同じく最初の行が有効で、どの行の「*off」でもサーフェスがオフになります。3 部構成のスキン (|頭|胴|脚) も同じ規則に従います。
- ミッションプレビュー: スケールされたキャラクターの環境マッピング (クローム) と光沢は、ゲームと同じくキャラクター自身の空間で計算されます。手に持った柄も同様です。
- ミッションプレビュー: キャラクターはゲームが距離と大きさで選ぶ詳細レベルで描画されます (G2_ComputeLOD)。小さく遠いフィギュアは Movie Duels と同じく粗いメッシュを使います。モデルタブの「ゲームと同じく自動」はゲームのシングルプレイの半径を使います。

## Español

- Vista previa y editor de menús: el nombre de una textura se busca exactamente en el orden del juego (la extensión indicada solo si el juego puede leerla, luego jpg, png, tga); ya no hay .jpeg ni extensiones dobles.
- Personajes: cuando una skin nombra dos veces la misma superficie, vale la primera línea como en el juego, y un "*off" en cualquier línea apaga la superficie; las skins de tres partes (|cabeza|torso|piernas) siguen la misma regla.
- Vista previa de misión: el mapeado de entorno (cromo) y el brillo de los personajes escalados se calculan en el espacio del propio personaje como en el juego, también para las empuñaduras en su mano.
- Vista previa de misión: los personajes se dibujan con el nivel de detalle que el juego elige según la distancia y el tamaño (G2_ComputeLOD); una figura pequeña y lejana usa la malla más tosca como en Movie Duels; el "automático como en el juego" de la pestaña Model usa el radio de un jugador del juego.

## Čeština

- Náhled a editor nabídek: název textury se hledá přesně v pořadí hry (uvedená přípona jen tehdy, když ji hra umí číst, pak jpg, png, tga) - už žádné .jpeg ani zdvojené přípony.
- Postavy: když skin jmenuje stejnou plochu dvakrát, platí jako ve hře první řádek a „*off“ v kterémkoli řádku plochu vypne; třídílné skiny (|hlava|trup|nohy) se řídí stejným pravidlem.
- Náhled mise: mapování prostředí (chrom) a lesk zvětšených postav se počítají jako ve hře v prostoru postavy, i pro jílce v její ruce.
- Náhled mise: postavy se kreslí s úrovní detailu, kterou hra volí podle vzdálenosti a velikosti (G2_ComputeLOD) - malá vzdálená postava používá jako v Movie Duels hrubší síť; „automaticky jako ve hře“ na kartě Model používá poloměr hry pro jednoho hráče.
