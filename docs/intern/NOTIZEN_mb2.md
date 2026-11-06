# Notizen Modell-Block Stufe 2: GLM-Schreiber, GLA/XSI aus G2C, animation.cfg, Karten-GLA und Plaetze (+ Modus-1-Kuerzel)

Auftrag `auftrag_mb2.md`. Direkt in `Y:\karte` auf `karte1` ab 5f332a3d2 (mb1a2), Build `build-test`, Laeufe `laeufe/mb3_*`
(Gegenlauf rc610 `mb3_rel610_allein`, Kuerzel `mb3_k1` bis `mb3_k5`, Rundlauf `mb3_rl_<spiel>`, Model-Tab `mb3_ui*`, Handbuch
`mb3_hb_*`, Schluss `mb3e_*`). Laufskript `lauf.py` wie mb1a2. kFassung bleibt rc610. Kein Spiel, kein 3ds Max, kein Blender
gestartet. G2C (`DennisHerrm/G2C-toolkit`, Stand 0b00bf40) im Scratchpad geklont, nur gelesen; nichts daraus ausgefuehrt.

## 0. Modus 1: die Zeilen der Kuerzel-Tabelle

- **Gegenlauf mit der rc610-Release-exe allein** (`mb3_rel610_allein`, `laeufe/rel610`): 2607 OK / 45 FEHLER - dieselben Zeilen
  ("die Zeile wartet auf eine Taste", "aufgenommen", "loest aus"). Im R3-Schluss (06.11. ~10:50) war dieselbe exe 2651/0. Also
  kein Fehler eines Stands, sondern etwas am Rechner.
- **Nachgestellt** in `kuerzel` (Modus mit den Kuerzel-Pruefungen, 6 Minuten): `mb3_k1` 29 FEHLER. Mit Protokollzeilen im
  Programm (wer hat den Fokus, welche Zeile wartet, was wird aufgenommen) zeigte `mb3_k2` (44 FEHLER) die Ursache: der Klick
  traf die Zeile ("Zeile Paste wartet"), 26 Bilder spaeter "aufgenommen GamepadLStickRight" - 14 Aufnahmen von Gamepad-Sticks
  (L/R, links/rechts/oben). Der Fokus lag die ganze Zeit beim Kuerzelfenster.
- **Ursache**: der Win32-Backend von ImGui fragt XInput in jedem Bild ab, auch wenn das Fenster im Hintergrund ist
  (`imgui_impl_win32.cpp` ImGui_ImplWin32_UpdateGamepads), und gibt Sticks als Tasten weiter (ab 10 % Ausschlag). Mit einem
  angeschlossenen Controller, der bewegt wird oder driftet, nahm die Kuerzel-Aufnahme den Stick als Belegung - ein echter
  Programmfehler (ein Anwender mit Controller bekaeme "GamepadLStickRight" als Kuerzel), im Test zeitabhaengig (wann bewegt sich
  der Stick?), daher allein 25, parallel 51 und am Vormittag 0.
- **Behoben**: die Aufnahme uebergeht Gamepad-Tasten wie schon Maus- und Zusatztasten (`ImGui::IsGamepadKey`, app.cpp
  drawKeyBindings). Dazu eine feste Pruefung im Selbsttest `kuerzel` (und damit Modus 1): waehrend die Zeile Delete wartet,
  ein Gamepad-Stick nach rechts (io.AddKeyAnalogEvent) - die Zeile muss weiter warten, Delete unveraendert.
  Rot `mb3_k3_rot` (Stand ohne Behebung): 201 / 1 FEHLER, im Protokoll "aufgenommen GamepadLStickRight". Gruen `mb3_k5`
  (parallel mit zwei anderen Selbsttests): 202 / 0.
- Die Protokollzeilen bleiben (Detailprotokoll): Aufnahme abgebrochen mit dem Fenster, das den Fokus hat; Zeile wartet;
  aufgenommen; Esc. Die Pruefung "wartet auf eine Taste" schreibt bei Fehlschlag Fokus, Fenster unter der Maus und Mausort.

## 1. GLM-Schreiber

