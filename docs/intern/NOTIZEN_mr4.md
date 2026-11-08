# Notizen MR4: Modell-Block-Reste 5, 6, 7 (08.11.2026, Worktree `Y:\mr4`, Zweig `mr4`, Basis fe1f374c3)

Build `Y:/mr4/build-mr4` (wie `Y:/karte/build-test`), Laeufe `m4_*` unter `Y:\mr4\laeufe`, Skripte `Y:\werkzeuge\m4\`.

## Punkt 5: CJK-Umbruch ausserhalb des Model-Tabs (mit P18)

- Vorher brach nur der Model-Tab japanische und chinesische Saetze zwischen den Zeichen um (`gui/textumbruch.h`, p_render);
  ueberall sonst die ImGui-Regel (ImFontCalcWordWrapPositionEx: nur an Leerzeichen und nach `.,;!?"` / `、。` - die erste Zeile
  endete am ersten Komma).
- Jetzt im ganzen Programm: 208 Text-Aufrufe in 46 Dateien (Mission, Karten-Editor mit Optimierer/Lump-Editor/Dialogen, Menue-Editor
  mit Seitenleisten, Helfer, Tutorials, KI, Zeitleiste, Update, Wegpunkte, Figur animieren ...) auf `cjkText*`, dazu neu
  `cjkTextSize` (statt `CalcTextSize(..., breite)`, 13 Stellen) und `cjkAddText` (statt `AddText(..., breite)`, 11 Stellen),
  damit reservierte Hoehe und gezeichnete Zeilen zusammenpassen. Ohne CJK-Zeichen oder ohne Umbruch ruft jede Funktion genau
  ihr ImGui-Gegenstueck (gleiches Bild in en/de/es/cs).
- Gemessen wird die Zeile jetzt als Summe der Vorschuebe (CalcTextSizeA ohne das Aufrunden je Aufruf): aufgerundet je Stueck
  brach eine CJK-Zeile bis zu einem Bildpunkt je Zeichen zu frueh um.
- Neu `tools/lint_cjkumbruch.py` (ctest `lint_cjkumbruch`, `lint_cjkumbruch_selbsttest`): kein `ImGui::TextWrapped`, kein
  ImGui-Text zwischen `PushTextWrapPos`/`PopTextWrapPos`, kein `CalcTextSize`/`AddText` mit Umbruchbreite in gui/.
- P18 (Codepruefung seit rc610): die Messdaten der Testhilfe (`cjkUmbruchStand`) fuellen sich nur im Selbsttest
  (`BHED_EDITORTEST` gesetzt, Feld `messen`); sonst nur die Zeilen.
- Selbsttest `cjkweit` (karteneditor, `BHED_KARTE_NUR=cjkweit`, Lauf mit SPRACHE=ja / zh-Hans): Startflaeche (Karten mit
  AddText), Tutorials-Fenster, Helfer "Was willst du zuerst tun?" - jede umbrochene Zeile ausser der letzten ist so voll, dass die
  naechste Gruppe (Stueck mit den Zeichen, die nach JIS X 4051 nicht am Zeilenanfang stehen, neu `bhed::gruppeEnde`) nicht mehr
  hineinpasst; dazu P18.

| Pruefung | rot | gruen |
|---|---|---|
| lint_cjkumbruch | 232 Stellen | 0 |
| cjkweit ja | `m4_cjk_rot_ja` 13/4 (0 CJK-Texte im Umbruch je Stelle; P18 "Aufrufe 0 -> 1, Zeilen gemessen 0 -> 8") | `m4_cjk_gr3_ja` 17/0 (Startflaeche 2159 Texte, Tutorials 114, Helfer 696) |
| cjkweit zh-Hans | `m4_cjk_rot_zh` 13/4 | `m4_cjk_gr3_zh-Hans` 17/0 |

- Zwischenlauf `m4_cjk_gr2_ja` 16/1 war ein Fehler der Pruefung (Naeherung "3 x erstes Zeichen" statt der Gruppe; "(ICARUS)。"
  und "ニュー・" haengen zusammen) - mit `gruppeEnde` gruen; ctest `umbruch` prueft gruppeEnde.
- Handbuchbilder neu (angesehen): ja/zh `h_start`, `h_ueber`; ja `h_helfer`, `h_helfer_zeig`, `h_helfer_mcp`, `h_ki_frage`
  (Laeufe `m4_hb_*`); zh Helfer siehe Schluss.

## Punkt 6: Spielnachweise GLA

