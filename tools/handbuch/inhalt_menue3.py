# -*- coding: utf-8 -*-
"""Handbuchseiten des Menue-Editors, Stufe M3 (HUD-Editor, asiatische Texte, Saebelarten,
Haltung der Figur, Klaenge) und M4 (Video-Import, Ton, Lesbarkeit)."""
from inhalt_basis import T

MENUE_EDITOR_M3 = {
    "id": "menue_editor_m3",
    "gruppe": "start",
    "titel": T("Menu editor: HUD, Asian texts and sabers", "Menü-Editor: HUD, asiatische Texte und Lichtschwerter",
               "菜单编辑器:HUD、亚洲语言文本与光剑", "メニューエディター: HUD、アジア言語のテキスト、ライトセーバー"),
    "kurz": T(
        "Edit the HUD with example values like the game draws it, write your own texts in Korean, Japanese and Chinese, all Movie Duels blade styles and black blades, the character's pose and the sounds like in the game.",
        "Das HUD mit Beispielwerten so bearbeiten, wie das Spiel es zeichnet, eigene Texte auf Koreanisch, Japanisch und Chinesisch, alle Klingenarten von Movie Duels und schwarze Klingen, die Haltung der Figur und die Klänge wie im Spiel.",
        "用示例数值编辑 HUD,效果与游戏绘制一致;用韩语、日语和中文编写自定义文本;Movie Duels 的全部光刃样式与黑色光刃;角色姿态与声音与游戏一致。",
        "ゲームと同じ描き方で例の値を使って HUD を編集し、韓国語・日本語・中国語で独自テキストを書き、Movie Duels の全ブレードスタイルと黒いブレード、キャラクターの姿勢とサウンドをゲームと同じにします。"),
    "bloecke": [
        ("h2", "hud", T("HUD editor", "HUD-Editor", "HUD 编辑器", "HUD エディター")),
        ("img", "menue_hud", T("ui/hud.menu with health 70, armor 30, force 100 and the saber style medium - tics, numbers and crosshair drawn like the game.",
                               "ui/hud.menu mit Gesundheit 70, Rüstung 30, Macht 100 und dem Säbelstil mittel - Striche, Zahlen und Fadenkreuz wie im Spiel gezeichnet.",
                               "ui/hud.menu,生命值 70、护甲 30、原力 100、光剑风格为中等——刻度、数字和准星的绘制与游戏一致。",
                               "体力 70、アーマー 30、フォース 100、セーバースタイル中の ui/hud.menu。目盛り・数字・照準をゲームと同じように描きます。"), "breit"),
        ("p", T(
            "Open `ui/hud.menu` like any other menu. The game does not draw the HUD with the menu alone: the game code reads the position, colour and image of named items (`health_tic1` to `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) and draws tics, numbers and pictures with the player's values on top. The preview does exactly that: **{{MeHudWerte}}** in the properties has sliders for health, armor, force, ammo, the saber style and the crosshair; full tics are white, the last one fades with the rest, armor under a quarter blinks, ammo 0 turns red. Move or recolour an item and the HUD follows at once.",
            "Öffne `ui/hud.menu` wie jedes andere Menü. Das Spiel zeichnet das HUD nicht allein mit dem Menü: Der Spielcode liest Lage, Farbe und Bild benannter Items (`health_tic1` bis `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) und zeichnet darüber Striche, Zahlen und Bilder mit den Werten des Spielers. Genau das macht die Vorschau: **{{MeHudWerte}}** in den Eigenschaften hat Regler für Gesundheit, Rüstung, Macht, Munition, den Säbelstil und das Fadenkreuz; volle Striche sind weiß, der letzte blendet mit dem Rest aus, Rüstung unter einem Viertel blinkt, Munition 0 wird rot. Verschiebst oder färbst du ein Item, folgt das HUD sofort.",
            "像其他菜单一样打开 `ui/hud.menu`。游戏并不只靠菜单绘制 HUD:游戏代码读取具名项(`health_tic1` 至 `4`、`armor_tic…`、`force_tic…`、`ammo_tic…`、`healthamount`、`armoramount`、`forceamount`、`ammoamount`、`saberstyle_fast/medium/strong`、`frame`)的位置、颜色和图像,并用玩家的数值在其上绘制刻度、数字和图片。预览正是这样做的:属性中的 **{{MeHudWerte}}** 提供生命值、护甲、原力、弹药、光剑风格和准星的滑块;满刻度为白色,最后一格随余量淡出,护甲低于四分之一时闪烁,弹药为 0 时变红。移动项或改变颜色,HUD 会立即跟随。",
            "`ui/hud.menu` はほかのメニューと同じように開きます。ゲームはメニューだけで HUD を描くのではありません。ゲームコードが名前付き項目(`health_tic1`〜`4`、`armor_tic…`、`force_tic…`、`ammo_tic…`、`healthamount`、`armoramount`、`forceamount`、`ammoamount`、`saberstyle_fast/medium/strong`、`frame`)の位置・色・画像を読み、その上にプレイヤーの値で目盛り・数字・画像を描きます。プレビューはまさにそれを行います。プロパティの **{{MeHudWerte}}** には体力・アーマー・フォース・弾薬・セーバースタイル・照準のスライダーがあり、満タンの目盛りは白、最後の目盛りは残りに応じて薄くなり、4 分の 1 未満のアーマーは点滅し、弾薬 0 は赤になります。項目を動かしたり色を変えたりすると HUD がすぐに追従します。")),
        ("p", T(
            "This is reproduced for Jedi Academy single player. The multiplayer game code and Movie Duels draw their HUD differently (whole-number steps, HUD modes, block points); there the preview shows only the menus and says so.",
            "Nachgebildet ist das für Jedi Academy im Einzelspieler. Der Mehrspieler-Spielcode und Movie Duels zeichnen ihr HUD anders (ganzzahlige Schritte, HUD-Arten, Blockpunkte); dort zeigt die Vorschau nur die Menüs und sagt das.",
            "这适用于 Jedi Academy 单人模式。多人模式的游戏代码和 Movie Duels 以不同方式绘制 HUD(整数步长、HUD 模式、格挡点);此时预览只显示菜单并给出提示。",
            "再現しているのは Jedi Academy のシングルプレイです。マルチプレイのゲームコードと Movie Duels は HUD を別の方法で描きます(整数刻み、HUD モード、ブロックポイント)。その場合プレビューはメニューだけを表示し、その旨を示します。")),
        ("h2", "asiatisch", T("Texts in Korean, Japanese and Chinese", "Texte auf Koreanisch, Japanisch und Chinesisch", "韩语、日语和中文文本", "韓国語・日本語・中国語のテキスト")),
        ("img", "menue_texte_koreanisch", T("An own text in Korean, drawn from the game's Hangul glyph pages.",
                                            "Ein eigener Text auf Koreanisch, gezeichnet aus den Hangul-Glyphenseiten des Spiels.",
                                            "一段自定义韩语文本,使用游戏的韩文字形页绘制。",
                                            "独自の韓国語テキスト。ゲームのハングル字形ページから描画します。"), "breit"),
        ("p", T(
            "**{{MeTexte}}** also offers Korean, Taiwanese, Japanese and Chinese. Type normally; the file gets the encoding in which the game's font reads that language (KSC 5601, Big5, Shift-JIS, GB 2312) - other languages stay Latin-1 like the menus. Characters the game cannot show as one character (for example Hangul outside KSC 5601 or half-width katakana) are stored as `?` and listed below the table. With the preview language set to one of these, the preview draws the characters from the game's glyph pages (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) at the game's size, spacing and scale.",
            "**{{MeTexte}}** bietet auch Koreanisch, Taiwanesisch, Japanisch und Chinesisch. Tippe ganz normal; in die Datei kommt die Kodierung, in der die Schrift des Spiels diese Sprache liest (KSC 5601, Big5, Shift-JIS, GB 2312) - andere Sprachen bleiben Latin-1 wie die Menüs. Zeichen, die das Spiel nicht als ein Zeichen darstellen kann (etwa Hangul außerhalb von KSC 5601 oder Halbbreiten-Katakana), werden als `?` gespeichert und unter der Tabelle genannt. Steht die Vorschausprache auf einer dieser Sprachen, zeichnet die Vorschau die Zeichen aus den Glyphenseiten des Spiels (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) in Größe, Abstand und Maßstab des Spiels.",
            "**{{MeTexte}}** 也提供韩语、台湾繁体中文、日语和简体中文。正常输入即可;文件使用游戏字体读取该语言时的编码(KSC 5601、Big5、Shift-JIS、GB 2312)——其他语言与菜单一样保持 Latin-1。游戏无法作为单个字符显示的字符(例如 KSC 5601 之外的韩文或半角片假名)保存为 `?`,并列在表格下方。将预览语言设为这些语言之一时,预览会以游戏的大小、间距和缩放从游戏的字形页(`fonts/kor_…`、`jap_…`、`chi_…`、`tai_…`)绘制字符。",
            "**{{MeTexte}}** には韓国語・台湾語(繁体字)・日本語・中国語もあります。普通に入力すれば、ファイルにはゲームのフォントがその言語を読むエンコーディング(KSC 5601、Big5、Shift-JIS、GB 2312)で書き込まれます。ほかの言語はメニューと同じく Latin-1 のままです。ゲームが 1 文字として表示できない文字(KSC 5601 外のハングルや半角カタカナなど)は `?` として保存され、表の下に表示されます。プレビュー言語をこれらのいずれかにすると、ゲームの字形ページ(`fonts/kor_…`、`jap_…`、`chi_…`、`tai_…`)から、ゲームと同じ大きさ・間隔・倍率で文字を描きます。")),
        ("h2", "saebel", T("Movie Duels blade styles and black blades", "Klingenarten von Movie Duels und schwarze Klingen", "Movie Duels 光刃样式与黑色光刃", "Movie Duels のブレードスタイルと黒いブレード")),
        ("p", T(
            "With Movie Duels data the saber menu follows `cg_SFXSabers` for every style: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (default), 5 Original Trilogy, 6 Return of the Jedi, 7 The Force Awakens, 8 Unstable, 9 Rebels, 10 Clone Wars, 11 Maul - each with its own glow and core images, sizes (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) and shape. Unstable hilts always get the unstable blade. Black blades are drawn with their blend mode, so they darken what is behind them like in the game.",
            "Mit Movie-Duels-Daten folgt das Säbelmenü `cg_SFXSabers` für jede Art: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (Vorgabe), 5 Original-Trilogie, 6 Rückkehr der Jedi-Ritter, 7 Das Erwachen der Macht, 8 Instabil, 9 Rebels, 10 Clone Wars, 11 Maul - jede mit eigenen Leucht- und Kernbildern, Größen (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) und Form. Instabile Griffe bekommen immer die instabile Klinge. Schwarze Klingen werden mit ihrer Mischart gezeichnet und dunkeln wie im Spiel ab, was hinter ihnen liegt.",
            "使用 Movie Duels 数据时,光剑菜单对每种样式都遵循 `cg_SFXSabers`:0(Raven)、1 第一部、2 第二部、3 第三部、4 SFX(默认)、5 原三部曲、6 绝地归来、7 原力觉醒、8 不稳定、9 反抗军、10 克隆人战争、11 摩尔——各有自己的光晕和剑芯图像、大小(`cg_SFXSabersGlowSize…`、`cg_SFXSabersCoreSize…`)和形状。不稳定剑柄始终使用不稳定光刃。黑色光刃按其混合方式绘制,会像游戏中一样使背后变暗。",
            "Movie Duels のデータでは、セーバーメニューはすべてのスタイルで `cg_SFXSabers` に従います: 0(Raven)、1 エピソード I、2 エピソード II、3 エピソード III、4 SFX(既定)、5 オリジナル三部作、6 ジェダイの帰還、7 フォースの覚醒、8 不安定、9 反乱者たち、10 クローン・ウォーズ、11 モール。それぞれ固有の光と芯の画像、大きさ(`cg_SFXSabersGlowSize…`、`cg_SFXSabersCoreSize…`)、形があります。不安定な柄には常に不安定なブレードが付きます。黒いブレードは合成方法どおりに描かれ、ゲームと同じく背後を暗くします。")),
        ("h2", "figur", T("The character's pose and the sounds", "Haltung der Figur und Klänge", "角色姿态与声音", "キャラクターの姿勢とサウンド")),
        ("p", T(
            "The animation of a model (`model_g2anim`, in the character menu `ui_char_anim`) starts when the game sets it - when the menus load, and again when a script such as `uiScript character` builds the character - and runs at the game's frame timing, blended between two frames, looping or stopping like the game. In **{{MeBedienen}}** scripts read exactly the words the game reads: `uiScript` takes its name and the words that script needs, `exec` one word - commands after them run on even without a `;`. `exec \"music …\"` starts the background music like the console.",
            "Die Animation eines Modells (`model_g2anim`, im Figurenmenü `ui_char_anim`) beginnt, wenn das Spiel sie setzt - beim Laden der Menüs und erneut, wenn ein Skript wie `uiScript character` die Figur baut - und läuft im Bildtakt des Spiels, zwischen zwei Bildern gemischt, geschleift oder endend wie im Spiel. Im **{{MeBedienen}}** lesen Skripte genau die Wörter, die das Spiel liest: `uiScript` seinen Namen und die Wörter, die dieses Skript braucht, `exec` ein Wort - Befehle danach laufen auch ohne `;` weiter. `exec \"music …\"` startet die Hintergrundmusik wie die Konsole.",
            "模型的动画(`model_g2anim`,角色菜单中为 `ui_char_anim`)在游戏设置它时开始——菜单加载时,以及 `uiScript character` 等脚本构建角色时再次开始——并以游戏的帧节奏运行,在两帧之间混合,像游戏一样循环或结束。在 **{{MeBedienen}}** 中,脚本读取的词与游戏完全相同:`uiScript` 读取名称及该脚本所需的词,`exec` 读取一个词——即使没有 `;`,其后的命令也会继续执行。`exec \"music …\"` 像控制台一样启动背景音乐。",
            "モデルのアニメーション(`model_g2anim`、キャラクターメニューでは `ui_char_anim`)は、ゲームが設定したとき(メニューの読み込み時、および `uiScript character` などのスクリプトがキャラクターを組み立てたとき)に始まり、ゲームのフレーム間隔で 2 フレーム間を補間しながら、ゲームと同じようにループまたは終了します。**{{MeBedienen}}** では、スクリプトはゲームと同じ語を読みます。`uiScript` は名前とそのスクリプトに必要な語を、`exec` は 1 語を読み、`;` がなくても後続のコマンドは続けて実行されます。`exec \"music …\"` はコンソールと同じく BGM を開始します。")),
    ],
}