- `bhed/modell/glmdatei.h` (`src/modell/glmdatei.cpp`): `glmSchreiben(Inhalt)` schreibt den Aufbau von Carcass/G2C
  (G2C `src/mdxm.cpp` writeMdxm; mdx_format.h): Kopf, Verzeichnis, Hierarchie, je LOD Flaechenkopf, Dreiecke, Ecken, UV,
  Knochenverweise. Gemessen vor dem Bau (`scratchpad/mb3/glm_analyse.py` ueber alle Archive): jede .glm aus JKA, JK2 und
  Movie Duels hat genau diesen Aufbau, aber die Datei traegt mehr, als das Spiel liest - das letzte Gewicht jeder Ecke
  (bei 30 % nicht 1023 - Summe), unbenutzte Gewichtsfelder und Knochenbits, Rest hinter der Null der Namen (90 von 127
  JKA-Dateien), Knochenverweise nicht aufsteigend (Movie Duels 16754 Flaechen) oder mit unbenutzten Knochen, eine
  doppelte Kinderzeile (Movie Duels nahdar: hips nennt hips_tunic zweimal), 64 Zeichen lange Namen ohne Null.
- Deshalb traegt das Dokument die Rohfelder (`glmRohfelder`, von `ausGlm` gerufen): je Ecke `dateiPack`/`dateiGewicht`, je
  Flaeche und Stufe die Verweise der Datei (vor und nach dem Umlegen JK2<->JKA), Namen/Shader als 64 Bytes, die Kinderliste.
  Der Schreiber nimmt sie, solange die Werte passen (Ecke: die Felder ergeben genau die Knochen/Gewichte, die readGlm daraus
  liest; Name: gleich), sonst kodiert er neu wie writeMdxm (Gewichte > 0 in ihrer Reihenfolge, 10 Bit gerundet, das letzte
  mitgeschrieben; Verweise: die der Datei, wenn sie alle benutzten Knochen enthalten, sonst aufsteigend). Ein umgelegtes
  Modell (JK2-Modell unter JKA) wird unveraendert mit den Verweisen der Datei geschrieben, geaendert ganz mit dem Skelett
  des Dokuments (der Bericht sagt es).
- Grenzen, die das Format setzt (Fehler, nicht schreibbar): mehr als 32 Knochen an einer Flaeche, Namen ueber 63 Zeichen
  (ausser genau so in der Datei).
- Model-Tab: Datei > Modell speichern (Strg+S, BehavEd Alt+S) und Speichern unter (Strg+Umschalt+S, Alt+V). Aus einer Datei
  geoeffnet schreibt Strg+S dorthin, aus den Archiven fragt es nach einer Datei. Vorher die Grenzen des Zielspiels
  (modellBefunde: Ecken/Dreiecke je Flaeche, 32 Knochen, 4 Gewichte, Ghoul2-Instanzen, Animationen hinter dem Ende); liegt
  etwas darueber, fragt ein eigenes Fenster (kein Windows-Dialog) mit Trotzdem speichern / Abbrechen. Geschrieben wird
  ueber sicherSchreiben (Temp-Datei, atomar). Erreichbar ueber Menue, Aktionssuche (X/F3), Rechtsklick, Tooltips.

## 2. GLA, animation.cfg, Karten-GLA, Plaetze, XSI

- **GLA**: `schreibGla` (gla.cpp) im Aufbau von writeMdxa (G2C `src/mdxa.cpp`): Kopf, Verzeichnis, Skelett mit Flaggen und
  Kinderliste (neu gelesen), Bildverweise je 3 Bytes, auf 4 ausgerichtet, Vorrat unveraendert. Rest hinter der Null des
  Namens bleibt, solange der Name gleich ist.
- **Frame-Zahl `_humanoid.gla`**: die Datei des Spiels hat 21376 Frames (JKA, Kopf und erste Zeile der animation.cfg "21376
  frames; 1417 sequences", 53 Knochen), JK2 17278 (989 Sequenzen, 72 Knochen), Movie Duels 60861 (2635 Zeilen, "erzeugt von
  g2c"). Die 30384 der G2C-README stammen aus Ravens SDK-Quellen (`_humanoid_info.txt`/`_humanoid.car`, 1683 Sequenzen, ein
  anderer Bau; G2C `docs/CAR_FORMAT.md` "Versionsversatz im Beispielmaterial"), nicht aus der ausgelieferten GLA.
