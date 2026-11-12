# NOTIZEN D3D12-3 (12.11.2026): Direct3D 12 gleich schnell, Geraeteverlust, Startmarke

Zweig `p_dx12` (Worktree `Y:\p_dx12`), Werkzeuge `Y:\werkzeuge\dx12\r3`. Auftrag `auftraege/welle_1211.md` Abschnitt 1 und die
Funde R1/R5 aus `plaene/codepruefung_seit_rc611.md`. Vorgaenger: Fable (zwei WIP-Commits), Nachfolger Opus 5.5 ab 17:45.
Alle Zeitwerte dieser Runde sind **Richtwerte unter Last** (bis 18:20 rund 29 Kerne Fremdlast, danach 6-16 Kerne; ab 18:25 eigene
Laeufe mit Leerlauf-Prioritaet nach der Lastregel des Hauptchats). Endzahlen: `Y:\werkzeuge\dx12\messung_final.sh` im Ruhefenster.

## Was sich aendert

| Punkt | Aenderung | Dateien |
|---|---|---|
| R1 Geraeteverlust | Jedes Objekt des Abgebers traegt die Generation des Geraets (`g_generation`, hochgezaehlt in `anlegen()`). Ein Objekt eines frueheren Geraets gibt beim Freigeben nichts in Bloecke, Vorrat und Deskriptor-Freilisten des neuen Abgebers, wird beim Gebrauch uebergangen und gemeldet (zaehlt im Befund als nicht umgesetzt). `Map` auf ein altes Objekt liefert DXGI_ERROR_DEVICE_REMOVED statt in freigegebenen Speicher zu schreiben. `kartegpu::freigeben()` laeuft im Abbau (Verlust und Ende) | abgeber12_intern.h, abgeber12_geraet_win32.cpp, abgeber12_kontext_win32.cpp, main_win32.cpp |
| R1 Ansichten danach | Nach einem Geraeteverlust zeigten - auch unter Direct3D 11 - die 2D-Ansichten keine Pinsel und keine Vorlage mehr, die Kamera keine Vorlage (Built map as in the game), der Model-Tab ein leeres Bild, bis sich etwas aenderte. Jetzt: 2D-Puffer merken sich den Geraetestand, die Vorlage und das Schnell-Backen laden ihre .bsp aus den Bytes neu hoch (`nachVerlustNeuHochladen`), die Modellansicht zeichnet bei neuem Geraet neu | app_karte_xy.cpp, app_karte_echtlicht.cpp, app_modelltab_ansicht.cpp |
| R5 Startmarke | Geloescht nach 60 Bildern ODER 2 s nach dem ersten gezeigten Bild (vorher nur 60 Bilder - im Leerlauf bis 15 s), bei WM_ENDSESSION (Abmelden/Herunterfahren) und bei jedem geordneten Prozessende (atexit) | main_win32.cpp |
| Teilabgabe unter Last | Hoechstens 4 Teilabgaben je Bild (`BHED_D3D12_TEILABGABEN=<n>`). Unter fremder Last zaehlte die Uhr auch die Zeit, in der der Hauptfaden nicht rechnete: jeder Zielwechsel ging einzeln ab (bis 364 Abgaben je Bild) | abgeber12_*.cpp |
| Start | Das D3D12-Geraet entsteht in einem eigenen Faden gleich nach den Einstellungen, waehrend Befehlsmodell und Spielordner laden (`render::grafikVorwaermen`); die Startmarke wird davor geprueft und angelegt; nicht mit `BHED_D3D12_FEHLER` | main_win32.cpp, backend*.cpp/h |
| PSO-Zwischenspeicher | ID3D12PipelineLibrary unter `<Einstellungen>\shader\pso12.bin` (WIP des Vorgaengers, `BHED_D3D12_PSOLAGER=0` aus) | abgeber12_*.cpp |
| Grafikkarte | Portalbild: mit Mehrfachabtastung tauscht die Hauptszene ihr Ziel mit dem Portalsatz statt das Portalbild zu kopieren (8x 1920x1440: 0,28 ms GPU). Spielgamma zeichnet mit Mehrfachabtastung gleich in das aufgeloeste Bild (ein Aufloesen und ein Vollbild-Durchgang mit Proben weniger, 0,14 + 0,10 ms). Vom Pixel-Shader gelesene dynamische Puffer (Lichtliste und Kacheln des Echtzeit-Lichts) bekommen je Map eine Kopie im Grafikspeicher (`BHED_D3D12_SRVKOPIE=0` aus). Alle drei wirken unter beiden Grafikwegen bzw. nur unter D3D12, Bild bitgleich (siehe Bildvergleich) | gpumap_win32.cpp, abgeber12_*.cpp |
| Messhilfen | Zeile je Bild ueber 100 ms (`BHED_D3D12_LANGBILD=<ms>`), Zeiten des Starts (`D3D12-Start ...`), Speicherbericht des Abgebers an den Messstellen von `leistung` | abgeber12_*.cpp, selbsttest_leistung.inc |

