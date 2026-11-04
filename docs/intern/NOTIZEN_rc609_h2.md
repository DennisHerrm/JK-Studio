# Notizen rc609 H2: Kampfpunkt-Vorschläge hinter Deckung (wie im Spiel)

Auftrag `auftrag_rc609_h2.md` (Plan H Stufe 2 aus `plaene/wegpunkte_und_enttext.md`, Mike: point_combat „sprinkled around randomly (usually behind cover) … used by gunners to move to cover or get a better angle for shooting“). Worktree `Y:\deckung`, Zweig `rc609h2`, Läufe `Y:\deckung\laeufe\h2_*` (eigenes `lauf.py`), Skripte in `<SP neu>/h2` (`cpmess.py` Messung der Originalkarten). Baut auf W4 auf (Zuordnung `navKampfPunkte`, Abdeckung, Laufgitter aus W3a).

## 1: Regeln aus der Quelle

- Wer Kampfpunkte nimmt: `NPC_FindCombatPoint` (OpenJK `code/game/NPC_combat.cpp:2721-2899`, Movie Duels 9 `:3487` gleich). Der Schütze (Sturmtruppler, ST_Commander) verlangt `CP_CLEAR|CP_COVER`, wenn er den Feind nicht mehr sieht (`AI_Stormtrooper.cpp:2071-2077`) oder der Feind zu nah ist (`:2080-2111`), und immer `CP_AVOID_ENEMY|CP_HAS_ROUTE|CP_TRYFAR` mit avoidDist 200 (`:2129-2132`); Rückfall über `NPC_FindCombatPointRetry` (`:2901-3000`), CP_CLEAR wird nie aufgegeben.
- Daraus die Deckungsregel für einen Punkt (origin = Entity + 0.125, `SP_point_combat`) und einen Feind (Ursprung, Kopf = Ursprung + ps.viewheight 36, `CalcEntitySpot SPOT_HEAD_LEAN` ohne eyePoint, `NPC_utils.cpp:94-135`):
  - Kasten: `gi.trace(origin, NPC->mins, NPC->maxs, origin, clipmask)` nicht startsolid/allsolid (`:2860-2864`; playerMins/Maxs −16..16 × −24..40, `NPC_stats.cpp:2028`, MASK_NPCSOLID).
  - CP_CLEAR: Abstand ≤ stats.visrange (Vorgabe 1024, `NPC_stats.cpp:1988`; `:2812-2816`), und von der Mündung (`CalcEntitySpot SPOT_WEAPON` um den origin versetzt, `:2869-2878`) frei auf Ursprung oder Kopf (`G_ClearLOS` Position→Entity, `NPC_senses.cpp:967-984`, Strahl CONTENTS_OPAQUE durch höchstens drei func_glass, `:929-955`).
  - CP_COVER: vom origin selbst weder Ursprung noch Kopf (`:2881-2884`) – „geduckt verdeckt“: der origin liegt auf Hüfthöhe (Boden + 25), unter der geduckten Blickhöhe; geduckt (`ucmd.upmove = -127` am Ziel, `AI_Stormtrooper.cpp:1445/2570`, CROUCH_MAXS_2 16) ist die Figur verdeckt, stehend schießt sie über die Deckung.
  - CP_AVOID_ENEMY: Abstand ≥ avoidDist 200 (`:2844-2848`).
- Mündung: `CalcMuzzlePoint` ohne gezeichnetes Waffenmodell (`g_weapon.cpp:449-498`, WP_BLASTER: Blickhöhe − 1, 2 vor, 1 rechts) – der vorhandene Port `muzzlePoint` (scene.cpp); die Figur blickt zum Feind.
- Jedi Outcast (`codeJK2/game/NPC_combat.cpp:2489-2700`): CP_COVER prüft nur die Position des Feindes (`NPC_ClearLOS(origin, enemyPosition)`), CP_CLEAR vom origin aus (nicht von der Mündung) auf Ursprung oder Kopf, Abstand bis visrange; avoidDist nur gegen einen Feind mit Schwert (`AI_Stormtrooper.cpp:2203-2207` JK2); gesammelt in CP_COLLECT_RADIUS 512 um den Feind (`:2524`). Also gilt dort: Körper verdeckt, Kopf sichtbar.
- MD9 gleich OpenJK (Zeile für Zeile verglichen).
- Kern: `wegDeckungGilt`, `wegDeckungVorschlaege` (`include/bhed/wegpunkte.h`, `src/wegpunkte.cpp`); `gClearLos` aus `scene.cpp` herausgelöst (vorher `sichtFrei` im anonymen Namensraum, jetzt eine Stelle für beide).

## 2: „Vorschläge erzeugen“ im Wegpunkt-Fenster