- **animation.cfg** (`bhed/modell/animcfg.h`): Zeilen mit Kommentaren und Zeilenenden; unveraendert Byte fuer Byte,
  geaendert im Format der Raven-Datei (`%-20s\t%d\t%d\t%d\t%d`). Sie gehoert ins Modelldokument (`Inhalt::cfg`): die
  **Marker der Zeitleiste** sind ihre Zeilen - unter dem Bildregler erster Frame, Anzahl, loopFrames, fps der gewaehlten
  Sequenz als Zahlen und als Balken, dessen Enden man zieht; jede Aenderung ein Rueckgaengig-Schritt, die Animationsliste
  folgt. Die Leiste des Model-Tabs hat dafuer eine vierte Zeile.
- **Pruefungen** (`cfgPruefen`, je Zielspiel): hinter dem Ende der GLA (G2_bones.cpp:838; T-Pose in MD/OpenJK, Standpose im
  Original), firstFrame/numFrames 0..65535 und loopFrames -1..127 (die asserts in G_ParseAnimationFile, unsigned short /
  signed char), Dateigroesse gegen den Puffer des Lesers (JKA SP 80000, MP 60000, JK2 40000 je minus 2; MD SP keine, MD MP
  524288), Namen ausserhalb der Animationsliste. Karten-GLA: 53 Knochen, Name im Kopf = Ordner, nur SP (codemp/codeJK2 kennen
  kein `_humanoid_%s`). MAX_ANIM_FILES 16 (JKA) / 64 (JK2, MD). Plaetze BOTH_CIN_1..50 (JKA), 1..125 (MD9 anims.h; der
  Stufenplan nannte 126).
- **Ziel-GLA erst beim Bauen** (Nachtrag shank 06.11.): Feld Ziel mit Erklaerung je Ziel, jederzeit wechselbar; die
  Belegung (Platz -> Quelle) gilt fuer alle Ziele.
  - Karten-GLA `_humanoid_<karte>`: Plaetze BOTH_CIN_n; eine vorhandene Karten-GLA der Spieldaten wird beim Bauen
    zusammengefuehrt (freie Plaetze aus ihr), nicht ueberschrieben.
  - `_humanoid.gla` fuers ganze Spiel: die GLA bleibt ganz, jede Zeile ihrer animation.cfg (und BOTH_CIN_n) ist ein Platz,
    die neuen Frames kommen hinten dazu (`anhaengen`); nie ins Spielverzeichnis (`modellImSpielordner`), im Mehrspieler der
    Hinweis Server/Client.
  - Eigene GLA fuer ein eigenes Skelett: `models/players/<name>/<name>`, Name im Kopf = Ordner, die .glm nennt sie danach
    (glaPfad im Dokument, beim naechsten Speichern); im Mehrspieler gemeldet.
  - Zusammenfuehren: die Vorratseintraege (14 Bytes) werden unveraendert uebernommen und gleiche geteilt - jede Quelle
    braucht dieselben Knochennamen und dieselbe Grundstellung (sonst bedeuteten die Eintraege etwas anderes).
- **XSI** (Carcass-Weg): der dotXSI-Leser, das Auswerten gegen ein Skelett und das Packen der Knochen aus G2C, unveraendert
  unter `third_party/g2c` (GPL-2.0, Stand 0b00bf40, eigenes Ziel ohne unsere Warnungspolitik wie imgui). `xsiAufnehmen`
  nimmt das Skelett der GLA (Massstab wie carbuild.cpp:173) und legt das Ergebnis auf den naechsten freien Platz.
  Geprueft im ctest mit dem Max-Beispiel aus G2C `tests/tests.cpp` (30 Grad um Z an Bild 3, fehlender Knochen gemeldet).

## Rot und gruen

