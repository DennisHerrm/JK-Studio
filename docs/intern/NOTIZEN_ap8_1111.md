# Notizen AP8 (11./12.11.): Bereich Weights nach dem Entwurf, Gewichts-Werkzeuge wie 3ds Max und Blender - MBENTWURF Teil B, rc612

Begonnen von AP8 (Fable 5.2, drei WIP-Stände auf `b19b7f494`: Bibliothek mit ctest `modell_gewichte2`, Feld, Selbsttest, Handbuchquelle;
GUI nie gebaut, Selbsttest nie gelaufen), fertig gemacht von Opus 5.5 nach `Y:\werkzeuge\ap8\UEBERGABE.md`: auf karte1 `440ebdffb` (AP3, AP9,
AP7, AP6) gesetzt, gebaut, rot gegen das alte Feld, neues Feld angeschlossen, Funde der Läufe behoben, Fotos, Handbuchbilder, Notizen.

Auftrag `auftrag_mbentwurf_b_1011.md`, Paket AP8; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`,
Bereich Weights), shanks Wünsche vom 04.11. („gibt es auch die möglichkeiten Vertex zu mirroren? also weights bei skinning?“, Bild 1 „schöner
… übersichtlicher“) und 10.11. („es geht ja nicht nur um controls like blender oder 3ds max sondern das aussehen“) und die Abgleichtabelle
`abgleich_programm_entwurf.md`, Abschnitt 10 (W0a-W6, W8-W13; W7 Auto-Skin ist AP12, hier nur sein Platz). Worktree `Y:\p_ap8`, Zweig `p_ap8`.
Selbsttest `gui/selbsttest_ap8.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap8 | ap8foto`; Handbuch `BHED_EDITORTEST=handbuch`,
`BHED_HB_NUR=ap8`), Skripte und Läufe `Y:\werkzeuge\ap8\`.

## Was sich ändert

Das rechte Feld des Bereichs Weights (`drawModellGewichte`, neu `gui/app_modelltab_gewichte.cpp`) zeichnet über den Baustein von AP3. Vorher
(rc611): EINE ImGui-Gruppe „Weights“ mit Knochen-Klappliste, Pinselarten, Reglern, zwei Spiegel-Knöpfen ±X und der Prüfzeile. Das alte Feld
(`drawModellGewichteAlt` in `gui/app_modelltab_mb2.cpp`) ist weg; alles, was es konnte, steht im neuen Feld, in „Alle Aktionen suchen“ und im
Rechtsklick der Ansicht (Untermenü „Weight tools“ mit Gewichts-Werkzeug, Bereinigen, Spiegeln, Sperren).

| wie 3ds Max (Reiter Ändern) | wie Blender (Reiter Daten) |
|---|---|
| **Modifier-Stapel**: Skin (gewählt) über Edit Poly (Klick wechselt nach Modellieren) | **Pinsel**: Hauptknopf Malen, Regler Gewicht/Radius/Stärke, Mischung Hinzufügen/Abziehen/Setzen, Raster Weichzeichnen · Verlauf / Gewicht holen (Umschalt+X) · Verwischen, Tipp mit den Tasten |
| **Gewichts-Werkzeug**: Hauptknopf Gewichte malen (Umschalter: aus = linke Taste wählt Ecken), Festwerte 0 · .1 · .25 · .5 · .75 · .9 · 1, Regler Gewicht, Raster Setzen · Skalieren / Ring · Schleife / Wachsen · Schrumpfen / Mischen, eingeklappt „Pinsel-Optionen“ (Mischung, Verwischen, Verlauf, Radius, Stärke, Spiegeln beim Malen, Faktor Skalieren) | **Optionen**: Auto-Normalisieren, X-Spiegeln, „Höchstens 4 je Ecke (JKA)“ fest an |
| | **Bereinigen**: Alles normalisieren · Säubern / Glätten · Begrenzen: 4, eingeklappt Grenze, Faktor, Runden |
| **Spiegeln**: Spiegelmodus (links blau, rechts grün, ohne Gegenstück gelb), Blau → Grün · Grün → Blau / Seiten tauschen · Nur Auswahl, Toleranz, Tabelle Knochenpaare · Ecken ohne Gegenstück · flächenübergreifend, Tipp zur 32er-Grenze | **Spiegeln**: Gruppe spiegeln · Alle Gruppen, Gewichte spiegeln · Namen tauschen (L↔R) · Topologie-Spiegeln · X-Spiegeln beim Malen, Toleranz, dieselbe Tabelle |
| **Automatisch (Auto-Skin)**: der Inhalt von AP12 (`drawModellAutoSkinGruppe`) jetzt in der Form der Gruppen (Verfahren als Wahl, Regler, Schalter in zwei Spalten, Hauptknopf, Ergebnis als Tabelle); das Fenster „Auto-Skin“ bleibt über die Aktion | gleich |
| **Knochen**: Klappliste, Pose (Sequenz, Bild ab 0), Sperren, Knochen zeigen, Liste der Gesperrten mit x, Knopf Sperren… (Menü), Tipp | gleich, Bild ab 1, Taste K am Knopf |
| **JKA-Grenzen**: Balken Ecken je Fläche, Knochen je Fläche (gewählte Fläche, sonst die der Grenze nächste), Gewichte je Ecke; Flächen im Modell; Kurzinfo „n / 32 Knochen“; Warnzeilen; Übernehmen | gleich |
| **Erweiterte Parameter** (zu): Normalisieren, Höchstens 4, Bereinigen-Knöpfe mit Max-Namen, Grenze „Remove Zero Limit“ | – |

Neu am Pinsel und an den Befehlen (Bibliothek `include/bhed/modell/gewichte.h`, `src/modell/gewichte.cpp`, ctest `modell_gewichte2`):
- **Schweißnetz**: Ecken mit bitgleichem Ort sind EIN Punkt, über alle Flächen hinweg (in JKA sind Arme, Rumpf, Kopf eigene Flächen, an Grenzen
  und UV-Nähten liegen mehrere Ecken am selben Ort) - jeder Befehl und das Spiegeln schreiben je Punkt, damit das Netz in der Pose nicht reißt.
- **Auto-Normalisieren** (Blender „Auto Normalize“, 3ds Max „Normalize“), ab Werk an; **Sperren** je Knochen (Blender Schloss der Vertexgruppe,
  K-Menü mit den acht Einträgen von `VIEW3D_MT_wpaint_vgroup_lock_pie`). Ein gesperrter Knochen lässt sich nicht malen (Blender „Active group is
  locked, aborting“), Normalisieren arbeitet um ihn herum (`do_weight_paint_normalize_all_locked(_try_active)`).
- **Verlauf** (Blender Gradient, linear/radial) und **Verwischen** (Blender Smear) als Pinsel; **Gewichts-Werkzeug** wie 3ds Max: Festwerte und
  Setzen auf die Auswahl, Skalieren (×0,95), Mischen, Wachsen/Schrumpfen/Ring/Schleife über die Punkte (Ring/Schleife über Vierecke, ein
  Dreiecksnetz wird dafür wie „Zu Vierecken“ gepaart, ohne das Dokument zu ändern).
- **Bereinigen** wie Blender Menü Weights: Normalize All (Lock Active), Clean (Grenze, eine Zuordnung bleibt), Smooth (Faktor, Runden),
  Limit Total 4 - auf die gewählten Ecken, ohne Auswahl auf alle.
- **Spiegeln** wie 3ds Max (Seite nach Seite, Tauschen, nur Auswahl, Toleranz) und wie Blender (`vgroup_mirror`: Gruppe/alle, Gewichte, Namen
  tauschen, Topologie nach `ED_mesh_mirrtopo_init`); Paare der Knochen über die Namen (lhumerus ↔ rhumerus, l_/r_), danach die Prüfung der
  Knochen je Fläche des Zielspiels.
- **Ansicht**: im Bereich Weights sind die Knochen ab Werk zu sehen (der Schalter merkt sich seinen Stand je Bereich), Pinselkreis nur beim
  Malen, Spiegelmodus als Punkte mit dunklem Rand, Strecke des Verlaufs.
- **Nähte dick** (offener Punkt von AP7) gilt jetzt auch für die Naht-Striche der 3D-Ansicht im Bereich UV (`ueberlagerung`,
  `modellUvNahtDicke()`), geprüft in `selbsttest_ap7.inc` (U6).

Eigene Symbole (Block am Ende von `enum class UiIcon`, Zeichnung in `gui/uiicons.cpp`): GwPinsel, GwSetzen, GwSkalieren, GwRing, GwSchleife,
GwWachsen, GwSchrumpfen, GwMischen, GwVerlauf, GwVerwischen, GwTropfen, GwSumme, GwBesen, GwGrenze, GwNachRechts, GwNachLinks.
Texte in `tools/i18n_ap8.py` (6 Sprachen); zwei seit dem neuen Feld unbenutzte aus `tools/i18n_mb1a2.py` entfernt (`MbRGewichte`,
`MbGwGespiegelt`); lint_i18n ok. Baustein `gui/ui_rollout.*` unverändert (zwei Knöpfe setzen `KnopfOpt::tasteHalten`: Gewicht holen, Sperren…).

## Werkzeuge wie im Vorbild - was nachgelesen ist und was nicht dokumentiert ist

- Blender 5.0.0, Quelltext gelesen, Rechnung übertragen (GPL v2 oder später, als GPL v2; Zeile in `THIRD-PARTY.md`): `object_vgroup.cc`
  (Normalize All, Clean, Smooth, Limit Total, Mirror, Sperren), `deform.cc` (`BKE_defvert_normalize_ex`), `mesh_mirror.cc` (Topologie),
  `paint_weight.cc` (Normalisieren mit Sperren, Smear, Gradient), `paint_vertex_weight_ops.cc`, Belegung `blender_default.py` (`km_weight_paint`:
  K, Umschalt+X, Strg/Umschalt beim Strich).
- 3ds Max: Autodesk-Hilfe „Weight Tool Dialog (Skin Modifier)“ (Festwerte, Set, Scale Weight 0,95, Shrink/Grow/Ring/Loop, Blend),
  „Parameters Rollout (Skin Modifier)“ und „Mirror Parameters Rollout“ (Mirror Mode links blau/rechts grün, Paste Blue to Green / Green to Blue,
  Mirror Paste, Mirror Thresh), help.autodesk.com.
- **Nicht dokumentiert** (so umgesetzt, im Code vermerkt): die Formel von „Blend“ in 3ds Max (die Hilfe sagt nur „smooth out the transition“) -
  hier der Glätten-Schritt von Blender mit Faktor 0,5 und Normalisieren; wie „Scale Weight“ mit der Normalisierung zusammenspielt - hier
  skaliert der Knochen, die übrigen teilen sich den Rest; dass Ecken ohne Gegenstück im Spiegelmodus gelb sind, steht so nur im Entwurf.
- **Wie im Vorbild gemessen statt angenommen**: der Verlauf merkt sich die Bildorte der Ecken beim ersten Ereignis (Blender `vert_cache->elem[].sco`)
  - siehe Funde.

## JKA-Grenzen

- Höchstens 4 Gewichte je Ecke (fest an, nicht abwählbar), Knochen je Fläche nach `modell::grenzen(Zielspiel)` (32), Gewichte auf 10 Bit wie
  `G2_GetVertBoneWeight` (das letzte als Rest 1 - Summe). „Übernehmen“ schreibt die vier größten, normiert, in 10 Bit.
- „Flächen im Modell (Hinweis ab 32)“ aus dem Entwurf: steht als Zahl ohne Hinweis - die Engine hat keine Grenze 32 für Flächen (keine Quelle).

## Funde (rot nachgestellt, behoben, grün)

1. **Viertes Gewicht mit Rest 0** (Fund von AP12): `fuersSpiel` ließ einen letzten Knochen mit Rest 0 stehen; Schreiben/Lesen wich ab
   („…1:0.00293255132 | Datei …1:0.00293254852“). Jetzt fällt der letzte Knochen weg, solange sein Rest ≤ 0 ist. ctest `modell_gewichte2` rot → grün.
2. **Altes Spiegeln ließ Ecken aus**: eine zweite Ecke am selben Ort und die Ecke der Nachbarfläche blieben unbeschrieben - jetzt über das
   Schweißnetz und je Punkt sein Partner. ctest rot → grün.
3. **Verlauf mit Rückkopplung** (Lauf `g2_ap8`/`r3_ap8`): jede Mausbewegung rechnete mit den Bildorten der Ecken in der laufenden Pose - die hängen
   an den Gewichten, die der Verlauf gerade ändert. Die Ecke genau am Anfang bekam 0,71 bzw. 0,52 statt 0,8, 21 Ecken vor dem Anfang lagen unter
   dem vollen Wert. Jetzt wie Blender mit den Bildorten vom Beginn (`ModellGewichte::verlaufBild`): grün.

## Selbsttest (rot -> grün)

- Rot gegen das alte Feld (`ap8_rot1`): **25 OK, 127 FEHLER** (grün schon: W10 Ansicht, W13, W9 Übernehmen, die Aktionstabelle).
- Erster Lauf mit neuem Feld (`g1_ap8`): 85 OK, 28 FEHLER - Testfehler (Popups heißen im Register „##Popup_…“; Festwerte/Setzen/Glätten trugen
  eine `###`-Kennung im Register, jetzt ohne; gesperrte Elemente per `klickAn`; Bildort im selben Bild wie die Rücknahme berechnet; „Smear“ gibt es
  in Painting und Weights - der Klick nimmt den Eintrag von Weights) und zwei echte: die Taste Umschalt+X fiel am schmalen Blender-Knopf weg
  (jetzt `tasteHalten`) und der Verlauf (Fund 3). Keine Prüfung abgeschwächt.
