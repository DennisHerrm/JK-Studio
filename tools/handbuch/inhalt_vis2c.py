"""VIS2c: Abschnitt "Einen Bau anhalten und fortsetzen" der Seite karte_teilbau (Bauen anhalten / Bauen fortsetzen...,
das Fenster "Bauen fortsetzen", was von VIS, LIGHT und BSP bleibt, die zwei Wege).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rest4.py.
"""
from inhalt_basis import T

TITEL = ("Pausing and resuming a build", "Einen Bau anhalten und fortsetzen", "暂停并继续编译", "ビルドの一時停止と再開",
         "Pausar y reanudar una compilación", "Pozastavení sestavení a pokračování")

BILD_FENSTER = (
    "The window **{{KeBpTitel}}** after a build was paused in the VIS step: what is saved, whether the map has changed since, and the two ways to go on.",
    "Das Fenster **{{KeBpTitel}}**, nachdem ein Bau im VIS-Schritt angehalten wurde: was gespeichert ist, ob sich die Karte seitdem geändert hat, und die zwei Wege weiterzumachen.",
    "在 VIS 步骤暂停编译之后的 **{{KeBpTitel}}** 窗口:保存了什么、地图此后是否改动过,以及两种继续的方式。",
    "VIS のステップでビルドを一時停止したあとの **{{KeBpTitel}}** ウィンドウ:何が保存されているか、そのあとマップが変わったか、そして続ける 2 つの方法。",
    "La ventana **{{KeBpTitel}}** después de pausar una compilación en el paso VIS: qué está guardado, si el mapa ha cambiado desde entonces y las dos maneras de continuar.",
    "Okno **{{KeBpTitel}}** po pozastavení sestavení v kroku VIS: co je uloženo, zda se mapa od té doby změnila, a dva způsoby, jak pokračovat.")

EINLEITUNG = (
    "A long build does not have to run in one piece. **{{KeBpAnhalten}}** - the button in the window {{KeFsTitel}}, a right click in that window, or {{KeLBuild}} > {{KeBpAnhalten}} - stops the build and saves what is done in the folder `<map>.bhedpause` beside the built map: a copy of the map, the .bsp, .prt and .srf of the finished steps and the state of the running step. You can close JK Studio; {{KeLBuild}} > **{{KeBpFortsetzen}}** continues later. While a build runs, the editor stays usable: you keep working on the map, q3map2 builds the file as it was saved when the build started.",
    "Ein langer Bau muss nicht an einem Stück laufen. **{{KeBpAnhalten}}** - der Knopf im Fenster {{KeFsTitel}}, ein Rechtsklick in diesem Fenster oder {{KeLBuild}} > {{KeBpAnhalten}} - hält den Bau an und sichert, was fertig ist, im Ordner `<karte>.bhedpause` neben der gebauten Karte: eine Kopie der Karte, die .bsp, .prt und .srf der fertigen Schritte und den Stand des laufenden Schritts. JK Studio kann geschlossen werden; {{KeLBuild}} > **{{KeBpFortsetzen}}** macht später weiter. Während ein Bau läuft, bleibt der Editor bedienbar: du arbeitest an der Karte weiter, q3map2 baut die Datei, wie sie beim Start des Baus gespeichert wurde.",
    "很长的编译不必一口气跑完。**{{KeBpAnhalten}}**——{{KeFsTitel}} 窗口中的按钮、在该窗口中点右键,或 {{KeLBuild}} > {{KeBpAnhalten}}——会停止编译,并把已完成的部分保存到已编译地图旁边的 `<地图>.bhedpause` 文件夹:地图的副本、已完成步骤的 .bsp、.prt 和 .srf,以及正在运行的步骤的状态。可以关闭 JK Studio;之后用 {{KeLBuild}} > **{{KeBpFortsetzen}}** 继续。编译运行期间编辑器仍可使用:你可以继续编辑地图,q3map2 编译的是编译开始时保存的文件。",
    "長いビルドを一度に最後まで走らせる必要はありません。**{{KeBpAnhalten}}**({{KeFsTitel}} ウィンドウのボタン、そのウィンドウでの右クリック、または {{KeLBuild}} > {{KeBpAnhalten}})でビルドを止め、できた分をビルドしたマップの隣のフォルダー `<マップ>.bhedpause` に保存します。マップのコピー、完了したステップの .bsp・.prt・.srf、実行中のステップの状態が入ります。JK Studio は閉じてかまいません。あとで {{KeLBuild}} > **{{KeBpFortsetzen}}** で続けます。ビルドの実行中もエディターは使えます。マップの編集を続けても、q3map2 はビルド開始時に保存されたファイルをビルドします。",
    "Una compilación larga no tiene que ejecutarse de una vez. **{{KeBpAnhalten}}** (el botón de la ventana {{KeFsTitel}}, un clic derecho en esa ventana o {{KeLBuild}} > {{KeBpAnhalten}}) detiene la compilación y guarda lo que está hecho en la carpeta `<mapa>.bhedpause` junto al mapa compilado: una copia del mapa, los .bsp, .prt y .srf de los pasos terminados y el estado del paso en curso. Puedes cerrar JK Studio; {{KeLBuild}} > **{{KeBpFortsetzen}}** continúa más tarde. Mientras se compila, el editor sigue siendo utilizable: sigues trabajando en el mapa y q3map2 compila el archivo tal como se guardó al empezar la compilación.",
    "Dlouhé sestavení nemusí běžet vcelku. **{{KeBpAnhalten}}** - tlačítko v okně {{KeFsTitel}}, kliknutí pravým tlačítkem v tomto okně nebo {{KeLBuild}} > {{KeBpAnhalten}} - sestavení zastaví a uloží, co je hotové, do složky `<mapa>.bhedpause` vedle sestavované mapy: kopii mapy, .bsp, .prt a .srf hotových kroků a stav běžícího kroku. JK Studio můžete zavřít; {{KeLBuild}} > **{{KeBpFortsetzen}}** později pokračuje. Během sestavení zůstává editor použitelný: na mapě pracujete dál, q3map2 sestavuje soubor tak, jak byl uložen při spuštění sestavení.")

