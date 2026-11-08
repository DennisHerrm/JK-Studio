# Notizen PTFIX3: Painting-/model2scale-Funde und Chamfer-Feinheiten (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 31606ed3b)

Build `Y:/karte/build-test`, Laeufe `p3_*` unter `Y:\karte\laeufe`, Skripte und Protokolle `Y:\werkzeuge\p3\`.
Quelle der Funde: `JKStudio_Uebergabe/plaene/codepruefung_seit_rc610.md` (P23-P31).

## Punkt 1: Painting P23, P24, P29, P30, P31, P25, P26

Neue Selbsttest-Gruppe `modellblock` BHED_MODELL_NUR=ptfix3 (Sturmtruppler aus den Spieldaten, Yoda und Yoda-Schatten aus
Movie Duels), ctest `modell_malen` um `eigenschaftSpitzeTest` erweitert (ersetzter operator new zaehlt die Bytes).

- P23: Die Leinwaende haengen am Ladestand des Modelldokuments (`Dokument::ladeStand`, zaehlt nur `laden()`), nicht am Pfad.
  Dasselbe Modell nach "verwerfen" (oder nach einer Aenderung auf der Platte) neu geladen verwirft sie.
- P24: Je Detailstufe merkt die Leinwand die Dreiecke beim ersten Zeigen (`Ladestand` mit Herkunft Teil/Dreieck). "Auf neue UV
  umbacken" paart nur mit denselben Dreiecken derselben Stufe; sind es andere (Detach, neuer Teil), meldet es das
  (`PtUmbackenAnders`, 6 Sprachen in `tools/i18n_ptfix3.py`) statt "UV nicht geaendert".
- P29: Schluessel wie GenerateImageMappingName (klein, `\` -> `/`, bis zum ersten Punkt, hoechstens 63 Zeichen; rd-vanilla
  tr_image.cpp:95-111 OpenJK, MD9 :92-107); die Dateisuche wie R_LoadImage (genannte Endung zuerst, wenn es einen Leser gibt,
  dann jpg, png, tga; rd-common tr_image_load.cpp:106-140). Welche Datei die Leinwand traegt: die der ersten Registrierung im
  Spiel - R_LoadMDXM die Shader der Flaechen in ihrer Folge (tr_ghoul2.cpp:5404-5416 MD9), danach RE_RegisterSkin die Zeilen
  der Haut (tr_skin.cpp:290-337, neu `GlmSurface::hautZeile`), R_FindShader nimmt das Skript vor dem Bild.
  Gefunden in den Daten (Movie Duels): 8 Bilder mit zwei Endungen unter models/players (u. a. yoda/hair .tga/.png,
  captain_solo/hair, or_trooper/helmet). Yoda: Haut `hair.tga`, Skript `hair` mit `map models/players/yoda/hair` - das Spiel
  laedt hair.png (png vor tga); JK Studio malte vorher auf hair.tga (das Spiel liest es nie) und hatte eine zweite Leinwand.
- P30: Die Flaechen der Leinwand werden bei jedem Zugriff neu bestimmt (Detach/Grundkoerper in einen neuen Teil, Hautwechsel).
  Der Pinsel malt nur auf Flaechen, deren Shader das Bild in einer Stufe mit Grund-UV zeigt (tcGen base, kein tcMod); zeigen es
  alle nur verschoben, auf den Flaechen, deren eigenes Bild es ist. Die Vorschau (Stufen, Flaechenbild) bekommt weiter alle
  Flaechen, die das Bild zeigen. Yoda-Schatten: `smoke` nennen body/head/caps nur mit `tcMod scroll`, auch der Shader smoke
  selbst dreht es (`tcMod rotate`) - Pinsel jetzt auf den 312 Dreiecken der Rauchflaechen statt auf 4451.
- P31: `ModelTextures::ladeNummer` (jedes texturesFor eine neue, atomar) statt der Adresse `stufen.data()`.
- P25: Wiederholen als eine lineare Kette ueber alle Leinwaende: Schritte mit kleinerer Nummer als der juengste abgelegte sind
  verworfen.
- P26: `stapelAendern` vergleicht erst und kopiert dann; ein Eigenschafts-Schritt kopiert keine Bildpunkte.

| Pruefung | rot (Code vor der Behebung) | gruen |
|---|---|---|
| Selbsttest ptfix3 | `p3_ptfix3_rot` 3 OK / 9 FEHLER | `p3_ptfix3_g1` 12 OK / 0 |
| P25 Strg+Y nach neuem Strich | Helm 7336 Texel zurueck | Helm 0, Rumpf 2629 |
| P31 Adresse der Stufenliste nach 4 (gruen: 2) Ladevorgaengen gleich | Stufen mit dem Bild 0 von 3 | 2 von 3 (die dritte ist trooper-armor_spec) |
| P23 verwerfen | 2629 Texel, ungespeichert ja, zweite Frage | 0 Texel, nein, keine Frage |
| P24 Umbacken in Stufe 2 | "UV nicht geaendert", nichts gebacken | 277552 Texel gebacken, "umgebacken" |
| P30 Detach eines Rumpf-Polygons | Leinwand 2242 von 2243 Dreiecken | 2243 |
| P24 Umbacken nach Detach | "UV nicht geaendert" | "Dreiecke geaendert" |
| P29 Yoda head_hair | hair.tga und hair.png, zwei Leinwaende | eine Leinwand hair.png |
| P30 Yoda-Schatten smoke | 4451 Dreiecke | 312 = Rauchflaechen |
| ctest modell_malen P26 | `ct_malen_rot.txt` 32,00 MB Spitze | `ct_malen_g1.txt` 0,00 MB |

- Gegenlaeufe gruen: pt 41/0, ptfix 5/0, ptfix2 (ja) 5/0, ptstufen 4/0 (`p3_*_g1`).
- Handbuch Painting (6 Sprachen): Namen nur mit anderer Endung sind ein Bild, Pinsel nur ueber Stufen ohne tcGen/tcMod.
- Abweichung/offen: `textureCandidates` (Vorschau, Menue-Editor) versucht weiter jede genannte Endung zuerst und zusaetzlich
  `.jpeg` - das Spiel laedt nur jpg/png/tga (R_LoadImage). Fuer das Painting jetzt wie die Engine; die allgemeine Suche ist nicht
  Teil dieses Auftrags. Bei doppelten Hautzeilen derselben Flaeche nimmt applySkin die letzte (die Engine die erste).

## Changelog

## English

- Model tab, Painting: loading the same model again after "discard" really discards the painted images; "Bake onto new UVs" pairs only the same triangles of the same detail level and says so when the triangles have changed.
- Model tab, Painting: images whose names differ only in the extension are one image like in the game, painted into the file the game loads (Yoda's hair.png instead of hair.tga); the brush paints only on surfaces that show the image with their own UVs.
- Model tab, Painting: Redo after a new stroke no longer brings back a stroke on another image, new parts and skins join the canvas, reloaded textures always show the painted image, and toggling a layer property no longer copies all layers.

## Deutsch

- Model-Tab, Painting: Dasselbe Modell nach „verwerfen“ neu laden verwirft die bemalten Bilder wirklich; „Auf neue UV umbacken“ paart nur dieselben Dreiecke derselben Detailstufe und meldet geänderte Dreiecke.
- Model-Tab, Painting: Bilder, deren Namen sich nur in der Endung unterscheiden, sind wie im Spiel ein Bild und werden in die Datei gemalt, die das Spiel lädt (Yodas hair.png statt hair.tga); der Pinsel malt nur auf Flächen, die das Bild mit ihrer eigenen UV zeigen.
- Model-Tab, Painting: Wiederholen nach einem neuen Strich holt keinen Strich auf einem anderen Bild zurück, neue Teile und Häute gehören zur Leinwand, neu geladene Texturen zeigen immer das Gemalte, und eine Ebenen-Eigenschaft umschalten kopiert nicht mehr alle Ebenen.

## 中文

- 模型标签页, 绘制: 在"放弃"后重新加载同一模型会真正放弃已绘制的图像; "烘焙到新 UV"只配对同一细节级别中的相同三角形, 三角形改变时会提示。
- 模型标签页, 绘制: 名称仅扩展名不同的图像与游戏中一样是同一张图像, 绘制到游戏加载的文件中 (Yoda 的 hair.png 而不是 hair.tga); 画笔只绘制以自身 UV 显示该图像的表面。
- 模型标签页, 绘制: 新笔画后重做不再恢复另一张图像上的笔画, 新部件和皮肤会加入画布, 重新加载的纹理总是显示所绘内容, 切换图层属性不再复制所有图层。

## 日本語

- モデルタブ、ペイント: 「破棄」の後に同じモデルを再読み込みすると、塗った画像が本当に破棄されます。「新しい UV に焼き付け」は同じ詳細レベルの同じ三角形だけを対応させ、三角形が変わったときはそう表示します。
- モデルタブ、ペイント: 拡張子だけが違う名前の画像はゲームと同じく 1 枚の画像で、ゲームが読み込むファイルに塗られます (Yoda の hair.tga ではなく hair.png)。ブラシは自身の UV でその画像を表示するサーフェスにだけ塗ります。
- モデルタブ、ペイント: 新しいストロークの後のやり直しで別の画像のストロークが戻らなくなり、新しいパーツやスキンがキャンバスに加わり、再読み込みしたテクスチャは常に塗った内容を表示し、レイヤーの属性の切り替えで全レイヤーをコピーしなくなりました。

## Español

- Pestaña Model, Pintura: volver a cargar el mismo modelo después de "descartar" descarta de verdad las imágenes pintadas; "Hornear sobre UV nuevas" empareja solo los mismos triángulos del mismo nivel de detalle y avisa cuando los triángulos han cambiado.
- Pestaña Model, Pintura: las imágenes cuyos nombres solo se diferencian en la extensión son una sola imagen como en el juego y se pintan en el archivo que carga el juego (hair.png de Yoda en lugar de hair.tga); el pincel solo pinta en superficies que muestran la imagen con sus propias UV.
- Pestaña Model, Pintura: rehacer tras un trazo nuevo ya no recupera un trazo de otra imagen, las piezas y skins nuevas se unen al lienzo, las texturas recargadas siempre muestran lo pintado y cambiar una propiedad de capa ya no copia todas las capas.

## Čeština

- Karta Model, Malování: opětovné načtení stejného modelu po „zahodit“ namalované obrázky opravdu zahodí; „Přepéct na nové UV“ páruje jen stejné trojúhelníky stejné úrovně detailu a ohlásí změněné trojúhelníky.
- Karta Model, Malování: obrázky, jejichž názvy se liší jen příponou, jsou jako ve hře jeden obrázek a malují se do souboru, který hra načte (Yodův hair.png místo hair.tga); štětec maluje jen na plochy, které obrázek ukazují s vlastními UV.
- Karta Model, Malování: Znovu po novém tahu už nevrátí tah na jiném obrázku, nové části a skiny patří k plátnu, znovu načtené textury vždy ukazují namalované a přepnutí vlastnosti vrstvy už nekopíruje všechny vrstvy.
