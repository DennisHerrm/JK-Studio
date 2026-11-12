# Notizen AP5 (12.11.): Model-Tab, Bereich Ansehen/Prüfen vollständig nach dem Entwurf - MBENTWURF Teil B, rc612

Paket AP5 aus `auftrag_mbentwurf_b_1011.md` (Opus 5.5, ein Agent): die Zeilen P1-P11, X18, X19 (ohne Kopieren), R7, R10 und Z20 der
Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md`. Maßstab: der freigegebene Entwurf (`entwurf_modelltab.html`,
`feldInhalt` „pruefen“, Rechtsklick-Menüs, `figurPruefen`), shanks Wunsch vom 04.11. („Alles, was ModView kann“ im Startbereich) und vom
10.11. (Anzeige-Schalter in die Kopfzeile, „es geht ja nicht nur um controls … sondern das aussehen“). Vorbild für Inhalt und Regeln:
ModView (Raven, JACoders/OpenJK-tools `ModView/`, GPL v2) und die Engine (OpenJK, Movie Duels Update 9). Worktree `Y:\p_ap5`, Zweig
`p_ap5` auf karte1 `43f6b6b21`. Selbsttest `gui/selbsttest_ap5.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap5 | ap5foto`;
Handbuch `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap5`), ctest `modell_szene`, Skripte und Läufe `Y:\werkzeuge\ap5\`.

## Was sich ändert

Neue Dateien: `gui/app_modelltab_pruefen.cpp` (Zustand `ModellPruefen`/`modellPr()` im eigenen Block am Ende von `gui/modelltab.h`),
`include/bhed/modell/szene.h` und `src/modell/szene.cpp` (ModView-Skript), `tests/modell_szene.cpp`, `tools/i18n_ap5.py` (Präfix `Pq`,
6 Sprachen), `tools/handbuch/inhalt_ap5.py`.

| Zeile | Was | Quelle / Regel | Datei |
|---|---|---|---|
| P1 Modell | Raster Andere GLA · Texturen neu laden (Alt+R jetzt in beiden Bedienmodi, auch in der Tastenliste wie Blender) · **Importieren …** (Wahl: von der Platte .glm/.md3 oder jedes Modell des Spiels aus den .pk3); Foto zieht nach „Boden und Kamera“ | Entwurf `wk("📥", "Importieren …")`, ModView „Refresh textures“ Alt+R | `rolloutModell` (`app_modelltab.cpp`), `blenderTasten` |
| P2 Detailstufe und Skin | LOD 1-10 alle wählbar; darüber zeigt die Ansicht die letzte Stufe, der Satz sagt „LOD 10 zeigt LOD 1, wie im Spiel“; „Das Modell hat n LODs. n Dreiecke.“; Skins als **zweispaltige Auswahl** mit dem Flächenzähler rechts in der Zeile „Skin“ (Z20) | G2_ComputeLOD: `lodBias >= numLods -> numLods - 1` (tr_ghoul2.cpp:868-872 MD9, OpenJK rd-vanilla gleich) | `rolloutLod`, `modellPrHaeute` |
| P3a Anzeige | die fünf fehlenden Schalter von ModView in den Menüs der Kopfzeile (wie 3ds Max [Standard] bzw. [+] > Knochen und Prüfen, wie Blender Viewport Shading bzw. Overlays), in „Alle Aktionen suchen“ und im Rechtsklick der Ansicht: **Alpha-Blending** (aus = alles deckend), **Texturfilter** (aus = nächster Bildpunkt), **Knochengewicht-Schwelle** (mit Zahl), **Fläche hervorheben (+ Knochenbezüge)**, **nicht schattenwerfende Flächen** | ModView model.cpp:3537 (glDisable(GL_BLEND)), textures.cpp:256 (GL_NEAREST), r_glm.cpp:91 (Gewicht nur, wenn `w*100 > Schwelle`, ohne Umverteilen), r_surface.cpp:618-700 (gelbe Linien, rote Striche glLineStipple(8,0xAAAA)), model.cpp:2847 (DrawTagOrigin je Knochenbezug mit „( Nummer )“). Schatten-Regel aus der Engine: Fläche mit `numVerts >= SHADER_MAX_VERTEXES/2` wirft ihren Stencil-Schatten nicht selbst, das Spiel nimmt dieselbe Fläche der letzten LOD (tr_ghoul2.cpp:2307 OpenJK, MD9 :2666/2690); SHADER_MAX_VERTEXES 1000 (OpenJK qfiles.h:33), 4000 (MD9) | `app_modelltab_kopf.cpp` (Katalog, je ein Eintrag), `gpumap.h`/`gpumap_win32.cpp` (`ModellAnsichtArt::opak`, `punktFilter`, nur im Modellfenster), `modellPrUeberlagerung`, `modellPrSchwelleAnwenden` |
| P4 Boden und Kamera | neue Gruppe: Boden −24 / tiefster Punkt, FOV 10° / 80° / 90°, Hintergrund (Haken + Farbe), Gewicht-Schwelle (Regler), Ansicht zurücksetzen, Foto (1920+) | Entwurf `roll("pkamera")`, ModView (Boden −24 DT85, FOV-Knöpfe, Clear colour, 5 %) | `modellPrRolloutBoden` |
| P5 Anbauten | unverändert (Form von AP3: Zeile mit ✕, Tipp-Kasten, Kurzinfo) - geprüft | - | - |
| P6 Prüfung für das Spiel | Balken der gewählten Fläche (ohne Auswahl: die der Grenze nächste) für Ecken, Knochen, Gewichte; Zeilen Flächen im Modell, Sequenzen innerhalb der GLA, vom Spielcode gesuchte Tags (n / m mit fehlenden im Tooltip), Shader ohne Bild, LODs, ohne eigenen Schatten; die Grenzen des Zielspiels und die Befunde bleiben darunter | Tags: `modell::gesuchteTags` - SP G_SetG2PlayerModelInfo nur für ein Standard-Skelett (OpenJK g_client.cpp:895-920/1171-1192, MD9 :1269-1351/1681-1703, JK2 nach Modellname codeJK2 g_client.cpp:806-845/904-925), MP CG_RegisterClientModelname (cg_players.c:628-655, MD9 :863-906 mit Holstern) | `modellPrPruefZeilen`, `include/bhed/modell/g2.h` |
| P7 Szene | neue Gruppe: **Szene speichern** (als ModView-Skript .mvs: Modell, Haut, Anbauten an Tags/Knochen, Primär-/Sekundärsequenz samt Sekundär-Start, vom Anwender geschaltete Flächen, Wurzel; dazu `jks_lod`, `jks_fov`, `jks_floor(z)`, die ModView übergeht), **ModView-Skript lesen** (meldet, was es nicht übernimmt: Anbau an Anbau, multiseq, ethnic, Sequenzen eines Anbaus), NPC-Browser | ModView script.cpp (Script_Write/Read, Parse_*), GenericParser2 (`bhed/efx/gp2.h`) | `bhed/modell/szene.h`, `modellSzeneSpeichern/Lesen` |
| P8 | Werkzeugleiste wie Blender (Messen, Fläche/Knochen wählen, Foto) - von AP4, geprüft | - | - |
| P9 | Tasten wie 3ds Max Q, F3, Alt+W, /, „,“ „.“ vorhanden - geprüft; Alt+X (Durchsicht) gehört zu R14/AP13 | - | - |
| P10 | wie Blender **Z** = Schattierungs-Auswahl (Drahtgitter/Solid/Material), **Umschalt+Alt+Z** = Overlays der Ansicht an/aus (Raster, Überlagerung, Hinweis; Kopfzeile und Gizmo bleiben); Alt+Z (Röntgen) gehört zu R14/AP13 | blender_keys.txt: `wm.call_menu_pie Z name=VIEW3D_MT_shading_pie`, `wm.context_toggle Umschalt+Alt+Z space_data.overlay.show_overlays` | `blenderTasten`, `modellPrSchritt` |
| P11 | eine .glm öffnet immer in Ansehen/Prüfen | Entwurf MODI.pruefen „Startbereich beim Öffnen einer .glm“ | `modellDokumentLaden` |
| X18 | Gruppe **Auswahl**: die Info der gewählten Fläche, des Knochens oder Tags (Ecken, Dreiecke, Knochenbezüge, Shader, Eltern, Status bzw. Nummer, Eltern, Kinder …) als Tabelle; Info im Rechtsklick und Tooltip der Zeile hatte AP2 schon | Entwurf `infoText` | `modellPrRolloutAuswahl`, `modellExplorerInfo` (explorer, eine Funktion) |
| X19 | Rechtsklick der Fläche: Info, an / aus / aus + ohne Kinder, Als Wurzel setzen, Wurzel lösen, alle _off aus/an, Ursprungszustand - von AP2, geprüft; Kopieren/Einfügen gehört AP11 | - | - |
| R7 | wie 3ds Max ab Werk **vier Ansichten** in Modellieren, Weights, Painting, Animation; Alt+W merkt sich die Wahl je Bereich; wie Blender eine große Ansicht | Entwurf | `modellPrSchritt` |
| R10 | unten rechts in der Ansicht von Ansehen/Prüfen „LOD 1 · 5138 Dreiecke · wie im Spiel gezeichnet (Ghoul2 aus dem Mission-Tab)“ (die gezeichneten Dreiecke); in einer schmalen Ansicht nur „LOD · Dreiecke“ | Entwurf | `modellPrUeberlagerung` |
| Z20 | Skins zweispaltig in „Detailstufe und Skin“ (AP1 hatte die Klappliste schon dorthin gezogen); „Flächenliste ins Detailprotokoll“ in der Suche | - | `modellPrHaeute`, `modellPrAktionenSuche` |
| P13 (Text) | deutsch „Prüfung für das Spiel“ statt „Prüfen fürs Spiel“ | Entwurf | `tools/i18n_mb1a1.py` |

Finden (shank 06.11.): jede neue Gruppe und jeder Schalter steht an der Stelle des Entwurfs bzw. der Kopfzeile, in „Alle Aktionen suchen“
(Flächenliste, Boden und Kamera, Auswahl, Szene, Importieren, die fünf Schalter; wie Blender dazu Overlays und Schattierung mit Taste), im
Rechtsklick der Ansicht (dieselben Menüs wie die Kopfzeile) und mit Tooltip.

Das Foto mit Überlagerung (Foto 1920+, Selbsttests) zeichnet jetzt mit dem Stand der Schalter (vorher galt die Knochengewicht-Schwelle erst
ab dem nächsten Bild) und mit den Prüfanzeigen von AP5; „Overlays aus“ gilt auch dort.

## Funde (rot nachgestellt, behoben, grün)

1. **Alt+R fiel im Feld wie Blender vom Knopf** (Nachbarlauf ap3 F5 wie Blender, Knopf 299 Punkte breit, Tasten 0): `tasteHalten` (AP7) -
   wie im Entwurf bleibt die Taste, der Text bricht um; ap3 105/0, ap5 P1 wie Blender grün.
2. **Der einmalige Hinweis „Anzeige steht jetzt in der Kopfzeile“ lag unter dem Rand** (kopf n1: „Got it nicht gefunden“, MBKOPF 5 rot):
   steht jetzt vor den Gruppen; kopf 116/0.
3. **Szene speichern schrieb die Flächen der Haut als Anwender-Schalter** (ap5g1: `surfaces_off` belt, belt_gun statt head) und **beim Lesen
   löschte die Haut die gelesenen Schalter wieder** (G2API_SetSurfaceOnOffFromSkin leert die Überschreibungen erst im nächsten Bild):
   gespeichert wird nur, was von der Vorgabe der Haut abweicht (ModView „different from default“); beim Lesen gilt die Haut sofort, dann
   die Flächen (Reihenfolge Script_Parse_ReadOptionalArgs); P7 grün.
4. **Foto mit Überlagerung ohne Schwelle/Prüfanzeigen** (kopf n1: MBKOPF 4a „ohne Wirkung“ für Schwelle, Hervorheben, Schatten): das
   Foto baut das gezeichnete Modell vorher auf und zeichnet die Prüfanzeigen; Hervorheben zählt wie die Auswahlklammer als „braucht
   Auswahl“; „nicht schattenwerfende Flächen“ wirkt nur, wenn eine Fläche die Grenze erreicht (Katalog `wirkt`).
5. **Fotos: „…“ allein in einer Zeile** („Read ModView script / …“) - Texte ohne Leerzeichen vor den Punkten; **schwarzes Farbfeld ohne
   Haken** - zeigt die Farbe, die der Haken setzt; **Hinweis R10 im kleinen Fenster zweizeilig über der Figur** - nur „LOD · Dreiecke“,
   wenn der Satz nicht in eine Zeile passt (Foto klein); **Namen der Knochenbezüge auf den gelben Linien kaum lesbar** - dunkel hinterlegt
   (Foto en, Handbuch ja).
6. Lint i18n: `MbLodJetzt` unbenutzt (der Satz des Entwurfs ersetzt „Gezeichnet: LOD x von y“) - entfernt; Lint d3dbesitz:
   `g_samplerPunkt` in `shutdown()` freigegeben; Lint portability: `<cstddef>` im ctest.

## Selbsttest (rot -> grün)

- Rot gegen die alte Oberfläche (dieselben Texte, Kopfdateien und der Test; `app_modelltab*.cpp`, `gpumap_win32.cpp` alt, AP5-Funktionen leer;
  Lauf `ap5rot`): **17 OK, 114 FEHLER** (`Y:\werkzeuge\ap5\rot1_zeilen.txt`). Grün waren nur, was es schon gab: X19 (AP2) 6, P8 (AP4), P9,
  Alt+R wie 3ds Max, R7 für Ansehen/UV (eine Ansicht) und wie Blender, „ohne Haken wieder der Verlauf“ (2), „das x löst den Anbau“, R10
  „in den anderen Bereichen kein Hinweis“.
- Danach wurde der Test nur in der Mechanik verfeinert (vor jedem Klick ins Feld rollen, Zeilen nur innerhalb ihrer Gruppe suchen - „Ecken“
  heißt in „Modell“ gleich, Suche je Eintrag mit getipptem Text, Schatten an tusken mit einer Fläche von 615 Ecken, Kamera des Einpassens
  nach einem Bild festhalten, die Haut des Ladens zurück); keine Prüfung abgeschwächt.
- Grün: Läufe `ap5g1` 71/27 -> `g3` 106/14 -> `g4` 118/2 -> `g7` **120/0** -> Schluss `ap5end` (siehe Schlussrunde).
- Je Zeile: Ort (Gruppe im rechten Feld in der Reihenfolge des Entwurfs, Menü der Kopfzeile, unten rechts in der Ansicht), Element (Knöpfe,
  Zeilen, Werte gegen eigene Rechnung: Dreiecke, Tags, Sequenzen, Flächen ohne Schatten, Knochenbezüge, Ecken unter der Schwelle) und
  Bedienung (Klick, Taste Z / Umschalt+Alt+Z / Alt+W, Suche, Rechtsklick) mit ihrer Wirkung (Bildunterschied für Filter und Alpha am Foto der
  Ansicht und Neuzeichnen der Ansicht; Szene speichern -> lesen gibt denselben Stand; ein ModView-Skript von Hand).
- ctest `modell_szene` (23 Prüfungen): ModView-Skript, Fehlermeldungen wie Script_Read, nicht Abgebildetes, Kreis, Kennung wie
  GenerateHandleName - grün (vorher rot mit Eingaben in einer Zeile: GenericParser2 erwartet Zeilenumbrüche, wie ModView sie schreibt).

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

- `gui/selbsttest_ap3.inc` (Muster der Gruppen von Ansehen/Prüfen): die Gruppen des Entwurfs (Auswahl, Boden und Kamera, Szene dazu), in
  „Modell“ Importieren statt Foto, Alt+R in beiden Modi (Tasten 1), Szene im Reiter Dienstprogramme; vor dem Klick auf „Detailstufe und
  Skin“ ins Bild rollen (wie Blender steht sie mit der Gruppe Auswahl tiefer).
- `gui/selbsttest_hilfe2.inc` (Tour k-skins): die Häute sind eine zweispaltige Auswahl im Feld statt einer Klappliste - die andere Haut
  direkt anklicken.
- `gui/selbsttest_handbuch.inc` (Bild m_flaechen): der Flächenzähler steht rechts in der Zeile „Skin“.
- `gui/selbsttest_mbkopf.inc` (4a „jeder Schalter ändert das Foto“): die Knochengewicht-Schwelle verschiebt bei 5 % (Vorgabe von ModView)
  die Ecken von Kyle um Bruchteile eines Bildpunkts (0-2 Bildpunkte) - ihre Wirkung prüft ap5 bei 0.25 gegen eine eigene Zählung (1668 Ecken).

## Fotos

`Y:\werkzeuge\ap5\fotos\ap5_f_<variante>\` (Selbsttest `ap5foto`, 3818 × 2104 bei Oberfläche 1,5): en, de, zh-Hans, ja, es, cs, hell,
klein (1328 × 844 bei 1,0) - je Bedienmodus Ansehen/Prüfen ganz, das rechte Feld in Seiten, Prüfanzeigen (rote Striche, Hervorheben),
Modellieren (vier Ansichten wie 3ds Max), wie Blender die Schattierungs-Auswahl (Z). Entwurf neben Programm: `Y:\werkzeuge\ap5\vergleich\`.
Angesehen: en beide Modi ganz und alle Feldseiten, Prüfanzeigen und Modellieren wie 3ds Max (vier Ansichten), ja und de Feldseiten, klein
Ansehen/Prüfen wie 3ds Max (vor und nach Fund 5), hell Prüfanzeigen wie Blender, Handbuchbilder h_ap5_pruefen de und ja (Knochennamen vor
und nach dem Hinterlegen). Alle Texte in allen Sprachen ohne Abschneiden; CJK bricht richtig um.

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap5.py` (6 Sprachen): Seite „Model-Tab: Ansehen und Prüfen“ (Gruppen, Prüfung für das Spiel,
  Prüfanzeigen von ModView, Szene und Ansichten); wie AP4, AP6-AP10 nicht eingehängt - AP14 führt zusammen (Anleitung im Kopf der Datei).
