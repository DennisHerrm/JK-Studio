# Notizen AP7 (11./12.11.): Bereich UV - das rechte Feld nach dem freigegebenen Entwurf - MBENTWURF Teil B, rc612

Begonnen von AP7 (Fable 5.2, zwei WIP-Stände auf `b19b7f494`), fertig gemacht von Opus 5.5 nach `Y:\werkzeuge\ap7\UEBERGABE.md`: auf
karte1 `1685e8ca3` (AP3 + AP9) gesetzt, Testfehler behoben, Ergänzung des Bausteins (`KnopfOpt::tasteHalten`), Feinschliff, Läufe,
Fotos, Handbuchbilder, Notizen.

Auftrag `auftrag_mbentwurf_b_1011.md`, Paket AP7; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`,
Bereich UV), shanks Wünsche vom 04.11. (Bild 1: „kann man das noch schöner machen? einfach benutzerfreundlicher und übersichtlicher“) und
10.11. („es geht ja nicht nur um controls like blender oder 3ds max sondern das aussehen“) und die Abgleichtabelle
`abgleich_programm_entwurf.md`, Abschnitt 9 (U0a-U10; U11 ist mit erledigt). Worktree `Y:\p_ap7`, Zweig `p_ap7`, begonnen auf dem
Schnittstellen-Commit von AP3 `b19b7f494` (Baustein `gui/ui_rollout.h`), fertig auf karte1 `1685e8ca3`. Selbsttest `gui/selbsttest_ap7.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap7 | ap7foto`), Skripte und Läufe `Y:\werkzeuge\ap7\`.

## Was sich ändert

Das rechte Feld des Bereichs UV (`drawModellUvBereich`, `gui/wof/app_modelltab_uv.cpp`) zeichnet über den Baustein von AP3 und steht im
Reiter, den der Bereich vorgibt (wie 3ds Max: Ändern, wie Blender: Daten). Vorher: fünf ImGui-Gruppen „Unwrap UVW / Seams / Unwrap /
Arrange and pack / Check“ mit Knöpfen in Textbreite, Befunden als Sätze und der grauen Tastenzeile am Ende.

| Gruppe | Inhalt (immer sichtbar) | unter „Weitere“ (eingeklappt) |
|---|---|---|
| **Auswahl** (Kurzinfo „<Textur bzw. Fläche> · n Inseln“) | Textur (Klappliste), Zeile „n Flächen, n Dreiecke, LOD n“, Ebenen als Segment mit der Taste des Bedienmodus (1 Ecke · 2 Kante · 4 bzw. 3 Fläche · 5 bzw. 4 Insel), Raster **Insel wählen** · **Alles wählen**, Haken UV-Editor | – |
| **① Nähte festlegen** | Raster Naht markieren · Naht löschen, volle Breite Nähte aus dem Skelett, Tipp „Für Figuren: erst Nähte aus dem Skelett, dann von Hand nachbessern.“ | Nähte nach Form · Nähte nach Winkel · Ring aufschneiden · Nähte spiegeln, Regler Winkel |
| **② Abwickeln** | **Hauptknopf Abwickeln**, Raster Schnell abwickeln · Projizieren, Segment Vorlage (Figur / Requisit / Harte Fläche), Segment Projektion (Eben / Zylinder / Kugel / Kasten) | Neu lösen · Pinnen · Alle Pins lösen, Segment Verfahren (LSCM / minimale Dehnung), Segment Achse (X / Y / Z / Passend) |
| **③ Aufräumen** | Raster Entspannen · Dichte angleichen, volle Breite Packen, Regler Abstand 0-16 px, Drehen ☑ erlaubt (derselbe Haken `ui::schalter` wie in Prüfen) | Begradigen · Rechteck · Gespiegelt stapeln · Ausrichten · Sperren, Segment „Was packen“ (Automatisch / Auswahl / Alles) |
| **Prüfen** (Kurzinfo grün „alles gut“ bzw. gelb „n Befunde“) | Schalter in zwei Spalten: Schachbrett · Dehnung zeigen · Überlappungen zeigen · **Nähte dick**; Tabelle Dehnung · Überlappungen · (Umgeklappte Dreiecke, nur wenn es welche gibt) · **Ausnutzung der Textur** · Texel-Dichte | Tabelle Inseln · Gespiegelte Stapel · Umgeklappte Dreiecke · Offene Ringe · Winkelfehler · Konform · Flächenabweichung · Dichte min/Median/max |
| Tipp am Ende | „JKA speichert einen UV-Satz je Ecke. Noch nicht abgewickelte Nähte und Pins werden nicht mit dem Modell gespeichert.“ | |

Neu als Aktionen (Menü UV, Rechtsklick > UV, Alle Aktionen suchen, Tooltip):
- **Insel wählen**: erweitert die Auswahl auf ihre ganzen Inseln (Blender `uv.select_linked`, 3ds Max „Element“); steht die Maus im
  UV-Editor über einem Dreieck (Taste L wie Blender, `uv.select_linked_pick` mit `extend`), kommt dessen Insel dazu.
- **Alles wählen**: jede UV-Ecke der Flächen der Textur (3ds Max Strg+A, Blender A).
- **Nähte dick**: Nähte im UV-Editor mit dickem Strich (ein Sechstel der Zeichenhöhe, mindestens 4 statt 2 Bildpunkte).
- Tasten wie Blender nach dem Entwurf: **L** Insel, **A** alles, **Strg+A** Dichte angleichen, **Strg+P** Packen.
- **Ausnutzung der Textur** (`UvBefunde::ausnutzung`, `src/modell/wof/uv_pruefen.cpp`): Anteil der Texel der Kachel 0..1, den mindestens
  eine Insel bedeckt - gezählt im Raster der Überlappungsprüfung (Größe der Textur), gestapelte Inseln einmal, UV außerhalb der Kachel nicht.

Eigene Symbole (Block am Ende von `enum class UiIcon`, Zeichnung in `gui/uiicons.cpp`): UvInsel, UvAlles, UvSchere, UvNahtWeg,
UvProjektion, UvDichte, UvPacken, UvPin, UvPinWeg, UvGerade, UvWinkel, UvRing, UvSpiegel, UvEntspannen (Welle statt des Monds „Ruhig“).

**Ergänzung des Bausteins** (`gui/ui_rollout.*`, API kompatibel, eingetragen in `Y:\werkzeuge\ap3\SCHNITTSTELLE.md`):
`KnopfOpt::tasteHalten` (Vorgabe `false`, letztes Feld). Mit `true` bleibt die Taste am Knopf, solange jedes Wort neben ihr in die Zeile
passt; der Text bricht um und die Reihe wird höher (Entwurf: „Dichte angleichen“ zweizeilig mit Strg+A). CJK bricht zwischen den Zeichen,
dort muss das breiteste Zeichenpaar passen (die Sperrregeln von `textumbruch.h` halten höchstens zwei Zeichen zusammen). Ohne das Feld
gilt die alte Regel (Taste weg, sobald dem Text weniger als min(Textbreite, 4 Zeichenhöhen) bleibt) - AP3, AP9 und die übrigen Bereiche
zeichnen unverändert. Alle Knöpfe des Bereichs UV setzen es (`feldKnopf`).

Texte: neue in `tools/i18n_ap7.py`; 13 seit dem neuen Feld unbenutzte aus `tools/i18n_uv1b.py` entfernt (UvRolloutMax, UvRolloutBlender,
UvTastenMax, UvTastenBlender, UvDrehen, UvInselnZahl, UvPrUeberlappt, UvPrGekippt, UvPrGedehnt, UvPrRinge, UvPrDichte, UvMass,
UvMassDichte); lint_i18n ok.

## Bewusst anders als Entwurf bzw. Vorschlag der Tabelle

- **U2, Ebenen:** der Entwurf zeigt drei Ebenen (1 Ecke, 2 Kante, 3 Fläche) und „Insel wählen“ als Knopf. Das Programm behält seine vierte
  Ebene **Insel** im Segment (ein Klick im UV-Editor wählt dann ganze Inseln - wie 3ds Max „Element“, Blender Taste 4) UND bekommt den Knopf
  „Insel wählen“ als Befehl. Nichts geht verloren, beide Wege des Entwurfs sind da.
- **U8, Tasten wie 3ds Max:** das Segment zeigt 4 für Fläche und 5 für Insel (Karte von 3ds Max: 3 ist der Rand) - wie schon in Teil A als
  bewusst vermerkt; der Entwurf nennt 3.
- **U9, Strg+A / Strg+P wie Blender:** der Entwurf verlangt sie; in Blenders heutiger Belegung haben Dichte angleichen und Packen keine
  Taste (nur Menü UV), Strg+A / Strg+P stehen in `blender_default.py`, `km_uv_editor`, nur unter `params.legacy` (Belegung von Blender 2.7x).
  Umgesetzt wie im Entwurf; nachgelesen am 11.11. in
  https://github.com/blender/blender/blob/main/scripts/presets/keyconfig/keymap_data/blender_default.py (L `uv.select_linked_pick`
  extend, Strg+L `uv.select_linked`, A `uv.select_all` SELECT).
- **U4, Hauptknopf wie Blender:** am Knopf steht wie im Entwurf die Taste U; in Blender (und im Programm) öffnet U das Menü Abwickeln, dessen
  erster Eintrag Abwickeln ist - der Tooltip sagt das.
- **U4, Schnell abwickeln / Projizieren:** der Entwurf hat je einen Knopf; das Programm hat drei Vorlagen und vier Projektionen. Der Knopf
  führt die darunter im Segment gewählte aus (Vorgabe der Vorlage: mit Skelett Figur, sonst Requisit - „Figur“ weist ein Modell ohne
  Gliedmaßen ab). Die sieben Einzelaktionen bleiben in Menü, Rechtsklick und Suche.
- **U6, Wörter der Schalter:** „Dehnung zeigen“ / „Überlappungen zeigen“ wie die Aktionen in Menü und Suche (Entwurf: „Dehnung farbig“,
  „Überlappungen“) - ein Name je Funktion.
- **U6, Bewertung in Worten:** „gering“ heißt: keine Insel hat ein über 1,5 gedehntes Dreieck (die vorhandene Grenze aus WhiteoutFlakes,
  `PruefOptionen::dehnungsGrenze`); sonst steht die Zahl der Inseln da. „gleich“ bei der Texel-Dichte heißt: keine Insel hat die halbe oder
  doppelte mittlere Dichte (vorhandene Regel der Prüfung); in Klammern die größte Abweichung vom Median. Keine neuen Schwellen erfunden.
- **U6, Nähte dick:** wirkt im UV-Editor. Die Naht-Striche der 3D-Ansicht zeichnet `ueberlagerung` in `gui/app_modelltab_ansicht.cpp`
  (Datei der Nachbarn AP8/AP3) mit festen 2 Bildpunkten - dort genügt nach dem Zusammenführen `modellUvNahtDicke()` statt `2.0F`.
- **U7, Text des Tipps:** der Entwurf sagt „Weitere Sätze bleiben nur im Projekt.“ - das Programm kennt keinen zweiten UV-Satz
  (`include/bhed/modell/dokument.h`: „JKA kennt je Ecke genau einen UV-Satz“). Der zweite Satz nennt stattdessen, was wirklich nicht in der
  Datei landet: noch nicht abgewickelte Nähte und Pins.
- **U7, Tastenzeile:** die graue Tastenzeile am Ende des Felds ist weg; die Tasten des Bereichs stehen in der Statuszeile (AP1, R8) und an
  den Knöpfen. Die letzte Meldung stand im Bereich UV doppelt (eigene Zeile und die des Felds) - jetzt einmal.
- **U0b:** der Blender-Aufbau (Werkzeugleiste T, Outliner oben rechts, N-Leiste) ist AP4; hier nur das Feld im Stil wie Blender, Reiter Daten.

## Selbsttest (rot -> grün)

- Rot (`ap7rot`, exe des alten Felds + neuer Selbsttest): `mb_ap7` 11 OK, 109 FEHLER; ctest `modell_uv` 3 rote Zeilen (Ausnutzung nicht
  gerechnet). Schon vorher grün: U1, U10, Naht markieren samt Rückgängig, zweiter Klick auf „erlaubt“.
- Erster grüner Lauf (`ap7g1`, Stand auf `b19b7f494`): 79 OK, 17 FEHLER - zwei Ursachen, beide behoben, keine Prüfung abgeschwächt:
  - **Testfehler U4** (3 Prüfungen, 8 Folge-Klicks, 2 Zeilen „nichts ging verloren“): `klick("Unwrap", "mtrollouts")` traf die
    Gruppenüberschrift „Unwrap###uvrabw“ (das Element-Register vergleicht auch den sichtbaren Titel) und klappte die Gruppe zu. Jetzt
    `klickAn` auf das Element mit genau der Beschriftung des Hauptknopfs.
  - **U5 wie Blender** (1 Prüfung, echter Befund): an „Match density“ fehlte Strg+A - der Baustein ließ die Taste im schmaleren
    Blender-Feld weg. Behoben mit `KnopfOpt::tasteHalten` (oben); die Prüfung verlangt weiter genau 2 Tasten in der Gruppe.
- Grün auf karte1 (`ap7k`, `ap7m` nach der CJK-Regel): **88 OK, 0 FEHLER** - dieselben 88 Prüfzeilen wie im Lauf `ap7g1` (dort 79 OK +
  9 rot), die 8 Zeilen Unterschied waren nur die fehlgeschlagenen Klicks.
- ctest `modell_uv`: 4 neue Zeilen Ausnutzung (169/4096, gestapelt 169/4096, außerhalb 0, ganze Kachel 1), grün.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

- `gui/selbsttest_uv.inc`, Prüfung „UV Tasten je Modus wie die Tabellen“: Packen wie Blender `""` -> `"Ctrl+P"`; neu in der Liste: Dichte
  `D` / `Ctrl+A`, Insel wählen `""` / `L`, Alles wählen `Ctrl+A` / `A`.
- `gui/selbsttest_mr.inc`, Prüfung „MR Blender UV: jede Max-Taste hat eine Blender-Taste (oder Blender hat keine)“: `UvDichte` und `UvPacken`
  stehen nicht mehr in der Liste der Aktionen ohne Blender-Taste.
- Unverändert grün gebraucht: `rig3::rollenZu(tr(Str::UvGrPruefen), "mtrollouts")` der Handbuchbilder (`uvhb`) - die Gruppe „Check“ behält
  deshalb ihre Kennung (Titel ohne `###`).

