# -*- coding: utf-8 -*-
"""Handbuchseite des Menue-Editors, Stufe M3: HUD-Editor, asiatische Texte, Saebelarten,
Haltung der Figur, Klaenge."""
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

SEITEN = [MENUE_EDITOR_M3]
