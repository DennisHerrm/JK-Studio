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

## Punkt 7: Helligkeit Original-JKA gegen Vorschau

## Schluss

## Offen

## Changelog

## English

## Deutsch

## 中文

## 日本語

## Español

## Čeština
