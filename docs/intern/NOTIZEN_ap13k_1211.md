# Notizen AP13K (12.11.): Model-Tab, Knochen-Darstellung (R14), Durchsicht, Knochen im Vordergrund, Rig-Stile - MBENTWURF Teil B, rc612

Auftrag `auftraege/welle_1211.md` Abschnitt 3 nach `auftrag_mbentwurf_b_1011.md`, Zeilen **R14**, **P9** (Alt+X) und der Alt+Z-Teil von
**P10** der Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md`, dazu shanks Wunsch vom 12.11. „Rig-Darstellung
wählbar (3 Vorbilder)“ (`naechste_auftraege.md`). Begonnen von Fable 5.2 (WIP 2fee19957 Rot-Stand, be0aa282e Umsetzung), fortgesetzt und
abgeschlossen von Opus 5.5. Worktree `Y:\p_ent2`, Zweig `p_ap13k`, rebased auf karte1 `6f79e39f9` (mit AP13S). Selbsttest `gui/selbsttest_ap13k.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap13k | ap13kfoto`; Handbuch `BHED_HB_NUR=ap13k`), ctest `modell_rigformen`, Skripte,
Läufe, Vorbilder und Blender-Werte unter `Y:\werkzeuge\ap13k\`, Rig-Daten `Y:\werkzeuge\rigs\*.json`.

## Was sich ändert

Neue Dateien: `include/bhed/modell/rigformen.h` + `src/modell/rigformen.cpp` (die Formen der Stile in der Grundstellung, ohne GUI),
`gui/app_modelltab_knochen.h/.cpp` (Szene der Formen, Projektion, Treffen, Menüs, Suche, Tasten, Gruppe), `gui/gpumap_formen.inc`
(Durchgang „Formen“ und Durchsicht im Zeichner), `tests/modell_rigformen.cpp`, `tools/i18n_ap13k.py` (Präfix `Kd`, 6 Sprachen),
`tools/handbuch/inhalt_ap13k.py`. Einstellungen `modelBoneDisplay`, `modelRigStyle`, `modelBonesInFront` (Vorgabe an).

| Punkt | Was | Quelle / Regel |
|---|---|---|
| R14 wie 3ds Max | Knochen als Bone-Kasten (verjüngt, gefüllt wie im Entwurf), „Bone fins“ schaltbar | bis rc611 schon so, jetzt im Bild der Ansicht statt als Linie der Überlagerung |
| R14 wie Blender | „Anzeigen als“: Oktaeder (Vorgabe), Stick, B-Bone, Hülle, Draht; Overlays-Menü, Rechtsklick, Gruppe, Aktionssuche; gewählter Knochen hell wie Blenders Pose | Blender 5.0 ohne Fenster ausgelesen (`blender_werte.txt`: display_type, Themafarben) |
| Im Vordergrund | Knochen und Griffe liegen ab Werk vor der Figur (wie bis rc611); aus: die Figur verdeckt sie | Blender `show_in_front` |
| Durchsicht P9 / Alt+Z P10 | Alt+X wie 3ds Max (See-Through der Figur, alle Ansichten), Alt+Z wie Blender (X-Ray je Ansicht); Deckkraft 0,5; Menü der Schattierung, Gruppe, Suche | Blender `xray_alpha` 0,5 ausgelesen |
| Rig-Stile | Standard (Entwurf, Vorgabe), Control Rig (Ringe, nach shanks SmartAnim-Bild), CAT-Kästen (`cat_rig.json`: Maße, Farben, Fußplatten, IK-Ziele, Bodenform), Blender-Oktaeder (`blender_rig.json`: Oktaeder, Kreise/Quadrate, IK-Ziele, Bodenkreis); Griffe nach Seite gefärbt (A10); IK/FK am Glied | Vorbilder `Y:\werkzeuge\ap13k\vorbild\`, eigene Grafik |
| Treffen | Klick und Ziehen treffen die sichtbare Form (Ring, Kante und Inneres eines Kastens, Kreis des Pols, Oktaeder, Fußplatte); die leere Mitte eines Rings trifft nicht | - |

Die Formen der neuen Stile und alle Knochen stehen im Bild der Ansicht (Zeichner), nicht mehr in der ImGui-Überlagerung - nur so kann die Figur
sie verdecken und die Durchsicht sie tönen. Der Stil ändert nur das Aussehen (S5: Pose, Dokument und Schlüssel gleich in allen vier Stilen).

## Funde (rot nachgestellt, behoben, grün)

1. **Formen doppelt gemischt mit Durchsicht** (Opus): mit Durchsicht und „Im Vordergrund“ aus zeichnete der Zeichner die Formen einmal vor der
   Kopie (hinter der Figur getönt) und danach noch einmal mit Tiefentest - neben der Figur lag jede halb deckende Form zweimal übereinander.
   Rot: neue Zeile V4 `ap13k_v4rot` wie 3ds Max 4442 Bildpunkte von Formen neben der Figur, 3244 anders. Behoben: das Mischen der
   Durchsicht setzt ein Stencil-Bit an der Figur, der zweite Durchgang zeichnet nur dort (`gpumap_formen.inc`, `FormTiefe::NurAufFigur`).
   Grün: siehe Schlussrunde.
2. **Tooltip fehlte** an den Untermenüs „Anzeigen als“ und „Rig-Stil“ (Opus): rot `lint_i18n` 1 Beanstandung (`KdAnzeigenAlsHint` nie benutzt),
   grün nach dem Einhängen 0. Der Tooltip zeigt sich nur, solange das Untermenü zu ist (im ersten Handbuchbild verdeckte er die Arten).
3. **Fotos zu klein** (Opus): der Fotolauf ging für „ganze Figur“ auf alles Einpassen (mit Boden) und 1,35-faches Sichtfeld - die Figur war ein
   Zehntel der Ansicht hoch. Der Schritt entfällt, die Figur füllt die Ansicht wieder.
4. Aus Fables letzten Läufen (g2 60/3 unter Direct3D 12, 59/3 unter 11): drei Testerwartungen falsch gestellt (Pole nach Gestalt statt nach Art
   gezählt - der Säbelgriff ist auch ein Kreuz; Farbe an Griffen gemessen, die unter IK zurücktreten; Klick in den Unterschenkel-Kasten, vor dem
   die Fußplatte des anderen Beins liegt). Fable hat sie berichtigt und die Zurücktreten-Prüfung als eigene Zeile ergänzt; Linienbreite mit der
   Oberfläche, Bodenform von CAT als Band. Bestätigt grün im ersten Opus-Lauf (`ap13k_h1` 63/0).

## Selbsttest (rot -> grün)

| Lauf | Ergebnis |
|---|---|
| `ap13k_rot` (Fable, Stand 2fee19957 mit Stubs) | 12 OK / 76 FEHLER |
| `ap13k_g1`, `ap13k_g2`, `ap13k_g2d11` (Fable) | 55/9, 60/3, 59/3 |
| `ap13k_h1` / `ap13k_h1d11` (Opus, be0aa282e) | 63 / 0 unter Direct3D 12 und 11 (dieselben Zeilen; D3D12 dazu die Abgeber-Zeile) |
| `ap13k_v4rot` (V4 neu, Zeichner alt) | 65 / 1 (V4 wie 3ds Max) |
| `ap13k_j1` / `ap13k_j1d11` (Fix V4) | 66 / 0 und 65 / 0, V4 0 von 4442 anders |
| Schluss nach dem Rebase `ap13k_m1` / `ap13k_m1d11` | 66 / 0 (Direct3D 12) und 65 / 0 (Direct3D 11) |

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

- `selbsttest_ap10.inc` A14: die erwartete Folge der Gruppen im Bereich Animation (alle Gruppen; wie Blender Reiter Daten) enthält die neue
  Gruppe „Knochen- und Rig-Darstellung“ hinter den Bewegungspfaden. Rot im Nachbarlauf `n1_mb_ap10` 69 / 3 (genau diese drei Zeilen),
  grün siehe Schlussrunde.

## Fotos

`ap13kfoto` (25 Bilder 3818 × 2104 je Grafikweg): Direct3D 12 gegen 11 Bild für Bild, größte Abweichung 1 von 255, 0 Bildpunkte über 2.
Angesehen neben den Vorbildern (`vorbild\z029232_0/1/2`) und dem Entwurf (`Y:\werkzeuge\mbe\fotos\entwurf\e_*_anim.png`): Control Rig mit
Ringen um Kopf, Hals, Brust, Becken, Fußplatten, Polkugeln, IK/FK am Glied, Bodenkreis; CAT mit Kästen, Hand-IK-Quadraten, Fußplatten, Kreuzen
der Pole, Bodenform; Blender mit Oktaedern, Kreisen und Quadraten, Bodenkreis.
Nach dem Rebase noch einmal de, ja, hell, klein (1350 × 900 bei Oberfläche 1,0), je 2 / 0, angesehen: Texte passen (ja: „コントロールリグ
(リング)“ bricht im Segment um, nicht abgeschnitten), im hellen Thema sind die Kästen durch die getönte Figur zu sehen. Die Figur füllt
nach Fund 3 die Ansicht von schräg vorn wie in den Vorbildern.

## Handbuch

Eigene Quelle `tools/handbuch/inhalt_ap13k.py` (6 Sprachen): Seite „Model-Tab: Knochen, Rig-Stile und Durchsicht“; wie AP4-AP10 nicht
eingehängt - AP14 führt zusammen (Anleitung im Kopf der Datei). Bilder `h_ap13k_arten`, `h_ap13k_durchsicht`, `h_ap13k_control`,
`h_ap13k_cat`, `h_ap13k_blender`. In 6 Sprachen aufgenommen (`hb.sh`, je 2 / 0, 2538 × 1472), angesehen (de Anzeigen als > Stick, ja Control Rig in vier Ansichten).

## Schlussrunde des Zweigs

Auf dem Stand nach dem Rebase auf karte1 `6f79e39f9` (mit AP13S):

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX, `--parallel 2`) | 0 Fehler, 0 Warnungen |
| ctest | 259 / 259 vor dem Rebase; danach `-R modell|i18n|handbuch|lint` 34 / 34 (darin `modell_rigformen`, `modell_stapel`) |
| `ap13k` | 66 / 0 (Direct3D 12), 65 / 0 (Direct3D 11) - Schlusslauf `ap13k_s1`, `ap13k_s1d11` |
| `ap13kfoto` | en unter beiden Wegen bildgleich; de, ja, hell, klein je 2 / 0, angesehen |
| Handbuchbilder | 6 Sprachen je 2 / 0 |
| Nachbargruppen Direct3D 12 | funktionen 63 · tasten 45 · rig 28 · blender 69 · raster 42 · kopf 117 · ent2 23 · ap4 106 · ap5 121 · ap13s 123 · ap10 72 (nach der Anpassung; vorher 69 / 3) - je 0 FEHLER |
| Nachbargruppen Direct3D 11 | raster 41 · kopf 116 - je 0 FEHLER |
| Lints (`lints.sh`) | wie karte1: pruefcodes 11, portability 101, efxflags und fremdprogramm mit derselben Meldung; i18n ok |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen, volles ctest nach dem Rebase (AP13S
berührt die Dateien dieses Zweigs nicht außer den Listen am Ende), Docks, Sprachläufe. Keine Bildzeit-Messung: die Formen sind ein Zeichenaufruf
je Ansicht, die Durchsicht eine Kopie und ein Vollbild-Dreieck nur bei eingeschalteter Durchsicht; ohne Messung keine Aussage zum Tempo.

## Offen / Fragen an shank

1. **„Im Vordergrund“ ab Werk an** (wie bis rc611, Knochen über der Figur) oder aus wie Blender/3ds Max? Eine Stelle: `Settings::modellKnochenVorn`.
2. **Durchsicht wie 3ds Max:** die Figur mit Texturen halb deckend (so gebaut, Deckkraft wie Blender 0,5) oder grau in der See-Through-Farbe von
   Max? Eine Stelle: `kDurchsichtAlpha` in `gui/app_modelltab_knochen.h` (für grau käme eine Farbe im Mischen dazu).
3. **Control Rig:** Bodenkreis und Marke über dem Kopf sind nur Schmuck (kein Griff dahinter, `schmuck = true` in `rigformen.cpp`) - so lassen?
- Control Rig nur nach shanks Bild (keine SmartAnim-Szene): Farben rechts rot, links blau, Mitte grün, Becken/Brust gelb; Ringweite aus der Haut
  mal 1,2 (Annahme, nach Bild). CAT-Maße in Einheiten des Rigs mal 0,64 mal Maßstab (Bein gegen 29,19).

## Stand je Zeile der Abgleichtabelle (für den Hauptchat)

| Zeile | Stand |
|---|---|
| R14 | gleich: Max Bone-Kasten mit Flossen, Blender Oktaeder/Stick/B-Bone/Hülle/Draht im Overlays-Menü; Durchsicht Alt+X / Alt+Z mit Mischstufe im Zeichner unter beiden Grafikwegen |
| P9 | gleich: Alt+X schaltet die Durchsicht, Statuszeile nennt es |
| P10 | Alt+Z gleich (Röntgen je Ansicht); Z und Umschalt+Alt+Z hatte AP5 schon |

## Changelog

## English

- Model tab: bones are drawn in the picture of the view. Like 3ds Max as bone boxes (with fins on demand), like Blender with Display as: Octahedral, Stick, B-Bone, Envelope or Wire.
- Bones and controls lie in front of the figure by default; switch In front off and the figure hides them.
- See-through: Alt+X like 3ds Max (the figure in every view), Alt+Z like Blender (X-ray of the active view). The figure lies half opaque over what is behind it.
- Rig style in the Animation area: Standard, Control rig with rings, CAT boxes or Blender octahedra. Controls are coloured by side; the style changes only the look, never the rig.
- A click or drag hits the visible shape of a bone or control: a ring, a box edge or inside, the sphere of a pole, an octahedron.
- Everything looks the same under Direct3D 12 and Direct3D 11.

## Deutsch

- Model-Tab: Knochen stehen im Bild der Ansicht. Wie 3ds Max als Bone-Kästen (Flossen auf Wunsch), wie Blender mit Anzeigen als: Oktaeder, Stick, B-Bone, Hülle oder Draht.
- Knochen und Griffe liegen ab Werk vor der Figur; ist „Im Vordergrund“ aus, verdeckt die Figur sie.
- Durchsicht: Alt+X wie 3ds Max (die Figur in jeder Ansicht), Alt+Z wie Blender (Röntgen der aktiven Ansicht). Die Figur liegt halb deckend über dem, was hinter ihr ist.
- Rig-Stil im Bereich Animation: Standard, Control Rig mit Ringen, CAT-Kästen oder Blender-Oktaeder. Griffe sind nach Seite gefärbt; der Stil ändert nur das Aussehen, nie das Rig.
- Klick und Ziehen treffen die sichtbare Form eines Knochens oder Griffs: Ring, Kante oder Inneres eines Kastens, Kugel eines Pols, Oktaeder.
- Alles sieht unter Direct3D 12 und Direct3D 11 gleich aus.

## 中文

- 模型选项卡: 骨骼绘制在视图画面中。同 3ds Max 为骨骼方盒 (可选鳍), 同 Blender 可选显示为: 八面锥、棍形、B-骨骼、封套或线框。
- 骨骼和控制器默认位于角色前面; 关闭“在前面”后角色会遮住它们。
- 透明显示: Alt+X 同 3ds Max (所有视图中的角色), Alt+Z 同 Blender (当前视图的透视模式)。角色以半透明覆盖其后方内容。
- 动画区域的绑定样式: 标准、带圆环的控制绑定、CAT 方盒或 Blender 八面锥。控制器按侧着色; 样式只改变外观, 从不改变绑定。
- 单击或拖动命中骨骼或控制器的可见形状: 圆环、方盒的边或内部、极向量的球、八面锥。
- 在 Direct3D 12 和 Direct3D 11 下外观完全相同。

## 日本語

- モデルタブ: ボーンをビューの画像内に描きます。3ds Max 風はボーンボックス (フィンは任意)、Blender 風は表示方法: 八面体、スティック、Bボーン、エンベロープ、ワイヤー。
- ボーンとコントロールは既定でフィギュアの手前。「最前面」をオフにするとフィギュアが隠します。
- シースルー: Alt+X は 3ds Max 風 (全ビューのフィギュア)、Alt+Z は Blender 風 (アクティブビューの透過表示)。フィギュアは背後の上に半透明で重なります。
- アニメーション領域のリグのスタイル: 標準、リング付きコントロールリグ、CAT ボックス、Blender 八面体。コントロールは左右で色分け。スタイルは見た目だけを変え、リグは変えません。
- クリックとドラッグはボーンやコントロールの見えている形に当たります: リング、ボックスの辺や内側、ポールの球、八面体。
- Direct3D 12 と Direct3D 11 で同じ見た目です。

## Español

- Pestaña Model: los huesos se dibujan en la imagen de la vista. Como 3ds Max como cajas de hueso (aletas a elección), como Blender con Mostrar como: Octaédrico, Varilla, B-Bone, Envolvente o Alambre.
- Huesos y controles quedan por defecto delante de la figura; con Al frente desactivado la figura los tapa.
- Ver a través: Alt+X como 3ds Max (la figura en todas las vistas), Alt+Z como Blender (rayos X de la vista activa). La figura queda semitransparente sobre lo que hay detrás.
- Estilo de rig en el área Animación: Estándar, rig de control con anillos, cajas CAT u octaedros de Blender. Los controles se colorean por lado; el estilo solo cambia el aspecto, nunca el rig.
- Un clic o arrastre acierta la forma visible de un hueso o control: un anillo, el borde o el interior de una caja, la esfera de un polo, un octaedro.
- Todo se ve igual con Direct3D 12 y Direct3D 11.

## Čeština

- Karta Model: kosti se kreslí do obrazu pohledu. Jako 3ds Max jako kvádry kostí (ploutve volitelně), jako Blender se Zobrazit jako: osmistěn, tyčinka, B-Bone, obálka nebo drát.
- Kosti a ovladače leží ve výchozím stavu před postavou; po vypnutí V popředí je postava zakryje.
- Průhlednost: Alt+X jako 3ds Max (postava ve všech pohledech), Alt+Z jako Blender (rentgen aktivního pohledu). Postava leží napůl průhledně nad tím, co je za ní.
- Styl rigu v oblasti Animace: Standardní, ovládací rig s kruhy, kvádry CAT nebo osmistěny Blenderu. Ovladače jsou obarveny podle strany; styl mění jen vzhled, nikdy rig.
- Kliknutí a tažení zasáhne viditelný tvar kosti nebo ovladače: kruh, hranu nebo vnitřek kvádru, kouli pólu, osmistěn.
- Pod Direct3D 12 a Direct3D 11 vypadá vše stejně.
