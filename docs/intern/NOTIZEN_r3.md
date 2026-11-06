# Notizen R3: Tester-Bugs 06.11. - Modelle innen nach Spiegeln, Glow im Echtzeit-Licht

Auftrag `auftrag_r3.md` (mit TD1, siehe `NOTIZEN_td1.md`). Direkt in `Y:\karte` auf `karte1` ab d93fad798 (Modell-Block 1a-1), Build `build-test`, Laeufe `laeufe/r3_*` (rot `r3_rot` mit `laeufe/r3_exe_rot` = d93fad798 plus Zaehler und Selbsttest r3; Schnell-Backen rot `r3_2`; gruen `r3_g1`, `r3_3`; Bildzeit `r3_zvor`/`r3_znach`, `r3_z2rot`/`r3_z2nach`; Handbuch `r3_hb_*`; Schluss `r3e_*`). Laufskript `lauf.py` wie rc610. kFassung bleibt rc610. Kein Spiel gestartet, radiant.exe nicht gestartet.

Selbsttest: `BHED_EDITORTEST=karteneditor BHED_KARTE_NUR=r3` (auch im vollen karteneditor-Lauf), `BHED_R3_ZEIT=1` nur die Bildzeit, Handbuch `BHED_HB_NUR=r3`. Testdaten als .pk3 im Laufordner (Extra Resource Path fuer den Editor, Spielordner der Sitzung fuer das Spielbild): ein Wuerfel als .md3 (md3Paket; Vorderseite rot, alle anderen Seiten blau) und der Glow-Shader des Testers in derselben Form (`textures/r3/licht`; `textures/md_concept/light` gibt es in den Movie-Duels-Daten nicht).

## 1. Gespiegelte Modelle ("renders inside")

- Was das Werkzeug schreibt: Flip X/Z (Select_FlipAxis -> scaleSelected -1, NRC) setzt `modelscale_vec` "-1 1 1" / "1 1 -1" / zweimal "-1 1 -1"; der Drehpunkt rastet auf das Raster (Ursprung abseits des Rasters wandert um den Rest - wie NRC, nicht geaendert).
- Quellen:
  - q3map2 `model.cpp:1216` `transform_lefthanded = MATRIX4_LEFTHANDED == matrix4_handedness( transform )`, `:1430-1432` tauscht je Dreieck zwei Indizes (auch die Clip-Dreiecke `:1446-1448`): eingebackene `misc_model` zeigen gespiegelt ihre Aussenseite.
  - NRC `radiant/renderstate.cpp:1561` `glFrontFace( RENDER_CULLFACE && matrix4_handedness == RIGHTHANDED ? GL_CW : GL_CCW )` je Renderable: NRC zeigt jedes gespiegelte Modell von aussen.
  - OpenJK `code/rd-vanilla/tr_backend.cpp:126-165` GL_Cull: Umkehr nur fuer `viewParms.isMirror`, keine je Entity; MD9 `code/rd-vanilla/tr_backend.cpp` GL_Cull gleich (verglichen). `code/cgame/cg_main.cpp:2043-2047` CG_DrawMiscEnts: AnglesToAxis + ScaleModelAxis mit `modelScale` - negative Achsen spiegeln die Achsen, die Wicklung dreht sich, das Spiel zeigt `misc_model_static` (und `misc_model_breakable`, `misc_model_ghoul`) gespiegelt von innen.
- Behoben: Kamera (`src/karte/kameraszene.cpp`): `misc_model` mit Determinante < 0 mit umgekehrter Wicklung (0, 2, 1) wie InsertModel - gilt fuer Editor-Licht, Echtzeit-Licht und die Modelle des Editors ueber "Gebaute Karte wie im Spiel". Laufzeit-Modelle bleiben wie im Spiel (innen) - bewusst anders als NRC. Echtzeit-Licht-Flaechen der misc_model (`echtModellFlaechen`, Schatten, Gras, Flaechenlicht) ebenso umgekehrt. Auswahl/Picking war schon NRC 1:1 (`auswahltest.cpp:328-330` nach Haendigkeit), 2D zeichnet Draht ohne Wegschneiden, Schatten zweiseitig - unveraendert.
- Kartenpruefung neu `modellgespiegelt` (Warnung): misc_model_static, misc_model_breakable, misc_model_ghoul mit Produkt der Achsen < 0, Skalierung wie die Spawn-Funktion (g_misc.cpp:408-415, g_breakable.cpp:1172-1180, g_misc_model.cpp:192-200).
- Gefunden unterwegs: das Schnell-Backen (Gebaute Karte wie im Spiel) gab q3map2 die Extra Resource Paths und den Import-Ordner nicht mit (`-fs_pakpath`, im Build-Menue `bauVariablen` "ExtraResourcePaths", NRC ToolXMLConstructor::flush) - Modelle von dort wurden nicht eingebacken. Jetzt dieselben Pfade.