- Grün (`k1_ap8`, nach Fund 3): **104 OK, 0 FEHLER**; nach dem Feinschliff (Tabelle schmal, Rand der Spiegelpunkte) war die Ecke am Anfang 0,69 Bildpunkte rechts des Zuganfangs (ImGui führt die Maus auf ganzen Bildpunkten) und trug richtig 0,79989 - der Zug beginnt jetzt auf dem ganzen Bildpunkt rechts der Ecke; letzter Lauf `z2_ap8`: **104 OK, 0 FEHLER**.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

- `gui/selbsttest_mb1a2.inc` (mb2) und `gui/selbsttest_mr.inc` (blender): rechnen die Pinsel-Arithmetik von rc611 nach - jetzt ausdrücklich
  ohne Auto-Normalisieren (ab Werk an, geprüft in ap8 W4). Mit Auto-Normalisieren bleibt eine Ecke mit nur einem Knochen bei 1 (Blender und 3ds Max
  ebenso), und in der Pose zieht rhand bemalte Kopf-Ecken aus dem Pinsel (gemessen: head_face Ecke 21 beim Strich mit Umschalt 54,8 Bildpunkte
  von der Mitte, Radius 40). Auf dem Bau von karte1 (exe von p_ap6) war blender 68/0.
- `gui/selbsttest_ap12.inc`: die drei Regler melden sich mit ihrer Beschriftung statt `##asaufloesung`/`##assteifigkeit`/`##asglaetten`.
- `gui/selbsttest_ap7.inc` U6: „Nähte dick“ prüft zusätzlich die Striche der 3D-Ansicht.