## Fotos

`Y:\werkzeuge\ap7\fotos\` (Selbsttest `ap7foto`, 3818 × 2104 bei Oberfläche 1,5; je Variante 0 FEHLER): en, de, zh-Hans, ja, es, cs,
hell, klein (1328 × 844 bei 1,0). Neben dem Entwurf: `vergleich\v_ap7_f_<variante>_<max|blender>_feld.png` (en auch `_auf`),
Mindestgröße `vergleich\v_klein_blender_seiten.png`. Angesehen:
- Max und Blender en: Aufbau wie Entwurf (Kurzinfo „body.tga · 31 islands“, ① ② ③, Raster mit Symbol und Taste, blauer Hauptknopf,
  Prüfen mit Schaltern in zwei Spalten und Tabelle, Tipp am Ende); Blender: Panels rund, „Match density“ zweizeilig mit Ctrl+A wie im
  Entwurf.
- ja vor der CJK-Regel: an „シームを設定“ und „密度を揃える“ fehlte die Taste (Text ohne Leerzeichen) - mit der Zeichenpaar-Regel
  jetzt da („密度を揃 / える Ctrl+A“, Umbruch nach den Sperrregeln); zh-Hans, cs, de, es: nichts abgeschnitten, lange Texte brechen um.
- hell: Hauptknopf und Segmente in der Betonungsfarbe des hellen Themas, Haken gleich.
- klein: Raster und Segmente gehen auf eine Spalte, nichts abgeschnitten; wo ein Wort neben der Taste keinen Platz hat, steht die Taste
  nur im Tooltip.

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap7.py` (6 Sprachen): Abschnitt „Das Feld rechts: Auswahl und drei Schritte“ für die Seite
  „Model-Tab: UV erstellen“ (`einhaengen(seite)` setzt ihn vor „Auswahl an beiden Stellen“), Absatz zur Gruppe Prüfen, Hinweis zu den neuen
  Tasten; Bilder `h_ap7_max`, `h_ap7_blender` (`BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap7`) in 6 Sprachen unter
  `handbuch/bilder/<sprache>/` (2538 × 1472, angesehen en Max und ja Blender; je Lauf 0 FEHLER). Die Bilder der Seite UV
  (`BHED_HB_NUR=uv`) laufen mit dem neuen Feld weiter (en 0 FEHLER).