- **Modus 1 Kuerzel** (Punkt 0): rot rc610-Release-exe allein 45 FEHLER, `mb3_k1` 29, `mb3_k2` 44 (14x "aufgenommen Gamepad..."),
  feste Gamepad-Pruefung `mb3_k3_rot` 1 FEHLER; gruen `mb3_k5` 202/0, Schluss `mb3e_kuerzel` 202/0, Modus 1 unter Last und
  allein siehe Schluss.
- **GLM-Rundlauf** (`BHED_MODELL_NUR=rundlauf`): erster Lauf Movie Duels (`mb3_rl_md`) 919 von 924 gleich - 4 Dateien mit
  64 Zeichen langem Namen ohne Null abgewiesen (holo.glm, holo2.glm, electrostaff.glm, w_saber.glm), nahdar/model.glm mit
  4 Bytes weniger (die doppelte Kinderzeile in hips). Mit der Kinderliste und den 64-Zeichen-Namen der Datei: `mb3_rl_md2`
  924 / 924 Byte fuer Byte.
- **Speichern im Model-Tab** (`BHED_MODELL_NUR=mb3`): `mb3_ui1` rot 4 - die Frage im Programm wurde nie gezeichnet
  (drawModellSpeichernFrage fehlte im Bild), das Ziehen am Markerbalken ohne Wirkung (die Leiste hatte drei Zeilen, die
  Markerzeile lag unter dem Rand). Gruen `mb3_ui2` 16/0; mit den drei Zielen `mb3_ui3` 21/0.
- **Handbuchfotos angesehen**, je rot im Foto, gruen danach: die Frage nannte nach Rueckgaengig ohne gezeichnetes Bild noch
  eine alte Zeile ("ROOT endet bei 60871") - modellSpeichern gleicht die Animationsliste jetzt vorher ab; im Bereich Animation
  stand die Anzeige vor den Plaetzen (jetzt dort ab Werk zu); "Crear para el juego..." ragte ueber den Rand (die Knoepfe
  brechen jetzt um); die Namen der Ziele waren in der Auswahl abgeschnitten (kuerzer, die Erklaerung steht darunter).
- Die Pruefungen im ctest `modell_dateien` (neu) und die uebrigen Zeilen der Gruppe mb3 waren beim ersten Lauf gruen (kein
  Rotlauf mit abgeschaltetem Weg).

## Rundlauf GLM/GLA (Schluss `mb3e_rl_*`, je Datei, wie das Spiel sie findet)

| Spiel | .glm | Byte fuer Byte | .gla | Byte fuer Byte |
|---|---|---|---|---|
| JKA (GameData original) | 127 | 127 | 66 | 66 |
| Jedi Outcast | 71 | 71 | 21 | 21 |
| Movie Duels | 924 | 924 | 121 | 120 + 1 nicht lesbar |

Die eine GLA ist `models/map_objects/gunship/republicgunship.gla`: im Text-Modus uebertragen (jede Null ein Leerzeichen),
das Spiel scheitert an ihr ebenso (looksTextMangled). Zusammen 1122 .glm und 208 .gla.
Nebenbefund: Movie Duels nahdar fuehrt hips_tunic zweimal als Kind von hips; RenderSurfaces folgt childIndexes und zeichnet
sie im Spiel zweimal (gleiches Bild), JK Studio einmal (Kinder aus den Eltern) - nicht geaendert.

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele, `mb3e_bau.txt`): 0 Warnungen, 0 Fehler. ctest -j16: 223 / 223 (neu
  `modell_dateien`: GLM-Probe Byte fuer Byte, Gewicht geaendert neu kodiert und wie das Spiel gelesen, 32/33 Knochen, 63/64
  Zeichen, GLA mit Rest hinter dem Namen, animation.cfg mit CRLF/Kommentaren/ohne Schlussumbruch, jede Grenze knapp darunter
  und darueber, Karten-GLA aus zwei Quellen, XSI).
- Lints: alle ok bis auf portability 99 (wie mb1a1/mb1a2), pruefcodes 11, efxflags, fremdprogramm - unveraendert, keine Datei
  dieses Auftrags. lint_sources fand die G2C-Quellen in CMakeLists als "src/*.cpp" - jetzt ueber file(GLOB).