- Bilder `h_ap5_max`, `h_ap5_blender`, `h_ap5_pruefen` in 6 Sprachen unter `handbuch/bilder/<sprache>/`.

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles`, `bau_end`, `bau_schluss`) |
| ctest | 258 / 258 (neu: `modell_szene`) |
| `ap5` | 120 / 0 (Schlusslauf `ap5schluss`; rot 17 / 114 gegen die alte Oberfläche) |
| `ap5foto` | en, de, zh-Hans, ja, es, cs, hell, klein - je 1 / 0 (en, hell, klein nach den Funden noch einmal) |
| Handbuchbilder | 6 Sprachen, je 1 / 0 (nach dem Hinterlegen der Knochennamen noch einmal) |
| Nachbargruppen (letzter Lauf je Gruppe) | ap1 137 · ap2 64 · ap3 105 · ap4 82 · ap6 94 · ap7 88 · ap8 104 · ap9 97 · ap10 70 · ap12 35 · kopf 116 · kopfrot 26 · nav 133 · raster 41 · blender 68 · mbv 216 · mb2 38 · mb3 22 · mb5 160 · mb6 38 · md 87 · pt 42 · uv 51 · funktionen 62 · tasten 44 - je 0 FEHLER; karteneditor hilfe2 (Tour k-skins) 56 / 0 (danach die bekannte Zeile „Beenden: Programm läuft nach No weiter … Exit Radiant“, ebenso in den Läufen von AP3 und AP4) |
| Lints (21 aus `lints.sh`, dazu efxflags, fremdprogramm, d3dbesitz, merkmalsbits) | wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm mit derselben Meldung; i18n, d3dbesitz ok (nach Fund 6) |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen (Rundläufe, uvmd3, mdkarte, pt-Varianten …),
Docks (R7 ändert nur die Zahl der Ansichten, nicht die Fensteranordnung), karteneditor voll, menueeditor, der volle Handbuch-Bildlauf (darin
das angepasste Bild m_flaechen), sauberer Neubau, Sprachläufe. Keine Bildzeit-Messung: die neuen Gruppen zeichnen nur in Ansehen/Prüfen, die
Prüfanzeigen nur, wenn ihr Schalter an ist; Alpha/Filter setzen je Bild einen Zustand mehr - ohne Messung keine Aussage zum Tempo.

## Offen / Fragen an shank

1. **Strg+S** speichert im Model-Tab weiter die .glm (Datei-Menü, seit Modell-Block 2). Der Entwurf zeigt „Szene speichern Strg+S“ - beides
   auf einer Taste geht nicht. So lassen (Szene speichern ohne Taste, per Knopf/Suche) oder Strg+S in Ansehen/Prüfen für die Szene?
2. **Importieren** öffnet .glm und .md3 (Platte, .pk3). Modelle aus XSI, FBX, OBJ, glTF liest das Programm noch nicht (der Import dieser Formate
   gilt Animationen) - eigene Arbeit (Stufe 1d/2).
3. **„In neuem Tab öffnen“** (Strg+O) gehört zu R5/AP11 (Modelle in Tabs); Strg+O öffnet heute wie bisher.
4. Gruppe „Auswahl“ steht nach „Modell“ (der Entwurf hat keine eigene Gruppe, nur „Info“ im Rechtsklick) - passt der Platz?
5. Alt+X (wie 3ds Max Durchsicht) und Alt+Z (wie Blender Röntgen) brauchen eine Mischstufe im Zeichner - R14/AP13.
- Der Wert „7490 Dreiecke“ in „Modell“ zählt alle Flächen der Stufe (auch die von Haut/_off ausgeblendeten), der Hinweis in der Ansicht die
  gezeichneten (5138) - so wie der Entwurf es benennt („wie im Spiel gezeichnet“).
- Für D3D12: `ModellAnsichtArt::opak` und `punktFilter` (nur im Modellfenster) müssen im D3D12-Zeichner nachgezogen werden.
- Baustein AP3: `wertTabelle` bricht im kleinen Fenster einen langen Pfad mitten im Wort („_humanoi / d“) - gleicher Fund wie AP8.

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP5 nicht angefasst)

| Zeile | Stand |
|---|---|
| P1 Gruppe Modell | gleich (Datei/GLA, Andere GLA, Texturen neu laden Alt+R, Importieren); offen und nicht AP5: „In neuem Tab öffnen“ (AP11), Modell-Import XSI/FBX/OBJ/glTF (Stufe 1d/2); mehr: Flächen, Dreiecke, Ecken, Knochen in der Tabelle |
| P2 Detailstufe und Skin | gleich (1-10 wählbar, Satz des Entwurfs mit „zeigt LOD n wie im Spiel“, Picmip, Skins zweispaltig); mehr: „Auto“ wie im Spiel |
| P3 Anzeige | bewusst anders (shank 10.11.: Kopfzeilen-Menüs, MBKOPF) |
| P3a 18 Schalter | gleich (die fünf fehlenden ergänzt, an der Stelle von MBKOPF) |
| P4 Boden und Kamera | gleich |
| P5 Anbauten | gleich (Form AP3) |
| P6 Prüfung für das Spiel | gleich (Balken, Zeilen, Kurzinfo); bewusst anders: „Flächen im Modell“ ohne „Hinweis ab 32“ - in der Engine gibt es keine solche Grenze (der Entwurf nennt „Quelle Forum“); mehr: Zielspiel, Grenzen, Ghoul2-Instanzen, „ohne eigenen Schatten“ |
| P7 Szene | gleich (Szene speichern als .mvs, ModView-Skript lesen, NPC-Browser); bewusst anders: Szene speichern ohne Strg+S (Frage 1) |
| P8 Werkzeugleiste wie Blender | gleich (AP4) |
| P9 Tasten wie 3ds Max | gleich bis auf Alt+X (offen, R14/AP13) |
| P10 Tasten wie Blender | gleich für Z und Umschalt+Alt+Z; Alt+Z offen (R14/AP13) |
| P11 Startbereich | gleich |
| X18 Info zur Zeile | gleich (Rechtsklick „Info“ und Tooltip von AP2, Gruppe „Auswahl“ von AP5) |
| X19 Rechtsklick Fläche | gleich ohne Kopieren/Einfügen (AP11) |
| R7 vier Ansichten | gleich |
| R10 Hinweis in der Ansicht | gleich |
| Z20 Skin in der Leiste | gleich (in „Detailstufe und Skin“, Flächenliste in der Suche) |
| P13 Texte | gleich |

## Changelog

## English

- Model tab, View/Check: the groups of the design - Model (with Import…), Selection, Level of detail and skin, Floor and camera, Attachments, Check for the game, Scene.
- All ten levels of detail can be chosen like r_lodbias in the game; a level above the last one of the model shows the last one, and the group says so. Skins are a choice in two columns.
- Floor and camera: floor at -24 or under the lowest point of the pose, field of view 10°, 80° or 90°, plain background, weight threshold, Reset view and Photo (1920+).
- Check for the game: bars of the selected surface against the limits of the target game, surfaces, sequences within the GLA, the tags the game code looks for, shaders without image, LODs and surfaces without their own shadow.
- Scene: save model, skin, attachments, sequences and switched surfaces as a ModView script (.mvs) and read ModView scripts.
- The five missing check displays of ModView are in the header menus of the view: alpha blending, texture filter, bone weight threshold, highlight surface with its bone references, non-shadow-casting surfaces.
- Bottom right in the view: level of detail and drawn triangles. Like 3ds Max, Modeling, Weights, Painting and Animation start with four views; like Blender Z chooses the shading and Shift+Alt+Z hides the overlays. A .glm always opens in View/Check.

## Deutsch

- Model-Tab, Ansehen/Prüfen: die Gruppen des Entwurfs - Modell (mit Importieren…), Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Prüfung für das Spiel, Szene.
- Alle zehn Detailstufen sind wählbar wie r_lodbias im Spiel; eine Stufe über der letzten des Modells zeigt die letzte, und die Gruppe sagt es. Skins sind eine Auswahl in zwei Spalten.
- Boden und Kamera: Boden auf -24 oder unter dem tiefsten Punkt der Pose, Sichtfeld 10°, 80° oder 90°, einfarbiger Hintergrund, Gewicht-Schwelle, Ansicht zurücksetzen und Foto (1920+).
- Prüfung für das Spiel: Balken der gewählten Fläche gegen die Grenzen des Zielspiels, Flächen, Sequenzen innerhalb der GLA, die vom Spielcode gesuchten Tags, Shader ohne Bild, LODs und Flächen ohne eigenen Schatten.
- Szene: Modell, Skin, Anbauten, Sequenzen und geschaltete Flächen als ModView-Skript (.mvs) speichern und ModView-Skripte lesen.
- Die fünf fehlenden Prüfanzeigen von ModView stehen in den Menüs der Kopfzeile der Ansicht: Alpha-Blending, Texturfilter, Knochengewicht-Schwelle, Fläche hervorheben mit ihren Knochenbezügen, nicht schattenwerfende Flächen.
- Unten rechts in der Ansicht: Detailstufe und gezeichnete Dreiecke. Wie 3ds Max beginnen Modellieren, Weights, Painting und Animation mit vier Ansichten; wie Blender wählt Z die Schattierung und Umschalt+Alt+Z blendet die Overlays aus. Eine .glm öffnet immer in Ansehen/Prüfen.

## 中文

- 模型选项卡“查看/检查”: 设计中的各组 - 模型 (含导入……)、所选内容、细节级别和皮肤、地面和相机、附件、针对游戏检查、场景。
- 十个细节级别都可像游戏中的 r_lodbias 一样选择; 高于模型最后一级的级别显示最后一级, 组中会说明。皮肤以两列方式选择。
- 地面和相机: 地面位于 -24 或姿势最低点之下, 视野 10°、80° 或 90°, 纯色背景, 权重阈值, 重置视图和照片 (1920+)。
- 针对游戏检查: 所选面相对于目标游戏上限的进度条、面数、GLA 范围内的序列、游戏代码查找的标签、无图像的着色器、LOD 以及无自身阴影的面。
- 场景: 将模型、皮肤、附件、序列和已切换的面保存为 ModView 脚本 (.mvs), 并可读取 ModView 脚本。
- ModView 缺少的五个检查显示位于视图标题栏的菜单中: Alpha 混合、纹理过滤、骨骼权重阈值、高亮面及其骨骼引用、不投射阴影的面。
- 视图右下角显示细节级别和绘制的三角形数。像 3ds Max 一样, 建模、权重、绘制和动画以四个视图开始; 像 Blender 一样, Z 选择着色, Shift+Alt+Z 隐藏叠加层。打开 .glm 时总是进入“查看/检查”。

## 日本語

- モデルタブの表示/チェック: デザインのグループ - モデル (インポート... 付き)、選択、詳細レベルとスキン、床とカメラ、アタッチメント、ゲーム向けチェック、シーン。
- ゲームの r_lodbias と同じく 10 段階の詳細レベルをすべて選べます。モデルの最後のレベルより上は最後のレベルを表示し、グループがそれを示します。スキンは 2 列の選択です。
- 床とカメラ: 床を -24 またはポーズの最低点の下に、視野角 10°/80°/90°、単色の背景、ウェイトしきい値、ビューをリセット、写真 (1920+)。
- ゲーム向けチェック: 選択したサーフェスの対象ゲーム上限に対するバー、サーフェス数、GLA 内のシーケンス、ゲームコードが探すタグ、画像のないシェーダー、LOD、自前の影を落とさないサーフェス。
- シーン: モデル、スキン、アタッチメント、シーケンス、切り替えたサーフェスを ModView スクリプト (.mvs) として保存し、ModView スクリプトを読み込みます。
- ModView で足りなかった 5 つのチェック表示がビューのヘッダーメニューにあります: アルファブレンド、テクスチャフィルター、ボーンウェイトのしきい値、ボーン参照付きのサーフェス強調、影を落とさないサーフェス。
- ビュー右下に詳細レベルと描画中の三角形数。3ds Max 風ではモデリング、ウェイト、ペイント、アニメーションが 4 つのビューで始まり、Blender 風では Z でシェーディングを選び Shift+Alt+Z でオーバーレイを隠します。.glm は常に表示/チェックで開きます。

## Español

- Pestaña Model, Ver/Comprobar: los grupos del diseño - Modelo (con Importar...), Selección, Nivel de detalle y skin, Suelo y cámara, Acoplados, Comprobar para el juego, Escena.
- Los diez niveles de detalle se pueden elegir como r_lodbias en el juego; un nivel por encima del último del modelo muestra el último, y el grupo lo dice. Los skins son una elección en dos columnas.
- Suelo y cámara: suelo a -24 o bajo el punto más bajo de la pose, campo de visión de 10°, 80° o 90°, fondo liso, umbral de peso, Restablecer vista y Foto (1920+).
- Comprobar para el juego: barras de la superficie elegida frente a los límites del juego de destino, superficies, secuencias dentro de la GLA, los tags que busca el código del juego, shaders sin imagen, LOD y superficies sin sombra propia.
- Escena: guardar modelo, skin, acoplados, secuencias y superficies conmutadas como script de ModView (.mvs) y leer scripts de ModView.
- Los cinco indicadores de ModView que faltaban están en los menús de la cabecera de la vista: mezcla alfa, filtro de texturas, umbral de peso de hueso, resaltar superficie con sus referencias de huesos, superficies sin sombra propia.
- Abajo a la derecha en la vista: nivel de detalle y triángulos dibujados. Como 3ds Max, Modelado, Weights, Painting y Animación empiezan con cuatro vistas; como Blender, Z elige el sombreado y Mayús+Alt+Z oculta las superposiciones. Un .glm siempre se abre en Ver/Comprobar.

## Čeština

- Karta Model, Prohlížení/Kontrola: skupiny návrhu - Model (s Importovat...), Výběr, Úroveň detailu a skin, Podlaha a kamera, Připojené modely, Kontrola pro hru, Scéna.
- Všech deset úrovní detailu lze zvolit jako r_lodbias ve hře; úroveň nad poslední úrovní modelu ukazuje tu poslední a skupina to uvádí. Skiny jsou volba ve dvou sloupcích.
- Podlaha a kamera: podlaha na -24 nebo pod nejnižším bodem pózy, zorné pole 10°, 80° nebo 90°, jednobarevné pozadí, práh váhy, Obnovit pohled a Fotka (1920+).
- Kontrola pro hru: pruhy vybrané plochy proti limitům cílové hry, plochy, sekvence uvnitř GLA, tagy, které hledá kód hry, shadery bez obrázku, LOD a plochy bez vlastního stínu.
- Scéna: uložit model, skin, připojené modely, sekvence a přepnuté plochy jako skript ModView (.mvs) a načítat skripty ModView.
- Pět chybějících kontrolních zobrazení ModView je v nabídkách záhlaví pohledu: alfa prolínání, filtrování textur, práh váhy kosti, zvýraznění plochy s odkazy kostí, plochy bez vlastního stínu.
- Vpravo dole v pohledu: úroveň detailu a vykreslené trojúhelníky. Jako 3ds Max začínají Modelování, Weights, Painting a Animace se čtyřmi pohledy; jako Blender volí Z stínování a Shift+Alt+Z skryje překryvy. Soubor .glm se vždy otevře v Prohlížení/Kontrola.
