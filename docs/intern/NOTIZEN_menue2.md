# Notizen Menü-Editor, Stufe M2 (Zweig menue2)

Auftrag: SP\auftrag_m2.md mit SP\auftrag_gemeinsam.md; Ziel (shank) ein Widget-/
Menü-Editor für JKA, JK2 und Movie Duels nach Art von Unreal, Vorbild
MovieDuels_Settings_Menu. Dazu die Wünsche aus dem Hauptstrang: Vergleich mit
dem echten Spiel, Modelle 1:1 über die GPU-Figurendarstellung, Feeder,
Bildzeit, Ebenenliste, Lichtschwerter 1:1, Grundsatz „nichts zuschneiden“,
rechte Spalte mit Reitern, Live-Diagnose, Panels für spätere Fenster-Freiheit,
zuletzt das moderne Aussehen.

## Aufbau (neu in M2)

| Teil | Datei | Herkunft |
|---|---|---|
| RoQ-Decoder, -Kodierer, Video/GIF-Import | `src/menue/roq*.cpp`, `gui/videoimport_win32.cpp` | OpenJK code/client/cl_cin.cpp (Decoder 1:1), Kodierer eigen, Media Foundation |
| Feeder (Spezies, Köpfe, Farben, Sprachen, Karten, Videos …) | `src/menue/feeder.cpp` | OpenJK ui_main (UI_BuildPlayerModel_List, UI_FeederItemText …) |
| 3D-Modelle, Lichtschwerter | `src/menue/modell.cpp`, `gui/gpumap_win32.cpp` (Menü-Ziele) | ui_shared Item_Model_Paint, tr_light R_SetupEntityLighting, ui_saber UI_SaberDrawBlade/UI_DoSaber, tr_surface RB_SurfaceSaberGlow/RB_SurfaceLine/DoSprite; Movie Duels ui_saber (cg_SFXSabers 0/4), MD-Renderer (Griff-Klecks 2.5) |
| Detailbereich, Bauteil-Bibliothek | `src/menue/bauteile.cpp` | eigen |
| Klänge als Ereignisse | `src/menue/skript.cpp` | ui_shared Item_SetFocus, Script_Play, Menus_Activate; snd_mem .wav→.mp3 |
| Texte schreiben (.str/.sp) | `src/menue/texte.cpp` | StringEd/StripEd-Format, eigen |
| Prüfungen vor dem Export | `src/menue/pruefung.cpp` | Grenzen aus OpenJK/MD ui_shared.h, Regeln eigen |
| Live-Diagnose | `src/menue/diagnose.cpp`, `src/menue/menudef_sp.inc` | eigen; Script_Show/Hide/SetFocus/Close (Menu_ItemsMatchingGroup, Q_strncmp-Platzhalter), OpenJK code/ui/menudef.h |
| Im Spiel testen | `src/menue/imspiel.cpp` | Regeln wie karte::spielTestPlanen (FS_Startup) |
| Schrift aus TTF | `src/menue/schriftbau.cpp`, `gui/schrift_ttf.cpp` | tr_font.h dfontdat_t (7180 Bytes), stb_truetype aus Dear ImGui |
| Oberfläche: Panels, Reiter, Leiste, Kacheln | `gui/app_menue.cpp` | eigen |

Quellen nur lesend nach SP\m2\oj (OpenJK) und SP\m2\md (Movie Duels,
github.com/MovieDuels/MovieDuels-SJE-SP-Rend2) geholt.

## Ergebnisse je Punkt

1. **Offene M1-Punkte** – fertig: Festbreitenschrift im Code-Editor, Feeder-Daten,
   Modelle über die GPU-Figurendarstellung (vorher CPU-Raster, kaputt), animierte
   Übergänge, SP-Schriftnummern, strenges assetGlobalDef.
2. **RoQ** – Decoder 1:1 (cl_cin.cpp), Kodierer, Video/GIF → `video/`, `videoMap` in Shadern.
3. **Detailbereich und Menüsätze** – drei Ebenen aus der Bibliothek; menus.txt öffnet alle
   Dateien, `open` wechselt die Datei.