- Nicht eingehängt: AP14 führt das Handbuch zusammen (`inhalt.py`, `inhalt_es.py`, `inhalt_cs.py`).
  Die vorhandenen Bilder `h_uv_editor` / `h_uv_schnell` zeigen bis dahin noch das alte Feld; ihr Text (`inhalt_uv1b.py`) nennt nur Befehle,
  die es weiter gibt.

## Schlussrunde

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen |
| ctest | 256 / 256 |
| `ap7` | 88 / 0 |
| Nachbargruppen (`ap7k_summe.txt`) | uv 51 · uvmd3 4 · blender 68 · ap3 105 · ap9 97 · funktionen 62 · tasten 44 - je 0 FEHLER |
| nach der CJK-Regel (`ap7m_summe.txt`) | ap7 88 / 0 · ap3 105 / 0 |
| Fotos `ap7foto` | 8 Varianten, je 0 FEHLER |
| Handbuchbilder | ap7 in 6 Sprachen, uv en - je 0 FEHLER; `gen_handbuch.py --pruefen` ok (der Hinweis „2 Einträge in uebersetzung/ nicht mehr gebraucht“ steht auf karte1 genauso) |
| Lints (21 aus `lints.sh`) | ok; pruefcodes 11, portability 99 wie karte1 |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen, karteneditor, menueeditor,
UV-Rundläufe (`netzAus`/`zurueckschreiben` unverändert), sauberer Neubau.