## Fotos

`Y:\werkzeuge\ap8\fotos\` (Selbsttest `ap8foto`, 3818 × 2104 bei Oberfläche 1,5; je Variante 0 FEHLER): en, de, zh-Hans, ja, es, cs, hell, klein
(1328 × 844 bei 1,0). Neben dem Entwurf: `vergleich\v_en_max.png`, `v_en_blender.png`. Angesehen:
- Max und Blender en: Gruppenfolge, Hauptknöpfe blau, Festwerte als Reihe, Raster mit Symbolen, Tabelle und Tipp wie im Entwurf; Blender mit
  runden Panels, Umschalt+X zweizeilig am Knopf, K am Knopf Sperren….
- de, zh-Hans, ja, es, cs: nichts abgeschnitten, lange Texte brechen um.
- hell: Hauptknopf und Wahl in der Betonungsfarbe des hellen Themas. Spiegelmodus: blaue Punkte standen blau auf der blauen Figur (Gewicht 0) -
  jetzt mit dunklem Rand.
- klein: die Spalte „flächenübergreifend“ brach die Flächennamen mitten im Wort - im schmalen Feld steht jetzt die Zahl der Paare, alle Paare
  im Tooltip der Zeile.

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap8.py` (6 Sprachen): Seite „Model-Tab: Weights“ (Feld, Malen, Gewichts-Werkzeug und Bereinigen,
  Spiegeln, Knochen/Sperren/Grenzen); Bilder `h_ap8_max`, `h_ap8_spiegeln`, `h_ap8_blender` in 6 Sprachen unter `handbuch/bilder/<sprache>/`
  (2538 x 1472, je Lauf 0 FEHLER; angesehen en Spiegelmodus und ja Blender). Nicht eingehängt: AP14 führt das Handbuch zusammen.