4. **Bibliothek** – Vorlagen auf die Fläche ziehen, Auswahl als eigene Vorlage.
5. **Klänge** – soundLoop (startBackgroundTrack), play, playlooped, focusSound bzw.
   itemFocusSound aus assetGlobalDef; menuEnter/ExitSound spielt die Engine nicht (nur
   registriert). Im Selbsttest stumm, gezählt.
6. **Mehrsprachig** – Fenster Texte: eigene @-Texte je Sprache des Spiels, benutzte Schlüssel
   (fehlende per Klick anlegen), Paket `strings/<Sprache>/<PAKET>.str` bzw. `strip/<paket>.sp`
   (alle Sprachen in einer Datei), Vorschau in der gewählten Sprache. Eingaben als Latin-1
   wie die Menüs; asiatische Kodierungen nicht.
7. **Prüfungen** – Grenzen je Fassung (SP 150 Items/64 Menüs/4096 Skript, MP 512/128/2048,
   MD 4096/256/4096, Multi 64/128, Farben 10, Spalten 16), nie sichtbar, fehlende Dateien
   (Bilder/Shader, Modelle, Videos, Schriften, Klänge mit .wav→.mp3), Verträglichkeit
   JA/JK2/MD; Fenster mit Zielfassungen; Export „nur Änderungen“ (offene Menüs, die vom
   Spiel abweichen, plus Paket) mit Warnung, wenn die Engine etwas verliert.
8. **Im Spiel testen** – Paket `zzz_jkstudio_menue.pk3` in `<Einstellungen>/menuetest/<spiel>`,
   Start mit `+set fs_homepath` (höchster Rang, ersetzt gleichnamige Menüs); ohne fs_homepath
   (jasp/jk2sp) nur Mod-Ordner nach Frage. Im Selbsttest nie gestartet.
9. **TTF → .fontdat** – stb_truetype rastert 32..255 (Windows-1252), Packen in 256/512 breite
   TGA (weiß, Deckung im Alpha), Werte wie Ravens Schriften (baseline = Zeilen über der
   Grundlinie − 1, mHeight = Auf- + Abstieg); Rundlauf mit dem Leser der Vorschau.
10. **HUD-Editor** – nicht begonnen (siehe Offen).
11. **Handbuch, Selbsttest, Notizen** – neue Seite „Menü-Editor: Vorschau, Prüfungen und
    Paket“ in vier Sprachen mit Bildern; M1-Bilder neu (neues Aussehen).

Dazu aus dem Hauptstrang:

- **Vergleich mit dem Spiel** (Movie-Duels-Programm mit fs_game base, eigenes fs_homepath,
  stumm, Fenster, nur PrintWindow des eigenen Fensters, eigene PID beendet): gefunden und
  rot→grün behoben: StringEd REFERENCE-Rest, Shader-Stufen einzeln, videoMap,
  Dokument-Reiter, pk3-Ordnereinträge, MP `0_0`, unsichtbares Modell, CPU-Raster,
  `<FEEDER_PLAYER_SPECIES>`, `$whiteimage`, Säbellage, Säbelklingen (Band → Sprites wie
  RB_SurfaceSaberGlow), Laufzeit erbt die Cvars der Vorschau.
- **Lichtschwerter**: Griff aus ui_saber/.sab, liegend (curYaw, 0, 90), Klingen 1:1
  (tagHack mal xscale für alle Säbelarten, Glanz-Sprites, Kern, Tiefe des Griffs), Movie
  Duels cg_SFXSabers 0 und 4. Das Referenzbild des Spiels mit Vorgabe 4 zeigte eine
  fehlende SFX-Textur (fs_game base ohne MD-Daten) – darum mit `+set cg_SFXSabers 0`
  verglichen: deckungsgleich.
- **Rechte Spalte** mit Reitern Hierarchie/Text/Eigenschaften, Fläche in voller Höhe,
  altes Layout als Einstellung, Umbruch oder Seitwärts-Scrollen einstellbar, Minimap im
  schmalen Editor kleiner/aus; jedes Teil ein Panel mit fester ID.
- **Live-Diagnose**: entprellt im Hintergrund, Wellenlinien, Randsymbole, Tipps, Probleme
  unter dem Text, Markierung auf der Fläche, Schnellkorrekturen (Strg+.).

## Messungen

Bildzeit des Arbeitsbereichs (Selbsttest-Vergleich, unter Last durch parallele Bauten):