MENUE_EDITOR_M4 = {
    "id": "menue_editor_m4",
    "gruppe": "start",
    "titel": T("Menu editor: videos, sound and readability", "Menü-Editor: Videos, Ton und Lesbarkeit",
               "菜单编辑器:视频、声音与易读性", "メニューエディター: 動画、音、見やすさ"),
    "kurz": T(
        "Import videos the way the game can play them - size, aspect ratio, frame rate, sound, the shader with videoMap - get clear messages for missing codecs, switch the preview sounds off and read the toolbars with labels.",
        "Videos so importieren, wie das Spiel sie abspielen kann - Größe, Seitenverhältnis, Bildrate, Ton, der Shader mit videoMap -, klare Meldungen bei fehlenden Codecs, die Klänge der Vorschau ausschalten und die Leisten mit Beschriftung lesen.",
        "按游戏能够播放的方式导入视频——尺寸、宽高比、帧率、声音、带 videoMap 的着色器——缺少编解码器时给出明确提示,关闭预览声音,并可让工具栏显示文字。",
        "ゲームが再生できる形で動画を取り込みます(サイズ、縦横比、フレームレート、音声、videoMap 付きシェーダー)。コーデックが足りないときは分かりやすく知らせ、プレビューの音を消し、ツールバーに名前を表示できます。"),
    "bloecke": [
        ("h2", "video", T("Importing a video", "Ein Video importieren", "导入视频", "動画を取り込む")),
        ("img", "menue_video_dialog", T("The import dialog for a 1920 x 1080 video with sound: size 512 x 512, aspect ratio, frames per second, quality, sound (off by default), use in the menu and the estimated size.",
                                        "Der Import-Dialog für ein 1920 x 1080 großes Video mit Ton: Größe 512 x 512, Seitenverhältnis, Bilder pro Sekunde, Qualität, Ton (ab Werk aus), Verwendung im Menü und geschätzte Größe.",
                                        "一段 1920 x 1080 带声音视频的导入对话框:尺寸 512 x 512、宽高比、每秒帧数、质量、声音(默认关闭)、在菜单中的用途以及估算的大小。",
                                        "音声付き 1920 x 1080 動画の取り込みダイアログ: サイズ 512 x 512、縦横比、フレームレート、画質、音声(既定はオフ)、メニューでの使い方、見積もりサイズ。"), "breit"),
        ("p", T(
            "**{{MeVideoImport}}** reads MP4, WebM, AVI, WMV, MOV and MKV through Windows, animated GIFs, and RoQ files - a RoQ is taken over unchanged. Choose **{{MeVideoZielGroesse}}**: the game plays only sides that are powers of two, with at most 512 x 512 pixels in total (full-screen videos stop otherwise, larger videos overflow the game's video buffer); 512 x 512 is the default, like every Jedi Academy and Movie Duels video. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (the game stretches the video onto the element anyway), {{MeVideoBalken}} or {{MeVideoZuschneiden}}. Pictures are reduced by averaging all covered pixels, so fine patterns do not flicker. Below the options you see the length, the number of frames and the size the file will have.",
            "**{{MeVideoImport}}** liest MP4, WebM, AVI, WMV, MOV und MKV über Windows, animierte GIFs und RoQ-Dateien - ein RoQ wird unverändert übernommen. Wähle die **{{MeVideoZielGroesse}}**: Das Spiel spielt nur Seiten, die Zweierpotenzen sind, mit höchstens 512 x 512 Pixeln insgesamt (Vollbild-Videos brechen sonst ab, größere Videos laufen über den Videopuffer des Spiels); 512 x 512 ist die Vorgabe, wie bei allen Videos von Jedi Academy und Movie Duels. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (das Spiel streckt das Video ohnehin auf das Element), {{MeVideoBalken}} oder {{MeVideoZuschneiden}}. Verkleinert wird, indem alle überdeckten Pixel gemittelt werden - feine Muster flimmern nicht. Unter den Einstellungen stehen Länge, Bildanzahl und die Größe, die die Datei haben wird.",
            "**{{MeVideoImport}}** 通过 Windows 读取 MP4、WebM、AVI、WMV、MOV 和 MKV,也可读取动态 GIF 和 RoQ 文件——RoQ 会原样采用。选择 **{{MeVideoZielGroesse}}**:游戏只能播放边长为 2 的幂、总像素最多 512 x 512 的视频(否则全屏视频会中止,更大的视频会溢出游戏的视频缓冲区);默认 512 x 512,与 Jedi Academy 和 Movie Duels 的所有视频相同。**{{MeVideoEinpassen}}**:{{MeVideoStrecken}}(游戏本来就会把视频拉伸到元素上)、{{MeVideoBalken}} 或 {{MeVideoZuschneiden}}。缩小时会对覆盖到的所有像素取平均,细密图案不会闪烁。选项下方显示时长、帧数以及文件的大小。",
            "**{{MeVideoImport}}** は Windows を通じて MP4・WebM・AVI・WMV・MOV・MKV を読み、アニメーション GIF と RoQ ファイルも読みます。RoQ はそのまま取り込みます。**{{MeVideoZielGroesse}}** を選びます。ゲームが再生できるのは辺が 2 のべき乗で合計 512 x 512 ピクセル以下の動画だけです(そうでないと全画面動画は止まり、大きな動画はゲームの動画バッファからあふれます)。既定は Jedi Academy と Movie Duels の全動画と同じ 512 x 512 です。**{{MeVideoEinpassen}}**: {{MeVideoStrecken}}(ゲームはどのみち動画を要素に合わせて引き伸ばします)、{{MeVideoBalken}}、{{MeVideoZuschneiden}}。縮小は覆われたすべてのピクセルの平均で行うので、細かい模様がちらつきません。設定の下に長さ、フレーム数、ファイルの大きさが表示されます。")),
        ("img", "menue_video_m4", T("The imported video as background of background_video in the Jedi Academy main menu - the preview plays it like the game.",
                                    "Das importierte Video als Hintergrund von background_video im Hauptmenü von Jedi Academy - die Vorschau spielt es wie das Spiel.",
                                    "导入的视频作为 Jedi Academy 主菜单中 background_video 的背景——预览像游戏一样播放它。",
                                    "Jedi Academy のメインメニューで background_video の背景にした取り込み動画。プレビューはゲームと同じように再生します。"), "breit"),
        ("p", T(
            "A menu shows a video through a shader with `videoMap`. The import writes it for you: `menu/video/<name>` in `shaders/<package>.shader`, next to `video/<name>.roq`. **{{MeVideoVerwendung}}** decides what happens in the menu: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, behind all others) or **{{MeVideoAlsCinematic}}** - only multiplayer menus draw that keyword, single-player menus show nothing (the check says so). Shader, video and menu go together into the exported .pk3 and into **{{MeImSpielTesten}}**.",
            "Ein Menü zeigt ein Video über einen Shader mit `videoMap`. Der Import schreibt ihn für dich: `menu/video/<Name>` in `shaders/<Paket>.shader`, neben `video/<Name>.roq`. **{{MeVideoVerwendung}}** bestimmt, was im Menü passiert: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, hinter allen anderen) oder **{{MeVideoAlsCinematic}}** - dieses Schlüsselwort zeichnen nur Mehrspieler-Menüs, Einzelspieler-Menüs zeigen nichts (die Prüfung sagt das). Shader, Video und Menü kommen zusammen in die exportierte .pk3 und in **{{MeImSpielTesten}}**.",
            "菜单通过带 `videoMap` 的着色器显示视频。导入会替你写好:在 `shaders/<包>.shader` 中写入 `menu/video/<名称>`,与 `video/<名称>.roq` 放在一起。**{{MeVideoVerwendung}}** 决定菜单中的效果:**{{MeVideoAlsHintergrund}}**、**{{MeVideoNeuesElement}}**(640 x 480,位于所有其他元素之后)或 **{{MeVideoAlsCinematic}}**——只有多人菜单会绘制该关键字,单人菜单不显示任何内容(检查会提示)。着色器、视频和菜单会一起进入导出的 .pk3 和 **{{MeImSpielTesten}}**。",
            "メニューは `videoMap` 付きのシェーダーで動画を表示します。取り込みがそれを書きます: `shaders/<パッケージ>.shader` に `menu/video/<名前>`、その隣に `video/<名前>.roq`。**{{MeVideoVerwendung}}** でメニューでの扱いを決めます: **{{MeVideoAlsHintergrund}}**、**{{MeVideoNeuesElement}}**(640 x 480、ほかのすべての後ろ)、**{{MeVideoAlsCinematic}}**(このキーワードを描くのはマルチプレイヤーのメニューだけで、シングルプレイヤーのメニューには何も表示されません。チェックがそう知らせます)。シェーダー、動画、メニューはまとめて書き出しの .pk3 と **{{MeImSpielTesten}}** に入ります。")),
        ("h2", "ton", T("Sound", "Ton", "声音", "音")),
        ("p", T(
            "Menus always play their videos silently - the game starts them without sound. **{{MeVideoTon}}** is therefore off; switch it on for full-screen videos that the game plays with sound (for example `cinematic <name>` from the console or a script). The sound is stored like in the game's videos: 22050 Hz, mono or stereo, between the frames. A video without a sound track gets a note. **{{MeTonAn}}** / **{{MeTonAus}}** in the toolbar switches all preview sounds - focus and click sounds and the menu music - and stops sounds that are playing; the setting is remembered.",
            "Menüs spielen ihre Videos immer stumm - das Spiel startet sie ohne Ton. **{{MeVideoTon}}** ist deshalb aus; schalte es für Vollbild-Videos ein, die das Spiel mit Ton abspielt (zum Beispiel `cinematic <Name>` aus der Konsole oder einem Skript). Gespeichert wird der Ton wie in den Videos des Spiels: 22050 Hz, mono oder stereo, zwischen den Bildern. Ein Video ohne Tonspur bekommt einen Hinweis. **{{MeTonAn}}** / **{{MeTonAus}}** in der Leiste schaltet alle Klänge der Vorschau - Fokus- und Klickklänge und die Menümusik - und hält laufende an; die Einstellung bleibt gespeichert.",
            "菜单始终静音播放视频——游戏启动它们时不带声音。因此 **{{MeVideoTon}}** 默认关闭;对于游戏带声音播放的全屏视频(例如从控制台或脚本执行 `cinematic <名称>`)可以打开它。声音按游戏视频的方式保存:22050 Hz,单声道或立体声,位于帧之间。没有音轨的视频会得到提示。工具栏中的 **{{MeTonAn}}** / **{{MeTonAus}}** 切换预览中的所有声音——焦点音、点击音和菜单音乐——并停止正在播放的声音;该设置会被记住。",
            "メニューは動画を常に無音で再生します(ゲームは音なしで開始します)。そのため **{{MeVideoTon}}** はオフです。ゲームが音付きで再生する全画面動画(例: コンソールやスクリプトからの `cinematic <名前>`)ではオンにしてください。音声はゲームの動画と同じく 22050 Hz、モノラルまたはステレオで、フレームの間に保存されます。音声トラックのない動画には注意が表示されます。ツールバーの **{{MeTonAn}}** / **{{MeTonAus}}** はプレビューのすべての音(フォーカス音、クリック音、メニュー音楽)を切り替え、再生中の音も止めます。設定は保存されます。")),
        ("h2", "codecs", T("Missing codecs", "Fehlende Codecs", "缺少编解码器", "コーデックがないとき")),
        ("p", T(
            "If Windows cannot decode a video, the dialog says which codec it is and what helps: HEVC, AV1 and VP8/VP9 need a free extension from the Microsoft Store, AVI files with DivX/Xvid or unknown codecs should be converted to MP4 (H.264). Nothing is stored in that case - no empty video.",
            "Kann Windows ein Video nicht dekodieren, nennt der Dialog den Codec und was hilft: HEVC, AV1 und VP8/VP9 brauchen eine kostenlose Erweiterung aus dem Microsoft Store, AVI-Dateien mit DivX/Xvid oder unbekannten Codecs wandelst du in MP4 (H.264) um. Abgelegt wird dann nichts - kein leeres Video.",
            "如果 Windows 无法解码某个视频,对话框会说明是哪种编解码器以及解决办法:HEVC、AV1 和 VP8/VP9 需要 Microsoft Store 中的免费扩展,使用 DivX/Xvid 或未知编解码器的 AVI 文件请转换为 MP4(H.264)。此时不会保存任何内容——不会出现空视频。",
            "Windows が動画をデコードできない場合、ダイアログはどのコーデックか、どうすればよいかを示します。HEVC・AV1・VP8/VP9 は Microsoft Store の無料拡張機能が必要で、DivX/Xvid や不明なコーデックの AVI は MP4(H.264)に変換してください。その場合は何も保存しません(空の動画は作りません)。")),
        ("h2", "lesbar", T("Readability", "Lesbarkeit", "易读性", "見やすさ")),
        ("img", "menue_leiste_text", T("The toolbars with labels; eye and lock in the hierarchy are real symbols.",
                                       "Die Leisten mit Beschriftung; Auge und Schloss in der Hierarchie sind echte Symbole.",
                                       "带文字的工具栏;层级中的眼睛和锁是真正的图标。",
                                       "名前付きのツールバー。階層の目と鍵は本物のアイコンです。"), "breit"),
        ("p", T(
            "Every toolbar symbol names itself and what it does when you hover it; **{{MeEinstLeisteText}}** in the settings puts the name next to each symbol. In the hierarchy the eye hides an element in the preview only and the lock keeps it from being moved. Text never gets smaller than 75 % of the normal size (12 pixels), however far you zoom out with Ctrl and the mouse wheel.",
            "Jedes Symbol der Leisten nennt beim Darüberfahren seinen Namen und seine Wirkung; **{{MeEinstLeisteText}}** in den Einstellungen schreibt den Namen neben jedes Symbol. In der Hierarchie blendet das Auge ein Element nur in der Vorschau aus, das Schloss schützt es vor dem Verschieben. Die Schrift wird nie kleiner als 75 % der normalen Größe (12 Pixel), wie weit du auch mit Strg und dem Mausrad verkleinerst.",
            "将鼠标悬停在工具栏图标上,会显示其名称和作用;设置中的 **{{MeEinstLeisteText}}** 会在每个图标旁写上名称。在层级中,眼睛只在预览中隐藏元素,锁可防止元素被移动。无论用 Ctrl 和鼠标滚轮缩小多少,文字都不会小于正常大小的 75%(12 像素)。",
            "ツールバーのアイコンにマウスを重ねると名前と働きが表示されます。設定の **{{MeEinstLeisteText}}** で各アイコンの横に名前を出せます。階層では、目は要素をプレビューでだけ隠し、鍵は移動を防ぎます。Ctrl とマウスホイールでどれだけ縮小しても、文字は通常の 75%(12 ピクセル)より小さくなりません。")),
    ],
}

SEITEN = [MENUE_EDITOR_M3, MENUE_EDITOR_M4]
