# NOTIZEN reste2 - Fehlerliste Karte/Modell vor rc607

Zweig `reste2` (Worktree `Y:\hilfe`, ab karte1 0a544a42 mit reste1 und modell1), Build
`Y:/hilfe/build-hilfe`. Laeufe in `Y:\rs2\<name>` (ausserhalb des Worktrees, Idle-Prioritaet,
Skript `SP\rs2_lauf.sh`).

## Stand

- Fertig: 1 (Stufe 8 "Notify when a build is done").
- In Arbeit: 2-8.

## 1. Stufe 8 allein: "Notify when a build is done"

Nachgestellt: der Befund aus i3ns8/i3ns8alt (29.10.) - "nicht ganz sichtbar (329..858, sichtbar
330..856)" und "Klick kam nicht an (aktiv: -, unter der Maus: -, Fenster -)" - kam in diesem Stand
nicht von selbst (s8rot: 37 OK / 0). Ein Messschritt (nicht eingecheckt) zeigte die Lage: das
Build-Menue hat 34 Eintraege und ist damit fast so hoch wie der Bildschirm (2560 x 1600, Arbeitsflaeche
1528 hoch).
- Stil des Programms (Vorgabe): ImGui begrenzt das Menue auf den Bildschirm und rollt es. Mit 20
  Eintraegen mehr (s8diagp20) genau der alte Befund: der Eintrag liegt unter dem sichtbaren Teil (die
  Meldung nannte nur die waagerechte Lage - es war die senkrechte), die Maus darunter ausserhalb jedes
  Fensters. Ein Mensch dreht das Mausrad - der Test tat es nicht. Ursache im Test.
- Stil wie NRC: das Menue war 1688 hoch (Fenster 4..1692) und lief unten ueber den Bildschirm hinaus;
  "Notify" (1644..1692) und "Im Spiel testen" lagen ausserhalb - fuer einen Menschen unerreichbar (der
  Selbsttest kam nur hin, weil er die Maus auch ausserhalb des Bildschirms setzen kann). Ursache im
  Programm. NRC (Qt 5.15, Fusion) hat keinen Scroller (SH_Menu_Scrollable 0) und bricht ein zu hohes
  Menue in Spalten um (QMenuPrivate::updateActionRects, qmenu.cpp:478-489: passt eine Zeile nicht mehr
  unter column_max_y, beginnt rechts daneben eine neue Spalte).

Behoben:
- Programm (gui/app_karte_qtmenue.cpp): die Menues im Stil von NRC brechen wie QMenu in Spalten um
  (zeileAnfangen/zeileBeenden je Eintrag, Untermenue und Trenner; Breite = Spalten mal
  max_column_width, Hoehe = tiefste Zeile wie sizeHint). Abweichung: column_max_y ist die Hoehe, in der
  ImGui das Menue zeigen kann (GetPopupAllowedExtentRect, ohne Taskleiste) statt des ganzen
  Bildschirms (Windows: UseFullScreenForPopupMenu) - sonst laege die letzte Zeile einer vollen Spalte
  unter der Taskleiste. Untermenues in einer linken Spalte gehen rechts neben dem ganzen Menue auf (ImGui
  meidet das ganze Elternfenster), Qt oeffnet sie neben der Spalte.
- Selbsttest (gui/selbsttest_karte8.inc): so viele Fuelleintraege, wie Zeilen des Programms auf den
  Bildschirm passen (hier 51) - das Menue ist damit auf jedem Bildschirm zu hoch. Stil des Programms:
  Melden liegt unter dem sichtbaren Teil, das Menue rollt; `radBisSichtbar` dreht das Mausrad ueber dem
  Menue, bis der Eintrag zu sehen ist, dann der Klick. Stil wie NRC: mindestens zwei Spalten, jeder
  Eintrag auf dem Bildschirm, der Klick schaltet Melden wieder an.
- rot (s8rot2, Test neu, Programm alt): Stil des Programms gruen (Mausrad), Stil wie NRC "114
  Eintraege, 1 Spalten, 85 ausserhalb von 4..1524 (Fenster 4..5532)".
- gruen (s8gruen2): 21 OK / 0, "85 Eintraege, 3 Spalten, 0 ausserhalb von 4..1524 (Fenster 16..1524)".

## English

## Deutsch

## 中文

## 日本語