| Menü | vorher | nachher |
|---|---|---|
| ui/main.menu (SP) | – | 0.6–0.8 ms |
| ui/jamp/main.menu | – | 0.6–0.8 ms |
| ui/character.menu (Figur) | 82.05 ms (CPU-Raster) | 1.0–1.9 ms |
| ui/saber.menu (Säbel mit Klingen) | – | 1.8–3.9 ms |

Diagnose über alle Originalmenüs (ctest menue_rundlauf, Mass für Fehlalarme): JA
1 Klammer, 80 nie sichtbar, 62 fehlende Dateien, 54 Ziele ohne Item/Menü; JK2 2/59/13/84;
MD 18 Anführungszeichen, 185 Engine-Meldungen, 8 Klammern, 408/173/130 – jeweils die
Regeln der Engine (z. B. MP `play sound/interface/button1` ohne Anführungszeichen: botlib
trennt am `/`, das Spiel sucht „sound“).

## Zuschnitte und Sonderfälle der Engine

Gesucht: Code, der einzelne Originalmenüs kennt. Gefunden keiner in src/menue und
gui/app_menue.cpp (grep nach Menü-/Itemnamen). Übernommene Sonderfälle der Engine, je mit
Quelle im Code:

- Itemnamen, die der Engine-Code selbst anfasst (Menu_FindItemByName/ShowItemByName mit
  festem Namen bzw. `%s_fbutton`, `%s_hexpic`, `%s_iconpic`, `%s_level%ddesc`; uiScript-
  Argumente) – kein „nie sichtbar“ (`pruefung.cpp` kEngineNamen).
- uiScript-Befehle für Figur/Säbel suchen die Items „character“, „saber“, „saber2“
  (UI_UpdateCharacter), `ui_char_model` ist in ItemParse_asset_model fest verdrahtet.
- Script_Close „all“, Platzhalter `name*` mit Q_strncmp (Groß/Klein), setfocus findet nur
  das erste Item eines Namens.
- Menüsatz-Dateien ui/menus.txt, ui/jampmenus.txt, ui/jk2mpmenus.txt,
  ui/MovieDuels-MP-Menus.txt (ui_menuFiles-Vorgaben).
- Säbel: Hack „auf die Seite“ (curYaw, 0, 90), tagHack-Versätze mal DC->xscale.
- SP liest Zahlen mit atoi/atof: Namen aus menudef.h mit Wert 0 (LISTBOX_TEXT,
  FEEDER_SAVEGAMES) ergeben zufällig dasselbe – keine Meldung; andere mit Korrektur auf die Zahl.
- Klänge: .wav fehlt → .mp3 (S_LoadSound_Actual).

Zufällige, selbst gebaute Menüs (ctest menue_zufall, je 300 für JA SP, JA MP, MD SP):
byte-gleich, Vorschau ohne Absturz, alle Diagnose-Stellen im Text, jede Korrektur lesbar,
fehlende/zusätzliche } in 600/600 Fällen gemeldet.

## Abweichungen vom Spiel (bewusst, offen benannt)

- Zufallsanteile (Q_flrand bei Säbelradius/Griff-Klecks) nehmen ihre Mitte – die Vorschau flackert nicht.
- Movie Duels cg_SFXSabers 1–3 und 5–11 (Ep1/2/3, OT, RotJ, TFA, Unstable, Rebels, CW, Maul)
  und die Unstable-Säbelarten nicht nachgebildet – die Vorschau meldet es; schwarze Klingen
  (blendFunc blend/GL_ZERO) nicht.
- Fehlende Textur einer Klinge: die Engine nimmt das Ersatzbild, die Vorschau zeichnet nichts.
- Eigene Texte nur Latin-1 (westliche Sprachen).

## Geprüft am Spiel / nicht geprüft

Geprüft (Movie Duels SP, fs_game base): Hauptmenü SP (Logo, Seitentexte, Video, Fokus),
Figurenmenü (Modell, Haut, Spezies-Text, Farbfelder), Säbelmenü (Griff, Lage, Klingen mit
cg_SFXSabers 0), Einstellungsmenü.

Nicht geprüft: Mehrspieler-Menüs (das MD-MP-Programm zeigt seine eigene Oberfläche),
Haltung der Figur (Animation), OpenJK-Säbel gegen ein OpenJK-Programm (nur Quellport, kein
Programm vorhanden), Klänge, Untermenüs außer Haupt/Figur/Säbel/Einstellungen, Test im
Spiel mit einem echten Start (nie gestartet), TTF-Schriften im Spiel.