Beim Setzen auf karte1 kamen zwei Fehler aus dem Zusammenführen der Listen (beide Seiten behalten): eine fehlende Klammer im Zweig
`BHED_HB_NUR=ap9` (`selbsttest_handbuch.inc`, der Bau brach ab) und ein fehlendes `break` nach dem letzten Painting-Symbol in
`uiicons.cpp` (`MalMaskeUmkehr` hätte zusätzlich das Symbol Insel wählen gezeichnet) - beide behoben.

## Offen

- U0b: Blender-Aufbau (Werkzeugleiste T, Outliner oben rechts, N-Leiste) - AP4.
- Nähte dick in der 3D-Ansicht: `ueberlagerung` (`gui/app_modelltab_ansicht.cpp`) zeichnet fest 2 Bildpunkte; nach dem Zusammenführen
  dort `modellUvNahtDicke()` einsetzen (Datei von AP8/AP3, nicht angefasst).
- Handbuch einhängen und die alten Bilder `h_uv_editor` / `h_uv_schnell` neu aufnehmen: AP14.

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP7 nicht angefasst)

| Zeile | Stand |
|---|---|
| U0a Aufbau wie 3ds Max | gleich: Reiter Ändern, Auswahl, ① ② ③, Prüfen, Tipp, flacher Stil |
| U0b Aufbau wie Blender | Feld gleich (Reiter Daten, runde Panels); Werkzeugleiste/Outliner/N-Leiste offen - AP4 |
| U1 UV-Fenster + 3D-Ansicht | gleich (war schon gleich) |
| U2 Auswahl | gleich (Kurzinfo, Insel wählen, Alles wählen); bewusst mehr: Ebene Insel im Segment, Textur-Zeile |
| U3 ① Nähte | gleich (Nummer, Raster, Skelett-Knopf, Tipp); mehr unter Weitere |
| U4 ② Abwickeln | gleich (Hauptknopf blau mit U, Raster, Projektion als Segment); bewusst: Vorlage als zweites Segment |
| U5 ③ Aufräumen | gleich (Nummer, Raster, Packen voll, Regler, Haken; Blender Strg+A/Strg+P am Knopf) |
| U6 Prüfen | gleich (Kurzinfo, 4 Schalter, Tabelle mit Ausnutzung); bewusst: Wörter der Schalter wie die Aktionen |
| U7 Tipp | bewusst anders: zweiter Satz nennt, was wirklich nicht gespeichert wird; Tastenzeile in der Statuszeile |
| U8 Max-Tasten | bewusst (4 Fläche, 5 Insel wie 3ds Max; Strg+A alles) |
| U9 Blender-Tasten | gleich (L, A, Strg+A, Strg+P) |
| U10 kurze Zeitleiste | gleich (AP1) |
| U11 Texte | gleich (Auswahl · ① Nähte festlegen · ② Abwickeln · ③ Aufräumen · Prüfen) |