- `gui/app_deckung.cpp` (neu), Abschnitt Kampfpunkte: Knopf, „Nur in gemalten Bereichen“ (W3b), Abstand, Höchstens, Fortschritt und Abbrechen, Liste mit Grund („Deckung gegen n Feindstellen, Richtung x°“, Tooltip Ort → Mitte der Feindstellen), Annehmen je Zeile, Alle annehmen (EIN Rückgängig-Schritt), Verwerfen. Geisterpunkte gestrichelt mit Pfeil zu den Feindstellen.
- Kandidaten: das Laufgitter der Erzeugung (W3a) von den Startpunkten aus, Zellen (32 × 32), die ein Wegpunkt zu Fuß in der Reichweite der Abdeckung (W4) erreicht. Feindstellen: Gitterzellen je Rasterweite. Je Kandidat die Zahl der Feindstellen, gegen die die Regel gilt; die meisten zuerst, mit Abstand untereinander und zu vorhandenen point_combat, höchstens MAX_COMBAT_POINTS 512 zusammen.
- Gelaufen wird in der Welt der Erzeugung (Türen offen), geprüft in der Welt zur Laufzeit (`navWeltSpawn`: Türen zu, START_OFF ohne contents) – siehe Rot unten.
- Danach alle zugleich wie im Spiel zugeordnet (`navKampfPunkte` auf einer Welt mit den Vorschlägen, `navWeltFuer` neu aus `navWeltBasis` herausgelöst); was „has no waypoint“ bekäme, entfällt (Zahl im Fenster).
- Im Hintergrund wie W3b (eigener Faden, Kandidaten auf alle Kerne bis auf einen verteilt, gleiches Ergebnis wie ein Faden – ctest); Übernahme nur, wenn sich die Entities nicht geändert haben.
- Vorgaben (gemessen, `cpmess.py`, nur gelesen): Jedi Academy (GameData original/base) 23 Karten mit 2998 point_combat: Median Abstand zum nächsten 231, Median Kampfpunkte je Wegpunkt 0,519; Movie Duels (Ordner MD) 75 Karten, 9097: 200 und 1,0; Jedi Outcast 23 Karten, 2964: 145 und 0,449. Abstand und Höchstens = diese Werte des Regelwerks (Höchstens = Anteil × Wegpunkte der Karte).

## 3: Messung an Originalkarten

- Wo stehen sie: siehe Vorgaben; Median je Karte 118 (Raven SP), 56 (MD), 104 (JK2); Median Abstand zum nächsten Wegpunkt 116 / 125 / 79.
- Vergleich (`BHED_DK_VERGLEICH=1`: Kampfpunkte der Karte weg, Wegnetz bleibt, so viele Vorschläge wie Originale, Regelwerk Movie Duels): Originale mit einem Vorschlag in einer Rasterweite (192):
  - t1_fatal 120 von 219 (134 Vorschläge), t2_rogue 178 von 235 (194), hoth2 56 von 183 (183), duel_tusken 290 von 345 (188), duel_jeditemple 314 von 487 (334); zusammen 958 von 1469 (65 %). Nur berichtet, die Regeln sind nicht darauf zugeschnitten.

## Rot und Ursachen

- ctest `deckung` (neu, `tests/deckungtest.cpp`, Brushes mit `kollisionsSpur`): 19/0 im ersten Lauf (Regelfälle aus der Quelle: niedrige/hohe Wand, Freifeld, avoidDist, visrange, Kasten in der Wand, Jedi Outcast anders als Jedi Academy; Vorschläge: Regel, Abstand, Reihenfolge, 4 Fäden = 1 Faden, Bereich, Abbrechen, ohne Wegpunkt nichts).
- Selbsttest `deckung` (neu, `gui/selbsttest_deckung.inc`) erster Lauf rot auf beiden Karten:
  - jtg (`h2_dkjtg`): Regel mit der Spur des Spiels 506 von 507 – an einer Stelle diagnostiziert: (−3104 1984 671) steht in der Tür `temple_doors1` (*12, −3136..−3070 × 1966..1982 × 646..794). Die Suche prüfte den Kasten in der Welt der Erzeugung (Türen offen), das Spiel zur Laufzeit mit geschlossener Tür: startsolid. Gegenprobe t1_danger (`h2_dkt1`): 510 von 511, gleiche Art.
  - Beide: Absturz (0xC0000005) beim Klick „Alle annehmen“ – die Liste wurde nach dem Annehmen im selben Bild weiter gelesen.