VIS = (
    "**VIS**: a full VIS of bhed_q3map2 (`-vis` without `-fast`) saves its finished portals and ends within seconds. The resumed VIS computes only the other portals; the window {{KeFsTitel}} says how many were taken over.",
    "**VIS**: ein volles VIS von bhed_q3map2 (`-vis` ohne `-fast`) sichert seine fertigen Portale und endet in Sekunden. Das fortgesetzte VIS rechnet nur die übrigen Portale; das Fenster {{KeFsTitel}} sagt, wie viele übernommen wurden.",
    "**VIS**:bhed_q3map2 的完整 VIS(不带 `-fast` 的 `-vis`)会保存已完成的门户并在几秒内结束。继续后的 VIS 只计算其余门户;{{KeFsTitel}} 窗口会显示接手了多少个。",
    "**VIS**:bhed_q3map2 の完全な VIS(`-fast` なしの `-vis`)は、完了したポータルを保存して数秒で終わります。再開した VIS は残りのポータルだけを計算し、{{KeFsTitel}} ウィンドウに引き継いだ数が表示されます。",
    "**VIS**: un VIS completo de bhed_q3map2 (`-vis` sin `-fast`) guarda sus portales terminados y acaba en segundos. El VIS reanudado solo calcula los demás portales; la ventana {{KeFsTitel}} dice cuántos se han recuperado.",
    "**VIS**: plné VIS programu bhed_q3map2 (`-vis` bez `-fast`) uloží hotové portály a skončí během několika sekund. VIS po pokračování počítá jen ostatní portály; okno {{KeFsTitel}} říká, kolik jich bylo převzato.")

