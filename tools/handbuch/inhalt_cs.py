# -*- coding: utf-8 -*-
"""Das Handbuch auf Tschechisch (cs): {englischer Text: Text} fuer
tools/handbuch/inhalt_basis.py (tools/sprachen.py, handbuch_texte).

Reihenfolge wie die Texte beim Bau entstehen (Seite fuer Seite). Aendert
sich ein englischer Text, fehlt hier die Uebersetzung: der Bau zeigt dann
den englischen und nennt ihn (gen_handbuch.py, `--pflicht=cs` als Fehler).
{{Kennung}} wird in der Sprache des Programms beschriftet - auch die
NetRadiant-Menues, die englisch bleiben (tools/i18n_sprache_cs.py).
"""

# Tastennamen wie auf einer tschechischen Tastatur (die Tasten selbst sind
# meist englisch beschriftet; Pfeile und Leertaste heissen so).
TASTEN = {"Space": "Mezerník", "Up": "Nahoru", "Down": "Dolů", "Left": "Vlevo", "Right": "Vpravo"}

TEXTE = {
    "Nothing is selected yet. Click the brush in a 2D view first, then the texture.":
        "Zatím není nic vybráno. Nejdřív klikněte na brush ve 2D pohledu, pak na texturu.",
    "That made an entity. Press Ctrl+Z, then drag with the LEFT mouse button in the 2D view.":
        "Tím vznikla entita. Stiskněte Ctrl+Z a pak táhněte LEVÝM tlačítkem myši ve 2D pohledu.",
    "That drew a second brush. Press Ctrl+Z and start the drag just above the top edge of the first one.":
        "Tím se nakreslil druhý brush. Stiskněte Ctrl+Z a začněte táhnout těsně nad horní hranou prvního.",
    "That drew a brush. Press Ctrl+Z and click with the RIGHT mouse button.":
        "Tím se nakreslil brush. Stiskněte Ctrl+Z a klikněte PRAVÝM tlačítkem myši.",
    "That was another light. Right-click again and choose info > info_player_start.":
        "To bylo další světlo. Klikněte znovu pravým tlačítkem a zvolte info > info_player_start.",
    "No map is open yet. Load one first: {{MenuFile}} > {{MapLoad}} - or open a mission.":
        "Zatím není otevřena žádná mapa. Nejdřív nějakou načtěte: {{MenuFile}} > {{MapLoad}} - nebo otevřete misi.",
    "This script has no camera yet. Insert a camera command first ({{GroupEvents}} list: double-click camera) - or open a mission with a cutscene.":
        "Tento skript zatím nemá kameru. Nejdřív vložte příkaz kamery (seznam {{GroupEvents}}: dvojklik na camera) - nebo otevřete misi s animační sekvencí.",
    "Select the camera first: click a camera shot on the map or the camera row in the timeline. Its buttons appear on the left.":
        "Nejdřív vyberte kameru: klikněte na záběr kamery v mapě nebo na řádek kamery na časové ose. Její tlačítka se objeví vlevo.",
    "This map has no figure yet. Open a mission with figures ({{MenuFile}} > {{LoadMission}}) or place one: the tile {{EntKindNpc}} on the left edge of the view.":
        "Tato mapa zatím nemá postavu. Otevřete misi s postavami ({{MenuFile}} > {{LoadMission}}) nebo nějakou umístěte: dlaždice {{EntKindNpc}} u levého okraje pohledu.",
    "This figure does not walk yet - its path has no walk target. Pick a figure that walks, or let one walk first ([[tut-p3]]).":
        "Tato postava zatím nechodí - její dráha nemá cíl chůze. Vyberte postavu, která chodí, nebo nejdřív nějakou nechte jít ([[tut-p3]]).",
    "A map is open, so the tab shows the map instead of the start page. Close it with {{MenuFile}} > {{MapClose}}.":
        "Je otevřena mapa, takže karta ukazuje mapu místo úvodní stránky. Zavřete ji pomocí {{MenuFile}} > {{MapClose}}.",
    "Nothing is selected. Click the door brush in a 2D view first, then right-click and choose func > func_door.":
        "Nic není vybráno. Nejdřív klikněte na brush dveří ve 2D pohledu, pak klikněte pravým tlačítkem a zvolte func > func_door.",
    "That was a light. Press Ctrl+Z, select the door brush and choose func > func_door.":
        "To bylo světlo. Stiskněte Ctrl+Z, vyberte brush dveří a zvolte func > func_door.",
    "You are in {{MeBedienen}} mode - clicks press buttons there. Switch to {{MeBearbeiten}} to move elements.":
        "Jste v režimu {{MeBedienen}} - kliknutí tam mačkají tlačítka. Pro posouvání prvků přepněte na {{MeBearbeiten}}.",
    "This tutorial also runs inside the program: **{{MenuHelp}} → {{TourMenu}}**.":
        "Tento tutoriál běží i přímo v programu: **{{MenuHelp}} → {{TourMenu}}**.",
    "Write a script from scratch":
        "Napsat skript od začátku",
    "A new empty script, an affect block for one figure and a first command inside it.":
        "Nový prázdný skript, blok affect pro jednu postavu a v něm první příkaz.",
    "Scripts · five steps":
        "Skripty · pět kroků",
    "**Start an empty script.** Click the **+** at the end of the tab bar (or **{{MenuFile}} → {{FileNew}}**). A tab `unnamed.txt` opens; switch the mode bar to **{{ViewEvents}}**.":
        "**Začněte prázdný skript.** Klikněte na **+** na konci řádku karet (nebo **{{MenuFile}} → {{FileNew}}**). Otevře se karta `unnamed.txt`; přepněte lištu režimů na **{{ViewEvents}}**.",
    "A new tab and the Events list.":
        "Nová karta a seznam Události.",
    "**Insert an affect block.** In the **{{GroupEvents}}** list on the left, double-click `affect`. It appears in the tree, already open, because a block holds commands.":
        "**Vložte blok affect.** V seznamu **{{GroupEvents}}** vlevo dvakrát klikněte na `affect`. Objeví se ve stromu, rovnou otevřený, protože blok obsahuje příkazy.",
    "**Name the figure.** Double-click the new line (or press <kbd>Enter</kbd>) to open the **{{EditorTitle}}**. Type the name of the figure into the first field - with a map loaded, the arrow next to it lists every name of the map. Leave `FLUSH` and press **{{EditorOk}}**.":
        "**Pojmenujte postavu.** Dvakrát klikněte na nový řádek (nebo stiskněte <kbd>Enter</kbd>) a otevře se **{{EditorTitle}}**. Do prvního pole napište jméno postavy - s načtenou mapou šipka vedle něj nabídne všechna jména z mapy. `FLUSH` nechte a stiskněte **{{EditorOk}}**.",
    "The event editor for affect.":
        "Editor událostí pro affect.",
    "**Put a command inside.** Click the `affect` line once, then double-click `wait` in the list: because the selected block is open, the command goes to the start of the block. Commands inserted after that follow it, in order.":
        "**Vložte dovnitř příkaz.** Klikněte jednou na řádek `affect` a pak v seznamu dvakrát klikněte na `wait`: protože je vybraný blok otevřený, příkaz jde na začátek bloku. Příkazy vložené poté ho následují v pořadí.",
    "**Save.** **{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) asks for a name the first time. Script names are lower case without spaces, for example `my_first.txt`.":
        "**Uložte.** **{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) se napoprvé zeptá na název. Názvy skriptů jsou malými písmeny bez mezer, například `my_first.txt`.",
    "The finished little script.":
        "Hotový malý skript.",
    "With a closed block selected, a new command goes after the block, not into it.":
        "Je-li vybrán zavřený blok, nový příkaz jde za blok, ne do něj.",
    "Only `affect`, `task`, `if`, `else` and `loop` can hold commands.":
        "Příkazy mohou obsahovat jen `affect`, `task`, `if`, `else` a `loop`.",
    "Steps":
        "Kroky",
    "Good to know":
        "Dobré vědět",
    "Three ways to add a command":
        "Tři způsoby, jak přidat příkaz",
    "Double-click, drag and drop, or the Insert key - and how to put a command exactly where you want it.":
        "Dvojklik, přetažení, nebo klávesa Insert - a jak dát příkaz přesně tam, kam chcete.",
    "Scripts · four steps":
        "Skripty · čtyři kroky",
    "**Double-click.** Select the line after which the command should come, then double-click the command in the **{{GroupEvents}}** list. That is how the original BehavEd worked.":
        "**Dvojklik.** Vyberte řádek, za který má příkaz přijít, a pak dvakrát klikněte na příkaz v seznamu **{{GroupEvents}}**. Tak fungoval původní BehavEd.",
    "**Insert key.** Click a command in the list once (it stays highlighted), select a line in the tree and press <kbd>Insert</kbd>. Handy when you add the same command several times.":
        "**Klávesa Insert.** Klikněte jednou na příkaz v seznamu (zůstane zvýrazněný), vyberte řádek ve stromu a stiskněte <kbd>Insert</kbd>. Hodí se, když přidáváte stejný příkaz několikrát.",
    "**Drag.** Drag the command from the list into the tree. While you drag, a line shows where it lands: above a line, below it, or - on the middle of a block - inside the block.":
        "**Přetažení.** Přetáhněte příkaz ze seznamu do stromu. Během tažení čára ukazuje, kam dopadne: nad řádek, pod něj, nebo - doprostřed bloku - do bloku.",
    "The Events list on the left, the script tree in the middle.":
        "Seznam Události vlevo, strom skriptu uprostřed.",
    "**Right-click.** Right-click a line: the submenus **{{GroupEvents}}** and **Macros** insert after that line - or into the block, if you right-clicked an open block.":
        "**Pravé tlačítko.** Klikněte pravým tlačítkem na řádek: podnabídky **{{GroupEvents}}** a **Makra** vkládají za tento řádek - nebo do bloku, pokud jste klikli na otevřený blok.",
    "Hover over a command in the list to read what it does.":
        "Najeďte myší na příkaz v seznamu a přečtěte si, co dělá.",
    "Macros at the end of the list add several commands at once, for example `walkOnly`.":
        "Makra na konci seznamu přidají několik příkazů najednou, například `walkOnly`.",
    "Change the values of a command":
        "Změna hodnot příkazu",
    "The event editor: lists, numbers, file fields, names from the map - and expressions such as get, tag and random.":
        "Editor událostí: seznamy, čísla, souborová pole, jména z mapy - a výrazy jako get, tag a random.",
    "**Open the editor.** Double-click a line or press <kbd>Enter</kbd>. The help text of the command is on top, each value has its own field.":
        "**Otevřete editor.** Dvakrát klikněte na řádek nebo stiskněte <kbd>Enter</kbd>. Nahoře je text nápovědy příkazu, každá hodnota má vlastní pole.",
    "The event editor for a camera command.":
        "Editor událostí pro příkaz kamery.",
    "**Pick from a list.** Fields with fixed choices are lists you can also type into: start typing to filter. A value that is not in the list is allowed (mods bring their own), but it is marked **unknown** - check for a typo.":
        "**Výběr ze seznamu.** Pole s pevnými volbami jsou seznamy, do kterých lze i psát: začněte psát a seznam se zúží. Hodnota, která v seznamu není, je povolena (mody přinášejí vlastní), ale je označena jako **neznámá** - zkontrolujte překlep.",
    "**Files and names.** File fields have **{{EditorBrowse}}** to browse and **{{EditorPlay}}** to play a sound or open a script. With a map loaded, an arrow next to name fields picks a name from the map.":
        "**Soubory a jména.** Souborová pole mají **{{EditorBrowse}}** pro procházení a **{{EditorPlay}}** pro přehrání zvuku nebo otevření skriptu. S načtenou mapou vybírá šipka vedle polí se jmény jméno z mapy.",
    "**An expression instead of a value.** **{{EditorExpr}}** turns a field into free text. **{{EditorHelper}}** builds one for you: **{{EditorGet}}** reads a variable (`get`), **{{EditorTag}}** the place or angles of a `ref_tag` (`tag`), **{{EditorRnd}}** a random number (`random`). **{{EditorRevert}}** goes back to a plain value.":
        "**Výraz místo hodnoty.** **{{EditorExpr}}** změní pole na volný text. **{{EditorHelper}}** ho sestaví za vás: **{{EditorGet}}** čte proměnnou (`get`), **{{EditorTag}}** místo nebo úhly `ref_tag` (`tag`), **{{EditorRnd}}** náhodné číslo (`random`). **{{EditorRevert}}** se vrátí k prosté hodnotě.",
    "**Done.** **{{EditorOk}}** (<kbd>Enter</kbd>) writes the values, **{{EditorCancel}}** (<kbd>Esc</kbd>) leaves everything as it was. Under each field you see the value it had before (**{{EditorWas}}**).":
        "**Hotovo.** **{{EditorOk}}** (<kbd>Enter</kbd>) zapíše hodnoty, **{{EditorCancel}}** (<kbd>Esc</kbd>) nechá vše, jak bylo. Pod každým polem vidíte hodnotu, kterou mělo předtím (**{{EditorWas}}**).",
    "Move, copy and comment out lines":
        "Přesun, kopírování a zakomentování řádků",
    "Rearrange a script: select several lines, drag them, duplicate them, and switch lines off without deleting them.":
        "Přeuspořádejte skript: vyberte několik řádků, přetáhněte je, duplikujte je a vypněte řádky, aniž byste je mazali.",
    "**Select.** Click a line; <kbd>Ctrl</kbd>+click adds lines, <kbd>Shift</kbd>+click selects a range. Every action works on everything selected.":
        "**Výběr.** Klikněte na řádek; <kbd>Ctrl</kbd>+kliknutí přidá řádky, <kbd>Shift</kbd>+kliknutí vybere rozsah. Každá akce působí na vše vybrané.",
    "**Move.** Drag the selection to a new place, or press <kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd> to move it one line - it stays inside its block. A block moves with everything inside it.":
        "**Přesun.** Přetáhněte výběr na nové místo, nebo stiskněte <kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd> a posunete ho o řádek - zůstane ve svém bloku. Blok se přesouvá se vším, co obsahuje.",
    "**Duplicate.** **{{ActClone}}** (<kbd>Space</kbd>) inserts a copy right below - the quickest way to a second camera shot or a second walk.":
        "**Duplikovat.** **{{ActClone}}** (<kbd>Space</kbd>) vloží kopii hned pod - nejrychlejší cesta k druhému záběru kamery nebo druhé chůzi.",
    "**Copy between scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>), switch tabs, select a line, **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>). Pasting puts the commands below the selected line.":
        "**Kopírování mezi skripty.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>), přepněte kartu, vyberte řádek, **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>). Vložení umístí příkazy pod vybraný řádek.",
    "**Switch off without deleting.** **{{ActRem}}** (<kbd>Backspace</kbd>) turns the lines into comments; the game skips them. Press it again (or <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) to switch them back on.":
        "**Vypnout bez smazání.** **{{ActRem}}** (<kbd>Backspace</kbd>) změní řádky na komentáře; hra je přeskočí. Dalším stiskem (nebo <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) je znovu zapnete.",
    "**{{ActDelete}}** (<kbd>Delete</kbd>) removes lines for good - but <kbd>Ctrl</kbd>+<kbd>Z</kbd> brings them back.":
        "**{{ActDelete}}** (<kbd>Delete</kbd>) odebere řádky natrvalo - ale <kbd>Ctrl</kbd>+<kbd>Z</kbd> je vrátí.",
    "Undo anything - even one step from the middle":
        "Vrátit cokoli - i jeden krok ze středu",
    "Undo and redo, the full history, and undoing one single old step while keeping everything after it.":
        "Zpět a znovu, celá historie a vrácení jediného starého kroku se zachováním všeho po něm.",
    "**Undo and redo.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> undoes the last change, <kbd>Ctrl</kbd>+<kbd>Y</kbd> redoes it. The **Undo** entry in the **{{MenuEdit}}** menu names the step, for example *Undo: delete*.":
        "**Zpět a znovu.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> vrátí poslední změnu, <kbd>Ctrl</kbd>+<kbd>Y</kbd> ji provede znovu. Položka **Zpět** v nabídce **{{MenuEdit}}** krok pojmenuje, například *Zpět: odstranění*.",
    "**See the history.** Right-click the **Undo** button in the action column (or **{{MenuEdit}} → {{UndoListMenu}}**). The list shows every step, newest at the top.":
        "**Zobrazte historii.** Klikněte pravým tlačítkem na tlačítko **Zpět** ve sloupci akcí (nebo **{{MenuEdit}} → {{UndoListMenu}}**). Seznam ukazuje každý krok, nejnovější nahoře.",
    "The undo history.":
        "Historie kroků zpět.",
    "**Go back several steps.** Click a step and press **{{UndoUpToHere}}**: that step and all newer ones are undone.":
        "**Vraťte se o několik kroků.** Klikněte na krok a stiskněte **{{UndoUpToHere}}**: tento krok a všechny novější se vrátí.",
    "**Undo one old step only.** Click a step and press **{{UndoOnlyThis}}**: only that change is undone, everything you did after it stays.":
        "**Vraťte jen jeden starý krok.** Klikněte na krok a stiskněte **{{UndoOnlyThis}}**: vrátí se jen tato změna, vše, co jste udělali po ní, zůstane.",
    "Entities on the map have their own undo: <kbd>Ctrl</kbd>+<kbd>Z</kbd> acts on them while the mouse is over the map or the entity list.":
        "Entity v mapě mají vlastní Zpět: <kbd>Ctrl</kbd>+<kbd>Z</kbd> působí na ně, dokud je myš nad mapou nebo seznamem entit.",
    "To take back a command to the state it had when the file was opened, right-click it → **{{ActRevertOriginal}}**.":
        "Chcete-li vrátit příkaz do stavu, který měl při otevření souboru, klikněte na něj pravým tlačítkem → **{{ActRevertOriginal}}**.",
    "Find and replace in all open scripts":
        "Najít a nahradit ve všech otevřených skriptech",
    "Rename a figure in every script of a mission at once - with the Find window.":
        "Přejmenujte postavu ve všech skriptech mise najednou - oknem Najít.",
    "**Open all scripts.** For a mission, **{{LoadMission}}** opens every script in its own tab.":
        "**Otevřete všechny skripty.** U mise otevře **{{LoadMission}}** každý skript na vlastní kartě.",
    "**Open Find.** **{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>). The window stays open while you work.":
        "**Otevřete Najít.** **{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>). Okno zůstane otevřené, zatímco pracujete.",
    "The Find window.":
        "Okno Najít.",
    "Finds commands by name, for example `camera`.":
        "Najde příkazy podle názvu, například `camera`.",
    "Finds a value in any field.":
        "Najde hodnotu v libovolném poli.",
    "The new value for replacing.":
        "Nová hodnota pro nahrazení.",
    "Only whole values / only in the selected lines.":
        "Jen celé hodnoty / jen ve vybraných řádcích.",
    "Step through the hits (<kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>).":
        "Procházení nálezů (<kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>).",
    "Replaces this hit / every hit in this script.":
        "Nahradí tento nález / každý nález v tomto skriptu.",
    "Replaces in every open script - one undo step each.":
        "Nahradí v každém otevřeném skriptu - po jednom kroku zpět.",
    "**Search a value.** Type the old name into **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) and **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) step through the hits. Tick **{{FindWhole}}** so that `kyle` does not also hit `kyle2`.":
        "**Hledání hodnoty.** Napište staré jméno do **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) a **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) procházejí nálezy. Zaškrtněte **{{FindWhole}}**, aby `kyle` nenacházelo i `kyle2`.",
    "**Search a command.** **{{FindNamed}}** finds commands by name, for example every `camera`. Both fields together find only commands that match both.":
        "**Hledání příkazu.** **{{FindNamed}}** najde příkazy podle názvu, například každé `camera`. Obě pole dohromady najdou jen příkazy, které odpovídají oběma.",
    "**Replace.** Type the new name into **{{FindReplaceWith}}** and press **{{FindReplaceAllDocs}}**. Each script gets one undo step.":
        "**Nahrazení.** Napište nové jméno do **{{FindReplaceWith}}** a stiskněte **{{FindReplaceAllDocs}}**. Každý skript dostane jeden krok zpět.",
    "Right-click a line → **{{ActFindSimilar}}** fills the Find window for you: every command of this kind, or every command with one of its values.":
        "Pravé tlačítko na řádku → **{{ActFindSimilar}}** vyplní okno Najít za vás: každý příkaz tohoto druhu, nebo každý příkaz s některou z jeho hodnot.",
    "Bookmarks and the split view":
        "Záložky a rozdělené zobrazení",
    "Jump between places in a long script, and compare two scripts side by side.":
        "Skákejte mezi místy v dlouhém skriptu a porovnávejte dva skripty vedle sebe.",
    "**Set a bookmark.** Select a line and press <kbd>Ctrl</kbd>+<kbd>F2</kbd> (**{{MenuEdit}} → {{MenuBookmarks}}**). A mark appears next to the line.":
        "**Nastavte záložku.** Vyberte řádek a stiskněte <kbd>Ctrl</kbd>+<kbd>F2</kbd> (**{{MenuEdit}} → {{MenuBookmarks}}**). Vedle řádku se objeví značka.",
    "**Jump.** <kbd>F2</kbd> goes to the next bookmark, <kbd>Shift</kbd>+<kbd>F2</kbd> to the previous one. Bookmarks are kept per file.":
        "**Skok.** <kbd>F2</kbd> přejde na další záložku, <kbd>Shift</kbd>+<kbd>F2</kbd> na předchozí. Záložky se pamatují pro každý soubor.",
    "**Split the view.** Open a second script, then **{{MenuFile}} → {{SplitToggle}} → 2**. The second script shows beside the first.":
        "**Rozdělte zobrazení.** Otevřete druhý skript, pak **{{MenuFile}} → {{SplitToggle}} → 2**. Druhý skript se zobrazí vedle prvního.",
    "Two scripts side by side.":
        "Dva skripty vedle sebe.",
    "**Copy across.** The right side is read-only, but you can drag lines from it into your script - they are copied. To edit the right one, click its tab.":
        "**Kopírování napříč.** Pravá strana je jen pro čtení, ale můžete z ní přetahovat řádky do svého skriptu - zkopírují se. Chcete-li upravit ten pravý, klikněte na jeho kartu.",
    "The split view works in the **{{ViewEvents}}** mode; the map and model views need the room.":
        "Rozdělené zobrazení funguje v režimu **{{ViewEvents}}**; zobrazení mapy a modelu potřebují místo.",
    "Compile and fix errors":
        "Kompilace a oprava chyb",
    "Turn the script into the .ibi the game runs, read the messages and jump to the line that is wrong.":
        "Převeďte skript na .ibi, které hra spouští, přečtěte si zprávy a skočte na řádek, který je špatně.",
    "**Compile.** Press **{{Compile}}** at the bottom of the action column. It saves the `.txt` and writes the `.ibi` next to it. The button flashes green when it worked.":
        "**Kompilace.** Stiskněte **{{Compile}}** dole ve sloupci akcí. Uloží `.txt` a vedle něj zapíše `.ibi`. Když se to povede, tlačítko zabliká zeleně.",
    "The Compile button.":
        "Tlačítko Kompilovat.",
    "**When it fails.** The button flashes red, the status line says *Compile failed*, and the first wrong line is selected in the tree.":
        "**Když selže.** Tlačítko zabliká červeně, stavový řádek hlásí *Kompilace selhala* a první špatný řádek se ve stromu vybere.",
    "**Read the messages.** Click the red or yellow counter in the status line at the bottom: the **{{MessagesTitle}}** window lists every message. Each one names the line and what is wrong.":
        "**Přečtěte si zprávy.** Klikněte na červené nebo žluté počítadlo ve stavovém řádku dole: okno **{{MessagesTitle}}** vypíše každou zprávu. Každá uvádí řádek a co je špatně.",
    "**Fix and compile again.** Usually it is a value in the wrong form - text without quotes, a missing `}`, a comparison other than `= < > !`. Fix it in the editor and press **{{Compile}}** again.":
        "**Opravte a zkompilujte znovu.** Obvykle jde o hodnotu ve špatném tvaru - text bez uvozovek, chybějící `}`, jiné porovnání než `= < > !`. Opravte ji v editoru a stiskněte znovu **{{Compile}}**.",
    "For a mission, **{{MissionSave}}** compiles every script at once and refuses to pack while one does not compile.":
        "U mise zkompiluje **{{MissionSave}}** všechny skripty najednou a odmítne balit, dokud se některý nedá zkompilovat.",
    "Every save and compile keeps a copy: **{{MenuFile}} → {{FileBackupFolder}}** shows the last ten.":
        "Každé uložení a kompilace uchová kopii: **{{MenuFile}} → {{FileBackupFolder}}** ukáže posledních deset.",
    "Wait until a figure has arrived":
        "Počkat, až postava dorazí",
    "task, dowait and do: let a figure finish its walk before it turns or speaks - or let it do two things at once.":
        "task, dowait a do: nechte postavu dokončit chůzi, než se otočí nebo promluví - nebo ať dělá dvě věci zároveň.",
    "**Why.** An `affect` block runs beside the script - the script does not wait for it, so two figures in two blocks already walk at the same time. But inside a block, `set ( SET_NAVGOAL )` only *starts* the walk: the next command follows at once, and the figure would turn or talk while still walking.":
        "**Proč.** Blok `affect` běží vedle skriptu - skript na něj nečeká, takže dvě postavy ve dvou blocích už jdou současně. Ale uvnitř bloku `set ( SET_NAVGOAL )` chůzi jen *spustí*: další příkaz následuje hned a postava by se otáčela nebo mluvila, ještě když jde.",
    "**Put the walk into a task.** Inside the figure's `affect` block insert `task` and name it, for example `walk`. Move the `SET_NAVGOAL` (and `SET_WALKING`, if you use it) into the task block.":
        "**Vložte chůzi do úlohy.** Do bloku `affect` postavy vložte `task` a pojmenujte ho, například `walk`. Přesuňte `SET_NAVGOAL` (a `SET_WALKING`, pokud ho používáte) do bloku task.",
    "**dowait.** After the task insert `dowait ( \"walk\" )`. It starts the task and waits until it is done - until the figure has arrived. Everything after it happens when the figure stands.":
        "**dowait.** Za task vložte `dowait ( \"walk\" )`. Spustí úlohu a počká, až skončí - až postava dorazí. Vše po něm se stane, když postava stojí.",
    "Two affect blocks, each with a walk task and dowait.":
        "Dva bloky affect, každý s úlohou chůze a dowait.",
    "**Two things at once.** `do ( \"walk\" )` starts a task and goes on at once. Declare a second task - for example `look` with a `SET_LOOK_TARGET` - and start both with `do`. Later, `wait ( \"walk\" )` waits for the first one.":
        "**Dvě věci zároveň.** `do ( \"walk\" )` spustí úlohu a hned pokračuje. Deklarujte druhou úlohu - například `look` s `SET_LOOK_TARGET` - a obě spusťte pomocí `do`. Později `wait ( \"walk\" )` počká na první.",
    "**Check it in the timeline.** Play the scene: what follows a `dowait` starts where the walk block ends. Drag the walk target, and the rest moves with it.":
        "**Zkontrolujte to na časové ose.** Přehrajte scénu: co následuje po `dowait`, začíná tam, kde končí blok chůze. Přetáhněte cíl chůze a zbytek se posune s ním.",
    "Commands marked with `*` in their help text (such as `SET_NAVGOAL`) finish their task only when the action is done - that is what makes `dowait` wait.":
        "Příkazy označené v textu nápovědy `*` (jako `SET_NAVGOAL`) dokončí svou úlohu, až když je akce hotová - díky tomu `dowait` čeká.",
    "Let two scripts wait for each other":
        "Nechat dva skripty čekat na sebe",
    "signal and waitsignal: one script pauses until the other says go.":
        "signal a waitsignal: jeden skript se zastaví, dokud mu druhý nedá pokyn.",
    "**Where to wait.** In the script that should pause, insert `waitsignal` and type a name, for example `door_open`.":
        "**Kde čekat.** Do skriptu, který se má zastavit, vložte `waitsignal` a napište název, například `door_open`.",
    "**Where to give the signal.** In the other script, at the moment it should go on, insert `signal ( \"door_open\" )`.":
        "**Kde dát signál.** Do druhého skriptu, v okamžiku, kdy má pokračovat, vložte `signal ( \"door_open\" )`.",
    "**Check the names.** Both must be spelled the same. Only one script can wait for a signal of a given name.":
        "**Zkontrolujte názvy.** Oba musí být napsané stejně. Na signál daného názvu může čekat jen jeden skript.",
    "The timeline cannot know how long a `waitsignal` waits in the game - everything after it is placed as if the signal came at once.":
        "Časová osa nemůže vědět, jak dlouho `waitsignal` ve hře čeká - vše po něm se umístí, jako by signál přišel hned.",
    "Use macros":
        "Používání maker",
    "walkOnly, runOnly, standOnly and the patrols: several set commands in one line.":
        "walkOnly, runOnly, standOnly a hlídky: několik příkazů set na jednom řádku.",
    "Scripts · three steps":
        "Skripty · tři kroky",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list, each with the number of commands it holds. Hover over one to read what it does.":
        "**Najděte je.** Makra jsou na konci seznamu **{{GroupEvents}}**, každé s počtem příkazů, které obsahuje. Najeďte na některé myší a přečtěte si, co dělá.",
    "**Insert.** Double-click or drag a macro like a command. `walkOnly`, for example, makes a figure walk and ignore enemies and alerts - five `set` commands.":
        "**Vložení.** Makro dvakrát klikněte nebo přetáhněte jako příkaz. `walkOnly` například nechá postavu jít a ignorovat nepřátele a poplachy - pět příkazů `set`.",
    "A macro in the tree.":
        "Makro ve stromu.",
    "**Fold or unfold.** **{{TreeFoldMacros}}** in the action column shows a macro as one line. Switch it off to see and change the single commands.":
        "**Sbalit nebo rozbalit.** **{{TreeFoldMacros}}** ve sloupci akcí ukáže makro jako jeden řádek. Vypněte to a uvidíte a změníte jednotlivé příkazy.",
    "Fly a camera shot yourself":
        "Proletět záběr kamery sami",
    "Pilot mode: sit in the camera, fly with the mouse and WASD, and set keys along the timeline.":
        "Režim pilota: usaďte se v kameře, leťte myší a WASD a nastavujte klíče podél časové osy.",
    "Camera · five steps":
        "Kamera · pět kroků",
    "**Select a shot.** Click the camera on the map or a shot in the **{{CamShots}}** list on the left.":
        "**Vyberte záběr.** Klikněte na kameru v mapě nebo na záběr v seznamu **{{CamShots}}** vlevo.",
    "**Start Pilot.** Press **{{CamPilot}}** on the camera bar. You now look through this shot.":
        "**Spusťte pilota.** Stiskněte **{{CamPilot}}** na liště kamery. Teď se díváte tímto záběrem.",
    "**Fly.** Hold the right mouse button to look around, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move, <kbd>Q</kbd> / <kbd>E</kbd> go down and up, <kbd>Shift</kbd> is faster.":
        "**Leťte.** Podržením pravého tlačítka myši se rozhlížíte, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> pohyb, <kbd>Q</kbd> / <kbd>E</kbd> dolů a nahoru, <kbd>Shift</kbd> je rychlejší.",
    "Flying a shot in pilot mode.":
        "Let záběrem v režimu pilota.",
    "**Set keys along the way.** Move the playhead in the timeline, fly to where the camera should be at that moment and press <kbd>Enter</kbd> (**{{CamPilotKey}}**). Repeat for more keys.":
        "**Nastavujte klíče po cestě.** Posuňte ukazatel přehrávání na časové ose, přeleťte tam, kde má kamera v tu chvíli být, a stiskněte <kbd>Enter</kbd> (**{{CamPilotKey}}**). Opakujte pro další klíče.",
    "**Finish.** **{{CamPilotApply}}** writes the view into the script; <kbd>Esc</kbd> (**{{CamPilotCancel}}**) leaves it as it was.":
        "**Dokončení.** **{{CamPilotApply}}** zapíše pohled do skriptu; <kbd>Esc</kbd> (**{{CamPilotCancel}}**) ho nechá, jak byl.",
    "Put a shot where you are looking":
        "Umístit záběr tam, kam se díváte",
    "Look around until the picture is right, then move a shot - or a new one - exactly there.":
        "Rozhlížejte se, dokud obraz nesedí, pak tam přesně posuňte záběr - nebo nový.",
    "Camera · four steps":
        "Kamera · čtyři kroky",
    "**Fly to the view you want.** Right mouse button + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>.":
        "**Přeleťte k požadovanému pohledu.** Pravé tlačítko myši + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>.",
    "**A new shot.** **{{CamNewShot}}** adds `camera ( MOVE )` and `camera ( PAN )` at your view, after the selected shot.":
        "**Nový záběr.** **{{CamNewShot}}** přidá `camera ( MOVE )` a `camera ( PAN )` ve vašem pohledu, za vybraný záběr.",
    "The camera panel with its shots.":
        "Panel kamery s jejími záběry.",
    "**Or move an existing one.** Select a shot and press **{{CamSetToView}}**: position and direction are replaced, one undo step.":
        "**Nebo posuňte existující.** Vyberte záběr a stiskněte **{{CamSetToView}}**: poloha a směr se nahradí, jeden krok zpět.",
    "**Check.** **{{MapThroughCam}}** on the map toolbar shows what the selected shot sees, with its zoom and the black cinema bars.":
        "**Kontrola.** **{{MapThroughCam}}** na liště mapy ukáže, co vybraný záběr vidí, s jeho přiblížením a černými filmovými pruhy.",
    "Save views you come back to: <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd> stores the view, <kbd>1</kbd> … <kbd>9</kbd> jumps back. They are kept per map.":
        "Pohledy, ke kterým se vracíte, si uložte: <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd> pohled uloží, <kbd>1</kbd> … <kbd>9</kbd> skočí zpět. Pamatují se pro každou mapu.",
    "Aim the camera at a mark on the map":
        "Namířit kameru na značku v mapě",
    "Use a ref_tag as the camera position - move the mark, and the shot follows.":
        "Použijte ref_tag jako polohu kamery - posuňte značku a záběr ji následuje.",
    "**Place a mark.** Click the tile **{{EntKindTag}}** on the left edge of the view, then click where the camera should stand. Give it a **{{EntName}}** in the inspector, such as `cam_door`.":
        "**Umístěte značku.** Klikněte na dlaždici **{{EntKindTag}}** u levého okraje pohledu a pak klikněte tam, kde má kamera stát. V inspektoru jí dejte **{{EntName}}**, například `cam_door`.",
    "**Camera to the mark.** Right-click the mark in the entity list → **{{EntityCamToTag}}**. Two camera commands are inserted that use the mark's place and angles.":
        "**Kamera ke značce.** Pravé tlačítko na značce v seznamu entit → **{{EntityCamToTag}}**. Vloží se dva příkazy kamery, které používají místo a úhly značky.",
    "The context menu of a mark.":
        "Místní nabídka značky.",
    "**Move the mark.** Drag the mark in the view or change its **{{EntOrigin}}**: the shot moves with it.":
        "**Posuňte značku.** Přetáhněte značku v pohledu nebo změňte její **{{EntOrigin}}**: záběr se posune s ní.",
    "**Turn it.** The mark's facing is the camera's direction. If the mark aims at a target, move the target to change the view.":
        "**Otočte ji.** Natočení značky je směr kamery. Míří-li značka na cíl, pohled změníte posunutím cíle.",
    "Zoom, fade to black and shake":
        "Přiblížení, zatmívání do černa a otřesy",
    "The camera commands ZOOM, FADE and SHAKE - and where to see them in the timeline.":
        "Příkazy kamery ZOOM, FADE a SHAKE - a kde je vidět na časové ose.",
    "**Insert a camera command.** Double-click `camera` in the **{{GroupEvents}}** list, then open it with <kbd>Enter</kbd>. The first field chooses what it does.":
        "**Vložte příkaz kamery.** Dvakrát klikněte na `camera` v seznamu **{{GroupEvents}}** a otevřete ho klávesou <kbd>Enter</kbd>. První pole volí, co dělá.",
    "**Zoom.** Choose `ZOOM`: the first number is the field of view (80 is normal, smaller is closer), the second the time in milliseconds to get there.":
        "**Přiblížení.** Zvolte `ZOOM`: první číslo je zorné pole (80 je normální, menší je blíž), druhé čas v milisekundách, za který se ho dosáhne.",
    "**Fade.** Choose `FADE`: start colour and opacity, end colour and opacity (0 to 1), and the time. `0 0 0`, `0` to `0 0 0`, `1` fades to black.":
        "**Zatmívání.** Zvolte `FADE`: počáteční barva a krytí, koncová barva a krytí (0 až 1) a čas. `0 0 0`, `0` na `0 0 0`, `1` zatmí do černa.",
    "The camera rows of the timeline.":
        "Řádky kamery na časové ose.",
    "Retime a cutscene in the timeline":
        "Přečasovat animační sekvenci na časové ose",
    "Make a shot longer, start a line of dialogue later, move a whole part - by dragging blocks.":
        "Prodlužte záběr, posuňte začátek repliky dialogu, přesuňte celou část - tažením bloků.",
    "Timeline · five steps":
        "Časová osa · pět kroků",
    "**Play it.** Press ▶ under the map and watch. Switch **Sound** on to hear the dialogue you are matching the cuts against.":
        "**Přehrajte to.** Stiskněte ▶ pod mapou a sledujte. Zapněte **Zvuk** a uslyšíte dialog, podle kterého ladíte střihy.",
    "**Move a block.** Drag a block left or right. Everything after it moves along, so the rest stays in sync. Hold <kbd>Alt</kbd> to move only this block.":
        "**Posuňte blok.** Táhněte blok doleva nebo doprava. Vše po něm se posune s ním, takže zbytek zůstane synchronní. Podržením <kbd>Alt</kbd> posunete jen tento blok.",
    "Dragging a block in the timeline: everything after it moves along.":
        "Tažení bloku na časové ose: vše po něm se posune s ním.",
    "**Change a duration.** Drag the right edge of a block - a camera move, a wait - to make it shorter or longer.":
        "**Změňte délku.** Táhněte pravý okraj bloku - pohyb kamery, čekání - a zkrátíte nebo prodloužíte ho.",
    "**Check the script.** Every drag changes a value in the script (a wait, a time). Click a block to see its line.":
        "**Zkontrolujte skript.** Každé tažení změní hodnotu ve skriptu (čekání, čas). Kliknutím na blok uvidíte jeho řádek.",
    "A warning sign on a camera block means it overlaps the next one - in the game the next move would start from where this one began, and the camera jumps.":
        "Výstražná značka na bloku kamery znamená, že překrývá další - ve hře by další pohyb začal tam, kde tento začal, a kamera by skočila.",
    "Look through the camera while you work":
        "Dívat se kamerou během práce",
    "Through camera, the small preview picture, follow mode and the speed of playback.":
        "Pohled kamerou, malý náhled, režim sledování a rychlost přehrávání.",
    "**Through camera.** **{{MapThroughCam}}** on the map toolbar shows the picture of the selected shot, with zoom and cinema bars.":
        "**Pohled kamerou.** **{{MapThroughCam}}** na liště mapy ukáže obraz vybraného záběru, s přiblížením a filmovými pruhy.",
    "**The small preview.** **{{CamPreview}}** on the camera bar puts a small picture of the camera's view in the bottom right corner - you keep flying freely. Click the picture to look through it.":
        "**Malý náhled.** **{{CamPreview}}** na liště kamery umístí malý obrázek pohledu kamery do pravého dolního rohu - dál volně létáte. Kliknutím na obrázek se jím podíváte.",
    "Looking through the camera while playing.":
        "Pohled kamerou během přehrávání.",
    "**Follow.** **{{TlFollow}}** in the timeline moves your view along with the camera while it plays.":
        "**Sledování.** **{{TlFollow}}** na časové ose posouvá váš pohled spolu s kamerou, zatímco hraje.",
    "**Slow motion.** The speed list next to the play button plays at 0.25x to 2x. Sound plays only at 1x.":
        "**Zpomalený pohyb.** Seznam rychlostí vedle tlačítka přehrávání hraje od 0.25x do 2x. Zvuk hraje jen při 1x.",
    "Walk, run or walk backwards":
        "Chůze, běh nebo chůze pozpátku",
    "Change how a figure moves - right on its path in the map.":
        "Změňte, jak se postava pohybuje - přímo na její dráze v mapě.",
    "Figures · four steps":
        "Postavy · čtyři kroky",
    "**Show the path.** Click the figure on the map or its name in the timeline. Its path appears: grey ahead of the playhead, yellow where it already walked.":
        "**Zobrazte dráhu.** Klikněte na postavu v mapě nebo na její jméno na časové ose. Objeví se její dráha: šedá před ukazatelem přehrávání, žlutá tam, kde už prošla.",
    "**Right-click a walk target** on the path. The menu offers **{{BahnGehen}}**, **{{BahnRennen}}** and **{{BahnRueckwaerts}}**.":
        "**Klikněte pravým tlačítkem na cíl chůze** na dráze. Nabídka nabízí **{{BahnGehen}}**, **{{BahnRennen}}** a **{{BahnRueckwaerts}}**.",
    "The menu of a walk target.":
        "Nabídka cíle chůze.",
    "**Pick one.** behaved writes `SET_WALKING` / `SET_RUNNING` before this walk. It counts from this walk on - later walks keep it unless you change them too.":
        "**Vyberte jedno.** behaved zapíše před tuto chůzi `SET_WALKING` / `SET_RUNNING`. Platí od této chůze dál - pozdější chůze si to ponechají, pokud je nezměníte také.",
    "**Watch.** Play the timeline: the figure is faster, and the walk block in the timeline is shorter.":
        "**Sledujte.** Přehrajte časovou osu: postava je rychlejší a blok chůze na časové ose je kratší.",
    "Make a figure look at someone":
        "Nechat postavu dívat se na někoho",
    "Look at another figure while walking, turn towards a target, or face the walking direction again.":
        "Dívat se při chůzi na jinou postavu, otočit se k cíli, nebo se zase dívat ve směru chůze.",
    "**While walking.** Right-click a walk target on the path → **{{BahnAnsehen}}** → pick a figure. The figure walks and keeps its head on the other one.":
        "**Při chůzi.** Pravé tlačítko na cíli chůze na dráze → **{{BahnAnsehen}}** → vyberte postavu. Postava jde a drží hlavu otočenou k té druhé.",
    "The menu of a walk target: Look at while walking lists the figures of the scene.":
        "Nabídka cíle chůze: Dívat se na cíl při chůzi vypíše postavy scény.",
    "**Standing.** Right-click the target entity in the entity list → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) or **{{EntityAsLook}}** (`SET_LOOK_TARGET`). The command goes into the script after the selected line.":
        "**Ve stoje.** Pravé tlačítko na cílové entitě v seznamu entit → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) nebo **{{EntityAsLook}}** (`SET_LOOK_TARGET`). Příkaz se vloží do skriptu za vybraný řádek.",
    "**Back to normal.** Right-click the walk target → **{{BahnBlickFrei}}**.":
        "**Zpět k normálu.** Pravé tlačítko na cíli chůze → **{{BahnBlickFrei}}**.",
    "Turn a figure at a moment":
        "Otočit postavu v daném okamžiku",
    "Set which way a figure faces at a certain time - with the rotate ring on the map.":
        "Nastavte, kam se postava v určitém čase dívá - otočným kruhem v mapě.",
    "**Go to the moment.** Move the playhead in the timeline to where the figure should face another way.":
        "**Přejděte na okamžik.** Posuňte ukazatel přehrávání na časové ose tam, kde se má postava dívat jinam.",
    "**Select the figure** on the map and switch the gizmo bar to **{{GizmoRotate}}**.":
        "**Vyberte postavu** v mapě a přepněte lištu gizma na **{{GizmoRotate}}**.",
    "**Turn.** Drag the ring. When you let go, behaved writes the facing at this time into the script (`SET_DYAW` for an NPC, which then turns with its turn rate as in the game; `SET_ANGLES` for the player), and the status line says *Facing … degrees at … s*.":
        "**Otočte.** Táhněte kruh. Když pustíte, behaved zapíše natočení v tomto čase do skriptu (`SET_DYAW` pro NPC, které se pak otáčí svou rychlostí otáčení jako ve hře; `SET_ANGLES` pro hráče) a stavový řádek hlásí *Natočení … stupňů v … s*.",
    "The rotate ring.":
        "Otočný kruh.",
    "**During a walk.** Turning while the figure walks sets the facing for when it arrives.":
        "**Během chůze.** Otočení během chůze postavy nastaví natočení pro chvíli, kdy dorazí.",
    "If the time is not known - a `waitsignal` or `dowait` before - behaved says so instead of guessing.":
        "Není-li čas znám - předtím `waitsignal` nebo `dowait` - behaved to řekne místo hádání.",
    "Let a figure play an animation":
        "Nechat postavu přehrát animaci",
    "Find an animation in the model view, watch it, and put it into the script with one click.":
        "Najděte animaci v zobrazení modelu, prohlédněte si ji a jedním kliknutím ji vložte do skriptu.",
    "Figures · five steps":
        "Postavy · pět kroků",
    "**Load the figure's model.** Switch to **{{ViewModel}}** and press **{{NpcBrowser}}**: click a character - its model and skin load as in the game.":
        "**Načtěte model postavy.** Přepněte na **{{ViewModel}}** a stiskněte **{{NpcBrowser}}**: klikněte na postavu - její model a skin se načtou jako ve hře.",
    "**Find an animation.** Type into **{{ModelAnimSearch}}**, for example `TALK`. **BOTH**, **TORSO**, **LEGS** filter by body part.":
        "**Najděte animaci.** Pište do **{{ModelAnimSearch}}**, například `TALK`. **BOTH**, **TORSO**, **LEGS** filtrují podle části těla.",
    "The model view with its animation list.":
        "Zobrazení modelu se seznamem animací.",
    "**Watch it.** Click an animation: it plays. **{{ModelLoop}}**, the speed and the frame buttons help to look closely.":
        "**Prohlédněte si ji.** Klikněte na animaci: přehraje se. **{{ModelLoop}}**, rychlost a tlačítka snímků pomohou podívat se zblízka.",
    "**Into the script.** Select a line in your script first (inside the figure's `affect` block), then double-click the animation or press **{{ModelAnimToScript}}**: `set ( SET_ANIM_BOTH, \"…\" )` is inserted after that line.":
        "**Do skriptu.** Nejdřív vyberte řádek ve svém skriptu (uvnitř bloku `affect` postavy), pak dvakrát klikněte na animaci nebo stiskněte **{{ModelAnimToScript}}**: za tento řádek se vloží `set ( SET_ANIM_BOTH, \"…\" )`.",
    "**See it on the map.** Switch back to **{{ViewMap}}** and play the timeline: the figure plays the animation at that point.":
        "**Podívejte se na ni v mapě.** Přepněte zpět na **{{ViewMap}}** a přehrajte časovou osu: postava v tom místě přehraje animaci.",
    "Add a line of dialogue":
        "Přidat repliku dialogu",
    "A sound on a figure's voice channel, listened to and timed against the camera.":
        "Zvuk na hlasovém kanálu postavy, poslechnutý a načasovaný vůči kameře.",
    "**Insert sound.** Inside the figure's `affect` block, insert `sound`. In the editor choose the channel `CHAN_VOICE` and pick the file with **{{EditorBrowse}}**; **{{EditorPlay}}** plays it.":
        "**Vložte sound.** Do bloku `affect` postavy vložte `sound`. V editoru zvolte kanál `CHAN_VOICE` a soubor vyberte pomocí **{{EditorBrowse}}**; **{{EditorPlay}}** ho přehraje.",
    "**Hear it in the scene.** Switch **{{TlAudio}}** on in the timeline and play: the line sounds at its moment.":
        "**Poslechněte si ji ve scéně.** Zapněte na časové ose **{{TlAudio}}** a přehrajte: replika zazní ve svém okamžiku.",
    "**See it.** The **{{FlowTracks}}** view (top of the script column) has a **{{TracksDialog}}** row - every voice line as a block, so you see where the cuts fall.":
        "**Podívejte se.** Zobrazení **{{FlowTracks}}** (nahoře ve sloupci skriptu) má řádek **{{TracksDialog}}** - každá hlasová replika jako blok, takže vidíte, kam padají střihy.",
    "The Tracks view.":
        "Zobrazení Stopy.",
    "**Time it.** A `wait` after the sound keeps the next command until the line is over. Drag the wait's edge in the timeline to fit.":
        "**Načasujte ji.** `wait` po zvuku pozdrží další příkaz, dokud replika neskončí. Přetáhněte okraj čekání na časové ose, aby seděl.",
    "Place, move and turn entities":
        "Umisťování, posouvání a otáčení entit",
    "Put a walk target on the floor, move it on the grid, turn it, copy it.":
        "Položte cíl chůze na podlahu, posuňte ho po mřížce, otočte ho, zkopírujte ho.",
    "Map · five steps":
        "Mapa · pět kroků",
    "**Place.** Click a tile on the left edge of the view - for example **{{EntKindNav}}** - then click on the floor. Hold <kbd>Shift</kbd> to place several. <kbd>Esc</kbd> stops.":
        "**Umístění.** Klikněte na dlaždici u levého okraje pohledu - například **{{EntKindNav}}** - a pak klikněte na podlahu. Podržením <kbd>Shift</kbd> umístíte několik. <kbd>Esc</kbd> skončí.",
    "Placing walk targets.":
        "Umisťování cílů chůze.",
    "**Move.** Drag the entity over the floor, or drag a coloured arrow to move it along one axis. <kbd>Ctrl</kbd> snaps to 8 units.":
        "**Posun.** Táhněte entitu po podlaze, nebo táhněte barevnou šipku a posunete ji podél jedné osy. <kbd>Ctrl</kbd> přichytává k 8 jednotkám.",
    "**Turn.** Switch the gizmo bar to **{{GizmoRotate}}** and drag the ring; <kbd>Ctrl</kbd> turns in 15 degree steps.":
        "**Otočení.** Přepněte lištu gizma na **{{GizmoRotate}}** a táhněte kruh; <kbd>Ctrl</kbd> otáčí po 15 stupních.",
    "**Copy.** <kbd>Alt</kbd>+drag makes a copy and moves it. Or press **{{EntDuplicate}}** in the inspector.":
        "**Kopie.** <kbd>Alt</kbd>+tažení vytvoří kopii a posune ji. Nebo stiskněte v inspektoru **{{EntDuplicate}}**.",
    "**Onto the floor.** **{{EntDropFloor}}** in the inspector puts it exactly on the floor below - the inspector shows how high the feet are.":
        "**Na podlahu.** **{{EntDropFloor}}** v inspektoru ji položí přesně na podlahu pod ní - inspektor ukazuje, jak vysoko jsou nohy.",
    "**{{Grid}}** on the map toolbar snaps everything to 8 to 64 units while dragging and placing.":
        "**{{Grid}}** na liště mapy přichytává při tažení a umisťování vše k 8 až 64 jednotkám.",
    "Find an entity and fly to it":
        "Najít entitu a přeletět k ní",
    "Search the entity list, fly there, hide what is in the way.":
        "Hledejte v seznamu entit, přeleťte tam, skryjte, co překáží.",
    "Map · four steps":
        "Mapa · čtyři kroky",
    "**Search.** Type a name or class into the search box above the entity list, for example `navgoal` or `kyle`.":
        "**Hledání.** Do vyhledávacího pole nad seznamem entit napište název nebo třídu, například `navgoal` nebo `kyle`.",
    "**Fly there.** Double-click the row: the view flies to the entity and selects it.":
        "**Přeletět tam.** Dvakrát klikněte na řádek: pohled přeletí k entitě a vybere ji.",
    "**Hide groups.** Click the eye in front of a group to hide it in the view; <kbd>Alt</kbd>+click shows only that group. **{{EntityShowAll}}** brings everything back.":
        "**Skrytí skupin.** Kliknutím na oko před skupinou ji v pohledu skryjete; <kbd>Alt</kbd>+kliknutí zobrazí jen tuto skupinu. **{{EntityShowAll}}** vrátí vše.",
    "The entity list with its groups.":
        "Seznam entit se skupinami.",
    "**Names on the map.** **{{MapNames}}** on the map toolbar writes every name into the view.":
        "**Názvy v mapě.** **{{MapNames}}** na liště mapy vypíše do pohledu každý název.",
    "Edit an entity's keys":
        "Úprava klíčů entity",
    "Name, position, spawnflags and all other keys in the inspector - and the NPC type search.":
        "Název, poloha, spawnflags a všechny ostatní klíče v inspektoru - a hledání typu NPC.",
    "**Select the entity.** Click it in the view or in the list. The inspector under the list shows its class in colour; hover over it for a description from the game code.":
        "**Vyberte entitu.** Klikněte na ni v pohledu nebo v seznamu. Inspektor pod seznamem ukazuje barevně její třídu; najeďte na ni myší a uvidíte popis z kódu hry.",
    "**General.** **{{EntName}}** is how scripts address it, **{{EntOrigin}}** and **{{EntAngle}}** where it stands and faces.":
        "**Obecné.** **{{EntName}}** je to, jak ji skripty oslovují, **{{EntOrigin}}** a **{{EntAngle}}** kde stojí a kam se dívá.",
    "The inspector.":
        "Inspektor.",
    "Flies the view to the entity.":
        "Přeletí pohledem k entitě.",
    "Puts it exactly on the floor below.":
        "Položí ji přesně na podlahu pod ní.",
    "Copies or removes it - every selected entity.":
        "Zkopíruje ji nebo odebere - každou vybranou entitu.",
    "Undo / Redo":
        "Zpět / Znovu",
    "The history of entity changes, separate from the script's.":
        "Historie změn entit, oddělená od historie skriptu.",
    "Shows every key as plain text.":
        "Zobrazí každý klíč jako prostý text.",
    "Filter fields":
        "Filtrovat pole",
    "Shows only the keys whose name contains the text.":
        "Zobrazí jen klíče, jejichž název obsahuje text.",
    "Next to `NPC_type`: searches every character of the game.":
        "Vedle `NPC_type`: prohledá každou postavu hry.",
    "**Keys.** Under **{{EntSecFields}}**, change values or add one with **{{EntAddKey}}**. For a character, the **...** next to `NPC_type` searches every NPC of the game. Script keys have **{{EntOpenScript}}** to open the script.":
        "**Klíče.** Pod **{{EntSecFields}}** měňte hodnoty nebo přidejte novou pomocí **{{EntAddKey}}**. U postavy prohledá **...** vedle `NPC_type` každé NPC hry. Skriptové klíče mají **{{EntOpenScript}}** pro otevření skriptu.",
    "**Everything as text.** **{{EntRaw}}** shows all keys as plain text, like Hammer without SmartEdit.":
        "**Vše jako text.** **{{EntRaw}}** ukáže všechny klíče jako prostý text, jako Hammer bez SmartEdit.",
    "Entity changes are kept with the mission: **{{MissionSave}}** writes them into the `.ent` file.":
        "Změny entit se uchovávají s misí: **{{MissionSave}}** je zapíše do souboru `.ent`.",
    "Look at the map from above":
        "Podívat se na mapu shora",
    "Top view with a cut height - to see the floor plan of a level, one floor at a time.":
        "Pohled shora s výškou řezu - pro půdorys úrovně, patro po patře.",
    "Map · three steps":
        "Mapa · tři kroky",
    "**Switch on {{MapTopView}}** on the map toolbar. The view looks straight down; the wheel zooms, right or middle mouse and <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move.":
        "**Zapněte {{MapTopView}}** na liště mapy. Pohled se dívá přímo dolů; kolečko přibližuje, pravé nebo prostřední tlačítko a <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> posouvají.",
    "The top view.":
        "Pohled shora.",
    "**Cut away ceilings.** Drag the **cut height** box at the top right: everything higher is not drawn. Lower it to look at a lower floor.":
        "**Odřízněte stropy.** Táhněte políčko **výška řezu** vpravo nahoře: nic výš se nekreslí. Snižte ho a podíváte se na nižší patro.",
    "**Work as usual.** Selecting, dragging and placing work in the top view too - handy for paths and waypoints.":
        "**Pracujte jako obvykle.** Výběr, tažení a umisťování fungují i v pohledu shora - hodí se pro dráhy a navigační body.",
    "See better in dark maps":
        "Lépe vidět v tmavých mapách",
    "Brightness, ambient light and switching effects, fog or sky off while you work.":
        "Jas, okolní světlo a vypnutí efektů, mlhy nebo oblohy během práce.",
    "**Open {{MapViewSettings}}** (the gear on the map toolbar). **{{MapBrightness}}** lightens everything, **{{MapMinLight}}** lifts only the darkest areas - 0 is exactly like the game.":
        "**Otevřete {{MapViewSettings}}** (ozubené kolo na liště mapy). **{{MapBrightness}}** zesvětlí vše, **{{MapMinLight}}** zvedne jen nejtmavší místa - 0 je přesně jako ve hře.",
    "The view settings.":
        "Nastavení zobrazení.",
    "**Switch things off.** **{{MapFog}}**, **{{MapEffects}}**, **{{MapGlow}}** and **{{MapSky}}** on the toolbar switch those parts off. Without the sky you can look into the map from outside.":
        "**Vypněte věci.** **{{MapFog}}**, **{{MapEffects}}**, **{{MapGlow}}** a **{{MapSky}}** na liště tyto části vypnou. Bez oblohy se můžete dívat do mapy zvenku.",
    "**Only this script.** **{{MapScriptFocus}}** in the view settings dims every entity the active script does not use.":
        "**Jen tento skript.** **{{MapScriptFocus}}** v nastavení zobrazení ztlumí každou entitu, kterou aktivní skript nepoužívá.",
    "A door that opens":
        "Dveře, které se otevírají",
    "Animate a door or platform with move commands: two keys, and the script moves it.":
        "Animujte dveře nebo plošinu příkazy move: dva klíče a skript je posune.",
    "Animation · five steps":
        "Animace · pět kroků",
    "**Select the door.** Click it in the view - doors and platforms are `func_` entities. In the inspector press **{{EntAnimate}}**.":
        "**Vyberte dveře.** Klikněte na ně v pohledu - dveře a plošiny jsou entity `func_`. V inspektoru stiskněte **{{EntAnimate}}**.",
    "**Mode.** In the **{{AnimTitle}}** window keep **{{AnimModeMove}}**: the movement becomes `move` commands in the script.":
        "**Režim.** V okně **{{AnimTitle}}** ponechte **{{AnimModeMove}}**: pohyb se stane příkazy `move` ve skriptu.",
    "The Animation window.":
        "Okno Animace.",
    "**First key.** Put the playhead where the door should start moving and press **{{AnimAddKey}}** - this is the closed position.":
        "**První klíč.** Nastavte ukazatel přehrávání tam, kde se mají dveře začít pohybovat, a stiskněte **{{AnimAddKey}}** - to je zavřená poloha.",
    "**Second key.** Move the playhead later (for example 2 seconds) and drag the door up or aside in the view: while the window is open, dragging sets a key at the playhead.":
        "**Druhý klíč.** Posuňte ukazatel přehrávání později (například o 2 sekundy) a přetáhněte dveře v pohledu nahoru nebo stranou: dokud je okno otevřené, tažení nastaví klíč u ukazatele přehrávání.",
    "**Play.** The door now opens in the timeline. **{{AnimLinear}}** makes it move at constant speed instead of easing out.":
        "**Přehrání.** Dveře se teď na časové ose otevírají. **{{AnimLinear}}** je nechá pohybovat se stálou rychlostí místo dojíždění.",
    "Animated entities need a name; behaved gives the door one if it has none.":
        "Animované entity potřebují název; behaved dveřím nějaký dá, pokud žádný nemají.",
    "Change a ROFF file of the game":
        "Změnit soubor ROFF hry",
    "Turn a ROFF flight from the game into keys, change it, and save it as your own copy.":
        "Převeďte let ROFF ze hry na klíče, změňte ho a uložte jako vlastní kopii.",
    "Animation · four steps":
        "Animace · čtyři kroky",
    "**Select the entity** that plays the ROFF (a ship, a boulder) and press **{{EntAnimate}}**.":
        "**Vyberte entitu**, která ROFF přehrává (loď, balvan), a stiskněte **{{EntAnimate}}**.",
    "**Edit.** Under **{{AnimRoffPlays}}** press **{{AnimRoffEdit}}** next to the file. behaved turns the frames into a few keys - the game file stays untouched, you edit a copy with its own name.":
        "**Úprava.** Pod **{{AnimRoffPlays}}** stiskněte **{{AnimRoffEdit}}** vedle souboru. behaved převede snímky na několik klíčů - soubor hry zůstane nedotčen, upravujete kopii s vlastním názvem.",
    "**Change the keys.** Drag the diamonds in the view, change times in the table, add keys with **{{AnimAddKey}}**.":
        "**Změňte klíče.** Táhněte kosočtverce v pohledu, měňte časy v tabulce, přidávejte klíče pomocí **{{AnimAddKey}}**.",
    "Editing ROFF keys.":
        "Úprava klíčů ROFF.",
    "**Save.** Saving the script writes `scripts/<name>.rof`; **{{MissionSave}}** puts it into the `.pk3`.":
        "**Uložení.** Uložení skriptu zapíše `scripts/<name>.rof`; **{{MissionSave}}** ho vloží do `.pk3`.",
    "Building a ROFF from scratch: [[tut-p5]]. The details: [[animation#roff]].":
        "Sestavení ROFF od začátku: [[tut-p5]]. Podrobnosti: [[animation#roff]].",
    "Connect a character to a script":
        "Připojit postavu ke skriptu",
    "In the Graph view: drag a wire from a character to a script, and choose when it runs.":
        "V zobrazení Graf: táhněte spoj od postavy ke skriptu a zvolte, kdy se spustí.",
    "Missions · four steps":
        "Mise · čtyři kroky",
    "**Open the Graph.** Above the script column, switch from **{{FlowClassic}}** to **{{FlowGraph}}**. Every entity and script of the mission is a node.":
        "**Otevřete Graf.** Nad sloupcem skriptu přepněte z **{{FlowClassic}}** na **{{FlowGraph}}**. Každá entita a skript mise je uzel.",
    "**Drag a wire.** Drag from the dot on the right of a character node to a script node.":
        "**Táhněte spoj.** Táhněte od tečky vpravo u uzlu postavy k uzlu skriptu.",
    "The Graph view.":
        "Zobrazení Graf.",
    "**Choose when.** A menu asks **{{GraphConnectAs}}**: `spawnscript` (when it appears), `deathscript`, `usescript` and more. behaved writes the key into the entity.":
        "**Zvolte kdy.** Nabídka se zeptá **{{GraphConnectAs}}**: `spawnscript` (když se objeví), `deathscript`, `usescript` a další. behaved zapíše klíč do entity.",
    "**New script.** Drag a command from the **{{GroupEvents}}** list onto empty space in the graph: a new script with that command is made.":
        "**Nový skript.** Přetáhněte příkaz ze seznamu **{{GroupEvents}}** na prázdné místo v grafu: vytvoří se nový skript s tímto příkazem.",
    "Continue and pack a mission":
        "Pokračovat v misi a zabalit ji",
    "Open your mission again, check it and pack it into a .pk3 for the game.":
        "Otevřete svou misi znovu, zkontrolujte ji a zabalte do .pk3 pro hru.",
    "**Open it again.** **{{MenuFile}} → {{RecentMissions}}**, or the list on the start page of the **{{ViewMap}}** tab, opens map, entities and every script with one click. **{{MissionOpenFolder}}** opens one from its folder.":
        "**Otevřete ji znovu.** **{{MenuFile}} → {{RecentMissions}}**, nebo seznam na úvodní stránce karty **{{ViewMap}}**, otevře mapu, entity a každý skript jedním kliknutím. **{{MissionOpenFolder}}** otevře misi z její složky.",
    "**Work.** Change scripts and entities. A dot next to the mission name means unsaved changes.":
        "**Práce.** Měňte skripty a entity. Tečka vedle názvu mise znamená neuložené změny.",
    "**Save mission.** **{{MissionSave}}** shows every file it will pack and checks the mission: errors in red (they must be fixed), warnings in orange.":
        "**Uložení mise.** **{{MissionSave}}** ukáže každý soubor, který zabalí, a misi zkontroluje: chyby červeně (musí se opravit), varování oranžově.",
    "The Save mission window.":
        "Okno Uložit misi.",
    "Where the `.pk3` is written; **{{MissionChoose}}** picks another place.":
        "Kam se `.pk3` zapíše; **{{MissionChoose}}** vybere jiné místo.",
    "Saves, compiles and packs - or closes the window. Above them: every file of the package and the checks.":
        "Uloží, zkompiluje a zabalí - nebo zavře okno. Nad nimi: každý soubor balíčku a kontroly.",
    "**Pack.** Press **{{MissionSaveNow}}**: scripts are saved and compiled, the `.ent` written and the `.pk3` packed. Start the game with `devmap <name>`.":
        "**Zabalení.** Stiskněte **{{MissionSaveNow}}**: skripty se uloží a zkompilují, `.ent` se zapíše a `.pk3` zabalí. Spusťte hru s `devmap <name>`.",
    "Give NPCs paths to walk":
        "Dát NPC cesty k chůzi",
    "Create the NPC waypoint network automatically, check it, and add paths by hand.":
        "Vytvořte síť navigačních bodů NPC automaticky, zkontrolujte ji a přidejte cesty ručně.",
    "**Open the window.** **{{WegTitel}}** on the map toolbar. **{{WegZeigen}}** shows the network: green by hand, blue automatic, orange jumps.":
        "**Otevřete okno.** **{{WegTitel}}** na liště mapy. **{{WegZeigen}}** ukáže síť: zeleně ručně, modře automaticky, oranžově skoky.",
    "**Create automatically.** Press **{{WegErzeugen}}**: waypoints are spread over the walkable floor and connected. The spacing slider sets how dense (the original levels use about 230).":
        "**Vytvořit automaticky.** Stiskněte **{{WegErzeugen}}**: navigační body se rozprostřou po pochozí podlaze a spojí se. Posuvník rozestupu nastavuje hustotu (původní úrovně používají asi 230).",
    "The NPC waypoints window.":
        "Okno navigačních bodů NPC.",
    "Shows the network on the map.":
        "Zobrazí síť v mapě.",
    "Place points by clicking the floor / connect two points.":
        "Umisťování bodů kliknutím na podlahu / spojení dvou bodů.",
    "Spread a network over the walkable floor / remove what was created.":
        "Rozprostřít síť po pochozí podlaze / odebrat vytvořené.",
    "Finds what the game would reject; click a problem to fly there.":
        "Najde, co by hra odmítla; kliknutím na problém tam přeletíte.",
    "**Check.** **{{WegPruefen}}** finds what the game would reject - click a problem to fly there.":
        "**Kontrola.** **{{WegPruefen}}** najde, co by hra odmítla - kliknutím na problém tam přeletíte.",
    "**By hand.** **{{WegSetzen}}**: click the floor to place a path; **{{WegVerbinden}}** connects two points (<kbd>Ctrl</kbd>: a jump).":
        "**Ručně.** **{{WegSetzen}}**: kliknutím na podlahu umístíte cestu; **{{WegVerbinden}}** spojí dva body (<kbd>Ctrl</kbd>: skok).",
    "More in [[wegpunkte]].":
        "Více v [[wegpunkte]].",
    "Look at a character model":
        "Prohlédnout si model postavy",
    "Load any character of the game, turn it, look at its skins and its skeleton.":
        "Načtěte libovolnou postavu hry, otáčejte jí, prohlédněte si její skiny a kostru.",
    "Model · four steps":
        "Model · čtyři kroky",
    "**Switch to {{ViewModel}}** in the mode bar.":
        "**Přepněte na {{ViewModel}}** v liště režimů.",
    "**Pick a character.** **{{NpcBrowser}}** lists every character of the game - type to search, click to load it with its skin.":
        "**Vyberte postavu.** **{{NpcBrowser}}** vypíše každou postavu hry - pište pro hledání, kliknutím ji načtete s jejím skinem.",
    "**Look around.** Left mouse turns, middle mouse moves, the wheel zooms. **{{ModelTurntable}}** turns it by itself, **{{ModelResetView}}** brings it back to the front.":
        "**Rozhlédněte se.** Levé tlačítko otáčí, prostřední posouvá, kolečko přibližuje. **{{ModelTurntable}}** ji otáčí samo, **{{ModelResetView}}** ji vrátí zepředu.",
    "The model view.":
        "Zobrazení modelu.",
    "**Details.** **{{ModelSkin}}** switches skins, **{{ModelBones}}** shows the skeleton, **{{ModelTextures}}** switches textures off.":
        "**Podrobnosti.** **{{ModelSkin}}** přepíná skiny, **{{ModelBones}}** ukáže kostru, **{{ModelTextures}}** vypne textury.",
    "Tutorials: scripts":
        "Tutoriály: skripty",
    "Tutorials: camera and timeline":
        "Tutoriály: kamera a časová osa",
    "Tutorials: figures":
        "Tutoriály: postavy",
    "Tutorials: map and entities":
        "Tutoriály: mapa a entity",
    "Tutorials: animation":
        "Tutoriály: animace",
    "Tutorials: missions and models":
        "Tutoriály: mise a modely",
    "Only the first time: variables and if":
        "Jen poprvé: proměnné a if",
    "Remember that something has happened - declare a variable, set it, ask for it with if.":
        "Zapamatujte si, že se něco stalo - deklarujte proměnnou, nastavte ji, zeptejte se na ni pomocí if.",
    "**Declare once.** In a script that runs only once - the intro, a spawn script - insert `declare`, choose `FLOAT` and type a name, for example `seen`. A declared variable starts at 0; a map can hold 16 of them.":
        "**Deklarujte jednou.** Do skriptu, který běží jen jednou - úvod, spawn skript - vložte `declare`, zvolte `FLOAT` a napište název, například `seen`. Deklarovaná proměnná začíná na 0; mapa jich pojme 16.",
    "**Ask with if.** In the script that may run more often (a trigger's script, see [[k-trigger]]) insert `if`. In the editor press **{{EditorHelper}}** on the first value and build `get ( FLOAT, \"seen\" )` with **{{EditorGet}}**; as comparison choose `=`, as the other value `0`.":
        "**Zeptejte se pomocí if.** Do skriptu, který může běžet častěji (skript triggeru, viz [[k-trigger]]), vložte `if`. V editoru stiskněte u první hodnoty **{{EditorHelper}}** a sestavte `get ( FLOAT, \"seen\" )` pomocí **{{EditorGet}}**; jako porovnání zvolte `=`, jako druhou hodnotu `0`.",
    "The event editor for if: get ( FLOAT, \"seen\" ), =, 0.":
        "Editor událostí pro if: get ( FLOAT, \"seen\" ), =, 0.",
    "**Do it inside.** Put the commands of the scene into the `if` block. An `else` right after it runs when the scene was already seen.":
        "**Udělejte to uvnitř.** Vložte příkazy scény do bloku `if`. `else` hned za ním se spustí, když už scéna byla viděna.",
    "**Remember.** At the end of the `if` block insert `set` with the variable name `seen` and the value `1` - next time the `if` is false.":
        "**Zapamatujte si.** Na konec bloku `if` vložte `set` s názvem proměnné `seen` a hodnotou `1` - příště bude `if` nepravdivé.",
    "The finished if / else with declare and set.":
        "Hotové if / else s declare a set.",
    "**Free it** with `free ( \"seen\" )` when it is no longer needed, so the name is available again.":
        "**Uvolněte ji** pomocí `free ( \"seen\" )`, když už není potřeba, aby byl název znovu k dispozici.",
    "Comparisons are `=`, `<`, `>` and `!` (not equal) - behaved warns about anything else.":
        "Porovnání jsou `=`, `<`, `>` a `!` (nerovná se) - na cokoli jiného behaved upozorní.",
    "A guard walks his round":
        "Strážný obchází svou pochůzku",
    "A loop around walk targets: the figure walks from point to point, waits, and starts again - forever.":
        "Smyčka kolem cílů chůze: postava jde od bodu k bodu, čeká a začne znovu - navždy.",
    "**Place the round.** In the **{{ViewMap}}** tab place three or four walk targets with the tile **{{EntKindNav}}** (<kbd>Shift</kbd> places several) and name them `round1`, `round2`, `round3`.":
        "**Rozmístěte pochůzku.** Na kartě **{{ViewMap}}** umístěte tři nebo čtyři cíle chůze dlaždicí **{{EntKindNav}}** (<kbd>Shift</kbd> umístí několik) a pojmenujte je `round1`, `round2`, `round3`.",
    "**The guard.** Place a character (**{{EntKindNpc}}**), name it `guard`, and give it an `affect ( \"guard\", FLUSH )` block in the script. Inside, insert the macro `walkOnly` first.":
        "**Strážný.** Umístěte postavu (**{{EntKindNpc}}**), pojmenujte ji `guard` a dejte jí ve skriptu blok `affect ( \"guard\", FLUSH )`. Dovnitř nejdřív vložte makro `walkOnly`.",
    "**The loop.** Insert `loop` and keep `-1` - forever.":
        "**Smyčka.** Vložte `loop` a ponechte `-1` - navždy.",
    "**One leg per point.** Inside the loop: a `task` with `set ( SET_NAVGOAL, round1 )`, then `dowait` for it, then `wait ( 2000 )`. Copy these three lines for each point and change the name. Play the timeline - the guard walks the round.":
        "**Jeden úsek na bod.** Uvnitř smyčky: `task` s `set ( SET_NAVGOAL, round1 )`, pak `dowait` na něj, pak `wait ( 2000 )`. Tyto tři řádky zkopírujte pro každý bod a změňte název. Přehrajte časovou osu - strážný obchází pochůzku.",
    "The guard's loop: a task, dowait and wait for each point.":
        "Smyčka strážného: task, dowait a wait pro každý bod.",
    "**Run instead.** Replace `walkOnly` with `runOnly` - or right-click a walk target on the path and choose **{{BahnRennen}}**.":
        "**Místo toho běh.** Nahraďte `walkOnly` za `runOnly` - nebo klikněte pravým tlačítkem na cíl chůze na dráze a zvolte **{{BahnRennen}}**.",
    "The guard and his three walk targets on the pad.":
        "Strážný a jeho tři cíle chůze na plošině.",
    "The macros `patrolRun`, `patrolNoChase` and `patrolWalkNoChase` set how a patrolling guard reacts to enemies.":
        "Makra `patrolRun`, `patrolNoChase` a `patrolWalkNoChase` nastavují, jak hlídkující strážný reaguje na nepřátele.",
    "Let the camera follow a figure":
        "Nechat kameru sledovat postavu",
    "camera FOLLOW turns the camera after everyone of a camera group - and DISTANCE keeps it close.":
        "camera FOLLOW otáčí kameru za každým z kamerové skupiny - a DISTANCE ji drží blízko.",
    "**Put the figure into a group.** In the figure's `affect` block insert `set ( SET_CAMERA_GROUP, \"hero\" )`.":
        "**Zařaďte postavu do skupiny.** Do bloku `affect` postavy vložte `set ( SET_CAMERA_GROUP, \"hero\" )`.",
    "**Follow.** After the camera shot insert `camera` and choose `FOLLOW`: the group `hero`, a turning speed (for example `100`) and `1` to blend into it. The camera now keeps turning towards the figure.":
        "**Sledování.** Za záběr kamery vložte `camera` a zvolte `FOLLOW`: skupinu `hero`, rychlost otáčení (například `100`) a `1` pro plynulý přechod. Kamera se teď stále otáčí k postavě.",
    "**Keep a distance.** `camera ( DISTANCE, 200, 1 )` keeps the camera 200 units from the group, moving with it.":
        "**Držet odstup.** `camera ( DISTANCE, 200, 1 )` drží kameru 200 jednotek od skupiny a pohybuje se s ní.",
    "Through the camera: it keeps turning towards Kyle.":
        "Pohledem kamery: stále se otáčí ke Kylovi.",
    "`SET_CAMERA_GROUP_Z_OFS` aims higher or lower on the followed figure.":
        "`SET_CAMERA_GROUP_Z_OFS` míří na sledovanou postavu výš nebo níž.",
    "Show a video or a line of text":
        "Zobrazit video nebo řádek textu",
    "SET_VIDEO_PLAY for a .roq film in the middle of a scene, print for a short text on screen.":
        "SET_VIDEO_PLAY pro film .roq uprostřed scény, print pro krátký text na obrazovce.",
    "Camera · three steps":
        "Kamera · tři kroky",
    "**A video.** Insert `set` and choose `SET_VIDEO_PLAY`; as the value type the name of a film in `video/` without its ending - Jedi Academy plays `.roq` files this way, for example its intro `jk0101_sw`.":
        "**Video.** Vložte `set` a zvolte `SET_VIDEO_PLAY`; jako hodnotu napište název filmu ve `video/` bez přípony - Jedi Academy takto přehrává soubory `.roq`, například svůj úvod `jk0101_sw`.",
    "SET_VIDEO_PLAY in the event editor.":
        "SET_VIDEO_PLAY v editoru událostí.",
    "**Fade it.** `SET_VIDEO_FADE_IN` and `SET_VIDEO_FADE_OUT` (`true`) set before the video let it fade.":
        "**Prolínání.** `SET_VIDEO_FADE_IN` a `SET_VIDEO_FADE_OUT` (`true`) nastavené před videem ho nechají prolnout.",
    "**Text.** `print ( \"Three days later...\" )` shows a line in the middle of the screen.":
        "**Text.** `print ( \"Three days later...\" )` zobrazí řádek uprostřed obrazovky.",
    "Video, fade and a line of text in the tree.":
        "Video, prolínání a řádek textu ve stromu.",
    "behaved does not play videos in the timeline; it shows the command as a block so the timing stays visible.":
        "behaved videa na časové ose nepřehrává; ukazuje příkaz jako blok, aby načasování zůstalo vidět.",
    "Start a fight after the cutscene":
        "Začít boj po animační sekvenci",
    "Keep an enemy calm while the camera runs, then let him attack the player.":
        "Udržte nepřítele v klidu, dokud běží kamera, pak ho nechte zaútočit na hráče.",
    "**Calm at first.** Give the enemy a spawn script (key `spawnscript`, **{{EntOpenScript}}** creates it) with the macro `standOnly` - he stands and ignores enemies.":
        "**Nejdřív klid.** Dejte nepříteli spawn skript (klíč `spawnscript`, **{{EntOpenScript}}** ho vytvoří) s makrem `standOnly` - stojí a ignoruje nepřátele.",
    "**The scene.** The cutscene runs as usual; the enemy can walk and talk like any figure.":
        "**Scéna.** Animační sekvence běží jako obvykle; nepřítel může chodit a mluvit jako každá postava.",
    "**End the camera.** At the end of the intro: `camera ( DISABLE )` - the player plays again.":
        "**Ukončete kameru.** Na konci úvodu: `camera ( DISABLE )` - hráč zase hraje.",
    "**Attack.** Right after it, an `affect ( \"enemy\", FLUSH )` block with `set ( SET_IGNOREENEMIES, false )`, `set ( SET_LOOK_FOR_ENEMIES, true )` and `set ( SET_ENEMY, \"player\" )`. The macro `default` resets all behaviour in one line.":
        "**Útok.** Hned poté blok `affect ( \"enemy\", FLUSH )` s `set ( SET_IGNOREENEMIES, false )`, `set ( SET_LOOK_FOR_ENEMIES, true )` a `set ( SET_ENEMY, \"player\" )`. Makro `default` obnoví veškeré chování na jednom řádku.",
    "After camera ( DISABLE ): the enemy turns on the player.":
        "Po camera ( DISABLE ): nepřítel se obrátí proti hráči.",
    "**More than one.** Copy the block for each enemy, or put the lines into a script of their own and start it with `run`.":
        "**Víc než jeden.** Zkopírujte blok pro každého nepřítele, nebo dejte řádky do vlastního skriptu a spusťte ho pomocí `run`.",
    "`SET_BEHAVIOR_STATE` with `BS_DEFAULT` gives the figure back to its normal AI after scripted walking.":
        "`SET_BEHAVIOR_STATE` s `BS_DEFAULT` vrátí postavu po skriptované chůzi její normální AI.",
    "Hold the player during a talk":
        "Zadržet hráče během rozhovoru",
    "Lock the player in place - without a cutscene camera - and free him again.":
        "Zamkněte hráče na místě - bez kamery animační sekvence - a zase ho uvolněte.",
    "Figures · three steps":
        "Postavy · tři kroky",
    "**Lock.** Insert `affect ( \"player\", FLUSH )` and inside it `set ( SET_PLAYER_LOCKED, true )`. `SET_LOCK_PLAYER_WEAPONS` keeps the weapon as well.":
        "**Zamknutí.** Vložte `affect ( \"player\", FLUSH )` a v něm `set ( SET_PLAYER_LOCKED, true )`. `SET_LOCK_PLAYER_WEAPONS` zamkne i zbraň.",
    "**Talk.** The other figures speak and move as usual; the player watches from his own view.":
        "**Rozhovor.** Ostatní postavy mluví a pohybují se jako obvykle; hráč přihlíží ze svého pohledu.",
    "**Free.** At the end the same block with `false` - otherwise the player stays frozen.":
        "**Uvolnění.** Na konci tentýž blok s `false` - jinak hráč zůstane zmrazený.",
    "Lock and free the player around a talk.":
        "Zamknutí a uvolnění hráče kolem rozhovoru.",
    "With `camera ( ENABLE )` the game takes the controls away by itself; the lock is for moments without a cutscene camera.":
        "S `camera ( ENABLE )` hra odebere ovládání sama; zámek je pro chvíle bez kamery animační sekvence.",
    "Ignite a lightsaber in a scene":
        "Zapnout světelný meč ve scéně",
    "Give a figure its saber and switch the blade on and off at the right moment.":
        "Dejte postavě její meč a zapněte a vypněte čepel ve správnou chvíli.",
    "**The weapon.** In the figure's `affect` block (or its spawn script): `set ( SET_WEAPON, WP_SABER )`.":
        "**Zbraň.** V bloku `affect` postavy (nebo v jejím spawn skriptu): `set ( SET_WEAPON, WP_SABER )`.",
    "**On.** Where it should ignite: `set ( SET_SABERACTIVE, true )`. In the timeline the blade appears at that moment - with its sound when **{{TlAudio}}** is on and the camera can see the figure, as in the game.":
        "**Zapnutí.** Kde se má zapnout: `set ( SET_SABERACTIVE, true )`. Na časové ose se čepel objeví v tu chvíli - se zvukem, když je zapnuto **{{TlAudio}}** a kamera postavu vidí, jako ve hře.",
    "Kyle with his blade ignited, in the view.":
        "Kyle se zapnutou čepelí v pohledu.",
    "**Off.** `set ( SET_SABERACTIVE, false )`.":
        "**Vypnutí.** `set ( SET_SABERACTIVE, false )`.",
    "The saber commands in Kyle's affect block.":
        "Příkazy meče v Kylově bloku affect.",
    "`SET_SABER1` gives the figure another saber by its name from the game's saber files.":
        "`SET_SABER1` dá postavě jiný meč podle jeho názvu ze souborů mečů hry.",
    "**Walls.** A blade that touches a wall is shortened there, with sparks, sound and a burn mark - [[figuren#schwerter]].":
        "**Zdi.** Čepel, která se dotkne zdi, se tam zkrátí, s jiskrami, zvukem a spáleninou - [[figuren#schwerter]].",
    "**Clash.** For two blades to clash in Movie Duels, put both NPCs in `BS_CINEMATIC` and add `set ( SET_SABERCOLLISIONS, true )` before the attack animation. Under OpenJK rules only the animations `BOTH_CIN_16` and `BOTH_CIN_17` clash.":
        "**Střet.** Aby se dvě čepele v Movie Duels střetly, dejte obě NPC do `BS_CINEMATIC` a před útočnou animaci přidejte `set ( SET_SABERCOLLISIONS, true )`. Podle pravidel OpenJK se střetávají jen animace `BOTH_CIN_16` a `BOTH_CIN_17`.",
    "Start a scene when the player walks somewhere":
        "Spustit scénu, když hráč někam dojde",
    "Point a trigger of the map at a script runner of your own - the scene starts when the player enters the area.":
        "Namiřte trigger mapy na vlastní spouštěč skriptu - scéna začne, když hráč vstoupí do oblasti.",
    "**A runner.** In the **{{ViewMap}}** tab click the tile **{{EntKindRunner}}** and click near the place. behaved names it and sets its `usescript` to a script in your mission folder.":
        "**Spouštěč.** Na kartě **{{ViewMap}}** klikněte na dlaždici **{{EntKindRunner}}** a klikněte poblíž místa. behaved ho pojmenuje a nastaví jeho `usescript` na skript ve složce vaší mise.",
    "**Its script.** In the inspector press **{{EntOpenScript}}** next to `usescript`: the script is created and opened. Write the scene in it.":
        "**Jeho skript.** V inspektoru stiskněte **{{EntOpenScript}}** vedle `usescript`: skript se vytvoří a otevře. Napište do něj scénu.",
    "**Find a trigger.** Triggers are brushes of the map: type `trigger` into the search box of the entity list and pick one where the player passes. Fly there with a double-click.":
        "**Najděte trigger.** Triggery jsou brushe mapy: napište `trigger` do vyhledávacího pole seznamu entit a vyberte takový, kudy hráč prochází. Dvojklikem tam přeletíte.",
    "**Point it at the runner.** In the trigger's inspector set its key `target` to the runner's name. The **{{FlowGraph}}** view now shows the arrow from trigger to runner to script.":
        "**Namiřte ho na spouštěč.** V inspektoru triggeru nastavte jeho klíč `target` na název spouštěče. Zobrazení **{{FlowGraph}}** teď ukazuje šipku od triggeru ke spouštěči a ke skriptu.",
    "The trigger's keys: target now names the runner.":
        "Klíče triggeru: target teď jmenuje spouštěč.",
    "**Once or every time.** A runner runs once; its key `count` set to `-1` runs it every time. For only-the-first-time logic see [[k-variablen]].":
        "**Jednou nebo pokaždé.** Spouštěč se spustí jednou; s klíčem `count` nastaveným na `-1` se spouští pokaždé. Logiku jen napoprvé viz [[k-variablen]].",
    "New trigger areas need brushes, which are made in a map editor - behaved can change every trigger of a map, but not draw a new one. A script can also start a runner directly with `use`.":
        "Nové oblasti triggerů potřebují brushe, které se dělají v editoru map - behaved může změnit každý trigger mapy, ale nový nenakreslí. Skript může spouštěč spustit i přímo pomocí `use`.",
    "Open a door from the script":
        "Otevřít dveře ze skriptu",
    "use opens a door, fires a trigger or starts a runner - by its name.":
        "use otevře dveře, aktivuje trigger nebo spustí spouštěč - podle jeho názvu.",
    "**Find the door.** Doors are in the group **Movable (func_*)** of the entity list. Click one to see it in the inspector.":
        "**Najděte dveře.** Dveře jsou ve skupině **Pohyblivé (func_*)** seznamu entit. Kliknutím na ně je uvidíte v inspektoru.",
    "**Its name.** The door needs a `targetname`. A door with a name waits to be used instead of opening when someone walks up - which is what a scripted door should do.":
        "**Jejich název.** Dveře potřebují `targetname`. Dveře s názvem čekají na použití, místo aby se otevřely, když k nim někdo přijde - přesně to mají skriptované dveře dělat.",
    "**Insert use.** Select the line in the script after which the door should open, then right-click the door in the entity list → **{{EntityUse}}**: `use ( \"door1\" )` is inserted.":
        "**Vložte use.** Vyberte ve skriptu řádek, po kterém se mají dveře otevřít, a pak klikněte pravým tlačítkem na dveře v seznamu entit → **{{EntityUse}}**: vloží se `use ( \"door1\" )`.",
    "Right-click a door in the entity list: Insert use.":
        "Pravé tlačítko na dveřích v seznamu entit: Vložit use.",
    "**Hear it.** The door plays its own sounds (its `soundSet`). Play the timeline to check the moment.":
        "**Poslechněte si je.** Dveře přehrávají vlastní zvuky (svůj `soundSet`). Přehrajte časovou osu a zkontrolujte okamžik.",
    "Moving the door yourself instead - other distance, other speed: [[k-tuer]].":
        "Místo toho posouvat dveře sami - jiná vzdálenost, jiná rychlost: [[k-tuer]].",
    "Go to the next map at the end":
        "Na konci přejít na další mapu",
    "A level change entity, used by the last line of the script.":
        "Entita změny úrovně, použitá posledním řádkem skriptu.",
    "**Place it.** **{{EntNew}}** → **{{EntKindLevel}}**. In the inspector set `mapname` to the next map (for example `taspir2`) and give it a **{{EntName}}** such as `next_map`.":
        "**Umístěte ji.** **{{EntNew}}** → **{{EntKindLevel}}**. V inspektoru nastavte `mapname` na další mapu (například `taspir2`) a dejte jí **{{EntName}}**, například `next_map`.",
    "The level change with mapname and a name.":
        "Změna úrovně s mapname a názvem.",
    "**Use it.** At the end of the script: `use ( \"next_map\" )` (right-click the entity → **{{EntityUse}}**).":
        "**Použijte ji.** Na konci skriptu: `use ( \"next_map\" )` (pravé tlačítko na entitě → **{{EntityUse}}**).",
    "**Check it.** **{{MissionSave}}** warns when a target points to nothing; play the mission to see the loading screen.":
        "**Zkontrolujte to.** **{{MissionSave}}** upozorní, když cíl na nic neukazuje; zahrajte si misi a uvidíte načítací obrazovku.",
    "Add an effect and change the music":
        "Přidat efekt a změnit hudbu",
    "An fx_runner that starts on cue, and a music change from the script.":
        "fx_runner, který začne na povel, a změna hudby ze skriptu.",
    "**An effect.** Click the tile **{{EntKindFx}}** and click where it should appear. In the inspector set `fxFile` to an effect of the game (a path under `effects/`, for example `env/fire`), give it a name, and tick `STARTOFF` so it waits.":
        "**Efekt.** Klikněte na dlaždici **{{EntKindFx}}** a klikněte tam, kde se má objevit. V inspektoru nastavte `fxFile` na efekt hry (cesta pod `effects/`, například `env/fire`), dejte mu název a zaškrtněte `STARTOFF`, aby čekal.",
    "**Start it.** In the script: `use ( \"fire1\" )` at the right moment. **{{MapEffects}}** on the toolbar shows effects in the view.":
        "**Spusťte ho.** Ve skriptu: `use ( \"fire1\" )` ve správnou chvíli. **{{MapEffects}}** na liště ukáže efekty v pohledu.",
    "A fire effect on the landing pad.":
        "Efekt ohně na přistávací plošině.",
    "**Music.** **{{EntNew}}** → **{{EntKindMusic}}**, set `music` to a music file and a name; `use` it from the script to switch the music.":
        "**Hudba.** **{{EntNew}}** → **{{EntKindMusic}}**, nastavte `music` na hudební soubor a název; ze skriptu ji přepnete pomocí `use`.",
    "**Dynamic music.** `set ( SET_MUSIC_STATE, ... )` switches the game's own music between its states, for example to action.":
        "**Dynamická hudba.** `set ( SET_MUSIC_STATE, ... )` přepíná vlastní hudbu hry mezi jejími stavy, například na akci.",
    "Change a cutscene of the game":
        "Změnit animační sekvenci hry",
    "Open a mission of Jedi Academy, change a camera shot of its intro, and pack it as your own version.":
        "Otevřete misi Jedi Academy, změňte záběr kamery v jejím úvodu a zabalte ji jako vlastní verzi.",
    "Missions · five steps":
        "Mise · pět kroků",
    "**Open it.** **{{MenuFile}} → {{LoadMission}}**, choose `assets0.pk3` in the game's `base` folder and pick a map, for example `taspir1`. The map and all its scripts open in tabs.":
        "**Otevřete ji.** **{{MenuFile}} → {{LoadMission}}**, zvolte `assets0.pk3` ve složce hry `base` a vyberte mapu, například `taspir1`. Mapa a všechny její skripty se otevřou na kartách.",
    "The missions of an archive.":
        "Mise archivu.",
    "**Find the scene.** In the **{{ViewMap}}** tab, **Scripts (N)** lists the scripts the map uses - pick the intro. The timeline shows its camera.":
        "**Najděte scénu.** Na kartě **{{ViewMap}}** vypíše **Skripty (N)** skripty, které mapa používá - vyberte úvod. Časová osa ukazuje jeho kameru.",
    "**Change a shot.** Select a shot in the **{{CamShots}}** list, fly to a better view and press **{{CamSetToView}}** - or change a time in the timeline.":
        "**Změňte záběr.** Vyberte záběr v seznamu **{{CamShots}}**, přeleťte k lepšímu pohledu a stiskněte **{{CamSetToView}}** - nebo změňte čas na časové ose.",
    "**Pack it.** **{{MissionSave}}** writes the changed scripts with the map into `zz_<map>.pk3`. Archives load in alphabetical order, so `zz_` wins over the game's own files.":
        "**Zabalte ji.** **{{MissionSave}}** zapíše změněné skripty s mapou do `zz_<map>.pk3`. Archivy se načítají v abecedním pořadí, takže `zz_` vyhraje nad vlastními soubory hry.",
    "**Undo it.** Delete the `zz_` file from the game folder and the game plays its original again - the game files are never touched.":
        "**Vraťte to.** Smažte soubor `zz_` ze složky hry a hra znovu hraje svůj originál - soubory hry se nikdy nemění.",
    "Check a script for multiplayer":
        "Zkontrolovat skript pro hru více hráčů",
    "Switch the target game and read what works differently on a multiplayer server.":
        "Přepněte cílovou hru a přečtěte si, co na serveru pro více hráčů funguje jinak.",
    "Missions · three steps":
        "Mise · tři kroky",
    "**Switch.** **{{MenuView}} → {{ViewTarget}}** → multiplayer. An **MP** badge appears in the status line.":
        "**Přepnutí.** **{{MenuView}} → {{ViewTarget}}** → hra pro více hráčů. Ve stavovém řádku se objeví odznak **MP**.",
    "**Read the notes.** Click the notes counter in the status line: the **{{MessagesTitle}}** window lists every command that does nothing or works differently on a server - checked against the multiplayer game code.":
        "**Přečtěte si poznámky.** Klikněte na počítadlo poznámek ve stavovém řádku: okno **{{MessagesTitle}}** vypíše každý příkaz, který na serveru nic nedělá nebo funguje jinak - ověřeno proti kódu hry pro více hráčů.",
    "The Messages window with the multiplayer notes.":
        "Okno Zprávy s poznámkami pro hru více hráčů.",
    "**Switch back** to single player when you are done; the script itself is not changed.":
        "**Přepněte zpět** na hru pro jednoho hráče, až budete hotovi; samotný skript se nemění.",
    "Set up the program your way":
        "Nastavit program po svém",
    "Shortcuts, theme, language, interface size and the column layout.":
        "Zkratky, motiv, jazyk, velikost rozhraní a rozvržení sloupců.",
    "Settings · five steps":
        "Nastavení · pět kroků",
    "**Shortcuts.** **{{MenuEdit}} → {{KeysTitle}}**: click a shortcut, press the new key. <kbd>Esc</kbd> cancels, <kbd>Backspace</kbd> clears; a clash shows which action already has the key.":
        "**Zkratky.** **{{MenuEdit}} → {{KeysTitle}}**: klikněte na zkratku, stiskněte novou klávesu. <kbd>Esc</kbd> zruší, <kbd>Backspace</kbd> vymaže; při kolizi se ukáže, která akce už klávesu má.",
    "The Keyboard shortcuts window.":
        "Okno Klávesové zkratky.",
    "**Theme and language.** **{{MenuView}} → {{ViewTheme}}** (six themes, among them BehavEd Classic and High Contrast) and **{{ViewLanguage}}**.":
        "**Motiv a jazyk.** **{{MenuView}} → {{ViewTheme}}** (šest motivů, mezi nimi BehavEd klasický a Vysoký kontrast) a **{{ViewLanguage}}**.",
    "**Size.** **{{MenuView}} → {{ViewDpiScale}}**, or <kbd>Ctrl</kbd>+mouse wheel anywhere outside the timeline.":
        "**Velikost.** **{{MenuView}} → {{ViewDpiScale}}**, nebo <kbd>Ctrl</kbd>+kolečko myši kdekoli mimo časovou osu.",
    "**Columns.** Drag the dividers between the columns; each mode keeps its own widths. **{{MenuDebug}} → Restore default layout** brings them back.":
        "**Sloupce.** Táhněte oddělovače mezi sloupci; každý režim si pamatuje vlastní šířky. **{{MenuDebug}} → Obnovit výchozí rozvržení** je vrátí.",
    "**The guide.** <kbd>F1</kbd> opens this guide - also rebindable.":
        "**Příručka.** <kbd>F1</kbd> otevře tuto příručku - i tuto klávesu lze změnit.",
    "All settings: [[einstellungen]]. All keys: [[tasten]].":
        "Všechna nastavení: [[einstellungen]]. Všechny klávesy: [[tasten]].",
    "When the enemy dies, the door opens":
        "Když nepřítel zemře, otevřou se dveře",
    "A deathscript on a figure: the moment it falls, a script of yours runs - here it opens a door.":
        "Deathscript na postavě: ve chvíli, kdy padne, se spustí váš skript - zde otevře dveře.",
    "**Give the figure a deathscript.** Select the enemy in the **{{ViewMap}}** tab. In the inspector add the key `deathscript` with a script name of your mission, for example `taspir_tut/rosh_dies` - or, in the **{{FlowGraph}}** view, drag a wire from the figure to a script and choose `deathscript` under **{{GraphConnectAs}}**.":
        "**Dejte postavě deathscript.** Vyberte nepřítele na kartě **{{ViewMap}}**. V inspektoru přidejte klíč `deathscript` s názvem skriptu vaší mise, například `taspir_tut/rosh_dies` - nebo v zobrazení **{{FlowGraph}}** táhněte spoj od postavy ke skriptu a pod **{{GraphConnectAs}}** zvolte `deathscript`.",
    "The enemy's deathscript in the inspector.":
        "Deathscript nepřítele v inspektoru.",
    "**Create the script.** Press **{{EntOpenScript}}** next to the key: the script is created in the mission folder and opened.":
        "**Vytvořte skript.** Stiskněte **{{EntOpenScript}}** vedle klíče: skript se vytvoří ve složce mise a otevře.",
    "**Open the door.** In it, insert `use` with the door's name - right-click the door in the entity list → **{{EntityUse}}** does it for you. A `wait` before it lets the moment breathe.":
        "**Otevřete dveře.** Vložte do něj `use` s názvem dveří - pravé tlačítko na dveřích v seznamu entit → **{{EntityUse}}** to udělá za vás. `wait` před ním dá okamžiku prostor.",
    "The deathscript: a short wait, then the door.":
        "Deathscript: krátké čekání, pak dveře.",
    "**Other moments.** The same works with `painscript` (hit), `awakescript` (notices the player), `angerscript` (starts fighting), `fleescript` and `spawnscript`.":
        "**Jiné okamžiky.** Totéž funguje s `painscript` (zásah), `awakescript` (všimne si hráče), `angerscript` (začne bojovat), `fleescript` a `spawnscript`.",
    "The player talks to a figure":
        "Hráč mluví s postavou",
    "usescript and SET_PLAYER_USABLE: pressing the use key on a figure starts its script - a conversation.":
        "usescript a SET_PLAYER_USABLE: stisk klávesy použití na postavě spustí její skript - rozhovor.",
    "**Make it usable.** In the figure's spawn script insert `set ( SET_PLAYER_USABLE, true )` - otherwise the use key does nothing.":
        "**Udělejte ji použitelnou.** Do spawn skriptu postavy vložte `set ( SET_PLAYER_USABLE, true )` - jinak klávesa použití nic nedělá.",
    "**The script for the talk.** Add the key `usescript` to the figure (or connect it in the **{{FlowGraph}}** view as `usescript`) and press **{{EntOpenScript}}**.":
        "**Skript pro rozhovor.** Přidejte postavě klíč `usescript` (nebo ji spojte v zobrazení **{{FlowGraph}}** jako `usescript`) a stiskněte **{{EntOpenScript}}**.",
    "**Write the talk.** Sounds on `CHAN_VOICE`, a `SET_LOOK_TARGET` at the player, perhaps a short camera. The figure says its lines every time the player presses use.":
        "**Napište rozhovor.** Zvuky na `CHAN_VOICE`, `SET_LOOK_TARGET` na hráče, možná krátká kamera. Postava řekne své repliky pokaždé, když hráč stiskne použití.",
    "The spawn script makes the figure usable.":
        "Spawn skript udělá postavu použitelnou.",
    "**Only once?** Set `SET_PLAYER_USABLE` to `false` at the end of the usescript - or use a variable ([[k-variablen]]).":
        "**Jen jednou?** Na konci usescriptu nastavte `SET_PLAYER_USABLE` na `false` - nebo použijte proměnnou ([[k-variablen]]).",
    "Let several scripts work together":
        "Nechat několik skriptů spolupracovat",
    "run starts a script, signal and waitsignal hand over - and the split view shows both at once.":
        "run spustí skript, signal a waitsignal předávají - a rozdělené zobrazení ukáže oba najednou.",
    "**Split the work.** Long scenes are easier in parts: the intro starts the next part with `run ( \"taspir_tut/talk\" )` - the name as under `scripts/`, without ending.":
        "**Rozdělte práci.** Dlouhé scény jsou snazší po částech: úvod spustí další část pomocí `run ( \"taspir_tut/talk\" )` - název jako pod `scripts/`, bez přípony.",
    "**Hand over.** Where the intro must wait for the other part, insert `waitsignal ( \"talk_done\" )`; at the end of the other script `signal ( \"talk_done\" )`.":
        "**Předání.** Kde musí úvod počkat na druhou část, vložte `waitsignal ( \"talk_done\" )`; na konec druhého skriptu `signal ( \"talk_done\" )`.",
    "**See both.** **{{MenuFile}} → {{SplitToggle}} → 2** shows the two scripts side by side; drag lines across to copy them.":
        "**Podívejte se na oba.** **{{MenuFile}} → {{SplitToggle}} → 2** ukáže oba skripty vedle sebe; přetažením řádků napříč je zkopírujete.",
    "Two scripts that hand over with signal and waitsignal.":
        "Dva skripty, které si předávají pomocí signal a waitsignal.",
    "**Keep the overview.** The **{{FlowGraph}}** view draws `run` connections between the scripts of the mission.":
        "**Udržte přehled.** Zobrazení **{{FlowGraph}}** kreslí spojení `run` mezi skripty mise.",
    "Fix what Save mission reports":
        "Opravit, co hlásí Uložit misi",
    "The checks of Save mission - what each message means, which ones stop the packing, and how to fix them.":
        "Kontroly Uložit misi - co která zpráva znamená, které zastaví balení a jak je opravit.",
    "Missions · six steps":
        "Mise · šest kroků",
    "**Open the check.** **{{MissionSave}}** lists every file it will pack and, below, what it found. Errors in red stop the packing until they are fixed; warnings in orange let you pack anyway - but read them.":
        "**Otevřete kontrolu.** **{{MissionSave}}** vypíše každý soubor, který zabalí, a pod tím, co našla. Chyby červeně zastaví balení, dokud se neopraví; varování oranžově vás nechají zabalit i tak - ale přečtěte si je.",
    "Save mission with two missing scripts and a target of the original map.":
        "Uložit misi se dvěma chybějícími skripty a cílem původní mapy.",
    "**S001 (warning) - a script is missing.** An entity names a script that does not exist yet; in the game nothing would happen. Select the entity and press **{{EntOpenScript}}** next to the key: the script is created.":
        "**S001 (varování) - chybí skript.** Entita jmenuje skript, který zatím neexistuje; ve hře by se nic nestalo. Vyberte entitu a stiskněte **{{EntOpenScript}}** vedle klíče: skript se vytvoří.",
    "**S002 (error) - a script does not compile.** Open it and press **{{Compile}}**: the wrong line is selected ([[k-uebersetzen]]).":
        "**S002 (chyba) - skript nelze zkompilovat.** Otevřete ho a stiskněte **{{Compile}}**: špatný řádek se vybere ([[k-uebersetzen]]).",
    "**S003 (warning) - no player start.** The mission cannot start. Place one with the tile **{{EntKindStart}}**.":
        "**S003 (varování) - chybí start hráče.** Mise nemůže začít. Umístěte ho dlaždicí **{{EntKindStart}}**.",
    "**S005 (warning) - a target points to nothing.** A `target` names an entity that does not exist - often a leftover of the original map, like the one in the picture.":
        "**S005 (varování) - cíl na nic neukazuje.** `target` jmenuje entitu, která neexistuje - často zbytek původní mapy, jako ten na obrázku.",
    "**S004 and S006.** A missing `ENDMARKER` is added for you (warning); an invalid mission name is an error - letters, digits and `_` only.":
        "**S004 a S006.** Chybějící `ENDMARKER` se přidá za vás (varování); neplatný název mise je chyba - jen písmena, číslice a `_`.",
    "Open a compiled script of the game":
        "Otevřít zkompilovaný skript hry",
    "The game only ships .ibi files. behaved turns them back into a script you can read and change.":
        "Hra dodává jen soubory .ibi. behaved je převede zpět na skript, který můžete číst a měnit.",
    "**Find it.** **{{MenuFile}} → {{Pk3Browse}}**, choose **{{Pk3Scripts}}** and type part of the name into **{{Pk3Filter}}**, for example `taspir1/intro`.":
        "**Najděte ho.** **{{MenuFile}} → {{Pk3Browse}}**, zvolte **{{Pk3Scripts}}** a do **{{Pk3Filter}}** napište část názvu, například `taspir1/intro`.",
    "The files in the game's archives.":
        "Soubory v archivech hry.",
    "**Open it.** Double-click the entry: the `.ibi` opens as a script in the current tab - commands, values and blocks, exactly as Raven wrote them. Open an empty tab first (**+**) if you want to keep the script you are working on; with unsaved changes behaved asks before.":
        "**Otevřete ho.** Dvakrát klikněte na položku: `.ibi` se otevře jako skript v aktuální kartě - příkazy, hodnoty a bloky přesně tak, jak je Raven napsal. Chcete-li si ponechat skript, na kterém pracujete, nejdřív otevřete prázdnou kartu (**+**); při neuložených změnách se behaved předem zeptá.",
    "A compiled script of the game, opened as text.":
        "Zkompilovaný skript hry otevřený jako text.",
    "**From disk.** **{{MenuFile}} → {{FileOpen}}** opens `.ibi` files from a folder the same way.":
        "**Z disku.** **{{MenuFile}} → {{FileOpen}}** stejně otevře soubory `.ibi` ze složky.",
    "**Change it.** A script from an archive has no file yet - the first save asks where to put it. To ship a changed version of a game scene, see [[k-szene-aendern]].":
        "**Změňte ho.** Skript z archivu zatím nemá soubor - první uložení se zeptá, kam ho dát. Jak dodat změněnou verzi scény hry, viz [[k-szene-aendern]].",
    "Find files in the game archives":
        "Najít soubory v archivech hry",
    "Scripts, maps and models of the game and every mod, searchable in one window.":
        "Skripty, mapy a modely hry a každého modu, prohledávatelné v jednom okně.",
    "**Open the browser.** **{{MenuFile}} → {{Pk3Browse}}**. It lists the files of every game folder you added.":
        "**Otevřete prohlížeč.** **{{MenuFile}} → {{Pk3Browse}}**. Vypíše soubory každé složky hry, kterou jste přidali.",
    "The Files in .pk3 window.":
        "Okno Soubory v .pk3.",
    "**Choose the kind.** **{{Pk3Scripts}}**, **{{Pk3Maps}}** or **{{Pk3Models}}**. The **{{Pk3Archive}}** list narrows it to one `.pk3`.":
        "**Zvolte druh.** **{{Pk3Scripts}}**, **{{Pk3Maps}}** nebo **{{Pk3Models}}**. Seznam **{{Pk3Archive}}** ho zúží na jeden `.pk3`.",
    "**Search.** Type into **{{Pk3Filter}}** - part of a name is enough.":
        "**Hledání.** Pište do **{{Pk3Filter}}** - stačí část názvu.",
    "**Open.** Double-click: a script replaces the one in the current tab (behaved asks first if it has unsaved changes), a map loads, a model opens with its skin in the **{{ViewModel}}** tab.":
        "**Otevření.** Dvojklik: skript nahradí ten v aktuální kartě (behaved se nejdřív zeptá, má-li neuložené změny), mapa se načte, model se otevře se svým skinem na kartě **{{ViewModel}}**.",
    "See what you have changed":
        "Zobrazit, co jste změnili",
    "The change margin, going back to the original of one command, and finding identical commands.":
        "Okraj změn, návrat k originálu jednoho příkazu a hledání stejných příkazů.",
    "**The margin.** Change a value: a coloured bar appears left of the line - orange for changed and not saved, green for changed and saved, blue for back to the original. **{{MenuView}} → {{ViewChangeHistory}}** switches it on or off.":
        "**Okraj.** Změňte hodnotu: vlevo od řádku se objeví barevný pruh - oranžový pro změněno a neuloženo, zelený pro změněno a uloženo, modrý pro zpět v původním stavu. **{{MenuView}} → {{ViewChangeHistory}}** ho zapíná a vypíná.",
    "Changed lines in the margin.":
        "Změněné řádky v okraji.",
    "**What was it before?** Hover over the bar: the original line is shown.":
        "**Co tam bylo předtím?** Najeďte myší na pruh: ukáže se původní řádek.",
    "**Back to the original.** Right-click the line → **{{ActRevertOriginal}}**: only this command goes back, everything else stays.":
        "**Zpět na originál.** Pravé tlačítko na řádku → **{{ActRevertOriginal}}**: vrátí se jen tento příkaz, vše ostatní zůstane.",
    "**Identical commands.** **{{MenuView}} → {{ViewHighlightSame}}** marks every command that looks exactly like the selected one - handy for a camera that is used several times.":
        "**Stejné příkazy.** **{{MenuView}} → {{ViewHighlightSame}}** označí každý příkaz, který vypadá přesně jako vybraný - hodí se pro kameru použitou několikrát.",
    "Get an old version back":
        "Získat zpět starou verzi",
    "Automatic copies on every save, your own backup, and restoring it.":
        "Automatické kopie při každém uložení, vlastní záloha a její obnovení.",
    "**Automatic copies.** Every save and every compile keeps a copy - the last ten, `backup1.txt` (newest) to `backup10.txt`. **{{MenuFile}} → {{FileBackupFolder}}** opens the folder.":
        "**Automatické kopie.** Každé uložení a každá kompilace uchová kopii - posledních deset, `backup1.txt` (nejnovější) až `backup10.txt`. **{{MenuFile}} → {{FileBackupFolder}}** otevře složku.",
    "The File menu.":
        "Nabídka Soubor.",
    "**Your own backup.** **{{FileBackup}}** (<kbd>Alt</kbd>+<kbd>B</kbd>) writes `<script>.bak` next to the script - before a big change.":
        "**Vlastní záloha.** **{{FileBackup}}** (<kbd>Alt</kbd>+<kbd>B</kbd>) zapíše `<script>.bak` vedle skriptu - před velkou změnou.",
    "**Restore it.** **{{FileRestore}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) goes back to the `.bak`, after a question.":
        "**Obnovení.** **{{FileRestore}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) se po dotazu vrátí k `.bak`.",
    "**Smaller steps.** For the last few changes the undo history is quicker: [[k-rueckgaengig]].":
        "**Menší kroky.** Pro posledních pár změn je rychlejší historie kroků zpět: [[k-rueckgaengig]].",
    "Understand the warnings":
        "Porozumět varováním",
    "The Messages window: values that are not in their list, unknown fields, notes while reading a file.":
        "Okno Zprávy: hodnoty, které nejsou ve svém seznamu, neznámá pole, poznámky při čtení souboru.",
    "**The counters.** The status line counts errors and notes. Click a counter to open the **{{MessagesTitle}}** window.":
        "**Počítadla.** Stavový řádek počítá chyby a poznámky. Kliknutím na počítadlo otevřete okno **{{MessagesTitle}}**.",
    "**Read them.** Each message names its code and the command. A misspelt value such as `SET_WALKNG` is reported as *not in SET_TYPES (kept unchanged)* - behaved keeps it, because mods bring their own values, but it is far more often a typo.":
        "**Přečtěte si je.** Každá zpráva uvádí svůj kód a příkaz. Chybně napsaná hodnota jako `SET_WALKNG` se hlásí jako *není v SET_TYPES (ponecháno beze změny)* - behaved ji ponechá, protože mody přinášejí vlastní hodnoty, ale mnohem častěji jde o překlep.",
    "V004: a value that is not in its list.":
        "V004: hodnota, která není ve svém seznamu.",
    "**Fix and check.** Correct the value in the editor - the message disappears. Messages never change the script by themselves.":
        "**Opravte a zkontrolujte.** Opravte hodnotu v editoru - zpráva zmizí. Zprávy nikdy samy skript nemění.",
    "Move many entities at once":
        "Posunout mnoho entit najednou",
    "Select with a frame, move and turn the group, copy it.":
        "Vyberte rámečkem, posuňte a otočte skupinu, zkopírujte ji.",
    "**Draw a frame.** In the **{{ViewMap}}** tab drag over empty space in the view: every entity inside is selected. <kbd>Ctrl</kbd> or <kbd>Shift</kbd> adds to it.":
        "**Nakreslete rámeček.** Na kartě **{{ViewMap}}** táhněte přes prázdné místo v pohledu: každá entita uvnitř se vybere. <kbd>Ctrl</kbd> nebo <kbd>Shift</kbd> přidávají.",
    "Several entities selected with a frame.":
        "Několik entit vybraných rámečkem.",
    "**Move the group.** Drag one of them, or use the arrows: all move together.":
        "**Posuňte skupinu.** Táhněte jednu z nich, nebo použijte šipky: posunou se všechny spolu.",
    "**Turn the group.** Switch the gizmo bar to **{{GizmoRotate}}** and drag the ring.":
        "**Otočte skupinu.** Přepněte lištu gizma na **{{GizmoRotate}}** a táhněte kruh.",
    "**Copy it.** <kbd>Alt</kbd>+drag copies the whole group - **{{EntDuplicate}}** in the inspector does the same.":
        "**Zkopírujte ji.** <kbd>Alt</kbd>+tažení zkopíruje celou skupinu - totéž udělá **{{EntDuplicate}}** v inspektoru.",
    "Remember viewpoints and camera paths":
        "Pamatovat si pohledy a dráhy kamery",
    "Number keys for views you need again - and only the camera path you are working on.":
        "Číselné klávesy pro pohledy, které budete znovu potřebovat - a jen ta dráha kamery, na které pracujete.",
    "**Store a view.** With the mouse over the map press <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd>; the status line confirms it. <kbd>1</kbd> … <kbd>9</kbd> jumps back. The views are kept per map.":
        "**Uložte pohled.** S myší nad mapou stiskněte <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd>; stavový řádek to potvrdí. <kbd>1</kbd> … <kbd>9</kbd> skočí zpět. Pohledy se pamatují pro každou mapu.",
    "**All paths or one.** In the camera panel, **{{CamPathAll}}** draws every camera move of the script, **{{CamPathActive}}** only the one around the playhead - clearer in long scenes.":
        "**Všechny dráhy, nebo jedna.** V panelu kamery kreslí **{{CamPathAll}}** každý pohyb kamery skriptu, **{{CamPathActive}}** jen ten kolem ukazatele přehrávání - v dlouhých scénách přehlednější.",
    "Every camera path of the scene.":
        "Každá dráha kamery scény.",
    "**Fly to a shot.** **{{CamFlyToShot}}** puts your view next to the selected shot.":
        "**Přeletět k záběru.** **{{CamFlyToShot}}** umístí váš pohled vedle vybraného záběru.",
    "A figure that appears on cue":
        "Postava, která se objeví na povel",
    "Spawner keys and spawnflags: a character that waits until the script calls it.":
        "Klíče a spawnflags spawneru: postava, která čeká, dokud ji skript nezavolá.",
    "**Two names.** A character spawner has `NPC_targetname` - the name of the figure, used by `affect` - and may have `targetname` - the name of the spawner itself.":
        "**Dva názvy.** Spawner postavy má `NPC_targetname` - jméno postavy, používané v `affect` - a může mít `targetname` - název samotného spawneru.",
    "**Wait for the cue.** Give the spawner a `targetname`, for example `rosh_spawner`. In the game it then spawns its figure only when it is used.":
        "**Čekat na povel.** Dejte spawneru `targetname`, například `rosh_spawner`. Ve hře pak spawne svou postavu, až když je použit.",
    "A spawner's keys and spawnflags.":
        "Klíče a spawnflags spawneru.",
    "**Call it.** In the script: `use ( \"rosh_spawner\" )` - from then on the figure exists and `affect ( \"rosh\" )` reaches it. (The preview shows the figure from the start.)":
        "**Zavolejte ji.** Ve skriptu: `use ( \"rosh_spawner\" )` - od té chvíle postava existuje a `affect ( \"rosh\" )` ji zastihne. (Náhled ukazuje postavu od začátku.)",
    "**Spawnflags.** The checkboxes under **{{EntFlags}}** set the class's options; hover over one for what it does.":
        "**Spawnflags.** Zaškrtávací políčka pod **{{EntFlags}}** nastavují volby třídy; najeďte myší na některé a uvidíte, co dělá.",
    "Load a separate .ent file":
        "Načíst samostatný soubor .ent",
    "Swap the entities of a map without touching the map - the way many mods add their own scenes.":
        "Vyměňte entity mapy, aniž byste se mapy dotkli - tak přidává vlastní scény mnoho modů.",
    "**Open it.** **{{MapMore}}** → **{{OpenEnt}}** loads an `.ent` file: its entities replace the ones of the map.":
        "**Otevřete ho.** **{{MapMore}}** → **{{OpenEnt}}** načte soubor `.ent`: jeho entity nahradí ty z mapy.",
    "The Maps & files menu.":
        "Nabídka Mapy a soubory.",
    "**Work as usual.** The entities appear in the list and the view; change them like any other.":
        "**Pracujte jako obvykle.** Entity se objeví v seznamu a v pohledu; měňte je jako jakékoli jiné.",
    "**Saved with the mission.** **{{MissionSave}}** writes the entities into `maps/<name>.ent` in the `.pk3` - the game loads it instead of the map's own.":
        "**Uloženo s misí.** **{{MissionSave}}** zapíše entity do `maps/<name>.ent` v `.pk3` - hra ho načte místo vlastního souboru mapy.",
    "Time a scene in the Tracks view":
        "Načasovat scénu v zobrazení Stopy",
    "The script over time, right in the script column: camera, figures and dialogue as rows.":
        "Skript v čase, přímo ve sloupci skriptu: kamera, postavy a dialog jako řádky.",
    "Timeline · four steps":
        "Časová osa · čtyři kroky",
    "**Switch to Tracks.** Above the script column choose **{{FlowTracks}}**: rows for **{{TracksCamera}}**, each figure, **{{TracksDialog}}** and **{{TracksScript}}**.":
        "**Přepněte na Stopy.** Nad sloupcem skriptu zvolte **{{FlowTracks}}**: řádky pro **{{TracksCamera}}**, každou postavu, **{{TracksDialog}}** a **{{TracksScript}}**.",
    "**Jump to a line.** Click a block: its line is selected.":
        "**Skok na řádek.** Klikněte na blok: jeho řádek se vybere.",
    "**Change a duration.** Drag the right edge of a block.":
        "**Změňte délku.** Táhněte pravý okraj bloku.",
    "**Move through time.** Drag in the ruler; **{{TracksPlay}}** plays.":
        "**Pohyb v čase.** Táhněte v pravítku; **{{TracksPlay}}** přehrává.",
    "The timeline in detail":
        "Časová osa podrobně",
    "Waits as blocks, quiet rows, frames instead of seconds, speed - and the warning for overlapping camera moves.":
        "Čekání jako bloky, tiché řádky, snímky místo sekund, rychlost - a varování před překrývajícími se pohyby kamery.",
    "Waits shown as blocks.":
        "Čekání zobrazená jako bloky.",
    "**Frames.** Click the time display to switch between seconds and frames.":
        "**Snímky.** Kliknutím na zobrazení času přepínáte mezi sekundami a snímky.",
    "**Speed.** The list next to the play button plays at 0.25x to 2x; sound only at 1x.":
        "**Rychlost.** Seznam vedle tlačítka přehrávání hraje od 0.25x do 2x; zvuk jen při 1x.",
    "**Overlap warning.** A warning sign on a camera block: it overlaps the next move. In the game the next one starts from where this one began, and the camera jumps - shorten one of them.":
        "**Varování před překryvem.** Výstražná značka na bloku kamery: překrývá další pohyb. Ve hře další začne tam, kde tento začal, a kamera skočí - jeden z nich zkraťte.",
    "Skins and skeletons of a model":
        "Skiny a kostry modelu",
    "Another skin, another skeleton, and the surfaces a skin switches on or off.":
        "Jiný skin, jiná kostra a povrchy, které skin zapíná nebo vypíná.",
    "**Switch skins.** In the **{{ViewModel}}** tab the **{{ModelSkin}}** list shows the skins that come with the model; **{{OpenSkin}}** loads another one.":
        "**Přepínání skinů.** Na kartě **{{ViewModel}}** ukazuje seznam **{{ModelSkin}}** skiny, které k modelu patří; **{{OpenSkin}}** načte jiný.",
    "The playback bar with the skin list.":
        "Lišta přehrávání se seznamem skinů.",
    "**Surfaces.** The counter of surfaces shows how many the skin draws; click it to list them all in the detail log.":
        "**Povrchy.** Počítadlo povrchů ukazuje, kolik jich skin kreslí; kliknutím je všechny vypíšete do podrobného protokolu.",
    "**Caps.** **{{ModelCaps}}** shows the `_off` surfaces that close the holes where limbs attach.":
        "**Záslepky.** **{{ModelCaps}}** ukáže povrchy `_off`, které zakrývají otvory, kde se připojují končetiny.",
    "**Another skeleton.** **{{ModelLoadGla}}** loads a different `.gla` with its animations.":
        "**Jiná kostra.** **{{ModelLoadGla}}** načte jiný `.gla` s jeho animacemi.",
    "This tutorial also runs inside the program: **{{MenuHelp}} → {{TourMenu}}** - or ask the helper (<kbd>F1</kbd>).":
        "Tento tutoriál běží i přímo v programu: **{{MenuHelp}} → {{TourMenu}}** - nebo se zeptejte pomocníka (<kbd>F1</kbd>).",
    "Set up the game folder and the preferences":
        "Nastavit složku hry a předvolby",
    "Tell the program where Jedi Academy is installed, check what it found, and choose the rules of the preview.":
        "Řekněte programu, kde je nainstalováno Jedi Academy, zkontrolujte, co našel, a zvolte pravidla náhledu.",
    "**Open the paths.** **{{MenuEdit}} → {{PathsTitle}}**. In the map editor switch to **{{ViewEvents}}** first - the menus **{{MenuFile}}**, **{{MenuEdit}}** and **{{MenuView}}** are there.":
        "**Otevřete cesty.** **{{MenuEdit}} → {{PathsTitle}}**. V editoru map nejdřív přepněte na **{{ViewEvents}}** - tam jsou nabídky **{{MenuFile}}**, **{{MenuEdit}}** a **{{MenuView}}**.",
    "**Add the game folder.** Press **{{Pk3AddPath}}** and choose the `base` folder of Jedi Academy - the one with `assets0.pk3`. For Movie Duels add its `MD` folder too.":
        "**Přidejte složku hry.** Stiskněte **{{Pk3AddPath}}** a zvolte složku `base` hry Jedi Academy - tu s `assets0.pk3`. Pro Movie Duels přidejte i jeho složku `MD`.",
    "The tutorial in the program: the button is highlighted.":
        "Tutoriál v programu: tlačítko je zvýrazněné.",
    "**Check the list.** Every folder shows how many archives it found. If it says none, you picked the wrong folder - remove it and try again. Then close the window.":
        "**Zkontrolujte seznam.** Každá složka ukazuje, kolik archivů našla. Pokud žádný, vybrali jste špatnou složku - odeberte ji a zkuste to znovu. Pak okno zavřete.",
    "**Open the preferences.** **{{MenuEdit}} → {{AppPrefs}}**: sound levels, the game rules of the preview, difficulty and how the program behaves.":
        "**Otevřete předvolby.** **{{MenuEdit}} → {{AppPrefs}}**: hlasitost zvuku, pravidla hry pro náhled, obtížnost a chování programu.",
    "**Choose the rules.** **{{PrefsRegelwerk}}**: *auto* follows your game folder; OpenJK or Movie Duels make the preview behave exactly like that game - walking, doors, timing.":
        "**Zvolte pravidla.** **{{PrefsRegelwerk}}**: *automaticky* se řídí vaší složkou hry; OpenJK nebo Movie Duels nechají náhled chovat se přesně jako tato hra - chůze, dveře, načasování.",
    "The program only reads the game folder. It never writes into it - your own files go into your mission or mod folder.":
        "Program složku hry jen čte. Nikdy do ní nezapisuje - vaše vlastní soubory jdou do složky vaší mise nebo modu.",
    "Check for updates":
        "Vyhledat aktualizace",
    "Look for a new version, read what changed, and decide whether the program checks at start.":
        "Vyhledejte novou verzi, přečtěte si, co se změnilo, a rozhodněte, zda program kontroluje při spuštění.",
    "Settings · four steps":
        "Nastavení · čtyři kroky",
    "**Look for a new version.** **{{MenuHelp}} → {{UpdMenu}}**. The program asks GitHub for the newest release - only now, because you chose it.":
        "**Vyhledejte novou verzi.** **{{MenuHelp}} → {{UpdMenu}}**. Program se zeptá GitHubu na nejnovější vydání - jen teď, protože jste to zvolili.",
    "The tutorial points at the Help menu.":
        "Tutoriál ukazuje na nabídku Nápověda.",
    "**Read the answer.** The window says that you have the newest version - or shows the new one with its notes and **{{UpdInstall}}**. Without a connection it says so; **{{UpdRetry}}** tries again.":
        "**Přečtěte si odpověď.** Okno řekne, že máte nejnovější verzi - nebo ukáže novou s poznámkami a **{{UpdInstall}}**. Bez připojení to oznámí; **{{UpdRetry}}** to zkusí znovu.",
    "**At start or not.** **{{MenuHelp}} → {{UpdAutoCheck}}** switches the check at start on or off. Nothing else is ever sent.":
        "**Při spuštění, nebo ne.** **{{MenuHelp}} → {{UpdAutoCheck}}** zapíná nebo vypíná kontrolu při spuštění. Nic jiného se nikdy neposílá.",
    "**What changed?** **{{ChangelogMenu}}** in the window (or in **{{MenuHelp}}**) lists the changes of every version.":
        "**Co se změnilo?** **{{ChangelogMenu}}** v okně (nebo v **{{MenuHelp}}**) vypíše změny každé verze.",
    "A door and a trigger in the map editor":
        "Dveře a trigger v editoru map",
    "Turn a brush into a door, tell it which way to open, and let a trigger open it when the player walks in.":
        "Udělejte z brushe dveře, řekněte jim, kam se otevírají, a nechte trigger otevřít je, když hráč vejde.",
    "Map editor · seven steps":
        "Editor map · sedm kroků",
    "**Open the map editor.** **{{ViewMapEditor}}** in the mode bar. Open your map from [[tut-karte]] - or any map with a room.":
        "**Otevřete editor map.** **{{ViewMapEditor}}** v liště režimů. Otevřete svou mapu z [[tut-karte]] - nebo libovolnou mapu s místností.",
    "**Draw the door.** Drag a thin brush with the LEFT mouse button in a 2D view, where the door should be - for example 16 units thick.":
        "**Nakreslete dveře.** Táhněte LEVÝM tlačítkem myši ve 2D pohledu tenký brush tam, kde mají být dveře - například 16 jednotek silný.",
    "**Make it a door.** Keep it selected, right-click in the 2D view and choose **func → func_door**. The brush is now an entity that moves.":
        "**Udělejte z něj dveře.** Nechte ho vybraný, klikněte pravým tlačítkem ve 2D pohledu a zvolte **func → func_door**. Brush je teď entita, která se pohybuje.",
    "**Which way it opens.** Press <kbd>N</kbd> for the entity inspector. Set the key `angle` to `90` (it slides north; `-1` is up, `-2` down) and press <kbd>Enter</kbd>.":
        "**Kam se otevírají.** Stiskněte <kbd>N</kbd> pro inspektor entit. Nastavte klíč `angle` na `90` (vysune se na sever; `-1` je nahoru, `-2` dolů) a stiskněte <kbd>Enter</kbd>.",
    "**Draw the trigger.** Draw a second brush in front of the door, right-click and choose **trigger → trigger_multiple**. Triggers are invisible in the game.":
        "**Nakreslete trigger.** Nakreslete před dveřmi druhý brush, klikněte pravým tlačítkem a zvolte **trigger → trigger_multiple**. Triggery jsou ve hře neviditelné.",
    "**Connect them.** Select the trigger first, then the door (<kbd>Shift</kbd>+click), and press <kbd>Ctrl</kbd>+<kbd>K</kbd> - **{{KeLEntity}} → {{KeLConnectEntities}}**. The trigger's `target` now names the door's `targetname`.":
        "**Spojte je.** Vyberte nejdřív trigger, pak dveře (<kbd>Shift</kbd>+kliknutí) a stiskněte <kbd>Ctrl</kbd>+<kbd>K</kbd> - **{{KeLEntity}} → {{KeLConnectEntities}}**. `target` triggeru teď jmenuje `targetname` dveří.",
    "**Try it.** Build the map and walk through it as in [[tut-karte]]: the door opens when you step into the trigger.":
        "**Vyzkoušejte to.** Sestavte mapu a projděte ji jako v [[tut-karte]]: dveře se otevřou, když vstoupíte do triggeru.",
    "A door without a name opens by itself when the player touches it. Once a trigger names it, it waits for the trigger.":
        "Dveře bez názvu se otevřou samy, když se jich hráč dotkne. Jakmile je trigger jmenuje, čekají na trigger.",
    "Change a game menu":
        "Změnit menu hry",
    "Open a menu of the game, move a button, try it like in the game and pack it for your mod.":
        "Otevřete menu hry, posuňte tlačítko, vyzkoušejte ho jako ve hře a zabalte ho pro svůj mod.",
    "Menu editor · five steps":
        "Editor menu · pět kroků",
    "**Open the menu editor.** **{{ViewMenuEditor}}** in the mode bar.":
        "**Otevřete editor menu.** **{{ViewMenuEditor}}** v liště režimů.",
    "**Open a menu of the game.** **{{MeAusSpiel}}** and pick one, for example `ui/jamp/main.menu`. It is read from the game files and drawn like the game - the game folder is not changed.":
        "**Otevřete menu hry.** **{{MeAusSpiel}}** a vyberte jedno, například `ui/jamp/main.menu`. Načte se ze souborů hry a kreslí se jako ve hře - složka hry se nemění.",
    "**Move an element.** Click a button on the canvas and drag it - or nudge it with the arrow keys. Its `rect` in the code below changes with it.":
        "**Posuňte prvek.** Klikněte na tlačítko na plátně a táhněte ho - nebo ho postrčte šipkami. Jeho `rect` v kódu níže se mění s ním.",
    "**Try it like in the game.** **{{MeBedienen}}**: clicks now press buttons and change cvars - only in the preview, never in your game. **{{MeBearbeiten}}** brings you back.":
        "**Vyzkoušejte to jako ve hře.** **{{MeBedienen}}**: kliknutí teď mačkají tlačítka a mění cvary - jen v náhledu, nikdy ve vaší hře. **{{MeBearbeiten}}** vás vrátí zpět.",
    "Step 4 of the tutorial in the menu editor.":
        "Krok 4 tutoriálu v editoru menu.",
    "**Pack it.** **{{MeExportPk3}}** writes the changed menu into a .pk3 for your mod folder, for example `zz_menu.pk3`. The game folder itself is never written.":
        "**Zabalte ho.** **{{MeExportPk3}}** zapíše změněné menu do .pk3 pro složku vašeho modu, například `zz_menu.pk3`. Do samotné složky hry se nikdy nezapisuje.",
    "More in [[menue_editor]].":
        "Více v [[menue_editor]].",
    "The helper and the tutorials":
        "Pomocník a tutoriály",
    "Press F1, write what you want to do, and the program shows you the way. Interactive tutorials wait for your real clicks.":
        "Stiskněte F1, napište, co chcete udělat, a program vám ukáže cestu. Interaktivní tutoriály čekají na vaše skutečná kliknutí.",
    "The helper: what do you want to do?":
        "Pomocník: co chcete udělat?",
    "Press <kbd>F1</kbd> - or **{{MenuHelp}} → {{HlfMenu}}**. In the map editor <kbd>F1</kbd> opens the manual as in NetRadiant; there use **{{KeLHelp}} → {{HlfMenu}}**.":
        "Stiskněte <kbd>F1</kbd> - nebo **{{MenuHelp}} → {{HlfMenu}}**. V editoru map otevře <kbd>F1</kbd> příručku jako v NetRadiantu; tam použijte **{{KeLHelp}} → {{HlfMenu}}**.",
    "Write what you want to do, in your own words. For example \"add a light\" or \"make an NPC walk\". German, English, Chinese, Japanese, Spanish and Czech all work.":
        "Napište vlastními slovy, co chcete udělat. Například \"přidat světlo\" nebo \"nechat NPC chodit\". Funguje čeština, angličtina, němčina, čínština, japonština i španělština.",
    "Pick a result. **{{HlfShowMe}}** starts the tutorial at the right step, or highlights the button or menu entry. Nothing is changed.":
        "Vyberte výsledek. **{{HlfShowMe}}** spustí tutoriál ve správném kroku, nebo zvýrazní tlačítko či položku nabídky. Nic se nezmění.",
    "**{{HlfGuide}}** opens the matching page of this guide.":
        "**{{HlfGuide}}** otevře odpovídající stránku této příručky.",
    "The helper: a question in your own words, the best matches with \"Show me\".":
        "Pomocník: otázka vlastními slovy, nejlepší shody s \"Ukázat\".",
    "Show me: the program darkens the screen and highlights the menu entry.":
        "Ukázat: program ztmaví obrazovku a zvýrazní položku nabídky.",
    "The helper works without the internet. It knows every tutorial step, every page of this guide, every menu entry and every setting of the program - also of the map editor, with the names from NetRadiant.":
        "Pomocník funguje bez internetu. Zná každý krok tutoriálu, každou stránku této příručky, každou položku nabídky a každé nastavení programu - i editoru map, s názvy z NetRadiantu.",
    "Interactive tutorials":
        "Interaktivní tutoriály",
    "A tutorial darkens the screen except for the control you need, and explains the next step in a small window.":
        "Tutoriál ztmaví obrazovku kromě ovládacího prvku, který potřebujete, a v malém okně vysvětlí další krok.",
    "It waits until you have really done the step - for example until the brush is there - and then moves on by itself.":
        "Čeká, dokud krok skutečně neuděláte - například dokud tam není brush - a pak sám pokračuje.",
    "If something else happens, it tells you kindly what to do instead - for example \"That was a light - choose info_player_start\".":
        "Stane-li se něco jiného, laskavě vám řekne, co udělat místo toho - například \"To bylo světlo - zvolte info_player_start\".",
    "You can stop at any time; the program remembers how far you got.":
        "Můžete kdykoli přestat; program si pamatuje, kam jste se dostali.",
    "Tutorials in the program: the five script tutorials, [[tut-karte]] (15 steps: room, texture, light, player start, build, walk through, pack as .pk3) [[k-kamera-ansicht]] for a cutscene and [[k-modell]].":
        "Tutoriály v programu: pět tutoriálů ke skriptům, [[tut-karte]] (15 kroků: místnost, textura, světlo, start hráče, sestavení, procházka, zabalení jako .pk3) [[k-kamera-ansicht]] pro animační sekvenci a [[k-modell]].",
    "On the very first start the program asks **{{EsTitle}}** - pick a goal, or just look around.":
        "Při úplně prvním spuštění se program zeptá **{{EsTitle}}** - vyberte cíl, nebo se jen rozhlédněte.",
    "Ask Claude (optional)":
        "Zeptat se Clauda (volitelné)",
    "If you want, you can connect Claude, an AI from Anthropic. It is off until you switch it on. You need your own Anthropic API key; the questions then go to Anthropic and cost money on your account.":
        "Chcete-li, můžete připojit Clauda, AI od Anthropic. Je vypnutý, dokud ho nezapnete. Potřebujete vlastní klíč API od Anthropic; otázky pak jdou společnosti Anthropic a stojí peníze na vašem účtu.",
    "Open the helper (<kbd>F1</kbd>) and unfold **{{KiSection}}**.":
        "Otevřete pomocníka (<kbd>F1</kbd>) a rozbalte **{{KiSection}}**.",
    "Tick **{{KiEnable}}**, paste your key and press **{{KiKeySave}}**. The key is stored encrypted on this PC.":
        "Zaškrtněte **{{KiEnable}}**, vložte svůj klíč a stiskněte **{{KiKeySave}}**. Klíč se na tomto PC uloží šifrovaně.",
    "Write your question in the helper and press **{{KiAsk}}**. Claude answers and can start a tutorial or highlight a control for you.":
        "Napište otázku do pomocníka a stiskněte **{{KiAsk}}**. Claude odpoví a může za vás spustit tutoriál nebo zvýraznit ovládací prvek.",
    "Claude can also do things for you with the program's own commands, for example hollow out a brush. You always see the list first and decide: **{{KiRun}}** or **{{KiReject}}**. One job is one undo step - <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes all of it back.":
        "Claude za vás může i něco udělat vlastními příkazy programu, například vydlabat brush. Vždy nejdřív uvidíte seznam a rozhodnete: **{{KiRun}}** nebo **{{KiReject}}**. Jedna úloha je jeden krok zpět - <kbd>Ctrl</kbd>+<kbd>Z</kbd> vrátí všechno.",
    "Before every change the AI asks: Run or Reject. Top right: AI connected, with Disconnect.":
        "Před každou změnou se AI zeptá: Spustit, nebo Odmítnout. Vpravo nahoře: AI připojena, s Odpojit.",
    "Connect your own AI app (MCP)":
        "Připojit vlastní aplikaci AI (MCP)",
    "MCP (Model Context Protocol) is a common way for AI apps to use programs. Claude Desktop, Claude Code, Cursor and VS Code can look at JK Studio, show you things and - after you confirm - run program commands.":
        "MCP (Model Context Protocol) je běžný způsob, jak aplikace AI používají programy. Claude Desktop, Claude Code, Cursor a VS Code mohou prohlížet JK Studio, ukazovat vám věci a - po vašem potvrzení - spouštět příkazy programu.",
    "In the helper, unfold **{{McpSection}}**.":
        "V pomocníkovi rozbalte **{{McpSection}}**.",
    "Press the copy button for your app. It puts a ready setup on the clipboard: the app starts JK Studio itself with `--mcp`. This needs no network and is the safest way.":
        "Stiskněte tlačítko kopírování pro svou aplikaci. Vloží do schránky hotové nastavení: aplikace spustí JK Studio sama s `--mcp`. Nepotřebuje to síť a je to nejbezpečnější způsob.",
    "Paste it into the settings of the app (Claude Desktop: claude_desktop_config.json, Cursor: .cursor/mcp.json, VS Code: .vscode/mcp.json; Claude Code: run the command) and restart the app.":
        "Vložte ho do nastavení aplikace (Claude Desktop: claude_desktop_config.json, Cursor: .cursor/mcp.json, VS Code: .vscode/mcp.json; Claude Code: spusťte příkaz) a aplikaci restartujte.",
    "Connect your own AI app: ready setups to copy for Claude Desktop, Claude Code, Cursor and VS Code.":
        "Připojit vlastní aplikaci AI: hotová nastavení ke zkopírování pro Claude Desktop, Claude Code, Cursor a VS Code.",
    "If JK Studio is already running, you can instead tick **{{McpHttp}}**. Only programs on this PC can reach it, and only with the secret token from the copy button. **{{McpNewToken}}** makes a new token at any time.":
        "Pokud JK Studio už běží, můžete místo toho zaškrtnout **{{McpHttp}}**. Dosáhnou na něj jen programy na tomto PC, a jen s tajným tokenem z tlačítka kopírování. **{{McpNewToken}}** kdykoli vytvoří nový token.",
    "Is this safe?":
        "Je to bezpečné?",
    "**Off by default.** Nothing connects until you switch it on yourself. While an AI is connected, **{{KiConnected}}** shows at the top right, with **{{KiDisconnect}}**.":
        "**Ve výchozím stavu vypnuto.** Nic se nepřipojí, dokud to sami nezapnete. Je-li AI připojena, vpravo nahoře se ukazuje **{{KiConnected}}** s **{{KiDisconnect}}**.",
    "**The AI cannot touch your files.** It has no access to files, no command line and no internet through JK Studio. It can only read what the program shows, show you things, and use the program's own commands.":
        "**AI se nemůže dotknout vašich souborů.** Přes JK Studio nemá přístup k souborům, příkazový řádek ani internet. Může jen číst, co program ukazuje, ukazovat vám věci a používat vlastní příkazy programu.",
    "**You decide.** Every change is shown first and only runs after **{{KiRun}}**. Saving, overwriting and deleting files always ask, even if you allowed changes without asking. Every job is one undo step.":
        "**Rozhodujete vy.** Každá změna se nejdřív ukáže a proběhne až po **{{KiRun}}**. Ukládání, přepisování a mazání souborů se vždy zeptá, i když jste povolili změny bez dotazu. Každá úloha je jeden krok zpět.",
    "**Tricks in files do not work.** Text inside a map or script (for example \"delete all files\") is passed to the AI as data, never as an order - and there is no command it could use for that anyway.":
        "**Triky v souborech nefungují.** Text uvnitř mapy nebo skriptu (například \"smaž všechny soubory\") se AI předá jako data, nikdy jako příkaz - a stejně neexistuje příkaz, který by k tomu mohla použít.",
    "**Websites cannot reach it.** The local connection only answers this PC, checks where a request comes from and needs the secret token.":
        "**Webové stránky na to nedosáhnou.** Místní připojení odpovídá jen tomuto PC, kontroluje, odkud požadavek přichází, a potřebuje tajný token.",
    "**Your key stays yours.** The API key and the token are stored encrypted for your Windows user. The key only goes to api.anthropic.com over an encrypted connection, never into logs.":
        "**Váš klíč zůstane váš.** Klíč API a token se ukládají šifrovaně pro vašeho uživatele Windows. Klíč jde jen na api.anthropic.com šifrovaným připojením, nikdy do protokolů.",
    "**You can see everything.** **{{KiLog}}** lists every action of the AI. JK Studio sends no usage data anywhere.":
        "**Vidíte všechno.** **{{KiLog}}** vypíše každou akci AI. JK Studio nikam neposílá žádná data o používání.",
    "**Switch it off:** press **{{KiDisconnect}}**, untick **{{KiEnable}}** and **{{McpHttp}}**, or delete the key with **{{KiKeyDelete}}**.":
        "**Vypnutí:** stiskněte **{{KiDisconnect}}**, zrušte zaškrtnutí **{{KiEnable}}** a **{{McpHttp}}**, nebo smažte klíč pomocí **{{KiKeyDelete}}**.",
    "Finding things faster":
        "Najít věci rychleji",
    "**Where you work counts.** In the map editor, results of the map editor come first; in the menu editor, those of the menu editor. \"Settings\" in the map editor finds its own preferences.":
        "**Záleží na tom, kde pracujete.** V editoru map jsou první výsledky editoru map; v editoru menu ty z editoru menu. \"Nastavení\" v editoru map najde jeho vlastní předvolby.",
    "**Your own words, any form.** \"runs\", \"ran\", \"running\" or \"rennt\", \"läuft\" find the same. Small typos are fine; with no match at all the helper offers **Did you mean ...?**":
        "**Vlastní slova, libovolný tvar.** \"běží\", \"běžel\", \"běhání\" nebo \"runs\", \"running\" najdou totéž. Drobné překlepy nevadí; když není žádná shoda, pomocník nabídne **Mysleli jste ...?**",
    "**It also knows what is inside.** The buttons of the toolbars, the pages of the map editor's preferences, the menu editor and what a dialog contains are all part of the search.":
        "**Ví i to, co je uvnitř.** Tlačítka panelů nástrojů, stránky předvoleb editoru map, editor menu a obsah dialogů - to vše je součástí hledání.",
    "**Show me, step by step.** If the control is not visible yet, **{{HlfShowMe}}** first points at what opens it - for a setting of the main program searched in the map editor: first **{{ViewEvents}}**, then **{{MenuEdit}} → {{AppPrefs}}**, then the field itself.":
        "**Ukázat, krok za krokem.** Pokud ovládací prvek ještě není vidět, **{{HlfShowMe}}** nejdřív ukáže na to, co ho otevře - u nastavení hlavního programu hledaného v editoru map: nejdřív **{{ViewEvents}}**, pak **{{MenuEdit}} → {{AppPrefs}}**, pak samotné pole.",
    "Ask follow-up questions: **{{KiAskMore}}** keeps the conversation, so Claude knows what you asked before. The answer appears while it is written; **{{KiStop}}** stops it at once - nothing of that question stays. **{{KiHistoryClear}}** deletes the conversation from your computer.":
        "Pokládejte doplňující otázky: **{{KiAskMore}}** zachová konverzaci, takže Claude ví, na co jste se ptali dřív. Odpověď se objevuje, zatímco se píše; **{{KiStop}}** ji okamžitě zastaví - z této otázky nic nezůstane. **{{KiHistoryClear}}** smaže konverzaci z vašeho počítače.",
    "In the menu editor Claude can also select, move, duplicate or delete elements and switch to **{{MeBedienen}}** - again only after **{{KiRun}}**, and the whole job is one undo step. Opening, saving or exporting a menu is not possible for the AI.":
        "V editoru menu může Claude také vybírat, posouvat, duplikovat nebo mazat prvky a přepnout na **{{MeBedienen}}** - opět jen po **{{KiRun}}** a celá úloha je jeden krok zpět. Otevírat, ukládat nebo exportovat menu AI nemůže.",
    "**The conversation stays on your PC.** It is stored encrypted for your Windows user and deleted with **{{KiHistoryClear}}**. After each request the program also wipes the key from its memory.":
        "**Konverzace zůstává na vašem PC.** Ukládá se šifrovaně pro vašeho uživatele Windows a smaže se pomocí **{{KiHistoryClear}}**. Po každém požadavku program také vymaže klíč ze své paměti.",
    "Scripts":
        "Skripty",
    "Camera and timeline":
        "Kamera a časová osa",
    "Figures":
        "Postavy",
    "Map and entities":
        "Mapa a entity",
    "Animation":
        "Animace",
    "Missions, models and settings":
        "Mise, modely a nastavení",
    "Map editor and menu editor":
        "Editor map a editor menu",
    "Welcome":
        "Vítejte",
    "JK Studio - mod tools for Jedi Knight: Jedi Academy and Jedi Outcast. Its BehavEd part writes the scripts that run cutscenes and missions, the preview shows them on the map while you write them, and the map editor builds the maps.":
        "JK Studio - nástroje pro mody do Jedi Knight: Jedi Academy a Jedi Outcast. Jeho část BehavEd píše skripty, které řídí animační sekvence a mise, náhled je ukazuje v mapě, zatímco je píšete, a editor map staví mapy.",
    "A mission of your own on taspir1 in JK Studio: the script on the right, the map in the middle, the timeline below. Every part is explained on [[fenster]].":
        "Vlastní mise na taspir1 v JK Studiu: skript vpravo, mapa uprostřed, časová osa dole. Každá část je vysvětlena na [[fenster]].",
    "**{{MenuHelp}} → {{HelpAbout}}**: name and version, the three parts BehavEd (scripts), preview and map editor, and what the BehavEd part was measured against.":
        "**{{MenuHelp}} → {{HelpAbout}}**: název a verze, tři části BehavEd (skripty), náhled a editor map, a proti čemu byla část BehavEd změřena.",
    "JK Studio is a free fan project. It is not affiliated with Lucasfilm, Disney or Raven Software. Up to rc605 it was called BehavEd-Remake; it updates itself and keeps your settings.":
        "JK Studio je bezplatný fanouškovský projekt. Není spojen s Lucasfilm, Disney ani Raven Software. Do rc605 se jmenoval BehavEd-Remake; aktualizuje se sám a zachová vaše nastavení.",
    "The game runs its scenes with **ICARUS** scripts: lists of commands such as \"walk to this spot\", \"play this sound\", \"move the camera there\". Raven's original BehavEd let you build those lists from buttons. JK Studio does the same, file for file compatible, and adds what the original never had: the map in 3D, the figures walking their paths, the camera flying its shots, and a timeline that plays it all - before you ever start the game.":
        "Hra řídí své scény skripty **ICARUS**: seznamy příkazů jako \"jdi na toto místo\", \"přehraj tento zvuk\", \"posuň kameru tam\". Původní BehavEd od Raven umožňoval sestavovat tyto seznamy z tlačítek. JK Studio dělá totéž, soubor po souboru kompatibilně, a přidává, co originál nikdy neměl: mapu ve 3D, postavy chodící po svých drahách, kameru prolétající své záběry a časovou osu, která to vše přehraje - dřív, než vůbec spustíte hru.",
    "Where to start":
        "Kde začít",
    "New here? Read [[erste-schritte]] and [[fenster]] - ten minutes, and every later page makes sense.":
        "Jste tu noví? Přečtěte si [[erste-schritte]] a [[fenster]] - deset minut a každá další stránka bude dávat smysl.",
    "Want to make something right away? Pick one of the tutorials below.":
        "Chcete hned něco vytvořit? Vyberte si některý z tutoriálů níže.",
    "Looking for one thing? Type it into the search box at the top left - it searches every page and jumps to the spot.":
        "Hledáte jednu věc? Napište ji do vyhledávacího pole vlevo nahoře - prohledá každou stránku a skočí na místo.",
    "Tutorials":
        "Tutoriály",
    "Besides the big mission there are %d short tutorials - each shows one thing in three to six steps. All of them, with pictures: [[tutorials]].":
        "Kromě velké mise je tu %d krátkých tutoriálů - každý ukazuje jednu věc ve třech až šesti krocích. Všechny s obrázky: [[tutorials]].",
    "What the program can do":
        "Co program umí",
    "Area":
        "Oblast",
    "In short":
        "Stručně",
    "Build scripts from the command list, edit every value in a dialog, copy, move, undo - like the original BehavEd.":
        "Sestavujte skripty ze seznamu příkazů, upravujte každou hodnotu v dialogu, kopírujte, přesouvejte, vracejte - jako v původním BehavEd.",
    "See a script as a tree, as a graph of who starts whom, or as tracks over time.":
        "Zobrazte skript jako strom, jako graf toho, kdo koho spouští, nebo jako stopy v čase.",
    "The map in 3D with lights, effects and models; fly through it, select and move entities.":
        "Mapa ve 3D se světly, efekty a modely; prolétejte jí, vybírejte a posouvejte entity.",
    "Camera shots as keys on the map: move and turn them with the gizmo, fly them yourself, preview them.":
        "Záběry kamery jako klíče v mapě: posouvejte a otáčejte je gizmem, proleťte je sami, prohlédněte si náhled.",
    "Play the scene, see every command in time, drag commands to change when they happen.":
        "Přehrajte scénu, uvidíte každý příkaz v čase, tažením příkazů změníte, kdy se stanou.",
    "Paths of walking figures and flying ships: start and end handles, new points, walk or run, facing.":
        "Dráhy chodících postav a letících lodí: úchyty začátku a konce, nové body, chůze nebo běh, natočení.",
    "Animate ships, doors and platforms - as move commands or as a ROFF file, built and edited right here.":
        "Animujte lodě, dveře a plošiny - jako příkazy move nebo jako soubor ROFF, sestavené a upravené přímo zde.",
    "Open a whole mission with all its scripts, build your own, and pack it into a .pk3 the game loads.":
        "Otevřete celou misi se všemi skripty, postavte vlastní a zabalte ji do .pk3, který hra načte.",
    "**Nothing is lost.** Every change can be undone with <kbd>Ctrl</kbd>+<kbd>Z</kbd>, and files are only written when you save. The game files themselves are never changed: new things go next to your scripts or into your own .pk3.":
        "**Nic se neztratí.** Každou změnu lze vrátit pomocí <kbd>Ctrl</kbd>+<kbd>Z</kbd> a soubory se zapisují, jen když uložíte. Samotné soubory hry se nikdy nemění: nové věci jdou vedle vašich skriptů nebo do vašeho vlastního .pk3.",
    "Getting started":
        "Začínáme",
    "Unpack, tell the program where the game is, open a mission. That is all the setup there is.":
        "Rozbalte, řekněte programu, kde je hra, otevřete misi. To je celé nastavení.",
    "Install":
        "Instalace",
    "Download the latest `JK Studio-…-windows-x64.zip` from the releases page - under **Assets**, not \"Source code\", which contains no program - and unpack it into a folder of your choice, for example next to your game. Nothing is installed into Windows.":
        "Stáhněte nejnovější `JK Studio-…-windows-x64.zip` ze stránky vydání - pod **Assets**, ne \"Source code\", který žádný program neobsahuje - a rozbalte ho do libovolné složky, například vedle hry. Do Windows se nic neinstaluje.",
    "Start `jkstudio.exe`. At startup the program looks for a newer version on GitHub and offers it; nothing is downloaded without asking you (the switch is in **{{MenuHelp}}**).":
        "Spusťte `jkstudio.exe`. Při spuštění program hledá na GitHubu novější verzi a nabídne ji; bez dotazu se nic nestahuje (přepínač je v **{{MenuHelp}}**).",
    "**Just the program?** `JK Studio-…-windows-x64.exe` from the same release runs on its own: on the first start it puts its data into `%APPDATA%\\behaved\\data\\base`. The guide is only in the zip. Windows SmartScreen may warn the first time because the program is not code-signed: click **More info → Run anyway**.":
        "**Jen program?** `JK Studio-…-windows-x64.exe` ze stejného vydání běží samostatně: při prvním spuštění uloží svá data do `%APPDATA%\\behaved\\data\\base`. Příručka je jen v zipu. Windows SmartScreen může napoprvé varovat, protože program není podepsaný: klikněte na **Další informace → Přesto spustit**.",
    "Tell it where the game is":
        "Řekněte mu, kde je hra",
    "Maps, models, sounds and the scripts of the game live in the game's `.pk3` archives. Open **{{MenuEdit}} → {{PathsTitle}}** and add the game folders with **{{Pk3AddPath}}**: first `base`, then a mod such as Movie Duels (`MD`). The bottom folder wins, exactly as `fs_game` overrides `base` in the game.":
        "Mapy, modely, zvuky a skripty hry jsou v archivech `.pk3` hry. Otevřete **{{MenuEdit}} → {{PathsTitle}}** a přidejte složky hry pomocí **{{Pk3AddPath}}**: nejdřív `base`, pak mod jako Movie Duels (`MD`). Dolní složka vyhrává, přesně jako `fs_game` ve hře přepisuje `base`.",
    "The game folders: base first, the mod below it - the bottom one wins.":
        "Složky hry: nejdřív base, mod pod ní - vyhrává ta dolní.",
    "Open something":
        "Něco otevřete",
    "There are several ways in. The start page in the **{{ViewMap}}** tab (shown while no map is open) offers a new mission, a mission from a `.pk3`, just a map, and your recent missions. From the menu:":
        "Cest dovnitř je několik. Úvodní stránka na kartě **{{ViewMap}}** (zobrazená, dokud není otevřena žádná mapa) nabízí novou misi, misi z `.pk3`, jen mapu a vaše poslední mise. Z nabídky:",
    "**A mission** - **{{MenuFile}} → {{LoadMission}}** picks a `.pk3` and opens a mission with its map, its entities and all its scripts in tabs. See [[missionen]].":
        "**Mise** - **{{MenuFile}} → {{LoadMission}}** vybere `.pk3` a otevře misi s její mapou, entitami a všemi skripty na kartách. Viz [[missionen]].",
    "**A script** - **{{MenuFile}} → {{FileOpen}}** opens a `.txt` or a compiled `.ibi` from disk.":
        "**Skript** - **{{MenuFile}} → {{FileOpen}}** otevře `.txt` nebo zkompilované `.ibi` z disku.",
    "**Your last missions** - **{{MenuFile}} → {{RecentMissions}}** reopens a whole mission with one click.":
        "**Vaše poslední mise** - **{{MenuFile}} → {{RecentMissions}}** znovu otevře celou misi jedním kliknutím.",
    "The start page appears in the Mission tab while no map is open.":
        "Úvodní stránka se objeví na kartě Mise, dokud není otevřena žádná mapa.",
    "Language":
        "Jazyk",
    "The program speaks English, German, Chinese, Japanese, Spanish and Czech; a text a language does not have yet appears in English. It starts in the language of Windows; change it under **{{MenuView}} → {{ViewLanguage}}**. Command names and values stay as the game writes them (`affect`, `SET_NAVGOAL`), because that is what goes into the file.":
        "Program mluví anglicky, německy, čínsky, japonsky, španělsky a česky; text, který jazyk ještě nemá, se zobrazí anglicky. Začne v jazyce Windows; změníte ho v **{{MenuView}} → {{ViewLanguage}}**. Názvy příkazů a hodnoty zůstávají tak, jak je píše hra (`affect`, `SET_NAVGOAL`), protože právě to jde do souboru.",
    "This guide opens with <kbd>F1</kbd>, from **{{MenuHelp}} → {{HelpGuide}}**, from **{{StartGuideFirst}}** and **{{StartGuideTut}}** on the start page, and from the **{{AboutGuide}}** button in {{HelpAbout}}. It lives in the `handbuch` folder next to `jkstudio.exe` and works without internet. **{{MenuHelp}} → {{TourMenu}}** shows the first steps right in the program, one highlighted click at a time.":
        "Tato příručka se otevře klávesou <kbd>F1</kbd>, z **{{MenuHelp}} → {{HelpGuide}}**, ze **{{StartGuideFirst}}** a **{{StartGuideTut}}** na úvodní stránce a tlačítkem **{{AboutGuide}}** v {{HelpAbout}}. Je ve složce `handbuch` vedle `jkstudio.exe` a funguje bez internetu. **{{MenuHelp}} → {{TourMenu}}** ukáže první kroky přímo v programu, po jednom zvýrazněném kliknutí.",
    "What JK Studio can do":
        "Co JK Studio umí",
    "A tour of the whole program: writing scripts, seeing them on the map, cameras, figures, animation, missions and the tools around them.":
        "Prohlídka celého programu: psaní skriptů, jejich zobrazení v mapě, kamery, postavy, animace, mise a nástroje kolem.",
    "JK Studio writes ICARUS scripts - the scripts behind every cutscene and every scripted moment of Jedi Academy and its mods. It writes them exactly like Raven's original BehavEd, file for file, and then shows you what they do: on the map, in 3D, over time. Each section below links to the tutorials that practise it.":
        "JK Studio píše skripty ICARUS - skripty za každou animační sekvencí a každým skriptovaným okamžikem Jedi Academy a jejích modů. Píše je přesně jako původní BehavEd od Raven, soubor po souboru, a pak vám ukáže, co dělají: v mapě, ve 3D, v čase. Každá část níže odkazuje na tutoriály, které ji procvičují.",
    "From an idea to the game":
        "Od nápadu do hry",
    "Write the script":
        "Napsat skript",
    "See it on the map":
        "Podívat se na něj v mapě",
    "Time it":
        "Načasovat ho",
    "Save mission":
        "Uložit misi",
    "Play it":
        "Zahrát si",
    "commands, values, blocks":
        "příkazy, hodnoty, bloky",
    "figures, camera, ships":
        "postavy, kamera, lodě",
    "timeline, Tracks":
        "časová osa, Stopy",
    "checks, compile, .pk3":
        "kontroly, kompilace, .pk3",
    "devmap <name>":
        "devmap <name>",
    "Write scripts like the original":
        "Psát skripty jako v originálu",
    "The command list, the script tree and the action column.":
        "Seznam příkazů, strom skriptu a sloupec akcí.",
    "Every command of the game in one list - double-click, drag or press <kbd>Insert</kbd> to add it. [[k-befehl-einfuegen]]":
        "Každý příkaz hry v jednom seznamu - přidáte ho dvojklikem, přetažením nebo klávesou <kbd>Insert</kbd>. [[k-befehl-einfuegen]]",
    "Each value in its own field: lists of allowed values, names picked from the map, sounds you can play, expressions with `get`, `tag` and `random`. [[k-werte-bearbeiten]]":
        "Každá hodnota ve vlastním poli: seznamy povolených hodnot, jména vybraná z mapy, zvuky, které lze přehrát, výrazy s `get`, `tag` a `random`. [[k-werte-bearbeiten]]",
    "Copy, move, comment out, undo every step - even one single step from the middle of the history. [[k-zeilen-ordnen]], [[k-rueckgaengig]]":
        "Kopírujte, přesouvejte, zakomentujte, vraťte každý krok - i jediný krok ze středu historie. [[k-zeilen-ordnen]], [[k-rueckgaengig]]",
    "Compile to the `.ibi` the game runs, with clear messages that jump to the wrong line. [[k-uebersetzen]]":
        "Kompilace do `.ibi`, které hra spouští, s jasnými zprávami, které skočí na špatný řádek. [[k-uebersetzen]]",
    "Every command and every `SET_` field explained: [[befehle]].":
        "Každý příkaz a každé pole `SET_` vysvětleno: [[befehle]].",
    "Three views of the same script":
        "Tři pohledy na tentýž skript",
    "Graph: what starts what.":
        "Graf: co co spouští.",
    "Tracks: the script over time.":
        "Stopy: skript v čase.",
    "**{{FlowClassic}}** is the tree of the original. **{{FlowGraph}}** shows the mission as nodes - the player start, runners, characters, scripts - and lets you connect them by dragging. **{{FlowTracks}}** lays the script out over time, with the camera, every figure and the dialogue in rows. [[skriptfluss]], [[k-graph]]":
        "**{{FlowClassic}}** je strom originálu. **{{FlowGraph}}** ukazuje misi jako uzly - start hráče, spouštěče, postavy, skripty - a umožňuje je spojovat tažením. **{{FlowTracks}}** rozloží skript v čase, s kamerou, každou postavou a dialogem v řádcích. [[skriptfluss]], [[k-graph]]",
    "The map in 3D - as the game draws it":
        "Mapa ve 3D - jak ji kreslí hra",
    "The landing pad of taspir1 with the mission's entities.":
        "Přistávací plošina taspir1 s entitami mise.",
    "Maps load straight from the game's `.pk3` files, with their shaders, lightmaps, fog, glow, effects, sky portals and mirrors. Fly through them, look from above, save views on number keys. Place entities with tiles, drag and turn them on the grid, edit every key in the inspector. [[karte]], [[k-entity-setzen]], [[k-entity-felder]], [[k-draufsicht]]":
        "Mapy se načítají přímo ze souborů `.pk3` hry, s jejich shadery, lightmapami, mlhou, září, efekty, portály oblohy a zrcadly. Prolétejte jimi, dívejte se shora, ukládejte pohledy na číselné klávesy. Umisťujte entity dlaždicemi, táhněte a otáčejte je na mřížce, upravujte každý klíč v inspektoru. [[karte]], [[k-entity-setzen]], [[k-entity-felder]], [[k-draufsicht]]",
    "Cameras you can see and fly":
        "Kamery, které vidíte a můžete proletět",
    "A camera path played through the camera.":
        "Dráha kamery přehraná pohledem kamery.",
    "Each `camera ( MOVE )` and `PAN` is a shot on the map with its path. Move and turn shots with the gizmo, put them where you are looking, or sit in a shot and fly it in pilot mode. Zoom, fade, shake and follow play in the view exactly as in the game. [[kamera]], [[k-pilot]], [[k-kamera-ansicht]], [[k-zoom-blende]]":
        "Každý `camera ( MOVE )` a `PAN` je záběr v mapě se svou dráhou. Posouvejte a otáčejte záběry gizmem, umístěte je tam, kam se díváte, nebo se usaďte v záběru a proleťte ho v režimu pilota. Zoom, zatmívání, otřesy a sledování se v pohledu přehrávají přesně jako ve hře. [[kamera]], [[k-pilot]], [[k-kamera-ansicht]], [[k-zoom-blende]]",
    "Figures that walk their paths":
        "Postavy, které chodí po svých drahách",
    "A figure in the middle of its walk, with its path.":
        "Postava uprostřed chůze, se svou dráhou.",
    "Characters spawn with their real models and animations and walk where the script sends them. Their paths have handles: drag a walk target, insert a point mid-walk, switch walk, run or walk backwards, set where a figure looks and which way it faces. [[figuren]], [[k-gangart]], [[k-ansehen]], [[k-blickrichtung]]":
        "Postavy se objevují se svými skutečnými modely a animacemi a chodí tam, kam je skript pošle. Jejich dráhy mají úchyty: táhněte cíl chůze, vložte bod uprostřed chůze, přepněte chůzi, běh nebo chůzi pozpátku, nastavte, kam se postava dívá a kam je natočená. [[figuren]], [[k-gangart]], [[k-ansehen]], [[k-blickrichtung]]",
    "A timeline for the whole scene":
        "Časová osa pro celou scénu",
    "Play the scene with sound, at any speed. Every command is a block in time - drag it and everything after moves along, drag its edge to change a duration. [[zeitleiste]], [[k-neu-timen]]":
        "Přehrajte scénu se zvukem, libovolnou rychlostí. Každý příkaz je blok v čase - táhněte ho a vše po něm se posune s ním, táhněte jeho okraj a změníte délku. [[zeitleiste]], [[k-neu-timen]]",
    "Animation: move commands and ROFF files":
        "Animace: příkazy move a soubory ROFF",
    "The Raven's Claw taking off on a ROFF path.":
        "Raven's Claw vzlétá po dráze ROFF.",
    "Set keys over time for a door, a platform or a ship. behaved writes them as `move` commands or builds a ROFF file, and it turns the game's own ROFF files into keys you can change - as a copy, never touching the original. [[animation]], [[tut-p5]], [[k-tuer]], [[k-roff-bearbeiten]]":
        "Nastavte klíče v čase pro dveře, plošinu nebo loď. behaved je zapíše jako příkazy `move` nebo sestaví soubor ROFF a vlastní soubory ROFF hry převede na klíče, které můžete měnit - jako kopii, originálu se nedotkne. [[animation]], [[tut-p5]], [[k-tuer]], [[k-roff-bearbeiten]]",
    "Missions: from nothing to a .pk3":
        "Mise: od ničeho k .pk3",
    "Saving a mission: every file, every check.":
        "Uložení mise: každý soubor, každá kontrola.",
    "Start a mission on any map with one click, open the missions of the game or a mod, continue them from their folder. **{{MissionSave}}** checks everything, compiles every script and packs one `.pk3` with map, entities, scripts and ROFF files - ready for `devmap`. NPC waypoint networks are created and checked on the way. [[missionen]], [[tut-projekt]], [[k-wegpunkte]]":
        "Začněte misi na libovolné mapě jedním kliknutím, otevřete mise hry nebo modu, pokračujte v nich z jejich složky. **{{MissionSave}}** vše zkontroluje, zkompiluje každý skript a zabalí jeden `.pk3` s mapou, entitami, skripty a soubory ROFF - připravený pro `devmap`. Sítě navigačních bodů NPC se po cestě vytvoří a zkontrolují. [[missionen]], [[tut-projekt]], [[k-wegpunkte]]",
    "Character models and their animations":
        "Modely postav a jejich animace",
    "A character in the model view, with its animation list.":
        "Postava v zobrazení modelu se seznamem animací.",
    "Every character of the game with its skin, every animation with its frames - click to play, double-click to put it into the script. [[modell]], [[k-animation-abspielen]], [[k-modell]]":
        "Každá postava hry se svým skinem, každá animace se svými snímky - kliknutím ji přehrajete, dvojklikem ji vložíte do skriptu. [[modell]], [[k-animation-abspielen]], [[k-modell]]",
    "The tools around it":
        "Nástroje kolem",
    "Find and replace in one script or in all open ones. [[k-suchen-ersetzen]]":
        "Najít a nahradit v jednom skriptu nebo ve všech otevřených. [[k-suchen-ersetzen]]",
    "Tabs, bookmarks and a split view to compare scripts and copy across. [[k-lesezeichen]]":
        "Karty, záložky a rozdělené zobrazení pro porovnání skriptů a kopírování napříč. [[k-lesezeichen]]",
    "Backups on every save, a backup folder, revert any command to how it was. [[speichern]]":
        "Zálohy při každém uložení, složka záloh, vrácení libovolného příkazu do původního stavu. [[speichern]]",
    "Six languages, six themes, any interface size, every shortcut rebindable. [[einstellungen]], [[tasten]]":
        "Šest jazyků, šest motivů, libovolná velikost rozhraní, každou zkratku lze změnit. [[einstellungen]], [[tasten]]",
    "Checks for updates and installs them. [[einstellungen#updates]]":
        "Vyhledává aktualizace a instaluje je. [[einstellungen#updates]]",
    "The window":
        "Okno",
    "One window, three modes. The mode bar on the left decides what the big area shows; script, map and timeline always talk to each other.":
        "Jedno okno, tři režimy. Lišta režimů vlevo určuje, co ukazuje velká plocha; skript, mapa a časová osa spolu vždy komunikují.",
    "The window in the Mission tab with a mission open.":
        "Okno na kartě Mise s otevřenou misí.",
    "Menu bar":
        "Řádek nabídek",
    "File, Edit, Debug, View, Help.":
        "Soubor, Úpravy, Ladění, Zobrazení, Nápověda.",
    "Tabs":
        "Karty",
    "One tab per open script; **+** opens an empty one, the mouse wheel over the bar scrolls through many.":
        "Jedna karta na otevřený skript; **+** otevře prázdnou, kolečko myši nad lištou posouvá mezi mnoha.",
    "Mode bar":
        "Lišta režimů",
    "**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** - what the big area shows.":
        "**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** - co ukazuje velká plocha.",
    "Entity list and inspector":
        "Seznam entit a inspektor",
    "Every entity of the map in groups; below, the keys of the selected one.":
        "Každá entita mapy ve skupinách; pod tím klíče vybrané.",
    "Map":
        "Mapa",
    "The toolbar and the map in 3D, with the placement tiles on the left.":
        "Lišta nástrojů a mapa ve 3D, s dlaždicemi pro umisťování vlevo.",
    "Script column":
        "Sloupec skriptu",
    "The script as **{{FlowClassic}}** tree, **{{FlowGraph}}** or **{{FlowTracks}}**.":
        "Skript jako strom **{{FlowClassic}}**, **{{FlowGraph}}** nebo **{{FlowTracks}}**.",
    "Timeline":
        "Časová osa",
    "Plays the scene; every command is a block in time.":
        "Přehrává scénu; každý příkaz je blok v čase.",
    "Status line":
        "Stavový řádek",
    "Messages, the undo history, when the script was compiled.":
        "Zprávy, historie kroků zpět, kdy byl skript zkompilován.",
    "The parts":
        "Části",
    "Part":
        "Část",
    "What it is for":
        "K čemu slouží",
    "{{MenuFile}}, {{MenuEdit}}, {{MenuDebug}}, {{MenuView}}, {{MenuHelp}} - opening and saving, game folders, look and language, the guide.":
        "{{MenuFile}}, {{MenuEdit}}, {{MenuDebug}}, {{MenuView}}, {{MenuHelp}} - otevírání a ukládání, složky hry, vzhled a jazyk, příručka.",
    "Script tabs":
        "Karty skriptů",
    "One tab per open script. A `*` means unsaved changes, `+` opens a new script, tabs can be dragged into another order. When there are more tabs than fit, turn the mouse wheel over the tab bar to scroll them; the small arrow at the left lists every open script. All tabs of a mission share the map and the camera.":
        "Jedna karta na otevřený skript. `*` znamená neuložené změny, `+` otevře nový skript, karty lze přetahovat do jiného pořadí. Když je karet víc, než se vejde, otáčejte kolečkem myši nad lištou karet a posunete je; malá šipka vlevo vypíše každý otevřený skript. Všechny karty mise sdílejí mapu a kameru.",
    "The strip at the far left: **{{GroupEvents}}** (build scripts), **{{ViewMap}}** (the mission: the map in 3D, entities, cameras, timeline - this tab was called *Map* before version rc600), **{{ViewModel}}** (look at a character model).":
        "Pruh úplně vlevo: **{{GroupEvents}}** (tvorba skriptů), **{{ViewMap}}** (mise: mapa ve 3D, entity, kamery, časová osa - před verzí rc600 se tato karta jmenovala *Mapa*), **{{ViewModel}}** (prohlížení modelu postavy).",
    "Left column":
        "Levý sloupec",
    "Depends on the mode: the list of commands, the entities of the map with their inspector, or the animations of a model.":
        "Záleží na režimu: seznam příkazů, entity mapy s inspektorem, nebo animace modelu.",
    "The script itself - as a tree ({{FlowClassic}}), a graph ({{FlowGraph}}) or over time ({{FlowTracks}}). See [[skriptfluss]].":
        "Samotný skript - jako strom ({{FlowClassic}}), graf ({{FlowGraph}}) nebo v čase ({{FlowTracks}}). Viz [[skriptfluss]].",
    "The button column on the right, as in the original: edit, file, tree options, **{{Compile}}**. In the **{{ViewMap}}** tab it makes room for the map.":
        "Sloupec tlačítek vpravo, jako v originálu: úpravy, soubor, volby stromu, **{{Compile}}**. Na kartě **{{ViewMap}}** uvolní místo mapě.",
    "Under the map: plays the scene and shows every command in time. See [[zeitleiste]].":
        "Pod mapou: přehrává scénu a ukazuje každý příkaz v čase. Viz [[zeitleiste]].",
    "Messages, the error and note counters (click them for the list), the undo history and when the script was last compiled.":
        "Zprávy, počítadla chyb a poznámek (kliknutím zobrazíte seznam), historie kroků zpět a kdy byl skript naposledy zkompilován.",
    "Two scripts side by side":
        "Dva skripty vedle sebe",
    "**{{MenuFile}} → {{SplitToggle}}** shows up to four scripts beside each other in Events mode - for comparing and for copying commands across. Drag a command from one pane into another to copy it; click into a pane to work in it.":
        "**{{MenuFile}} → {{SplitToggle}}** ukáže v režimu Události až čtyři skripty vedle sebe - pro porovnání a kopírování příkazů napříč. Přetažením příkazu z jednoho panelu do druhého ho zkopírujete; kliknutím do panelu v něm pracujete.",
    "Size, look and layout":
        "Velikost, vzhled a rozvržení",
    "<kbd>Ctrl</kbd> + mouse wheel scales the whole interface (50 - 300 %); **{{MenuView}} → {{ViewDpiScale}}** does the same.":
        "<kbd>Ctrl</kbd> + kolečko myši mění měřítko celého rozhraní (50 - 300 %); **{{MenuView}} → {{ViewDpiScale}}** dělá totéž.",
    "Drag the borders between the columns to resize them; each mode remembers its widths. **{{MenuDebug}} → {{DebugReset}}** puts everything back.":
        "Tažením hranic mezi sloupci měníte jejich velikost; každý režim si pamatuje své šířky. **{{MenuDebug}} → {{DebugReset}}** vrátí vše zpět.",
    "Opening a file in Explorer while JK Studio runs hands it to the open window instead of starting a second program.":
        "Otevření souboru v Průzkumníkovi, zatímco JK Studio běží, ho předá otevřenému oknu, místo aby se spustil druhý program.",
    "Writing scripts":
        "Psaní skriptů",
    "An ICARUS script is a tree of commands. Pick commands from the list, put them in the right block, fill in their values.":
        "Skript ICARUS je strom příkazů. Vybírejte příkazy ze seznamu, dávejte je do správného bloku, vyplňujte jejich hodnoty.",
    "Events mode: the commands on the left, the script tree in the middle, the action buttons on the right.":
        "Režim Události: příkazy vlevo, strom skriptu uprostřed, tlačítka akcí vpravo.",
    "How a script is built":
        "Jak je skript postaven",
    "Commands run from top to bottom. `affect ( \"name\", FLUSH ) { ... }` sends the commands inside it to one figure or entity - that is how a cutscene moves its actors. `task` declares a group of commands, `do` starts it alongside, `dowait` waits for it, `wait` pauses. `if`, `else`, `loop` work as in other languages. Only `affect`, `task`, `if`, `else` and `loop` can hold commands.":
        "Příkazy běží shora dolů. `affect ( \"name\", FLUSH ) { ... }` posílá příkazy uvnitř jedné postavě nebo entitě - tak animační sekvence hýbe svými herci. `task` deklaruje skupinu příkazů, `do` ji spustí souběžně, `dowait` na ni počká, `wait` čeká. `if`, `else`, `loop` fungují jako v jiných jazycích. Příkazy mohou obsahovat jen `affect`, `task`, `if`, `else` a `loop`.",
    "Adding commands":
        "Přidávání příkazů",
    "In the **{{GroupEvents}}** list on the left, **double-click** a command to insert it after the selected line (or into the selected block).":
        "V seznamu **{{GroupEvents}}** vlevo **dvakrát klikněte** na příkaz a vložíte ho za vybraný řádek (nebo do vybraného bloku).",
    "Or **drag** it into the tree: where you let go decides whether it lands before, inside or after a line.":
        "Nebo ho **přetáhněte** do stromu: kde ho pustíte, rozhoduje, zda dopadne před řádek, dovnitř nebo za něj.",
    "Macros at the end of the list insert several commands at once; **{{TreeFoldMacros}}** shows them as one line.":
        "Makra na konci seznamu vloží několik příkazů najednou; **{{TreeFoldMacros}}** je ukáže jako jeden řádek.",
    "Right-click a line for the same lists as submenus, plus edit, copy and **{{ActFindSimilar}}**.":
        "Pravým tlačítkem na řádku získáte tytéž seznamy jako podnabídky, plus úpravy, kopírování a **{{ActFindSimilar}}**.",
    "The event editor":
        "Editor událostí",
    "Double-click a line (or press <kbd>Enter</kbd>) to open the **{{EditorTitle}}**. The help text of the command stands on top, each value has its own field below: lists for fixed choices, number and text fields, file fields with **{{EditorBrowse}}** to browse and **{{EditorPlay}}** to play a sound or open a script. When a map is loaded, an arrow next to name fields picks a name from the map.":
        "Dvakrát klikněte na řádek (nebo stiskněte <kbd>Enter</kbd>) a otevře se **{{EditorTitle}}**. Nahoře stojí text nápovědy příkazu, pod ním má každá hodnota vlastní pole: seznamy pro pevné volby, číselná a textová pole, souborová pole s **{{EditorBrowse}}** pro procházení a **{{EditorPlay}}** pro přehrání zvuku nebo otevření skriptu. Je-li načtena mapa, šipka vedle polí se jmény vybere jméno z mapy.",
    "**{{EditorExpr}}** turns a field into a free expression; **{{EditorHelper}}** builds `get ( ... )`, `tag ( ... )` or `random ( ... )` with **{{EditorGet}}**, **{{EditorTag}}** and **{{EditorRnd}}**; **{{EditorRevert}}** goes back.":
        "**{{EditorExpr}}** změní pole na volný výraz; **{{EditorHelper}}** sestaví `get ( ... )`, `tag ( ... )` nebo `random ( ... )` pomocí **{{EditorGet}}**, **{{EditorTag}}** a **{{EditorRnd}}**; **{{EditorRevert}}** se vrátí.",
    "A value that is not in its list is allowed - mods bring their own - but marked **unknown**, because far more often it is a typo.":
        "Hodnota, která není ve svém seznamu, je povolena - mody přinášejí vlastní - ale označena jako **neznámá**, protože mnohem častěji jde o překlep.",
    "**{{EditorOk}}** (<kbd>Enter</kbd>) writes the values, **{{EditorCancel}}** (<kbd>Esc</kbd>) drops them, **{{EditorReEvaluate}}** resets the fields to the defaults.":
        "**{{EditorOk}}** (<kbd>Enter</kbd>) zapíše hodnoty, **{{EditorCancel}}** (<kbd>Esc</kbd>) je zahodí, **{{EditorReEvaluate}}** vrátí pole na výchozí hodnoty.",
    "Selecting, moving, copying":
        "Výběr, přesun, kopírování",
    "Click selects a line, <kbd>Ctrl</kbd>+click adds one, <kbd>Shift</kbd>+click selects a range. Every action works on everything selected.":
        "Kliknutí vybere řádek, <kbd>Ctrl</kbd>+kliknutí přidá další, <kbd>Shift</kbd>+kliknutí vybere rozsah. Každá akce působí na vše vybrané.",
    "Drag lines to move them. Blocks move with everything inside; a block cannot go into itself.":
        "Tažením řádků je přesunete. Bloky se přesouvají se vším uvnitř; blok nemůže jít sám do sebe.",
    "**{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}** - pasting into a block puts the commands in exactly once, also when the block and its lines were copied together.":
        "**{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}** - vložení do bloku vloží příkazy přesně jednou, i když byly blok a jeho řádky zkopírovány spolu.",
    "**{{ActClone}}** inserts a copy right below, **{{ActRem}}** turns lines into comments and back, **{{ActDelete}}** removes them.":
        "**{{ActClone}}** vloží kopii hned pod, **{{ActRem}}** mění řádky na komentáře a zpět, **{{ActDelete}}** je odebere.",
    "**Undo** and **Redo** remember every step. Right-click either button for the full history - you can even undo one single step and keep everything after it.":
        "**Zpět** a **Znovu** si pamatují každý krok. Pravým tlačítkem na kterékoli z nich získáte celou historii - můžete dokonce vrátit jediný krok a vše po něm ponechat.",
    "Find, replace, bookmarks":
        "Hledání, nahrazení, záložky",
    "**{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>) opens a window that stays open: search by command name (**{{FindNamed}}**) or by a value (**{{FindContains}}**), step with <kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>, replace in this script or in all open ones. Bookmarks work like Notepad++: <kbd>Ctrl</kbd>+<kbd>F2</kbd> sets one, <kbd>F2</kbd> jumps to the next. They are kept per file.":
        "**{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>) otevře okno, které zůstane otevřené: hledejte podle názvu příkazu (**{{FindNamed}}**) nebo podle hodnoty (**{{FindContains}}**), procházejte <kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>, nahrazujte v tomto skriptu nebo ve všech otevřených. Záložky fungují jako v Notepad++: <kbd>Ctrl</kbd>+<kbd>F2</kbd> jednu nastaví, <kbd>F2</kbd> skočí na další. Pamatují se pro každý soubor.",
    "The change margin":
        "Okraj změn",
    "With **{{MenuView}} → {{ViewChangeHistory}}** a coloured margin marks what you changed: orange = changed, not saved; green = changed and saved; blue = back to the original. Hover it to see the original line, right-click a line → **Revert to original** to restore just that command.":
        "S **{{MenuView}} → {{ViewChangeHistory}}** označuje barevný okraj, co jste změnili: oranžová = změněno, neuloženo; zelená = změněno a uloženo; modrá = zpět v původním stavu. Najeďte na něj myší a uvidíte původní řádek, pravým tlačítkem na řádku → **Vrátit na původní** obnovíte jen tento příkaz.",
    "Script Flow: Classic, Graph, Tracks":
        "Průběh skriptu: Klasický, Graf, Stopy",
    "Three views of the same script - switch any time, nothing is lost.":
        "Tři pohledy na tentýž skript - přepínejte kdykoli, nic se neztratí.",
    "The buttons **{{FlowClassic}} / {{FlowGraph}} / {{FlowTracks}}** at the top of the {{GroupScriptFlow}} column switch the view. In a split view every pane has its own.":
        "Tlačítka **{{FlowClassic}} / {{FlowGraph}} / {{FlowTracks}}** nahoře ve sloupci {{GroupScriptFlow}} přepínají pohled. V rozděleném zobrazení má každý panel vlastní.",
    "Classic":
        "Klasický",
    "The tree of the original BehavEd - every command one line, blocks to fold open; see [[skripte]]. Since rc601 you can build and change a script in **every** view - the tutorials on scripts have a switch at the top that shows their steps for Classic, Graph or Tracks.":
        "Strom původního BehavEd - každý příkaz jeden řádek, bloky k rozbalení; viz [[skripte]]. Od rc601 můžete skript sestavovat a měnit v **každém** pohledu - tutoriály ke skriptům mají nahoře přepínač, který ukazuje jejich kroky pro Klasický, Graf nebo Stopy.",
    "Graph: what starts what":
        "Graf: co co spouští",
    "The graph of a mission: player start, triggers, script runners, figures and scripts with their connections.":
        "Graf mise: start hráče, triggery, spouštěče skriptů, postavy a skripty s jejich spojeními.",
    "A mission is held together by its entities: a `trigger` targets a `target_scriptrunner`, which runs a script, which affects figures and starts other scripts. The graph draws exactly that. Double-click a node to open its script or show the entity in the map; drag from the dot on the right of a node to another node to connect them - for a figure you choose what kind of script (spawnscript, deathscript, ...). Drag entities from the list or commands into the graph; a command dropped on free space creates a new script.":
        "Misi drží pohromadě její entity: `trigger` cílí na `target_scriptrunner`, který spustí skript, který ovlivňuje postavy a spouští další skripty. Graf kreslí přesně to. Dvojklikem na uzel otevřete jeho skript nebo ukážete entitu v mapě; tažením z tečky vpravo u uzlu na jiný uzel je spojíte - u postavy zvolíte, jaký druh skriptu (spawnscript, deathscript, ...). Přetahujte do grafu entity ze seznamu nebo příkazy; příkaz puštěný na volné místo vytvoří nový skript.",
    "**{{GraphFit}}** shows everything, **{{GraphRelayout}}** sorts the nodes, **{{GraphRightAngles}}** draws the wires at right angles through the gaps. Double-click a wire for a bend point; right-click removes it.":
        "**{{GraphFit}}** ukáže vše, **{{GraphRelayout}}** seřadí uzly, **{{GraphRightAngles}}** kreslí spoje v pravých úhlech mezerami. Dvojklik na spoj přidá bod ohybu; pravé tlačítko ho odebere.",
    "**Editing in the Graph.** Click a script node: on the right, **{{GraphCommands}}** lists every command of the script, blocks indented. Click a line to select it, double-click to open the editor, right-click for the same menu as in Classic (edit, duplicate, copy, cut, paste, move, REM, delete, insert). A command from the **{{GroupEvents}}** list dropped on a line goes after it; double-clicking a command in the list inserts it after the selected line. A script that is not open shows **{{GraphEditHere}}**.":
        "**Úpravy v Grafu.** Klikněte na uzel skriptu: vpravo vypíše **{{GraphCommands}}** každý příkaz skriptu, bloky odsazené. Kliknutím na řádek ho vyberete, dvojklikem otevřete editor, pravým tlačítkem stejnou nabídku jako v Klasickém (upravit, duplikovat, kopírovat, vyjmout, vložit, posunout, REM, odstranit, vložit nový). Příkaz ze seznamu **{{GroupEvents}}** puštěný na řádek jde za něj; dvojklik na příkaz v seznamu ho vloží za vybraný řádek. Skript, který není otevřený, ukazuje **{{GraphEditHere}}**.",
    "Tracks: the script over time":
        "Stopy: skript v čase",
    "Tracks: camera shots, figures, dialogue and script as blocks in time.":
        "Stopy: záběry kamery, postavy, dialog a skript jako bloky v čase.",
    "A compact timeline inside the script column: rows for **{{TracksCamera}}**, every figure, **{{TracksDialog}}** (voice lines) and **{{TracksScript}}**. Click a block to select its line, drag its right edge to change the duration, drag in the ruler to move through time. The big [[zeitleiste]] under the map does the same with more detail.":
        "Kompaktní časová osa uvnitř sloupce skriptu: řádky pro **{{TracksCamera}}**, každou postavu, **{{TracksDialog}}** (hlasové repliky) a **{{TracksScript}}**. Kliknutím na blok vyberete jeho řádek, tažením pravého okraje změníte délku, tažením v pravítku se pohybujete v čase. Velká [[zeitleiste]] pod mapou dělá totéž podrobněji.",
    "**Editing in Tracks.** Double-click a block to open the editor; right-click it for the menu of the Classic line. Drag a command from the **{{GroupEvents}}** list into a row: a yellow line shows the moment, and the command goes after the last command of that row that starts before it - in a figure's row, inside that figure's `affect` block. On the right, **{{TracksList}}** lists every command, also those that take no time (`affect`, `task`, `declare`) and an empty script - it works like the list in the Graph. Switch it off with the checkbox to give the timeline the whole width.":
        "**Úpravy ve Stopách.** Dvojklikem na blok otevřete editor; pravým tlačítkem nabídku klasického řádku. Přetáhněte příkaz ze seznamu **{{GroupEvents}}** do řádku: žlutá čára ukazuje okamžik a příkaz jde za poslední příkaz tohoto řádku, který začíná před ním - v řádku postavy dovnitř bloku `affect` této postavy. Vpravo vypíše **{{TracksList}}** každý příkaz, i ty, které nezaberou čas (`affect`, `task`, `declare`), a prázdný skript - funguje jako seznam v Grafu. Vypnete ho zaškrtávacím políčkem a časová osa dostane celou šířku.",
    "Saving, compiling, checks":
        "Ukládání, kompilace, kontroly",
    "The game runs `.ibi` files. You write `.txt`; **Compile!** makes the `.ibi` - byte for byte like Raven's IBIZE.":
        "Hra spouští soubory `.ibi`. Vy píšete `.txt`; **Kompilovat!** vytvoří `.ibi` - bajt po bajtu jako IBIZE od Raven.",
    "Saving":
        "Ukládání",
    "**{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) always writes the `.txt` source - also when you opened an `.ibi` (it lands next to it).":
        "**{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) vždy zapíše zdrojový `.txt` - i když jste otevřeli `.ibi` (uloží se vedle něj).",
    "**{{ActSaveAll}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) saves every tab with changes; tabs from an archive have no file to write back to and are skipped.":
        "**{{ActSaveAll}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) uloží každou kartu se změnami; karty z archivu nemají soubor, kam zapisovat, a přeskočí se.",
    "Every save and compile keeps a copy - `backup1.txt` (newest) to `backup10.txt` - in the `backup` folder next to `jkstudio.exe`, one subfolder per script; **{{MenuFile}} → {{FileBackupFolder}}** shows them. **{{FileBackup}}** / **{{FileRestore}}** keep and restore one extra copy (`<script>.bak` next to the script) by hand.":
        "Každé uložení a kompilace uchová kopii - `backup1.txt` (nejnovější) až `backup10.txt` - ve složce `backup` vedle `jkstudio.exe`, jedna podsložka na skript; **{{MenuFile}} → {{FileBackupFolder}}** je ukáže. **{{FileBackup}}** / **{{FileRestore}}** ručně uchová a obnoví jednu kopii navíc (`<script>.bak` vedle skriptu).",
    "Own ROFF paths are written as `.rof` files together with the script - see [[animation#roff]].":
        "Vlastní dráhy ROFF se zapisují jako soubory `.rof` spolu se skriptem - viz [[animation#roff]].",
    "Compiling":
        "Kompilace",
    "**{{Compile}}** in the action column writes `<script>.ibi` next to the `.txt` (and saves the `.txt`). The button flashes green with **{{CompileDone}}**, or red with **{{CompileFailedShort}}** - then the first bad line is selected and the status line says why. The compiler is built in and was checked against 1011 `.ibi` files from Raven's IBIZE, byte for byte.":
        "**{{Compile}}** ve sloupci akcí zapíše `<script>.ibi` vedle `.txt` (a uloží `.txt`). Tlačítko zabliká zeleně s **{{CompileDone}}**, nebo červeně s **{{CompileFailedShort}}** - pak se vybere první špatný řádek a stavový řádek řekne proč. Kompilátor je vestavěný a byl ověřen proti 1011 souborům `.ibi` z IBIZE od Raven, bajt po bajtu.",
    "Notes and errors":
        "Poznámky a chyby",
    "While you write, the program checks every line: unknown commands, values that are not in their list, a text field without quotes, a misspelled field (\"did you mean ...?\"). The counters in the status line show how many there are - click them for the list, click a message to jump to its line. **{{MenuView}} → {{ViewTarget}}** adds notes on what does not work on a multiplayer server.":
        "Zatímco píšete, program kontroluje každý řádek: neznámé příkazy, hodnoty, které nejsou ve svém seznamu, textové pole bez uvozovek, chybně napsané pole (\"mysleli jste ...?\"). Počítadla ve stavovém řádku ukazují, kolik jich je - kliknutím zobrazíte seznam, kliknutím na zprávu skočíte na její řádek. **{{MenuView}} → {{ViewTarget}}** přidá poznámky o tom, co na serveru pro více hráčů nefunguje.",
    "Camera":
        "Kamera",
    "Every camera MOVE and PAN of the script is a shot you can see, grab and fly - the script is rewritten as you drag.":
        "Každý MOVE a PAN kamery ve skriptu je záběr, který vidíte, můžete uchopit a proletět - skript se přepisuje, zatímco táhnete.",
    "A selected camera: its path on the map, the gizmo, the toolbar on top and the list of shots on the left.":
        "Vybraná kamera: její dráha v mapě, gizmo, lišta nahoře a seznam záběrů vlevo.",
    "Shots":
        "Záběry",
    "In ICARUS a shot is `camera ( MOVE, <position>, <time> )` followed by `camera ( PAN, <angles>, <time> )`. JK Studio reads them in order and draws each as a camera symbol on the map. Click the camera symbol to select the camera; the list **{{CamShots}}** on the left shows every shot with its time and travel time (`moves 6.0 s`). Click a line to jump there.":
        "V ICARUS je záběr `camera ( MOVE, <position>, <time> )` následovaný `camera ( PAN, <angles>, <time> )`. JK Studio je čte v pořadí a každý kreslí jako symbol kamery v mapě. Kliknutím na symbol kamery kameru vyberete; seznam **{{CamShots}}** vlevo ukazuje každý záběr s jeho časem a dobou přesunu (`pohyb 6.0 s`). Kliknutím na řádek tam skočíte.",
    "**{{CamPrevShot}} / {{CamNextShot}}** step through the shots; **{{CamFlyToShot}}** puts your view exactly where the selected shot stands and looks.":
        "**{{CamPrevShot}} / {{CamNextShot}}** procházejí záběry; **{{CamFlyToShot}}** umístí váš pohled přesně tam, kde vybraný záběr stojí a kam se dívá.",
    "**{{CamNewShot}}** adds a MOVE and a PAN at your current view, after the selected shot. **{{CamSetToView}}** moves the selected shot to where you are looking.":
        "**{{CamNewShot}}** přidá MOVE a PAN ve vašem aktuálním pohledu, za vybraný záběr. **{{CamSetToView}}** posune vybraný záběr tam, kam se díváte.",
    "**{{CamDeleteShot}}** removes it. Under **{{CamSelectedShot}}** you type exact values: {{CamPosition}}, {{CamAngles}} and {{CamDuration}} (0 = a cut).":
        "**{{CamDeleteShot}}** ho odebere. Pod **{{CamSelectedShot}}** zadáte přesné hodnoty: {{CamPosition}}, {{CamAngles}} a {{CamDuration}} (0 = střih).",
    "Moving and turning a shot":
        "Posun a otočení záběru",
    "The toolbar above the view switches the gizmo: **{{GizmoMove}}** shows three arrows and a box in the middle - drag an arrow to move along one axis, the box to move in the view plane. **{{GizmoRotate}}** shows three rings for pitch, yaw and roll. **{{GizmoSpaceWorld}} / {{GizmoSpaceLocal}}** decides whether the arrows follow the map or the camera. Letting go writes the new values into the script - one undo step.":
        "Lišta nad pohledem přepíná gizmo: **{{GizmoMove}}** ukáže tři šipky a rámeček uprostřed - tažením šipky posunete podél jedné osy, rámečkem v rovině pohledu. **{{GizmoRotate}}** ukáže tři kruhy pro sklon, otočení a náklon. **{{GizmoSpaceWorld}} / {{GizmoSpaceLocal}}** rozhoduje, zda šipky sledují mapu, nebo kameru. Puštěním se nové hodnoty zapíší do skriptu - jeden krok zpět.",
    "{{GizmoMove}}: arrows and the centre box.":
        "{{GizmoMove}}: šipky a rámeček uprostřed.",
    "{{GizmoRotate}}: three rings.":
        "{{GizmoRotate}}: tři kruhy.",
    "If the timeline stands **between** two keys when you grab the gizmo, a new key is created at that moment instead of changing the start - like the auto-key of animation programs. While piloting, <kbd>Enter</kbd> sets such a key too ([[kamera#steuern]]).":
        "Stojí-li časová osa při uchopení gizma **mezi** dvěma klíči, vytvoří se v tom okamžiku nový klíč, místo aby se měnil začátek - jako auto-klíč animačních programů. Při pilotování takový klíč nastaví i <kbd>Enter</kbd> ([[kamera#steuern]]).",
    "Camera paths":
        "Dráhy kamery",
    "The selected camera draws its path: already flown parts in yellow, the rest in grey. With many shots that gets crowded - **{{CamPathActive}}** shows only the move into the current shot and on to the next, **{{CamPathAll}}** shows every move. Both buttons sit in the camera panel.":
        "Vybraná kamera kreslí svou dráhu: už prolétnuté části žlutě, zbytek šedě. S mnoha záběry to houstne - **{{CamPathActive}}** ukáže jen pohyb do aktuálního záběru a dál k dalšímu, **{{CamPathAll}}** ukáže každý pohyb. Obě tlačítka jsou v panelu kamery.",
    "Fly a shot yourself (Pilot)":
        "Proletět záběr sami (Pilot)",
    "**{{CamPilot}}** puts you into the shot: hold the right mouse button to look around, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> to move, <kbd>Q</kbd> <kbd>E</kbd> for down and up. **{{CamPilotApply}}** writes the view into the script, <kbd>Esc</kbd> cancels. While piloting, move the timeline and press <kbd>Enter</kbd> (**{{CamPilotKey}}**) to set key after key along the way. While piloting, the view uses the shot's zoom and shows the game frame, so you frame the shot as the game will show it.":
        "**{{CamPilot}}** vás usadí do záběru: podržením pravého tlačítka myši se rozhlížíte, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> pohyb, <kbd>Q</kbd> <kbd>E</kbd> dolů a nahoru. **{{CamPilotApply}}** zapíše pohled do skriptu, <kbd>Esc</kbd> zruší. Při pilotování posouvejte časovou osu a stiskem <kbd>Enter</kbd> (**{{CamPilotKey}}**) nastavujte klíč za klíčem po cestě. Při pilotování používá pohled přiblížení záběru a ukazuje rámeček hry, takže záběr zarámujete tak, jak ho ukáže hra.",
    "Preview and looking through":
        "Náhled a pohled kamerou",
    "**{{CamPreview}}** (in the bar of the selected camera, top left in the view) shows a small picture of what the camera sees, bottom right in the view; it follows the gizmo while you drag. Click the picture to look through the camera full size; while you look through it, playing the timeline shows the cutscene as the game will. **{{MapThroughCam}}** in the same bar does this directly.":
        "**{{CamPreview}}** (v liště vybrané kamery, vlevo nahoře v pohledu) ukáže malý obrázek toho, co kamera vidí, vpravo dole v pohledu; při tažení sleduje gizmo. Kliknutím na obrázek se kamerou podíváte v plné velikosti; zatímco se jí díváte, přehrávání časové osy ukáže animační sekvenci tak, jak ji ukáže hra. **{{MapThroughCam}}** ve stejné liště to udělá přímo.",
    "**What the game shows.** The game computes a camera's field of view for its own window, so through the camera the picture sits in a frame with the game's aspect ratio: black outside, the cinematic bars and fades inside - exactly the picture the player sees. Set the aspect ratio and **cg_fovAspectAdjust** under **{{MapViewSettings}}** → **{{GameScreen}}**. Automatic follows the game rules in the preferences ([[einstellungen#prefs]]): Movie Duels runs at your desktop resolution with cg_fovAspectAdjust 1 (a ZOOM keeps its vertical angle, wide screens see more at the sides), OpenJK starts at 800x600 with 0 (the ZOOM is the horizontal angle). If unsure, type `cg_fovAspectAdjust` and `r_mode` in the game console. In the free view a light frame shows what a camera placed here would see (zoom 90); switch it off with **{{GameFrameFree}}**. Before `camera ( ENABLE )` and after `DISABLE` the game shows the player's view with cg_fov 80 - playback and the preview do the same in scripts that switch the camera themselves. `ROLL` tilts the picture, and `camera ( PATH, ... )` flies the recorded ROFF path, including its `cut`, `fov`, `fovzoom` and `fovaccel` notes.":
        "**Co ukazuje hra.** Hra počítá zorné pole kamery pro vlastní okno, takže pohledem kamery je obraz v rámečku s poměrem stran hry: venku černá, uvnitř filmové pruhy a zatmívání - přesně obraz, který vidí hráč. Poměr stran a **cg_fovAspectAdjust** nastavíte v **{{MapViewSettings}}** → **{{GameScreen}}**. Automaticky se řídí pravidly hry v předvolbách ([[einstellungen#prefs]]): Movie Duels běží v rozlišení vaší plochy s cg_fovAspectAdjust 1 (ZOOM si zachová svislý úhel, široké obrazovky vidí víc po stranách), OpenJK začíná na 800x600 s 0 (ZOOM je vodorovný úhel). Nejste-li si jistí, zadejte v konzoli hry `cg_fovAspectAdjust` a `r_mode`. Ve volném pohledu ukazuje světlý rámeček, co by viděla kamera umístěná zde (zoom 90); vypnete ho pomocí **{{GameFrameFree}}**. Před `camera ( ENABLE )` a po `DISABLE` ukazuje hra pohled hráče s cg_fov 80 - přehrávání a náhled dělají totéž ve skriptech, které kameru přepínají samy. `ROLL` naklání obraz a `camera ( PATH, ... )` prolétá zaznamenanou dráhu ROFF včetně jejích poznámek `cut`, `fov`, `fovzoom` a `fovaccel`.",
    "Looking through the camera while the timeline plays.":
        "Pohled kamerou během přehrávání časové osy.",
    "Some shots stand on a `ref_tag` or look at one. Then the panel says so, and moving the shot moves that ref_tag in the map - the script itself only names it.":
        "Některé záběry stojí na `ref_tag` nebo se na něj dívají. Panel to pak uvádí a posunutí záběru posune tento ref_tag v mapě - samotný skript ho jen jmenuje.",
    "Playing":
        "Přehrávání",
    "Sound like in the game":
        "Zvuk jako ve hře",
    "The preview plays the scene's sounds the way the game does. You hear from the camera the view shows - the cutscene camera while playing. Every sound is updated every frame: move the camera away and it fades, cut to another place and what is there becomes audible. A voice line that started out of range keeps running silently. Like the game, only sources the camera could see (its PVS) make sabers hum, speakers loop, doors and effects sound; voices, ROFF note sounds and breaking objects are always heard. An invisible NPC does not ignite its saber audibly - the ignition sound comes when it appears. The levels follow the game's defaults (effects 0.5, voices 1.0, music 0.25) and can be changed under **{{MenuEdit}} → {{AppPrefs}}**. Sound only plays at 1x speed.":
        "Náhled přehrává zvuky scény tak, jak to dělá hra. Slyšíte z kamery, kterou pohled ukazuje - při přehrávání z kamery animační sekvence. Každý zvuk se aktualizuje v každém snímku: odsuňte kameru a zeslábne, střihněte na jiné místo a uslyšíte, co je tam. Hlasová replika, která začala mimo dosah, běží dál potichu. Jako ve hře jen zdroje, které kamera mohla vidět (její PVS), nechají meče bzučet, reproduktory hrát ve smyčce, dveře a efekty znít; hlasy, zvuky poznámek ROFF a rozbíjené objekty jsou slyšet vždy. Neviditelné NPC nezapne svůj meč slyšitelně - zvuk zapnutí přijde, až se objeví. Hlasitosti se řídí výchozími hodnotami hry (efekty 0.5, hlasy 1.0, hudba 0.25) a lze je změnit v **{{MenuEdit}} → {{AppPrefs}}**. Zvuk hraje jen při rychlosti 1x.",
    "A ROFF `sound` note on a figure stays where it started under Movie Duels rules and moves with the figure under OpenJK rules ([[einstellungen#prefs]]). Under OpenJK rules every note of a ROFF frame fires (Movie Duels and Jedi Outcast fire only the first), and `loop sfx <sound>` gives the entity a looping sound until `loop sfx kill`; a `USE <script>` note starts that script on the entity, and `loop rof absolute|relative` restarts the path like the game (the task of the `play` never finishes, so a `dowait` on it waits forever).":
        "Poznámka ROFF `sound` na postavě zůstane podle pravidel Movie Duels tam, kde začala, a podle pravidel OpenJK se pohybuje s postavou ([[einstellungen#prefs]]). Podle pravidel OpenJK se spustí každá poznámka snímku ROFF (Movie Duels a Jedi Outcast spustí jen první) a `loop sfx <sound>` dá entitě zvuk ve smyčce až do `loop sfx kill`; poznámka `USE <script>` spustí tento skript na entitě a `loop rof absolute|relative` spustí dráhu znovu jako hra (úloha `play` nikdy neskončí, takže `dowait` na ni čeká navždy).",
    "Changing time by dragging":
        "Změna času tažením",
    "Figures and paths":
        "Postavy a dráhy",
    "Figures walk to their navgoals and ships fly their moves - you see the whole path and can change it on the map.":
        "Postavy chodí ke svým navgoalům a lodě létají svými pohyby - vidíte celou dráhu a můžete ji měnit v mapě.",
    "A walking figure: the path ahead in grey, the part already walked in yellow, the handles at start and end.":
        "Jdoucí postava: dráha před ní šedě, už prošlá část žlutě, úchyty na začátku a konci.",
    "Selecting figures and ships":
        "Výběr postav a lodí",
    "Click a figure or a ship in the view to select its entity - the spawner of the figure, the brush entity of the ship. Marker and gizmo follow the model while it moves, and an outline shows what is selected. The arrow at the figure points where the model looks.":
        "Kliknutím na postavu nebo loď v pohledu vyberete její entitu - spawner postavy, brush entitu lodi. Značka a gizmo sledují model, zatímco se pohybuje, a obrys ukazuje, co je vybrané. Šipka u postavy ukazuje, kam se model dívá.",
    "Start, end and new points":
        "Začátek, konec a nové body",
    "**Drag the end handle** to move the walk target (the `waypoint_navgoal`) or the end of a flight.":
        "**Táhněte úchyt konce** a posunete cíl chůze (`waypoint_navgoal`) nebo konec letu.",
    "**Drag the start handle** to move only the start, not the whole line.":
        "**Táhněte úchyt začátku** a posunete jen začátek, ne celou čáru.",
    "**Rotate** works the same way: in the middle of a flight the rings create a new point with the new angles. For a figure (NPC) they write `set ( SET_DYAW )` at that time - the figure turns with its turn rate, as in the game; for the player it stays `set ( SET_ANGLES )`, which turns at once. While walking it applies when the figure arrives, because the game turns walking figures toward their direction. If a watch target is active at that moment, the status line warns: in the game the figure keeps facing it.":
        "**Otočení** funguje stejně: uprostřed letu vytvoří kruhy nový bod s novými úhly. U postavy (NPC) zapíší v tom čase `set ( SET_DYAW )` - postava se otáčí svou rychlostí otáčení, jako ve hře; u hráče zůstane `set ( SET_ANGLES )`, které otočí okamžitě. Při chůzi platí po příchodu postavy, protože hra natáčí jdoucí postavy ve směru chůze. Je-li v tom okamžiku aktivní cíl sledování, stavový řádek varuje: ve hře se postava dál dívá na něj.",
    "Walk or run, shared targets":
        "Chůze nebo běh, sdílené cíle",
    "Right-click the end handle of a walk (or its band in the timeline) for its menu:":
        "Pravým tlačítkem na úchytu konce chůze (nebo na jejím pásu na časové ose) otevřete její nabídku:",
    "**{{BahnGehen}}** / **{{BahnRennen}}** - writes `SET_WALKING` or `SET_RUNNING` before the navgoal.":
        "**{{BahnGehen}}** / **{{BahnRennen}}** - zapíše před navgoal `SET_WALKING` nebo `SET_RUNNING`.",
    "**{{BahnAnsehen}}** - the figure keeps turning toward another figure while it walks (`SET_WATCHTARGET`).":
        "**{{BahnAnsehen}}** - postava se při chůzi stále otáčí k jiné postavě (`SET_WATCHTARGET`).",
    "**{{BahnRueckwaerts}}** - a look mark behind the start makes it walk backwards; **{{BahnBlickFrei}}** ends that.":
        "**{{BahnRueckwaerts}}** - značka pohledu za startem ji nechá jít pozpátku; **{{BahnBlickFrei}}** to ukončí.",
    "When several figures share one navgoal, moving it would move all of them. JK Studio notices and gives this walk its own copy of the target first - the status line tells you which one it created.":
        "Sdílí-li několik postav jeden navgoal, jeho posunutí by posunulo všechny. JK Studio si toho všimne a dá této chůzi nejdřív vlastní kopii cíle - stavový řádek vám řekne, kterou vytvořil.",
    "Walking backwards is detected in existing scripts too: when a figure moves against the way it faces, the scene plays `BOTH_WALKBACK1` or `BOTH_RUNBACK1` like the game.":
        "Chůze pozpátku se rozpozná i v existujících skriptech: když se postava pohybuje proti směru, kam je natočená, scéna přehraje `BOTH_WALKBACK1` nebo `BOTH_RUNBACK1` jako hra.",
    "How figures move - like in the game":
        "Jak se postavy pohybují - jako ve hře",
    "The preview moves NPCs by the game's own rules, frame by frame: steering, movement physics and paths. What you see is what the game does with your script:":
        "Náhled pohybuje NPC podle vlastních pravidel hry, snímek po snímku: řízení, fyzika pohybu a cesty. Co vidíte, je to, co hra s vaším skriptem udělá:",
    "**Speed.** A walk uses the speed that is set when the figure starts - also when `SET_WALKING`, `SET_RUNNING`, `SET_WALKSPEED` or `SET_RUNSPEED` come right after `SET_NAVGOAL`. A new speed during the walk changes the rest of the way. Without a speed in the script the values of the .npc file apply.":
        "**Rychlost.** Chůze používá rychlost nastavenou, když postava vyráží - i když `SET_WALKING`, `SET_RUNNING`, `SET_WALKSPEED` nebo `SET_RUNSPEED` přijdou hned po `SET_NAVGOAL`. Nová rychlost během chůze změní zbytek cesty. Bez rychlosti ve skriptu platí hodnoty ze souboru .npc.",
    "**Stopping.** `SET_NAVGOAL` with `NULL` stops the figure where it is; a `dowait` on the walk continues. A new navgoal during a walk starts from where the figure is.":
        "**Zastavení.** `SET_NAVGOAL` s `NULL` zastaví postavu tam, kde je; `dowait` na chůzi pokračuje. Nový navgoal během chůze začne od místa, kde postava je.",
    "**Turning.** NPCs turn with their turn rate: 60 + 3 × `SET_YAWSPEED` degrees per second (330 with the usual yawSpeed 90). `SET_DYAW`, `SET_VIEWTARGET` and `SET_WATCHTARGET` turn smoothly, `SET_ANGLES` turns at once. `rotate` and `move` do nothing on an NPC - in the game too. Difficulty and game rules change the rate of some classes ([[einstellungen#prefs]]).":
        "**Otáčení.** NPC se otáčejí svou rychlostí otáčení: 60 + 3 × `SET_YAWSPEED` stupňů za sekundu (330 s obvyklým yawSpeed 90). `SET_DYAW`, `SET_VIEWTARGET` a `SET_WATCHTARGET` otáčejí plynule, `SET_ANGLES` otočí okamžitě. `rotate` a `move` na NPC nic nedělají - ve hře také ne. Obtížnost a pravidla hry mění rychlost některých tříd ([[einstellungen#prefs]]).",
    "**Gravity.** Figures stand on the floor of the map, fall, climb steps, stick to slopes and ride lifts - with the map's `gravity` and `SET_GRAVITY`. A door, lift or train that a figure is in the way of stops like in the game: a door turns back (a crusher keeps pressing), a train waits, and the figure takes the door's or train's damage (`dmg`, 2 by default); a figure on a lift that would be pressed against something stays behind if it has room, otherwise it holds the lift (not for brushes moved by a script). A figure that is spawned when the map loads is still falling when the cutscene starts, like in the game. `DROPTOFLOOR` changes nothing in single player.":
        "**Gravitace.** Postavy stojí na podlaze mapy, padají, stoupají po schodech, drží se svahů a jezdí výtahy - s `gravity` mapy a `SET_GRAVITY`. Dveře, výtah nebo vlak, kterým postava stojí v cestě, se zastaví jako ve hře: dveře se vrátí (drtič dál tlačí), vlak čeká a postava utrží poškození dveří nebo vlaku (`dmg`, výchozí 2); postava na výtahu, která by byla přimáčknuta k něčemu, zůstane pozadu, má-li místo, jinak výtah zadrží (ne u brushů pohybovaných skriptem). Postava spawnutá při načtení mapy ještě padá, když animační sekvence začne, jako ve hře. `DROPTOFLOOR` ve hře pro jednoho hráče nic nemění.",
    "**Figures of the map.** Every character that spawns when the map loads (an `NPC_*` entity without `targetname`) stands in the preview, also when no script names it - like the droid next to Anakin in md_am_sith. A script tab leaves out the actors of other cutscenes: a figure with `NPC_targetname` and the CINEMATIC spawnflag does nothing on its own in the game, so it shows only in the tabs whose scripts use it (in md_am_sith the credits show Anakin `ani4`, not `ani1` to `ani5` of the other scenes). An empty tab shows the map as it loads, with every figure. Classes such as `NPC_Droid_R2D2`, `NPC_Kyle` or `NPC_Stormtrooper` pick their character type themselves, as in the game; where the game chooses at random (`NPC_Stormtrooper` without spawnflags, the Movie Duels random spawners), the preview shows the first choice.":
        "**Postavy mapy.** Každá postava, která se objeví při načtení mapy (entita `NPC_*` bez `targetname`), stojí v náhledu, i když ji žádný skript nejmenuje - jako droid vedle Anakina v md_am_sith. Karta skriptu vynechá herce jiných animačních sekvencí: postava s `NPC_targetname` a spawnflagem CINEMATIC ve hře sama nic nedělá, takže se ukazuje jen na kartách, jejichž skripty ji používají (v md_am_sith ukazují titulky Anakina `ani4`, ne `ani1` až `ani5` ostatních scén). Prázdná karta ukazuje mapu tak, jak se načte, se všemi postavami. Třídy jako `NPC_Droid_R2D2`, `NPC_Kyle` nebo `NPC_Stormtrooper` si svůj typ postavy vybírají samy, jako ve hře; kde hra volí náhodně (`NPC_Stormtrooper` bez spawnflags, náhodné spawnery Movie Duels), náhled ukazuje první volbu.",
    "**Paths.** A figure walks straight to its target only where the game would: close by (under 110 units), between neighbouring waypoints (under 500) or with a free line (under 400). Otherwise it walks over the waypoint network, point by point - connect your targets with waypoints ([[wegpunkte]]). Around corners a walk shows as several pieces of the same command.":
        "**Cesty.** Postava jde ke svému cíli rovně jen tam, kde by to dělala hra: blízko (pod 110 jednotek), mezi sousedními navigačními body (pod 500) nebo s volnou linií (pod 400). Jinak jde po síti navigačních bodů, bod po bodu - spojte své cíle navigačními body ([[wegpunkte]]). Za rohy se chůze ukazuje jako několik kusů téhož příkazu.",
    "**No path.** If the network has no path, the figure stands still and a `dowait` on the walk never ends - in the game too. The status list names the figure and the target.":
        "**Žádná cesta.** Nemá-li síť cestu, postava stojí a `dowait` na chůzi nikdy neskončí - ve hře také. Seznam stavů jmenuje postavu a cíl.",
    "**Doors.** Doors and other moving brushes stand where the game has them at that moment. An NPC waits at a closed door and walks on when it opens. A locked door (or one only the player can open) blocks the way while it is closed. Walls and doors that a script moves (`move`, `rotate`) or removes are where the script has them. A figure standing on a lift or platform that moves rides along with it, also while it walks, like the game.":
        "**Dveře.** Dveře a jiné pohyblivé brushe stojí tam, kde je hra v tu chvíli má. NPC čeká u zavřených dveří a jde dál, když se otevřou. Zamčené dveře (nebo ty, které může otevřít jen hráč) blokují cestu, dokud jsou zavřené. Zdi a dveře, které skript posouvá (`move`, `rotate`) nebo odebírá, jsou tam, kde je skript má. Postava stojící na pohybujícím se výtahu nebo plošině jede s ní, i když jde, jako ve hře.",
    "**Ledges, curves and other NPCs.** NPCs fall from ledges and land - with the game's in-air animation on a long fall and its landing animation where the game plays one (a hard landing slows them down), also when a figure was spawned in the air; a script animation on the upper body that began before a jump does not come back after it, like the game; curved surfaces (patches) block them like walls. They do not walk through each other: if two NPCs meet, the preview shows how the game lets them pass - and the times of `dowait` follow. As in the game, a figure made invisible (`SET_INVISIBLE true`) or not solid (`SET_SOLID false`) blocks nobody, figures that stand in the map without a script do, and a killed figure stays in the way until it becomes a corpse (5 s in Movie Duels, 0.5 s in OpenJK; a Mark1 or Interrogator never does). A figure made solid again (`SET_SOLID true`) while it stands inside another one becomes solid only once it is free - a `dowait` on that `set` waits as long as in the game. `SET_ORIGIN` and `SET_ANGLES` with `tag( \"name\", ORIGIN )` or `ANGLES` put the figure at the ref_tag or navgoal of that name; without such a tag the command does nothing, like the game. A script run by an entity (a script runner, a trigger, an NPC spawner) reads that entity's `parm1` ... `parm16` keys with `get( STRING, \"SET_PARM1\" )`, and inside an `affect` block `get` reads the parms of the affected entity - so `affect ( $get( STRING, \"SET_PARM1\" )$ )` and `move ( $tag( get( STRING, \"SET_PARM1\" ), ORIGIN )$, ... )` move the entity named there to the tag named there, like the game; `move` also takes `tag( \"name\", ORIGIN )` as its destination. NPCs that know force jump (FP_LEVITATION in their .npc) jump like the game: across jump edges of the waypoint net (targetJump) and toward their goal after being blocked for 2 seconds; the preview plays the game's animations - the jump animation (as they leave the ground the legs switch to the jump pose while the upper body finishes it; a flip stays whole-body) and the landing animation, also after a `BS_JUMP` jump that only lands after `BS_DEFAULT`. After `SET_NO_ACROBATICS true` an NPC never jumps (it still takes jump edges and stops at them, like the game). In the behavior state `BS_JUMP` an NPC turns to its navgoal, crouches, jumps there in one arc and lands, like the game; a `dowait` on the navgoal ends after the landing animation once it has reached the goal (if it lands short, it jumps again). The arc is the game's own: it falls short of a navgoal above the NPC and goes beyond one below it, and a navgoal that hangs in the air is only the point it aims at.":
        "**Římsy, zakřivení a jiná NPC.** NPC padají z říms a přistávají - s animací ve vzduchu hry při dlouhém pádu a s její animací přistání tam, kde ji hra přehraje (tvrdé přistání je zpomalí), i když byla postava spawnuta ve vzduchu; skriptová animace horní části těla, která začala před skokem, se po něm nevrátí, jako ve hře; zakřivené povrchy (patche) je blokují jako zdi. Neprocházejí skrz sebe: když se dvě NPC potkají, náhled ukáže, jak je hra nechá projít - a časy `dowait` to následují. Jako ve hře postava zneviditelněná (`SET_INVISIBLE true`) nebo nepevná (`SET_SOLID false`) nikoho neblokuje, postavy stojící v mapě bez skriptu ano a zabitá postava zůstane v cestě, dokud se nestane mrtvolou (5 s v Movie Duels, 0.5 s v OpenJK; Mark1 nebo Interrogator nikdy). Postava znovu zpevněná (`SET_SOLID true`), zatímco stojí uvnitř jiné, se stane pevnou až tehdy, když je volná - `dowait` na tento `set` čeká stejně dlouho jako ve hře. `SET_ORIGIN` a `SET_ANGLES` s `tag( \"name\", ORIGIN )` nebo `ANGLES` umístí postavu na ref_tag nebo navgoal tohoto názvu; bez takového tagu příkaz nic nedělá, jako ve hře. Skript spuštěný entitou (spouštěčem skriptu, triggerem, spawnerem NPC) čte klíče `parm1` ... `parm16` této entity pomocí `get( STRING, \"SET_PARM1\" )` a uvnitř bloku `affect` čte `get` parametry ovlivňované entity - takže `affect ( $get( STRING, \"SET_PARM1\" )$ )` a `move ( $tag( get( STRING, \"SET_PARM1\" ), ORIGIN )$, ... )` posunou entitu tam jmenovanou na tag tam jmenovaný, jako hra; `move` bere jako cíl i `tag( \"name\", ORIGIN )`. NPC, která umějí skok Silou (FP_LEVITATION ve svém .npc), skáčou jako ve hře: přes hrany skoků sítě navigačních bodů (targetJump) a ke svému cíli po 2 sekundách zablokování; náhled přehrává animace hry - animaci skoku (při odrazu od země přepnou nohy do pózy skoku, zatímco horní část těla ji dokončí; salto zůstává celotělové) a animaci přistání, i po skoku `BS_JUMP`, který přistane až po `BS_DEFAULT`. Po `SET_NO_ACROBATICS true` NPC nikdy neskočí (hrany skoků stále bere a zastaví se u nich, jako ve hře). Ve stavu chování `BS_JUMP` se NPC otočí ke svému navgoalu, přikrčí se, skočí tam jedním obloukem a přistane, jako ve hře; `dowait` na navgoal skončí po animaci přistání, jakmile dosáhne cíle (přistane-li kratší, skočí znovu). Oblouk je vlastní oblouk hry: u navgoalu nad NPC dopadne kratší a u navgoalu pod ním přeletí dál, a navgoal visící ve vzduchu je jen bod, na který míří.",
    "**Deadly areas.** A figure that walks or falls into a `trigger_hurt` takes its damage like in the game (`dmg`, `delay`, `wait`, the spawnflags; a `use` switches the area on or off) and dies when its health runs out: it plays its death, stays where it fell and its `deathscript` runs. The health is the one the game gives it (the `health` key, otherwise the .npc file and the difficulty), `SET_HEALTH` changes it, armour (`SET_ARMOR`, the assassin droid's and droideka's own) takes its share like the game's armour (not in a fall), a rancor takes at most 10 per hit, `SET_INVINCIBLE true` protects it, and with `SET_UNDYING true` it keeps 1 health and its `deathscript` runs again with every hit that would kill it. Exploding laser trip mines hurt figures in the same way. The player is not hurt in the preview.":
        "**Smrtící oblasti.** Postava, která vejde nebo spadne do `trigger_hurt`, utrží jeho poškození jako ve hře (`dmg`, `delay`, `wait`, spawnflags; `use` oblast zapíná nebo vypíná) a zemře, když jí dojde zdraví: přehraje svou smrt, zůstane tam, kde padla, a spustí se její `deathscript`. Zdraví je to, které jí dá hra (klíč `health`, jinak soubor .npc a obtížnost), `SET_HEALTH` ho mění, brnění (`SET_ARMOR`, vlastní brnění droida zabijáka a droideky) bere svůj podíl jako brnění ve hře (ne při pádu), rankor utrží nejvýš 10 na zásah, `SET_INVINCIBLE true` ji chrání a s `SET_UNDYING true` si ponechá 1 zdraví a její `deathscript` se spustí znovu s každým zásahem, který by ji zabil. Explodující laserové miny zraňují postavy stejně. Hráč se v náhledu nezraní.",
    "**Level change.** When a script uses a `target_level_change` (directly or through a relay), the game loads the next map after that frame. The preview ends there too: the timeline, the camera and the figures stop at that moment, and nothing the script would do later happens. The status list names the time and the next map. A map name starting with `+` (OpenJK and Movie Duels, for example `+saberMenu` or `+creditsingame`) loads no map: the game opens that menu and pauses until the player closes it, so the preview goes on and the status list names the menu.":
        "**Změna úrovně.** Když skript použije `target_level_change` (přímo nebo přes relay), hra po tomto snímku načte další mapu. Náhled tam také končí: časová osa, kamera a postavy se v tom okamžiku zastaví a nic, co by skript udělal později, se nestane. Seznam stavů uvádí čas a další mapu. Název mapy začínající `+` (OpenJK a Movie Duels, například `+saberMenu` nebo `+creditsingame`) žádnou mapu nenačte: hra otevře toto menu a pozastaví se, dokud ho hráč nezavře, takže náhled pokračuje a seznam stavů menu jmenuje.",
    "**Mission failed.** `SET_MISSIONFAILED` makes the player die and shows the game's mission-failed screen. The game then stops every script as soon as no camera is running (right after that frame, or after the cutscene's `camera ( DISABLE )`); the world goes on. The preview does the same: script commands after that moment do not happen, figures already walking finish their walk, the player plays his guilt animation, and the status list names both times.":
        "**Mise selhala.** `SET_MISSIONFAILED` nechá hráče zemřít a ukáže obrazovku neúspěchu mise. Hra pak zastaví každý skript, jakmile neběží žádná kamera (hned po tomto snímku, nebo po `camera ( DISABLE )` animační sekvence); svět běží dál. Náhled dělá totéž: příkazy skriptu po tomto okamžiku se nestanou, postavy, které už jdou, dojdou, hráč přehraje svou animaci viny a seznam stavů uvede oba časy.",
    "**Weapon poses.** An NPC with a gun in its hand (not a lightsaber) holds it with its upper body like the game whenever no script animation plays there: under OpenJK and Jedi Outcast a rifle at the ready while it stands and walks (`TORSO_WEAPONIDLE3`), a pistol in front while it stands, and the arms swing with the legs while it runs; under Movie Duels a rifle lowered (`TORSO_WEAPONREST3`) or carried after `SET_WALKING true` (`TORSO_WEAPONIDLE3`). A script animation on the upper or the whole body wins while its timer runs.":
        "**Pózy se zbraní.** NPC se střelnou zbraní v ruce (ne se světelným mečem) ji drží horní částí těla jako ve hře, kdykoli tam nehraje skriptová animace: v OpenJK a Jedi Outcast pušku v pohotovosti, když stojí a jde (`TORSO_WEAPONIDLE3`), pistoli před sebou, když stojí, a při běhu paže kmitají s nohama; v Movie Duels pušku sklopenou (`TORSO_WEAPONREST3`) nebo nesenou po `SET_WALKING true` (`TORSO_WEAPONIDLE3`). Skriptová animace horní části nebo celého těla vyhrává, dokud běží její časovač.",
    "**BS_NOCLIP.** With `SET_BEHAVIOR_STATE BS_NOCLIP` a figure flies straight through walls to its goal, with the speed it has at that moment. Set `SET_WALKSPEED` or `SET_RUNSPEED` after `BS_NOCLIP`, otherwise a figure that has not walked yet does not move (and a `dowait` hangs).":
        "**BS_NOCLIP.** S `SET_BEHAVIOR_STATE BS_NOCLIP` letí postava rovně skrz zdi ke svému cíli rychlostí, kterou má v tu chvíli. Po `BS_NOCLIP` nastavte `SET_WALKSPEED` nebo `SET_RUNSPEED`, jinak se postava, která ještě nešla, nepohne (a `dowait` uvízne).",
    "**OpenJK or Movie Duels.** Under OpenJK rules NPCs re-steer and turn every 100 ms (higher-ranked saber users from {{DiffKnight}} on every 50 ms) - turning therefore takes about twice as long as under Movie Duels, where both happen every 50 ms - and the turn rates per difficulty differ, so a walk can end a little differently. Jedi Outcast missions follow the same 100 ms rhythm as OpenJK, also when an NPC turns on the spot. Choose the rules in [[einstellungen#prefs]].":
        "**OpenJK, nebo Movie Duels.** Podle pravidel OpenJK NPC přeřizují a otáčejí se každých 100 ms (výše postavení šermíři od {{DiffKnight}} každých 50 ms) - otáčení proto trvá asi dvakrát déle než v Movie Duels, kde se obojí děje každých 50 ms - a rychlosti otáčení podle obtížnosti se liší, takže chůze může skončit trochu jinak. Mise Jedi Outcast se řídí stejným rytmem 100 ms jako OpenJK, i když se NPC otáčí na místě. Pravidla zvolíte v [[einstellungen#prefs]].",
    "Lightsabers: walls and clashes":
        "Světelné meče: zdi a střety",
    "Lightsabers behave like in the game. A blade that reaches into a wall, the floor or a door is shortened to the surface and grows back; sparks fly and it sounds while it touches, and where it slides along the wall it burns a mark that glows briefly and fades after ten seconds. In water it boils. Sabers whose .sab file says `noIdleEffect` stay quiet while the figure only stands. Only figures the camera can see make sparks, marks and sounds - the game, too, only handles what is in the camera's view. Jumping on the timeline shows the same as playing through, and dragging the playhead backwards is as smooth as forwards: what has been computed once stays in memory (up to ten minutes of a scene).":
        "Světelné meče se chovají jako ve hře. Čepel, která zasahuje do zdi, podlahy nebo dveří, se zkrátí k povrchu a zase doroste; při dotyku létají jiskry a zní to, a kde klouže po zdi, vypaluje stopu, která krátce žhne a po deseti sekundách zmizí. Ve vodě vře. Meče, jejichž soubor .sab uvádí `noIdleEffect`, zůstávají tiché, dokud postava jen stojí. Jiskry, stopy a zvuky dělají jen postavy, které kamera vidí - i hra zpracovává jen to, co je v zorném poli kamery. Skok na časové ose ukáže totéž jako přehrání a tažení ukazatele přehrávání dozadu je stejně plynulé jako dopředu: co se jednou spočítalo, zůstane v paměti (až deset minut scény).",
    "Blades of two figures clash only where the game lets them. **Movie Duels:** both NPCs must be in `BS_CINEMATIC` (`set ( SET_BEHAVIOR_STATE, BS_CINEMATIC )`, or a spawner with the CINEMATIC spawnflag), and the attacker needs `set ( SET_SABERCOLLISIONS, true )`; `false` switches it off again. **OpenJK** has no `SET_SABERCOLLISIONS`: there blades clash only in the animations `BOTH_CIN_16` and `BOTH_CIN_17` of figures in `BS_CINEMATIC` (Movie Duels allows these two as well). A hit plays the block spark, one of that game's clash sounds and the clash flare. Fights the AI would fight on its own (`SET_ENEMY`, `BS_DEFAULT`) are not simulated - only what the script animates. In Movie Duels the fast saber style also ignites the blade twice as fast. The rules are chosen in [[einstellungen#prefs]].":
        "Čepele dvou postav se střetnou jen tam, kde to hra dovolí. **Movie Duels:** obě NPC musí být v `BS_CINEMATIC` (`set ( SET_BEHAVIOR_STATE, BS_CINEMATIC )`, nebo spawner se spawnflagem CINEMATIC) a útočník potřebuje `set ( SET_SABERCOLLISIONS, true )`; `false` to zase vypne. **OpenJK** nemá `SET_SABERCOLLISIONS`: tam se čepele střetávají jen v animacích `BOTH_CIN_16` a `BOTH_CIN_17` postav v `BS_CINEMATIC` (Movie Duels tyto dvě také povoluje). Zásah přehraje jiskru bloku, jeden ze zvuků střetu dané hry a záblesk střetu. Souboje, které by AI vybojovala sama (`SET_ENEMY`, `BS_DEFAULT`), se nesimulují - jen to, co animuje skript. V Movie Duels rychlý styl meče také zapne čepel dvakrát rychleji. Pravidla se volí v [[einstellungen#prefs]].",
    "Animating movers and ROFF files":
        "Animace moverů a soubory ROFF",
    "Ships, doors and platforms move by keys over time. Store them as move commands in the script, or as a ROFF file - and edit the ROFF files of the game.":
        "Lodě, dveře a plošiny se pohybují podle klíčů v čase. Uložte je jako příkazy move ve skriptu, nebo jako soubor ROFF - a upravujte soubory ROFF hry.",
    "The {{AnimTitle}} window in ROFF mode and the path of the ship on the map.":
        "Okno {{AnimTitle}} v režimu ROFF a dráha lodi v mapě.",
    "Opening the window":
        "Otevření okna",
    "Select a movable entity - `func_*` brush entities such as `func_static` ships and `func_door`, `misc_model_breakable`, `fx_runner` - and press **{{EntAnimate}}** in the inspector. If it has no `script_targetname` yet, it gets one, because a script can only move what has a name.":
        "Vyberte pohyblivou entitu - brush entity `func_*` jako lodě `func_static` a `func_door`, `misc_model_breakable`, `fx_runner` - a stiskněte v inspektoru **{{EntAnimate}}**. Pokud ještě nemá `script_targetname`, dostane ho, protože skript může pohybovat jen tím, co má název.",
    "Keys":
        "Klíče",
    "Put the timeline where the movement starts and press **{{AnimAddKey}}** - the first key stores where the entity stands.":
        "Nastavte časovou osu tam, kde pohyb začíná, a stiskněte **{{AnimAddKey}}** - první klíč uloží, kde entita stojí.",
    "Move on in time and press it again. Then drag the new diamond in the view (<kbd>Shift</kbd>: only up and down), or type values into the table: time, position, direction.":
        "Posuňte se v čase a stiskněte ho znovu. Pak táhněte nový kosočtverec v pohledu (<kbd>Shift</kbd>: jen nahoru a dolů), nebo zadejte hodnoty do tabulky: čas, poloha, směr.",
    "Or simply use the gizmo: while the window shows this entity and the timeline stands at or after the first key, moving or rotating it with the gizmo sets a key at the playhead (or changes the one that is there).":
        "Nebo prostě použijte gizmo: dokud okno ukazuje tuto entitu a časová osa stojí na prvním klíči nebo za ním, posun nebo otočení gizmem nastaví klíč u ukazatele přehrávání (nebo změní ten, který tam je).",
    "**{{AnimDeleteKey}}** and **{{AnimGotoKey}}** work on the selected row; **{{AnimRemove}}** takes the whole animation out of the script.":
        "**{{AnimDeleteKey}}** a **{{AnimGotoKey}}** pracují s vybraným řádkem; **{{AnimRemove}}** vyjme ze skriptu celou animaci.",
    "Script (move) or ROFF file":
        "Skript (move), nebo soubor ROFF",
    "Stored as":
        "Uloženo jako",
    "an `affect` block with `move` and `wait` at the start of the script":
        "blok `affect` s `move` a `wait` na začátku skriptu",
    "`scripts/<name>.rof`, played by `play ( \"PLAY_ROFF\", ... )`":
        "`scripts/<name>.rof`, přehrávaný pomocí `play ( \"PLAY_ROFF\", ... )`",
    "Between keys":
        "Mezi klíči",
    "each move starts fast and slows down at the end, like the game's movers (or constant with {{AnimLinear}})":
        "každý pohyb začne rychle a na konci zpomalí, jako movery hry (nebo stálá rychlost s {{AnimLinear}})",
    "one smooth curve through all keys ({{AnimRoffSmooth}}) or straight lines":
        "jedna hladká křivka přes všechny klíče ({{AnimRoffSmooth}}) nebo přímé čáry",
    "Good for":
        "Vhodné pro",
    "doors, lifts, a ship crossing the screen":
        "dveře, výtahy, loď přelétající přes obrazovku",
    "long free flights with many turns, effects at exact frames":
        "dlouhé volné lety s mnoha zatáčkami, efekty v přesných snímcích",
    "Switch with **{{AnimModeMove}}** / **{{AnimModeRoff}}** at the top of the window - the keys are converted, nothing is lost. In ROFF mode the field **{{AnimRoffName}}** names the file (for example `md_am/roffs/sta2`); below it you see how many frames and seconds it has.":
        "Přepínejte pomocí **{{AnimModeMove}}** / **{{AnimModeRoff}}** nahoře v okně - klíče se převedou, nic se neztratí. V režimu ROFF pole **{{AnimRoffName}}** jmenuje soubor (například `md_am/roffs/sta2`); pod ním vidíte, kolik má snímků a sekund.",
    "How ROFF files work":
        "Jak fungují soubory ROFF",
    "A ROFF file (`.rof`, \"Rotation and Origin File Format\") stores one small step per game frame: how far the entity moves and turns in the next 50 ms. The game adds these steps to wherever the entity stands when `PLAY_ROFF` runs - so a ROFF path is **relative**: move the entity and the whole path moves with it. All 108 files of the game and Movie Duels are version 2, all but one with 50 ms per frame - and that is what JK Studio writes.":
        "Soubor ROFF (`.rof`, \"Rotation and Origin File Format\") ukládá jeden malý krok na herní snímek: jak daleko se entita v dalších 50 ms posune a otočí. Hra tyto kroky přičítá k místu, kde entita stojí, když běží `PLAY_ROFF` - dráha ROFF je tedy **relativní**: posuňte entitu a celá dráha se posune s ní. Všech 108 souborů hry a Movie Duels má verzi 2, všechny kromě jednoho s 50 ms na snímek - a to JK Studio zapisuje.",
    "The keys you edit are kept as a comment line in the script right before the `play`; the `.rof` is built from them. That is why undo, copy and paste work on ROFF paths like on any command. When you **save the script**, the file is written to `scripts/<name>.rof` in the same scripts folder; **{{MissionSave}}** packs it into the mission's `.pk3`.":
        "Klíče, které upravujete, se uchovávají jako řádek komentáře ve skriptu těsně před `play`; `.rof` se z nich sestaví. Proto Zpět, kopírování a vkládání fungují na dráhách ROFF jako na každém příkazu. Když **uložíte skript**, soubor se zapíše do `scripts/<name>.rof` ve stejné složce skriptů; **{{MissionSave}}** ho zabalí do `.pk3` mise.",
    "Editing a ROFF file of the game":
        "Úprava souboru ROFF hry",
    "When a script plays a finished ROFF on the entity, the window lists it under **{{AnimRoffPlays}}**. **{{AnimRoffEdit}}** turns it into keys - either straight pieces or one smooth curve, whichever needs fewer keys - within half a unit of the original. The game's file is never overwritten: you edit a copy (`…_2`), and the `play` command is pointed at it.":
        "Když skript na entitě přehrává hotový ROFF, okno ho vypíše pod **{{AnimRoffPlays}}**. **{{AnimRoffEdit}}** ho převede na klíče - buď přímé úseky, nebo jednu hladkou křivku, podle toho, co potřebuje méně klíčů - s odchylkou do půl jednotky od originálu. Soubor hry se nikdy nepřepíše: upravujete kopii (`…_2`) a příkaz `play` se přesměruje na ni.",
    "A ship flying its ROFF path.":
        "Loď letící po své dráze ROFF.",
    "The map":
        "Mapa",
    "The map in 3D, drawn like the game draws it - with lights, glow, fog, effects and the figures of the script.":
        "Mapa ve 3D, kreslená tak, jak ji kreslí hra - se světly, září, mlhou, efekty a postavami skriptu.",
    "The map view with its toolbar on top and the placement tiles on the left.":
        "Zobrazení mapy s lištou nahoře a dlaždicemi pro umisťování vlevo.",
    "Opening a map":
        "Otevření mapy",
    "Switch to **{{ViewMap}}** in the mode bar. A mission brings its map along ([[missionen]]). Without a mission, **{{MapMore}}** opens a map from disk or from a `.pk3`, or loads a separate `.ent` file. **Scripts (N)** lists the scripts the map's entities point to - pick one to load its cutscene.":
        "Přepněte na **{{ViewMap}}** v liště režimů. Mise si mapu přinese s sebou ([[missionen]]). Bez mise otevře **{{MapMore}}** mapu z disku nebo z `.pk3`, nebo načte samostatný soubor `.ent`. **Skripty (N)** vypíše skripty, na které ukazují entity mapy - vyberte jeden a načtete jeho animační sekvenci.",
    "Moving around":
        "Pohyb",
    "W A S D (mouse over the view)":
        "W A S D (myš nad pohledem)",
    "Fly; <kbd>Q</kbd> / <kbd>E</kbd> down and up, <kbd>Shift</kbd> faster":
        "Let; <kbd>Q</kbd> / <kbd>E</kbd> dolů a nahoru, <kbd>Shift</kbd> rychleji",
    "Right mouse drag":
        "Tažení pravým tlačítkem",
    "Look around":
        "Rozhlížení",
    "Middle mouse drag":
        "Tažení prostředním tlačítkem",
    "Pan":
        "Posun",
    "Alt + middle mouse":
        "Alt + prostřední tlačítko",
    "Orbit":
        "Kroužení",
    "Wheel":
        "Kolečko",
    "Zoom":
        "Přiblížení",
    "Double-click an entity in the list":
        "Dvojklik na entitu v seznamu",
    "Fly there":
        "Přeletět tam",
    "Ctrl + 1 ... 9":
        "Ctrl + 1 ... 9",
    "Save the view; <kbd>1</kbd> ... <kbd>9</kbd> jumps back (kept per map)":
        "Uložit pohled; <kbd>1</kbd> ... <kbd>9</kbd> skočí zpět (pro každou mapu)",
    "What is shown":
        "Co se zobrazuje",
    "The buttons on the right of the toolbar switch parts of the picture on and off: entities, names, the figures of the script, effects, glow, dynamic light, fog and sky. **{{MapTopView}}** looks straight down like a map and cuts away everything above the cut height - drag the height box to look at another floor. **{{MapViewSettings}}** (gear) holds brightness, ambient light, detail, character shadows, **{{MapScriptFocus}}**, which dims every entity the active script does not use, and **{{GameScreen}}**: the game's aspect ratio and cg_fovAspectAdjust, which decide how wide every camera sees ([[kamera#vorschau]]).":
        "Tlačítka vpravo na liště zapínají a vypínají části obrazu: entity, názvy, postavy skriptu, efekty, záři, dynamické světlo, mlhu a oblohu. **{{MapTopView}}** se dívá přímo dolů jako mapa a odřízne vše nad výškou řezu - tažením políčka výšky se podíváte na jiné patro. **{{MapViewSettings}}** (ozubené kolo) obsahuje jas, okolní světlo, detail, stíny postav, **{{MapScriptFocus}}**, které ztlumí každou entitu, kterou aktivní skript nepoužívá, a **{{GameScreen}}**: poměr stran hry a cg_fovAspectAdjust, které rozhodují, jak široce každá kamera vidí ([[kamera#vorschau]]).",
    "The picture is meant to look like the game: shaders, lightmaps, glow (like `r_DynamicGlow` - on the map, on brush entities and models, on the figures' glowing parts such as droid eyes, on effects like fire and sparks and around lightsaber blades), fog (`r_drawfog`), dynamic lights, lens flares, grass, bushes and rain splashes on the ground (`surfaceSprites` - blade for blade where the game puts them, swaying and fading out with distance, also on brush entities such as a wall that appears when it is used; rain splashes only while it rains), weather (under Movie Duels also the lava of fx_lava) and saber trails, sparks and burn marks ([[figuren#schwerter]]). Weapons, medpacks, shields and ammo lie where the game drops them (items with a `targetname` appear only when used), and floor charging stations, security panels, wall cameras, turrets, emplaced guns, E-Webs, sentry guns and the ion cannon stand with the models the game gives them (without a player they do not aim or fire; a sentry gun unfolds one second after the map starts or after it is switched on and then slowly sweeps from side to side like the game's sentry without a target, and the ion cannon fires on its own - or after a use - with its muzzle flash and recoil, at twice its model size like in the game); laser trip mines hang on their wall and, when switched on, draw their beam and hum (with the warning sound when they arm); a script or trigger switches them on or off, the beam stops at doors and other moving parts where they are at that moment, at other trip mines and at solid breakable models, and a figure walking into the beam - or only the weapon in its hand - sets the mine off - explosion, then the mine is gone; like the game's blast damage it hurts the figures near it (the closer, the more; not through walls), and a figure whose health runs out dies like in a deadly area ([[figuren]]); like in the game the mine uses its targets, so doors, walls or a counter behind it switch, and scripts behind it run (the usescript of a counter, a script runner) - with one count for scripts and movers. An exploding mine also sets off the switched-on mines near it (mines hung close together go off together). Figures show every stage of their shader - shine, chrome (also on the weapon or saber hilt in their hand, turning with the figure as in the game), tinted parts such as the battle droids' paint, the black visors of the clone troopers, blinking and animated stages, moving texture stages (tcMod) such as the shimmer of the Emperor's hologram, and surfaces their shader moves with `deformVertexes` wave, bulge or move, such as the smoke around the shadow Yoda (md_yoda_shadow). Effects are drawn with every stage of their shader too - an explosion blends its two animated stages as in the game, and every stage, the first one included, takes the particle's colour and fading only where its shader asks for it (`rgbGen vertex`, `alphaGen vertex`); otherwise it shows its image as it is, like the game; an effect image without a shader script is drawn like the game's built-in shader for it - blended by its alpha with the particle's colour and fading, and visible through walls. Models that effects carry (`useModel`, such as the fog wisps on duel_exegol) are drawn too and turn towards the camera where their shader says `deformVertexes autosprite`. Beams of fx_target_beam fire towards their target like in the game, also when a script or trigger switches them on or off (a one-shot beam fires once per use); each shot aims at where the target is at that moment - a beam on a moving train follows it -, stops at doors, moving parts and figures in its way, and a beam switched back on waits out its pause (`wait`) like the game. **{{MapStats}}** shows frame rate, textures and graphics card values when something looks wrong.":
        "Obraz má vypadat jako hra: shadery, lightmapy, záře (jako `r_DynamicGlow` - na mapě, na brush entitách a modelech, na zářících částech postav jako oči droidů, na efektech jako oheň a jiskry a kolem čepelí světelných mečů), mlha (`r_drawfog`), dynamická světla, odlesky čočky, tráva, keře a cákance deště na zemi (`surfaceSprites` - stéblo po stéblu tam, kam je dává hra, kývající se a se vzdáleností mizející, i na brush entitách jako zeď, která se objeví, když je použita; cákance deště jen když prší), počasí (v Movie Duels i láva z fx_lava) a stopy, jiskry a spáleniny mečů ([[figuren#schwerter]]). Zbraně, lékárničky, štíty a munice leží tam, kam je hra pokládá (předměty s `targetname` se objeví, až když jsou použity), a podlahové nabíjecí stanice, bezpečnostní panely, nástěnné kamery, věže, stacionární zbraně, E-Weby, strážní děla a iontové dělo stojí s modely, které jim dává hra (bez hráče nemíří ani nestřílejí; strážní dělo se rozloží sekundu po startu mapy nebo po zapnutí a pak se pomalu otáčí ze strany na stranu jako strážní dělo hry bez cíle, a iontové dělo střílí samo - nebo po use - se zábleskem z hlavně a zpětným rázem, ve dvojnásobné velikosti modelu jako ve hře); laserové miny visí na své zdi a po zapnutí kreslí svůj paprsek a bzučí (s varovným zvukem při aktivaci); skript nebo trigger je zapíná a vypíná, paprsek se zastaví na dveřích a jiných pohyblivých částech tam, kde v tu chvíli jsou, na jiných laserových minách a na pevných rozbitných modelech, a postava, která vejde do paprsku - nebo jen zbraň v její ruce - minu odpálí - výbuch, pak mina zmizí; jako tlakové poškození ve hře zraňuje postavy poblíž (čím blíž, tím víc; ne skrz zdi) a postava, které dojde zdraví, zemře jako ve smrtící oblasti ([[figuren]]); jako ve hře mina použije své cíle, takže dveře, zdi nebo počítadlo za ní se přepnou a skripty za ní se spustí (usescript počítadla, spouštěč skriptu) - s jedním počtem pro skripty a movery. Explodující mina odpálí i zapnuté miny poblíž (miny zavěšené blízko sebe vybuchnou spolu). Postavy ukazují každou vrstvu svého shaderu - lesk, chrom (i na zbrani nebo jílci meče v ruce, otáčející se s postavou jako ve hře), tónované části jako nátěr bitevních droidů, černá hledí klonových vojáků, blikající a animované vrstvy, pohyblivé vrstvy textur (tcMod) jako třpyt Imperátorova hologramu a povrchy, které jejich shader pohybuje pomocí `deformVertexes` wave, bulge nebo move, jako kouř kolem stínového Yody (md_yoda_shadow). Efekty se kreslí také s každou vrstvou svého shaderu - výbuch prolíná své dvě animované vrstvy jako ve hře a každá vrstva, včetně první, přebírá barvu a mizení částice jen tam, kde to její shader žádá (`rgbGen vertex`, `alphaGen vertex`); jinak ukazuje svůj obrázek, jak je, jako hra; obrázek efektu bez skriptu shaderu se kreslí jako vestavěný shader hry pro něj - prolnutý podle své alfy s barvou a mizením částice a viditelný skrz zdi. Modely, které efekty nesou (`useModel`, jako cáry mlhy na duel_exegol), se kreslí také a otáčejí se ke kameře, kde jejich shader říká `deformVertexes autosprite`. Paprsky fx_target_beam střílejí ke svému cíli jako ve hře, i když je skript nebo trigger zapíná a vypíná (jednorázový paprsek vystřelí jednou na použití); každý výstřel míří tam, kde cíl v tu chvíli je - paprsek na jedoucím vlaku ho sleduje -, zastaví se na dveřích, pohyblivých částech a postavách v cestě a znovu zapnutý paprsek vyčká svou pauzu (`wait`) jako hra. **{{MapStats}}** ukáže snímkovou frekvenci, textury a hodnoty grafické karty, když něco vypadá špatně.",
    "Entities":
        "Entity",
    "Everything in a map that a script can talk to - figures, walk targets, camera marks, triggers, ships - is an entity. Select it, change it, move it, place new ones.":
        "Vše v mapě, s čím skript může mluvit - postavy, cíle chůze, značky kamery, triggery, lodě - je entita. Vyberte ji, změňte ji, posuňte ji, umisťujte nové.",
    "A selected ship: the list on the left, the inspector below it, the gizmo in the view.":
        "Vybraná loď: seznam vlevo, inspektor pod ním, gizmo v pohledu.",
    "The entity list":
        "Seznam entit",
    "The left column sorts the entities into groups - camera marks, walk targets, look targets, characters, script runners, effects, sound, movables, breakables, NPC waypoints. The eye hides a group in the view (<kbd>Alt</kbd>+click: show only this one), the search box filters by name or class. Click selects, double-click flies there, right-click inserts a command for it: walk target, look target, affect block, camera to this mark.":
        "Levý sloupec řadí entity do skupin - značky kamery, cíle chůze, cíle pohledu, postavy, spouštěče skriptů, efekty, zvuk, pohyblivé, rozbitné, navigační body NPC. Oko skryje skupinu v pohledu (<kbd>Alt</kbd>+kliknutí: zobrazit jen tuto), vyhledávací pole filtruje podle názvu nebo třídy. Kliknutí vybírá, dvojklik tam přeletí, pravé tlačítko pro ni vloží příkaz: cíl chůze, cíl pohledu, blok affect, kamera k této značce.",
    "Selecting in the view":
        "Výběr v pohledu",
    "Click a marker, a figure or a model to select it; <kbd>Ctrl</kbd>/<kbd>Shift</kbd>+click adds more; a box dragged over empty space selects everything inside. Hovering shows a box and the name. The transport bar under the map says how often the selected entity is used in the script, with buttons that jump to those lines.":
        "Kliknutím na značku, postavu nebo model ji vyberete; <kbd>Ctrl</kbd>/<kbd>Shift</kbd>+kliknutí přidá další; rámeček tažený přes prázdné místo vybere vše uvnitř. Při najetí myší se ukáže rámeček a název. Lišta přehrávání pod mapou říká, jak často se vybraná entita ve skriptu používá, s tlačítky, která skočí na tyto řádky.",
    "Move and Rotate":
        "Posun a otočení",
    "A small bar in the corner of the view switches the gizmo between **{{GizmoMove}}** and **{{GizmoRotate}}**. In Move you drag the entity over the floor or along one of the coloured arrows; the grid set in the toolbar snaps, <kbd>Ctrl</kbd> turns it off while you drag - without a grid, <kbd>Ctrl</kbd> snaps an arrow drag to 8 units. In Rotate you get rings (<kbd>Ctrl</kbd>: 15-degree steps): blue turns left and right; ships and models also get green (pitch) and red (roll); figures only have the blue ring, because the game only knows `angle` for them. <kbd>Alt</kbd>+drag copies first. Several selected entities move and turn as a group.":
        "Malá lišta v rohu pohledu přepíná gizmo mezi **{{GizmoMove}}** a **{{GizmoRotate}}**. V Posunout táhnete entitu po podlaze nebo podél jedné z barevných šipek; mřížka nastavená v liště přichytává, <kbd>Ctrl</kbd> ji během tažení vypne - bez mřížky přichytí <kbd>Ctrl</kbd> tažení šipkou k 8 jednotkám. V Otočit dostanete kruhy (<kbd>Ctrl</kbd>: kroky po 15 stupních): modrý otáčí doleva a doprava; lodě a modely mají i zelený (sklon) a červený (náklon); postavy mají jen modrý kruh, protože hra pro ně zná jen `angle`. <kbd>Alt</kbd>+tažení nejdřív vytvoří kopii. Několik vybraných entit se posouvá a otáčí jako skupina.",
    "{{GizmoMove}}: arrows only.":
        "{{GizmoMove}}: jen šipky.",
    "{{GizmoRotate}}: three rings on a ship.":
        "{{GizmoRotate}}: tři kruhy na lodi.",
    "When the timeline stands in the middle of a movement, the gizmo does not change the start - it sets a new point at that moment. See [[figuren#griffe]] and [[animation#schluessel]].":
        "Stojí-li časová osa uprostřed pohybu, gizmo nemění začátek - nastaví v tom okamžiku nový bod. Viz [[figuren#griffe]] a [[animation#schluessel]].",
    "The inspector":
        "Inspektor",
    "Under the list the inspector shows the selected entity: its class with a description from the game code, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (how high the feet stand above the floor - red when inside it) and **{{EntAngle}}**, the spawnflags as checkboxes and all other keys as fields. Script fields have **{{EntOpenScript}}**, `NPC_type` has a search over every NPC of the game; when the class sets the type itself (`NPC_Droid_R2D2` → `r2d2`), the empty field shows it in grey, and a note says when the game chooses at random or ignores the key. The icon bar flies there, puts it **{{EntDropFloor}}**, duplicates, deletes, animates ([[animation]]) and shows **{{EntRaw}}**. Entity changes have their own undo while the mouse is over the map or the list.":
        "Pod seznamem ukazuje inspektor vybranou entitu: její třídu s popisem z kódu hry, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (jak vysoko nad podlahou stojí nohy - červeně, když jsou v ní) a **{{EntAngle}}**, spawnflags jako zaškrtávací políčka a všechny ostatní klíče jako pole. Skriptová pole mají **{{EntOpenScript}}**, `NPC_type` má hledání přes každé NPC hry; když třída nastavuje typ sama (`NPC_Droid_R2D2` → `r2d2`), prázdné pole ho ukazuje šedě a poznámka uvádí, kdy hra volí náhodně nebo klíč ignoruje. Lišta ikon tam přeletí, položí ji **{{EntDropFloor}}**, duplikuje, odstraní, animuje ([[animation]]) a ukáže **{{EntRaw}}**. Změny entit mají vlastní Zpět, dokud je myš nad mapou nebo seznamem.",
    "Placing new entities":
        "Umisťování nových entit",
    "The tiles on the left edge of the view place the common kinds: character, walk target, camera/look mark, look target, script runner, effect, sound, NPC waypoint, player start. Click a tile, then click in the view - or drag the tile into the view; <kbd>Shift</kbd> places several. **{{EntNew}}** above the list and a right-click in the view (**{{EntNewHere}}**) offer every class. The **Grid** button snaps to 8, 16, 32 or 64 units.":
        "Dlaždice u levého okraje pohledu umisťují běžné druhy: postava, cíl chůze, značka kamery/pohledu, cíl pohledu, spouštěč skriptu, efekt, zvuk, navigační bod NPC, start hráče. Klikněte na dlaždici a pak klikněte do pohledu - nebo dlaždici přetáhněte do pohledu; <kbd>Shift</kbd> umístí několik. **{{EntNew}}** nad seznamem a pravé tlačítko v pohledu (**{{EntNewHere}}**) nabízejí každou třídu. Tlačítko **Mřížka** přichytává k 8, 16, 32 nebo 64 jednotkám.",
    "NPC waypoints":
        "Navigační body NPC",
    "The network NPCs walk on - to far walk targets in cutscenes, and when they chase or flee. Place it by hand or let the program build it, then check it.":
        "Síť, po které NPC chodí - k vzdáleným cílům chůze v animačních sekvencích a když pronásledují nebo prchají. Umístěte ji ručně nebo ji nechte postavit program a pak ji zkontrolujte.",
    "The {{WegTitel}} window and the network on the map.":
        "Okno {{WegTitel}} a síť v mapě.",
    "Open it with **{{WegTitel}}** in the map toolbar. **{{WegZeigen}}** draws the paths: green set by hand (`target`), blue automatic (nav goals and combat points), orange jumps (`targetJump`).":
        "Otevřete ho pomocí **{{WegTitel}}** na liště mapy. **{{WegZeigen}}** kreslí cesty: zeleně ručně nastavené (`target`), modře automatické (cíle navigace a bojové body), oranžově skoky (`targetJump`).",
    "**{{WegSetzen}}**: click the floor - each new point is connected to the one before. <kbd>Shift</kbd>+click starts a new path, <kbd>Esc</kbd> ends.":
        "**{{WegSetzen}}**: klikněte na podlahu - každý nový bod se spojí s předchozím. <kbd>Shift</kbd>+kliknutí začne novou cestu, <kbd>Esc</kbd> ukončí.",
    "**{{WegVerbinden}}**: click two points to connect them (<kbd>Ctrl</kbd>: a jump); clicking two connected points disconnects them. A point has at most four connections.":
        "**{{WegVerbinden}}**: kliknutím na dva body je spojíte (<kbd>Ctrl</kbd>: skok); kliknutí na dva spojené body je rozpojí. Bod má nejvýš čtyři spojení.",
    "**{{WegErzeugen}}** spreads waypoints over the walkable floor (**{{WegAbstand}}**: the original levels use about 230) and connects them - one undo step. **{{WegEntfernen}}** removes only those, hand-placed ones stay.":
        "**{{WegErzeugen}}** rozprostře navigační body po pochozí podlaze (**{{WegAbstand}}**: původní úrovně používají asi 230) a spojí je - jeden krok zpět. **{{WegEntfernen}}** odebere jen ty, ručně umístěné zůstanou.",
    "**{{WegPruefen}}** finds what the game would reject or NPCs cannot use; click a problem to fly there.":
        "**{{WegPruefen}}** najde, co by hra odmítla nebo co NPC nemohou použít; kliknutím na problém tam přeletíte.",
    "The cutscene preview uses this network for walks exactly like the game: far targets need a path here, otherwise the figure stands still ([[figuren#wieimspiel]]). A network needs at least four waypoints. Doors on the network count while they are open; a closed door counts only if it opens by itself for NPCs.":
        "Náhled animační sekvence používá tuto síť pro chůzi přesně jako hra: vzdálené cíle tu potřebují cestu, jinak postava stojí ([[figuren#wieimspiel]]). Síť potřebuje nejméně čtyři navigační body. Dveře na síti se počítají, dokud jsou otevřené; zavřené dveře se počítají, jen pokud se pro NPC otevírají samy.",
    "The map editor":
        "Editor map",
    "Build and edit maps for Jedi Academy and Jedi Outcast - with the menus, windows and shortcuts of NetRadiant-Custom, so tutorials written for it work step by step.":
        "Tvorba a úprava map pro Jedi Academy a Jedi Outcast - s nabídkami, okny a zkratkami NetRadiant-Custom, takže tutoriály pro něj napsané fungují krok za krokem.",
    "The map editor in the layout {{KeLayoutRegular}}: the 2D view on the left with the console below it, the camera on the top right, the texture browser below it.":
        "Editor map v rozvržení {{KeLayoutRegular}}: 2D pohled vlevo s konzolí pod ním, kamera vpravo nahoře, prohlížeč textur pod ní.",
    "Opening and saving maps":
        "Otevírání a ukládání map",
    "Switch to **{{ViewMapEditor}}** in the mode bar. The editor works with the game the program is set up for - Jedi Academy (also with Movie Duels) or Jedi Outcast - and its textures, shaders and entity definitions; the first time, it loads them in the background. Without game folders in the program settings there are no textures; under **{{KeLPreferences}}** (pages **{{KePgGame}}** and **{{KePgPaths}}**) the map editor can also use another game and other folders. **{{KeLFile}} > {{KeLNewMap}}** starts an empty map, **{{KeLOpen}}** (<kbd>Ctrl</kbd>+<kbd>O</kbd>) opens a `.map` file, **{{KeLOpenBsp}}** a compiled map of the game (page [[karte_oeffnen]]), **{{KeLSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) and **{{KeLSaveAs}}** write it - like NetRadiant-Custom, the previous file is kept as `.bak`. The last maps you opened are at the bottom of the **{{KeLFile}}** menu. Before a map with unsaved changes is closed, the editor asks whether to save it.":
        "Přepněte na **{{ViewMapEditor}}** v liště režimů. Editor pracuje s hrou, pro kterou je program nastaven - Jedi Academy (i s Movie Duels) nebo Jedi Outcast - a s jejími texturami, shadery a definicemi entit; napoprvé je načte na pozadí. Bez složek hry v nastavení programu nejsou textury; v **{{KeLPreferences}}** (stránky **{{KePgGame}}** a **{{KePgPaths}}**) může editor map použít i jinou hru a jiné složky. **{{KeLFile}} > {{KeLNewMap}}** začne prázdnou mapu, **{{KeLOpen}}** (<kbd>Ctrl</kbd>+<kbd>O</kbd>) otevře soubor `.map`, **{{KeLOpenBsp}}** zkompilovanou mapu hry (stránka [[karte_oeffnen]]), **{{KeLSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) a **{{KeLSaveAs}}** ji zapíší - jako v NetRadiant-Custom se předchozí soubor uchová jako `.bak`. Naposledy otevřené mapy jsou dole v nabídce **{{KeLFile}}**. Než se zavře mapa s neuloženými změnami, editor se zeptá, zda ji uložit.",
    "Menus, windows and layouts":
        "Nabídky, okna a rozvržení",
    "Menus, toolbars and dialogs are laid out like NetRadiant-Custom (release 20260114): the same order, separators, shortcuts and mnemonics (<kbd>Alt</kbd> + the underlined letter). The labels are in the program language, the colours come from the program's colour scheme. The plugins are described on the page [[karte_plugins]]. The building tools are described on the page [[karteneditor_bauen]], compiling with q3map2 on [[karte_bauen]].":
        "Nabídky, panely nástrojů a dialogy jsou uspořádány jako v NetRadiant-Custom (vydání 20260114): stejné pořadí, oddělovače, zkratky a mnemotechnické klávesy (<kbd>Alt</kbd> + podtržené písmeno). Popisky jsou v jazyce programu, barvy pocházejí z barevného schématu programu. Pluginy popisuje stránka [[karte_plugins]]. Stavební nástroje popisuje stránka [[karteneditor_bauen]], kompilaci pomocí q3map2 [[karte_bauen]].",
    "**Toolbars:** as in NetRadiant-Custom, **Open**, **Save**, **Undo**, **Redo** and **Select Touching** are hidden in the main toolbar by default - right-click a toolbar and tick them to show them. Every toolbar has a handle (the dotted strip at its start): drag it to dock the main, filter or plugin toolbar at another edge (top, left, right, bottom). Edge and order are kept after a restart.":
        "**Panely nástrojů:** jako v NetRadiant-Custom jsou v hlavním panelu ve výchozím stavu skryté **Open**, **Save**, **Undo**, **Redo** a **Select Touching** - klikněte pravým tlačítkem na panel nástrojů a zaškrtnutím je zobrazíte. Každý panel nástrojů má úchyt (tečkovaný proužek na začátku): tažením ukotvíte hlavní panel, panel filtrů nebo pluginů u jiného okraje (nahoře, vlevo, vpravo, dole). Okraj a pořadí zůstanou i po restartu.",
    "The main toolbar docked on the left, the plugin toolbar at the bottom.":
        "Hlavní panel ukotvený vlevo, panel pluginů dole.",
    "In the map editor the program's menu bar becomes NetRadiant-Custom's menu bar ({{KeLFile}} ... {{KeLHelp}}): there is only one, the script tabs are hidden and the editor starts right below it. What the program needs here too is added at the end of two menus: **{{KeLMisc}}** has **{{KeLLanguage}}**, **{{KeLColorScheme}}** and **{{KeLScaling}}**, **{{KeLHelp}}** has **{{KeLManual}}** (<kbd>F1</kbd>, this guide) and **{{KeLAboutBehavEd}}**; **{{KeLCheckForNetRadiantUpdate}}** looks for a new version of this program. Leaving the mode in the mode bar brings back the program's menus and tabs.":
        "V editoru map se řádek nabídek programu stane řádkem nabídek NetRadiant-Custom ({{KeLFile}} ... {{KeLHelp}}): je jen jeden, karty skriptů jsou skryté a editor začíná hned pod ním. Co program potřebuje i zde, je přidáno na konec dvou nabídek: **{{KeLMisc}}** má **{{KeLLanguage}}**, **{{KeLColorScheme}}** a **{{KeLScaling}}**, **{{KeLHelp}}** má **{{KeLManual}}** (<kbd>F1</kbd>, tato příručka) a **{{KeLAboutBehavEd}}**; **{{KeLCheckForNetRadiantUpdate}}** hledá novou verzi tohoto programu. Opuštění režimu v liště režimů vrátí nabídky a karty programu.",
    "Every window of the editor - the 2D views, the camera, the console and the texture browser - has a thin frame, so you can see where one ends and the next begins; between them sits NetRadiant-Custom's splitter with its grip dots. The active window - the one you clicked last - has its frame in the accent colour of the colour scheme, as NetRadiant-Custom outlines the active 2D view (**{{KeLView}} > {{KeLShow}} > {{KeLShowWindowOutline}}** switches that off for the 2D views). Drag a splitter to resize the windows; dragged to the edge, a window collapses and can be pulled out again.":
        "Každé okno editoru - 2D pohledy, kamera, konzole a prohlížeč textur - má tenký rámeček, takže vidíte, kde jedno končí a další začíná; mezi nimi je rozdělovač NetRadiant-Custom s úchytovými tečkami. Aktivní okno - to, na které jste klikli naposledy - má rámeček v barvě zvýraznění barevného schématu, tak jako NetRadiant-Custom obtahuje aktivní 2D pohled (**{{KeLView}} > {{KeLShow}} > {{KeLShowWindowOutline}}** to pro 2D pohledy vypne). Tažením rozdělovače měníte velikost oken; přetažené k okraji se okno sbalí a dá se zase vytáhnout.",
    "The {{KeLView}} menu.":
        "Nabídka {{KeLView}}.",
    "**{{KeLEdit}} > {{KeLPreferences}}** (<kbd>P</kbd>) holds NetRadiant-Custom's preferences. On the page **{{KePgLayout}}** you choose the window layout - {{KeLayoutRegular}}, {{KeLayoutFloating}}, {{KeLayoutSplit}} or {{KeLayoutRegularLeft}} - and whether the group dialog with entities and textures sits in the main window (**{{KePrBuiltInGroupDialog}}**). The new layout applies as soon as you press **{{KePrefOk}}**; NetRadiant-Custom needs a restart for that. In {{KeLayoutFloating}} the windows float inside the program window.":
        "**{{KeLEdit}} > {{KeLPreferences}}** (<kbd>P</kbd>) obsahuje předvolby NetRadiant-Custom. Na stránce **{{KePgLayout}}** zvolíte rozvržení oken - {{KeLayoutRegular}}, {{KeLayoutFloating}}, {{KeLayoutSplit}} nebo {{KeLayoutRegularLeft}} - a zda je skupinový dialog s entitami a texturami v hlavním okně (**{{KePrBuiltInGroupDialog}}**). Nové rozvržení platí, jakmile stisknete **{{KePrefOk}}**; NetRadiant-Custom k tomu potřebuje restart. V {{KeLayoutFloating}} plovou okna uvnitř okna programu.",
    "{{KeLPreferences}}, page {{KePgLayout}}.":
        "{{KeLPreferences}}, stránka {{KePgLayout}}.",
    "**{{KeNrcTerms}}** on the page **{{KePgInterface}}** shows NetRadiant-Custom's English labels in every menu and dialog of the map editor, so tutorials written for NetRadiant-Custom (for example on JKHub) match word for word. Tooltips stay in the program language.":
        "**{{KeNrcTerms}}** na stránce **{{KePgInterface}}** ukáže anglické popisky NetRadiant-Custom v každé nabídce a dialogu editoru map, takže tutoriály napsané pro NetRadiant-Custom (například na JKHub) odpovídají slovo od slova. Popisky nástrojů zůstanou v jazyce programu.",
    "2D view and camera":
        "2D pohled a kamera",
    "Right mouse drag (2D)":
        "Tažení pravým tlačítkem (2D)",
    "Scroll the view":
        "Posouvat pohled",
    "Wheel, Alt + right mouse drag (2D)":
        "Kolečko, Alt + tažení pravým tlačítkem (2D)",
    "Next 2D view (XY, XZ, YZ)":
        "Další 2D pohled (XY, XZ, YZ)",
    "Middle mouse (2D)":
        "Prostřední tlačítko (2D)",
    "Turn the camera towards the point; with <kbd>Ctrl</kbd> the camera moves there":
        "Otočit kameru k bodu; s <kbd>Ctrl</kbd> se kamera přesune tam",
    "Right click in the camera":
        "Pravé kliknutí v kameře",
    "Free look on and off: the mouse turns the view, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> or the arrow keys move":
        "Volné rozhlížení zap. a vyp.: myš otáčí pohled, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> nebo šipky pohybují",
    "Arrow keys (camera)":
        "Šipky (kamera)",
    "Forward, back, turn; <kbd>A</kbd> / <kbd>D</kbd> sideways; the wheel moves forward and back":
        "Dopředu, dozadu, otáčení; <kbd>A</kbd> / <kbd>D</kbd> do strany; kolečko pohybuje dopředu a dozadu",
    "Camera onto the selection":
        "Kamera na výběr",
    "How the camera draws (wireframe, flat, textured ...)":
        "Jak kamera kreslí (drátový model, ploché, texturované ...)",
    "Grid smaller / larger; <kbd>1</kbd> ... <kbd>9</kbd> set it directly (1 to 256 units)":
        "Mřížka menší / větší; <kbd>1</kbd> ... <kbd>9</kbd> ji nastaví přímo (1 až 256 jednotek)",
    "Selecting and building":
        "Výběr a stavba",
    "Left click":
        "Levé kliknutí",
    "Select what is under the mouse; another click on the same spot selects what lies behind it":
        "Vybrat, co je pod myší; další kliknutí na stejné místo vybere, co leží za tím",
    "Shift + left mouse drag":
        "Shift + tažení levým tlačítkem",
    "Select (or deselect) everything the mouse passes over":
        "Vybrat (nebo zrušit výběr) vše, přes co myš přejede",
    "Shift + right mouse drag":
        "Shift + tažení pravým tlačítkem",
    "Select with a rectangle":
        "Vybrat obdélníkem",
    "Ctrl + left mouse (camera)":
        "Ctrl + levé tlačítko (kamera)",
    "Select single faces":
        "Vybrat jednotlivé stěny",
    "Left mouse drag on the selection":
        "Tažení levým tlačítkem po výběru",
    "Move it; next to a selected brush: pull its faces to resize it":
        "Posunout ho; vedle vybraného brushe: tahat za jeho stěny a měnit velikost",
    "Left mouse drag in empty space (2D, nothing selected)":
        "Tažení levým tlačítkem v prázdném místě (2D, nic nevybráno)",
    "Draw a new brush with the current texture":
        "Nakreslit nový brush s aktuální texturou",
    "Clear the selection":
        "Zrušit výběr",
    "Duplicate the selection":
        "Duplikovat výběr",
    "Delete the selection":
        "Odstranit výběr",
    "Undo":
        "Zpět",
    "Redo":
        "Znovu",
    "Copy / paste":
        "Kopírovat / vložit",
    "Hide the selection":
        "Skrýt výběr",
    "Show everything hidden":
        "Zobrazit vše skryté",
    "Textures":
        "Textury",
    "The texture browser: the folders on the left, the textures of the current folder on the right.":
        "Prohlížeč textur: složky vlevo, textury aktuální složky vpravo.",
    "The {{KeLEntityInspector}}: the classes with their description, the spawnflags, all keys and values, and fields for the keys of the class.":
        "{{KeLEntityInspector}}: třídy s popisem, spawnflags, všechny klíče a hodnoty a pole pro klíče třídy.",
    "A right click in the 2D view (without moving the mouse) opens the entity menu: pick a class to place a new entity at that point. If brushes are selected and you pick a brush class such as `func_door`, the brushes become that entity. **{{KeLView}} > {{KeLEntityInspector}}** (<kbd>N</kbd>) shows the selected entity: its class with the description, the spawnflags as check boxes, all keys and values, and fields for the keys the class knows. Type a key and a value and press <kbd>Enter</kbd> to set it. **{{KeLView}} > {{KeLEntityList}}** (<kbd>Shift</kbd>+<kbd>L</kbd>) lists all entities of the map. **{{KeLEntity}} > {{KeLConnectEntities}}** (<kbd>Ctrl</kbd>+<kbd>K</kbd>) links two selected entities with `target` and `targetname`.":
        "Pravé kliknutí ve 2D pohledu (bez pohybu myši) otevře nabídku entit: vyberte třídu a v tom bodě umístíte novou entitu. Jsou-li vybrané brushe a vyberete brush třídu jako `func_door`, brushe se stanou touto entitou. **{{KeLView}} > {{KeLEntityInspector}}** (<kbd>N</kbd>) ukáže vybranou entitu: její třídu s popisem, spawnflags jako zaškrtávací políčka, všechny klíče a hodnoty a pole pro klíče, které třída zná. Napište klíč a hodnotu a stiskem <kbd>Enter</kbd> je nastavíte. **{{KeLView}} > {{KeLEntityList}}** (<kbd>Shift</kbd>+<kbd>L</kbd>) vypíše všechny entity mapy. **{{KeLEntity}} > {{KeLConnectEntities}}** (<kbd>Ctrl</kbd>+<kbd>K</kbd>) propojí dvě vybrané entity pomocí `target` a `targetname`.",
    "Console":
        "Konzole",
    "The console (in the layout {{KeLayoutRegular}} below the 2D view, otherwise **{{KeLView}} > {{KeLConsole}}**, <kbd>O</kbd>) shows the same messages as NetRadiant-Custom: what was loaded, warnings about missing textures and shaders, and how long loading and saving took.":
        "Konzole (v rozvržení {{KeLayoutRegular}} pod 2D pohledem, jinak **{{KeLView}} > {{KeLConsole}}**, <kbd>O</kbd>) ukazuje stejné zprávy jako NetRadiant-Custom: co se načetlo, varování o chybějících texturách a shaderech a jak dlouho trvalo načítání a ukládání.",
    "Shortcuts":
        "Zkratky",
    "{{KeMappedCommands}}: every command with its key.":
        "{{KeMappedCommands}}: každý příkaz s jeho klávesou.",
    "The map editor has NetRadiant-Custom's shortcuts. They apply while the map editor is shown; the program's own shortcuts are off then. **{{KeLEdit}} > {{KeLShortcuts}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd>) lists every command with its key; the fields at the top search {{KeByName}} or {{KeByKeys}}. Double-click a command (or press **{{KeEdit}}**) and then the new key; **{{KeClear}}** removes the key, **{{KeReset}}** brings back NetRadiant-Custom's key, **{{KeResetAll}}** all of them. If the key already belongs to another command, the editor asks first.":
        "Editor map má zkratky NetRadiant-Custom. Platí, dokud je editor map zobrazen; vlastní zkratky programu jsou pak vypnuté. **{{KeLEdit}} > {{KeLShortcuts}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd>) vypíše každý příkaz s jeho klávesou; pole nahoře hledají {{KeByName}} nebo {{KeByKeys}}. Dvakrát klikněte na příkaz (nebo stiskněte **{{KeEdit}}**) a pak novou klávesu; **{{KeClear}}** klávesu odebere, **{{KeReset}}** vrátí klávesu NetRadiant-Custom, **{{KeResetAll}}** všechny. Patří-li klávesa už jinému příkazu, editor se nejdřív zeptá.",
    "Keyboards with <kbd>AltGr</kbd>: where `[` and `]` need <kbd>AltGr</kbd> (German: <kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd>), they still work - `[` and `]` change the grid, and <kbd>Shift</kbd>+<kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd> count as <kbd>Shift</kbd>+<kbd>[</kbd> / <kbd>]</kbd>. But <kbd>Ctrl</kbd>+<kbd>[</kbd> and <kbd>Ctrl</kbd>+<kbd>]</kbd> (**{{KeLFarClipPlaneIn}}** / **{{KeLFarClipPlaneOut}}**) cannot be pressed, because <kbd>AltGr</kbd> already counts as <kbd>Ctrl</kbd>+<kbd>Alt</kbd> - exactly as in NetRadiant-Custom. Use the menu **{{KeLView}} > {{KeLCamera}}**, or give the commands `CubicClipZoomIn` and `CubicClipZoomOut` other keys in **{{KeMappedCommands}}**.":
        "Klávesnice s <kbd>AltGr</kbd>: kde `[` a `]` potřebují <kbd>AltGr</kbd> (německá klávesnice: <kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd>), fungují i tak - `[` a `]` mění mřížku a <kbd>Shift</kbd>+<kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd> se počítají jako <kbd>Shift</kbd>+<kbd>[</kbd> / <kbd>]</kbd>. Ale <kbd>Ctrl</kbd>+<kbd>[</kbd> a <kbd>Ctrl</kbd>+<kbd>]</kbd> (**{{KeLFarClipPlaneIn}}** / **{{KeLFarClipPlaneOut}}**) stisknout nelze, protože <kbd>AltGr</kbd> se už počítá jako <kbd>Ctrl</kbd>+<kbd>Alt</kbd> - přesně jako v NetRadiant-Custom. Použijte nabídku **{{KeLView}} > {{KeLCamera}}**, nebo dejte příkazům `CubicClipZoomIn` a `CubicClipZoomOut` jiné klávesy v **{{KeMappedCommands}}**.",
    "Map editor: building tools":
        "Editor map: stavební nástroje",
    "Vertices, edges and faces, the clipper, CSG, brush shapes, transformations, patches, texturing, regions, filters, layers and models - the tools of NetRadiant-Custom with its keys, menus and dialogs.":
        "Vrcholy, hrany a stěny, ořezávač, CSG, tvary brushů, transformace, patche, texturování, oblasti, filtry, vrstvy a modely - nástroje NetRadiant-Custom s jeho klávesami, nabídkami a dialogy.",
    "Vertices, edges and faces":
        "Vrcholy, hrany a stěny",
    "Vertex mode (<kbd>V</kbd>): the points of the selected brush can be dragged.":
        "Režim vrcholů (<kbd>V</kbd>): body vybraného brushe lze táhnout.",
    "Select a brush or patch and press <kbd>V</kbd> (vertices), <kbd>E</kbd> (edges) or <kbd>F</kbd> (faces). Click a point to select it, drag it to reshape the brush; the brush is rebuilt from its points as in NetRadiant-Custom. The same key again returns to normal selection. <kbd>B</kbd> is the build tool (extrude, build chains, clone), <kbd>G</kbd> the UV tool for the texture of a face. **{{KeLTextureLock}}** (<kbd>Shift</kbd>+<kbd>T</kbd>) keeps textures in place while you move brushes.":
        "Vyberte brush nebo patch a stiskněte <kbd>V</kbd> (vrcholy), <kbd>E</kbd> (hrany) nebo <kbd>F</kbd> (stěny). Kliknutím na bod ho vyberete, tažením měníte tvar brushe; brush se z bodů znovu sestaví jako v NetRadiant-Custom. Stejná klávesa znovu vrátí běžný výběr. <kbd>B</kbd> je stavební nástroj (vytažení, stavba řetězů, klonování), <kbd>G</kbd> nástroj UV pro texturu stěny. **{{KeLTextureLock}}** (<kbd>Shift</kbd>+<kbd>T</kbd>) drží textury na místě, když posouváte brushe.",
    "Clipper":
        "Ořezávač",
    "The clipper (<kbd>X</kbd>): the line shows where the selected brush will be cut.":
        "Ořezávač (<kbd>X</kbd>): čára ukazuje, kde se vybraný brush rozřízne.",
    "Select brushes and press <kbd>X</kbd>. Click up to three points in a 2D view, or on surfaces in the camera, to define the cutting plane; drag the points to correct them. <kbd>Enter</kbd> cuts and keeps one side, <kbd>Shift</kbd>+<kbd>Enter</kbd> splits into both halves, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> flips which side is kept. A double click cuts or splits, as set on the preference page **{{KePgClipper}}**.":
        "Vyberte brushe a stiskněte <kbd>X</kbd>. Klikněte až na tři body ve 2D pohledu nebo na povrchy v kameře a určíte rovinu řezu; tažením bodů je opravíte. <kbd>Enter</kbd> ořízne a ponechá jednu stranu, <kbd>Shift</kbd>+<kbd>Enter</kbd> rozdělí na obě poloviny, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> obrátí, která strana zůstane. Dvojklik ořízne nebo rozdělí podle nastavení na stránce předvoleb **{{KePgClipper}}**.",
    "CSG":
        "CSG",
    "{{KeLCSGTool}}: hollow, shrink, expand and caulk in one window.":
        "{{KeLCSGTool}}: vydlabání, zmenšení, zvětšení a caulk v jednom okně.",
    "**{{KeLBrush}} > {{KeLCSG}}**: **{{KeLCSGSubtract}}** (<kbd>Shift</kbd>+<kbd>U</kbd>) cuts the selected brushes out of everything they touch, **{{KeLCSGMerge}}** joins brushes into one convex brush, **{{KeLCSGWrapMerge}}** (<kbd>Ctrl</kbd>+<kbd>U</kbd>) wraps them in one, **{{KeLCSGIntersect}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>) keeps only the common part, **{{KeLMakeRoom}}** turns a brush into a hollow room. **{{KeLMakeDetail}}** (<kbd>Alt</kbd>+<kbd>D</kbd>) and **{{KeLMakeStructural}}** (<kbd>Alt</kbd>+<kbd>S</kbd>) set the detail flag.":
        "**{{KeLBrush}} > {{KeLCSG}}**: **{{KeLCSGSubtract}}** (<kbd>Shift</kbd>+<kbd>U</kbd>) vyřízne vybrané brushe ze všeho, čeho se dotýkají, **{{KeLCSGMerge}}** spojí brushe do jednoho konvexního brushe, **{{KeLCSGWrapMerge}}** (<kbd>Ctrl</kbd>+<kbd>U</kbd>) je obalí jedním, **{{KeLCSGIntersect}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>) ponechá jen společnou část, **{{KeLMakeRoom}}** udělá z brushe dutou místnost. **{{KeLMakeDetail}}** (<kbd>Alt</kbd>+<kbd>D</kbd>) a **{{KeLMakeStructural}}** (<kbd>Alt</kbd>+<kbd>S</kbd>) nastavují příznak detailu.",
    "Brush shapes and transformations":
        "Tvary brushů a transformace",
    "The number of sides for **{{KeLPrism}}**.":
        "Počet stran pro **{{KeLPrism}}**.",
    "**{{KeLBrush}}** turns the selected brush into a **{{KeLPrism}}**, **{{KeLCone}}**, **{{KeLSphere}}**, **{{KeLRock}}** or **{{KeLIcosahedron}}**; a small dialog asks for the number of sides. <kbd>Ctrl</kbd>+<kbd>3</kbd> ... <kbd>9</kbd> make a prism with that many sides around the axis of the active 2D view. **{{KeLModify}}** rotates and mirrors the selection around its centre, **{{KeLArbitraryRotation}}** (<kbd>Shift</kbd>+<kbd>R</kbd>) and **{{KeLArbitraryScale}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) take exact values. **{{KeLRepeatTransforms}}** (<kbd>Ctrl</kbd>+<kbd>R</kbd>) repeats the last move, rotation or scale, **{{KeLResetTransforms}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) forgets it.":
        "**{{KeLBrush}}** změní vybraný brush na **{{KeLPrism}}**, **{{KeLCone}}**, **{{KeLSphere}}**, **{{KeLRock}}** nebo **{{KeLIcosahedron}}**; malý dialog se zeptá na počet stran. <kbd>Ctrl</kbd>+<kbd>3</kbd> ... <kbd>9</kbd> vytvoří hranol s tolika stranami kolem osy aktivního 2D pohledu. **{{KeLModify}}** otáčí a zrcadlí výběr kolem jeho středu, **{{KeLArbitraryRotation}}** (<kbd>Shift</kbd>+<kbd>R</kbd>) a **{{KeLArbitraryScale}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) berou přesné hodnoty. **{{KeLRepeatTransforms}}** (<kbd>Ctrl</kbd>+<kbd>R</kbd>) zopakuje poslední posun, otočení nebo změnu měřítka, **{{KeLResetTransforms}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) ji zapomene.",
    "Patches":
        "Patche",
    "**{{KeLCurve}} > {{KeLCylinder9x3}}** made a cylinder in the bounds of the selected brush.":
        "**{{KeLCurve}} > {{KeLCylinder9x3}}** vytvořil válec v rozměrech vybraného brushe.",
    "Texturing":
        "Texturování",
    "The {{KeSiTitle}} (<kbd>S</kbd>).":
        "{{KeSiTitle}} (<kbd>S</kbd>).",
    "The {{KeSiTitle}} (<kbd>S</kbd>) shows the texture of the selected faces: shift, scale and rotation, the step sizes for the arrow keys, fit (<kbd>Ctrl</kbd>+<kbd>F</kbd>) and the projection (axial, ortho, from the camera). Without the window, <kbd>Shift</kbd> + arrow keys shift the texture, <kbd>Ctrl</kbd> + arrow keys scale it, <kbd>Shift</kbd>+<kbd>PgUp</kbd> / <kbd>PgDn</kbd> rotate it. **{{KeLSelectTextured}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>A</kbd>) selects all faces with the current texture. With **{{KeLTagsGUI}}** the texture browser gets tags for sorting textures.":
        "{{KeSiTitle}} (<kbd>S</kbd>) ukazuje texturu vybraných stěn: posun, měřítko a otočení, velikosti kroků pro šipky, přizpůsobení (<kbd>Ctrl</kbd>+<kbd>F</kbd>) a projekci (osová, orto, z kamery). Bez okna posouvá <kbd>Shift</kbd> + šipky texturu, <kbd>Ctrl</kbd> + šipky mění měřítko, <kbd>Shift</kbd>+<kbd>PgUp</kbd> / <kbd>PgDn</kbd> ji otáčejí. **{{KeLSelectTextured}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>A</kbd>) vybere všechny stěny s aktuální texturou. S **{{KeLTagsGUI}}** dostane prohlížeč textur štítky pro třídění textur.",
    "Selection, regions and files":
        "Výběr, oblasti a soubory",
    "**{{KeLMisc}} > {{KeLMapInfo}}** (<kbd>M</kbd>): how many brushes, patches and entities the map has.":
        "**{{KeLMisc}} > {{KeLMapInfo}}** (<kbd>M</kbd>): kolik brushů, patchů a entit mapa má.",
    "**{{KeLSelectInside}}** and **{{KeLSelectTouching}}** select what lies inside or touches the selected brush. **{{KeLView}} > {{KeLRegion}}** shows only part of the map: **{{KeLSetXY}}** (the visible area of the 2D view), **{{KeLSetBrush}}**, **{{KeLSetSelection}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>R</kbd>), **{{KeLOff}}**. **{{KeLFile}} > {{KeLSaveRegion}}** writes the region with walls around it and a player start, **{{KeLSaveSelected}}** only the selection, **{{KeLImport}}** adds another map. **{{KeLFindBrush}}** selects a brush by its entity and brush number. The map is saved automatically every 15 minutes (preference page **{{KePgAutosave}}**).":
        "**{{KeLSelectInside}}** a **{{KeLSelectTouching}}** vyberou, co leží uvnitř vybraného brushe nebo se ho dotýká. **{{KeLView}} > {{KeLRegion}}** ukáže jen část mapy: **{{KeLSetXY}}** (viditelná oblast 2D pohledu), **{{KeLSetBrush}}**, **{{KeLSetSelection}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>R</kbd>), **{{KeLOff}}**. **{{KeLFile}} > {{KeLSaveRegion}}** zapíše oblast se zdmi kolem a startem hráče, **{{KeLSaveSelected}}** jen výběr, **{{KeLImport}}** přidá jinou mapu. **{{KeLFindBrush}}** vybere brush podle čísla entity a brushe. Mapa se automaticky ukládá každých 15 minut (stránka předvoleb **{{KePgAutosave}}**).",
    "Filters":
        "Filtry",
    "**{{KeLView}} > {{KeLFilter}}** and the filter bar on the right hide kinds of objects: world brushes (<kbd>Alt</kbd>+<kbd>1</kbd>), entities, caulk, clips, liquids, detail brushes, patches, lights, models, triggers and more. Some buttons do something else on a right click, as their tooltip says: make the selection detail or structural, create a func_group, put caulk or a tool texture on it. **{{KeLInvertFilters}}** and **{{KeLResetFilters}}** turn all of them around or off.":
        "**{{KeLView}} > {{KeLFilter}}** a lišta filtrů vpravo skrývají druhy objektů: brushe světa (<kbd>Alt</kbd>+<kbd>1</kbd>), entity, caulk, clipy, kapaliny, detailní brushe, patche, světla, modely, triggery a další. Některá tlačítka dělají při pravém kliknutí něco jiného, jak říká jejich popisek: udělat výběr detailním nebo strukturálním, vytvořit func_group, dát na něj caulk nebo nástrojovou texturu. **{{KeLInvertFilters}}** a **{{KeLResetFilters}}** je všechny obrátí nebo vypnou.",
    "Layers":
        "Vrstvy",
    "The {{KeLLayersBrowser}} (<kbd>L</kbd>).":
        "{{KeLLayersBrowser}} (<kbd>L</kbd>).",
    "Layers group parts of the map. In the {{KeLLayersBrowser}} (<kbd>L</kbd>) you create and rename layers, nest them by dragging, hide or show them with the eye, and move the selection into a layer (<kbd>Shift</kbd>+<kbd>Q</kbd>); <kbd>Ctrl</kbd>+<kbd>Q</kbd> extends the selection to its layers. New objects go into the current layer. The layers are saved in the `.map` file as in NetRadiant-Custom.":
        "Vrstvy seskupují části mapy. V {{KeLLayersBrowser}} (<kbd>L</kbd>) vytváříte a přejmenováváte vrstvy, vnořujete je tažením, skrýváte nebo zobrazujete je okem a přesouváte výběr do vrstvy (<kbd>Shift</kbd>+<kbd>Q</kbd>); <kbd>Ctrl</kbd>+<kbd>Q</kbd> rozšíří výběr na jeho vrstvy. Nové objekty jdou do aktuální vrstvy. Vrstvy se ukládají do souboru `.map` jako v NetRadiant-Custom.",
    "Models":
        "Modely",
    "The {{KeLModelBrowser}} (<kbd>/</kbd>) with the models of a folder.":
        "{{KeLModelBrowser}} (<kbd>/</kbd>) s modely složky.",
    "Entities like `misc_model` show their model (`.md3`, `.glm`, `.ase`) in both views, with skin, `modelscale` and angles. In the {{KeLModelBrowser}} (<kbd>/</kbd>) a double click on a folder loads its models; a click on a model gives it to the selected entities, a double click inserts a new `misc_model` in front of the camera. **{{KeLRefreshModels}}** reloads the models after they changed.":
        "Entity jako `misc_model` ukazují svůj model (`.md3`, `.glm`, `.ase`) v obou pohledech, se skinem, `modelscale` a úhly. V {{KeLModelBrowser}} (<kbd>/</kbd>) dvojklik na složku načte její modely; kliknutí na model ho dá vybraným entitám, dvojklik vloží nový `misc_model` před kameru. **{{KeLRefreshModels}}** znovu načte modely po jejich změně.",
    "Preferences":
        "Předvolby",
    "{{KeLPreferences}}, page {{KePgOrtho}}.":
        "{{KeLPreferences}}, stránka {{KePgOrtho}}.",
    "All pages of NetRadiant-Custom's preferences work: **{{KePgBrush}}** (default texture scale, brush type of new maps, always caulk), **{{KePgGrid}}**, **{{KePgUndo}}**, **{{KePgOrtho}}** (solid or stippled selection, follow the mouse at the edge while dragging, zoom to the mouse pointer), **{{KePgBuild}}**, **{{KePgSettings}}** (nudge after duplicating, load the last map). Under **{{KePgGame}}** you choose Jedi Academy or Jedi Outcast, a mod and the mapping mode (single or multiplayer entities), under **{{KePgPaths}}** the game folder and up to five extra folders. Choosing another game asks to restart the map editor, as NetRadiant-Custom does.":
        "Fungují všechny stránky předvoleb NetRadiant-Custom: **{{KePgBrush}}** (výchozí měřítko textur, typ brushů nových map, vždy caulk), **{{KePgGrid}}**, **{{KePgUndo}}**, **{{KePgOrtho}}** (plný nebo tečkovaný výběr, sledovat myš u okraje při tažení, přibližovat k ukazateli myši), **{{KePgBuild}}**, **{{KePgSettings}}** (postrčit po duplikaci, načíst poslední mapu). V **{{KePgGame}}** zvolíte Jedi Academy nebo Jedi Outcast, mod a režim mapování (entity pro jednoho nebo více hráčů), v **{{KePgPaths}}** složku hry a až pět dalších složek. Volba jiné hry si vyžádá restart editoru map, jako v NetRadiant-Custom.",
    "Menus and windows":
        "Nabídky a okna",
    "The menu {{KeLMisc}}, torn off at its dashed line, stays open as a window.":
        "Nabídka {{KeLMisc}}, odtržená na své přerušované čáře, zůstane otevřená jako okno.",
    "A click on the dashed line at the top of a menu tears it off into a window that stays open (**{{KePrDetachableMenus}}** on the page **{{KePgLayout}}**). **{{KeLMisc}} > {{KeLFullscreen}}** (<kbd>F11</kbd>) fills the screen, **{{KeLMaximizeView}}** (<kbd>F12</kbd>) enlarges the view under the mouse. **{{KeLOpenGLFont}}** chooses the font of the 2D and camera views, **{{KeLSet2DBackgroundImage}}** puts an image file into the bounds of the selection in the active 2D view, for tracing. The plugins are described on [[karte_plugins]], compiling with q3map2 on [[karte_bauen]].":
        "Kliknutí na přerušovanou čáru nahoře v nabídce ji odtrhne do okna, které zůstane otevřené (**{{KePrDetachableMenus}}** na stránce **{{KePgLayout}}**). **{{KeLMisc}} > {{KeLFullscreen}}** (<kbd>F11</kbd>) vyplní obrazovku, **{{KeLMaximizeView}}** (<kbd>F12</kbd>) zvětší pohled pod myší. **{{KeLOpenGLFont}}** volí písmo 2D pohledů a kamery, **{{KeLSet2DBackgroundImage}}** vloží obrázek do rozměrů výběru v aktivním 2D pohledu, pro obkreslování. Pluginy popisuje [[karte_plugins]], kompilaci pomocí q3map2 [[karte_bauen]].",
    "Map editor: compiling the map":
        "Editor map: kompilace mapy",
    "Turn the map into a playable .bsp with q3map2 from the {{KeLBuild}} menu: it runs in the background with its output in the console, shows leaks as a red line and portals on request, and never writes into the game folder.":
        "Převeďte mapu na hratelný .bsp pomocí q3map2 z nabídky {{KeLBuild}}: běží na pozadí s výstupem v konzoli, ukazuje úniky jako červenou čáru a na požádání portály a nikdy nezapisuje do složky hry.",
    "The {{KeLBuild}} menu":
        "Nabídka {{KeLBuild}}",
    "The {{KeLBuild}} menu: the entries of NetRadiant-Custom's build menu for Jedi Academy, and at the bottom {{KeBmAbbrechen}}.":
        "Nabídka {{KeLBuild}}: položky nabídky Build NetRadiant-Custom pro Jedi Academy a dole {{KeBmAbbrechen}}.",
    "Each entry runs one or more q3map2 commands on the map. The map compiler `bhed_q3map2.exe` is q3map2 from NetRadiant-Custom, built with the program and shipped next to `jkstudio.exe`; it produces the same results. **{{KeLRunRecentBuild}}** (<kbd>F5</kbd>) repeats the last entry. **{{KeLCustomize}}** opens the dialog **{{KeBmTitel}}**: entries and their commands can be added (double-click the `+`), renamed, reordered by dragging (with <kbd>Ctrl</kbd> copied) and deleted; **{{KeBmVariablen}}** shows the variables such as `[MapFile]` and `[q3map2]`. The menu is saved when the map editor closes.":
        "Každá položka spustí na mapě jeden nebo více příkazů q3map2. Kompilátor map `bhed_q3map2.exe` je q3map2 z NetRadiant-Custom, sestavený s programem a dodávaný vedle `jkstudio.exe`; dává stejné výsledky. **{{KeLRunRecentBuild}}** (<kbd>F5</kbd>) zopakuje poslední položku. **{{KeLCustomize}}** otevře dialog **{{KeBmTitel}}**: položky a jejich příkazy lze přidávat (dvojklik na `+`), přejmenovávat, přeřazovat tažením (s <kbd>Ctrl</kbd> kopírovat) a mazat; **{{KeBmVariablen}}** ukazuje proměnné jako `[MapFile]` a `[q3map2]`. Nabídka se uloží při zavření editoru map.",
    "{{KeLBuild}} > {{KeLCustomize}}: the entry [Final] Fast with its three commands.":
        "{{KeLBuild}} > {{KeLCustomize}}: položka [Final] Fast se svými třemi příkazy.",
    "The three stages":
        "Tři fáze",
    "Stage":
        "Fáze",
    "What it does":
        "Co dělá",
    "BSP (`-meta`)":
        "BSP (`-meta`)",
    "Turns brushes and patches into the BSP tree and the surfaces, writes `.bsp`, `.prt` and `.srf`. If the map is not sealed, it stops with a leak.":
        "Převede brushe a patche na strom BSP a povrchy, zapíše `.bsp`, `.prt` a `.srf`. Není-li mapa utěsněná, zastaví se s únikem.",
    "VIS (`-vis`)":
        "VIS (`-vis`)",
    "Computes which areas can see each other, so the game draws less. Needs the portal file of the BSP stage.":
        "Spočítá, které oblasti na sebe vidí, aby hra kreslila méně. Potřebuje soubor portálů z fáze BSP.",
    "LIGHT (`-light`)":
        "LIGHT (`-light`)",
    "Bakes the light of the light entities and the sky into lightmaps and the light grid.":
        "Zapeče světlo světelných entit a oblohy do lightmap a mřížky světla.",
    "**[Test] Quick and Crude** runs all three stages quickly for a first look, **[Final] Fast** and **[Final] Extreme Quality** give the light its real quality. The single stages are useful while working on one part, for example only **Single: light stage** after moving lights.":
        "**[Test] Quick and Crude** spustí rychle všechny tři fáze pro první pohled, **[Final] Fast** a **[Final] Extreme Quality** dají světlu skutečnou kvalitu. Jednotlivé fáze se hodí při práci na jedné části, například jen **Single: light stage** po posunutí světel.",
    "While it builds":
        "Během sestavování",
    "q3map2 runs in the background: its output appears in the console, the status bar shows the entry, the step and the time.":
        "q3map2 běží na pozadí: jeho výstup se objevuje v konzoli, stavový řádek ukazuje položku, krok a čas.",
    "The map is saved first (an unnamed map asks for a name). With a region, only the region is built (`<map>.reg`).":
        "Mapa se nejdřív uloží (nepojmenovaná mapa se zeptá na název). S oblastí se sestaví jen oblast (`<map>.reg`).",
    "q3map2 starts without a window and reports back over a local connection, as in NetRadiant-Custom. Warnings appear yellow, errors red; an error stops the remaining steps.":
        "q3map2 se spouští bez okna a hlásí se zpět přes místní připojení, jako v NetRadiant-Custom. Varování se objevují žlutě, chyby červeně; chyba zastaví zbývající kroky.",
    "You can keep working - look around, select, edit. **{{KeLBuild}} > {{KeBmAbbrechen}}** ends q3map2.":
        "Můžete dál pracovat - rozhlížet se, vybírat, upravovat. **{{KeLBuild}} > {{KeBmAbbrechen}}** ukončí q3map2.",
    "At the end a line names the entry, the steps and the time - and q3map2's exit code if it was not 0.":
        "Na konci řádek uvede položku, kroky a čas - a návratový kód q3map2, pokud nebyl 0.",
    "The console after [Final] Fast.":
        "Konzole po [Final] Fast.",
    "Without **{{KePrBuildMonitor}}** (page **{{KePgBuild}}** of the preferences) the steps run one after another, their output still comes into the console; **{{KePrBuildDump}}** also writes it to `junk.txt` in the settings folder.":
        "Bez **{{KePrBuildMonitor}}** (stránka **{{KePgBuild}}** předvoleb) běží kroky jeden po druhém, jejich výstup stále přichází do konzole; **{{KePrBuildDump}}** ho také zapíše do `junk.txt` ve složce nastavení.",
    "Finding leaks":
        "Hledání úniků",
    "A hole in a wall: the red line runs from an entity inside to the outside. The camera stands on the line and looks along it.":
        "Díra ve zdi: červená čára vede od entity uvnitř ven. Kamera stojí na čáře a dívá se podél ní.",
    "The inside of a map must be sealed by brushes. If q3map2 finds a way from an entity to the void, the BSP stage stops with **MAP LEAKED** (red), and the leak line appears red in the 2D views and the camera. The **{{KeDbgTitel}}** lists the entity it leaks from - a click selects it.":
        "Vnitřek mapy musí být utěsněný brushi. Najde-li q3map2 cestu od entity do prázdna, fáze BSP se zastaví s **MAP LEAKED** (červeně) a čára úniku se objeví červeně ve 2D pohledech a v kameře. **{{KeDbgTitel}}** vypíše entitu, od které únik vede - kliknutí ji vybere.",
    "Ctrl+Shift+K":
        "Ctrl+Shift+K",
    "{{KeLNextLeakSpot}}: camera and 2D views to the next point of the line, looking along it":
        "{{KeLNextLeakSpot}}: kamera a 2D pohledy na další bod čáry, s pohledem podél ní",
    "Ctrl+Shift+L":
        "Ctrl+Shift+L",
    "{{KeLPreviousLeakSpot}}: back to the previous point":
        "{{KeLPreviousLeakSpot}}: zpět na předchozí bod",
    "Follow the line until it leaves the map - there is the hole. Close it and build again. **{{KeLFile}} > {{KeLPointfile}}** switches the line off and on (it reads `<map>.lin`); saving the map removes it.":
        "Sledujte čáru, dokud neopustí mapu - tam je díra. Uzavřete ji a sestavte znovu. **{{KeLFile}} > {{KeLPointfile}}** čáru vypíná a zapíná (čte `<map>.lin`); uložení mapy ji odstraní.",
    "**{{KeLBuild}} > {{KeOptMenue}}** also shows the leak and detail brushes the line passes through - detail does not seal. It counts the map against the limits of the game and finds other build problems: [[karte_optimierer]].":
        "**{{KeLBuild}} > {{KeOptMenue}}** ukáže také únik a detailní brushe, kterými čára prochází - detail neutěsňuje. Spočítá mapu vůči limitům hry a najde další problémy sestavení: [[karte_optimierer]].",
    "Showing portals":
        "Zobrazení portálů",
    "Plugins > prtview > Load .prt file":
        "Plugins > prtview > Load .prt file",
    "Configure Portal Viewer":
        "Configure Portal Viewer",
    "Portals are the openings between the areas that the VIS stage works with. **{{KeLPlugins}} > prtview > Load .prt file** loads the portal file of the last BSP stage (**{{KePrtZeige3d}}**, **{{KePrtZeige2d}}**): in the camera each portal is a coloured, half-transparent polygon with an outline, in the 2D views a blue line. Many small portals where the player sees far make the map slow - hint brushes and detail brushes help. **Configure Portal Viewer** sets colours, line widths, opacity, fog and a clipping cube; **Toggle portals (3D)/(2D)** and **Unload .prt file** switch the display.":
        "Portály jsou otvory mezi oblastmi, se kterými pracuje fáze VIS. **{{KeLPlugins}} > prtview > Load .prt file** načte soubor portálů poslední fáze BSP (**{{KePrtZeige3d}}**, **{{KePrtZeige2d}}**): v kameře je každý portál barevný poloprůhledný mnohoúhelník s obrysem, ve 2D pohledech modrá čára. Mnoho malých portálů tam, kde hráč vidí daleko, mapu zpomaluje - pomáhají hint brushe a detailní brushe. **Configure Portal Viewer** nastavuje barvy, šířky čar, krytí, mlhu a ořezovou krychli; **Toggle portals (3D)/(2D)** a **Unload .prt file** přepínají zobrazení.",
    "The portals of the test map: blue in the 2D view, coloured in the camera at the passage between the two rooms.":
        "Portály testovací mapy: modře ve 2D pohledu, barevně v kameře u průchodu mezi dvěma místnostmi.",
    "Where the files go":
        "Kam jdou soubory",
    "Page {{KePgBuild}} of the preferences with **{{KePrBuildZiel}}**.":
        "Stránka {{KePgBuild}} předvoleb s **{{KePrBuildZiel}}**.",
    "The `.bsp` and the other files are written next to the `.map`, as in NetRadiant-Custom. **{{KePrBuildZiel}}** on the page **{{KePgBuild}}** puts them into a folder of your choice instead (the map is copied there and built). Nothing is ever written into the game folder: if the map or the output folder lies there, the build runs in a work folder in the settings folder, and the console says so.":
        "`.bsp` a ostatní soubory se zapisují vedle `.map`, jako v NetRadiant-Custom. **{{KePrBuildZiel}}** na stránce **{{KePgBuild}}** je místo toho dá do složky podle vaší volby (mapa se tam zkopíruje a sestaví). Do složky hry se nikdy nic nezapisuje: leží-li tam mapa nebo výstupní složka, sestavení běží v pracovní složce ve složce nastavení a konzole to oznámí.",
    "Testing in the game":
        "Vyzkoušení ve hře",
    "Copy the built `<name>.bsp` into the `maps` folder of the game, for example `GameData\\base\\maps` (with a mod: its folder, e.g. `MD\\maps`). The map editor does not do this itself - it never writes into the game folder.":
        "Zkopírujte sestavený `<name>.bsp` do složky `maps` hry, například `GameData\\base\\maps` (s modem: jeho složka, např. `MD\\maps`). Editor map to sám nedělá - nikdy nezapisuje do složky hry.",
    "Start the game yourself, open the console with <kbd>Shift</kbd>+<kbd>~</kbd> and type `devmap <name>` - the name without `.bsp`. `devmap` also allows cheats such as `noclip` and `god` for looking around.":
        "Spusťte hru sami, otevřete konzoli pomocí <kbd>Shift</kbd>+<kbd>~</kbd> a zadejte `devmap <name>` - název bez `.bsp`. `devmap` povoluje i cheaty jako `noclip` a `god` pro rozhlížení.",
    "After the next build, copy the `.bsp` again and type `devmap <name>` once more - the game loads the new version.":
        "Po dalším sestavení zkopírujte `.bsp` znovu a znovu zadejte `devmap <name>` - hra načte novou verzi.",
    "**{{KePrBuildRun}}** on the page **{{KePgBuild}}** starts the game after a build with `+devmap <name>`, as in NetRadiant-Custom - the `.bsp` must then already be in a folder the game searches.":
        "**{{KePrBuildRun}}** na stránce **{{KePgBuild}}** spustí hru po sestavení s `+devmap <name>`, jako v NetRadiant-Custom - `.bsp` pak už musí být ve složce, kterou hra prohledává.",
    "AutoCaulk":
        "AutoCaulk",
    "**{{KeLBrush}} > {{KeLAutoCaulkSelected}}** (<kbd>F4</kbd>) gives every face of the selected brushes that cannot be seen from inside the caulk texture - fewer surfaces, a faster map. Stand with the camera inside the selection; q3map2 decides which faces are hidden. One undo step takes it back.":
        "**{{KeLBrush}} > {{KeLAutoCaulkSelected}}** (<kbd>F4</kbd>) dá každé stěně vybraných brushů, kterou nelze vidět zevnitř, texturu caulk - méně povrchů, rychlejší mapa. Postavte se s kamerou dovnitř výběru; q3map2 rozhodne, které stěny jsou skryté. Jeden krok zpět to vrátí.",
    "Map editor: opening and changing finished maps":
        "Editor map: otevírání a změna hotových map",
    "Open a compiled map of the game - straight from its .pk3 - with {{KeLFile}} > {{KeLOpenBsp}}, change its entities and write them back with {{KeLBuild}} > {{KeBmNurEntities}}: the geometry and the original light stay byte for byte as they are.":
        "Otevřete zkompilovanou mapu hry - přímo z jejího .pk3 - pomocí {{KeLFile}} > {{KeLOpenBsp}}, změňte její entity a zapište je zpět pomocí {{KeLBuild}} > {{KeBmNurEntities}}: geometrie a původní světlo zůstanou bajt po bajtu, jak jsou.",
    "Opening a compiled map":
        "Otevření zkompilované mapy",
    "{{KeLFile}} > {{KeLOpenBsp}}: the dialog **{{KeBspTitel}}** with the filter `duel` - each map next to the archive the game loads it from; below the list the options for [[karte_oeffnen#modelle|misc_model]] and [[karte_oeffnen#lichter|lights]].":
        "{{KeLFile}} > {{KeLOpenBsp}}: dialog **{{KeBspTitel}}** s filtrem `duel` - každá mapa vedle archivu, ze kterého ji hra načítá; pod seznamem volby pro [[karte_oeffnen#modelle|misc_model]] a [[karte_oeffnen#lichter|světla]].",
    "**{{KeLFile}} > {{KeLOpenBsp}}** lists every map (`maps/…/*.bsp`) of the game folders as the game sees it: for each name only the copy the game would load - a mod before `base`, a later .pk3 before an earlier one. The right column names the .pk3 or the folder. Type part of the name into **{{KeBspFilter}}**.":
        "**{{KeLFile}} > {{KeLOpenBsp}}** vypíše každou mapu (`maps/…/*.bsp`) složek hry tak, jak ji vidí hra: pro každý název jen kopii, kterou by hra načetla - mod před `base`, pozdější .pk3 před dřívějším. Pravý sloupec jmenuje .pk3 nebo složku. Do **{{KeBspFilter}}** napište část názvu.",
    "Double-click a map or select it and click **{{KeBspOeffnen}}**. **{{KeBspDatei}}** opens a .bsp from anywhere on disk instead.":
        "Dvakrát klikněte na mapu nebo ji vyberte a klikněte na **{{KeBspOeffnen}}**. **{{KeBspDatei}}** místo toho otevře .bsp odkudkoli z disku.",
    "The map editor copies the .bsp into a work folder in the settings folder (`karte_bsp\\<name>`) - never into the game folder - and turns it back into a map with q3map2 `-convert`, as NetRadiant-Custom does when it imports a .bsp. Then the map is open like a `.map`. If the open map has unsaved changes, the editor asks first.":
        "Editor map zkopíruje .bsp do pracovní složky ve složce nastavení (`karte_bsp\\<name>`) - nikdy do složky hry - a převede ho zpět na mapu pomocí q3map2 `-convert`, jako to dělá NetRadiant-Custom při importu .bsp. Pak je mapa otevřená jako `.map`. Má-li otevřená mapa neuložené změny, editor se nejdřív zeptá.",
    "mp/duel1 opened from the game's .pk3: 362 brushes, 66 patches and 26 entities from the .bsp; in the background 14 `misc_model` were found again and 938 triangles of models that were not found became 779 thin brushes in the layer {{KeRkEbene}} (see [[karte_oeffnen#modelle]]).":
        "mp/duel1 otevřená z .pk3 hry: 362 brushů, 66 patchů a 26 entit z .bsp; na pozadí bylo znovu nalezeno 14 `misc_model` a 938 trojúhelníků nenalezených modelů se stalo 779 tenkými brushi ve vrstvě {{KeRkEbene}} (viz [[karte_oeffnen#modelle]]).",
    "A compiled map is not the original `.map`. Brushes with their textures, patches and entities come back. What q3map2 bakes into the .bsp does not: the models of `misc_model` entities, decals and the light entities - maps compiled for Jedi Academy normally no longer contain them, their light exists only in the lightmaps and the light grid. On some faces the texture alignment can differ slightly. Compiled again with LIGHT, such a map gets new light, calculated without the original lights. To change only entities - spawn points, NPCs, items, scripts, keys - keep the original light instead. Three own functions bring back more: the [[karte_oeffnen#editordaten|editor data]] of a map built with {{KeEdMenue}} (everything, exactly), [[karte_oeffnen#modelle|misc_model from the geometry]] and [[karte_oeffnen#lichter|estimated lights]].":
        "Zkompilovaná mapa není původní `.map`. Brushe s jejich texturami, patche a entity se vrátí. Co q3map2 zapeče do .bsp, se nevrátí: modely entit `misc_model`, decaly a světelné entity - mapy zkompilované pro Jedi Academy je obvykle už neobsahují, jejich světlo existuje jen v lightmapách a mřížce světla. Na některých stěnách se zarovnání textury může mírně lišit. Znovu zkompilovaná s LIGHT dostane taková mapa nové světlo, spočítané bez původních světel. Chcete-li měnit jen entity - body spawnu, NPC, předměty, skripty, klíče - ponechte místo toho původní světlo. Tři vlastní funkce vrátí víc: [[karte_oeffnen#editordaten|data editoru]] mapy sestavené s {{KeEdMenue}} (vše, přesně), [[karte_oeffnen#modelle|misc_model z geometrie]] a [[karte_oeffnen#lichter|odhadnutá světla]].",
    "What a .bsp no longer contains":
        "Co .bsp už neobsahuje",
    "q3map2 removes some entities while it builds the map, so they cannot come back from the .bsp - this is not a fault of the conversion. An example is a Jedi Academy map from a tester: of 388 lights 9 came back, of 265 `misc_model` none, all 233 `info_null` are there.":
        "q3map2 některé entity při sestavování mapy odstraní, takže se z .bsp vrátit nemohou - to není chyba převodu. Příkladem je mapa Jedi Academy od testera: z 388 světel se vrátilo 9, z 265 `misc_model` žádný, všech 233 `info_null` je tam.",
    "**Lights without a targetname**: only lights that can be switched in the game stay in the .bsp; q3map2 strips all others after calculating their light into the lightmaps (q3map2 `writebsp.cpp`, SetLightStyles). In the example these are the 9 lights with a targetname. [[karte_oeffnen#lichter|{{KeLsOpt}}]] places estimated lights instead.":
        "**Světla bez targetname**: v .bsp zůstanou jen světla, která lze ve hře přepínat; všechna ostatní q3map2 odstraní poté, co spočítá jejich světlo do lightmap (q3map2 `writebsp.cpp`, SetLightStyles). V příkladu je to 9 světel s targetname. [[karte_oeffnen#lichter|{{KeLsOpt}}]] místo nich umístí odhadnutá světla.",
    "**`misc_model`**: q3map2 bakes the model into the geometry of the map and removes the entity (`model.cpp`, AddTriangleModels; `bspfile_abstract.cpp`, UnparseEntities - unless the map was built with `-keepmodels`). The same happens to `_decal` and `_skybox`. [[karte_oeffnen#modelle|{{KeRkModelle}}]] finds the models in the geometry again.":
        "**`misc_model`**: q3map2 zapeče model do geometrie mapy a entitu odstraní (`model.cpp`, AddTriangleModels; `bspfile_abstract.cpp`, UnparseEntities - pokud mapa nebyla sestavena s `-keepmodels`). Totéž se stane s `_decal` a `_skybox`. [[karte_oeffnen#modelle|{{KeRkModelle}}]] modely v geometrii znovu najde.",
    "**`func_group`**: its brushes go into worldspawn (`map.cpp`, MoveBrushesToWorld), the group itself is gone.":
        "**`func_group`**: její brushe jdou do worldspawn (`map.cpp`, MoveBrushesToWorld), samotná skupina zmizí.",
    "**Layers, group keys and the exact texture alignment**: the .bsp has no layers; the keys of a `func_group` go with the group; the alignment of a brush face is rebuilt from the vertices of its surface (`-convert`) and can differ slightly.":
        "**Vrstvy, klíče skupin a přesné zarovnání textur**: .bsp nemá vrstvy; klíče `func_group` odejdou se skupinou; zarovnání stěny brushe se znovu sestaví z vrcholů jejího povrchu (`-convert`) a může se mírně lišit.",
    "**Faces without a brush**: the triangles of a model whose file is missing and faces of non-solid brushes are in the .bsp only as drawing surfaces. They come back neither as brush nor as model; [[karte_oeffnen#modelle|{{KeRkFlaechenOpt}}]] keeps them as thin brushes.":
        "**Stěny bez brushe**: trojúhelníky modelu, jehož soubor chybí, a stěny nepevných brushů jsou v .bsp jen jako vykreslované povrchy. Nevrátí se ani jako brush, ani jako model; [[karte_oeffnen#modelle|{{KeRkFlaechenOpt}}]] je ponechá jako tenké brushe.",
    "**Everything else stays**, for example `info_null` (the targets of spotlights), triggers, doors and their keys. Hint, areaportal and skip brushes come back structural, as in the source.":
        "**Vše ostatní zůstane**, například `info_null` (cíle reflektorů), triggery, dveře a jejich klíče. Brushe hint, areaportal a skip se vrátí strukturální, jako ve zdroji.",
    "**{{KeLFile}} > {{KeLImport}}** also takes a .bsp, as in NetRadiant-Custom: it is converted in the work folder and inserted into the open map.":
        "**{{KeLFile}} > {{KeLImport}}** bere také .bsp, jako v NetRadiant-Custom: převede se v pracovní složce a vloží do otevřené mapy.",
    "Keeping the editor data":
        "Zachování dat editoru",
    "For your own maps nothing has to get lost. Switch on **{{KeLBuild}} > {{KeEdMenue}}** (off by default, not in NetRadiant-Custom) before you build:":
        "U vlastních map se nemusí nic ztratit. Než sestavíte, zapněte **{{KeLBuild}} > {{KeEdMenue}}** (ve výchozím stavu vypnuto, není v NetRadiant-Custom):",
    "Every BSP step runs with `-keeplights`: all lights stay in the entity lump. The game frees lights without a targetname when the map starts (OpenJK `SP_light`), so they cost nothing in the game; the optimizer counts their bytes in the check **{{KeOptPrEnttext}}**.":
        "Každý krok BSP běží s `-keeplights`: všechna světla zůstanou v lumpu entit. Hra světla bez targetname při startu mapy uvolní (OpenJK `SP_light`), takže ve hře nic nestojí; optimalizátor počítá jejich bajty v kontrole **{{KeOptPrEnttext}}**.",
    "After the build `<map>.bhed` lies next to the .bsp: the exact source map with a fingerprint of the geometry of this .bsp. `misc_model` does not go into the .bsp - the game would spawn it as an empty entity - but into this file.":
        "Po sestavení leží vedle .bsp `<map>.bhed`: přesná zdrojová mapa s otiskem geometrie tohoto .bsp. `misc_model` nejde do .bsp - hra by ho spawnula jako prázdnou entitu - ale do tohoto souboru.",
    "**{{KeLFile}} > {{KeLOpenBsp}}** finds the file next to the .bsp - also in the .pk3 files of the game - and, if the fingerprint fits, opens the source map instead of a decompile: `misc_model`, `func_group`, texture alignment, detail and layers are back. If it belongs to another build, the console says so and the .bsp is decompiled as usual.":
        "**{{KeLFile}} > {{KeLOpenBsp}}** najde soubor vedle .bsp - i v souborech .pk3 hry - a pokud otisk sedí, otevře místo dekompilace zdrojovou mapu: `misc_model`, `func_group`, zarovnání textur, detail a vrstvy jsou zpět. Patří-li k jinému sestavení, konzole to oznámí a .bsp se dekompiluje jako obvykle.",
    "Lightmaps, light grid, visibility and geometry are byte for byte the same as without the option - the self-test compares lumps 1 to 17; only the entity lump grows. Put the .bhed into your .pk3 next to the .bsp if others should be able to open the map in the editor.":
        "Lightmapy, mřížka světla, viditelnost a geometrie jsou bajt po bajtu stejné jako bez této volby - autotest porovnává lumpy 1 až 17; zvětší se jen lump entit. Vložte .bhed do svého .pk3 vedle .bsp, mají-li ostatní mít možnost otevřít mapu v editoru.",
    "Recovering misc_model from the geometry":
        "Obnovení misc_model z geometrie",
    "Without editor data the models are only triangles in the .bsp. With **{{KeRkModelle}}** in the dialog {{KeBspTitel}} (on by default, not in NetRadiant-Custom) the editor looks for them after opening, in the background with a progress window:":
        "Bez dat editoru jsou modely v .bsp jen trojúhelníky. S **{{KeRkModelle}}** v dialogu {{KeBspTitel}} (ve výchozím stavu zapnuto, není v NetRadiant-Custom) je editor po otevření hledá, na pozadí s oknem průběhu:",
    "The candidates are the model files of the game in the folders of the map's shaders. bhed_q3map2 loads them exactly as the compiler did (`-bhedmodels`, the same model loader as `-bsp`).":
        "Kandidáty jsou soubory modelů hry ve složkách shaderů mapy. bhed_q3map2 je načte přesně jako kompilátor (`-bhedmodels`, tentýž zavaděč modelů jako `-bsp`).",
    "Each model is searched in the triangles of the .bsp - also when q3map2 merged it into larger surfaces (`forcemeta`). Only **exact** matches become a `misc_model` with `origin`, `angle` or `angles` and `modelscale`; the console names the largest vertex error.":
        "Každý model se hledá v trojúhelnících .bsp - i když ho q3map2 sloučil do větších povrchů (`forcemeta`). Jen **přesné** shody se stanou `misc_model` s `origin`, `angle` nebo `angles` a `modelscale`; konzole uvede největší chybu vrcholu.",
    "The models come as one undo step. Their triangles are not doubled: the decompile has no brushes for them, so {{KeBmNurEntities}} still works. If brushes or patches change before the search is done (for example with {{KeLImport}}), the result is dropped - open the .bsp again.":
        "Modely přijdou jako jeden krok zpět. Jejich trojúhelníky se nezdvojí: dekompilace pro ně nemá brushe, takže {{KeBmNurEntities}} stále funguje. Změní-li se brushe nebo patche před dokončením hledání (například pomocí {{KeLImport}}), výsledek se zahodí - otevřete .bsp znovu.",
    "The test map built without editor data and opened as .bsp: the pillar is back as `misc_model` with its angle and scale.":
        "Testovací mapa sestavená bez dat editoru a otevřená jako .bsp: sloup je zpět jako `misc_model` se svým úhlem a měřítkem.",
    "On jeditemple_garden (tester map, source known) all 265 of 265 `misc_model` came back with no wrong model; the largest difference to the source is 0.002 units in the position and 0.0002° in the angle. Original maps of the game have models from other folders too; what is not found stays triangles.":
        "Na jeditemple_garden (mapa testera, zdroj známý) se vrátilo všech 265 z 265 `misc_model` bez jediného špatného modelu; největší rozdíl oproti zdroji je 0.002 jednotky v poloze a 0.0002° v úhlu. Původní mapy hry mají modely i z jiných složek; co se nenajde, zůstane trojúhelníky.",
    "**{{KeRkFlaechenOpt}}** (on by default): triangles that belong to no recognized model and lie on no brush become thin detail brushes with the texture `system/nodraw` on their sides in the layer **{{KeRkEbene}}**. Check them - hide the layer or delete what you do not need; they can double geometry that exists in another form.":
        "**{{KeRkFlaechenOpt}}** (ve výchozím stavu zapnuto): trojúhelníky, které nepatří k žádnému rozpoznanému modelu a neleží na žádném brushi, se stanou tenkými detailními brushi s texturou `system/nodraw` na bocích ve vrstvě **{{KeRkEbene}}**. Zkontrolujte je - skryjte vrstvu nebo smažte, co nepotřebujete; mohou zdvojit geometrii, která existuje v jiné podobě.",
    "Estimating lights (experimental)":
        "Odhad světel (experimentální)",
    "**{{KeLsOpt}}** in the dialog {{KeBspTitel}} (off by default, not in NetRadiant-Custom) places lights where the light grid of the .bsp says light comes from: from each grid point the directed light is followed back until it hits geometry, the points where many rays meet become lights, their strength and colour come from the grid. They go into the layer **{{KeLsEbene}}** and carry the key `_bhed_geschaetzt 1` (q3map2 and the game ignore it). Lights that stayed in the .bsp are not doubled.":
        "**{{KeLsOpt}}** v dialogu {{KeBspTitel}} (ve výchozím stavu vypnuto, není v NetRadiant-Custom) umístí světla tam, odkud podle mřížky světla .bsp světlo přichází: z každého bodu mřížky se směrové světlo sleduje zpět, dokud nenarazí na geometrii, body, kde se setká mnoho paprsků, se stanou světly, jejich síla a barva pocházejí z mřížky. Jdou do vrstvy **{{KeLsEbene}}** a nesou klíč `_bhed_geschaetzt 1` (q3map2 i hra ho ignorují). Světla, která zůstala v .bsp, se nezdvojí.",
    "The test map without editor data: the estimated lights in the 2D view (the original had three lights on a line, one of them switchable and kept in the .bsp).":
        "Testovací mapa bez dat editoru: odhadnutá světla ve 2D pohledu (originál měl tři světla na přímce, jedno z nich přepínatelné a zachované v .bsp).",
    "This is an **estimate**, not the original lights. On jeditemple_garden 279 lights were estimated for 388 in the source; baked again with them, the lightmaps differ from the original by 38 of 255 steps on average (baked again with the true lights: 9), and only 2 estimated lights lie within 32 units of a source light. Use the estimated lights as a starting point and adjust them by eye - or build your own maps with {{KeEdMenue}}.":
        "Je to **odhad**, ne původní světla. Na jeditemple_garden bylo odhadnuto 279 světel místo 388 ve zdroji; znovu zapečené s nimi se lightmapy od originálu liší v průměru o 38 z 255 stupňů (znovu zapečené se skutečnými světly: 9) a jen 2 odhadnutá světla leží do 32 jednotek od světla zdroje. Použijte odhadnutá světla jako výchozí bod a dolaďte je okem - nebo stavte vlastní mapy s {{KeEdMenue}}.",
    "Keeping the original light":
        "Zachování původního světla",
    "{{KeLBuild}} > {{KeBmNurEntities}} - at the bottom of the menu, available after {{KeLOpenBsp}}.":
        "{{KeLBuild}} > {{KeBmNurEntities}} - dole v nabídce, k dispozici po {{KeLOpenBsp}}.",
    "Change entities: keys and values (also of worldspawn), move, add or delete point entities such as spawn points, NPC spawners and items, change the keys of brush entities.":
        "Měňte entity: klíče a hodnoty (i worldspawn), posouvejte, přidávejte nebo mažte bodové entity jako body spawnu, spawnery NPC a předměty, měňte klíče brush entit.",
    "**{{KeLBuild}} > {{KeBmNurEntities}}** saves the map, takes a fresh copy of the original .bsp and runs q3map2 `-onlyents` in the background (output in the console, as when building). Only the entity lump is rebuilt; the editor puts it into the copy of the original, so every other lump - geometry, lightmaps, light grid, visibility - stays byte for byte as it was.":
        "**{{KeLBuild}} > {{KeBmNurEntities}}** uloží mapu, vezme čerstvou kopii původního .bsp a spustí na pozadí q3map2 `-onlyents` (výstup v konzoli, jako při sestavení). Znovu se sestaví jen lump entit; editor ho vloží do kopie originálu, takže každý další lump - geometrie, lightmapy, mřížka světla, viditelnost - zůstane bajt po bajtu, jak byl.",
    "The console confirms it with **Entities updated** and the number of unchanged lumps. The result is `<name>.bsp` in the work folder; the untouched original stays next to it as `<name>_original.bsp`.":
        "Konzole to potvrdí hlášením **Entity aktualizovány** a počtem nezměněných lumpů. Výsledkem je `<name>.bsp` v pracovní složce; nedotčený originál zůstane vedle jako `<name>_original.bsp`.",
    "To play it, copy the new .bsp into the `maps` folder of the game as described on [[karte_bauen]] - for a map from a .pk3 best under a new name, so the original in the .pk3 stays as it is (for example `duel1_neu.bsp` in `maps\\mp`, started with `devmap mp/duel1_neu`).":
        "Chcete-li si ji zahrát, zkopírujte nový .bsp do složky `maps` hry, jak je popsáno na [[karte_bauen]] - u mapy z .pk3 nejlépe pod novým názvem, aby originál v .pk3 zůstal, jak je (například `duel1_neu.bsp` v `maps\\mp`, spuštěná pomocí `devmap mp/duel1_neu`).",
    "Brushes, patches, brush entities and baked entities such as `misc_model` are part of the geometry. If any of them changed since the map was opened, **{{KeBmNurEntities}}** does not run q3map2; the box **{{KeBmGeometrieTitel}}** explains that the map has to be built again with BSP, VIS and LIGHT - with new light. Undo brings the geometry back to the original, and the entry works again.":
        "Brushe, patche, brush entity a zapečené entity jako `misc_model` jsou součástí geometrie. Změnilo-li se od otevření mapy cokoli z toho, **{{KeBmNurEntities}}** q3map2 nespustí; okno **{{KeBmGeometrieTitel}}** vysvětlí, že mapu je nutné znovu sestavit s BSP, VIS a LIGHT - s novým světlem. Zpět vrátí geometrii do původního stavu a položka zase funguje.",
    "Lights with a targetname can be switched in the game; q3map2 gives each of them its own light style and bakes its light into extra lightmap stages. If such a light was added, removed, renamed or got another style since the map was opened, **{{KeBmNurEntities}}** does not run q3map2 either: the styles would be numbered anew while the lightmaps keep the old ones, and the wrong lights would switch. Build the map in full instead.":
        "Světla s targetname lze ve hře přepínat; q3map2 dá každému z nich vlastní styl světla a zapeče jeho světlo do dalších vrstev lightmapy. Bylo-li od otevření mapy takové světlo přidáno, odebráno, přejmenováno nebo dostalo jiný styl, **{{KeBmNurEntities}}** q3map2 také nespustí: styly by se očíslovaly znovu, zatímco lightmapy si ponechají staré, a přepínala by se špatná světla. Místo toho mapu sestavte celou.",
    "Jedi Outcast maps keep their light entities in the .bsp. If the original contains lights, q3map2 runs with `-keeplights` so that they stay. NetRadiant-Custom's own **Entity-Only compile** in the same menu also runs `-onlyents`, but without the check and without putting the result into the original - with Jedi Outcast maps it can drop lights and rewrite the light grid. For a map opened with **{{KeLOpenBsp}}**, use **{{KeBmNurEntities}}**.":
        "Mapy Jedi Outcast si ponechávají světelné entity v .bsp. Obsahuje-li originál světla, q3map2 běží s `-keeplights`, aby zůstala. Vlastní **Entity-Only compile** NetRadiant-Custom ve stejné nabídce také spouští `-onlyents`, ale bez kontroly a bez vložení výsledku do originálu - u map Jedi Outcast může zahodit světla a přepsat mřížku světla. U mapy otevřené pomocí **{{KeLOpenBsp}}** použijte **{{KeBmNurEntities}}**.",
    "Map editor: plugins":
        "Editor map: pluginy",
    "The {{KeLPlugins}} menu has every plugin of NetRadiant-Custom - bobToolz, brushexport2, MeshTex, ShaderPlug, SunPlug, UFO:AI and prtview - with all their entries and dialogs, ported from the source code.":
        "Nabídka {{KeLPlugins}} má každý plugin NetRadiant-Custom - bobToolz, brushexport2, MeshTex, ShaderPlug, SunPlug, UFO:AI a prtview - se všemi jejich položkami a dialogy, převedené ze zdrojového kódu.",
    "The {{KeLPlugins}} menu":
        "Nabídka {{KeLPlugins}}",
    "{{KeLPlugins}} > bobToolz: the entries of NetRadiant-Custom, with their icons.":
        "{{KeLPlugins}} > bobToolz: položky NetRadiant-Custom s jejich ikonami.",
    "The entries keep the English names of NetRadiant-Custom, as the plugins have them there; the dialogs are in the program language - with **{{KeNrcTerms}}** in NetRadiant-Custom's English. Every plugin has an **About** entry. Messages go to the console, errors in red. Each change a plugin makes is one undo step.":
        "Položky si ponechávají anglické názvy NetRadiant-Custom, jak je tam pluginy mají; dialogy jsou v jazyce programu - s **{{KeNrcTerms}}** v angličtině NetRadiant-Custom. Každý plugin má položku **About**. Zprávy jdou do konzole, chyby červeně. Každá změna, kterou plugin udělá, je jeden krok zpět.",
    "bobToolz":
        "bobToolz",
    "Entry":
        "Položka",
    "Flip terrain":
        "Flip terrain",
    "Two selected terrain brushes (triangular prisms with a common edge): turns their common diagonal the other way.":
        "Dva vybrané brushe terénu (trojboké hranoly se společnou hranou): otočí jejich společnou úhlopříčku na druhou stranu.",
    "Split patch rows / columns, Split patch":
        "Split patch rows / columns, Split patch",
    "Cuts the selected patch into strips along its rows, its columns, or both.":
        "Rozřízne vybraný patch na pruhy podél řádků, sloupců nebo obojího.",
    "Merge Patches":
        "Merge Patches",
    "Joins two or more selected patches that share an edge into one.":
        "Spojí dva nebo více vybraných patchů, které sdílejí hranu, do jednoho.",
    "Plot Splines":
        "Plot Splines",
    "Draws the paths of `info_train_spline_main` chains with their `info_train_spline_control` points (see Make Chain).":
        "Kreslí dráhy řetězů `info_train_spline_main` s jejich body `info_train_spline_control` (viz Make Chain).",
    "Tree Planter, Drop Entity":
        "Tree Planter, Drop Entity",
    "Select the ground brushes and switch **Tree Planter** on: each left click in the XY view puts an entity from `data\\karte\\bt\\tp_ent.txt` onto them, with a random model, angle and scale (as shipped, the file names tree models from Quake - put your own models there). **Drop Entity** drops the selected entities onto the same brushes.":
        "Vyberte brushe země a zapněte **Tree Planter**: každé levé kliknutí v pohledu XY na ně položí entitu z `data\\karte\\bt\\tp_ent.txt` s náhodným modelem, úhlem a měřítkem (v dodané podobě soubor jmenuje modely stromů z Quake - dejte tam své vlastní modely). **Drop Entity** pustí vybrané entity na tytéž brushe.",
    "Explode Entity":
        "Explode Entity",
    "Splits the selected brush entity: each of its brushes becomes an entity of its own with the same keys (not worldspawn).":
        "Rozdělí vybranou brush entitu: každý její brush se stane samostatnou entitou se stejnými klíči (ne worldspawn).",
    "Caulk Selection":
        "Caulk Selection",
    "Gives every face of the selected brushes and patches the caulk texture.":
        "Dá každé stěně vybraných brushů a patchů texturu caulk.",
    "Polygon Builder":
        "Polygon Builder",
    "Dialog **{{KeBtPolygon}}**: turns one selected brush into a prism with the given number of sides - optionally hollow with a border, inverse, aligned to the top edge.":
        "Dialog **{{KeBtPolygon}}**: změní jeden vybraný brush na hranol se zadaným počtem stran - volitelně dutý s okrajem, obrácený, zarovnaný k horní hraně.",
    "Brush Cleanup":
        "Brush Cleanup",
    "Removes invalid and duplicate planes from all brushes of the map and reports how many.":
        "Odstraní neplatné a duplicitní roviny ze všech brushů mapy a nahlásí kolik.",
    "Vis Viewer":
        "Vis Viewer",
    "Reads the compiled `<map>.bsp` next to the map - after a build, or the original after {{KeLOpenBsp}} - and shows which BSP leafs are visible from the camera or the selection; the window **{{KeBtBspLeafs}}** lists them.":
        "Přečte zkompilovaný `<map>.bsp` vedle mapy - po sestavení, nebo originál po {{KeLOpenBsp}} - a ukáže, které listy BSP jsou vidět z kamery nebo výběru; okno **{{KeBtBspLeafs}}** je vypíše.",
    "PitOMatic":
        "PitOMatic",
    "Turns one selected brush into a death pit: triggers for the falling scream (`*falling1.wav`) and for removing powerups, a `trigger_hurt` at the bottom.":
        "Změní jeden vybraný brush na smrtící jámu: triggery pro výkřik při pádu (`*falling1.wav`) a pro odebrání powerupů, `trigger_hurt` na dně.",
    "Reset Textures...":
        "Reset Textures...",
    "Dialog **{{KeBtTexReset}}**: replaces a texture name and resets scale, shift and rotation - on the selected or on all brushes.":
        "Dialog **{{KeBtTexReset}}**: nahradí název textury a obnoví měřítko, posun a otočení - na vybraných nebo na všech brushích.",
    "Path Plotter...":
        "Path Plotter...",
    "Shows the flight path of a `trigger_push` - see below.":
        "Ukáže letovou dráhu `trigger_push` - viz níže.",
    "Make Chain...":
        "Make Chain...",
    "Dialog **{{KeBtMakeChain}}**: builds a chain of `info_train_spline_main` entities with control points, named after the base name.":
        "Dialog **{{KeBtMakeChain}}**: sestaví řetěz entit `info_train_spline_main` s řídicími body, pojmenovaných podle základního názvu.",
    "Intersect..., Find Duplicates":
        "Intersect..., Find Duplicates",
    "Select brushes that intersect each other (in the whole map or among the selected ones, optionally only duplicates), or every brush that exactly duplicates another.":
        "Vybere brushe, které se navzájem protínají (v celé mapě nebo mezi vybranými, volitelně jen duplikáty), nebo každý brush, který přesně duplikuje jiný.",
    "Door Builder...":
        "Door Builder...",
    "Turns one selected brush into two door halves (`func_door`) with door and trim textures from `bt\\door-tex*.txt`, North-South or East-West.":
        "Změní jeden vybraný brush na dvě poloviny dveří (`func_door`) s texturami dveří a obložení z `bt\\door-tex*.txt`, sever-jih nebo východ-západ.",
    "Aperture Door...":
        "Aperture Door...",
    "Builds an aperture door (an iris of segments) from the selected brush - from parameters or from a winding.":
        "Postaví z vybraného brushe clonové dveře (duhovku ze segmentů) - z parametrů nebo z windingu.",
    "Stair Builder...":
        "Stair Builder...",
    "Turns one selected brush into stairs: step height, direction, style (Original, Bob's Style, Corner Style), main and riser texture, optionally detail brushes.":
        "Změní jeden vybraný brush na schody: výška schodu, směr, styl (Original, Bob's Style, Corner Style), hlavní textura a textura podstupnice, volitelně detailní brushe.",
    "Path Plotter":
        "Path Plotter",
    "Select a `trigger_push` that has a `target`.":
        "Vyberte `trigger_push`, který má `target`.",
    "**{{KeLPlugins}} > bobToolz > Path Plotter...** opens **{{KeBtPathPlotter}}**: number of points, distance multiplier and gravity of the path. **{{KeBtEnable}}** draws it in the 2D views and the camera.":
        "**{{KeLPlugins}} > bobToolz > Path Plotter...** otevře **{{KeBtPathPlotter}}**: počet bodů, násobitel vzdálenosti a gravitace dráhy. **{{KeBtEnable}}** ji nakreslí ve 2D pohledech a v kameře.",
    "The dialog {{KeBtPathPlotter}} with NetRadiant-Custom's values.":
        "Dialog {{KeBtPathPlotter}} s hodnotami NetRadiant-Custom.",
    "Move the target - the path follows, unless **{{KeBtNoUpdate}}** is ticked. **{{KeBtDisable}}** removes it.":
        "Posuňte cíl - dráha ho sleduje, pokud není zaškrtnuto **{{KeBtNoUpdate}}**. **{{KeBtDisable}}** ji odebere.",
    "brushexport2":
        "brushexport2",
    "The window {{KeBeTitel}}.":
        "Okno {{KeBeTitel}}.",
    "**Export selected as Wavefront Object** writes the selected brushes as a `.obj` model, for example to continue them in a modelling program. The window chooses how faces are grouped (**{{KeBeCollapse}}**, **{{KeBeCollapseMat}}**, **{{KeBeNoCollapse}}**), which materials to leave out (**{{KeBeIgnored}}** - for example `textures/common/caulk`), and whether to shorten material names, write objects instead of groups, weld vertices and write a `.mtl` file. **{{KeBeSaveAs}}** asks for the file - the first time in the folder `karte_export` in the settings folder - and **{{KeBeSave}}** writes the same file again.":
        "**Export selected as Wavefront Object** zapíše vybrané brushe jako model `.obj`, například pro pokračování v modelovacím programu. Okno volí, jak se stěny seskupí (**{{KeBeCollapse}}**, **{{KeBeCollapseMat}}**, **{{KeBeNoCollapse}}**), které materiály vynechat (**{{KeBeIgnored}}** - například `textures/common/caulk`) a zda zkracovat názvy materiálů, zapisovat objekty místo skupin, svařit vrcholy a zapsat soubor `.mtl`. **{{KeBeSaveAs}}** se zeptá na soubor - napoprvé ve složce `karte_export` ve složce nastavení - a **{{KeBeSave}}** zapíše znovu tentýž soubor.",
    "MeshTex":
        "MeshTex",
    "MeshTex > Set S/T Scale...":
        "MeshTex > Set S/T Scale...",
    "MeshTex lays textures onto patches - curves, arches, terrain - where the Surface Inspector reaches its limits. **{{KeMtSetScale}}** sets the scale along S and T: natural (as on a brush face, `*`) or as a number of tiles, counted from a \"zero\" column or row, optionally measured along a reference row or column. **{{KeMtGetInfo}}** shows the scale of the selected patch and can hand it to {{KeMtSetScale}}. **{{KeMtGenFunc}}** sets S and T as a linear function of the row and column distances. The **Align** entries fit the texture to the patch edges (minimum or maximum, stretched or shrunk, or automatically); **Help...** shows MeshTex's own explanation.":
        "MeshTex pokládá textury na patche - křivky, oblouky, terén - tam, kde Inspektor povrchu naráží na své meze. **{{KeMtSetScale}}** nastaví měřítko podél S a T: přirozené (jako na stěně brushe, `*`) nebo jako počet dlaždic, počítáno od \"nulového\" sloupce nebo řádku, volitelně měřeno podél referenčního řádku nebo sloupce. **{{KeMtGetInfo}}** ukáže měřítko vybraného patche a může ho předat do {{KeMtSetScale}}. **{{KeMtGenFunc}}** nastaví S a T jako lineární funkci vzdáleností řádků a sloupců. Položky **Align** přizpůsobí texturu hranám patche (minimum nebo maximum, roztažené nebo zmenšené, nebo automaticky); **Help...** ukáže vlastní vysvětlení MeshTex.",
    "ShaderPlug, SunPlug, UFO:AI, prtview":
        "ShaderPlug, SunPlug, UFO:AI, prtview",
    "Plugin":
        "Plugin",
    "ShaderPlug":
        "ShaderPlug",
    "**Create tag file** writes the tag file of the texture browser: every shader gets the name of its `.shader` file as a tag. The texture browser loads it again at once (NetRadiant-Custom only after a restart).":
        "**Create tag file** zapíše soubor štítků prohlížeče textur: každý shader dostane jako štítek název svého souboru `.shader`. Prohlížeč textur ho hned znovu načte (NetRadiant-Custom až po restartu).",
    "SunPlug":
        "SunPlug",
    "In NetRadiant-Custom it contains **ET-MapCoordinator**: it calculates `mapcoordsmins` and `mapcoordsmaxs` for the command map of Enemy Territory from the size of the map and writes them into worldspawn with **{{KeSunSet}}**. Jedi Academy does not use these keys.":
        "V NetRadiant-Custom obsahuje **ET-MapCoordinator**: spočítá `mapcoordsmins` a `mapcoordsmaxs` pro taktickou mapu Enemy Territory z velikosti mapy a zapíše je do worldspawn pomocí **{{KeSunSet}}**. Jedi Academy tyto klíče nepoužívá.",
    "UFO:AI":
        "UFO:AI",
    "Tools for the game UFO:AI: filters by its content flags (NoDraw, WeaponClip, ActorClip, StepOn, Level 1 to 8), **Perform check** and its worldspawn keys (**Worldspawn**, **Worldspawn reset**). They are there because NetRadiant-Custom shows them for every game.":
        "Nástroje pro hru UFO:AI: filtry podle jejích příznaků obsahu (NoDraw, WeaponClip, ActorClip, StepOn, Level 1 až 8), **Perform check** a její klíče worldspawn (**Worldspawn**, **Worldspawn reset**). Jsou tu, protože NetRadiant-Custom je ukazuje pro každou hru.",
    "prtview":
        "prtview",
    "Shows the portals of the last BSP stage - see [[karte_bauen]].":
        "Ukazuje portály poslední fáze BSP - viz [[karte_bauen]].",
    "open: no Jedi Outcast source - checked against the Jedi Academy value":
        "otevřené: žádný zdrojový kód Jedi Outcast - ověřeno proti hodnotě Jedi Academy",
    "Limit":
        "Limit",
    "In the source":
        "Ve zdrojovém kódu",
    "Allowed":
        "Povoleno",
    "Kind":
        "Druh",
    "Exceeded":
        "Překročeno",
    "Source (file:line)":
        "Zdroj (soubor:řádek)",
    "Map editor: optimizer and limits":
        "Editor map: optimalizátor a limity",
    "**{{KeLBuild}} > {{KeOptMenue}}** counts what the map uses against every limit of Jedi Academy, Movie Duels and Jedi Outcast - each limit taken from the source code of the engine -, finds build problems such as leaks, uncaulked hidden faces and patches with too many collision planes, and fixes many of them at the push of a button.":
        "**{{KeLBuild}} > {{KeOptMenue}}** spočítá, co mapa používá, vůči každému limitu Jedi Academy, Movie Duels a Jedi Outcast - každý limit převzatý ze zdrojového kódu enginu -, najde problémy sestavení jako úniky, skryté stěny bez caulku a patche s příliš mnoha kolizními rovinami a mnohé z nich opraví stiskem tlačítka.",
    "Opening the optimizer":
        "Otevření optimalizátoru",
    "{{KeLBuild}} > **{{KeOptMenue}}** - its own entry, NetRadiant-Custom has no optimizer.":
        "{{KeLBuild}} > **{{KeOptMenue}}** - vlastní položka, NetRadiant-Custom optimalizátor nemá.",
    "**{{KeLBuild}} > {{KeOptMenue}}** opens the window **{{KeOptTitel}}** and analyzes the map in the background - also the largest maps; the editor stays usable meanwhile. The console shows a summary at the end.":
        "**{{KeLBuild}} > {{KeOptMenue}}** otevře okno **{{KeOptTitel}}** a analyzuje mapu na pozadí - i ty největší mapy; editor mezitím zůstane použitelný. Konzole na konci ukáže souhrn.",
    "**{{KeOptRegelwerk}}** chooses the limits: **{{KeOptAuto}}** takes the game the editor works with (Jedi Academy, Movie Duels or Jedi Outcast); Jedi Academy multiplayer can be chosen as well.":
        "**{{KeOptRegelwerk}}** volí limity: **{{KeOptAuto}}** vezme hru, se kterou editor pracuje (Jedi Academy, Movie Duels nebo Jedi Outcast); zvolit lze i Jedi Academy pro více hráčů.",
    "If the map was built (see [[karte_bauen]]) or opened as a compiled map ([[karte_oeffnen]]), the .bsp is analyzed too: light grid, lightmaps, visibility, triangles per view, the leak line of the last BSP stage. Without a .bsp only the map is counted.":
        "Byla-li mapa sestavena (viz [[karte_bauen]]) nebo otevřena jako zkompilovaná mapa ([[karte_oeffnen]]), analyzuje se i .bsp: mřížka světla, lightmapy, viditelnost, trojúhelníky na pohled, čára úniku poslední fáze BSP. Bez .bsp se počítá jen mapa.",
    "On the page **{{KePgOptimizer}}** of **{{KeLPreferences}}** the optimizer can run by itself after every build; it opens only when there are problems.":
        "Na stránce **{{KePgOptimizer}}** v **{{KeLPreferences}}** může optimalizátor běžet sám po každém sestavení; otevře se, jen když jsou problémy.",
    "Limits":
        "Limity",
    "The tab **{{KeOptReiterGrenzen}}** with mp/duel1: each limit with value, allowed value, share and traffic light; on the right where the limit is in the source and the largest contributors.":
        "Karta **{{KeOptReiterGrenzen}}** s mp/duel1: každý limit s hodnotou, povolenou hodnotou, podílem a semaforem; vpravo, kde je limit ve zdrojovém kódu, a největší podíly.",
    "**{{KeOptOk}}** - well below the limit. **{{KeOptKnapp}}** - from 90 % of it (adjustable). **{{KeOptUeber}}** - over a limit the game enforces: the map does not load, crashes or loses something.":
        "**{{KeOptOk}}** - hluboko pod limitem. **{{KeOptKnapp}}** - od 90 % limitu (nastavitelné). **{{KeOptUeber}}** - přes limit, který hra vynucuje: mapa se nenačte, spadne nebo o něco přijde.",
    "**{{KeOptHinweis}}** - a format limit of the .bsp file (`MAX_MAP_*` in `qfiles.h`) that neither the engine nor q3map2 checks. Raven's own maps exceed several of them, so the optimizer never calls them over - it only notes them.":
        "**{{KeOptHinweis}}** - limit formátu souboru .bsp (`MAX_MAP_*` v `qfiles.h`), který nekontroluje engine ani q3map2. Vlastní mapy Raven několik z nich překračují, takže optimalizátor je nikdy neoznačí jako přes - jen je zaznamená.",
    "Values marked * are estimates (for example the entities that remain after spawning, or what a model costs). The column **{{KeOptSpAus}}** says whether a value was counted in the map or in the .bsp.":
        "Hodnoty označené * jsou odhady (například entity, které zůstanou po spawnu, nebo co stojí model). Sloupec **{{KeOptSpAus}}** říká, zda byla hodnota spočítána v mapě, nebo v .bsp.",
    "A click on a limit selects the largest contributors in the map and centers the views on them.":
        "Kliknutí na limit vybere největší podíly v mapě a vystředí na ně pohledy.",
    "Every limit is taken from the source code, not estimated: OpenJK for Jedi Academy (single player `code/`, multiplayer `codemp/`), the source of Movie Duels for Movie Duels, OpenJK with `JK2_MODE` and `codeJK2/` for Jedi Outcast. Where a mod raises a limit, the optimizer still checks against the stricter vanilla value (for example 2048 instead of 4096 patch planes). The tables at the end of the page list each limit with file and line; the test `karte_grenzen` reads these lines from the sources again.":
        "Každý limit je převzatý ze zdrojového kódu, ne odhadnutý: OpenJK pro Jedi Academy (hra pro jednoho hráče `code/`, pro více hráčů `codemp/`), zdrojový kód Movie Duels pro Movie Duels, OpenJK s `JK2_MODE` a `codeJK2/` pro Jedi Outcast. Kde mod limit zvyšuje, optimalizátor stále kontroluje proti přísnější hodnotě vanilla (například 2048 místo 4096 rovin patchů). Tabulky na konci stránky uvádějí každý limit se souborem a řádkem; test `karte_grenzen` tyto řádky znovu čte ze zdrojů.",
    "Checks and fixes":
        "Kontroly a opravy",
    "The tab **{{KeOptReiterBefunde}}**: each finding with severity, count and fix; on the right what it means, its source and the objects.":
        "Karta **{{KeOptReiterBefunde}}**: každý nález se závažností, počtem a opravou; vpravo, co znamená, jeho zdroj a objekty.",
    "A click on a finding selects its objects and centers the 2D views and the camera on them.":
        "Kliknutí na nález vybere jeho objekty a vystředí na ně 2D pohledy a kameru.",
    "**{{KeOptBeheben}}** shows what would change; **{{KeOptAnwenden}}** does it. Every fix is one step of **Undo**, and the map is counted again afterwards - the finding disappears or shrinks.":
        "**{{KeOptBeheben}}** ukáže, co by se změnilo; **{{KeOptAnwenden}}** to provede. Každá oprava je jeden krok **Zpět** a mapa se poté spočítá znovu - nález zmizí nebo se zmenší.",
    "The preview of a fix before it is applied.":
        "Náhled opravy před jejím použitím.",
    "**{{KeOptAllesSichere}}** applies only the fixes marked {{KeOptSicher}} - the ones that cannot change how the map looks or plays - in a single undo step.":
        "**{{KeOptAllesSichere}}** použije jen opravy označené {{KeOptSicher}} - ty, které nemohou změnit, jak mapa vypadá nebo se hraje - v jediném kroku zpět.",
    "Fixes that set a worldspawn key (light grid, lightmaps) take effect with the next build; the console says so. **{{KeOptExport}}** writes everything with sources into a text file.":
        "Opravy, které nastavují klíč worldspawn (mřížka světla, lightmapy), se projeví při dalším sestavení; konzole to oznámí. **{{KeOptExport}}** zapíše vše se zdroji do textového souboru.",
    "Check":
        "Kontrola",
    "What it means":
        "Co znamená",
    "Fix":
        "Oprava",
    "Safe":
        "Bezpečné",
    "Settings":
        "Nastavení",
    "{{KeLPreferences}}, page **{{KePgOptimizer}}** - a page of its own, NetRadiant-Custom has none.":
        "{{KeLPreferences}}, stránka **{{KePgOptimizer}}** - vlastní stránka, NetRadiant-Custom žádnou nemá.",
    "**{{KePrOptKnapp}}** sets from which share a limit counts as near, **{{KePrOptNachBau}}** runs the optimizer after every build, **{{KePrOptRegelwerk}}** fixes the rule set, **{{KePrOptPortale}}** and **{{KePrOptDreiecke}}** are the thresholds for the VIS and triangle notes.":
        "**{{KePrOptKnapp}}** nastavuje, od jakého podílu se limit počítá jako těsný, **{{KePrOptNachBau}}** spouští optimalizátor po každém sestavení, **{{KePrOptRegelwerk}}** pevně nastaví sadu pravidel, **{{KePrOptPortale}}** a **{{KePrOptDreiecke}}** jsou prahy pro poznámky o VIS a trojúhelnících.",
    "The limits of each game":
        "Limity každé hry",
    "**{{KeOptSpErlaubt}}** is the value the optimizer checks against - one less than the define where the engine itself takes a slot (for example the world model or the box model), and the stricter vanilla value where a mod raised it. The source column names every place: the define and the line that stops the game.":
        "**{{KeOptSpErlaubt}}** je hodnota, proti které optimalizátor kontroluje - o jedna menší než define tam, kde si engine sám bere jedno místo (například model světa nebo model krabice), a přísnější hodnota vanilla tam, kde ji mod zvýšil. Sloupec zdroje jmenuje každé místo: define a řádek, který hru zastaví.",
    "Jedi Academy (OpenJK SP)":
        "Jedi Academy (OpenJK SP)",
    "Jedi Academy MP (OpenJK)":
        "Jedi Academy MP (OpenJK)",
    "Movie Duels":
        "Movie Duels",
    "Jedi Outcast (OpenJK JK2_MODE)":
        "Jedi Outcast (OpenJK JK2_MODE)",
    "Report":
        "Hlášení",
    "NetRadiant-Custom":
        "NetRadiant-Custom",
    "JK Studio":
        "JK Studio",
    "After leaving vertex or edge mode the components stay selected; the next drag deforms the brush.":
        "Po opuštění režimu vrcholů nebo hran zůstanou komponenty vybrané; další tažení deformuje brush.",
    "Leaving vertex or edge mode deselects its components.":
        "Opuštění režimu vrcholů nebo hran zruší výběr jeho komponent.",
    "NRC#65":
        "NRC#65",
    "Bevel and inverted bevel caps can create two degenerate patches, depending on the patch orientation.":
        "Zakrytí zkosením a obráceným zkosením může podle orientace patche vytvořit dva degenerované patche.",
    "The cap direction is chosen by size and shape, so every cap has a surface.":
        "Směr zakrytí se volí podle velikosti a tvaru, takže každé zakrytí má povrch.",
    "NRC#302, GL#206":
        "NRC#302, GL#206",
    "A path with umlauts or other non-ASCII characters: the map does not open, the settings get damaged.":
        "Cesta s přehláskami nebo jinými znaky mimo ASCII: mapa se neotevře, nastavení se poškodí.",
    "Editor and bhed_q3map2 handle any path (e.g. C:\\Users\\Jörg\\...).":
        "Editor a bhed_q3map2 zvládnou libovolnou cestu (např. C:\\Users\\Jörg\\...).",
    "NRC#130":
        "NRC#130",
    "Saving with a relative engine path (../../Academy/) triggers an assertion.":
        "Uložení s relativní cestou k enginu (../../Academy/) vyvolá aserci.",
    "Paths are made absolute when they are set.":
        "Cesty se při nastavení převedou na absolutní.",
    "NRC#164":
        "NRC#164",
    "AutoCaulk fails with \"reading ..._ac.caulk failure\" without a reason.":
        "AutoCaulk selže s \"reading ..._ac.caulk failure\" bez udání důvodu.",
    "The console shows q3map2's output and the reason.":
        "Konzole ukáže výstup q3map2 a důvod.",
    "NRC#213":
        "NRC#213",
    "With the Jedi Academy shaders AutoCaulk puts the non-solid system/nodraw on solid brushes and uses watercaulk, which does not exist there.":
        "Se shadery Jedi Academy dává AutoCaulk na pevné brushe nepevný system/nodraw a používá watercaulk, který tam neexistuje.",
    "The names of system.shader in Jedi Academy and Jedi Outcast: nodraw_solid, caulk_water, caulk_lava, caulk_acid; a shader the game lacks is left out. AutoCaulk runs q3map2 with the game of the editor (-game ja/jk2).":
        "Názvy ze system.shader v Jedi Academy a Jedi Outcast: nodraw_solid, caulk_water, caulk_lava, caulk_acid; shader, který hře chybí, se vynechá. AutoCaulk spouští q3map2 s hrou editoru (-game ja/jk2).",
    "NRC#145":
        "NRC#145",
    "The Surface Inspector shows unnormalized brush-primitive shifts from other editors as the spinner limit (8192); an arrow or Enter then moves the texture.":
        "Inspektor povrchu ukazuje nenormalizované posuny brush primitives z jiných editorů jako mez číselníku (8192); šipka nebo Enter pak texturu posune.",
    "Outside the spinner range the value is shown reduced by whole tiles - same alignment; the .map stays unchanged until a value is set.":
        "Mimo rozsah číselníku se hodnota ukazuje zmenšená o celé dlaždice - stejné zarovnání; .map zůstane beze změny, dokud se hodnota nenastaví.",
    "NRC#261":
        "NRC#261",
    "Caps only work for certain patch sizes; the console says so in English only.":
        "Zakrytí funguje jen pro určité velikosti patchů; konzole to hlásí jen anglicky.",
    "Same limit (intended); below the English line a hint in the program language names the size each cap needs.":
        "Stejný limit (záměrný); pod anglickým řádkem uvede nápověda v jazyce programu velikost, kterou každé zakrytí potřebuje.",
    "NRC#279":
        "NRC#279",
    "Text errors fixed after tag 20260114 (bobToolz Door Builder \"East - West\", \"Distance Multiplier\").":
        "Chyby v textech opravené po tagu 20260114 (bobToolz Door Builder \"East - West\", \"Distance Multiplier\").",
    "Taken over.":
        "Převzato.",
    "NRC master":
        "NRC master",
    "Choosing a texture in the texture browser now really sets it as the shader of new brushes (it was never stored).":
        "Volba textury v prohlížeči textur ji teď skutečně nastaví jako shader nových brushů (nikdy se neukládala).",
    "The state of the tag window was stored twice and got lost; the **{{KePrefClean}}** button of the preferences had no function.":
        "Stav okna štítků se ukládal dvakrát a ztrácel se; tlačítko **{{KePrefClean}}** v předvolbách nemělo funkci.",
    "Every menu entry, button, key and field was checked for its effect, and every command that changes the map undoes and redoes byte for byte.":
        "Každá položka nabídky, tlačítko, klávesa a pole byly zkontrolovány na svůj účinek a každý příkaz, který mění mapu, se vrací a opakuje bajt po bajtu.",
    "-convert (File > Open .bsp) writes hint, areaportal and skip brushes of Jedi Academy, Jedi Outcast and SoF2 maps as detail: in these games \"structural\" has no content flag, so the check for structural transparent brushes never applies.":
        "-convert (File > Open .bsp) zapisuje brushe hint, areaportal a skip map Jedi Academy, Jedi Outcast a SoF2 jako detail: v těchto hrách nemá \"structural\" příznak obsahu, takže kontrola strukturálních průhledných brushů nikdy neplatí.",
    "The compile flags of the shaders decide as when building (q3map_structural / surfaceparm structural): such brushes come back structural, as in the source map.":
        "Příznaky kompilace shaderů rozhodují jako při sestavení (q3map_structural / surfaceparm structural): takové brushe se vrátí strukturální, jako ve zdrojové mapě.",
    "Tester (Stage 7)":
        "Tester (fáze 7)",
    "Not deterministic: every build writes a different .bsp (shader pointers as sort keys, uninitialized lightmap coordinates, thread races in -vis and -light); the names of generated style shaders change with every build.":
        "Nedeterministické: každé sestavení zapíše jiný .bsp (ukazatele na shadery jako klíče řazení, neinicializované souřadnice lightmap, souběhy vláken v -vis a -light); názvy generovaných shaderů stylů se mění s každým sestavením.",
    "The BSP and light stages give the same result for the same map every time, with any number of threads equal to a 1-thread build; with one thread -light stays byte-identical to NRC. The vis stage is as in NRC: the same with one thread, with several threads its result may differ slightly from build to build.":
        "Fáze BSP a světla dávají pro stejnou mapu pokaždé stejný výsledek, s libovolným počtem vláken shodný se sestavením s 1 vláknem; s jedním vláknem zůstane -light bajtově shodný s NRC. Fáze vis je jako v NRC: s jedním vláknem stejná, s více vlákny se její výsledek může mezi sestaveními mírně lišit.",
    "GL#173":
        "GL#173",
    "Dirtmapping (-dirty) darkens faces next to the sky as if the sky were a wall.":
        "Dirtmapping (-dirty) ztmavuje stěny vedle oblohy, jako by obloha byla zeď.",
    "A dirt ray that leaves the map through the sky does not darken.":
        "Paprsek špíny, který opustí mapu přes oblohu, neztmavuje.",
    "NRC#101":
        "NRC#101",
    "A map name with a dot (test.q3, foo_0.1) given without .map loses the part after the dot.":
        "Název mapy s tečkou (test.q3, foo_0.1) zadaný bez .map ztratí část za tečkou.",
    "Only q3map2's own extensions are replaced (the editor always passes full paths anyway).":
        "Nahrazují se jen vlastní přípony q3map2 (editor stejně vždy předává celé cesty).",
    "NRC#102, GL#154":
        "NRC#102, GL#154",
    "Memory errors: a brush side without a plane reads mapplanes[-1]; a deluxel reference without a deluxemap.":
        "Chyby paměti: strana brushe bez roviny čte mapplanes[-1]; odkaz na deluxel bez deluxemapy.",
    "Fixed (checked with AddressSanitizer and UBSan over the regression maps and all decompiled maps of the three games); the output is unchanged.":
        "Opraveno (ověřeno pomocí AddressSanitizer a UBSan přes regresní mapy a všechny dekompilované mapy tří her); výstup se nezměnil.",
    "PR #316":
        "PR #316",
    "Thin brush parts under one cubic unit are discarded; a crossing face narrower than 0.2 units vanishes; faces almost on another face's plane are not drawn; brushes with a small cross-section are not split, so part of them has no collision.":
        "Tenké části brushů pod jednu krychlovou jednotku se zahazují; protínající stěna užší než 0.2 jednotky zmizí; stěny téměř v rovině jiné stěny se nekreslí; brushe s malým průřezem se nerozdělí, takže část z nich nemá kolize.",
    "The five fixes of the pull request for this are taken over - each checked alone on the original maps of Jedi Academy and Jedi Outcast: no map got worse. Left out: dropping portals only when degenerate - with it 42 of 106 original maps leak (narrow gaps that NetRadiant-Custom closes).":
        "Pět oprav z pull requestu k tomu je převzato - každá ověřena samostatně na původních mapách Jedi Academy a Jedi Outcast: žádná mapa se nezhoršila. Vynecháno: zahazování portálů jen tehdy, když jsou degenerované - s ním propouští 42 ze 106 původních map (úzké mezery, které NetRadiant-Custom uzavírá).",
    "Crashes or undefined behaviour with broken map files (no entity at all, a quote as the last character, huge numbers) and when patches are subdivided for the lightmaps.":
        "Pády nebo nedefinované chování s poškozenými soubory map (vůbec žádná entita, uvozovka jako poslední znak, obrovská čísla) a při dělení patchů pro lightmapy.",
    "Found by running bhed_q3map2 under AddressSanitizer and UBSan over 300 deliberately broken maps and the original maps; fixed. A map without entities now gets a clear error. The output of valid maps stays byte for byte the same.":
        "Nalezeno spuštěním bhed_q3map2 pod AddressSanitizer a UBSan přes 300 záměrně poškozených map a původní mapy; opraveno. Mapa bez entit teď dostane jasnou chybu. Výstup platných map zůstane bajt po bajtu stejný.",
    "ASan, UBSan":
        "ASan, UBSan",
    "(editor) Numbers from about 1e10 are cut when a map is saved; 1e20 becomes 1.":
        "(editor) Čísla zhruba od 1e10 se při uložení mapy oříznou; 1e20 se stane 1.",
    "The value is kept (found with the new fuzzer for broken map files); ordinary coordinates are written exactly as before.":
        "Hodnota se zachová (nalezeno novým fuzzerem pro poškozené soubory map); běžné souřadnice se zapisují přesně jako dřív.",
    "Decimal":
        "Desetinná čísla",
    "(engine) A misc_model_static whose cull point origin + (0 0 zoffset+1) lies in solid is never drawn.":
        "(engine) misc_model_static, jehož bod ořezu origin + (0 0 zoffset+1) leží v pevné hmotě, se nikdy nevykreslí.",
    "The preview \"as in the game\" does not show it either, and the optimizer offers the zoffset that brings it back ([[karte_optimierer]]).":
        "Náhled \"jako ve hře\" ho také neukáže a optimalizátor nabídne zoffset, který ho vrátí ([[karte_optimierer]]).",
    "OpenJK#1358":
        "OpenJK#1358",
    "(game) Light styles: after entity edits, switchable lights (with a targetname) turn the wrong lights on and off - -onlyents numbers the styles anew, the lightmaps keep the old ones.":
        "(hra) Styly světla: po úpravách entit zapínají a vypínají přepínatelná světla (s targetname) špatná světla - -onlyents čísluje styly znovu, lightmapy si ponechají staré.",
    "**{{KeBmNurEntities}}** refuses when the switchable lights differ from the original .bsp and asks for a full build ([[karte_oeffnen]]).":
        "**{{KeBmNurEntities}}** odmítne, když se přepínatelná světla liší od původního .bsp, a požádá o úplné sestavení ([[karte_oeffnen]]).",
    "JKHub":
        "JKHub",
    "**Camera inside a brush** (Stage 7): back faces are culled, so a brush you stand in disappears - unless its shader says `cull none`, `cull twosided` or `cull disable` (fog volumes, glass, water); then its walls stay visible from inside, as in NetRadiant-Custom (renderstate.cpp OpenGLShader::construct, shaders.cpp `cull`).":
        "**Kamera uvnitř brushe** (fáze 7): zadní stěny se ořezávají, takže brush, ve kterém stojíte, zmizí - pokud jeho shader neříká `cull none`, `cull twosided` nebo `cull disable` (objemy mlhy, sklo, voda); pak jeho stěny zůstanou viditelné zevnitř, jako v NetRadiant-Custom (renderstate.cpp OpenGLShader::construct, shaders.cpp `cull`).",
    "**NRC#281**: Ctrl+K to several targets gives them the same targetname - as in NRC.":
        "**NRC#281**: Ctrl+K k několika cílům jim dá stejné targetname - jako v NRC.",
    "**NRC#132**: a region build leaks when the region has no entities and the camera is inside a brush - as in NRC; JK Studio warns before such a build.":
        "**NRC#132**: sestavení oblasti propouští, když oblast nemá entity a kamera je uvnitř brushe - jako v NRC; JK Studio před takovým sestavením varuje.",
    "**NRC#48**: external lightmaps together with light styles do not work in RBSP (Jedi Academy, Jedi Outcast) - a limit of the format; the build menu and the optimizer refuse the combination and explain the way around it.":
        "**NRC#48**: externí lightmapy spolu se styly světla v RBSP (Jedi Academy, Jedi Outcast) nefungují - omezení formátu; nabídka Build i optimalizátor tuto kombinaci odmítnou a vysvětlí, jak to obejít.",
    "**GL#189, NRC#144**: surface lights under a floor shine through it because of the backsplash - as in NRC; the optimizer finds such lights ([[karte_optimierer]]).":
        "**GL#189, NRC#144**: světelné povrchy pod podlahou jí prosvítají kvůli backsplash - jako v NRC; optimalizátor taková světla najde ([[karte_optimierer]]).",
    "**Menus in the style of JK Studio.** The menu bar and all menus of the map editor look like the menus of the rest of the program. Entries, order, submenus, checks, shortcuts, the mnemonic keys and what each entry does are those of NetRadiant-Custom. **Preferences > Interface > {{KeMenueNrc}}** brings back the look of NetRadiant-Custom.":
        "**Nabídky ve stylu JK Studia.** Řádek nabídek a všechny nabídky editoru map vypadají jako nabídky zbytku programu. Položky, pořadí, podnabídky, zaškrtnutí, zkratky, mnemotechnické klávesy a co která položka dělá, jsou ty z NetRadiant-Custom. **Preferences > Rozhraní > {{KeMenueNrc}}** vrátí vzhled NetRadiant-Custom.",
    "**Program settings in View.** At the end of **{{KeLView}}**: {{ViewTarget}}, {{ViewTheme}}, {{ViewLanguage}}, {{ViewDpiScale}} and {{KeKnopfGroesse}} - with the same effect as in the program's View menu. Language, theme and scaling also stay at the end of **{{KeLMisc}}**.":
        "**Nastavení programu v Zobrazení.** Na konci **{{KeLView}}**: {{ViewTarget}}, {{ViewTheme}}, {{ViewLanguage}}, {{ViewDpiScale}} a {{KeKnopfGroesse}} - se stejným účinkem jako v nabídce Zobrazení programu. Jazyk, motiv a měřítko zůstávají i na konci **{{KeLMisc}}**.",
    "**Toolbar button size.** {{KeKnopfKlein}}, {{KeKnopfNormal}} or {{KeKnopfGross}} for the main, filter and plugin toolbars, under **Preferences > Interface** and in **{{KeLView}} > {{KeKnopfGroesse}}**. It applies at once and is saved; the views move up or down with the toolbars.":
        "**Velikost tlačítek panelů nástrojů.** {{KeKnopfKlein}}, {{KeKnopfNormal}} nebo {{KeKnopfGross}} pro hlavní panel, panel filtrů a pluginů, v **Preferences > Rozhraní** a v **{{KeLView}} > {{KeKnopfGroesse}}**. Platí hned a ukládá se; pohledy se s panely nástrojů posunou nahoru nebo dolů.",
    "**Ctrl + mouse wheel** does not scale the program inside the map editor: as in NetRadiant-Custom the wheel belongs to the views (zoom in 2D, move in the camera, scroll in the texture browser), also with Ctrl. The scaling is in **{{KeLView}} > {{ViewDpiScale}}**.":
        "**Ctrl + kolečko myši** v editoru map nemění měřítko programu: jako v NetRadiant-Custom patří kolečko pohledům (přiblížení ve 2D, pohyb v kameře, posun v prohlížeči textur), i s Ctrl. Měřítko je v **{{KeLView}} > {{ViewDpiScale}}**.",
    "**Window title.** In the map editor the title shows the map and a `*` when it has unsaved changes, as in NetRadiant-Custom.":
        "**Název okna.** V editoru map ukazuje název mapu a `*`, má-li neuložené změny, jako v NetRadiant-Custom.",
    "**Own build functions.** The window {{KeFsTitel}} with the remaining time, {{KeTkVis}} and {{KeTkLicht}}, {{KeZfMenue}} ([[karte_teilbau]]), {{KeEdMenue}}, {{KeRkModelle}} and {{KeLsOpt}} ([[karte_oeffnen#editordaten]]) - each with its own menu entry or option and the hint \"not in NetRadiant-Custom\". bhed_q3map2 has the switches `-bhedvisreuse`, `-bhedlightcache` and `-bhedmodels` for them; without them it builds as before.":
        "**Vlastní funkce sestavení.** Okno {{KeFsTitel}} se zbývajícím časem, {{KeTkVis}} a {{KeTkLicht}}, {{KeZfMenue}} ([[karte_teilbau]]), {{KeEdMenue}}, {{KeRkModelle}} a {{KeLsOpt}} ([[karte_oeffnen#editordaten]]) - každá s vlastní položkou nabídky nebo volbou a poznámkou \"není v NetRadiant-Custom\". bhed_q3map2 pro ně má přepínače `-bhedvisreuse`, `-bhedlightcache` a `-bhedmodels`; bez nich sestavuje jako dřív.",
    "**Own editor tools (stage 8b).** {{KeSfSchalter}} for the split layout, {{KeSpMenue}} (F6), {{KeLgMenue}}, {{KeAmMenue}}, {{KeSgMenue}} and {{KeUlMenue}} ([[karte_werkzeuge8b]]) - each with its own menu entry or option and the hint \"not in NetRadiant-Custom\", all off by default. The Terrain Generator comes from the NetRadiant-Custom source after the release 20260114; its fixed seed is an own addition. {{KeRkModelle}} looks in all model folders with a cached model index. bhed_q3map2 `-light -bhedgpu` runs the shadow tests on the graphics card with byte-identical results, but stays off (no gain on the measured computer).":
        "**Vlastní nástroje editoru (fáze 8b).** {{KeSfSchalter}} pro rozdělené rozvržení, {{KeSpMenue}} (F6), {{KeLgMenue}}, {{KeAmMenue}}, {{KeSgMenue}} a {{KeUlMenue}} ([[karte_werkzeuge8b]]) - každý s vlastní položkou nabídky nebo volbou a poznámkou \"není v NetRadiant-Custom\", všechny ve výchozím stavu vypnuté. Generátor terénu pochází ze zdrojového kódu NetRadiant-Custom po vydání 20260114; jeho pevné semínko je vlastní doplněk. {{KeRkModelle}} hledá ve všech složkách modelů s indexem modelů v mezipaměti. bhed_q3map2 `-light -bhedgpu` provádí testy stínů na grafické kartě s bajtově shodnými výsledky, ale zůstává vypnuto (na měřeném počítači bez zisku).",
    "Map editor: differences to NetRadiant-Custom":
        "Editor map: rozdíly oproti NetRadiant-Custom",
    "The map editor follows NetRadiant-Custom 20260114 in look and handling. Known bugs of NetRadiant-Custom, Xonotic NetRadiant, GtkRadiant and q3map2 were checked one by one: what JK Studio does differently, and what stays as in NetRadiant-Custom on purpose.":
        "Editor map se vzhledem a ovládáním řídí NetRadiant-Custom 20260114. Známé chyby NetRadiant-Custom, Xonotic NetRadiant, GtkRadiant a q3map2 byly zkontrolovány jedna po druhé: co JK Studio dělá jinak a co záměrně zůstává jako v NetRadiant-Custom.",
    "Own handling (not in NetRadiant-Custom)":
        "Vlastní ovládání (není v NetRadiant-Custom)",
    "{{KeLView}}: the program settings at the end, menus in the style of JK Studio.":
        "{{KeLView}}: nastavení programu na konci, nabídky ve stylu JK Studia.",
    "Preferences > Interface: {{KeMenueNrc}} and {{KeKnopfGroesse}}.":
        "Preferences > Rozhraní: {{KeMenueNrc}} a {{KeKnopfGroesse}}.",
    "{{KeKnopfGroesse}} {{KeKnopfKlein}}: the toolbars take less room.":
        "{{KeKnopfGroesse}} {{KeKnopfKlein}}: panely nástrojů zaberou méně místa.",
    "Editor: fixed bugs":
        "Editor: opravené chyby",
    "Found during the check":
        "Nalezeno při kontrole",
    "bhed_q3map2: fixed bugs and deliberate output differences":
        "bhed_q3map2: opravené chyby a záměrné rozdíly výstupu",
    "A .bsp built by bhed_q3map2 therefore differs from one built by NetRadiant-Custom's q3map2 in the order of the surfaces and the packing of the lightmaps - not in the geometry. NetRadiant-Custom's q3map2 itself never writes the same file twice.":
        ".bsp sestavený pomocí bhed_q3map2 se proto od toho sestaveného pomocí q3map2 z NetRadiant-Custom liší pořadím povrchů a skládáním lightmap - ne geometrií. Samotný q3map2 z NetRadiant-Custom nikdy nezapíše stejný soubor dvakrát.",
    "Preview as in the game":
        "Náhled jako ve hře",
    "Kept as in NetRadiant-Custom":
        "Ponecháno jako v NetRadiant-Custom",
    "BSP (all threads)":
        "BSP (všechna vlákna)",
    "duel_pit":
        "duel_pit",
    "0.20 s":
        "0.20 s",
    "VIS (all threads)":
        "VIS (všechna vlákna)",
    "0.15 s":
        "0.15 s",
    "0.10 s":
        "0.10 s",
    "LIGHT (all threads)":
        "LIGHT (všechna vlákna)",
    "0.66 s":
        "0.66 s",
    "0.61 s":
        "0.61 s",
    "BSP (1 thread)":
        "BSP (1 vlákno)",
    "VIS (1 thread)":
        "VIS (1 vlákno)",
    "LIGHT (1 thread)":
        "LIGHT (1 vlákno)",
    "3.02 s":
        "3.02 s",
    "3.33 s":
        "3.33 s",
    "3.08 s":
        "3.08 s",
    "mp__duel1":
        "mp__duel1",
    "0.36 s":
        "0.36 s",
    "0.31 s":
        "0.31 s",
    "0.11 s":
        "0.11 s",
    "2.53 s":
        "2.53 s",
    "2.99 s":
        "2.99 s",
    "1.92 s":
        "1.92 s",
    "0.35 s":
        "0.35 s",
    "0.16 s":
        "0.16 s",
    "8.72 s":
        "8.72 s",
    "9.68 s":
        "9.68 s",
    "9.63 s":
        "9.63 s",
    "t1_inter":
        "t1_inter",
    "0.26 s":
        "0.26 s",
    "0.30 s":
        "0.30 s",
    "0.25 s":
        "0.25 s",
    "md_ch_ben":
        "md_ch_ben",
    "0.81 s":
        "0.81 s",
    "0.71 s":
        "0.71 s",
    "0.76 s":
        "0.76 s",
    "2.77 s":
        "2.77 s",
    "2.27 s":
        "2.27 s",
    "2.23 s":
        "2.23 s",
    "3.73 s":
        "3.73 s",
    "4.24 s":
        "4.24 s",
    "2.58 s":
        "2.58 s",
    "0.86 s":
        "0.86 s",
    "30.53 s":
        "30.53 s",
    "24.90 s":
        "24.90 s",
    "24.60 s":
        "24.60 s",
    "12.66 s":
        "12.66 s",
    "12.65 s":
        "12.65 s",
    "12.76 s":
        "12.76 s",
    "kejim_post":
        "kejim_post",
    "4.74 s":
        "4.74 s",
    "4.59 s":
        "4.59 s",
    "4.14 s":
        "4.14 s",
    "745.40 s":
        "745.40 s",
    "621.47 s":
        "621.47 s",
    "573.70 s":
        "573.70 s",
    "9.18 s":
        "9.18 s",
    "10.28 s":
        "10.28 s",
    "7.21 s":
        "7.21 s",
    "duel_mos_espa":
        "duel_mos_espa",
    "53.35 s":
        "53.35 s",
    "44.86 s":
        "44.86 s",
    "19.57 s":
        "19.57 s",
    "Invert Selection (select all)":
        "Obrátit výběr (vybrat vše)",
    "29.74 s":
        "29.74 s",
    "5.2 ms":
        "5.2 ms",
    "1.34 s":
        "1.34 s",
    "0.7 ms":
        "0.7 ms",
    "md_am_sith":
        "md_am_sith",
    "1.36 s":
        "1.36 s",
    "0.8 ms":
        "0.8 ms",
    "Open .bsp (decompile)":
        "Otevřít .bsp (dekompilace)",
    "7.71 s":
        "7.71 s",
    "7.98 s":
        "7.98 s",
    "2.01 s":
        "2.01 s",
    "2.64 s":
        "2.64 s",
    "3.29 s":
        "3.29 s",
    "3.42 s":
        "3.42 s",
    "Load .map":
        "Načíst .map",
    "1.31 s":
        "1.31 s",
    "1.29 s":
        "1.29 s",
    "662.5 ms":
        "662.5 ms",
    "717.4 ms":
        "717.4 ms",
    "805.6 ms":
        "805.6 ms",
    "827.7 ms":
        "827.7 ms",
    "Textures loaded":
        "Textury načteny",
    "1.64 s":
        "1.64 s",
    "1.80 s":
        "1.80 s",
    "4.2 ms":
        "4.2 ms",
    "821.6 ms":
        "821.6 ms",
    "1.93 s":
        "1.93 s",
    "2.16 s":
        "2.16 s",
    "Save .map":
        "Uložit .map",
    "815.8 ms":
        "815.8 ms",
    "745.4 ms":
        "745.4 ms",
    "142.5 ms":
        "142.5 ms",
    "173.7 ms":
        "173.7 ms",
    "175.9 ms":
        "175.9 ms",
    "189.6 ms":
        "189.6 ms",
    "Move everything":
        "Posunout vše",
    "285.1 ms":
        "285.1 ms",
    "329.4 ms":
        "329.4 ms",
    "53.2 ms":
        "53.2 ms",
    "72.0 ms":
        "72.0 ms",
    "62.9 ms":
        "62.9 ms",
    "68.0 ms":
        "68.0 ms",
    "Undo the move":
        "Vrátit posun",
    "294.1 ms":
        "294.1 ms",
    "353.0 ms":
        "353.0 ms",
    "26.2 ms":
        "26.2 ms",
    "36.4 ms":
        "36.4 ms",
    "33.3 ms":
        "33.3 ms",
    "32.6 ms":
        "32.6 ms",
    "Undo CSG Subtract":
        "Vrátit CSG odečíst",
    "171.1 ms":
        "171.1 ms",
    "207.4 ms":
        "207.4 ms",
    "8.6 ms":
        "8.6 ms",
    "7.9 ms":
        "7.9 ms",
    "12.1 ms":
        "12.1 ms",
    "10.1 ms":
        "10.1 ms",
    "Optimizer start":
        "Start optimalizátoru",
    "48.6 ms":
        "48.6 ms",
    "39.2 ms":
        "39.2 ms",
    "10.2 ms":
        "10.2 ms",
    "13.1 ms":
        "13.1 ms",
    "14.7 ms":
        "14.7 ms",
    "11.8 ms":
        "11.8 ms",
    "Optimizer check":
        "Kontrola optimalizátoru",
    "1.47 s":
        "1.47 s",
    "162.4 ms":
        "162.4 ms",
    "201.8 ms":
        "201.8 ms",
    "303.7 ms":
        "303.7 ms",
    "257.4 ms":
        "257.4 ms",
    "Switch to the map editor until the game data is ready":
        "Přepnout do editoru map, než jsou data hry připravena",
    "273.3 ms":
        "273.3 ms",
    "258.1 ms":
        "258.1 ms",
    "Preview: prepare the scene (md_am_sith)":
        "Náhled: příprava scény (md_am_sith)",
    "1.44 s":
        "1.44 s",
    "1.40 s":
        "1.40 s",
    "Preview: frame time, scene (mean)":
        "Náhled: čas snímku, scéna (průměr)",
    "15.0 ms":
        "15.0 ms",
    "Preview: frame time, free flight (mean)":
        "Náhled: čas snímku, volný let (průměr)",
    "8.2 ms":
        "8.2 ms",
    "8.1 ms":
        "8.1 ms",
    "Step":
        "Krok",
    "before (Stage 6)":
        "předtím (fáze 6)",
    "now":
        "now",
    "Map editor: speed and real-time light":
        "Editor map: rychlost a světlo v reálném čase",
    "See the light while you build: {{KeLichtMenue}} lights the camera on the graphics card, {{KeBackenMenue}} builds the map in the background and shows the real lightmaps. Building and editing got faster - with the same result as before.":
        "Vidět světlo během stavby: {{KeLichtMenue}} nasvětlí kameru na grafické kartě, {{KeBackenMenue}} sestaví mapu na pozadí a ukáže skutečné lightmapy. Sestavování a úpravy se zrychlily - se stejným výsledkem jako dřív.",
    "Real-time light":
        "Světlo v reálném čase",
    "{{KeLBuild}}: {{KeLichtMenue}} and {{KeBackenMenue}} at the end of the menu (not in NetRadiant-Custom).":
        "{{KeLBuild}}: {{KeLichtMenue}} a {{KeBackenMenue}} na konci nabídky (není v NetRadiant-Custom).",
    "Switch on **{{KeLBuild}} > {{KeLichtMenue}}**. The camera (render mode textured) is lit by the lights of the map - point lights, spotlights, the sun of the sky shader (`q3map_sun`) and surface lights (`q3map_surfacelight`) - with the formulas of q3map2 -light and with shadows.":
        "Zapněte **{{KeLBuild}} > {{KeLichtMenue}}**. Kamera (režim vykreslování texturovaný) je nasvětlena světly mapy - bodovými světly, reflektory, sluncem shaderu oblohy (`q3map_sun`) a světelnými povrchy (`q3map_surfacelight`) - podle vzorců q3map2 -light a se stíny.",
    "Move a light or change its keys: the camera follows at once. Up to 512 lights light each frame - every light whose reach touches the picture, the strongest first, also far away; the 128 most important cast shadows, the rest light without shadow.":
        "Posuňte světlo nebo změňte jeho klíče: kamera hned následuje. Každý snímek nasvětlí až 512 světel - každé světlo, jehož dosah zasahuje do obrazu, nejsilnější první, i daleko; 128 nejdůležitějších vrhá stíny, zbytek svítí bez stínu.",
    "Real-time light in a closed room with one light and a wall. Top right the camera says that this is a preview.":
        "Světlo v reálném čase v uzavřené místnosti s jedním světlem a zdí. Vpravo nahoře kamera říká, že jde o náhled.",
    "The real-time light is a **preview** and is never saved as a result: no bounce light, no filter, no supersampling, shadows from shadow maps instead of rays. The final light always comes from bhed_q3map2 - {{KeBackenMenue}} or a build from the {{KeLBuild}} menu.":
        "Světlo v reálném čase je **náhled** a nikdy se neukládá jako výsledek: žádné odražené světlo, žádný filtr, žádný supersampling, stíny ze stínových map místo paprsků. Konečné světlo vždy pochází z bhed_q3map2 - {{KeBackenMenue}} nebo sestavení z nabídky {{KeLBuild}}.",
    "Real-time light like in the game":
        "Světlo v reálném čase jako ve hře",
    "The preview follows the rules of q3map2 -light -fast, so that the picture matches the baked lightmaps (a real JKA mapper's test map was the yardstick):":
        "Náhled se řídí pravidly q3map2 -light -fast, aby obraz odpovídal zapečeným lightmapám (měřítkem byla testovací mapa skutečného mappera JKA):",
    "**Sky and sun:** the ray to the sun ends at the sky, as in q3map2 - the sky brush itself (caulk on its other sides, a lid above) no longer shadows the sun. `sun`, `q3map_sun`, `q3map_sunExt` and `q3map_skyLight` (many weak suns over the sky) all count; the sun has two shadow cascades (whole map, around the camera).":
        "**Obloha a slunce:** paprsek ke slunci končí u oblohy, jako v q3map2 - samotný brush oblohy (caulk na ostatních stranách, víko nad ním) už slunce nezastíní. Počítají se `sun`, `q3map_sun`, `q3map_sunExt` i `q3map_skyLight` (mnoho slabých sluncí po obloze); slunce má dvě kaskády stínů (celá mapa, okolí kamery).",
    "**Surface light:** `q3map_surfacelight` per triangle, divided by `q3map_lightSubdivide`, with the exact form factor and the back splash (`q3map_backsplash`); the colour from `q3map_lightRGB` or `q3map_lightImage`.":
        "**Světelný povrch:** `q3map_surfacelight` na trojúhelník, dělený podle `q3map_lightSubdivide`, s přesným konfiguračním faktorem a backsplash (`q3map_backsplash`); barva z `q3map_lightRGB` nebo `q3map_lightImage`.",
    "**What casts shadows:** only what casts shadows in q3map2 - world brushes and func_group (other entities only with `_castShadows`), no nodraw surfaces, transparent shaders only with `alphashadow`, patches only with **{{KeLBuild}} > {{KePatchSchatten}}** (also passed to the quick bake). Back faces go into the shadow maps, so outside light no longer leaks inside.":
        "**Co vrhá stíny:** jen to, co vrhá stíny v q3map2 - brushe světa a func_group (jiné entity jen s `_castShadows`), žádné povrchy nodraw, průhledné shadery jen s `alphashadow`, patche jen s **{{KeLBuild}} > {{KePatchSchatten}}** (předává se i rychlému zapečení). Zadní stěny jdou do stínových map, takže venkovní světlo už neprosakuje dovnitř.",
    "**Like in the game:** with real-time light (and with the quick bake shown) nodraw surfaces such as caulk disappear, and transparent shaders are drawn with the alphaFunc and blendFunc of their first stage.":
        "**Jako ve hře:** se světlem v reálném čase (a se zobrazeným rychlým zapečením) zmizí povrchy nodraw jako caulk a průhledné shadery se kreslí s alphaFunc a blendFunc své první vrstvy.",
    "A room whose ceiling is a sky brush (caulk on all other sides, a caulk lid above): the sun of the sky shader shines through the sky into the room, as after the bake.":
        "Místnost, jejíž strop je brush oblohy (caulk na všech ostatních stranách, víko z caulku nad ním): slunce shaderu oblohy svítí oblohou do místnosti, jako po zapečení.",
    "Measured against the quick bake at fixed camera points (md_ch_ben, mp/duel1, md_eitd_jedi with sky light, a large tester map): the mean brightness difference fell from 2-95 to 0.2-2 of 255. What remains is mostly the lightmap resolution (one luxel per 16 units) and bounce light, which the preview does not have.":
        "Změřeno proti rychlému zapečení v pevných bodech kamery (md_ch_ben, mp/duel1, md_eitd_jedi se světlem oblohy, velká mapa testera): průměrný rozdíl jasu klesl z 2-95 na 0.2-2 z 255. Co zbývá, je hlavně rozlišení lightmap (jeden luxel na 16 jednotek) a odražené světlo, které náhled nemá.",
    "Quick bake":
        "Rychlé zapečení",
    "Switch on **{{KeLBuild}} > {{KeBackenMenue}}**. Two seconds after the last change bhed_q3map2 builds the map in a temporary folder - `-meta`, `-vis -fast` (when the map has no leak) and `-light -fast` - at low priority, so the editor stays smooth.":
        "Zapněte **{{KeLBuild}} > {{KeBackenMenue}}**. Dvě sekundy po poslední změně bhed_q3map2 sestaví mapu v dočasné složce - `-meta`, `-vis -fast` (když mapa nemá únik) a `-light -fast` - s nízkou prioritou, takže editor zůstane plynulý.",
    "When it is done, the camera shows the real lightmaps of this build; top right it says **{{KeLichtGebacken}}**. Any change cancels a running build and starts a new one after the pause. Your map and a .bsp next to it are not touched.":
        "Když je hotovo, kamera ukáže skutečné lightmapy tohoto sestavení; vpravo nahoře stojí **{{KeLichtGebacken}}**. Každá změna běžící sestavení zruší a po pauze spustí nové. Vaší mapy ani .bsp vedle ní se to nedotkne.",
    "The same room after the quick bake: the lightmaps of bhed_q3map2.":
        "Tatáž místnost po rychlém zapečení: lightmapy z bhed_q3map2.",
    "The quick bake writes the same lightmaps, light grid and visibility as bhed_q3map2 on the command line with the same switches - the self-test compares the files. It is faster than a full build because of `-fast` and the missing bounce passes; for the release build use the {{KeLBuild}} menu.":
        "Rychlé zapečení zapíše stejné lightmapy, mřížku světla a viditelnost jako bhed_q3map2 na příkazovém řádku se stejnými přepínači - autotest soubory porovnává. Je rychlejší než úplné sestavení díky `-fast` a chybějícím průchodům odrazů; pro sestavení k vydání použijte nabídku {{KeLBuild}}.",
    "What got faster":
        "Co se zrychlilo",
    "Every change was measured before and after (median of three runs, low priority, the same maps) and gives the same result: the .bsp of -bsp and -light is byte for byte the one of a 1-thread build of Stage 6, the saved maps are identical.":
        "Každá změna byla změřena před a po (medián tří běhů, nízká priorita, stejné mapy) a dává stejný výsledek: .bsp z -bsp a -light je bajt po bajtu ten ze sestavení s 1 vláknem ve fázi 6, uložené mapy jsou shodné.",
    "**-light** uses all cores for the lightmaps (light by light in blocks, the adaptive supersampling predicted and checked exactly) and for the light grid.":
        "**-light** používá všechna jádra pro lightmapy (světlo po světle v blocích, adaptivní supersampling předpovězený a přesně zkontrolovaný) a pro mřížku světla.",
    "**-bsp**: the T-junction fix searches in parallel and applies in order; finding equal light grid points uses buckets instead of a search through all points.":
        "**-bsp**: oprava T-spojů hledá paralelně a aplikuje v pořadí; hledání stejných bodů mřížky světla používá přihrádky místo prohledávání všech bodů.",
    "**-vis** stays as in NetRadiant-Custom: with several threads it may differ slightly from build to build, with `-threads 1` it is always the same. Two ways to make it the same with several threads were tried and measured; both were slower.":
        "**-vis** zůstává jako v NetRadiant-Custom: s více vlákny se může mezi sestaveními mírně lišit, s `-threads 1` je vždy stejný. Dva způsoby, jak ho udělat stejným i s více vlákny, byly vyzkoušeny a změřeny; oba byly pomalejší.",
    "**Editor**: invert selection, the selection test of the views and the NPC files of the optimizer (read in the background).":
        "**Editor**: obrácení výběru, test výběru v pohledech a soubory NPC optimalizátoru (čtené na pozadí).",
    "kejim_post \"Textures loaded\" was so fast before only because Stage 6 loaded Jedi Outcast without its folders in this run (0 shaders, every face with the replacement image - the error fixed in Stage 7); now the 1187 shaders and their images are loaded. Steps that were not changed vary by about 20 % from run to run.":
        "kejim_post \"Textury načteny\" bylo dřív tak rychlé jen proto, že fáze 6 v tomto běhu načetla Jedi Outcast bez jeho složek (0 shaderů, každá stěna s náhradním obrázkem - chyba opravená ve fázi 7); teď se načte 1187 shaderů a jejich obrázky. Kroky, které se neměnily, kolísají mezi běhy asi o 20 %.",
    "Times on the test laptop (32 threads), median of three runs; \"-\" = not measured. Your computer will give other numbers, the ratio stays similar.":
        "Časy na testovacím notebooku (32 vláken), medián tří běhů; \"-\" = neměřeno. Váš počítač dá jiná čísla, poměr zůstane podobný.",
    "Map editor: build progress and partial compiles":
        "Editor map: průběh sestavení a dílčí kompilace",
    "See how far a build is and how long it still takes, build again only what changed - with the same result as a full build - and add a new part to a compiled map without compiling the map again.":
        "Podívejte se, jak daleko sestavení je a jak dlouho ještě potrvá, sestavte znovu jen to, co se změnilo - se stejným výsledkem jako úplné sestavení - a přidejte novou část do zkompilované mapy, aniž byste mapu znovu kompilovali.",
    "Build progress":
        "Průběh sestavení",
    "The window **{{KeFsTitel}}** in the second build of a map: step 3 (LIGHT), bounce 3 of 16, all steps at 43 % and the remaining time from the last build of this map.":
        "Okno **{{KeFsTitel}}** při druhém sestavení mapy: krok 3 (LIGHT), odraz 3 z 16, všechny kroky na 43 % a zbývající čas podle posledního sestavení této mapy.",
    "While a build from the {{KeLBuild}} menu runs, the window **{{KeFsTitel}}** shows the entry, the step and its stage (BSP, VIS, LIGHT), the phase q3map2 is in with its own progress - the dots and digits q3map2 prints, or the bounce - a bar over all steps and the time.":
        "Zatímco běží sestavení z nabídky {{KeLBuild}}, okno **{{KeFsTitel}}** ukazuje položku, krok a jeho fázi (BSP, VIS, LIGHT), fázi, ve které q3map2 je, s jejím vlastním průběhem - tečky a číslice, které q3map2 vypisuje, nebo odraz - pruh přes všechny kroky a čas.",
    "**Remaining time**: in the first build of a map with an entry it is estimated from the running phase and shown as \"at least\". From the second build on it comes from the times of the last build of this map with this entry - the step times are kept in `karte_bauzeiten.json` in the settings folder.":
        "**Zbývající čas**: při prvním sestavení mapy s danou položkou se odhaduje z běžící fáze a ukazuje jako \"nejméně\". Od druhého sestavení pochází z časů posledního sestavení této mapy s touto položkou - časy kroků se uchovávají v `karte_bauzeiten.json` ve složce nastavení.",
    "When the build is done, the console lists the time of each step and compares it with the last build: `1 BSP: 0.4 s (last build 0.5 s, -20 %)`.":
        "Když je sestavení hotové, konzole vypíše čas každého kroku a porovná ho s posledním sestavením: `1 BSP: 0.4 s (poslední sestavení 0.5 s, -20 %)`.",
    "**{{KeBmAbbrechen}}** in the window stops q3map2, like {{KeLBuild}} > {{KeBmAbbrechen}}.":
        "**{{KeBmAbbrechen}}** v okně zastaví q3map2, jako {{KeLBuild}} > {{KeBmAbbrechen}}.",
    "**{{KeLBuild}} > {{KeFsMelden}}** (on by default): if JK Studio is not the front window when a build ends, its taskbar button flashes - no sound, no dialog.":
        "**{{KeLBuild}} > {{KeFsMelden}}** (ve výchozím stavu zapnuto): není-li JK Studio oknem v popředí, když sestavení skončí, jeho tlačítko na hlavním panelu zabliká - bez zvuku, bez dialogu.",
    "The progress comes from q3map2's own output; a phase with many lightmaps or a long bounce moves in steps. The remaining time is only as good as the last build was comparable - another entry, other switches or a busy computer change it.":
        "Průběh pochází z vlastního výstupu q3map2; fáze s mnoha lightmapami nebo dlouhým odrazem postupuje po krocích. Zbývající čas je jen tak dobrý, jak srovnatelné bylo poslední sestavení - jiná položka, jiné přepínače nebo vytížený počítač ho změní.",
    "Partial compiles":
        "Dílčí kompilace",
    "Two switches in the {{KeLBuild}} menu let every build reuse what has not changed since the last one. The result is **byte for byte the .bsp of a full build** - the ctest `q3map2_teilbau` builds maps both ways and compares the files.":
        "Dva přepínače v nabídce {{KeLBuild}} nechají každé sestavení znovu použít, co se od posledního nezměnilo. Výsledkem je **bajt po bajtu .bsp úplného sestavení** - ctest `q3map2_teilbau` sestavuje mapy oběma způsoby a porovnává soubory.",
    "**{{KeTkVis}}**: `-vis` runs with `-bhedvisreuse`. bhed_q3map2 keeps the visibility in `<map>.bhedvis` together with the portal file and the switches. If the portals (`.prt`), the switches and the far plane key (`_farplanedist`) are the same, it copies the visibility instead of calculating it - the console says **visibility reused**. Portals change only with structural brushes; entities, detail brushes, patches and textures do not change them.":
        "**{{KeTkVis}}**: `-vis` běží s `-bhedvisreuse`. bhed_q3map2 uchovává viditelnost v `<map>.bhedvis` spolu se souborem portálů a přepínači. Jsou-li portály (`.prt`), přepínače a klíč vzdálené roviny (`_farplanedist`) stejné, viditelnost zkopíruje místo výpočtu - konzole hlásí **visibility reused**. Portály se mění jen se strukturálními brushi; entity, detailní brushe, patche a textury je nemění.",
    "**{{KeTkLicht}}**: `-light` runs with `-bhedlightcache`. bhed_q3map2 keeps every lightmap and the light grid in `<map>.bhedlight` with a fingerprint of everything that goes into it - the geometry, the lights that reach it, the switches. The next run calculates only the lightmaps whose fingerprint changed: move one light, and only the lightmaps it reaches are lit again. The console says how many came from the cache: `light cache: 9 of 14 lightmaps …`.":
        "**{{KeTkLicht}}**: `-light` běží s `-bhedlightcache`. bhed_q3map2 uchovává každou lightmapu a mřížku světla v `<map>.bhedlight` s otiskem všeho, co do nich vstupuje - geometrie, světel, která na ně dosáhnou, přepínačů. Další běh spočítá jen lightmapy, jejichž otisk se změnil: posuňte jedno světlo a znovu se nasvětlí jen lightmapy, na které dosáhne. Konzole řekne, kolik jich přišlo z mezipaměti: `light cache: 9 of 14 lightmaps …`.",
    "**Only the post-processing changed** - `-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` or the worldspawn keys `_minlight`, `_minvertexlight`, `_mingridlight`: every lightmap and the grid come from the cache, only the post-processing runs again, without tracing a single ray.":
        "**Změnilo se jen dodatečné zpracování** - `-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` nebo klíče worldspawn `_minlight`, `_minvertexlight`, `_mingridlight`: každá lightmapa i mřížka přijdou z mezipaměti, znovu běží jen dodatečné zpracování, bez trasování jediného paprsku.",
    "With `-bounce` the light cache is not used: bounce light couples every lightmap with every other one. With several threads `-vis` itself can differ slightly from run to run (see [[karte_tempo]]); the reused visibility is the one of the last run - for a comparison with a full build use `-vis -threads 1`. The cache files lie next to the map; delete them to force a full build. They do not belong into the .pk3.":
        "S `-bounce` se mezipaměť světla nepoužije: odražené světlo spojuje každou lightmapu s každou jinou. S více vlákny se může `-vis` sám mezi běhy mírně lišit (viz [[karte_tempo]]); znovu použitá viditelnost je ta z posledního běhu - pro porovnání s úplným sestavením použijte `-vis -threads 1`. Soubory mezipaměti leží vedle mapy; smažte je a vynutíte úplné sestavení. Do .pk3 nepatří.",
    "Adding to a compiled map":
        "Přidání do zkompilované mapy",
    "The dialog **{{KeZfTitel}}**: the compiled test map is the template (drawn in the camera, not editable), the new pillar in the layer \"Neu\" is the part to add.":
        "Dialog **{{KeZfTitel}}**: zkompilovaná testovací mapa je šablona (kreslená v kameře, neupravitelná), nový sloup ve vrstvě \"Neu\" je část k přidání.",
    "Open the source map you build the addition in - or a new map - and choose **{{KeLBuild}} > {{KeZfMenue}}**. As the template take a map of the game (**{{KeZfAusSpiel}}**) or any .bsp (**{{KeBspDatei}}**). It is drawn in the camera and the 2D views and cannot be selected or changed.":
        "Otevřete zdrojovou mapu, ve které přídavek stavíte - nebo novou mapu - a zvolte **{{KeLBuild}} > {{KeZfMenue}}**. Jako šablonu vezměte mapu hry (**{{KeZfAusSpiel}}**) nebo libovolný .bsp (**{{KeBspDatei}}**). Kreslí se v kameře a 2D pohledech a nelze ji vybrat ani změnit.",
    "Build the addition - brushes, patches, entities - in a layer of its own and pick it under **{{KeZfEbene}}** (the current layer is preselected).":
        "Postavte přídavek - brushe, patche, entity - ve vlastní vrstvě a vyberte ji pod **{{KeZfEbene}}** (aktuální vrstva je předvybraná).",
    "**{{KeZfPruefen}}** builds the layer and runs the checks below without adding anything. **{{KeZfBauen}}** builds the layer (`-meta`, `-light -fast -samples 2`) and inserts it with `q3map2 -mergebsp -world -fixnames`: as detail into the world of the template, whose BSP tree, visibility and lightmaps stay. The result is `<template>_merged.bsp` in the folder `karte_zusammen` in the settings folder; the template itself is not changed.":
        "**{{KeZfPruefen}}** sestaví vrstvu a provede kontroly níže, aniž by cokoli přidal. **{{KeZfBauen}}** sestaví vrstvu (`-meta`, `-light -fast -samples 2`) a vloží ji pomocí `q3map2 -mergebsp -world -fixnames`: jako detail do světa šablony, jejíž strom BSP, viditelnost a lightmapy zůstanou. Výsledkem je `<template>_merged.bsp` ve složce `karte_zusammen` ve složce nastavení; samotná šablona se nemění.",
    "After {{KeZfBauen}}: the light of the addition from the template, the collision check and the merged .bsp.":
        "Po {{KeZfBauen}}: světlo přídavku ze šablony, kontrola kolizí a sloučený .bsp.",
    "What is checked":
        "Co se kontroluje",
    "**Weather brushes**: `system/inside` and `system/outside` must not be mixed - after merging, a map with both does not load. If it would, nothing is added.":
        "**Brushe počasí**: `system/inside` a `system/outside` se nesmí míchat - po sloučení se mapa s oběma nenačte. Pokud by se tak stalo, nic se nepřidá.",
    "**Weather zones**: if the template or the addition has `misc_weather_zone` entities, inside and outside brushes only count inside a zone; weather brushes of the addition outside every zone get a warning, because they have no effect.":
        "**Zóny počasí**: má-li šablona nebo přídavek entity `misc_weather_zone`, vnitřní a venkovní brushe se počítají jen uvnitř zóny; brushe počasí přídavku mimo všechny zóny dostanou varování, protože nemají účinek.",
    "**Names**: a `target` or `targetname` of the addition that the template uses too is renamed by `-fixnames` - links between template and addition are lost. The dialog lists these names.":
        "**Názvy**: `target` nebo `targetname` přídavku, který používá i šablona, přejmenuje `-fixnames` - vazby mezi šablonou a přídavkem se ztratí. Dialog tyto názvy vypíše.",
    "**Limits**: the merged .bsp is counted like in the [[karte_optimierer|optimizer]]; a limit over the maximum is an error.":
        "**Limity**: sloučený .bsp se počítá jako v [[karte_optimierer|optimalizátoru]]; limit přes maximum je chyba.",
    "**Collision**: the merged .bsp is loaded into the game's collision model (OpenJK `CM_BoxTrace`, ported); every new solid brush must stop the player box when it walks into it from outside.":
        "**Kolize**: sloučený .bsp se načte do kolizního modelu hry (OpenJK `CM_BoxTrace`, převedeno); každý nový pevný brush musí zastavit krabici hráče, když do něj vejde zvenku.",
    "The light of the addition is **approximated**: q3map2 cannot light the addition against the geometry of the template. {{KeZfLichtGitter}} takes the average of the template's light grid around the addition as ambient light, {{KeZfLichtLichter}} the lights without a targetname that stayed in the template - only a map built with {{KeEdMenue}} (`-keeplights`) has them. The light grid of the template is not updated - models and players near the addition are lit as before. For the final map build everything in full.":
        "Světlo přídavku je **přibližné**: q3map2 nemůže přídavek nasvětlit proti geometrii šablony. {{KeZfLichtGitter}} bere jako okolní světlo průměr mřížky světla šablony kolem přídavku, {{KeZfLichtLichter}} světla bez targetname, která v šabloně zůstala - má je jen mapa sestavená s {{KeEdMenue}} (`-keeplights`). Mřížka světla šablony se neaktualizuje - modely a hráči u přídavku jsou nasvětleni jako dřív. Pro konečnou mapu vše sestavte celé.",
    "Weather rules from the JKHub thread on weather brushes: never mix inside and outside brushes in one map; with weather zones the brushes only work inside a zone; the game makes the `.weather` cache file again when the map loads, so an old one need not be deleted.":
        "Pravidla počasí z vlákna JKHub o brushích počasí: nikdy nemíchejte vnitřní a venkovní brushe v jedné mapě; se zónami počasí fungují brushe jen uvnitř zóny; hra soubor mezipaměti `.weather` vytvoří při načtení mapy znovu, takže starý není třeba mazat.",
    "Map editor: terrain, playing, logic, mirror and more":
        "Editor map: terén, hraní, logika, zrcadlo a další",
    "Generate terrain, walk through the map without compiling, see the links between entities, check lightmap density and collision, edit symmetric maps with the mirror mode and jump through the undo history.":
        "Generujte terén, procházejte mapu bez kompilace, prohlížejte vazby mezi entitami, kontrolujte hustotu lightmap a kolize, upravujte symetrické mapy zrcadlovým režimem a skákejte historií kroků zpět.",
    "Everything on this page except the Terrain Generator is **not in NetRadiant-Custom**: each function has its own menu entry with that hint, and with all of them off the editor behaves like NetRadiant-Custom. The new entries are at the end of **{{KeLView}}**, above the program settings.":
        "Vše na této stránce kromě generátoru terénu **není v NetRadiant-Custom**: každá funkce má vlastní položku nabídky s touto poznámkou a se všemi vypnutými se editor chová jako NetRadiant-Custom. Nové položky jsou na konci **{{KeLView}}**, nad nastavením programu.",
    "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} and {{KeUlMenue}}.":
        "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} a {{KeUlMenue}}.",
    "Terrain Generator":
        "Generátor terénu",
    "**Plugins > TerrainGenerator > Generate Terrain** is the plugin from the current NetRadiant-Custom source (master, after the release 20260114), ported line by line: the same heights, triangles and textures as the original for the same values. The dialog stays open while you work in the views.":
        "**Plugins > TerrainGenerator > Generate Terrain** je plugin z aktuálního zdrojového kódu NetRadiant-Custom (master, po vydání 20260114), převedený řádek po řádku: pro stejné hodnoty stejné výšky, trojúhelníky a textury jako originál. Dialog zůstane otevřený, zatímco pracujete v pohledech.",
    "The dialog with **{{KeTgUseSel}}**: the size comes from the selected brush.":
        "Dialog s **{{KeTgUseSel}}**: velikost pochází z vybraného brushe.",
    "A **{{KeTgShHill}}** made from the selected brush - one func_group of detail brushes.":
        "**{{KeTgShHill}}** vytvořený z vybraného brushe - jedna func_group z detailních brushů.",
    "**{{KeTgTarget}}** {{KeTgUseSel}} takes the size of the selected brush and replaces it; {{KeTgManual}} builds the terrain around the origin.":
        "**{{KeTgTarget}}** {{KeTgUseSel}} vezme velikost vybraného brushe a nahradí ho; {{KeTgManual}} postaví terén kolem počátku.",
    "**{{KeTgBaseShape}}** (hill, crater, ridge, slope, volcano, valley, tunnel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, terraces and the grid step as in the original; **{{KeTgGenerate}}** is one undo step.":
        "**{{KeTgBaseShape}}** (kopec, kráter, hřbet, svah, sopka, údolí, tunel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, terasy a krok mřížky jako v originálu; **{{KeTgGenerate}}** je jeden krok zpět.",
    "Own addition: **{{KeTgSeed}} {{KeTgSeedFix}}** starts the random numbers with a fixed value - the same values then give the same terrain again. Off (default), the random sequence runs on as in NetRadiant-Custom.":
        "Vlastní doplněk: **{{KeTgSeed}} {{KeTgSeedFix}}** spustí náhodná čísla pevnou hodnotou - stejné hodnoty pak dají znovu stejný terén. Vypnuto (výchozí) běží náhodná posloupnost dál jako v NetRadiant-Custom.",
    "Free split windows":
        "Volná rozdělená okna",
    "**Preferences > Interface > Layout > {{KeSfSchalter}}** (layout Split): every one of the four windows gets a small menu in its corner - camera, XY, XZ or YZ, also two windows with the same view. Ctrl+Tab changes the type of the active 2D window, the splitter positions are kept. Off (default), the four windows are fixed as in NetRadiant-Custom.":
        "**Preferences > Rozhraní > Rozvržení > {{KeSfSchalter}}** (rozvržení Rozdělené): každé ze čtyř oken dostane v rohu malou nabídku - kamera, XY, XZ nebo YZ, i dvě okna se stejným pohledem. Ctrl+Tab mění typ aktivního 2D okna, polohy rozdělovačů se zachovají. Vypnuto (výchozí) jsou čtyři okna pevná jako v NetRadiant-Custom.",
    "Play in the editor":
        "Hraní v editoru",
    "**{{KeSpMenue}}** (F6): walking through the open map - position and speed at the top, the keys at the bottom.":
        "**{{KeSpMenue}}** (F6): procházka otevřenou mapou - poloha a rychlost nahoře, klávesy dole.",
    "No compile: the collision comes straight from the brushes and patches with the contents q3map2 would give them (surfaceparms of the game, clip brushes, nonsolid, water ...). Walking, jumping, stairs and slopes use the port of the game's movement code (Pmove) - the same as the preview of the program: JKA and Movie Duels, or the Jedi Outcast rules.":
        "Žádná kompilace: kolize pocházejí přímo z brushů a patchů s obsahem, který by jim dal q3map2 (surfaceparms hry, clip brushe, nonsolid, voda ...). Chůze, skoky, schody a svahy používají port kódu pohybu hry (Pmove) - tentýž jako náhled programu: JKA a Movie Duels, nebo pravidla Jedi Outcast.",
    "W/A/S/D move, Space jumps, C or Ctrl crouches, Shift walks, the mouse looks, N switches noclip, **Esc ends** and puts the camera back where it was.":
        "W/A/S/D pohyb, mezerník skok, C nebo Ctrl skrčení, Shift chůze, myš rozhlížení, N přepíná noclip, **Esc ukončí** a vrátí kameru, kde byla.",
    "**{{KeSpSpawn}}**: on, the start is the first info_player_start (its origin + 9 and its angle, as the game picks it); off, the start is the camera.":
        "**{{KeSpSpawn}}**: zapnuto je startem první info_player_start (jeho origin + 9 a jeho úhel, jak ho vybírá hra); vypnuto je startem kamera.",
    "Only the player moves: no doors, movers, triggers, NPCs or weapons. The map is not changed - playing is not an undo step.":
        "Pohybuje se jen hráč: žádné dveře, movery, triggery, NPC ani zbraně. Mapa se nemění - hraní není krok zpět.",
    "Logic view":
        "Logický pohled",
    "**{{KeLgTitel}}**: trigger_multiple fires target_relay, which opens the door and spawns the NPC; the NPC's scripts below it.":
        "**{{KeLgTitel}}**: trigger_multiple spustí target_relay, který otevře dveře a spawne NPC; pod ním skripty NPC.",
    "**{{KeLgMenue}}** draws every link between the entities as a graph from left to right: target, target2 ... target5, NPC_target, the targets of doors and every *target* key of the entity definitions; the script keys (spawnscript, usescript, deathscript ...) are listed in the box of the entity.":
        "**{{KeLgMenue}}** kreslí každou vazbu mezi entitami jako graf zleva doprava: target, target2 ... target5, NPC_target, cíle dveří a každý klíč *target* z definic entit; skriptové klíče (spawnscript, usescript, deathscript ...) jsou vypsány v rámečku entity.",
    "A click on a box selects the entity and centers the 2D views and the camera on it; a double-click on a script line opens the ICARUS script in the script editor. Mouse wheel zooms, right or middle mouse button moves.":
        "Kliknutí na rámeček vybere entitu a vystředí na ni 2D pohledy a kameru; dvojklik na řádek skriptu otevře skript ICARUS v editoru skriptů. Kolečko myši přibližuje, pravé nebo prostřední tlačítko posouvá.",
    "A target without a matching targetname appears as a red box - a link that leads nowhere in the game.":
        "Cíl bez odpovídajícího targetname se objeví jako červený rámeček - vazba, která ve hře nikam nevede.",
    "Camera view modes":
        "Režimy pohledu kamery",
    "**{{KeAmDichte}}**: one square is one lightmap luxel.":
        "**{{KeAmDichte}}**: jeden čtverec je jeden luxel lightmapy.",
    "**{{KeAmKollision}}**: the trigger brush in green - it does not block the player.":
        "**{{KeAmKollision}}**: brush triggeru zeleně - hráče neblokuje.",
    "**{{KeAmDichte}}**: a checkerboard of the luxel size per face, computed like q3map2 (q3map_lightmapSampleSize, _samplesize and _lightmapscale of the entity, -samplesize; enlarged until the face fits into 128 luxels). Finer than 16 reddish, coarser bluish, faces without lightmap grey.":
        "**{{KeAmDichte}}**: šachovnice o velikosti luxelu na stěnu, spočítaná jako v q3map2 (q3map_lightmapSampleSize, _samplesize a _lightmapscale entity, -samplesize; zvětšeno, dokud se stěna nevejde do 128 luxelů). Jemnější než 16 načervenalé, hrubší namodralé, stěny bez lightmapy šedé.",
    "**{{KeAmKollision}}**: brushes by the contents q3map2 gives them - playerclip magenta, monsterclip orange, botclip yellow, shotclip cyan, nodrop dark red, water, slime, lava, fog, triggers. Visible walls the player walks through (no player collision) glow bright red.":
        "**{{KeAmKollision}}**: brushe podle obsahu, který jim dá q3map2 - playerclip purpurová, monsterclip oranžová, botclip žlutá, shotclip azurová, nodrop tmavě červená, voda, sliz, láva, mlha, triggery. Viditelné zdi, kterými hráč projde (bez kolize s hráčem), září jasně červeně.",
    "**{{KeAmSicht}}** and **{{KeAmLast}}** need the compiled .bsp next to the map (with VIS): what the engine draws from the camera's cluster in green, or a heat map of the triangles drawn from each place. The numbers are shown at the top left of the camera.":
        "**{{KeAmSicht}}** a **{{KeAmLast}}** potřebují zkompilovaný .bsp vedle mapy (s VIS): co engine kreslí z clusteru kamery, zeleně, nebo teplotní mapu trojúhelníků kreslených z každého místa. Čísla se ukazují vlevo nahoře v kameře.",
    "Mirror mode":
        "Zrcadlový režim",
    "**{{KeSgMenue}} > {{KeSgX}}**: the pillar was moved 64 units to the right - its counterpart on the other side of the magenta plane followed.":
        "**{{KeSgMenue}} > {{KeSgX}}**: sloup byl posunut o 64 jednotek doprava - jeho protějšek na druhé straně purpurové roviny ho následoval.",
    "**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} or {{KeSgZ}}; the plane goes through the origin or, with **{{KeSgMitte}}**, through the center of the selection on the grid. It is drawn as a magenta line in the 2D views.":
        "**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} nebo {{KeSgZ}}; rovina jde počátkem nebo s **{{KeSgMitte}}** středem výběru na mřížce. Ve 2D pohledech se kreslí jako purpurová čára.",
    "Every change to the selection is written to its counterpart in the same undo step - live while dragging. Brushes and patches count one by one, point entities and groups as a whole; origin, angle and angles are mirrored, textures like **Flip** (with Texture Lock as set).":
        "Každá změna výběru se zapíše na jeho protějšek ve stejném kroku zpět - živě při tažení. Brushe a patche se počítají jeden po druhém, bodové entity a skupiny jako celek; origin, angle a angles se zrcadlí, textury jako **Flip** (se Zámkem textury podle nastavení).",
    "If the exact mirror image already exists, it becomes the counterpart; otherwise the first change creates a mirrored copy. Deleting removes both; undo brings back both exactly. Objects that lie symmetric on the plane get no counterpart.":
        "Existuje-li už přesný zrcadlový obraz, stane se protějškem; jinak první změna vytvoří zrcadlenou kopii. Odstranění odebere obojí; Zpět přesně vrátí obojí. Objekty ležící symetricky na rovině protějšek nedostanou.",
    "Copied entities keep their keys, also targetname and target - rename them where the two sides should act separately.":
        "Zkopírované entity si ponechají své klíče, i targetname a target - přejmenujte je tam, kde mají obě strany jednat odděleně.",
    "Undo history":
        "Historie kroků zpět",
    "**{{KeUlTitel}}**: the start, three steps with the command names NetRadiant-Custom records, the current state highlighted.":
        "**{{KeUlTitel}}**: začátek, tři kroky s názvy příkazů, které NetRadiant-Custom zaznamenává, aktuální stav zvýrazněný.",
    "**{{KeUlMenue}}** lists all undo steps (oldest first) and below them the redo steps in grey. A click jumps to the state after that step with as many Undo or Redo as needed - each with its console line, as with Ctrl+Z and Ctrl+Y. The list goes back as far as the undo queue (**Preferences > Undo Queue Size**, default 512).":
        "**{{KeUlMenue}}** vypíše všechny kroky zpět (nejstarší první) a pod nimi šedě kroky znovu. Kliknutí skočí do stavu po daném kroku s tolika Zpět nebo Znovu, kolik je třeba - každý se svým řádkem v konzoli, jako u Ctrl+Z a Ctrl+Y. Seznam sahá zpět tak daleko jako fronta kroků zpět (**Preferences > Velikost fronty kroků zpět**, výchozí 512).",
    "misc_model from all model folders":
        "misc_model ze všech složek modelů",
    "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) now looks for models in all model folders of the game, not only in the folders of the map's shaders. A model index (per model and mesh: vertex count, triangle set and shader) lies in the settings folder as `karte_modellindex_<…>.txt`. The first time - and for new or changed .pk3 files - bhed_q3map2 loads the missing models once (about half a minute for a big install); after that only the models whose meshes match a triangle surface of the .bsp, or whose shader the .bsp uses, are loaded. The console names the index and the candidates.":
        "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) teď hledá modely ve všech složkách modelů hry, nejen ve složkách shaderů mapy. Index modelů (na model a síť: počet vrcholů, sada trojúhelníků a shader) leží ve složce nastavení jako `karte_modellindex_<…>.txt`. Napoprvé - a pro nové nebo změněné soubory .pk3 - bhed_q3map2 jednou načte chybějící modely (asi půl minuty u velké instalace); poté se načtou jen modely, jejichž sítě odpovídají povrchu z trojúhelníků v .bsp, nebo jejichž shader .bsp používá. Konzole uvede index a kandidáty.",
    "before":
        "předtím",
    "jeditemple_garden (source known)":
        "jeditemple_garden (zdroj známý)",
    "GPU shadow tests (switched off)":
        "Testy stínů na GPU (vypnuté)",
    "bhed_q3map2 has the command-line switch `-bhedgpu` for `-light`: the test \"does the ray from the luxel to the light hit something?\" runs on the graphics card (Direct3D 11), the light itself stays on the CPU. The result is **byte for byte the same** as without it - checked on five maps and by the ctest `q3map2_gpu`. On the measured computer (32 threads, RTX 4070 Laptop) it was not faster (mp/duel1 2.3 s → 3.2 s, kejim_post 10.0 s → 11.3 s), so it is not in the Build menu and stays off.":
        "bhed_q3map2 má pro `-light` přepínač příkazového řádku `-bhedgpu`: test \"zasáhne paprsek od luxelu ke světlu něco?\" běží na grafické kartě (Direct3D 11), samotné světlo zůstává na CPU. Výsledek je **bajt po bajtu stejný** jako bez něj - ověřeno na pěti mapách a ctestem `q3map2_gpu`. Na měřeném počítači (32 vláken, RTX 4070 Laptop) to nebylo rychlejší (mp/duel1 2.3 s → 3.2 s, kejim_post 10.0 s → 11.3 s), takže to není v nabídce Build a zůstává vypnuto.",
    "Your first map: a room to walk in":
        "Vaše první mapa: místnost k procházení",
    "From an empty map to a room you can walk through in the game: brushes, textures, caulk, a light, the player start, the optimizer, building with the progress window, walking through it in the editor - and the .pk3 for the game.":
        "Od prázdné mapy k místnosti, kterou můžete ve hře projít: brushe, textury, caulk, světlo, start hráče, optimalizátor, sestavení s oknem průběhu, procházka v editoru - a .pk3 pro hru.",
    "Map editor · about 15 minutes":
        "Editor map · asi 15 minut",
    "Every picture on this page was taken while the program did exactly these steps, and the self-test checks the result every time: the room builds without a leak, the player stands at the start and can walk, and the .pk3 holds the map. The menus and shortcuts are those of NetRadiant-Custom, so other mapping tutorials fit as well - the page [[karteneditor]] explains the windows.":
        "Každý obrázek na této stránce vznikl, zatímco program dělal přesně tyto kroky, a autotest pokaždé kontroluje výsledek: místnost se sestaví bez úniku, hráč stojí na startu a může chodit a .pk3 obsahuje mapu. Nabídky a zkratky jsou ty z NetRadiant-Custom, takže sedí i jiné tutoriály k mapování - okna vysvětluje stránka [[karteneditor]].",
    "**Set up the game.** Switch the mode bar to **{{ViewMapEditor}}**. The editor uses the game the program is set up for; its folders come from the program settings. **{{KeLEdit}} > {{KeLPreferences}}**, page **{{KePgGame}}**, shows the game, page **{{KePgPaths}}** the engine path (empty: taken from the program's game folders).":
        "**Nastavte hru.** Přepněte lištu režimů na **{{ViewMapEditor}}**. Editor používá hru, pro kterou je program nastaven; její složky pocházejí z nastavení programu. **{{KeLEdit}} > {{KeLPreferences}}**, stránka **{{KePgGame}}**, ukazuje hru, stránka **{{KePgPaths}}** cestu k enginu (prázdná: převzato ze složek hry programu).",
    "{{KeLPreferences}}, page {{KePgPaths}}.":
        "{{KeLPreferences}}, stránka {{KePgPaths}}.",
    "**New map.** **{{KeLFile}} > {{KeLNewMap}}** starts an empty map - only the worldspawn. Zoom the 2D view out a little with the mouse wheel.":
        "**Nová mapa.** **{{KeLFile}} > {{KeLNewMap}}** začne prázdnou mapu - jen worldspawn. Kolečkem myši 2D pohled trochu oddalte.",
    "The {{KeLFile}} menu.":
        "Nabídka {{KeLFile}}.",
    "**Draw a brush.** In the 2D top view, drag with the left mouse button from one corner to the other - here from -256 -256 to 256 256, a box 512 units wide. Its height comes from the work zone (at first -64 to 64).":
        "**Nakreslete brush.** Ve 2D pohledu shora táhněte levým tlačítkem myši od jednoho rohu k druhému - zde od -256 -256 do 256 256, krabice široká 512 jednotek. Její výška pochází z pracovní zóny (zpočátku -64 až 64).",
    "The new brush, selected (red).":
        "Nový brush, vybraný (červeně).",
    "**Make it taller.** <kbd>Num 1</kbd> switches the 2D view to the front view (XZ). Grab just above the top edge and drag up: the top face follows on the grid. Here to 192 - the room is now 256 high. <kbd>Num 7</kbd> goes back to the top view.":
        "**Zvyšte ho.** <kbd>Num 1</kbd> přepne 2D pohled na pohled zepředu (XZ). Uchopte těsně nad horní hranou a táhněte nahoru: horní stěna následuje po mřížce. Zde na 192 - místnost je teď 256 vysoká. <kbd>Num 7</kbd> se vrátí na pohled shora.",
    "The front view: the brush is now 256 high.":
        "Pohled zepředu: brush je teď 256 vysoký.",
    "**Texture.** Double-click a folder in the tree of the texture browser and click a texture with the left mouse button: every face of the selected brush gets it. Pick something without transparency for the walls.":
        "**Textura.** Dvakrát klikněte na složku ve stromu prohlížeče textur a klikněte levým tlačítkem na texturu: dostane ji každá stěna vybraného brushe. Na zdi vyberte něco bez průhlednosti.",
    "The wall texture on all six faces.":
        "Textura zdi na všech šesti stěnách.",
    "**Hollow it out.** The toolbar button **{{KeCsgTitle}}** opens the CSG tool. Leave the thickness at 16 and switch the caulk button off for now (AutoCaulk does that later), then press **{{KeCsgTipPull}}**: six walls 16 thick grow outwards around the room, and the inside stays exactly the box you drew. The toolbar button **Room** (Make Room) does the same with the grid size as thickness.":
        "**Vydlabejte ho.** Tlačítko panelu nástrojů **{{KeCsgTitle}}** otevře nástroj CSG. Tloušťku ponechte na 16 a tlačítko caulk zatím vypněte (to později udělá AutoCaulk), pak stiskněte **{{KeCsgTipPull}}**: kolem místnosti vyrostou ven šest zdí o tloušťce 16 a vnitřek zůstane přesně krabicí, kterou jste nakreslili. Tlačítko panelu nástrojů **Room** (Make Room) dělá totéž s velikostí mřížky jako tloušťkou.",
    "{{KeCsgTitle}} with the caulk button off.":
        "{{KeCsgTitle}} s vypnutým tlačítkem caulk.",
    "**A floor texture.** In the front view, <kbd>Shift</kbd> + left click on the floor selects only the floor brush; click a floor texture in the browser. <kbd>Esc</kbd> clears the selection.":
        "**Textura podlahy.** V pohledu zepředu vybere <kbd>Shift</kbd> + levé kliknutí na podlahu jen brush podlahy; klikněte v prohlížeči na texturu podlahy. <kbd>Esc</kbd> zruší výběr.",
    "The hollow room: six wall brushes.":
        "Dutá místnost: šest brushů zdí.",
    "**A light.** Right-click inside the room in the 2D view: the entity menu opens. **light** asks for the intensity (300 is a good start) - **OK** places it where you clicked.":
        "**Světlo.** Klikněte pravým tlačítkem uvnitř místnosti ve 2D pohledu: otevře se nabídka entit. **light** se zeptá na intenzitu (300 je dobrý začátek) - **OK** ho umístí tam, kam jste klikli.",
    "**Where the player starts.** Right-click near the floor: **info > info_player_start**. Without it the game does not know where to put the player. Its box stands on the floor.":
        "**Kde hráč začíná.** Klikněte pravým tlačítkem blízko podlahy: **info > info_player_start**. Bez něj hra neví, kam hráče umístit. Jeho krabice stojí na podlaze.",
    "The entity menu, submenu info.":
        "Nabídka entit, podnabídka info.",
    "**Save.** **{{KeLFile}} > {{KeLSaveAs}}** - for example `meinekarte.map` in a folder `maps` of your own, outside the game folders. The name of the file becomes the name of the map in the game.":
        "**Uložení.** **{{KeLFile}} > {{KeLSaveAs}}** - například `meinekarte.map` ve vlastní složce `maps`, mimo složky hry. Název souboru se stane názvem mapy ve hře.",
    "The saved room in the editor.":
        "Uložená místnost v editoru.",
    "**Caulk the outside.** Faces nobody can see still cost lightmap space and drawing time. Fly the camera into the room, drag a rectangle around the whole room with <kbd>Shift</kbd> + right mouse button in the top view and press <kbd>F4</kbd> (**{{KeLBrush}} > {{KeLAutoCaulkSelected}}**): every face that cannot be seen from inside becomes caulk. Building in caulk from the start works too: give new brushes the caulk texture and texture only the faces you see.":
        "**Caulk zvenku.** Stěny, které nikdo nevidí, stále stojí místo v lightmapě a čas kreslení. Vleťte kamerou do místnosti, v pohledu shora natáhněte pomocí <kbd>Shift</kbd> + pravého tlačítka obdélník kolem celé místnosti a stiskněte <kbd>F4</kbd> (**{{KeLBrush}} > {{KeLAutoCaulkSelected}}**): každá stěna, kterou nelze vidět zevnitř, dostane caulk. Stavět v caulku od začátku funguje také: dejte novým brushům texturu caulk a texturujte jen stěny, které vidíte.",
    "After AutoCaulk: the camera inside the room.":
        "Po AutoCaulk: kamera uvnitř místnosti.",
    "**Check it.** **{{KeLBuild}} > {{KeOptMenue}}** counts the game's limits and looks for build problems before you compile. A green room has nothing to fix.":
        "**Kontrola.** **{{KeLBuild}} > {{KeOptMenue}}** spočítá limity hry a hledá problémy sestavení, než zkompilujete. Zelená místnost nemá co opravovat.",
    "The optimizer for the tutorial room.":
        "Optimalizátor pro místnost z tutoriálu.",
    "**Build.** **{{KeLBuild}} > [Final] Fast** runs q3map2 three times - BSP, VIS and the light with bounce. The progress window shows the stage, the progress and the time left. If the map leaked, the console says so in red and a line in the 2D view shows the way out - see [[karte_bauen]].":
        "**Sestavení.** **{{KeLBuild}} > [Final] Fast** spustí q3map2 třikrát - BSP, VIS a světlo s odrazy. Okno průběhu ukazuje fázi, průběh a zbývající čas. Pokud mapa propouští, konzole to oznámí červeně a čára ve 2D pohledu ukáže cestu ven - viz [[karte_bauen]].",
    "The progress window during the build.":
        "Okno průběhu během sestavení.",
    "**Walk through it.** **{{KeLView}} > {{KeSpSpawn}}** and then **{{KeSpMenue}}** (<kbd>F6</kbd>): you stand at the info_player_start and walk with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, jump with <kbd>Space</kbd>; <kbd>Esc</kbd> ends. This works even without compiling - the collision comes straight from the brushes (see [[karte_werkzeuge8b#spielen]]).":
        "**Projděte ji.** **{{KeLView}} > {{KeSpSpawn}}** a pak **{{KeSpMenue}}** (<kbd>F6</kbd>): stojíte na info_player_start a chodíte pomocí <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, skáčete <kbd>Space</kbd>; <kbd>Esc</kbd> ukončí. Funguje to i bez kompilace - kolize pocházejí přímo z brushů (viz [[karte_werkzeuge8b#spielen]]).",
    "Walking through the room in the editor.":
        "Procházka místností v editoru.",
    "**Into the game.** **{{KeLBuild}} > {{KePkMenue}}** packs `maps/meinekarte.bsp` into `meinekarte.pk3` next to the .bsp. The program never writes into the game folders - copy the .pk3 yourself into `GameData/base` (Jedi Academy, Jedi Outcast) or into the folder of your mod (for example Movie Duels). Start the game, open the console with <kbd>Shift</kbd>+<kbd>~</kbd> and type `devmap meinekarte`.":
        "**Do hry.** **{{KeLBuild}} > {{KePkMenue}}** zabalí `maps/meinekarte.bsp` do `meinekarte.pk3` vedle .bsp. Program nikdy nezapisuje do složek hry - zkopírujte .pk3 sami do `GameData/base` (Jedi Academy, Jedi Outcast) nebo do složky svého modu (například Movie Duels). Spusťte hru, otevřete konzoli pomocí <kbd>Shift</kbd>+<kbd>~</kbd> a zadejte `devmap meinekarte`.",
    "{{KeLBuild}} with **{{KePkMenue}}** (not in NetRadiant-Custom).":
        "{{KeLBuild}} s **{{KePkMenue}}** (není v NetRadiant-Custom).",
    "Where to go next":
        "Kam dál",
    "Brushes, clipper, patches and the CSG tools: [[karteneditor_bauen]].":
        "Brushe, ořezávač, patche a nástroje CSG: [[karteneditor_bauen]].",
    "Build menu, leaks, portals and the build folder: [[karte_bauen]]; faster rebuilds: [[karte_teilbau]].":
        "Nabídka Build, úniky, portály a složka sestavení: [[karte_bauen]]; rychlejší opětovná sestavení: [[karte_teilbau]].",
    "Limits of the game and fixes: [[karte_optimierer]]; the light preview in the camera: [[karte_tempo]].":
        "Limity hry a opravy: [[karte_optimierer]]; náhled světla v kameře: [[karte_tempo]].",
    "Terrain, logic view, mirror mode and playing in the editor: [[karte_werkzeuge8b]].":
        "Terén, logický pohled, zrcadlový režim a hraní v editoru: [[karte_werkzeuge8b]].",
    "A cutscene on your map: [[tut-projekt]].":
        "Animační sekvence na vaší mapě: [[tut-projekt]].",
    "Map editor: GPU bake":
        "Editor map: zapékání na GPU",
    "Compute the whole light of a map on the graphics card - lightmaps, bounce and light grid - byte for byte the same as on the CPU, and much faster on big maps.":
        "Spočítejte celé světlo mapy na grafické kartě - lightmapy, odrazy a mřížku světla - bajt po bajtu stejně jako na CPU a na velkých mapách mnohem rychleji.",
    "**{{KeLBuild}} > {{KeGpuMenue}}** (not in NetRadiant-Custom) decides where bhed_q3map2 computes the light of every `-light` step: **{{KeGpuAuto}}** (the default), **{{KeGpuEin}}** (switch `-bhedgpubake`) or **{{KeGpuAus}}**. The compiled map is **the same, byte for byte** - the graphics card is only faster. The choice is kept (setting `BhedGpuBakeMode`).":
        "**{{KeLBuild}} > {{KeGpuMenue}}** (není v NetRadiant-Custom) rozhoduje, kde bhed_q3map2 počítá světlo každého kroku `-light`: **{{KeGpuAuto}}** (výchozí), **{{KeGpuEin}}** (přepínač `-bhedgpubake`) nebo **{{KeGpuAus}}**. Zkompilovaná mapa je **stejná, bajt po bajtu** - grafická karta je jen rychlejší. Volba se pamatuje (nastavení `BhedGpuBakeMode`).",
    "**{{KeGpuAuto}}** takes the graphics card only when its result is bit-identical (it computes with double precision) and it is expected to be faster: large maps and bounce rounds. Small maps, and `-samples 4` on large maps, stay on the processor; a graphics card without double precision is never chosen. It also remembers, per map and light switches, how long the last processor run and the last graphics card run took, and from then on takes the faster one (also for the quick bake). The console names the choice and the reason, for example \"GPU bake (Auto): processor - little light work (34490 super luxels)\".":
        "**{{KeGpuAuto}}** vezme grafickou kartu jen tehdy, když je její výsledek bitově shodný (počítá s dvojnásobnou přesností) a očekává se, že bude rychlejší: velké mapy a kola odrazů. Malé mapy a `-samples 4` na velkých mapách zůstávají na procesoru; grafická karta bez dvojnásobné přesnosti se nikdy nezvolí. Pamatuje si také, pro každou mapu a přepínače světla, jak dlouho trval poslední běh procesoru a poslední běh grafické karty, a od té doby bere rychlejší (i pro rychlé zapečení). Konzole uvede volbu a důvod, například \"Zapékání na GPU (Auto): procesor - málo práce se světlem (34490 superluxelů)\".",
    "{{KeLBuild}}: {{KeGpuMenue}}, next to the partial compiles.":
        "{{KeLBuild}}: {{KeGpuMenue}}, vedle dílčích kompilací.",
    "While the map builds, the build progress window and the status line name the graphics card. After the build the console shows the **deviation report**: every 64th lightmap is computed a second time on the CPU and compared luxel by luxel. The last report is also in the tooltip of {{KeGpuMenue}}. The quick bake of the real-time light ([[karte_tempo]]) uses the graphics card too when the entry is on.":
        "Zatímco se mapa sestavuje, okno průběhu sestavení a stavový řádek jmenují grafickou kartu. Po sestavení ukáže konzole **zprávu o odchylkách**: každá 64. lightmapa se spočítá podruhé na CPU a porovná luxel po luxelu. Poslední zpráva je také v popisku {{KeGpuMenue}}. Rychlé zapečení světla v reálném čase ([[karte_tempo]]) používá grafickou kartu také, když je položka zapnutá.",
    "After the build: the deviation report in the console.":
        "Po sestavení: zpráva o odchylkách v konzoli.",
    "What runs on the graphics card":
        "Co běží na grafické kartě",
    "**Lightmaps**: for every luxel the contribution of every light with its shadow ray, the style check, the adaptive supersampling of `-samples` (also several levels), the filter of `-filter` and adding the lights in their order.":
        "**Lightmapy**: pro každý luxel příspěvek každého světla s jeho stínovým paprskem, kontrola stylu, adaptivní supersampling z `-samples` (i několik úrovní), filtr z `-filter` a sčítání světel v jejich pořadí.",
    "**Bounce** (`-bounce`): every round, with the surface lights the radiosity creates.":
        "**Odrazy** (`-bounce`): každé kolo, se světelnými povrchy, které radiozita vytvoří.",
    "**Light grid** (the light for models and players): every grid point with all lights.":
        "**Mřížka světla** (světlo pro modely a hráče): každý bod mřížky se všemi světly.",
    "On the processor as always: loading, the light lists, vertex light, creating the bounce lights, stitching, storing and writing the .bsp - and everything the graphics card cannot do exactly (see below).":
        "Na procesoru jako vždy: načítání, seznamy světel, vrcholové světlo, vytváření světel odrazů, sešívání, ukládání a zápis .bsp - a vše, co grafická karta nedokáže spočítat přesně (viz níže).",
    "Why the result is the same":
        "Proč je výsledek stejný",
    "Graphics cards usually compute a little differently from the processor - that is why GPU lightmappers elsewhere give slightly different light. bhed_q3map2 does the **same calculations in the same order** as q3map2 on the processor:":
        "Grafické karty obvykle počítají trochu jinak než procesor - proto GPU lightmappery jinde dávají mírně odlišné světlo. bhed_q3map2 dělá **stejné výpočty ve stejném pořadí** jako q3map2 na procesoru:",
    "every operation is rounded exactly like on the processor (no fused multiply-add); divisions are rounded exactly in integer arithmetic;":
        "každá operace se zaokrouhluje přesně jako na procesoru (žádné fused multiply-add); dělení se zaokrouhluje přesně v celočíselné aritmetice;",
    "q3map2 normalizes vectors and takes square roots in double precision; the graphics card decides the same rounded result with extra-precise float arithmetic and, in the rare close cases, in double precision;":
        "q3map2 normalizuje vektory a počítá odmocniny s dvojnásobnou přesností; grafická karta rozhodne stejný zaokrouhlený výsledek pomocí zvlášť přesné aritmetiky float a ve vzácných těsných případech s dvojnásobnou přesností;",
    "the shadow rays walk q3map2's own trace tree with its tolerances, and the supersampling follows the same order of stamps as the processor;":
        "stínové paprsky procházejí vlastním trasovacím stromem q3map2 s jeho tolerancemi a supersampling se řídí stejným pořadím vzorků jako procesor;",
    "nothing is summed up in the order threads happen to finish - every luxel adds its lights in list order.":
        "nic se nesčítá v pořadí, v jakém vlákna náhodou skončí - každý luxel sčítá svá světla v pořadí seznamu.",
    "Whatever the graphics card cannot compute exactly - alpha shadows and light filters (they need the texture), a value outside the safe range, a result that is not finite - is computed for that lightmap (or grid point) on the processor. If the graphics card fails or is reset by Windows during the build, the processor computes the rest. A lightmap too big for the graphics memory is computed in parts of its light list.":
        "Co grafická karta nedokáže spočítat přesně - alfa stíny a filtry světla (potřebují texturu), hodnota mimo bezpečný rozsah, výsledek, který není konečný - se pro danou lightmapu (nebo bod mřížky) spočítá na procesoru. Selže-li grafická karta nebo ji Windows během sestavení resetují, zbytek spočítá procesor. Lightmapa příliš velká pro grafickou paměť se počítá po částech svého seznamu světel.",
    "Graphics cards without double precision (for example Intel UHD) also run the GPU bake, but there the rare close cases are only approximated: the light differs very slightly (the report shows how much). On such a computer leave {{KeGpuMenue}} off for the final build.":
        "Grafické karty bez dvojnásobné přesnosti (například Intel UHD) zapékání na GPU také provedou, ale tam se vzácné těsné případy jen přibližují: světlo se liší velmi nepatrně (zpráva ukáže o kolik). Na takovém počítači nechte {{KeGpuMenue}} pro konečné sestavení vypnuté.",
    "How much faster":
        "O kolik rychlejší",
    "Measured on a notebook with an Intel i9-14900HX (32 threads) and an NVIDIA RTX 4070 Laptop GPU, the whole `-light` step (median of three runs). The processor uses all 32 threads; the GPU bake still needs the processor for the parts listed above, so the gain is largest on big maps with many lights and bounce rounds.":
        "Měřeno na notebooku s Intel i9-14900HX (32 vláken) a NVIDIA RTX 4070 Laptop GPU, celý krok `-light` (medián tří běhů). Procesor používá všech 32 vláken; zapékání na GPU stále potřebuje procesor pro výše uvedené části, takže zisk je největší na velkých mapách s mnoha světly a koly odrazů.",
    "Switches":
        "Přepínače",
    "Processor":
        "Procesor",
    "Graphics card":
        "Grafická karta",
    "Same luxels":
        "Stejné luxely",
    "Small maps gain little: starting the graphics card costs a few tenths of a second. One case is slower on the graphics card: duel_mygeeto with `-samples 4` (101 s against 67 s) - the deep supersampling at many shadow edges branches differently for every luxel, and graphics cards are slow at that. With `-samples 2` (the Build menu) every map measured was as fast or faster.":
        "Malé mapy získají málo: spuštění grafické karty stojí několik desetin sekundy. Jeden případ je na grafické kartě pomalejší: duel_mygeeto s `-samples 4` (101 s proti 67 s) - hluboký supersampling na mnoha hranách stínů se větví pro každý luxel jinak a v tom jsou grafické karty pomalé. S `-samples 2` (nabídka Build) byla každá změřená mapa stejně rychlá nebo rychlejší.",
    "For the command line":
        "Pro příkazový řádek",
    "`-bhedgpubake` - the light on the graphics card (with `-light`).":
        "`-bhedgpubake` - světlo na grafické kartě (s `-light`).",
    "`-bhedgpubakeauto` - the faster of processor and graphics card, the graphics card only bit-identical; `-bhedgpubakezeiten <cpu ms> <gpu ms>` gives the measured times of the last runs (0 = unknown).":
        "`-bhedgpubakeauto` - rychlejší z procesoru a grafické karty, grafická karta jen bitově shodně; `-bhedgpubakezeiten <cpu ms> <gpu ms>` předá změřené časy posledních běhů (0 = neznámé).",
    "`-bhedgpubakecheck N` - every N-th lightmap is computed on the CPU too (default 64, up to one million pairs of light and luxel; `1` checks all, `0` none). The report is also written to `<map>.bhedgpu.txt`.":
        "`-bhedgpubakecheck N` - každá N-tá lightmapa se spočítá i na CPU (výchozí 64, až milion dvojic světla a luxelu; `1` kontroluje všechny, `0` žádné). Zpráva se zapíše také do `<map>.bhedgpu.txt`.",
    "`-bhedgpuadapter N` or a part of the name - which graphics card (otherwise the fastest).":
        "`-bhedgpuadapter N` nebo část názvu - která grafická karta (jinak nejrychlejší).",
    "Hardware ray tracing (DXR) was measured as a pre-test (`-bhedgpudxr` with `-bhedgpu`): it traces the shadow rays about twice as fast, but without q3map2's tolerances it misses about 0.15 % of the hits and cannot see the solid parts of the trace tree - and the shadow rays are only 7-8 % of the time of a GPU bake. It is therefore not used for baking.":
        "Hardwarový ray tracing (DXR) byl změřen jako předběžný test (`-bhedgpudxr` s `-bhedgpu`): trasuje stínové paprsky asi dvakrát rychleji, ale bez tolerancí q3map2 mine asi 0.15 % zásahů a nevidí pevné části trasovacího stromu - a stínové paprsky jsou jen 7-8 % času zapékání na GPU. Pro zapékání se proto nepoužívá.",
    "Map editor: test in game and more tools":
        "Editor map: vyzkoušení ve hře a další nástroje",
    "Start the game with the current map at the press of a button, and turn models from Blender and other games into brushes.":
        "Spusťte hru s aktuální mapou stiskem tlačítka a převeďte modely z Blenderu a jiných her na brushe.",
    "Test in game":
        "Vyzkoušet ve hře",
    "**{{KeLBuild}} > {{KeStMenue}}** lists the engine from the preferences and every game program found in the engine folder: Jedi Academy SP/MP, OpenJK, Movie Duels and, for Jedi Outcast maps, Jedi Outcast SP/MP and OpenJO. A click starts the chosen program with the current map (`+devmap`). The choice is remembered.":
        "**{{KeLBuild}} > {{KeStMenue}}** vypíše engine z předvoleb a každý program hry nalezený ve složce enginu: Jedi Academy SP/MP, OpenJK, Movie Duels a pro mapy Jedi Outcast Jedi Outcast SP/MP a OpenJO. Kliknutí spustí zvolený program s aktuální mapou (`+devmap`). Volba se pamatuje.",
    "**Which .bsp:** the quick bake of the current map if it is up to date (**{{KeBackenMenue}}**), otherwise a .bsp that is newer than the saved, unchanged map, otherwise the program builds first with the first **[Test]** entry of the Build menu and starts the game when the build is done.":
        "**Který .bsp:** rychlé zapečení aktuální mapy, je-li aktuální (**{{KeBackenMenue}}**), jinak .bsp novější než uložená nezměněná mapa, jinak program nejdřív sestaví mapu první položkou **[Test]** nabídky Build a spustí hru, až je sestavení hotové.",
    "**Where it goes:** the .bsp is packed as `zzz_jkstudio_test.pk3` into the folder `spieltest` next to the program settings - never into the game folder. The game learns about the folder from a start parameter: `fs_cdpath` (all engines of this family; your game settings stay as they are) or, if the game already has a map with the same name, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), because that folder takes precedence. Which parameter a program knows is read from its .exe.":
        "**Kam jde:** .bsp se zabalí jako `zzz_jkstudio_test.pk3` do složky `spieltest` vedle nastavení programu - nikdy do složky hry. Hra se o složce dozví ze spouštěcího parametru: `fs_cdpath` (všechny enginy této rodiny; vaše nastavení hry zůstanou, jak jsou), nebo, má-li hra už mapu se stejným názvem, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), protože tato složka má přednost. Který parametr program zná, se čte z jeho .exe.",
    "**Only if neither works** (for example Jedi Academy SP with a map name the game already has), the program asks before it writes the archive into the game: into a mod folder `jkstudio_test` (started with `+set fs_game jkstudio_test`), or into the folder of the active mod. You can delete the file later.":
        "**Jen když nefunguje ani jedno** (například Jedi Academy SP s názvem mapy, který hra už má), program se zeptá, než zapíše archiv do hry: do složky modu `jkstudio_test` (spuštěné s `+set fs_game jkstudio_test`), nebo do složky aktivního modu. Soubor můžete později smazat.",
    "The console shows the way that was chosen and the whole command line. Map names with spaces cannot be loaded with `devmap`; save the map under a name without spaces.":
        "Konzole ukáže zvolený způsob a celý příkazový řádek. Názvy map s mezerami nelze načíst pomocí `devmap`; uložte mapu pod názvem bez mezer.",
    "Not in NetRadiant-Custom. NetRadiant-Custom's own **Run Engine After Compile** (preferences, page Build) stays as it is: it starts the engine from the game folder after every build.":
        "Není v NetRadiant-Custom. Vlastní **Po kompilaci spustit engine** NetRadiant-Custom (předvolby, stránka Sestavení) zůstává, jak je: po každém sestavení spustí engine ze složky hry.",
    "Geometry to brushes":
        "Geometrie na brushe",
    "**{{KeLFile}} > {{KeGiMenue}}** reads a model (.obj, .fbx, .glb or .gltf, for example from Blender) and turns it into brushes. A window lists the parts of the file; for each part choose **{{KeGiAuto}}**, **{{KeGiPinsel}}** or **{{KeGiModellClip}}** - the column **{{KeGiErgebnis}}** shows at once how many brushes it becomes and whether they are detail. **{{KeGiEinfuegen}}** inserts everything as one undo step and selects it; then the optimizer opens.":
        "**{{KeLFile}} > {{KeGiMenue}}** přečte model (.obj, .fbx, .glb nebo .gltf, například z Blenderu) a převede ho na brushe. Okno vypíše části souboru; pro každou část zvolte **{{KeGiAuto}}**, **{{KeGiPinsel}}** nebo **{{KeGiModellClip}}** - sloupec **{{KeGiErgebnis}}** hned ukáže, kolik brushů z ní bude a zda jsou detail. **{{KeGiEinfuegen}}** vloží vše jako jeden krok zpět a vybere to; pak se otevře optimalizátor.",
    "**Closed parts** are split into convex brushes (a BSP over the faces of the model, like q3map2 builds one). Faces that lie in one plane become one brush side. Sides on the surface of the model get its material as shader (**{{KeGiVorsatz}}** + material name) and its UV as texture alignment; sides inside the model, which nobody sees, get caulk.":
        "**Uzavřené části** se rozdělí na konvexní brushe (BSP přes stěny modelu, jak ho staví q3map2). Stěny ležící v jedné rovině se stanou jednou stranou brushe. Strany na povrchu modelu dostanou jeho materiál jako shader (**{{KeGiVorsatz}}** + název materiálu) a jeho UV jako zarovnání textury; strany uvnitř modelu, které nikdo nevidí, dostanou caulk.",
    "**Open parts** (single surfaces, decoration) become thin brushes behind each flat face; small and open parts become detail, so they do not cut the map into BSP leaves.":
        "**Otevřené části** (jednotlivé povrchy, dekorace) se stanou tenkými brushi za každou rovnou stěnou; malé a otevřené části se stanou detailem, aby nerozřezaly mapu na listy BSP.",
    "**{{KeGiModellClip}}** keeps the part as a model: it is written as .obj into the program's import folder (next to the settings, never into the game), a misc_model points to it, and clip brushes from the same decomposition make it solid. The editor and q3map2 (`-fs_pakpath`) search this folder. **{{KeGiAuto}}** takes brushes for closed parts with at most 256 brushes, otherwise model + clip.":
        "**{{KeGiModellClip}}** ponechá část jako model: zapíše se jako .obj do složky importu programu (vedle nastavení, nikdy do hry), ukazuje na něj misc_model a clip brushe ze stejného rozkladu ho dělají pevným. Editor i q3map2 (`-fs_pakpath`) tuto složku prohledávají. **{{KeGiAuto}}** bere brushe pro uzavřené části s nejvýš 256 brushi, jinak model + clip.",
    "**{{KeGiSkala}}** and **{{KeGiYOben}}**: Blender and glTF use Y as up, the game Z; one Blender metre is one unit unless you scale it. A **.bsp** of another id Tech 3 game (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) opens like **{{KeLOpenBsp}}**, with the reader of that game.":
        "**{{KeGiSkala}}** a **{{KeGiYOben}}**: Blender a glTF používají Y jako nahoru, hra Z; jeden metr v Blenderu je jedna jednotka, pokud ho neškálujete. **.bsp** jiné hry na id Tech 3 (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) se otevře jako **{{KeLOpenBsp}}**, čtečkou dané hry.",
    "Drag a model into the map":
        "Přetáhnout model do mapy",
    "Drag an .obj, .fbx, .glb or .gltf file from the Explorer onto the map editor. The program converts it to an MD3 with its own shader and copies the textures (embedded in the file or next to it); all of it goes into the import folder next to the settings, never into the game. A misc_model appears under the mouse pointer in the 2D view (or in front of the camera) and is selected; one undo removes it. Models larger than 511 units are scaled down in the MD3 and get `modelscale`.":
        "Přetáhněte soubor .obj, .fbx, .glb nebo .gltf z Průzkumníka na editor map. Program ho převede na MD3 s vlastním shaderem a zkopíruje textury (vložené v souboru nebo vedle něj); vše jde do složky importu vedle nastavení, nikdy do hry. Pod ukazatelem myši ve 2D pohledu (nebo před kamerou) se objeví vybraný misc_model; jedno Zpět ho odebere. Modely větší než 511 jednotek se v MD3 zmenší a dostanou `modelscale`.",
    "**{{KeStMenue}}** and **{{KePkMenue}}** pack the import folder into their archive, so the game finds the models, shaders and textures.":
        "**{{KeStMenue}}** a **{{KePkMenue}}** zabalí složku importu do svého archivu, aby hra našla modely, shadery a textury.",
    "Check jumps and space":
        "Kontrola skoků a prostoru",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeSjMenue}} > {{KeSjPruefen}}** checks the selection (or 512 units around the camera) with the movement of the game: where the player can stand (green crosses) or fits only crouched (yellow), and from which edges a running jump lands on another floor - blue for a normal jump, purple where only the force jump of the chosen level gets there. It runs in the background; **{{KeSjLeeren}}** removes the display.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeSjMenue}} > {{KeSjPruefen}}** zkontroluje výběr (nebo 512 jednotek kolem kamery) s pohybem hry: kde hráč může stát (zelené křížky) nebo se vejde jen v dřepu (žluté) a ze kterých hran skok z rozběhu dopadne na jinou podlahu - modře pro normální skok, fialově tam, kam se dostane jen skok Silou zvolené úrovně. Běží na pozadí; **{{KeSjLeeren}}** zobrazení odebere.",
    "The movement is the port of the game code: running speed 250, gravity 800, jump 225, the player box 32 x 32 x 64 (crouched 40), steps up to 18 units, floors up to 45 degrees. The force jump follows the game: holding the jump key it rises up to 96, 192 or 384 units (levels 1 to 3); letting go earlier jumps lower. Jedi Outcast and Movie Duels compute it the same way.":
        "Pohyb je port kódu hry: rychlost běhu 250, gravitace 800, skok 225, krabice hráče 32 x 32 x 64 (v dřepu 40), schody do 18 jednotek, podlahy do 45 stupňů. Skok Silou se řídí hrou: při držení klávesy skoku stoupá až do 96, 192 nebo 384 jednotek (úrovně 1 až 3); dřívější puštění skočí níž. Jedi Outcast a Movie Duels to počítají stejně.",
    "The chosen force jump level also applies to **{{KeSpMenue}}** (F6): hold the space bar to jump higher.":
        "Zvolená úroveň skoku Silou platí i pro **{{KeSpMenue}}** (F6): podržením mezerníku skočíte výš.",
    "NPC paths":
        "Cesty NPC",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeNwMenue}} > {{KeNwNetz}}** shows the network the game builds from the waypoints when it loads the map - connections turquoise, jump connections orange. The game builds a network only from more than three waypoints; the console says so. **{{KeNwLauf}}**: select two entities (for example two waypoints, or an info_player_start and a waypoint) - an NPC walks from the first to the last one with the game's path finding and movement; the path is yellow and a box walks along it in real time. The console says after how many seconds it arrives, or where it gets stuck. **{{KeNwRennen}}** switches between running and walking.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeNwMenue}} > {{KeNwNetz}}** ukáže síť, kterou hra sestaví z navigačních bodů při načtení mapy - spojení tyrkysově, spojení skokem oranžově. Hra sestaví síť jen z více než tří navigačních bodů; konzole to oznámí. **{{KeNwLauf}}**: vyberte dvě entity (například dva navigační body, nebo info_player_start a navigační bod) - NPC jde od první k poslední s hledáním cesty a pohybem hry; cesta je žlutá a krabice po ní jde v reálném čase. Konzole řekne, po kolika sekundách dorazí, nebo kde uvízne. **{{KeNwRennen}}** přepíná mezi během a chůzí.",
    "Linked copies":
        "Propojené kopie",
    "Select brushes and patches (for example a pillar or a lamp) and choose **{{KeLView}} > {{KeWzMenue}} > {{KeVbMenue}} > {{KeVbKopie}}**: they become a linked group, and a copy appears next to it. Select the copy and repeat for more copies. From now on, whatever you change in one copy - add, delete, resize or retexture brushes - happens in all of them in the same step; one undo takes it back everywhere. Moving a whole copy moves only that copy. **{{KeVbLoesen}}** makes a copy independent again.":
        "Vyberte brushe a patche (například sloup nebo lampu) a zvolte **{{KeLView}} > {{KeWzMenue}} > {{KeVbMenue}} > {{KeVbKopie}}**: stanou se propojenou skupinou a vedle se objeví kopie. Vyberte kopii a pro další kopie to zopakujte. Od teď cokoli změníte v jedné kopii - přidání, smazání, změna velikosti nebo textury brushů - se stane ve všech v tomtéž kroku; jedno Zpět to vrátí všude. Posunutí celé kopie posune jen ji. **{{KeVbLoesen}}** udělá kopii zase samostatnou.",
    "The copies are func_groups with the keys `_bhed_link` and `_bhed_link_origin`; the game and q3map2 treat them like any func_group. Copies are moved, not rotated.":
        "Kopie jsou func_group s klíči `_bhed_link` a `_bhed_link_origin`; hra i q3map2 s nimi zacházejí jako s každou func_group. Kopie se posouvají, neotáčejí.",
    "Curve tool":
        "Nástroj křivek",
    "Place a few point entities (for example info_null) along the course you want and select them in that order. **{{KeLView}} > {{KeWzMenue}} > {{KeKuMenue}}** lays a smooth curve through them and builds, as one undo step: **{{KeKuWeg}}** (a flat strip, 128 wide), **{{KeKuRohr}}**, **{{KeKuGelaender}}** (a rail on each side at hand height), **{{KeKuBruecke}}** (deck with sides and underside), **{{KeKuPfad}}** (a chain of path_corner linked by targetname/target - for func_train or a camera track) or **{{KeKuWegpunkte}}** (a linked chain of waypoints). Patches get the texture selected in the texture browser; the spacing follows the grid.":
        "Umístěte podél požadovaného průběhu několik bodových entit (například info_null) a vyberte je v tomto pořadí. **{{KeLView}} > {{KeWzMenue}} > {{KeKuMenue}}** jimi proloží hladkou křivku a jako jeden krok zpět postaví: **{{KeKuWeg}}** (plochý pruh, 128 široký), **{{KeKuRohr}}**, **{{KeKuGelaender}}** (zábradlí na každé straně ve výšce ruky), **{{KeKuBruecke}}** (mostovka s boky a spodkem), **{{KeKuPfad}}** (řetěz path_corner propojený přes targetname/target - pro func_train nebo dráhu kamery) nebo **{{KeKuWegpunkte}}** (propojený řetěz navigačních bodů). Patche dostanou texturu vybranou v prohlížeči textur; rozestup se řídí mřížkou.",
    "Scatter models":
        "Rozmístění modelů",
    "Place one misc_model_static (or misc_model) as the template - for example a rock or a crate - and select it together with the brushes of the area (for example the floor). **{{KeLView}} > {{KeWzMenue}} > {{KeVtMenue}}** puts 10, 25 or 50 copies onto the walkable surfaces in that area, with random angle and scale, at least 48 units apart, as one undo step. Without brushes it uses 256 units around the template.":
        "Umístěte jeden misc_model_static (nebo misc_model) jako šablonu - například kámen nebo bednu - a vyberte ho spolu s brushi oblasti (například podlahou). **{{KeLView}} > {{KeWzMenue}} > {{KeVtMenue}}** položí 10, 25 nebo 50 kopií na pochozí povrchy v této oblasti, s náhodným úhlem a měřítkem, nejméně 48 jednotek od sebe, jako jeden krok zpět. Bez brushů použije 256 jednotek kolem šablony.",
    "Every misc_model_static becomes a static object of the game, and Jedi Academy and Movie Duels stop with an error after 2000 (MAX_MISC_ENTS). Scattering stops at that limit, and the optimizer counts misc_model_static against it. Jedi Outcast has no misc_model_static; there the copies are misc_model.":
        "Každý misc_model_static se stane statickým objektem hry a Jedi Academy a Movie Duels se po 2000 (MAX_MISC_ENTS) zastaví s chybou. Rozmisťování se na tomto limitu zastaví a optimalizátor proti němu misc_model_static počítá. Jedi Outcast misc_model_static nemá; tam jsou kopie misc_model.",
    "Terrain sculpt":
        "Modelování terénu",
    "Make a terrain of patches (for example a big patch with many rows, or the patches of the terrain generator). **{{KeLView}} > {{KeWzMenue}} > {{KeFoMenue}} > {{KeFoAn}}**: now the left mouse button in the camera shapes the patches under the pointer instead of selecting - hold it and move. Choose **{{KeFoHeben}}**, **{{KeFoSenken}}**, **{{KeFoGlaetten}}** or **{{KeFoEbnen}}** (to the height where you pressed) and a radius of 64, 128 or 256; the effect fades towards the edge of the circle. One stroke is one undo step.":
        "Vytvořte terén z patchů (například velký patch s mnoha řádky, nebo patche generátoru terénu). **{{KeLView}} > {{KeWzMenue}} > {{KeFoMenue}} > {{KeFoAn}}**: teď levé tlačítko myši v kameře místo výběru tvaruje patche pod ukazatelem - podržte ho a pohybujte. Zvolte **{{KeFoHeben}}**, **{{KeFoSenken}}**, **{{KeFoGlaetten}}** nebo **{{KeFoEbnen}}** (na výšku, kde jste stiskli) a poloměr 64, 128 nebo 256; účinek k okraji kruhu slábne. Jeden tah je jeden krok zpět.",
    "Only patches are shaped; terrain brushes keep their shape (shape them with the vertex tool or Terrain from the plugins). How textures blend on the terrain is described below.":
        "Tvarují se jen patche; brushe terénu si ponechají svůj tvar (tvarujte je nástrojem vrcholů nebo terénem z pluginů). Jak se textury na terénu prolínají, je popsáno níže.",
    "Texture blending":
        "Prolínání textur",
    "Make the terrain a func_group (brushes or patches), select it and choose **{{KeLView}} > {{KeWzMenue}} > {{KeUbMenue}}**. Choose 2 to 4 **{{KeUbSchichten}}** and a texture for each (**{{KeUbAktuell}}** takes the one selected in the texture browser), then **{{KeUbEinrichten}}**: the group gets the q3map2 keys `_indexmap`, `_layers` and `_shader`, its faces the blend shader, and the import folder an index map and the shaders for every layer and every pair of layers - one undo step. Now paint with the mouse in the top view of the window: choose the layer and the **{{KeUbRadius}}**; top is north (largest Y), as in the 2D view. Building the map blends the textures where the layers meet.":
        "Udělejte z terénu func_group (brushe nebo patche), vyberte ji a zvolte **{{KeLView}} > {{KeWzMenue}} > {{KeUbMenue}}**. Zvolte 2 až 4 **{{KeUbSchichten}}** a pro každou texturu (**{{KeUbAktuell}}** vezme tu vybranou v prohlížeči textur), pak **{{KeUbEinrichten}}**: skupina dostane klíče q3map2 `_indexmap`, `_layers` a `_shader`, její stěny shader prolínání a složka importu mapu indexů a shadery pro každou vrstvu a každou dvojici vrstev - jeden krok zpět. Teď malujte myší v pohledu shora v okně: zvolte vrstvu a **{{KeUbRadius}}**; nahoře je sever (největší Y), jako ve 2D pohledu. Sestavení mapy prolne textury tam, kde se vrstvy setkávají.",
    "The blend shaders use `q3map_terrain` and per-vertex alpha: the finer the terrain (more vertices), the finer the transitions. Texture blending is not in NetRadiant-Custom; the keys and the index map are those of q3map2.":
        "Shadery prolínání používají `q3map_terrain` a alfu na vrchol: čím jemnější terén (víc vrcholů), tím jemnější přechody. Prolínání textur není v NetRadiant-Custom; klíče a mapa indexů jsou ty z q3map2.",
    "Asset browser":
        "Prohlížeč prostředků",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeAbMenue}}** opens a window with the models (.md3, .glm, .ase, .obj), the prefabs and the sounds of the game, each with a preview: the texture of a model, the waveform of a sound, the outline of a prefab from above. **{{KeAbSuche}}** narrows the list - every word must appear in the path. Drag an entry onto a 2D view or the camera: a model becomes a misc_model, a sound a target_speaker, a prefab is inserted - at the place under the pointer. A double-click puts it in front of the camera. Everything is one undo step.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeAbMenue}}** otevře okno s modely (.md3, .glm, .ase, .obj), prefaby a zvuky hry, každý s náhledem: textura modelu, průběh vlny zvuku, obrys prefabu shora. **{{KeAbSuche}}** zúží seznam - každé slovo se musí objevit v cestě. Přetáhněte položku na 2D pohled nebo do kamery: model se stane misc_model, zvuk target_speaker, prefab se vloží - na místo pod ukazatelem. Dvojklik ho umístí před kameru. Vše je jeden krok zpět.",
    "Prefabs are .map files in the import folder (`import/prefabs` in the settings folder). Select brushes and entities, enter a name under {{KeAbPrefabs}} and click **{{KeAbPrefabSpeichern}}**. **{{KeLBuild}} > {{KePkMenue}}** takes the import folder along. The asset browser is not in NetRadiant-Custom.":
        "Prefaby jsou soubory .map ve složce importu (`import/prefabs` ve složce nastavení). Vyberte brushe a entity, zadejte název pod {{KeAbPrefabs}} a klikněte na **{{KeAbPrefabSpeichern}}**. **{{KeLBuild}} > {{KePkMenue}}** vezme složku importu s sebou. Prohlížeč prostředků není v NetRadiant-Custom.",
    "To Blender and back":
        "Do Blenderu a zpět",
    "Select brushes and choose **{{KeLFile}} > {{KeBlMenue}}**: they are written as .glb (or .obj) with Y up, the way Blender expects, one object per brush, the shader names as materials. Edit the part in Blender and export it again as .glb or .obj with the default settings. **{{KeLFile}} > {{KeGiMenue}}** with **{{KeGiYOben}}** switched on (the default) brings it back to the same place in the map.":
        "Vyberte brushe a zvolte **{{KeLFile}} > {{KeBlMenue}}**: zapíší se jako .glb (nebo .obj) s Y nahoru, jak to Blender očekává, jeden objekt na brush, názvy shaderů jako materiály. Upravte část v Blenderu a exportujte ji znovu jako .glb nebo .obj s výchozím nastavením. **{{KeLFile}} > {{KeGiMenue}}** se zapnutým **{{KeGiYOben}}** (výchozí) ji vrátí na stejné místo v mapě.",
    "Restore after a crash":
        "Obnovení po pádu",
    "The map editor saves automatically like NetRadiant-Custom (preferences, page Autosave: `<map>.autosave.map` next to the map). If the program did not close properly - a crash, a power cut - the editor notices it at the next start and asks whether to load the autosave, when it is newer than the map. The restored map keeps its name and counts as changed: save it to keep it. Answer no, and the last map opens as usual.":
        "Editor map ukládá automaticky jako NetRadiant-Custom (předvolby, stránka Autoukládání: `<map>.autosave.map` vedle mapy). Pokud se program neukončil správně - pád, výpadek proudu - editor si toho při dalším spuštění všimne a zeptá se, zda načíst automatické uložení, je-li novější než mapa. Obnovená mapa si ponechá svůj název a počítá se jako změněná: pro zachování ji uložte. Odpovíte-li ne, otevře se poslední mapa jako obvykle.",
    "Menu editor":
        "Editor menu",
    "Edit the game's menus (.menu) as text and on a 640x480 canvas at the same time - read, drawn and operated like in the game.":
        "Upravujte menu hry (.menu) jako text a zároveň na plátně 640x480 - čtená, kreslená a ovládaná jako ve hře.",
    "Open a menu":
        "Otevření menu",
    "Click **{{ViewMenuEditor}}** in the mode bar. **{{MeAusSpiel}}** lists every `ui/` menu of the game set up in the preferences (read from the .pk3, never changed); **{{MeDateiOeffnen}}** or dropping a file opens one from disk. The path decides how the file is read, exactly like the game: `ui/jamp/`, `ui/jk2mp/` and `ui/MD_MP/` with the multiplayer precompiler (#include, #define, macros, menudef.h), everything else like single player.":
        "Klikněte na **{{ViewMenuEditor}}** v liště režimů. **{{MeAusSpiel}}** vypíše každé menu `ui/` hry nastavené v předvolbách (čtené z .pk3, nikdy neměněné); **{{MeDateiOeffnen}}** nebo puštění souboru otevře menu z disku. Cesta rozhoduje, jak se soubor čte, přesně jako ve hře: `ui/jamp/`, `ui/jk2mp/` a `ui/MD_MP/` s preprocesorem pro více hráčů (#include, #define, makra, menudef.h), vše ostatní jako pro jednoho hráče.",
    "Canvas and pages":
        "Plátno a stránky",
    "The multiplayer main menu of Jedi Academy, read from the game files and drawn like the game.":
        "Hlavní menu Jedi Academy pro více hráčů, načtené ze souborů hry a kreslené jako ve hře.",
    "Every `menuDef` of the file is a tab above the canvas; items that are shown by a cvar with several values (`cvarTest` + `showCvar`, like `ui_tab` pages) get a tab per value. **+** adds a new `menuDef`. The canvas is the game's virtual 640x480 screen: **{{MeStrecken}}** like the original game, **{{MeBalken}}**, and preset resolutions to check the aspect ratio.":
        "Každý `menuDef` souboru je karta nad plátnem; prvky, které se ukazují podle cvaru s několika hodnotami (`cvarTest` + `showCvar`, jako stránky `ui_tab`), dostanou kartu na každou hodnotu. **+** přidá nový `menuDef`. Plátno je virtuální obrazovka hry 640x480: **{{MeStrecken}}** jako původní hra, **{{MeBalken}}** a přednastavená rozlišení pro kontrolu poměru stran.",
    "Click selects (Shift/Ctrl adds), dragging moves, the white handles resize. Only the numbers of `rect` change in the text - comments and indentation stay.":
        "Kliknutí vybírá (Shift/Ctrl přidává), tažení posouvá, bílé úchyty mění velikost. V textu se mění jen čísla `rect` - komentáře a odsazení zůstanou.",
    "**{{MeAnKanten}}** snaps to edges and centres of the other items, the screen and your guides (drag them out of the rulers); **{{MeAnRaster}}** to the grid. Alt switches snapping off while dragging, Shift keeps the direction. Arrow keys move by 1, with Shift by 10.":
        "**{{MeAnKanten}}** přichytává k okrajům a středům ostatních prvků, obrazovky a vašich vodítek (vytáhněte je z pravítek); **{{MeAnRaster}}** k mřížce. Alt během tažení přichytávání vypne, Shift drží směr. Šipky posouvají o 1, s Shift o 10.",
    "**{{MePalette}}** inserts new items (text, button, image, yes/no, multi choice, slider, edit field, list box, model, key bind) at the end of the page's `menuDef`. **{{MeBildDazu}}** takes a PNG, JPG or TGA as `gfx/menus/<name>`; it is packed into the .pk3 on export.":
        "**{{MePalette}}** vkládá nové prvky (text, tlačítko, obrázek, ano/ne, výběr z více, posuvník, textové pole, seznam, model, přiřazení klávesy) na konec `menuDef` stránky. **{{MeBildDazu}}** vezme PNG, JPG nebo TGA jako `gfx/menus/<name>`; při exportu se zabalí do .pk3.",
    "**{{MeEigenschaften}}** edits the selected item; with several items it aligns, distributes and groups them. **{{MeNachVorn}}**/**{{MeNachHinten}}** change the drawing order (the game draws in file order). **{{MeSperren}}** keeps an item from moving.":
        "**{{MeEigenschaften}}** upravuje vybraný prvek; s několika prvky je zarovnává, rozmisťuje a seskupuje. **{{MeNachVorn}}**/**{{MeNachHinten}}** mění pořadí kreslení (hra kreslí v pořadí souboru). **{{MeSperren}}** zabrání posunutí prvku.",
    "Interact and cvars":
        "Ovládání a cvary",
    "**{{MeBedienen}}** runs the menu like the game: hovering gives focus (`mouseEnter`, `onFocus`), a click runs the item's `action`, yes/no, multi choice and sliders change their cvar, and `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... work like in ui_shared. What needs the game (sounds, `exec`, `uiScript`) appears in **{{MeProtokoll}}**.":
        "**{{MeBedienen}}** spustí menu jako hra: najetí myší dává fokus (`mouseEnter`, `onFocus`), kliknutí spustí `action` prvku, ano/ne, výběr z více a posuvníky mění svůj cvar a `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... fungují jako v ui_shared. Co potřebuje hru (zvuky, `exec`, `uiScript`), se objeví v **{{MeProtokoll}}**.",
    "**{{MeCvars}}** lists every cvar the menu reads or sets, the ones that control visibility first, with the values that occur. Change them to see other states. They exist only in the preview - your game configuration is never touched.":
        "**{{MeCvars}}** vypíše každý cvar, který menu čte nebo nastavuje, ty řídící viditelnost první, s hodnotami, které se vyskytují. Změňte je a uvidíte jiné stavy. Existují jen v náhledu - konfigurace vaší hry se nikdy nezmění.",
    "Code editor":
        "Editor kódu",
    "Interact: the page cvar `ui_tab` shows the red box, a click on Yes/No changes `ui_probe`.":
        "Ovládání: cvar stránky `ui_tab` ukazuje červený rámeček, kliknutí na Ano/Ne mění `ui_probe`.",
    "find / replace (regex, case, whole word, all open menus); matches are marked in the minimap":
        "najít / nahradit (regex, velikost písmen, celé slovo, všechna otevřená menu); shody jsou označené v minimapě",
    "next occurrence / all occurrences / extra cursor (multi-cursor)":
        "další výskyt / všechny výskyty / další kurzor (více kurzorů)",
    "completion: keywords, cvars, item names, shader paths, @MENUS strings":
        "doplňování: klíčová slova, cvary, názvy prvků, cesty shaderů, řetězce @MENUS",
    "go to the menuDef or itemDef of the name under the cursor":
        "přejít na menuDef nebo itemDef názvu pod kurzorem",
    "bookmark like in the script editor (also a click in the margin)":
        "záložka jako v editoru skriptů (i kliknutím do okraje)",
    "comment lines, indent / outdent":
        "zakomentovat řádky, odsadit / zrušit odsazení",
    "The margin works like in the script editor: orange = changed since saving, green = changed and saved, blue = back to the state when opened. `menuDef`, `itemDef` and script blocks fold with the +/- next to the line numbers. Clicking an item on the canvas jumps to its `itemDef`, and the cursor in an `itemDef` selects the item.":
        "Okraj funguje jako v editoru skriptů: oranžová = změněno od uložení, zelená = změněno a uloženo, modrá = zpět ve stavu při otevření. `menuDef`, `itemDef` a bloky skriptů se sbalují pomocí +/- vedle čísel řádků. Kliknutí na prvek na plátně skočí na jeho `itemDef` a kurzor v `itemDef` prvek vybere.",
    "Find with a regular expression: matches in the text and in the minimap, an engine message marked red in line 49.":
        "Hledání regulárním výrazem: shody v textu a v minimapě, zpráva enginu označená červeně na řádku 49.",
    "Save and export":
        "Uložení a export",
    "**{{MeSpeichern}}** writes the text exactly as it is in the editor; a file opened from the game is saved elsewhere. **{{MeExportPk3}}** packs the menu under its game path plus your images into a .pk3. Neither ever writes into the game folders.":
        "**{{MeSpeichern}}** zapíše text přesně tak, jak je v editoru; soubor otevřený ze hry se uloží jinam. **{{MeExportPk3}}** zabalí menu pod jeho cestou ve hře plus vaše obrázky do .pk3. Ani jedno nikdy nezapisuje do složek hry.",
    "Drawn like the game: fonts from the .fontdat files, text positions as in RE_Font_DrawString, item types as in ui_shared. Placeholders show what needs the running game: ownerdraw items (with their number), list feeders, 3D models and cinematics.":
        "Kresleno jako ve hře: písma ze souborů .fontdat, polohy textu jako v RE_Font_DrawString, typy prvků jako v ui_shared. Zástupné symboly ukazují, co potřebuje běžící hru: prvky ownerdraw (s jejich číslem), feedery seznamů, 3D modely a filmy.",
    "Missions":
        "Mise",
    "A mission is a map with its entities and all its scripts. Open one whole, build your own, and pack it into a .pk3 that the game loads.":
        "Mise je mapa s jejími entitami a všemi skripty. Otevřete ji celou, postavte vlastní a zabalte ji do .pk3, který hra načte.",
    "Opening a mission":
        "Otevření mise",
    "**{{LoadMission}}** (menu {{MenuFile}} or the map toolbar) picks a `.pk3` and lists **all missions of that game** - the maps that have scripts, from every archive of the game folders, exactly as the game finds them (for a name, the archive that wins counts). It does not matter which archive of the game you pick: `assets1.pk3` of plain Jedi Academy lists `t1_sour`, `kor1`, `yavin1` … just like `assets0.pk3`. The missions of the chosen archive stand at the top; the others follow once they have been read (the maps are read in the background - the line above the list shows how far). The filter field narrows the list by name. Pick one and everything is set up: the map, its `.ent` with the mission's figures, and **every script of the mission in its own tab**. All tabs share the map and the camera.":
        "**{{LoadMission}}** (nabídka {{MenuFile}} nebo lišta mapy) vybere `.pk3` a vypíše **všechny mise této hry** - mapy, které mají skripty, ze všech archivů složek hry, přesně tak, jak je hra najde (pro daný název platí archiv, který vyhrává). Nezáleží na tom, který archiv hry vyberete: `assets1.pk3` čistého Jedi Academy vypíše `t1_sour`, `kor1`, `yavin1` … stejně jako `assets0.pk3`. Mise zvoleného archivu stojí nahoře; ostatní následují, jakmile se přečtou (mapy se čtou na pozadí - řádek nad seznamem ukazuje, jak daleko). Pole filtru zúží seznam podle názvu. Vyberte jednu a vše se nastaví: mapa, její `.ent` s postavami mise a **každý skript mise na vlastní kartě**. Všechny karty sdílejí mapu a kameru.",
    "The missions of the game, here narrowed with the filter field - those of the chosen archive first, the other archives of the game below.":
        "Mise hry, zde zúžené polem filtru - nejdřív ty ze zvoleného archivu, pod nimi ostatní archivy hry.",
    "The tabs open **in the order in which the game starts the scripts**. For this, JK Studio plays the map from the moment it loads, like the engine: the spawn scripts of the entities, the spawn script of `worldspawn`, `trigger_always`, NPC spawners without a `targetname`, the targets of the player start when the player enters the game - and everything that follows from them through `run`, `use`, relays, counters, spawners and `kill` with a death script. The tab of the first script is in front. Scripts that only the player sets off (a trigger he walks into, a fight) come last, in the order of the entities.":
        "Karty se otevřou **v pořadí, ve kterém hra skripty spouští**. K tomu JK Studio přehraje mapu od chvíle, kdy se načte, jako engine: spawn skripty entit, spawn skript `worldspawn`, `trigger_always`, spawnery NPC bez `targetname`, cíle startu hráče, když hráč vstoupí do hry - a vše, co z nich následuje přes `run`, `use`, relaye, počítadla, spawnery a `kill` s death skriptem. Karta prvního skriptu je vpředu. Skripty, které spouští jen hráč (trigger, do kterého vejde, boj), přijdou poslední, v pořadí entit.",
    "Which game folders apply is decided by the archive: if its folder is one of your game folders ({{MenuEdit}} → {{PathsTitle}}), those are used. If it belongs to another installation - its `base`, or a mod folder next to a `base` - JK Studio uses that installation's folders **for this session only**, in the engine's order (`base`, then the mod), and does not mix in your configured folders. A `.pk3` in any other folder is added behind your game folders, also only for this session. Nothing is saved: the game folders window shows the folders of the session with a button back to your own ({{PathsSessionBack}}). Each mission tab keeps the folders of its game: with a Jedi Outcast mission in one tab and a Jedi Academy mission in another, switching tabs switches the game folders and the rule set with them.":
        "Které složky hry platí, rozhoduje archiv: je-li jeho složka jednou z vašich složek hry ({{MenuEdit}} → {{PathsTitle}}), použijí se ty. Patří-li k jiné instalaci - její `base`, nebo složka modu vedle `base` - použije JK Studio složky této instalace **jen pro tuto relaci**, v pořadí enginu (`base`, pak mod), a nemíchá do nich vaše nastavené složky. `.pk3` v jakékoli jiné složce se přidá za vaše složky hry, také jen pro tuto relaci. Nic se neukládá: okno složek hry ukazuje složky relace s tlačítkem zpět k vašim vlastním ({{PathsSessionBack}}). Každá karta mise si pamatuje složky své hry: s misí Jedi Outcast na jedné kartě a misí Jedi Academy na druhé přepnutí karet přepne i složky hry a sadu pravidel.",
    "Recent missions":
        "Poslední mise",
    "**{{MenuFile}} → {{RecentMissions}}** and the start page list the missions you opened last - from an archive or from a mission folder. One click opens the whole project again: map, entities and all scripts. An entry whose file is gone is removed from the list.":
        "**{{MenuFile}} → {{RecentMissions}}** a úvodní stránka vypisují mise, které jste naposledy otevřeli - z archivu nebo ze složky mise. Jedno kliknutí znovu otevře celý projekt: mapu, entity a všechny skripty. Položka, jejíž soubor zmizel, se ze seznamu odebere.",
    "A new mission":
        "Nová mise",
    "**{{MissionNew}}** starts a mission on any map. Choose the map, give the mission a name (letters, digits and `_`; the game loads it as `maps/<name>.bsp`, so `devmap <name>`), and a script folder. JK Studio creates a player start, a script runner, the intro script and a mission folder under `Documents\\behaved\\missions`. The intro runs when the player spawns.":
        "**{{MissionNew}}** začne misi na libovolné mapě. Zvolte mapu, dejte misi název (písmena, číslice a `_`; hra ji načte jako `maps/<name>.bsp`, tedy `devmap <name>`) a složku skriptů. JK Studio vytvoří start hráče, spouštěč skriptu, úvodní skript a složku mise pod `Documents\\behaved\\missions`. Úvod se spustí, když se hráč objeví.",
    "The {{MissionNewTitle}} dialog.":
        "Dialog {{MissionNewTitle}}.",
    "A mission that was never packed lives in its folder; **{{MissionOpenFolder}}** continues it after a restart.":
        "Mise, která nikdy nebyla zabalena, žije ve své složce; **{{MissionOpenFolder}}** v ní po restartu pokračuje.",
    "Saving a mission":
        "Uložení mise",
    "**{{MissionSave}}** shows what will go into the archive - every file with its size - and checks the mission first: a script that an entity uses but that does not exist, a script that does not compile, no player start, a target that points nowhere, a `.str` without `ENDMARKER`. Errors (red) must be fixed, warnings (orange) may stay. **{{MissionSaveNow}}** saves all scripts, writes the `.ent`, compiles the scripts, adds your own ROFF files and packs the `.pk3` into the game folder.":
        "**{{MissionSave}}** ukáže, co půjde do archivu - každý soubor s velikostí - a nejdřív misi zkontroluje: skript, který entita používá, ale který neexistuje, skript, který nelze zkompilovat, chybějící start hráče, cíl, který nikam neukazuje, `.str` bez `ENDMARKER`. Chyby (červeně) se musí opravit, varování (oranžově) mohou zůstat. **{{MissionSaveNow}}** uloží všechny skripty, zapíše `.ent`, zkompiluje skripty, přidá vaše vlastní soubory ROFF a zabalí `.pk3` do složky hry.",
    "How a mission starts in the game":
        "Jak mise ve hře začíná",
    "The player spawns":
        "Hráč se objeví",
    "info_player_start":
        "info_player_start",
    "target_scriptrunner":
        "target_scriptrunner",
    "The intro script":
        "Úvodní skript",
    "affect blocks":
        "bloky affect",
    "its key target fires the runner":
        "jeho klíč target spustí spouštěč",
    "its key usescript names the script":
        "jeho klíč usescript jmenuje skript",
    "camera ENABLE ... DISABLE":
        "camera ENABLE ... DISABLE",
    "figures walk, look, speak":
        "postavy chodí, dívají se, mluví",
    "their spawnscripts run when they appear":
        "jejich spawnscripty běží, když se objeví",
    "A new mission sets this up for you - the Graph view shows the same chain.":
        "Nová mise to nastaví za vás - zobrazení Graf ukazuje tentýž řetěz.",
    "The {{MissionSaveTitle}} dialog with its checks.":
        "Dialog {{MissionSaveTitle}} s jeho kontrolami.",
    "The archive goes into the game folder that wins - the bottom one under **{{MenuEdit}} → {{PathsTitle}}** - as `zz_<name>.pk3`, so it wins over older files. Start the game and type `devmap <name>` in the console to play it.":
        "Archiv jde do složky hry, která vyhrává - té dolní v **{{MenuEdit}} → {{PathsTitle}}** - jako `zz_<name>.pk3`, aby vyhrál nad staršími soubory. Spusťte hru a pro hraní zadejte do konzole `devmap <name>`.",
    "Model view and characters":
        "Zobrazení modelu a postavy",
    "Look at any character of the game with its skins and animations, and put an animation into the script with a double-click.":
        "Prohlédněte si libovolnou postavu hry s jejími skiny a animacemi a dvojklikem vložte animaci do skriptu.",
    "Model mode: the animation list on the left, the model in the middle, the playback bar below.":
        "Režim Model: seznam animací vlevo, model uprostřed, lišta přehrávání dole.",
    "Switch to **{{ViewModel}}** in the mode bar. **{{NpcBrowser}}** lists every character of the game (from the `.npc` files) - click one to load its model and skin as the game shows it. **{{OpenGlm}}** and **{{OpenGlmPk3}}** open any model. Left mouse orbits, middle mouse pans, the wheel zooms; **{{ModelTurntable}}**, **{{ModelBones}}** and **{{ModelResetView}}** sit in the toolbar.":
        "Přepněte na **{{ViewModel}}** v liště režimů. **{{NpcBrowser}}** vypíše každou postavu hry (ze souborů `.npc`) - kliknutím ji načtete s modelem a skinem tak, jak ji ukazuje hra. **{{OpenGlm}}** a **{{OpenGlmPk3}}** otevřou libovolný model. Levé tlačítko krouží, prostřední posouvá, kolečko přibližuje; **{{ModelTurntable}}**, **{{ModelBones}}** a **{{ModelResetView}}** jsou v liště nástrojů.",
    "The **{{ModelAnimations}}** list on the left shows every animation of the skeleton with its length; filter by name or by BOTH / TORSO / LEGS / FACE. Click plays it; double-click (or **{{ModelAnimToScript}}**) puts `set ( SET_ANIM_BOTH, \"…\" )` after the selected line of the open script - `SET_ANIM_UPPER` for TORSO and `SET_ANIM_LOWER` for LEGS animations.":
        "Seznam **{{ModelAnimations}}** vlevo ukazuje každou animaci kostry s její délkou; filtrujte podle názvu nebo podle BOTH / TORSO / LEGS / FACE. Kliknutí ji přehraje; dvojklik (nebo **{{ModelAnimToScript}}**) vloží `set ( SET_ANIM_BOTH, \"…\" )` za vybraný řádek otevřeného skriptu - `SET_ANIM_UPPER` pro animace TORSO a `SET_ANIM_LOWER` pro LEGS.",
    "Your own mission: take-off on Taspir":
        "Vlastní mise: vzlet na Taspiru",
    "A complete little mission for Jedi Academy, built from nothing: Kyle and Rosh walk across a landing pad, the camera follows them, the Raven's Claw lifts off and flies past the camera, the picture shakes and fades to black - and the game plays it.":
        "Kompletní malá mise pro Jedi Academy, postavená z ničeho: Kyle a Rosh jdou přes přistávací plošinu, kamera je sleduje, Raven's Claw vzlétne a proletí kolem kamery, obraz se otřese a zatmí do černa - a hra to přehraje.",
    "Eight parts · base game · about 30 minutes":
        "Osm částí · základní hra · asi 30 minut",
    "The finished cutscene, seen through its camera.":
        "Hotová animační sekvence, viděná její kamerou.",
    "This tutorial uses only the base game: the map `taspir1` - the landing pad at the start of the Taspir III mission - with Kyle's ship, the Raven's Claw, and the characters Kyle and Rosh. You need nothing but Jedi Academy and JK Studio. Every picture on the following pages was taken while exactly this mission was built, step by step.":
        "Tento tutoriál používá jen základní hru: mapu `taspir1` - přistávací plošinu na začátku mise Taspir III - s Kylovou lodí Raven's Claw a postavami Kyle a Rosh. Nepotřebujete nic než Jedi Academy a JK Studio. Každý obrázek na následujících stránkách vznikl, zatímco se stavěla přesně tato mise, krok za krokem.",
    "The mission in eight parts":
        "Mise v osmi částech",
    "Create the mission":
        "Vytvoření mise",
    "Place the characters":
        "Umístění postav",
    "Let them walk":
        "Nechat je jít",
    "The camera":
        "Kamera",
    "The ship takes off":
        "Loď vzlétá",
    "Cut, shake, fade":
        "Střih, otřesy, zatmívání",
    "A second script":
        "Druhý skript",
    "Save and play":
        "Uložit a hrát",
    "player start + script runner":
        "start hráče + spouštěč skriptu",
    "NPC spawners, walk targets":
        "spawnery NPC, cíle chůze",
    "affect, task, dowait":
        "affect, task, dowait",
    "shots, travel time, pilot":
        "záběry, doba přesunu, pilot",
    "a ROFF file":
        "soubor ROFF",
    "SHAKE, PAN, FADE":
        "SHAKE, PAN, FADE",
    "spawnscript, macro":
        "spawnscript, makro",
    "zz_taspir_tut.pk3":
        "zz_taspir_tut.pk3",
    "devmap taspir_tut":
        "devmap taspir_tut",
    "Before you start":
        "Než začnete",
    "Add the game folder once: **{{MenuEdit}} → {{PathsTitle}}** → **{{Pk3AddPath}}** → the `base` folder of Jedi Academy (the one with `assets0.pk3`). See [[erste-schritte#spielordner]].":
        "Jednou přidejte složku hry: **{{MenuEdit}} → {{PathsTitle}}** → **{{Pk3AddPath}}** → složka `base` hry Jedi Academy (ta s `assets0.pk3`). Viz [[erste-schritte#spielordner]].",
    "If the window is new to you, read [[fenster]] first - five minutes.":
        "Je-li pro vás okno nové, přečtěte si nejdřív [[fenster]] - pět minut.",
    "Everything can be undone with <kbd>Ctrl</kbd>+<kbd>Z</kbd>. The game files are never changed: your mission goes into its own folder and its own `.pk3`.":
        "Vše lze vrátit pomocí <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Soubory hry se nikdy nemění: vaše mise jde do vlastní složky a vlastního `.pk3`.",
    "1 · Create the mission":
        "1 · Vytvoření mise",
    "A new mission on taspir1: map, player start, script runner and an intro script are made for you.":
        "Nová mise na taspir1: mapa, start hráče, spouštěč skriptu a úvodní skript se vytvoří za vás.",
    "**Open the start page.** Switch the mode bar to **{{ViewMap}}**. Without a map, the tab shows the start page.":
        "**Otevřete úvodní stránku.** Přepněte lištu režimů na **{{ViewMap}}**. Bez mapy ukazuje karta úvodní stránku.",
    "The start page of the Mission tab.":
        "Úvodní stránka karty Mise.",
    "**New mission.** Click **{{StartCardNew}}**. In the list type `taspir1` and pick it. As **{{MissionName}}** type `taspir_tut` - letters, digits and `_`. The **{{MissionFolder}}** is filled in by itself. Press **{{MissionCreate}}**.":
        "**Nová mise.** Klikněte na **{{StartCardNew}}**. V seznamu napište `taspir1` a vyberte ji. Jako **{{MissionName}}** napište `taspir_tut` - písmena, číslice a `_`. **{{MissionFolder}}** se vyplní samo. Stiskněte **{{MissionCreate}}**.",
    "The New mission window.":
        "Okno Nová mise.",
    "Map list":
        "Seznam map",
    "Type to search, click the map the mission is built on.":
        "Pište pro hledání, klikněte na mapu, na které je mise postavena.",
    "Letters, digits and `_`. The game loads it with `devmap <name>`.":
        "Písmena, číslice a `_`. Hra ji načte pomocí `devmap <name>`.",
    "The folder under `scripts/` for the mission's scripts.":
        "Složka pod `scripts/` pro skripty mise.",
    "Creates the mission - or closes the window.":
        "Vytvoří misi - nebo zavře okno.",
    "**Look at what you got.** The map opens on the landing pad. behaved has added a `target_scriptrunner` next to the player start: when the player spawns, the script `taspir_tut/intro` runs. It is open in a tab - a short camera shot behind the player start.":
        "**Podívejte se, co máte.** Mapa se otevře na přistávací plošině. behaved přidal vedle startu hráče `target_scriptrunner`: když se hráč objeví, spustí se skript `taspir_tut/intro`. Je otevřený na kartě - krátký záběr kamery za startem hráče.",
    "The new mission: the player start is selected, the intro script is open on the right.":
        "Nová mise: start hráče je vybraný, úvodní skript je otevřený vpravo.",
    "**Switch off the original intro.** taspir1 has its own start: its player start already triggers a runner with `taspir1/preIntro`, and the same trigger now starts yours too - both would play at once. Type `t271` into the search box of the entity list - that is the runner of the original (its `usescript` is `taspir1/preIntro`) - select it and press <kbd>Delete</kbd>. The **{{FlowGraph}}** view shows such connections at a glance ([[skriptfluss#graph]]).":
        "**Vypněte původní úvod.** taspir1 má vlastní začátek: jeho start hráče už spouští spouštěč s `taspir1/preIntro` a tentýž trigger teď spouští i ten váš - oba by hrály současně. Napište `t271` do vyhledávacího pole seznamu entit - to je spouštěč originálu (jeho `usescript` je `taspir1/preIntro`) - vyberte ho a stiskněte <kbd>Delete</kbd>. Zobrazení **{{FlowGraph}}** ukazuje taková spojení na první pohled ([[skriptfluss#graph]]).",
    "Every map of the game works this way. Its own entities stay; remove what you do not want - it changes only your mission, never the game.":
        "Takto funguje každá mapa hry. Její vlastní entity zůstávají; odeberte, co nechcete - mění se jen vaše mise, nikdy hra.",
    "Part %d of 8 · Taspir mission":
        "Část %d z 8 · mise Taspir",
    "2 · Place the characters":
        "2 · Umístění postav",
    "Kyle and Rosh at the south edge of the pad, and a walk target in front of the ship for each of them.":
        "Kyle a Rosh na jižním okraji plošiny a pro každého cíl chůze před lodí.",
    "**Look at the pad.** Hold the right mouse button and fly with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> until you see the pad from the south, a little from above.":
        "**Podívejte se na plošinu.** Podržte pravé tlačítko myši a leťte pomocí <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, dokud neuvidíte plošinu z jihu, trochu shora.",
    "Placing the two characters and their walk targets.":
        "Umisťování obou postav a jejich cílů chůze.",
    "**Place Kyle.** Click the tile **{{EntKindNpc}}** on the left edge of the view, then click near the south edge of the pad. A character marker appears and is selected.":
        "**Umístěte Kyla.** Klikněte na dlaždici **{{EntKindNpc}}** u levého okraje pohledu a pak klikněte blízko jižního okraje plošiny. Objeví se značka postavy a je vybraná.",
    "**Name him.** In the inspector under the entity list set **{{EntName}}** to `kyle` and `NPC_type` to `Kyle` (the **...** button lists every character of the game). **{{EntDropFloor}}** puts him exactly on the ground.":
        "**Pojmenujte ho.** V inspektoru pod seznamem entit nastavte **{{EntName}}** na `kyle` a `NPC_type` na `Kyle` (tlačítko **...** vypíše každou postavu hry). **{{EntDropFloor}}** ho postaví přesně na zem.",
    "The inspector with Kyle selected.":
        "Inspektor s vybraným Kylem.",
    "**Place Rosh** the same way beside him: **{{EntName}}** `rosh`, `NPC_type` `rosh_penin`.":
        "**Umístěte Roshe** stejně vedle něj: **{{EntName}}** `rosh`, `NPC_type` `rosh_penin`.",
    "**Two walk targets.** Click the tile **{{EntKindNav}}** and click in front of the ship - name it `nav_kyle`. A second one beside it: `nav_rosh`.":
        "**Dva cíle chůze.** Klikněte na dlaždici **{{EntKindNav}}** a klikněte před lodí - pojmenujte ho `nav_kyle`. Druhý vedle něj: `nav_rosh`.",
    "The new entities in the list and on the map.":
        "Nové entity v seznamu a v mapě.",
    "Hold <kbd>Shift</kbd> while clicking to place several entities of one kind. More in [[entities#setzen]].":
        "Podržením <kbd>Shift</kbd> při klikání umístíte několik entit jednoho druhu. Více v [[entities#setzen]].",
    "3 · Let them walk":
        "3 · Nechat je jít",
    "An affect block for each character: walk to the target, wait until there, then look at the other one.":
        "Blok affect pro každou postavu: jít k cíli, počkat, až tam bude, pak se podívat na toho druhého.",
    "**Write the walk for Kyle.** In the intro script, after the camera lines, insert `affect` (double-click it in the **{{GroupEvents}}** list) and give it the name `kyle`. Inside it insert `task` named `walk`, and inside the task `set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, nav_kyle )`. After the task: `dowait ( walk )` and `set ( SET_LOOK_TARGET, rosh )`.":
        "**Napište chůzi pro Kyla.** V úvodním skriptu za řádky kamery vložte `affect` (dvakrát na něj klikněte v seznamu **{{GroupEvents}}**) a dejte mu jméno `kyle`. Dovnitř vložte `task` s názvem `walk` a do úlohy `set ( SET_WALKING, true )` a `set ( SET_NAVGOAL, nav_kyle )`. Za úlohu: `dowait ( walk )` a `set ( SET_LOOK_TARGET, rosh )`.",
    "The intro script.":
        "Úvodní skript.",
    "**The same for Rosh** - copy the block (**{{ActCopy}}**, **{{ActPaste}}**) and change the names: `rosh`, `nav_rosh`, look target `kyle`.":
        "**Totéž pro Roshe** - zkopírujte blok (**{{ActCopy}}**, **{{ActPaste}}**) a změňte jména: `rosh`, `nav_rosh`, cíl pohledu `kyle`.",
    "**Watch them walk.** Drag the playhead in the timeline. Both walk at the same time - an `affect` block does not hold up the script. Click Kyle to see his path: grey ahead, yellow behind.":
        "**Sledujte je jít.** Táhněte ukazatel přehrávání na časové ose. Oba jdou současně - blok `affect` skript nezdržuje. Kliknutím na Kyla uvidíte jeho dráhu: šedě před ním, žlutě za ním.",
    "Kyle in the middle of his walk.":
        "Kyle uprostřed chůze.",
    "**Walk or run.** Right-click the walk target at the end of his path: **{{BahnGehen}}**, **{{BahnRennen}}**, **{{BahnRueckwaerts}}** or **{{BahnAnsehen}}** change how he gets there.":
        "**Chůze nebo běh.** Klikněte pravým tlačítkem na cíl chůze na konci jeho dráhy: **{{BahnGehen}}**, **{{BahnRennen}}**, **{{BahnRueckwaerts}}** nebo **{{BahnAnsehen}}** změní, jak se tam dostane.",
    "**A detour.** Grab Kyle in the middle of his walk and drag him aside: behaved inserts a new walk target there, and his path bends around it.":
        "**Oklika.** Uchopte Kyla uprostřed chůze a táhněte ho stranou: behaved tam vloží nový cíl chůze a jeho dráha se kolem něj ohne.",
    "A path point inserted in the middle of the walk.":
        "Bod dráhy vložený uprostřed chůze.",
    "Why the task? `SET_NAVGOAL` only starts the walk. `dowait` waits until the task is done - until he has arrived - so he looks at Rosh only when he stands. More in [[k-tasks]].":
        "Proč úloha? `SET_NAVGOAL` chůzi jen spustí. `dowait` počká, až je úloha hotová - až dorazí - takže se na Roshe podívá, až když stojí. Více v [[k-tasks]].",
    "4 · The camera":
        "4 · Kamera",
    "A wide shot of the pad, a move down behind the two, set from the view and flown by hand.":
        "Celkový záběr plošiny, sjezd dolů za ty dva, nastavený z pohledu a proletěný ručně.",
    "**The first shot** is already there - the template behind the player start. Select it in the **{{CamShots}}** list, fly out to a wide view of the pad from the south-east and press **{{CamSetToView}}**: the shot is now where you are looking.":
        "**První záběr** už tam je - šablona za startem hráče. Vyberte ho v seznamu **{{CamShots}}**, vyleťte k celkovému pohledu na plošinu z jihovýchodu a stiskněte **{{CamSetToView}}**: záběr je teď tam, kam se díváte.",
    "**A second shot.** Fly down behind Kyle and Rosh, low, looking at the ship. Press **{{CamNewShot}}** and set its **{{CamDuration}}** to `4` seconds: the camera flies from the wide shot to this one while the two walk.":
        "**Druhý záběr.** Sleťte dolů za Kyla a Roshe, nízko, s pohledem na loď. Stiskněte **{{CamNewShot}}** a nastavte jeho **{{CamDuration}}** na `4` sekundy: kamera letí z celkového záběru do tohoto, zatímco ti dva jdou.",
    "The camera with its shots and its path.":
        "Kamera se svými záběry a dráhou.",
    "**Fine-tune in the view.** The camera bar has **{{GizmoMove}}** (arrows) and **{{GizmoRotate}}** (rings). Releasing writes the change into the script.":
        "**Doladění v pohledu.** Lišta kamery má **{{GizmoMove}}** (šipky) a **{{GizmoRotate}}** (kruhy). Puštěním se změna zapíše do skriptu.",
    "Moving a shot with the arrows.":
        "Posun záběru šipkami.",
    "**Or fly it.** **{{CamPilot}}** puts you into the shot: right mouse looks, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move. <kbd>Enter</kbd> sets a key at the playhead, **{{CamPilotApply}}** keeps it, <kbd>Esc</kbd> throws it away.":
        "**Nebo ho proleťte.** **{{CamPilot}}** vás usadí do záběru: pravé tlačítko rozhlíží, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> pohyb. <kbd>Enter</kbd> nastaví klíč u ukazatele přehrávání, **{{CamPilotApply}}** ho ponechá, <kbd>Esc</kbd> ho zahodí.",
    "**Look through it.** **{{CamPreview}}** shows a small picture of what the camera sees; click it to look through the camera.":
        "**Podívejte se jím.** **{{CamPreview}}** ukáže malý obrázek toho, co kamera vidí; kliknutím se kamerou podíváte.",
    "The camera preview in the corner of the view.":
        "Náhled kamery v rohu pohledu.",
    "5 · The ship takes off":
        "5 · Loď vzlétá",
    "Animate the Raven's Claw with four keys and save the flight as a ROFF file.":
        "Animujte Raven's Claw čtyřmi klíči a uložte let jako soubor ROFF.",
    "**Select the ship.** The Raven's Claw on the pad is a `misc_model_breakable` named `ravensClaw`. Click it on the map, then **{{EntAnimate}}** in the inspector.":
        "**Vyberte loď.** Raven's Claw na plošině je `misc_model_breakable` s názvem `ravensClaw`. Klikněte na ni v mapě, pak na **{{EntAnimate}}** v inspektoru.",
    "The ship selected, the Animation window open.":
        "Loď vybraná, okno Animace otevřené.",
    "**ROFF file.** In the **{{AnimTitle}}** window choose **{{AnimModeRoff}}**. The script gets a block that plays the file, and the first key is where the ship stands now.":
        "**Soubor ROFF.** V okně **{{AnimTitle}}** zvolte **{{AnimModeRoff}}**. Skript dostane blok, který soubor přehraje, a první klíč je tam, kde loď teď stojí.",
    "The Animation window in ROFF mode.":
        "Okno Animace v režimu ROFF.",
    "How the movement is stored: as `move` commands in the script, or as a ROFF file the script plays.":
        "Jak se pohyb ukládá: jako příkazy `move` ve skriptu, nebo jako soubor ROFF, který skript přehraje.",
    "The name of the ROFF, as in `play ( PLAY_ROFF, ... )`; saved as `scripts/<name>.rof` next to the script.":
        "Název ROFF, jako v `play ( PLAY_ROFF, ... )`; uloží se jako `scripts/<name>.rof` vedle skriptu.",
    "On: one curve through all keys, easing in and out. Off: straight from key to key at constant speed.":
        "Zap.: jedna křivka přes všechny klíče, s rozjezdem a dojezdem. Vyp.: rovně od klíče ke klíči stálou rychlostí.",
    "Adds a key at the playhead, with the entity where it is now.":
        "Přidá klíč u ukazatele přehrávání, s entitou tam, kde je teď.",
    "Removes the selected key / moves the timeline to it.":
        "Odebere vybraný klíč / posune na něj časovou osu.",
    "Removes the animation block from the script.":
        "Odebere blok animace ze skriptu.",
    "Key table":
        "Tabulka klíčů",
    "Time, position and direction of every key. Type new values, or click a row to jump there.":
        "Čas, poloha a směr každého klíče. Zadejte nové hodnoty, nebo kliknutím na řádek tam skočíte.",
    "**Set the keys.** At 5 s press **{{AnimAddKey}}** - the ship still stands. At 6.5 s drag it straight up a little: a key is set at the playhead. At 8.5 s drag it forward and higher, and tip the nose up with the ring. At 10.5 s it is far away and high.":
        "**Nastavte klíče.** V 5 s stiskněte **{{AnimAddKey}}** - loď ještě stojí. V 6.5 s ji táhněte kousek přímo nahoru: u ukazatele přehrávání se nastaví klíč. V 8.5 s ji táhněte dopředu a výš a kruhem zvedněte příď. V 10.5 s je daleko a vysoko.",
    "The keys of the flight, with the rotate ring.":
        "Klíče letu s otočným kruhem.",
    "**Watch.** Play the timeline: the ship lifts off and flies away. **{{AnimRoffSmooth}}** makes one curve through the keys.":
        "**Sledujte.** Přehrajte časovou osu: loď vzlétne a odletí. **{{AnimRoffSmooth}}** udělá přes klíče jednu křivku.",
    "The Raven's Claw taking off.":
        "Raven's Claw vzlétá.",
    "**Saved with the script.** Saving writes `scripts/taspir_tut/roffs/ravensClaw.rof` next to the script; **{{MissionSave}}** packs it into the `.pk3`.":
        "**Uloženo se skriptem.** Uložení zapíše `scripts/taspir_tut/roffs/ravensClaw.rof` vedle skriptu; **{{MissionSave}}** ho zabalí do `.pk3`.",
    "Animating with move commands instead of a file: [[k-tuer]]. Changing a ROFF of the game: [[k-roff-bearbeiten]].":
        "Animace příkazy move místo souboru: [[k-tuer]]. Změna ROFF hry: [[k-roff-bearbeiten]].",
    "6 · Cut, shake, follow, fade":
        "6 · Střih, otřesy, sledování, zatmívání",
    "A third shot from outside the pad, a shake at lift-off, the camera turning after the ship, a fade to black - and the timing fixed in the timeline.":
        "Třetí záběr zvenku plošiny, otřesy při vzletu, kamera otáčející se za lodí, zatmívání do černa - a načasování opravené na časové ose.",
    "**Cut.** After the second camera move insert a `wait` of 5 seconds, then a new shot from outside the pad, looking at the ship, with **{{CamDuration}}** `0` - a cut.":
        "**Střih.** Za druhý pohyb kamery vložte `wait` o 5 sekundách, pak nový záběr zvenku plošiny s pohledem na loď, s **{{CamDuration}}** `0` - střih.",
    "**Shake.** Insert `camera` and choose `SHAKE`: strength `4`, `2000` milliseconds - the engines start.":
        "**Otřesy.** Vložte `camera` a zvolte `SHAKE`: síla `4`, `2000` milisekund - motory startují.",
    "**Follow the ship.** Two `camera ( PAN )` commands with a duration turn the camera up after the ship while it climbs; a `wait` after each lets them finish.":
        "**Sledujte loď.** Dva příkazy `camera ( PAN )` s délkou otočí kameru nahoru za lodí, zatímco stoupá; `wait` po každém je nechá doběhnout.",
    "**Fade out.** `camera` with `FADE`: from `0 0 0` and `0` to `0 0 0` and `1` in `1500` ms - black. Then `camera ( DISABLE )` gives control back to the player.":
        "**Zatmívání.** `camera` s `FADE`: z `0 0 0` a `0` na `0 0 0` a `1` za `1500` ms - černá. Pak `camera ( DISABLE )` vrátí ovládání hráči.",
    "**Fix the timing.** Play and watch. Drag blocks in the timeline: everything after moves along. Drag a right edge to change a duration.":
        "**Opravte načasování.** Přehrajte a sledujte. Táhněte bloky na časové ose: vše po nich se posune s nimi. Tažením pravého okraje změníte délku.",
    "**The whole scene.** Turn on **{{MapThroughCam}}** and play from the start.":
        "**Celá scéna.** Zapněte **{{MapThroughCam}}** a přehrajte od začátku.",
    "The finished cutscene through the camera.":
        "Hotová animační sekvence pohledem kamery.",
    "7 · A second script":
        "7 · Druhý skript",
    "Kyle gets a spawn script with a macro - and the split view shows both scripts side by side.":
        "Kyle dostane spawn skript s makrem - a rozdělené zobrazení ukáže oba skripty vedle sebe.",
    "**A spawnscript key.** Select Kyle, add the key `spawnscript` with the value `taspir_tut/kyle_spawn` (**{{EntAddKey}}** in the inspector).":
        "**Klíč spawnscript.** Vyberte Kyla, přidejte klíč `spawnscript` s hodnotou `taspir_tut/kyle_spawn` (**{{EntAddKey}}** v inspektoru).",
    "**Open creates it.** Press **{{EntOpenScript}}** next to the key: the script does not exist yet, so behaved creates it in the mission folder and opens it in a tab.":
        "**Otevřít ho vytvoří.** Stiskněte **{{EntOpenScript}}** vedle klíče: skript ještě neexistuje, takže ho behaved vytvoří ve složce mise a otevře na kartě.",
    "**A macro.** Double-click `standOnly` at the end of the **{{GroupEvents}}** list: three `set` commands that make Kyle stand and ignore enemies. Add `set ( SET_WEAPON, WP_SABER )`.":
        "**Makro.** Dvakrát klikněte na `standOnly` na konci seznamu **{{GroupEvents}}**: tři příkazy `set`, které nechají Kyla stát a ignorovat nepřátele. Přidejte `set ( SET_WEAPON, WP_SABER )`.",
    "The spawn script with the standOnly macro.":
        "Spawn skript s makrem standOnly.",
    "**Side by side.** **{{MenuFile}} → {{SplitToggle}} → 2** shows the intro beside the spawn script. Drag lines from one to the other to copy them.":
        "**Vedle sebe.** **{{MenuFile}} → {{SplitToggle}} → 2** ukáže úvod vedle spawn skriptu. Přetažením řádků z jednoho do druhého je zkopírujete.",
    "8 · Save and play":
        "8 · Uložit a hrát",
    "Check the mission, pack it into a .pk3 and start it in the game.":
        "Zkontrolujte misi, zabalte ji do .pk3 a spusťte ji ve hře.",
    "**Save mission.** Press **{{MissionSave}}** in the map toolbar. The window lists every file it will pack and checks the mission: errors in red must be fixed, warnings in orange may stay.":
        "**Uložte misi.** Stiskněte **{{MissionSave}}** na liště mapy. Okno vypíše každý soubor, který zabalí, a misi zkontroluje: chyby červeně se musí opravit, varování oranžově mohou zůstat.",
    "**Pack.** Press **{{MissionSaveNow}}**: scripts are saved and compiled, the `.ent` with your entities and the ROFF are written, and `zz_taspir_tut.pk3` goes into the game folder.":
        "**Zabalení.** Stiskněte **{{MissionSaveNow}}**: skripty se uloží a zkompilují, `.ent` s vašimi entitami a ROFF se zapíší a `zz_taspir_tut.pk3` jde do složky hry.",
    "The message after saving.":
        "Zpráva po uložení.",
    "**Play.** Start Jedi Academy, open the console (<kbd>Shift</kbd>+<kbd>~</kbd>) and type `devmap taspir_tut`. The cutscene runs as soon as you spawn.":
        "**Hrajte.** Spusťte Jedi Academy, otevřete konzoli (<kbd>Shift</kbd>+<kbd>~</kbd>) a zadejte `devmap taspir_tut`. Animační sekvence běží, jakmile se objevíte.",
    "**Next time.** **{{MenuFile}} → {{RecentMissions}}** opens the whole mission again - map, entities and both scripts.":
        "**Příště.** **{{MenuFile}} → {{RecentMissions}}** znovu otevře celou misi - mapu, entity i oba skripty.",
    "Recent missions in the File menu.":
        "Poslední mise v nabídce Soubor.",
    "Share the `.pk3` with others: it holds everything the mission needs. Players put it into their `base` folder.":
        "Sdílejte `.pk3` s ostatními: obsahuje vše, co mise potřebuje. Hráči ho vloží do své složky `base`.",
    "Start with the Taspir mission - it builds a whole cutscene for Jedi Academy from nothing and touches almost every part of the program. The short tutorials below each show one thing in a few steps.":
        "Začněte misí Taspir - postaví z ničeho celou animační sekvenci pro Jedi Academy a dotkne se skoro každé části programu. Krátké tutoriály níže ukazují každý jednu věc v několika krocích.",
    "Interactive tutorials in the program":
        "Interaktivní tutoriály v programu",
    "Some tutorials also run inside the program: **{{MenuHelp}} → {{TourMenu}}** (or **{{TourStartGuide}}** on the start page) opens a small window with the list. Pick one and press **{{TourStart}}**: the program darkens the screen except for the control you need, frames it in yellow and moves on by itself as soon as you have done the step - whether you used the button, the menu or the shortcut.":
        "Některé tutoriály běží i přímo v programu: **{{MenuHelp}} → {{TourMenu}}** (nebo **{{TourStartGuide}}** na úvodní stránce) otevře malé okno se seznamem. Vyberte jeden a stiskněte **{{TourStart}}**: program ztmaví obrazovku kromě ovládacího prvku, který potřebujete, orámuje ho žlutě a sám pokračuje, jakmile krok uděláte - ať jste použili tlačítko, nabídku nebo zkratku.",
    "An interactive tutorial: the tutorial window and the highlighted control.":
        "Interaktivní tutoriál: okno tutoriálu a zvýrazněný ovládací prvek.",
    "**{{TourBack}}** goes one step back. **{{TourSkip}}** skips a step - where it can, the program prepares the next one (it never changes your script). **{{TourNext}}** moves on after a step that only shows something.":
        "**{{TourBack}}** jde o krok zpět. **{{TourSkip}}** krok přeskočí - kde to jde, program připraví další (nikdy nemění váš skript). **{{TourNext}}** pokračuje po kroku, který jen něco ukazuje.",
    "**{{TourShowMe}}** moves a pointer to the right place; nothing is clicked. **{{TourDoIt}}** does the step for you - offered only where that is safe, never for saving (which asks for a file name).":
        "**{{TourShowMe}}** přesune ukazatel na správné místo; na nic se neklikne. **{{TourDoIt}}** udělá krok za vás - nabízí se jen tam, kde je to bezpečné, nikdy pro ukládání (které se ptá na název souboru).",
    "**{{TourGuide}}** opens the page with its pictures in your browser. The search box below the list searches this guide, offline.":
        "**{{TourGuide}}** otevře stránku s jejími obrázky v prohlížeči. Vyhledávací pole pod seznamem prohledává tuto příručku, offline.",
    "The program remembers how far you got; next time you continue where you stopped.":
        "Program si pamatuje, kam jste se dostali; příště pokračujete, kde jste přestali.",
    "The first five are the script tutorials [[k-neues-skript]], [[k-befehl-einfuegen]], [[k-werte-bearbeiten]], [[k-zeilen-ordnen]] and [[k-uebersetzen]], in the {{FlowClassic}} view; then [[tut-karte]] in the map editor , [[k-kamera-ansicht]] in the preview and [[k-modell]]. Or press <kbd>F1</kbd> and ask the helper ([[helfer]]).":
        "Prvních pět jsou tutoriály ke skriptům [[k-neues-skript]], [[k-befehl-einfuegen]], [[k-werte-bearbeiten]], [[k-zeilen-ordnen]] a [[k-uebersetzen]] v pohledu {{FlowClassic}}; pak [[tut-karte]] v editoru map, [[k-kamera-ansicht]] v náhledu a [[k-modell]]. Nebo stiskněte <kbd>F1</kbd> a zeptejte se pomocníka ([[helfer]]).",
    "Also interactive: figures and paths ([[k-gangart]], [[k-wegpunkte]]), playing and retiming in the timeline ([[k-neu-timen]]), a new mission ([[tut-p1]]), skins ([[k-skins]]), the game folder ([[k-spielordner]]), updates ([[k-update]]), a door with a trigger in the map editor ([[tut-karte-tuer]]) and the menu editor ([[tut-menue]]).":
        "Také interaktivní: postavy a dráhy ([[k-gangart]], [[k-wegpunkte]]), přehrávání a přečasování na časové ose ([[k-neu-timen]]), nová mise ([[tut-p1]]), skiny ([[k-skins]]), složka hry ([[k-spielordner]]), aktualizace ([[k-update]]), dveře s triggerem v editoru map ([[tut-karte-tuer]]) a editor menu ([[tut-menue]]).",
    "All tutorials at a glance":
        "Všechny tutoriály v přehledu",
    "Pick any of them - each one stands on its own. Below the list: the big project, then every short tutorial as a card with a picture.":
        "Vyberte si kterýkoli - každý stojí sám za sebe. Pod seznamem: velký projekt, pak každý krátký tutoriál jako karta s obrázkem.",
    "The big project":
        "Velký projekt",
    "Your first map":
        "Vaše první mapa",
    "Missions and models":
        "Mise a modely",
    "The big one: your own mission":
        "Ten velký: vlastní mise",
    "All tutorials use the base game - the map `taspir1` and its characters - so the pictures match what you see. Any other map, and any mod such as Movie Duels, works the same way. If the parts of the window are new to you, read [[fenster]] and [[karte#bewegen]] first.":
        "Všechny tutoriály používají základní hru - mapu `taspir1` a její postavy - takže obrázky odpovídají tomu, co vidíte. Jakákoli jiná mapa a jakýkoli mod jako Movie Duels funguje stejně. Jsou-li pro vás části okna nové, přečtěte si nejdřív [[fenster]] a [[karte#bewegen]].",
    "Build a room in the map editor, light it, walk through it in the editor and bring it into the game as a .pk3.":
        "Postavte místnost v editoru map, nasvětlete ji, projděte ji v editoru a přeneste do hry jako .pk3.",
    "Short tutorials":
        "Krátké tutoriály",
    "Each one shows one small thing in three to six steps - start with any of them.":
        "Každý ukazuje jednu malou věc ve třech až šesti krocích - začněte kterýmkoli.",
    "Settings and updates":
        "Nastavení a aktualizace",
    "Where the program keeps its paths and options, how to change keys, and how updates arrive.":
        "Kde program uchovává své cesty a volby, jak měnit klávesy a jak přicházejí aktualizace.",
    "Game folders":
        "Složky hry",
    "**{{MenuEdit}} → {{PathsTitle}}** lists the folders whose `.pk3` archives the program reads, with **{{PrefsUp}}**, **{{PrefsDown}}**, **{{PrefsRemove}}** and **{{PrefsOpenFolder}}** for each. The bottom one wins over everything above it, as `fs_game` overrides `base` in the engine. A folder without archives says so - then the path is wrong.":
        "**{{MenuEdit}} → {{PathsTitle}}** vypíše složky, jejichž archivy `.pk3` program čte, s **{{PrefsUp}}**, **{{PrefsDown}}**, **{{PrefsRemove}}** a **{{PrefsOpenFolder}}** u každé. Dolní vyhrává nade vším nad ní, jako `fs_game` v enginu přepisuje `base`. Složka bez archivů to oznámí - pak je cesta špatně.",
    "The game folders.":
        "Složky hry.",
    "Jedi Outcast (JK2)":
        "Jedi Outcast (JK2)",
    "JK Studio also works with the original Jedi Outcast. Its compiled scripts (`.ibi`, version 1.57) and its maps have the same format as in Jedi Academy, and ICARUS has the same commands. In **{{AppPrefs}}** → **{{PrefsRegelwerk}}**, **{{PrefsRegelJk2}}** is the third rule set next to OpenJK and Movie Duels; the model for it is the JK2 game code in OpenJK (`codeJK2`).":
        "JK Studio funguje i s původním Jedi Outcast. Jeho zkompilované skripty (`.ibi`, verze 1.57) a jeho mapy mají stejný formát jako v Jedi Academy a ICARUS má stejné příkazy. V **{{AppPrefs}}** → **{{PrefsRegelwerk}}** je **{{PrefsRegelJk2}}** třetí sadou pravidel vedle OpenJK a Movie Duels; jejím vzorem je kód hry JK2 v OpenJK (`codeJK2`).",
    "**Command set**: under Jedi Outcast the lists for animations, mission objectives, behaviour states and SET_ fields come from JK2 (`data/jk2`). Of the 1356 scripts in Jedi Outcast's `assets0.pk3`, 1352 compile back byte for byte; the check leaves 326 notes, which are real mistakes in Raven's scripts. An own command file in the preferences always wins.":
        "**Sada příkazů**: v Jedi Outcast pocházejí seznamy animací, úkolů mise, stavů chování a polí SET_ z JK2 (`data/jk2`). Z 1356 skriptů v `assets0.pk3` Jedi Outcast se 1352 zkompiluje zpět bajt po bajtu; kontrola zanechá 326 poznámek, což jsou skutečné chyby ve skriptech Raven. Vlastní soubor příkazů v předvolbách vždy vyhrává.",
    "**Game folders**: Jedi Outcast has its own list in **{{MenuEdit}} → {{PathsTitle}}** (**{{PathsJk2}}**). Both games have files with the same name but different content, so the two lists are never mixed; the list of the active rule set is used. **{{Pk3AddPath}}** puts a Jedi Outcast folder into its list by itself.":
        "**Složky hry**: Jedi Outcast má vlastní seznam v **{{MenuEdit}} → {{PathsTitle}}** (**{{PathsJk2}}**). Obě hry mají soubory se stejným názvem, ale jiným obsahem, takže se oba seznamy nikdy nemíchají; použije se seznam aktivní sady pravidel. **{{Pk3AddPath}}** dá složku Jedi Outcast do jejího seznamu sama.",
    "**Automatic**: Jedi Outcast when only Jedi Outcast folders are set up, or when a Jedi Outcast mission is opened with **{{LoadMission}}** (`jk2sp.exe` next to the folder, or JK2 maps such as `kejim_post`). A Movie Duels folder always means Movie Duels; when it is unclear, OpenJK.":
        "**Automaticky**: Jedi Outcast, když jsou nastaveny jen složky Jedi Outcast, nebo když se pomocí **{{LoadMission}}** otevře mise Jedi Outcast (`jk2sp.exe` vedle složky, nebo mapy JK2 jako `kejim_post`). Složka Movie Duels vždy znamená Movie Duels; když není jasno, OpenJK.",
    "**Figures**: NPCs come from `ext_data/NPCs.cfg`, the player is `kyle` (also in scripts), human figures use JK2's skeleton with 72 bones, and JK2's NPC classes pick JK2's figures. Lightsabers need no `.sab` file: the hilt and the blade length come from the NPC class (Desann 48, reborn 32, everyone else 40) and the colour from `saberColor` in NPCs.cfg.":
        "**Postavy**: NPC pocházejí z `ext_data/NPCs.cfg`, hráč je `kyle` (i ve skriptech), lidské postavy používají kostru JK2 se 72 kostmi a třídy NPC JK2 vybírají postavy JK2. Světelné meče nepotřebují soubor `.sab`: jílec a délka čepele pocházejí ze třídy NPC (Desann 48, reborni 32, všichni ostatní 40) a barva ze `saberColor` v NPCs.cfg.",
    "**Walking and camera**: NPCs walk to navgoals with JK2's navigator: straight when the way is free, otherwise along the waypoint network from `maps/<map>.nav` (built from the waypoints when the file is missing or does not match the map; then connections blocked at map start are re-checked every 1.5 s like the game), with JK2's speeds - running starts at full speed, walking speeds up step by step, and NPCs slow down shortly before the goal; an NPC that cannot get around another NPC tells it to step aside - also one that is standing without a goal, but never the player - and two NPCs blocking each other keep going; a connection blocked by a locked door, something breakable or another NPC is marked as failed for all NPCs, who then take another route until it is free again, like the game. Weapons come from JK2's NPC table, an NPC Kyle takes the player's weapon (also when a script changes it outside the camera, until Kyle gets a weapon of his own), and the blade flickers like in JK2. The camera shows FOLLOW and shake one frame later, camera ROFFs ignore notes, the swamp water in `yavin_swamp` is not solid, and rain and snow follow JK2. `dowait` on a SET_NAVGOAL that finds no goal waits forever, as in the game.":
        "**Chůze a kamera**: NPC chodí k navgoalům s navigátorem JK2: rovně, když je cesta volná, jinak po síti navigačních bodů z `maps/<map>.nav` (sestavené z navigačních bodů, když soubor chybí nebo k mapě nepatří; pak se spojení zablokovaná při startu mapy znovu kontrolují každých 1.5 s jako ve hře), s rychlostmi JK2 - běh začíná plnou rychlostí, chůze zrychluje krok za krokem a NPC krátce před cílem zpomalí; NPC, které nemůže obejít jiné NPC, mu řekne, ať uhne - i takovému, které stojí bez cíle, ale nikdy hráči - a dvě NPC, která se navzájem blokují, pokračují; spojení zablokované zamčenými dveřmi, něčím rozbitným nebo jiným NPC se označí jako neprůchodné pro všechna NPC, která pak jdou jinou cestou, dokud není zase volné, jako ve hře. Zbraně pocházejí z tabulky NPC JK2, NPC Kyle bere zbraň hráče (i když ji skript mimo kameru změní, dokud Kyle nedostane vlastní zbraň) a čepel bliká jako v JK2. Kamera ukazuje FOLLOW a otřesy o jeden snímek později, ROFF kamery ignorují poznámky, bažinatá voda v `yavin_swamp` není pevná a déšť a sníh se řídí JK2. `dowait` na SET_NAVGOAL, který nenajde cíl, čeká navždy, jako ve hře.",
    "Not yet exactly like Jedi Outcast: an NPC that gets blocked by another one does not push it aside, blocked waypoints and connections are not remembered during the scene, and subtitles are not shown.":
        "Zatím ne přesně jako v Jedi Outcast: NPC, které jiné zablokuje, ho neodstrčí, zablokované navigační body a spojení se během scény nepamatují a titulky se nezobrazují.",
    "**{{MenuEdit}} → {{AppPrefs}}** (or **{{AppPrefs}}** in the action column, <kbd>Alt</kbd>+<kbd>P</kbd>) holds the options of the original BehavEd and those of the cutscene preview:":
        "**{{MenuEdit}} → {{AppPrefs}}** (nebo **{{AppPrefs}}** ve sloupci akcí, <kbd>Alt</kbd>+<kbd>P</kbd>) obsahuje volby původního BehavEd a volby náhledu animační sekvence:",
    "**{{PrefsDirectories}}**: {{PrefsScriptPath}}, {{PrefsCommandFile}} (the `.bhc` that defines every command - **{{PrefsReload}}** reads it again), {{PrefsSourcePath}}.":
        "**{{PrefsDirectories}}**: {{PrefsScriptPath}}, {{PrefsCommandFile}} (`.bhc`, který definuje každý příkaz - **{{PrefsReload}}** ho znovu přečte), {{PrefsSourcePath}}.",
    "**{{PrefsTonPegel}}**: {{PrefsTonEffekte}}, {{PrefsTonStimmen}}, {{PrefsTonMusik}} - like the game's sound settings; **{{PrefsTonSpielwerte}}** restores 0.5 / 1.0 / 0.25. How the preview sounds: [[zeitleiste#ton]].":
        "**{{PrefsTonPegel}}**: {{PrefsTonEffekte}}, {{PrefsTonStimmen}}, {{PrefsTonMusik}} - jako nastavení zvuku ve hře; **{{PrefsTonSpielwerte}}** obnoví 0.5 / 1.0 / 0.25. Jak náhled zní: [[zeitleiste#ton]].",
    "**{{PrefsGroupPreview}}** → **{{PrefsRegelwerk}}** decides which game the cutscene preview imitates: OpenJK (plain Jedi Academy) or Movie Duels, whose game code differs in places - NPC turn speeds per class and difficulty, how often NPCs re-steer and turn (OpenJK every 100 ms, Movie Duels every 50 ms), the default difficulty, lightsaber clashes, cg_fovAspectAdjust and the game window, ROFF sounds on figures. The third choice is Jedi Outcast ([[einstellungen#jedioutcast]]). Automatic picks Movie Duels when a Movie Duels folder is among the game folders, Jedi Outcast for Jedi Outcast folders, otherwise OpenJK; the status list names the rule set when a map loads. Aspect ratio and cg_fovAspectAdjust themselves are under **{{MapViewSettings}}** → **{{GameScreen}}** ([[kamera#vorschau]]).":
        "**{{PrefsGroupPreview}}** → **{{PrefsRegelwerk}}** rozhoduje, kterou hru náhled animační sekvence napodobuje: OpenJK (čisté Jedi Academy) nebo Movie Duels, jehož kód hry se místy liší - rychlosti otáčení NPC podle třídy a obtížnosti, jak často NPC přeřizují a otáčejí se (OpenJK každých 100 ms, Movie Duels každých 50 ms), výchozí obtížnost, střety světelných mečů, cg_fovAspectAdjust a okno hry, zvuky ROFF na postavách. Třetí volbou je Jedi Outcast ([[einstellungen#jedioutcast]]). Automaticky vybere Movie Duels, je-li mezi složkami hry složka Movie Duels, Jedi Outcast pro složky Jedi Outcast, jinak OpenJK; seznam stavů uvede sadu pravidel při načtení mapy. Samotný poměr stran a cg_fovAspectAdjust jsou v **{{MapViewSettings}}** → **{{GameScreen}}** ([[kamera#vorschau]]).",
    "**{{PrefsDifficulty}}**: {{DiffPadawan}}, {{DiffJedi}}, {{DiffKnight}}, {{DiffMaster}} - the game's g_spskill. The first entry is the game's default (Movie Duels {{DiffKnight}}, OpenJK {{DiffPadawan}}). Like the game, it changes how fast stormtroopers, reborn and some other NPCs turn, how fast tusken raiders walk and run (from {{DiffKnight}} on) and how close two lightsabers must come to clash. Entities the map marks for other difficulties (or `notsingle`) are left out, as the game removes them when the map loads - also an NPC that a script names but whose spawner is removed this way (the script's `affect` on it does nothing). Changing it rebuilds the preview at once.":
        "**{{PrefsDifficulty}}**: {{DiffPadawan}}, {{DiffJedi}}, {{DiffKnight}}, {{DiffMaster}} - g_spskill hry. První položka je výchozí ve hře (Movie Duels {{DiffKnight}}, OpenJK {{DiffPadawan}}). Jako ve hře mění, jak rychle se otáčejí stormtroopeři, reborni a některá další NPC, jak rychle chodí a běhají tuskenští nájezdníci (od {{DiffKnight}}) a jak blízko se musí dva světelné meče dostat, aby se střetly. Entity, které mapa označuje pro jiné obtížnosti (nebo `notsingle`), se vynechají, jako je hra při načtení mapy odebere - i NPC, které skript jmenuje, ale jehož spawner je takto odebrán (`affect` skriptu na něj nic nedělá). Změna náhled hned znovu sestaví.",
    "Language, theme and interface size are in the **{{MenuView}}** menu.":
        "Jazyk, motiv a velikost rozhraní jsou v nabídce **{{MenuView}}**.",
    "The preview part of the preferences: sound levels, game rules and difficulty.":
        "Část předvoleb pro náhled: hlasitost zvuku, pravidla hry a obtížnost.",
    "Changing keys":
        "Změna kláves",
    "**{{MenuEdit}} → {{KeysTitle}}** lists every action that has a key. Click a shortcut and press the new key; <kbd>Esc</kbd> cancels, <kbd>Backspace</kbd> clears. A key that is already taken is named. The defaults are in [[tasten]].":
        "**{{MenuEdit}} → {{KeysTitle}}** vypíše každou akci, která má klávesu. Klikněte na zkratku a stiskněte novou klávesu; <kbd>Esc</kbd> zruší, <kbd>Backspace</kbd> vymaže. Klávesa, která už je obsazená, se uvede. Výchozí hodnoty jsou v [[tasten]].",
    "Updates and changelog":
        "Aktualizace a seznam změn",
    "**{{MenuHelp}} → {{UpdMenu}}** asks GitHub for a newer version and shows what is new; **{{UpdInstall}}** replaces the program files (with the single `.exe`, just the program, whatever its file name) and offers a restart. The switch next to it decides whether this happens at startup. **{{MenuHelp}} → {{ChangelogMenu}}** shows what was fixed and added, version by version, in your language - it also opens once by itself after an update.":
        "**{{MenuHelp}} → {{UpdMenu}}** se zeptá GitHubu na novější verzi a ukáže, co je nového; **{{UpdInstall}}** nahradí soubory programu (u samostatného `.exe` jen program, ať má jakýkoli název souboru) a nabídne restart. Přepínač vedle rozhoduje, zda se to děje při spuštění. **{{MenuHelp}} → {{ChangelogMenu}}** ukáže, co bylo opraveno a přidáno, verze po verzi, ve vašem jazyce - po aktualizaci se také jednou otevře samo.",
    "Keyboard shortcuts":
        "Klávesové zkratky",
    "The default keys. Most of them can be changed under Edit → Keyboard shortcuts.":
        "Výchozí klávesy. Většinu z nich lze změnit v Úpravy → Klávesové zkratky.",
    "Script":
        "Skript",
    "Edit the selected command":
        "Upravit vybraný příkaz",
    "Insert the command selected in the Events list":
        "Vložit příkaz vybraný v seznamu Události",
    "Delete":
        "Odstranit",
    "Duplicate":
        "Duplikovat",
    "Copy":
        "Kopírovat",
    "Cut (also Ctrl+T)":
        "Vyjmout (také Ctrl+T)",
    "Paste":
        "Vložit",
    "REM: comment out / back (Ctrl+Backspace: uncomment)":
        "REM: zakomentovat / zpět (Ctrl+Backspace: odkomentovat)",
    "Move up (Alt+Down: down), staying in the block":
        "Posunout nahoru (Alt+Down: dolů), v rámci bloku",
    "Undo (Ctrl+Y or Ctrl+Shift+Z: redo)":
        "Zpět (Ctrl+Y nebo Ctrl+Shift+Z: znovu)",
    "Find; F3 / Shift+F3 next / previous":
        "Najít; F3 / Shift+F3 další / předchozí",
    "Toggle bookmark; F2 / Shift+F2 next / previous":
        "Přepnout záložku; F2 / Shift+F2 další / předchozí",
    "Expand item and children (Ctrl+Num+: all)":
        "Rozbalit položku a potomky (Ctrl+Num+: vše)",
    "Collapse item (Ctrl+Num-: all)":
        "Sbalit položku (Ctrl+Num-: vše)",
    "Files":
        "Soubory",
    "Open this guide":
        "Otevřít tuto příručku",
    "Open":
        "Otevřít",
    "Save":
        "Uložit",
    "Save as (as in the original BehavEd)":
        "Uložit jako (jako v původním BehavEd)",
    "Save all":
        "Uložit vše",
    "Backup; Alt+R restore":
        "Záloha; Alt+R obnovit",
    "Recently opened files":
        "Naposledy otevřené soubory",
    "Prefs; Alt+X exit":
        "Předvolby; Alt+X konec",
    "Map and camera":
        "Mapa a kamera",
    "W A S D":
        "W A S D",
    "Fly (mouse over the view); Q / E down / up, Shift faster; right mouse looks around":
        "Let (myš nad pohledem); Q / E dolů / nahoru, Shift rychleji; pravé tlačítko rozhlíží",
    "Save view 1 (… 9); 1 … 9 jumps back":
        "Uložit pohled 1 (… 9); 1 … 9 skočí zpět",
    "While dragging an entity: rings in 15-degree steps; with a grid set it turns the grid off, without one an arrow snaps to 8 units":
        "Při tažení entity: kruhy po 15 stupních; s nastavenou mřížkou ji vypne, bez ní přichytí šipku k 8 jednotkám",
    "Alt+drag an entity: copy it first":
        "Alt+tažení entity: nejdřív ji zkopírovat",
    "While dragging a flight target or a key diamond: only up and down":
        "Při tažení cíle letu nebo kosočtverce klíče: jen nahoru a dolů",
    "Cancel placing, the waypoint tool or piloting":
        "Zrušit umisťování, nástroj navigačních bodů nebo pilotování",
    "While piloting: set a camera key at the playhead":
        "Při pilotování: nastavit klíč kamery u ukazatele přehrávání",
    "Left / Right":
        "Left / Right",
    "50 ms back / forward, mouse over the timeline (Shift: 1 s)":
        "50 ms zpět / dopředu, myš nad časovou osou (Shift: 1 s)",
    "Home / End":
        "Home / End",
    "Jump to start / end (mouse over the timeline)":
        "Skok na začátek / konec (myš nad časovou osou)",
    "While dragging a block: move only this block":
        "Při tažení bloku: posunout jen tento blok",
    "Window":
        "Okno",
    "Ctrl + wheel":
        "Ctrl + kolečko",
    "Interface size (over the timeline tracks: zoom the timeline)":
        "Velikost rozhraní (nad stopami časové osy: přiblížení časové osy)",
    "Glossary":
        "Slovníček",
    "The words of Jedi Academy scripting in one place.":
        "Slova skriptování Jedi Academy na jednom místě.",
    "Term":
        "Pojem",
    "Meaning":
        "Význam",
    "The scripting system of the game. Scripts are text (`.txt`) and run compiled (`.ibi`).":
        "Skriptovací systém hry. Skripty jsou text (`.txt`) a běží zkompilované (`.ibi`).",
    "A compiled script - what the game actually runs. **{{Compile}}** makes it.":
        "Zkompilovaný skript - to, co hra skutečně spouští. Vytvoří ho **{{Compile}}**.",
    "The command description file: every command, its fields and their choices.":
        "Soubor s popisem příkazů: každý příkaz, jeho pole a jejich volby.",
    "A block whose commands go to one named figure or entity.":
        "Blok, jehož příkazy jdou jedné pojmenované postavě nebo entitě.",
    "A named group of commands; `do` starts it alongside, `dowait` waits until it is done.":
        "Pojmenovaná skupina příkazů; `do` ji spustí souběžně, `dowait` počká, až skončí.",
    "A pause in milliseconds. In the game it ends one frame (50 ms) later than written.":
        "Pauza v milisekundách. Ve hře končí o jeden snímek (50 ms) později, než je napsáno.",
    "Sends a figure walking to a `waypoint_navgoal` - straight when it is close, otherwise over the waypoint network. See [[figuren#wieimspiel]].":
        "Pošle postavu k `waypoint_navgoal` - rovně, když je blízko, jinak po síti navigačních bodů. Viz [[figuren#wieimspiel]].",
    "How fast an NPC turns: 60 + 3 × value degrees per second.":
        "Jak rychle se NPC otáčí: 60 + 3 × hodnota stupňů za sekundu.",
    "Movie Duels only: lets the blades of NPCs in `BS_CINEMATIC` clash. See [[figuren#schwerter]].":
        "Jen Movie Duels: nechá čepele NPC v `BS_CINEMATIC` střetávat se. Viz [[figuren#schwerter]].",
    "The entity that makes a character appear; `NPC_type` says which one, `NPC_targetname` names it for scripts.":
        "Entita, která nechá objevit se postavu; `NPC_type` říká kterou, `NPC_targetname` ji pojmenuje pro skripty.",
    "A named mark in the map - for camera positions and look targets.":
        "Pojmenovaná značka v mapě - pro polohy kamery a cíle pohledu.",
    "Runs a script when it is used (by a trigger, another entity or a script).":
        "Spustí skript, když je použit (triggerem, jinou entitou nebo skriptem).",
    "A brush model of the map that scripts can move - ships, doors, platforms.":
        "Brush model mapy, který skripty mohou pohybovat - lodě, dveře, plošiny.",
    "A recorded movement: one small step of position and angle per game frame. See [[animation#roff]].":
        "Zaznamenaný pohyb: jeden malý krok polohy a úhlu na herní snímek. Viz [[animation#roff]].",
    "A zip archive the game reads its files from. A mission is packed into one.":
        "Archiv zip, ze kterého hra čte své soubory. Mise se do jednoho zabalí.",
    "The entities of a map in a separate file; Movie Duels keeps the map and puts its own entities next to it.":
        "Entity mapy v samostatném souboru; Movie Duels ponechá mapu a vlastní entity dá vedle ní.",
    "A large Jedi Academy mod with cutscene missions. Its game code differs from OpenJK in places; the preview follows either one - [[einstellungen#prefs]].":
        "Velký mod Jedi Academy s misemi s animačními sekvencemi. Jeho kód hry se od OpenJK místy liší; náhled se řídí kterýmkoli z nich - [[einstellungen#prefs]].",
    "Every command":
        "Každý příkaz",
    "What each command of the Events list does, every camera command, every macro and every SET_ field - with the value it expects.":
        "Co dělá každý příkaz seznamu Události, každý příkaz kamery, každé makro a každé pole SET_ - s hodnotou, kterou očekává.",
    "Commands are inserted from the **{{GroupEvents}}** list (double-click, drag or <kbd>Insert</kbd>) and changed in the **{{EditorTitle}}** (double-click the line). Hover over a command in the list to read the help text the game's designers wrote for it. How to put them together: [[skripte]].":
        "Příkazy se vkládají ze seznamu **{{GroupEvents}}** (dvojklik, přetažení nebo <kbd>Insert</kbd>) a mění v **{{EditorTitle}}** (dvojklik na řádek). Najeďte myší na příkaz v seznamu a přečtěte si text nápovědy, který k němu napsali designéři hry. Jak je skládat dohromady: [[skripte]].",
    "The commands of the Events list":
        "Příkazy seznamu Události",
    "Each command has its own page with pictures of the editor and of all three views: [[cmd]].":
        "Každý příkaz má vlastní stránku s obrázky editoru a všech tří pohledů: [[cmd]].",
    "Command":
        "Příkaz",
    "Example":
        "Příklad",
    "Clears every command still waiting for this entity - what an `affect` block with `FLUSH` does at its start.":
        "Smaže každý příkaz, který na tuto entitu ještě čeká - to, co dělá blok `affect` s `FLUSH` na svém začátku.",
    "Runs the block only if the comparison holds (`=`, `<`, `>`, `!` for *not equal*). `else` must follow right after an `if`.":
        "Spustí blok, jen když porovnání platí (`=`, `<`, `>`, `!` pro *nerovná se*). `else` musí následovat hned za `if`.",
    "Repeats its block; the number says how often, `-1` means forever.":
        "Opakuje svůj blok; číslo říká, jak často, `-1` znamená navždy.",
    "Sends the commands of its block to one entity - a figure, a ship, a door - by its name. `FLUSH` replaces what the entity was doing, `INSERT` adds to it. The script itself does not wait for the block.":
        "Pošle příkazy svého bloku jedné entitě - postavě, lodi, dveřím - podle jejího názvu. `FLUSH` nahradí, co entita dělala, `INSERT` k tomu přidá. Skript sám na blok nečeká.",
    "Starts another script on the entity that runs this one.":
        "Spustí další skript na entitě, na které běží tento.",
    "Pauses for so many milliseconds - or, with a name, until that task is finished.":
        "Čeká tolik milisekund - nebo s názvem, dokud tato úloha neskončí.",
    "Pauses until some other script calls `signal` with the same name. Only one script can wait for a given signal.":
        "Čeká, dokud jiný skript nezavolá `signal` se stejným názvem. Na daný signál může čekat jen jeden skript.",
    "Wakes the script that waits for this name.":
        "Probudí skript, který na tento název čeká.",
    "Plays a sound on a channel of the entity - `CHAN_VOICE` for dialogue, so the mouth moves.":
        "Přehraje zvuk na kanálu entity - `CHAN_VOICE` pro dialog, aby se hýbala ústa.",
    "Moves an entity (a door, a platform, a ship) to a position - and, if given, to new angles - in so many milliseconds. The Animate window writes these for you ([[k-tuer]]).":
        "Posune entitu (dveře, plošinu, loď) na polohu - a je-li zadáno, na nové úhly - za tolik milisekund. Okno Animace je píše za vás ([[k-tuer]]).",
    "Turns an entity to angles (pitch, yaw, roll) in so many milliseconds.":
        "Otočí entitu na úhly (sklon, otočení, náklon) za tolik milisekund.",
    "Uses an entity by name, as if the player pressed it - opens a door, fires a trigger.":
        "Použije entitu podle názvu, jako by ji stiskl hráč - otevře dveře, spustí trigger.",
    "Kills the entity with this name.":
        "Zabije entitu s tímto názvem.",
    "Removes the entity with this name from the game.":
        "Odebere entitu s tímto názvem ze hry.",
    "Shows a text in the middle of the screen.":
        "Zobrazí text uprostřed obrazovky.",
    "A comment - does nothing in the game. **{{ActRem}}** turns any line into one.":
        "Komentář - ve hře nic nedělá. **{{ActRem}}** z něj udělá kterýkoli řádek.",
    "Declares a variable (`FLOAT`, `STRING`, `VECTOR`; at most 16 per map) - `free` gives the name back.":
        "Deklaruje proměnnou (`FLOAT`, `STRING`, `VECTOR`; nejvýš 16 na mapu) - `free` název vrátí.",
    "Changes a value: an entity field (`SET_NAVGOAL`, `SET_ANIM_BOTH`, ... - see the full list below) or a declared variable.":
        "Mění hodnotu: pole entity (`SET_NAVGOAL`, `SET_ANIM_BOTH`, ... - viz úplný seznam níže) nebo deklarovanou proměnnou.",
    "Controls the cutscene camera; the first value chooses what it does - see the next section.":
        "Ovládá kameru animační sekvence; první hodnota volí, co dělá - viz další část.",
    "Declares a named group of commands - it does not run yet.":
        "Deklaruje pojmenovanou skupinu příkazů - ta ještě neběží.",
    "Starts a task and goes on at once.":
        "Spustí úlohu a hned pokračuje.",
    "Starts a task and waits until it is done - for a walk: until the figure has arrived.":
        "Spustí úlohu a počká, až skončí - u chůze: až postava dorazí.",
    "Plays a ROFF file on the entity of the current `affect` block - a recorded movement.":
        "Přehraje soubor ROFF na entitě aktuálního bloku `affect` - zaznamenaný pohyb.",
    "A command marked with `*` in its help text (for example `SET_NAVGOAL`) finishes its task only when the action is done - that is what lets `dowait` wait for it.":
        "Příkaz označený v textu nápovědy `*` (například `SET_NAVGOAL`) dokončí svou úlohu, až když je akce hotová - díky tomu na něj může `dowait` čekat.",
    "camera ( ... ) - the camera commands":
        "camera ( ... ) - příkazy kamery",
    "Values":
        "Hodnoty",
    "What it does (help text of the game)":
        "Co dělá (text nápovědy hry)",
    "Puts game in camera mode":
        "Přepne hru do režimu kamery",
    "Takes game out of camera mode":
        "Vyvede hru z režimu kamery",
    "number, whole number":
        "číslo, celé číslo",
    "Normal is 80, 10 is zoomed in, max is 120.  Second value is time in ms to take to zoom to the new FOV":
        "Normální je 80, 10 je přiblíženo, maximum je 120.  Druhá hodnota je čas v ms, za který se přiblíží na nové FOV",
    "vector, whole number":
        "vektor, celé číslo",
    "Move to a absolute vector origin or TAG(\"targetname\", ORIGIN) over time over number of milliseconds":
        "Přesun na absolutní počátek vektoru nebo TAG(\"targetname\", ORIGIN) v průběhu daného počtu milisekund",
    "vector, vector, whole number":
        "vektor, vektor, celé číslo",
    "Pan to absolute angle from current angle in dir (no dir will use shortest) over number of milliseconds":
        "Otočení na absolutní úhel z aktuálního úhlu ve směru dir (bez dir použije nejkratší) v průběhu daného počtu milisekund",
    "Roll to relative angle offsets of current angle over number of milliseconds":
        "Náklon o relativní posun aktuálního úhlu v průběhu daného počtu milisekund",
    "text, number, whole number":
        "text, číslo, celé číslo",
    "Get on track and move at speed, last number is whether or not to lerp to the start pos":
        "Najet na trať a pohybovat se rychlostí, poslední číslo určuje, zda plynule přejít do výchozí polohy",
    "Follow ents with matching cameraGroup at angleSpeed, last number is whether or not to lerp to the start angle":
        "Sledovat entity se shodnou cameraGroup rychlostí angleSpeed, poslední číslo určuje, zda plynule přejít na výchozí úhel",
    "Keep this distance from cameraGroup (if any), last number is whether or not to lerp to the start angle":
        "Držet tuto vzdálenost od cameraGroup (pokud nějaká je), poslední číslo určuje, zda plynule přejít na výchozí úhel",
    "vector, number, vector, number, whole number":
        "vektor, číslo, vektor, číslo, celé číslo",
    "Fade from [start Red Green Blue], [Opacity] to [end Red Green Blue], [Opacity] (all fields valid ranges are 0 to 1) over [number of milliseconds]":
        "Prolnutí z [počáteční červená zelená modrá], [krytí] na [koncová červená zelená modrá], [krytí] (platný rozsah všech polí je 0 až 1) v průběhu [počtu milisekund]",
    "Intensity (0-16) and duration, in milliseconds":
        "Intenzita (0-16) a doba v milisekundách",
    "text":
        "text",
    "Path to ROF file":
        "Cesta k souboru ROF",
    "Macros":
        "Makra",
    "At the end of the **{{GroupEvents}}** list. A macro inserts several `set` commands at once; **{{TreeFoldMacros}}** shows it as one line.":
        "Na konci seznamu **{{GroupEvents}}**. Makro vloží několik příkazů `set` najednou; **{{TreeFoldMacros}}** ho ukáže jako jeden řádek.",
    "Macro":
        "Makro",
    "Commands":
        "Příkazy",
    "simply stands, ignores enemies & alerts":
        "jen stojí, ignoruje nepřátele a poplachy",
    "simply walks, ignores enemies & alerts":
        "jen chodí, ignoruje nepřátele a poplachy",
    "simply runs, ignores enemies & alerts":
        "jen běhá, ignoruje nepřátele a poplachy",
    "simply stands, ignores alerts":
        "jen stojí, ignoruje poplachy",
    "simply walks, ignores alerts":
        "jen chodí, ignoruje poplachy",
    "simply runs, ignores alerts":
        "jen běhá, ignoruje poplachy",
    "simply stands, ignores enemies":
        "jen stojí, ignoruje nepřátele",
    "simply walks, ignores enemies":
        "jen chodí, ignoruje nepřátele",
    "simply runs, ignores enemies":
        "jen běhá, ignoruje nepřátele",
    "looks for enemies, shoots at but doesn't chase them":
        "hledá nepřátele, střílí na ně, ale nepronásleduje je",
    "patrols in a run, chases enemies":
        "hlídkuje během, pronásleduje nepřátele",
    "patrols, sticks to patrol route even when shooting enemy":
        "hlídkuje, drží se hlídkové trasy i při střelbě na nepřítele",
    "patrols in a walk, sticks to walking patrol route even when shooting enemy":
        "hlídkuje chůzí, drží se pěší hlídkové trasy i při střelbě na nepřítele",
    "patrols in a run, sticks to running patrol route even when shooting enemy":
        "hlídkuje během, drží se běžecké hlídkové trasy i při střelbě na nepřítele",
    "resets to default behavior":
        "obnoví výchozí chování",
    "set ( SET_... ) - every field":
        "set ( SET_... ) - každé pole",
    "All fields `set` can change, grouped as in the game's own list (`Q3_Interface.h`). The description is the one the editor shows - Raven's own words. Type part of a name into the search box of this guide to find a field.":
        "Všechna pole, která `set` může měnit, seskupená jako ve vlastním seznamu hry (`Q3_Interface.h`). Popis je ten, který ukazuje editor - vlastní slova Raven. Pole najdete tak, že část názvu napíšete do vyhledávacího pole této příručky.",
    "Group":
        "Skupina",
    "Field":
        "Pole",
    "Value":
        "Hodnota",
    "Parm strings":
        "Řetězce parm",
    "Set entity parm1":
        "Nastaví parm1 entity",
    "Set entity parm2":
        "Nastaví parm2 entity",
    "Set entity parm3":
        "Nastaví parm3 entity",
    "Set entity parm4":
        "Nastaví parm4 entity",
    "Set entity parm5":
        "Nastaví parm5 entity",
    "Set entity parm6":
        "Nastaví parm6 entity",
    "Set entity parm7":
        "Nastaví parm7 entity",
    "Set entity parm8":
        "Nastaví parm8 entity",
    "Set entity parm9":
        "Nastaví parm9 entity",
    "Set entity parm10":
        "Nastaví parm10 entity",
    "Set entity parm11":
        "Nastaví parm11 entity",
    "Set entity parm12":
        "Nastaví parm12 entity",
    "Set entity parm13":
        "Nastaví parm13 entity",
    "Set entity parm14":
        "Nastaví parm14 entity",
    "Set entity parm15":
        "Nastaví parm15 entity",
    "Set entity parm16":
        "Nastaví parm16 entity",
    "Scripts and other file paths":
        "Skripty a jiné cesty k souborům",
    "Script to run when spawned":
        "Skript, který se spustí při spawnu",
    "Script to run when used":
        "Skript, který se spustí při použití",
    "Script to run when startled":
        "Skript, který se spustí při vylekání",
    "Script run when find an enemy for the first time":
        "Skript, který se spustí, když poprvé najde nepřítele",
    "Script to run when you shoot":
        "Skript, který se spustí, když střílíte",
    "Script to run when killed someone":
        "Skript, který se spustí, když někoho zabije",
    "Script to run when you can't find your enemy":
        "Skript, který se spustí, když nemůžete najít nepřítele",
    "Script to run when hit":
        "Skript, který se spustí při zásahu",
    "Script to run when hit and low health":
        "Skript, který se spustí při zásahu s nízkým zdravím",
    "Script to run when killed":
        "Skript, který se spustí při smrti",
    "Script to run after a delay":
        "Skript, který se spustí po prodlevě",
    "Script to run when blocked by teammate":
        "Skript, který se spustí při zablokování spoluhráčem",
    "Script to run when player has shot own team repeatedly":
        "Skript, který se spustí, když hráč opakovaně střelí do vlastního týmu",
    "Script to run when player kills a teammate":
        "Skript, který se spustí, když hráč zabije spoluhráče",
    "Play a video (inGame)":
        "Přehrát video (ve hře)",
    "Script to run when skipping the running cinematic":
        "Skript, který se spustí při přeskočení běžící animační sekvence",
    "Turn off the centered movers on the given track":
        "Vypnout vystředěné movery na dané trati",
    "Turn on the centered movers on the given track":
        "Zapnout vystředěné movery na dané trati",
    "just blindly sets whatever skin you set!  include full path after \"base/\"... eg: \"models/players/tavion_new/model_possessed.skin\"":
        "prostě slepě nastaví jakýkoli skin, který zadáte!  uveďte celou cestu za \"base/\"... např.: \"models/players/tavion_new/model_possessed.skin\"",
    "Standard strings":
        "Standardní řetězce",
    "Set enemy by targetname":
        "Nastavit nepřítele podle targetname",
    "Set for BS_FOLLOW_LEADER":
        "Nastavení pro BS_FOLLOW_LEADER",
    "*Move to this navgoal then continue script":
        "*Dojít k tomuto navgoalu, pak pokračovat ve skriptu",
    "Set captureGoal by targetname":
        "Nastavit captureGoal podle targetname",
    "Set angles toward ent by targetname":
        "Nastavit úhly k entitě podle targetname",
    "Set angles toward ent by targetname, will *continue* to face them... only in BS_CINEMATIC":
        "Nastavit úhly k entitě podle targetname, bude se k ní *stále* natáčet... jen v BS_CINEMATIC",
    "Set/change your targetname":
        "Nastavit/změnit vlastní targetname",
    "Set/change what to use when hit":
        "Nastavit/změnit, co použít při zásahu",
    "all ents with this cameraGroup will be focused on":
        "všechny entity s touto cameraGroup budou zaměřeny",
    "What tag on all clients to try to track":
        "Který tag na všech klientech se pokusit sledovat",
    "object for NPC to look at":
        "objekt, na který se má NPC dívat",
    "object to place on NPC right hand bolt":
        "objekt, který se umístí na bolt pravé ruky NPC",
    "object to remove from NPC right hand bolt":
        "objekt, který se odebere z boltu pravé ruky NPC",
    "object to place on NPC left hand bolt":
        "objekt, který se umístí na bolt levé ruky NPC",
    "object to remove from NPC left hand bolt":
        "objekt, který se odebere z boltu levé ruky NPC",
    "Color of text RED,WHITE,BLUE, YELLOW":
        "Barva textu RED,WHITE,BLUE, YELLOW",
    "Copy the origin of the ent with targetname to your origin":
        "Zkopírovat počátek entity s targetname do vlastního počátku",
    "This NPC will attack the target NPC's enemies":
        "Toto NPC bude útočit na nepřátele cílového NPC",
    "Set/change your target":
        "Nastavit/změnit vlastní target",
    "Set/change your target2, on NPC's, this fires when they're knocked out by the red hypo":
        "Nastavit/změnit vlastní target2, u NPC se spustí, když je vyřadí červená injekce",
    "What trigger_location you're in - Can only be gotten, not set!":
        "Ve kterém trigger_location se nacházíte - lze jen číst, ne nastavit!",
    "Target that is fired when someone completes the BS_REMOVE behaviorState":
        "Cíl, který se spustí, když někdo dokončí stav chování BS_REMOVE",
    "Load the savegame that was auto-saved when you started the holodeck":
        "Načíst uloženou hru, která se automaticky uložila při spuštění holodecku",
    "Lock legs to a certain yaw angle (or \"off\" or \"auto\" uses current)":
        "Zamknout nohy na určitý úhel otočení (nebo \"off\" či \"auto\" použije aktuální)",
    "Make the player look through this ent's eyes - also shunts player movement control to this ent":
        "Nechat hráče dívat se očima této entity - také přesune ovládání pohybu hráče na tuto entitu",
    "Looping sound to play on entity":
        "Zvuk ve smyčce, který se přehrává na entitě",
    "Specify name of entity to freeze - !!!NOTE!!! since the ent is frozen, you must have some other entity unfreeze it!!!":
        "Zadejte název entity ke zmrazení - !!!POZOR!!! protože entita je zmrazená, musí ji rozmrazit nějaká jiná entita!!!",
    "Specify name of entity to unfreeze":
        "Zadejte název entity k rozmrazení",
    "Name of a saber in sabers.cfg to use in first hand.  \"none\" removes current saber":
        "Název meče v sabers.cfg pro první ruku.  \"none\" odebere aktuální meč",
    "Name of a saber in sabers.cfg to use in second hand.  \"none\" removes current saber":
        "Název meče v sabers.cfg pro druhou ruku.  \"none\" odebere aktuální meč",
    "Name of an NPC config in NPC2.cfg to use for this ent":
        "Název konfigurace NPC v NPC2.cfg pro tuto entitu",
    "Name of an vehicle config in vehicles.cfg to make this ent drive":
        "Název konfigurace vozidla ve vehicles.cfg, které má tato entita řídit",
    "key of text string to print":
        "klíč textového řetězce k vypsání",
    "key of text string to print in LCARS frame":
        "klíč textového řetězce k vypsání v rámečku LCARS",
    "key of text string to print in center of screen.":
        "klíč textového řetězce k vypsání uprostřed obrazovky.",
    "vectors":
        "vektory",
    "vector":
        "vektor",
    "Set origin explicitly or with TAG":
        "Nastavit počátek přímo nebo pomocí TAG",
    "Set angles explicitly or with TAG":
        "Nastavit úhly přímo nebo pomocí TAG",
    "Set origin here as soon as the area is clear":
        "Nastavit počátek sem, jakmile je oblast volná",
    "Removes this ent's saber from their hand, turns it off, and places it at the specified location":
        "Odebere entitě meč z ruky, vypne ho a umístí ho na zadané místo",
    "floats":
        "desetinná čísla",
    "number":
        "číslo",
    "Velocity along X axis":
        "Rychlost podél osy X",
    "Velocity along Y axis":
        "Rychlost podél osy Y",
    "Velocity along Z axis":
        "Rychlost podél osy Z",
    "Vertical offset from original origin... offset/ent's speed * 1000ms is duration":
        "Svislý posun od původního počátku... posun/rychlost entity * 1000ms je doba",
    "Pitch for NPC to turn to":
        "Sklon, na který se má NPC otočit",
    "Yaw for NPC to turn to":
        "Otočení, na které se má NPC otočit",
    "Speed-up slow down game (0 - 1.0)":
        "Zrychlení/zpomalení hry (0 - 1.0)",
    "when following an ent with the camera, apply this z ofs":
        "při sledování entity kamerou použít tento posun z",
    "How far away NPC can see":
        "Jak daleko NPC vidí",
    "How far an NPC can hear":
        "Jak daleko NPC slyší",
    "How often to look for enemies (0 - 1.0)":
        "Jak často hledat nepřátele (0 - 1.0)",
    "Change this ent's gravity - 800 default":
        "Změnit gravitaci této entity - výchozí 800",
    "Set face to Aux expression for number of seconds":
        "Nastavit obličeji výraz Aux na daný počet sekund",
    "Set face to Blink expression for number of seconds":
        "Nastavit obličeji výraz Blink na daný počet sekund",
    "Set face to Blinkfrown expression for number of seconds":
        "Nastavit obličeji výraz Blinkfrown na daný počet sekund",
    "Set face to Frown expression for number of seconds":
        "Nastavit obličeji výraz Frown na daný počet sekund",
    "Set face to Normal expression for number of seconds":
        "Nastavit obličeji výraz Normal na daný počet sekund",
    "Set face to Eyes closed":
        "Nastavit obličeji zavřené oči",
    "Set face to Eyes open":
        "Nastavit obličeji otevřené oči",
    "Change an entity's wait field":
        "Změnit pole wait entity",
    "How far away to stay from leader in BS_FOLLOW_LEADER":
        "Jak daleko se držet od vůdce v BS_FOLLOW_LEADER",
    "Scale the entity model":
        "Škálovat model entity",
    "Used to ensure rendering for entities with geographically sprawling animations (world units)":
        "Zajišťuje vykreslení entit s prostorově rozsáhlými animacemi (jednotky světa)",
    "Only to be used in a 'get'. (Distance to player)*(Distance to player)":
        "Používat jen v 'get'. (Vzdálenost k hráči)*(Vzdálenost k hráči)",
    "ints":
        "celá čísla",
    "whole number":
        "celé číslo",
    "Hold lower anim for number of milliseconds":
        "Držet animaci dolní části na daný počet milisekund",
    "Hold upper anim for number of milliseconds":
        "Držet animaci horní části na daný počet milisekund",
    "Hold lower and upper anims for number of milliseconds":
        "Držet animace dolní i horní části na daný počet milisekund",
    "Change health":
        "Změnit zdraví",
    "Change armor":
        "Změnit brnění",
    "Change walkSpeed":
        "Změnit walkSpeed",
    "Change runSpeed":
        "Změnit runSpeed",
    "Change yawSpeed":
        "Změnit yawSpeed",
    "Change aggression 1-5":
        "Změnit agresivitu 1-5",
    "Change aim 1-5":
        "Změnit míření 1-5",
    "Change ent's friction - 6 default":
        "Změnit tření entity - výchozí 6",
    "How far the ent can shoot - 0 uses weapon":
        "Jak daleko entita dostřelí - 0 použije zbraň",
    "Horizontal field of view":
        "Vodorovné zorné pole",
    "Vertical field of view":
        "Svislé zorné pole",
    "How many milliseconds to wait before running delayscript":
        "Kolik milisekund čekat před spuštěním delayscript",
    "NPC move forward -127(back) to 127":
        "Pohyb NPC dopředu -127(dozadu) až 127",
    "NPC move right -127(left) to 127":
        "Pohyb NPC doprava -127(doleva) až 127",
    "frame to start animation sequence on":
        "snímek, na kterém začne animační sekvence",
    "frame to end animation sequence on":
        "snímek, na kterém skončí animační sekvence",
    "frame to set animation sequence to":
        "snímek, na který se nastaví animační sekvence",
    "Change an entity's count field":
        "Změnit pole count entity",
    "Time between shots for an NPC - reset to defaults when changes weapon":
        "Čas mezi výstřely NPC - při změně zbraně se obnoví na výchozí",
    "Amount of time until Mission Status should be shown after death":
        "Doba, za kterou se po smrti ukáže stav mise",
    "Width of NPC bounding box.":
        "Šířka ohraničujícího rámečku NPC.",
    "Activate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).":
        "Aktivovat určitou čepel meče 1 (0 - (MAX_BLADES - 1)).",
    "Deactivate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).":
        "Deaktivovat určitou čepel meče 1 (0 - (MAX_BLADES - 1)).",
    "Activate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).":
        "Aktivovat určitou čepel meče 2 (0 - (MAX_BLADES - 1)).",
    "Deactivate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).":
        "Deaktivovat určitou čepel meče 2 (0 - (MAX_BLADES - 1)).",
    "Damage this entity with set amount.":
        "Způsobit této entitě poškození v zadané výši.",
    "booleans":
        "pravdivostní hodnoty",
    "Do not react to pain":
        "Nereagovat na bolest",
    "Do not acquire enemies":
        "Nevybírat si nepřátele",
    "Do not get enemy set by allies in area(ambush)":
        "Nepřebírat nepřítele nastaveného spojenci v oblasti (přepadení)",
    "Others won't shoot you":
        "Ostatní na vás nebudou střílet",
    "Others won't pick you as enemy":
        "Ostatní si vás nevyberou jako nepřítele",
    "Don't fire your weapon":
        "Nestřílet ze své zbraně",
    "Keep current enemy until dead":
        "Držet aktuálního nepřítele, dokud nezemře",
    "Force NPC to crouch":
        "Přinutit NPC dřepět",
    "Force NPC to move at walkSpeed":
        "Přinutit NPC pohybovat se rychlostí walkSpeed",
    "Force NPC to move at runSpeed":
        "Přinutit NPC pohybovat se rychlostí runSpeed",
    "NPC will chase after enemies":
        "NPC bude pronásledovat nepřátele",
    "NPC will be on the lookout for enemies":
        "NPC bude vyhlížet nepřátele",
    "NPC will face in the direction it's moving":
        "NPC se bude dívat ve směru pohybu",
    "NPC will not run from danger":
        "NPC nebude utíkat před nebezpečím",
    "NPC will not move unless you aim at him":
        "NPC se nepohne, dokud na něj nezamíříte",
    "Can take damage down to 1 but not die":
        "Může utrpět poškození až na 1, ale nezemře",
    "Will not avoid other NPCs or architecture":
        "Nebude se vyhýbat jiným NPC ani architektuře",
    "Make yourself notsolid or solid":
        "Udělat se nepevným nebo pevným",
    "Can be activateby the player's \"use\" button":
        "Lze aktivovat tlačítkem \"use\" hráče",
    "For non-NPCs, loop your animation sequence":
        "U ne-NPC opakovat animační sekvenci ve smyčce",
    "Player interface on/off":
        "Rozhraní hráče zap./vyp.",
    "NPC has no shields (Borg do not adapt)":
        "NPC nemá štíty (Borgové se nepřizpůsobí)",
    "Makes an NPC not solid and not visible":
        "Udělá NPC nepevné a neviditelné",
    "Draws only in mirrors/portals":
        "Kreslí se jen v zrcadlech/portálech",
    "Force Invincibility effect, also godmode":
        "Vynutit efekt nezranitelnosti, také godmode",
    "Makes an NPC greet teammates":
        "Nechá NPC zdravit spoluhráče",
    "Makes video playback fade in":
        "Nechá přehrávání videa roztmívat",
    "Makes video playback fade out":
        "Nechá přehrávání videa zatmívat",
    "Makes it so player cannot move":
        "Zařídí, že se hráč nemůže pohnout",
    "Makes it so player cannot switch weapons":
        "Zařídí, že hráč nemůže přepínat zbraně",
    "Stops this ent from taking impact damage":
        "Zabrání této entitě utrpět poškození nárazem",
    "Stops this ent from taking knockback from weapons":
        "Zabrání, aby tuto entitu odhazovaly zbraně",
    "Force NPC to use altfire when shooting":
        "Přinutit NPC při střelbě použít alternativní palbu",
    "NPCs will do generic responses when this is on (usescripts override generic responses as well)":
        "Když je zapnuto, NPC dávají obecné odpovědi (usescripty přepisují i obecné odpovědi)",
    "Completely unkillable":
        "Zcela nezabitelný",
    "NPCs will not do their combat talking noises when this is on":
        "Když je zapnuto, NPC nedělají své bojové zvuky mluvení",
    "Player has turned on his own- scripts will stop, NPCs will turn on him and level changes load the brig":
        "Hráč se obrátil proti svým - skripty se zastaví, NPC se obrátí proti němu a změny úrovně načtou vězení",
    "Allows turning off an animating shader in a script":
        "Umožní ve skriptu vypnout animovaný shader",
    "Sets a shader with an image map to be under frame control":
        "Nastaví shader s obrázkovou mapou pod řízení snímků",
    "Turns saber on/off":
        "Zapíná/vypíná meč",
    "Only set this on things you move with script commands that you *want* to open/close area portals.  Default is off.":
        "Nastavujte to jen u věcí, které pohybujete skriptovými příkazy a *chcete*, aby otevíraly/zavíraly area portály.  Výchozí je vypnuto.",
    "When true, only a heavy weapon class missile/laser can damage this ent.":
        "Když je true, může tuto entitu poškodit jen střela/laser třídy těžkých zbraní.",
    "When true, ion_cannon is shielded from any kind of damage.":
        "Když je true, je ion_cannon chráněn před jakýmkoli poškozením.",
    "This NPC cannot alert groups or be part of a group":
        "Toto NPC nemůže alarmovat skupiny ani být součástí skupiny",
    "Makes NPC will hold down the fire button, until this is set to false":
        "NPC bude držet tlačítko palby, dokud se to nenastaví na false",
    "NPC will hold down the fire button, but they won't play firing anim":
        "NPC bude držet tlačítko palby, ale nepřehraje animaci střelby",
    "NPC will remove only when it's safe (Player is not in PVS)":
        "NPC se odebere, jen když je to bezpečné (hráč není v PVS)",
    "Turn on/off Boba Fett's Jet Pack":
        "Zapnout/vypnout raketový batoh Boby Fetta",
    "Makes NPC immune to jedi mind-trick":
        "Udělá NPC odolné vůči ovládnutí mysli Jedi",
    "in lieu of using a target_activate or target_deactivate":
        "místo použití target_activate nebo target_deactivate",
    "provides an alternate way of changing func_usable to be visible or not, DOES NOT AFFECT SOLID":
        "jiný způsob, jak přepnout viditelnost func_usable, NEOVLIVŇUJE PEVNOST",
    "Increment secret areas found counter":
        "Zvýšit počítadlo nalezených tajných oblastí",
    "End of game dissolve into star background and credits":
        "Konec hry: rozplynutí do hvězdného pozadí a titulky",
    "NPCs will use their closest combat points, not try and find ones next to the player, or flank player":
        "NPC použijí své nejbližší bojové body, nebudou hledat ty u hráče ani hráče obcházet",
    "NPC will have a minlight of 96":
        "NPC bude mít minlight 96",
    "NPC will not be affected by force powers":
        "NPC nebudou ovlivňovat schopnosti Síly",
    "NPC will not scream and tumble and fall to hit death over large drops":
        "NPC nebude křičet, kutálet se a padat k smrti při velkých pádech",
    "NPC will not be dismemberable if you set this to false (default is true)":
        "NPC nepůjde rozsekat, nastavíte-li to na false (výchozí je true)",
    "Jedi won't jump, roll or cartwheel":
        "Jedi nebude skákat, kutálet se ani dělat hvězdy",
    "When true NPC will always display subtitle regardless of subtitle setting":
        "Je-li true, NPC vždy zobrazí titulky bez ohledu na nastavení titulků",
    "Removes entities that could muck up cinematics, explosives, turrets, seekers.":
        "Odebere entity, které by mohly pokazit animační sekvence, výbušniny, věže, seekery.",
    "Turns on/off HUD":
        "Zapíná/vypíná HUD",
    "This entity will *always* be drawn - use only for special case cinematic NPCs that have anims that cover multiple rooms!!!":
        "Tato entita se bude *vždy* kreslit - používejte jen pro zvláštní NPC v animačních sekvencích s animacemi přes více místností!!!",
    "Set a Saboteur to cloak (true) or un-cloak (false).":
        "Nastavit Sabotérovi maskování (true) nebo zrušení maskování (false).",
    "Causes this ent to start force healing at whatever level of force heal they have":
        "Tato entita začne léčení Silou na své úrovni léčení Silou",
    "Causes this ent to start force speeding at whatever level of force speed they have (may not do anything for NPCs?)":
        "Tato entita začne zrychlení Silou na své úrovni zrychlení Silou (u NPC možná nic neudělá?)",
    "Causes this ent to do a force push at whatever level of force push they have - will not fail":
        "Tato entita provede odstrčení Silou na své úrovni odstrčení Silou - neselže",
    "Causes this ent to do a force push anim, sound and effect, will not push anything":
        "Tato entita provede animaci, zvuk a efekt odstrčení Silou, nic neodstrčí",
    "Causes this ent to do a jedi mind trick at whatever level of mind trick they have (may not do anything for NPCs?)":
        "Tato entita provede ovládnutí mysli Jedi na své úrovni ovládnutí mysli (u NPC možná nic neudělá?)",
    "Causes this ent to grip their enemy at whatever level of grip they have (will grip until scripted to stop)":
        "Tato entita sevře svého nepřítele na své úrovni sevření (svírá, dokud to skript nezastaví)",
    "Causes this ent to lightning at whatever level of lightning they have (will lightning until scripted to stop)":
        "Tato entita metá blesky na své úrovni blesků (metá, dokud to skript nezastaví)",
    "Causes this ent to throw their saber at whatever level of saber throw they have (will throw saber until scripted to stop)":
        "Tato entita hodí svůj meč na své úrovni hodu mečem (hází, dokud to skript nezastaví)",
    "Causes this ent to go into force rage at whatever level of force rage they have":
        "Tato entita upadne do zuřivosti Síly na své úrovni zuřivosti Síly",
    "Causes this ent to start a force protect at whatever level of force protect they have":
        "Tato entita začne ochranu Silou na své úrovni ochrany Silou",
    "Causes this ent to do start a force absorb at whatever level of force absorb they have":
        "Tato entita začne pohlcování Síly na své úrovni pohlcování Síly",
    "Causes this ent to start force draining their enemy at whatever level of force drain they have (will drain until scripted to stop)":
        "Tato entita začne vysávat svého nepřítele Silou na své úrovni vysávání Síly (vysává, dokud to skript nezastaví)",
    "Set the player to wear his/her winter gear (skins torso_g1 and lower_e1), or restore the default skins.":
        "Oblékne hráče do zimní výstroje (skiny torso_g1 a lower_e1), nebo obnoví výchozí skiny.",
    "This NPC/player will not have any bone angle overrides or pitch or roll (should only be used in cinematics)":
        "Toto NPC/hráč nebude mít žádné přepsání úhlů kostí ani sklon či náklon (používat jen v animačních sekvencích)",
    "calls":
        "volání",
    "Cannot set this, only get it - valid values are 0 through 3":
        "Nelze nastavit, jen číst - platné hodnoty jsou 0 až 3",
    "Special tables":
        "Zvláštní tabulky",
    "Torso and head anim":
        "Animace trupu a hlavy",
    "Legs anim":
        "Animace nohou",
    "Set same anim on torso and legs":
        "Nastavit stejnou animaci trupu i nohou",
    "Your team":
        "Váš tým",
    "Team in which to look for enemies":
        "Tým, ve kterém hledat nepřátele",
    "Change current bState":
        "Změnit aktuální bState",
    "Change fallback bState":
        "Změnit záložní bState",
    "Set/Chang a temp bState":
        "Nastavit/změnit dočasný bState",
    "Events you can initiate":
        "Události, které můžete vyvolat",
    "Change/Stow/Drop weapon":
        "Změnit/schovat/odhodit zbraň",
    "Give items":
        "Dát předměty",
    "Set the state of the dynamic music":
        "Nastavit stav dynamické hudby",
    "Change force power level":
        "Změnit úroveň schopnosti Síly",
    "Set color of first blade of first saber":
        "Nastavit barvu první čepele prvního meče",
    "Set color of second blade of first saber":
        "Nastavit barvu druhé čepele prvního meče",
    "Cut off a part of a body and send the limb flying":
        "Useknout část těla a poslat končetinu letět",
    "Show objective on mission screen":
        "Ukázat úkol na obrazovce mise",
    "Hide objective from mission screen":
        "Skrýt úkol na obrazovce mise",
    "Mark objective as completed":
        "Označit úkol jako splněný",
    "Mark objective as completed, no update sent to screen":
        "Označit úkol jako splněný, bez aktualizace na obrazovce",
    "Mark objective as failed":
        "Označit úkol jako nesplněný",
    "Mission failed screen activates":
        "Aktivuje se obrazovka neúspěchu mise",
    "Show tactical info on mission objectives screen":
        "Ukázat taktické informace na obrazovce úkolů mise",
    "Hide tactical info on mission objectives screen":
        "Skrýt taktické informace na obrazovce úkolů mise",
    "Force all objectives to be hidden":
        "Vynutit skrytí všech úkolů",
    "Used to get whether the player has chosen the light (succeeded) or dark (failed) side.":
        "Slouží ke zjištění, zda si hráč zvolil světlou (úspěch), nebo temnou (neúspěch) stranu.",
    "Text to appear in mission status screen":
        "Text, který se objeví na obrazovce stavu mise",
    "Brings up specified menu screen":
        "Vyvolá zadanou obrazovku menu",
    "Show closing credits":
        "Ukázat závěrečné titulky",
    "Lean left, right or stop leaning":
        "Naklonit se doleva, doprava, nebo přestat",
    "Expressions: get, tag, random":
        "Výrazy: get, tag, random",
    "Instead of a fixed value, a field can hold an expression. In the editor, **{{EditorHelper}}** builds them:":
        "Místo pevné hodnoty může pole obsahovat výraz. V editoru je sestaví **{{EditorHelper}}**:",
    "**{{EditorGet}}** - `get ( FLOAT, \"name\" )` reads a variable declared with `declare`.":
        "**{{EditorGet}}** - `get ( FLOAT, \"name\" )` čte proměnnou deklarovanou pomocí `declare`.",
    "**{{EditorTag}}** - `tag ( \"cam_door\", ORIGIN )` takes the position (or `ANGLES`) of a `ref_tag` on the map. That is how cameras stand on marks.":
        "**{{EditorTag}}** - `tag ( \"cam_door\", ORIGIN )` vezme polohu (nebo `ANGLES`) `ref_tag` v mapě. Tak stojí kamery na značkách.",
    "**{{EditorRnd}}** - `random ( 1000, 3000 )` a random number between two values, for example a wait.":
        "**{{EditorRnd}}** - `random ( 1000, 3000 )` náhodné číslo mezi dvěma hodnotami, například pro čekání.",
    "Every button":
        "Každé tlačítko",
    "Each button of the program - the mode bar, the action column, the map toolbar, the placement tiles, the inspector, the camera, the timeline and the model view - with what it does.":
        "Každé tlačítko programu - lišta režimů, sloupec akcí, lišta mapy, dlaždice pro umisťování, inspektor, kamera, časová osa a zobrazení modelu - s tím, co dělá.",
    "Hover over any button in the program to see the same explanation as a tooltip. Buttons with a coloured frame are switches: on or off.":
        "Najeďte myší na libovolné tlačítko v programu a uvidíte stejné vysvětlení jako popisek. Tlačítka s barevným rámečkem jsou přepínače: zapnuto nebo vypnuto.",
    "The mode bar":
        "Lišta režimů",
    "The strip at the far left switches what the window shows. The script stays the same in every mode.":
        "Pruh úplně vlevo přepíná, co okno ukazuje. Skript zůstává v každém režimu stejný.",
    "Mode":
        "Režim",
    "What it shows":
        "Co ukazuje",
    "The **{{GroupEvents}}** list of every command and macro, the script tree, the action column - for writing scripts. [[skripte]]":
        "Seznam **{{GroupEvents}}** všech příkazů a maker, strom skriptu, sloupec akcí - pro psaní skriptů. [[skripte]]",
    "The mission: the entity list and inspector, the map in 3D with the timeline below - for cameras, figures, entities and saving the mission. [[karte]]":
        "Mise: seznam entit a inspektor, mapa ve 3D s časovou osou pod ní - pro kamery, postavy, entity a uložení mise. [[karte]]",
    "A character model with its animations - to find an animation and put it into the script. [[modell]]":
        "Model postavy s jejími animacemi - pro nalezení animace a vložení do skriptu. [[modell]]",
    "Build and edit maps like in NetRadiant-Custom: 2D views, camera, texture browser and entities, with its menus and shortcuts. [[karteneditor]]":
        "Tvorba a úprava map jako v NetRadiant-Custom: 2D pohledy, kamera, prohlížeč textur a entity, s jeho nabídkami a zkratkami. [[karteneditor]]",
    "The action column (Events mode)":
        "Sloupec akcí (režim Události)",
    "The action column, as in the original BehavEd.":
        "Sloupec akcí, jako v původním BehavEd.",
    "Button":
        "Tlačítko",
    "Removes the selected lines (<kbd>Delete</kbd>).":
        "Odebere vybrané řádky (<kbd>Delete</kbd>).",
    "Inserts a copy right below (<kbd>Space</kbd>).":
        "Vloží kopii hned pod (<kbd>Space</kbd>).",
    "Clipboard (<kbd>Ctrl</kbd>+<kbd>C</kbd> / <kbd>X</kbd> / <kbd>V</kbd>). Paste goes below the selection; text from other programs is accepted after a question.":
        "Schránka (<kbd>Ctrl</kbd>+<kbd>C</kbd> / <kbd>X</kbd> / <kbd>V</kbd>). Vložení jde pod výběr; text z jiných programů se přijme po dotazu.",
    "Turns lines into comments and back (<kbd>Backspace</kbd>).":
        "Mění řádky na komentáře a zpět (<kbd>Backspace</kbd>).",
    "The Find window (<kbd>Ctrl</kbd>+<kbd>F</kbd>): search by command or value, replace in one or all scripts. [[k-suchen-ersetzen]]":
        "Okno Najít (<kbd>Ctrl</kbd>+<kbd>F</kbd>): hledání podle příkazu nebo hodnoty, nahrazení v jednom nebo všech skriptech. [[k-suchen-ersetzen]]",
    "Moves the selection one line, inside its block (<kbd>Alt</kbd>+<kbd>Up</kbd>/<kbd>Down</kbd>).":
        "Posune výběr o řádek, v rámci jeho bloku (<kbd>Alt</kbd>+<kbd>Up</kbd>/<kbd>Down</kbd>).",
    "Undo and redo. Right-click either one for the full history. [[k-rueckgaengig]]":
        "Zpět a znovu. Pravým tlačítkem na kterékoli získáte celou historii. [[k-rueckgaengig]]",
    "An empty script / open a `.txt` or a compiled `.ibi`.":
        "Prázdný skript / otevřít `.txt` nebo zkompilované `.ibi`.",
    "Recently opened files (<kbd>Alt</kbd>+<kbd>M</kbd>).":
        "Naposledy otevřené soubory (<kbd>Alt</kbd>+<kbd>M</kbd>).",
    "Adds another script to the end of this one.":
        "Přidá jiný skript na konec tohoto.",
    "Save this script, every changed tab, or under a new name.":
        "Uložit tento skript, každou změněnou kartu, nebo pod novým názvem.",
    "Writes the script as plain text and opens it.":
        "Zapíše skript jako prostý text a otevře ho.",
    "Keeps a copy next to the script (`.bak`) / goes back to it (<kbd>Alt</kbd>+<kbd>B</kbd> / <kbd>Alt</kbd>+<kbd>R</kbd>).":
        "Uchová kopii vedle skriptu (`.bak`) / vrátí se k ní (<kbd>Alt</kbd>+<kbd>B</kbd> / <kbd>Alt</kbd>+<kbd>R</kbd>).",
    "Shows the value types in the tree.":
        "Ukazuje typy hodnot ve stromu.",
    "Shows numbers short: `1000` instead of `1000.000`.":
        "Ukazuje čísla krátce: `1000` místo `1000.000`.",
    "Shows a macro as one line.":
        "Ukazuje makro jako jeden řádek.",
    "Expands or collapses every block.":
        "Rozbalí nebo sbalí každý blok.",
    "Saves the script and writes the `.ibi` the game runs; green when it worked, red with the messages when not. [[k-uebersetzen]]":
        "Uloží skript a zapíše `.ibi`, které hra spouští; zeleně, když se to povedlo, červeně se zprávami, když ne. [[k-uebersetzen]]",
    "The map toolbar (Mission mode)":
        "Lišta mapy (režim Mise)",
    "The toolbar above the map.":
        "Lišta nad mapou.",
    "Mission":
        "Mise",
    "**{{MissionNew}}**, **{{LoadMission}}**, **{{MissionSave}}** - and the name of the open mission.":
        "**{{MissionNew}}**, **{{LoadMission}}**, **{{MissionSave}}** - a název otevřené mise.",
    "Open a map, a mission folder or an `.ent` file; **Scripts (N)** next to it lists the map's scripts.":
        "Otevřít mapu, složku mise nebo soubor `.ent`; **Skripty (N)** vedle vypíše skripty mapy.",
    "What the view shows":
        "Co pohled ukazuje",
    "Entities, names, figures, effects, glow, dynamic light, fog, sky - each a switch.":
        "Entity, názvy, postavy, efekty, záře, dynamické světlo, mlha, obloha - každé je přepínač.",
    "Looking":
        "Pohled",
    "Through the camera, top view, the NPC waypoint window.":
        "Pohled kamerou, pohled shora, okno navigačních bodů NPC.",
    "Tools":
        "Nástroje",
    "Grid, view settings, statistics.":
        "Mřížka, nastavení zobrazení, statistiky.",
    "Every button in detail:":
        "Každé tlačítko podrobně:",
    "Starts a mission on any map: player start, script runner, intro script and mission folder are created. [[tut-p1]]":
        "Začne misi na libovolné mapě: vytvoří se start hráče, spouštěč skriptu, úvodní skript a složka mise. [[tut-p1]]",
    "Opens a mission from a `.pk3`: map, entities, every script in tabs, the figures. [[missionen]]":
        "Otevře misi z `.pk3`: mapu, entity, každý skript na kartách, postavy. [[missionen]]",
    "Saves every script, writes the `.ent`, compiles and packs the `.pk3` - after checking the mission. [[tut-p8]]":
        "Uloží každý skript, zapíše `.ent`, zkompiluje a zabalí `.pk3` - po kontrole mise. [[tut-p8]]",
    "The name of the open mission; a dot means unsaved changes. Hover for its folder.":
        "Název otevřené mise; tečka znamená neuložené změny. Najetím myší uvidíte její složku.",
    "Opens a map from disk or from a `.pk3`, a mission folder, or a separate `.ent` file.":
        "Otevře mapu z disku nebo z `.pk3`, složku mise, nebo samostatný soubor `.ent`.",
    "The scripts the map's entities point to - pick one to open it.":
        "Skripty, na které ukazují entity mapy - vyberte jeden a otevřete ho.",
    "Shows the entity markers: walk targets, marks, runners, spawners.":
        "Ukazuje značky entit: cíle chůze, značky, spouštěče, spawnery.",
    "Writes the names of entities and figures into the view.":
        "Vypisuje do pohledu názvy entit a postav.",
    "Shows the script's figures, moved by the timeline.":
        "Ukazuje postavy skriptu, pohybované časovou osou.",
    "Shows the effects of the map (`fx_runner`).":
        "Ukazuje efekty mapy (`fx_runner`).",
    "The glow of shaders marked `glow`, as `r_DynamicGlow`.":
        "Záře shaderů označených `glow`, jako `r_DynamicGlow`.",
    "Lightsabers and effect lights light up walls and figures.":
        "Světelné meče a světla efektů osvětlují zdi a postavy.",
    "The fog of the map, as the game draws it.":
        "Mlha mapy, jak ji kreslí hra.",
    "The sky; switched off you can look into the map from outside.":
        "Obloha; vypnutá umožní dívat se do mapy zvenku.",
    "Shows what the camera sees - the selected camera command, or the camera at the playhead. [[k-ansichten]]":
        "Ukazuje, co vidí kamera - vybraný příkaz kamery, nebo kamera u ukazatele přehrávání. [[k-ansichten]]",
    "Looks straight down; a cut height hides ceilings. [[k-draufsicht]]":
        "Dívá se přímo dolů; výška řezu skryje stropy. [[k-draufsicht]]",
    "The NPC waypoint window: show, create and check the network. [[wegpunkte]]":
        "Okno navigačních bodů NPC: zobrazení, vytvoření a kontrola sítě. [[wegpunkte]]",
    "Snaps entities to 8 to 64 units while dragging and placing.":
        "Přichytává entity při tažení a umisťování k 8 až 64 jednotkám.",
    "Brightness, ambient light, detail, shadows, highlighting the active script's entities, and the game screen: aspect ratio, cg_fovAspectAdjust and the frame in the free view ([[kamera#vorschau]]). [[k-hell]]":
        "Jas, okolní světlo, detail, stíny, zvýraznění entit aktivního skriptu a obrazovka hry: poměr stran, cg_fovAspectAdjust a rámeček ve volném pohledu ([[kamera#vorschau]]). [[k-hell]]",
    "Frame rate, textures, triangles and graphics card values.":
        "Snímková frekvence, textury, trojúhelníky a hodnoty grafické karty.",
    "The placement tiles":
        "Dlaždice pro umisťování",
    "The strip on the left edge of the map. Click a tile, then click on the floor - or drag the tile into the view. <kbd>Shift</kbd> places several, <kbd>Esc</kbd> stops. [[k-entity-setzen]]":
        "Pruh u levého okraje mapy. Klikněte na dlaždici a pak na podlahu - nebo dlaždici přetáhněte do pohledu. <kbd>Shift</kbd> umístí několik, <kbd>Esc</kbd> skončí. [[k-entity-setzen]]",
    "A character that spawns: set `NPC_type` and a name.":
        "Postava, která se objeví: nastavte `NPC_type` a název.",
    "A place a figure walks to (`SET_NAVGOAL`).":
        "Místo, kam postava jde (`SET_NAVGOAL`).",
    "A mark with a direction - for cameras (`tag`) and look targets.":
        "Značka se směrem - pro kamery (`tag`) a cíle pohledu.",
    "A point to look at or to aim at.":
        "Bod, na který se dívat nebo mířit.",
    "Runs a script when it is used - by a trigger, the player start or another script.":
        "Spustí skript, když je použit - triggerem, startem hráče nebo jiným skriptem.",
    "Plays an effect file (`.efx`).":
        "Přehraje soubor efektu (`.efx`).",
    "Plays a sound, once or in a loop.":
        "Přehraje zvuk, jednou nebo ve smyčce.",
    "A point of the NPC navigation network.":
        "Bod navigační sítě NPC.",
    "Where the player spawns.":
        "Kde se hráč objeví.",
    "The entity inspector":
        "Inspektor entit",
    "The inspector under the entity list.":
        "Inspektor pod seznamem entit.",
    "Puts it on the floor below, like `DROPTOFLOOR` in the game.":
        "Položí ji na podlahu pod ní, jako `DROPTOFLOOR` ve hře.",
    "A copy - of every selected entity. <kbd>Alt</kbd>+drag in the view does the same.":
        "Kopie - každé vybrané entity. <kbd>Alt</kbd>+tažení v pohledu udělá totéž.",
    "Removes it.":
        "Odebere ji.",
    "Opens the Animation window for doors, platforms, ships and effects. [[animation]]":
        "Otevře okno Animace pro dveře, plošiny, lodě a efekty. [[animation]]",
    "Undoes the last entity change (entities have their own history).":
        "Vrátí poslední změnu entity (entity mají vlastní historii).",
    "Redoes it.":
        "Provede ji znovu.",
    "Adds a key.":
        "Přidá klíč.",
    "Next to a script key: opens the script - and creates it in the mission folder if it does not exist yet.":
        "Vedle skriptového klíče: otevře skript - a vytvoří ho ve složce mise, pokud ještě neexistuje.",
    "The camera bar":
        "Lišta kamery",
    "Appears in the view when the camera is selected. [[kamera]]":
        "Objeví se v pohledu, když je vybraná kamera. [[kamera]]",
    "The camera bar above the selected shot.":
        "Lišta kamery nad vybraným záběrem.",
    "Arrows: move the shot along one axis, the box in the middle in the view plane.":
        "Šipky: posunou záběr podél jedné osy, rámeček uprostřed v rovině pohledu.",
    "Rings: turn the shot.":
        "Kruhy: otočí záběr.",
    "World or Local: the arrows follow the map axes or the camera.":
        "Svět nebo Lokální: šipky sledují osy mapy, nebo kameru.",
    "The previous shot.":
        "Předchozí záběr.",
    "The next shot.":
        "Další záběr.",
    "Puts the shot where you are looking.":
        "Umístí záběr tam, kam se díváte.",
    "Sit in the shot and fly it; <kbd>Enter</kbd> sets a key. [[k-pilot]]":
        "Usaďte se v záběru a proleťte ho; <kbd>Enter</kbd> nastaví klíč. [[k-pilot]]",
    "Look through the camera.":
        "Podívat se kamerou.",
    "A small picture of the camera's view in the corner.":
        "Malý obrázek pohledu kamery v rohu.",
    "A new shot at your view, after the selected one.":
        "Nový záběr ve vašem pohledu, za vybraný.",
    "Removes the shot.":
        "Odebere záběr.",
    "The timeline":
        "Časová osa",
    "The transport bar of the timeline.":
        "Lišta přehrávání časové osy.",
    "Back to the start, play / stop - the time display next to it switches between seconds and frames.":
        "Zpět na začátek, přehrát / zastavit - zobrazení času vedle přepíná mezi sekundami a snímky.",
    "Playback":
        "Přehrávání",
    "Loop, sound, follow the camera - and the speed list.":
        "Smyčka, zvuk, sledování kamery - a seznam rychlostí.",
    "Shows more or less time.":
        "Ukazuje víc nebo méně času.",
    "Rows":
        "Řádky",
    "One row per figure, quiet rows, waits as blocks.":
        "Jeden řádek na postavu, tiché řádky, čekání jako bloky.",
    "Inserts a camera shot at the playhead.":
        "Vloží záběr kamery u ukazatele přehrávání.",
    "Back to the start of the script.":
        "Zpět na začátek skriptu.",
    "Plays the scene - camera, figures, sound - in game time.":
        "Přehraje scénu - kameru, postavy, zvuk - v herním čase.",
    "Starts again at the end.":
        "Na konci začne znovu.",
    "Plays the sounds while the timeline runs - heard from the camera the view shows, like in the game. [[zeitleiste#ton]]":
        "Přehrává zvuky, zatímco běží časová osa - slyšené z kamery, kterou pohled ukazuje, jako ve hře. [[zeitleiste#ton]]",
    "Moves the view along with the camera.":
        "Posouvá pohled spolu s kamerou.",
    "Shows more time.":
        "Ukazuje víc času.",
    "Shows less time, in more detail.":
        "Ukazuje méně času, podrobněji.",
    "The time display switches between seconds and frames when you click it; the list next to the play button sets the speed. Drag blocks to move them in time - [[zeitleiste]].":
        "Zobrazení času po kliknutí přepíná mezi sekundami a snímky; seznam vedle tlačítka přehrávání nastavuje rychlost. Tažením bloků je posunete v čase - [[zeitleiste]].",
    "The model toolbar (Model mode)":
        "Lišta modelu (režim Model)",
    "The toolbar above the model.":
        "Lišta nad modelem.",
    "A `.glm` from disk, a model from the archives, or a character of the game (**{{NpcBrowser}}**).":
        "`.glm` z disku, model z archivů, nebo postava hry (**{{NpcBrowser}}**).",
    "Skin and skeleton":
        "Skin a kostra",
    "Another `.skin`, another `.gla`.":
        "Jiný `.skin`, jiný `.gla`.",
    "Turntable, bones, caps, textures.":
        "Otočný stůl, kosti, záslepky, textury.",
    "The whole model in view, from the front.":
        "Celý model v pohledu, zepředu.",
    "Opens a `.glm` model from disk.":
        "Otevře model `.glm` z disku.",
    "Opens a model from the game's archives.":
        "Otevře model z archivů hry.",
    "Every character of the game - click one to load it as the game shows it.":
        "Každá postava hry - kliknutím ji načtete tak, jak ji ukazuje hra.",
    "Loads a `.skin` for the model.":
        "Načte `.skin` pro model.",
    "Loads another skeleton / animation file (`.gla`).":
        "Načte jiný soubor kostry / animací (`.gla`).",
    "Turns the model slowly around itself.":
        "Pomalu otáčí modelem kolem sebe.",
    "Shows the skeleton.":
        "Ukazuje kostru.",
    "Shows the caps of the surfaces where limbs attach.":
        "Ukazuje záslepky povrchů, kde se připojují končetiny.",
    "Textures on or off.":
        "Textury zap. nebo vyp.",
    "In the animation list: copies the animation name.":
        "V seznamu animací: zkopíruje název animace.",
    "Puts `set ( SET_ANIM_BOTH, ... )` for the animation into the script. [[k-animation-abspielen]]":
        "Vloží do skriptu `set ( SET_ANIM_BOTH, ... )` pro animaci. [[k-animation-abspielen]]",
    "Everything inside an `affect` block is done by the entity named in it - Kyle walks, the door moves, the ship plays its ROFF. The script itself does **not** wait for the block: two `affect` blocks one after the other run at the same time. With `FLUSH` the entity first drops what it was doing; with `INSERT` the new commands are added to what it already does.":
        "Vše uvnitř bloku `affect` dělá entita v něm jmenovaná - Kyle jde, dveře se pohybují, loď přehrává svůj ROFF. Samotný skript na blok **nečeká**: dva bloky `affect` za sebou běží současně. S `FLUSH` entita nejdřív zahodí, co dělala; s `INSERT` se nové příkazy přidají k tomu, co už dělá.",
    "Where to enter what":
        "Kam co zadat",
    "The event editor for `%s` - the numbers match the table below.":
        "Editor událostí pro `%s` - čísla odpovídají tabulce níže.",
    "Name":
        "Název",
    "The name of the entity - `NPC_targetname` of a figure, `targetname` or `script_targetname` of anything else. With a map loaded, the arrow lists every name of the map.":
        "Název entity - `NPC_targetname` postavy, `targetname` nebo `script_targetname` čehokoli jiného. S načtenou mapou šipka vypíše každý název mapy.",
    "Type":
        "Typ",
    "`FLUSH` - forget earlier commands and do these. `INSERT` - add these to what the entity is already doing.":
        "`FLUSH` - zapomenout dřívější příkazy a dělat tyto. `INSERT` - přidat tyto k tomu, co entita už dělá.",
    "`affect ( \"kyle\", FLUSH )` with `set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, \"nav_kyle\" )` inside - Kyle walks to the walk target `nav_kyle`.":
        "`affect ( \"kyle\", FLUSH )` s `set ( SET_WALKING, true )` a `set ( SET_NAVGOAL, \"nav_kyle\" )` uvnitř - Kyle jde k cíli chůze `nav_kyle`.",
    "In Classic, Graph and Tracks":
        "V Klasickém, Grafu a Stopách",
    "The switch at the top of the page chooses the view. All three work on the same script - switching loses nothing.":
        "Přepínač nahoře na stránce volí pohled. Všechny tři pracují na tomtéž skriptu - přepnutím se nic neztratí.",
    "**In Classic** it is a line with a box in front: click the box to open or close the block. Commands inside are indented. A command inserted while the block is open and selected goes inside it.":
        "**V Klasickém** je to řádek s rámečkem vpředu: kliknutím na rámeček blok otevřete nebo zavřete. Příkazy uvnitř jsou odsazené. Příkaz vložený, když je blok otevřený a vybraný, jde dovnitř.",
    "`%s` in the script tree.":
        "`%s` ve stromu skriptu.",
    "**In Graph** the block and its contents are listed on the right, the contents indented. Select the block line and double-click a command in the **{{GroupEvents}}** list to put it inside.":
        "**V Grafu** jsou blok a jeho obsah vypsány vpravo, obsah odsazený. Vyberte řádek bloku a dvakrát klikněte na příkaz v seznamu **{{GroupEvents}}**, aby šel dovnitř.",
    "`%s` in the list of the selected script node.":
        "`%s` v seznamu vybraného uzlu skriptu.",
    "**In Tracks** an `affect` block has no block of its own: it gives its figure a **row**, and the commands inside it are the blocks on that row. The `affect` line itself is in the **{{TracksList}}** on the right.":
        "**Ve Stopách** nemá blok `affect` vlastní blok: dává své postavě **řádek** a příkazy uvnitř jsou bloky na tomto řádku. Samotný řádek `affect` je v **{{TracksList}}** vpravo.",
    "`%s` in Tracks.":
        "`%s` ve Stopách.",
    "A name that is not on the map does nothing in the game - check the arrow list.":
        "Název, který v mapě není, ve hře nic nedělá - zkontrolujte seznam pod šipkou.",
    "Want the script to wait until a figure is done? Put the action into a `task` and use `dowait` - see [[k-tasks]].":
        "Chcete, aby skript počkal, až postava skončí? Dejte akci do `task` a použijte `dowait` - viz [[k-tasks]].",
    "affect - commands for one entity":
        "affect - příkazy pro jednu entitu",
    "Sends the commands of its block to one entity: a figure, a door, a ship.":
        "Pošle příkazy svého bloku jedné entitě: postavě, dveřím, lodi.",
    "Inside an `affect` block, `flush` throws away all commands still waiting for this entity - from this script or an earlier one. It is what `affect ( ..., FLUSH )` does at its start; on its own you use it in the middle of a block to start over.":
        "Uvnitř bloku `affect` zahodí `flush` všechny příkazy, které na tuto entitu ještě čekají - z tohoto skriptu nebo dřívějšího. Dělá to `affect ( ..., FLUSH )` na svém začátku; samostatně ho použijete uprostřed bloku, abyste začali znovu.",
    "This command has no values - the editor only shows its help text. Press **{{EditorOk}}**.":
        "Tento příkaz nemá hodnoty - editor ukazuje jen jeho text nápovědy. Stiskněte **{{EditorOk}}**.",
    "`flush ( )` at the top of Kyle's block, then the new walk.":
        "`flush ( )` nahoře v Kylově bloku, pak nová chůze.",
    "**In Classic** it is one line of the script tree. Double-click it (or select it and press <kbd>Enter</kbd>) to open the editor; right-click for the menu; drag it to move it.":
        "**V Klasickém** je to jeden řádek stromu skriptu. Dvakrát na něj klikněte (nebo ho vyberte a stiskněte <kbd>Enter</kbd>) a otevře se editor; pravým tlačítkem nabídka; tažením ho přesunete.",
    "**In Graph** the script is a node; select it and its commands are listed on the right under **{{GraphCommands}}**. Double-click the line to edit it, right-click for the menu, drop a command from the **{{GroupEvents}}** list on it to insert after it.":
        "**V Grafu** je skript uzel; vyberte ho a jeho příkazy se vypíší vpravo pod **{{GraphCommands}}**. Dvojklikem na řádek ho upravíte, pravým tlačítkem nabídka, puštěním příkazu ze seznamu **{{GroupEvents}}** na něj ho vložíte za něj.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph.":
        "**Ve Stopách** nezabere vlastní čas, takže nemá blok - stojí v **{{TracksList}}** vpravo. Kliknutí, dvojklik a pravé tlačítko tam fungují jako v Grafu.",
    "`flush` does not stop a figure that is already walking - give it a new goal or `SET_NAVGOAL` with an empty value.":
        "`flush` nezastaví postavu, která už jde - dejte jí nový cíl nebo `SET_NAVGOAL` s prázdnou hodnotou.",
    "flush - forget waiting commands":
        "flush - zapomenout čekající příkazy",
    "Clears every command the entity has not done yet.":
        "Smaže každý příkaz, který entita ještě neudělala.",
    "`if` compares two values. If the comparison holds, the commands in its block run; otherwise they are skipped (and an `else` right after it runs instead). Usually one side is a variable read with `get` - for example *has the player seen this scene before?*":
        "`if` porovná dvě hodnoty. Platí-li porovnání, příkazy v jeho bloku se spustí; jinak se přeskočí (a místo toho se spustí `else` hned za ním). Obvykle je jedna strana proměnná čtená pomocí `get` - například *viděl už hráč tuto scénu?*",
    "First value":
        "První hodnota",
    "A number, a text or an expression. **{{EditorHelper}}** → **{{EditorGet}}** reads a variable: `get ( FLOAT, \"seen\" )`.":
        "Číslo, text nebo výraz. **{{EditorHelper}}** → **{{EditorGet}}** čte proměnnou: `get ( FLOAT, \"seen\" )`.",
    "Comparison":
        "Porovnání",
    "`=` equal, `<` less, `>` greater, `!` not equal.":
        "`=` rovná se, `<` menší, `>` větší, `!` nerovná se.",
    "Second value":
        "Druhá hodnota",
    "What the first value is compared with - here `0`.":
        "S čím se první hodnota porovná - zde `0`.",
    "`if ( get( FLOAT, \"seen\" ), =, 0 )` - true the first time, when `seen` is still 0.":
        "`if ( get( FLOAT, \"seen\" ), =, 0 )` - pravdivé napoprvé, kdy je `seen` ještě 0.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The timeline plays the commands inside as if the condition were true - it cannot know the value in the game.":
        "**Ve Stopách** nezabere vlastní čas, takže nemá blok - stojí v **{{TracksList}}** vpravo. Kliknutí, dvojklik a pravé tlačítko tam fungují jako v Grafu. Časová osa přehraje příkazy uvnitř, jako by podmínka byla pravdivá - nemůže znát hodnotu ve hře.",
    "Declare the variable first with `declare` - see [[cmd-declare]] and [[k-variablen]].":
        "Nejdřív proměnnou deklarujte pomocí `declare` - viz [[cmd-declare]] a [[k-variablen]].",
    "ICARUS only knows `=`, `<`, `>` and `!` - an unknown comparison stops the compile.":
        "ICARUS zná jen `=`, `<`, `>` a `!` - neznámé porovnání zastaví kompilaci.",
    "if - only when a condition holds":
        "if - jen když platí podmínka",
    "Runs its block only if a comparison is true.":
        "Spustí svůj blok, jen když je porovnání pravdivé.",
    "`else` belongs to the `if` directly in front of it. Its block runs when that `if` did not. There must be nothing between the end of the `if` block and the `else`.":
        "`else` patří k `if` přímo před ním. Jeho blok se spustí, když se ten `if` nespustil. Mezi koncem bloku `if` a `else` nesmí nic být.",
    "After `if ( get( FLOAT, \"seen\" ), =, 0 ) { ... }` an `else ( ) { print ( \"Welcome back\" ) }` greets a returning player.":
        "Po `if ( get( FLOAT, \"seen\" ), =, 0 ) { ... }` pozdraví `else ( ) { print ( \"Welcome back\" ) }` vracejícího se hráče.",
    "An `else` without an `if` right before it is an error when you compile.":
        "`else` bez `if` hned před ním je při kompilaci chyba.",
    "else - otherwise":
        "else - jinak",
    "Runs its block when the `if` right before it was false.":
        "Spustí svůj blok, když byl `if` hned před ním nepravdivý.",
    "Everything inside a `loop` block runs again and again - three times with `3`, forever with `-1`. Use it for a guard's patrol, a blinking light or a repeated shake. Inside a forever loop, put a `wait` - otherwise the game freezes.":
        "Vše uvnitř bloku `loop` běží znovu a znovu - třikrát s `3`, navždy s `-1`. Použijte to pro pochůzku strážného, blikající světlo nebo opakované otřesy. Do nekonečné smyčky dejte `wait` - jinak hra zamrzne.",
    "Count":
        "Počet",
    "How often the block runs. `-1` means forever.":
        "Kolikrát blok proběhne. `-1` znamená navždy.",
    "`loop ( 3 )` with a camera shake and `wait ( 1000 )` inside - three shakes, a second apart.":
        "`loop ( 3 )` s otřesem kamery a `wait ( 1000 )` uvnitř - tři otřesy, sekundu od sebe.",
    "**In Tracks** the commands inside the loop are blocks, repeated as often as the loop runs (a forever loop is shown a few times). The `loop` line itself is in the **{{TracksList}}**.":
        "**Ve Stopách** jsou příkazy uvnitř smyčky bloky, opakované tolikrát, kolikrát smyčka proběhne (nekonečná smyčka se ukáže několikrát). Samotný řádek `loop` je v **{{TracksList}}**.",
    "A forever loop in the intro never ends - the script never gets past it. Put it in an `affect` block of its own or in a separate script.":
        "Nekonečná smyčka v úvodu nikdy neskončí - skript se přes ni nikdy nedostane. Dejte ji do vlastního bloku `affect` nebo do samostatného skriptu.",
    "loop - repeat a block":
        "loop - opakovat blok",
    "Repeats the commands of its block a number of times, or forever.":
        "Opakuje příkazy svého bloku několikrát, nebo navždy.",
    "`run` starts the named script right away, and this script goes on at once. The script is named like the file below `scripts/`, without `.txt` or `.ibi`.":
        "`run` hned spustí jmenovaný skript a tento skript okamžitě pokračuje. Skript se jmenuje jako soubor pod `scripts/`, bez `.txt` nebo `.ibi`.",
    "The path below `scripts/` - for example `taspir_tut/kyle_spawn`. **{{EditorBrowse}}** picks a file of the game, **{{EditorPlay}}** opens it.":
        "Cesta pod `scripts/` - například `taspir_tut/kyle_spawn`. **{{EditorBrowse}}** vybere soubor hry, **{{EditorPlay}}** ho otevře.",
    "`run ( \"taspir_tut/kyle_spawn\" )` starts Kyle's spawn script.":
        "`run ( \"taspir_tut/kyle_spawn\" )` spustí Kylův spawn skript.",
    "To wait until the other script reaches a point, let it `signal` and use `waitsignal` here - see [[k-signale]].":
        "Chcete-li počkat, až druhý skript dosáhne určitého místa, nechte ho poslat `signal` a zde použijte `waitsignal` - viz [[k-signale]].",
    "run - start another script":
        "run - spustit jiný skript",
    "The script stops here for the given time; then it goes on with the next command. 1000 is one second. Most of a cutscene's timing is waits: the camera starts a move, the script waits until the move is done, then the next shot.":
        "Skript se tu na zadanou dobu zastaví; pak pokračuje dalším příkazem. 1000 je jedna sekunda. Většina načasování animační sekvence jsou čekání: kamera začne pohyb, skript počká, až je pohyb hotový, pak další záběr.",
    "Time":
        "Čas",
    "Milliseconds: `1000` = one second. **{{EditorHelper}}** → **{{EditorRnd}}** builds `random ( 1000, 3000 )` for a different time each run.":
        "Milisekundy: `1000` = jedna sekunda. **{{EditorHelper}}** → **{{EditorRnd}}** sestaví `random ( 1000, 3000 )` pro jiný čas při každém běhu.",
    "`wait ( 2000 )` - two seconds, then the camera is switched off.":
        "`wait ( 2000 )` - dvě sekundy, pak se kamera vypne.",
    "**In Tracks** it is a block on the timeline, as long as it takes. Click it to select its line, double-click to edit it, drag its right edge to change the time, right-click for the menu. Waits are on the **Script** row.":
        "**Ve Stopách** je to blok na časové ose, tak dlouhý, jak trvá. Kliknutím vyberete jeho řádek, dvojklikem ho upravíte, tažením pravého okraje změníte čas, pravým tlačítkem nabídka. Čekání jsou na řádku **Skript**.",
    "Inside an `affect` block, a `wait` delays only that entity - the script goes on.":
        "Uvnitř bloku `affect` zdrží `wait` jen tuto entitu - skript pokračuje.",
    "To wait until a figure has arrived, a fixed time is a guess - use a `task` and `dowait` ([[cmd-dowait]]).":
        "Čekat, až postava dorazí, pevným časem je odhad - použijte `task` a `dowait` ([[cmd-dowait]]).",
    "wait - pause for a time":
        "wait - čekat určitou dobu",
    "Pauses the script for a number of milliseconds.":
        "Zastaví skript na daný počet milisekund.",
    "With a task name instead of a time, `wait` pauses until that task is done - for a walk: until the figure has arrived. Together with `do` it lets a figure do two things at once and still wait for one of them.":
        "S názvem úlohy místo času čeká `wait`, dokud úloha neskončí - u chůze: dokud postava nedorazí. Spolu s `do` to umožní postavě dělat dvě věci zároveň a přitom počkat na jednu z nich.",
    "The event editor for `wait` with a task name.":
        "Editor událostí pro `wait` s názvem úlohy.",
    "Task":
        "Úloha",
    "The name of a `task` of this entity that was started with `do`.":
        "Název `task` této entity, která byla spuštěna pomocí `do`.",
    "`do ( \"walk\" )`, then something else, then `wait ( \"walk\" )` - the script goes on when Kyle has arrived.":
        "`do ( \"walk\" )`, pak něco jiného, pak `wait ( \"walk\" )` - skript pokračuje, když Kyle dorazil.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}** on the right. You see what it does on the figure's row: what follows it starts where the task ends.":
        "**Ve Stopách** nemá vlastní blok - je v **{{TracksList}}** vpravo. Co dělá, vidíte na řádku postavy: co po něm následuje, začne tam, kde úloha končí.",
    "`dowait` is the short form of `do` + `wait` - [[cmd-dowait]].":
        "`dowait` je zkrácená forma `do` + `wait` - [[cmd-dowait]].",
    "wait - until a task is done":
        "wait - dokud úloha neskončí",
    "Pauses until a task started with `do` is finished.":
        "Čeká, dokud neskončí úloha spuštěná pomocí `do`.",
    "Two scripts can wait for each other: this one stops at `waitsignal ( \"door_open\" )` until another script reaches `signal ( \"door_open\" )`. Only one script can wait for a given name at a time.":
        "Dva skripty mohou čekat na sebe: tento se zastaví u `waitsignal ( \"door_open\" )`, dokud jiný skript nedosáhne `signal ( \"door_open\" )`. Na daný název může najednou čekat jen jeden skript.",
    "Signal":
        "Signál",
    "Any name - it must be spelled exactly like the one in `signal`.":
        "Libovolný název - musí být napsán přesně jako ten v `signal`.",
    "The intro waits with `waitsignal ( \"door_open\" )` until the door script says `signal ( \"door_open\" )`.":
        "Úvod čeká s `waitsignal ( \"door_open\" )`, dokud skript dveří neřekne `signal ( \"door_open\" )`.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The timeline cannot know how long it waits in the game - everything after it follows directly.":
        "**Ve Stopách** nezabere vlastní čas, takže nemá blok - stojí v **{{TracksList}}** vpravo. Kliknutí, dvojklik a pravé tlačítko tam fungují jako v Grafu. Časová osa nemůže vědět, jak dlouho ve hře čeká - vše po něm následuje přímo.",
    "See [[k-signale]] for both sides together.":
        "Obě strany dohromady viz [[k-signale]].",
    "waitsignal - wait for another script":
        "waitsignal - čekat na jiný skript",
    "Pauses until some other script calls `signal` with the same name.":
        "Čeká, dokud jiný skript nezavolá `signal` se stejným názvem.",
    "`signal` is the other half of `waitsignal`: when this line runs, the script waiting for the same name continues.":
        "`signal` je druhá polovina `waitsignal`: když tento řádek proběhne, skript čekající na stejný název pokračuje.",
    "The same name as in `waitsignal`.":
        "Stejný název jako ve `waitsignal`.",
    "The door script: `use ( \"bldg2_ext_door\" )`, `wait ( 1500 )`, `signal ( \"door_open\" )`.":
        "Skript dveří: `use ( \"bldg2_ext_door\" )`, `wait ( 1500 )`, `signal ( \"door_open\" )`.",
    "**In Tracks** it takes no time: it is a short mark on the **Script** row at the moment it runs. Click it to select its line, double-click to edit it, right-click for the menu. It is also in the **{{TracksList}}** on the right.":
        "**Ve Stopách** nezabere čas: je to krátká značka na řádku **Skript** v okamžiku, kdy proběhne. Kliknutím vyberete jeho řádek, dvojklikem ho upravíte, pravým tlačítkem nabídka. Je také v **{{TracksList}}** vpravo.",
    "A signal that nobody waits for is lost - the waiting script must already be at its `waitsignal`.":
        "Signál, na který nikdo nečeká, se ztratí - čekající skript už musí být u svého `waitsignal`.",
    "signal - wake a waiting script":
        "signal - probudit čekající skript",
    "Lets the script that waits for this name go on.":
        "Nechá skript, který na tento název čeká, pokračovat.",
    "Inside a figure's `affect` block, `sound` makes that figure speak or make a noise. On the channel `CHAN_VOICE` the mouth moves with the line, and the subtitle of the game is shown.":
        "Uvnitř bloku `affect` postavy nechá `sound` tuto postavu mluvit nebo vydat zvuk. Na kanálu `CHAN_VOICE` se ústa hýbou s replikou a zobrazí se titulky hry.",
    "Channel":
        "Kanál",
    "`CHAN_VOICE` for dialogue (lips move), `CHAN_AUTO` or `CHAN_BODY` for other sounds.":
        "`CHAN_VOICE` pro dialog (rty se hýbou), `CHAN_AUTO` nebo `CHAN_BODY` pro jiné zvuky.",
    "File":
        "Soubor",
    "The sound below `sound/` - **{{EditorBrowse}}** picks one from the game, **{{EditorPlay}}** plays it.":
        "Zvuk pod `sound/` - **{{EditorBrowse}}** vybere jeden ze hry, **{{EditorPlay}}** ho přehraje.",
    "`sound ( CHAN_VOICE, \"sound/chars/kyle/01kyk001.mp3\" )` in Kyle's block.":
        "`sound ( CHAN_VOICE, \"sound/chars/kyle/01kyk001.mp3\" )` v Kylově bloku.",
    "**In Tracks** a `CHAN_VOICE` sound is a block on the **Dialogue** row, as long as the file plays. Double-click it to edit it.":
        "**Ve Stopách** je zvuk `CHAN_VOICE` blok na řádku **Dialog**, tak dlouhý, jak soubor hraje. Dvojklikem ho upravíte.",
    "Dialogue: [[k-dialog]].":
        "Dialog: [[k-dialog]].",
    "sound - play a sound or a line":
        "sound - přehrát zvuk nebo repliku",
    "Plays a sound on a channel of the entity.":
        "Přehraje zvuk na kanálu entity.",
    "`move` slides the entity of the current `affect` block to a new position - and, if given, to new angles - over the given time. The **Animate** window writes these for you from keys you set in the view.":
        "`move` posune entitu aktuálního bloku `affect` na novou polohu - a je-li zadáno, na nové úhly - za zadanou dobu. Okno **Animace** je píše za vás z klíčů, které nastavíte v pohledu.",
    "Position":
        "Poloha",
    "Where it goes: three numbers `x y z`, relative to where the entity was built in the map.":
        "Kam jde: tři čísla `x y z`, relativně k místu, kde byla entita postavena v mapě.",
    "Angles":
        "Úhly",
    "Pitch, yaw and roll at the end - `0 0 0` keeps them.":
        "Sklon, otočení a náklon na konci - `0 0 0` je ponechá.",
    "Milliseconds for the whole move.":
        "Milisekundy pro celý pohyb.",
    "`move ( < 0 0 128 >, < 0 0 0 >, 2000 )` in the door's block - the door rises 128 units in two seconds.":
        "`move ( < 0 0 128 >, < 0 0 0 >, 2000 )` v bloku dveří - dveře se zvednou o 128 jednotek za dvě sekundy.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The view plays the movement, but a door or a platform has no row of its own in Tracks.":
        "**Ve Stopách** nezabere vlastní čas, takže nemá blok - stojí v **{{TracksList}}** vpravo. Kliknutí, dvojklik a pravé tlačítko tam fungují jako v Grafu. Pohled pohyb přehraje, ale dveře ani plošina nemají ve Stopách vlastní řádek.",
    "A door that opens: [[k-tuer]].":
        "Dveře, které se otevírají: [[k-tuer]].",
    "move - move an entity":
        "move - posunout entitu",
    "Moves a door, a platform or a ship to a position in so many milliseconds.":
        "Posune dveře, plošinu nebo loď na polohu za tolik milisekund.",
    "`rotate` turns the entity of the current `affect` block to the given angles (pitch, yaw, roll) over the given time.":
        "`rotate` otočí entitu aktuálního bloku `affect` na zadané úhly (sklon, otočení, náklon) za zadanou dobu.",
    "Pitch, yaw and roll - `0 90 0` turns it a quarter round.":
        "Sklon, otočení a náklon - `0 90 0` ji otočí o čtvrt otáčky.",
    "Milliseconds.":
        "Milisekundy.",
    "`rotate ( < 0 90 0 >, 1000 )` in the door's block.":
        "`rotate ( < 0 90 0 >, 1000 )` v bloku dveří.",
    "rotate - turn an entity":
        "rotate - otočit entitu",
    "Turns the entity to angles in so many milliseconds.":
        "Otočí entitu na úhly za tolik milisekund.",
    "`use` triggers the entity: a door opens, a trigger fires, a spawner spawns its figure, a script runner starts its script.":
        "`use` spustí entitu: dveře se otevřou, trigger se aktivuje, spawner spawne svou postavu, spouštěč skriptu spustí svůj skript.",
    "The `targetname` of the entity - the arrow lists the names of the map.":
        "`targetname` entity - šipka vypíše názvy mapy.",
    "`use ( \"bldg2_ext_door\" )` opens the door of the building.":
        "`use ( \"bldg2_ext_door\" )` otevře dveře budovy.",
    "The second `use` in the list takes an expression: `use ( get( STRING, \"door\" ) )` uses whatever name the variable holds.":
        "Druhé `use` v seznamu bere výraz: `use ( get( STRING, \"door\" ) )` použije jakýkoli název, který proměnná obsahuje.",
    "use - use an entity":
        "use - použít entitu",
    "Uses an entity by name, as if the player pressed it.":
        "Použije entitu podle názvu, jako by ji stiskl hráč.",
    "A figure dies (and its `deathscript` runs), a breakable object breaks.":
        "Postava zemře (a spustí se její `deathscript`), rozbitný objekt se rozbije.",
    "The name of the entity.":
        "Název entity.",
    "To make a figure just disappear, use `remove`.":
        "Chcete-li postavu prostě nechat zmizet, použijte `remove`.",
    "kill - kill an entity":
        "kill - zabít entitu",
    "Use it to clean up after a cutscene: figures that should not be there when the player gets control.":
        "Použijte to k úklidu po animační sekvenci: postavy, které tam nemají být, když hráč dostane ovládání.",
    "remove - take an entity out of the game":
        "remove - vyjmout entitu ze hry",
    "Removes the entity with this name - no death, it is simply gone.":
        "Odebere entitu s tímto názvem - žádná smrt, prostě zmizí.",
    "A line of text for the player - a time jump, a hint, a place name.":
        "Řádek textu pro hráče - časový skok, nápověda, název místa.",
    "Text":
        "Text",
    "The text to show.":
        "Text k zobrazení.",
    "Text in the game's string files (`@@`) is translated by the game.":
        "Text v souborech řetězců hry (`@@`) hra přeloží.",
    "print - text on the screen":
        "print - text na obrazovce",
    "`rem` keeps a note for you or the next person: what this part of the script does. **{{ActRem}}** turns any line into a comment and back.":
        "`rem` uchová poznámku pro vás nebo dalšího člověka: co tato část skriptu dělá. **{{ActRem}}** změní kterýkoli řádek na komentář a zpět.",
    "Comment":
        "Komentář",
    "Any text.":
        "Libovolný text.",
    "rem - a comment":
        "rem - komentář",
    "A note in the script - it does nothing in the game.":
        "Poznámka ve skriptu - ve hře nic nedělá.",
    "A variable remembers something during the level - for example whether a scene was already seen. Declare it once, in a script that runs once. It starts at 0 or empty. A map can hold 16 variables.":
        "Proměnná si během úrovně něco pamatuje - například zda už byla scéna viděna. Deklarujte ji jednou, ve skriptu, který běží jednou. Začíná na 0 nebo prázdná. Mapa pojme 16 proměnných.",
    "`FLOAT` for numbers, `STRING` for text, `VECTOR` for three numbers.":
        "`FLOAT` pro čísla, `STRING` pro text, `VECTOR` pro tři čísla.",
    "Any name - you use it again in `set`, `get` and `free`.":
        "Libovolný název - znovu ho použijete v `set`, `get` a `free`.",
    "See [[k-variablen]] for declare, if, set and free together.":
        "declare, if, set a free dohromady viz [[k-variablen]].",
    "declare - create a variable":
        "declare - vytvořit proměnnou",
    "Declares a variable that scripts can set and read.":
        "Deklaruje proměnnou, kterou mohou skripty nastavovat a číst.",
    "Only 16 variables fit; `free` gives one back when it is no longer needed.":
        "Vejde se jen 16 proměnných; `free` jednu vrátí, když už není potřeba.",
    "The name used in `declare`.":
        "Název použitý v `declare`.",
    "free - give a variable back":
        "free - vrátit proměnnou",
    "Frees a declared variable so the name and its place can be used again.":
        "Uvolní deklarovanou proměnnou, aby šlo název a její místo použít znovu.",
    "`set` with a `SET_` field is the most used command of all. Inside an `affect` block it changes that entity: `SET_NAVGOAL` makes a figure walk to a walk target, `SET_WALKING` makes it walk instead of run, `SET_LOOK_TARGET` turns its head, `SET_ANIM_BOTH` plays an animation. Every field is in [[befehle#set]].":
        "`set` s polem `SET_` je ze všech nejpoužívanější příkaz. Uvnitř bloku `affect` mění tuto entitu: `SET_NAVGOAL` nechá postavu jít k cíli chůze, `SET_WALKING` ji nechá jít místo běhu, `SET_LOOK_TARGET` otočí její hlavu, `SET_ANIM_BOTH` přehraje animaci. Každé pole je v [[befehle#set]].",
    "Which value - a `SET_` name from the list. Type part of it to filter.":
        "Která hodnota - název `SET_` ze seznamu. Napište jeho část a seznam se zúží.",
    "The new value. Its kind depends on the field: `true` / `false`, a number, a name from the map, an animation.":
        "Nová hodnota. Její druh závisí na poli: `true` / `false`, číslo, název z mapy, animace.",
    "`set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, \"nav_kyle\" )` - Kyle walks to `nav_kyle`.":
        "`set ( SET_WALKING, true )` a `set ( SET_NAVGOAL, \"nav_kyle\" )` - Kyle jde k `nav_kyle`.",
    "**In Tracks** it shows in the row of the figure it belongs to (the name in its `affect`), as long as it takes. Double-click the block to edit it, right-click for the menu. A walk (`SET_NAVGOAL`) lasts until the figure arrives; most other fields take no time.":
        "**Ve Stopách** se ukazuje v řádku postavy, ke které patří (název v jejím `affect`), tak dlouho, jak trvá. Dvojklikem na blok ho upravíte, pravým tlačítkem nabídka. Chůze (`SET_NAVGOAL`) trvá, dokud postava nedorazí; většina ostatních polí nezabere čas.",
    "A field marked `*` in its help text (like `SET_NAVGOAL`) can be waited for with `dowait`.":
        "Na pole označené v textu nápovědy `*` (jako `SET_NAVGOAL`) lze čekat pomocí `dowait`.",
    "A value that is not in the list is allowed (mods bring their own) but marked **unknown** - check it for a typo.":
        "Hodnota, která v seznamu není, je povolena (mody přinášejí vlastní), ale označena jako **neznámá** - zkontrolujte překlep.",
    "set - change a field of an entity":
        "set - změnit pole entity",
    "Changes a value of the entity: how it walks, where it goes, what it looks at, its animation, its health ...":
        "Mění hodnotu entity: jak chodí, kam jde, na co se dívá, její animaci, její zdraví ...",
    "The second `set` in the list sets a variable by its name instead of an entity field.":
        "Druhé `set` v seznamu nastavuje proměnnou podle jejího názvu místo pole entity.",
    "The event editor for `set` with a variable.":
        "Editor událostí pro `set` s proměnnou.",
    "Variable":
        "Proměnná",
    "The new value, for example `1`.":
        "Nová hodnota, například `1`.",
    "`set ( \"seen\", \"1\" )` at the end of the scene - next time the `if` is false.":
        "`set ( \"seen\", \"1\" )` na konci scény - příště bude `if` nepravdivé.",
    "set - change a variable":
        "set - změnit proměnnou",
    "Gives a declared variable a new value.":
        "Dá deklarované proměnné novou hodnotu.",
    "A `task` does nothing where it stands. It gives a name to its block, so that `do` can start it and `wait` or `dowait` can wait until it is finished - the only way to wait until a figure has really arrived somewhere.":
        "`task` tam, kde stojí, nic nedělá. Dává jméno svému bloku, aby ho `do` mohlo spustit a `wait` nebo `dowait` mohly počkat, až skončí - jediný způsob, jak počkat, až postava někam skutečně dorazí.",
    "Any name, unique inside this `affect` block - for example `walk`.":
        "Libovolný název, jedinečný v tomto bloku `affect` - například `walk`.",
    "`task ( \"walk\" )` with `SET_WALKING` and `SET_NAVGOAL` inside, then `dowait ( \"walk\" )`.":
        "`task ( \"walk\" )` se `SET_WALKING` a `SET_NAVGOAL` uvnitř, pak `dowait ( \"walk\" )`.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The commands inside appear on the figure's row when the task is started.":
        "**Ve Stopách** nezabere vlastní čas, takže nemá blok - stojí v **{{TracksList}}** vpravo. Kliknutí, dvojklik a pravé tlačítko tam fungují jako v Grafu. Příkazy uvnitř se objeví na řádku postavy, když se úloha spustí.",
    "See [[k-tasks]].":
        "Viz [[k-tasks]].",
    "task - a named group of commands":
        "task - pojmenovaná skupina příkazů",
    "Declares a group of commands that runs later, with `do` or `dowait`.":
        "Deklaruje skupinu příkazů, která poběží později, s `do` nebo `dowait`.",
    "`do` starts the named task, and the next command follows right away - so a figure can walk and look around at the same time.":
        "`do` spustí jmenovanou úlohu a další příkaz následuje hned - takže postava může jít a zároveň se rozhlížet.",
    "The name of a `task` in this block.":
        "Název `task` v tomto bloku.",
    "`do ( \"walk\" )`, `do ( \"look\" )`, then `wait ( \"walk\" )`.":
        "`do ( \"walk\" )`, `do ( \"look\" )`, pak `wait ( \"walk\" )`.",
    "do - start a task":
        "do - spustit úlohu",
    "`dowait` is `do` and `wait` in one line. For a walk task it waits until the figure has arrived - everything after it happens when the figure stands.":
        "`dowait` je `do` a `wait` na jednom řádku. U úlohy chůze čeká, dokud postava nedorazí - vše po něm se stane, když postava stojí.",
    "`dowait ( \"walk\" )`, then `set ( SET_LOOK_TARGET, \"rosh\" )` - Kyle looks at Rosh when he has arrived.":
        "`dowait ( \"walk\" )`, pak `set ( SET_LOOK_TARGET, \"rosh\" )` - Kyle se podívá na Roshe, až dorazí.",
    "dowait - start a task and wait for it":
        "dowait - spustit úlohu a počkat na ni",
    "Starts a task and waits until it is finished.":
        "Spustí úlohu a počká, až skončí.",
    "A ROFF file is a movement over time - a ship that lifts off and flies away, a falling pillar. **Animate** writes one for you from keys.":
        "Soubor ROFF je pohyb v čase - loď, která vzlétne a odletí, padající sloup. **Animace** vám ho napíše z klíčů.",
    "The ROFF below `scripts/` - without `.rof`.":
        "ROFF pod `scripts/` - bez `.rof`.",
    "`play ( PLAY_ROFF, \"taspir_tut/roffs/ravensClaw\" )` in the ship's block.":
        "`play ( PLAY_ROFF, \"taspir_tut/roffs/ravensClaw\" )` v bloku lodi.",
    "**In Tracks** it shows in the row of the figure it belongs to (the name in its `affect`), as long as it takes. Double-click the block to edit it, right-click for the menu.":
        "**Ve Stopách** se ukazuje v řádku postavy, ke které patří (název v jejím `affect`), tak dlouho, jak trvá. Dvojklikem na blok ho upravíte, pravým tlačítkem nabídka.",
    "Make and change ROFF files: [[k-roff-bearbeiten]].":
        "Vytváření a změna souborů ROFF: [[k-roff-bearbeiten]].",
    "play - play a ROFF file":
        "play - přehrát soubor ROFF",
    "Plays a recorded movement (ROFF) on the entity of the current `affect` block.":
        "Přehraje zaznamenaný pohyb (ROFF) na entitě aktuálního bloku `affect`.",
    "From here on the player's view is replaced by the script's camera, the HUD disappears and the player cannot move. Every cutscene starts with it.":
        "Od tohoto místa nahradí pohled hráče kamera skriptu, HUD zmizí a hráč se nemůže pohnout. Začíná tím každá animační sekvence.",
    "The event editor for `camera ( ENABLE )`.":
        "Editor událostí pro `camera ( ENABLE )`.",
    "Camera command":
        "Příkaz kamery",
    "The first field chooses what the camera does - here `ENABLE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `ENABLE`. Jeho změna změní pole níže.",
    "`camera ( ENABLE )` - the first camera line of the intro.":
        "`camera ( ENABLE )` - první řádek kamery v úvodu.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}**. The **Camera** row starts with the first `MOVE` after it.":
        "**Ve Stopách** nemá vlastní blok - je v **{{TracksList}}**. Řádek **Kamera** začíná prvním `MOVE` po něm.",
    "Without a `MOVE` and `PAN` right after it, the camera stays where the player was.":
        "Bez `MOVE` a `PAN` hned po něm zůstane kamera tam, kde byl hráč.",
    "camera ENABLE - the cutscene starts":
        "camera ENABLE - animační sekvence začíná",
    "Puts the game into camera mode.":
        "Přepne hru do režimu kamery.",
    "The player gets control back, the HUD returns. The last line of every cutscene.":
        "Hráč dostane zpět ovládání, HUD se vrátí. Poslední řádek každé animační sekvence.",
    "The event editor for `camera ( DISABLE )`.":
        "Editor událostí pro `camera ( DISABLE )`.",
    "The first field chooses what the camera does - here `DISABLE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `DISABLE`. Jeho změna změní pole níže.",
    "`camera ( DISABLE )` after the last wait.":
        "`camera ( DISABLE )` po posledním čekání.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}**. The last shot on the **Camera** row ends here.":
        "**Ve Stopách** nemá vlastní blok - je v **{{TracksList}}**. Poslední záběr na řádku **Kamera** zde končí.",
    "Forgetting it leaves the player stuck in the cutscene.":
        "Když na něj zapomenete, hráč uvízne v animační sekvenci.",
    "camera DISABLE - back to the game":
        "camera DISABLE - zpět do hry",
    "Takes the game out of camera mode.":
        "Vyvede hru z režimu kamery.",
    "With time `0` the camera jumps there (a cut); with a time it flies there. Together with `PAN` it makes a shot.":
        "S časem `0` kamera skočí tam (střih); s časem tam letí. Spolu s `PAN` tvoří záběr.",
    "The event editor for `camera ( MOVE )`.":
        "Editor událostí pro `camera ( MOVE )`.",
    "The first field chooses what the camera does - here `MOVE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `MOVE`. Jeho změna změní pole níže.",
    "`x y z` - or **{{EditorHelper}}** → **{{EditorTag}}** for a camera mark (`ref_tag`) on the map: `tag ( \"cam1\", ORIGIN )`.":
        "`x y z` - nebo **{{EditorHelper}}** → **{{EditorTag}}** pro značku kamery (`ref_tag`) v mapě: `tag ( \"cam1\", ORIGIN )`.",
    "Milliseconds for the flight; `0` jumps.":
        "Milisekundy letu; `0` skočí.",
    "`camera ( MOVE, < -750 -2700 1000 >, 3000 )` - three seconds' flight.":
        "`camera ( MOVE, < -750 -2700 1000 >, 3000 )` - třísekundový let.",
    "**In Tracks** every `MOVE` starts a new shot on the **Camera** row; a move with time is marked *moving*. Double-click the shot to edit its `MOVE`.":
        "**Ve Stopách** začíná každý `MOVE` nový záběr na řádku **Kamera**; pohyb s časem je označen *v pohybu*. Dvojklikem na záběr upravíte jeho `MOVE`.",
    "**Insert camera here** in the timeline writes `MOVE` and `PAN` from the view - [[k-kamera-ansicht]].":
        "**Vložit kameru sem** na časové ose zapíše `MOVE` a `PAN` z pohledu - [[k-kamera-ansicht]].",
    "camera MOVE - place or fly the camera":
        "camera MOVE - umístit kameru nebo s ní letět",
    "Moves the camera to a position - at once or over time.":
        "Posune kameru na polohu - hned, nebo v čase.",
    "`PAN` sets where the camera looks: pitch (up/down), yaw (left/right), roll. With a time it turns slowly.":
        "`PAN` nastaví, kam se kamera dívá: sklon (nahoru/dolů), otočení (doleva/doprava), náklon. S časem se otáčí pomalu.",
    "The event editor for `camera ( PAN )`.":
        "Editor událostí pro `camera ( PAN )`.",
    "The first field chooses what the camera does - here `PAN`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `PAN`. Jeho změna změní pole níže.",
    "Pitch, yaw, roll the camera ends at.":
        "Sklon, otočení, náklon, na kterých kamera skončí.",
    "Direction":
        "Směr",
    "Which way to turn - `0 0 0` takes the shortest way.":
        "Kterým směrem otáčet - `0 0 0` vezme nejkratší cestu.",
    "Milliseconds; `0` turns at once.":
        "Milisekundy; `0` otočí hned.",
    "**In Tracks** the camera row shows the shots - from one `MOVE` to the next. This command is listed in the **{{TracksList}}**; the timeline under the map shows it in its own camera row.":
        "**Ve Stopách** ukazuje řádek kamery záběry - od jednoho `MOVE` k dalšímu. Tento příkaz je vypsán v **{{TracksList}}**; časová osa pod mapou ho ukazuje ve vlastním řádku kamery.",
    "camera PAN - turn the camera":
        "camera PAN - otočit kameru",
    "Turns the camera to angles - at once or over time.":
        "Otočí kameru na úhly - hned, nebo v čase.",
    "A smaller field of view zooms in, a larger one zooms out. Normal is 90 (after `ENABLE`); 80 is the player's own view before `ENABLE` and after `DISABLE`.":
        "Menší zorné pole přibližuje, větší oddaluje. Normální je 90 (po `ENABLE`); 80 je vlastní pohled hráče před `ENABLE` a po `DISABLE`.",
    "The event editor for `camera ( ZOOM )`.":
        "Editor událostí pro `camera ( ZOOM )`.",
    "The first field chooses what the camera does - here `ZOOM`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `ZOOM`. Jeho změna změní pole níže.",
    "Field of view":
        "Zorné pole",
    "Degrees - Raven's scripts use 6.3 (far zoomed in) to 110 (wide). Normal: 90.":
        "Stupně - skripty Raven používají 6.3 (silně přiblíženo) až 110 (široce). Normálně: 90.",
    "Milliseconds to get there.":
        "Milisekundy, za které se ho dosáhne.",
    "`camera ( ZOOM, 40, 2000 )` - zoom in over two seconds.":
        "`camera ( ZOOM, 40, 2000 )` - přiblížení během dvou sekund.",
    "How wide a zoom looks also depends on the game screen (aspect ratio, cg_fovAspectAdjust): [[kamera#vorschau]].":
        "Jak široce zoom působí, závisí i na obrazovce hry (poměr stran, cg_fovAspectAdjust): [[kamera#vorschau]].",
    "camera ZOOM - change the field of view":
        "camera ZOOM - změnit zorné pole",
    "Changes how much the camera sees.":
        "Mění, kolik kamera vidí.",
    "Tilts the picture to the side - for a dizzy or dramatic shot.":
        "Nakloní obraz do strany - pro závratný nebo dramatický záběr.",
    "The event editor for `camera ( ROLL )`.":
        "Editor událostí pro `camera ( ROLL )`.",
    "The first field chooses what the camera does - here `ROLL`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `ROLL`. Jeho změna změní pole níže.",
    "Angle":
        "Úhel",
    "The roll angle the camera ends at (not added to the current roll).":
        "Úhel náklonu, na kterém kamera skončí (nepřičítá se k aktuálnímu náklonu).",
    "Milliseconds; with a time it takes the shorter way there.":
        "Milisekundy; s časem jde tam kratší cestou.",
    "camera ROLL - tilt the picture":
        "camera ROLL - naklonit obraz",
    "Rolls the camera to an angle.":
        "Nakloní kameru na úhel.",
    "A track is a chain of path corners in the map. The camera rides along it at a speed.":
        "Trať je řetěz rohů cesty v mapě. Kamera po ní jede rychlostí.",
    "The event editor for `camera ( TRACK )`.":
        "Editor událostí pro `camera ( TRACK )`.",
    "The first field chooses what the camera does - here `TRACK`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `TRACK`. Jeho změna změní pole níže.",
    "Track":
        "Trať",
    "The `targetname` of the first path corner.":
        "`targetname` prvního rohu cesty.",
    "Speed":
        "Rychlost",
    "Units per second.":
        "Jednotky za sekundu.",
    "Lerp":
        "Plynulý přechod",
    "`1`: glide to the start of the track first; `0`: jump there.":
        "`1`: nejdřív plynule najet na začátek trati; `0`: skočit tam.",
    "Camera paths: [[k-blickpunkte]].":
        "Dráhy kamery: [[k-blickpunkte]].",
    "camera TRACK - ride a camera path":
        "camera TRACK - jet po dráze kamery",
    "Puts the camera on a track of the map and moves along it.":
        "Postaví kameru na trať mapy a pohybuje se po ní.",
    "The camera keeps looking at every entity whose `cameraGroup` matches - it turns as they move.":
        "Kamera se stále dívá na každou entitu, jejíž `cameraGroup` se shoduje - otáčí se, jak se pohybují.",
    "The event editor for `camera ( FOLLOW )`.":
        "Editor událostí pro `camera ( FOLLOW )`.",
    "The first field chooses what the camera does - here `FOLLOW`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `FOLLOW`. Jeho změna změní pole níže.",
    "Camera group":
        "Skupina kamery",
    "The `cameraGroup` value of the entities to follow (or a name).":
        "Hodnota `cameraGroup` entit, které se mají sledovat (nebo název).",
    "Angle speed":
        "Rychlost úhlu",
    "How fast the camera may turn.":
        "Jak rychle se kamera smí otáčet.",
    "`1`: turn smoothly to the start angle; `0`: snap.":
        "`1`: plynule se otočit na výchozí úhel; `0`: skokem.",
    "See [[k-kamera-folgen]].":
        "Viz [[k-kamera-folgen]].",
    "camera FOLLOW - keep figures in view":
        "camera FOLLOW - držet postavy v záběru",
    "Turns the camera to follow entities of a camera group.":
        "Otáčí kameru za entitami skupiny kamery.",
    "Used with `FOLLOW`: the camera also moves to stay this far from the group.":
        "Používá se s `FOLLOW`: kamera se také pohybuje, aby zůstala v této vzdálenosti od skupiny.",
    "The event editor for `camera ( DISTANCE )`.":
        "Editor událostí pro `camera ( DISTANCE )`.",
    "The first field chooses what the camera does - here `DISTANCE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `DISTANCE`. Jeho změna změní pole níže.",
    "Distance":
        "Vzdálenost",
    "Units.":
        "Jednotky.",
    "`1`: move smoothly; `0`: jump.":
        "`1`: pohybovat se plynule; `0`: skočit.",
    "camera DISTANCE - keep a distance":
        "camera DISTANCE - držet vzdálenost",
    "Keeps the camera at a distance from its camera group.":
        "Drží kameru ve vzdálenosti od její skupiny kamery.",
    "Black at the end of a scene, white for a flash, or back from black at the start.":
        "Černá na konci scény, bílá pro záblesk, nebo zpět z černé na začátku.",
    "The event editor for `camera ( FADE )`.":
        "Editor událostí pro `camera ( FADE )`.",
    "The first field chooses what the camera does - here `FADE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `FADE`. Jeho změna změní pole níže.",
    "Start colour":
        "Počáteční barva",
    "Red, green, blue from 0 to 1 - `0 0 0` is black.":
        "Červená, zelená, modrá od 0 do 1 - `0 0 0` je černá.",
    "Start opacity":
        "Počáteční krytí",
    "0 = clear picture, 1 = full colour.":
        "0 = čistý obraz, 1 = plná barva.",
    "End colour":
        "Koncová barva",
    "Red, green, blue at the end.":
        "Červená, zelená, modrá na konci.",
    "End opacity":
        "Koncové krytí",
    "0 to 1.":
        "0 až 1.",
    "`camera ( FADE, < 0 0 0 >, 0, < 0 0 0 >, 1, 1500 )` - fade to black in 1.5 seconds.":
        "`camera ( FADE, < 0 0 0 >, 0, < 0 0 0 >, 1, 1500 )` - zatmívání do černa za 1.5 sekundy.",
    "Zoom, fade and shake: [[k-zoom-blende]].":
        "Zoom, zatmívání a otřesy: [[k-zoom-blende]].",
    "camera FADE - fade to or from a colour":
        "camera FADE - prolnout do barvy nebo z ní",
    "Fades the picture from one colour and opacity to another.":
        "Prolne obraz z jedné barvy a krytí do jiné.",
    "An explosion, an engine starting, a ship taking off.":
        "Výbuch, startující motor, vzlétající loď.",
    "The event editor for `camera ( SHAKE )`.":
        "Editor událostí pro `camera ( SHAKE )`.",
    "The first field chooses what the camera does - here `SHAKE`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `SHAKE`. Jeho změna změní pole níže.",
    "Intensity":
        "Intenzita",
    "0 to 16.":
        "0 až 16.",
    "camera SHAKE - shake the picture":
        "camera SHAKE - otřást obrazem",
    "Shakes the camera for a while.":
        "Na chvíli otřese kamerou.",
    "A recorded camera flight - made in an editor or with **Animate** - is played as the camera's movement. The path starts where the camera is; JK Studio plays it like the game, including the notes `cut`, `fov`, `fovzoom` and `fovaccel`. A `MOVE` ends the path.":
        "Zaznamenaný let kamery - vytvořený v editoru nebo pomocí **Animace** - se přehraje jako pohyb kamery. Dráha začíná tam, kde je kamera; JK Studio ji přehrává jako hra, včetně poznámek `cut`, `fov`, `fovzoom` a `fovaccel`. `MOVE` dráhu ukončí.",
    "The event editor for `camera ( PATH )`.":
        "Editor událostí pro `camera ( PATH )`.",
    "The first field chooses what the camera does - here `PATH`. Changing it changes the fields below.":
        "První pole volí, co kamera dělá - zde `PATH`. Jeho změna změní pole níže.",
    "The ROFF file below `scripts/`, without `.rof`.":
        "Soubor ROFF pod `scripts/`, bez `.rof`.",
    "camera PATH - fly a recorded path":
        "camera PATH - proletět zaznamenanou dráhu",
    "Plays a ROFF file as the camera's path.":
        "Přehraje soubor ROFF jako dráhu kamery.",
    "Commands one by one":
        "Příkazy jeden po druhém",
    "One page per command: what it does, where to enter what in the editor, an example, and how it looks in Classic, Graph and Tracks.":
        "Jedna stránka na příkaz: co dělá, kam co zadat v editoru, příklad a jak vypadá v Klasickém, Grafu a Stopách.",
    "The full list of every `SET_` field, every camera command and every macro is on [[befehle]]. Here, each command of the **{{GroupEvents}}** list has its own page with pictures.":
        "Úplný seznam každého pole `SET_`, každého příkazu kamery a každého makra je na [[befehle]]. Zde má každý příkaz seznamu **{{GroupEvents}}** vlastní stránku s obrázky.",
    "Flow":
        "Průběh",
    "Waiting":
        "Čekání",
    "Tasks":
        "Úlohy",
    "Variables and text":
        "Proměnné a text",
    "Start here":
        "Začněte zde",
    "Map and scene":
        "Mapa a scéna",
    "Reference":
        "Reference",
    "**Open an empty tab.** Click the **+** at the end of the tab bar (or **{{MenuFile}} → {{FileNew}}**). A new tab `unnamed.txt` opens with an empty script. The mode bar on the left must be on **{{ViewEvents}}** - then the **{{GroupEvents}}** list with every command is on the left.":
        "**Otevřete prázdnou kartu.** Klikněte na **+** na konci řádku karet (nebo **{{MenuFile}} → {{FileNew}}**). Otevře se nová karta `unnamed.txt` s prázdným skriptem. Lišta režimů vlevo musí být na **{{ViewEvents}}** - pak je vlevo seznam **{{GroupEvents}}** se všemi příkazy.",
    "The + at the end of the tab bar opens an empty script.":
        "+ na konci řádku karet otevře prázdný skript.",
    "**Find affect in the list.** The **{{GroupEvents}}** list shows every command of the game with its values. `affect` is near the top. Hover over it to read what it does.":
        "**Najděte affect v seznamu.** Seznam **{{GroupEvents}}** ukazuje každý příkaz hry s jeho hodnotami. `affect` je skoro nahoře. Najeďte na něj myší a přečtěte si, co dělá.",
    "affect in the Events list.":
        "affect v seznamu Události.",
    "**Double-click it.** `affect` appears in the script tree in the middle. It is already open (the box in front shows a minus), because a block holds other commands.":
        "**Dvakrát na něj klikněte.** `affect` se objeví ve stromu skriptu uprostřed. Je už otevřený (rámeček vpředu ukazuje minus), protože blok obsahuje jiné příkazy.",
    "The new affect block in the tree.":
        "Nový blok affect ve stromu.",
    "**Name the figure.** Double-click the new line (or select it and press <kbd>Enter</kbd>). The **{{EditorTitle}}** opens. Type the name of the figure into the first field - with a map loaded, the small arrow next to it lists every name of the map. Leave `FLUSH` in the second field and press **{{EditorOk}}**.":
        "**Pojmenujte postavu.** Dvakrát klikněte na nový řádek (nebo ho vyberte a stiskněte <kbd>Enter</kbd>). Otevře se **{{EditorTitle}}**. Do prvního pole napište jméno postavy - s načtenou mapou malá šipka vedle něj vypíše každý název mapy. Ve druhém poli nechte `FLUSH` a stiskněte **{{EditorOk}}**.",
    "The event editor for affect: the name field and Ok.":
        "Editor událostí pro affect: pole názvu a OK.",
    "**Select the block, then pick the next command.** Click the `affect` line once (1). Then double-click `wait` in the list (2). Because the selected block is open, the command goes to the start of the block.":
        "**Vyberte blok, pak vyberte další příkaz.** Klikněte jednou na řádek `affect` (1). Pak v seznamu dvakrát klikněte na `wait` (2). Protože je vybraný blok otevřený, příkaz jde na začátek bloku.",
    "1: the selected affect block. 2: wait in the list.":
        "1: vybraný blok affect. 2: wait v seznamu.",
    "**Check the result.** `wait` now sits inside the block, indented under `affect`. Commands you insert after it follow it, in order, inside the same block.":
        "**Zkontrolujte výsledek.** `wait` teď sedí uvnitř bloku, odsazený pod `affect`. Příkazy, které vložíte po něm, ho následují v pořadí, ve stejném bloku.",
    "wait inside the affect block.":
        "wait uvnitř bloku affect.",
    "**Save.** Press **{{FileSave}}** in the action column on the right (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The first time, behaved asks for a name - script names are lower case without spaces, for example `my_first.txt`.":
        "**Uložte.** Stiskněte **{{FileSave}}** ve sloupci akcí vpravo (nebo <kbd>Ctrl</kbd>+<kbd>S</kbd>). Napoprvé se behaved zeptá na název - názvy skriptů jsou malými písmeny bez mezer, například `my_first.txt`.",
    "Save in the action column.":
        "Uložit ve sloupci akcí.",
    "**Double-click.** Select the line after which the command should come (1), then double-click the command in the **{{GroupEvents}}** list (2). The new line appears right below the selected one - here a `wait` after `camera ( MOVE )`. That is how the original BehavEd worked.":
        "**Dvojklik.** Vyberte řádek, za který má příkaz přijít (1), pak dvakrát klikněte na příkaz v seznamu **{{GroupEvents}}** (2). Nový řádek se objeví hned pod vybraným - zde `wait` za `camera ( MOVE )`. Tak fungoval původní BehavEd.",
    "1: the selected line. 2: the command in the list.":
        "1: vybraný řádek. 2: příkaz v seznamu.",
    "**Insert key.** Click a command in the list once - it stays highlighted (1). Select a line in the tree (2) and press <kbd>Insert</kbd>. Handy when you add the same command several times: select, <kbd>Insert</kbd>, select, <kbd>Insert</kbd> ...":
        "**Klávesa Insert.** Klikněte jednou na příkaz v seznamu - zůstane zvýrazněný (1). Vyberte řádek ve stromu (2) a stiskněte <kbd>Insert</kbd>. Hodí se, když přidáváte stejný příkaz několikrát: vybrat, <kbd>Insert</kbd>, vybrat, <kbd>Insert</kbd> ...",
    "1: the command stays selected in the list. 2: where it goes.":
        "1: příkaz zůstane v seznamu vybraný. 2: kam půjde.",
    "**Drag.** Drag a command from the list (1) onto a line of the tree (2). While you drag, a line shows where it lands: above a line, below it, or - on the middle of an open block - inside the block.":
        "**Přetažení.** Přetáhněte příkaz ze seznamu (1) na řádek stromu (2). Během tažení čára ukazuje, kam dopadne: nad řádek, pod něj, nebo - doprostřed otevřeného bloku - do bloku.",
    "Drag from the list (1) onto the tree (2).":
        "Přetažení ze seznamu (1) do stromu (2).",
    "**Right-click.** Right-click a line in the tree: the submenus **{{GroupEvents}}** and **Macros** list every command; the one you click is inserted after that line - or into the block, if you right-clicked an open block.":
        "**Pravé tlačítko.** Klikněte pravým tlačítkem na řádek ve stromu: podnabídky **{{GroupEvents}}** a **Makra** vypisují každý příkaz; ten, na který kliknete, se vloží za tento řádek - nebo do bloku, pokud jste klikli na otevřený blok.",
    "The context menu of a line with its Events submenu.":
        "Místní nabídka řádku s podnabídkou Události.",
    "**Open the editor.** Double-click a line in the tree or select it and press <kbd>Enter</kbd>. The help text of the command stands on top; below it, every value has its own field.":
        "**Otevřete editor.** Dvakrát klikněte na řádek ve stromu, nebo ho vyberte a stiskněte <kbd>Enter</kbd>. Nahoře stojí text nápovědy příkazu; pod ním má každá hodnota vlastní pole.",
    "**Fields for values.** Numbers, vectors and text are typed into their fields. A vector like a position has three numbers separated by spaces.":
        "**Pole pro hodnoty.** Čísla, vektory a text se píší do svých polí. Vektor jako poloha má tři čísla oddělená mezerami.",
    "The value fields of camera ( MOVE ): a position and a time.":
        "Pole hodnot camera ( MOVE ): poloha a čas.",
    "**Fields with fixed choices.** Fields like `SET_TYPES` or `true` / `false` are lists you can also type into - start typing to filter the list. A value that is not in the list is allowed (mods bring their own), but it is marked **unknown**: check it for a typo.":
        "**Pole s pevnými volbami.** Pole jako `SET_TYPES` nebo `true` / `false` jsou seznamy, do kterých lze i psát - začněte psát a seznam se zúží. Hodnota, která v seznamu není, je povolena (mody přinášejí vlastní), ale je označena jako **neznámá**: zkontrolujte překlep.",
    "The two list fields of set ( SET_WALKING, true ).":
        "Dvě seznamová pole set ( SET_WALKING, true ).",
    "**Files.** File fields have **{{EditorBrowse}}** to pick a file of the game and **{{EditorPlay}}** to play a sound or open a script. With a map loaded, an arrow next to name fields picks a name from the map.":
        "**Soubory.** Souborová pole mají **{{EditorBrowse}}** pro výběr souboru hry a **{{EditorPlay}}** pro přehrání zvuku nebo otevření skriptu. S načtenou mapou vybírá šipka vedle polí se jmény jméno z mapy.",
    "A sound command: browse and play.":
        "Příkaz zvuku: procházet a přehrát.",
    "**Expressions.** Some fields have **{{EditorHelper}}**: it builds an expression for you - **{{EditorGet}}** reads a variable (`get`), **{{EditorTag}}** the place or angles of a `ref_tag` (`tag`), **{{EditorRnd}}** a random number (`random`).":
        "**Výrazy.** Některá pole mají **{{EditorHelper}}**: sestaví za vás výraz - **{{EditorGet}}** čte proměnnou (`get`), **{{EditorTag}}** místo nebo úhly `ref_tag` (`tag`), **{{EditorRnd}}** náhodné číslo (`random`).",
    "Helper next to a field of the camera command.":
        "Pomocník vedle pole příkazu kamery.",
    "**Free text.** Other fields have **{{EditorExpr}}**: it turns the field into free text, for anything the lists do not offer. **{{EditorRevert}}** goes back to a plain value.":
        "**Volný text.** Jiná pole mají **{{EditorExpr}}**: změní pole na volný text, pro cokoli, co seznamy nenabízejí. **{{EditorRevert}}** se vrátí k prosté hodnotě.",
    "Expr! next to the fields of affect.":
        "Výraz! vedle polí affect.",
    "**Done.** **{{EditorOk}}** (<kbd>Enter</kbd>) writes the values into the script, **{{EditorCancel}}** (<kbd>Esc</kbd>) leaves everything as it was. Under each field you see the value it had before (**{{EditorWas}}**).":
        "**Hotovo.** **{{EditorOk}}** (<kbd>Enter</kbd>) zapíše hodnoty do skriptu, **{{EditorCancel}}** (<kbd>Esc</kbd>) nechá vše, jak bylo. Pod každým polem vidíte hodnotu, kterou mělo předtím (**{{EditorWas}}**).",
    "Ok and Cancel.":
        "OK a Zrušit.",
    "**Select.** Click a line in the tree. <kbd>Ctrl</kbd>+click adds more lines, <kbd>Shift</kbd>+click selects everything between. Every button of the action column works on all selected lines at once.":
        "**Výběr.** Klikněte na řádek ve stromu. <kbd>Ctrl</kbd>+kliknutí přidá další řádky, <kbd>Shift</kbd>+kliknutí vybere vše mezi. Každé tlačítko sloupce akcí působí na všechny vybrané řádky najednou.",
    "Two selected lines.":
        "Dva vybrané řádky.",
    "**Move.** Drag the selection to a new place - or press **{{ActMoveUp}}** / **{{ActMoveDown}}** in the action column (<kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd>) to move it one line. It stays inside its block. A block moves with everything inside it.":
        "**Přesun.** Přetáhněte výběr na nové místo - nebo stiskněte **{{ActMoveUp}}** / **{{ActMoveDown}}** ve sloupci akcí (<kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd>) a posunete ho o řádek. Zůstane ve svém bloku. Blok se přesouvá se vším, co obsahuje.",
    "Move up and Move down.":
        "Posunout nahoru a Posunout dolů.",
    "**Duplicate.** **{{ActClone}}** (<kbd>Space</kbd>) inserts a copy of the selection right below - the quickest way to a second camera shot or a second walk.":
        "**Duplikovat.** **{{ActClone}}** (<kbd>Space</kbd>) vloží kopii výběru hned pod - nejrychlejší cesta k druhému záběru kamery nebo druhé chůzi.",
    "Duplicate.":
        "Duplikovat.",
    "**Copy between scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>) copies the selection. Switch to another tab, select a line and press **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>): the commands are inserted below that line.":
        "**Kopírování mezi skripty.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>) zkopíruje výběr. Přepněte na jinou kartu, vyberte řádek a stiskněte **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>): příkazy se vloží pod tento řádek.",
    "Copy and Paste.":
        "Kopírovat a Vložit.",
    "**Switch off without deleting.** **{{ActRem}}** (<kbd>Backspace</kbd>) turns the selected lines into comments: they stay in the script, but the game skips them. Press it again (or <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) to switch them back on.":
        "**Vypnout bez smazání.** **{{ActRem}}** (<kbd>Backspace</kbd>) změní vybrané řádky na komentáře: zůstanou ve skriptu, ale hra je přeskočí. Dalším stiskem (nebo <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) je znovu zapnete.",
    "REM: the two lines are now comments.":
        "REM: oba řádky jsou teď komentáře.",
    "**Undo and redo.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> undoes the last change, <kbd>Ctrl</kbd>+<kbd>Y</kbd> redoes it. In the **{{MenuEdit}}** menu the entry names the step - here *Undo: edit*.":
        "**Zpět a znovu.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> vrátí poslední změnu, <kbd>Ctrl</kbd>+<kbd>Y</kbd> ji provede znovu. V nabídce **{{MenuEdit}}** položka krok pojmenuje - zde *Zpět: úprava*.",
    "The Edit menu names the step that Undo takes back.":
        "Nabídka Úpravy pojmenuje krok, který Zpět vrátí.",
    "**See the whole history.** Right-click the **Undo** button in the action column (or **{{MenuEdit}} → {{UndoListMenu}}**). The list shows every step, newest at the top.":
        "**Zobrazte celou historii.** Klikněte pravým tlačítkem na tlačítko **Zpět** ve sloupci akcí (nebo **{{MenuEdit}} → {{UndoListMenu}}**). Seznam ukazuje každý krok, nejnovější nahoře.",
    "**Go back several steps.** Click a step in the list and press **{{UndoUpToHere}}**: that step and all newer ones are undone.":
        "**Vraťte se o několik kroků.** Klikněte na krok v seznamu a stiskněte **{{UndoUpToHere}}**: tento krok a všechny novější se vrátí.",
    "Undo up to here.":
        "Vrátit až sem.",
    "Undo only this step.":
        "Vrátit jen tento krok.",
    "**Open all scripts.** For a mission, **{{MenuFile}} → {{LoadMission}}** opens every script in its own tab - then a search can reach all of them.":
        "**Otevřete všechny skripty.** U mise otevře **{{MenuFile}} → {{LoadMission}}** každý skript na vlastní kartě - pak hledání zasáhne všechny.",
    "Load mission in the File menu.":
        "Načíst misi v nabídce Soubor.",
    "**Open Find.** Press **{{ActFind}}** in the action column (<kbd>Ctrl</kbd>+<kbd>F</kbd>). The window stays open while you work in the tree. Its fields:":
        "**Otevřete Najít.** Stiskněte **{{ActFind}}** ve sloupci akcí (<kbd>Ctrl</kbd>+<kbd>F</kbd>). Okno zůstane otevřené, zatímco pracujete ve stromu. Jeho pole:",
    "**Search a value.** Type the old name into **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) and **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) jump from hit to hit. Tick **{{FindWhole}}** so that `kyle` does not also hit `kyle2`.":
        "**Hledání hodnoty.** Napište staré jméno do **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) a **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) skáčou od nálezu k nálezu. Zaškrtněte **{{FindWhole}}**, aby `kyle` nenacházelo i `kyle2`.",
    "Item containing, the buttons to step through the hits, and Whole-string match only.":
        "Položka obsahující, tlačítka pro procházení nálezů a Jen shoda celého řetězce.",
    "Item called.":
        "Položka s názvem.",
    "**Replace.** Type the new name into **{{FindReplaceWith}}** and press **{{FindReplaceAllDocs}}**. Every open script is changed; each one gets its own undo step.":
        "**Nahrazení.** Napište nové jméno do **{{FindReplaceWith}}** a stiskněte **{{FindReplaceAllDocs}}**. Změní se každý otevřený skript; každý dostane vlastní krok zpět.",
    "Replace with and Replace All in All Opened Documents.":
        "Nahradit čím a Nahradit vše ve všech otevřených dokumentech.",
    "**Set a bookmark.** Select a line and press <kbd>Ctrl</kbd>+<kbd>F2</kbd> - or **{{MenuEdit}} → {{MenuBookmarks}} → {{ActBookmarkToggle}}**.":
        "**Nastavte záložku.** Vyberte řádek a stiskněte <kbd>Ctrl</kbd>+<kbd>F2</kbd> - nebo **{{MenuEdit}} → {{MenuBookmarks}} → {{ActBookmarkToggle}}**.",
    "Toggle bookmark in the Edit menu.":
        "Přepnout záložku v nabídce Úpravy.",
    "**See it.** A mark appears in the margin next to the line. Clicking the margin sets or removes a bookmark too.":
        "**Podívejte se.** V okraji vedle řádku se objeví značka. Kliknutí do okraje také záložku nastaví nebo odebere.",
    "The bookmarked line.":
        "Řádek se záložkou.",
    "Next and Previous bookmark.":
        "Další a předchozí záložka.",
    "**Copy across.** The right side is read-only, but you can drag lines from it into your script on the left - they are copied. To edit the right one, click its tab.":
        "**Kopírování napříč.** Pravá strana je jen pro čtení, ale můžete z ní přetahovat řádky do svého skriptu vlevo - zkopírují se. Chcete-li upravit ten pravý, klikněte na jeho kartu.",
    "Left: your script. Right: the other one, read-only.":
        "Vlevo: váš skript. Vpravo: ten druhý, jen pro čtení.",
    "**Compile.** Press **{{Compile}}** at the bottom of the action column. It saves the `.txt` and writes the `.ibi` the game runs next to it. When it worked, the button flashes green.":
        "**Kompilace.** Stiskněte **{{Compile}}** dole ve sloupci akcí. Uloží `.txt` a vedle něj zapíše `.ibi`, které hra spouští. Když se to povede, tlačítko zabliká zeleně.",
    "**When it fails.** The button flashes red, and the first wrong line is selected in the tree - here an `if` with the comparison `~`, which ICARUS does not know.":
        "**Když selže.** Tlačítko zabliká červeně a první špatný řádek se ve stromu vybere - zde `if` s porovnáním `~`, které ICARUS nezná.",
    "Compile failed: the button and the selected wrong line.":
        "Kompilace selhala: tlačítko a vybraný špatný řádek.",
    "**Read the messages.** Click the counter in the status line at the bottom: the **{{MessagesTitle}}** window lists every message with its line and what is wrong.":
        "**Přečtěte si zprávy.** Klikněte na počítadlo ve stavovém řádku dole: okno **{{MessagesTitle}}** vypíše každou zprávu s jejím řádkem a co je špatně.",
    "The counters in the status line open the Messages window.":
        "Počítadla ve stavovém řádku otevřou okno Zprávy.",
    "**Fix and compile again.** Double-click the wrong line, correct the value - here the comparison: `=`, `<`, `>` or `!` - press **{{EditorOk}}** and **{{Compile}}** again.":
        "**Opravte a zkompilujte znovu.** Dvakrát klikněte na špatný řádek, opravte hodnotu - zde porovnání: `=`, `<`, `>` nebo `!` - stiskněte **{{EditorOk}}** a znovu **{{Compile}}**.",
    "The wrong value in the event editor.":
        "Špatná hodnota v editoru událostí.",
    "**Why.** An `affect` block runs beside the script - the script does not wait for it. That is why Kyle and Rosh, each in his own block, already walk at the same time.":
        "**Proč.** Blok `affect` běží vedle skriptu - skript na něj nečeká. Proto Kyle a Rosh, každý ve svém bloku, už jdou současně.",
    "Two affect blocks: they run side by side.":
        "Dva bloky affect: běží vedle sebe.",
    "**The problem.** Inside a block, `set ( SET_NAVGOAL )` only *starts* the walk; the next command follows at once - the figure would turn or talk while still walking. **The fix:** put the walk into a `task`. Insert `task`, name it `walk`, and move `SET_WALKING` and `SET_NAVGOAL` into it.":
        "**Problém.** Uvnitř bloku `set ( SET_NAVGOAL )` chůzi jen *spustí*; další příkaz následuje hned - postava by se otáčela nebo mluvila, ještě když jde. **Řešení:** dejte chůzi do `task`. Vložte `task`, pojmenujte ho `walk` a přesuňte do něj `SET_WALKING` a `SET_NAVGOAL`.",
    "The task walk with the walk inside.":
        "Úloha walk s chůzí uvnitř.",
    "dowait waits for the walk.":
        "dowait čeká na chůzi.",
    "**Two things at once.** `do` starts a task and goes on at once. Two tasks - `walk` and `look` - started with two `do`, and `wait ( \"walk\" )` later waits for the first one.":
        "**Dvě věci zároveň.** `do` spustí úlohu a hned pokračuje. Dvě úlohy - `walk` a `look` - spuštěné dvěma `do` a `wait ( \"walk\" )` později čeká na první.",
    "do, do and wait.":
        "do, do a wait.",
    "**Check it in the timeline.** Switch to **{{ViewMap}}**, expand the figure's row in the timeline: what follows the `dowait` starts where the walk ends.":
        "**Zkontrolujte to na časové ose.** Přepněte na **{{ViewMap}}**, rozbalte řádek postavy na časové ose: co následuje po `dowait`, začíná tam, kde chůze končí.",
    "Kyle's walk in the timeline.":
        "Kylova chůze na časové ose.",
    "**Where to wait.** In the script that should pause - here the intro - insert `waitsignal` and type a name, for example `door_open`.":
        "**Kde čekat.** Do skriptu, který se má zastavit - zde úvod - vložte `waitsignal` a napište název, například `door_open`.",
    "waitsignal in the intro (left).":
        "waitsignal v úvodu (vlevo).",
    "**Where to give the signal.** In the other script, at the moment the first should go on, insert `signal ( \"door_open\" )`.":
        "**Kde dát signál.** Do druhého skriptu, v okamžiku, kdy má první pokračovat, vložte `signal ( \"door_open\" )`.",
    "signal in the other script (right).":
        "signal v druhém skriptu (vpravo).",
    "**Check the names.** Both must be spelled exactly the same. Only one script can wait for a signal of a given name. The split view (**{{MenuFile}} → {{SplitToggle}} → 2**) shows both at once.":
        "**Zkontrolujte názvy.** Oba musí být napsané přesně stejně. Na signál daného názvu může čekat jen jeden skript. Rozdělené zobrazení (**{{MenuFile}} → {{SplitToggle}} → 2**) ukáže oba najednou.",
    "The same name on both sides.":
        "Stejný název na obou stranách.",
    "Waits as blocks in the timeline.":
        "Čekání jako bloky na časové ose.",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list - scroll down. Each shows how many commands it holds; hover over one to read what it does.":
        "**Najděte je.** Makra jsou na konci seznamu **{{GroupEvents}}** - posuňte se dolů. Každé ukazuje, kolik příkazů obsahuje; najeďte na některé myší a přečtěte si, co dělá.",
    "standOnly and the other macros at the end of the Events list.":
        "standOnly a další makra na konci seznamu Události.",
    "**Insert.** Double-click or drag a macro like a command. `standOnly`, for example, makes a figure stand and ignore enemies - three `set` commands in one line.":
        "**Vložení.** Makro dvakrát klikněte nebo přetáhněte jako příkaz. `standOnly` například nechá postavu stát a ignorovat nepřátele - tři příkazy `set` na jednom řádku.",
    "Fold Macros and a macro in the tree.":
        "Sbalit makra a makro ve stromu.",
    "**Declare once.** In a script that runs only once - the intro, a spawn script - insert `declare`. In the editor choose `FLOAT` and type a name, for example `seen`. A declared variable starts at 0; a map can hold 16 of them.":
        "**Deklarujte jednou.** Do skriptu, který běží jen jednou - úvod, spawn skript - vložte `declare`. V editoru zvolte `FLOAT` a napište název, například `seen`. Deklarovaná proměnná začíná na 0; mapa jich pojme 16.",
    "declare: the type and the name.":
        "declare: typ a název.",
    "**Ask with if.** In the script that may run more often, insert `if`. On the first value press **{{EditorHelper}}** and build `get ( FLOAT, \"seen\" )` with **{{EditorGet}}**; as comparison `=`, as the other value `0`.":
        "**Zeptejte se pomocí if.** Do skriptu, který může běžet častěji, vložte `if`. U první hodnoty stiskněte **{{EditorHelper}}** a sestavte `get ( FLOAT, \"seen\" )` pomocí **{{EditorGet}}**; jako porovnání `=`, jako druhou hodnotu `0`.",
    "The event editor for if: the variable and the value.":
        "Editor událostí pro if: proměnná a hodnota.",
    "**Do it inside, or else.** Put the commands of the scene into the `if` block. An `else` right after it runs when the scene was already seen.":
        "**Udělejte to uvnitř, nebo jinak.** Vložte příkazy scény do bloku `if`. `else` hned za ním se spustí, když už scéna byla viděna.",
    "The if and the else block.":
        "Blok if a blok else.",
    "set ( \"seen\", \"1\" ) inside the if.":
        "set ( \"seen\", \"1\" ) uvnitř if.",
    "free at the end.":
        "free na konci.",
    "**Open it.** Double-click the entry: the `.ibi` opens as a script in the current tab - commands, values and blocks, exactly as Raven wrote them. Open an empty tab first (**+**) if you want to keep the script you are working on.":
        "**Otevřete ho.** Dvakrát klikněte na položku: `.ibi` se otevře jako skript v aktuální kartě - příkazy, hodnoty a bloky přesně tak, jak je Raven napsal. Chcete-li si ponechat skript, na kterém pracujete, nejdřív otevřete prázdnou kartu (**+**).",
    "Open in the File menu.":
        "Otevřít v nabídce Soubor.",
    "**Change it.** A script from an archive has no file yet - the first **{{FileSave}}** asks where to put it. To ship a changed version of a game scene, see [[k-szene-aendern]].":
        "**Změňte ho.** Skript z archivu zatím nemá soubor - první **{{FileSave}}** se zeptá, kam ho dát. Jak dodat změněnou verzi scény hry, viz [[k-szene-aendern]].",
    "The kind of file and the archive.":
        "Druh souboru a archiv.",
    "The filter.":
        "Filtr.",
    "**Open.** Double-click an entry: a script replaces the one in the current tab (behaved asks first if it has unsaved changes), a map loads, a model opens with its skin in the **{{ViewModel}}** tab.":
        "**Otevření.** Dvakrát klikněte na položku: skript nahradí ten v aktuální kartě (behaved se nejdřív zeptá, má-li neuložené změny), mapa se načte, model se otevře se svým skinem na kartě **{{ViewModel}}**.",
    "Double-click an entry to open it.":
        "Dvojklikem na položku ji otevřete.",
    "The original line over the margin.":
        "Původní řádek nad okrajem.",
    "Revert to original in the context menu.":
        "Vrátit na původní v místní nabídce.",
    "Highlight identical commands in the View menu.":
        "Zvýraznit stejné příkazy v nabídce Zobrazení.",
    "Open backup folder in the File menu.":
        "Otevřít složku záloh v nabídce Soubor.",
    "Backup.":
        "Záloha.",
    "Restore.":
        "Obnovit.",
    "**Smaller steps.** For the last few changes the undo history is quicker: right-click **Undo** ([[k-rueckgaengig]]).":
        "**Menší kroky.** Pro posledních pár změn je rychlejší historie kroků zpět: pravé tlačítko na **Zpět** ([[k-rueckgaengig]]).",
    "**The counters.** The status line at the bottom counts errors and notes. Click a counter to open the **{{MessagesTitle}}** window.":
        "**Počítadla.** Stavový řádek dole počítá chyby a poznámky. Kliknutím na počítadlo otevřete okno **{{MessagesTitle}}**.",
    "The counters at the bottom left.":
        "Počítadla vlevo dole.",
    "**Fix and check.** Double-click the line, correct the value in the editor and press **{{EditorOk}}** - the message disappears. Messages never change the script by themselves.":
        "**Opravte a zkontrolujte.** Dvakrát klikněte na řádek, opravte hodnotu v editoru a stiskněte **{{EditorOk}}** - zpráva zmizí. Zprávy nikdy samy skript nemění.",
    "The misspelt value in the editor.":
        "Chybně napsaná hodnota v editoru.",
    "**Select a shot.** In the **{{ViewMap}}** tab click the camera on the map, or a shot in the **{{CamShots}}** list on the left - the camera panel replaces the entity inspector while the camera is selected.":
        "**Vyberte záběr.** Na kartě **{{ViewMap}}** klikněte na kameru v mapě nebo na záběr v seznamu **{{CamShots}}** vlevo - panel kamery nahradí inspektor entit, dokud je kamera vybraná.",
    "The shots of the camera in the panel on the left.":
        "Záběry kamery v panelu vlevo.",
    "**Start Pilot.** Press **{{CamPilot}}** - in the camera panel or on the bar above the camera in the view. You now look through this shot.":
        "**Spusťte pilota.** Stiskněte **{{CamPilot}}** - v panelu kamery nebo na liště nad kamerou v pohledu. Teď se díváte tímto záběrem.",
    "Pilot in the camera panel.":
        "Pilotovat v panelu kamery.",
    "**Set keys along the way.** Move the playhead in the timeline below the map, fly to where the camera should be at that moment and press <kbd>Enter</kbd> or **{{CamPilotKey}}**. Repeat for more keys.":
        "**Nastavujte klíče po cestě.** Posuňte ukazatel přehrávání na časové ose pod mapou, přeleťte tam, kde má kamera v tu chvíli být, a stiskněte <kbd>Enter</kbd> nebo **{{CamPilotKey}}**. Opakujte pro další klíče.",
    "Set key while piloting, with the timeline below.":
        "Nastavit klíč při pilotování, s časovou osou dole.",
    "**Finish.** **{{CamPilotApply}}** writes the view into the script; **{{CamPilotCancel}}** (<kbd>Esc</kbd>) leaves it as it was.":
        "**Dokončení.** **{{CamPilotApply}}** zapíše pohled do skriptu; **{{CamPilotCancel}}** (<kbd>Esc</kbd>) ho nechá, jak byl.",
    "Apply and Cancel.":
        "Použít a Zrušit.",
    "**Fly to the view you want.** Hold the right mouse button in the map and fly with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> until the picture is right.":
        "**Přeleťte k požadovanému pohledu.** Podržte pravé tlačítko myši v mapě a leťte pomocí <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, dokud obraz nesedí.",
    "Your own view of the map.":
        "Váš vlastní pohled na mapu.",
    "**A new shot here.** Select the camera and press **{{CamNewShot}}**: a `camera ( MOVE )` and `camera ( PAN )` at your view are added after the selected shot.":
        "**Nový záběr zde.** Vyberte kameru a stiskněte **{{CamNewShot}}**: za vybraný záběr se přidají `camera ( MOVE )` a `camera ( PAN )` ve vašem pohledu.",
    "New camera shot here in the camera panel.":
        "Nový záběr kamery zde v panelu kamery.",
    "**Or move an existing one.** Select a shot in **{{CamShots}}** and press **{{CamSetToView}}**: its position and direction are replaced by your view - one undo step.":
        "**Nebo posuňte existující.** Vyberte záběr v **{{CamShots}}** a stiskněte **{{CamSetToView}}**: jeho poloha a směr se nahradí vaším pohledem - jeden krok zpět.",
    "Set to view.":
        "Nastavit na pohled.",
    "Through camera on the toolbar.":
        "Pohled kamerou na liště.",
    "**Place a mark.** Click the tile **{{EntKindTag}}** on the left edge of the view, then click where the camera should stand.":
        "**Umístěte značku.** Klikněte na dlaždici **{{EntKindTag}}** u levého okraje pohledu a pak klikněte tam, kde má kamera stát.",
    "The tile for a camera mark.":
        "Dlaždice pro značku kamery.",
    "**Camera to the mark.** Right-click the mark in the entity list → **{{EntityCamToTag}}**. Two camera commands are inserted that take the mark's place and angles (`tag`).":
        "**Kamera ke značce.** Pravé tlačítko na značce v seznamu entit → **{{EntityCamToTag}}**. Vloží se dva příkazy kamery, které berou místo a úhly značky (`tag`).",
    "**Move the mark.** Select it and drag it in the view, or change its **{{EntOrigin}}** in the inspector: the shot moves with it.":
        "**Posuňte značku.** Vyberte ji a táhněte v pohledu, nebo změňte její **{{EntOrigin}}** v inspektoru: záběr se posune s ní.",
    "A selected entity with its arrows.":
        "Vybraná entita se svými šipkami.",
    "**Turn it.** The mark's facing is the camera's direction: turn it with the ring, or type **{{EntAngle}}** in the inspector.":
        "**Otočte ji.** Natočení značky je směr kamery: otočte ji kruhem, nebo zadejte **{{EntAngle}}** v inspektoru.",
    "The first field of camera chooses the command.":
        "První pole camera volí příkaz.",
    "**Zoom.** Choose `ZOOM`: (1) the field of view - 80 is normal, smaller is closer - and (2) the time in milliseconds to get there.":
        "**Přiblížení.** Zvolte `ZOOM`: (1) zorné pole - 80 je normální, menší je blíž - a (2) čas v milisekundách, za který se ho dosáhne.",
    "camera ( ZOOM ): field of view and time.":
        "camera ( ZOOM ): zorné pole a čas.",
    "**Fade.** Choose `FADE`: (1) start colour and (2) its opacity, (3) end colour and (4) its opacity - all from 0 to 1 - and (5) the time. `0 0 0`, `0` to `0 0 0`, `1` fades to black.":
        "**Zatmívání.** Zvolte `FADE`: (1) počáteční barva a (2) její krytí, (3) koncová barva a (4) její krytí - vše od 0 do 1 - a (5) čas. `0 0 0`, `0` na `0 0 0`, `1` zatmí do černa.",
    "camera ( FADE ): five values.":
        "camera ( FADE ): pět hodnot.",
    "**Shake.** Choose `SHAKE`: (1) the strength from 0 to 16 and (2) the duration in milliseconds.":
        "**Otřesy.** Zvolte `SHAKE`: (1) síla od 0 do 16 a (2) doba v milisekundách.",
    "camera ( SHAKE ): strength and duration.":
        "camera ( SHAKE ): síla a doba.",
    "The rows of the timeline, among them Zoom, Fade and Shake.":
        "Řádky časové osy, mezi nimi Přiblížení, Prolínání a Otřesy.",
    "**Play it.** Press ▶ in the timeline below the map. Switch **{{TlAudio}}** on to hear the dialogue you are matching the cuts against.":
        "**Přehrajte to.** Stiskněte ▶ na časové ose pod mapou. Zapněte **{{TlAudio}}** a uslyšíte dialog, podle kterého ladíte střihy.",
    "To start, play - and Sound.":
        "Na začátek, přehrát - a Zvuk.",
    "The right edge of a block.":
        "Pravý okraj bloku.",
    "The time display.":
        "Zobrazení času.",
    "**Check the script.** Every drag changes a value in the script (a wait, a time). Click a block: its line is selected in the script column on the right.":
        "**Zkontrolujte skript.** Každé tažení změní hodnotu ve skriptu (čekání, čas). Klikněte na blok: jeho řádek se vybere ve sloupci skriptu vpravo.",
    "A clicked block and its line in the script.":
        "Kliknutý blok a jeho řádek ve skriptu.",
    "**Through camera.** **{{MapThroughCam}}** on the map toolbar shows the picture of the selected shot - or, with nothing selected, what the camera sees at the playhead.":
        "**Pohled kamerou.** **{{MapThroughCam}}** na liště mapy ukáže obraz vybraného záběru - nebo, když není nic vybráno, co kamera vidí u ukazatele přehrávání.",
    "Through camera.":
        "Pohled kamerou.",
    "**The small preview.** **{{CamPreview}}** on the camera bar puts a small picture of the camera's view in the corner - you keep flying freely. Click the picture to look through it.":
        "**Malý náhled.** **{{CamPreview}}** na liště kamery umístí malý obrázek pohledu kamery do rohu - dál volně létáte. Kliknutím na obrázek se jím podíváte.",
    "The camera preview.":
        "Náhled kamery.",
    "Follow.":
        "Sledovat.",
    "The speed list.":
        "Seznam rychlostí.",
    "SET_CAMERA_GROUP in Kyle's block.":
        "SET_CAMERA_GROUP v Kylově bloku.",
    "**Follow.** After the camera shot insert `camera` and choose `FOLLOW`: (1) the group `hero`, (2) a turning speed, for example `100`, and (3) `1` to blend into it.":
        "**Sledování.** Za záběr kamery vložte `camera` a zvolte `FOLLOW`: (1) skupinu `hero`, (2) rychlost otáčení, například `100`, a (3) `1` pro plynulý přechod.",
    "camera ( FOLLOW ): group, speed, blend.":
        "camera ( FOLLOW ): skupina, rychlost, přechod.",
    "**Keep a distance.** `camera ( DISTANCE )` with (1) `200` and (2) `1` keeps the camera 200 units from the group, moving with it.":
        "**Držet odstup.** `camera ( DISTANCE )` s (1) `200` a (2) `1` drží kameru 200 jednotek od skupiny a pohybuje se s ní.",
    "camera ( DISTANCE ).":
        "camera ( DISTANCE ).",
    "**Watch it.** Turn on **{{MapThroughCam}}** and play the timeline: the view keeps turning towards the figure.":
        "**Sledujte to.** Zapněte **{{MapThroughCam}}** a přehrajte časovou osu: pohled se stále otáčí k postavě.",
    "**Fade it.** `SET_VIDEO_FADE_IN` and `SET_VIDEO_FADE_OUT` set to `true` before the video let it fade in and out.":
        "**Prolínání.** `SET_VIDEO_FADE_IN` a `SET_VIDEO_FADE_OUT` nastavené na `true` před videem ho nechají roztmívat a zatmívat.",
    "The two fade lines before the video.":
        "Dva řádky prolínání před videem.",
    "Video, fades and a line of text in the tree.":
        "Video, prolínání a řádek textu ve stromu.",
    "The status line confirms the stored view.":
        "Stavový řádek potvrdí uložený pohled.",
    "Fly to shot.":
        "Přeletět k záběru.",
    "**Jump to a line, change a duration.** Click a block to select its line; drag its right edge to change the duration - just like in the timeline below the map.":
        "**Skok na řádek, změna délky.** Kliknutím na blok vyberete jeho řádek; tažením pravého okraje změníte délku - stejně jako na časové ose pod mapou.",
    "Blocks have a right edge you can drag.":
        "Bloky mají pravý okraj, který lze táhnout.",
    "**Move through time.** Drag in the ruler at the top; **{{TracksPlay}}** plays.":
        "**Pohyb v čase.** Táhněte v pravítku nahoře; **{{TracksPlay}}** přehrává.",
    "The ruler and Play at the top of the Tracks view.":
        "Pravítko a Přehrát nahoře v zobrazení Stopy.",
    "Character rows and Waits.":
        "Řádky postav a Čekání.",
    "The camera rows.":
        "Řádky kamery.",
    "Kyle and his path.":
        "Kyle a jeho dráha.",
    "**Right-click the walk target** at the end of the path. The menu offers **{{BahnGehen}}**, **{{BahnRennen}}** and **{{BahnRueckwaerts}}**.":
        "**Klikněte pravým tlačítkem na cíl chůze** na konci dráhy. Nabídka nabízí **{{BahnGehen}}**, **{{BahnRennen}}** a **{{BahnRueckwaerts}}**.",
    "**Pick one.** behaved writes `SET_WALKING` / `SET_RUNNING` before this walk - here **{{BahnRennen}}** added `set ( SET_RUNNING, true )`. It counts from this walk on.":
        "**Vyberte jedno.** behaved zapíše před tuto chůzi `SET_WALKING` / `SET_RUNNING` - zde **{{BahnRennen}}** přidal `set ( SET_RUNNING, true )`. Platí od této chůze dál.",
    "The new SET_RUNNING line.":
        "Nový řádek SET_RUNNING.",
    "**Watch.** Play the timeline: the figure runs, and its walk block is shorter.":
        "**Sledujte.** Přehrajte časovou osu: postava běží a její blok chůze je kratší.",
    "The shorter walk in the timeline.":
        "Kratší chůze na časové ose.",
    "Look at while walking.":
        "Dívat se na cíl při chůzi.",
    "**Standing.** Right-click the target in the entity list → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) or **{{EntityAsLook}}** (`SET_LOOK_TARGET`). The command goes into the script after the selected line.":
        "**Ve stoje.** Pravé tlačítko na cíli v seznamu entit → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) nebo **{{EntityAsLook}}** (`SET_LOOK_TARGET`). Příkaz se vloží do skriptu za vybraný řádek.",
    "The context menu of an entity in the list.":
        "Místní nabídka entity v seznamu.",
    "Face the walking direction.":
        "Dívat se ve směru chůze.",
    "**Go to the moment.** Move the playhead in the timeline to where the figure should face another way. The time display shows where you are.":
        "**Přejděte na okamžik.** Posuňte ukazatel přehrávání na časové ose tam, kde se má postava dívat jinam. Zobrazení času ukazuje, kde jste.",
    "The playhead and the time display.":
        "Ukazatel přehrávání a zobrazení času.",
    "Rotate on the gizmo bar, the figure selected.":
        "Otočit na liště gizma, postava vybraná.",
    "**Turn.** Drag the ring. When you let go, behaved writes the facing at this time into the script (`SET_DYAW` for an NPC, which then turns with its turn rate as in the game; `SET_ANGLES` for the player); the status line says *Facing … degrees at … s*.":
        "**Otočte.** Táhněte kruh. Když pustíte, behaved zapíše natočení v tomto čase do skriptu (`SET_DYAW` pro NPC, které se pak otáčí svou rychlostí otáčení jako ve hře; `SET_ANGLES` pro hráče); stavový řádek hlásí *Natočení … stupňů v … s*.",
    "The ring around the standing figure.":
        "Kruh kolem stojící postavy.",
    "The ring on a walking figure.":
        "Kruh na jdoucí postavě.",
    "The NPC browser.":
        "Prohlížeč NPC.",
    "Loop in the playback bar.":
        "Smyčka v liště přehrávání.",
    "Into script.":
        "Do skriptu.",
    "The figure in the scene.":
        "Postava ve scéně.",
    "**Insert sound.** Inside the figure's `affect` block insert `sound`. In the editor choose the channel `CHAN_VOICE` and pick the file with **{{EditorBrowse}}**; **{{EditorPlay}}** plays it.":
        "**Vložte sound.** Do bloku `affect` postavy vložte `sound`. V editoru zvolte kanál `CHAN_VOICE` a soubor vyberte pomocí **{{EditorBrowse}}**; **{{EditorPlay}}** ho přehraje.",
    "The sound command: browse and play.":
        "Příkaz zvuku: procházet a přehrát.",
    "Sound in the timeline.":
        "Zvuk na časové ose.",
    "A spawn script with standOnly.":
        "Spawn skript se standOnly.",
    "The figures in the scene.":
        "Postavy ve scéně.",
    "camera ( DISABLE ).":
        "camera ( DISABLE ).",
    "**Attack.** Right after it, an `affect` block for the enemy with `SET_IGNOREENEMIES false`, `SET_LOOK_FOR_ENEMIES true` and `SET_ENEMY player`. The macro `default` resets all behaviour in one line.":
        "**Útok.** Hned poté blok `affect` pro nepřítele s `SET_IGNOREENEMIES false`, `SET_LOOK_FOR_ENEMIES true` a `SET_ENEMY player`. Makro `default` obnoví veškeré chování na jednom řádku.",
    "The three lines that start the fight.":
        "Tři řádky, které začnou boj.",
    "run starts another script.":
        "run spustí jiný skript.",
    "Lock the player.":
        "Zamknout hráče.",
    "The scene, seen by the player.":
        "Scéna viděná hráčem.",
    "Free the player again.":
        "Znovu uvolnit hráče.",
    "SET_WEAPON WP_SABER.":
        "SET_WEAPON WP_SABER.",
    "SET_SABERACTIVE true.":
        "SET_SABERACTIVE true.",
    "**See it.** Play the timeline: Kyle ignites his blade.":
        "**Podívejte se.** Přehrajte časovou osu: Kyle zapne čepel.",
    "Kyle with his blade ignited.":
        "Kyle se zapnutou čepelí.",
    "SET_SABERACTIVE false.":
        "SET_SABERACTIVE false.",
    "The arrows of a selected entity.":
        "Šipky vybrané entity.",
    "**Copy.** <kbd>Alt</kbd>+drag makes a copy and moves it - or press **{{EntDuplicate}}** in the inspector.":
        "**Kopie.** <kbd>Alt</kbd>+tažení vytvoří kopii a posune ji - nebo stiskněte **{{EntDuplicate}}** v inspektoru.",
    "Duplicate in the inspector.":
        "Duplikovat v inspektoru.",
    "**Onto the floor.** **{{EntDropFloor}}** puts it exactly on the floor below - the inspector shows how high the feet are.":
        "**Na podlahu.** **{{EntDropFloor}}** ji položí přesně na podlahu pod ní - inspektor ukazuje, jak vysoko jsou nohy.",
    "Onto floor.":
        "Na podlahu.",
    "**Search.** Type a name or class into the search box above the entity list, for example `nav` or `kyle`.":
        "**Hledání.** Do vyhledávacího pole nad seznamem entit napište název nebo třídu, například `nav` nebo `kyle`.",
    "The search box of the entity list.":
        "Vyhledávací pole seznamu entit.",
    "A row of the entity list.":
        "Řádek seznamu entit.",
    "Names on the map.":
        "Názvy v mapě.",
    "**Select the entity.** Click it in the view or in the list. The inspector under the list shows its class in colour at the top; hover over it for a description from the game code.":
        "**Vyberte entitu.** Klikněte na ni v pohledu nebo v seznamu. Inspektor pod seznamem ukazuje nahoře barevně její třídu; najeďte na ni myší a uvidíte popis z kódu hry.",
    "The class at the top of the inspector.":
        "Třída nahoře v inspektoru.",
    "**Keys.** Under **{{EntSecFields}}**, change values or add one with **{{EntAddKey}}**. Script keys have **{{EntOpenScript}}** next to them.":
        "**Klíče.** Pod **{{EntSecFields}}** měňte hodnoty nebo přidejte novou pomocí **{{EntAddKey}}**. Skriptové klíče mají vedle sebe **{{EntOpenScript}}**.",
    "Fields and + Key.":
        "Pole a + Klíč.",
    "All keys (raw).":
        "Všechny klíče (surově).",
    "The cut height.":
        "Výška řezu.",
    "A selected figure in the top view.":
        "Vybraná postava v pohledu shora.",
    "**Switch things off.** **{{MapEffects}}**, **{{MapGlow}}**, **{{MapDynLights}}**, **{{MapFog}}** and **{{MapSky}}** on the toolbar switch those parts off. Without the sky you can look into the map from outside.":
        "**Vypněte věci.** **{{MapEffects}}**, **{{MapGlow}}**, **{{MapDynLights}}**, **{{MapFog}}** a **{{MapSky}}** na liště tyto části vypnou. Bez oblohy se můžete dívat do mapy zvenku.",
    "The switches on the map toolbar.":
        "Přepínače na liště mapy.",
    "Highlight this script's entities.":
        "Zvýraznit entity tohoto skriptu.",
    "The tile for a script runner.":
        "Dlaždice pro spouštěč skriptu.",
    "Open next to usescript.":
        "Otevřít vedle usescript.",
    "**Find a trigger.** Triggers are brushes of the map: type `trigger` into the search box of the entity list and pick one where the player passes. Double-click flies there.":
        "**Najděte trigger.** Triggery jsou brushe mapy: napište `trigger` do vyhledávacího pole seznamu entit a vyberte takový, kudy hráč prochází. Dvojklik tam přeletí.",
    "Searching for triggers.":
        "Hledání triggerů.",
    "**Point it at the runner.** In the trigger's inspector set its key `target` to the runner's name.":
        "**Namiřte ho na spouštěč.** V inspektoru triggeru nastavte jeho klíč `target` na název spouštěče.",
    "**Once or every time.** A runner runs once. Type `count` into the new-key field, press **{{EntAddKey}}** and set it to `-1` to run it every time. For only-the-first-time logic see [[k-variablen]].":
        "**Jednou nebo pokaždé.** Spouštěč se spustí jednou. Napište `count` do pole nového klíče, stiskněte **{{EntAddKey}}** a nastavte ho na `-1`, aby se spouštěl pokaždé. Logiku jen napoprvé viz [[k-variablen]].",
    "A new key in the inspector.":
        "Nový klíč v inspektoru.",
    "**Find the door.** Doors are in the group **Movable (func_*)** of the entity list. Open the group and click one.":
        "**Najděte dveře.** Dveře jsou ve skupině **Pohyblivé (func_*)** seznamu entit. Otevřete skupinu a klikněte na jedny.",
    "The group of movable entities.":
        "Skupina pohyblivých entit.",
    "**Its name.** The door needs a name (`targetname`). A door with a name waits to be used instead of opening when someone walks up - which is what a scripted door should do.":
        "**Jejich název.** Dveře potřebují název (`targetname`). Dveře s názvem čekají na použití, místo aby se otevřely, když k nim někdo přijde - přesně to mají skriptované dveře dělat.",
    "The door's name in the inspector.":
        "Název dveří v inspektoru.",
    "**Insert use.** Select the line in the script after which the door should open, then right-click the door in the entity list → **{{EntityUse}}**.":
        "**Vložte use.** Vyberte ve skriptu řádek, po kterém se mají dveře otevřít, a pak klikněte pravým tlačítkem na dveře v seznamu entit → **{{EntityUse}}**.",
    "Insert use in the context menu.":
        "Vložit use v místní nabídce.",
    "**The result.** `use ( \"bldg2_ext_door\" )` is in the script. The door plays its own sounds; play the timeline to check the moment.":
        "**Výsledek.** `use ( \"bldg2_ext_door\" )` je ve skriptu. Dveře přehrávají vlastní zvuky; přehrajte časovou osu a zkontrolujte okamžik.",
    "The use line.":
        "Řádek use.",
    "**Place it.** **{{EntNew}}** above the entity list → **{{EntKindLevel}}**.":
        "**Umístěte ji.** **{{EntNew}}** nad seznamem entit → **{{EntKindLevel}}**.",
    "Level change in the + Entity menu.":
        "Změna úrovně v nabídce + Entita.",
    "**Set it up.** In the inspector set `mapname` to the next map (for example `taspir2`) and give it a **{{EntName}}** such as `next_map`.":
        "**Nastavte ji.** V inspektoru nastavte `mapname` na další mapu (například `taspir2`) a dejte jí **{{EntName}}**, například `next_map`.",
    "**Use it.** At the end of the script: `use ( \"next_map\" )` - right-click the entity → **{{EntityUse}}** writes it.":
        "**Použijte ji.** Na konci skriptu: `use ( \"next_map\" )` - pravé tlačítko na entitě → **{{EntityUse}}** ho zapíše.",
    "use ( \"next_map\" ).":
        "use ( \"next_map\" ).",
    "The checks of Save mission.":
        "Kontroly Uložit misi.",
    "**An effect.** Click the tile **{{EntKindFx}}** and click where it should appear.":
        "**Efekt.** Klikněte na dlaždici **{{EntKindFx}}** a klikněte tam, kde se má objevit.",
    "The tile for an effect.":
        "Dlaždice pro efekt.",
    "**Set it up.** In the inspector set `fxFile` to an effect of the game (a path under `effects/`, for example `env/fire`), give it a name and tick `STARTOFF` so it waits.":
        "**Nastavte ho.** V inspektoru nastavte `fxFile` na efekt hry (cesta pod `effects/`, například `env/fire`), dejte mu název a zaškrtněte `STARTOFF`, aby čekal.",
    "A fire on the landing pad.":
        "Oheň na přistávací plošině.",
    "**Start it.** In the script: `use ( \"fire1\" )` at the right moment.":
        "**Spusťte ho.** Ve skriptu: `use ( \"fire1\" )` ve správnou chvíli.",
    "use starts the effect.":
        "use spustí efekt.",
    "Music in the + Entity menu.":
        "Hudba v nabídce + Entita.",
    "SET_MUSIC_STATE.":
        "SET_MUSIC_STATE.",
    "The group with Move.":
        "Skupina s Posunout.",
    "The group with Rotate.":
        "Skupina s Otočit.",
    "Duplicate copies every selected entity.":
        "Duplikovat zkopíruje každou vybranou entitu.",
    "**Two names.** A character spawner has its **{{EntName}}** - the `NPC_targetname`, the name of the figure that `affect` uses - and may have a `targetname`, the name of the spawner itself.":
        "**Dva názvy.** Spawner postavy má svůj **{{EntName}}** - `NPC_targetname`, jméno postavy, které používá `affect` - a může mít `targetname`, název samotného spawneru.",
    "Name and NPC_type of a spawner.":
        "Název a NPC_type spawneru.",
    "use calls the spawner.":
        "use zavolá spawner.",
    "The spawnflags.":
        "Spawnflags.",
    "Open .ent in the Maps & files menu.":
        "Otevřít .ent v nabídce Mapy a soubory.",
    "The entity list.":
        "Seznam entit.",
    "The files of the package, among them maps/taspir_tut.ent.":
        "Soubory balíčku, mezi nimi maps/taspir_tut.ent.",
    "**Select the door.** Click it in the view or the list - doors and platforms are `func_` entities. In the inspector press **{{EntAnimate}}**.":
        "**Vyberte dveře.** Klikněte na ně v pohledu nebo v seznamu - dveře a plošiny jsou entity `func_`. V inspektoru stiskněte **{{EntAnimate}}**.",
    "Animate in the inspector.":
        "Animovat v inspektoru.",
    "Script (move).":
        "Skript (move).",
    "Key at playhead.":
        "Klíč u ukazatele přehrávání.",
    "The second key in the table; the door has moved up.":
        "Druhý klíč v tabulce; dveře se posunuly nahoru.",
    "Linear.":
        "Lineární.",
    "Animate.":
        "Animovat.",
    "**Edit.** In ROFF mode the window lists the keys of the file. A ROFF of the game is first turned into keys with **{{AnimRoffEdit}}** - the game file stays untouched, you edit a copy with its own name.":
        "**Úprava.** V režimu ROFF okno vypíše klíče souboru. ROFF hry se nejdřív převede na klíče pomocí **{{AnimRoffEdit}}** - soubor hry zůstane nedotčen, upravujete kopii s vlastním názvem.",
    "The flight with its keys.":
        "Let s jeho klíči.",
    "The files of the package, among them the .rof.":
        "Soubory balíčku, mezi nimi .rof.",
    "**Open the Graph.** Above the script column, switch from **{{FlowClassic}}** to **{{FlowGraph}}**. Every entity and script of the mission is a node; the arrows show what starts what.":
        "**Otevřete Graf.** Nad sloupcem skriptu přepněte z **{{FlowClassic}}** na **{{FlowGraph}}**. Každá entita a skript mise je uzel; šipky ukazují, co co spouští.",
    "**Drag a wire.** Drag from the dot on the right of a character node to a script node. A menu asks **{{GraphConnectAs}}**: `spawnscript` (when it appears), `deathscript`, `usescript` and more - behaved writes the key into the entity.":
        "**Táhněte spoj.** Táhněte od tečky vpravo u uzlu postavy k uzlu skriptu. Nabídka se zeptá **{{GraphConnectAs}}**: `spawnscript` (když se objeví), `deathscript`, `usescript` a další - behaved zapíše klíč do entity.",
    "Character nodes with their dots on the right.":
        "Uzly postav s tečkami vpravo.",
    "**Check the key.** Select the character: the inspector shows the new key, with **{{EntOpenScript}}** next to it.":
        "**Zkontrolujte klíč.** Vyberte postavu: inspektor ukazuje nový klíč s **{{EntOpenScript}}** vedle něj.",
    "The script keys of a character.":
        "Skriptové klíče postavy.",
    "**Open it again.** **{{MenuFile}} → {{RecentMissions}}**, or the list on the start page of the **{{ViewMap}}** tab, opens map, entities and every script with one click.":
        "**Otevřete ji znovu.** **{{MenuFile}} → {{RecentMissions}}**, nebo seznam na úvodní stránce karty **{{ViewMap}}**, otevře mapu, entity a každý skript jedním kliknutím.",
    "Recent missions.":
        "Poslední mise.",
    "**Work.** Change scripts and entities. A dot next to the mission name in the map toolbar means unsaved changes.":
        "**Práce.** Měňte skripty a entity. Tečka vedle názvu mise na liště mapy znamená neuložené změny.",
    "The mission name with its dot.":
        "Název mise s tečkou.",
    "**Save mission.** **{{MissionSave}}** shows every file it will pack and checks the mission: errors in red must be fixed, warnings in orange may stay.":
        "**Uložení mise.** **{{MissionSave}}** ukáže každý soubor, který zabalí, a misi zkontroluje: chyby červeně se musí opravit, varování oranžově mohou zůstat.",
    "**Open the window.** **{{WegTitel}}** on the map toolbar.":
        "**Otevřete okno.** **{{WegTitel}}** na liště mapy.",
    "NPC waypoints on the toolbar.":
        "Navigační body NPC na liště.",
    "**Show and create.** **{{WegZeigen}}** shows the network - green by hand, blue automatic, orange jumps. **{{WegErzeugen}}** spreads waypoints over the walkable floor; the spacing slider sets how dense.":
        "**Zobrazení a vytvoření.** **{{WegZeigen}}** ukáže síť - zeleně ručně, modře automaticky, oranžově skoky. **{{WegErzeugen}}** rozprostře navigační body po pochozí podlaze; posuvník rozestupu nastavuje hustotu.",
    "Check network.":
        "Zkontrolovat síť.",
    "Place waypoints and Connect.":
        "Umístit navigační body a Spojit.",
    "**Switch to {{ViewModel}}** in the mode bar on the left.":
        "**Přepněte na {{ViewModel}}** v liště režimů vlevo.",
    "Model in the mode bar.":
        "Model v liště režimů.",
    "Skin, Bones and Textures.":
        "Skin, Kosti a Textury.",
    "**Find the scene.** In the **{{ViewMap}}** tab, **Scripts (N)** on the map toolbar lists the scripts the map uses - pick the intro. The timeline shows its camera.":
        "**Najděte scénu.** Na kartě **{{ViewMap}}** vypíše **Skripty (N)** na liště mapy skripty, které mapa používá - vyberte úvod. Časová osa ukazuje jeho kameru.",
    "The scripts of the map.":
        "Skripty mapy.",
    "**Change a shot.** Select a shot in **{{CamShots}}**, fly to a better view and press **{{CamSetToView}}** - or change a time in the timeline.":
        "**Změňte záběr.** Vyberte záběr v **{{CamShots}}**, přeleťte k lepšímu pohledu a stiskněte **{{CamSetToView}}** - nebo změňte čas na časové ose.",
    "Save mission.":
        "Uložit misi.",
    "**Undo it.** Delete the `zz_` file from the game folder (the message after saving has **{{ToastExplorer}}**) and the game plays its original again - the game files are never touched.":
        "**Vraťte to.** Smažte soubor `zz_` ze složky hry (zpráva po uložení má **{{ToastExplorer}}**) a hra znovu hraje svůj originál - soubory hry se nikdy nemění.",
    "Show in Explorer opens the folder.":
        "Zobrazit v Průzkumníkovi otevře složku.",
    "**Switch.** **{{MenuView}} → {{ViewTarget}}** → **{{TargetMp}}**. An **MP** badge appears in the status line.":
        "**Přepnutí.** **{{MenuView}} → {{ViewTarget}}** → **{{TargetMp}}**. Ve stavovém řádku se objeví odznak **MP**.",
    "Target game: multiplayer.":
        "Cílová hra: více hráčů.",
    "**Switch back** to **{{TargetSp}}** when you are done; the script itself is not changed.":
        "**Přepněte zpět** na **{{TargetSp}}**, až budete hotovi; samotný skript se nemění.",
    "Target game: single player.":
        "Cílová hra: jeden hráč.",
    "**Theme.** **{{MenuView}} → {{ViewTheme}}**: six themes, among them BehavEd Classic and High Contrast.":
        "**Motiv.** **{{MenuView}} → {{ViewTheme}}**: šest motivů, mezi nimi BehavEd klasický a Vysoký kontrast.",
    "Theme in the View menu.":
        "Motiv v nabídce Zobrazení.",
    "**Language.** **{{MenuView}} → {{ViewLanguage}}**: English, Deutsch, 中文, 日本語, Español, Čeština.":
        "**Jazyk.** **{{MenuView}} → {{ViewLanguage}}**: English, Deutsch, 中文, 日本語, Español, Čeština.",
    "Language in the View menu.":
        "Jazyk v nabídce Zobrazení.",
    "Interface Scale in the View menu.":
        "Měřítko rozhraní v nabídce Zobrazení.",
    "**Columns.** Drag the dividers between the columns; each mode keeps its own widths. **{{MenuDebug}} → {{DebugReset}}** brings them back.":
        "**Sloupce.** Táhněte oddělovače mezi sloupci; každý režim si pamatuje vlastní šířky. **{{MenuDebug}} → {{DebugReset}}** je vrátí.",
    "Restore default layout in the Debug menu.":
        "Obnovit výchozí rozvržení v nabídce Ladění.",
    "**The guide.** <kbd>F1</kbd> opens this guide - also **{{MenuHelp}} → {{HelpGuide}}**.":
        "**Příručka.** <kbd>F1</kbd> otevře tuto příručku - také **{{MenuHelp}} → {{HelpGuide}}**.",
    "Guide and tutorials in the Help menu.":
        "Příručka a tutoriály v nabídce Nápověda.",
    "**Give the figure a deathscript.** Select the enemy. In the inspector add the key `deathscript` with a script name of your mission, for example `taspir_tut/rosh_dies` - or connect it in the **{{FlowGraph}}** view as `deathscript`.":
        "**Dejte postavě deathscript.** Vyberte nepřítele. V inspektoru přidejte klíč `deathscript` s názvem skriptu vaší mise, například `taspir_tut/rosh_dies` - nebo ho spojte v zobrazení **{{FlowGraph}}** jako `deathscript`.",
    "**Create the script.** Press **{{EntOpenScript}}** next to the key: the script is created in the mission folder and opened in a tab.":
        "**Vytvořte skript.** Stiskněte **{{EntOpenScript}}** vedle klíče: skript se vytvoří ve složce mise a otevře na kartě.",
    "Open next to deathscript.":
        "Otevřít vedle deathscript.",
    "**Other moments.** The same works with `painscript` (hit), `awakescript` (notices the player), `angerscript` (starts fighting), `fleescript` and `spawnscript` - all listed in the inspector.":
        "**Jiné okamžiky.** Totéž funguje s `painscript` (zásah), `awakescript` (všimne si hráče), `angerscript` (začne bojovat), `fleescript` a `spawnscript` - všechny jsou vypsány v inspektoru.",
    "SET_PLAYER_USABLE in the spawn script.":
        "SET_PLAYER_USABLE ve spawn skriptu.",
    "A voice line.":
        "Hlasová replika.",
    "SET_PLAYER_USABLE false at the end.":
        "SET_PLAYER_USABLE false na konci.",
    "run and waitsignal in the intro, signal in talk.":
        "run a waitsignal v úvodu, signal v talk.",
    "**Hand over.** Where the intro must wait for the other part, insert `waitsignal ( \"talk_done\" )`; at the end of the other script `signal ( \"talk_done\" )` - see [[k-signale]].":
        "**Předání.** Kde musí úvod počkat na druhou část, vložte `waitsignal ( \"talk_done\" )`; na konec druhého skriptu `signal ( \"talk_done\" )` - viz [[k-signale]].",
    "The same signal name on both sides.":
        "Stejný název signálu na obou stranách.",
    "**S001 (warning) - a script is missing.** An entity names a script that does not exist yet. Select the entity and press **{{EntOpenScript}}** next to the key: the script is created.":
        "**S001 (varování) - chybí skript.** Entita jmenuje skript, který zatím neexistuje. Vyberte entitu a stiskněte **{{EntOpenScript}}** vedle klíče: skript se vytvoří.",
    "Open next to a script key.":
        "Otevřít vedle skriptového klíče.",
    "Compile shows the wrong line.":
        "Kompilace ukáže špatný řádek.",
    "The tile for a player start.":
        "Dlaždice pro start hráče.",
    "**S005 (warning) - a target points to nothing.** A `target` names an entity that does not exist - often a leftover of the original map, like the one in the first picture.":
        "**S005 (varování) - cíl na nic neukazuje.** `target` jmenuje entitu, která neexistuje - často zbytek původní mapy, jako ten na prvním obrázku.",
    "S005 at the bottom of the list.":
        "S005 dole v seznamu.",
    "The mission name is chosen when the mission is created.":
        "Název mise se volí při vytvoření mise.",
    "**Place the round.** In the **{{ViewMap}}** tab click the tile **{{EntKindNav}}** and place three walk targets (<kbd>Shift</kbd> places several); name them `round1`, `round2`, `round3`.":
        "**Rozmístěte pochůzku.** Na kartě **{{ViewMap}}** klikněte na dlaždici **{{EntKindNav}}** a umístěte tři cíle chůze (<kbd>Shift</kbd> umístí několik); pojmenujte je `round1`, `round2`, `round3`.",
    "The tile for walk targets.":
        "Dlaždice pro cíle chůze.",
    "**The guard.** Place a character, name it `guard`, and give it an `affect ( \"guard\", FLUSH )` block. Inside, insert the macro `walkOnly` first, then `loop` with `-1` - forever.":
        "**Strážný.** Umístěte postavu, pojmenujte ji `guard` a dejte jí blok `affect ( \"guard\", FLUSH )`. Dovnitř nejdřív vložte makro `walkOnly`, pak `loop` s `-1` - navždy.",
    "walkOnly and the loop in the guard's block.":
        "walkOnly a smyčka v bloku strážného.",
    "**One leg per point.** Inside the loop: a `task` with `set ( SET_NAVGOAL, round1 )`, then `dowait` for it, then `wait ( 1000 )`. Copy these lines for each point and change the name.":
        "**Jeden úsek na bod.** Uvnitř smyčky: `task` s `set ( SET_NAVGOAL, round1 )`, pak `dowait` na něj, pak `wait ( 1000 )`. Tyto řádky zkopírujte pro každý bod a změňte název.",
    "A task, dowait and wait for each point.":
        "task, dowait a wait pro každý bod.",
    "**Watch.** Play the timeline - the guard walks the round.":
        "**Sledujte.** Přehrajte časovou osu - strážný obchází pochůzku.",
    "**Switch skins.** In the **{{ViewModel}}** tab the **{{ModelSkin}}** list in the playback bar shows the skins that come with the model; **{{OpenSkin}}** loads another one.":
        "**Přepínání skinů.** Na kartě **{{ViewModel}}** ukazuje seznam **{{ModelSkin}}** v liště přehrávání skiny, které k modelu patří; **{{OpenSkin}}** načte jiný.",
    "**Surfaces.** The counter next to it shows how many surfaces the skin draws; click it to list them all in the detail log.":
        "**Povrchy.** Počítadlo vedle ukazuje, kolik povrchů skin kreslí; kliknutím je všechny vypíšete do podrobného protokolu.",
    "The surface counter.":
        "Počítadlo povrchů.",
    "Show caps.":
        "Zobrazit záslepky.",
    "Load .gla.":
        "Načíst .gla.",
    "The runner of the original, found with the search box.":
        "Spouštěč originálu, nalezený vyhledávacím polem.",
    "The tile for a character.":
        "Dlaždice pro postavu.",
    "Rosh's name and NPC_type.":
        "Roshovo jméno a NPC_type.",
    "Rosh's block selected, with Copy and Paste.":
        "Roshův blok vybraný, s Kopírovat a Vložit.",
    "The first shot selected, with Set to view.":
        "První záběr vybraný, s Nastavit na pohled.",
    "The third shot selected in the camera panel - its Travel time is below the list.":
        "Třetí záběr vybraný v panelu kamery - jeho doba přesunu je pod seznamem.",
    "The two PAN commands that follow the ship.":
        "Dva příkazy PAN, které sledují loď.",
    "camera ( FADE ): from transparent to black.":
        "camera ( FADE ): z průhledné do černé.",
    "Kyle's fields and + Key.":
        "Kylova pole a + Klíč.",
    "Open next to spawnscript.":
        "Otevřít vedle spawnscript.",
    "The window names the .pk3 it writes into the game folder.":
        "Okno jmenuje .pk3, který zapíše do složky hry.",
    "**Switch to Graph.** Click **{{FlowGraph}}** above the script area. The graph shows the mission: the player start, the scripts and the entities that start them.":
        "**Přepněte na Graf.** Klikněte na **{{FlowGraph}}** nad oblastí skriptu. Graf ukazuje misi: start hráče, skripty a entity, které je spouštějí.",
    "The view switch above the script area: Graph.":
        "Přepínač pohledu nad oblastí skriptu: Graf.",
    "**Drag affect onto free space.** Drag `affect` from the **{{GroupEvents}}** list (1) onto an empty spot of the graph (2). A new script is created in the mission folder, with `affect` as its first command, and opens in its own tab.":
        "**Přetáhněte affect na volné místo.** Přetáhněte `affect` ze seznamu **{{GroupEvents}}** (1) na prázdné místo grafu (2). Ve složce mise se vytvoří nový skript s `affect` jako prvním příkazem a otevře se na vlastní kartě.",
    "1: affect in the Events list. 2: free space in the graph.":
        "1: affect v seznamu Události. 2: volné místo v grafu.",
    "**The new node.** The new script appears as a node where you dropped it, and it is selected. On the right, **{{GraphCommands}}** shows its one line: `affect`.":
        "**Nový uzel.** Nový skript se objeví jako uzel tam, kde jste ho pustili, a je vybraný. Vpravo ukazuje **{{GraphCommands}}** jeho jediný řádek: `affect`.",
    "1: the new script node. 2: its commands on the right.":
        "1: nový uzel skriptu. 2: jeho příkazy vpravo.",
    "**Name the figure.** Double-click the `affect` line on the right. The **{{EditorTitle}}** opens - the same as in Classic. Type the name of the figure into the first field and press **{{EditorOk}}**.":
        "**Pojmenujte postavu.** Dvakrát klikněte na řádek `affect` vpravo. Otevře se **{{EditorTitle}}** - stejný jako v Klasickém. Do prvního pole napište jméno postavy a stiskněte **{{EditorOk}}**.",
    "**Add the next command into the block.** Click the `affect` line (1), then double-click `wait` in the list (2). Because `affect` is an open block, `wait` goes inside it.":
        "**Přidejte další příkaz do bloku.** Klikněte na řádek `affect` (1), pak v seznamu dvakrát klikněte na `wait` (2). Protože `affect` je otevřený blok, `wait` jde dovnitř.",
    "1: the selected affect line. 2: wait in the Events list.":
        "1: vybraný řádek affect. 2: wait v seznamu Události.",
    "**Check the result.** `wait` stands indented under `affect` - it is inside the block. The node counts the commands of the script.":
        "**Zkontrolujte výsledek.** `wait` stojí odsazený pod `affect` - je uvnitř bloku. Uzel počítá příkazy skriptu.",
    "wait inside the affect block, in the list on the right.":
        "wait uvnitř bloku affect, v seznamu vpravo.",
    "**Save.** **{{FileSave}}** in the action column (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The script already has its place in the mission folder.":
        "**Uložení.** **{{FileSave}}** ve sloupci akcí (nebo <kbd>Ctrl</kbd>+<kbd>S</kbd>). Skript už má své místo ve složce mise.",
    "**Open an empty tab and switch to Tracks.** Click **+** at the end of the tab bar, then **{{FlowTracks}}** above the script area. The timeline says that nothing is on it yet - and on the right, **{{TracksList}}** says *No commands yet*.":
        "**Otevřete prázdnou kartu a přepněte na Stopy.** Klikněte na **+** na konci řádku karet, pak na **{{FlowTracks}}** nad oblastí skriptu. Časová osa říká, že na ní zatím nic není - a vpravo říká **{{TracksList}}** *Zatím žádné příkazy*.",
    "1: Tracks. 2: the empty command list on the right.":
        "1: Stopy. 2: prázdný seznam příkazů vpravo.",
    "**Drag affect into the list.** Drag `affect` from the **{{GroupEvents}}** list (1) into the command list on the right (2). An `affect` block has no time of its own, so it shows in the list, not yet in the timeline.":
        "**Přetáhněte affect do seznamu.** Přetáhněte `affect` ze seznamu **{{GroupEvents}}** (1) do seznamu příkazů vpravo (2). Blok `affect` nemá vlastní čas, takže se ukáže v seznamu, zatím ne na časové ose.",
    "1: affect in the Events list. 2: the command list.":
        "1: affect v seznamu Události. 2: seznam příkazů.",
    "**Name the figure.** Double-click the `affect` line in the list. The **{{EditorTitle}}** opens: type the name of the figure into the first field and press **{{EditorOk}}**.":
        "**Pojmenujte postavu.** Dvakrát klikněte na řádek `affect` v seznamu. Otevře se **{{EditorTitle}}**: do prvního pole napište jméno postavy a stiskněte **{{EditorOk}}**.",
    "**Add a walk into the block.** Click the `affect` line (1), then double-click `set` in the **{{GroupEvents}}** list (2) - it goes inside the open block. In the editor choose `SET_NAVGOAL` and the walk target `nav_kyle`.":
        "**Přidejte do bloku chůzi.** Klikněte na řádek `affect` (1), pak dvakrát klikněte na `set` v seznamu **{{GroupEvents}}** (2) - jde dovnitř otevřeného bloku. V editoru zvolte `SET_NAVGOAL` a cíl chůze `nav_kyle`.",
    "1: the affect line. 2: set in the Events list.":
        "1: řádek affect. 2: set v seznamu Události.",
    "**Now it is on the timeline.** The figure got its own row, and the walk is a block on it - as long as the figure needs to get there. Commands without time (`affect`, the block itself) stay in the list.":
        "**Teď je na časové ose.** Postava dostala vlastní řádek a chůze je na něm blok - tak dlouhý, jak postava potřebuje, aby tam došla. Příkazy bez času (`affect`, samotný blok) zůstávají v seznamu.",
    "Kyle's row with the walk, and the list on the right.":
        "Kylův řádek s chůzí a seznam vpravo.",
    "**Save.** **{{FileSave}}** in the action column (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The first time, behaved asks for a name.":
        "**Uložení.** **{{FileSave}}** ve sloupci akcí (nebo <kbd>Ctrl</kbd>+<kbd>S</kbd>). Napoprvé se behaved zeptá na název.",
    "**Where the commands are.** In **{{FlowGraph}}**, click a script node (the open script is selected at the start). On the right, under **{{GraphCommands}}**, every command of that script is listed - blocks indented. Click a line to select it, double-click to edit it, right-click for the menu.":
        "**Kde jsou příkazy.** V **{{FlowGraph}}** klikněte na uzel skriptu (otevřený skript je na začátku vybraný). Vpravo pod **{{GraphCommands}}** je vypsán každý příkaz tohoto skriptu - bloky odsazené. Kliknutím na řádek ho vyberete, dvojklikem upravíte, pravým tlačítkem otevřete nabídku.",
    "The open script's node (1) and its commands (2).":
        "Uzel otevřeného skriptu (1) a jeho příkazy (2).",
    "**Double-click in the Events list.** Click a line on the right (1), then double-click a command in the **{{GroupEvents}}** list (2). It is inserted after that line - into the block, if the line is an open block.":
        "**Dvojklik v seznamu Události.** Klikněte na řádek vpravo (1), pak dvakrát klikněte na příkaz v seznamu **{{GroupEvents}}** (2). Vloží se za tento řádek - do bloku, je-li řádek otevřený blok.",
    "1: the selected line. 2: the command in the Events list.":
        "1: vybraný řádek. 2: příkaz v seznamu Události.",
    "**Drag onto a line.** Drag a command from the **{{GroupEvents}}** list (1) onto a line of **{{GraphCommands}}** (2): it lands right after that line.":
        "**Přetažení na řádek.** Přetáhněte příkaz ze seznamu **{{GroupEvents}}** (1) na řádek **{{GraphCommands}}** (2): dopadne hned za tento řádek.",
    "1: the command. 2: the line it goes after.":
        "1: příkaz. 2: řádek, za který jde.",
    "**Drag onto the node.** Dropped on a script node in the graph (2), the command goes to the end of that script. Dropped on free space, it starts a new script.":
        "**Přetažení na uzel.** Puštěný na uzel skriptu v grafu (2) jde příkaz na konec tohoto skriptu. Puštěný na volné místo začne nový skript.",
    "1: the command. 2: the script node - the command goes to its end.":
        "1: příkaz. 2: uzel skriptu - příkaz jde na jeho konec.",
    "**Right-click.** Right-click a line on the right: the submenus **{{GroupEvents}}** and **Macros** insert after that line - the same menu as in Classic.":
        "**Pravé tlačítko.** Klikněte pravým tlačítkem na řádek vpravo: podnabídky **{{GroupEvents}}** a **Makra** vkládají za tento řádek - stejná nabídka jako v Klasickém.",
    "The menu of a line in the Graph, with its Events submenu.":
        "Nabídka řádku v Grafu s podnabídkou Události.",
    "**Where the commands are.** **{{FlowTracks}}** shows the script over time: the camera, one row per figure, the waits. On the right, **{{TracksList}}** lists every command - also those that take no time, such as `affect`, `task` or `declare`.":
        "**Kde jsou příkazy.** **{{FlowTracks}}** ukazuje skript v čase: kameru, jeden řádek na postavu, čekání. Vpravo vypíše **{{TracksList}}** každý příkaz - i ty, které nezaberou čas, jako `affect`, `task` nebo `declare`.",
    "1: the rows of the timeline. 2: the command list.":
        "1: řádky časové osy. 2: seznam příkazů.",
    "**Double-click in the Events list.** Click a block or a line of the list (1), then double-click a command in the **{{GroupEvents}}** list (2). It goes right after the selected command.":
        "**Dvojklik v seznamu Události.** Klikněte na blok nebo řádek seznamu (1), pak dvakrát klikněte na příkaz v seznamu **{{GroupEvents}}** (2). Jde hned za vybraný příkaz.",
    "1: the selected block. 2: the command in the Events list.":
        "1: vybraný blok. 2: příkaz v seznamu Události.",
    "**Drag into a row at a moment.** Drag a command from the **{{GroupEvents}}** list (1) into a row of the timeline (2). A yellow line shows the moment. The command is placed after the last command of that row that starts before the line - in a figure's row, inside that figure's `affect` block.":
        "**Přetažení do řádku v okamžiku.** Přetáhněte příkaz ze seznamu **{{GroupEvents}}** (1) do řádku časové osy (2). Žlutá čára ukazuje okamžik. Příkaz se umístí za poslední příkaz tohoto řádku, který začíná před čárou - v řádku postavy dovnitř bloku `affect` této postavy.",
    "1: the command. 2: the moment in Kyle's row.":
        "1: příkaz. 2: okamžik v Kylově řádku.",
    "**Drag into the list.** For commands without time - an `affect`, a `task`, a `declare` - drag into the list on the right (2): after the line you drop on, or at the end on free space.":
        "**Přetažení do seznamu.** Příkazy bez času - `affect`, `task`, `declare` - přetáhněte do seznamu vpravo (2): za řádek, na který je pustíte, nebo na volné místo na konec.",
    "1: the command. 2: the line in the command list.":
        "1: příkaz. 2: řádek v seznamu příkazů.",
    "**Right-click.** Right-click a block (or a line of the list): **{{GroupEvents}}** and **Macros** insert after it.":
        "**Pravé tlačítko.** Klikněte pravým tlačítkem na blok (nebo řádek seznamu): **{{GroupEvents}}** a **Makra** vkládají za něj.",
    "The menu of a block, with its Events submenu.":
        "Nabídka bloku s podnabídkou Události.",
    "**Open the editor.** In **{{FlowGraph}}**, double-click a line under **{{GraphCommands}}** (or select it and choose **Edit...** from its right-click menu). The **{{EditorTitle}}** is the same in every view.":
        "**Otevřete editor.** V **{{FlowGraph}}** dvakrát klikněte na řádek pod **{{GraphCommands}}** (nebo ho vyberte a z nabídky pravého tlačítka zvolte **Upravit...**). **{{EditorTitle}}** je v každém pohledu stejný.",
    "Double-click a line of the list to edit it.":
        "Dvojklikem na řádek seznamu ho upravíte.",
    "**Fields with fixed choices.** Fields like `SET_TYPES` or `true` / `false` are lists you can also type into. A value that is not in the list is allowed, but marked **unknown**.":
        "**Pole s pevnými volbami.** Pole jako `SET_TYPES` nebo `true` / `false` jsou seznamy, do kterých lze i psát. Hodnota, která v seznamu není, je povolena, ale označena jako **neznámá**.",
    "**Expressions and free text.** **{{EditorHelper}}** builds `get`, `tag` and `random`; **{{EditorExpr}}** turns a field into free text.":
        "**Výrazy a volný text.** **{{EditorHelper}}** sestaví `get`, `tag` a `random`; **{{EditorExpr}}** změní pole na volný text.",
    "**Done.** **{{EditorOk}}** writes the values, **{{EditorCancel}}** leaves everything as it was. The line on the right shows the new values at once.":
        "**Hotovo.** **{{EditorOk}}** zapíše hodnoty, **{{EditorCancel}}** nechá vše, jak bylo. Řádek vpravo hned ukáže nové hodnoty.",
    "**Open the editor.** In **{{FlowTracks}}**, double-click a block - or a line of the command list. The **{{EditorTitle}}** is the same in every view.":
        "**Otevřete editor.** Ve **{{FlowTracks}}** dvakrát klikněte na blok - nebo na řádek seznamu příkazů. **{{EditorTitle}}** je v každém pohledu stejný.",
    "Double-click a block to edit its command.":
        "Dvojklikem na blok upravíte jeho příkaz.",
    "**Change a time by dragging.** Drag the right edge of a block: a `wait`, a camera move or a walk gets longer or shorter. The value in the script changes with it.":
        "**Změna času tažením.** Táhněte pravý okraj bloku: `wait`, pohyb kamery nebo chůze se prodlouží nebo zkrátí. Hodnota ve skriptu se mění s ním.",
    "The right edge of the wait block.":
        "Pravý okraj bloku wait.",
    "**Fields for values.** Numbers, vectors and text are typed into their fields.":
        "**Pole pro hodnoty.** Čísla, vektory a text se píší do svých polí.",
    "**Fields with fixed choices.** Lists you can also type into; an unknown value is allowed but marked.":
        "**Pole s pevnými volbami.** Seznamy, do kterých lze i psát; neznámá hodnota je povolena, ale označena.",
    "**Done.** **{{EditorOk}}** writes the values; the block moves or changes its length at once.":
        "**Hotovo.** **{{EditorOk}}** zapíše hodnoty; blok se hned posune nebo změní délku.",
    "**Select.** Click a line under **{{GraphCommands}}**. The action column on the right works on it, as in Classic.":
        "**Výběr.** Klikněte na řádek pod **{{GraphCommands}}**. Sloupec akcí vpravo na něm pracuje, jako v Klasickém.",
    "The selected line.":
        "Vybraný řádek.",
    "**Right-click for the rest.** The menu of a line has everything: **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** and **{{ActDelete}}**.":
        "**Pravé tlačítko pro zbytek.** Nabídka řádku má vše: **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** a **{{ActDelete}}**.",
    "The menu of a line in the Graph.":
        "Nabídka řádku v Grafu.",
    "Edit...":
        "Upravit...",
    "Opens the event editor.":
        "Otevře editor událostí.",
    "A copy right below.":
        "Kopie hned pod.",
    "Copy / Cut / Paste":
        "Kopírovat / Vyjmout / Vložit",
    "Through the clipboard - also into another script.":
        "Přes schránku - i do jiného skriptu.",
    "Move up / Move down":
        "Posunout nahoru / Posunout dolů",
    "One line up or down, inside its block.":
        "O řádek nahoru nebo dolů, v rámci bloku.",
    "REM":
        "REM",
    "Switches the line off (comment) or on again.":
        "Vypne řádek (komentář) nebo ho znovu zapne.",
    "Removes the line.":
        "Odebere řádek.",
    "**Or the action column.** **{{ActMoveUp}}** / **{{ActMoveDown}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActPaste}}** and **{{ActRem}}** on the right do the same for the selected line.":
        "**Nebo sloupec akcí.** **{{ActMoveUp}}** / **{{ActMoveDown}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActPaste}}** a **{{ActRem}}** vpravo dělají totéž pro vybraný řádek.",
    "Move up and Move down in the action column.":
        "Posunout nahoru a Posunout dolů ve sloupci akcí.",
    "**Select.** Click a block in the timeline - or a line of the command list.":
        "**Výběr.** Klikněte na blok na časové ose - nebo na řádek seznamu příkazů.",
    "The selected block.":
        "Vybraný blok.",
    "**Right-click for the rest.** The menu of a block has **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** and **{{ActDelete}}**.":
        "**Pravé tlačítko pro zbytek.** Nabídka bloku má **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** a **{{ActDelete}}**.",
    "The menu of a block in Tracks.":
        "Nabídka bloku ve Stopách.",
    "A copy right after it - on the timeline it follows the first.":
        "Kopie hned za ním - na časové ose následuje první.",
    "Through the clipboard.":
        "Přes schránku.",
    "One line up or down in the script - that changes the order in time.":
        "O řádek nahoru nebo dolů ve skriptu - to mění pořadí v čase.",
    "Switches the command off or on.":
        "Vypne nebo zapne příkaz.",
    "**Moving changes the time.** Moving a `wait` up or down shifts everything after it - watch the blocks move.":
        "**Posun mění čas.** Posun `wait` nahoru nebo dolů posune vše po něm - sledujte, jak se bloky pohnou.",
    "After Move up: the blocks after it start earlier.":
        "Po Posunout nahoru: bloky po něm začínají dřív.",
    "**Build the task.** Click the `affect` line, double-click `task` in the **{{GroupEvents}}** list and name it `walk` in the editor. Then click the `task` line and double-click `set` twice - for `SET_WALKING` and `SET_NAVGOAL`.":
        "**Sestavte úlohu.** Klikněte na řádek `affect`, dvakrát klikněte na `task` v seznamu **{{GroupEvents}}** a v editoru ho pojmenujte `walk`. Pak klikněte na řádek `task` a dvakrát dvojklikněte na `set` - pro `SET_WALKING` a `SET_NAVGOAL`.",
    "The task walk with the walk inside, in the list.":
        "Úloha walk s chůzí uvnitř, v seznamu.",
    "**dowait after it.** Drag `dowait` from the **{{GroupEvents}}** list onto the `task` line: a command dropped on a line goes *after* it - after the whole block. Name the task `walk` in the editor.":
        "**dowait za ní.** Přetáhněte `dowait` ze seznamu **{{GroupEvents}}** na řádek `task`: příkaz puštěný na řádek jde *za* něj - za celý blok. V editoru úlohu pojmenujte `walk`.",
    "dowait walk after the task.":
        "dowait walk za úlohou.",
    "**Check it in Tracks.** Switch to **{{FlowTracks}}**: what follows the `dowait` starts where the walk ends.":
        "**Zkontrolujte to ve Stopách.** Přepněte na **{{FlowTracks}}**: co následuje po `dowait`, začíná tam, kde chůze končí.",
    "Kyle's row: the look starts after the walk.":
        "Kylův řádek: pohled začíná po chůzi.",
    "**Build the task in the list.** A `task` has no time of its own - build it in the command list: click the `affect` line, double-click `task`, name it `walk`; click the `task` line and double-click `set` twice for `SET_WALKING` and `SET_NAVGOAL`.":
        "**Sestavte úlohu v seznamu.** `task` nemá vlastní čas - sestavte ho v seznamu příkazů: klikněte na řádek `affect`, dvakrát klikněte na `task`, pojmenujte ho `walk`; klikněte na řádek `task` a dvakrát dvojklikněte na `set` pro `SET_WALKING` a `SET_NAVGOAL`.",
    "The task walk in the command list.":
        "Úloha walk v seznamu příkazů.",
    "**dowait.** Drag `dowait` onto the `task` line of the list - it goes after the block - and name the task `walk`. In the figure's row, everything after it now starts where the walk ends.":
        "**dowait.** Přetáhněte `dowait` na řádek `task` v seznamu - jde za blok - a úlohu pojmenujte `walk`. V řádku postavy teď vše po něm začíná tam, kde chůze končí.",
    "**Two at once.** With `do` instead of `dowait`, the next command starts at once - its block moves to the start of the walk.":
        "**Dvě zároveň.** S `do` místo `dowait` začne další příkaz hned - jeho blok se posune na začátek chůze.",
    "With do: the look starts with the walk.":
        "S do: pohled začíná s chůzí.",
    "**Both scripts in the graph.** The graph shows which entity starts which script. Click the script that should wait - here the intro - and insert `waitsignal` into its list (select a line, double-click `waitsignal`). Name the signal `door_open`.":
        "**Oba skripty v grafu.** Graf ukazuje, která entita spouští který skript. Klikněte na skript, který má čekat - zde úvod - a vložte do jeho seznamu `waitsignal` (vyberte řádek, dvakrát klikněte na `waitsignal`). Signál pojmenujte `door_open`.",
    "The intro node and its waitsignal.":
        "Uzel úvodu a jeho waitsignal.",
    "**The other script.** Click the other script's node. If it is not open, **{{GraphEditHere}}** opens it; then insert `signal ( \"door_open\" )` in its list.":
        "**Druhý skript.** Klikněte na uzel druhého skriptu. Není-li otevřený, otevře ho **{{GraphEditHere}}**; pak do jeho seznamu vložte `signal ( \"door_open\" )`.",
    "The other script: Edit here opens it.":
        "Druhý skript: Upravit zde ho otevře.",
    "**Check the names.** Both must be spelled exactly the same - only one script can wait for a given name.":
        "**Zkontrolujte názvy.** Oba musí být napsané přesně stejně - na daný název může čekat jen jeden skript.",
    "**waitsignal in Tracks.** Insert `waitsignal` into the script that waits (double-click in the **{{GroupEvents}}** list after selecting a block). It shows as a short block in the **Script** row.":
        "**waitsignal ve Stopách.** Vložte `waitsignal` do skriptu, který čeká (dvojklik v seznamu **{{GroupEvents}}** po výběru bloku). Ukáže se jako krátký blok v řádku **Skript**.",
    "waitsignal in the Script row.":
        "waitsignal v řádku Skript.",
    "**The timeline cannot know the wait.** How long a `waitsignal` waits depends on the game - everything after it is placed as if the signal came at once.":
        "**Časová osa čekání nezná.** Jak dlouho `waitsignal` čeká, závisí na hře - vše po něm se umístí, jako by signál přišel hned.",
    "Everything after the waitsignal follows it directly.":
        "Vše po waitsignal ho přímo následuje.",
    "**signal in the other script.** Open the other script's tab and insert `signal ( \"door_open\" )` there - in its list or after one of its blocks.":
        "**signal v druhém skriptu.** Otevřete kartu druhého skriptu a vložte tam `signal ( \"door_open\" )` - do jeho seznamu nebo za některý z jeho bloků.",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list.":
        "**Najděte je.** Makra jsou na konci seznamu **{{GroupEvents}}**.",
    "**Drag one into the list.** Drag a macro (1) onto a line under **{{GraphCommands}}** (2). It shows as one line, `$ standOnly` - its `set` commands are inside.":
        "**Přetáhněte jedno do seznamu.** Přetáhněte makro (1) na řádek pod **{{GraphCommands}}** (2). Ukáže se jako jeden řádek, `$ standOnly` - jeho příkazy `set` jsou uvnitř.",
    "1: the macro. 2: the line it goes after.":
        "1: makro. 2: řádek, za který jde.",
    "**Or right-click.** The menu of a line has a **Macros** submenu.":
        "**Nebo pravé tlačítko.** Nabídka řádku má podnabídku **Makra**.",
    "The menu of a line, with its Macros submenu.":
        "Nabídka řádku s podnabídkou Makra.",
    "**Drag one into the list.** A macro sets values and takes no time, so it goes into the command list on the right: drag it (1) onto a line (2).":
        "**Přetáhněte jedno do seznamu.** Makro nastavuje hodnoty a nezabere čas, takže jde do seznamu příkazů vpravo: přetáhněte ho (1) na řádek (2).",
    "1: the macro. 2: the line in the command list.":
        "1: makro. 2: řádek v seznamu příkazů.",
    "**Or right-click.** The menu of a block or a line has a **Macros** submenu.":
        "**Nebo pravé tlačítko.** Nabídka bloku nebo řádku má podnabídku **Makra**.",
    "The menu of a block, with its Macros submenu.":
        "Nabídka bloku s podnabídkou Makra.",
    "**Declare.** Select the first line under **{{GraphCommands}}** and double-click `declare` in the **{{GroupEvents}}** list. In the editor choose `FLOAT` and type the name `seen`.":
        "**Deklarace.** Vyberte první řádek pod **{{GraphCommands}}** a dvakrát klikněte na `declare` v seznamu **{{GroupEvents}}**. V editoru zvolte `FLOAT` a napište název `seen`.",
    "declare, if and else in the list of the Graph.":
        "declare, if a else v seznamu Grafu.",
    "**Ask with if.** Insert `if` the same way. In the editor, **{{EditorHelper}}** → **{{EditorGet}}** builds `get ( FLOAT, \"seen\" )`; comparison `=`, value `0`.":
        "**Zeptejte se pomocí if.** Vložte `if` stejně. V editoru sestaví **{{EditorHelper}}** → **{{EditorGet}}** `get ( FLOAT, \"seen\" )`; porovnání `=`, hodnota `0`.",
    "**Fill the blocks.** Select the `if` line and insert the scene's commands - they go inside the open block. `else` right after the `if` runs when the scene was seen before; `set ( \"seen\", \"1\" )` at the end of the `if` remembers it.":
        "**Naplňte bloky.** Vyberte řádek `if` a vložte příkazy scény - jdou dovnitř otevřeného bloku. `else` hned za `if` se spustí, když už scéna byla viděna; `set ( \"seen\", \"1\" )` na konci `if` si to zapamatuje.",
    "The if and else blocks, indented in the list.":
        "Bloky if a else, odsazené v seznamu.",
    "**Build it in the list.** `declare`, `if` and `else` take no time - build them in the command list on the right, the same way as in the Graph.":
        "**Sestavte to v seznamu.** `declare`, `if` a `else` nezaberou čas - sestavte je v seznamu příkazů vpravo, stejně jako v Grafu.",
    "declare, if and else in the command list of Tracks.":
        "declare, if a else v seznamu příkazů Stop.",
    "**Ask with if.** In the editor, **{{EditorHelper}}** → **{{EditorGet}}** builds `get ( FLOAT, \"seen\" )`; comparison `=`, value `0`.":
        "**Zeptejte se pomocí if.** V editoru sestaví **{{EditorHelper}}** → **{{EditorGet}}** `get ( FLOAT, \"seen\" )`; porovnání `=`, hodnota `0`.",
    "**What the timeline shows.** The timeline plays the commands inside the `if` as if it were true - it cannot know the value in the game.":
        "**Co časová osa ukazuje.** Časová osa přehraje příkazy uvnitř `if`, jako by byl pravdivý - nemůže znát hodnotu ve hře.",
    "The commands inside the if on the timeline.":
        "Příkazy uvnitř if na časové ose.",
    " · Classic, Graph, Tracks":
        " · Klasický, Graf, Stopy",
    "Guide":
        "Příručka",
    "Search the guide...  (/)":
        "Hledat v příručce...  (/)",
    "Light or dark":
        "Světlý nebo tmavý",
    "Previous":
        "Zpět",
    "Next":
        "Další",
    "Pause":
        "Pozastavit",
    "View:":
        "Pohled:",
    "Nothing found.":
        "Nic nenalezeno.",
    "on":
        "na",
    "Picture still in English":
        "Obrázek je zatím anglicky",
    "No.":
        "Č.",
    "JK Studio guide - mod tools for Jedi Knight: Jedi Academy and Jedi Outcast. The pictures show the program itself; they are made by its self-test. Not affiliated with Lucasfilm, Disney or Raven Software.":
        "Příručka k JK Studiu - nástroje pro mody do Jedi Knight: Jedi Academy a Jedi Outcast. Obrázky ukazují samotný program; pořizuje je jeho autotest. Bez spojení s Lucasfilm, Disney ani Raven Software.",
    # uebersetzung1: Texte aus karte9d, mission1 und menue2 (nach Integration 3)
    "**Grab the figure in the middle of its walk** (timeline between start and arrival) and drag: a new point is created at that moment - for figures a new `ref_tag` as navgoal (it does not change the nav network, so the figure walks on to its old goal as in the game), for ships a split `move`. If a figure finds no path from a point, a red dashed line with the note \"No path from here to ...\" leads to the goal it cannot reach.":
        "**Uchopte postavu uprostřed chůze** (časová osa mezi začátkem a příchodem) a táhněte: v tom okamžiku vznikne nový bod - u postav nový `ref_tag` jako navgoal (nemění navigační síť, takže postava jde dál ke svému starému cíli jako ve hře), u lodí rozdělený `move`. Pokud postava z nějakého bodu nenajde cestu, vede k nedosažitelnému cíli červená přerušovaná čára s poznámkou \"Odsud nevede cesta k ...\".",
    "**After a flight** - in the pause before the next one or after the last - moving or rotating a ship also creates a new key: a new `move` from the end of the previous flight to this moment. The start and the other keys keep their place, angles and times. Likewise, moving a figure after its last walk adds a new navgoal (a `ref_tag`) it walks to when it arrives; its spawner - the start - stays. While the timeline plays, no gizmo is shown. One <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes back a whole gizmo change - a new point together with its new navgoal - and <kbd>Ctrl</kbd>+<kbd>Y</kbd> brings it back exactly.":
        "**Po letu** - v pauze před dalším nebo po posledním - vytvoří posunutí nebo otočení lodi také nový klíč: nový `move` od konce předchozího letu do tohoto okamžiku. Začátek a ostatní klíče si ponechají místo, úhly a časy. Stejně tak posunutí postavy po její poslední chůzi přidá nový navgoal (`ref_tag`), ke kterému po příchodu dojde; její spawner - začátek - zůstává. Během přehrávání časové osy se gizmo nezobrazuje. Jedno <kbd>Ctrl</kbd>+<kbd>Z</kbd> vrátí celou změnu gizma - nový bod i s jeho novým navgoalem - a <kbd>Ctrl</kbd>+<kbd>Y</kbd> ji přesně obnoví.",
    "The texture browser shows the textures and shaders of the game, folder by folder. In the layout {{KeLayoutRegular}} it sits below the camera; in the other layouts **{{KeLView}} > {{KeLTextureBrowser}}** (<kbd>T</kbd>) opens it. A click on a texture makes it the current one and puts it on the selected brushes or faces; the middle mouse button only picks it. Right mouse drag scrolls, a right click opens the texture's menu. In the camera, the middle mouse button on a face picks up its texture; together with <kbd>Ctrl</kbd> or <kbd>Shift</kbd> it paints the current texture onto faces, as in NetRadiant-Custom. **{{KeSeMenue}}** in that menu opens the shader file in an editor with syntax colours and the margin of the script editor (line numbers, change marks); **{{KeSave}}** never writes into the game folder - a game shader file goes under the same name into the import folder, which comes first in the search path - and the shaders, the texture browser, the camera and the real-time light reload at once. Any change to a .shader file in the import folder (also from another text editor) reloads them too.":
        "Prohlížeč textur ukazuje textury a shadery hry, složku po složce. V rozvržení {{KeLayoutRegular}} je pod kamerou; v ostatních rozvrženích ho otevře **{{KeLView}} > {{KeLTextureBrowser}}** (<kbd>T</kbd>). Kliknutí na texturu z ní udělá aktuální a dá ji na vybrané brushe nebo stěny; prostřední tlačítko ji jen vybere. Tažení pravým tlačítkem posouvá, pravé kliknutí otevře nabídku textury. V kameře prostřední tlačítko na stěně převezme její texturu; spolu s <kbd>Ctrl</kbd> nebo <kbd>Shift</kbd> maluje aktuální texturu na stěny, jako v NetRadiant-Custom. **{{KeSeMenue}}** v této nabídce otevře soubor shaderů v editoru s barevnou syntaxí a okrajem editoru skriptů (čísla řádků, značky změn); **{{KeSave}}** nikdy nezapisuje do složky hry - soubor shaderů hry jde pod stejným názvem do složky importu, která je v cestě hledání první - a shadery, prohlížeč textur, kamera a světlo v reálném čase se ihned znovu načtou. Každá změna souboru .shader ve složce importu (i z jiného textového editoru) je také znovu načte.",
    "The shader editor (live) with the fog shader of a test map: syntax colours, line numbers and change marks as in the script editor; it saves to the import folder.":
        "Editor shaderů (živě) s mlhovým shaderem testovací mapy: barevná syntaxe, čísla řádků a značky změn jako v editoru skriptů; ukládá do složky importu.",
    "The **{{KeLCurve}}** menu builds patches in the bounds of the selection: plane (**{{KeLSimplePatchMesh}}**, <kbd>Shift</kbd>+<kbd>P</kbd>), bevel, end cap, cylinders, cone, sphere and the exact versions with a size dialog. **{{KeLInsertDelete}}** adds or removes rows and columns, **{{KeLMatrix}}** inverts (<kbd>Ctrl</kbd>+<kbd>I</kbd>), transposes (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>), redisperses and smooths. **{{KeLCapSelection}}** (<kbd>Shift</kbd>+<kbd>C</kbd>) closes the ends, **{{KeLThicken}}** (<kbd>Ctrl</kbd>+<kbd>T</kbd>) gives a patch a thickness, **{{KeLDeform}}** adds noise. The texture of a patch: **{{KeLNaturalize}}** (<kbd>Ctrl</kbd>+<kbd>N</kbd>), invert X and Y, reset. **{{KePbMenue}}** joins two selected patches with a new patch whose first and last row are exactly the control points of one edge of each (with selected control points the edge that holds them, otherwise the nearest pair of edges with the same number of points) - no seam, one undo step.":
        "Nabídka **{{KeLCurve}}** staví patche v rozměrech výběru: rovinu (**{{KeLSimplePatchMesh}}**, <kbd>Shift</kbd>+<kbd>P</kbd>), zkosení, koncovou čepičku, válce, kužel, kouli a přesné verze s dialogem velikosti. **{{KeLInsertDelete}}** přidává nebo odebírá řádky a sloupce, **{{KeLMatrix}}** obrací (<kbd>Ctrl</kbd>+<kbd>I</kbd>), transponuje (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>), znovu rozkládá a vyhlazuje. **{{KeLCapSelection}}** (<kbd>Shift</kbd>+<kbd>C</kbd>) uzavře konce, **{{KeLThicken}}** (<kbd>Ctrl</kbd>+<kbd>T</kbd>) dá patchi tloušťku, **{{KeLDeform}}** přidá šum. Textura patche: **{{KeLNaturalize}}** (<kbd>Ctrl</kbd>+<kbd>N</kbd>), obrácení X a Y, obnovení. **{{KePbMenue}}** spojí dva vybrané patche novým patchem, jehož první a poslední řada jsou přesně kontrolní body po jedné hraně každého z nich (s vybranými kontrolními body hrana, která je obsahuje, jinak nejbližší dvojice hran se stejným počtem bodů) - bez švu, jeden krok zpět.",
    "Meshes of a model that the .bsp does not have (the mapper hid them, for example with a nodraw shader) get a `_remap` to `textures/system/nodraw`: the editor and a new build show exactly what the .bsp holds (q3map2 does not emit nodraw surfaces). Only a mesh that shares its shader with a found one cannot be hidden this way.":
        "Sítě modelu, které .bsp nemá (mapper je skryl, například shaderem nodraw), dostanou `_remap` na `textures/system/nodraw`: editor i nové sestavení ukazují přesně to, co obsahuje .bsp (q3map2 plochy nodraw nevytváří). Jen síť, která sdílí shader s nalezenou sítí, takto skrýt nelze.",
    "While **File > Open .bsp** still recovers the misc_model entities in the background, fixes and imports wait (the buttons are disabled, the details show the hint): a geometry change in that time would drop the recovery, or bring it back after an Undo as a new step that clears Redo.":
        "Dokud **File > Open .bsp** na pozadí ještě obnovuje entity misc_model, opravy a importy čekají (tlačítka jsou neaktivní, podrobnosti ukazují upozornění): změna geometrie v té době by obnovu zahodila, nebo by ji po kroku zpět vrátila jako nový krok, který vymaže historii Znovu.",
    "Hint suggestions are only shown, never applied: where a hint brush helps depends on the gameplay. A detail suggestion for decoration is an estimate too: after a build without a leak it leaves out every brush with a face at the void of that .bsp (in the test room a small plug in a hole of the outer wall - as detail the map leaks), but a stale .bsp or a gap the sample points miss can still fool it - never make brushes detail that seal the map.":
        "Návrhy hintů se jen ukazují, nikdy nepoužijí: kde hint brush pomůže, závisí na hratelnosti. Návrh detailu pro dekorace je také odhad: po sestavení bez úniku vynechá každý brush se stěnou k prázdnu daného .bsp (v testovací místnosti malá zátka v díře vnější stěny - jako detail mapa uniká), ale zastaralé .bsp nebo mezera, kterou vzorkovací body minou, ho mohou stále oklamat - nikdy neudělejte detailními brushe, které utěsňují mapu.",
    "**Models and patches:** misc_model entities cast shadows like in q3map2 (in the world always, otherwise only with `_castShadows`; `_castShadows 0` switches it off) and their surfaces with `q3map_surfacelight` shine per triangle; patches shine over the same tessellation as in -light. Brush faces shine over the triangles of `-meta`; `q3map_sunExt` with deviance and samples and lights with `_samples` get their jittered copies with q3map2's own random sequence.":
        "**Modely a patche:** entity misc_model vrhají stíny jako v q3map2 (ve světě vždy, jinak jen s `_castShadows`; `_castShadows 0` to vypne) a jejich plochy s `q3map_surfacelight` svítí po trojúhelnících; patche svítí přes stejnou teselaci jako v -light. Stěny brushů svítí přes trojúhelníky `-meta`; `q3map_sunExt` s odchylkou a vzorky a světla s `_samples` dostanou své rozptýlené kopie s vlastní náhodnou posloupností q3map2.",
    "Fog, effects and sky portal":
        "Mlha, efekty a portál oblohy",
    "**Fog:** brushes with a `fogParms` shader and the global fog of worldspawn (`_fog`) are drawn as the game draws them (RB_FogPass with R_FogFactor): colour and distance to opaque from the shader, the fog surface from the visible side of the brush (or `q3map_fogDir`), deeper below it is denser.":
        "**Mlha:** brushe se shaderem `fogParms` a globální mlha worldspawn (`_fog`) se kreslí tak, jak je kreslí hra (RB_FogPass s R_FogFactor): barva a vzdálenost do neprůhlednosti ze shaderu, povrch mlhy z viditelné strany brushe (nebo `q3map_fogDir`), hlouběji pod ním je hustší.",
    "**Effects:** fx_runner entities play their effect with the effect system of the cutscene preview - repeat interval `delay`/`random`, direction to the `target` (or `angle`, otherwise up); START_OFF and ONESHOT wait for a use. **{{KeLBuild}} > {{KeEchtEffekte}}** switches them off. The editor has no .bsp, so particles do not bounce off walls.":
        "**Efekty:** entity fx_runner přehrávají svůj efekt systémem efektů z náhledu animačních sekvencí - interval opakování `delay`/`random`, směr k `target` (nebo `angle`, jinak nahoru); START_OFF a ONESHOT čekají na use. **{{KeLBuild}} > {{KeEchtEffekte}}** je vypne. Editor nemá .bsp, proto se částice neodrážejí od stěn.",
    "**Sky portal:** with a misc_skyportal the sky shows the scene from the portal's place, with the same view and field of view, as in the game (CG_DrawSkyBoxPortal).":
        "**Portál oblohy:** s misc_skyportal ukazuje obloha scénu z místa portálu, se stejným pohledem a zorným polem, jako ve hře (CG_DrawSkyBoxPortal).",
    "A fog brush over the floor of a room (fogParms 0.1 0.9 0.1, 256): the floor gets the fog colour of the game.":
        "Mlhový brush nad podlahou místnosti (fogParms 0.1 0.9 0.1, 256): podlaha dostane barvu mlhy jako ve hře.",
    "**{{MeMeldungen}}** shows what the game would print while loading the file, word for word and with the line (for example `Unknown item keyword 'foo' Line #21`). A click jumps to the line; the live diagnosis marks it in the text ([[menue_editor_m2#diagnose]]).":
        "**{{MeMeldungen}}** ukazují, co by hra vypsala při načítání souboru, slovo od slova a s řádkem (například `Unknown item keyword 'foo' Line #21`). Kliknutí skočí na řádek; živá diagnostika ho označí v textu ([[menue_editor_m2#diagnose]]).",
    "A new button from the widgets: selected on the canvas, its itemDef in the Text tab on the right.":
        "Nové tlačítko z prvků: vybrané na plátně, jeho itemDef vpravo na kartě Text.",
    "Menu editor: preview, checks and packaging":
        "Editor menu: náhled, kontroly a balíček",
    "The right column with tabs, the live diagnosis while typing, videos, 3D models and sabers like in the game, the widget library, texts in several languages, sounds, the checks for the game, a test in the game and fonts from TrueType.":
        "Pravý sloupec s kartami, živá diagnostika při psaní, videa, 3D modely a světelné meče jako ve hře, knihovna prvků, texty v několika jazycích, zvuky, kontroly pro hru, test ve hře a písma z TrueType.",
    "Layout: canvas at full height":
        "Rozvržení: plátno na celou výšku",
    "Widgets as tiles on the left, the canvas in the middle, the right column with the tab Hierarchy.":
        "Prvky jako dlaždice vlevo, plátno uprostřed, pravý sloupec s kartou Hierarchie.",
    "The same column with the tab Text: the code editor, the problems below.":
        "Tentýž sloupec s kartou Text: editor kódu, pod ním problémy.",
    "The right column holds **{{MeReiterHierarchie}}** (menus, groups and items; the eye hides an item in the preview only, the lock keeps it from moving, dragging changes the drawing order), **{{MeReiterText}}** (the code editor with the problems below) and **{{MeReiterEigenschaften}}** (properties, cvars, engine messages and the script log as sections). Drag the divider to change the width; the tab and the width are remembered. A click on the canvas moves the text cursor to the itemDef even while another tab is open - switch to **{{MeReiterText}}** and the line is there. The old layout with the text at the bottom is in the preferences (**{{MeEinstTextUnten}}**), as is **{{MeEinstUmbruch}}** (otherwise long lines scroll sideways with Shift + mouse wheel).":
        "Pravý sloupec obsahuje **{{MeReiterHierarchie}}** (menu, skupiny a prvky; oko skryje prvek jen v náhledu, zámek mu brání v posunu, tažení mění pořadí kreslení), **{{MeReiterText}}** (editor kódu, pod ním problémy) a **{{MeReiterEigenschaften}}** (vlastnosti, cvary, zprávy enginu a protokol skriptu jako oddíly). Šířku změníte tažením oddělovače; karta i šířka se pamatují. Kliknutí na plátno přesune textový kurzor na itemDef, i když je otevřená jiná karta - přepněte na **{{MeReiterText}}** a řádek je tam. Staré rozvržení s textem dole je v předvolbách (**{{MeEinstTextUnten}}**), stejně jako **{{MeEinstUmbruch}}** (jinak se dlouhé řádky posouvají do strany pomocí Shift + kolečka myši).",
    "Live diagnosis":
        "Živá diagnostika",
    "A broken menu: the brace that is never closed and where it is missing, the misspelled keyword with the game's message, a show on an item that does not exist; the item is marked on the canvas.":
        "Rozbité menu: složená závorka, která se nikdy neuzavře, a kde chybí, chybně napsané klíčové slovo se zprávou hry, show na prvek, který neexistuje; prvek je na plátně označen.",
    "While you type, the menu is checked in the background a moment later. A wavy line marks the exact place - red for errors, yellow for warnings, blue for hints - and the margin shows a symbol; hovering shows the message: the game's own wording where there is one (`Unknown item keyword 'backcolr'`) plus a short explanation. The problems list below the text jumps to the place and selects the item on the canvas; items with errors and warnings are outlined there.":
        "Během psaní se menu o chvilku později zkontroluje na pozadí. Vlnovka označí přesné místo - červená pro chyby, žlutá pro varování, modrá pro tipy - a na okraji je symbol; po najetí myší se zobrazí zpráva: vlastní znění hry, kde nějaké je (`Unknown item keyword 'backcolr'`), a krátké vysvětlení. Seznam problémů pod textem skočí na místo a vybere prvek na plátně; prvky s chybami a varováními jsou tam orámované.",
    "Braces `{ }` and `( )` and quotation marks: missing, extra or mismatched - both the opening and the closing place; the indentation tells where a brace is missing.":
        "Závorky `{ }` a `( )` a uvozovky: chybějící, přebývající nebo nespárované - otevírací i uzavírací místo; odsazení ukazuje, kde závorka chybí.",
    "Unknown keywords per version, numbers where single player silently reads 0, duplicate menuDef names, targets of open/close/show/hide/setfocus that do not exist, cvars that look like a typo of a known cvar.":
        "Neznámá klíčová slova podle verze, čísla tam, kde hra pro jednoho hráče tiše čte 0, duplicitní názvy menuDef, cíle open/close/show/hide/setfocus, které neexistují, cvary, které vypadají jako překlep známého cvaru.",
    "The checks for the game (the same rules as **{{MePruefen}}**): limits, items that are never visible, missing files.":
        "Kontroly pro hru (stejná pravidla jako **{{MePruefen}}**): limity, prvky, které nikdy nejsou vidět, chybějící soubory.",
    "**{{MeDiagBeheben}}** in the list or <kbd>Ctrl</kbd>+<kbd>.</kbd> at the cursor applies a quick fix where the answer is clear: insert the missing brace, the right keyword, a similar existing file, the known cvar. Undo takes it back.":
        "**{{MeDiagBeheben}}** v seznamu nebo <kbd>Ctrl</kbd>+<kbd>.</kbd> u kurzoru použije rychlou opravu tam, kde je odpověď jasná: vloží chybějící závorku, správné klíčové slovo, podobný existující soubor, známý cvar. Krok zpět ji vrátí.",
    "Videos, models and sabers":
        "Videa, modely a světelné meče",
    "A RoQ video as cinematic, decoded like the game.":
        "Video RoQ jako cinematic, dekódované jako ve hře.",
    "A character model with skin and animation, drawn on the graphics card.":
        "Model postavy se skinem a animací, kreslený na grafické kartě.",
    "Library, detail area and menu sets":
        "Knihovna, oblast detailu a sady menu",
    "The detail area in three levels from the library, ready to operate.":
        "Oblast detailu ve třech úrovních z knihovny, hned ovladatelná.",
    "**{{MeBibliothek}}** holds ready parts (button, slider with cvar, yes/no, tab bar, detail area in three levels, list); drag one onto the canvas or double-click it. **{{MeVorlageSpeichern}}** turns the selection into your own template. **{{MeMenuesatzOeffnen}}** opens every file of `menus.txt`; `open` in a script then switches to the file of the menu.":
        "**{{MeBibliothek}}** obsahuje hotové díly (tlačítko, posuvník s cvarem, ano/ne, lišta karet, oblast detailu ve třech úrovních, seznam); přetáhněte jeden na plátno nebo na něj dvakrát klikněte. **{{MeVorlageSpeichern}}** udělá z výběru vlastní šablonu. **{{MeMenuesatzOeffnen}}** otevře všechny soubory `menus.txt`; `open` ve skriptu pak přepne do souboru daného menu.",
    "Texts in several languages and sounds":
        "Texty v několika jazycích a zvuky",
    "An own text @MYMENU_START, shown in German in the preview.":
        "Vlastní text @MYMENU_START, v náhledu německy.",
    "**{{MeTexte}}** lists your own @-texts in every language of the game and the @-keys the menu uses (missing ones can be created with one click). They are written into the package as `strings/<language>/<PACKAGE>.str` (Jedi Academy, Movie Duels) or `strip/<package>.sp` (Jedi Outcast). The language box next to it switches the preview. In **{{MeBedienen}}** the menu plays its sounds like the game - `soundLoop`, `play`, `playlooped`, the focus sound of the item or of `assetGlobalDef` (**{{MeTonAn}}** in the toolbar switches them off).":
        "**{{MeTexte}}** vypisují vaše vlastní @-texty v každém jazyce hry a @-klíče, které menu používá (chybějící vytvoříte jedním kliknutím). Do balíčku se zapisují jako `strings/<jazyk>/<BALÍČEK>.str` (Jedi Academy, Movie Duels) nebo `strip/<balíček>.sp` (Jedi Outcast). Výběr jazyka vedle přepíná náhled. V režimu **{{MeBedienen}}** přehrává menu své zvuky jako hra - `soundLoop`, `play`, `playlooped`, zvuk fokusu prvku nebo z `assetGlobalDef` (**{{MeTonAn}}** na panelu nástrojů je vypne).",
    "Check, export and test in the game":
        "Kontrola, export a test ve hře",
    "**{{MePruefen}}** checks all open menus for Jedi Academy, Jedi Outcast and Movie Duels: the limits of each version (items per menu, menus, script length, multi values, colour ranges, list columns - the game drops what is beyond), items that are never visible, missing images, shaders, models, videos, fonts and sounds, and keywords or item types another version does not know. **{{MeExportPk3}}** writes all open menus or, with **{{MeExportAenderungen}}**, only the changed ones - each time with your package (images, videos, texts, fonts) and with a warning if the game would lose something.":
        "**{{MePruefen}}** zkontroluje všechna otevřená menu pro Jedi Academy, Jedi Outcast a Movie Duels: limity každé verze (prvky na menu, menu, délka skriptu, vícenásobné hodnoty, rozsahy barev, sloupce seznamů - co je nad nimi, hra zahodí), prvky, které nikdy nejsou vidět, chybějící obrázky, shadery, modely, videa, písma a zvuky a klíčová slova nebo typy prvků, které jiná verze nezná. **{{MeExportPk3}}** zapíše všechna otevřená menu nebo s **{{MeExportAenderungen}}** jen změněná - pokaždé s vaším balíčkem (obrázky, videa, texty, písma) a s varováním, pokud by hra něco ztratila.",
    "**{{MeImSpielTesten}}** puts the changed menus into `menuetest` in the settings folder - never into the game folder - and starts the game program found next to the game folders with `fs_homepath` pointing there, so your files replace the game's. A program that cannot be given its own folder asks before anything is written into a mod folder.":
        "**{{MeImSpielTesten}}** uloží změněná menu do `menuetest` ve složce nastavení - nikdy do složky hry - a spustí program hry nalezený vedle složek hry s `fs_homepath` nasměrovaným tam, takže vaše soubory nahradí soubory hry. U programu, kterému nelze dát vlastní složku, se před zápisem čehokoli do složky modu zeptá.",
    "Fonts from TrueType":
        "Písma z TrueType",
    "**{{MeSchriftBauen}}** turns a .ttf or .otf into a game font: `fonts/<name>.fontdat` (the exact format the game reads) and `fonts/<name>.tga` with all characters of the menu texts (Windows-1252). Both go into the package; the preview uses them at once.":
        "**{{MeSchriftBauen}}** převede .ttf nebo .otf na herní písmo: `fonts/<název>.fontdat` (přesně formát, který hra čte) a `fonts/<název>.tga` se všemi znaky textů menu (Windows-1252). Obojí jde do balíčku; náhled je hned používá.",
    # uebersetzung2: Zeitleiste neu, Menue-Editor M3-M5b, Fenster anordnen, VIS (nach Integration 4)
    "**See it in the timeline.** Open the figure's row in the timeline (the arrow at its name): its row **{{ZlBefehle}}** shows its waits and signals at their time.":
        "**Podívejte se na časovou osu.** Rozbalte řádek postavy na časové ose (šipka u jejího jména): její řádek **{{ZlBefehle}}** ukazuje její čekání a signály v jejich čase.",
    "**Shake.** Choose `SHAKE`: strength 0 to 16 and the duration in milliseconds. In the **{{ZlDetail}}** view the timeline shows zoom, fade and shake in their own rows.":
        "**Otřesy.** Zvolte `SHAKE`: síla 0 až 16 a délka v milisekundách. V pohledu **{{ZlDetail}}** ukazuje časová osa přiblížení, prolínání a otřesy ve vlastních řádcích.",
    "**Step exactly.** With the mouse over the timeline, <kbd>Left</kbd> / <kbd>Right</kbd> move the playhead by 50 ms, with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds.":
        "**Přesné krokování.** S myší nad časovou osou posouvají <kbd>←</kbd> / <kbd>→</kbd> ukazatel přehrávání o 50 ms, se <kbd>Shift</kbd> o sekundu, s <kbd>Ctrl</kbd> o pět sekund.",
    "**Check it.** Play the timeline. Open the figure's row: its row **{{ZlBefehle}}** shows the look commands at their time.":
        "**Zkontrolujte to.** Přehrajte časovou osu. Rozbalte řádek postavy: její řádek **{{ZlBefehle}}** ukazuje příkazy pohledu v jejich čase.",
    "**Watch it.** Turn on **{{MapThroughCam}}** and play the timeline: the view keeps turning towards the figure. In the **{{ZlDetail}}** view of the timeline, the **{{TlCamFollow}}** row shows how long it lasts.":
        "**Sledujte to.** Zapněte **{{MapThroughCam}}** a přehrajte časovou osu: pohled se stále otáčí k postavě. V pohledu **{{ZlDetail}}** časové osy ukazuje řádek **{{TlCamFollow}}**, jak dlouho to trvá.",
    "**Waits as blocks.** The **{{ZlSkript}}** row shows every `wait` as a hatched block with what it waits for - drag its right edge to change it directly.":
        "**Čekání jako bloky.** Řádek **{{ZlSkript}}** ukazuje každý `wait` jako šrafovaný blok s tím, na co čeká - tažením jeho pravého okraje ho přímo změníte.",
    "**Only active.** **{{ZlNurAktive}}** hides the actors that do nothing in the visible section.":
        "**Jen aktivní.** **{{ZlNurAktive}}** skryje postavy, které ve viditelném úseku nic nedělají.",
    "The timeline plays the scene and shows when everything happens - camera shots with names, speech with subtitles, music, the script's waits and every actor. Click a block and the side bar explains it; drag it and the script changes.":
        "Časová osa přehrává scénu a ukazuje, kdy se co děje - záběry kamery se jmény, řeč s titulky, hudbu, čekání skriptu a každou postavu. Klikněte na blok a postranní panel ho vysvětlí; tažením se změní skript.",
    "The timeline in the Overview: camera position and rotation with the names of the marks, speech with speaker and subtitle, music, the script flow with its waits, one row per actor.":
        "Časová osa v Přehledu: poloha a otočení kamery se jmény značek, řeč s mluvčím a titulkem, hudba, průběh skriptu s jeho čekáním, jeden řádek na postavu.",
    "Playing and moving around":
        "Přehrávání a pohyb",
    "The play button (▶) under the map starts and stops; with the mouse over the timeline <kbd>Space</kbd> does the same. Click or drag on the ruler to set the time - the 3D view shows the scene at that moment. <kbd>Left</kbd> / <kbd>Right</kbd> move by one game frame (50 ms, the grid of the game logic), with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds, <kbd>Home</kbd> jumps to the start. The legend under the rows lists these keys. The speed field slows the scene down (sound only plays at 1x), **{{TlLoop}}** repeats it. **{{ZlUt}}** shows the subtitles in the preview picture the way the game does (the game setting g_subtitles): with the script camera on, in the middle of the lower black bar, two lines at a time, each pair as long as its letters take of the sound; Movie Duels draws them smaller (cg_textprintscale, 0.66 by default). As in the game, Jedi Academy and Movie Duels show none by default, Jedi Outcast shows them in cinematics, and a figure with `SET_USE_SUBTITLES` always shows its lines; `SET_CAPTIONTEXTCOLOR` changes the colour.":
        "Tlačítko přehrávání (▶) pod mapou spouští a zastavuje; s myší nad časovou osou dělá totéž <kbd>Mezerník</kbd>. Kliknutím nebo tažením na pravítku nastavíte čas - 3D pohled ukazuje scénu v tomto okamžiku. <kbd>←</kbd> / <kbd>→</kbd> posunou o jeden herní snímek (50 ms, rastr herní logiky), se <kbd>Shift</kbd> o sekundu, s <kbd>Ctrl</kbd> o pět sekund, <kbd>Home</kbd> skočí na začátek. Legenda pod řádky tyto klávesy uvádí. Pole rychlosti scénu zpomalí (zvuk hraje jen při 1x), **{{TlLoop}}** ji opakuje. **{{ZlUt}}** zobrazuje titulky v obrazu náhledu tak, jak to dělá hra (herní nastavení g_subtitles): se zapnutou skriptovou kamerou uprostřed spodního černého pruhu, vždy dva řádky, každou dvojici tak dlouho, kolik zvuku zaberou její písmena; Movie Duels je kreslí menší (cg_textprintscale, ve výchozím stavu 0,66). Jako ve hře je Jedi Academy a Movie Duels ve výchozím stavu nezobrazují, Jedi Outcast je zobrazuje v animačních sekvencích a postava se `SET_USE_SUBTITLES` své repliky zobrazuje vždy; `SET_CAPTIONTEXTCOLOR` mění barvu.",
    "A scene starts with a section of about 30 seconds. **{{TlFit}}** shows the whole scene, − and + zoom out and in, <kbd>Ctrl</kbd>+wheel zooms around the mouse pointer, <kbd>Shift</kbd>+wheel scrolls. Only while zoomed in, a slim navigation bar above the ruler shows which part of the scene you see - click or drag it to move there.":
        "Scéna začíná úsekem asi 30 sekund. **{{TlFit}}** zobrazí celou scénu, − a + oddálí a přiblíží, <kbd>Ctrl</kbd>+kolečko přibližuje kolem ukazatele myši, <kbd>Shift</kbd>+kolečko posouvá. Jen při přiblížení ukazuje tenký navigační pruh nad pravítkem, kterou část scény vidíte - kliknutím nebo tažením se tam přesunete.",
    "Overview and Detail":
        "Přehled a Detail",
    "**{{ZlUebersicht}}** shows the scene at a glance: **{{ZlOrt}}** has one block per shot, from one camera move to the next, named after its mark (or as readable coordinates); **{{ZlDrehung}}** shows the pans, a pan without duration is marked as a cut. **{{ZlSprache}}** shows every line as long as its sound file, with speaker, subtitle from the game's string files (Jedi Academy and Movie Duels: strings/english/<map>.str, Jedi Outcast: strip/<map>.sp, with the extra packages some maps read) and the waveform. **{{ZlMusik}}** shows which music plays from when. **{{ZlSkript}}** shows the waits of the script as hatched blocks with what they wait for, and its commands as labelled marks; a map change is a red mark. Each actor has one row: meaningful animations with their names on top, its walks numbered below, a dashed line where it stands still, hatched where it is not visible. The arrow at the name opens its **{{ZlAnimation}}** and **{{ZlBefehle}}**.":
        "**{{ZlUebersicht}}** ukazuje scénu na první pohled: **{{ZlOrt}}** má jeden blok na záběr, od jednoho pohybu kamery k dalšímu, pojmenovaný podle své značky (nebo čitelnými souřadnicemi); **{{ZlDrehung}}** ukazuje švenky, švenk bez délky je označen jako střih. **{{ZlSprache}}** ukazuje každou repliku tak dlouhou jako její zvukový soubor, s mluvčím, titulkem z textových souborů hry (Jedi Academy a Movie Duels: strings/english/<mapa>.str, Jedi Outcast: strip/<mapa>.sp, včetně doplňkových balíčků, které některé mapy čtou) a průběhem vlny. **{{ZlMusik}}** ukazuje, která hudba hraje od kdy. **{{ZlSkript}}** ukazuje čekání skriptu jako šrafované bloky s tím, na co čekají, a jeho příkazy jako popsané značky; změna mapy je červená značka. Každá postava má jeden řádek: nahoře významné animace se jmény, dole její očíslované chůze, přerušovaná čára, kde stojí, šrafování, kde není vidět. Šipka u jména rozbalí její **{{ZlAnimation}}** a **{{ZlBefehle}}**.",
    "Detail: camera move, pan, zoom and fade separately, field of view and fade as lines. A move that starts before the previous one has finished gets its own lane and the note \"jumps back\".":
        "Detail: pohyb, švenk, přiblížení a prolínání kamery zvlášť, zorné pole a prolínání jako čáry. Pohyb, který začne dřív, než skončil předchozí, dostane vlastní dráhu a upozornění \"skočí zpět\".",
    "**{{ZlDetail}}** (or <kbd>Tab</kbd> over the timeline) splits the camera into move, pan, zoom and fade and draws the field of view and the fade as lines. They are straight between two commands because the game interpolates them linearly - there are no ease curves. If a camera move starts before the previous one of the same kind has finished, it gets a lane of its own and the red note **{{ZlSpringt}}**: the game only takes over position and angle at the end of a move (cg_camera.cpp), so the new move starts from where the previous one began - the camera jumps back.":
        "**{{ZlDetail}}** (nebo <kbd>Tab</kbd> nad časovou osou) rozdělí kameru na pohyb, švenk, přiblížení a prolínání a kreslí zorné pole a prolínání jako čáry. Mezi dvěma příkazy jsou rovné, protože je hra interpoluje lineárně - křivky náběhu neexistují. Když pohyb kamery začne dřív, než skončil předchozí téhož druhu, dostane vlastní dráhu a červené upozornění **{{ZlSpringt}}**: hra převezme polohu a úhel až na konci pohybu (cg_camera.cpp), nový pohyb tedy začne tam, kde začal předchozí - kamera skočí zpět.",
    "What happens here?":
        "Co se tu děje?",
    "A clicked block: the side bar explains it in plain words, with time, command and script line.":
        "Blok, na který jste klikli: postranní panel ho vysvětlí srozumitelnými slovy, s časem, příkazem a řádkem skriptu.",
    "Click any block or mark: the time jumps there, its line is selected in the script, and below the script column the box **{{ZlWas}}** says in plain words what it does - where the camera moves and how long, who speaks which line, what a wait waits for and who sends that signal, which animation plays for how long. **{{ZlZeileOeffnen}}** opens the command in the editor. A command that comes from another script (run, use, spawnscript) names that script and its line there - **{{ZlZeileOeffnen}}** opens that script and selects the line; one the simulation inserted has no line.":
        "Klikněte na libovolný blok nebo značku: čas tam skočí, jeho řádek se vybere ve skriptu a pod sloupcem skriptu řekne rámeček **{{ZlWas}}** srozumitelnými slovy, co dělá - kam se kamera pohybuje a jak dlouho, kdo říká kterou repliku, na co wait čeká a kdo ten signál posílá, která animace hraje a jak dlouho. **{{ZlZeileOeffnen}}** otevře příkaz v editoru. Příkaz, který pochází z jiného skriptu (run, use, spawnscript), uvádí ten skript a svůj řádek v něm - **{{ZlZeileOeffnen}}** otevře onen skript a vybere řádek; příkaz vložený simulací řádek nemá.",
    "Filters and search":
        "Filtry a hledání",
    "**{{ZlNurAktive}}** hides actors that do nothing in the visible section. **{{ZlProbleme}}** shows only places with notes: camera moves that jump back, commands the preview does not simulate yet, the map change. The search field filters by name, mark, file, subtitle or command (for example `luke`, `cam1b`, `SET_ANIM`).":
        "**{{ZlNurAktive}}** skryje postavy, které ve viditelném úseku nic nedělají. **{{ZlProbleme}}** ukáže jen místa s upozorněními: pohyby kamery, které skočí zpět, příkazy, které náhled zatím nenapodobuje, změnu mapy. Pole hledání filtruje podle jména, značky, souboru, titulku nebo příkazu (například `luke`, `cam1b`, `SET_ANIM`).",
    "Problems: only the rows with notes remain.":
        "Problémy: zůstanou jen řádky s upozorněními.",
    "Drag a camera block or a line left or right: the `wait` before it changes, so it happens earlier or later and everything after it moves along; with <kbd>Alt</kbd> the `wait` after it gets shorter by the same amount and the rest stays. Drag the right edge of a camera block to change its duration, the right edge of a wait to make it longer or shorter. A line keeps its length - it is the length of the sound file. A walk cannot be made longer or shorter: its duration comes from the path finding and the speed of the figure. Everything snaps to the game's 50 ms frames, and while you drag a note says what changes in the script.":
        "Přetáhněte blok kamery nebo repliku doleva či doprava: změní se `wait` před ním, takže se stane dřív nebo později, a vše za ním se posune s ním; s <kbd>Alt</kbd> se `wait` za ním zkrátí o stejnou hodnotu a zbytek zůstane na místě. Tažením pravého okraje bloku kamery změníte jeho délku, tažením pravého okraje čekání ho prodloužíte nebo zkrátíte. Replika si ponechá svou délku - je to délka zvukového souboru. Chůzi nelze prodloužit ani zkrátit: její délka vychází z hledání cesty a rychlosti postavy. Vše se přichytává k 50ms snímkům hry a během tažení upozornění říká, co se ve skriptu změní.",
    "A dragged wait: changed blocks are outlined with a dashed line until <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes the change back.":
        "Přetažené čekání: změněné bloky mají přerušovaný obrys, dokud <kbd>Ctrl</kbd>+<kbd>Z</kbd> změnu nevrátí.",
    "Every drag writes the value into the script (comments and formatting stay) and is exactly one step for <kbd>Ctrl</kbd>+<kbd>Z</kbd> - afterwards the script is byte for byte what it was.":
        "Každé tažení zapíše hodnotu do skriptu (komentáře a formátování zůstanou) a je přesně jeden krok pro <kbd>Ctrl</kbd>+<kbd>Z</kbd> - potom je skript bajt po bajtu takový, jaký byl.",
    "Recording a walk":
        "Nahrání chůze",
    "Recording: the path so far, the walk targets as numbered circles, the box of the figure and the hint at the bottom.":
        "Nahrávání: dosavadní cesta, cíle chůze jako očíslované kruhy, rámeček postavy a nápověda dole.",
    "Written is what the game uses to let an NPC walk: one `ref_tag` per point and one `set ( SET_NAVGOAL, ... )`, which waits in the game until the figure has arrived, with `SET_RUNNING`/`SET_WALKING` before it where the gait changes. Points are kept only where they are needed: where the path leaves a straight line by more than the tolerance (in units, adjustable while recording), where a straight line would run into a wall, and where the gait changes. The commands go into the figure's own commands at the time of the playhead (a new `affect` block if it has none) - one step for <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Afterwards the preview walks it like the game: between the points the NPC finds its own way at its own speed - the game has no timing per stretch, so the recording sets only the points and the gait.":
        "Zapíše se to, čím hra nechává NPC chodit: pro každý bod jeden `ref_tag` a jeden `set ( SET_NAVGOAL, ... )`, který ve hře čeká, dokud postava nedorazí, a před ním `SET_RUNNING`/`SET_WALKING` tam, kde se mění způsob pohybu. Body zůstanou jen tam, kde jsou potřeba: kde se cesta odchýlí od přímky víc než o toleranci (v jednotkách, nastavitelnou během nahrávání), kde by přímka narazila do zdi a kde se mění způsob pohybu. Příkazy se vloží do vlastních příkazů postavy v čase ukazatele přehrávání (nový blok `affect`, pokud žádný nemá) - jeden krok pro <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Potom ji náhled vede jako hra: mezi body si NPC hledá cestu sám, svou rychlostí - hra nezná čas pro jednotlivé úseky, proto nahrávání určí jen body a způsob pohybu.",
    "Subtitles in the preview picture, in cinematics: two lines in the middle of the lower bar.":
        "Titulky v obrazu náhledu, v animačních sekvencích: dva řádky uprostřed spodního pruhu.",
    "Times come from a re-creation of the game's ICARUS engine, including its quirks: a `wait` ends one game frame (50 ms) later than written, and a `dowait` waits for the task it names. A command the preview does not simulate yet (for example `camera ( TRACK )` or `print`) is shown as a red mark instead of being guessed. The rules of OpenJK or Movie Duels apply as set in [[einstellungen#prefs]].":
        "Časy pocházejí z napodobení enginu ICARUS hry, včetně jeho zvláštností: `wait` skončí o jeden herní snímek (50 ms) později, než je napsáno, a `dowait` čeká na úlohu, kterou jmenuje. Příkaz, který náhled zatím nenapodobuje (například `camera ( TRACK )` nebo `print`), se zobrazí jako červená značka, místo aby se hádal. Platí pravidla OpenJK nebo Movie Duels podle nastavení v [[einstellungen#prefs]].",
    "In the timeline a ROFF flight is a block in the row of its entity (\"ROFF name\"); drag it to start the flight earlier or later.":
        "Na časové ose je let ROFF blok v řádku své entity (\"ROFF název\"); tažením let začne dřív nebo později.",
    "**Full VIS**: before a `-vis` without `-fast` the console names the portals, the clusters and an estimate of the memory the VIS needs - read from the portal file of the BSP step, counted like q3map2's CreatePassages. bhed_q3map2 keeps of each passage only the part that is not empty (md_rev_TarisUpper2: 1.4 instead of 9 GB) and prints after CreatePassages what the passages really take. From the portal count of the optimizer's VIS hint, or when the memory comes close to what the computer has, a warning follows, also in the window: a full VIS can take hours, on open maps days. Its progress counts the expected work of the portals, not their number - the expensive ones come last - so the remaining time in the first build is close to the real one. Ways out: `-vis -fast` for test builds, small structural brushes that do not seal as detail (optimizer: **{{KeOptPrDeko}}**), and **{{KeTkVis}}** for later builds. For a map from File > Open .bsp it also says whether the original was built with `-vis -fast`.":
        "**Plné VIS**: před `-vis` bez `-fast` vypíše konzole portály, clustery a odhad paměti, kterou VIS potřebuje - načteno ze souboru portálů kroku BSP, spočítáno jako CreatePassages v q3map2. bhed_q3map2 si z každé pasáže ponechá jen tu část, která není prázdná (md_rev_TarisUpper2: 1,4 místo 9 GB), a po CreatePassages vypíše, kolik pasáže skutečně zabírají. Od počtu portálů z upozornění optimalizátoru na VIS, nebo když se paměť blíží tomu, co počítač má, následuje varování, i v okně: plné VIS může trvat hodiny, na otevřených mapách dny. Jeho průběh počítá očekávanou práci portálů, ne jejich počet - drahé přijdou nakonec -, takže zbývající čas je už při prvním sestavení blízko skutečnému. Východiska: `-vis -fast` pro zkušební sestavení, malé strukturální brushe, které neutěsňují, jako detail (optimalizátor: **{{KeOptPrDeko}}**) a **{{KeTkVis}}** pro pozdější sestavení. U mapy z File > Open .bsp se také uvede, zda byl originál sestaven s `-vis -fast`.",
    "**{{KeLBuild}} > {{KeVisExakt}}** (off by default): with several threads a full VIS can come out slightly different from build to build - as with NetRadiant-Custom's q3map2, where a portal uses whatever other portals happen to be finished; every result is valid. Switched on, every `-vis` step gets `-bhedvisexact`, and bhed_q3map2 computes with all threads exactly what one thread computes, byte for byte. You need it for **reproducible builds**: the same .map gives the same .bsp - to compare two builds, for checksums, or when several people build the same map. It waits where a portal needs one that is not finished yet - on closed maps about as fast, on open maps much slower (duel_mos_espa: 5 instead of 1.5 minutes). Off is the faster way.":
        "**{{KeLBuild}} > {{KeVisExakt}}** (ve výchozím stavu vypnuto): s více vlákny může plné VIS vyjít mezi sestaveními mírně odlišně - jako u q3map2 z NetRadiant-Custom, kde portál použije ty ostatní portály, které jsou právě hotové; každý výsledek je platný. Po zapnutí dostane každý krok `-vis` přepínač `-bhedvisexact` a bhed_q3map2 spočítá se všemi vlákny přesně to, co spočítá jedno vlákno, bajt po bajtu. Potřebujete to pro **reprodukovatelná sestavení**: stejná .map dá stejnou .bsp - k porovnání dvou sestavení, pro kontrolní součty nebo když stejnou mapu sestavuje více lidí. Čeká tam, kde portál potřebuje ještě nehotový portál - na uzavřených mapách zhruba stejně rychle, na otevřených mnohem pomaleji (duel_mos_espa: 5 místo 1,5 minuty). Vypnuto je rychlejší cesta.",
    "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} and {{KeTkLicht}} switched on, below them {{KeVisExakt}}, {{KeZfMenue}} and {{KeFsMelden}} (not in NetRadiant-Custom).":
        "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} a {{KeTkLicht}} zapnuté, pod nimi {{KeVisExakt}}, {{KeZfMenue}} a {{KeFsMelden}} (nejsou v NetRadiant-Custom).",
    "While the graphics card bakes, the processor works at the same time: it computes the lightmaps the graphics card leaves to it, takes over lightmaps the graphics card has not started yet, and shares the points of the light grid with it - whoever is faster does more. With `-randomsamples` (**[Final] Extreme Quality**) the graphics card computes every lightmap that needs no random supersampling; a lightmap with random supersampling is computed on the processor, with the same random numbers as on one thread. The result stays bit-identical.":
        "Zatímco grafická karta zapéká, pracuje současně procesor: počítá lightmapy, které mu grafická karta přenechá, přebírá lightmapy, které grafická karta ještě nezačala, a dělí se s ní o body světelné mřížky - kdo je rychlejší, udělá víc. S `-randomsamples` (**[Final] Extreme Quality**) počítá grafická karta každou lightmapu, která nepotřebuje náhodné nadvzorkování; lightmapa s náhodným nadvzorkováním se počítá na procesoru, se stejnými náhodnými čísly jako na jednom vlákně. Výsledek zůstává bitově shodný.",
    "`cinematic` and `videoMap` play RoQ videos; **{{MeVideoImport}}** converts a video or GIF to RoQ and puts it into `video/`. `ITEM_TYPE_MODEL` shows the model with the camera, light and rotation of the game; the character menu builds the player from the cvars and the feeders (species, heads, colours) like the game. Sabers lie on their side with hilt and blades from the .sab: length, radius, number of blades, single/staff, colour - glow and core drawn like the renderer. Movie Duels follows `cg_SFXSabers` with all its styles (see the next page).":
        "`cinematic` a `videoMap` přehrávají videa RoQ; **{{MeVideoImport}}** převede video nebo GIF do RoQ a uloží ho do `video/`. `ITEM_TYPE_MODEL` zobrazí model s kamerou, světlem a otáčením hry; menu postavy sestaví hráče z cvarů a feederů (druhy, hlavy, barvy) jako hra. Světelné meče leží na boku s jílcem a čepelemi ze .sab: délka, poloměr, počet čepelí, jednoduchý/dvojitý, barva - záře a jádro kreslené jako renderer. Movie Duels se řídí `cg_SFXSabers` se všemi jeho styly (viz další stránka).",
    "Menu editor: HUD, Asian texts and sabers":
        "Editor menu: HUD, asijské texty a světelné meče",
    "Edit the HUD with example values like the game draws it, write your own texts in Korean, Japanese and Chinese, all Movie Duels blade styles and black blades, the character's pose and the sounds like in the game.":
        "Upravujte HUD s ukázkovými hodnotami tak, jak ho kreslí hra, pište vlastní texty v korejštině, japonštině a čínštině, všechny styly čepelí Movie Duels a černé čepele, póza postavy a zvuky jako ve hře.",
    "HUD editor":
        "Editor HUD",
    "ui/hud.menu with health 70, armor 30, force 100 and the saber style medium - tics, numbers and crosshair drawn like the game.":
        "ui/hud.menu se zdravím 70, brněním 30, Silou 100 a středním stylem meče - dílky, čísla a zaměřovač kreslené jako ve hře.",
    "Open `ui/hud.menu` like any other menu. The game does not draw the HUD with the menu alone: the game code reads the position, colour and image of named items (`health_tic1` to `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) and draws tics, numbers and pictures with the player's values on top. The preview does exactly that: **{{MeHudWerte}}** in the properties has sliders for health, armor, force, ammo, the saber style and the crosshair; full tics are white, the last one fades with the rest, armor under a quarter blinks, ammo 0 turns red. Move or recolour an item and the HUD follows at once.":
        "Otevřete `ui/hud.menu` jako každé jiné menu. Hra nekreslí HUD jen pomocí menu: herní kód čte polohu, barvu a obrázek pojmenovaných itemů (`health_tic1` až `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) a kreslí přes ně dílky, čísla a obrázky s hodnotami hráče. Přesně to dělá náhled: **{{MeHudWerte}}** ve vlastnostech má posuvníky pro zdraví, brnění, Sílu, munici, styl meče a zaměřovač; plné dílky jsou bílé, poslední slábne se zbytkem, brnění pod čtvrtinou bliká, munice 0 zčervená. Posuňte nebo přebarvěte item a HUD ho hned následuje.",
    "This is reproduced for Jedi Academy single player. The multiplayer game code and Movie Duels draw their HUD differently (whole-number steps, HUD modes, block points); there the preview shows only the menus and says so.":
        "Napodobeno je to pro Jedi Academy pro jednoho hráče. Herní kód pro více hráčů a Movie Duels kreslí svůj HUD jinak (celočíselné kroky, režimy HUD, body bloku); tam náhled zobrazí jen menu a řekne to.",
    "Texts in Korean, Japanese and Chinese":
        "Texty v korejštině, japonštině a čínštině",
    "An own text in Korean, drawn from the game's Hangul glyph pages.":
        "Vlastní text v korejštině, nakreslený ze stránek glyfů hangul hry.",
    "**{{MeTexte}}** also offers Korean, Taiwanese, Japanese and Chinese. Type normally; the file gets the encoding in which the game's font reads that language (KSC 5601, Big5, Shift-JIS, GB 2312) - other languages stay Latin-1 like the menus. Characters the game cannot show as one character (for example Hangul outside KSC 5601 or half-width katakana) are stored as `?` and listed below the table. With the preview language set to one of these, the preview draws the characters from the game's glyph pages (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) at the game's size, spacing and scale.":
        "**{{MeTexte}}** nabízí také korejštinu, tchajwanštinu, japonštinu a čínštinu. Pište normálně; soubor dostane kódování, ve kterém písmo hry daný jazyk čte (KSC 5601, Big5, Shift-JIS, GB 2312) - ostatní jazyky zůstávají v Latin-1 jako menu. Znaky, které hra neumí zobrazit jako jeden znak (například hangul mimo KSC 5601 nebo katakana poloviční šířky), se uloží jako `?` a vypíšou pod tabulkou. Je-li jazyk náhledu nastaven na jeden z těchto jazyků, kreslí náhled znaky ze stránek glyfů hry (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) ve velikosti, rozestupu a měřítku hry.",
    "Movie Duels blade styles and black blades":
        "Styly čepelí Movie Duels a černé čepele",
    "With Movie Duels data the saber menu follows `cg_SFXSabers` for every style: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (default), 5 Original Trilogy, 6 Return of the Jedi, 7 The Force Awakens, 8 Unstable, 9 Rebels, 10 Clone Wars, 11 Maul - each with its own glow and core images, sizes (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) and shape. Unstable hilts always get the unstable blade. Black blades are drawn with their blend mode, so they darken what is behind them like in the game.":
        "S daty Movie Duels se menu mečů řídí `cg_SFXSabers` pro každý styl: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (výchozí), 5 Original Trilogy, 6 Return of the Jedi, 7 The Force Awakens, 8 Unstable, 9 Rebels, 10 Clone Wars, 11 Maul - každý s vlastními obrázky záře a jádra, velikostmi (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) a tvarem. Nestabilní jílce dostanou vždy nestabilní čepel. Černé čepele se kreslí se svým režimem míchání, takže jako ve hře ztmaví to, co je za nimi.",
    "The character's pose and the sounds":
        "Póza postavy a zvuky",
    "The animation of a model (`model_g2anim`, in the character menu `ui_char_anim`) starts when the game sets it - when the menus load, and again when a script such as `uiScript character` builds the character - and runs at the game's frame timing, blended between two frames, looping or stopping like the game. In **{{MeBedienen}}** scripts read exactly the words the game reads: `uiScript` takes its name and the words that script needs, `exec` one word - commands after them run on even without a `;`. `exec \"music …\"` starts the background music like the console.":
        "Animace modelu (`model_g2anim`, v menu postavy `ui_char_anim`) začne, když ji hra nastaví - při načtení menu a znovu, když skript jako `uiScript character` sestaví postavu - a běží v taktu snímků hry, prolínaná mezi dvěma snímky, ve smyčce nebo se zastavením jako ve hře. V režimu **{{MeBedienen}}** čtou skripty přesně ta slova, která čte hra: `uiScript` svůj název a slova, která daný skript potřebuje, `exec` jedno slovo - příkazy za nimi běží dál i bez `;`. `exec \"music …\"` spustí hudbu na pozadí jako konzole.",
    "Menu editor: videos, sound and readability":
        "Editor menu: videa, zvuk a čitelnost",
    "Import videos the way the game can play them - size, aspect ratio, frame rate, sound, the shader with videoMap - get clear messages for missing codecs, switch the preview sounds off and read the toolbars with labels.":
        "Importujte videa tak, jak je hra umí přehrát - velikost, poměr stran, snímková frekvence, zvuk, shader s videoMap -, dostávejte jasné zprávy při chybějících kodecích, vypněte zvuky náhledu a čtěte panely nástrojů s popisky.",
    "Importing a video":
        "Import videa",
    "The import dialog for a 1920 x 1080 video with sound: size 512 x 512, aspect ratio, frames per second, quality, sound (off by default), use in the menu and the estimated size.":
        "Dialog importu pro video 1920 x 1080 se zvukem: velikost 512 x 512, poměr stran, snímky za sekundu, kvalita, zvuk (ve výchozím stavu vypnutý), použití v menu a odhadovaná velikost.",
    "**{{MeVideoImport}}** reads MP4, WebM, AVI, WMV, MOV and MKV through Windows, animated GIFs, and RoQ files - a RoQ is taken over unchanged. Choose **{{MeVideoZielGroesse}}**: the game plays only sides that are powers of two, with at most 512 x 512 pixels in total (full-screen videos stop otherwise, larger videos overflow the game's video buffer); 512 x 512 is the default, like every Jedi Academy and Movie Duels video. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (the game stretches the video onto the element anyway), {{MeVideoBalken}} or {{MeVideoZuschneiden}}. Pictures are reduced by averaging all covered pixels, so fine patterns do not flicker. Below the options you see the length, the number of frames and the size the file will have.":
        "**{{MeVideoImport}}** čte MP4, WebM, AVI, WMV, MOV a MKV přes Windows, animované GIFy a soubory RoQ - RoQ se převezme beze změny. Zvolte **{{MeVideoZielGroesse}}**: hra přehrává jen strany, které jsou mocninami dvou, s nejvýše 512 x 512 pixely celkem (videa na celou obrazovku jinak skončí, větší videa přetečou vyrovnávací paměť videa hry); 512 x 512 je výchozí, jako u všech videí Jedi Academy a Movie Duels. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (hra video na prvek stejně roztáhne), {{MeVideoBalken}} nebo {{MeVideoZuschneiden}}. Obrázky se zmenšují průměrováním všech zakrytých pixelů, takže jemné vzory neblikají. Pod volbami vidíte délku, počet snímků a velikost, kterou soubor bude mít.",
    "The imported video as background of background_video in the Jedi Academy main menu - the preview plays it like the game.":
        "Importované video jako pozadí background_video v hlavním menu Jedi Academy - náhled ho přehrává jako hra.",
    "A menu shows a video through a shader with `videoMap`. The import writes it for you: `menu/video/<name>` in `shaders/<package>.shader`, next to `video/<name>.roq`. **{{MeVideoVerwendung}}** decides what happens in the menu: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, behind all others) or **{{MeVideoAlsCinematic}}** - only multiplayer menus draw that keyword, single-player menus show nothing (the check says so). Shader, video and menu go together into the exported .pk3 and into **{{MeImSpielTesten}}**.":
        "Menu zobrazuje video přes shader s `videoMap`. Import ho zapíše za vás: `menu/video/<název>` do `shaders/<balíček>.shader`, vedle `video/<název>.roq`. **{{MeVideoVerwendung}}** určuje, co se stane v menu: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, za všemi ostatními) nebo **{{MeVideoAlsCinematic}}** - toto klíčové slovo kreslí jen menu pro více hráčů, menu pro jednoho hráče nezobrazí nic (kontrola to řekne). Shader, video a menu jdou společně do exportované .pk3 a do **{{MeImSpielTesten}}**.",
    "Sound":
        "Zvuk",
    "Menus always play their videos silently - the game starts them without sound. **{{MeVideoTon}}** is therefore off; switch it on for full-screen videos that the game plays with sound (for example `cinematic <name>` from the console or a script). The sound is stored like in the game's videos: 22050 Hz, mono or stereo, between the frames. A video without a sound track gets a note. **{{MeTonAn}}** / **{{MeTonAus}}** in the toolbar switches all preview sounds - focus and click sounds and the menu music - and stops sounds that are playing; the setting is remembered.":
        "Menu přehrávají svá videa vždy bez zvuku - hra je spouští bez zvuku. **{{MeVideoTon}}** je proto vypnuté; zapněte ho pro videa na celou obrazovku, která hra přehrává se zvukem (například `cinematic <název>` z konzole nebo skriptu). Zvuk se ukládá jako ve videích hry: 22050 Hz, mono nebo stereo, mezi snímky. Video bez zvukové stopy dostane upozornění. **{{MeTonAn}}** / **{{MeTonAus}}** v panelu nástrojů zapíná a vypíná všechny zvuky náhledu - zvuky fokusu a kliknutí a hudbu menu - a zastaví právě hrající zvuky; nastavení se pamatuje.",
    "Missing codecs":
        "Chybějící kodeky",
    "If Windows cannot decode a video, the dialog says which codec it is and what helps: HEVC, AV1 and VP8/VP9 need a free extension from the Microsoft Store, AVI files with DivX/Xvid or unknown codecs should be converted to MP4 (H.264). Nothing is stored in that case - no empty video.":
        "Pokud Windows video nedokáže dekódovat, dialog řekne, o jaký kodek jde a co pomůže: HEVC, AV1 a VP8/VP9 potřebují bezplatné rozšíření z Microsoft Storu, soubory AVI s DivX/Xvid nebo neznámými kodeky převeďte do MP4 (H.264). V takovém případě se nic neuloží - žádné prázdné video.",
    "Readability":
        "Čitelnost",
    "The toolbars with labels; eye and lock in the hierarchy are real symbols.":
        "Panely nástrojů s popisky; oko a zámek v hierarchii jsou skutečné symboly.",
    "Every toolbar symbol names itself and what it does when you hover it; **{{MeEinstLeisteText}}** in the settings puts the name next to each symbol. In the hierarchy the eye hides an element in the preview only and the lock keeps it from being moved. Text never gets smaller than 75 % of the normal size (12 pixels), however far you zoom out with Ctrl and the mouse wheel.":
        "Každý symbol panelů nástrojů při najetí myší řekne svůj název a co dělá; **{{MeEinstLeisteText}}** v nastavení napíše název vedle každého symbolu. V hierarchii oko skryje prvek jen v náhledu a zámek brání jeho posunutí. Písmo nikdy není menší než 75 % normální velikosti (12 pixelů), ať oddálíte pomocí Ctrl a kolečka myši jakkoli.",
    "Menu editor: layouts and events":
        "Editor menu: rozvržení a události",
    "Arrange elements side by side, stacked, in a grid or in two columns with one click, and decide what happens on hover, click, focus and Enter with ready-made effects and action blocks - the result stays ordinary .menu code.":
        "Uspořádejte prvky jedním kliknutím vedle sebe, pod sebe, do mřížky nebo do dvou sloupců a hotovými efekty a bloky akcí určete, co se stane při najetí myší, kliknutí, fokusu a Enteru - výsledkem zůstává běžný kód .menu.",
    "Layouts":
        "Rozvržení",
    "Three buttons in a panel, arranged with Side by side: margins 8, spacing 8 - the layout is kept as a comment line.":
        "Tři tlačítka v panelu, uspořádaná pomocí Vedle sebe: okraje 8, mezera 8 - rozvržení zůstane jako řádek komentáře.",
    "Select two or more elements (Ctrl+click on the canvas or in the hierarchy) and click **{{MeLayoutWaagrecht}}**, **{{MeLayoutSenkrecht}}**, **{{MeLayoutRaster}}** or **{{MeLayoutTeiler}}** in the **{{MeLayout}}** section (Ctrl+Shift+1 to 4). If one of the selected elements encloses all the others - a panel or background - it becomes the area and the others are placed inside it; otherwise the layout uses the box around the selection. Without a selection, **{{MeLayoutZeigMir}}** selects the first elements for you.":
        "Vyberte dva nebo více prvků (Ctrl+kliknutí na plátně nebo v hierarchii) a v oddílu **{{MeLayout}}** klikněte na **{{MeLayoutWaagrecht}}**, **{{MeLayoutSenkrecht}}**, **{{MeLayoutRaster}}** nebo **{{MeLayoutTeiler}}** (Ctrl+Shift+1 až 4). Pokud jeden z vybraných prvků obklopuje všechny ostatní - panel nebo pozadí -, stane se plochou a ostatní se umístí do něj; jinak rozvržení použije rámeček kolem výběru. Bez výběru **{{MeLayoutZeigMir}}** vybere první prvky za vás.",
    "Select an element of the layout to change it: **{{MeLayoutArt}}**, **{{MeLayoutAbstand}}**, **{{MeLayoutRaender}}**, alignment (**{{MeLayoutAnfang}}**, **{{MeLayoutMitte}}**, **{{MeLayoutEnde}}**, **{{MeLayoutFuellen}}**), **{{MeLayoutSpalten}}** of a grid, **{{MeLayoutTeilung}}** and **{{MeLayoutGriff}}** of two columns. **{{MeLayoutSpacerNeu}}** puts a spacer behind the selected element - it takes the free space, for example to push a button to the right edge. Move the area and the elements follow. Every change is one undo step.":
        "Vyberte prvek rozvržení a změňte ho: **{{MeLayoutArt}}**, **{{MeLayoutAbstand}}**, **{{MeLayoutRaender}}**, zarovnání (**{{MeLayoutAnfang}}**, **{{MeLayoutMitte}}**, **{{MeLayoutEnde}}**, **{{MeLayoutFuellen}}**), **{{MeLayoutSpalten}}** mřížky, **{{MeLayoutTeilung}}** a **{{MeLayoutGriff}}** u dvou sloupců. **{{MeLayoutSpacerNeu}}** vloží výplň za vybraný prvek - zabere volné místo, například aby odsunula tlačítko k pravému okraji. Posuňte plochu a prvky se posunou s ní. Každá změna je jeden krok zpět.",
    "The game knows no layouts: the .menu only gets fixed `rect` values, and the layout is stored as a comment `// jkstudio:layout …` inside the menuDef, which the game skips and the editor reads again. Menus without a layout stay byte-identical. For texts, the width of the longest translation in all languages of the game data counts; if a text does not fit, the section names the element and the language. **{{MeLayoutVersteckt}}** lets elements that are hidden after opening (`visible 0` or `hide` in onOpen) give up their place. **{{MeLayoutAufloesen}}** removes only the comment - the rects stay.":
        "Hra rozvržení nezná: do .menu se dostanou jen pevné hodnoty `rect` a rozvržení se uloží jako komentář `// jkstudio:layout …` uvnitř menuDef, který hra přeskočí a editor znovu přečte. Menu bez rozvržení zůstanou bajtově shodná. U textů se počítá šířka nejdelšího překladu ve všech jazycích herních dat; pokud se text nevejde, oddíl uvede prvek a jazyk. **{{MeLayoutVersteckt}}** nechá prvky skryté po otevření (`visible 0` nebo `hide` v onOpen) uvolnit své místo. **{{MeLayoutAufloesen}}** odstraní jen komentář - recty zůstanou.",
    "Events":
        "Události",
    "The button got the Glow effect and a click sound: the glow element appears on hover, the click plays the sound.":
        "Tlačítko dostalo efekt Záře a zvuk kliknutí: zářící prvek se objeví při najetí myší, kliknutí přehraje zvuk.",
    "The **{{MeEreignisse}}** section shows for the selected element what happens on **{{MeEvHover}}** (mouseEnter), **{{MeEvHoverEnde}}** (mouseExit), **{{MeEvKlick}}** (action), **{{MeEvFokus}}** (onFocus), **{{MeEvFokusEnde}}** (leaveFocus) and **{{MeEvEnter}}** (accept) - only the events the game's engine knows. Start with a ready-made hover effect: **{{MeEvLeuchten}}** (a glow element behind it, like the original menus), **{{MeEvFarbe}}**, **{{MeEvRahmen}}** or **{{MeEvGroesser}}**, in the colour next to them. **{{MeEvHoverKlang}}** sets `focusSound`, **{{MeEvKlickKlang}}** a `play` at the start of the click; the button next to each field lets you listen.":
        "Oddíl **{{MeEreignisse}}** ukazuje pro vybraný prvek, co se stane při **{{MeEvHover}}** (mouseEnter), **{{MeEvHoverEnde}}** (mouseExit), **{{MeEvKlick}}** (action), **{{MeEvFokus}}** (onFocus), **{{MeEvFokusEnde}}** (leaveFocus) a **{{MeEvEnter}}** (accept) - jen události, které engine hry zná. Začněte hotovým efektem při najetí: **{{MeEvLeuchten}}** (zářící prvek za ním, jako v původních menu), **{{MeEvFarbe}}**, **{{MeEvRahmen}}** nebo **{{MeEvGroesser}}**, v barvě vedle nich. **{{MeEvHoverKlang}}** nastaví `focusSound`, **{{MeEvKlickKlang}}** `play` na začátku kliknutí; tlačítkem vedle každého pole si zvuk poslechnete.",
    "Each event lists its actions as blocks: element names as a list, colours with a colour picker, sounds with a listen button. **{{MeEvAktionNeu}}** offers only the commands of this game's engine (the commandList of ui_shared, so `setcvartocvar` only in multiplayer). The blocks write the script text - the Text tab shows the same at once - and every change is one undo step. Switch on **{{MeBedienen}}** and the events run like in the game.":
        "Každá událost vypisuje své akce jako bloky: názvy prvků jako seznam, barvy s výběrem barvy, zvuky s tlačítkem k poslechu. **{{MeEvAktionNeu}}** nabízí jen příkazy enginu této hry (commandList z ui_shared, takže `setcvartocvar` jen pro více hráčů). Bloky zapisují text skriptu - karta Text ukáže hned totéž - a každá změna je jeden krok zpět. Zapněte **{{MeBedienen}}** a události běží jako ve hře.",
    "Preview checked against the original game":
        "Náhled ověřený proti původní hře",
    "All Jedi Academy menus outside a running game were photographed in the original game (single and multiplayer) and compared with the preview. Since then the preview also draws: missing `@` texts as nothing (like the game), missing images of shader elements as the game's grey default box, the colours, heads and parts of the species set in `ui_char_model`, the multiplayer start values (player name, model, server name, limits), the multiplayer head list in rows and columns, and the multiplayer owner-draws for team slots, game type, skill, team colour and map preview.":
        "Všechna menu Jedi Academy mimo běžící hru byla vyfotografována v původní hře (pro jednoho i více hráčů) a porovnána s náhledem. Od té doby náhled kreslí také: chybějící texty `@` jako nic (jako hra), chybějící obrázky prvků se shaderem jako šedý výchozí rámeček hry, barvy, hlavy a části druhu nastaveného v `ui_char_model`, počáteční hodnoty hry pro více hráčů (jméno hráče, model, název serveru, limity), seznam hlav pro více hráčů v řádcích a sloupcích a owner-draw pro více hráčů pro místa v týmu, typ hry, obtížnost, barvu týmu a náhled mapy.",
    "Menu editor: projects, mod profiles, Jedi Outcast":
        "Editor menu: projekty, profily modů, Jedi Outcast",
    "Keep a menu mod in a project folder laid out like the pk3, with automatic backup and restore after a crash; tell the editor what a mod's engine knows with a mod profile; layouts that follow show and hide; Jedi Outcast menus tested in the real game.":
        "Mějte mod menu ve složce projektu uspořádané jako pk3, s automatickou zálohou a obnovením po pádu; profilem modu řekněte editoru, co zná engine modu; rozvržení, která se řídí show a hide; menu Jedi Outcast ověřená ve skutečné hře.",
    "Projects":
        "Projekty",
    "**{{MePrjProjekt}} > {{MePrjNeu}}** creates a project file `*.jksmenu` with the folders `ui`, `gfx`, `video`, `shaders` and `strings` next to it - the same layout as the finished pk3. Menus that are open come along; without one, the project starts with an empty full-screen menu. In a project, **{{MeSpeichern}}** saves everything: the menus, their images, videos, texts and fonts, and the project file. The project file remembers the game, the mod profile, the menus and which one was open, the preview (resolution, letterbox, language, cvars) and the export (pk3 name, only changes). **{{MePrjExport}}** packs the pk3.":
        "**{{MePrjProjekt}} > {{MePrjNeu}}** vytvoří soubor projektu `*.jksmenu` a vedle něj složky `ui`, `gfx`, `video`, `shaders` a `strings` - stejné uspořádání jako hotová pk3. Otevřená menu se přidají; bez nich začne projekt prázdným menu na celou obrazovku. V projektu uloží **{{MeSpeichern}}** vše: menu, jejich obrázky, videa, texty a písma i soubor projektu. Soubor projektu si pamatuje hru, profil modu, menu a které bylo otevřené, náhled (rozlišení, pruhy, jazyk, cvary) a export (název pk3, jen změny). **{{MePrjExport}}** zabalí pk3.",
    "Every 30 seconds the editor backs up menus that are not saved yet. If JK Studio was not closed properly, it asks the next time whether to restore them - the restored text counts as an unsaved change, so Undo goes back to the saved version. A double-click on a `.jksmenu` opens JK Studio once you allowed it with **{{MePrjZuordnen}}** (only for your Windows account).":
        "Každých 30 sekund editor zálohuje menu, která ještě nejsou uložená. Pokud JK Studio nebylo řádně ukončeno, příště se zeptá, zda je obnovit - obnovený text se počítá jako neuložená změna, takže Zpět vrátí uloženou verzi. Dvojklik na `.jksmenu` otevře JK Studio, jakmile to povolíte přes **{{MePrjZuordnen}}** (jen pro váš účet Windows).",
    "After a crash: the editor offers the backed-up menus.":
        "Po pádu: editor nabídne zálohovaná menu.",
    "Mod profiles":
        "Profily modů",
    "A mod's engine may know more than the game: extra keywords, owner-draw and feeder numbers, uiScripts, cvars and menu lists. Choose the mod's profile under **{{MePrjProjekt}} > {{MeProfil}}**: the editor then reads these keywords without a message, shows owner-draws as labelled placeholders and lists the values under **Mod keywords** - the file stays exactly as it is. With the mod's source code, `tools/gen_menue_profil.py` writes the profile; without it, **{{MeProfilVorlage}}** writes a template to fill in. Jedi Academy, Jedi Outcast and Movie Duels have built-in profiles.":
        "Engine modu může znát víc než hra: další klíčová slova, čísla owner-draw a feederů, uiScripts, cvary a seznamy menu. Vyberte profil modu v **{{MePrjProjekt}} > {{MeProfil}}**: editor pak tato klíčová slova čte bez hlášení, zobrazuje owner-draw jako popsané zástupné symboly a vypisuje hodnoty pod **Klíčová slova modu** - soubor zůstane přesně takový, jaký je. Se zdrojovým kódem modu zapíše profil `tools/gen_menue_profil.py`; bez něj zapíše **{{MeProfilVorlage}}** šablonu k vyplnění. Jedi Academy, Jedi Outcast a Movie Duels mají vestavěné profily.",
    "A mod keyword from the profile, labelled in the properties.":
        "Klíčové slovo modu z profilu, popsané ve vlastnostech.",
    "Layouts that follow show and hide":
        "Rozvržení, která se řídí show a hide",
    "With **{{MeLayoutVersteckt}}** switched on, every script that shows or hides elements of the layout (a click, a hover, onOpen) also gets `setitemrect` lines that move the other elements into place - calculated for the state after that script and kept up to date with every layout change. Jedi Outcast has no `setitemrect`; there the editor explains what to do instead.":
        "Se zapnutým **{{MeLayoutVersteckt}}** dostane každý skript, který zobrazuje nebo skrývá prvky rozvržení (kliknutí, najetí myší, onOpen), také řádky `setitemrect`, které posunou ostatní prvky na místo - spočítané pro stav po tomto skriptu a aktualizované při každé změně rozvržení. Jedi Outcast `setitemrect` nemá; tam editor vysvětlí, co dělat místo toho.",
    "Button 2 is hidden after opening; Show and Hide move button 3 like the layout.":
        "Tlačítko 2 je po otevření skryté; Zobrazit a Skrýt posouvají tlačítko 3 jako rozvržení.",
    "Jedi Outcast":
        "Jedi Outcast",
    "The Jedi Outcast menus were compared with the real game (Steam version). The editor now follows its engine: no `setitemrect`, `setitemtext`, `transition2` or `delay`, no `accept` event, the logo models (`.md3`) with the game's camera. **{{MeImSpielTesten}}** starts `jk2sp.exe` with your own folder as `fs_basepath` and the game data via `fs_cdpath` - nothing is written into the game folder.":
        "Menu Jedi Outcast byla porovnána se skutečnou hrou (verze ze Steamu). Editor se nyní řídí jejím enginem: žádné `setitemrect`, `setitemtext`, `transition2` ani `delay`, žádná událost `accept`, modely loga (`.md3`) s kamerou hry. **{{MeImSpielTesten}}** spustí `jk2sp.exe` s vaší vlastní složkou jako `fs_basepath` a herními daty přes `fs_cdpath` - do složky hry se nic nezapíše.",
    "Your first own menu":
        "Vaše první vlastní menu",
    "From an empty editor to a project with three buttons side by side that glow on hover - saved, ready to pack as a pk3.":
        "Od prázdného editoru k projektu se třemi tlačítky vedle sebe, která při najetí myší září - uloženo a připraveno k zabalení jako pk3.",
    "Menu editor · about 5 minutes":
        "Editor menu · asi 5 minut",
    "The pictures were taken while the program did exactly these steps; the self-test checks after each step that it really happened. In the program, the helper (F1) runs this tutorial as a tour and moves on by itself when a step is done.":
        "Obrázky vznikly, zatímco program prováděl přesně tyto kroky; autotest po každém kroku ověřuje, že se opravdu stal. V programu pomocník (F1) předvede tento tutoriál jako prohlídku a sám pokračuje, jakmile je krok hotový.",
    "**Open the menu editor.** Click **{{ViewMenuEditor}}** in the mode bar.":
        "**Otevřete editor menu.** Klikněte v liště režimů na **{{ViewMenuEditor}}**.",
    "The empty menu editor with the project buttons.":
        "Prázdný editor menu s tlačítky projektu.",
    "**Start a project.** Click **{{MePrjNeu}}** and choose a folder for the project file. The editor creates the folders like in a pk3 and an empty full-screen menu.":
        "**Založte projekt.** Klikněte na **{{MePrjNeu}}** a vyberte složku pro soubor projektu. Editor vytvoří složky jako v pk3 a prázdné menu na celou obrazovku.",
    "The new project with its first menu.":
        "Nový projekt s prvním menu.",
    "**Three buttons.** Click **{{MeWButton}}** in the building blocks three times.":
        "**Tři tlačítka.** Klikněte v součástech třikrát na **{{MeWButton}}**.",
    "Three new buttons, still on top of each other.":
        "Tři nová tlačítka, zatím přes sebe.",
    "**Side by side.** Select the three buttons (Ctrl+click) and click **{{MeLayoutWaagrecht}}** under Layout in the properties.":
        "**Vedle sebe.** Vyberte tři tlačítka (Ctrl+kliknutí) a ve vlastnostech pod Rozvržení klikněte na **{{MeLayoutWaagrecht}}**.",
    "The buttons side by side with even spacing.":
        "Tlačítka vedle sebe se stejnými mezerami.",
    "**Glow on hover.** Select the first button, open the **Events** tab and click **{{MeEvLeuchten}}**.":
        "**Záře při najetí.** Vyberte první tlačítko, otevřete kartu **Události** a klikněte na **{{MeEvLeuchten}}**.",
    "The glow effect: show on hover, hide when the mouse leaves.":
        "Efekt záře: zobrazit při najetí myší, skrýt, když myš odjede.",
    "**Save.** Click **{{MeSpeichern}}** - in a project it saves everything. **{{MePrjProjekt}} > {{MePrjExport}}** then packs the pk3 for the game.":
        "**Uložte.** Klikněte na **{{MeSpeichern}}** - v projektu to uloží vše. **{{MePrjProjekt}} > {{MePrjExport}}** pak zabalí pk3 pro hru.",
    "Saved: the project file and the menu in the project folder.":
        "Uloženo: soubor projektu a menu ve složce projektu.",
    "Arrange windows and screens":
        "Uspořádání oken a obrazovek",
    "Take a panel out":
        "Vyjmutí panelu",
    "The menu {{DockMenu}} of the mission: show or hide panels, put them into their own window, save and load layouts.":
        "Menu {{DockMenu}} mise: zobrazit nebo skrýt panely, dát je do vlastního okna, uložit a načíst rozvržení.",
    "**{{DockPanels}}** hides a panel completely; its space goes to the neighbours as well. The section **{{DockOther}}** opens the helper, the tutorials and the messages - they are windows of their own anyway.":
        "**{{DockPanels}}** skryje panel úplně; i jeho místo připadne sousedům. Oddíl **{{DockOther}}** otevírá pomocníka, tutoriály a zprávy - ta jsou tak jako tak vlastními okny.",
    "Script Flow dropped onto the timeline window: both panels in one window, as tabs. The mission view uses the whole main window.":
        "Script Flow puštěný na okno časové osy: oba panely v jednom okně, jako karty. Pohled mise využívá celé hlavní okno.",
    "Dock a panel back":
        "Zpětné ukotvení panelu",
    "Or click the panel under **{{DockOwnWindow}}** again.":
        "Nebo na panel v **{{DockOwnWindow}}** klikněte znovu.",
    "Save, load and reset layouts":
        "Uložení, načtení a obnovení rozvržení",
    "Screens that change":
        "Když se obrazovky mění",
    "A screen is unplugged or arranged differently: windows that were on it come back onto the screen of the main window, at their size.":
        "Obrazovka se odpojí nebo se jinak uspořádá: okna, která na ní byla, se vrátí na obrazovku hlavního okna, ve své velikosti.",
    "A screen is smaller than a window: the window is made just small enough to fit.":
        "Obrazovka je menší než okno: okno se zmenší právě tak, aby se vešlo.",
    "A window moves onto a screen with a different scale (100 %, 150 % ...): its size and text follow the scale of that screen.":
        "Okno se přesune na obrazovku s jiným měřítkem (100 %, 150 % ...): jeho velikost a písmo se řídí měřítkem té obrazovky.",
    "Map editor":
        "Editor map",
    "The first value of `camera` chooses the command. behaved shows every one of them in the timeline (in the **{{ZlDetail}}** view in its own row) and plays it in the view. Times are in milliseconds.":
        "První hodnota `camera` volí příkaz. behaved zobrazuje každý z nich na časové ose (v pohledu **{{ZlDetail}}** ve vlastním řádku) a přehrává ho v pohledu. Časy jsou v milisekundách.",
    "Shows the whole scene.":
        "Zobrazí celou scénu.",
    "One row per actor and the camera shots with their names - the scene at a glance. [[zeitleiste#zeilen]]":
        "Jeden řádek na postavu a záběry kamery se jmény - scéna na první pohled. [[zeitleiste#zeilen]]",
    "Camera move, pan, zoom and fade separately, field of view and fade as lines, every actor's animations and commands.":
        "Pohyb, švenk, přiblížení a prolínání kamery zvlášť, zorné pole a prolínání jako čáry, animace a příkazy každé postavy.",
    "Only shows actors that do something in the visible section.":
        "Zobrazí jen postavy, které ve viditelném úseku něco dělají.",
    "Only shows places with notes: camera moves that jump back, commands the preview does not simulate.":
        "Zobrazí jen místa s upozorněními: pohyby kamery, které skočí zpět, příkazy, které náhled nenapodobuje.",
    "**See it in the timeline.** The **{{ZlSkript}}** row shows every wait as a hatched block that says what it waits for, and a click on it names who sends the signal and when. If no script sends the signal, the side bar says so - in the game the script would wait forever.":
        "**Podívejte se na časovou osu.** Řádek **{{ZlSkript}}** ukazuje každé čekání jako šrafovaný blok, který říká, na co čeká, a kliknutí na něj uvede, kdo signál posílá a kdy. Pokud signál neposílá žádný skript, řekne to postranní panel - ve hře by skript čekal navždy.",
    "**See them in the timeline.** In the **{{ZlDetail}}** view the timeline has its own rows for zoom, fade and shake, and draws the field of view and the fade as lines.":
        "**Podívejte se na ně na časové ose.** V pohledu **{{ZlDetail}}** má časová osa vlastní řádky pro přiblížení, prolínání a otřesy a kreslí zorné pole a prolínání jako čáry.",
    "**Change a duration.** Drag the right edge of a camera block or a wait to make it shorter or longer. A walk keeps its length: it comes from the path finding and the speed of the figure.":
        "**Změňte délku.** Tažením pravého okraje bloku kamery nebo čekání ho zkrátíte nebo prodloužíte. Chůze si ponechá svou délku: vychází z hledání cesty a rychlosti postavy.",
    "**Step exactly.** With the mouse over the timeline, <kbd>Left</kbd> / <kbd>Right</kbd> move the playhead by 50 ms, with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds. The time display shows where you are; click it to switch to frames.":
        "**Přesné krokování.** S myší nad časovou osou posouvají <kbd>←</kbd> / <kbd>→</kbd> ukazatel přehrávání o 50 ms, se <kbd>Shift</kbd> o sekundu, s <kbd>Ctrl</kbd> o pět sekund. Zobrazení času ukazuje, kde jste; kliknutím na něj přepnete na snímky.",
    "**Rows.** **{{ZlUebersicht}}** shows one row per actor, **{{ZlDetail}}** the camera split into move, pan, zoom and fade and every actor's animations and commands. **{{ZlNurAktive}}** hides actors that do nothing in the visible section.":
        "**Řádky.** **{{ZlUebersicht}}** ukazuje jeden řádek na postavu, **{{ZlDetail}}** kameru rozdělenou na pohyb, švenk, přiblížení a prolínání a animace a příkazy každé postavy. **{{ZlNurAktive}}** skryje postavy, které ve viditelném úseku nic nedělají.",
    "**Camera rows.** Switch to **{{ZlDetail}}** to see move, pan, zoom, fade and shake in their own rows. A move that starts before the previous one has finished gets its own lane and the red note **{{ZlSpringt}}** - in the game the camera jumps back; shorten the earlier one or start the later one after it.":
        "**Řádky kamery.** Přepněte na **{{ZlDetail}}** a uvidíte pohyb, švenk, přiblížení, prolínání a otřesy ve vlastních řádcích. Pohyb, který začne dřív, než skončil předchozí, dostane vlastní dráhu a červené upozornění **{{ZlSpringt}}** - ve hře kamera skočí zpět; zkraťte dřívější pohyb nebo nechte pozdější začít až po něm.",
    "**Check it.** Open the figure's row in the timeline: its row **{{ZlBefehle}}** shows the look commands at their time.":
        "**Zkontrolujte to.** Rozbalte řádek postavy na časové ose: její řádek **{{ZlBefehle}}** ukazuje příkazy pohledu v jejich čase.",
    "The rows of the opened figure: walks above, its commands below.":
        "Řádky rozbalené postavy: nahoře chůze, pod nimi její příkazy.",
    "**Time it.** A `wait` after the sound keeps the next command until the line is over. In the **{{ZlSprache}}** row the line is as long as its sound file - drag the right edge of the wait in **{{ZlSkript}}** until it fits.":
        "**Načasujte to.** `wait` po zvuku zdrží další příkaz, dokud replika neskončí. V řádku **{{ZlSprache}}** je replika stejně dlouhá jako její zvukový soubor - táhněte pravý okraj waitu v **{{ZlSkript}}**, dokud nesedí.",
    # uebersetzung2 nach Integration 5
    "Menu editor: flow, tab order, styles and templates":
        "Editor menu: tok, pořadí tabulátoru, styly a šablony",
    "See which menu opens which and where players get stuck, set the keyboard order with clicks, change colours and fonts everywhere at once, start from ready-made menus or a settings wizard, trace a design over a reference image and reset elements to the game's original.":
        "Podívejte se, které menu otevírá které a kde hráči uvíznou, nastavte pořadí klávesnice kliknutím, změňte barvy a písma všude najednou, začněte hotovými menu nebo průvodcem nastavením, obkreslete návrh přes referenční obrázek a vraťte prvky do originálu hry.",
    "Menu flow":
        "Tok menu",
    "The flow of a small menu set: the main menu (green frame) opens Options and Credits; Credits is a dead end (red), 'waise' is never opened (grey).":
        "Tok malé sady menu: hlavní menu (zelený rámeček) otevírá Options a Credits; Credits je slepá ulička (červená), 'waise' se nikdy neotevře (šedá).",
    "**{{MeFluss}}** in the toolbar shows every menu of the open files as a box and every `open` as an arrow, column by column from the menus the game opens itself (green frame: the main menu, the loading screen, the in-game menus - taken from the engine code). Red boxes are dead ends - they take input but nothing leads out - or targets that no open file defines; orange boxes are opened from another menu but have no way back (no `close` of themselves, no onESC, no `open` of the menu they came from); grey boxes are never reached. The list below says in plain words what to do. Click a box or a line of the list to jump to that menu. Open the whole menu set first, otherwise menus from other files look missing.":
        "**{{MeFluss}}** v panelu nástrojů zobrazí každé menu otevřených souborů jako rámeček a každé `open` jako šipku, sloupec po sloupci od menu, která otevírá hra sama (zelený rámeček: hlavní menu, obrazovka načítání, menu během hry - převzato z kódu enginu). Červené rámečky jsou slepé uličky - přijímají vstup, ale nic nevede ven - nebo cíle, které žádný otevřený soubor nedefinuje; oranžové se otevírají z jiného menu, ale nemají cestu zpět (žádné vlastní `close`, žádný onESC, žádné `open` menu, ze kterého přišly); šedé nejsou nikdy dosaženy. Seznam pod tím srozumitelnými slovy říká, co dělat. Kliknutím na rámeček nebo řádek seznamu přejdete k danému menu. Nejdřív otevřete celou sadu menu, jinak menu z jiných souborů vypadají jako chybějící.",
    "Tab order":
        "Pořadí tabulátoru",
    "The numbers show the order in which Tab and Arrow down move the focus in the game.":
        "Čísla ukazují pořadí, v jakém Tab a šipka dolů posouvají fokus ve hře.",
    "**{{MeTabFolge}}** puts a number on every element that can take the keyboard focus, in the order the engine uses (Menu_SetNextCursorItem: the order of the itemDefs, without decorations, hidden elements and elements switched off by a cvar; from no focus the game starts at the second element of the file). To change it, click the elements one after another in the new order - after the last click the editor rearranges the itemDefs (one Undo step). The game draws in the same order, so a later element lies over an earlier one; elements that cannot take the focus stay where they are.":
        "**{{MeTabFolge}}** dá číslo každému prvku, který může dostat fokus klávesnice, v pořadí, které používá engine (Menu_SetNextCursorItem: pořadí itemDef bez dekorací, skrytých prvků a prvků vypnutých cvarem; bez fokusu začne hra druhým prvkem souboru). Chcete-li ho změnit, klikejte na prvky jeden po druhém v novém pořadí - po posledním kliknutí editor přeskupí itemDef (jeden krok zpět). Hra kreslí ve stejném pořadí, takže pozdější prvek leží přes dřívější; prvky, které fokus dostat nemohou, zůstanou na místě.",
    "Styles":
        "Styly",
    "All colours, fonts, text sizes and backgrounds of the file with their number of uses.":
        "Všechny barvy, písma, velikosti textu a pozadí souboru s počtem jejich použití.",
    "**{{MeStile}}** collects every colour (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), font, text size and background of the file - the most used first. Change a value and every place that uses it changes at once, as ordinary values in the .menu (spacing, comments and quotes stay; one Undo step). Values from #include files or macros are counted but stay as they are.":
        "**{{MeStile}}** shromáždí každou barvu (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), písmo, velikost textu a pozadí souboru - nejpoužívanější první. Změňte hodnotu a hned se změní všechna místa, která ji používají, jako běžné hodnoty v .menu (mezery, komentáře a uvozovky zůstanou; jeden krok zpět). Hodnoty ze souborů #include nebo maker se započítají, ale zůstanou, jak jsou.",
    "Templates and settings wizard":
        "Šablony a průvodce nastavením",
    "A settings page with three tabs from the template - the tabs appear as pages above the canvas.":
        "Stránka nastavení se třemi kartami ze šablony - karty se objeví jako stránky nad plátnem.",
    "The **+** next to the pages offers **{{MeVorlageLeer}}** or a whole menu: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** and **{{MeVorlageLaden}}**. They use only keywords every game version reads; the main menu and the loading screen get the names the engine looks for (single player `mainMenu` and `loadscreen`, multiplayer `main` and `Connect`), and the main menu opens the settings and credits templates - the menu flow shows the result without problems.":
        "**+** vedle stránek nabízí **{{MeVorlageLeer}}** nebo celé menu: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** a **{{MeVorlageLaden}}**. Používají jen klíčová slova, která čte každá verze hry; hlavní menu a obrazovka načítání dostanou jména, která engine hledá (pro jednoho hráče `mainMenu` a `loadscreen`, pro více hráčů `main` a `Connect`), a hlavní menu otevírá šablony nastavení a titulků - tok menu ukáže výsledek bez problémů.",
    "The settings wizard knows the controls of the game's own menus.":
        "Průvodce nastavením zná ovládací prvky vlastních menu hry.",
    "Reference image, original and limits":
        "Referenční obrázek, originál a limity",
    "A reference image under the canvas.":
        "Referenční obrázek pod plátnem.",
    "**{{MeRefBild}}** places a PNG, JPG or TGA semi-transparently on the canvas, under the menu or - when a full-screen background covers it - over it (**{{MeRefOben}}**), with adjustable **{{MeRefDeckkraft}}**. It is only a drawing aid and never goes into the menu or the pk3.":
        "**{{MeRefBild}}** položí na plátno poloprůhledný PNG, JPG nebo TGA, pod menu, nebo - když ho zakrývá pozadí na celou obrazovku - nad něj (**{{MeRefOben}}**), s nastavitelným **{{MeRefDeckkraft}}**. Je to jen pomůcka pro kreslení a nikdy se nedostane do menu ani do pk3.",
    "An element of the game's main menu was moved - the Properties name the difference and offer the reset.":
        "Prvek hlavního menu hry byl posunut - Vlastnosti uvádějí rozdíl a nabízejí obnovení.",
    "For a menu from the game, the Properties show in **{{MeOriginal}}** whether the selected element differs from the game's original file and in which keywords; **{{MeOriginalZuruecksetzen}}** puts the original itemDef back (one Undo step). Below the pages, a line shows how full the menu is against the limits of the game version - elements per menu, menus and the longest script - orange from 80 percent, red at the limit, because beyond it the game silently drops the rest.":
        "U menu ze hry ukazují Vlastnosti v **{{MeOriginal}}**, zda se vybraný prvek liší od původního souboru hry a ve kterých klíčových slovech; **{{MeOriginalZuruecksetzen}}** vrátí původní itemDef (jeden krok zpět). Pod stránkami ukazuje řádek, jak je menu zaplněné vůči limitům verze hry - prvky na menu, menu a nejdelší skript - oranžově od 80 procent, červeně na limitu, protože za ním hra zbytek tiše zahodí.",
    "Next to the limits, **Readability** counts hints with guide values of the editor (not limits of the game): text lower than 8 points on the 640 x 480 screen, contrast below 3:1 on filled areas, and texts that fit their field in one language of the game but not in another (measured with the game's fonts and the texts of every language in the game data). Hover for the list with solutions; click to select the next element with a hint.":
        "Vedle limitů počítá **Čitelnost** upozornění podle orientačních hodnot editoru (ne limitů hry): text nižší než 8 bodů na obrazovce 640 x 480, kontrast pod 3:1 na vyplněných plochách a texty, které se do svého pole vejdou v jednom jazyce hry, ale ne v jiném (měřeno písmy hry a texty všech jazyků v herních datech). Po najetí myší se zobrazí seznam s řešeními; kliknutím vyberete další prvek s upozorněním.",
    "Closing a tab with unsaved changes now asks first: save, close without saving, or cancel.":
        "Zavření karty s neuloženými změnami se teď nejdřív zeptá: uložit, zavřít bez uložení, nebo zrušit.",
    "**Upper body and legs.** When an NPC's legs and upper body play different animations (`SET_ANIM_LOWER`/`SET_ANIM_UPPER`, a shot while walking), the preview turns the spine like the game: the Motion bone of the leg animation is taken off the view and spread over the three spine bones (pitch, yaw and roll, `cg_motionBoneComp 2`); head and spine turn about the axes of their own bones.":
        "**Trup a nohy.** Když nohy a trup NPC přehrávají různé animace (`SET_ANIM_LOWER`/`SET_ANIM_UPPER`, výstřel za chůze), náhled otáčí páteří jako hra: kost Motion animace nohou se odečte od pohledu a rozdělí na tři kosti zad (sklon, otočení a náklon, `cg_motionBoneComp 2`); hlava a trup se otáčejí kolem os svých vlastních kostí.",
    "**Lightsaber stance.** An NPC standing with a lit saber takes the stance the game gives it (PM_SetSaberMove LS_READY): the `readyAnim` of its .sab, otherwise by saber type and style - arc saber, two sabers, staff, lance or trident; while walking or running the upper body follows the legs, and standing still without input the stance covers the whole body.":
        "**Postoj se světelným mečem.** NPC, který stojí se zapnutou čepelí, zaujme postoj, který mu dá hra (PM_SetSaberMove LS_READY): `readyAnim` ze svého .sab, jinak podle typu a stylu meče – oblouková čepel, dva meče, hůl, kopí nebo trojzubec; při chůzi a běhu trup následuje nohy a v klidu bez vstupu platí postoj pro celé tělo.",
    "**Standing pose.** Without a script animation an NPC stands the way the game picks it: with a gun in `BOTH_STAND3`, an Imperial officer in `BOTH_STAND4`, otherwise in `BOTH_STAND1`.":
        "**Postoj.** Bez animace ze skriptu stojí NPC tak, jak to zvolí hra: se střelnou zbraní v `BOTH_STAND3`, imperiální důstojník v `BOTH_STAND4`, jinak v `BOTH_STAND1`.",
    "Standing still on a slope or a step, the figure puts its feet like the game (the slope leg animations, one step every 100 ms); with a lightsaber its upper body takes the ready stance or follows the legs, as the game decides it (PM_TorsoAnimLightsaber).":
        "Stojí-li postava na svahu nebo na schodu, staví nohy jako ve hře (animace nohou pro svah, jeden stupeň za 100 ms); se světelným mečem zaujme horní část těla bojový postoj nebo následuje nohy, jak rozhodne hra (PM_TorsoAnimLightsaber).",
    # Abschluss rc607 (Uebersetzung 3): dock4, M7, reste2, Zeitleiste 4, vis4
    "Select a figure (an NPC) in the view or in its row and press **{{AufKnopf}}** in the transport bar: from the time of the playhead you steer it in the preview - <kbd>W</kbd>/<kbd>S</kbd>/<kbd>A</kbd>/<kbd>D</kbd> move, <kbd>Shift</kbd> walks instead of running, <kbd>Q</kbd>/<kbd>E</kbd> or the right mouse button turn. It moves with the game's movement code (Pmove), at the speeds from its .npc file and against the walls of the map. The camera follows the figure. Its model moves as in the game: the legs run, walk or step backwards as the game picks them, turn up to 60° toward the direction it moves while the upper body keeps looking ahead, blend from one animation to the next, and a weapon stance or a held animation of the upper body stays; the path is drawn, and the numbered circles are the walk targets that will be written. **{{AufFertig}}** (<kbd>Enter</kbd>) writes them, **{{AufAbbrechen}}** (<kbd>Esc</kbd>) changes nothing.":
        "Vyberte postavu (NPC) v pohledu nebo v jejím řádku a stiskněte **{{AufKnopf}}** v liště přehrávání: od času ukazatele přehrávání ji řídíte v náhledu - <kbd>W</kbd>/<kbd>S</kbd>/<kbd>A</kbd>/<kbd>D</kbd> pohyb, <kbd>Shift</kbd> chůze místo běhu, <kbd>Q</kbd>/<kbd>E</kbd> nebo pravé tlačítko myši otáčí. Pohybuje se pohybovým kódem hry (Pmove), rychlostmi ze svého souboru .npc a naráží na stěny mapy. Kamera postavu sleduje. Její model se pohybuje jako ve hře: nohy běží, jdou nebo couvají, jak je hra zvolí, natáčejí se až o 60° ke směru pohybu, zatímco horní část těla dál hledí vpřed, animace plynule přecházejí jedna v druhou a postoj se zbraní nebo držená animace horní části těla zůstává; dráha se kreslí a očíslované kruhy jsou cíle chůze, které se zapíšou. **{{AufFertig}}** (<kbd>Enter</kbd>) je zapíše, **{{AufAbbrechen}}** (<kbd>Esc</kbd>) nic nezmění.",
    "With `-randomsamples` (**[Final] Extreme Quality**) there is a third choice: the graphics card also computes the lightmaps with random subsamples (it gathers them over several blocks and computes them together, while the processor takes the biggest ones). It is faster only on some maps (with many small lightmaps, such as kejim_post or taspir1) and slower on others (duel_mos_espa). So **{{KeGpuAuto}}** measures it once only after the graphics card has been faster than the processor on this map, and takes it from then on only where it was the fastest of the three. The result is the same, byte for byte. The console then says \"GPU bake (Auto): {{KeGpuWahlGpuZ}} - ...\".":
        "S `-randomsamples` (**[Final] Extreme Quality**) existuje třetí možnost: grafická karta počítá lightmapy i s náhodnými podvzorky (sbírá je z několika bloků a počítá je společně, zatímco procesor si bere ty největší). Rychlejší je jen na některých mapách (s mnoha malými lightmapami, jako kejim_post nebo taspir1) a na jiných pomalejší (duel_mos_espa). Proto ji **{{KeGpuAuto}}** změří jednou, a to teprve když byla grafická karta na této mapě rychlejší než procesor, a od té doby ji bere jen tam, kde byla ze tří nejrychlejší. Výsledek je stejný, bajt po bajtu. Konzole pak napíše „Zapékání na GPU (Auto): {{KeGpuWahlGpuZ}} - ...“.",
    "Put any panel anywhere - beside, below or as a tab of another one in the main window, or in its own window on a second screen - and save the arrangement under a name, per area.":
        "Umístěte jakýkoli panel kamkoli - vedle, pod nebo jako kartu jiného panelu v hlavním okně, nebo do vlastního okna na druhé obrazovce - a uložte rozvržení pod názvem, pro každou oblast zvlášť.",
    "Every area - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** and **{{ViewMenuEditor}}** - is made of panels that are docked in the main window. As long as you move nothing, every panel stays in its place: the standard layout, to the point exactly as before, with the same dividers.":
        "Každá oblast - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** a **{{ViewMenuEditor}}** - se skládá z panelů ukotvených v hlavním okně. Dokud nic nepřesunete, zůstane každý panel na svém místě: standardní rozvržení, na bod přesně jako dřív, se stejnými předěly.",
    "Open the menu **{{DockMenu}}** (in the map editor it is the last menu, after NetRadiant's) and click the panel under **{{DockOwnWindow}}**, for example the timeline. It becomes a window of its own; its place in the main window goes to its neighbours - the mission view gets taller.":
        "Otevřete menu **{{DockMenu}}** (v editoru map je to poslední menu, za menu NetRadiantu) a klikněte na panel pod **{{DockOwnWindow}}**, například na časovou osu. Stane se samostatným oknem; jeho místo v hlavním okně připadne sousedům - pohled mise se zvětší na výšku.",
    "Or point at the top left corner of a panel: a small triangle appears. Drag it away and the panel comes out with it; a click on it shows the panel's tab instead.":
        "Nebo ukažte na levý horní roh panelu: objeví se malý trojúhelník. Odtáhněte ho a panel vyjede s ním; kliknutí na něj místo toho ukáže kartu panelu.",
    "Drag the window by its title bar out of the main window: it becomes a real Windows window and can go onto a second screen. It keeps drawing there - the 3D views and the real-time light of the map editor included. Files can be dragged onto such a window as onto the main window (for example a model onto the XY view of the map editor, or a .menu file onto the canvas of the menu editor).":
        "Přetáhněte okno za jeho záhlaví ven z hlavního okna: stane se z něj skutečné okno Windows a může jít na druhou obrazovku. Kreslí tam dál - včetně 3D pohledů a světla v reálném čase v editoru map. Soubory lze na takové okno přetáhnout stejně jako na hlavní okno (například model na pohled XY editoru map nebo soubor .menu na plátno editoru menu).",
    "The triangle in the top left corner of a panel appears under the mouse: drag it to take the panel out.":
        "Trojúhelník v levém horním rohu panelu se objeví pod myší: táhněte ho, abyste panel vyjmuli.",
    "Combine panels and dock them anywhere":
        "Spojujte panely a ukotvěte je kamkoli",
    "Drag the window of a panel over another panel - in its own window or in the main window: markers appear. Drop it on the middle marker and both become tabs; drop it on an arrow and it sits beside, above or below. So the Script Flow can go under the map view, the entity layers can become a tab of the map view, the camera of the map editor can go below the XY view - like in Blender. The dividers between docked panels can then be dragged, a tab can be dragged out again. This works for the panels of one area; dialogs such as Find stay on their own.":
        "Přetáhněte okno panelu nad jiný panel - ve vlastním okně nebo v hlavním okně: objeví se značky. Pusťte ho na prostřední značku a oba se stanou kartami; pusťte ho na šipku a bude vedle, nad nebo pod ním. Tak může tok skriptu jít pod pohled mapy, vrstvy entit se mohou stát kartou pohledu mapy, kamera editoru map může jít pod pohled XY - jako v Blenderu. Předěly mezi ukotvenými panely pak lze táhnout a kartu lze zase vytáhnout ven. Funguje to pro panely jedné oblasti; dialogy jako Hledat zůstávají samostatné.",
    "A panel window dragged over the main window: the markers show where it will dock - here as a tab of the mission view.":
        "Okno panelu přetažené nad hlavní okno: značky ukazují, kde se ukotví - zde jako karta pohledu mise.",
    "Or drop it on a marker in the main window, wherever you want it.":
        "Nebo ho pusťte na značku v hlavním okně, kamkoli chcete.",
    "**{{DockReset}}** brings all panels of the area back to the standard layout.":
        "**{{DockReset}}** vrátí všechny panely oblasti do standardního rozvržení.",
    "Each area has its own layouts. **{{DockSaveAs}}** stores where every panel is - docked in the main window, in its own window, hidden -, where these windows are, on which screen, and how they are combined. **{{DockLoad}}** switches to a saved layout, **{{DockDelete}}** removes one, **{{DockReset}}** brings every panel of the area back to the standard layout. The current layout of every area is kept in the settings and is there again at the next start.":
        "Každá oblast má vlastní rozvržení. **{{DockSaveAs}}** uloží, kde je každý panel - ukotvený v hlavním okně, ve vlastním okně, skrytý -, kde jsou tato okna, na které obrazovce a jak jsou spojena. **{{DockLoad}}** přepne na uložené rozvržení, **{{DockDelete}}** jedno odstraní, **{{DockReset}}** vrátí každý panel oblasti do standardního rozvržení. Aktuální rozvržení každé oblasti se ukládá v nastavení a při dalším spuštění je tu znovu.",
    "NetRadiant's layouts stay the default (Preferences > Layout): **Regular**, **Regular Left** and **Split** (also with freely assigned places and with the built-in entity window) each have their standard layout with exactly NetRadiant's sizes and NetRadiant's dividers - dragging, collapsing and **Maximize view** work as in NetRadiant. A view taken out gives its place to its neighbours like a hidden view in NetRadiant; in its own window it draws everything it shows in the main window, a click into it makes it the active view, keyboard shortcuts and free look then work in that window - on every screen. Once a view is docked somewhere else (for example as a tab of the camera), ImGui's dividers apply; **{{DockReset}}** brings NetRadiant's layout back. In **Floating** the views float anyway.":
        "Rozvržení NetRadiantu zůstávají výchozí (Preferences > Layout): **Regular**, **Regular Left** a **Split** (i s volně přiřazenými místy a s vestavěným oknem entit) mají každé své standardní rozvržení s přesně stejnými velikostmi a předěly jako NetRadiant - tažení, sbalení a **Maximize view** fungují jako v NetRadiantu. Vyjmutý pohled přenechá své místo sousedům jako skrytý pohled v NetRadiantu; ve vlastním okně kreslí vše, co ukazuje v hlavním okně, kliknutím do něj se stane aktivním pohledem a klávesové zkratky i volné rozhlížení pak fungují v tomto okně - na každé obrazovce. Jakmile je pohled ukotven jinde (například jako karta kamery), platí předěly ImGui; **{{DockReset}}** vrátí rozvržení NetRadiantu. Ve **Floating** pohledy plují tak jako tak.",
    "Both layouts of the menu editor are made of panels: with the tabs on the right **{{MePalette}}**, **{{DockPanelCanvas}}** and the column **{{DockPanelMenuSide}}**; with the text below (preferences) the palette with the hierarchy, the canvas, **{{MeReiterEigenschaften}}** and **{{MeReiterText}}** - for example the canvas large on a second screen while the text stays in the main window.":
        "Obě rozvržení editoru menu se skládají z panelů: s kartami vpravo **{{MePalette}}**, **{{DockPanelCanvas}}** a sloupec **{{DockPanelMenuSide}}**; s textem dole (předvolby) paleta s hierarchií, plátno, **{{MeReiterEigenschaften}}** a **{{MeReiterText}}** - například plátno zvětšené na druhé obrazovce, zatímco text zůstane v hlavním okně.",
    "Troubleshooting":
        "Řešení potíží",
    "A panel is lost or the arrangement is muddled: **{{DockMenu}} > {{DockReset}}** brings the standard layout of the area back.":
        "Panel se ztratil nebo je uspořádání zmatené: **{{DockMenu}} > {{DockReset}}** vrátí standardní rozvržení oblasti.",
    "Docking inside the main window causes a problem on your computer: start JK Studio with the environment variable `BHED_DOCKSPACE=0` (for example in a command prompt `set BHED_DOCKSPACE=0`, then `jkstudio.exe` from the same prompt). Then every panel stays at its fixed place in the main window again and can only be taken out into its own window, as in earlier versions; dropping a panel window on its old place docks it back. Your layouts are kept.":
        "Ukotvování uvnitř hlavního okna působí na vašem počítači potíže: spusťte JK Studio s proměnnou prostředí `BHED_DOCKSPACE=0` (například v příkazovém řádku `set BHED_DOCKSPACE=0` a pak `jkstudio.exe` ze stejného příkazového řádku). Pak každý panel znovu zůstane na svém pevném místě v hlavním okně a lze ho jen vyjmout do vlastního okna, jako v dřívějších verzích; puštěním okna panelu na jeho staré místo se znovu ukotví. Vaše rozvržení zůstanou zachována.",
    "**{{MeAssistent}}** lists every game setting (cvar) that the original menus of the game offer, with the control they use there: a slider with its range, yes/no, or a choice with its entries. Tick the settings you want and click the button below the search field - the editor adds a settings page with these controls, a title and a back button. The list has three columns - setting, control and text; drag a column border to widen it, a cut-off text shows in full when you point at it.":
        "**{{MeAssistent}}** vypíše každé nastavení hry (cvar), které nabízejí původní menu hry, s ovládacím prvkem, který tam používají: posuvník s rozsahem, ano/ne nebo výběr s položkami. Zaškrtněte požadovaná nastavení a klikněte na tlačítko pod vyhledávacím polem - editor přidá stránku nastavení s těmito prvky, nadpisem a tlačítkem zpět. Seznam má tři sloupce - nastavení, ovládací prvek a text; přetažením okraje sloupce ho rozšíříte, oříznutý text se celý ukáže, když na něj ukážete.",
    "Menu editor: game values, reload in the game, components":
        "Editor menu: hodnoty hry, opětovné načtení ve hře, komponenty",
    "Every element that shows values of the running game is now drawn like the game, with sample values you can change; key bindings show real keys; the loading and connect screens look like in the game; a game started with Test in game reloads your menus after saving; components keep copies in sync; models get handles for turning, distance and field of view.":
        "Každý prvek, který ukazuje hodnoty z běžící hry, se teď kreslí jako ve hře, s ukázkovými hodnotami, které můžete měnit; přiřazení kláves ukazují skutečné klávesy; obrazovky načítání a připojení vypadají jako ve hře; hra spuštěná přes Vyzkoušet ve hře po uložení znovu načte vaše menu; komponenty udržují kopie ve shodě; modely dostanou úchyty pro otáčení, vzdálenost a zorné pole.",
    "Elements with values from the game":
        "Prvky s hodnotami ze hry",
    "Some elements (`ownerdraw`) are drawn by the game's code itself: the Force stars and Force points of the multiplayer player menus, the server source and filter, the bot names, the graphics driver page (GL_VENDOR and extensions - the editor reads them from your real OpenGL driver), the datapad with objectives, weapons and Force powers, the Pazaak board of Movie Duels and more. The editor now draws each of them exactly like the engine. Where the game takes the values from the running game or the server, it uses sample values and marks the element with **{{MeOdBeispiel}}** at the top left (only while editing). Change the samples under **Properties > {{MeOdBeispiel}}**: the selected weapon and Force power with its level, the objectives, health, Force and ammo, and for multiplayer the server settings (g_jediVmerc, g_forceBasedTeams, highest Force rank, server refresh). Elements that the game itself draws empty (for example the voice chat entries) stay empty; lists that are filled only during a match (player list) show sample names.":
        "Některé prvky (`ownerdraw`) kreslí sám kód hry: hvězdy a body Síly v menu hráče pro více hráčů, zdroj a filtr serverů, jména botů, stránku grafického ovladače (GL_VENDOR a rozšíření - editor je čte z vašeho skutečného ovladače OpenGL), datapad s úkoly, zbraněmi a schopnostmi Síly, hrací desku Pazaak z Movie Duels a další. Editor teď každý z nich kreslí přesně jako engine. Kde hra bere hodnoty z běžící hry nebo ze serveru, použije ukázkové hodnoty a označí prvek vlevo nahoře nápisem **{{MeOdBeispiel}}** (jen při úpravách). Ukázky změníte ve **Vlastnosti > {{MeOdBeispiel}}**: zvolenou zbraň a schopnost Síly s její úrovní, úkoly, zdraví, Sílu a munici a pro hru více hráčů nastavení serveru (g_jediVmerc, g_forceBasedTeams, nejvyšší hodnost Síly, obnovení serverů). Prvky, které hra sama kreslí prázdné (například položky hlasového chatu), zůstanou prázdné; seznamy, které se plní jen během zápasu (seznam hráčů), ukazují ukázková jména.",
    "The datapad weapons page of Jedi Academy with sample weapons - HUD ticks, frame, icons and description as in the game.":
        "Stránka zbraní datapadu Jedi Academy s ukázkovými zbraněmi - značky HUD, rámeček, ikony a popis jako ve hře.",
    "Jedi Outcast and Movie Duels draw their own datapad: health, armour, Force and ammo rings of the Jedi Outcast HUD, an icon strip with prongs behind the weapons, inventory and Force powers, and in Movie Duels smaller icons with one neighbour on each side, its 25 Force powers and its own texts. The preview follows the game whose data is loaded.":
        "Jedi Outcast a Movie Duels kreslí vlastní datapad: kruhy zdraví, brnění, Síly a munice z HUD Jedi Outcast, pás ikon s hroty za zbraněmi, inventářem a schopnostmi Síly, a v Movie Duels menší ikony s jedním sousedem na každé straně, jeho 25 schopností Síly a vlastní texty. Náhled se řídí hrou, jejíž data jsou načtena.",
    "**Key bindings**: the key binding elements of the controls menus now show the keys the game binds by default (from the game's default config, for example `default.cfg` or `mpdefault.cfg`), with the game's key names - instead of `???`. Commands the game does not list stay `???`, as in the game.":
        "**Přiřazení kláves**: prvky přiřazení kláves v menu ovládání teď ukazují klávesy, které hra přiřazuje ve výchozím stavu (z výchozí konfigurace hry, například `default.cfg` nebo `mpdefault.cfg`), s názvy kláves hry - místo `???`. Příkazy, které hra neuvádí, zůstanou `???`, jako ve hře.",
    "Loading and connect screens":
        "Obrazovky načítání a připojení",
    "The single player loading screen with level shot, weapons and Force powers from the sample, loading bar and map name.":
        "Obrazovka načítání pro jednoho hráče s obrázkem úrovně, zbraněmi a schopnostmi Síly z ukázky, ukazatelem načítání a názvem mapy.",
    "The `loadscreen` menu of single player and the `Connect` menu of multiplayer are only part of what the game shows: while loading, the game draws the level shot, the weapon and Force icons of the saved game, the loading bar and the map name over it; while connecting, it writes map, server and connection state. The editor draws all of this like the game. Under **Properties > {{MeOdBeispiel}}** you choose the map, the map message, the loading progress and the special first map (black screen with a quote), or for multiplayer the server, the map, the connection state, the packets and the message. The switch {{MeHudWie}} shows the plain menu instead.":
        "Menu `loadscreen` pro jednoho hráče a menu `Connect` pro více hráčů jsou jen částí toho, co hra ukazuje: při načítání přes ně hra kreslí obrázek úrovně, ikony zbraní a Síly z uložené hry, ukazatel načítání a název mapy; při připojování vypisuje mapu, server a stav připojení. Editor to vše kreslí jako hra. Ve **Vlastnosti > {{MeOdBeispiel}}** zvolíte mapu, text mapy, průběh načítání a zvláštní první mapu (černá obrazovka s citátem), nebo pro hru více hráčů server, mapu, stav připojení, pakety a zprávu. Přepínač {{MeHudWie}} místo toho ukáže samotné menu.",
    "Reload in the running game":
        "Opětovné načtení v běžící hře",
    "**{{MeImSpielTesten}}** now puts your menus as loose files into its own test folder (never into the game folder) when it can reload them in the running game: with OpenJK and Movie Duels. The original programs of Jedi Academy and Jedi Outcast know `ui_load` too, but they ignore keys sent to a window in the background - for them the test keeps using the .pk3 and a new start. While that game runs, every save writes the files again and gives `ui_load` to the game: it closes its menus, reads the menu set again and opens the last menu - without restarting. The editor only sends keys to the window of the game it started itself.":
        "**{{MeImSpielTesten}}** teď ukládá vaše menu jako volné soubory do vlastní testovací složky (nikdy do složky hry), pokud je může znovu načíst v běžící hře: s OpenJK a Movie Duels. Původní programy Jedi Academy a Jedi Outcast `ui_load` také znají, ale ignorují klávesy poslané oknu na pozadí - pro ně test dál používá .pk3 a nové spuštění. Dokud tato hra běží, každé uložení soubory znovu zapíše a pošle hře `ui_load`: zavře svá menu, znovu načte sadu menu a otevře poslední menu - bez restartu. Editor posílá klávesy jen do okna hry, kterou sám spustil.",
    "Components":
        "Komponenty",
    "A button as component with one linked copy - the Properties show the original and its copies.":
        "Tlačítko jako komponenta s jednou propojenou kopií - Vlastnosti ukazují originál a jeho kopie.",
    "Select an element and click **{{MeKompAnlegen}}** in the Properties: it becomes the original of a component. **{{MeKompKopie}}** inserts a linked copy next to it. Move the copy wherever you like - it keeps its own name and position; everything else follows the original: change the text, colour or size of the original and every copy changes with it (one Undo step). In the .menu file original and copies stay ordinary itemDefs with a comment line before them (`// jkstudio:komponente <name>`, `// jkstudio:instanz <name>`); the game reads them like any other element.":
        "Vyberte prvek a ve Vlastnostech klikněte na **{{MeKompAnlegen}}**: stane se originálem komponenty. **{{MeKompKopie}}** vedle něj vloží propojenou kopii. Kopii přesuňte, kam chcete - ponechá si vlastní název a polohu; vše ostatní následuje originál: změňte text, barvu nebo velikost originálu a každá kopie se změní s ním (jeden krok Zpět). V souboru .menu zůstanou originál i kopie běžnými itemDef s řádkem komentáře před nimi (`// jkstudio:komponente <name>`, `// jkstudio:instanz <name>`); hra je čte jako jakýkoli jiný prvek.",
    "Aligning models":
        "Zarovnání modelů",
    "The handles of a selected model: turn (top), distance (right), height (left), field of view (bottom).":
        "Úchyty vybraného modelu: otáčení (nahoře), vzdálenost (vpravo), výška (vlevo), zorné pole (dole).",
    "A selected model element gets four handles. Drag the top one sideways to turn the model (`model_angle`), the right one up or down to move it farther away or closer, the left one to raise or lower it, and the bottom one sideways to change the field of view (`model_fovx`, `model_fovy`). The game places a menu model from the height of `model_g2mins`/`model_g2maxs` (not from `model_origin`), so distance and height change these two values. You see the result while dragging; releasing writes it into the menu as one Undo step.":
        "Vybraný prvek modelu dostane čtyři úchyty. Horní táhněte do strany a model se otočí (`model_angle`), pravý nahoru nebo dolů a model se vzdálí nebo přiblíží, levý ho zvedne nebo sníží a dolní do strany změní zorné pole (`model_fovx`, `model_fovy`). Hra umisťuje model menu podle výšky `model_g2mins`/`model_g2maxs` (ne podle `model_origin`), proto vzdálenost a výška mění tyto dvě hodnoty. Výsledek vidíte během tažení; puštěním se zapíše do menu jako jeden krok Zpět.",
    "**New projects** can start in multiplayer format: choose **{{MePrjSp}}** or **{{MePrjMp}}** before **{{MePrjNeu}}**.":
        "**Nové projekty** mohou začínat ve formátu pro více hráčů: před **{{MePrjNeu}}** zvolte **{{MePrjSp}}** nebo **{{MePrjMp}}**.",
    "Checked against the game":
        "Ověřeno proti hře",
    "The whole-menu templates were photographed in Movie Duels, in the original Jedi Academy and in Jedi Outcast and compared with the preview. Sliders have the size of each game: 128 wide in Jedi Academy and Jedi Outcast single player, 96 in Movie Duels and in multiplayer. Jedi Outcast treats every character after `^` as a colour (`^B` is green) and counts colour codes in the text width - the preview does the same, so centred and right-aligned texts sit where the game puts them.":
        "Šablony celých menu byly vyfotografovány v Movie Duels, v původním Jedi Academy a v Jedi Outcast a porovnány s náhledem. Posuvníky mají velikost podle hry: 128 na šířku v Jedi Academy a Jedi Outcast pro jednoho hráče, 96 v Movie Duels a ve hře více hráčů. Jedi Outcast bere každý znak po `^` jako barvu (`^B` je zelená) a počítá barevné kódy do šířky textu - náhled dělá totéž, takže text zarovnaný na střed a vpravo stojí tam, kam ho dá hra.",
    # Abschluss rc607: Bilder der Tutorial-Schritte
    "Step 1 in the program: the tutorial highlights where to go next.":
        "Krok 1 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 2 in the program: the tutorial highlights where to go next.":
        "Krok 2 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 3 in the program: the tutorial highlights where to go next.":
        "Krok 3 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 4 in the program: the tutorial highlights where to go next.":
        "Krok 4 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 5 in the program: the tutorial highlights where to go next.":
        "Krok 5 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 6 in the program: the tutorial highlights where to go next.":
        "Krok 6 v programu: tutoriál zvýrazní, kde pokračovat.",
    "Step 7 in the program: the tutorial highlights where to go next.":
        "Krok 7 v programu: tutoriál zvýrazní, kde pokračovat.",
    # Abschluss rc607: Untermenue neben der Spalte, deformVertexes move, Andocken ohne Platz
    "**Menus taller than the screen.** A menu with many entries (for example **{{KeLBuild}}** with a long build list) can be taller than the screen. In the style of JK Studio it then scrolls with the mouse wheel; with {{KeMenueNrc}} it wraps into columns like in NetRadiant-Custom, so every entry stays on the screen; a submenu opens beside its column, as in Qt.":
        "**Menu vyšší než obrazovka.** Menu s mnoha položkami (například **{{KeLBuild}}** s dlouhým seznamem sestavení) může být vyšší než obrazovka. Ve stylu JK Studia se pak posouvá kolečkem myši; s {{KeMenueNrc}} se zalomí do sloupců jako v NetRadiant-Custom, takže každá položka zůstane na obrazovce; podmenu se otevře vedle svého sloupce, jako v Qt.",
    "**Decals:** impact and scorch marks of the effects land on the faces the game would mark: like q3map2, the editor builds the world of the map with its BSP tree - only the visible part of each brush face, the models of `misc_model` baked into the world, and only faces next to an open leaf of the tree (no mark on the floor hidden under a step). On a large map this takes a few seconds like a build; it runs in the background, and until it is done the last world counts. Faces whose shader moves its vertices (`deformVertexes move`) count over the whole box of the movement, as q3map2 sorts them into the tree.":
        "**Obtisky (decals):** stopy po zásazích a spáleniny z efektů dopadají na stěny, které by označila hra: jako q3map2 sestaví editor svět mapy s jejím stromem BSP - jen viditelnou část každé stěny brushe, modely `misc_model` zapečené do světa a jen stěny vedle otevřeného listu stromu (žádná stopa na podlaze skryté pod schodem). Na velké mapě to trvá několik sekund jako sestavení; běží to na pozadí, a dokud to není hotové, platí poslední svět. Stěny, jejichž shader posouvá vrcholy (`deformVertexes move`), se počítají v celém rozsahu pohybu, jak je q3map2 řadí do stromu.",
    "Close its window with its **x** - the panel goes back to its place, nothing is lost. If that place is gone because you rearranged the area, it docks beside its neighbour and your arrangement stays.":
        "Zavřete jeho okno jeho **x** - panel se vrátí na své místo, nic se neztratí. Pokud to místo už neexistuje, protože jste oblast přeuspořádali, ukotví se vedle svého souseda a vaše uspořádání zůstane.",
}