## Changelog

## English

- Model tab, UV area: the panel on the right follows the approved design - Selection, then the numbered steps 1 Set seams, 2 Unwrap, 3 Clean up, then Check. Buttons of equal width in two columns with symbol and the key of the control mode, Unwrap as the highlighted main button, a tip box in the steps.
- Selection shows the texture and the number of islands in its heading; new: Select island (extends the selection to whole islands) and Select all.
- Quick unwrap and Project are one button each; the template (Character, Prop, Hard surface) and the projection (Planar, Cylinder, Sphere, Box) are chosen right below. Commands needed less often are folded away under More in each step.
- Check: the heading says "all good" or the number of findings; stretch, overlaps, the new texture use in percent and the texel density stand as a table; new switch Thick seams for the UV editor.
- Like Blender: L selects the island under the mouse, A selects everything, Ctrl+A matches the density, Ctrl+P packs. Like 3ds Max: Ctrl+A selects everything.

## Deutsch

- Model-Tab, Bereich UV: das Feld rechts folgt dem freigegebenen Entwurf - Auswahl, dann die nummerierten Schritte 1 Nähte festlegen, 2 Abwickeln, 3 Aufräumen, dann Prüfen. Gleich breite Knöpfe in zwei Spalten mit Symbol und der Taste des Bedienmodus, Abwickeln als hervorgehobener Hauptknopf, ein Tippkasten in den Schritten.
- Auswahl zeigt in der Überschrift die Textur und die Zahl der Inseln; neu: Insel wählen (erweitert die Auswahl auf ganze Inseln) und Alles wählen.
- Schnell abwickeln und Projizieren sind je ein Knopf; die Vorlage (Figur, Requisit, Harte Fläche) und die Projektion (Eben, Zylinder, Kugel, Kasten) wählt man direkt darunter. Seltener gebrauchte Befehle stehen in jedem Schritt eingeklappt unter Weitere.
- Prüfen: die Überschrift sagt „alles gut“ oder die Zahl der Befunde; Dehnung, Überlappungen, die neue Ausnutzung der Textur in Prozent und die Texel-Dichte stehen als Tabelle; neuer Schalter Nähte dick für den UV-Editor.
- Wie Blender: L wählt die Insel unter der Maus, A wählt alles, Strg+A gleicht die Dichte an, Strg+P packt. Wie 3ds Max: Strg+A wählt alles.

