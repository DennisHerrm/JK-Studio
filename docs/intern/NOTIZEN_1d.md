# Notizen Modell-Block 1d: Modellieren und Requisite mit einem Klick in die Karte

Stand 07.11.2026, Zweig `karte1`, auf 6a7aa4cd0 (1b). Ein Agent. Laufnamen `md_*` (Schluss `md_e_*`), Bau `Y:/karte/build-test`,
Laufskript `lauf.py` wie 1b. kFassung bleibt rc610.

## Quellen

- WhiteoutLib 35d068d (nur gelesen): `include/whiteout/models/wem/geometry/modelling.h` (Vertrag der Werkzeuge: Plan mit
  fester Topologie, Betrag als base + amount * direction, Resample der UV in der Quellflaeche, AmountRange D14, FinishTool mit
  kDefaultShadingAngle 60 Grad, ModelRefusal), `src/.../modelling.cpp` (PlanJoinTriangles/quadShape, DefaultWeldThreshold),
  `prepare.cpp` (CoincidenceTolerance), `mesh.h`. WhiteoutFlakes (nur gelesen): `edit/mesh_*`, `ui/tool_caddy.*`,
  `guide/markdown/mesh.md` (Werkzeugliste, Kaestchen mit Uebernehmen / + / Abbrechen). Beides BSD-3 (Fernando Sahmkow) mit
  LICENSE-AI.md; die abgeleiteten Dateien tragen den vollen Kopf: `include/bhed/modell/wof/poly.h`,
  `src/modell/wof/mesh_{netz,werkzeuge,werkzeuge2,grundkoerper}.cpp`, `mesh_intern.h`, `gui/wof/app_modelltab_mesh.cpp`.
- Retopologie (libSatsuma/LEMON) nicht eingebaut - nicht verlangt fuer Requisiten und nicht sauber ohne neue Abhaengigkeiten.
- Tasten: 3ds Max laut Autodesk-Hilfe "Editable Poly"-Tastaturbefehle (Umschalt+E Extrude, Strg+Umschalt+E Connect,
  Strg+Umschalt+C Chamfer, Alt+C Cut, Strg+Umschalt+W Target Weld, Ruecktaste/Strg+Ruecktaste Remove, Entf Delete, 1-5
  Ebenen); Blender aus `plaene/modellblock_stufe0/belege/blender_keys.txt` (Keymap Mesh: Alt+E, I, Strg+B, Strg+Umschalt+B,
  Strg+R, K, M, Alt+M, Umschalt+A, X/Entf, W, A, Alt+A, 1-3).
- JKA-Grenzen: SHADER_MAX_VERTEXES 1000, iMAX_G2_BONEREFS_PER_SURFACE 32, iMAX_G2_BONEWEIGHTS_PER_VERT 4 (vorhanden in
  `bhed/modell/g2.h`), MAX_QPATH 64 (Name/Shader/Pfad), Tag = ein Dreieck (G2_ProcessSurfaceBolt liest drei Ecken).
- MD3: qfiles.h (md3Header_t 108 Bytes, md3Frame_t 56), R_LoadMD3 (Normale lat/lng), q_math.c RadiusFromBounds.
- Spiel: Cmd_SetViewpos_f (g_cmds.cpp, OpenJK). Requisit-Ablage nach `plaene/modell_in_karte.md`; Einsetzen ueber das
  vorhandene `modellEinsetzen` (rc609 M0) und den Import-Ordner (Suchpfad von Editor und q3map2, kommt mit in die .pk3).

## 1. Gebaut

- Arbeitsnetz `PolyNetz` (Vielecke ueber den Dreiecken des Dokuments, Netz::polyVon je Dreieck, feste Punkte ueber
  Netz::aufbauen(..., punkte)); nur beruehrte Teile werden zurueckgeschrieben (Rundlauf ohne Aenderung Byte fuer Byte).