## Schlussrunde

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen |
| ctest | 257 / 257 (neu `modell_gewichte2`) |
| `ap8` | 104 / 0 (rot 25 / 127 gegen das alte Feld) |
| Nachbargruppen (Läufe `k1_*`, `k2_*`, `z1_blender`) | mb2 38 · mb3 22 · rig 27 · blender 68 · ap3 105 · ap6 94 · ap7 88 · ap9 97 · ap12 35 · funktionen 62 · tasten 44 - je 0 FEHLER |
| Fotos `ap8foto` | 8 Varianten, je 0 FEHLER |
| Handbuchbilder | ap8 in 6 Sprachen, je 0 FEHLER; `gen_handbuch.py --pruefen` ok (Hinweis "2 Einträge in uebersetzung/ nicht mehr gebraucht" wie auf karte1) |
| Lints (21 aus `lints.sh`, dazu efxflags, fremdprogramm, d3dbesitz, merkmalsbits) | ok bzw. wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm mit derselben Meldung wie karte1 |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen, karteneditor, menueeditor,
sauberer Neubau, Sprachläufe.

## Offen / Fragen an shank

- Auto-Normalisieren ab Werk AN (wie Blender und 3ds Max; rc611 malte ohne) - einverstanden?
- Spiegel-Toleranz ab Werk 0,5 (Entwurf, 3ds Max „Mirror Thresh“); auf kyle bleiben damit 701 Ecken ohne Gegenstück (Holster, Gürtel - die Figur
  ist nicht spiegelgleich), der Entwurf zeigt 3 - die Zahl ist gemessen, nicht geschönt.