## 中文

- 模型选项卡的 UV 区域:右侧面板按已确认的设计排列——“选择”,然后是带编号的步骤 1 设置接缝、2 展开、3 整理,最后是“检查”。按钮等宽、分两列,带图标和当前操作模式的按键,“展开”是突出显示的主按钮,步骤中有提示框。
- “选择”在标题中显示纹理和岛的数量;新增:选择岛(把选择扩展到整个岛)和全选。
- 快速展开和投影各为一个按钮;模板(角色、道具、硬表面)和投影方式(平面、圆柱、球体、长方体)就在下方选择。不常用的命令折叠在每个步骤的“更多”中。
- 检查:标题显示“一切正常”或问题数量;拉伸、重叠、新增的纹理利用率(百分比)和纹素密度以表格显示;新增用于 UV 编辑器的开关“加粗接缝”。
- Blender 模式:L 选择鼠标下的岛,A 全选,Ctrl+A 统一密度,Ctrl+P 打包。3ds Max 模式:Ctrl+A 全选。

## 日本語

- モデルタブの UV エリア: 右のパネルが承認済みのデザインどおりになりました。「選択」、番号付きの手順 1 シームを決める・2 展開・3 仕上げ、そして「チェック」。ボタンは同じ幅の 2 列で、アイコンと操作モードのキー付き。「展開」は強調されたメインボタンで、手順にはヒントの枠があります。
- 「選択」は見出しにテクスチャとアイランドの数を表示します。新機能: アイランドを選択(選択をアイランド全体に広げる)とすべて選択。
- クイック展開と投影はそれぞれ 1 つのボタンになり、テンプレート(キャラクター、小道具、ハードサーフェス)と投影方法(平面、円柱、球、ボックス)をすぐ下で選びます。あまり使わないコマンドは各手順の「その他」に折りたたまれています。
- チェック: 見出しに「問題なし」または指摘の件数。ストレッチ、重なり、新しいテクスチャ使用率(パーセント)、テクセル密度が表で並びます。UV エディター用の新しいスイッチ「シームを太く」。
- Blender モード: L でマウス下のアイランドを選択、A ですべて選択、Ctrl+A で密度をそろえ、Ctrl+P でパック。3ds Max モード: Ctrl+A ですべて選択。

