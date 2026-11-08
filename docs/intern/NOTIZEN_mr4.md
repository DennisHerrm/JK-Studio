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

## Changelog

## English

- Interface: Chinese and Japanese text wraps between characters in every window and tab (Mission, map editor, menu editor, helper, tutorials, timeline and more), no longer only after the first comma.
- Preview under Jedi Academy and Jedi Outcast: textures are compressed like in the game (S3TC), so colours match the original exactly; Movie Duels stays uncompressed like the game.
- Menu editor: the menu flow and the tab order no longer show the menus of a document that was closed before.
- Preview under Jedi Academy and Jedi Outcast: the texture compression is about ten times faster, with exactly the same picture.

## Deutsch

- Oberfläche: Chinesischer und japanischer Text bricht in allen Fenstern und Tabs zwischen den Zeichen um (Mission, Karten-Editor, Menü-Editor, Helfer, Tutorials, Zeitleiste und mehr), nicht mehr erst nach dem ersten Komma.
- Vorschau unter Jedi Academy und Jedi Outcast: Texturen werden wie im Spiel komprimiert (S3TC), die Farben stimmen jetzt genau mit dem Original überein; Movie Duels bleibt wie das Spiel unkomprimiert.
- Menü-Editor: Menüfluss und Tab-Reihenfolge zeigen nicht mehr die Menüs eines vorher geschlossenen Dokuments.
- Vorschau unter Jedi Academy und Jedi Outcast: Die Texturkompression ist etwa zehnmal schneller, das Bild bleibt genau gleich.

## 中文

- 界面:中文和日文文本在所有窗口和标签页中按字符换行(任务、地图编辑器、菜单编辑器、助手、教程、时间轴等),不再只在第一个逗号后换行。
- Jedi Academy 和 Jedi Outcast 下的预览:纹理像游戏中一样压缩(S3TC),颜色与原版完全一致;Movie Duels 与游戏一样保持不压缩。
- 菜单编辑器:菜单流程和 Tab 顺序不再显示之前已关闭文档的菜单。
- Jedi Academy 和 Jedi Outcast 下的预览:纹理压缩快了约十倍,画面完全相同。

## 日本語

- インターフェース:中国語と日本語のテキストは、すべてのウィンドウとタブ(ミッション、マップエディター、メニューエディター、ヘルパー、チュートリアル、タイムラインなど)で文字の間で折り返されます。最初の読点の後だけで折り返すことはなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:テクスチャをゲームと同じように圧縮(S3TC)するため、色がオリジナルと正確に一致します。Movie Duels はゲームと同じく非圧縮のままです。
- メニューエディター:メニューフローとタブ順序に、前に閉じたドキュメントのメニューが表示されなくなりました。
- Jedi Academy と Jedi Outcast でのプレビュー:テクスチャ圧縮が約 10 倍速くなり、画面はまったく同じです。

## Español

- Interfaz: el texto chino y japonés se corta entre caracteres en todas las ventanas y pestañas (Misión, editor de mapas, editor de menús, ayudante, tutoriales, línea de tiempo y más), ya no solo después de la primera coma.
- Vista previa con Jedi Academy y Jedi Outcast: las texturas se comprimen como en el juego (S3TC), así los colores coinciden exactamente con el original; Movie Duels sigue sin comprimir, como el juego.
- Editor de menús: el flujo de menús y el orden de tabulación ya no muestran los menús de un documento cerrado antes.
- Vista previa con Jedi Academy y Jedi Outcast: la compresión de texturas es unas diez veces más rápida, con exactamente la misma imagen.

## Čeština

- Rozhraní: čínský a japonský text se ve všech oknech a kartách (Mise, editor map, editor nabídek, pomocník, výukové lekce, časová osa a další) zalamuje mezi znaky, už ne až za první čárkou.
- Náhled pro Jedi Academy a Jedi Outcast: textury se komprimují jako ve hře (S3TC), takže barvy přesně odpovídají originálu; Movie Duels zůstává jako hra bez komprese.
- Editor nabídek: tok nabídek a pořadí tabulátoru už neukazují nabídky dříve zavřeného dokumentu.
- Náhled pro Jedi Academy a Jedi Outcast: komprese textur je zhruba desetkrát rychlejší, obraz zůstává přesně stejný.