## Rot / Gruen

| Punkt | Rot | Gruen |
|---|---|---|
| R1 Geraeteverlust, D3D12 (Selbsttest `grafik`, `BHED_GRAFIK_NUR=verlust`) | `r1rot12`/`r1rotb12`: Absturz 0x87D (Debugschicht) im ersten Bild nach dem Neuaufbau mit offenem Karten-Editor | `r1gruen12` 6 OK / 0 FEHLER |
| R1 Ansichten nach dem Verlust, D3D11 | `r1rotb11` 2/3: Kamera 2 757 834 Bildpunkte anders, 2D 145 369 bzw. 55 598, Modellansicht nicht lesbar | `r1gruen11` 5/0: Kamera 1383 (bewegte Texturen, Grenze 1 %), 2D 0, Modell 0 |
| R1 Modellansicht (Bau nur ohne diese eine Zeile) | `r1modrot11`/`r1modrot12` je 1 FEHLER (Modellansicht leer) | `r1gruen11`/`r1gruen12` |
| R5 Startmarke (`r5_lauf.py`: Start 1 meldet ab, Start 2 mit demselben Ordner) | `r5rot`: Start 2 laeuft mit Direct3D 11, Einstellung umgestellt, 2 FEHLER | `r5gruen`: Start 2 mit Direct3D 12, Marke weg, 5/0 |
| Teilabgabe unter Last | `d_sp12` (29 Kerne Fremdlast): md_am_sith 175 / 258 ms je Bild (D3D11 12 / 20), .map oeffnen 134,8 s (D3D11 22,3 s), bis 364 Abgaben je Bild; mit `TEILABGABE=0` 14,7 / 19,0 ms und 31,8 s | `d_sp12k4`: hoechstens 7 Abgaben je Bild (4 Teilabgaben + Kopien + Ende). Unter derselben Volllast nicht wiederholt (Lastregel; keine kuenstliche Last erzeugt) |
| Groesstes Bild beim Optimierer (duel_mos_espa, Grenze 250 ms) | karte1 (r3/basis) unter D3D12: 58 bzw. 40 ms | neu 26 ms, D3D11 28 ms (Fremdlast 8 Kerne). Die 139-262 ms des Vorgaengers kamen unter Volllast - dieselbe Ursache wie die Teilabgabe |

Gruppen dieses Zweigs: `grafik` in allen Varianten (`gr_varianten.sh grv`: Vorgabe 23/0, D3D11 22/0, absturz, neuaufbau, dll, geraet,
swap, abgeber, imgui je 22/0), `leistung` e und ksmrn unter beiden Wegen, Karten-Editor Stufe 5 gross unter beiden Wegen.

## Bildvergleich

