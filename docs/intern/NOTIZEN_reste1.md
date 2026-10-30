# NOTIZEN reste1 - Fehlerliste vor rc607

Zweig `reste1` (Worktree `Y:\sprache_cs`, ab pruefung1 32dfcdd8), Build `Y:/sprache_cs/build-sprache`.
Laeufe in `Y:\r1_laeufe\<name>` (Idle-Prioritaet, `BHED_HANDBUCH=Y:\sprache_cs\handbuch` fuer Modus 1/reiter).
Kuenstliche Last: `SP\r1_last.py` startet waehrend eines Abschnitts eigene Python-Arbeiter (normale
Prioritaet, Takt 50-100 ms, 90-97 % rechnend), beendet sie per taskkill /T (nachgeprueft: keine leben
weiter); jeder Arbeiter endet ausserdem von selbst nach der Zeitgrenze.

## Stand

- Fertig (je rot/gruen, committet): 1 Decals im Karteneditor, 2 Tutorial-Klick unter Last, 3 Port der
  Bauueberwachung, 5 Map-Tab ohne Karte ruht. Schlussrunde: sauberer Neubau 0 Warnungen, ctest 160/160,
  Modus 1, reiter, karteneditor reste1 und stufe3/bau, mission1 (siehe Laeufe).
- Offen: 4 (Stufe 8 "Notify when a build is done"), 6 (g_figurBilder); karteneditor fotos und karte
  nicht gelaufen.
- Naechster Schritt: 4 mit `BHED_KARTE_NUR=stufe8` nachstellen, dann 6 (siehe Offen).

## 1. Decals im Karteneditor (fertig)

Ursache: seit Pruefung 2 ist ein Decal eine Marke (CG_ImpactMark -> R_MarkFragments gegen die
Flaechen der Welt). Die Echtzeit-Effekte des Editors (`gui/app_karte_echtfx.inc`) reichten keine Welt
herein (`geo = nullptr`), also fiel jedes Decal weg.

Behoben: `markenWeltAusKarte` (src/karte/spielkollision.cpp) baut die Welt fuer R_MarkFragments aus
den Brushes und Patches so, wie q3map2 sie als Zeichenflaechen in die Welt der .bsp schreibt: nur
worldspawn und func_group (MoveBrushesToWorld; andere Brush-Entities sind eigene Modelle, und
R_MarkFragments geht nur tr.world ab), je Seite eine ebene Flaeche ausser C_NODRAW ohne C_FOG
(ClipSidesIntoTree), keine Ursprungs-Brushes, Patches ausser SURF_NODRAW (ParseMesh: SF_SKIP), die
Ebene eingerastet wie FindFloatPlane. `spurWeltAusFlaechen` (src/saberspuren.cpp) legt einen
achsparallelen Baum darueber (Median der Flaechenmitten, <= 8 Flaechen je Blatt, beruehrende Flaechen
in beiden Kindern), den R_BoxSurfaces_r unveraendert abgeht. `buildEffectMesh` bekommt die Welt als
neuen letzten Parameter `marken`; der Editor baut sie beim ersten Decal nach einer Geometrieaenderung.

- Test `karte_marken` (tests/kartenmarken.cpp, gegen das Vorbild): dieselben Quadrate wie
  CG_ImpactMark auf der Editor-Welt und auf der mit bhed_q3map2 gebauten `spielraum.bsp` - Boden (vier
  Lagen, eine schraeg), Wand (zwei), Stufen, Rampe, Patch (Scheitel, Flanke), nonsolid: gleiche Flaeche
  (< 1 %) und gleicher Kasten (< 0,05); Tuer (func_door), clip, monsterclip: keine Marke, wie in der
  .bsp. 18/18.
- Selbsttest `BHED_KARTE_NUR=reste1` (gui/selbsttest_reste1.inc, auch im vollen karteneditor):
  fx_runner `blaster/wall_impact` 8 ueber dem Boden, Kamera 40 darueber, Punkt-Entitaeten
  ausgeblendet. rot (alter app_karte_echtfx.inc): Mitte vorher = nachher 53/50/46, Ecken 0. gruen:
  Mitte 0/0/0 (die Brandflecken der Wiederholungen liegen uebereinander), Ecken 300.
- Abweichung (bewusst, im Test benannt): der Editor schneidet die ganzen Seiten, q3map2 nur den
  sichtbaren Teil (visibleHull). Unter der Stufe 16 kommt darum ein Stueck Boden im Inneren der Stufe
  dazu - nicht zu sehen. Nicht nachgebaut: misc_model, die q3map2 in die Welt backt (ihre Dreiecke
  tragen im Spiel Marken).

## 2. Tutorial-Klick in Modus 1 unter Last

