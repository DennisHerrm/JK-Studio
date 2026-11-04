# Notizen rc609 L: Licht geht durch Patches

Auftrag `auftrag_rc609_l.md` (Plan L, Tester über shank: „Sometimes light goes through patches“). Worktree `Y:\patchlicht`, Zweig `rc609l`, Läufe `Y:\patchlicht\laeufe\l_*` (eigenes `lauf.py`, Kopie von `SP/ab/lauf.py`).

## 1: Nachgestellt (rot) und Ursache

- Selbsttest `patchlicht` (neu, `gui/selbsttest_patchlicht.inc`): Testkarte im Laufordner, sechs geschlossene Kisten (innen 512 x 512 x 384), je ein Licht 64 unter der Decke; dazwischen Bogen-Patch (3 x 3, Scheitel 192), flacher Patch (z 160), Brush, nichts (Bezug), Bogen in `func_group`, flacher Patch in `func_group` mit `"_cs" "0"`. Gemessen: Boden unter der Mitte als Anteil am Boden der leeren Kiste - Lightmap der .bsp (Schnell-Backen, voller Bau) und Bildpunkte (Echtzeit-Licht, Schnell-Backen, „Gebaute Karte wie im Spiel“ aus Schnell-Backen und vollem Bau), Schalter „Patch shadows“ aus und an.
- **Rot** (`l_rot`, rc608-Verhalten hinter derselben Schnittstelle): 4 OK, 7 FEHLER.
  - **Hauptursache („sometimes“): der volle Bau nahm den Schalter nicht mit.** Mit Schalter AN zeigen Echtzeit-Licht und Schnell-Backen den Schatten, der volle Bau mit `[Test] Quick and Crude` (ebenso `Single: light stage`, `[Test] Floodlight` und alle JK2-Einträge - nur `[Final] ...` haben in NRCs Vorgabe `-patchshadows`) lässt das Licht durch: Lightmap unter Bogen, flachem Patch und Gruppe 73 = 1,00 der leeren Kiste (Brush 0,0). Licht-Schritt `-light -fast -nogrid` ohne `-patchshadows`.
  - Echtzeit-Licht: `func_group` mit `"_cs" "0"` warf Schatten (Bildpunkt 0 statt 43), q3map2 nicht (Schnell-Backen: 73 = 1,00). `entitaetWirft` setzte Welt und `func_group` fest auf 1; q3map2 liest `_castShadows`/`_cs` auch dort (`map.cpp ParseEntityCompileParams` + `bspfile_abstract.cpp GetEntityShadowFlags`).
  - Echtzeit-Licht: Schattennetz der Patches aus der Editor-Unterteilung (36 Dreiecke statt 34; duel4 3236 statt 6956), nicht aus `TessellatedMesh(ds, IterationsForCurve(longestCurve, 8))` wie `light_trace.cpp PopulateWithBSPModel`.
  - Kein Hinweis im Bau, Schalter nicht in der Karte.
- Nicht die Ursache (gemessen bzw. geprüft): Schnell-Backen (gibt `-patchshadows` mit, grün schon vorher), GPU-Weg (`-bhedgpu*` nehmen die Dreiecke aus dem Trace-Baum, der Patches nur mit `patchShadows` enthält), Rückseiten (Schattenkarten `CULL_NONE`), `surfaceparm trans`/`nodraw` (wie q3map2: kein Schatten), der Shader-Schlüssel `q3map_patchShadows` (wird in q3map2 gelesen, aber nirgends benutzt - wie im Original, nicht „verbessert“).

## 2: Behoben

- **Schalter als Einstellung der Karte:** worldspawn `"_bhed_patchshadows" "1"/"0"` (`kPatchSchattenSchluessel`, `patchSchattenAn`/`patchSchattenSetzen` in `gui/app_karte_bau.cpp`), ein Rückgängig-Schritt; ohne Schlüssel gilt die Vorgabe aus den Einstellungen (`BhedPatchShadows`, beim Umschalten mitgesetzt). Echtzeit-Licht, Schnell-Backen und voller Bau lesen dieselbe Einstellung.
- **Voller Bau:** Schalter an -> jeder Licht-Schritt bekommt `-patchshadows` (`karte::befehlMitPatchSchatten`, wie `-bhedvisexact`). Schalter aus -> der Eintrag bleibt, wie er ist (NRC: `[Final]` mit, die anderen ohne).
- **Hinweis:** Licht-Schritt ohne `-patchshadows` bei einer Karte mit Patches -> Warnung in der Konsole und orange im Fortschrittsfenster während LIGHT (`patchHinweisMelden`, `KePatchSchattenHinweis`). Tooltip des Schalters um `KePatchSchattenKarteTip` ergänzt (in der Karte gespeichert, gilt für den vollen Bau, Lichtgitter übergeht Patches immer).
- **Echtzeit-Licht 1:1:** `entitaetWirft` wie `ParseEntityCompileParams`/`GetEntityShadowFlags`; Schattennetz der Patches über `echtPatchSteuernetz` + `echtNetzTesselieren(IterationsForCurve(longestCurve, 8))` + `MeshQuadIterator` (patchSubdivisions 8: `games.cpp game_sof2`, gilt für ja und jk2); entartete Patches fallen wie in `ParsePatch` weg.

## Wie im Original (nicht geändert, sichtbar gemacht)