| Vergleich | Ergebnis |
|---|---|
| D3D12 karte1 gegen neu, `leistung` e (Portal, Spielgamma, Kopie der Lichtpuffer) | 4 von 4 Bildern bitgleich |
| D3D12 karte1 gegen neu, `leistung` ksmrn (Mission, Szene, Model, Menue) | 7 von 7 Bildern bitgleich |
| D3D11 karte1 gegen neu, e | 2 gleich, 2 schwanken schon zwischen zwei Laeufen desselben Baus (Texturbrowser, Echtzeit-Licht, hoechstens 3/255) |
| D3D11 gegen D3D12 (neu), e | wie vor dieser Runde: `editor_vorlage` 1416 Bildpunkte um 1/255, `editor`/`editor_echtlicht` 1076/1045 (Kanten, siehe Punkt 5) |

## Messung (Richtwerte)

Messfenster 19:13-19:17 (eigene Sperre, Fremdlast 4-12 Kerne, Leerlauf-Prioritaet), `leistung` Abschnitt e, je ein Lauf, ohne
Teilabgabe (`BHED_D3D12_TEILABGABE=0`: reine Arbeit der Grafikkarte; D3D11 mit Treiber-Teilabgabe), Tabelle `r3/t_g.txt`.
Werte CPU / GPU / Bildabstand in ms.

| Ansicht | D3D11 | D3D12 karte1 | D3D12 neu | neu gegen karte1 |
|---|---|---|---|---|
| Vorlage wie im Spiel ruhend | 5,92 / 3,214 / 6,29 | 3,75 / 2,664 / 4,84 | 3,85 / 2,497 / 4,76 | GPU -6 % |
| Vorlage wie im Spiel, Kamera dreht | 6,11 / 3,264 / 6,48 | 4,14 / 2,221 / 4,98 | 3,76 / 2,060 / 4,55 | GPU -7 % |
| duel_yavin ohne Halme | 5,83 / 3,580 / 6,31 | 3,36 / 3,531 / 5,39 | 3,04 / 3,031 / 4,38 | GPU -14 %, unter D3D11 |
| duel_yavin mit Halmen | 6,45 / 3,958 / 6,89 | 4,05 / 3,902 / 6,55 | 3,27 / 3,290 / 4,89 | GPU -16 %, unter D3D11 |
| md_am_sith ohne / mit Effekten | 4,49 / 2,615 / 5,10, 6,95 / 3,006 / 7,33 | 2,51 / 3,137 / 4,32, 4,72 / 3,269 / 6,61 | 3,66 / 2,651 / 4,95, 6,30 / 2,762 / 7,15 | GPU -15 % |
| Echtzeit-Licht ruhend | 9,10 / 22,962 / 22,99 | 8,12 / 22,687 / 22,73 | 8,51 / 22,827 / 22,87 | gleich, wie D3D11 |
| Echtzeit-Licht, Kamera dreht | 7,92 / 11,936 / 13,47 | 5,23 / 11,833 / 12,52 | 7,06 / 12,336 / 13,28 | GPU +3-4 % (Spanne unter Last) |

Die GPU-Gewinne kommen aus Portaltausch und Spielgamma ins aufgeloeste Bild (Karten mit Himmelsportal und Mehrfachabtastung); der
Abstand zu Direct3D 11 im Echtzeit-Licht (+4-5 % im Ruhefenster des Vorgaengers) ist hier nicht mehr zu sehen. Ruhefenster:
`messung_final.sh` (unveraendert D3D11 gegen D3D12 aus demselben Bau; neu: der Anlauf fuellt pso12.bin, die Laeufe starten warm;
`MPRIO=normal` im Ruhefenster).

**Groesstes Bild beim Optimierer** (duel_mos_espa, Stufe 5 gross, Fremdlast 8 Kerne): D3D12 neu 26 ms, D3D11 28 ms, karte1 40-58 ms.