- W1 Modifier-Stapel: leichter Stapel (zwei Zeilen) jetzt, der gemeinsame Stapel mit Auge kommt mit AP13 (Wunsch 5 an AP3).
- „Flächen im Modell (Hinweis ab 32)“ weggelassen (keine Quelle in der Engine).
- W0b: Ansicht-Kopf „Benutzer-Perspektive · Weight Paint“ und Werkzeugleiste links (T) gehören zu AP4 (Blender-Aufbau).
- Gleiche Namen in „Alle Aktionen suchen“ aus verschiedenen Bereichen (z. B. „Smear“ in Painting und Weights) stehen ohne Bereich da - Vorschlag
  für AP14: Bereich als Zusatz in der Suche.
- Baustein (AP3): `wertTabelle` bricht im schmalen Feld (1280 x 720) lange Wörter der Namensspalte mitten im Wort ("Vertices without counterp / art").
- Wünsche an den Baustein (`Y:\werkzeuge\ap8\WUNSCH_AP3.md`, 6 Punkte) unverändert offen.

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP8 nicht angefasst)

| Zeile | Stand |
|---|---|
| W0a Aufbau wie 3ds Max | gleich (Reiter Ändern, sechs Gruppen + Erweitert zu); vier Ansichten per Alt+W wie bisher |
| W0b Aufbau wie Blender | Feld gleich (Reiter Daten, sieben Gruppen); Werkzeugleiste T, Kopf „Weight Paint“ - AP4 |
| W1 Modifier-Stapel | gleich in der Form (Skin über Edit Poly); gemeinsamer Stapel mit Auge - AP13 |
| W2 Gewichts-Werkzeug | gleich (Hauptknopf, Festwerte, Setzen/Skalieren/Ring/Schleife/Wachsen/Mischen); bewusst mehr: Schrumpfen, Regler Gewicht, Pinsel-Optionen |
| W3 Pinsel (Blender) | gleich (Malen, Regler, Weichzeichnen/Verlauf/Gewicht holen/Verwischen); bewusst mehr: Mischung als Wahl |
| W4 Optionen + Bereinigen | gleich |
| W5 Spiegeln (Max) | gleich (Modus blau/grün/gelb, vier Knöpfe, Toleranz, Tabelle, Tipp) |
| W6 Spiegeln (Blender) | gleich; bewusst mehr: Toleranz (ohne findet eine Figur des Spiels kaum Gegenstücke, Blender sucht auf 0,00002) |
| W7 Auto-Skin | Platz im Feld, Form der Gruppen (AP12 bleibt Herr des Inhalts) |
| W8 Knochen | gleich (Gewählt, Pose, Tipp); bewusst mehr: Sperren, Knochen zeigen |
| W9 JKA-Grenzen | gleich (drei Balken, Flächen im Modell); bewusst ohne „Hinweis ab 32“ |
| W10 Ansicht | gleich (Knochen ab Werk sichtbar, Gewichtsfarben, Pinselkreis) |
| W11 Tasten Max | gleich |
| W12 Tasten Blender | gleich (Umschalt+X, K Sperren-Menü) |
| W13 kurze Zeitleiste | gleich (AP1; geprüft, dass sie steht und die Pose läuft) |
| W14 Texte | gleich (Gruppennamen beider Modi) |