LICHT = (
    "**LIGHT**: the step is stopped. With **{{KeTkLicht}}** switched on, every finished pass - the direct light and each bounce - stays in the light cache and is not computed again; only the pass that was running is lost. Without the switch the whole light step starts again. The same holds for {{KeBmAbbrechen}} in the light step.",
    "**LIGHT**: der Schritt wird beendet. Ist **{{KeTkLicht}}** eingeschaltet, bleibt jeder fertige Durchgang - das direkte Licht und jeder Bounce - im Licht-Cache und wird nicht noch einmal gerechnet; verloren geht nur der Durchgang, der gerade lief. Ohne den Schalter beginnt der ganze Licht-Schritt neu. Dasselbe gilt für {{KeBmAbbrechen}} im Licht-Schritt.",
    "**LIGHT**:该步骤会被终止。打开 **{{KeTkLicht}}** 时,每一遍已完成的计算——直接光照和每次反弹——都会留在光照缓存中,不再重新计算;只有正在运行的那一遍会丢失。不打开该开关时,整个光照步骤将重新开始。在光照步骤中使用 {{KeBmAbbrechen}} 时也是如此。",
    "**LIGHT**:このステップは中止されます。**{{KeTkLicht}}** がオンなら、終わったパス(直接光と各バウンス)はすべてライトキャッシュに残り、計算し直しません。失われるのは実行中だったパスだけです。スイッチがオフなら、ライトのステップ全体を最初からやり直します。ライトのステップでの {{KeBmAbbrechen}} も同じです。",
    "**LIGHT**: el paso se detiene. Con **{{KeTkLicht}}** activado, cada pasada terminada (la luz directa y cada rebote) queda en la caché de luz y no se vuelve a calcular; solo se pierde la pasada que estaba en curso. Sin el interruptor, todo el paso de luz empieza de nuevo. Lo mismo vale para {{KeBmAbbrechen}} en el paso de luz.",
    "**LIGHT**: krok se ukončí. Je-li zapnuto **{{KeTkLicht}}**, zůstane každý hotový průchod - přímé světlo a každý odraz - v mezipaměti světla a nepočítá se znovu; ztratí se jen průchod, který právě běžel. Bez přepínače začne celý krok světla znovu. Totéž platí pro {{KeBmAbbrechen}} v kroku světla.")

BSP = (
    "**BSP**: the step is stopped and starts again when the build is resumed - it is the short one.",
    "**BSP**: der Schritt wird beendet und beginnt beim Fortsetzen neu - er ist der kurze.",
    "**BSP**:该步骤会被终止,继续编译时重新开始——它是耗时最短的一步。",
    "**BSP**:このステップは中止され、ビルドの再開時に最初からやり直します。もともと短いステップです。",
    "**BSP**: el paso se detiene y empieza de nuevo al reanudar la compilación; es el paso corto.",
    "**BSP**: krok se ukončí a při pokračování začne znovu - je to ten krátký.")

BILD_FORTSCHRITT = (
    "The resumed build in the window **{{KeFsTitel}}**: the VIS continues from the saved portals, and {{KeBpAnhalten}} stands beside {{KeBmAbbrechen}}.",
    "Der fortgesetzte Bau im Fenster **{{KeFsTitel}}**: das VIS macht mit den gesicherten Portalen weiter, und {{KeBpAnhalten}} steht neben {{KeBmAbbrechen}}.",
    "**{{KeFsTitel}}** 窗口中继续进行的编译:VIS 从保存的门户接着计算,{{KeBpAnhalten}} 位于 {{KeBmAbbrechen}} 旁边。",
    "**{{KeFsTitel}}** ウィンドウに表示された再開後のビルド:VIS は保存したポータルから続行し、{{KeBpAnhalten}} は {{KeBmAbbrechen}} の隣にあります。",
    "La compilación reanudada en la ventana **{{KeFsTitel}}**: el VIS continúa desde los portales guardados, y {{KeBpAnhalten}} está junto a {{KeBmAbbrechen}}.",
    "Sestavení po pokračování v okně **{{KeFsTitel}}**: VIS pokračuje z uložených portálů a {{KeBpAnhalten}} stojí vedle {{KeBmAbbrechen}}.")