**Programmstart bis erstes Bild** (Leerlauf-Prioritaet, Fremdlast 4-10 Kerne): D3D12 neu 449-546 ms (ein Ausreisser 1306 ms),
karte1 613-714 ms, D3D11 564-609 ms. Das Geraet entsteht im Faden in 300-380 ms, der Rest des Starts der Schnittstelle sind 46-86 ms
(vorher 167-310 ms).

**Speicher** (MB, Abschnitt e; Arbeitssatz / Grafikspeicher; Spitze privat):

| Stelle | D3D11 | D3D12 karte1 | D3D12 neu |
|---|---|---|---|
| nach der .map | 1894 / 3548 | 3180 / 2753 | 3169 / 2757 |
| mit Echtzeit-Licht | 1879 / 3548 | 2815 / 2587 | 2804 / 2591 |
| Ende | 1765 / 3548 | 2693 / 2587 | 2687 / 2591 |
| Spitze privat (zwei Laeufe je Weg) | 6689-6711 | 7213-7769 | 7215-7343 |

Grafikspeicher: Direct3D 12 braucht 22-27 % WENIGER als Direct3D 11 (der Treiber hielt in diesen Laeufen 3548 MB, im Ruhefenster
des Vorgaengers 2438 MB - die Zahl schwankt mit dem Budget des Systems). Arbeitssatz und privater Speicher sind hoeher: Der Abgeber
lagert ungebrauchte Texturen aus (hier 387-431 Texturen, 1902-2049 MB), Windows haelt deren Inhalt im Arbeitsspeicher des Prozesses.
Ohne Auslagern (`BHED_D3D12_AUSLAGERN=0`, Lauf `d_sp12n`) gleicht der private Speicher Direct3D 11 (6688 gegen 6765 MB), der
Grafikspeicher steigt auf 4258 MB. Der Arbeitssatz schwankt je Lauf um mehrere GB (Windows kuerzt ihn unter Last) und taugt nur im
Ruhefenster. **Waechst der private Speicher in langen Laeufen?** Nein: in jedem Lauf liegt "Ende" unter "nach der .map" (z. B. 7130 ->
6731 MB), im Ruhefenster des Vorgaengers 8114 -> 7977 MB. Die 8-GB-Grenze des TEMPO-Fotolaufs ksemrn ist die Summe aus Abschnitt ks
(Spitze privat 4,0 GB) und dem um 0,5-0,6 GB hoeheren Stand des Karten-Editors durch das Auslagern - kein Leck. Abhilfe fuer die
Pruefung: in Teilen laufen lassen (wie TEMPO es tut). Ob der Abgeber weniger Arbeitsspeicher statt weniger Grafikspeicher halten soll
(`BHED_D3D12_AUSLAGERN=budget`), ist Frage 2.

## Punkt 5: Kanten um 1/255

Versuch p90 (voruebergehend, wieder entfernt): die acht Proben je Bildpunkt und das aufgeloeste Bild der Kamera des Karten-Editors
direkt aus dem Ziel gelesen, je Grafikweg ein `leistung`-Lauf (e). Ergebnis (`proben_vgl.py r3/pr11 r3/pr12`): in allen drei
Kamerabildern (2x 1920x1440, 1364x786) sind **die Proben und das aufgeloeste Bild unter beiden Wegen bitgleich** (0 von 2,76 Mio.
Bildpunkten verschieden). Die Kanten entstehen erst danach, beim Zeichnen des Kamerabilds ins Fenster: Die Kamera loescht mit Alpha 0,
an Dreieckskanten bleibt Alpha zwischen 0 und 1 (im Vorlage-Bild 2365 Bildpunkte), und ImGui mischt diese Bildpunkte mit
SRC_ALPHA/INV_SRC_ALPHA ueber den Fensterhintergrund - genau dort weichen die Fotos um 1/255 ab (1416 Bildpunkte). Das Spielbild
loescht mit Alpha 1 und ist gleich. Ein Fehler des Abgebers ist es nicht (gleiche Eingaben, gleiche Mischart); eine Angleichung
(Kamera ohne Mischen ins Fenster oder Loeschen mit Alpha 1) aenderte das Bild unter beiden Wegen an den Kanten und ist deshalb nicht
gemacht.