## Changelog

## English

- Model tab, Weights area: the panel on the right follows the approved design. Like 3ds Max: modifier stack, weight tool, mirror, auto-skin, bone, JKA limits and closed advanced parameters. Like Blender: brush, options, clean up, mirror, auto-skin, bone, JKA limits.
- Weight tool like 3ds Max: Paint weights switches painting on and off; the fixed values 0 to 1 set the weight on the selected vertices at once; new Scale, Blend, Grow, Shrink, Ring and Loop.
- New brushes like Blender: Gradient (linear or radial) and Smear; Auto-normalize keeps the weights of a vertex at 1 (on from the start).
- Clean up like Blender: Normalize all, Clean, Smooth and Limit: 4, on the selected vertices or on all.
- Mirror like 3ds Max (mirror mode left blue, right green, without counterpart yellow; blue to green, green to blue, swap sides, selection only, tolerance) and like Blender (mirror group or all groups, flip names, topology mirror); vertices at the same place on different surfaces count as one.
- Bones can be locked (like Blender: K opens the lock menu); the bones are visible in this area from the start; the limits of the game stand as bars.
- UV area: Thick seams now also thickens the seams in the 3D view.

## Deutsch

- Model-Tab, Bereich Weights: das Feld rechts folgt dem freigegebenen Entwurf. Wie 3ds Max: Modifier-Stapel, Gewichts-Werkzeug, Spiegeln, Auto-Skin, Knochen, JKA-Grenzen und zugeklappt Erweiterte Parameter. Wie Blender: Pinsel, Optionen, Bereinigen, Spiegeln, Auto-Skin, Knochen, JKA-Grenzen.
- Gewichts-Werkzeug wie 3ds Max: Gewichte malen schaltet das Malen an und aus; die Festwerte 0 bis 1 setzen das Gewicht sofort auf die gewählten Ecken; neu Skalieren, Mischen, Wachsen, Schrumpfen, Ring und Schleife.
- Neue Pinsel wie Blender: Verlauf (linear oder radial) und Verwischen; Auto-Normalisieren hält die Gewichte einer Ecke bei 1 (ab Werk an).
- Bereinigen wie Blender: Alles normalisieren, Säubern, Glätten und Begrenzen: 4, auf die gewählten Ecken oder auf alle.
- Spiegeln wie 3ds Max (Spiegelmodus links blau, rechts grün, ohne Gegenstück gelb; Blau nach Grün, Grün nach Blau, Seiten tauschen, nur Auswahl, Toleranz) und wie Blender (Gruppe oder alle Gruppen spiegeln, Namen tauschen, Topologie-Spiegeln); Ecken am selben Ort auf verschiedenen Flächen zählen als eine.
- Knochen lassen sich sperren (wie Blender: K öffnet das Sperren-Menü); die Knochen sind in diesem Bereich ab Werk zu sehen; die Grenzen des Spiels stehen als Balken da.
- Bereich UV: Nähte dick macht jetzt auch die Nähte in der 3D-Ansicht dick.

