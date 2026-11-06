# Notizen Modell-Block Stufe 3b: Dopesheet, Kurveneditor, Animationswerkzeuge, Aufraeumen, Pfade ziehen

Auftrag `auftrag_mb3b.md`. Direkt in `Y:\karte` auf `karte1` ab 666c42ae8 (Stufe 3), Build `build-test`, Laeufe `laeufe/mb5_*`
(Oberflaeche `mb5_ui*`, Reduzieren `mb5_kr_<spiel>`, Handbuch `mb5_hb_<sprache>`, Bildzeit `mb5_zeit_*`, Schluss `mb5e_*`).
Laufskript `lauf.py` wie mb3. kFassung bleibt rc610. Kein Spiel, kein 3ds Max, kein Blender gestartet; Spieldaten nur gelesen.
AnimEasy: nur die Ideenliste (`animeasy_ideen.md`), kein Code, keine Texte, keine Symbole.

## Quellen

- **Blender** (blender/blender, main 792201f45edfa057f84fc92d4341683e62d5223b, GPL-2.0-or-later, per `gh api` gelesen):
  `blenkernel/intern/curve.cc` (calchandleNurb_intern, BKE_nurb_handle_smooth_fcurve mit bezier_handle_calc_smooth_fcurve,
  tridiagonal_solve_with_limits, bezier_eq_*, bezier_clamp, bezier_output_handle), `blenkernel/intern/fcurve.cc`
  (BKE_fcurve_handles_recalc_ex, BKE_fcurve_correct_bezpart, solve_cubic, findzero, berekeny, fcurve_eval_keyframes_interpolate),
  `blenlib/intern/math_solvers.cc` (BLI_tridiagonal_solve, _cyclic), `math_rotation_c.cc` (interp_qt_qtqt),
  `editors/animation/keyframes_general.cc` (clean_fcurve, breakdown/blend_to_neighbor/blend_to_default/push_pull/ease/
  smooth/time_offset _fcurve_segment, ED_ANIM_get_1d_gauss_kernel), `editors/space_graph/graph_slider_ops.cc` (Vorgaben und
  Bereiche der Regler: Ease Schaerfe 2, Glaetten Sigma 0,33 / Breite 6, Push/Pull 0..2), `editors/armature/pose_slide.cc` und
  `editors/animation/transformable.cc` (Posen-Schieber, Quaternion mit interp_qt_qtqt), `animrig/intern/fcurve.cc`
  (initialize_bezt: Henkel bei +-1 Frame, Vorgabe Bezier + HD_AUTO_ANIM), `blenloader/intern/versioning_userdef.cc`
  (auto_smoothing_new = FCURVE_SMOOTH_CONT_ACCEL seit 2.81 - die Vorgabe fuer neue Kurven).
- **3ds Max**: die Rechnung der Bezier-Controller ist nicht offen. Der Modus "wie 3ds Max" zeigt dieselben Kurven mit den
  Namen von Max (Track View - Dope Sheet / Curve Editor; Tangenten Auto, Glatt, Linear, Benutzerdefiniert, getrennt) - nicht als
  Max-Rechnung belegt. Tasten nur aus `belege/max_hotkeys_de.txt` (K = Keys einstellen).
- Tasten wie Blender nur aus `belege/blender_keys.txt` (I, X Dopesheet, G/S Graph Editor, Alt+S, Alt+D); was dort fehlt (T, V,
  Pos1, Strg+C/V im Editor), hat keine Taste, ist aber ueber Menue, Rechtsklick und Suche erreichbar.

## 1. Kurven (`bhed/modell/animkurven.h`, `src/modell/animkurven.cpp`)

- Je Schluessel der Schicht (Stufe 3) jetzt Interpolation (Konstant, Linear = slerp wie Stufe 3, Bezier) und je Kanal (q w x y z,
  t x y z) Henkelarten links/rechts und Henkel relativ zum Schluessel. Bezier je Kanal wie die F-Kurve von Blender, das
  Quaternion fuer die Pose normiert (BKE_pchan_to_mat4). Neue Schluessel der Oberflaeche: Bezier + Auto Clamped (beide Vorbilder).