Befund aus den Integrationslaeufen (m4modus1, i3e1cs, s5bmodus1): bei k-werte-bearbeiten mit offenem
Ereigniseditor geht genau EIN Klick auf "Weiter" verloren (danach je Pruefung einen Schritt hinterher,
am Ende Schritt 6/7), ohne "Klick kam nicht an" - das Druecken traf den Knopf, das Loslassen loeste
nichts aus. Ein Knopf (PressedOnClickRelease) loest nur aus, wenn er beim Loslassen noch unter der
Maus liegt. Das Tutorial-Fenster wird nach jedem Schrittwechsel neu gestellt (fensterNeuSetzen), waechst
ein Bild spaeter (AlwaysAutoResize) und wird, wenn es dann uebersteht oder ein neues Ziel verdeckt,
noch einmal gestellt - ein bis drei Bilder nach dem Wechsel. Der Selbsttest wartete fest 0,25 s; in
langsamen Bildern faellt der naechste Klick genau in diese Zeit.

Nachgestellt (Modus reiter, Abschnitt Tour-Pruefung unter kuenstlicher Last): zweimal ohne Verlust
(32 Arbeiter zu 90 %: 28 s; 48 zu 97 %: gut 4 min fuer den Abschnitt) - der genaue Verlust bei
k-werte-bearbeiten kam in reiter nicht. Mit dem neuen Melder in klick() kam derselbe Mechanismus an
anderer Stelle (Lauf r2b, 48 Arbeiter zu 97 %): "All tutorials" nach dem Start von k-zeilen-ordnen -
"Klick kam nicht an" und "beim Loslassen nicht mehr unter der Maus (2184/1381, Element jetzt
2136/1335-2233/1367)": Schritt 1 war erledigt, 0,6 s spaeter (Wanduhr, Ablauf::bild) schaltete die Tour
von selbst weiter, und das Fenster aenderte sich mitten im Klick. Unter Last faellt diese Frist in
weniger Bilder, also leichter in einen Klick.

Behoben:
- Programm (gui/tour.cpp): das Fenster wird nicht verschoben (fensterStellen) und schaltet nicht von
  selbst weiter (Ablauf::bild), solange die Maus auf einem seiner Knoepfe gedrueckt ist - beides
  geschieht im Bild nach dem Loslassen. Auch ein Mensch verlor in langsamen Bildern so einen Klick.
- Selbsttest (gui/selbsttest_programm.inc): vor jedem der 21 Klicks ins Tutorial-Fenster `tourRuhig()`
  (das Fenster drei Bilder unveraendert und kein erledigter Schritt, der gleich weiterschaltet), nach
  "Weiter" `tourBis(Schritt)` statt fester Pause, vor dem Doppelklick auf die set-Zeile `ruhigBis`.
- klick() (gui/selbsttest.cpp) meldet jetzt "beim Loslassen nicht mehr unter der Maus", wenn ein Knopf
  zwischen Druecken und Loslassen wanderte - bisher ging so ein Klick still verloren.

Nachweis unter Last: siehe Abschnitt "Laeufe".

## 3. Port 39000 der Bauueberwachung

Ursache: der Editor lauschte fest auf 127.0.0.1:39000 (NRC); ein zweiter Bau (zweites JK Studio)
bekam den Port nicht und brach mit "Failed to get a listening socket" ab.

Behoben (gui/app_karte_bau.cpp): der erste freie Port aus 39000-39063; ist es nicht 39000, wird die
Adresse in der Befehlszeile dieses Schritts ersetzt (q3map2 `-connect 127.0.0.1:<port>`) und in der
Konsole gemeldet ("Port 39000 is in use (another build), monitoring on 127.0.0.1:39001"). Sind alle
belegt, NRCs Meldung mit dem Bereich. Selbsttest Stufe 3/4 (`BHED_KARTE_NUR=stufe3
BHED_STUFE3_NUR=bau`): 39000 belegt -> Bau mit 39001, verbunden, fertig; alle 64 belegt -> Meldung,
kein Bau.

## 5. Map-Tab ohne geladene Karte ruht

Ursache: drawMapView kehrt ohne Karte frueh zurueck (Startflaeche) - mapDirty wird aber erst beim
Zeichnen des Kartenbilds weiter unten geloescht und blieb darum stehen (ab Start true); darfRuhen lehnte
immer ab, die Oberflaeche lief ohne Eingabe mit voller Bildrate. (modell1 vermutete `gpuBereit` ohne
Netz - die Stelle wird ohne Karte gar nicht erreicht; ein erster Versuch dort blieb rot und ist wieder
heraus.) Behoben (gui/app_view3d_bedienung.inc): im Zweig ohne Karte `mapDirty = false`; das Laden
einer Karte setzt es selbst. Selbsttest `reste1::ruheOhneKarte` (Modus reiter und 1): Map-Tab ohne
Karte, Ruhe erlaubt, hoechstens 8 Bilder/s.
- rot (Lauf r2b, alt): mapDirty 1, 36 Bilder in 2,17 s; (r2e, erster Versuch): mapDirty 1, 133 in 2,07 s.

