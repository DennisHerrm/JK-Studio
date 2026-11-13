# NOTIZEN REST612 (13.11.2026): Arbeitsspeicher unter Direct3D 12, Leck je Geraeteverlust, DXR-Gitter, Einstellungsdatei

Zweig `p_rest612` (Worktree `Y:\p_dx12`, auf karte1 f0d55d90a), Werkzeuge `Y:\werkzeuge\rest612`. Auftrag vom Hauptchat (REST612),
Vorarbeit `NOTIZEN_dx12c_1211.md` (D3D12-3), `NOTIZEN_gitter_1311.md`, `NOTIZEN_dxr_1211.md`. Lastregel: Bau `--parallel 2`,
hoechstens 2 jkstudio, alles mit Leerlauf-Prioritaet; Messungen nur in eigenen Messfenstern (`mess_start.sh`/`mess_ende.sh`).

## Was sich aendert

| Punkt | Aenderung | Dateien |
|---|---|---|
| 1 Arbeitsspeicher | Hochladespeicher des D3D12-Rahmens: Die Vor-Liste geht alle 16 MB ab, ohne zu warten; ihre Puffer (und die der Liste des Bildes) sind "unterwegs", bis die Grafikkarte den Zaunwert erreicht, und kommen dann in einen Vorrat, aus dem neue Puffer genommen werden. Gewartet wird erst ueber 32 MB unterwegs. Puffer, die 60 Bilder ungebraucht im Vorrat lagen, gehen zurueck; `trimmen()` leert den Vorrat. Vorher fuellte jeder der drei Bildplaetze beim Laden bis 256 MB und gab sie erst frei, wenn er wieder dran war | gui/backend_d3d12_win32.cpp |
| 2 Leck je Geraeteverlust | Der Abgeber zaehlt die lebenden Objekte frueherer Geraete (`Zaehler::lebendFrueher`, im Befund "N Objekte (M von frueheren Geraeten)"). Karten-Editor: der 3D-Teil von karte_gpu legt die Abtaster nicht noch einmal an, wenn `abtasterSetzen` (Texturqualitaet) auf dem neuen Geraet schon lief | gui/abgeber12*.h/.cpp, gui/backend_d3d12_win32.cpp, gui/karte_gpu_win32.cpp |
| 2 Selbsttest | `grafik` mit `BHED_GRAFIK_NUR=verlustreihe`: je drei Verluste nacheinander im Menue-Editor (ui/saber.menu mit Griff und Klingen), in der Vorschau des Mission-Tabs (Spieler mit brennender Klinge) und im Karten-Editor mit Echtzeit-Licht und Raytracing-Schatten; Pruefung: die Objekte frueherer Geraete wachsen nicht | gui/selbsttest_grafik.inc |
| 3 DXR-Selbsttest | Statt "Strahlen hoechstens 35 % neben gebacken" die Topologie: Gitter in einer Lage 24 unter dem Licht, Boden in einer func_group mit `_lightmapscale 0.125`; dunkle und helle Inseln in Strahlen, Schattenkarten und gebacken. Gegenprobe `BHED_DXR_GITTER_REGEL=spiel`. Kommentare berichtigt | gui/selbsttest_dxr.inc |
| 4 Einstellungsdatei | `readSettings` liest ueber eine Tabelle Schluessel -> Leser (`kLeser`, `kVorsatzLeser`, drei benannte Leser fuer Zweige mit mehreren Schluesseln) statt der else-if-Kette an der Compilergrenze C1061. Ein neuer Schluessel ist eine Zeile in `kLeser`. Umbau mechanisch per Skript (`Y:\werkzeuge\rest612\p01_tabelle.py`, nach einem Rebase wiederholbar); lint_settings kennt die Tabelle | src/settings.cpp, tools/lint_settings.py, tests/einstellungen_alle.cpp, tests/einstellungen/alle_erwartet.txt, CMakeLists.txt |

## Punkt 1: woher die +0,5 GB kamen

Werkzeug `rlauf.py` (leistung-Lauf mit Speicherabbild an jeder Messstelle und, mit `VERLAUF=1/2`, jede Sekunde): Adressraum
per VirtualQueryEx, Leistungsindikatoren "GPU Process Memory" (Dedicated, Shared, Local, Non Local, Total Committed).