- Werkzeuge: Extrude (Gruppe / lokale Normale / je Polygon; Rand), Inset (Gruppe / je Polygon, even offset, UV wandert in der
  Quellflaeche), Bevel (Hoehe + Umriss), Chamfer (Kanten mit Gehrung und Flicken, Punkte), Connect (Kanten mit Segmenten und
  Slide, Punkte), Loop Cut, Cut (Klicks auf Kanten/Punkte, laeuft weiter), Weld, Target Weld (nur ueber eine Kante, Autodesk),
  Merge (Mitte = Collapse je Gruppe, erster/letzter wie Blender), Bridge (Raender, Polygone, Segmente, Verdrehen), Remove
  (Kanten, Punkte, mit Aufraeumen), Delete, Detach (Element, neue Flaeche), Symmetrize (X/Y/Z, Knochen l_/r_), To Quads
  (Kosten wie WL/Blender, Diagonalen bleiben), Grundkoerper (Kasten, Ebene, Zylinder, Kugel, Kegel, Rohr, Torus mit UV).
  Jede Ablehnung mit Grund (WL ModelRefusal), Netz unberuehrt. Betragsgrenze exakt (quadratisch je Dreieck).
- Normalen der beruehrten Vielecke neu nach 60 Grad (WL FinishTool), Gewichte neuer Ecken gemischt (vier, Summe 1).
- Model-Tab Bereich "Modellieren": Unterebenen 1-5 / 1-3, Auswahl je Ebene, Klick/Rahmen (Strg/Alt bzw. Umschalt), Polygone
  und Auswahl als Ueberlagerung, Werkzeugkaestchen (live, Uebernehmen / + / Abbrechen, Enter/Esc, Grenzen im Kaestchen),
  Rollouts Edit Poly / Edit Mode, JKA-Grenzen live je Flaeche, LODs angleichen (.glm), Menue "Modellieren", Rechtsklick,
  Aktionssuche, Tooltips mit Taste; jedes Werkzeug ein Rueckgaengig-Schritt.
- MD3-Schreiber: bewegte/neue Ecken (andere Bilder um denselben Weg), neue Flaechen, Kasten und Radius der Bilder neu; neue
  .md3 ohne Datei (ein Bild, keine Tags).
- Requisit: "Neues Requisit" (leere .md3, Textur des Karten-Editors), "In die Karte" (models/map_objects/<karte>/<name>.md3 im
  Import-Ordner, misc_model in der Mitte der Kamera, wartet auf den neuen Suchpfad), Strg+S -> Live-Neuladen jedes Exemplars;
  Rueckweg: Knopf neben dem Schluessel model der Entity (Entity-Inspektor) oeffnet das .md3 im Model-Tab.
- ctest `modell_mesh` (59 Pruefungen), Selbsttest `md`, `mdkarte`, `mdspiel`, `mdzeit`, Handbuch `BHED_HB_NUR=md`
  (h_md_modell, h_md_karte, 2538 x 1472, alle 6 Sprachen angesehen: en/cs ganz, Rollouts cs ohne Abschneiden).
- Handbuchseite `modell-modellieren` (6 Sprachen), Texte `tools/i18n_md1d.py`, About-Zeile, THIRD-PARTY.md.

## 2. Rot -> gruen

- MD3 ohne Datei: Bilder bei 104 statt 108 (md3Header_t mit ofsEnd) -> readMd3 fand 0 Flaechen (ctest "MD3 neu geschrieben"
  rot) -> kKopfGanz 108 -> gruen.
- Bridge Deckel/Boden eines Kastens: die Verbindungen sind schon Kanten, dritte Seite (ctest rot, "zweimal gleich gerichtet")
  -> Ablehnung WuerdeKneifen wie WL WouldPinch -> gruen (Test jetzt: zwei Kaesten 16/28/14 und die Ablehnung).
- Kanten-Karte war statisch fuer alle Netze (Vorschau-Kopie und Basis teilten sie) - vor dem ersten Lauf behoben (Fehlerquelle).
- `md_a1`: Max Umschalt+E rot - Testfehler (LeftShift statt ImGuiMod_Shift) -> `md_a2` gruen.
- `md_a1/md_a2`: "Enter uebernimmt Inset" rot - Erwartung falsch (nach To Quads + Rueckgaengig ist der Deckel ein Dreieck:
  11/24/15, Log "Inset: done (3 changed)"); Enter/Esc zusaetzlich wie G/R/S ueber IsKeyPressed -> `md_a3` 62/0.
- `md_hb1`: Requisit grau ohne Textur -> loadModelTextures + Abgleich der Flaechen/Shader einer .md3 -> `md_hb2` mit Textur.
- `md_hb1`: Beenden "Programm laeuft nach No weiter" (Karte geaendert) -> Testraum am Ende zurueckgenommen -> `md_hb2` 0 FEHLER.
- `md_e_funk` (Schluss): das Hauptmenue hiess wie der Bereich ("Modeling") - der Klick des alten Tests auf den Bereich traf
  das Menue (2 FEHLER: Bereich nicht gewaehlt, "View/Check" kam nicht an) -> Menue "Edit Mesh / Netz bearbeiten" (Blender:
  Menue Mesh), Handbuch nennt den Bereich ueber MbBereichModellieren -> `md_f_funk` (siehe 5).
