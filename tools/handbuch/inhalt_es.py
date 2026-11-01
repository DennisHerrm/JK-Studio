# -*- coding: utf-8 -*-
"""Das Handbuch auf Spanisch (es): englischer Text -> spanischer Text.

Ueberlagerung fuer tools/handbuch/inhalt_basis.py (tools/sprachen.py): jeder
T(en, de, zh, ja) und jeder Text aus uebersetzung/ findet hier seinen
spanischen Text ueber den englischen. Aendert sich ein englischer Text, faellt
die Seite fuer ihn auf Englisch zurueck, und gen_handbuch.py nennt ihn
("es: N Texte noch englisch"); --pflicht=es macht das zum Fehler.

Begriffe wie im Programm (tools/i18n_sprache_es.py, Glossar in
docs/intern/NOTIZEN_sprache_es.md). {{Kennung}}, [[Seite]], `Code` und
<kbd>-Tasten bleiben wie im Englischen; Tasten heissen wie auf spanischen
Tastaturen (TASTEN).
"""

TASTEN = {"Shift": "Mayús", "Delete": "Supr", "Home": "Inicio", "End": "Fin", "Space": "Espacio",
          "Backspace": "Retroceso", "Up": "Arriba", "Down": "Abajo", "Enter": "Intro"}

TEXTE = {
    "Each version is divided by area (map editor, menu editor, timeline and so on), and each group can be folded. Points without an area stand under **{{ChangelogAllgemein}}**.":
        "Cada versión está dividida por áreas (editor de mapas, editor de menús, línea de tiempo, etc.) y cada grupo se puede plegar. Los puntos sin área están en **{{ChangelogAllgemein}}**.",
    "The `base` next to a mod folder is always read as well, as in the game: if you only add `MD`, its menus still find their shaders and images in `base`.":
        "El `base` junto a una carpeta de mod siempre se lee también, como en el juego: si solo añades `MD`, sus menús encuentran igualmente sus shaders e imágenes en `base`.",
    "Every action is a block: the first row holds the command and its target, with the up and delete buttons always in the same place, the values lie below. A `transition` shows its from and to rectangles as labelled groups, with time and step; `setitemcolor` has its colour swatch next to the numbers. Everything stays inside the panel at any width.":
        "Cada acción es un bloque: la primera fila tiene el comando y su destino, con los botones de subir y borrar siempre en el mismo sitio; los valores van debajo. Una `transition` muestra sus rectángulos de origen y destino como grupos con etiqueta, con tiempo y paso; `setitemcolor` tiene la muestra de color junto a los números. Todo queda dentro del panel a cualquier ancho.",
    "**{{MeReiterHierarchie}}** shows the elements of the open tab only and follows the selection on the canvas. **{{MeGanzeDatei}}** shows every `menuDef` of the file, for example to move an element to another page. On a state tab (`ui_sel=0_0`) only what is visible there is listed. The panel keeps a minimum width at which all numbers stay readable; long rows wrap.":
        "**{{MeReiterHierarchie}}** muestra solo los elementos de la pestaña abierta y sigue la selección del lienzo. **{{MeGanzeDatei}}** muestra todos los `menuDef` del archivo, por ejemplo para mover un elemento a otra página. En una pestaña de estado (`ui_sel=0_0`) solo aparece lo visible allí. El panel mantiene un ancho mínimo en el que todos los números son legibles; las filas largas se ajustan.",
    "wrap long lines on or off, as in VS Code (on by default, the file does not change); without wrapping Shift+mouse wheel or the scrollbar at the bottom scrolls sideways":
        "activa o desactiva el ajuste de líneas largas, como en VS Code (activado de forma predeterminada, el archivo no cambia); sin ajuste, Mayús+rueda o la barra inferior desplazan en horizontal",
    "**Zoom:** Ctrl+mouse wheel zooms around the pointer (25 to 800 %), Ctrl+0 fits, Ctrl+1 is 100 %, the middle button or Space+drag moves the view. The zoom is only a view: it is not an undo step and not in the file. Rulers, handles, snapping and dragging stay right at every step.":
        "**Zoom:** Ctrl+rueda del ratón acerca alrededor del puntero (25 a 800 %), Ctrl+0 ajusta, Ctrl+1 es 100 %, el botón central o Espacio+arrastrar mueve la vista. El zoom es solo una vista: no es un paso de deshacer ni está en el archivo. Reglas, tiradores, ajuste y arrastre son correctos en cada nivel.",
    "**Mouse pointer:** It shows what a press does: resize arrows on the handles and on the edges of the selected element (the edges resize too), the move pointer on the element itself, the text cursor over text.":
        "**Puntero del ratón:** muestra qué hará una pulsación: flechas de tamaño en los tiradores y bordes del elemento elegido (los bordes también escalan), el puntero de mover sobre el elemento, el cursor de texto sobre el texto.",
    "**Write text in place:** Double-click an element on the canvas and type. The text appears in the game's font, size, colour and wrapping, with cursor, selection, copy and paste. Esc discards, Enter or a click elsewhere keeps it as one undo step; Ctrl+Enter starts a new line where the element wraps. A text from the game (`@MENUS_...`) stays as it is: the element gets its own text in your project's text package, and the other languages keep their translation. Where the text comes from a cvar, the game itself or an image, the double-click says so. **{{MeReiterHierarchie}}** shows the text in the row, **{{MeReiterEigenschaften}}** has it at the very top.":
        "**Escribir el texto en el sitio:** haz doble clic en un elemento del lienzo y escribe. El texto aparece con la fuente, el tamaño, el color y el ajuste del juego, con cursor, selección, copiar y pegar. Esc descarta, Intro o un clic fuera lo conserva como un solo paso de deshacer; Ctrl+Intro crea una línea nueva donde el elemento ajusta. Un texto del juego (`@MENUS_...`) queda como está: el elemento recibe su propio texto en el paquete de textos del proyecto y los demás idiomas conservan su traducción. Si el texto viene de un cvar, del propio juego o de una imagen, el doble clic lo indica. **{{MeReiterHierarchie}}** muestra el texto en la fila, **{{MeReiterEigenschaften}}** arriba del todo.",
    "**Properties** shows only what the game reads for this kind of element; a model has no text fields, for example, but its own group with model, skin, animation from a searchable list (the names of the model's `animation.cfg`), facing, height, distance and field of view. Lines in the file that do not apply stay there and appear under **{{MeWeitereEintraege}}**. With nothing selected, Properties has the switch between **{{MeVollbild}}** and **{{MeUeberlagerung}}**.":
        "**Properties** muestra solo lo que el juego lee para este tipo de elemento; un modelo, por ejemplo, no tiene campos de texto, sino su propio grupo con modelo, skin, animación de una lista con búsqueda (los nombres del `animation.cfg` del modelo), orientación, altura, distancia y campo de visión. Las líneas del archivo que no corresponden permanecen allí y aparecen en **{{MeWeitereEintraege}}**. Sin nada seleccionado, Properties tiene el interruptor entre **{{MeVollbild}}** y **{{MeUeberlagerung}}**.",
    "**An empty menu:** With full screen the game draws nothing underneath. In single player an empty menu is black (without `backcolor`, even a plain image in `background` stays invisible); in multiplayer the game shows its grey default box until a `background` or an element covers it, and the diagnosis says so. Without full screen you see through the menu; the canvas marks this with a checkerboard, which you can switch off and which never goes into the file.":
        "**Un menú vacío:** con pantalla completa el juego no dibuja nada debajo. En un jugador un menú vacío es negro (sin `backcolor`, incluso una imagen simple en `background` queda invisible); en multijugador el juego muestra su cuadro gris predeterminado hasta que un `background` o un elemento lo cubre, y el diagnóstico lo indica. Sin pantalla completa se ve a través del menú; el lienzo lo marca con un tablero, que se puede desactivar y nunca entra en el archivo.",
    "Nothing is selected yet. Click the brush in a 2D view first, then the texture.":
        "Aún no hay nada seleccionado. Pulsa primero el brush en una vista 2D y luego la textura.",
    "That made an entity. Press Ctrl+Z, then drag with the LEFT mouse button in the 2D view.":
        "Eso ha creado una entidad. Pulsa Ctrl+Z y arrastra con el botón IZQUIERDO del ratón en la vista 2D.",
    "That drew a second brush. Press Ctrl+Z and start the drag just above the top edge of the first one.":
        "Eso ha dibujado un segundo brush. Pulsa Ctrl+Z y empieza a arrastrar justo encima del borde superior del primero.",
    "That drew a brush. Press Ctrl+Z and click with the RIGHT mouse button.":
        "Eso ha dibujado un brush. Pulsa Ctrl+Z y haz clic con el botón DERECHO del ratón.",
    "That was another light. Right-click again and choose info > info_player_start.":
        "Eso era otra luz. Vuelve a hacer clic derecho y elige info > info_player_start.",
    "No map is open yet. Load one first: {{MenuFile}} > {{MapLoad}} - or open a mission.":
        "Aún no hay ningún mapa abierto. Carga uno primero: {{MenuFile}} > {{MapLoad}}, o abre una misión.",
    "This script has no camera yet. Insert a camera command first ({{GroupEvents}} list: double-click camera) - or open a mission with a cutscene.":
        "Este script aún no tiene cámara. Inserta primero un comando de cámara (lista {{GroupEvents}}: doble clic en camera), o abre una misión con una cinemática.",
    "Select the camera first: click a camera shot on the map or the camera row in the timeline. Its buttons appear on the left.":
        "Selecciona primero la cámara: pulsa un plano de cámara en el mapa o la fila de la cámara en la línea de tiempo. Sus botones aparecen a la izquierda.",
    "This map has no figure yet. Open a mission with figures ({{MenuFile}} > {{LoadMission}}) or place one: the tile {{EntKindNpc}} on the left edge of the view.":
        "Este mapa aún no tiene ninguna figura. Abre una misión con figuras ({{MenuFile}} > {{LoadMission}}) o coloca una: la ficha {{EntKindNpc}} en el borde izquierdo de la vista.",
    "This figure does not walk yet - its path has no walk target. Pick a figure that walks, or let one walk first ([[tut-p3]]).":
        "Esta figura aún no camina: su trayecto no tiene destino. Elige una figura que camine, o haz que una camine primero ([[tut-p3]]).",
    "A map is open, so the tab shows the map instead of the start page. Close it with {{MenuFile}} > {{MapClose}}.":
        "Hay un mapa abierto, así que la pestaña muestra el mapa en vez de la página de inicio. Ciérralo con {{MenuFile}} > {{MapClose}}.",
    "Nothing is selected. Click the door brush in a 2D view first, then right-click and choose func > func_door.":
        "No hay nada seleccionado. Pulsa primero el brush de la puerta en una vista 2D, luego haz clic derecho y elige func > func_door.",
    "That was a light. Press Ctrl+Z, select the door brush and choose func > func_door.":
        "Eso era una luz. Pulsa Ctrl+Z, selecciona el brush de la puerta y elige func > func_door.",
    "You are in {{MeBedienen}} mode - clicks press buttons there. Switch to {{MeBearbeiten}} to move elements.":
        "Estás en el modo {{MeBedienen}}: ahí los clics pulsan botones. Cambia a {{MeBearbeiten}} para mover elementos.",
    "This tutorial also runs inside the program: **{{MenuHelp}} → {{TourMenu}}**.":
        "Este tutorial también se ejecuta dentro del programa: **{{MenuHelp}} → {{TourMenu}}**.",
    "Write a script from scratch":
        "Escribir un script desde cero",
    "A new empty script, an affect block for one figure and a first command inside it.":
        "Un script nuevo vacío, un bloque affect para una figura y un primer comando dentro.",
    "Scripts · five steps":
        "Scripts · cinco pasos",
    "**Start an empty script.** Click the **+** at the end of the tab bar (or **{{MenuFile}} → {{FileNew}}**). A tab `unnamed.txt` opens; switch the mode bar to **{{ViewEvents}}**.":
        "**Empieza un script vacío.** Pulsa el **+** al final de la barra de pestañas (o **{{MenuFile}} → {{FileNew}}**). Se abre una pestaña `unnamed.txt`; cambia la barra de modos a **{{ViewEvents}}**.",
    "A new tab and the Events list.":
        "Una pestaña nueva y la lista de eventos.",
    "**Insert an affect block.** In the **{{GroupEvents}}** list on the left, double-click `affect`. It appears in the tree, already open, because a block holds commands.":
        "**Inserta un bloque affect.** En la lista **{{GroupEvents}}** de la izquierda, haz doble clic en `affect`. Aparece en el árbol, ya abierto, porque un bloque contiene comandos.",
    "**Name the figure.** Double-click the new line (or press <kbd>Enter</kbd>) to open the **{{EditorTitle}}**. Type the name of the figure into the first field - with a map loaded, the arrow next to it lists every name of the map. Leave `FLUSH` and press **{{EditorOk}}**.":
        "**Pon nombre a la figura.** Haz doble clic en la línea nueva (o pulsa <kbd>Intro</kbd>) para abrir el **{{EditorTitle}}**. Escribe el nombre de la figura en el primer campo; con un mapa cargado, la flecha de al lado lista todos los nombres del mapa. Deja `FLUSH` y pulsa **{{EditorOk}}**.",
    "The event editor for affect.":
        "El editor de eventos para affect.",
    "**Put a command inside.** Click the `affect` line once, then double-click `wait` in the list: because the selected block is open, the command goes to the start of the block. Commands inserted after that follow it, in order.":
        "**Mete un comando dentro.** Pulsa una vez la línea `affect` y luego haz doble clic en `wait` en la lista: como el bloque seleccionado está abierto, el comando va al principio del bloque. Los comandos que insertes después lo siguen, en orden.",
    "**Save.** **{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) asks for a name the first time. Script names are lower case without spaces, for example `my_first.txt`.":
        "**Guarda.** **{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) pide un nombre la primera vez. Los nombres de script van en minúsculas y sin espacios, por ejemplo `mi_primero.txt`.",
    "The finished little script.":
        "El pequeño script terminado.",
    "With a closed block selected, a new command goes after the block, not into it.":
        "Con un bloque cerrado seleccionado, un comando nuevo va detrás del bloque, no dentro.",
    "Only `affect`, `task`, `if`, `else` and `loop` can hold commands.":
        "Solo `affect`, `task`, `if`, `else` y `loop` pueden contener comandos.",
    "Steps":
        "Pasos",
    "Good to know":
        "Bueno saberlo",
    "Three ways to add a command":
        "Tres formas de añadir un comando",
    "Double-click, drag and drop, or the Insert key - and how to put a command exactly where you want it.":
        "Doble clic, arrastrar y soltar, o la tecla Insert, y cómo poner un comando exactamente donde quieres.",
    "Scripts · four steps":
        "Scripts · cuatro pasos",
    "**Double-click.** Select the line after which the command should come, then double-click the command in the **{{GroupEvents}}** list. That is how the original BehavEd worked.":
        "**Doble clic.** Selecciona la línea tras la que debe ir el comando y haz doble clic en el comando en la lista **{{GroupEvents}}**. Así funcionaba el BehavEd original.",
    "**Insert key.** Click a command in the list once (it stays highlighted), select a line in the tree and press <kbd>Insert</kbd>. Handy when you add the same command several times.":
        "**Tecla Insert.** Pulsa una vez un comando en la lista (queda resaltado), selecciona una línea en el árbol y pulsa <kbd>Insert</kbd>. Práctico cuando añades varias veces el mismo comando.",
    "**Drag.** Drag the command from the list into the tree. While you drag, a line shows where it lands: above a line, below it, or - on the middle of a block - inside the block.":
        "**Arrastra.** Arrastra el comando desde la lista al árbol. Mientras arrastras, una línea muestra dónde caerá: encima de una línea, debajo, o (en mitad de un bloque) dentro del bloque.",
    "The Events list on the left, the script tree in the middle.":
        "La lista de eventos a la izquierda, el árbol del script en el centro.",
    "**Right-click.** Right-click a line: the submenus **{{GroupEvents}}** and **Macros** insert after that line - or into the block, if you right-clicked an open block.":
        "**Clic derecho.** Haz clic derecho en una línea: los submenús **{{GroupEvents}}** y **Macros** insertan tras esa línea, o dentro del bloque si hiciste clic derecho en un bloque abierto.",
    "Hover over a command in the list to read what it does.":
        "Pasa el ratón por encima de un comando de la lista para leer lo que hace.",
    "Macros at the end of the list add several commands at once, for example `walkOnly`.":
        "Las macros al final de la lista añaden varios comandos a la vez, por ejemplo `walkOnly`.",
    "Change the values of a command":
        "Cambiar los valores de un comando",
    "The event editor: lists, numbers, file fields, names from the map - and expressions such as get, tag and random.":
        "El editor de eventos: listas, números, campos de archivo, nombres del mapa, y expresiones como get, tag y random.",
    "**Open the editor.** Double-click a line or press <kbd>Enter</kbd>. The help text of the command is on top, each value has its own field.":
        "**Abre el editor.** Haz doble clic en una línea o pulsa <kbd>Intro</kbd>. El texto de ayuda del comando está arriba y cada valor tiene su propio campo.",
    "The event editor for a camera command.":
        "El editor de eventos para un comando de cámara.",
    "**Pick from a list.** Fields with fixed choices are lists you can also type into: start typing to filter. A value that is not in the list is allowed (mods bring their own), but it is marked **unknown** - check for a typo.":
        "**Elige de una lista.** Los campos con opciones fijas son listas en las que también puedes escribir: empieza a escribir para filtrar. Se permite un valor que no esté en la lista (los mods traen los suyos), pero se marca como **desconocido**: revisa si hay una errata.",
    "**Files and names.** File fields have **{{EditorBrowse}}** to browse and **{{EditorPlay}}** to play a sound or open a script. With a map loaded, an arrow next to name fields picks a name from the map.":
        "**Archivos y nombres.** Los campos de archivo tienen **{{EditorBrowse}}** para buscar y **{{EditorPlay}}** para reproducir un sonido o abrir un script. Con un mapa cargado, una flecha junto a los campos de nombre elige un nombre del mapa.",
    "**An expression instead of a value.** **{{EditorExpr}}** turns a field into free text. **{{EditorHelper}}** builds one for you: **{{EditorGet}}** reads a variable (`get`), **{{EditorTag}}** the place or angles of a `ref_tag` (`tag`), **{{EditorRnd}}** a random number (`random`). **{{EditorRevert}}** goes back to a plain value.":
        "**Una expresión en vez de un valor.** **{{EditorExpr}}** convierte un campo en texto libre. **{{EditorHelper}}** te construye una: **{{EditorGet}}** lee una variable (`get`), **{{EditorTag}}** el lugar o los ángulos de un `ref_tag` (`tag`), **{{EditorRnd}}** un número aleatorio (`random`). **{{EditorRevert}}** vuelve a un valor simple.",
    "**Done.** **{{EditorOk}}** (<kbd>Enter</kbd>) writes the values, **{{EditorCancel}}** (<kbd>Esc</kbd>) leaves everything as it was. Under each field you see the value it had before (**{{EditorWas}}**).":
        "**Listo.** **{{EditorOk}}** (<kbd>Intro</kbd>) escribe los valores, **{{EditorCancel}}** (<kbd>Esc</kbd>) lo deja todo como estaba. Bajo cada campo ves el valor que tenía antes (**{{EditorWas}}**).",
    "Move, copy and comment out lines":
        "Mover, copiar y comentar líneas",
    "Rearrange a script: select several lines, drag them, duplicate them, and switch lines off without deleting them.":
        "Reordena un script: selecciona varias líneas, arrástralas, duplícalas y desactiva líneas sin borrarlas.",
    "**Select.** Click a line; <kbd>Ctrl</kbd>+click adds lines, <kbd>Shift</kbd>+click selects a range. Every action works on everything selected.":
        "**Selecciona.** Pulsa una línea; <kbd>Ctrl</kbd>+clic añade líneas, <kbd>Mayús</kbd>+clic selecciona un rango. Cada acción actúa sobre todo lo seleccionado.",
    "**Move.** Drag the selection to a new place, or press <kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd> to move it one line - it stays inside its block. A block moves with everything inside it.":
        "**Mueve.** Arrastra la selección a un sitio nuevo, o pulsa <kbd>Alt</kbd>+<kbd>Arriba</kbd> / <kbd>Alt</kbd>+<kbd>Abajo</kbd> para moverla una línea; se queda dentro de su bloque. Un bloque se mueve con todo lo que contiene.",
    "**Duplicate.** **{{ActClone}}** (<kbd>Space</kbd>) inserts a copy right below - the quickest way to a second camera shot or a second walk.":
        "**Duplica.** **{{ActClone}}** (<kbd>Espacio</kbd>) inserta una copia justo debajo: la forma más rápida de tener un segundo plano de cámara o un segundo tramo de marcha.",
    "**Copy between scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>), switch tabs, select a line, **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>). Pasting puts the commands below the selected line.":
        "**Copia entre scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>), cambia de pestaña, selecciona una línea, **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>). Al pegar, los comandos van debajo de la línea seleccionada.",
    "**Switch off without deleting.** **{{ActRem}}** (<kbd>Backspace</kbd>) turns the lines into comments; the game skips them. Press it again (or <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) to switch them back on.":
        "**Desactiva sin borrar.** **{{ActRem}}** (<kbd>Retroceso</kbd>) convierte las líneas en comentarios; el juego se las salta. Púlsalo de nuevo (o <kbd>Ctrl</kbd>+<kbd>Retroceso</kbd>) para volver a activarlas.",
    "**{{ActDelete}}** (<kbd>Delete</kbd>) removes lines for good - but <kbd>Ctrl</kbd>+<kbd>Z</kbd> brings them back.":
        "**{{ActDelete}}** (<kbd>Supr</kbd>) quita las líneas definitivamente, pero <kbd>Ctrl</kbd>+<kbd>Z</kbd> las recupera.",
    "Undo anything - even one step from the middle":
        "Deshacer cualquier cosa, incluso un paso de en medio",
    "Undo and redo, the full history, and undoing one single old step while keeping everything after it.":
        "Deshacer y rehacer, el historial completo, y deshacer un único paso antiguo conservando todo lo posterior.",
    "**Undo and redo.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> undoes the last change, <kbd>Ctrl</kbd>+<kbd>Y</kbd> redoes it. The **Undo** entry in the **{{MenuEdit}}** menu names the step, for example *Undo: delete*.":
        "**Deshacer y rehacer.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> deshace el último cambio, <kbd>Ctrl</kbd>+<kbd>Y</kbd> lo rehace. La entrada **Deshacer** del menú **{{MenuEdit}}** nombra el paso, por ejemplo *Deshacer: eliminar*.",
    "**See the history.** Right-click the **Undo** button in the action column (or **{{MenuEdit}} → {{UndoListMenu}}**). The list shows every step, newest at the top.":
        "**Mira el historial.** Haz clic derecho en el botón **Deshacer** de la columna de acciones (o **{{MenuEdit}} → {{UndoListMenu}}**). La lista muestra cada paso, el más reciente arriba.",
    "The undo history.":
        "El historial de deshacer.",
    "**Go back several steps.** Click a step and press **{{UndoUpToHere}}**: that step and all newer ones are undone.":
        "**Retrocede varios pasos.** Pulsa un paso y luego **{{UndoUpToHere}}**: ese paso y todos los más recientes se deshacen.",
    "**Undo one old step only.** Click a step and press **{{UndoOnlyThis}}**: only that change is undone, everything you did after it stays.":
        "**Deshaz solo un paso antiguo.** Pulsa un paso y luego **{{UndoOnlyThis}}**: solo se deshace ese cambio y todo lo que hiciste después se queda.",
    "Entities on the map have their own undo: <kbd>Ctrl</kbd>+<kbd>Z</kbd> acts on them while the mouse is over the map or the entity list.":
        "Las entidades del mapa tienen su propio deshacer: <kbd>Ctrl</kbd>+<kbd>Z</kbd> actúa sobre ellas mientras el ratón está sobre el mapa o la lista de entidades.",
    "To take back a command to the state it had when the file was opened, right-click it → **{{ActRevertOriginal}}**.":
        "Para devolver un comando al estado que tenía al abrir el archivo, haz clic derecho en él → **{{ActRevertOriginal}}**.",
    "Find and replace in all open scripts":
        "Buscar y reemplazar en todos los scripts abiertos",
    "Rename a figure in every script of a mission at once - with the Find window.":
        "Renombra una figura en todos los scripts de una misión a la vez, con la ventana Buscar.",
    "**Open all scripts.** For a mission, **{{LoadMission}}** opens every script in its own tab.":
        "**Abre todos los scripts.** En una misión, **{{LoadMission}}** abre cada script en su propia pestaña.",
    "**Open Find.** **{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>). The window stays open while you work.":
        "**Abre Buscar.** **{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>). La ventana se queda abierta mientras trabajas.",
    "The Find window.":
        "La ventana Buscar.",
    "Finds commands by name, for example `camera`.":
        "Encuentra comandos por nombre, por ejemplo `camera`.",
    "Finds a value in any field.":
        "Encuentra un valor en cualquier campo.",
    "The new value for replacing.":
        "El valor nuevo para reemplazar.",
    "Only whole values / only in the selected lines.":
        "Solo valores completos / solo en las líneas seleccionadas.",
    "Step through the hits (<kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>).":
        "Recorre los resultados (<kbd>F3</kbd> / <kbd>Mayús</kbd>+<kbd>F3</kbd>).",
    "Replaces this hit / every hit in this script.":
        "Reemplaza este resultado / todos los de este script.",
    "Replaces in every open script - one undo step each.":
        "Reemplaza en todos los scripts abiertos: un paso de deshacer por script.",
    "**Search a value.** Type the old name into **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) and **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) step through the hits. Tick **{{FindWhole}}** so that `kyle` does not also hit `kyle2`.":
        "**Busca un valor.** Escribe el nombre antiguo en **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) y **{{FindPrevBtn}}** (<kbd>Mayús</kbd>+<kbd>F3</kbd>) recorren los resultados. Marca **{{FindWhole}}** para que `kyle` no encuentre también `kyle2`.",
    "**Search a command.** **{{FindNamed}}** finds commands by name, for example every `camera`. Both fields together find only commands that match both.":
        "**Busca un comando.** **{{FindNamed}}** encuentra comandos por nombre, por ejemplo todos los `camera`. Los dos campos juntos encuentran solo comandos que cumplen ambos.",
    "**Replace.** Type the new name into **{{FindReplaceWith}}** and press **{{FindReplaceAllDocs}}**. Each script gets one undo step.":
        "**Reemplaza.** Escribe el nombre nuevo en **{{FindReplaceWith}}** y pulsa **{{FindReplaceAllDocs}}**. Cada script recibe un paso de deshacer.",
    "Right-click a line → **{{ActFindSimilar}}** fills the Find window for you: every command of this kind, or every command with one of its values.":
        "Clic derecho en una línea → **{{ActFindSimilar}}** te rellena la ventana Buscar: todos los comandos de este tipo, o todos los que tienen uno de sus valores.",
    "Bookmarks and the split view":
        "Marcadores y vista dividida",
    "Jump between places in a long script, and compare two scripts side by side.":
        "Salta entre lugares de un script largo y compara dos scripts uno al lado del otro.",
    "**Set a bookmark.** Select a line and press <kbd>Ctrl</kbd>+<kbd>F2</kbd> (**{{MenuEdit}} → {{MenuBookmarks}}**). A mark appears next to the line.":
        "**Pon un marcador.** Selecciona una línea y pulsa <kbd>Ctrl</kbd>+<kbd>F2</kbd> (**{{MenuEdit}} → {{MenuBookmarks}}**). Aparece una marca junto a la línea.",
    "**Jump.** <kbd>F2</kbd> goes to the next bookmark, <kbd>Shift</kbd>+<kbd>F2</kbd> to the previous one. Bookmarks are kept per file.":
        "**Salta.** <kbd>F2</kbd> va al marcador siguiente, <kbd>Mayús</kbd>+<kbd>F2</kbd> al anterior. Los marcadores se guardan por archivo.",
    "**Split the view.** Open a second script, then **{{MenuFile}} → {{SplitToggle}} → 2**. The second script shows beside the first.":
        "**Divide la vista.** Abre un segundo script y luego **{{MenuFile}} → {{SplitToggle}} → 2**. El segundo script aparece junto al primero.",
    "Two scripts side by side.":
        "Dos scripts uno al lado del otro.",
    "**Copy across.** The right side is read-only, but you can drag lines from it into your script - they are copied. To edit the right one, click its tab.":
        "**Copia de un lado a otro.** El lado derecho es de solo lectura, pero puedes arrastrar líneas de él a tu script: se copian. Para editar el derecho, pulsa su pestaña.",
    "The split view works in the **{{ViewEvents}}** mode; the map and model views need the room.":
        "La vista dividida funciona en el modo **{{ViewEvents}}**; las vistas de mapa y de modelo necesitan el espacio.",
    "Compile and fix errors":
        "Compilar y corregir errores",
    "Turn the script into the .ibi the game runs, read the messages and jump to the line that is wrong.":
        "Convierte el script en el .ibi que ejecuta el juego, lee los mensajes y salta a la línea errónea.",
    "**Compile.** Press **{{Compile}}** at the bottom of the action column. It saves the `.txt` and writes the `.ibi` next to it. The button flashes green when it worked.":
        "**Compila.** Pulsa **{{Compile}}** abajo en la columna de acciones. Guarda el `.txt` y escribe el `.ibi` a su lado. El botón parpadea en verde cuando funciona.",
    "The Compile button.":
        "El botón Compilar.",
    "**When it fails.** The button flashes red, the status line says *Compile failed*, and the first wrong line is selected in the tree.":
        "**Si falla.** El botón parpadea en rojo, la línea de estado dice *Error al compilar* y la primera línea errónea queda seleccionada en el árbol.",
    "**Read the messages.** Click the red or yellow counter in the status line at the bottom: the **{{MessagesTitle}}** window lists every message. Each one names the line and what is wrong.":
        "**Lee los mensajes.** Pulsa el contador rojo o amarillo de la línea de estado de abajo: la ventana **{{MessagesTitle}}** lista todos los mensajes. Cada uno nombra la línea y qué está mal.",
    "**Fix and compile again.** Usually it is a value in the wrong form - text without quotes, a missing `}`, a comparison other than `= < > !`. Fix it in the editor and press **{{Compile}}** again.":
        "**Corrige y compila de nuevo.** Casi siempre es un valor con forma incorrecta: texto sin comillas, una `}` que falta, una comparación distinta de `= < > !`. Corrígelo en el editor y vuelve a pulsar **{{Compile}}**.",
    "For a mission, **{{MissionSave}}** compiles every script at once and refuses to pack while one does not compile.":
        "En una misión, **{{MissionSave}}** compila todos los scripts a la vez y se niega a empaquetar mientras uno no compile.",
    "Every save and compile keeps a copy: **{{MenuFile}} → {{FileBackupFolder}}** shows the last ten.":
        "Cada guardado y cada compilación conservan una copia: **{{MenuFile}} → {{FileBackupFolder}}** muestra las diez últimas.",
    "Wait until a figure has arrived":
        "Esperar a que una figura haya llegado",
    "task, dowait and do: let a figure finish its walk before it turns or speaks - or let it do two things at once.":
        "task, dowait y do: deja que una figura termine su marcha antes de girarse o hablar, o que haga dos cosas a la vez.",
    "**Why.** An `affect` block runs beside the script - the script does not wait for it, so two figures in two blocks already walk at the same time. But inside a block, `set ( SET_NAVGOAL )` only *starts* the walk: the next command follows at once, and the figure would turn or talk while still walking.":
        "**Por qué.** Un bloque `affect` se ejecuta junto al script: el script no lo espera, así que dos figuras en dos bloques ya caminan a la vez. Pero dentro de un bloque, `set ( SET_NAVGOAL )` solo *inicia* la marcha: el comando siguiente sigue enseguida y la figura se giraría o hablaría aún caminando.",
    "**Put the walk into a task.** Inside the figure's `affect` block insert `task` and name it, for example `walk`. Move the `SET_NAVGOAL` (and `SET_WALKING`, if you use it) into the task block.":
        "**Mete la marcha en una tarea.** Dentro del bloque `affect` de la figura inserta `task` y ponle nombre, por ejemplo `walk`. Mueve el `SET_NAVGOAL` (y `SET_WALKING`, si lo usas) al bloque de la tarea.",
    "**dowait.** After the task insert `dowait ( \"walk\" )`. It starts the task and waits until it is done - until the figure has arrived. Everything after it happens when the figure stands.":
        "**dowait.** Tras la tarea inserta `dowait ( \"walk\" )`. Inicia la tarea y espera a que termine, hasta que la figura haya llegado. Todo lo que va después ocurre cuando la figura está parada.",
    "Two affect blocks, each with a walk task and dowait.":
        "Dos bloques affect, cada uno con una tarea de marcha y dowait.",
    "**Two things at once.** `do ( \"walk\" )` starts a task and goes on at once. Declare a second task - for example `look` with a `SET_LOOK_TARGET` - and start both with `do`. Later, `wait ( \"walk\" )` waits for the first one.":
        "**Dos cosas a la vez.** `do ( \"walk\" )` inicia una tarea y sigue enseguida. Declara una segunda tarea (por ejemplo `look` con un `SET_LOOK_TARGET`) e inicia las dos con `do`. Más tarde, `wait ( \"walk\" )` espera a la primera.",
    "**Check it in the timeline.** Play the scene: what follows a `dowait` starts where the walk block ends. Drag the walk target, and the rest moves with it.":
        "**Compruébalo en la línea de tiempo.** Reproduce la escena: lo que sigue a un `dowait` empieza donde termina el bloque de la marcha. Arrastra el destino y el resto se mueve con él.",
    "Commands marked with `*` in their help text (such as `SET_NAVGOAL`) finish their task only when the action is done - that is what makes `dowait` wait.":
        "Los comandos marcados con `*` en su texto de ayuda (como `SET_NAVGOAL`) solo terminan su tarea cuando la acción ha concluido: eso es lo que hace esperar a `dowait`.",
    "Let two scripts wait for each other":
        "Hacer que dos scripts se esperen",
    "signal and waitsignal: one script pauses until the other says go.":
        "signal y waitsignal: un script se detiene hasta que el otro da la señal.",
    "**Where to wait.** In the script that should pause, insert `waitsignal` and type a name, for example `door_open`.":
        "**Dónde esperar.** En el script que debe detenerse, inserta `waitsignal` y escribe un nombre, por ejemplo `door_open`.",
    "**Where to give the signal.** In the other script, at the moment it should go on, insert `signal ( \"door_open\" )`.":
        "**Dónde dar la señal.** En el otro script, en el momento en que debe seguir, inserta `signal ( \"door_open\" )`.",
    "**Check the names.** Both must be spelled the same. Only one script can wait for a signal of a given name.":
        "**Comprueba los nombres.** Los dos deben escribirse igual. Solo un script puede esperar una señal de un nombre dado.",
    "The timeline cannot know how long a `waitsignal` waits in the game - everything after it is placed as if the signal came at once.":
        "La línea de tiempo no puede saber cuánto espera un `waitsignal` en el juego: todo lo que va después se coloca como si la señal llegara enseguida.",
    "Use macros":
        "Usar macros",
    "walkOnly, runOnly, standOnly and the patrols: several set commands in one line.":
        "walkOnly, runOnly, standOnly y las patrullas: varios comandos set en una línea.",
    "Scripts · three steps":
        "Scripts · tres pasos",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list, each with the number of commands it holds. Hover over one to read what it does.":
        "**Encuéntralas.** Las macros están al final de la lista **{{GroupEvents}}**, cada una con el número de comandos que contiene. Pasa el ratón por encima de una para leer lo que hace.",
    "**Insert.** Double-click or drag a macro like a command. `walkOnly`, for example, makes a figure walk and ignore enemies and alerts - five `set` commands.":
        "**Inserta.** Haz doble clic o arrastra una macro como un comando. `walkOnly`, por ejemplo, hace que una figura camine e ignore enemigos y alertas: cinco comandos `set`.",
    "A macro in the tree.":
        "Una macro en el árbol.",
    "**Fold or unfold.** **{{TreeFoldMacros}}** in the action column shows a macro as one line. Switch it off to see and change the single commands.":
        "**Pliega o despliega.** **{{TreeFoldMacros}}** en la columna de acciones muestra una macro como una sola línea. Desactívalo para ver y cambiar cada comando.",
    "Fly a camera shot yourself":
        "Volar tú mismo un plano de cámara",
    "Pilot mode: sit in the camera, fly with the mouse and WASD, and set keys along the timeline.":
        "Modo piloto: siéntate en la cámara, vuela con el ratón y WASD, y pon claves a lo largo de la línea de tiempo.",
    "Camera · five steps":
        "Cámara · cinco pasos",
    "**Select a shot.** Click the camera on the map or a shot in the **{{CamShots}}** list on the left.":
        "**Selecciona un plano.** Pulsa la cámara en el mapa o un plano de la lista **{{CamShots}}** de la izquierda.",
    "**Start Pilot.** Press **{{CamPilot}}** on the camera bar. You now look through this shot.":
        "**Inicia el piloto.** Pulsa **{{CamPilot}}** en la barra de la cámara. Ahora miras a través de este plano.",
    "**Fly.** Hold the right mouse button to look around, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move, <kbd>Q</kbd> / <kbd>E</kbd> go down and up, <kbd>Shift</kbd> is faster.":
        "**Vuela.** Mantén el botón derecho para mirar alrededor; <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> mueven, <kbd>Q</kbd> / <kbd>E</kbd> bajan y suben, <kbd>Mayús</kbd> va más rápido.",
    "Flying a shot in pilot mode.":
        "Volando un plano en modo piloto.",
    "**Set keys along the way.** Move the playhead in the timeline, fly to where the camera should be at that moment and press <kbd>Enter</kbd> (**{{CamPilotKey}}**). Repeat for more keys.":
        "**Pon claves por el camino.** Mueve el cabezal en la línea de tiempo, vuela hasta donde debe estar la cámara en ese momento y pulsa <kbd>Intro</kbd> (**{{CamPilotKey}}**). Repite para más claves.",
    "**Finish.** **{{CamPilotApply}}** writes the view into the script; <kbd>Esc</kbd> (**{{CamPilotCancel}}**) leaves it as it was.":
        "**Termina.** **{{CamPilotApply}}** escribe la vista en el script; <kbd>Esc</kbd> (**{{CamPilotCancel}}**) lo deja como estaba.",
    "Put a shot where you are looking":
        "Poner un plano donde estás mirando",
    "Look around until the picture is right, then move a shot - or a new one - exactly there.":
        "Mira alrededor hasta que la imagen sea la buena y mueve ahí exactamente un plano, o uno nuevo.",
    "Camera · four steps":
        "Cámara · cuatro pasos",
    "**Fly to the view you want.** Right mouse button + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>.":
        "**Vuela hasta la vista que quieres.** Botón derecho + <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>.",
    "**A new shot.** **{{CamNewShot}}** adds `camera ( MOVE )` and `camera ( PAN )` at your view, after the selected shot.":
        "**Un plano nuevo.** **{{CamNewShot}}** añade `camera ( MOVE )` y `camera ( PAN )` en tu vista, tras el plano seleccionado.",
    "The camera panel with its shots.":
        "El panel de la cámara con sus planos.",
    "**Or move an existing one.** Select a shot and press **{{CamSetToView}}**: position and direction are replaced, one undo step.":
        "**O mueve uno existente.** Selecciona un plano y pulsa **{{CamSetToView}}**: posición y dirección se sustituyen, un paso de deshacer.",
    "**Check.** **{{MapThroughCam}}** on the map toolbar shows what the selected shot sees, with its zoom and the black cinema bars.":
        "**Comprueba.** **{{MapThroughCam}}** en la barra del mapa muestra lo que ve el plano seleccionado, con su zoom y las barras negras de cine.",
    "Save views you come back to: <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd> stores the view, <kbd>1</kbd> … <kbd>9</kbd> jumps back. They are kept per map.":
        "Guarda las vistas a las que vuelves: <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd> guarda la vista, <kbd>1</kbd> … <kbd>9</kbd> vuelve a ella. Se guardan por mapa.",
    "Aim the camera at a mark on the map":
        "Apuntar la cámara a una marca del mapa",
    "Use a ref_tag as the camera position - move the mark, and the shot follows.":
        "Usa un ref_tag como posición de la cámara: mueve la marca y el plano la sigue.",
    "**Place a mark.** Click the tile **{{EntKindTag}}** on the left edge of the view, then click where the camera should stand. Give it a **{{EntName}}** in the inspector, such as `cam_door`.":
        "**Coloca una marca.** Pulsa la ficha **{{EntKindTag}}** en el borde izquierdo de la vista y luego pulsa donde debe estar la cámara. Dale un **{{EntName}}** en el inspector, como `cam_puerta`.",
    "**Camera to the mark.** Right-click the mark in the entity list → **{{EntityCamToTag}}**. Two camera commands are inserted that use the mark's place and angles.":
        "**Cámara a la marca.** Clic derecho en la marca en la lista de entidades → **{{EntityCamToTag}}**. Se insertan dos comandos de cámara que usan el lugar y los ángulos de la marca.",
    "The context menu of a mark.":
        "El menú contextual de una marca.",
    "**Move the mark.** Drag the mark in the view or change its **{{EntOrigin}}**: the shot moves with it.":
        "**Mueve la marca.** Arrastra la marca en la vista o cambia su **{{EntOrigin}}**: el plano se mueve con ella.",
    "**Turn it.** The mark's facing is the camera's direction. If the mark aims at a target, move the target to change the view.":
        "**Gírala.** La orientación de la marca es la dirección de la cámara. Si la marca apunta a un objetivo, mueve el objetivo para cambiar la vista.",
    "Zoom, fade to black and shake":
        "Zoom, fundido a negro y temblor",
    "The camera commands ZOOM, FADE and SHAKE - and where to see them in the timeline.":
        "Los comandos de cámara ZOOM, FADE y SHAKE, y dónde verlos en la línea de tiempo.",
    "**Insert a camera command.** Double-click `camera` in the **{{GroupEvents}}** list, then open it with <kbd>Enter</kbd>. The first field chooses what it does.":
        "**Inserta un comando de cámara.** Haz doble clic en `camera` en la lista **{{GroupEvents}}** y ábrelo con <kbd>Intro</kbd>. El primer campo elige lo que hace.",
    "**Zoom.** Choose `ZOOM`: the first number is the field of view (80 is normal, smaller is closer), the second the time in milliseconds to get there.":
        "**Zoom.** Elige `ZOOM`: el primer número es el campo de visión (80 es normal, menos es más cerca), el segundo el tiempo en milisegundos para llegar.",
    "**Fade.** Choose `FADE`: start colour and opacity, end colour and opacity (0 to 1), and the time. `0 0 0`, `0` to `0 0 0`, `1` fades to black.":
        "**Fundido.** Elige `FADE`: color y opacidad iniciales, color y opacidad finales (de 0 a 1), y el tiempo. `0 0 0`, `0` a `0 0 0`, `1` funde a negro.",
    "The camera rows of the timeline.":
        "Las filas de cámara de la línea de tiempo.",
    "Retime a cutscene in the timeline":
        "Reajustar los tiempos de una cinemática en la línea de tiempo",
    "Make a shot longer, start a line of dialogue later, move a whole part - by dragging blocks.":
        "Alarga un plano, empieza más tarde una frase de diálogo, mueve una parte entera, arrastrando bloques.",
    "Timeline · five steps":
        "Línea de tiempo · cinco pasos",
    "**Play it.** Press ▶ under the map and watch. Switch **Sound** on to hear the dialogue you are matching the cuts against.":
        "**Reprodúcela.** Pulsa ▶ bajo el mapa y mira. Activa **Sonido** para oír el diálogo con el que cuadras los cortes.",
    "**Move a block.** Drag a block left or right. Everything after it moves along, so the rest stays in sync. Hold <kbd>Alt</kbd> to move only this block.":
        "**Mueve un bloque.** Arrastra un bloque a izquierda o derecha. Todo lo que va detrás se mueve también, así el resto sigue sincronizado. Mantén <kbd>Alt</kbd> para mover solo este bloque.",
    "Dragging a block in the timeline: everything after it moves along.":
        "Arrastrando un bloque en la línea de tiempo: todo lo que va detrás se mueve también.",
    "**Change a duration.** Drag the right edge of a block - a camera move, a wait - to make it shorter or longer.":
        "**Cambia una duración.** Arrastra el borde derecho de un bloque (un movimiento de cámara, una espera) para acortarlo o alargarlo.",
    "**Check the script.** Every drag changes a value in the script (a wait, a time). Click a block to see its line.":
        "**Revisa el script.** Cada arrastre cambia un valor del script (una espera, un tiempo). Pulsa un bloque para ver su línea.",
    "A warning sign on a camera block means it overlaps the next one - in the game the next move would start from where this one began, and the camera jumps.":
        "Una señal de aviso en un bloque de cámara significa que se solapa con el siguiente: en el juego, el siguiente movimiento empezaría desde donde empezó este y la cámara saltaría.",
    "Look through the camera while you work":
        "Mirar a través de la cámara mientras trabajas",
    "Through camera, the small preview picture, follow mode and the speed of playback.":
        "A través de la cámara, la pequeña imagen de vista previa, el modo seguir y la velocidad de reproducción.",
    "**Through camera.** **{{MapThroughCam}}** on the map toolbar shows the picture of the selected shot, with zoom and cinema bars.":
        "**A través de la cámara.** **{{MapThroughCam}}** en la barra del mapa muestra la imagen del plano seleccionado, con zoom y barras de cine.",
    "**The small preview.** **{{CamPreview}}** on the camera bar puts a small picture of the camera's view in the bottom right corner - you keep flying freely. Click the picture to look through it.":
        "**La vista previa pequeña.** **{{CamPreview}}** en la barra de la cámara pone una pequeña imagen de lo que ve la cámara en la esquina inferior derecha; tú sigues volando libremente. Pulsa la imagen para mirar a través de ella.",
    "Looking through the camera while playing.":
        "Mirando a través de la cámara durante la reproducción.",
    "**Follow.** **{{TlFollow}}** in the timeline moves your view along with the camera while it plays.":
        "**Seguir.** **{{TlFollow}}** en la línea de tiempo mueve tu vista junto con la cámara mientras se reproduce.",
    "**Slow motion.** The speed list next to the play button plays at 0.25x to 2x. Sound plays only at 1x.":
        "**Cámara lenta.** La lista de velocidad junto al botón de reproducir va de 0,25x a 2x. El sonido solo suena a 1x.",
    "Walk, run or walk backwards":
        "Caminar, correr o caminar hacia atrás",
    "Change how a figure moves - right on its path in the map.":
        "Cambia cómo se mueve una figura, directamente en su trayecto en el mapa.",
    "Figures · four steps":
        "Figuras · cuatro pasos",
    "**Show the path.** Click the figure on the map or its name in the timeline. Its path appears: grey ahead of the playhead, yellow where it already walked.":
        "**Muestra el trayecto.** Pulsa la figura en el mapa o su nombre en la línea de tiempo. Aparece su trayecto: gris por delante del cabezal, amarillo por donde ya caminó.",
    "**Right-click a walk target** on the path. The menu offers **{{BahnGehen}}**, **{{BahnRennen}}** and **{{BahnRueckwaerts}}**.":
        "**Clic derecho en un destino** del trayecto. El menú ofrece **{{BahnGehen}}**, **{{BahnRennen}}** y **{{BahnRueckwaerts}}**.",
    "The menu of a walk target.":
        "El menú de un destino.",
    "**Pick one.** behaved writes `SET_WALKING` / `SET_RUNNING` before this walk. It counts from this walk on - later walks keep it unless you change them too.":
        "**Elige uno.** behaved escribe `SET_WALKING` / `SET_RUNNING` antes de este tramo. Vale a partir de este tramo; los siguientes lo mantienen salvo que también los cambies.",
    "**Watch.** Play the timeline: the figure is faster, and the walk block in the timeline is shorter.":
        "**Mira.** Reproduce la línea de tiempo: la figura va más rápida y el bloque de la marcha en la línea de tiempo es más corto.",
    "Make a figure look at someone":
        "Hacer que una figura mire a alguien",
    "Look at another figure while walking, turn towards a target, or face the walking direction again.":
        "Mirar a otra figura mientras camina, girarse hacia un objetivo o volver a mirar en la dirección de la marcha.",
    "**While walking.** Right-click a walk target on the path → **{{BahnAnsehen}}** → pick a figure. The figure walks and keeps its head on the other one.":
        "**Mientras camina.** Clic derecho en un destino del trayecto → **{{BahnAnsehen}}** → elige una figura. La figura camina y mantiene la cabeza en la otra.",
    "The menu of a walk target: Look at while walking lists the figures of the scene.":
        "El menú de un destino: Mirar a esto al caminar lista las figuras de la escena.",
    "**Standing.** Right-click the target entity in the entity list → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) or **{{EntityAsLook}}** (`SET_LOOK_TARGET`). The command goes into the script after the selected line.":
        "**De pie.** Clic derecho en la entidad objetivo en la lista de entidades → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) o **{{EntityAsLook}}** (`SET_LOOK_TARGET`). El comando va al script tras la línea seleccionada.",
    "**Back to normal.** Right-click the walk target → **{{BahnBlickFrei}}**.":
        "**Volver a lo normal.** Clic derecho en el destino → **{{BahnBlickFrei}}**.",
    "Turn a figure at a moment":
        "Girar una figura en un momento dado",
    "Set which way a figure faces at a certain time - with the rotate ring on the map.":
        "Fija hacia dónde mira una figura en un momento concreto, con el anillo de rotación en el mapa.",
    "**Go to the moment.** Move the playhead in the timeline to where the figure should face another way.":
        "**Ve al momento.** Mueve el cabezal en la línea de tiempo hasta donde la figura debe mirar hacia otro lado.",
    "**Select the figure** on the map and switch the gizmo bar to **{{GizmoRotate}}**.":
        "**Selecciona la figura** en el mapa y cambia la barra del gizmo a **{{GizmoRotate}}**.",
    "**Turn.** Drag the ring. When you let go, behaved writes the facing at this time into the script (`SET_DYAW` for an NPC, which then turns with its turn rate as in the game; `SET_ANGLES` for the player), and the status line says *Facing … degrees at … s*.":
        "**Gira.** Arrastra el anillo. Al soltar, behaved escribe la orientación en ese momento en el script (`SET_DYAW` para un NPC, que gira entonces con su velocidad de giro como en el juego; `SET_ANGLES` para el jugador), y la línea de estado dice *Orientación … grados en … s*.",
    "The rotate ring.":
        "El anillo de rotación.",
    "**During a walk.** Turning while the figure walks sets the facing for when it arrives.":
        "**Durante una marcha.** Girar mientras la figura camina fija la orientación para cuando llegue.",
    "If the time is not known - a `waitsignal` or `dowait` before - behaved says so instead of guessing.":
        "Si no se conoce el momento (hay un `waitsignal` o un `dowait` antes), behaved lo dice en vez de adivinar.",
    "Let a figure play an animation":
        "Hacer que una figura reproduzca una animación",
    "Find an animation in the model view, watch it, and put it into the script with one click.":
        "Encuentra una animación en la vista de modelos, mírala y ponla en el script con un clic.",
    "Figures · five steps":
        "Figuras · cinco pasos",
    "**Load the figure's model.** Switch to **{{ViewModel}}** and press **{{NpcBrowser}}**: click a character - its model and skin load as in the game.":
        "**Carga el modelo de la figura.** Cambia a **{{ViewModel}}** y pulsa **{{NpcBrowser}}**: pulsa un personaje y su modelo y su skin se cargan como en el juego.",
    "**Find an animation.** Type into **{{ModelAnimSearch}}**, for example `TALK`. **BOTH**, **TORSO**, **LEGS** filter by body part.":
        "**Busca una animación.** Escribe en **{{ModelAnimSearch}}**, por ejemplo `TALK`. **BOTH**, **TORSO**, **LEGS** filtran por parte del cuerpo.",
    "The model view with its animation list.":
        "La vista de modelos con su lista de animaciones.",
    "**Watch it.** Click an animation: it plays. **{{ModelLoop}}**, the speed and the frame buttons help to look closely.":
        "**Mírala.** Pulsa una animación: se reproduce. **{{ModelLoop}}**, la velocidad y los botones de fotograma ayudan a mirar de cerca.",
    "**Into the script.** Select a line in your script first (inside the figure's `affect` block), then double-click the animation or press **{{ModelAnimToScript}}**: `set ( SET_ANIM_BOTH, \"…\" )` is inserted after that line.":
        "**Al script.** Selecciona antes una línea de tu script (dentro del bloque `affect` de la figura) y luego haz doble clic en la animación o pulsa **{{ModelAnimToScript}}**: se inserta `set ( SET_ANIM_BOTH, \"…\" )` tras esa línea.",
    "**See it on the map.** Switch back to **{{ViewMap}}** and play the timeline: the figure plays the animation at that point.":
        "**Mírala en el mapa.** Vuelve a **{{ViewMap}}** y reproduce la línea de tiempo: la figura reproduce la animación en ese punto.",
    "Add a line of dialogue":
        "Añadir una frase de diálogo",
    "A sound on a figure's voice channel, listened to and timed against the camera.":
        "Un sonido en el canal de voz de una figura, escuchado y sincronizado con la cámara.",
    "**Insert sound.** Inside the figure's `affect` block, insert `sound`. In the editor choose the channel `CHAN_VOICE` and pick the file with **{{EditorBrowse}}**; **{{EditorPlay}}** plays it.":
        "**Inserta sound.** Dentro del bloque `affect` de la figura, inserta `sound`. En el editor elige el canal `CHAN_VOICE` y escoge el archivo con **{{EditorBrowse}}**; **{{EditorPlay}}** lo reproduce.",
    "**Hear it in the scene.** Switch **{{TlAudio}}** on in the timeline and play: the line sounds at its moment.":
        "**Óyela en la escena.** Activa **{{TlAudio}}** en la línea de tiempo y reproduce: la frase suena en su momento.",
    "**See it.** The **{{FlowTracks}}** view (top of the script column) has a **{{TracksDialog}}** row - every voice line as a block, so you see where the cuts fall.":
        "**Mírala.** La vista **{{FlowTracks}}** (arriba de la columna del script) tiene una fila **{{TracksDialog}}**: cada frase como un bloque, así ves dónde caen los cortes.",
    "The Tracks view.":
        "La vista de pistas.",
    "**Time it.** A `wait` after the sound keeps the next command until the line is over. Drag the wait's edge in the timeline to fit.":
        "**Sincronízala.** Un `wait` tras el sonido retiene el comando siguiente hasta que termina la frase. Arrastra el borde de la espera en la línea de tiempo para ajustarla.",
    "Place, move and turn entities":
        "Colocar, mover y girar entidades",
    "Put a walk target on the floor, move it on the grid, turn it, copy it.":
        "Pon un destino en el suelo, muévelo por la rejilla, gíralo, cópialo.",
    "Map · five steps":
        "Mapa · cinco pasos",
    "**Place.** Click a tile on the left edge of the view - for example **{{EntKindNav}}** - then click on the floor. Hold <kbd>Shift</kbd> to place several. <kbd>Esc</kbd> stops.":
        "**Coloca.** Pulsa una ficha en el borde izquierdo de la vista (por ejemplo **{{EntKindNav}}**) y luego pulsa en el suelo. Mantén <kbd>Mayús</kbd> para colocar varios. <kbd>Esc</kbd> para.",
    "Placing walk targets.":
        "Colocando destinos.",
    "**Move.** Drag the entity over the floor, or drag a coloured arrow to move it along one axis. <kbd>Ctrl</kbd> snaps to 8 units.":
        "**Mueve.** Arrastra la entidad por el suelo, o arrastra una flecha de color para moverla a lo largo de un eje. <kbd>Ctrl</kbd> ajusta a 8 unidades.",
    "**Turn.** Switch the gizmo bar to **{{GizmoRotate}}** and drag the ring; <kbd>Ctrl</kbd> turns in 15 degree steps.":
        "**Gira.** Cambia la barra del gizmo a **{{GizmoRotate}}** y arrastra el anillo; <kbd>Ctrl</kbd> gira en pasos de 15 grados.",
    "**Copy.** <kbd>Alt</kbd>+drag makes a copy and moves it. Or press **{{EntDuplicate}}** in the inspector.":
        "**Copia.** <kbd>Alt</kbd>+arrastrar hace una copia y la mueve. O pulsa **{{EntDuplicate}}** en el inspector.",
    "**Onto the floor.** **{{EntDropFloor}}** in the inspector puts it exactly on the floor below - the inspector shows how high the feet are.":
        "**Al suelo.** **{{EntDropFloor}}** en el inspector la pone exactamente sobre el suelo de debajo; el inspector muestra a qué altura están los pies.",
    "**{{Grid}}** on the map toolbar snaps everything to 8 to 64 units while dragging and placing.":
        "**{{Grid}}** en la barra del mapa ajusta todo a 8 a 64 unidades al arrastrar y colocar.",
    "Find an entity and fly to it":
        "Encontrar una entidad y volar hasta ella",
    "Search the entity list, fly there, hide what is in the way.":
        "Busca en la lista de entidades, vuela hasta allí, oculta lo que estorba.",
    "Map · four steps":
        "Mapa · cuatro pasos",
    "**Search.** Type a name or class into the search box above the entity list, for example `navgoal` or `kyle`.":
        "**Busca.** Escribe un nombre o una clase en el campo de búsqueda encima de la lista de entidades, por ejemplo `navgoal` o `kyle`.",
    "**Fly there.** Double-click the row: the view flies to the entity and selects it.":
        "**Vuela hasta allí.** Haz doble clic en la fila: la vista vuela hasta la entidad y la selecciona.",
    "**Hide groups.** Click the eye in front of a group to hide it in the view; <kbd>Alt</kbd>+click shows only that group. **{{EntityShowAll}}** brings everything back.":
        "**Oculta grupos.** Pulsa el ojo delante de un grupo para ocultarlo en la vista; <kbd>Alt</kbd>+clic muestra solo ese grupo. **{{EntityShowAll}}** lo devuelve todo.",
    "The entity list with its groups.":
        "La lista de entidades con sus grupos.",
    "**Names on the map.** **{{MapNames}}** on the map toolbar writes every name into the view.":
        "**Nombres en el mapa.** **{{MapNames}}** en la barra del mapa escribe cada nombre en la vista.",
    "Edit an entity's keys":
        "Editar las claves de una entidad",
    "Name, position, spawnflags and all other keys in the inspector - and the NPC type search.":
        "Nombre, posición, spawnflags y todas las demás claves en el inspector, y la búsqueda de tipos de NPC.",
    "**Select the entity.** Click it in the view or in the list. The inspector under the list shows its class in colour; hover over it for a description from the game code.":
        "**Selecciona la entidad.** Púlsala en la vista o en la lista. El inspector bajo la lista muestra su clase en color; pasa el ratón por encima para leer una descripción del código del juego.",
    "**General.** **{{EntName}}** is how scripts address it, **{{EntOrigin}}** and **{{EntAngle}}** where it stands and faces.":
        "**General.** **{{EntName}}** es como la nombran los scripts, **{{EntOrigin}}** y **{{EntAngle}}** dónde está y hacia dónde mira.",
    "The inspector.":
        "El inspector.",
    "Flies the view to the entity.":
        "Hace volar la vista hasta la entidad.",
    "Puts it exactly on the floor below.":
        "La pone exactamente sobre el suelo de debajo.",
    "Copies or removes it - every selected entity.":
        "La copia o la quita: cada entidad seleccionada.",
    "Undo / Redo":
        "Deshacer / Rehacer",
    "The history of entity changes, separate from the script's.":
        "El historial de cambios de entidades, aparte del del script.",
    "Shows every key as plain text.":
        "Muestra cada clave como texto plano.",
    "Filter fields":
        "Filtrar campos",
    "Shows only the keys whose name contains the text.":
        "Muestra solo las claves cuyo nombre contiene el texto.",
    "Next to `NPC_type`: searches every character of the game.":
        "Junto a `NPC_type`: busca entre todos los personajes del juego.",
    "**Keys.** Under **{{EntSecFields}}**, change values or add one with **{{EntAddKey}}**. For a character, the **...** next to `NPC_type` searches every NPC of the game. Script keys have **{{EntOpenScript}}** to open the script.":
        "**Claves.** En **{{EntSecFields}}**, cambia valores o añade uno con **{{EntAddKey}}**. Para un personaje, el **...** junto a `NPC_type` busca entre todos los NPC del juego. Las claves de script tienen **{{EntOpenScript}}** para abrir el script.",
    "**Everything as text.** **{{EntRaw}}** shows all keys as plain text, like Hammer without SmartEdit.":
        "**Todo como texto.** **{{EntRaw}}** muestra todas las claves como texto plano, como Hammer sin SmartEdit.",
    "Entity changes are kept with the mission: **{{MissionSave}}** writes them into the `.ent` file.":
        "Los cambios de entidades se guardan con la misión: **{{MissionSave}}** los escribe en el archivo `.ent`.",
    "Look at the map from above":
        "Mirar el mapa desde arriba",
    "Top view with a cut height - to see the floor plan of a level, one floor at a time.":
        "Vista superior con una altura de corte: para ver el plano de un nivel, planta a planta.",
    "Map · three steps":
        "Mapa · tres pasos",
    "**Switch on {{MapTopView}}** on the map toolbar. The view looks straight down; the wheel zooms, right or middle mouse and <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move.":
        "**Activa {{MapTopView}}** en la barra del mapa. La vista mira hacia abajo en vertical; la rueda amplía, el botón derecho o central y <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> mueven.",
    "The top view.":
        "La vista superior.",
    "**Cut away ceilings.** Drag the **cut height** box at the top right: everything higher is not drawn. Lower it to look at a lower floor.":
        "**Recorta los techos.** Arrastra el cuadro de **altura de corte** arriba a la derecha: lo que está más alto no se dibuja. Bájalo para ver una planta inferior.",
    "**Work as usual.** Selecting, dragging and placing work in the top view too - handy for paths and waypoints.":
        "**Trabaja como siempre.** Seleccionar, arrastrar y colocar también funcionan en la vista superior: práctico para trayectos y waypoints.",
    "See better in dark maps":
        "Ver mejor en mapas oscuros",
    "Brightness, ambient light and switching effects, fog or sky off while you work.":
        "Brillo, luz ambiental, y desactivar efectos, niebla o cielo mientras trabajas.",
    "**Open {{MapViewSettings}}** (the gear on the map toolbar). **{{MapBrightness}}** lightens everything, **{{MapMinLight}}** lifts only the darkest areas - 0 is exactly like the game.":
        "**Abre {{MapViewSettings}}** (el engranaje de la barra del mapa). **{{MapBrightness}}** aclara todo, **{{MapMinLight}}** levanta solo las zonas más oscuras; 0 es exactamente como el juego.",
    "The view settings.":
        "Los ajustes de la vista.",
    "**Switch things off.** **{{MapFog}}**, **{{MapEffects}}**, **{{MapGlow}}** and **{{MapSky}}** on the toolbar switch those parts off. Without the sky you can look into the map from outside.":
        "**Desactiva cosas.** **{{MapFog}}**, **{{MapEffects}}**, **{{MapGlow}}** y **{{MapSky}}** en la barra desactivan esas partes. Sin el cielo puedes mirar el mapa desde fuera.",
    "**Only this script.** **{{MapScriptFocus}}** in the view settings dims every entity the active script does not use.":
        "**Solo este script.** **{{MapScriptFocus}}** en los ajustes de la vista atenúa todas las entidades que el script activo no usa.",
    "A door that opens":
        "Una puerta que se abre",
    "Animate a door or platform with move commands: two keys, and the script moves it.":
        "Anima una puerta o una plataforma con comandos move: dos claves y el script la mueve.",
    "Animation · five steps":
        "Animación · cinco pasos",
    "**Select the door.** Click it in the view - doors and platforms are `func_` entities. In the inspector press **{{EntAnimate}}**.":
        "**Selecciona la puerta.** Púlsala en la vista: las puertas y plataformas son entidades `func_`. En el inspector pulsa **{{EntAnimate}}**.",
    "**Mode.** In the **{{AnimTitle}}** window keep **{{AnimModeMove}}**: the movement becomes `move` commands in the script.":
        "**Modo.** En la ventana **{{AnimTitle}}** deja **{{AnimModeMove}}**: el movimiento se convierte en comandos `move` en el script.",
    "The Animation window.":
        "La ventana de animación.",
    "**First key.** Put the playhead where the door should start moving and press **{{AnimAddKey}}** - this is the closed position.":
        "**Primera clave.** Pon el cabezal donde la puerta debe empezar a moverse y pulsa **{{AnimAddKey}}**: esta es la posición cerrada.",
    "**Second key.** Move the playhead later (for example 2 seconds) and drag the door up or aside in the view: while the window is open, dragging sets a key at the playhead.":
        "**Segunda clave.** Mueve el cabezal más tarde (por ejemplo 2 segundos) y arrastra la puerta hacia arriba o a un lado en la vista: mientras la ventana está abierta, arrastrar pone una clave en el cabezal.",
    "**Play.** The door now opens in the timeline. **{{AnimLinear}}** makes it move at constant speed instead of easing out.":
        "**Reproduce.** La puerta se abre ahora en la línea de tiempo. **{{AnimLinear}}** hace que se mueva a velocidad constante en vez de frenar al final.",
    "Animated entities need a name; behaved gives the door one if it has none.":
        "Las entidades animadas necesitan un nombre; behaved le da uno a la puerta si no lo tiene.",
    "Change a ROFF file of the game":
        "Cambiar un archivo ROFF del juego",
    "Turn a ROFF flight from the game into keys, change it, and save it as your own copy.":
        "Convierte un vuelo ROFF del juego en claves, cámbialo y guárdalo como tu propia copia.",
    "Animation · four steps":
        "Animación · cuatro pasos",
    "**Select the entity** that plays the ROFF (a ship, a boulder) and press **{{EntAnimate}}**.":
        "**Selecciona la entidad** que reproduce el ROFF (una nave, una roca) y pulsa **{{EntAnimate}}**.",
    "**Edit.** Under **{{AnimRoffPlays}}** press **{{AnimRoffEdit}}** next to the file. behaved turns the frames into a few keys - the game file stays untouched, you edit a copy with its own name.":
        "**Edita.** En **{{AnimRoffPlays}}** pulsa **{{AnimRoffEdit}}** junto al archivo. behaved convierte los fotogramas en unas pocas claves; el archivo del juego no se toca, editas una copia con su propio nombre.",
    "**Change the keys.** Drag the diamonds in the view, change times in the table, add keys with **{{AnimAddKey}}**.":
        "**Cambia las claves.** Arrastra los rombos en la vista, cambia los tiempos en la tabla, añade claves con **{{AnimAddKey}}**.",
    "Editing ROFF keys.":
        "Editando claves de un ROFF.",
    "**Save.** Saving the script writes `scripts/<name>.rof`; **{{MissionSave}}** puts it into the `.pk3`.":
        "**Guarda.** Al guardar el script se escribe `scripts/<nombre>.rof`; **{{MissionSave}}** lo mete en el `.pk3`.",
    "Building a ROFF from scratch: [[tut-p5]]. The details: [[animation#roff]].":
        "Crear un ROFF desde cero: [[tut-p5]]. Los detalles: [[animation#roff]].",
    "Connect a character to a script":
        "Conectar un personaje a un script",
    "In the Graph view: drag a wire from a character to a script, and choose when it runs.":
        "En la vista Grafo: arrastra un cable de un personaje a un script y elige cuándo se ejecuta.",
    "Missions · four steps":
        "Misiones · cuatro pasos",
    "**Open the Graph.** Above the script column, switch from **{{FlowClassic}}** to **{{FlowGraph}}**. Every entity and script of the mission is a node.":
        "**Abre el Grafo.** Encima de la columna del script, cambia de **{{FlowClassic}}** a **{{FlowGraph}}**. Cada entidad y cada script de la misión es un nodo.",
    "**Drag a wire.** Drag from the dot on the right of a character node to a script node.":
        "**Arrastra un cable.** Arrastra desde el punto a la derecha de un nodo de personaje hasta un nodo de script.",
    "The Graph view.":
        "La vista Grafo.",
    "**Choose when.** A menu asks **{{GraphConnectAs}}**: `spawnscript` (when it appears), `deathscript`, `usescript` and more. behaved writes the key into the entity.":
        "**Elige cuándo.** Un menú pregunta **{{GraphConnectAs}}**: `spawnscript` (cuando aparece), `deathscript`, `usescript` y más. behaved escribe la clave en la entidad.",
    "**New script.** Drag a command from the **{{GroupEvents}}** list onto empty space in the graph: a new script with that command is made.":
        "**Script nuevo.** Arrastra un comando de la lista **{{GroupEvents}}** a un espacio vacío del grafo: se crea un script nuevo con ese comando.",
    "Continue and pack a mission":
        "Continuar y empaquetar una misión",
    "Open your mission again, check it and pack it into a .pk3 for the game.":
        "Vuelve a abrir tu misión, compruébala y empaquétala en un .pk3 para el juego.",
    "**Open it again.** **{{MenuFile}} → {{RecentMissions}}**, or the list on the start page of the **{{ViewMap}}** tab, opens map, entities and every script with one click. **{{MissionOpenFolder}}** opens one from its folder.":
        "**Vuelve a abrirla.** **{{MenuFile}} → {{RecentMissions}}**, o la lista de la página de inicio de la pestaña **{{ViewMap}}**, abre el mapa, las entidades y todos los scripts con un clic. **{{MissionOpenFolder}}** abre una desde su carpeta.",
    "**Work.** Change scripts and entities. A dot next to the mission name means unsaved changes.":
        "**Trabaja.** Cambia scripts y entidades. Un punto junto al nombre de la misión indica cambios sin guardar.",
    "**Save mission.** **{{MissionSave}}** shows every file it will pack and checks the mission: errors in red (they must be fixed), warnings in orange.":
        "**Guarda la misión.** **{{MissionSave}}** muestra cada archivo que va a empaquetar y comprueba la misión: errores en rojo (hay que corregirlos), avisos en naranja.",
    "The Save mission window.":
        "La ventana Guardar misión.",
    "Where the `.pk3` is written; **{{MissionChoose}}** picks another place.":
        "Dónde se escribe el `.pk3`; **{{MissionChoose}}** elige otro sitio.",
    "Saves, compiles and packs - or closes the window. Above them: every file of the package and the checks.":
        "Guarda, compila y empaqueta, o cierra la ventana. Encima: cada archivo del paquete y las comprobaciones.",
    "**Pack.** Press **{{MissionSaveNow}}**: scripts are saved and compiled, the `.ent` written and the `.pk3` packed. Start the game with `devmap <name>`.":
        "**Empaqueta.** Pulsa **{{MissionSaveNow}}**: los scripts se guardan y compilan, se escribe el `.ent` y se empaqueta el `.pk3`. Inicia el juego con `devmap <nombre>`.",
    "Give NPCs paths to walk":
        "Dar caminos a los NPC",
    "Create the NPC waypoint network automatically, check it, and add paths by hand.":
        "Crea automáticamente la red de waypoints de NPC, compruébala y añade caminos a mano.",
    "**Open the window.** **{{WegTitel}}** on the map toolbar. **{{WegZeigen}}** shows the network: green by hand, blue automatic, orange jumps.":
        "**Abre la ventana.** **{{WegTitel}}** en la barra del mapa. **{{WegZeigen}}** muestra la red: verde a mano, azul automática, naranja saltos.",
    "**Create automatically.** Press **{{WegErzeugen}}**: waypoints are spread over the walkable floor and connected. The spacing slider sets how dense (the original levels use about 230).":
        "**Crea automáticamente.** Pulsa **{{WegErzeugen}}**: se reparten waypoints por el suelo transitable y se conectan. El deslizador de separación fija la densidad (los niveles originales usan unos 230).",
    "The NPC waypoints window.":
        "La ventana de waypoints de NPC.",
    "Shows the network on the map.":
        "Muestra la red en el mapa.",
    "Place points by clicking the floor / connect two points.":
        "Coloca puntos pulsando en el suelo / conecta dos puntos.",
    "Spread a network over the walkable floor / remove what was created.":
        "Reparte una red por el suelo transitable / quita lo creado.",
    "Finds what the game would reject; click a problem to fly there.":
        "Encuentra lo que el juego rechazaría; pulsa un problema para volar hasta allí.",
    "**Check.** **{{WegPruefen}}** finds what the game would reject - click a problem to fly there.":
        "**Comprueba.** **{{WegPruefen}}** encuentra lo que el juego rechazaría; pulsa un problema para volar hasta allí.",
    "**By hand.** **{{WegSetzen}}**: click the floor to place a path; **{{WegVerbinden}}** connects two points (<kbd>Ctrl</kbd>: a jump).":
        "**A mano.** **{{WegSetzen}}**: pulsa en el suelo para colocar un camino; **{{WegVerbinden}}** conecta dos puntos (<kbd>Ctrl</kbd>: un salto).",
    "More in [[wegpunkte]].":
        "Más en [[wegpunkte]].",
    "Look at a character model":
        "Ver un modelo de personaje",
    "Load any character of the game, turn it, look at its skins and its skeleton.":
        "Carga cualquier personaje del juego, gíralo, mira sus skins y su esqueleto.",
    "Model · four steps":
        "Modelo · cuatro pasos",
    "**Switch to {{ViewModel}}** in the mode bar.":
        "**Cambia a {{ViewModel}}** en la barra de modos.",
    "**Pick a character.** **{{NpcBrowser}}** lists every character of the game - type to search, click to load it with its skin.":
        "**Elige un personaje.** **{{NpcBrowser}}** lista todos los personajes del juego: escribe para buscar, pulsa para cargarlo con su skin.",
    "**Look around.** Left mouse turns, middle mouse moves, the wheel zooms. **{{ModelTurntable}}** turns it by itself, **{{ModelResetView}}** brings it back to the front.":
        "**Mira alrededor.** El botón izquierdo gira, el central mueve, la rueda amplía. **{{ModelTurntable}}** lo hace girar solo, **{{ModelResetView}}** lo devuelve de frente.",
    "The model view.":
        "La vista de modelos.",
    "**Details.** **{{ModelSkin}}** switches skins, **{{ModelBones}}** shows the skeleton, **{{ModelTextures}}** switches textures off.":
        "**Detalles.** **{{ModelSkin}}** cambia de skin, **{{ModelBones}}** muestra el esqueleto, **{{ModelTextures}}** desactiva las texturas.",
    "Tutorials: scripts":
        "Tutoriales: scripts",
    "Tutorials: camera and timeline":
        "Tutoriales: cámara y línea de tiempo",
    "Tutorials: figures":
        "Tutoriales: figuras",
    "Tutorials: map and entities":
        "Tutoriales: mapa y entidades",
    "Tutorials: animation":
        "Tutoriales: animación",
    "Tutorials: missions and models":
        "Tutoriales: misiones y modelos",
    "Only the first time: variables and if":
        "Solo la primera vez: variables e if",
    "Remember that something has happened - declare a variable, set it, ask for it with if.":
        "Recuerda que algo ha pasado: declara una variable, dale un valor y pregunta por ella con if.",
    "**Declare once.** In a script that runs only once - the intro, a spawn script - insert `declare`, choose `FLOAT` and type a name, for example `seen`. A declared variable starts at 0; a map can hold 16 of them.":
        "**Declara una vez.** En un script que se ejecute solo una vez (la introducción, un script de aparición) inserta `declare`, elige `FLOAT` y escribe un nombre, por ejemplo `visto`. Una variable declarada empieza en 0; un mapa puede tener 16.",
    "**Ask with if.** In the script that may run more often (a trigger's script, see [[k-trigger]]) insert `if`. In the editor press **{{EditorHelper}}** on the first value and build `get ( FLOAT, \"seen\" )` with **{{EditorGet}}**; as comparison choose `=`, as the other value `0`.":
        "**Pregunta con if.** En el script que puede ejecutarse más veces (el script de un trigger, mira [[k-trigger]]) inserta `if`. En el editor pulsa **{{EditorHelper}}** en el primer valor y construye `get ( FLOAT, \"visto\" )` con **{{EditorGet}}**; como comparación elige `=` y como otro valor `0`.",
    "The event editor for if: get ( FLOAT, \"seen\" ), =, 0.":
        "El editor de eventos para if: get ( FLOAT, \"visto\" ), =, 0.",
    "**Do it inside.** Put the commands of the scene into the `if` block. An `else` right after it runs when the scene was already seen.":
        "**Hazlo dentro.** Mete los comandos de la escena en el bloque `if`. Un `else` justo después se ejecuta cuando la escena ya se vio.",
    "**Remember.** At the end of the `if` block insert `set` with the variable name `seen` and the value `1` - next time the `if` is false.":
        "**Recuerda.** Al final del bloque `if` inserta `set` con el nombre de variable `visto` y el valor `1`: la próxima vez el `if` es falso.",
    "The finished if / else with declare and set.":
        "El if / else terminado con declare y set.",
    "**Free it** with `free ( \"seen\" )` when it is no longer needed, so the name is available again.":
        "**Libérala** con `free ( \"visto\" )` cuando ya no haga falta, para que el nombre vuelva a estar disponible.",
    "Comparisons are `=`, `<`, `>` and `!` (not equal) - behaved warns about anything else.":
        "Las comparaciones son `=`, `<`, `>` y `!` (distinto); behaved avisa de cualquier otra.",
    "A guard walks his round":
        "Un guardia hace su ronda",
    "A loop around walk targets: the figure walks from point to point, waits, and starts again - forever.":
        "Un bucle por destinos: la figura camina de punto a punto, espera y vuelve a empezar, para siempre.",
    "**Place the round.** In the **{{ViewMap}}** tab place three or four walk targets with the tile **{{EntKindNav}}** (<kbd>Shift</kbd> places several) and name them `round1`, `round2`, `round3`.":
        "**Coloca la ronda.** En la pestaña **{{ViewMap}}** coloca tres o cuatro destinos con la ficha **{{EntKindNav}}** (<kbd>Mayús</kbd> coloca varios) y llámalos `ronda1`, `ronda2`, `ronda3`.",
    "**The guard.** Place a character (**{{EntKindNpc}}**), name it `guard`, and give it an `affect ( \"guard\", FLUSH )` block in the script. Inside, insert the macro `walkOnly` first.":
        "**El guardia.** Coloca un personaje (**{{EntKindNpc}}**), llámalo `guard` y dale un bloque `affect ( \"guard\", FLUSH )` en el script. Dentro, inserta primero la macro `walkOnly`.",
    "**The loop.** Insert `loop` and keep `-1` - forever.":
        "**El bucle.** Inserta `loop` y deja `-1`: para siempre.",
    "**One leg per point.** Inside the loop: a `task` with `set ( SET_NAVGOAL, round1 )`, then `dowait` for it, then `wait ( 2000 )`. Copy these three lines for each point and change the name. Play the timeline - the guard walks the round.":
        "**Un tramo por punto.** Dentro del bucle: una `task` con `set ( SET_NAVGOAL, ronda1 )`, luego `dowait` para ella y luego `wait ( 2000 )`. Copia estas tres líneas para cada punto y cambia el nombre. Reproduce la línea de tiempo: el guardia hace la ronda.",
    "The guard's loop: a task, dowait and wait for each point.":
        "El bucle del guardia: una task, dowait y wait por cada punto.",
    "**Run instead.** Replace `walkOnly` with `runOnly` - or right-click a walk target on the path and choose **{{BahnRennen}}**.":
        "**Que corra.** Sustituye `walkOnly` por `runOnly`, o haz clic derecho en un destino del trayecto y elige **{{BahnRennen}}**.",
    "The guard and his three walk targets on the pad.":
        "El guardia y sus tres destinos en la plataforma.",
    "The macros `patrolRun`, `patrolNoChase` and `patrolWalkNoChase` set how a patrolling guard reacts to enemies.":
        "Las macros `patrolRun`, `patrolNoChase` y `patrolWalkNoChase` fijan cómo reacciona a los enemigos un guardia que patrulla.",
    "Let the camera follow a figure":
        "Hacer que la cámara siga a una figura",
    "camera FOLLOW turns the camera after everyone of a camera group - and DISTANCE keeps it close.":
        "camera FOLLOW gira la cámara tras todos los de un grupo de cámara, y DISTANCE la mantiene cerca.",
    "**Put the figure into a group.** In the figure's `affect` block insert `set ( SET_CAMERA_GROUP, \"hero\" )`.":
        "**Mete la figura en un grupo.** En el bloque `affect` de la figura inserta `set ( SET_CAMERA_GROUP, \"heroe\" )`.",
    "**Follow.** After the camera shot insert `camera` and choose `FOLLOW`: the group `hero`, a turning speed (for example `100`) and `1` to blend into it. The camera now keeps turning towards the figure.":
        "**Sigue.** Tras el plano de cámara inserta `camera` y elige `FOLLOW`: el grupo `heroe`, una velocidad de giro (por ejemplo `100`) y `1` para fundirse con ello. La cámara se sigue girando ahora hacia la figura.",
    "**Keep a distance.** `camera ( DISTANCE, 200, 1 )` keeps the camera 200 units from the group, moving with it.":
        "**Guarda la distancia.** `camera ( DISTANCE, 200, 1 )` mantiene la cámara a 200 unidades del grupo, moviéndose con él.",
    "Through the camera: it keeps turning towards Kyle.":
        "A través de la cámara: se sigue girando hacia Kyle.",
    "`SET_CAMERA_GROUP_Z_OFS` aims higher or lower on the followed figure.":
        "`SET_CAMERA_GROUP_Z_OFS` apunta más arriba o más abajo en la figura seguida.",
    "Show a video or a line of text":
        "Mostrar un vídeo o una línea de texto",
    "SET_VIDEO_PLAY for a .roq film in the middle of a scene, print for a short text on screen.":
        "SET_VIDEO_PLAY para una película .roq en mitad de una escena, print para un texto corto en pantalla.",
    "Camera · three steps":
        "Cámara · tres pasos",
    "**A video.** Insert `set` and choose `SET_VIDEO_PLAY`; as the value type the name of a film in `video/` without its ending - Jedi Academy plays `.roq` files this way, for example its intro `jk0101_sw`.":
        "**Un vídeo.** Inserta `set` y elige `SET_VIDEO_PLAY`; como valor escribe el nombre de una película de `video/` sin su extensión. Jedi Academy reproduce así archivos `.roq`, por ejemplo su introducción `jk0101_sw`.",
    "SET_VIDEO_PLAY in the event editor.":
        "SET_VIDEO_PLAY en el editor de eventos.",
    "**Fade it.** `SET_VIDEO_FADE_IN` and `SET_VIDEO_FADE_OUT` (`true`) set before the video let it fade.":
        "**Fundido.** `SET_VIDEO_FADE_IN` y `SET_VIDEO_FADE_OUT` (`true`) puestos antes del vídeo lo hacen fundirse.",
    "**Text.** `print ( \"Three days later...\" )` shows a line in the middle of the screen.":
        "**Texto.** `print ( \"Tres días después...\" )` muestra una línea en el centro de la pantalla.",
    "Video, fade and a line of text in the tree.":
        "Vídeo, fundido y una línea de texto en el árbol.",
    "behaved does not play videos in the timeline; it shows the command as a block so the timing stays visible.":
        "behaved no reproduce vídeos en la línea de tiempo; muestra el comando como un bloque para que los tiempos sigan visibles.",
    "Start a fight after the cutscene":
        "Empezar un combate tras la cinemática",
    "Keep an enemy calm while the camera runs, then let him attack the player.":
        "Mantén tranquilo a un enemigo mientras corre la cámara y luego deja que ataque al jugador.",
    "**Calm at first.** Give the enemy a spawn script (key `spawnscript`, **{{EntOpenScript}}** creates it) with the macro `standOnly` - he stands and ignores enemies.":
        "**Tranquilo al principio.** Dale al enemigo un script de aparición (clave `spawnscript`, **{{EntOpenScript}}** lo crea) con la macro `standOnly`: se queda quieto e ignora a los enemigos.",
    "**The scene.** The cutscene runs as usual; the enemy can walk and talk like any figure.":
        "**La escena.** La cinemática se desarrolla como siempre; el enemigo puede caminar y hablar como cualquier figura.",
    "**End the camera.** At the end of the intro: `camera ( DISABLE )` - the player plays again.":
        "**Termina la cámara.** Al final de la introducción: `camera ( DISABLE )`, el jugador vuelve a jugar.",
    "**Attack.** Right after it, an `affect ( \"enemy\", FLUSH )` block with `set ( SET_IGNOREENEMIES, false )`, `set ( SET_LOOK_FOR_ENEMIES, true )` and `set ( SET_ENEMY, \"player\" )`. The macro `default` resets all behaviour in one line.":
        "**Ataque.** Justo después, un bloque `affect ( \"enemy\", FLUSH )` con `set ( SET_IGNOREENEMIES, false )`, `set ( SET_LOOK_FOR_ENEMIES, true )` y `set ( SET_ENEMY, \"player\" )`. La macro `default` restablece todo el comportamiento en una línea.",
    "After camera ( DISABLE ): the enemy turns on the player.":
        "Tras camera ( DISABLE ): el enemigo se vuelve contra el jugador.",
    "**More than one.** Copy the block for each enemy, or put the lines into a script of their own and start it with `run`.":
        "**Más de uno.** Copia el bloque para cada enemigo, o pon las líneas en un script propio e inícialo con `run`.",
    "`SET_BEHAVIOR_STATE` with `BS_DEFAULT` gives the figure back to its normal AI after scripted walking.":
        "`SET_BEHAVIOR_STATE` con `BS_DEFAULT` devuelve la figura a su IA normal tras una marcha con script.",
    "Hold the player during a talk":
        "Retener al jugador durante una conversación",
    "Lock the player in place - without a cutscene camera - and free him again.":
        "Bloquea al jugador en su sitio, sin cámara de cinemática, y vuelve a liberarlo.",
    "Figures · three steps":
        "Figuras · tres pasos",
    "**Lock.** Insert `affect ( \"player\", FLUSH )` and inside it `set ( SET_PLAYER_LOCKED, true )`. `SET_LOCK_PLAYER_WEAPONS` keeps the weapon as well.":
        "**Bloquea.** Inserta `affect ( \"player\", FLUSH )` y dentro `set ( SET_PLAYER_LOCKED, true )`. `SET_LOCK_PLAYER_WEAPONS` retiene también el arma.",
    "**Talk.** The other figures speak and move as usual; the player watches from his own view.":
        "**Conversación.** Las demás figuras hablan y se mueven como siempre; el jugador mira desde su propia vista.",
    "**Free.** At the end the same block with `false` - otherwise the player stays frozen.":
        "**Libera.** Al final, el mismo bloque con `false`; si no, el jugador se queda congelado.",
    "Lock and free the player around a talk.":
        "Bloquear y liberar al jugador alrededor de una conversación.",
    "With `camera ( ENABLE )` the game takes the controls away by itself; the lock is for moments without a cutscene camera.":
        "Con `camera ( ENABLE )` el juego quita los controles él solo; el bloqueo es para momentos sin cámara de cinemática.",
    "Ignite a lightsaber in a scene":
        "Encender un sable láser en una escena",
    "Give a figure its saber and switch the blade on and off at the right moment.":
        "Dale a una figura su sable y enciende y apaga la hoja en el momento justo.",
    "**The weapon.** In the figure's `affect` block (or its spawn script): `set ( SET_WEAPON, WP_SABER )`.":
        "**El arma.** En el bloque `affect` de la figura (o en su script de aparición): `set ( SET_WEAPON, WP_SABER )`.",
    "**On.** Where it should ignite: `set ( SET_SABERACTIVE, true )`. In the timeline the blade appears at that moment - with its sound when **{{TlAudio}}** is on and the camera can see the figure, as in the game.":
        "**Encendido.** Donde debe encenderse: `set ( SET_SABERACTIVE, true )`. En la línea de tiempo la hoja aparece en ese momento, con su sonido si **{{TlAudio}}** está activado y la cámara ve a la figura, como en el juego.",
    "Kyle with his blade ignited, in the view.":
        "Kyle con la hoja encendida, en la vista.",
    "**Off.** `set ( SET_SABERACTIVE, false )`.":
        "**Apagado.** `set ( SET_SABERACTIVE, false )`.",
    "The saber commands in Kyle's affect block.":
        "Los comandos del sable en el bloque affect de Kyle.",
    "`SET_SABER1` gives the figure another saber by its name from the game's saber files.":
        "`SET_SABER1` da a la figura otro sable por su nombre, de los archivos de sables del juego.",
    "**Walls.** A blade that touches a wall is shortened there, with sparks, sound and a burn mark - [[figuren#schwerter]].":
        "**Paredes.** Una hoja que toca una pared se acorta ahí, con chispas, sonido y una marca de quemadura: [[figuren#schwerter]].",
    "**Clash.** For two blades to clash in Movie Duels, put both NPCs in `BS_CINEMATIC` and add `set ( SET_SABERCOLLISIONS, true )` before the attack animation. Under OpenJK rules only the animations `BOTH_CIN_16` and `BOTH_CIN_17` clash.":
        "**Choque.** Para que dos hojas choquen en Movie Duels, pon los dos NPC en `BS_CINEMATIC` y añade `set ( SET_SABERCOLLISIONS, true )` antes de la animación de ataque. Con las reglas de OpenJK solo chocan las animaciones `BOTH_CIN_16` y `BOTH_CIN_17`.",
    "Start a scene when the player walks somewhere":
        "Empezar una escena cuando el jugador llega a un sitio",
    "Point a trigger of the map at a script runner of your own - the scene starts when the player enters the area.":
        "Apunta un trigger del mapa a un ejecutor de scripts propio: la escena empieza cuando el jugador entra en la zona.",
    "**A runner.** In the **{{ViewMap}}** tab click the tile **{{EntKindRunner}}** and click near the place. behaved names it and sets its `usescript` to a script in your mission folder.":
        "**Un ejecutor.** En la pestaña **{{ViewMap}}** pulsa la ficha **{{EntKindRunner}}** y pulsa cerca del sitio. behaved le pone nombre y fija su `usescript` a un script de la carpeta de tu misión.",
    "**Its script.** In the inspector press **{{EntOpenScript}}** next to `usescript`: the script is created and opened. Write the scene in it.":
        "**Su script.** En el inspector pulsa **{{EntOpenScript}}** junto a `usescript`: el script se crea y se abre. Escribe en él la escena.",
    "**Find a trigger.** Triggers are brushes of the map: type `trigger` into the search box of the entity list and pick one where the player passes. Fly there with a double-click.":
        "**Busca un trigger.** Los triggers son brushes del mapa: escribe `trigger` en el campo de búsqueda de la lista de entidades y elige uno por donde pase el jugador. Vuela hasta allí con un doble clic.",
    "**Point it at the runner.** In the trigger's inspector set its key `target` to the runner's name. The **{{FlowGraph}}** view now shows the arrow from trigger to runner to script.":
        "**Apúntalo al ejecutor.** En el inspector del trigger, pon su clave `target` al nombre del ejecutor. La vista **{{FlowGraph}}** muestra ahora la flecha del trigger al ejecutor y al script.",
    "The trigger's keys: target now names the runner.":
        "Las claves del trigger: target nombra ahora al ejecutor.",
    "**Once or every time.** A runner runs once; its key `count` set to `-1` runs it every time. For only-the-first-time logic see [[k-variablen]].":
        "**Una vez o siempre.** Un ejecutor se ejecuta una vez; con su clave `count` a `-1` se ejecuta siempre. Para la lógica de solo la primera vez, mira [[k-variablen]].",
    "New trigger areas need brushes, which are made in a map editor - behaved can change every trigger of a map, but not draw a new one. A script can also start a runner directly with `use`.":
        "Las zonas de trigger nuevas necesitan brushes, que se hacen en un editor de mapas: behaved puede cambiar cualquier trigger de un mapa, pero no dibujar uno nuevo. Un script también puede iniciar un ejecutor directamente con `use`.",
    "Open a door from the script":
        "Abrir una puerta desde el script",
    "use opens a door, fires a trigger or starts a runner - by its name.":
        "use abre una puerta, dispara un trigger o inicia un ejecutor, por su nombre.",
    "**Find the door.** Doors are in the group **Movable (func_*)** of the entity list. Click one to see it in the inspector.":
        "**Busca la puerta.** Las puertas están en el grupo **Móviles (func_*)** de la lista de entidades. Pulsa una para verla en el inspector.",
    "**Its name.** The door needs a `targetname`. A door with a name waits to be used instead of opening when someone walks up - which is what a scripted door should do.":
        "**Su nombre.** La puerta necesita un `targetname`. Una puerta con nombre espera a ser usada en vez de abrirse cuando alguien se acerca, que es lo que debe hacer una puerta con script.",
    "**Insert use.** Select the line in the script after which the door should open, then right-click the door in the entity list → **{{EntityUse}}**: `use ( \"door1\" )` is inserted.":
        "**Inserta use.** Selecciona la línea del script tras la que debe abrirse la puerta y luego haz clic derecho en la puerta en la lista de entidades → **{{EntityUse}}**: se inserta `use ( \"door1\" )`.",
    "Right-click a door in the entity list: Insert use.":
        "Clic derecho en una puerta en la lista de entidades: Insertar use.",
    "**Hear it.** The door plays its own sounds (its `soundSet`). Play the timeline to check the moment.":
        "**Óyela.** La puerta reproduce sus propios sonidos (su `soundSet`). Reproduce la línea de tiempo para comprobar el momento.",
    "Moving the door yourself instead - other distance, other speed: [[k-tuer]].":
        "Mover tú mismo la puerta, con otra distancia u otra velocidad: [[k-tuer]].",
    "Go to the next map at the end":
        "Ir al mapa siguiente al final",
    "A level change entity, used by the last line of the script.":
        "Una entidad de cambio de nivel, usada por la última línea del script.",
    "**Place it.** **{{EntNew}}** → **{{EntKindLevel}}**. In the inspector set `mapname` to the next map (for example `taspir2`) and give it a **{{EntName}}** such as `next_map`.":
        "**Colócala.** **{{EntNew}}** → **{{EntKindLevel}}**. En el inspector pon `mapname` al mapa siguiente (por ejemplo `taspir2`) y dale un **{{EntName}}** como `siguiente_mapa`.",
    "The level change with mapname and a name.":
        "El cambio de nivel con mapname y un nombre.",
    "**Use it.** At the end of the script: `use ( \"next_map\" )` (right-click the entity → **{{EntityUse}}**).":
        "**Úsala.** Al final del script: `use ( \"siguiente_mapa\" )` (clic derecho en la entidad → **{{EntityUse}}**).",
    "**Check it.** **{{MissionSave}}** warns when a target points to nothing; play the mission to see the loading screen.":
        "**Compruébala.** **{{MissionSave}}** avisa cuando un target no apunta a nada; juega la misión para ver la pantalla de carga.",
    "Add an effect and change the music":
        "Añadir un efecto y cambiar la música",
    "An fx_runner that starts on cue, and a music change from the script.":
        "Un fx_runner que empieza a la señal, y un cambio de música desde el script.",
    "**An effect.** Click the tile **{{EntKindFx}}** and click where it should appear. In the inspector set `fxFile` to an effect of the game (a path under `effects/`, for example `env/fire`), give it a name, and tick `STARTOFF` so it waits.":
        "**Un efecto.** Pulsa la ficha **{{EntKindFx}}** y pulsa donde debe aparecer. En el inspector pon `fxFile` a un efecto del juego (una ruta bajo `effects/`, por ejemplo `env/fire`), dale un nombre y marca `STARTOFF` para que espere.",
    "**Start it.** In the script: `use ( \"fire1\" )` at the right moment. **{{MapEffects}}** on the toolbar shows effects in the view.":
        "**Inícialo.** En el script: `use ( \"fuego1\" )` en el momento justo. **{{MapEffects}}** en la barra muestra los efectos en la vista.",
    "A fire effect on the landing pad.":
        "Un efecto de fuego en la plataforma de aterrizaje.",
    "**Music.** **{{EntNew}}** → **{{EntKindMusic}}**, set `music` to a music file and a name; `use` it from the script to switch the music.":
        "**Música.** **{{EntNew}}** → **{{EntKindMusic}}**, pon `music` a un archivo de música y un nombre; úsala con `use` desde el script para cambiar la música.",
    "**Dynamic music.** `set ( SET_MUSIC_STATE, ... )` switches the game's own music between its states, for example to action.":
        "**Música dinámica.** `set ( SET_MUSIC_STATE, ... )` cambia la propia música del juego entre sus estados, por ejemplo a acción.",
    "Change a cutscene of the game":
        "Cambiar una cinemática del juego",
    "Open a mission of Jedi Academy, change a camera shot of its intro, and pack it as your own version.":
        "Abre una misión de Jedi Academy, cambia un plano de cámara de su introducción y empaquétala como tu propia versión.",
    "Missions · five steps":
        "Misiones · cinco pasos",
    "**Open it.** **{{MenuFile}} → {{LoadMission}}**, choose `assets0.pk3` in the game's `base` folder and pick a map, for example `taspir1`. The map and all its scripts open in tabs.":
        "**Ábrela.** **{{MenuFile}} → {{LoadMission}}**, elige `assets0.pk3` en la carpeta `base` del juego y escoge un mapa, por ejemplo `taspir1`. El mapa y todos sus scripts se abren en pestañas.",
    "The missions of an archive.":
        "Las misiones de un archivo .pk3.",
    "**Find the scene.** In the **{{ViewMap}}** tab, **Scripts (N)** lists the scripts the map uses - pick the intro. The timeline shows its camera.":
        "**Busca la escena.** En la pestaña **{{ViewMap}}**, **Scripts (N)** lista los scripts que usa el mapa: elige la introducción. La línea de tiempo muestra su cámara.",
    "**Change a shot.** Select a shot in the **{{CamShots}}** list, fly to a better view and press **{{CamSetToView}}** - or change a time in the timeline.":
        "**Cambia un plano.** Selecciona un plano en la lista **{{CamShots}}**, vuela a una vista mejor y pulsa **{{CamSetToView}}**, o cambia un tiempo en la línea de tiempo.",
    "**Pack it.** **{{MissionSave}}** writes the changed scripts with the map into `zz_<map>.pk3`. Archives load in alphabetical order, so `zz_` wins over the game's own files.":
        "**Empaquétala.** **{{MissionSave}}** escribe los scripts cambiados con el mapa en `zz_<mapa>.pk3`. Los archivos .pk3 se cargan en orden alfabético, así que `zz_` gana a los archivos propios del juego.",
    "**Undo it.** Delete the `zz_` file from the game folder and the game plays its original again - the game files are never touched.":
        "**Deshazlo.** Borra el archivo `zz_` de la carpeta del juego y el juego vuelve a reproducir el original: los archivos del juego nunca se tocan.",
    "Check a script for multiplayer":
        "Comprobar un script para multijugador",
    "Switch the target game and read what works differently on a multiplayer server.":
        "Cambia el juego de destino y lee lo que funciona de otra manera en un servidor multijugador.",
    "Missions · three steps":
        "Misiones · tres pasos",
    "**Switch.** **{{MenuView}} → {{ViewTarget}}** → multiplayer. An **MP** badge appears in the status line.":
        "**Cambia.** **{{MenuView}} → {{ViewTarget}}** → multijugador. Aparece una insignia **MP** en la línea de estado.",
    "**Read the notes.** Click the notes counter in the status line: the **{{MessagesTitle}}** window lists every command that does nothing or works differently on a server - checked against the multiplayer game code.":
        "**Lee las notas.** Pulsa el contador de notas de la línea de estado: la ventana **{{MessagesTitle}}** lista cada comando que no hace nada o funciona de otra manera en un servidor, comprobado con el código del juego multijugador.",
    "The Messages window with the multiplayer notes.":
        "La ventana de mensajes con las notas de multijugador.",
    "**Switch back** to single player when you are done; the script itself is not changed.":
        "**Vuelve** a un jugador cuando termines; el script en sí no cambia.",
    "Set up the program your way":
        "Configurar el programa a tu manera",
    "Shortcuts, theme, language, interface size and the column layout.":
        "Atajos, tema, idioma, tamaño de la interfaz y la disposición de columnas.",
    "Settings · five steps":
        "Ajustes · cinco pasos",
    "**Shortcuts.** **{{MenuEdit}} → {{KeysTitle}}**: click a shortcut, press the new key. <kbd>Esc</kbd> cancels, <kbd>Backspace</kbd> clears; a clash shows which action already has the key.":
        "**Atajos.** **{{MenuEdit}} → {{KeysTitle}}**: pulsa un atajo y luego la tecla nueva. <kbd>Esc</kbd> cancela, <kbd>Retroceso</kbd> lo borra; un conflicto muestra qué acción tiene ya la tecla.",
    "The Keyboard shortcuts window.":
        "La ventana de atajos de teclado.",
    "**Theme and language.** **{{MenuView}} → {{ViewTheme}}** (six themes, among them BehavEd Classic and High Contrast) and **{{ViewLanguage}}**.":
        "**Tema e idioma.** **{{MenuView}} → {{ViewTheme}}** (seis temas, entre ellos BehavEd clásico y Alto contraste) y **{{ViewLanguage}}**.",
    "**Size.** **{{MenuView}} → {{ViewDpiScale}}**, or <kbd>Ctrl</kbd>+mouse wheel anywhere outside the timeline.":
        "**Tamaño.** **{{MenuView}} → {{ViewDpiScale}}**, o <kbd>Ctrl</kbd>+rueda del ratón en cualquier sitio fuera de la línea de tiempo.",
    "**Columns.** Drag the dividers between the columns; each mode keeps its own widths. **{{MenuDebug}} → Restore default layout** brings them back.":
        "**Columnas.** Arrastra los separadores entre las columnas; cada modo guarda sus propios anchos. **{{MenuDebug}} → Restaurar la disposición predeterminada** los devuelve.",
    "**The guide.** <kbd>F1</kbd> opens this guide - also rebindable.":
        "**La guía.** <kbd>F1</kbd> abre esta guía; también se puede reasignar.",
    "All settings: [[einstellungen]]. All keys: [[tasten]].":
        "Todos los ajustes: [[einstellungen]]. Todas las teclas: [[tasten]].",
    "When the enemy dies, the door opens":
        "Cuando muere el enemigo, la puerta se abre",
    "A deathscript on a figure: the moment it falls, a script of yours runs - here it opens a door.":
        "Un deathscript en una figura: en el momento en que cae se ejecuta un script tuyo; aquí abre una puerta.",
    "**Give the figure a deathscript.** Select the enemy in the **{{ViewMap}}** tab. In the inspector add the key `deathscript` with a script name of your mission, for example `taspir_tut/rosh_dies` - or, in the **{{FlowGraph}}** view, drag a wire from the figure to a script and choose `deathscript` under **{{GraphConnectAs}}**.":
        "**Dale a la figura un deathscript.** Selecciona al enemigo en la pestaña **{{ViewMap}}**. En el inspector añade la clave `deathscript` con un nombre de script de tu misión, por ejemplo `taspir_tut/rosh_muere`, o, en la vista **{{FlowGraph}}**, arrastra un cable de la figura a un script y elige `deathscript` en **{{GraphConnectAs}}**.",
    "The enemy's deathscript in the inspector.":
        "El deathscript del enemigo en el inspector.",
    "**Create the script.** Press **{{EntOpenScript}}** next to the key: the script is created in the mission folder and opened.":
        "**Crea el script.** Pulsa **{{EntOpenScript}}** junto a la clave: el script se crea en la carpeta de la misión y se abre.",
    "**Open the door.** In it, insert `use` with the door's name - right-click the door in the entity list → **{{EntityUse}}** does it for you. A `wait` before it lets the moment breathe.":
        "**Abre la puerta.** Dentro, inserta `use` con el nombre de la puerta; clic derecho en la puerta en la lista de entidades → **{{EntityUse}}** lo hace por ti. Un `wait` antes deja respirar el momento.",
    "The deathscript: a short wait, then the door.":
        "El deathscript: una espera corta y luego la puerta.",
    "**Other moments.** The same works with `painscript` (hit), `awakescript` (notices the player), `angerscript` (starts fighting), `fleescript` and `spawnscript`.":
        "**Otros momentos.** Lo mismo funciona con `painscript` (golpeado), `awakescript` (ve al jugador), `angerscript` (empieza a luchar), `fleescript` y `spawnscript`.",
    "The player talks to a figure":
        "El jugador habla con una figura",
    "usescript and SET_PLAYER_USABLE: pressing the use key on a figure starts its script - a conversation.":
        "usescript y SET_PLAYER_USABLE: pulsar la tecla de usar sobre una figura inicia su script, una conversación.",
    "**Make it usable.** In the figure's spawn script insert `set ( SET_PLAYER_USABLE, true )` - otherwise the use key does nothing.":
        "**Hazla usable.** En el script de aparición de la figura inserta `set ( SET_PLAYER_USABLE, true )`; si no, la tecla de usar no hace nada.",
    "**The script for the talk.** Add the key `usescript` to the figure (or connect it in the **{{FlowGraph}}** view as `usescript`) and press **{{EntOpenScript}}**.":
        "**El script de la conversación.** Añade la clave `usescript` a la figura (o conéctala en la vista **{{FlowGraph}}** como `usescript`) y pulsa **{{EntOpenScript}}**.",
    "**Write the talk.** Sounds on `CHAN_VOICE`, a `SET_LOOK_TARGET` at the player, perhaps a short camera. The figure says its lines every time the player presses use.":
        "**Escribe la conversación.** Sonidos en `CHAN_VOICE`, un `SET_LOOK_TARGET` al jugador, quizá una cámara corta. La figura dice sus frases cada vez que el jugador pulsa usar.",
    "The spawn script makes the figure usable.":
        "El script de aparición hace usable a la figura.",
    "**Only once?** Set `SET_PLAYER_USABLE` to `false` at the end of the usescript - or use a variable ([[k-variablen]]).":
        "**¿Solo una vez?** Pon `SET_PLAYER_USABLE` a `false` al final del usescript, o usa una variable ([[k-variablen]]).",
    "Let several scripts work together":
        "Hacer que varios scripts trabajen juntos",
    "run starts a script, signal and waitsignal hand over - and the split view shows both at once.":
        "run inicia un script, signal y waitsignal se pasan el testigo, y la vista dividida muestra ambos a la vez.",
    "**Split the work.** Long scenes are easier in parts: the intro starts the next part with `run ( \"taspir_tut/talk\" )` - the name as under `scripts/`, without ending.":
        "**Divide el trabajo.** Las escenas largas son más fáciles por partes: la introducción inicia la parte siguiente con `run ( \"taspir_tut/charla\" )`, con el nombre como bajo `scripts/`, sin extensión.",
    "**Hand over.** Where the intro must wait for the other part, insert `waitsignal ( \"talk_done\" )`; at the end of the other script `signal ( \"talk_done\" )`.":
        "**Pasa el testigo.** Donde la introducción debe esperar a la otra parte, inserta `waitsignal ( \"charla_hecha\" )`; al final del otro script, `signal ( \"charla_hecha\" )`.",
    "**See both.** **{{MenuFile}} → {{SplitToggle}} → 2** shows the two scripts side by side; drag lines across to copy them.":
        "**Mira los dos.** **{{MenuFile}} → {{SplitToggle}} → 2** muestra los dos scripts uno al lado del otro; arrastra líneas de uno a otro para copiarlas.",
    "Two scripts that hand over with signal and waitsignal.":
        "Dos scripts que se pasan el testigo con signal y waitsignal.",
    "**Keep the overview.** The **{{FlowGraph}}** view draws `run` connections between the scripts of the mission.":
        "**Mantén la visión de conjunto.** La vista **{{FlowGraph}}** dibuja las conexiones `run` entre los scripts de la misión.",
    "Fix what Save mission reports":
        "Corregir lo que informa Guardar misión",
    "The checks of Save mission - what each message means, which ones stop the packing, and how to fix them.":
        "Las comprobaciones de Guardar misión: qué significa cada mensaje, cuáles detienen el empaquetado y cómo corregirlos.",
    "Missions · six steps":
        "Misiones · seis pasos",
    "**Open the check.** **{{MissionSave}}** lists every file it will pack and, below, what it found. Errors in red stop the packing until they are fixed; warnings in orange let you pack anyway - but read them.":
        "**Abre la comprobación.** **{{MissionSave}}** lista cada archivo que va a empaquetar y, debajo, lo que encontró. Los errores en rojo detienen el empaquetado hasta corregirlos; los avisos en naranja dejan empaquetar igualmente, pero léelos.",
    "Save mission with two missing scripts and a target of the original map.":
        "Guardar misión con dos scripts que faltan y un target del mapa original.",
    "**S001 (warning) - a script is missing.** An entity names a script that does not exist yet; in the game nothing would happen. Select the entity and press **{{EntOpenScript}}** next to the key: the script is created.":
        "**S001 (aviso): falta un script.** Una entidad nombra un script que aún no existe; en el juego no pasaría nada. Selecciona la entidad y pulsa **{{EntOpenScript}}** junto a la clave: se crea el script.",
    "**S002 (error) - a script does not compile.** Open it and press **{{Compile}}**: the wrong line is selected ([[k-uebersetzen]]).":
        "**S002 (error): un script no compila.** Ábrelo y pulsa **{{Compile}}**: la línea errónea queda seleccionada ([[k-uebersetzen]]).",
    "**S003 (warning) - no player start.** The mission cannot start. Place one with the tile **{{EntKindStart}}**.":
        "**S003 (aviso): no hay inicio de jugador.** La misión no puede empezar. Coloca uno con la ficha **{{EntKindStart}}**.",
    "**S005 (warning) - a target points to nothing.** A `target` names an entity that does not exist - often a leftover of the original map, like the one in the picture.":
        "**S005 (aviso): un target no apunta a nada.** Un `target` nombra una entidad que no existe; a menudo es un resto del mapa original, como el de la imagen.",
    "**S004 and S006.** A missing `ENDMARKER` is added for you (warning); an invalid mission name is an error - letters, digits and `_` only.":
        "**S004 y S006.** Un `ENDMARKER` que falta se añade por ti (aviso); un nombre de misión no válido es un error: solo letras, cifras y `_`.",
    "Open a compiled script of the game":
        "Abrir un script compilado del juego",
    "The game only ships .ibi files. behaved turns them back into a script you can read and change.":
        "El juego solo trae archivos .ibi. behaved los vuelve a convertir en un script que puedes leer y cambiar.",
    "**Find it.** **{{MenuFile}} → {{Pk3Browse}}**, choose **{{Pk3Scripts}}** and type part of the name into **{{Pk3Filter}}**, for example `taspir1/intro`.":
        "**Búscalo.** **{{MenuFile}} → {{Pk3Browse}}**, elige **{{Pk3Scripts}}** y escribe parte del nombre en **{{Pk3Filter}}**, por ejemplo `taspir1/intro`.",
    "The files in the game's archives.":
        "Los archivos de los .pk3 del juego.",
    "**Open it.** Double-click the entry: the `.ibi` opens as a script in the current tab - commands, values and blocks, exactly as Raven wrote them. Open an empty tab first (**+**) if you want to keep the script you are working on; with unsaved changes behaved asks before.":
        "**Ábrelo.** Doble clic en la entrada: el `.ibi` se abre como script en la pestaña actual (comandos, valores y bloques, exactamente como los escribió Raven). Abre antes una pestaña vacía (**+**) si quieres conservar el script en el que trabajas; con cambios sin guardar, behaved pregunta antes.",
    "A compiled script of the game, opened as text.":
        "Un script compilado del juego, abierto como texto.",
    "**From disk.** **{{MenuFile}} → {{FileOpen}}** opens `.ibi` files from a folder the same way.":
        "**Desde el disco.** **{{MenuFile}} → {{FileOpen}}** abre del mismo modo archivos `.ibi` de una carpeta.",
    "**Change it.** A script from an archive has no file yet - the first save asks where to put it. To ship a changed version of a game scene, see [[k-szene-aendern]].":
        "**Cámbialo.** Un script de un .pk3 aún no tiene archivo: el primer guardado pregunta dónde ponerlo. Para distribuir una versión cambiada de una escena del juego, mira [[k-szene-aendern]].",
    "Find files in the game archives":
        "Buscar archivos en los .pk3 del juego",
    "Scripts, maps and models of the game and every mod, searchable in one window.":
        "Scripts, mapas y modelos del juego y de cada mod, en una ventana con búsqueda.",
    "**Open the browser.** **{{MenuFile}} → {{Pk3Browse}}**. It lists the files of every game folder you added.":
        "**Abre el navegador.** **{{MenuFile}} → {{Pk3Browse}}**. Lista los archivos de cada carpeta del juego que hayas añadido.",
    "The Files in .pk3 window.":
        "La ventana Archivos en .pk3.",
    "**Choose the kind.** **{{Pk3Scripts}}**, **{{Pk3Maps}}** or **{{Pk3Models}}**. The **{{Pk3Archive}}** list narrows it to one `.pk3`.":
        "**Elige el tipo.** **{{Pk3Scripts}}**, **{{Pk3Maps}}** o **{{Pk3Models}}**. La lista **{{Pk3Archive}}** lo limita a un `.pk3`.",
    "**Search.** Type into **{{Pk3Filter}}** - part of a name is enough.":
        "**Busca.** Escribe en **{{Pk3Filter}}**; basta con parte de un nombre.",
    "**Open.** Double-click: a script replaces the one in the current tab (behaved asks first if it has unsaved changes), a map loads, a model opens with its skin in the **{{ViewModel}}** tab.":
        "**Abre.** Doble clic: un script sustituye al de la pestaña actual (behaved pregunta antes si tiene cambios sin guardar), un mapa se carga, un modelo se abre con su skin en la pestaña **{{ViewModel}}**.",
    "See what you have changed":
        "Ver lo que has cambiado",
    "The change margin, going back to the original of one command, and finding identical commands.":
        "El margen de cambios, volver al original de un comando y encontrar comandos idénticos.",
    "**The margin.** Change a value: a coloured bar appears left of the line - orange for changed and not saved, green for changed and saved, blue for back to the original. **{{MenuView}} → {{ViewChangeHistory}}** switches it on or off.":
        "**El margen.** Cambia un valor: aparece una barra de color a la izquierda de la línea; naranja para cambiado y sin guardar, verde para cambiado y guardado, azul para de nuevo como el original. **{{MenuView}} → {{ViewChangeHistory}}** lo activa o desactiva.",
    "Changed lines in the margin.":
        "Líneas cambiadas en el margen.",
    "**What was it before?** Hover over the bar: the original line is shown.":
        "**¿Qué había antes?** Pasa el ratón por encima de la barra: se muestra la línea original.",
    "**Back to the original.** Right-click the line → **{{ActRevertOriginal}}**: only this command goes back, everything else stays.":
        "**Vuelve al original.** Clic derecho en la línea → **{{ActRevertOriginal}}**: solo ese comando vuelve atrás, todo lo demás se queda.",
    "**Identical commands.** **{{MenuView}} → {{ViewHighlightSame}}** marks every command that looks exactly like the selected one - handy for a camera that is used several times.":
        "**Comandos idénticos.** **{{MenuView}} → {{ViewHighlightSame}}** marca cada comando que es exactamente igual al seleccionado: práctico para una cámara que se usa varias veces.",
    "Get an old version back":
        "Recuperar una versión antigua",
    "Automatic copies on every save, your own backup, and restoring it.":
        "Copias automáticas en cada guardado, tu propia copia de seguridad y cómo restaurarla.",
    "**Automatic copies.** Every save and every compile keeps a copy - the last ten, `backup1.txt` (newest) to `backup10.txt`. **{{MenuFile}} → {{FileBackupFolder}}** opens the folder.":
        "**Copias automáticas.** Cada guardado y cada compilación conservan una copia: las diez últimas, de `backup1.txt` (la más reciente) a `backup10.txt`. **{{MenuFile}} → {{FileBackupFolder}}** abre la carpeta.",
    "The File menu.":
        "El menú Archivo.",
    "**Your own backup.** **{{FileBackup}}** (<kbd>Alt</kbd>+<kbd>B</kbd>) writes `<script>.bak` next to the script - before a big change.":
        "**Tu propia copia.** **{{FileBackup}}** (<kbd>Alt</kbd>+<kbd>B</kbd>) escribe `<script>.bak` junto al script, antes de un cambio grande.",
    "**Restore it.** **{{FileRestore}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) goes back to the `.bak`, after a question.":
        "**Restáurala.** **{{FileRestore}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) vuelve al `.bak`, tras una pregunta.",
    "**Smaller steps.** For the last few changes the undo history is quicker: [[k-rueckgaengig]].":
        "**Pasos más pequeños.** Para los últimos cambios es más rápido el historial de deshacer: [[k-rueckgaengig]].",
    "Understand the warnings":
        "Entender los avisos",
    "The Messages window: values that are not in their list, unknown fields, notes while reading a file.":
        "La ventana de mensajes: valores que no están en su lista, campos desconocidos, notas al leer un archivo.",
    "**The counters.** The status line counts errors and notes. Click a counter to open the **{{MessagesTitle}}** window.":
        "**Los contadores.** La línea de estado cuenta errores y notas. Pulsa un contador para abrir la ventana **{{MessagesTitle}}**.",
    "**Read them.** Each message names its code and the command. A misspelt value such as `SET_WALKNG` is reported as *not in SET_TYPES (kept unchanged)* - behaved keeps it, because mods bring their own values, but it is far more often a typo.":
        "**Léelos.** Cada mensaje nombra su código y el comando. Un valor mal escrito como `SET_WALKNG` se informa como *no está en SET_TYPES (se deja sin cambiar)*: behaved lo conserva porque los mods traen sus propios valores, pero casi siempre es una errata.",
    "V004: a value that is not in its list.":
        "V004: un valor que no está en su lista.",
    "**Fix and check.** Correct the value in the editor - the message disappears. Messages never change the script by themselves.":
        "**Corrige y comprueba.** Corrige el valor en el editor y el mensaje desaparece. Los mensajes nunca cambian el script por sí solos.",
    "Move many entities at once":
        "Mover muchas entidades a la vez",
    "Select with a frame, move and turn the group, copy it.":
        "Selecciona con un marco, mueve y gira el grupo, cópialo.",
    "**Draw a frame.** In the **{{ViewMap}}** tab drag over empty space in the view: every entity inside is selected. <kbd>Ctrl</kbd> or <kbd>Shift</kbd> adds to it.":
        "**Dibuja un marco.** En la pestaña **{{ViewMap}}** arrastra sobre un espacio vacío de la vista: se selecciona cada entidad que quede dentro. <kbd>Ctrl</kbd> o <kbd>Mayús</kbd> añade a la selección.",
    "Several entities selected with a frame.":
        "Varias entidades seleccionadas con un marco.",
    "**Move the group.** Drag one of them, or use the arrows: all move together.":
        "**Mueve el grupo.** Arrastra una de ellas, o usa las flechas: se mueven todas juntas.",
    "**Turn the group.** Switch the gizmo bar to **{{GizmoRotate}}** and drag the ring.":
        "**Gira el grupo.** Cambia la barra del gizmo a **{{GizmoRotate}}** y arrastra el anillo.",
    "**Copy it.** <kbd>Alt</kbd>+drag copies the whole group - **{{EntDuplicate}}** in the inspector does the same.":
        "**Cópialo.** <kbd>Alt</kbd>+arrastrar copia el grupo entero; **{{EntDuplicate}}** en el inspector hace lo mismo.",
    "Remember viewpoints and camera paths":
        "Recordar puntos de vista y trayectos de cámara",
    "Number keys for views you need again - and only the camera path you are working on.":
        "Teclas numéricas para las vistas que vuelves a necesitar, y solo el trayecto de cámara en el que trabajas.",
    "**Store a view.** With the mouse over the map press <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd>; the status line confirms it. <kbd>1</kbd> … <kbd>9</kbd> jumps back. The views are kept per map.":
        "**Guarda una vista.** Con el ratón sobre el mapa pulsa <kbd>Ctrl</kbd>+<kbd>1</kbd> … <kbd>9</kbd>; la línea de estado lo confirma. <kbd>1</kbd> … <kbd>9</kbd> vuelve a ella. Las vistas se guardan por mapa.",
    "**All paths or one.** In the camera panel, **{{CamPathAll}}** draws every camera move of the script, **{{CamPathActive}}** only the one around the playhead - clearer in long scenes.":
        "**Todos los trayectos o uno.** En el panel de la cámara, **{{CamPathAll}}** dibuja cada movimiento de cámara del script, **{{CamPathActive}}** solo el que rodea al cabezal: más claro en escenas largas.",
    "Every camera path of the scene.":
        "Todos los trayectos de cámara de la escena.",
    "**Fly to a shot.** **{{CamFlyToShot}}** puts your view next to the selected shot.":
        "**Vuela a un plano.** **{{CamFlyToShot}}** pone tu vista junto al plano seleccionado.",
    "A figure that appears on cue":
        "Una figura que aparece a la señal",
    "Spawner keys and spawnflags: a character that waits until the script calls it.":
        "Claves y spawnflags del spawner: un personaje que espera hasta que el script lo llama.",
    "**Two names.** A character spawner has `NPC_targetname` - the name of the figure, used by `affect` - and may have `targetname` - the name of the spawner itself.":
        "**Dos nombres.** Un spawner de personaje tiene `NPC_targetname` (el nombre de la figura, que usa `affect`) y puede tener `targetname` (el nombre del propio spawner).",
    "**Wait for the cue.** Give the spawner a `targetname`, for example `rosh_spawner`. In the game it then spawns its figure only when it is used.":
        "**Espera la señal.** Dale al spawner un `targetname`, por ejemplo `rosh_spawner`. En el juego, entonces solo hace aparecer su figura cuando se usa.",
    "A spawner's keys and spawnflags.":
        "Las claves y spawnflags de un spawner.",
    "**Call it.** In the script: `use ( \"rosh_spawner\" )` - from then on the figure exists and `affect ( \"rosh\" )` reaches it. (The preview shows the figure from the start.)":
        "**Llámalo.** En el script: `use ( \"rosh_spawner\" )`; desde entonces la figura existe y `affect ( \"rosh\" )` la alcanza. (La vista previa muestra la figura desde el principio.)",
    "**Spawnflags.** The checkboxes under **{{EntFlags}}** set the class's options; hover over one for what it does.":
        "**Spawnflags.** Las casillas bajo **{{EntFlags}}** fijan las opciones de la clase; pasa el ratón por encima de una para ver lo que hace.",
    "Load a separate .ent file":
        "Cargar un archivo .ent aparte",
    "Swap the entities of a map without touching the map - the way many mods add their own scenes.":
        "Cambia las entidades de un mapa sin tocar el mapa: así añaden muchos mods sus propias escenas.",
    "**Open it.** **{{MapMore}}** → **{{OpenEnt}}** loads an `.ent` file: its entities replace the ones of the map.":
        "**Ábrelo.** **{{MapMore}}** → **{{OpenEnt}}** carga un archivo `.ent`: sus entidades sustituyen a las del mapa.",
    "The Maps & files menu.":
        "El menú Mapas y archivos.",
    "**Work as usual.** The entities appear in the list and the view; change them like any other.":
        "**Trabaja como siempre.** Las entidades aparecen en la lista y en la vista; cámbialas como cualquier otra.",
    "**Saved with the mission.** **{{MissionSave}}** writes the entities into `maps/<name>.ent` in the `.pk3` - the game loads it instead of the map's own.":
        "**Se guarda con la misión.** **{{MissionSave}}** escribe las entidades en `maps/<nombre>.ent` dentro del `.pk3`; el juego lo carga en vez de las del propio mapa.",
    "Time a scene in the Tracks view":
        "Sincronizar una escena en la vista de pistas",
    "The script over time, right in the script column: camera, figures and dialogue as rows.":
        "El script en el tiempo, directamente en la columna del script: cámara, figuras y diálogo como filas.",
    "Timeline · four steps":
        "Línea de tiempo · cuatro pasos",
    "**Switch to Tracks.** Above the script column choose **{{FlowTracks}}**: rows for **{{TracksCamera}}**, each figure, **{{TracksDialog}}** and **{{TracksScript}}**.":
        "**Cambia a Pistas.** Encima de la columna del script elige **{{FlowTracks}}**: filas para **{{TracksCamera}}**, cada figura, **{{TracksDialog}}** y **{{TracksScript}}**.",
    "**Jump to a line.** Click a block: its line is selected.":
        "**Salta a una línea.** Pulsa un bloque: se selecciona su línea.",
    "**Change a duration.** Drag the right edge of a block.":
        "**Cambia una duración.** Arrastra el borde derecho de un bloque.",
    "**Move through time.** Drag in the ruler; **{{TracksPlay}}** plays.":
        "**Muévete en el tiempo.** Arrastra en la regla; **{{TracksPlay}}** reproduce.",
    "The timeline in detail":
        "La línea de tiempo en detalle",
    "Waits as blocks, quiet rows, frames instead of seconds, speed - and the warning for overlapping camera moves.":
        "Esperas como bloques, filas inactivas, fotogramas en vez de segundos, velocidad, y el aviso de movimientos de cámara solapados.",
    "Waits shown as blocks.":
        "Esperas mostradas como bloques.",
    "**Frames.** Click the time display to switch between seconds and frames.":
        "**Fotogramas.** Pulsa el indicador de tiempo para cambiar entre segundos y fotogramas.",
    "**Speed.** The list next to the play button plays at 0.25x to 2x; sound only at 1x.":
        "**Velocidad.** La lista junto al botón de reproducir va de 0,25x a 2x; el sonido solo a 1x.",
    "**Overlap warning.** A warning sign on a camera block: it overlaps the next move. In the game the next one starts from where this one began, and the camera jumps - shorten one of them.":
        "**Aviso de solapamiento.** Una señal de aviso en un bloque de cámara: se solapa con el siguiente movimiento. En el juego, el siguiente empieza desde donde empezó este y la cámara salta; acorta uno de los dos.",
    "Skins and skeletons of a model":
        "Skins y esqueletos de un modelo",
    "Another skin, another skeleton, and the surfaces a skin switches on or off.":
        "Otra skin, otro esqueleto, y las superficies que una skin activa o desactiva.",
    "**Switch skins.** In the **{{ViewModel}}** tab the **{{ModelSkin}}** list shows the skins that come with the model; **{{OpenSkin}}** loads another one.":
        "**Cambia de skin.** En la pestaña **{{ViewModel}}**, la lista **{{ModelSkin}}** muestra las skins que trae el modelo; **{{OpenSkin}}** carga otra.",
    "The playback bar with the skin list.":
        "La barra de reproducción con la lista de skins.",
    "**Surfaces.** The counter of surfaces shows how many the skin draws; click it to list them all in the detail log.":
        "**Superficies.** El contador de superficies muestra cuántas dibuja la skin; púlsalo para listarlas todas en el registro detallado.",
    "**Caps.** **{{ModelCaps}}** shows the `_off` surfaces that close the holes where limbs attach.":
        "**Tapas.** **{{ModelCaps}}** muestra las superficies `_off` que cierran los huecos donde se unen las extremidades.",
    "**Another skeleton.** **{{ModelLoadGla}}** loads a different `.gla` with its animations.":
        "**Otro esqueleto.** **{{ModelLoadGla}}** carga otro `.gla` con sus animaciones.",
    "This tutorial also runs inside the program: **{{MenuHelp}} → {{TourMenu}}** - or ask the helper (<kbd>F1</kbd>).":
        "Este tutorial también se ejecuta dentro del programa: **{{MenuHelp}} → {{TourMenu}}**, o pregunta al asistente (<kbd>F1</kbd>).",
    "Set up the game folder and the preferences":
        "Configurar la carpeta del juego y las preferencias",
    "Tell the program where Jedi Academy is installed, check what it found, and choose the rules of the preview.":
        "Dile al programa dónde está instalado Jedi Academy, comprueba lo que encontró y elige las reglas de la vista previa.",
    "**Open the paths.** **{{MenuEdit}} → {{PathsTitle}}**. In the map editor switch to **{{ViewEvents}}** first - the menus **{{MenuFile}}**, **{{MenuEdit}}** and **{{MenuView}}** are there.":
        "**Abre las rutas.** **{{MenuEdit}} → {{PathsTitle}}**. En el editor de mapas cambia antes a **{{ViewEvents}}**: ahí están los menús **{{MenuFile}}**, **{{MenuEdit}}** y **{{MenuView}}**.",
    "**Add the game folder.** Press **{{Pk3AddPath}}** and choose the `base` folder of Jedi Academy - the one with `assets0.pk3`. For Movie Duels add its `MD` folder too.":
        "**Añade la carpeta del juego.** Pulsa **{{Pk3AddPath}}** y elige la carpeta `base` de Jedi Academy, la que tiene `assets0.pk3`. Para Movie Duels añade también su carpeta `MD`.",
    "The tutorial in the program: the button is highlighted.":
        "El tutorial en el programa: el botón está resaltado.",
    "**Check the list.** Every folder shows how many archives it found. If it says none, you picked the wrong folder - remove it and try again. Then close the window.":
        "**Revisa la lista.** Cada carpeta muestra cuántos archivos .pk3 encontró. Si dice que ninguno, elegiste la carpeta equivocada: quítala y prueba de nuevo. Luego cierra la ventana.",
    "**Open the preferences.** **{{MenuEdit}} → {{AppPrefs}}**: sound levels, the game rules of the preview, difficulty and how the program behaves.":
        "**Abre las preferencias.** **{{MenuEdit}} → {{AppPrefs}}**: volúmenes de sonido, las reglas del juego de la vista previa, la dificultad y cómo se comporta el programa.",
    "**Choose the rules.** **{{PrefsRegelwerk}}**: *auto* follows your game folder; OpenJK or Movie Duels make the preview behave exactly like that game - walking, doors, timing.":
        "**Elige las reglas.** **{{PrefsRegelwerk}}**: *automático* sigue a tu carpeta del juego; OpenJK o Movie Duels hacen que la vista previa se comporte exactamente como ese juego: marchas, puertas, tiempos.",
    "The program only reads the game folder. It never writes into it - your own files go into your mission or mod folder.":
        "El programa solo lee la carpeta del juego. Nunca escribe en ella: tus propios archivos van a la carpeta de tu misión o de tu mod.",
    "Check for updates":
        "Buscar actualizaciones",
    "Look for a new version, read what changed, and decide whether the program checks at start.":
        "Busca una versión nueva, lee qué cambió y decide si el programa comprueba al iniciar.",
    "Settings · four steps":
        "Ajustes · cuatro pasos",
    "**Look for a new version.** **{{MenuHelp}} → {{UpdMenu}}**. The program asks GitHub for the newest release - only now, because you chose it.":
        "**Busca una versión nueva.** **{{MenuHelp}} → {{UpdMenu}}**. El programa pregunta a GitHub por la versión más reciente, solo ahora, porque tú lo has elegido.",
    "The tutorial points at the Help menu.":
        "El tutorial señala el menú Ayuda.",
    "**Read the answer.** The window says that you have the newest version - or shows the new one with its notes and **{{UpdInstall}}**. Without a connection it says so; **{{UpdRetry}}** tries again.":
        "**Lee la respuesta.** La ventana dice que tienes la versión más reciente, o muestra la nueva con sus notas y **{{UpdInstall}}**. Sin conexión lo dice; **{{UpdRetry}}** lo vuelve a intentar.",
    "**At start or not.** **{{MenuHelp}} → {{UpdAutoCheck}}** switches the check at start on or off. Nothing else is ever sent.":
        "**Al iniciar o no.** **{{MenuHelp}} → {{UpdAutoCheck}}** activa o desactiva la comprobación al iniciar. Nunca se envía nada más.",
    "**What changed?** **{{ChangelogMenu}}** in the window (or in **{{MenuHelp}}**) lists the changes of every version.":
        "**¿Qué cambió?** **{{ChangelogMenu}}** en la ventana (o en **{{MenuHelp}}**) lista los cambios de cada versión.",
    "A door and a trigger in the map editor":
        "Una puerta y un trigger en el editor de mapas",
    "Turn a brush into a door, tell it which way to open, and let a trigger open it when the player walks in.":
        "Convierte un brush en una puerta, dile hacia dónde abrirse y deja que un trigger la abra cuando entre el jugador.",
    "Map editor · seven steps":
        "Editor de mapas · siete pasos",
    "**Open the map editor.** **{{ViewMapEditor}}** in the mode bar. Open your map from [[tut-karte]] - or any map with a room.":
        "**Abre el editor de mapas.** **{{ViewMapEditor}}** en la barra de modos. Abre tu mapa de [[tut-karte]], o cualquier mapa con una sala.",
    "**Draw the door.** Drag a thin brush with the LEFT mouse button in a 2D view, where the door should be - for example 16 units thick.":
        "**Dibuja la puerta.** Arrastra un brush fino con el botón IZQUIERDO en una vista 2D, donde debe estar la puerta; por ejemplo de 16 unidades de grosor.",
    "**Make it a door.** Keep it selected, right-click in the 2D view and choose **func → func_door**. The brush is now an entity that moves.":
        "**Conviértelo en puerta.** Mantenlo seleccionado, haz clic derecho en la vista 2D y elige **func → func_door**. El brush es ahora una entidad que se mueve.",
    "**Which way it opens.** Press <kbd>N</kbd> for the entity inspector. Set the key `angle` to `90` (it slides north; `-1` is up, `-2` down) and press <kbd>Enter</kbd>.":
        "**Hacia dónde abre.** Pulsa <kbd>N</kbd> para el inspector de entidades. Pon la clave `angle` a `90` (se desliza hacia el norte; `-1` es arriba, `-2` abajo) y pulsa <kbd>Intro</kbd>.",
    "**Draw the trigger.** Draw a second brush in front of the door, right-click and choose **trigger → trigger_multiple**. Triggers are invisible in the game.":
        "**Dibuja el trigger.** Dibuja un segundo brush delante de la puerta, haz clic derecho y elige **trigger → trigger_multiple**. Los triggers son invisibles en el juego.",
    "**Connect them.** Select the trigger first, then the door (<kbd>Shift</kbd>+click), and press <kbd>Ctrl</kbd>+<kbd>K</kbd> - **{{KeLEntity}} → {{KeLConnectEntities}}**. The trigger's `target` now names the door's `targetname`.":
        "**Conéctalos.** Selecciona primero el trigger, luego la puerta (<kbd>Mayús</kbd>+clic) y pulsa <kbd>Ctrl</kbd>+<kbd>K</kbd>: **{{KeLEntity}} → {{KeLConnectEntities}}**. El `target` del trigger nombra ahora el `targetname` de la puerta.",
    "**Try it.** Build the map and walk through it as in [[tut-karte]]: the door opens when you step into the trigger.":
        "**Pruébalo.** Compila el mapa y recórrelo como en [[tut-karte]]: la puerta se abre cuando entras en el trigger.",
    "A door without a name opens by itself when the player touches it. Once a trigger names it, it waits for the trigger.":
        "Una puerta sin nombre se abre sola cuando el jugador la toca. Cuando un trigger la nombra, espera al trigger.",
    "Change a game menu":
        "Cambiar un menú del juego",
    "Open a menu of the game, move a button, try it like in the game and pack it for your mod.":
        "Abre un menú del juego, mueve un botón, pruébalo como en el juego y empaquétalo para tu mod.",
    "Menu editor · five steps":
        "Editor de menús · cinco pasos",
    "**Open the menu editor.** **{{ViewMenuEditor}}** in the mode bar.":
        "**Abre el editor de menús.** **{{ViewMenuEditor}}** en la barra de modos.",
    "**Open a menu of the game.** **{{MeAusSpiel}}** and pick one, for example `ui/jamp/main.menu`. It is read from the game files and drawn like the game - the game folder is not changed.":
        "**Abre un menú del juego.** **{{MeAusSpiel}}** y elige uno, por ejemplo `ui/jamp/main.menu`. Se lee de los archivos del juego y se dibuja como en el juego; la carpeta del juego no cambia.",
    "**Move an element.** Click a button on the canvas to select it, then press it again and drag it - or nudge it with the arrow keys. Its `rect` in the code below changes with it.":
        "**Mueve un elemento.** Pulsa un botón en el lienzo para seleccionarlo, vuelve a pulsarlo y arrástralo, o empújalo con las flechas. Su `rect` en el código de abajo cambia con él.",
    "**Try it like in the game.** **{{MeBedienen}}**: clicks now press buttons and change cvars - only in the preview, never in your game. **{{MeBearbeiten}}** brings you back.":
        "**Pruébalo como en el juego.** **{{MeBedienen}}**: los clics pulsan ahora botones y cambian cvars, solo en la vista previa, nunca en tu juego. **{{MeBearbeiten}}** te devuelve.",
    "Step 4 of the tutorial in the menu editor.":
        "Paso 4 del tutorial en el editor de menús.",
    "**Pack it.** **{{MeExportPk3}}** writes the changed menu into a .pk3 for your mod folder, for example `zz_menu.pk3`. The game folder itself is never written.":
        "**Empaquétalo.** **{{MeExportPk3}}** escribe el menú cambiado en un .pk3 para la carpeta de tu mod, por ejemplo `zz_menu.pk3`. Nunca se escribe en la propia carpeta del juego.",
    "More in [[menue_editor]].":
        "Más en [[menue_editor]].",
    "The helper and the tutorials":
        "El asistente y los tutoriales",
    "Press F1, write what you want to do, and the program shows you the way. Interactive tutorials wait for your real clicks.":
        "Pulsa F1, escribe lo que quieres hacer y el programa te muestra el camino. Los tutoriales interactivos esperan a tus clics reales.",
    "The helper: what do you want to do?":
        "El asistente: ¿qué quieres hacer?",
    "Press <kbd>F1</kbd> - or **{{MenuHelp}} → {{HlfMenu}}**. In the map editor <kbd>F1</kbd> opens the manual as in NetRadiant; there use **{{KeLHelp}} → {{HlfMenu}}**.":
        "Pulsa <kbd>F1</kbd>, o **{{MenuHelp}} → {{HlfMenu}}**. En el editor de mapas <kbd>F1</kbd> abre el manual como en NetRadiant; ahí usa **{{KeLHelp}} → {{HlfMenu}}**.",
    "Write what you want to do, in your own words. For example \"add a light\" or \"make an NPC walk\". German, English, Chinese, Japanese, Spanish and Czech all work.":
        "Escribe con tus palabras lo que quieres hacer. Por ejemplo \"añadir una luz\" o \"hacer que un NPC camine\". Funcionan alemán, inglés, chino, japonés, español y checo.",
    "Pick a result. **{{HlfShowMe}}** starts the tutorial at the right step, or highlights the button or menu entry. Nothing is changed.":
        "Elige un resultado. **{{HlfShowMe}}** inicia el tutorial en el paso adecuado, o resalta el botón o la entrada de menú. No se cambia nada.",
    "**{{HlfGuide}}** opens the matching page of this guide.":
        "**{{HlfGuide}}** abre la página correspondiente de esta guía.",
    "The helper: a question in your own words, the best matches with \"Show me\".":
        "El asistente: una pregunta con tus palabras, los mejores resultados con \"Muéstramelo\".",
    "Show me: the program darkens the screen and highlights the menu entry.":
        "Muéstramelo: el programa oscurece la pantalla y resalta la entrada de menú.",
    "The helper works without the internet. It knows every tutorial step, every page of this guide, every menu entry and every setting of the program - also of the map editor, with the names from NetRadiant.":
        "El asistente funciona sin internet. Conoce cada paso de los tutoriales, cada página de esta guía, cada entrada de menú y cada ajuste del programa, también del editor de mapas, con los nombres de NetRadiant.",
    "Interactive tutorials":
        "Tutoriales interactivos",
    "A tutorial darkens the screen except for the control you need, and explains the next step in a small window.":
        "Un tutorial oscurece la pantalla salvo el control que necesitas y explica el siguiente paso en una ventana pequeña.",
    "It waits until you have really done the step - for example until the brush is there - and then moves on by itself.":
        "Espera hasta que hayas hecho de verdad el paso (por ejemplo, hasta que esté el brush) y luego avanza solo.",
    "If something else happens, it tells you kindly what to do instead - for example \"That was a light - choose info_player_start\".":
        "Si pasa otra cosa, te dice amablemente qué hacer en su lugar; por ejemplo \"Eso era una luz: elige info_player_start\".",
    "You can stop at any time; the program remembers how far you got.":
        "Puedes parar en cualquier momento; el programa recuerda hasta dónde llegaste.",
    "Tutorials in the program: the five script tutorials, [[tut-karte]] (15 steps: room, texture, light, player start, build, walk through, pack as .pk3) [[k-kamera-ansicht]] for a cutscene and [[k-modell]].":
        "Tutoriales en el programa: los cinco tutoriales de scripts, [[tut-karte]] (15 pasos: sala, textura, luz, inicio del jugador, compilar, recorrer, empaquetar como .pk3), [[k-kamera-ansicht]] para una cinemática y [[k-modell]].",
    "On the very first start the program asks **{{EsTitle}}** - pick a goal, or just look around.":
        "En el primer inicio el programa pregunta **{{EsTitle}}**: elige un objetivo, o simplemente echa un vistazo.",
    "Ask Claude (optional)":
        "Preguntar a Claude (opcional)",
    "If you want, you can connect Claude, an AI from Anthropic. It is off until you switch it on. You need your own Anthropic API key; the questions then go to Anthropic and cost money on your account.":
        "Si quieres, puedes conectar Claude, una IA de Anthropic. Está desactivado hasta que lo actives. Necesitas tu propia clave de API de Anthropic; las preguntas se envían entonces a Anthropic y cuestan dinero en tu cuenta.",
    "Open the helper (<kbd>F1</kbd>) and unfold **{{KiSection}}**.":
        "Abre el asistente (<kbd>F1</kbd>) y despliega **{{KiSection}}**.",
    "Tick **{{KiEnable}}**, paste your key and press **{{KiKeySave}}**. The key is stored encrypted on this PC.":
        "Marca **{{KiEnable}}**, pega tu clave y pulsa **{{KiKeySave}}**. La clave se guarda cifrada en este PC.",
    "Write your question in the helper and press **{{KiAsk}}**. Claude answers and can start a tutorial or highlight a control for you.":
        "Escribe tu pregunta en el asistente y pulsa **{{KiAsk}}**. Claude responde y puede iniciar un tutorial o resaltarte un control.",
    "Claude can also do things for you with the program's own commands, for example hollow out a brush. You always see the list first and decide: **{{KiRun}}** or **{{KiReject}}**. One job is one undo step - <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes all of it back.":
        "Claude también puede hacer cosas por ti con los comandos propios del programa, por ejemplo vaciar un brush. Siempre ves antes la lista y decides: **{{KiRun}}** o **{{KiReject}}**. Un trabajo es un paso de deshacer: <kbd>Ctrl</kbd>+<kbd>Z</kbd> lo revierte todo.",
    "Before every change the AI asks: Run or Reject. Top right: AI connected, with Disconnect.":
        "Antes de cada cambio la IA pregunta: Ejecutar o Rechazar. Arriba a la derecha: IA conectada, con Desconectar.",
    "Connect your own AI app (MCP)":
        "Conectar tu propia app de IA (MCP)",
    "MCP (Model Context Protocol) is a common way for AI apps to use programs. Claude Desktop, Claude Code, Cursor and VS Code can look at JK Studio, show you things and - after you confirm - run program commands.":
        "MCP (Model Context Protocol) es una forma habitual de que las apps de IA usen programas. Claude Desktop, Claude Code, Cursor y VS Code pueden ver JK Studio, mostrarte cosas y, tras tu confirmación, ejecutar comandos del programa.",
    "In the helper, unfold **{{McpSection}}**.":
        "En el asistente, despliega **{{McpSection}}**.",
    "Press the copy button for your app. It puts a ready setup on the clipboard: the app starts JK Studio itself with `--mcp`. This needs no network and is the safest way.":
        "Pulsa el botón de copiar de tu app. Pone en el portapapeles una configuración lista: la app inicia JK Studio ella misma con `--mcp`. No necesita red y es la forma más segura.",
    "Paste it into the settings of the app (Claude Desktop: claude_desktop_config.json, Cursor: .cursor/mcp.json, VS Code: .vscode/mcp.json; Claude Code: run the command) and restart the app.":
        "Pégala en la configuración de la app (Claude Desktop: claude_desktop_config.json, Cursor: .cursor/mcp.json, VS Code: .vscode/mcp.json; Claude Code: ejecuta el comando) y reinicia la app.",
    "Connect your own AI app: ready setups to copy for Claude Desktop, Claude Code, Cursor and VS Code.":
        "Conectar tu propia app de IA: configuraciones listas para copiar para Claude Desktop, Claude Code, Cursor y VS Code.",
    "If JK Studio is already running, you can instead tick **{{McpHttp}}**. Only programs on this PC can reach it, and only with the secret token from the copy button. **{{McpNewToken}}** makes a new token at any time.":
        "Si JK Studio ya se está ejecutando, puedes marcar en su lugar **{{McpHttp}}**. Solo los programas de este PC pueden llegar a ella, y solo con el token secreto del botón de copiar. **{{McpNewToken}}** crea un token nuevo en cualquier momento.",
    "Is this safe?":
        "¿Es seguro?",
    "**Off by default.** Nothing connects until you switch it on yourself. While an AI is connected, **{{KiConnected}}** shows at the top right, with **{{KiDisconnect}}**.":
        "**Desactivado por defecto.** No se conecta nada hasta que lo actives tú mismo. Mientras una IA está conectada, arriba a la derecha aparece **{{KiConnected}}**, con **{{KiDisconnect}}**.",
    "**The AI cannot touch your files.** It has no access to files, no command line and no internet through JK Studio. It can only read what the program shows, show you things, and use the program's own commands.":
        "**La IA no puede tocar tus archivos.** No tiene acceso a archivos, ni línea de comandos, ni internet a través de JK Studio. Solo puede leer lo que muestra el programa, mostrarte cosas y usar los comandos propios del programa.",
    "**You decide.** Every change is shown first and only runs after **{{KiRun}}**. Saving, overwriting and deleting files always ask, even if you allowed changes without asking. Every job is one undo step.":
        "**Tú decides.** Cada cambio se muestra antes y solo se ejecuta tras **{{KiRun}}**. Guardar, sobrescribir y borrar archivos siempre preguntan, aunque hayas permitido cambios sin preguntar. Cada trabajo es un paso de deshacer.",
    "**Tricks in files do not work.** Text inside a map or script (for example \"delete all files\") is passed to the AI as data, never as an order - and there is no command it could use for that anyway.":
        "**Los trucos en archivos no funcionan.** El texto dentro de un mapa o de un script (por ejemplo \"borra todos los archivos\") se pasa a la IA como datos, nunca como orden, y además no hay ningún comando que pudiera usar para eso.",
    "**Websites cannot reach it.** The local connection only answers this PC, checks where a request comes from and needs the secret token.":
        "**Los sitios web no pueden llegar a ella.** La conexión local solo responde a este PC, comprueba de dónde viene una petición y necesita el token secreto.",
    "**Your key stays yours.** The API key and the token are stored encrypted for your Windows user. The key only goes to api.anthropic.com over an encrypted connection, never into logs.":
        "**Tu clave sigue siendo tuya.** La clave de API y el token se guardan cifrados para tu usuario de Windows. La clave solo va a api.anthropic.com por una conexión cifrada, nunca a los registros.",
    "**You can see everything.** **{{KiLog}}** lists every action of the AI. JK Studio sends no usage data anywhere.":
        "**Puedes verlo todo.** **{{KiLog}}** lista cada acción de la IA. JK Studio no envía datos de uso a ningún sitio.",
    "**Switch it off:** press **{{KiDisconnect}}**, untick **{{KiEnable}}** and **{{McpHttp}}**, or delete the key with **{{KiKeyDelete}}**.":
        "**Desactívalo:** pulsa **{{KiDisconnect}}**, desmarca **{{KiEnable}}** y **{{McpHttp}}**, o borra la clave con **{{KiKeyDelete}}**.",
    "Finding things faster":
        "Encontrar las cosas más rápido",
    "**Where you work counts.** In the map editor, results of the map editor come first; in the menu editor, those of the menu editor. \"Settings\" in the map editor finds its own preferences.":
        "**Cuenta dónde trabajas.** En el editor de mapas, los resultados del editor de mapas salen primero; en el editor de menús, los del editor de menús. \"Ajustes\" en el editor de mapas encuentra sus propias preferencias.",
    "**Your own words, any form.** \"runs\", \"ran\", \"running\" or \"rennt\", \"läuft\" find the same. Small typos are fine; with no match at all the helper offers **Did you mean ...?**":
        "**Con tus palabras, en cualquier forma.** \"corre\", \"corrió\", \"corriendo\" o \"runs\", \"running\" encuentran lo mismo. Las erratas pequeñas no importan; si no hay ningún resultado, el asistente ofrece **¿Quisiste decir...?**",
    "**It also knows what is inside.** The buttons of the toolbars, the pages of the map editor's preferences, the menu editor and what a dialog contains are all part of the search.":
        "**También sabe lo que hay dentro.** Los botones de las barras, las páginas de las preferencias del editor de mapas, el editor de menús y lo que contiene un diálogo forman parte de la búsqueda.",
    "**Show me, step by step.** If the control is not visible yet, **{{HlfShowMe}}** first points at what opens it - for a setting of the main program searched in the map editor: first **{{ViewEvents}}**, then **{{MenuEdit}} → {{AppPrefs}}**, then the field itself.":
        "**Muéstramelo, paso a paso.** Si el control aún no está visible, **{{HlfShowMe}}** señala primero lo que lo abre; para un ajuste del programa principal buscado en el editor de mapas: primero **{{ViewEvents}}**, luego **{{MenuEdit}} → {{AppPrefs}}** y luego el propio campo.",
    "Ask follow-up questions: **{{KiAskMore}}** keeps the conversation, so Claude knows what you asked before. The answer appears while it is written; **{{KiStop}}** stops it at once - nothing of that question stays. **{{KiHistoryClear}}** deletes the conversation from your computer.":
        "Haz preguntas de seguimiento: **{{KiAskMore}}** conserva la conversación, así Claude sabe lo que preguntaste antes. La respuesta aparece mientras se escribe; **{{KiStop}}** la detiene enseguida y no queda nada de esa pregunta. **{{KiHistoryClear}}** borra la conversación de tu ordenador.",
    "In the menu editor Claude can also select, move, duplicate or delete elements and switch to **{{MeBedienen}}** - again only after **{{KiRun}}**, and the whole job is one undo step. Opening, saving or exporting a menu is not possible for the AI.":
        "En el editor de menús Claude también puede seleccionar, mover, duplicar o eliminar elementos y cambiar a **{{MeBedienen}}**, de nuevo solo tras **{{KiRun}}**, y todo el trabajo es un paso de deshacer. Abrir, guardar o exportar un menú no es posible para la IA.",
    "**The conversation stays on your PC.** It is stored encrypted for your Windows user and deleted with **{{KiHistoryClear}}**. After each request the program also wipes the key from its memory.":
        "**La conversación se queda en tu PC.** Se guarda cifrada para tu usuario de Windows y se borra con **{{KiHistoryClear}}**. Tras cada petición, el programa también borra la clave de su memoria.",
    "Scripts":
        "Scripts",
    "Camera and timeline":
        "Cámara y línea de tiempo",
    "Figures":
        "Figuras",
    "Map and entities":
        "Mapa y entidades",
    "Animation":
        "Animación",
    "Missions, models and settings":
        "Misiones, modelos y ajustes",
    "Map editor and menu editor":
        "Editor de mapas y editor de menús",
    "Welcome":
        "Bienvenida",
    "JK Studio - mod tools for Jedi Knight: Jedi Academy and Jedi Outcast. Its BehavEd part writes the scripts that run cutscenes and missions, the preview shows them on the map while you write them, and the map editor builds the maps.":
        "JK Studio: herramientas de mods para Jedi Knight: Jedi Academy y Jedi Outcast. Su parte BehavEd escribe los scripts que controlan cinemáticas y misiones, la vista previa los muestra en el mapa mientras los escribes, y el editor de mapas construye los mapas.",
    "A mission of your own on taspir1 in JK Studio: the script on the right, the map in the middle, the timeline below. Every part is explained on [[fenster]].":
        "Una misión propia en taspir1 en JK Studio: el script a la derecha, el mapa en el centro, la línea de tiempo debajo. Cada parte se explica en [[fenster]].",
    "**{{MenuHelp}} → {{HelpAbout}}**: name and version, the three parts BehavEd (scripts), preview and map editor, and what the BehavEd part was measured against.":
        "**{{MenuHelp}} → {{HelpAbout}}**: nombre y versión, las tres partes BehavEd (scripts), vista previa y editor de mapas, y con qué se midió la parte BehavEd.",
    "JK Studio is a free fan project. It is not affiliated with Lucasfilm, Disney or Raven Software. Up to rc605 it was called BehavEd-Remake; it updates itself and keeps your settings.":
        "JK Studio es un proyecto gratuito de fans. No tiene relación con Lucasfilm, Disney ni Raven Software. Hasta la rc605 se llamaba BehavEd-Remake; se actualiza solo y conserva tus ajustes.",
    "The game runs its scenes with **ICARUS** scripts: lists of commands such as \"walk to this spot\", \"play this sound\", \"move the camera there\". Raven's original BehavEd let you build those lists from buttons. JK Studio does the same, file for file compatible, and adds what the original never had: the map in 3D, the figures walking their paths, the camera flying its shots, and a timeline that plays it all - before you ever start the game.":
        "El juego ejecuta sus escenas con scripts **ICARUS**: listas de comandos como \"camina hasta este sitio\", \"reproduce este sonido\", \"mueve la cámara allí\". El BehavEd original de Raven permitía construir esas listas con botones. JK Studio hace lo mismo, compatible archivo a archivo, y añade lo que el original nunca tuvo: el mapa en 3D, las figuras recorriendo sus trayectos, la cámara volando sus planos y una línea de tiempo que lo reproduce todo, antes de iniciar siquiera el juego.",
    "Where to start":
        "Por dónde empezar",
    "New here? Read [[erste-schritte]] and [[fenster]] - ten minutes, and every later page makes sense.":
        "¿Eres nuevo? Lee [[erste-schritte]] y [[fenster]]: diez minutos, y cada página posterior tendrá sentido.",
    "Want to make something right away? Pick one of the tutorials below.":
        "¿Quieres hacer algo ya? Elige uno de los tutoriales de abajo.",
    "Looking for one thing? Type it into the search box at the top left - it searches every page and jumps to the spot.":
        "¿Buscas algo concreto? Escríbelo en el campo de búsqueda de arriba a la izquierda: busca en cada página y salta al sitio.",
    "Tutorials":
        "Tutoriales",
    "Besides the big mission there are %d short tutorials - each shows one thing in three to six steps. All of them, with pictures: [[tutorials]].":
        "Además de la gran misión hay %d tutoriales cortos: cada uno muestra una cosa en tres a seis pasos. Todos, con imágenes: [[tutorials]].",
    "What the program can do":
        "Lo que puede hacer el programa",
    "Area":
        "Área",
    "In short":
        "En resumen",
    "Build scripts from the command list, edit every value in a dialog, copy, move, undo - like the original BehavEd.":
        "Construye scripts desde la lista de comandos, edita cada valor en un diálogo, copia, mueve, deshaz, como en el BehavEd original.",
    "See a script as a tree, as a graph of who starts whom, or as tracks over time.":
        "Ve un script como árbol, como grafo de quién inicia a quién, o como pistas en el tiempo.",
    "The map in 3D with lights, effects and models; fly through it, select and move entities.":
        "El mapa en 3D con luces, efectos y modelos; vuela por él, selecciona y mueve entidades.",
    "Camera shots as keys on the map: move and turn them with the gizmo, fly them yourself, preview them.":
        "Planos de cámara como claves en el mapa: muévelos y gíralos con el gizmo, vuélalos tú mismo, previsualízalos.",
    "Play the scene, see every command in time, drag commands to change when they happen.":
        "Reproduce la escena, ve cada comando en el tiempo, arrastra comandos para cambiar cuándo ocurren.",
    "Paths of walking figures and flying ships: start and end handles, new points, walk or run, facing.":
        "Trayectos de figuras que caminan y de naves que vuelan: asas de inicio y fin, puntos nuevos, caminar o correr, orientación.",
    "Animate ships, doors and platforms - as move commands or as a ROFF file, built and edited right here.":
        "Anima naves, puertas y plataformas, como comandos move o como archivo ROFF, construidos y editados aquí mismo.",
    "Open a whole mission with all its scripts, build your own, and pack it into a .pk3 the game loads.":
        "Abre una misión entera con todos sus scripts, crea la tuya y empaquétala en un .pk3 que carga el juego.",
    "**Nothing is lost.** Every change can be undone with <kbd>Ctrl</kbd>+<kbd>Z</kbd>, and files are only written when you save. The game files themselves are never changed: new things go next to your scripts or into your own .pk3.":
        "**No se pierde nada.** Cada cambio se puede deshacer con <kbd>Ctrl</kbd>+<kbd>Z</kbd>, y los archivos solo se escriben al guardar. Los propios archivos del juego nunca cambian: lo nuevo va junto a tus scripts o a tu propio .pk3.",
    "Getting started":
        "Primeros pasos",
    "Unpack, tell the program where the game is, open a mission. That is all the setup there is.":
        "Descomprime, dile al programa dónde está el juego, abre una misión. Esa es toda la configuración.",
    "Install":
        "Instalar",
    "Download the latest `JK Studio-…-windows-x64.zip` from the releases page - under **Assets**, not \"Source code\", which contains no program - and unpack it into a folder of your choice, for example next to your game. Nothing is installed into Windows.":
        "Descarga el último `JK Studio-…-windows-x64.zip` de la página de versiones (en **Assets**, no \"Source code\", que no contiene ningún programa) y descomprímelo en una carpeta de tu elección, por ejemplo junto a tu juego. No se instala nada en Windows.",
    "Start `jkstudio.exe`. At startup the program looks for a newer version on GitHub and offers it; nothing is downloaded without asking you (the switch is in **{{MenuHelp}}**).":
        "Inicia `jkstudio.exe`. Al iniciar, el programa busca una versión más reciente en GitHub y la ofrece; no se descarga nada sin preguntarte (el interruptor está en **{{MenuHelp}}**).",
    "**Just the program?** `JK Studio-…-windows-x64.exe` from the same release runs on its own: on the first start it puts its data into `%APPDATA%\\behaved\\data\\base`. The guide is only in the zip. Windows SmartScreen may warn the first time because the program is not code-signed: click **More info → Run anyway**.":
        "**¿Solo el programa?** `JK Studio-…-windows-x64.exe` de la misma versión funciona por sí solo: en el primer inicio pone sus datos en `%APPDATA%\\behaved\\data\\base`. La guía solo está en el zip. Windows SmartScreen puede avisar la primera vez porque el programa no está firmado: pulsa **Más información → Ejecutar de todas formas**.",
    "Tell it where the game is":
        "Dile dónde está el juego",
    "Maps, models, sounds and the scripts of the game live in the game's `.pk3` archives. Open **{{MenuEdit}} → {{PathsTitle}}** and add the game folders with **{{Pk3AddPath}}**: first `base`, then a mod such as Movie Duels (`MD`). The bottom folder wins, exactly as `fs_game` overrides `base` in the game.":
        "Los mapas, modelos, sonidos y scripts del juego están en sus archivos `.pk3`. Abre **{{MenuEdit}} → {{PathsTitle}}** y añade las carpetas del juego con **{{Pk3AddPath}}**: primero `base`, luego un mod como Movie Duels (`MD`). Gana la carpeta de abajo, exactamente como `fs_game` sustituye a `base` en el juego.",
    "The game folders: base first, the mod below it - the bottom one wins.":
        "Las carpetas del juego: base primero, el mod debajo; gana la de abajo.",
    "Open something":
        "Abrir algo",
    "There are several ways in. The start page in the **{{ViewMap}}** tab (shown while no map is open) offers a new mission, a mission from a `.pk3`, just a map, and your recent missions. From the menu:":
        "Hay varias maneras de empezar. La página de inicio de la pestaña **{{ViewMap}}** (visible mientras no hay ningún mapa abierto) ofrece una misión nueva, una misión de un `.pk3`, solo un mapa y tus misiones recientes. Desde el menú:",
    "**A mission** - **{{MenuFile}} → {{LoadMission}}** picks a `.pk3` and opens a mission with its map, its entities and all its scripts in tabs. See [[missionen]].":
        "**Una misión**: **{{MenuFile}} → {{LoadMission}}** elige un `.pk3` y abre una misión con su mapa, sus entidades y todos sus scripts en pestañas. Mira [[missionen]].",
    "**A script** - **{{MenuFile}} → {{FileOpen}}** opens a `.txt` or a compiled `.ibi` from disk.":
        "**Un script**: **{{MenuFile}} → {{FileOpen}}** abre un `.txt` o un `.ibi` compilado del disco.",
    "**Your last missions** - **{{MenuFile}} → {{RecentMissions}}** reopens a whole mission with one click.":
        "**Tus últimas misiones**: **{{MenuFile}} → {{RecentMissions}}** vuelve a abrir una misión entera con un clic.",
    "The start page appears in the Mission tab while no map is open.":
        "La página de inicio aparece en la pestaña Misión mientras no hay ningún mapa abierto.",
    "Language":
        "Idioma",
    "The program speaks English, German, Chinese, Japanese, Spanish and Czech; a text a language does not have yet appears in English. It starts in the language of Windows; change it under **{{MenuView}} → {{ViewLanguage}}**. Command names and values stay as the game writes them (`affect`, `SET_NAVGOAL`), because that is what goes into the file.":
        "El programa habla inglés, alemán, chino, japonés, español y checo; un texto que un idioma aún no tiene aparece en inglés. Empieza en el idioma de Windows; cámbialo en **{{MenuView}} → {{ViewLanguage}}**. Los nombres de los comandos y los valores se quedan como los escribe el juego (`affect`, `SET_NAVGOAL`), porque así van en el archivo.",
    "This guide opens with <kbd>F1</kbd>, from **{{MenuHelp}} → {{HelpGuide}}**, from **{{StartGuideFirst}}** and **{{StartGuideTut}}** on the start page, and from the **{{AboutGuide}}** button in {{HelpAbout}}. It lives in the `handbuch` folder next to `jkstudio.exe` and works without internet. **{{MenuHelp}} → {{TourMenu}}** shows the first steps right in the program, one highlighted click at a time.":
        "Esta guía se abre con <kbd>F1</kbd>, desde **{{MenuHelp}} → {{HelpGuide}}**, desde **{{StartGuideFirst}}** y **{{StartGuideTut}}** en la página de inicio, y desde el botón **{{AboutGuide}}** de {{HelpAbout}}. Está en la carpeta `handbuch` junto a `jkstudio.exe` y funciona sin internet. **{{MenuHelp}} → {{TourMenu}}** muestra los primeros pasos directamente en el programa, un clic resaltado cada vez.",
    "What JK Studio can do":
        "Lo que puede hacer JK Studio",
    "A tour of the whole program: writing scripts, seeing them on the map, cameras, figures, animation, missions and the tools around them.":
        "Un recorrido por todo el programa: escribir scripts, verlos en el mapa, cámaras, figuras, animación, misiones y las herramientas de alrededor.",
    "JK Studio writes ICARUS scripts - the scripts behind every cutscene and every scripted moment of Jedi Academy and its mods. It writes them exactly like Raven's original BehavEd, file for file, and then shows you what they do: on the map, in 3D, over time. Each section below links to the tutorials that practise it.":
        "JK Studio escribe scripts ICARUS, los scripts detrás de cada cinemática y de cada momento con script de Jedi Academy y sus mods. Los escribe exactamente como el BehavEd original de Raven, archivo a archivo, y luego te muestra lo que hacen: en el mapa, en 3D, en el tiempo. Cada sección de abajo enlaza a los tutoriales que lo practican.",
    "From an idea to the game":
        "De una idea al juego",
    "Write the script":
        "Escribe el script",
    "See it on the map":
        "Míralo en el mapa",
    "Time it":
        "Sincronízalo",
    "Save mission":
        "Guarda la misión",
    "Play it":
        "Juégalo",
    "commands, values, blocks":
        "comandos, valores, bloques",
    "figures, camera, ships":
        "figuras, cámara, naves",
    "timeline, Tracks":
        "línea de tiempo, pistas",
    "checks, compile, .pk3":
        "comprobaciones, compilar, .pk3",
    "devmap <name>":
        "devmap <nombre>",
    "Write scripts like the original":
        "Escribir scripts como el original",
    "The command list, the script tree and the action column.":
        "La lista de comandos, el árbol del script y la columna de acciones.",
    "Every command of the game in one list - double-click, drag or press <kbd>Insert</kbd> to add it. [[k-befehl-einfuegen]]":
        "Cada comando del juego en una lista: doble clic, arrastra o pulsa <kbd>Insert</kbd> para añadirlo. [[k-befehl-einfuegen]]",
    "Each value in its own field: lists of allowed values, names picked from the map, sounds you can play, expressions with `get`, `tag` and `random`. [[k-werte-bearbeiten]]":
        "Cada valor en su propio campo: listas de valores permitidos, nombres elegidos del mapa, sonidos que puedes reproducir, expresiones con `get`, `tag` y `random`. [[k-werte-bearbeiten]]",
    "Copy, move, comment out, undo every step - even one single step from the middle of the history. [[k-zeilen-ordnen]], [[k-rueckgaengig]]":
        "Copia, mueve, comenta, deshaz cada paso, incluso un único paso de en medio del historial. [[k-zeilen-ordnen]], [[k-rueckgaengig]]",
    "Compile to the `.ibi` the game runs, with clear messages that jump to the wrong line. [[k-uebersetzen]]":
        "Compila al `.ibi` que ejecuta el juego, con mensajes claros que saltan a la línea errónea. [[k-uebersetzen]]",
    "Every command and every `SET_` field explained: [[befehle]].":
        "Cada comando y cada campo `SET_` explicado: [[befehle]].",
    "Three views of the same script":
        "Tres vistas del mismo script",
    "Graph: what starts what.":
        "Grafo: qué inicia qué.",
    "Tracks: the script over time.":
        "Pistas: el script en el tiempo.",
    "**{{FlowClassic}}** is the tree of the original. **{{FlowGraph}}** shows the mission as nodes - the player start, runners, characters, scripts - and lets you connect them by dragging. **{{FlowTracks}}** lays the script out over time, with the camera, every figure and the dialogue in rows. [[skriptfluss]], [[k-graph]]":
        "**{{FlowClassic}}** es el árbol del original. **{{FlowGraph}}** muestra la misión como nodos (el inicio del jugador, ejecutores, personajes, scripts) y te deja conectarlos arrastrando. **{{FlowTracks}}** extiende el script en el tiempo, con la cámara, cada figura y el diálogo en filas. [[skriptfluss]], [[k-graph]]",
    "The map in 3D - as the game draws it":
        "El mapa en 3D, como lo dibuja el juego",
    "The landing pad of taspir1 with the mission's entities.":
        "La plataforma de aterrizaje de taspir1 con las entidades de la misión.",
    "Maps load straight from the game's `.pk3` files, with their shaders, lightmaps, fog, glow, effects, sky portals and mirrors. Fly through them, look from above, save views on number keys. Place entities with tiles, drag and turn them on the grid, edit every key in the inspector. [[karte]], [[k-entity-setzen]], [[k-entity-felder]], [[k-draufsicht]]":
        "Los mapas se cargan directamente de los archivos `.pk3` del juego, con sus shaders, lightmaps, niebla, resplandor, efectos, portales de cielo y espejos. Vuela por ellos, mira desde arriba, guarda vistas en las teclas numéricas. Coloca entidades con fichas, arrástralas y gíralas en la rejilla, edita cada clave en el inspector. [[karte]], [[k-entity-setzen]], [[k-entity-felder]], [[k-draufsicht]]",
    "Cameras you can see and fly":
        "Cámaras que puedes ver y volar",
    "A camera path played through the camera.":
        "Un trayecto de cámara reproducido a través de la cámara.",
    "Each `camera ( MOVE )` and `PAN` is a shot on the map with its path. Move and turn shots with the gizmo, put them where you are looking, or sit in a shot and fly it in pilot mode. Zoom, fade, shake and follow play in the view exactly as in the game. [[kamera]], [[k-pilot]], [[k-kamera-ansicht]], [[k-zoom-blende]]":
        "Cada `camera ( MOVE )` y `PAN` es un plano en el mapa con su trayecto. Mueve y gira los planos con el gizmo, ponlos donde estás mirando, o siéntate en un plano y vuélalo en modo piloto. Zoom, fundido, temblor y seguir se reproducen en la vista exactamente como en el juego. [[kamera]], [[k-pilot]], [[k-kamera-ansicht]], [[k-zoom-blende]]",
    "Figures that walk their paths":
        "Figuras que recorren sus trayectos",
    "A figure in the middle of its walk, with its path.":
        "Una figura en mitad de su marcha, con su trayecto.",
    "Characters spawn with their real models and animations and walk where the script sends them. Their paths have handles: drag a walk target, insert a point mid-walk, switch walk, run or walk backwards, set where a figure looks and which way it faces. [[figuren]], [[k-gangart]], [[k-ansehen]], [[k-blickrichtung]]":
        "Los personajes aparecen con sus modelos y animaciones reales y caminan adonde los manda el script. Sus trayectos tienen asas: arrastra un destino, inserta un punto en mitad de la marcha, cambia entre caminar, correr o caminar hacia atrás, fija adónde mira una figura y hacia dónde se orienta. [[figuren]], [[k-gangart]], [[k-ansehen]], [[k-blickrichtung]]",
    "A timeline for the whole scene":
        "Una línea de tiempo para toda la escena",
    "Play the scene with sound, at any speed. Every command is a block in time - drag it and everything after moves along, drag its edge to change a duration. [[zeitleiste]], [[k-neu-timen]]":
        "Reproduce la escena con sonido, a cualquier velocidad. Cada comando es un bloque en el tiempo: arrástralo y todo lo que va detrás se mueve también, arrastra su borde para cambiar una duración. [[zeitleiste]], [[k-neu-timen]]",
    "Animation: move commands and ROFF files":
        "Animación: comandos move y archivos ROFF",
    "The Raven's Claw taking off on a ROFF path.":
        "La Garra del Cuervo despegando por un trayecto ROFF.",
    "Set keys over time for a door, a platform or a ship. behaved writes them as `move` commands or builds a ROFF file, and it turns the game's own ROFF files into keys you can change - as a copy, never touching the original. [[animation]], [[tut-p5]], [[k-tuer]], [[k-roff-bearbeiten]]":
        "Pon claves en el tiempo para una puerta, una plataforma o una nave. behaved las escribe como comandos `move` o construye un archivo ROFF, y convierte los propios archivos ROFF del juego en claves que puedes cambiar, como copia, sin tocar nunca el original. [[animation]], [[tut-p5]], [[k-tuer]], [[k-roff-bearbeiten]]",
    "Missions: from nothing to a .pk3":
        "Misiones: de la nada a un .pk3",
    "Saving a mission: every file, every check.":
        "Guardando una misión: cada archivo, cada comprobación.",
    "Start a mission on any map with one click, open the missions of the game or a mod, continue them from their folder. **{{MissionSave}}** checks everything, compiles every script and packs one `.pk3` with map, entities, scripts and ROFF files - ready for `devmap`. NPC waypoint networks are created and checked on the way. [[missionen]], [[tut-projekt]], [[k-wegpunkte]]":
        "Empieza una misión en cualquier mapa con un clic, abre las misiones del juego o de un mod, continúalas desde su carpeta. **{{MissionSave}}** lo comprueba todo, compila cada script y empaqueta un `.pk3` con mapa, entidades, scripts y archivos ROFF, listo para `devmap`. Por el camino se crean y comprueban las redes de waypoints de NPC. [[missionen]], [[tut-projekt]], [[k-wegpunkte]]",
    "Character models and their animations":
        "Modelos de personajes y sus animaciones",
    "A character in the model view, with its animation list.":
        "Un personaje en la vista de modelos, con su lista de animaciones.",
    "Every character of the game with its skin, every animation with its frames - click to play, double-click to put it into the script. [[modell]], [[k-animation-abspielen]], [[k-modell]]":
        "Cada personaje del juego con su skin, cada animación con sus fotogramas: pulsa para reproducirla, doble clic para ponerla en el script. [[modell]], [[k-animation-abspielen]], [[k-modell]]",
    "The tools around it":
        "Las herramientas de alrededor",
    "Find and replace in one script or in all open ones. [[k-suchen-ersetzen]]":
        "Buscar y reemplazar en un script o en todos los abiertos. [[k-suchen-ersetzen]]",
    "Tabs, bookmarks and a split view to compare scripts and copy across. [[k-lesezeichen]]":
        "Pestañas, marcadores y una vista dividida para comparar scripts y copiar de uno a otro. [[k-lesezeichen]]",
    "Backups on every save, a backup folder, revert any command to how it was. [[speichern]]":
        "Copias de seguridad en cada guardado, una carpeta de copias, devolver cualquier comando a como estaba. [[speichern]]",
    "Six languages, six themes, any interface size, every shortcut rebindable. [[einstellungen]], [[tasten]]":
        "Seis idiomas, seis temas, cualquier tamaño de interfaz, cada atajo reasignable. [[einstellungen]], [[tasten]]",
    "Checks for updates and installs them. [[einstellungen#updates]]":
        "Busca actualizaciones y las instala. [[einstellungen#updates]]",
    "The window":
        "La ventana",
    "One window, three modes. The mode bar on the left decides what the big area shows; script, map and timeline always talk to each other.":
        "Una ventana, tres modos. La barra de modos de la izquierda decide qué muestra el área grande; script, mapa y línea de tiempo siempre se comunican entre sí.",
    "The window in the Mission tab with a mission open.":
        "La ventana en la pestaña Misión con una misión abierta.",
    "Menu bar":
        "Barra de menús",
    "File, Edit, Debug, View, Help.":
        "Archivo, Edición, Depuración, Ver, Ayuda.",
    "Tabs":
        "Pestañas",
    "One tab per open script; **+** opens an empty one, the mouse wheel over the bar scrolls through many.":
        "Una pestaña por script abierto; **+** abre una vacía, la rueda del ratón sobre la barra recorre muchas.",
    "Mode bar":
        "Barra de modos",
    "**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** - what the big area shows.":
        "**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**: lo que muestra el área grande.",
    "Entity list and inspector":
        "Lista de entidades e inspector",
    "Every entity of the map in groups; below, the keys of the selected one.":
        "Cada entidad del mapa en grupos; debajo, las claves de la seleccionada.",
    "Map":
        "Mapa",
    "The toolbar and the map in 3D, with the placement tiles on the left.":
        "La barra de herramientas y el mapa en 3D, con las fichas de colocación a la izquierda.",
    "Script column":
        "Columna del script",
    "The script as **{{FlowClassic}}** tree, **{{FlowGraph}}** or **{{FlowTracks}}**.":
        "El script como árbol **{{FlowClassic}}**, **{{FlowGraph}}** o **{{FlowTracks}}**.",
    "Timeline":
        "Línea de tiempo",
    "Plays the scene; every command is a block in time.":
        "Reproduce la escena; cada comando es un bloque en el tiempo.",
    "Status line":
        "Línea de estado",
    "Messages, the undo history, when the script was compiled.":
        "Mensajes, el historial de deshacer, cuándo se compiló el script.",
    "The parts":
        "Las partes",
    "Part":
        "Parte",
    "What it is for":
        "Para qué sirve",
    "{{MenuFile}}, {{MenuEdit}}, {{MenuDebug}}, {{MenuView}}, {{MenuHelp}} - opening and saving, game folders, look and language, the guide.":
        "{{MenuFile}}, {{MenuEdit}}, {{MenuDebug}}, {{MenuView}}, {{MenuHelp}}: abrir y guardar, carpetas del juego, aspecto e idioma, la guía.",
    "Script tabs":
        "Pestañas de scripts",
    "One tab per open script. A `*` means unsaved changes, `+` opens a new script, tabs can be dragged into another order. When there are more tabs than fit, turn the mouse wheel over the tab bar to scroll them; the small arrow at the left lists every open script. All tabs of a mission share the map and the camera.":
        "Una pestaña por script abierto. Un `*` indica cambios sin guardar, `+` abre un script nuevo, las pestañas se pueden arrastrar a otro orden. Si hay más pestañas de las que caben, gira la rueda del ratón sobre la barra de pestañas para desplazarlas; la flechita de la izquierda lista todos los scripts abiertos. Todas las pestañas de una misión comparten el mapa y la cámara.",
    "The strip at the far left: **{{GroupEvents}}** (build scripts), **{{ViewMap}}** (the mission: the map in 3D, entities, cameras, timeline - this tab was called *Map* before version rc600), **{{ViewModel}}** (look at a character model).":
        "La franja del extremo izquierdo: **{{GroupEvents}}** (crear scripts), **{{ViewMap}}** (la misión: el mapa en 3D, entidades, cámaras, línea de tiempo; esta pestaña se llamaba *Map* antes de la versión rc600), **{{ViewModel}}** (ver un modelo de personaje).",
    "Left column":
        "Columna izquierda",
    "Depends on the mode: the list of commands, the entities of the map with their inspector, or the animations of a model.":
        "Depende del modo: la lista de comandos, las entidades del mapa con su inspector, o las animaciones de un modelo.",
    "The script itself - as a tree ({{FlowClassic}}), a graph ({{FlowGraph}}) or over time ({{FlowTracks}}). See [[skriptfluss]].":
        "El propio script: como árbol ({{FlowClassic}}), como grafo ({{FlowGraph}}) o en el tiempo ({{FlowTracks}}). Mira [[skriptfluss]].",
    "The button column on the right, as in the original: edit, file, tree options, **{{Compile}}**. In the **{{ViewMap}}** tab it makes room for the map.":
        "La columna de botones de la derecha, como en el original: editar, archivo, opciones del árbol, **{{Compile}}**. En la pestaña **{{ViewMap}}** deja sitio al mapa.",
    "Under the map: plays the scene and shows every command in time. See [[zeitleiste]].":
        "Bajo el mapa: reproduce la escena y muestra cada comando en el tiempo. Mira [[zeitleiste]].",
    "Messages, the error and note counters (click them for the list), the undo history and when the script was last compiled.":
        "Mensajes, los contadores de errores y notas (púlsalos para ver la lista), el historial de deshacer y cuándo se compiló el script por última vez.",
    "Two scripts side by side":
        "Dos scripts uno al lado del otro",
    "**{{MenuFile}} → {{SplitToggle}}** shows up to four scripts beside each other in Events mode - for comparing and for copying commands across. Drag a command from one pane into another to copy it; click into a pane to work in it.":
        "**{{MenuFile}} → {{SplitToggle}}** muestra hasta cuatro scripts uno junto a otro en el modo Eventos, para comparar y para copiar comandos de uno a otro. Arrastra un comando de un panel a otro para copiarlo; pulsa en un panel para trabajar en él.",
    "Size, look and layout":
        "Tamaño, aspecto y disposición",
    "<kbd>Ctrl</kbd> + mouse wheel scales the whole interface (50 - 300 %); **{{MenuView}} → {{ViewDpiScale}}** does the same.":
        "<kbd>Ctrl</kbd> + rueda del ratón escala toda la interfaz (50 - 300 %); **{{MenuView}} → {{ViewDpiScale}}** hace lo mismo.",
    "Drag the borders between the columns to resize them; each mode remembers its widths. **{{MenuDebug}} → {{DebugReset}}** puts everything back.":
        "Arrastra los bordes entre las columnas para cambiar su tamaño; cada modo recuerda sus anchos. **{{MenuDebug}} → {{DebugReset}}** lo devuelve todo a su sitio.",
    "Opening a file in Explorer while JK Studio runs hands it to the open window instead of starting a second program.":
        "Abrir un archivo en el Explorador mientras JK Studio está abierto lo pasa a la ventana abierta en vez de iniciar un segundo programa.",
    "Writing scripts":
        "Escribir scripts",
    "An ICARUS script is a tree of commands. Pick commands from the list, put them in the right block, fill in their values.":
        "Un script ICARUS es un árbol de comandos. Elige comandos de la lista, ponlos en el bloque correcto y rellena sus valores.",
    "Events mode: the commands on the left, the script tree in the middle, the action buttons on the right.":
        "Modo Eventos: los comandos a la izquierda, el árbol del script en el centro, los botones de acción a la derecha.",
    "How a script is built":
        "Cómo se construye un script",
    "Commands run from top to bottom. `affect ( \"name\", FLUSH ) { ... }` sends the commands inside it to one figure or entity - that is how a cutscene moves its actors. `task` declares a group of commands, `do` starts it alongside, `dowait` waits for it, `wait` pauses. `if`, `else`, `loop` work as in other languages. Only `affect`, `task`, `if`, `else` and `loop` can hold commands.":
        "Los comandos se ejecutan de arriba abajo. `affect ( \"nombre\", FLUSH ) { ... }` envía los comandos que contiene a una figura o entidad: así mueve una cinemática a sus actores. `task` declara un grupo de comandos, `do` lo inicia en paralelo, `dowait` lo espera, `wait` hace una pausa. `if`, `else`, `loop` funcionan como en otros lenguajes. Solo `affect`, `task`, `if`, `else` y `loop` pueden contener comandos.",
    "Adding commands":
        "Añadir comandos",
    "In the **{{GroupEvents}}** list on the left, **double-click** a command to insert it after the selected line (or into the selected block).":
        "En la lista **{{GroupEvents}}** de la izquierda, haz **doble clic** en un comando para insertarlo tras la línea seleccionada (o dentro del bloque seleccionado).",
    "Or **drag** it into the tree: where you let go decides whether it lands before, inside or after a line.":
        "O **arrástralo** al árbol: dónde lo sueltas decide si cae antes, dentro o después de una línea.",
    "Macros at the end of the list insert several commands at once; **{{TreeFoldMacros}}** shows them as one line.":
        "Las macros al final de la lista insertan varios comandos a la vez; **{{TreeFoldMacros}}** las muestra como una sola línea.",
    "Right-click a line for the same lists as submenus, plus edit, copy and **{{ActFindSimilar}}**.":
        "Clic derecho en una línea para tener las mismas listas como submenús, además de editar, copiar y **{{ActFindSimilar}}**.",
    "The event editor":
        "El editor de eventos",
    "Double-click a line (or press <kbd>Enter</kbd>) to open the **{{EditorTitle}}**. The help text of the command stands on top, each value has its own field below: lists for fixed choices, number and text fields, file fields with **{{EditorBrowse}}** to browse and **{{EditorPlay}}** to play a sound or open a script. When a map is loaded, an arrow next to name fields picks a name from the map.":
        "Haz doble clic en una línea (o pulsa <kbd>Intro</kbd>) para abrir el **{{EditorTitle}}**. El texto de ayuda del comando está arriba y cada valor tiene su propio campo debajo: listas para opciones fijas, campos de número y de texto, campos de archivo con **{{EditorBrowse}}** para buscar y **{{EditorPlay}}** para reproducir un sonido o abrir un script. Con un mapa cargado, una flecha junto a los campos de nombre elige un nombre del mapa.",
    "**{{EditorExpr}}** turns a field into a free expression; **{{EditorHelper}}** builds `get ( ... )`, `tag ( ... )` or `random ( ... )` with **{{EditorGet}}**, **{{EditorTag}}** and **{{EditorRnd}}**; **{{EditorRevert}}** goes back.":
        "**{{EditorExpr}}** convierte un campo en una expresión libre; **{{EditorHelper}}** construye `get ( ... )`, `tag ( ... )` o `random ( ... )` con **{{EditorGet}}**, **{{EditorTag}}** y **{{EditorRnd}}**; **{{EditorRevert}}** vuelve atrás.",
    "A value that is not in its list is allowed - mods bring their own - but marked **unknown**, because far more often it is a typo.":
        "Se permite un valor que no esté en su lista (los mods traen los suyos), pero se marca como **desconocido**, porque casi siempre es una errata.",
    "**{{EditorOk}}** (<kbd>Enter</kbd>) writes the values, **{{EditorCancel}}** (<kbd>Esc</kbd>) drops them, **{{EditorReEvaluate}}** resets the fields to the defaults.":
        "**{{EditorOk}}** (<kbd>Intro</kbd>) escribe los valores, **{{EditorCancel}}** (<kbd>Esc</kbd>) los descarta, **{{EditorReEvaluate}}** devuelve los campos a los valores predeterminados.",
    "Selecting, moving, copying":
        "Seleccionar, mover, copiar",
    "Click selects a line, <kbd>Ctrl</kbd>+click adds one, <kbd>Shift</kbd>+click selects a range. Every action works on everything selected.":
        "Clic selecciona una línea, <kbd>Ctrl</kbd>+clic añade una, <kbd>Mayús</kbd>+clic selecciona un rango. Cada acción actúa sobre todo lo seleccionado.",
    "Drag lines to move them. Blocks move with everything inside; a block cannot go into itself.":
        "Arrastra líneas para moverlas. Los bloques se mueven con todo lo que contienen; un bloque no puede ir dentro de sí mismo.",
    "**{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}** - pasting into a block puts the commands in exactly once, also when the block and its lines were copied together.":
        "**{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**: pegar en un bloque mete los comandos exactamente una vez, también cuando se copiaron juntos el bloque y sus líneas.",
    "**{{ActClone}}** inserts a copy right below, **{{ActRem}}** turns lines into comments and back, **{{ActDelete}}** removes them.":
        "**{{ActClone}}** inserta una copia justo debajo, **{{ActRem}}** convierte líneas en comentarios y al revés, **{{ActDelete}}** las quita.",
    "**Undo** and **Redo** remember every step. Right-click either button for the full history - you can even undo one single step and keep everything after it.":
        "**Deshacer** y **Rehacer** recuerdan cada paso. Clic derecho en cualquiera de los dos botones para ver el historial completo: incluso puedes deshacer un único paso y conservar todo lo posterior.",
    "Find, replace, bookmarks":
        "Buscar, reemplazar, marcadores",
    "**{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>) opens a window that stays open: search by command name (**{{FindNamed}}**) or by a value (**{{FindContains}}**), step with <kbd>F3</kbd> / <kbd>Shift</kbd>+<kbd>F3</kbd>, replace in this script or in all open ones. Bookmarks work like Notepad++: <kbd>Ctrl</kbd>+<kbd>F2</kbd> sets one, <kbd>F2</kbd> jumps to the next. They are kept per file.":
        "**{{ActFind}}** (<kbd>Ctrl</kbd>+<kbd>F</kbd>) abre una ventana que se queda abierta: busca por nombre de comando (**{{FindNamed}}**) o por un valor (**{{FindContains}}**), avanza con <kbd>F3</kbd> / <kbd>Mayús</kbd>+<kbd>F3</kbd>, reemplaza en este script o en todos los abiertos. Los marcadores funcionan como en Notepad++: <kbd>Ctrl</kbd>+<kbd>F2</kbd> pone uno, <kbd>F2</kbd> salta al siguiente. Se guardan por archivo.",
    "The change margin":
        "El margen de cambios",
    "With **{{MenuView}} → {{ViewChangeHistory}}** a coloured margin marks what you changed: orange = changed, not saved; green = changed and saved; blue = back to the original. Hover it to see the original line, right-click a line → **Revert to original** to restore just that command.":
        "Con **{{MenuView}} → {{ViewChangeHistory}}** un margen de color marca lo que cambiaste: naranja = cambiado, sin guardar; verde = cambiado y guardado; azul = de nuevo como el original. Pasa el ratón por encima para ver la línea original; clic derecho en una línea → **Volver al original** restaura solo ese comando.",
    "Script Flow: Classic, Graph, Tracks":
        "Flujo del script: Clásica, Grafo, Pistas",
    "Three views of the same script - switch any time, nothing is lost.":
        "Tres vistas del mismo script: cambia cuando quieras, no se pierde nada.",
    "The buttons **{{FlowClassic}} / {{FlowGraph}} / {{FlowTracks}}** at the top of the {{GroupScriptFlow}} column switch the view. In a split view every pane has its own.":
        "Los botones **{{FlowClassic}} / {{FlowGraph}} / {{FlowTracks}}** arriba de la columna {{GroupScriptFlow}} cambian la vista. En una vista dividida cada panel tiene la suya.",
    "Classic":
        "Clásica",
    "The tree of the original BehavEd - every command one line, blocks to fold open; see [[skripte]]. Since rc601 you can build and change a script in **every** view - the tutorials on scripts have a switch at the top that shows their steps for Classic, Graph or Tracks.":
        "El árbol del BehavEd original: cada comando una línea, bloques que se despliegan; mira [[skripte]]. Desde la rc601 puedes construir y cambiar un script en **cualquier** vista; los tutoriales de scripts tienen arriba un interruptor que muestra sus pasos para Clásica, Grafo o Pistas.",
    "Graph: what starts what":
        "Grafo: qué inicia qué",
    "The graph of a mission: player start, triggers, script runners, figures and scripts with their connections.":
        "El grafo de una misión: inicio del jugador, triggers, ejecutores de scripts, figuras y scripts con sus conexiones.",
    "A mission is held together by its entities: a `trigger` targets a `target_scriptrunner`, which runs a script, which affects figures and starts other scripts. The graph draws exactly that. Double-click a node to open its script or show the entity in the map; drag from the dot on the right of a node to another node to connect them - for a figure you choose what kind of script (spawnscript, deathscript, ...). Drag entities from the list or commands into the graph; a command dropped on free space creates a new script.":
        "Una misión se sostiene por sus entidades: un `trigger` apunta a un `target_scriptrunner`, que ejecuta un script, que afecta a figuras e inicia otros scripts. El grafo dibuja exactamente eso. Doble clic en un nodo abre su script o muestra la entidad en el mapa; arrastra desde el punto a la derecha de un nodo hasta otro nodo para conectarlos; para una figura eliges qué tipo de script (spawnscript, deathscript, ...). Arrastra entidades de la lista o comandos al grafo; un comando soltado en un espacio libre crea un script nuevo.",
    "**{{GraphFit}}** shows everything, **{{GraphRelayout}}** sorts the nodes, **{{GraphRightAngles}}** draws the wires at right angles through the gaps. Double-click a wire for a bend point; right-click removes it.":
        "**{{GraphFit}}** lo muestra todo, **{{GraphRelayout}}** ordena los nodos, **{{GraphRightAngles}}** dibuja los cables en ángulo recto por los huecos. Doble clic en un cable añade un punto de curva; clic derecho lo quita.",
    "**Editing in the Graph.** Click a script node: on the right, **{{GraphCommands}}** lists every command of the script, blocks indented. Click a line to select it, double-click to open the editor, right-click for the same menu as in Classic (edit, duplicate, copy, cut, paste, move, REM, delete, insert). A command from the **{{GroupEvents}}** list dropped on a line goes after it; double-clicking a command in the list inserts it after the selected line. A script that is not open shows **{{GraphEditHere}}**.":
        "**Editar en el Grafo.** Pulsa un nodo de script: a la derecha, **{{GraphCommands}}** lista cada comando del script, con los bloques sangrados. Clic en una línea la selecciona, doble clic abre el editor, clic derecho abre el mismo menú que en Clásica (editar, duplicar, copiar, cortar, pegar, mover, REM, eliminar, insertar). Un comando de la lista **{{GroupEvents}}** soltado sobre una línea va detrás de ella; doble clic en un comando de la lista lo inserta tras la línea seleccionada. Un script que no está abierto muestra **{{GraphEditHere}}**.",
    "Tracks: the script over time":
        "Pistas: el script en el tiempo",
    "Tracks: camera shots, figures, dialogue and script as blocks in time.":
        "Pistas: planos de cámara, figuras, diálogo y script como bloques en el tiempo.",
    "A compact timeline inside the script column: rows for **{{TracksCamera}}**, every figure, **{{TracksDialog}}** (voice lines) and **{{TracksScript}}**. Click a block to select its line, drag its right edge to change the duration, drag in the ruler to move through time. The big [[zeitleiste]] under the map does the same with more detail.":
        "Una línea de tiempo compacta dentro de la columna del script: filas para **{{TracksCamera}}**, cada figura, **{{TracksDialog}}** (frases de voz) y **{{TracksScript}}**. Clic en un bloque selecciona su línea, arrastra su borde derecho para cambiar la duración, arrastra en la regla para moverte en el tiempo. La gran [[zeitleiste]] bajo el mapa hace lo mismo con más detalle.",
    "**Editing in Tracks.** Double-click a block to open the editor; right-click it for the menu of the Classic line. Drag a command from the **{{GroupEvents}}** list into a row: a yellow line shows the moment, and the command goes after the last command of that row that starts before it - in a figure's row, inside that figure's `affect` block. On the right, **{{TracksList}}** lists every command, also those that take no time (`affect`, `task`, `declare`) and an empty script - it works like the list in the Graph. Switch it off with the checkbox to give the timeline the whole width.":
        "**Editar en Pistas.** Doble clic en un bloque abre el editor; clic derecho abre el menú de la línea Clásica. Arrastra un comando de la lista **{{GroupEvents}}** a una fila: una línea amarilla muestra el momento, y el comando va tras el último comando de esa fila que empiece antes; en la fila de una figura, dentro del bloque `affect` de esa figura. A la derecha, **{{TracksList}}** lista cada comando, también los que no ocupan tiempo (`affect`, `task`, `declare`) y un script vacío; funciona como la lista del Grafo. Desactívala con la casilla para dar todo el ancho a la línea de tiempo.",
    "Saving, compiling, checks":
        "Guardar, compilar, comprobaciones",
    "The game runs `.ibi` files. You write `.txt`; **Compile!** makes the `.ibi` - byte for byte like Raven's IBIZE.":
        "El juego ejecuta archivos `.ibi`. Tú escribes `.txt`; **¡Compilar!** crea el `.ibi`, byte a byte como el IBIZE de Raven.",
    "Saving":
        "Guardar",
    "**{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) always writes the `.txt` source - also when you opened an `.ibi` (it lands next to it).":
        "**{{FileSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) siempre escribe la fuente `.txt`, también cuando abriste un `.ibi` (queda a su lado).",
    "**{{ActSaveAll}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) saves every tab with changes; tabs from an archive have no file to write back to and are skipped.":
        "**{{ActSaveAll}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>S</kbd>) guarda cada pestaña con cambios; las pestañas de un .pk3 no tienen archivo donde escribir y se omiten.",
    "Every save and compile keeps a copy - `backup1.txt` (newest) to `backup10.txt` - in the `backup` folder next to `jkstudio.exe`, one subfolder per script; **{{MenuFile}} → {{FileBackupFolder}}** shows them. **{{FileBackup}}** / **{{FileRestore}}** keep and restore one extra copy (`<script>.bak` next to the script) by hand.":
        "Cada guardado y cada compilación conservan una copia (de `backup1.txt`, la más reciente, a `backup10.txt`) en la carpeta `backup` junto a `jkstudio.exe`, una subcarpeta por script; **{{MenuFile}} → {{FileBackupFolder}}** las muestra. **{{FileBackup}}** / **{{FileRestore}}** guardan y restauran a mano una copia extra (`<script>.bak` junto al script).",
    "Own ROFF paths are written as `.rof` files together with the script - see [[animation#roff]].":
        "Los trayectos ROFF propios se escriben como archivos `.rof` junto con el script; mira [[animation#roff]].",
    "Compiling":
        "Compilar",
    "**{{Compile}}** in the action column writes `<script>.ibi` next to the `.txt` (and saves the `.txt`). The button flashes green with **{{CompileDone}}**, or red with **{{CompileFailedShort}}** - then the first bad line is selected and the status line says why. The compiler is built in and was checked against 1011 `.ibi` files from Raven's IBIZE, byte for byte.":
        "**{{Compile}}** en la columna de acciones escribe `<script>.ibi` junto al `.txt` (y guarda el `.txt`). El botón parpadea en verde con **{{CompileDone}}**, o en rojo con **{{CompileFailedShort}}**: entonces queda seleccionada la primera línea errónea y la línea de estado dice por qué. El compilador va integrado y se comprobó con 1011 archivos `.ibi` del IBIZE de Raven, byte a byte.",
    "Notes and errors":
        "Notas y errores",
    "While you write, the program checks every line: unknown commands, values that are not in their list, a text field without quotes, a misspelled field (\"did you mean ...?\"). The counters in the status line show how many there are - click them for the list, click a message to jump to its line. **{{MenuView}} → {{ViewTarget}}** adds notes on what does not work on a multiplayer server.":
        "Mientras escribes, el programa comprueba cada línea: comandos desconocidos, valores que no están en su lista, un campo de texto sin comillas, un campo mal escrito (\"¿quisiste decir ...?\"). Los contadores de la línea de estado muestran cuántos hay: púlsalos para ver la lista, pulsa un mensaje para saltar a su línea. **{{MenuView}} → {{ViewTarget}}** añade notas sobre lo que no funciona en un servidor multijugador.",
    "Camera":
        "Cámara",
    "Every camera MOVE and PAN of the script is a shot you can see, grab and fly - the script is rewritten as you drag.":
        "Cada MOVE y PAN de cámara del script es un plano que puedes ver, agarrar y volar: el script se reescribe mientras arrastras.",
    "A selected camera: its path on the map, the gizmo, the toolbar on top and the list of shots on the left.":
        "Una cámara seleccionada: su trayecto en el mapa, el gizmo, la barra de arriba y la lista de planos a la izquierda.",
    "Shots":
        "Planos",
    "In ICARUS a shot is `camera ( MOVE, <position>, <time> )` followed by `camera ( PAN, <angles>, <time> )`. JK Studio reads them in order and draws each as a camera symbol on the map. Click the camera symbol to select the camera; the list **{{CamShots}}** on the left shows every shot with its time and travel time (`moves 6.0 s`). Click a line to jump there.":
        "En ICARUS un plano es `camera ( MOVE, <posición>, <tiempo> )` seguido de `camera ( PAN, <ángulos>, <tiempo> )`. JK Studio los lee en orden y dibuja cada uno como un símbolo de cámara en el mapa. Pulsa el símbolo de cámara para seleccionar la cámara; la lista **{{CamShots}}** de la izquierda muestra cada plano con su tiempo y su tiempo de viaje (`se mueve 6.0 s`). Pulsa una línea para saltar allí.",
    "**{{CamPrevShot}} / {{CamNextShot}}** step through the shots; **{{CamFlyToShot}}** puts your view exactly where the selected shot stands and looks.":
        "**{{CamPrevShot}} / {{CamNextShot}}** recorren los planos; **{{CamFlyToShot}}** pone tu vista exactamente donde está y mira el plano seleccionado.",
    "**{{CamNewShot}}** adds a MOVE and a PAN at your current view, after the selected shot. **{{CamSetToView}}** moves the selected shot to where you are looking.":
        "**{{CamNewShot}}** añade un MOVE y un PAN en tu vista actual, tras el plano seleccionado. **{{CamSetToView}}** mueve el plano seleccionado adonde estás mirando.",
    "**{{CamDeleteShot}}** removes it. Under **{{CamSelectedShot}}** you type exact values: {{CamPosition}}, {{CamAngles}} and {{CamDuration}} (0 = a cut).":
        "**{{CamDeleteShot}}** lo quita. En **{{CamSelectedShot}}** escribes valores exactos: {{CamPosition}}, {{CamAngles}} y {{CamDuration}} (0 = un corte).",
    "Moving and turning a shot":
        "Mover y girar un plano",
    "The toolbar above the view switches the gizmo: **{{GizmoMove}}** shows three arrows and a box in the middle - drag an arrow to move along one axis, the box to move in the view plane. **{{GizmoRotate}}** shows three rings for pitch, yaw and roll. **{{GizmoSpaceWorld}} / {{GizmoSpaceLocal}}** decides whether the arrows follow the map or the camera. Letting go writes the new values into the script - one undo step.":
        "La barra encima de la vista cambia el gizmo: **{{GizmoMove}}** muestra tres flechas y un cuadro en el centro; arrastra una flecha para mover a lo largo de un eje, el cuadro para mover en el plano de la vista. **{{GizmoRotate}}** muestra tres anillos para cabeceo, guiñada y alabeo. **{{GizmoSpaceWorld}} / {{GizmoSpaceLocal}}** decide si las flechas siguen al mapa o a la cámara. Al soltar se escriben los valores nuevos en el script: un paso de deshacer.",
    "{{GizmoMove}}: arrows and the centre box.":
        "{{GizmoMove}}: flechas y el cuadro central.",
    "{{GizmoRotate}}: three rings.":
        "{{GizmoRotate}}: tres anillos.",
    "If the timeline stands **between** two keys when you grab the gizmo, a new key is created at that moment instead of changing the start - like the auto-key of animation programs. While piloting, <kbd>Enter</kbd> sets such a key too ([[kamera#steuern]]).":
        "Si la línea de tiempo está **entre** dos claves cuando agarras el gizmo, se crea una clave nueva en ese momento en vez de cambiar el inicio, como la clave automática de los programas de animación. Mientras pilotas, <kbd>Intro</kbd> también pone una clave así ([[kamera#steuern]]).",
    "Camera paths":
        "Trayectos de cámara",
    "The selected camera draws its path: already flown parts in yellow, the rest in grey. With many shots that gets crowded - **{{CamPathActive}}** shows only the move into the current shot and on to the next, **{{CamPathAll}}** shows every move. Both buttons sit in the camera panel.":
        "La cámara seleccionada dibuja su trayecto: las partes ya voladas en amarillo, el resto en gris. Con muchos planos se llena: **{{CamPathActive}}** muestra solo el movimiento hacia el plano actual y hasta el siguiente, **{{CamPathAll}}** muestra todos. Los dos botones están en el panel de la cámara.",
    "Fly a shot yourself (Pilot)":
        "Volar tú mismo un plano (piloto)",
    "**{{CamPilot}}** puts you into the shot: hold the right mouse button to look around, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> to move, <kbd>Q</kbd> <kbd>E</kbd> for down and up. **{{CamPilotApply}}** writes the view into the script, <kbd>Esc</kbd> cancels. While piloting, move the timeline and press <kbd>Enter</kbd> (**{{CamPilotKey}}**) to set key after key along the way. While piloting, the view uses the shot's zoom and shows the game frame, so you frame the shot as the game will show it.":
        "**{{CamPilot}}** te pone en el plano: mantén el botón derecho para mirar alrededor, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> para moverte, <kbd>Q</kbd> <kbd>E</kbd> para bajar y subir. **{{CamPilotApply}}** escribe la vista en el script, <kbd>Esc</kbd> cancela. Mientras pilotas, mueve la línea de tiempo y pulsa <kbd>Intro</kbd> (**{{CamPilotKey}}**) para poner una clave tras otra por el camino. Mientras pilotas, la vista usa el zoom del plano y muestra el marco del juego, así encuadras el plano como lo mostrará el juego.",
    "Preview and looking through":
        "Vista previa y mirar a través",
    "**{{CamPreview}}** (in the bar of the selected camera, top left in the view) shows a small picture of what the camera sees, bottom right in the view; it follows the gizmo while you drag. Click the picture to look through the camera full size; while you look through it, playing the timeline shows the cutscene as the game will. **{{MapThroughCam}}** in the same bar does this directly.":
        "**{{CamPreview}}** (en la barra de la cámara seleccionada, arriba a la izquierda en la vista) muestra una pequeña imagen de lo que ve la cámara, abajo a la derecha en la vista; sigue al gizmo mientras arrastras. Pulsa la imagen para mirar a través de la cámara a tamaño completo; mientras miras a través de ella, reproducir la línea de tiempo muestra la cinemática como la mostrará el juego. **{{MapThroughCam}}** en la misma barra lo hace directamente.",
    "**What the game shows.** The game computes a camera's field of view for its own window, so through the camera the picture sits in a frame with the game's aspect ratio: black outside, the cinematic bars and fades inside - exactly the picture the player sees. Set the aspect ratio and **cg_fovAspectAdjust** under **{{MapViewSettings}}** → **{{GameScreen}}**. Automatic follows the game rules in the preferences ([[einstellungen#prefs]]): Movie Duels runs at your desktop resolution with cg_fovAspectAdjust 1 (a ZOOM keeps its vertical angle, wide screens see more at the sides), OpenJK starts at 800x600 with 0 (the ZOOM is the horizontal angle). If unsure, type `cg_fovAspectAdjust` and `r_mode` in the game console. In the free view a light frame shows what a camera placed here would see (zoom 90); switch it off with **{{GameFrameFree}}**. Before `camera ( ENABLE )` and after `DISABLE` the game shows the player's view with cg_fov 80 - playback and the preview do the same in scripts that switch the camera themselves. `ROLL` tilts the picture, and `camera ( PATH, ... )` flies the recorded ROFF path, including its `cut`, `fov`, `fovzoom` and `fovaccel` notes.":
        "**Lo que muestra el juego.** El juego calcula el campo de visión de una cámara para su propia ventana, así que a través de la cámara la imagen está en un marco con la relación de aspecto del juego: negro fuera, las barras de cinemática y los fundidos dentro; exactamente la imagen que ve el jugador. Fija la relación de aspecto y **cg_fovAspectAdjust** en **{{MapViewSettings}}** → **{{GameScreen}}**. Automático sigue las reglas del juego de las preferencias ([[einstellungen#prefs]]): Movie Duels funciona a la resolución de tu escritorio con cg_fovAspectAdjust 1 (un ZOOM mantiene su ángulo vertical, las pantallas anchas ven más por los lados), OpenJK empieza a 800x600 con 0 (el ZOOM es el ángulo horizontal). Si dudas, escribe `cg_fovAspectAdjust` y `r_mode` en la consola del juego. En la vista libre, un marco claro muestra lo que vería una cámara colocada aquí (zoom 90); desactívalo con **{{GameFrameFree}}**. Antes de `camera ( ENABLE )` y tras `DISABLE` el juego muestra la vista del jugador con cg_fov 80; la reproducción y la vista previa hacen lo mismo en los scripts que cambian la cámara ellos mismos. `ROLL` inclina la imagen y `camera ( PATH, ... )` vuela el trayecto ROFF grabado, incluidas sus notas `cut`, `fov`, `fovzoom` y `fovaccel`.",
    "Looking through the camera while the timeline plays.":
        "Mirando a través de la cámara mientras se reproduce la línea de tiempo.",
    "Some shots stand on a `ref_tag` or look at one. Then the panel says so, and moving the shot moves that ref_tag in the map - the script itself only names it.":
        "Algunos planos están sobre un `ref_tag` o miran a uno. Entonces el panel lo dice, y mover el plano mueve ese ref_tag en el mapa; el script en sí solo lo nombra.",
    "Playing":
        "Reproducir",
    "Sound like in the game":
        "Sonido como en el juego",
    "The preview plays the scene's sounds the way the game does. You hear from the camera the view shows - the cutscene camera while playing. Every sound is updated every frame: move the camera away and it fades, cut to another place and what is there becomes audible. A voice line that started out of range keeps running silently. Like the game, only sources the camera could see (its PVS) make sabers hum, speakers loop, doors and effects sound; voices, ROFF note sounds and breaking objects are always heard. An invisible NPC does not ignite its saber audibly - the ignition sound comes when it appears. The levels follow the game's defaults (effects 0.5, voices 1.0, music 0.25) and can be changed under **{{MenuEdit}} → {{AppPrefs}}**. Sound only plays at 1x speed.":
        "La vista previa reproduce los sonidos de la escena como lo hace el juego. Oyes desde la cámara que muestra la vista; durante la reproducción, la cámara de la cinemática. Cada sonido se actualiza en cada fotograma: aleja la cámara y se apaga, corta a otro sitio y lo que hay allí se vuelve audible. Una frase que empezó fuera de alcance sigue corriendo en silencio. Como en el juego, solo las fuentes que la cámara podría ver (su PVS) hacen zumbar los sables, repetir los altavoces y sonar puertas y efectos; las voces, los sonidos de notas ROFF y los objetos que se rompen siempre se oyen. Un NPC invisible no enciende su sable de forma audible: el sonido de encendido llega cuando aparece. Los volúmenes siguen los valores predeterminados del juego (efectos 0.5, voces 1.0, música 0.25) y se pueden cambiar en **{{MenuEdit}} → {{AppPrefs}}**. El sonido solo suena a velocidad 1x.",
    "A ROFF `sound` note on a figure stays where it started under Movie Duels rules and moves with the figure under OpenJK rules ([[einstellungen#prefs]]). Under OpenJK rules every note of a ROFF frame fires (Movie Duels and Jedi Outcast fire only the first), and `loop sfx <sound>` gives the entity a looping sound until `loop sfx kill`; a `USE <script>` note starts that script on the entity, and `loop rof absolute|relative` restarts the path like the game (the task of the `play` never finishes, so a `dowait` on it waits forever).":
        "Una nota `sound` de ROFF en una figura se queda donde empezó con las reglas de Movie Duels y se mueve con la figura con las reglas de OpenJK ([[einstellungen#prefs]]). Con las reglas de OpenJK se dispara cada nota de un fotograma ROFF (Movie Duels y Jedi Outcast solo disparan la primera), y `loop sfx <sonido>` da a la entidad un sonido en bucle hasta `loop sfx kill`; una nota `USE <script>` inicia ese script en la entidad, y `loop rof absolute|relative` reinicia el trayecto como el juego (la tarea del `play` nunca termina, así que un `dowait` sobre ella espera para siempre).",
    "Changing time by dragging":
        "Cambiar el tiempo arrastrando",
    "Figures and paths":
        "Figuras y trayectos",
    "Figures walk to their navgoals and ships fly their moves - you see the whole path and can change it on the map.":
        "Las figuras caminan hasta sus navgoals y las naves vuelan sus movimientos: ves el trayecto entero y puedes cambiarlo en el mapa.",
    "A walking figure: the path ahead in grey, the part already walked in yellow, the handles at start and end.":
        "Una figura caminando: el trayecto por delante en gris, la parte ya recorrida en amarillo, las asas al inicio y al final.",
    "Selecting figures and ships":
        "Seleccionar figuras y naves",
    "Click a figure or a ship in the view to select its entity - the spawner of the figure, the brush entity of the ship. Marker and gizmo follow the model while it moves, and an outline shows what is selected. The arrow at the figure points where the model looks.":
        "Pulsa una figura o una nave en la vista para seleccionar su entidad: el spawner de la figura, la entidad de brushes de la nave. El marcador y el gizmo siguen al modelo mientras se mueve, y un contorno muestra lo seleccionado. La flecha de la figura apunta adonde mira el modelo.",
    "Start, end and new points":
        "Inicio, final y puntos nuevos",
    "**Drag the end handle** to move the walk target (the `waypoint_navgoal`) or the end of a flight.":
        "**Arrastra el asa final** para mover el destino (el `waypoint_navgoal`) o el final de un vuelo.",
    "**Drag the start handle** to move only the start, not the whole line.":
        "**Arrastra el asa inicial** para mover solo el inicio, no toda la línea.",
    "**Rotate** works the same way: in the middle of a flight the rings create a new point with the new angles. For a figure (NPC) they write `set ( SET_DYAW )` at that time - the figure turns with its turn rate, as in the game; for the player it stays `set ( SET_ANGLES )`, which turns at once. While walking it applies when the figure arrives, because the game turns walking figures toward their direction. If a watch target is active at that moment, the status line warns: in the game the figure keeps facing it.":
        "**Rotar** funciona igual: en mitad de un vuelo, los anillos crean un punto nuevo con los ángulos nuevos. Para una figura (NPC) escriben `set ( SET_DYAW )` en ese momento: la figura gira con su velocidad de giro, como en el juego; para el jugador sigue siendo `set ( SET_ANGLES )`, que gira enseguida. Mientras camina se aplica cuando la figura llega, porque el juego gira a las figuras que caminan hacia su dirección. Si en ese momento hay activo un objetivo de mirada, la línea de estado avisa: en el juego la figura sigue mirándolo.",
    "Walk or run, shared targets":
        "Caminar o correr, destinos compartidos",
    "Right-click the end handle of a walk (or its band in the timeline) for its menu:":
        "Clic derecho en el asa final de una marcha (o en su banda de la línea de tiempo) para su menú:",
    "**{{BahnGehen}}** / **{{BahnRennen}}** - writes `SET_WALKING` or `SET_RUNNING` before the navgoal.":
        "**{{BahnGehen}}** / **{{BahnRennen}}**: escribe `SET_WALKING` o `SET_RUNNING` antes del navgoal.",
    "**{{BahnAnsehen}}** - the figure keeps turning toward another figure while it walks (`SET_WATCHTARGET`).":
        "**{{BahnAnsehen}}**: la figura se sigue girando hacia otra figura mientras camina (`SET_WATCHTARGET`).",
    "**{{BahnRueckwaerts}}** - a look mark behind the start makes it walk backwards; **{{BahnBlickFrei}}** ends that.":
        "**{{BahnRueckwaerts}}**: una marca de mirada detrás del inicio la hace caminar hacia atrás; **{{BahnBlickFrei}}** lo termina.",
    "When several figures share one navgoal, moving it would move all of them. JK Studio notices and gives this walk its own copy of the target first - the status line tells you which one it created.":
        "Cuando varias figuras comparten un navgoal, moverlo las movería a todas. JK Studio lo nota y primero da a esta marcha su propia copia del destino; la línea de estado te dice cuál creó.",
    "Walking backwards is detected in existing scripts too: when a figure moves against the way it faces, the scene plays `BOTH_WALKBACK1` or `BOTH_RUNBACK1` like the game.":
        "La marcha hacia atrás también se detecta en scripts existentes: cuando una figura se mueve en contra de hacia donde mira, la escena reproduce `BOTH_WALKBACK1` o `BOTH_RUNBACK1` como el juego.",
    "How figures move - like in the game":
        "Cómo se mueven las figuras, como en el juego",
    "The preview moves NPCs by the game's own rules, frame by frame: steering, movement physics and paths. What you see is what the game does with your script:":
        "La vista previa mueve a los NPC con las propias reglas del juego, fotograma a fotograma: dirección, física del movimiento y trayectos. Lo que ves es lo que el juego hace con tu script:",
    "**Speed.** A walk uses the speed that is set when the figure starts - also when `SET_WALKING`, `SET_RUNNING`, `SET_WALKSPEED` or `SET_RUNSPEED` come right after `SET_NAVGOAL`. A new speed during the walk changes the rest of the way. Without a speed in the script the values of the .npc file apply.":
        "**Velocidad.** Una marcha usa la velocidad fijada cuando la figura arranca, también cuando `SET_WALKING`, `SET_RUNNING`, `SET_WALKSPEED` o `SET_RUNSPEED` vienen justo después de `SET_NAVGOAL`. Una velocidad nueva durante la marcha cambia el resto del camino. Sin velocidad en el script valen los valores del archivo .npc.",
    "**Stopping.** `SET_NAVGOAL` with `NULL` stops the figure where it is; a `dowait` on the walk continues. A new navgoal during a walk starts from where the figure is.":
        "**Parar.** `SET_NAVGOAL` con `NULL` detiene la figura donde está; un `dowait` sobre la marcha continúa. Un navgoal nuevo durante una marcha empieza desde donde está la figura.",
    "**Turning.** NPCs turn with their turn rate: 60 + 3 × `SET_YAWSPEED` degrees per second (330 with the usual yawSpeed 90). `SET_DYAW`, `SET_VIEWTARGET` and `SET_WATCHTARGET` turn smoothly, `SET_ANGLES` turns at once. `rotate` and `move` do nothing on an NPC - in the game too. Difficulty and game rules change the rate of some classes ([[einstellungen#prefs]]).":
        "**Girar.** Los NPC giran con su velocidad de giro: 60 + 3 × `SET_YAWSPEED` grados por segundo (330 con el yawSpeed habitual de 90). `SET_DYAW`, `SET_VIEWTARGET` y `SET_WATCHTARGET` giran suavemente, `SET_ANGLES` gira enseguida. `rotate` y `move` no hacen nada en un NPC, en el juego tampoco. La dificultad y las reglas del juego cambian la velocidad de algunas clases ([[einstellungen#prefs]]).",
    "**Gravity.** Figures stand on the floor of the map, fall, climb steps, stick to slopes and ride lifts - with the map's `gravity` and `SET_GRAVITY`. A door, lift or train that a figure is in the way of stops like in the game: a door turns back (a crusher keeps pressing), a train waits, and the figure takes the door's or train's damage (`dmg`, 2 by default); a figure on a lift that would be pressed against something stays behind if it has room, otherwise it holds the lift (not for brushes moved by a script). A figure that is spawned when the map loads is still falling when the cutscene starts, like in the game. `DROPTOFLOOR` changes nothing in single player.":
        "**Gravedad.** Las figuras están sobre el suelo del mapa, caen, suben escalones, se pegan a las pendientes y van en ascensores, con la `gravity` del mapa y `SET_GRAVITY`. Una puerta, un ascensor o un tren al que estorba una figura se detiene como en el juego: una puerta da la vuelta (una trituradora sigue apretando), un tren espera, y la figura recibe el daño de la puerta o del tren (`dmg`, 2 por defecto); una figura en un ascensor que la aplastaría contra algo se queda atrás si tiene sitio, si no, retiene el ascensor (no para brushes movidos por un script). Una figura que aparece al cargar el mapa todavía está cayendo cuando empieza la cinemática, como en el juego. `DROPTOFLOOR` no cambia nada en un jugador.",
    "**Figures of the map.** Every character that spawns when the map loads (an `NPC_*` entity without `targetname`) stands in the preview, also when no script names it - like the droid next to Anakin in md_am_sith. A script tab leaves out the actors of other cutscenes: a figure with `NPC_targetname` and the CINEMATIC spawnflag does nothing on its own in the game, so it shows only in the tabs whose scripts use it (in md_am_sith the credits show Anakin `ani4`, not `ani1` to `ani5` of the other scenes). An empty tab shows the map as it loads, with every figure. Classes such as `NPC_Droid_R2D2`, `NPC_Kyle` or `NPC_Stormtrooper` pick their character type themselves, as in the game; where the game chooses at random (`NPC_Stormtrooper` without spawnflags, the Movie Duels random spawners), the preview shows the first choice.":
        "**Figuras del mapa.** Cada personaje que aparece al cargar el mapa (una entidad `NPC_*` sin `targetname`) está en la vista previa, también cuando ningún script lo nombra; como el droide junto a Anakin en md_am_sith. Una pestaña de script deja fuera a los actores de otras cinemáticas: una figura con `NPC_targetname` y el spawnflag CINEMATIC no hace nada por sí sola en el juego, así que solo aparece en las pestañas cuyos scripts la usan (en md_am_sith los créditos muestran a Anakin `ani4`, no de `ani1` a `ani5` de las otras escenas). Una pestaña vacía muestra el mapa tal como se carga, con cada figura. Las clases como `NPC_Droid_R2D2`, `NPC_Kyle` o `NPC_Stormtrooper` eligen su tipo de personaje ellas mismas, como en el juego; donde el juego elige al azar (`NPC_Stormtrooper` sin spawnflags, los spawners aleatorios de Movie Duels), la vista previa muestra la primera opción.",
    "**Paths.** A figure walks straight to its target only where the game would: close by (under 110 units), between neighbouring waypoints (under 500) or with a free line (under 400). Otherwise it walks over the waypoint network, point by point - connect your targets with waypoints ([[wegpunkte]]). Around corners a walk shows as several pieces of the same command.":
        "**Trayectos.** Una figura camina en línea recta hacia su destino solo donde lo haría el juego: cerca (menos de 110 unidades), entre waypoints vecinos (menos de 500) o con la línea libre (menos de 400). Si no, camina por la red de waypoints, punto a punto; conecta tus destinos con waypoints ([[wegpunkte]]). En las esquinas, una marcha aparece como varios trozos del mismo comando.",
    "**No path.** If the network has no path, the figure stands still and a `dowait` on the walk never ends - in the game too. The status list names the figure and the target.":
        "**Sin camino.** Si la red no tiene camino, la figura se queda quieta y un `dowait` sobre la marcha nunca termina; en el juego también. La lista de estado nombra la figura y el destino.",
    "**Doors.** Doors and other moving brushes stand where the game has them at that moment. An NPC waits at a closed door and walks on when it opens. A locked door (or one only the player can open) blocks the way while it is closed. Walls and doors that a script moves (`move`, `rotate`) or removes are where the script has them. A figure standing on a lift or platform that moves rides along with it, also while it walks, like the game.":
        "**Puertas.** Las puertas y otros brushes móviles están donde los tiene el juego en ese momento. Un NPC espera ante una puerta cerrada y sigue cuando se abre. Una puerta bloqueada (o una que solo puede abrir el jugador) corta el camino mientras está cerrada. Las paredes y puertas que mueve un script (`move`, `rotate`) o que quita están donde las tiene el script. Una figura de pie sobre un ascensor o una plataforma que se mueve viaja con ella, también mientras camina, como el juego.",
    "**Ledges, curves and other NPCs.** NPCs fall from ledges and land - with the game's in-air animation on a long fall and its landing animation where the game plays one (a hard landing slows them down), also when a figure was spawned in the air; a script animation on the upper body that began before a jump does not come back after it, like the game; curved surfaces (patches) block them like walls. They do not walk through each other: if two NPCs meet, the preview shows how the game lets them pass - and the times of `dowait` follow. As in the game, a figure made invisible (`SET_INVISIBLE true`) or not solid (`SET_SOLID false`) blocks nobody, figures that stand in the map without a script do, and a killed figure stays in the way until it becomes a corpse (5 s in Movie Duels, 0.5 s in OpenJK; a Mark1 or Interrogator never does). A figure made solid again (`SET_SOLID true`) while it stands inside another one becomes solid only once it is free - a `dowait` on that `set` waits as long as in the game. `SET_ORIGIN` and `SET_ANGLES` with `tag( \"name\", ORIGIN )` or `ANGLES` put the figure at the ref_tag or navgoal of that name; without such a tag the command does nothing, like the game. A script run by an entity (a script runner, a trigger, an NPC spawner) reads that entity's `parm1` ... `parm16` keys with `get( STRING, \"SET_PARM1\" )`, and inside an `affect` block `get` reads the parms of the affected entity - so `affect ( $get( STRING, \"SET_PARM1\" )$ )` and `move ( $tag( get( STRING, \"SET_PARM1\" ), ORIGIN )$, ... )` move the entity named there to the tag named there, like the game; `move` also takes `tag( \"name\", ORIGIN )` as its destination. NPCs that know force jump (FP_LEVITATION in their .npc) jump like the game: across jump edges of the waypoint net (targetJump) and toward their goal after being blocked for 2 seconds; the preview plays the game's animations - the jump animation (as they leave the ground the legs switch to the jump pose while the upper body finishes it; a flip stays whole-body) and the landing animation, also after a `BS_JUMP` jump that only lands after `BS_DEFAULT`. After `SET_NO_ACROBATICS true` an NPC never jumps (it still takes jump edges and stops at them, like the game). In the behavior state `BS_JUMP` an NPC turns to its navgoal, crouches, jumps there in one arc and lands, like the game; a `dowait` on the navgoal ends after the landing animation once it has reached the goal (if it lands short, it jumps again). The arc is the game's own: it falls short of a navgoal above the NPC and goes beyond one below it, and a navgoal that hangs in the air is only the point it aims at.":
        "**Bordes, curvas y otros NPC.** Los NPC caen de los bordes y aterrizan, con la animación en el aire del juego en una caída larga y su animación de aterrizaje donde el juego reproduce una (un aterrizaje duro los frena), también cuando una figura apareció en el aire; una animación de script en la parte superior del cuerpo que empezó antes de un salto no vuelve después, como en el juego; las superficies curvas (patches) los bloquean como paredes. No se atraviesan: si dos NPC se encuentran, la vista previa muestra cómo los deja pasar el juego, y los tiempos de `dowait` lo siguen. Como en el juego, una figura hecha invisible (`SET_INVISIBLE true`) o no sólida (`SET_SOLID false`) no bloquea a nadie, las figuras que están en el mapa sin script sí, y una figura muerta estorba hasta convertirse en cadáver (5 s en Movie Duels, 0,5 s en OpenJK; un Mark1 o un Interrogator nunca). Una figura que vuelve a ser sólida (`SET_SOLID true`) mientras está dentro de otra solo se vuelve sólida cuando queda libre: un `dowait` sobre ese `set` espera tanto como en el juego. `SET_ORIGIN` y `SET_ANGLES` con `tag( \"nombre\", ORIGIN )` o `ANGLES` ponen la figura en el ref_tag o navgoal de ese nombre; sin ese tag el comando no hace nada, como el juego. Un script ejecutado por una entidad (un ejecutor de scripts, un trigger, un NPC spawner) lee las claves `parm1` ... `parm16` de esa entidad con `get( STRING, \"SET_PARM1\" )`, y dentro de un bloque `affect` `get` lee los parms de la entidad afectada; así `affect ( $get( STRING, \"SET_PARM1\" )$ )` y `move ( $tag( get( STRING, \"SET_PARM1\" ), ORIGIN )$, ... )` mueven la entidad nombrada ahí al tag nombrado ahí, como el juego; `move` también acepta `tag( \"nombre\", ORIGIN )` como destino. Los NPC que conocen el salto de Fuerza (FP_LEVITATION en su .npc) saltan como en el juego: por los bordes de salto de la red de waypoints (targetJump) y hacia su objetivo tras estar bloqueados 2 segundos; la vista previa reproduce las animaciones del juego: la animación de salto (al dejar el suelo, las piernas pasan a la pose de salto mientras la parte superior la termina; una voltereta sigue siendo de cuerpo entero) y la animación de aterrizaje, también tras un salto `BS_JUMP` que solo aterriza después de `BS_DEFAULT`. Tras `SET_NO_ACROBATICS true` un NPC nunca salta (sigue tomando bordes de salto y se para en ellos, como en el juego). En el estado de comportamiento `BS_JUMP` un NPC se gira hacia su navgoal, se agacha, salta hasta allí en un arco y aterriza, como en el juego; un `dowait` sobre el navgoal termina tras la animación de aterrizaje una vez alcanzado el objetivo (si se queda corto, vuelve a saltar). El arco es el del juego: se queda corto ante un navgoal por encima del NPC y se pasa en uno por debajo, y un navgoal que cuelga en el aire es solo el punto al que apunta.",
    "**Deadly areas.** A figure that walks or falls into a `trigger_hurt` takes its damage like in the game (`dmg`, `delay`, `wait`, the spawnflags; a `use` switches the area on or off) and dies when its health runs out: it plays its death, stays where it fell and its `deathscript` runs. The health is the one the game gives it (the `health` key, otherwise the .npc file and the difficulty), `SET_HEALTH` changes it, armour (`SET_ARMOR`, the assassin droid's and droideka's own) takes its share like the game's armour (not in a fall), a rancor takes at most 10 per hit, `SET_INVINCIBLE true` protects it, and with `SET_UNDYING true` it keeps 1 health and its `deathscript` runs again with every hit that would kill it. Exploding laser trip mines hurt figures in the same way. The player is not hurt in the preview.":
        "**Zonas mortales.** Una figura que camina o cae en un `trigger_hurt` recibe su daño como en el juego (`dmg`, `delay`, `wait`, los spawnflags; un `use` activa o desactiva la zona) y muere cuando se le acaba la salud: reproduce su muerte, se queda donde cayó y se ejecuta su `deathscript`. La salud es la que le da el juego (la clave `health`, si no el archivo .npc y la dificultad), `SET_HEALTH` la cambia, la armadura (`SET_ARMOR`, la propia del droide asesino y del droideka) se lleva su parte como la armadura del juego (no en una caída), un rancor recibe como máximo 10 por golpe, `SET_INVINCIBLE true` lo protege, y con `SET_UNDYING true` conserva 1 de salud y su `deathscript` vuelve a ejecutarse con cada golpe que lo mataría. Las minas láser que explotan hieren a las figuras del mismo modo. El jugador no sufre daño en la vista previa.",
    "**Level change.** When a script uses a `target_level_change` (directly or through a relay), the game loads the next map after that frame. The preview ends there too: the timeline, the camera and the figures stop at that moment, and nothing the script would do later happens. The status list names the time and the next map. A map name starting with `+` (OpenJK and Movie Duels, for example `+saberMenu` or `+creditsingame`) loads no map: the game opens that menu and pauses until the player closes it, so the preview goes on and the status list names the menu.":
        "**Cambio de nivel.** Cuando un script usa un `target_level_change` (directamente o a través de un relé), el juego carga el mapa siguiente tras ese fotograma. La vista previa termina ahí también: la línea de tiempo, la cámara y las figuras se paran en ese momento y no ocurre nada de lo que el script haría después. La lista de estado nombra el momento y el mapa siguiente. Un nombre de mapa que empieza por `+` (OpenJK y Movie Duels, por ejemplo `+saberMenu` o `+creditsingame`) no carga ningún mapa: el juego abre ese menú y se pausa hasta que el jugador lo cierra, así que la vista previa sigue y la lista de estado nombra el menú.",
    "**Mission failed.** `SET_MISSIONFAILED` makes the player die and shows the game's mission-failed screen. The game then stops every script as soon as no camera is running (right after that frame, or after the cutscene's `camera ( DISABLE )`); the world goes on. The preview does the same: script commands after that moment do not happen, figures already walking finish their walk, the player plays his guilt animation, and the status list names both times.":
        "**Misión fracasada.** `SET_MISSIONFAILED` hace morir al jugador y muestra la pantalla de misión fracasada del juego. El juego detiene entonces cada script en cuanto no hay ninguna cámara en marcha (justo tras ese fotograma, o tras el `camera ( DISABLE )` de la cinemática); el mundo sigue. La vista previa hace lo mismo: los comandos de script tras ese momento no ocurren, las figuras que ya caminan terminan su marcha, el jugador reproduce su animación de culpa y la lista de estado nombra ambos momentos.",
    "**Weapon poses.** An NPC with a gun in its hand (not a lightsaber) holds it with its upper body like the game whenever no script animation plays there: under OpenJK and Jedi Outcast a rifle at the ready while it stands and walks (`TORSO_WEAPONIDLE3`), a pistol in front while it stands, and the arms swing with the legs while it runs; under Movie Duels a rifle lowered (`TORSO_WEAPONREST3`) or carried after `SET_WALKING true` (`TORSO_WEAPONIDLE3`). A script animation on the upper or the whole body wins while its timer runs.":
        "**Poses de arma.** Un NPC con un arma de fuego en la mano (no un sable láser) la sostiene con la parte superior del cuerpo como en el juego siempre que no se reproduzca ahí ninguna animación de script: con OpenJK y Jedi Outcast, un rifle preparado mientras está de pie y camina (`TORSO_WEAPONIDLE3`), una pistola delante mientras está de pie, y los brazos se balancean con las piernas mientras corre; con Movie Duels, un rifle bajado (`TORSO_WEAPONREST3`) o llevado tras `SET_WALKING true` (`TORSO_WEAPONIDLE3`). Una animación de script en la parte superior o en todo el cuerpo gana mientras corre su temporizador.",
    "**BS_NOCLIP.** With `SET_BEHAVIOR_STATE BS_NOCLIP` a figure flies straight through walls to its goal, with the speed it has at that moment. Set `SET_WALKSPEED` or `SET_RUNSPEED` after `BS_NOCLIP`, otherwise a figure that has not walked yet does not move (and a `dowait` hangs).":
        "**BS_NOCLIP.** Con `SET_BEHAVIOR_STATE BS_NOCLIP` una figura vuela en línea recta a través de las paredes hasta su objetivo, con la velocidad que tenga en ese momento. Fija `SET_WALKSPEED` o `SET_RUNSPEED` tras `BS_NOCLIP`; si no, una figura que aún no ha caminado no se mueve (y un `dowait` se queda colgado).",
    "**OpenJK or Movie Duels.** Under OpenJK rules NPCs re-steer and turn every 100 ms (higher-ranked saber users from {{DiffKnight}} on every 50 ms) - turning therefore takes about twice as long as under Movie Duels, where both happen every 50 ms - and the turn rates per difficulty differ, so a walk can end a little differently. Jedi Outcast missions follow the same 100 ms rhythm as OpenJK, also when an NPC turns on the spot. Choose the rules in [[einstellungen#prefs]].":
        "**OpenJK o Movie Duels.** Con las reglas de OpenJK los NPC corrigen el rumbo y giran cada 100 ms (los usuarios de sable de mayor rango, a partir de {{DiffKnight}}, cada 50 ms): por eso girar tarda aproximadamente el doble que con Movie Duels, donde ambas cosas ocurren cada 50 ms, y las velocidades de giro por dificultad son distintas, así que una marcha puede terminar un poco distinta. Las misiones de Jedi Outcast siguen el mismo ritmo de 100 ms que OpenJK, también cuando un NPC gira en el sitio. Elige las reglas en [[einstellungen#prefs]].",
    "Lightsabers: walls and clashes":
        "Sables láser: paredes y choques",
    "Lightsabers behave like in the game. A blade that reaches into a wall, the floor or a door is shortened to the surface and grows back; sparks fly and it sounds while it touches, and where it slides along the wall it burns a mark that glows briefly and fades after ten seconds. In water it boils. Sabers whose .sab file says `noIdleEffect` stay quiet while the figure only stands. Only figures the camera can see make sparks, marks and sounds - the game, too, only handles what is in the camera's view. Jumping on the timeline shows the same as playing through, and dragging the playhead backwards is as smooth as forwards: what has been computed once stays in memory (up to ten minutes of a scene).":
        "Los sables láser se comportan como en el juego. Una hoja que entra en una pared, en el suelo o en una puerta se acorta hasta la superficie y vuelve a crecer; saltan chispas y suena mientras toca, y donde se desliza por la pared quema una marca que brilla un momento y se desvanece a los diez segundos. En el agua hierve. Los sables cuyo archivo .sab dice `noIdleEffect` se quedan en silencio mientras la figura solo está de pie. Solo las figuras que ve la cámara hacen chispas, marcas y sonidos: el juego también solo procesa lo que está en la vista de la cámara. Saltar en la línea de tiempo muestra lo mismo que reproducir, y arrastrar el cabezal hacia atrás es tan fluido como hacia delante: lo calculado una vez se queda en memoria (hasta diez minutos de una escena).",
    "Blades of two figures clash only where the game lets them. **Movie Duels:** both NPCs must be in `BS_CINEMATIC` (`set ( SET_BEHAVIOR_STATE, BS_CINEMATIC )`, or a spawner with the CINEMATIC spawnflag), and the attacker needs `set ( SET_SABERCOLLISIONS, true )`; `false` switches it off again. **OpenJK** has no `SET_SABERCOLLISIONS`: there blades clash only in the animations `BOTH_CIN_16` and `BOTH_CIN_17` of figures in `BS_CINEMATIC` (Movie Duels allows these two as well). A hit plays the block spark, one of that game's clash sounds and the clash flare. Fights the AI would fight on its own (`SET_ENEMY`, `BS_DEFAULT`) are not simulated - only what the script animates. In Movie Duels the fast saber style also ignites the blade twice as fast. The rules are chosen in [[einstellungen#prefs]].":
        "Las hojas de dos figuras solo chocan donde lo permite el juego. **Movie Duels:** los dos NPC deben estar en `BS_CINEMATIC` (`set ( SET_BEHAVIOR_STATE, BS_CINEMATIC )`, o un spawner con el spawnflag CINEMATIC), y el atacante necesita `set ( SET_SABERCOLLISIONS, true )`; `false` lo vuelve a desactivar. **OpenJK** no tiene `SET_SABERCOLLISIONS`: ahí las hojas solo chocan en las animaciones `BOTH_CIN_16` y `BOTH_CIN_17` de figuras en `BS_CINEMATIC` (Movie Duels también permite estas dos). Un golpe reproduce la chispa de bloqueo, uno de los sonidos de choque de ese juego y el destello del choque. Los combates que la IA libraría por su cuenta (`SET_ENEMY`, `BS_DEFAULT`) no se simulan: solo lo que anima el script. En Movie Duels el estilo de sable rápido también enciende la hoja el doble de rápido. Las reglas se eligen en [[einstellungen#prefs]].",
    "Animating movers and ROFF files":
        "Animar móviles y archivos ROFF",
    "Ships, doors and platforms move by keys over time. Store them as move commands in the script, or as a ROFF file - and edit the ROFF files of the game.":
        "Las naves, puertas y plataformas se mueven con claves en el tiempo. Guárdalas como comandos move en el script, o como archivo ROFF, y edita los archivos ROFF del juego.",
    "The {{AnimTitle}} window in ROFF mode and the path of the ship on the map.":
        "La ventana {{AnimTitle}} en modo ROFF y el trayecto de la nave en el mapa.",
    "Opening the window":
        "Abrir la ventana",
    "Select a movable entity - `func_*` brush entities such as `func_static` ships and `func_door`, `misc_model_breakable`, `fx_runner` - and press **{{EntAnimate}}** in the inspector. If it has no `script_targetname` yet, it gets one, because a script can only move what has a name.":
        "Selecciona una entidad móvil (entidades de brushes `func_*` como las naves `func_static` y `func_door`, `misc_model_breakable`, `fx_runner`) y pulsa **{{EntAnimate}}** en el inspector. Si aún no tiene `script_targetname`, recibe uno, porque un script solo puede mover lo que tiene nombre.",
    "Keys":
        "Claves",
    "Put the timeline where the movement starts and press **{{AnimAddKey}}** - the first key stores where the entity stands.":
        "Pon la línea de tiempo donde empieza el movimiento y pulsa **{{AnimAddKey}}**: la primera clave guarda dónde está la entidad.",
    "Move on in time and press it again. Then drag the new diamond in the view (<kbd>Shift</kbd>: only up and down), or type values into the table: time, position, direction.":
        "Avanza en el tiempo y vuelve a pulsarlo. Luego arrastra el rombo nuevo en la vista (<kbd>Mayús</kbd>: solo arriba y abajo), o escribe valores en la tabla: tiempo, posición, dirección.",
    "Or simply use the gizmo: while the window shows this entity and the timeline stands at or after the first key, moving or rotating it with the gizmo sets a key at the playhead (or changes the one that is there).":
        "O usa simplemente el gizmo: mientras la ventana muestra esta entidad y la línea de tiempo está en la primera clave o después, moverla o girarla con el gizmo pone una clave en el cabezal (o cambia la que ya hay).",
    "**{{AnimDeleteKey}}** and **{{AnimGotoKey}}** work on the selected row; **{{AnimRemove}}** takes the whole animation out of the script.":
        "**{{AnimDeleteKey}}** y **{{AnimGotoKey}}** actúan sobre la fila seleccionada; **{{AnimRemove}}** saca toda la animación del script.",
    "Script (move) or ROFF file":
        "Script (move) o archivo ROFF",
    "Stored as":
        "Se guarda como",
    "an `affect` block with `move` and `wait` at the start of the script":
        "un bloque `affect` con `move` y `wait` al principio del script",
    "`scripts/<name>.rof`, played by `play ( \"PLAY_ROFF\", ... )`":
        "`scripts/<nombre>.rof`, reproducido por `play ( \"PLAY_ROFF\", ... )`",
    "Between keys":
        "Entre claves",
    "each move starts fast and slows down at the end, like the game's movers (or constant with {{AnimLinear}})":
        "cada movimiento empieza rápido y frena al final, como los móviles del juego (o constante con {{AnimLinear}})",
    "one smooth curve through all keys ({{AnimRoffSmooth}}) or straight lines":
        "una curva suave por todas las claves ({{AnimRoffSmooth}}) o líneas rectas",
    "Good for":
        "Bueno para",
    "doors, lifts, a ship crossing the screen":
        "puertas, ascensores, una nave que cruza la pantalla",
    "long free flights with many turns, effects at exact frames":
        "vuelos libres largos con muchos giros, efectos en fotogramas exactos",
    "Switch with **{{AnimModeMove}}** / **{{AnimModeRoff}}** at the top of the window - the keys are converted, nothing is lost. In ROFF mode the field **{{AnimRoffName}}** names the file (for example `md_am/roffs/sta2`); below it you see how many frames and seconds it has.":
        "Cambia con **{{AnimModeMove}}** / **{{AnimModeRoff}}** arriba de la ventana: las claves se convierten, no se pierde nada. En el modo ROFF el campo **{{AnimRoffName}}** nombra el archivo (por ejemplo `md_am/roffs/sta2`); debajo ves cuántos fotogramas y segundos tiene.",
    "How ROFF files work":
        "Cómo funcionan los archivos ROFF",
    "A ROFF file (`.rof`, \"Rotation and Origin File Format\") stores one small step per game frame: how far the entity moves and turns in the next 50 ms. The game adds these steps to wherever the entity stands when `PLAY_ROFF` runs - so a ROFF path is **relative**: move the entity and the whole path moves with it. All 108 files of the game and Movie Duels are version 2, all but one with 50 ms per frame - and that is what JK Studio writes.":
        "Un archivo ROFF (`.rof`, \"Rotation and Origin File Format\") guarda un pequeño paso por fotograma de juego: cuánto se mueve y gira la entidad en los 50 ms siguientes. El juego suma estos pasos a donde esté la entidad cuando se ejecuta `PLAY_ROFF`; por eso un trayecto ROFF es **relativo**: mueve la entidad y todo el trayecto se mueve con ella. Los 108 archivos del juego y de Movie Duels son de la versión 2, todos menos uno con 50 ms por fotograma, y eso es lo que escribe JK Studio.",
    "The keys you edit are kept as a comment line in the script right before the `play`; the `.rof` is built from them. That is why undo, copy and paste work on ROFF paths like on any command. When you **save the script**, the file is written to `scripts/<name>.rof` in the same scripts folder; **{{MissionSave}}** packs it into the mission's `.pk3`.":
        "Las claves que editas se guardan como una línea de comentario en el script justo antes del `play`; el `.rof` se construye a partir de ellas. Por eso deshacer, copiar y pegar funcionan en los trayectos ROFF como en cualquier comando. Al **guardar el script**, el archivo se escribe en `scripts/<nombre>.rof` en la misma carpeta de scripts; **{{MissionSave}}** lo empaqueta en el `.pk3` de la misión.",
    "Editing a ROFF file of the game":
        "Editar un archivo ROFF del juego",
    "When a script plays a finished ROFF on the entity, the window lists it under **{{AnimRoffPlays}}**. **{{AnimRoffEdit}}** turns it into keys - either straight pieces or one smooth curve, whichever needs fewer keys - within half a unit of the original. The game's file is never overwritten: you edit a copy (`…_2`), and the `play` command is pointed at it.":
        "Cuando un script reproduce un ROFF terminado en la entidad, la ventana lo lista en **{{AnimRoffPlays}}**. **{{AnimRoffEdit}}** lo convierte en claves (o trozos rectos o una curva suave, lo que necesite menos claves) a menos de media unidad del original. El archivo del juego nunca se sobrescribe: editas una copia (`…_2`) y el comando `play` pasa a apuntar a ella.",
    "A ship flying its ROFF path.":
        "Una nave volando su trayecto ROFF.",
    "The map":
        "El mapa",
    "The map in 3D, drawn like the game draws it - with lights, glow, fog, effects and the figures of the script.":
        "El mapa en 3D, dibujado como lo dibuja el juego: con luces, resplandor, niebla, efectos y las figuras del script.",
    "The map view with its toolbar on top and the placement tiles on the left.":
        "La vista del mapa con su barra arriba y las fichas de colocación a la izquierda.",
    "Opening a map":
        "Abrir un mapa",
    "Switch to **{{ViewMap}}** in the mode bar. A mission brings its map along ([[missionen]]). Without a mission, **{{MapMore}}** opens a map from disk or from a `.pk3`, or loads a separate `.ent` file. **Scripts (N)** lists the scripts the map's entities point to - pick one to load its cutscene.":
        "Cambia a **{{ViewMap}}** en la barra de modos. Una misión trae su mapa ([[missionen]]). Sin misión, **{{MapMore}}** abre un mapa del disco o de un `.pk3`, o carga un archivo `.ent` aparte. **Scripts (N)** lista los scripts a los que apuntan las entidades del mapa: elige uno para cargar su cinemática.",
    "Moving around":
        "Moverse",
    "W A S D (mouse over the view)":
        "W A S D (ratón sobre la vista)",
    "Fly; <kbd>Q</kbd> / <kbd>E</kbd> down and up, <kbd>Shift</kbd> faster":
        "Volar; <kbd>Q</kbd> / <kbd>E</kbd> bajar y subir, <kbd>Mayús</kbd> más rápido",
    "Right mouse drag":
        "Arrastrar con el botón derecho",
    "Look around":
        "Mirar alrededor",
    "Middle mouse drag":
        "Arrastrar con el botón central",
    "Pan":
        "Desplazar",
    "Alt + middle mouse":
        "Alt + botón central",
    "Orbit":
        "Orbitar",
    "Wheel":
        "Rueda",
    "Zoom":
        "Zoom",
    "Double-click an entity in the list":
        "Doble clic en una entidad de la lista",
    "Fly there":
        "Volar hasta allí",
    "Ctrl + 1 ... 9":
        "Ctrl + 1 ... 9",
    "Save the view; <kbd>1</kbd> ... <kbd>9</kbd> jumps back (kept per map)":
        "Guardar la vista; <kbd>1</kbd> ... <kbd>9</kbd> vuelve a ella (se guarda por mapa)",
    "What is shown":
        "Qué se muestra",
    "The buttons on the right of the toolbar switch parts of the picture on and off: entities, names, the figures of the script, effects, glow, dynamic light, fog and sky. **{{MapTopView}}** looks straight down like a map and cuts away everything above the cut height - drag the height box to look at another floor. **{{MapViewSettings}}** (gear) holds brightness, ambient light, detail, character shadows, **{{MapScriptFocus}}**, which dims every entity the active script does not use, and **{{GameScreen}}**: the game's aspect ratio and cg_fovAspectAdjust, which decide how wide every camera sees ([[kamera#vorschau]]).":
        "Los botones a la derecha de la barra activan y desactivan partes de la imagen: entidades, nombres, las figuras del script, efectos, resplandor, luz dinámica, niebla y cielo. **{{MapTopView}}** mira hacia abajo en vertical como un plano y recorta todo lo que está por encima de la altura de corte; arrastra el cuadro de altura para ver otra planta. **{{MapViewSettings}}** (engranaje) contiene brillo, luz ambiental, detalle, sombras de personajes, **{{MapScriptFocus}}**, que atenúa todas las entidades que el script activo no usa, y **{{GameScreen}}**: la relación de aspecto del juego y cg_fovAspectAdjust, que deciden cuánto ve cada cámara ([[kamera#vorschau]]).",
    "The picture is meant to look like the game: shaders, lightmaps, glow (like `r_DynamicGlow` - on the map, on brush entities and models, on the figures' glowing parts such as droid eyes, on effects like fire and sparks and around lightsaber blades), fog (`r_drawfog`), dynamic lights, lens flares, grass, bushes and rain splashes on the ground (`surfaceSprites` - blade for blade where the game puts them, swaying and fading out with distance, also on brush entities such as a wall that appears when it is used; rain splashes only while it rains), weather (under Movie Duels also the lava of fx_lava) and saber trails, sparks and burn marks ([[figuren#schwerter]]). Weapons, medpacks, shields and ammo lie where the game drops them (items with a `targetname` appear only when used), and floor charging stations, security panels, wall cameras, turrets, emplaced guns, E-Webs, sentry guns and the ion cannon stand with the models the game gives them (without a player they do not aim or fire; a sentry gun unfolds one second after the map starts or after it is switched on and then slowly sweeps from side to side like the game's sentry without a target, and the ion cannon fires on its own - or after a use - with its muzzle flash and recoil, at twice its model size like in the game); laser trip mines hang on their wall and, when switched on, draw their beam and hum (with the warning sound when they arm); a script or trigger switches them on or off, the beam stops at doors and other moving parts where they are at that moment, at other trip mines and at solid breakable models, and a figure walking into the beam - or only the weapon in its hand - sets the mine off - explosion, then the mine is gone; like the game's blast damage it hurts the figures near it (the closer, the more; not through walls), and a figure whose health runs out dies like in a deadly area ([[figuren]]); like in the game the mine uses its targets, so doors, walls or a counter behind it switch, and scripts behind it run (the usescript of a counter, a script runner) - with one count for scripts and movers. An exploding mine also sets off the switched-on mines near it (mines hung close together go off together). Figures show every stage of their shader - shine, chrome (also on the weapon or saber hilt in their hand, turning with the figure as in the game), tinted parts such as the battle droids' paint, the black visors of the clone troopers, blinking and animated stages, moving texture stages (tcMod) such as the shimmer of the Emperor's hologram, and surfaces their shader moves with `deformVertexes` wave, bulge or move, such as the smoke around the shadow Yoda (md_yoda_shadow). Effects are drawn with every stage of their shader too - an explosion blends its two animated stages as in the game, and every stage, the first one included, takes the particle's colour and fading only where its shader asks for it (`rgbGen vertex`, `alphaGen vertex`); otherwise it shows its image as it is, like the game; an effect image without a shader script is drawn like the game's built-in shader for it - blended by its alpha with the particle's colour and fading, and visible through walls. Models that effects carry (`useModel`, such as the fog wisps on duel_exegol) are drawn too and turn towards the camera where their shader says `deformVertexes autosprite`. Beams of fx_target_beam fire towards their target like in the game, also when a script or trigger switches them on or off (a one-shot beam fires once per use); each shot aims at where the target is at that moment - a beam on a moving train follows it -, stops at doors, moving parts and figures in its way, and a beam switched back on waits out its pause (`wait`) like the game. **{{MapStats}}** shows frame rate, textures and graphics card values when something looks wrong.":
        "La imagen quiere parecerse al juego: shaders, lightmaps, resplandor (como `r_DynamicGlow`: en el mapa, en entidades de brushes y modelos, en las partes brillantes de las figuras como los ojos de los droides, en efectos como el fuego y las chispas y alrededor de las hojas de los sables láser), niebla (`r_drawfog`), luces dinámicas, destellos de lente, hierba, arbustos y salpicaduras de lluvia en el suelo (`surfaceSprites`: brizna a brizna donde los pone el juego, meciéndose y desvaneciéndose con la distancia, también en entidades de brushes como una pared que aparece al usarse; salpicaduras de lluvia solo mientras llueve), el clima (con Movie Duels también la lava de fx_lava) y las estelas, chispas y quemaduras de los sables ([[figuren#schwerter]]). Las armas, botiquines, escudos y munición están donde los deja el juego (los objetos con `targetname` solo aparecen al usarse), y las estaciones de recarga de suelo, paneles de seguridad, cámaras de pared, torretas, armas emplazadas, E-Webs, torretas centinela y el cañón de iones están con los modelos que les da el juego (sin jugador no apuntan ni disparan; una torreta centinela se despliega un segundo después de empezar el mapa o de activarse y luego barre lentamente de lado a lado como la centinela del juego sin objetivo, y el cañón de iones dispara solo, o tras un use, con su fogonazo y su retroceso, al doble del tamaño de su modelo como en el juego); las minas láser cuelgan de su pared y, activadas, dibujan su rayo y zumban (con el sonido de aviso al armarse); un script o un trigger las activa o desactiva, el rayo se detiene en las puertas y otras partes móviles donde estén en ese momento, en otras minas láser y en modelos rompibles sólidos, y una figura que entra en el rayo (o solo el arma de su mano) hace estallar la mina: explosión, y la mina desaparece; como el daño de explosión del juego, hiere a las figuras cercanas (cuanto más cerca, más; no a través de paredes), y una figura a la que se le acaba la salud muere como en una zona mortal ([[figuren]]); como en el juego, la mina usa sus destinos, así que las puertas, paredes o contadores tras ella cambian y los scripts tras ella se ejecutan (el usescript de un contador, un ejecutor de scripts), con un solo conteo para scripts y móviles. Una mina que estalla hace estallar también las minas activadas cercanas (las minas colgadas juntas estallan juntas). Las figuras muestran cada etapa de su shader: brillo, cromado (también en el arma o la empuñadura del sable de su mano, girando con la figura como en el juego), partes teñidas como la pintura de los droides de combate, los visores negros de los soldados clon, etapas parpadeantes y animadas, etapas de textura móviles (tcMod) como el brillo del holograma del Emperador, y superficies que su shader mueve con `deformVertexes` wave, bulge o move, como el humo alrededor del Yoda sombra (md_yoda_shadow). Los efectos también se dibujan con cada etapa de su shader: una explosión mezcla sus dos etapas animadas como en el juego, y cada etapa, incluida la primera, toma el color y el desvanecimiento de la partícula solo donde su shader lo pide (`rgbGen vertex`, `alphaGen vertex`); si no, muestra su imagen tal cual, como el juego; una imagen de efecto sin script de shader se dibuja como el shader integrado del juego para ella: mezclada por su alfa con el color y el desvanecimiento de la partícula, y visible a través de las paredes. Los modelos que llevan los efectos (`useModel`, como las volutas de niebla de duel_exegol) también se dibujan y se giran hacia la cámara donde su shader dice `deformVertexes autosprite`. Los rayos de fx_target_beam disparan hacia su objetivo como en el juego, también cuando un script o un trigger los activa o desactiva (un rayo de un solo disparo dispara una vez por use); cada disparo apunta adonde está el objetivo en ese momento (un rayo en un tren en marcha lo sigue), se detiene en las puertas, partes móviles y figuras que haya en medio, y un rayo que se vuelve a activar espera su pausa (`wait`) como el juego. **{{MapStats}}** muestra fotogramas por segundo, texturas y valores de la tarjeta gráfica cuando algo se ve mal.",
    "Entities":
        "Entidades",
    "Everything in a map that a script can talk to - figures, walk targets, camera marks, triggers, ships - is an entity. Select it, change it, move it, place new ones.":
        "Todo lo que hay en un mapa con lo que un script puede hablar (figuras, destinos, marcas de cámara, triggers, naves) es una entidad. Selecciónala, cámbiala, muévela, coloca nuevas.",
    "A selected ship: the list on the left, the inspector below it, the gizmo in the view.":
        "Una nave seleccionada: la lista a la izquierda, el inspector debajo, el gizmo en la vista.",
    "The entity list":
        "La lista de entidades",
    "The left column sorts the entities into groups - camera marks, walk targets, look targets, characters, script runners, effects, sound, movables, breakables, NPC waypoints. The eye hides a group in the view (<kbd>Alt</kbd>+click: show only this one), the search box filters by name or class. Click selects, double-click flies there, right-click inserts a command for it: walk target, look target, affect block, camera to this mark.":
        "La columna izquierda ordena las entidades en grupos: marcas de cámara, destinos, objetivos de mirada, personajes, ejecutores de scripts, efectos, sonido, móviles, rompibles, waypoints de NPC. El ojo oculta un grupo en la vista (<kbd>Alt</kbd>+clic: mostrar solo este), el campo de búsqueda filtra por nombre o clase. Clic selecciona, doble clic vuela hasta allí, clic derecho inserta un comando para ella: destino, objetivo de mirada, bloque affect, cámara a esta marca.",
    "Selecting in the view":
        "Seleccionar en la vista",
    "Click a marker, a figure or a model to select it; <kbd>Ctrl</kbd>/<kbd>Shift</kbd>+click adds more; a box dragged over empty space selects everything inside. Hovering shows a box and the name. The transport bar under the map says how often the selected entity is used in the script, with buttons that jump to those lines.":
        "Pulsa un marcador, una figura o un modelo para seleccionarlo; <kbd>Ctrl</kbd>/<kbd>Mayús</kbd>+clic añade más; un cuadro arrastrado sobre un espacio vacío selecciona todo lo que queda dentro. Al pasar el ratón se muestran un cuadro y el nombre. La barra de transporte bajo el mapa dice cuántas veces se usa en el script la entidad seleccionada, con botones que saltan a esas líneas.",
    "Move and Rotate":
        "Mover y Rotar",
    "A small bar in the corner of the view switches the gizmo between **{{GizmoMove}}** and **{{GizmoRotate}}**. In Move you drag the entity over the floor or along one of the coloured arrows; the grid set in the toolbar snaps, <kbd>Ctrl</kbd> turns it off while you drag - without a grid, <kbd>Ctrl</kbd> snaps an arrow drag to 8 units. In Rotate you get rings (<kbd>Ctrl</kbd>: 15-degree steps): blue turns left and right; ships and models also get green (pitch) and red (roll); figures only have the blue ring, because the game only knows `angle` for them. <kbd>Alt</kbd>+drag copies first. Several selected entities move and turn as a group.":
        "Una barra pequeña en la esquina de la vista cambia el gizmo entre **{{GizmoMove}}** y **{{GizmoRotate}}**. En Mover arrastras la entidad por el suelo o a lo largo de una de las flechas de color; la rejilla fijada en la barra ajusta, <kbd>Ctrl</kbd> la desactiva mientras arrastras; sin rejilla, <kbd>Ctrl</kbd> ajusta a 8 unidades al arrastrar una flecha. En Rotar tienes anillos (<kbd>Ctrl</kbd>: pasos de 15 grados): el azul gira a izquierda y derecha; las naves y modelos tienen además el verde (cabeceo) y el rojo (alabeo); las figuras solo tienen el anillo azul, porque el juego solo conoce `angle` para ellas. <kbd>Alt</kbd>+arrastrar copia primero. Varias entidades seleccionadas se mueven y giran como un grupo.",
    "{{GizmoMove}}: arrows only.":
        "{{GizmoMove}}: solo flechas.",
    "{{GizmoRotate}}: three rings on a ship.":
        "{{GizmoRotate}}: tres anillos en una nave.",
    "When the timeline stands in the middle of a movement, the gizmo does not change the start - it sets a new point at that moment. See [[figuren#griffe]] and [[animation#schluessel]].":
        "Cuando la línea de tiempo está en mitad de un movimiento, el gizmo no cambia el inicio: pone un punto nuevo en ese momento. Mira [[figuren#griffe]] y [[animation#schluessel]].",
    "The inspector":
        "El inspector",
    "Under the list the inspector shows the selected entity: its class with a description from the game code, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (how high the feet stand above the floor - red when inside it) and **{{EntAngle}}**, the spawnflags as checkboxes and all other keys as fields. Script fields have **{{EntOpenScript}}**, `NPC_type` has a search over every NPC of the game; when the class sets the type itself (`NPC_Droid_R2D2` → `r2d2`), the empty field shows it in grey, and a note says when the game chooses at random or ignores the key. The icon bar flies there, puts it **{{EntDropFloor}}**, duplicates, deletes, animates ([[animation]]) and shows **{{EntRaw}}**. Entity changes have their own undo while the mouse is over the map or the list.":
        "Bajo la lista, el inspector muestra la entidad seleccionada: su clase con una descripción del código del juego, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (a qué altura sobre el suelo están los pies; en rojo si están dentro) y **{{EntAngle}}**, los spawnflags como casillas y todas las demás claves como campos. Los campos de script tienen **{{EntOpenScript}}**, `NPC_type` tiene una búsqueda entre todos los NPC del juego; cuando la clase fija el tipo ella misma (`NPC_Droid_R2D2` → `r2d2`), el campo vacío lo muestra en gris, y una nota dice cuándo el juego elige al azar o ignora la clave. La barra de iconos vuela hasta allí, la pone **{{EntDropFloor}}**, duplica, elimina, anima ([[animation]]) y muestra **{{EntRaw}}**. Los cambios de entidades tienen su propio deshacer mientras el ratón está sobre el mapa o la lista.",
    "Placing new entities":
        "Colocar entidades nuevas",
    "The tiles on the left edge of the view place the common kinds: character, walk target, camera/look mark, look target, script runner, effect, sound, NPC waypoint, player start. Click a tile, then click in the view - or drag the tile into the view; <kbd>Shift</kbd> places several. **{{EntNew}}** above the list and a right-click in the view (**{{EntNewHere}}**) offer every class. The **Grid** button snaps to 8, 16, 32 or 64 units.":
        "Las fichas del borde izquierdo de la vista colocan los tipos habituales: personaje, destino, marca de cámara/mirada, objetivo de mirada, ejecutor de scripts, efecto, sonido, waypoint de NPC, inicio del jugador. Pulsa una ficha y luego pulsa en la vista, o arrastra la ficha a la vista; <kbd>Mayús</kbd> coloca varias. **{{EntNew}}** encima de la lista y un clic derecho en la vista (**{{EntNewHere}}**) ofrecen todas las clases. El botón **Rejilla** ajusta a 8, 16, 32 o 64 unidades.",
    "NPC waypoints":
        "Waypoints de NPC",
    "The network NPCs walk on - to far walk targets in cutscenes, and when they chase or flee. Place it by hand or let the program build it, then check it.":
        "La red por la que caminan los NPC: a destinos lejanos en las cinemáticas, y cuando persiguen o huyen. Colócala a mano o deja que la construya el programa, y luego compruébala.",
    "The {{WegTitel}} window and the network on the map.":
        "La ventana {{WegTitel}} y la red en el mapa.",
    "Open it with **{{WegTitel}}** in the map toolbar. **{{WegZeigen}}** draws the paths: green set by hand (`target`), blue automatic (nav goals and combat points), orange jumps (`targetJump`).":
        "Ábrela con **{{WegTitel}}** en la barra del mapa. **{{WegZeigen}}** dibuja los caminos: en verde los puestos a mano (`target`), en azul los automáticos (nav goals y puntos de combate), en naranja los saltos (`targetJump`).",
    "**{{WegSetzen}}**: click the floor - each new point is connected to the one before. <kbd>Shift</kbd>+click starts a new path, <kbd>Esc</kbd> ends.":
        "**{{WegSetzen}}**: pulsa en el suelo; cada punto nuevo se conecta con el anterior. <kbd>Mayús</kbd>+clic empieza un camino nuevo, <kbd>Esc</kbd> termina.",
    "**{{WegVerbinden}}**: click two points to connect them (<kbd>Ctrl</kbd>: a jump); clicking two connected points disconnects them. A point has at most four connections.":
        "**{{WegVerbinden}}**: pulsa dos puntos para conectarlos (<kbd>Ctrl</kbd>: un salto); pulsar dos puntos conectados los desconecta. Un punto tiene como máximo cuatro conexiones.",
    "**{{WegErzeugen}}** spreads waypoints over the walkable floor (**{{WegAbstand}}**: the original levels use about 230) and connects them - one undo step. **{{WegEntfernen}}** removes only those, hand-placed ones stay.":
        "**{{WegErzeugen}}** reparte waypoints por el suelo transitable (**{{WegAbstand}}**: los niveles originales usan unos 230) y los conecta, en un paso de deshacer. **{{WegEntfernen}}** quita solo esos; los colocados a mano se quedan.",
    "**{{WegPruefen}}** finds what the game would reject or NPCs cannot use; click a problem to fly there.":
        "**{{WegPruefen}}** encuentra lo que el juego rechazaría o los NPC no pueden usar; pulsa un problema para volar hasta allí.",
    "The cutscene preview uses this network for walks exactly like the game: far targets need a path here, otherwise the figure stands still ([[figuren#wieimspiel]]). A network needs at least four waypoints. Doors on the network count while they are open; a closed door counts only if it opens by itself for NPCs.":
        "La vista previa de las cinemáticas usa esta red para las marchas exactamente como el juego: los destinos lejanos necesitan aquí un camino; si no, la figura se queda quieta ([[figuren#wieimspiel]]). Una red necesita al menos cuatro waypoints. Las puertas de la red cuentan mientras están abiertas; una puerta cerrada solo cuenta si se abre sola para los NPC.",
    "The map editor":
        "El editor de mapas",
    "Build and edit maps for Jedi Academy and Jedi Outcast - with the menus, windows and shortcuts of NetRadiant-Custom, so tutorials written for it work step by step.":
        "Crea y edita mapas para Jedi Academy y Jedi Outcast, con los menús, ventanas y atajos de NetRadiant-Custom, para que los tutoriales escritos para él funcionen paso a paso.",
    "The map editor in the layout {{KeLayoutRegular}}: the 2D view on the left with the console below it, the camera on the top right, the texture browser below it.":
        "El editor de mapas con la disposición {{KeLayoutRegular}}: la vista 2D a la izquierda con la consola debajo, la cámara arriba a la derecha y el navegador de texturas debajo.",
    "Opening and saving maps":
        "Abrir y guardar mapas",
    "Switch to **{{ViewMapEditor}}** in the mode bar. The editor works with the game the program is set up for - Jedi Academy (also with Movie Duels) or Jedi Outcast - and its textures, shaders and entity definitions; the first time, it loads them in the background. Without game folders in the program settings there are no textures; under **{{KeLPreferences}}** (pages **{{KePgGame}}** and **{{KePgPaths}}**) the map editor can also use another game and other folders. **{{KeLFile}} > {{KeLNewMap}}** starts an empty map, **{{KeLOpen}}** (<kbd>Ctrl</kbd>+<kbd>O</kbd>) opens a `.map` file, **{{KeLOpenBsp}}** a compiled map of the game (page [[karte_oeffnen]]), **{{KeLSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) and **{{KeLSaveAs}}** write it - like NetRadiant-Custom, the previous file is kept as `.bak`. The last maps you opened are at the bottom of the **{{KeLFile}}** menu. Before a map with unsaved changes is closed, the editor asks whether to save it.":
        "Cambia a **{{ViewMapEditor}}** en la barra de modos. El editor trabaja con el juego para el que está configurado el programa (Jedi Academy, también con Movie Duels, o Jedi Outcast) y sus texturas, shaders y definiciones de entidades; la primera vez los carga en segundo plano. Sin carpetas del juego en los ajustes del programa no hay texturas; en **{{KeLPreferences}}** (páginas **{{KePgGame}}** y **{{KePgPaths}}**) el editor de mapas también puede usar otro juego y otras carpetas. **{{KeLFile}} > {{KeLNewMap}}** empieza un mapa vacío, **{{KeLOpen}}** (<kbd>Ctrl</kbd>+<kbd>O</kbd>) abre un archivo `.map`, **{{KeLOpenBsp}}** un mapa compilado del juego (página [[karte_oeffnen]]), **{{KeLSave}}** (<kbd>Ctrl</kbd>+<kbd>S</kbd>) y **{{KeLSaveAs}}** lo escriben; como en NetRadiant-Custom, el archivo anterior se conserva como `.bak`. Los últimos mapas abiertos están al final del menú **{{KeLFile}}**. Antes de cerrar un mapa con cambios sin guardar, el editor pregunta si guardarlo.",
    "Menus, windows and layouts":
        "Menús, ventanas y disposiciones",
    "Menus, toolbars and dialogs are laid out like NetRadiant-Custom (release 20260114): the same order, separators, shortcuts and mnemonics (<kbd>Alt</kbd> + the underlined letter). The labels are in the program language, the colours come from the program's colour scheme. The plugins are described on the page [[karte_plugins]]. The building tools are described on the page [[karteneditor_bauen]], compiling with q3map2 on [[karte_bauen]].":
        "Los menús, barras y diálogos están dispuestos como en NetRadiant-Custom (versión 20260114): el mismo orden, separadores, atajos y mnemónicos (<kbd>Alt</kbd> + la letra subrayada). Los nombres de menú se quedan en inglés como en NetRadiant-Custom; los diálogos y descripciones están en el idioma del programa y los colores vienen de la combinación de colores del programa. Los plugins se describen en la página [[karte_plugins]]. Las herramientas de construcción se describen en la página [[karteneditor_bauen]], la compilación con q3map2 en [[karte_bauen]].",
    "**Toolbars:** as in NetRadiant-Custom, **Open**, **Save**, **Undo**, **Redo** and **Select Touching** are hidden in the main toolbar by default - right-click a toolbar and tick them to show them. Every toolbar has a handle (the dotted strip at its start): drag it to dock the main, filter or plugin toolbar at another edge (top, left, right, bottom). Edge and order are kept after a restart.":
        "**Barras de herramientas:** como en NetRadiant-Custom, **Open**, **Save**, **Undo**, **Redo** y **Select Touching** están ocultos por defecto en la barra principal; haz clic derecho en una barra y márcalos para mostrarlos. Cada barra tiene un asa (la franja punteada al principio): arrástrala para acoplar la barra principal, la de filtros o la de plugins a otro borde (arriba, izquierda, derecha, abajo). El borde y el orden se mantienen tras reiniciar.",
    "The main toolbar docked on the left, the plugin toolbar at the bottom.":
        "La barra principal acoplada a la izquierda, la de plugins abajo.",
    "In the map editor the program's menu bar becomes NetRadiant-Custom's menu bar ({{KeLFile}} ... {{KeLHelp}}): there is only one, the script tabs are hidden and the editor starts right below it. What the program needs here too is added at the end of two menus: **{{KeLMisc}}** has **{{KeLLanguage}}**, **{{KeLColorScheme}}** and **{{KeLScaling}}**, **{{KeLHelp}}** has **{{KeLManual}}** (<kbd>F1</kbd>, this guide) and **{{KeLAboutBehavEd}}**; **{{KeLCheckForNetRadiantUpdate}}** looks for a new version of this program. Leaving the mode in the mode bar brings back the program's menus and tabs.":
        "En el editor de mapas, la barra de menús del programa se convierte en la de NetRadiant-Custom ({{KeLFile}} ... {{KeLHelp}}): solo hay una, las pestañas de script se ocultan y el editor empieza justo debajo. Lo que el programa necesita también aquí se añade al final de dos menús: **{{KeLMisc}}** tiene **{{KeLLanguage}}**, **{{KeLColorScheme}}** y **{{KeLScaling}}**, **{{KeLHelp}}** tiene **{{KeLManual}}** (<kbd>F1</kbd>, esta guía) y **{{KeLAboutBehavEd}}**; **{{KeLCheckForNetRadiantUpdate}}** busca una versión nueva de este programa. Salir del modo en la barra de modos devuelve los menús y las pestañas del programa.",
    "Every window of the editor - the 2D views, the camera, the console and the texture browser - has a thin frame, so you can see where one ends and the next begins; between them sits NetRadiant-Custom's splitter with its grip dots. The active window - the one you clicked last - has its frame in the accent colour of the colour scheme, as NetRadiant-Custom outlines the active 2D view (**{{KeLView}} > {{KeLShow}} > {{KeLShowWindowOutline}}** switches that off for the 2D views). Drag a splitter to resize the windows; dragged to the edge, a window collapses and can be pulled out again.":
        "Cada ventana del editor (las vistas 2D, la cámara, la consola y el navegador de texturas) tiene un marco fino, para que veas dónde termina una y empieza la siguiente; entre ellas está el divisor de NetRadiant-Custom con sus puntos de agarre. La ventana activa (la última que pulsaste) tiene el marco en el color de acento de la combinación de colores, igual que NetRadiant-Custom remarca la vista 2D activa (**{{KeLView}} > {{KeLShow}} > {{KeLShowWindowOutline}}** lo desactiva para las vistas 2D). Arrastra un divisor para cambiar el tamaño de las ventanas; arrastrada hasta el borde, una ventana se pliega y se puede volver a sacar.",
    "The {{KeLView}} menu.":
        "El menú {{KeLView}}.",
    "**{{KeLEdit}} > {{KeLPreferences}}** (<kbd>P</kbd>) holds NetRadiant-Custom's preferences. On the page **{{KePgLayout}}** you choose the window layout - {{KeLayoutRegular}}, {{KeLayoutFloating}}, {{KeLayoutSplit}} or {{KeLayoutRegularLeft}} - and whether the group dialog with entities and textures sits in the main window (**{{KePrBuiltInGroupDialog}}**). The new layout applies as soon as you press **{{KePrefOk}}**; NetRadiant-Custom needs a restart for that. In {{KeLayoutFloating}} the windows float inside the program window.":
        "**{{KeLEdit}} > {{KeLPreferences}}** (<kbd>P</kbd>) contiene las preferencias de NetRadiant-Custom. En la página **{{KePgLayout}}** eliges la disposición de ventanas ({{KeLayoutRegular}}, {{KeLayoutFloating}}, {{KeLayoutSplit}} o {{KeLayoutRegularLeft}}) y si el diálogo de grupo con entidades y texturas va en la ventana principal (**{{KePrBuiltInGroupDialog}}**). La nueva disposición se aplica en cuanto pulsas **{{KePrefOk}}**; NetRadiant-Custom necesita reiniciar para eso. En {{KeLayoutFloating}} las ventanas flotan dentro de la ventana del programa.",
    "{{KeLPreferences}}, page {{KePgLayout}}.":
        "{{KeLPreferences}}, página {{KePgLayout}}.",
    "**{{KeNrcTerms}}** on the page **{{KePgInterface}}** shows NetRadiant-Custom's English labels in every menu and dialog of the map editor, so tutorials written for NetRadiant-Custom (for example on JKHub) match word for word. Tooltips stay in the program language.":
        "**{{KeNrcTerms}}** en la página **{{KePgInterface}}** muestra los nombres ingleses de NetRadiant-Custom en cada menú y diálogo del editor de mapas, para que los tutoriales escritos para NetRadiant-Custom (por ejemplo en JKHub) coincidan palabra por palabra. Las descripciones emergentes siguen en el idioma del programa.",
    "2D view and camera":
        "Vista 2D y cámara",
    "Right mouse drag (2D)":
        "Arrastrar con el botón derecho (2D)",
    "Scroll the view":
        "Desplazar la vista",
    "Wheel, Alt + right mouse drag (2D)":
        "Rueda, Alt + arrastrar con el botón derecho (2D)",
    "Next 2D view (XY, XZ, YZ)":
        "Vista 2D siguiente (XY, XZ, YZ)",
    "Middle mouse (2D)":
        "Botón central (2D)",
    "Turn the camera towards the point; with <kbd>Ctrl</kbd> the camera moves there":
        "Gira la cámara hacia el punto; con <kbd>Ctrl</kbd> la cámara se mueve allí",
    "Right click in the camera":
        "Clic derecho en la cámara",
    "Free look on and off: the mouse turns the view, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> or the arrow keys move":
        "Activa y desactiva la vista libre: el ratón gira la vista, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> o las flechas mueven",
    "Arrow keys (camera)":
        "Flechas (cámara)",
    "Forward, back, turn; <kbd>A</kbd> / <kbd>D</kbd> sideways; the wheel moves forward and back":
        "Adelante, atrás, girar; <kbd>A</kbd> / <kbd>D</kbd> de lado; la rueda mueve adelante y atrás",
    "Camera onto the selection":
        "Cámara a la selección",
    "How the camera draws (wireframe, flat, textured ...)":
        "Cómo dibuja la cámara (alambre, plano, texturizado ...)",
    "Grid smaller / larger; <kbd>1</kbd> ... <kbd>9</kbd> set it directly (1 to 256 units)":
        "Rejilla más pequeña / más grande; <kbd>1</kbd> ... <kbd>9</kbd> la fijan directamente (de 1 a 256 unidades)",
    "Selecting and building":
        "Seleccionar y construir",
    "Left click":
        "Clic izquierdo",
    "Select what is under the mouse; another click on the same spot selects what lies behind it":
        "Selecciona lo que hay bajo el ratón; otro clic en el mismo sitio selecciona lo que hay detrás",
    "Shift + left mouse drag":
        "Mayús + arrastrar con el botón izquierdo",
    "Select (or deselect) everything the mouse passes over":
        "Selecciona (o deselecciona) todo lo que toca el ratón",
    "Shift + right mouse drag":
        "Mayús + arrastrar con el botón derecho",
    "Select with a rectangle":
        "Seleccionar con un rectángulo",
    "Ctrl + left mouse (camera)":
        "Ctrl + botón izquierdo (cámara)",
    "Select single faces":
        "Seleccionar caras sueltas",
    "Left mouse drag on the selection":
        "Arrastrar con el botón izquierdo sobre la selección",
    "Move it; next to a selected brush: pull its faces to resize it":
        "La mueve; junto a un brush seleccionado: tira de sus caras para cambiar su tamaño",
    "Left mouse drag in empty space (2D, nothing selected)":
        "Arrastrar con el botón izquierdo en un espacio vacío (2D, nada seleccionado)",
    "Draw a new brush with the current texture":
        "Dibujar un brush nuevo con la textura actual",
    "Clear the selection":
        "Quitar la selección",
    "Duplicate the selection":
        "Duplicar la selección",
    "Delete the selection":
        "Eliminar la selección",
    "Undo":
        "Deshacer",
    "Redo":
        "Rehacer",
    "Copy / paste":
        "Copiar / pegar",
    "Hide the selection":
        "Ocultar la selección",
    "Show everything hidden":
        "Mostrar todo lo oculto",
    "Textures":
        "Texturas",
    "The texture browser: the folders on the left, the textures of the current folder on the right.":
        "El navegador de texturas: las carpetas a la izquierda, las texturas de la carpeta actual a la derecha.",
    "The {{KeLEntityInspector}}: the classes with their description, the spawnflags, all keys and values, and fields for the keys of the class.":
        "El {{KeLEntityInspector}}: las clases con su descripción, los spawnflags, todas las claves y valores, y campos para las claves de la clase.",
    "A right click in the 2D view (without moving the mouse) opens the entity menu: pick a class to place a new entity at that point. If brushes are selected and you pick a brush class such as `func_door`, the brushes become that entity. **{{KeLView}} > {{KeLEntityInspector}}** (<kbd>N</kbd>) shows the selected entity: its class with the description, the spawnflags as check boxes, all keys and values, and fields for the keys the class knows. Type a key and a value and press <kbd>Enter</kbd> to set it. **{{KeLView}} > {{KeLEntityList}}** (<kbd>Shift</kbd>+<kbd>L</kbd>) lists all entities of the map. **{{KeLEntity}} > {{KeLConnectEntities}}** (<kbd>Ctrl</kbd>+<kbd>K</kbd>) links two selected entities with `target` and `targetname`.":
        "Un clic derecho en la vista 2D (sin mover el ratón) abre el menú de entidades: elige una clase para colocar una entidad nueva en ese punto. Si hay brushes seleccionados y eliges una clase de brushes como `func_door`, los brushes se convierten en esa entidad. **{{KeLView}} > {{KeLEntityInspector}}** (<kbd>N</kbd>) muestra la entidad seleccionada: su clase con la descripción, los spawnflags como casillas, todas las claves y valores, y campos para las claves que conoce la clase. Escribe una clave y un valor y pulsa <kbd>Intro</kbd> para fijarla. **{{KeLView}} > {{KeLEntityList}}** (<kbd>Mayús</kbd>+<kbd>L</kbd>) lista todas las entidades del mapa. **{{KeLEntity}} > {{KeLConnectEntities}}** (<kbd>Ctrl</kbd>+<kbd>K</kbd>) enlaza dos entidades seleccionadas con `target` y `targetname`.",
    "Console":
        "Consola",
    "The console (in the layout {{KeLayoutRegular}} below the 2D view, otherwise **{{KeLView}} > {{KeLConsole}}**, <kbd>O</kbd>) shows the same messages as NetRadiant-Custom: what was loaded, warnings about missing textures and shaders, and how long loading and saving took.":
        "La consola (en la disposición {{KeLayoutRegular}} debajo de la vista 2D, si no **{{KeLView}} > {{KeLConsole}}**, <kbd>O</kbd>) muestra los mismos mensajes que NetRadiant-Custom: qué se cargó, avisos de texturas y shaders que faltan y cuánto tardaron la carga y el guardado.",
    "Shortcuts":
        "Atajos",
    "{{KeMappedCommands}}: every command with its key.":
        "{{KeMappedCommands}}: cada comando con su tecla.",
    "The map editor has NetRadiant-Custom's shortcuts. They apply while the map editor is shown; the program's own shortcuts are off then. **{{KeLEdit}} > {{KeLShortcuts}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd>) lists every command with its key; the fields at the top search {{KeByName}} or {{KeByKeys}}. Double-click a command (or press **{{KeEdit}}**) and then the new key; **{{KeClear}}** removes the key, **{{KeReset}}** brings back NetRadiant-Custom's key, **{{KeResetAll}}** all of them. If the key already belongs to another command, the editor asks first.":
        "El editor de mapas tiene los atajos de NetRadiant-Custom. Valen mientras se muestra el editor de mapas; los atajos propios del programa están entonces desactivados. **{{KeLEdit}} > {{KeLShortcuts}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>P</kbd>) lista cada comando con su tecla; los campos de arriba buscan {{KeByName}} o {{KeByKeys}}. Haz doble clic en un comando (o pulsa **{{KeEdit}}**) y luego la tecla nueva; **{{KeClear}}** quita la tecla, **{{KeReset}}** recupera la tecla de NetRadiant-Custom, **{{KeResetAll}}** todas. Si la tecla ya pertenece a otro comando, el editor pregunta antes.",
    "Keyboards with <kbd>AltGr</kbd>: where `[` and `]` need <kbd>AltGr</kbd> (German: <kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd>), they still work - `[` and `]` change the grid, and <kbd>Shift</kbd>+<kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd> count as <kbd>Shift</kbd>+<kbd>[</kbd> / <kbd>]</kbd>. But <kbd>Ctrl</kbd>+<kbd>[</kbd> and <kbd>Ctrl</kbd>+<kbd>]</kbd> (**{{KeLFarClipPlaneIn}}** / **{{KeLFarClipPlaneOut}}**) cannot be pressed, because <kbd>AltGr</kbd> already counts as <kbd>Ctrl</kbd>+<kbd>Alt</kbd> - exactly as in NetRadiant-Custom. Use the menu **{{KeLView}} > {{KeLCamera}}**, or give the commands `CubicClipZoomIn` and `CubicClipZoomOut` other keys in **{{KeMappedCommands}}**.":
        "Teclados con <kbd>AltGr</kbd>: donde `[` y `]` necesitan <kbd>AltGr</kbd> (en español con las teclas a la derecha de la P; en alemán <kbd>AltGr</kbd>+<kbd>8</kbd> / <kbd>9</kbd>), siguen funcionando: `[` y `]` cambian la rejilla, y <kbd>Mayús</kbd>+<kbd>AltGr</kbd> con esas teclas cuenta como <kbd>Mayús</kbd>+<kbd>[</kbd> / <kbd>]</kbd>. Pero <kbd>Ctrl</kbd>+<kbd>[</kbd> y <kbd>Ctrl</kbd>+<kbd>]</kbd> (**{{KeLFarClipPlaneIn}}** / **{{KeLFarClipPlaneOut}}**) no se pueden pulsar, porque <kbd>AltGr</kbd> ya cuenta como <kbd>Ctrl</kbd>+<kbd>Alt</kbd>, exactamente como en NetRadiant-Custom. Usa el menú **{{KeLView}} > {{KeLCamera}}**, o da a los comandos `CubicClipZoomIn` y `CubicClipZoomOut` otras teclas en **{{KeMappedCommands}}**.",
    "Map editor: building tools":
        "Editor de mapas: herramientas de construcción",
    "Vertices, edges and faces, the clipper, CSG, brush shapes, transformations, patches, texturing, regions, filters, layers and models - the tools of NetRadiant-Custom with its keys, menus and dialogs.":
        "Vértices, aristas y caras, el clipper, CSG, formas de brush, transformaciones, patches, texturizado, regiones, filtros, capas y modelos: las herramientas de NetRadiant-Custom con sus teclas, menús y diálogos.",
    "Vertices, edges and faces":
        "Vértices, aristas y caras",
    "Vertex mode (<kbd>V</kbd>): the points of the selected brush can be dragged.":
        "Modo vértices (<kbd>V</kbd>): los puntos del brush seleccionado se pueden arrastrar.",
    "Select a brush or patch and press <kbd>V</kbd> (vertices), <kbd>E</kbd> (edges) or <kbd>F</kbd> (faces). Click a point to select it, drag it to reshape the brush; the brush is rebuilt from its points as in NetRadiant-Custom. The same key again returns to normal selection. <kbd>B</kbd> is the build tool (extrude, build chains, clone), <kbd>G</kbd> the UV tool for the texture of a face. **{{KeLTextureLock}}** (<kbd>Shift</kbd>+<kbd>T</kbd>) keeps textures in place while you move brushes.":
        "Selecciona un brush o un patch y pulsa <kbd>V</kbd> (vértices), <kbd>E</kbd> (aristas) o <kbd>F</kbd> (caras). Pulsa un punto para seleccionarlo y arrástralo para dar otra forma al brush; el brush se reconstruye a partir de sus puntos como en NetRadiant-Custom. La misma tecla otra vez vuelve a la selección normal. <kbd>B</kbd> es la herramienta de construcción (extruir, construir cadenas, clonar), <kbd>G</kbd> la herramienta UV para la textura de una cara. **{{KeLTextureLock}}** (<kbd>Mayús</kbd>+<kbd>T</kbd>) mantiene las texturas en su sitio mientras mueves brushes.",
    "Clipper":
        "Clipper",
    "The clipper (<kbd>X</kbd>): the line shows where the selected brush will be cut.":
        "El clipper (<kbd>X</kbd>): la línea muestra por dónde se cortará el brush seleccionado.",
    "Select brushes and press <kbd>X</kbd>. Click up to three points in a 2D view, or on surfaces in the camera, to define the cutting plane; drag the points to correct them. <kbd>Enter</kbd> cuts and keeps one side, <kbd>Shift</kbd>+<kbd>Enter</kbd> splits into both halves, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> flips which side is kept. A double click cuts or splits, as set on the preference page **{{KePgClipper}}**.":
        "Selecciona brushes y pulsa <kbd>X</kbd>. Pulsa hasta tres puntos en una vista 2D, o sobre superficies en la cámara, para definir el plano de corte; arrastra los puntos para corregirlos. <kbd>Intro</kbd> corta y conserva un lado, <kbd>Mayús</kbd>+<kbd>Intro</kbd> divide en las dos mitades, <kbd>Ctrl</kbd>+<kbd>Intro</kbd> cambia qué lado se conserva. Un doble clic corta o divide, según se fije en la página de preferencias **{{KePgClipper}}**.",
    "CSG":
        "CSG",
    "{{KeLCSGTool}}: hollow, shrink, expand and caulk in one window.":
        "{{KeLCSGTool}}: vaciar, encoger, expandir y poner caulk en una sola ventana.",
    "**{{KeLBrush}} > {{KeLCSG}}**: **{{KeLCSGSubtract}}** (<kbd>Shift</kbd>+<kbd>U</kbd>) cuts the selected brushes out of everything they touch, **{{KeLCSGMerge}}** joins brushes into one convex brush, **{{KeLCSGWrapMerge}}** (<kbd>Ctrl</kbd>+<kbd>U</kbd>) wraps them in one, **{{KeLCSGIntersect}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>) keeps only the common part, **{{KeLMakeRoom}}** turns a brush into a hollow room. **{{KeLMakeDetail}}** (<kbd>Alt</kbd>+<kbd>D</kbd>) and **{{KeLMakeStructural}}** (<kbd>Alt</kbd>+<kbd>S</kbd>) set the detail flag.":
        "**{{KeLBrush}} > {{KeLCSG}}**: **{{KeLCSGSubtract}}** (<kbd>Mayús</kbd>+<kbd>U</kbd>) recorta los brushes seleccionados de todo lo que tocan, **{{KeLCSGMerge}}** une brushes en un brush convexo, **{{KeLCSGWrapMerge}}** (<kbd>Ctrl</kbd>+<kbd>U</kbd>) los envuelve en uno, **{{KeLCSGIntersect}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>U</kbd>) conserva solo la parte común, **{{KeLMakeRoom}}** convierte un brush en una sala hueca. **{{KeLMakeDetail}}** (<kbd>Alt</kbd>+<kbd>D</kbd>) y **{{KeLMakeStructural}}** (<kbd>Alt</kbd>+<kbd>S</kbd>) fijan la marca de detalle.",
    "Brush shapes and transformations":
        "Formas de brush y transformaciones",
    "The number of sides for **{{KeLPrism}}**.":
        "El número de lados para **{{KeLPrism}}**.",
    "**{{KeLBrush}}** turns the selected brush into a **{{KeLPrism}}**, **{{KeLCone}}**, **{{KeLSphere}}**, **{{KeLRock}}** or **{{KeLIcosahedron}}**; a small dialog asks for the number of sides. <kbd>Ctrl</kbd>+<kbd>3</kbd> ... <kbd>9</kbd> make a prism with that many sides around the axis of the active 2D view. **{{KeLModify}}** rotates and mirrors the selection around its centre, **{{KeLArbitraryRotation}}** (<kbd>Shift</kbd>+<kbd>R</kbd>) and **{{KeLArbitraryScale}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd>) take exact values. **{{KeLRepeatTransforms}}** (<kbd>Ctrl</kbd>+<kbd>R</kbd>) repeats the last move, rotation or scale, **{{KeLResetTransforms}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) forgets it.":
        "**{{KeLBrush}}** convierte el brush seleccionado en un **{{KeLPrism}}**, **{{KeLCone}}**, **{{KeLSphere}}**, **{{KeLRock}}** o **{{KeLIcosahedron}}**; un diálogo pequeño pide el número de lados. <kbd>Ctrl</kbd>+<kbd>3</kbd> ... <kbd>9</kbd> crean un prisma con tantos lados alrededor del eje de la vista 2D activa. **{{KeLModify}}** gira y refleja la selección alrededor de su centro, **{{KeLArbitraryRotation}}** (<kbd>Mayús</kbd>+<kbd>R</kbd>) y **{{KeLArbitraryScale}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>S</kbd>) toman valores exactos. **{{KeLRepeatTransforms}}** (<kbd>Ctrl</kbd>+<kbd>R</kbd>) repite el último movimiento, giro o escala, **{{KeLResetTransforms}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) lo olvida.",
    "Patches":
        "Patches",
    "**{{KeLCurve}} > {{KeLCylinder9x3}}** made a cylinder in the bounds of the selected brush.":
        "**{{KeLCurve}} > {{KeLCylinder9x3}}** ha creado un cilindro en los límites del brush seleccionado.",
    "Texturing":
        "Texturizado",
    "The {{KeSiTitle}} (<kbd>S</kbd>).":
        "El {{KeSiTitle}} (<kbd>S</kbd>).",
    "The {{KeSiTitle}} (<kbd>S</kbd>) shows the texture of the selected faces: shift, scale and rotation, the step sizes for the arrow keys, fit (<kbd>Ctrl</kbd>+<kbd>F</kbd>) and the projection (axial, ortho, from the camera). Without the window, <kbd>Shift</kbd> + arrow keys shift the texture, <kbd>Ctrl</kbd> + arrow keys scale it, <kbd>Shift</kbd>+<kbd>PgUp</kbd> / <kbd>PgDn</kbd> rotate it. **{{KeLSelectTextured}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>A</kbd>) selects all faces with the current texture. With **{{KeLTagsGUI}}** the texture browser gets tags for sorting textures.":
        "El {{KeSiTitle}} (<kbd>S</kbd>) muestra la textura de las caras seleccionadas: desplazamiento, escala y rotación, los pasos para las flechas, ajustar (<kbd>Ctrl</kbd>+<kbd>F</kbd>) y la proyección (axial, orto, desde la cámara). Sin la ventana, <kbd>Mayús</kbd> + flechas desplazan la textura, <kbd>Ctrl</kbd> + flechas la escalan, <kbd>Mayús</kbd>+<kbd>RePág</kbd> / <kbd>AvPág</kbd> la giran. **{{KeLSelectTextured}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>A</kbd>) selecciona todas las caras con la textura actual. Con **{{KeLTagsGUI}}** el navegador de texturas tiene etiquetas para ordenar texturas.",
    "Selection, regions and files":
        "Selección, regiones y archivos",
    "**{{KeLMisc}} > {{KeLMapInfo}}** (<kbd>M</kbd>): how many brushes, patches and entities the map has.":
        "**{{KeLMisc}} > {{KeLMapInfo}}** (<kbd>M</kbd>): cuántos brushes, patches y entidades tiene el mapa.",
    "**{{KeLSelectInside}}** and **{{KeLSelectTouching}}** select what lies inside or touches the selected brush. **{{KeLView}} > {{KeLRegion}}** shows only part of the map: **{{KeLSetXY}}** (the visible area of the 2D view), **{{KeLSetBrush}}**, **{{KeLSetSelection}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>R</kbd>), **{{KeLOff}}**. **{{KeLFile}} > {{KeLSaveRegion}}** writes the region with walls around it and a player start, **{{KeLSaveSelected}}** only the selection, **{{KeLImport}}** adds another map. **{{KeLFindBrush}}** selects a brush by its entity and brush number. The map is saved automatically every 15 minutes (preference page **{{KePgAutosave}}**).":
        "**{{KeLSelectInside}}** y **{{KeLSelectTouching}}** seleccionan lo que está dentro del brush seleccionado o lo toca. **{{KeLView}} > {{KeLRegion}}** muestra solo una parte del mapa: **{{KeLSetXY}}** (la zona visible de la vista 2D), **{{KeLSetBrush}}**, **{{KeLSetSelection}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>R</kbd>), **{{KeLOff}}**. **{{KeLFile}} > {{KeLSaveRegion}}** escribe la región con paredes alrededor y un inicio del jugador, **{{KeLSaveSelected}}** solo la selección, **{{KeLImport}}** añade otro mapa. **{{KeLFindBrush}}** selecciona un brush por su número de entidad y de brush. El mapa se guarda automáticamente cada 15 minutos (página de preferencias **{{KePgAutosave}}**).",
    "Filters":
        "Filtros",
    "**{{KeLView}} > {{KeLFilter}}** and the filter bar on the right hide kinds of objects: world brushes (<kbd>Alt</kbd>+<kbd>1</kbd>), entities, caulk, clips, liquids, detail brushes, patches, lights, models, triggers and more. Some buttons do something else on a right click, as their tooltip says: make the selection detail or structural, create a func_group, put caulk or a tool texture on it. **{{KeLInvertFilters}}** and **{{KeLResetFilters}}** turn all of them around or off.":
        "**{{KeLView}} > {{KeLFilter}}** y la barra de filtros de la derecha ocultan tipos de objetos: brushes del mundo (<kbd>Alt</kbd>+<kbd>1</kbd>), entidades, caulk, clips, líquidos, brushes de detalle, patches, luces, modelos, triggers y más. Algunos botones hacen otra cosa con un clic derecho, como dice su descripción: hacer la selección de detalle o estructural, crear un func_group, ponerle caulk o una textura de herramienta. **{{KeLInvertFilters}}** y **{{KeLResetFilters}}** los invierten todos o los quitan.",
    "Layers":
        "Capas",
    "The {{KeLLayersBrowser}} (<kbd>L</kbd>).":
        "El {{KeLLayersBrowser}} (<kbd>L</kbd>).",
    "Layers group parts of the map. In the {{KeLLayersBrowser}} (<kbd>L</kbd>) you create and rename layers, nest them by dragging, hide or show them with the eye, and move the selection into a layer (<kbd>Shift</kbd>+<kbd>Q</kbd>); <kbd>Ctrl</kbd>+<kbd>Q</kbd> extends the selection to its layers. New objects go into the current layer. The layers are saved in the `.map` file as in NetRadiant-Custom.":
        "Las capas agrupan partes del mapa. En el {{KeLLayersBrowser}} (<kbd>L</kbd>) creas y renombras capas, las anidas arrastrando, las ocultas o muestras con el ojo y mueves la selección a una capa (<kbd>Mayús</kbd>+<kbd>Q</kbd>); <kbd>Ctrl</kbd>+<kbd>Q</kbd> amplía la selección a sus capas. Los objetos nuevos van a la capa actual. Las capas se guardan en el archivo `.map` como en NetRadiant-Custom.",
    "Models":
        "Modelos",
    "The {{KeLModelBrowser}} (<kbd>/</kbd>) with the models of a folder.":
        "El {{KeLModelBrowser}} (<kbd>/</kbd>) con los modelos de una carpeta.",
    "Entities like `misc_model` show their model (`.md3`, `.glm`, `.ase`) in both views, with skin, `modelscale` and angles. In the {{KeLModelBrowser}} (<kbd>/</kbd>) a double click on a folder loads its models; a click on a model gives it to the selected entities, a double click inserts a new `misc_model` in front of the camera. **{{KeLRefreshModels}}** reloads the models after they changed.":
        "Las entidades como `misc_model` muestran su modelo (`.md3`, `.glm`, `.ase`) en ambas vistas, con skin, `modelscale` y ángulos. En el {{KeLModelBrowser}} (<kbd>/</kbd>) un doble clic en una carpeta carga sus modelos; un clic en un modelo se lo da a las entidades seleccionadas, un doble clic inserta un `misc_model` nuevo delante de la cámara. **{{KeLRefreshModels}}** recarga los modelos después de cambiarlos.",
    "Preferences":
        "Preferencias",
    "{{KeLPreferences}}, page {{KePgOrtho}}.":
        "{{KeLPreferences}}, página {{KePgOrtho}}.",
    "All pages of NetRadiant-Custom's preferences work: **{{KePgBrush}}** (default texture scale, brush type of new maps, always caulk), **{{KePgGrid}}**, **{{KePgUndo}}**, **{{KePgOrtho}}** (solid or stippled selection, follow the mouse at the edge while dragging, zoom to the mouse pointer), **{{KePgBuild}}**, **{{KePgSettings}}** (nudge after duplicating, load the last map). Under **{{KePgGame}}** you choose Jedi Academy or Jedi Outcast, a mod and the mapping mode (single or multiplayer entities), under **{{KePgPaths}}** the game folder and up to five extra folders. Choosing another game asks to restart the map editor, as NetRadiant-Custom does.":
        "Todas las páginas de las preferencias de NetRadiant-Custom funcionan: **{{KePgBrush}}** (escala de textura predeterminada, tipo de brush de los mapas nuevos, siempre caulk), **{{KePgGrid}}**, **{{KePgUndo}}**, **{{KePgOrtho}}** (selección sólida o punteada, seguir al ratón en el borde al arrastrar, zoom hacia el puntero), **{{KePgBuild}}**, **{{KePgSettings}}** (desplazar tras duplicar, cargar el último mapa). En **{{KePgGame}}** eliges Jedi Academy o Jedi Outcast, un mod y el modo de mapeado (entidades de un jugador o multijugador), en **{{KePgPaths}}** la carpeta del juego y hasta cinco carpetas extra. Elegir otro juego pide reiniciar el editor de mapas, como en NetRadiant-Custom.",
    "Menus and windows":
        "Menús y ventanas",
    "The menu {{KeLMisc}}, torn off at its dashed line, stays open as a window.":
        "El menú {{KeLMisc}}, separado por su línea discontinua, se queda abierto como ventana.",
    "A click on the dashed line at the top of a menu tears it off into a window that stays open (**{{KePrDetachableMenus}}** on the page **{{KePgLayout}}**). **{{KeLMisc}} > {{KeLFullscreen}}** (<kbd>F11</kbd>) fills the screen, **{{KeLMaximizeView}}** (<kbd>F12</kbd>) enlarges the view under the mouse. **{{KeLOpenGLFont}}** chooses the font of the 2D and camera views, **{{KeLSet2DBackgroundImage}}** puts an image file into the bounds of the selection in the active 2D view, for tracing. The plugins are described on [[karte_plugins]], compiling with q3map2 on [[karte_bauen]].":
        "Un clic en la línea discontinua de arriba de un menú lo separa en una ventana que se queda abierta (**{{KePrDetachableMenus}}** en la página **{{KePgLayout}}**). **{{KeLMisc}} > {{KeLFullscreen}}** (<kbd>F11</kbd>) llena la pantalla, **{{KeLMaximizeView}}** (<kbd>F12</kbd>) amplía la vista bajo el ratón. **{{KeLOpenGLFont}}** elige la fuente de las vistas 2D y de cámara, **{{KeLSet2DBackgroundImage}}** pone un archivo de imagen en los límites de la selección en la vista 2D activa, para calcarlo. Los plugins se describen en [[karte_plugins]], la compilación con q3map2 en [[karte_bauen]].",
    "Map editor: compiling the map":
        "Editor de mapas: compilar el mapa",
    "Turn the map into a playable .bsp with q3map2 from the {{KeLBuild}} menu: it runs in the background with its output in the console, shows leaks as a red line and portals on request, and never writes into the game folder.":
        "Convierte el mapa en un .bsp jugable con q3map2 desde el menú {{KeLBuild}}: se ejecuta en segundo plano con su salida en la consola, muestra las fugas como una línea roja y los portales si se pide, y nunca escribe en la carpeta del juego.",
    "The {{KeLBuild}} menu":
        "El menú {{KeLBuild}}",
    "The {{KeLBuild}} menu: the entries of NetRadiant-Custom's build menu for Jedi Academy, and at the bottom {{KeBmAbbrechen}}.":
        "El menú {{KeLBuild}}: las entradas del menú de compilación de NetRadiant-Custom para Jedi Academy, y al final {{KeBmAbbrechen}}.",
    "Each entry runs one or more q3map2 commands on the map. The map compiler `bhed_q3map2.exe` is q3map2 from NetRadiant-Custom, built with the program and shipped next to `jkstudio.exe`; it produces the same results. **{{KeLRunRecentBuild}}** (<kbd>F5</kbd>) repeats the last entry. **{{KeLCustomize}}** opens the dialog **{{KeBmTitel}}**: entries and their commands can be added (double-click the `+`), renamed, reordered by dragging (with <kbd>Ctrl</kbd> copied) and deleted; **{{KeBmVariablen}}** shows the variables such as `[MapFile]` and `[q3map2]`. The menu is saved when the map editor closes.":
        "Cada entrada ejecuta uno o varios comandos de q3map2 sobre el mapa. El compilador de mapas `bhed_q3map2.exe` es el q3map2 de NetRadiant-Custom, compilado con el programa y distribuido junto a `jkstudio.exe`; produce los mismos resultados. **{{KeLRunRecentBuild}}** (<kbd>F5</kbd>) repite la última entrada. **{{KeLCustomize}}** abre el diálogo **{{KeBmTitel}}**: las entradas y sus comandos se pueden añadir (doble clic en el `+`), renombrar, reordenar arrastrando (con <kbd>Ctrl</kbd> se copian) y eliminar; **{{KeBmVariablen}}** muestra las variables como `[MapFile]` y `[q3map2]`. El menú se guarda al cerrar el editor de mapas.",
    "{{KeLBuild}} > {{KeLCustomize}}: the entry [Final] Fast with its three commands.":
        "{{KeLBuild}} > {{KeLCustomize}}: la entrada [Final] Fast con sus tres comandos.",
    "The three stages":
        "Las tres etapas",
    "Stage":
        "Etapa",
    "What it does":
        "Qué hace",
    "BSP (`-meta`)":
        "BSP (`-meta`)",
    "Turns brushes and patches into the BSP tree and the surfaces, writes `.bsp`, `.prt` and `.srf`. If the map is not sealed, it stops with a leak.":
        "Convierte brushes y patches en el árbol BSP y las superficies, escribe `.bsp`, `.prt` y `.srf`. Si el mapa no está sellado, se detiene con una fuga.",
    "VIS (`-vis`)":
        "VIS (`-vis`)",
    "Computes which areas can see each other, so the game draws less. Needs the portal file of the BSP stage.":
        "Calcula qué zonas pueden verse entre sí, para que el juego dibuje menos. Necesita el archivo de portales de la etapa BSP.",
    "LIGHT (`-light`)":
        "LIGHT (`-light`)",
    "Bakes the light of the light entities and the sky into lightmaps and the light grid.":
        "Hornea la luz de las entidades de luz y del cielo en lightmaps y la rejilla de luz.",
    "**[Test] Quick and Crude** runs all three stages quickly for a first look, **[Final] Fast** and **[Final] Extreme Quality** give the light its real quality. The single stages are useful while working on one part, for example only **Single: light stage** after moving lights.":
        "**[Test] Quick and Crude** ejecuta las tres etapas rápidamente para un primer vistazo, **[Final] Fast** y **[Final] Extreme Quality** dan a la luz su calidad real. Las etapas sueltas sirven mientras trabajas en una parte, por ejemplo solo **Single: light stage** tras mover luces.",
    "While it builds":
        "Mientras compila",
    "q3map2 runs in the background: its output appears in the console, the status bar shows the entry, the step and the time.":
        "q3map2 se ejecuta en segundo plano: su salida aparece en la consola y la barra de estado muestra la entrada, el paso y el tiempo.",
    "The map is saved first (an unnamed map asks for a name). With a region, only the region is built (`<map>.reg`).":
        "Primero se guarda el mapa (un mapa sin nombre pide un nombre). Con una región, solo se compila la región (`<mapa>.reg`).",
    "q3map2 starts without a window and reports back over a local connection, as in NetRadiant-Custom. Warnings appear yellow, errors red; an error stops the remaining steps.":
        "q3map2 arranca sin ventana e informa por una conexión local, como en NetRadiant-Custom. Los avisos aparecen en amarillo, los errores en rojo; un error detiene los pasos restantes.",
    "You can keep working - look around, select, edit. **{{KeLBuild}} > {{KeBmAbbrechen}}** ends q3map2.":
        "Puedes seguir trabajando: mirar alrededor, seleccionar, editar. **{{KeLBuild}} > {{KeBmAbbrechen}}** termina q3map2.",
    "At the end a line names the entry, the steps and the time - and q3map2's exit code if it was not 0.":
        "Al final, una línea nombra la entrada, los pasos y el tiempo, y el código de salida de q3map2 si no fue 0.",
    "The console after [Final] Fast.":
        "La consola tras [Final] Fast.",
    "Without **{{KePrBuildMonitor}}** (page **{{KePgBuild}}** of the preferences) the steps run one after another, their output still comes into the console; **{{KePrBuildDump}}** also writes it to `junk.txt` in the settings folder.":
        "Sin **{{KePrBuildMonitor}}** (página **{{KePgBuild}}** de las preferencias) los pasos se ejecutan uno tras otro y su salida sigue llegando a la consola; **{{KePrBuildDump}}** la escribe además en `junk.txt` en la carpeta de ajustes.",
    "Finding leaks":
        "Encontrar fugas",
    "A hole in a wall: the red line runs from an entity inside to the outside. The camera stands on the line and looks along it.":
        "Un agujero en una pared: la línea roja va de una entidad del interior al exterior. La cámara está sobre la línea y mira a lo largo de ella.",
    "The inside of a map must be sealed by brushes. If q3map2 finds a way from an entity to the void, the BSP stage stops with **MAP LEAKED** (red), and the leak line appears red in the 2D views and the camera. The **{{KeDbgTitel}}** lists the entity it leaks from - a click selects it.":
        "El interior de un mapa debe estar sellado por brushes. Si q3map2 encuentra un camino de una entidad al vacío, la etapa BSP se detiene con **MAP LEAKED** (en rojo), y la línea de fuga aparece en rojo en las vistas 2D y en la cámara. La **{{KeDbgTitel}}** lista la entidad desde la que se fuga: un clic la selecciona.",
    "Ctrl+Shift+K":
        "Ctrl+Mayús+K",
    "{{KeLNextLeakSpot}}: camera and 2D views to the next point of the line, looking along it":
        "{{KeLNextLeakSpot}}: la cámara y las vistas 2D al siguiente punto de la línea, mirando a lo largo de ella",
    "Ctrl+Shift+L":
        "Ctrl+Mayús+L",
    "{{KeLPreviousLeakSpot}}: back to the previous point":
        "{{KeLPreviousLeakSpot}}: vuelve al punto anterior",
    "Follow the line until it leaves the map - there is the hole. Close it and build again. **{{KeLFile}} > {{KeLPointfile}}** switches the line off and on (it reads `<map>.lin`); saving the map removes it.":
        "Sigue la línea hasta que salga del mapa: ahí está el agujero. Ciérralo y compila de nuevo. **{{KeLFile}} > {{KeLPointfile}}** desactiva y activa la línea (lee `<mapa>.lin`); guardar el mapa la quita.",
    "**{{KeLBuild}} > {{KeOptMenue}}** also shows the leak and detail brushes the line passes through - detail does not seal. It counts the map against the limits of the game and finds other build problems: [[karte_optimierer]].":
        "**{{KeLBuild}} > {{KeOptMenue}}** también muestra la fuga y los brushes de detalle por los que pasa la línea: el detalle no sella. Cuenta el mapa frente a los límites del juego y encuentra otros problemas de compilación: [[karte_optimierer]].",
    "Showing portals":
        "Mostrar portales",
    "Plugins > prtview > Load .prt file":
        "Plugins > prtview > Load .prt file",
    "Configure Portal Viewer":
        "Configure Portal Viewer",
    "Portals are the openings between the areas that the VIS stage works with. **{{KeLPlugins}} > prtview > Load .prt file** loads the portal file of the last BSP stage (**{{KePrtZeige3d}}**, **{{KePrtZeige2d}}**): in the camera each portal is a coloured, half-transparent polygon with an outline, in the 2D views a blue line. Many small portals where the player sees far make the map slow - hint brushes and detail brushes help. **Configure Portal Viewer** sets colours, line widths, opacity, fog and a clipping cube; **Toggle portals (3D)/(2D)** and **Unload .prt file** switch the display.":
        "Los portales son las aberturas entre las zonas con las que trabaja la etapa VIS. **{{KeLPlugins}} > prtview > Load .prt file** carga el archivo de portales de la última etapa BSP (**{{KePrtZeige3d}}**, **{{KePrtZeige2d}}**): en la cámara cada portal es un polígono de color semitransparente con contorno, en las vistas 2D una línea azul. Muchos portales pequeños donde el jugador ve lejos hacen lento el mapa; los brushes hint y de detalle ayudan. **Configure Portal Viewer** fija colores, anchos de línea, opacidad, niebla y un cubo de recorte; **Toggle portals (3D)/(2D)** y **Unload .prt file** cambian la visualización.",
    "The portals of the test map: blue in the 2D view, coloured in the camera at the passage between the two rooms.":
        "Los portales del mapa de prueba: azules en la vista 2D, de color en la cámara en el paso entre las dos salas.",
    "Where the files go":
        "Adónde van los archivos",
    "Page {{KePgBuild}} of the preferences with **{{KePrBuildZiel}}**.":
        "Página {{KePgBuild}} de las preferencias con **{{KePrBuildZiel}}**.",
    "The `.bsp` and the other files are written next to the `.map`, as in NetRadiant-Custom. **{{KePrBuildZiel}}** on the page **{{KePgBuild}}** puts them into a folder of your choice instead (the map is copied there and built). Nothing is ever written into the game folder: if the map or the output folder lies there, the build runs in a work folder in the settings folder, and the console says so.":
        "El `.bsp` y los demás archivos se escriben junto al `.map`, como en NetRadiant-Custom. **{{KePrBuildZiel}}** en la página **{{KePgBuild}}** los pone en cambio en una carpeta de tu elección (el mapa se copia ahí y se compila). Nunca se escribe nada en la carpeta del juego: si el mapa o la carpeta de salida están ahí, la compilación se hace en una carpeta de trabajo de la carpeta de ajustes, y la consola lo dice.",
    "Testing in the game":
        "Probar en el juego",
    "Copy the built `<name>.bsp` into the `maps` folder of the game, for example `GameData\\base\\maps` (with a mod: its folder, e.g. `MD\\maps`). The map editor does not do this itself - it never writes into the game folder.":
        "Copia el `<nombre>.bsp` compilado en la carpeta `maps` del juego, por ejemplo `GameData\\base\\maps` (con un mod: su carpeta, p. ej. `MD\\maps`). El editor de mapas no lo hace él mismo: nunca escribe en la carpeta del juego.",
    "Start the game yourself, open the console with <kbd>Shift</kbd>+<kbd>~</kbd> and type `devmap <name>` - the name without `.bsp`. `devmap` also allows cheats such as `noclip` and `god` for looking around.":
        "Inicia tú mismo el juego, abre la consola con <kbd>Mayús</kbd>+<kbd>~</kbd> y escribe `devmap <nombre>`, el nombre sin `.bsp`. `devmap` también permite trucos como `noclip` y `god` para mirar alrededor.",
    "After the next build, copy the `.bsp` again and type `devmap <name>` once more - the game loads the new version.":
        "Tras la siguiente compilación, vuelve a copiar el `.bsp` y escribe otra vez `devmap <nombre>`: el juego carga la versión nueva.",
    "**{{KePrBuildRun}}** on the page **{{KePgBuild}}** starts the game after a build with `+devmap <name>`, as in NetRadiant-Custom - the `.bsp` must then already be in a folder the game searches.":
        "**{{KePrBuildRun}}** en la página **{{KePgBuild}}** inicia el juego tras una compilación con `+devmap <nombre>`, como en NetRadiant-Custom; el `.bsp` debe estar entonces ya en una carpeta en la que busque el juego.",
    "AutoCaulk":
        "AutoCaulk",
    "**{{KeLBrush}} > {{KeLAutoCaulkSelected}}** (<kbd>F4</kbd>) gives every face of the selected brushes that cannot be seen from inside the caulk texture - fewer surfaces, a faster map. Stand with the camera inside the selection; q3map2 decides which faces are hidden. One undo step takes it back.":
        "**{{KeLBrush}} > {{KeLAutoCaulkSelected}}** (<kbd>F4</kbd>) da la textura caulk a cada cara de los brushes seleccionados que no se puede ver desde dentro: menos superficies, un mapa más rápido. Pon la cámara dentro de la selección; q3map2 decide qué caras están ocultas. Un paso de deshacer lo revierte.",
    "Map editor: opening and changing finished maps":
        "Editor de mapas: abrir y cambiar mapas terminados",
    "Open a compiled map of the game - straight from its .pk3 - with {{KeLFile}} > {{KeLOpenBsp}}, change its entities and write them back with {{KeLBuild}} > {{KeBmNurEntities}}: the geometry and the original light stay byte for byte as they are.":
        "Abre un mapa compilado del juego (directamente de su .pk3) con {{KeLFile}} > {{KeLOpenBsp}}, cambia sus entidades y escríbelas de vuelta con {{KeLBuild}} > {{KeBmNurEntities}}: la geometría y la luz original quedan byte a byte como están.",
    "Opening a compiled map":
        "Abrir un mapa compilado",
    "{{KeLFile}} > {{KeLOpenBsp}}: the dialog **{{KeBspTitel}}** with the filter `duel` - each map next to the archive the game loads it from; below the list the options for [[karte_oeffnen#modelle|misc_model]] and [[karte_oeffnen#lichter|lights]].":
        "{{KeLFile}} > {{KeLOpenBsp}}: el diálogo **{{KeBspTitel}}** con el filtro `duel`; cada mapa junto al archivo .pk3 desde el que lo carga el juego; bajo la lista, las opciones para [[karte_oeffnen#modelle|misc_model]] y [[karte_oeffnen#lichter|luces]].",
    "**{{KeLFile}} > {{KeLOpenBsp}}** lists every map (`maps/…/*.bsp`) of the game folders as the game sees it: for each name only the copy the game would load - a mod before `base`, a later .pk3 before an earlier one. The right column names the .pk3 or the folder. Type part of the name into **{{KeBspFilter}}**.":
        "**{{KeLFile}} > {{KeLOpenBsp}}** lista cada mapa (`maps/…/*.bsp`) de las carpetas del juego tal como lo ve el juego: para cada nombre solo la copia que cargaría el juego; un mod antes que `base`, un .pk3 posterior antes que uno anterior. La columna derecha nombra el .pk3 o la carpeta. Escribe parte del nombre en **{{KeBspFilter}}**.",
    "Double-click a map or select it and click **{{KeBspOeffnen}}**. **{{KeBspDatei}}** opens a .bsp from anywhere on disk instead.":
        "Haz doble clic en un mapa, o selecciónalo y pulsa **{{KeBspOeffnen}}**. **{{KeBspDatei}}** abre en cambio un .bsp de cualquier lugar del disco.",
    "The map editor copies the .bsp into a work folder in the settings folder (`karte_bsp\\<name>`) - never into the game folder - and turns it back into a map with q3map2 `-convert`, as NetRadiant-Custom does when it imports a .bsp. Then the map is open like a `.map`. If the open map has unsaved changes, the editor asks first.":
        "El editor de mapas copia el .bsp en una carpeta de trabajo de la carpeta de ajustes (`karte_bsp\\<nombre>`), nunca en la carpeta del juego, y lo vuelve a convertir en un mapa con q3map2 `-convert`, como hace NetRadiant-Custom al importar un .bsp. Luego el mapa está abierto como un `.map`. Si el mapa abierto tiene cambios sin guardar, el editor pregunta antes.",
    "mp/duel1 opened from the game's .pk3: 362 brushes, 66 patches and 26 entities from the .bsp; in the background 14 `misc_model` were found again and 938 triangles of models that were not found became 779 thin brushes in the layer {{KeRkEbene}} (see [[karte_oeffnen#modelle]]).":
        "mp/duel1 abierto desde el .pk3 del juego: 362 brushes, 66 patches y 26 entidades del .bsp; en segundo plano se volvieron a encontrar 14 `misc_model` y 938 triángulos de modelos no encontrados se convirtieron en 779 brushes finos en la capa {{KeRkEbene}} (mira [[karte_oeffnen#modelle]]).",
    "A compiled map is not the original `.map`. Brushes with their textures, patches and entities come back. What q3map2 bakes into the .bsp does not: the models of `misc_model` entities, decals and the light entities - maps compiled for Jedi Academy normally no longer contain them, their light exists only in the lightmaps and the light grid. On some faces the texture alignment can differ slightly. Compiled again with LIGHT, such a map gets new light, calculated without the original lights. To change only entities - spawn points, NPCs, items, scripts, keys - keep the original light instead. Three own functions bring back more: the [[karte_oeffnen#editordaten|editor data]] of a map built with {{KeEdMenue}} (everything, exactly), [[karte_oeffnen#modelle|misc_model from the geometry]] and [[karte_oeffnen#lichter|estimated lights]].":
        "Un mapa compilado no es el `.map` original. Vuelven los brushes con sus texturas, los patches y las entidades. Lo que q3map2 hornea en el .bsp no vuelve: los modelos de las entidades `misc_model`, las calcomanías y las entidades de luz; los mapas compilados para Jedi Academy normalmente ya no las contienen, su luz solo existe en los lightmaps y en la rejilla de luz. En algunas caras la alineación de la textura puede diferir un poco. Compilado de nuevo con LIGHT, un mapa así recibe luz nueva, calculada sin las luces originales. Para cambiar solo entidades (puntos de aparición, NPC, objetos, scripts, claves), conserva en cambio la luz original. Tres funciones propias recuperan más: los [[karte_oeffnen#editordaten|datos del editor]] de un mapa compilado con {{KeEdMenue}} (todo, exacto), [[karte_oeffnen#modelle|misc_model a partir de la geometría]] y [[karte_oeffnen#lichter|luces estimadas]].",
    "What a .bsp no longer contains":
        "Lo que ya no contiene un .bsp",
    "q3map2 removes some entities while it builds the map, so they cannot come back from the .bsp - this is not a fault of the conversion. An example is a Jedi Academy map from a tester: of 388 lights 9 came back, of 265 `misc_model` none, all 233 `info_null` are there.":
        "q3map2 quita algunas entidades mientras compila el mapa, así que no pueden volver desde el .bsp; no es un fallo de la conversión. Un ejemplo es un mapa de Jedi Academy de un probador: de 388 luces volvieron 9, de 265 `misc_model` ninguno, y están los 233 `info_null`.",
    "**Lights without a targetname**: only lights that can be switched in the game stay in the .bsp; q3map2 strips all others after calculating their light into the lightmaps (q3map2 `writebsp.cpp`, SetLightStyles). In the example these are the 9 lights with a targetname. [[karte_oeffnen#lichter|{{KeLsOpt}}]] places estimated lights instead.":
        "**Luces sin targetname**: solo las luces que se pueden conmutar en el juego se quedan en el .bsp; q3map2 quita todas las demás tras calcular su luz en los lightmaps (q3map2 `writebsp.cpp`, SetLightStyles). En el ejemplo son las 9 luces con targetname. [[karte_oeffnen#lichter|{{KeLsOpt}}]] coloca en su lugar luces estimadas.",
    "**`misc_model`**: q3map2 bakes the model into the geometry of the map and removes the entity (`model.cpp`, AddTriangleModels; `bspfile_abstract.cpp`, UnparseEntities - unless the map was built with `-keepmodels`). The same happens to `_decal` and `_skybox`. [[karte_oeffnen#modelle|{{KeRkModelle}}]] finds the models in the geometry again.":
        "**`misc_model`**: q3map2 hornea el modelo en la geometría del mapa y quita la entidad (`model.cpp`, AddTriangleModels; `bspfile_abstract.cpp`, UnparseEntities; salvo que el mapa se compilara con `-keepmodels`). Lo mismo les pasa a `_decal` y `_skybox`. [[karte_oeffnen#modelle|{{KeRkModelle}}]] vuelve a encontrar los modelos en la geometría.",
    "**`func_group`**: its brushes go into worldspawn (`map.cpp`, MoveBrushesToWorld), the group itself is gone.":
        "**`func_group`**: sus brushes van a worldspawn (`map.cpp`, MoveBrushesToWorld) y el grupo en sí desaparece.",
    "**Layers, group keys and the exact texture alignment**: the .bsp has no layers; the keys of a `func_group` go with the group; the alignment of a brush face is rebuilt from the vertices of its surface (`-convert`) and can differ slightly.":
        "**Capas, claves de grupo y la alineación exacta de las texturas**: el .bsp no tiene capas; las claves de un `func_group` se van con el grupo; la alineación de una cara de brush se reconstruye a partir de los vértices de su superficie (`-convert`) y puede diferir un poco.",
    "**Faces without a brush**: the triangles of a model whose file is missing and faces of non-solid brushes are in the .bsp only as drawing surfaces. They come back neither as brush nor as model; [[karte_oeffnen#modelle|{{KeRkFlaechenOpt}}]] keeps them as thin brushes.":
        "**Caras sin brush**: los triángulos de un modelo cuyo archivo falta y las caras de brushes no sólidos solo están en el .bsp como superficies de dibujo. No vuelven ni como brush ni como modelo; [[karte_oeffnen#modelle|{{KeRkFlaechenOpt}}]] las conserva como brushes finos.",
    "**Everything else stays**, for example `info_null` (the targets of spotlights), triggers, doors and their keys. Hint, areaportal and skip brushes come back structural, as in the source.":
        "**Todo lo demás se queda**, por ejemplo `info_null` (los destinos de los focos), triggers, puertas y sus claves. Los brushes hint, areaportal y skip vuelven como estructurales, como en la fuente.",
    "**{{KeLFile}} > {{KeLImport}}** also takes a .bsp, as in NetRadiant-Custom: it is converted in the work folder and inserted into the open map.":
        "**{{KeLFile}} > {{KeLImport}}** también acepta un .bsp, como en NetRadiant-Custom: se convierte en la carpeta de trabajo y se inserta en el mapa abierto.",
    "Keeping the editor data":
        "Conservar los datos del editor",
    "For your own maps nothing has to get lost. Switch on **{{KeLBuild}} > {{KeEdMenue}}** (off by default, not in NetRadiant-Custom) before you build:":
        "En tus propios mapas no tiene por qué perderse nada. Activa **{{KeLBuild}} > {{KeEdMenue}}** (desactivado por defecto, no está en NetRadiant-Custom) antes de compilar:",
    "Every BSP step runs with `-keeplights`: all lights stay in the entity lump. The game frees lights without a targetname when the map starts (OpenJK `SP_light`), so they cost nothing in the game; the optimizer counts their bytes in the check **{{KeOptPrEnttext}}**.":
        "Cada paso BSP se ejecuta con `-keeplights`: todas las luces se quedan en el lump de entidades. El juego libera las luces sin targetname al empezar el mapa (OpenJK `SP_light`), así que no cuestan nada en el juego; el optimizador cuenta sus bytes en la comprobación **{{KeOptPrEnttext}}**.",
    "After the build `<map>.bhed` lies next to the .bsp: the exact source map with a fingerprint of the geometry of this .bsp. `misc_model` does not go into the .bsp - the game would spawn it as an empty entity - but into this file.":
        "Tras la compilación, `<mapa>.bhed` queda junto al .bsp: el mapa fuente exacto con una huella de la geometría de este .bsp. `misc_model` no va al .bsp (el juego lo haría aparecer como una entidad vacía), sino a este archivo.",
    "**{{KeLFile}} > {{KeLOpenBsp}}** finds the file next to the .bsp - also in the .pk3 files of the game - and, if the fingerprint fits, opens the source map instead of a decompile: `misc_model`, `func_group`, texture alignment, detail and layers are back. If it belongs to another build, the console says so and the .bsp is decompiled as usual.":
        "**{{KeLFile}} > {{KeLOpenBsp}}** encuentra el archivo junto al .bsp (también en los .pk3 del juego) y, si la huella coincide, abre el mapa fuente en vez de descompilar: vuelven `misc_model`, `func_group`, la alineación de texturas, el detalle y las capas. Si pertenece a otra compilación, la consola lo dice y el .bsp se descompila como siempre.",
    "Lightmaps, light grid, visibility and geometry are byte for byte the same as without the option - the self-test compares lumps 1 to 17; only the entity lump grows. Put the .bhed into your .pk3 next to the .bsp if others should be able to open the map in the editor.":
        "Los lightmaps, la rejilla de luz, la visibilidad y la geometría son idénticos byte a byte a los de sin la opción (el autotest compara los lumps 1 a 17); solo crece el lump de entidades. Pon el .bhed en tu .pk3 junto al .bsp si otros deben poder abrir el mapa en el editor.",
    "Recovering misc_model from the geometry":
        "Recuperar misc_model a partir de la geometría",
    "Without editor data the models are only triangles in the .bsp. With **{{KeRkModelle}}** in the dialog {{KeBspTitel}} (on by default, not in NetRadiant-Custom) the editor looks for them after opening, in the background with a progress window:":
        "Sin datos del editor, los modelos son solo triángulos en el .bsp. Con **{{KeRkModelle}}** en el diálogo {{KeBspTitel}} (activado por defecto, no está en NetRadiant-Custom) el editor los busca tras abrir, en segundo plano con una ventana de progreso:",
    "The candidates are the model files of the game in the folders of the map's shaders. bhed_q3map2 loads them exactly as the compiler did (`-bhedmodels`, the same model loader as `-bsp`).":
        "Los candidatos son los archivos de modelos del juego en las carpetas de los shaders del mapa. bhed_q3map2 los carga exactamente como lo hizo el compilador (`-bhedmodels`, el mismo cargador de modelos que `-bsp`).",
    "Each model is searched in the triangles of the .bsp - also when q3map2 merged it into larger surfaces (`forcemeta`). Only **exact** matches become a `misc_model` with `origin`, `angle` or `angles` and `modelscale`; the console names the largest vertex error.":
        "Cada modelo se busca en los triángulos del .bsp, también cuando q3map2 lo fusionó en superficies mayores (`forcemeta`). Solo las coincidencias **exactas** se convierten en un `misc_model` con `origin`, `angle` o `angles` y `modelscale`; la consola nombra el mayor error de vértice.",
    "The models come as one undo step. Their triangles are not doubled: the decompile has no brushes for them, so {{KeBmNurEntities}} still works. If brushes or patches change before the search is done (for example with {{KeLImport}}), the result is dropped - open the .bsp again.":
        "Los modelos llegan como un solo paso de deshacer. Sus triángulos no se duplican: la descompilación no tiene brushes para ellos, así que {{KeBmNurEntities}} sigue funcionando. Si los brushes o patches cambian antes de que termine la búsqueda (por ejemplo con {{KeLImport}}), el resultado se descarta: vuelve a abrir el .bsp.",
    "The test map built without editor data and opened as .bsp: the pillar is back as `misc_model` with its angle and scale.":
        "El mapa de prueba compilado sin datos del editor y abierto como .bsp: la columna vuelve como `misc_model` con su ángulo y su escala.",
    "On jeditemple_garden (tester map, source known) all 265 of 265 `misc_model` came back with no wrong model; the largest difference to the source is 0.002 units in the position and 0.0002° in the angle. Original maps of the game have models from other folders too; what is not found stays triangles.":
        "En jeditemple_garden (mapa de un probador, fuente conocida) volvieron los 265 de 265 `misc_model` sin ningún modelo erróneo; la mayor diferencia con la fuente es de 0,002 unidades en la posición y 0,0002° en el ángulo. Los mapas originales del juego también tienen modelos de otras carpetas; lo que no se encuentra se queda en triángulos.",
    "**{{KeRkFlaechenOpt}}** (on by default): triangles that belong to no recognized model and lie on no brush become thin detail brushes with the texture `system/nodraw` on their sides in the layer **{{KeRkEbene}}**. Check them - hide the layer or delete what you do not need; they can double geometry that exists in another form.":
        "**{{KeRkFlaechenOpt}}** (activado por defecto): los triángulos que no pertenecen a ningún modelo reconocido ni están sobre ningún brush se convierten en brushes de detalle finos con la textura `system/nodraw` en sus lados, en la capa **{{KeRkEbene}}**. Revísalos: oculta la capa o borra lo que no necesites; pueden duplicar geometría que existe de otra forma.",
    "Estimating lights (experimental)":
        "Estimar luces (experimental)",
    "**{{KeLsOpt}}** in the dialog {{KeBspTitel}} (off by default, not in NetRadiant-Custom) places lights where the light grid of the .bsp says light comes from: from each grid point the directed light is followed back until it hits geometry, the points where many rays meet become lights, their strength and colour come from the grid. They go into the layer **{{KeLsEbene}}** and carry the key `_bhed_geschaetzt 1` (q3map2 and the game ignore it). Lights that stayed in the .bsp are not doubled.":
        "**{{KeLsOpt}}** en el diálogo {{KeBspTitel}} (desactivado por defecto, no está en NetRadiant-Custom) coloca luces donde la rejilla de luz del .bsp dice que viene la luz: desde cada punto de la rejilla se sigue la luz dirigida hacia atrás hasta que toca geometría; los puntos donde se cruzan muchos rayos se convierten en luces, y su intensidad y color vienen de la rejilla. Van a la capa **{{KeLsEbene}}** y llevan la clave `_bhed_geschaetzt 1` (q3map2 y el juego la ignoran). Las luces que se quedaron en el .bsp no se duplican.",
    "The test map without editor data: the estimated lights in the 2D view (the original had three lights on a line, one of them switchable and kept in the .bsp).":
        "El mapa de prueba sin datos del editor: las luces estimadas en la vista 2D (el original tenía tres luces en línea, una de ellas conmutable y conservada en el .bsp).",
    "This is an **estimate**, not the original lights. On jeditemple_garden 279 lights were estimated for 388 in the source; baked again with them, the lightmaps differ from the original by 38 of 255 steps on average (baked again with the true lights: 9), and only 2 estimated lights lie within 32 units of a source light. Use the estimated lights as a starting point and adjust them by eye - or build your own maps with {{KeEdMenue}}.":
        "Es una **estimación**, no las luces originales. En jeditemple_garden se estimaron 279 luces para 388 en la fuente; horneados de nuevo con ellas, los lightmaps difieren del original en 38 de 255 pasos de media (horneados de nuevo con las luces verdaderas: 9), y solo 2 luces estimadas están a menos de 32 unidades de una luz de la fuente. Usa las luces estimadas como punto de partida y ajústalas a ojo, o compila tus propios mapas con {{KeEdMenue}}.",
    "Keeping the original light":
        "Conservar la luz original",
    "{{KeLBuild}} > {{KeBmNurEntities}} - at the bottom of the menu, available after {{KeLOpenBsp}}.":
        "{{KeLBuild}} > {{KeBmNurEntities}}: al final del menú, disponible tras {{KeLOpenBsp}}.",
    "Change entities: keys and values (also of worldspawn), move, add or delete point entities such as spawn points, NPC spawners and items, change the keys of brush entities.":
        "Cambia entidades: claves y valores (también de worldspawn), mueve, añade o elimina entidades de punto como puntos de aparición, NPC spawners y objetos, cambia las claves de las entidades de brushes.",
    "**{{KeLBuild}} > {{KeBmNurEntities}}** saves the map, takes a fresh copy of the original .bsp and runs q3map2 `-onlyents` in the background (output in the console, as when building). Only the entity lump is rebuilt; the editor puts it into the copy of the original, so every other lump - geometry, lightmaps, light grid, visibility - stays byte for byte as it was.":
        "**{{KeLBuild}} > {{KeBmNurEntities}}** guarda el mapa, toma una copia nueva del .bsp original y ejecuta q3map2 `-onlyents` en segundo plano (salida en la consola, como al compilar). Solo se reconstruye el lump de entidades; el editor lo pone en la copia del original, así que todos los demás lumps (geometría, lightmaps, rejilla de luz, visibilidad) quedan byte a byte como estaban.",
    "The console confirms it with **Entities updated** and the number of unchanged lumps. The result is `<name>.bsp` in the work folder; the untouched original stays next to it as `<name>_original.bsp`.":
        "La consola lo confirma con **Entidades actualizadas** y el número de lumps sin cambios. El resultado es `<nombre>.bsp` en la carpeta de trabajo; el original intacto queda a su lado como `<nombre>_original.bsp`.",
    "To play it, copy the new .bsp into the `maps` folder of the game as described on [[karte_bauen]] - for a map from a .pk3 best under a new name, so the original in the .pk3 stays as it is (for example `duel1_neu.bsp` in `maps\\mp`, started with `devmap mp/duel1_neu`).":
        "Para jugarlo, copia el .bsp nuevo en la carpeta `maps` del juego como se describe en [[karte_bauen]]; para un mapa de un .pk3, mejor con un nombre nuevo, para que el original del .pk3 quede como está (por ejemplo `duel1_nuevo.bsp` en `maps\\mp`, iniciado con `devmap mp/duel1_nuevo`).",
    "Brushes, patches, brush entities and baked entities such as `misc_model` are part of the geometry. If any of them changed since the map was opened, **{{KeBmNurEntities}}** does not run q3map2; the box **{{KeBmGeometrieTitel}}** explains that the map has to be built again with BSP, VIS and LIGHT - with new light. Undo brings the geometry back to the original, and the entry works again.":
        "Los brushes, patches, entidades de brushes y entidades horneadas como `misc_model` son parte de la geometría. Si alguno ha cambiado desde que se abrió el mapa, **{{KeBmNurEntities}}** no ejecuta q3map2; el cuadro **{{KeBmGeometrieTitel}}** explica que el mapa debe compilarse de nuevo con BSP, VIS y LIGHT, con luz nueva. Deshacer devuelve la geometría al original y la entrada vuelve a funcionar.",
    "Lights with a targetname can be switched in the game; q3map2 gives each of them its own light style and bakes its light into extra lightmap stages. If such a light was added, removed, renamed or got another style since the map was opened, **{{KeBmNurEntities}}** does not run q3map2 either: the styles would be numbered anew while the lightmaps keep the old ones, and the wrong lights would switch. Build the map in full instead.":
        "Las luces con targetname se pueden conmutar en el juego; q3map2 da a cada una su propio estilo de luz y hornea su luz en etapas de lightmap extra. Si una luz así se añadió, se quitó, se renombró o recibió otro estilo desde que se abrió el mapa, **{{KeBmNurEntities}}** tampoco ejecuta q3map2: los estilos se numerarían de nuevo mientras los lightmaps conservan los antiguos, y se conmutarían las luces equivocadas. Compila en cambio el mapa completo.",
    "Jedi Outcast maps keep their light entities in the .bsp. If the original contains lights, q3map2 runs with `-keeplights` so that they stay. NetRadiant-Custom's own **Entity-Only compile** in the same menu also runs `-onlyents`, but without the check and without putting the result into the original - with Jedi Outcast maps it can drop lights and rewrite the light grid. For a map opened with **{{KeLOpenBsp}}**, use **{{KeBmNurEntities}}**.":
        "Los mapas de Jedi Outcast conservan sus entidades de luz en el .bsp. Si el original contiene luces, q3map2 se ejecuta con `-keeplights` para que se queden. El propio **Entity-Only compile** de NetRadiant-Custom en el mismo menú también ejecuta `-onlyents`, pero sin la comprobación y sin poner el resultado en el original: con mapas de Jedi Outcast puede perder luces y reescribir la rejilla de luz. Para un mapa abierto con **{{KeLOpenBsp}}**, usa **{{KeBmNurEntities}}**.",
    "Map editor: plugins":
        "Editor de mapas: plugins",
    "The {{KeLPlugins}} menu has every plugin of NetRadiant-Custom - bobToolz, brushexport2, MeshTex, ShaderPlug, SunPlug, UFO:AI and prtview - with all their entries and dialogs, ported from the source code.":
        "El menú {{KeLPlugins}} tiene todos los plugins de NetRadiant-Custom (bobToolz, brushexport2, MeshTex, ShaderPlug, SunPlug, UFO:AI y prtview) con todas sus entradas y diálogos, portados del código fuente.",
    "The {{KeLPlugins}} menu":
        "El menú {{KeLPlugins}}",
    "{{KeLPlugins}} > bobToolz: the entries of NetRadiant-Custom, with their icons.":
        "{{KeLPlugins}} > bobToolz: las entradas de NetRadiant-Custom, con sus iconos.",
    "The entries keep the English names of NetRadiant-Custom, as the plugins have them there; the dialogs are in the program language - with **{{KeNrcTerms}}** in NetRadiant-Custom's English. Every plugin has an **About** entry. Messages go to the console, errors in red. Each change a plugin makes is one undo step.":
        "Las entradas conservan los nombres ingleses de NetRadiant-Custom, como los tienen allí los plugins; los diálogos están en el idioma del programa, con **{{KeNrcTerms}}** en el inglés de NetRadiant-Custom. Cada plugin tiene una entrada **About**. Los mensajes van a la consola, los errores en rojo. Cada cambio que hace un plugin es un paso de deshacer.",
    "bobToolz":
        "bobToolz",
    "Entry":
        "Entrada",
    "Flip terrain":
        "Flip terrain",
    "Two selected terrain brushes (triangular prisms with a common edge): turns their common diagonal the other way.":
        "Dos brushes de terreno seleccionados (prismas triangulares con una arista común): gira su diagonal común al otro lado.",
    "Split patch rows / columns, Split patch":
        "Split patch rows / columns, Split patch",
    "Cuts the selected patch into strips along its rows, its columns, or both.":
        "Corta el patch seleccionado en tiras a lo largo de sus filas, sus columnas o ambas.",
    "Merge Patches":
        "Merge Patches",
    "Joins two or more selected patches that share an edge into one.":
        "Une en uno dos o más patches seleccionados que comparten una arista.",
    "Plot Splines":
        "Plot Splines",
    "Draws the paths of `info_train_spline_main` chains with their `info_train_spline_control` points (see Make Chain).":
        "Dibuja los trayectos de las cadenas `info_train_spline_main` con sus puntos `info_train_spline_control` (mira Make Chain).",
    "Tree Planter, Drop Entity":
        "Tree Planter, Drop Entity",
    "Select the ground brushes and switch **Tree Planter** on: each left click in the XY view puts an entity from `data\\karte\\bt\\tp_ent.txt` onto them, with a random model, angle and scale (as shipped, the file names tree models from Quake - put your own models there). **Drop Entity** drops the selected entities onto the same brushes.":
        "Selecciona los brushes del suelo y activa **Tree Planter**: cada clic izquierdo en la vista XY pone sobre ellos una entidad de `data\\karte\\bt\\tp_ent.txt`, con un modelo, ángulo y escala aleatorios (tal como se distribuye, el archivo nombra modelos de árboles de Quake; pon ahí tus propios modelos). **Drop Entity** deja caer las entidades seleccionadas sobre los mismos brushes.",
    "Explode Entity":
        "Explode Entity",
    "Splits the selected brush entity: each of its brushes becomes an entity of its own with the same keys (not worldspawn).":
        "Divide la entidad de brushes seleccionada: cada uno de sus brushes se convierte en una entidad propia con las mismas claves (no worldspawn).",
    "Caulk Selection":
        "Caulk Selection",
    "Gives every face of the selected brushes and patches the caulk texture.":
        "Da a cada cara de los brushes y patches seleccionados la textura caulk.",
    "Polygon Builder":
        "Polygon Builder",
    "Dialog **{{KeBtPolygon}}**: turns one selected brush into a prism with the given number of sides - optionally hollow with a border, inverse, aligned to the top edge.":
        "Diálogo **{{KeBtPolygon}}**: convierte un brush seleccionado en un prisma con el número de lados indicado; opcionalmente hueco con borde, inverso o alineado a la arista superior.",
    "Brush Cleanup":
        "Brush Cleanup",
    "Removes invalid and duplicate planes from all brushes of the map and reports how many.":
        "Quita los planos no válidos y duplicados de todos los brushes del mapa e informa de cuántos.",
    "Vis Viewer":
        "Vis Viewer",
    "Reads the compiled `<map>.bsp` next to the map - after a build, or the original after {{KeLOpenBsp}} - and shows which BSP leafs are visible from the camera or the selection; the window **{{KeBtBspLeafs}}** lists them.":
        "Lee el `<mapa>.bsp` compilado junto al mapa (tras una compilación, o el original tras {{KeLOpenBsp}}) y muestra qué hojas del BSP son visibles desde la cámara o la selección; la ventana **{{KeBtBspLeafs}}** las lista.",
    "PitOMatic":
        "PitOMatic",
    "Turns one selected brush into a death pit: triggers for the falling scream (`*falling1.wav`) and for removing powerups, a `trigger_hurt` at the bottom.":
        "Convierte un brush seleccionado en un foso mortal: triggers para el grito de caída (`*falling1.wav`) y para quitar los potenciadores, un `trigger_hurt` en el fondo.",
    "Reset Textures...":
        "Reset Textures...",
    "Dialog **{{KeBtTexReset}}**: replaces a texture name and resets scale, shift and rotation - on the selected or on all brushes.":
        "Diálogo **{{KeBtTexReset}}**: sustituye un nombre de textura y restablece escala, desplazamiento y rotación, en los brushes seleccionados o en todos.",
    "Path Plotter...":
        "Path Plotter...",
    "Shows the flight path of a `trigger_push` - see below.":
        "Muestra la trayectoria de vuelo de un `trigger_push`; mira abajo.",
    "Make Chain...":
        "Make Chain...",
    "Dialog **{{KeBtMakeChain}}**: builds a chain of `info_train_spline_main` entities with control points, named after the base name.":
        "Diálogo **{{KeBtMakeChain}}**: construye una cadena de entidades `info_train_spline_main` con puntos de control, nombradas según el nombre base.",
    "Intersect..., Find Duplicates":
        "Intersect..., Find Duplicates",
    "Select brushes that intersect each other (in the whole map or among the selected ones, optionally only duplicates), or every brush that exactly duplicates another.":
        "Selecciona los brushes que se cortan entre sí (en todo el mapa o entre los seleccionados, opcionalmente solo duplicados), o cada brush que duplica exactamente a otro.",
    "Door Builder...":
        "Door Builder...",
    "Turns one selected brush into two door halves (`func_door`) with door and trim textures from `bt\\door-tex*.txt`, North-South or East-West.":
        "Convierte un brush seleccionado en dos hojas de puerta (`func_door`) con texturas de puerta y de marco de `bt\\door-tex*.txt`, norte-sur o este-oeste.",
    "Aperture Door...":
        "Aperture Door...",
    "Builds an aperture door (an iris of segments) from the selected brush - from parameters or from a winding.":
        "Construye una puerta de diafragma (un iris de segmentos) a partir del brush seleccionado, desde parámetros o desde un contorno.",
    "Stair Builder...":
        "Stair Builder...",
    "Turns one selected brush into stairs: step height, direction, style (Original, Bob's Style, Corner Style), main and riser texture, optionally detail brushes.":
        "Convierte un brush seleccionado en una escalera: altura del escalón, dirección, estilo (Original, Bob's Style, Corner Style), textura principal y de contrahuella, opcionalmente brushes de detalle.",
    "Path Plotter":
        "Path Plotter",
    "Select a `trigger_push` that has a `target`.":
        "Selecciona un `trigger_push` que tenga `target`.",
    "**{{KeLPlugins}} > bobToolz > Path Plotter...** opens **{{KeBtPathPlotter}}**: number of points, distance multiplier and gravity of the path. **{{KeBtEnable}}** draws it in the 2D views and the camera.":
        "**{{KeLPlugins}} > bobToolz > Path Plotter...** abre **{{KeBtPathPlotter}}**: número de puntos, multiplicador de distancia y gravedad del trayecto. **{{KeBtEnable}}** lo dibuja en las vistas 2D y en la cámara.",
    "The dialog {{KeBtPathPlotter}} with NetRadiant-Custom's values.":
        "El diálogo {{KeBtPathPlotter}} con los valores de NetRadiant-Custom.",
    "Move the target - the path follows, unless **{{KeBtNoUpdate}}** is ticked. **{{KeBtDisable}}** removes it.":
        "Mueve el destino: el trayecto lo sigue, salvo que esté marcado **{{KeBtNoUpdate}}**. **{{KeBtDisable}}** lo quita.",
    "brushexport2":
        "brushexport2",
    "The window {{KeBeTitel}}.":
        "La ventana {{KeBeTitel}}.",
    "**Export selected as Wavefront Object** writes the selected brushes as a `.obj` model, for example to continue them in a modelling program. The window chooses how faces are grouped (**{{KeBeCollapse}}**, **{{KeBeCollapseMat}}**, **{{KeBeNoCollapse}}**), which materials to leave out (**{{KeBeIgnored}}** - for example `textures/common/caulk`), and whether to shorten material names, write objects instead of groups, weld vertices and write a `.mtl` file. **{{KeBeSaveAs}}** asks for the file - the first time in the folder `karte_export` in the settings folder - and **{{KeBeSave}}** writes the same file again.":
        "**Export selected as Wavefront Object** escribe los brushes seleccionados como modelo `.obj`, por ejemplo para continuarlos en un programa de modelado. La ventana elige cómo se agrupan las caras (**{{KeBeCollapse}}**, **{{KeBeCollapseMat}}**, **{{KeBeNoCollapse}}**), qué materiales dejar fuera (**{{KeBeIgnored}}**, por ejemplo `textures/common/caulk`) y si acortar los nombres de los materiales, escribir objetos en vez de grupos, soldar vértices y escribir un archivo `.mtl`. **{{KeBeSaveAs}}** pide el archivo (la primera vez en la carpeta `karte_export` de la carpeta de ajustes) y **{{KeBeSave}}** vuelve a escribir el mismo archivo.",
    "MeshTex":
        "MeshTex",
    "MeshTex > Set S/T Scale...":
        "MeshTex > Set S/T Scale...",
    "MeshTex lays textures onto patches - curves, arches, terrain - where the Surface Inspector reaches its limits. **{{KeMtSetScale}}** sets the scale along S and T: natural (as on a brush face, `*`) or as a number of tiles, counted from a \"zero\" column or row, optionally measured along a reference row or column. **{{KeMtGetInfo}}** shows the scale of the selected patch and can hand it to {{KeMtSetScale}}. **{{KeMtGenFunc}}** sets S and T as a linear function of the row and column distances. The **Align** entries fit the texture to the patch edges (minimum or maximum, stretched or shrunk, or automatically); **Help...** shows MeshTex's own explanation.":
        "MeshTex coloca texturas en patches (curvas, arcos, terreno) donde el inspector de superficies llega a sus límites. **{{KeMtSetScale}}** fija la escala a lo largo de S y T: natural (como en una cara de brush, `*`) o como número de mosaicos, contados desde una columna o fila \"cero\", opcionalmente medidos a lo largo de una fila o columna de referencia. **{{KeMtGetInfo}}** muestra la escala del patch seleccionado y puede pasarla a {{KeMtSetScale}}. **{{KeMtGenFunc}}** fija S y T como función lineal de las distancias de filas y columnas. Las entradas **Align** ajustan la textura a los bordes del patch (mínimo o máximo, estirada o encogida, o automáticamente); **Help...** muestra la propia explicación de MeshTex.",
    "ShaderPlug, SunPlug, UFO:AI, prtview":
        "ShaderPlug, SunPlug, UFO:AI, prtview",
    "Plugin":
        "Plugin",
    "ShaderPlug":
        "ShaderPlug",
    "**Create tag file** writes the tag file of the texture browser: every shader gets the name of its `.shader` file as a tag. The texture browser loads it again at once (NetRadiant-Custom only after a restart).":
        "**Create tag file** escribe el archivo de etiquetas del navegador de texturas: cada shader recibe como etiqueta el nombre de su archivo `.shader`. El navegador de texturas lo vuelve a cargar enseguida (NetRadiant-Custom solo tras reiniciar).",
    "SunPlug":
        "SunPlug",
    "In NetRadiant-Custom it contains **ET-MapCoordinator**: it calculates `mapcoordsmins` and `mapcoordsmaxs` for the command map of Enemy Territory from the size of the map and writes them into worldspawn with **{{KeSunSet}}**. Jedi Academy does not use these keys.":
        "En NetRadiant-Custom contiene **ET-MapCoordinator**: calcula `mapcoordsmins` y `mapcoordsmaxs` para el mapa de mando de Enemy Territory a partir del tamaño del mapa y los escribe en worldspawn con **{{KeSunSet}}**. Jedi Academy no usa estas claves.",
    "UFO:AI":
        "UFO:AI",
    "Tools for the game UFO:AI: filters by its content flags (NoDraw, WeaponClip, ActorClip, StepOn, Level 1 to 8), **Perform check** and its worldspawn keys (**Worldspawn**, **Worldspawn reset**). They are there because NetRadiant-Custom shows them for every game.":
        "Herramientas para el juego UFO:AI: filtros por sus marcas de contenido (NoDraw, WeaponClip, ActorClip, StepOn, Level 1 a 8), **Perform check** y sus claves de worldspawn (**Worldspawn**, **Worldspawn reset**). Están porque NetRadiant-Custom las muestra para cada juego.",
    "prtview":
        "prtview",
    "Shows the portals of the last BSP stage - see [[karte_bauen]].":
        "Muestra los portales de la última etapa BSP; mira [[karte_bauen]].",
    "open: no Jedi Outcast source - checked against the Jedi Academy value":
        "pendiente: sin código fuente de Jedi Outcast, comprobado con el valor de Jedi Academy",
    "Limit":
        "Límite",
    "In the source":
        "En el código fuente",
    "Allowed":
        "Permitido",
    "Kind":
        "Tipo",
    "Exceeded":
        "Excedido",
    "Source (file:line)":
        "Fuente (archivo:línea)",
    "Map editor: optimizer and limits":
        "Editor de mapas: optimizador y límites",
    "**{{KeLBuild}} > {{KeOptMenue}}** counts what the map uses against every limit of Jedi Academy, Movie Duels and Jedi Outcast - each limit taken from the source code of the engine -, finds build problems such as leaks, uncaulked hidden faces and patches with too many collision planes, and fixes many of them at the push of a button.":
        "**{{KeLBuild}} > {{KeOptMenue}}** cuenta lo que usa el mapa frente a cada límite de Jedi Academy, Movie Duels y Jedi Outcast (cada límite sacado del código fuente del motor), encuentra problemas de compilación como fugas, caras ocultas sin caulk y patches con demasiados planos de colisión, y corrige muchos de ellos con solo pulsar un botón.",
    "Opening the optimizer":
        "Abrir el optimizador",
    "{{KeLBuild}} > **{{KeOptMenue}}** - its own entry, NetRadiant-Custom has no optimizer.":
        "{{KeLBuild}} > **{{KeOptMenue}}**: una entrada propia, NetRadiant-Custom no tiene optimizador.",
    "**{{KeLBuild}} > {{KeOptMenue}}** opens the window **{{KeOptTitel}}** and analyzes the map in the background - also the largest maps; the editor stays usable meanwhile. The console shows a summary at the end.":
        "**{{KeLBuild}} > {{KeOptMenue}}** abre la ventana **{{KeOptTitel}}** y analiza el mapa en segundo plano, también los mapas más grandes; el editor se puede seguir usando mientras tanto. La consola muestra un resumen al final.",
    "**{{KeOptRegelwerk}}** chooses the limits: **{{KeOptAuto}}** takes the game the editor works with (Jedi Academy, Movie Duels or Jedi Outcast); Jedi Academy multiplayer can be chosen as well.":
        "**{{KeOptRegelwerk}}** elige los límites: **{{KeOptAuto}}** toma el juego con el que trabaja el editor (Jedi Academy, Movie Duels o Jedi Outcast); también se puede elegir Jedi Academy multijugador.",
    "If the map was built (see [[karte_bauen]]) or opened as a compiled map ([[karte_oeffnen]]), the .bsp is analyzed too: light grid, lightmaps, visibility, triangles per view, the leak line of the last BSP stage. Without a .bsp only the map is counted.":
        "Si el mapa se compiló (mira [[karte_bauen]]) o se abrió como mapa compilado ([[karte_oeffnen]]), también se analiza el .bsp: rejilla de luz, lightmaps, visibilidad, triángulos por vista, la línea de fuga de la última etapa BSP. Sin .bsp solo se cuenta el mapa.",
    "On the page **{{KePgOptimizer}}** of **{{KeLPreferences}}** the optimizer can run by itself after every build; it opens only when there are problems.":
        "En la página **{{KePgOptimizer}}** de **{{KeLPreferences}}** el optimizador se puede ejecutar solo tras cada compilación; solo se abre cuando hay problemas.",
    "Limits":
        "Límites",
    "The tab **{{KeOptReiterGrenzen}}** with mp/duel1: each limit with value, allowed value, share and traffic light; on the right where the limit is in the source and the largest contributors.":
        "La pestaña **{{KeOptReiterGrenzen}}** con mp/duel1: cada límite con valor, valor permitido, porción y semáforo; a la derecha, dónde está el límite en el código fuente y los que más aportan.",
    "**{{KeOptOk}}** - well below the limit. **{{KeOptKnapp}}** - from 90 % of it (adjustable). **{{KeOptUeber}}** - over a limit the game enforces: the map does not load, crashes or loses something.":
        "**{{KeOptOk}}**: muy por debajo del límite. **{{KeOptKnapp}}**: a partir del 90 % (ajustable). **{{KeOptUeber}}**: por encima de un límite que el juego impone: el mapa no carga, se cuelga o pierde algo.",
    "**{{KeOptHinweis}}** - a format limit of the .bsp file (`MAX_MAP_*` in `qfiles.h`) that neither the engine nor q3map2 checks. Raven's own maps exceed several of them, so the optimizer never calls them over - it only notes them.":
        "**{{KeOptHinweis}}**: un límite del formato del archivo .bsp (`MAX_MAP_*` en `qfiles.h`) que ni el motor ni q3map2 comprueban. Los propios mapas de Raven superan varios, así que el optimizador nunca los da por excedidos: solo los anota.",
    "Values marked * are estimates (for example the entities that remain after spawning, or what a model costs). The column **{{KeOptSpAus}}** says whether a value was counted in the map or in the .bsp.":
        "Los valores marcados con * son estimaciones (por ejemplo las entidades que quedan tras aparecer, o lo que cuesta un modelo). La columna **{{KeOptSpAus}}** dice si un valor se contó en el mapa o en el .bsp.",
    "A click on a limit selects the largest contributors in the map and centers the views on them.":
        "Un clic en un límite selecciona en el mapa a los que más aportan y centra las vistas en ellos.",
    "Every limit is taken from the source code, not estimated: OpenJK for Jedi Academy (single player `code/`, multiplayer `codemp/`), the source of Movie Duels for Movie Duels, OpenJK with `JK2_MODE` and `codeJK2/` for Jedi Outcast. Where a mod raises a limit, the optimizer still checks against the stricter vanilla value (for example 2048 instead of 4096 patch planes). The tables at the end of the page list each limit with file and line; the test `karte_grenzen` reads these lines from the sources again.":
        "Cada límite sale del código fuente, no se estima: OpenJK para Jedi Academy (un jugador `code/`, multijugador `codemp/`), el código de Movie Duels para Movie Duels, OpenJK con `JK2_MODE` y `codeJK2/` para Jedi Outcast. Donde un mod sube un límite, el optimizador sigue comprobando con el valor original, más estricto (por ejemplo 2048 en vez de 4096 planos de patch). Las tablas al final de la página listan cada límite con archivo y línea; el test `karte_grenzen` vuelve a leer esas líneas de las fuentes.",
    "Checks and fixes":
        "Comprobaciones y correcciones",
    "The tab **{{KeOptReiterBefunde}}**: each finding with severity, count and fix; on the right what it means, its source and the objects.":
        "La pestaña **{{KeOptReiterBefunde}}**: cada hallazgo con gravedad, cantidad y corrección; a la derecha, qué significa, su fuente y los objetos.",
    "A click on a finding selects its objects and centers the 2D views and the camera on them.":
        "Un clic en un hallazgo selecciona sus objetos y centra en ellos las vistas 2D y la cámara.",
    "**{{KeOptBeheben}}** shows what would change; **{{KeOptAnwenden}}** does it. Every fix is one step of **Undo**, and the map is counted again afterwards - the finding disappears or shrinks.":
        "**{{KeOptBeheben}}** muestra lo que cambiaría; **{{KeOptAnwenden}}** lo hace. Cada corrección es un paso de **Deshacer**, y después se vuelve a contar el mapa: el hallazgo desaparece o se reduce.",
    "The preview of a fix before it is applied.":
        "La vista previa de una corrección antes de aplicarla.",
    "**{{KeOptAllesSichere}}** applies only the fixes marked {{KeOptSicher}} - the ones that cannot change how the map looks or plays - in a single undo step.":
        "**{{KeOptAllesSichere}}** aplica solo las correcciones marcadas como {{KeOptSicher}} (las que no pueden cambiar cómo se ve o se juega el mapa) en un solo paso de deshacer.",
    "Fixes that set a worldspawn key (light grid, lightmaps) take effect with the next build; the console says so. **{{KeOptExport}}** writes everything with sources into a text file.":
        "Las correcciones que fijan una clave de worldspawn (rejilla de luz, lightmaps) surten efecto con la siguiente compilación; la consola lo dice. **{{KeOptExport}}** escribe todo con las fuentes en un archivo de texto.",
    "Check":
        "Comprobación",
    "What it means":
        "Qué significa",
    "Fix":
        "Corrección",
    "Safe":
        "Seguro",
    "Settings":
        "Ajustes",
    "{{KeLPreferences}}, page **{{KePgOptimizer}}** - a page of its own, NetRadiant-Custom has none.":
        "{{KeLPreferences}}, página **{{KePgOptimizer}}**: una página propia, NetRadiant-Custom no la tiene.",
    "**{{KePrOptKnapp}}** sets from which share a limit counts as near, **{{KePrOptNachBau}}** runs the optimizer after every build, **{{KePrOptRegelwerk}}** fixes the rule set, **{{KePrOptPortale}}** and **{{KePrOptDreiecke}}** are the thresholds for the VIS and triangle notes.":
        "**{{KePrOptKnapp}}** fija a partir de qué porción un límite cuenta como cercano, **{{KePrOptNachBau}}** ejecuta el optimizador tras cada compilación, **{{KePrOptRegelwerk}}** fija las reglas, **{{KePrOptPortale}}** y **{{KePrOptDreiecke}}** son los umbrales de las notas de VIS y de triángulos.",
    "The limits of each game":
        "Los límites de cada juego",
    "**{{KeOptSpErlaubt}}** is the value the optimizer checks against - one less than the define where the engine itself takes a slot (for example the world model or the box model), and the stricter vanilla value where a mod raised it. The source column names every place: the define and the line that stops the game.":
        "**{{KeOptSpErlaubt}}** es el valor con el que comprueba el optimizador: uno menos que el define donde el propio motor ocupa una plaza (por ejemplo el modelo del mundo o el modelo de caja), y el valor original más estricto donde un mod lo subió. La columna de fuente nombra cada lugar: el define y la línea que detiene el juego.",
    "Jedi Academy (OpenJK SP)":
        "Jedi Academy (OpenJK SP)",
    "Jedi Academy MP (OpenJK)":
        "Jedi Academy MP (OpenJK)",
    "Movie Duels":
        "Movie Duels",
    "Jedi Outcast (OpenJK JK2_MODE)":
        "Jedi Outcast (OpenJK JK2_MODE)",
    "Report":
        "Informe",
    "NetRadiant-Custom":
        "NetRadiant-Custom",
    "JK Studio":
        "JK Studio",
    "After leaving vertex or edge mode the components stay selected; the next drag deforms the brush.":
        "Tras salir del modo vértices o aristas, los componentes siguen seleccionados; el siguiente arrastre deforma el brush.",
    "Leaving vertex or edge mode deselects its components.":
        "Salir del modo vértices o aristas deselecciona sus componentes.",
    "NRC#65":
        "NRC#65",
    "Bevel and inverted bevel caps can create two degenerate patches, depending on the patch orientation.":
        "Las tapas de bisel y de bisel invertido pueden crear dos patches degenerados, según la orientación del patch.",
    "The cap direction is chosen by size and shape, so every cap has a surface.":
        "La dirección de la tapa se elige por tamaño y forma, así cada tapa tiene superficie.",
    "NRC#302, GL#206":
        "NRC#302, GL#206",
    "A path with umlauts or other non-ASCII characters: the map does not open, the settings get damaged.":
        "Una ruta con diéresis u otros caracteres no ASCII: el mapa no se abre y los ajustes se dañan.",
    "Editor and bhed_q3map2 handle any path (e.g. C:\\Users\\Jörg\\...).":
        "El editor y bhed_q3map2 manejan cualquier ruta (p. ej. C:\\Users\\Jörg\\...).",
    "NRC#130":
        "NRC#130",
    "Saving with a relative engine path (../../Academy/) triggers an assertion.":
        "Guardar con una ruta del motor relativa (../../Academy/) dispara una aserción.",
    "Paths are made absolute when they are set.":
        "Las rutas se vuelven absolutas al fijarlas.",
    "NRC#164":
        "NRC#164",
    "AutoCaulk fails with \"reading ..._ac.caulk failure\" without a reason.":
        "AutoCaulk falla con \"reading ..._ac.caulk failure\" sin motivo.",
    "The console shows q3map2's output and the reason.":
        "La consola muestra la salida de q3map2 y el motivo.",
    "NRC#213":
        "NRC#213",
    "With the Jedi Academy shaders AutoCaulk puts the non-solid system/nodraw on solid brushes and uses watercaulk, which does not exist there.":
        "Con los shaders de Jedi Academy, AutoCaulk pone el system/nodraw no sólido en brushes sólidos y usa watercaulk, que allí no existe.",
    "The names of system.shader in Jedi Academy and Jedi Outcast: nodraw_solid, caulk_water, caulk_lava, caulk_acid; a shader the game lacks is left out. AutoCaulk runs q3map2 with the game of the editor (-game ja/jk2).":
        "Los nombres de system.shader en Jedi Academy y Jedi Outcast: nodraw_solid, caulk_water, caulk_lava, caulk_acid; un shader que le falta al juego se deja fuera. AutoCaulk ejecuta q3map2 con el juego del editor (-game ja/jk2).",
    "NRC#145":
        "NRC#145",
    "The Surface Inspector shows unnormalized brush-primitive shifts from other editors as the spinner limit (8192); an arrow or Enter then moves the texture.":
        "El inspector de superficies muestra los desplazamientos brush-primitive no normalizados de otros editores como el límite del selector (8192); una flecha o Intro mueve entonces la textura.",
    "Outside the spinner range the value is shown reduced by whole tiles - same alignment; the .map stays unchanged until a value is set.":
        "Fuera del rango del selector, el valor se muestra reducido en mosaicos enteros (misma alineación); el .map no cambia hasta que se fija un valor.",
    "NRC#261":
        "NRC#261",
    "Caps only work for certain patch sizes; the console says so in English only.":
        "Las tapas solo funcionan con ciertos tamaños de patch; la consola lo dice solo en inglés.",
    "Same limit (intended); below the English line a hint in the program language names the size each cap needs.":
        "Mismo límite (intencionado); bajo la línea en inglés, una indicación en el idioma del programa nombra el tamaño que necesita cada tapa.",
    "NRC#279":
        "NRC#279",
    "Text errors fixed after tag 20260114 (bobToolz Door Builder \"East - West\", \"Distance Multiplier\").":
        "Errores de texto corregidos tras la etiqueta 20260114 (bobToolz Door Builder \"East - West\", \"Distance Multiplier\").",
    "Taken over.":
        "Adoptado.",
    "NRC master":
        "NRC master",
    "Choosing a texture in the texture browser now really sets it as the shader of new brushes (it was never stored).":
        "Elegir una textura en el navegador de texturas ahora la fija de verdad como shader de los brushes nuevos (nunca se guardaba).",
    "The state of the tag window was stored twice and got lost; the **{{KePrefClean}}** button of the preferences had no function.":
        "El estado de la ventana de etiquetas se guardaba dos veces y se perdía; el botón **{{KePrefClean}}** de las preferencias no tenía función.",
    "Every menu entry, button, key and field was checked for its effect, and every command that changes the map undoes and redoes byte for byte.":
        "Se comprobó el efecto de cada entrada de menú, botón, tecla y campo, y cada comando que cambia el mapa deshace y rehace byte a byte.",
    "-convert (File > Open .bsp) writes hint, areaportal and skip brushes of Jedi Academy, Jedi Outcast and SoF2 maps as detail: in these games \"structural\" has no content flag, so the check for structural transparent brushes never applies.":
        "-convert (File > Abrir .bsp) escribe como detalle los brushes hint, areaportal y skip de los mapas de Jedi Academy, Jedi Outcast y SoF2: en estos juegos \"structural\" no tiene marca de contenido, así que la comprobación de brushes transparentes estructurales nunca se aplica.",
    "The compile flags of the shaders decide as when building (q3map_structural / surfaceparm structural): such brushes come back structural, as in the source map.":
        "Las marcas de compilación de los shaders deciden como al compilar (q3map_structural / surfaceparm structural): esos brushes vuelven como estructurales, como en el mapa fuente.",
    "Tester (Stage 7)":
        "Probador (etapa 7)",
    "Not deterministic: every build writes a different .bsp (shader pointers as sort keys, uninitialized lightmap coordinates, thread races in -vis and -light); the names of generated style shaders change with every build.":
        "No determinista: cada compilación escribe un .bsp distinto (punteros de shader como claves de orden, coordenadas de lightmap sin inicializar, carreras entre hilos en -vis y -light); los nombres de los shaders de estilo generados cambian en cada compilación.",
    "The BSP and light stages give the same result for the same map every time, with any number of threads equal to a 1-thread build; with one thread -light stays byte-identical to NRC. The vis stage is as in NRC: the same with one thread, with several threads its result may differ slightly from build to build.":
        "Las etapas BSP y de luz dan siempre el mismo resultado para el mismo mapa, con cualquier número de hilos igual a una compilación de 1 hilo; con un hilo, -light queda idéntico byte a byte a NRC. La etapa vis es como en NRC: igual con un hilo; con varios hilos su resultado puede diferir un poco de una compilación a otra.",
    "GL#173":
        "GL#173",
    "Dirtmapping (-dirty) darkens faces next to the sky as if the sky were a wall.":
        "El dirtmapping (-dirty) oscurece las caras junto al cielo como si el cielo fuera una pared.",
    "A dirt ray that leaves the map through the sky does not darken.":
        "Un rayo de suciedad que sale del mapa por el cielo no oscurece.",
    "NRC#101":
        "NRC#101",
    "A map name with a dot (test.q3, foo_0.1) given without .map loses the part after the dot.":
        "Un nombre de mapa con punto (test.q3, foo_0.1) dado sin .map pierde la parte tras el punto.",
    "Only q3map2's own extensions are replaced (the editor always passes full paths anyway).":
        "Solo se sustituyen las extensiones propias de q3map2 (el editor siempre pasa rutas completas de todos modos).",
    "NRC#102, GL#154":
        "NRC#102, GL#154",
    "Memory errors: a brush side without a plane reads mapplanes[-1]; a deluxel reference without a deluxemap.":
        "Errores de memoria: una cara de brush sin plano lee mapplanes[-1]; una referencia de deluxel sin deluxemap.",
    "Fixed (checked with AddressSanitizer and UBSan over the regression maps and all decompiled maps of the three games); the output is unchanged.":
        "Corregido (comprobado con AddressSanitizer y UBSan sobre los mapas de regresión y todos los mapas descompilados de los tres juegos); la salida no cambia.",
    "PR #316":
        "PR #316",
    "Thin brush parts under one cubic unit are discarded; a crossing face narrower than 0.2 units vanishes; faces almost on another face's plane are not drawn; brushes with a small cross-section are not split, so part of them has no collision.":
        "Las partes finas de brush de menos de una unidad cúbica se descartan; una cara de cruce más estrecha de 0,2 unidades desaparece; las caras casi sobre el plano de otra cara no se dibujan; los brushes de sección pequeña no se dividen, así que parte de ellos no tiene colisión.",
    "The five fixes of the pull request for this are taken over - each checked alone on the original maps of Jedi Academy and Jedi Outcast: no map got worse. Left out: dropping portals only when degenerate - with it 42 of 106 original maps leak (narrow gaps that NetRadiant-Custom closes).":
        "Se adoptan las cinco correcciones del pull request correspondiente, cada una comprobada por separado con los mapas originales de Jedi Academy y Jedi Outcast: ningún mapa empeoró. Se deja fuera: descartar portales solo cuando son degenerados; con ello 42 de 106 mapas originales tienen fugas (huecos estrechos que NetRadiant-Custom cierra).",
    "Crashes or undefined behaviour with broken map files (no entity at all, a quote as the last character, huge numbers) and when patches are subdivided for the lightmaps.":
        "Cuelgues o comportamiento indefinido con archivos de mapa rotos (ninguna entidad, unas comillas como último carácter, números enormes) y al subdividir patches para los lightmaps.",
    "Found by running bhed_q3map2 under AddressSanitizer and UBSan over 300 deliberately broken maps and the original maps; fixed. A map without entities now gets a clear error. The output of valid maps stays byte for byte the same.":
        "Encontrados ejecutando bhed_q3map2 con AddressSanitizer y UBSan sobre 300 mapas rotos a propósito y los mapas originales; corregidos. Un mapa sin entidades da ahora un error claro. La salida de los mapas válidos queda idéntica byte a byte.",
    "ASan, UBSan":
        "ASan, UBSan",
    "(editor) Numbers from about 1e10 are cut when a map is saved; 1e20 becomes 1.":
        "(editor) Los números a partir de aproximadamente 1e10 se cortan al guardar un mapa; 1e20 se convierte en 1.",
    "The value is kept (found with the new fuzzer for broken map files); ordinary coordinates are written exactly as before.":
        "El valor se conserva (encontrado con el nuevo fuzzer de archivos de mapa rotos); las coordenadas normales se escriben exactamente como antes.",
    "Decimal":
        "Decimal",
    "(engine) A misc_model_static whose cull point origin + (0 0 zoffset+1) lies in solid is never drawn.":
        "(motor) Un misc_model_static cuyo punto de descarte origin + (0 0 zoffset+1) está dentro de un sólido nunca se dibuja.",
    "The preview \"as in the game\" does not show it either, and the optimizer offers the zoffset that brings it back ([[karte_optimierer]]).":
        "La vista previa \"como en el juego\" tampoco lo muestra, y el optimizador ofrece el zoffset que lo recupera ([[karte_optimierer]]).",
    "OpenJK#1358":
        "OpenJK#1358",
    "(game) Light styles: after entity edits, switchable lights (with a targetname) turn the wrong lights on and off - -onlyents numbers the styles anew, the lightmaps keep the old ones.":
        "(juego) Estilos de luz: tras editar entidades, las luces conmutables (con targetname) encienden y apagan las luces equivocadas; -onlyents numera los estilos de nuevo y los lightmaps conservan los antiguos.",
    "**{{KeBmNurEntities}}** refuses when the switchable lights differ from the original .bsp and asks for a full build ([[karte_oeffnen]]).":
        "**{{KeBmNurEntities}}** se niega cuando las luces conmutables difieren del .bsp original y pide una compilación completa ([[karte_oeffnen]]).",
    "JKHub":
        "JKHub",
    "**Camera inside a brush** (Stage 7): back faces are culled, so a brush you stand in disappears - unless its shader says `cull none`, `cull twosided` or `cull disable` (fog volumes, glass, water); then its walls stay visible from inside, as in NetRadiant-Custom (renderstate.cpp OpenGLShader::construct, shaders.cpp `cull`).":
        "**Cámara dentro de un brush** (etapa 7): las caras traseras se descartan, así que un brush en el que estás desaparece, salvo que su shader diga `cull none`, `cull twosided` o `cull disable` (volúmenes de niebla, cristal, agua); entonces sus paredes siguen visibles desde dentro, como en NetRadiant-Custom (renderstate.cpp OpenGLShader::construct, shaders.cpp `cull`).",
    "**NRC#281**: Ctrl+K to several targets gives them the same targetname - as in NRC.":
        "**NRC#281**: Ctrl+K a varios destinos les da el mismo targetname, como en NRC.",
    "**NRC#132**: a region build leaks when the region has no entities and the camera is inside a brush - as in NRC; JK Studio warns before such a build.":
        "**NRC#132**: una compilación de región tiene fugas cuando la región no tiene entidades y la cámara está dentro de un brush, como en NRC; JK Studio avisa antes de una compilación así.",
    "**NRC#48**: external lightmaps together with light styles do not work in RBSP (Jedi Academy, Jedi Outcast) - a limit of the format; the build menu and the optimizer refuse the combination and explain the way around it.":
        "**NRC#48**: los lightmaps externos junto con estilos de luz no funcionan en RBSP (Jedi Academy, Jedi Outcast), un límite del formato; el menú de compilación y el optimizador rechazan la combinación y explican cómo evitarla.",
    "**GL#189, NRC#144**: surface lights under a floor shine through it because of the backsplash - as in NRC; the optimizer finds such lights ([[karte_optimierer]]).":
        "**GL#189, NRC#144**: las luces de superficie bajo un suelo lo atraviesan por el backsplash, como en NRC; el optimizador encuentra esas luces ([[karte_optimierer]]).",
    "**Menus in the style of JK Studio.** The menu bar and all menus of the map editor look like the menus of the rest of the program. Entries, order, submenus, checks, shortcuts, the mnemonic keys and what each entry does are those of NetRadiant-Custom. **Preferences > Interface > {{KeMenueNrc}}** brings back the look of NetRadiant-Custom.":
        "**Menús con el estilo de JK Studio.** La barra de menús y todos los menús del editor de mapas se ven como los menús del resto del programa. Las entradas, el orden, los submenús, las marcas, los atajos, las teclas mnemónicas y lo que hace cada entrada son los de NetRadiant-Custom. **Preferences > Interfaz > {{KeMenueNrc}}** recupera el aspecto de NetRadiant-Custom.",
    "**Program settings in View.** At the end of **{{KeLView}}**: {{ViewTarget}}, {{ViewTheme}}, {{ViewLanguage}}, {{ViewDpiScale}} and {{KeKnopfGroesse}} - with the same effect as in the program's View menu. Language, theme and scaling also stay at the end of **{{KeLMisc}}**.":
        "**Ajustes del programa en View.** Al final de **{{KeLView}}**: {{ViewTarget}}, {{ViewTheme}}, {{ViewLanguage}}, {{ViewDpiScale}} y {{KeKnopfGroesse}}, con el mismo efecto que en el menú Ver del programa. El idioma, el tema y la escala también siguen al final de **{{KeLMisc}}**.",
    "**Toolbar button size.** {{KeKnopfKlein}}, {{KeKnopfNormal}} or {{KeKnopfGross}} for the main, filter and plugin toolbars, under **Preferences > Interface** and in **{{KeLView}} > {{KeKnopfGroesse}}**. It applies at once and is saved; the views move up or down with the toolbars.":
        "**Tamaño de los botones de las barras.** {{KeKnopfKlein}}, {{KeKnopfNormal}} o {{KeKnopfGross}} para las barras principal, de filtros y de plugins, en **Preferences > Interfaz** y en **{{KeLView}} > {{KeKnopfGroesse}}**. Se aplica enseguida y se guarda; las vistas suben o bajan con las barras.",
    "**Ctrl + mouse wheel** does not scale the program inside the map editor: as in NetRadiant-Custom the wheel belongs to the views (zoom in 2D, move in the camera, scroll in the texture browser), also with Ctrl. The scaling is in **{{KeLView}} > {{ViewDpiScale}}**.":
        "**Ctrl + rueda del ratón** no escala el programa dentro del editor de mapas: como en NetRadiant-Custom, la rueda pertenece a las vistas (zoom en 2D, moverse en la cámara, desplazarse en el navegador de texturas), también con Ctrl. La escala está en **{{KeLView}} > {{ViewDpiScale}}**.",
    "**Window title.** In the map editor the title shows the map and a `*` when it has unsaved changes, as in NetRadiant-Custom.":
        "**Título de la ventana.** En el editor de mapas el título muestra el mapa y un `*` cuando tiene cambios sin guardar, como en NetRadiant-Custom.",
    "**Own build functions.** The window {{KeFsTitel}} with the remaining time, {{KeTkVis}} and {{KeTkLicht}}, {{KeZfMenue}} ([[karte_teilbau]]), {{KeEdMenue}}, {{KeRkModelle}} and {{KeLsOpt}} ([[karte_oeffnen#editordaten]]) - each with its own menu entry or option and the hint \"not in NetRadiant-Custom\". bhed_q3map2 has the switches `-bhedvisreuse`, `-bhedlightcache` and `-bhedmodels` for them; without them it builds as before.":
        "**Funciones de compilación propias.** La ventana {{KeFsTitel}} con el tiempo restante, {{KeTkVis}} y {{KeTkLicht}}, {{KeZfMenue}} ([[karte_teilbau]]), {{KeEdMenue}}, {{KeRkModelle}} y {{KeLsOpt}} ([[karte_oeffnen#editordaten]]), cada una con su propia entrada de menú u opción y la indicación \"no está en NetRadiant-Custom\". bhed_q3map2 tiene para ellas las opciones `-bhedvisreuse`, `-bhedlightcache` y `-bhedmodels`; sin ellas compila como antes.",
    "**Own editor tools (stage 8b).** {{KeSfSchalter}} for the split layout, {{KeSpMenue}} (F6), {{KeLgMenue}}, {{KeAmMenue}}, {{KeSgMenue}} and {{KeUlMenue}} ([[karte_werkzeuge8b]]) - each with its own menu entry or option and the hint \"not in NetRadiant-Custom\", all off by default. The Terrain Generator comes from the NetRadiant-Custom source after the release 20260114; its fixed seed is an own addition. {{KeRkModelle}} looks in all model folders with a cached model index. bhed_q3map2 `-light -bhedgpu` runs the shadow tests on the graphics card with byte-identical results, but stays off (no gain on the measured computer).":
        "**Herramientas propias del editor (etapa 8b).** {{KeSfSchalter}} para la disposición dividida, {{KeSpMenue}} (F6), {{KeLgMenue}}, {{KeAmMenue}}, {{KeSgMenue}} y {{KeUlMenue}} ([[karte_werkzeuge8b]]), cada una con su propia entrada de menú u opción y la indicación \"no está en NetRadiant-Custom\", todas desactivadas por defecto. El Terrain Generator viene del código fuente de NetRadiant-Custom posterior a la versión 20260114; su semilla fija es un añadido propio. {{KeRkModelle}} busca en todas las carpetas de modelos con un índice de modelos en caché. bhed_q3map2 `-light -bhedgpu` ejecuta las pruebas de sombra en la tarjeta gráfica con resultados idénticos byte a byte, pero sigue desactivado (sin ganancia en el ordenador medido).",
    "Map editor: differences to NetRadiant-Custom":
        "Editor de mapas: diferencias con NetRadiant-Custom",
    "The map editor follows NetRadiant-Custom 20260114 in look and handling. Known bugs of NetRadiant-Custom, Xonotic NetRadiant, GtkRadiant and q3map2 were checked one by one: what JK Studio does differently, and what stays as in NetRadiant-Custom on purpose.":
        "El editor de mapas sigue a NetRadiant-Custom 20260114 en aspecto y manejo. Los errores conocidos de NetRadiant-Custom, Xonotic NetRadiant, GtkRadiant y q3map2 se comprobaron uno a uno: qué hace JK Studio de otra manera, y qué se queda a propósito como en NetRadiant-Custom.",
    "Own handling (not in NetRadiant-Custom)":
        "Manejo propio (no está en NetRadiant-Custom)",
    "{{KeLView}}: the program settings at the end, menus in the style of JK Studio.":
        "{{KeLView}}: los ajustes del programa al final, menús con el estilo de JK Studio.",
    "Preferences > Interface: {{KeMenueNrc}} and {{KeKnopfGroesse}}.":
        "Preferences > Interfaz: {{KeMenueNrc}} y {{KeKnopfGroesse}}.",
    "{{KeKnopfGroesse}} {{KeKnopfKlein}}: the toolbars take less room.":
        "{{KeKnopfGroesse}} {{KeKnopfKlein}}: las barras ocupan menos sitio.",
    "Editor: fixed bugs":
        "Editor: errores corregidos",
    "Found during the check":
        "Encontrado durante la comprobación",
    "bhed_q3map2: fixed bugs and deliberate output differences":
        "bhed_q3map2: errores corregidos y diferencias deliberadas en la salida",
    "A .bsp built by bhed_q3map2 therefore differs from one built by NetRadiant-Custom's q3map2 in the order of the surfaces and the packing of the lightmaps - not in the geometry. NetRadiant-Custom's q3map2 itself never writes the same file twice.":
        "Por eso un .bsp compilado por bhed_q3map2 difiere de uno compilado por el q3map2 de NetRadiant-Custom en el orden de las superficies y el empaquetado de los lightmaps, no en la geometría. El propio q3map2 de NetRadiant-Custom nunca escribe dos veces el mismo archivo.",
    "Preview as in the game":
        "Vista previa como en el juego",
    "Kept as in NetRadiant-Custom":
        "Conservado como en NetRadiant-Custom",
    "BSP (all threads)":
        "BSP (todos los hilos)",
    "duel_pit":
        "duel_pit",
    "0.20 s":
        "0,20 s",
    "VIS (all threads)":
        "VIS (todos los hilos)",
    "0.15 s":
        "0,15 s",
    "0.10 s":
        "0,10 s",
    "LIGHT (all threads)":
        "LIGHT (todos los hilos)",
    "0.66 s":
        "0,66 s",
    "0.61 s":
        "0,61 s",
    "BSP (1 thread)":
        "BSP (1 hilo)",
    "VIS (1 thread)":
        "VIS (1 hilo)",
    "LIGHT (1 thread)":
        "LIGHT (1 hilo)",
    "3.02 s":
        "3,02 s",
    "3.33 s":
        "3,33 s",
    "3.08 s":
        "3,08 s",
    "mp__duel1":
        "mp__duel1",
    "0.36 s":
        "0,36 s",
    "0.31 s":
        "0,31 s",
    "0.11 s":
        "0,11 s",
    "2.53 s":
        "2,53 s",
    "2.99 s":
        "2,99 s",
    "1.92 s":
        "1,92 s",
    "0.35 s":
        "0,35 s",
    "0.16 s":
        "0,16 s",
    "8.72 s":
        "8,72 s",
    "9.68 s":
        "9,68 s",
    "9.63 s":
        "9,63 s",
    "t1_inter":
        "t1_inter",
    "0.26 s":
        "0,26 s",
    "0.30 s":
        "0,30 s",
    "0.25 s":
        "0,25 s",
    "md_ch_ben":
        "md_ch_ben",
    "0.81 s":
        "0,81 s",
    "0.71 s":
        "0,71 s",
    "0.76 s":
        "0,76 s",
    "2.77 s":
        "2,77 s",
    "2.27 s":
        "2,27 s",
    "2.23 s":
        "2,23 s",
    "3.73 s":
        "3,73 s",
    "4.24 s":
        "4,24 s",
    "2.58 s":
        "2,58 s",
    "0.86 s":
        "0,86 s",
    "30.53 s":
        "30,53 s",
    "24.90 s":
        "24,90 s",
    "24.60 s":
        "24,60 s",
    "12.66 s":
        "12,66 s",
    "12.65 s":
        "12,65 s",
    "12.76 s":
        "12,76 s",
    "kejim_post":
        "kejim_post",
    "4.74 s":
        "4,74 s",
    "4.59 s":
        "4,59 s",
    "4.14 s":
        "4,14 s",
    "745.40 s":
        "745,40 s",
    "621.47 s":
        "621,47 s",
    "573.70 s":
        "573,70 s",
    "9.18 s":
        "9,18 s",
    "10.28 s":
        "10,28 s",
    "7.21 s":
        "7,21 s",
    "duel_mos_espa":
        "duel_mos_espa",
    "53.35 s":
        "53,35 s",
    "44.86 s":
        "44,86 s",
    "19.57 s":
        "19,57 s",
    "Invert Selection (select all)":
        "Invert Selection (seleccionar todo)",
    "29.74 s":
        "29,74 s",
    "5.2 ms":
        "5,2 ms",
    "1.34 s":
        "1,34 s",
    "0.7 ms":
        "0,7 ms",
    "md_am_sith":
        "md_am_sith",
    "1.36 s":
        "1,36 s",
    "0.8 ms":
        "0,8 ms",
    "Open .bsp (decompile)":
        "Abrir .bsp (descompilar)",
    "7.71 s":
        "7,71 s",
    "7.98 s":
        "7,98 s",
    "2.01 s":
        "2,01 s",
    "2.64 s":
        "2,64 s",
    "3.29 s":
        "3,29 s",
    "3.42 s":
        "3,42 s",
    "Load .map":
        "Cargar .map",
    "1.31 s":
        "1,31 s",
    "1.29 s":
        "1,29 s",
    "662.5 ms":
        "662,5 ms",
    "717.4 ms":
        "717,4 ms",
    "805.6 ms":
        "805,6 ms",
    "827.7 ms":
        "827,7 ms",
    "Textures loaded":
        "Texturas cargadas",
    "1.64 s":
        "1,64 s",
    "1.80 s":
        "1,80 s",
    "4.2 ms":
        "4,2 ms",
    "821.6 ms":
        "821,6 ms",
    "1.93 s":
        "1,93 s",
    "2.16 s":
        "2,16 s",
    "Save .map":
        "Guardar .map",
    "815.8 ms":
        "815,8 ms",
    "745.4 ms":
        "745,4 ms",
    "142.5 ms":
        "142,5 ms",
    "173.7 ms":
        "173,7 ms",
    "175.9 ms":
        "175,9 ms",
    "189.6 ms":
        "189,6 ms",
    "Move everything":
        "Mover todo",
    "285.1 ms":
        "285,1 ms",
    "329.4 ms":
        "329,4 ms",
    "53.2 ms":
        "53,2 ms",
    "72.0 ms":
        "72,0 ms",
    "62.9 ms":
        "62,9 ms",
    "68.0 ms":
        "68,0 ms",
    "Undo the move":
        "Deshacer el movimiento",
    "294.1 ms":
        "294,1 ms",
    "353.0 ms":
        "353,0 ms",
    "26.2 ms":
        "26,2 ms",
    "36.4 ms":
        "36,4 ms",
    "33.3 ms":
        "33,3 ms",
    "32.6 ms":
        "32,6 ms",
    "Undo CSG Subtract":
        "Deshacer CSG Subtract",
    "171.1 ms":
        "171,1 ms",
    "207.4 ms":
        "207,4 ms",
    "8.6 ms":
        "8,6 ms",
    "7.9 ms":
        "7,9 ms",
    "12.1 ms":
        "12,1 ms",
    "10.1 ms":
        "10,1 ms",
    "Optimizer start":
        "Inicio del optimizador",
    "48.6 ms":
        "48,6 ms",
    "39.2 ms":
        "39,2 ms",
    "10.2 ms":
        "10,2 ms",
    "13.1 ms":
        "13,1 ms",
    "14.7 ms":
        "14,7 ms",
    "11.8 ms":
        "11,8 ms",
    "Optimizer check":
        "Comprobación del optimizador",
    "1.47 s":
        "1,47 s",
    "162.4 ms":
        "162,4 ms",
    "201.8 ms":
        "201,8 ms",
    "303.7 ms":
        "303,7 ms",
    "257.4 ms":
        "257,4 ms",
    "Switch to the map editor until the game data is ready":
        "Cambiar al editor de mapas hasta que los datos del juego estén listos",
    "273.3 ms":
        "273,3 ms",
    "258.1 ms":
        "258,1 ms",
    "Preview: prepare the scene (md_am_sith)":
        "Vista previa: preparar la escena (md_am_sith)",
    "1.44 s":
        "1,44 s",
    "1.40 s":
        "1,40 s",
    "Preview: frame time, scene (mean)":
        "Vista previa: tiempo por fotograma, escena (media)",
    "15.0 ms":
        "15,0 ms",
    "Preview: frame time, free flight (mean)":
        "Vista previa: tiempo por fotograma, vuelo libre (media)",
    "8.2 ms":
        "8,2 ms",
    "8.1 ms":
        "8,1 ms",
    "Step":
        "Paso",
    "before (Stage 6)":
        "antes (etapa 6)",
    "now":
        "ahora",
    "Map editor: speed and real-time light":
        "Editor de mapas: velocidad y luz en tiempo real",
    "See the light while you build: {{KeLichtMenue}} lights the camera on the graphics card, {{KeBackenMenue}} builds the map in the background and shows the real lightmaps. Building and editing got faster - with the same result as before.":
        "Mira la luz mientras construyes: {{KeLichtMenue}} ilumina la cámara en la tarjeta gráfica, {{KeBackenMenue}} compila el mapa en segundo plano y muestra los lightmaps reales. Compilar y editar se volvió más rápido, con el mismo resultado que antes.",
    "Real-time light":
        "Luz en tiempo real",
    "{{KeLBuild}}: {{KeLichtMenue}} and {{KeBackenMenue}} at the end of the menu (not in NetRadiant-Custom).":
        "{{KeLBuild}}: {{KeLichtMenue}} y {{KeBackenMenue}} al final del menú (no están en NetRadiant-Custom).",
    "Switch on **{{KeLBuild}} > {{KeLichtMenue}}**. The camera (render mode textured) is lit by the lights of the map - point lights, spotlights, the sun of the sky shader (`q3map_sun`) and surface lights (`q3map_surfacelight`) - with the formulas of q3map2 -light and with shadows.":
        "Activa **{{KeLBuild}} > {{KeLichtMenue}}**. La cámara (modo de render texturizado) se ilumina con las luces del mapa (luces puntuales, focos, el sol del shader del cielo (`q3map_sun`) y las luces de superficie (`q3map_surfacelight`)) con las fórmulas de q3map2 -light y con sombras.",
    "Move a light or change its keys: the camera follows at once. Up to 512 lights light each frame - every light whose reach touches the picture, the strongest first, also far away; the 128 most important cast shadows, the rest light without shadow.":
        "Mueve una luz o cambia sus claves: la cámara lo sigue enseguida. Hasta 512 luces iluminan cada fotograma (cada luz cuyo alcance toca la imagen, la más fuerte primero, también lejos); las 128 más importantes proyectan sombras, el resto ilumina sin sombra.",
    "Real-time light in a closed room with one light and a wall. Top right the camera says that this is a preview.":
        "Luz en tiempo real en una sala cerrada con una luz y una pared. Arriba a la derecha, la cámara dice que es una vista previa.",
    "The real-time light is a **preview** and is never saved as a result: no bounce light, no filter, no supersampling, shadows from shadow maps instead of rays. The final light always comes from bhed_q3map2 - {{KeBackenMenue}} or a build from the {{KeLBuild}} menu.":
        "La luz en tiempo real es una **vista previa** y nunca se guarda como resultado: sin luz de rebote, sin filtro, sin supermuestreo, sombras de mapas de sombras en vez de rayos. La luz final siempre sale de bhed_q3map2: {{KeBackenMenue}} o una compilación del menú {{KeLBuild}}.",
    "Real-time light like in the game":
        "Luz en tiempo real como en el juego",
    "The preview follows the rules of q3map2 -light -fast, so that the picture matches the baked lightmaps (a real JKA mapper's test map was the yardstick):":
        "La vista previa sigue las reglas de q3map2 -light -fast, para que la imagen coincida con los lightmaps horneados (el mapa de prueba de un mapper real de JKA fue la referencia):",
    "**Sky and sun:** the ray to the sun ends at the sky, as in q3map2 - the sky brush itself (caulk on its other sides, a lid above) no longer shadows the sun. `sun`, `q3map_sun`, `q3map_sunExt` and `q3map_skyLight` (many weak suns over the sky) all count; the sun has two shadow cascades (whole map, around the camera).":
        "**Cielo y sol:** el rayo hacia el sol termina en el cielo, como en q3map2; el propio brush del cielo (caulk en sus otras caras, una tapa encima) ya no hace sombra al sol. Cuentan `sun`, `q3map_sun`, `q3map_sunExt` y `q3map_skyLight` (muchos soles débiles por el cielo); el sol tiene dos cascadas de sombra (todo el mapa, alrededor de la cámara).",
    "**Surface light:** `q3map_surfacelight` per triangle, divided by `q3map_lightSubdivide`, with the exact form factor and the back splash (`q3map_backsplash`); the colour from `q3map_lightRGB` or `q3map_lightImage`.":
        "**Luz de superficie:** `q3map_surfacelight` por triángulo, dividido por `q3map_lightSubdivide`, con el factor de forma exacto y el backsplash (`q3map_backsplash`); el color de `q3map_lightRGB` o `q3map_lightImage`.",
    "**What casts shadows:** only what casts shadows in q3map2 - world brushes and func_group (other entities only with `_castShadows`), no nodraw surfaces, transparent shaders only with `alphashadow`, patches only with **{{KeLBuild}} > {{KePatchSchatten}}** (also passed to the quick bake). Back faces go into the shadow maps, so outside light no longer leaks inside.":
        "**Lo que proyecta sombras:** solo lo que proyecta sombras en q3map2: brushes del mundo y func_group (otras entidades solo con `_castShadows`), ninguna superficie nodraw, shaders transparentes solo con `alphashadow`, patches solo con **{{KeLBuild}} > {{KePatchSchatten}}** (también se pasa al horneado rápido). Las caras traseras entran en los mapas de sombras, así la luz de fuera ya no se cuela dentro.",
    "**Like in the game:** with real-time light (and with the quick bake shown) nodraw surfaces such as caulk disappear, and transparent shaders are drawn with the alphaFunc and blendFunc of their first stage.":
        "**Como en el juego:** con la luz en tiempo real (y con el horneado rápido mostrado) desaparecen las superficies nodraw como el caulk, y los shaders transparentes se dibujan con el alphaFunc y el blendFunc de su primera etapa.",
    "A room whose ceiling is a sky brush (caulk on all other sides, a caulk lid above): the sun of the sky shader shines through the sky into the room, as after the bake.":
        "Una sala cuyo techo es un brush de cielo (caulk en todas las demás caras, una tapa de caulk encima): el sol del shader del cielo brilla a través del cielo dentro de la sala, como tras el horneado.",
    "Measured against the quick bake at fixed camera points (md_ch_ben, mp/duel1, md_eitd_jedi with sky light, a large tester map): the mean brightness difference fell from 2-95 to 0.2-2 of 255. What remains is mostly the lightmap resolution (one luxel per 16 units) and bounce light, which the preview does not have.":
        "Medido frente al horneado rápido en puntos de cámara fijos (md_ch_ben, mp/duel1, md_eitd_jedi con luz del cielo, un mapa grande de un probador): la diferencia media de brillo bajó de 2-95 a 0,2-2 de 255. Lo que queda es sobre todo la resolución de los lightmaps (un luxel por cada 16 unidades) y la luz de rebote, que la vista previa no tiene.",
    "Quick bake":
        "Horneado rápido",
    "Switch on **{{KeLBuild}} > {{KeBackenMenue}}**. Two seconds after the last change bhed_q3map2 builds the map in a temporary folder - `-meta`, `-vis -fast` (when the map has no leak) and `-light -fast` - at low priority, so the editor stays smooth.":
        "Activa **{{KeLBuild}} > {{KeBackenMenue}}**. Dos segundos después del último cambio, bhed_q3map2 compila el mapa en una carpeta temporal (`-meta`, `-vis -fast` (si el mapa no tiene fugas) y `-light -fast`) con prioridad baja, así el editor sigue fluido.",
    "When it is done, the camera shows the real lightmaps of this build; top right it says **{{KeLichtGebacken}}**. Any change cancels a running build and starts a new one after the pause. Your map and a .bsp next to it are not touched.":
        "Cuando termina, la cámara muestra los lightmaps reales de esta compilación; arriba a la derecha dice **{{KeLichtGebacken}}**. Cualquier cambio cancela una compilación en curso y empieza una nueva tras la pausa. Tu mapa y un .bsp junto a él no se tocan.",
    "The same room after the quick bake: the lightmaps of bhed_q3map2.":
        "La misma sala tras el horneado rápido: los lightmaps de bhed_q3map2.",
    "The quick bake writes the same lightmaps, light grid and visibility as bhed_q3map2 on the command line with the same switches - the self-test compares the files. It is faster than a full build because of `-fast` and the missing bounce passes; for the release build use the {{KeLBuild}} menu.":
        "El horneado rápido escribe los mismos lightmaps, rejilla de luz y visibilidad que bhed_q3map2 en la línea de comandos con las mismas opciones; el autotest compara los archivos. Es más rápido que una compilación completa por `-fast` y la falta de pasadas de rebote; para la compilación final usa el menú {{KeLBuild}}.",
    "What got faster":
        "Qué se volvió más rápido",
    "Every change was measured before and after (median of three runs, low priority, the same maps) and gives the same result: the .bsp of -bsp and -light is byte for byte the one of a 1-thread build of Stage 6, the saved maps are identical.":
        "Cada cambio se midió antes y después (mediana de tres ejecuciones, prioridad baja, los mismos mapas) y da el mismo resultado: el .bsp de -bsp y -light es byte a byte el de una compilación de 1 hilo de la etapa 6, y los mapas guardados son idénticos.",
    "**-light** uses all cores for the lightmaps (light by light in blocks, the adaptive supersampling predicted and checked exactly) and for the light grid.":
        "**-light** usa todos los núcleos para los lightmaps (luz a luz en bloques, el supermuestreo adaptativo predicho y comprobado exactamente) y para la rejilla de luz.",
    "**-bsp**: the T-junction fix searches in parallel and applies in order; finding equal light grid points uses buckets instead of a search through all points.":
        "**-bsp**: la corrección de uniones en T busca en paralelo y aplica en orden; encontrar puntos iguales de la rejilla de luz usa cubetas en vez de buscar entre todos los puntos.",
    "**-vis** stays as in NetRadiant-Custom: with several threads it may differ slightly from build to build, with `-threads 1` it is always the same. Two ways to make it the same with several threads were tried and measured; both were slower.":
        "**-vis** se queda como en NetRadiant-Custom: con varios hilos puede diferir un poco de una compilación a otra, con `-threads 1` siempre es igual. Se probaron y midieron dos formas de hacerlo igual con varios hilos; las dos eran más lentas.",
    "**Editor**: invert selection, the selection test of the views and the NPC files of the optimizer (read in the background).":
        "**Editor**: invertir la selección, la prueba de selección de las vistas y los archivos de NPC del optimizador (leídos en segundo plano).",
    "kejim_post \"Textures loaded\" was so fast before only because Stage 6 loaded Jedi Outcast without its folders in this run (0 shaders, every face with the replacement image - the error fixed in Stage 7); now the 1187 shaders and their images are loaded. Steps that were not changed vary by about 20 % from run to run.":
        "\"Texturas cargadas\" de kejim_post era antes tan rápido solo porque la etapa 6 cargó Jedi Outcast sin sus carpetas en esa ejecución (0 shaders, cada cara con la imagen de sustitución; el error corregido en la etapa 7); ahora se cargan los 1187 shaders y sus imágenes. Los pasos que no se cambiaron varían alrededor de un 20 % entre ejecuciones.",
    "Times on the test laptop (32 threads), median of three runs; \"-\" = not measured. Your computer will give other numbers, the ratio stays similar.":
        "Tiempos en el portátil de pruebas (32 hilos), mediana de tres ejecuciones; \"-\" = no medido. Tu ordenador dará otros números, la proporción sigue siendo parecida.",
    "Map editor: build progress and partial compiles":
        "Editor de mapas: progreso de la compilación y compilaciones parciales",
    "See how far a build is and how long it still takes, build again only what changed - with the same result as a full build - and add a new part to a compiled map without compiling the map again.":
        "Mira cuánto le falta a una compilación y cuánto tardará, vuelve a compilar solo lo que cambió (con el mismo resultado que una compilación completa) y añade una parte nueva a un mapa compilado sin volver a compilar el mapa.",
    "Build progress":
        "Progreso de la compilación",
    "The window **{{KeFsTitel}}** in the second build of a map: step 3 (LIGHT), bounce 3 of 16, all steps at 43 % and the remaining time from the last build of this map.":
        "La ventana **{{KeFsTitel}}** en la segunda compilación de un mapa: paso 3 (LIGHT), rebote 3 de 16, todos los pasos al 43 % y el tiempo restante según la última compilación de este mapa.",
    "While a build from the {{KeLBuild}} menu runs, the window **{{KeFsTitel}}** shows the entry, the step and its stage (BSP, VIS, LIGHT), the phase q3map2 is in with its own progress - the dots and digits q3map2 prints, or the bounce - a bar over all steps and the time.":
        "Mientras se ejecuta una compilación del menú {{KeLBuild}}, la ventana **{{KeFsTitel}}** muestra la entrada, el paso y su etapa (BSP, VIS, LIGHT), la fase en la que está q3map2 con su propio progreso (los puntos y cifras que imprime q3map2, o el rebote), una barra de todos los pasos y el tiempo.",
    "**Remaining time**: in the first build of a map with an entry it is estimated from the running phase and shown as \"at least\". From the second build on it comes from the times of the last build of this map with this entry - the step times are kept in `karte_bauzeiten.json` in the settings folder.":
        "**Tiempo restante**: en la primera compilación de un mapa con una entrada se estima a partir de la fase en curso y se muestra como \"al menos\". A partir de la segunda compilación sale de los tiempos de la última compilación de este mapa con esta entrada; los tiempos de los pasos se guardan en `karte_bauzeiten.json` en la carpeta de ajustes.",
    "When the build is done, the console lists the time of each step and compares it with the last build: `1 BSP: 0.4 s (last build 0.5 s, -20 %)`.":
        "Cuando termina la compilación, la consola lista el tiempo de cada paso y lo compara con la última compilación: `1 BSP: 0.4 s (última compilación 0.5 s, -20 %)`.",
    "**{{KeBmAbbrechen}}** in the window stops q3map2, like {{KeLBuild}} > {{KeBmAbbrechen}}.":
        "**{{KeBmAbbrechen}}** en la ventana detiene q3map2, como {{KeLBuild}} > {{KeBmAbbrechen}}.",
    "**{{KeLBuild}} > {{KeFsMelden}}** (on by default): if JK Studio is not the front window when a build ends, its taskbar button flashes - no sound, no dialog.":
        "**{{KeLBuild}} > {{KeFsMelden}}** (activado por defecto): si JK Studio no es la ventana activa cuando termina una compilación, su botón de la barra de tareas parpadea: sin sonido ni diálogo.",
    "The progress comes from q3map2's own output; a phase with many lightmaps or a long bounce moves in steps. The remaining time is only as good as the last build was comparable - another entry, other switches or a busy computer change it.":
        "El progreso sale de la propia salida de q3map2; una fase con muchos lightmaps o un rebote largo avanza a saltos. El tiempo restante solo es tan bueno como comparable fue la última compilación: otra entrada, otras opciones o un ordenador ocupado lo cambian.",
    "Partial compiles":
        "Compilaciones parciales",
    "Two switches in the {{KeLBuild}} menu let every build reuse what has not changed since the last one. The result is **byte for byte the .bsp of a full build** - the ctest `q3map2_teilbau` builds maps both ways and compares the files.":
        "Dos interruptores del menú {{KeLBuild}} permiten que cada compilación reutilice lo que no ha cambiado desde la anterior. El resultado es **byte a byte el .bsp de una compilación completa**: el ctest `q3map2_teilbau` compila mapas de las dos formas y compara los archivos.",
    "**{{KeTkVis}}**: `-vis` runs with `-bhedvisreuse`. bhed_q3map2 keeps the visibility in `<map>.bhedvis` together with the portal file and the switches. If the portals (`.prt`), the switches and the far plane key (`_farplanedist`) are the same, it copies the visibility instead of calculating it - the console says **visibility reused**. Portals change only with structural brushes; entities, detail brushes, patches and textures do not change them.":
        "**{{KeTkVis}}**: `-vis` se ejecuta con `-bhedvisreuse`. bhed_q3map2 guarda la visibilidad en `<mapa>.bhedvis` junto con el archivo de portales y las opciones. Si los portales (`.prt`), las opciones y la clave del plano lejano (`_farplanedist`) son iguales, copia la visibilidad en vez de calcularla; la consola dice **visibilidad reutilizada**. Los portales solo cambian con brushes estructurales; las entidades, los brushes de detalle, los patches y las texturas no los cambian.",
    "**{{KeTkLicht}}**: `-light` runs with `-bhedlightcache`. bhed_q3map2 keeps every lightmap and the light grid in `<map>.bhedlight` with a fingerprint of everything that goes into it - the geometry, the lights that reach it, the switches. The next run calculates only the lightmaps whose fingerprint changed: move one light, and only the lightmaps it reaches are lit again. The console says how many came from the cache: `light cache: 9 of 14 lightmaps …`.":
        "**{{KeTkLicht}}**: `-light` se ejecuta con `-bhedlightcache`. bhed_q3map2 guarda cada lightmap y la rejilla de luz en `<mapa>.bhedlight` con una huella de todo lo que entra en ellos: la geometría, las luces que los alcanzan, las opciones. La ejecución siguiente calcula solo los lightmaps cuya huella cambió: mueve una luz y solo se vuelven a iluminar los lightmaps que alcanza. La consola dice cuántos salieron de la caché: `light cache: 9 of 14 lightmaps …`.",
    "**Only the post-processing changed** - `-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` or the worldspawn keys `_minlight`, `_minvertexlight`, `_mingridlight`: every lightmap and the grid come from the cache, only the post-processing runs again, without tracing a single ray.":
        "**Solo cambió el posprocesado** (`-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` o las claves de worldspawn `_minlight`, `_minvertexlight`, `_mingridlight`): cada lightmap y la rejilla salen de la caché, solo se vuelve a ejecutar el posprocesado, sin trazar un solo rayo.",
    "With `-bounce` the light cache is not used: bounce light couples every lightmap with every other one. With several threads `-vis` itself can differ slightly from run to run (see [[karte_tempo]]); the reused visibility is the one of the last run - for a comparison with a full build use `-vis -threads 1`. The cache files lie next to the map; delete them to force a full build. They do not belong into the .pk3.":
        "Con `-bounce` no se usa la caché de luz: la luz de rebote acopla cada lightmap con todos los demás. Con varios hilos, el propio `-vis` puede diferir un poco entre ejecuciones (mira [[karte_tempo]]); la visibilidad reutilizada es la de la última ejecución; para compararla con una compilación completa usa `-vis -threads 1`. Los archivos de caché están junto al mapa; bórralos para forzar una compilación completa. No van en el .pk3.",
    "Adding to a compiled map":
        "Añadir a un mapa compilado",
    "The dialog **{{KeZfTitel}}**: the compiled test map is the template (drawn in the camera, not editable), the new pillar in the layer \"Neu\" is the part to add.":
        "El diálogo **{{KeZfTitel}}**: el mapa de prueba compilado es la plantilla (dibujada en la cámara, no editable), la columna nueva de la capa \"Neu\" es la parte que se añade.",
    "Open the source map you build the addition in - or a new map - and choose **{{KeLBuild}} > {{KeZfMenue}}**. As the template take a map of the game (**{{KeZfAusSpiel}}**) or any .bsp (**{{KeBspDatei}}**). It is drawn in the camera and the 2D views and cannot be selected or changed.":
        "Abre el mapa fuente en el que construyes el añadido (o un mapa nuevo) y elige **{{KeLBuild}} > {{KeZfMenue}}**. Como plantilla toma un mapa del juego (**{{KeZfAusSpiel}}**) o cualquier .bsp (**{{KeBspDatei}}**). Se dibuja en la cámara y en las vistas 2D y no se puede seleccionar ni cambiar.",
    "Build the addition - brushes, patches, entities - in a layer of its own and pick it under **{{KeZfEbene}}** (the current layer is preselected).":
        "Construye el añadido (brushes, patches, entidades) en una capa propia y elígela en **{{KeZfEbene}}** (la capa actual viene preseleccionada).",
    "**{{KeZfPruefen}}** builds the layer and runs the checks below without adding anything. **{{KeZfBauen}}** builds the layer (`-meta`, `-light -fast -samples 2`) and inserts it with `q3map2 -mergebsp -world -fixnames`: as detail into the world of the template, whose BSP tree, visibility and lightmaps stay. The result is `<template>_merged.bsp` in the folder `karte_zusammen` in the settings folder; the template itself is not changed.":
        "**{{KeZfPruefen}}** compila la capa y ejecuta las comprobaciones de abajo sin añadir nada. **{{KeZfBauen}}** compila la capa (`-meta`, `-light -fast -samples 2`) y la inserta con `q3map2 -mergebsp -world -fixnames`: como detalle en el mundo de la plantilla, cuyo árbol BSP, visibilidad y lightmaps se mantienen. El resultado es `<plantilla>_merged.bsp` en la carpeta `karte_zusammen` de la carpeta de ajustes; la plantilla en sí no cambia.",
    "After {{KeZfBauen}}: the light of the addition from the template, the collision check and the merged .bsp.":
        "Tras {{KeZfBauen}}: la luz del añadido a partir de la plantilla, la comprobación de colisión y el .bsp combinado.",
    "What is checked":
        "Qué se comprueba",
    "**Weather brushes**: `system/inside` and `system/outside` must not be mixed - after merging, a map with both does not load. If it would, nothing is added.":
        "**Brushes de clima**: `system/inside` y `system/outside` no deben mezclarse; tras combinar, un mapa con ambos no carga. Si pasara, no se añade nada.",
    "**Weather zones**: if the template or the addition has `misc_weather_zone` entities, inside and outside brushes only count inside a zone; weather brushes of the addition outside every zone get a warning, because they have no effect.":
        "**Zonas de clima**: si la plantilla o el añadido tienen entidades `misc_weather_zone`, los brushes de interior y exterior solo cuentan dentro de una zona; los brushes de clima del añadido fuera de toda zona reciben un aviso, porque no tienen efecto.",
    "**Names**: a `target` or `targetname` of the addition that the template uses too is renamed by `-fixnames` - links between template and addition are lost. The dialog lists these names.":
        "**Nombres**: un `target` o `targetname` del añadido que también usa la plantilla se renombra con `-fixnames`; se pierden los enlaces entre la plantilla y el añadido. El diálogo lista esos nombres.",
    "**Limits**: the merged .bsp is counted like in the [[karte_optimierer|optimizer]]; a limit over the maximum is an error.":
        "**Límites**: el .bsp combinado se cuenta como en el [[karte_optimierer|optimizador]]; un límite por encima del máximo es un error.",
    "**Collision**: the merged .bsp is loaded into the game's collision model (OpenJK `CM_BoxTrace`, ported); every new solid brush must stop the player box when it walks into it from outside.":
        "**Colisión**: el .bsp combinado se carga en el modelo de colisión del juego (OpenJK `CM_BoxTrace`, portado); cada brush sólido nuevo debe detener la caja del jugador cuando entra en él desde fuera.",
    "The light of the addition is **approximated**: q3map2 cannot light the addition against the geometry of the template. {{KeZfLichtGitter}} takes the average of the template's light grid around the addition as ambient light, {{KeZfLichtLichter}} the lights without a targetname that stayed in the template - only a map built with {{KeEdMenue}} (`-keeplights`) has them. The light grid of the template is not updated - models and players near the addition are lit as before. For the final map build everything in full.":
        "La luz del añadido se **aproxima**: q3map2 no puede iluminar el añadido frente a la geometría de la plantilla. {{KeZfLichtGitter}} toma como luz ambiental la media de la rejilla de luz de la plantilla alrededor del añadido, {{KeZfLichtLichter}} las luces sin targetname que se quedaron en la plantilla; solo un mapa compilado con {{KeEdMenue}} (`-keeplights`) las tiene. La rejilla de luz de la plantilla no se actualiza: los modelos y jugadores cerca del añadido se iluminan como antes. Para el mapa final, compílalo todo completo.",
    "Weather rules from the JKHub thread on weather brushes: never mix inside and outside brushes in one map; with weather zones the brushes only work inside a zone; the game makes the `.weather` cache file again when the map loads, so an old one need not be deleted.":
        "Reglas de clima del hilo de JKHub sobre los brushes de clima: nunca mezcles brushes de interior y exterior en un mapa; con zonas de clima los brushes solo funcionan dentro de una zona; el juego vuelve a crear el archivo de caché `.weather` al cargar el mapa, así que no hace falta borrar uno antiguo.",
    "Map editor: terrain, playing, logic, mirror and more":
        "Editor de mapas: terreno, jugar, lógica, espejo y más",
    "Generate terrain, walk through the map without compiling, see the links between entities, check lightmap density and collision, edit symmetric maps with the mirror mode and jump through the undo history.":
        "Genera terreno, recorre el mapa sin compilar, mira los enlaces entre entidades, comprueba la densidad de lightmap y la colisión, edita mapas simétricos con el modo espejo y salta por el historial de deshacer.",
    "Everything on this page except the Terrain Generator is **not in NetRadiant-Custom**: each function has its own menu entry with that hint, and with all of them off the editor behaves like NetRadiant-Custom. The new entries are at the end of **{{KeLView}}**, above the program settings.":
        "Todo lo de esta página salvo el Terrain Generator **no está en NetRadiant-Custom**: cada función tiene su propia entrada de menú con esa indicación, y con todas desactivadas el editor se comporta como NetRadiant-Custom. Las entradas nuevas están al final de **{{KeLView}}**, encima de los ajustes del programa.",
    "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} and {{KeUlMenue}}.":
        "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} y {{KeUlMenue}}.",
    "Terrain Generator":
        "Terrain Generator",
    "**Plugins > TerrainGenerator > Generate Terrain** is the plugin from the current NetRadiant-Custom source (master, after the release 20260114), ported line by line: the same heights, triangles and textures as the original for the same values. The dialog stays open while you work in the views.":
        "**Plugins > TerrainGenerator > Generate Terrain** es el plugin del código fuente actual de NetRadiant-Custom (master, posterior a la versión 20260114), portado línea a línea: las mismas alturas, triángulos y texturas que el original con los mismos valores. El diálogo se queda abierto mientras trabajas en las vistas.",
    "The dialog with **{{KeTgUseSel}}**: the size comes from the selected brush.":
        "El diálogo con **{{KeTgUseSel}}**: el tamaño viene del brush seleccionado.",
    "A **{{KeTgShHill}}** made from the selected brush - one func_group of detail brushes.":
        "Una **{{KeTgShHill}}** hecha a partir del brush seleccionado: un func_group de brushes de detalle.",
    "**{{KeTgTarget}}** {{KeTgUseSel}} takes the size of the selected brush and replaces it; {{KeTgManual}} builds the terrain around the origin.":
        "**{{KeTgTarget}}** {{KeTgUseSel}} toma el tamaño del brush seleccionado y lo sustituye; {{KeTgManual}} construye el terreno alrededor del origen.",
    "**{{KeTgBaseShape}}** (hill, crater, ridge, slope, volcano, valley, tunnel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, terraces and the grid step as in the original; **{{KeTgGenerate}}** is one undo step.":
        "**{{KeTgBaseShape}}** (colina, cráter, cresta, pendiente, volcán, valle, túnel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, terrazas y el paso de rejilla como en el original; **{{KeTgGenerate}}** es un paso de deshacer.",
    "Own addition: **{{KeTgSeed}} {{KeTgSeedFix}}** starts the random numbers with a fixed value - the same values then give the same terrain again. Off (default), the random sequence runs on as in NetRadiant-Custom.":
        "Añadido propio: **{{KeTgSeed}} {{KeTgSeedFix}}** inicia los números aleatorios con un valor fijo; los mismos valores vuelven a dar entonces el mismo terreno. Desactivado (por defecto), la secuencia aleatoria sigue como en NetRadiant-Custom.",
    "Free split windows":
        "Ventanas divididas libres",
    "**Preferences > Interface > Layout > {{KeSfSchalter}}** (layout Split): every one of the four windows gets a small menu in its corner - camera, XY, XZ or YZ, also two windows with the same view. Ctrl+Tab changes the type of the active 2D window, the splitter positions are kept. Off (default), the four windows are fixed as in NetRadiant-Custom.":
        "**Preferences > Interfaz > Disposición > {{KeSfSchalter}}** (disposición Split): cada una de las cuatro ventanas recibe un menú pequeño en su esquina (cámara, XY, XZ o YZ, también dos ventanas con la misma vista). Ctrl+Tab cambia el tipo de la ventana 2D activa y las posiciones de los divisores se conservan. Desactivado (por defecto), las cuatro ventanas son fijas como en NetRadiant-Custom.",
    "Play in the editor":
        "Jugar en el editor",
    "**{{KeSpMenue}}** (F6): walking through the open map - position and speed at the top, the keys at the bottom.":
        "**{{KeSpMenue}}** (F6): recorriendo el mapa abierto; posición y velocidad arriba, las teclas abajo.",
    "No compile: the collision comes straight from the brushes and patches with the contents q3map2 would give them (surfaceparms of the game, clip brushes, nonsolid, water ...). Walking, jumping, stairs and slopes use the port of the game's movement code (Pmove) - the same as the preview of the program: JKA and Movie Duels, or the Jedi Outcast rules.":
        "Sin compilar: la colisión sale directamente de los brushes y patches con el contenido que les daría q3map2 (surfaceparms del juego, brushes de clip, nonsolid, agua ...). Caminar, saltar, las escaleras y las pendientes usan el port del código de movimiento del juego (Pmove), el mismo que la vista previa del programa: JKA y Movie Duels, o las reglas de Jedi Outcast.",
    "W/A/S/D move, Space jumps, C or Ctrl crouches, Shift walks, the mouse looks, N switches noclip, **Esc ends** and puts the camera back where it was.":
        "W/A/S/D mueven, Espacio salta, C o Ctrl agacha, Mayús camina, el ratón mira, N activa noclip, **Esc termina** y devuelve la cámara adonde estaba.",
    "**{{KeSpSpawn}}**: on, the start is the first info_player_start (its origin + 9 and its angle, as the game picks it); off, the start is the camera.":
        "**{{KeSpSpawn}}**: activado, el inicio es el primer info_player_start (su origin + 9 y su ángulo, como lo elige el juego); desactivado, el inicio es la cámara.",
    "Only the player moves: no doors, movers, triggers, NPCs or weapons. The map is not changed - playing is not an undo step.":
        "Solo se mueve el jugador: ni puertas, ni móviles, ni triggers, ni NPC, ni armas. El mapa no cambia: jugar no es un paso de deshacer.",
    "Logic view":
        "Vista lógica",
    "**{{KeLgTitel}}**: trigger_multiple fires target_relay, which opens the door and spawns the NPC; the NPC's scripts below it.":
        "**{{KeLgTitel}}**: trigger_multiple dispara target_relay, que abre la puerta y hace aparecer al NPC; los scripts del NPC debajo.",
    "**{{KeLgMenue}}** draws every link between the entities as a graph from left to right: target, target2 ... target5, NPC_target, the targets of doors and every *target* key of the entity definitions; the script keys (spawnscript, usescript, deathscript ...) are listed in the box of the entity.":
        "**{{KeLgMenue}}** dibuja cada enlace entre las entidades como un grafo de izquierda a derecha: target, target2 ... target5, NPC_target, los destinos de las puertas y cada clave *target* de las definiciones de entidades; las claves de script (spawnscript, usescript, deathscript ...) se listan en la caja de la entidad.",
    "A click on a box selects the entity and centers the 2D views and the camera on it; a double-click on a script line opens the ICARUS script in the script editor. Mouse wheel zooms, right or middle mouse button moves.":
        "Un clic en una caja selecciona la entidad y centra en ella las vistas 2D y la cámara; un doble clic en una línea de script abre el script ICARUS en el editor de scripts. La rueda del ratón amplía, el botón derecho o central mueve.",
    "A target without a matching targetname appears as a red box - a link that leads nowhere in the game.":
        "Un target sin targetname coincidente aparece como caja roja: un enlace que en el juego no lleva a ninguna parte.",
    "Camera view modes":
        "Modos de vista de la cámara",
    "**{{KeAmDichte}}**: one square is one lightmap luxel.":
        "**{{KeAmDichte}}**: un cuadro es un luxel de lightmap.",
    "**{{KeAmKollision}}**: the trigger brush in green - it does not block the player.":
        "**{{KeAmKollision}}**: el brush del trigger en verde; no bloquea al jugador.",
    "**{{KeAmDichte}}**: a checkerboard of the luxel size per face, computed like q3map2 (q3map_lightmapSampleSize, _samplesize and _lightmapscale of the entity, -samplesize; enlarged until the face fits into 128 luxels). Finer than 16 reddish, coarser bluish, faces without lightmap grey.":
        "**{{KeAmDichte}}**: un tablero de ajedrez del tamaño del luxel por cara, calculado como q3map2 (q3map_lightmapSampleSize, _samplesize y _lightmapscale de la entidad, -samplesize; ampliado hasta que la cara cabe en 128 luxels). Más fino que 16 rojizo, más basto azulado, las caras sin lightmap en gris.",
    "**{{KeAmKollision}}**: brushes by the contents q3map2 gives them - playerclip magenta, monsterclip orange, botclip yellow, shotclip cyan, nodrop dark red, water, slime, lava, fog, triggers. Visible walls the player walks through (no player collision) glow bright red.":
        "**{{KeAmKollision}}**: los brushes según el contenido que les da q3map2: playerclip magenta, monsterclip naranja, botclip amarillo, shotclip cian, nodrop rojo oscuro, agua, slime, lava, niebla, triggers. Las paredes visibles que el jugador atraviesa (sin colisión para el jugador) brillan en rojo vivo.",
    "**{{KeAmSicht}}** and **{{KeAmLast}}** need the compiled .bsp next to the map (with VIS): what the engine draws from the camera's cluster in green, or a heat map of the triangles drawn from each place. The numbers are shown at the top left of the camera.":
        "**{{KeAmSicht}}** y **{{KeAmLast}}** necesitan el .bsp compilado junto al mapa (con VIS): lo que el motor dibuja desde el cluster de la cámara en verde, o un mapa de calor de los triángulos dibujados desde cada sitio. Los números se muestran arriba a la izquierda de la cámara.",
    "Mirror mode":
        "Modo espejo",
    "**{{KeSgMenue}} > {{KeSgX}}**: the pillar was moved 64 units to the right - its counterpart on the other side of the magenta plane followed.":
        "**{{KeSgMenue}} > {{KeSgX}}**: la columna se movió 64 unidades a la derecha y su homóloga al otro lado del plano magenta la siguió.",
    "**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} or {{KeSgZ}}; the plane goes through the origin or, with **{{KeSgMitte}}**, through the center of the selection on the grid. It is drawn as a magenta line in the 2D views.":
        "**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} o {{KeSgZ}}; el plano pasa por el origen o, con **{{KeSgMitte}}**, por el centro de la selección en la rejilla. Se dibuja como una línea magenta en las vistas 2D.",
    "Every change to the selection is written to its counterpart in the same undo step - live while dragging. Brushes and patches count one by one, point entities and groups as a whole; origin, angle and angles are mirrored, textures like **Flip** (with Texture Lock as set).":
        "Cada cambio en la selección se escribe en su homólogo en el mismo paso de deshacer, en directo al arrastrar. Los brushes y patches cuentan uno a uno, las entidades de punto y los grupos como un todo; origin, angle y angles se reflejan, las texturas como con **Flip** (con Texture Lock según esté).",
    "If the exact mirror image already exists, it becomes the counterpart; otherwise the first change creates a mirrored copy. Deleting removes both; undo brings back both exactly. Objects that lie symmetric on the plane get no counterpart.":
        "Si la imagen especular exacta ya existe, pasa a ser el homólogo; si no, el primer cambio crea una copia reflejada. Eliminar quita los dos; deshacer devuelve los dos exactamente. Los objetos simétricos sobre el plano no reciben homólogo.",
    "Copied entities keep their keys, also targetname and target - rename them where the two sides should act separately.":
        "Las entidades copiadas conservan sus claves, también targetname y target; renómbralas donde los dos lados deban actuar por separado.",
    "Undo history":
        "Historial de deshacer",
    "**{{KeUlTitel}}**: the start, three steps with the command names NetRadiant-Custom records, the current state highlighted.":
        "**{{KeUlTitel}}**: el inicio, tres pasos con los nombres de comando que registra NetRadiant-Custom, el estado actual resaltado.",
    "**{{KeUlMenue}}** lists all undo steps (oldest first) and below them the redo steps in grey. A click jumps to the state after that step with as many Undo or Redo as needed - each with its console line, as with Ctrl+Z and Ctrl+Y. The list goes back as far as the undo queue (**Preferences > Undo Queue Size**, default 512).":
        "**{{KeUlMenue}}** lista todos los pasos de deshacer (el más antiguo primero) y debajo los pasos de rehacer en gris. Un clic salta al estado tras ese paso con tantos Undo o Redo como haga falta, cada uno con su línea en la consola, como con Ctrl+Z y Ctrl+Y. La lista llega tan atrás como la cola de deshacer (**Preferences > Undo Queue Size**, por defecto 512).",
    "misc_model from all model folders":
        "misc_model de todas las carpetas de modelos",
    "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) now looks for models in all model folders of the game, not only in the folders of the map's shaders. A model index (per model and mesh: vertex count, triangle set and shader) lies in the settings folder as `karte_modellindex_<…>.txt`. The first time - and for new or changed .pk3 files - bhed_q3map2 loads the missing models once (about half a minute for a big install); after that only the models whose meshes match a triangle surface of the .bsp, or whose shader the .bsp uses, are loaded. The console names the index and the candidates.":
        "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) busca ahora modelos en todas las carpetas de modelos del juego, no solo en las carpetas de los shaders del mapa. Un índice de modelos (por modelo y malla: número de vértices, conjunto de triángulos y shader) está en la carpeta de ajustes como `karte_modellindex_<…>.txt`. La primera vez (y para archivos .pk3 nuevos o cambiados) bhed_q3map2 carga una vez los modelos que faltan (cerca de medio minuto para una instalación grande); después solo se cargan los modelos cuyas mallas coinciden con una superficie de triángulos del .bsp, o cuyo shader usa el .bsp. La consola nombra el índice y los candidatos.",
    "before":
        "antes",
    "jeditemple_garden (source known)":
        "jeditemple_garden (fuente conocida)",
    "GPU shadow tests (switched off)":
        "Pruebas de sombra en GPU (desactivadas)",
    "bhed_q3map2 has the command-line switch `-bhedgpu` for `-light`: the test \"does the ray from the luxel to the light hit something?\" runs on the graphics card (Direct3D 11), the light itself stays on the CPU. The result is **byte for byte the same** as without it - checked on five maps and by the ctest `q3map2_gpu`. On the measured computer (32 threads, RTX 4070 Laptop) it was not faster (mp/duel1 2.3 s → 3.2 s, kejim_post 10.0 s → 11.3 s), so it is not in the Build menu and stays off.":
        "bhed_q3map2 tiene la opción de línea de comandos `-bhedgpu` para `-light`: la prueba \"¿el rayo del luxel a la luz choca con algo?\" se ejecuta en la tarjeta gráfica (Direct3D 11); la luz en sí se queda en la CPU. El resultado es **idéntico byte a byte** al de sin ella, comprobado en cinco mapas y con el ctest `q3map2_gpu`. En el ordenador medido (32 hilos, RTX 4070 Laptop) no fue más rápido (mp/duel1 2,3 s → 3,2 s, kejim_post 10,0 s → 11,3 s), así que no está en el menú Build y sigue desactivada.",
    "Your first map: a room to walk in":
        "Tu primer mapa: una sala por la que caminar",
    "From an empty map to a room you can walk through in the game: brushes, textures, caulk, a light, the player start, the optimizer, building with the progress window, walking through it in the editor - and the .pk3 for the game.":
        "De un mapa vacío a una sala que puedes recorrer en el juego: brushes, texturas, caulk, una luz, el inicio del jugador, el optimizador, compilar con la ventana de progreso, recorrerla en el editor, y el .pk3 para el juego.",
    "Map editor · about 15 minutes":
        "Editor de mapas · unos 15 minutos",
    "Every picture on this page was taken while the program did exactly these steps, and the self-test checks the result every time: the room builds without a leak, the player stands at the start and can walk, and the .pk3 holds the map. The menus and shortcuts are those of NetRadiant-Custom, so other mapping tutorials fit as well - the page [[karteneditor]] explains the windows.":
        "Cada imagen de esta página se tomó mientras el programa hacía exactamente estos pasos, y el autotest comprueba el resultado cada vez: la sala compila sin fugas, el jugador está en el inicio y puede caminar, y el .pk3 contiene el mapa. Los menús y atajos son los de NetRadiant-Custom, así que otros tutoriales de mapeado también encajan; la página [[karteneditor]] explica las ventanas.",
    "**Set up the game.** Switch the mode bar to **{{ViewMapEditor}}**. The editor uses the game the program is set up for; its folders come from the program settings. **{{KeLEdit}} > {{KeLPreferences}}**, page **{{KePgGame}}**, shows the game, page **{{KePgPaths}}** the engine path (empty: taken from the program's game folders).":
        "**Configura el juego.** Cambia la barra de modos a **{{ViewMapEditor}}**. El editor usa el juego para el que está configurado el programa; sus carpetas vienen de los ajustes del programa. **{{KeLEdit}} > {{KeLPreferences}}**, página **{{KePgGame}}**, muestra el juego; la página **{{KePgPaths}}**, la ruta del motor (vacía: se toma de las carpetas del juego del programa).",
    "{{KeLPreferences}}, page {{KePgPaths}}.":
        "{{KeLPreferences}}, página {{KePgPaths}}.",
    "**New map.** **{{KeLFile}} > {{KeLNewMap}}** starts an empty map - only the worldspawn. Zoom the 2D view out a little with the mouse wheel.":
        "**Mapa nuevo.** **{{KeLFile}} > {{KeLNewMap}}** empieza un mapa vacío, solo con el worldspawn. Aleja un poco la vista 2D con la rueda del ratón.",
    "The {{KeLFile}} menu.":
        "El menú {{KeLFile}}.",
    "**Draw a brush.** In the 2D top view, drag with the left mouse button from one corner to the other - here from -256 -256 to 256 256, a box 512 units wide. Its height comes from the work zone (at first -64 to 64).":
        "**Dibuja un brush.** En la vista 2D superior, arrastra con el botón izquierdo de una esquina a la otra; aquí de -256 -256 a 256 256, una caja de 512 unidades de ancho. Su altura viene de la zona de trabajo (al principio de -64 a 64).",
    "The new brush, selected (red).":
        "El brush nuevo, seleccionado (en rojo).",
    "**Make it taller.** <kbd>Num 1</kbd> switches the 2D view to the front view (XZ). Grab just above the top edge and drag up: the top face follows on the grid. Here to 192 - the room is now 256 high. <kbd>Num 7</kbd> goes back to the top view.":
        "**Hazlo más alto.** <kbd>Num 1</kbd> cambia la vista 2D a la vista frontal (XZ). Agarra justo encima del borde superior y arrastra hacia arriba: la cara superior lo sigue en la rejilla. Aquí hasta 192: la sala mide ahora 256 de alto. <kbd>Num 7</kbd> vuelve a la vista superior.",
    "The front view: the brush is now 256 high.":
        "La vista frontal: el brush mide ahora 256 de alto.",
    "**Texture.** Double-click a folder in the tree of the texture browser and click a texture with the left mouse button: every face of the selected brush gets it. Pick something without transparency for the walls.":
        "**Textura.** Haz doble clic en una carpeta del árbol del navegador de texturas y pulsa una textura con el botón izquierdo: cada cara del brush seleccionado la recibe. Elige algo sin transparencia para las paredes.",
    "The wall texture on all six faces.":
        "La textura de pared en las seis caras.",
    "**Hollow it out.** The toolbar button **{{KeCsgTitle}}** opens the CSG tool. Leave the thickness at 16 and switch the caulk button off for now (AutoCaulk does that later), then press **{{KeCsgTipPull}}**: six walls 16 thick grow outwards around the room, and the inside stays exactly the box you drew. The toolbar button **Room** (Make Room) does the same with the grid size as thickness.":
        "**Vacíalo.** El botón **{{KeCsgTitle}}** de la barra abre la herramienta CSG. Deja el grosor en 16 y desactiva por ahora el botón de caulk (AutoCaulk lo hará después), y pulsa **{{KeCsgTipPull}}**: seis paredes de 16 de grosor crecen hacia fuera alrededor de la sala, y el interior queda exactamente como la caja que dibujaste. El botón **Room** (Make Room) de la barra hace lo mismo con el tamaño de rejilla como grosor.",
    "{{KeCsgTitle}} with the caulk button off.":
        "{{KeCsgTitle}} con el botón de caulk desactivado.",
    "**A floor texture.** In the front view, <kbd>Shift</kbd> + left click on the floor selects only the floor brush; click a floor texture in the browser. <kbd>Esc</kbd> clears the selection.":
        "**Una textura de suelo.** En la vista frontal, <kbd>Mayús</kbd> + clic izquierdo en el suelo selecciona solo el brush del suelo; pulsa una textura de suelo en el navegador. <kbd>Esc</kbd> quita la selección.",
    "The hollow room: six wall brushes.":
        "La sala hueca: seis brushes de pared.",
    "**A light.** Right-click inside the room in the 2D view: the entity menu opens. **light** asks for the intensity (300 is a good start) - **OK** places it where you clicked.":
        "**Una luz.** Haz clic derecho dentro de la sala en la vista 2D: se abre el menú de entidades. **light** pide la intensidad (300 es un buen comienzo); **OK** la coloca donde pulsaste.",
    "**Where the player starts.** Right-click near the floor: **info > info_player_start**. Without it the game does not know where to put the player. Its box stands on the floor.":
        "**Dónde empieza el jugador.** Haz clic derecho cerca del suelo: **info > info_player_start**. Sin él, el juego no sabe dónde poner al jugador. Su caja está sobre el suelo.",
    "The entity menu, submenu info.":
        "El menú de entidades, submenú info.",
    "**Save.** **{{KeLFile}} > {{KeLSaveAs}}** - for example `meinekarte.map` in a folder `maps` of your own, outside the game folders. The name of the file becomes the name of the map in the game.":
        "**Guarda.** **{{KeLFile}} > {{KeLSaveAs}}**, por ejemplo `mimapa.map` en una carpeta `maps` propia, fuera de las carpetas del juego. El nombre del archivo pasa a ser el nombre del mapa en el juego.",
    "The saved room in the editor.":
        "La sala guardada en el editor.",
    "**Caulk the outside.** Faces nobody can see still cost lightmap space and drawing time. Fly the camera into the room, drag a rectangle around the whole room with <kbd>Shift</kbd> + right mouse button in the top view and press <kbd>F4</kbd> (**{{KeLBrush}} > {{KeLAutoCaulkSelected}}**): every face that cannot be seen from inside becomes caulk. Building in caulk from the start works too: give new brushes the caulk texture and texture only the faces you see.":
        "**Pon caulk por fuera.** Las caras que nadie puede ver siguen costando espacio de lightmap y tiempo de dibujo. Vuela con la cámara dentro de la sala, arrastra un rectángulo alrededor de toda la sala con <kbd>Mayús</kbd> + botón derecho en la vista superior y pulsa <kbd>F4</kbd> (**{{KeLBrush}} > {{KeLAutoCaulkSelected}}**): cada cara que no se puede ver desde dentro pasa a ser caulk. Construir con caulk desde el principio también funciona: da a los brushes nuevos la textura caulk y texturiza solo las caras que ves.",
    "After AutoCaulk: the camera inside the room.":
        "Tras AutoCaulk: la cámara dentro de la sala.",
    "**Check it.** **{{KeLBuild}} > {{KeOptMenue}}** counts the game's limits and looks for build problems before you compile. A green room has nothing to fix.":
        "**Compruébalo.** **{{KeLBuild}} > {{KeOptMenue}}** cuenta los límites del juego y busca problemas de compilación antes de compilar. Una sala en verde no tiene nada que corregir.",
    "The optimizer for the tutorial room.":
        "El optimizador para la sala del tutorial.",
    "**Build.** **{{KeLBuild}} > [Final] Fast** runs q3map2 three times - BSP, VIS and the light with bounce. The progress window shows the stage, the progress and the time left. If the map leaked, the console says so in red and a line in the 2D view shows the way out - see [[karte_bauen]].":
        "**Compila.** **{{KeLBuild}} > [Final] Fast** ejecuta q3map2 tres veces: BSP, VIS y la luz con rebote. La ventana de progreso muestra la etapa, el progreso y el tiempo restante. Si el mapa tiene una fuga, la consola lo dice en rojo y una línea en la vista 2D muestra la salida; mira [[karte_bauen]].",
    "The progress window during the build.":
        "La ventana de progreso durante la compilación.",
    "**Walk through it.** **{{KeLView}} > {{KeSpSpawn}}** and then **{{KeSpMenue}}** (<kbd>F6</kbd>): you stand at the info_player_start and walk with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, jump with <kbd>Space</kbd>; <kbd>Esc</kbd> ends. This works even without compiling - the collision comes straight from the brushes (see [[karte_werkzeuge8b#spielen]]).":
        "**Recórrela.** **{{KeLView}} > {{KeSpSpawn}}** y luego **{{KeSpMenue}}** (<kbd>F6</kbd>): estás en el info_player_start y caminas con <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, saltas con <kbd>Espacio</kbd>; <kbd>Esc</kbd> termina. Funciona incluso sin compilar: la colisión sale directamente de los brushes (mira [[karte_werkzeuge8b#spielen]]).",
    "Walking through the room in the editor.":
        "Recorriendo la sala en el editor.",
    "**Into the game.** **{{KeLBuild}} > {{KePkMenue}}** packs `maps/meinekarte.bsp` into `meinekarte.pk3` next to the .bsp. The program never writes into the game folders - copy the .pk3 yourself into `GameData/base` (Jedi Academy, Jedi Outcast) or into the folder of your mod (for example Movie Duels). Start the game, open the console with <kbd>Shift</kbd>+<kbd>~</kbd> and type `devmap meinekarte`.":
        "**Al juego.** **{{KeLBuild}} > {{KePkMenue}}** empaqueta `maps/mimapa.bsp` en `mimapa.pk3` junto al .bsp. El programa nunca escribe en las carpetas del juego: copia tú mismo el .pk3 en `GameData/base` (Jedi Academy, Jedi Outcast) o en la carpeta de tu mod (por ejemplo Movie Duels). Inicia el juego, abre la consola con <kbd>Mayús</kbd>+<kbd>~</kbd> y escribe `devmap mimapa`.",
    "{{KeLBuild}} with **{{KePkMenue}}** (not in NetRadiant-Custom).":
        "{{KeLBuild}} con **{{KePkMenue}}** (no está en NetRadiant-Custom).",
    "Where to go next":
        "Por dónde seguir",
    "Brushes, clipper, patches and the CSG tools: [[karteneditor_bauen]].":
        "Brushes, clipper, patches y las herramientas CSG: [[karteneditor_bauen]].",
    "Build menu, leaks, portals and the build folder: [[karte_bauen]]; faster rebuilds: [[karte_teilbau]].":
        "Menú de compilación, fugas, portales y la carpeta de compilación: [[karte_bauen]]; recompilaciones más rápidas: [[karte_teilbau]].",
    "Limits of the game and fixes: [[karte_optimierer]]; the light preview in the camera: [[karte_tempo]].":
        "Límites del juego y correcciones: [[karte_optimierer]]; la vista previa de la luz en la cámara: [[karte_tempo]].",
    "Terrain, logic view, mirror mode and playing in the editor: [[karte_werkzeuge8b]].":
        "Terreno, vista lógica, modo espejo y jugar en el editor: [[karte_werkzeuge8b]].",
    "A cutscene on your map: [[tut-projekt]].":
        "Una cinemática en tu mapa: [[tut-projekt]].",
    "Map editor: GPU bake":
        "Editor de mapas: horneado en GPU",
    "Compute the whole light of a map on the graphics card - lightmaps, bounce and light grid - byte for byte the same as on the CPU, and much faster on big maps.":
        "Calcula toda la luz de un mapa en la tarjeta gráfica (lightmaps, rebote y rejilla de luz), byte a byte igual que en la CPU, y mucho más rápido en mapas grandes.",
    "**{{KeLBuild}} > {{KeGpuMenue}}** (not in NetRadiant-Custom) decides where bhed_q3map2 computes the light of every `-light` step: **{{KeGpuAuto}}** (the default), **{{KeGpuEin}}** (switch `-bhedgpubake`) or **{{KeGpuAus}}**. The compiled map is **the same, byte for byte** - the graphics card is only faster. The choice is kept (setting `BhedGpuBakeMode`).":
        "**{{KeLBuild}} > {{KeGpuMenue}}** (no está en NetRadiant-Custom) decide dónde calcula bhed_q3map2 la luz de cada paso `-light`: **{{KeGpuAuto}}** (por defecto), **{{KeGpuEin}}** (opción `-bhedgpubake`) o **{{KeGpuAus}}**. El mapa compilado es **el mismo, byte a byte**: la tarjeta gráfica solo es más rápida. La elección se conserva (ajuste `BhedGpuBakeMode`).",
    "**{{KeGpuAuto}}** takes the graphics card only when its result is bit-identical (it computes with double precision) and it is expected to be faster: large maps and bounce rounds. Small maps, and `-samples 4` on large maps, stay on the processor; a graphics card without double precision is never chosen. It also remembers, per map and light switches, how long the last processor run and the last graphics card run took, and from then on takes the faster one (also for the quick bake). The console names the choice and the reason, for example \"GPU bake (Auto): processor - little light work (34490 super luxels)\".":
        "**{{KeGpuAuto}}** toma la tarjeta gráfica solo cuando su resultado es idéntico bit a bit (calcula con doble precisión) y se espera que sea más rápida: mapas grandes y rondas de rebote. Los mapas pequeños, y `-samples 4` en mapas grandes, se quedan en el procesador; una tarjeta gráfica sin doble precisión nunca se elige. También recuerda, por mapa y opciones de luz, cuánto tardaron la última ejecución en procesador y la última en tarjeta gráfica, y desde entonces toma la más rápida (también para el horneado rápido). La consola nombra la elección y el motivo, por ejemplo \"Horneado en GPU (Auto): procesador - poco trabajo de luz (34490 superluxels)\".",
    "{{KeLBuild}}: {{KeGpuMenue}}, next to the partial compiles.":
        "{{KeLBuild}}: {{KeGpuMenue}}, junto a las compilaciones parciales.",
    "While the map builds, the build progress window and the status line name the graphics card. After the build the console shows the **deviation report**: every 64th lightmap is computed a second time on the CPU and compared luxel by luxel. The last report is also in the tooltip of {{KeGpuMenue}}. The quick bake of the real-time light ([[karte_tempo]]) uses the graphics card too when the entry is on.":
        "Mientras se compila el mapa, la ventana de progreso de la compilación y la línea de estado nombran la tarjeta gráfica. Tras la compilación, la consola muestra el **informe de desviación**: uno de cada 64 lightmaps se calcula una segunda vez en la CPU y se compara luxel a luxel. El último informe también está en la descripción emergente de {{KeGpuMenue}}. El horneado rápido de la luz en tiempo real ([[karte_tempo]]) también usa la tarjeta gráfica cuando la entrada está activada.",
    "After the build: the deviation report in the console.":
        "Tras la compilación: el informe de desviación en la consola.",
    "What runs on the graphics card":
        "Qué se ejecuta en la tarjeta gráfica",
    "**Lightmaps**: for every luxel the contribution of every light with its shadow ray, the style check, the adaptive supersampling of `-samples` (also several levels), the filter of `-filter` and adding the lights in their order.":
        "**Lightmaps**: para cada luxel, la aportación de cada luz con su rayo de sombra, la comprobación de estilo, el supermuestreo adaptativo de `-samples` (también varios niveles), el filtro de `-filter` y la suma de las luces en su orden.",
    "**Bounce** (`-bounce`): every round, with the surface lights the radiosity creates.":
        "**Rebote** (`-bounce`): cada ronda, con las luces de superficie que crea la radiosidad.",
    "**Light grid** (the light for models and players): every grid point with all lights.":
        "**Rejilla de luz** (la luz para modelos y jugadores): cada punto de la rejilla con todas las luces.",
    "On the processor as always: loading, the light lists, vertex light, creating the bounce lights, stitching, storing and writing the .bsp - and everything the graphics card cannot do exactly (see below).":
        "En el procesador como siempre: la carga, las listas de luces, la luz de vértices, la creación de las luces de rebote, el cosido, el guardado y la escritura del .bsp, y todo lo que la tarjeta gráfica no puede hacer exactamente (mira abajo).",
    "Why the result is the same":
        "Por qué el resultado es el mismo",
    "Graphics cards usually compute a little differently from the processor - that is why GPU lightmappers elsewhere give slightly different light. bhed_q3map2 does the **same calculations in the same order** as q3map2 on the processor:":
        "Las tarjetas gráficas suelen calcular un poco distinto del procesador; por eso otros lightmappers por GPU dan una luz ligeramente distinta. bhed_q3map2 hace **los mismos cálculos en el mismo orden** que q3map2 en el procesador:",
    "every operation is rounded exactly like on the processor (no fused multiply-add); divisions are rounded exactly in integer arithmetic;":
        "cada operación se redondea exactamente como en el procesador (sin multiplicación-suma fusionada); las divisiones se redondean exactamente con aritmética entera;",
    "q3map2 normalizes vectors and takes square roots in double precision; the graphics card decides the same rounded result with extra-precise float arithmetic and, in the rare close cases, in double precision;":
        "q3map2 normaliza vectores y saca raíces cuadradas en doble precisión; la tarjeta gráfica decide el mismo resultado redondeado con aritmética de coma flotante de precisión extra y, en los raros casos ajustados, en doble precisión;",
    "the shadow rays walk q3map2's own trace tree with its tolerances, and the supersampling follows the same order of stamps as the processor;":
        "los rayos de sombra recorren el propio árbol de trazado de q3map2 con sus tolerancias, y el supermuestreo sigue el mismo orden de sellos que el procesador;",
    "nothing is summed up in the order threads happen to finish - every luxel adds its lights in list order.":
        "nada se suma en el orden en que terminan los hilos: cada luxel suma sus luces en el orden de la lista.",
    "Whatever the graphics card cannot compute exactly - alpha shadows and light filters (they need the texture), a value outside the safe range, a result that is not finite - is computed for that lightmap (or grid point) on the processor. If the graphics card fails or is reset by Windows during the build, the processor computes the rest. A lightmap too big for the graphics memory is computed in parts of its light list.":
        "Lo que la tarjeta gráfica no puede calcular exactamente (sombras alfa y filtros de luz, que necesitan la textura, un valor fuera del rango seguro, un resultado no finito) se calcula para ese lightmap (o punto de rejilla) en el procesador. Si la tarjeta gráfica falla o Windows la reinicia durante la compilación, el procesador calcula el resto. Un lightmap demasiado grande para la memoria gráfica se calcula por partes de su lista de luces.",
    "Graphics cards without double precision (for example Intel UHD) also run the GPU bake, but there the rare close cases are only approximated: the light differs very slightly (the report shows how much). On such a computer leave {{KeGpuMenue}} off for the final build.":
        "Las tarjetas gráficas sin doble precisión (por ejemplo Intel UHD) también ejecutan el horneado en GPU, pero ahí los raros casos ajustados solo se aproximan: la luz difiere muy ligeramente (el informe muestra cuánto). En un ordenador así, deja {{KeGpuMenue}} desactivado para la compilación final.",
    "How much faster":
        "Cuánto más rápido",
    "Measured on a notebook with an Intel i9-14900HX (32 threads) and an NVIDIA RTX 4070 Laptop GPU, the whole `-light` step (median of three runs). The processor uses all 32 threads; the GPU bake still needs the processor for the parts listed above, so the gain is largest on big maps with many lights and bounce rounds.":
        "Medido en un portátil con un Intel i9-14900HX (32 hilos) y una NVIDIA RTX 4070 Laptop GPU, todo el paso `-light` (mediana de tres ejecuciones). El procesador usa los 32 hilos; el horneado en GPU sigue necesitando el procesador para las partes listadas arriba, así que la ganancia es mayor en mapas grandes con muchas luces y rondas de rebote.",
    "Switches":
        "Opciones",
    "Processor":
        "Procesador",
    "Graphics card":
        "Tarjeta gráfica",
    "Same luxels":
        "Luxels iguales",
    "Small maps gain little: starting the graphics card costs a few tenths of a second. One case is slower on the graphics card: duel_mygeeto with `-samples 4` (101 s against 67 s) - the deep supersampling at many shadow edges branches differently for every luxel, and graphics cards are slow at that. With `-samples 2` (the Build menu) every map measured was as fast or faster.":
        "Los mapas pequeños ganan poco: arrancar la tarjeta gráfica cuesta unas décimas de segundo. Un caso es más lento en la tarjeta gráfica: duel_mygeeto con `-samples 4` (101 s frente a 67 s); el supermuestreo profundo en muchos bordes de sombra se ramifica distinto en cada luxel, y las tarjetas gráficas son lentas en eso. Con `-samples 2` (el menú Build) cada mapa medido fue igual de rápido o más.",
    "For the command line":
        "Para la línea de comandos",
    "`-bhedgpubake` - the light on the graphics card (with `-light`).":
        "`-bhedgpubake`: la luz en la tarjeta gráfica (con `-light`).",
    "`-bhedgpubakeauto` - the faster of processor and graphics card, the graphics card only bit-identical; `-bhedgpubakezeiten <cpu ms> <gpu ms>` gives the measured times of the last runs (0 = unknown).":
        "`-bhedgpubakeauto`: lo más rápido de procesador y tarjeta gráfica, la tarjeta gráfica solo si es idéntica bit a bit; `-bhedgpubakezeiten <ms cpu> <ms gpu>` da los tiempos medidos de las últimas ejecuciones (0 = desconocido).",
    "`-bhedgpubakecheck N` - every N-th lightmap is computed on the CPU too (default 64, up to one million pairs of light and luxel; `1` checks all, `0` none). The report is also written to `<map>.bhedgpu.txt`.":
        "`-bhedgpubakecheck N`: uno de cada N lightmaps se calcula también en la CPU (por defecto 64, hasta un millón de pares de luz y luxel; `1` los comprueba todos, `0` ninguno). El informe también se escribe en `<mapa>.bhedgpu.txt`.",
    "`-bhedgpuadapter N` or a part of the name - which graphics card (otherwise the fastest).":
        "`-bhedgpuadapter N` o parte del nombre: qué tarjeta gráfica (si no, la más rápida).",
    "Hardware ray tracing (DXR) was measured as a pre-test (`-bhedgpudxr` with `-bhedgpu`): it traces the shadow rays about twice as fast, but without q3map2's tolerances it misses about 0.15 % of the hits and cannot see the solid parts of the trace tree - and the shadow rays are only 7-8 % of the time of a GPU bake. It is therefore not used for baking.":
        "El trazado de rayos por hardware (DXR) se midió como prueba previa (`-bhedgpudxr` con `-bhedgpu`): traza los rayos de sombra aproximadamente el doble de rápido, pero sin las tolerancias de q3map2 falla cerca del 0,15 % de los impactos y no ve las partes sólidas del árbol de trazado; y los rayos de sombra son solo el 7-8 % del tiempo de un horneado en GPU. Por eso no se usa para hornear.",
    "Map editor: test in game and more tools":
        "Editor de mapas: probar en el juego y más herramientas",
    "Start the game with the current map at the press of a button, and turn models from Blender and other games into brushes.":
        "Inicia el juego con el mapa actual con solo pulsar un botón, y convierte en brushes modelos de Blender y de otros juegos.",
    "Test in game":
        "Probar en el juego",
    "**{{KeLBuild}} > {{KeStMenue}}** lists the engine from the preferences and every game program found in the engine folder: Jedi Academy SP/MP, OpenJK, Movie Duels and, for Jedi Outcast maps, Jedi Outcast SP/MP and OpenJO. A click starts the chosen program with the current map (`+devmap`). The choice is remembered.":
        "**{{KeLBuild}} > {{KeStMenue}}** lista el motor de las preferencias y cada programa de juego encontrado en la carpeta del motor: Jedi Academy SP/MP, OpenJK, Movie Duels y, para mapas de Jedi Outcast, Jedi Outcast SP/MP y OpenJO. Un clic inicia el programa elegido con el mapa actual (`+devmap`). La elección se recuerda.",
    "**Which .bsp:** the quick bake of the current map if it is up to date (**{{KeBackenMenue}}**), otherwise a .bsp that is newer than the saved, unchanged map, otherwise the program builds first with the first **[Test]** entry of the Build menu and starts the game when the build is done.":
        "**Qué .bsp:** el horneado rápido del mapa actual si está al día (**{{KeBackenMenue}}**); si no, un .bsp más reciente que el mapa guardado y sin cambios; si no, el programa compila primero con la primera entrada **[Test]** del menú Build e inicia el juego cuando termina la compilación.",
    "**Where it goes:** the .bsp is packed as `zzz_jkstudio_test.pk3` into the folder `spieltest` next to the program settings - never into the game folder. The game learns about the folder from a start parameter: `fs_cdpath` (all engines of this family; your game settings stay as they are) or, if the game already has a map with the same name, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), because that folder takes precedence. Which parameter a program knows is read from its .exe.":
        "**Adónde va:** el .bsp se empaqueta como `zzz_jkstudio_test.pk3` en la carpeta `spieltest` junto a los ajustes del programa, nunca en la carpeta del juego. El juego conoce la carpeta por un parámetro de inicio: `fs_cdpath` (todos los motores de esta familia; los ajustes de tu juego no cambian) o, si el juego ya tiene un mapa con el mismo nombre, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), porque esa carpeta tiene prioridad. Qué parámetro conoce un programa se lee de su .exe.",
    "**Only if neither works** (for example Jedi Academy SP with a map name the game already has), the program asks before it writes the archive into the game: into a mod folder `jkstudio_test` (started with `+set fs_game jkstudio_test`), or into the folder of the active mod. You can delete the file later.":
        "**Solo si ninguno funciona** (por ejemplo Jedi Academy SP con un nombre de mapa que el juego ya tiene), el programa pregunta antes de escribir el archivo en el juego: en una carpeta de mod `jkstudio_test` (iniciada con `+set fs_game jkstudio_test`), o en la carpeta del mod activo. Puedes borrar el archivo más tarde.",
    "The console shows the way that was chosen and the whole command line. Map names with spaces cannot be loaded with `devmap`; save the map under a name without spaces.":
        "La consola muestra el camino elegido y la línea de comandos completa. Los nombres de mapa con espacios no se pueden cargar con `devmap`; guarda el mapa con un nombre sin espacios.",
    "Not in NetRadiant-Custom. NetRadiant-Custom's own **Run Engine After Compile** (preferences, page Build) stays as it is: it starts the engine from the game folder after every build.":
        "No está en NetRadiant-Custom. El propio **Run Engine After Compile** de NetRadiant-Custom (preferencias, página Build) se queda como está: inicia el motor desde la carpeta del juego tras cada compilación.",
    "Geometry to brushes":
        "Geometría a brushes",
    "**{{KeLFile}} > {{KeGiMenue}}** reads a model (.obj, .fbx, .glb or .gltf, for example from Blender) and turns it into brushes. A window lists the parts of the file; for each part choose **{{KeGiAuto}}**, **{{KeGiPinsel}}** or **{{KeGiModellClip}}** - the column **{{KeGiErgebnis}}** shows at once how many brushes it becomes and whether they are detail. **{{KeGiEinfuegen}}** inserts everything as one undo step and selects it; then the optimizer opens.":
        "**{{KeLFile}} > {{KeGiMenue}}** lee un modelo (.obj, .fbx, .glb o .gltf, por ejemplo de Blender) y lo convierte en brushes. Una ventana lista las piezas del archivo; para cada pieza elige **{{KeGiAuto}}**, **{{KeGiPinsel}}** o **{{KeGiModellClip}}**; la columna **{{KeGiErgebnis}}** muestra enseguida cuántos brushes salen y si son de detalle. **{{KeGiEinfuegen}}** lo inserta todo como un paso de deshacer y lo selecciona; luego se abre el optimizador.",
    "**Closed parts** are split into convex brushes (a BSP over the faces of the model, like q3map2 builds one). Faces that lie in one plane become one brush side. Sides on the surface of the model get its material as shader (**{{KeGiVorsatz}}** + material name) and its UV as texture alignment; sides inside the model, which nobody sees, get caulk.":
        "**Las piezas cerradas** se dividen en brushes convexos (un BSP sobre las caras del modelo, como lo construye q3map2). Las caras que están en un plano se convierten en un lado de brush. Los lados en la superficie del modelo reciben su material como shader (**{{KeGiVorsatz}}** + nombre del material) y su UV como alineación de textura; los lados dentro del modelo, que nadie ve, reciben caulk.",
    "**Open parts** (single surfaces, decoration) become thin brushes behind each flat face; small and open parts become detail, so they do not cut the map into BSP leaves.":
        "**Las piezas abiertas** (superficies sueltas, decoración) se convierten en brushes finos detrás de cada cara plana; las piezas pequeñas y abiertas pasan a ser de detalle, para que no corten el mapa en hojas de BSP.",
    "**{{KeGiModellClip}}** keeps the part as a model: it is written as .obj into the program's import folder (next to the settings, never into the game), a misc_model points to it, and clip brushes from the same decomposition make it solid. The editor and q3map2 (`-fs_pakpath`) search this folder. **{{KeGiAuto}}** takes brushes for closed parts with at most 256 brushes, otherwise model + clip.":
        "**{{KeGiModellClip}}** conserva la pieza como modelo: se escribe como .obj en la carpeta de importación del programa (junto a los ajustes, nunca en el juego), un misc_model apunta a él y brushes de colisión de la misma descomposición lo hacen sólido. El editor y q3map2 (`-fs_pakpath`) buscan en esta carpeta. **{{KeGiAuto}}** toma brushes para las piezas cerradas con como máximo 256 brushes; si no, modelo + colisión.",
    "**{{KeGiSkala}}** and **{{KeGiYOben}}**: Blender and glTF use Y as up, the game Z; one Blender metre is one unit unless you scale it. A **.bsp** of another id Tech 3 game (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) opens like **{{KeLOpenBsp}}**, with the reader of that game.":
        "**{{KeGiSkala}}** y **{{KeGiYOben}}**: Blender y glTF usan Y como arriba, el juego Z; un metro de Blender es una unidad salvo que lo escales. Un **.bsp** de otro juego id Tech 3 (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) se abre como con **{{KeLOpenBsp}}**, con el lector de ese juego.",
    "Drag a model into the map":
        "Arrastrar un modelo al mapa",
    "Drag an .obj, .fbx, .glb or .gltf file from the Explorer onto the map editor. The program converts it to an MD3 with its own shader and copies the textures (embedded in the file or next to it); all of it goes into the import folder next to the settings, never into the game. A misc_model appears under the mouse pointer in the 2D view (or in front of the camera) and is selected; one undo removes it. Models larger than 511 units are scaled down in the MD3 and get `modelscale`.":
        "Arrastra un archivo .obj, .fbx, .glb o .gltf desde el Explorador al editor de mapas. El programa lo convierte en un MD3 con su propio shader y copia las texturas (incrustadas en el archivo o a su lado); todo va a la carpeta de importación junto a los ajustes, nunca al juego. Aparece un misc_model bajo el puntero del ratón en la vista 2D (o delante de la cámara) y queda seleccionado; un deshacer lo quita. Los modelos de más de 511 unidades se reducen en el MD3 y reciben `modelscale`.",
    "**{{KeStMenue}}** and **{{KePkMenue}}** pack the import folder into their archive, so the game finds the models, shaders and textures.":
        "**{{KeStMenue}}** y **{{KePkMenue}}** empaquetan la carpeta de importación en su archivo, para que el juego encuentre los modelos, shaders y texturas.",
    "Check jumps and space":
        "Comprobar saltos y espacio",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeSjMenue}} > {{KeSjPruefen}}** checks the selection (or 512 units around the camera) with the movement of the game: where the player can stand (green crosses) or fits only crouched (yellow), and from which edges a running jump lands on another floor - blue for a normal jump, purple where only the force jump of the chosen level gets there. It runs in the background; **{{KeSjLeeren}}** removes the display.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeSjMenue}} > {{KeSjPruefen}}** comprueba la selección (o 512 unidades alrededor de la cámara) con el movimiento del juego: dónde puede estar de pie el jugador (cruces verdes) o solo cabe agachado (amarillo), y desde qué bordes un salto con carrerilla cae en otro suelo; azul para un salto normal, morado donde solo llega el salto de Fuerza del nivel elegido. Se ejecuta en segundo plano; **{{KeSjLeeren}}** quita la visualización.",
    "The movement is the port of the game code: running speed 250, gravity 800, jump 225, the player box 32 x 32 x 64 (crouched 40), steps up to 18 units, floors up to 45 degrees. The force jump follows the game: holding the jump key it rises up to 96, 192 or 384 units (levels 1 to 3); letting go earlier jumps lower. Jedi Outcast and Movie Duels compute it the same way.":
        "El movimiento es el port del código del juego: velocidad de carrera 250, gravedad 800, salto 225, la caja del jugador de 32 x 32 x 64 (agachado 40), escalones de hasta 18 unidades, suelos de hasta 45 grados. El salto de Fuerza sigue al juego: manteniendo la tecla de salto sube hasta 96, 192 o 384 unidades (niveles 1 a 3); soltarla antes salta más bajo. Jedi Outcast y Movie Duels lo calculan igual.",
    "The chosen force jump level also applies to **{{KeSpMenue}}** (F6): hold the space bar to jump higher.":
        "El nivel de salto de Fuerza elegido también vale para **{{KeSpMenue}}** (F6): mantén la barra espaciadora para saltar más alto.",
    "NPC paths":
        "Caminos de NPC",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeNwMenue}} > {{KeNwNetz}}** shows the network the game builds from the waypoints when it loads the map - connections turquoise, jump connections orange. The game builds a network only from more than three waypoints; the console says so. **{{KeNwLauf}}**: select two entities (for example two waypoints, or an info_player_start and a waypoint) - an NPC walks from the first to the last one with the game's path finding and movement; the path is yellow and a box walks along it in real time. The console says after how many seconds it arrives, or where it gets stuck. **{{KeNwRennen}}** switches between running and walking.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeNwMenue}} > {{KeNwNetz}}** muestra la red que construye el juego a partir de los waypoints al cargar el mapa: conexiones en turquesa, conexiones de salto en naranja. El juego solo construye una red a partir de más de tres waypoints; la consola lo dice. **{{KeNwLauf}}**: selecciona dos entidades (por ejemplo dos waypoints, o un info_player_start y un waypoint): un NPC camina de la primera a la última con la búsqueda de caminos y el movimiento del juego; el camino es amarillo y una caja lo recorre en tiempo real. La consola dice a los cuántos segundos llega, o dónde se atasca. **{{KeNwRennen}}** cambia entre correr y caminar.",
    "Linked copies":
        "Copias vinculadas",
    "Select brushes and patches (for example a pillar or a lamp) and choose **{{KeLView}} > {{KeWzMenue}} > {{KeVbMenue}} > {{KeVbKopie}}**: they become a linked group, and a copy appears next to it. Select the copy and repeat for more copies. From now on, whatever you change in one copy - add, delete, resize or retexture brushes - happens in all of them in the same step; one undo takes it back everywhere. Moving a whole copy moves only that copy. **{{KeVbLoesen}}** makes a copy independent again.":
        "Selecciona brushes y patches (por ejemplo una columna o una lámpara) y elige **{{KeLView}} > {{KeWzMenue}} > {{KeVbMenue}} > {{KeVbKopie}}**: se convierten en un grupo vinculado y aparece una copia al lado. Selecciona la copia y repite para más copias. A partir de ahora, lo que cambies en una copia (añadir, eliminar, cambiar el tamaño o la textura de brushes) ocurre en todas en el mismo paso; un deshacer lo revierte en todas. Mover una copia entera mueve solo esa copia. **{{KeVbLoesen}}** vuelve a independizar una copia.",
    "The copies are func_groups with the keys `_bhed_link` and `_bhed_link_origin`; the game and q3map2 treat them like any func_group. Copies are moved, not rotated.":
        "Las copias son func_groups con las claves `_bhed_link` y `_bhed_link_origin`; el juego y q3map2 los tratan como cualquier func_group. Las copias se mueven, no se giran.",
    "Curve tool":
        "Herramienta de curvas",
    "Place a few point entities (for example info_null) along the course you want and select them in that order. **{{KeLView}} > {{KeWzMenue}} > {{KeKuMenue}}** lays a smooth curve through them and builds, as one undo step: **{{KeKuWeg}}** (a flat strip, 128 wide), **{{KeKuRohr}}**, **{{KeKuGelaender}}** (a rail on each side at hand height), **{{KeKuBruecke}}** (deck with sides and underside), **{{KeKuPfad}}** (a chain of path_corner linked by targetname/target - for func_train or a camera track) or **{{KeKuWegpunkte}}** (a linked chain of waypoints). Patches get the texture selected in the texture browser; the spacing follows the grid.":
        "Coloca unas cuantas entidades de punto (por ejemplo info_null) a lo largo del recorrido que quieres y selecciónalas en ese orden. **{{KeLView}} > {{KeWzMenue}} > {{KeKuMenue}}** traza por ellas una curva suave y construye, como un paso de deshacer: **{{KeKuWeg}}** (una franja plana, de 128 de ancho), **{{KeKuRohr}}**, **{{KeKuGelaender}}** (una barandilla a cada lado a la altura de la mano), **{{KeKuBruecke}}** (tablero con lados y parte inferior), **{{KeKuPfad}}** (una cadena de path_corner enlazados por targetname/target, para func_train o un recorrido de cámara) o **{{KeKuWegpunkte}}** (una cadena enlazada de waypoints). Los patches reciben la textura seleccionada en el navegador de texturas; la separación sigue a la rejilla.",
    "Scatter models":
        "Dispersar modelos",
    "Place one misc_model_static (or misc_model) as the template - for example a rock or a crate - and select it together with the brushes of the area (for example the floor). **{{KeLView}} > {{KeWzMenue}} > {{KeVtMenue}}** puts 10, 25 or 50 copies onto the walkable surfaces in that area, with random angle and scale, at least 48 units apart, as one undo step. Without brushes it uses 256 units around the template.":
        "Coloca un misc_model_static (o misc_model) como plantilla (por ejemplo una roca o una caja) y selecciónalo junto con los brushes de la zona (por ejemplo el suelo). **{{KeLView}} > {{KeWzMenue}} > {{KeVtMenue}}** pone 10, 25 o 50 copias sobre las superficies transitables de esa zona, con ángulo y escala aleatorios, separadas al menos 48 unidades, como un paso de deshacer. Sin brushes usa 256 unidades alrededor de la plantilla.",
    "Every misc_model_static becomes a static object of the game, and Jedi Academy and Movie Duels stop with an error after 2000 (MAX_MISC_ENTS). Scattering stops at that limit, and the optimizer counts misc_model_static against it. Jedi Outcast has no misc_model_static; there the copies are misc_model.":
        "Cada misc_model_static se convierte en un objeto estático del juego, y Jedi Academy y Movie Duels se detienen con un error a partir de 2000 (MAX_MISC_ENTS). La dispersión se detiene en ese límite, y el optimizador cuenta los misc_model_static frente a él. Jedi Outcast no tiene misc_model_static; ahí las copias son misc_model.",
    "Terrain sculpt":
        "Esculpir terreno",
    "Make a terrain of patches (for example a big patch with many rows, or the patches of the terrain generator). **{{KeLView}} > {{KeWzMenue}} > {{KeFoMenue}} > {{KeFoAn}}**: now the left mouse button in the camera shapes the patches under the pointer instead of selecting - hold it and move. Choose **{{KeFoHeben}}**, **{{KeFoSenken}}**, **{{KeFoGlaetten}}** or **{{KeFoEbnen}}** (to the height where you pressed) and a radius of 64, 128 or 256; the effect fades towards the edge of the circle. One stroke is one undo step.":
        "Haz un terreno de patches (por ejemplo un patch grande con muchas filas, o los patches del generador de terreno). **{{KeLView}} > {{KeWzMenue}} > {{KeFoMenue}} > {{KeFoAn}}**: ahora el botón izquierdo en la cámara da forma a los patches bajo el puntero en vez de seleccionar; mantenlo pulsado y muévelo. Elige **{{KeFoHeben}}**, **{{KeFoSenken}}**, **{{KeFoGlaetten}}** o **{{KeFoEbnen}}** (a la altura donde pulsaste) y un radio de 64, 128 o 256; el efecto se suaviza hacia el borde del círculo. Un trazo es un paso de deshacer.",
    "Only patches are shaped; terrain brushes keep their shape (shape them with the vertex tool or Terrain from the plugins). How textures blend on the terrain is described below.":
        "Solo se da forma a los patches; los brushes de terreno conservan su forma (dales forma con la herramienta de vértices o con Terrain de los plugins). Cómo se mezclan las texturas en el terreno se describe abajo.",
    "Texture blending":
        "Mezcla de texturas",
    "Make the terrain a func_group (brushes or patches), select it and choose **{{KeLView}} > {{KeWzMenue}} > {{KeUbMenue}}**. Choose 2 to 4 **{{KeUbSchichten}}** and a texture for each (**{{KeUbAktuell}}** takes the one selected in the texture browser), then **{{KeUbEinrichten}}**: the group gets the q3map2 keys `_indexmap`, `_layers` and `_shader`, its faces the blend shader, and the import folder an index map and the shaders for every layer and every pair of layers - one undo step. Now paint with the mouse in the top view of the window: choose the layer and the **{{KeUbRadius}}**; top is north (largest Y), as in the 2D view. Building the map blends the textures where the layers meet.":
        "Haz del terreno un func_group (brushes o patches), selecciónalo y elige **{{KeLView}} > {{KeWzMenue}} > {{KeUbMenue}}**. Elige de 2 a 4 **{{KeUbSchichten}}** y una textura para cada una (**{{KeUbAktuell}}** toma la seleccionada en el navegador de texturas), luego **{{KeUbEinrichten}}**: el grupo recibe las claves de q3map2 `_indexmap`, `_layers` y `_shader`, sus caras el shader de mezcla, y la carpeta de importación un mapa de índices y los shaders para cada capa y cada par de capas, en un paso de deshacer. Ahora pinta con el ratón en la vista superior de la ventana: elige la capa y el **{{KeUbRadius}}**; arriba es el norte (Y mayor), como en la vista 2D. Al compilar el mapa se mezclan las texturas donde se encuentran las capas.",
    "The blend shaders use `q3map_terrain` and per-vertex alpha: the finer the terrain (more vertices), the finer the transitions. Texture blending is not in NetRadiant-Custom; the keys and the index map are those of q3map2.":
        "Los shaders de mezcla usan `q3map_terrain` y alfa por vértice: cuanto más fino el terreno (más vértices), más finas las transiciones. La mezcla de texturas no está en NetRadiant-Custom; las claves y el mapa de índices son los de q3map2.",
    "Asset browser":
        "Navegador de recursos",
    "**{{KeLView}} > {{KeWzMenue}} > {{KeAbMenue}}** opens a window with the models (.md3, .glm, .ase, .obj), the prefabs and the sounds of the game, each with a preview: the texture of a model, the waveform of a sound, the outline of a prefab from above. **{{KeAbSuche}}** narrows the list - every word must appear in the path. Drag an entry onto a 2D view or the camera: a model becomes a misc_model, a sound a target_speaker, a prefab is inserted - at the place under the pointer. A double-click puts it in front of the camera. Everything is one undo step.":
        "**{{KeLView}} > {{KeWzMenue}} > {{KeAbMenue}}** abre una ventana con los modelos (.md3, .glm, .ase, .obj), los prefabs y los sonidos del juego, cada uno con vista previa: la textura de un modelo, la forma de onda de un sonido, el contorno de un prefab desde arriba. **{{KeAbSuche}}** reduce la lista: cada palabra debe aparecer en la ruta. Arrastra una entrada a una vista 2D o a la cámara: un modelo se convierte en misc_model, un sonido en target_speaker, un prefab se inserta, en el sitio bajo el puntero. Un doble clic lo pone delante de la cámara. Todo es un paso de deshacer.",
    "Prefabs are .map files in the import folder (`import/prefabs` in the settings folder). Select brushes and entities, enter a name under {{KeAbPrefabs}} and click **{{KeAbPrefabSpeichern}}**. **{{KeLBuild}} > {{KePkMenue}}** takes the import folder along. The asset browser is not in NetRadiant-Custom.":
        "Los prefabs son archivos .map en la carpeta de importación (`import/prefabs` en la carpeta de ajustes). Selecciona brushes y entidades, escribe un nombre en {{KeAbPrefabs}} y pulsa **{{KeAbPrefabSpeichern}}**. **{{KeLBuild}} > {{KePkMenue}}** se lleva la carpeta de importación. El navegador de recursos no está en NetRadiant-Custom.",
    "To Blender and back":
        "A Blender y de vuelta",
    "Select brushes and choose **{{KeLFile}} > {{KeBlMenue}}**: they are written as .glb (or .obj) with Y up, the way Blender expects, one object per brush, the shader names as materials. Edit the part in Blender and export it again as .glb or .obj with the default settings. **{{KeLFile}} > {{KeGiMenue}}** with **{{KeGiYOben}}** switched on (the default) brings it back to the same place in the map.":
        "Selecciona brushes y elige **{{KeLFile}} > {{KeBlMenue}}**: se escriben como .glb (u .obj) con Y arriba, como espera Blender, un objeto por brush, los nombres de shader como materiales. Edita la pieza en Blender y vuelve a exportarla como .glb u .obj con los ajustes predeterminados. **{{KeLFile}} > {{KeGiMenue}}** con **{{KeGiYOben}}** activado (por defecto) la devuelve al mismo sitio del mapa.",
    "Restore after a crash":
        "Restaurar tras un fallo",
    "The map editor saves automatically like NetRadiant-Custom (preferences, page Autosave: `<map>.autosave.map` next to the map). If the program did not close properly - a crash, a power cut - the editor notices it at the next start and asks whether to load the autosave, when it is newer than the map. The restored map keeps its name and counts as changed: save it to keep it. Answer no, and the last map opens as usual.":
        "El editor de mapas guarda automáticamente como NetRadiant-Custom (preferencias, página Autosave: `<mapa>.autosave.map` junto al mapa). Si el programa no se cerró correctamente (un fallo, un corte de luz), el editor lo nota en el siguiente inicio y pregunta si cargar el autoguardado, cuando es más reciente que el mapa. El mapa restaurado conserva su nombre y cuenta como cambiado: guárdalo para conservarlo. Si respondes que no, el último mapa se abre como siempre.",
    "Menu editor":
        "Editor de menús",
    "Edit the game's menus (.menu) as text and on a 640x480 canvas at the same time - read, drawn and operated like in the game.":
        "Edita los menús del juego (.menu) como texto y en un lienzo de 640x480 a la vez: leídos, dibujados y manejados como en el juego.",
    "Open a menu":
        "Abrir un menú",
    "Click **{{ViewMenuEditor}}** in the mode bar. **{{MeAusSpiel}}** lists every `ui/` menu of the game set up in the preferences (read from the .pk3, never changed); **{{MeDateiOeffnen}}** or dropping a file opens one from disk. The path decides how the file is read, exactly like the game: `ui/jamp/`, `ui/jk2mp/` and `ui/MD_MP/` with the multiplayer precompiler (#include, #define, macros, menudef.h), everything else like single player.":
        "Pulsa **{{ViewMenuEditor}}** en la barra de modos. **{{MeAusSpiel}}** lista cada menú `ui/` del juego configurado en las preferencias (leído del .pk3, nunca cambiado); **{{MeDateiOeffnen}}** o soltar un archivo abre uno del disco. La ruta decide cómo se lee el archivo, exactamente como el juego: `ui/jamp/`, `ui/jk2mp/` y `ui/MD_MP/` con el precompilador de multijugador (#include, #define, macros, menudef.h), todo lo demás como un jugador.",
    "Canvas and pages":
        "Lienzo y páginas",
    "The multiplayer main menu of Jedi Academy, read from the game files and drawn like the game.":
        "El menú principal de multijugador de Jedi Academy, leído de los archivos del juego y dibujado como en el juego.",
    "Every `menuDef` of the file is a tab above the canvas; items that are shown by a cvar with several values (`cvarTest` + `showCvar`, like `ui_tab` pages) get a tab per value. **+** adds a new `menuDef`. The canvas is the game's virtual 640x480 screen: **{{MeStrecken}}** like the original game, **{{MeBalken}}**, and preset resolutions to check the aspect ratio.":
        "Cada `menuDef` del archivo es una pestaña encima del lienzo; los elementos que muestra una cvar con varios valores (`cvarTest` + `showCvar`, como las páginas de `ui_tab`) reciben una pestaña por valor. **+** añade un `menuDef` nuevo. El lienzo es la pantalla virtual de 640x480 del juego: **{{MeStrecken}}** como el juego original, **{{MeBalken}}**, y resoluciones predefinidas para comprobar la relación de aspecto.",
    "An element keeps its type: **type** in the properties only shows it. For another type, insert a new element. In the text editor you can still write anything; if a line makes the game drop the whole menu, the preview reads on, keeps all elements in place and says above the canvas which line it is.":
        "Un elemento conserva su tipo: **type** en las propiedades solo lo muestra. Para otro tipo, inserta un elemento nuevo. En el editor de texto puedes seguir escribiendo lo que quieras; si una línea hace que el juego descarte todo el menú, la vista previa sigue leyendo, deja todos los elementos en su sitio e indica sobre el lienzo qué línea es.",
    "Ctrl+Z, Ctrl+Y and the arrows in the bar work wherever the focus is in the menu editor and also restore the selection. Only a field you are typing in and the text editor use their own keys; selecting something is not a step.":
        "Ctrl+Z, Ctrl+Y y las flechas de la barra funcionan en todo el editor de menús y también restauran la selección. Solo el campo en el que escribes y el editor de texto usan sus propias teclas; seleccionar no es un paso.",
    "A click selects (Shift/Ctrl adds) and never moves anything. Press a selected element again and drag it to move it - the whole selection moves along, in one undo step; the white handles resize. Where elements overlap, a press means the front one, the one with the frame and name under the pointer; Alt+click goes through the stack, Alt+drag moves the selected one even when it is covered. Only the numbers of `rect` change in the text - comments and indentation stay.":
        "Un clic selecciona (Mayús/Ctrl añade) y nunca mueve nada. Vuelve a pulsar un elemento seleccionado y arrástralo para moverlo: toda la selección se mueve con él, en un solo paso de deshacer; las asas blancas cambian el tamaño. Donde hay elementos superpuestos, una pulsación se refiere al de delante, el que muestra el marco y el nombre bajo el puntero; Alt+clic recorre la pila y Alt+arrastrar mueve el seleccionado aunque esté tapado. En el texto solo cambian los números de `rect`: los comentarios y la sangría se quedan.",
    "**{{MeAnKanten}}** snaps to edges and centres of the other items, the screen and your guides (drag them out of the rulers); **{{MeAnRaster}}** to the grid. Alt switches snapping off while dragging, Shift keeps the direction. Arrow keys move by 1, with Shift by 10.":
        "**{{MeAnKanten}}** ajusta a los bordes y centros de los demás elementos, de la pantalla y de tus guías (arrástralas desde las reglas); **{{MeAnRaster}}** a la rejilla. Alt desactiva el ajuste mientras arrastras, Mayús mantiene la dirección. Las flechas mueven 1, con Mayús 10.",
    "**{{MePalette}}** holds one bare element each (text, button, image, yes/no, multi choice, slider, edit field, list box, model, key bind). Drag one onto the canvas: a frame shows where it lands and it snaps like a moved element. A double-click puts it in the middle, next to what is already there; a plain click does nothing. **{{MeBibliothek}}** further down holds ready-made parts of one or many elements with everything already set; the tooltip says what each contains. **{{MeBildDazu}}** takes a PNG, JPG or TGA as `gfx/menus/<name>`; it is packed into the .pk3 on export.":
        "**{{MePalette}}** contiene un elemento básico de cada tipo (texto, botón, imagen, sí/no, opción múltiple, deslizador, campo de edición, lista, modelo, asignación de tecla). Arrastra uno al lienzo: un marco muestra dónde queda y se ajusta como al mover. Un doble clic lo pone en el centro, junto a lo que ya hay; un clic simple no hace nada. **{{MeBibliothek}}**, más abajo, contiene piezas listas de uno o muchos elementos con todo configurado; la descripción emergente dice qué contienen. **{{MeBildDazu}}** toma un PNG, JPG o TGA como `gfx/menus/<name>`; al exportar se empaqueta en el .pk3.",
    "**{{MeEigenschaften}}** edits the selected item; with several items it aligns, distributes and groups them. **{{MeNachVorn}}**/**{{MeNachHinten}}** change the drawing order (the game draws in file order). **{{MeSperren}}** keeps an item from moving.":
        "**{{MeEigenschaften}}** edita el elemento seleccionado; con varios elementos los alinea, distribuye y agrupa. **{{MeNachVorn}}**/**{{MeNachHinten}}** cambian el orden de dibujo (el juego dibuja en el orden del archivo). **{{MeSperren}}** impide que un elemento se mueva.",
    "Interact and cvars":
        "Interactuar y cvars",
    "**{{MeBedienen}}** runs the menu like the game: hovering gives focus (`mouseEnter`, `onFocus`), a click runs the item's `action`, yes/no, multi choice and sliders change their cvar, and `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... work like in ui_shared. What needs the game (sounds, `exec`, `uiScript`) appears in **{{MeProtokoll}}**.":
        "**{{MeBedienen}}** ejecuta el menú como el juego: pasar el ratón da el foco (`mouseEnter`, `onFocus`), un clic ejecuta la `action` del elemento, sí/no, opción múltiple y los deslizadores cambian su cvar, y `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... funcionan como en ui_shared. Lo que necesita el juego (sonidos, `exec`, `uiScript`) aparece en **{{MeProtokoll}}**.",
    "**{{MeCvars}}** lists every cvar the menu reads or sets, the ones that control visibility first, with the values that occur. Change them to see other states. They exist only in the preview - your game configuration is never touched.":
        "**{{MeCvars}}** lista cada cvar que el menú lee o fija, primero las que controlan la visibilidad, con los valores que aparecen. Cámbialas para ver otros estados. Solo existen en la vista previa: la configuración de tu juego nunca se toca.",
    "Code editor":
        "Editor de código",
    "Interact: the page cvar `ui_tab` shows the red box, a click on Yes/No changes `ui_probe`.":
        "Interactuar: la cvar de página `ui_tab` muestra el cuadro rojo, un clic en Sí/No cambia `ui_probe`.",
    "find and replace, as in the script section (regex, case, whole word, all open menus). Replace All is one undo step and reports the count; Ctrl+H puts the cursor in the replace field; matches are marked in the minimap":
        "buscar y reemplazar, como en la sección de scripts (regex, mayúsculas, palabra completa, todos los menús abiertos). Reemplazar todo es un solo paso de deshacer e indica la cantidad; Ctrl+H pone el cursor en el campo de reemplazo; las coincidencias se marcan en el minimapa",
    "next occurrence / all occurrences / extra cursor (multi-cursor)":
        "siguiente aparición / todas las apariciones / cursor extra (multicursor)",
    "completion: keywords, cvars, item names, shader paths, @MENUS strings":
        "autocompletado: palabras clave, cvars, nombres de elementos, rutas de shaders, cadenas @MENUS",
    "go to the menuDef or itemDef of the name under the cursor":
        "ir al menuDef o itemDef del nombre bajo el cursor",
    "bookmark like in the script editor (also a click in the margin)":
        "marcador como en el editor de scripts (también con un clic en el margen)",
    "comment lines, indent / outdent":
        "comentar líneas, aumentar / reducir sangría",
    "The margin works like in the script editor: orange = changed since saving, green = changed and saved, blue = back to the state when opened. `menuDef`, `itemDef` and script blocks fold with the +/- next to the line numbers. Clicking an item on the canvas jumps to its `itemDef`, and the cursor in an `itemDef` selects the item.":
        "El margen funciona como en el editor de scripts: naranja = cambiado desde que se guardó, verde = cambiado y guardado, azul = de nuevo como al abrir. `menuDef`, `itemDef` y los bloques de script se pliegan con el +/- junto a los números de línea. Pulsar un elemento en el lienzo salta a su `itemDef`, y el cursor en un `itemDef` selecciona el elemento.",
    "Find with a regular expression: matches in the text and in the minimap, an engine message marked red in line 49.":
        "Buscar con una expresión regular: coincidencias en el texto y en el minimapa, un mensaje del motor marcado en rojo en la línea 49.",
    "Save and export":
        "Guardar y exportar",
    "**{{MeSpeichern}}** writes the text exactly as it is in the editor; a file opened from the game is saved elsewhere. **{{MeExportPk3}}** packs the menu under its game path plus your images into a .pk3. Neither ever writes into the game folders.":
        "**{{MeSpeichern}}** escribe el texto exactamente como está en el editor; un archivo abierto del juego se guarda en otro sitio. **{{MeExportPk3}}** empaqueta el menú con su ruta del juego más tus imágenes en un .pk3. Ninguno de los dos escribe nunca en las carpetas del juego.",
    "Drawn like the game: fonts from the .fontdat files, text positions as in RE_Font_DrawString, item types as in ui_shared. Placeholders show what needs the running game: ownerdraw items (with their number), list feeders, 3D models and cinematics.":
        "Dibujado como el juego: fuentes de los archivos .fontdat, posiciones de texto como en RE_Font_DrawString, tipos de elemento como en ui_shared. Los marcadores muestran lo que necesita el juego en marcha: elementos ownerdraw (con su número), feeders de listas, modelos 3D y cinemáticas.",
    "Missions":
        "Misiones",
    "A mission is a map with its entities and all its scripts. Open one whole, build your own, and pack it into a .pk3 that the game loads.":
        "Una misión es un mapa con sus entidades y todos sus scripts. Abre una entera, crea la tuya y empaquétala en un .pk3 que carga el juego.",
    "Opening a mission":
        "Abrir una misión",
    "**{{LoadMission}}** (menu {{MenuFile}} or the map toolbar) picks a `.pk3` and lists **all missions of that game** - the maps that have scripts, from every archive of the game folders, exactly as the game finds them (for a name, the archive that wins counts). It does not matter which archive of the game you pick: `assets1.pk3` of plain Jedi Academy lists `t1_sour`, `kor1`, `yavin1` … just like `assets0.pk3`. The missions of the chosen archive stand at the top; the others follow once they have been read (the maps are read in the background - the line above the list shows how far). The filter field narrows the list by name. Pick one and everything is set up: the map, its `.ent` with the mission's figures, and **every script of the mission in its own tab**. All tabs share the map and the camera.":
        "**{{LoadMission}}** (menú {{MenuFile}} o la barra del mapa) elige un `.pk3` y lista **todas las misiones de ese juego**: los mapas que tienen scripts, de cada archivo de las carpetas del juego, exactamente como los encuentra el juego (para un nombre cuenta el archivo que gana). No importa qué archivo del juego elijas: `assets1.pk3` del Jedi Academy normal lista `t1_sour`, `kor1`, `yavin1` … igual que `assets0.pk3`. Las misiones del archivo elegido están arriba; las demás siguen una vez leídas (los mapas se leen en segundo plano; la línea encima de la lista muestra cuánto falta). El campo de filtro reduce la lista por nombre. Elige una y todo queda preparado: el mapa, su `.ent` con las figuras de la misión, y **cada script de la misión en su propia pestaña**. Todas las pestañas comparten el mapa y la cámara.",
    "The missions of the game, here narrowed with the filter field - those of the chosen archive first, the other archives of the game below.":
        "Las misiones del juego, aquí reducidas con el campo de filtro: primero las del archivo elegido, debajo los demás archivos del juego.",
    "The tabs open **in the order in which the game starts the scripts**. For this, JK Studio plays the map from the moment it loads, like the engine: the spawn scripts of the entities, the spawn script of `worldspawn`, `trigger_always`, NPC spawners without a `targetname`, the targets of the player start when the player enters the game - and everything that follows from them through `run`, `use`, relays, counters, spawners and `kill` with a death script. The tab of the first script is in front. Scripts that only the player sets off (a trigger he walks into, a fight) come last, in the order of the entities.":
        "Las pestañas se abren **en el orden en que el juego inicia los scripts**. Para ello, JK Studio reproduce el mapa desde el momento en que se carga, como el motor: los scripts de aparición de las entidades, el script de aparición de `worldspawn`, `trigger_always`, los NPC spawners sin `targetname`, los destinos del inicio del jugador cuando el jugador entra en el juego, y todo lo que se sigue de ellos mediante `run`, `use`, relés, contadores, spawners y `kill` con un script de muerte. La pestaña del primer script queda delante. Los scripts que solo dispara el jugador (un trigger en el que entra, un combate) van al final, en el orden de las entidades.",
    "Which game folders apply is decided by the archive: if its folder is one of your game folders ({{MenuEdit}} → {{PathsTitle}}), those are used. If it belongs to another installation - its `base`, or a mod folder next to a `base` - JK Studio uses that installation's folders **for this session only**, in the engine's order (`base`, then the mod), and does not mix in your configured folders. A `.pk3` in any other folder is added behind your game folders, also only for this session. Nothing is saved: the game folders window shows the folders of the session with a button back to your own ({{PathsSessionBack}}). Each mission tab keeps the folders of its game: with a Jedi Outcast mission in one tab and a Jedi Academy mission in another, switching tabs switches the game folders and the rule set with them.":
        "Qué carpetas del juego valen lo decide el archivo: si su carpeta es una de tus carpetas del juego ({{MenuEdit}} → {{PathsTitle}}), se usan esas. Si pertenece a otra instalación (su `base`, o una carpeta de mod junto a un `base`), JK Studio usa las carpetas de esa instalación **solo para esta sesión**, en el orden del motor (`base` y luego el mod), y no mezcla tus carpetas configuradas. Un `.pk3` en cualquier otra carpeta se añade detrás de tus carpetas del juego, también solo para esta sesión. No se guarda nada: la ventana de carpetas del juego muestra las carpetas de la sesión con un botón para volver a las tuyas ({{PathsSessionBack}}). Cada pestaña de misión conserva las carpetas de su juego: con una misión de Jedi Outcast en una pestaña y una de Jedi Academy en otra, cambiar de pestaña cambia con ellas las carpetas del juego y las reglas.",
    "Recent missions":
        "Misiones recientes",
    "**{{MenuFile}} → {{RecentMissions}}** and the start page list the missions you opened last - from an archive or from a mission folder. One click opens the whole project again: map, entities and all scripts. An entry whose file is gone is removed from the list.":
        "**{{MenuFile}} → {{RecentMissions}}** y la página de inicio listan las misiones que abriste por última vez, desde un archivo o desde una carpeta de misión. Un clic vuelve a abrir el proyecto entero: mapa, entidades y todos los scripts. Una entrada cuyo archivo ya no existe se quita de la lista.",
    "A new mission":
        "Una misión nueva",
    "**{{MissionNew}}** starts a mission on any map. Choose the map, give the mission a name (letters, digits and `_`; the game loads it as `maps/<name>.bsp`, so `devmap <name>`), and a script folder. JK Studio creates a player start, a script runner, the intro script and a mission folder under `Documents\\behaved\\missions`. The intro runs when the player spawns.":
        "**{{MissionNew}}** empieza una misión en cualquier mapa. Elige el mapa, dale un nombre a la misión (letras, cifras y `_`; el juego la carga como `maps/<nombre>.bsp`, o sea `devmap <nombre>`) y una carpeta de scripts. JK Studio crea un inicio de jugador, un ejecutor de scripts, el script de introducción y una carpeta de misión en `Documentos\\behaved\\missions`. La introducción se ejecuta cuando aparece el jugador.",
    "The {{MissionNewTitle}} dialog.":
        "El diálogo {{MissionNewTitle}}.",
    "A mission that was never packed lives in its folder; **{{MissionOpenFolder}}** continues it after a restart.":
        "Una misión que nunca se empaquetó vive en su carpeta; **{{MissionOpenFolder}}** la continúa tras reiniciar.",
    "Saving a mission":
        "Guardar una misión",
    "**{{MissionSave}}** shows what will go into the archive - every file with its size - and checks the mission first: a script that an entity uses but that does not exist, a script that does not compile, no player start, a target that points nowhere, a `.str` without `ENDMARKER`. Errors (red) must be fixed, warnings (orange) may stay. **{{MissionSaveNow}}** saves all scripts, writes the `.ent`, compiles the scripts, adds your own ROFF files and packs the `.pk3` into the game folder.":
        "**{{MissionSave}}** muestra lo que irá en el archivo (cada archivo con su tamaño) y comprueba antes la misión: un script que usa una entidad pero no existe, un script que no compila, falta de inicio del jugador, un target que no apunta a nada, un `.str` sin `ENDMARKER`. Los errores (rojo) hay que corregirlos, los avisos (naranja) pueden quedarse. **{{MissionSaveNow}}** guarda todos los scripts, escribe el `.ent`, compila los scripts, añade tus propios archivos ROFF y empaqueta el `.pk3` en la carpeta del juego.",
    "How a mission starts in the game":
        "Cómo empieza una misión en el juego",
    "The player spawns":
        "Aparece el jugador",
    "info_player_start":
        "info_player_start",
    "target_scriptrunner":
        "target_scriptrunner",
    "The intro script":
        "El script de introducción",
    "affect blocks":
        "bloques affect",
    "its key target fires the runner":
        "su clave target dispara el ejecutor",
    "its key usescript names the script":
        "su clave usescript nombra el script",
    "camera ENABLE ... DISABLE":
        "camera ENABLE ... DISABLE",
    "figures walk, look, speak":
        "las figuras caminan, miran, hablan",
    "their spawnscripts run when they appear":
        "sus spawnscripts se ejecutan cuando aparecen",
    "A new mission sets this up for you - the Graph view shows the same chain.":
        "Una misión nueva te prepara esto; la vista Grafo muestra la misma cadena.",
    "The {{MissionSaveTitle}} dialog with its checks.":
        "El diálogo {{MissionSaveTitle}} con sus comprobaciones.",
    "The archive goes into the game folder that wins - the bottom one under **{{MenuEdit}} → {{PathsTitle}}** - as `zz_<name>.pk3`, so it wins over older files. Start the game and type `devmap <name>` in the console to play it.":
        "El archivo va a la carpeta del juego que gana (la de abajo en **{{MenuEdit}} → {{PathsTitle}}**) como `zz_<nombre>.pk3`, para que gane a los archivos más antiguos. Inicia el juego y escribe `devmap <nombre>` en la consola para jugarla.",
    "Model view and characters":
        "Vista de modelos y personajes",
    "Look at any character of the game with its skins and animations, and put an animation into the script with a double-click.":
        "Mira cualquier personaje del juego con sus skins y animaciones, y pon una animación en el script con un doble clic.",
    "Model mode: the animation list on the left, the model in the middle, the playback bar below.":
        "Modo Modelo: la lista de animaciones a la izquierda, el modelo en el centro, la barra de reproducción debajo.",
    "Switch to **{{ViewModel}}** in the mode bar. **{{NpcBrowser}}** lists every character of the game (from the `.npc` files) - click one to load its model and skin as the game shows it. **{{OpenGlm}}** and **{{OpenGlmPk3}}** open any model. Left mouse orbits, middle mouse pans, the wheel zooms; **{{ModelTurntable}}**, **{{ModelBones}}** and **{{ModelResetView}}** sit in the toolbar.":
        "Cambia a **{{ViewModel}}** en la barra de modos. **{{NpcBrowser}}** lista todos los personajes del juego (de los archivos `.npc`): pulsa uno para cargar su modelo y su skin como los muestra el juego. **{{OpenGlm}}** y **{{OpenGlmPk3}}** abren cualquier modelo. El botón izquierdo orbita, el central desplaza, la rueda amplía; **{{ModelTurntable}}**, **{{ModelBones}}** y **{{ModelResetView}}** están en la barra.",
    "The **{{ModelAnimations}}** list on the left shows every animation of the skeleton with its length; filter by name or by BOTH / TORSO / LEGS / FACE. Click plays it; double-click (or **{{ModelAnimToScript}}**) puts `set ( SET_ANIM_BOTH, \"…\" )` after the selected line of the open script - `SET_ANIM_UPPER` for TORSO and `SET_ANIM_LOWER` for LEGS animations.":
        "La lista **{{ModelAnimations}}** de la izquierda muestra cada animación del esqueleto con su duración; filtra por nombre o por BOTH / TORSO / LEGS / FACE. Clic la reproduce; doble clic (o **{{ModelAnimToScript}}**) pone `set ( SET_ANIM_BOTH, \"…\" )` tras la línea seleccionada del script abierto; `SET_ANIM_UPPER` para animaciones TORSO y `SET_ANIM_LOWER` para animaciones LEGS.",
    "Your own mission: take-off on Taspir":
        "Tu propia misión: despegue en Taspir",
    "A complete little mission for Jedi Academy, built from nothing: Kyle and Rosh walk across a landing pad, the camera follows them, the Raven's Claw lifts off and flies past the camera, the picture shakes and fades to black - and the game plays it.":
        "Una pequeña misión completa para Jedi Academy, construida desde cero: Kyle y Rosh cruzan una plataforma de aterrizaje, la cámara los sigue, la Garra del Cuervo despega y pasa junto a la cámara, la imagen tiembla y se funde a negro, y el juego lo reproduce.",
    "Eight parts · base game · about 30 minutes":
        "Ocho partes · juego base · unos 30 minutos",
    "The finished cutscene, seen through its camera.":
        "La cinemática terminada, vista a través de su cámara.",
    "This tutorial uses only the base game: the map `taspir1` - the landing pad at the start of the Taspir III mission - with Kyle's ship, the Raven's Claw, and the characters Kyle and Rosh. You need nothing but Jedi Academy and JK Studio. Every picture on the following pages was taken while exactly this mission was built, step by step.":
        "Este tutorial usa solo el juego base: el mapa `taspir1` (la plataforma de aterrizaje al principio de la misión Taspir III) con la nave de Kyle, la Garra del Cuervo, y los personajes Kyle y Rosh. No necesitas nada más que Jedi Academy y JK Studio. Cada imagen de las páginas siguientes se tomó mientras se construía exactamente esta misión, paso a paso.",
    "The mission in eight parts":
        "La misión en ocho partes",
    "Create the mission":
        "Crear la misión",
    "Place the characters":
        "Colocar los personajes",
    "Let them walk":
        "Hacerlos caminar",
    "The camera":
        "La cámara",
    "The ship takes off":
        "La nave despega",
    "Cut, shake, fade":
        "Corte, temblor, fundido",
    "A second script":
        "Un segundo script",
    "Save and play":
        "Guardar y jugar",
    "player start + script runner":
        "inicio del jugador + ejecutor de scripts",
    "NPC spawners, walk targets":
        "NPC spawners, destinos",
    "affect, task, dowait":
        "affect, task, dowait",
    "shots, travel time, pilot":
        "planos, tiempo de viaje, piloto",
    "a ROFF file":
        "un archivo ROFF",
    "SHAKE, PAN, FADE":
        "SHAKE, PAN, FADE",
    "spawnscript, macro":
        "spawnscript, macro",
    "zz_taspir_tut.pk3":
        "zz_taspir_tut.pk3",
    "devmap taspir_tut":
        "devmap taspir_tut",
    "Before you start":
        "Antes de empezar",
    "Add the game folder once: **{{MenuEdit}} → {{PathsTitle}}** → **{{Pk3AddPath}}** → the `base` folder of Jedi Academy (the one with `assets0.pk3`). See [[erste-schritte#spielordner]].":
        "Añade una vez la carpeta del juego: **{{MenuEdit}} → {{PathsTitle}}** → **{{Pk3AddPath}}** → la carpeta `base` de Jedi Academy (la que tiene `assets0.pk3`). Mira [[erste-schritte#spielordner]].",
    "If the window is new to you, read [[fenster]] first - five minutes.":
        "Si la ventana es nueva para ti, lee antes [[fenster]]: cinco minutos.",
    "Everything can be undone with <kbd>Ctrl</kbd>+<kbd>Z</kbd>. The game files are never changed: your mission goes into its own folder and its own `.pk3`.":
        "Todo se puede deshacer con <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Los archivos del juego nunca cambian: tu misión va a su propia carpeta y a su propio `.pk3`.",
    "1 · Create the mission":
        "1 · Crear la misión",
    "A new mission on taspir1: map, player start, script runner and an intro script are made for you.":
        "Una misión nueva en taspir1: el mapa, el inicio del jugador, el ejecutor de scripts y un script de introducción se crean por ti.",
    "**Open the start page.** Switch the mode bar to **{{ViewMap}}**. Without a map, the tab shows the start page.":
        "**Abre la página de inicio.** Cambia la barra de modos a **{{ViewMap}}**. Sin mapa, la pestaña muestra la página de inicio.",
    "The start page of the Mission tab.":
        "La página de inicio de la pestaña Misión.",
    "**New mission.** Click **{{StartCardNew}}**. In the list type `taspir1` and pick it. As **{{MissionName}}** type `taspir_tut` - letters, digits and `_`. The **{{MissionFolder}}** is filled in by itself. Press **{{MissionCreate}}**.":
        "**Misión nueva.** Pulsa **{{StartCardNew}}**. En la lista escribe `taspir1` y elígelo. Como **{{MissionName}}** escribe `taspir_tut` (letras, cifras y `_`). La **{{MissionFolder}}** se rellena sola. Pulsa **{{MissionCreate}}**.",
    "The New mission window.":
        "La ventana Nueva misión.",
    "Map list":
        "Lista de mapas",
    "Type to search, click the map the mission is built on.":
        "Escribe para buscar y pulsa el mapa sobre el que se construye la misión.",
    "Letters, digits and `_`. The game loads it with `devmap <name>`.":
        "Letras, cifras y `_`. El juego la carga con `devmap <nombre>`.",
    "The folder under `scripts/` for the mission's scripts.":
        "La carpeta bajo `scripts/` para los scripts de la misión.",
    "Creates the mission - or closes the window.":
        "Crea la misión, o cierra la ventana.",
    "**Look at what you got.** The map opens on the landing pad. behaved has added a `target_scriptrunner` next to the player start: when the player spawns, the script `taspir_tut/intro` runs. It is open in a tab - a short camera shot behind the player start.":
        "**Mira lo que tienes.** El mapa se abre en la plataforma de aterrizaje. behaved ha añadido un `target_scriptrunner` junto al inicio del jugador: cuando aparece el jugador se ejecuta el script `taspir_tut/intro`. Está abierto en una pestaña: un plano de cámara corto detrás del inicio del jugador.",
    "The new mission: the player start is selected, the intro script is open on the right.":
        "La misión nueva: el inicio del jugador está seleccionado, el script de introducción abierto a la derecha.",
    "**Switch off the original intro.** taspir1 has its own start: its player start already triggers a runner with `taspir1/preIntro`, and the same trigger now starts yours too - both would play at once. Type `t271` into the search box of the entity list - that is the runner of the original (its `usescript` is `taspir1/preIntro`) - select it and press <kbd>Delete</kbd>. The **{{FlowGraph}}** view shows such connections at a glance ([[skriptfluss#graph]]).":
        "**Desactiva la introducción original.** taspir1 tiene su propio inicio: su inicio del jugador ya dispara un ejecutor con `taspir1/preIntro`, y el mismo disparador inicia ahora también el tuyo; los dos se reproducirían a la vez. Escribe `t271` en el campo de búsqueda de la lista de entidades (es el ejecutor del original, su `usescript` es `taspir1/preIntro`), selecciónalo y pulsa <kbd>Supr</kbd>. La vista **{{FlowGraph}}** muestra esas conexiones de un vistazo ([[skriptfluss#graph]]).",
    "Every map of the game works this way. Its own entities stay; remove what you do not want - it changes only your mission, never the game.":
        "Cada mapa del juego funciona así. Sus propias entidades se quedan; quita lo que no quieras: solo cambia tu misión, nunca el juego.",
    "Part %d of 8 · Taspir mission":
        "Parte %d de 8 · Misión de Taspir",
    "2 · Place the characters":
        "2 · Colocar los personajes",
    "Kyle and Rosh at the south edge of the pad, and a walk target in front of the ship for each of them.":
        "Kyle y Rosh en el borde sur de la plataforma, y un destino delante de la nave para cada uno.",
    "**Look at the pad.** Hold the right mouse button and fly with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> until you see the pad from the south, a little from above.":
        "**Mira la plataforma.** Mantén el botón derecho y vuela con <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> hasta que veas la plataforma desde el sur, un poco desde arriba.",
    "Placing the two characters and their walk targets.":
        "Colocando los dos personajes y sus destinos.",
    "**Place Kyle.** Click the tile **{{EntKindNpc}}** on the left edge of the view, then click near the south edge of the pad. A character marker appears and is selected.":
        "**Coloca a Kyle.** Pulsa la ficha **{{EntKindNpc}}** en el borde izquierdo de la vista y luego pulsa cerca del borde sur de la plataforma. Aparece un marcador de personaje y queda seleccionado.",
    "**Name him.** In the inspector under the entity list set **{{EntName}}** to `kyle` and `NPC_type` to `Kyle` (the **...** button lists every character of the game). **{{EntDropFloor}}** puts him exactly on the ground.":
        "**Ponle nombre.** En el inspector bajo la lista de entidades pon **{{EntName}}** a `kyle` y `NPC_type` a `Kyle` (el botón **...** lista todos los personajes del juego). **{{EntDropFloor}}** lo pone exactamente en el suelo.",
    "The inspector with Kyle selected.":
        "El inspector con Kyle seleccionado.",
    "**Place Rosh** the same way beside him: **{{EntName}}** `rosh`, `NPC_type` `rosh_penin`.":
        "**Coloca a Rosh** igual a su lado: **{{EntName}}** `rosh`, `NPC_type` `rosh_penin`.",
    "**Two walk targets.** Click the tile **{{EntKindNav}}** and click in front of the ship - name it `nav_kyle`. A second one beside it: `nav_rosh`.":
        "**Dos destinos.** Pulsa la ficha **{{EntKindNav}}** y pulsa delante de la nave; llámalo `nav_kyle`. Un segundo a su lado: `nav_rosh`.",
    "The new entities in the list and on the map.":
        "Las entidades nuevas en la lista y en el mapa.",
    "Hold <kbd>Shift</kbd> while clicking to place several entities of one kind. More in [[entities#setzen]].":
        "Mantén <kbd>Mayús</kbd> al pulsar para colocar varias entidades de un tipo. Más en [[entities#setzen]].",
    "3 · Let them walk":
        "3 · Hacerlos caminar",
    "An affect block for each character: walk to the target, wait until there, then look at the other one.":
        "Un bloque affect para cada personaje: caminar hasta el destino, esperar a llegar y luego mirar al otro.",
    "**Write the walk for Kyle.** In the intro script, after the camera lines, insert `affect` (double-click it in the **{{GroupEvents}}** list) and give it the name `kyle`. Inside it insert `task` named `walk`, and inside the task `set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, nav_kyle )`. After the task: `dowait ( walk )` and `set ( SET_LOOK_TARGET, rosh )`.":
        "**Escribe la marcha de Kyle.** En el script de introducción, tras las líneas de cámara, inserta `affect` (doble clic en la lista **{{GroupEvents}}**) y dale el nombre `kyle`. Dentro inserta `task` llamada `walk`, y dentro de la tarea `set ( SET_WALKING, true )` y `set ( SET_NAVGOAL, nav_kyle )`. Tras la tarea: `dowait ( walk )` y `set ( SET_LOOK_TARGET, rosh )`.",
    "The intro script.":
        "El script de introducción.",
    "**The same for Rosh** - copy the block (**{{ActCopy}}**, **{{ActPaste}}**) and change the names: `rosh`, `nav_rosh`, look target `kyle`.":
        "**Lo mismo para Rosh**: copia el bloque (**{{ActCopy}}**, **{{ActPaste}}**) y cambia los nombres: `rosh`, `nav_rosh`, objetivo de mirada `kyle`.",
    "**Watch them walk.** Drag the playhead in the timeline. Both walk at the same time - an `affect` block does not hold up the script. Click Kyle to see his path: grey ahead, yellow behind.":
        "**Míralos caminar.** Arrastra el cabezal en la línea de tiempo. Los dos caminan a la vez: un bloque `affect` no detiene el script. Pulsa a Kyle para ver su trayecto: gris por delante, amarillo por detrás.",
    "Kyle in the middle of his walk.":
        "Kyle en mitad de su marcha.",
    "**Walk or run.** Right-click the walk target at the end of his path: **{{BahnGehen}}**, **{{BahnRennen}}**, **{{BahnRueckwaerts}}** or **{{BahnAnsehen}}** change how he gets there.":
        "**Caminar o correr.** Clic derecho en el destino al final de su trayecto: **{{BahnGehen}}**, **{{BahnRennen}}**, **{{BahnRueckwaerts}}** o **{{BahnAnsehen}}** cambian cómo llega.",
    "**A detour.** Grab Kyle in the middle of his walk and drag him aside: behaved inserts a new walk target there, and his path bends around it.":
        "**Un rodeo.** Agarra a Kyle en mitad de su marcha y arrástralo a un lado: behaved inserta ahí un destino nuevo y su trayecto se curva a su alrededor.",
    "A path point inserted in the middle of the walk.":
        "Un punto de trayecto insertado en mitad de la marcha.",
    "Why the task? `SET_NAVGOAL` only starts the walk. `dowait` waits until the task is done - until he has arrived - so he looks at Rosh only when he stands. More in [[k-tasks]].":
        "¿Por qué la tarea? `SET_NAVGOAL` solo inicia la marcha. `dowait` espera a que termine la tarea, hasta que ha llegado, así solo mira a Rosh cuando está parado. Más en [[k-tasks]].",
    "4 · The camera":
        "4 · La cámara",
    "A wide shot of the pad, a move down behind the two, set from the view and flown by hand.":
        "Un plano general de la plataforma, un movimiento hacia abajo detrás de los dos, fijado desde la vista y volado a mano.",
    "**The first shot** is already there - the template behind the player start. Select it in the **{{CamShots}}** list, fly out to a wide view of the pad from the south-east and press **{{CamSetToView}}**: the shot is now where you are looking.":
        "**El primer plano** ya está: la plantilla detrás del inicio del jugador. Selecciónalo en la lista **{{CamShots}}**, vuela hacia fuera hasta una vista general de la plataforma desde el sureste y pulsa **{{CamSetToView}}**: el plano está ahora donde miras.",
    "**A second shot.** Fly down behind Kyle and Rosh, low, looking at the ship. Press **{{CamNewShot}}** and set its **{{CamDuration}}** to `4` seconds: the camera flies from the wide shot to this one while the two walk.":
        "**Un segundo plano.** Vuela hacia abajo detrás de Kyle y Rosh, bajo, mirando a la nave. Pulsa **{{CamNewShot}}** y pon su **{{CamDuration}}** a `4` segundos: la cámara vuela del plano general a este mientras los dos caminan.",
    "The camera with its shots and its path.":
        "La cámara con sus planos y su trayecto.",
    "**Fine-tune in the view.** The camera bar has **{{GizmoMove}}** (arrows) and **{{GizmoRotate}}** (rings). Releasing writes the change into the script.":
        "**Ajusta en la vista.** La barra de la cámara tiene **{{GizmoMove}}** (flechas) y **{{GizmoRotate}}** (anillos). Al soltar se escribe el cambio en el script.",
    "Moving a shot with the arrows.":
        "Moviendo un plano con las flechas.",
    "**Or fly it.** **{{CamPilot}}** puts you into the shot: right mouse looks, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> move. <kbd>Enter</kbd> sets a key at the playhead, **{{CamPilotApply}}** keeps it, <kbd>Esc</kbd> throws it away.":
        "**O vuélalo.** **{{CamPilot}}** te pone en el plano: el botón derecho mira, <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> mueven. <kbd>Intro</kbd> pone una clave en el cabezal, **{{CamPilotApply}}** la conserva, <kbd>Esc</kbd> la descarta.",
    "**Look through it.** **{{CamPreview}}** shows a small picture of what the camera sees; click it to look through the camera.":
        "**Mira a través de él.** **{{CamPreview}}** muestra una pequeña imagen de lo que ve la cámara; púlsala para mirar a través de la cámara.",
    "The camera preview in the corner of the view.":
        "La vista previa de cámara en la esquina de la vista.",
    "5 · The ship takes off":
        "5 · La nave despega",
    "Animate the Raven's Claw with four keys and save the flight as a ROFF file.":
        "Anima la Garra del Cuervo con cuatro claves y guarda el vuelo como archivo ROFF.",
    "**Select the ship.** The Raven's Claw on the pad is a `misc_model_breakable` named `ravensClaw`. Click it on the map, then **{{EntAnimate}}** in the inspector.":
        "**Selecciona la nave.** La Garra del Cuervo de la plataforma es un `misc_model_breakable` llamado `ravensClaw`. Púlsala en el mapa y luego **{{EntAnimate}}** en el inspector.",
    "The ship selected, the Animation window open.":
        "La nave seleccionada, la ventana de animación abierta.",
    "**ROFF file.** In the **{{AnimTitle}}** window choose **{{AnimModeRoff}}**. The script gets a block that plays the file, and the first key is where the ship stands now.":
        "**Archivo ROFF.** En la ventana **{{AnimTitle}}** elige **{{AnimModeRoff}}**. El script recibe un bloque que reproduce el archivo, y la primera clave es donde está ahora la nave.",
    "The Animation window in ROFF mode.":
        "La ventana de animación en modo ROFF.",
    "How the movement is stored: as `move` commands in the script, or as a ROFF file the script plays.":
        "Cómo se guarda el movimiento: como comandos `move` en el script, o como archivo ROFF que reproduce el script.",
    "The name of the ROFF, as in `play ( PLAY_ROFF, ... )`; saved as `scripts/<name>.rof` next to the script.":
        "El nombre del ROFF, como en `play ( PLAY_ROFF, ... )`; se guarda como `scripts/<nombre>.rof` junto al script.",
    "On: one curve through all keys, easing in and out. Off: straight from key to key at constant speed.":
        "Activado: una curva por todas las claves, que arranca y frena suavemente. Desactivado: en línea recta de clave a clave a velocidad constante.",
    "Adds a key at the playhead, with the entity where it is now.":
        "Añade una clave en el cabezal, con la entidad donde está ahora.",
    "Removes the selected key / moves the timeline to it.":
        "Quita la clave seleccionada / mueve la línea de tiempo hasta ella.",
    "Removes the animation block from the script.":
        "Quita el bloque de animación del script.",
    "Key table":
        "Tabla de claves",
    "Time, position and direction of every key. Type new values, or click a row to jump there.":
        "Tiempo, posición y dirección de cada clave. Escribe valores nuevos, o pulsa una fila para saltar allí.",
    "**Set the keys.** At 5 s press **{{AnimAddKey}}** - the ship still stands. At 6.5 s drag it straight up a little: a key is set at the playhead. At 8.5 s drag it forward and higher, and tip the nose up with the ring. At 10.5 s it is far away and high.":
        "**Pon las claves.** En 5 s pulsa **{{AnimAddKey}}**: la nave sigue quieta. En 6,5 s arrástrala un poco hacia arriba en vertical: se pone una clave en el cabezal. En 8,5 s arrástrala hacia delante y más arriba, e inclina el morro hacia arriba con el anillo. En 10,5 s está lejos y alta.",
    "The keys of the flight, with the rotate ring.":
        "Las claves del vuelo, con el anillo de rotación.",
    "**Watch.** Play the timeline: the ship lifts off and flies away. **{{AnimRoffSmooth}}** makes one curve through the keys.":
        "**Mira.** Reproduce la línea de tiempo: la nave despega y se aleja volando. **{{AnimRoffSmooth}}** traza una curva por las claves.",
    "The Raven's Claw taking off.":
        "La Garra del Cuervo despegando.",
    "**Saved with the script.** Saving writes `scripts/taspir_tut/roffs/ravensClaw.rof` next to the script; **{{MissionSave}}** packs it into the `.pk3`.":
        "**Se guarda con el script.** Al guardar se escribe `scripts/taspir_tut/roffs/ravensClaw.rof` junto al script; **{{MissionSave}}** lo empaqueta en el `.pk3`.",
    "Animating with move commands instead of a file: [[k-tuer]]. Changing a ROFF of the game: [[k-roff-bearbeiten]].":
        "Animar con comandos move en vez de un archivo: [[k-tuer]]. Cambiar un ROFF del juego: [[k-roff-bearbeiten]].",
    "6 · Cut, shake, follow, fade":
        "6 · Corte, temblor, seguimiento, fundido",
    "A third shot from outside the pad, a shake at lift-off, the camera turning after the ship, a fade to black - and the timing fixed in the timeline.":
        "Un tercer plano desde fuera de la plataforma, un temblor al despegar, la cámara girando tras la nave, un fundido a negro, y los tiempos ajustados en la línea de tiempo.",
    "**Cut.** After the second camera move insert a `wait` of 5 seconds, then a new shot from outside the pad, looking at the ship, with **{{CamDuration}}** `0` - a cut.":
        "**Corte.** Tras el segundo movimiento de cámara inserta un `wait` de 5 segundos y luego un plano nuevo desde fuera de la plataforma, mirando a la nave, con **{{CamDuration}}** `0`: un corte.",
    "**Shake.** Insert `camera` and choose `SHAKE`: strength `4`, `2000` milliseconds - the engines start.":
        "**Temblor.** Inserta `camera` y elige `SHAKE`: intensidad `4`, `2000` milisegundos; arrancan los motores.",
    "**Follow the ship.** Two `camera ( PAN )` commands with a duration turn the camera up after the ship while it climbs; a `wait` after each lets them finish.":
        "**Sigue a la nave.** Dos comandos `camera ( PAN )` con duración giran la cámara hacia arriba tras la nave mientras sube; un `wait` tras cada uno les deja terminar.",
    "**Fade out.** `camera` with `FADE`: from `0 0 0` and `0` to `0 0 0` and `1` in `1500` ms - black. Then `camera ( DISABLE )` gives control back to the player.":
        "**Fundido de salida.** `camera` con `FADE`: de `0 0 0` y `0` a `0 0 0` y `1` en `1500` ms: negro. Luego `camera ( DISABLE )` devuelve el control al jugador.",
    "**Fix the timing.** Play and watch. Drag blocks in the timeline: everything after moves along. Drag a right edge to change a duration.":
        "**Ajusta los tiempos.** Reproduce y mira. Arrastra bloques en la línea de tiempo: todo lo que va detrás se mueve también. Arrastra un borde derecho para cambiar una duración.",
    "**The whole scene.** Turn on **{{MapThroughCam}}** and play from the start.":
        "**La escena entera.** Activa **{{MapThroughCam}}** y reproduce desde el principio.",
    "The finished cutscene through the camera.":
        "La cinemática terminada a través de la cámara.",
    "7 · A second script":
        "7 · Un segundo script",
    "Kyle gets a spawn script with a macro - and the split view shows both scripts side by side.":
        "Kyle recibe un script de aparición con una macro, y la vista dividida muestra los dos scripts uno al lado del otro.",
    "**A spawnscript key.** Select Kyle, add the key `spawnscript` with the value `taspir_tut/kyle_spawn` (**{{EntAddKey}}** in the inspector).":
        "**Una clave spawnscript.** Selecciona a Kyle y añade la clave `spawnscript` con el valor `taspir_tut/kyle_spawn` (**{{EntAddKey}}** en el inspector).",
    "**Open creates it.** Press **{{EntOpenScript}}** next to the key: the script does not exist yet, so behaved creates it in the mission folder and opens it in a tab.":
        "**Abrir lo crea.** Pulsa **{{EntOpenScript}}** junto a la clave: el script aún no existe, así que behaved lo crea en la carpeta de la misión y lo abre en una pestaña.",
    "**A macro.** Double-click `standOnly` at the end of the **{{GroupEvents}}** list: three `set` commands that make Kyle stand and ignore enemies. Add `set ( SET_WEAPON, WP_SABER )`.":
        "**Una macro.** Haz doble clic en `standOnly` al final de la lista **{{GroupEvents}}**: tres comandos `set` que hacen que Kyle se quede quieto e ignore a los enemigos. Añade `set ( SET_WEAPON, WP_SABER )`.",
    "The spawn script with the standOnly macro.":
        "El script de aparición con la macro standOnly.",
    "**Side by side.** **{{MenuFile}} → {{SplitToggle}} → 2** shows the intro beside the spawn script. Drag lines from one to the other to copy them.":
        "**Uno al lado del otro.** **{{MenuFile}} → {{SplitToggle}} → 2** muestra la introducción junto al script de aparición. Arrastra líneas de uno a otro para copiarlas.",
    "8 · Save and play":
        "8 · Guardar y jugar",
    "Check the mission, pack it into a .pk3 and start it in the game.":
        "Comprueba la misión, empaquétala en un .pk3 e iníciala en el juego.",
    "**Save mission.** Press **{{MissionSave}}** in the map toolbar. The window lists every file it will pack and checks the mission: errors in red must be fixed, warnings in orange may stay.":
        "**Guarda la misión.** Pulsa **{{MissionSave}}** en la barra del mapa. La ventana lista cada archivo que va a empaquetar y comprueba la misión: los errores en rojo hay que corregirlos, los avisos en naranja pueden quedarse.",
    "**Pack.** Press **{{MissionSaveNow}}**: scripts are saved and compiled, the `.ent` with your entities and the ROFF are written, and `zz_taspir_tut.pk3` goes into the game folder.":
        "**Empaqueta.** Pulsa **{{MissionSaveNow}}**: los scripts se guardan y compilan, se escriben el `.ent` con tus entidades y el ROFF, y `zz_taspir_tut.pk3` va a la carpeta del juego.",
    "The message after saving.":
        "El mensaje tras guardar.",
    "**Play.** Start Jedi Academy, open the console (<kbd>Shift</kbd>+<kbd>~</kbd>) and type `devmap taspir_tut`. The cutscene runs as soon as you spawn.":
        "**Juega.** Inicia Jedi Academy, abre la consola (<kbd>Mayús</kbd>+<kbd>~</kbd>) y escribe `devmap taspir_tut`. La cinemática empieza en cuanto apareces.",
    "**Next time.** **{{MenuFile}} → {{RecentMissions}}** opens the whole mission again - map, entities and both scripts.":
        "**La próxima vez.** **{{MenuFile}} → {{RecentMissions}}** vuelve a abrir la misión entera: mapa, entidades y los dos scripts.",
    "Recent missions in the File menu.":
        "Misiones recientes en el menú Archivo.",
    "Share the `.pk3` with others: it holds everything the mission needs. Players put it into their `base` folder.":
        "Comparte el `.pk3` con otros: contiene todo lo que necesita la misión. Los jugadores lo ponen en su carpeta `base`.",
    "Start with the Taspir mission - it builds a whole cutscene for Jedi Academy from nothing and touches almost every part of the program. The short tutorials below each show one thing in a few steps.":
        "Empieza con la misión de Taspir: construye desde cero una cinemática entera para Jedi Academy y toca casi todas las partes del programa. Los tutoriales cortos de abajo muestran cada uno una cosa en pocos pasos.",
    "Interactive tutorials in the program":
        "Tutoriales interactivos en el programa",
    "Some tutorials also run inside the program: **{{MenuHelp}} → {{TourMenu}}** (or **{{TourStartGuide}}** on the start page) opens a small window with the list. Pick one and press **{{TourStart}}**: the program darkens the screen except for the control you need, frames it in yellow and moves on by itself as soon as you have done the step - whether you used the button, the menu or the shortcut.":
        "Algunos tutoriales también se ejecutan dentro del programa: **{{MenuHelp}} → {{TourMenu}}** (o **{{TourStartGuide}}** en la página de inicio) abre una ventana pequeña con la lista. Elige uno y pulsa **{{TourStart}}**: el programa oscurece la pantalla salvo el control que necesitas, lo enmarca en amarillo y avanza solo en cuanto has hecho el paso, tanto si usaste el botón, el menú o el atajo.",
    "An interactive tutorial: the tutorial window and the highlighted control.":
        "Un tutorial interactivo: la ventana del tutorial y el control resaltado.",
    "**{{TourBack}}** goes one step back. **{{TourSkip}}** skips a step - where it can, the program prepares the next one (it never changes your script). **{{TourNext}}** moves on after a step that only shows something.":
        "**{{TourBack}}** retrocede un paso. **{{TourSkip}}** salta un paso; donde puede, el programa prepara el siguiente (nunca cambia tu script). **{{TourNext}}** avanza tras un paso que solo muestra algo.",
    "**{{TourShowMe}}** moves a pointer to the right place; nothing is clicked. **{{TourDoIt}}** does the step for you - offered only where that is safe, never for saving (which asks for a file name).":
        "**{{TourShowMe}}** mueve un puntero al sitio correcto; no se pulsa nada. **{{TourDoIt}}** hace el paso por ti; solo se ofrece donde es seguro, nunca para guardar (que pide un nombre de archivo).",
    "**{{TourGuide}}** opens the page with its pictures in your browser. The search box below the list searches this guide, offline.":
        "**{{TourGuide}}** abre la página con sus imágenes en tu navegador. El campo de búsqueda bajo la lista busca en esta guía, sin conexión.",
    "The program remembers how far you got; next time you continue where you stopped.":
        "El programa recuerda hasta dónde llegaste; la próxima vez sigues donde lo dejaste.",
    "The first five are the script tutorials [[k-neues-skript]], [[k-befehl-einfuegen]], [[k-werte-bearbeiten]], [[k-zeilen-ordnen]] and [[k-uebersetzen]], in the {{FlowClassic}} view; then [[tut-karte]] in the map editor , [[k-kamera-ansicht]] in the preview and [[k-modell]]. Or press <kbd>F1</kbd> and ask the helper ([[helfer]]).":
        "Los cinco primeros son los tutoriales de scripts [[k-neues-skript]], [[k-befehl-einfuegen]], [[k-werte-bearbeiten]], [[k-zeilen-ordnen]] y [[k-uebersetzen]], en la vista {{FlowClassic}}; luego [[tut-karte]] en el editor de mapas, [[k-kamera-ansicht]] en la vista previa y [[k-modell]]. O pulsa <kbd>F1</kbd> y pregunta al asistente ([[helfer]]).",
    "Also interactive: figures and paths ([[k-gangart]], [[k-wegpunkte]]), playing and retiming in the timeline ([[k-neu-timen]]), a new mission ([[tut-p1]]), skins ([[k-skins]]), the game folder ([[k-spielordner]]), updates ([[k-update]]), a door with a trigger in the map editor ([[tut-karte-tuer]]) and the menu editor ([[tut-menue]]).":
        "También interactivos: figuras y trayectos ([[k-gangart]], [[k-wegpunkte]]), reproducir y reajustar tiempos en la línea de tiempo ([[k-neu-timen]]), una misión nueva ([[tut-p1]]), skins ([[k-skins]]), la carpeta del juego ([[k-spielordner]]), actualizaciones ([[k-update]]), una puerta con un trigger en el editor de mapas ([[tut-karte-tuer]]) y el editor de menús ([[tut-menue]]).",
    "All tutorials at a glance":
        "Todos los tutoriales de un vistazo",
    "Pick any of them - each one stands on its own. Below the list: the big project, then every short tutorial as a card with a picture.":
        "Elige cualquiera: cada uno se sostiene por sí solo. Bajo la lista: el gran proyecto y luego cada tutorial corto como una tarjeta con imagen.",
    "The big project":
        "El gran proyecto",
    "Your first map":
        "Tu primer mapa",
    "Missions and models":
        "Misiones y modelos",
    "The big one: your own mission":
        "El grande: tu propia misión",
    "All tutorials use the base game - the map `taspir1` and its characters - so the pictures match what you see. Any other map, and any mod such as Movie Duels, works the same way. If the parts of the window are new to you, read [[fenster]] and [[karte#bewegen]] first.":
        "Todos los tutoriales usan el juego base (el mapa `taspir1` y sus personajes), así las imágenes coinciden con lo que ves. Cualquier otro mapa, y cualquier mod como Movie Duels, funciona igual. Si las partes de la ventana son nuevas para ti, lee antes [[fenster]] y [[karte#bewegen]].",
    "Build a room in the map editor, light it, walk through it in the editor and bring it into the game as a .pk3.":
        "Construye una sala en el editor de mapas, ilumínala, recórrela en el editor y llévala al juego como .pk3.",
    "Short tutorials":
        "Tutoriales cortos",
    "Each one shows one small thing in three to six steps - start with any of them.":
        "Cada uno muestra una cosa pequeña en tres a seis pasos: empieza por cualquiera.",
    "Settings and updates":
        "Ajustes y actualizaciones",
    "Where the program keeps its paths and options, how to change keys, and how updates arrive.":
        "Dónde guarda el programa sus rutas y opciones, cómo cambiar teclas y cómo llegan las actualizaciones.",
    "Game folders":
        "Carpetas del juego",
    "**{{MenuEdit}} → {{PathsTitle}}** lists the folders whose `.pk3` archives the program reads, with **{{PrefsUp}}**, **{{PrefsDown}}**, **{{PrefsRemove}}** and **{{PrefsOpenFolder}}** for each. The bottom one wins over everything above it, as `fs_game` overrides `base` in the engine. A folder without archives says so - then the path is wrong.":
        "**{{MenuEdit}} → {{PathsTitle}}** lista las carpetas cuyos archivos `.pk3` lee el programa, con **{{PrefsUp}}**, **{{PrefsDown}}**, **{{PrefsRemove}}** y **{{PrefsOpenFolder}}** para cada una. La de abajo gana a todas las de encima, igual que `fs_game` sustituye a `base` en el motor. Una carpeta sin archivos .pk3 lo dice: entonces la ruta está mal.",
    "The game folders.":
        "Las carpetas del juego.",
    "Jedi Outcast (JK2)":
        "Jedi Outcast (JK2)",
    "JK Studio also works with the original Jedi Outcast. Its compiled scripts (`.ibi`, version 1.57) and its maps have the same format as in Jedi Academy, and ICARUS has the same commands. In **{{AppPrefs}}** → **{{PrefsRegelwerk}}**, **{{PrefsRegelJk2}}** is the third rule set next to OpenJK and Movie Duels; the model for it is the JK2 game code in OpenJK (`codeJK2`).":
        "JK Studio también funciona con el Jedi Outcast original. Sus scripts compilados (`.ibi`, versión 1.57) y sus mapas tienen el mismo formato que en Jedi Academy, e ICARUS tiene los mismos comandos. En **{{AppPrefs}}** → **{{PrefsRegelwerk}}**, **{{PrefsRegelJk2}}** es la tercera regla junto a OpenJK y Movie Duels; su modelo es el código de juego de JK2 en OpenJK (`codeJK2`).",
    "**Command set**: under Jedi Outcast the lists for animations, mission objectives, behaviour states and SET_ fields come from JK2 (`data/jk2`). Of the 1356 scripts in Jedi Outcast's `assets0.pk3`, 1352 compile back byte for byte; the check leaves 326 notes, which are real mistakes in Raven's scripts. An own command file in the preferences always wins.":
        "**Conjunto de comandos**: con Jedi Outcast, las listas de animaciones, objetivos de misión, estados de comportamiento y campos SET_ vienen de JK2 (`data/jk2`). De los 1356 scripts de `assets0.pk3` de Jedi Outcast, 1352 vuelven a compilar byte a byte; la comprobación deja 326 notas, que son errores reales en los scripts de Raven. Un archivo de comandos propio en las preferencias siempre gana.",
    "**Game folders**: Jedi Outcast has its own list in **{{MenuEdit}} → {{PathsTitle}}** (**{{PathsJk2}}**). Both games have files with the same name but different content, so the two lists are never mixed; the list of the active rule set is used. **{{Pk3AddPath}}** puts a Jedi Outcast folder into its list by itself.":
        "**Carpetas del juego**: Jedi Outcast tiene su propia lista en **{{MenuEdit}} → {{PathsTitle}}** (**{{PathsJk2}}**). Los dos juegos tienen archivos con el mismo nombre pero distinto contenido, así que las dos listas nunca se mezclan; se usa la lista de las reglas activas. **{{Pk3AddPath}}** pone por sí solo una carpeta de Jedi Outcast en su lista.",
    "**Automatic**: Jedi Outcast when only Jedi Outcast folders are set up, or when a Jedi Outcast mission is opened with **{{LoadMission}}** (`jk2sp.exe` next to the folder, or JK2 maps such as `kejim_post`). A Movie Duels folder always means Movie Duels; when it is unclear, OpenJK.":
        "**Automático**: Jedi Outcast cuando solo hay carpetas de Jedi Outcast, o cuando se abre una misión de Jedi Outcast con **{{LoadMission}}** (`jk2sp.exe` junto a la carpeta, o mapas de JK2 como `kejim_post`). Una carpeta de Movie Duels siempre significa Movie Duels; cuando no está claro, OpenJK.",
    "**Figures**: NPCs come from `ext_data/NPCs.cfg`, the player is `kyle` (also in scripts), human figures use JK2's skeleton with 72 bones, and JK2's NPC classes pick JK2's figures. Lightsabers need no `.sab` file: the hilt and the blade length come from the NPC class (Desann 48, reborn 32, everyone else 40) and the colour from `saberColor` in NPCs.cfg.":
        "**Figuras**: los NPC vienen de `ext_data/NPCs.cfg`, el jugador es `kyle` (también en los scripts), las figuras humanas usan el esqueleto de JK2 con 72 huesos, y las clases de NPC de JK2 eligen las figuras de JK2. Los sables láser no necesitan archivo `.sab`: la empuñadura y la longitud de la hoja vienen de la clase de NPC (Desann 48, reborn 32, todos los demás 40) y el color de `saberColor` en NPCs.cfg.",
    "**Walking and camera**: NPCs walk to navgoals with JK2's navigator: straight when the way is free, otherwise along the waypoint network from `maps/<map>.nav` (built from the waypoints when the file is missing or does not match the map; then connections blocked at map start are re-checked every 1.5 s like the game), with JK2's speeds - running starts at full speed, walking speeds up step by step, and NPCs slow down shortly before the goal; an NPC that cannot get around another NPC tells it to step aside - also one that is standing without a goal, but never the player - and two NPCs blocking each other keep going; a connection blocked by a locked door, something breakable or another NPC is marked as failed for all NPCs, who then take another route until it is free again, like the game. Weapons come from JK2's NPC table, an NPC Kyle takes the player's weapon (also when a script changes it outside the camera, until Kyle gets a weapon of his own), and the blade flickers like in JK2. The camera shows FOLLOW and shake one frame later, camera ROFFs ignore notes, the swamp water in `yavin_swamp` is not solid, and rain and snow follow JK2. `dowait` on a SET_NAVGOAL that finds no goal waits forever, as in the game.":
        "**Marcha y cámara**: los NPC caminan hasta los navgoals con el navegador de JK2: en línea recta si el camino está libre, si no por la red de waypoints de `maps/<mapa>.nav` (construida a partir de los waypoints si el archivo falta o no coincide con el mapa; entonces las conexiones bloqueadas al empezar el mapa se vuelven a comprobar cada 1,5 s como en el juego), con las velocidades de JK2: correr empieza a toda velocidad, caminar acelera paso a paso, y los NPC frenan poco antes del objetivo; un NPC que no puede rodear a otro NPC le dice que se aparte (también a uno que está quieto sin objetivo, pero nunca al jugador), y dos NPC que se bloquean siguen adelante; una conexión bloqueada por una puerta cerrada con llave, algo rompible u otro NPC se marca como fallida para todos los NPC, que toman entonces otro camino hasta que vuelva a estar libre, como en el juego. Las armas vienen de la tabla de NPC de JK2, un NPC Kyle toma el arma del jugador (también cuando un script la cambia fuera de la cámara, hasta que Kyle recibe un arma propia), y la hoja parpadea como en JK2. La cámara muestra FOLLOW y el temblor un fotograma más tarde, los ROFF de cámara ignoran las notas, el agua del pantano de `yavin_swamp` no es sólida, y la lluvia y la nieve siguen a JK2. `dowait` sobre un SET_NAVGOAL que no encuentra objetivo espera para siempre, como en el juego.",
    "Not yet exactly like Jedi Outcast: an NPC that gets blocked by another one does not push it aside, blocked waypoints and connections are not remembered during the scene, and subtitles are not shown.":
        "Aún no exactamente como en Jedi Outcast: un NPC bloqueado por otro no lo aparta de un empujón, los waypoints y conexiones bloqueados no se recuerdan durante la escena, y no se muestran los subtítulos.",
    "**{{MenuEdit}} → {{AppPrefs}}** (or **{{AppPrefs}}** in the action column, <kbd>Alt</kbd>+<kbd>P</kbd>) holds the options of the original BehavEd and those of the cutscene preview:":
        "**{{MenuEdit}} → {{AppPrefs}}** (o **{{AppPrefs}}** en la columna de acciones, <kbd>Alt</kbd>+<kbd>P</kbd>) contiene las opciones del BehavEd original y las de la vista previa de cinemáticas:",
    "**{{PrefsDirectories}}**: {{PrefsScriptPath}}, {{PrefsCommandFile}} (the `.bhc` that defines every command - **{{PrefsReload}}** reads it again), {{PrefsSourcePath}}.":
        "**{{PrefsDirectories}}**: {{PrefsScriptPath}}, {{PrefsCommandFile}} (el `.bhc` que define cada comando; **{{PrefsReload}}** lo vuelve a leer), {{PrefsSourcePath}}.",
    "**{{PrefsTonPegel}}**: {{PrefsTonEffekte}}, {{PrefsTonStimmen}}, {{PrefsTonMusik}} - like the game's sound settings; **{{PrefsTonSpielwerte}}** restores 0.5 / 1.0 / 0.25. How the preview sounds: [[zeitleiste#ton]].":
        "**{{PrefsTonPegel}}**: {{PrefsTonEffekte}}, {{PrefsTonStimmen}}, {{PrefsTonMusik}}, como los ajustes de sonido del juego; **{{PrefsTonSpielwerte}}** restaura 0,5 / 1,0 / 0,25. Cómo suena la vista previa: [[zeitleiste#ton]].",
    "**{{PrefsGroupPreview}}** → **{{PrefsRegelwerk}}** decides which game the cutscene preview imitates: OpenJK (plain Jedi Academy) or Movie Duels, whose game code differs in places - NPC turn speeds per class and difficulty, how often NPCs re-steer and turn (OpenJK every 100 ms, Movie Duels every 50 ms), the default difficulty, lightsaber clashes, cg_fovAspectAdjust and the game window, ROFF sounds on figures. The third choice is Jedi Outcast ([[einstellungen#jedioutcast]]). Automatic picks Movie Duels when a Movie Duels folder is among the game folders, Jedi Outcast for Jedi Outcast folders, otherwise OpenJK; the status list names the rule set when a map loads. Aspect ratio and cg_fovAspectAdjust themselves are under **{{MapViewSettings}}** → **{{GameScreen}}** ([[kamera#vorschau]]).":
        "**{{PrefsGroupPreview}}** → **{{PrefsRegelwerk}}** decide qué juego imita la vista previa de cinemáticas: OpenJK (Jedi Academy normal) o Movie Duels, cuyo código de juego difiere en algunos puntos: velocidades de giro de los NPC por clase y dificultad, cada cuánto corrigen el rumbo y giran los NPC (OpenJK cada 100 ms, Movie Duels cada 50 ms), la dificultad predeterminada, los choques de sables láser, cg_fovAspectAdjust y la ventana del juego, los sonidos ROFF en figuras. La tercera opción es Jedi Outcast ([[einstellungen#jedioutcast]]). Automático elige Movie Duels si hay una carpeta de Movie Duels entre las carpetas del juego, Jedi Outcast para carpetas de Jedi Outcast, y si no, OpenJK; la lista de estado nombra las reglas cuando se carga un mapa. La relación de aspecto y cg_fovAspectAdjust en sí están en **{{MapViewSettings}}** → **{{GameScreen}}** ([[kamera#vorschau]]).",
    "**{{PrefsDifficulty}}**: {{DiffPadawan}}, {{DiffJedi}}, {{DiffKnight}}, {{DiffMaster}} - the game's g_spskill. The first entry is the game's default (Movie Duels {{DiffKnight}}, OpenJK {{DiffPadawan}}). Like the game, it changes how fast stormtroopers, reborn and some other NPCs turn, how fast tusken raiders walk and run (from {{DiffKnight}} on) and how close two lightsabers must come to clash. Entities the map marks for other difficulties (or `notsingle`) are left out, as the game removes them when the map loads - also an NPC that a script names but whose spawner is removed this way (the script's `affect` on it does nothing). Changing it rebuilds the preview at once.":
        "**{{PrefsDifficulty}}**: {{DiffPadawan}}, {{DiffJedi}}, {{DiffKnight}}, {{DiffMaster}}: el g_spskill del juego. La primera entrada es el valor predeterminado del juego (Movie Duels {{DiffKnight}}, OpenJK {{DiffPadawan}}). Como en el juego, cambia lo rápido que giran los soldados de asalto, los reborn y algunos otros NPC, lo rápido que caminan y corren los incursores tusken (a partir de {{DiffKnight}}) y lo cerca que deben estar dos sables láser para chocar. Las entidades que el mapa marca para otras dificultades (o `notsingle`) se dejan fuera, como las quita el juego al cargar el mapa; también un NPC que nombra un script pero cuyo spawner se quita así (el `affect` del script sobre él no hace nada). Cambiarlo reconstruye enseguida la vista previa.",
    "Language, theme and interface size are in the **{{MenuView}}** menu.":
        "El idioma, el tema y el tamaño de la interfaz están en el menú **{{MenuView}}**.",
    "The preview part of the preferences: sound levels, game rules and difficulty.":
        "La parte de vista previa de las preferencias: volúmenes de sonido, reglas del juego y dificultad.",
    "Changing keys":
        "Cambiar teclas",
    "**{{MenuEdit}} → {{KeysTitle}}** lists every action that has a key. Click a shortcut and press the new key; <kbd>Esc</kbd> cancels, <kbd>Backspace</kbd> clears. A key that is already taken is named. The defaults are in [[tasten]].":
        "**{{MenuEdit}} → {{KeysTitle}}** lista cada acción que tiene una tecla. Pulsa un atajo y luego la tecla nueva; <kbd>Esc</kbd> cancela, <kbd>Retroceso</kbd> lo borra. Una tecla ya ocupada se nombra. Los valores predeterminados están en [[tasten]].",
    "Updates and changelog":
        "Actualizaciones y registro de cambios",
    "**{{MenuHelp}} → {{UpdMenu}}** asks GitHub for a newer version and shows what is new; **{{UpdInstall}}** replaces the program files (with the single `.exe`, just the program, whatever its file name) and offers a restart. The switch next to it decides whether this happens at startup. **{{MenuHelp}} → {{ChangelogMenu}}** shows what was fixed and added, version by version, in your language - it also opens once by itself after an update.":
        "**{{MenuHelp}} → {{UpdMenu}}** pregunta a GitHub por una versión más reciente y muestra lo nuevo; **{{UpdInstall}}** sustituye los archivos del programa (con el `.exe` suelto, solo el programa, sea cual sea su nombre de archivo) y ofrece reiniciar. El interruptor de al lado decide si esto ocurre al iniciar. **{{MenuHelp}} → {{ChangelogMenu}}** muestra qué se corrigió y añadió, versión a versión, en tu idioma; también se abre una vez solo tras una actualización.",
    "Keyboard shortcuts":
        "Atajos de teclado",
    "The default keys. Most of them can be changed under Edit → Keyboard shortcuts.":
        "Las teclas predeterminadas. La mayoría se pueden cambiar en Edición → Atajos de teclado.",
    "Script":
        "Script",
    "Edit the selected command":
        "Editar el comando seleccionado",
    "Insert the command selected in the Events list":
        "Insertar el comando seleccionado en la lista de eventos",
    "Delete":
        "Eliminar",
    "Duplicate":
        "Duplicar",
    "Copy":
        "Copiar",
    "Cut (also Ctrl+T)":
        "Cortar (también Ctrl+T)",
    "Paste":
        "Pegar",
    "REM: comment out / back (Ctrl+Backspace: uncomment)":
        "REM: comentar / descomentar (Ctrl+Retroceso: descomentar)",
    "Move up (Alt+Down: down), staying in the block":
        "Subir (Alt+Abajo: bajar), dentro del bloque",
    "Undo (Ctrl+Y or Ctrl+Shift+Z: redo)":
        "Deshacer (Ctrl+Y o Ctrl+Mayús+Z: rehacer)",
    "Find; F3 / Shift+F3 next / previous":
        "Buscar; F3 / Mayús+F3 siguiente / anterior",
    "Toggle bookmark; F2 / Shift+F2 next / previous":
        "Poner/quitar marcador; F2 / Mayús+F2 siguiente / anterior",
    "Expand item and children (Ctrl+Num+: all)":
        "Desplegar el elemento y sus hijos (Ctrl+Num+: todos)",
    "Collapse item (Ctrl+Num-: all)":
        "Plegar el elemento (Ctrl+Num-: todos)",
    "Files":
        "Archivos",
    "Open this guide":
        "Abrir esta guía",
    "Open":
        "Abrir",
    "Save":
        "Guardar",
    "Save as (as in the original BehavEd)":
        "Guardar como (como en el BehavEd original)",
    "Save all":
        "Guardar todo",
    "Backup; Alt+R restore":
        "Copia de seguridad; Alt+R restaurar",
    "Recently opened files":
        "Archivos abiertos recientemente",
    "Prefs; Alt+X exit":
        "Preferencias; Alt+X salir",
    "Map and camera":
        "Mapa y cámara",
    "W A S D":
        "W A S D",
    "Fly (mouse over the view); Q / E down / up, Shift faster; right mouse looks around":
        "Volar (ratón sobre la vista); Q / E bajar / subir, Mayús más rápido; el botón derecho mira alrededor",
    "Save view 1 (… 9); 1 … 9 jumps back":
        "Guardar la vista 1 (… 9); 1 … 9 vuelve a ella",
    "While dragging an entity: rings in 15-degree steps; with a grid set it turns the grid off, without one an arrow snaps to 8 units":
        "Al arrastrar una entidad: anillos en pasos de 15 grados; con rejilla la desactiva, sin ella una flecha ajusta a 8 unidades",
    "Alt+drag an entity: copy it first":
        "Alt+arrastrar una entidad: primero la copia",
    "While dragging a flight target or a key diamond: only up and down":
        "Al arrastrar un destino de vuelo o un rombo de clave: solo arriba y abajo",
    "Cancel placing, the waypoint tool or piloting":
        "Cancelar la colocación, la herramienta de waypoints o el pilotaje",
    "While piloting: set a camera key at the playhead":
        "Mientras pilotas: poner una clave de cámara en el cabezal",
    "Left / Right":
        "Izquierda / Derecha",
    "50 ms back / forward, mouse over the timeline (Shift: 1 s)":
        "50 ms atrás / adelante, ratón sobre la línea de tiempo (Mayús: 1 s)",
    "Home / End":
        "Inicio / Fin",
    "Jump to start / end (mouse over the timeline)":
        "Saltar al principio / al final (ratón sobre la línea de tiempo)",
    "While dragging a block: move only this block":
        "Al arrastrar un bloque: mover solo este bloque",
    "Window":
        "Ventana",
    "Ctrl + wheel":
        "Ctrl + rueda",
    "Interface size (over the timeline tracks: zoom the timeline)":
        "Tamaño de la interfaz (sobre las pistas de la línea de tiempo: zoom de la línea de tiempo)",
    "Glossary":
        "Glosario",
    "The words of Jedi Academy scripting in one place.":
        "Las palabras de los scripts de Jedi Academy en un solo sitio.",
    "Term":
        "Término",
    "Meaning":
        "Significado",
    "The scripting system of the game. Scripts are text (`.txt`) and run compiled (`.ibi`).":
        "El sistema de scripts del juego. Los scripts son texto (`.txt`) y se ejecutan compilados (`.ibi`).",
    "A compiled script - what the game actually runs. **{{Compile}}** makes it.":
        "Un script compilado: lo que el juego ejecuta de verdad. **{{Compile}}** lo crea.",
    "The command description file: every command, its fields and their choices.":
        "El archivo de descripción de comandos: cada comando, sus campos y sus opciones.",
    "A block whose commands go to one named figure or entity.":
        "Un bloque cuyos comandos van a una figura o entidad con nombre.",
    "A named group of commands; `do` starts it alongside, `dowait` waits until it is done.":
        "Un grupo de comandos con nombre; `do` lo inicia en paralelo, `dowait` espera a que termine.",
    "A pause in milliseconds. In the game it ends one frame (50 ms) later than written.":
        "Una pausa en milisegundos. En el juego termina un fotograma (50 ms) más tarde de lo escrito.",
    "Sends a figure walking to a `waypoint_navgoal` - straight when it is close, otherwise over the waypoint network. See [[figuren#wieimspiel]].":
        "Manda a una figura caminar hasta un `waypoint_navgoal`: en línea recta si está cerca, si no por la red de waypoints. Mira [[figuren#wieimspiel]].",
    "How fast an NPC turns: 60 + 3 × value degrees per second.":
        "Lo rápido que gira un NPC: 60 + 3 × valor grados por segundo.",
    "Movie Duels only: lets the blades of NPCs in `BS_CINEMATIC` clash. See [[figuren#schwerter]].":
        "Solo Movie Duels: deja chocar las hojas de los NPC en `BS_CINEMATIC`. Mira [[figuren#schwerter]].",
    "The entity that makes a character appear; `NPC_type` says which one, `NPC_targetname` names it for scripts.":
        "La entidad que hace aparecer a un personaje; `NPC_type` dice cuál, `NPC_targetname` lo nombra para los scripts.",
    "A named mark in the map - for camera positions and look targets.":
        "Una marca con nombre en el mapa, para posiciones de cámara y objetivos de mirada.",
    "Runs a script when it is used (by a trigger, another entity or a script).":
        "Ejecuta un script cuando se usa (por un trigger, otra entidad o un script).",
    "A brush model of the map that scripts can move - ships, doors, platforms.":
        "Un modelo de brushes del mapa que los scripts pueden mover: naves, puertas, plataformas.",
    "A recorded movement: one small step of position and angle per game frame. See [[animation#roff]].":
        "Un movimiento grabado: un pequeño paso de posición y ángulo por fotograma de juego. Mira [[animation#roff]].",
    "A zip archive the game reads its files from. A mission is packed into one.":
        "Un archivo zip del que el juego lee sus archivos. Una misión se empaqueta en uno.",
    "The entities of a map in a separate file; Movie Duels keeps the map and puts its own entities next to it.":
        "Las entidades de un mapa en un archivo aparte; Movie Duels conserva el mapa y pone sus propias entidades al lado.",
    "A large Jedi Academy mod with cutscene missions. Its game code differs from OpenJK in places; the preview follows either one - [[einstellungen#prefs]].":
        "Un gran mod de Jedi Academy con misiones de cinemáticas. Su código de juego difiere de OpenJK en algunos puntos; la vista previa sigue a cualquiera de los dos: [[einstellungen#prefs]].",
    "Every command":
        "Todos los comandos",
    "What each command of the Events list does, every camera command, every macro and every SET_ field - with the value it expects.":
        "Lo que hace cada comando de la lista de eventos, cada comando de cámara, cada macro y cada campo SET_, con el valor que espera.",
    "Commands are inserted from the **{{GroupEvents}}** list (double-click, drag or <kbd>Insert</kbd>) and changed in the **{{EditorTitle}}** (double-click the line). Hover over a command in the list to read the help text the game's designers wrote for it. How to put them together: [[skripte]].":
        "Los comandos se insertan desde la lista **{{GroupEvents}}** (doble clic, arrastrar o <kbd>Insert</kbd>) y se cambian en el **{{EditorTitle}}** (doble clic en la línea). Pasa el ratón por encima de un comando de la lista para leer el texto de ayuda que escribieron para él los diseñadores del juego. Cómo combinarlos: [[skripte]].",
    "The commands of the Events list":
        "Los comandos de la lista de eventos",
    "Each command has its own page with pictures of the editor and of all three views: [[cmd]].":
        "Cada comando tiene su propia página con imágenes del editor y de las tres vistas: [[cmd]].",
    "Command":
        "Comando",
    "Example":
        "Ejemplo",
    "Clears every command still waiting for this entity - what an `affect` block with `FLUSH` does at its start.":
        "Borra cada comando que aún espera para esta entidad: lo que hace un bloque `affect` con `FLUSH` al empezar.",
    "Runs the block only if the comparison holds (`=`, `<`, `>`, `!` for *not equal*). `else` must follow right after an `if`.":
        "Ejecuta el bloque solo si se cumple la comparación (`=`, `<`, `>`, `!` para *distinto*). `else` debe seguir justo después de un `if`.",
    "Repeats its block; the number says how often, `-1` means forever.":
        "Repite su bloque; el número dice cuántas veces, `-1` significa para siempre.",
    "Sends the commands of its block to one entity - a figure, a ship, a door - by its name. `FLUSH` replaces what the entity was doing, `INSERT` adds to it. The script itself does not wait for the block.":
        "Envía los comandos de su bloque a una entidad (una figura, una nave, una puerta) por su nombre. `FLUSH` sustituye lo que estaba haciendo la entidad, `INSERT` lo añade. El script en sí no espera al bloque.",
    "Starts another script on the entity that runs this one.":
        "Inicia otro script en la entidad que ejecuta este.",
    "Pauses for so many milliseconds - or, with a name, until that task is finished.":
        "Hace una pausa de tantos milisegundos o, con un nombre, hasta que termina esa tarea.",
    "Pauses until some other script calls `signal` with the same name. Only one script can wait for a given signal.":
        "Hace una pausa hasta que algún otro script llama a `signal` con el mismo nombre. Solo un script puede esperar una señal dada.",
    "Wakes the script that waits for this name.":
        "Despierta al script que espera este nombre.",
    "Plays a sound on a channel of the entity - `CHAN_VOICE` for dialogue, so the mouth moves.":
        "Reproduce un sonido en un canal de la entidad: `CHAN_VOICE` para el diálogo, así se mueve la boca.",
    "Moves an entity (a door, a platform, a ship) to a position - and, if given, to new angles - in so many milliseconds. The Animate window writes these for you ([[k-tuer]]).":
        "Mueve una entidad (una puerta, una plataforma, una nave) a una posición (y, si se indica, a ángulos nuevos) en tantos milisegundos. La ventana de animación los escribe por ti ([[k-tuer]]).",
    "Turns an entity to angles (pitch, yaw, roll) in so many milliseconds.":
        "Gira una entidad a unos ángulos (cabeceo, guiñada, alabeo) en tantos milisegundos.",
    "Uses an entity by name, as if the player pressed it - opens a door, fires a trigger.":
        "Usa una entidad por su nombre, como si el jugador la pulsara: abre una puerta, dispara un trigger.",
    "Kills the entity with this name.":
        "Mata a la entidad con este nombre.",
    "Removes the entity with this name from the game.":
        "Quita del juego la entidad con este nombre.",
    "Shows a text in the middle of the screen.":
        "Muestra un texto en el centro de la pantalla.",
    "A comment - does nothing in the game. **{{ActRem}}** turns any line into one.":
        "Un comentario: no hace nada en el juego. **{{ActRem}}** convierte cualquier línea en uno.",
    "Declares a variable (`FLOAT`, `STRING`, `VECTOR`; at most 16 per map) - `free` gives the name back.":
        "Declara una variable (`FLOAT`, `STRING`, `VECTOR`; como máximo 16 por mapa); `free` devuelve el nombre.",
    "Changes a value: an entity field (`SET_NAVGOAL`, `SET_ANIM_BOTH`, ... - see the full list below) or a declared variable.":
        "Cambia un valor: un campo de entidad (`SET_NAVGOAL`, `SET_ANIM_BOTH`, ...; mira la lista completa abajo) o una variable declarada.",
    "Controls the cutscene camera; the first value chooses what it does - see the next section.":
        "Controla la cámara de la cinemática; el primer valor elige lo que hace: mira la sección siguiente.",
    "Declares a named group of commands - it does not run yet.":
        "Declara un grupo de comandos con nombre; todavía no se ejecuta.",
    "Starts a task and goes on at once.":
        "Inicia una tarea y sigue enseguida.",
    "Starts a task and waits until it is done - for a walk: until the figure has arrived.":
        "Inicia una tarea y espera a que termine; para una marcha: hasta que la figura haya llegado.",
    "Plays a ROFF file on the entity of the current `affect` block - a recorded movement.":
        "Reproduce un archivo ROFF en la entidad del bloque `affect` actual: un movimiento grabado.",
    "A command marked with `*` in its help text (for example `SET_NAVGOAL`) finishes its task only when the action is done - that is what lets `dowait` wait for it.":
        "Un comando marcado con `*` en su texto de ayuda (por ejemplo `SET_NAVGOAL`) solo termina su tarea cuando la acción ha concluido: eso es lo que deja que `dowait` lo espere.",
    "camera ( ... ) - the camera commands":
        "camera ( ... ): los comandos de cámara",
    "Values":
        "Valores",
    "What it does (help text of the game)":
        "Qué hace (texto de ayuda del juego)",
    "Puts game in camera mode":
        "Pone el juego en modo cámara",
    "Takes game out of camera mode":
        "Saca el juego del modo cámara",
    "number, whole number":
        "número, número entero",
    "Normal is 80, 10 is zoomed in, max is 120.  Second value is time in ms to take to zoom to the new FOV":
        "Lo normal es 80, 10 es zoom cercano, el máximo es 120. El segundo valor es el tiempo en ms para llegar al nuevo FOV",
    "vector, whole number":
        "vector, número entero",
    "Move to a absolute vector origin or TAG(\"targetname\", ORIGIN) over time over number of milliseconds":
        "Moverse a un origen vectorial absoluto o TAG(\"targetname\", ORIGIN) en un número de milisegundos",
    "vector, vector, whole number":
        "vector, vector, número entero",
    "Pan to absolute angle from current angle in dir (no dir will use shortest) over number of milliseconds":
        "Girar a un ángulo absoluto desde el ángulo actual en la dirección indicada (sin dirección usa la más corta) en un número de milisegundos",
    "Roll to relative angle offsets of current angle over number of milliseconds":
        "Alabear a desplazamientos de ángulo relativos al ángulo actual en un número de milisegundos",
    "text, number, whole number":
        "texto, número, número entero",
    "Get on track and move at speed, last number is whether or not to lerp to the start pos":
        "Ponerse en el recorrido y moverse a esa velocidad; el último número indica si interpolar o no hasta la posición inicial",
    "Follow ents with matching cameraGroup at angleSpeed, last number is whether or not to lerp to the start angle":
        "Seguir a las entidades con el cameraGroup coincidente a angleSpeed; el último número indica si interpolar o no hasta el ángulo inicial",
    "Keep this distance from cameraGroup (if any), last number is whether or not to lerp to the start angle":
        "Mantener esta distancia del cameraGroup (si hay); el último número indica si interpolar o no hasta el ángulo inicial",
    "vector, number, vector, number, whole number":
        "vector, número, vector, número, número entero",
    "Fade from [start Red Green Blue], [Opacity] to [end Red Green Blue], [Opacity] (all fields valid ranges are 0 to 1) over [number of milliseconds]":
        "Fundido de [Rojo Verde Azul iniciales], [Opacidad] a [Rojo Verde Azul finales], [Opacidad] (todos los campos van de 0 a 1) en [número de milisegundos]",
    "Intensity (0-16) and duration, in milliseconds":
        "Intensidad (0-16) y duración, en milisegundos",
    "text":
        "texto",
    "Path to ROF file":
        "Ruta al archivo ROF",
    "Macros":
        "Macros",
    "At the end of the **{{GroupEvents}}** list. A macro inserts several `set` commands at once; **{{TreeFoldMacros}}** shows it as one line.":
        "Al final de la lista **{{GroupEvents}}**. Una macro inserta varios comandos `set` a la vez; **{{TreeFoldMacros}}** la muestra como una sola línea.",
    "Macro":
        "Macro",
    "Commands":
        "Comandos",
    "simply stands, ignores enemies & alerts":
        "simplemente está de pie, ignora enemigos y alertas",
    "simply walks, ignores enemies & alerts":
        "simplemente camina, ignora enemigos y alertas",
    "simply runs, ignores enemies & alerts":
        "simplemente corre, ignora enemigos y alertas",
    "simply stands, ignores alerts":
        "simplemente está de pie, ignora alertas",
    "simply walks, ignores alerts":
        "simplemente camina, ignora alertas",
    "simply runs, ignores alerts":
        "simplemente corre, ignora alertas",
    "simply stands, ignores enemies":
        "simplemente está de pie, ignora enemigos",
    "simply walks, ignores enemies":
        "simplemente camina, ignora enemigos",
    "simply runs, ignores enemies":
        "simplemente corre, ignora enemigos",
    "looks for enemies, shoots at but doesn't chase them":
        "busca enemigos, les dispara pero no los persigue",
    "patrols in a run, chases enemies":
        "patrulla corriendo, persigue a los enemigos",
    "patrols, sticks to patrol route even when shooting enemy":
        "patrulla, se ciñe a la ruta de patrulla incluso al disparar a un enemigo",
    "patrols in a walk, sticks to walking patrol route even when shooting enemy":
        "patrulla caminando, se ciñe a la ruta caminando incluso al disparar a un enemigo",
    "patrols in a run, sticks to running patrol route even when shooting enemy":
        "patrulla corriendo, se ciñe a la ruta corriendo incluso al disparar a un enemigo",
    "resets to default behavior":
        "vuelve al comportamiento por defecto",
    "set ( SET_... ) - every field":
        "set ( SET_... ): todos los campos",
    "All fields `set` can change, grouped as in the game's own list (`Q3_Interface.h`). The description is the one the editor shows - Raven's own words. Type part of a name into the search box of this guide to find a field.":
        "Todos los campos que puede cambiar `set`, agrupados como en la propia lista del juego (`Q3_Interface.h`). La descripción es la que muestra el editor, con las palabras de Raven. Escribe parte de un nombre en el campo de búsqueda de esta guía para encontrar un campo.",
    "Group":
        "Grupo",
    "Field":
        "Campo",
    "Value":
        "Valor",
    "Parm strings":
        "Cadenas parm",
    "Set entity parm1":
        "Fijar parm1 de la entidad",
    "Set entity parm2":
        "Fijar parm2 de la entidad",
    "Set entity parm3":
        "Fijar parm3 de la entidad",
    "Set entity parm4":
        "Fijar parm4 de la entidad",
    "Set entity parm5":
        "Fijar parm5 de la entidad",
    "Set entity parm6":
        "Fijar parm6 de la entidad",
    "Set entity parm7":
        "Fijar parm7 de la entidad",
    "Set entity parm8":
        "Fijar parm8 de la entidad",
    "Set entity parm9":
        "Fijar parm9 de la entidad",
    "Set entity parm10":
        "Fijar parm10 de la entidad",
    "Set entity parm11":
        "Fijar parm11 de la entidad",
    "Set entity parm12":
        "Fijar parm12 de la entidad",
    "Set entity parm13":
        "Fijar parm13 de la entidad",
    "Set entity parm14":
        "Fijar parm14 de la entidad",
    "Set entity parm15":
        "Fijar parm15 de la entidad",
    "Set entity parm16":
        "Fijar parm16 de la entidad",
    "Scripts and other file paths":
        "Scripts y otras rutas de archivo",
    "Script to run when spawned":
        "Script que se ejecuta al aparecer",
    "Script to run when used":
        "Script que se ejecuta al usarlo",
    "Script to run when startled":
        "Script que se ejecuta al sobresaltarse",
    "Script run when find an enemy for the first time":
        "Script que se ejecuta al encontrar un enemigo por primera vez",
    "Script to run when you shoot":
        "Script que se ejecuta al disparar",
    "Script to run when killed someone":
        "Script que se ejecuta al matar a alguien",
    "Script to run when you can't find your enemy":
        "Script que se ejecuta cuando no encuentras a tu enemigo",
    "Script to run when hit":
        "Script que se ejecuta al recibir un golpe",
    "Script to run when hit and low health":
        "Script que se ejecuta al recibir un golpe con poca salud",
    "Script to run when killed":
        "Script que se ejecuta al morir",
    "Script to run after a delay":
        "Script que se ejecuta tras una espera",
    "Script to run when blocked by teammate":
        "Script que se ejecuta cuando te bloquea un compañero",
    "Script to run when player has shot own team repeatedly":
        "Script que se ejecuta cuando el jugador ha disparado varias veces a su propio equipo",
    "Script to run when player kills a teammate":
        "Script que se ejecuta cuando el jugador mata a un compañero",
    "Play a video (inGame)":
        "Reproducir un vídeo (en el juego)",
    "Script to run when skipping the running cinematic":
        "Script que se ejecuta al saltarse la cinemática en curso",
    "Turn off the centered movers on the given track":
        "Apagar los móviles centrados en el recorrido indicado",
    "Turn on the centered movers on the given track":
        "Encender los móviles centrados en el recorrido indicado",
    "just blindly sets whatever skin you set!  include full path after \"base/\"... eg: \"models/players/tavion_new/model_possessed.skin\"":
        "¡fija a ciegas la skin que indiques! incluye la ruta completa tras \"base/\"... p. ej.: \"models/players/tavion_new/model_possessed.skin\"",
    "Standard strings":
        "Cadenas estándar",
    "Set enemy by targetname":
        "Fijar el enemigo por targetname",
    "Set for BS_FOLLOW_LEADER":
        "Fijar para BS_FOLLOW_LEADER",
    "*Move to this navgoal then continue script":
        "*Moverse a este navgoal y luego continuar el script",
    "Set captureGoal by targetname":
        "Fijar captureGoal por targetname",
    "Set angles toward ent by targetname":
        "Fijar los ángulos hacia la entidad por targetname",
    "Set angles toward ent by targetname, will *continue* to face them... only in BS_CINEMATIC":
        "Fijar los ángulos hacia la entidad por targetname; *seguirá* mirándola... solo en BS_CINEMATIC",
    "Set/change your targetname":
        "Fijar/cambiar tu targetname",
    "Set/change what to use when hit":
        "Fijar/cambiar qué usar al recibir un golpe",
    "all ents with this cameraGroup will be focused on":
        "se enfocarán todas las entidades con este cameraGroup",
    "What tag on all clients to try to track":
        "Qué tag intentar seguir en todos los clientes",
    "object for NPC to look at":
        "objeto que mira el NPC",
    "object to place on NPC right hand bolt":
        "objeto que colocar en el bolt de la mano derecha del NPC",
    "object to remove from NPC right hand bolt":
        "objeto que quitar del bolt de la mano derecha del NPC",
    "object to place on NPC left hand bolt":
        "objeto que colocar en el bolt de la mano izquierda del NPC",
    "object to remove from NPC left hand bolt":
        "objeto que quitar del bolt de la mano izquierda del NPC",
    "Color of text RED,WHITE,BLUE, YELLOW":
        "Color del texto RED, WHITE, BLUE, YELLOW",
    "Copy the origin of the ent with targetname to your origin":
        "Copiar el origen de la entidad con targetname a tu origen",
    "This NPC will attack the target NPC's enemies":
        "Este NPC atacará a los enemigos del NPC objetivo",
    "Set/change your target":
        "Fijar/cambiar tu target",
    "Set/change your target2, on NPC's, this fires when they're knocked out by the red hypo":
        "Fijar/cambiar tu target2; en los NPC se dispara cuando los deja inconscientes la hipodérmica roja",
    "What trigger_location you're in - Can only be gotten, not set!":
        "En qué trigger_location estás: solo se puede leer, no fijar",
    "Target that is fired when someone completes the BS_REMOVE behaviorState":
        "Target que se dispara cuando alguien completa el behaviorState BS_REMOVE",
    "Load the savegame that was auto-saved when you started the holodeck":
        "Cargar la partida guardada automáticamente al iniciar el holodeck",
    "Lock legs to a certain yaw angle (or \"off\" or \"auto\" uses current)":
        "Bloquear las piernas en un ángulo de guiñada (o \"off\"; \"auto\" usa el actual)",
    "Make the player look through this ent's eyes - also shunts player movement control to this ent":
        "Hace que el jugador mire a través de los ojos de esta entidad; también pasa a ella el control del movimiento del jugador",
    "Looping sound to play on entity":
        "Sonido en bucle que reproducir en la entidad",
    "Specify name of entity to freeze - !!!NOTE!!! since the ent is frozen, you must have some other entity unfreeze it!!!":
        "Nombre de la entidad que congelar. ¡¡¡NOTA!!! como la entidad queda congelada, ¡¡¡otra entidad tiene que descongelarla!!!",
    "Specify name of entity to unfreeze":
        "Nombre de la entidad que descongelar",
    "Name of a saber in sabers.cfg to use in first hand.  \"none\" removes current saber":
        "Nombre de un sable de sabers.cfg para la primera mano. \"none\" quita el sable actual",
    "Name of a saber in sabers.cfg to use in second hand.  \"none\" removes current saber":
        "Nombre de un sable de sabers.cfg para la segunda mano. \"none\" quita el sable actual",
    "Name of an NPC config in NPC2.cfg to use for this ent":
        "Nombre de una configuración de NPC de NPC2.cfg para esta entidad",
    "Name of an vehicle config in vehicles.cfg to make this ent drive":
        "Nombre de una configuración de vehículo de vehicles.cfg que conducirá esta entidad",
    "key of text string to print":
        "clave de la cadena de texto que imprimir",
    "key of text string to print in LCARS frame":
        "clave de la cadena de texto que imprimir en un marco LCARS",
    "key of text string to print in center of screen.":
        "clave de la cadena de texto que imprimir en el centro de la pantalla.",
    "vectors":
        "vectores",
    "vector":
        "vector",
    "Set origin explicitly or with TAG":
        "Fijar el origen explícitamente o con TAG",
    "Set angles explicitly or with TAG":
        "Fijar los ángulos explícitamente o con TAG",
    "Set origin here as soon as the area is clear":
        "Fijar el origen aquí en cuanto la zona esté despejada",
    "Removes this ent's saber from their hand, turns it off, and places it at the specified location":
        "Quita el sable de la mano de esta entidad, lo apaga y lo coloca en la posición indicada",
    "floats":
        "decimales",
    "number":
        "número",
    "Velocity along X axis":
        "Velocidad a lo largo del eje X",
    "Velocity along Y axis":
        "Velocidad a lo largo del eje Y",
    "Velocity along Z axis":
        "Velocidad a lo largo del eje Z",
    "Vertical offset from original origin... offset/ent's speed * 1000ms is duration":
        "Desplazamiento vertical desde el origen original... desplazamiento/velocidad de la entidad * 1000 ms es la duración",
    "Pitch for NPC to turn to":
        "Cabeceo hacia el que se gira el NPC",
    "Yaw for NPC to turn to":
        "Guiñada hacia la que se gira el NPC",
    "Speed-up slow down game (0 - 1.0)":
        "Acelerar o ralentizar el juego (0 - 1.0)",
    "when following an ent with the camera, apply this z ofs":
        "al seguir una entidad con la cámara, aplicar este desplazamiento en z",
    "How far away NPC can see":
        "Hasta dónde ve el NPC",
    "How far an NPC can hear":
        "Hasta dónde oye un NPC",
    "How often to look for enemies (0 - 1.0)":
        "Cada cuánto busca enemigos (0 - 1.0)",
    "Change this ent's gravity - 800 default":
        "Cambiar la gravedad de esta entidad (800 por defecto)",
    "Set face to Aux expression for number of seconds":
        "Poner la cara en la expresión Aux durante un número de segundos",
    "Set face to Blink expression for number of seconds":
        "Poner la cara en la expresión Blink durante un número de segundos",
    "Set face to Blinkfrown expression for number of seconds":
        "Poner la cara en la expresión Blinkfrown durante un número de segundos",
    "Set face to Frown expression for number of seconds":
        "Poner la cara en la expresión Frown durante un número de segundos",
    "Set face to Normal expression for number of seconds":
        "Poner la cara en la expresión Normal durante un número de segundos",
    "Set face to Eyes closed":
        "Poner la cara con los ojos cerrados",
    "Set face to Eyes open":
        "Poner la cara con los ojos abiertos",
    "Change an entity's wait field":
        "Cambiar el campo wait de una entidad",
    "How far away to stay from leader in BS_FOLLOW_LEADER":
        "A qué distancia del líder mantenerse en BS_FOLLOW_LEADER",
    "Scale the entity model":
        "Escalar el modelo de la entidad",
    "Used to ensure rendering for entities with geographically sprawling animations (world units)":
        "Para asegurar el dibujado de entidades con animaciones muy extensas en el espacio (unidades del mundo)",
    "Only to be used in a 'get'. (Distance to player)*(Distance to player)":
        "Solo para usar en un 'get'. (Distancia al jugador)*(Distancia al jugador)",
    "ints":
        "enteros",
    "whole number":
        "número entero",
    "Hold lower anim for number of milliseconds":
        "Mantener la animación inferior durante un número de milisegundos",
    "Hold upper anim for number of milliseconds":
        "Mantener la animación superior durante un número de milisegundos",
    "Hold lower and upper anims for number of milliseconds":
        "Mantener las animaciones inferior y superior durante un número de milisegundos",
    "Change health":
        "Cambiar la salud",
    "Change armor":
        "Cambiar la armadura",
    "Change walkSpeed":
        "Cambiar walkSpeed",
    "Change runSpeed":
        "Cambiar runSpeed",
    "Change yawSpeed":
        "Cambiar yawSpeed",
    "Change aggression 1-5":
        "Cambiar la agresividad 1-5",
    "Change aim 1-5":
        "Cambiar la puntería 1-5",
    "Change ent's friction - 6 default":
        "Cambiar la fricción de la entidad (6 por defecto)",
    "How far the ent can shoot - 0 uses weapon":
        "Hasta dónde puede disparar la entidad (0 usa el arma)",
    "Horizontal field of view":
        "Campo de visión horizontal",
    "Vertical field of view":
        "Campo de visión vertical",
    "How many milliseconds to wait before running delayscript":
        "Cuántos milisegundos esperar antes de ejecutar delayscript",
    "NPC move forward -127(back) to 127":
        "El NPC avanza de -127 (atrás) a 127",
    "NPC move right -127(left) to 127":
        "El NPC se mueve a la derecha de -127 (izquierda) a 127",
    "frame to start animation sequence on":
        "fotograma en el que empieza la secuencia de animación",
    "frame to end animation sequence on":
        "fotograma en el que termina la secuencia de animación",
    "frame to set animation sequence to":
        "fotograma al que fijar la secuencia de animación",
    "Change an entity's count field":
        "Cambiar el campo count de una entidad",
    "Time between shots for an NPC - reset to defaults when changes weapon":
        "Tiempo entre disparos de un NPC; vuelve a los valores por defecto al cambiar de arma",
    "Amount of time until Mission Status should be shown after death":
        "Tiempo hasta que se muestra el estado de la misión tras morir",
    "Width of NPC bounding box.":
        "Ancho de la caja delimitadora del NPC.",
    "Activate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).":
        "Activar una hoja concreta del sable 1 (0 - (MAX_BLADES - 1)).",
    "Deactivate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).":
        "Desactivar una hoja concreta del sable 1 (0 - (MAX_BLADES - 1)).",
    "Activate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).":
        "Activar una hoja concreta del sable 2 (0 - (MAX_BLADES - 1)).",
    "Deactivate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).":
        "Desactivar una hoja concreta del sable 2 (0 - (MAX_BLADES - 1)).",
    "Damage this entity with set amount.":
        "Dañar esta entidad con la cantidad indicada.",
    "booleans":
        "booleanos",
    "Do not react to pain":
        "No reaccionar al dolor",
    "Do not acquire enemies":
        "No buscar enemigos",
    "Do not get enemy set by allies in area(ambush)":
        "No aceptar enemigos fijados por aliados de la zona (emboscada)",
    "Others won't shoot you":
        "Los demás no te disparan",
    "Others won't pick you as enemy":
        "Los demás no te eligen como enemigo",
    "Don't fire your weapon":
        "No disparar tu arma",
    "Keep current enemy until dead":
        "Mantener el enemigo actual hasta que muera",
    "Force NPC to crouch":
        "Obligar al NPC a agacharse",
    "Force NPC to move at walkSpeed":
        "Obligar al NPC a moverse a walkSpeed",
    "Force NPC to move at runSpeed":
        "Obligar al NPC a moverse a runSpeed",
    "NPC will chase after enemies":
        "El NPC perseguirá a los enemigos",
    "NPC will be on the lookout for enemies":
        "El NPC estará atento a los enemigos",
    "NPC will face in the direction it's moving":
        "El NPC mirará en la dirección en la que se mueve",
    "NPC will not run from danger":
        "El NPC no huirá del peligro",
    "NPC will not move unless you aim at him":
        "El NPC no se moverá salvo que le apuntes",
    "Can take damage down to 1 but not die":
        "Puede recibir daño hasta 1 pero no morir",
    "Will not avoid other NPCs or architecture":
        "No esquivará a otros NPC ni la arquitectura",
    "Make yourself notsolid or solid":
        "Hacerte no sólido o sólido",
    "Can be activateby the player's \"use\" button":
        "Se puede activar con el botón \"usar\" del jugador",
    "For non-NPCs, loop your animation sequence":
        "Para entidades que no son NPC, repetir en bucle tu secuencia de animación",
    "Player interface on/off":
        "Interfaz del jugador activada/desactivada",
    "NPC has no shields (Borg do not adapt)":
        "El NPC no tiene escudos (los Borg no se adaptan)",
    "Makes an NPC not solid and not visible":
        "Hace que un NPC no sea sólido ni visible",
    "Draws only in mirrors/portals":
        "Solo se dibuja en espejos/portales",
    "Force Invincibility effect, also godmode":
        "Efecto de invencibilidad forzado, también modo dios",
    "Makes an NPC greet teammates":
        "Hace que un NPC salude a sus compañeros",
    "Makes video playback fade in":
        "Hace que la reproducción del vídeo entre con fundido",
    "Makes video playback fade out":
        "Hace que la reproducción del vídeo salga con fundido",
    "Makes it so player cannot move":
        "Hace que el jugador no pueda moverse",
    "Makes it so player cannot switch weapons":
        "Hace que el jugador no pueda cambiar de arma",
    "Stops this ent from taking impact damage":
        "Impide que esta entidad reciba daño por impacto",
    "Stops this ent from taking knockback from weapons":
        "Impide que esta entidad sea empujada por las armas",
    "Force NPC to use altfire when shooting":
        "Obligar al NPC a usar el fuego secundario al disparar",
    "NPCs will do generic responses when this is on (usescripts override generic responses as well)":
        "Con esto activado, los NPC dan respuestas genéricas (los usescripts también sustituyen a las respuestas genéricas)",
    "Completely unkillable":
        "Totalmente imposible de matar",
    "NPCs will not do their combat talking noises when this is on":
        "Con esto activado, los NPC no hacen sus ruidos de charla de combate",
    "Player has turned on his own- scripts will stop, NPCs will turn on him and level changes load the brig":
        "El jugador se ha vuelto contra los suyos: los scripts se detienen, los NPC se vuelven contra él y los cambios de nivel cargan el calabozo",
    "Allows turning off an animating shader in a script":
        "Permite apagar un shader animado en un script",
    "Sets a shader with an image map to be under frame control":
        "Pone un shader con un mapa de imagen bajo control de fotogramas",
    "Turns saber on/off":
        "Enciende/apaga el sable",
    "Only set this on things you move with script commands that you *want* to open/close area portals.  Default is off.":
        "Ponlo solo en cosas que mueves con comandos de script y que *quieres* que abran/cierren areaportals. Por defecto desactivado.",
    "When true, only a heavy weapon class missile/laser can damage this ent.":
        "Si es verdadero, solo un misil/láser de clase de arma pesada puede dañar esta entidad.",
    "When true, ion_cannon is shielded from any kind of damage.":
        "Si es verdadero, el ion_cannon está protegido de cualquier tipo de daño.",
    "This NPC cannot alert groups or be part of a group":
        "Este NPC no puede alertar a grupos ni formar parte de un grupo",
    "Makes NPC will hold down the fire button, until this is set to false":
        "Hace que el NPC mantenga pulsado el botón de disparo hasta que esto se ponga en falso",
    "NPC will hold down the fire button, but they won't play firing anim":
        "El NPC mantendrá pulsado el botón de disparo, pero no reproducirá la animación de disparo",
    "NPC will remove only when it's safe (Player is not in PVS)":
        "El NPC solo se quitará cuando sea seguro (el jugador no está en el PVS)",
    "Turn on/off Boba Fett's Jet Pack":
        "Encender/apagar la mochila propulsora de Boba Fett",
    "Makes NPC immune to jedi mind-trick":
        "Hace al NPC inmune al truco mental jedi",
    "in lieu of using a target_activate or target_deactivate":
        "en lugar de usar un target_activate o target_deactivate",
    "provides an alternate way of changing func_usable to be visible or not, DOES NOT AFFECT SOLID":
        "ofrece otra forma de hacer visible o no un func_usable, NO AFECTA A LA SOLIDEZ",
    "Increment secret areas found counter":
        "Incrementar el contador de zonas secretas encontradas",
    "End of game dissolve into star background and credits":
        "Fin del juego: disolverse en un fondo de estrellas y créditos",
    "NPCs will use their closest combat points, not try and find ones next to the player, or flank player":
        "Los NPC usarán sus puntos de combate más cercanos, sin intentar buscar los que están junto al jugador ni flanquearlo",
    "NPC will have a minlight of 96":
        "El NPC tendrá una luz mínima de 96",
    "NPC will not be affected by force powers":
        "Al NPC no le afectarán los poderes de la Fuerza",
    "NPC will not scream and tumble and fall to hit death over large drops":
        "El NPC no gritará ni caerá dando tumbos hasta morir en caídas grandes",
    "NPC will not be dismemberable if you set this to false (default is true)":
        "El NPC no se podrá desmembrar si lo pones en falso (por defecto verdadero)",
    "Jedi won't jump, roll or cartwheel":
        "El jedi no saltará, rodará ni hará la rueda",
    "When true NPC will always display subtitle regardless of subtitle setting":
        "Si es verdadero, el NPC siempre mostrará subtítulos, sea cual sea el ajuste de subtítulos",
    "Removes entities that could muck up cinematics, explosives, turrets, seekers.":
        "Quita las entidades que podrían estropear las cinemáticas: explosivos, torretas, buscadores.",
    "Turns on/off HUD":
        "Enciende/apaga el HUD",
    "This entity will *always* be drawn - use only for special case cinematic NPCs that have anims that cover multiple rooms!!!":
        "Esta entidad se dibujará *siempre*: ¡¡¡úsalo solo para NPC de cinemática especiales cuyas animaciones abarcan varias salas!!!",
    "Set a Saboteur to cloak (true) or un-cloak (false).":
        "Hacer que un saboteador se vuelva invisible (true) o visible (false).",
    "Causes this ent to start force healing at whatever level of force heal they have":
        "Hace que esta entidad empiece a curarse con la Fuerza al nivel de curación que tenga",
    "Causes this ent to start force speeding at whatever level of force speed they have (may not do anything for NPCs?)":
        "Hace que esta entidad empiece la velocidad de la Fuerza al nivel que tenga (¿puede que no haga nada en los NPC?)",
    "Causes this ent to do a force push at whatever level of force push they have - will not fail":
        "Hace que esta entidad haga un empujón de la Fuerza al nivel que tenga; no fallará",
    "Causes this ent to do a force push anim, sound and effect, will not push anything":
        "Hace que esta entidad haga la animación, el sonido y el efecto de un empujón de la Fuerza; no empujará nada",
    "Causes this ent to do a jedi mind trick at whatever level of mind trick they have (may not do anything for NPCs?)":
        "Hace que esta entidad haga un truco mental jedi al nivel que tenga (¿puede que no haga nada en los NPC?)",
    "Causes this ent to grip their enemy at whatever level of grip they have (will grip until scripted to stop)":
        "Hace que esta entidad estrangule a su enemigo al nivel de estrangulamiento que tenga (estrangulará hasta que un script lo pare)",
    "Causes this ent to lightning at whatever level of lightning they have (will lightning until scripted to stop)":
        "Hace que esta entidad lance relámpagos al nivel que tenga (lanzará relámpagos hasta que un script lo pare)",
    "Causes this ent to throw their saber at whatever level of saber throw they have (will throw saber until scripted to stop)":
        "Hace que esta entidad lance su sable al nivel de lanzamiento que tenga (lanzará el sable hasta que un script lo pare)",
    "Causes this ent to go into force rage at whatever level of force rage they have":
        "Hace que esta entidad entre en ira de la Fuerza al nivel que tenga",
    "Causes this ent to start a force protect at whatever level of force protect they have":
        "Hace que esta entidad empiece una protección de la Fuerza al nivel que tenga",
    "Causes this ent to do start a force absorb at whatever level of force absorb they have":
        "Hace que esta entidad empiece una absorción de la Fuerza al nivel que tenga",
    "Causes this ent to start force draining their enemy at whatever level of force drain they have (will drain until scripted to stop)":
        "Hace que esta entidad empiece a succionar con la Fuerza a su enemigo al nivel que tenga (succionará hasta que un script lo pare)",
    "Set the player to wear his/her winter gear (skins torso_g1 and lower_e1), or restore the default skins.":
        "Pone al jugador su ropa de invierno (skins torso_g1 y lower_e1), o restaura las skins por defecto.",
    "This NPC/player will not have any bone angle overrides or pitch or roll (should only be used in cinematics)":
        "Este NPC/jugador no tendrá ninguna sustitución de ángulos de huesos ni cabeceo ni alabeo (solo debería usarse en cinemáticas)",
    "calls":
        "llamadas",
    "Cannot set this, only get it - valid values are 0 through 3":
        "No se puede fijar, solo leer; valores válidos de 0 a 3",
    "Special tables":
        "Tablas especiales",
    "Torso and head anim":
        "Animación de torso y cabeza",
    "Legs anim":
        "Animación de piernas",
    "Set same anim on torso and legs":
        "Poner la misma animación en torso y piernas",
    "Your team":
        "Tu equipo",
    "Team in which to look for enemies":
        "Equipo en el que buscar enemigos",
    "Change current bState":
        "Cambiar el bState actual",
    "Change fallback bState":
        "Cambiar el bState de reserva",
    "Set/Chang a temp bState":
        "Fijar/cambiar un bState temporal",
    "Events you can initiate":
        "Eventos que puedes iniciar",
    "Change/Stow/Drop weapon":
        "Cambiar/guardar/soltar arma",
    "Give items":
        "Dar objetos",
    "Set the state of the dynamic music":
        "Fijar el estado de la música dinámica",
    "Change force power level":
        "Cambiar el nivel de un poder de la Fuerza",
    "Set color of first blade of first saber":
        "Fijar el color de la primera hoja del primer sable",
    "Set color of second blade of first saber":
        "Fijar el color de la segunda hoja del primer sable",
    "Cut off a part of a body and send the limb flying":
        "Cortar una parte del cuerpo y lanzar la extremidad por los aires",
    "Show objective on mission screen":
        "Mostrar el objetivo en la pantalla de la misión",
    "Hide objective from mission screen":
        "Ocultar el objetivo de la pantalla de la misión",
    "Mark objective as completed":
        "Marcar el objetivo como completado",
    "Mark objective as completed, no update sent to screen":
        "Marcar el objetivo como completado, sin avisar en pantalla",
    "Mark objective as failed":
        "Marcar el objetivo como fallido",
    "Mission failed screen activates":
        "Se activa la pantalla de misión fracasada",
    "Show tactical info on mission objectives screen":
        "Mostrar información táctica en la pantalla de objetivos de la misión",
    "Hide tactical info on mission objectives screen":
        "Ocultar información táctica de la pantalla de objetivos de la misión",
    "Force all objectives to be hidden":
        "Forzar que se oculten todos los objetivos",
    "Used to get whether the player has chosen the light (succeeded) or dark (failed) side.":
        "Sirve para saber si el jugador ha elegido el lado luminoso (éxito) o el oscuro (fracaso).",
    "Text to appear in mission status screen":
        "Texto que aparece en la pantalla de estado de la misión",
    "Brings up specified menu screen":
        "Abre la pantalla de menú indicada",
    "Show closing credits":
        "Mostrar los créditos finales",
    "Lean left, right or stop leaning":
        "Inclinarse a la izquierda, a la derecha o dejar de inclinarse",
    "Expressions: get, tag, random":
        "Expresiones: get, tag, random",
    "Instead of a fixed value, a field can hold an expression. In the editor, **{{EditorHelper}}** builds them:":
        "En vez de un valor fijo, un campo puede contener una expresión. En el editor, **{{EditorHelper}}** las construye:",
    "**{{EditorGet}}** - `get ( FLOAT, \"name\" )` reads a variable declared with `declare`.":
        "**{{EditorGet}}**: `get ( FLOAT, \"nombre\" )` lee una variable declarada con `declare`.",
    "**{{EditorTag}}** - `tag ( \"cam_door\", ORIGIN )` takes the position (or `ANGLES`) of a `ref_tag` on the map. That is how cameras stand on marks.":
        "**{{EditorTag}}**: `tag ( \"cam_puerta\", ORIGIN )` toma la posición (o `ANGLES`) de un `ref_tag` del mapa. Así se colocan las cámaras sobre marcas.",
    "**{{EditorRnd}}** - `random ( 1000, 3000 )` a random number between two values, for example a wait.":
        "**{{EditorRnd}}**: `random ( 1000, 3000 )` un número aleatorio entre dos valores, por ejemplo para una espera.",
    "Every button":
        "Todos los botones",
    "Each button of the program - the mode bar, the action column, the map toolbar, the placement tiles, the inspector, the camera, the timeline and the model view - with what it does.":
        "Cada botón del programa (la barra de modos, la columna de acciones, la barra del mapa, las fichas de colocación, el inspector, la cámara, la línea de tiempo y la vista de modelos) con lo que hace.",
    "Hover over any button in the program to see the same explanation as a tooltip. Buttons with a coloured frame are switches: on or off.":
        "Pasa el ratón por encima de cualquier botón del programa para ver la misma explicación como descripción emergente. Los botones con marco de color son interruptores: activados o desactivados.",
    "The mode bar":
        "La barra de modos",
    "The strip at the far left switches what the window shows. The script stays the same in every mode.":
        "La franja del extremo izquierdo cambia lo que muestra la ventana. El script es el mismo en todos los modos.",
    "Mode":
        "Modo",
    "What it shows":
        "Qué muestra",
    "The **{{GroupEvents}}** list of every command and macro, the script tree, the action column - for writing scripts. [[skripte]]":
        "La lista **{{GroupEvents}}** de todos los comandos y macros, el árbol del script, la columna de acciones: para escribir scripts. [[skripte]]",
    "The mission: the entity list and inspector, the map in 3D with the timeline below - for cameras, figures, entities and saving the mission. [[karte]]":
        "La misión: la lista de entidades y el inspector, el mapa en 3D con la línea de tiempo debajo: para cámaras, figuras, entidades y guardar la misión. [[karte]]",
    "A character model with its animations - to find an animation and put it into the script. [[modell]]":
        "Un modelo de personaje con sus animaciones: para encontrar una animación y ponerla en el script. [[modell]]",
    "Build and edit maps like in NetRadiant-Custom: 2D views, camera, texture browser and entities, with its menus and shortcuts. [[karteneditor]]":
        "Crea y edita mapas como en NetRadiant-Custom: vistas 2D, cámara, navegador de texturas y entidades, con sus menús y atajos. [[karteneditor]]",
    "The action column (Events mode)":
        "La columna de acciones (modo Eventos)",
    "The action column, as in the original BehavEd.":
        "La columna de acciones, como en el BehavEd original.",
    "Button":
        "Botón",
    "Removes the selected lines (<kbd>Delete</kbd>).":
        "Quita las líneas seleccionadas (<kbd>Supr</kbd>).",
    "Inserts a copy right below (<kbd>Space</kbd>).":
        "Inserta una copia justo debajo (<kbd>Espacio</kbd>).",
    "Clipboard (<kbd>Ctrl</kbd>+<kbd>C</kbd> / <kbd>X</kbd> / <kbd>V</kbd>). Paste goes below the selection; text from other programs is accepted after a question.":
        "Portapapeles (<kbd>Ctrl</kbd>+<kbd>C</kbd> / <kbd>X</kbd> / <kbd>V</kbd>). Pegar va debajo de la selección; el texto de otros programas se acepta tras una pregunta.",
    "Turns lines into comments and back (<kbd>Backspace</kbd>).":
        "Convierte líneas en comentarios y al revés (<kbd>Retroceso</kbd>).",
    "The Find window (<kbd>Ctrl</kbd>+<kbd>F</kbd>): search by command or value, replace in one or all scripts. [[k-suchen-ersetzen]]":
        "La ventana Buscar (<kbd>Ctrl</kbd>+<kbd>F</kbd>): busca por comando o valor, reemplaza en uno o en todos los scripts. [[k-suchen-ersetzen]]",
    "Moves the selection one line, inside its block (<kbd>Alt</kbd>+<kbd>Up</kbd>/<kbd>Down</kbd>).":
        "Mueve la selección una línea, dentro de su bloque (<kbd>Alt</kbd>+<kbd>Arriba</kbd>/<kbd>Abajo</kbd>).",
    "Undo and redo. Right-click either one for the full history. [[k-rueckgaengig]]":
        "Deshacer y rehacer. Clic derecho en cualquiera de los dos para ver el historial completo. [[k-rueckgaengig]]",
    "An empty script / open a `.txt` or a compiled `.ibi`.":
        "Un script vacío / abrir un `.txt` o un `.ibi` compilado.",
    "Recently opened files (<kbd>Alt</kbd>+<kbd>M</kbd>).":
        "Archivos abiertos recientemente (<kbd>Alt</kbd>+<kbd>M</kbd>).",
    "Adds another script to the end of this one.":
        "Añade otro script al final de este.",
    "Save this script, every changed tab, or under a new name.":
        "Guarda este script, cada pestaña cambiada, o con un nombre nuevo.",
    "Writes the script as plain text and opens it.":
        "Escribe el script como texto plano y lo abre.",
    "Keeps a copy next to the script (`.bak`) / goes back to it (<kbd>Alt</kbd>+<kbd>B</kbd> / <kbd>Alt</kbd>+<kbd>R</kbd>).":
        "Guarda una copia junto al script (`.bak`) / vuelve a ella (<kbd>Alt</kbd>+<kbd>B</kbd> / <kbd>Alt</kbd>+<kbd>R</kbd>).",
    "Shows the value types in the tree.":
        "Muestra los tipos de los valores en el árbol.",
    "Shows numbers short: `1000` instead of `1000.000`.":
        "Muestra los números cortos: `1000` en vez de `1000.000`.",
    "Shows a macro as one line.":
        "Muestra una macro como una sola línea.",
    "Expands or collapses every block.":
        "Despliega o pliega todos los bloques.",
    "Saves the script and writes the `.ibi` the game runs; green when it worked, red with the messages when not. [[k-uebersetzen]]":
        "Guarda el script y escribe el `.ibi` que ejecuta el juego; verde cuando funciona, rojo con los mensajes cuando no. [[k-uebersetzen]]",
    "The map toolbar (Mission mode)":
        "La barra del mapa (modo Misión)",
    "The toolbar above the map.":
        "La barra de herramientas encima del mapa.",
    "Mission":
        "Misión",
    "**{{MissionNew}}**, **{{LoadMission}}**, **{{MissionSave}}** - and the name of the open mission.":
        "**{{MissionNew}}**, **{{LoadMission}}**, **{{MissionSave}}**, y el nombre de la misión abierta.",
    "Open a map, a mission folder or an `.ent` file; **Scripts (N)** next to it lists the map's scripts.":
        "Abre un mapa, una carpeta de misión o un archivo `.ent`; **Scripts (N)** a su lado lista los scripts del mapa.",
    "What the view shows":
        "Lo que muestra la vista",
    "Entities, names, figures, effects, glow, dynamic light, fog, sky - each a switch.":
        "Entidades, nombres, figuras, efectos, resplandor, luz dinámica, niebla, cielo: cada uno es un interruptor.",
    "Looking":
        "Mirar",
    "Through the camera, top view, the NPC waypoint window.":
        "A través de la cámara, vista superior, la ventana de waypoints de NPC.",
    "Tools":
        "Herramientas",
    "Grid, view settings, statistics.":
        "Rejilla, ajustes de la vista, estadísticas.",
    "Every button in detail:":
        "Cada botón en detalle:",
    "Starts a mission on any map: player start, script runner, intro script and mission folder are created. [[tut-p1]]":
        "Empieza una misión en cualquier mapa: se crean el inicio del jugador, el ejecutor de scripts, el script de introducción y la carpeta de la misión. [[tut-p1]]",
    "Opens a mission from a `.pk3`: map, entities, every script in tabs, the figures. [[missionen]]":
        "Abre una misión de un `.pk3`: mapa, entidades, cada script en pestañas, las figuras. [[missionen]]",
    "Saves every script, writes the `.ent`, compiles and packs the `.pk3` - after checking the mission. [[tut-p8]]":
        "Guarda cada script, escribe el `.ent`, compila y empaqueta el `.pk3`, tras comprobar la misión. [[tut-p8]]",
    "The name of the open mission; a dot means unsaved changes. Hover for its folder.":
        "El nombre de la misión abierta; un punto indica cambios sin guardar. Pasa el ratón por encima para ver su carpeta.",
    "Opens a map from disk or from a `.pk3`, a mission folder, or a separate `.ent` file.":
        "Abre un mapa del disco o de un `.pk3`, una carpeta de misión o un archivo `.ent` aparte.",
    "The scripts the map's entities point to - pick one to open it.":
        "Los scripts a los que apuntan las entidades del mapa: elige uno para abrirlo.",
    "Shows the entity markers: walk targets, marks, runners, spawners.":
        "Muestra los marcadores de entidades: destinos, marcas, ejecutores, spawners.",
    "Writes the names of entities and figures into the view.":
        "Escribe en la vista los nombres de las entidades y las figuras.",
    "Shows the script's figures, moved by the timeline.":
        "Muestra las figuras del script, movidas por la línea de tiempo.",
    "Shows the effects of the map (`fx_runner`).":
        "Muestra los efectos del mapa (`fx_runner`).",
    "The glow of shaders marked `glow`, as `r_DynamicGlow`.":
        "El resplandor de los shaders marcados con `glow`, como `r_DynamicGlow`.",
    "Lightsabers and effect lights light up walls and figures.":
        "Los sables láser y las luces de efectos iluminan paredes y figuras.",
    "The fog of the map, as the game draws it.":
        "La niebla del mapa, como la dibuja el juego.",
    "The sky; switched off you can look into the map from outside.":
        "El cielo; desactivado puedes mirar el mapa desde fuera.",
    "Shows what the camera sees - the selected camera command, or the camera at the playhead. [[k-ansichten]]":
        "Muestra lo que ve la cámara: el comando de cámara seleccionado, o la cámara en el cabezal. [[k-ansichten]]",
    "Looks straight down; a cut height hides ceilings. [[k-draufsicht]]":
        "Mira hacia abajo en vertical; una altura de corte oculta los techos. [[k-draufsicht]]",
    "The NPC waypoint window: show, create and check the network. [[wegpunkte]]":
        "La ventana de waypoints de NPC: mostrar, crear y comprobar la red. [[wegpunkte]]",
    "Snaps entities to 8 to 64 units while dragging and placing.":
        "Ajusta las entidades a 8 a 64 unidades al arrastrar y colocar.",
    "Brightness, ambient light, detail, shadows, highlighting the active script's entities, and the game screen: aspect ratio, cg_fovAspectAdjust and the frame in the free view ([[kamera#vorschau]]). [[k-hell]]":
        "Brillo, luz ambiental, detalle, sombras, resaltar las entidades del script activo, y la pantalla del juego: relación de aspecto, cg_fovAspectAdjust y el marco en la vista libre ([[kamera#vorschau]]). [[k-hell]]",
    "Frame rate, textures, triangles and graphics card values.":
        "Fotogramas por segundo, texturas, triángulos y valores de la tarjeta gráfica.",
    "The placement tiles":
        "Las fichas de colocación",
    "The strip on the left edge of the map. Click a tile, then click on the floor - or drag the tile into the view. <kbd>Shift</kbd> places several, <kbd>Esc</kbd> stops. [[k-entity-setzen]]":
        "La franja del borde izquierdo del mapa. Pulsa una ficha y luego pulsa en el suelo, o arrastra la ficha a la vista. <kbd>Mayús</kbd> coloca varias, <kbd>Esc</kbd> para. [[k-entity-setzen]]",
    "A character that spawns: set `NPC_type` and a name.":
        "Un personaje que aparece: fija `NPC_type` y un nombre.",
    "A place a figure walks to (`SET_NAVGOAL`).":
        "Un sitio al que camina una figura (`SET_NAVGOAL`).",
    "A mark with a direction - for cameras (`tag`) and look targets.":
        "Una marca con dirección, para cámaras (`tag`) y objetivos de mirada.",
    "A point to look at or to aim at.":
        "Un punto al que mirar o apuntar.",
    "Runs a script when it is used - by a trigger, the player start or another script.":
        "Ejecuta un script cuando se usa: por un trigger, el inicio del jugador u otro script.",
    "Plays an effect file (`.efx`).":
        "Reproduce un archivo de efecto (`.efx`).",
    "Plays a sound, once or in a loop.":
        "Reproduce un sonido, una vez o en bucle.",
    "A point of the NPC navigation network.":
        "Un punto de la red de navegación de los NPC.",
    "Where the player spawns.":
        "Donde aparece el jugador.",
    "The entity inspector":
        "El inspector de entidades",
    "The inspector under the entity list.":
        "El inspector bajo la lista de entidades.",
    "Puts it on the floor below, like `DROPTOFLOOR` in the game.":
        "La pone sobre el suelo de debajo, como `DROPTOFLOOR` en el juego.",
    "A copy - of every selected entity. <kbd>Alt</kbd>+drag in the view does the same.":
        "Una copia, de cada entidad seleccionada. <kbd>Alt</kbd>+arrastrar en la vista hace lo mismo.",
    "Removes it.":
        "La quita.",
    "Opens the Animation window for doors, platforms, ships and effects. [[animation]]":
        "Abre la ventana de animación para puertas, plataformas, naves y efectos. [[animation]]",
    "Undoes the last entity change (entities have their own history).":
        "Deshace el último cambio de entidad (las entidades tienen su propio historial).",
    "Redoes it.":
        "Lo rehace.",
    "Adds a key.":
        "Añade una clave.",
    "Next to a script key: opens the script - and creates it in the mission folder if it does not exist yet.":
        "Junto a una clave de script: abre el script, y lo crea en la carpeta de la misión si aún no existe.",
    "The camera bar":
        "La barra de la cámara",
    "Appears in the view when the camera is selected. [[kamera]]":
        "Aparece en la vista cuando la cámara está seleccionada. [[kamera]]",
    "The camera bar above the selected shot.":
        "La barra de la cámara encima del plano seleccionado.",
    "Arrows: move the shot along one axis, the box in the middle in the view plane.":
        "Flechas: mueven el plano a lo largo de un eje; el cuadro del centro, en el plano de la vista.",
    "Rings: turn the shot.":
        "Anillos: giran el plano.",
    "World or Local: the arrows follow the map axes or the camera.":
        "Mundo o Local: las flechas siguen los ejes del mapa o a la cámara.",
    "The previous shot.":
        "El plano anterior.",
    "The next shot.":
        "El plano siguiente.",
    "Puts the shot where you are looking.":
        "Pone el plano donde estás mirando.",
    "Sit in the shot and fly it; <kbd>Enter</kbd> sets a key. [[k-pilot]]":
        "Siéntate en el plano y vuélalo; <kbd>Intro</kbd> pone una clave. [[k-pilot]]",
    "Look through the camera.":
        "Mirar a través de la cámara.",
    "A small picture of the camera's view in the corner.":
        "Una pequeña imagen de lo que ve la cámara, en la esquina.",
    "A new shot at your view, after the selected one.":
        "Un plano nuevo en tu vista, tras el seleccionado.",
    "Removes the shot.":
        "Quita el plano.",
    "The timeline":
        "La línea de tiempo",
    "The transport bar of the timeline.":
        "La barra de transporte de la línea de tiempo.",
    "Back to the start, play / stop - the time display next to it switches between seconds and frames.":
        "Volver al inicio, reproducir / detener; el indicador de tiempo de al lado cambia entre segundos y fotogramas.",
    "Playback":
        "Reproducción",
    "Loop, sound, follow the camera - and the speed list.":
        "Bucle, sonido, seguir a la cámara, y la lista de velocidad.",
    "Shows more or less time.":
        "Muestra más o menos tiempo.",
    "Rows":
        "Filas",
    "One row per figure, quiet rows, waits as blocks.":
        "Una fila por figura, filas inactivas, esperas como bloques.",
    "Inserts a camera shot at the playhead.":
        "Inserta un plano de cámara en el cabezal.",
    "Back to the start of the script.":
        "Vuelve al inicio del script.",
    "Plays the scene - camera, figures, sound - in game time.":
        "Reproduce la escena (cámara, figuras, sonido) en tiempo de juego.",
    "Starts again at the end.":
        "Al llegar al final, vuelve a empezar.",
    "Plays the sounds while the timeline runs - heard from the camera the view shows, like in the game. [[zeitleiste#ton]]":
        "Reproduce los sonidos mientras avanza la línea de tiempo, oídos desde la cámara que muestra la vista, como en el juego. [[zeitleiste#ton]]",
    "Moves the view along with the camera.":
        "Mueve la vista junto con la cámara.",
    "Shows more time.":
        "Muestra más tiempo.",
    "Shows less time, in more detail.":
        "Muestra menos tiempo, con más detalle.",
    "The time display switches between seconds and frames when you click it; the list next to the play button sets the speed. Drag blocks to move them in time - [[zeitleiste]].":
        "El indicador de tiempo cambia entre segundos y fotogramas al pulsarlo; la lista junto al botón de reproducir fija la velocidad. Arrastra bloques para moverlos en el tiempo: [[zeitleiste]].",
    "The model toolbar (Model mode)":
        "La barra del modelo (modo Modelo)",
    "The toolbar above the model.":
        "La barra de herramientas encima del modelo.",
    "A `.glm` from disk, a model from the archives, or a character of the game (**{{NpcBrowser}}**).":
        "Un `.glm` del disco, un modelo de los .pk3, o un personaje del juego (**{{NpcBrowser}}**).",
    "Skin and skeleton":
        "Skin y esqueleto",
    "Another `.skin`, another `.gla`.":
        "Otro `.skin`, otro `.gla`.",
    "Turntable, bones, caps, textures.":
        "Plato giratorio, huesos, tapas, texturas.",
    "The whole model in view, from the front.":
        "El modelo entero a la vista, de frente.",
    "Opens a `.glm` model from disk.":
        "Abre un modelo `.glm` del disco.",
    "Opens a model from the game's archives.":
        "Abre un modelo de los archivos .pk3 del juego.",
    "Every character of the game - click one to load it as the game shows it.":
        "Cada personaje del juego: pulsa uno para cargarlo como lo muestra el juego.",
    "Loads a `.skin` for the model.":
        "Carga un `.skin` para el modelo.",
    "Loads another skeleton / animation file (`.gla`).":
        "Carga otro esqueleto / archivo de animaciones (`.gla`).",
    "Turns the model slowly around itself.":
        "Hace girar el modelo lentamente sobre sí mismo.",
    "Shows the skeleton.":
        "Muestra el esqueleto.",
    "Shows the caps of the surfaces where limbs attach.":
        "Muestra las tapas de las superficies donde se unen las extremidades.",
    "Textures on or off.":
        "Texturas activadas o desactivadas.",
    "In the animation list: copies the animation name.":
        "En la lista de animaciones: copia el nombre de la animación.",
    "Puts `set ( SET_ANIM_BOTH, ... )` for the animation into the script. [[k-animation-abspielen]]":
        "Pone `set ( SET_ANIM_BOTH, ... )` para la animación en el script. [[k-animation-abspielen]]",
    "Everything inside an `affect` block is done by the entity named in it - Kyle walks, the door moves, the ship plays its ROFF. The script itself does **not** wait for the block: two `affect` blocks one after the other run at the same time. With `FLUSH` the entity first drops what it was doing; with `INSERT` the new commands are added to what it already does.":
        "Todo lo que hay dentro de un bloque `affect` lo hace la entidad nombrada en él: Kyle camina, la puerta se mueve, la nave reproduce su ROFF. El script en sí **no** espera al bloque: dos bloques `affect` uno tras otro se ejecutan a la vez. Con `FLUSH` la entidad deja primero lo que estaba haciendo; con `INSERT` los comandos nuevos se añaden a lo que ya hace.",
    "Where to enter what":
        "Dónde poner qué",
    "The event editor for `%s` - the numbers match the table below.":
        "El editor de eventos para `%s`: los números coinciden con la tabla de abajo.",
    "Name":
        "Nombre",
    "The name of the entity - `NPC_targetname` of a figure, `targetname` or `script_targetname` of anything else. With a map loaded, the arrow lists every name of the map.":
        "El nombre de la entidad: `NPC_targetname` de una figura, `targetname` o `script_targetname` de cualquier otra cosa. Con un mapa cargado, la flecha lista todos los nombres del mapa.",
    "Type":
        "Tipo",
    "`FLUSH` - forget earlier commands and do these. `INSERT` - add these to what the entity is already doing.":
        "`FLUSH`: olvidar los comandos anteriores y hacer estos. `INSERT`: añadir estos a lo que ya está haciendo la entidad.",
    "`affect ( \"kyle\", FLUSH )` with `set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, \"nav_kyle\" )` inside - Kyle walks to the walk target `nav_kyle`.":
        "`affect ( \"kyle\", FLUSH )` con `set ( SET_WALKING, true )` y `set ( SET_NAVGOAL, \"nav_kyle\" )` dentro: Kyle camina hasta el destino `nav_kyle`.",
    "In Classic, Graph and Tracks":
        "En Clásica, Grafo y Pistas",
    "The switch at the top of the page chooses the view. All three work on the same script - switching loses nothing.":
        "El interruptor de arriba de la página elige la vista. Las tres trabajan con el mismo script: cambiar no pierde nada.",
    "**In Classic** it is a line with a box in front: click the box to open or close the block. Commands inside are indented. A command inserted while the block is open and selected goes inside it.":
        "**En Clásica** es una línea con un cuadro delante: pulsa el cuadro para abrir o cerrar el bloque. Los comandos de dentro van sangrados. Un comando insertado mientras el bloque está abierto y seleccionado va dentro.",
    "`%s` in the script tree.":
        "`%s` en el árbol del script.",
    "**In Graph** the block and its contents are listed on the right, the contents indented. Select the block line and double-click a command in the **{{GroupEvents}}** list to put it inside.":
        "**En Grafo** el bloque y su contenido se listan a la derecha, con el contenido sangrado. Selecciona la línea del bloque y haz doble clic en un comando de la lista **{{GroupEvents}}** para meterlo dentro.",
    "`%s` in the list of the selected script node.":
        "`%s` en la lista del nodo de script seleccionado.",
    "**In Tracks** an `affect` block has no block of its own: it gives its figure a **row**, and the commands inside it are the blocks on that row. The `affect` line itself is in the **{{TracksList}}** on the right.":
        "**En Pistas** un bloque `affect` no tiene bloque propio: da a su figura una **fila**, y los comandos que contiene son los bloques de esa fila. La propia línea `affect` está en la **{{TracksList}}** de la derecha.",
    "`%s` in Tracks.":
        "`%s` en Pistas.",
    "A name that is not on the map does nothing in the game - check the arrow list.":
        "Un nombre que no está en el mapa no hace nada en el juego: revisa la lista de la flecha.",
    "Want the script to wait until a figure is done? Put the action into a `task` and use `dowait` - see [[k-tasks]].":
        "¿Quieres que el script espere hasta que una figura termine? Mete la acción en una `task` y usa `dowait`; mira [[k-tasks]].",
    "affect - commands for one entity":
        "affect: comandos para una entidad",
    "Sends the commands of its block to one entity: a figure, a door, a ship.":
        "Envía los comandos de su bloque a una entidad: una figura, una puerta, una nave.",
    "Inside an `affect` block, `flush` throws away all commands still waiting for this entity - from this script or an earlier one. It is what `affect ( ..., FLUSH )` does at its start; on its own you use it in the middle of a block to start over.":
        "Dentro de un bloque `affect`, `flush` desecha todos los comandos que aún esperan para esta entidad, de este script o de uno anterior. Es lo que hace `affect ( ..., FLUSH )` al empezar; por sí solo se usa en mitad de un bloque para empezar de nuevo.",
    "This command has no values - the editor only shows its help text. Press **{{EditorOk}}**.":
        "Este comando no tiene valores: el editor solo muestra su texto de ayuda. Pulsa **{{EditorOk}}**.",
    "`flush ( )` at the top of Kyle's block, then the new walk.":
        "`flush ( )` al principio del bloque de Kyle y luego la nueva marcha.",
    "**In Classic** it is one line of the script tree. Double-click it (or select it and press <kbd>Enter</kbd>) to open the editor; right-click for the menu; drag it to move it.":
        "**En Clásica** es una línea del árbol del script. Haz doble clic en ella (o selecciónala y pulsa <kbd>Intro</kbd>) para abrir el editor; clic derecho para el menú; arrástrala para moverla.",
    "**In Graph** the script is a node; select it and its commands are listed on the right under **{{GraphCommands}}**. Double-click the line to edit it, right-click for the menu, drop a command from the **{{GroupEvents}}** list on it to insert after it.":
        "**En Grafo** el script es un nodo; selecciónalo y sus comandos se listan a la derecha en **{{GraphCommands}}**. Doble clic en la línea para editarla, clic derecho para el menú; suelta encima un comando de la lista **{{GroupEvents}}** para insertarlo detrás.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph.":
        "**En Pistas** no ocupa tiempo propio, así que no tiene bloque: está en la **{{TracksList}}** de la derecha. Clic, doble clic y clic derecho funcionan ahí como en el Grafo.",
    "`flush` does not stop a figure that is already walking - give it a new goal or `SET_NAVGOAL` with an empty value.":
        "`flush` no detiene a una figura que ya camina: dale un objetivo nuevo o `SET_NAVGOAL` con un valor vacío.",
    "flush - forget waiting commands":
        "flush: olvidar los comandos en espera",
    "Clears every command the entity has not done yet.":
        "Borra cada comando que la entidad aún no ha hecho.",
    "`if` compares two values. If the comparison holds, the commands in its block run; otherwise they are skipped (and an `else` right after it runs instead). Usually one side is a variable read with `get` - for example *has the player seen this scene before?*":
        "`if` compara dos valores. Si se cumple la comparación, se ejecutan los comandos de su bloque; si no, se saltan (y se ejecuta en su lugar un `else` justo detrás). Normalmente un lado es una variable leída con `get`; por ejemplo *¿ha visto el jugador esta escena antes?*",
    "First value":
        "Primer valor",
    "A number, a text or an expression. **{{EditorHelper}}** → **{{EditorGet}}** reads a variable: `get ( FLOAT, \"seen\" )`.":
        "Un número, un texto o una expresión. **{{EditorHelper}}** → **{{EditorGet}}** lee una variable: `get ( FLOAT, \"visto\" )`.",
    "Comparison":
        "Comparación",
    "`=` equal, `<` less, `>` greater, `!` not equal.":
        "`=` igual, `<` menor, `>` mayor, `!` distinto.",
    "Second value":
        "Segundo valor",
    "What the first value is compared with - here `0`.":
        "Con qué se compara el primer valor; aquí `0`.",
    "`if ( get( FLOAT, \"seen\" ), =, 0 )` - true the first time, when `seen` is still 0.":
        "`if ( get( FLOAT, \"visto\" ), =, 0 )`: verdadero la primera vez, cuando `visto` aún vale 0.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The timeline plays the commands inside as if the condition were true - it cannot know the value in the game.":
        "**En Pistas** no ocupa tiempo propio, así que no tiene bloque: está en la **{{TracksList}}** de la derecha. Clic, doble clic y clic derecho funcionan ahí como en el Grafo. La línea de tiempo reproduce los comandos de dentro como si la condición fuera verdadera: no puede saber el valor en el juego.",
    "Declare the variable first with `declare` - see [[cmd-declare]] and [[k-variablen]].":
        "Declara antes la variable con `declare`; mira [[cmd-declare]] y [[k-variablen]].",
    "ICARUS only knows `=`, `<`, `>` and `!` - an unknown comparison stops the compile.":
        "ICARUS solo conoce `=`, `<`, `>` y `!`; una comparación desconocida detiene la compilación.",
    "if - only when a condition holds":
        "if: solo si se cumple una condición",
    "Runs its block only if a comparison is true.":
        "Ejecuta su bloque solo si una comparación es verdadera.",
    "`else` belongs to the `if` directly in front of it. Its block runs when that `if` did not. There must be nothing between the end of the `if` block and the `else`.":
        "`else` pertenece al `if` que tiene justo delante. Su bloque se ejecuta cuando ese `if` no se ejecutó. No puede haber nada entre el final del bloque `if` y el `else`.",
    "After `if ( get( FLOAT, \"seen\" ), =, 0 ) { ... }` an `else ( ) { print ( \"Welcome back\" ) }` greets a returning player.":
        "Tras `if ( get( FLOAT, \"visto\" ), =, 0 ) { ... }`, un `else ( ) { print ( \"Bienvenido de nuevo\" ) }` saluda al jugador que vuelve.",
    "An `else` without an `if` right before it is an error when you compile.":
        "Un `else` sin un `if` justo delante es un error al compilar.",
    "else - otherwise":
        "else: si no",
    "Runs its block when the `if` right before it was false.":
        "Ejecuta su bloque cuando el `if` justo anterior fue falso.",
    "Everything inside a `loop` block runs again and again - three times with `3`, forever with `-1`. Use it for a guard's patrol, a blinking light or a repeated shake. Inside a forever loop, put a `wait` - otherwise the game freezes.":
        "Todo lo que hay dentro de un bloque `loop` se ejecuta una y otra vez: tres veces con `3`, para siempre con `-1`. Úsalo para la patrulla de un guardia, una luz que parpadea o un temblor repetido. Dentro de un bucle infinito pon un `wait`; si no, el juego se congela.",
    "Count":
        "Repeticiones",
    "How often the block runs. `-1` means forever.":
        "Cuántas veces se ejecuta el bloque. `-1` significa para siempre.",
    "`loop ( 3 )` with a camera shake and `wait ( 1000 )` inside - three shakes, a second apart.":
        "`loop ( 3 )` con un temblor de cámara y `wait ( 1000 )` dentro: tres temblores, separados un segundo.",
    "**In Tracks** the commands inside the loop are blocks, repeated as often as the loop runs (a forever loop is shown a few times). The `loop` line itself is in the **{{TracksList}}**.":
        "**En Pistas** los comandos dentro del bucle son bloques, repetidos tantas veces como se ejecuta el bucle (un bucle infinito se muestra unas cuantas veces). La propia línea `loop` está en la **{{TracksList}}**.",
    "A forever loop in the intro never ends - the script never gets past it. Put it in an `affect` block of its own or in a separate script.":
        "Un bucle infinito en la introducción nunca termina: el script nunca pasa de él. Ponlo en un bloque `affect` propio o en un script aparte.",
    "loop - repeat a block":
        "loop: repetir un bloque",
    "Repeats the commands of its block a number of times, or forever.":
        "Repite los comandos de su bloque un número de veces, o para siempre.",
    "`run` starts the named script right away, and this script goes on at once. The script is named like the file below `scripts/`, without `.txt` or `.ibi`.":
        "`run` inicia enseguida el script nombrado, y este script sigue al momento. El script se nombra como el archivo bajo `scripts/`, sin `.txt` ni `.ibi`.",
    "The path below `scripts/` - for example `taspir_tut/kyle_spawn`. **{{EditorBrowse}}** picks a file of the game, **{{EditorPlay}}** opens it.":
        "La ruta bajo `scripts/`, por ejemplo `taspir_tut/kyle_spawn`. **{{EditorBrowse}}** elige un archivo del juego, **{{EditorPlay}}** lo abre.",
    "`run ( \"taspir_tut/kyle_spawn\" )` starts Kyle's spawn script.":
        "`run ( \"taspir_tut/kyle_spawn\" )` inicia el script de aparición de Kyle.",
    "To wait until the other script reaches a point, let it `signal` and use `waitsignal` here - see [[k-signale]].":
        "Para esperar a que el otro script llegue a un punto, haz que envíe `signal` y usa aquí `waitsignal`; mira [[k-signale]].",
    "run - start another script":
        "run: iniciar otro script",
    "The script stops here for the given time; then it goes on with the next command. 1000 is one second. Most of a cutscene's timing is waits: the camera starts a move, the script waits until the move is done, then the next shot.":
        "El script se detiene aquí el tiempo indicado; luego sigue con el comando siguiente. 1000 es un segundo. Casi toda la sincronización de una cinemática son esperas: la cámara empieza un movimiento, el script espera a que termine y luego viene el plano siguiente.",
    "Time":
        "Tiempo",
    "Milliseconds: `1000` = one second. **{{EditorHelper}}** → **{{EditorRnd}}** builds `random ( 1000, 3000 )` for a different time each run.":
        "Milisegundos: `1000` = un segundo. **{{EditorHelper}}** → **{{EditorRnd}}** construye `random ( 1000, 3000 )` para un tiempo distinto en cada ejecución.",
    "`wait ( 2000 )` - two seconds, then the camera is switched off.":
        "`wait ( 2000 )`: dos segundos y luego se apaga la cámara.",
    "**In Tracks** it is a block on the timeline, as long as it takes. Click it to select its line, double-click to edit it, drag its right edge to change the time, right-click for the menu. Waits are on the **Script** row.":
        "**En Pistas** es un bloque en la línea de tiempo, tan largo como dura. Púlsalo para seleccionar su línea, doble clic para editarlo, arrastra su borde derecho para cambiar el tiempo, clic derecho para el menú. Las esperas están en la fila **Script**.",
    "Inside an `affect` block, a `wait` delays only that entity - the script goes on.":
        "Dentro de un bloque `affect`, un `wait` retrasa solo a esa entidad; el script sigue.",
    "To wait until a figure has arrived, a fixed time is a guess - use a `task` and `dowait` ([[cmd-dowait]]).":
        "Para esperar a que una figura haya llegado, un tiempo fijo es adivinar: usa una `task` y `dowait` ([[cmd-dowait]]).",
    "wait - pause for a time":
        "wait: pausa de un tiempo",
    "Pauses the script for a number of milliseconds.":
        "Detiene el script un número de milisegundos.",
    "With a task name instead of a time, `wait` pauses until that task is done - for a walk: until the figure has arrived. Together with `do` it lets a figure do two things at once and still wait for one of them.":
        "Con un nombre de tarea en vez de un tiempo, `wait` espera hasta que esa tarea termine; para una marcha: hasta que la figura haya llegado. Junto con `do` deja que una figura haga dos cosas a la vez y aun así espere a una de ellas.",
    "The event editor for `wait` with a task name.":
        "El editor de eventos para `wait` con un nombre de tarea.",
    "Task":
        "Tarea",
    "The name of a `task` of this entity that was started with `do`.":
        "El nombre de una `task` de esta entidad que se inició con `do`.",
    "`do ( \"walk\" )`, then something else, then `wait ( \"walk\" )` - the script goes on when Kyle has arrived.":
        "`do ( \"walk\" )`, luego otra cosa, luego `wait ( \"walk\" )`: el script sigue cuando Kyle ha llegado.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}** on the right. You see what it does on the figure's row: what follows it starts where the task ends.":
        "**En Pistas** no tiene bloque propio: está en la **{{TracksList}}** de la derecha. Lo que hace se ve en la fila de la figura: lo que le sigue empieza donde termina la tarea.",
    "`dowait` is the short form of `do` + `wait` - [[cmd-dowait]].":
        "`dowait` es la forma corta de `do` + `wait`: [[cmd-dowait]].",
    "wait - until a task is done":
        "wait: hasta que termine una tarea",
    "Pauses until a task started with `do` is finished.":
        "Espera hasta que termine una tarea iniciada con `do`.",
    "Two scripts can wait for each other: this one stops at `waitsignal ( \"door_open\" )` until another script reaches `signal ( \"door_open\" )`. Only one script can wait for a given name at a time.":
        "Dos scripts pueden esperarse: este se detiene en `waitsignal ( \"door_open\" )` hasta que otro script llega a `signal ( \"door_open\" )`. Solo un script puede esperar un nombre dado a la vez.",
    "Signal":
        "Señal",
    "Any name - it must be spelled exactly like the one in `signal`.":
        "Cualquier nombre: debe escribirse exactamente igual que en `signal`.",
    "The intro waits with `waitsignal ( \"door_open\" )` until the door script says `signal ( \"door_open\" )`.":
        "La introducción espera con `waitsignal ( \"door_open\" )` hasta que el script de la puerta dice `signal ( \"door_open\" )`.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The timeline cannot know how long it waits in the game - everything after it follows directly.":
        "**En Pistas** no ocupa tiempo propio, así que no tiene bloque: está en la **{{TracksList}}** de la derecha. Clic, doble clic y clic derecho funcionan ahí como en el Grafo. La línea de tiempo no puede saber cuánto espera en el juego: todo lo que va después sigue directamente.",
    "See [[k-signale]] for both sides together.":
        "Mira [[k-signale]] para los dos lados juntos.",
    "waitsignal - wait for another script":
        "waitsignal: esperar a otro script",
    "Pauses until some other script calls `signal` with the same name.":
        "Espera hasta que otro script llama a `signal` con el mismo nombre.",
    "`signal` is the other half of `waitsignal`: when this line runs, the script waiting for the same name continues.":
        "`signal` es la otra mitad de `waitsignal`: cuando se ejecuta esta línea, el script que espera el mismo nombre continúa.",
    "The same name as in `waitsignal`.":
        "El mismo nombre que en `waitsignal`.",
    "The door script: `use ( \"bldg2_ext_door\" )`, `wait ( 1500 )`, `signal ( \"door_open\" )`.":
        "El script de la puerta: `use ( \"bldg2_ext_door\" )`, `wait ( 1500 )`, `signal ( \"door_open\" )`.",
    "**In Tracks** it takes no time: it is a short mark on the **Script** row at the moment it runs. Click it to select its line, double-click to edit it, right-click for the menu. It is also in the **{{TracksList}}** on the right.":
        "**En Pistas** no ocupa tiempo: es una marca corta en la fila **Script** en el momento en que se ejecuta. Púlsala para seleccionar su línea, doble clic para editarla, clic derecho para el menú. También está en la **{{TracksList}}** de la derecha.",
    "A signal that nobody waits for is lost - the waiting script must already be at its `waitsignal`.":
        "Una señal que nadie espera se pierde: el script que espera ya debe estar en su `waitsignal`.",
    "signal - wake a waiting script":
        "signal: despertar a un script que espera",
    "Lets the script that waits for this name go on.":
        "Deja continuar al script que espera este nombre.",
    "Inside a figure's `affect` block, `sound` makes that figure speak or make a noise. On the channel `CHAN_VOICE` the mouth moves with the line, and the subtitle of the game is shown.":
        "Dentro del bloque `affect` de una figura, `sound` hace que esa figura hable o haga un ruido. En el canal `CHAN_VOICE` la boca se mueve con la frase y se muestra el subtítulo del juego.",
    "Channel":
        "Canal",
    "`CHAN_VOICE` for dialogue (lips move), `CHAN_AUTO` or `CHAN_BODY` for other sounds.":
        "`CHAN_VOICE` para el diálogo (se mueven los labios), `CHAN_AUTO` o `CHAN_BODY` para otros sonidos.",
    "File":
        "Archivo",
    "The sound below `sound/` - **{{EditorBrowse}}** picks one from the game, **{{EditorPlay}}** plays it.":
        "El sonido bajo `sound/`: **{{EditorBrowse}}** elige uno del juego, **{{EditorPlay}}** lo reproduce.",
    "`sound ( CHAN_VOICE, \"sound/chars/kyle/01kyk001.mp3\" )` in Kyle's block.":
        "`sound ( CHAN_VOICE, \"sound/chars/kyle/01kyk001.mp3\" )` en el bloque de Kyle.",
    "**In Tracks** a `CHAN_VOICE` sound is a block on the **Dialogue** row, as long as the file plays. Double-click it to edit it.":
        "**En Pistas** un sonido `CHAN_VOICE` es un bloque en la fila **Diálogo**, tan largo como suena el archivo. Doble clic para editarlo.",
    "Dialogue: [[k-dialog]].":
        "Diálogo: [[k-dialog]].",
    "sound - play a sound or a line":
        "sound: reproducir un sonido o una frase",
    "Plays a sound on a channel of the entity.":
        "Reproduce un sonido en un canal de la entidad.",
    "`move` slides the entity of the current `affect` block to a new position - and, if given, to new angles - over the given time. The **Animate** window writes these for you from keys you set in the view.":
        "`move` desliza la entidad del bloque `affect` actual a una posición nueva (y, si se indica, a ángulos nuevos) en el tiempo indicado. La ventana **Animar** los escribe por ti a partir de las claves que pones en la vista.",
    "Position":
        "Posición",
    "Where it goes: three numbers `x y z`, relative to where the entity was built in the map.":
        "Adónde va: tres números `x y z`, relativos a donde se construyó la entidad en el mapa.",
    "Angles":
        "Ángulos",
    "Pitch, yaw and roll at the end - `0 0 0` keeps them.":
        "Cabeceo, guiñada y alabeo al final; `0 0 0` los mantiene.",
    "Milliseconds for the whole move.":
        "Milisegundos para todo el movimiento.",
    "`move ( < 0 0 128 >, < 0 0 0 >, 2000 )` in the door's block - the door rises 128 units in two seconds.":
        "`move ( < 0 0 128 >, < 0 0 0 >, 2000 )` en el bloque de la puerta: la puerta sube 128 unidades en dos segundos.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The view plays the movement, but a door or a platform has no row of its own in Tracks.":
        "**En Pistas** no ocupa tiempo propio, así que no tiene bloque: está en la **{{TracksList}}** de la derecha. Clic, doble clic y clic derecho funcionan ahí como en el Grafo. La vista reproduce el movimiento, pero una puerta o una plataforma no tienen fila propia en Pistas.",
    "A door that opens: [[k-tuer]].":
        "Una puerta que se abre: [[k-tuer]].",
    "move - move an entity":
        "move: mover una entidad",
    "Moves a door, a platform or a ship to a position in so many milliseconds.":
        "Mueve una puerta, una plataforma o una nave a una posición en tantos milisegundos.",
    "`rotate` turns the entity of the current `affect` block to the given angles (pitch, yaw, roll) over the given time.":
        "`rotate` gira la entidad del bloque `affect` actual a los ángulos indicados (cabeceo, guiñada, alabeo) en el tiempo indicado.",
    "Pitch, yaw and roll - `0 90 0` turns it a quarter round.":
        "Cabeceo, guiñada y alabeo: `0 90 0` la gira un cuarto de vuelta.",
    "Milliseconds.":
        "Milisegundos.",
    "`rotate ( < 0 90 0 >, 1000 )` in the door's block.":
        "`rotate ( < 0 90 0 >, 1000 )` en el bloque de la puerta.",
    "rotate - turn an entity":
        "rotate: girar una entidad",
    "Turns the entity to angles in so many milliseconds.":
        "Gira la entidad a unos ángulos en tantos milisegundos.",
    "`use` triggers the entity: a door opens, a trigger fires, a spawner spawns its figure, a script runner starts its script.":
        "`use` activa la entidad: una puerta se abre, un trigger se dispara, un spawner hace aparecer su figura, un ejecutor de scripts inicia su script.",
    "The `targetname` of the entity - the arrow lists the names of the map.":
        "El `targetname` de la entidad: la flecha lista los nombres del mapa.",
    "`use ( \"bldg2_ext_door\" )` opens the door of the building.":
        "`use ( \"bldg2_ext_door\" )` abre la puerta del edificio.",
    "The second `use` in the list takes an expression: `use ( get( STRING, \"door\" ) )` uses whatever name the variable holds.":
        "El segundo `use` de la lista acepta una expresión: `use ( get( STRING, \"puerta\" ) )` usa el nombre que contenga la variable.",
    "use - use an entity":
        "use: usar una entidad",
    "Uses an entity by name, as if the player pressed it.":
        "Usa una entidad por su nombre, como si el jugador la pulsara.",
    "A figure dies (and its `deathscript` runs), a breakable object breaks.":
        "Una figura muere (y se ejecuta su `deathscript`), un objeto rompible se rompe.",
    "The name of the entity.":
        "El nombre de la entidad.",
    "To make a figure just disappear, use `remove`.":
        "Para que una figura simplemente desaparezca, usa `remove`.",
    "kill - kill an entity":
        "kill: matar una entidad",
    "Use it to clean up after a cutscene: figures that should not be there when the player gets control.":
        "Úsalo para limpiar tras una cinemática: figuras que no deberían estar cuando el jugador recupera el control.",
    "remove - take an entity out of the game":
        "remove: sacar una entidad del juego",
    "Removes the entity with this name - no death, it is simply gone.":
        "Quita la entidad con este nombre: sin muerte, simplemente desaparece.",
    "A line of text for the player - a time jump, a hint, a place name.":
        "Una línea de texto para el jugador: un salto en el tiempo, una pista, el nombre de un lugar.",
    "Text":
        "Texto",
    "The text to show.":
        "El texto que mostrar.",
    "Text in the game's string files (`@@`) is translated by the game.":
        "El texto de los archivos de cadenas del juego (`@@`) lo traduce el juego.",
    "print - text on the screen":
        "print: texto en la pantalla",
    "`rem` keeps a note for you or the next person: what this part of the script does. **{{ActRem}}** turns any line into a comment and back.":
        "`rem` guarda una nota para ti o para la siguiente persona: qué hace esta parte del script. **{{ActRem}}** convierte cualquier línea en comentario y al revés.",
    "Comment":
        "Comentario",
    "Any text.":
        "Cualquier texto.",
    "rem - a comment":
        "rem: un comentario",
    "A note in the script - it does nothing in the game.":
        "Una nota en el script: no hace nada en el juego.",
    "A variable remembers something during the level - for example whether a scene was already seen. Declare it once, in a script that runs once. It starts at 0 or empty. A map can hold 16 variables.":
        "Una variable recuerda algo durante el nivel, por ejemplo si ya se vio una escena. Declárala una vez, en un script que se ejecute una vez. Empieza en 0 o vacía. Un mapa puede tener 16 variables.",
    "`FLOAT` for numbers, `STRING` for text, `VECTOR` for three numbers.":
        "`FLOAT` para números, `STRING` para texto, `VECTOR` para tres números.",
    "Any name - you use it again in `set`, `get` and `free`.":
        "Cualquier nombre: lo vuelves a usar en `set`, `get` y `free`.",
    "See [[k-variablen]] for declare, if, set and free together.":
        "Mira [[k-variablen]] para declare, if, set y free juntos.",
    "declare - create a variable":
        "declare: crear una variable",
    "Declares a variable that scripts can set and read.":
        "Declara una variable que los scripts pueden fijar y leer.",
    "Only 16 variables fit; `free` gives one back when it is no longer needed.":
        "Solo caben 16 variables; `free` devuelve una cuando ya no hace falta.",
    "The name used in `declare`.":
        "El nombre usado en `declare`.",
    "free - give a variable back":
        "free: devolver una variable",
    "Frees a declared variable so the name and its place can be used again.":
        "Libera una variable declarada para que el nombre y su plaza se puedan volver a usar.",
    "`set` with a `SET_` field is the most used command of all. Inside an `affect` block it changes that entity: `SET_NAVGOAL` makes a figure walk to a walk target, `SET_WALKING` makes it walk instead of run, `SET_LOOK_TARGET` turns its head, `SET_ANIM_BOTH` plays an animation. Every field is in [[befehle#set]].":
        "`set` con un campo `SET_` es el comando más usado de todos. Dentro de un bloque `affect` cambia esa entidad: `SET_NAVGOAL` hace que una figura camine hasta un destino, `SET_WALKING` hace que camine en vez de correr, `SET_LOOK_TARGET` gira su cabeza, `SET_ANIM_BOTH` reproduce una animación. Cada campo está en [[befehle#set]].",
    "Which value - a `SET_` name from the list. Type part of it to filter.":
        "Qué valor: un nombre `SET_` de la lista. Escribe parte de él para filtrar.",
    "The new value. Its kind depends on the field: `true` / `false`, a number, a name from the map, an animation.":
        "El valor nuevo. Su tipo depende del campo: `true` / `false`, un número, un nombre del mapa, una animación.",
    "`set ( SET_WALKING, true )` and `set ( SET_NAVGOAL, \"nav_kyle\" )` - Kyle walks to `nav_kyle`.":
        "`set ( SET_WALKING, true )` y `set ( SET_NAVGOAL, \"nav_kyle\" )`: Kyle camina hasta `nav_kyle`.",
    "**In Tracks** it shows in the row of the figure it belongs to (the name in its `affect`), as long as it takes. Double-click the block to edit it, right-click for the menu. A walk (`SET_NAVGOAL`) lasts until the figure arrives; most other fields take no time.":
        "**En Pistas** aparece en la fila de la figura a la que pertenece (el nombre de su `affect`), tan largo como dura. Doble clic en el bloque para editarlo, clic derecho para el menú. Una marcha (`SET_NAVGOAL`) dura hasta que la figura llega; la mayoría de los demás campos no ocupan tiempo.",
    "A field marked `*` in its help text (like `SET_NAVGOAL`) can be waited for with `dowait`.":
        "Un campo marcado con `*` en su texto de ayuda (como `SET_NAVGOAL`) se puede esperar con `dowait`.",
    "A value that is not in the list is allowed (mods bring their own) but marked **unknown** - check it for a typo.":
        "Se permite un valor que no esté en la lista (los mods traen los suyos), pero se marca como **desconocido**: revisa si hay una errata.",
    "set - change a field of an entity":
        "set: cambiar un campo de una entidad",
    "Changes a value of the entity: how it walks, where it goes, what it looks at, its animation, its health ...":
        "Cambia un valor de la entidad: cómo camina, adónde va, qué mira, su animación, su salud ...",
    "The second `set` in the list sets a variable by its name instead of an entity field.":
        "El segundo `set` de la lista fija una variable por su nombre en vez de un campo de entidad.",
    "The event editor for `set` with a variable.":
        "El editor de eventos para `set` con una variable.",
    "Variable":
        "Variable",
    "The new value, for example `1`.":
        "El valor nuevo, por ejemplo `1`.",
    "`set ( \"seen\", \"1\" )` at the end of the scene - next time the `if` is false.":
        "`set ( \"visto\", \"1\" )` al final de la escena: la próxima vez el `if` es falso.",
    "set - change a variable":
        "set: cambiar una variable",
    "Gives a declared variable a new value.":
        "Da un valor nuevo a una variable declarada.",
    "A `task` does nothing where it stands. It gives a name to its block, so that `do` can start it and `wait` or `dowait` can wait until it is finished - the only way to wait until a figure has really arrived somewhere.":
        "Una `task` no hace nada donde está. Da un nombre a su bloque, para que `do` pueda iniciarlo y `wait` o `dowait` puedan esperar a que termine: la única forma de esperar hasta que una figura haya llegado de verdad a algún sitio.",
    "Any name, unique inside this `affect` block - for example `walk`.":
        "Cualquier nombre, único dentro de este bloque `affect`; por ejemplo `walk`.",
    "`task ( \"walk\" )` with `SET_WALKING` and `SET_NAVGOAL` inside, then `dowait ( \"walk\" )`.":
        "`task ( \"walk\" )` con `SET_WALKING` y `SET_NAVGOAL` dentro, luego `dowait ( \"walk\" )`.",
    "**In Tracks** it takes no time of its own, so it has no block - it stands in the **{{TracksList}}** on the right. Click, double-click and right-click work there as in the Graph. The commands inside appear on the figure's row when the task is started.":
        "**En Pistas** no ocupa tiempo propio, así que no tiene bloque: está en la **{{TracksList}}** de la derecha. Clic, doble clic y clic derecho funcionan ahí como en el Grafo. Los comandos de dentro aparecen en la fila de la figura cuando se inicia la tarea.",
    "See [[k-tasks]].":
        "Mira [[k-tasks]].",
    "task - a named group of commands":
        "task: un grupo de comandos con nombre",
    "Declares a group of commands that runs later, with `do` or `dowait`.":
        "Declara un grupo de comandos que se ejecuta más tarde, con `do` o `dowait`.",
    "`do` starts the named task, and the next command follows right away - so a figure can walk and look around at the same time.":
        "`do` inicia la tarea nombrada y el comando siguiente sigue enseguida; así una figura puede caminar y mirar alrededor a la vez.",
    "The name of a `task` in this block.":
        "El nombre de una `task` de este bloque.",
    "`do ( \"walk\" )`, `do ( \"look\" )`, then `wait ( \"walk\" )`.":
        "`do ( \"walk\" )`, `do ( \"look\" )`, luego `wait ( \"walk\" )`.",
    "do - start a task":
        "do: iniciar una tarea",
    "`dowait` is `do` and `wait` in one line. For a walk task it waits until the figure has arrived - everything after it happens when the figure stands.":
        "`dowait` es `do` y `wait` en una línea. Para una tarea de marcha espera hasta que la figura ha llegado: todo lo que va después ocurre cuando la figura está parada.",
    "`dowait ( \"walk\" )`, then `set ( SET_LOOK_TARGET, \"rosh\" )` - Kyle looks at Rosh when he has arrived.":
        "`dowait ( \"walk\" )`, luego `set ( SET_LOOK_TARGET, \"rosh\" )`: Kyle mira a Rosh cuando ha llegado.",
    "dowait - start a task and wait for it":
        "dowait: iniciar una tarea y esperarla",
    "Starts a task and waits until it is finished.":
        "Inicia una tarea y espera hasta que termine.",
    "A ROFF file is a movement over time - a ship that lifts off and flies away, a falling pillar. **Animate** writes one for you from keys.":
        "Un archivo ROFF es un movimiento en el tiempo: una nave que despega y se aleja volando, una columna que cae. **Animar** escribe uno por ti a partir de claves.",
    "The ROFF below `scripts/` - without `.rof`.":
        "El ROFF bajo `scripts/`, sin `.rof`.",
    "`play ( PLAY_ROFF, \"taspir_tut/roffs/ravensClaw\" )` in the ship's block.":
        "`play ( PLAY_ROFF, \"taspir_tut/roffs/ravensClaw\" )` en el bloque de la nave.",
    "**In Tracks** it shows in the row of the figure it belongs to (the name in its `affect`), as long as it takes. Double-click the block to edit it, right-click for the menu.":
        "**En Pistas** aparece en la fila de la figura a la que pertenece (el nombre de su `affect`), tan largo como dura. Doble clic en el bloque para editarlo, clic derecho para el menú.",
    "Make and change ROFF files: [[k-roff-bearbeiten]].":
        "Crear y cambiar archivos ROFF: [[k-roff-bearbeiten]].",
    "play - play a ROFF file":
        "play: reproducir un archivo ROFF",
    "Plays a recorded movement (ROFF) on the entity of the current `affect` block.":
        "Reproduce un movimiento grabado (ROFF) en la entidad del bloque `affect` actual.",
    "From here on the player's view is replaced by the script's camera, the HUD disappears and the player cannot move. Every cutscene starts with it.":
        "A partir de aquí la vista del jugador se sustituye por la cámara del script, el HUD desaparece y el jugador no puede moverse. Cada cinemática empieza con esto.",
    "The event editor for `camera ( ENABLE )`.":
        "El editor de eventos para `camera ( ENABLE )`.",
    "Camera command":
        "Comando de cámara",
    "The first field chooses what the camera does - here `ENABLE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `ENABLE`. Cambiarlo cambia los campos de abajo.",
    "`camera ( ENABLE )` - the first camera line of the intro.":
        "`camera ( ENABLE )`: la primera línea de cámara de la introducción.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}**. The **Camera** row starts with the first `MOVE` after it.":
        "**En Pistas** no tiene bloque propio: está en la **{{TracksList}}**. La fila **Cámara** empieza con el primer `MOVE` posterior.",
    "Without a `MOVE` and `PAN` right after it, the camera stays where the player was.":
        "Sin un `MOVE` y un `PAN` justo después, la cámara se queda donde estaba el jugador.",
    "camera ENABLE - the cutscene starts":
        "camera ENABLE: empieza la cinemática",
    "Puts the game into camera mode.":
        "Pone el juego en modo cámara.",
    "The player gets control back, the HUD returns. The last line of every cutscene.":
        "El jugador recupera el control, el HUD vuelve. La última línea de cada cinemática.",
    "The event editor for `camera ( DISABLE )`.":
        "El editor de eventos para `camera ( DISABLE )`.",
    "The first field chooses what the camera does - here `DISABLE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `DISABLE`. Cambiarlo cambia los campos de abajo.",
    "`camera ( DISABLE )` after the last wait.":
        "`camera ( DISABLE )` tras la última espera.",
    "**In Tracks** it has no block of its own - it is in the **{{TracksList}}**. The last shot on the **Camera** row ends here.":
        "**En Pistas** no tiene bloque propio: está en la **{{TracksList}}**. Aquí termina el último plano de la fila **Cámara**.",
    "Forgetting it leaves the player stuck in the cutscene.":
        "Olvidarlo deja al jugador atrapado en la cinemática.",
    "camera DISABLE - back to the game":
        "camera DISABLE: de vuelta al juego",
    "Takes the game out of camera mode.":
        "Saca el juego del modo cámara.",
    "With time `0` the camera jumps there (a cut); with a time it flies there. Together with `PAN` it makes a shot.":
        "Con tiempo `0` la cámara salta allí (un corte); con un tiempo vuela hasta allí. Junto con `PAN` forma un plano.",
    "The event editor for `camera ( MOVE )`.":
        "El editor de eventos para `camera ( MOVE )`.",
    "The first field chooses what the camera does - here `MOVE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `MOVE`. Cambiarlo cambia los campos de abajo.",
    "`x y z` - or **{{EditorHelper}}** → **{{EditorTag}}** for a camera mark (`ref_tag`) on the map: `tag ( \"cam1\", ORIGIN )`.":
        "`x y z`, o **{{EditorHelper}}** → **{{EditorTag}}** para una marca de cámara (`ref_tag`) del mapa: `tag ( \"cam1\", ORIGIN )`.",
    "Milliseconds for the flight; `0` jumps.":
        "Milisegundos para el vuelo; `0` salta.",
    "`camera ( MOVE, < -750 -2700 1000 >, 3000 )` - three seconds' flight.":
        "`camera ( MOVE, < -750 -2700 1000 >, 3000 )`: tres segundos de vuelo.",
    "**In Tracks** every `MOVE` starts a new shot on the **Camera** row; a move with time is marked *moving*. Double-click the shot to edit its `MOVE`.":
        "**En Pistas** cada `MOVE` empieza un plano nuevo en la fila **Cámara**; un movimiento con tiempo se marca como *en movimiento*. Doble clic en el plano para editar su `MOVE`.",
    "**Insert camera here** in the timeline writes `MOVE` and `PAN` from the view - [[k-kamera-ansicht]].":
        "**Insertar cámara aquí** en la línea de tiempo escribe `MOVE` y `PAN` desde la vista: [[k-kamera-ansicht]].",
    "camera MOVE - place or fly the camera":
        "camera MOVE: colocar o volar la cámara",
    "Moves the camera to a position - at once or over time.":
        "Mueve la cámara a una posición, enseguida o con el tiempo.",
    "`PAN` sets where the camera looks: pitch (up/down), yaw (left/right), roll. With a time it turns slowly.":
        "`PAN` fija adónde mira la cámara: cabeceo (arriba/abajo), guiñada (izquierda/derecha), alabeo. Con un tiempo gira despacio.",
    "The event editor for `camera ( PAN )`.":
        "El editor de eventos para `camera ( PAN )`.",
    "The first field chooses what the camera does - here `PAN`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `PAN`. Cambiarlo cambia los campos de abajo.",
    "Pitch, yaw, roll the camera ends at.":
        "Cabeceo, guiñada y alabeo con los que termina la cámara.",
    "Direction":
        "Dirección",
    "Which way to turn - `0 0 0` takes the shortest way.":
        "Hacia dónde girar; `0 0 0` toma el camino más corto.",
    "Milliseconds; `0` turns at once.":
        "Milisegundos; `0` gira enseguida.",
    "**In Tracks** the camera row shows the shots - from one `MOVE` to the next. This command is listed in the **{{TracksList}}**; the timeline under the map shows it in its own camera row.":
        "**En Pistas** la fila de la cámara muestra los planos, de un `MOVE` al siguiente. Este comando se lista en la **{{TracksList}}**; la línea de tiempo bajo el mapa lo muestra en su propia fila de cámara.",
    "camera PAN - turn the camera":
        "camera PAN: girar la cámara",
    "Turns the camera to angles - at once or over time.":
        "Gira la cámara a unos ángulos, enseguida o con el tiempo.",
    "A smaller field of view zooms in, a larger one zooms out. Normal is 90 (after `ENABLE`); 80 is the player's own view before `ENABLE` and after `DISABLE`.":
        "Un campo de visión menor acerca, uno mayor aleja. Lo normal es 90 (tras `ENABLE`); 80 es la propia vista del jugador antes de `ENABLE` y tras `DISABLE`.",
    "The event editor for `camera ( ZOOM )`.":
        "El editor de eventos para `camera ( ZOOM )`.",
    "The first field chooses what the camera does - here `ZOOM`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `ZOOM`. Cambiarlo cambia los campos de abajo.",
    "Field of view":
        "Campo de visión",
    "Degrees - Raven's scripts use 6.3 (far zoomed in) to 110 (wide). Normal: 90.":
        "Grados: los scripts de Raven usan de 6,3 (zoom muy cercano) a 110 (amplio). Normal: 90.",
    "Milliseconds to get there.":
        "Milisegundos para llegar.",
    "`camera ( ZOOM, 40, 2000 )` - zoom in over two seconds.":
        "`camera ( ZOOM, 40, 2000 )`: acercar en dos segundos.",
    "How wide a zoom looks also depends on the game screen (aspect ratio, cg_fovAspectAdjust): [[kamera#vorschau]].":
        "Lo amplio que se ve un zoom también depende de la pantalla del juego (relación de aspecto, cg_fovAspectAdjust): [[kamera#vorschau]].",
    "camera ZOOM - change the field of view":
        "camera ZOOM: cambiar el campo de visión",
    "Changes how much the camera sees.":
        "Cambia cuánto ve la cámara.",
    "Tilts the picture to the side - for a dizzy or dramatic shot.":
        "Inclina la imagen hacia un lado, para un plano mareante o dramático.",
    "The event editor for `camera ( ROLL )`.":
        "El editor de eventos para `camera ( ROLL )`.",
    "The first field chooses what the camera does - here `ROLL`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `ROLL`. Cambiarlo cambia los campos de abajo.",
    "Angle":
        "Ángulo",
    "The roll angle the camera ends at (not added to the current roll).":
        "El ángulo de alabeo con el que termina la cámara (no se suma al alabeo actual).",
    "Milliseconds; with a time it takes the shorter way there.":
        "Milisegundos; con un tiempo toma el camino más corto hasta allí.",
    "camera ROLL - tilt the picture":
        "camera ROLL: inclinar la imagen",
    "Rolls the camera to an angle.":
        "Alabea la cámara hasta un ángulo.",
    "A track is a chain of path corners in the map. The camera rides along it at a speed.":
        "Un recorrido es una cadena de path corners en el mapa. La cámara viaja por él a una velocidad.",
    "The event editor for `camera ( TRACK )`.":
        "El editor de eventos para `camera ( TRACK )`.",
    "The first field chooses what the camera does - here `TRACK`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `TRACK`. Cambiarlo cambia los campos de abajo.",
    "Track":
        "Recorrido",
    "The `targetname` of the first path corner.":
        "El `targetname` del primer path corner.",
    "Speed":
        "Velocidad",
    "Units per second.":
        "Unidades por segundo.",
    "Lerp":
        "Interpolar",
    "`1`: glide to the start of the track first; `0`: jump there.":
        "`1`: deslizarse primero hasta el inicio del recorrido; `0`: saltar allí.",
    "Camera paths: [[k-blickpunkte]].":
        "Trayectos de cámara: [[k-blickpunkte]].",
    "camera TRACK - ride a camera path":
        "camera TRACK: viajar por un recorrido de cámara",
    "Puts the camera on a track of the map and moves along it.":
        "Pone la cámara en un recorrido del mapa y la mueve a lo largo de él.",
    "The camera keeps looking at every entity whose `cameraGroup` matches - it turns as they move.":
        "La cámara sigue mirando a cada entidad cuyo `cameraGroup` coincide: gira a medida que se mueven.",
    "The event editor for `camera ( FOLLOW )`.":
        "El editor de eventos para `camera ( FOLLOW )`.",
    "The first field chooses what the camera does - here `FOLLOW`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `FOLLOW`. Cambiarlo cambia los campos de abajo.",
    "Camera group":
        "Grupo de cámara",
    "The `cameraGroup` value of the entities to follow (or a name).":
        "El valor `cameraGroup` de las entidades que seguir (o un nombre).",
    "Angle speed":
        "Velocidad angular",
    "How fast the camera may turn.":
        "Lo rápido que puede girar la cámara.",
    "`1`: turn smoothly to the start angle; `0`: snap.":
        "`1`: girar suavemente al ángulo inicial; `0`: de golpe.",
    "See [[k-kamera-folgen]].":
        "Mira [[k-kamera-folgen]].",
    "camera FOLLOW - keep figures in view":
        "camera FOLLOW: mantener a la vista a las figuras",
    "Turns the camera to follow entities of a camera group.":
        "Gira la cámara para seguir a las entidades de un grupo de cámara.",
    "Used with `FOLLOW`: the camera also moves to stay this far from the group.":
        "Se usa con `FOLLOW`: la cámara también se mueve para quedarse a esta distancia del grupo.",
    "The event editor for `camera ( DISTANCE )`.":
        "El editor de eventos para `camera ( DISTANCE )`.",
    "The first field chooses what the camera does - here `DISTANCE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `DISTANCE`. Cambiarlo cambia los campos de abajo.",
    "Distance":
        "Distancia",
    "Units.":
        "Unidades.",
    "`1`: move smoothly; `0`: jump.":
        "`1`: moverse suavemente; `0`: saltar.",
    "camera DISTANCE - keep a distance":
        "camera DISTANCE: guardar una distancia",
    "Keeps the camera at a distance from its camera group.":
        "Mantiene la cámara a una distancia de su grupo de cámara.",
    "Black at the end of a scene, white for a flash, or back from black at the start.":
        "Negro al final de una escena, blanco para un destello, o volver del negro al principio.",
    "The event editor for `camera ( FADE )`.":
        "El editor de eventos para `camera ( FADE )`.",
    "The first field chooses what the camera does - here `FADE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `FADE`. Cambiarlo cambia los campos de abajo.",
    "Start colour":
        "Color inicial",
    "Red, green, blue from 0 to 1 - `0 0 0` is black.":
        "Rojo, verde, azul de 0 a 1; `0 0 0` es negro.",
    "Start opacity":
        "Opacidad inicial",
    "0 = clear picture, 1 = full colour.":
        "0 = imagen limpia, 1 = color total.",
    "End colour":
        "Color final",
    "Red, green, blue at the end.":
        "Rojo, verde, azul al final.",
    "End opacity":
        "Opacidad final",
    "0 to 1.":
        "De 0 a 1.",
    "`camera ( FADE, < 0 0 0 >, 0, < 0 0 0 >, 1, 1500 )` - fade to black in 1.5 seconds.":
        "`camera ( FADE, < 0 0 0 >, 0, < 0 0 0 >, 1, 1500 )`: fundido a negro en 1,5 segundos.",
    "Zoom, fade and shake: [[k-zoom-blende]].":
        "Zoom, fundido y temblor: [[k-zoom-blende]].",
    "camera FADE - fade to or from a colour":
        "camera FADE: fundido a o desde un color",
    "Fades the picture from one colour and opacity to another.":
        "Funde la imagen de un color y una opacidad a otros.",
    "An explosion, an engine starting, a ship taking off.":
        "Una explosión, un motor que arranca, una nave que despega.",
    "The event editor for `camera ( SHAKE )`.":
        "El editor de eventos para `camera ( SHAKE )`.",
    "The first field chooses what the camera does - here `SHAKE`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `SHAKE`. Cambiarlo cambia los campos de abajo.",
    "Intensity":
        "Intensidad",
    "0 to 16.":
        "De 0 a 16.",
    "camera SHAKE - shake the picture":
        "camera SHAKE: hacer temblar la imagen",
    "Shakes the camera for a while.":
        "Hace temblar la cámara durante un rato.",
    "A recorded camera flight - made in an editor or with **Animate** - is played as the camera's movement. The path starts where the camera is; JK Studio plays it like the game, including the notes `cut`, `fov`, `fovzoom` and `fovaccel`. A `MOVE` ends the path.":
        "Un vuelo de cámara grabado (hecho en un editor o con **Animar**) se reproduce como movimiento de la cámara. El trayecto empieza donde está la cámara; JK Studio lo reproduce como el juego, incluidas las notas `cut`, `fov`, `fovzoom` y `fovaccel`. Un `MOVE` termina el trayecto.",
    "The event editor for `camera ( PATH )`.":
        "El editor de eventos para `camera ( PATH )`.",
    "The first field chooses what the camera does - here `PATH`. Changing it changes the fields below.":
        "El primer campo elige lo que hace la cámara; aquí `PATH`. Cambiarlo cambia los campos de abajo.",
    "The ROFF file below `scripts/`, without `.rof`.":
        "El archivo ROFF bajo `scripts/`, sin `.rof`.",
    "camera PATH - fly a recorded path":
        "camera PATH: volar un trayecto grabado",
    "Plays a ROFF file as the camera's path.":
        "Reproduce un archivo ROFF como trayecto de la cámara.",
    "Commands one by one":
        "Comandos uno a uno",
    "One page per command: what it does, where to enter what in the editor, an example, and how it looks in Classic, Graph and Tracks.":
        "Una página por comando: qué hace, dónde poner qué en el editor, un ejemplo, y cómo se ve en Clásica, Grafo y Pistas.",
    "The full list of every `SET_` field, every camera command and every macro is on [[befehle]]. Here, each command of the **{{GroupEvents}}** list has its own page with pictures.":
        "La lista completa de cada campo `SET_`, cada comando de cámara y cada macro está en [[befehle]]. Aquí, cada comando de la lista **{{GroupEvents}}** tiene su propia página con imágenes.",
    "Flow":
        "Flujo",
    "Waiting":
        "Esperar",
    "Tasks":
        "Tareas",
    "Variables and text":
        "Variables y texto",
    "Start here":
        "Empieza aquí",
    "Map and scene":
        "Mapa y escena",
    "Reference":
        "Referencia",
    "**Open an empty tab.** Click the **+** at the end of the tab bar (or **{{MenuFile}} → {{FileNew}}**). A new tab `unnamed.txt` opens with an empty script. The mode bar on the left must be on **{{ViewEvents}}** - then the **{{GroupEvents}}** list with every command is on the left.":
        "**Abre una pestaña vacía.** Pulsa el **+** al final de la barra de pestañas (o **{{MenuFile}} → {{FileNew}}**). Se abre una pestaña nueva `unnamed.txt` con un script vacío. La barra de modos de la izquierda debe estar en **{{ViewEvents}}**: entonces la lista **{{GroupEvents}}** con todos los comandos está a la izquierda.",
    "The + at the end of the tab bar opens an empty script.":
        "El + al final de la barra de pestañas abre un script vacío.",
    "**Find affect in the list.** The **{{GroupEvents}}** list shows every command of the game with its values. `affect` is near the top. Hover over it to read what it does.":
        "**Busca affect en la lista.** La lista **{{GroupEvents}}** muestra cada comando del juego con sus valores. `affect` está cerca del principio. Pasa el ratón por encima para leer lo que hace.",
    "affect in the Events list.":
        "affect en la lista de eventos.",
    "**Double-click it.** `affect` appears in the script tree in the middle. It is already open (the box in front shows a minus), because a block holds other commands.":
        "**Haz doble clic.** `affect` aparece en el árbol del script del centro. Ya está abierto (el cuadro de delante muestra un menos), porque un bloque contiene otros comandos.",
    "The new affect block in the tree.":
        "El nuevo bloque affect en el árbol.",
    "**Name the figure.** Double-click the new line (or select it and press <kbd>Enter</kbd>). The **{{EditorTitle}}** opens. Type the name of the figure into the first field - with a map loaded, the small arrow next to it lists every name of the map. Leave `FLUSH` in the second field and press **{{EditorOk}}**.":
        "**Pon nombre a la figura.** Haz doble clic en la línea nueva (o selecciónala y pulsa <kbd>Intro</kbd>). Se abre el **{{EditorTitle}}**. Escribe el nombre de la figura en el primer campo; con un mapa cargado, la flechita de al lado lista todos los nombres del mapa. Deja `FLUSH` en el segundo campo y pulsa **{{EditorOk}}**.",
    "The event editor for affect: the name field and Ok.":
        "El editor de eventos para affect: el campo de nombre y Aceptar.",
    "**Select the block, then pick the next command.** Click the `affect` line once (1). Then double-click `wait` in the list (2). Because the selected block is open, the command goes to the start of the block.":
        "**Selecciona el bloque y elige el siguiente comando.** Pulsa una vez la línea `affect` (1). Luego haz doble clic en `wait` en la lista (2). Como el bloque seleccionado está abierto, el comando va al principio del bloque.",
    "1: the selected affect block. 2: wait in the list.":
        "1: el bloque affect seleccionado. 2: wait en la lista.",
    "**Check the result.** `wait` now sits inside the block, indented under `affect`. Commands you insert after it follow it, in order, inside the same block.":
        "**Revisa el resultado.** `wait` está ahora dentro del bloque, sangrado bajo `affect`. Los comandos que insertes después lo siguen, en orden, dentro del mismo bloque.",
    "wait inside the affect block.":
        "wait dentro del bloque affect.",
    "**Save.** Press **{{FileSave}}** in the action column on the right (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The first time, behaved asks for a name - script names are lower case without spaces, for example `my_first.txt`.":
        "**Guarda.** Pulsa **{{FileSave}}** en la columna de acciones de la derecha (o <kbd>Ctrl</kbd>+<kbd>S</kbd>). La primera vez, behaved pide un nombre; los nombres de script van en minúsculas y sin espacios, por ejemplo `mi_primero.txt`.",
    "Save in the action column.":
        "Guardar en la columna de acciones.",
    "**Double-click.** Select the line after which the command should come (1), then double-click the command in the **{{GroupEvents}}** list (2). The new line appears right below the selected one - here a `wait` after `camera ( MOVE )`. That is how the original BehavEd worked.":
        "**Doble clic.** Selecciona la línea tras la que debe ir el comando (1) y haz doble clic en el comando de la lista **{{GroupEvents}}** (2). La línea nueva aparece justo debajo de la seleccionada; aquí un `wait` tras `camera ( MOVE )`. Así funcionaba el BehavEd original.",
    "1: the selected line. 2: the command in the list.":
        "1: la línea seleccionada. 2: el comando en la lista.",
    "**Insert key.** Click a command in the list once - it stays highlighted (1). Select a line in the tree (2) and press <kbd>Insert</kbd>. Handy when you add the same command several times: select, <kbd>Insert</kbd>, select, <kbd>Insert</kbd> ...":
        "**Tecla Insert.** Pulsa una vez un comando de la lista: queda resaltado (1). Selecciona una línea del árbol (2) y pulsa <kbd>Insert</kbd>. Práctico cuando añades varias veces el mismo comando: seleccionar, <kbd>Insert</kbd>, seleccionar, <kbd>Insert</kbd> ...",
    "1: the command stays selected in the list. 2: where it goes.":
        "1: el comando sigue seleccionado en la lista. 2: adónde va.",
    "**Drag.** Drag a command from the list (1) onto a line of the tree (2). While you drag, a line shows where it lands: above a line, below it, or - on the middle of an open block - inside the block.":
        "**Arrastra.** Arrastra un comando de la lista (1) a una línea del árbol (2). Mientras arrastras, una línea muestra dónde caerá: encima de una línea, debajo, o (en mitad de un bloque abierto) dentro del bloque.",
    "Drag from the list (1) onto the tree (2).":
        "Arrastra desde la lista (1) al árbol (2).",
    "**Right-click.** Right-click a line in the tree: the submenus **{{GroupEvents}}** and **Macros** list every command; the one you click is inserted after that line - or into the block, if you right-clicked an open block.":
        "**Clic derecho.** Haz clic derecho en una línea del árbol: los submenús **{{GroupEvents}}** y **Macros** listan cada comando; el que pulses se inserta tras esa línea, o dentro del bloque si hiciste clic derecho en un bloque abierto.",
    "The context menu of a line with its Events submenu.":
        "El menú contextual de una línea con su submenú Eventos.",
    "**Open the editor.** Double-click a line in the tree or select it and press <kbd>Enter</kbd>. The help text of the command stands on top; below it, every value has its own field.":
        "**Abre el editor.** Haz doble clic en una línea del árbol o selecciónala y pulsa <kbd>Intro</kbd>. El texto de ayuda del comando está arriba; debajo, cada valor tiene su propio campo.",
    "**Fields for values.** Numbers, vectors and text are typed into their fields. A vector like a position has three numbers separated by spaces.":
        "**Campos para valores.** Los números, vectores y texto se escriben en sus campos. Un vector como una posición tiene tres números separados por espacios.",
    "The value fields of camera ( MOVE ): a position and a time.":
        "Los campos de valor de camera ( MOVE ): una posición y un tiempo.",
    "**Fields with fixed choices.** Fields like `SET_TYPES` or `true` / `false` are lists you can also type into - start typing to filter the list. A value that is not in the list is allowed (mods bring their own), but it is marked **unknown**: check it for a typo.":
        "**Campos con opciones fijas.** Los campos como `SET_TYPES` o `true` / `false` son listas en las que también puedes escribir: empieza a escribir para filtrar la lista. Se permite un valor que no esté en la lista (los mods traen los suyos), pero se marca como **desconocido**: revisa si hay una errata.",
    "The two list fields of set ( SET_WALKING, true ).":
        "Los dos campos de lista de set ( SET_WALKING, true ).",
    "**Files.** File fields have **{{EditorBrowse}}** to pick a file of the game and **{{EditorPlay}}** to play a sound or open a script. With a map loaded, an arrow next to name fields picks a name from the map.":
        "**Archivos.** Los campos de archivo tienen **{{EditorBrowse}}** para elegir un archivo del juego y **{{EditorPlay}}** para reproducir un sonido o abrir un script. Con un mapa cargado, una flecha junto a los campos de nombre elige un nombre del mapa.",
    "A sound command: browse and play.":
        "Un comando de sonido: examinar y reproducir.",
    "**Expressions.** Some fields have **{{EditorHelper}}**: it builds an expression for you - **{{EditorGet}}** reads a variable (`get`), **{{EditorTag}}** the place or angles of a `ref_tag` (`tag`), **{{EditorRnd}}** a random number (`random`).":
        "**Expresiones.** Algunos campos tienen **{{EditorHelper}}**: te construye una expresión; **{{EditorGet}}** lee una variable (`get`), **{{EditorTag}}** el lugar o los ángulos de un `ref_tag` (`tag`), **{{EditorRnd}}** un número aleatorio (`random`).",
    "Helper next to a field of the camera command.":
        "Ayuda junto a un campo del comando de cámara.",
    "**Free text.** Other fields have **{{EditorExpr}}**: it turns the field into free text, for anything the lists do not offer. **{{EditorRevert}}** goes back to a plain value.":
        "**Texto libre.** Otros campos tienen **{{EditorExpr}}**: convierte el campo en texto libre, para cualquier cosa que las listas no ofrezcan. **{{EditorRevert}}** vuelve a un valor simple.",
    "Expr! next to the fields of affect.":
        "¡Expresión! junto a los campos de affect.",
    "**Done.** **{{EditorOk}}** (<kbd>Enter</kbd>) writes the values into the script, **{{EditorCancel}}** (<kbd>Esc</kbd>) leaves everything as it was. Under each field you see the value it had before (**{{EditorWas}}**).":
        "**Listo.** **{{EditorOk}}** (<kbd>Intro</kbd>) escribe los valores en el script, **{{EditorCancel}}** (<kbd>Esc</kbd>) lo deja todo como estaba. Bajo cada campo ves el valor que tenía antes (**{{EditorWas}}**).",
    "Ok and Cancel.":
        "Aceptar y Cancelar.",
    "**Select.** Click a line in the tree. <kbd>Ctrl</kbd>+click adds more lines, <kbd>Shift</kbd>+click selects everything between. Every button of the action column works on all selected lines at once.":
        "**Selecciona.** Pulsa una línea del árbol. <kbd>Ctrl</kbd>+clic añade más líneas, <kbd>Mayús</kbd>+clic selecciona todo lo que hay entre medias. Cada botón de la columna de acciones actúa a la vez sobre todas las líneas seleccionadas.",
    "Two selected lines.":
        "Dos líneas seleccionadas.",
    "**Move.** Drag the selection to a new place - or press **{{ActMoveUp}}** / **{{ActMoveDown}}** in the action column (<kbd>Alt</kbd>+<kbd>Up</kbd> / <kbd>Alt</kbd>+<kbd>Down</kbd>) to move it one line. It stays inside its block. A block moves with everything inside it.":
        "**Mueve.** Arrastra la selección a un sitio nuevo, o pulsa **{{ActMoveUp}}** / **{{ActMoveDown}}** en la columna de acciones (<kbd>Alt</kbd>+<kbd>Arriba</kbd> / <kbd>Alt</kbd>+<kbd>Abajo</kbd>) para moverla una línea. Se queda dentro de su bloque. Un bloque se mueve con todo lo que contiene.",
    "Move up and Move down.":
        "Subir y Bajar.",
    "**Duplicate.** **{{ActClone}}** (<kbd>Space</kbd>) inserts a copy of the selection right below - the quickest way to a second camera shot or a second walk.":
        "**Duplica.** **{{ActClone}}** (<kbd>Espacio</kbd>) inserta una copia de la selección justo debajo: la forma más rápida de tener un segundo plano de cámara o una segunda marcha.",
    "Duplicate.":
        "Duplicar.",
    "**Copy between scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>) copies the selection. Switch to another tab, select a line and press **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>): the commands are inserted below that line.":
        "**Copia entre scripts.** **{{ActCopy}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd>) copia la selección. Cambia a otra pestaña, selecciona una línea y pulsa **{{ActPaste}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>): los comandos se insertan debajo de esa línea.",
    "Copy and Paste.":
        "Copiar y Pegar.",
    "**Switch off without deleting.** **{{ActRem}}** (<kbd>Backspace</kbd>) turns the selected lines into comments: they stay in the script, but the game skips them. Press it again (or <kbd>Ctrl</kbd>+<kbd>Backspace</kbd>) to switch them back on.":
        "**Desactiva sin borrar.** **{{ActRem}}** (<kbd>Retroceso</kbd>) convierte las líneas seleccionadas en comentarios: se quedan en el script, pero el juego se las salta. Púlsalo de nuevo (o <kbd>Ctrl</kbd>+<kbd>Retroceso</kbd>) para volver a activarlas.",
    "REM: the two lines are now comments.":
        "REM: las dos líneas son ahora comentarios.",
    "**Undo and redo.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> undoes the last change, <kbd>Ctrl</kbd>+<kbd>Y</kbd> redoes it. In the **{{MenuEdit}}** menu the entry names the step - here *Undo: edit*.":
        "**Deshacer y rehacer.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> deshace el último cambio, <kbd>Ctrl</kbd>+<kbd>Y</kbd> lo rehace. En el menú **{{MenuEdit}}** la entrada nombra el paso; aquí *Deshacer: editar*.",
    "The Edit menu names the step that Undo takes back.":
        "El menú Edición nombra el paso que revierte Deshacer.",
    "**See the whole history.** Right-click the **Undo** button in the action column (or **{{MenuEdit}} → {{UndoListMenu}}**). The list shows every step, newest at the top.":
        "**Mira el historial completo.** Haz clic derecho en el botón **Deshacer** de la columna de acciones (o **{{MenuEdit}} → {{UndoListMenu}}**). La lista muestra cada paso, el más reciente arriba.",
    "**Go back several steps.** Click a step in the list and press **{{UndoUpToHere}}**: that step and all newer ones are undone.":
        "**Retrocede varios pasos.** Pulsa un paso de la lista y luego **{{UndoUpToHere}}**: ese paso y todos los más recientes se deshacen.",
    "Undo up to here.":
        "Deshacer hasta aquí.",
    "Undo only this step.":
        "Deshacer solo este paso.",
    "**Open all scripts.** For a mission, **{{MenuFile}} → {{LoadMission}}** opens every script in its own tab - then a search can reach all of them.":
        "**Abre todos los scripts.** En una misión, **{{MenuFile}} → {{LoadMission}}** abre cada script en su propia pestaña; así una búsqueda puede alcanzarlos todos.",
    "Load mission in the File menu.":
        "Cargar misión en el menú Archivo.",
    "**Open Find.** Press **{{ActFind}}** in the action column (<kbd>Ctrl</kbd>+<kbd>F</kbd>). The window stays open while you work in the tree. Its fields:":
        "**Abre Buscar.** Pulsa **{{ActFind}}** en la columna de acciones (<kbd>Ctrl</kbd>+<kbd>F</kbd>). La ventana se queda abierta mientras trabajas en el árbol. Sus campos:",
    "**Search a value.** Type the old name into **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) and **{{FindPrevBtn}}** (<kbd>Shift</kbd>+<kbd>F3</kbd>) jump from hit to hit. Tick **{{FindWhole}}** so that `kyle` does not also hit `kyle2`.":
        "**Busca un valor.** Escribe el nombre antiguo en **{{FindContains}}**. **{{FindNextBtn}}** (<kbd>F3</kbd>) y **{{FindPrevBtn}}** (<kbd>Mayús</kbd>+<kbd>F3</kbd>) saltan de resultado en resultado. Marca **{{FindWhole}}** para que `kyle` no encuentre también `kyle2`.",
    "Item containing, the buttons to step through the hits, and Whole-string match only.":
        "Elemento que contiene, los botones para recorrer los resultados y Solo palabra completa.",
    "Item called.":
        "Elemento llamado.",
    "**Replace.** Type the new name into **{{FindReplaceWith}}** and press **{{FindReplaceAllDocs}}**. Every open script is changed; each one gets its own undo step.":
        "**Reemplaza.** Escribe el nombre nuevo en **{{FindReplaceWith}}** y pulsa **{{FindReplaceAllDocs}}**. Cambia cada script abierto; cada uno recibe su propio paso de deshacer.",
    "Replace with and Replace All in All Opened Documents.":
        "Reemplazar por y Reemplazar todo en todos los documentos abiertos.",
    "**Set a bookmark.** Select a line and press <kbd>Ctrl</kbd>+<kbd>F2</kbd> - or **{{MenuEdit}} → {{MenuBookmarks}} → {{ActBookmarkToggle}}**.":
        "**Pon un marcador.** Selecciona una línea y pulsa <kbd>Ctrl</kbd>+<kbd>F2</kbd>, o **{{MenuEdit}} → {{MenuBookmarks}} → {{ActBookmarkToggle}}**.",
    "Toggle bookmark in the Edit menu.":
        "Poner/quitar marcador en el menú Edición.",
    "**See it.** A mark appears in the margin next to the line. Clicking the margin sets or removes a bookmark too.":
        "**Míralo.** Aparece una marca en el margen junto a la línea. Pulsar el margen también pone o quita un marcador.",
    "The bookmarked line.":
        "La línea marcada.",
    "Next and Previous bookmark.":
        "Marcador siguiente y anterior.",
    "**Copy across.** The right side is read-only, but you can drag lines from it into your script on the left - they are copied. To edit the right one, click its tab.":
        "**Copia de un lado a otro.** El lado derecho es de solo lectura, pero puedes arrastrar líneas de él a tu script de la izquierda: se copian. Para editar el derecho, pulsa su pestaña.",
    "Left: your script. Right: the other one, read-only.":
        "Izquierda: tu script. Derecha: el otro, de solo lectura.",
    "**Compile.** Press **{{Compile}}** at the bottom of the action column. It saves the `.txt` and writes the `.ibi` the game runs next to it. When it worked, the button flashes green.":
        "**Compila.** Pulsa **{{Compile}}** abajo en la columna de acciones. Guarda el `.txt` y escribe a su lado el `.ibi` que ejecuta el juego. Cuando funciona, el botón parpadea en verde.",
    "**When it fails.** The button flashes red, and the first wrong line is selected in the tree - here an `if` with the comparison `~`, which ICARUS does not know.":
        "**Si falla.** El botón parpadea en rojo y la primera línea errónea queda seleccionada en el árbol; aquí un `if` con la comparación `~`, que ICARUS no conoce.",
    "Compile failed: the button and the selected wrong line.":
        "Error al compilar: el botón y la línea errónea seleccionada.",
    "**Read the messages.** Click the counter in the status line at the bottom: the **{{MessagesTitle}}** window lists every message with its line and what is wrong.":
        "**Lee los mensajes.** Pulsa el contador de la línea de estado de abajo: la ventana **{{MessagesTitle}}** lista cada mensaje con su línea y qué está mal.",
    "The counters in the status line open the Messages window.":
        "Los contadores de la línea de estado abren la ventana de mensajes.",
    "**Fix and compile again.** Double-click the wrong line, correct the value - here the comparison: `=`, `<`, `>` or `!` - press **{{EditorOk}}** and **{{Compile}}** again.":
        "**Corrige y compila de nuevo.** Haz doble clic en la línea errónea, corrige el valor (aquí la comparación: `=`, `<`, `>` o `!`), pulsa **{{EditorOk}}** y otra vez **{{Compile}}**.",
    "The wrong value in the event editor.":
        "El valor erróneo en el editor de eventos.",
    "**Why.** An `affect` block runs beside the script - the script does not wait for it. That is why Kyle and Rosh, each in his own block, already walk at the same time.":
        "**Por qué.** Un bloque `affect` se ejecuta junto al script: el script no lo espera. Por eso Kyle y Rosh, cada uno en su bloque, ya caminan a la vez.",
    "Two affect blocks: they run side by side.":
        "Dos bloques affect: se ejecutan en paralelo.",
    "**The problem.** Inside a block, `set ( SET_NAVGOAL )` only *starts* the walk; the next command follows at once - the figure would turn or talk while still walking. **The fix:** put the walk into a `task`. Insert `task`, name it `walk`, and move `SET_WALKING` and `SET_NAVGOAL` into it.":
        "**El problema.** Dentro de un bloque, `set ( SET_NAVGOAL )` solo *inicia* la marcha; el comando siguiente sigue enseguida: la figura se giraría o hablaría aún caminando. **La solución:** mete la marcha en una `task`. Inserta `task`, llámala `walk` y mueve dentro `SET_WALKING` y `SET_NAVGOAL`.",
    "The task walk with the walk inside.":
        "La tarea walk con la marcha dentro.",
    "dowait waits for the walk.":
        "dowait espera a la marcha.",
    "**Two things at once.** `do` starts a task and goes on at once. Two tasks - `walk` and `look` - started with two `do`, and `wait ( \"walk\" )` later waits for the first one.":
        "**Dos cosas a la vez.** `do` inicia una tarea y sigue enseguida. Dos tareas (`walk` y `look`) iniciadas con dos `do`, y más tarde `wait ( \"walk\" )` espera a la primera.",
    "do, do and wait.":
        "do, do y wait.",
    "**Check it in the timeline.** Switch to **{{ViewMap}}**, expand the figure's row in the timeline: what follows the `dowait` starts where the walk ends.":
        "**Compruébalo en la línea de tiempo.** Cambia a **{{ViewMap}}** y despliega la fila de la figura en la línea de tiempo: lo que sigue al `dowait` empieza donde termina la marcha.",
    "Kyle's walk in the timeline.":
        "La marcha de Kyle en la línea de tiempo.",
    "**Where to wait.** In the script that should pause - here the intro - insert `waitsignal` and type a name, for example `door_open`.":
        "**Dónde esperar.** En el script que debe detenerse (aquí la introducción), inserta `waitsignal` y escribe un nombre, por ejemplo `door_open`.",
    "waitsignal in the intro (left).":
        "waitsignal en la introducción (izquierda).",
    "**Where to give the signal.** In the other script, at the moment the first should go on, insert `signal ( \"door_open\" )`.":
        "**Dónde dar la señal.** En el otro script, en el momento en que el primero debe seguir, inserta `signal ( \"door_open\" )`.",
    "signal in the other script (right).":
        "signal en el otro script (derecha).",
    "**Check the names.** Both must be spelled exactly the same. Only one script can wait for a signal of a given name. The split view (**{{MenuFile}} → {{SplitToggle}} → 2**) shows both at once.":
        "**Comprueba los nombres.** Los dos deben escribirse exactamente igual. Solo un script puede esperar una señal de un nombre dado. La vista dividida (**{{MenuFile}} → {{SplitToggle}} → 2**) muestra ambos a la vez.",
    "The same name on both sides.":
        "El mismo nombre en los dos lados.",
    "Waits as blocks in the timeline.":
        "Esperas como bloques en la línea de tiempo.",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list - scroll down. Each shows how many commands it holds; hover over one to read what it does.":
        "**Encuéntralas.** Las macros están al final de la lista **{{GroupEvents}}**: desplázate hacia abajo. Cada una muestra cuántos comandos contiene; pasa el ratón por encima de una para leer lo que hace.",
    "standOnly and the other macros at the end of the Events list.":
        "standOnly y las demás macros al final de la lista de eventos.",
    "**Insert.** Double-click or drag a macro like a command. `standOnly`, for example, makes a figure stand and ignore enemies - three `set` commands in one line.":
        "**Inserta.** Haz doble clic o arrastra una macro como un comando. `standOnly`, por ejemplo, hace que una figura se quede quieta e ignore a los enemigos: tres comandos `set` en una línea.",
    "Fold Macros and a macro in the tree.":
        "Plegar macros y una macro en el árbol.",
    "**Declare once.** In a script that runs only once - the intro, a spawn script - insert `declare`. In the editor choose `FLOAT` and type a name, for example `seen`. A declared variable starts at 0; a map can hold 16 of them.":
        "**Declara una vez.** En un script que se ejecute solo una vez (la introducción, un script de aparición) inserta `declare`. En el editor elige `FLOAT` y escribe un nombre, por ejemplo `visto`. Una variable declarada empieza en 0; un mapa puede tener 16.",
    "declare: the type and the name.":
        "declare: el tipo y el nombre.",
    "**Ask with if.** In the script that may run more often, insert `if`. On the first value press **{{EditorHelper}}** and build `get ( FLOAT, \"seen\" )` with **{{EditorGet}}**; as comparison `=`, as the other value `0`.":
        "**Pregunta con if.** En el script que puede ejecutarse más veces, inserta `if`. En el primer valor pulsa **{{EditorHelper}}** y construye `get ( FLOAT, \"visto\" )` con **{{EditorGet}}**; como comparación `=`, como otro valor `0`.",
    "The event editor for if: the variable and the value.":
        "El editor de eventos para if: la variable y el valor.",
    "**Do it inside, or else.** Put the commands of the scene into the `if` block. An `else` right after it runs when the scene was already seen.":
        "**Hazlo dentro, o si no.** Mete los comandos de la escena en el bloque `if`. Un `else` justo después se ejecuta cuando la escena ya se vio.",
    "The if and the else block.":
        "El bloque if y el bloque else.",
    "set ( \"seen\", \"1\" ) inside the if.":
        "set ( \"visto\", \"1\" ) dentro del if.",
    "free at the end.":
        "free al final.",
    "**Open it.** Double-click the entry: the `.ibi` opens as a script in the current tab - commands, values and blocks, exactly as Raven wrote them. Open an empty tab first (**+**) if you want to keep the script you are working on.":
        "**Ábrelo.** Doble clic en la entrada: el `.ibi` se abre como script en la pestaña actual (comandos, valores y bloques, exactamente como los escribió Raven). Abre antes una pestaña vacía (**+**) si quieres conservar el script en el que trabajas.",
    "Open in the File menu.":
        "Abrir en el menú Archivo.",
    "**Change it.** A script from an archive has no file yet - the first **{{FileSave}}** asks where to put it. To ship a changed version of a game scene, see [[k-szene-aendern]].":
        "**Cámbialo.** Un script de un .pk3 aún no tiene archivo: el primer **{{FileSave}}** pregunta dónde ponerlo. Para distribuir una versión cambiada de una escena del juego, mira [[k-szene-aendern]].",
    "The kind of file and the archive.":
        "El tipo de archivo y el archivo .pk3.",
    "The filter.":
        "El filtro.",
    "**Open.** Double-click an entry: a script replaces the one in the current tab (behaved asks first if it has unsaved changes), a map loads, a model opens with its skin in the **{{ViewModel}}** tab.":
        "**Abre.** Doble clic en una entrada: un script sustituye al de la pestaña actual (behaved pregunta antes si tiene cambios sin guardar), un mapa se carga, un modelo se abre con su skin en la pestaña **{{ViewModel}}**.",
    "Double-click an entry to open it.":
        "Doble clic en una entrada para abrirla.",
    "The original line over the margin.":
        "La línea original sobre el margen.",
    "Revert to original in the context menu.":
        "Volver al original en el menú contextual.",
    "Highlight identical commands in the View menu.":
        "Resaltar comandos idénticos en el menú Ver.",
    "Open backup folder in the File menu.":
        "Abrir la carpeta de copias de seguridad en el menú Archivo.",
    "Backup.":
        "Copia de seguridad.",
    "Restore.":
        "Restaurar.",
    "**Smaller steps.** For the last few changes the undo history is quicker: right-click **Undo** ([[k-rueckgaengig]]).":
        "**Pasos más pequeños.** Para los últimos cambios es más rápido el historial de deshacer: clic derecho en **Deshacer** ([[k-rueckgaengig]]).",
    "**The counters.** The status line at the bottom counts errors and notes. Click a counter to open the **{{MessagesTitle}}** window.":
        "**Los contadores.** La línea de estado de abajo cuenta errores y notas. Pulsa un contador para abrir la ventana **{{MessagesTitle}}**.",
    "The counters at the bottom left.":
        "Los contadores abajo a la izquierda.",
    "**Fix and check.** Double-click the line, correct the value in the editor and press **{{EditorOk}}** - the message disappears. Messages never change the script by themselves.":
        "**Corrige y comprueba.** Haz doble clic en la línea, corrige el valor en el editor y pulsa **{{EditorOk}}**: el mensaje desaparece. Los mensajes nunca cambian el script por sí solos.",
    "The misspelt value in the editor.":
        "El valor mal escrito en el editor.",
    "**Select a shot.** In the **{{ViewMap}}** tab click the camera on the map, or a shot in the **{{CamShots}}** list on the left - the camera panel replaces the entity inspector while the camera is selected.":
        "**Selecciona un plano.** En la pestaña **{{ViewMap}}** pulsa la cámara en el mapa, o un plano de la lista **{{CamShots}}** de la izquierda: el panel de la cámara sustituye al inspector de entidades mientras la cámara está seleccionada.",
    "The shots of the camera in the panel on the left.":
        "Los planos de la cámara en el panel de la izquierda.",
    "**Start Pilot.** Press **{{CamPilot}}** - in the camera panel or on the bar above the camera in the view. You now look through this shot.":
        "**Inicia el piloto.** Pulsa **{{CamPilot}}**, en el panel de la cámara o en la barra encima de la cámara en la vista. Ahora miras a través de este plano.",
    "Pilot in the camera panel.":
        "Pilotar en el panel de la cámara.",
    "**Set keys along the way.** Move the playhead in the timeline below the map, fly to where the camera should be at that moment and press <kbd>Enter</kbd> or **{{CamPilotKey}}**. Repeat for more keys.":
        "**Pon claves por el camino.** Mueve el cabezal en la línea de tiempo bajo el mapa, vuela hasta donde debe estar la cámara en ese momento y pulsa <kbd>Intro</kbd> o **{{CamPilotKey}}**. Repite para más claves.",
    "Set key while piloting, with the timeline below.":
        "Poner clave mientras pilotas, con la línea de tiempo debajo.",
    "**Finish.** **{{CamPilotApply}}** writes the view into the script; **{{CamPilotCancel}}** (<kbd>Esc</kbd>) leaves it as it was.":
        "**Termina.** **{{CamPilotApply}}** escribe la vista en el script; **{{CamPilotCancel}}** (<kbd>Esc</kbd>) lo deja como estaba.",
    "Apply and Cancel.":
        "Aplicar y Cancelar.",
    "**Fly to the view you want.** Hold the right mouse button in the map and fly with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> until the picture is right.":
        "**Vuela hasta la vista que quieres.** Mantén el botón derecho en el mapa y vuela con <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> hasta que la imagen sea la buena.",
    "Your own view of the map.":
        "Tu propia vista del mapa.",
    "**A new shot here.** Select the camera and press **{{CamNewShot}}**: a `camera ( MOVE )` and `camera ( PAN )` at your view are added after the selected shot.":
        "**Un plano nuevo aquí.** Selecciona la cámara y pulsa **{{CamNewShot}}**: se añaden un `camera ( MOVE )` y un `camera ( PAN )` en tu vista tras el plano seleccionado.",
    "New camera shot here in the camera panel.":
        "Nuevo plano de cámara aquí en el panel de la cámara.",
    "**Or move an existing one.** Select a shot in **{{CamShots}}** and press **{{CamSetToView}}**: its position and direction are replaced by your view - one undo step.":
        "**O mueve uno existente.** Selecciona un plano en **{{CamShots}}** y pulsa **{{CamSetToView}}**: su posición y su dirección se sustituyen por tu vista, en un paso de deshacer.",
    "Set to view.":
        "Ajustar a la vista.",
    "Through camera on the toolbar.":
        "A través de la cámara en la barra.",
    "**Place a mark.** Click the tile **{{EntKindTag}}** on the left edge of the view, then click where the camera should stand.":
        "**Coloca una marca.** Pulsa la ficha **{{EntKindTag}}** en el borde izquierdo de la vista y luego pulsa donde debe estar la cámara.",
    "The tile for a camera mark.":
        "La ficha de una marca de cámara.",
    "**Camera to the mark.** Right-click the mark in the entity list → **{{EntityCamToTag}}**. Two camera commands are inserted that take the mark's place and angles (`tag`).":
        "**Cámara a la marca.** Clic derecho en la marca en la lista de entidades → **{{EntityCamToTag}}**. Se insertan dos comandos de cámara que toman el lugar y los ángulos de la marca (`tag`).",
    "**Move the mark.** Select it and drag it in the view, or change its **{{EntOrigin}}** in the inspector: the shot moves with it.":
        "**Mueve la marca.** Selecciónala y arrástrala en la vista, o cambia su **{{EntOrigin}}** en el inspector: el plano se mueve con ella.",
    "A selected entity with its arrows.":
        "Una entidad seleccionada con sus flechas.",
    "**Turn it.** The mark's facing is the camera's direction: turn it with the ring, or type **{{EntAngle}}** in the inspector.":
        "**Gírala.** La orientación de la marca es la dirección de la cámara: gírala con el anillo, o escribe **{{EntAngle}}** en el inspector.",
    "The first field of camera chooses the command.":
        "El primer campo de camera elige el comando.",
    "**Zoom.** Choose `ZOOM`: (1) the field of view - 80 is normal, smaller is closer - and (2) the time in milliseconds to get there.":
        "**Zoom.** Elige `ZOOM`: (1) el campo de visión (80 es normal, menos es más cerca) y (2) el tiempo en milisegundos para llegar.",
    "camera ( ZOOM ): field of view and time.":
        "camera ( ZOOM ): campo de visión y tiempo.",
    "**Fade.** Choose `FADE`: (1) start colour and (2) its opacity, (3) end colour and (4) its opacity - all from 0 to 1 - and (5) the time. `0 0 0`, `0` to `0 0 0`, `1` fades to black.":
        "**Fundido.** Elige `FADE`: (1) color inicial y (2) su opacidad, (3) color final y (4) su opacidad (todo de 0 a 1) y (5) el tiempo. `0 0 0`, `0` a `0 0 0`, `1` funde a negro.",
    "camera ( FADE ): five values.":
        "camera ( FADE ): cinco valores.",
    "**Shake.** Choose `SHAKE`: (1) the strength from 0 to 16 and (2) the duration in milliseconds.":
        "**Temblor.** Elige `SHAKE`: (1) la intensidad de 0 a 16 y (2) la duración en milisegundos.",
    "camera ( SHAKE ): strength and duration.":
        "camera ( SHAKE ): intensidad y duración.",
    "The rows of the timeline, among them Zoom, Fade and Shake.":
        "Las filas de la línea de tiempo, entre ellas Zoom, Fundido y Temblor.",
    "**Play it.** Press ▶ in the timeline below the map. Switch **{{TlAudio}}** on to hear the dialogue you are matching the cuts against.":
        "**Reprodúcela.** Pulsa ▶ en la línea de tiempo bajo el mapa. Activa **{{TlAudio}}** para oír el diálogo con el que cuadras los cortes.",
    "To start, play - and Sound.":
        "Al inicio, reproducir, y Sonido.",
    "The right edge of a block.":
        "El borde derecho de un bloque.",
    "The time display.":
        "El indicador de tiempo.",
    "**Check the script.** Every drag changes a value in the script (a wait, a time). Click a block: its line is selected in the script column on the right.":
        "**Revisa el script.** Cada arrastre cambia un valor del script (una espera, un tiempo). Pulsa un bloque: su línea se selecciona en la columna del script de la derecha.",
    "A clicked block and its line in the script.":
        "Un bloque pulsado y su línea en el script.",
    "**Through camera.** **{{MapThroughCam}}** on the map toolbar shows the picture of the selected shot - or, with nothing selected, what the camera sees at the playhead.":
        "**A través de la cámara.** **{{MapThroughCam}}** en la barra del mapa muestra la imagen del plano seleccionado o, sin nada seleccionado, lo que ve la cámara en el cabezal.",
    "Through camera.":
        "A través de la cámara.",
    "**The small preview.** **{{CamPreview}}** on the camera bar puts a small picture of the camera's view in the corner - you keep flying freely. Click the picture to look through it.":
        "**La vista previa pequeña.** **{{CamPreview}}** en la barra de la cámara pone una pequeña imagen de lo que ve la cámara en la esquina; tú sigues volando libremente. Pulsa la imagen para mirar a través de ella.",
    "The camera preview.":
        "La vista previa de cámara.",
    "Follow.":
        "Seguir.",
    "The speed list.":
        "La lista de velocidad.",
    "SET_CAMERA_GROUP in Kyle's block.":
        "SET_CAMERA_GROUP en el bloque de Kyle.",
    "**Follow.** After the camera shot insert `camera` and choose `FOLLOW`: (1) the group `hero`, (2) a turning speed, for example `100`, and (3) `1` to blend into it.":
        "**Sigue.** Tras el plano de cámara inserta `camera` y elige `FOLLOW`: (1) el grupo `heroe`, (2) una velocidad de giro, por ejemplo `100`, y (3) `1` para fundirse con ello.",
    "camera ( FOLLOW ): group, speed, blend.":
        "camera ( FOLLOW ): grupo, velocidad, fundido.",
    "**Keep a distance.** `camera ( DISTANCE )` with (1) `200` and (2) `1` keeps the camera 200 units from the group, moving with it.":
        "**Guarda la distancia.** `camera ( DISTANCE )` con (1) `200` y (2) `1` mantiene la cámara a 200 unidades del grupo, moviéndose con él.",
    "camera ( DISTANCE ).":
        "camera ( DISTANCE ).",
    "**Watch it.** Turn on **{{MapThroughCam}}** and play the timeline: the view keeps turning towards the figure.":
        "**Míralo.** Activa **{{MapThroughCam}}** y reproduce la línea de tiempo: la vista se sigue girando hacia la figura.",
    "**Fade it.** `SET_VIDEO_FADE_IN` and `SET_VIDEO_FADE_OUT` set to `true` before the video let it fade in and out.":
        "**Fundido.** `SET_VIDEO_FADE_IN` y `SET_VIDEO_FADE_OUT` puestos a `true` antes del vídeo lo hacen entrar y salir con fundido.",
    "The two fade lines before the video.":
        "Las dos líneas de fundido antes del vídeo.",
    "Video, fades and a line of text in the tree.":
        "Vídeo, fundidos y una línea de texto en el árbol.",
    "The status line confirms the stored view.":
        "La línea de estado confirma la vista guardada.",
    "Fly to shot.":
        "Volar al plano.",
    "**Jump to a line, change a duration.** Click a block to select its line; drag its right edge to change the duration - just like in the timeline below the map.":
        "**Salta a una línea, cambia una duración.** Pulsa un bloque para seleccionar su línea; arrastra su borde derecho para cambiar la duración, igual que en la línea de tiempo bajo el mapa.",
    "Blocks have a right edge you can drag.":
        "Los bloques tienen un borde derecho que puedes arrastrar.",
    "**Move through time.** Drag in the ruler at the top; **{{TracksPlay}}** plays.":
        "**Muévete en el tiempo.** Arrastra en la regla de arriba; **{{TracksPlay}}** reproduce.",
    "The ruler and Play at the top of the Tracks view.":
        "La regla y Reproducir arriba de la vista de pistas.",
    "Character rows and Waits.":
        "Filas de personajes y Esperas.",
    "The camera rows.":
        "Las filas de cámara.",
    "Kyle and his path.":
        "Kyle y su trayecto.",
    "**Right-click the walk target** at the end of the path. The menu offers **{{BahnGehen}}**, **{{BahnRennen}}** and **{{BahnRueckwaerts}}**.":
        "**Clic derecho en el destino** al final del trayecto. El menú ofrece **{{BahnGehen}}**, **{{BahnRennen}}** y **{{BahnRueckwaerts}}**.",
    "**Pick one.** behaved writes `SET_WALKING` / `SET_RUNNING` before this walk - here **{{BahnRennen}}** added `set ( SET_RUNNING, true )`. It counts from this walk on.":
        "**Elige uno.** behaved escribe `SET_WALKING` / `SET_RUNNING` antes de este tramo; aquí **{{BahnRennen}}** añadió `set ( SET_RUNNING, true )`. Vale a partir de este tramo.",
    "The new SET_RUNNING line.":
        "La nueva línea SET_RUNNING.",
    "**Watch.** Play the timeline: the figure runs, and its walk block is shorter.":
        "**Mira.** Reproduce la línea de tiempo: la figura corre y su bloque de marcha es más corto.",
    "The shorter walk in the timeline.":
        "La marcha más corta en la línea de tiempo.",
    "Look at while walking.":
        "Mirar a esto al caminar.",
    "**Standing.** Right-click the target in the entity list → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) or **{{EntityAsLook}}** (`SET_LOOK_TARGET`). The command goes into the script after the selected line.":
        "**De pie.** Clic derecho en el objetivo en la lista de entidades → **{{EntityAsWatch}}** (`SET_WATCHTARGET`) o **{{EntityAsLook}}** (`SET_LOOK_TARGET`). El comando va al script tras la línea seleccionada.",
    "The context menu of an entity in the list.":
        "El menú contextual de una entidad en la lista.",
    "Face the walking direction.":
        "Mirar en la dirección de la marcha.",
    "**Go to the moment.** Move the playhead in the timeline to where the figure should face another way. The time display shows where you are.":
        "**Ve al momento.** Mueve el cabezal en la línea de tiempo hasta donde la figura debe mirar hacia otro lado. El indicador de tiempo muestra dónde estás.",
    "The playhead and the time display.":
        "El cabezal y el indicador de tiempo.",
    "Rotate on the gizmo bar, the figure selected.":
        "Rotar en la barra del gizmo, con la figura seleccionada.",
    "**Turn.** Drag the ring. When you let go, behaved writes the facing at this time into the script (`SET_DYAW` for an NPC, which then turns with its turn rate as in the game; `SET_ANGLES` for the player); the status line says *Facing … degrees at … s*.":
        "**Gira.** Arrastra el anillo. Al soltar, behaved escribe la orientación en ese momento en el script (`SET_DYAW` para un NPC, que gira entonces con su velocidad de giro como en el juego; `SET_ANGLES` para el jugador); la línea de estado dice *Orientación … grados en … s*.",
    "The ring around the standing figure.":
        "El anillo alrededor de la figura quieta.",
    "The ring on a walking figure.":
        "El anillo en una figura que camina.",
    "The NPC browser.":
        "El navegador de NPC.",
    "Loop in the playback bar.":
        "Bucle en la barra de reproducción.",
    "Into script.":
        "Al script.",
    "The figure in the scene.":
        "La figura en la escena.",
    "**Insert sound.** Inside the figure's `affect` block insert `sound`. In the editor choose the channel `CHAN_VOICE` and pick the file with **{{EditorBrowse}}**; **{{EditorPlay}}** plays it.":
        "**Inserta sound.** Dentro del bloque `affect` de la figura inserta `sound`. En el editor elige el canal `CHAN_VOICE` y escoge el archivo con **{{EditorBrowse}}**; **{{EditorPlay}}** lo reproduce.",
    "The sound command: browse and play.":
        "El comando sound: examinar y reproducir.",
    "Sound in the timeline.":
        "Sonido en la línea de tiempo.",
    "A spawn script with standOnly.":
        "Un script de aparición con standOnly.",
    "The figures in the scene.":
        "Las figuras en la escena.",
    "camera ( DISABLE ).":
        "camera ( DISABLE ).",
    "**Attack.** Right after it, an `affect` block for the enemy with `SET_IGNOREENEMIES false`, `SET_LOOK_FOR_ENEMIES true` and `SET_ENEMY player`. The macro `default` resets all behaviour in one line.":
        "**Ataque.** Justo después, un bloque `affect` para el enemigo con `SET_IGNOREENEMIES false`, `SET_LOOK_FOR_ENEMIES true` y `SET_ENEMY player`. La macro `default` restablece todo el comportamiento en una línea.",
    "The three lines that start the fight.":
        "Las tres líneas que empiezan el combate.",
    "run starts another script.":
        "run inicia otro script.",
    "Lock the player.":
        "Bloquear al jugador.",
    "The scene, seen by the player.":
        "La escena, vista por el jugador.",
    "Free the player again.":
        "Volver a liberar al jugador.",
    "SET_WEAPON WP_SABER.":
        "SET_WEAPON WP_SABER.",
    "SET_SABERACTIVE true.":
        "SET_SABERACTIVE true.",
    "**See it.** Play the timeline: Kyle ignites his blade.":
        "**Míralo.** Reproduce la línea de tiempo: Kyle enciende su hoja.",
    "Kyle with his blade ignited.":
        "Kyle con la hoja encendida.",
    "SET_SABERACTIVE false.":
        "SET_SABERACTIVE false.",
    "The arrows of a selected entity.":
        "Las flechas de una entidad seleccionada.",
    "**Copy.** <kbd>Alt</kbd>+drag makes a copy and moves it - or press **{{EntDuplicate}}** in the inspector.":
        "**Copia.** <kbd>Alt</kbd>+arrastrar hace una copia y la mueve; o pulsa **{{EntDuplicate}}** en el inspector.",
    "Duplicate in the inspector.":
        "Duplicar en el inspector.",
    "**Onto the floor.** **{{EntDropFloor}}** puts it exactly on the floor below - the inspector shows how high the feet are.":
        "**Al suelo.** **{{EntDropFloor}}** la pone exactamente sobre el suelo de debajo; el inspector muestra a qué altura están los pies.",
    "Onto floor.":
        "Al suelo.",
    "**Search.** Type a name or class into the search box above the entity list, for example `nav` or `kyle`.":
        "**Busca.** Escribe un nombre o una clase en el campo de búsqueda encima de la lista de entidades, por ejemplo `nav` o `kyle`.",
    "The search box of the entity list.":
        "El campo de búsqueda de la lista de entidades.",
    "A row of the entity list.":
        "Una fila de la lista de entidades.",
    "Names on the map.":
        "Nombres en el mapa.",
    "**Select the entity.** Click it in the view or in the list. The inspector under the list shows its class in colour at the top; hover over it for a description from the game code.":
        "**Selecciona la entidad.** Púlsala en la vista o en la lista. El inspector bajo la lista muestra arriba su clase en color; pasa el ratón por encima para leer una descripción del código del juego.",
    "The class at the top of the inspector.":
        "La clase arriba del inspector.",
    "**Keys.** Under **{{EntSecFields}}**, change values or add one with **{{EntAddKey}}**. Script keys have **{{EntOpenScript}}** next to them.":
        "**Claves.** En **{{EntSecFields}}**, cambia valores o añade uno con **{{EntAddKey}}**. Las claves de script tienen **{{EntOpenScript}}** al lado.",
    "Fields and + Key.":
        "Campos y + Clave.",
    "All keys (raw).":
        "Todas las claves (en bruto).",
    "The cut height.":
        "La altura de corte.",
    "A selected figure in the top view.":
        "Una figura seleccionada en la vista superior.",
    "**Switch things off.** **{{MapEffects}}**, **{{MapGlow}}**, **{{MapDynLights}}**, **{{MapFog}}** and **{{MapSky}}** on the toolbar switch those parts off. Without the sky you can look into the map from outside.":
        "**Desactiva cosas.** **{{MapEffects}}**, **{{MapGlow}}**, **{{MapDynLights}}**, **{{MapFog}}** y **{{MapSky}}** en la barra desactivan esas partes. Sin el cielo puedes mirar el mapa desde fuera.",
    "The switches on the map toolbar.":
        "Los interruptores de la barra del mapa.",
    "Highlight this script's entities.":
        "Resaltar las entidades de este script.",
    "The tile for a script runner.":
        "La ficha de un ejecutor de scripts.",
    "Open next to usescript.":
        "Abrir junto a usescript.",
    "**Find a trigger.** Triggers are brushes of the map: type `trigger` into the search box of the entity list and pick one where the player passes. Double-click flies there.":
        "**Busca un trigger.** Los triggers son brushes del mapa: escribe `trigger` en el campo de búsqueda de la lista de entidades y elige uno por donde pase el jugador. Doble clic vuela hasta allí.",
    "Searching for triggers.":
        "Buscando triggers.",
    "**Point it at the runner.** In the trigger's inspector set its key `target` to the runner's name.":
        "**Apúntalo al ejecutor.** En el inspector del trigger, pon su clave `target` al nombre del ejecutor.",
    "**Once or every time.** A runner runs once. Type `count` into the new-key field, press **{{EntAddKey}}** and set it to `-1` to run it every time. For only-the-first-time logic see [[k-variablen]].":
        "**Una vez o siempre.** Un ejecutor se ejecuta una vez. Escribe `count` en el campo de clave nueva, pulsa **{{EntAddKey}}** y ponla a `-1` para que se ejecute siempre. Para la lógica de solo la primera vez, mira [[k-variablen]].",
    "A new key in the inspector.":
        "Una clave nueva en el inspector.",
    "**Find the door.** Doors are in the group **Movable (func_*)** of the entity list. Open the group and click one.":
        "**Busca la puerta.** Las puertas están en el grupo **Móviles (func_*)** de la lista de entidades. Abre el grupo y pulsa una.",
    "The group of movable entities.":
        "El grupo de entidades móviles.",
    "**Its name.** The door needs a name (`targetname`). A door with a name waits to be used instead of opening when someone walks up - which is what a scripted door should do.":
        "**Su nombre.** La puerta necesita un nombre (`targetname`). Una puerta con nombre espera a ser usada en vez de abrirse cuando alguien se acerca, que es lo que debe hacer una puerta con script.",
    "The door's name in the inspector.":
        "El nombre de la puerta en el inspector.",
    "**Insert use.** Select the line in the script after which the door should open, then right-click the door in the entity list → **{{EntityUse}}**.":
        "**Inserta use.** Selecciona la línea del script tras la que debe abrirse la puerta y luego haz clic derecho en la puerta en la lista de entidades → **{{EntityUse}}**.",
    "Insert use in the context menu.":
        "Insertar use en el menú contextual.",
    "**The result.** `use ( \"bldg2_ext_door\" )` is in the script. The door plays its own sounds; play the timeline to check the moment.":
        "**El resultado.** `use ( \"bldg2_ext_door\" )` está en el script. La puerta reproduce sus propios sonidos; reproduce la línea de tiempo para comprobar el momento.",
    "The use line.":
        "La línea use.",
    "**Place it.** **{{EntNew}}** above the entity list → **{{EntKindLevel}}**.":
        "**Colócala.** **{{EntNew}}** encima de la lista de entidades → **{{EntKindLevel}}**.",
    "Level change in the + Entity menu.":
        "Cambio de nivel en el menú + Entidad.",
    "**Set it up.** In the inspector set `mapname` to the next map (for example `taspir2`) and give it a **{{EntName}}** such as `next_map`.":
        "**Prepárala.** En el inspector pon `mapname` al mapa siguiente (por ejemplo `taspir2`) y dale un **{{EntName}}** como `siguiente_mapa`.",
    "**Use it.** At the end of the script: `use ( \"next_map\" )` - right-click the entity → **{{EntityUse}}** writes it.":
        "**Úsala.** Al final del script: `use ( \"siguiente_mapa\" )`; clic derecho en la entidad → **{{EntityUse}}** lo escribe.",
    "use ( \"next_map\" ).":
        "use ( \"siguiente_mapa\" ).",
    "The checks of Save mission.":
        "Las comprobaciones de Guardar misión.",
    "**An effect.** Click the tile **{{EntKindFx}}** and click where it should appear.":
        "**Un efecto.** Pulsa la ficha **{{EntKindFx}}** y pulsa donde debe aparecer.",
    "The tile for an effect.":
        "La ficha de un efecto.",
    "**Set it up.** In the inspector set `fxFile` to an effect of the game (a path under `effects/`, for example `env/fire`), give it a name and tick `STARTOFF` so it waits.":
        "**Prepáralo.** En el inspector pon `fxFile` a un efecto del juego (una ruta bajo `effects/`, por ejemplo `env/fire`), dale un nombre y marca `STARTOFF` para que espere.",
    "A fire on the landing pad.":
        "Un fuego en la plataforma de aterrizaje.",
    "**Start it.** In the script: `use ( \"fire1\" )` at the right moment.":
        "**Inícialo.** En el script: `use ( \"fuego1\" )` en el momento justo.",
    "use starts the effect.":
        "use inicia el efecto.",
    "Music in the + Entity menu.":
        "Música en el menú + Entidad.",
    "SET_MUSIC_STATE.":
        "SET_MUSIC_STATE.",
    "The group with Move.":
        "El grupo con Mover.",
    "The group with Rotate.":
        "El grupo con Rotar.",
    "Duplicate copies every selected entity.":
        "Duplicar copia cada entidad seleccionada.",
    "**Two names.** A character spawner has its **{{EntName}}** - the `NPC_targetname`, the name of the figure that `affect` uses - and may have a `targetname`, the name of the spawner itself.":
        "**Dos nombres.** Un spawner de personaje tiene su **{{EntName}}** (el `NPC_targetname`, el nombre de la figura que usa `affect`) y puede tener un `targetname`, el nombre del propio spawner.",
    "Name and NPC_type of a spawner.":
        "Nombre y NPC_type de un spawner.",
    "use calls the spawner.":
        "use llama al spawner.",
    "The spawnflags.":
        "Los spawnflags.",
    "Open .ent in the Maps & files menu.":
        "Abrir .ent en el menú Mapas y archivos.",
    "The entity list.":
        "La lista de entidades.",
    "The files of the package, among them maps/taspir_tut.ent.":
        "Los archivos del paquete, entre ellos maps/taspir_tut.ent.",
    "**Select the door.** Click it in the view or the list - doors and platforms are `func_` entities. In the inspector press **{{EntAnimate}}**.":
        "**Selecciona la puerta.** Púlsala en la vista o en la lista: las puertas y plataformas son entidades `func_`. En el inspector pulsa **{{EntAnimate}}**.",
    "Animate in the inspector.":
        "Animar en el inspector.",
    "Script (move).":
        "Script (move).",
    "Key at playhead.":
        "Clave en el cabezal.",
    "The second key in the table; the door has moved up.":
        "La segunda clave en la tabla; la puerta ha subido.",
    "Linear.":
        "Lineal.",
    "Animate.":
        "Animar.",
    "**Edit.** In ROFF mode the window lists the keys of the file. A ROFF of the game is first turned into keys with **{{AnimRoffEdit}}** - the game file stays untouched, you edit a copy with its own name.":
        "**Edita.** En el modo ROFF la ventana lista las claves del archivo. Un ROFF del juego se convierte primero en claves con **{{AnimRoffEdit}}**: el archivo del juego no se toca, editas una copia con su propio nombre.",
    "The flight with its keys.":
        "El vuelo con sus claves.",
    "The files of the package, among them the .rof.":
        "Los archivos del paquete, entre ellos el .rof.",
    "**Open the Graph.** Above the script column, switch from **{{FlowClassic}}** to **{{FlowGraph}}**. Every entity and script of the mission is a node; the arrows show what starts what.":
        "**Abre el Grafo.** Encima de la columna del script, cambia de **{{FlowClassic}}** a **{{FlowGraph}}**. Cada entidad y cada script de la misión es un nodo; las flechas muestran qué inicia qué.",
    "**Drag a wire.** Drag from the dot on the right of a character node to a script node. A menu asks **{{GraphConnectAs}}**: `spawnscript` (when it appears), `deathscript`, `usescript` and more - behaved writes the key into the entity.":
        "**Arrastra un cable.** Arrastra desde el punto a la derecha de un nodo de personaje hasta un nodo de script. Un menú pregunta **{{GraphConnectAs}}**: `spawnscript` (cuando aparece), `deathscript`, `usescript` y más; behaved escribe la clave en la entidad.",
    "Character nodes with their dots on the right.":
        "Nodos de personaje con sus puntos a la derecha.",
    "**Check the key.** Select the character: the inspector shows the new key, with **{{EntOpenScript}}** next to it.":
        "**Revisa la clave.** Selecciona el personaje: el inspector muestra la clave nueva, con **{{EntOpenScript}}** al lado.",
    "The script keys of a character.":
        "Las claves de script de un personaje.",
    "**Open it again.** **{{MenuFile}} → {{RecentMissions}}**, or the list on the start page of the **{{ViewMap}}** tab, opens map, entities and every script with one click.":
        "**Vuelve a abrirla.** **{{MenuFile}} → {{RecentMissions}}**, o la lista de la página de inicio de la pestaña **{{ViewMap}}**, abre el mapa, las entidades y todos los scripts con un clic.",
    "Recent missions.":
        "Misiones recientes.",
    "**Work.** Change scripts and entities. A dot next to the mission name in the map toolbar means unsaved changes.":
        "**Trabaja.** Cambia scripts y entidades. Un punto junto al nombre de la misión en la barra del mapa indica cambios sin guardar.",
    "The mission name with its dot.":
        "El nombre de la misión con su punto.",
    "**Save mission.** **{{MissionSave}}** shows every file it will pack and checks the mission: errors in red must be fixed, warnings in orange may stay.":
        "**Guarda la misión.** **{{MissionSave}}** muestra cada archivo que va a empaquetar y comprueba la misión: los errores en rojo hay que corregirlos, los avisos en naranja pueden quedarse.",
    "**Open the window.** **{{WegTitel}}** on the map toolbar.":
        "**Abre la ventana.** **{{WegTitel}}** en la barra del mapa.",
    "NPC waypoints on the toolbar.":
        "Waypoints de NPC en la barra.",
    "**Show and create.** **{{WegZeigen}}** shows the network - green by hand, blue automatic, orange jumps. **{{WegErzeugen}}** spreads waypoints over the walkable floor; the spacing slider sets how dense.":
        "**Muestra y crea.** **{{WegZeigen}}** muestra la red: verde a mano, azul automática, naranja saltos. **{{WegErzeugen}}** reparte waypoints por el suelo transitable; el deslizador de separación fija la densidad.",
    "Check network.":
        "Comprobar red.",
    "Place waypoints and Connect.":
        "Colocar waypoints y Conectar.",
    "**Switch to {{ViewModel}}** in the mode bar on the left.":
        "**Cambia a {{ViewModel}}** en la barra de modos de la izquierda.",
    "Model in the mode bar.":
        "Modelo en la barra de modos.",
    "Skin, Bones and Textures.":
        "Skin, Huesos y Texturas.",
    "**Find the scene.** In the **{{ViewMap}}** tab, **Scripts (N)** on the map toolbar lists the scripts the map uses - pick the intro. The timeline shows its camera.":
        "**Busca la escena.** En la pestaña **{{ViewMap}}**, **Scripts (N)** en la barra del mapa lista los scripts que usa el mapa: elige la introducción. La línea de tiempo muestra su cámara.",
    "The scripts of the map.":
        "Los scripts del mapa.",
    "**Change a shot.** Select a shot in **{{CamShots}}**, fly to a better view and press **{{CamSetToView}}** - or change a time in the timeline.":
        "**Cambia un plano.** Selecciona un plano en **{{CamShots}}**, vuela a una vista mejor y pulsa **{{CamSetToView}}**, o cambia un tiempo en la línea de tiempo.",
    "Save mission.":
        "Guardar misión.",
    "**Undo it.** Delete the `zz_` file from the game folder (the message after saving has **{{ToastExplorer}}**) and the game plays its original again - the game files are never touched.":
        "**Deshazlo.** Borra el archivo `zz_` de la carpeta del juego (el mensaje tras guardar tiene **{{ToastExplorer}}**) y el juego vuelve a reproducir el original: los archivos del juego nunca se tocan.",
    "Show in Explorer opens the folder.":
        "Mostrar en el Explorador abre la carpeta.",
    "**Switch.** **{{MenuView}} → {{ViewTarget}}** → **{{TargetMp}}**. An **MP** badge appears in the status line.":
        "**Cambia.** **{{MenuView}} → {{ViewTarget}}** → **{{TargetMp}}**. Aparece una insignia **MP** en la línea de estado.",
    "Target game: multiplayer.":
        "Juego de destino: multijugador.",
    "**Switch back** to **{{TargetSp}}** when you are done; the script itself is not changed.":
        "**Vuelve** a **{{TargetSp}}** cuando termines; el script en sí no cambia.",
    "Target game: single player.":
        "Juego de destino: un jugador.",
    "**Theme.** **{{MenuView}} → {{ViewTheme}}**: six themes, among them BehavEd Classic and High Contrast.":
        "**Tema.** **{{MenuView}} → {{ViewTheme}}**: seis temas, entre ellos BehavEd clásico y Alto contraste.",
    "Theme in the View menu.":
        "Tema en el menú Ver.",
    "**Language.** **{{MenuView}} → {{ViewLanguage}}**: English, Deutsch, 中文, 日本語, Español, Čeština.":
        "**Idioma.** **{{MenuView}} → {{ViewLanguage}}**: English, Deutsch, 中文, 日本語, Español, Čeština.",
    "Language in the View menu.":
        "Idioma en el menú Ver.",
    "Interface Scale in the View menu.":
        "Escala de la interfaz en el menú Ver.",
    "**Columns.** Drag the dividers between the columns; each mode keeps its own widths. **{{MenuDebug}} → {{DebugReset}}** brings them back.":
        "**Columnas.** Arrastra los separadores entre las columnas; cada modo guarda sus propios anchos. **{{MenuDebug}} → {{DebugReset}}** los devuelve.",
    "Restore default layout in the Debug menu.":
        "Restaurar la disposición predeterminada en el menú Depuración.",
    "**The guide.** <kbd>F1</kbd> opens this guide - also **{{MenuHelp}} → {{HelpGuide}}**.":
        "**La guía.** <kbd>F1</kbd> abre esta guía; también **{{MenuHelp}} → {{HelpGuide}}**.",
    "Guide and tutorials in the Help menu.":
        "Guía y tutoriales en el menú Ayuda.",
    "**Give the figure a deathscript.** Select the enemy. In the inspector add the key `deathscript` with a script name of your mission, for example `taspir_tut/rosh_dies` - or connect it in the **{{FlowGraph}}** view as `deathscript`.":
        "**Dale a la figura un deathscript.** Selecciona al enemigo. En el inspector añade la clave `deathscript` con un nombre de script de tu misión, por ejemplo `taspir_tut/rosh_muere`, o conéctalo en la vista **{{FlowGraph}}** como `deathscript`.",
    "**Create the script.** Press **{{EntOpenScript}}** next to the key: the script is created in the mission folder and opened in a tab.":
        "**Crea el script.** Pulsa **{{EntOpenScript}}** junto a la clave: el script se crea en la carpeta de la misión y se abre en una pestaña.",
    "Open next to deathscript.":
        "Abrir junto a deathscript.",
    "**Other moments.** The same works with `painscript` (hit), `awakescript` (notices the player), `angerscript` (starts fighting), `fleescript` and `spawnscript` - all listed in the inspector.":
        "**Otros momentos.** Lo mismo funciona con `painscript` (golpeado), `awakescript` (ve al jugador), `angerscript` (empieza a luchar), `fleescript` y `spawnscript`, todos listados en el inspector.",
    "SET_PLAYER_USABLE in the spawn script.":
        "SET_PLAYER_USABLE en el script de aparición.",
    "A voice line.":
        "Una frase de voz.",
    "SET_PLAYER_USABLE false at the end.":
        "SET_PLAYER_USABLE false al final.",
    "run and waitsignal in the intro, signal in talk.":
        "run y waitsignal en la introducción, signal en la charla.",
    "**Hand over.** Where the intro must wait for the other part, insert `waitsignal ( \"talk_done\" )`; at the end of the other script `signal ( \"talk_done\" )` - see [[k-signale]].":
        "**Pasa el testigo.** Donde la introducción debe esperar a la otra parte, inserta `waitsignal ( \"charla_hecha\" )`; al final del otro script, `signal ( \"charla_hecha\" )`; mira [[k-signale]].",
    "The same signal name on both sides.":
        "El mismo nombre de señal en los dos lados.",
    "**S001 (warning) - a script is missing.** An entity names a script that does not exist yet. Select the entity and press **{{EntOpenScript}}** next to the key: the script is created.":
        "**S001 (aviso): falta un script.** Una entidad nombra un script que aún no existe. Selecciona la entidad y pulsa **{{EntOpenScript}}** junto a la clave: se crea el script.",
    "Open next to a script key.":
        "Abrir junto a una clave de script.",
    "Compile shows the wrong line.":
        "Compilar muestra la línea errónea.",
    "The tile for a player start.":
        "La ficha de un inicio del jugador.",
    "**S005 (warning) - a target points to nothing.** A `target` names an entity that does not exist - often a leftover of the original map, like the one in the first picture.":
        "**S005 (aviso): un target no apunta a nada.** Un `target` nombra una entidad que no existe; a menudo es un resto del mapa original, como el de la primera imagen.",
    "S005 at the bottom of the list.":
        "S005 al final de la lista.",
    "The mission name is chosen when the mission is created.":
        "El nombre de la misión se elige al crear la misión.",
    "**Place the round.** In the **{{ViewMap}}** tab click the tile **{{EntKindNav}}** and place three walk targets (<kbd>Shift</kbd> places several); name them `round1`, `round2`, `round3`.":
        "**Coloca la ronda.** En la pestaña **{{ViewMap}}** pulsa la ficha **{{EntKindNav}}** y coloca tres destinos (<kbd>Mayús</kbd> coloca varios); llámalos `ronda1`, `ronda2`, `ronda3`.",
    "The tile for walk targets.":
        "La ficha de los destinos.",
    "**The guard.** Place a character, name it `guard`, and give it an `affect ( \"guard\", FLUSH )` block. Inside, insert the macro `walkOnly` first, then `loop` with `-1` - forever.":
        "**El guardia.** Coloca un personaje, llámalo `guard` y dale un bloque `affect ( \"guard\", FLUSH )`. Dentro, inserta primero la macro `walkOnly` y luego `loop` con `-1`: para siempre.",
    "walkOnly and the loop in the guard's block.":
        "walkOnly y el bucle en el bloque del guardia.",
    "**One leg per point.** Inside the loop: a `task` with `set ( SET_NAVGOAL, round1 )`, then `dowait` for it, then `wait ( 1000 )`. Copy these lines for each point and change the name.":
        "**Un tramo por punto.** Dentro del bucle: una `task` con `set ( SET_NAVGOAL, ronda1 )`, luego `dowait` para ella y luego `wait ( 1000 )`. Copia estas líneas para cada punto y cambia el nombre.",
    "A task, dowait and wait for each point.":
        "Una task, dowait y wait por cada punto.",
    "**Watch.** Play the timeline - the guard walks the round.":
        "**Mira.** Reproduce la línea de tiempo: el guardia hace la ronda.",
    "**Switch skins.** In the **{{ViewModel}}** tab the **{{ModelSkin}}** list in the playback bar shows the skins that come with the model; **{{OpenSkin}}** loads another one.":
        "**Cambia de skin.** En la pestaña **{{ViewModel}}**, la lista **{{ModelSkin}}** de la barra de reproducción muestra las skins que trae el modelo; **{{OpenSkin}}** carga otra.",
    "**Surfaces.** The counter next to it shows how many surfaces the skin draws; click it to list them all in the detail log.":
        "**Superficies.** El contador de al lado muestra cuántas superficies dibuja la skin; púlsalo para listarlas todas en el registro detallado.",
    "The surface counter.":
        "El contador de superficies.",
    "Show caps.":
        "Mostrar tapas.",
    "Load .gla.":
        "Cargar .gla.",
    "The runner of the original, found with the search box.":
        "El ejecutor del original, encontrado con el campo de búsqueda.",
    "The tile for a character.":
        "La ficha de un personaje.",
    "Rosh's name and NPC_type.":
        "El nombre y el NPC_type de Rosh.",
    "Rosh's block selected, with Copy and Paste.":
        "El bloque de Rosh seleccionado, con Copiar y Pegar.",
    "The first shot selected, with Set to view.":
        "El primer plano seleccionado, con Ajustar a la vista.",
    "The third shot selected in the camera panel - its Travel time is below the list.":
        "El tercer plano seleccionado en el panel de la cámara; su tiempo de viaje está debajo de la lista.",
    "The two PAN commands that follow the ship.":
        "Los dos comandos PAN que siguen a la nave.",
    "camera ( FADE ): from transparent to black.":
        "camera ( FADE ): de transparente a negro.",
    "Kyle's fields and + Key.":
        "Los campos de Kyle y + Clave.",
    "Open next to spawnscript.":
        "Abrir junto a spawnscript.",
    "The window names the .pk3 it writes into the game folder.":
        "La ventana nombra el .pk3 que escribe en la carpeta del juego.",
    "**Switch to Graph.** Click **{{FlowGraph}}** above the script area. The graph shows the mission: the player start, the scripts and the entities that start them.":
        "**Cambia a Grafo.** Pulsa **{{FlowGraph}}** encima de la zona del script. El grafo muestra la misión: el inicio del jugador, los scripts y las entidades que los inician.",
    "The view switch above the script area: Graph.":
        "El cambio de vista encima de la zona del script: Grafo.",
    "**Drag affect onto free space.** Drag `affect` from the **{{GroupEvents}}** list (1) onto an empty spot of the graph (2). A new script is created in the mission folder, with `affect` as its first command, and opens in its own tab.":
        "**Arrastra affect a un espacio libre.** Arrastra `affect` de la lista **{{GroupEvents}}** (1) a un sitio vacío del grafo (2). Se crea un script nuevo en la carpeta de la misión, con `affect` como primer comando, y se abre en su propia pestaña.",
    "1: affect in the Events list. 2: free space in the graph.":
        "1: affect en la lista de eventos. 2: espacio libre en el grafo.",
    "**The new node.** The new script appears as a node where you dropped it, and it is selected. On the right, **{{GraphCommands}}** shows its one line: `affect`.":
        "**El nodo nuevo.** El script nuevo aparece como un nodo donde lo soltaste, y queda seleccionado. A la derecha, **{{GraphCommands}}** muestra su única línea: `affect`.",
    "1: the new script node. 2: its commands on the right.":
        "1: el nodo de script nuevo. 2: sus comandos a la derecha.",
    "**Name the figure.** Double-click the `affect` line on the right. The **{{EditorTitle}}** opens - the same as in Classic. Type the name of the figure into the first field and press **{{EditorOk}}**.":
        "**Pon nombre a la figura.** Haz doble clic en la línea `affect` de la derecha. Se abre el **{{EditorTitle}}**, el mismo que en Clásica. Escribe el nombre de la figura en el primer campo y pulsa **{{EditorOk}}**.",
    "**Add the next command into the block.** Click the `affect` line (1), then double-click `wait` in the list (2). Because `affect` is an open block, `wait` goes inside it.":
        "**Añade el siguiente comando al bloque.** Pulsa la línea `affect` (1) y luego haz doble clic en `wait` en la lista (2). Como `affect` es un bloque abierto, `wait` va dentro.",
    "1: the selected affect line. 2: wait in the Events list.":
        "1: la línea affect seleccionada. 2: wait en la lista de eventos.",
    "**Check the result.** `wait` stands indented under `affect` - it is inside the block. The node counts the commands of the script.":
        "**Revisa el resultado.** `wait` está sangrado bajo `affect`: está dentro del bloque. El nodo cuenta los comandos del script.",
    "wait inside the affect block, in the list on the right.":
        "wait dentro del bloque affect, en la lista de la derecha.",
    "**Save.** **{{FileSave}}** in the action column (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The script already has its place in the mission folder.":
        "**Guarda.** **{{FileSave}}** en la columna de acciones (o <kbd>Ctrl</kbd>+<kbd>S</kbd>). El script ya tiene su sitio en la carpeta de la misión.",
    "**Open an empty tab and switch to Tracks.** Click **+** at the end of the tab bar, then **{{FlowTracks}}** above the script area. The timeline says that nothing is on it yet - and on the right, **{{TracksList}}** says *No commands yet*.":
        "**Abre una pestaña vacía y cambia a Pistas.** Pulsa **+** al final de la barra de pestañas y luego **{{FlowTracks}}** encima de la zona del script. La línea de tiempo dice que aún no hay nada en ella, y a la derecha **{{TracksList}}** dice *Aún no hay comandos*.",
    "1: Tracks. 2: the empty command list on the right.":
        "1: Pistas. 2: la lista de comandos vacía a la derecha.",
    "**Drag affect into the list.** Drag `affect` from the **{{GroupEvents}}** list (1) into the command list on the right (2). An `affect` block has no time of its own, so it shows in the list, not yet in the timeline.":
        "**Arrastra affect a la lista.** Arrastra `affect` de la lista **{{GroupEvents}}** (1) a la lista de comandos de la derecha (2). Un bloque `affect` no ocupa tiempo propio, así que aparece en la lista, aún no en la línea de tiempo.",
    "1: affect in the Events list. 2: the command list.":
        "1: affect en la lista de eventos. 2: la lista de comandos.",
    "**Name the figure.** Double-click the `affect` line in the list. The **{{EditorTitle}}** opens: type the name of the figure into the first field and press **{{EditorOk}}**.":
        "**Pon nombre a la figura.** Haz doble clic en la línea `affect` de la lista. Se abre el **{{EditorTitle}}**: escribe el nombre de la figura en el primer campo y pulsa **{{EditorOk}}**.",
    "**Add a walk into the block.** Click the `affect` line (1), then double-click `set` in the **{{GroupEvents}}** list (2) - it goes inside the open block. In the editor choose `SET_NAVGOAL` and the walk target `nav_kyle`.":
        "**Añade una marcha al bloque.** Pulsa la línea `affect` (1) y luego haz doble clic en `set` en la lista **{{GroupEvents}}** (2): va dentro del bloque abierto. En el editor elige `SET_NAVGOAL` y el destino `nav_kyle`.",
    "1: the affect line. 2: set in the Events list.":
        "1: la línea affect. 2: set en la lista de eventos.",
    "**Now it is on the timeline.** The figure got its own row, and the walk is a block on it - as long as the figure needs to get there. Commands without time (`affect`, the block itself) stay in the list.":
        "**Ahora está en la línea de tiempo.** La figura recibe su propia fila, y la marcha es un bloque en ella, tan largo como lo que tarda la figura en llegar. Los comandos sin tiempo (`affect`, el propio bloque) se quedan en la lista.",
    "Kyle's row with the walk, and the list on the right.":
        "La fila de Kyle con la marcha, y la lista a la derecha.",
    "**Save.** **{{FileSave}}** in the action column (or <kbd>Ctrl</kbd>+<kbd>S</kbd>). The first time, behaved asks for a name.":
        "**Guarda.** **{{FileSave}}** en la columna de acciones (o <kbd>Ctrl</kbd>+<kbd>S</kbd>). La primera vez, behaved pide un nombre.",
    "**Where the commands are.** In **{{FlowGraph}}**, click a script node (the open script is selected at the start). On the right, under **{{GraphCommands}}**, every command of that script is listed - blocks indented. Click a line to select it, double-click to edit it, right-click for the menu.":
        "**Dónde están los comandos.** En **{{FlowGraph}}**, pulsa un nodo de script (el script abierto está seleccionado al principio). A la derecha, en **{{GraphCommands}}**, se lista cada comando de ese script, con los bloques sangrados. Clic en una línea la selecciona, doble clic la edita, clic derecho abre el menú.",
    "The open script's node (1) and its commands (2).":
        "El nodo del script abierto (1) y sus comandos (2).",
    "**Double-click in the Events list.** Click a line on the right (1), then double-click a command in the **{{GroupEvents}}** list (2). It is inserted after that line - into the block, if the line is an open block.":
        "**Doble clic en la lista de eventos.** Pulsa una línea de la derecha (1) y luego haz doble clic en un comando de la lista **{{GroupEvents}}** (2). Se inserta tras esa línea; dentro del bloque si la línea es un bloque abierto.",
    "1: the selected line. 2: the command in the Events list.":
        "1: la línea seleccionada. 2: el comando en la lista de eventos.",
    "**Drag onto a line.** Drag a command from the **{{GroupEvents}}** list (1) onto a line of **{{GraphCommands}}** (2): it lands right after that line.":
        "**Arrastra a una línea.** Arrastra un comando de la lista **{{GroupEvents}}** (1) a una línea de **{{GraphCommands}}** (2): cae justo después de esa línea.",
    "1: the command. 2: the line it goes after.":
        "1: el comando. 2: la línea tras la que va.",
    "**Drag onto the node.** Dropped on a script node in the graph (2), the command goes to the end of that script. Dropped on free space, it starts a new script.":
        "**Arrastra al nodo.** Soltado sobre un nodo de script del grafo (2), el comando va al final de ese script. Soltado en un espacio libre, empieza un script nuevo.",
    "1: the command. 2: the script node - the command goes to its end.":
        "1: el comando. 2: el nodo de script; el comando va a su final.",
    "**Right-click.** Right-click a line on the right: the submenus **{{GroupEvents}}** and **Macros** insert after that line - the same menu as in Classic.":
        "**Clic derecho.** Haz clic derecho en una línea de la derecha: los submenús **{{GroupEvents}}** y **Macros** insertan tras esa línea, el mismo menú que en Clásica.",
    "The menu of a line in the Graph, with its Events submenu.":
        "El menú de una línea en el Grafo, con su submenú Eventos.",
    "**Where the commands are.** **{{FlowTracks}}** shows the script over time: the camera, one row per figure, the waits. On the right, **{{TracksList}}** lists every command - also those that take no time, such as `affect`, `task` or `declare`.":
        "**Dónde están los comandos.** **{{FlowTracks}}** muestra el script en el tiempo: la cámara, una fila por figura, las esperas. A la derecha, **{{TracksList}}** lista cada comando, también los que no ocupan tiempo, como `affect`, `task` o `declare`.",
    "1: the rows of the timeline. 2: the command list.":
        "1: las filas de la línea de tiempo. 2: la lista de comandos.",
    "**Double-click in the Events list.** Click a block or a line of the list (1), then double-click a command in the **{{GroupEvents}}** list (2). It goes right after the selected command.":
        "**Doble clic en la lista de eventos.** Pulsa un bloque o una línea de la lista (1) y luego haz doble clic en un comando de la lista **{{GroupEvents}}** (2). Va justo después del comando seleccionado.",
    "1: the selected block. 2: the command in the Events list.":
        "1: el bloque seleccionado. 2: el comando en la lista de eventos.",
    "**Drag into a row at a moment.** Drag a command from the **{{GroupEvents}}** list (1) into a row of the timeline (2). A yellow line shows the moment. The command is placed after the last command of that row that starts before the line - in a figure's row, inside that figure's `affect` block.":
        "**Arrastra a una fila en un momento.** Arrastra un comando de la lista **{{GroupEvents}}** (1) a una fila de la línea de tiempo (2). Una línea amarilla muestra el momento. El comando se coloca tras el último comando de esa fila que empiece antes de la línea; en la fila de una figura, dentro del bloque `affect` de esa figura.",
    "1: the command. 2: the moment in Kyle's row.":
        "1: el comando. 2: el momento en la fila de Kyle.",
    "**Drag into the list.** For commands without time - an `affect`, a `task`, a `declare` - drag into the list on the right (2): after the line you drop on, or at the end on free space.":
        "**Arrastra a la lista.** Para comandos sin tiempo (un `affect`, una `task`, un `declare`), arrastra a la lista de la derecha (2): tras la línea sobre la que sueltas, o al final en un espacio libre.",
    "1: the command. 2: the line in the command list.":
        "1: el comando. 2: la línea en la lista de comandos.",
    "**Right-click.** Right-click a block (or a line of the list): **{{GroupEvents}}** and **Macros** insert after it.":
        "**Clic derecho.** Haz clic derecho en un bloque (o en una línea de la lista): **{{GroupEvents}}** y **Macros** insertan tras él.",
    "The menu of a block, with its Events submenu.":
        "El menú de un bloque, con su submenú Eventos.",
    "**Open the editor.** In **{{FlowGraph}}**, double-click a line under **{{GraphCommands}}** (or select it and choose **Edit...** from its right-click menu). The **{{EditorTitle}}** is the same in every view.":
        "**Abre el editor.** En **{{FlowGraph}}**, haz doble clic en una línea de **{{GraphCommands}}** (o selecciónala y elige **Editar...** en su menú de clic derecho). El **{{EditorTitle}}** es el mismo en todas las vistas.",
    "Double-click a line of the list to edit it.":
        "Doble clic en una línea de la lista para editarla.",
    "**Fields with fixed choices.** Fields like `SET_TYPES` or `true` / `false` are lists you can also type into. A value that is not in the list is allowed, but marked **unknown**.":
        "**Campos con opciones fijas.** Los campos como `SET_TYPES` o `true` / `false` son listas en las que también puedes escribir. Se permite un valor que no esté en la lista, pero se marca como **desconocido**.",
    "**Expressions and free text.** **{{EditorHelper}}** builds `get`, `tag` and `random`; **{{EditorExpr}}** turns a field into free text.":
        "**Expresiones y texto libre.** **{{EditorHelper}}** construye `get`, `tag` y `random`; **{{EditorExpr}}** convierte un campo en texto libre.",
    "**Done.** **{{EditorOk}}** writes the values, **{{EditorCancel}}** leaves everything as it was. The line on the right shows the new values at once.":
        "**Listo.** **{{EditorOk}}** escribe los valores, **{{EditorCancel}}** lo deja todo como estaba. La línea de la derecha muestra enseguida los valores nuevos.",
    "**Open the editor.** In **{{FlowTracks}}**, double-click a block - or a line of the command list. The **{{EditorTitle}}** is the same in every view.":
        "**Abre el editor.** En **{{FlowTracks}}**, haz doble clic en un bloque o en una línea de la lista de comandos. El **{{EditorTitle}}** es el mismo en todas las vistas.",
    "Double-click a block to edit its command.":
        "Doble clic en un bloque para editar su comando.",
    "**Change a time by dragging.** Drag the right edge of a block: a `wait`, a camera move or a walk gets longer or shorter. The value in the script changes with it.":
        "**Cambia un tiempo arrastrando.** Arrastra el borde derecho de un bloque: un `wait`, un movimiento de cámara o una marcha se alarga o se acorta. El valor del script cambia con él.",
    "The right edge of the wait block.":
        "El borde derecho del bloque de espera.",
    "**Fields for values.** Numbers, vectors and text are typed into their fields.":
        "**Campos para valores.** Los números, vectores y texto se escriben en sus campos.",
    "**Fields with fixed choices.** Lists you can also type into; an unknown value is allowed but marked.":
        "**Campos con opciones fijas.** Listas en las que también puedes escribir; se permite un valor desconocido, pero se marca.",
    "**Done.** **{{EditorOk}}** writes the values; the block moves or changes its length at once.":
        "**Listo.** **{{EditorOk}}** escribe los valores; el bloque se mueve o cambia de longitud enseguida.",
    "**Select.** Click a line under **{{GraphCommands}}**. The action column on the right works on it, as in Classic.":
        "**Selecciona.** Pulsa una línea de **{{GraphCommands}}**. La columna de acciones de la derecha actúa sobre ella, como en Clásica.",
    "The selected line.":
        "La línea seleccionada.",
    "**Right-click for the rest.** The menu of a line has everything: **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** and **{{ActDelete}}**.":
        "**Clic derecho para lo demás.** El menú de una línea lo tiene todo: **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** y **{{ActDelete}}**.",
    "The menu of a line in the Graph.":
        "El menú de una línea en el Grafo.",
    "Edit...":
        "Editar...",
    "Opens the event editor.":
        "Abre el editor de eventos.",
    "A copy right below.":
        "Una copia justo debajo.",
    "Copy / Cut / Paste":
        "Copiar / Cortar / Pegar",
    "Through the clipboard - also into another script.":
        "Por el portapapeles, también a otro script.",
    "Move up / Move down":
        "Subir / Bajar",
    "One line up or down, inside its block.":
        "Una línea arriba o abajo, dentro de su bloque.",
    "REM":
        "REM",
    "Switches the line off (comment) or on again.":
        "Desactiva la línea (comentario) o la vuelve a activar.",
    "Removes the line.":
        "Quita la línea.",
    "**Or the action column.** **{{ActMoveUp}}** / **{{ActMoveDown}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActPaste}}** and **{{ActRem}}** on the right do the same for the selected line.":
        "**O la columna de acciones.** **{{ActMoveUp}}** / **{{ActMoveDown}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActPaste}}** y **{{ActRem}}** a la derecha hacen lo mismo con la línea seleccionada.",
    "Move up and Move down in the action column.":
        "Subir y Bajar en la columna de acciones.",
    "**Select.** Click a block in the timeline - or a line of the command list.":
        "**Selecciona.** Pulsa un bloque en la línea de tiempo, o una línea de la lista de comandos.",
    "The selected block.":
        "El bloque seleccionado.",
    "**Right-click for the rest.** The menu of a block has **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** and **{{ActDelete}}**.":
        "**Clic derecho para lo demás.** El menú de un bloque tiene **{{ActEdit}}**, **{{ActClone}}**, **{{ActCopy}}**, **{{ActCut}}**, **{{ActPaste}}**, **{{MoveUp}}**, **{{MoveDown}}**, **{{ActRem}}** y **{{ActDelete}}**.",
    "The menu of a block in Tracks.":
        "El menú de un bloque en Pistas.",
    "A copy right after it - on the timeline it follows the first.":
        "Una copia justo detrás; en la línea de tiempo sigue al primero.",
    "Through the clipboard.":
        "Por el portapapeles.",
    "One line up or down in the script - that changes the order in time.":
        "Una línea arriba o abajo en el script: eso cambia el orden en el tiempo.",
    "Switches the command off or on.":
        "Desactiva o activa el comando.",
    "**Moving changes the time.** Moving a `wait` up or down shifts everything after it - watch the blocks move.":
        "**Mover cambia el tiempo.** Subir o bajar un `wait` desplaza todo lo que va detrás: mira cómo se mueven los bloques.",
    "After Move up: the blocks after it start earlier.":
        "Tras Subir: los bloques posteriores empiezan antes.",
    "**Build the task.** Click the `affect` line, double-click `task` in the **{{GroupEvents}}** list and name it `walk` in the editor. Then click the `task` line and double-click `set` twice - for `SET_WALKING` and `SET_NAVGOAL`.":
        "**Construye la tarea.** Pulsa la línea `affect`, haz doble clic en `task` en la lista **{{GroupEvents}}** y llámala `walk` en el editor. Luego pulsa la línea `task` y haz doble clic en `set` dos veces, para `SET_WALKING` y `SET_NAVGOAL`.",
    "The task walk with the walk inside, in the list.":
        "La tarea walk con la marcha dentro, en la lista.",
    "**dowait after it.** Drag `dowait` from the **{{GroupEvents}}** list onto the `task` line: a command dropped on a line goes *after* it - after the whole block. Name the task `walk` in the editor.":
        "**dowait detrás.** Arrastra `dowait` de la lista **{{GroupEvents}}** a la línea `task`: un comando soltado sobre una línea va *detrás* de ella, detrás de todo el bloque. Pon `walk` como tarea en el editor.",
    "dowait walk after the task.":
        "dowait walk tras la tarea.",
    "**Check it in Tracks.** Switch to **{{FlowTracks}}**: what follows the `dowait` starts where the walk ends.":
        "**Compruébalo en Pistas.** Cambia a **{{FlowTracks}}**: lo que sigue al `dowait` empieza donde termina la marcha.",
    "Kyle's row: the look starts after the walk.":
        "La fila de Kyle: la mirada empieza tras la marcha.",
    "**Build the task in the list.** A `task` has no time of its own - build it in the command list: click the `affect` line, double-click `task`, name it `walk`; click the `task` line and double-click `set` twice for `SET_WALKING` and `SET_NAVGOAL`.":
        "**Construye la tarea en la lista.** Una `task` no ocupa tiempo propio: constrúyela en la lista de comandos. Pulsa la línea `affect`, haz doble clic en `task` y llámala `walk`; pulsa la línea `task` y haz doble clic en `set` dos veces para `SET_WALKING` y `SET_NAVGOAL`.",
    "The task walk in the command list.":
        "La tarea walk en la lista de comandos.",
    "**dowait.** Drag `dowait` onto the `task` line of the list - it goes after the block - and name the task `walk`. In the figure's row, everything after it now starts where the walk ends.":
        "**dowait.** Arrastra `dowait` a la línea `task` de la lista (va detrás del bloque) y pon `walk` como tarea. En la fila de la figura, todo lo que va detrás empieza ahora donde termina la marcha.",
    "**Two at once.** With `do` instead of `dowait`, the next command starts at once - its block moves to the start of the walk.":
        "**Dos a la vez.** Con `do` en vez de `dowait`, el comando siguiente empieza enseguida: su bloque se mueve al inicio de la marcha.",
    "With do: the look starts with the walk.":
        "Con do: la mirada empieza con la marcha.",
    "**Both scripts in the graph.** The graph shows which entity starts which script. Click the script that should wait - here the intro - and insert `waitsignal` into its list (select a line, double-click `waitsignal`). Name the signal `door_open`.":
        "**Los dos scripts en el grafo.** El grafo muestra qué entidad inicia qué script. Pulsa el script que debe esperar (aquí la introducción) e inserta `waitsignal` en su lista (selecciona una línea, doble clic en `waitsignal`). Llama a la señal `door_open`.",
    "The intro node and its waitsignal.":
        "El nodo de la introducción y su waitsignal.",
    "**The other script.** Click the other script's node. If it is not open, **{{GraphEditHere}}** opens it; then insert `signal ( \"door_open\" )` in its list.":
        "**El otro script.** Pulsa el nodo del otro script. Si no está abierto, **{{GraphEditHere}}** lo abre; luego inserta `signal ( \"door_open\" )` en su lista.",
    "The other script: Edit here opens it.":
        "El otro script: Editar aquí lo abre.",
    "**Check the names.** Both must be spelled exactly the same - only one script can wait for a given name.":
        "**Comprueba los nombres.** Los dos deben escribirse exactamente igual: solo un script puede esperar un nombre dado.",
    "**waitsignal in Tracks.** Insert `waitsignal` into the script that waits (double-click in the **{{GroupEvents}}** list after selecting a block). It shows as a short block in the **Script** row.":
        "**waitsignal en Pistas.** Inserta `waitsignal` en el script que espera (doble clic en la lista **{{GroupEvents}}** tras seleccionar un bloque). Aparece como un bloque corto en la fila **Script**.",
    "waitsignal in the Script row.":
        "waitsignal en la fila Script.",
    "**The timeline cannot know the wait.** How long a `waitsignal` waits depends on the game - everything after it is placed as if the signal came at once.":
        "**La línea de tiempo no puede saber la espera.** Cuánto espera un `waitsignal` depende del juego: todo lo que va después se coloca como si la señal llegara enseguida.",
    "Everything after the waitsignal follows it directly.":
        "Todo lo que va tras el waitsignal lo sigue directamente.",
    "**signal in the other script.** Open the other script's tab and insert `signal ( \"door_open\" )` there - in its list or after one of its blocks.":
        "**signal en el otro script.** Abre la pestaña del otro script e inserta ahí `signal ( \"door_open\" )`, en su lista o tras uno de sus bloques.",
    "**Find them.** The macros are at the end of the **{{GroupEvents}}** list.":
        "**Encuéntralas.** Las macros están al final de la lista **{{GroupEvents}}**.",
    "**Drag one into the list.** Drag a macro (1) onto a line under **{{GraphCommands}}** (2). It shows as one line, `$ standOnly` - its `set` commands are inside.":
        "**Arrastra una a la lista.** Arrastra una macro (1) a una línea de **{{GraphCommands}}** (2). Aparece como una línea, `$ standOnly`; sus comandos `set` están dentro.",
    "1: the macro. 2: the line it goes after.":
        "1: la macro. 2: la línea tras la que va.",
    "**Or right-click.** The menu of a line has a **Macros** submenu.":
        "**O clic derecho.** El menú de una línea tiene un submenú **Macros**.",
    "The menu of a line, with its Macros submenu.":
        "El menú de una línea, con su submenú Macros.",
    "**Drag one into the list.** A macro sets values and takes no time, so it goes into the command list on the right: drag it (1) onto a line (2).":
        "**Arrastra una a la lista.** Una macro fija valores y no ocupa tiempo, así que va a la lista de comandos de la derecha: arrástrala (1) a una línea (2).",
    "1: the macro. 2: the line in the command list.":
        "1: la macro. 2: la línea en la lista de comandos.",
    "**Or right-click.** The menu of a block or a line has a **Macros** submenu.":
        "**O clic derecho.** El menú de un bloque o de una línea tiene un submenú **Macros**.",
    "The menu of a block, with its Macros submenu.":
        "El menú de un bloque, con su submenú Macros.",
    "**Declare.** Select the first line under **{{GraphCommands}}** and double-click `declare` in the **{{GroupEvents}}** list. In the editor choose `FLOAT` and type the name `seen`.":
        "**Declara.** Selecciona la primera línea de **{{GraphCommands}}** y haz doble clic en `declare` en la lista **{{GroupEvents}}**. En el editor elige `FLOAT` y escribe el nombre `visto`.",
    "declare, if and else in the list of the Graph.":
        "declare, if y else en la lista del Grafo.",
    "**Ask with if.** Insert `if` the same way. In the editor, **{{EditorHelper}}** → **{{EditorGet}}** builds `get ( FLOAT, \"seen\" )`; comparison `=`, value `0`.":
        "**Pregunta con if.** Inserta `if` del mismo modo. En el editor, **{{EditorHelper}}** → **{{EditorGet}}** construye `get ( FLOAT, \"visto\" )`; comparación `=`, valor `0`.",
    "**Fill the blocks.** Select the `if` line and insert the scene's commands - they go inside the open block. `else` right after the `if` runs when the scene was seen before; `set ( \"seen\", \"1\" )` at the end of the `if` remembers it.":
        "**Rellena los bloques.** Selecciona la línea `if` e inserta los comandos de la escena: van dentro del bloque abierto. `else` justo tras el `if` se ejecuta cuando la escena ya se vio; `set ( \"visto\", \"1\" )` al final del `if` lo recuerda.",
    "The if and else blocks, indented in the list.":
        "Los bloques if y else, sangrados en la lista.",
    "**Build it in the list.** `declare`, `if` and `else` take no time - build them in the command list on the right, the same way as in the Graph.":
        "**Constrúyelo en la lista.** `declare`, `if` y `else` no ocupan tiempo: constrúyelos en la lista de comandos de la derecha, igual que en el Grafo.",
    "declare, if and else in the command list of Tracks.":
        "declare, if y else en la lista de comandos de Pistas.",
    "**Ask with if.** In the editor, **{{EditorHelper}}** → **{{EditorGet}}** builds `get ( FLOAT, \"seen\" )`; comparison `=`, value `0`.":
        "**Pregunta con if.** En el editor, **{{EditorHelper}}** → **{{EditorGet}}** construye `get ( FLOAT, \"visto\" )`; comparación `=`, valor `0`.",
    "**What the timeline shows.** The timeline plays the commands inside the `if` as if it were true - it cannot know the value in the game.":
        "**Lo que muestra la línea de tiempo.** La línea de tiempo reproduce los comandos dentro del `if` como si fuera verdadero: no puede saber el valor en el juego.",
    "The commands inside the if on the timeline.":
        "Los comandos dentro del if en la línea de tiempo.",
    " · Classic, Graph, Tracks":
        " · Clásica, Grafo, Pistas",
    "Guide":
        "Guía",
    "Search the guide...  (/)":
        "Buscar en la guía...  (/)",
    "Light or dark":
        "Claro u oscuro",
    "Previous":
        "Anterior",
    "Next":
        "Siguiente",
    "Pause":
        "Pausa",
    "View:":
        "Vista:",
    "Nothing found.":
        "No se encontró nada.",
    "on":
        "en",
    "Picture still in English":
        "Imagen aún en inglés",
    "No.":
        "N.º",
    "JK Studio guide - mod tools for Jedi Knight: Jedi Academy and Jedi Outcast. The pictures show the program itself; they are made by its self-test. Not affiliated with Lucasfilm, Disney or Raven Software.":
        "Guía de JK Studio: herramientas de mods para Jedi Knight: Jedi Academy y Jedi Outcast. Las imágenes muestran el propio programa; las toma su autotest. Sin relación con Lucasfilm, Disney ni Raven Software.",
    # uebersetzung1: Texte aus karte9d, mission1 und menue2 (nach Integration 3)
    "**Grab the figure in the middle of its walk** (timeline between start and arrival) and drag: a new point is created at that moment - for figures a new `ref_tag` as navgoal (it does not change the nav network, so the figure walks on to its old goal as in the game), for ships a split `move`. If a figure finds no path from a point, a red dashed line with the note \"No path from here to ...\" leads to the goal it cannot reach.":
        "**Agarra la figura en mitad de su marcha** (la línea de tiempo entre el inicio y la llegada) y arrastra: se crea un punto nuevo en ese momento; para las figuras, un `ref_tag` nuevo como navgoal (no cambia la red de navegación, así que la figura sigue hacia su antiguo destino como en el juego), para las naves, un `move` dividido. Si una figura no encuentra camino desde un punto, una línea roja discontinua con la nota \"No hay camino desde aquí hasta ...\" lleva al destino que no puede alcanzar.",
    "**After a flight** - in the pause before the next one or after the last - moving or rotating a ship also creates a new key: a new `move` from the end of the previous flight to this moment. The start and the other keys keep their place, angles and times. Likewise, moving a figure after its last walk adds a new navgoal (a `ref_tag`) it walks to when it arrives; its spawner - the start - stays. While the timeline plays, no gizmo is shown. One <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes back a whole gizmo change - a new point together with its new navgoal - and <kbd>Ctrl</kbd>+<kbd>Y</kbd> brings it back exactly.":
        "**Tras un vuelo** (en la pausa antes del siguiente o después del último), mover o girar una nave también crea una clave nueva: un `move` nuevo desde el final del vuelo anterior hasta este momento. El inicio y las demás claves conservan su lugar, sus ángulos y sus tiempos. Del mismo modo, mover una figura después de su última marcha añade un navgoal nuevo (un `ref_tag`) al que camina cuando llega; su spawner (el inicio) se queda. Mientras se reproduce la línea de tiempo no se muestra ningún gizmo. Un solo <kbd>Ctrl</kbd>+<kbd>Z</kbd> deshace un cambio de gizmo completo (un punto nuevo junto con su navgoal nuevo) y <kbd>Ctrl</kbd>+<kbd>Y</kbd> lo restaura exactamente.",
    "The texture browser shows the textures and shaders of the game, folder by folder. In the layout {{KeLayoutRegular}} it sits below the camera; in the other layouts **{{KeLView}} > {{KeLTextureBrowser}}** (<kbd>T</kbd>) opens it. A click on a texture makes it the current one and puts it on the selected brushes or faces; the middle mouse button only picks it. Right mouse drag scrolls, a right click opens the texture's menu. In the camera, the middle mouse button on a face picks up its texture; together with <kbd>Ctrl</kbd> or <kbd>Shift</kbd> it paints the current texture onto faces, as in NetRadiant-Custom. **{{KeSeMenue}}** in that menu opens the shader file in an editor with syntax colours and the margin of the script editor (line numbers, change marks); **{{KeSave}}** never writes into the game folder - a game shader file goes under the same name into the import folder, which comes first in the search path - and the shaders, the texture browser, the camera and the real-time light reload at once. Any change to a .shader file in the import folder (also from another text editor) reloads them too.":
        "El navegador de texturas muestra las texturas y shaders del juego, carpeta a carpeta. En la disposición {{KeLayoutRegular}} está debajo de la cámara; en las demás disposiciones lo abre **{{KeLView}} > {{KeLTextureBrowser}}** (<kbd>T</kbd>). Un clic en una textura la hace actual y la pone en los brushes o caras seleccionados; el botón central solo la elige. Arrastrar con el botón derecho desplaza, un clic derecho abre el menú de la textura. En la cámara, el botón central sobre una cara toma su textura; junto con <kbd>Ctrl</kbd> o <kbd>Mayús</kbd> pinta la textura actual en las caras, como en NetRadiant-Custom. **{{KeSeMenue}}** en ese menú abre el archivo de shaders en un editor con colores de sintaxis y el margen del editor de scripts (números de línea, marcas de cambios); **{{KeSave}}** nunca escribe en la carpeta del juego (un archivo de shaders del juego va con el mismo nombre a la carpeta de importación, que va primero en la ruta de búsqueda), y los shaders, el navegador de texturas, la cámara y la luz en tiempo real se recargan enseguida. Cualquier cambio en un archivo .shader de la carpeta de importación (también desde otro editor de texto) también los recarga.",
    "The shader editor (live) with the fog shader of a test map: syntax colours, line numbers and change marks as in the script editor; it saves to the import folder.":
        "El editor de shaders (en vivo) con el shader de niebla de un mapa de prueba: colores de sintaxis, números de línea y marcas de cambios como en el editor de scripts; guarda en la carpeta de importación.",
    "The **{{KeLCurve}}** menu builds patches in the bounds of the selection: plane (**{{KeLSimplePatchMesh}}**, <kbd>Shift</kbd>+<kbd>P</kbd>), bevel, end cap, cylinders, cone, sphere and the exact versions with a size dialog. **{{KeLInsertDelete}}** adds or removes rows and columns, **{{KeLMatrix}}** inverts (<kbd>Ctrl</kbd>+<kbd>I</kbd>), transposes (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>), redisperses and smooths. **{{KeLCapSelection}}** (<kbd>Shift</kbd>+<kbd>C</kbd>) closes the ends, **{{KeLThicken}}** (<kbd>Ctrl</kbd>+<kbd>T</kbd>) gives a patch a thickness, **{{KeLDeform}}** adds noise. The texture of a patch: **{{KeLNaturalize}}** (<kbd>Ctrl</kbd>+<kbd>N</kbd>), invert X and Y, reset. **{{KePbMenue}}** joins two selected patches with a new patch whose first and last row are exactly the control points of one edge of each (with selected control points the edge that holds them, otherwise the nearest pair of edges with the same number of points) - no seam, one undo step.":
        "El menú **{{KeLCurve}}** construye patches en los límites de la selección: plano (**{{KeLSimplePatchMesh}}**, <kbd>Mayús</kbd>+<kbd>P</kbd>), bisel, tapa final, cilindros, cono, esfera y las versiones exactas con un diálogo de tamaño. **{{KeLInsertDelete}}** añade o quita filas y columnas, **{{KeLMatrix}}** invierte (<kbd>Ctrl</kbd>+<kbd>I</kbd>), transpone (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>M</kbd>), redistribuye y suaviza. **{{KeLCapSelection}}** (<kbd>Mayús</kbd>+<kbd>C</kbd>) cierra los extremos, **{{KeLThicken}}** (<kbd>Ctrl</kbd>+<kbd>T</kbd>) da grosor a un patch, **{{KeLDeform}}** añade ruido. La textura de un patch: **{{KeLNaturalize}}** (<kbd>Ctrl</kbd>+<kbd>N</kbd>), invertir X e Y, restablecer. **{{KePbMenue}}** une dos patches seleccionados con un patch nuevo cuya primera y última fila son exactamente los puntos de control de un borde de cada uno (con puntos de control seleccionados, el borde que los contiene; si no, el par de bordes más cercano con el mismo número de puntos): sin costura, un solo paso de deshacer.",
    "Meshes of a model that the .bsp does not have (the mapper hid them, for example with a nodraw shader) get a `_remap` to `textures/system/nodraw`: the editor and a new build show exactly what the .bsp holds (q3map2 does not emit nodraw surfaces). Only a mesh that shares its shader with a found one cannot be hidden this way.":
        "Las mallas de un modelo que el .bsp no tiene (el mapper las ocultó, por ejemplo con un shader nodraw) reciben un `_remap` a `textures/system/nodraw`: el editor y una compilación nueva muestran exactamente lo que contiene el .bsp (q3map2 no emite superficies nodraw). Solo una malla que comparte su shader con una encontrada no se puede ocultar así.",
    "While **File > Open .bsp** still recovers the misc_model entities in the background, fixes and imports wait (the buttons are disabled, the details show the hint): a geometry change in that time would drop the recovery, or bring it back after an Undo as a new step that clears Redo.":
        "Mientras **File > Open .bsp** todavía recupera las entidades misc_model en segundo plano, las correcciones y las importaciones esperan (los botones están desactivados, los detalles muestran el aviso): un cambio de geometría en ese tiempo descartaría la recuperación, o la traería de vuelta tras un deshacer como un paso nuevo que vacía el rehacer.",
    "Hint suggestions are only shown, never applied: where a hint brush helps depends on the gameplay. A detail suggestion for decoration is an estimate too: after a build without a leak it leaves out every brush with a face at the void of that .bsp (in the test room a small plug in a hole of the outer wall - as detail the map leaks), but a stale .bsp or a gap the sample points miss can still fool it - never make brushes detail that seal the map.":
        "Las sugerencias de hint solo se muestran, nunca se aplican: dónde ayuda un brush hint depende de la jugabilidad. Una sugerencia de detalle para la decoración también es una estimación: tras una compilación sin fugas deja fuera todo brush con una cara al vacío de ese .bsp (en la sala de prueba, un pequeño tapón en un agujero de la pared exterior; como detail, el mapa tiene una fuga), pero un .bsp desactualizado o un hueco que no alcanzan los puntos de muestra aún pueden engañarla: nunca hagas de detalle los brushes que sellan el mapa.",
    "**Models and patches:** misc_model entities cast shadows like in q3map2 (in the world always, otherwise only with `_castShadows`; `_castShadows 0` switches it off) and their surfaces with `q3map_surfacelight` shine per triangle; patches shine over the same tessellation as in -light. Brush faces shine over the triangles of `-meta`; `q3map_sunExt` with deviance and samples and lights with `_samples` get their jittered copies with q3map2's own random sequence.":
        "**Modelos y patches:** las entidades misc_model proyectan sombras como en q3map2 (en el mundo siempre; si no, solo con `_castShadows`; `_castShadows 0` lo desactiva) y sus superficies con `q3map_surfacelight` brillan por triángulo; los patches brillan con la misma teselación que en -light. Las caras de los brushes brillan con los triángulos de `-meta`; `q3map_sunExt` con desviación y muestras y las luces con `_samples` reciben sus copias desplazadas con la propia secuencia aleatoria de q3map2.",
    "Fog, effects and sky portal":
        "Niebla, efectos y portal de cielo",
    "**Fog:** brushes with a `fogParms` shader and the global fog of worldspawn (`_fog`) are drawn as the game draws them (RB_FogPass with R_FogFactor): colour and distance to opaque from the shader, the fog surface from the visible side of the brush (or `q3map_fogDir`), deeper below it is denser.":
        "**Niebla:** los brushes con un shader `fogParms` y la niebla global de worldspawn (`_fog`) se dibujan como los dibuja el juego (RB_FogPass con R_FogFactor): el color y la distancia hasta la opacidad, del shader; la superficie de la niebla, de la cara visible del brush (o `q3map_fogDir`); cuanto más hondo por debajo, más densa.",
    "**Effects:** fx_runner entities play their effect with the effect system of the cutscene preview - repeat interval `delay`/`random`, direction to the `target` (or `angle`, otherwise up); START_OFF and ONESHOT wait for a use. **{{KeLBuild}} > {{KeEchtEffekte}}** switches them off. The editor has no .bsp, so particles do not bounce off walls.":
        "**Efectos:** las entidades fx_runner reproducen su efecto con el sistema de efectos de la vista previa de cinemáticas: intervalo de repetición `delay`/`random`, dirección hacia el `target` (o `angle`; si no, hacia arriba); START_OFF y ONESHOT esperan un use. **{{KeLBuild}} > {{KeEchtEffekte}}** los desactiva. El editor no tiene .bsp, así que las partículas no rebotan en las paredes.",
    "**Sky portal:** with a misc_skyportal the sky shows the scene from the portal's place, with the same view and field of view, as in the game (CG_DrawSkyBoxPortal).":
        "**Portal de cielo:** con un misc_skyportal el cielo muestra la escena desde el lugar del portal, con la misma vista y el mismo campo de visión, como en el juego (CG_DrawSkyBoxPortal).",
    "A fog brush over the floor of a room (fogParms 0.1 0.9 0.1, 256): the floor gets the fog colour of the game.":
        "Un brush de niebla sobre el suelo de una sala (fogParms 0.1 0.9 0.1, 256): el suelo toma el color de niebla del juego.",
    "**{{MeMeldungen}}** shows what the game would print while loading the file, word for word and with the line (for example `Unknown item keyword 'foo' Line #21`). A click jumps to the line; the live diagnosis marks it in the text ([[menue_editor_m2#diagnose]]).":
        "**{{MeMeldungen}}** muestra lo que el juego imprimiría al cargar el archivo, palabra por palabra y con la línea (por ejemplo `Unknown item keyword 'foo' Line #21`). Un clic salta a la línea; el diagnóstico en vivo la marca en el texto ([[menue_editor_m2#diagnose]]).",
    "A new button from the widgets: selected on the canvas, its itemDef in the Text tab on the right.":
        "Un botón nuevo de los elementos: seleccionado en el lienzo, su itemDef en la pestaña Texto de la derecha.",
    "Menu editor: preview, checks and packaging":
        "Editor de menús: vista previa, comprobaciones y paquete",
    "The right column with tabs, the live diagnosis while typing, videos, 3D models and sabers like in the game, the widget library, texts in several languages, sounds, the checks for the game, a test in the game and fonts from TrueType.":
        "La columna derecha con pestañas, el diagnóstico en vivo mientras escribes, vídeos, modelos 3D y sables como en el juego, la biblioteca de elementos, textos en varios idiomas, sonidos, las comprobaciones para el juego, una prueba en el juego y fuentes a partir de TrueType.",
    "Layout: canvas at full height":
        "Distribución: lienzo a toda altura",
    "Widgets as tiles on the left, the canvas in the middle, the right column with the tab Hierarchy.":
        "Los elementos como mosaicos a la izquierda, el lienzo en el centro, la columna derecha con la pestaña Jerarquía.",
    "The same column with the tab Text: the code editor, the problems below.":
        "La misma columna con la pestaña Texto: el editor de código y, debajo, los problemas.",
    "The right column holds **{{MeReiterHierarchie}}** (menus, groups and items; the eye hides an item in the preview only, the lock keeps it from moving, dragging changes the drawing order), **{{MeReiterText}}** (the code editor with the problems below) and **{{MeReiterEigenschaften}}** (properties, cvars, engine messages and the script log as sections). Drag the divider to change the width; the tab and the width are remembered. A click on the canvas moves the text cursor to the itemDef even while another tab is open - switch to **{{MeReiterText}}** and the line is there. The old layout with the text at the bottom is in the preferences (**{{MeEinstTextUnten}}**), as is **{{MeEinstUmbruch}}** (otherwise long lines scroll sideways with Shift + mouse wheel).":
        "La columna derecha contiene **{{MeReiterHierarchie}}** (menús, grupos y elementos; el ojo oculta un elemento solo en la vista previa, el candado impide moverlo, arrastrar cambia el orden de dibujo), **{{MeReiterText}}** (el editor de código con los problemas debajo) y **{{MeReiterEigenschaften}}** (propiedades, cvars, mensajes del motor y el registro de scripts como secciones). Arrastra el divisor para cambiar el ancho; la pestaña y el ancho se recuerdan. Un clic en el lienzo lleva el cursor de texto al itemDef aunque haya otra pestaña abierta: cambia a **{{MeReiterText}}** y la línea está ahí. El diseño antiguo con el texto abajo está en las preferencias (**{{MeEinstTextUnten}}**), igual que **{{MeEinstUmbruch}}** (si no, las líneas largas se desplazan en horizontal con Mayús + rueda del ratón).",
    "Live diagnosis":
        "Diagnóstico en vivo",
    "A broken menu: the brace that is never closed and where it is missing, the misspelled keyword with the game's message, a show on an item that does not exist; the item is marked on the canvas.":
        "Un menú roto: la llave que nunca se cierra y dónde falta, la palabra clave mal escrita con el mensaje del juego, un show sobre un elemento que no existe; el elemento está marcado en el lienzo.",
    "While you type, the menu is checked in the background a moment later. A wavy line marks the exact place - red for errors, yellow for warnings, blue for hints - and the margin shows a symbol; hovering shows the message: the game's own wording where there is one (`Unknown item keyword 'backcolr'`) plus a short explanation. The problems list below the text jumps to the place and selects the item on the canvas; items with errors and warnings are outlined there.":
        "Mientras escribes, el menú se comprueba en segundo plano un momento después. Una línea ondulada marca el lugar exacto (roja para errores, amarilla para advertencias, azul para sugerencias) y el margen muestra un símbolo; al pasar el ratón se ve el mensaje: el texto del propio juego donde lo hay (`Unknown item keyword 'backcolr'`) más una breve explicación. La lista de problemas debajo del texto salta al lugar y selecciona el elemento en el lienzo; allí los elementos con errores y advertencias aparecen contorneados.",
    "Braces `{ }` and `( )` and quotation marks: missing, extra or mismatched - both the opening and the closing place; the indentation tells where a brace is missing.":
        "Llaves `{ }`, paréntesis `( )` y comillas: que faltan, que sobran o que no casan, tanto el lugar de apertura como el de cierre; la sangría indica dónde falta una llave.",
    "Unknown keywords per version, numbers where single player silently reads 0, duplicate menuDef names, targets of open/close/show/hide/setfocus that do not exist, cvars that look like a typo of a known cvar.":
        "Palabras clave desconocidas según la versión, números donde el modo de un jugador lee 0 en silencio, nombres de menuDef duplicados, destinos de open/close/show/hide/setfocus que no existen, cvars que parecen una errata de una cvar conocida.",
    "The checks for the game (the same rules as **{{MePruefen}}**): limits, items that are never visible, missing files.":
        "Las comprobaciones para el juego (las mismas reglas que **{{MePruefen}}**): límites, elementos que nunca son visibles, archivos que faltan.",
    "**{{MeDiagBeheben}}** in the list or <kbd>Ctrl</kbd>+<kbd>.</kbd> at the cursor applies a quick fix where the answer is clear: insert the missing brace, the right keyword, a similar existing file, the known cvar. Undo takes it back.":
        "**{{MeDiagBeheben}}** en la lista o <kbd>Ctrl</kbd>+<kbd>.</kbd> en el cursor aplica una corrección rápida donde la respuesta es clara: insertar la llave que falta, la palabra clave correcta, un archivo existente parecido, la cvar conocida. Deshacer la revierte.",
    "Videos, models and sabers":
        "Vídeos, modelos y sables",
    "A RoQ video as cinematic, decoded like the game.":
        "Un vídeo RoQ como cinematic, decodificado como en el juego.",
    "A character model with skin and animation, drawn on the graphics card.":
        "Un modelo de personaje con skin y animación, dibujado en la tarjeta gráfica.",
    "Library, detail area and menu sets":
        "Biblioteca, área de detalle y conjuntos de menús",
    "The detail area in three levels from the library, ready to operate.":
        "El área de detalle en tres niveles desde la biblioteca, lista para usar.",
    "**{{MeBibliothek}}** holds ready parts (button with background and click sound, slider with cvar, tab bar, detail area in three levels, list); drag one onto the canvas or double-click it. **{{MeVorlageSpeichern}}** turns the selection into your own template. **{{MeMenuesatzOeffnen}}** opens every file of `menus.txt`; `open` in a script then switches to the file of the menu.":
        "**{{MeBibliothek}}** contiene piezas listas (botón con fondo y sonido de clic, deslizador con cvar, barra de pestañas, área de detalle en tres niveles, lista); arrastra una al lienzo o haz doble clic. **{{MeVorlageSpeichern}}** convierte la selección en una plantilla propia. **{{MeMenuesatzOeffnen}}** abre todos los archivos de `menus.txt`; `open` en un script cambia entonces al archivo del menú.",
    "Texts in several languages and sounds":
        "Textos en varios idiomas y sonidos",
    "An own text @MYMENU_START, shown in German in the preview.":
        "Un texto propio @MYMENU_START, mostrado en alemán en la vista previa.",
    "**{{MeTexte}}** lists your own @-texts in every language of the game and the @-keys the menu uses (missing ones can be created with one click). They are written into the package as `strings/<language>/<PACKAGE>.str` (Jedi Academy, Movie Duels) or `strip/<package>.sp` (Jedi Outcast). The language box next to it switches the preview. In **{{MeBedienen}}** the menu plays its sounds like the game - `soundLoop`, `play`, `playlooped`, the focus sound of the item or of `assetGlobalDef` (**{{MeTonAn}}** in the toolbar switches them off).":
        "**{{MeTexte}}** lista tus propios textos @ en cada idioma del juego y las claves @ que usa el menú (las que faltan se crean con un clic). Se escriben en el paquete como `strings/<idioma>/<PAQUETE>.str` (Jedi Academy, Movie Duels) o `strip/<paquete>.sp` (Jedi Outcast). El selector de idioma de al lado cambia la vista previa. En **{{MeBedienen}}** el menú reproduce sus sonidos como el juego (`soundLoop`, `play`, `playlooped`, el sonido de foco del elemento o de `assetGlobalDef`); **{{MeTonAn}}** en la barra de herramientas los desactiva.",
    "Check, export and test in the game":
        "Comprobar, exportar y probar en el juego",
    "**{{MePruefen}}** checks all open menus for Jedi Academy, Jedi Outcast and Movie Duels: the limits of each version (items per menu, menus, script length, multi values, colour ranges, list columns - the game drops what is beyond), items that are never visible, missing images, shaders, models, videos, fonts and sounds, and keywords or item types another version does not know. **{{MeExportPk3}}** writes all open menus or, with **{{MeExportAenderungen}}**, only the changed ones - each time with your package (images, videos, texts, fonts) and with a warning if the game would lose something.":
        "**{{MePruefen}}** comprueba todos los menús abiertos para Jedi Academy, Jedi Outcast y Movie Duels: los límites de cada versión (elementos por menú, menús, longitud de scripts, valores múltiples, rangos de color, columnas de listas; el juego descarta lo que pasa de ahí), elementos que nunca son visibles, imágenes, shaders, modelos, vídeos, fuentes y sonidos que faltan, y palabras clave o tipos de elemento que otra versión no conoce. **{{MeExportPk3}}** escribe todos los menús abiertos o, con **{{MeExportAenderungen}}**, solo los cambiados, cada vez con tu paquete (imágenes, vídeos, textos, fuentes) y con una advertencia si el juego fuera a perder algo.",
    "**{{MeImSpielTesten}}** puts the changed menus into `menuetest` in the settings folder - never into the game folder - and starts the game program found next to the game folders with `fs_homepath` pointing there, so your files replace the game's. A program that cannot be given its own folder asks before anything is written into a mod folder.":
        "**{{MeImSpielTesten}}** pone los menús cambiados en `menuetest` dentro de la carpeta de ajustes (nunca en la carpeta del juego) e inicia el programa del juego que encuentra junto a las carpetas del juego con `fs_homepath` apuntando allí, de modo que tus archivos sustituyen a los del juego. Un programa al que no se le puede dar su propia carpeta pregunta antes de escribir nada en una carpeta de mod.",
    "Fonts from TrueType":
        "Fuentes a partir de TrueType",
    "**{{MeSchriftBauen}}** turns a .ttf or .otf into a game font: `fonts/<name>.fontdat` (the exact format the game reads) and `fonts/<name>.tga` with all characters of the menu texts (Windows-1252). Both go into the package; the preview uses them at once.":
        "**{{MeSchriftBauen}}** convierte un .ttf o .otf en una fuente del juego: `fonts/<nombre>.fontdat` (el formato exacto que lee el juego) y `fonts/<nombre>.tga` con todos los caracteres de los textos de menú (Windows-1252). Ambos van al paquete; la vista previa los usa enseguida.",
    # uebersetzung2: Zeitleiste neu, Menue-Editor M3-M5b, Fenster anordnen, VIS (nach Integration 4)
    "**See it in the timeline.** Open the figure's row in the timeline (the arrow at its name): its row **{{ZlBefehle}}** shows its waits and signals at their time.":
        "**Míralo en la línea de tiempo.** Despliega la fila de la figura en la línea de tiempo (la flecha junto a su nombre): su fila **{{ZlBefehle}}** muestra sus esperas y señales en su momento.",
    "**Shake.** Choose `SHAKE`: strength 0 to 16 and the duration in milliseconds. In the **{{ZlDetail}}** view the timeline shows zoom, fade and shake in their own rows.":
        "**Temblor.** Elige `SHAKE`: fuerza de 0 a 16 y la duración en milisegundos. En la vista **{{ZlDetail}}**, la línea de tiempo muestra el zoom, el fundido y el temblor en filas propias.",
    "**Step exactly.** With the mouse over the timeline, <kbd>Left</kbd> / <kbd>Right</kbd> move the playhead by 50 ms, with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds.":
        "**Avanza con precisión.** Con el ratón sobre la línea de tiempo, <kbd>←</kbd> / <kbd>→</kbd> mueven el cabezal 50 ms, con <kbd>Mayús</kbd> un segundo, con <kbd>Ctrl</kbd> cinco segundos.",
    "**Check it.** Play the timeline. Open the figure's row: its row **{{ZlBefehle}}** shows the look commands at their time.":
        "**Compruébalo.** Reproduce la línea de tiempo. Despliega la fila de la figura: su fila **{{ZlBefehle}}** muestra los comandos de mirada en su momento.",
    "**Watch it.** Turn on **{{MapThroughCam}}** and play the timeline: the view keeps turning towards the figure. In the **{{ZlDetail}}** view of the timeline, the **{{TlCamFollow}}** row shows how long it lasts.":
        "**Míralo.** Activa **{{MapThroughCam}}** y reproduce la línea de tiempo: la vista no deja de girar hacia la figura. En la vista **{{ZlDetail}}** de la línea de tiempo, la fila **{{TlCamFollow}}** muestra cuánto dura.",
    "**Waits as blocks.** The **{{ZlSkript}}** row shows every `wait` as a hatched block with what it waits for - drag its right edge to change it directly.":
        "**Esperas como bloques.** La fila **{{ZlSkript}}** muestra cada `wait` como un bloque rayado con lo que espera: arrastra su borde derecho para cambiarlo directamente.",
    "**Only active.** **{{ZlNurAktive}}** hides the actors that do nothing in the visible section.":
        "**Solo activos.** **{{ZlNurAktive}}** oculta los actores que no hacen nada en el tramo visible.",
    "The timeline plays the scene and shows when everything happens - camera shots with names, speech with subtitles, music, the script's waits and every actor. Click a block and the side bar explains it; drag it and the script changes.":
        "La línea de tiempo reproduce la escena y muestra cuándo pasa cada cosa: planos de cámara con nombre, voces con subtítulos, música, las esperas del script y cada actor. Pulsa un bloque y la barra lateral lo explica; arrástralo y el script cambia.",
    "The timeline in the Overview: camera position and rotation with the names of the marks, speech with speaker and subtitle, music, the script flow with its waits, one row per actor.":
        "La línea de tiempo en el Resumen: posición y rotación de la cámara con los nombres de las marcas, voces con hablante y subtítulo, música, el flujo del script con sus esperas, una fila por actor.",
    "Playing and moving around":
        "Reproducir y moverse",
    "The play button (▶) under the map starts and stops; with the mouse over the timeline <kbd>Space</kbd> does the same. Click or drag on the ruler to set the time - the 3D view shows the scene at that moment. <kbd>Left</kbd> / <kbd>Right</kbd> move by one game frame (50 ms, the grid of the game logic), with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds, <kbd>Home</kbd> jumps to the start. The legend under the rows lists these keys. The speed field slows the scene down (sound only plays at 1x), **{{TlLoop}}** repeats it. **{{ZlUt}}** shows the subtitles in the preview picture the way the game does (the game setting g_subtitles): with the script camera on, in the middle of the lower black bar, two lines at a time, each pair as long as its letters take of the sound; Movie Duels draws them smaller (cg_textprintscale, 0.66 by default). As in the game, Jedi Academy and Movie Duels show none by default, Jedi Outcast shows them in cinematics, and a figure with `SET_USE_SUBTITLES` always shows its lines; `SET_CAPTIONTEXTCOLOR` changes the colour.":
        "El botón de reproducir (▶) bajo el mapa inicia y detiene; con el ratón sobre la línea de tiempo, <kbd>Espacio</kbd> hace lo mismo. Pulsa o arrastra en la regla para fijar el tiempo: la vista 3D muestra la escena en ese momento. <kbd>←</kbd> / <kbd>→</kbd> avanzan un fotograma del juego (50 ms, el paso de la lógica del juego), con <kbd>Mayús</kbd> un segundo, con <kbd>Ctrl</kbd> cinco segundos, <kbd>Inicio</kbd> salta al principio. La leyenda bajo las filas enumera estas teclas. El campo de velocidad ralentiza la escena (el sonido solo suena a 1x), **{{TlLoop}}** la repite. **{{ZlUt}}** muestra los subtítulos en la imagen de la vista previa como lo hace el juego (el ajuste del juego g_subtitles): con la cámara del script activa, en el centro de la franja negra inferior, dos líneas cada vez, cada par durante lo que sus letras ocupan del sonido; Movie Duels los dibuja más pequeños (cg_textprintscale, 0,66 por defecto). Como en el juego, Jedi Academy y Movie Duels no muestran ninguno por defecto, Jedi Outcast los muestra en las secuencias, y una figura con `SET_USE_SUBTITLES` siempre muestra sus frases; `SET_CAPTIONTEXTCOLOR` cambia el color.",
    "A scene starts with a section of about 30 seconds. **{{TlFit}}** shows the whole scene, − and + zoom out and in, <kbd>Ctrl</kbd>+wheel zooms around the mouse pointer, <kbd>Shift</kbd>+wheel scrolls. Only while zoomed in, a slim navigation bar above the ruler shows which part of the scene you see - click or drag it to move there.":
        "Una escena empieza con un tramo de unos 30 segundos. **{{TlFit}}** muestra toda la escena, − y + alejan y acercan, <kbd>Ctrl</kbd>+rueda amplía alrededor del puntero del ratón, <kbd>Mayús</kbd>+rueda desplaza. Solo con la vista ampliada, una barra de navegación fina sobre la regla muestra qué parte de la escena ves: pulsa o arrástrala para ir allí.",
    "Overview and Detail":
        "Resumen y Detalle",
    "**{{ZlUebersicht}}** shows the scene at a glance: **{{ZlOrt}}** has one block per shot, from one camera move to the next, named after its mark (or as readable coordinates); **{{ZlDrehung}}** shows the pans, a pan without duration is marked as a cut. **{{ZlSprache}}** shows every line as long as its sound file, with speaker, subtitle from the game's string files (Jedi Academy and Movie Duels: strings/english/<map>.str, Jedi Outcast: strip/<map>.sp, with the extra packages some maps read) and the waveform. **{{ZlMusik}}** shows which music plays from when. **{{ZlSkript}}** shows the waits of the script as hatched blocks with what they wait for, and its commands as labelled marks; a map change is a red mark. Each actor has one row: meaningful animations with their names on top, its walks numbered below, a dashed line where it stands still, hatched where it is not visible. The arrow at the name opens its **{{ZlAnimation}}** and **{{ZlBefehle}}**.":
        "**{{ZlUebersicht}}** muestra la escena de un vistazo: **{{ZlOrt}}** tiene un bloque por plano, de un movimiento de cámara al siguiente, con el nombre de su marca (o con coordenadas legibles); **{{ZlDrehung}}** muestra los giros, y un giro sin duración se marca como corte. **{{ZlSprache}}** muestra cada frase tan larga como su archivo de sonido, con hablante, subtítulo de los archivos de textos del juego (Jedi Academy y Movie Duels: strings/english/<mapa>.str, Jedi Outcast: strip/<mapa>.sp, con los paquetes extra que leen algunos mapas) y la forma de onda. **{{ZlMusik}}** muestra qué música suena y desde cuándo. **{{ZlSkript}}** muestra las esperas del script como bloques rayados con lo que esperan, y sus comandos como marcas con rótulo; un cambio de mapa es una marca roja. Cada actor tiene una fila: arriba las animaciones significativas con su nombre, abajo sus recorridos numerados, una línea discontinua donde está quieto, rayado donde no se ve. La flecha junto al nombre despliega su **{{ZlAnimation}}** y sus **{{ZlBefehle}}**.",
    "Detail: camera move, pan, zoom and fade separately, field of view and fade as lines. A move that starts before the previous one has finished gets its own lane and the note \"jumps back\".":
        "Detalle: movimiento, giro, zoom y fundido de cámara por separado, campo de visión y fundido como líneas. Un movimiento que empieza antes de que termine el anterior recibe su propio carril y el aviso \"salta atrás\".",
    "**{{ZlDetail}}** (or <kbd>Tab</kbd> over the timeline) splits the camera into move, pan, zoom and fade and draws the field of view and the fade as lines. They are straight between two commands because the game interpolates them linearly - there are no ease curves. If a camera move starts before the previous one of the same kind has finished, it gets a lane of its own and the red note **{{ZlSpringt}}**: the game only takes over position and angle at the end of a move (cg_camera.cpp), so the new move starts from where the previous one began - the camera jumps back.":
        "**{{ZlDetail}}** (o <kbd>Tab</kbd> sobre la línea de tiempo) divide la cámara en movimiento, giro, zoom y fundido, y dibuja el campo de visión y el fundido como líneas. Son rectas entre dos comandos porque el juego los interpola de forma lineal: no hay curvas de suavizado. Si un movimiento de cámara empieza antes de que termine el anterior del mismo tipo, recibe un carril propio y el aviso rojo **{{ZlSpringt}}**: el juego solo asume la posición y el ángulo al final de un movimiento (cg_camera.cpp), así que el nuevo movimiento parte de donde empezó el anterior: la cámara salta atrás.",
    "What happens here?":
        "¿Qué pasa aquí?",
    "A clicked block: the side bar explains it in plain words, with time, command and script line.":
        "Un bloque pulsado: la barra lateral lo explica con palabras sencillas, con tiempo, comando y línea del script.",
    "Click any block or mark: the time jumps there, its line is selected in the script, and below the script column the box **{{ZlWas}}** says in plain words what it does - where the camera moves and how long, who speaks which line, what a wait waits for and who sends that signal, which animation plays for how long. **{{ZlZeileOeffnen}}** opens the command in the editor. A command that comes from another script (run, use, spawnscript) names that script and its line there - **{{ZlZeileOeffnen}}** opens that script and selects the line; one the simulation inserted has no line.":
        "Pulsa cualquier bloque o marca: el tiempo salta allí, su línea se selecciona en el script y, bajo la columna del script, el cuadro **{{ZlWas}}** dice con palabras sencillas lo que hace: adónde se mueve la cámara y cuánto tarda, quién dice qué frase, qué espera un wait y quién envía esa señal, qué animación se reproduce y durante cuánto tiempo. **{{ZlZeileOeffnen}}** abre el comando en el editor. Un comando que viene de otro script (run, use, spawnscript) nombra ese script y su línea allí: **{{ZlZeileOeffnen}}** abre ese script y selecciona la línea; uno que ha insertado la simulación no tiene línea.",
    "Filters and search":
        "Filtros y búsqueda",
    "**{{ZlNurAktive}}** hides actors that do nothing in the visible section. **{{ZlProbleme}}** shows only places with notes: camera moves that jump back, commands the preview does not simulate yet, the map change. The search field filters by name, mark, file, subtitle or command (for example `luke`, `cam1b`, `SET_ANIM`).":
        "**{{ZlNurAktive}}** oculta los actores que no hacen nada en el tramo visible. **{{ZlProbleme}}** muestra solo los puntos con avisos: movimientos de cámara que saltan atrás, comandos que la vista previa aún no simula, el cambio de mapa. El campo de búsqueda filtra por nombre, marca, archivo, subtítulo o comando (por ejemplo `luke`, `cam1b`, `SET_ANIM`).",
    "Problems: only the rows with notes remain.":
        "Problemas: solo quedan las filas con avisos.",
    "Drag a camera block or a line left or right: the `wait` before it changes, so it happens earlier or later and everything after it moves along; with <kbd>Alt</kbd> the `wait` after it gets shorter by the same amount and the rest stays. Drag the right edge of a camera block to change its duration, the right edge of a wait to make it longer or shorter. A line keeps its length - it is the length of the sound file. A walk cannot be made longer or shorter: its duration comes from the path finding and the speed of the figure. Everything snaps to the game's 50 ms frames, and while you drag a note says what changes in the script.":
        "Arrastra un bloque de cámara o una frase a la izquierda o a la derecha: cambia el `wait` anterior, así que pasa antes o después y todo lo que viene detrás se mueve con él; con <kbd>Alt</kbd>, el `wait` siguiente se acorta lo mismo y el resto se queda donde está. Arrastra el borde derecho de un bloque de cámara para cambiar su duración, y el borde derecho de una espera para alargarla o acortarla. Una frase conserva su duración: es la del archivo de sonido. Un recorrido no se puede alargar ni acortar: su duración sale de la búsqueda de caminos y de la velocidad de la figura. Todo se ajusta a los fotogramas de 50 ms del juego y, mientras arrastras, un aviso dice qué cambia en el script.",
    "A dragged wait: changed blocks are outlined with a dashed line until <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes the change back.":
        "Una espera arrastrada: los bloques cambiados llevan un contorno discontinuo hasta que <kbd>Ctrl</kbd>+<kbd>Z</kbd> deshace el cambio.",
    "Every drag writes the value into the script (comments and formatting stay) and is exactly one step for <kbd>Ctrl</kbd>+<kbd>Z</kbd> - afterwards the script is byte for byte what it was.":
        "Cada arrastre escribe el valor en el script (los comentarios y el formato se mantienen) y es exactamente un paso de <kbd>Ctrl</kbd>+<kbd>Z</kbd>: después, el script es byte a byte lo que era.",
    "Recording a walk":
        "Grabar un recorrido",
    "Recording: the path so far, the walk targets as numbered circles, the box of the figure and the hint at the bottom.":
        "Grabación: el camino hasta ahora, los destinos como círculos numerados, la caja de la figura y la indicación de abajo.",
    "Written is what the game uses to let an NPC walk: one `ref_tag` per point and one `set ( SET_NAVGOAL, ... )`, which waits in the game until the figure has arrived, with `SET_RUNNING`/`SET_WALKING` before it where the gait changes. Points are kept only where they are needed: where the path leaves a straight line by more than the tolerance (in units, adjustable while recording), where a straight line would run into a wall, and where the gait changes. The commands go into the figure's own commands at the time of the playhead (a new `affect` block if it has none) - one step for <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Afterwards the preview walks it like the game: between the points the NPC finds its own way at its own speed - the game has no timing per stretch, so the recording sets only the points and the gait.":
        "Se escribe lo que usa el juego para que un NPC camine: un `ref_tag` por punto y un `set ( SET_NAVGOAL, ... )`, que en el juego espera hasta que la figura ha llegado, con `SET_RUNNING`/`SET_WALKING` delante donde cambia la forma de andar. Solo se mantienen los puntos necesarios: donde el camino se aparta de una línea recta más que la tolerancia (en unidades, ajustable durante la grabación), donde una línea recta chocaría con una pared y donde cambia la forma de andar. Los comandos van a los comandos propios de la figura en el tiempo del cabezal (un bloque `affect` nuevo si no tiene ninguno): un paso de <kbd>Ctrl</kbd>+<kbd>Z</kbd>. Después, la vista previa lo recorre como el juego: entre los puntos, el NPC busca su propio camino a su propia velocidad; el juego no tiene tiempos por tramo, así que la grabación solo fija los puntos y la forma de andar.",
    "Subtitles in the preview picture, in cinematics: two lines in the middle of the lower bar.":
        "Subtítulos en la imagen de la vista previa, en secuencias: dos líneas en el centro de la franja inferior.",
    "Times come from a re-creation of the game's ICARUS engine, including its quirks: a `wait` ends one game frame (50 ms) later than written, and a `dowait` waits for the task it names. A command the preview does not simulate yet (for example `camera ( TRACK )` or `print`) is shown as a red mark instead of being guessed. The rules of OpenJK or Movie Duels apply as set in [[einstellungen#prefs]].":
        "Los tiempos salen de una recreación del motor ICARUS del juego, con sus peculiaridades: un `wait` termina un fotograma del juego (50 ms) más tarde de lo escrito, y un `dowait` espera a la tarea que nombra. Un comando que la vista previa aún no simula (por ejemplo `camera ( TRACK )` o `print`) se muestra como una marca roja en vez de adivinarlo. Se aplican las reglas de OpenJK o de Movie Duels según lo ajustado en [[einstellungen#prefs]].",
    "In the timeline a ROFF flight is a block in the row of its entity (\"ROFF name\"); drag it to start the flight earlier or later.":
        "En la línea de tiempo, un vuelo ROFF es un bloque en la fila de su entidad (\"ROFF nombre\"); arrástralo para que el vuelo empiece antes o después.",
    "**Full VIS**: before a `-vis` without `-fast` the console names the portals, the clusters and an estimate of the memory the VIS needs - read from the portal file of the BSP step, counted like q3map2's CreatePassages. bhed_q3map2 keeps of each passage only the part that is not empty (md_rev_TarisUpper2: 1.4 instead of 9 GB) and prints after CreatePassages what the passages really take. From the portal count of the optimizer's VIS hint, or when the memory comes close to what the computer has, a warning follows, also in the window: a full VIS can take hours, on open maps days. Its progress counts the expected work of the portals, not their number - the expensive ones come last - so the remaining time in the first build is close to the real one. Ways out: `-vis -fast` for test builds, small structural brushes that do not seal as detail (optimizer: **{{KeOptPrDeko}}**), and **{{KeTkVis}}** for later builds. For a map from File > Open .bsp it also says whether the original was built with `-vis -fast`.":
        "**VIS completo**: antes de un `-vis` sin `-fast`, la consola indica los portales, los clústeres y una estimación de la memoria que necesita el VIS, leídos del archivo de portales del paso BSP y contados como CreatePassages de q3map2. bhed_q3map2 guarda de cada pasaje solo la parte que no está vacía (md_rev_TarisUpper2: 1,4 en vez de 9 GB) e imprime tras CreatePassages lo que ocupan realmente los pasajes. A partir del número de portales del aviso de VIS del optimizador, o cuando la memoria se acerca a la que tiene el ordenador, sigue una advertencia, también en la ventana: un VIS completo puede tardar horas, en mapas abiertos días. Su progreso cuenta el trabajo esperado de los portales, no su número (los costosos van al final), así que el tiempo restante ya se acerca al real en la primera compilación. Salidas: `-vis -fast` para compilaciones de prueba, los brushes estructurales pequeños que no sellan, como detail (optimizador: **{{KeOptPrDeko}}**), y **{{KeTkVis}}** para compilaciones posteriores. Para un mapa de File > Open .bsp también dice si el original se compiló con `-vis -fast`.",
    "**{{KeLBuild}} > {{KeVisExakt}}** (off by default): with several threads a full VIS can come out slightly different from build to build - as with NetRadiant-Custom's q3map2, where a portal uses whatever other portals happen to be finished; every result is valid. Switched on, every `-vis` step gets `-bhedvisexact`, and bhed_q3map2 computes with all threads exactly what one thread computes, byte for byte. You need it for **reproducible builds**: the same .map gives the same .bsp - to compare two builds, for checksums, or when several people build the same map. It waits where a portal needs one that is not finished yet - on closed maps about as fast, on open maps much slower (duel_mos_espa: 5 instead of 1.5 minutes). Off is the faster way.":
        "**{{KeLBuild}} > {{KeVisExakt}}** (desactivado por defecto): con varios hilos, un VIS completo puede salir ligeramente distinto de una compilación a otra; como en el q3map2 de NetRadiant-Custom, un portal usa los otros portales que ya estén terminados en ese momento, y todos los resultados son válidos. Activado, cada paso `-vis` recibe `-bhedvisexact`, y bhed_q3map2 calcula con todos los hilos exactamente lo mismo que un solo hilo, byte a byte. Lo necesitas para **compilaciones reproducibles**: el mismo .map da el mismo .bsp, para comparar dos compilaciones, para sumas de comprobación o cuando varias personas compilan el mismo mapa. Espera donde un portal necesita otro aún sin terminar: en mapas cerrados es casi igual de rápido, en mapas abiertos mucho más lento (duel_mos_espa: 5 minutos en vez de 1,5). Desactivado es el camino más rápido.",
    "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} and {{KeTkLicht}} switched on, below them {{KeVisExakt}}, {{KeZfMenue}} and {{KeFsMelden}} (not in NetRadiant-Custom).":
        "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} y {{KeTkLicht}} activados, debajo {{KeVisExakt}}, {{KeZfMenue}} y {{KeFsMelden}} (no están en NetRadiant-Custom).",
    "While the graphics card bakes, the processor works at the same time: it computes the lightmaps the graphics card leaves to it, takes over lightmaps the graphics card has not started yet, and shares the points of the light grid with it - whoever is faster does more. With `-randomsamples` (**[Final] Extreme Quality**) the graphics card computes every lightmap that needs no random supersampling; a lightmap with random supersampling is computed on the processor, with the same random numbers as on one thread. The result stays bit-identical.":
        "Mientras la tarjeta gráfica hornea, el procesador trabaja a la vez: calcula los lightmaps que le deja la tarjeta gráfica, se hace cargo de los lightmaps que la tarjeta aún no ha empezado y se reparte con ella los puntos de la rejilla de luz; quien es más rápido hace más. Con `-randomsamples` (**[Final] Extreme Quality**), la tarjeta gráfica calcula cada lightmap que no necesita supermuestreo aleatorio; un lightmap con supermuestreo aleatorio se calcula en el procesador, con los mismos números aleatorios que en un solo hilo. El resultado sigue siendo idéntico bit a bit.",
    "`cinematic` and `videoMap` play RoQ videos; **{{MeVideoImport}}** converts a video or GIF to RoQ and puts it into `video/`. `ITEM_TYPE_MODEL` shows the model with the camera, light and rotation of the game; the character menu builds the player from the cvars and the feeders (species, heads, colours) like the game. Sabers lie on their side with hilt and blades from the .sab: length, radius, number of blades, single/staff, colour - glow and core drawn like the renderer. Movie Duels follows `cg_SFXSabers` with all its styles (see the next page).":
        "`cinematic` y `videoMap` reproducen vídeos RoQ; **{{MeVideoImport}}** convierte un vídeo o GIF a RoQ y lo pone en `video/`. `ITEM_TYPE_MODEL` muestra el modelo con la cámara, la luz y la rotación del juego; el menú de personaje construye el jugador a partir de las cvars y los feeders (especies, cabezas, colores) como el juego. Los sables yacen de lado con la empuñadura y las hojas del .sab: longitud, radio, número de hojas, simple/doble, color; el brillo y el núcleo se dibujan como en el renderizador. Movie Duels sigue `cg_SFXSabers` con todos sus estilos (ver la página siguiente).",
    "Menu editor: HUD, Asian texts and sabers":
        "Editor de menús: HUD, textos asiáticos y sables",
    "Edit the HUD with example values like the game draws it, write your own texts in Korean, Japanese and Chinese, all Movie Duels blade styles and black blades, the character's pose and the sounds like in the game.":
        "Edita el HUD con valores de ejemplo tal como lo dibuja el juego, escribe tus propios textos en coreano, japonés y chino, todos los estilos de hoja de Movie Duels y las hojas negras, la pose del personaje y los sonidos como en el juego.",
    "HUD editor":
        "Editor del HUD",
    "ui/hud.menu with health 70, armor 30, force 100 and the saber style medium - tics, numbers and crosshair drawn like the game.":
        "ui/hud.menu con salud 70, blindaje 30, Fuerza 100 y el estilo de sable medio: marcas, números y cruz filar dibujados como en el juego.",
    "Open `ui/hud.menu` like any other menu. The game does not draw the HUD with the menu alone: the game code reads the position, colour and image of named items (`health_tic1` to `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) and draws tics, numbers and pictures with the player's values on top. The preview does exactly that: **{{MeHudWerte}}** in the properties has sliders for health, armor, force, ammo, the saber style and the crosshair; full tics are white, the last one fades with the rest, armor under a quarter blinks, ammo 0 turns red. Move or recolour an item and the HUD follows at once.":
        "Abre `ui/hud.menu` como cualquier otro menú. El juego no dibuja el HUD solo con el menú: el código del juego lee la posición, el color y la imagen de los items con nombre (`health_tic1` a `4`, `armor_tic…`, `force_tic…`, `ammo_tic…`, `healthamount`, `armoramount`, `forceamount`, `ammoamount`, `saberstyle_fast/medium/strong`, `frame`) y dibuja encima marcas, números e imágenes con los valores del jugador. La vista previa hace exactamente eso: **{{MeHudWerte}}** en las propiedades tiene controles deslizantes para salud, blindaje, Fuerza, munición, el estilo de sable y la cruz filar; las marcas llenas son blancas, la última se desvanece con el resto, el blindaje por debajo de un cuarto parpadea y la munición a 0 se vuelve roja. Mueve o cambia el color de un item y el HUD lo sigue al instante.",
    "This is reproduced for Jedi Academy single player. The multiplayer game code and Movie Duels draw their HUD differently (whole-number steps, HUD modes, block points); there the preview shows only the menus and says so.":
        "Esto está recreado para Jedi Academy en un jugador. El código del juego multijugador y Movie Duels dibujan su HUD de otra forma (pasos enteros, modos de HUD, puntos de bloqueo); allí la vista previa solo muestra los menús y lo indica.",
    "Texts in Korean, Japanese and Chinese":
        "Textos en coreano, japonés y chino",
    "An own text in Korean, drawn from the game's Hangul glyph pages.":
        "Un texto propio en coreano, dibujado con las páginas de glifos hangul del juego.",
    "**{{MeTexte}}** also offers Korean, Taiwanese, Japanese and Chinese. Type normally; the file gets the encoding in which the game's font reads that language (KSC 5601, Big5, Shift-JIS, GB 2312) - other languages stay Latin-1 like the menus. Characters the game cannot show as one character (for example Hangul outside KSC 5601 or half-width katakana) are stored as `?` and listed below the table. With the preview language set to one of these, the preview draws the characters from the game's glyph pages (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) at the game's size, spacing and scale.":
        "**{{MeTexte}}** también ofrece coreano, taiwanés, japonés y chino. Escribe con normalidad; el archivo recibe la codificación con la que la fuente del juego lee ese idioma (KSC 5601, Big5, Shift-JIS, GB 2312); los demás idiomas siguen en Latin-1 como los menús. Los caracteres que el juego no puede mostrar como un solo carácter (por ejemplo, hangul fuera de KSC 5601 o katakana de ancho medio) se guardan como `?` y se enumeran bajo la tabla. Con el idioma de la vista previa en uno de estos, la vista previa dibuja los caracteres con las páginas de glifos del juego (`fonts/kor_…`, `jap_…`, `chi_…`, `tai_…`) con el tamaño, el espaciado y la escala del juego.",
    "Movie Duels blade styles and black blades":
        "Estilos de hoja de Movie Duels y hojas negras",
    "With Movie Duels data the saber menu follows `cg_SFXSabers` for every style: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (default), 5 Original Trilogy, 6 Return of the Jedi, 7 The Force Awakens, 8 Unstable, 9 Rebels, 10 Clone Wars, 11 Maul - each with its own glow and core images, sizes (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) and shape. Unstable hilts always get the unstable blade. Black blades are drawn with their blend mode, so they darken what is behind them like in the game.":
        "Con los datos de Movie Duels, el menú de sables sigue `cg_SFXSabers` en cada estilo: 0 (Raven), 1 Episode I, 2 Episode II, 3 Episode III, 4 SFX (por defecto), 5 Original Trilogy, 6 Return of the Jedi, 7 The Force Awakens, 8 Unstable, 9 Rebels, 10 Clone Wars, 11 Maul; cada uno con sus propias imágenes de brillo y de núcleo, tamaños (`cg_SFXSabersGlowSize…`, `cg_SFXSabersCoreSize…`) y forma. Las empuñaduras inestables siempre reciben la hoja inestable. Las hojas negras se dibujan con su modo de mezcla, así que oscurecen lo que tienen detrás como en el juego.",
    "The character's pose and the sounds":
        "La pose del personaje y los sonidos",
    "The animation of a model (`model_g2anim`, in the character menu `ui_char_anim`) starts when the game sets it - when the menus load, and again when a script such as `uiScript character` builds the character - and runs at the game's frame timing, blended between two frames, looping or stopping like the game. In **{{MeBedienen}}** scripts read exactly the words the game reads: `uiScript` takes its name and the words that script needs, `exec` one word - commands after them run on even without a `;`. `exec \"music …\"` starts the background music like the console.":
        "La animación de un modelo (`model_g2anim`, en el menú de personaje `ui_char_anim`) empieza cuando el juego la pone (al cargar los menús, y de nuevo cuando un script como `uiScript character` construye el personaje) y avanza con el ritmo de fotogramas del juego, mezclada entre dos fotogramas, en bucle o deteniéndose como en el juego. En **{{MeBedienen}}**, los scripts leen exactamente las palabras que lee el juego: `uiScript` toma su nombre y las palabras que necesita ese script, `exec` una palabra; los comandos que siguen se ejecutan aunque no haya `;`. `exec \"music …\"` inicia la música de fondo como la consola.",
    "Menu editor: videos, sound and readability":
        "Editor de menús: vídeos, sonido y legibilidad",
    "Import videos the way the game can play them - size, aspect ratio, frame rate, sound, the shader with videoMap - get clear messages for missing codecs, switch the preview sounds off and read the toolbars with labels.":
        "Importa vídeos tal como el juego puede reproducirlos (tamaño, proporción, fotogramas por segundo, sonido, el shader con videoMap), recibe mensajes claros cuando faltan códecs, desactiva los sonidos de la vista previa y lee las barras con texto.",
    "Importing a video":
        "Importar un vídeo",
    "The import dialog for a 1920 x 1080 video with sound: size 512 x 512, aspect ratio, frames per second, quality, sound (off by default), use in the menu and the estimated size.":
        "El diálogo de importación de un vídeo de 1920 x 1080 con sonido: tamaño 512 x 512, proporción, fotogramas por segundo, calidad, sonido (desactivado por defecto), uso en el menú y el tamaño estimado.",
    "**{{MeVideoImport}}** reads MP4, WebM, AVI, WMV, MOV and MKV through Windows, animated GIFs, and RoQ files - a RoQ is taken over unchanged. Choose **{{MeVideoZielGroesse}}**: the game plays only sides that are powers of two, with at most 512 x 512 pixels in total (full-screen videos stop otherwise, larger videos overflow the game's video buffer); 512 x 512 is the default, like every Jedi Academy and Movie Duels video. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (the game stretches the video onto the element anyway), {{MeVideoBalken}} or {{MeVideoZuschneiden}}. Pictures are reduced by averaging all covered pixels, so fine patterns do not flicker. Below the options you see the length, the number of frames and the size the file will have.":
        "**{{MeVideoImport}}** lee MP4, WebM, AVI, WMV, MOV y MKV a través de Windows, GIF animados y archivos RoQ (un RoQ se incluye sin cambios). Elige **{{MeVideoZielGroesse}}**: el juego solo reproduce lados que sean potencias de dos, con 512 x 512 píxeles en total como máximo (si no, los vídeos a pantalla completa se detienen y los vídeos más grandes desbordan el búfer de vídeo del juego); 512 x 512 es el valor por defecto, como todos los vídeos de Jedi Academy y Movie Duels. **{{MeVideoEinpassen}}**: {{MeVideoStrecken}} (de todos modos, el juego estira el vídeo sobre el elemento), {{MeVideoBalken}} o {{MeVideoZuschneiden}}. Las imágenes se reducen promediando todos los píxeles cubiertos, así que los patrones finos no parpadean. Bajo las opciones ves la duración, el número de fotogramas y el tamaño que tendrá el archivo.",
    "The imported video as background of background_video in the Jedi Academy main menu - the preview plays it like the game.":
        "El vídeo importado como fondo de background_video en el menú principal de Jedi Academy: la vista previa lo reproduce como el juego.",
    "A menu shows a video through a shader with `videoMap`. The import writes it for you: `menu/video/<name>` in `shaders/<package>.shader`, next to `video/<name>.roq`. **{{MeVideoVerwendung}}** decides what happens in the menu: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, behind all others) or **{{MeVideoAlsCinematic}}** - only multiplayer menus draw that keyword, single-player menus show nothing (the check says so). Shader, video and menu go together into the exported .pk3 and into **{{MeImSpielTesten}}**.":
        "Un menú muestra un vídeo mediante un shader con `videoMap`. La importación lo escribe por ti: `menu/video/<nombre>` en `shaders/<paquete>.shader`, junto a `video/<nombre>.roq`. **{{MeVideoVerwendung}}** decide qué pasa en el menú: **{{MeVideoAlsHintergrund}}**, **{{MeVideoNeuesElement}}** (640 x 480, detrás de todos los demás) o **{{MeVideoAlsCinematic}}**: solo los menús multijugador dibujan esa palabra clave, los menús de un jugador no muestran nada (la comprobación lo indica). El shader, el vídeo y el menú van juntos al .pk3 exportado y a **{{MeImSpielTesten}}**.",
    "Sound":
        "Sonido",
    "Menus always play their videos silently - the game starts them without sound. **{{MeVideoTon}}** is therefore off; switch it on for full-screen videos that the game plays with sound (for example `cinematic <name>` from the console or a script). The sound is stored like in the game's videos: 22050 Hz, mono or stereo, between the frames. A video without a sound track gets a note. **{{MeTonAn}}** / **{{MeTonAus}}** in the toolbar switches all preview sounds - focus and click sounds and the menu music - and stops sounds that are playing; the setting is remembered.":
        "Los menús siempre reproducen sus vídeos sin sonido: el juego los inicia sin sonido. Por eso **{{MeVideoTon}}** está desactivado; actívalo para los vídeos a pantalla completa que el juego reproduce con sonido (por ejemplo `cinematic <nombre>` desde la consola o un script). El sonido se guarda como en los vídeos del juego: 22050 Hz, mono o estéreo, entre los fotogramas. Un vídeo sin pista de sonido recibe un aviso. **{{MeTonAn}}** / **{{MeTonAus}}** en la barra activa o desactiva todos los sonidos de la vista previa (sonidos de foco y de clic y la música del menú) y detiene los que están sonando; el ajuste se recuerda.",
    "Missing codecs":
        "Códecs que faltan",
    "If Windows cannot decode a video, the dialog says which codec it is and what helps: HEVC, AV1 and VP8/VP9 need a free extension from the Microsoft Store, AVI files with DivX/Xvid or unknown codecs should be converted to MP4 (H.264). Nothing is stored in that case - no empty video.":
        "Si Windows no puede descodificar un vídeo, el diálogo dice qué códec es y qué ayuda: HEVC, AV1 y VP8/VP9 necesitan una extensión gratuita de Microsoft Store; los archivos AVI con DivX/Xvid o códecs desconocidos conviene convertirlos a MP4 (H.264). En ese caso no se guarda nada: ningún vídeo vacío.",
    "Readability":
        "Legibilidad",
    "The toolbars with labels; eye and lock in the hierarchy are real symbols.":
        "Las barras con texto; el ojo y el candado de la jerarquía son símbolos de verdad.",
    "Every toolbar symbol names itself and what it does when you hover it; **{{MeEinstLeisteText}}** in the settings puts the name next to each symbol. In the hierarchy the eye hides an element in the preview only and the lock keeps it from being moved. Text never gets smaller than 75 % of the normal size (12 pixels), however far you zoom out with Ctrl and the mouse wheel.":
        "Cada símbolo de las barras dice su nombre y lo que hace al pasar el ratón por encima; **{{MeEinstLeisteText}}** en las preferencias pone el nombre junto a cada símbolo. En la jerarquía, el ojo oculta un elemento solo en la vista previa y el candado impide moverlo. El texto nunca se hace más pequeño que el 75 % del tamaño normal (12 píxeles), por mucho que alejes con Ctrl y la rueda del ratón.",
    "Menu editor: layouts and events":
        "Editor de menús: disposiciones y eventos",
    "Arrange elements side by side, stacked, in a grid or in two columns with one click, and decide what happens on hover, click, focus and Enter with ready-made effects and action blocks - the result stays ordinary .menu code.":
        "Ordena elementos en fila, apilados, en cuadrícula o en dos columnas con un clic, y decide qué pasa al pasar el ratón, al hacer clic, con el foco y con Intro mediante efectos ya hechos y bloques de acciones: el resultado sigue siendo código .menu normal.",
    "Layouts":
        "Disposiciones",
    "Three buttons in a panel, arranged with Side by side: margins 8, spacing 8 - the layout is kept as a comment line.":
        "Tres botones en un panel, ordenados con En fila: márgenes 8, espaciado 8; la disposición se guarda como línea de comentario.",
    "Select two or more elements (Ctrl+click on the canvas or in the hierarchy) and click **{{MeLayoutWaagrecht}}**, **{{MeLayoutSenkrecht}}**, **{{MeLayoutRaster}}** or **{{MeLayoutTeiler}}** in the **{{MeLayout}}** section (Ctrl+Shift+1 to 4). If one of the selected elements encloses all the others - a panel or background - it becomes the area and the others are placed inside it; otherwise the layout uses the box around the selection. Without a selection, **{{MeLayoutZeigMir}}** selects the first elements for you.":
        "Selecciona dos o más elementos (Ctrl+clic en el lienzo o en la jerarquía) y pulsa **{{MeLayoutWaagrecht}}**, **{{MeLayoutSenkrecht}}**, **{{MeLayoutRaster}}** o **{{MeLayoutTeiler}}** en la sección **{{MeLayout}}** (Ctrl+Mayús+1 a 4). Si uno de los elementos seleccionados rodea a todos los demás (un panel o un fondo), se convierte en el área y los demás se colocan dentro; si no, la disposición usa la caja alrededor de la selección. Sin selección, **{{MeLayoutZeigMir}}** selecciona por ti los primeros elementos.",
    "Select an element of the layout to change it: **{{MeLayoutArt}}**, **{{MeLayoutAbstand}}**, **{{MeLayoutRaender}}**, alignment (**{{MeLayoutAnfang}}**, **{{MeLayoutMitte}}**, **{{MeLayoutEnde}}**, **{{MeLayoutFuellen}}**), **{{MeLayoutSpalten}}** of a grid, **{{MeLayoutTeilung}}** and **{{MeLayoutGriff}}** of two columns. **{{MeLayoutSpacerNeu}}** puts a spacer behind the selected element - it takes the free space, for example to push a button to the right edge. Move the area and the elements follow. Every change is one undo step.":
        "Selecciona un elemento de la disposición para cambiarla: **{{MeLayoutArt}}**, **{{MeLayoutAbstand}}**, **{{MeLayoutRaender}}**, alineación (**{{MeLayoutAnfang}}**, **{{MeLayoutMitte}}**, **{{MeLayoutEnde}}**, **{{MeLayoutFuellen}}**), **{{MeLayoutSpalten}}** de una cuadrícula, **{{MeLayoutTeilung}}** y **{{MeLayoutGriff}}** de dos columnas. **{{MeLayoutSpacerNeu}}** pone un espaciador detrás del elemento seleccionado: ocupa el espacio libre, por ejemplo para empujar un botón hasta el borde derecho. Mueve el área y los elementos la siguen. Cada cambio es un paso de deshacer.",
    "The game knows no layouts: the .menu only gets fixed `rect` values, and the layout is stored as a comment `// jkstudio:layout …` inside the menuDef, which the game skips and the editor reads again. Menus without a layout stay byte-identical. For texts, the width of the longest translation in all languages of the game data counts; if a text does not fit, the section names the element and the language. **{{MeLayoutVersteckt}}** lets elements that are hidden after opening (`visible 0` or `hide` in onOpen) give up their place. **{{MeLayoutAufloesen}}** removes only the comment - the rects stay.":
        "El juego no conoce las disposiciones: el .menu solo recibe valores `rect` fijos, y la disposición se guarda como un comentario `// jkstudio:layout …` dentro del menuDef, que el juego se salta y el editor vuelve a leer. Los menús sin disposición quedan idénticos byte a byte. Para los textos cuenta el ancho de la traducción más larga en todos los idiomas de los datos del juego; si un texto no cabe, la sección nombra el elemento y el idioma. **{{MeLayoutVersteckt}}** hace que los elementos ocultos tras abrir el menú (`visible 0` o `hide` en onOpen) cedan su sitio. **{{MeLayoutAufloesen}}** solo quita el comentario: los rects se quedan.",
    "Events":
        "Eventos",
    "The button got the Glow effect and a click sound: the glow element appears on hover, the click plays the sound.":
        "El botón ha recibido el efecto Brillo y un sonido de clic: el elemento de brillo aparece al pasar el ratón y el clic reproduce el sonido.",
    "The **{{MeEreignisse}}** section shows for the selected element what happens on **{{MeEvHover}}** (mouseEnter), **{{MeEvHoverEnde}}** (mouseExit), **{{MeEvKlick}}** (action), **{{MeEvFokus}}** (onFocus), **{{MeEvFokusEnde}}** (leaveFocus) and **{{MeEvEnter}}** (accept) - only the events the game's engine knows. Start with a ready-made hover effect: **{{MeEvLeuchten}}** (a glow element behind it, like the original menus), **{{MeEvFarbe}}**, **{{MeEvRahmen}}** or **{{MeEvGroesser}}**, in the colour next to them. **{{MeEvHoverKlang}}** sets `focusSound`, **{{MeEvKlickKlang}}** a `play` at the start of the click; the button next to each field lets you listen.":
        "La sección **{{MeEreignisse}}** muestra para el elemento seleccionado qué pasa en **{{MeEvHover}}** (mouseEnter), **{{MeEvHoverEnde}}** (mouseExit), **{{MeEvKlick}}** (action), **{{MeEvFokus}}** (onFocus), **{{MeEvFokusEnde}}** (leaveFocus) y **{{MeEvEnter}}** (accept): solo los eventos que conoce el motor del juego. Empieza con un efecto ya hecho al pasar el ratón: **{{MeEvLeuchten}}** (un elemento de brillo detrás, como en los menús originales), **{{MeEvFarbe}}**, **{{MeEvRahmen}}** o **{{MeEvGroesser}}**, en el color que tienen al lado. **{{MeEvHoverKlang}}** pone `focusSound`, **{{MeEvKlickKlang}}** un `play` al empezar el clic; el botón junto a cada campo te deja escucharlo.",
    "Each event lists its actions as blocks: element names as a list, colours with a colour picker, sounds with a listen button. **{{MeEvAktionNeu}}** offers only the commands of this game's engine (the commandList of ui_shared, so `setcvartocvar` only in multiplayer). The blocks write the script text - the Text tab shows the same at once - and every change is one undo step. Switch on **{{MeBedienen}}** and the events run like in the game.":
        "Cada evento enumera sus acciones como bloques: nombres de elementos como lista, colores con selector de color, sonidos con botón de escucha. **{{MeEvAktionNeu}}** solo ofrece los comandos del motor de este juego (la commandList de ui_shared, así que `setcvartocvar` solo en multijugador). Los bloques escriben el texto del script (la pestaña Texto muestra lo mismo al instante) y cada cambio es un paso de deshacer. Activa **{{MeBedienen}}** y los eventos funcionan como en el juego.",
    "Preview checked against the original game":
        "Vista previa comprobada con el juego original",
    "All Jedi Academy menus outside a running game were photographed in the original game (single and multiplayer) and compared with the preview. Since then the preview also draws: missing `@` texts as nothing (like the game), missing images of shader elements as the game's grey default box, the colours, heads and parts of the species set in `ui_char_model`, the multiplayer start values (player name, model, server name, limits), the multiplayer head list in rows and columns, and the multiplayer owner-draws for team slots, game type, skill, team colour and map preview.":
        "Todos los menús de Jedi Academy fuera de una partida en curso se fotografiaron en el juego original (un jugador y multijugador) y se compararon con la vista previa. Desde entonces, la vista previa también dibuja: los textos `@` que faltan como nada (como el juego), las imágenes que faltan de elementos con shader como la caja gris por defecto del juego, los colores, cabezas y partes de la especie puesta en `ui_char_model`, los valores iniciales del multijugador (nombre del jugador, modelo, nombre del servidor, límites), la lista de cabezas del multijugador en filas y columnas, y los owner-draws del multijugador para puestos de equipo, tipo de juego, dificultad, color de equipo y vista previa del mapa.",
    "Menu editor: projects, mod profiles, Jedi Outcast":
        "Editor de menús: proyectos, perfiles de mod, Jedi Outcast",
    "Keep a menu mod in a project folder laid out like the pk3, with automatic backup and restore after a crash; tell the editor what a mod's engine knows with a mod profile; layouts that follow show and hide; Jedi Outcast menus tested in the real game.":
        "Guarda un mod de menús en una carpeta de proyecto organizada como el pk3, con copia automática y restauración tras un cierre inesperado; indica al editor lo que conoce el motor de un mod con un perfil de mod; disposiciones que siguen a show y hide; menús de Jedi Outcast probados en el juego real.",
    "Projects":
        "Proyectos",
    "**{{MePrjProjekt}} > {{MePrjNeu}}** creates a project file `*.jksmenu` with the folders `ui`, `gfx`, `video`, `shaders` and `strings` next to it - the same layout as the finished pk3. Menus that are open come along; without one, the project starts with an empty full-screen menu. In a project, **{{MeSpeichern}}** saves everything: the menus, their images, videos, texts and fonts, and the project file. The project file remembers the game, the mod profile, the menus and which one was open, the preview (resolution, letterbox, language, cvars) and the export (pk3 name, only changes). **{{MePrjExport}}** packs the pk3.":
        "**{{MePrjProjekt}} > {{MePrjNeu}}** crea un archivo de proyecto `*.jksmenu` con las carpetas `ui`, `gfx`, `video`, `shaders` y `strings` a su lado: la misma organización que el pk3 terminado. Los menús abiertos se incluyen; sin ninguno, el proyecto empieza con un menú vacío a pantalla completa. En un proyecto, **{{MeSpeichern}}** lo guarda todo: los menús, sus imágenes, vídeos, textos y fuentes, y el archivo del proyecto. El archivo del proyecto recuerda el juego, el perfil de mod, los menús y cuál estaba abierto, la vista previa (resolución, bandas, idioma, cvars) y la exportación (nombre del pk3, solo cambios). **{{MePrjExport}}** empaqueta el pk3.",
    "Every 30 seconds the editor backs up menus that are not saved yet. If JK Studio was not closed properly, it asks the next time whether to restore them - the restored text counts as an unsaved change, so Undo goes back to the saved version. A double-click on a `.jksmenu` opens JK Studio once you allowed it with **{{MePrjZuordnen}}** (only for your Windows account).":
        "Cada 30 segundos, el editor hace una copia de los menús que aún no están guardados. Si JK Studio no se cerró correctamente, la vez siguiente pregunta si quieres restaurarlos: el texto restaurado cuenta como un cambio sin guardar, así que Deshacer vuelve a la versión guardada. Un doble clic en un `.jksmenu` abre JK Studio cuando lo hayas permitido con **{{MePrjZuordnen}}** (solo para tu cuenta de Windows).",
    "After a crash: the editor offers the backed-up menus.":
        "Tras un cierre inesperado: el editor ofrece los menús copiados.",
    "Mod profiles":
        "Perfiles de mod",
    "A mod's engine may know more than the game: extra keywords, owner-draw and feeder numbers, uiScripts, cvars and menu lists. Choose the mod's profile under **{{MePrjProjekt}} > {{MeProfil}}**: the editor then reads these keywords without a message, shows owner-draws as labelled placeholders and lists the values under **Mod keywords** - the file stays exactly as it is. With the mod's source code, `tools/gen_menue_profil.py` writes the profile; without it, **{{MeProfilVorlage}}** writes a template to fill in. Jedi Academy, Jedi Outcast and Movie Duels have built-in profiles.":
        "El motor de un mod puede conocer más que el juego: palabras clave extra, números de owner-draw y de feeder, uiScripts, cvars y listas de menús. Elige el perfil del mod en **{{MePrjProjekt}} > {{MeProfil}}**: entonces el editor lee esas palabras clave sin mensaje, muestra los owner-draws como marcadores con rótulo y enumera los valores en **Palabras clave del mod**; el archivo se queda exactamente como está. Con el código fuente del mod, `tools/gen_menue_profil.py` escribe el perfil; sin él, **{{MeProfilVorlage}}** escribe una plantilla para rellenar. Jedi Academy, Jedi Outcast y Movie Duels tienen perfiles integrados.",
    "A mod keyword from the profile, labelled in the properties.":
        "Una palabra clave del mod del perfil, con rótulo en las propiedades.",
    "Layouts that follow show and hide":
        "Disposiciones que siguen a show y hide",
    "With **{{MeLayoutVersteckt}}** switched on, every script that shows or hides elements of the layout (a click, a hover, onOpen) also gets `setitemrect` lines that move the other elements into place - calculated for the state after that script and kept up to date with every layout change. Jedi Outcast has no `setitemrect`; there the editor explains what to do instead.":
        "Con **{{MeLayoutVersteckt}}** activado, cada script que muestra u oculta elementos de la disposición (un clic, el paso del ratón, onOpen) también recibe líneas `setitemrect` que recolocan los demás elementos, calculadas para el estado tras ese script y actualizadas con cada cambio de la disposición. Jedi Outcast no tiene `setitemrect`; allí el editor explica qué hacer en su lugar.",
    "Button 2 is hidden after opening; Show and Hide move button 3 like the layout.":
        "El botón 2 está oculto tras abrir; Mostrar y Ocultar mueven el botón 3 como la disposición.",
    "Jedi Outcast":
        "Jedi Outcast",
    "The Jedi Outcast menus were compared with the real game (Steam version). The editor now follows its engine: no `setitemrect`, `setitemtext`, `transition2` or `delay`, no `accept` event, the logo models (`.md3`) with the game's camera. **{{MeImSpielTesten}}** starts `jk2sp.exe` with your own folder as `fs_basepath` and the game data via `fs_cdpath` - nothing is written into the game folder.":
        "Los menús de Jedi Outcast se compararon con el juego real (versión de Steam). El editor sigue ahora su motor: sin `setitemrect`, `setitemtext`, `transition2` ni `delay`, sin el evento `accept`, los modelos del logotipo (`.md3`) con la cámara del juego. **{{MeImSpielTesten}}** inicia `jk2sp.exe` con tu propia carpeta como `fs_basepath` y los datos del juego mediante `fs_cdpath`: no se escribe nada en la carpeta del juego.",
    "Your first own menu":
        "Tu primer menú propio",
    "From an empty editor to a project with three buttons side by side that glow on hover - saved, ready to pack as a pk3.":
        "De un editor vacío a un proyecto con tres botones en fila que brillan al pasar el ratón: guardado y listo para empaquetar como pk3.",
    "Menu editor · about 5 minutes":
        "Editor de menús · unos 5 minutos",
    "The pictures were taken while the program did exactly these steps; the self-test checks after each step that it really happened. In the program, the helper (F1) runs this tutorial as a tour and moves on by itself when a step is done.":
        "Las imágenes se tomaron mientras el programa hacía exactamente estos pasos; la autocomprobación verifica tras cada paso que de verdad ha ocurrido. En el programa, el asistente (F1) ejecuta este tutorial como recorrido y avanza solo cuando un paso está hecho.",
    "**Open the menu editor.** Click **{{ViewMenuEditor}}** in the mode bar.":
        "**Abre el editor de menús.** Pulsa **{{ViewMenuEditor}}** en la barra de modos.",
    "The empty menu editor with the project buttons.":
        "El editor de menús vacío con los botones de proyecto.",
    "**Start a project.** Click **{{MePrjNeu}}** and choose a folder for the project file. The editor creates the folders like in a pk3 and an empty full-screen menu.":
        "**Empieza un proyecto.** Pulsa **{{MePrjNeu}}** y elige una carpeta para el archivo del proyecto. El editor crea las carpetas como en un pk3 y un menú vacío a pantalla completa.",
    "The new project with its first menu.":
        "El proyecto nuevo con su primer menú.",
    "**Three buttons.** Double-click **{{MeWButton}}** in the building blocks three times, or drag it onto the canvas.":
        "**Tres botones.** Haz doble clic tres veces en **{{MeWButton}}** en los bloques de construcción, o arrástralo al lienzo.",
    "Three new buttons next to each other, not yet evenly spaced.":
        "Tres botones nuevos uno junto a otro, aún sin espaciado uniforme.",
    "**Side by side.** Select the three buttons (Ctrl+click) and click **{{MeLayoutWaagrecht}}** under Layout in the properties.":
        "**En fila.** Selecciona los tres botones (Ctrl+clic) y pulsa **{{MeLayoutWaagrecht}}** en Disposición, en las propiedades.",
    "The buttons side by side with even spacing.":
        "Los botones en fila con el mismo espaciado.",
    "**Glow on hover.** Select the first button, open the **Events** tab and click **{{MeEvLeuchten}}**.":
        "**Brillo al pasar el ratón.** Selecciona el primer botón, abre la pestaña **Eventos** y pulsa **{{MeEvLeuchten}}**.",
    "The glow effect: show on hover, hide when the mouse leaves.":
        "El efecto de brillo: se muestra al pasar el ratón y se oculta cuando el ratón sale.",
    "**Save.** Click **{{MeSpeichern}}** - in a project it saves everything. **{{MePrjProjekt}} > {{MePrjExport}}** then packs the pk3 for the game.":
        "**Guarda.** Pulsa **{{MeSpeichern}}**: en un proyecto lo guarda todo. Después, **{{MePrjProjekt}} > {{MePrjExport}}** empaqueta el pk3 para el juego.",
    "Saved: the project file and the menu in the project folder.":
        "Guardado: el archivo del proyecto y el menú en la carpeta del proyecto.",
    "Arrange windows and screens":
        "Organizar ventanas y pantallas",
    "Take a panel out":
        "Sacar un panel",
    "The menu {{DockMenu}} of the mission: show or hide panels, put them into their own window, save and load layouts.":
        "El menú {{DockMenu}} de la misión: mostrar u ocultar paneles, ponerlos en su propia ventana, guardar y cargar disposiciones.",
    "**{{DockPanels}}** hides a panel completely; its space goes to the neighbours as well. The section **{{DockOther}}** opens the helper, the tutorials and the messages - they are windows of their own anyway.":
        "**{{DockPanels}}** oculta un panel por completo; su espacio también pasa a los vecinos. La sección **{{DockOther}}** abre el asistente, los tutoriales y los mensajes: de todos modos son ventanas propias.",
    "Script Flow dropped onto the timeline window: both panels in one window, as tabs. The mission view uses the whole main window.":
        "Script Flow soltado sobre la ventana de la línea de tiempo: ambos paneles en una ventana, como pestañas. La vista de misión usa toda la ventana principal.",
    "Dock a panel back":
        "Volver a acoplar un panel",
    "Or click the panel under **{{DockOwnWindow}}** again.":
        "O vuelve a pulsar el panel en **{{DockOwnWindow}}**.",
    "Save, load and reset layouts":
        "Guardar, cargar y restablecer disposiciones",
    "Screens that change":
        "Pantallas que cambian",
    "A screen is unplugged or arranged differently: windows that were on it come back onto the screen of the main window, at their size.":
        "Se desconecta una pantalla o se organiza de otra forma: las ventanas que estaban en ella vuelven a la pantalla de la ventana principal, con su tamaño.",
    "A screen is smaller than a window: the window is made just small enough to fit.":
        "Una pantalla es más pequeña que una ventana: la ventana se reduce justo lo necesario para que quepa.",
    "A window moves onto a screen with a different scale (100 %, 150 % ...): its size and text follow the scale of that screen.":
        "Una ventana pasa a una pantalla con otra escala (100 %, 150 % ...): su tamaño y su texto siguen la escala de esa pantalla.",
    "Map editor":
        "Editor de mapas",
    "The first value of `camera` chooses the command. behaved shows every one of them in the timeline (in the **{{ZlDetail}}** view in its own row) and plays it in the view. Times are in milliseconds.":
        "El primer valor de `camera` elige el comando. behaved muestra cada uno de ellos en la línea de tiempo (en la vista **{{ZlDetail}}**, en su propia fila) y lo reproduce en la vista. Los tiempos van en milisegundos.",
    "Shows the whole scene.":
        "Muestra toda la escena.",
    "One row per actor and the camera shots with their names - the scene at a glance. [[zeitleiste#zeilen]]":
        "Una fila por actor y los planos de cámara con su nombre: la escena de un vistazo. [[zeitleiste#zeilen]]",
    "Camera move, pan, zoom and fade separately, field of view and fade as lines, every actor's animations and commands.":
        "Movimiento, giro, zoom y fundido de cámara por separado, campo de visión y fundido como líneas, las animaciones y los comandos de cada actor.",
    "Only shows actors that do something in the visible section.":
        "Muestra solo los actores que hacen algo en el tramo visible.",
    "Only shows places with notes: camera moves that jump back, commands the preview does not simulate.":
        "Muestra solo los puntos con avisos: movimientos de cámara que saltan atrás, comandos que la vista previa no simula.",
    "**See it in the timeline.** The **{{ZlSkript}}** row shows every wait as a hatched block that says what it waits for, and a click on it names who sends the signal and when. If no script sends the signal, the side bar says so - in the game the script would wait forever.":
        "**Míralo en la línea de tiempo.** La fila **{{ZlSkript}}** muestra cada espera como un bloque rayado que dice lo que espera, y un clic en él nombra quién envía la señal y cuándo. Si ningún script envía la señal, la barra lateral lo dice: en el juego, el script esperaría para siempre.",
    "**See them in the timeline.** In the **{{ZlDetail}}** view the timeline has its own rows for zoom, fade and shake, and draws the field of view and the fade as lines.":
        "**Míralos en la línea de tiempo.** En la vista **{{ZlDetail}}**, la línea de tiempo tiene filas propias para el zoom, el fundido y el temblor, y dibuja el campo de visión y el fundido como líneas.",
    "**Change a duration.** Drag the right edge of a camera block or a wait to make it shorter or longer. A walk keeps its length: it comes from the path finding and the speed of the figure.":
        "**Cambia una duración.** Arrastra el borde derecho de un bloque de cámara o de una espera para acortarlo o alargarlo. Un recorrido conserva su duración: sale de la búsqueda de caminos y de la velocidad de la figura.",
    "**Step exactly.** With the mouse over the timeline, <kbd>Left</kbd> / <kbd>Right</kbd> move the playhead by 50 ms, with <kbd>Shift</kbd> by one second, with <kbd>Ctrl</kbd> by five seconds. The time display shows where you are; click it to switch to frames.":
        "**Avanza con precisión.** Con el ratón sobre la línea de tiempo, <kbd>←</kbd> / <kbd>→</kbd> mueven el cabezal 50 ms, con <kbd>Mayús</kbd> un segundo, con <kbd>Ctrl</kbd> cinco segundos. El indicador de tiempo muestra dónde estás; púlsalo para cambiar a fotogramas.",
    "**Rows.** **{{ZlUebersicht}}** shows one row per actor, **{{ZlDetail}}** the camera split into move, pan, zoom and fade and every actor's animations and commands. **{{ZlNurAktive}}** hides actors that do nothing in the visible section.":
        "**Filas.** **{{ZlUebersicht}}** muestra una fila por actor, **{{ZlDetail}}** la cámara dividida en movimiento, giro, zoom y fundido, y las animaciones y los comandos de cada actor. **{{ZlNurAktive}}** oculta los actores que no hacen nada en el tramo visible.",
    "**Camera rows.** Switch to **{{ZlDetail}}** to see move, pan, zoom, fade and shake in their own rows. A move that starts before the previous one has finished gets its own lane and the red note **{{ZlSpringt}}** - in the game the camera jumps back; shorten the earlier one or start the later one after it.":
        "**Filas de cámara.** Cambia a **{{ZlDetail}}** para ver movimiento, giro, zoom, fundido y temblor en filas propias. Un movimiento que empieza antes de que termine el anterior recibe su propio carril y el aviso rojo **{{ZlSpringt}}**: en el juego la cámara salta atrás; acorta el anterior o haz que el siguiente empiece después.",
    "**Check it.** Open the figure's row in the timeline: its row **{{ZlBefehle}}** shows the look commands at their time.":
        "**Compruébalo.** Despliega la fila de la figura en la línea de tiempo: su fila **{{ZlBefehle}}** muestra los comandos de mirada en su momento.",
    "The rows of the opened figure: walks above, its commands below.":
        "Las filas de la figura desplegada: arriba los recorridos, debajo sus comandos.",
    "**Time it.** A `wait` after the sound keeps the next command until the line is over. In the **{{ZlSprache}}** row the line is as long as its sound file - drag the right edge of the wait in **{{ZlSkript}}** until it fits.":
        "**Ajusta el tiempo.** Un `wait` después del sonido retiene el siguiente comando hasta que termina la frase. En la fila **{{ZlSprache}}**, la frase dura lo mismo que su archivo de sonido: arrastra el borde derecho del wait en **{{ZlSkript}}** hasta que encaje.",
    # uebersetzung2 nach Integration 5
    "Menu editor: flow, tab order, styles and templates":
        "Editor de menús: flujo, orden de tabulación, estilos y plantillas",
    "See which menu opens which and where players get stuck, set the keyboard order with clicks, change colours and fonts everywhere at once, start from ready-made menus or a settings wizard, trace a design over a reference image and reset elements to the game's original.":
        "Mira qué menú abre a cuál y dónde se atascan los jugadores, fija el orden del teclado con clics, cambia colores y fuentes en todas partes a la vez, empieza con menús ya hechos o con un asistente de ajustes, calca un diseño sobre una imagen de referencia y restablece elementos al original del juego.",
    "Menu flow":
        "Flujo de menús",
    "The flow of a small menu set: the main menu (green frame) opens Options and Credits; Credits is a dead end (red), 'waise' is never opened (grey).":
        "El flujo de un conjunto de menús pequeño: el menú principal (marco verde) abre Options y Credits; Credits es un callejón sin salida (rojo) y 'waise' nunca se abre (gris).",
    "**{{MeFluss}}** in the toolbar shows every menu of the open files as a box and every `open` as an arrow, column by column from the menus the game opens itself (green frame: the main menu, the loading screen, the in-game menus - taken from the engine code). Red boxes are dead ends - they take input but nothing leads out - or targets that no open file defines; orange boxes are opened from another menu but have no way back (no `close` of themselves, no onESC, no `open` of the menu they came from); grey boxes are never reached. The list below says in plain words what to do. Click a box or a line of the list to jump to that menu. Open the whole menu set first, otherwise menus from other files look missing.":
        "**{{MeFluss}}** en la barra muestra cada menú de los archivos abiertos como una caja y cada `open` como una flecha, columna a columna desde los menús que abre el propio juego (marco verde: el menú principal, la pantalla de carga, los menús durante la partida, tomados del código del motor). Las cajas rojas son callejones sin salida (reciben entradas, pero nada lleva fuera) o destinos que ningún archivo abierto define; las naranjas se abren desde otro menú pero no tienen camino de vuelta (ningún `close` de sí mismas, ningún onESC, ningún `open` del menú del que vienen); las grises nunca se alcanzan. La lista de abajo dice con palabras sencillas qué hacer. Pulsa una caja o una línea de la lista para ir a ese menú. Abre antes todo el conjunto de menús; si no, los menús de otros archivos parecen ausentes.",
    "Tab order":
        "Orden de tabulación",
    "The numbers show the order in which Tab and Arrow down move the focus in the game.":
        "Los números muestran el orden en que Tab y Flecha abajo mueven el foco en el juego.",
    "**{{MeTabFolge}}** puts a number on every element that can take the keyboard focus, in the order the engine uses (Menu_SetNextCursorItem: the order of the itemDefs, without decorations, hidden elements and elements switched off by a cvar; from no focus the game starts at the second element of the file). To change it, click the elements one after another in the new order - after the last click the editor rearranges the itemDefs (one Undo step). The game draws in the same order, so a later element lies over an earlier one; elements that cannot take the focus stay where they are.":
        "**{{MeTabFolge}}** pone un número en cada elemento que puede recibir el foco del teclado, en el orden que usa el motor (Menu_SetNextCursorItem: el orden de los itemDef, sin decoraciones, elementos ocultos ni elementos desactivados por una cvar; sin foco, el juego empieza por el segundo elemento del archivo). Para cambiarlo, pulsa los elementos uno tras otro en el orden nuevo: tras el último clic, el editor reordena los itemDef (un paso de deshacer). El juego dibuja en el mismo orden, así que un elemento posterior queda encima de uno anterior; los elementos que no pueden recibir el foco se quedan donde están.",
    "Styles":
        "Estilos",
    "All colours, fonts, text sizes and backgrounds of the file with their number of uses.":
        "Todos los colores, fuentes, tamaños de texto y fondos del archivo con su número de usos.",
    "**{{MeStile}}** collects every colour (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), font, text size and background of the file - the most used first. Change a value and every place that uses it changes at once, as ordinary values in the .menu (spacing, comments and quotes stay; one Undo step). Values from #include files or macros are counted but stay as they are.":
        "**{{MeStile}}** reúne cada color (forecolor, backcolor, bordercolor, outlinecolor, focusColor, disableColor, descColor), fuente, tamaño de texto y fondo del archivo, los más usados primero. Cambia un valor y cambian a la vez todos los sitios que lo usan, como valores normales en el .menu (los espacios, comentarios y comillas se mantienen; un paso de deshacer). Los valores de archivos #include o de macros se cuentan, pero se quedan como están.",
    "Templates and settings wizard":
        "Plantillas y asistente de ajustes",
    "A settings page with three tabs from the template - the tabs appear as pages above the canvas.":
        "Una página de ajustes con tres pestañas a partir de la plantilla: las pestañas aparecen como páginas sobre el lienzo.",
    "The **+** next to the pages offers **{{MeVorlageLeer}}** or a whole menu: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** and **{{MeVorlageLaden}}**. They use only keywords every game version reads; the main menu and the loading screen get the names the engine looks for (single player `mainMenu` and `loadscreen`, multiplayer `main` and `Connect`), and the main menu opens the settings and credits templates - the menu flow shows the result without problems.":
        "El **+** junto a las páginas ofrece **{{MeVorlageLeer}}** o un menú entero: **{{MeVorlageHaupt}}**, **{{MeVorlageEinst}}**, **{{MeVorlageCredits}}** y **{{MeVorlageLaden}}**. Solo usan palabras clave que lee cualquier versión del juego; el menú principal y la pantalla de carga reciben los nombres que busca el motor (en un jugador `mainMenu` y `loadscreen`, en multijugador `main` y `Connect`), y el menú principal abre las plantillas de ajustes y de créditos: el flujo de menús muestra el resultado sin problemas.",
    "The settings wizard knows the controls of the game's own menus.":
        "El asistente de ajustes conoce los controles de los propios menús del juego.",
    "Reference image, original and limits":
        "Imagen de referencia, original y límites",
    "A reference image under the canvas.":
        "Una imagen de referencia bajo el lienzo.",
    "**{{MeRefBild}}** places a PNG, JPG or TGA semi-transparently on the canvas, under the menu or - when a full-screen background covers it - over it (**{{MeRefOben}}**), with adjustable **{{MeRefDeckkraft}}**. It is only a drawing aid and never goes into the menu or the pk3.":
        "**{{MeRefBild}}** pone un PNG, JPG o TGA semitransparente sobre el lienzo, bajo el menú o, cuando un fondo a pantalla completa lo tapa, encima (**{{MeRefOben}}**), con **{{MeRefDeckkraft}}** ajustable. Es solo una ayuda para dibujar y nunca va al menú ni al pk3.",
    "An element of the game's main menu was moved - the Properties name the difference and offer the reset.":
        "Se ha movido un elemento del menú principal del juego: las Propiedades nombran la diferencia y ofrecen el restablecimiento.",
    "For a menu from the game, the Properties show in **{{MeOriginal}}** whether the selected element differs from the game's original file and in which keywords; **{{MeOriginalZuruecksetzen}}** puts the original itemDef back (one Undo step). Below the pages, a line shows how full the menu is against the limits of the game version - elements per menu, menus and the longest script - orange from 80 percent, red at the limit, because beyond it the game silently drops the rest.":
        "Para un menú del juego, las Propiedades muestran en **{{MeOriginal}}** si el elemento seleccionado difiere del archivo original del juego y en qué palabras clave; **{{MeOriginalZuruecksetzen}}** vuelve a poner el itemDef original (un paso de deshacer). Bajo las páginas, una línea muestra lo lleno que está el menú respecto a los límites de la versión del juego (elementos por menú, menús y el script más largo): naranja a partir del 80 por ciento, rojo en el límite, porque por encima el juego descarta el resto sin avisar.",
    "Next to the limits, **Readability** counts hints with guide values of the editor (not limits of the game): text lower than 8 points on the 640 x 480 screen, contrast below 3:1 on filled areas, and texts that fit their field in one language of the game but not in another (measured with the game's fonts and the texts of every language in the game data). Hover for the list with solutions; click to select the next element with a hint.":
        "Junto a los límites, **Legibilidad** cuenta avisos con valores orientativos del editor (no límites del juego): texto de menos de 8 puntos en la pantalla de 640 x 480, contraste por debajo de 3:1 en áreas rellenas, y textos que caben en su campo en un idioma del juego pero no en otro (medidos con las fuentes del juego y los textos de cada idioma de los datos del juego). Pasa el ratón por encima para ver la lista con soluciones; pulsa para seleccionar el siguiente elemento con un aviso.",
    "Closing a tab with unsaved changes now asks first: save, close without saving, or cancel.":
        "Cerrar una pestaña con cambios sin guardar ahora pregunta antes: guardar, cerrar sin guardar o cancelar.",
    "**Upper body and legs.** When an NPC's legs and upper body play different animations (`SET_ANIM_LOWER`/`SET_ANIM_UPPER`, a shot while walking), the preview turns the spine like the game: the Motion bone of the leg animation is taken off the view and spread over the three spine bones (pitch, yaw and roll, `cg_motionBoneComp 2`); head and spine turn about the axes of their own bones.":
        "**Torso y piernas.** Cuando las piernas y el torso de un NPC reproducen animaciones distintas (`SET_ANIM_LOWER`/`SET_ANIM_UPPER`, un disparo caminando), la vista previa gira la columna como el juego: el hueso Motion de la animación de piernas se resta de la mirada y se reparte entre los tres huesos de la espalda (cabeceo, guiñada y alabeo, `cg_motionBoneComp 2`); la cabeza y el tronco giran sobre los ejes de sus propios huesos.",
    "**Lightsaber stance.** An NPC standing with a lit saber takes the stance the game gives it (PM_SetSaberMove LS_READY): the `readyAnim` of its .sab, otherwise by saber type and style - arc saber, two sabers, staff, lance or trident; while walking or running the upper body follows the legs, and standing still without input the stance covers the whole body.":
        "**Postura con sable de luz.** Un NPC de pie con la hoja encendida adopta la postura que le da el juego (PM_SetSaberMove LS_READY): el `readyAnim` de su .sab o, si no, según el tipo y el estilo del sable: sable de arco, dos sables, bastón, lanza o tridente; al caminar o correr el torso sigue a las piernas, y quieto sin entrada la postura abarca todo el cuerpo.",
    "**Standing pose.** Without a script animation an NPC stands the way the game picks it: with a gun in `BOTH_STAND3`, an Imperial officer in `BOTH_STAND4`, otherwise in `BOTH_STAND1`.":
        "**Postura de pie.** Sin animación de script, un NPC se queda de pie como lo elige el juego: con un arma de fuego en `BOTH_STAND3`, un oficial imperial en `BOTH_STAND4` y, si no, en `BOTH_STAND1`.",
    "Standing still on a slope or a step, the figure puts its feet like the game (the slope leg animations, one step every 100 ms); with a lightsaber its upper body takes the ready stance or follows the legs, as the game decides it (PM_TorsoAnimLightsaber).":
        "Si la figura está quieta en una pendiente o un escalón, apoya los pies como en el juego (las animaciones de piernas para pendientes, un paso cada 100 ms); con un sable de luz, la parte superior del cuerpo adopta la postura de combate o sigue a las piernas, según lo decida el juego (PM_TorsoAnimLightsaber).",
    # Abschluss rc607 (Uebersetzung 3): dock4, M7, reste2, Zeitleiste 4, vis4
    "Select a figure (an NPC) in the view or in its row and press **{{AufKnopf}}** in the transport bar: from the time of the playhead you steer it in the preview - <kbd>W</kbd>/<kbd>S</kbd>/<kbd>A</kbd>/<kbd>D</kbd> move, <kbd>Shift</kbd> walks instead of running, <kbd>Q</kbd>/<kbd>E</kbd> or the right mouse button turn. It moves with the game's movement code (Pmove), at the speeds from its .npc file and against the walls of the map. The camera follows the figure. Its model moves as in the game: the legs run, walk or step backwards as the game picks them, turn up to 60° toward the direction it moves while the upper body keeps looking ahead, blend from one animation to the next, and a weapon stance or a held animation of the upper body stays; the path is drawn, and the numbered circles are the walk targets that will be written. **{{AufFertig}}** (<kbd>Enter</kbd>) writes them, **{{AufAbbrechen}}** (<kbd>Esc</kbd>) changes nothing.":
        "Selecciona una figura (un NPC) en la vista o en su fila y pulsa **{{AufKnopf}}** en la barra de reproducción: desde el momento del cabezal la manejas en la vista previa - <kbd>W</kbd>/<kbd>S</kbd>/<kbd>A</kbd>/<kbd>D</kbd> la mueven, <kbd>Mayús</kbd> la hace andar en vez de correr, <kbd>Q</kbd>/<kbd>E</kbd> o el botón derecho del ratón la giran. Se mueve con el código de movimiento del juego (Pmove), a las velocidades de su archivo .npc y contra las paredes del mapa. La cámara sigue a la figura. Su modelo se mueve como en el juego: las piernas corren, andan o retroceden según lo elija el juego, giran hasta 60° hacia la dirección del movimiento mientras el torso sigue mirando al frente, pasan con un fundido de una animación a la siguiente, y una postura de arma o una animación mantenida del torso se conserva; el recorrido se dibuja, y los círculos numerados son los destinos que se escribirán. **{{AufFertig}}** (<kbd>Intro</kbd>) los escribe, **{{AufAbbrechen}}** (<kbd>Esc</kbd>) no cambia nada.",
    "With `-randomsamples` (**[Final] Extreme Quality**) there is a third choice: the graphics card also computes the lightmaps with random subsamples (it gathers them over several blocks and computes them together, while the processor takes the biggest ones). It is faster only on some maps (with many small lightmaps, such as kejim_post or taspir1) and slower on others (duel_mos_espa). So **{{KeGpuAuto}}** measures it once only after the graphics card has been faster than the processor on this map, and takes it from then on only where it was the fastest of the three. The result is the same, byte for byte. The console then says \"GPU bake (Auto): {{KeGpuWahlGpuZ}} - ...\".":
        "Con `-randomsamples` (**[Final] Extreme Quality**) hay una tercera opción: la tarjeta gráfica calcula también los lightmaps con submuestras aleatorias (los reúne de varios bloques y los calcula juntos, mientras el procesador se encarga de los más grandes). Solo es más rápida en algunos mapas (con muchos lightmaps pequeños, como kejim_post o taspir1) y más lenta en otros (duel_mos_espa). Por eso **{{KeGpuAuto}}** la mide una vez, y solo después de que la tarjeta gráfica haya sido más rápida que el procesador en este mapa, y a partir de entonces la usa solo donde fue la más rápida de las tres. El resultado es el mismo, byte a byte. La consola dice entonces \"Horneado en GPU (Auto): {{KeGpuWahlGpuZ}} - ...\".",
    "Put any panel anywhere - beside, below or as a tab of another one in the main window, or in its own window on a second screen - and save the arrangement under a name, per area.":
        "Pon cualquier panel donde quieras - al lado, debajo o como pestaña de otro en la ventana principal, o en su propia ventana en una segunda pantalla - y guarda la disposición con un nombre, por área.",
    "Every area - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** and **{{ViewMenuEditor}}** - is made of panels that are docked in the main window. As long as you move nothing, every panel stays in its place: the standard layout, to the point exactly as before, with the same dividers.":
        "Cada área - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** y **{{ViewMenuEditor}}** - se compone de paneles acoplados en la ventana principal. Mientras no muevas nada, cada panel se queda en su sitio: la disposición estándar, exacta al punto como antes, con los mismos divisores.",
    "Open the menu **{{DockMenu}}** (in the map editor it is the last menu, after NetRadiant's) and click the panel under **{{DockOwnWindow}}**, for example the timeline. It becomes a window of its own; its place in the main window goes to its neighbours - the mission view gets taller.":
        "Abre el menú **{{DockMenu}}** (en el editor de mapas es el último menú, después de los de NetRadiant) y pulsa el panel bajo **{{DockOwnWindow}}**, por ejemplo la línea de tiempo. Se convierte en una ventana propia; su sitio en la ventana principal pasa a sus vecinos - la vista de la misión se hace más alta.",
    "Or point at the top left corner of a panel: a small triangle appears. Drag it away and the panel comes out with it; a click on it shows the panel's tab instead.":
        "O señala la esquina superior izquierda de un panel: aparece un pequeño triángulo. Arrástralo fuera y el panel sale con él; un clic sobre él muestra en su lugar la pestaña del panel.",
    "Drag the window by its title bar out of the main window: it becomes a real Windows window and can go onto a second screen. It keeps drawing there - the 3D views and the real-time light of the map editor included. Files can be dragged onto such a window as onto the main window (for example a model onto the XY view of the map editor, or a .menu file onto the canvas of the menu editor).":
        "Arrastra la ventana por su barra de título fuera de la ventana principal: se convierte en una ventana real de Windows y puede ir a una segunda pantalla. Allí sigue dibujando, incluidas las vistas 3D y la luz en tiempo real del editor de mapas. Se pueden arrastrar archivos a una ventana así igual que a la ventana principal (por ejemplo un modelo a la vista XY del editor de mapas, o un archivo .menu al lienzo del editor de menús).",
    "The triangle in the top left corner of a panel appears under the mouse: drag it to take the panel out.":
        "El triángulo de la esquina superior izquierda de un panel aparece bajo el ratón: arrástralo para sacar el panel.",
    "Combine panels and dock them anywhere":
        "Combina paneles y acóplalos donde quieras",
    "Drag the window of a panel over another panel - in its own window or in the main window: markers appear. Drop it on the middle marker and both become tabs; drop it on an arrow and it sits beside, above or below. So the Script Flow can go under the map view, the entity layers can become a tab of the map view, the camera of the map editor can go below the XY view - like in Blender. The dividers between docked panels can then be dragged, a tab can be dragged out again. This works for the panels of one area; dialogs such as Find stay on their own.":
        "Arrastra la ventana de un panel sobre otro panel - en su propia ventana o en la ventana principal: aparecen marcadores. Suéltala en el marcador central y ambos se convierten en pestañas; suéltala en una flecha y queda al lado, encima o debajo. Así el flujo del script puede ir bajo la vista del mapa, las capas de entidades pueden ser una pestaña de la vista del mapa y la cámara del editor de mapas puede ir debajo de la vista XY, como en Blender. Después se pueden arrastrar los divisores entre paneles acoplados, y una pestaña se puede volver a sacar. Esto funciona con los paneles de un área; los diálogos como Buscar siguen por su cuenta.",
    "A panel window dragged over the main window: the markers show where it will dock - here as a tab of the mission view.":
        "Una ventana de panel arrastrada sobre la ventana principal: los marcadores muestran dónde se acoplará - aquí como pestaña de la vista de la misión.",
    "Or drop it on a marker in the main window, wherever you want it.":
        "O suéltala en un marcador de la ventana principal, donde la quieras.",
    "**{{DockReset}}** brings all panels of the area back to the standard layout.":
        "**{{DockReset}}** devuelve todos los paneles del área a la disposición estándar.",
    "Each area has its own layouts. **{{DockSaveAs}}** stores where every panel is - docked in the main window, in its own window, hidden -, where these windows are, on which screen, and how they are combined. **{{DockLoad}}** switches to a saved layout, **{{DockDelete}}** removes one, **{{DockReset}}** brings every panel of the area back to the standard layout. The current layout of every area is kept in the settings and is there again at the next start.":
        "Cada área tiene sus propias disposiciones. **{{DockSaveAs}}** guarda dónde está cada panel - acoplado en la ventana principal, en su propia ventana u oculto -, dónde están esas ventanas, en qué pantalla y cómo se combinan. **{{DockLoad}}** cambia a una disposición guardada, **{{DockDelete}}** elimina una, **{{DockReset}}** devuelve cada panel del área a la disposición estándar. La disposición actual de cada área se guarda en la configuración y vuelve a estar ahí en el siguiente inicio.",
    "NetRadiant's layouts stay the default (Preferences > Layout): **Regular**, **Regular Left** and **Split** (also with freely assigned places and with the built-in entity window) each have their standard layout with exactly NetRadiant's sizes and NetRadiant's dividers - dragging, collapsing and **Maximize view** work as in NetRadiant. A view taken out gives its place to its neighbours like a hidden view in NetRadiant; in its own window it draws everything it shows in the main window, a click into it makes it the active view, keyboard shortcuts and free look then work in that window - on every screen. Once a view is docked somewhere else (for example as a tab of the camera), ImGui's dividers apply; **{{DockReset}}** brings NetRadiant's layout back. In **Floating** the views float anyway.":
        "Las disposiciones de NetRadiant siguen siendo las predeterminadas (Preferences > Layout): **Regular**, **Regular Left** y **Split** (también con lugares asignados libremente y con la ventana de entidades integrada) tienen cada una su disposición estándar con exactamente los tamaños y los divisores de NetRadiant - arrastrar, plegar y **Maximize view** funcionan como en NetRadiant. Una vista sacada cede su sitio a sus vecinas como una vista oculta en NetRadiant; en su propia ventana dibuja todo lo que muestra en la ventana principal, un clic en ella la convierte en la vista activa, y los atajos de teclado y la vista libre funcionan entonces en esa ventana - en cualquier pantalla. En cuanto una vista se acopla en otro sitio (por ejemplo como pestaña de la cámara), valen los divisores de ImGui; **{{DockReset}}** recupera la disposición de NetRadiant. En **Floating** las vistas flotan de todos modos.",
    "Both layouts of the menu editor are made of panels: with the tabs on the right **{{MePalette}}**, **{{DockPanelCanvas}}** and the column **{{DockPanelMenuSide}}**; with the text below (preferences) the palette with the hierarchy, the canvas, **{{MeReiterEigenschaften}}** and **{{MeReiterText}}** - for example the canvas large on a second screen while the text stays in the main window.":
        "Las dos disposiciones del editor de menús se componen de paneles: con las pestañas a la derecha, **{{MePalette}}**, **{{DockPanelCanvas}}** y la columna **{{DockPanelMenuSide}}**; con el texto debajo (preferencias), la paleta con la jerarquía, el lienzo, **{{MeReiterEigenschaften}}** y **{{MeReiterText}}** - por ejemplo el lienzo grande en una segunda pantalla mientras el texto se queda en la ventana principal.",
    "Troubleshooting":
        "Solución de problemas",
    "A panel is lost or the arrangement is muddled: **{{DockMenu}} > {{DockReset}}** brings the standard layout of the area back.":
        "Un panel se ha perdido o la disposición está revuelta: **{{DockMenu}} > {{DockReset}}** recupera la disposición estándar del área.",
    "Docking inside the main window causes a problem on your computer: start JK Studio with the environment variable `BHED_DOCKSPACE=0` (for example in a command prompt `set BHED_DOCKSPACE=0`, then `jkstudio.exe` from the same prompt). Then every panel stays at its fixed place in the main window again and can only be taken out into its own window, as in earlier versions; dropping a panel window on its old place docks it back. Your layouts are kept.":
        "El acoplamiento dentro de la ventana principal da problemas en tu ordenador: inicia JK Studio con la variable de entorno `BHED_DOCKSPACE=0` (por ejemplo, en un símbolo del sistema `set BHED_DOCKSPACE=0` y luego `jkstudio.exe` desde el mismo símbolo del sistema). Entonces cada panel vuelve a quedarse en su sitio fijo de la ventana principal y solo se puede sacar a su propia ventana, como en versiones anteriores; al soltar una ventana de panel en su antiguo sitio, se vuelve a acoplar. Tus disposiciones se conservan.",
    "**{{MeAssistent}}** lists every game setting (cvar) that the original menus of the game offer, with the control they use there: a slider with its range, yes/no, or a choice with its entries. Tick the settings you want and click the button below the search field - the editor adds a settings page with these controls, a title and a back button. The list has three columns - setting, control and text; drag a column border to widen it, a cut-off text shows in full when you point at it.":
        "**{{MeAssistent}}** enumera todos los ajustes del juego (cvars) que ofrecen los menús originales del juego, con el control que usan allí: un deslizador con su rango, sí/no, o una selección con sus entradas. Marca los ajustes que quieras y pulsa el botón bajo el campo de búsqueda - el editor añade una página de ajustes con esos controles, un título y un botón para volver. La lista tiene tres columnas - ajuste, control y texto; arrastra el borde de una columna para ensancharla, y un texto cortado se muestra entero al señalarlo.",
    "Menu editor: game values, reload in the game, components":
        "Editor de menús: valores del juego, recarga en el juego, componentes",
    "Every element that shows values of the running game is now drawn like the game, with sample values you can change; key bindings show real keys; the loading and connect screens look like in the game; a game started with Test in game reloads your menus after saving; components keep copies in sync; models get handles for turning, distance and field of view.":
        "Cada elemento que muestra valores de la partida en curso se dibuja ahora como en el juego, con valores de ejemplo que puedes cambiar; las asignaciones de teclas muestran teclas reales; las pantallas de carga y de conexión se ven como en el juego; un juego iniciado con Probar en el juego recarga tus menús al guardar; los componentes mantienen sus copias sincronizadas; los modelos tienen tiradores para el giro, la distancia y el campo de visión.",
    "Elements with values from the game":
        "Elementos con valores del juego",
    "Some elements (`ownerdraw`) are drawn by the game's code itself: the Force stars and Force points of the multiplayer player menus, the server source and filter, the bot names, the graphics driver page (GL_VENDOR and extensions - the editor reads them from your real OpenGL driver), the datapad with objectives, weapons and Force powers, the Pazaak board of Movie Duels and more. The editor now draws each of them exactly like the engine. Where the game takes the values from the running game or the server, it uses sample values and marks the element with **{{MeOdBeispiel}}** at the top left (only while editing). Change the samples under **Properties > {{MeOdBeispiel}}**: the selected weapon and Force power with its level, the objectives, health, Force and ammo, and for multiplayer the server settings (g_jediVmerc, g_forceBasedTeams, highest Force rank, server refresh). Elements that the game itself draws empty (for example the voice chat entries) stay empty; lists that are filled only during a match (player list) show sample names.":
        "Algunos elementos (`ownerdraw`) los dibuja el propio código del juego: las estrellas y los puntos de Fuerza de los menús de jugador multijugador, la fuente y el filtro de servidores, los nombres de los bots, la página del controlador gráfico (GL_VENDOR y extensiones - el editor los lee de tu controlador OpenGL real), el datapad con objetivos, armas y poderes de la Fuerza, el tablero de Pazaak de Movie Duels y más. El editor dibuja ahora cada uno exactamente como el motor. Donde el juego toma los valores de la partida en curso o del servidor, usa valores de ejemplo y marca el elemento con **{{MeOdBeispiel}}** arriba a la izquierda (solo mientras editas). Cambia los ejemplos en **Propiedades > {{MeOdBeispiel}}**: el arma elegida y el poder de la Fuerza con su nivel, los objetivos, la salud, la Fuerza y la munición, y para multijugador los ajustes del servidor (g_jediVmerc, g_forceBasedTeams, rango de Fuerza máximo, actualización de servidores). Los elementos que el propio juego dibuja vacíos (por ejemplo las entradas del chat de voz) siguen vacíos; las listas que solo se llenan durante una partida (lista de jugadores) muestran nombres de ejemplo.",
    "The datapad weapons page of Jedi Academy with sample weapons - HUD ticks, frame, icons and description as in the game.":
        "La página de armas del datapad de Jedi Academy con armas de ejemplo - marcas del HUD, marco, iconos y descripción como en el juego.",
    "Jedi Outcast and Movie Duels draw their own datapad: health, armour, Force and ammo rings of the Jedi Outcast HUD, an icon strip with prongs behind the weapons, inventory and Force powers, and in Movie Duels smaller icons with one neighbour on each side, its 25 Force powers and its own texts. The preview follows the game whose data is loaded.":
        "Jedi Outcast y Movie Duels dibujan su propio datapad: los anillos de salud, blindaje, Fuerza y munición del HUD de Jedi Outcast, una tira de iconos con puntas detrás de las armas, el inventario y los poderes de la Fuerza, y en Movie Duels iconos más pequeños con un vecino a cada lado, sus 25 poderes de la Fuerza y sus propios textos. La vista previa sigue al juego cuyos datos están cargados.",
    "**Key bindings**: the key binding elements of the controls menus now show the keys the game binds by default (from the game's default config, for example `default.cfg` or `mpdefault.cfg`), with the game's key names - instead of `???`. Commands the game does not list stay `???`, as in the game.":
        "**Asignaciones de teclas**: los elementos de asignación de teclas de los menús de controles muestran ahora las teclas que el juego asigna por defecto (de la configuración por defecto del juego, por ejemplo `default.cfg` o `mpdefault.cfg`), con los nombres de tecla del juego - en lugar de `???`. Los comandos que el juego no incluye siguen en `???`, como en el juego.",
    "Loading and connect screens":
        "Pantallas de carga y de conexión",
    "The single player loading screen with level shot, weapons and Force powers from the sample, loading bar and map name.":
        "La pantalla de carga de un jugador con imagen del nivel, armas y poderes de la Fuerza del ejemplo, barra de carga y nombre del mapa.",
    "The `loadscreen` menu of single player and the `Connect` menu of multiplayer are only part of what the game shows: while loading, the game draws the level shot, the weapon and Force icons of the saved game, the loading bar and the map name over it; while connecting, it writes map, server and connection state. The editor draws all of this like the game. Under **Properties > {{MeOdBeispiel}}** you choose the map, the map message, the loading progress and the special first map (black screen with a quote), or for multiplayer the server, the map, the connection state, the packets and the message. The switch {{MeHudWie}} shows the plain menu instead.":
        "El menú `loadscreen` de un jugador y el menú `Connect` de multijugador son solo parte de lo que muestra el juego: al cargar, el juego dibuja encima la imagen del nivel, los iconos de armas y de Fuerza de la partida guardada, la barra de carga y el nombre del mapa; al conectar, escribe el mapa, el servidor y el estado de la conexión. El editor dibuja todo esto como el juego. En **Propiedades > {{MeOdBeispiel}}** eliges el mapa, el texto del mapa, el progreso de carga y el primer mapa especial (pantalla negra con una cita), o para multijugador el servidor, el mapa, el estado de la conexión, los paquetes y el mensaje. El interruptor {{MeHudWie}} muestra en su lugar el menú solo.",
    "Reload in the running game":
        "Recargar en el juego en marcha",
    "**{{MeImSpielTesten}}** now puts your menus as loose files into its own test folder (never into the game folder) when it can reload them in the running game: with OpenJK and Movie Duels. The original programs of Jedi Academy and Jedi Outcast know `ui_load` too, but they ignore keys sent to a window in the background - for them the test keeps using the .pk3 and a new start. While that game runs, every save writes the files again and gives `ui_load` to the game: it closes its menus, reads the menu set again and opens the last menu - without restarting. The editor only sends keys to the window of the game it started itself.":
        "**{{MeImSpielTesten}}** pone ahora tus menús como archivos sueltos en su propia carpeta de prueba (nunca en la carpeta del juego) cuando puede recargarlos en el juego en marcha: con OpenJK y Movie Duels. Los programas originales de Jedi Academy y Jedi Outcast también conocen `ui_load`, pero ignoran las teclas enviadas a una ventana en segundo plano - para ellos la prueba sigue usando el .pk3 y un nuevo inicio. Mientras ese juego está en marcha, cada guardado vuelve a escribir los archivos y le da `ui_load` al juego: cierra sus menús, vuelve a leer el conjunto de menús y abre el último menú - sin reiniciar. El editor solo envía teclas a la ventana del juego que él mismo inició.",
    "Components":
        "Componentes",
    "A button as component with one linked copy - the Properties show the original and its copies.":
        "Un botón como componente con una copia vinculada - las Propiedades muestran el original y sus copias.",
    "Select an element and click **{{MeKompAnlegen}}** in the Properties: it becomes the original of a component. **{{MeKompKopie}}** inserts a linked copy next to it. Move the copy wherever you like - it keeps its own name and position; everything else follows the original: change the text, colour or size of the original and every copy changes with it (one Undo step). In the .menu file original and copies stay ordinary itemDefs with a comment line before them (`// jkstudio:komponente <name>`, `// jkstudio:instanz <name>`); the game reads them like any other element.":
        "Selecciona un elemento y pulsa **{{MeKompAnlegen}}** en las Propiedades: se convierte en el original de un componente. **{{MeKompKopie}}** inserta una copia vinculada a su lado. Mueve la copia adonde quieras - conserva su propio nombre y posición; todo lo demás sigue al original: cambia el texto, el color o el tamaño del original y cada copia cambia con él (un solo paso de Deshacer). En el archivo .menu, el original y las copias siguen siendo itemDefs normales con una línea de comentario delante (`// jkstudio:komponente <name>`, `// jkstudio:instanz <name>`); el juego los lee como cualquier otro elemento.",
    "Aligning models":
        "Alinear modelos",
    "The buttons of a selected model at its top left: turn, distance, height, field of view.":
        "Los botones de un modelo elegido, arriba a la izquierda: girar, distancia, altura, campo de visión.",
    "A selected model element gets four large buttons at its top left, beside the resize handles (above the element if it is too small). Drag the first sideways to turn the model (`model_angle`: 180 faces the viewer, 0 turns away), the second up or down to move it farther away or closer, the third to raise or lower it, and the fourth sideways to change the field of view (`model_fovx`, `model_fovy`). A new model stands fully visible from the front, with values from the game's own menus. Important: `model_g2anim` only plays if it comes before `asset_model` in the file (with `isCharacter` anywhere); otherwise the game shows the T-pose, and so does the preview.":
        "Un elemento de modelo elegido tiene cuatro botones grandes arriba a la izquierda, junto a los tiradores de tamaño (encima del elemento si es demasiado pequeño). Arrastra el primero en horizontal para girar el modelo (`model_angle`: 180 mira al espectador, 0 le da la espalda), el segundo arriba o abajo para alejarlo o acercarlo, el tercero para subirlo o bajarlo y el cuarto en horizontal para cambiar el campo de visión (`model_fovx`, `model_fovy`). Un modelo nuevo queda de frente y totalmente visible, con valores de los menús del propio juego. Importante: `model_g2anim` solo se reproduce si va antes de `asset_model` en el archivo (`isCharacter` en cualquier lugar); si no, el juego muestra la pose en T, y la vista previa también.",
    "**New projects** can start in multiplayer format: choose **{{MePrjSp}}** or **{{MePrjMp}}** before **{{MePrjNeu}}**.":
        "Los **proyectos nuevos** pueden empezar en formato multijugador: elige **{{MePrjSp}}** o **{{MePrjMp}}** antes de **{{MePrjNeu}}**.",
    "Checked against the game":
        "Comprobado con el juego",
    "The whole-menu templates were photographed in Movie Duels, in the original Jedi Academy and in Jedi Outcast and compared with the preview. Sliders have the size of each game: 128 wide in Jedi Academy and Jedi Outcast single player, 96 in Movie Duels and in multiplayer. Jedi Outcast treats every character after `^` as a colour (`^B` is green) and counts colour codes in the text width - the preview does the same, so centred and right-aligned texts sit where the game puts them.":
        "Las plantillas de menú completo se fotografiaron en Movie Duels, en el Jedi Academy original y en Jedi Outcast y se compararon con la vista previa. Los deslizadores tienen el tamaño de cada juego: 128 de ancho en Jedi Academy y Jedi Outcast para un jugador, 96 en Movie Duels y en multijugador. Jedi Outcast trata cada carácter después de `^` como un color (`^B` es verde) y cuenta los códigos de color en el ancho del texto - la vista previa hace lo mismo, de modo que los textos centrados y alineados a la derecha quedan donde los pone el juego.",
    # Abschluss rc607: Bilder der Tutorial-Schritte
    "Step 1 in the program: the tutorial highlights where to go next.":
        "Paso 1 en el programa: el tutorial resalta dónde seguir.",
    "Step 2 in the program: the tutorial highlights where to go next.":
        "Paso 2 en el programa: el tutorial resalta dónde seguir.",
    "Step 3 in the program: the tutorial highlights where to go next.":
        "Paso 3 en el programa: el tutorial resalta dónde seguir.",
    "Step 4 in the program: the tutorial highlights where to go next.":
        "Paso 4 en el programa: el tutorial resalta dónde seguir.",
    "Step 5 in the program: the tutorial highlights where to go next.":
        "Paso 5 en el programa: el tutorial resalta dónde seguir.",
    "Step 6 in the program: the tutorial highlights where to go next.":
        "Paso 6 en el programa: el tutorial resalta dónde seguir.",
    "Step 7 in the program: the tutorial highlights where to go next.":
        "Paso 7 en el programa: el tutorial resalta dónde seguir.",
    # Abschluss rc607: Untermenue neben der Spalte, deformVertexes move, Andocken ohne Platz
    "**Menus taller than the screen.** A menu with many entries (for example **{{KeLBuild}}** with a long build list) can be taller than the screen. In the style of JK Studio it then scrolls with the mouse wheel; with {{KeMenueNrc}} it wraps into columns like in NetRadiant-Custom, so every entry stays on the screen; a submenu opens beside its column, as in Qt.":
        "**Menús más altos que la pantalla.** Un menú con muchas entradas (por ejemplo **{{KeLBuild}}** con una lista de compilación larga) puede ser más alto que la pantalla. Con el estilo de JK Studio se desplaza entonces con la rueda del ratón; con {{KeMenueNrc}} se reparte en columnas como en NetRadiant-Custom, para que cada entrada quede en la pantalla; un submenú se abre junto a su columna, como en Qt.",
    "With `-meta` (the default of the Build menu) q3map2 puts the faces together anew; the editor does the same and follows the BSP step of the Build menu entry you built last. A mark takes only the first 64 faces in its box - the same ones as in the game.":
        "Con `-meta` (el valor predeterminado del menú Build) q3map2 vuelve a componer las caras; el editor hace lo mismo y se guía por el paso BSP de la última entrada compilada del menú Build. Una marca solo toma las primeras 64 caras de su caja, las mismas que en el juego.",
    "**Decals:** impact and scorch marks of the effects land on the faces the game would mark: like q3map2, the editor builds the world of the map with its BSP tree - only the visible part of each brush face, the models of `misc_model` baked into the world, and only faces next to an open leaf of the tree (no mark on the floor hidden under a step). On a large map this takes a few seconds like a build; it runs in the background, and until it is done the last world counts. Faces whose shader moves its vertices (`deformVertexes move`) count over the whole box of the movement, as q3map2 sorts them into the tree.":
        "**Decals:** las marcas de impacto y de quemadura de los efectos caen en las caras que marcaría el juego: como q3map2, el editor construye el mundo del mapa con su árbol BSP - solo la parte visible de cada cara de brush, los modelos de `misc_model` integrados en el mundo, y solo las caras junto a una hoja abierta del árbol (ninguna marca en el suelo oculto bajo un escalón). En un mapa grande esto tarda unos segundos, como una compilación; se hace en segundo plano y, hasta que termina, cuenta el último mundo. Las caras cuyo shader mueve sus vértices (`deformVertexes move`) cuentan en toda la caja del movimiento, como las ordena q3map2 en el árbol.",
    "Close its window with its **x** - the panel goes back to its place, nothing is lost. If that place is gone because you rearranged the area, it docks beside its neighbour and your arrangement stays.":
        "Cierra su ventana con su **x** - el panel vuelve a su sitio, no se pierde nada. Si ese sitio ya no existe porque has reorganizado el área, se acopla junto a su vecino y tu disposición se conserva.",
}