## 2. Glow im Echtzeit-Licht

- Vorbild: RB_StageIteratorGeneric zeichnet alle Stufen; RB_DrawSurfs (`tr_backend.cpp:1335-1389`) zeichnet mit `g_bRenderGlowingObjects` die Stufen mit `glow` noch einmal in einen geleerten Farbpuffer mit der Tiefe der Szene, RB_BlurGlowTexture zeichnet weich, RB_DrawGlowOverlay legt mit `GL_ONE GL_ONE_MINUS_SRC_COLOR` (r_DynamicGlowSoft 1) bzw. `GL_ONE GL_ONE` auf.
- Umsetzung: `spielDarstellung` (gui/app_karte_echtlicht.cpp) liefert fuer jeden Shader die weiteren Stufen nach der Grundstufe (`basisStufen`: $lightmap und die Bildstufe, die mit ihr multipliziert, in beiden Reihenfolgen, sonst die erste; eine andere Form: keine) und davon die mit `glow` (`glueh`; dazu eine leuchtende Grundstufe ohne Lightmap). Gezeichnet wird nur, was die Ansicht genau kann (`stufeDarstellbar`: festes Bild, tcGen base ohne tcMod, rgbGen identity/identityLighting/const bzw. keins, alphaGen Bild/identity/const, keine Wellen) - der Rest bleibt wie bisher weg. Die Kamera zeichnet sie nach der Grundstufe (GL_LEQUAL, ohne Licht) und danach das Gluehen: `kartegpu::gluehenBeginnen/gluehenVoll/gluehenAuflegen` (eigener Puffer mit den Proben und der Tiefe des Ziels), Weichzeichnen mit demselben Code wie das Spielbild (`gpu::weichzeichnen`, aus `zeichneGluehen` herausgezogen, `gpu::gluehenWeichzeichnen` mit eigenen kleinen Puffern 320 x 240).
- Editor-Licht (ohne Echtzeit): zeigt wie NRC nur `qer_editorimage` - unveraendert (NRC zeichnet keine Stufen).
- Tooltip des Echtzeit-Lichts neu (`KeLichtGrenzenTip3`), Handbuch karte_tempo: Absatz, Bild `h_r3_glow` (6 Sprachen), Notiz zu gespiegelten Modellen.

## Rot und gruen

| Punkt | Pruefung | rot | gruen |
|---|---|---|---|
| 1 Spiegeln | r3: Anteil Rot je Wuerfel (A ohne, B Flip X, C Flip Z, D X+Z, E static, F static Flip X) | `r3_rot` (ganzer Wuerfel): Editor und Echtzeit B 0,000, C 0,000 (A 0,739, D 0,601 mit Ober-/Seitenflaechen im Ausschnitt) | `r3_3` 22/0, Vorderseite: Editor, Echtzeit, Spielbild mit Editor-Modellen und reines Spielbild A-E 1,000, F 0,000 (wie im Spiel innen) |
| 1 Kartenpruefung | Befund `modellgespiegelt` genau 1 (F) | `r3_rot`: keiner | `r3_3`: modellgespiegelt 1 |
| 1 Schnell-Backen | Wuerfelshader in der schnell.bsp | `r3_2`: fehlt (Spielbild ohne alle Wuerfel) | `r3_3`: drin, reines Spielbild wie oben |
| 2 Glow | Platte mit dem Shader des Testers: Gelb der Platte, Schein daneben (6-40 Bildpunkte) | `r3_rot`: Echtzeit 0,0 / 2,0, Spielbild 143,1 / 78,2 | `r3_3`: Echtzeit 142,1 / 79,1 (Verhaeltnis 0,993 / 1,012), 1 Glow-Aufruf, 1 weitere Stufe; Editor-Licht 0,0 (wie NRC) |

