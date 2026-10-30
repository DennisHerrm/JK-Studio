# -*- coding: utf-8 -*-
"""Handbuchseite des Menue-Editors, Stufe M6: Menuefluss, Tab-Reihenfolge, Stile, Vorlagen ganzer
Menues, Einstellungs-Assistent, Referenzbild, Vergleich mit dem Original und Live-Grenzen."""
from inhalt_basis import T

MENUE_EDITOR_M6 = {
    "id": "menue_editor_m6",
    "gruppe": "start",
    "titel": T("Menu editor: flow, tab order, styles and templates", "Menü-Editor: Fluss, Tab-Reihenfolge, Stile und Vorlagen",
               "菜单编辑器:流程、Tab 顺序、样式与模板", "メニューエディター: 流れ、Tab の順序、スタイル、テンプレート"),
    "kurz": T(
        "See which menu opens which and where players get stuck, set the keyboard order with clicks, change colours and fonts everywhere at once, start from ready-made menus or a settings wizard, trace a design over a reference image and reset elements to the game's original.",
        "Sehen, welches Menü welches öffnet und wo Spieler festhängen, die Tastatur-Reihenfolge per Klick festlegen, Farben und Schriften überall auf einmal ändern, mit fertigen Menüs oder dem Einstellungs-Assistenten beginnen, einen Entwurf über einem Referenzbild nachbauen und Elemente auf das Original des Spiels zurücksetzen.",
        "查看哪个菜单打开哪个菜单以及玩家会卡在哪里,用单击设置键盘顺序,一次性修改所有颜色和字体,从现成的菜单或设置向导开始,照着参考图重建设计,并把元素重置为游戏原版。",
        "どのメニューがどのメニューを開き、プレイヤーがどこで行き詰まるかを確認し、クリックでキーボードの順序を決め、色やフォントを一度に全部変え、出来上がったメニューや設定ウィザードから始め、参考画像の上でデザインを再現し、要素をゲームのオリジナルに戻せます。"),
    "bloecke": [
        ("h2", "fluss", T("Menu flow", "Menüfluss", "菜单流程", "メニューの流れ")),
        ("img", "menue_fluss", T("The flow of a small menu set: the main menu (green frame) opens Options and Credits; Credits is a dead end (red), 'waise' is never opened (grey).",
                                 "Der Fluss eines kleinen Menüsatzes: Das Hauptmenü (grüner Rahmen) öffnet Optionen und Credits; Credits ist eine Sackgasse (rot), „waise“ wird nie geöffnet (grau).",
                                 "一个小菜单集的流程:主菜单(绿色边框)打开 Options 和 Credits;Credits 是死胡同(红色),“waise”从未被打开(灰色)。",
                                 "小さなメニューセットの流れ: メインメニュー(緑の枠)が Options と Credits を開きます。Credits は行き止まり(赤)、「waise」は一度も開かれません(灰色)。"), "breit"),
        ("p", T(
            "**{{MeFluss}}** in the toolbar shows every menu of the open files as a box and every `open` as an arrow, column by column from the menus the game opens itself (green frame: the main menu, the loading screen, the in-game menus - taken from the engine code). Red boxes are dead ends - they take input but nothing leads out - or targets that no open file defines; orange boxes are opened from another menu but have no way back (no `close` of themselves, no onESC, no `open` of the menu they came from); grey boxes are never reached. The list below says in plain words what to do. Click a box or a line of the list to jump to that menu. Open the whole menu set first, otherwise menus from other files look missing.",
            "**{{MeFluss}}** in der Leiste zeigt jedes Menü der offenen Dateien als Kasten und jedes `open` als Pfeil, Spalte für Spalte ab den Menüs, die das Spiel selbst öffnet (grüner Rahmen: Hauptmenü, Ladebildschirm, die Menüs im Spiel - aus dem Engine-Code). Rote Kästen sind Sackgassen - sie nehmen Eingaben an, aber nichts führt hinaus - oder Ziele, die keine offene Datei hat; orange Kästen werden aus einem anderen Menü geöffnet, haben aber keinen Weg zurück (kein `close` ihrer selbst, kein onESC, kein `open` des Menüs, aus dem sie kommen); graue Kästen erreicht man nie. Die Liste darunter sagt in normalen Worten, was zu tun ist. Ein Klick auf einen Kasten oder eine Zeile springt in das Menü. Öffne vorher den ganzen Menüsatz, sonst wirken Menüs aus anderen Dateien fehlend.",
            "工具栏中的 **{{MeFluss}}** 把已打开文件中的每个菜单显示为方框,每个 `open` 显示为箭头,从游戏自己打开的菜单开始逐列排列(绿色边框:主菜单、加载画面、游戏内菜单——取自引擎代码)。红色方框是死胡同(接受输入但没有出口)或没有任何已打开文件定义的目标;橙色方框由其他菜单打开,但没有返回路径(没有关闭自身的 `close`、没有 onESC,也不 `open` 来源菜单);灰色方框永远无法到达。下方列表用通俗的话说明该怎么做。单击方框或列表中的一行即可跳转到该菜单。请先打开整个菜单集,否则其他文件中的菜单会显示为缺失。",
            "ツールバーの **{{MeFluss}}** は、開いているファイルのメニューを箱、`open` を矢印として、ゲームが自分で開くメニュー(緑の枠: メインメニュー、ロード画面、ゲーム内メニュー。エンジンのコードから)から列ごとに表示します。赤い箱は行き止まり(入力は受け付けるが出口がない)か、どのファイルにもない移動先です。オレンジの箱はほかのメニューから開かれるのに戻る手段がありません(自分を閉じる `close`、onESC、元のメニューを開く `open` のどれもない)。灰色の箱にはたどり着けません。下の一覧に何をすればよいかが書かれています。箱か一覧の行をクリックするとそのメニューへ移動します。先にメニューセット全体を開いてください。そうしないとほかのファイルのメニューが見つからないように見えます。")),
        ("h2", "tabfolge", T("Tab order", "Tab-Reihenfolge", "Tab 顺序", "Tab の順序")),
        ("img", "menue_tabfolge", T("The numbers show the order in which Tab and Arrow down move the focus in the game.",
                                    "Die Nummern zeigen, in welcher Reihenfolge Tab und Pfeil runter im Spiel den Fokus weitergeben.",
                                    "编号表示游戏中 Tab 和向下箭头移动焦点的顺序。",
                                    "番号は、ゲーム内で Tab と下矢印がフォーカスを移す順序です。"), "breit"),
        ("p", T(
            "**{{MeTabFolge}}** puts a number on every element that can take the keyboard focus, in the order the engine uses (Menu_SetNextCursorItem: the order of the itemDefs, without decorations, hidden elements and elements switched off by a cvar; from no focus the game starts at the second element of the file). To change it, click the elements one after another in the new order - after the last click the editor rearranges the itemDefs (one Undo step). The game draws in the same order, so a later element lies over an earlier one; elements that cannot take the focus stay where they are.",
            "**{{MeTabFolge}}** setzt auf jedes Element, das den Tastaturfokus nehmen kann, eine Nummer in der Reihenfolge der Engine (Menu_SetNextCursorItem: die Reihenfolge der itemDefs, ohne Dekorationen, versteckte und per Cvar abgeschaltete Elemente; ohne Fokus beginnt das Spiel beim zweiten Element der Datei). Zum Ändern klickst du die Elemente nacheinander in der neuen Reihenfolge an - nach dem letzten Klick stellt der Editor die itemDefs um (ein Rückgängig-Schritt). Das Spiel zeichnet in derselben Reihenfolge, ein späteres Element liegt also über einem früheren; Elemente ohne Fokus bleiben an ihrer Stelle.",
            "**{{MeTabFolge}}** 按引擎使用的顺序,在每个可以获得键盘焦点的元素上显示编号(Menu_SetNextCursorItem:itemDef 的顺序,不含装饰、隐藏元素和被 cvar 关闭的元素;没有焦点时,游戏从文件中的第二个元素开始)。要修改顺序,请按新顺序依次单击元素——最后一次单击后编辑器会重新排列 itemDef(一步撤销)。游戏按相同顺序绘制,因此后面的元素会覆盖前面的元素;不能获得焦点的元素保持原位。",
            "**{{MeTabFolge}}** は、キーボードのフォーカスを受け取れる要素に、エンジンが使う順序で番号を付けます(Menu_SetNextCursorItem: itemDef の順序。装飾、非表示、cvar で無効な要素は除く。フォーカスがないとき、ゲームはファイルの 2 番目の要素から始めます)。変更するには、新しい順序で要素を順にクリックします。最後のクリックでエディターが itemDef を並べ替えます(元に戻す 1 回分)。ゲームは同じ順序で描くので、後の要素が前の要素の上に重なります。フォーカスを受け取れない要素はそのままです。")),
        ("h2", "stile", T("Styles", "Stile", "样式", "スタイル")),
        ("img", "menue_stile", T("All colours, fonts, text sizes and backgrounds of the file with their number of uses.",
                                 "Alle Farben, Schriften, Textgrößen und Hintergründe der Datei mit der Zahl ihrer Verwendungen.",
                                 "文件中所有颜色、字体、文字大小和背景及其使用次数。",
                                 "ファイル内のすべての色、フォント、文字サイズ、背景と、その使用回数。"), "breit"),
        ("p", T(
            "**{{MeStile}}** collects every colour (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), font, text size and background of the file - the most used first. Change a value and every place that uses it changes at once, as ordinary values in the .menu (spacing, comments and quotes stay; one Undo step). Values from #include files or macros are counted but stay as they are.",
            "**{{MeStile}}** sammelt jede Farbe (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), Schrift, Textgröße und jeden Hintergrund der Datei - die häufigsten zuerst. Änderst du einen Wert, ändert er sich an jeder Stelle auf einmal, als normale Werte im .menu (Abstände, Kommentare und Anführungszeichen bleiben; ein Rückgängig-Schritt). Werte aus #include-Dateien oder Makros werden mitgezählt, bleiben aber, wie sie sind.",
            "**{{MeStile}}** 收集文件中的所有颜色(forecolor、backcolor、bordercolor、outlinecolor、focusColor、disableColor、descColor)、字体、文字大小和背景——最常用的排在前面。修改一个值,所有使用它的地方都会一起改变,在 .menu 中仍是普通的值(间距、注释和引号保持不变;一步撤销)。来自 #include 文件或宏的值会被计数,但保持不变。",
            "**{{MeStile}}** は、ファイルのすべての色(forecolor、backcolor、bordercolor、outlinecolor、focusColor、disableColor、descColor)、フォント、文字サイズ、背景を、よく使われる順に集めます。値を変えると、それを使うすべての場所が一度に変わり、.menu には普通の値として入ります(間隔、コメント、引用符はそのまま。元に戻す 1 回分)。#include ファイルやマクロの値は数えますが、変更しません。")),
        ("h2", "vorlagen", T("Templates and settings wizard", "Vorlagen und Einstellungs-Assistent", "模板与设置向导", "テンプレートと設定ウィザード")),
        ("img", "menue_vorlage", T("A settings page with three tabs from the template - the tabs appear as pages above the canvas.",
                                   "Eine Einstellungsseite mit drei Reitern aus der Vorlage - die Reiter erscheinen als Seiten über der Fläche.",
                                   "由模板生成的带三个标签页的设置页——标签页显示为画布上方的页面。",
                                   "テンプレートから作った 3 つのタブ付き設定ページ。タブはキャンバスの上にページとして表示されます。"), "breit"),
        ("p", T(
            "The **+** next to the pages offers **{{MeVorlageLeer}}** or a whole menu: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** and **{{MeVorlageLaden}}**. They use only keywords every game version reads; the main menu and the loading screen get the names the engine looks for (single player `mainMenu` and `loadscreen`, multiplayer `main` and `Connect`), and the main menu opens the settings and credits templates - the menu flow shows the result without problems.",
            "Das **+** neben den Seiten bietet **{{MeVorlageLeer}}** oder ein ganzes Menü an: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** und **{{MeVorlageLaden}}**. Sie nutzen nur Schlüsselwörter, die jede Spielfassung liest; Hauptmenü und Ladebildschirm bekommen die Namen, nach denen die Engine sucht (Einzelspieler `mainMenu` und `loadscreen`, Mehrspieler `main` und `Connect`), und das Hauptmenü öffnet die Vorlagen für Einstellungen und Credits - der Menüfluss zeigt das Ergebnis ohne Probleme.",
            "页面旁的 **+** 提供 **{{MeVorlageLeer}}** 或整个菜单:**{{MeVorlageHaupt}}**、**{{MeVorlageEinst}}**、**{{MeVorlageCredits}}** 和 **{{MeVorlageLaden}}**。它们只使用每个游戏版本都能读取的关键字;主菜单和加载画面使用引擎查找的名称(单人 `mainMenu` 和 `loadscreen`,多人 `main` 和 `Connect`),主菜单会打开设置和制作人员名单模板——菜单流程显示的结果没有问题。",
            "ページの横の **+** で **{{MeVorlageLeer}}** か、メニュー全体を追加できます: **{{MeVorlageHaupt}}**、**{{MeVorlageEinst}}**、**{{MeVorlageCredits}}**、**{{MeVorlageLaden}}**。どのゲームのバージョンでも読めるキーワードだけを使います。メインメニューとロード画面にはエンジンが探す名前(シングル `mainMenu` と `loadscreen`、マルチ `main` と `Connect`)が付き、メインメニューは設定とクレジットのテンプレートを開きます。メニューの流れには問題なく表示されます。")),
        ("img", "menue_assistent", T("The settings wizard knows the controls of the game's own menus.", "Der Einstellungs-Assistent kennt die Steuerungen aus den Menüs des Spiels.",
                                     "设置向导了解游戏自身菜单中的控件。", "設定ウィザードはゲーム自身のメニューの操作部品を知っています。"), "breit"),
        ("p", T(
            "**{{MeAssistent}}** lists every game setting (cvar) that the original menus of the game offer, with the control they use there: a slider with its range, yes/no, or a choice with its entries. Tick the settings you want and click the button below the search field - the editor adds a settings page with these controls, a title and a back button. The list has three columns - setting, control and text; drag a column border to widen it, a cut-off text shows in full when you point at it.",
            "**{{MeAssistent}}** listet jede Spieleinstellung (Cvar), die die Original-Menüs des Spiels anbieten, mit der Steuerung, die sie dort haben: Schieber mit Bereich, Ja/Nein oder Auswahl mit ihren Einträgen. Kreuze die gewünschten an und klicke den Knopf unter dem Suchfeld - der Editor legt eine Einstellungsseite mit diesen Steuerungen, einem Titel und einem Zurück-Knopf an. Die Liste hat drei Spalten - Einstellung, Steuerung und Text; eine Spaltengrenze lässt sich ziehen, ein abgeschnittener Text erscheint ganz, wenn der Mauszeiger darauf steht.",
            "**{{MeAssistent}}** 列出游戏原版菜单提供的每个游戏设置(cvar),以及它们在那里使用的控件:带范围的滑块、是/否,或带选项的选择。勾选需要的设置并单击搜索框下方的按钮——编辑器会添加一个包含这些控件、标题和返回按钮的设置页。列表有三列——设置、控件和文字;拖动列边界可以加宽,被截断的文字在鼠标指向时完整显示。",
            "**{{MeAssistent}}** は、ゲーム本来のメニューにあるすべての設定(cvar)を、そこで使われている操作部品(範囲付きスライダー、はい/いいえ、項目付きの選択)と一緒に一覧にします。必要な設定にチェックを入れ、検索欄の下のボタンをクリックすると、それらの操作部品、タイトル、戻るボタンの付いた設定ページが追加されます。一覧は設定・操作部品・テキストの 3 列で、列の境界をドラッグすると広げられます。切れたテキストはマウスを重ねると全体が表示されます。")),
        ("h2", "hilfen", T("Reference image, original and limits", "Referenzbild, Original und Grenzen", "参考图、原版与上限", "参考画像、オリジナル、上限")),
        ("img", "menue_referenzbild", T("A reference image under the canvas.", "Ein Referenzbild unter der Fläche.", "画布下方的参考图。", "キャンバスの下の参考画像。"), "breit"),
        ("p", T(
            "**{{MeRefBild}}** places a PNG, JPG or TGA semi-transparently on the canvas, under the menu or - when a full-screen background covers it - over it (**{{MeRefOben}}**), with adjustable **{{MeRefDeckkraft}}**. It is only a drawing aid and never goes into the menu or the pk3.",
            "**{{MeRefBild}}** legt ein PNG, JPG oder TGA halbtransparent auf die Fläche, unter das Menü oder - wenn ein Vollbild-Hintergrund es verdeckt - darüber (**{{MeRefOben}}**), mit einstellbarer **{{MeRefDeckkraft}}**. Es ist nur eine Zeichenhilfe und kommt nie ins Menü oder in die pk3.",
            "**{{MeRefBild}}** 将 PNG、JPG 或 TGA 半透明地放在画布上,位于菜单下方,或在全屏背景遮住时位于上方(**{{MeRefOben}}**),**{{MeRefDeckkraft}}** 可调。它只是绘图辅助,绝不会进入菜单或 pk3。",
            "**{{MeRefBild}}** は PNG、JPG、TGA を半透明でキャンバスに置きます。メニューの下か、全画面の背景で隠れる場合は上(**{{MeRefOben}}**)に置け、**{{MeRefDeckkraft}}** を調整できます。描画の補助にすぎず、メニューや pk3 には入りません。")),
        ("img", "menue_vergleich", T("An element of the game's main menu was moved - the Properties name the difference and offer the reset.",
                                    "Ein Element des Hauptmenüs wurde verschoben - die Eigenschaften nennen den Unterschied und bieten das Zurücksetzen an.",
                                    "游戏主菜单中的一个元素被移动了——属性中会指出差异并提供重置。",
                                    "ゲームのメインメニューの要素を動かしました。プロパティが違いを示し、元に戻す操作を提示します。"), "breit"),
        ("p", T(
            "For a menu from the game, the Properties show in **{{MeOriginal}}** whether the selected element differs from the game's original file and in which keywords; **{{MeOriginalZuruecksetzen}}** puts the original itemDef back (one Undo step). Below the pages, a line shows how full the menu is against the limits of the game version - elements per menu, menus and the longest script - orange from 80 percent, red at the limit, because beyond it the game silently drops the rest.",
            "Bei einem Menü aus dem Spiel zeigen die Eigenschaften unter **{{MeOriginal}}**, ob das gewählte Element von der Originaldatei des Spiels abweicht und in welchen Schlüsselwörtern; **{{MeOriginalZuruecksetzen}}** setzt das Original-itemDef wieder ein (ein Rückgängig-Schritt). Unter den Seiten zeigt eine Zeile, wie voll das Menü gegen die Grenzen der Spielfassung ist - Elemente je Menü, Menüs und das längste Skript -, orange ab 80 Prozent, rot an der Grenze, denn darüber lässt das Spiel den Rest still weg.",
            "对于来自游戏的菜单,属性中的 **{{MeOriginal}}** 会显示所选元素是否与游戏原始文件不同以及在哪些关键字上不同;**{{MeOriginalZuruecksetzen}}** 会放回原版 itemDef(一步撤销)。页面下方的一行显示菜单相对于游戏版本上限的使用程度——每个菜单的元素、菜单数和最长脚本——80% 起为橙色,达到上限为红色,因为超出后游戏会悄悄丢弃其余部分。",
            "ゲームのメニューでは、プロパティの **{{MeOriginal}}** に、選んだ要素がゲームの元ファイルと違うかどうか、どのキーワードが違うかが表示されます。**{{MeOriginalZuruecksetzen}}** で元の itemDef に戻せます(元に戻す 1 回分)。ページの下の行は、ゲームのバージョンの上限(メニューごとの要素、メニュー数、最長スクリプト)に対する使用量を示し、80% からオレンジ、上限で赤になります。上限を超えるとゲームは残りを黙って捨てるためです。")),
        ("p", T(
            "Next to the limits, **Readability** counts hints with guide values of the editor (not limits of the game): text lower than 8 points on the 640 x 480 screen, contrast below 3:1 on filled areas, and texts that fit their field in one language of the game but not in another (measured with the game's fonts and the texts of every language in the game data). Hover for the list with solutions; click to select the next element with a hint.",
            "Neben den Grenzen zählt **Lesbarkeit** Hinweise nach Richtwerten des Editors (keine Grenzen des Spiels): Text unter 8 Punkten Höhe im Bildschirm 640 x 480, Kontrast unter 3:1 auf gefüllten Flächen und Texte, die in einer Sprache des Spiels in ihr Feld passen, in einer anderen aber nicht (gemessen mit den Schriften des Spiels und den Texten aller Sprachen der Spieldaten). Mit der Maus darüber siehst du die Liste mit Lösungen; ein Klick wählt das nächste Element mit Hinweis.",
            "在上限旁边,**可读性** 按编辑器的参考值(不是游戏的上限)统计提示:在 640 x 480 屏幕上低于 8 点的文字、填充区域上低于 3:1 的对比度,以及在游戏的一种语言中放得下、在另一种语言中放不下的文字(使用游戏字体和游戏数据中所有语言的文本测量)。鼠标悬停可查看带解决方法的列表;单击可选中下一个有提示的元素。",
            "上限の横の **読みやすさ** は、エディターの目安(ゲームの上限ではありません)でヒントを数えます: 640 x 480 画面で 8 ポイント未満の文字、塗りつぶし領域で 3:1 未満のコントラスト、ゲームのある言語ではフィールドに収まるのに別の言語では収まらない文字(ゲームのフォントとゲームデータの全言語のテキストで測定)。マウスを重ねると解決方法付きの一覧が出て、クリックすると次のヒントのある要素を選びます。")),
        ("p", T(
            "Closing a tab with unsaved changes now asks first: save, close without saving, or cancel.",
            "Schließt du einen Reiter mit ungespeicherten Änderungen, fragt der Editor jetzt zuerst: speichern, ohne Speichern schließen oder abbrechen.",
            "关闭含有未保存更改的标签页时,编辑器现在会先询问:保存、不保存并关闭,或取消。",
            "保存していない変更のあるタブを閉じると、まず確認します: 保存、保存せずに閉じる、キャンセル。")),
    ],
}

SEITEN = [MENUE_EDITOR_M6]