- Bildzeit Ueberlagerung, alles gewaehlt (`md_zeit1`) 1,28 / 2,30 ms -> Schirmorte je Punkt einmal, Auswahl als Feld
  (`md_zeit2`) 0,91 / 2,00 ms, gleiches Bild.

## 3. Im Spiel (Movie Duels)

Selbsttest `mdspiel` (`md_s1`): Testraum m0raum, Requisit (Kasten 32, Deckel Inset 6 + Extrude 16, Textur der Spielkiste
models/map_objects/factory/s_crate_01) mit einem Klick in die Karte, Vorschau-Foto der Editor-Kamera (setviewpos -208 -144 40
0, Sichtfeld 80), Bau q3map2 -meta. Skript `md_spiel.py`: MovieDuels-SP per Popen, eigener fs_homepath, fs_game MD, stumm,
Fenster ausserhalb des Sichtbaren, ohne Fokus, Foto per PrintWindow; Paarbild `laeufe/md_spiel_paar.png` (1920 x 2226,
Spiel oben 1920 x 1440, Vorschau unten 1364 x 786, ungeskaliert, angesehen).

- Im Spiel steht das Requisit wie in der Vorschau: Kasten mit schmalerem, hoeherem Deckel, Textur gleich verteilt (das "I"
  unten vorn, der orange Streifen oben), auf dem Boden an derselben Stelle im Raster vor der Tuer. Die Karte wurde ohne
  -light gebaut: das Requisit hat dort eine schwarze Lightmap und ist dunkler als im Editor (misc_model wird eingebacken).
- Abweichung: Movie Duels zeigt die dritte Person (Schwert); `cg_thirdPerson 0`, `give all`, `weapon 2` und `setviewpos`
  wirkten im 45-s-Fenster nicht sichtbar, das Bild ist daher kein deckungsgleicher Blick. Gegenprobe Lage/Form/Textur nur
  per Auge.
- Spielordner vor und nach jedem der drei Starts gleich (Name, Groesse, Zeit aller Dateien).
- Original-JKA nicht gestartet (Steam lief und war angemeldet; aus Zeitgruenden offen, siehe 7).

## 4. Bildzeit (allein, BHED_D3D_DEBUG=0, vor 19:00)

Sturmtruppler LOD 0 (2945 Punkte, 6551 Kanten, 3703 Polygone, 5199 Kanten gezeichnet), CPU je Bild Mittel / p99:
Ansehen 0,38-0,45 / 0,88-1,42 ms; Modellieren nichts gewaehlt 0,57-0,71 / 0,87-1,54 ms; alles gewaehlt 0,91 / 2,00 ms
(vorher 1,28 / 2,30). Schwankung zwischen `md_zeit1` und `md_zeit2` im Ansehen-Bereich ohne Code-Aenderung dort: Messrauschen.

## 5. Schluss

- Sauberer Neubau (`--clean-first`, `laeufe/md_bau_clean.txt`) 0 Fehler, 0 Warnungen; Folgebauten (`md_bau_2`, `md_bau_3`) 0 / 0.
- ctest 230/231 (`laeufe/md_e_ctest.txt`), `modell_mesh` gruen; rot nur `q3map2_pr316` (externer Datenordner leer, bekannt).
  `hilfe_orte` war rot (neue Handbuchseite) -> `gen_hilfe_orte.py` -> gruen.
- Selbsttest `md_e_*` (Summe `laeufe/md_e_summe.txt`, mit BHED_RT_GLA/BHED_MB_BELEGE/BHED_MB7_* wie 1b): tasten 43/0, mb2 37/0,
  blender 44/0, mb3 21/0, rig 26/0, mb5 159/0, mb6 37/0, uv 50/0, uvmd3 3/0, md 62/0, mdkarte 7/0, mb7 24/0, mb7spiel 4/0,
  mb7textur 1/0, ph 27/0, rt 19/0, rtjka 1/0, pr 15/0, uvrundlauf md/jka/jk2 924/127/71 gleich, rundlauf 2/0 x3,
  rigrundlauf/kurvenrundlauf/ikrundlauf/bild je Spiel 0 FEHLER, karteneditor modobj 25/0, aseb 27/0, modell 14/0,
  kuerzel 202/0, mission1 98/0, Modus 1 2635/0. funktionen 59/2 (siehe 2) -> nach dem Umbenennen `md_f_funk` 60/0,
  `md_f_md` 62/0, Handbuchbilder `md_hb3_*` 6 Sprachen 0 FEHLER.