AKTUELL = (
    "**{{KeBpAktuell}}** builds the entry again for the map as it is now and reuses what is safe. The VIS takes the saved portals only if the BSP step writes the same portal file - detail brushes, entities and textures may have changed -, otherwise it says so and computes from the start. The light cache checks every lightmap. The result is that of a full build of the current map.",
    "**{{KeBpAktuell}}** baut den Eintrag für die Karte von jetzt neu und verwendet wieder, was sicher ist. Das VIS nimmt die gesicherten Portale nur, wenn der BSP-Schritt dieselbe Portaldatei schreibt - Detail-Brushes, Entities und Texturen dürfen sich geändert haben -, sonst sagt es das und rechnet von vorn. Der Licht-Cache prüft jede Lightmap. Das Ergebnis ist das eines vollen Baus der aktuellen Karte.",
    "**{{KeBpAktuell}}** 针对现在的地图重新编译该条目,并重用可以安全重用的部分。只有 BSP 步骤写出相同的门户文件时(细节画刷、实体和贴图可以有改动),VIS 才会使用保存的门户;否则它会说明并从头计算。光照缓存会检查每个光照贴图。结果与完整编译当前地图相同。",
    "**{{KeBpAktuell}}** は、今のマップに対してエントリーをビルドし直し、安全に使える分を再利用します。BSP のステップが同じポータルファイルを書き出した場合だけ(ディテールブラシ、エンティティ、テクスチャは変わっていてもかまいません)、VIS は保存したポータルを使います。そうでなければその旨を表示して最初から計算します。ライトキャッシュはライトマップを 1 つずつ確かめます。結果は現在のマップをすべてビルドした場合と同じです。",
    "**{{KeBpAktuell}}** compila de nuevo la entrada para el mapa tal como está ahora y reutiliza lo que es seguro. El VIS solo toma los portales guardados si el paso BSP escribe el mismo archivo de portales (los brushes de detalle, las entidades y las texturas pueden haber cambiado); si no, lo dice y calcula desde el principio. La caché de luz comprueba cada lightmap. El resultado es el de una compilación completa del mapa actual.",
    "**{{KeBpAktuell}}** sestaví položku znovu pro mapu, jaká je teď, a znovu použije, co je bezpečné. VIS převezme uložené portály jen tehdy, když krok BSP zapíše stejný soubor portálů - detailní brushe, entity a textury se změnit mohly -, jinak to oznámí a počítá od začátku. Mezipaměť světla kontroluje každou lightmapu. Výsledek je stejný jako při plném sestavení aktuální mapy.")