- Selbsttests parallel (unter Last, zusammen mit Modus 1 und den Handbuchfotos): modellblock funktionen 60/0, tasten 43/0,
  mb2 37/0, blender 44/0, mb3 21/0, rundlauf jka/jk2/md je 2/0 (Zahlen oben), modell 14/0, mission1 98/0, kuerzel 202/0;
  danach bild jka 77/77, jk2 41/41, md 547 gleich + 5 wie im Spiel anders (wie mb1a1/mb1a2).
- **Modus 1**: unter Last (`mb3e_mod1_last`, parallel mit 17 anderen Laeufen) 2635 / 0, allein (`mb3e_mod1_allein`) 2635 / 0.
  Die Zahl ist kleiner als 2651 vor mb1a2, weil dort die 20 Teiler-Zeilen des Model-Tabs durch eine ersetzt wurden; dazu
  kommen die drei Gamepad-Zeilen.
- Bildzeit allein (`modellleistung`, BHED_D3D_DEBUG=0) gegen die exe von mb1a2 im selben Zustand: Summe der 23 Mittel
  16.31 (mb1a2) / 16.55 ms (jetzt), Sturmtruppler Animation 0.39/0.64 gegen 0.42/0.81 (Mittel/p99), jedi_hf ruhig 0.84/3.05
  gegen 0.75/1.17, clonetrooper_p2 Animation 1.43/11.15 gegen 1.17/2.19 - innerhalb der Streuung zwischen zwei Laeufen; neu
  ist eine Zeile der Leiste (Marker).
- Handbuch: Seite modell-dateien in 6 Sprachen, Bilder h_mb3_animation/befund/speichern (2538 x 1472, alle angesehen).

## Offen

- Reste aus 1a-2 nicht klein, nicht gemacht: .mvs/.ideal (ModView) und Stapelumwandlung (die Umwandlung kann jetzt den
  GLM-Schreiber nutzen); Movie Duels imperial/jedi_zm zweite Flaeche `hips` mit "*off" - das Spiel zeichnet sie mit dem
  Default-Shader (16x16, Grau 32 mit weissem Rand, wie ihn der Menue-Editor schon nachbaut), JK Studio in der Ersatzfarbe;
  dafuer muesste der Figurenweg den "<default>"-Shader samt seiner Stufe (rgbGen) bekommen.
- Kein Spielversuch (Auftrag): ob eine von JK Studio gebaute Karten-GLA, `_humanoid.gla` im Mod-Ordner und eine eigene GLA im
  Spiel laden und spielen, ist nur ueber die Dateien belegt (neu gelesen, gleiche Pose Bild fuer Bild). Aufbau `ms0` liegt
  bereit.
- XSI: geprueft mit dem kleinen Max-Beispiel aus G2C; ein echter Export aus Softimage/Max/Blender lag nicht vor.
- MAX_ANIM_FILES im Ziel "eigene GLA" rechnet mit zwei Saetzen (_humanoid und das eigene); die Saetze einer ganzen Szene
  (alle Figuren einer Mission) zaehlt erst Stufe 7.
- nahdar: hips nennt hips_tunic zweimal (siehe Rundlauf); im Mission-Tab einmal gezeichnet, im Spiel zweimal - bildgleich.
- Die neuen Pruefzeilen des ctest `modell_dateien` und die meisten der Gruppe mb3 waren beim ersten Lauf gruen (kein
  eigener Rotlauf mit abgeschaltetem Weg).

## Vorschlag fuer Stufe 3 (Rig) und 3b (Animationswerkzeuge)