- Vergleichsläufe (`h2_v_*`) rot in der eigenen Annahme „erreichbar vom info_player_start“ (CP_HAS_ROUTE nachgebildet): t1_fatal 115 von 141, t2_rogue 202 von 207, hoth2 183 von 183 verworfen (der Spielerstart hängt in Raven-Karten oft an einem eigenen Teil). CP_HAS_ROUTE fragt `NAV::InSameRegion(NPC, …)` mit der Figur zur Laufzeit – statisch nicht bestimmbar; die Kandidaten sind schon zu Fuß von einem Wegpunkt erreichbar. Behoben: nur „has no waypoint“ verwirft.

## Behoben, grün

- Sicht und Kasten in der Welt zur Laufzeit (`WegDeckungWunsch::kastenFrei`, `sicht` aus `navWeltSpawn`), Laufen weiter mit offenen Türen; Annehmen/Verwerfen nach dem Zeichnen der Liste.
- `deckung` jtg (`h2_dkjtg2`) **8/0**: 47560 Kandidaten, 1330 Feindstellen, 15624 mit Deckung, 512 Vorschläge (Höchstens-Vorgabe 1,0 × 788 Wegpunkte, gekappt bei 512), 0 verworfen, 6,3 s (31 Fäden); Regel mit der Spur des Spiels 512/512; einzeln annehmen 1 Schritt, zugeordnet = GetNearestNode; alle 512 zugeordnet, 2 Schritte; Strg+Z ×2 bytegleich; Abbrechen bytegleich ohne Schritt; Bereich 3 Vorschläge, 0 außerhalb.
- Gegenprobe t1_danger (`h2_dkt12`, nicht zum Entwickeln benutzt) **8/0**: 134347 Kandidaten, 4008 Feindstellen, 511 zugeordnet, 1 verworfen (has no waypoint), Regel 511/511, Bereich 7, 0 außerhalb.
- Vorhandene Tests: `kampfpunkte` 15/0, `wegerzeugung` 7/0, `wegmalen` 17/0, `wegpunkte2` 29/0, `wegpunkte` 53/0, `mission1` 98/0; ctest 208/210 (handbuch_sprachen lief parallel zur Handbuch-Erzeugung, danach grün; lint_endtable beanstandet `app_karte_texturliste.cpp:167/179/180` aus rc609 K, schon am Startpunkt, nicht geändert).
- Handbuch: Seite NPC-Wegpunkte, Abschnitt nach den Kampfpunkten mit `h_weg_deckung` (6 Sprachen, `BHED_HB_NUR=deckung`, 3840x2160@2, 2560 breit); `gen_handbuch.py` es/cs 0 Texte englisch. Texte in `tools/i18n_rc609_h2.py`.
- Lints lint_sources, lint_fassung, lint_i18n, lint_i18n_erzeugt, lint_headers, lint_feldinit, lint_gapp, lint_settings ok; lint_portability 107 wie am Startpunkt.

## Spielbeleg (Movie Duels, Laden)

- `h2_dkjtg3` (`deckung` jtg 8/0 mit `BHED_DK_ENT`): die Entities mit allen 512 angenommenen Vorschlägen als `maps/h2deck.bsp` im eigenen fs_homepath (`laeufe/h2_mdhome`, fs_game MD, developer 1, Fenster außer Sicht, `mddeck.py` aus W4 `mdkampf.py`), nichts im Spielordner.
- Ergebnis: 512 Kampfpunkte geladen, „has no waypoint“ im Spiel 0 = Editor 0, „combat point … in solid“ (G_CheckInSolid) 0. 5 der 512 melden „Nav(2) in solid“ (der Kasten von SpawnedPoint ±20 steckt): sie werden kein eigener Knoten und bekommen wie in W4 den nächsten Wegpunkt; das Spiel läuft weiter (MD9 ohne delayedShutDown, OpenJK g_delayedShutdown 0).
- Jtg hat nach der vollen Erzeugung 788 Wegpunkte; mit 512 Kampfpunkten liegt das Netz über NUM_NODES 1023 – SpawnedPoint nimmt die übrigen nicht als Knoten (`g_navigator.cpp:2301`), die Zuordnung (navKampfPunkte, gleich gebaut) rechnet damit.

## Schluss

- Bau aller Ziele mit /W4 /WX ohne Warnung; ctest 208/210 (siehe oben); `deckung` jtg 8/0 (`h2_dkjtg3`, mit Spielbeleg), t1_danger 8/0; `kampfpunkte` 15/0, `wegerzeugung` 7/0, `wegmalen` 17/0, `wegpunkte2` 29/0, `wegpunkte` 53/0, `mission1` 98/0, `karteneditor` kurz (`BHED_KARTE_NUR=fotos`) 78/0, Modus 1 2651/0; Lints wie oben. Läufe parallel; der Bau allein.

## Eigene Annahmen (keine Engine-Quelle)