## Offen

- Objekte frueherer Geraete in den Zeichenschichten (Puffer/Puffer3/GebackenNetz von karte_gpu, Bildvorraete) werden beim
  Geraetewechsel nicht freigegeben, sondern vergessen ("beim Abbau leben noch 284-1356 Objekte"): ein Leck je Geraeteverlust,
  kein Fehler mehr (Generation). Unter Direct3D 11 dasselbe Verhalten seit rc596.
- Startmarke: nach einem echten Absturz in den ersten 2 s steht die Einstellung danach dauerhaft auf Direct3D 11 (Frage 1).

## Fragen an shank

1. (vom Vorgaenger, weiter offen) Nach einem abgestuerzten Start mit Direct3D 12 stellt das Programm die Einstellung dauerhaft auf
   Direct3D 11 (mit Hinweis im Menue). Soll es beim naechsten Start Direct3D 12 noch einmal versuchen und erst beim zweiten Absturz
   in Folge umstellen?
2. Speicher: Direct3D 12 haelt heute 22-27 % weniger Grafikspeicher als Direct3D 11, dafuer 0,5-0,6 GB mehr Arbeitsspeicher (der
   Inhalt ausgelagerter Texturen). Soll es lieber wie Direct3D 11 mehr Grafikspeicher und weniger Arbeitsspeicher halten (nur unter
   Budgetdruck auslagern)?

## Changelog

## English

- When the graphics card is lost or reset while you work (driver update, waking from sleep), every view comes back by itself: the 2D views of the map editor with brushes and template, the template in the camera and in "Built map as in the game", and the model tab view. With Direct3D 12 the program no longer crashes there.
- Direct3D 12 no longer switches to Direct3D 11 because Windows logged off or shut down, or because the program was closed in its first seconds.
- Direct3D 12 stays smooth when other programs keep the processor busy (for example a map compile): before, frames could take ten times as long as with Direct3D 11.
- Faster start with Direct3D 12: the graphics device is created while the rest loads, and pipeline states are kept on disk.
- Less work for the graphics card with multisampling: the sky portal needs no copy, and the game gamma is drawn straight into the final picture. The picture is unchanged.

## Deutsch

- Geht die Grafikkarte während der Arbeit verloren oder wird zurückgesetzt (Treiber-Update, Aufwachen aus dem Ruhezustand), kommt jede Ansicht von selbst zurück: die 2D-Ansichten des Karten-Editors mit Pinseln und Vorlage, die Vorlage in der Kamera und in "Built map as in the game" und die Ansicht des Model-Tabs. Mit Direct3D 12 stürzt das Programm dort nicht mehr ab.
- Direct3D 12 schaltet nicht mehr auf Direct3D 11 um, weil Windows abgemeldet oder heruntergefahren oder das Programm in den ersten Sekunden geschlossen wurde.
- Direct3D 12 bleibt flüssig, wenn andere Programme den Prozessor auslasten (etwa ein Kartenbau): vorher dauerten Bilder bis zehnmal so lange wie mit Direct3D 11.
- Schnellerer Start mit Direct3D 12: Das Grafikgerät entsteht, während der Rest lädt, und die Pipeline-Zustände liegen auf der Platte.
- Weniger Arbeit für die Grafikkarte mit Mehrfachabtastung: Das Himmelsportal braucht keine Kopie, und das Spielgamma wird gleich ins fertige Bild gezeichnet. Das Bild bleibt gleich.

## 中文