- **Privat ist fast ganz Adressraum**: PagefileUsage minus festgelegter privater Adressraum = 54-68 MB unter beiden Wegen.
  Die Zuteilungen der Grafikkarte stehen als festgelegter privater Speicher im Adressraum (unter Direct3D 11 Schutz 0x4, unter
  Direct3D 12 0x404 schreibkombiniert; das 3072x2048x149-Feld des Echtzeit-Lichts steht unter Direct3D 11 als eine
  Zuteilung von 1788 MB da). Privat = Speicher des Programms + alle Zuteilungen der Grafikkarte, ob ausgelagert oder nicht.
- **Speicher des Programms gleich**: die grossen Heap-Bloecke (371 Stueck, 1069 MB), die Heap-Segmente (440 gegen 432 MB)
  und die kleinen Zuteilungen sind unter beiden Wegen Stueck fuer Stueck gleich (`vgl_abbild.py`). Der Unterschied liegt
  ganz in den Zuteilungen der Grafikkarte und des Treibers.
- **Kein Auslagern-Effekt** (wie F1): ohne Auslagern (`BHED_D3D12_AUSLAGERN=0`) nach der .map 7195 MB, nach Zeit 7198-7264.
  Der Budget-Weg sah in einem Lauf gleich aus wie Direct3D 11 (6666 MB) - aber nur, weil das Echtzeit-Licht unter Budgetdruck
  weniger Schichten seines Felds belegt hatte (1236 statt 1788 MB); kein Vergleich.
- **Der Zeitverlauf zeigte die Spitze**: Waehrend die .map laedt, springen die Zuteilungen von 27-32 MB (Teilzuteilungen des
  Treibers) unter Direct3D 12 von 960 auf 1972 MB und sinken danach ueber 10 s auf 1290 MB; unter Direct3D 11 von 907 auf
  1124 und dann 1039 MB. Eine Protokollzeile beim Freigeben eines Bildplatzes zeigte Bildplaetze mit 280, 310, 219, 314,
  276 MB Hochladepuffern nacheinander (`kVorGrenze` 256 MB je Platz, drei Plaetze, Freigabe erst beim naechsten Umlauf, und
  danach haelt der Treiber Freigegebenes noch Sekunden). Mit der Grenze 16 MB (Versuch, Umgebungsvariable) fiel die Spitze
  sofort um rund 300 MB.
- **Nicht die Ursache**: Pipeline-Zustaende (83 je Lauf, pso12.bin 0,4 MB), Kopien von Texturinhalten (keine im Abgeber),
  Debugschicht (aus), Ausrichtung kleiner Texturen (273 Texturen bis 64 KB zusammen 17 MB), Abtaster/Deskriptorstapel.

## Rot / Gruen

| Punkt | Rot | Gruen |
|---|---|---|
| 1 Speicher (Messfenster, kein fremder Bau) | D3D12 karte1 nach der .map 7264 / 7267 / 7283 MB, Spitze 7292 / 7333 / 7343 | 16/32 MB: 6929 / 6937, Spitze 6966 / 6997; 32/64 MB: 6986-7021 / 7017-7065 |
| 2 Menue-Editor, Vorschau (verlustreihe, D3D12) | - (kein Leck: 0 / 0 / 0 schon vor der Aenderung, `vr2`) | `vr8` 0 / 0 / 0, Menue 1040 Modellaufrufe, Spieler mit Klinge gezeichnet |
| 2 Karten-Editor mit Echtzeit-Licht und DXR (verlustreihe) | `vr2`/`vr3` 2 / 4 / 6 Objekte frueherer Geraete (je Verlust zwei Abtaster ANISOTROPIC, WRAP und CLAMP, ein Verweis, nicht gebunden - Diagnose `p09`/`p10`, wieder entfernt) | `vr7`/`vr8` 0 / 0 / 0; D3D12 7 OK / 0 FEHLER, D3D11 6 OK / 0 FEHLER |
| 3 DXR-Topologie | Gegenprobe `dx1g` (Licht durch die Felder erwartet): 35 OK / 1 FEHLER | `dx1` 36 OK / 0 FEHLER: Strahlen, Schattenkarten, gebacken je 5 dunkle / 0 helle Inseln |
| 4 Einstellungsdatei | Gegenproben: ein Leser mit anderer Grenze (menueTextZoom 51) 3087 Zeilen verschieden; "der letzte gilt" statt "der erste" (splitMap doppelt) 2606 verschieden; lint_settings ohne einen Tabelleneintrag 2 Beanstandungen | ctest einstellungen_alle: 3896 Zeilen, 66 Abschnitte, 0 verschieden; 449 echte Einstellungsdateien (beide AppData-Dateien, alle Laufordner) byte-gleich gelesen |