- Feindstellen: begehbarer Boden (Laufgitter) je Rasterweite; der Feind steht (Ursprung = Boden + 24). Messgrund: die Rasterweite ist der Maßstab der Abdeckung (W3a/W4).
- Kandidaten nahe dem Netz: Reichweite der Abdeckung (W4); origin = Boden + 25 wie „Kampfpunkt setzen“.
- Rangfolge nach der Zahl der Feindstellen; Abstand/Höchstens aus den Originalkarten (oben).
- Mündung ohne Waffenmodell (CalcMuzzlePoint-Zweig WP_BLASTER), Blick waagerecht zum Feind; im Spiel nimmt die Figur ihren aktuellen Blick und, wenn gezeichnet, den Bolzen des Waffenmodells (renderInfo.muzzlePoint).
- Kopf des Feindes ohne eyePoint (Ursprung + 36).
- CP_AVOID_ENEMY: SafePathExists hängt vom Standort der Figur ab, nicht geprüft; CP_HAS_ROUTE ebenso (siehe Rot).

## Offen

- Spielbeleg mit einem Schützen im Kampf: das Spiel druckt nicht, welchen Kampfpunkt eine Figur wählt (MD9 hat dafür keine Ausgabe; d_npcai/Debug_Printf nur für Sicht und Verstecken), die Position einer Figur steht nicht im Protokoll. Belegt ist nur das Laden (oben), nicht die Wahl im Kampf.
- Bildzeit/Tempo nicht gemessen (Regel für parallele Agenten).
- Fund nebenbei (nicht H2): im selben Spiellauf „Nav(1) in solid: wpauto_398@(-320 1920 328)“ – ein von der vollen Erzeugung (W3a) gesetzter Wegpunkt, den das Spiel nicht als Knoten nimmt.
- lint_endtable (ctest) beanstandet `gui/app_karte_texturliste.cpp:167/179/180` (rc609 K, schon am Startpunkt 1dad0cd5).

### Changelog

- English: NPC waypoints: "Generate suggestions" finds combat points behind cover the way a gunner picks them in the game (NPC_FindCombatPoint with cover and clear shot: hidden from the point itself, a free shot from the muzzle when standing); ghost points with their reason, accept one or all in one undo step, only in painted areas if you like, in the background with progress and cancel. Spacing and count default to the original maps of the rule set.
- Deutsch: NPC-Wegpunkte: „Vorschläge erzeugen“ findet Kampfpunkte hinter Deckung so, wie ein Schütze sie im Spiel wählt (NPC_FindCombatPoint mit Deckung und freiem Schuss: vom Punkt selbst verdeckt, von der Mündung im Stehen frei); Geisterpunkte mit Grund, einzeln oder alle in einem Rückgängig-Schritt annehmen, auf Wunsch nur in gemalten Bereichen, im Hintergrund mit Fortschritt und Abbrechen. Abstand und Zahl nach den Originalkarten des Regelwerks.
- 中文:NPC 路径点:“生成建议”按游戏中枪手的选择方式寻找有掩护的战斗点(NPC_FindCombatPoint,有掩护且射击无阻:从点本身被遮挡,站立时从枪口可自由射击);以带原因的虚影点显示,可逐个或在一个撤销步骤中全部接受,可只在绘制的区域内,在后台运行并显示进度、可取消。间距和数量默认取自该规则集的原版地图。
- 日本語: NPC ウェイポイント: 「戦闘ポイントを提案」は、ゲームの射手と同じ方法で遮蔽物の陰の戦闘ポイントを探します (NPC_FindCombatPoint、遮蔽と射線: ポイント自体からは隠れ、立った銃口からは撃てる)。理由付きのゴーストポイントで、1 つずつまたは元に戻す 1 回分ですべて受け入れられ、描いた範囲だけにもでき、進行表示と中止付きでバックグラウンドで動きます。間隔と数の既定値はルールセットのオリジナルマップから。
- Español: Puntos de ruta de NPC: "Sugerir puntos de combate" busca puntos de combate a cubierto como los elige un tirador en el juego (NPC_FindCombatPoint con cobertura y tiro libre: oculto desde el propio punto, tiro libre desde la boca del arma de pie); puntos fantasma con su motivo, aceptar uno o todos en un solo paso de deshacer, si se quiere solo en zonas pintadas, en segundo plano con progreso y cancelación. Separación y cantidad predeterminadas según los mapas originales del conjunto de reglas.
- Čeština: Body trasy NPC: „Navrhnout bojové body“ hledá bojové body za krytem tak, jak je ve hře vybírá střelec (NPC_FindCombatPoint s krytem a volnou střelbou: ze samotného bodu zakrytý, z ústí zbraně vestoje volná střelba); přízračné body s důvodem, přijmout jeden nebo všechny v jednom kroku zpět, volitelně jen v namalovaných oblastech, na pozadí s průběhem a zrušením. Rozestup a počet výchozí podle původních map sady pravidel.
