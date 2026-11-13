# Notizen AP11 (13.11.): Modelle in Tabs, Kopieren/Einfügen von Flächen, „In neuem Tab öffnen“ - rc612

Auftrag AP11 (Opus 5.5) aus `auftraege/auftrag_mbentwurf_b_1011.md` und `auftraege/welle_1211.md` (shank 04.11.: „Kopieren/Einfügen,
Modelle in Tabs“), Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md` Zeilen **R5**, **X19**, **P1**.
Worktree `Y:\p_ap7`, Zweig `p_ap11`, begonnen auf karte1 `68a029b87`, gesetzt auf karte1 `f0d55d90a` (D3D12-3), zuletzt auf
`8ccba4a3c` (REST612; keine Einstellungsschlüssel von AP11, kein Konflikt in `settings.cpp`). Selbsttest `gui/selbsttest_ap11.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap11 | ap11foto | ap11hb`), ctest `modell_ablage`, Skripte und Läufe `Y:\werkzeuge\ap11\`.

## Was sich ändert

Neue Dateien: `gui/app_modelltabs.cpp` (Reiter, Wechsel, Schließen, Laden in neuem Reiter, Ablage, Reiterzeile; Schnittstelle im eigenen
Block am Ende von `gui/modelltab.h`), `include/bhed/modell/ablage.h` + `src/modell/ablage.cpp` (Rechenteil der Ablage),
`tests/modell_ablage.cpp`, `tools/i18n_ap11.py` (Präfix `Mx`, 6 Sprachen), `tools/handbuch/inhalt_ap11.py`, `gui/selbsttest_ap11.inc`.

| Zeile | Was | Datei |
|---|---|---|
| R5 | Über den Ansichten je geöffnetem Modell ein Reiter („kyle/model.glm“, ImGui-Reiterleiste), x schließt (ungespeichert: die Frage wie heute, der Reiter wird dafür aktiv; der letzte wird leer), „+“ öffnet (Modelldatei, Spieldateien), Rechtsklick „Reiter schließen“, „Andere Reiter schließen“, Tooltip mit dem ganzen Pfad (und „Ungespeicherte Änderungen“), Punkt für Ungespeichertes. Rechts der Hinweis „Ablage: torso aus kyle/model.glm · Strg+V fügt ein“ (höchstens 45 % der Zeile, gekürzt mit Tooltip). Je Modell bleibt alles: Datei, GLA/Animation/Bild, Auswahl (auch je Detailstufe), Rückgängig, Modifier-Stapel und Begleitdatei (Augen, geladener Inhalt, Drehmodus), Rig/Control Rig, Bemalung (Leinwände samt Ebenen), Einfrieren, Sperren, Auto-Skin-Ergebnis, Zeitleiste (Pose ohne Schlüssel), Kamera, Ansichten, Ansichtsverlauf, vier Ansichten je Bereich. Ein schon offenes Modell öffnet nicht doppelt. Beenden fragt je Modell. | `app_modelltabs.cpp`, Reiterzeile in `drawModellArbeitsflaeche` (`app_modelltab_ansicht.cpp`), `modellReiterSchritt` in `gui::draw` (`app.cpp`), Beenden (`app_modelltab_mb3.cpp`) |
| X19 | Rechtsklick einer Fläche wie im Entwurf: Info · Kopieren (Strg+C) · Einfügen als Kind hiervon (Strg+V) · Status · Wurzel · Wurzel lösen · Alle „_off“-Flächen aus / an · Alle auf Ursprungszustand (+ „Alle Flächen an“ wie bisher). Einfügen: Gewichte nach Knochennamen (G2_Find_Bone: ohne gross/klein), fehlender Knochen → nächster vorhandener Elternknochen (Summe je Ecke bleibt, Meldung nennt jeden), alle Detailstufen (überzählige fallen weg, fehlende mit der gröbsten gefüllt, Meldung), Shader = Bild, das die Quelle zeigte, Modifier-Stapel mit neuen Kennungen, Namen _1/_2 (höchstens 63 Zeichen), Grenzen des Zielspiels (Ecken, Dreiecke, Knochen je Fläche, Gewichte je Ecke) in der Meldung, ein Rückgängig-Schritt im Ziel, in eine .md3 starr. | `modellAblageMenue` (`app_modelltabs.cpp`) in `flaechenMenue` (`app_modelltab_explorer.cpp`), `bhed/modell/ablage.h` |
| P1 | Gruppe Modell: „In neuem Tab öffnen“ (Strg+O) neben „Importieren …“ (Menü Modelldatei / Spieldateien); Strg+O öffnet in einem neuen Reiter, Alt+O und „Importieren …“ ersetzen wie bisher das Modell des aktiven Reiters; Datei-Menü des Model-Tabs mit beiden. | `rolloutModell` (`app_modelltab.cpp`), `modellDateiTasten`/`modellDateiMenue` (`app_modelltab_mb2.cpp`) |
| Kleinkram | `PrTempoHint` entfernt (unbenutzt; den Tooltip des Tempo-Faktors trägt seit ENT3B `E3TempoHint` mit denselben Einheiten je Sekunde). | `tools/i18n_rt.py` |

### Aufbau des Wechsels (Speicher- und Absturzsicherheit)

- Das aktive Modell arbeitet wie bisher in `App` (model, modelPath, anim, animList …, modelTextures, modellWelt, mtab) und in den Zuständen
  der Module. Jeder andere Reiter hält seinen Teil in einem `AppTeil` und einem Fach je Modul (`ModellFach`, 13 Tausch-Funktionen). Der
  Wechsel tauscht beides mit `std::swap` - nichts wird kopiert oder neu geladen, Zeiger in Heap-Daten wandern mit ihrem Modell.
- Was ein Modul nur aus dem Modell ableitet (Pose, Haut, Gewichtsbild, Bilder der vier Ansichten, Stapel-Zeichnung, Spiegelnetz, Formen
  der Knochen, Kurvenpunkte, Grenzen-Liste, erkannte Vorlage …), verwirft es im Tausch. `dateiVon` des Stapels (Adresse des Dokuments)
  zeigte nach dem Tausch auf dasselbe Objekt mit anderem Modell - verworfen.
- Stand, Ladestand, Netzstand und Schrittkennung der Dokumente zählen seit AP11 für alle gemeinsam (`src/modell/dokument.cpp`): zwei
  Modelle haben nie denselben Stand, jeder Zwischenspeicher, der sich etwas nach dem Stand merkt, rechnet nach dem Wechsel neu.
- Gewechselt, geschlossen und in einem neuen Reiter geladen wird nur am Anfang eines Bilds (`modellReiterSchritt` aus `gui::draw`),
  nie mitten im Bild, in dem Zeichner und Ereignisse noch Indizes auf das alte Modell halten; offene Menüs schließen dabei, Fragen
  (Modale) bleiben. Vor dem Tausch enden offene Striche/Züge (übernommen) und G/R/S, Kästchen, Regler (abgebrochen wie Esc).
- Der Puffer der Grafikkarte hängt an der Wache des GlmModel (rc610 R1): std::swap meldet beide Adressen, die Puffer bauen neu.

## Selbsttest (rot -> grün)

Roter Lauf = Selbsttest und Bausteine ohne die Anschlüsse (WIP-Commit „building blocks and self test (red state, not connected)“).

| Lauf | Ergebnis |
|---|---|
| `ap11` rot (`laeufe/rot_ap11.out`) | 19 OK / 51 FEHLER (jede Prüfzeile der drei Zeilen rot; grün nur die Invarianten „Zwischenspeicher passen zum gezeigten Modell“, die mit einem Modell ohnehin gelten, und Zeilen, die nur mit einem Reiter zufällig stimmten - diese verlangen seither den Wechsel bzw. das Schließen und sind im roten Stand ebenfalls rot) |
| `ap11` erster grüner Lauf (`g1`) | 53 / 14 - Funde unten (Testfehler und eine Programmänderung) |
| `ap11` grün vor dem Rebase (`g2`) | 64 / 0 |
| `ap11` grün nach dem Rebase auf karte1 `f0d55d90a` (`g3`, mit Gerätverlust R1) | 67 / 0 |
| ctest `modell_ablage` | 31 Prüfzeilen grün (neuer Rechenteil, mit dem Modul geschrieben) |

### Funde im ersten grünen Lauf (behoben)

- Die Fragen beim Schließen und Beenden nannten nur `model.glm` (`modellFrageName`, `fileName`) - mit mehreren Modellen heißen fast alle
  so. Jetzt Ordner und Datei wie der Reiter („kyle/model.glm“); ebenso die Zeile „Datei“ der Gruppe Modell (Entwurf: „kyle/model.glm“).
- Testfehler: jedi_hm hat keine Fläche „torso“/„hips“/„head“ (der Test wählt jetzt eine passende Fläche), vier Ansichten setzt
  `modellPrSchritt` beim Bereichswechsel nach der Wahl je Bereich (der Test setzt sie nach dem Wechsel), Zahl der Reiter bei Strg+O.
- Prüfzeilen, die im roten Stand nur mit einem Reiter zufällig stimmten, verlangen seither den Wechsel bzw. das Schließen.

### Gerätverlust mit mehreren offenen Modellen (Hinweis Hauptchat, wie R1 von D3D12-3)

Zwei Reiter offen, Bild beider Modelle, `render::geraetVerlorenSpielen()`, Neuaufbau: die aktive Ansicht (kyle) 0 von 1984 x 1353
Bildpunkten anders, nach dem Wechsel das Modell, das während des Verlusts im Fach lag (jedi_hm), ebenso 0 anders; Abgeber ohne Fehler.
Grund: Texturen (`TextureSet::Tex`) und Puffer eines abgelegten Modells laden beim nächsten Zeichnen neu (Zwischenspeicher nach
Adresse und `render::texturStand()`), die Ansicht zeichnet nach dem Wechsel ohnehin neu (Stand der Ansicht geleert). Die Zeile von
D3D12-3 (Gerätestand im Stand der Ansicht) gilt damit für jedes Modell; sie stand vor dem Rebase nicht im Zweig, die Prüfzeile wurde
nach dem Rebase geschrieben und war gleich grün (kein Fehler zum Nachstellen gefunden).

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

- `ap3` F5 und `ap5` P1 zählten in der Gruppe Modell genau drei Knöpfe (Andere GLA, Texturen neu laden, Importieren); der Entwurf (P1)
  verlangt den vierten „In neuem Tab öffnen“ (Strg+O) neben „Importieren …“. Beide prüfen jetzt vier Knöpfe mit Symbol, zwei mit
  Taste, den vierten in derselben Zeile rechts neben „Importieren …“, gleich breit (rot in der Runde mit drei, grün angepasst).

## Schlussrunde des Zweigs

Bau mit `--parallel 2 -- /nodeReuse:false`, Läufe auf Leerlauf-Priorität, höchstens zwei Programme gleichzeitig.

| Was | Ergebnis |
|---|---|
| Bau (vor dem letzten Rebase `b13`, nach dem Rebase auf `8ccba4a3c` `b14`, alle Ziele) | 0 Warnungen |
| alle 74 modellblock-Läufe von `tools/testrunde.py` unter Direct3D 12 (`ap11s_summe.txt`) | 63 grün, 11 rot: 9 schon auf karte1 rot (unten), `ap3`/`ap5` (vierter Knopf, oben angepasst, danach 106 / 0 und 121 / 0) |
| Gruppen außerhalb der Tabelle: `ent3a`, `ent3b`, `fix612`, `fix612b`, `fix612babsturz`, `fix612c` | 56 / 0, 38 / 0, 27 / 0, 39 / 0, 3 / 0, 36 / 0 |
| Zeichengruppen unter Direct3D 11: `ap11`, `kopf`, `raster`, `ap4`, `ap5`, `ap13k`, `pt`, `bild_md` | 8 grün (`ap11` 69 / 0) |
| `ap11` unter Direct3D 12 in der Runde | 70 / 0 |
| nach dem Rebase auf `8ccba4a3c` (Bau `b14`): `ap11` unter Direct3D 12 / 11 | 70 / 0, 69 / 0 |
| ctest (Bau `b14`, `-j 2`) | 273 / 273 (mit `modell_ablage`, `einstellungen_alle`, Handbuch- und i18n-Prüfungen) |
| Lints (Stand des Zweigs) | wie karte1: pruefcodes 11, portability 101, efxflags und fremdprogramm wie bisher, alle übrigen ok |

**Schon auf karte1 rot** (Grundlinie: dieselben Läufe mit dem Bau von `Y:\p_dx12` = karte1 + Einstellungstabelle, nur lesend
benutzt; jeweils dieselbe Prüfzeile rot): `mb_rig` (MB4 Pfad: 84 statt 21 Punkte je Bild), `mb_mb6` (2: Glied auf FK ab Frame +24),
`mb_mb5` (Pfad ziehen bei +8), `mb_ptfix3` (P24 Umbacken nach Detach), `mb_ptsprung` (Texel nach `ptsprung_flaeche.bin`), `mb_pfix`
(H2 Vorschau mit neuem Teil), `mb_rrl_md`/`mb_rrl_jka`/`mb_rrl_jk2` (Rig-Rundlauf: Figuren mit Rig + ohne Rolle ≠ alle Figuren). Mit
der zuerst immer sichtbaren Reiterzeile war `mb_mb5` zusätzlich rot (Rechtsklick der Ansicht) - mit der Zeile nur bei Bedarf wieder
wie auf karte1.

## Fotos (angesehen, neben dem Entwurfsbild)

`Y:\werkzeuge\ap11\fotos\ap11_f_<variante>\` (en, de, ja, hell, klein = 1280 x 720 bei Oberfläche 1,0, d3d11; je wie 3ds Max und wie
Blender: Reiterzeile mit Ablage, Rechtsklick der Fläche, Menü von „+“), Entwurf darüber in `Y:\werkzeuge\ap11\neben\`. Reiterzeile,
„+“, Hinweis der Ablage rechts, Menü der Fläche und Gruppe Modell stehen wie im Entwurf; Blender: die Zeile rechts der Werkzeugleiste
über der Ansicht wie im Entwurf. Direct3D 11 gegen 12: 0 Bildpunkte über 8/255 anders (je 1 Bildpunkt um ≤ 8). Bei Mindestgröße gehen die
Reiter vor: der Hinweis nimmt, was sie übrig lassen (höchstens 45 %), sonst steht er im Tooltip von „+“ (vorher kürzte er die Namen
der Reiter auf „kyl…“). Japanisch bricht „新しいタブで開く…“ im Knopf mit Taste zwischen den Zeichen um wie „テクスチャを再読み込み“.

## Handbuch

`tools/handbuch/inhalt_ap11.py` (6 Sprachen): Abschnitt „Mehrere Modelle in Reitern“ auf modell-ansehen hinter dem Absatz „aufbau“
(vor dem Szenen-Explorer), Bilder `h_ap11_max_reiter`, `h_ap11_blender_reiter`, `h_ap11_max_menue` (2538 x 1472, je Sprache aus
`BHED_MODELL_NUR=ap11hb`), eine Zeile in QUELLEN (`Anhang("inhalt_ap11", "modell-ansehen")`). Handbuch und `src/hilfe_orte.inc` neu
erzeugt (der Knopf „In neuem Tab öffnen“ ist ein Hilfe-Ort der Gruppe Modell). Für AP14b: jedes vorhandene Bild des Model-Tabs
bleibt, wie es ist (mit einem Modell und leerer Ablage keine Reiterzeile); neu aufzunehmen ist durch AP11 kein vorhandenes Bild.

## Entscheidungen (mit Grund)

- Strg+O öffnet in einem neuen Reiter (Entwurf P1), Alt+O (BehavEd „&Open“) und „Importieren …“ ersetzen wie bisher das Modell des
  aktiven Reiters; ist der aktive Reiter leer und unverändert, lädt „In neuem Tab öffnen“ dort.
- Strg+C / Strg+V gelten im Bereich Animation weiter der Pose (wie Blender pose.copy/paste, schon so belegt); dort kopiert der
  Rechtsklick der Fläche.
- Fehlt im Ziel ein Knochen, bekommt der nächste vorhandene Elternknochen sein Gewicht (die Summe je Ecke bleibt, die Meldung nennt
  jeden) statt abzulehnen; hat das Ziel (eine .glm) kein Skelett geladen, lehnt Einfügen mit Meldung ab (ohne Namen keine Zuordnung).
- Die Reiterzeile steht, sobald sie gebraucht wird: ab dem zweiten Modell oder mit gefüllter Ablage (dann mit dem Hinweis). Mit einem
  Modell und leerer Ablage sieht der Model-Tab aus wie bisher - so verlangt es der Auftrag („Bilder und alle vorhandenen Selbsttests
  laufen mit einem Modell genauso wie heute“), und „Platz nur, wenn gebraucht“. Ein erster Stand mit immer sichtbarer Zeile machte die
  Ansicht eine Zeile niedriger; in der Runde fiel `mb5` (Rechtsklick der Ansicht) zusätzlich rot. Der Entwurf zeigt nur den Fall mit
  zwei Modellen. Ein weiteres Modell öffnet man mit einem Modell über „In neuem Tab öffnen“ (Gruppe Modell), Strg+O, das Datei-Menü
  oder die Suche.
- Nicht doppelt: ein schon offenes Modell wechselt in seinen Reiter (Meldung), statt einen zweiten mit demselben Pfad zu öffnen.

## Neuer Stand der Abgleichzeilen

- **R5** gleich: Reiter je Modell über der Ansicht („kyle/model.glm“), x schließt, „+“ öffnet (Strg+O), rechts der Hinweis der Ablage,
  Flächen zwischen den Modellen kopieren. Bewusst anders: die Zeile steht erst ab dem zweiten Modell oder mit gefüllter Ablage (Auftrag:
  mit einem Modell wie heute; „Platz nur, wenn gebraucht“); bei Mindestgröße steht der Hinweis im Tooltip von „+“.
- **X19** gleich: Info, Kopieren (Strg+C), Einfügen als Kind hiervon (Strg+V), Status an / aus / aus + ohne Kinder, Als Wurzel setzen,
  Wurzel lösen, Alle „_off“-Flächen aus / an, Alle auf Ursprungszustand; mehr als der Entwurf wie bisher „Alle Flächen an“ und
  Einfrieren. Die Titelzeile „Fläche torso“ des Entwurfsbilds hat kein Menü des Baums (offen, klein, alle Menüs gleich).
- **P1** gleich: Kurzinfo „n Dreiecke“, Datei „kyle/model.glm“, Raster Andere GLA · Texturen neu laden (Alt+R) · Importieren … ·
  In neuem Tab öffnen (Strg+O). Offen mit Grund: Modell-Import aus .xsi/.fbx/.obj/.glb (eigene Arbeit, Stufe 1d/2, Abgleichtabelle),
  die Zeile GLA zeigt den Pfad statt „_humanoid.gla“ (nicht Teil dieser Zeile, unverändert).

## Offen / Fragen an shank

- (keine Fragen) - Hinweis: die neun schon auf karte1 roten modellblock-Läufe (oben) gehören keinem AP11-Teil.

## Changelog

## English

- Model tab: several models in tabs above the view - each tab keeps its file, animation, selection, undo history, modifier stack, rig, painting, timeline and views; "+" and Ctrl+O open another model, the x closes a tab and asks about unsaved changes, exit asks for every model.
- Copy surfaces from one model into another: Copy (Ctrl+C) and "Paste as child of this" (Ctrl+V) in the right-click menu of a surface; weights go to the bones of the same name (a missing bone hands its weight to the nearest parent bone and the message names it), LODs, shader and modifier stack come along, and the limits of the game are checked.
- Model group: "Open in new tab" (Ctrl+O) next to "Import ...".

## Deutsch

- Model-Tab: mehrere Modelle in Reitern über der Ansicht - jeder Reiter behält Datei, Animation, Auswahl, Rückgängig-Verlauf, Modifier-Stapel, Rig, Bemalung, Zeitleiste und Ansichten; „+“ und Strg+O öffnen ein weiteres Modell, das x schließt einen Reiter und fragt bei ungespeicherten Änderungen, beim Beenden fragt jedes Modell.
- Flächen von einem Modell in ein anderes kopieren: Kopieren (Strg+C) und „Einfügen als Kind hiervon“ (Strg+V) im Rechtsklick-Menü einer Fläche; Gewichte gehen an die gleichnamigen Knochen (ein fehlender gibt sein Gewicht an den nächsten Elternknochen, die Meldung nennt ihn), Detailstufen, Shader und Modifier-Stapel kommen mit, die Grenzen des Spiels werden geprüft.
- Gruppe Modell: „In neuem Tab öffnen“ (Strg+O) neben „Importieren …“.

## 中文

- Model 标签: 视图上方以标签页显示多个模型 - 每个标签页保留自己的文件、动画、选择、撤销历史、修改器堆栈、绑定、绘制、时间轴和视图; “+”和 Ctrl+O 打开另一个模型, x 关闭标签页并在有未保存更改时询问, 退出时每个模型都会询问。
- 将面从一个模型复制到另一个模型: 面的右键菜单中的“复制”(Ctrl+C) 和“粘贴为此项的子项”(Ctrl+V); 权重转到同名骨骼 (缺少的骨骼把权重交给最近的父骨骼, 消息会说明), LOD、着色器和修改器堆栈一并带入, 并检查游戏的限制。
- “模型”组: “导入...”旁的“在新标签页中打开”(Ctrl+O)。

## 日本語

- Model タブ: ビュー上部のタブで複数のモデルを扱えるように - 各タブはファイル、アニメーション、選択、元に戻す履歴、モディファイアスタック、リグ、ペイント、タイムライン、ビューを保持。「+」と Ctrl+O で別のモデルを開き、x でタブを閉じ (未保存の変更があれば確認)、終了時はモデルごとに確認します。
- サーフェスを別のモデルにコピー: サーフェスの右クリック メニューの「コピー」(Ctrl+C) と「これの子として貼り付け」(Ctrl+V)。ウェイトは同名のボーンへ (ないボーンは最も近い親ボーンにウェイトを渡し、メッセージに表示)、LOD、シェーダー、モディファイアスタックも移り、ゲームの制限を確認します。
- 「モデル」グループ: 「インポート...」の隣に「新しいタブで開く」(Ctrl+O)。

## Español

- Pestaña Model: varios modelos en pestañas sobre la vista; cada pestaña conserva su archivo, animación, selección, historial de deshacer, pila de modificadores, rig, pintura, línea de tiempo y vistas; «+» y Ctrl+O abren otro modelo, la x cierra una pestaña y pregunta si hay cambios sin guardar, al salir pregunta cada modelo.
- Copiar superficies de un modelo a otro: Copiar (Ctrl+C) y «Pegar como hijo de esta» (Ctrl+V) en el menú contextual de una superficie; los pesos van a los huesos del mismo nombre (un hueso que falta pasa su peso al hueso padre más cercano y el mensaje lo nombra), vienen los LOD, el sombreador y la pila de modificadores, y se comprueban los límites del juego.
- Grupo Modelo: «Abrir en pestaña nueva» (Ctrl+O) junto a «Importar...».

## Čeština

- Karta Model: více modelů na kartách nad pohledem - každá karta si ponechá soubor, animaci, výběr, historii zpět, zásobník modifikátorů, rig, malbu, časovou osu a pohledy; „+“ a Ctrl+O otevřou další model, x zavře kartu a při neuložených změnách se zeptá, při ukončení se zeptá každý model.
- Kopírování ploch z jednoho modelu do jiného: Kopírovat (Ctrl+C) a „Vložit jako potomka této“ (Ctrl+V) v nabídce pravého tlačítka plochy; váhy přejdou na stejnojmenné kosti (chybějící kost předá váhu nejbližší rodičovské kosti a zpráva ji uvede), přejdou i LOD, shader a zásobník modifikátorů a kontrolují se limity hry.
- Skupina Model: „Otevřít na nové kartě“ (Ctrl+O) vedle „Importovat...“.