ALT = (
    "**{{KeBpAlt}}** finishes exactly the paused build on the saved copy of the map - the map you are editing is not touched. When it is done, its .bsp replaces the one beside the map. Its commands come from the entry of the Build menu, as for every build - nothing in the saved folder is started. If the entry now has other steps or a step starts a program other than the compiler of JK Studio, the button is greyed out and the window says why. If the map folder was moved or renamed, the saved state counts at the new place.",
    "**{{KeBpAlt}}** führt genau den angehaltenen Bau auf der gesicherten Kopie der Karte zu Ende - die Karte, an der du arbeitest, bleibt unberührt. Ist er fertig, ersetzt seine .bsp die neben der Karte. Die Befehle kommen aus dem Eintrag des Build-Menüs, wie bei jedem Bau - aus dem gesicherten Ordner wird nichts gestartet. Hat der Eintrag inzwischen andere Schritte oder startet ein Schritt ein anderes Programm als den Compiler von JK Studio, ist der Knopf ausgegraut und das Fenster sagt, warum. Wurde der Kartenordner verschoben oder umbenannt, gilt der Zwischenstand am neuen Ort.",
    "**{{KeBpAlt}}** 在保存的地图副本上原样完成被暂停的编译——你正在编辑的地图不会被改动。完成后,它的 .bsp 会替换地图旁边的那个。命令来自 Build 菜单的条目,与每次编译相同——不会启动保存文件夹中的任何内容。如果该条目现在的步骤不同,或某一步启动的不是 JK Studio 的编译器,按钮会变灰,窗口会说明原因。如果地图文件夹被移动或重命名,保存的状态在新位置有效。",
    "**{{KeBpAlt}}** は、一時停止したビルドを保存したマップのコピーでそのまま最後まで行います。編集中のマップには触れません。完了すると、その .bsp がマップの隣の .bsp に置き換わります。コマンドはほかのビルドと同じく Build メニューのエントリーから作られ、保存フォルダーの中のものは何も起動しません。エントリーのステップが変わっていたり、JK Studio のコンパイラ以外のプログラムを起動するステップがあったりすると、ボタンは灰色になり、ウィンドウに理由が表示されます。マップのフォルダーを移動または名前変更した場合、保存した状態は新しい場所で使われます。",
    "**{{KeBpAlt}}** termina exactamente la compilación pausada sobre la copia guardada del mapa; el mapa que estás editando no se toca. Cuando acaba, su .bsp sustituye al que está junto al mapa. Sus comandos salen de la entrada del menú Build, como en cualquier compilación; no se inicia nada de la carpeta guardada. Si la entrada tiene ahora otros pasos o un paso inicia un programa que no es el compilador de JK Studio, el botón aparece en gris y la ventana explica por qué. Si la carpeta del mapa se movió o se renombró, el estado guardado vale en el lugar nuevo.",
    "**{{KeBpAlt}}** dokončí přesně pozastavené sestavení na uložené kopii mapy - mapa, kterou upravujete, zůstane nedotčená. Po dokončení jeho .bsp nahradí ten vedle mapy. Příkazy pocházejí z položky nabídky Build jako u každého sestavení - z uložené složky se nic nespouští. Má-li položka teď jiné kroky nebo spouští-li některý krok jiný program než kompilátor JK Studio, tlačítko je šedé a okno řekne proč. Byla-li složka mapy přesunuta nebo přejmenována, uložený stav platí na novém místě.")

VERWERFEN = (
    "**{{KeBpVerwerfen}}** deletes the folder. A normal build leaves a saved state as it is; pausing again replaces it.",
    "**{{KeBpVerwerfen}}** löscht den Ordner. Ein normaler Bau lässt einen Zwischenstand, wie er ist; erneutes Anhalten ersetzt ihn.",
    "**{{KeBpVerwerfen}}** 会删除该文件夹。普通的编译不会改动已保存的状态;再次暂停会替换它。",
    "**{{KeBpVerwerfen}}** はそのフォルダーを削除します。通常のビルドは保存した状態をそのまま残し、もう一度一時停止すると置き換わります。",
    "**{{KeBpVerwerfen}}** borra la carpeta. Una compilación normal deja el estado guardado como está; al pausar de nuevo se sustituye.",
    "**{{KeBpVerwerfen}}** složku smaže. Běžné sestavení ponechá uložený stav, jak je; nové pozastavení ho nahradí.")