- 工作中显卡丢失或被重置时(驱动更新、从睡眠唤醒),每个视图都会自动恢复:地图编辑器的 2D 视图(含笔刷和模板)、摄像机和 "Built map as in the game" 中的模板,以及模型选项卡的视图。使用 Direct3D 12 时程序在此不再崩溃。
- 因 Windows 注销或关机、或程序在最初几秒内被关闭,Direct3D 12 不再切换到 Direct3D 11。
- 其他程序占满处理器时(例如编译地图),Direct3D 12 仍保持流畅:此前每帧耗时可达 Direct3D 11 的十倍。
- Direct3D 12 启动更快:图形设备在其余部分加载时创建,管线状态保存在磁盘上。
- 多重采样时显卡工作更少:天空门户无需复制,游戏伽马直接绘制到最终画面。画面不变。

## 日本語

- 作業中にグラフィックカードが失われたりリセットされたりしても(ドライバー更新、スリープからの復帰)、すべてのビューが自動的に戻ります。マップエディターの 2D ビュー(ブラシとテンプレート)、カメラと "Built map as in the game" のテンプレート、モデルタブのビューです。Direct3D 12 ではここでクラッシュしなくなりました。
- Windows のサインアウトやシャットダウン、または起動直後の数秒でプログラムを閉じたことで、Direct3D 12 が Direct3D 11 に切り替わることはなくなりました。
- 他のプログラムがプロセッサーを占有していても(マップのコンパイルなど)Direct3D 12 は滑らかなままです。以前はフレームに Direct3D 11 の最大 10 倍の時間がかかることがありました。
- Direct3D 12 の起動が速くなりました。残りを読み込んでいる間にグラフィックデバイスを作成し、パイプライン状態をディスクに保存します。
- マルチサンプリング時のグラフィックカードの作業が減りました。スカイポータルにコピーが不要になり、ゲームのガンマは最終画像に直接描画されます。画像は変わりません。

## Español

- Si la tarjeta gráfica se pierde o se reinicia mientras trabajas (actualización del controlador, salida de la suspensión), cada vista vuelve sola: las vistas 2D del editor de mapas con pinceles y plantilla, la plantilla en la cámara y en "Built map as in the game", y la vista de la pestaña Model. Con Direct3D 12 el programa ya no se cierra ahí.
- Direct3D 12 ya no pasa a Direct3D 11 porque Windows cerró la sesión o se apagó, o porque el programa se cerró en sus primeros segundos.
- Direct3D 12 sigue fluido cuando otros programas ocupan el procesador (por ejemplo, al compilar un mapa): antes los fotogramas podían tardar diez veces más que con Direct3D 11.
- Inicio más rápido con Direct3D 12: el dispositivo gráfico se crea mientras se carga el resto, y los estados de canalización se guardan en disco.
- Menos trabajo para la tarjeta gráfica con multimuestreo: el portal del cielo no necesita copia y el gamma del juego se dibuja directamente en la imagen final. La imagen no cambia.

## Čeština

- Když se grafická karta během práce ztratí nebo resetuje (aktualizace ovladače, probuzení z režimu spánku), každý pohled se sám obnoví: 2D pohledy editoru map se štětci a předlohou, předloha v kameře a v "Built map as in the game" a pohled karty Model. S Direct3D 12 už program na tomto místě nespadne.
- Direct3D 12 se už nepřepne na Direct3D 11 kvůli odhlášení nebo vypnutí Windows ani kvůli zavření programu v prvních sekundách.
- Direct3D 12 zůstává plynulé, i když jiné programy vytěžují procesor (například sestavení mapy): dříve mohly snímky trvat až desetkrát déle než s Direct3D 11.
- Rychlejší start s Direct3D 12: grafické zařízení vzniká, zatímco se načítá zbytek, a stavy pipeline se ukládají na disk.
- Méně práce pro grafickou kartu při vícenásobném vzorkování: portál oblohy nepotřebuje kopii a herní gama se kreslí přímo do výsledného obrazu. Obraz zůstává stejný.