## Handbuch

Keine Bedienung geaendert, keine neuen Programmtexte (die neue Konsolenzeile der Bauueberwachung ist
englisch wie NRCs uebrige Zeilen dort). Die Seiten erwaehnten weder fehlende Decals noch den Port.

## Gegen das Vorbild geprueft / nicht geprueft

- Geprueft: Marken auf der Editor-Welt gegen dieselben auf der von bhed_q3map2 gebauten .bsp
  (karte_marken); der Bau mit belegtem Port gegen einen echten bhed_q3map2-Lauf (verbunden, fertig).
- Nicht gegen das Spiel selbst (kein Spielstart): die Marken folgen dem Port von R_MarkFragments, der
  seit Pruefung 2 gegen die Engine-Quellen steht.

## Offen

- 4. Stufe 8 allein: "Notify when a build is done" - nicht bearbeitet (Zeit). Befund aus den Logs: das
  Element ragt waagerecht 1-2 px ueber den sichtbaren Teil des Menues (329..858 gegen 330..856), und
  der Klick fand unter der Maus kein Fenster ("aktiv: -, unter der Maus: -, Fenster -") - das Build-Menue
  war beim Druecken schon zu. Naechster Schritt: `BHED_KARTE_NUR=stufe8` nachstellen, qtPunkt
  (Pruefbar) mit dem Untermenue "Im Spiel testen" davor ansehen.
- 6. g_figurBilder - nicht bearbeitet (Zeit). Naechster Schritt: beim Austausch der Figurbilder
  (neue Haut) die alten Adressen aus g_figurBilder/g_figurBildDaten freigeben und den Inhaltseintrag
  nur, wenn ihn keine Adresse mehr nutzt (Generation erhoehen); Test: Haut mehrfach wechseln,
  speicherStand().figurBilder bleibt gleich, Bild Bildpunkt fuer Bildpunkt gleich.
- Nebenbefund: der vorhandene Schritt "2D-Ansicht XY auf die Leck-Linie" (Stufe 3/5) stuerzt ab
  (Lesezugriff 0x4), wenn der Bau keine Leck-Linie liefert (hier: bhed_q3map2.exe fehlte im
  Laufordner) - der Test sollte die leere Linie abfangen.
- Nicht gelaufen (Zeit): karteneditor kurz/fotos, karte; modellleistung gibt es in diesem Zweig nicht
  (modell1 ist nicht enthalten).

## Laeufe

Idle-Prioritaet, Laufordner `Y:\r1_laeufe\<name>`; "unter Last" = mit `r1_last.py` im Abschnitt
Tour-Pruefung (dazu die anderen Agenten).
- r1rot / r1rot2 (karteneditor reste1, alter echtfx): 12 OK / 1 FEHLER (Decal); r1rot zeigte noch den
  Kasten des fx_runner (blau) - darum blendet der Test Punkt-Entitaeten aus. r1gruen: 13 / 0.
- r2rot (reiter, Melder ohne Behebung, 32 Arbeiter 90 %): 90 / 0; r2rot2 (48 Arbeiter 97 %): 90 / 0 -
  der Verlust bei k-werte-bearbeiten kam nicht.
- r2b (reiter, Fenster-Sperre und Warten, ohne Sperre beim Weiterschalten, ohne Punkt 5, 48/97 %):
  89 / 4 - "All tutorials" verloren (Mechanismus oben), Punkt 5 rot.
- r2e (reiter, alle Behebungen zu Punkt 2, erster Versuch Punkt 5, 48/97 % ueber beide
  Tour-Abschnitte): 90 / 1 - alle Tutorial-Klicks angekommen; Punkt 5 noch rot (falsche Stelle).
- r3rot (stufe3 bau, alter Port-Code): Port-Test rot ("Setting up", NRC-Abbruch); der Lauf hatte
  bhed_q3map2.exe nicht im Laufordner (Skriptfehler, danach behoben) und stuerzte im vorhandenen
  Schritt "2D-Ansicht XY auf die Leck-Linie" ab (Lesezugriff 0x4, keine Leck-Linie ohne q3map2) -
  ein Fehler des Tests bei fehlendem Bau, nicht dieser Liste; offen.