## Español

- Pestaña Model, área UV: el panel de la derecha sigue el diseño aprobado: Selección, luego los pasos numerados 1 Fijar costuras, 2 Desplegar, 3 Ordenar y por último Comprobar. Botones de igual ancho en dos columnas con símbolo y la tecla del modo de control, Desplegar como botón principal destacado y un cuadro de consejo en los pasos.
- Selección muestra en su cabecera la textura y el número de islas; nuevo: Seleccionar isla (amplía la selección a islas enteras) y Seleccionar todo.
- Despliegue rápido y Proyectar son un botón cada uno; la plantilla (Personaje, Accesorio, Superficie dura) y la proyección (Plana, Cilindro, Esfera, Caja) se eligen justo debajo. Los comandos menos usados quedan plegados bajo Más en cada paso.
- Comprobar: la cabecera dice «todo bien» o el número de avisos; el estiramiento, los solapamientos, el nuevo uso de la textura en porcentaje y la densidad de téxeles aparecen como tabla; nuevo interruptor Costuras gruesas para el editor UV.
- Como Blender: L selecciona la isla bajo el ratón, A selecciona todo, Ctrl+A iguala la densidad, Ctrl+P empaqueta. Como 3ds Max: Ctrl+A selecciona todo.

## Čeština

- Karta Model, oblast UV: panel vpravo odpovídá schválenému návrhu - Výběr, potom očíslované kroky 1 Určit švy, 2 Rozbalit, 3 Uklidit a nakonec Kontrola. Stejně široká tlačítka ve dvou sloupcích se symbolem a klávesou režimu ovládání, Rozbalit jako zvýrazněné hlavní tlačítko, v krocích rámeček s tipem.
- Výběr ukazuje v záhlaví texturu a počet ostrovů; nově: Vybrat ostrov (rozšíří výběr na celé ostrovy) a Vybrat vše.
- Rychlé rozbalení a Promítnout jsou po jednom tlačítku; šablona (Postava, Rekvizita, Tvrdý povrch) a projekce (Rovinná, Válec, Koule, Kvádr) se volí hned pod nimi. Méně používané příkazy jsou v každém kroku sbalené pod Další.
- Kontrola: záhlaví říká „vše v pořádku“ nebo počet nálezů; natažení, překryvy, nové využití textury v procentech a hustota texelů jsou jako tabulka; nový přepínač Tlusté švy pro editor UV.
- Jako Blender: L vybere ostrov pod myší, A vybere vše, Ctrl+A sjednotí hustotu, Ctrl+P zabalí. Jako 3ds Max: Ctrl+A vybere vše.
