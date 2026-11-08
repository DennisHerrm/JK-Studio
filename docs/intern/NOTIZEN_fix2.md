# Notizen FIX2 (08.11.2026): Funde der Code-Pruefung seit rc610 ausserhalb von Painting/CJK

Auftrag `JKStudio_Uebergabe/auftraege/auftrag_fix2_0811.md`, Funde aus `plaene/codepruefung_seit_rc610.md` (PRUEF3).
Worktree `Y:\fix2`, Zweig `fix2` (Basis fe1f374c3), Build `Y:/fix2/build-fix2`, Laeufe `Y:/fix2/laeufe` (Vorsatz `f2_`),
Skripte `Y:\werkzeuge\f2\`.

## P5: Animationsimport, Bildzahl begrenzt

- Vorher: `n = floor(sekunden * fps) + 1` als `int` ohne Grenze; ein Schluessel bei 2e7 s (ms statt s) legte je Bild einen
  Vektor aller Knochen an (bad_alloc, Programmende), eine Zeit ueber int gab unter MSVC n = 1 und "gelesen".
- Jetzt: erst alle Animationen ausmessen (in double), dann ablehnen (`FehlerArt::AnimZuLang`: Bilder, MB noetig, MB frei),
  wenn alle Bilder nicht in `int` passen (`GlaAnimation::numFrames`, `AnimEntry::firstFrame`) oder die Bilder
  (je Bild und Knochen Welt `Starr` + lokale Stellung `BoneMatrix`, dazu je Bild ein Vektor) mehr brauchen als frei ist
  (`GlobalMemoryStatusEx` ullAvailPhys). Grund der Grenze: der Speicher, den der Import selbst braucht, und der Typ der
  Bildzahl - keine feste Zahl. `bad_alloc` beim Abtasten gibt dieselbe Meldung.
- Rot `f2_p5_rot_ctest` (ctest modell_animimport unter 3-GB-Jobgrenze, `Y:\werkzeuge\f2\jobgrenze.py`): Absturz
  0xC0000409 bei 3000 MB. Gruen `f2_p5_gruen_ctest`: 0 Fehler (2 s gelesen, 61 Bilder; 2e7 s und 1e9 s abgelehnt).
- Bytegleich: presetbau-Ausgabe der vier Testdateien und einer BVH mit 10000 Bildern vorher = nachher (`f2_p5_bytegleich.txt`).

## P14: Meldungen des Animationsimports in 6 Sprachen

- Fehler und Hinweise der Bibliothek als Art (`bhed/modell/fehler.h`: KeineAnimation, KeinSkelett, AchsenUnbestimmt,
  AnimZuLang, BvhOhneSeiten, BvhBoden, UngleicheSkalierung; `AnimImport::fehlerArt`, `hinweise`, `vornAusSkelett`),
  Text in der Sprache der Oberflaeche ueber `modellFehlerText`; assimp-Meldungen bleiben, wie assimp sie schreibt.
- Rot `f2_ai_rot` (Selbsttest modellblock/ai, en): 2 FEHLER ("(aus dem Skelett)", "keine Animation in der Datei").
  Gruen `f2_ai_gruen` 31/0 (en), `f2_ai_gruen_es` 31/0 (es: "del esqueleto", "no hay animación en el archivo").

## P17: presetbau --von/--bis

- Bereich ausserhalb der Animation gibt "FEHLER --von/--bis ..." statt Lesen ausserhalb von `roh`. ctests
  `presetbau_bereich`, `presetbau_bereich2` (testgang.bvh, 31 Bilder): rot 0/2 ("nichts zu schreiben" bzw. Schleife aus
  Bildern ausserhalb), gruen 2/2.

## P9: Animationsimport-Tempo

- `schluesselVor` lief je Bild, Knoten und Art vom ersten Schluessel; jetzt laeuft der Index je Spur mit (nur bei
  aufsteigenden Zeiten, sonst wie bisher von vorn - dasselbe i), `Decompose` der Ruhe einmal je Knoten.
- Bytegleich: FNV der Bilder und Grundstellungen der vier Testdateien gleich (`f2_p9_ctest.txt` gegen
  `f2_p5_gruen_ctest.txt`), presetbau-Ausgabe der langen BVH (10000 Bilder, 53 Gelenke, `Y:\werkzeuge\f2\langbvh.py`)
  sha256 2bffb0a20d62def7 vorher und nachher.
- Zeit (presetbau --einmal, `Y:\werkzeuge\f2\p9zeit.py`): vorher 2,72/2,68 s (ohne fremde Builds, 29 % Grundlast),
  im Wechsel gemessen 2,89 s vorher gegen 0,64-0,74 s nachher, waehrend andere Agenten bauten. **Messung ohne fremde
  Last offen.**

## P11: Karten-Editor Echtzeit-Licht, spielDarstellung zwischengespeichert

- Je Shader einmal gerechnet; neu nach Flush & Reload Shaders (`Editor::neuGeladen`), anderem Spielkontext (weak_ptr)
  und dem Leeren der Shader-Skripte in `weltAktualisieren` - dieselben Ereignisse wie `skript()` und `echtShaderInfo`.
  Aufrufer nehmen eine Referenz.
- Bildgleich: alle 32 Fotos von karteneditor r3 (8), rd (9) und editorportal (15, mit duel_executor, duel_wbw,
  duel_calodan) bytegleich vorher (`f2_*_vorher`, Basis-exe) und nachher (`f2_*_nachher`), `f2_p11_bildvergleich.txt`.
- **Bildzeit offen**: andere Agenten bauten und spielten Movie Duels; Messweg: `leistung` mit `BHED_LEISTUNG_NUR=e`,
  `BHED_LEISTUNG_MAP=Y:\werkzeuge\sp_af9c\s9c\taris\md_rev_TarisUpper2_converted.map`, `BHED_D3D_DEBUG=0`, Basis-exe
  `Y:\werkzeuge\f2\basis_bin` gegen den neuen Bau, allein.

## P10: Lizenzkopf der Blender-Teile

- `src/modell/wof/mesh_werkzeuge.cpp`: SPDX "BSD-3-Clause AND GPL-2.0-or-later", die Blender-Teile (segmentieren mit
  FaseEnde/FaseInfo, die Segment-Daten in chamferKern: selcount, tri_corner_test, pipe_test) zwischen Marken; als Ganzes
  GPL v2 wie JK Studio. THIRD-PARTY.md: Blender-Zeile nennt fase_blender.h und die markierten Teile; neue Zeile fuer die
  bisher nicht genannten Blender-Teile (weight_to_rgb, bone_octahedral_verts, paint_vertex_weight_utils, Pinsel-Abfall:
  gewichte.*, app_modelltab_ansicht.cpp). Fenster Ueber: Zeile `AboutBlenderLizenz` (6 Sprachen). Handbuch
  Modellieren > Herkunft und Lizenz (6 Sprachen).
- lint_sources prueft: jede Datei, die Blenders Quellen nennt (bmesh_*.cc), traegt GPL-2.0-or-later im Kopf und steht in
  der Blender-Zeile. Rot `f2_lint_p10_rot.txt` 3 Beanstandungen, gruen `f2_lint_p10_gruen.txt` ok.

## Wetter JK2: zwei "rain init" mit verschiedenem count

- OpenJK spielt JK2 mit rd-vanilla (`code/rd-vanilla/tr_WorldEffects.cpp`, kein JK2_MODE-Zweig): `R_WorldEffectCommand`
  liest nur das erste Wort, jedes "rain ..." legt eine neue Wolke an (bis MAX_PARTICLE_CLOUDS 5, :1891-1911). Zwei
  fx_rain mit verschiedenem count sind zwei Configstrings (`G_FindConfigstringIndex`, codeJK2/game/g_fx.cpp:305-321),
  also zwei Wolken; gleicher count einer. `gui/app_wetter.cpp` macht genau das - keine Aenderung. Nicht geprueft: der
  Renderer des Original-jk2sp.exe (Quelle nicht auf der Platte).

## P16: tcMod turb und rotate wie rd-vanilla

- Quelle (OpenJK `code/rd-vanilla`, MD9 gleich): `ComputeTexCoords` (tr_shade.cpp:1725) wendet jede tcMod der Reihe nach
  an; `RB_CalcTurbulentTexCoords` (tr_shade_calc.cpp:936) addiert `sinTable[(int)(((x + z) / 1024 + now) * 1024) & MASK]
  * amplitude`, `RB_CalcRotateTexCoords` (:1009) liest sin/cos ebenso aus der Tafel; `tr.sinTable[i] = sin(DEG2RAD(i * 360
  / 1023))` (tr_init.cpp:1727). Hier stand turb nach der ganzen Matrix (wie rend2) mit `sin(frac(...) * 2 pi)`, rotate mit
  genauem cos/sin.
- Jetzt: `gpu::texModAbbildung` gibt zusaetzlich den linearen Teil der Schritte hinter turb (turbMat); `gpu::turbWerte`
  rechnet 1024 now und seinen Rest modulo 1024 in double und die Matrix mal Amplitude. Neuer float4 am Ende der
  Stapelkonstanten (`gTexTurbMat`, Lage 44, Block 48 float), der Figurenstufe (`gStufeTurbMat`, kStufeFloats 56) und des
  Echtzeit-Lichts (`tcTurb`). Die vier Shaderstellen (Vorschau Karte und Figurenstufe, Echtzeit-Licht kShader3/4) nehmen
  den Index wie `(int)` (zur Null hin) `& 1023` aus der Tafel; rotate auf der CPU aus der Tafel. Mehrere turb in einer
  Stufe: der letzte zaehlte (wie vor FIX2) - in FIX3 behoben (Abschnitt unten).
- Rot `f2_p16_rot.txt` (ctest gpuconst, Shader-Rechnung gegen rd-vanilla an drei Ecken und drei Zeiten): turb + scale
  0,57, rotate 0,0026, scroll + turb 0,0017, turb + rotate + scale 1,18; gruen `f2_p16_gruen.txt`: alle 0,000000.
- Fotos (Bau mit P16, `f2_r3_p16`, `f2_rd_p16`, `f2_ep_p16`): alle 3D-Bilder bytegleich mit P11-nachher (die Testkarten
  zeigen kein turb und kein rotate); nur `karteneditor_start.bmp` weicht ab (Mauskoordinaten x -224/-240 in der
  Statusleiste - die Maus steht anderswo, kein Bildfehler).
- **Offen**: ein Spielfoto mit einem turb-Shader (z. B. Wasser mit turb + scale) gegen die Vorschau; ohne Spielstart
  hier nur gegen den rd-vanilla-Code geprueft (ctest).

## Schluss

- Sauberer Neubau (`--clean-first`, `f2_neubau.log`): 0 Warnungen.
- ctest 237/237 (`f2_s_ctest.txt`).
- Selbsttests auf dem Neubau: modellblock ai 31/0, pr 15/0, gluehen 9/0, figurlicht 8/0, Modus 1 2635/0. Mit dem Bau von
  P16 (Code gleich bis auf Kommentare): karteneditor r3 22/0, rd 20/0, editorportal 19/0.
- Lints: sources, cbuffer, i18n, i18n_erzeugt, settings, endtable, d3dbesitz, fassung, feldinit, gapp, tabellenid,
  merkmalsbits, abstand, bildvergleich, breitenabzug, doppelanzeige, headers, hoehe, imgui_alpha, imgui_context, layout,
  mapstate, msvclink, resize, rueckkopplung ok; portability 99, pruefcodes 11, fremdprogramm 1 (kartenpruefung.py),
  efxflags "keine Flagtabelle" wie vorher, keine Meldung zu geaenderten Dateien; link/tidy nur unter Linux.
- Handbuch `--pruefen`: handbuch_sprachen ok.

## Abweichungen

- P5: die Grenze ist der freie Arbeitsspeicher (und int fuer die Bildzahl), keine feste Bildzahl: OpenJK begrenzt eine
  Sequenz auf 65535 Bilder (`animation_t` unsigned short, code/game/bg_public.h:461), Movie Duels nicht (int, MD9
  bg_public.h:722) - eine feste Zahl haette lange Aufnahmen fuer Movie Duels abgelehnt.
- P10: die Blender-Teile bleiben in mesh_werkzeuge.cpp (markiert, Kopf "BSD-3-Clause AND GPL-2.0-or-later") statt nach
  fase_blender.cpp verschoben - sie brauchen viele Hilfen der Datei (PolyNetz, Vielecke, Plan).

## Offen

- Bildzeit P11 (Karten-Editor Echtzeit-Licht) und Zeit P9 ohne fremde Last (andere Agenten bauten/spielten).
- P16 Spielfoto mit turb-Shader: erledigt in FIX3 Punkt 2.

## FIX3 Punkt 1: mehrere tcMod turb in einer Stufe

Auftrag `auftrag_fix3_0811.md`, Laufnamen `f3_`, Skripte `Y:\werkzeuge\f2\f3\`.

- Quelle: rd-vanilla `ComputeTexCoords` (tr_shade.cpp:1725) ruft fuer JEDE tcMod turb `RB_CalcTurbulentTexCoords` an ihrer
  Stelle der Kette (bis `TR_MAX_TEXMODS` 4, tr_local.h:279; MD9 gleich). Bisher galt nur der letzte turb einer Stufe.
- Jetzt: `gpu::texModAbbildung` liefert eine `TurbKette` (je turb Amplitude, Phase und der lineare Teil der Schritte hinter
  ihm), `gpu::turbFelder` die Werte fuer die Shader. Die Shader addieren je turb `turbVersatz` (eine HLSL-Funktion, von
  Karte, Figurenstufe und Echtzeit-Licht gemeinsam genutzt, `gpu::turbHlsl`).
  - Stapelblock: zweiter bis vierter turb in `gTurbMat2..4` (48, 52, 56), `gTurbQ23` (60) und `gTurbQ4` (13, in der Luecke
    hinter gAlphaSchwelle) - der Block bleibt 64 float, ein Ring-Eintrag (256 Bytes), kein zusaetzlicher Speicher je Stapel.
    Lagen aus dem Shader gelesen: "gTurbQ4@13 gTurbMat2@48 gTurbMat3@52 gTurbMat4@56 gTurbQ23@60 - passt".
  - Figurenstufe: `gStufeTurbMat2..4`, `gStufeTurbQ23`, `gStufeTurbQ4` (kStufeFloats 56 -> 76), im Ring zwei Eintraege je Stufe
    (`kStufeRingEintraege`).
  - Echtzeit-Licht: `tcTurb2..4`, `tcTurbQ23`, `tcTurbQ4` in B3 (kShader3/kShader4).
- Rot `f3_p1/f3_p1_rot.txt` (ctest gpuconst, Shader gegen rd-vanilla an drei Ecken und drei Zeiten): "turb, scale 2 3, turb"
  0,854, "turb, turb, rotate 30" 0,400, "turb, rotate 30, turb, scale 2 3" 1,200, "turb, turb, turb, turb" 0,423 (vier FEHL).
  Gruen `f3_p1/f3_p1_gruen.txt`: alle acht Faelle 0,000000; die Probe liest jetzt den gepackten Block an den Lagen der Shader.
  gpushader (Text: vier `+= turbVersatz(` in Karte und Figur), hlsl, figurstufen, gpudraw gruen.
- Selbsttests auf dem Bau: karteneditor r3 22/0 (`f3_p1_r3`), rd 20/0 (`f3_p1_rd`), editorportal 19/0 (`f3_p1_ep`),
  figurlicht 8/0. Fotos gegen FIX2-P16: r3 7, rd 8, editorportal 15 bytegleich; nur `karteneditor_start.bmp` weicht wie in FIX2
  ab (Mauskoordinaten in der Statusleiste). Die Testkarten dort zeigen kein turb - geprueft gegen die Spielformel per ctest,
  im Spiel per Punkt 2.

## FIX3 Punkt 2: Spielfoto mit turb- und rotate-Shader

- Testkarte `f3turb` (`Y:/werkzeuge/f2/f3/karte.py`, bhed_q3map2 aus build-fix2): geschlossener Raum 1024 x 1024 x 512, alle
  Innenflaechen `textures/f3/turbrot` ohne Lightmap (`surfaceparm nolightmap`, Farbe = Bild), Stufe
  `tcMod turb 0 0.3 0.15 0.05`, `tcMod rotate 9`, `tcMod turb 0 0.2 0.6 0.11`, `tcMod scale 1.25 1.25` (zwei turb, der erste
  durch rotate und scale, der zweite durch scale); Muster 256 x 256 mit 8 x 8 farbigen Feldern. Karte, Shader und Bild im
  eigenen fs_homepath (`home_<lauf>/MD`) bzw. `f3turb.pk3` fuer die Vorschau.
- Spiel (`spiel.py`): Movie Duels SP per Popen (vorher keine fremden MovieDuels-/jasp-Prozesse), ICARUS-Skriptkamera
  (MOVE -480 0 256, PAN 8 15, ZOOM 80), 1920 x 1440. Zeit festgehalten: nach dem Laden `timescale 0.05` (1 ms Spielzeit je Bild,
  Com_ModifyMsec MD9 common.cpp:1391) und `sv_fps 1000`; das Kameraskript schreibt in einer Schleife `print`, mit
  `g_ICARUSDebug 4` steht jede Zeile mit level.time im Protokoll; drei Bilder per `screenshot_tga` (das PNG von Movie Duels war
  bei 2 von 3 Bildern nicht lesbar). level.time vor den Bildern 8692, 8722, 8752 (Lauf `f3s5`).
- Vorschau (`vorschau.sh`, Selbsttest geschuetze mit BHED_GV_PK3/KARTE/KAMERA/ANSICHT 1920x1440/ZEITEN): dieselbe Kamera,
  Fotos zu vielen Zeiten (`f3_p2_vorschau_weit`, `_fein`); die Kartenansicht 1920 x 1440 ausgeschnitten (`vergleich.py`).
  Das Spielbild liegt 48-50 ms vor der gedruckten level.time (cg.time laeuft der Serverzeit nach); bestes Bild je Spielbild:
  f3a 8644 ms, f3b 8672 ms, f3c 8706 ms - mittlere Abweichung 0,83/0,84/0,84 (0..255), Pixel mit Abweichung ueber 48:
  0,002 % (nur einzelne Kantenpixel, Filterung). Die Vorschau zeigt wie das Spiel Stufen: ueber 3-5 ms bleibt das Bild gleich
  (der Index in die Sinustafel springt nur alle paar ms), daneben steigt die Abweichung sofort auf 5,6-25.
- Gegenproben zur selben Zeit: Bau FIX2 d04f18e5d (nur der letzte turb; aus `git checkout d04f18e5d --` gebaut,
  `Y:/werkzeuge/f2/f3/bin_fix2`, Lauf `f3_p2_vorschau_fix2`) 86,95/87,46/88,42, Pixel ueber 48: 86,8/87,5/89,1 %;
  Basis vor FIX2 fe1f374c3 (`f3_p2_vorschau_basis`) 85,9/86,8/87,6, 88,7-90,3 % - das Bild ist sichtbar verschoben.
- Paarbilder (Spiel oben, Vorschau unten, ungeskaliert 1920 x 2880): `Y:/werkzeuge/f2/f3/laeufe/f3s5/vergleich/`
  `paar_f3[abc]_spiel_oben_vorschau_<ms>_unten.png`, Gegenprobe `paar_f3b_spiel_oben_vorschau_FIX2_d04f18e5d_unten.png`;
  angesehen (verkleinert und ein Ausschnitt in voller Groesse): gleich, nur Kantenpixel; die Gegenprobe ist um mehrere Felder
  verschoben. Spielordner Movie Duels vorher = nachher.
- Geprueft ist damit die Vorschau Karte (Stapelblock). Figurenstufe und Echtzeit-Licht rechnen mit denselben Werten
  (`turbFelder`, `turbVersatz`) - geprueft per ctest gegen die Spielformel, nicht per Spielfoto.

## FIX3 Punkt 3: Codepruefung MRESTE Teil 3 und MR4 (nur gelesen)

- Funde P20-P28 (4 M, 5 N) angehaengt an `JKStudio_Uebergabe/plaene/codepruefung_seit_rc610.md` ("Nachtrag FIX3"), nichts
  behoben: P20 Texturkompression laesst alle Shader mit `$lightmap`-Stufe aus, P23 dasselbe Modell neu laden verwirft die
  Bilder nie, P24 Umbacken nach LOD-Wechsel, P27 model2scale-Licht im Spiel s-fach, P21 Himmel unkomprimiert, P22
  Regelwerkwechsel laedt Bilder nicht neu, P25 Wiederholen ueber Leinwaende, P26 Speicherspitze bei Eigenschafts-Schritten,
  P28 CJK-Umbruch je Bild neu.

## FIX3 Schluss

- Sauberer Neubau (`--clean-first`, `f3_neubau.log`): 0 Warnungen. ctest 237/237 (`f3_s_ctest.txt`).
- karteneditor r3 22/0, rd 20/0, editorportal 19/0 (`f3_s_*`), Fotos bytegleich mit `f3_p1_*` (8, 9, 15). Modus 1 2635/0.
- Lints (`f3_lints.txt`): wie nach FIX2 (portability 99, pruefcodes 11, fremdprogramm 1, efxflags ohne Tafel, sonst ok), keine
  Meldung zu geaenderten Dateien. Adressen ab fe1f374c3: 0.
- Offen: Bildzeit allein (Stapelblock 64 statt 48 float - noch ein Ring-Eintrag je Stapel; Figurenstufe zwei Eintraege; je
  weiterem turb ein Zweig im Vertexshader) - nicht gemessen, andere Agenten bauten und testeten; ebenso P9/P11 aus FIX2.

## FIX4 P33: tcMod-Kette, turb und rotate bitgleich zu rd-vanilla

Auftrag `auftrag_fix4_0811.md`, Laufnamen `f4_`, Skripte `Y:\werkzeuge\f2\f4\`.

- Quelle: rd-vanilla `ComputeTexCoords` (tr_shade.cpp:1725) wendet jede tcMod der Reihe nach in float auf st an;
  `RB_CalcTurbulentTexCoords` (tr_shade_calc.cpp:936-951, MD9 :903-915) Index `(int)(((x + z) * 1.0/128 * 0.125 + now) * 1024)` in
  double, Wert `sinTable[...] * amplitude` in float; `tr.sinTable` (tr_init.cpp:1727) ganz in float (`DEG2RAD` mit `PI_DIV_180` als
  float, q_math.h:85-88; `sin` ueber die float-Ueberladung aus <cmath>, q_shared.h:120); `RB_CalcRotateTexCoords` (:1009) translate
  in double (0.5 ist double), als float abgelegt; `RB_CalcTransformTexCoords` `s * m00 + t * m10 + tr`.
- Vorher (FIX2/FIX3): Tafel in double mit genauem pi (580 von 1024 Eintraegen um eine float-Stelle daneben), im Shader `sin` der
  Grafikkarte; Index `floor(ort + qr)` in float (kippte bei ort + 1024 now dicht an einer ganzen Zahl); rotate-translate in float;
  die Kette wie rd-rend2 zu EINER Abbildung zusammengerechnet (rundet anders: "turb, scale 2 3" bis 48 float-Stufen, "turb, turb,
  rotate" bis 5120). Der Kommentar "denselben floor" stimmte nicht.
- Jetzt:
  - `gpu::sinTafel` in float wie R_InitFuncTables; die Shader lesen die Tafel als Bitmuster (`static const uint kSinTafel[1024]`,
    von der CPU in den HLSL-Text geschrieben) statt `sin` zu rechnen.
  - `turbIndex` (HLSL): (int) des auf double gerundeten ort + 1024 now ohne double auf der GPU - TwoSum gibt s + e = p + q exakt;
    ist |s| nicht ganz, bleibt floor(|s|); ist |s| ganz, entscheidet die halbe double-Stufe unter |s| + c (Gleichstand gerade);
    ausserhalb von int 0 wie 0x80000000 & 1023. `precise` (kein mad, nichts umgestellt).
  - Die Kette je Schritt wie rd-vanilla (`gpu::tcKette`, HLSL `tcKette`/`tcSchritt`): Art (affin = transform/rotate/stretch,
    turb, scale, scroll) und die Werte, die das Spiel an die Rechenfunktion gibt; turb mit 1024 now (in float exakt) und
    Amplitude. Stapelblock: gTcM0 (0), gTcO01 (4), gTcM1..3 (44, 48, 52), gTcO23 (56), gTcArt (60) - 28 statt 30 float, Block
    weiter 64 float (ein Ring-Eintrag); gTurbQ4 bei 13 entfaellt. Figurenstufe gStufeTcM0.. (kStufeFloats 76 -> 72), Echtzeit-Licht
    B3 tcM0.. (eine float4 weniger), Wolkenstufe (turb wie bisher aussen vor), Menue-Editor `tcKetteOhneTurb` (turb wie bisher
    aussen vor). rotate: erster Schritt uebernommen statt mit der Einheit malgenommen (-sin(0) = -0 bleibt -0).
- Rot `f4_rot/f4_rot_ctest.txt` (Stand faec3e7eb, neue Pruefungen): ctest gpuconst 7 FEHL - Tafel 580/1024, turb-Versatz 82984 von
  100000 Ecken (feste Ziehung inkl. Vorzeichenwechsel, 1024 now ueber 2^24, dicht an ganz), rotate-Werte 28/30, Shader "turb"
  1155/4000, "scroll, turb" 1158/4000, "rotate 30" 3443/4000, "transform" 2235/4000; neuer ctest turbgpu (der HLSL-Text als
  Compute-Shader) WARP 105867 von 131072 Werten, Intel UHD Graphics 82231 von 131072. Zwischenstand (Index und Tafel exakt, Kette
  noch zusammengerechnet): Ketten aus mehreren Schritten 5 FEHL (48 bis 5120 float-Stufen) - daher je Schritt.
- Gruen `f4_p33_2.txt`: gpuconst alle bitgleich (Tafel 0/1024, turb 0/100000, rotate 0/30, sechs einzelne Schritte je 0/4000, acht
  Ketten aus FIX2/FIX3 je 0/4009 an festen und gezogenen Ecken bis 50000 s, vorher Grenze 1e-4); turbgpu WARP und Intel UHD
  Graphics: turb allein 0/131072, fuenf Ketten (turb, scale; turb, rotate, scale; scroll, turb, transform; turb, rotate, turb,
  scale; scale, scroll, rotate, transform) je 0/32768; hlsl 96/96 uebersetzt (vs_4_0 mit precise), gpushader, figurstufen gruen.
- Fotos auf dem Neubau (`f4_s_r3`, `f4_s_rd`, `f4_s_ep`) gegen `f3_s_*`: r3 8, rd 9, editorportal 15 bytegleich (auch
  `karteneditor_start.bmp`); die Testkarten zeigen kein turb - turb gegen das Spiel war FIX3 Punkt 2, jetzt per ctest bitgleich.

## FIX4 P34: Grenze des Animationsimports fest statt am freien Speicher

- Vorher (FIX2 P5): abgelehnt, sobald die Bilder mehr brauchten als `ullAvailPhys` - dieselbe Datei mit offenem Browser
  abgelehnt, nach dem Schliessen gelesen.
- Jetzt zwei feste Grenzen: (1) Zielformat GLA - je Bild und Knochen ein Verweis von 3 Byte (mdx_format.h:426-439), Bildzahl und
  alle Offsets int (:389-395): Bilder x Knochen x 3 muss in int passen (`FehlerArt::AnimZuLangGla`, neue Meldung "zu lang fuer
  eine GLA: Bilder x Knochen, hoechstens ..."); (2) der Arbeitsspeicher des Rechners (`ullTotalPhys`, fest je Rechner;
  `AnimZuLang` "braeuchten X MB Arbeitsspeicher, dieser Rechner hat Y MB"). Das Commit-Limit (`ullTotalPageFile`) waechst mit der
  Auslagerungsdatei und waere wieder vom Augenblick abhaengig. `bad_alloc` beim Abtasten bleibt gefangen (Meldung 2). Texte in 6
  Sprachen (`tools/i18n_fix2.py`, i18n mit gen_i18n.py erzeugt).
- Test: eine Jobgrenze aendert `GlobalMemoryStatusEx` nicht (gemessen: ullAvailPhys 43 GB unter 300-MB-Grenze, PROCESS_MEMORY und
  JOB_MEMORY, `Y:\werkzeuge\f2\f4\gms.py`); der ctest lenkt deshalb `GlobalMemoryStatusEx` in seiner Importtabelle um und taeuscht
  freien und gesamten Speicher vor, ohne den Rechner zu fuellen.
- Rot `f4_rot/f4_rot_ctest.txt` (ctest modell_animimport, 1000 s = 30001 Bilder x 8 Knochen, 34 MB): mit 16 MB und 1 MB frei
  abgelehnt "(16 MB free)" statt gelesen; 16 MB Arbeitsspeicher vorgetaeuscht: gelesen bzw. "8 MB free" - 4 FEHLER.
- Gruen `f4_p34_gruen.txt`: frei echt/16 MB/1 MB alle gelesen, 30001 Bilder, FNV 4afff20baa6cc0d4 gleich; 16 MB Arbeitsspeicher:
  abgelehnt (c = 16) bei freiem Speicher echt und 8 MB; GLA-Grenze bei 8 Knochen 89478485 Bilder: 2982616 s (89478482 Bilder,
  2147483568 Byte) nicht die GLA-Grenze, 2982616,5 s (89478498 Bilder) AnimZuLangGla; 2e7 s und 1e9 s AnimZuLangGla.

## FIX4 Schluss

- Sauberer Neubau (`--clean-first`, `f4_neubau.log`): 0 Warnungen. ctest 238/238 (`f4_s_ctest.txt`, neu: turbgpu).
- Selbsttests auf dem Neubau: modellblock ai 31/0, karteneditor r3 22/0, rd 20/0, editorportal 19/0, Modus 1 2635/0
  (`f4_schluss_summe.txt`); Fotos bytegleich wie oben.
- Lints (`f4_lints.txt`): wie nach FIX3 (portability 99, pruefcodes 11, fremdprogramm 1, efxflags ohne Tafel, sonst ok), keine
  Meldung zu geaenderten Dateien. Adressen ab fe1f374c3: 0.
- Nicht gemacht: Handbuch (kein Abschnitt beschreibt die tcMod-Rechnung oder die Importgrenze); Bildzeit allein (Vertexshader je
  Schritt mit Zweig, turbIndex etwa 30 Rechenschritte je turb und Achse) nicht gemessen - andere Agenten bauten und testeten.
- Gesehen, nicht geaendert: `defTabelle` (deformVertexes der Figurenstufe, gpushader.cpp) rechnet die Sinuswelle weiter mit `sin`
  der Grafikkarte statt der Tafel des Spiels; `waveValue` (src/wave.cpp: stretch, rgbGen/alphaGen wave) rechnet die Welle
  mit der Formel statt ueber die Tafeln wie WAVEVALUE (tr_shade_calc.cpp:30) - beides eigene Funde, nicht Teil von P33.

## FIX5 Punkt 1: waveValue ueber die Tafeln des Spiels

Auftrag `auftrag_fix5_0811.md`, Laufnamen `f5_`, Skripte `Y:\werkzeuge\f2\f5\`.

- Quelle (rd-vanilla, MD9 gleich): `WAVEVALUE` (tr_shade_calc.cpp:30) `base + table[Q_ftol((phase + floatTime * freq) *
  FUNCTABLE_SIZE) & MASK] * amplitude` in float, `EvalWaveForm` (:60: noise ueber `R_NoiseGet4f(0, 0, 0, (floatTime + phase) *
  frequency)`, random ueber `GetNoiseTime(refdef.time + phase) <= frequency`), `EvalWaveFormClamped` (:79), die Tafeln aus
  `R_InitFuncTables` (tr_init.cpp:1724-1747: triangle 0 -> 1 -> 0 -> -1 -> 0), `R_NoiseInit`/`R_NoiseGet4f`/`GetNoiseTime`
  (rd-common/tr_noise.cpp, `srand(1001)` und `rand()` der MSVC-Laufzeit), `RB_CalcWaveColor` (:689) und `RB_CalcWaveAlpha` (:724):
  die Farbe als Byte `(int)(255 * glow)`.
- Vorher: `waveValue` mit der Formel - `sin(t * 2 pi)` des Rechners statt der Tafel, triangle nur 0..1..0 (eine halbe Periode zu
  lang, nie negativ), noise als feste Mitte, `random` unbekannt (None); rgbGen/alphaGen wave als float ohne das Byte.
- Jetzt (`src/wave.cpp`): die fuenf Tafeln in float wie R_InitFuncTables (`funcTafel`, `gpu::sinTafel` nimmt dieselbe), der Index
  wie Q_ftol (`funcIndex`, ausserhalb von long 0 wie cvttss2si), noise mit `rauschen4` (R_NoiseGet4f mit der Zufallstafel des
  Spiels), `WaveFunc::Random` (`GetNoiseTime`, refdef.time aus den Sekunden: `spielZeitMs`), `waveValueClamped`, `waveFarbe` (das
  Byte). rgbGen wave und alphaGen wave nehmen `waveFarbe` in der Vorschau (mapview `batchStateAus`), an Figuren
  (`figurWelle`), im Echtzeit-Licht und im Menue-Editor; stretch und deformVertexes move nehmen `waveValue`.
- `tests/wavetest.cpp` wurde nie gebaut (stand in keiner CMake-Liste) - jetzt ctest `wavetest`, neu geschrieben: die Gegenprobe ist
  der Code des Spiels (WAVEVALUE, EvalWaveForm, Tafeln, R_NoiseGet4f mit MSVC-rand, RB_CalcWaveColor/Alpha), je Form 20000 Wellen
  aus einer festen Ziehung zu ganzen ms bis 1 h.
- Rot `f5_rot/f5_rot_wavetest.txt` (Bibliothek von 23e9b4ac6, neue Probe): 25 FEHL - waveValue sin 19989/20000, triangle 19993,
  square 8, sawtooth 17380, inversesawtooth 17320, noise 20000, random 20000 abweichend; Farbe (Byte) 5023 bis 14870 von 20000;
  "random" unbekannt; triangle bei 0,25/0,75 s, sinTable[256].
- Gruen `f5_gruen/f5_wavetest.txt`: alle 0 von 20000, 0 Fehlschlaege.
- deformVertexes wave an der Karte: siehe Punkt 2 (dieselbe Rechnung `verformeEcke` fuer Karte und Figuren).

## FIX5 Punkt 2: deformVertexes an Karte und Figuren wie RB_DeformTessGeometry

- Quelle: `RB_DeformTessGeometry` (tr_shade_calc.cpp:553) wendet die deformVertexes in der Reihenfolge des Shaders an:
  `RB_CalcDeformVertexes` (:129; bei frequency 0 `EvalWaveForm` fuer alle Ecken, sonst WAVEVALUE mit `phase + (x + y + z) *
  deformationSpread`), `RB_CalcDeformNormals` (:182; R_NoiseGet4f an xyz * 0.98f, +100/+200, dann `VectorNormalizeFast` mit
  `Q_rsqrt`), `RB_CalcBulgeVertexes` (:216; ohne speed und width `xyz += normal * height`, sonst `sinTable[(int)(FUNCTABLE_SIZE /
  (M_PI * 2) * (st[0] * width + now)) & MASK]`, now = refdef.time (int ms) * speed * 0.001f, M_PI als float, q_math.h:40),
  `RB_CalcMoveVertexes` (:263). ParseDeform: `deformationSpread = 1.0f / atof(token)` in double.
- Vorher: Karte - wave mit `frac()` und `sin(t * 2 pi)` der Grafikkarte fuer jede Form, frequency 0 wie jede andere, die Zeit aus
  gZeit; normal mit einer Streufunktion statt R_NoiseGet4f; bulge `sin(s * breite + Sekunden * tempo)`, auch ohne width/speed;
  move ueber waveValue (Formel); feste Reihenfolge wave, normal, bulge, move; wave und bulge entlang der ungewackelten Normalen.
  Figuren - eigene Fassung (`defTabelle`) mit `sin` der Grafikkarte, Index ohne precise, bulge mit Sekunden und 162,974661726 (double
  statt float), ohne normal, ohne noise bei frequency 0.
- Jetzt: `gpu::verformung` (gpuconst.cpp) rechnet, was fuer alle Ecken gleich ist, auf der CPU in float wie das Spiel (EvalWaveForm
  bei frequency 0, floatTime * frequency, bulge-now aus ganzen ms, move-Versatz, die Folge) und packt vier float4; die HLSL-Funktion
  `verformeEcke` (kVerformText, gpushader.cpp; Tafel kSinTafel, Zufallstafel kRauschTafel/kRauschPerm als Bitmuster, precise)
  rechnet je Ecke in der Reihenfolge des Shaders - in der Karte (`gDeform`, `gDeformSpread`, neu `gDeformArt` @23, `gBulge`,
  `gDeformMove`, `gDeformNormal`) und an der Figurenstufe (`gDef0..3`, kStufeFloats 72 -> 68). `DeformZusatz::folge` haelt die
  Reihenfolge aus dem Shader (shaderscript.cpp). wave mit frequency != 0 und move mit noise/random bleiben weg (TableForFunc bricht
  dort im Spiel mit ERR_DROP ab).
- Neuer ctest `defgpu` (tests/defgpu.cpp): die ECHTEN Vertex-Shader (vertexShaderHlsl mit den deform-Merkmalen, figurVertexShaderHlsl)
  als Compute-Shader, ihr `main` aus einem Kern gerufen (Kamera und Knochen Einheitsmatrix), Werte aus den Packern des Programms;
  dazu `verformeEcke` allein (Ecke UND Normale). Gegenprobe RB_DeformTessGeometry abgeschrieben; 12 Faelle x 8192 Ecken (alle fuenf
  Formen, frequency 0 mit sin und noise, bulge mit und ohne width/speed, move, "normal, dann wave", "wave, bulge, move"), Zeiten in
  ganzen ms bis 1 h; WARP und Intel UHD Graphics.
- Rot `f5_rot/f5_rot_defgpu.txt` (Stand 23e9b4ac6, Probe ohne verformung/folge, Figur gepackt wie das alte deformWerte): 37 FEHL -
  Karte jeder Fall (z. B. wave sin 8060/8192, bulge ohne width 8192, move 8192, normal dann wave 8192), Figur wave sin 359,
  frequency 0 406 (WARP), noise 8192, bulge 288, move 822, normal dann wave 8191, wave/bulge/move 97; Figur triangle/square/saw
  waren schon gleich (exakte Tafeln).
- Gruen `f5_gruen/f5_defgpu.txt`: alle 82 Pruefungen 0 Abweichungen (Karte, Figur, verformeEcke je WARP und Intel), 97 s.
- Die Uebersetzung: mit `[unroll]` ueber die vier Plaetze der Folge setzte fxc das Rauschen zwoelfmal ein (hlslprobe nach 6 min
  abgebrochen); jetzt `[loop]` - hlslprobe 104 Shader in 55 s.

## FIX5 Punkt 3: Tempo ohne Bildaenderung

- Vertexshader der Karte: nur so viele tcMod-Schritte, wie die Stufe hat (numTexMods stand schon im Shadernamen, wurde aber nicht
  genutzt - jeder Shader rechnete vier Schritte mit je vier Zweigen), und der turb-Teil (turbIndex, je turb und Achse rund 30
  Rechenschritte) nur mit dem neuen Merkmal `kTurb` (eine tcMod turb in der Kette, `_tb` im Namen). Ohne turb kommt die Art 2 nie
  vor; der Zweig faellt beim Uebersetzen weg. Gleiche Bits: ein Schritt hinter numTexMods hat die Art 0 und laesst st, wie es ist.
- Geprueft bitgleich: defgpu "tcMod im Vertex-Shader der Karte" (scroll; rotate, scale; scale, scroll, rotate, transform; turb;
  scroll, turb, transform - je 8192 Ecken, WARP und Intel: 0 abweichend gegen ComputeTexCoords); gpushader (Schrittzahl, kTurb),
  gpuconst, hlslprobe (tcMod 1..4 mit und ohne turb uebersetzt).
- Figurenstufe: ein Shader fuer alle (die Art kommt aus dem Konstantenpuffer, gleich fuer alle Ecken eines Aufrufs - ein
  einheitlicher Zweig); nicht in Varianten zerlegt.
- **Offen**: Bildzeit vorher/nachher - nicht gemessen, andere Agenten bauten und testeten. Messblock: `sh
  Y:/werkzeuge/f2/f5/messblock.sh <bau 23e9b4ac6> <bau fix2>` (leistung k+s, je zweimal im Wechsel, Summe
  `laeufe/f5_mess_summe.txt`).

## FIX5 Punkt 4: Spielfoto mit rgbGen wave und deformVertexes wave

- Testkarte `f5welle` (`Y:/werkzeuge/f2/f5/karte.py`, wie f3turb): geschlossener Raum 1024 x 1024 x 512, alle Innenflaechen
  `textures/f5/welle` - `surfaceparm nolightmap`, `q3map_tessSize 32` (Ecken zum Wogen), `deformVertexes wave 96 sin 0 10 0.1 0.35`,
  Stufe `rgbGen wave sin 0.6 0.35 0.2 0.3`; Muster 8 x 8 Felder.
- Spiel (`spiel.py`, wie FIX3): Movie Duels SP per Popen (vorher keine fremden MovieDuels-/jasp-Prozesse), ICARUS-Kamera MOVE -480 0 256,
  PAN 8 15, ZOOM 80, 1920 x 1440, `timescale 0.05`, `sv_fps 1000`, drei `screenshot_tga` (Lauf `f5s3`, level.time davor 8860, 8890,
  8928). Spielordner Movie Duels vorher = nachher.
- Vorschau (`vorschau.sh`, Selbsttest geschuetze wie FIX3, 63 Fotos 1 ms auseinander; der Schritt "Szene: nicht nach 3000 Bildern"
  meldet wie in FIX3 FEHLER, weil die Karte keine Figuren hat - die Fotos entstehen trotzdem), Ausschnitt 1920 x 1440 ab (623, 144).
- Ergebnis (`vergleich.py`): bestes Bild je Spielbild f5a 8810 ms, f5b 8843 ms, f5c 8880 ms (50, 47, 48 ms vor der gedruckten
  level.time wie in FIX3) - mittlere Abweichung 0,17 / 0,19 / 0,23 von 255, Pixel mit Abweichung ueber 48: 0,000 %. Gegenprobe Bau
  FIX2 d04f18e5d (Wellen und deformVertexes wie vor FIX5) zu denselben Zeiten: 0,60 / 0,60 / 0,73.
- Paarbilder (Spiel oben, Vorschau unten, ungeskaliert 1920 x 2880): `Y:/werkzeuge/f2/f5/laeufe/f5s3/vergleich/`
  `paar_f5[abc]_spiel_oben_vorschau_<ms>_unten.png`, Gegenprobe `paar_f5b_spiel_oben_vorschau_FIX2_d04f18e5d_unten.png`; angesehen
  (verkleinert und Ausschnitte): gleich, die Wellen der Waende und des Bodens liegen auf denselben Pixeln.
- Zwischenfall: die ersten zwei Starts (f5s1, f5s2) hatten einen beim Kopieren verdorbenen Launcher-Schluessel (sed "f3a" -> "f5a" traf
  "md-launcher-7f3a9c21"). Movie Duels startete deshalb seinen Launcher (2 x MovieDuels-Launcher.exe, Eltern = meine Spiel-PIDs) und
  beendete sich; die beiden Launcher liefen etwa 1 min, dann per taskkill /T beendet; der Launcher legte
  `%TEMP%\MovieDuels-Launcher-music.mp3` an (entfernt). Ob ein Fenster oder Musik auf shanks Schirm kam, ist nicht bekannt.
  Schluessel berichtigt; spiel.py beendet seither auch einen selbst gestarteten Launcher.

## FIX5 Nachtrag PRUEF5: P35, P36, P37, P38

- **P35** (GPU-Probe prueft anderen Bytecode): neue Hilfe `tests/vsstrom.h` laesst den Vertex-Shader selbst laufen (Stream-Output, Profil
  und Fahnen wie im Programm) und zerlegt den Bytecode (D3DDisassemble). Rot `f5_rot2/f5_rot_p35_turbgpu.txt`/`_defgpu.txt`: mit den
  echten Profilen vs_4_0 (Fahnen 0 und O3) und vs_4_1 O3 rechnete die Intel UHD Graphics NICHT bitgleich - turb 37953 von 131072,
  "turb, scale" 8737 von 32768, die Karte "normal, dann wave" 13 von 8192, tcMod-Ketten 1836 bis 5187 von 8192; WARP 0. Der Bytecode
  ist dabei richtig (vs_4_x ohne refactoringAllowed - mehr kann SM4 nicht ausdruecken; der Treiber haelt sich nicht daran). vs_5_0
  (precise je Anweisung, `mul [precise(x)]`) war auf beiden bitgleich. Behebung: Karte und Figur ab Merkmalsstufe 11_0 als vs_5_0
  (`kartenVsZiel`, gpumap_win32.cpp), Echtzeit-Licht vs3 ebenso (vs4 war schon vs_5_0). Gruen `f5_gruen2/turbgpu.txt` 84 ok,
  `defgpu.txt` 106 ok (alle Faelle als vs_5_0 auf WARP und Intel, Bytecode mit [precise]); vs_4_x unter 11_0 auf WARP gemessen, auf der
  Grafikkarte als Hinweis (sie nimmt es nicht). Offen: NVIDIA/AMD nicht gemessen (keine da).
- **P36** (stretch, "Bit fuer Bit"): stretch nimmt seit Punkt 1 EvalWaveForm ueber die Tafeln; dazu jetzt `p = 1.0f / Welle` ohne
  Pruefung wie RB_CalcStretchTexCoords (Welle 0 -> inf; vorher p = 1). Rot `f5_rot2/f5_rot_gpuconst.txt` ("stretch mit Welle 0"),
  gruen `f5_gruen2/gpuconst_probe.txt` (dazu 2858 Zeiten matrix/translate gleich). Changelog FIX4 (6 Sprachen) und gpuconst.h: bitgleich
  bei derselben Ecke; an Figuren und deformierten Flaechen kommt die Ecke aus der eigenen Haut-/Verformungsrechnung.
- **P37** (GLA-Grenze): `glaDateiGroesse` (gla.h) rechnet die Datei vorab; `schreibGla` lehnt ueber 2^31 - 1 Byte ab (statt
  ofsCompBonePool/ofsEnd still abzuschneiden), mit der Knochenzahl des geschriebenen Skeletts (nach dem Uebertragen die des Ziels); der
  Import zaehlt Kopf, Verzeichnis und Hierarchie mit. Rot `f5_rot2/f5_rot_animimport.txt`: 2982616 s (89478482 Bilder x 8 Knochen,
  2147483568 Byte Bildteil + 1536 Byte davor) wurde angenommen; gruen `f5_gruen2/animimport.txt`: abgelehnt "hoechstens 89478421
  Bilder", 2982600 s weiter angenommen; modell_dateien: glaDateiGroesse = geschriebene Datei, 89478485 Bilder x 8 und der volle Vorrat
  ueber der Grenze.
- **P38** (Tempo): mit Punkt 3 faellt der turb-Teil samt Sinustafel aus den Varianten ohne turb; dazu `[branch]` in `tcSchritt` (die Art
  ist je Aufruf gleich - die Figurenstufe und das Echtzeit-Licht rechnen ohne turb den turb-Teil nicht mit). Rot
  `f5_rot2/f5_rot_hlsl.txt` (Stand vor Punkt 3: "Sinustafel ohne turb drin" fuer 1..4 Schritte), gruen `f5_gruen2/hlslprobe.txt`
  (ohne turb weg, mit turb drin). Bildzeit offen (siehe Punkt 3).

## FIX5 Schluss

- Sauberer Neubau (`--clean-first`, `f5_neubau.log`, 22 min): 0 Warnungen. ctest 240/240 (`f5_s_ctest.txt`, neu: wavetest, defgpu).
- Selbsttests auf dem Neubau: karteneditor r3 22/0, rd 20/0, editorportal 19/0, figurlicht 8/0, Modus 1 2635/0 (`f5_schluss_summe.txt`).
- Fotos gegen `f4_s_*`: r3 8 und editorportal 15 bytegleich; rd 7 gleich, `rd_stufen_echtzeit.bmp`/`rd_stufen_spielbild.bmp` auf der
  Platte mit `rgbGen wave sin 0.5 0.5 0 0.5` um 1 bzw. 2 von 255 anders (68749 Pixel, nur diese Platte) - die Welle kommt jetzt aus
  der Tafel und als Byte wie RB_CalcWaveColor (Punkt 1, gewollt).
- Lints (`f5_lints.txt`): wie nach FIX4 (portability 100 -> nach dem Nachtrag von `<cstddef>` in wave.cpp 99 wie vorher, pruefcodes 11, fremdprogramm 1,
  efxflags ohne Tafel, sonst ok). Handbuch `--pruefen`: ok. Adressen ab fe1f374c3: 0.
- Nicht gemessen (andere Agenten bauten und testeten): Bildzeit vorher/nachher (Messblock `Y:/werkzeuge/f2/f5/messblock.sh`), die
  Uebersetzungszeit der Figurenstufe (der Figuren-Vertex-Shader enthaelt jetzt verformeEcke mit der Zufallstafel; hlslprobe uebersetzt
  104 Shader in 55 s). NVIDIA/AMD nicht gegengeprueft.
- Offen: die Reihenfolge der deformVertexes haelt jede Art einmal (zwei Zeilen derselben Art: die Engine wendet beide an, hier zaehlen
  die Werte der letzten); autosprite/autosprite2 an Figuren; die Atof-Rundung der Wellenwerte (strtof statt (float)atof - erst bei
  Zahlen mit sehr vielen Ziffern verschieden); unter Merkmalsstufe 11_0 bleibt vs_4_0 (dort ist precise nicht zugesichert).

## Changelog

## English

- Animation import: a file whose last key lies far away (a time in milliseconds instead of seconds) gives the message "animation too long" instead of ending the program.
- Animation import: errors and hints (no animation, axes, BVH sides and floor, non-uniform scale, front from the skeleton) appear in the language of the interface.
- Animation import: long takes are read much faster (a 10000-frame BVH in a quarter of the time), with the same result.
- Map editor: the real-time light computes the game look of each shader only once instead of every frame - same picture.
- Preview and map editor: "tcMod turb" acts at its place in the chain (a following "tcMod scale" scales the wave like in the game) and, like "tcMod rotate", reads the game's sine table.
- Preview and map editor: several "tcMod turb" in one shader stage each act at their own place, as in the game (before, only the last one counted).
- About window, guide and THIRD-PARTY.md name the parts ported from Blender (chamfer with segments, weight colours, octahedral bones, weight brush falloff) and their licence GPL-2.0-or-later.
- Preview and map editor: the tcMod chain is computed step by step like the game, bit for bit (turb with the game's sine table and index, rotate, scale, scroll, transform, stretch) - for the same vertex; on figures and on surfaces moved by deformVertexes the vertex itself comes from the preview's own skinning or deformation.
- Animation import: the limit no longer depends on how much memory is free at the moment - a file is too long only if it does not fit into a GLA or into the memory of the computer.
- Preview, figures, map editor and menu editor: shader waves (rgbGen and alphaGen wave, tcMod stretch, deformVertexes move) use the game's tables - "triangle" swings below zero as in the game, "noise" and "random" waves move as in the game, colours change in the same 1/255 steps.
- Preview and figures: deformVertexes wave, normal, bulge and move are computed per vertex as in the game and in the order of the shader; figures now also show deformVertexes normal.
- Preview: map vertex shaders compute only the tcMod steps of their stage, and the turbulence part only where a shader uses it - same picture.
- Preview and map editor: with feature level 11 the vertex shaders use shader model 5, so the game's rounding also holds on the graphics card (an Intel card was off in the last digit with shader model 4).
- Animation import: a take is also refused when the whole GLA file, not only its frames, would exceed the 2 GB limit of the format.

## Deutsch

- Animations-Import: eine Datei, deren letzter Schlüssel weit weg liegt (Zeit in Millisekunden statt Sekunden), gibt die Meldung „Animation zu lang“, statt das Programm zu beenden.
- Animations-Import: Fehler und Hinweise (keine Animation, Achsen, BVH-Seiten und -Boden, ungleichmäßige Skalierung, vorn aus dem Skelett) erscheinen in der Sprache der Oberfläche.
- Animations-Import: lange Aufnahmen werden viel schneller gelesen (eine BVH mit 10000 Bildern in einem Viertel der Zeit), mit gleichem Ergebnis.
- Karten-Editor: das Echtzeit-Licht rechnet das Spielbild jedes Shaders nur einmal statt in jedem Bild - gleiches Bild.
- Vorschau und Karten-Editor: „tcMod turb“ wirkt an seiner Stelle der Kette (ein folgendes „tcMod scale“ skaliert die Welle wie im Spiel) und liest wie „tcMod rotate“ die Sinustafel des Spiels.
- Vorschau und Karten-Editor: mehrere „tcMod turb“ in einer Shaderstufe wirken jeder an seiner Stelle wie im Spiel (bisher zählte nur der letzte).
- Fenster Über, Handbuch und THIRD-PARTY.md nennen die aus Blender portierten Teile (Chamfer mit Segmenten, Gewichtsfarben, Oktaeder-Knochen, Abfall des Gewichtspinsels) und ihre Lizenz GPL-2.0-or-later.
- Vorschau und Karten-Editor: die tcMod-Kette wird wie im Spiel Schritt für Schritt gerechnet, Bit für Bit (turb mit Sinustafel und Index des Spiels, rotate, scale, scroll, transform, stretch) - bei derselben Ecke; an Figuren und an Flächen, die deformVertexes bewegt, kommt schon die Ecke aus der eigenen Haut- bzw. Verformungsrechnung der Vorschau.
- Animations-Import: die Grenze hängt nicht mehr davon ab, wie viel Speicher gerade frei ist - zu lang ist eine Datei nur, wenn sie nicht in eine GLA oder nicht in den Arbeitsspeicher des Rechners passt.
- Vorschau, Figuren, Karten- und Menü-Editor: Wellen in Shadern (rgbGen und alphaGen wave, tcMod stretch, deformVertexes move) nehmen die Tafeln des Spiels - „triangle“ schwingt wie im Spiel unter null, „noise“- und „random“-Wellen bewegen sich wie im Spiel, Farben ändern sich in denselben 1/255-Schritten.
- Vorschau und Figuren: deformVertexes wave, normal, bulge und move werden je Ecke wie im Spiel und in der Reihenfolge des Shaders gerechnet; Figuren zeigen jetzt auch deformVertexes normal.
- Vorschau: die Vertex-Shader der Karte rechnen nur die tcMod-Schritte ihrer Stufe und den Turbulenz-Teil nur, wo ein Shader ihn nutzt - gleiches Bild.
- Vorschau und Karten-Editor: ab Merkmalsstufe 11 nutzen die Vertex-Shader Shader-Modell 5, damit die Rundung des Spiels auch auf der Grafikkarte gilt (eine Intel-Karte lag mit Shader-Modell 4 in der letzten Stelle daneben).
- Animations-Import: eine Aufnahme wird auch abgelehnt, wenn die ganze GLA-Datei, nicht nur ihre Bilder, die 2-GB-Grenze des Formats überschritte.

## 中文

- 动画导入: 最后一个关键帧位置很远的文件 (时间以毫秒而非秒表示) 会显示"动画过长"的提示, 而不再导致程序退出。
- 动画导入: 错误和提示 (没有动画、坐标轴、BVH 左右与地面、非均匀缩放、由骨架确定前方) 以界面语言显示。
- 动画导入: 长动作捕捉读取快得多 (10000 帧的 BVH 只需四分之一的时间), 结果相同。
- 地图编辑器: 实时光照对每个着色器的游戏外观只计算一次, 而不是每帧计算 - 画面相同。
- 预览和地图编辑器: "tcMod turb" 在链中自己的位置起作用 (随后的 "tcMod scale" 像游戏中一样缩放波动), 并像 "tcMod rotate" 一样读取游戏的正弦表。
- 预览和地图编辑器: 同一着色器阶段中的多个 "tcMod turb" 像游戏中一样各自在自己的位置起作用 (以前只有最后一个有效)。
- 关于窗口、手册和 THIRD-PARTY.md 列出从 Blender 移植的部分 (带分段的倒角、权重颜色、八面体骨骼、权重笔刷衰减) 及其许可 GPL-2.0-or-later。
- 预览和地图编辑器: tcMod 链像游戏中一样逐步计算, 逐位一致 (turb 使用游戏的正弦表和索引, rotate、scale、scroll、transform、stretch)——前提是顶点相同; 在角色和被 deformVertexes 移动的表面上, 顶点本身来自预览自己的蒙皮或变形计算。
- 动画导入: 限制不再取决于当前可用内存的多少 - 只有当文件无法存入 GLA 或超出计算机内存时才算过长。
- 预览、角色、地图编辑器和菜单编辑器: 着色器中的波形 (rgbGen 和 alphaGen wave、tcMod stretch、deformVertexes move) 使用游戏的函数表——“triangle”像游戏中一样摆动到零以下, “noise”和“random”波形像游戏中一样变化, 颜色以相同的 1/255 步长变化。
- 预览和角色: deformVertexes wave、normal、bulge 和 move 像游戏中一样逐顶点并按着色器中的顺序计算; 角色现在也显示 deformVertexes normal。
- 预览: 地图顶点着色器只计算其阶段的 tcMod 步骤, 只有在着色器使用湍流时才计算湍流部分——画面相同。
- 预览和地图编辑器: 在功能级别 11 上顶点着色器使用着色器模型 5, 使游戏的舍入在显卡上同样成立 (一块 Intel 显卡在着色器模型 4 下最后一位有偏差)。
- 动画导入: 当整个 GLA 文件 (而不仅是其帧) 超过该格式 2 GB 的上限时, 也会拒绝该动画。

## 日本語

- アニメーションのインポート: 最後のキーが遠く離れたファイル (時間が秒ではなくミリ秒) では、プログラムが終了せず「アニメーションが長すぎます」と表示されます。
- アニメーションのインポート: エラーとヒント (アニメーションなし、軸、BVH の左右と床、不均一なスケール、スケルトンから決めた前方) がインターフェースの言語で表示されます。
- アニメーションのインポート: 長いテイクの読み込みが大幅に速くなりました (10000 フレームの BVH が 4 分の 1 の時間)。結果は同じです。
- マップエディター: リアルタイムライトは各シェーダーのゲームでの見た目を毎フレームではなく一度だけ計算します - 画像は同じです。
- プレビューとマップエディター: 「tcMod turb」がチェーンの自分の位置で働き (後に続く「tcMod scale」がゲームと同じく波を拡大縮小)、「tcMod rotate」と同じくゲームのサインテーブルを使います。
- プレビューとマップエディター: 1 つのシェーダーステージにある複数の「tcMod turb」が、ゲームと同じくそれぞれ自分の位置で働きます (以前は最後の 1 つだけが有効でした)。
- バージョン情報、ガイド、THIRD-PARTY.md に Blender から移植した部分 (セグメント付きの面取り、ウェイトの色、八面体ボーン、ウェイトブラシの減衰) とそのライセンス GPL-2.0-or-later を記載しました。
- プレビューとマップエディター: tcMod チェーンをゲームと同じく 1 ステップずつ、ビット単位で同じに計算します (turb はゲームのサインテーブルとインデックス、rotate、scale、scroll、transform、stretch)(同じ頂点の場合。キャラクターと deformVertexes で動く面では、頂点そのものがプレビュー独自のスキニングまたは変形計算から来ます)。
- アニメーションのインポート: 制限がその時点の空きメモリに左右されなくなりました - GLA に収まらない場合かコンピューターのメモリに収まらない場合だけ長すぎると判断します。
- プレビュー、キャラクター、マップエディター、メニューエディター: シェーダーの波形 (rgbGen と alphaGen wave、tcMod stretch、deformVertexes move) はゲームの関数テーブルを使います。「triangle」はゲームと同じくゼロより下まで振れ、「noise」と「random」の波形はゲームと同じく動き、色は同じ 1/255 刻みで変わります。
- プレビューとキャラクター: deformVertexes の wave・normal・bulge・move を頂点ごとにゲームと同じく、シェーダー内の順序で計算します。キャラクターでも deformVertexes normal が表示されます。
- プレビュー: マップの頂点シェーダーはステージの tcMod ステップだけを計算し、乱流部分はシェーダーが使う場合だけ計算します (画像は同じ)。
- プレビューとマップエディター: 機能レベル 11 では頂点シェーダーにシェーダーモデル 5 を使い、ゲームの丸めがグラフィックカード上でも成り立ちます (Intel のカードはシェーダーモデル 4 で最後の桁がずれていました)。
- アニメーションのインポート: フレームだけでなく GLA ファイル全体が形式の 2 GB の上限を超える場合も拒否します。

## Español

- Importación de animaciones: un archivo cuya última clave está muy lejos (un tiempo en milisegundos en lugar de segundos) muestra el aviso "animación demasiado larga" en lugar de cerrar el programa.
- Importación de animaciones: los errores y avisos (sin animación, ejes, lados y suelo de BVH, escala no uniforme, delante según el esqueleto) aparecen en el idioma de la interfaz.
- Importación de animaciones: las tomas largas se leen mucho más rápido (un BVH de 10000 fotogramas en una cuarta parte del tiempo), con el mismo resultado.
- Editor de mapas: la luz en tiempo real calcula el aspecto de juego de cada shader una sola vez en lugar de en cada fotograma; la imagen es la misma.
- Vista previa y editor de mapas: "tcMod turb" actúa en su lugar de la cadena (un "tcMod scale" posterior escala la onda como en el juego) y, como "tcMod rotate", lee la tabla de senos del juego.
- Vista previa y editor de mapas: varios "tcMod turb" en una misma etapa de shader actúan cada uno en su lugar, como en el juego (antes solo contaba el último).
- La ventana Acerca de, la guía y THIRD-PARTY.md nombran las partes portadas de Blender (chaflán con segmentos, colores de peso, huesos octaédricos, caída del pincel de pesos) y su licencia GPL-2.0-or-later.
- Vista previa y editor de mapas: la cadena tcMod se calcula paso a paso como en el juego, bit a bit (turb con la tabla de senos y el índice del juego, rotate, scale, scroll, transform, stretch), con el mismo vértice; en las figuras y en las superficies que mueve deformVertexes el vértice ya sale del propio cálculo de piel o deformación de la vista previa.
- Importación de animaciones: el límite ya no depende de cuánta memoria esté libre en ese momento; un archivo solo es demasiado largo si no cabe en un GLA o en la memoria del equipo.
- Vista previa, figuras, editor de mapas y editor de menús: las ondas de los shaders (rgbGen y alphaGen wave, tcMod stretch, deformVertexes move) usan las tablas del juego: «triangle» baja de cero como en el juego, las ondas «noise» y «random» se mueven como en el juego y los colores cambian en los mismos pasos de 1/255.
- Vista previa y figuras: deformVertexes wave, normal, bulge y move se calculan por vértice como en el juego y en el orden del shader; las figuras ahora también muestran deformVertexes normal.
- Vista previa: los vertex shaders del mapa calculan solo los pasos tcMod de su etapa y la parte de turbulencia solo donde un shader la usa; la misma imagen.
- Vista previa y editor de mapas: con el nivel de características 11 los vertex shaders usan el modelo de shader 5, para que el redondeo del juego se cumpla también en la tarjeta gráfica (una tarjeta Intel se desviaba en la última cifra con el modelo 4).
- Importación de animaciones: una toma también se rechaza cuando todo el archivo GLA, no solo sus fotogramas, superaría el límite de 2 GB del formato.

## Čeština

- Import animací: soubor, jehož poslední klíč leží daleko (čas v milisekundách místo v sekundách), zobrazí hlášení „animace je příliš dlouhá“ místo ukončení programu.
- Import animací: chyby a upozornění (žádná animace, osy, strany a podlaha BVH, nerovnoměrné měřítko, předek z kostry) se zobrazují v jazyce rozhraní.
- Import animací: dlouhé záznamy se načítají mnohem rychleji (BVH s 10000 snímky za čtvrtinu času), se stejným výsledkem.
- Editor map: světlo v reálném čase počítá herní vzhled každého shaderu jen jednou místo v každém snímku - obraz je stejný.
- Náhled a editor map: „tcMod turb“ působí na svém místě v řetězci (následující „tcMod scale“ škáluje vlnu jako ve hře) a stejně jako „tcMod rotate“ čte sinusovou tabulku hry.
- Náhled a editor map: více „tcMod turb“ v jedné fázi shaderu působí každý na svém místě jako ve hře (dříve platil jen poslední).
- Okno O programu, příručka a THIRD-PARTY.md uvádějí části převzaté z Blenderu (zkosení se segmenty, barvy vah, osmistěnné kosti, útlum štětce vah) a jejich licenci GPL-2.0-or-later.
- Náhled a editor map: řetězec tcMod se počítá krok za krokem jako ve hře, bit po bitu (turb se sinusovou tabulkou a indexem hry, rotate, scale, scroll, transform, stretch) - při stejném vrcholu; u postav a ploch, které pohybuje deformVertexes, pochází už samotný vrchol z vlastního výpočtu kůže nebo deformace náhledu.
- Import animací: limit už nezávisí na tom, kolik paměti je právě volné - soubor je příliš dlouhý, jen když se nevejde do GLA nebo do paměti počítače.
- Náhled, postavy, editor map a editor menu: vlny v shaderech (rgbGen a alphaGen wave, tcMod stretch, deformVertexes move) používají tabulky hry – „triangle“ kmitá jako ve hře i pod nulu, vlny „noise“ a „random“ se pohybují jako ve hře, barvy se mění ve stejných krocích po 1/255.
- Náhled a postavy: deformVertexes wave, normal, bulge a move se počítají po vrcholech jako ve hře a v pořadí shaderu; postavy nyní ukazují i deformVertexes normal.
- Náhled: vertex shadery mapy počítají jen kroky tcMod své fáze a část turbulence jen tam, kde ji shader používá – stejný obraz.
- Náhled a editor map: od úrovně funkcí 11 používají vertex shadery model shaderů 5, aby zaokrouhlování hry platilo i na grafické kartě (karta Intel se s modelem 4 lišila v poslední číslici).
- Import animací: záznam se odmítne i tehdy, když by celý soubor GLA, nejen jeho snímky, překročil hranici formátu 2 GB.