- r3gruen (stufe3 bau): 113 / 1 - Port-Tests gruen ("Port 39000 is in use (another build), monitoring
  on 127.0.0.1:39001" - 39000 hielt gerade ein anderer Bau); der eine Fehler: ".bsp wie NRCs" braucht
  BHED_Q3MAP2_DATEN, wenn der Laufordner ausserhalb des Worktrees liegt.
- m1e (Modus 1, Stand r2e, 48 Arbeiter 97 % ueber Tour-Pruefung bis "All tutorials", dazu der saubere
  Neubau parallel): 2648 OK / 1 FEHLER - alle Tutorial-Klicks angekommen, der Melder in klick() schlug
  nirgends an; der eine Fehler ist Punkt 5 (erster Versuch, vor der Behebung an der richtigen Stelle).
- r5gruen (reiter, Endstand, 48 Arbeiter 97 % ueber beide Tour-Abschnitte): 91 OK / 0 FEHLER;
  Punkt 5: mapDirty 0, 13 Bilder in 3,4 s.
- Sauberer Neubau (`--clean-first`, 21 min unter Last): 0 Warnungen, 0 Fehler.
- ctest: 160/160 (darin neu karte_marken).
- lint_sources, lint_fassung, lint_headers: ok.

## English
- Map editor real-time effects: decals of effects (blaster marks, scorch marks) appear again. They are clipped against the brushes and patches of the map the way the game clips them against the compiled map (only world brushes and func_group, no nodraw sides; doors and other brush entities get no marks).
- Two map builds at the same time (also from two JK Studio windows): the second build no longer stops with "Failed to get a listening socket on port 39000" - the build monitor takes the next free port (39001-39063) and hands it to q3map2.
- Tutorials: the tutorial window is no longer moved while a mouse button is held on one of its buttons, so a quick second click on "Next" is not lost when the program is slow.
- The Map tab without a loaded map rests when nothing happens (before it kept drawing at the full frame rate).

## Deutsch
- Karteneditor, Echtzeit-Effekte: Decals der Effekte (Einschlaege, Brandflecken) erscheinen wieder. Sie werden gegen die Brushes und Patches der Karte geschnitten, wie das Spiel sie gegen die gebaute Karte schneidet (nur Welt-Brushes und func_group, keine nodraw-Seiten; Tueren und andere Brush-Entities bekommen keine Marken).
- Zwei Kartenbauten gleichzeitig (auch aus zwei JK-Studio-Fenstern): der zweite bricht nicht mehr mit "Failed to get a listening socket on port 39000" ab - die Bauueberwachung nimmt den naechsten freien Port (39001-39063) und gibt ihn q3map2 mit.
- Tutorials: das Tutorial-Fenster wird nicht mehr verschoben, solange eine Maustaste auf einem seiner Knoepfe gedrueckt ist - ein schneller zweiter Klick auf "Weiter" geht auch bei langsamem Programm nicht mehr verloren.
- Der Map-Tab ohne geladene Karte ruht, wenn nichts geschieht (vorher zeichnete er mit voller Bildrate weiter).

## 中文
- 地图编辑器实时特效：特效贴花（弹痕、烧焦痕迹）重新显示。它们像游戏针对编译后的地图那样，针对地图的笔刷和面片进行裁剪（仅世界笔刷和 func_group，不含 nodraw 面；门和其他笔刷实体不会留下痕迹）。
- 同时进行两次地图编译（包括从两个 JK Studio 窗口）：第二次编译不再因 "Failed to get a listening socket on port 39000" 而中止——编译监视使用下一个空闲端口（39001-39063）并将其传给 q3map2。
- 教程：在教程窗口的按钮上按住鼠标键时，窗口不再移动，因此程序较慢时快速第二次点击"下一步"也不会丢失。
- 未加载地图时，地图标签页在无操作时进入休眠（之前一直以全帧率绘制）。

## 日本語
- マップエディターのリアルタイムエフェクト：エフェクトのデカール（着弾痕、焦げ跡）が再び表示されます。ゲームがコンパイル済みマップに対して行うのと同じように、マップのブラシとパッチに対してクリップされます（ワールドブラシと func_group のみ、nodraw 面は除く。ドアなどのブラシエンティティには跡が付きません）。
- 2 つのマップビルドを同時に実行（2 つの JK Studio ウィンドウからでも）：2 つ目のビルドが "Failed to get a listening socket on port 39000" で止まらなくなりました。ビルド監視は次の空きポート（39001-39063）を使い、q3map2 に渡します。
- チュートリアル：ボタン上でマウスボタンを押している間はチュートリアルウィンドウが移動しなくなり、プログラムが遅いときでも「次へ」の素早い 2 回目のクリックが失われません。
- マップを読み込んでいないマップタブは、何も起きていないときに休止します（以前はフルフレームレートで描画し続けていました）。