Toleranz Glow 15 % (Gelb) und 25 % (Schein): die Grundstufe ist im Echtzeit-Licht das Licht je Bildpunkt, im Spielbild die Lightmap von -light -fast mal Spielgamma; Weichzeichnen und Auflegen sind derselbe Code bzw. dieselbe Formel. Gemessen 0,7 % und 1,2 %.

## Bildzeit (allein, BHED_D3D_DEBUG=0, 240 Bilder mit drehender Kamera; die Ansicht laeuft mit der Bildwiederholung)

| Szene | vorher (`r3_exe_rot`) | nachher |
|---|---|---|
| Glow-Platte, Echtzeit-Licht | 4,54 / 11,20 ms (ohne Glow) | 4,42 / 8,97 ms (mit Stufe und Gluehen) |
| R2a-Grasflaeche ohne Glow: Ecke ohne/mit Gras | 4,19 / 8,76, 4,53 / 10,16 ms | 4,27 / 9,99, 4,54 / 10,45 ms |
| dto. Mitte ohne/mit Gras | 4,42 / 8,85, 4,42 / 9,78 ms | 4,39 / 9,98, 4,87 / 10,57 ms |

Im Rauschen der Bildwiederholung (~4,2 ms); das Spielbild ist bytegleich gerechnet (zeichneGluehen ruft dieselben Durchgaenge mit denselben Werten).

## Schluss

- Sauberer Neubau (`--clean-first`, `r3e_bau.txt`) 0 Fehler, 0 Warnungen (/W4 /WX).
- ctest 221/221 (`r3e_ctest.txt`, -j 16).
- Selbsttests parallel auf `laeufe/r3e_exe` (11 gleichzeitig): karteneditor fotos 78/0, r2a 22/0, modobj 25/0, r3 22/0, licht1 15/0; editorportal 19/0; menueeditor 2781/2 (3 uebersprungen, Daten fehlen) - die zwei F46 B6-1 Hierarchie unter Last rot, allein `r3e_me_sw` 5/0 (wie "unter Last rot, allein gruen" der rc610-Endrunde); gluehen 9/0, karte 233/0, teil6 15/0; Modus 1 2651/0.
- Lints wie vorher (portability 99, pruefcodes 11, fremdprogramm 1, efxflags "keine Flagtabelle" - app_view3d.cpp nicht beruehrt); Handbuch `gen_handbuch.py`: es/cs 0 Texte, 0 Bilder englisch.
- Handbuchbild `h_r3_glow` 6 Sprachen (BHED_HB_FENSTER=3840x2160@1.5, 1989 bis 2055 breit, angesehen).

## Offen

- Gluehen in der Spiegel-/Portalsicht und von Effekten (fx_runner) im Echtzeit-Licht nicht nachgebaut; Stufen mit tcMod, tcGen, Wellen, animMap mit mehreren Bildern und rgbGen vertex/lightingDiffuse fehlen im Echtzeit-Licht weiter (bis zum Bau).
- "Gebaute Karte wie im Spiel" legt die Modelle des Editors ueber das Spielbild (seit rc608 Teil 11/13b) - gleich gezeichnet wie im Spiel, aber nicht das Bild des Spiels selbst.
- Brush-Entities mit `model2` (md3 zur Laufzeit) zeichnet die Kamera nicht (wie NRC) - nicht geprueft.
- app_wetter.cpp: mehrere fx_lava einer Karte geben je eine Wolke, das Spiel nur eine (G_FindConfigstringIndex) - nicht nachgestellt, nicht geaendert.

## Changelog

### English
- Map editor: models mirrored with Flip X/Y/Z no longer show their inside. A misc_model is turned around like q3map2 bakes it, in the camera with and without real-time light and in the shadows of the real-time light; models the game loads at run time (misc_model_static, misc_model_breakable) stay as the game shows them, and the map check warns about them.
- Real-time light: further shader stages (such as a GL_ONE GL_ONE glow texture) are drawn like in the game, and stages with glow glow - blurred and laid over with the same code as "Built map as in the game".
- Quick bake: models from the extra resource paths and the import folder are baked in, like in the build menu.