- Stufe 3: Knochen-Erkennung je Rolle (Wurzel, Becken, Wirbel, Kopf, Arme/Beine l/r, Haende, Fuesse, Saebelgriffe ueber die
  Tags *r_hand/*l_hand) aus Namen und Hierarchie; gemessen als Trefferquote ueber alle Skelette der drei Spiele (JKA 53,
  JK2 72, MD-Sonderskelette 43/59). Control Rig als eigene Objekte im Szenen-Explorer, Griffe im Max-Modus als Bone-Kasten
  mit Gizmo, im Blender-Modus als Oktaeder; "Backen" schreibt ueber `xsiAufnehmen`-Weg (compressBone) Frames in eine GLA,
  die dann wie hier auf ein Ziel geht. Rundlauf: Rig anlegen, nichts aendern, backen -> Pose je Bild innerhalb der
  Kompression gleich (14 Bytes, MC_UnCompressQuat).
- Stufe 3b: die Marker dieser Stufe sind schon die animation.cfg-Zeilen - darauf Dopesheet und Kurveneditor (Schluessel je
  Knochen aus den Vorratseintraegen), Bewegungspfade in 3D (Prio A: Schluessel auf der Bahn ziehen), Pose kopieren/gespiegelt
  (l_/r_ wie die Gewichte in 1a-2), Animation spiegeln, Aufraeumen, Timing. Backen wieder ueber compressBone, Ziel waehlen wie
  hier.

## Changelog

### English
- Model tab: save the model as .glm (Ctrl+S, Save as Ctrl+Shift+S) with every level of detail, hierarchy, tags, weights, flags and shaders. An unchanged model is written byte for byte as it was - checked with every .glm of Jedi Academy (127), Jedi Outcast (71) and Movie Duels (924). Before saving, the limits of the target game are checked; above a limit JK Studio asks in its own window and can still save.
- Model tab, area Animation: the GLA of the model and its animation.cfg can be saved (unchanged byte for byte; every .gla of the three games checked), with checks per target game - sequences behind the end of the GLA (T-pose), frames above 65535, loopFrames, file size per game, unknown names.
- Timeline markers: the line of animation.cfg for the selected sequence is shown below the frame slider - edit first frame, count, loop and fps or drag the ends of the bar; one undo step each.
- Build for the game: choose the target only when building and switch it at any time - map GLA _humanoid_<map> (BOTH_CIN slots, an existing map GLA is merged), _humanoid.gla for the whole game (only into a mod folder, never into the game folder) or an own GLA for an own skeleton (the .glm then names it). Drag sequences from the animation list onto the slots or use the right-click; the frames are taken over exactly.
- Record XSI: a dotXSI animation from Softimage, 3ds Max or Blender is recorded against the skeleton the way Carcass does it (with code from G2C) and lands on the next free slot.
- Fix: a connected game controller no longer becomes a keyboard shortcut (the shortcut recorder took a moving stick as "GamepadLStickRight").

### Deutsch
- Model-Tab: das Modell als .glm speichern (Strg+S, Speichern unter Strg+Umschalt+S) mit allen Detailstufen, Hierarchie, Tags, Gewichten, Flaggen und Shadern. Ein unverändertes Modell wird Byte für Byte wie vorher geschrieben - geprüft an jeder .glm von Jedi Academy (127), Jedi Outcast (71) und Movie Duels (924). Vor dem Speichern prüft JK Studio die Grenzen des Zielspiels; darüber fragt es in einem eigenen Fenster und kann trotzdem speichern.
- Model-Tab, Bereich Animation: die GLA des Modells und ihre animation.cfg lassen sich speichern (unverändert Byte für Byte; jede .gla der drei Spiele geprüft), mit Prüfungen je Zielspiel - Sequenzen hinter dem Ende der GLA (T-Pose), Frames über 65535, loopFrames, Dateigröße je Spiel, unbekannte Namen.
- Marker in der Zeitleiste: die Zeile der animation.cfg zur gewählten Sequenz steht unter dem Bildregler - erster Frame, Anzahl, Schleife und fps ändern oder die Enden des Balkens ziehen; je ein Rückgängig-Schritt.
- Für das Spiel bauen: das Ziel erst beim Bauen wählen und jederzeit wechseln - Karten-GLA _humanoid_<karte> (Plätze BOTH_CIN, eine vorhandene Karten-GLA wird zusammengeführt), _humanoid.gla fürs ganze Spiel (nur in einen Mod-Ordner, nie ins Spielverzeichnis) oder eine eigene GLA für ein eigenes Skelett (die .glm nennt sie dann). Sequenzen aus der Animationsliste auf die Plätze ziehen oder per Rechtsklick; die Frames werden genau übernommen.
- XSI aufnehmen: eine dotXSI-Animation aus Softimage, 3ds Max oder Blender wird wie von Carcass gegen das Skelett aufgenommen (mit Code aus G2C) und landet auf dem nächsten freien Platz.
- Behoben: ein angeschlossener Controller wird kein Tastenkürzel mehr (die Kürzel-Aufnahme nahm einen bewegten Stick als "GamepadLStickRight").

### 中文
- 模型选项卡:将模型保存为 .glm (Ctrl+S,另存为 Ctrl+Shift+S),包含所有细节级别、层次、标签、权重、标志和着色器。未修改的模型逐字节按原样写出——已在 Jedi Academy (127)、Jedi Outcast (71) 和 Movie Duels (924) 的每个 .glm 上检查。保存前检查目标游戏的限制;超出时 JK Studio 在自己的窗口中询问,仍可保存。
- 模型选项卡,动画区域:可以保存模型的 GLA 及其 animation.cfg (未修改时逐字节相同;已检查三款游戏的每个 .gla),并按目标游戏检查——超出 GLA 末尾的序列 (T 姿势)、超过 65535 的帧、loopFrames、各游戏的文件大小、未知名称。
- 时间轴标记:所选序列在 animation.cfg 中的行显示在帧滑块下方——修改第一帧、帧数、循环和 fps,或拖动条的两端;每次一个撤销步骤。
- 为游戏生成:只在生成时选择目标,并可随时更改——地图 GLA _humanoid_<地图> (BOTH_CIN 槽位,现有地图 GLA 会被合并)、整个游戏的 _humanoid.gla (只能放入 Mod 文件夹,绝不放入游戏目录) 或自有骨架的自有 GLA (之后 .glm 会引用它)。把动画列表中的序列拖到槽位上或使用右键;帧会被准确沿用。
- 录入 XSI:来自 Softimage、3ds Max 或 Blender 的 dotXSI 动画按 Carcass 的方式对照骨架录入 (使用 G2C 的代码),并放到下一个空闲槽位。
- 修复:连接的游戏手柄不再变成快捷键 (快捷键录制曾把移动的摇杆记为 "GamepadLStickRight")。

### 日本語
- モデルタブ: モデルを .glm として保存 (Ctrl+S、名前を付けて保存 Ctrl+Shift+S)。すべての詳細レベル、階層、タグ、ウェイト、フラグ、シェーダーを含みます。変更していないモデルはバイト単位で元どおりに書かれます - Jedi Academy (127)、Jedi Outcast (71)、Movie Duels (924) のすべての .glm で確認済み。保存前に対象ゲームの上限を確認し、超えると JK Studio 自身のウィンドウで尋ね、それでも保存できます。
- モデルタブのアニメーション領域: モデルの GLA とその animation.cfg を保存できます (変更なしならバイト単位で同じ。3 本のゲームのすべての .gla で確認)。対象ゲームごとのチェック付き - GLA の末尾を超えるシーケンス (T ポーズ)、65535 を超えるフレーム、loopFrames、ゲームごとのファイルサイズ、未知の名前。
- タイムラインのマーカー: 選択シーケンスの animation.cfg の行がフレームスライダーの下に表示されます - 最初のフレーム、数、ループ、fps を変えるか、バーの端をドラッグ。それぞれ元に戻す 1 回分です。
- ゲーム用に作成: 出力先は作成時に選び、いつでも切り替えられます - マップ GLA _humanoid_<マップ> (BOTH_CIN スロット、既存のマップ GLA は結合)、ゲーム全体の _humanoid.gla (Mod フォルダーにだけ、ゲームフォルダーには入れません)、独自スケルトン用の独自 GLA (その後 .glm が参照)。アニメーション一覧のシーケンスをスロットへドラッグするか右クリック。フレームは正確に引き継がれます。
- XSI の取り込み: Softimage、3ds Max、Blender の dotXSI アニメーションを Carcass と同じようにスケルトンに対して取り込み (G2C のコードを使用)、次の空きスロットに置きます。
- 修正: 接続したゲームコントローラーがショートカットにならなくなりました (ショートカットの記録が動いたスティックを "GamepadLStickRight" として記録していました)。

### Español
- Pestaña Model: guarda el modelo como .glm (Ctrl+S, Guardar como Ctrl+Mayús+S) con todos los niveles de detalle, jerarquía, tags, pesos, marcas y shaders. Un modelo sin cambios se escribe byte a byte como era; comprobado con cada .glm de Jedi Academy (127), Jedi Outcast (71) y Movie Duels (924). Antes de guardar se comprueban los límites del juego de destino; por encima de un límite JK Studio pregunta en su propia ventana y puede guardar igualmente.
- Pestaña Model, área Animación: el GLA del modelo y su animation.cfg se pueden guardar (sin cambios, byte a byte; comprobado cada .gla de los tres juegos), con comprobaciones por juego de destino: secuencias más allá del final del GLA (pose T), fotogramas por encima de 65535, loopFrames, tamaño del archivo por juego, nombres desconocidos.
- Marcadores en la línea de tiempo: la línea de animation.cfg de la secuencia elegida aparece bajo el control de fotograma; cambia primer fotograma, número, bucle y fps o arrastra los extremos de la barra; un paso de deshacer cada vez.
- Crear para el juego: el destino se elige al crear y se puede cambiar en cualquier momento: GLA de mapa _humanoid_<mapa> (huecos BOTH_CIN, un GLA de mapa existente se combina), _humanoid.gla para todo el juego (solo en una carpeta de mod, nunca en la carpeta del juego) o un GLA propio para un esqueleto propio (el .glm lo nombra después). Arrastra secuencias de la lista de animaciones a los huecos o usa el clic derecho; los fotogramas se toman exactamente.
- Grabar XSI: una animación dotXSI de Softimage, 3ds Max o Blender se graba contra el esqueleto como lo hace Carcass (con código de G2C) y va al siguiente hueco libre.
- Corregido: un mando conectado ya no se convierte en un atajo de teclado (la grabación de atajos tomaba un stick en movimiento como "GamepadLStickRight").

### Čeština
- Karta Model: uložte model jako .glm (Ctrl+S, Uložit jako Ctrl+Shift+S) se všemi úrovněmi detailu, hierarchií, tagy, váhami, příznaky a shadery. Nezměněný model se zapíše bajt po bajtu jako dřív - ověřeno na každém .glm z Jedi Academy (127), Jedi Outcast (71) a Movie Duels (924). Před uložením se kontrolují limity cílové hry; nad limitem se JK Studio zeptá ve vlastním okně a může přesto uložit.
- Karta Model, oblast Animace: GLA modelu a jeho animation.cfg lze uložit (beze změny bajt po bajtu; ověřeno každé .gla tří her), s kontrolami pro cílovou hru - sekvence za koncem GLA (T-póza), snímky nad 65535, loopFrames, velikost souboru podle hry, neznámá jména.
- Značky na časové ose: řádek animation.cfg vybrané sekvence je pod posuvníkem snímků - změňte první snímek, počet, smyčku a fps nebo táhněte konce pruhu; každá změna jeden krok zpět.
- Sestavit pro hru: cíl se volí až při sestavení a lze ho kdykoli změnit - mapové GLA _humanoid_<mapa> (sloty BOTH_CIN, existující mapové GLA se sloučí), _humanoid.gla pro celou hru (jen do složky modu, nikdy do složky hry) nebo vlastní GLA pro vlastní kostru (.glm ho pak uvede). Sekvence ze seznamu animací přetáhněte na sloty nebo použijte pravé tlačítko; snímky se převezmou přesně.
- Zaznamenat XSI: animace dotXSI ze Softimage, 3ds Max nebo Blenderu se zaznamená proti kostře tak jako v Carcassu (s kódem z G2C) a skončí na dalším volném slotu.
- Opraveno: připojený herní ovladač se už nestane klávesovou zkratkou (záznam zkratek bral pohybující se páčku jako "GamepadLStickRight").