- Henkel nach jeder Aenderung: BKE_fcurve_handles_recalc_ex mit Continuous Acceleration; die Enden waagerecht (konstante
  Extrapolation). Ein gezogener Auto-Henkel wird Ausgerichtet, ein Vektor-Henkel Frei (wie Blender).
- Quaternion-Vorzeichen: ein neuer Schluessel liegt in der Halbkugel des Nachbarn (q und -q drehen gleich, die Kanaele laufen
  sonst durch 0); "Quaternion-Filter" stellt das fuer alte Schichten her. Ein Euler-Filter entfaellt (keine Euler-Kanaele).
- **Bindebasis** (`RigSchicht::bindeBasis`): Knochen mit Schluesseln sind ganz durch sie beschrieben (A' = B M B^-1); die
  Ansicht rechnet das ueber worldMatricesLerp mit M_eff = B^-1 A^-1 B M (rigNachMalAuf), Backen direkt. "Reduzieren" legt so an.

## 2. Editor (`gui/app_modelltab_kurven.cpp`)

- Unter den Ansichten, ueber der Zeitleiste, nur solange offen (die Ansichten werden niedriger, die Kante zieht die Hoehe):
  wie 3ds Max "Track View - Dope Sheet" / "Track View - Curve Editor", wie Blender "Dope Sheet" / "Graph Editor".
- Dopesheet: Zusammenfassung, Sammelzeile je Glied (Rumpf/Kopf, Arme, Beine, weitere; aus der Rig-Erkennung), Zeile je Knochen;
  Rauten; Klick/Umschalt, Rahmen, Ziehen in ganzen Frames, Lineal setzt die Zeit, Rad zoomt, Mitte schiebt.
- Kurveneditor: Kanaele je Knochen mit Auge, Schluessel (Zeit und Wert) und Henkel ziehen, Werte-Zoom Strg+Rad.
- Aktionen (56) in 11 Gruppen: Menue Animation > Animationswerkzeuge, Rechtsklick im Editor und in der Ansicht, Suche X/F3 (Zeile
  "Gruppe: Name (Taste)"), Rollout "Kurven und Animationswerkzeuge", Tooltip an jeder Zeile.
- Werkzeuge mit Regler (Popup): Vorschau sofort ueber `Dokument::vorschau`, Uebernehmen = ein Rueckgaengig-Schritt.

## 3. Werkzeuge

- Schluessel-Werkzeuge je Kanal (Blender keyframes_general.cc): Breakdown, Zum Nachbarn, Zur Vorgabe, Verstaerken/Abschwaechen,
  Ease, Glaetten (Gauss). Posen-Schieber am Zeitcursor (pose_slide.cc): Breakdowner, Push, Relax, Blend, Blend to Rest.
- Timing: auf den Cursor / erster bzw. letzter auf den Cursor, Zwischenframes einfuegen/entfernen, Zeitversatz (time_offset,
  zyklisch), Zeiten teilen / vom zuletzt gewaehlten.
- Werte: Null, aktueller Wert, negieren (umgekehrte Drehung, -t), umkehren (Zeit), am Cursor spiegeln.
- Anim Offset: eine Aenderung an einem Schluessel (Griff ziehen, Pfad ziehen) geht mit q_k * (q_v^-1 q_n), t_k + dt auf die
  anderen Schluessel des Knochens (nur gewaehlte, wenn welche gewaehlt sind).
- Animation spiegeln: je Frame die gespiegelte Pose (Stufe 3 poseEinfuegen, D' = S D(Partner) S) fuer alle Knochen.
- Posen-Bibliothek im Modelldokument (benannt, Rueckgaengig), anwenden auch gespiegelt.
- Marker der animation.cfg: Schluessel am ersten/letzten Frame, Schleife schliessen (letzter = loopFrames), Schluessel waehlen.

## 4. Aufraeumen und Reduzieren

- Ueberfluessige Schluessel wie clean_fcurve (Schwelle 0,001; ein Schluessel faellt, wenn jeder Kanal ihn entfernen wuerde),
  statische Kanaele (ein Schluessel; ueber der GLA faellt auch die Einheit wie cleardefault), Quaternion-Filter.
- **Reduzieren** (`rigReduzieren`): Ziel sind die 14 Bytes, die `rigBacken` mit der Schicht schreiben wuerde. Je Knochen gierig:
  Schluessel an Anfang und Ende, dann an das Bild mit dem groessten Anteil an der Grenze; danach jeden inneren Schluessel probeweise
  entfernen. Linear und Bezier (Auto Clamped, Continuous Acceleration) werden beide versucht, das mit weniger Schluesseln bleibt.
  Grenze: die aus Stufe 3 (`kompressionsGrenze`, jetzt eine Funktion: |1 - |q|^2| |I - R|_F + (4*3 + 2*3*2 sqrt 2)/16383) fuer die
  Drehung; die Verschiebung rundet compressBone auf 1/64, Grenze eine Stufe. Schnelle Pruefung ueber die Drehmatrix vor der
  Kompression (|C - P| <= Quantisierung), im Zweifel compressBone; zum Schluss `reduziertPruefen` mit echtem Backen.

### Reduktion auf allen Sequenzen von _humanoid (Schluss `mb5e_kr_<spiel>`, Bericht `mb5_reduktion_<spiel>.txt`)

| Spiel | Sequenzen | Frames | Schluessel vorher (Knochen x Frames) | nachher | Anteil | groesster Anteil an der Grenze | ueber der Grenze | 1 Stufe Verschiebung | Zeit (parallel) |
|---|---|---|---|---|---|---|---|---|---|
| JKA | 1289 | 21376 | 1 189 744 | 691 233 | 58,1 % | 1,0000 (BOTH_PAIN15 lfemurX Bild 10468) | 0 | 0 | 218 s |
| JK2 | 883 | 17278 | 1 314 288 | 862 381 | 65,6 % | 1,0000 (BOTH_BUTTON1 rfemurYZ Bild 1354) | 0 | 0 | 284 s |
| Movie Duels | 2434 | 60861 | 3 281 283 | 1 394 080 | 42,5 % | 1,0000 (BOTH_VS_ATR_S ltalus Bild 19289) | 0 | 0 | 514 s |

- Die Grenze ist eng (die Quantisierung der GLA selbst, rund 0,04 Grad): wo eine Bewegung krumm ist, braucht fast jedes Frame
  einen Schluessel. Knochen linear / Bezier / statisch: JKA 2574 / 43 710 / 22 033, JK2 1672 / 44 891 / 17 013, MD 6205 / 71 358 /
  51 439. Keine Verschiebung weicht ab (auch nicht um eine Stufe). Vor der Korrektur der Bildnummern (relative Zeit) waren es
  691 232 / 862 358 / 1 394 065 Schluessel (`mb5_kr_*`), ebenfalls alle innerhalb der Grenze.
- ctest: eine Quelle aus linearen Schluesseln bei 0/20/40/59 (53 Knochen, 60 Frames) reduziert sich auf genau 4 Schluessel je
  Knochen (3180 -> 212), groesster Anteil 0,167; Gegenprobe: ohne einen noetigen Schluessel 38 Knochen-Frames ueber der Grenze.

## 5. Bewegungspfade ziehen

- Punkte des Becken- und Wurzelpfads (Rolle Becken, Wurzel, Bewegung) in der Ansicht ziehen: Verschiebung in der Bildebene durch
  den Punkt (je Achse aus der Projektion, Ortho und Perspektive), ein Schluessel am Frame (rigKnochenVerschieben), Strg+Z; Anim
  Offset geht mit. Haende, Fuesse, Saebelspitze: nur Anzeige bis IK (Stufe 4), Tooltip und Meldung sagen es.

## Rot und gruen

- **ctest `modell_kurven`** (neu, 67 Pruefzeilen): Henkel gegen Werte von Hand (Auto ohne Glaettung 10/2,5614 = 3,90411 je Seite,
  Auto Clamped am Extremum waagerecht und ohne Ueberschwingen, Gegenprobe Auto geneigt; Vektor ein Drittel; Ausgerichtet gegenueber
  mit eigener Laenge; Frei bleibt; Continuous Acceleration: x ein Drittel, zweite Ableitung am Schluessel links 0,0282 / rechts
  0,0271, Gegenprobe ohne Glaettung -0,0515 / 0,0443), findzero/berekeny/correct_bezpart, Konstant/Linear/Bezier, normiertes
  Quaternion; jedes Werkzeug gegen einen Wert von Hand (Ease 1 = 9,2195 aus der Sigmoid-Formel), Posen-Schieber (Breakdown 0,5 =
  45 Grad, Relax 1 = 36 Grad bei 4/10, Rest = Einheit), Timing, Werte, Anim Offset (+10 Grad auf alle), Spiegeln (636
  Knochen-Frames, groesster Abstand 0,000011), clean_fcurve, statische Kanaele, Quaternion-Filter, Reduzieren mit Gegenprobe,
  Bindebasis in der Ansicht = Rechnung des Backens (0,000042).
- **Henkel bei grossen Bildnummern**: `mb5_ui6` Vektor-Henkel 3,33398 statt 10/3 (BOTH_WALK1 liegt bei Frame 30293 der MD-GLA,
  float traegt dort 0,002). ctest rot: Frame 19010 -> 3,333984. Jetzt rechnen Henkel und Auswertung relativ zum ersten bzw. linken
  Schluessel (die Rechnung von Blender haengt nur von Abstaenden ab): ctest 3,333333, `mb5_ui7` 3,33333.
- **Editor ueber der Figur**: als eigenes Fenster lag er ueber dem unteren Teil der Ansicht - `mb5_ui3` traf ein Klick auf den
  Pfad der Hand den Editor statt den Pfad (FEHLER "Pfad der Hand"). Jetzt unter den Ansichten (die Ansichten werden niedriger,
  Kante zum Ziehen): `mb5_ui4` gruen.
- **Doppelte Namen in der Suche**: `mb5_ui2` "Smooth: Regler offen" FEHLER - die Suche nach "Glaetten" traf das Glaetten der
  Weights, wie 3ds Max hiessen Interpolation und Tangente beide "Linear". Jetzt steht die Gruppe davor ("Schluessel-Werkzeuge:
  Glaetten", "Tangenten: Linear"): `mb5_ui3` gruen.
- **Abgeschnittene Leiste**: im Foto `h_mb5_dopesheet` (erste Aufnahme) endete die Menueleiste des Editors bei "Nur au". Jetzt ein
  Menue "Animationswerkzeuge" und ein Menue "Ansicht" (Filter, Alles zeigen, Schliessen); neu aufgenommen und angesehen.
- **Ueberladen**: nach dem Reduzieren zeigte der Kurveneditor alle 371 Kurven auf einmal. "Nur ausgewaehlte" ist jetzt ab Werk an
  (wie Blender "Only Show Selected"; ohne Auswahl alle).
- **Nicht nachgestellt**: `mb5_ui2` stuerzte beim zweiten Klick in die Ansicht nach den Posen-Schiebern ab (lesend, "Figur:
  zeichnen"; damals Editor als eigenes Fenster; die PDB des Laufs war schon ueberschrieben). Mit derselben Folge und vier Klicks
  unten links in der Ansicht (`mb5_ui8`, Pruefzeile "Klicks in die Ansicht nach den Schiebern") kein Absturz - nicht geaendert.
- Testfehler unterwegs (nicht im Programm): Klick fuer den Fokus waehlte den Griff ab, Frames ausserhalb von BOTH_WALK1 (26 bzw.
  30 Frames), zwei Kurven mit Wert 0 uebereinander, Vektor-Henkel neben Auto-Punkten werden von Continuous Acceleration mitgeloest
  (Blender: bezier_output_handle_inner nimmt HD_VECT - die Erwartung "ein Drittel im Wert" war falsch, jetzt gegen die Rechnung),
  clean_fcurve behaelt einen Haltepunkt (gleich dem vorigen, anders als der naechste), drei Esc fuer das Menue.
- Selbsttest `mb5` (Gruppe in `modellblock`): `mb5_ui1` 6/11 (Absturz im Test: nullptr), `mb5_ui3` 134/29, `mb5_ui4` 146/17,
  `mb5_ui6` 151/2, `mb5_ui7` 152/1, `mb5_ui8` **155/0**.

## Bildzeit (allein, BHED_D3D_DEBUG=0, nacheinander)

- `modellleistung` mit der exe der Stufe 3 (666c42ae8, Kopie `mb4_zeit_nach`) gegen die neue, dieselben 23 Bloecke, zweimal:
  Summe der Mittel 18,69 / 18,35 ms (`mb5_zeit_vor` / `mb5_zeit_nach`) und 16,41 / 16,00 ms (`..._vor2` / `..._nach2`); Summe der p99
  38,99 / 47,39 und 36,34 / 33,62 ms - innerhalb der Streuung zwischen zwei Laeufen (einzelne Spitzen in einem Block), Bildabstand
  4,2-4,3 ms. Ohne offenen Editor ist neu nur die Bezier-Auswertung neuer Schluessel beim Posieren.
- Editoren (`mb5_zeit_kurven2`, `BHED_MODELL1_NUR=kurven`): Sturmtruppler, BOTH_RUN1 reduziert (908 Schluessel an allen 53 Knochen),
  Animation laeuft:
  - ohne Editor: Bild-CPU 0,63 / 1,34 ms (Mittel / p99)
  - Dopesheet offen (alle 53 Knochen): 0,66 / 1,28 ms, der Editor selbst 0,07 ms
  - Kurveneditor offen, 53 Knochen x 7 Kanaele (371 Kurven, 28 063 Punkte je Bild): 1,99 / 3,00 ms, der Editor 1,33 ms;
    Bildabstand 4,33 ms - fluessig. Die Kurven werden nur nach einer Aenderung neu abgetastet (4 Proben je Frame, Zwischenspeicher
    nach Dokumentstand), je Bild nur gezeichnet.

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele, `mb5e_bau.txt`): 0 Warnungen, 0 Fehler. Danach nur die Werteskala des
  Kurveneditors (Bau `mb5e_bau2.txt`: 0 Warnungen) - dafuer `mb5e2_mb5` 155/0 und die Handbuchbilder neu.
- ctest -j16 (`mb5e_ctest.txt`): 225 / 225 (neu `modell_kurven`); nach dem Handbuch-Bau `handbuch|hilfe|i18n` 4 / 4,
  `gen_handbuch.py --pruefen --pflicht=es --pflicht=cs` ok, `src/hilfe_orte.inc` unveraendert.
- Lints (`mb5e_lint.txt`): alle ok bis auf die Werte der Stufe 3 (portability 99, pruefcodes 11, efxflags, fremdprogramm;
  link/tidy brauchen Linux).
- Selbsttests parallel (23 zugleich, `mb5e_summe.txt`): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0, mb3 21/0,
  rig 26/0, **mb5 155/0**, rundlauf jka/jk2/md je 2/0, rigrundlauf jka/jk2/md je 4/0 (Rig ohne Aenderung backt weiter Byte fuer
  Byte), bild jka/jk2/md je 1/0, **kurvenrundlauf jka/jk2/md je 2/0**; modell 14/0, mission1 98/0, kuerzel 202/0, **Modus 1
  2635/0**. Dazu die Handbuchlaeufe `mb5_hb4_<sprache>` je 155/0.
- Handbuch: Seite `modell-kurven` in 6 Sprachen, Bilder h_mb5_dopesheet / kurven / werkzeuge / aufraeumen (2538 x 1472, angesehen:
  de, ja, cs, es). Beim Ansehen behoben: abgeschnittene Menueleiste des Editors, ueberladener Kurveneditor nach dem Reduzieren
  (jetzt "Nur ausgewaehlte"), Tooltip der Werkzeugleiste im Bild (Maus vorher in die Ansicht), die Zahlen der Werteskala lagen
  uebereinander (jetzt Schritte 1/2/5 x 10^n mit mindestens 1,5 Zeilen Abstand).
- Kein Spiel, kein 3ds Max, kein Blender gestartet; Spielordner nur gelesen. Blender-Quellen per `gh api` gelesen (Lesekopie im
  Scratchpad `bl/`).

## Abweichungen und Offen

- Tasten wie Blender: X, Alt+S und Alt+D oeffnen in Blender ein Menue (Delete / Smoothing / Blending); hier loeschen X direkt
  und Alt+S / Alt+D oeffnen den Regler Glaetten bzw. Zum Nachbarn mischen (die anderen Eintraege der Blender-Menues stehen im
  Menue Animationswerkzeuge). G/S arbeiten nur in der Zeit (ganze Frames, die GLA kennt keine Zwischenbilder).
- Schluessel einer Schicht sind je Knochen gemeinsam fuer alle sieben Kanaele (eine Raute = ein Zeitpunkt aller Kanaele); in
  Blender hat jeder Kanal eigene Schluessel. Die Kurven und Werkzeuge rechnen trotzdem je Kanal; "Ueberfluessige entfernen"
  nimmt einen Schluessel nur, wenn ihn jeder Kanal entfernen wuerde.
- Linear dreht mit slerp (wie Stufe 3 und der Linear-Rotation-Controller von 3ds Max), Blender mischt linear je Kanal und
  normiert (nlerp) - bei Bezier ist es genau Blender.
- 3ds Max: Namen und Aufbau wie die Spuransicht, die Bezier-Rechnung ist die von Blender (Max legt seine nicht offen).
- Pfade von Haenden, Fuessen und Saebelspitze ziehen: erst mit IK (Stufe 4).
- Die Posen-Bibliothek lebt im Modelldokument (wie die Schicht der Stufe 3 nicht auf der Platte gespeichert).
- Reduzieren nur ueber die ganze Sequenz und alle Knochen (nicht nur gewaehlte Knochen/Schluessel).

## Vorschlag Stufe 4 (IK/FK, Pfade an Gliedern)

- Zwei-Glied-IK fuer Arm und Bein mit den Pol-Zielen aus Stufe 3 (analytisch, Ebene aus Wurzel/Ziel/Pol), Fussrolle um Ferse/
  Ballen/Zehe; das Ergebnis als FK-Schluessel der drei Knochen in derselben Schicht (Dopesheet und Kurven zeigen es sofort).
- Wie 3ds Max: Hand/Fuss greifen = IK, ein Glied drehen = FK, der Wechsel schreibt Schluessel; wie Blender: IK/FK-Regler je Glied
  und Einrasten IK->FK / FK->IK (Rigify-Idee). Pruefung: hin und zurueck an jedem Frame unter der Kompressionsgrenze
  (`kompressionsGrenze`), rot ohne Einrasten.
- Pfade von Haenden, Fuessen und Saebelspitze ziehen (jetzt nur Anzeige): der gezogene Punkt wird das IK-Ziel am Frame, mit
  Anim Offset auch ueber eine Spanne; Pruefung wie beim Becken (Punkt unter der Maus, Bildebene).
- Dazu aus 3b offen: Reduzieren nur fuer gewaehlte Knochen/Spanne, eigene Schluessel je Kanal (wenn shank es braucht), Posen-
  Bibliothek als Datei.

## Changelog

### English
- Model tab, area Animation: dope sheet and curve editor below the views (like 3ds Max Track View - Dope Sheet / Curve Editor, like Blender Dope Sheet / Graph Editor): keys of the control rig as diamonds with summary rows per limb, box select, move and scale in whole frames, copy, paste, delete; one curve per channel with constant, linear and Bezier interpolation and the tangents Auto Clamped, Auto, Vector, Aligned and Free, computed like the F-curves of Blender.
- Animation tools in the menu Animation, the right-click and the search: breakdown, blend to neighbour or default, push/pull, ease and smooth on keys; breakdowner, push, relax and blend for the pose at the current frame; snap, insert and remove frames, time offset, share key times; zero, current value, negate, reverse, mirror at the current frame; anim offset; mirror a whole animation left/right; pose library; keys at the markers of animation.cfg and closing the loop.
- Clean up: reduce baked GLA frames to few editable keys - baked again every bone frame stays within the compression bound of the GLA; remove redundant keys and static channels, quaternion filter.
- Drag the motion path of the pelvis or root in the view to move the bone at that frame.

### Deutsch
- Model-Tab, Bereich Animation: Dopesheet und Kurveneditor unter den Ansichten (wie 3ds Max Spuransicht - Dope Sheet / Kurveneditor, wie Blender Dope Sheet / Graph-Editor): Schlüssel des Control Rigs als Rauten mit Sammelzeilen je Glied, Rahmenauswahl, Verschieben und Skalieren in ganzen Frames, Kopieren, Einfügen, Löschen; eine Kurve je Kanal mit konstanter, linearer und Bezier-Interpolation und den Tangenten automatisch geklemmt, automatisch, Vektor, ausgerichtet und frei, gerechnet wie die F-Kurven von Blender.
- Animationswerkzeuge im Menü Animation, im Rechtsklick und in der Suche: Breakdown, zum Nachbarn oder zur Vorgabe mischen, verstärken/abschwächen, Ease und Glätten auf Schlüsseln; Breakdowner, Push, Relax und Blend für die Pose am aktuellen Frame; einrasten, Frames einfügen und entfernen, Zeitversatz, Schlüsselzeiten teilen; Null, aktueller Wert, negieren, umkehren, am aktuellen Frame spiegeln; Anim-Offset; eine ganze Animation links/rechts spiegeln; Posen-Bibliothek; Schlüssel an den Markern der animation.cfg und Schleife schließen.
- Aufräumen: gebackene GLA-Frames zu wenigen bearbeitbaren Schlüsseln reduzieren - wieder gebacken bleibt jedes Knochen-Frame innerhalb der Kompressionsgrenze der GLA; überflüssige Schlüssel und statische Kanäle entfernen, Quaternion-Filter.
- Den Bewegungspfad des Beckens oder der Wurzel in der Ansicht ziehen, um den Knochen an diesem Frame zu verschieben.

### 中文
- 模型选项卡的动画区域: 视图下方的摄影表和曲线编辑器 (像 3ds Max 的轨迹视图 - 摄影表 / 曲线编辑器, 像 Blender 的摄影表 / 曲线编辑器): 控制绑定的关键帧显示为菱形, 每个肢体一行汇总, 框选、按整帧移动和缩放、复制、粘贴、删除; 每个通道一条曲线, 支持常量、线性和贝塞尔插值以及自动钳制、自动、矢量、对齐和自由切线, 计算方式与 Blender 的 F 曲线相同。
- 动画菜单、右键和搜索中的动画工具: 关键帧的细分、混合到相邻或默认值、推/拉、缓动和平滑; 当前帧姿势的细分器、推动、放松和混合; 吸附、插入和删除帧、时间偏移、共享关键帧时间; 归零、当前值、取反、反向、以当前帧镜像; 动画偏移; 左右镜像整个动画; 姿势库; animation.cfg 标记处的关键帧和闭合循环。
- 清理: 将烘焙的 GLA 帧精简为少量可编辑的关键帧——再次烘焙后每个骨骼帧都在 GLA 的压缩界限内; 删除多余关键帧和静态通道, 四元数过滤。
- 在视图中拖动骨盆或根部的运动路径, 在该帧移动骨骼。

### 日本語
- モデルタブのアニメーション領域: ビューの下にドープシートとカーブエディター (3ds Max 風はトラック ビュー - ドープ シート / カーブ エディタ、Blender 風はドープシート / グラフエディター): コントロールリグのキーをひし形で表示し肢ごとの集約行、枠選択、整数フレームでの移動とスケール、コピー、貼り付け、削除。チャンネルごとのカーブで一定・リニア・ベジェ補間と自動クランプ、自動、ベクトル、整列、フリーの接線を Blender の F カーブと同じに計算します。
- アニメーションメニュー、右クリック、検索のアニメーションツール: キーのブレークダウン、隣や既定値へのブレンド、プッシュ/プル、イーズ、スムーズ。現在フレームのポーズのブレークダウナー、プッシュ、リラックス、ブレンド。スナップ、フレームの挿入と削除、タイムオフセット、キー時間の共有。ゼロ、現在の値、反転、逆再生、現在フレームでミラー。アニメオフセット。アニメーション全体の左右ミラー。ポーズライブラリ。animation.cfg のマーカーでのキーとループを閉じる。
- クリーンアップ: ベイク済み GLA フレームを編集できる少数のキーに削減 - 再ベイクしても各ボーンフレームは GLA の圧縮の上限内。不要なキーと静的チャンネルの削除、クォータニオンフィルター。
- ビューで骨盤またはルートのモーションパスをドラッグし、そのフレームのボーンを動かします。

### Español
- Pestaña Model, área Animación: hoja de claves y editor de curvas bajo las vistas (como en 3ds Max Vista de pistas - Hoja de claves / Editor de curvas, como en Blender Hoja de claves / Editor de gráficas): claves del rig de control como rombos con filas de resumen por miembro, selección con marco, mover y escalar en fotogramas enteros, copiar, pegar, borrar; una curva por canal con interpolación constante, lineal y Bézier y las tangentes automático limitado, automático, vector, alineado y libre, calculadas como las curvas F de Blender.
- Herramientas de animación en el menú Animación, el clic derecho y la búsqueda: intermedio, mezclar con la vecina o con el valor por defecto, empujar/tirar, suavizado de entrada/salida y suavizar sobre claves; pose intermedia, exagerar, relajar y mezclar para la pose del fotograma actual; ajustar, insertar y quitar fotogramas, desfase de tiempo, compartir tiempos de clave; cero, valor actual, negar, invertir, reflejar en el fotograma actual; desfase de animación; reflejar una animación entera izquierda/derecha; biblioteca de poses; claves en los marcadores de animation.cfg y cerrar el bucle.
- Limpiar: reducir los fotogramas GLA horneados a pocas claves editables; horneado de nuevo, cada fotograma de hueso queda dentro del límite de compresión del GLA; quitar claves redundantes y canales estáticos, filtro de cuaterniones.
- Arrastra la trayectoria de la pelvis o la raíz en la vista para mover el hueso en ese fotograma.

### Čeština
- Karta Model, oblast Animace: dopesheet a editor křivek pod pohledy (jako v 3ds Max Track View - Dope Sheet / Curve Editor, jako v Blenderu Dope Sheet / Editor grafů): klíče ovládacího rigu jako kosočtverce se souhrnnými řádky po končetinách, výběr rámečkem, posun a škálování o celé snímky, kopírování, vkládání, mazání; jedna křivka na kanál s konstantní, lineární a Bézierovou interpolací a tečnami automatická omezená, automatická, vektor, zarovnaná a volná, počítané jako F-křivky Blenderu.
- Nástroje animace v nabídce Animace, v pravém tlačítku a ve vyhledávání: mezifáze, smíchat se sousedem nebo s výchozím, zesílit/zeslabit, náběh/doběh a vyhlazení na klíčích; mezipóza, zesílit, uvolnit a smíchat pro pózu na aktuálním snímku; přichytit, vložit a odebrat snímky, časový posun, sdílet časy klíčů; nula, aktuální hodnota, negovat, obrátit, zrcadlit podle aktuálního snímku; posun animace; zrcadlit celou animaci vlevo/vpravo; knihovna póz; klíče na značkách animation.cfg a uzavření smyčky.
- Úklid: redukovat upečené snímky GLA na několik upravitelných klíčů - po novém zapečení zůstane každý snímek kosti v mezích komprese GLA; odebrat nadbytečné klíče a statické kanály, filtr kvaternionů.
- Táhněte dráhu pohybu pánve nebo kořene v pohledu a posuňte tak kost na tom snímku.