NOTIZ = (
    "With **{{KeVisExakt}}** a resumed VIS is byte for byte the VIS without a pause - the tests `q3map2_vispause` and `q3map2_baupause` compare every lump. Without the switch it is a valid run of the usual rule: two builds with that rule never give the same bytes anyway, they differ by a few cluster pairs. The VIS state of a very large map takes gigabytes (2.9 GB were measured); the window shows its size, and the console warns before the VIS if the drive is too full. A cancelled VIS leaves its last state beside the .bsp - the next VIS with the same portals continues from it.",
    "Mit **{{KeVisExakt}}** ist ein fortgesetztes VIS Byte für Byte das VIS ohne Pause - die Tests `q3map2_vispause` und `q3map2_baupause` vergleichen jeden Lump. Ohne den Schalter ist es ein gültiger Lauf der üblichen Regel: zwei Bauten mit ihr ergeben ohnehin nie dieselben Bytes, sie unterscheiden sich um einige Cluster-Paare. Der VIS-Zwischenstand einer sehr großen Karte ist Gigabytes groß (gemessen wurden 2,9 GB); das Fenster zeigt seine Größe, und die Konsole warnt vor dem VIS, wenn das Laufwerk zu voll ist. Ein abgebrochenes VIS lässt seinen letzten Stand neben der .bsp - das nächste VIS mit denselben Portalen macht damit weiter.",
    "打开 **{{KeVisExakt}}** 时,继续后的 VIS 与不暂停的 VIS 逐字节相同——测试 `q3map2_vispause` 和 `q3map2_baupause` 会比较每一个 lump。不打开该开关时,它是通常规则下的一次有效运行:按该规则进行的两次编译本来就不会得到相同的字节,它们相差几对簇。很大的地图的 VIS 状态会占用数 GB(实测为 2.9 GB);窗口会显示其大小,驱动器空间不足时控制台会在 VIS 之前给出警告。被取消的 VIS 会把最后的状态留在 .bsp 旁边——下一次门户相同的 VIS 会从这里继续。",
    "**{{KeVisExakt}}** がオンなら、再開した VIS は一時停止なしの VIS とバイト単位で同じです。テスト `q3map2_vispause` と `q3map2_baupause` がすべての lump を比較します。スイッチがオフなら、通常の規則での有効な実行です。その規則での 2 回のビルドはもともと同じバイトにならず、クラスターの組がいくつか違います。非常に大きなマップの VIS の状態は数 GB になります(実測で 2.9 GB)。ウィンドウにはその大きさが表示され、ドライブの空きが足りないときは VIS の前にコンソールが警告します。中止した VIS は最後の状態を .bsp の隣に残し、同じポータルの次の VIS がそこから続けます。",
    "Con **{{KeVisExakt}}**, un VIS reanudado es byte a byte el VIS sin pausa: las pruebas `q3map2_vispause` y `q3map2_baupause` comparan cada lump. Sin el interruptor es una ejecución válida de la regla habitual: dos compilaciones con ella nunca dan los mismos bytes de todos modos, difieren en unos pocos pares de clústeres. El estado de VIS de un mapa muy grande ocupa gigabytes (se midieron 2,9 GB); la ventana muestra su tamaño y la consola avisa antes del VIS si la unidad está demasiado llena. Un VIS cancelado deja su último estado junto al .bsp; el siguiente VIS con los mismos portales continúa desde él.",
    "S **{{KeVisExakt}}** je VIS po pokračování bajt po bajtu stejné jako VIS bez pauzy - testy `q3map2_vispause` a `q3map2_baupause` porovnávají každý lump. Bez přepínače jde o platný běh obvyklého pravidla: dvě sestavení s ním stejně nikdy nedají stejné bajty, liší se o několik párů clusterů. Stav VIS velmi velké mapy zabírá gigabajty (naměřeno 2,9 GB); okno ukazuje jeho velikost a konzole před VIS varuje, je-li jednotka příliš plná. Zrušené VIS ponechá svůj poslední stav vedle .bsp - další VIS se stejnými portály z něj pokračuje.")

ALLE = (TITEL, BILD_FENSTER, EINLEITUNG, VIS, LICHT, BSP, BILD_FORTSCHRITT, AKTUELL, ALT, VERWERFEN, NOTIZ)


def _t(x):
    return T(*x[:4])


def bloecke():
    """die Bloecke fuer die Seite karte_teilbau (inhalt_karte8.py), nach dem Abschnitt "Bau-Fortschritt" - als Funktion:
    T() schlaegt die weiteren Sprachen nach, und inhalt_es.py / inhalt_cs.py laden dieses Modul fuer ES / CS"""
    return [
        ("h2", "anhalten", _t(TITEL)),
        ("img", "h_vis2c_fortsetzen", _t(BILD_FENSTER)),
        ("p", _t(EINLEITUNG)),
        ("list", [_t(VIS), _t(LICHT), _t(BSP)]),
        ("img", "h_vis2c_fortschritt", _t(BILD_FORTSCHRITT)),
        ("list", [_t(AKTUELL), _t(ALT), _t(VERWERFEN)]),
        ("note", _t(NOTIZ)),
    ]


ES = {x[0]: x[4] for x in ALLE}
CS = {x[0]: x[5] for x in ALLE}
