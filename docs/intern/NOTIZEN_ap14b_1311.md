# Notizen AP14b (13.11.): alte Handbuchbilder des Model-Tabs neu aufgenommen, Texte und Aufnahmeschritte auf den heutigen Aufbau - rc612

Auftrag AP14b (Opus 5.5): die Liste `Y:\werkzeuge\ap14\bilder_neu.md` von AP14 (alte Bilder im Aufbau von rc611) nach AP11 neu aufnehmen,
vor und nach der Aufnahme ansehen, Texte und Handbuchschritte berichtigen, die übrigen Model-Tab-Bilder durchsehen. Worktree `Y:\p_ap5`,
Zweig `p_ap14b`, auf karte1 `9fe5a817c`. Kein Programmcode geändert - nur Handbuchschritte der Selbsttests (`gui/selbsttest_*.inc`),
Handbuchquellen und Bilder. Werkzeuge, Läufe und Fotos: `Y:\werkzeuge\ap14b\` (`hb.sh`, `voll.sh`, `platz_a.sh`, `platz_b.sh`).

Aufnahme: `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=<gruppe>` (bzw. `BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ent3ahb | ap6hb`),
je Sprache ein Lauf, danach `tools/handbuch/bilder.py`; Bilder 2538 × 1472 (Ausschnitte des vollen Laufs in ihrer Größe). Die Gruppen
mb5, rt und ph brauchen die Daten des Modell-Blocks wie `tools/testrunde.py` (`BHED_RT_GLA`, `BHED_MB_BELEGE`, `BHED_MB7_BSP`) - ohne sie
sind rt (Gruppe nicht gelaufen) und ph (kein Flug) rot. Der volle Lauf (`voll.sh`) mit der Übungsmission taspir1 auf einem subst-Laufwerk,
nur die Bilder des Model-Tabs und k_pruefen (`BHED_FOTO_NUR`). Höchstens zwei Programme gleichzeitig, Leerlauf-Priorität.

## Aufgenommene Bilder

Alle in sechs Sprachen (en, de, zh, ja, es, cs), jedes englische vor und nach der Aufnahme angesehen, aus jeder Sprache Stichproben
(de: „Überlagerungen“, abgeschnittene Texte; zh/ja/es/cs: Suche, Befund, Rahmen).

| Liste | Bilder | Gruppe |
|---|---|---|
| A | h_mb1_ansehen, h_mb1_menue, h_mb1_vier | mb1a1 |
| A | h_mb2_auge, h_mb2_blender, h_mb2_insskript | mb1a2 |
| A | h_mb3_animation, h_mb3_speichern, h_mb3_befund | mb2 |
| A | h_md_modell, h_md_karte | md |
| A | h_uv_editor, h_uv_schnell | uv |
| A | h_mb4_ghosts, h_mb4_uebersicht, h_mb4_menue | mb3 |
| A | h_mb5_dopesheet, h_mb5_kurven, h_mb5_werkzeuge, h_mb5_aufraeumen | mb5 |
| A | h_mb6_ik, h_mb6_fussrolle, h_mb6_pfade, h_mb6_raum | mb6 |
| A | h_ph_sprung, h_ph_figur | ph |
| A | h_rt_uebertragen, h_rt_ketten (dazu h_pr_gang derselben Gruppe) | rt |
| A | h_mb7_posieren, h_mb7_bauen, h_mb7_mehrere | mb7 |
| A, D | h_modell, k_pruefen, m_abspielen, m_flaechen; dazu (Punkt 3, alle zeigten den Model-Tab von rc608) m_npc, m_modus, m_insskript, m_details, m_caps, m_gla, k_modell_leiste, r_leiste_modell | voller Lauf |
| B | h_ap2_max, h_ap2_blender, h_ap2_ebenen | ap2 |
| B | h_mbk_max, h_mbk_cube, h_mbk_blender | mbkopf |
| B | h_mbr_max, h_mbr_vier, h_mbr_blender | mbraster |
| B | h_mbn_max, h_mbn_orbit, h_mbn_blender | mbnav |
| C (nur de) | h_ap6_blender, h_ap7_blender, h_ap8_blender, h_ap9_blender, h_ap10_blender; dazu h_ap13s_blender (12.11. aufgenommen, vor der Umbenennung - stand nicht in der Liste) | ap6hb, ap7, ap8, ap9, ap10, ap13shb |
| C + Punkt 3 | h_ap1_max, h_ap1_blender, h_ap1_pruefen, h_ap3_max, h_ap3_blender, h_ap4_max, h_ap4_blender, h_ap4_gruppen, h_ap4_seitenleiste, h_ap5_max, h_ap5_blender, h_ap5_pruefen, h_ap13k_arten, h_ap13k_blender, h_ap13k_cat, h_ap13k_control, h_ap13k_durchsicht, h_ent3a_explorer, h_ent3a_leiste, h_ent3a_seite, h_ent3a_stapel (alle Sprachen) | ap1, ap3, ap4, ap5, ap13k, ent3ahb |
| nach ROT612 (Rebase auf 8e851b6cb) | h_pt_malen | pt |

**Punkt 3 (übrige Bilder durchgesehen):** sichtbar alt war außer der Liste die Gruppe Modell mit drei statt vier Knöpfen (AP11 brachte
„In neuem Tab öffnen“) in h_ap1_pruefen, h_ap3_*, h_ap4_seitenleiste, h_ap5_*, h_ap13k_arten, h_ap13k_durchsicht, h_ent3a_explorer -
ihre Gruppen sind ganz neu aufgenommen. Die Bilder des vollen Laufs (m_*, k_modell_leiste, r_leiste_modell) zeigten noch den Model-Tab von
rc608 (Animationsliste, Abspielleiste mit Skin-Liste). Aktuell und nicht neu: h_ap9_max, h_ap10_max, h_ap12_*, h_ap13s_max, h_ap11_*,
h_ent3b_*, h_ai_bibliothek, h_ap6_max, h_ap7_max, h_ap8_max, h_ap8_spiegeln (Feld und Zeitleiste wie heute, Bedienung wie 3ds Max ohne
das Menü Überlagerungen).

**Entfallen:** h_mb2_weights, h_mb2_kontext (6 Sprachen) samt Fotoschritten - kein Handbuchtext zeigt sie mehr.

## Aufnahmeschritte angepasst (nur Handbuchschritte)

| Bild | was nicht mehr stimmte | Änderung | Datei |
|---|---|---|---|
| h_mb1_vier | das Rechtsklick-Menü des Explorers aus h_mb1_menue stand noch offen - Esc schließt es nicht (ENTSCHEID3A C4 gilt dem Menü der Ansicht) | Klick daneben (Statuszeile) statt Esc | `gui/selbsttest_mb1a1.inc` |
| h_mb3_befund | der Befund stand nicht im Bild (das Feld zeigte die Plätze), der „Marker unter der Zeitleiste“ ist seit AP1 die Zeitkonfiguration | Gruppe animation.cfg oben ins Feld, Fenster Zeitkonfiguration auf | `gui/selbsttest_mb2.inc` |
| h_mb5_werkzeuge | das Untermenü Animation > Animation tools > Key tools ragt seit dem längeren Menü Animation unten über das Hauptfenster hinaus und wird ein eigenes Fenster (Viewports) - auf dem Foto stand nur „Animation tools“ hervorgehoben | „Alle Aktionen suchen“ (X) mit „Key tools“, Maus auf Breakdown (Tooltip); Text und Bildunterschrift nennen die Suche | `gui/selbsttest_mb5.inc` |
| h_ph_sprung | der Bereich Animation zeigt vier Ansichten: der Sprung klein und abgeschnitten | eine große Ansicht (`vierAnsichten = false`) | `gui/selbsttest_ph.inc` |
| m_modus, m_abspielen, m_insskript, m_details, m_caps, m_gla, h_modell | Ausschnitt ab zwei Zeilenhöhen schnitt die Bereichsreihe des Model-Tabs mittendurch | Ausschnitt ab dem Hauptmenü | `gui/selbsttest_handbuch.inc` |
| m_abspielen | „Schleife erzwingen“ stand am Rand des Ausschnitts (58 % der Breite), deutsch abgeschnitten | Ausschnitt bis hinter den Knopf | `gui/selbsttest_handbuch.inc` |
| m_flaechen | die Zeile Skin lag unter dem Rand des Felds, die Marke darunter | Gruppe Detailstufe und Skin oben ins Feld (`hbModellGruppeOben`) | `gui/selbsttest_handbuch.inc` |
| k_modell_leiste | zeigte die Leiste unter der Ansicht (die Skins stehen seit AP1 Z20 im Befehlsfeld) | zeigt die Gruppe Detailstufe und Skin | `gui/selbsttest_handbuch.inc` |
| h_mb2_weights, h_mb2_kontext | vom Handbuch nicht mehr gezeigt (modell-weights ist seit AP8 die Seite von inhalt_ap8) | Fotoschritte und Dateien (6 Sprachen) entfernt, die Schritte davor bleiben (gleicher Stand für die folgenden Bilder) | `gui/selbsttest_mb1a2.inc` |

## Texte (6 Sprachen)

- h_mb1_ansehen: „Kyle mit einem Säbel“ - im Bild steht der Sturmtruppler (`inhalt_mb1a1.py`).
- h_mb3_befund: die Gruppe animation.cfg nennt den Befund, das Fenster Zeitkonfiguration zeigt die Zeile (`inhalt_mb2.py`).
- h_mb5_dopesheet: Dopesheet in den Spuren der Zeitleiste, Spurleiste der Auswahl, grün = drehen, gewählter Schlüssel weiß gerahmt (vorher
  „unter den Ansichten“, „Zusammenfassung“, „orange“); h_mb5_werkzeuge: die Suche (`inhalt_mb3b.py`).
- „Rollout“ → Gruppe in den Bildunterschriften h_uv_schnell, h_mb6_ik, h_mb6_raum, h_ph_sprung, h_ph_figur, h_rt_uebertragen (mit dem
  heutigen Namen der Gruppe „Übertragen (Retargeting)“) (`inhalt_uv1b.py`, `inhalt_mb4.py`, `inhalt_ph.py`, `inhalt_rt.py`).
- Schrittseiten k-animation-abspielen, k-modell, k-skins (`inhalt_schritte2.py`) und ihre Kurzfassungen (`inhalt_kurz.py`, `inhalt_kurz3.py`),
  Bild auf der Seite Funktionen (`inhalt_funktionen.py`): Animationen sind die Sequenzen im Szenen-Explorer (Suche, eine Gruppe je Knoten),
  „Schleife erzwingen“ und Tempo in der Zeitleiste, Ins Skript öffnet das Fenster (Skript, Figur, Befehl, Einfügen), Knochen/Texturen/.skin
  öffnen in der Werkzeugleiste, die Skins in der Gruppe Detailstufe und Skin des Befehlsfelds; Bildunterschriften ohne „Animationsliste“ und
  „Abspielleiste“. Übersetzungen de/zh/ja in `tools/handbuch/uebersetzung/*/`, es/cs in `inhalt_es.py`/`inhalt_cs.py` (Schlüssel = englischer Text).

## Läufe

Je Gruppe und Sprache ein Lauf (`Y:\werkzeuge\ap14b\hb\<gruppe>_<sprache>.out`), Bau `bau3`..`bau7` (behaved) auf karte1 `9fe5a817c`,
nach dem Rebase auf `8e851b6cb` (ROT612) `bau8` (alle Ziele) und pt.

| Gruppe | Ergebnis |
|---|---|
| mb1a1, mb1a2, mb2, md, uv, ph, rt, mb7, ap2, mbkopf, mbraster, mbnav, ap1, ap3, ap4, ap5, ap13k, ent3ahb (6 Sprachen); ap6hb, ap7, ap8, ap9, ap10, ap13shb (de); voller Lauf (6 Sprachen, 8 / 0) | 0 FEHLER |
| mb3, mb5, mb6 (6 Sprachen, auf `9fe5a817c`) | nur die auf karte1 bekannten Zeilen rot (mb3 „MB4 Pfad: 21 Punkte je Bild“, mb5 „Pfad ziehen“, mb6 „FK ab Frame +24“ / „Max E am Oberarm“), die ROT612 behoben hat; die Bilder dieser Gruppen sind davon nicht betroffen (angesehen) |
| nach dem Rebase auf `8e851b6cb` (ROT612): pt (h_pt_malen, 6 Sprachen) | 2 / 0 je Sprache - wieder eine Ansicht von vorn (figurLaden von ROT612) |
| nach dem Rebase: mb3, mb5, mb6 (en) | 28 / 0, 161 / 0, 39 / 0 - die drei bekannten Zeilen grün; Key tools über die Suche wie vorher im Bild |

Rot vor der Änderung (nachgestellt, Fotos in `Y:\werkzeuge\ap14b\fotos_befund\` bzw. `vorher\`, `nachher\`): h_mb1_vier mit offenem
Menü, h_mb5_werkzeuge ohne Untermenü (zweimal: Klick, dann Maus), h_mb3_befund ohne Befund, h_ph_sprung in vier kleinen Ansichten,
m_flaechen mit der Marke unter dem Feld, m_abspielen deutsch abgeschnitten, m_modus mit halber Bereichsreihe, k_modell_leiste ohne Skins.
Ohne `BHED_RT_GLA`/`BHED_MB_BELEGE`/`BHED_MB7_BSP` waren rt (2 FEHLER, Gruppe nicht gelaufen) und ph (2 FEHLER, kein Flug) rot - eine Frage
der Umgebung, nicht des Programms.

## Beobachtung (kein Fehler der Bilder, nicht behoben - kein Programmcode in diesem Auftrag)

- Das Rechtsklick-Menü des Szenen-Explorers schließt Esc nicht (C4 gilt dem Menü der Ansicht). Foto des Laufs:
  `Y:\werkzeuge\ap14b\fotos_befund\h_mb1_vier_explorermenue_offen.bmp`. In 3ds Max und Blender schließt Esc jedes Kontextmenü.
- Das Untermenü Animation > Animation tools ragt im Hauptfenster 2538 × 1472 unten hinaus und wird ein eigenes Fenster (Viewports); im
  Programm sichtbar, nur nicht auf einem Foto des Hauptfensters (`fotos_befund\h_mb5_werkzeuge_untermenue_zu.bmp`).

## Schlussrunde des Zweigs

- Bau `bau8` nach dem Rebase (alle Ziele, `--parallel 2`, /W4 /WX): 0 Warnungen.
- ctest handbuch_sprachen, handbuch_suche, hilfe_orte, tour (geänderte Handbuchquellen): 4 / 4. Weitere ctests nicht: kein Programmcode
  geändert, nur Handbuchschritte der Selbsttests (die laufen in den Handbuchgruppen oben).
- `gen_handbuch.py --pruefen` grün (184 Seiten × 6 Sprachen, es/cs 0 Texte englisch, keine fehlenden Bilder; der Hinweis „2 Einträge in
  uebersetzung/ werden nicht mehr gebraucht“ stand schon auf karte1); Handbuch mit `gen_handbuch.py` neu erzeugt (geänderte Seiten,
  `suche.js`, `touren.json`).
- Lints: pruefcodes 11, portability 101 (wie karte1), imgui_context, headers, i18n, sources, gapp ok.
- Bewusst nicht: Selbsttest-Gruppen außerhalb des Handbuchs (keine Prüfzeile geändert, nur Aufnahmeschritte), Sprachläufe anderer Tabs.

## Changelog

### English

- Guide: the pictures of the Model tab show today's layout (scene explorer, command panel, timeline) in all six languages; the step pages for models (play an animation, look at a model, skins) describe the sequences in the scene explorer, the timeline, the window Into script and the skins in the command panel.

### Deutsch

- Handbuch: die Bilder des Model-Tabs zeigen in allen sechs Sprachen den heutigen Aufbau (Szenen-Explorer, Befehlsfeld, Zeitleiste); die Schrittseiten zu Modellen (Animation abspielen, Modell ansehen, Skins) beschreiben die Sequenzen im Szenen-Explorer, die Zeitleiste, das Fenster Ins Skript und die Skins im Befehlsfeld.

### 中文

- 手册: 模型选项卡的图片在全部六种语言中显示当前布局 (场景资源管理器、命令面板、时间轴); 模型相关的步骤页面 (播放动画、查看模型、皮肤) 介绍场景资源管理器中的序列、时间轴、“插入脚本”窗口以及命令面板中的皮肤。

### 日本語

- ガイド: モデルタブの画像が 6 言語すべてで現在のレイアウト (シーンエクスプローラー、コマンドパネル、タイムライン) を示すようになりました。モデルの手順ページ (アニメーションの再生、モデルを見る、スキン) は、シーンエクスプローラーのシーケンス、タイムライン、「スクリプトへ」ウィンドウ、コマンドパネルのスキンを説明します。

### Español

- Guía: las imágenes de la pestaña Model muestran la disposición actual (explorador de escena, panel de comandos, línea de tiempo) en los seis idiomas; las páginas de pasos sobre modelos (reproducir una animación, ver un modelo, skins) describen las secuencias del explorador de escena, la línea de tiempo, la ventana Al script y las skins del panel de comandos.

### Čeština

- Příručka: obrázky karty Model ukazují ve všech šesti jazycích dnešní uspořádání (průzkumník scény, panel příkazů, časová osa); stránky s postupy k modelům (přehrát animaci, prohlédnout model, skiny) popisují sekvence v průzkumníku scény, časovou osu, okno Do skriptu a skiny v panelu příkazů.