## 中文

- 模型选项卡的 Weights 区域:右侧面板按已确认的设计排列。3ds Max 模式:修改器堆栈、权重工具、镜像、自动蒙皮、骨骼、JKA 限制,以及折叠的高级参数。Blender 模式:笔刷、选项、清理、镜像、自动蒙皮、骨骼、JKA 限制。
- 3ds Max 风格的权重工具:“绘制权重”用于开关绘制;固定值 0 到 1 立即把权重设置到所选顶点;新增缩放、混合、扩大、缩小、环形和循环。
- 新的 Blender 风格笔刷:渐变(线性或径向)和涂抹;自动规格化使每个顶点的权重之和保持为 1(默认开启)。
- Blender 风格的清理:全部规格化、清除、平滑和限制为 4,作用于所选顶点或全部顶点。
- 3ds Max 风格的镜像(镜像模式左蓝右绿、无对应点为黄色;蓝到绿、绿到蓝、交换两侧、仅所选、容差)以及 Blender 风格的镜像(镜像组或全部组、翻转名称、拓扑镜像);不同表面上同一位置的顶点视为一个。
- 骨骼可以锁定(Blender 模式:K 打开锁定菜单);在此区域中骨骼默认可见;游戏的限制以进度条显示。
- UV 区域:“加粗接缝”现在也会加粗 3D 视图中的接缝。

## 日本語

- モデルタブの Weights エリア: 右のパネルが承認済みのデザインに沿って並びます。3ds Max 風: モディファイアスタック、ウェイトツール、ミラー、オートスキン、ボーン、JKA の制限、閉じた詳細パラメータ。Blender 風: ブラシ、オプション、クリーンアップ、ミラー、オートスキン、ボーン、JKA の制限。
- 3ds Max 風のウェイトツール: 「ウェイトペイント」でペイントをオン/オフします。固定値 0〜1 は選択した頂点にすぐウェイトを設定します。新たにスケール、ブレンド、拡大、縮小、リング、ループ。
- Blender 風の新しいブラシ: グラデーション (線形または放射状) とスミア。自動正規化は頂点のウェイトの合計を 1 に保ちます (初期設定でオン)。
- Blender 風のクリーンアップ: すべて正規化、クリーン、スムーズ、制限: 4 を、選択した頂点またはすべてに適用。
- 3ds Max 風のミラー (ミラーモードは左が青、右が緑、対応のない頂点は黄色。青から緑、緑から青、左右を入れ替え、選択のみ、許容値) と Blender 風のミラー (グループまたは全グループをミラー、名前を反転、トポロジーミラー)。別々のサーフェスで同じ位置にある頂点は 1 つとして扱います。
- ボーンをロックできます (Blender 風: K でロックメニュー)。このエリアではボーンが最初から表示され、ゲームの制限がバーで示されます。
- UV エリア: 「シームを太く」が 3D ビューのシームにも効くようになりました。