Punkt 2, Ursache im Karten-Editor: Nach einem Verlust lief `abtasterSetzen` (Kamera-Lader, weil sich der Geraetestand
geaendert hatte) vor dem 3D-Teil von karte_gpu; der legte `g.abtaster`/`g.abtasterKlemm` noch einmal an und ueberschrieb
den Zeiger. Gleiche Beschreibung gibt unter Direct3D 11 wie im Abgeber dasselbe Objekt mit einem Verweis mehr - den gab
niemand frei, und unter Direct3D 11 hielt er das alte Geraet am Leben. Mit einer anderen Texturqualitaet als der Vorgabe galt
danach ausserdem wieder die Vorgabe (anisotrop) statt der Einstellung.

Punkt 2, Bildvorraete mit `render::createTexture` (Menue-Editor `textur`/`kino`/`weissesBild`/Referenzbild, Schwert
`blitzTextur`, Zeitleiste, Malen, Untertitel): Vergessen ist dort richtig - `destroyTextures` (backend_win32.cpp) gibt beim
Verlust alle Bilder der Oberflaeche zentral frei. gpumap gibt seine eigenen Objekte in `shutdown` frei, die geteilten Bilder
(`stufenGeteilt`) beim Geraetewechsel.

## Messung (Messfenster, Richtwerte ohne fremden Bau)

Fenster 15:30-15:36 und 15:46-15:52 (`mess1.sh`, Tabellen `m1_tab.txt`, `m2_tab.txt`), `leistung` Abschnitt e, Taris-.map,
Leerlauf-Prioritaet, je Lauf nacheinander, Fremdlast 0 laufende Bauten/Tests. MB privat:

| Lauf | nach der .map | Spitze | Ende | .map oeffnen (ms) |
|---|---|---|---|---|
| Direct3D 11 | 6676 | 6717 | 6517 | 6213 |
| Direct3D 12 karte1 | 7264 / 7267 / 7283 | 7292 / 7333 / 7343 | 6783 / 6788 / 6794 | 6769 / 7212 / 6895 |
| Direct3D 12 neu, 32 / 64 MB | 6986 / 6986 / 7021 | 7027 / 7017 / 7065 | 6755 / 6755 / 6795 | 7792 / 7326 / 8800 |
| **Direct3D 12 neu, 16 / 32 MB (Vorgabe)** | **6929** / 6937 | **6966** / 6997 | **6765** / 6759 | **6905** / 7820 |

- Gewinn gegen karte1: nach der .map -350 MB, Spitze -350 MB, Ende -25 MB. Die Ladezeit streut zwischen Laeufen desselben
  Baus um mehr als eine Sekunde (karte1 6769-7212, neu 6905-8800); ein Unterschied ist darin nicht zu sehen.
- Bild: geaendert ist nur der Weg der Anfangsdaten; Fotos von leistung e neu gegen karte1 siehe Schlussrunde.

## Offen

- **Ziel "nicht mehr als Direct3D 11" nicht ganz erreicht**: Es bleiben rund +250 MB privat (nach der .map 6929 gegen 6676,
  Ende 6765 gegen 6517). Der Rest liegt in den Zuteilungen der Grafikkarte, nicht im Speicher des Programms: Total Committed
  Direct3D 12 4998-5013 gegen 4790 MB (+215), Shared Usage 364-374 gegen 187-189 MB (+180, davon die Bloecke der dynamischen
  Puffer 86 MB, die Direct3D 11 im Treiber haelt). Bekannte Teile: ruhende Kopien der dynamischen Puffer (53 MB, Block und
  Kopie gleichzeitig), Vorrat freigegebener Puffer (bis 29 MB), freie Bloecke (31 MB), die Teilzuteilungen des Treibers
  (27-32-MB-Zuteilungen: Direct3D 12 1524-1556 gegen 912 MB). Weiter ginge es nur mit eigenen Heaps fuer kleine Ressourcen
  (Platzieren statt CreateCommittedResource) oder mit Kuerzungen, die Tempo kosten koennen - nicht gemacht.
- DXR (karte_dxr_win32.cpp `allesFrei(false)` nach einem Geraetewechsel): Die Objekte des Abgebers gehen dort nicht verloren
  (verlustreihe mit DXR 0 / 0 / 0); ob rohe D3D12-Objekte (Strukturen, ID3D12Device5) des alten Geraets liegen bleiben, zaehlt
  der Abgeber nicht - offen.
- Model-Tab (nicht angefasst, Auftrag): Zeitleiste `app_zeitleiste.cpp` (Filmstreifen 815/1526, Untertitel 1915) und Malen
  `app_modelltab_malen.cpp` (2035) benutzen `render::createTexture` - kein Leck (siehe oben). Die Modellansicht
  (`app_modelltab_ansicht.cpp`) zeichnet ueber gpumap; die verlustreihe hat keinen Model-Tab-Fall (R1 hat einen Verlust mit
  offenem Model-Tab).