- Lints wie vorher: portability 99 (zwischendurch 125 durch fehlende Includes in den neuen Dateien -> ergaenzt), pruefcodes 11,
  efxflags-Meldung, fremdprogramm (kartenpruefung.py), sonst ok; Handbuch es/cs 0 englisch.

## 6. Abweichungen

- Eigenes Vielecks-Netz statt WL Mesh/Topology (Halbkanten aus Indizes); Kanten als Punktpaare, Auswahl je Ebene als Listen.
- Extrude-Waende, Bridge und Grundkoerper uebernehmen die UV der Randecken (WL ebenso); nur Inset, Chamfer, Connect und
  Bevel-Umriss rechnen die UV in der Quellflaeche mit (WL Resample); Gewichte neuer Punkte: gemischt (Kante) bzw. vom
  Ursprungspunkt (Kopien).
- Chamfer mit einem Segment (WL bis 64 mit Profil); Bridge ohne Segment-Profil; Cut nur innerhalb eines Polygons je Klick
  (Max schneidet ueber mehrere, hier Klick fuer Klick); Collapse behaelt die UV jeder Ecke (WL: Mitte des UV-Kastens).
- Symmetrize-Schwelle = WL DefaultWeldThreshold (Diagonale * 1e-3); Normalen nach 60 Grad statt Glaettungsgruppen.
- Merge erster/letzter nach der Reihenfolge der Klicks (Blender: Auswahlverlauf).
- Requisit nur fuer .md3 (eine .glm geht nicht als misc_model in die Karte).

## 7. Offen

- Original-JKA-Vergleich (Steam angemeldet, GameData-Kopie erlaubt) nicht gemacht.
- Deckungsgleicher Spielblick (erste Person in Movie Duels).
- Retopologie, Make Planar, Hinge, Slice, Chamfer mit Segmenten/Profil.

## Changelog

### English
- Model tab: Modeling area - polygon tools like 3ds Max Editable Poly or Blender's Edit Mode on .glm and .md3: extrude, inset, bevel, chamfer, connect, loop cut, cut, weld, target weld, merge, bridge, remove, delete, detach, symmetrize, to quads and primitives (box, plane, cylinder, sphere, cone, tube, torus).
- A tool caddy shows the result live (Apply, + and Cancel); each tool is one undo step and says why when it cannot work; UVs and bone weights carry over.
- JKA limits live per surface (1000 vertices, 32 bones, 4 weights, 63-character names, tags one triangle); LODs of a .glm can take the drawn level.
- A prop goes into the open map with one click (models/map_objects/<map>/, misc_model at the camera); Ctrl+S updates every copy, and the model key of the entity opens it in the model tab again.

### Deutsch
- Model-Tab: Bereich Modellieren - Polygon-Werkzeuge wie 3ds Max (Editable Poly) oder Blender (Edit Mode) auf .glm und .md3: Extrudieren, Einfügen, Abschrägen, Abkanten, Verbinden, Schleifenschnitt, Schneiden, Verschweißen, Zielschweißen, Zusammenführen, Brücke, Entfernen, Löschen, Abtrennen, Symmetrisieren, zu Vierecken und Grundkörper (Kasten, Ebene, Zylinder, Kugel, Kegel, Rohr, Torus).
- Ein Werkzeugkästchen zeigt das Ergebnis live (Übernehmen, + und Abbrechen); jedes Werkzeug ist ein Rückgängig-Schritt und sagt, warum es nicht geht; UV und Knochen-Gewichte bleiben erhalten.
- JKA-Grenzen live je Fläche (1000 Ecken, 32 Knochen, 4 Gewichte, Namen bis 63 Zeichen, Tags ein Dreieck); die LODs einer .glm können die gezeichnete Stufe übernehmen.
- Ein Requisit kommt mit einem Klick in die offene Karte (models/map_objects/<karte>/, misc_model an der Kamera); Strg+S aktualisiert jedes Exemplar, der Schlüssel model der Entity öffnet es wieder im Model-Tab.