### Deutsch
- Karten-Editor: mit Flip X/Y/Z gespiegelte Modelle zeigen nicht mehr ihr Inneres. Ein misc_model wird umgedreht, wie q3map2 es einbackt - in der Kamera mit und ohne Echtzeit-Licht und in den Schatten des Echtzeit-Lichts; Modelle, die das Spiel zur Laufzeit lädt (misc_model_static, misc_model_breakable), bleiben, wie das Spiel sie zeigt, und die Kartenprüfung warnt davor.
- Echtzeit-Licht: weitere Shaderstufen (etwa eine Leuchttextur mit GL_ONE GL_ONE) werden wie im Spiel gezeichnet, und Stufen mit glow leuchten - weichgezeichnet und aufgelegt mit demselben Code wie „Gebaute Karte wie im Spiel“.
- Schnell-Backen: Modelle aus den Extra Resource Paths und dem Import-Ordner werden eingebacken, wie im Build-Menü.

### 中文
- 地图编辑器:用 Flip X/Y/Z 镜像的模型不再显示其内侧。misc_model 会像 q3map2 烘焙时那样翻转——在开启或关闭实时光照的摄像机中以及实时光照的阴影中;游戏在运行时加载的模型(misc_model_static、misc_model_breakable)保持游戏中的样子,地图检查会对此发出警告。
- 实时光照:后续着色器阶段(例如 GL_ONE GL_ONE 的发光纹理)按游戏方式绘制,带 glow 的阶段会发光——用与“已构建的地图如游戏中显示”相同的代码模糊并叠加。
- 快速烘焙:与构建菜单相同,会烘焙额外资源路径和导入文件夹中的模型。

### 日本語
- マップエディター: Flip X/Y/Z で反転したモデルの内側が見えなくなりました。misc_model は q3map2 が焼き込むときと同じように裏返されます (リアルタイムライトの有無にかかわらずカメラで、またリアルタイムライトの影でも)。ゲームが実行時に読み込むモデル (misc_model_static、misc_model_breakable) はゲームと同じ表示のままで、マップチェックが警告します。
- リアルタイムライト: 後続のシェーダーステージ (GL_ONE GL_ONE のグローテクスチャなど) がゲームと同じように描かれ、glow 付きのステージが光ります。ぼかしと合成は「ビルドしたマップをゲームと同じ表示に」と同じコードです。
- クイックベイク: ビルドメニューと同じく、追加リソースパスとインポートフォルダーのモデルも焼き込まれます。

### Español
- Editor de mapas: los modelos reflejados con Flip X/Y/Z ya no muestran su interior. Un misc_model se da la vuelta como lo integra q3map2, en la cámara con y sin luz en tiempo real y en las sombras de la luz en tiempo real; los modelos que el juego carga en tiempo de ejecución (misc_model_static, misc_model_breakable) se quedan como los muestra el juego y la comprobación del mapa avisa de ellos.
- Luz en tiempo real: las etapas siguientes del shader (como una textura de brillo GL_ONE GL_ONE) se dibujan como en el juego y las etapas con glow brillan, desenfocadas y superpuestas con el mismo código que «Mapa compilado como en el juego».
- Horneado rápido: los modelos de las rutas de recursos adicionales y de la carpeta de importación se integran, como en el menú de compilación.

### Čeština
- Editor map: modely zrcadlené pomocí Flip X/Y/Z už neukazují svůj vnitřek. misc_model se otočí, jak ho zapeče q3map2 - v kameře se světlem v reálném čase i bez něj a ve stínech světla v reálném čase; modely, které hra načítá za běhu (misc_model_static, misc_model_breakable), zůstanou, jak je ukazuje hra, a kontrola mapy na ně upozorní.
- Světlo v reálném čase: další fáze shaderu (například zářící textura GL_ONE GL_ONE) se kreslí jako ve hře a fáze s glow září - rozmazané a položené stejným kódem jako „Sestavená mapa jako ve hře“.
- Rychlé zapečení: modely z dalších cest ke zdrojům a ze složky importu se zapečou, jako v nabídce sestavení.