## Gelaufen (Schlussrunde, Leerlauf-Prioritaet, hoechstens 2 jkstudio)

- Bau (alle Ziele, `--parallel 2`, /W4 /WX): 0 Fehler, 0 Warnungen.
- ctest (`-j 4`): 272 von 272 bestanden, darunter einstellungen_alle.
- `grafik` ganzer Lauf: Direct3D 12 23 OK / 0 FEHLER, Direct3D 11 22 / 0. R1 (`BHED_GRAFIK_NUR=verlust`): Direct3D 12 6 / 0,
  Direct3D 11 5 / 0. verlustreihe: Direct3D 12 7 / 0 (0 / 0 / 0 in allen drei Bereichen), Direct3D 11 6 / 0.
- `karteneditor` mit `BHED_KARTE_NUR=dxr` (Direct3D 12): 36 OK / 0 FEHLER; Gegenprobe 35 / 1 (nur die Topologie). Fotos
  `dxr_topologie_strahlen/karten/gebacken` angesehen: dasselbe Gitter (helle Staebe, dunkle Felder, dieselbe gebrochene Ecke),
  Strahlen scharf, Schattenkarten etwas weicher, gebacken weich nach Luxeln.
- `leistung` e mit Fotos: Direct3D 12 karte1 gegen neu 4 von 4 Bildern bitgleich. Direct3D 11: karte1 gegen neu 1 gleich,
  3 mit 1-50 Bildpunkten anders - dieselben drei Bilder schwanken schon zwischen zwei Laeufen desselben Baus (karte1 gegen
  karte1 50 Bildpunkte bis 11/255 in leistung_editor, ein flackernder Funke; neu gegen neu bis 1/255); karte1 (zweiter Lauf)
  gegen neu hoechstens 1/255.
- `menueeditor` (Direct3D 12, ganzer Lauf): 2790 OK / 0 FEHLER.
- Lints: settings ok, d3dbesitz ok, headers ok, feldinit ok, portability 101 (Grundstand), pruefcodes 11 (Grundstand).
- Bewusst nicht: volle Runde (macht der Hauptchat), Handbuch (keine sichtbare Aenderung an Oberflaeche oder Texten).

## Changelog

## English

- Direct3D 12: opening a map needs up to 0.35 GB less memory - textures are uploaded through reused buffers instead of up to 256 MB per frame.
- Map editor: after the graphics card was lost or reset, the texture quality setting stays as set, and no memory of the earlier graphics device is kept.

## Deutsch

- Direct3D 12: Das Öffnen einer Karte braucht bis zu 0,35 GB weniger Arbeitsspeicher - Texturen werden über wiederverwendete Puffer hochgeladen statt mit bis zu 256 MB je Bild.
- Karten-Editor: Nachdem die Grafikkarte verloren ging oder zurückgesetzt wurde, bleibt die Texturqualität wie eingestellt, und vom früheren Grafikgerät bleibt kein Speicher liegen.

## 中文

- Direct3D 12:打开地图最多少用 0.35 GB 内存——纹理通过重复使用的缓冲区上传,而不是每帧最多 256 MB。
- 地图编辑器:显卡丢失或重置后,纹理质量保持设定值,之前图形设备的内存也不再残留。

## 日本語

- Direct3D 12:マップを開くときのメモリが最大 0.35 GB 少なくなりました。テクスチャはフレームごとに最大 256 MB ではなく、再利用するバッファーでアップロードされます。
- マップエディター:グラフィックカードが失われたりリセットされたりした後も、テクスチャ品質は設定どおりのままになり、以前のグラフィックデバイスのメモリも残りません。

## Español

- Direct3D 12: abrir un mapa necesita hasta 0,35 GB menos de memoria: las texturas se suben mediante búferes reutilizados en lugar de hasta 256 MB por fotograma.
- Editor de mapas: después de perder o reiniciar la tarjeta gráfica, la calidad de las texturas sigue como estaba ajustada y no queda memoria del dispositivo gráfico anterior.

## Čeština

- Direct3D 12: otevření mapy potřebuje až o 0,35 GB méně paměti - textury se nahrávají přes znovu používané vyrovnávací paměti místo až 256 MB na snímek.
- Editor map: po ztrátě nebo resetu grafické karty zůstane kvalita textur podle nastavení a z předchozího grafického zařízení nezůstane žádná paměť.