### 中文
- 模型选项卡: 建模区域 - 像 3ds Max 可编辑多边形或 Blender 编辑模式一样的多边形工具, 适用于 .glm 和 .md3: 挤出、插入、倒角挤出、切角、连接、循环切割、切割、焊接、目标焊接、合并、桥接、移除、删除、分离、对称、转为四边形以及基本体 (盒子、平面、圆柱、球、圆锥、管、圆环)。
- 工具框实时显示结果 (应用、+ 和取消); 每个工具是一个撤销步骤, 无法执行时会说明原因; UV 和骨骼权重得以保留。
- 每个表面实时显示 JKA 限制 (1000 顶点、32 骨骼、4 权重、名称最多 63 个字符、标签为一个三角形); .glm 的 LOD 可以采用当前绘制的层级。
- 一键把道具放入打开的地图 (models/map_objects/<地图>/, 放在摄像机处的 misc_model); Ctrl+S 更新所有副本, 实体的 model 键可再次在模型选项卡中打开它。

### 日本語
- モデルタブ: モデリング領域 - 3ds Max の編集可能ポリゴンや Blender の編集モードのようなポリゴンツールを .glm と .md3 に: 押し出し、インセット、ベベル、面取り、接続、ループカット、カット、溶接、ターゲット溶接、マージ、ブリッジ、除去、削除、切り離し、対称化、四角形化、プリミティブ (ボックス、平面、円柱、球、円錐、チューブ、トーラス)。
- ツールボックスで結果をライブ表示 (適用、+、キャンセル)。各ツールは 1 回の元に戻す単位で、できない時は理由を表示。UV とボーンウェイトは保たれます。
- サーフェスごとに JKA の上限をライブ表示 (頂点 1000、ボーン 32、ウェイト 4、名前 63 文字、タグは三角形 1 つ)。.glm の LOD は表示中のレベルを受け継げます。
- 小道具をワンクリックで開いているマップへ (models/map_objects/<マップ>/、カメラ位置に misc_model)。Ctrl+S で全コピーを更新し、エンティティの model キーから再びモデルタブで開けます。

### Español
- Pestaña Model: área Modelado - herramientas de polígonos como Editable Poly de 3ds Max o el modo edición de Blender sobre .glm y .md3: extruir, insertar, biselar, chaflán, conectar, corte en bucle, cortar, soldar, soldar a destino, fusionar, puente, quitar, borrar, separar, simetrizar, a cuadriláteros y primitivas (caja, plano, cilindro, esfera, cono, tubo, toro).
- Una caja de herramientas muestra el resultado en vivo (Aplicar, + y Cancelar); cada herramienta es un paso de deshacer y explica por qué no puede actuar; se conservan las UV y los pesos de hueso.
- Límites de JKA en vivo por superficie (1000 vértices, 32 huesos, 4 pesos, nombres de 63 caracteres, etiquetas de un triángulo); los LOD de un .glm pueden tomar el nivel mostrado.
- Un accesorio va al mapa abierto con un clic (models/map_objects/<mapa>/, misc_model en la cámara); Ctrl+S actualiza cada copia y la clave model de la entidad lo vuelve a abrir en la pestaña Model.

### Čeština
- Karta Model: oblast Modelování - polygonové nástroje jako Editable Poly v 3ds Max nebo režim úprav v Blenderu pro .glm a .md3: vysunutí, vsazení, zkosení, zkosení hran, spojení, řez smyčkou, řez, svaření, svaření k cíli, sloučení, most, odebrání, smazání, oddělení, symetrizace, na čtyřúhelníky a tělesa (kvádr, rovina, válec, koule, kužel, trubka, torus).
- Schránka nástroje ukazuje výsledek živě (Použít, + a Zrušit); každý nástroj je jeden krok zpět a řekne, proč nejde; UV a váhy kostí zůstávají.
- Limity JKA živě pro každou plochu (1000 vrcholů, 32 kostí, 4 váhy, názvy do 63 znaků, tagy jeden trojúhelník); úrovně LOD souboru .glm mohou převzít zobrazenou úroveň.
- Rekvizita jde jedním kliknutím do otevřené mapy (models/map_objects/<mapa>/, misc_model u kamery); Ctrl+S aktualizuje každou kopii a klíč model entity ji znovu otevře v kartě Model.