## Tests (Schlussrunde)

Siehe Abschnitt „Schlussrunde“ unten (Zahlen nach dem letzten Lauf).

## Offen

- HUD-Editor (hud.menu, ownerdraws mit Spielwerten) – Punkt 10, nicht begonnen.
- Weitere Movie-Duels-Säbelarten, schwarze Klingen.
- Docking/Fenster frei anordnen (eigene Stufe; die Panels sind vorbereitet).
- Asiatische Kodierungen für eigene Texte.

## Changelog

## English
- Menu editor: the right column has tabs **Hierarchy**, **Text** and **Properties**, the canvas uses the full height; the old layout and line wrapping are preferences.
- Live diagnosis while typing: wavy lines at the exact place, margin symbols, hover with the game's message and an explanation, a problems list, marked items on the canvas and quick fixes (Ctrl+.) - braces and quotes, unknown keywords, numbers, duplicate names, missing targets, cvar typos, limits, missing files.
- Videos (RoQ, import from video/GIF), 3D models and lightsabers drawn like the game, a widget library with your own templates, menu sets, sounds, texts in several languages, checks for Jedi Academy/Jedi Outcast/Movie Duels, export of only the changed menus, a test in the game and fonts from TrueType.

## Deutsch
- Menü-Editor: Die rechte Spalte hat die Reiter **Hierarchie**, **Text** und **Eigenschaften**, die Fläche nutzt die volle Höhe; das alte Layout und der Zeilenumbruch sind Einstellungen.
- Live-Diagnose beim Tippen: Wellenlinien an der genauen Stelle, Randsymbole, Tipp mit der Meldung des Spiels und einer Erklärung, eine Liste der Probleme, markierte Items auf der Fläche und Schnellkorrekturen (Strg+.) - Klammern und Anführungszeichen, unbekannte Schlüsselwörter, Zahlen, doppelte Namen, fehlende Ziele, Cvar-Tippfehler, Grenzen, fehlende Dateien.
- Videos (RoQ, Import aus Video/GIF), 3D-Modelle und Lichtschwerter wie im Spiel, eine Bauteil-Bibliothek mit eigenen Vorlagen, Menüsätze, Klänge, Texte in mehreren Sprachen, Prüfungen für Jedi Academy/Jedi Outcast/Movie Duels, Export nur der geänderten Menüs, ein Test im Spiel und Schriften aus TrueType.

## 中文
- 菜单编辑器:右侧栏带有 **层级**、**文本** 和 **属性** 选项卡,画布占满整个高度;旧布局和自动换行可在首选项中设置。
- 输入时实时诊断:在确切位置显示波浪线、边栏符号、悬停显示游戏消息及说明、问题列表、画布上标出的项以及快速修复(Ctrl+.)——括号和引号、未知关键字、数字、重复名称、缺失目标、cvar 拼写错误、限制、缺失文件。
- 与游戏一致的视频(RoQ,可从视频/GIF 导入)、3D 模型和光剑,带自定义模板的控件库、菜单集、声音、多语言文本、针对 Jedi Academy/Jedi Outcast/Movie Duels 的检查、仅导出已更改的菜单、游戏内测试以及由 TrueType 生成的字体。

## 日本語
- メニューエディター: 右列に **階層**・**テキスト**・**プロパティ** のタブ、キャンバスは全高を使います。旧レイアウトと行の折り返しは設定で選べます。
- 入力中のライブ診断: 正確な場所の波線、余白の記号、ゲームのメッセージと説明のツールチップ、問題一覧、キャンバス上の項目の表示、クイック修正(Ctrl+.)。括弧と引用符、不明なキーワード、数値、重複した名前、存在しない対象、cvar の打ち間違い、制限、欠けているファイル。
- ゲームと同じ動画(RoQ、動画/GIF から取り込み)、3D モデル、ライトセーバー、独自テンプレート付きの部品ライブラリ、メニューセット、サウンド、複数言語のテキスト、Jedi Academy/Jedi Outcast/Movie Duels 向けの検査、変更したメニューだけの書き出し、ゲーム内テスト、TrueType からのフォント。