## Español

- Pestaña Model, área Weights: el panel de la derecha sigue el diseño aprobado. Como 3ds Max: pila de modificadores, herramienta de pesos, reflejar, auto-skin, hueso, límites de JKA y, cerrados, los parámetros avanzados. Como Blender: pincel, opciones, limpiar, reflejar, auto-skin, hueso, límites de JKA.
- Herramienta de pesos como 3ds Max: Pintar pesos activa y desactiva la pintura; los valores fijos de 0 a 1 ponen el peso en los vértices seleccionados al instante; nuevos Escalar, Mezclar, Ampliar, Reducir, Anillo y Bucle.
- Pinceles nuevos como Blender: Degradado (lineal o radial) y Difuminar; Auto-normalizar mantiene en 1 la suma de pesos de un vértice (activado de serie).
- Limpiar como Blender: Normalizar todo, Limpiar, Suavizar y Limitar: 4, en los vértices seleccionados o en todos.
- Reflejar como 3ds Max (modo espejo izquierda azul, derecha verde, sin pareja amarillo; azul a verde, verde a azul, intercambiar lados, solo selección, tolerancia) y como Blender (reflejar grupo o todos los grupos, invertir nombres, reflejo por topología); los vértices en el mismo lugar de superficies distintas cuentan como uno.
- Los huesos se pueden bloquear (como Blender: K abre el menú de bloqueo); en esta área los huesos se ven desde el principio; los límites del juego aparecen como barras.
- Área UV: Costuras gruesas ahora también engrosa las costuras en la vista 3D.

## Čeština

- Karta Model, oblast Weights: panel vpravo odpovídá schválenému návrhu. Jako 3ds Max: zásobník modifikátorů, nástroj vah, zrcadlení, auto-skin, kost, meze JKA a zavřené pokročilé parametry. Jako Blender: štětec, možnosti, vyčištění, zrcadlení, auto-skin, kost, meze JKA.
- Nástroj vah jako 3ds Max: Malovat váhy zapíná a vypíná malování; pevné hodnoty 0 až 1 nastaví váhu na vybraných vrcholech okamžitě; nově Škálovat, Prolnout, Rozšířit, Zmenšit, Prstenec a Smyčka.
- Nové štětce jako Blender: Přechod (lineární nebo radiální) a Rozmazat; Automatická normalizace drží součet vah vrcholu na 1 (zapnuto od začátku).
- Vyčištění jako Blender: Normalizovat vše, Vyčistit, Vyhladit a Omezit: 4, na vybraných vrcholech nebo na všech.
- Zrcadlení jako 3ds Max (režim zrcadlení vlevo modrá, vpravo zelená, bez protějšku žlutá; modrá na zelenou, zelená na modrou, prohodit strany, jen výběr, tolerance) a jako Blender (zrcadlit skupinu nebo všechny skupiny, prohodit názvy, zrcadlení podle topologie); vrcholy na stejném místě na různých površích se počítají jako jeden.
- Kosti lze zamknout (jako Blender: K otevře nabídku zámků); kosti jsou v této oblasti vidět od začátku; meze hry se zobrazují jako pruhy.
- Oblast UV: Silné švy nyní zesílí i švy ve 3D pohledu.
