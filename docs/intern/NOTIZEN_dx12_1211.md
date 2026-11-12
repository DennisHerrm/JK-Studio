# NOTIZEN D3D12-2 (12.11.2026): Direct3D 12 wird Vorgabe

Agent D3D12-2 (Opus 5.5), Worktree `Y:\p_dx12`, Zweig `p_dx12`, Basis karte1 6bc1eaef0. Werkzeuge und Rohdaten:
`Y:\werkzeuge\dx12\r2\` (Patchskripte p20-p32, Versuchsreihen `versuch.sh`/`tab.py`, Messungen `messung_*.txt`), Vorgaenger:
`docs/intern/NOTIZEN_d3d12_1011.md` und `Y:\werkzeuge\dx12\UEBERGABE.md`.

Auftrag (shank 11.11.: "ich will definitiv das D3D12 der neue Standard wird"): Rebase auf karte1, alles zeichnen, was der
D3D11-Weg inzwischen kann; Ursachen der Langsamkeit ohne Bildverlust beheben; Editor-Mips wie im Spiel fuer beide Schnittstellen;
Direct3D 12 als Vorgabe mit Direct3D 11 als automatischem Rueckfall an allen Fehlerstellen; Messung im Ruhefenster.

## Was sich aendert

- **Direct3D 12 ist Vorgabe** (`Settings::grafikD3d12 = true`). Menue Ansicht > Grafikschnittstelle: "Direct3D 12 (Vorgabe)",
  "Direct3D 11 (Rueckfall)"; der Tooltip bricht um und nennt, was laeuft.
- **Rueckfall auf Direct3D 11** an jeder Fehlerstelle:
  - beim Start wie bisher: keine d3d12.dll/Einstiege (`dll`), kein Geraet (`geraet`), keine Swapchain (`swap`), Abgeber
    (`abgeber`), imgui_impl_dx12 (`imgui`);
  - NEU **Startmarke**: vor dem Start mit Direct3D 12 legt das Programm `<Einstellungen>\d3d12_start.marke` an und loescht sie
    nach 60 gezeigten Bildern (auch nach gescheitertem Start und beim geordneten Ende). Findet der naechste Start sie noch, ist
    der letzte Start mit Direct3D 12 abgestuerzt oder haengen geblieben: diesmal Direct3D 11, die Einstellung geht auf
    Direct3D 11, der Tooltip sagt es (Text `GrafikRueckfall`). Gilt nur fuer die Wahl aus der Einstellung (mit `BHED_D3D12`
    entscheidet der Aufrufer). Gespielt mit `BHED_D3D12_FEHLER=absturz`.
  - NEU **Geraeteverlust**: kommt beim Neuaufbau kein D3D12-Geraet, traegt Direct3D 11 weiter (vorher drei Versuche, dann
    Meldungsfenster und Ende). Gespielt mit `BHED_D3D12_FEHLER=neuaufbau`.
- **Editor-Texturen rechnen ihre Mip-Stufen ab Werk wie das Spiel** (R_MipMap, `kartegpu::mipsWieSpiel`, Entscheidung shank
  11.11.), unter beiden Schnittstellen; die Stufen des Treibers nur noch mit `karte=BhedMipsLikeGame=0` oder
  `BHED_KARTE_MIPS=treiber`.
- **Abgeber** (`gui/abgeber12*`):
  - Konstantenpuffer-Plaetze wie die Zeichenschichten sie brauchen: Vertex/Pixel b0..b8 (MBRASTER bindet `JeRaster` an b8),
    Geometrie-Shader b0 und b5 (Schattenvolumen `g_cbSchatten`). Die Wurzelsignatur bleibt bei 60 von 64 Worten.
  - **Teilabgabe**: zeichnet die offene Befehlsliste schon 1 ms auf, geht sie beim naechsten Wechsel der Ziele an die
    Grafikkarte (`BHED_D3D12_TEILABGABE=<Mikrosekunden>`, 0 = nur am Ende des Bildes).
  - Auslagern nach dem Zaunwert (p19 des Vorgaengers, geprueft und angewandt): nach dem Laden einer Karte geht sofort, was
    vorher gebraucht wurde, statt erst nach weiteren vier Sekunden.
  - lange Wartezeiten auf einen Zaunwert (ueber 250 ms) stehen im Protokoll; die Schlusszeile nennt angelegte Ressourcen,
    Teilabgaben und das Warten auf die Grafikkarte.
- **Selbsttest `leistung` Abschnitt `r`**: Model-Tab mit aufgelegtem Control Rig (ENTSCHEID2: je Bild IK fuer vier Glieder),
  ruhend und abspielend.

## Ursachen mit Beleg

| Befund | Beleg | Abhilfe |
|---|---|---|
| MBRASTER (Bodenraster im Model-Tab) fehlte unter Direct3D 12 | Lauf `r_raster12` (Bau vor dem Fix): 31 FEHLER, Protokoll "nicht umgesetzt: SetConstantBuffers(Platz ausserhalb der Wurzelsignatur)" - `zeichneRaster` bindet b8 | b0..b8 in der Wurzelsignatur (p20); `g_raster12` 42/0 |
| Schattenvolumen (Schattenart 2) unter Direct3D 12 nicht gezeichnet | Lauf `r_npc12b`: Debugschicht "Root Signature doesn't match Geometry Shader: ... BaseShaderRegister=5"; Befund "nicht umgesetzte Aufrufe 2" | GS-Register b0 und b5 (p20); `g_npc12` 15/0 |
| Absturz in jedem Lauf nach dem Rebase (auch Direct3D 11) | `r_npc11`: 0xC0000005 in `_Find_seq::_Search_impl`; derselbe Lauf mit dem Bau von p_ent2 gruen | veraltete Objektdateien des inkrementellen Baus ueber 144 Commits - sauberer Neubau (`--clean-first`), danach gruen |
| Bildabstand im Karten-Editor 5,0-6,2 statt 4,17 ms, obwohl Hauptfaden (2,7-3,6 ms) und Grafikkarte (3,1-3,8 ms) je unter dem Takt | Messung `messung_b1.txt` (Ruhe, 0 Fremdprozesse); Versuchsreihen va/vb: mit Teilabgabe 4,17-4,29 ms | Der Abgeber schickte das ganze Bild am Ende in EINER Liste ab: die Grafikkarte begann erst, wenn der Hauptfaden fertig war - Prozessor und Grafikkarte liefen hintereinander statt nebeneinander. Der D3D11-Treiber schickt in Teilen ab. Teilabgabe nach 1 ms an einem Zielwechsel (p27/p28/p31) |
| Teilabgabe nach Zeichenbefehlen (64) frass Speicher und Zeit | Reihe va: t64 Arbeitssatz ueber 8 GB (Lauf abgebrochen), Hauptfaden +50 % | nach Zeit an einem Zielwechsel statt nach Zahl (p28) |
| Bloecke der dynamischen Puffer: anderer Heap schneller? | Reihe vc: Hochlade-Heap duel_yavin Hauptfaden 2,49 -> 4,03 ms, GPU_UPLOAD (ReBAR, auf diesem Rechner "ja") 10,5 ms | nicht noetig - eigener Heap (CUSTOM, L0, WRITE_BACK) bleibt; Versuchsschalter wieder entfernt |
| Grosse Bloecke gingen nach drei freien an das System zurueck (Verdacht aus dem Profil: DestroyAllocation im Bild) | Rotlauf vd mit Bildprotokoll: im Bild ~0 neue Ressourcen | nicht noetig - kein Churn, nicht geaendert |

## Messung (Ruhefenster, 12.11. nachmittags, allein: 0 fremde Bau-/Testprozesse, Fremdlast 0,6-2,0 Kerne)

`sh /y/werkzeuge/dx12/messung_final.sh` mit `RUNDEN="1 2 3" PROFIL="" NUR=ksemnr PRAEFIX=fz`, Direct3D 11 und Direct3D 12
abwechselnd aus demselben Bau (BHED_D3D12=0/1), Median der drei Runden (Spanne in `Y:\werkzeuge\dx12\r2\messung_final_1211.txt`).
"Vorher" = derselbe Ablauf heute frueh auf dem Stand nach dem Rebase, vor den Aenderungen dieser Runde (`messung_b1.txt`, eine Runde);
"11.11." = Ruhefenster des Vorgaengers (naechste_auftraege.md). Werte: Hauptfaden-CPU / Bildabstand in ms.

| Ansicht | D3D11 | D3D12 vorher (12.11. frueh) | D3D12 jetzt | jetzt gegen D3D11 |
|---|---|---|---|---|
| Mission duel1 ruhend | 0,50 / 4,16 | 0,53 / 4,17 | 0,44 / 4,17 | CPU -12 %, Abstand gleich |
| Mission md_toaj_jedi ruhend | 0,53 / 4,17 | 0,71 / 4,17 | 0,60 / 4,23 | CPU +0,07 ms (+13 %), Abstand +1,4 % |
| Mission md_toaj_jedi Flug | 3,18 / 5,41 | 3,46 / 5,26 | 3,29 / 5,29 | CPU +3,5 %, Abstand -2,2 % |
| Mission Szene angehalten | 0,98 / 4,36 | 1,42 / 4,43 | 0,93 / 4,17 | besser |
| Mission Szene spielt | 4,00 / 4,25 | 7,25 / 7,43 | 4,13 / 4,25 | CPU +3,2 %, Abstand gleich |
| Mission duel_mustafar ruhend | 1,30 / 4,17 | 1,57 / 4,17 | 1,08 / 4,18 | besser |
| Karten-Editor Vorlage ruhend | 2,71 / 4,17 | 3,47 / 4,37 | 2,77 / 4,25 | CPU +2,2 %, Abstand +1,9 % |
| Karten-Editor Vorlage Kamera dreht | 2,82 / 4,17 | 4,13 / 4,89 | 2,80 / 4,23 | Abstand +1,4 % |
| Karten-Editor duel_yavin ohne Halme | 2,55 / 4,17 | 3,20 / 5,56 | 2,48 / 4,25 | Abstand +1,9 % |
| Karten-Editor duel_yavin mit Halmen | 2,62 / 4,17 | 3,65 / 6,22 | 2,54 / 4,27 | Abstand +2,4 % |
| Karten-Editor md_am_sith ohne Effekte | 2,01 / 4,16 | 2,71 / 4,25 | 1,96 / 4,18 | gleich |
| Karten-Editor md_am_sith mit Effekten | 3,22 / 4,18 | 4,29 / 6,25 | 3,01 / 4,20 | gleich |
| Karten-Editor Echtzeit-Licht ruhend | 5,13 / 22,06 | 6,20 / 22,60 | 5,03 / 22,63 | Abstand +2,6 % (Grafikkarte) |
| Karten-Editor Echtzeit-Licht dreht | 3,76 / 12,02 | 5,10 / 12,31 | 3,86 / 12,24 | Abstand +1,8 % |
| Model ruhend / Animation | 0,72 / 4,17, 0,78 / 4,20 | 0,92 / 4,17, 1,05 / 4,17 | 0,66 / 4,17, 0,78 / 4,17 | gleich |
| Model mit Control Rig ruhend / Animation (NEU, ENTSCHEID2) | 0,58 / 4,17, 0,74 / 4,17 | 0,75 / 4,17, 1,04 / 4,17 | 0,61 / 4,17, 0,78 / 4,17 | CPU +0,03-0,04 ms (+5 %), Abstand gleich |
| Menue-Editor ruhend / Maus | 0,54 / 4,22, 0,56 / 4,22 | 0,62 / 4,17, 0,72 / 4,17 | 0,49 / 4,17, 0,51 / 4,17 | besser |

Zum Vergleich der Stand vom 11.11. (Ruhefenster des Vorgaengers): Bildabstand im Karten-Editor 6,3-7,5 statt 4,17 ms, Hauptfaden
duel1 +51 %. Die 240er Grenze haelt Direct3D 12 jetzt in jeder Ansicht, die sie unter Direct3D 11 haelt (Ausnahme Echtzeit-Licht,
dort bestimmt unter beiden die Grafikkarte).

**Control Rig (offener Punkt ENTSCHEID2):** stormtrooper, BOTH_WALK1, Rig aufgelegt (vier IK-Spuren): Hauptfaden 0,58 ms ruhend
und 0,74 ms beim Abspielen (Direct3D 11) bzw. 0,61 / 0,78 ms (Direct3D 12) - gegen 0,72 / 0,78 ms ohne Rig. Die IK je Bild kostet
im Mittel nichts Messbares; die Bildgrenze 4,17 ms bleibt mit 85 % Luft.

**GPU-Zeit je Bild:** mit Teilabgabe misst die Spanne zwischen den Zeitmarken auch die Pausen der Grafikkarte zwischen den Teilen
(sie wartet auf den naechsten Teil) - die GPU-Spalte von `messung_final_1211.txt` ist fuer Direct3D 12 deshalb zu hoch. Die
reine Arbeit kommt aus einem Lauf ohne Teilabgabe (`BHED_D3D12_TEILABGABE=0`, Reihe gp, je ein Lauf, ms):

| Ansicht | D3D11 | D3D12 | |
|---|---|---|---|
| Mission duel1 / Szene angehalten / Szene spielt | 0,260 / 0,373 / 2,796 | 0,259 / 0,350 / 1,663 | gleich bzw. weniger |
| Mission duel_mustafar | 0,372 | 0,447 | +0,075 ms (+20 %) |
| Karten-Editor Vorlage ruhend / dreht | 2,684 / 2,242 | 2,659 / 2,217 | gleich |
| Karten-Editor duel_yavin ohne / mit Halmen | 3,379 / 3,468 | 3,535 / 3,797 | +4,6 % / +9,5 % |
| Karten-Editor md_am_sith ohne / mit Effekten | 2,980 / 3,103 | 3,146 / 3,256 | +5,6 % / +4,9 % |
| Echtzeit-Licht ruhend / dreht | 21,667 / 11,478 | 22,696 / 11,929 | +4,7 % / +3,9 % |
| Model, Menue-Editor | 0,41-0,52 | 0,37-0,56 | gleich bzw. weniger |

**Speicher (Median, MB, Arbeitssatz / Grafikspeicher):**

| Stelle | D3D11 | D3D12 vorher | D3D12 jetzt |
|---|---|---|---|
| Start | 232 / 52 | 290 / 104 | 291 / 104 |
| nach duel1 | 552 / 331 | 736 / 384 | 736 / 384 |
| nach md_toaj_jedi | 1290 / 563 | 1645 / 617 | 1676 / 617 |
| nach md_am_sith | 1608 / 782 | 1991 / 708 | 2038 / 708 |
| nach duel_mustafar | 1654 / 838 | 2066 / 749 | 2162 / 749 |
| Karten-Editor nach der .map | 2412 / 2438 | 3648 / 3771 | 3769 / **2808** |
| Karten-Editor mit Echtzeit-Licht | 2392 / 2437 | - | 3455 / 2650 |
| Ende | 2519 / 2436 | - | 3141 / 466 |

Grafikspeicher: im Karten-Editor nach der .map 2808 statt 3771 MB (Auslagern nach dem Zaunwert), Rest +370 MB (+15 %) gegen
Direct3D 11; in der Vorschau 9-11 % WENIGER als Direct3D 11. Arbeitssatz: weiter +16 bis +56 % (der D3D11-Treiber haelt deutlich
weniger seiner privaten Seiten im Arbeitssatz; privat nur +3 bis +8 %: 7515 gegen 8103-8175 MB Spitze). Nicht eingegrenzt.

**Laden:** Programmstart bis erstes Bild 363 -> 493 ms (+130 ms: D3D12-Geraet, Wurzelsignatur, Swapchain); erstes Bild nach dem
Laden einer Karte +8 bis +12 % (Pipeline-Zustaende entstehen beim ersten Gebrauch); Karten-Editor oeffnen gleich (7073 / 7045 ms);
geladen gleich. Leerlauf: 0,6-1,1 % gegen 1,1-1,8 % eines Kerns.

## Rot / Gruen je Punkt

| Punkt | Rot (Bau vor dem Fix) | Gruen |
|---|---|---|
| MBRASTER unter Direct3D 12 (b8) | `r_raster12b` 11 OK / 31 FEHLER | `g_raster12` 42 / 0 |
| Schattenvolumen unter Direct3D 12 (GS b5) | `r_npc12b` 14 / 1 (nicht umgesetzt 2, Debugschicht 1) | `g_npc12` 15 / 0 |
| Vorgabe Direct3D 12, Einstellung, Mips wie das Spiel | `r_gr_vor` 14 / 4 | `g_gr_vor` 19 / 0 |
| Startmarke (abgestuerzter Start) | `r_gr_abs` 13 / 5 | `g_gr_abs` 18 / 0 |
| Neuaufbau ohne D3D12-Geraet -> Direct3D 11 | `r_gr_neu` 14 / 5 | `g_gr_neu` 18 / 0 |
| Rueckfall beim Start dll / geraet / swap / abgeber / imgui | (Weg unveraendert) | `g_gr_dll`, `g_gr_geraet`, `g_gr_swap`, `g_gr_abgeber`, `g_gr_imgui` je 18 / 0 |
| Direct3D 11 ausdruecklich (`BHED_D3D12=0`) | - | `g_gr_11` 18 / 0 |
| Bildabstand Karten-Editor | `messung_b1`: 4,37-6,25 ms | `messung_final_1211`: 4,18-4,27 ms (Direct3D 11 4,16-4,18) |

Die Pruefung "nach dem Start liegt keine Startmarke mehr" war im ersten Rotlauf aus einem Testfehler rot (sie lief vor dem
60. Bild) - jetzt wartet sie auf mehr als 60 Bilder; ihr eigentliches Rot ist der Absturzfall.

## Bildvergleich Direct3D 11 gegen Direct3D 12

Fotos je Schnittstelle aus demselben Bau (BHED_FOTOS), pixelgenau mit `Y:\werkzeuge\dx12\vgl.py`:

| Fotos | Ergebnis |
|---|---|
| `leistung` (feste Lage, feste Zeit; Laeufe f_l11 / f_l12) | `leistung_toaj`, `_mustafar`, `_szene40`, `_editor_vorlage_spiel`, `_menue` gleich; `leistung_editor_vorlage` 1416 Bildpunkte um 1/255, `_editor_echtlicht` 1052 (hoechstens 11/255), `_editor` (Texturbrowser mit Kamera) 1041, `leistung_modell` 2, `leistung_modell_rig` 3 Bildpunkte um 1/255 - alles Kanten der Mehrfachabtastung (Vorgaenger: mit CameraMSAA=0 gleich), dieselben Zahlen wie vor dieser Runde; `leistung_mission` 89 Bildpunkte = die fps-Zahl (3 gegen 2) und die bekannte Lavakante |
| `grafik` (h_gr_11 / h_gr_vor) | gleich bis auf `grafik_menue` (Ziffer im Tooltip) und `grafik_mipstufen` (Stufen des Treibers gegen die des Abgebers, 97 Bildpunkte, hoechstens 2/255 - genau der Grund fuer die neue Vorgabe; `grafik_mipstufen_spiel` gleich) |
| AP5 `ModellAnsichtArt::opak` (mb_ap5, `ap5_max_alphaaus`, `ap5_blender_alphaaus`) | gleich |
| AP5 `punktFilter` (`ap5_max_filteraus`, `ap5_blender_filteraus`) | 10 bzw. 13 Bildpunkte um 1/255 (Kanten wie oben) |
| Nachbargruppen (Runde sw1) | Unterschiede ueber 1/255 sind Inhalt, nicht Zeichnen: Laufordner im Pfad (d11/d12, 223 Bildpunkte), laufende Animation und Szene (mb_ap2, mission1), fps-Zahl, Bauzeiten, wann der Texturbrowser nachgeladen hat (s9c); angesehen in `Y:\werkzeuge\dx12\r2\vgl_*` |

Versuch Loeschwert (p16 des Vorgaengers, hier p32): die Kanten bleiben gleich (1416/1052/2 Bildpunkte) - wieder entfernt.

## Pruefliste der bekannten D3D12-Tempo-Regeln (Microsoft, NVIDIA "DX12 Do's and Don'ts", AMD RDNA Guide, PIX)

| Regel | Stand im Abgeber |
|---|---|
| Viele kleine Committed Resources (64-KB-Ausrichtung) | offen: jede Textur ist eine eigene Zuteilung. Placed Resources in grossen Heaps mit 4-KB-Ausrichtung fuer kleine Texturen wuerden Speicher sparen, machen aber das Auslagern (heute je Ressource) zu einer Frage je Heap. Nicht gebaut; Groesse des Verschnitts nicht gemessen |
| Kleine Puffer in einem Ring/Unterzuteiler | erledigt: Bloecke 256 B bis 64 KB aus Seiten zu 1 MB, grosse einzeln mit Vorrat |
| MSAA-/Zwischenziele nicht doppelt, Aliasing | nicht gebaut: die Zeichenschichten legen ihre Ziele an wie unter Direct3D 11; Aliasing aendert die Zeichenschichten |
| MakeResident/Evict nur unter Budgetdruck | bewusst anders: nach Zeit (4 s ungenutzt, Zaunwert erreicht). Nur unter Budgetdruck behielt der Abgeber 4,8 GB (Budget des Systems 7,2 GB, Vorgaenger-Messfenster 21:50); Direct3D 11 haelt meist nur 2,1-2,4 GB |
| Ein Command Allocator je Bild im Flug, Reset nach dem Zaun; kein Zaun-Warten mitten im Bild | erledigt (drei Bildplaetze). Gewartet wird nur bei Map(READ) (Fotos, Lesen von Bildern) und wenn die Vor-Liste beim Laden 256 MB erreicht |
| 2-3 Bilder im Flug, wartbare Swapchain, FLIP_DISCARD | erledigt (Latenz 2, drei Puffer, wie Direct3D 11) |
| PSOs vorab/zwischenspeichern | teilweise: je Schluessel einmal angelegt und behalten; keine ID3D12PipelineLibrary - das Anlegen beim ersten Gebrauch kostet beim ersten Bild nach dem Laden (erstes Bild +10 bis +30 %, siehe Messung). Offen |
| Zeichnungen nach PSO sortieren | nicht moeglich ohne Bildaenderung: die Reihenfolge ist die des Spiels (Mischen) |
| Redundante Set-Aufrufe vermeiden | erledigt: Zustandsspeicher mit Schmutzbits je Platz |
| Root Signature klein, Root-CBV statt Tabellen | erledigt bis auf die Groesse: 60 Worte (Root-CBVs je Platz, je Textur eine Tabelle mit einem festen Deskriptor) |
| SetDescriptorHeaps einmal je Liste, keine Deskriptor-Kopien je Zeichnung | erledigt (listeNeu; jede Sicht hat ihren festen Deskriptor) |
| Barrieren buendeln, Puffer ohne ausdrueckliche Wechsel | erledigt (wechsle mit Feld; Puffer implizit aus COMMON) |
| Dauerhaft abgebildeter Hochlade-Ring | erledigt |
| Copy-Queue fuer grosse ruhende Daten | nicht gebaut: Hochladen in der Vor-Liste der Direct-Queue |
| Wenige ExecuteCommandLists, aber frueh genug | umgesetzt mit Messung: Teilabgabe nach 1 ms (siehe Ursachen) |
| Listen auf mehreren Faeden aufnehmen | nicht gebaut (die Zeichenschichten zeichnen auf einem Faden, wie unter Direct3D 11) |
| Aufloesen im Render-Pass statt ResolveSubresource + Kopie | nicht gebaut: der Rest von +0,2 ms GPU (8x Kopie/Aufloesen im Treiber) bleibt offen; ein Aufloesen im Shader rundet anders (Bildaenderung) |
| Keine unnoetigen Clears; schnelles Loeschen | versucht (p16 des Vorgaengers als p32: Loeschwert folgt der Loeschfarbe) - aenderte die Kanten nicht, kein messbarer Gewinn, wieder entfernt |
| Messen ohne Debugschicht | erledigt (`BHED_D3D_DEBUG=0` in den Messlaeufen) |

## Offen / Fragen an shank

- **Kanten der Mehrfachabtastung** (1/255 auf Dreieckskanten, Karten-Editor-Kamera 1000-1400 Bildpunkte, Echtzeit-Licht bis
  11/255, Model 2-13 Bildpunkte): Ursache weiter offen; der Loeschwert ist es nicht (Versuch p32). Vor dieser Runde ebenso.
- **GPU-Zeit** ohne Teilabgabe +4 bis +10 % im Karten-Editor (duel_yavin mit Halmen +9,5 %), Echtzeit-Licht +4-5 %, duel_mustafar
  +0,075 ms: Treiber bei 8x Kopie/Aufloesen (Vorgaenger: +0,2 ms mit Himmelsportal) und vermutlich die Ecken der Halme aus dem
  Arbeitsspeicher (Bloecke im CUSTOM-Heap); Plan: Portalbild ohne Kopie (Zielsaetze tauschen), ruhende Kopie auch fuer je Bild
  beschriebene grosse Eckpuffer (doppelt gepuffert). Bildabstand bleibt trotzdem an der Grenze.
- **Arbeitssatz** +16 bis +56 %, Grafikspeicher Karten-Editor +370 MB (+15 %): kleine Texturen als Placed Resources (4-KB-Ausrichtung)
  und Aliasing der Zwischenziele waeren die naechsten Schritte (Pruefliste); nicht gebaut.
- **Groesstes Bild beim Optimierer auf der grossen Karte** (ke_bau, duel_mos_espa): Direct3D 12 139-262 ms, Direct3D 11 72-93 ms
  (Grenze 250 ms; einmal unter Volllast mit neun Laeufen 262 ms = rot, allein und mit vier Laeufen gruen). Vermutung: Zurueckholen
  ausgelagerter Texturen (MakeResident) oder Pipeline-Zustaende beim ersten Gebrauch. Abhilfe waere EnqueueMakeResident bzw.
  eine Pipeline-Bibliothek.
- **mb_ap8 W12 (Blender-Teil)** unter Volllast zweimal rot (Tupfer malt 0 statt 0,5), allein dreimal gruen (start.sh mit/ohne
  Teilabgabe und Auslagern, testrunde allein). Zeitabhaengig; unter Direct3D 11 nie rot gesehen.
- **ke_grund** "Klick nicht ganz sichtbar (929..1619, sichtbar 928..1610)": auch unter Direct3D 11 und mit dem Bau von p_ent2
  (karte1 6bc1eaef0) rot - nicht aus diesem Zweig.
- Zwei testrunde-Pools nebeneinander mit denselben ke_*-Teilen belegen dieselben Ports (39000 ff.) - ke_bau unter beiden
  Schnittstellen rot "Port nicht belegt"; nacheinander gruen. Hinweis fuer kuenftige Doppelrunden.
- Erstes Bild nach dem Laden +8 bis +12 %, Programmstart +130 ms: ID3D12PipelineLibrary waere der Weg (Pruefliste).
- Der 37-Sekunden-Halt in "Figurenmodelle suchen" (einmal in `messung_b1`, Direct3D 12, duel_mustafar) kam in sieben weiteren
  Laeufen nicht wieder; lange Wartezeiten auf einen Zaunwert stehen jetzt im Protokoll ("D3D12: ... ms auf den Zaunwert").

Fragen an shank (gesammelt, nicht blockierend):
1. Startmarke: nach einem abgestuerzten Start mit Direct3D 12 stellt das Programm die Einstellung dauerhaft auf Direct3D 11 (mit
   Hinweis im Menue). Soll es stattdessen beim naechsten Start Direct3D 12 noch einmal versuchen?
2. Rest-GPU-Zeit +4-10 % im Karten-Editor (Bildabstand gleich): reicht das fuer "nicht langsamer", oder soll der Portal-/Halm-Umbau
  vor dem Release kommen?

## Changelog

## English

- Direct3D 12 is now the default graphics API: it draws the interface and all 3D views. Direct3D 11 stays available under View → Graphics API and takes over by itself when Direct3D 12 is missing, fails to start, did not finish the last start (the setting then switches to Direct3D 11 and the menu says so) or is lost while working.
- Faster with Direct3D 12: the graphics card starts on a frame while it is still being prepared, so the map editor holds the full refresh rate again.
- The model tab grid and the shadow volumes (shadow type 2) are drawn with Direct3D 12 as with Direct3D 11.
- Map editor textures compute their mipmaps like the game (R_MipMap) by default, with both graphics APIs.

## Deutsch

- Direct3D 12 ist jetzt die vorgegebene Grafikschnittstelle: Es zeichnet die Oberfläche und alle 3D-Ansichten. Direct3D 11 bleibt unter Ansicht → Grafikschnittstelle wählbar und übernimmt von selbst, wenn Direct3D 12 fehlt, nicht startet, den letzten Start nicht beendet hat (dann steht die Einstellung auf Direct3D 11, und das Menü sagt es) oder während der Arbeit ausfällt.
- Schneller mit Direct3D 12: Die Grafikkarte beginnt ein Bild schon, während es noch vorbereitet wird - der Karten-Editor hält wieder die volle Bildrate.
- Das Raster im Model-Tab und die Schattenvolumen (Schattenart 2) erscheinen mit Direct3D 12 wie mit Direct3D 11.
- Texturen im Karten-Editor rechnen ihre Mip-Stufen ab Werk wie das Spiel (R_MipMap), mit beiden Grafikschnittstellen.

## 中文

- Direct3D 12 现在是默认图形接口:它绘制界面和所有 3D 视图。Direct3D 11 仍可在“视图 → 图形接口”中选择;当缺少 Direct3D 12、Direct3D 12 无法启动、上次启动未完成(此时设置会改为 Direct3D 11,菜单会说明)或在工作中失效时,会自动接管。
- Direct3D 12 更快:显卡在一帧仍在准备时就开始处理,地图编辑器重新达到完整刷新率。
- 模型选项卡中的网格和阴影体(阴影类型 2)在 Direct3D 12 下与 Direct3D 11 下相同。
- 地图编辑器中的纹理默认像游戏一样计算 mipmap(R_MipMap),两种图形接口都如此。

## 日本語

- Direct3D 12 が既定のグラフィックス API になりました。インターフェースとすべての 3D ビューを描画します。Direct3D 11 は「表示 → グラフィックス API」で引き続き選べ、Direct3D 12 がない、起動できない、前回の起動が完了しなかった(その場合は設定が Direct3D 11 になり、メニューに表示されます)、作業中に失われた場合には自動的に引き継ぎます。
- Direct3D 12 で高速化:フレームの準備中にグラフィックカードが処理を始めるため、マップエディターが再びフルのリフレッシュレートを保ちます。
- モデルタブのグリッドとシャドウボリューム(影の種類 2)は、Direct3D 12 でも Direct3D 11 と同じように描画されます。
- マップエディターのテクスチャは、どちらのグラフィックス API でも既定でゲームと同じようにミップマップを計算します(R_MipMap)。

## Español

- Direct3D 12 es ahora la interfaz gráfica predeterminada: dibuja la interfaz y todas las vistas 3D. Direct3D 11 sigue disponible en Ver → Interfaz gráfica y toma el relevo por sí solo si falta Direct3D 12, no logra iniciarse, el último inicio no terminó (entonces el ajuste pasa a Direct3D 11 y el menú lo indica) o falla durante el trabajo.
- Más rápido con Direct3D 12: la tarjeta gráfica empieza un fotograma mientras aún se prepara, y el editor de mapas vuelve a mantener la frecuencia de actualización completa.
- La cuadrícula de la pestaña Model y los volúmenes de sombra (tipo de sombra 2) se dibujan con Direct3D 12 igual que con Direct3D 11.
- Las texturas del editor de mapas calculan sus mipmaps como el juego (R_MipMap) de forma predeterminada, con ambas interfaces gráficas.

## Čeština

- Direct3D 12 je nyní výchozí grafické rozhraní: kreslí rozhraní i všechny 3D pohledy. Direct3D 11 zůstává k dispozici v Zobrazení → Grafické rozhraní a samo převezme práci, když Direct3D 12 chybí, nespustí se, poslední spuštění nebylo dokončeno (nastavení se pak přepne na Direct3D 11 a nabídka to uvede) nebo během práce selže.
- Rychlejší s Direct3D 12: grafická karta začne snímek zpracovávat ještě během jeho přípravy, takže editor map opět drží plnou obnovovací frekvenci.
- Mřížka na kartě Model a stínové objemy (typ stínu 2) se s Direct3D 12 kreslí stejně jako s Direct3D 11.
- Textury v editoru map počítají mipmapy ve výchozím stavu jako hra (R_MipMap), s oběma grafickými rozhraními.