- Ohne `-patchshadows` werfen Patches keinen Schatten (`light_trace.cpp:828`). Vorgabe des Schalters bleibt aus.
- NRCs Bau-Menü: nur `[Final] Fast`/`[Final] Extreme Quality` haben `-patchshadows`; der Schalter entfernt es dort nicht.
- Das Lichtgitter übergeht Patches immer (`ti.skipGrid`, `TraceTriangle` „FIXME this is an ugly hack“): Modelle unter einem Patch bleiben im Spiel hell - im Tooltip und im Handbuch genannt.
- `func_group`/Entities mit `_cs 0` werfen keinen Schatten, auch nicht mit `-patchshadows`.

## Tests

- `patchlicht` **grün** (`l_gruen`): 11 OK, 0 FEHLER. Schalter aus: Patches 1,00, Brush 0,00 in allen Wegen; an: Bogen, flacher Patch, Gruppe 0,00, `_cs 0` 1,00, Brush 0,00 - Echtzeit (Bild), Schnell-Backen (Lightmap und Bild), Spielbild Schnell-Backen, voller Bau (Lightmap, Licht-Schritt mit `-patchshadows`, kein Hinweis; aus: Hinweis), Spielbild voller Bau (die gebaute .bsp allein durch den Zeichner der Vorschau wie Teil 11). Schattennetz Echtzeit 34 Dreiecke = von Hand aus q3map2 nachgerechnet (Bogen longestCurve 384 -> 2 Iterationen -> 9 x 2 -> 16; flach 2; Gruppe 16; `_cs 0` keine).
- Gegenprobe `maps/mp/duel4.bsp` (124 Patches, diente nicht zum Finden), rückgewonnen mit geschätzten Lichtern, `[Test] Quick and Crude`: aus Hinweis, kein `-patchshadows`, 0 Patch-Dreiecke in Echtzeit; an `-patchshadows`, 6956 Patch-Dreiecke, Lightmap-Summe 7166417 -> 6559444, 2560 von 16384 Texeln um mehr als 10 % dunkler. Rot vorher: an == aus (7166417), kein Hinweis.
- Rotlauf mit Licht 1000, Grünlauf danach mit Licht 2500 (hellere Handbuchbilder); die Prüfungen sind Anteile an der leeren Kiste.
- ctest `kartebau`: `befehlMitPatchSchatten` / `lichtOhnePatchSchatten`.
- Handbuch: Seite „Karten-Editor: Tempo und Echtzeit-Licht“, Absatz „Licht durch Patches“ und Bilder `h_patchlicht_aus`/`h_patchlicht_an` (1920 x 1080, Kamerabild ohne Oberfläche: ein Bild für alle Sprachen, `NEUTRALE_BILDER`), 6 Sprachen.

- Schluss (einmal, parallel): Bau aller Ziele 0 Warnungen (frischer Build-Ordner `build-patchlicht`); ctest 202 von 202 (darin `karte_echtlicht`, `karte_bau`); `karteneditor` mit `BHED_KARTE_NUR=fotos` 78 OK, 0 FEHLER (`l_ke`); lint_sources, lint_fassung, lint_headers, lint_feldinit, lint_i18n, lint_i18n_erzeugt, lint_settings ok; Handbuch mit gen_handbuch.py (es/cs 0 Texte englisch). `patchlicht` grün `l_gruen` (mit Gegenprobe) und `l_gruen2` (Licht 2500, ohne Gegenprobe) 10/0.

## Offen

- Im Spiel nicht gestartet (Regeln). Der volle Bau ist bhed_q3map2 selbst; das Spielbild zeigt dieselben Lightmaps.
- Die Vorgabe `BhedPatchShadows` bleibt global für Karten ohne Schlüssel; eine Karte mit Schlüssel trägt ihn in die .bsp (worldspawn, vom Spiel übergangen).

### Changelog

- English: Map editor: "Patch shadows" is now saved in the map and also used by every full build (the light stage gets -patchshadows); a build without it warns when the map has patches. The real-time light follows q3map2 for `_cs` on func_group and divides patches like -light.
- Deutsch: Karten-Editor: „Patch-Schatten“ wird jetzt in der Karte gespeichert und gilt auch für jeden vollen Bau (der Licht-Schritt bekommt -patchshadows); ein Bau ohne warnt, wenn die Karte Patches hat. Das Echtzeit-Licht folgt q3map2 bei `_cs` an func_group und unterteilt Patches wie -light.
- 中文:地图编辑器:“面片阴影”现在保存在地图中,并用于每次完整编译(光照步骤加上 -patchshadows);地图有面片而编译不带它时会给出警告。实时光照在 func_group 的 `_cs` 上遵循 q3map2,并像 -light 一样细分面片。
- 日本語:マップエディター: 「パッチの影」がマップに保存され、すべてのフルビルドでも使われるようになりました (ライトの段階に -patchshadows が付きます)。パッチのあるマップをそれなしでビルドすると警告します。リアルタイムライトは func_group の `_cs` で q3map2 に従い、パッチを -light と同じように分割します。
- Español: Editor de mapas: "Sombras de patches" se guarda ahora en el mapa y también lo usa cada compilación completa (el paso de luz recibe -patchshadows); una compilación sin él avisa si el mapa tiene patches. La luz en tiempo real sigue a q3map2 con `_cs` en func_group y divide los patches como -light.
- Čeština: Editor map: „Stíny patchů“ se nyní ukládají v mapě a platí i pro každé úplné sestavení (krok světla dostane -patchshadows); sestavení bez nich upozorní, má-li mapa patche. Světlo v reálném čase se u `_cs` na func_group řídí q3map2 a dělí patche jako -light.