- Aufbau wie Stufe 0 (`nachweise_im_spiel.md`): Testkarte `raum_licht.bsp` als `ms0kart`, Skript `kam3` (Kamera von oben,
  BOTH_CIN_16, 10,2 s), eigener fs_homepath je Lauf, Fenster ausser Sicht, stumm (`Y:\werkzeuge\m4\ms6\`, `aufbau6.py`, `md.py`).
- Neu gebaute Karten-GLA (`Y:\werkzeuge\m4\glagross.py`: Skelett und Bewegung aus Ravens `_humanoid_t1_sour`, eigener Kopf, eigene
  Frames und eigener Vorrat - nicht Ravens Datei umbenannt; mit "einmalig" je Knochen und Frame ein eigener Vorratseintrag, also so
  gross, wie eine GLA mit diesen Frames werden kann). Movie Duels SP:

| Lauf | Frames | Datei | Vorrat | BOTH_CIN_16 ab | Ergebnis |
|---|---|---|---|---|---|
| g3124 | 3124 | 1,5 MB | 71910 | 1321 | geladen (sucht danach `_humanoid_ms0kart.cfg`), Figur geht 588,1181 -> 469,249 |
| g65535 | 65535 | 59 MB | 3473355 | 63801 | gleich (467,250) |
| g131070 | 131070 | 118 MB | 6946710 | 63801 | gleich (467,250) |
| gunter / gkreuz | 131070 | 118 MB | 6946710 | 59152 / 65400 (bis 65603) | gleiche Bahn (030 s 634,1133 / 640,1124 ... 042 s 592,1186) |

- Ergebnis: in Movie Duels keine Speichergrenze bis zur groessten moeglichen Karten-GLA (Anfang hoechstens 65535, Anzahl
  hoechstens 65535 -> 131070 Frames; Vorrat 24 Bit). Frames ueber 65535 spielen (G2 rechnet in int, nur animation_t.firstFrame
  ist unsigned short). Erster gkreuz-Lauf: das Skript lief nicht an (Ansicht blieb, Protokoll gleich) - einmal wiederholt, gruen.
- Pruefung im Tool: cfgPruefen meldet schon Anfang/Anzahl > 65535 (ErsterFrame/Anzahl) und GlaVorrat > 24 Bit; eine weitere
  Groessengrenze haette keine Quelle. ctest `modell_dateien`: neue Zeilen fuer die gemessenen Faelle (131070 Frames, Sequenzen ueber
  65535 hinaus: kein Befund, MdSp und JkaSp).
- Original-JKA (Steam angemeldet, vor jedem Start geprueft, GameData danach gleich): o131070 und o3124 bleiben nach `setviewpos`
  stehen - das Skript startet nicht, das Bild steht (auch mit der kleinen GLA, also nicht die Groesse). Stufe 0 lief am 04.11. mit
  demselben Skript; vermutlich haelt das Original ohne Fokus an (Fokus-Ausnahme gilt nur fuer Movie Duels) - offen.
- Die neu gebaute GLA aus dem Werkzeug selbst ("Fuer das Spiel bauen") ist seit Stufe 7 in MD und im Original belegt
  (NOTIZEN_mb7.md, Endlage < 1 px). MP uebersprungen (MD-MP-Daten bewusst geloescht).

## Punkt 7: Helligkeit Original-JKA gegen Vorschau

- Die 75,5 waren ein Vorschaufoto mit den Regeln von Movie Duels (Selbsttest `jk_mds2`, MD-Daten): dort zieht die Gamma-Stufe von
  Movie Duels (r_gammaShaders, `spielGammaWert`) Grau 32 auf 30. Mit den Daten und Regeln des Original-JKA (`mdspiel` mit neu
  `BHED_MB_SPIEL=jka`) ist die Vorschau gleich hell (L 76,74 gegen 76,73), aber ohne den Farbstich des Originals: Spielfoto R 77,49
  G 76,71 B 77,33, Vorschau 76,81 / 76,72 / 76,65; haeufigster Wert Spiel (33, 32, 33), Vorschau (32, 32, 32).
- Ursache (OpenJK 1:1): Upload32 (rd-vanilla/tr_image.cpp:629-689) laedt jedes Bild mit allowTC bei TC_S3TC_DXT als DXT5
  (r_texturebits 0), jede Mip-Stufe einzeln; r_ext_compress_textures ist 1 (tr_init.cpp:296-388; das Protokoll des Originals sagt
  "using GL_EXT_texture_compression_s3tc", Movie Duels "...ignoring texture compression"). 5:6:5 macht aus Grau 32 (33, 32, 33);
  tr.defaultImage hat auch Alpha 32 (memset), also DXT5 mit Alpha. Ausgenommen: noTC (tr_shader.cpp:2324), Lightmaps
  (r_ext_compress_lightmaps 0).
- Neu `bhed/dxt.h` (`dxt5Rundlauf`: Farbblock mit zwei 5:6:5-Farben auf der Hauptachse und zwei Zwischenstufen, Alphablock mit
  acht Stufen), angewandt beim Hochladen (`ladeMitStufen`) unter Jedi Academy und Jedi Outcast (nicht Movie Duels), nach der
  Pyramide aus dem unkomprimierten Bild; `noTC` im Shaderverzeichnis gelesen. Den Block packt im Spiel der Treiber - dessen
  Verfahren steht nicht in der Quelle; gemessen gegen das Foto (unten).

| Pruefung | rot (`m4_hell_rot2`) | gruen (`m4_hell_gr`) |
|---|---|---|
| ohne Requisit gegen `pj/jka_ohne1` | Mittel 84,91/84,91/84,91 (Spiel 85,68/84,92/85,68), gleiche Punkte 23,6 % | 85,68/84,91/85,68, gleiche Punkte 99,94 %, mittlere Abw. 0,001 |
| mit Requisit gegen `pj/jka_mit1` | 76,81/76,72/76,65 (Spiel 77,49/76,71/77,33), 27,7 % | 77,50/76,72/77,34, 96,8 %, 0,037 |
| Movie Duels (`m4_hell_md`) | - | unveraendert 75,55/75,46/75,40, Grau 30 |

- Die Haeufigkeiten der Farbwerte stimmen danach bis auf einzelne Punkte (Spiel (33,32,33) 1803538, Vorschau 1803539; (247,247,247)
  7354 / 7354). Das Requisit (eine echte Textur) ist die Gegenprobe ausserhalb des Standardbilds.
- ctest `dxt` (neu): Grau 32 -> 33 32 33, Weiss bleibt, Alpha 100 -> 109, Teilblock.
- Die beiden Spielfotos stammen vom 07.11. (p_jka); ein neuer Original-Start war nicht noetig. Bildzeit/Ladezeit mit DXT nicht
  gemessen (allein, Hauptchat).

## Schluss

- Sauberer Neubau (`--clean-first`) 0 Warnungen; ctest 238/238 (237 mit -j 12, `q3map2_pr316` allein danach). Rot dabei:
  `hilfe_orte` - der Generator las `ImGui::TextDisabled(tr(...))` im Pfade-Dialog, nach der Umstellung fehlte `PathsJk2Note` im
  Helfer-Index; `tools/gen_hilfe_orte.py` liest jetzt auch `cjkText*` (Index wieder gleich). Ebenso `lint_imgui_context` 3 ->
  0 (cjkText* schicken immer ein Element ab) und `lint_doppelanzeige` (liest auch cjkText*).
- Selbsttests: Modus 1 `m4_modus1` 2635/0, `m4_rdcjk_ja`/`_zh` 13/0, `m4_cjk_gr3_*` 17/0, `m4_hell_gr`/`m4_hell_md` 5/0,
  `m4_mission1` 98/0, `m4_menueeditor` 2780/4 (nur M6 Fluss: "1 Menues" statt 4 - dasselbe Bild schon am 06.11. in
  `e10_menueeditor_es` und `r2be_me_ja` unter `Y:\karte\laeufe`, also Altbestand; der Fluss rechnet ohne Text). Lints wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung
  wie vorher), `lint_cjkumbruch` 0, Handbuch `--pruefen` ok.

## Offen

- Original-JKA: grosse Karten-GLA nicht gemessen (Spiel haelt ohne Fokus an, Ausnahme nur fuer Movie Duels).
- Bildzeit und Ladezeit: CJK-Umbruch (nur ja/zh mit Umbruch) und DXT beim Hochladen unter JKA/JK2 nicht gemessen (allein, Hauptchat).
- Gegenprobe DXT auf einer weiteren Karte des Originals (hier nur Standardbild und Requisit).
- Handbuchtext zur Vorschau (DXT unter JKA/JK2) nicht ergaenzt; Changelog steht unten.
- Menue-Editor-Seitenleisten in ja/zh nicht neu fotografiert (nur Startflaeche, Helfer, KI, Ueber).

# MR5 (08.11.2026, gleicher Worktree, Basis c5d16c76b, Laeufe `m5_*`)

## MR5 Punkt 1: menueeditor "M6 Fluss" (4 FEHLER seit 06.11.)

- Ursache (Programmfehler, nicht der Test): Menuefluss, Tab-Folge, Lesbarkeit und das Pruefen-Fenster merkten sich ihr Ergebnis
  unter "Adresse des Dokuments + Pufferversion". Nach `schliesseAlleOhneSpeichern` legt `oeffne` das neue Dokument an derselben
  Heap-Adresse an, und ein frisch geoeffneter Puffer hat immer Version 1 (`setzeText`). Im vollen Lauf war vorher ein anderes,
  unveraendertes Dokument offen - der Fluss zeigte dessen Menue ("1 Menues, 1 unerreichbar, 0 Kanten"), "credits" fehlte im
  Fenster, der Zurueck-Knopf landete im falschen Menue. Allein (`BHED_MENUE_NUR=m6`, `m5b+m6`) gruen, weil kein passender
  Vorgaenger da war (`m5_me_m6_rot`, `m5_me_m5bm6_rot` 17/0, 65/0).
- Behebung: jedes Dokument bekommt eine eigene, nie wiederkehrende Nummer (`Dokument::nummer`); alle vier Schluessel nehmen sie statt
  der Adresse (app_menue_m6.inc flussAktuell/tabFolgeVon/lesbarkeitVon, Pruefen-Fenster in app_menue.cpp).
- Neue Pruefung in M6 (vor der Probe): 200 Mal anderes Menue oeffnen, Fluss und Tab-Folge rechnen, alles schliessen, Probe oeffnen -
  Fluss muss 4 Menues zeigen, die Tab-Folge darf das Item des vorigen Dokuments nicht enthalten.

| Lauf | Ergebnis |
|---|---|
| `m5_me_m6_rot2` (vorher) | 17/1: "200 Wechsel, Fluss falsch 200, Tab-Folge falsch 200" |
| `m5_me_gr_en` / `_ja` / `_es` (nachher, voll) | je 2784/0 (vorher 2780/4), "Fluss falsch 0, Tab-Folge falsch 0" |

## MR5 Punkt 2: DXT-Vorschau schneller, Bild gleich

- Die Kompression lief je Bild im Grafikfaden beim ersten Zeichnen (ladeMitStufen), jede Stufe nacheinander, je Block mit
  Potenzverfahren in double: 1024 x 1024 samt Pyramide rund 35 ms (unter Fremdlast gemessen).
- Jetzt (src/dxt.cpp): `dxt5Rundlauf(const Ebene*, n)` nimmt alle Stufen einer Pyramide in einem Auftrag und verteilt die
  Blockzeilen auf die festen Arbeitsfaeden (bhed/arbeiter.h, kleine Bilder unter 16 Blockzeilen ohne Faeden); jeder Block rechnet fuer
  sich, die Plaetze teilen keinen Punkt. Bloecke in einer Farbe (Alpha egal) nehmen direkt e(q(Farbe)) - genau der Wert, auf den der
  allgemeine Weg dort kommt (Mittel exakt, Streuung 0, Achse (1,1,1), t = 0, Zwischenstufen (3e+1)/3 = e).
- "Ergebnis je Bild merken" nicht gebaut: nach dem Kartenwechsel gibt gpumap alle Bilder frei, ein Zwischenspeicher haette die Bilder
  im Hauptspeicher verdoppelt; mit den Faeden bleibt pro 1024er-Bild wenige ms.
- ctest `dxt` erweitert: die Fassung aus MR4 liegt wortgleich im Test (`referenz`); 48 Einzelbilder (1x1 bis 512x512, Rauschen,
  Verlauf, Flaechen, gleiche Farbe mit anderem Alpha, Kanten, Teilbloecke) und 3 Pyramiden (1024x1024, 300x77, 8x512; 30 Stufen) Byte
  fuer Byte gleich. Gegenprobe, dass der Test anschlaegt: absichtlich falscher Gruenkanal im schnellen Weg -> "Pyramiden: 30 Stufen,
  davon abweichend 11", 4 Fehler; zurueck -> 0.
- Im Programm: dasselbe Kartenbild mit der MR4-exe und der neuen exe (`m5_sour_mit`, `m5_sour_neu`, t1_sour unter JKA-Regeln) - die
  Kartenansicht 1920 x 1440 ist Bildpunkt fuer Bildpunkt gleich (Unterschied nur in der Statuszeile ausserhalb der Ansicht).
- Zeit (ctest-Ausgabe, Fremdlast durch andere Agenten - nur Anhalt): 1024 x 1024 samt Pyramide 35,0 -> 3,4 ms (MR4 -> jetzt).
  Ladezeit einer Karte allein: offen (Hauptchat, ohne Fremdlast).

## MR5 Punkt 3: DXT-Gegenprobe auf einer zweiten Original-Karte, Handbuchtext

- Kein neuer Spielstart: vorhandenes Spielfoto des Original-JKA (jasp.exe, 03.11., Endrunde rc608) `SP/er1/fotos/er1_sour.png` -
  t1_sour (Entities wie in er1.pk3), 1920 x 1440, Protokoll "using GL_EXT_texture_compression_s3tc", "compressed textures: enabled".
  Die Karte diente nicht zum Finden der DXT-Regel (gefunden auf m0raum, MR4).
- Vorschau gleich aufgebaut (Selbsttest `geschuetze` mit BHED_GV_*, er1.pk3, Daten "GameData original/base", Regelwerk OpenJK =
  JKA-Regeln, Kamera aus viewpos 9133 -4842 100 Gier 225, Nachkommastellen unbekannt -> je +0,5, Nick 0 wie die dritte Person mit
  Nick 0), einmal mit der exe vor MR4 Punkt 7 (`m5_sour_ohne`), einmal mit DXT (`m5_sour_neu`; `m5_sour_mit` = MR4-exe, gleich).
- Auswertung `Y:\werkzeuge\m4\m5\dxtvgl.py`, Paarbild `Y:\werkzeuge\m4\m5\paar_sour.png` (Spiel, ohne, mit; 1:1, angesehen):

| Mass | Spiel | ohne DXT | mit DXT |
|---|---|---|---|
| R-G / B-G im Mittel | +5,956 / -14,929 | +5,863 / -15,101 | +5,898 / -15,084 |
| haeufigste Farbe (25,21,14) | 27551 | 39055 | 27602 |
| (20,17,10) | 22424 | 22967 | 19332 |
| Abstand der Farbhistogramme zum Spiel | - | 0,2111 | 0,1966 |
| Abstand der (R-G, B-G)-Verteilung | - | 0,1043 | 0,0950 |
| Bloecke 480x480, R-G/B-G naeher am Spiel | - | - | 11 von 12 |

- Ergebnis: mit DXT naeher am Original, wie auf m0raum. Gleiche Punkte nur 2,6 % (Kamera auf 1 Einheit ungenau, viewpos schneidet ab) -
  darum Verteilungen statt Punktgleichheit. Block 2,2 (Spieler, Twi'lek) mit DXT 0,03 weiter weg.
- Handbuch "Die Karte": neuer Hinweis zur Texturkompression unter JKA/JK2 (S3TC wie r_ext_compress_textures 1, noTC und Lightmaps
  nicht, Movie Duels aus) in 6 Sprachen (`tools/handbuch/inhalt_mr4.py`), `--pruefen` ok, Handbuch neu erzeugt (karte.html, suche.js).

## MR5 Punkt 4: Menue-Editor-Bilder ja/zh neu

- Bildlauf `menueeditor` mit BHED_MENUE_BILDER=1 (Fenster 2560 x 1500) je Sprache: `m5_mb_ja` 571/6, `m5_mb_zh` 570/7. Die 6 bzw. 7
  roten Zeilen sind dieselben wie im Bildlauf vom 03.11. (`Y:\karte\laeufe\gjambilder` 567/6, `gzhHansmbilder` 566/7, gleiche Namen):
  Codebereich im Reiter Text 154 Punkte zu schmal (waagrechter Bildlauf), "レイアウトを解除"/"解除布局" und "次の場合は隠す"/"在以下情况隐藏"
  nicht gefunden, Eng-Spalte, "##wx" nicht ganz sichtbar, zh Kontext Duplizieren - Altbestand, hier nicht behoben.
- Uebernommen: alle Bilder, die das Handbuch schon hat, ausser den Bildern nach einem Fehlschlag im selben Teil (menue_reiter_properties,
  menue_ereignisse, menue_eng_auf, menue_eng_beide, menue_felder_mehrfach, menue_felder_ownerdraw, zh dazu menue_kontext_leer - die
  bleiben wie vorher): je 56 Bilder ja und zh (darunter alle menue_leiste_*), Ausschnitte 2538 x 1444 aus dem Fenster 2560 x 1500,
  zusammen 13,1 -> 13,3 MB. Angesehen: ja menue_leiste_properties (Ereigniszeile bricht zwischen den Zeichen um), zh
  menue_leiste_events, ja menue_reiter_text.

## MR5 Punkt 5: Original-JKA grosse GLA - offen

- Quelle: `com_unfocused`/`com_minimized` gibt es nur in OpenJK (SDL-Fenster); dort drosseln sie nur die Bildrate
  (common.cpp:1405-1408 `com_maxfpsUnfocused`/`com_maxfpsMinimized`), sie halten nichts an. Das Original (jasp.exe, Raven-Quelle
  win32/win_main.cpp, win_wndproc.cpp) kennt sie nicht: VID_AppActivate setzt nur `g_wv.activeApp`/`isMinimized`, die Hauptschleife
  schlaeft nur minimiert (Sleep 5) und im Debug-Bau ohne Fokus (Sleep 50). Auch cl_main.cpp pausiert nicht nach dem Fokus.
  Stufe 6/7 liefen im Original ohne Fokus (Endlage < 1 px).
- Neu gemessen (Steam angemeldet, vor jedem Start geprueft, kein fremder jasp, GameData danach gleich):
  - `o0`: derselbe Aufbau wie MR4 (`ms0kart`, `npc spawn kyle ms0npc`, `runscript ms0npc ms0/kam3`), aber ganz OHNE Karten-GLA - das
    Skript startet ebenso nicht (Kamera bleibt, 20 Fotos 15-72 s). Also nicht die GLA und nicht ihre Groesse.
  - `o0a`: wie o0, dazu dem eigenen Spielfenster dreimal WM_ACTIVATE (WA_ACTIVE) geschickt (nur die Nachricht, Vordergrund blieb bei
    der fremden PID): keine Wirkung, kein "AppActivate" im Protokoll (developer 1).
  - Der Lauf vom 04.11. (Stufe 0) mit gleicher cfg und gleichem Skript lief; sein Protokoll endet genauso (runscript erscheint nie im
    Protokoll). Was sich seitdem unterscheidet, ist nicht gefunden.
- Offen: Ursache, warum das Skript im Original seit dem 08.11. nicht anlaeuft; damit bleibt die grosse GLA im Original ungemessen.
  Fokus wird nicht gebraucht (Quelle), ein Weg mit Fokus waere ohnehin nicht erlaubt. Skripte `Y:\werkzeuge\m4\ms6\orig_akt.py`,
  Laeufe `Y:\werkzeuge\m4\ms6\laeufe\o0`, `o0a`, Uebersichten `Y:\werkzeuge\m4\m5\o0_uebersicht.png`, `o0a_uebersicht.png`.

## MR5 Schluss

- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) 0 Warnungen; ctest 238/238 (237 mit -j 12, `q3map2_pr316`
  allein danach).
- Selbsttests nach dem Neubau: Modus 1 `m5_modus1` 2635/0, `m5_cjk_ja` / `m5_cjk_zh` (cjkweit) je 17/0; menueeditor en/ja/es je
  2784/0 (Punkt 1, danach nur DXT/Handbuch geaendert).
- Lints wie die Basis: portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung wie vorher, lint_cjkumbruch 0, alle anderen ok.
  Handbuch `--pruefen` ok.

## MR5 Offen

- Zeit allein (Fremdlast durch andere Agenten die ganze Zeit): DXT-Kosten beim Laden einer Karte unter JKA/JK2 (ctest-Ausgabe nur
  als Anhalt 35,0 -> 3,4 ms je 1024er-Bild samt Pyramide), dazu weiter Bild-/Ladezeit CJK aus MR4.
- Original-JKA grosse GLA (Punkt 5): Skript startet im Original seit 08.11. auch ohne Karten-GLA nicht; Ursache offen.
- Menue-Editor-Bildlauf ja/zh: 6/7 rote Zeilen (Altbestand seit 03.11.), die Bilder danach blieben alt.

# MR6 (08.11.2026, gleicher Worktree, Basis 02dfe2d5a, Laeufe `m6_*`)

## MR6 Punkt 1: Original-JKA startet das ICARUS-Skript nicht - Ursache gefunden, grosse GLA gemessen

- Kein Unterschied in den Dateien: Karte, `kam3.IBI`, `ms0.cfg` und das geschriebene `jaconfig.cfg` sind bytegleich mit dem Lauf vom
  04.11. (`sp_654alt/ms0/home_oa`), die Konsolenlogs bis auf Serverkennung und Karten-GLA-Zeilen gleich. Auch nicht der Pfad nach dem
  Umzug: `m6_y` (fs_basepath ueber das subst-Laufwerk Y:) und `m6_c` (derselbe Ordner ueber den echten C:-Pfad) schlagen beide fehl.
  Seit 05.11. liegen Steam-Cloud-Spielstaende in `GameData\base\saves` (shanks, vom April) - im Lauf nicht beteiligt.
- Ursache ist ein Wettlauf in der Ablauf-cfg, kein Spielfehler: `runscript ms0npc ms0/kam3` kam, bevor das NPC skriptfaehig war.
  `npc spawn` legt das NPC sofort mit targetname an (NPC_Spawn_Do, OpenJK `code/game/NPC_spawn.cpp:1688-1689`), die ICARUS-Nummer
  bekommt es aber erst in NPC_Begin (`NPC_spawn.cpp:1245` -> `Q3_Interface.cpp:7517-7526` InitEntity), eine Denkrunde spaeter
  (`:1758-1759`) bzw. bei belegter Stelle in Schritten von 0,5 s (`:1012-1016`). Davor findet runscript das NPC, aber
  `Run(ICARUS_INVALID, ...)` tut nichts und meldet nichts (`Q3_Interface.cpp:7768-7785`, DebugPrint nur im Debug-Bau).
- Die cfg wartet in Bildern (`wait 120` nach dem Spawn, `wait 60` vor runscript). Gemessen mit Zeitmarken im Konsolenlog
  (`orig_m6.py`, logfile 2): das Original laeuft hier mit rund 180 Bildern/s (1000 Bilder = 5,53 s, com_maxfps 85 greift nicht),
  also 180 Bilder = 1,0 s vom Spawn bis runscript. `m6_k` (runscript alle 100 Bilder): der Versuch 1,0 s nach dem Spawn bleibt
  wirkungslos, der bei 1,55 s startet das Skript. `m6_j`: erster Aufruf wirkungslos, zweiter 5,5 s spaeter startet. `m6_e`
  (Skript auf dem Spieler, der schon eine Nummer hat): startet sofort. In `m6_y`/`m6_c` kam dazu "NPC ms0npc ... couldn't spawn,
  waiting 0.50 secs" (Stelle belegt). Movie Duels (OpenJK) haelt com_maxfps 85 ein - 180 Bilder = 2,1 s, darum lief es dort.
  Am 04.11. reichte die Zeit; die Bildrate von damals ist nicht mehr messbar (Fotos ohne Zeitbezug), Last durch parallele Laeufe
  ist die naheliegende Erklaerung.
- Behebung (Laufskript, nicht Programm): `cfg_m6o.cfg` = Aufbau wie MR4, aber `wait 600` nach dem Spawn und `wait 300` vor runscript.
  JK Studio selbst erzeugt keine runscript-Befehle (gesucht in src/), betroffen waren nur diese Spielnachweise.

| Lauf | Aufbau | Ergebnis |
|---|---|---|
| `m6_y` (rot) | wie MR5 `o0` (ohne Karten-GLA), wait 120/60 | Kamera bleibt beim Spieler, 9 Fotos gleich |
| `m6_g` (gruen) | ohne Karten-GLA, wait 600/300 | Kamera von oben ab 14 s |
| `m6_o131070` (gruen) | Karten-GLA 131070 Frames, 118 MB, BOTH_CIN_16 ab 63801 | geladen (sucht `_humanoid_ms0kart.cfg`), Figur 607,1173 -> 492,270, nach dem Skript zurueck |
| `m6_okreuz` (gruen) | dieselbe GLA, BOTH_CIN_16 ab 65400 (bis 65603) | spielt ueber 65535 hinaus, kurze Bahn 657,1140 -> 601,1175 wie in MD (`gkreuz`) |

- Ergebnis Original-JKA: die groesste moegliche Karten-GLA (131070 Frames, Vorrat 6946710 Eintraege) laedt und spielt, ohne
  Speichermeldung. Die Endlage 492,270 ist auf 1-2 Bildpunkte gleich wie mit der kleinen GLA (3124 Frames) im Lauf vom 04.11.
  (`oa`: 493,270). Damit gilt die Aussage aus MR4 Punkt 6 (keine weitere Groessengrenze, Pruefung im Tool: Anfang/Anzahl <= 65535,
  Vorrat 24 Bit) auch im Original. GameData nach jedem Lauf gleich, vor jedem Start Steam/ActiveUser geprueft, kein fremder jasp.
- Laeufe und Skripte: `Y:\werkzeuge\m4\ms6\` (`orig_m6.py`, `cfg_m6*.cfg`, `laeufe\m6_*`).

## MR6 Punkt 2: Bildlauf des Menue-Editors ja/zh gruen, Bilder neu

Die 6 (ja) bzw. 7 (zh) roten Zeilen aus `m5_mb_ja`/`m5_mb_zh` (seit 03.11. auch in en, `er1mbilder`: "Codebereich 154 Punkte") hatten
fuenf Ursachen - drei im Programm, zwei im Test:

- **Suchzeile des Text-Editors (Programm):** Feld fest 14 Zeichen breit, dahinter `.*`, `Aa`, `ab` ohne Umbruch - in der schmalen
  Spalte (Bildlauf 17 Zeichen, von Hand ab 16) lagen `Aa` und `ab` rechts ausserhalb (ScrollMax.x 154, nicht erreichbar). Jetzt gibt
  das Feld bis 6 Zeichen nach, danach brechen die Schalter um (`daneben`); das Ersetzen-Feld hoechstens so breit wie die Spalte.
- **Linke Spalte bei wenig Platz (Programm):** die Enge wird nur gerechnet, solange ein Dokument offen ist. "Links" endet bei 200 % mit
  offener linker Spalte und schliesst alles; "Eng" oeffnet bei 1920 x 1080 / 200 % ein neues Dokument - die Enge stand noch von vorher,
  der Wechsel "eng" fand nicht statt, die Spalte blieb offen. Jetzt vergisst der Zustand die Enge ohne Dokument. Neue Vorstufe in
  `schritteEng` stellt genau diese Folge nach (bisher nur im vollen Bildlauf sichtbar).
- **zh "复制" zweimal im Rechtsklickmenue (Programm/Uebersetzung):** EntDuplicate hiess wie ActCopy; der Test klickte zweimal "Kopieren".
  zh EntDuplicate jetzt "创建副本" (wie Windows/Office), Handbuch zh neu erzeugt; die Pruefung des Kontextmenues verlangt jetzt in jeder
  Sprache, dass kein Befehl zweimal gleich heisst.
- **M5 Layout "Aufloesen" (Test):** nach der neuen Auswahl beginnt die Spalte oben; in der schmalen Spalte lag der Abschnitt Layout
  darunter. Wie vor dem Spacer wird jetzt zum Abschnitt gerollt.
- **3b Felder (Test):** `insBild` fand das Feld nicht, wenn es unter dem Rand lag (wird nicht gezeichnet), und rollte dann gar nicht -
  das Foto `menue_felder_bedingung` zeigte das Feld nicht, der Klick auf die Auswahl ging daneben; "##wx" lag nach dem Rollen auf
  "Neuer Wert" oberhalb. Jetzt rollt `insBild` seitenweise, bis das Feld da ist, und holt es ganz ins Bild (auch nach oben).

| Pruefung | rot | gruen |
|---|---|---|
| voller Bildlauf ja / zh | `m5_mb_ja` 571/6, `m5_mb_zh` 570/7 | `m6_mb_ja` 579/0, `m6_mb_zh` 579/0 (keine "nicht gewertet"-Zeile) |
| Teil m5 ja / zh | `m6_rot_m5_ja` 9/1 | `m6_gr_m5_ja`, `_zh` je 10/0 |
| Teil feldneu ja / zh | `m6_rot_feldneu_ja`, `_zh` je 6/2 | `m6_gr_feldneu_ja`, `_zh` je 8/0 |
| Teil kontext zh | `m6_rot_kontext_zh` 50/1 | `m6_gr_kontext_zh` 51/0 |
| Teil eng (neue Vorstufe, Programm ohne die Behebung) | `m6_rot2_eng_en` 4/3 (normaler Lauf), `m6_rot2_eng_ja` 4/2 (Bildlauf) | `m6_gr_eng_ja`, `_zh` je 7/0 |

- Bilder: alle 59 Handbuchbilder des Menue-Editors, die der Bildlauf erzeugt, je Sprache ja und zh neu (2538 x 1444, darunter die bisher
  ausgelassenen `menue_reiter_properties`, `menue_ereignisse`, `menue_felder_mehrfach`; `menue_eng_auf/_beide`, `menue_felder_ownerdraw`,
  `menue_kontext_leer` hat das Handbuch nicht). Angesehen: ja `menue_reiter_text` (Suchzeile mit `.*` `Aa` `ab` ganz), ja
  `menue_felder_mehrfach` (jetzt mit der Liste One/Two statt dem Kopf der Spalte), ja `menue_reiter_properties`, zh `menue_kontext`
  (复制 und 创建副本 getrennt).

## MR6 Punkt 3: ja "ファイルのキー" in der rechten Leiste

- Nicht abgeschnitten: neue strenge Pruefung im Teil Leiste ("File keys steht ganz in der Spalte", ohne die halbe Lueckenbreite, die
  `klick` zulaesst). Bildlauf (schmalste Spalte, 17 Zeichen) ja: Schalter 2327..2493, sichtbar bis 2493 - endet genau am Rand; zh
  ebenso gruen (`m6_mb_ja`/`_zh`, `m6_gr_leiste_ja` 104/0). In der Vergroesserung stehen "ファイルのキー" und "文件键" ganz da; das Suchfeld
  davor gibt den Platz ab (`breit - sb`). Keine Aenderung am Programm noetig; die Bilder sind mit Punkt 2 neu.

## MR6 Nachtrag: Pruefungsfunde P20, P21, P22, P28, P32 (plaene/codepruefung_seit_rc610.md)

- **P20 [M]** DXT liess die Bilder aller Shader mit `$lightmap`-Stufe unkomprimiert (`!quelle.lightmapStufe`). Das ist das Bild einer
  normalen Stufe (R_FindImageFile aus ParseStage mit allowTC); unkomprimiert bleiben im Spiel nur die Lightmaps selbst
  (r_ext_compress_lightmaps 0), und die laufen nie durch ladeMitStufen. Bedingung gestrichen.
- **P21 [N]** Himmelsseiten: ParseSkyParms laedt sie mit allowTC = !noTC (rd-vanilla tr_shader.cpp:1970) - jetzt DXT5 wie jedes Bild,
  die Seiten tragen noTC des Himmelsshaders (app_ablage.inc).
- **P22 [N]** Regelwerk Movie Duels <-> OpenJK/JK2: `gpu::bilderNeuHochladen()` verwirft Karten-, Himmels- und Figurenbilder
  (Lightmaps bleiben), das naechste Bild laedt nach der neuen Regel.
- Pruefung (Selbsttest geschuetze, BHED_GV_DXT=1, t1_sour aus er1.pk3 unter den Daten des Originals, OpenJK-Regeln): Zaehlung je
  Hochladen (`gpu::bilderDxt`), Himmelsprobe mit sechs Seiten ohne/mit noTC (`gpu::himmelProbe`, t1_sour zeigt in dieser Kamera keinen
  Himmel; t1_fatal mit Himmelsbox zeigte ihn nach oben auch nicht - `m6_nrot_fatal`), danach Wechsel auf Movie Duels.

| Lauf | $lightmap-Stufe DXT5 | Himmelsprobe DXT5 | nach Wechsel auf MD |
|---|---|---|---|
| `m6_nrot2_sour` (rot, ohne die drei Behebungen) | 0 von 32 | 0 von 6 | 0 Bilder neu |
| `m6_ngr_sour` (gruen) | 32 von 32 (305 von 305 Bildern) | 6 von 6, mit noTC 0 | 277 neu, davon DXT5 0 |

- Bildvergleich gegen das Spielfoto (`SP/er1/fotos/er1_sour.png`, jasp.exe; `dxtvgl.py`, gleiche Kamera rot/gruen): Mittel R-G
  +5,871 -> +5,894 (Spiel +5,956), B-G -15,118 -> -15,112 (-14,929), mittlere Abw. 4,964 -> 4,956, gleiche Punkte 1,90 -> 1,93 %;
  Bloecke naeher 5 von 12 (die 32 Bilder mit $lightmap-Stufe nehmen in dieser Ansicht wenig Flaeche ein). Paarbild
  `Y:\werkzeuge\m4\m6\paar_sour_p20.png`.
- **P28 [N]** CJK-Umbruch je Bild: ausserhalb des Selbsttests merkt sich `umbrechen` das Ergebnis je (Schrift, Groesse, Breite, Text),
  was 120 Bilder nicht gefragt war, faellt heraus. Selbsttest cjkweit: stehender Helfer 0,6 s mit messen aus - rot `m6_nrot_cjk_ja`
  798 Aufrufe / 798 umbrochen, gruen `m6_ngr_cjk_ja` 798 / 6 (18/0). Bildzeit allein nicht gemessen (Fremdlast).
- **P32 [N]** Menue-Editor: Texte der Vorschau unter `Dokument::nummer` statt Name, Leiste merkt `Dokument::nummer` statt der Adresse.
  M6: (a) 50 Mal zwei gleichnamige Dokumente mit verschiedenen eigenen Texten - rot `m6_nrot3_m6` "zweiter falsch 50", gruen
  `m6_ngr_m6` 0; (b) 40 Runden Farbzeile offen, alles zu, neues Dokument mit derselben Auswahl - rot 7 von 7 Runden mit gleicher Adresse
  uebernommen, gruen 0 (wieder 7 Runden an derselben Adresse). `m6_ngr_m6` 20/0.

## MR6 Schluss

- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) 0 Warnungen; ctest 237/237 mit -j 8 ohne `q3map2_pr316`,
  `q3map2_pr316` allein danach 1/1.
- Selbsttests nach dem Neubau: menueeditor en / ja je 2787/0 (`m6_ende_me_en`, `_ja`; vorher 2784, dazu die drei neuen Pruefungen),
  cjkweit ja / zh je 18/0 (`m6_ende_cjk_ja`, `_zh`), Modus 1 `m6_ende_modus1` 2635/0; Bildlauf ja/zh 579/0 (Punkt 2).
- Lints wie MR5 (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung wie vorher, lint_cjkumbruch 0, sonst ok);
  Handbuch `--pruefen` ok.

## MR6 Offen

- Bildzeit allein (Fremdlast durch zwei andere Agenten die ganze Zeit): CJK-Umbruch mit Zwischenspeicher (P28), DXT beim Laden.
- P21 nur ueber die Himmelsprobe belegt (sechs Seiten 8 x 8); eine Kartenansicht mit sichtbarer Himmelsbox unter JKA-Regeln wurde
  nicht gefunden (t1_sour ohne Himmelsbox im Bild, t1_fatal nach oben schwarz).
- Statuszeile der Menue-Editor-Bilder zeigt den Pfad des Laufordners (`Y:\mr4\laeufe\...`, wie schon in MR5).

## MR7: Pruefungsfunde P39-P43 (Nachtrag PRUEF5, plaene/codepruefung_seit_rc610.md)

- **P39** Regelwerk umstellen: `gpu::bilderNeuHochladen` verwirft die Bilder BEIDER Szenen (Vorschau und Spielbild des
  Karten-Editors, die geparkte ueber `tauscheSzene` wie `gibAlleSzenenFrei`). Selbsttest geschuetze (BHED_GV_DXT=1, t1_sour aus
  er1.pk3, OpenJK-Regeln): das Spielbild des Karten-Editors (`spielbildZeichnen`, gpu-Szene 1) mit derselben Karte vor und nach
  dem Wechsel auf Movie Duels.
  - rot `m7_rot3_sour`: Szene 1 vor dem Wechsel 259 Bilder (259 DXT5), nach dem Wechsel 0 Bilder neu (alle blieben DXT5).
  - gruen `m7_gr3_sour` (geschuetze 4/0): siehe Tabelle unten. Der erste Lauf `m7_rot_sour`/`m7_gr_sour` las die .bsp aus
    `App::mapBytes` (nie gefuellt) und zeichnete kein Spielbild - Test korrigiert (.bsp aus BHED_GV_PK3), zaehlt nicht.
- **P40** Echtzeit-Licht (und die gebackene Karte) des Karten-Editors unter JKA/JK2-Regeln wie Vorschau und Spiel: das Bildlager
  haelt je Bild eine zweite Fassung "wie im Spiel" (`holen(name, spielDxt(keinTc))`), im Arbeitsfaden mit
  `TextureSet::Tex::stufenWieUpload32` (R_MipMap-Pyramide aus dem unkomprimierten Bild, jede Stufe DXT5 - dieselbe Funktion nimmt
  jetzt `ladeMitStufen` der Vorschau), ohne Gamma und Basisstufe des Editors; Himmelsseiten DXT5 wie `holeHimmel`. noTC je Stufe
  (`SpielStufe::keinTc`, `SpielDarstellung::keinTc/himmelKeinTc` aus P43). Der Editor selbst (Radiant-Ansicht) bleibt unkomprimiert.
  Gilt fuer Grundstufe, erste Stufe, bewegte Grundstufe, weitere Stufen, Gluehen, Himmel, Effekte, Halme und gebackene Flaechen.
  Selbsttest karteneditor rd, neuer Teil "RD DXT" (OpenJK-Regeln, Echtzeit-Licht ohne gebackene Karte): die Mip-Stufen der Bilder,
  mit denen das Echtzeit-Licht zeichnet (von der Grafikkarte gelesen, `kartegpu::texturLesen`), Byte fuer Byte gegen die Stufen der
  Vorschau; danach die fuenf Platten gegen "Built map as in the game" (Spalten, 8 %).
  - rot `m7_rot2_rd`: 4 Bilder je 0 von 8/5/8/5 Stufen gleich (erste Abweichung Stufe 0: 210 statt 211, 40 statt 41); Platten
    scroll/welle/anim/umwelt/grund 0,0015/0,0007/0,0108/0,0029/0,0187 (bestanden, DXT ist klein gegen 8 %).
  - gruen `m7_gr_rd`: alle Stufen gleich (8/8, 5/5, 8/8, 5/5); Platten 0,0003/0,0003/0,0001/0,0002/0,0185; rd 29/0.
  - Fotos `laeufe/m7_gr_rd/fotos/rd_stufen_echtzeit_ojk.bmp` und `rd_stufen_spielbild_ojk.bmp` (1920 x 1080), angesehen.
  - Der erste Rotlauf `m7_rot_rd` mass das Echtzeit-Licht noch mit gebackener Karte (spielGebacken, die Pinsel ruhten) - Test
    korrigiert (backen aus, Schattenwuerfel abwarten), zaehlt nicht.
- **P43** noTC in der Reihenfolge des Skripts: `ShaderInfo::Stage::keinTc` (noTC beim Lesen der Stufe), `bildKeinTc` (der Stufe, die
  `image` lieferte), `himmelKeinTc` (beim Lesen von skyParms) statt `noTC` am Ende des Shaders (ParseStage :1263/1288/1329,
  ParseSkyParms :1970, gesetzt :2324). Die Vorschau nimmt den Wert jetzt auch bei einem Treffer im Bildspeicher (`texCache`, vorher
  fehlte keinTc dort ganz), fuer Zusatzstufen, Bildfolgen der Grundstufe und Wolkenschichten (vorher immer komprimiert).
  - ctest bilder: rot `m7_rot_ctest_bilder` 3 Fehlschlaege (noTC nach der Stufe, zwischen zwei Stufen, nach skyParms), gruen
    `m7_gr_ctest_bilder` 1/1.
- **P41** CJK-Umbruch-Speicher: Schluessel mit der Nummer der Schrift im Atlas statt ihrer Adresse, `buildFonts` leert den Speicher
  (`cjkSchriftenNeu`). Selbsttest cjkweit (ja): 12 Mal umbrechen, Schriften neu bauen, wieder umbrechen; gezaehlt: Ergebnisse des
  Testtexts, die direkt nach dem Neubau noch gemerkt sind, und Runden ohne neuen Umbruch.
  - rot `m7_rot2_cjk_ja`: nach den Neubauten noch 61 Ergebnisse der alten Schriften gemerkt; einmal lag die neue Schrift an derselben
    Adresse (im ersten Rotlauf `m7_rot_cjk_ja` nie - Windows verteilt kleine Bloecke zufaellig, deshalb die Zaehlung der gemerkten).
  - gruen `m7_gr_cjk_ja`: 0 gemerkt, 0 Runden ohne Umbruch; cjkweit 19/0.
- **P42** Selbsttest geschuetze (Vergleich): das Regelwerk vor dem Vergleich wird gemerkt und am Ende zurueckgestellt, neue Pruefung.
  rot `m7_rot_sour` "Regelwerk am Ende 2, vorher 1", gruen `m7_gr_sour` und `m7_gr3_sour` "1, vorher 1".

| Lauf | Szene 1 vor dem Wechsel | Szene 1 nach dem Wechsel auf Movie Duels |
|---|---|---|
| `m7_rot3_sour` (ohne P39) | 259 Bilder, 259 DXT5 | 0 Bilder neu |
| `m7_gr3_sour` | 259 Bilder, 259 DXT5 | 260 Bilder neu, davon DXT5 0 (32 mit $lightmap-Stufe) |

## MR7 Schluss

- Commits je Punkt: P39 b6328c5a4, P43 4e81224e2, P40 6fa53e6aa, P41 26a7a9876, P42 93f4bb4bb (P43 vor P40, weil P40 noTC je Stufe
  aus P43 nimmt - so baut jeder Stand).
- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) 0 Warnungen; ctest 237/237 mit -j 8 ohne `q3map2_pr316`,
  `q3map2_pr316` allein 1/1.
- Selbsttests nach dem Neubau: geschuetze (BHED_GV_DXT=1, t1_sour) 4/0 `m7_ende_sour`, cjkweit ja 19/0 `m7_ende_cjk_ja`,
  karteneditor r3 22/0 `m7_ende_r3`, rd 29/0 `m7_ende_rd` (RD DXT wie `m7_gr_rd`), editorportal 19/0 `m7_ende_portal`, Modus 1
  2635/0 `m7_ende_modus1`.
- Lints wie MR6 (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung wie vorher, lint_cjkumbruch 0, sonst ok);
  Handbuch neu erzeugt (karte.html, suche.js in 6 Sprachen), `--pruefen` ok.

## MR7 Offen

- Keine Bildzeit gemessen (Auftrag): Echtzeit-Licht unter JKA-Regeln laedt je Bild eine zweite Fassung (DXT5 im Arbeitsfaden,
  Speicher doppelt fuer die Bilder, die beide Ansichten zeigen).
- Gegen das Spiel selbst nicht neu verglichen: die Stufen sind Byte fuer Byte die der Vorschau, deren DXT5 in MR4-MR6 gegen
  Original-JKA geprueft wurde.
- Das Spiel laedt ein Bild nur einmal je Name (R_FindImageFile_NoLoad: der erste Shader entscheidet allowTC); Vorschau und
  Echtzeit-Licht entscheiden je Shader bzw. Stufe. Ein Bild, das ein noTC-Shader und ein anderer Shader teilen, kann daher je nach
  Ladereihenfolge im Spiel anders aussehen - nicht nachgebaut.
- Die Schatten des Echtzeit-Lichts (Alpha-Masken, kameraTexturBild) nehmen ebenfalls die Spielfassung; die Vorlage (vorlageZeichnen)
  auch.
- `App::mapBytes` wird nirgends gefuellt (toter Eintrag, nicht von MR7).

# MR8 (08.11.2026, gleicher Worktree, Basis fcc7d3cbe, Laeufe `m8_*`, Skripte `Y:\werkzeuge\m4\m8\`)

## MR8 Punkt 1: ein Bild je Name wie im Spiel

- Quelle (OpenJK rd-vanilla, Movie Duels 9 gleich): `R_FindImageFile` fragt zuerst `R_FindImageFile_NoLoad` (tr_image.cpp:936-971,
  MD9 :920-965). Das sucht in `AllocatedImages` nach `GenerateImageMappingName` (klein, ohne Endung, `\` -> `/`) und gibt das
  vorhandene `image_t` zurueck - mit mipmap, allowPicmip, glWrapClampMode des ERSTEN (R_CreateImage :983-1053, glTexParameter WRAP
  einmal beim Anlegen) und dessen Upload32 (allowTC des Ersten, :629-689). Warnungen nur fuer mipmap/allowPicmip/glWrapClampMode
  ("WARNING: reused image %s with mixed ... parm", :955-962; MD9 nur mit `g_DebugSaberCombat`, :941-953), fuer allowTC keine.
- Werte je Aufruf: ParseStage map/clampMap/animMap mit `!shader.noMipMaps, !shader.noPicMip, !shader.noTC, GL_REPEAT|GL_CLAMP`
  (tr_shader.cpp:1259-1335), ParseSkyParms `qtrue, qtrue, !noTC, GL_CLAMP` (:1970), Shader ohne Skript `mipRawImage, mipRawImage,
  qtrue, mipRawImage ? GL_REPEAT : GL_CLAMP` (:3502). `nomipmaps` setzt noMipMaps und noPicMip, `nopicmip` nur noPicMip
  (:2301-2311) - beides wie noTC in der Reihenfolge des Skripts (vorher las JK Studio `nomipmaps` gar nicht).
- Ladereihenfolge im Spiel (SP): SV_SpawnServer -> Spielmodul spawnt die Entities und laeuft 4 Bilder (Ghoul2-Modelle
  registrieren ihre Flaechen-Shader beim Laden, tr_ghoul2.cpp:3751) -> CG_Init/CG_RegisterGraphics: CG_RegisterEffects (Effekte
  vor der Welt, cg_main.cpp:1328) -> `cgi_R_LoadWorldMap` (:1338): RE_LoadWorldMap R_LoadFogs je Nebel (tr_bsp.cpp:1088), dann
  R_LoadSurfaces je Flaeche der Datei (ShaderForShaderNum :389/477/547/622) -> HUD-Shader, Modelle (CS_MODELS), Gegenstaende.
- Gebaut: `bhed/bildregister.h` - spielt R_FindShader/ParseShader/R_FindImageFile nach (je Shadername einmal, Bilder in Stufen-
  reihenfolge, Himmelsseiten an der Stelle von skyParms) und merkt je Bild die Werte des Ersten samt Warnungen. Die Vorschau
  (loadMapTextures, auch Spielbild des Karten-Editors ueber loadMapTexturesFuer) fuellt es beim Kartenwechsel in der Reihenfolge
  von RE_LoadWorldMap (`weltShaderReihenfolge`: Nebel, dann Flaechen), danach kommt jeder Shader beim ersten loadTextureFor dazu
  (Modelle, Effekte, Klingen). Jedes Bild (Grundstufe, Zusatzstufen, animMap je Bild, Himmel, Wolken) nimmt keinTc, Klammern und
  keinMip des Ersten (`wieErsterLaden`); ohne Mipmaps laedt ladeMitStufen nur Stufe 0 (Tex::keinMip). Das Echtzeit-Licht nimmt
  das Register des Spielbilds (Reihenfolge der .bsp), sonst registriert es jeden Shader beim ersten Zeichnen (`spielLaden`);
  noTC und nomipmaps des Ersten gelten fuer die Fassung des Spiels im Bildlager. Die Warnungen der Engine stehen im Protokoll
  ("Bild je Name: WARNING: reused image ...").
- ctest bilder (imagetest): Register beide Reihenfolgen, Shader ohne Skript, RegisterShaderNoMip, animMap nach nomipmaps,
  skyParms nach einer Stufe, Warnungstexte, Weltreihenfolge (Nebel vor Flaechen) - 10 neue Pruefungen gruen.
- Selbsttest geschuetze (t1_sour aus er1.pk3, OpenJK-Regeln, BHED_GV_DXT=1): die ersten zwei Shader mit Bild in Weltreihenfolge
  (Platz 14 textures/desert/stucco_portalsky1, Platz 26 textures/desert/stucco_ceiling_512_seamless) bekommen fuer die Probe ein
  Skript mit demselben Bild, je Fall noTC / nomipmaps / clampMap einmal beim ersten, einmal beim zweiten; von der Grafikkarte
  gelesen: Stufe 0, Stufenzahl, Klammern beider Plaetze.

| Fall | rot `m8_rot1_sour` (erster / zweiter) | gruen `m8_gr1_sour` |
|---|---|---|
| noTC beim ersten | DXT5 0 / 1, Bytes verschieden | beide unkomprimiert, Bytes gleich |
| noTC beim zweiten | DXT5 1 / 0, Bytes verschieden | beide DXT5, Bytes gleich |
| nomipmaps beim ersten | Stufen 10 / 10 (nomipmaps nicht gelesen) | Stufen 1 / 1, Warnung mixed mipmap + allowPicmip |
| nomipmaps beim zweiten | 10 / 10 (gruen) | 10 / 10, dieselben Warnungen |
| clampMap beim ersten | Klammern 1 / 0 | 1 / 1, Warnung mixed glWrapClampMode |
| clampMap beim zweiten | Klammern 0 / 1 | 0 / 0, dieselbe Warnung |

  geschuetze rot 5 OK / 5 FEHLER, gruen 10/0. t1_sour selbst laedt kein Bild mit gemischten Werten (keine Warnung beim Laden).
- Selbsttest karteneditor rd (Echtzeit-Licht, OpenJK-Regeln): `textures/rd/m8tc` (noTC) und `textures/rd/m8normal` mit demselben
  Bild (64 x 64, Verlauf mit Rauschen), beide Reihenfolgen: rot `m8_rot1_rd` "erster DXT5 0, zweiter DXT5 1, gleich 0" bzw.
  "1, 0, 0" (29/2); gruen `m8_gr1_rd` "0, 0, 1" bzw. "1, 1, 1" (31/0).
- Original-JKA-Shader (assets0-3.pk3) durchsucht (`notc_suche.py`): kein Bild, das ein noTC-Shader und ein Shader ohne noTC per
  Stufe teilen - der Fall entsteht in Mods und eigenen Karten. Kein Spielfoto (unter Movie Duels ist die Kompression aus,
  Original-JKA nur nach der Steam-Regel; die Regel ist in der Quelle eindeutig).

## MR8 Punkt 2: Speicher ohne Qualitaetsverlust

- Vorher: Vorschau (gpu-Szene 0), Spielbild des Karten-Editors (Szene 1) und Echtzeit-Licht (Bildlager, Fassung des Spiels seit
  MR7 P40) legten fuer dasselbe Bild je eine eigene Textur an, Byte fuer Byte gleich.
- Jetzt (gui/gpumap_win32.cpp `stufenGeteilt`): ladeMitStufen und kartegpu::bildStufenAnlegen geben die FERTIGEN Stufen (nach
  DXT5-Rundlauf bzw. Pyramide) ab; gesucht wird nach zwei XXH64-Werten (zwei Startwerte, wie die Figurenbilder seit rc603) ueber
  Groesse, Stufenzahl und alle Bytes. Gleiche Bytes -> dieselbe Textur (eigene Referenz je Nutzer, Freigabe wie bisher mit
  Release); die Liste haelt eine Referenz und raeumt Bilder ohne Nutzer alle 64 neuen Bilder, beim Kartenwechsel und bei der
  Abfrage auf, bei neuem Grafikgeraet und beim Herunterfahren gibt sie alles frei. Format (RGBA8, unveraenderlich) und Stufen
  bleiben, am Bild aendert sich nichts.
- Selbsttest geschuetze (t1_sour, Vorschau und Spielbild mit derselben Karte, P39-Schritt), neue Pruefung "MR8 P2":

| Lauf | Szene 0 / Szene 1 | gemeinsame Texturen | Bytes beider Szenen / belegt | DXGI belegt |
|---|---|---|---|---|
| rot `m8_rot2_sour` | 109 / 258 | 0 | 525,8 / 525,8 MB | 799,1 MB |
| gruen `m8_gr2_sour` | 109 / 258 | 125 | 525,8 / 256,5 MB | 522,7 MB |

  Liste im gruenen Lauf: 1032 Mal geteilt (1189,8 MB nicht noch einmal belegt, ueber alle Kartenladungen des Laufs), 301 angelegt,
  am Ende 187 Bilder mit 270,1 MB. Kartenansicht 1920 x 1440 (`er1_sour_4000.bmp`) rot gegen gruen gleich; abweichend nur 206
  Punkte unten rechts ausserhalb der Ansicht (Statuszeile).
- Selbsttest karteneditor rd (Echtzeit-Licht und Spielbild): "MR8 P2 - Echtzeit-Licht und Spielbild teilen textures/rd/streifen":
  rot `m8_rot2_rd` "dieselbe Textur 0" (31/1), gruen `m8_gr2_rd` "1" (32/0).
- Keine Bildzeit gemessen (Auftrag). Die Pruefwerte kosten je Bild einmal zwei XXH64 ueber die Stufen (beim Hochladen).

## MR8 Punkt 3: App::mapBytes

- Seit dem ersten Stand (rc568, d0ed723c1) nie gefuellt und nie gelesen; der Netzbau bei anderer Feinheit rechnet aus `App::geo`
  (app_view3d.cpp, buildMesh/buildModelMesh), nicht aus den Bytes. Der einzige Leser war der erste P39-Test in MR7 (dort schon
  berichtigt, .bsp aus BHED_GV_PK3). Entfernt; der saubere Neubau zeigt, dass ihn nichts mehr nennt.

## MR8 Schluss

- Commits: Punkt 1 751a42842, Punkt 2 1e3d98525, Punkt 3 (mapBytes) 2fbf01613, dazu `<cstddef>` in src/bildregister.cpp
  (lint_portability 100 -> 99) und diese Notizen.
- Sauberer Neubau (`--clean-first`, `--parallel 8 -- /nodeReuse:false`) rc 0, 0 Warnungen; ctest 237/237 (-j 8 ohne
  `q3map2_pr316`), `q3map2_pr316` allein 1/1.
- Selbsttests nach dem Neubau: geschuetze (BHED_GV_DXT=1, t1_sour) 11/0 `m8_ende_sour`, karteneditor rd 32/0 `m8_ende_rd`, r3 22/0
  `m8_ende_r3`, editorportal 19/0 `m8_ende_portal`, menueeditor en 2787/0 `m8_ende_me_en`, figurlicht 8/0 `m8_ende_figurlicht`,
  Modus 1 2635/0 `m8_ende_modus1`.
- Lints wie MR7 (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung wie vorher, lint_cjkumbruch 0, sonst ok).
  Handbuch: Hinweis auf der Seite "Die Karte" um "ein Bild je Name" ergaenzt (tools/handbuch/inhalt_mr4.py, 6 Sprachen),
  `gen_handbuch.py` und `--pruefen` ok.
- Keine Bildzeit gemessen (Auftrag).

## MR8 Offen

- Registrierungen VOR der Welt sind nicht vorangestellt: im Spiel laden die Ghoul2-Modelle der beim Start gespawnten Entities
  (Server, 4 Bilder) und die Effekte (CG_RegisterEffects) ihre Bilder vor RE_LoadWorldMap; im Programm kommen Modelle und Effekte
  nach der Welt ins Register (beim ersten loadTextureFor). Teilt ein Effekt- oder Figurenbild seinen Namen mit einem Weltbild und
  hat andere Werte, entscheidet im Spiel der Effekt bzw. die Figur. Ebenso nicht nachgebaut: Bilder, die aus der vorigen Karte
  oder den Menues stehen bleiben (AllocatedImages ueberlebt den Kartenwechsel, R_ImageList_LevelLoadEnd raeumt nur Ungenutztes).
- Fehlt das Bild einer Stufe, bricht ParseShader im Spiel dort ab (die Stufen danach laden nichts); das Register nimmt alle
  Stufen.
- `nopicmip` / allowPicmip des Ersten wird gemerkt und gewarnt, aber die Vorschau wendet r_picmip (Einstellung, Vorgabe 0) weiter
  auf alle Bilder an.
- Echtzeit-Licht: Klammern des Ersten nicht (der Editor-Abtaster wiederholt immer); unter Movie Duels zeigt es weiter die
  Editor-Fassung der Bilder. Ohne gebautes Spielbild gibt es keine Reihenfolge der .bsp - dann entscheidet der zuerst gezeichnete
  Shader.
- Kein Spielfoto zu Punkt 1 (Movie Duels komprimiert nicht, Original-JKA nur nach der Steam-Regel).

# MR9 (08.11.2026, gleicher Worktree, Basis bcf7f31de, Laeufe `m9_*`, Skripte `Y:\werkzeuge\m4\m9\`)

Auftrag nach 3 von 5 Punkten plus P58 abgebrochen (shank: nur noch ein Agent). Fertig: Punkt 1, 2, 3 und P58. Offen siehe unten.

## MR9 Punkt 1: Registrierreihenfolge vor der Welt

- Quelle (OpenJK SP, MD9 und codeJK2 gleich gebaut): SV_SpawnServer (server/sv_init.cpp:311-350, MD9 :321-364) ruft
  G_SpawnEntitiesFromString (level.time 1000) und fuenf ge->RunFrame (1000-1400); G2API_InitGhoul2Model laedt sofort
  (G2_TestModelPointers -> RE_RegisterModel, G2_API.cpp:771-825), R_LoadMDXM registriert je Flaeche den Shader
  (tr_ghoul2.cpp:3751), RE_RegisterModel .md3 die Stufen rueckwaerts (Name_2, Name_1, Name; tr_model.cpp:630-700), auch aus
  dem Zwischenspeicher (:217-232). Danach CG_Init -> CG_RegisterEffects (cg_main.cpp:1327-1329, MD9 :2270, codeJK2 :1209) in
  CS_EFFECTS-Reihenfolge, erst dann cgi_R_LoadWorldMap.
- Gebaut: `BildRegister::modell/haut/effekt` (RE_RegisterModel, RE_RegisterSkin mit drei Teilen und CommaParse,
  RegisterEffect mit Eigenschaften in Dateireihenfolge und Untereffekten sofort) und `vorWeltRegistrieren` (app_ablage.inc) vor
  der Welt: (1) Spawnfunktionen in Entity-Reihenfolge - misc_turret/misc_turretG2/misc_ns_turret/emplaced_gun/emplaced_eweb/
  misc_sentry_turret/misc_model_ghoul/misc_trip_mine (`serverG2Modell`, dieselben Pfade wie geschuetzeAufstellen), fx_runner
  und finish_spawning_turret/SP_PAS als G_EffectIndex; (2) RunFrame 1300 FinishSpawningItem (alle Gegenstaende ohne
  targetname, auch ITMSF_INVISIBLE, `gegenstandWeltModell`); (3) RunFrame 1400 NPC_Spawn_Go der Spawner ohne targetname ->
  G_SetG2PlayerModel (Haut, dann model.glm, sonst stormtrooper); (4) CG_RegisterEffects. NPC-/Klingendaten werden dafuer frueher
  gelesen (`npcDatenSicherstellen`, aus loadActorModels herausgezogen). Spielbild des Karten-Editors mit seinen Entities.
- ctest bilder (imagetest): md3-Stufen rueckwaerts, alle Shader einer Flaeche, glm-Hierarchie mit [nomaterial], Haut in drei
  Teilen (Kopf vor Rumpf, ohne tag_/_off,*off, fehlender Kopf laedt nichts weiter), Effekt mit Untereffekt/Modell/unbekannter
  Gruppe - 7 neue Pruefungen.
- Selbsttest geschuetze (t1_sour, OpenJK, DXT): erster Shader des ersten Gegenstands (models/items/medpac) und erster
  Weltshader (textures/desert/stucco_portalsky1) bekommen dasselbe Bild, einer mit noTC. Rot `m9_rot1_sour`: "Welt DXT5 1,
  zuerst geladen von textures/desert/stucco_portalsky1" bzw. "DXT5 0" - FEHLER/FEHLER. Gruen `m9_gr1_sour`: siehe Schluss.

## MR9 Punkt 2: fehlendes Stufenbild

- ParseStage bricht ab, wenn R_FindImageFile ein Bild von map/clampMap/animMap nicht findet (tr_shader.cpp:1263-1268,
  1288-1293, 1329-1334); ParseShader scheitert, defaultShader (:3489-3493), ShaderForShaderNum gibt tr.defaultShader
  (tr_bsp.cpp:352). Register: `setzeVorhanden` (Datei in Archiv oder lose, Endungen wie textureCandidates, ohne Entpacken),
  danach keine weiteren Stufen, `fehlerhaft(shader)`; die Weltflaeche zeigt dann standardShaderTextur (Figuren hatten das schon
  seit rc604).
- Rot `m9_rot1_sour`: "512x512, Kasten 0" (FEHLER). Gruen siehe Schluss.

## MR9 Punkt 3: nopicmip

- Upload32 halbiert nur mit allowPicmip (tr_image.cpp:575-580); das Bild hat das allowPicmip des Ersten. `bildPicmipFuer(bild)`
  (Register: !picmip -> 0) fuer texCache-Schluessel und Verkleinerung (Vorrat, bildLesen, Karten- und Figurenwege).
- Rot `m9_rot1_sour` (r_picmip 1): "nopicmip 256 (Datei 512), mit picmip 256 (Datei 512)" (FEHLER). Gruen siehe Schluss.

## P58 (PRUEF8): noTC in Figuren- und Modellshadern

- figurStufenListe, Gluehstufen und bySurface (Datei, texCache, Skript) setzen keinTc der Stufe bzw. bildKeinTc des
  Hautshaders. Rot `m9_rot1_sour`: "Grundflaeche noTC 0, Stufen 0/2 ohne TC" (FEHLER).

## MR9 Schluss

- Rot `m9_rot1_sour` (neue Pruefungen, altes Verhalten): 11 OK / 5 FEHLER (P1 zweimal, P2, P3, P58). Gruen `m9_gr5_sour`:
  16 OK / 0 FEHLER - "noTC am Gegenstand - Welt DXT5 0, zuerst geladen von models/items/medpac", "noTC an der Welt - Welt DXT5 1,
  zuerst geladen von models/items/medpac", "P2 - 16x16, Kasten 1, Stufe danach registriert 0", "P3 - nopicmip 512 (Datei 512),
  mit picmip 256 (Datei 512)", "P58 - Grundflaeche noTC 1, ohne noTC 0; Stufen 2/2 ohne TC". t1_sour: vor der Welt 19
  Ghoul2-Modelle (Server) und 3 Effekte, 40 Bilder.
- Unterwegs: die Registrierung steht jetzt VOR dem Bildvorrat (P3: der Vorrat verkleinerte sonst mit r_picmip, bevor das
  allowPicmip des Ersten bekannt war - `m9_gr4_sour` zeigte "nopicmip 256"). Die Probe P3 nimmt das eigene Bild des ersten
  Weltshaders (der erste Entwurf nahm m8->bild, das P2 beiden Shadern gab - dann war 512/512 richtig).
- Bau `--parallel 8 -- /nodeReuse:false` rc 0, 0 Warnungen; ctest 237/237 (-j 16 ohne q3map2_pr316), bilder danach 1/1.
  Kein sauberer Neubau, kein Modus 1, keine weiteren Selbsttests (Auftrag verkuerzt).

## MR9 Offen

- Punkt 4 (Echtzeit-Licht) nicht eingebaut. Vorbereitet, nicht angewendet: `Y:\werkzeuge\m4\m9\patch_p4a.py` (+`patch_p4as.py`
  Test): eigener CLAMP-Abtaster `abtasterKlemm`, Zustand3::klammern aus SpielLaden (Klammern des Ersten). 4b (Reihenfolge ohne
  Spielbild) nicht begonnen. 4c = P57.
- P57/P55/P56 vorbereitet, nicht angewendet: `patch_p57.py` (+`patch_p57s.py` Test rd mit Gamma 2 unter Movie Duels): Fassung des
  Spiels im Bildlager unabhaengig von dxt (Pyramide wie Upload32, ohne Gamma/Basisstufe), Bild verschieben statt kopieren,
  DXT im Bildlager ohne gemeinsame Mannschaft (`dxt5Rundlauf(..., mitMannschaft)`), Bildlager-Faeden vor dem Ende von wWinMain
  anhalten (P56). Der doppelte Editor-Eintrag in shaderZiehen (P55) ist nicht angefasst.
- P59: das Register deckt "der erste R_FindImageFile entscheidet" fuer Karte, Echtzeit-Licht und dieselbe Datei zweimal im
  Shader ab (MR8, wieErsterLaden je Stufe). Figuren stehen nicht im Register; fuer dieselbe Datei zweimal im Figurenshader ist
  `patch_p59.py` vorbereitet (mit ctest und Selbsttest), nicht angewendet.
- texCache und JK2-Endungsfolge (Hinweis PRUEF8): in mr4 gibt es keine JK2-eigene Endungsfolge (textureCandidates fest jpg,
  png, tga, jpeg); gilt fuer den Zweig mit FIX6, beim Merge pruefen.
- Vor der Welt nicht nachgebaut: G_EffectIndex/G2 weiterer Spawnfunktionen (NPC_PrecacheByClassName, emplaced-Effekte,
  Fahrzeug-RegisterAssets), Waffen- und Klingenmodelle der NPC (NPC_Begin bei 1500, ob vor CG_Init ist unbelegt), Bilder aus
  Menues und voriger Karte. Freigegebene Entity-Plaetze (G_Spawn) koennen die Reihenfolge der Denkbilder aendern; genommen ist
  die Reihenfolge der Entity-Zeichenkette. Skyparms nach einer gescheiterten Stufe: im Spiel fehlt dann auch der Himmel; die
  Vorschau zeichnet Himmelsflaechen weiter. Modelle/Effekte mit gescheitertem Shader (Shader 0) in der Vorschau nur fuer
  Figuren. Echtzeit-Licht-Register ohne setzeVorhanden (alle Bilder gelten als da).
- Handbuch nicht geaendert (keine sichtbare Bedienung neu). Keine Bildzeit gemessen.

## Changelog

## English

- Interface: Chinese and Japanese text wraps between characters in every window and tab (Mission, map editor, menu editor, helper, tutorials, timeline and more), no longer only after the first comma.
- Preview under Jedi Academy and Jedi Outcast: textures are compressed like in the game (S3TC), so colours match the original exactly; Movie Duels stays uncompressed like the game.
- Menu editor: the menu flow and the tab order no longer show the menus of a document that was closed before.
- Preview under Jedi Academy and Jedi Outcast: the texture compression is about ten times faster, with exactly the same picture.
- Menu editor: in a narrow right column the search line of the text editor wraps its switches (Aa, ab) instead of pushing them out of view.
- Menu editor: with little space the left column closes again for every newly opened menu, also after all menus were closed.
- Chinese: "Duplicate" in right-click menus now reads 创建副本 and no longer looks like "Copy" (复制).
- Preview under Jedi Academy and Jedi Outcast: the pictures of shaders with a lightmap stage and the sky boxes are compressed like in the game too, and switching between Movie Duels and OpenJK rules uploads the pictures again.
- Japanese and Chinese: long texts are no longer wrapped again in every frame.
- Real-time light and built map in the map editor under Jedi Academy and Jedi Outcast: textures are compressed like in the preview and the game (S3TC), the same surface looks the same in both views.
- Preview and map editor: `noTC` takes effect only from the line where it stands, as in the game; extra stages, image sequences and cloud layers of a `noTC` shader stay uncompressed.
- Switching between Movie Duels and OpenJK rules also uploads the pictures of the built map in the map editor again.
- Japanese and Chinese: after changing the OpenGL font in the map editor, texts are wrapped with the new font.
- Preview, real-time light and built map under Jedi Academy and Jedi Outcast: a picture that several shaders use is loaded once, as in the game - the shader that loads it first decides compression, mipmaps and clamping for all (`noTC`, `nomipmaps`, `clampMap`); `nomipmaps` now takes effect.
- Preview and map editor: a picture that several views show with the same bytes lies only once on the graphics card (a map in the preview and as built map: 257 instead of 526 MB of textures), the picture stays exactly the same.
- Preview: models the game loads while the level starts (items, turrets, map figures) and effects decide a shared picture before the map itself, as in the game; a shader whose stage picture is missing shows the default box like the game; `nopicmip` pictures keep their size with texture detail (r_picmip); `noTC` also applies to figure and model shaders.

## Deutsch

- Oberfläche: Chinesischer und japanischer Text bricht in allen Fenstern und Tabs zwischen den Zeichen um (Mission, Karten-Editor, Menü-Editor, Helfer, Tutorials, Zeitleiste und mehr), nicht mehr erst nach dem ersten Komma.
- Vorschau unter Jedi Academy und Jedi Outcast: Texturen werden wie im Spiel komprimiert (S3TC), die Farben stimmen jetzt genau mit dem Original überein; Movie Duels bleibt wie das Spiel unkomprimiert.
- Menü-Editor: Menüfluss und Tab-Reihenfolge zeigen nicht mehr die Menüs eines vorher geschlossenen Dokuments.
- Vorschau unter Jedi Academy und Jedi Outcast: Die Texturkompression ist etwa zehnmal schneller, das Bild bleibt genau gleich.
- Menü-Editor: In einer schmalen rechten Spalte bricht die Suchzeile des Text-Editors ihre Schalter (Aa, ab) um, statt sie aus dem Bild zu schieben.
- Menü-Editor: Bei wenig Platz geht die linke Spalte für jedes neu geöffnete Menü wieder zu, auch nachdem alle Menüs geschlossen waren.
- Chinesisch: „Duplizieren“ heißt in Rechtsklickmenüs jetzt 创建副本 und sieht nicht mehr aus wie „Kopieren“ (复制).
- Vorschau unter Jedi Academy und Jedi Outcast: Auch die Bilder von Shadern mit Lightmap-Stufe und die Himmelsboxen werden wie im Spiel komprimiert, und der Wechsel zwischen den Regeln von Movie Duels und OpenJK lädt die Bilder neu.
- Japanisch und Chinesisch: Lange Texte werden nicht mehr in jedem Bild neu umbrochen.
- Echtzeit-Licht und gebaute Karte im Karten-Editor unter Jedi Academy und Jedi Outcast: Texturen werden wie in der Vorschau und im Spiel komprimiert (S3TC), dieselbe Fläche sieht in beiden Ansichten gleich aus.
- Vorschau und Karten-Editor: `noTC` gilt wie im Spiel erst ab der Zeile, in der es steht; Zusatzstufen, Bildfolgen und Wolkenschichten eines `noTC`-Shaders bleiben unkomprimiert.
- Der Wechsel zwischen den Regeln von Movie Duels und OpenJK lädt auch die Bilder der gebauten Karte im Karten-Editor neu.
- Japanisch und Chinesisch: Nach dem Umstellen der OpenGL-Schrift im Karten-Editor werden Texte mit der neuen Schrift umbrochen.
- Vorschau, Echtzeit-Licht und gebaute Karte unter Jedi Academy und Jedi Outcast: ein Bild, das mehrere Shader benutzen, wird wie im Spiel nur einmal geladen - der Shader, der es zuerst lädt, entscheidet für alle über Kompression, Mip-Stufen und Klammern (`noTC`, `nomipmaps`, `clampMap`); `nomipmaps` wirkt jetzt.
- Vorschau und Karten-Editor: ein Bild, das mehrere Ansichten mit gleichen Bytes zeigen, liegt nur einmal auf der Grafikkarte (eine Karte in der Vorschau und als gebaute Karte: 257 statt 526 MB Texturen), das Bild bleibt genau gleich.
- Vorschau: Modelle, die das Spiel beim Levelstart lädt (Gegenstände, Geschütze, Kartenfiguren), und Effekte bestimmen ein gemeinsames Bild vor der Karte selbst, wie im Spiel; ein Shader, dessen Stufenbild fehlt, zeigt wie im Spiel das Standardkästchen; Bilder mit `nopicmip` behalten bei der Bildstufe (r_picmip) ihre Größe; `noTC` gilt auch für Figuren- und Modellshader.

## 中文

- 界面:中文和日文文本在所有窗口和标签页中按字符换行(任务、地图编辑器、菜单编辑器、助手、教程、时间轴等),不再只在第一个逗号后换行。
- Jedi Academy 和 Jedi Outcast 下的预览:纹理像游戏中一样压缩(S3TC),颜色与原版完全一致;Movie Duels 与游戏一样保持不压缩。
- 菜单编辑器:菜单流程和 Tab 顺序不再显示之前已关闭文档的菜单。
- Jedi Academy 和 Jedi Outcast 下的预览:纹理压缩快了约十倍,画面完全相同。
- 菜单编辑器:右侧栏较窄时,文本编辑器的搜索行会把开关(Aa、ab)换到下一行,而不是挤出视野。
- 菜单编辑器:空间不足时,每个新打开的菜单都会再次收起左侧栏,即使之前关闭了所有菜单。
- 中文:右键菜单中的“Duplicate”现在显示为“创建副本”,不再与“复制”(Copy)相同。
- Jedi Academy 和 Jedi Outcast 下的预览:带光照贴图阶段的着色器图片和天空盒也像游戏中一样压缩;在 Movie Duels 与 OpenJK 规则之间切换时会重新上传图片。
- 日文和中文:长文本不再在每一帧重新换行。
- Jedi Academy 和 Jedi Outcast 下地图编辑器的实时光照和已构建地图:纹理像预览和游戏中一样压缩(S3TC),同一表面在两个视图中看起来相同。
- 预览和地图编辑器:`noTC` 与游戏中一样从其所在的行开始生效;`noTC` 着色器的附加阶段、图像序列和云层保持不压缩。
- 在 Movie Duels 与 OpenJK 规则之间切换时,也会重新上传地图编辑器中已构建地图的图片。
- 日文和中文:在地图编辑器中更改 OpenGL 字体后,文本会按新字体换行。
- Jedi Academy 与 Jedi Outcast 规则下的预览、实时光照和已编译地图:多个着色器共用的图像与游戏中一样只加载一次——最先加载它的着色器为所有着色器决定压缩、mipmap 和边缘钳制(`noTC`、`nomipmaps`、`clampMap`);`nomipmaps` 现在生效。
- 预览与地图编辑器:多个视图显示的字节相同的图像在显卡上只保存一份(同一张地图在预览中和作为已编译地图:纹理 257 MB 而非 526 MB),图像完全不变。
- 预览：与游戏一致，关卡开始时游戏加载的模型（物品、炮塔、地图角色）和特效会先于地图本身决定共享图片；缺少阶段图片的着色器与游戏一样显示默认方框；带 `nopicmip` 的图片在纹理细节 (r_picmip) 下保持原尺寸；`noTC` 也适用于角色和模型着色器。

## 日本語

- インターフェース:中国語と日本語のテキストは、すべてのウィンドウとタブ(ミッション、マップエディター、メニューエディター、ヘルパー、チュートリアル、タイムラインなど)で文字の間で折り返されます。最初の読点の後だけで折り返すことはなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:テクスチャをゲームと同じように圧縮(S3TC)するため、色がオリジナルと正確に一致します。Movie Duels はゲームと同じく非圧縮のままです。
- メニューエディター:メニューフローとタブ順序に、前に閉じたドキュメントのメニューが表示されなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:テクスチャ圧縮が約 10 倍速くなり、画面はまったく同じです。
- メニューエディター:右の列が狭いとき、テキストエディターの検索行はスイッチ(Aa、ab)を見えない所へ押し出さず、次の行に折り返します。
- メニューエディター:スペースが少ないとき、すべてのメニューを閉じた後でも、新しく開いたメニューごとに左の列が再び閉じます。
- 中国語:右クリックメニューの「Duplicate」は「创建副本」と表示され、「Copy」(复制)と同じに見えなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:ライトマップステージを持つシェーダーの画像とスカイボックスもゲームと同じように圧縮され、Movie Duels と OpenJK のルールを切り替えると画像を読み込み直します。
- 日本語と中国語:長いテキストを毎フレーム折り返し直すことはなくなりました。
- Jedi Academy と Jedi Outcast でのマップエディターのリアルタイムライトとビルド済みマップ:テクスチャをプレビューやゲームと同じように圧縮(S3TC)するため、同じ面がどちらのビューでも同じに見えます。
- プレビューとマップエディター:`noTC` はゲームと同じく書かれた行から有効になります。`noTC` シェーダーの追加ステージ、画像シーケンス、雲のレイヤーは非圧縮のままです。
- Movie Duels と OpenJK のルールを切り替えると、マップエディターのビルド済みマップの画像も読み込み直します。
- 日本語と中国語:マップエディターで OpenGL フォントを変更すると、テキストは新しいフォントで折り返されます。
- Jedi Academy と Jedi Outcast のルールでのプレビュー、リアルタイムライト、ビルド済みマップ:複数のシェーダーが使う画像はゲームと同じく一度だけ読み込まれ、最初に読み込んだシェーダーが圧縮、ミップマップ、クランプをすべてについて決めます(`noTC`、`nomipmaps`、`clampMap`)。`nomipmaps` が有効になりました。
- プレビューとマップエディター:複数のビューが同じバイトで表示する画像はグラフィックカード上に一つだけ置かれます(プレビューとビルド済みマップの同じマップ:テクスチャ 526 MB ではなく 257 MB)。画像はまったく同じです。
- プレビュー：ゲームと同じく、レベル開始時にゲームが読み込むモデル（アイテム、タレット、マップのキャラクター）とエフェクトが、マップ本体より先に共有画像を決めます。ステージ画像が欠けたシェーダーはゲーム同様デフォルトの枠を表示します。`nopicmip` の画像はテクスチャ詳細 (r_picmip) でもサイズを保ちます。`noTC` はキャラクターとモデルのシェーダーにも適用されます。

## Español

- Interfaz: el texto chino y japonés se corta entre caracteres en todas las ventanas y pestañas (Misión, editor de mapas, editor de menús, ayudante, tutoriales, línea de tiempo y más), ya no solo después de la primera coma.
- Vista previa con Jedi Academy y Jedi Outcast: las texturas se comprimen como en el juego (S3TC), así los colores coinciden exactamente con el original; Movie Duels sigue sin comprimir, como el juego.
- Editor de menús: el flujo de menús y el orden de tabulación ya no muestran los menús de un documento cerrado antes.
- Vista previa con Jedi Academy y Jedi Outcast: la compresión de texturas es unas diez veces más rápida, con exactamente la misma imagen.
- Editor de menús: con la columna derecha estrecha, la línea de búsqueda del editor de texto pasa sus interruptores (Aa, ab) a la línea siguiente en lugar de sacarlos de la vista.
- Editor de menús: con poco espacio, la columna izquierda vuelve a cerrarse para cada menú abierto, también después de cerrar todos los menús.
- Chino: «Duplicar» en los menús del botón derecho ahora es 创建副本 y ya no se ve igual que «Copiar» (复制).
- Vista previa con Jedi Academy y Jedi Outcast: las imágenes de shaders con etapa de lightmap y las cajas de cielo también se comprimen como en el juego, y al cambiar entre las reglas de Movie Duels y OpenJK las imágenes se vuelven a cargar.
- Japonés y chino: los textos largos ya no se vuelven a cortar en cada fotograma.
- Luz en tiempo real y mapa compilado del editor de mapas con Jedi Academy y Jedi Outcast: las texturas se comprimen como en la vista previa y en el juego (S3TC), la misma superficie se ve igual en ambas vistas.
- Vista previa y editor de mapas: `noTC` actúa, como en el juego, solo desde la línea donde está; las etapas adicionales, secuencias de imágenes y capas de nubes de un shader con `noTC` quedan sin comprimir.
- Al cambiar entre las reglas de Movie Duels y OpenJK también se vuelven a cargar las imágenes del mapa compilado en el editor de mapas.
- Japonés y chino: tras cambiar la fuente OpenGL en el editor de mapas, los textos se cortan con la nueva fuente.
- Vista previa, luz en tiempo real y mapa compilado con las reglas de Jedi Academy y Jedi Outcast: una imagen que usan varios shaders se carga una sola vez, como en el juego; el shader que la carga primero decide para todos la compresión, los mipmaps y el ajuste al borde (`noTC`, `nomipmaps`, `clampMap`); `nomipmaps` ahora tiene efecto.
- Vista previa y editor de mapas: una imagen que varias vistas muestran con los mismos bytes está una sola vez en la tarjeta gráfica (un mapa en la vista previa y como mapa compilado: 257 en lugar de 526 MB de texturas); la imagen queda exactamente igual.
- Vista previa: los modelos que el juego carga al iniciar el nivel (objetos, torretas, figuras del mapa) y los efectos deciden una imagen compartida antes que el propio mapa, como en el juego; un shader al que le falta la imagen de una etapa muestra la caja por defecto como el juego; las imágenes con `nopicmip` conservan su tamaño con el detalle de texturas (r_picmip); `noTC` también se aplica a los shaders de figuras y modelos.

## Čeština

- Rozhraní: čínský a japonský text se ve všech oknech a kartách (Mise, editor map, editor nabídek, pomocník, výukové lekce, časová osa a další) zalamuje mezi znaky, už ne až za první čárkou.
- Náhled pro Jedi Academy a Jedi Outcast: textury se komprimují jako ve hře (S3TC), takže barvy přesně odpovídají originálu; Movie Duels zůstává jako hra bez komprese.
- Editor nabídek: tok nabídek a pořadí tabulátoru už neukazují nabídky dříve zavřeného dokumentu.
- Náhled pro Jedi Academy a Jedi Outcast: komprese textur je zhruba desetkrát rychlejší, obraz zůstává přesně stejný.
- Editor nabídek: v úzkém pravém sloupci vyhledávací řádek textového editoru zalomí své přepínače (Aa, ab) na další řádek, místo aby je vysunul z obrazu.
- Editor nabídek: při nedostatku místa se levý sloupec znovu zavře pro každou nově otevřenou nabídku, i když byly předtím všechny nabídky zavřené.
- Čínština: „Duplikovat“ v nabídkách pravého tlačítka je nyní 创建副本 a už nevypadá stejně jako „Kopírovat“ (复制).
- Náhled pro Jedi Academy a Jedi Outcast: obrázky shaderů s fází lightmapy a nebeské boxy se také komprimují jako ve hře a přepnutí mezi pravidly Movie Duels a OpenJK nahraje obrázky znovu.
- Japonština a čínština: dlouhé texty se už nezalamují znovu v každém snímku.
- Světlo v reálném čase a sestavená mapa v editoru map pro Jedi Academy a Jedi Outcast: textury se komprimují jako v náhledu a ve hře (S3TC), stejná plocha vypadá v obou pohledech stejně.
- Náhled a editor map: `noTC` platí jako ve hře až od řádku, kde stojí; další fáze, sekvence obrázků a vrstvy mraků shaderu s `noTC` zůstávají nekomprimované.
- Přepnutí mezi pravidly Movie Duels a OpenJK nahraje znovu i obrázky sestavené mapy v editoru map.
- Japonština a čínština: po změně písma OpenGL v editoru map se texty zalamují novým písmem.
- Náhled, světlo v reálném čase a sestavená mapa podle pravidel Jedi Academy a Jedi Outcast: obrázek, který používá více shaderů, se jako ve hře načte jen jednou - shader, který ho načte první, rozhodne za všechny o kompresi, mipmapách a upnutí u okraje (`noTC`, `nomipmaps`, `clampMap`); `nomipmaps` nyní platí.
- Náhled a editor map: obrázek, který více pohledů zobrazuje se stejnými bajty, leží na grafické kartě jen jednou (mapa v náhledu a jako sestavená mapa: 257 místo 526 MB textur), obraz zůstává přesně stejný.
- Náhled: modely, které hra načte při startu úrovně (předměty, věže, postavy mapy), a efekty určují sdílený obrázek před samotnou mapou, jako ve hře; shader, jemuž chybí obrázek fáze, zobrazí jako hra výchozí rámeček; obrázky s `nopicmip` si při detailu textur (r_picmip) zachovají velikost; `noTC` platí i pro shadery postav a modelů.
